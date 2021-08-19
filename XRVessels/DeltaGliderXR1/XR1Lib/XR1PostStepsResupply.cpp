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

#include "XR1FuelPostSteps.h"
#include "AreaIDs.h"
#include "XRPayloadBay.h"

//---------------------------------------------------------------------------

ResupplyPostStep::ResupplyPostStep(DeltaGliderXR1& vessel) :
    XR1PrePostStep(vessel),
    m_prevResupplyEnabledStatus(false), m_prevFuelHatchStatus(DoorStatus::DOOR_CLOSED), m_prevLoxHatchStatus(DoorStatus::DOOR_CLOSED), m_prevExternalCoolingStatus(DoorStatus::DOOR_CLOSED),
    m_refuelingSequenceStartSimt(-1), m_loxSequenceStartSimt(-1), m_externalCoolingSequenceStartSimt(-1),
    m_resupplyStartupTime(5.0), // time in seconds
    m_prevSimt(-1), m_resupplyMovementFirstDetectedSimt(-1)
{
    // create our pressure objects; each line has a slightly different pressure rate
    m_pMainLinePressure = new LinePressure(GetXR1().m_mainExtLinePressure, GetXR1().m_nominalMainExtLinePressure, GetXR1().m_mainSupplyLineStatus, GetXR1().m_mainFuelFlowSwitch, MAIN_SUPPLY_PSI_LIMIT, PRESSURE_MOVEMENT_RATE * 1.14, GetXR1());
    m_pScramLinePressure = new LinePressure(GetXR1().m_scramExtLinePressure, GetXR1().m_nominalScramExtLinePressure, GetXR1().m_scramSupplyLineStatus, GetXR1().m_scramFuelFlowSwitch, SCRAM_SUPPLY_PSI_LIMIT, PRESSURE_MOVEMENT_RATE * 1.0, GetXR1());
    m_pApuLinePressure = new LinePressure(GetXR1().m_apuExtLinePressure, GetXR1().m_nominalApuExtLinePressure, GetXR1().m_apuSupplyLineStatus, GetXR1().m_apuFuelFlowSwitch, APU_SUPPLY_PSI_LIMIT, PRESSURE_MOVEMENT_RATE * 0.92, GetXR1());
    m_pLoxLinePressure = new LinePressure(GetXR1().m_loxExtLinePressure, GetXR1().m_nominalLoxExtLinePressure, GetXR1().m_loxSupplyLineStatus, GetXR1().m_loxFlowSwitch, LOX_SUPPLY_PSI_LIMIT, PRESSURE_MOVEMENT_RATE * 0.86, GetXR1());
}

ResupplyPostStep::~ResupplyPostStep()
{
    // clean up
    delete m_pMainLinePressure;
    delete m_pScramLinePressure;
    delete m_pApuLinePressure;
    delete m_pLoxLinePressure;
}

void ResupplyPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // assume coolant NOT flowing; this is reset for each poststep below
    GetXR1().m_isExternalCoolantFlowing = false;

    // may resupply if grounded and stopped or if docked
    // Note: because of an Orbiter 2016 core anomaly (or feature?) the ship can lose GroundContact and/or have spurious groundspeed on startup, so we give the ship 2 seconds to settle down first.
    bool resupplyEnabled = (GetXR1().IsLanded() || GetXR1().IsDocked() || simt < STARTUP_DELAY_BEFORE_ISLANDED_VALID);

    // begin workaround ========================================================================
    /* NOTE: we need to work around some odd Orbiter core issue here:
       For some odd reason, switching back to the XR5 causes a very minute amount of movement of the ship for one
       frame until ground contact is reestablished the next frame:
        airspeed = 5.25e-10
        groundContact = false

        As a workaround, we only break resupply contact if we've been moving for more than one 2/10th of a second AND
        if we have at least detected some motion (m_resupplyMovementFirstDetectedSimt >= 0): the odd disconnect
        happens on startup, too.
    */
    if (resupplyEnabled == false)
    {
        // check whether we just stated moving (i.e., whether resupply was just disconnected since the previous timestep)
        if ((m_resupplyMovementFirstDetectedSimt <= 0) && m_prevResupplyEnabledStatus)
            m_resupplyMovementFirstDetectedSimt = simt;  // remember when movement started

        if (m_resupplyMovementFirstDetectedSimt >= 0)       // has the ship moved?
        {
            // if we haven't been moving long enough to break contact yet, keep resupply enabled.
            const double movementTime = simt - m_resupplyMovementFirstDetectedSimt;  // will never be negative
            if (movementTime < 0.20)  // moving less than 2/10th second?
                resupplyEnabled = true;  // still OK
        }
        else  // the ship has not moved (we have detected no movement yet), so resupply is still enabled (necessary for startup b0rk by Orbiter core)
        {
            resupplyEnabled = true;
        }
    }
    else   // resupply is enabled, so reset 'movement first detected' latch
    {
        m_resupplyMovementFirstDetectedSimt = -1;
    }
    // end workaround ========================================================================

    if (resupplyEnabled)
    {
        //
        // Check whether fuel hatch is open
        //
        if (GetXR1().fuelhatch_status == DoorStatus::DOOR_OPEN)
        {
            // check if the hatch just opened
            if (m_prevFuelHatchStatus != DoorStatus::DOOR_OPEN)
            {
                // start the refueling sequence countdown; this sound is NOT the hatch opening; it is the supply line extending from outside the ship
                m_refuelingSequenceStartSimt = simt + m_resupplyStartupTime;
                GetXR1().LoadXR1Sound(GetXR1().FuelResupplyLine, "Resupply Line Extend.wav", XRSound::PlaybackType::InternalOnly);
                GetXR1().PlaySound(GetXR1().FuelResupplyLine, DeltaGliderXR1::ST_Other, RESUPPLY_LINE_EXTEND_VOL);
            }

            // check whether refueling online yet
            if (simt >= m_refuelingSequenceStartSimt)
            {
                // check whether we just reached a refueling enabled state
                if (m_prevSimt < m_refuelingSequenceStartSimt)
                {
                    GetXR1().LoadXR1Sound(GetXR1().FuelResupplyLine, "Resupply Line Attach.wav", XRSound::PlaybackType::InternalOnly);
                    GetXR1().PlaySound(GetXR1().FuelResupplyLine, DeltaGliderXR1::ST_Other);   // use max volume for this
                    GetXR1().ShowInfo("Refueling Systems Online.wav", DeltaGliderXR1::ST_InformationCallout, "External fuel line attached;&refueling systems ONLINE.");

                    // determine which fuel lines should have pressure
                    const XR1ConfigFileParser& config = *GetXR1().GetXR1Config();
                    const bool isDocked = GetXR1().IsDocked();
                    const bool onEarth = GetXR1().IsLandedOnEarth();

                    bool mainFuelAvailable = false;
                    bool scramFuelAvailable = false;
                    bool apuFuelAvailable = false;

                    if (isDocked)
                    {
                        mainFuelAvailable = config.AllowDockResupply[TANK_MAIN];
                        scramFuelAvailable = config.AllowDockResupply[TANK_SCRAM];
                        apuFuelAvailable = config.AllowDockResupply[TANK_APU];
                    }
                    else  // we are grounded
                    {
                        mainFuelAvailable = (config.AllowEarthOnlyResupply[TANK_MAIN] ? onEarth : config.AllowGroundResupply[TANK_MAIN]);
                        scramFuelAvailable = (config.AllowEarthOnlyResupply[TANK_SCRAM] ? onEarth : config.AllowGroundResupply[TANK_SCRAM]);
                        apuFuelAvailable = (config.AllowEarthOnlyResupply[TANK_APU] ? onEarth : config.AllowGroundResupply[TANK_APU]);
                    }

                    // mark for "target nominal pressure" for all fuel lines; this will start the pressure gauges moving
                    if (mainFuelAvailable)
                        m_pMainLinePressure->m_pressureTarget = -1;

                    if (scramFuelAvailable)
                        m_pScramLinePressure->m_pressureTarget = -1;

                    if (apuFuelAvailable)
                        m_pApuLinePressure->m_pressureTarget = -1;

                    // refueling begins at next timestep
                }
                else    // refueling online!
                {
                    PerformRefueling(simt, simdt, mjd);     // will check individual line switches to determine flow
                }
            }
        }
        else    // fuel hatch is CLOSED
        {
            m_refuelingSequenceStartSimt = -1;  // refueling disabled now
            if (m_prevFuelHatchStatus == DoorStatus::DOOR_OPEN)     // was the hatch just closed?
            {
                // play a thump of the hatch closing
                GetXR1().LoadXR1Sound(GetXR1().FuelResupplyLine, "Resupply Line Attach.wav", XRSound::PlaybackType::InternalOnly);
                GetXR1().PlaySound(GetXR1().FuelResupplyLine, DeltaGliderXR1::ST_Other);   // use max volume for this
                GetXR1().ShowInfo("Refueling Systems Offline.wav", DeltaGliderXR1::ST_InformationCallout, "External fuel line detached;&refueling systems OFFLINE.");
                GetXR1().CloseFuelHatch(true);
                DisconnectFuelLines();  // reset
            }
        }

        //
        // Check whether lox hatch is open
        //
        if (GetXR1().loxhatch_status == DoorStatus::DOOR_OPEN)
        {
            // check if the hatch just opened
            if (m_prevLoxHatchStatus != DoorStatus::DOOR_OPEN)
            {
                // start the LOX resupply sequence countdown
                m_loxSequenceStartSimt = simt + m_resupplyStartupTime;
                GetXR1().LoadXR1Sound(GetXR1().LoxResupplyLine, "Resupply Line Extend.wav", XRSound::PlaybackType::InternalOnly);
                GetXR1().PlaySound(GetXR1().LoxResupplyLine, DeltaGliderXR1::ST_Other, RESUPPLY_LINE_EXTEND_VOL);
            }

            // check whether LOX resupply online yet
            if (simt >= m_loxSequenceStartSimt)
            {
                // check whether we just reached a refueling enabled state
                if (m_prevSimt < m_loxSequenceStartSimt)
                {
                    GetXR1().LoadXR1Sound(GetXR1().LoxResupplyLine, "Resupply Line Attach.wav", XRSound::PlaybackType::InternalOnly);
                    GetXR1().PlaySound(GetXR1().LoxResupplyLine, DeltaGliderXR1::ST_Other);   // use max volume for this
                    GetXR1().ShowInfo("LOX Resupply Systems Online.wav", DeltaGliderXR1::ST_InformationCallout, "External LOX line attached;&LOX resupply systems ONLINE.");

                    // determine if the LOX line should have pressure
                    const XR1ConfigFileParser& config = *GetXR1().GetXR1Config();
                    bool loxAvailable = false;

                    if (GetXR1().IsDocked())
                        loxAvailable = config.AllowDockResupply[TANK_LOX];
                    else  // we are grounded
                        loxAvailable = (config.AllowEarthOnlyResupply[TANK_LOX] ? GetXR1().IsLandedOnEarth() : config.AllowGroundResupply[TANK_LOX]);

                    // mark for "target nominal pressure" for all fuel lines; this will start the pressure gauges moving
                    if (loxAvailable)
                        m_pLoxLinePressure->m_pressureTarget = -1;     // mark for "target nominal pressure" to start the pressure gauge moving

                    // resupply begins at next timestep
                }
                else    // LOX resupply online!
                {
                    PerformLoxResupply(simt, simdt, mjd);  // will check LOX switch to determine flow
                }
            }
        }
        else    // LOX hatch is CLOSED
        {
            m_loxSequenceStartSimt = -1;  // refueling disabled now
            if (m_prevLoxHatchStatus == DoorStatus::DOOR_OPEN)     // was the hatch just closed?
            {
                GetXR1().StopSound(GetXR1().LoxResupplyLine);
                GetXR1().ShowInfo("LOX Resupply Systems Offline.wav", DeltaGliderXR1::ST_InformationCallout, "External LOX line detached;&LOX resupply systems OFFLINE.");
                GetXR1().CloseLoxHatch(true);  // need to reset line pressures to 0, etc.
                DisconnectLoxLine();  // reset
            }
        }

        //
        // Check whether external cooling hatch is open
        //
        if (GetXR1().externalcooling_status == DoorStatus::DOOR_OPEN)
        {
            // check if the hatch just opened
            if (m_prevExternalCoolingStatus != DoorStatus::DOOR_OPEN)
            {
                // start the external cooling sequence countdown
                m_externalCoolingSequenceStartSimt = simt + m_resupplyStartupTime;
                // TODO: see if I still need this for other reasons now that I've switched to XRSound
                // WORK AROUND ORBITERSOUND BUG: if PlaySound invoked during the first frame of the simulation
                // (e.g., by starting paused with external cooling online), OrbiterSound crashes (trashes the stack
                // and executes a 'ret' to a bad pointer).  Therefore we don't play the extend sound if the simt < 0.5.
                if (simt > 0.5)
                {
                    GetXR1().LoadXR1Sound(GetXR1().ExternalCoolingLine, "Resupply Line Extend.wav", XRSound::PlaybackType::InternalOnly);
                    GetXR1().PlaySound(GetXR1().ExternalCoolingLine, DeltaGliderXR1::ST_Other, RESUPPLY_LINE_EXTEND_VOL);
                }
            }

            // check whether external cooling online yet
            if (simt >= m_externalCoolingSequenceStartSimt)
            {
                // check whether we just reached an external cooling enabled state
                if (m_prevSimt < m_externalCoolingSequenceStartSimt)
                {
                    GetXR1().LoadXR1Sound(GetXR1().ExternalCoolingLine, "Resupply Line Attach.wav", XRSound::PlaybackType::InternalOnly);
                    GetXR1().PlaySound(GetXR1().ExternalCoolingLine, DeltaGliderXR1::ST_Other);   // use max volume for this
                    GetXR1().ShowInfo("External Cooling Online.wav", DeltaGliderXR1::ST_InformationCallout, "External coolant line attached;&External cooling systems ONLINE.");
                    // resupply begins at next timestep
                }
                else    // external cooling online!
                {
                    GetXR1().m_isExternalCoolantFlowing = true;

                    // ship coolant is actually cooled by UpdateCoolantTempPostStep
                }
            }
        }
        else    // external cooling hatch is CLOSED
        {
            m_externalCoolingSequenceStartSimt = -1;   // external cooling disabled now
            if (m_prevExternalCoolingStatus == DoorStatus::DOOR_OPEN)      // was the hatch just closed?
            {
                GetXR1().StopSound(GetXR1().ExternalCoolingLine);
                GetXR1().ShowInfo("External Cooling Offline.wav", DeltaGliderXR1::ST_InformationCallout, "External cooling line detached;&External cooling systems OFFLINE.");
                GetXR1().CloseExternalCoolingHatch(true);
            }
        }
    }
    else    // resupply DISABLED
    {
        // check whether we just started moving or just undocked
        if (m_prevResupplyEnabledStatus)
        {
            // we were enabled the previous timestep; close all open hatches and show a warning if either hatch is still open
            if ((GetXR1().fuelhatch_status != DoorStatus::DOOR_CLOSED) || (GetXR1().loxhatch_status != DoorStatus::DOOR_CLOSED))
            {
                // close the hatches and sound a hatch thump
                GetXR1().CloseFuelHatch(true);  // will reset line pressures to 0
                GetXR1().CloseLoxHatch(true);   // will reset line pressure to 0
                DisconnectFuelLines();      // reset
                DisconnectLoxLine();
                // stop the fuel/lox flowing sounds
                GetXR1().StopSound(GetXR1().FuelResupplyLine);
                GetXR1().StopSound(GetXR1().LoxResupplyLine);

                GetXR1().ShowWarning("Warning Resupply Operations Terminated.wav", DeltaGliderXR1::ST_WarningCallout, "WARNING: Resupply ops terminated;&FUEL and LOX hatches closed.");
            }

            // check for external cooling
            if (GetXR1().externalcooling_status != DoorStatus::DOOR_CLOSED)
            {
                // close the hatch and sound a hatch thump
                GetXR1().CloseExternalCoolingHatch(true);

                // no sound playing while external cooling active, so no sound to stop here

                GetXR1().ShowWarning("Warning External Cooling Offline.wav", DeltaGliderXR1::ST_WarningCallout, "WARNING: External cooling offline;&Cooling hatch closed.");
            }

            // resupply no longer available, so reset timestamps for next time
            m_refuelingSequenceStartSimt = -1;
            m_loxSequenceStartSimt = -1;
            m_externalCoolingSequenceStartSimt = -1;
        }
    }

    // adjust pressure for all lines; this occurs each step regardless of state
    m_pMainLinePressure->AdjustPressure(simt, simdt, mjd);
    m_pScramLinePressure->AdjustPressure(simt, simdt, mjd);
    m_pApuLinePressure->AdjustPressure(simt, simdt, mjd);
    m_pLoxLinePressure->AdjustPressure(simt, simdt, mjd);

    // NOTE: no sound for external coolant flowing
    // handle fuel/lox flow sounds; handled by a single sound
    int flowCount = static_cast<int>(GetXR1().m_mainFuelFlowSwitch) +
        static_cast<int>(GetXR1().m_scramFuelFlowSwitch) +
        static_cast<int>(GetXR1().m_apuFuelFlowSwitch) +
        static_cast<int>(GetXR1().m_loxFlowSwitch);

    if (flowCount > 0)
    {
        // determine volume level
        const int volume = FUEL_RESUPPLY_BASE_VOL + ((flowCount - 1) * FUEL_RESUPPLY_INC_VOL);

        // always play this sound so we can adjust the volume
        GetXR1().PlaySound(GetXR1().FuelResupply, DeltaGliderXR1::ST_Other, volume, true);   // loop this sound
    }
    else  // all flow is HALTED
    {
        GetXR1().StopSound(GetXR1().FuelResupply);
    }

    // save data for next timestep
    m_prevSimt = simt;
    m_prevResupplyEnabledStatus = resupplyEnabled;
    m_prevFuelHatchStatus = GetXR1().fuelhatch_status;
    m_prevLoxHatchStatus = GetXR1().loxhatch_status;
    m_prevExternalCoolingStatus = GetXR1().externalcooling_status;
}

// reset fuel pressure state; invoked when refueling line disconnected
void ResupplyPostStep::DisconnectFuelLines()
{
    m_pMainLinePressure->Disconnected();
    m_pScramLinePressure->Disconnected();
    m_pApuLinePressure->Disconnected();
}

// reset lox pressure state; invoked when refueling line disconnected
void ResupplyPostStep::DisconnectLoxLine()
{
    m_pLoxLinePressure->Disconnected();
}

// Check individual refueling lines and handle refueling operations; this is only invoked when 
// refueling systems are ONLINE; however, FUEL PRESSURE may be building yet.
void ResupplyPostStep::PerformRefueling(const double simt, const double simdt, const double mjd)
{
    const XR1ConfigFileParser& config = *GetXR1().GetXR1Config();
    const bool onEarth = GetXR1().IsLandedOnEarth();

    bool mainFuelAvailable = false;
    bool scramFuelAvailable = false;
    bool apuFuelAvailable = false;

    if (GetXR1().IsDocked())
    {
        mainFuelAvailable = config.AllowDockResupply[TANK_MAIN];
        scramFuelAvailable = config.AllowDockResupply[TANK_SCRAM];
        apuFuelAvailable = config.AllowDockResupply[TANK_APU];
    }
    else  // we are grounded
    {
        mainFuelAvailable = (config.AllowEarthOnlyResupply[TANK_MAIN] ? onEarth : config.AllowGroundResupply[TANK_MAIN]);
        scramFuelAvailable = (config.AllowEarthOnlyResupply[TANK_SCRAM] ? onEarth : config.AllowGroundResupply[TANK_SCRAM]);
        apuFuelAvailable = (config.AllowEarthOnlyResupply[TANK_APU] ? onEarth : config.AllowGroundResupply[TANK_APU]);
    }

    // 
    // Handle pressure climb / variance for all three fuel lines (main, scram, apu)
    //

    if (mainFuelAvailable && GetXR1().m_mainFuelFlowSwitch)
        FlowMainFuel(simt, simdt, mjd);

    if (scramFuelAvailable && GetXR1().m_scramFuelFlowSwitch)
        FlowScramFuel(simt, simdt, mjd);

    if (apuFuelAvailable && GetXR1().m_apuFuelFlowSwitch)
        FlowApuFuel(simt, simdt, mjd);
}

// Invoked at each timestep when fuel flowing into main tank
void ResupplyPostStep::FlowMainFuel(const double simt, const double simdt, const double mjd)
{
    double mainTankQty = GetXR1().GetXRPropellantMass(GetXR1().ph_main);
    const double mainTankMaxQty = GetXR1().GetXRPropellantMaxMass(GetXR1().ph_main);

    bool haltFlow = false;

    // if main tank already full, we cannot refuel a full tank
    if (mainTankQty >= mainTankMaxQty)
    {
        GetXR1().ShowInfo("Main Fuel Tanks Full.wav", DeltaGliderXR1::ST_InformationCallout, "Main fuel tanks already full.");
        haltFlow = true;
    }
    else   // tanks not full yet
    {
        // adjust by pressure
        const double pressureFrac = GetXR1().m_mainExtLinePressure / GetXR1().m_nominalMainExtLinePressure;  // 0...1
        const double fuelFlowForThisStep = (FUEL_LOAD_RATE * simdt * pressureFrac); // main tank loads with no load fraction (i.e., effectively 1.0)

        mainTankQty += fuelFlowForThisStep;

        // check limits
        const char* pMsg = nullptr;
        if (mainTankQty > mainTankMaxQty)  // main tank overflow
        {
            mainTankQty = mainTankMaxQty;

            // halt fuel flow ONLY if cross-feed is not set to RCS; i.e., fuel is not draining into the RCS tank
            if (GetXR1().m_xfeedMode != XFEED_MODE::XF_RCS)
                haltFlow = true;

            // no need for a msg here; the FuelCalloutsPostStep will handle it
        }

        GetXR1().SetXRPropellantMass(GetXR1().ph_main, mainTankQty);
    }

    // flow sounds are handled by our caller
    // NOTE: "main fuel tank full" is handled by our FuelCalloutsPostStep
    if (haltFlow)
    {
        GetXR1().m_mainFuelFlowSwitch = false;

        // refresh the switch and its LED
        GetXR1().TriggerRedrawArea(AID_MAINSUPPLYLINE_SWITCH);
        GetXR1().TriggerRedrawArea(AID_MAINSUPPLYLINE_SWITCH_LED);

        // flow sound will stop next timestep 
    }
}

// Invoked at each timestep when fuel flowing into scram tank
void ResupplyPostStep::FlowScramFuel(const double simt, const double simdt, const double mjd)
{
    double scramTankQty = GetXR1().GetXRPropellantMass(GetXR1().ph_scram);
    const double scramTankMaxQty = GetXR1().GetXRPropellantMaxMass(GetXR1().ph_scram);

    bool haltFlow = false;

    // if SCRAM tank is hidden and no SCRAM tank present in bay, we cannot flow any fuel to resupply anything
    // Note: if the SCRAM tank is hidden, then by definition we have a payload bay, so no need to check if m_pPayloadBay is null here
    if (GetXR1().m_SCRAMTankHidden && (GetXR1().m_pPayloadBay->GetPropellantMaxMass(PROP_TYPE::PT_SCRAM) <= 0))  // < 0 for sanity check
    {
        GetXR1().ShowWarning(nullptr, DeltaGliderXR1::ST_None, "No SCRAM fuel tank in bay.");
        GetXR1().PlayErrorBeep();
        haltFlow = true;
        goto FlowScramFuelExit;
    }

    // if scram tank already full, we cannot refuel a full tank
    if (scramTankQty >= scramTankMaxQty)
    {
        GetXR1().ShowInfo("Scram Fuel Tanks Full.wav", DeltaGliderXR1::ST_InformationCallout, "SCRAM fuel tanks already full.");
        haltFlow = true;
    }
    else   // tanks not full yet
    {
        // adjust by pressure
        const double pressureFrac = GetXR1().m_scramExtLinePressure / GetXR1().m_nominalScramExtLinePressure;  // 0...1
        const double fuelFlowForThisStep = (FUEL_LOAD_RATE * simdt * SCRAM_FLOW_FRACTION * pressureFrac);

        scramTankQty += fuelFlowForThisStep;

        // check limits
        const char* pMsg = nullptr;
        if (scramTankQty > scramTankMaxQty)  // scram tank overflow
        {
            scramTankQty = scramTankMaxQty;
            haltFlow = true;
            // no need for a msg here; the FuelCalloutsPostStep will handle it
        }
        GetXR1().SetXRPropellantMass(GetXR1().ph_scram, scramTankQty);
    }

    // flow sounds are handled by our caller
    // NOTE: "scram fuel tank full" is handled by our FuelCalloutsPostStep
FlowScramFuelExit:
    if (haltFlow)
    {
        GetXR1().m_scramFuelFlowSwitch = false;

        // refresh the switch and its LED
        GetXR1().TriggerRedrawArea(AID_SCRAMSUPPLYLINE_SWITCH);
        GetXR1().TriggerRedrawArea(AID_SCRAMSUPPLYLINE_SWITCH_LED);

        // flow sound will stop next timestep 
    }
}

// Invoked at each timestep when fuel flowing into apu tank
void ResupplyPostStep::FlowApuFuel(const double simt, const double simdt, const double mjd)
{
    double apuTankQty = GetXR1().m_apuFuelQty;
    const double apuTankMaxQty = APU_FUEL_CAPACITY;

    bool haltFlow = false;

    // if apu tank already full, we cannot refuel a full tank
    if (apuTankQty >= apuTankMaxQty)
    {
        GetXR1().ShowInfo("APU Fuel Tanks Full.wav", DeltaGliderXR1::ST_InformationCallout, "APU fuel tanks already full.");
        haltFlow = true;
    }
    else   // tanks not full yet
    {
        // adjust by pressure
        const double pressureFrac = GetXR1().m_apuExtLinePressure / GetXR1().m_nominalApuExtLinePressure;  // 0...1
        const double fuelFlowForThisStep = (FUEL_LOAD_RATE * simdt * APU_FLOW_FRACTION * pressureFrac);

        apuTankQty += fuelFlowForThisStep;

        // check limits

        const char* pMsg = nullptr;
        if (apuTankQty > apuTankMaxQty)  // apu tank overflow
        {
            apuTankQty = apuTankMaxQty;
            haltFlow = true;
            // no need for a msg here; the FuelCalloutsPostStep will handle it
        }

        GetXR1().m_apuFuelQty = apuTankQty;
    }

    // flow sounds are handled by our caller
    // NOTE: "apu fuel tank full" is handled by our FuelCalloutsPostStep
    if (haltFlow)
    {
        GetXR1().m_apuFuelFlowSwitch = false;

        // refresh the switch and its LED
        GetXR1().TriggerRedrawArea(AID_APUSUPPLYLINE_SWITCH);
        GetXR1().TriggerRedrawArea(AID_APUSUPPLYLINE_SWITCH_LED);

        // flow sound will stop next timestep 
    }
}

// **** LOX Resupply 

// Check LOX switch and handle resupply operations; this is only invoked when 
// LOX resupply systems are ONLINE; however, LOX PRESSURE may be building yet.
void ResupplyPostStep::PerformLoxResupply(const double simt, const double simdt, const double mjd)
{
    const XR1ConfigFileParser& config = *GetXR1().GetXR1Config();

    bool loxAvailable = false;

    if (GetXR1().IsDocked())
        loxAvailable = config.AllowDockResupply[TANK_LOX];
    else  // we are grounded
        loxAvailable = (config.AllowEarthOnlyResupply[TANK_LOX] ? GetXR1().IsLandedOnEarth() : config.AllowGroundResupply[TANK_LOX]);

    // 
    // Handle LOX flow
    //

    if (loxAvailable)
    {
        if (GetXR1().m_loxFlowSwitch)
            FlowLox(simt, simdt, mjd);
    }
}

// Invoked at each timestep when LOX flowing into main tank
void ResupplyPostStep::FlowLox(const double simt, const double simdt, const double mjd)
{
    double loxTankQty = GetXR1().GetXRLOXMass();
    const double loxTankMaxQty = GetXR1().GetXRLOXMaxMass();

    bool haltFlow = false;

    // if main tank already full, we cannot refuel a full tank
    if (loxTankQty >= loxTankMaxQty)
    {
        GetXR1().ShowInfo("LOX Tanks Full.wav", DeltaGliderXR1::ST_InformationCallout, "LOX fuel tanks already full.");
        haltFlow = true;
    }
    else   // tanks not full yet
    {
        // LOX flow fraction is based on tank capacity AND a minimum flow rate per second * pressureFraction
        const double pressureFrac = GetXR1().m_loxExtLinePressure / GetXR1().m_nominalLoxExtLinePressure;  // 0...1
        const double flowRate = max(GetXR1().GetXRLOXMaxMass() * LOX_LOAD_FRAC * pressureFrac, LOX_MIN_FLOW_RATE * pressureFrac);
        const double loxFlowForThisStep = (flowRate * simdt);  // mass * load rate in TANK FRACTION/SECOND

        loxTankQty += loxFlowForThisStep;

        // check limits

        const char* pMsg = nullptr;
        if (loxTankQty > loxTankMaxQty)  // tank overflow?
        {
            loxTankQty = loxTankMaxQty;
            haltFlow = true;

            // no need for a msg here; the FuelCalloutsPostStep will handle it
        }

        GetXR1().SetXRLOXMass(loxTankQty);  // updates payload LOX mass as well
    }

    // flow sounds are handled by our caller
    // NOTE: "lox fuel tank full" is handled by our FuelCalloutsPostStep
    if (haltFlow)
    {
        GetXR1().m_loxFlowSwitch = false;

        // refresh the switch and its LED
        GetXR1().TriggerRedrawArea(AID_LOXSUPPLYLINE_SWITCH);
        GetXR1().TriggerRedrawArea(AID_LOXSUPPLYLINE_SWITCH_LED);

        // flow sound will stop next timestep 
    }
}

//---------------------------------------------------------------------------

// LinePressure class methods to manage line pressure.
// Set/adjust fuel or lox line pressure.  If target pressure reached, sets a new pressure target.
//
// linePressure = ref to line pressure; resides in XR1 objects so that display areas can access it
// nominalLinePressure = ref to NOMINAL ("max normal") line pressure; resides in XR1 objects so that display areas can access it
// pressureNominalLineStatusFlag = bool ref set to true once pressure builds to nominal level
// flowInProgress = bool ref set to true if liquid is flowing in the line, false if not
// maxPressure = max (nominal) line pressure in PSI; initial pressure will build to ~maxPressure.
// pressureMovementRate = fraction of max pressure to move in one second; e.g., 0.20 = 20% of max pressure
LinePressure::LinePressure(double& linePressure, double& nominalLinePressure, bool& pressureNominalLineStatusFlag, const bool& flowInProgress, double maxPressure, double pressureMovementRate, DeltaGliderXR1& xr1) :
    m_linePressure(linePressure), m_nominalLinePressure(nominalLinePressure), m_pressureNominalLineStatusFlag(pressureNominalLineStatusFlag),
    m_maxPressure(maxPressure), m_pressureMovementRate(pressureMovementRate), m_flowInProgress(flowInProgress), m_xr1(xr1),
    m_initialPressureTarget(0), m_pressureTarget(0)
{
    Disconnected();     // init to disconnected state
}

// invoked when refueling line disconnected
void LinePressure::Disconnected()
{
    // reset state, but do not reset line pressure flag here
    m_pressureNominalLineStatusFlag = false;  // prevent fuel from flowing
    m_pressureTarget = 0;  // pressure will gradually fall to zero
}

// Invoked from ResupplyPostStep for each resupply line : adjust line pressure to its target
void LinePressure::AdjustPressure(const double simt, const double simdt, const double mjd)
{
    // if pressure target is < 0 it means we are performing the initial pressurization to NOMINAL
    if (m_pressureTarget < 0)
    {
        // Set NOMINAL pressure to the maximum normal PSI, which will be for ground refueling.
        // Docked refueling will be slower than ground refueling.
        m_nominalLinePressure = m_maxPressure * RESUPPLY_GROUND_PSI_FACTOR;

        // Set pressure target based on whether we are grounded (higher-pressure pumps)
        // or docked (lower-pressure pumps).
        if (m_xr1.GroundContact())
            m_pressureTarget = m_maxPressure * RESUPPLY_GROUND_PSI_FACTOR;
        else
            m_pressureTarget = m_maxPressure * RESUPPLY_DOCKED_PSI_FACTOR;

        // actual pressure may vary +-RESUPPLY_RANDOM_LIMIT fraction
        const double sign = ((oapiRand() < 0.5) ? -1.0 : 1.0);
        const double varianceFrac = RESUPPLY_RANDOM_LIMIT * oapiRand() * sign;
        m_pressureTarget += (m_maxPressure * varianceFrac);  // NOTE: variance is by MAX PRESSURE here
        m_initialPressureTarget = m_pressureTarget; // this will be nominal pressure for this fueling session
    }
    else    // pressure target set; move toward it
    {
        const double rateFraction = (m_pressureTarget > 0 ? 1.0 : 2.2);  // pressure falls to zero more rapidly than it pressurizes

        // compute delta in PSI for this fraction of a second
        const double psiDelta = simdt * (m_pressureMovementRate * m_maxPressure * rateFraction);
        if (m_linePressure < m_pressureTarget)
        {
            m_linePressure += psiDelta;
            if (m_linePressure > m_pressureTarget)
                m_linePressure = m_pressureTarget;  // don't overshoot it
        }
        else if (m_linePressure > m_pressureTarget)
        {
            m_linePressure -= psiDelta;
            if (m_linePressure < m_pressureTarget)
                m_linePressure = m_pressureTarget;  // don't undershoot it
        }
        else   // pressure target REACHED
        {
            // set 'pressure nominal' flag if pressure > 0
            m_pressureNominalLineStatusFlag = (m_linePressure > 0);   // this must remain TRUE as long as fuel can flow

            // If refueling in progress, set a new target right away to simulate fluctuating fuel flow.
            // In addition, set pressure target base to m_initialPressureTarget * .81 to simulate flow.
            if (m_flowInProgress)
            {
                // adjust the pressure target by a variance based on the NOMINAL pressure; i.e., successive variances do not "stack"
                const double sign = ((oapiRand() < 0.5) ? -1.0 : 1.0);
                const double varianceFrac = RESUPPLY_RANDOM_LIMIT * oapiRand() * sign;
                const double variance = (m_maxPressure * varianceFrac);  // in PSI; variance is by MAX PRESSURE here
                m_pressureTarget = (m_initialPressureTarget * 0.81) + variance;  // 19% lower pressure when flowing

                // keep target pressure within fixed limits
                if (m_pressureTarget > (m_maxPressure * RESUPPLY_UPPER_LIMIT))
                    m_pressureTarget -= (variance * 2); // go lower instead
                else if (m_pressureTarget < (m_maxPressure * RESUPPLY_LOWER_LIMIT))
                    m_pressureTarget += (variance * 2); // go higher instead
            }
            else    // flow is IDLE
            {
                // restore target pressure to m_initialPressureTarget; remains constant for now
                // nothing to do here: m_pressureTarget = m_initialPressureTarget;
            }
        }
    }
}
