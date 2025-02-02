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
#include "AreaIDs.h"

//---------------------------------------------------------------------------

// NOTE: this is also active if DESCENT HOLD is activated in order to hold the ship level

AttitudeHoldPreStep::AttitudeHoldPreStep(DeltaGliderXR1& vessel) :
    XR1PrePostStep(vessel),
    m_prevCustomAutopilotMode(AUTOPILOT::AP_NOTSET), m_performedAPUWarningCallout(false), m_apuRanOnceWhileAPActive(false),
    m_forceOnlineCallout(false)
{
    ResetLearningData();
    ResetLastYawThrusterLevels();
}

void AttitudeHoldPreStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    if (GetXR1().IsCrashed())  // note: autopilot still works if crew is incapacitated!
        return;     // nothing to do

    const AUTOPILOT customAutopilotMode = GetXR1().m_customAutopilotMode;
    const bool descentHoldActive = (customAutopilotMode == AUTOPILOT::AP_DESCENTHOLD);

    // special check: if descent hold active BUT our previous autopilot mode as Attitude Hold, must reset the attitude hold autopilot data here
    if (descentHoldActive && (m_prevCustomAutopilotMode == AUTOPILOT::AP_ATTITUDEHOLD))
        ResetAutopilot();

    // if ATTITUDE HOLD or DESCENT HOLD engaged, ensure that AUTO MODE is set *and* update max RCS thrust levels once per second to adjust for payload mass changes.
    if ((customAutopilotMode == AUTOPILOT::AP_ATTITUDEHOLD) || (descentHoldActive))
    {
        if (GetXR1().m_cogShiftAutoModeActive == false)
        {
            GetXR1().m_cogShiftAutoModeActive = true;   // this will LOCK the manual COG shift controls
            GetXR1().TriggerRedrawArea(AID_COG_AUTO_LED);
        }

        // Note: no need to check for CENTER mode active here; the AutoCenteringSimpleButtonAreasPostStep will handle it.
    }

    // check whether the ATTITUDE HOLD or DESCENT HOLD autopilot is engaged AND that we have already set the previous state correctly
    if (((customAutopilotMode == AUTOPILOT::AP_ATTITUDEHOLD) || descentHoldActive) && (m_prevCustomAutopilotMode != AUTOPILOT::AP_NOTSET))
    {
        // zero major control surfaces if configured to do so in the pref file
        if (GetXR1().GetXR1Config()->EnableManualFlightControlsForAttitudeHold == false)
        {
            GetVessel().SetControlSurfaceLevel(AIRCTRL_ELEVATOR, 0);
            GetVessel().SetControlSurfaceLevel(AIRCTRL_RUDDER, 0);
            GetVessel().SetControlSurfaceLevel(AIRCTRL_AILERON, 0);
            // do not reset flaps; they are not used on the XR1, but are used by the XR5 subclass
            // do not reset rudder trim or elevator trim.
        }

        // If we are outside an atmosphere, recenter the COG if it is off-center.
        if ((GetXR1().InAtm() == false) && (GetXR1().m_centerOfLift != NEUTRAL_CENTER_OF_LIFT))
        {
            GetXR1().m_cogForceRecenter = true;     // signal that the autopilot is requesting this; NOTE: no need for us to reset this; the PreStep will do it automatically.
            GetXR1().SetRecenterCenterOfGravityMode(true);
        }

        // suspend autpilot if time acc is too high
        const double timeAcc = oapiGetTimeAcceleration();
        if ((timeAcc > 100.0) || (GetXR1().InAtm() && (timeAcc > 60)))
        {
            GetXR1().m_customAutopilotSuspended = true;
            return;
        }
        else
        {
            GetXR1().m_customAutopilotSuspended = false;  // reset
        }

        // get our angular velocity in degrees per second
        // NOTE: 
        //  x = pitch
        //  y = yaw (slip angle)
        //  z = roll
        VECTOR3 angularVelocity;
        GetVessel().GetAngularVel(angularVelocity);
        angularVelocity *= DEG; // convert to degrees

        // handle BANK
        double targetBank = (descentHoldActive ? 0 : GetXR1().m_setBank);             // in degrees; -180 to +180
        const double currentBank = GetVessel().GetBank() * DEG;   // in degrees

        //
        // handle *inverted* attitude hold
        //
        const bool isInverted = (abs(currentBank) > 90);
        const THGROUP_TYPE ttBankLeft = THGROUP_ATT_BANKLEFT;
        const THGROUP_TYPE ttBankRight = THGROUP_ATT_BANKRIGHT;

        // assume NOT inverted
        THGROUP_TYPE ttPitchUp = THGROUP_ATT_PITCHUP;
        THGROUP_TYPE ttPitchDown = THGROUP_ATT_PITCHDOWN;
        THGROUP_TYPE ttYawLeft = THGROUP_ATT_YAWLEFT;
        THGROUP_TYPE ttYawRight = THGROUP_ATT_YAWRIGHT;

        if (isInverted)  // for efficiency, plus we do invert the pitch & yaw thruster deltas in here
        {
            // swap pitch and yaw thruster directions here since we are inverted
           // swap(ttPitchUp, ttPitchDown);
            //swap(ttYawLeft, ttYawRight);

            // example 1 numbers here are for banking right (clockwise)        across the -179 -> +179 threshold: we want currentBank to DECREASE 2 degrees in this sample case
            // example 2 numbers here are for banking left (counter-clockwise) across the +179 -> -179 threshold: we want currentBank to INCREASE 2 degrees in this sample case
            // example 3 numbers here are for banking left (counter-clockwise) from +100 to -100, crossing the +179 -> -179 threshold along the way: we want currentBank to INCREASE 160 (80 + 80) degrees in this sample case
            // Example #4 is moot here since its case is not inverted, but it is here for testing the math anyway: 
            // example 4 numbers here are for banking left (counter-clockwise) from  -20 to  +20, crossing the 0 threshold along the way: we want currentBank to DECREASE 40 (20 + 20) degrees in this sample (normal) case

            // example 1:                     +179    - -179 = +358 degree-rotation needed to reach via normal operation
            // example 2:                     -179    - +179 = -358 degree-rotation needed to reach via normal operation
            // example 3:                     -100    - +100 = +200 degree-rotation needed to reach via normal operation
            // example 4:                     -20     - +20  =  -40 degree-rotation needed to reach via normal operation
            const double normalDistance = abs(targetBank - currentBank);

            // example 1:                                         +179    - -179 = +358 - 360    = -2   (abs 2)   : 2-degree rotation needed via this method
            // example 2:                                         -179    - +179 = -358 - 360    = 718  (abs 718) : 718-degree rotation needed via this method
            // example 3:                                         -100    - +100 = -200 - 360    = -560 (abs 560) : 560-degree rotation needed via this method
            // example 4:                                         -20     - +20  =  -40 - 360    = -400 (abs 400) : 400-degree rotation needed via this method
            const double crossThresholdBankingRightDistance = abs(targetBank - currentBank - 360);  // rotating clockwise along +Z axis

            // example 1:                                         +179    - -179 = +358 + 360    = +718 (abs 718) : 718-degree rotation needed via this method
            // example 2:                                         -179    - +179 = -358 + 360    = +2   (abs 2)   : 2-degree rotation needed via this method
            // example 3:                                         -100    - +100 = -200 + 360    = +160 (abs 160) : 160-degree rotation needed via this method
            // example 4:                                         -20     - +20  =  -40 + 360    = +320 (abs 320) : 320-degree rotation needed via this method
            const double crossThresholdBankingLeftDistance = abs(targetBank - currentBank + 360);  // rotating counter-clockwise along +Z axis
            if ((crossThresholdBankingRightDistance < normalDistance) && (crossThresholdBankingRightDistance < crossThresholdBankingLeftDistance))
            {
                // example 1 falls into here
                // we're banking right (clockwise), going from -179 to +179. Therefore, translate target of +179 to -181 for this frame (-360).
                targetBank -= 360;
            }
            else if ((crossThresholdBankingLeftDistance < normalDistance) && (crossThresholdBankingLeftDistance < crossThresholdBankingRightDistance))
            {
                // examples 2 & 3 fall into here
                // we're banking left (counter-clockwise), going from +179 to -179. Therefore, translate target of -179 to +181 for this frame (+360).
                targetBank += 360;
            }
            // else [example 4 would fall to here if the inverted check wasn't in place] this is normal operation (inverted threshold not crossed), so no adjustment needed
        }

        // ignore return value here; bank targets should never request COL changes
        FireThrusterGroups(targetBank, currentBank, angularVelocity.z, ttBankRight, ttBankLeft, simdt, 20.0, false, isInverted, AXIS::ROLL);  // never invert angular velocity target for roll

        // never CLEAR this flag here; once the initial bank is complete, this flag remains set until the autopilot is disengaged 
        // (UNLESS the AP has to snap across +90 or -90 on the bank setting: see LimitAttitudeHoldPitchAndBank method in XRVessel.cpp 
        GetXR1().m_initialAHBankCompleted |= (fabs(currentBank - targetBank) <= 3.0);

        // handle pitch ONLY if we completed our initial roll! This allows a clean roll without inducing excessive slip when first engaging the autopilot
        if (GetXR1().m_initialAHBankCompleted)
        {
            double requestedColShift = 0.0;     // requested center-of-lift shift; returned by FireThrusterGroups

            // NOTE: if Descent Hold is active, always operate in HOLD PITCH mode!
            if ((descentHoldActive == false) && GetXR1().m_holdAOA)
            {
                // trying to hold AOA
                const double currentPitch = GetVessel().GetPitch() * DEG;   // in degrees
                const double currentAOA = GetVessel().GetAOA() * DEG;       // in degrees
                const double targetAOA = GetXR1().m_setPitchOrAOA;          // in degrees

                // SPECIAL CHECK: if current PITCH is outside the MAX_ATTITUDE_HOLD_NORMAL range, hold on the pitch boundary and do not try to continue pitching the ship!
                // HOWEVER, allow the ship to REDUCE AOA TOWARD ZERO if necessary
                double newPitchTarget = 0;   // none set yet
                if (fabs(targetAOA) >= fabs(currentAOA))    // are we moving further away from level?
                {
                    if (currentPitch > MAX_ATTITUDE_HOLD_ABSOLUTE_PITCH_OR_AOA)
                        newPitchTarget = MAX_ATTITUDE_HOLD_ABSOLUTE_PITCH_OR_AOA;
                    else if (currentPitch < -MAX_ATTITUDE_HOLD_ABSOLUTE_PITCH_OR_AOA)
                        newPitchTarget = -MAX_ATTITUDE_HOLD_ABSOLUTE_PITCH_OR_AOA;
                }

                if (newPitchTarget != 0)
                {
                    // we are outside the maximum allowable pitch range trying to hold AOA!  Execute PITCH hold instead at the pitch limit.
                    // Note: always invert thruster rotation vs. angular velocity since we're holding since we're holding PITCH here
                    requestedColShift = FireThrusterGroups(newPitchTarget, currentPitch, angularVelocity.x, ttPitchUp, ttPitchDown, simdt, 20.0, true, isInverted, AXIS::PITCH);
                }
                else    // pitch is still OK; let's keep tracking AOA hold
                {
                    // holding AOA
                    // NOTE: must *not* reverse thruster direction if ship is INVERTED since AoA then goes UP when pitch goes DOWN
                    requestedColShift = FireThrusterGroups(targetAOA, currentAOA, angularVelocity.x, ttPitchUp, ttPitchDown, simdt, 20.0, !isInverted, isInverted, AXIS::PITCH);
                }
            }
            else  // holding PITCH
            {
                const double targetPitch = (descentHoldActive ? 0 : GetXR1().m_setPitchOrAOA);  // in degrees
                const double currentPitch = GetVessel().GetPitch() * DEG;   // in degrees
                // Note: always invert thruster rotation vs. angular velocity since we're holding since we're holding PITCH here
                requestedColShift = FireThrusterGroups(targetPitch, currentPitch, angularVelocity.x, ttPitchUp, ttPitchDown, simdt, 20.0, true, isInverted, AXIS::PITCH);
            }

            // reduce COG shift by time acc to maintain stability in atmosphereic flight under time acceleration
            requestedColShift /= timeAcc;

            // Only check for APU if we need to do a COL shift; this will only be requested if we are holding pitch
            // in an atmosphere and we need more than a small amount of RCS power.
            if (requestedColShift != 0.0)
            {
                // adjust the center of lift if requested; note that the shift may be positive (forward) or negative (aft)
                // Note, however, that APU power is necessary for this!
                if (GetXR1().CheckHydraulicPressure(false, false) == false)  // no audio for this; we handle it here
                {
                    // APU offline!  Play a warning callout if we have not done it *once and only once* since the autopilot was engaged.
                    // However, don't play a message here until at least 4 seconds after the simulation started; this will prevent us from
                    // stepping on the "All Systems Nominal" callout at startup.
                    // Also, don't perform this check if the APU is starting up.
                    if ((m_performedAPUWarningCallout == false) && (simt >= 4.0) && (GetXR1().apu_status != DoorStatus::DOOR_OPENING))
                    {
                        // NOTE: we will also hit this block if the APU fuel runs out with the autopilot running.
                        // Auto-start the APU if 1) that is enabled in the config file, AND 2) if we have not already auto-started it before while the AP was active.
                        // In other words, never auto-start the APU *twice* unless the pilot disengages and reengages the autopilot.
                        if (GetXR1().GetXR1Config()->APUAutostartForCOGShift && (m_apuRanOnceWhileAPActive == false))
                        {
                            // check the APU fuel 
                            // Note that we could just allow the ActivateAPU() method to check the fuel (which it does), but we want to sound a custom warning message
                            // here related to gravity shift instead of hydraulic pressure.
                            if (GetXR1().m_apuFuelQty <= 0)  // should never be < 0, but just in case...
                            {
                                // Note: we will also hit this if the APU fuel runs out with the autopilot running
                                GetXR1().ShowWarning("WARNING APU Fuel Depleted Center of Gravity Shift Offline.wav", DeltaGliderXR1::ST_WarningCallout, "Warning: APU fuel depleted.&Center of gravity shift offline.");
                            }
                            else
                            {
                                // APU fuel OK -- fire it up
                                // NOTE: this callout must be short (< APU startup time) so we don't step on the upcoming "COG shift online" callout.
                                GetXR1().ShowInfo("APU Autostart.wav", DeltaGliderXR1::ST_InformationCallout, "APU autostart initiated.");
                                GetXR1().ActivateAPU(DoorStatus::DOOR_OPENING);
                                GetXR1().PlayDoorSound(GetXR1().apu_status);  // beep
                                m_forceOnlineCallout = true;    // notify the pilot when system online

                                // SPECIAL CASE: disable the normal "On" call that would normally occur shortly so we don't step on the message we just started
                                GetXR1().m_skipNextAFCallout = true;
                            }
                        }
                        else    // APU auto-start disabled; warn the pilot
                        {
                            GetXR1().ShowWarning("Warning Center of Gravity Shift Offline.wav", DeltaGliderXR1::ST_WarningCallout, "Warning: APU offline; cannot&shift the center of gravity.");
                            m_performedAPUWarningCallout = true;   // don't re-warn the pilot until he turns off the autopilot and turns it on again
                            GetXR1().m_skipNextAPUWarning = true;  // don't perform the normal "APU Offline: no hydraulic pressure" callout; it would be redundant
                        }
                    }
                }
                else  // APU online
                {
                    m_apuRanOnceWhileAPActive = true;       // remember this

                    // if the APU just came online and the autpilot was engaged with it *offline*, notify the pilot
                    if (m_performedAPUWarningCallout || m_forceOnlineCallout)
                    {
                        GetXR1().ShowInfo("Center of Gravity Shift Online.wav", DeltaGliderXR1::ST_InformationCallout, "APU power-up complete; center&of gravity shift online.");
                        m_performedAPUWarningCallout = false;   // reset so the pilot is warned if he turns it off prematurely
                        m_forceOnlineCallout = false;           // latched this request; reset it
                    }

                    // do not perform COL if we are on the ground
                    if (GetVessel().GroundContact() == false)
                    {
                        // perform the COL shift, keeping it in range
                        GetXR1().ShiftCenterOfLift(requestedColShift);

                        // if the current center-of-lift is > the dead zone, request an elevator trim shift as well if the elevators are online
                        // ENHANCEMENT for the Vanguard: *always* request a PARTIAL trim shift based on how close we are to the edge of the dead zone;
                        // if we reach the dead zone, do a full shift at the normal rate.  Inside the dead zone, do a partial shift.
                        // OLD: if ((fabs(GetXR1().m_centerOfLift) > AP_ELEVATOR_TRIM_COL_DEAD_ZONE) && GetXR1().AreElevatorsOperational())
                        if (GetXR1().AreElevatorsOperational())
                        {
                            // bump the trim using a variable rate (movement fraction per second)
                            double trimLevel = GetVessel().GetControlSurfaceLevel(AIRCTRL_ELEVATORTRIM);
                            // OLD: double fracToMove = AP_ELEVATOR_TRIM_SPEED * simdt;
                            double trimStepPercentage = fabs(GetXR1().m_centerOfLift) / AP_ELEVATOR_TRIM_COL_DEAD_ZONE;  // >= 1.0 means use a FULL step
                            if (trimStepPercentage > 1.0)
                                trimStepPercentage = 1.0;   // keep in range
                            double fracToMove = AP_ELEVATOR_TRIM_SPEED * simdt * trimStepPercentage;
                            // DEBUG: sprintf(oapiDebugString(), "trimStepPercentage=%lf", trimStepPercentage);
                            if (GetXR1().m_centerOfLift < 0)
                                trimLevel -= fracToMove;        // nose down
                            else
                                trimLevel += fracToMove;        // nose up
                            GetVessel().SetControlSurfaceLevel(AIRCTRL_ELEVATORTRIM, trimLevel);
                            // debug: sprintf(oapiDebugString(), "col=%lf, elevatorTrim=%lf", GetXR1().m_centerOfLift, trimLevel);
                        }
                    }
                }
            }   // end requested col shift != 0.0
        }   // end initial bank completed

        // handle YAW (kill it) UNLESS we are in AUTO DESCENT mode OR the pilot fired the yaw thrusters himself OR the pilot moved the rudder
        const double currentYawPositiveThrusterGroupLevel = GetVessel().GetThrusterGroupLevel(ttYawLeft);
        const double currentYawNegativeThrusterGroupLevel = GetVessel().GetThrusterGroupLevel(ttYawRight);

        bool pilotFiringYawJets = false;
        if ((m_lastSetYawThrusterGroupLevels[0] <= 1.0) && (m_lastSetYawThrusterGroupLevels[1] <= 1.0))  // are both values valid?
        {
            if ((m_lastSetYawThrusterGroupLevels[0] != currentYawPositiveThrusterGroupLevel) ||
                (m_lastSetYawThrusterGroupLevels[1] != currentYawNegativeThrusterGroupLevel))
            {
                pilotFiringYawJets = true;
                // DEBUG: sprintf(oapiDebugString(), "PILOT FIRING YAW JETS");
            }
            else
            {
                pilotFiringYawJets = false;
                // DEBUG: *oapiDebugString() = 0;
            }
        }

        // treat rudder as active only if dynamic pressure >= 5.0 kPa
        const bool rudderActive = ((GetVessel().GetControlSurfaceLevel(AIRCTRL_RUDDER) != 0) &&
            (GetVessel().GetDynPressure() >= 5.0e3));
        /* DEBUG
        if (rudderActive)
            sprintf(oapiDebugString(), "RUDDER ACTIVE: %lf", GetVessel().GetControlSurfaceLevel(AIRCTRL_RUDDER));
        */

        if ((descentHoldActive == false) && (pilotFiringYawJets == false) && (rudderActive == false))
            KillRotation(angularVelocity.y, ttYawLeft, ttYawRight, simdt, false, m_lastSetYawThrusterGroupLevels);
    }
    else    // neither ATTITUDE HOLD nor DESCENT HOLD engaged -- kill the thrusters and reset the center of lift if the pilot just turned off the autopilot
    {
        if ((m_prevCustomAutopilotMode == AUTOPILOT::AP_ATTITUDEHOLD) || (m_prevCustomAutopilotMode == AUTOPILOT::AP_DESCENTHOLD))
            ResetAutopilot();
    }

    // remember the autopilot mode for the next timestep
    m_prevCustomAutopilotMode = customAutopilotMode;
}

// reset all autopilot data
void AttitudeHoldPreStep::ResetAutopilot()
{
    GetXR1().KillAllAttitudeThrusters();
    ResetLearningData();
    ResetLastYawThrusterLevels();
    ResetCenterOfLift();
    m_performedAPUWarningCallout = false;
    m_apuRanOnceWhileAPActive = false;
    m_forceOnlineCallout = false;
}

void AttitudeHoldPreStep::ResetCenterOfLift()
{
    // turn off auto mode
    GetXR1().m_cogShiftAutoModeActive = false;   // this will UNLOCK the manual COG shift controls
    GetXR1().TriggerRedrawArea(AID_COG_AUTO_LED);

    // no warning callout here: we already handled that elsewhere if and when it was necessary
    if (GetXR1().CheckHydraulicPressure(false, false) == false)  // no audio
        return;     // cannot reset lift

    // only enable shift to center if the COG is off-center; this will prevent the button from flickering for an instant if the COG is already centered
    if (GetXR1().m_centerOfLift != NEUTRAL_CENTER_OF_LIFT)
    {
        GetXR1().SetRecenterCenterOfGravityMode(true);
        GetXR1().m_cogForceRecenter = true;             // override AUTO MODE check
    }
    // redraw already triggered above: GetXR1().TriggerRedrawArea(AID_COG_CENTER_BUTTON); // light up the button, too
}

void AttitudeHoldPreStep::ResetLearningData()
{
    m_pitchLearningData.Reset();
}

// angularVelocity = degrees/second; NOTE: MAY BE NEGATIVE!
// angVelLimit = angular velocity limit in degrees/second
// reverseRotation = true to reverse rotation thrust (positive degreesDelta == positive angular velocity as well); e.g., for PITCH axis
// Returns: requested center-of-lift shift in meters; will be 0.0 for non-pitch axes or if not in an atmosphere.
double AttitudeHoldPreStep::FireThrusterGroups(const double targetValue, const double currentValue, double angularVelocity, THGROUP_TYPE thgPositive, THGROUP_TYPE thgNegative, const double simdt, double angVelLimit, const bool reverseRotation, const bool isShipInverted, const AXIS axis, const double masterThrustFrac)
{
    double retVal = 0.0;                     // assume no center-of-lift shift
    const double targetDeadZone = 0.01;      // in degrees (very tight hold)
    const double angVelDeadZone = 0.01;      // in degrees/second

    const bool descentHoldActive = (GetXR1().m_customAutopilotMode == AUTOPILOT::AP_DESCENTHOLD);

    // handle inverted attitude hold 
    if (isShipInverted && (axis != AXIS::ROLL))
    {
        swap(thgPositive, thgNegative);      // swap the thrusters
        angularVelocity = -angularVelocity;  // target angular velocity is reversed b/c the ship is upside-down & the thrusters are reversed now
    }

    // compute the optimal closing rate based on how far we have to go yet before reaching target attitude
    // NOTE: may be negative here!
    double degreesDelta = targetValue - currentValue;
    const bool deltaAngleDirection = (degreesDelta >= 0);

    // only fire thrusters if outside our deadzone
    if (fabs(degreesDelta) > targetDeadZone)
    {
        // if degreesDelta is NEGATIVE, we want a POSITIVE targetAngVel to counteract it unless the REVERSE flag is set
        // NOTE: do not reduce this too much, or the autopilot cannot hold a given angle precisely enough!
        // However, if it is too high the ship will oscillate due to too much thrust.
        // NOTE: this is value #1 to tweak if you want to fine-tune time acc behavior and accuracy
        double targetAngVel = degreesDelta * AP_ANGULAR_VELOCITY_DEGREES_DELTA_FRAC;  // rotation rate in degrees per second to reach target in reasonable time

        // if we have not reached our initial roll attitude, set a minimum roll rate here so we can reach it faster
        if (GetXR1().m_initialAHBankCompleted == false)
        {
            const double minAngVel = 10;     // minimum initial rotation = 10 degrees per second
            if (targetAngVel < 0)
            {
                if (targetAngVel > -minAngVel)
                    targetAngVel = -minAngVel;
            }
            else  // targetAngVel >= 0
            {
                if (targetAngVel < minAngVel)
                    targetAngVel = minAngVel;
            }
        }

#if 0   // DEBUG ONLY
        if (axis == ROLL)  // roll only
            sprintf(oapiDebugString(), "targetAngVel=%lf, m_initialAHBankCompleted=%d", targetAngVel, GetXR1().m_initialAHBankCompleted);
#endif

        // NOTE: must allow target angular velocity to reach zero here!  This is what determines whether we rotate or not.

        // reverse rotation if requested (i.e., for pitch)
        if (reverseRotation == false)
            targetAngVel = -targetAngVel;

        // check upper rotation limit (no lower limit, since we want rotation to stop once we reach our target)
        if (targetAngVel > angVelLimit)
            targetAngVel = angVelLimit;
        else if (targetAngVel < -angVelLimit)
            targetAngVel = -angVelLimit;

        // reduce thruster level as timestep size increases
        // NOTE: autopilot cannot hold attitude in atmosphere at 100x; however, it can in space.  Auto-suspend was handled previously by the PreStep.
        const double timeAccDivisor = max((simdt / 0.025), 1.0);   // min framerate for full-speed rotation (thruster levels) is 1/40-second (40 frames/sec)
        double thLevel = masterThrustFrac / timeAccDivisor;    // was 1.0 / timeAccDivisor

        // reduce thrust level if we are close to our angular velocity target already
        const double deltaV = fabs(targetAngVel - angularVelocity);

        // NOTE: this is the primary setting to control negative RCS thrust levels when we overshoot the target angular velocity
        if (descentHoldActive)
            thLevel *= min(1.0, deltaV * 1.0);  // must be more aggressive in holding attitude while hovering (only reduce thrust within 1 meter per second); NOTE: used to be x5.0.
        else    // normal attitude hold
            thLevel *= min(1.0, deltaV / 5);  // reduce thrust levels if within 5 meters per second

        //
        // Handle PITCH learning autopilot here to hold a stable pitch during reentry
        //
        // modify learning thrust fraction based on whether we closed on the target since the previous frame AND if we are reducing thrust because we are close
        double newLearningThrustFrac = 0;       // set below
        double learningThrustStep = 0;          // set below
        const bool activeLearningThrustDirection = (currentValue >= 0);  // only do learning mode for UP pitch

        // holding pitch in ATM applies to descent hold as well
        bool holdingPitchInAtm = (GetXR1().InAtm() && (axis == AXIS::PITCH));  // needed by COL adjustment code later; unlike the test below, this works for both positive and negative pitch

        // do NOT apply learning mode if in AUTO DESCENT mode
        if ((descentHoldActive == false) && GetXR1().InAtm() && activeLearningThrustDirection && (axis == AXIS::PITCH))  // only apply learning thrust in an atmosphere for positive pitch
        {
            // direction in which learning thrust is being applied; this will push AGAINST the air trying to rotate the ship
            if (thLevel < 1.0)
            {
                const double timeAcc = oapiGetTimeAcceleration();
                // NOTE: this is value #2 to tweak if you want to fine-tune time acc behavior and accuracy
                // Typical deltaV when holding attitude during reentry is 0.2, which / 50 = 250 frames to "catch up" to attitude target, or 6.25 seconds @ 40 fps
                learningThrustStep = deltaV / 50 / timeAcc;      // thrust step size per frame, modified for timeAcc
                newLearningThrustFrac = m_pitchLearningData.m_thrustFrac;   // will be set to this value if jets actually fire

                // No dead zone here!  If we end up firing the jets, we need accurate data no matter how small it is.
                // Only apply learning thrust if we need to push in the right direction (against the air)
                bool currentAngVelDirection = (targetAngVel >= angularVelocity);
                if (activeLearningThrustDirection == currentAngVelDirection)
                {
                    // back out the last applied learning thrust delta if requested
                    if (m_pitchLearningData.m_reverseLastLearningThrustStep)
                    {
                        newLearningThrustFrac -= m_pitchLearningData.m_lastLearningThrustStep;
                        // NOTE: do not reset 'm_reverseLastLearningThrustStep' flag here; we must only reset it if the jets actually fire and latch our request!
                    }

                    // increase learning thrust
                    newLearningThrustFrac += learningThrustStep;   // need more thrust to decrease ang velocity
                    thLevel += newLearningThrustFrac;   // apply learning thrust
                }
                else    // too much thrust; reduce learning thrust, but do not apply to this frame since thrusters are firing in other direction!
                {
                    // NOTE: cannot simply set newLearningThrustFrac here because the jets might not fire this frame, and we cannot directly
                    // update m_thrustFrac here because this 'else' block may be invoked multiple times before the positive jets fire again.  
                    // Therefore, we simply queue up the change to be applied the next time the positive jets fire.
                    m_pitchLearningData.m_reverseLastLearningThrustStep = true;
                }

                if (newLearningThrustFrac < 0)
                    newLearningThrustFrac = 0;
                else if (newLearningThrustFrac > 1.0)
                    newLearningThrustFrac = 1.0;
            }
        }

        bool positivePitchJetsFired = false;
        bool negativePitchJetsFired = false;
        // NOTE: angularVelocity may be negative here!
        if (angularVelocity > (targetAngVel + angVelDeadZone))
        {
            GetVessel().SetThrusterGroupLevel(thgNegative, thLevel);
            if (axis == AXIS::PITCH)
                negativePitchJetsFired = true;   // remember this
        }
        else
            GetVessel().SetThrusterGroupLevel(thgNegative, 0);

        if (angularVelocity < (targetAngVel - angVelDeadZone))
        {
            GetVessel().SetThrusterGroupLevel(thgPositive, thLevel);
            if (axis == AXIS::PITCH)
                positivePitchJetsFired = true;   // remember this
        }
        else
            GetVessel().SetThrusterGroupLevel(thgPositive, 0);

        // update pitch learning data for next time IF we actually fired the jets to apply the target thrust
        if (positivePitchJetsFired)
        {
            m_pitchLearningData.m_lastLearningThrustStep = learningThrustStep;  // save in case we need to reduce thrust next frame; i.e., back out this change
            m_pitchLearningData.m_thrustFrac = newLearningThrustFrac;
            m_pitchLearningData.m_reverseLastLearningThrustStep = false;  // reset flag since we know it was already processed above because the positive jets fired

#if 0   // DEBUG ONLY
            sprintf(oapiDebugString(), "angularVelocity=%f, targetAngVel=%f, deltaV=%f, deltaAngleDirection=%d, learningThrustFrac[pitch]=%f, learningThrustStep=%f",
                angularVelocity, targetAngVel, deltaV, deltaAngleDirection, newLearningThrustFrac, learningThrustStep);
#endif
        }

        // Adjust center-of-lift if we are holding pitch in an atmosphere and the jets fired at a level outside a dead zone.
        if (holdingPitchInAtm && (thLevel > AP_COL_DEAD_ZONE))
        {
            // Always use no more than a maximum (COL_MAX_SHIFT_RATE) step size per second here; there is no need to reach the target COL instantly.
            // In addition, it is more realistic since the pumps can only shift fuel fore/aft as a given rate.
            const double thLevelStepFraction = min(1.0, (thLevel * AP_COL_THRUSTLEVEL_TO_SHIFTSTEP_RATIO));
            const double stepSize = COL_MAX_SHIFT_RATE * simdt * thLevelStepFraction;
            retVal = (negativePitchJetsFired ? -stepSize : stepSize);

            // NOTE: if the ship is inverted, we need to reverse the COG shift direction because elevator UP == NEGATIVE pitch instead of POSITIVE pitch
            if (isShipInverted)
                retVal = -retVal;
        }

#if 0  // DEBUG ONLY 
        if (axis == PITCH)
        {
            sprintf(oapiDebugString(), "thLevel=%lf, degreesDelta=%lf, angVel=%lf, targetAngVel=%lf, cogShiftRequested=%lf, isShipInverted=%d", thLevel, degreesDelta, angularVelocity, targetAngVel, retVal, isShipInverted);
        }
#endif
        //sprintf(oapiDebugString(), "pitch=%lf, roll=%lf, slip=%lf", GetVessel().GetPitch()*DEG, GetVessel().GetBank() * DEG, GetVessel().GetSlipAngle()*DEG);
    }
    return retVal;
}

// angularVelocity = degrees/second; NOTE: MAY BE NEGATIVE!
// reverseRotation = true to reverse rotation thrust (positive degreesDelta == positive angular velocity as well); e.g., for PITCH axis
// pOutSetThrusterGroupsLevels = double[2] ptr to hold new thruster group values: [0] = thgPositive level, [1] = thgNegative level; may be null
void AttitudeHoldPreStep::KillRotation(const double angularVelocity, const THGROUP_TYPE thgPositive, const THGROUP_TYPE thgNegative, const double simdt, const bool reverseRotation, double* const pOutSetThrusterGroupsLevels, const double masterThrustFrac)
{
    const double angVelDeadZone = 0.05;       // in degrees/second

    // always reduce thruster level as timestep size increases, even if in atmosphere
    const double timeAccDivisor = max((simdt / 0.025), 1.0);     // min framerate for full-speed rotation (thruster levels) is 1/40-second (40 frames/sec)
    double thLevel = masterThrustFrac / timeAccDivisor;

    // reduce thrust level if we are close to our target velocity already
    thLevel *= min(1.0, fabs(angularVelocity) / 3);

    // sprintf(oapiDebugString(), "angularVelocity=%f, thLevel=%f", angularVelocity, thLevel);

    // WARNING: GetVessel().GetThrusterGroupLevel(thgPositive) always returns the current value at the BEGINNING of this timestep, so don't 
    // expect it to be updated immediately after it is set below!

    double newNegativeThLevel = 0;
    if (angularVelocity > angVelDeadZone)
    {
        GetVessel().SetThrusterGroupLevel(thgNegative, thLevel);
        newNegativeThLevel = thLevel;
    }
    else
        GetVessel().SetThrusterGroupLevel(thgNegative, 0);

    double newPositiveThLevel = 0;
    if (angularVelocity < -angVelDeadZone)
    {
        GetVessel().SetThrusterGroupLevel(thgPositive, thLevel);
        newPositiveThLevel = thLevel;
    }
    else
        GetVessel().SetThrusterGroupLevel(thgPositive, 0);

    // set new thruster level data out if requested
    if (pOutSetThrusterGroupsLevels != nullptr)
    {
        pOutSetThrusterGroupsLevels[0] = newPositiveThLevel;
        pOutSetThrusterGroupsLevels[1] = newNegativeThLevel;
    }
}
