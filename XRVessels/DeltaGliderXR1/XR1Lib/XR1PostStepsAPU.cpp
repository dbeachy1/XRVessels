/**
  XR Vessel add-ons for OpenOrbiter Space Flight Simulator
  Copyright (C) 2006-2025 Douglas Beachy

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

//---------------------------------------------------------------------------

APUPostStep::APUPostStep(DeltaGliderXR1& vessel) :
    XR1PrePostStep(vessel),
    m_prevDoorStatus(DoorStatus::NOT_SET), m_doorTargetSimt(0), m_prevQty(-1), m_firstTimeStep(true), m_poweringUpOrDown(false)
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
        if ((GetXR1().apu_status == DoorStatus::DOOR_OPEN) || (GetXR1().apu_status == DoorStatus::DOOR_OPENING))
            GetXR1().ActivateAPU(DoorStatus::DOOR_CLOSING);
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
        const XR1ConfigFileParser* pConfig = GetXR1().GetXR1Config();
        if (pConfig->APUIdleRuntimeCallouts > 0)        // callouts enabled?
        {
            // check for runtime callout if APU is running AND limited APU fuel enabled
            // NOTE: AF Ctrl already handled above
            if ((GetXR1().apu_status == DoorStatus::DOOR_OPEN) && (pConfig->APUFuelBurnRate > 0))
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
    if ((GetXR1().apu_status == DoorStatus::DOOR_OPEN) || (GetXR1().apu_status == DoorStatus::DOOR_OPENING))
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
            if (GetXR1().apu_status == DoorStatus::DOOR_OPEN)
                GetXR1().apu_status = DoorStatus::DOOR_CLOSING;
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
            static const char* pCalloutFilenames[numCallouts] = {
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

            for (int i = 0; i < numCallouts; i++)
            {
                const double calloutFrac = calloutFractions[i];
                const char* pCalloutFilename = pCalloutFilenames[i];

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
        doorStatus = GetXR1().apu_status = ((doorStatus == DoorStatus::DOOR_OPENING) ? DoorStatus::DOOR_OPEN : DoorStatus::DOOR_CLOSED);
        m_poweringUpOrDown = false;  // reset for next time

        // if APU just reached full ON state, turn AF CTRL ON as well *if* inside any atmosphere
        if ((doorStatus == DoorStatus::DOOR_OPEN) && (GetVessel().GetDynPressure() >= 5.0e3))   // 5 kPa dynamic pressure
            GetVessel().SetADCtrlMode(7);
    }

    // check whether door is functional and has just changed state
    if ((doorStatus != DoorStatus::DOOR_FAILED) && (doorStatus != m_prevDoorStatus))
    {
        const double spinupSpindownTime = 2.5;  // time in seconds (allow 1/10th second buffer so no gap in sound: sound is 2.6 sec long)
        // APU is audible only inside the ship
        if (doorStatus == DoorStatus::DOOR_OPENING)
        {
            GetXR1().LoadXR1Sound(GetXR1().APU, "APU Startup.wav", XRSound::PlaybackType::InternalOnly);
            GetXR1().PlaySound(GetXR1().APU, DeltaGliderXR1::ST_Other, APU_VOL);
            m_doorTargetSimt = simt + spinupSpindownTime;
            m_poweringUpOrDown = true;
            GetXR1().ShowInfo(nullptr, DeltaGliderXR1::ST_None, "APU powering up.");
        }
        else if (doorStatus == DoorStatus::DOOR_CLOSING)
        {
            GetXR1().LoadXR1Sound(GetXR1().APU, "APU Shutdown.wav", XRSound::PlaybackType::InternalOnly);
            GetXR1().PlaySound(GetXR1().APU, DeltaGliderXR1::ST_Other, APU_VOL);
            m_doorTargetSimt = simt + spinupSpindownTime;
            m_poweringUpOrDown = true;
            GetXR1().ShowInfo(nullptr, DeltaGliderXR1::ST_None, "APU powering down.");
        }
        else if (doorStatus == DoorStatus::DOOR_OPEN)
        {
            GetXR1().LoadXR1Sound(GetXR1().APU, "APU Run.wav", XRSound::PlaybackType::InternalOnly);
            GetXR1().PlaySound(GetXR1().APU, DeltaGliderXR1::ST_Other, APU_VOL, true);    // LOOP this sound
            if (m_prevDoorStatus != DoorStatus::NOT_SET)    // not the first time through here?
                GetXR1().ShowInfo(nullptr, DeltaGliderXR1::ST_None, "APU online.");
        }
        else if (doorStatus == DoorStatus::DOOR_CLOSED)
        {
            if (m_prevDoorStatus != DoorStatus::NOT_SET)    // not the first time through here?
                GetXR1().ShowInfo(nullptr, DeltaGliderXR1::ST_None, "APU offline.");
        }
    }

    // remember for next frame
    m_prevDoorStatus = doorStatus;
}

//---------------------------------------------------------------------------

DisableControlSurfForAPUPostStep::DisableControlSurfForAPUPostStep(DeltaGliderXR1& vessel) :
    XR1PrePostStep(vessel),
    m_initialStartupComplete(false)
{
}

// disable flight control surfaces and wheel brakes if APU is offline
void DisableControlSurfForAPUPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // NOTE: is it very difficult and cumbersome to delete and re-create control surfaces, so we simply force the AF mode to OFF here as necessary
    if (GetXR1().apu_status != DoorStatus::DOOR_OPEN)
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
