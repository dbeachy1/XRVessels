// ==============================================================
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// Copyright 2006-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR1FuelPostSteps.cpp
// Class defining fuel-related PostSteps for the DG-XR1
// ==============================================================

#include "XR1FuelPostSteps.h"
#include "AreaIDs.h"
#include "XRPayloadBay.h"

//---------------------------------------------------------------------------

// handles fuel and LOX callouts
FuelCalloutsPostStep::FuelCalloutsPostStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel),
    m_prevMainFuelFrac(-1), m_prevScramFuelFrac(-1), m_prevRcsFuelFrac(-1), m_prevLoxFrac(-1)
{
}

void FuelCalloutsPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())  // covers IsCrashed() as well
        return;     

    CheckFuelLevel("Main", GetXR1().ph_main, m_prevMainFuelFrac, wlMfuel);
    CheckFuelLevel("RCS", GetXR1().ph_rcs, m_prevRcsFuelFrac, wlRfuel);
    CheckFuelLevel("SCRAM", GetXR1().ph_scram, m_prevScramFuelFrac, wlNONE);  // no light for SCRAM fuel; low SCRAM fuel is not a critical warning condition: it is normal
    
    // NOTE: APU fuel is checked in APUPostStep later in this file

    CheckLoxLevel();
}

void FuelCalloutsPostStep::CheckFuelLevel(const char *pLabel, PROPELLANT_HANDLE ph, double &prevQtyFrac, WarningLight warningLight)
{
    // NOTE: we need to detect whether we just backed out an Orbiter refuel and ignore the fuel level change.
    // If landed on a pad, the Orbiter core starts us auto-refuelled, and then when your fuel PreStep (correctly) backs out the 
    // fuel level to zero a frame later, the code here sees the level go from 1.0 to 0.0 and so throws a
    // "Foo Fuel Depleted" warning on startup (see XR2 Phobos/Deimos misson scenario startup). 
    if (GetXR1().m_backedOutOrbiterCoreAutoRefuelThisFrame)
    {
        // Force a reset to the current fuel level (level is zero for backed-out tanks now) so we 
        // don't throw a warning due to the level going from 1.0 to 0.
        prevQtyFrac = -1;   
    }

    // check the fuel level and see whether it is low or depleted, even if we are crashed
    char pSoundFilename[64];
    char pMsg[64];

#define SEND_FUEL_WARNING(level) \
        sprintf(pSoundFilename, "Warning %s Fuel " level ".wav", pLabel);  \
        sprintf(pMsg, "WARNING: %s Fuel " level, pLabel);                  \
        GetXR1().ShowWarning(pSoundFilename, DeltaGliderXR1::ST_WarningCallout, pMsg)

    const double currentPropMassFrac = SAFE_FRACTION(GetXR1().GetXRPropellantMass(ph), GetXR1().GetXRPropellantMaxMass(ph));
    const double warningFrac = 0.05;

    if (prevQtyFrac != -1)  // not first time through here?
    {
        if ((currentPropMassFrac >= 1.0) && (prevQtyFrac < 1.0))   // just hit full?
        {
            sprintf(pSoundFilename, "%s Fuel Tanks Full.wav", pLabel);
            sprintf(pMsg, "%s fuel tanks full.", pLabel);
            GetXR1().ShowInfo(pSoundFilename, DeltaGliderXR1::ST_InformationCallout, pMsg);
        }
        else if ((currentPropMassFrac <= 0) && (prevQtyFrac > 0))    // just hit 0%?
        {
            SEND_FUEL_WARNING("Depleted");
            if (warningLight != wlNONE)
                GetXR1().m_MWSActive = true;
        }
        else if ((currentPropMassFrac < warningFrac) && (prevQtyFrac >= warningFrac)) // just crossed below 5% remaining?
        {
            SEND_FUEL_WARNING("Low");
            if (warningLight != wlNONE)
                GetXR1().m_MWSActive = true;
        }

        // warning light always blinks regardless of main MWS light
        if (warningLight != wlNONE)
        {
            if (currentPropMassFrac < warningFrac)
                GetXR1().m_warningLights[warningLight] = true;
            else
                GetXR1().m_warningLights[warningLight] = false;  // fuel level OK
        }
    }

    // update prevQty for next loop
    prevQtyFrac = currentPropMassFrac;
}

void FuelCalloutsPostStep::CheckLoxLevel()
{
    // check the LOX level and see whether it is low or depleted, even if we are crashed
    // This take payload LOX into account as well
    const double currentQtyFrac = GetXR1().GetXRLOXMass() / GetXR1().GetXRLOXMaxMass();
    const double warningFrac = 0.10;

    if (m_prevLoxFrac != -1)  // not first time through here?
    {
        // must set a threshold < 1.0 here since LOX is constantly consumed
        const double fullThreshold = 0.99999;
        if ((currentQtyFrac >= fullThreshold) && (m_prevLoxFrac < fullThreshold))   // just hit full?
        {
            GetXR1().ShowInfo("LOX Tanks Full.wav", DeltaGliderXR1::ST_InformationCallout, "LOX tanks full.");
        }
        else if ((currentQtyFrac <= 0) && (m_prevLoxFrac > 0))    // just hit 0%?
        {
            GetXR1().ShowWarning("Warning Oxygen Depleted.wav", DeltaGliderXR1::ST_WarningCallout, "WARNING: OXYGEN DEPLETED");
            GetXR1().m_MWSActive = true;
        }
        else if ((currentQtyFrac < warningFrac) && (m_prevLoxFrac >= warningFrac)) // just crossed below 5% remaining?
        {
            GetXR1().ShowWarning("Warning Oxygen Low.wav", DeltaGliderXR1::ST_WarningCallout, "WARNING: Oxygen low.");
            GetXR1().m_MWSActive = true;
        }

        // warning light always blinks regardless of main MWS light
        if (currentQtyFrac < warningFrac)
            GetXR1().m_warningLights[wlLox] = true;
        else
            GetXR1().m_warningLights[wlLox] = false;  // LOX level OK
    }

    // update m_prevQty for next loop
    m_prevLoxFrac = currentQtyFrac;
}

//---------------------------------------------------------------------------

APUPostStep::APUPostStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel),
    m_prevDoorStatus(NOT_SET), m_doorTargetSimt(0), m_prevQty(-1), m_firstTimeStep(true), m_poweringUpOrDown(false)
{
}

// handles all APU-related PostStep tasks
void APUPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // Note: this should run even if he crew is incapacitated
    // if we don't have focus and the APU is online AND auto-shutdown is enabled AND APU fuel is not infinite, turn it off to save fuel!
    // Exception: do not shut down the APU if Attitude Hold engaged in an atmosphere.
    // Exception #2: never auto-shutdown the APU during a replay: the user may want to switch vessels just to look around
    if ((GetVessel().HasFocus() == false) && (GetXR1().GetXR1Config()->APUAutoShutdown) && 
        (GetXR1().GetXR1Config()->GetAPUFuelBurnRate() > 0) && (!GetXR1().m_cogShiftAutoModeActive) &&
        !GetVessel().Playback())
    {
        // turn off the APU if not already off.
        if ((GetXR1().apu_status == DOOR_OPEN) || (GetXR1().apu_status == DOOR_OPENING))
            GetXR1().ActivateAPU(DOOR_CLOSING);
    }

    BurnAPUFuel(simt, simdt, mjd);
    UpdateAPUDoorState(simt, simdt, mjd);

    bool AFCtrlOn = (GetVessel().GetADCtrlMode() != 0);
    if (AFCtrlOn)
    {
        GetXR1().MarkAPUActive();  // reset callout time so that countdown starts when AF CTRL turned OFF
    }
    else  // AF CTRL = OFF
    {
        const XR1ConfigFileParser *pConfig = GetXR1().GetXR1Config();
        if (pConfig->APUIdleRuntimeCallouts > 0)        // callouts enabled?
        {
            // check for runtime callout if APU is running AND limited APU fuel enabled
            // NOTE: AF Ctrl already handled above
            if ((GetXR1().apu_status == DOOR_OPEN) && (pConfig->APUFuelBurnRate > 0))
            {
                // Note: don't need to worry about deltaFromLastLoad going negative here 
                const double deltaFromLastLoad = simt - GetXR1().m_latestHydraulicDoorRunningSimt;
                if (deltaFromLastLoad >= pConfig->APUIdleRuntimeCallouts)
                {
                    GetXR1().ShowWarning("Information APU Running.wav", DeltaGliderXR1::ST_WarningCallout, "Alert: APU running with no load.");
                    GetXR1().MarkAPUActive();  // reset the APU idle warning callout time
                }
            }
        }
    }
}

void APUPostStep::BurnAPUFuel(const double simt, const double simdt, const double mjd)
{
    // burn fuel if APU is running or starting up
    if ((GetXR1().apu_status == DOOR_OPEN) || (GetXR1().apu_status == DOOR_OPENING))
    {
        // burn fuel at the specified rate
        const double kgPerMin = GetXR1().GetXR1Config()->GetAPUFuelBurnRate();  // may be 0
        const double kgPerSec = kgPerMin / 60.0;

        if (GetXR1().m_apuFuelQty > 0.0)
        {
            GetXR1().m_apuFuelQty -= (kgPerSec * simdt);     // amount of fuel burned in this timestep
            if (GetXR1().m_apuFuelQty < 0.0)
                GetXR1().m_apuFuelQty = 0.0;
        }
    }

    const double warningFrac = 0.05;        // warn @ 5% remaining
    const double prevFrac = m_prevQty / APU_FUEL_CAPACITY;  // frac from previous timestep
    const double frac = GetXR1().m_apuFuelQty / APU_FUEL_CAPACITY;

    // check for APU fuel warnings and callouts IF this is not the first time through here
    if (m_prevQty >= 0)
    {
        if ((frac >= 1.0) && (prevFrac < 1.0))   // just hit full?
        {
            GetXR1().ShowInfo("APU Fuel Tanks Full.wav", DeltaGliderXR1::ST_InformationCallout, "APU fuel tanks full.");
        }
        else if ((frac <= 0) && (prevFrac > 0))    // just hit 0%?
        {
            GetXR1().ShowWarning("Warning APU Fuel Depleted No Hydraulic Pressure.wav", DeltaGliderXR1::ST_WarningCallout, "APU fuel tanks depleted:&NO HYDRAULIC PRESSURE!");
            GetXR1().m_MWSActive = true;

            // shut down the APU if it is running (we may be dumping fuel!)
            if (GetXR1().apu_status == DOOR_OPEN)
                GetXR1().apu_status = DOOR_CLOSING;
        }
        else if ((frac <= warningFrac) && (prevFrac > warningFrac))   // just cross warning threshold?
        {
            GetXR1().ShowWarning("Warning APU Fuel Low.wav", DeltaGliderXR1::ST_WarningCallout, "APU fuel low");
            GetXR1().m_MWSActive = true;
        }
        else    // let's check for normal APU fuel callouts
        {
            static const int numCallouts = 13;
            // declare explict size here to catch size errors if callouts changed
            // NOTE: for efficiency these should be listed in high -> low order
            static const double calloutFractions[numCallouts] = { 0.90, 0.80, 0.70, 0.60, 0.50, 0.40, 0.30, 0.20, 0.10, 0.04, 0.03, 0.02, 0.01 };
            static const char *pCalloutFilenames[numCallouts] = {
                "Information APU Fuel 90 Percent.wav",
                "Information APU Fuel 80 Percent.wav",
                "Information APU Fuel 70 Percent.wav",
                "Information APU Fuel 60 Percent.wav",
                "Information APU Fuel 50 Percent.wav",
                "Information APU Fuel 40 Percent.wav",
                "Information APU Fuel 30 Percent.wav",
                "Information APU Fuel 20 Percent.wav",
                "Information APU Fuel 10 Percent.wav",
                "Warning APU Fuel 4 Percent.wav",
                "Warning APU Fuel 3 Percent.wav",
                "Warning APU Fuel 2 Percent.wav",
                "Warning APU Fuel 1 Percent.wav"
            };

            for (int i=0; i < numCallouts; i++)
            {
                const double calloutFrac = calloutFractions[i];
                const char *pCalloutFilename = pCalloutFilenames[i];

                if ((frac <= calloutFrac) && (prevFrac > calloutFrac))   // just cross threshold?
                {
                    char temp[40];
                    if (frac <= warningFrac)
                    {
                        sprintf(temp, "Warning: APU fuel at %d%%", static_cast<int>(calloutFrac * 100));
                        GetXR1().ShowWarning(pCalloutFilename, DeltaGliderXR1::ST_WarningCallout, temp);
                    }
                    else    // not a warning callout
                    {
                        sprintf(temp, "APU fuel at %d%%", static_cast<int>(calloutFrac * 100));
                        GetXR1().ShowInfo(pCalloutFilename, DeltaGliderXR1::ST_InformationCallout, temp);
                    }
                    break;  // no sense in looking further
                }

                // OPTIMIZATION: terminate search if fuel level is > current callout check
                if (frac > calloutFrac)
                    break;  // we know that all remaining fuel level checks are BELOW our current APU fuel level
            }
        }

        // warning light always blinks regardless of main MWS light
        if (frac < warningFrac)
            GetXR1().m_apuWarning = true;
        else
            GetXR1().m_apuWarning = false;  // fuel level OK

        // vessel mass is updated automatically by UpdateMassPostStep
    }

    // save fuel qty for next step
    m_prevQty = GetXR1().m_apuFuelQty;
}

void APUPostStep::UpdateAPUDoorState(const double simt, const double simdt, const double mjd)
{
    // TODO: see if I still need this for other reasons now that I've switched to XRSound
    // work around OrbiterSound 3.5 CTD: do not load a sound in a PostStep when the simulation is paused!
    // also, ORBITER CORE BUG: oapiGetPause() == false even if simulation paused but we're still on the very first frame.
    if (m_firstTimeStep)
    {
        m_firstTimeStep = false;
        return;     // wait until Orbiter and XRSound finishes initializing
    }

    DoorStatus doorStatus = GetXR1().apu_status;

    // check whether we just reached m_doorTargetSimt
    if (m_poweringUpOrDown && (simt >= m_doorTargetSimt))
    {
        // APU has finished powering up or powering down now
        doorStatus = GetXR1().apu_status = ((doorStatus == DOOR_OPENING) ? DOOR_OPEN : DOOR_CLOSED);
        m_poweringUpOrDown = false;  // reset for next time

        // if APU just reached full ON state, turn AF CTRL ON as well *if* inside any atmosphere
        if ((doorStatus == DOOR_OPEN) && (GetVessel().GetDynPressure() >= 5.0e3))   // 5 kPa dynamic pressure
            GetVessel().SetADCtrlMode(7);
    }

    // check whether door is functional and has just changed state
    if ((doorStatus != DOOR_FAILED) && (doorStatus != m_prevDoorStatus))
    {
        const double spinupSpindownTime = 2.5;  // time in seconds (allow 1/10th second buffer so no gap in sound: sound is 2.6 sec long)
        // APU is audible only inside the ship
        if (doorStatus == DOOR_OPENING)
        {
            GetXR1().LoadXR1Sound(GetXR1().APU, "APU Startup.wav", XRSound::PlaybackType::InternalOnly);
            GetXR1().PlaySound(GetXR1().APU, DeltaGliderXR1::ST_Other, APU_VOL);
            m_doorTargetSimt = simt + spinupSpindownTime;
            m_poweringUpOrDown = true;
            GetXR1().ShowInfo(NULL, DeltaGliderXR1::ST_None, "APU powering up.");
        }
        else if (doorStatus == DOOR_CLOSING)
        {
            GetXR1().LoadXR1Sound(GetXR1().APU, "APU Shutdown.wav", XRSound::PlaybackType::InternalOnly);
            GetXR1().PlaySound(GetXR1().APU, DeltaGliderXR1::ST_Other, APU_VOL);
            m_doorTargetSimt = simt + spinupSpindownTime;
            m_poweringUpOrDown = true;
            GetXR1().ShowInfo(NULL, DeltaGliderXR1::ST_None, "APU powering down.");
        }
        else if (doorStatus == DOOR_OPEN)
        {
            GetXR1().LoadXR1Sound(GetXR1().APU, "APU Run.wav", XRSound::PlaybackType::InternalOnly);
            GetXR1().PlaySound(GetXR1().APU, DeltaGliderXR1::ST_Other, APU_VOL, true);    // LOOP this sound
            if (m_prevDoorStatus != NOT_SET)    // not the first time through here?
                GetXR1().ShowInfo(NULL, DeltaGliderXR1::ST_None, "APU online.");
        }
        else if (doorStatus == DOOR_CLOSED)
        {
            if (m_prevDoorStatus != NOT_SET)    // not the first time through here?
                GetXR1().ShowInfo(NULL, DeltaGliderXR1::ST_None, "APU offline.");
        }
    }

    // remember for next frame
    m_prevDoorStatus = doorStatus;
}

//---------------------------------------------------------------------------

UpdateMassPostStep::UpdateMassPostStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel)
{
}

void UpdateMassPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // adjust mass for APU fuel, LOX, passengers, etc.
    GetXR1().SetEmptyMass();
}

//---------------------------------------------------------------------------

DisableControlSurfForAPUPostStep::DisableControlSurfForAPUPostStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel),
    m_initialStartupComplete(false)
{
}

// disable flight control surfaces and wheel brakes if APU is offline
void DisableControlSurfForAPUPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // NOTE: is it very difficult and cumbersome to delete and re-create control surfaces, so we simply force the AF mode to OFF here as necessary
    if (GetXR1().apu_status != DOOR_OPEN)
    {
        // APU is still offline; ensure the AF mode == OFF
        DWORD ctrlMode = GetXR1().GetADCtrlMode();
        if (ctrlMode != 0)
        {
            // warn the user UNLESS the sim just started; necessary because "empty" scenarios default to ADCtrl ON
            // NOTE: we use an initialStartup flag here so we can flip the switch instantly instead of waiting one second
            if (m_initialStartupComplete)
            {
                // only warn the user if 1) we are moving in a noticable atmosphere, and 2) the ship is airborne
                bool warnUser = (GetVessel().GetDynPressure() > 5) && (GetVessel().GroundContact() == false);
                GetXR1().CheckHydraulicPressure(warnUser, warnUser);
            }

            GetVessel().SetADCtrlMode(0);      // all ctrl surfaces off
        }

        // NOTE: do not disable wheelbrakes with SetWheelbrakeLevel since we want the user to still be able to activate them; 
        // therefore, we set max wheelbrake force to zero here, since there is no hydraulic pressure to power them.
        GetVessel().SetMaxWheelbrakeForce(0);
    }
    else    // APU online
        GetVessel().SetMaxWheelbrakeForce(MAX_WHEELBRAKE_FORCE);   // brakes online

    m_initialStartupComplete = true;

    // NOTE: knob redraw will be handled by the VESSEL2::clbkADCtrlMode method
}

//---------------------------------------------------------------------------

FuelDumpPostStep::FuelDumpPostStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel),
    m_nextWarningSimt(-1), m_fuelDumpStream1(nullptr), m_fuelDumpStream2(nullptr), 
    m_fuelDumpLevel(0)
{
    // create the particle streams if the parent vessel supports them
    if (GetXR1().m_pFuelDumpParticleStreamSpec != nullptr)
        m_fuelDumpStream1 = GetVessel().AddParticleStream(GetXR1().m_pFuelDumpParticleStreamSpec, FUEL_DUMP_PARTICLE_STREAM_POS1, FUEL_DUMP_PARTICLE_STREAM_DIR1, &m_fuelDumpLevel);

    if (GetXR1().m_pFuelDumpParticleStreamSpec != nullptr)
        m_fuelDumpStream2 = GetVessel().AddParticleStream(GetXR1().m_pFuelDumpParticleStreamSpec, FUEL_DUMP_PARTICLE_STREAM_POS2, FUEL_DUMP_PARTICLE_STREAM_DIR2, &m_fuelDumpLevel);
}

// destructor
FuelDumpPostStep::~FuelDumpPostStep()
{
    if (m_fuelDumpStream1 != nullptr)
        GetVessel().DelExhaustStream(m_fuelDumpStream1);

    if (m_fuelDumpStream2 != nullptr)
        GetVessel().DelExhaustStream(m_fuelDumpStream2);
}

void FuelDumpPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    m_fuelDumpLevel = 0.0;      // 0 -> 1.0; used for dump particle level
    // flow weights:
    //   Main:  50%
    //   SCRAM: 25%
    //   LOX: 15%
    //   RCS: 5%
    //   APU: 5%
    if (GetXR1().m_mainFuelDumpInProgress)
    {
        if (DumpFuel(GetXR1().ph_main, simdt, GetXR1().m_mainFuelDumpInProgress, 1.0))
            m_fuelDumpLevel += 0.50;
    }

    if (GetXR1().m_rcsFuelDumpInProgress)
    {
        if (DumpFuel(GetXR1().ph_rcs, simdt, GetXR1().m_rcsFuelDumpInProgress, RCS_FLOW_FRACTION))
            m_fuelDumpLevel += 0.05;
    }

    if (GetXR1().m_scramFuelDumpInProgress)
    {
        if (DumpFuel(GetXR1().ph_scram, simdt, GetXR1().m_scramFuelDumpInProgress, SCRAM_FLOW_FRACTION))
            m_fuelDumpLevel += 0.25;
    }

    if (GetXR1().m_apuFuelDumpInProgress)
    {
        // must dump APU fuel manually here
        if (GetXR1().m_apuFuelQty > 0)
        {
            GetXR1().m_apuFuelQty -= (FUEL_DUMP_RATE * simdt * APU_FLOW_FRACTION);
            if (GetXR1().m_apuFuelQty <= 0)    // underflow?
                GetXR1().m_apuFuelQty = 0;     
            else
                m_fuelDumpLevel += 0.05;
        }

        if (GetXR1().m_apuFuelQty <= 0)    // did tank reach empty?
        {
            GetXR1().PlayErrorBeep();   // alert the pilot
            GetXR1().m_apuFuelDumpInProgress = false;   // halt the dump
        }
    }

    if (GetXR1().m_loxDumpInProgress)
    {
        // must dump LOX manually here
        // This take payload LOX into account as well
        if (GetXR1().GetXRLOXMass() > 0)
        {
            // LOX flow fraction is based on tank capacity AND a minimum flow rate per second
            const double flowRate = max(GetXR1().GetXRLOXMaxMass() * LOX_DUMP_FRAC, LOX_MIN_DUMP_RATE);

            double qty = GetXR1().GetXRLOXMass();
            qty -= (flowRate * simdt);  // mass * dump rate in TANK FRACTION/SECOND
            if (qty <= 0)     // underflow?
                qty = 0;      // prevent underflow
            else
                m_fuelDumpLevel += 0.15; 
            GetXR1().SetXRLOXMass(qty);  // updates payload LOX as well
        }

        if (GetXR1().GetXRLOXMass() <= 0)  // did tank reach empty?
        {
            GetXR1().PlayErrorBeep();   // alert the pilot
            GetXR1().SetLOXDumpState(false);  // halt the dump
        }
    } 

    // update the dump particle stream rate
    // TESTING ONLY: m_fuelDumpLevel = 1.0;
    // DEBUG: sprintf(oapiDebugString(), "m_fuelDumpLevel=%lf", m_fuelDumpLevel);  

    // manage the fuel flow sound
    const int flowCount = static_cast<int>(GetXR1().m_mainFuelDumpInProgress) + 
            static_cast<int>(GetXR1().m_rcsFuelDumpInProgress) + 
            static_cast<int>(GetXR1().m_scramFuelDumpInProgress) + 
            static_cast<int>(GetXR1().m_apuFuelDumpInProgress) + 
            static_cast<int>(GetXR1().m_loxDumpInProgress);
    if (flowCount > 0)  
    {
        // handle fuel/lox flow sounds
        // determine volume level
        const int volume = FUEL_DUMP_BASE_VOL + ((flowCount-1) * FUEL_DUMP_INC_VOL);

        // always play this sound so we can adjust the volume
        GetXR1().PlaySound(GetXR1().FuelDump, DeltaGliderXR1::ST_Other, volume, true);   // loop this sound (although we keep playing it here anyway)

        // show a warning every 5 seconds while any fuel dump is in progress (this also plays immediately the first time)
        if (simt >= m_nextWarningSimt)  // NOTE: warning always plays the first time because m_nextWarningSimt == -1
        {
            char temp[45];
            const bool isLOX = GetXR1().m_loxDumpInProgress;
            
            // test LOX first; priority is HIGHEST -> LOWEST
            // NOTE: these messages must match the text in XR1LowerPanelComponents
            if (isLOX)
                sprintf(temp, "WARNING: LOX dump in progress.");
            else if (GetXR1().m_apuFuelDumpInProgress)
                sprintf(temp, "WARNING: APU fuel dump in progress.");
            else if (GetXR1().m_mainFuelDumpInProgress)
                sprintf(temp, "WARNING: Main fuel dump in progress.");
            else if (GetXR1().m_rcsFuelDumpInProgress)
                sprintf(temp, "WARNING: RCS fuel dump in progress.");
            else if (GetXR1().m_scramFuelDumpInProgress)
                sprintf(temp, "WARNING: SCRAM fuel dump in progress.");
            else    // should never happen!
            {
                _ASSERTE(false);
                *temp = 0;
            }

            if (isLOX)
                GetXR1().ShowWarning("Warning LOX Dump.wav", DeltaGliderXR1::ST_WarningCallout, temp);
            else
                GetXR1().ShowWarning("Warning Fuel Dump.wav", DeltaGliderXR1::ST_WarningCallout, temp);

            m_nextWarningSimt = simt + 5.0; // reset
        }
    }
    else    // fuel not flowing (flowCount == 0)
    {
        GetXR1().StopSound(GetXR1().FuelDump);
        m_nextWarningSimt = -1;   // reset for next time
        m_fuelDumpLevel = 0;      // halted
    }
}

// rateFraction = fraction of speed to dump this tank
// Returns: true if dump continuing, false if dump was halted
bool FuelDumpPostStep::DumpFuel(const PROPELLANT_HANDLE ph, const double simdt, bool &dumpInProgress, const double rateFraction)
{
    // NOTE: it is possible for remaining to be zero here already, so we have to check to end the dump *outside* the "remaining > 0" if block below
    double remaining = GetXR1().GetXRPropellantMass(ph);
    if (remaining > 0)
    {
        // add oapiRand to fuel dump rate so that kg mass goes down by a random fraction
        // (looks better on the lower panel's mass display)
        remaining -= ((FUEL_DUMP_RATE + oapiRand()) * simdt * rateFraction);
        if (remaining < 0)    // underflow?
            remaining = 0;

        // update fuel remaning in tank
        GetXR1().SetXRPropellantMass(ph, remaining);
    }
    
    if (remaining <= 0)    // is tank empty?
    {
        // tank either just reached empty or was empty on entry
        GetXR1().PlayErrorBeep();   // alert the pilot
        dumpInProgress = false;     // halt the dump
    }

    return dumpInProgress;
}

//---------------------------------------------------------------------------

XFeedPostStep::XFeedPostStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel)
{
}

void XFeedPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    double mainToRCSFlow = 0.0;

    // NOTE: flow is to or from RCS tank here, so use RCS_FLOW_FRACTION
    switch (GetXR1().m_xfeedMode)
    {
    case XF_MAIN:
        // RCS -> MAIN
        mainToRCSFlow = -(FUEL_DUMP_RATE * simdt * RCS_FLOW_FRACTION); 
        break;

    case XF_RCS:
        // MAIN -> RCS
        mainToRCSFlow = (FUEL_DUMP_RATE * simdt * RCS_FLOW_FRACTION); 
        break;

    // no default handler for this; fall through and do nothing
    }

    // flow the fuel
    if (mainToRCSFlow != 0)
    {
        double mainTankQty = GetXR1().GetXRPropellantMass(GetXR1().ph_main);
        double rcsTankQty = GetXR1().GetXRPropellantMass(GetXR1().ph_rcs);
        const double mainTankMaxQty = GetXR1().GetXRPropellantMaxMass(GetXR1().ph_main);
        const double rcsTankMaxQty = GetXR1().GetXRPropellantMaxMass(GetXR1().ph_rcs);

        mainTankQty -= mainToRCSFlow;
        rcsTankQty += mainToRCSFlow;

        // check limits
        bool haltFlow = false;
        const char *pMsg = nullptr;
        if (mainTankQty < 0)  // main tank underflow
        {
            // fuel flowing to RCS; remove excess fuel added
            rcsTankQty += mainTankQty;  // mainTankQty is negative

            mainTankQty = 0;
            haltFlow = true;
            pMsg = "MAIN fuel tanks empty";
        }
        else if (mainTankQty > mainTankMaxQty)  // main tank overflow
        {
            // fuel flowing from RCS; replace excess fuel removed
            rcsTankQty += (mainTankQty - mainTankMaxQty);

            mainTankQty = mainTankMaxQty;
            haltFlow = true;
            pMsg = "MAIN fuel tanks full";
        }

        if (rcsTankQty < 0)   // RCS tank underflow
        {
            // fuel flowing to main; remove excess fuel added
            mainTankQty += rcsTankQty;  // rcsTankQty is negative

            rcsTankQty = 0;
            haltFlow = true;
            pMsg = "RCS fuel tanks empty";
        }
        else if (rcsTankQty > rcsTankMaxQty)  // RCS tank overflow
        {
            // fuel flowing from main; replace excess fuel removed
            mainTankQty += (rcsTankQty - rcsTankMaxQty);

            rcsTankQty = rcsTankMaxQty;
            haltFlow = true;
            pMsg = "RCS fuel tanks full";
        }

        GetXR1().SetXRPropellantMass(GetXR1().ph_main, mainTankQty);
        GetXR1().SetXRPropellantMass(GetXR1().ph_rcs, rcsTankQty);

        if (haltFlow)
        {
            GetXR1().SetCrossfeedMode(XF_OFF, pMsg);  // also triggers the knob to redraw
            // flow sound will stop next timestep 
        }
        else    // flow still in progress
        {
            // play sound if not already playing
            if (GetXR1().IsPlaying(GetXR1().FuelCrossFeed) == false)
                GetXR1().PlaySound(GetXR1().FuelCrossFeed, DeltaGliderXR1::ST_Other, FUEL_XFEED_VOL, true);   // loop this sound
        }
    }
    else    // fuel not flowing
    {
        GetXR1().StopSound(GetXR1().FuelCrossFeed);
    }
}

//---------------------------------------------------------------------------

ResupplyPostStep::ResupplyPostStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel),
    m_prevResupplyEnabledStatus(false), m_prevFuelHatchStatus(DOOR_CLOSED), m_prevLoxHatchStatus(DOOR_CLOSED), m_prevExternalCoolingStatus(DOOR_CLOSED),
    m_refuelingSequenceStartSimt(-1), m_loxSequenceStartSimt(-1), m_externalCoolingSequenceStartSimt(-1),
    m_resupplyStartupTime(5.0), // time in seconds
    m_prevSimt(-1), m_resupplyMovementFirstDetectedSimt(-1)
{
    // create our pressure objects; each line has a slightly different pressure rate
    m_pMainLinePressure  = new LinePressure(GetXR1().m_mainExtLinePressure,  GetXR1().m_nominalMainExtLinePressure,  GetXR1().m_mainSupplyLineStatus,  GetXR1().m_mainFuelFlowSwitch,  MAIN_SUPPLY_PSI_LIMIT,  PRESSURE_MOVEMENT_RATE * 1.14, GetXR1());
    m_pScramLinePressure = new LinePressure(GetXR1().m_scramExtLinePressure, GetXR1().m_nominalScramExtLinePressure, GetXR1().m_scramSupplyLineStatus, GetXR1().m_scramFuelFlowSwitch, SCRAM_SUPPLY_PSI_LIMIT, PRESSURE_MOVEMENT_RATE * 1.0,  GetXR1());
    m_pApuLinePressure   = new LinePressure(GetXR1().m_apuExtLinePressure,   GetXR1().m_nominalApuExtLinePressure,   GetXR1().m_apuSupplyLineStatus,   GetXR1().m_apuFuelFlowSwitch,   APU_SUPPLY_PSI_LIMIT,   PRESSURE_MOVEMENT_RATE * 0.92, GetXR1());
    m_pLoxLinePressure   = new LinePressure(GetXR1().m_loxExtLinePressure,   GetXR1().m_nominalLoxExtLinePressure,   GetXR1().m_loxSupplyLineStatus,   GetXR1().m_loxFlowSwitch,       LOX_SUPPLY_PSI_LIMIT,   PRESSURE_MOVEMENT_RATE * 0.86, GetXR1());
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
        if (GetXR1().fuelhatch_status == DOOR_OPEN)
        {
            // check if the hatch just opened
            if (m_prevFuelHatchStatus != DOOR_OPEN)
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
                    const XR1ConfigFileParser &config = *GetXR1().GetXR1Config();
                    const bool isDocked = GetXR1().IsDocked();
                    const bool onEarth = GetXR1().IsLandedOnEarth();

                    bool mainFuelAvailable = false;
                    bool scramFuelAvailable = false;
                    bool apuFuelAvailable = false;

                    if (isDocked)
                    {
                        mainFuelAvailable  = config.AllowDockResupply[TANK_MAIN];
                        scramFuelAvailable = config.AllowDockResupply[TANK_SCRAM];
                        apuFuelAvailable   = config.AllowDockResupply[TANK_APU];
                    }
                    else  // we are grounded
                    {
                        mainFuelAvailable  = (config.AllowEarthOnlyResupply[TANK_MAIN]  ? onEarth : config.AllowGroundResupply[TANK_MAIN]);
                        scramFuelAvailable = (config.AllowEarthOnlyResupply[TANK_SCRAM] ? onEarth : config.AllowGroundResupply[TANK_SCRAM]);
                        apuFuelAvailable   = (config.AllowEarthOnlyResupply[TANK_APU]   ? onEarth : config.AllowGroundResupply[TANK_APU]);
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
            if (m_prevFuelHatchStatus == DOOR_OPEN)     // was the hatch just closed?
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
        if (GetXR1().loxhatch_status == DOOR_OPEN)
        {
            // check if the hatch just opened
            if (m_prevLoxHatchStatus != DOOR_OPEN)
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
                    const XR1ConfigFileParser &config = *GetXR1().GetXR1Config();
                    bool loxAvailable = false;

					if (GetXR1().IsDocked())
                        loxAvailable  = config.AllowDockResupply[TANK_LOX];
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
            if (m_prevLoxHatchStatus == DOOR_OPEN)     // was the hatch just closed?
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
        if (GetXR1().externalcooling_status == DOOR_OPEN)  
        {
            // check if the hatch just opened
            if (m_prevExternalCoolingStatus != DOOR_OPEN)
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
            if (m_prevExternalCoolingStatus == DOOR_OPEN)      // was the hatch just closed?
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
            if ((GetXR1().fuelhatch_status != DOOR_CLOSED) || (GetXR1().loxhatch_status != DOOR_CLOSED))
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
            if (GetXR1().externalcooling_status != DOOR_CLOSED)
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
        const int volume = FUEL_RESUPPLY_BASE_VOL + ((flowCount-1) * FUEL_RESUPPLY_INC_VOL);

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
    const XR1ConfigFileParser &config = *GetXR1().GetXR1Config();
    const bool onEarth = GetXR1().IsLandedOnEarth();

    bool mainFuelAvailable = false;
    bool scramFuelAvailable = false;
    bool apuFuelAvailable = false;

	if (GetXR1().IsDocked())
    {
        mainFuelAvailable  = config.AllowDockResupply[TANK_MAIN];
        scramFuelAvailable = config.AllowDockResupply[TANK_SCRAM];
        apuFuelAvailable   = config.AllowDockResupply[TANK_APU];
    }
    else  // we are grounded
    {
        mainFuelAvailable  = (config.AllowEarthOnlyResupply[TANK_MAIN]  ? onEarth : config.AllowGroundResupply[TANK_MAIN]);
        scramFuelAvailable = (config.AllowEarthOnlyResupply[TANK_SCRAM] ? onEarth : config.AllowGroundResupply[TANK_SCRAM]);
        apuFuelAvailable   = (config.AllowEarthOnlyResupply[TANK_APU]   ? onEarth : config.AllowGroundResupply[TANK_APU]);
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
        const char *pMsg = nullptr;
        if (mainTankQty > mainTankMaxQty)  // main tank overflow
        {
            mainTankQty = mainTankMaxQty;

            // halt fuel flow ONLY if cross-feed is not set to RCS; i.e., fuel is not draining into the RCS tank
            if (GetXR1().m_xfeedMode != XF_RCS)
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
    if (GetXR1().m_SCRAMTankHidden && (GetXR1().m_pPayloadBay->GetPropellantMaxMass(PT_SCRAM) <= 0))  // < 0 for sanity check
    {
        GetXR1().ShowWarning(NULL, DeltaGliderXR1::ST_None, "No SCRAM fuel tank in bay.");
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
        const char *pMsg = nullptr;
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

        const char *pMsg = nullptr;
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
    const XR1ConfigFileParser &config = *GetXR1().GetXR1Config();

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

        const char *pMsg = nullptr;
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
LinePressure::LinePressure(double &linePressure, double &nominalLinePressure, bool &pressureNominalLineStatusFlag, const bool &flowInProgress, double maxPressure, double pressureMovementRate, DeltaGliderXR1 &xr1) :
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
                m_pressureTarget = (m_initialPressureTarget * 0.81)  + variance;  // 19% lower pressure when flowing

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

//---------------------------------------------------------------------------

// Handles LOX consumption
LOXConsumptionPostStep::LOXConsumptionPostStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel),
    m_previousAmbientO2Available(false), m_previousO2Level(-1)
{
}

void LOXConsumptionPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // if crew is DEAD, nothing to do here
    if (GetXR1().m_crewState == DEAD)
        return;

    const int crewMembers = GetXR1().GetCrewMembersCount();

    // compensate for reduced oxygen consumption if configured as such
    const double consumptionFraction = GetXR1().GetXR1Config()->GetLOXConsumptionFraction();   // 0 < n <= 1.0
    const double loxConsumptionPerSecond = crewMembers * LOX_CONSUMPTION_RATE * consumptionFraction * GetXR1().GetXR1Config()->LOXConsumptionMultiplier;  // WARNING: MAY BE ZERO!
    const double loxConsumptionThisTimestep = loxConsumptionPerSecond * simdt;  // WARNING: MAY BE ZERO!

    // no LOX consumption if landed in earth ATM or docked and both airlocks and noscone open, OR if in earth ATM and hatch open, OR if external cooling active
    bool ambientO2Available = false;
    const bool bothAirlocksOpen = ((GetXR1().ilock_proc >= 0.25) && (GetXR1().olock_proc >= 0.25) && (GetXR1().nose_proc >= 0.25));
    const bool externalCoolingActive = (GetXR1().externalcooling_status == DOOR_OPEN);
    const bool isHatchOpen = (GetXR1().hatch_proc > 0.25);  
    double loxQty = GetXR1().GetXRLOXMass();  // includes payload LOX as well
    double o2Level = GetXR1().m_cabinO2Level;   // fraction of O2 in cabin atm

    // check for cabin decompression due to open hatch
    if ((GetXR1().hatch_proc > 0.10) && (GetVessel().GetAtmPressure() < 50e3))
    {
        // decompression!
        GetXR1().ShowHatchDecompression();
        GetXR1().DecompressCabin();   // kills the crew as well
        return;     // all done for this step
    }

    // NOTE: airlock decompression is handled in a dedicated PostStep

    // NOTE: lox is NOT available if the cabin O2 level is 0, which means the cabin has decompressed!
    const bool loxAvailable = ((loxQty > 0) && (GetXR1().m_internalSystemsFailure == false) && (o2Level > 0));  // LOX flow fails internal systems failed

    if (GetXR1().InEarthAtm())
        ambientO2Available |= (isHatchOpen | bothAirlocksOpen);  // O2 available if hatch or both airlocks open

    if (GetXR1().IsDocked())  
        ambientO2Available |= bothAirlocksOpen; // O2 available if both airlocks open

    ambientO2Available |= externalCoolingActive; // O2 available if external cooling active (ambient oxygen feed on)

    // skip these checks the first time through here so that m_previousAmbientO2Available and m_previousO2Level have a chance to initialize
    if (m_previousO2Level > 0)
    {
        //
        // Use ambient O2 if available (e.g., docked w/open airlocks)
        //
        if (ambientO2Available)
        {
            // play an info message if we just reached this condition
            if (m_previousAmbientO2Available == false)
            {
                GetXR1().ShowInfo("Using External O2.wav", DeltaGliderXR1::ST_InformationCallout, "Using external oxygen;&internal O2 flow suspended.");
            }
            // Note: turn A/C sound on if using ambient air; we only want to turn off A/C sounds when LOX is depleted or unavailable (i.e., systems overheat)
            GetXR1().XRSoundOnOff(XRSound::AirConditioning, true);
        }
        else  // no ambient O2 available (using internal O2)
        {
            // play an info message if we just reached this condition
            if (m_previousAmbientO2Available)
                GetXR1().ShowInfo("Using Onboard O2.wav", DeltaGliderXR1::ST_InformationCallout, "Using onboard oxygen;&internal O2 flow resumed.");

            // consume oxygen if LOX available (delta will be 0.0 if LOX consumption disabled)
            if (loxAvailable)
            {
                loxQty -= loxConsumptionThisTimestep;
                if (loxQty < 0)
                    loxQty = 0;     // prevent underflow
            }

            // disable A/C sound if LOX exhausted or enable it if LOX available
            GetXR1().XRSoundOnOff(XRSound::AirConditioning, loxAvailable);  // no internal airflow if lox not available
        }

        //
        // Adjust ambient O2 level
        //
        if (ambientO2Available || loxAvailable)
        {
            // increment level if too low
            if (o2Level < NORMAL_O2_LEVEL)
            {
                o2Level += (AMBIENT_O2_REPLENTISHMENT_RATE * simdt);

                // NOTE: do not play callout here; callout already occurred when we crossed the LOC threshold
                if (o2Level > NORMAL_O2_LEVEL)     
                    o2Level = NORMAL_O2_LEVEL;  // avoid overrun
            }

            // level can never rise above normal, so no need to check it
        }
        else    // No O2 replentishment available; using existing cabin air only!
        {
            // only consume cabin air here if LOX consumption enabled 
            if (GetXR1().GetXR1Config()->GetLOXConsumptionFraction() > 0.0)
            {
                // level falls based on # of crew members AND whether crew is still alive
                const int crewMembers = GetXR1().GetCrewMembersCount();

                if (crewMembers > 0)
                {
                    o2Level -= (AMBIENT_O2_CONSUMPTION_RATE * crewMembers * simdt);
                }
            }
        }
        
        //
        // Check for crew unconsciousness or death UNLESS crew is already dead OR not on board (remember that the death threshold can vary slightly)
        //
        if ((GetXR1().m_crewState != DEAD) && (GetXR1().GetCrewMembersCount() > 0))
        {
            if ((o2Level <= CREW_DEATH_O2_LEVEL) && (m_previousO2Level > CREW_DEATH_O2_LEVEL))
            {
                // no audio for this since no one is awake to hear it
                GetXR1().ShowWarning(NULL, DeltaGliderXR1::ST_None, "CREW IS DEAD DUE TO HYPOXIA!");

                // blink this on the HUD was well
                sprintf(GetXR1().m_crashMessage, "OXYGEN DEPLETED!&CREW IS DEAD DUE TO HYPOXIA!");

                GetXR1().KillCrew();
                GetXR1().m_MWSActive = true;
            }
            else if ((o2Level <= CREW_LOC_O2_LEVEL) && (m_previousO2Level > CREW_LOC_O2_LEVEL))
            {
                // no audio for this since no one is awake to hear it
                GetXR1().ShowWarning(NULL, DeltaGliderXR1::ST_None, "CREW IS UNCONSCIOUS!");

                // blink this on the HUD was well
                sprintf(GetXR1().m_crashMessage, "OXYGEN DEPLETED!&CREW IS UNCONSCIOUS -- DEATH IMMINENT!");

                GetXR1().m_crewState = INCAPACITATED;
                GetXR1().m_MWSActive = true;
            }
            else if ((o2Level > CREW_LOC_O2_LEVEL) && (m_previousO2Level <= CREW_LOC_O2_LEVEL))   // is O2 level is now OK?
            {
                // crew is OK now unless DEAD

                // NOTE: this can only occur if some rescue crew member arrives, since the onboard crew will not be conscious to 
                // open the hatch or deploy the radiator, etc.; however, handle this anyway in case we implement external 
                // rescue ability someday.  
                if (GetXR1().m_crewState != DEAD)
                {
                    GetXR1().m_crewState = OK;

                    // reset HUD warning if msg begins with OXYGEN
                    if (strncmp(GetXR1().m_crashMessage, "OXYGEN", 6) == 0)
                        *GetXR1().m_crashMessage = 0;      // reset

                    GetXR1().ShowInfo(NULL, DeltaGliderXR1::ST_None, "O2 levels returning to normal;&Crew has regained consciousness.");
                }
            }
            else if ((o2Level <= CRITICAL_O2_LEVEL_WARNING) && (m_previousO2Level > CRITICAL_O2_LEVEL_WARNING))  // only play this once
            {
                GetXR1().ShowWarning("Warning Oxygen Levels Critical Hypoxia Imminent.wav", DeltaGliderXR1::ST_WarningCallout, "WARNING: OXYGEN LEVELS CRITICAL;&HYPOXIA IMMINENT!");
                // LOX light will already be blinking b/c tanks must be empty by this time
                GetXR1().m_MWSActive = true;
                
                // disable ATC until O2 returns to normal
                GetXR1().XRSoundOnOff(XRSound::RadioATCGroup, false);
            }
            else if ((o2Level > (CRITICAL_O2_LEVEL_WARNING + 0.01)) && (m_previousO2Level <= (CRITICAL_O2_LEVEL_WARNING + 0.01)))
            {
                GetXR1().ShowInfo("Normal Oxygen Levels Restored.wav", DeltaGliderXR1::ST_InformationCallout, "Oxygen levels returning to normal.");
                
                // re-enable ATC 
                GetXR1().XRSoundOnOff(XRSound::RadioATCGroup, true);
            }
        }
    }

    // set new O2 level
    GetXR1().m_cabinO2Level = o2Level;

    // DEBUG: sprintf(oapiDebugString(), "consumptionFraction=%lf, loxConsumptionPerSecond=%lf, loxQty=%lf, m_oxygenRemainingTime=%lf", consumptionFraction, loxConsumptionPerSecond, loxQty, GetXR1().m_oxygenRemainingTime);

    // update LOX remaining time in seconds and quantity
    // WARNING: must handle loxConsumptionPerSecond = 0 here!
    GetXR1().m_oxygenRemainingTime = ((loxConsumptionPerSecond <= 0) ? 0 : (loxQty / loxConsumptionPerSecond));
    GetXR1().SetXRLOXMass(loxQty);

    // save for next timestep
    m_previousAmbientO2Available = ambientO2Available;
    m_previousO2Level = o2Level;
}

//---------------------------------------------------------------------------
// NOTE: this must be a PostStep, instead of a PreStep as you might expect, because the Orbiter core seems to refuel the ship AFTER the PreSteps are fired.
// NOTE: take care to only check the ship's *internal* main fuel tank here, *not* the bay tanks (if any).
PreventAutoRefuelPostStep::PreventAutoRefuelPostStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel)
{
    // Since we just loaded, let's initialize the previous fuel qty values with the values loaded from the scenario file; otherwise,
    // Orbiter will auto-refuel us before we have a chance to read the original fuel levels.
    // NOTE: all of these values operate with *internal tank levels only*, since that is all that Orbiter refuels
    m_previousInternalFuelQty[0] = GetXR1().m_startupMainFuelFrac * oapiGetPropellantMaxMass(GetXR1().ph_main);
    m_previousInternalFuelQty[1] = GetXR1().m_startupRCSFuelFrac * oapiGetPropellantMaxMass(GetXR1().ph_rcs);  
    m_previousInternalFuelQty[2] = GetXR1().m_startupSCRAMFuelFrac * oapiGetPropellantMaxMass(GetXR1().ph_scram);

    // Initialize bay tank member variables to "not initialized yet": we must defer proper initialization of these
    // until clbkPrePostStep because the payload bay vessels are not attached yet in clbkPostCreation, from which we are called.
    for (int i=0; i < 3; i++)
        m_previousBayFuelQty[i] = -1;
}

void PreventAutoRefuelPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    GetXR1().m_backedOutOrbiterCoreAutoRefuelThisFrame = false;  // reset; this boolean flag is only ever set for one frame

    // perform one-time initialization if payload bay is present
    if ((GetXR1().m_pPayloadBay != nullptr) && (m_previousBayFuelQty[0] < 0))  // entire array is in sync, so checking just one is sufficient
    {
        m_previousBayFuelQty[0] = GetXR1().m_pPayloadBay->GetPropellantMaxMass(PT_Main);
        m_previousBayFuelQty[1] = 0;  // no bay RCS-only tanks, and bay tanks never feed the RCS internal tank directly
        m_previousBayFuelQty[2] = GetXR1().m_pPayloadBay->GetPropellantMaxMass(PT_SCRAM);
    }

    // allow auto-refueling if the user configured it in the prefs file OR if the ship is NOT landed (i.e., allow fuel MFD refueling in space)
    if (GetXR1().GetXR1Config()->OrbiterAutoRefuelingEnabled || (!GetXR1().GroundContact()))
        return;     // allow external refueling
    
    // Only disable refueling if:
    //   1) we are not actively refueling or cross-feeding, and 
    //   2) we did not just flow fuel from the bay tanks into the main tanks this timestep
    if (!GetXR1().IsRefuelingOrCrossfeeding())
    {
        // Note: we must always invoke DisableAutoRefuel here so that our m_previousInternalFuelQty and m_previousBayFuelQty values are always up-to-date
        DisableAutoRefuel(GetXR1().ph_main,  0, (GetXR1().m_MainFuelFlowedFromBayToMainThisTimestep == 0));
        DisableAutoRefuel(GetXR1().ph_rcs,   1, true);  // Note: there is no bay refuelling of the RCS tank
        DisableAutoRefuel(GetXR1().ph_scram, 2, (GetXR1().m_SCRAMFuelFlowedFromBayToMainThisTimestep == 0));
    }
    else    // we are refueling, so reset the fuel data to ensure we won't alter the fuel levels once refueling completes
    {
        ResetFuelData();
    }
}

// index = index into m_previousInternalFuelQty array
// bEnabled = true to enable fuel change to be backed out this timestep, false to not not change fuel levels this timestep
void PreventAutoRefuelPostStep::DisableAutoRefuel(PROPELLANT_HANDLE ph, const int index, const bool bEnabled)
{
    double internalFuelQty = oapiGetPropellantMass(ph);     // can't be const because we may set it below
    const double bayFuelQty = ((GetXR1().m_pPayloadBay != nullptr) ? GetXR1().m_pPayloadBay->GetPropellantMass(GetXR1().GetPropTypeForHandle(ph)) : 0);
    const double prevInternalFuelQty = m_previousInternalFuelQty[index];

    if (bEnabled && (prevInternalFuelQty >= 0))  // only check if we are enabled for this timestep AND we have valid data
    {
        // check whether fuel qty went UP since last timestep
        if (internalFuelQty > prevInternalFuelQty)
        {
            // We want to ALLOW payload tanks to refuel us.  Therefore, check whether the *bay fuel quantity* 
            // has changed as well.  If not, then Orbiter is refueling us.  If it *did* change,
            // however, then it means that a payload tank flowed the fuel, so we want to allow that.
            if (bayFuelQty == m_previousBayFuelQty[index])
            {
                // Orbiter is refueling us!  Back out the fuel change.
                // NOTE: this should only reset the *internal* tank: it should never affect the bay tanks
                GetVessel().SetPropellantMass(ph, prevInternalFuelQty);  
                internalFuelQty = prevInternalFuelQty;      // back out existing fuel qty as well (keep in sync w/new value)
                GetXR1().m_backedOutOrbiterCoreAutoRefuelThisFrame = true;
            }
        }
    }

    // remember these values for the next frame
    m_previousInternalFuelQty[index] = internalFuelQty;
    m_previousBayFuelQty[index] = bayFuelQty; 
}

//---------------------------------------------------------------------------

BoilOffPostStep::BoilOffPostStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel),
    m_stream1(nullptr), m_stream2(nullptr), m_level(0)
{
    // create the particle streams if the parent vessel supports them
    if (GetXR1().m_pBoilOffExhaustParticleStreamSpec != nullptr)
        m_stream1 = GetVessel().AddParticleStream(GetXR1().m_pBoilOffExhaustParticleStreamSpec, BOIL_OFF_PARTICLE_STREAM_POS1, BOIL_OFF_PARTICLE_STREAM_DIR1, &m_level);

    if (GetXR1().m_pBoilOffExhaustParticleStreamSpec != nullptr)
        m_stream2 = GetVessel().AddParticleStream(GetXR1().m_pBoilOffExhaustParticleStreamSpec, BOIL_OFF_PARTICLE_STREAM_POS2, BOIL_OFF_PARTICLE_STREAM_DIR2, &m_level);
}

// destructor
BoilOffPostStep::~BoilOffPostStep()
{
    if (m_stream1 != nullptr)
        GetVessel().DelExhaustStream(m_stream1);

    if (m_stream2 != nullptr)
        GetVessel().DelExhaustStream(m_stream2);
}

void BoilOffPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // sanity-check
    if (GetXR1().m_pBoilOffExhaustParticleStreamSpec == nullptr)
        return;

    // Boil-off only active if:
    // 1) ship is in GROUND CONTACT
    // 2) there is any MAIN FUEL remaining on board
    double newLevel = 0;  // assume disabled

    if (GetVessel().GroundContact() && (GetVessel().GetPropellantMass(GetXR1().ph_main) > 0))
    {
        // Note: if you don't want the exhaust to be visible outside of an atmosphere,
        // define the PARTICLESTREAMSPEC with PARTICLESTREAMSPEC::ATM_PLOG
        newLevel = 1.0;
    }

    m_level = newLevel;
}
