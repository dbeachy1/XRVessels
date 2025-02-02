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

#include "DeltaGliderXR1.h"
#include "XR1PreSteps.h"
#include "XRPayloadBay.h"

//---------------------------------------------------------------------------

TakeoffAndLandingCalloutsAndCrashPreStep::TakeoffAndLandingCalloutsAndCrashPreStep(DeltaGliderXR1& vessel) :
    XR1PrePostStep(vessel)
{
}

void TakeoffAndLandingCalloutsAndCrashPreStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    static const double airborneTriggerTime = 0.5;				// assume airborne 1/2-second after wheels-up
    const double airspeed = GetVessel().GetAirspeed();
    const double groundspeed = GetVessel().GetGroundspeed();

    // SPECIAL CASE: if config file could not be parsed, blink the warning message continuously
    if (GetXR1().GetXR1Config()->ParseFailed())
    {
        if (fmod(simt, 4.0) <= 3.5)   // on for 3.5 seconds, off for 1/2-second
        {
            sprintf(oapiDebugString(), "Error parsing '%s'; check the '%s' file for details.", GetXR1().GetXR1Config()->GetConfigFilenames(), XR_LOG_FILE);
        }
        else
        {
            *oapiDebugString() = 0;
        }

        goto exit;  // do not check anything else
    }

    // if any crash / critical status message, blink it on the HUD
    if (*GetXR1().m_crashMessage)
    {
        // let's blink the crash message on the main HUD
        if (fmod(simt, 3.0) <= 2.5)   // on for 2.5 seconds, off for 1/2-second
            sprintf(GetXR1().m_hudWarningText, GetXR1().m_crashMessage);
        else
            *GetXR1().m_hudWarningText = 0;

        goto exit;  // do not check anything else
    }

    // check whether on ground
    // NOTE: a good side-effect of using GetGearFullyUncompressedAltitude here (the main purpose is so that "wheels down" and "wheels up" callouts are correct
    // if gear compression in a subclass vessel is present) is that the pilot can cut his engines once his wheels touch and he is guaranteed 
    // that he will not collapse his gear *if* the gear doesn't collapse when it first touches down.  In other words, the gear can "absorb" a certain amount of 
    // touchdown rate, which is exactly what we want to model.
    if ((GetVessel().GroundContact() || GetXR1().GetGearFullyUncompressedAltitude() <= 0.0))
    {
        const double atmPressure = GetVessel().GetAtmPressure();
        // If there is an atmosphere AND APU offline AND groundspeed > 5 m/s, show a warning!
        // However, don't check within the first one second of sim time because Orbiter seems to move the vessel slightly on startup.
        if ((groundspeed > 5) && (atmPressure > 0) && (simt > 1.0))
            GetXR1().CheckHydraulicPressure(true, false);  // show a warning of APU offline, but do not beep

        // If there is an atmosphere AND AF Ctrl == OFF AND groundspeed > 20 m/s, show a warning!
        // However, don't check within the first one second of sim time because Orbiter seems to move the vessel slightly on startup.
        if ((groundspeed > 20) && (atmPressure > 0) && (simt > 1.0))
        {
            // Changed since XR2: "AF Ctrl Mode" may be just "Pitch" and still be OK.
            if ((GetVessel().GetADCtrlMode() & 1) == 0)
                GetXR1().ShowWarning("Warning AF Ctrl Surfaces Off.wav", DeltaGliderXR1::ST_WarningCallout, "WARNING: AF Ctrl=Off");
        }

        // check whether we just touched down
        if (GetXR1().m_takeoffTime > 0)
        {
            VECTOR3 asVector;
            GetXR1().GetAirspeedVector(FRAME_HORIZON, asVector);

            double touchdownVerticalSpeed = -(asVector.y);  // in m/s
            double previousFrameVerticalSpeed = -GetXR1().m_preStepPreviousVerticalSpeed;    // in m/s

            // As a scenario editor fix, if our previous frame's altitude was > 100 meters, assume this was a scenario editor "instant touchdown" and prevent 
            // any bogus damage/hard landing checks.  However, we cannot just check the previous frame's altitude because that can change rapidly between frames under time acc.
            // Therefore, we check for IsLanded() and touchdownVerticalSpeed == 0.0 here.
            if (GetXR1().IsLanded() && (touchdownVerticalSpeed == 0.0))
                previousFrameVerticalSpeed = 0.0;  // the scenario editor moved us

            // NOTE: if touchdownVerticalSpeed < previousFrameVerticalSpeed (meaning, the impact was SOFTER than the previous frame's value), use
            // the PREVIOUS frame as the impact velocity because Orbiter just "bounced" us up!
            if (touchdownVerticalSpeed < previousFrameVerticalSpeed)
            {
                // DEBUG: sprintf(oapiDebugString(), "touchdownVecticalSpeed=%lf, previousFrameVerticalSpeed=%lf", touchdownVerticalSpeed, previousFrameVerticalSpeed);
                touchdownVerticalSpeed = previousFrameVerticalSpeed;    // use the harder impact, which is the true impact velocity
            }

            // we just touched down (or crashed!)
            // no need to check for damage enabled here; DoCrash will handle it
            double momemtum = GetVessel().GetMass() * touchdownVerticalSpeed;  // mass * vertical speed in m/s

            if (momemtum > FULL_CRASH_THRESHOLD)
            {
                GetXR1().DoCrash("CRASH!!!", touchdownVerticalSpeed);
                goto exit;
            }

            if (GetXR1().gear_status == DoorStatus::DOOR_FAILED)
            {
                GetXR1().DoGearCollapse("Belly landing due to&failed landing gear!", touchdownVerticalSpeed, false);  // do not move the landing gear animation
                // Jump to "reset for ground mode" code, since the ship is not crashed -- otherwise, the next timestep through
                // here will cause a full crash to occur since m_takeoffTime is still > 0.
                goto resetForGroundMode;
            }

            // NOTE: must check gear DOOR status because we partially raise it when a crash occurs
            // check if gear is down
            if (GetXR1().gear_status != DoorStatus::DOOR_OPEN)
            {
                // do gear collapse here since momemtum was below the full crash threshold
                GetXR1().DoGearCollapse("Landing gear not deployed!", touchdownVerticalSpeed, false);   // do not move the landing gear animation
                goto resetForGroundMode;
            }

            // check bank and pitch (meaning, wheels did not touch down cleanly)
            // NOTE: for now, treat positive and negative pitch the same
            if (fabs(GetVessel().GetPitch()) > TOUCHDOWN_MAX_PITCH)
            {
                char temp[128];
                sprintf(temp, "Excessive pitch!&Touchdown Pitch=%.3f degrees", GetVessel().GetPitch() * DEG);
                GetXR1().DoGearCollapse(temp, touchdownVerticalSpeed, true);  // move landing gear animation
                goto resetForGroundMode;
            }

            if (GetVessel().GetPitch() < TOUCHDOWN_MIN_PITCH)
            {
                char temp[128];
                sprintf(temp, "Insufficient pitch!&Touchdown Pitch=%.3f degrees&Minimum pitch=%.3f degrees", (GetVessel().GetPitch() * DEG), TOUCHDOWN_MIN_PITCH);
                GetXR1().DoGearCollapse(temp, touchdownVerticalSpeed, true);  // move landing gear animation
                goto resetForGroundMode;
            }

            if (fabs(GetVessel().GetBank()) > TOUCHDOWN_BANK_LIMIT)
            {
                char temp[128];
                sprintf(temp, "Excessive bank!&Touchdown Bank=%.3f degrees", GetVessel().GetBank() * DEG);
                GetXR1().DoGearCollapse(temp, touchdownVerticalSpeed, true);    // move landing gear animation
                goto resetForGroundMode;
            }

            // check for landing gear collapse
            if (momemtum > LANDING_GEAR_MAX_MOMEMTUM)
            {
                GetXR1().DoGearCollapse(nullptr, touchdownVerticalSpeed, true);  // use default message here
            }
            else  // we have a good landing (or damage was disabled!)
            {
                if (groundspeed > 45.0)     // 45 m/s == ~100 mph
                {
                    // chirp the tires using volume based on the ship's Z axis velocity; maximum volume occurs at 100 m/s
                    const double chirpVolumeFrac = min((0.50 + (0.50 * (groundspeed / 100.0))), 1.0);
                    GetXR1().PlaySound(DeltaGliderXR1::WheelChirp, DeltaGliderXR1::ST_Other, static_cast<int>((255.0 * chirpVolumeFrac)));
                }

                char temp[128];
                sprintf(temp, "Gear touchdown at %.3f m/s.", touchdownVerticalSpeed);
                const char* pCalloutFilename = GetXR1().GetXR1Config()->TouchdownCallout;  // may be empty
                GetXR1().ShowInfo(pCalloutFilename, DeltaGliderXR1::ST_InformationCallout, temp);
            }

        resetForGroundMode:
            // reset for ground mode
            GetXR1().m_takeoffTime = 0;
            GetXR1().m_touchdownTime = simt;
            GetXR1().m_airborneTargetTime = 0;  // reset timer

            // switch off the Airspeed Hold autopilot if it is engaged
            GetXR1().SetAirspeedHoldMode(false, false);   // no message here
            GetXR1().m_setAirspeed = 0;   // reset airspeed target to zero

            // kill the main engines; this applies whether or not Airspeed Hold was engaged
            for (int i = 0; i < 2; i++)
            {
                GetVessel().SetThrusterLevel(GetXR1().th_main[i], 0);
                GetVessel().SetThrusterLevel(GetXR1().th_retro[i], 0);
            }

            // system will remain disarmed until vehicle comes to a full stop
            goto exit;
        }

        // NOTE: we could be either taking off or landing here

        // reset airborne timer in case we are bouncing during takeoff, or if we hovered just enough to bounce
        GetXR1().m_airborneTargetTime = 0;  // reset timer

        // check whether we are wheel-stop
        if (GetXR1().IsLanded())
        {
            // ready to launch!  Reset everything.
            if ((GetXR1().m_touchdownTime > 0) && (GetXR1().gear_status == DoorStatus::DOOR_OPEN))  // did we just land and gear still intact?
            {
                GetXR1().ShowInfo("Wheel Stop.wav", DeltaGliderXR1::ST_InformationCallout, "Wheel Stop.");
            }

            GetXR1().StopSound(DeltaGliderXR1::TiresRolling);
            GetXR1().m_takeoffTime = GetXR1().m_touchdownTime = 0;
        }
        else if (GetXR1().m_takeoffTime == 0)  // we're taking off or landing!  Let's check the speed.
        {
            // New for XRSound version: play tires rolling sound
            if (GetXR1().gear_status == DoorStatus::DOOR_OPEN)
            {
                // max volume reached at 100 knots
                const double level = groundspeed / KNOTS_TO_MPS(100);
                const float volume = VESSEL3_EXT::ComputeVariableVolume(0.1, 1.0, level);
                GetXR1().PlaySound(DeltaGliderXR1::TiresRolling, DeltaGliderXR1::ST_Other, static_cast<int>((255.0 * volume)), true);  // loop this
            }

            double v1CalloutVelocity, vrCalloutVelocity;  // initialized below
            // compute optimum V1 and Vr (rotate) callouts based on payload mass
            if (MAX_RECOMMENDED_PAYLOAD_MASS > 0)   // any payload supported?
            {
                const double massDeltaFromBaseline = GetXR1().GetMass() - FULLY_LOADED_MASS;  // factor over empty mass (includes payload mass)
                const double velocityFactorPerExtraKGofMass = (ROTATE_CALLOUT_AIRSPEED_HEAVY - ROTATE_CALLOUT_AIRSPEED_EMPTY) / MAX_RECOMMENDED_PAYLOAD_MASS;  // # of extra meters-per-second for rotation per extra KG of mass
                const double extraRotationVelocity = massDeltaFromBaseline * velocityFactorPerExtraKGofMass;

                v1CalloutVelocity = V1_CALLOUT_AIRSPEED + (extraRotationVelocity * 0.75);   // V1 only shifts by 75% of extra rotation speed
                vrCalloutVelocity = ROTATE_CALLOUT_AIRSPEED_EMPTY + extraRotationVelocity;  // baseline
            }
            else  // no payload supported
            {
                v1CalloutVelocity = V1_CALLOUT_AIRSPEED;
                vrCalloutVelocity = ROTATE_CALLOUT_AIRSPEED_EMPTY;
            }
            // DEBUG: sprintf(oapiDebugString(), "v1CalloutVelocity=%lf, vrCalloutVelocity=%lf, massDeltaFromBaseline=%lf, velocityFactorPerExtraKGofMass=%lf", v1CalloutVelocity, vrCalloutVelocity, massDeltaFromBaseline, velocityFactorPerExtraKGofMass);

            // NOTE: check for HIGHEST speeds first!
            if ((airspeed >= vrCalloutVelocity) && (GetXR1().m_preStepPreviousAirspeed < vrCalloutVelocity))  // taking off; check Rotate
            {
                GetXR1().PlaySound(GetXR1().Rotate, DeltaGliderXR1::ST_InformationCallout);
            }
            else if ((airspeed >= v1CalloutVelocity) && (GetXR1().m_preStepPreviousAirspeed < v1CalloutVelocity))  // taking off; check V1
            {
                GetXR1().PlaySound(GetXR1().V1, DeltaGliderXR1::ST_InformationCallout);
            }
            else  // check 100 knots (both takoff and landing)
            {
                double mpsKnots = KNOTS_TO_MPS(100);
                if ((airspeed >= mpsKnots) && (GetXR1().m_preStepPreviousAirspeed < mpsKnots) ||
                    (airspeed <= mpsKnots) && (GetXR1().m_preStepPreviousAirspeed > mpsKnots))
                {
                    GetXR1().PlaySound(GetXR1().OneHundredKnots, DeltaGliderXR1::ST_InformationCallout);
                }
            }
        }
    }
    else    // we're airborne -- disarm the takeoff callouts IF we've been airborne long enough to be sure it's not just a bounce
    {
        if (GetXR1().m_takeoffTime == 0)     // are we still taking off?
        {
            if (GetXR1().m_airborneTargetTime == 0)      // did we just become airborne?
            {
                GetXR1().m_airborneTargetTime = simt + airborneTriggerTime;  // start the timer running
            }
            else    // timer running
            {
                if (simt >= GetXR1().m_airborneTargetTime)
                {
                    // timer expired -- we're airborne!
                    GetXR1().StopSound(DeltaGliderXR1::TiresRolling);
                    const char* pCalloutFilename = GetXR1().GetXR1Config()->LiftoffCallout;  // may be empty
                    GetXR1().ShowInfo(pCalloutFilename, DeltaGliderXR1::ST_InformationCallout, "Liftoff!");
                    GetXR1().m_takeoffTime = simt;
                    GetXR1().m_touchdownTime = 0;      // reset

                    // start the MET timer if currently RESET
                    if (GetXR1().m_metMJDStartingTime < 0)
                    {
                        GetXR1().m_metMJDStartingTime = mjd;
                        GetXR1().m_metTimerRunning = true;
                    }
                }
            }
        }
    }

    // common exit point; this is here in case we need it later
exit:
    ;
    // NOTE: previous frame values such as m_preStepPreviousVerticalSpeed are updated by UpdatePreviousFieldsPreStep
}

//---------------------------------------------------------------------------

GearCalloutsPreStep::GearCalloutsPreStep(DeltaGliderXR1& vessel) :
    XR1PrePostStep(vessel),
    m_previousGearStatus(DoorStatus::NOT_SET)
{
}

void GearCalloutsPreStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard() || (GetXR1().gear_status == DoorStatus::DOOR_FAILED))
        return;     // no callouts if crashed or gear failed

    DoorStatus gearStatus = GetXR1().gear_status;

    // reset APU idle timer if the gear is in motion
    if ((gearStatus == DoorStatus::DOOR_OPENING) || (gearStatus == DoorStatus::DOOR_CLOSING))
        GetXR1().MarkAPUActive();  // reset the APU idle warning callout time

    // skip the first frame through here so we can initialize the previous gear status properly
    if (m_previousGearStatus >= DoorStatus::DOOR_CLOSED)
    {
        if (gearStatus != m_previousGearStatus)
        {
            // gear changed state
            if ((gearStatus == DoorStatus::DOOR_OPEN) || (gearStatus == DoorStatus::DOOR_CLOSED) || (gearStatus == DoorStatus::DOOR_FAILED))
            {
                GetXR1().StopSound(GetXR1().GearWhine);
                if (gearStatus != DoorStatus::DOOR_FAILED)
                {
                    const bool isGearUp = (gearStatus == DoorStatus::DOOR_CLOSED);
                    GetXR1().PlayGearLockedSound(isGearUp);  // gear is up if door closed
                    GetXR1().PlaySound(GetXR1().GearLockedThump, DeltaGliderXR1::ST_Other);
                    GetXR1().ShowInfo(nullptr, DeltaGliderXR1::ST_None, (isGearUp ? "Gear doors closed and locked." : "Gear down and locked."));
                }
            }
            else if (gearStatus == DoorStatus::DOOR_OPENING)
            {
                GetXR1().PlaySound(GetXR1().GearDown, DeltaGliderXR1::ST_InformationCallout);
                GetXR1().PlaySound(GetXR1().GearWhine, DeltaGliderXR1::ST_Other, GEAR_WHINE_VOL);
            }
            else
            {
                GetXR1().PlaySound(GetXR1().GearUp, DeltaGliderXR1::ST_InformationCallout);
                GetXR1().PlaySound(GetXR1().GearWhine, DeltaGliderXR1::ST_Other, GEAR_WHINE_VOL);
            }
        }
    }

    m_previousGearStatus = gearStatus;
}

//---------------------------------------------------------------------------

MachCalloutsPreStep::MachCalloutsPreStep(DeltaGliderXR1& vessel) :
    XR1PrePostStep(vessel),
    m_previousMach(-1), m_nextMinimumCalloutTime(-1)
{
}

void MachCalloutsPreStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())  // covers IsCrashed() as well
        return;     // no callouts if crashed

    const double mach = GetVessel().GetMachNumber();
    const bool groundContact = GetVessel().GroundContact();

    if (!groundContact && (mach <= 0))  // prevent resets when on ground
    {
        m_previousMach = MAXLONG;   // out of the atmosphere
        return;     // nothing more to do
    }

    // if no atmosphere, reset callout data; this is necessary in case the ship is instantly transported via editing the config file
    // CORE BUG WORKAROUND: on IO, GetAtmPressure() == 0 but GetMachNumber() > 1!  Therefore, we must check current mach number instead of atmPressure.
    // In addition, disable mach callouts if OAT temperature is not valid (e.g., static pressure too low)
    if ((m_previousMach <= 0) || (mach <= 0) || (GetXR1().IsOATValid() == false))
    {
        m_previousMach = 0;
        goto exit;  // no reason to perform additional checks
    }

    // do not play callouts until minimum time has elapsed, in case pilot is hovering at the same mach
    // also, do not play on the FIRST frame of the simulation
    if ((simt >= m_nextMinimumCalloutTime) && (m_previousMach >= 0))
    {
        // check for special mach callouts
        if ((m_previousMach >= 1.0) && (mach < 1.0))  // decelerating below mach 1
        {
            if (GetXR1().GetXR1Config()->EnableSonicBoom)
            {
                GetXR1().StopSound(GetXR1().SonicBoom);  // in case it's still playing from before
                GetXR1().PlaySound(GetXR1().SonicBoom, DeltaGliderXR1::ST_Other);
            }
            PlayMach(simt, "Subsonic.wav");
        }
        else if ((m_previousMach < 1.0) && (mach >= 1.0))  // accelerating past mach 1
        {
            if (GetXR1().GetXR1Config()->EnableSonicBoom)
            {
                GetXR1().StopSound(GetXR1().SonicBoom);  // in case it's still playing from before
                GetXR1().PlaySound(GetXR1().SonicBoom, DeltaGliderXR1::ST_Other);
            }
            PlayMach(simt, "Mach 1.wav");
        }
        else if ((m_previousMach < 27.0) && (mach >= 27.0))  // do not play "mach 27+" on deceleration
        {
            PlayMach(simt, "Mach 27 Plus.wav");
        }
        else  // perform standard mach callouts
        {
            for (double m = 2; m < 27; m++)
            {
                if (((m_previousMach < m) && (mach >= m)) ||  // acceleration
                    ((m_previousMach > m) && (mach <= m)))   // deceleration
                {
                    char temp[64];
                    sprintf(temp, "Mach %d.wav", static_cast<int>(m));
                    PlayMach(simt, temp);
                    break;
                }
            }
        }
    }

exit:
    // save for next loop
    m_previousMach = mach;
}

void MachCalloutsPreStep::PlayMach(const double simt, const char* pFilename)
{
    m_nextMinimumCalloutTime = simt + 1;    // reset timer

    // allow normal ATC chatter to continue; mach callouts are not that important
    // also, we don't want this to actually fade, so we don't keep re-sending it
    GetXR1().LoadXR1Sound(GetXR1().MachCallout, pFilename, XRSound::PlaybackType::Radio);
    GetXR1().PlaySound(GetXR1().MachCallout, DeltaGliderXR1::ST_VelocityCallout);
}

//---------------------------------------------------------------------------

AltitudeCalloutsPreStep::AltitudeCalloutsPreStep(DeltaGliderXR1& vessel) :
    XR1PrePostStep(vessel),
    m_nextMinimumCalloutTime(-1)
{
}

void AltitudeCalloutsPreStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())  // covers IsCrashed() as well
        return;     // no callouts if crashed

    // adjust altitude for landing gear if gear is down
    const double altitude = GetXR1().GetGearFullyUncompressedAltitude();   // adjust for gear down and/or GroundContact

     // get our vertical speed in meters per second
    VECTOR3 v;
    GetXR1().GetAirspeedVector(FRAME_HORIZON, v);
    const double currentDescentRate = (GetVessel().GroundContact() ? 0 : v.y);      // in m/s

   // if descending at > 0.25 m/s/s below 275 meters, warn pilot if gear is fully up; do NOT warn him if gear is in motion OR if the ship 
   // is below standard "wheels-down" altitude.
    if ((altitude < GetXR1().m_preStepPreviousGearFullyUncompressedAltitude) && (altitude < 275) && (GetXR1().gear_status != DoorStatus::DOOR_OPEN) &&
        (currentDescentRate <= -0.25) && (GetXR1().GetGearFullyCompressedAltitude() > 0))
    {
        GetXR1().ShowWarning("Warning Gear is Up.wav", DeltaGliderXR1::ST_WarningCallout, "ALERT: Landing gear is up!");
    }

    // do not play callouts until minimum time has elapsed, in case pilot is hovering at the same altitude
    // also, do not play on the FIRST frame of the simulation
    if ((simt >= m_nextMinimumCalloutTime) && (GetXR1().m_preStepPreviousGearFullyUncompressedAltitude >= 0))
    {
        // check special case for landing clearance
        const double landingClearanceAlt = GetXR1().GetXR1Config()->ClearedToLandCallout;
        if ((landingClearanceAlt > 0) && (GetXR1().m_preStepPreviousGearFullyUncompressedAltitude > landingClearanceAlt) && (altitude <= landingClearanceAlt))   // descent
        {
            // do not play the callout if vertical speed is too high; i.e., if we are going to crash!
            if (GetXR1().m_preStepPreviousVerticalSpeed > -150)   // vertical speed is in NEGATIVE m/s
                PlayAltitude(simt, "You are cleared to land.wav");
        }
        else  // normal altitude checks
        {
            static const double altitudeCallouts[] =
            {
                5000, 4000, 3000, 2000, 1000, 900, 800, 700, 600,
                500, 400, 300, 200, 100, 75, 50, 40, 30, 20, 15, 10,
                9, 8, 7, 6, 5, 4, 3, 2, 1
            };

            // optimization: skip loop if distance > max callout distance
            if (altitude <= altitudeCallouts[0])
            {
                for (int i = 0; i < (sizeof(altitudeCallouts) / sizeof(double)); i++)
                {
                    const double a = altitudeCallouts[i];

                    // play on descent only
                    if ((GetXR1().m_preStepPreviousGearFullyUncompressedAltitude > a) && (altitude <= a))   // descent
                    {
                        char temp[64];
                        sprintf(temp, "%d.wav", static_cast<int>(a));
                        PlayAltitude(simt, temp);
                        break;
                    }
                }
            }
        }
    }

    // note: m_preStepPreviousGearFullyUncompressedAltitude is updated explicitly in our UpdatePreviousFieldsPreStep method
}

void AltitudeCalloutsPreStep::PlayAltitude(const double simt, const char* pFilename)
{
    m_nextMinimumCalloutTime = simt + 1;    // reset timer

    GetXR1().LoadXR1Sound(GetXR1().AltitudeCallout, pFilename, XRSound::PlaybackType::Radio);  // audible outside vessel as well
    GetXR1().PlaySound(GetXR1().AltitudeCallout, DeltaGliderXR1::ST_AltitudeCallout);
}

//---------------------------------------------------------------------------

// Note: this prestep also handles prevention of docking if nosecone is closed
DockingCalloutsPreStep::DockingCalloutsPreStep(DeltaGliderXR1& vessel) :
    XR1PrePostStep(vessel),
    m_previousDistance(-1), m_nextMinimumCalloutTime(-1), m_previousSimt(-1), m_previousWasDocked(false),
    m_undockingMsgTime(-1), m_intervalStartTime(-1), m_intervalStartDistance(-1)
{
}

void DockingCalloutsPreStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())  // covers IsCrashed() as well
        return;     // no callouts if crashed

    // enable/disable the default XRSound docking thump + sounds: this is necessary so we don't hear the docking
    // sound at all (we are going to *undock* the ship just below.
    const bool bDockingThumpEnabled = (GetXR1().nose_status == DoorStatus::DOOR_OPEN);
    GetXR1().XRSoundOnOff(XRSound::Docking, bDockingThumpEnabled);

    // if the ship is marked as DOCKED by Orbiter but the nose is not open, UNDOCK IT
    if ((GetXR1().GetFlightStatus() & 0x2) && (GetXR1().nose_status != DoorStatus::DOOR_OPEN))
        GetXR1().Undock(0);     // undock port #0 (our only port)

    // check if docked 
    if (GetXR1().IsDocked())  // this will also return FALSE if the nosecone is not open
    {
        // check whether we just docked
        if (m_previousDistance >= 0)
        {
            // Note: this is a *docking distance callout*, not a normal *information* message
            GetXR1().ShowInfo("Contact.wav", DeltaGliderXR1::ST_DockingDistanceCallout, "Docking Port Contact!");
            m_previousDistance = -1;    // reset
        }

        m_previousWasDocked = true;   // remember this
        m_previousSimt = simt;
        return;     // nothing more to do when docked
    }
    else  // not docked
    {
        // check whether we just undocked IF we have had time to set m_previousWasDocked before
        if ((m_previousSimt >= 0) && m_previousWasDocked)
            m_undockingMsgTime = simt + 0.667;    // wait 2/3-second before playing confirmation

        if ((m_undockingMsgTime > 0) && (simt >= m_undockingMsgTime))
        {
            GetXR1().ShowInfo("Undocking Confirmed.wav", DeltaGliderXR1::ST_InformationCallout, "Undocking confirmed.");
            m_undockingMsgTime = -1;    // reset
        }

        m_previousWasDocked = false;   // remember this
    }

    // returns -1 if no docking target set
    const double distance = GetDockingDistance();
    if (distance < 0)
    {
        // no docking port in range, so reset intervals
        m_intervalStartTime = -1;
        m_intervalStartDistance = -1;
    }
    else   // docking port is in range, so check whether we need to reinitialize m_intervalStartTime
    {
        if (m_intervalStartTime < 0)
        {
            // docking port just came into range
            m_intervalStartDistance = distance;
            m_intervalStartTime = simt;
        }
    }

    if ((distance >= 0) && (m_previousDistance >= 0))  // no callouts if not in range OR if we just entered range but haven't updated previous distance yet.
    {
        _ASSERTE(m_intervalStartTime >= 0);
        _ASSERTE(m_intervalStartDistance >= 0);

        // Note: in order to support UCD (Universal Cargo Deck), we need to only play the warning if the ship has closed at least 0.1 meter over the last second (0.1 m/s)
        // Vessel distance "jitters" even when a vessel is attached to UCD which is attached in the XR payload bay.
        const double timeSinceIntervalStart = simt - m_intervalStartTime;
        double closingRate = 0;
        if (timeSinceIntervalStart >= 1.0)  // time to take another interval measurement?
        {
            // see if we are closing at >= 0.1 meter-per-second (postive == approaching the docking port)
            closingRate = -((distance - m_intervalStartDistance) / timeSinceIntervalStart);

            // DEBUG: sprintf(oapiDebugString(), "closingRate=%lf, m_intervalStartDistance=%lf, m_intervalStartTime=%lf, simt=%lf", closingRate, m_intervalStartDistance, m_intervalStartTime, simt);

            // reset for next interval measurement
            m_intervalStartDistance = distance;
            m_intervalStartTime = simt;
        }

        // if within 100 meters and closing at >= 0.02 meter-per-second, warn pilot if nosecone is closed; do NOT warn him if nosecone is OPEN or OPENING
        if ((distance < 100) && (closingRate >= 0.02) && ((GetXR1().nose_status != DoorStatus::DOOR_OPEN) && (GetXR1().nose_status != DoorStatus::DOOR_OPENING)))
        {
            char msg[128];
            sprintf(msg, "ALERT: %s is closed!", NOSECONE_LABEL);
            GetXR1().ShowWarning(WARNING_NOSECONE_IS_CLOSED_WAV, DeltaGliderXR1::ST_DockingDistanceCallout, msg);
        }

        // do not play callouts until minimum time has elapsed, in case pilot is hovering at the same distance
        // also, do not play on the FIRST frame of the simulation or if there is no active docking target
        if ((simt >= m_nextMinimumCalloutTime) && (m_previousDistance >= 0))
        {
            static const double distanceCallouts[] =
            {
                5000, 4000, 3000, 2000, 1000, 900, 800, 700, 600,
                500, 400, 300, 200, 100, 75, 50, 40, 30, 20, 15, 10,
                9, 8, 7, 6, 5, 4, 3, 2, 1
            };

            // optimization: skip loop if distance > max callout distance
            if (distance <= distanceCallouts[0])
            {
                for (int i = 0; i < (sizeof(distanceCallouts) / sizeof(double)); i++)
                {
                    const double a = distanceCallouts[i];

                    // play on approach only
                    if ((m_previousDistance > a) && (distance <= a))   // closing
                    {
                        char temp[64];
                        sprintf(temp, "%d.wav", static_cast<int>(a));
                        PlayDistance(simt, temp);
                        m_nextMinimumCalloutTime = simt + 1.0;  // reset
                        break;
                    }
                }
            }
        }
    }

    // save for next loop
    m_previousSimt = simt;
    m_previousDistance = distance;
}

void DockingCalloutsPreStep::PlayDistance(const double simt, const char* pFilename)
{
    m_nextMinimumCalloutTime = simt + 1;    // reset timer

    // use altitude callout since we won't be docking in an atmosphere
    GetXR1().LoadXR1Sound(GetXR1().AltitudeCallout, pFilename, XRSound::PlaybackType::Radio);  // audible outside vessel as well
    GetXR1().PlaySound(GetXR1().AltitudeCallout, DeltaGliderXR1::ST_DockingDistanceCallout);
}

// returns distance to target docking port in meters, or -1 if no port set
double DockingCalloutsPreStep::GetDockingDistance()
{
    double distance = -1;  // assume no target

    // obtain the global position of our docking port
    DOCKHANDLE hOurDock = GetVessel().GetDockHandle(0);
    VECTOR3 ourDockingPortLocalCoord;
    VECTOR3 temp;   // reused
    GetVessel().GetDockParams(hOurDock, ourDockingPortLocalCoord, temp, temp);

    VECTOR3 ourPos;
    GetVessel().Local2Global(ourDockingPortLocalCoord, ourPos);


    // NOTE: as of the XR1 1.9 release group, we no longer track XPDR for docking distance: this should fix
    // the spurrious "Nosecone is closed" warnings when using Universal Cargo Deck and vessels attached that 
    // default to the 108 MHz radio xpdr frequency and the XR also has a radio tuned to that default frequency.

    // NOTE: Orbiter does not provide a way for us to determine which NAV radio is marked "active" by the radio MFD,
    // so we have to just make a "best guess" by walking through all four of our nav radios and choosing a frequency
    // based on two criteria: 1) the closest TRANSMITTER_IDS in range, or 2) the closest TRANSMITTER_XPDR in range.
    double closestIDS = -1;      // out-of-range
    //double closestXPDR = -1;     // out-of-range

    // find the closest TRANSMITTER_IDS and TRANSMITTER_XPDR values
    for (int i = 0; i < 4; i++)
    {
        NAVHANDLE hNav = GetVessel().GetNavSource(i);
        if (hNav != nullptr)  // tuned and in range?
        {
            NAVDATA navdata;
            oapiGetNavData(hNav, &navdata);
            if ((navdata.type == TRANSMITTER_IDS) || (navdata.type == TRANSMITTER_XPDR))
            {
                // obtain target position (will either be the vessel itself (XPDR) or the docking port (IDS)
                VECTOR3 targetPos;
                oapiGetNavPos(hNav, &targetPos);

                // compute the distance between our docking port and the IDS or XPDR target
                VECTOR3 dp = ourPos - targetPos;       // delta position
                distance = sqrt((dp.x * dp.x) + (dp.y * dp.y) + (dp.z * dp.z));

                if (navdata.type == TRANSMITTER_IDS)
                {
                    // verify that the vessel is NOT attached in our cargo bay
                    if ((GetXR1().m_pPayloadBay != nullptr) && !(GetXR1().m_pPayloadBay->IsChildVesselAttached(navdata.ids.hVessel)))
                    {
                        if ((closestIDS == -1) || (distance < closestIDS))
                        {
                            closestIDS = distance;     // best IDS match so far
                        }
                    }
                }
                /*                else    // XPDR
                                {
                                    // verify that the vessel is NOT attached in our cargo bay
                                    if ((GetXR1().m_pPayloadBay != nullptr) && !(GetXR1().m_pPayloadBay->IsChildVesselAttached(navdata.xpdr.hVessel)))
                                    {
                                        if ((closestXPDR == -1) || (distance < closestXPDR))
                                        {
                                            closestXPDR = distance;     // best XPDR match so far
                                        }
                                    }
                                }
                */
            }
        }
    }

    // if any IDS found in range, use the closest one; otherwise, use the closest XPDR 
    if (closestIDS != -1)
    {
        distance = closestIDS;
        // sprintf(oapiDebugString(), "Docking distance [IDS] = %f", distance);
    }
    /*    else if (closestXPDR != -1)
        {
            distance = closestXPDR;
            // sprintf(oapiDebugString(), "Docking distance [XPDR] = %f", distance);
        }
    */
    return distance;
}
