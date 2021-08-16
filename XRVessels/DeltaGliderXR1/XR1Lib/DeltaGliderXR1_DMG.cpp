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
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// DeltaGliderXR1_DMG.cpp
// Handles XR1 Damage
// ==============================================================

#include "DeltaGliderXR1.h"
#include "AreaIDs.h"
#include "XRCommon_DMG.h"

void DeltaGliderXR1::TestDamage()
{
    // work around Orbiter startup step bug: do not check for damage within the first two seconds of startup UNLESS we are crashed
    if ((IsCrashed() == false) && (GetAbsoluteSimTime() < 2.0))
        return;

    bool newdamage = false;
    double dt = oapiGetSimStep();

    // fail the ailerons/elevons if marked as damaged in the scenario file load
    FailAileronsIfDamaged();

    // if crashed, damage everything!
    if (IsCrashed())
    {
        // do not process crash logic more than once
        if (m_crashProcessed == false)
        {
            m_crashProcessed = true; // do not do this more than once
            PerformCrashDamage();
            newdamage = true;
        }
        goto exit;  // nothing else to check
    }

    // if crew incapacitated, temporarily (for this timestep) disable systems so ship is unflyable
    if (IsCrewIncapacitatedOrNoPilotOnBoard())
    {
        // kill all the engine throttles
        for (int i = 0; i < 2; i++)
        {
            SetThrusterLevel(th_scram[i], 0);
            scram_intensity[i] = 0;

            SetThrusterLevel(th_hover[i], 0);
            SetThrusterLevel(th_main[i], 0);
            SetThrusterLevel(th_retro[i], 0);
        }

        // same for RCS jets
        for (int i=0; i < 14; i++)
            SetThrusterLevel(th_rcs[i], 0);

        // turn off the RCS and airfoil systems
        SetAttitudeMode(RCS_NONE);
        SetADCtrlMode(0);

        // APU check is handled manually elsewhere

        // ATC ON/OFF is handled at the point of crew being incapacitated; however, we must turn ATC off here in case the scenario was just reloaded.
        XRSoundOnOff(XRSound::RadioATCGroup, false);
        // cabin airflow still active
    }

    // airframe damage as a result of wingload stress
    // or excessive dynamic pressure
    
    double load = GetLift() / WING_AREA; // L/S
    double dynp = GetDynPressure();  // dynamic pressure

    if (GetXR1Config()->WingStressDamageEnabled && AllowDamageIfDockedCheck() && !Playback())
    {
        if (load > WINGLOAD_MAX || load < WINGLOAD_MIN || dynp > DYNP_MAX) 
        {
            double alpha = max ((dynp-DYNP_MAX) * 1e-5,         // amount over-limit * 100K
                (load > 0 ? load-WINGLOAD_MAX : WINGLOAD_MIN-load) * 5e-5);
            double p = 1.0 - exp (-alpha*dt); // probability of failure
            if (oapiRand() < p) 
            {
                const char *pMsg;
                // simulate structural failure by distorting the airfoil definition
                int rfail = static_cast<int>(oapiRand() * RAND_MAX);      // use oapiRand() here since it's already seeded with a random value
                switch (rfail & 3) 
                {
                case 0: // fail left wing
                    lwingstatus *= exp (-alpha*oapiRand());
                    pMsg = "Left Wing Failure!";
                    m_warningLights[static_cast<int>(WarningLight::wlLwng)] = true;
                    break;
                case 1: // fail right wing
                    rwingstatus *= exp (-alpha*oapiRand());
                    pMsg = "Right Wing Failure!";
                    m_warningLights[static_cast<int>(WarningLight::wlRwng)] = true;
                    break;
                case 2: 
                    { 
                        pMsg = "Left Aileron Failure!";
                        aileronfail[0] = aileronfail[1] = true;     // delete both aileron mesh groups
                        m_warningLights[static_cast<int>(WarningLight::wlLail)] = true;
                        brake_status = DoorStatus::DOOR_FAILED;  // airbrake inoperable as well
                        m_warningLights[static_cast<int>(WarningLight::wlAirb)] = true;
                        FailAileronsIfDamaged();   // delete control surface
                    } 
                    break;
                case 3: 
                    { 
                        pMsg = "Right Aileron Failure!";
                        aileronfail[2] = aileronfail[3] = true;     // delete both aileron mesh groups
                        m_warningLights[static_cast<int>(WarningLight::wlRail)] = true;
                        brake_status = DoorStatus::DOOR_FAILED;  // airbrake inoperable as well
                        m_warningLights[static_cast<int>(WarningLight::wlAirb)] = true;
                        FailAileronsIfDamaged();   // delete control surface
                    } 
                    break;
                }
                ShowWarning("Warning airframe damage.wav", ST_WarningCallout, pMsg);
            }   // if block
            
            newdamage = true;
        }
        else  // let's check for warnings
        {
#define CHECK_MAX_THRESHOLD(value, max, soundFilename, msg, lightIdx, lightIdx2, wingstatus, wingstatus2) \
    if (value > (max * warningThreshold))                      \
    {                                                          \
        ShowWarning(soundFilename, ST_WarningCallout, msg);    \
        m_warningLights[static_cast<int>(lightIdx)] = true;    \
        m_warningLights[static_cast<int>(lightIdx2)] = true;   \
        newdamage = true;                                      \
        wingWarnLightsOn = true;                               \
    }                                                          \
    else if (wingWarnLightsOn == false)                        \
    {                                                          \
        if (wingstatus == 1.0)  m_warningLights[static_cast<int>(lightIdx)] = false;  \
        if (wingstatus2 == 1.0) m_warningLights[static_cast<int>(lightIdx2)] = false; \
    }

#define CHECK_MIN_THRESHOLD(value, min, soundFilename, msg, lightIdx, lightIdx2, wingstatus, wingstatus2) \
    if (value < (min * warningThreshold))                      \
    {                                                          \
        ShowWarning(soundFilename, ST_WarningCallout, msg);    \
        m_warningLights[static_cast<int>(lightIdx)] = true;                      \
        m_warningLights[static_cast<int>(lightIdx2)] = true;                     \
        wingWarnLightsOn = true;                               \
        newdamage = true;                                      \
    }                                                          \
    else if (wingWarnLightsOn == false)                        \
    {                                                          \
        if (wingstatus == 1.0)  m_warningLights[static_cast<int>(lightIdx)] = false;  \
        if (wingstatus2 == 1.0) m_warningLights[static_cast<int>(lightIdx2)] = false; \
    }

            static const double warningThreshold = .85;     // 85%
            const char *pWingStress = "WARNING Wing Stress.wav";
            bool wingWarnLightsOn = false;  // true if either wing warning light is turned on below; we don't want to turn off
            CHECK_MAX_THRESHOLD(load, WINGLOAD_MAX, pWingStress, "Wing load over 85% of maximum.", WarningLight::wlRwng, WarningLight::wlLwng, rwingstatus, lwingstatus);
            CHECK_MIN_THRESHOLD(load, WINGLOAD_MIN, pWingStress, "Negative wing load over 85%&of maximum.", WarningLight::wlRwng, WarningLight::wlLwng, rwingstatus, lwingstatus);
            CHECK_MAX_THRESHOLD(dynp, DYNP_MAX,     "Warning dynamic pressure.wav", "Dynamic pressure over 85%&of maximum.", WarningLight::wlDynp, WarningLight::wlDynp, 1.0, 1.0);  // always OK to turn off warning light for DynP
        }
    }   // if WingStressDamageEnabled

    //
    // Check for door-related heat and/or dynamic pressure damage here
    // Note that a given damange type is only checked if it is enabled.
    //
    newdamage |= CheckAllDoorDamage();     // returns true if any door is damaged

    // 
    // Check SCRAM engine temperature
    //
    if (GetXR1Config()->ScramEngineOverheatDamageEnabled && !Playback())
    {
        // NOTE: since doors always work in tandem, we only need to check the LEFT engine here
        const double alpha = CheckScramTemperature(ramjet->Temp(0, 0), MAX_SCRAM_TEMPERATURE);
        if (alpha != 0)
        {
            // ENGINE DAMAGE -- check for critical engine failure vs. just engine damage
            // NOTE: as an example, alpha values if pilot is over max temp:
            //  0% over = 0.00
            //  5% over = 0.20
            // 10% over = 0.42
            // 20% over = 0.88
            // 30% over = 1.38
            // NOTE: do not integrate dt here; dt was already taken into account by CheckTemperature
            // pick a random engine and damage it based on alpha delta
            int engineIndex = ((oapiRand() < 0.5) ? 0 : 1);
            const double engineFrac = max(0, (1.0 - alpha));
            ramjet->SetEngineIntegrity(engineIndex, ramjet->GetEngineIntegrity(engineIndex) * engineFrac);
            
            // NOTE: SCRAM warning light already handled by CheckScramTemperature

            const double engineInteg = ramjet->GetEngineIntegrity(engineIndex);
            const double mach = GetMachNumber();
            char temp[80];
            if (oapiRand() > engineInteg)
            {
                sprintf(temp, "#%d SCRAM ENGINE EXPLOSION at Mach %.1lf!", (engineIndex+1), mach);
                DoCrash(temp, 0);
            }
            else
            {
                sprintf(temp, "SCRAM ENGINE #%d DAMAGE&at Mach %.1lf!&Engine Integrity=%.1lf%%", engineIndex, mach, engineInteg*100);
                // NOTE: audio callout already occurred
                ShowWarning(nullptr, DeltaGliderXR1::ST_None, temp, true);   // force this
                newdamage = true;
            }
        }
    }

    //
    // Check hull temperatures
    // 
    if (GetXR1Config()->HullHeatingDamageEnabled && AllowDamageIfDockedCheck() && !Playback())
        newdamage |= CheckHullHeatingDamage();

exit:
    if (newdamage)
    {
        m_MWSActive = true;
        ApplyDamage();
        //UpdateDamageDialog (this);
    }
    
    // if no warning present, reset the MWS automatically
    if (!IsWarningPresent())
        m_MWSActive = false;    // it's all good now...
}

//
// Check all hull surfaces for heat damage.
// 
// NOTE: CheckTemperature will turn on warning lights for doors and hull temp as necessary; however
// we must reset the HTMP light ourselves since any surface can trigger it.  CheckTemperature will only 
// SET the light, never CLEAR it.  Therefore, we clear it here ourselves.
//
// Returns: true if any surface damaged, false otherwise
bool DeltaGliderXR1::CheckHullHeatingDamage()
{
    bool newdamage = false;
    double alpha = 0;
    char temp[128];
    const double mach = GetMachNumber();
    m_warningLights[static_cast<int>(WarningLight::wlHtmp)] = false;     // assume hull temp warning light OFF

    // check nosecone temperature using both nosecone and hover doors
    if (CheckTemperature(m_noseconeTemp,  m_hullTemperatureLimits.noseCone, IS_DOOR_OPEN(nose_status)) != 0)
    {
        // HULL FAILURE - crew death!
        sprintf(temp, "NOSECONE BREACH at Mach %.1lf!", mach);
        DoCrash(temp, 0);       
    }

    //
    // Note: checking these lower hull items separately will increase our chances of hull breach when more than one door is open; this is what we want!
    //
    if ( (CheckTemperature(m_noseconeTemp,  m_hullTemperatureLimits.noseCone, IS_DOOR_OPEN(hoverdoor_status)) != 0) ||
         (CheckTemperature(m_noseconeTemp,  m_hullTemperatureLimits.noseCone, IS_DOOR_OPEN(gear_status)) != 0))
    {
        // LOWER HULL FAILURE - crew death!
        sprintf(temp, "LOWER HULL BREACH at Mach %.1lf!", mach);
        DoCrash(temp, 0);       
    }

    // This check assumes the retro doors are related to the wings
    const bool retroDoorsOpen = IS_DOOR_OPEN(rcover_status);
    if ((alpha = CheckTemperature(m_leftWingTemp,  m_hullTemperatureLimits.wings, retroDoorsOpen)) != 0)
    {
        // WING DAMAGE -- check for critical ship failure vs. just wing damage
        // NOTE: as an example, alphas values if pilot is over max temp:
        //  0% over = 0.00
        //  5% over = 0.10
        // 10% over = 0.21
        // 20% over = 0.44
        // 30% over = 0.69
        // 40% over = 0.96 
        // 50% over = 1.25
        // NOTE: do not integrate dt here; dt was already taken into account by CheckTemperature
        const double wingFrac = min(0, (1.0 - alpha));
        lwingstatus *= wingFrac;
        m_warningLights[static_cast<int>(WarningLight::wlLwng)] = true;   // warning light ON

        if (oapiRand() > lwingstatus)    
        {
            sprintf(temp, "LEFT WING BREACH at Mach %.1lf!", mach);
            DoCrash(temp, 0);
        }
        else
        {
            sprintf(temp, "LEFT WING DAMAGE at Mach %.1lf!&Wing Integrity=%.1lf%%", mach, lwingstatus*100);
            ShowWarning(nullptr, DeltaGliderXR1::ST_None, temp, true);   // force this 
            newdamage = true;
        }
    }

    // This check assumes the retro doors are related to the wings
    if ((alpha = CheckTemperature(m_rightWingTemp, m_hullTemperatureLimits.wings, retroDoorsOpen)) != 0)
    {
        const double wingFrac = min(0, (1.0 - alpha));
        rwingstatus *= wingFrac;
        m_warningLights[static_cast<int>(WarningLight::wlRwng)] = true;   // warning light ON

        // WING DAMAGE -- check for critical ship failure vs. just wing damage
        if (oapiRand() > rwingstatus)
        {
            sprintf(temp, "RIGHT WING BREACH at Mach %.1lf!", mach);
            DoCrash(temp, 0);
        }
        else
        {
            sprintf(temp, "RIGHT WING DAMAGE at Mach %.1lf&Wing Integrity=%.1lf%%", mach, rwingstatus*100);
            ShowWarning(nullptr, DeltaGliderXR1::ST_None, temp, true); // force this
            newdamage = true;
        }
    }

    // This check assumes the escape hatch is close to the cockpit.
    if (CheckTemperature(m_cockpitTemp, m_hullTemperatureLimits.cockpit, IS_DOOR_OPEN(hatch_status)) != 0)
    {
        // HULL FAILURE - crew death!
        sprintf(temp, "COCKPIT BREACH at Mach %.1lf!", mach);
        DoCrash(temp, 0);
    }

    // Note: the XR1 does not have a payload bay, but it's OK to check it here for the purpose of subclasses
    // top hull max temp is tied to: 1) radiators, and 2) bay doors
    if ((CheckTemperature(m_topHullTemp, m_hullTemperatureLimits.topHull, IS_DOOR_OPEN(radiator_status)) != 0) ||
        (CheckTemperature(m_topHullTemp, m_hullTemperatureLimits.topHull, IS_DOOR_OPEN(bay_status)) != 0))
    {
        // HULL FAILURE - crew death!
        sprintf(temp, "TOP HULL BREACH at Mach %.1lf!", mach);
        DoCrash(temp, 0);
    }

    return newdamage;
}

//
// Returns the *effective* external temperature of the air molecules, taking static pressure into account.
double DeltaGliderXR1::GetExternalTemperature() const
{
    /* Orbiter 2009 new atmosphere models fix: we cannot just take OAT as a baseline anymore because the temperature 
       reported by the core is very high in the upper atmosphere, even though dynamic pressure is practically non-existant.
       Empirical testing with the surface MFD shows that mach and temperature are valid at DYNAMIC PRESSURE 2.78 pascals and STATIC PRESSURE of about 0.014 pascal.

            100% OAT at staticPressure >= OAT_VALID_STATICP_THRESHOLD
              ...tapering smoothly down to...
            10% OAT at 0 kpa  (lower figure is arbitrary)
    */
    double effectiveOATFraction = (GetAtmPressure() / OAT_VALID_STATICP_THRESHOLD);

    // keep in range
    if (effectiveOATFraction > 1.0)
        effectiveOATFraction = 1.0;   // baseline temp is never greater than OAT
    else if (effectiveOATFraction < 0.1)
        effectiveOATFraction = 0.1;   // baseline temp is never less than 10% of OAT

    // WARNING: THIS SHOULD BE THE *ONLY* PLACE IN THE CODE WHERE GetAtmTemperature() IS INVOKED!  All other code should invoke GetExternalTemperature() instead.
    const double extTemp = GetAtmTemperature() * effectiveOATFraction;  // this is in Kelvin, so it is never negative
    return extTemp;
}

//
// Check for door-related heat and/or dynamic pressure damage here
// Note that a given damange type is only checked if it is enabled.
//
// Returns: true if any damage detected, false otherwise
bool DeltaGliderXR1::CheckAllDoorDamage()
{
    bool newdamage = false;
    newdamage |= CheckDoorFailure(&nose_status);
    newdamage |= CheckDoorFailure(&rcover_status);
    newdamage |= CheckDoorFailure(&hatch_status);
    newdamage |= CheckDoorFailure(&radiator_status);
    newdamage |= CheckDoorFailure(&gear_status);
    newdamage |= CheckDoorFailure(&hoverdoor_status);
    // SCRAM doors cannot fail for heat or pressure, so don't check them

    return newdamage;
}

// Perform crash damage; i.e., damage all systems.  This is invoked only once when a crash occurs.
void DeltaGliderXR1::PerformCrashDamage()
{
    // WARNING: do not set m_cabinO2Level = 0 here: it will trigger a "crew dead due to hypoxia" message on the HUD

    // turn on ALL warning lights
    for (int i=0; i < WARNING_LIGHT_COUNT; i++)
        m_warningLights[i] = true;

    // disable any autopilots
    m_customAutopilotMode = AUTOPILOT::AP_OFF;
    for (int i=0; i <=7; i++)
        DeactivateNavmode(i);

    // disable ATC and cabin airflow sounds from now on
    XRSoundOnOff(XRSound::RadioATCGroup, false);  
    XRSoundOnOff(XRSound::AirConditioning, false);

    // fail gear
    FailGear(true);

    // fail left wing
    if (lwingstatus == 1.0)     // not already damaged?
        lwingstatus = oapiRand() * 0.5;

    // fail right wing
    if (rwingstatus == 1.0)     // not already damaged?
        rwingstatus = oapiRand() * 0.5;

    // fail all ailerons 
    aileronfail[0] = aileronfail[1] = true;
    aileronfail[2] = aileronfail[3] = true;
    FailAileronsIfDamaged();

    // Deactivate doors
    hoverdoor_status = nose_status = hatch_status = radiator_status = brake_status = rcover_status = brake_status = DoorStatus::DOOR_FAILED;

    //
    // deactivate engines 
    //

    // kill all the engines
    for (int i = 0; i < 2; i++)
    {
        SetThrusterLevel(th_scram[i], 0);
        scram_intensity[i] = 0;

        SetThrusterLevel(th_hover[i], 0);
        SetThrusterLevel(th_main[i], 0);
        SetThrusterLevel(th_retro[i], 0);
    }

    // NOTE: do not delete thrusters here!  Orbiter doesn't like it.
    // So we'll set max thrust to zero and empty the fuel tanks instead.
    for (int i=0; i < 2; i++)
    {
        SetThrusterMax0(th_main[i], 0);
        SetThrusterMax0(th_retro[i], 0);                
        SetHoverThrusterMaxAndIntegrity(i, 0);
    }

    // zero fuel in the bay tanks as well
    SetXRPropellantMass(ph_main, 0);
    SetXRPropellantMass(ph_rcs, 0);
    SetXRPropellantMass(ph_scram, 0);

    // same for RCS jets
    for (int i=0; i < 14; i++)
        SetRCSThrusterMaxAndIntegrity(i, 0);

    // we have to disable the SCRAM engines manually
    ramjet->SetEngineIntegrity(0, 0);
    ramjet->SetEngineIntegrity(1, 0);

    // fuel lights will come on automatically via the fuel PostStep 

    // fail all remaining control surfaces
    ClearControlSurfaceDefinitions();

    // kill the APU
    apu_status = DoorStatus::DOOR_FAILED;   // this will deactivate all doors as well
    m_apuWarning = true;
    m_apuFuelQty = 0;
    StopSound(APU);
}

// Check whether ANY system is damaged. Invoked when loading or saving state.
// Returns: true if any damage present, false if all systems green
bool DeltaGliderXR1::IsDamagePresent() const
{
    bool retVal = false;        // assume no damage
    
    // loop through all surfaces
    for (int i=0; i <= static_cast<int>(D_END); i++)
    {
        DamageStatus ds = GetDamageStatus((DamageItem)i);
        if (ds.fracIntegrity < 1.0)
        {
            retVal = true;  // damage present
            break;
        }
    }

    return retVal;
}

// Check whether ANY warning is active.  Invoked on startup.
// Returns: true if any warning present, false if no warnings present
bool DeltaGliderXR1::IsWarningPresent() const
{
    bool retVal = false;        // assume no damage
    
    // loop through all warning lights
    for (int i=0; i < WARNING_LIGHT_COUNT; i++)
    {
        bool warningLightActive = m_warningLights[i];
        if (warningLightActive)
        {
            retVal = true;  // warning present
            break;
        }
    }

    // check for APU warning
    if (m_apuWarning)
        retVal = true;

    // if crew is DEAD or incapacitated, that's worth a warning...
    if (IsCrewIncapacitatedOrNoPilotOnBoard())
        retVal = true;

    return retVal;
}

// returns DamageStatus (a static variable)
// This queries the actual SYSTEM STATE (e.g., current thrust output) to determine whether an item is damaged.
const DamageStatus &DeltaGliderXR1::GetDamageStatus(DamageItem item) const
{
    double frac;
    const char *pLabel = "???";
    const char *pShortLabel = "???";
    char tempLabel[8];
    bool onlineOffline = true;     // assume online/offline

    switch (item)
    {   
    case DamageItem::LeftWing:
        frac = lwingstatus;
        pLabel = "Left Wing";
        pShortLabel = "LWng";
        onlineOffline = false;     // has partial failure
        break;

    case DamageItem::RightWing:
        frac = rwingstatus;
        pLabel = "Right Wing";
        pShortLabel = "RWng";
        onlineOffline = false;     // has partial failure
        break;

    case DamageItem::LeftAileron:
        frac = ((aileronfail[0] | aileronfail[1]) ? 0 : 1);    // either mesh index 0 or 1 could be marked FAILED, so we must check both
        pLabel = "Left Aileron";
        pShortLabel = "LAil";
        break;

    case DamageItem::RightAileron:
        frac = ((aileronfail[2] | aileronfail[3]) ? 0 : 1);    // either mesh index 2 or 3 could be marked FAILED, so we must check both
        pLabel = "Right Aileron";
        pShortLabel = "RAil";
        break;

    case DamageItem::LandingGear:
        frac = ((gear_status == DoorStatus::DOOR_FAILED) ? 0 : 1);
        pLabel = "Landing Gear";
        pShortLabel = "Gear";
        break;

    case DamageItem::Nosecone:
        frac = ((nose_status == DoorStatus::DOOR_FAILED) ? 0 : 1);
        pLabel = NOSECONE_LABEL;
        pShortLabel = NOSECONE_SHORT_LABEL;
        break;

    case DamageItem::RetroDoors:
        frac = ((rcover_status == DoorStatus::DOOR_FAILED) ? 0 : 1);
        pLabel = "Retro Doors";
        pShortLabel = "RDor";
        break;

    case DamageItem::Hatch:
        frac = ((hatch_status == DoorStatus::DOOR_FAILED) ? 0 : 1);
        pLabel = "Top Hatch";
        pShortLabel = "Htch";
        break;

    case DamageItem::Radiator:
        frac = ((radiator_status == DoorStatus::DOOR_FAILED) ? 0 : 1);
        pLabel = "Radiator";
        pShortLabel = "Rad";
        break;

    case DamageItem::Airbrake:
        frac = ((brake_status == DoorStatus::DOOR_FAILED) ? 0 : 1);
        pLabel = "Airbrake";
        pShortLabel = "Airb";
        break;

    case DamageItem::MainEngineLeft:
    {
        const double maxMainThrust = MAX_MAIN_THRUST[GetXR1Config()->MainEngineThrust];
        frac = (maxMainThrust > 0 ? (GetThrusterMax0(th_main[0]) / MAX_MAIN_THRUST[GetXR1Config()->MainEngineThrust]) : 1.0);  // if max main thrust set to zero via cheatcode, engines cannot fail (avoid divide-by-zero here as well)
        pLabel = "Left Main Engine";
        pShortLabel = "LEng";
        onlineOffline = false;     // has partial failure
        break;
    }

    case DamageItem::MainEngineRight:
    {
        const double maxMainThrust = MAX_MAIN_THRUST[GetXR1Config()->MainEngineThrust];
        frac = (maxMainThrust > 0 ? (GetThrusterMax0(th_main[1]) / MAX_MAIN_THRUST[GetXR1Config()->MainEngineThrust]) : 1.0);  // if max main thrust set to zero via cheatcode, engines cannot fail (avoid divide-by-zero here as well)
        pLabel = "Right Main Engine";
        pShortLabel = "REng";
        onlineOffline = false;     // has partial failure
        break;
    }

    case DamageItem::SCRAMEngineLeft:
        frac = ramjet->GetEngineIntegrity(0);
        pLabel = "Left SCRAM Engine";
        pShortLabel = "LScr";
        onlineOffline = false;     // has partial failure
        break;

    case DamageItem::SCRAMEngineRight:
        frac = ramjet->GetEngineIntegrity(1);
        pLabel = "Right SCRAM Engine";
        pShortLabel = "RScr";
        onlineOffline = false;     // has partial failure
        break;

    case DamageItem::HoverEngineFore:
        // must make explicit check for damage here because we can vary the max thrust based on gimbaling
        frac = m_hoverEngineIntegrity[0];
        pLabel = "Fore Hover Engine";
        pShortLabel = "FHov";
        onlineOffline = false;     // has partial failure
        break;

    case DamageItem::HoverEngineAft:
        // must make explicit check for damage here because we can vary the max thrust based on gimbaling
        frac = m_hoverEngineIntegrity[1];
        // can't do this: frac = GetThrusterMax0(th_hover[1]) / MAX_HOVER_THRUST[GetXR1Config()->HoverEngineThrust];
        pLabel = "Aft Hover Engine";
        pShortLabel = "AHov";
        onlineOffline = false;     // has partial failure
        break;

    case DamageItem::RetroEngineLeft:
        frac = (MAX_RETRO_THRUST > 0 ? (GetThrusterMax0(th_retro[0]) / MAX_RETRO_THRUST) : 1.0);    // if retro max thrust set to zero via cheatcode, engines cannot fail (avoid divide-by-zero here as well)
        pLabel = "Left Retro Engine";
        pShortLabel = "LRet";
        onlineOffline = false;     // has partial failure
        break;

    case DamageItem::RetroEngineRight:
        frac = (MAX_RETRO_THRUST > 0 ? (GetThrusterMax0(th_retro[1]) / MAX_RETRO_THRUST) : 1.0);    // if retro max thrust set to zero via cheatcode, engines cannot fail (avoid divide-by-zero here as well)
        pLabel = "Right Retro Engine";
        pShortLabel = "RRet";
        onlineOffline = false;     // has partial failure
        break;

    case DamageItem::RCS1:
    case DamageItem::RCS2:
    case DamageItem::RCS3:
    case DamageItem::RCS4:
    case DamageItem::RCS5:
    case DamageItem::RCS6:
    case DamageItem::RCS7:
    case DamageItem::RCS8:
    case DamageItem::RCS9:
    case DamageItem::RCS10:
    case DamageItem::RCS11:
    case DamageItem::RCS12:
    case DamageItem::RCS13:
    case DamageItem::RCS14:
        {
            int index = static_cast<int>(item) - static_cast<int>(DamageItem::RCS1);    // 0-13
            // these are display names for the MDA screens, so keep the length reasonable
            static const char *pLabels[] = 
            { 
                "Forward Lower RCS", "Aft Upper RCS", "Forward Upper RCS", "Aft Lower RCS",
                "Forward Star. RCS", "Aft Port RCS", "Forward Port RCS", "Aft Star. RCS",
                "Outboard Upper Port RCS", "Outboard Lower Star. RCS", "Outboard Upper Star. RCS", "Outboard Lower Port RCS", 
                "Aft RCS", "Forward RCS"
            };  
            
            // for simplicity, we do not use RCS thrust as a damage indicator; we use in internal RCS array instead
            frac = m_rcsIntegrityArray[index];  // internal array
            pLabel = pLabels[index];
            sprintf(tempLabel, "RCS%d", (index+1));     // RCS1...RCS14
            pShortLabel  = tempLabel;
            onlineOffline = false;     // has partial failure
            break;
        }

    default:        // should never happen!
        frac = 0;
        pLabel = "???????";
        pShortLabel = "????";
        break;
    }

    // populate the structure
    static DamageStatus dmgStatus;
    
    dmgStatus.fracIntegrity = frac;
    strcpy(dmgStatus.label, pLabel);
    strcpy(dmgStatus.shortLabel, pShortLabel);
    dmgStatus.onlineOffline = onlineOffline;

    return dmgStatus;   // return by reference
}

// This method will reset (repair) all damaged systems and clear all warning lights. Note that it is not invoked by 
// XR systems -- it is only used by XRVesselCtrl calls.
// This should normally NOT need to be overridden by vessel subclasses.
void DeltaGliderXR1::ResetDamageStatus()
{
    // first, clear all damage states
    for (int i=0; i <= static_cast<int>(D_END); i++)
        SetDamageStatus((DamageItem)i, 1.0);  // this will not reset warning lights, however

    // second, clear all warning lights
    for (int i=0; i < WARNING_LIGHT_COUNT; i++)
        m_warningLights[i] = false;

    // third, reset (recreate) any damaged control surfaces (i.e., surfaces with a 0 handle)
    ReinitializeDamageableControlSurfaces();

    // finally, restore any possibly-deleted mesh items
    SetDamageVisuals();
}

// Sets system damage based on an integrity value; invoked at load time AND from XRVesselCtrl whenever a damageable item is altered.
// Note that this is not called at runtime from the ship because the code merely needs to set the system settings
// (max engine thrust, etc.) to create damage.  In fact, that is what we do in this method.
// Also note that this method never *clear* warning lights, so you should first invoke ResetDamageStatus before invoking this
// method in a loop to set the status of each DamageItem.
void DeltaGliderXR1::SetDamageStatus(DamageItem item, double fracIntegrity)
{
// NOTE: because some warning lights can have multiple causes (e.g., left and right engines), we never CLEAR a warning flag here
#define SET_WARNING_LIGHT(wlIdx)  m_warningLights[static_cast<int>(wlIdx)] |= (fracIntegrity < 1.0)
#define UPDATE_DOOR_DMG(name)  UpdateDoorDamage(name##_status, name##_proc, fracIntegrity)

    switch (item)
    {   
    case DamageItem::LeftWing:
        lwingstatus = fracIntegrity;
        SET_WARNING_LIGHT(WarningLight::wlLwng);
        break;

    case DamageItem::RightWing:
        rwingstatus = fracIntegrity;
        SET_WARNING_LIGHT(WarningLight::wlRwng);
        break;

    case DamageItem::LeftAileron:
        aileronfail[0] = aileronfail[1] = ((fracIntegrity < 1.0) ? true : false);  // affect both aileron mesh groups here
        // NOTE: the control surfaces are failed later in our main damange method
        SET_WARNING_LIGHT(WarningLight::wlLail);
        break;

    case DamageItem::RightAileron:
        aileronfail[2] = aileronfail[3] = ((fracIntegrity < 1.0) ? true : false);  // affect both aileron mesh groups here
        // NOTE: the control surfaces are failed later in our main damange method
        SET_WARNING_LIGHT(WarningLight::wlRail);
        break;

    case DamageItem::LandingGear:
        UPDATE_DOOR_DMG(gear);
        SET_WARNING_LIGHT(WarningLight::wlGear);
        break;

    case DamageItem::Nosecone:
        UPDATE_DOOR_DMG(nose);
        SET_WARNING_LIGHT(WarningLight::wlNose);
        break;

    case DamageItem::RetroDoors:
        UPDATE_DOOR_DMG(rcover);
        SET_WARNING_LIGHT(WarningLight::wlRdor);
        break;

    case DamageItem::Hatch:
        UPDATE_DOOR_DMG(hatch);
        SET_WARNING_LIGHT(WarningLight::wlHtch);
        break;

    case DamageItem::Radiator:
        UPDATE_DOOR_DMG(radiator);
        SET_WARNING_LIGHT(WarningLight::wlRad);
        break;

    case DamageItem::Airbrake:
        UPDATE_DOOR_DMG(brake);
        SET_WARNING_LIGHT(WarningLight::wlAirb);
        break;

    case DamageItem::MainEngineLeft:
        SetThrusterMax0(th_main[0], MAX_MAIN_THRUST[GetXR1Config()->MainEngineThrust] * fracIntegrity);
        SET_WARNING_LIGHT(WarningLight::wlMain);
        break;

    case DamageItem::MainEngineRight:
        SetThrusterMax0(th_main[1], MAX_MAIN_THRUST[GetXR1Config()->MainEngineThrust] * fracIntegrity);
        SET_WARNING_LIGHT(WarningLight::wlMain);
        break;

    case DamageItem::SCRAMEngineLeft:
        ramjet->SetEngineIntegrity(0, fracIntegrity);
        SET_WARNING_LIGHT(WarningLight::wlScrm);
        break;

    case DamageItem::SCRAMEngineRight:
        ramjet->SetEngineIntegrity(1, fracIntegrity);
        SET_WARNING_LIGHT(WarningLight::wlScrm);
        break;

    case DamageItem::HoverEngineFore:
        SetHoverThrusterMaxAndIntegrity(0, fracIntegrity);
        SET_WARNING_LIGHT(WarningLight::wlHovr);
        break;

    case DamageItem::HoverEngineAft:
        SetHoverThrusterMaxAndIntegrity(1, fracIntegrity);
        SET_WARNING_LIGHT(WarningLight::wlHovr);
        break;

    case DamageItem::RetroEngineLeft:
        SetThrusterMax0(th_retro[0], MAX_RETRO_THRUST * fracIntegrity);
        SET_WARNING_LIGHT(WarningLight::wlRtro);
        break;

    case DamageItem::RetroEngineRight:
        SetThrusterMax0(th_retro[1], MAX_RETRO_THRUST * fracIntegrity);
        SET_WARNING_LIGHT(WarningLight::wlRtro);
        break;

    case DamageItem::RCS1:
    case DamageItem::RCS2:
    case DamageItem::RCS3:
    case DamageItem::RCS4:
    case DamageItem::RCS5:
    case DamageItem::RCS6:
    case DamageItem::RCS7:
    case DamageItem::RCS8:
    case DamageItem::RCS9:
    case DamageItem::RCS10:
    case DamageItem::RCS11:
    case DamageItem::RCS12:
    case DamageItem::RCS13:
    case DamageItem::RCS14:
        {
            int index = static_cast<int>(item) - static_cast<int>(DamageItem::RCS1);    // 0-13
            SetRCSThrusterMaxAndIntegrity(index, fracIntegrity);
            SET_WARNING_LIGHT(WarningLight::wlRcs);
        }
        break;

    default:        // should never happen!
        sprintf(oapiDebugString(), "WARNING: invalid damage ID in scenario file: %d", item);
        break;
    }

    // if any damage present, let's apply it (also calls SetDamageVisuals)
    if (IsDamagePresent())   
    {
        m_MWSActive = true;
        ApplyDamage();
        //UpdateDamageDialog (this);
    }
}

// Update a given door's state based on its integrity and proc (percentage-open state)
// Note: this method does not update the corresponding warning light state
void DeltaGliderXR1::UpdateDoorDamage(DoorStatus &doorStatus, double &doorProc, const double fracIntegrity)
{
    if (fracIntegrity < 1.0)
        doorStatus = DoorStatus::DOOR_FAILED;
    else 
    {
        // door is OK, so let's see if we are restoring this door from an OFFLINE state
        if (doorStatus == DoorStatus::DOOR_FAILED)
        {
            // door just came back online
            if (doorProc == 0.0)
                doorStatus = DoorStatus::DOOR_CLOSED;
            else if (doorProc == 1.0)
                doorStatus = DoorStatus::DOOR_OPEN;
            else  // door was halfway open or closed, but we have no way of knowing which...
            {
                doorStatus = DoorStatus::DOOR_CLOSING;   // ...so let's mark it 'closing'
            }
        }
    }
}

// Update the vessel performance (e.g., wing lift) to reflect any damage
// Note: items like RCS/engine thrust is computed internally, and should not be applied here.
void DeltaGliderXR1::ApplyDamage()
{
    // if crashed, use balance previously set by DoCrash()
    m_wingBalance = (IsCrashed() ? m_damagedWingBalance : ((rwingstatus-lwingstatus) * CRASH_WING_BALANCE_MULTIPLIER));
    const double minWingAreaPct = 0.2222;   // if crashed, will be 22.2% lift
    // total wing status is from 0...2, so we have to divide the wingAreaPct by 2 as well
    double wingArea = ((rwingstatus+lwingstatus) * (WING_AREA * ((1.0 - minWingAreaPct) / 2))) + (WING_AREA * minWingAreaPct);  // square meters 
    
    EditAirfoil(hwing, 0x09, _V(m_wingBalance, 0, m_centerOfLift), 0, 0, wingArea, 0);  // reset wing area and attack point

    if (rwingstatus < 1.0 || lwingstatus < 1.0) 
        m_MWSActive = true;

    SetDamageVisuals();
}

static UINT AileronGrp[8] = {29,51,30,52,35,55,36,54};

// Note: do not call this base class method from sub: visuals are vessel-specific
void DeltaGliderXR1::SetDamageVisuals()
{
    if (!exmesh) return;
    
    int i, j;
    
    // ailerons
    for (i = 0; i < 4; i++) 
    {
        for (j = 0; j < 2; j++)
            SetMeshGroupVisible(exmesh, AileronGrp[i*2+j], !aileronfail[i]);  // hide or show aileron 
    }
    
    // top hatch
    if (hatch_status == DoorStatus::DOOR_FAILED)
        SetXRAnimation(anim_hatch, 0.2);  // show partially deployed
}

// Ship crashed!
// pMsg = crash reason
// touchdownVerticalSpeed : if > 0, will be appended to the crash message.  NOTE: this should never be negative unless you want to ALWAYS kill the crew on impact!
void DeltaGliderXR1::DoCrash(const char *pMsg, double touchdownVerticalSpeed)
{
    // Note: allow crash during playback
    if (!GetXR1Config()->CrashDamageEnabled || !AllowDamageIfDockedCheck() || m_isCrashed)
        return;

    // msg will be blinked on the HUD by TakeoffAndLandingCalloutPostStep
    
    sprintf(m_crashMessage, "%s", pMsg);

    if (touchdownVerticalSpeed > 0)
    {
        char temp[128];
        sprintf(temp, "&Vertical Impact Velocity = %.2f m/s", touchdownVerticalSpeed);
        strcat(m_crashMessage, temp);
    }

    if (GetCrewMembersCount() > 0)   // anyone on board?
    {
        // at least one crew member is on board
        // NOTE: it is possible that the pilot did a soft belly landing, so we check that here
        // NOTE #2: it is possible to have a NEGATIVE touchdownVerticalSpeed here at this point if landing under light gravity conditions; this is
        // due to the Orbiter "bounce bug", where it bounces the ship.
        // NOTE #3: if crew is already dead, always display this message.  
        // Also note that if the crew is dead GetCrewMembersCount() returns 0, so be careful when modifying this section
        if ((touchdownVerticalSpeed <= 0) || (touchdownVerticalSpeed > CREW_IMPACT_DEATH_THRESHOLD) || (m_crewState == CrewState::DEAD))
        {
            strcat(m_crashMessage, "&You and the crew are DEAD!");
            KillCrew();
        }
        else   // the crew is NOT dead
        {
            if (touchdownVerticalSpeed > CREW_IMPACT_SEVERE_INJURY_THRESHOLD)
            {
                strcat(m_crashMessage, "&You and the crew&sustained SEVERE INJURIES,&but you survived!");
                m_crewState = CrewState::INCAPACITATED;
            }
            else if (touchdownVerticalSpeed > CREW_IMPACT_MODERATE_INJURY_THRESHOLD)
                strcat(m_crashMessage, "&You and the crew&sustained MODERATE INJURIES,&but you survived!");
            else if (touchdownVerticalSpeed > CREW_IMPACT_MINOR_INJURY_THRESHOLD)
                strcat(m_crashMessage, "&You and the crew&sustained MINOR INJURIES.");
            else    // light impact
            {
                strcat(m_crashMessage, "&You and the crew are UNINJURED.");

                // NOTE: cannot check for gear failed here, because gear fails on crash if wheels up
            }
        }
    }

    // play crash sound separately so it always plays immediately
    PlaySound(Crash, ST_Other);
    ShowWarning(nullptr, DeltaGliderXR1::ST_None, m_crashMessage, true);  // OK force this message because DoCrash() is only called once

    // set random new wing balance to make ship spiral
    m_damagedWingBalance = (oapiRand() * 6.0) + 3.0;  // was 8.0, but induced excessive spins sometime

    // now set left vs. right
    if (oapiRand() < 0.5)
        m_damagedWingBalance = -m_damagedWingBalance;

    // damage will be applied by the TestDemage routine since IsCrashed() == true now

    // must set this LAST so our last CRASHED warning gets through
    m_isCrashed = true;
}

// Ship crashed or had a hard landing!
// pMsg may be null; if null, default GEAR COLLAPSED message is used.
// setGearAnimState: true = set gear to random deployment, 0-0.25 or so.  false = don't change the gear's state
void DeltaGliderXR1::DoGearCollapse(const char *pMsg, double touchdownVerticalSpeed, bool setGearAnimState)
{
    // allow gear collapse during playback
    if (!GetXR1Config()->HardLandingsDamageEnabled || !AllowDamageIfDockedCheck())
        return;

    // play the gear collapse sound
    char temp[MAX_MESSAGE_LENGTH];
    if (pMsg == nullptr)
    {
        if (touchdownVerticalSpeed > CREW_IMPACT_MINOR_INJURY_THRESHOLD)
            sprintf(temp, "GEAR COLLAPSED!  Impact=%.3f m/s&You and the crew&sustained MINOR INJURIES.", touchdownVerticalSpeed);
        else    // light impact
            sprintf(temp, "GEAR COLLAPSED!  Impact=%.3f m/s&You and the crew&are UNINJURED.", touchdownVerticalSpeed);

        pMsg = temp;
    }
    
    ShowWarning("Gear Collapse.wav", ST_Other, pMsg, true);  // OK to force this becase we only do it once

    // show gear collapsed
    FailGear(setGearAnimState);

    // NOTE: do NOT delete the thruster group here; Orbiter does not like it!
    // hover thrusters damanged to random percentage of power
    for (int i=0; i < 2; i++)
    {
        // reduce the power somewhat
        double currentIntegrity = m_hoverEngineIntegrity[i];
        double frac = (oapiRand() + 0.20);  // thruster is still at least 20% functional
        if (frac > 0.89)
            frac = 0.89;  // hard cap
        double newIntegrity = currentIntegrity * frac;  // reduce max power
        SetHoverThrusterMaxAndIntegrity(i, newIntegrity);
    }

    // kill the hover engine thrust since we damaged them
    for (int i = 0; i < 2; i++)
        SetThrusterLevel(th_hover[i], 0);
}

// fail a door
// doorProc = nose_proc, ladder_proc, etc.
// anim = anim_gear, anim_rcover, etc.
void DeltaGliderXR1::FailDoor(double &doorProc, UINT anim)
{
    doorProc = fmod(oapiRand(), 0.3) + 0.2;     // damage range is 0.2 - 0.5
    SetXRAnimation(anim, doorProc);
}

// Set gear as FAILED
// setGearAnimState: true = show gear partially deployed if crashed into ground, false = don't change gear animation state
void DeltaGliderXR1::FailGear(bool setGearAnimState)
{
    if (setGearAnimState)
    {
        // fully compress the gear (only applies to subclasses)
        SetXRAnimation(m_animNoseGearCompression, 0);
        SetXRAnimation(m_animRearGearCompression, 0);

        // if ship CRASHED into the ground, show gear as partially deployed
        if (GetAltitude(ALTMODE_GROUND) < 100)  // close to planet's surface?
            FailDoor(gear_proc, anim_gear);
    }

    SetGearParameters(gear_proc);       // sets friction coeff and nosewheel steering
    SetMaxWheelbrakeForce(0);           // brakes disabled
    m_MWSActive = true;
    gear_status = DoorStatus::DOOR_FAILED;
    m_warningLights[static_cast<int>(WarningLight::wlGear)] = true;
}

// check HULL temperature and issue warning if necessary
// returns 0 if OK, > 0 = % max temperature exceeded^2
// doorOpen = is door on this surface open?
double DeltaGliderXR1::CheckTemperature(double tempK, double limitK, bool doorOpen)
{
    double retVal = 0;

    // NOTE: do not reset hull temp light here; we only turn it ON in this method
    // NOTE: do not set or reset door warning lights here; the dynamic pressure code sets and resets them.

    // adjust temp limit if door is open
    if (doorOpen)
        limitK = m_hullTemperatureLimits.doorOpen;

    // check for over-limit
    if (tempK > limitK)
    {
        m_warningLights[static_cast<int>(WarningLight::wlHtmp)] |= true;       // turn light ON

        // fail the structure if necessary
        const double dt = oapiGetSimStep();     // # of seconds since last timestep
        double exceededLimitMult = pow((tempK / limitK), 2);  // e.g. 1.21 = 10% over limit
        
        // # of seconds at this temp / average terminal failure interval (8 secs)
        double failureTimeFrac = dt / 8.0;
        double failureProbability = failureTimeFrac * exceededLimitMult;
        // sprintf(oapiDebugString(), "Damage failureProbablity=%lf", failureProbability);
        
        if (oapiRand() <= failureProbability)
        {
            retVal = (exceededLimitMult - 1.0);
            ShowWarning("Warning heat damage.wav", ST_WarningCallout, "WARNING: HEAT DAMAGE!", true);  // OK to force this because it will not get called each frame
        }
        else  // no new damage (yet!)
        {
            ShowWarning("Warning airframe overheating.wav", ST_WarningCallout, "WARNING: AIRFRAME OVERHEATING!");  // can't force this because we're in a post-step and the sound would never get to play since this gets call for each frame
        }
    }
    else    // let's check for a warning 
    {
        double criticalK = m_hullTemperatureLimits.criticalFrac * limitK;
        if (tempK >= criticalK)
        {
            m_warningLights[static_cast<int>(WarningLight::wlHtmp)] |= true;       // turn hull temp light ON
            ShowWarning("Warning Hull Temperature.wav", ST_WarningCallout, "WARNING: HULL TEMP. CRITICAL!");  // can't force this because we're in a post-step and the sound would never get to play since this gets call for each frame
        }
    }

    return retVal;  // remaining fraction is fraction over max heat; e.g., 0.2 = 20% over max heat value
}

// Fail a door if dynamic pressure exceeds limits, or issue a warning if a door is 
// open and dynamic pressure is high enough, if heating == 25% of failure heat level.
// Returns: true if door FAILED, false otherwise
bool DeltaGliderXR1::CheckDoorFailure(DoorStatus *doorStatus)
{
    bool retVal = false;        // assume no damage

    // do not re-check or warn if door already failed
    bool doorOpen = ((*doorStatus != DoorStatus::DOOR_CLOSED) && (*doorStatus != DoorStatus::DOOR_FAILED));
    const double dt = oapiGetSimStep();

    if (doorOpen)
    {
        // Door is open!  Check for damage or failure.
        // NOTE: once a door fails, it can only be repaired via the damage dialog; therefore, we never reset it here
        double dynP = GetDynPressure();
        if (doorStatus == &nose_status)
        {
            const double doorProc = nose_proc;
            if (IS_DOOR_FAILURE(m_noseconeTemp, OPEN_NOSECONE_LIMIT))
            {
                char msg[128];
                sprintf(msg, "%s FAILED due to excessive&heat and/or dynamic pressure!", NOSECONE_LABEL);
                ShowWarning("Warning Nosecone Failure.wav", ST_WarningCallout, msg, true); // OK to force this here
                *doorStatus = DoorStatus::DOOR_FAILED;
                m_warningLights[static_cast<int>(WarningLight::wlNose)] = true;
                FailDoor(nose_proc, anim_nose);
                retVal = true;   // new damage
            }
            else if (IS_DOOR_WARNING(m_noseconeTemp, OPEN_NOSECONE_LIMIT))     // this will check temperature limit for hover doors as well
            {
                char msg[128];
                sprintf(msg, "%s is open:&close it or reduce speed!", NOSECONE_LABEL);
                ShowWarning(WARNING_NOSECONE_OPEN_WAV, ST_WarningCallout, msg);
                m_warningLights[static_cast<int>(WarningLight::wlNose)] = true;
            }
            else if (IS_DOOR_FAILED() == false) 
                m_warningLights[static_cast<int>(WarningLight::wlNose)] = false;   // reset light
        }
        if (doorStatus == &hoverdoor_status)
        {
            // NOTE: the hover doors cannot fail due to dynamic pressure, so we only check them for temperature here via DOOR_WARNING
            if ((IS_DOOR_FAILED() == false) && (OPEN_DOOR_WARN_TEMP(m_noseconeTemp)))
            {
                ShowWarning("Warning Hover Doors Open.wav", ST_WarningCallout, "Hover doors are open:&close them or reduce speed!");
                // no warning light for hover doors since they can't be damaged for now
            }
        }
        else if (doorStatus == &rcover_status)
        {
            const double doorProc = rcover_proc;

            if (IS_DOOR_FAILURE(m_leftWingTemp,  RETRO_DOOR_LIMIT) || 
                IS_DOOR_FAILURE(m_rightWingTemp, RETRO_DOOR_LIMIT))
            {
                ShowWarning("Warning Retro Door Failure.wav", ST_WarningCallout, "Retro Doors FAILED due to excessive&heat and/or dynamic pressure!", true); // force this
                *doorStatus = DoorStatus::DOOR_FAILED;
                m_warningLights[static_cast<int>(WarningLight::wlRdor)] = true;
                FailDoor(rcover_proc, anim_rcover);
                retVal = true;   // new damage
            }
            else if (IS_DOOR_WARNING(m_leftWingTemp,  RETRO_DOOR_LIMIT) || 
                     IS_DOOR_WARNING(m_rightWingTemp, RETRO_DOOR_LIMIT))
            {
                ShowWarning("Warning Retro Doors Open.wav", ST_WarningCallout, "Retro Doors are open:&close them or reduce speed!");
                m_warningLights[static_cast<int>(WarningLight::wlRdor)] = true;
            }
            else if (IS_DOOR_FAILED() == false) 
                m_warningLights[static_cast<int>(WarningLight::wlRdor)] = false;   // reset light
        }
        else if (doorStatus == &hatch_status)
        {
            const double doorProc = hatch_proc;

            if (IS_DOOR_FAILURE(m_cockpitTemp, HATCH_OPEN_LIMIT))
            {
                ShowWarning("Warning Hatch Failure.wav", ST_WarningCallout, "Top Hatch FAILED due to excessive&heat and/or dynamic pressure!", true); // force this
                *doorStatus = DoorStatus::DOOR_FAILED;
                m_warningLights[static_cast<int>(WarningLight::wlHtch)] = true;
                FailDoor(hatch_proc, anim_hatch);
                retVal = true;   // new damage
            }
            else if (IS_DOOR_WARNING(m_cockpitTemp, HATCH_OPEN_LIMIT))
            {
                ShowWarning("Warning Hatch Open.wav", ST_WarningCallout, "Top Hatch is open:&close it or reduce speed!");
                m_warningLights[static_cast<int>(WarningLight::wlHtch)] = true;
            }
            else if (IS_DOOR_FAILED() == false) 
                m_warningLights[static_cast<int>(WarningLight::wlHtch)] = false;   // reset light
        }
        else if (doorStatus == &radiator_status)
        {
            const double doorProc = radiator_proc;
            if (IS_DOOR_FAILURE(m_topHullTemp, RADIATOR_LIMIT))
            {
                ShowWarning("Warning Radiator Failure.wav", ST_WarningCallout, "Radiator FAILED due to excessive&heat and/or dynamic pressure!", true); // force this
                *doorStatus = DoorStatus::DOOR_FAILED;
                m_warningLights[static_cast<int>(WarningLight::wlRad)] = true;
                FailDoor(radiator_proc, anim_radiator);
                retVal = true;   // new damage
            }
            else if (IS_DOOR_WARNING(m_topHullTemp, RADIATOR_LIMIT))
            {
                ShowWarning("Warning Radiator Deployed.wav", ST_WarningCallout, "Radiator is deployed:&stow it or reduce speed!");
                m_warningLights[static_cast<int>(WarningLight::wlRad)] = true;
            }
            else if (IS_DOOR_FAILED() == false) 
                m_warningLights[static_cast<int>(WarningLight::wlRad)] = false;   // reset light
        }
        else if (doorStatus == &gear_status)
        {
            const double doorProc = gear_proc;

            // use nosecone temps to check gear-down damage
            if (IS_DOOR_FAILURE(m_noseconeTemp, GEAR_LIMIT))
            {
                ShowWarning("Warning Gear Failure.wav", ST_WarningCallout, "Landing Gear FAILED due to excessive&heat and/or dynamic pressure!", true); // force this
                *doorStatus = DoorStatus::DOOR_FAILED;
                FailGear(true);     // also invoke FailDoor to show gear partially collapsed
                m_warningLights[static_cast<int>(WarningLight::wlGear)] = true;
                retVal = true;   // new damage
            }
            else if (IS_DOOR_WARNING(m_noseconeTemp, GEAR_LIMIT))
            {
                ShowWarning("Warning Gear Deployed.wav", ST_WarningCallout, "Gear is deployed:&retract it or reduce speed!");
                m_warningLights[static_cast<int>(WarningLight::wlGear)] = true;
            }
            else if (IS_DOOR_FAILED() == false) 
                m_warningLights[static_cast<int>(WarningLight::wlGear)] = false;   // reset light
        }
    }
    else if (IS_DOOR_FAILED() == false)
    {
        // door is closed; reset the warning light
        if (doorStatus == &nose_status)
            m_warningLights[static_cast<int>(WarningLight::wlNose)] = false;
        else if (doorStatus == &rcover_status)
            m_warningLights[static_cast<int>(WarningLight::wlRdor)] = false;
        else if (doorStatus == &hatch_status)
            m_warningLights[static_cast<int>(WarningLight::wlHtch)] = false;
        else if (doorStatus == &radiator_status)
            m_warningLights[static_cast<int>(WarningLight::wlRad)] = false;
        else if (doorStatus == &gear_status)
            m_warningLights[static_cast<int>(WarningLight::wlGear)] = false;
    }

    return retVal;
}

// check SCRAM ENGINE temperature and issue warning if necessary
// returns 0 if OK, > 0 = % max temperature exceeded^2 * 2; e.g., 0.42 = 10% over limit
double DeltaGliderXR1::CheckScramTemperature(double tempK, double limitK)
{
    double retVal = 0;

    // turn on SCRAM light if engines over-temp or if either is damaged; otherwise, turn it off
    m_warningLights[static_cast<int>(WarningLight::wlScrm)] = ((tempK > limitK) || (ramjet->GetEngineIntegrity(0) < 1.0) || (ramjet->GetEngineIntegrity(1) < 1.0));

    // check for over-limit
    if (tempK > limitK)
    {
        // fail the engines if necessary
        const double dt = oapiGetSimStep();     // # of seconds since last timestep
        const double exceededLimitMult = pow((tempK / limitK), 2);  // e.g. 1.21 = 10% over limit
        
        // # of seconds at this temp / average engine failure interval (8 secs)
        double failureTimeFrac = dt / 8.0;
        double failureProbability = failureTimeFrac * exceededLimitMult;
        // sprintf(oapiDebugString(), "SCRAM Damage failureProbablity=%lf", failureProbability);
        
        if (oapiRand() <= failureProbability)
        {
            retVal = ((exceededLimitMult - 1.0) * 2); // e.g., 0.42 = 10% over limit
            ShowWarning("Warning SCRAM Engine Damage.wav", ST_WarningCallout, "WARNING: SCRAM ENGINE HEAT&DAMAGE! CLOSE THE SCRAM DOORS!", true);  // OK to force this because it will not get called each frame
        }
        else  // no new damage (yet!)
        {
            ShowWarning("Warning SCRAM Engines Overheating.wav", ST_WarningCallout, "WARNING: SCRAM TEMPERATURE&CRITICAL! CLOSE THE SCRAM DOORS!"); // can't force this because we're in a post-step and the sound would never get to play since this gets call for each frame
        }
    }
    else    // let's check for a warning 
    {
        // check for temps approaching limits
        if ((tempK / limitK) > 0.97)
            ShowWarning("Warning SCRAM Temperature.wav", ST_WarningCallout, "SCRAM engines approaching limit!&Close the SCRAM doors!");
        else    // let's check for special case where mach >= n and temperature is greater than ambient (need to signal the pilot ASAP during reentry)
        {
            const double extTemp = GetExternalTemperature();
            const double mach = GetMachNumber();
            // only play warning is SCRAM throttle is CLOSED
            const double throttleLevelX2 = GetThrusterLevel(th_scram[0]) + GetThrusterLevel(th_scram[1]);
            if ((throttleLevelX2 == 0.0) && (tempK > extTemp) && (mach >= MACH_REENTRY_WARNING_THRESHOLD) && ((scramdoor_status != DoorStatus::DOOR_CLOSED) && (scramdoor_status != DoorStatus::DOOR_CLOSING)))
            {
                ShowWarning("Warning SCRAM doors open.wav", ST_WarningCallout, "WARNING: SCRAM DOORS OPEN!");
            }
        }
    }

    return retVal;  // remaining fraction is fraction over max heat; e.g., 0.2 = 20% over max heat value
}

//
// Perform SetThrusterMax0 for a hover engine.  This method is necessary because we must track hover engine
// damage separately via m_hoverEngineIntegrity: the hover engine max thrust is set when the engines are gimbaled,
// and so we cannot rely on that to track engine damage.
// 
// engineIndex: 0=fore, 1=aft
// integrityFrac: 0...1
//
void DeltaGliderXR1::SetHoverThrusterMaxAndIntegrity(int engineIndex, double integrityFrac)
{
    const double maxThrustInKN = MAX_HOVER_THRUST[GetXR1Config()->HoverEngineThrust] * integrityFrac;

    SetThrusterMax0(th_hover[engineIndex], maxThrustInKN);
    m_hoverEngineIntegrity[engineIndex] = integrityFrac;   // update internal damage status too
}

void DeltaGliderXR1::SetRCSThrusterMaxAndIntegrity(int index, double integrityFrac)
{
    double maxThrust = GetRCSThrustMax(index);
    SetThrusterMax0(th_rcs[index], maxThrust * integrityFrac);
    m_rcsIntegrityArray[index] = integrityFrac;     // set internal damage array as well
}

// Fail the ailerons/elevons/elevator trim if damaged
void DeltaGliderXR1::FailAileronsIfDamaged()
{
    // delete the aileron and elevator control surfaces if there are marked as damaged
    // For the purposes of XR vessels, anytime an aileron is damaged the elevator is damaged as well,
    // regardless of whether they are the same control surface.
    if (aileronfail[0] || aileronfail[1])
    {
        // fail left aileron
        if (hLeftAileron)
        {
            DelControlSurface (hLeftAileron);
            hLeftAileron = 0;
        }
    }

    if (aileronfail[2] || aileronfail[3])
    {
        // fail right aileron
        if (hRightAileron)
        {
            DelControlSurface (hRightAileron);
            hRightAileron = 0;
        }
    }

    // if any aileron damaged, fail the elevators and elevator trim as well
    if (!AreElevatorsOperational())
    {
        if (hElevator)  // not already failed
        {
            DelControlSurface(hElevator);
            hElevator = 0;
        }
        
        if (hElevatorTrim)
        {
            DelControlSurface(hElevatorTrim);
            hElevatorTrim = 0;
        }
    }
}
