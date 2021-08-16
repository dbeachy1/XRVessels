/**
  XR Vessel add-ons for OpenOrbiter Space Flight Simulator
  Copyright (C) 2006-2021 Douglas Beachy

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.

  Email: mailto:doug.beachy@outlook.com
  Web: https://www.alteaaerospace.com
**/

// ==============================================================
// Class implementing the XRVesselCtrl interface.
// These classes extend and use the XR Framework classes
//
// XR1VesselCtrl.cpp
// ==============================================================

#include "DeltaGliderXR1.h"
#include "XRPayloadBay.h"
#include "XRPayloadBaySlot.h"

// utility macros
#define LOWER_LIMIT(x, lim)  if (x < lim) x = lim
#define UPPER_LIMIT(x, lim)  if (x > lim) x = lim
#define LIMITS(x, low, high) if (x < low) x = low; else if (x > high) x = high

// NOTE: none of these methods perform any significant operations themselves on the internal state of the XR1: they call internal XR1 methods
// to do any "heavy lifting."  None of the other XR1 methods invoke any methods in this file; in other words, these methods are not required
// for operation of the XR1.  They are separate and stand-alone.

// Engine State: these methods return TRUE if XREngineID is valid for this ship and (for Set) the operation succeeded; otherwise, the methods do nothing.
// Remember that not all engines support all fields in XREngineStateWrite and not all ships support all engine types in XREngineID.
bool DeltaGliderXR1::SetEngineState(XREngineID id, const XREngineStateWrite &state)
{
    // make writable clone so we can limit the values
    XREngineStateWrite s;
    memcpy(&s, &state, sizeof(state));

    // keep structure values in range
    LIMITS(s.ThrottleLevel, 0, 1.0);
    LIMITS(s.GimbalX, -0.99, 0.99);  // note: gimbal code expects range to never reach 1.0 or -1.0
    LIMITS(s.GimbalY, -0.99, 0.99);
    LIMITS(s.Balance, -1.0, 1.0);

    int idx = 1;        // engine index; defaults to RIGHT / AFT
    switch (id)
    {
    case XREngineID::XRE_RetroLeft:
        idx = 0;
    case XREngineID::XRE_RetroRight:
        {
            // check for unsupported options
            if ((s.GimbalX != 0) || (s.GimbalY != 0) || (s.Balance != 0) || 
                s.CenteringModeX || s.CenteringModeBalance || s.AutoMode || s.DivergentMode)
                return false;

            // check the retro doors
            if (m_isRetroEnabled == false)
            {
                PlaySound(RetroDoorsAreClosed, DeltaGliderXR1::ST_WarningCallout);
                ShowWarning(NULL, DeltaGliderXR1::ST_None, "Retro Doors are closed.");
                return false;
            }

            // all fields OK
            SetThrusterLevel(th_retro[idx], s.ThrottleLevel);
            break;
        }

    case XREngineID::XRE_MainLeft:
        idx = 0;
    case XREngineID::XRE_MainRight:
        {
            // custom balance is not supported
            if ((s.Balance != 0.0) || s.CenteringModeBalance)
                return false;

            // all fields OK
            SetThrusterLevel(th_main[idx], s.ThrottleLevel);

            if (CheckHydraulicPressure(false, false))  // APU online?  (do not play a warning here)
            {
                VECTOR3 dir;
                GetThrusterDir(th_main[idx], dir);
                dir /= dir.z;
                dir.x = MAIN_YGIMBAL_RANGE * s.GimbalX;  // yaw
                dir.y = MAIN_PGIMBAL_RANGE * s.GimbalY;  // pitch
                SetThrusterDir(th_main[idx], dir);

                m_mainYawCenteringMode   = s.CenteringModeX;
                m_mainPitchCenteringMode = s.CenteringModeY;
                m_mainAutoMode = s.AutoMode;
                m_mainDivMode  = s.DivergentMode;
            }

            break;
        }

    case XREngineID::XRE_HoverFore:
        idx = 0;
    case XREngineID::XRE_HoverAft:
        {
            // gimbaling/auto/divergent not supported
            if ((s.GimbalX != 0) || (s.GimbalY != 0) || s.CenteringModeX || s.CenteringModeY || s.AutoMode || s.DivergentMode)
                return false;

            // check the hover doors
            if (m_isHoverEnabled == false)
            {
                PlaySound(HoverDoorsAreClosed, DeltaGliderXR1::ST_WarningCallout);
                ShowWarning(NULL, DeltaGliderXR1::ST_None, "Hover Doors are closed.");
                return false;
            }

            // all fields OK
            SetThrusterLevel(th_hover[idx], s.ThrottleLevel);
            m_hoverCenteringMode = s.CenteringModeBalance;  // set in XR1

            // must take damage into account here to set balance
            if (CheckHydraulicPressure(false, false))  // APU online?  (do not play a warning here)
            {
                m_hoverBalance = s.Balance * MAX_HOVER_IMBALANCE;     // set in XR1
                const int hoverThrustIdx   = GetXR1Config()->HoverEngineThrust;
                const double maxThrustFore = MAX_HOVER_THRUST[hoverThrustIdx] * GetDamageStatus(DamageItem::HoverEngineFore).fracIntegrity;
                const double maxThrustAft  = MAX_HOVER_THRUST[hoverThrustIdx] * GetDamageStatus(DamageItem::HoverEngineAft).fracIntegrity;
                SetThrusterMax0(th_hover[0], maxThrustFore * (1.0 + m_hoverBalance));
                SetThrusterMax0(th_hover[1], maxThrustAft *  (1.0 - m_hoverBalance));
            }

            break;
        }

    case XREngineID::XRE_ScramLeft:
        idx = 0;
    case XREngineID::XRE_ScramRight:
        {
            // check for unsupported options
            if ((s.Balance != 0.0) || (s.GimbalX != 0) || (s.Balance != 0) || 
                s.CenteringModeX || s.CenteringModeBalance || s.AutoMode || s.DivergentMode)
                return false;

            // make sure the SCRAM doors are enabled
            if (m_isScramEnabled == false)
            {
                PlaySound(ScramDoorsAreClosed, DeltaGliderXR1::ST_WarningCallout);
                ShowWarning(NULL, DeltaGliderXR1::ST_None, "SCRAM Doors are closed.");
                return false;
            }

            // all fields OK
            SetThrusterLevel(th_scram[idx], s.ThrottleLevel);

            if (CheckHydraulicPressure(false, false))  // APU online?  (do not play a warning here)
            {
                double phi = SCRAM_DEFAULT_DIR + (SCRAM_GIMBAL_RANGE * s.GimbalY);
                SetThrusterDir(th_scram[idx], _V(0, sin(phi), cos(phi)));
                m_scramCenteringMode = s.CenteringModeY;
            }

            break;
        }

    default:
        return false;   // unknown engine ID (client code error!)

    }

    return true;

}

bool DeltaGliderXR1::GetEngineState(XREngineID id, XREngineStateRead &state) const
{
    int idx = 1;        // engine index; defaults to RIGHT / AFT
    switch (id)
    {
    case XREngineID::XRE_RetroLeft:
        idx = 0;
    case XREngineID::XRE_RetroRight:
        {
            const THRUSTER_HANDLE th = th_retro[idx];
            const double thrusterLevel = state.ThrottleLevel = GetThrusterLevel(th);

            state.GimbalX              = 0;   // yaw
            state.GimbalY              = 0;   // pitch
            state.Balance              = 0;
            state.CenteringModeX       = false;
            state.CenteringModeY       = false;
            state.CenteringModeBalance = false;
            state.AutoMode             = false;
            state.DivergentMode        = false;

            // read-only data
            state.TSFC         = 1000 / GetThrusterIsp(th);
            state.FlowRate     = GetThrusterFlowRate(th);
            state.Thrust       = thrusterLevel * GetThrusterMax(th);
            state.FuelLevel    = SAFE_FRACTION(GetXRPropellantMass(ph_main), GetXRPropellantMaxMass(ph_main));
            state.MaxFuelMass  = GetXRPropellantMaxMass(ph_main);
            state.BayFuelMass  = GetXRBayPropellantMass(ph_main);
            state.DiffuserTemp = state.BurnerTemp = state.ExhaustTemp = -1;     // temperatures unsupported
            break;
        }

    case XREngineID::XRE_MainLeft:
        idx = 0;
    case XREngineID::XRE_MainRight:
        {
            const THRUSTER_HANDLE th = th_main[idx];
            const double thrusterLevel = state.ThrottleLevel = GetThrusterLevel(th);

            VECTOR3 dir;
            GetThrusterDir(th, dir);
            state.GimbalX = dir.x / MAIN_YGIMBAL_RANGE; // yaw
            state.GimbalY = dir.y / MAIN_PGIMBAL_RANGE; // pitch

            state.Balance = 0;
            state.CenteringModeX       = m_mainYawCenteringMode;
            state.CenteringModeY       = m_mainPitchCenteringMode;
            state.CenteringModeBalance = false;
            state.AutoMode             = m_mainAutoMode;
            state.DivergentMode        = m_mainDivMode;

            // read-only data
            state.TSFC         = 1000 / GetThrusterIsp(th);
            state.FlowRate     = GetThrusterFlowRate(th);
            state.Thrust       = thrusterLevel * GetThrusterMax(th);
            state.FuelLevel    = SAFE_FRACTION(GetXRPropellantMass(ph_main), GetXRPropellantMaxMass(ph_main));
            state.MaxFuelMass  = GetXRPropellantMaxMass(ph_main);
            state.BayFuelMass  = GetXRBayPropellantMass(ph_main);
            state.DiffuserTemp = state.BurnerTemp = state.ExhaustTemp = -1;     // temperatures unsupported
            break;
        }

    case XREngineID::XRE_HoverFore:
        idx = 0;
    case XREngineID::XRE_HoverAft:
        {
            const THRUSTER_HANDLE th = th_hover[idx];
            const double thrusterLevel = state.ThrottleLevel = GetThrusterLevel(th);

            state.GimbalX              = 0;
            state.GimbalY              = 0;   
            state.Balance              = m_hoverBalance / MAX_HOVER_IMBALANCE;
            state.CenteringModeX       = false;
            state.CenteringModeY       = false;
            state.CenteringModeBalance = m_hoverCenteringMode;
            state.AutoMode             = false;
            state.DivergentMode        = false;

            // read-only data
            state.TSFC         = 1000 / GetThrusterIsp(th);
            state.FlowRate     = GetThrusterFlowRate(th);
            state.Thrust       = thrusterLevel * GetThrusterMax(th);
            state.FuelLevel    = SAFE_FRACTION(GetXRPropellantMass(ph_main), GetXRPropellantMaxMass(ph_main));
            state.MaxFuelMass  = GetXRPropellantMaxMass(ph_main);
            state.BayFuelMass  = GetXRBayPropellantMass(ph_main);
            state.DiffuserTemp = state.BurnerTemp = state.ExhaustTemp = -1;     // temperatures unsupported
            break;
        }

    case XREngineID::XRE_ScramLeft:
        idx = 0;
    case XREngineID::XRE_ScramRight:
        {
            const THRUSTER_HANDLE th = th_scram[idx];
            const double thrusterLevel = state.ThrottleLevel = GetThrusterLevel(th);

            state.GimbalX              = 0;   // yaw
            VECTOR3 dir;
            GetThrusterDir(th, dir);
            state.GimbalY              = dir.y / (SCRAM_DEFAULT_DIR + SCRAM_GIMBAL_RANGE);  // pitch
            state.Balance              = 0;
            state.CenteringModeX       = false;
            state.CenteringModeY       = m_scramCenteringMode;  // pitch
            state.CenteringModeBalance = false;
            state.AutoMode             = false;
            state.DivergentMode        = false;

            // read-only data
            XR1Ramjet::THDEF *pThdef = ramjet->thdef[idx];    // SCRAMjet thruster definition data
            state.TSFC         = ramjet->TSFC(idx);
            state.FlowRate     = pThdef->dmf;   // kg/sec
            state.Thrust       = pThdef->F;
            state.FuelLevel    = SAFE_FRACTION(GetXRPropellantMass(ph_scram), GetXRPropellantMaxMass(ph_scram));
            state.MaxFuelMass  = GetXRPropellantMaxMass(ph_scram);
            state.BayFuelMass  = GetXRBayPropellantMass(ph_scram);
            // show visual temperatures here, not actual internal ones (i.e., don't use T[n] / SCRAM_COOLING directly)
            state.DiffuserTemp = ramjet->Temp(idx, 0);
            state.BurnerTemp = ramjet->Temp(idx, 1);
            state.ExhaustTemp = ramjet->Temp(idx, 2);
            break;
        }

    default:
        return false;   // unknown engine ID (client code error!)

    }

    return true;
}

// Door State
// returns TRUE if door/state combination is valid for this ship
bool DeltaGliderXR1::SetDoorState(XRDoorID id, XRDoorState state)            
{
    // NOTE: you cannot fail a door via SetDoorState: must use SetXRSystemStatus instead
    if (state == XRDoorState::XRDS_Failed)
        return false;   // invalid state

    bool retVal = true;

    // Note: each of these calls updates each door's proc as well ('percent open' state)
    switch (id)
    {
    case XRDoorID::XRD_DockingPort:
        ActivateNoseCone(ToDoorStatus(state));
        break;

    case XRDoorID::XRD_ScramDoors:
        ActivateScramDoors(ToDoorStatus(state));
        break;

    case XRDoorID::XRD_HoverDoors:
        ActivateHoverDoors(ToDoorStatus(state));
        break;

    case XRDoorID::XRD_Ladder:
        ActivateLadder(ToDoorStatus(state));
        break;  

    case XRDoorID::XRD_Gear:
        ActivateLandingGear(ToDoorStatus(state));
        break;

    case XRDoorID::XRD_RetroDoors:
        ActivateRCover(ToDoorStatus(state));
        break;

    case XRDoorID::XRD_OuterAirlock:
        ActivateOuterAirlock(ToDoorStatus(state));
        break;

    case XRDoorID::XRD_InnerAirlock:
        ActivateInnerAirlock(ToDoorStatus(state));
        break;  

    case XRDoorID::XRD_AirlockChamber:
        ActivateChamber(ToDoorStatus(state), false);
        break;

    case XRDoorID::XRD_CrewHatch:
        ActivateHatch(ToDoorStatus(state));
        break;

    case XRDoorID::XRD_Radiator:
        ActivateRadiator(ToDoorStatus(state));
        break;

    case XRDoorID::XRD_Speedbrake:
        ActivateAirbrake(ToDoorStatus(state));
        break;

    case XRDoorID::XRD_APU:
        ActivateAPU(ToDoorStatus(state));
        break;

    default:
        retVal = false;
        break;
    }

    return retVal;
}

#define SET_IF_NOT_NULL(ptr, value) if (ptr != nullptr) *ptr = value;

// Returns XRDS_DoorNotSupported if door does not exist for this ship; if pProc != nullptr, proc is set to 0 <= n <= 1.0 *unless* the door is not supported,
// in which case proc is set to -1.
XRDoorState DeltaGliderXR1::GetDoorState(XRDoorID id, double *pProc)  const
{
    XRDoorState retVal;

    switch (id)
    {
    case XRDoorID::XRD_DockingPort:
        retVal = ToXRDoorState(nose_status);
        SET_IF_NOT_NULL(pProc, nose_proc);
        break;

    case XRDoorID::XRD_ScramDoors:
        retVal = ToXRDoorState(scramdoor_status);
        SET_IF_NOT_NULL(pProc, scramdoor_proc);
        break;

    case XRDoorID::XRD_HoverDoors:
        retVal = ToXRDoorState(hoverdoor_status);
        SET_IF_NOT_NULL(pProc, hoverdoor_proc);
        break;

    case XRDoorID::XRD_Ladder:
        retVal = ToXRDoorState(ladder_status);
        SET_IF_NOT_NULL(pProc, ladder_proc);
        break;  

    case XRDoorID::XRD_Gear:
        retVal = ToXRDoorState(gear_status);
        SET_IF_NOT_NULL(pProc, gear_proc);
        break;

    case XRDoorID::XRD_RetroDoors:
        retVal = ToXRDoorState(rcover_status);
        SET_IF_NOT_NULL(pProc, rcover_proc);
        break;

    case XRDoorID::XRD_OuterAirlock:
        retVal = ToXRDoorState(olock_status);
        SET_IF_NOT_NULL(pProc, olock_proc);
        break;

    case XRDoorID::XRD_InnerAirlock:
        retVal = ToXRDoorState(ilock_status);
        SET_IF_NOT_NULL(pProc, ilock_proc);
        break;  

    case XRDoorID::XRD_AirlockChamber:
        retVal = ToXRDoorState(chamber_status);
        SET_IF_NOT_NULL(pProc, chamber_proc);
        break;

    case XRDoorID::XRD_CrewHatch:
        retVal = ToXRDoorState(hatch_status);
        SET_IF_NOT_NULL(pProc, hatch_proc);
        break;

    case XRDoorID::XRD_Radiator:
        retVal = ToXRDoorState(radiator_status);
        SET_IF_NOT_NULL(pProc, radiator_proc);
        break;

    case XRDoorID::XRD_Speedbrake:
        retVal = ToXRDoorState(brake_status);
        SET_IF_NOT_NULL(pProc, brake_proc);
        break;

    case XRDoorID::XRD_APU:
        retVal = ToXRDoorState(apu_status);
        SET_IF_NOT_NULL(pProc, -1);  // no proc for this, so proc state always == -1
        break;

    default:
        retVal = XRDoorState::XRDS_DoorNotSupported;
        SET_IF_NOT_NULL(pProc, -1);
        break;
    }

    return retVal;
}

// Repairs all damaged systems
// Returns true if this call is supported by this vessel
bool DeltaGliderXR1::ClearAllXRDamage()
{
    ResetDamageStatus();
    return true;
}

// Set the damage status of the XR Vessel; any unsupported fields in 'status' must be set to -1 (for doubles) or XRDMG_NotSupported (for XRDamageState)
bool DeltaGliderXR1::SetXRSystemStatus(const XRSystemStatusWrite &status)
{
    // Since we never CLEAR a damage light in a single SetDamageStatus call, we must first 
    // clear all damage items (and lights) before resetting them.
    ClearAllXRDamage();

    // Now apply the new status one item at a time
#define SET_DMG_INT(statusField, XREnum)  { double val = status.##statusField; LIMITS(val, 0, 1.0); SetDamageStatus(XREnum, val); }
#define SET_DMG_ENUM(statusField, XREnum)                       \
     {                                                          \
        double val;                                             \
        XRDamageState state = status.##statusField;             \
        if (state == XRDamageState::XRDMG_online) val = 1.0;    \
        else val = 0.0;  /* unsupported or offline */           \
        SetDamageStatus(XREnum, val);                           \
    }

    //          status.##field               DamageItem
    SET_DMG_INT(LeftWing,                    DamageItem::LeftWing);
    SET_DMG_INT(RightWing,                   DamageItem::RightWing);
    SET_DMG_INT(LeftMainEngine,              DamageItem::MainEngineLeft);
    SET_DMG_INT(RightMainEngine,             DamageItem::MainEngineRight);
    SET_DMG_INT(LeftSCRAMEngine,             DamageItem::SCRAMEngineLeft);
    SET_DMG_INT(RightSCRAMEngine,            DamageItem::SCRAMEngineRight);
    SET_DMG_INT(ForeHoverEngine,             DamageItem::HoverEngineFore);   // these are *logical* engines
    SET_DMG_INT(AftHoverEngine,              DamageItem::HoverEngineAft);
    SET_DMG_INT(LeftRetroEngine,             DamageItem::RetroEngineLeft);
    SET_DMG_INT(RightRetroEngine,            DamageItem::RetroEngineRight);
    SET_DMG_INT(ForwardLowerRCS,             DamageItem::RCS1);
    SET_DMG_INT(AftUpperRCS,                 DamageItem::RCS2);
    SET_DMG_INT(ForwardUpperRCS,             DamageItem::RCS3);
    SET_DMG_INT(AftLowerRCS,                 DamageItem::RCS4);
    SET_DMG_INT(ForwardStarboardRCS,         DamageItem::RCS5);
    SET_DMG_INT(AftPortRCS,                  DamageItem::RCS6);
    SET_DMG_INT(ForwardPortRCS,              DamageItem::RCS7);
    SET_DMG_INT(AftStarboardRCS,             DamageItem::RCS8);
    SET_DMG_INT(OutboardUpperPortRCS,        DamageItem::RCS9);
    SET_DMG_INT(OutboardLowerStarboardRCS,   DamageItem::RCS10);
    SET_DMG_INT(OutboardUpperStarboardRCS,   DamageItem::RCS11);
    SET_DMG_INT(OutboardLowerPortRCS,        DamageItem::RCS12);
    SET_DMG_INT(AftRCS,                      DamageItem::RCS13);
    SET_DMG_INT(ForwardRCS,                  DamageItem::RCS14);

    //          status.##field  DamageItem
    SET_DMG_ENUM(LeftAileron,   DamageItem::LeftAileron);
    SET_DMG_ENUM(RightAileron,  DamageItem::RightAileron);
    SET_DMG_ENUM(LandingGear,   DamageItem::LandingGear);
    SET_DMG_ENUM(DockingPort,   DamageItem::Nosecone);
    SET_DMG_ENUM(RetroDoors,    DamageItem::RetroDoors);
    SET_DMG_ENUM(TopHatch,      DamageItem::Hatch);
    SET_DMG_ENUM(Radiator,      DamageItem::Radiator);
    SET_DMG_ENUM(Speedbrake,    DamageItem::Airbrake);

    // check whether the user is attempting to set any unsupported fields
    bool retVal = true;
    retVal &= !(status.PayloadBayDoors != XRDamageState::XRDMG_NotSupported);
    retVal &= !(status.CrewElevator    != XRDamageState::XRDMG_NotSupported);

    return retVal;  // all fields set successfully
}

// Read the status of the XR vessel
void DeltaGliderXR1::GetXRSystemStatus(XRSystemStatusRead &status) const
{
    status.LeftWing                     = GetDamageStatus(DamageItem::LeftWing).fracIntegrity;
    status.RightWing                    = GetDamageStatus(DamageItem::RightWing).fracIntegrity;
    status.LeftMainEngine               = GetDamageStatus(DamageItem::MainEngineLeft).fracIntegrity;
    status.RightMainEngine              = GetDamageStatus(DamageItem::MainEngineRight).fracIntegrity;
    status.LeftSCRAMEngine              = GetDamageStatus(DamageItem::SCRAMEngineLeft).fracIntegrity;
    status.RightSCRAMEngine             = GetDamageStatus(DamageItem::SCRAMEngineRight).fracIntegrity;
    status.ForeHoverEngine              = GetDamageStatus(DamageItem::HoverEngineFore).fracIntegrity;   // these are *logical* engines
    status.AftHoverEngine               = GetDamageStatus(DamageItem::HoverEngineAft).fracIntegrity;
    status.LeftRetroEngine              = GetDamageStatus(DamageItem::RetroEngineLeft).fracIntegrity;
    status.RightRetroEngine             = GetDamageStatus(DamageItem::RetroEngineRight).fracIntegrity;
    status.ForwardLowerRCS              = GetDamageStatus(DamageItem::RCS1).fracIntegrity;
    status.AftUpperRCS                  = GetDamageStatus(DamageItem::RCS2).fracIntegrity;
    status.ForwardUpperRCS              = GetDamageStatus(DamageItem::RCS3).fracIntegrity;
    status.AftLowerRCS                  = GetDamageStatus(DamageItem::RCS4).fracIntegrity;
    status.ForwardStarboardRCS          = GetDamageStatus(DamageItem::RCS5).fracIntegrity;
    status.AftPortRCS                   = GetDamageStatus(DamageItem::RCS6).fracIntegrity;
    status.ForwardPortRCS               = GetDamageStatus(DamageItem::RCS7).fracIntegrity;
    status.AftStarboardRCS              = GetDamageStatus(DamageItem::RCS8).fracIntegrity;
    status.OutboardUpperPortRCS         = GetDamageStatus(DamageItem::RCS9).fracIntegrity;
    status.OutboardLowerStarboardRCS    = GetDamageStatus(DamageItem::RCS10).fracIntegrity;
    status.OutboardUpperStarboardRCS    = GetDamageStatus(DamageItem::RCS11).fracIntegrity;
    status.OutboardLowerPortRCS         = GetDamageStatus(DamageItem::RCS12).fracIntegrity;
    status.AftRCS                       = GetDamageStatus(DamageItem::RCS13).fracIntegrity;
    status.ForwardRCS                   = GetDamageStatus(DamageItem::RCS14).fracIntegrity;
    
    // boolean
    status.LeftAileron                  = ((GetDamageStatus(DamageItem::LeftAileron).fracIntegrity == 1.0) ? XRDamageState::XRDMG_online : XRDamageState::XRDMG_offline);   // includes left elevator if a separate elevator surface is present
    status.RightAileron                 = ((GetDamageStatus(DamageItem::RightAileron).fracIntegrity == 1.0) ? XRDamageState::XRDMG_online : XRDamageState::XRDMG_offline);  // includes right elevator if a separate elevator surface is present
    status.LandingGear                  = ((GetDamageStatus(DamageItem::LandingGear).fracIntegrity == 1.0) ? XRDamageState::XRDMG_online : XRDamageState::XRDMG_offline);
    status.DockingPort                  = ((GetDamageStatus(DamageItem::Nosecone).fracIntegrity == 1.0) ? XRDamageState::XRDMG_online : XRDamageState::XRDMG_offline);      // "nosecone" on some ships
    status.RetroDoors                   = ((GetDamageStatus(DamageItem::RetroDoors).fracIntegrity == 1.0) ? XRDamageState::XRDMG_online : XRDamageState::XRDMG_offline);
    status.TopHatch                     = ((GetDamageStatus(DamageItem::Hatch).fracIntegrity == 1.0) ? XRDamageState::XRDMG_online : XRDamageState::XRDMG_offline);         // "crew hatch" on some ships        
    status.Radiator                     = ((GetDamageStatus(DamageItem::Radiator).fracIntegrity == 1.0) ? XRDamageState::XRDMG_online : XRDamageState::XRDMG_offline);
    status.Speedbrake                   = ((GetDamageStatus(DamageItem::Airbrake).fracIntegrity == 1.0) ? XRDamageState::XRDMG_online : XRDamageState::XRDMG_offline);      // "airbrake" on some ships
    status.PayloadBayDoors              = XRDamageState::XRDMG_NotSupported;   // not supported
    status.CrewElevator                 = XRDamageState::XRDMG_NotSupported;   // not supported

    // The warning states below are not persisted in the scenario file; they are constantly recalculated dynamically.
    status.HullTemperatureWarning   = (m_warningLights[static_cast<int>(WarningLight::wlHtmp)] ? XRWarningState::XRW_warningActive : XRWarningState::XRW_warningInactive);
    status.MainFuelWarning          = (m_warningLights[static_cast<int>(WarningLight::wlMfuel)] ? XRWarningState::XRW_warningActive : XRWarningState::XRW_warningInactive);
    status.RCSFuelWarning           = (m_warningLights[static_cast<int>(WarningLight::wlRfuel)] ? XRWarningState::XRW_warningActive : XRWarningState::XRW_warningInactive);
    status.APUFuelWarning           = (m_apuWarning ? XRWarningState::XRW_warningActive : XRWarningState::XRW_warningInactive);
    status.LOXWarning               = (m_warningLights[static_cast<int>(WarningLight::wlLox)] ? XRWarningState::XRW_warningActive : XRWarningState::XRW_warningInactive);
    status.DynamicPressureWarning   = (m_warningLights[static_cast<int>(WarningLight::wlDynp)] ? XRWarningState::XRW_warningActive : XRWarningState::XRW_warningInactive);
    status.CoolantWarning           = (m_warningLights[static_cast<int>(WarningLight::wlCool)] ? XRWarningState::XRW_warningActive : XRWarningState::XRW_warningInactive);
    status.MasterWarning            = (IsWarningPresent() ? XRWarningState::XRW_warningActive : XRWarningState::XRW_warningInactive);     // warning active if *any* other warning active
    status.MWSLightState            = m_MWSLit;     // updated as the MWS light blinks: true = lit, false = not lit

    // API 2.1 fields
    status.RCSFuelLevel = SAFE_FRACTION(GetXRPropellantMass(ph_rcs), GetXRPropellantMaxMass(ph_rcs));
    status.RCSMaxFuelMass = GetXRPropellantMaxMass(ph_rcs);
    
    status.APUFuelLevel = SAFE_FRACTION(m_apuFuelQty, APU_FUEL_CAPACITY);
    status.APUMaxFuelMass = APU_FUEL_CAPACITY;

    status.LOXLevel = SAFE_FRACTION(GetXRLOXMass(), GetXRLOXMaxMass());
    status.LOXMaxMass = GetXRLOXMaxMass();

    status.BayLOXMass = GetXRBayLOXMass();

    // API 3.0 fields
    status.MWSAlarmState = m_MWSActive;
    status.CenterOfGravity = -(m_centerOfLift - NEUTRAL_CENTER_OF_LIFT);     // positive COL means COG is aft, negative means COG is forward
    status.COGAutoMode = m_cogShiftAutoModeActive;  // true = center-of-gravity shift in auto-mode because the Attitude Hold or Descent Hold autopilot is engaged

    status.CoolantTemp = m_coolantTemp;
    status.InternalSystemsFailure = m_internalSystemsFailure;

    status.NoseconeTemp = m_noseconeTemp;
    status.LeftWingTemp = m_leftWingTemp;
    status.RightWingTemp = m_rightWingTemp;
    status.CockpitTemp = m_cockpitTemp;
    status.TopHullTemp = m_topHullTemp;
    status.CabinO2Level = m_cabinO2Level;

    status.MaxSafeNoseconeTemp = m_hullTemperatureLimits.noseCone;
    status.MaxSafeWingTemp = m_hullTemperatureLimits.wings;
    status.MaxSafeCockpitTemp = m_hullTemperatureLimits.cockpit;
    status.MaxSafeTopHullTemp = m_hullTemperatureLimits.topHull;
 }  

// Kill all autopilots
void DeltaGliderXR1::KillAutopilots()
{
    KillAllAutopilots(); 
}

// Standard Autopilot status (on/off only)
XRAutopilotState DeltaGliderXR1::SetStandardAP(XRStdAutopilot id, bool on)  // returns the new state of the autopilot, or XRAPSTATE_NotSupported if requested autopilot not supported
{
    int navMode = GetNavmodeForXRStdAutopilot(id);
    if (navMode == -1)
        return XRAutopilotState::XRAPSTATE_NotSupported;

    // Note: no need to kill any custom autopilots here; clbkNavMode will handle that
    XRAutopilotState retVal;
    if (on)
    {
        ActivateNavmode(navMode);
        retVal = XRAutopilotState::XRAPSTATE_Engaged;
    }
    else
    {
        DeactivateNavmode(navMode);
        retVal = XRAutopilotState::XRAPSTATE_Disengaged;
    }   

    return retVal;
}

// Note: cannot be const due to core Orbiter API bug: GetNavmodeState is not declared 'const'
XRAutopilotState DeltaGliderXR1::GetStandardAP(XRStdAutopilot id) 
{
    int navMode = GetNavmodeForXRStdAutopilot(id);
    if (navMode == -1)
        return XRAutopilotState::XRAPSTATE_NotSupported;

    bool apOn = GetNavmodeState(navMode);

    return (apOn ? XRAutopilotState::XRAPSTATE_Engaged : XRAutopilotState::XRAPSTATE_Disengaged);
}

// Extended Autopilot methods
XRAutopilotState DeltaGliderXR1::SetAttitudeHoldAP(const XRAttitudeHoldState &state)  // returns the new state of the autopilot, or XRAPSTATE_NotSupported if autopilot not supported
{
    XRAutopilotState retVal;

    // set AP parameters
    m_holdAOA       = (state.mode == XRAttitudeHoldMode::XRAH_HoldPitch ? false : true);
    m_setPitchOrAOA = state.TargetPitch;
    m_setBank       = state.TargetBank;

    if (state.on == false)
    {
        // only modify the custom autopilot mode if this mode is already engaged
        if (m_customAutopilotMode == AUTOPILOT::AP_ATTITUDEHOLD)
            SetCustomAutopilotMode(AUTOPILOT::AP_OFF, true, false);

        retVal = XRAutopilotState::XRAPSTATE_Disengaged;
    }
    else  // set autopilot to ON
    {
        // if autopilot not already engaged, turn it on
        if (m_customAutopilotMode != AUTOPILOT::AP_ATTITUDEHOLD)
            ToggleAttitudeHold();  // use 'toggle' here because we don't have an explicit 'ActivateAttitudeHold' method

        retVal = XRAutopilotState::XRAPSTATE_Engaged;
    }

    return retVal;
}

XRAutopilotState DeltaGliderXR1::GetAttitudeHoldAP(XRAttitudeHoldState &state)  const
{
    state.on          = (m_customAutopilotMode == AUTOPILOT::AP_ATTITUDEHOLD);
    state.mode        = (m_holdAOA ? XRAttitudeHoldMode::XRAH_HoldAOA : XRAttitudeHoldMode::XRAH_HoldPitch);
    state.TargetPitch = m_setPitchOrAOA;
    state.TargetBank  = m_setBank;

    return (state.on ? XRAutopilotState::XRAPSTATE_Engaged : XRAutopilotState::XRAPSTATE_Disengaged);
}

XRAutopilotState DeltaGliderXR1::SetDescentHoldAP(const XRDescentHoldState &state)    // returns the new state of the autopilot, or XRAPSTATE_NotSupported if autopilot not supported
{
    XRAutopilotState retVal;
    if (state.on == false)
    {
        // only modify the custom autopilot mode if this mode is already engaged
        if (m_customAutopilotMode == AUTOPILOT::AP_DESCENTHOLD)
            SetCustomAutopilotMode(AUTOPILOT::AP_OFF, true, false);

        retVal = XRAutopilotState::XRAPSTATE_Disengaged;
    }
    else  // set autopilot to ON
    {
        // set AP parameters
        m_setDescentRate = state.TargetDescentRate;
        m_autoLand       = state.AutoLandMode;

        // if autopilot not already engaged, turn it on
        if (m_customAutopilotMode != AUTOPILOT::AP_DESCENTHOLD)
            ToggleDescentHold();  // use 'toggle' here because we don't have an explicit 'Activate...' method

        retVal = XRAutopilotState::XRAPSTATE_Engaged;
    }

    return retVal;
}

XRAutopilotState DeltaGliderXR1::GetDescentHoldAP(XRDescentHoldState &state) const
{
    state.on                = (m_customAutopilotMode == AUTOPILOT::AP_DESCENTHOLD);
    state.TargetDescentRate = m_setDescentRate;
    state.AutoLandMode      = m_autoLand;

    return (state.on ? XRAutopilotState::XRAPSTATE_Engaged : XRAutopilotState::XRAPSTATE_Disengaged);
}

XRAutopilotState DeltaGliderXR1::SetAirspeedHoldAP(const XRAirspeedHoldState &state)  // returns the new state of the autopilot, or XRAPSTATE_NotSupported if autopilot not supported
{
    XRAutopilotState retVal;
    if (state.on == false)
    {
        SetAirspeedHoldMode(false, true);   // turn off
        retVal = XRAutopilotState::XRAPSTATE_Disengaged;
    }
    else  // set autopilot to ON
    {
        // set AP parameters
        m_setAirspeed = state.TargetAirspeed;
        SetAirspeedHoldMode(true, true);
        retVal = XRAutopilotState::XRAPSTATE_Engaged;
    }

    return retVal;
}

XRAutopilotState DeltaGliderXR1::GetAirspeedHoldAP(XRAirspeedHoldState &state) const
{
    state.on             = m_airspeedHoldEngaged;
    state.TargetAirspeed = m_setAirspeed;

    return (state.on ? XRAutopilotState::XRAPSTATE_Engaged : XRAutopilotState::XRAPSTATE_Disengaged);
}

// Exterior lights: true = ON, false = OFF
bool DeltaGliderXR1::SetExteriorLight(XRLight light, bool state)
{
    switch (light)
    {
    case XRLight::XRL_Nav:
        SetNavlight(state);
        break;

    case XRLight::XRL_Beacon:
        SetBeacon(state);
        break;

    case XRLight::XRL_Strobe:
        SetStrobe(state);
        break;

    default:
        return false;       // should never happen!
    }

    return true;
}

bool DeltaGliderXR1::GetExteriorLight(XRLight light) const
{
    bool retVal;

    switch (light)
    {
    case XRLight::XRL_Nav:
        retVal = beacon[0].active;  // 0,1,2 are always in sync
        break;

    case XRLight::XRL_Beacon:
        retVal = beacon[3].active;  // 3,4 are always in sync
        break;

    case XRLight::XRL_Strobe:
        retVal = beacon[5].active;  // 5,6 are always in sync
        break;

    default:
        return false;       // should never happen!
    }

    return retVal;
}

// Secondary HUD mode (1-5) : 0 = OFF
// Returns: true on success, false if mode is unsupported
bool DeltaGliderXR1::SetSecondaryHUDMode(int modeNumber)
{
    if ((modeNumber < 0) || (modeNumber > 5))
        return false;       // invalid mode

    if (modeNumber == 0)
        DisableSecondaryHUD();
    else
        EnableAndSetSecondaryHUDMode(modeNumber);

    return true;
}

int DeltaGliderXR1::GetSecondaryHUDMode() const
{
    return m_secondaryHUDMode;
}

// Enable/disable tertiary HUD 
// use modeNumber 1 if only a single mode (i.e., on/off) is supported
// Returns true on success, or false if XR vessel does not support tertiary HUD
bool DeltaGliderXR1::SetTertiaryHUDState(bool on)  
{
    SetTertiaryHUDEnabled(on);
    return true;
}

bool DeltaGliderXR1::GetTertiaryHUDState() const
{
    return (m_tertiaryHUDOn ? 1 : 0);
}

// Reset the MWS (Master Warning System) alarm; note that under certain conditions the MWS cannot be reset (e.g., after a vessel crash)
// returns true if MWS alarm reset successfully, false if the alarm cannot be reset
bool DeltaGliderXR1::ResetMasterWarningAlarm()
{
    return ResetMWS();
}

//=========================================================================
// Utility methods
//=========================================================================

// convert a DoorStatus value to an XRDoorState value
XRDoorState DeltaGliderXR1::ToXRDoorState(DoorStatus status)
{
    XRDoorState retVal;
    
    switch (status)
    {
    case DoorStatus::DOOR_FAILED:
        retVal = XRDoorState::XRDS_Failed;
        break;

    case DoorStatus::DOOR_CLOSED:
        retVal = XRDoorState::XRDS_Closed;
        break;

    case DoorStatus::DOOR_OPEN:
        retVal = XRDoorState::XRDS_Open;
        break;

    case DoorStatus::DOOR_CLOSING:
        retVal = XRDoorState::XRDS_Closing;
        break;

    case DoorStatus::DOOR_OPENING:
        retVal = XRDoorState::XRDS_Opening;
        break;
    }

    return retVal;
}

// convert an XRDoorState value to a DoorStatus value
DoorStatus DeltaGliderXR1::ToDoorStatus(XRDoorState state)
{
    DoorStatus retVal;

    switch (state)
    {
    case XRDoorState::XRDS_Failed:
        retVal = DoorStatus::DOOR_FAILED;
        break;

    case XRDoorState::XRDS_Closed:
        retVal = DoorStatus::DOOR_CLOSED;
        break;

    case XRDoorState::XRDS_Open:
        retVal = DoorStatus::DOOR_OPEN;
        break;

    case XRDoorState::XRDS_Closing:
        retVal = DoorStatus::DOOR_CLOSING;
        break;

    case XRDoorState::XRDS_Opening:
        retVal = DoorStatus::DOOR_OPENING;
        break;
    }

    return retVal;
}

// Return the Orbiter navmode constant for the supplied XRStdAutopilot, or -1 if autopilot not supported
int DeltaGliderXR1::GetNavmodeForXRStdAutopilot(XRStdAutopilot id)
{
    int retVal; 
    switch (id)
    {
    case XRStdAutopilot::XRSAP_KillRot:
        retVal = NAVMODE_KILLROT;
        break;

    case XRStdAutopilot::XRSAP_Prograde:
        retVal = NAVMODE_PROGRADE;
        break;

    case XRStdAutopilot::XRSAP_Retrograde:
        retVal = NAVMODE_RETROGRADE;
        break;

    case XRStdAutopilot::XRSAP_Normal:
        retVal = NAVMODE_NORMAL;
        break;

    case XRStdAutopilot::XRSAP_AntiNormal:
        retVal = NAVMODE_ANTINORMAL;
        break;

        // LEVEL HORIZON and HOVER not supported; superceded by Attitude Hold and Descent Hold autopilots
    default:
        retVal = -1;
        break;
    }

    return retVal;
}

// Current center-of-Gravity 
// 0.0 = centered; +/- max value varies by vessel
double DeltaGliderXR1::GetCenterOfGravity() const
{
    // Note: must reverse this because CoL forward == CoG AFT
    return -m_centerOfLift;
}

// requestedShift = requested delta in meters from the current center-of-gravity; returns true on success, false if APU offline or if shift is maxed out
bool DeltaGliderXR1::ShiftCenterOfGravity(double requestedShift)
{
    requestedShift = -requestedShift;  // must *reverse* this because CoG shift == negative CoL shift
    // check APU and play a warning if the APU is offline
    if (!CheckHydraulicPressure(true, true))
        return false;

    return ShiftCenterOfLift(requestedShift);
}

// RCS Mode
// returns true if RCS DOCKING mode is active, false if RCS is in NORMAL mode
bool DeltaGliderXR1::IsRCSDockingMode() const
{
    return false;   // no docking mode for the XR1
}

// set or clear RCS docking mode
// Returns: true on success, false if RCS docking mode not supported
bool DeltaGliderXR1::SetRCSDockingMode(bool on)
{
    return false; // not supported
}

// Active EVA port
// returns true if crew elevator is the active EVA port, false if the docking port is active
bool DeltaGliderXR1::IsElevatorEVAPortActive() const
{
    return false;
}

// true = crew elevator active, false = docking port active
// returns: true on success, false if crew elevator not supported
bool DeltaGliderXR1::SetElevatorEVAPortActive(bool on)
{
    return false;
}

// Retrieve info/warning lines on status screen and tertiary HUD
// linesOut will be populated with the lines on the status screen/tertiary HUD, delimited by \r\n
// maxLinesToRetrieve = 0-64.
// NOTE: linesOut should contain space for at least 50 bytes per line retrieved.  XR HUDs display only the seven most-recent lines.
// Returns: # of lines copied to linesOut
int DeltaGliderXR1::GetStatusScreenText(char *pLinesOut, const int maxLinesToRetrieve) const
{
    const int lineCount = m_infoWarningTextLineGroup.GetLineCount();
    const int linesToRetrieve = min(maxLinesToRetrieve, lineCount);
    const int startingLineIndex = lineCount - linesToRetrieve;
    _ASSERTE(startingLineIndex >= 0);

    *pLinesOut = 0;  // empty contents

    // NOTE: lines are stored from OLD -> NEW, so we always copy the newest 'linesToRetrieve' lines in the vector
    for (int i=startingLineIndex; i < (startingLineIndex + linesToRetrieve); i++)
    {
        _ASSERTE(i >= 0);
        _ASSERTE(i < INFO_WARNING_BUFFER_LINES);
        TextLine textLine = m_infoWarningTextLineGroup.GetLine(i);
        
        // retrieve each line's string object and copy it to pLinesOut by value and terminating each with \r\n
        strcat(pLinesOut, textLine.text.c_str());
        strcat(pLinesOut, "\r\n");
    }
        
    return linesToRetrieve;
}

// Note: '&' character in the message string will generate a newline; tertiary HUD has approximately 38 characters per line.
// isWarning: true = show as warning (red text), false = show as info (green text)
void DeltaGliderXR1::WriteTertiaryHudMessage(const char *pMessage, const bool isWarning)
{
    if (isWarning)
        ShowWarning(NULL, ST_None, pMessage, false);
    else   // info message
        ShowInfo(NULL, ST_None, pMessage);
}

// Returns the name of the custom skin loaded for this vessel, if any.  NULL = no custom skin loaded.  
// e.g., if "SKIN foobar" is in the scenario file for this XR vessel, GetCustomSkinName() returns a pointer to the string "foobar".
const char *DeltaGliderXR1::GetCustomSkinName() const
{
    return (*skinpath ? skinpath : NULL);
}

//=========================================================================
//
// API methods added in XRVesselCtrl version 3.0
//

#define IS_SLOT_NUMBER_VALID(sn) ( m_pPayloadBay && (slotNumber > 0) && (slotNumber <= m_pPayloadBay->GetSlotCount()) )

// NOTE: these methods work for XR subclasses as well because they operate on the XRPayloadBay object, if any

// Returns the total number of payload bay slots in this XR vessel (1-n)
int DeltaGliderXR1::GetPayloadBaySlotCount() const 
{
    int slotCount = 0;
    if (m_pPayloadBay)
        slotCount = m_pPayloadBay->GetSlotCount();

    return slotCount;
}

// Returns true if the specified payload bay slot is free
//   slotNumber: 1 <= n <= GetPayloadBaySlotCount()
bool DeltaGliderXR1::IsPayloadBaySlotFree(const int slotNumber) const
{
    bool bSlotFree = false;
    if (IS_SLOT_NUMBER_VALID(slotNumber))
        bSlotFree = m_pPayloadBay->IsSlotEnabled(slotNumber);

    return bSlotFree;
}

// Returns details about an XR payload bay slot.  Returns NULL if slotNumber is invalid.
// Note: this method cannot be 'const' because we return 'this' as a non-const XRVesselCtrl *
//   slotNumber: 1 <= n <= GetPayloadBaySlotCount()
//   slotDataOut: will be populated with data for the specified slot if slotNumber is valid.  If slotNumber is invalied, the contents of slotDataOut are not changed.
// Returns: true if slotDataOut successfully populated (i.e., slotNumber was valid)
bool DeltaGliderXR1::GetPayloadSlotData(const int slotNumber, XRPayloadSlotData &slotDataOut)
{
    if (!IS_SLOT_NUMBER_VALID(slotNumber))
        return false;

    const XRPayloadBaySlot *pSlot = m_pPayloadBay->GetSlot(slotNumber);
    _ASSERTE(pSlot);
    
    // populate the XRPayloadSlotData to be returned to the caller
    slotDataOut.hCargoModuleVessel = pSlot->GetChild();  // may be NULL
    slotDataOut.pParentXRVessel = this;                  // for convenience later in case the caller is tracking multiple payload
    slotDataOut.hXRAttachmentHandle = pSlot->GetAttachmentHandle();
    slotDataOut.SlotNumber = pSlot->GetSlotNumber();
    slotDataOut.localCoordinates = pSlot->GetLocalCoordinates();
    slotDataOut.IsOccupied = pSlot->IsOccupied();
    slotDataOut.Dimensions = pSlot->GetDimensions();
    slotDataOut.BayLevel = pSlot->GetLevel();

    return true;
}
  
// Returns true if the supplied vessel 1) is an XR-compatible payload vessel, and 2) can be latched into the specified slot (i.e., it will fit)
//   slotNumber: 1 <= n <= GetPayloadBaySlotCount()
bool DeltaGliderXR1::CanAttachPayload(const OBJHANDLE hPayloadVessel, const int slotNumber) const
{
    // if crew is incapacitated, nothing to do here
    if (IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    if (!IS_SLOT_NUMBER_VALID(slotNumber))
        return false;

    const XRPayloadBaySlot *pSlot = m_pPayloadBay->GetSlot(slotNumber);
    _ASSERTE(pSlot);

    // retrieve the vessel for the supplied payload vessel handle
    VESSEL *pPayloadVessel = oapiGetVesselInterface(hPayloadVessel);
    if (!pPayloadVessel)
        return false;

    return pSlot->CheckSlotSpace(*pPayloadVessel);
}

// Attempts to grapple the given payload vessel into the specified slots; there is no distance check, although there is a size check.
// Note: hPayloadVessel must be an XR payload module in order to be gappled into the payload bay.
//   slotNumber: 1 <= n < GetPayloadBaySlotCount()
// Returns: true on success, false if payload could not be grappled into the requested slot
bool DeltaGliderXR1::GrapplePayloadModuleIntoSlot(const OBJHANDLE hPayloadVessel, const int slotNumber)
{
    // if crew is incapacitated, nothing to do here
    if (IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    if (!CanAttachPayload(hPayloadVessel, slotNumber))
        return false;

    // payload vessel is an XR payload vessel and can fit in the slot, so attach it.
    return m_pPayloadBay->AttachChild(hPayloadVessel, slotNumber);
}

// Detaches a payload vessel from the specified slot at the specified delta-V along the Y axis (up out of the bay).
// You should normally only call this on vessels that are in space.
//   slotNumber: 1 <= n <= GetPayloadBaySlotCount()
//   deltaV = +Y deployment velocity in meters-per-second (e.g., 0.2)
// Returns: true on success, false if slot is invalid or no payload is attached in the specified slot
bool DeltaGliderXR1::DeployPayloadInFlight(const int slotNumber, const double deltaV)
{
    // if crew is incapacitated, nothing to do here
    if (IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    if (!IS_SLOT_NUMBER_VALID(slotNumber))
        return false;

    return m_pPayloadBay->DetachChild(slotNumber, deltaV);
}

// Detaches a payload vessel from the specified slot and moves it to alongside the ship on the ground.
// You should normally only call this on vessels that are landed and stationary.
//   slotNumber: 1 <= n <= GetPayloadBaySlotCount()
// Returns: true on success, false if slot is invalid or no payload is attached in the specified slot
bool DeltaGliderXR1::DeployPayloadWhileLanded(const int slotNumber)
{
    // if crew is incapacitated, nothing to do here
    if (IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    if (!IS_SLOT_NUMBER_VALID(slotNumber))
        return false;

    return m_pPayloadBay->DetachChildLanded(slotNumber);
}

// Detaches all payload vessels at the specified delta-V along the Y axis (up out of the bay).
// You should normally only call this on vessels that are in space.
// Note: this does NOT check the status of the payload bay doors: it is the caller's responsibility to do that if desired.
//   deltaV = +Y deployment velocity in meters-per-second (e.g., 0.2)
// Returns: number of payload vessels deployed
int DeltaGliderXR1::DeployAllPayloadInFlight(const double deltaV)
{
    // if crew is incapacitated, nothing to do here
    if (IsCrewIncapacitatedOrNoPilotOnBoard())
        return 0;

    if (!m_pPayloadBay)
        return 0;

    return m_pPayloadBay->DetachAllChildren(deltaV);
}

// Detaches all payload vessels and moves them to alongside the ship on the ground.
// You should normally only call this on vessels that are landed and stationary.
// Note: this does NOT check the status of the payload bay doors: it is the caller's responsibility to do that if desired.
// Returns: number of payload vessels deployed
int DeltaGliderXR1::DeployAllPayloadWhileLanded()
{
    // if crew is incapacitated, nothing to do here
    if (IsCrewIncapacitatedOrNoPilotOnBoard())
        return 0;

    if (!m_pPayloadBay)
        return 0;

    return m_pPayloadBay->DetachAllChildrenLanded();
}

// Enables or disables MWS test mode (i.e., same as pressing or releasing the 'Test' MWS button)
// Returns: previous state of MWS test mode
bool DeltaGliderXR1::SetMWSTest(bool bTestMode)
{
    // if crew is incapacitated, nothing to do here
    if (IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    const bool bRetVal = m_mwsTestActive;
    m_mwsTestActive = bTestMode;
    
    return bRetVal;
}

// Returns true if 'recenter the center-of-gravity' mode is enabled
bool DeltaGliderXR1::GetRecenterCOGMode() const
{
    return m_cogShiftCenterModeActive;
}

// Enable or disable 'recenter the center-of-gravity' mode
// Returns: true on success, false if mode could not be set because no pilot is on board
bool DeltaGliderXR1::SetRecenterCOGMode(const bool bEnableRecenterMode)
{
    // if crew is incapacitated, nothing to do here
    if (IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    SetRecenterCenterOfGravityMode(bEnableRecenterMode);    // will redraw the affected panel areas, too

    return true;
}

// Returns the status of the external cooling line.
XRDoorState DeltaGliderXR1::GetExternalCoolingState() const 
{
    return ToXRDoorState(externalcooling_status);
}

// Deploys or retracts the external cooling line and shows a success or failure message on the secondary HUD.
// Returns: true on success, false on error
bool DeltaGliderXR1::SetExternalCoolingState(const bool bEnabled)
{
    // if crew is incapacitated, nothing to do here
    if (IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    return RequestExternalCooling(bEnabled);
}

// Sets fuel cross-feed mode and shows a status message on the secondary HUD
//   state: XF_MAIN, XF_OFF, or XF_RCS
// Returns: true on success, false if state is invalid or no crew members on board
bool DeltaGliderXR1::SetCrossFeedMode(XRXFEED_STATE state)
{
    // if crew is incapacitated, nothing to do here
    if (IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    // range-check
    if ((state < XRXFEED_STATE::XRXF_MAIN) || (state > XRXFEED_STATE::XRXF_RCS))
        return false;

    // NOTE: XFEED_STATE matches XFEED_MODE exactly -- do not change this!
    SetCrossfeedMode(static_cast<XFEED_MODE>(state), NULL);
    return true;
}

//=========================================================================