// ==============================================================
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// Copyright 2006-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR1PreSteps.cpp
// Class defining custom clbkPreStep callbacks for the DG-XR1
// ==============================================================

#include "DeltaGliderXR1.h"
#include "XR1PreSteps.h"
#include "AreaIDs.h"
#include "XRPayloadBay.h"

//---------------------------------------------------------------------------

// NOTE: this is also active if DESCENT HOLD is activated in order to hold the ship level

AttitudeHoldPreStep::AttitudeHoldPreStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel), 
    m_prevCustomAutopilotMode(AP_NOTSET), m_performedAPUWarningCallout(false), m_apuRanOnceWhileAPActive(false), 
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
    const bool descentHoldActive = (customAutopilotMode == AP_DESCENTHOLD);

    // special check: if descent hold active BUT our previous autopilot mode as Attitude Hold, must reset the attitude hold autopilot data here
    if (descentHoldActive && (m_prevCustomAutopilotMode == AP_ATTITUDEHOLD))
        ResetAutopilot();

    // if ATTITUDE HOLD or DESCENT HOLD engaged, ensure that AUTO MODE is set *and* update max RCS thrust levels once per second to adjust for payload mass changes.
    if ((customAutopilotMode == AP_ATTITUDEHOLD) || (descentHoldActive))
    {
        if (GetXR1().m_cogShiftAutoModeActive == false)
        {
            GetXR1().m_cogShiftAutoModeActive = true;   // this will LOCK the manual COG shift controls
            GetXR1().TriggerRedrawArea(AID_COG_AUTO_LED);
        }

        // Note: no need to check for CENTER mode active here; the AutoCenteringSimpleButtonAreasPostStep will handle it.
    }

    // check whether the ATTITUDE HOLD or DESCENT HOLD autopilot is engaged AND that we have already set the previous state correctly
    if (((customAutopilotMode == AP_ATTITUDEHOLD) || descentHoldActive) && (m_prevCustomAutopilotMode != AP_NOTSET))
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
        const THGROUP_TYPE ttBankLeft  = THGROUP_ATT_BANKLEFT;
        const THGROUP_TYPE ttBankRight = THGROUP_ATT_BANKRIGHT;

        // assume NOT inverted
        THGROUP_TYPE ttPitchUp   = THGROUP_ATT_PITCHUP;  
        THGROUP_TYPE ttPitchDown = THGROUP_ATT_PITCHDOWN;  
        THGROUP_TYPE ttYawLeft   = THGROUP_ATT_YAWLEFT;
        THGROUP_TYPE ttYawRight  = THGROUP_ATT_YAWRIGHT;

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
            const double crossThresholdBankingLeftDistance  = abs(targetBank - currentBank + 360);  // rotating counter-clockwise along +Z axis
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
        FireThrusterGroups(targetBank, currentBank, angularVelocity.z, ttBankRight, ttBankLeft, simdt, 20.0, false, isInverted, ROLL);  // never invert angular velocity target for roll

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
                    requestedColShift = FireThrusterGroups(newPitchTarget, currentPitch, angularVelocity.x, ttPitchUp, ttPitchDown, simdt, 20.0, true, isInverted, PITCH);
                }
                else    // pitch is still OK; let's keep tracking AOA hold
                {
                    // holding AOA
                    // NOTE: must *not* reverse thruster direction if ship is INVERTED since AoA then goes UP when pitch goes DOWN
                    requestedColShift = FireThrusterGroups(targetAOA, currentAOA, angularVelocity.x, ttPitchUp, ttPitchDown, simdt, 20.0, !isInverted, isInverted, PITCH);  
                }
            }
            else  // holding PITCH
            {
                const double targetPitch = (descentHoldActive ? 0 : GetXR1().m_setPitchOrAOA);  // in degrees
                const double currentPitch = GetVessel().GetPitch() * DEG;   // in degrees
                // Note: always invert thruster rotation vs. angular velocity since we're holding since we're holding PITCH here
                requestedColShift = FireThrusterGroups(targetPitch, currentPitch, angularVelocity.x, ttPitchUp, ttPitchDown, simdt, 20.0, true, isInverted, PITCH); 
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
                    if ((m_performedAPUWarningCallout == false) && (simt >= 4.0) && (GetXR1().apu_status != DOOR_OPENING))
                    {
                        // NOTE: we will also hit this block if the APU fuel runs out with the autopilot running.
                        // Auto-start the APU is enabled in the config file AND if we have not already auto-started it before while the AP was active.
                        // In other words, never auto-start the APU *twice* unless the pilot disengages and reengages the autopilot.
                        if (GetXR1().GetXR1Config()->APUAutostartForCOGShift && (m_apuRanOnceWhileAPActive == false))
                        {
                            // check the APU fuel 
                            // Note we could just allow the ActivateAPU() method to check the fuel (which it does), but we want to sound a custom warning message
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
                                GetXR1().ActivateAPU(DOOR_OPENING);
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
        const double currentYawPositiveThrusterGroupLevel  = GetVessel().GetThrusterGroupLevel(ttYawLeft); 
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
        if ((m_prevCustomAutopilotMode == AP_ATTITUDEHOLD) || (m_prevCustomAutopilotMode == AP_DESCENTHOLD))
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

    const bool descentHoldActive = (GetXR1().m_customAutopilotMode == AP_DESCENTHOLD);

    // handle inverted attitude hold 
    if (isShipInverted && (axis != ROLL))
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
        bool holdingPitchInAtm = (GetXR1().InAtm() && (axis == PITCH));  // needed by COL adjustment code later; unlike the test below, this works for both positive and negative pitch

        // do NOT apply learning mode if in AUTO DESCENT mode
        if ((descentHoldActive == false) && GetXR1().InAtm() && activeLearningThrustDirection && (axis == PITCH))  // only apply learning thrust in an atmosphere for positive pitch
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
            if (axis == PITCH)
                negativePitchJetsFired = true;   // remember this
        }
        else
            GetVessel().SetThrusterGroupLevel(thgNegative, 0);

        if (angularVelocity < (targetAngVel - angVelDeadZone))
        {
            GetVessel().SetThrusterGroupLevel(thgPositive, thLevel);
            if (axis == PITCH)
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
void AttitudeHoldPreStep::KillRotation(const double angularVelocity, const THGROUP_TYPE thgPositive, const THGROUP_TYPE thgNegative, const double simdt, const bool reverseRotation, double * const pOutSetThrusterGroupsLevels, const double masterThrustFrac)
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


//---------------------------------------------------------------------------

/*
    AUTO-DESCENT LOGIC:
        1. How much thrust does the ship have beyond the hover point, in m/s/s?
        2. We want to be able to hover the ship @ 1 meter.
        3. How fast can I fall and still be able to stop at 1 meter?  This is based on 1) altitude when slowdown begins, and 2) max acc in m/s/s.
                For example, if I am 100 meters up with max acc of 2 m/s/s, at descent rate of:
                     5 m/s I would need 2.5 seconds to cancel, falling 2.5 * 2.5 = 6.25 meters during that time
                    10 m/s I would need 5 seconds to cancel it out, and I would fall (10 / 2 = 5 m/s average velocity) 5 seconds * 5 mps = 25 meters during that time.
                    20 m/s I would need 10 seconds to cancel it out, and I would fall 10 seconds * 10 m/s = 100 meters during that time.
                    
                    Formula:
                                           5    /    2    = 2.5
                        secondsToCancel = (rate / maxAcc) 
                        
                                            2.5   *    2.5  =  6.25 meters
                        distanceFallen = (rate/2) * secondsToCancel   (rate/2 = average speed as rate goes from N to 0)
                        
                        distanceFallen = (rate/2) * (rate / maxAcc)  =
                        
                                          rate      rate
                                          ----  *   ----      =
                                           2        maxAcc
                                 
                                 100   = 20*20 / 2 * 2 = 
                                 100   = 400 / 4 
                                 100   = 100
                        distanceFallen = (rate*rate) / (2*maxAcc)   =  5*5 / 2 * 2  = 25 / 4 = 6.25 meters
                        
        4. Based on the above formula, for a given (current altitude+1), what is the MAX rate should I hit before beginning to brake?  
           i.e., since I have distance and maxAcc, solve for rate:
                
                distanceFallen = (rate * rate) / (2 * maxAcc)  : 100 = 20*20 / 2 * 2 : 100 = 400 / 4 = 100 (OK)

                distanceFallen = (rate^2) / (2 * maxAcc)
                distanceFallen * (2 * maxAcc) = (rate^2)
                sqrt(distanceFallen * (2 * maxAcc)) = rate
                
                rate = sqrt(distanceFallen * (2 * maxAcc))
                rate = sqrt(distanceFallen * 2 * maxAcc)
                
            Example for 25 meters and 2 m/s/s max acc:
                rate = sqrt(25 * 2 * 2)
                rate = sqrt(50 * 2)
                rate = sqrt(100)
                rate = 10  m/s  (OK)
            
            Example for 100 meters and 2 m/s/s max acc:
                rate = sqrt(100 * 2 * 2)
                rate = sqrt(400)
                rate = 20 m/s

    FORMULA: maxDescentRate = sqrt((altitude+1) * 2 * maxAcc)

*/

// NOTE: requires AttitudeHoldPreStep as well to hold ship level during descent
DescentHoldPreStep::DescentHoldPreStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel), 
    m_prevCustomAutopilotMode(AP_NOTSET)
{
}

void DescentHoldPreStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    if (GetXR1().IsCrashed())  // note: autopilot still works if crew is incapacitated!
        return;     // nothing to do

    // determine maximum hover thrust
    double maxHoverThrust = 0;
    for (int i=0; i < 2; i++)
        maxHoverThrust += GetVessel().GetThrusterMax(GetXR1().th_hover[i]);  // takes atmospheric pressure into account

    // save max hover b/c this is also used by the hover MDA for display purposes
    // NOTE: must not take ATM lift into account here because it is not linear, and so it throws off the calculation.
    // In practice this will give us some extra safety margin in an atmosphere, so it is still OK.
    const double mass = GetVessel().GetMass();
    VECTOR3 W;
    GetVessel().GetWeightVector(W);          // force from the primary G body
    const double maxShipHoverAcc = GetXR1().m_maxShipHoverAcc = (maxHoverThrust + W.y) / mass;  // weight is NEGATIVE

    const AUTOPILOT customAutopilotMode = GetXR1().m_customAutopilotMode;

    // get our altitude adjusted for gear-down
    const double altitude = GetXR1().GetGearFullyCompressedAltitude();   // don't terminate thrusters until we gear is fully compressed

    // if we just engaged DESCENT HOLD, initialize our latchedTargetDescentRate in case we engage auto-land under 20m altitude
    if ((customAutopilotMode == AP_DESCENTHOLD) && (m_prevCustomAutopilotMode == AP_NOTSET))
        GetXR1().m_latchedAutoTouchdownMinDescentRate = -3;  // in case we are not set later before auto-land engages
    // check whether the DESCENT HOLD autopilot is engaged AND that we have already set the previous state correctly
    else if ((customAutopilotMode == AP_DESCENTHOLD) && (m_prevCustomAutopilotMode != AP_NOTSET))
    {
        // NOTE: 'suspend autpilot' checks are handled by the Attitude Hold autopilot code, since that is also enabled when we are enabled

        // verify that the hover doors are open
        if (GetXR1().m_isHoverEnabled == false)
        {
            GetXR1().PlaySound(GetXR1().HoverDoorsAreClosed, DeltaGliderXR1::ST_WarningCallout);
            GetXR1().ShowWarning(NULL, DeltaGliderXR1::ST_None, "WARNING: Hover Doors are closed.");  // NOTE: "descent hold disengaged" will be displayed by SetCustomAutopilot
            GetXR1().SetCustomAutopilotMode(AP_OFF, false);  // do not play sounds for this
        }

        const double timeAcc = oapiGetTimeAcceleration();

        // wait until the ship is level: handled by the AttitudeHold autpilot
        const double currentBank = GetVessel().GetBank() * DEG;     // in degrees
        const double currentPitch = GetVessel().GetPitch() * DEG;   // in degrees

        if ((fabs(currentBank) > 5) || (fabs(currentPitch) > 5))
            return;     // ship not level yet

        // if we just touched down, switch off the autopilot
        if ((altitude <= 0) && ((GetXR1().m_setDescentRate < 0) || GetXR1().m_autoLand))
        {
            GetXR1().SetCustomAutopilotMode(AP_OFF, false);
            goto exit;    // nothing more to do this timestep
        }

        // determine how much margin we have on hover thrust vs. weight
        // NOTE: should not use our m_acceleration data since that was computed in the previous frame's PreStep
        // We would like to make it a PreStep, but testing shows that the acc values kept fluctuating constantly, making the gauges jump.  Orbiter core bug???

        // determine the ship's current acceleration
        VECTOR3 F,D,L;
        GetVessel().GetForceVector(F);           // sum of all forces acting on the ship
        GetVessel().GetDragVector(D);            // force from atm drag
        GetVessel().GetLiftVector(L);            // force from atm lift

        const double negEffectiveShipWeight = (W.y + L.y + D.y);   // sum of all lift and drag forces on the ship w/o any thrust
        const double planetAcc = negEffectiveShipWeight / mass;    // planetary acc on ship in m/s/s, including atm drag and lift
        const double acc = (F.y - W.y) / mass;                     // vehicle vertical acc in m/s/s
        double targetRate = GetXR1().m_setDescentRate;             // negative = descent 

        // determine ship's weight, which determines thrust required in order to hover
        const double weight = (-planetAcc * mass);   // make planetAcc positive

        // if insufficient thrust to hover, warn the pilot
        if (maxHoverThrust < weight)    
        {
            char msg[128];
            double massPctCapacity = (weight / maxHoverThrust * 100);
            // SANITY CHECK to keep massPctCapacity in range to prevent buffer overrun
            if (fabs(massPctCapacity) > 10000)
                massPctCapacity = 10000;    // ignore sign here on this funky condition

            sprintf(msg,  "WARNING: insufficient hover thrust&available to maintain hover!&Ship mass %.0f%% of hover capacity.", massPctCapacity);
            GetXR1().ShowWarning("Warning Insufficient Hover Thrust Available.wav", DeltaGliderXR1::ST_WarningCallout, msg);
			// if grounded, do not attempt to take off; otherwise, fall through and try to slow descent
			if (altitude == 0)
				goto exit;      // nothing more to do this timestep
        }

        // if auto-land enabled, perform a perfect, smooth landing
        if (GetXR1().m_autoLand)    // check whether m_autoLand mode is engaged
        {
            // set target descent rate based on altitude and engine thrust margins

            // Determine the maximum descent rate we can attain and still stop at 1 meter
            //         FORMULA: maxDescentRate = sqrt(currentAltitude * 2 * maxShipHoverAcc)

            const double altitudeTarget = 20.0;  
            const double minAutoDescentRate = -3.0;  // in m/s
            double safeMaxShipHoverAcc = maxShipHoverAcc * 0.80;  // reduce effective maxShipHoverAcc to allow a 20% safety margin, and so the final braking isn't so abrupt

            // NOTE: safeMaxShipHoverAcc may be negative!  If so, it means there isn't enough hover thrust (including the 20% safety margin) to halt the ship's descent.
            // [That would make our sqrt try to operate on a negative number.] 
            // Instead, we allow the ship to descend anyway by assuming the hovers can give us at least 1.0 m/s of acceleration (since that is a safe worst-case touchdown velocity anyway).
            // This is really only a factor when the ship is auto-landing on Earth at a relatively low altitude (so the atmosphere density reduces hover thrust).
            if (safeMaxShipHoverAcc < 1.0)
                safeMaxShipHoverAcc = 1.0;

            if (altitude >= altitudeTarget)      // negative numbers don't have square roots
            {
                targetRate = min(minAutoDescentRate, -sqrt((altitude - altitudeTarget) * 2 * safeMaxShipHoverAcc)); 

                // Cap the target rate based on thrust margin; otherwise, at a high rate of descent with very low margins the ship
                // "falls behind" because in the first timestep where the engines engage, the ship is already moving too fast to stop the descent.
                // Therefore, we cap the limit at the HIGHER of (meaning, a slower descent):
                //   1) -30 m/s/s for each m/s/s of deltaV the hovers can give us, or
                //   2) -1 m/s for each 7 m of altitude (e.g., 14.2 m/s @ 100m, 71 m/s @ 500m, etc.)
                //REMOVED: SHIP IS STABLE NOW:   3) for every 100 kpa of static pressure, -50 m/s; -50 m/s per 1 Earth density atmosphere, -25 m/s for 2xEarth atm, etc. OR 10 m/s min
                //REMOVED: SHIP IS STABLE NOW: We need step #3 because at that descent rate the ship begins to fly instead of hover, making a general mess of hovering anyway...

                // step 1: engine power limits
                const double minDescentForAltitude = -(altitude / 7);          // in m/s

                // step 2: altitude limits
                const double minSafeTargetRate = -(safeMaxShipHoverAcc * 30);  // in m/s
                double workingMinTargetRate = max(minDescentForAltitude, minSafeTargetRate);  

                // step 3: descent rate in atm limits
                /* Removed
                const double earthAtmMult = 1e5 / GetVessel().GetAtmPressure(); // 100 kpa = 1.0, 200 kpa = 0.5, etc.
                const double atmMinTargetRate = min(-10.0, (-50.0 * earthAtmMult));  // always allow at least -10 m/s descent
                if (atmMinTargetRate > workingMinTargetRate)
                    workingMinTargetRate = atmMinTargetRate;    // this is now the slowest descent rate
                */

                // check and set the real rate against our best choice
                if (targetRate < workingMinTargetRate)
                    targetRate = workingMinTargetRate;

                GetXR1().m_latchedAutoTouchdownMinDescentRate = targetRate;  // in case we cross the threshold next frame
            }
            else  // we are below our target altitude for auto-land
            {
                // LATER: FOR ALTITUDE HOLD: targetRate = sqrt(-(altitude - altitudeTarget) * 2 * maxShipHoverAcc);   // ascend to target altitude

                // perform gentle auto-land from here down
                // mesh with existing descent rate when entering touchdown zone @ altitudeTarget, down to 0.20 m/s, and then 0.10 m/s at touchdown
                targetRate = min(-0.20, ((altitude / altitudeTarget) * GetXR1().m_latchedAutoTouchdownMinDescentRate));   
                // DEBUG: sprintf(oapiDebugString(), "m_latchedAutoTouchdownMinDescentRate=%lf, targetRate=%lf, a/at=%lf", GetXR1().m_latchedAutoTouchdownMinDescentRate, targetRate, (altitude / altitudeTarget));

                if (altitude <= 0.25)
                    targetRate = -0.10;  // very soft touchdown
            }
        }

        // get our vertical speed in meters per second
        VECTOR3 v;
		GetXR1().GetAirspeedVector(FRAME_HORIZON, v);
        const double currentDescentRate = (GetVessel().GroundContact() ? 0 : v.y);      // in m/s

        // determine what rate of change (acc) we need in order to hit our target rate in a reasonable timeframe
        // A targetAcc of zero will hold the current descent rate; i.e., the ship will not be accelerated vertically
        double rateDelta = targetRate - currentDescentRate;     // in m/s; may be positive or negative

        // try to arrive at rate quickly (for accuracy) but in a reasonable time period so we don't overdrive the engines and oscillate
        // NOTE: this is the primary value to tune accuracy vs. oscillation
        // if not auto-landing, smooth out targetAcc
        const double absRateDelta = fabs(rateDelta);
        double targetAcc;

        // targetAcc will be rateDelta * (n >= 2.0) 
        // e.g., if rate = 10, mult = 2.0  (1/2-second)
        //       if rate = 20, mult = 4.0  (1/4-second)
        //       if rate = 100, mult = 20.0  (1/20-second)  : [this will certainly induce maximum thrust]
        double rateDeltaMultiplier = max(2.0, (absRateDelta / 5));  // n >= 2.0 (no upper limit)

        // must be more aggressive when auto-land engaged
        if (GetXR1().m_autoLand)
        {
            // there is no upper limit here; this is by design
            // TOO AGGRESSIVE: rateDeltaMultiplier *= 5;   // 5 * 2.0 = 10 = minimum multiplier set to reach target acc in 1/10th-second to keep auto-land accurate
            // This can *almost* land at 100x now (it lands at 80x successfully); at 100x it sometimes can't quite touch down, but in any case it doesn't crash the ship!
            rateDeltaMultiplier *= 2;   // 2 * 2.0 = 4 = minimum multiplier set to reach target acc in 0.25-second to keep auto-land accurate
        }

        // NOTE: (1 / rateDeltaMultiplier) = fraction of second to reach target acc; e.g., 5 = 1/5th-second
        targetAcc = (rateDelta * rateDeltaMultiplier); // target acc range is rateDelta * (n >= 2) m/s/s

        // DEBUG: sprintf(oapiDebugString(), "targetAcc=%f, rateDeltaMultiplier=%f, rateDelta=%f", targetAcc, rateDeltaMultiplier, rateDelta);

        // Determine effective acc required to maintain the requested acc (m/s/s); this takes gravity, drag, and our mass into account
        const double effectiveTargetAcc = -planetAcc + targetAcc;  // planet's pull (including atm drag and lift) + target rate
        
        // determine thrust required to maintain the requested rate of acc (m/s/s)
        double targetThrustRequired = effectiveTargetAcc * mass;   // in kN

        // set hover thrust level required to hold requested acc
        const double requiredThLevel = targetThrustRequired / maxHoverThrust;
        double thLevel = requiredThLevel;
        if (thLevel < 0)
            thLevel = 0;
        else if (thLevel > 1.0)
            thLevel = 1;

        const double thrustPerHoverEngine = targetThrustRequired / 2;
        for (int i = 0; i < 2; i++)
            GetVessel().SetThrusterLevel(GetXR1().th_hover[i], thLevel);

        /* DEBUG
        sprintf(oapiDebugString(), "rateDelta=%f, targetAcc=%f, VAcc=%f, effectiveTargetAcc=%f, targetThrustRequired=%f kN, targetHoverThrottle=%lf", 
            rateDelta, targetAcc, acc, effectiveTargetAcc, (targetThrustRequired / 1000), thLevel);
        */
    }
    else    // DESCENT HOLD not engaged -- kill the thrusters if the pilot just turned off the autopilot
    {
        if (m_prevCustomAutopilotMode == AP_DESCENTHOLD)
        {
            // kill the hover engines if we just touched down
            if (altitude < 0.5)
            {
                for (int i=0; i < 2; i++)
                    GetVessel().SetThrusterLevel(GetXR1().th_hover[i], 0);
            }

            GetXR1().KillAllAttitudeThrusters();
            GetXR1().m_setDescentRate = 0.0;       // reset
        }
    }

exit:
    // remember the autopilot mode for the next timestep
    m_prevCustomAutopilotMode = customAutopilotMode;
}

//-------------------------------------------------------------------------

// NOTE: requires AttitudeHoldPreStep as well to hold ship level during descent
AirspeedHoldPreStep::AirspeedHoldPreStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel), 
    m_prevAirspeedHold(PAH_NOTSET)
{
}

void AirspeedHoldPreStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    if (GetXR1().IsCrashed())  // note: autopilot still works if crew is incapacitated!
        return;     // nothing to do

    // Orbiter has a glitch updating its force vectors in the first few frames, so let's wait 1/10th second before engaging the autopilot
    if (simt < 0.10)
        return;

    // determine maximum main engine thrust
    double maxMainThrust = 0;
    for (int i=0; i < 2; i++)
        maxMainThrust += GetVessel().GetThrusterMax(GetXR1().th_main[i]);  // takes atmospheric pressure into account

    // determine maximum retro engine thrust
    double maxRetroThrust = 0;
    for (int i=0; i < 2; i++)
        maxRetroThrust += GetVessel().GetThrusterMax(GetXR1().th_retro[i]);  // takes atmospheric pressure into account

    // NOTE: must take ATM lift and drag into account here to reduce error amount, since ATM drag is a large part of the airspeed hold equation
    // Also, unlike DESCENT HOLD, the rate at which we reach our target airspeed is not critical.
    const double mass = GetVessel().GetMass();
    VECTOR3 W,F,T;
    GetVessel().GetWeightVector(W);          // force from primary G body
    GetVessel().GetForceVector(F);           // sum of all forces acting on the ship, INCLUDING THRUST and WHEEL DRAG
    GetVessel().GetThrustVector(T);          // force from engines

    // determine how much margin we have on main thrust vs. weight
    // determine the ship's current acceleration
    // NOTE: if grounded, must take surface drag into account, so we can't simply add Weight, Lift, and Drag here; instead, we must
    // take the TOTAL FORCE - thrust.  Unfortunately, Orbiter is adding some other *undocumented* forces in there, so can't use this when airborne.  The
    // only reason we use it when grounded is because there is no other way to obtain wheel drag.
    double negEffectiveShipWeight;
    if (GetXR1().GetGearFullyCompressedAltitude() <= 0.1)  // is the ship on the ground?
    {
        negEffectiveShipWeight = (F.z - T.z);  // sum of all lift and drag forces on the ship, including wheel drag, w/o any thrust
    }
    else  // we are airborne, so use the more accurate calculation without taking wheel drag into account
    {
        VECTOR3 L,D;
        GetVessel().GetLiftVector(L);
        GetVessel().GetDragVector(D);
        negEffectiveShipWeight = W.z + L.z + D.z;  // sum of all lift and drag forces on the ship, EXCLUDING wheel drag, w/o any thrust
    }

    // NOTE: this cannot be const because we may need to reset it to zero later
    double planetAcc = negEffectiveShipWeight / mass;   // planetary acc on ship in m/s/s, including atm drag and lift
    
    // save maxAcc for use by MDA display area as well, which includes ATM data
    GetXR1().m_maxMainAcc = (maxMainThrust + negEffectiveShipWeight) / mass;  // weight (including drag) is NEGATIVE

    // check whether the AIRSPEED HOLD autopilot is engaged AND that we have already set the previous state correctly
    if ((GetXR1().m_airspeedHoldEngaged) && (m_prevAirspeedHold != PAH_NOTSET))
    {
        // suspend autpilot if time acc is too high
        const double timeAcc = oapiGetTimeAcceleration();
        const bool inAtm = GetXR1().InAtm();
        if (timeAcc > 100)
        {
            GetXR1().m_airspeedHoldSuspended = true;
            return;    
        }
        else
        {
            GetXR1().m_airspeedHoldSuspended = false;  // reset
        }

        // NOTE: airspeed hold is turned off on touchdown by TakeoffAndLandingCalloutsAndCrashPreStep.

        double targetVelocity = GetXR1().m_setAirspeed;         // in m/s

        // determine ship's weight and drag, which determines thrust required in order to maintain set airspeed
        const double acc = (F.z - W.z) / mass;        // vehicle horizontal acc in m/s/s
        const double zWeight = (-planetAcc * mass);   // make planetAcc positive

        // get our airspeed in meters per second
        // NOTE: this autopilot really only works in an atmosphere
		const double currentAirspeed = GetVessel().GetAirspeed();  // in m/s

        // DEBUG: sprintf(oapiDebugString(), "maxMainThrust=%lf, zWeight=%lf, diff=%lf", maxMainThrust, zWeight, (maxMainThrust - zWeight));

        // if insufficient thrust to attain requested velocity, warn the pilot
        // SANITY CHECK: if targetVelocity <= currentVelocity, do NOT warn the pilot.  Orbiter seems to glitch every so often and "spike" the planetAcc values way high.
        if ((maxMainThrust < zWeight) && (currentAirspeed < targetVelocity))
        {
            // NOTE: do not show actual percentage here; it varies constantly and makes the tertiary HUD loop, making it difficult to read anyway
            GetXR1().ShowWarning("Warning Insufficient Main Thrust Available.wav", DeltaGliderXR1::ST_WarningCallout, "WARNING: insufficient main thrust&available to accelerate further&at this attitude.");
        }

        // determine what rate of change (acc) we need in order to hit our target airspeed in a reasonable timeframe
        // A targetAcc of zero will hold the current airspeed rate; i.e., the ship will not be accelerated horizontally
        double velDelta = targetVelocity - currentAirspeed;     // in m/s; may be positive or negative
        const double absVelDelta = fabs(velDelta);

        // Try to arrive at rate quickly (for accuracy) but in a reasonable time period so we don't overdrive the engines and oscillate.
        // NOTE: this is the primary value to tune accuracy vs. oscillation
        // targetAcc will be rateDelta * (n >= 2.0 m/s/s) 
        // e.g., if absVelDelta = 10, mult = 2.0   (2.0 m/s/s) : 10 / 5 = 2
        //       if absVelDelta = 20, mult = 4.0   (4.0 m/s/s) : 20 / 5 = 4
        //       if absVelDelta = 100, mult = 20.0 (20.0 m/s/s): 100 / 5 = 20
        // NOTE: velDeltaMultiplier must use absVelDelta because it is merely a positive *multiplier* for a positive or negative *rate*
        const double velDeltaMultiplier = max(2.0, (absVelDelta / 5));  // n >= 1.0 (no upper limit)

        // NOTE: (1 / velDeltaMultiplier) = fraction of second to reach target acc; e.g., 5 = 1/5th-second; may be negative
        double targetAcc = (velDelta * velDeltaMultiplier); // target acc range is [velDelta * (n >= 0.5)] m/s/s

        // WORKAROUND: If grounded and the SET rate == 0, prevent planetAcc from being NEGATIVE here, since it induces thruster oscillations on the ground
        if (GetVessel().GroundContact() && (GetXR1().m_setAirspeed == 0) && (planetAcc < 0))
            planetAcc = 0;

        // Determine effective acc required to maintain the requested acc (m/s/s); this takes gravity, drag, and our mass into account
        const double effectiveTargetAcc = -planetAcc + targetAcc;  // planet's pull (including atm drag and lift) + target rate
        
        // determine thrust required to maintain the requested rate of acc (m/s/s)
        double targetThrustRequired = effectiveTargetAcc * mass;   // in kN

        /* DEBUG
        sprintf(oapiDebugString(), "planetAcc=%f, targetAcc=%f, velDeltaMultiplier=%f, velDelta=%f, effectiveTargetAcc=%f, targetThrustRequired=%f", 
            planetAcc, targetAcc, velDeltaMultiplier, velDelta, effectiveTargetAcc, targetThrustRequired);
        */

        // set main thrust level required to hold requested acc
        const double requiredMainThLevel = targetThrustRequired / maxMainThrust;
        double mainThLevel = requiredMainThLevel;
        if (mainThLevel < 0)
            mainThLevel = 0;
        else if (mainThLevel > 1.0)
            mainThLevel = 1;

#if 0
        // NOT USED; THIS DOES NOT WORK OUTSIDE OF AN ATMOSPHERE BECAUSE THE SHIP CAN POINT AWAY FROM THE VELOCITY VECTOR
        // set retro thrust level required to hold requested acc
        const double requiredRetroThLevel = (-targetThrustRequired) / maxRetroThrust;
        double retroThLevel = requiredRetroThLevel;
        if (retroThLevel < 0)
            retroThLevel = 0;
        else if (retroThLevel > 1.0)
            retroThLevel = 1;
        
        // NOTE: retros only fire if dynamic pressre < 5 kPa
        const double dynamicPressure = GetVessel().GetDynPressure() / 1000;  // convert to kPa
        if (dynamicPressure > 5.0)
            retroThLevel = 0;       // do not fire the retros
        
        if  (retroThLevel > 0)     // let's use the retros to reach (lower) target velocity
        {
            if (GetXR1().rcover_status != DOOR_OPEN)
            {
                // cannot fire retros because doors as closed!
                GetXR1().ShowWarning("WARNING retro doors are closed.wav", DeltaGliderXR1::ST_WarningCallout, "AIRSPEED HOLD WARNING:&Open the retro doors!");
                retroThLevel = 0;       // cannot use retros
            }
        }
#else
        const double retroThLevel = 0;  // not used for now
#endif
        // set main and retro thrust
        for (int i = 0; i < 2; i++)
        {
            GetVessel().SetThrusterLevel(GetXR1().th_main[i], mainThLevel);
            GetVessel().SetThrusterLevel(GetXR1().th_retro[i], retroThLevel);
        }

        /* DEBUG
        sprintf(oapiDebugString(), "velDelta=%f, targetAcc=%f, VAcc=%f, effectiveTargetAcc=%f, targetThrustRequired=%f, mainThLevel=%lf, retroThLevel=%lf", 
            velDelta, targetAcc, acc, effectiveTargetAcc, (targetThrustRequired / 1000), mainThLevel, retroThLevel);
        */
    }

    // remember the airspeed hold status for the next timestep
    m_prevAirspeedHold = (GetXR1().m_airspeedHoldEngaged ? PAH_ON : PAH_OFF);
}

//---------------------------------------------------------------------------

MmuPreStep::MmuPreStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel)
{
}

void MmuPreStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
#ifdef MMU
    // set the airlock status so the MMU knows about it; NOTE: only check the OUTER door here because we are likely opening to vacuum!
    // No need to check for a full ship here, since UMmu handles that for us.
    bool airlockOpen = (*GetXR1().m_pActiveAirlockDoorStatus == DOOR_OPEN);
    GetXR1().UMmu.SetAirlockDoorState(airlockOpen);

    // NOTE: MMU crew member will only reenter the ship if airlockOpen == true, so no need to check it here
    int mmuReenteredShip = GetXR1().UMmu.ProcessUniversalMMu();	// return 0 if no Mmu else another number if one Mmu just reentered the ship
    if ((mmuReenteredShip == UMMU_TRANSFERED_TO_OUR_SHIP) || (mmuReenteredShip == UMMU_RETURNED_TO_OUR_SHIP))
    {
        const char *pName = CONST_UMMU(&GetXR1()).GetLastEnteredCrewName();

        // EVA reentry successful!  Show a message.
        char msg[120];
        sprintf(msg, "Crew member '%s'&ingressed successfully!", pName);
        GetXR1().ShowInfo("Ingress Successful.wav", DeltaGliderXR1::ST_InformationCallout, msg);

        // force the inner airlock door CLOSED if the O2 levels are 0.0 in case some dumb-dumb killed the crew by decompressing the ship earlier...
        // NOTE: O2 levels will be fixed automatically below
        if (GetXR1().m_cabinO2Level == 0.0)
        {
            // ignore APU checks for these; these need to happen instantly!
            GetXR1().ForceActivateInnerAirlock(DOOR_CLOSED);     
            GetXR1().ForceActivateCabinHatch(DOOR_CLOSED);     
        }

        // NOTE: if the crew is currently DEAD or INCAPACITATED, it means that we now have a crew on board again! 
        // NOTE: if any crew member DIED we already removed him from the ship, so it's OK to revive any incapacitated crew members here
        GetXR1().m_crewState = OK;  // fix INCAPACITATED, since we'll assume the new guy revived the incapacitated members
        GetXR1().m_cabinO2Level = NORMAL_O2_LEVEL;  // he fixed the O2 levels

        // remove any CREW DEAD message from the HUD; must clear both the PERSISTED msg (crashMessage) and CURRENT (non-persisted) message
        *GetXR1().m_crashMessage = *GetXR1().m_hudWarningText = 0;

        // update crew display to show the new member
        GetXR1().m_crewDisplayIndex = GetXR1().GetUMmuSlotNumberForName(pName);
        GetXR1().TriggerRedrawArea(AID_CREW_DISPLAY);

        // update passenger visuals since we just gained a new crew member
        GetXR1().SetPassengerVisuals();
    }
#endif
}

//---------------------------------------------------------------------------

TakeoffAndLandingCalloutsAndCrashPreStep::TakeoffAndLandingCalloutsAndCrashPreStep(DeltaGliderXR1 &vessel) : 
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

            if (GetXR1().gear_status == DOOR_FAILED)
            {
                GetXR1().DoGearCollapse("Belly landing due to&failed landing gear!", touchdownVerticalSpeed, false);  // do not move the landing gear animation
                // Jump to "reset for ground mode" code, since the ship is not crashed -- otherwise, the next timestep through
                // here will cause a full crash to occur since m_takeoffTime is still > 0.
                goto resetForGroundMode;
            }

            // NOTE: must check gear DOOR status because we partially raise it when a crash occurs
            // check if gear is down
            if (GetXR1().gear_status != DOOR_OPEN)
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
                GetXR1().DoGearCollapse(NULL, touchdownVerticalSpeed, true);  // use default message here
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
                const char *pCalloutFilename = GetXR1().GetXR1Config()->TouchdownCallout;  // may be empty
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
            for (int i=0; i < 2; i++)
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
            if ((GetXR1().m_touchdownTime > 0) && (GetXR1().gear_status == DOOR_OPEN))  // did we just land and gear still intact?
            {
                GetXR1().ShowInfo("Wheel Stop.wav", DeltaGliderXR1::ST_InformationCallout, "Wheel Stop.");
            }

            GetXR1().StopSound(DeltaGliderXR1::TiresRolling);
            GetXR1().m_takeoffTime = GetXR1().m_touchdownTime = 0;
        }
        else if (GetXR1().m_takeoffTime == 0)  // we're taking off or landing!  Let's check the speed.
        {
            // New for XRSound version: play tires rolling sound
            if (GetXR1().gear_status == DOOR_OPEN)
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
                    const char *pCalloutFilename = GetXR1().GetXR1Config()->LiftoffCallout;  // may be empty
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

GearCalloutsPreStep::GearCalloutsPreStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel),
    m_previousGearStatus(-1)
{
}

void GearCalloutsPreStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard() || (GetXR1().gear_status == DOOR_FAILED))
        return;     // no callouts if crashed or gear failed

    int gearStatus = GetXR1().gear_status;

    // reset APU idle timer if the gear is in motion
    if ((gearStatus == DOOR_OPENING) || (gearStatus == DOOR_CLOSING))
        GetXR1().MarkAPUActive();  // reset the APU idle warning callout time

    // skip the first frame through here so we can initialize the previous gear status properly
    if (m_previousGearStatus >= 0)
    {
        if (gearStatus != m_previousGearStatus)
        {
            // gear changed state
            if ((gearStatus == DOOR_OPEN) || (gearStatus == DOOR_CLOSED) || (gearStatus == DOOR_FAILED))
            {
                GetXR1().StopSound(GetXR1().GearWhine);
                 if (gearStatus != DOOR_FAILED)
                 {
                     const bool isGearUp = (gearStatus == DOOR_CLOSED);
                     GetXR1().PlayGearLockedSound(isGearUp);  // gear is up if door closed
                     GetXR1().PlaySound(GetXR1().GearLockedThump, DeltaGliderXR1::ST_Other);
                     GetXR1().ShowInfo(NULL, DeltaGliderXR1::ST_None, (isGearUp ? "Gear doors closed and locked." : "Gear down and locked."));
                 }
            }
            else if (gearStatus == DOOR_OPENING)
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

MachCalloutsPreStep::MachCalloutsPreStep(DeltaGliderXR1 &vessel) : 
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
                if ( ((m_previousMach < m) && (mach >= m)) ||  // acceleration
                     ((m_previousMach > m) && (mach <= m)) )   // deceleration
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

void MachCalloutsPreStep::PlayMach(const double simt, const char *pFilename)
{
    m_nextMinimumCalloutTime = simt + 1;    // reset timer

    // allow normal ATC chatter to continue; mach callouts are not that important
    // also, we don't want this to actually fade, so we don't keep re-sending it
    GetXR1().LoadXR1Sound(GetXR1().MachCallout, pFilename, XRSound::PlaybackType::Radio);  
    GetXR1().PlaySound(GetXR1().MachCallout, DeltaGliderXR1::ST_VelocityCallout);
}

//---------------------------------------------------------------------------

AltitudeCalloutsPreStep::AltitudeCalloutsPreStep(DeltaGliderXR1 &vessel) : 
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
    if ((altitude < GetXR1().m_preStepPreviousGearFullyUncompressedAltitude) && (altitude < 275) && (GetXR1().gear_status != DOOR_OPEN) && 
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
                for (int i=0; i < (sizeof(altitudeCallouts) / sizeof(double)); i++)
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

void AltitudeCalloutsPreStep::PlayAltitude(const double simt, const char *pFilename)
{
    m_nextMinimumCalloutTime = simt + 1;    // reset timer

    GetXR1().LoadXR1Sound(GetXR1().AltitudeCallout, pFilename, XRSound::PlaybackType::Radio);  // audible outside vessel as well
    GetXR1().PlaySound(GetXR1().AltitudeCallout, DeltaGliderXR1::ST_AltitudeCallout);
}

//---------------------------------------------------------------------------

// Note: this prestep also handles prevention of docking if nosecone is closed
DockingCalloutsPreStep::DockingCalloutsPreStep(DeltaGliderXR1 &vessel) : 
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
    const bool bDockingThumpEnabled = (GetXR1().nose_status == DOOR_OPEN);
    GetXR1().XRSoundOnOff(XRSound::Docking, bDockingThumpEnabled);

    // if the ship is marked as DOCKED by Orbiter but the nose is not open, UNDOCK IT
    if ((GetXR1().GetFlightStatus() & 0x2) && (GetXR1().nose_status != DOOR_OPEN)) 
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
        if ((distance < 100) && (closingRate >= 0.02) && ((GetXR1().nose_status != DOOR_OPEN) && (GetXR1().nose_status != DOOR_OPENING)))
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
                for (int i=0; i < (sizeof(distanceCallouts) / sizeof(double)); i++)
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

void DockingCalloutsPreStep::PlayDistance(const double simt, const char *pFilename)
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
    for (int i=0; i < 4; i++)
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

//---------------------------------------------------------------------------

// Update any data values from this frame that we want to preserve for the NEXT frame
// This must be invoked LAST in the PreStep order; also, we cannot access these fields from a *PostStep* because the state has changed across the call
UpdatePreviousFieldsPreStep::UpdatePreviousFieldsPreStep(DeltaGliderXR1 &vessel) :
    XR1PrePostStep(vessel)
{
}

void UpdatePreviousFieldsPreStep::clbkPrePostStep(const double simt, const double simdt, const double mjd) 
{
    GetXR1().m_preStepPreviousGearFullyUncompressedAltitude = GetXR1().GetGearFullyUncompressedAltitude();   // adjust for gear down and/or GroundContact
    GetXR1().m_preStepPreviousAirspeed = GetXR1().GetAirspeed();   // this is used for airspeed callouts during takeoff & landing

    VECTOR3 asVector;
	GetXR1().GetAirspeedVector(FRAME_HORIZON, asVector);
    GetXR1().m_preStepPreviousVerticalSpeed = asVector.y;
}

//---------------------------------------------------------------------------

// Update vessel spotlight levels
UpdateVesselLightsPreStep::UpdateVesselLightsPreStep(DeltaGliderXR1 &vessel) :
    XR1PrePostStep(vessel)
{
}

void UpdateVesselLightsPreStep::clbkPrePostStep(const double simt, const double simdt, const double mjd) 
{
    // Keep the main and hover thruster variable levels in sync with the actual thrust levels;
    // the Orbiter core will automatically do the rest by varying the ligt intensity to match.
    GetXR1().m_mainThrusterLightLevel = GetVessel().GetThrusterGroupLevel(THGROUP_MAIN);
    GetXR1().m_hoverThrusterLightLevel = GetVessel().GetThrusterGroupLevel(THGROUP_HOVER);
}

//-------------------------------------------------------------------------

// Enable/disable nosewheel steering based on APU status.
// This does NOT handle any animation.
// Also fixes poor ground turning performance by "cheating" and rotating the ship based on
// wheel deflection.  Based on code here: http://orbiter-forum.com/showthread.php?t=8392
NosewheelSteeringPreStep::NosewheelSteeringPreStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel)
{
}

void NosewheelSteeringPreStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    if (GetXR1().IsCrashed())  
    {
        GetVessel().SetNosewheelSteering(false);
        return;     // nothing more to do
    }

    bool bSteeringEnabled = false;

    // gear must be operational and DOWN AND LOCKED for steering to be active
    if (GetXR1().gear_status == DOOR_OPEN)
    {
        // check for ground contact and APU power
        if (GetVessel().GroundContact() && GetXR1().CheckHydraulicPressure(false, false))   // do not play a message or beep here: this is invoked each timestep
        {
            bSteeringEnabled = true;  // steering OK
        }
    }
    
    GetVessel().SetNosewheelSteering(bSteeringEnabled);
    GetXR1().AmplifyNosewheelSteering();  // rotate the ship to fix poor nosewheel steering performance inherent in all Orbiter vessels by default
}

//---------------------------------------------------------------------------

ScramjetSoundPreStep::ScramjetSoundPreStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel)
{
}

void ScramjetSoundPreStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // NOTE: engine sound is tied to thrust *produced*, not thrust *level*
    // The easiest way to check this is the flow rate
    double flowLeft  = GetXR1().ramjet->DMF(0);
    double flowRight = GetXR1().ramjet->DMF(1);
    double maxTotalFlow = GetXR1().GetXR1Config()->GetScramMaxDMF() * 2;
    double totalFlow = flowLeft + flowRight;
    double flowLevel = totalFlow / maxTotalFlow;    // 0...1

    // raise volume level earlier here b/c scram is too quiet at normal flow rates
    // flow should be from 127 (idle) to 255
    // OLD (before XRSound): int volume = 170 + static_cast<int>(flowLevel * 75);
    int volume = 127 + static_cast<int>(flowLevel * 128);
    
    // DEV DEBUGGING ONLY: sprintf(oapiDebugString(), "ScramjetSoundPreStep: flowLevel=%lf, volume=%d", flowLevel, volume);
    if (flowLevel == 0)
    {
        GetXR1().StopSound(GetXR1().ScramJet);  // no thrust
    }
    else  // flow > 0; play sound if not already started and/or set the volume level
    {
        // OK if sound already playing here
        GetXR1().PlaySound(GetXR1().ScramJet, DeltaGliderXR1::ST_Other, volume, true);  // loop forever
    }
}

//---------------------------------------------------------------------------

// Drain payload bay tanks to keep *main* tanks full.  This only affects main and SCRAM fuel tanks.
// This should be invoked FIRST in the PreStep order to ensure that the internal tanks stay full across the timestep.
DrainBayFuelTanksPreStep::DrainBayFuelTanksPreStep(DeltaGliderXR1 &vessel) :
    XR1PrePostStep(vessel)
{
}

void DrainBayFuelTanksPreStep::clbkPrePostStep(const double simt, const double simdt, const double mjd) 
{
    // sanity check, although this prestep should never be added to a vessel that does not have a payload bay
    if (GetXR1().m_pPayloadBay == nullptr)
        return; 

    // we track the amount of fuel flowed from bay -> main so that PreventAutoRefuelPostStep will know whether the fuel change came from us 
    GetXR1().m_MainFuelFlowedFromBayToMainThisTimestep = FlowBayFuel(GetXR1().ph_main, false);
    GetXR1().m_SCRAMFuelFlowedFromBayToMainThisTimestep = FlowBayFuel(GetXR1().ph_scram, GetXR1().m_SCRAMTankHidden);
    // Note: RCS internal tank is always standalone, and LOX is flowed manually separately
}

// flow fuel from the bay to the internal tank if possible
// isTankHidden: if true, empty the internal tank if there is no fuel in the bay; otherwise, flow normally.  false = flow normally
// Returns: amount of bay fuel flowed to main tank (in kg)
double DrainBayFuelTanksPreStep::FlowBayFuel(const PROPELLANT_HANDLE ph, const bool isTankHidden)
{
    // check whether the internal fuel tanks are less than full
    const PROP_TYPE pt = GetXR1().GetPropTypeForHandle(ph);
    double internalTankQty = GetXR1().GetPropellantMass(ph);
    const double maxInternalTankQty = GetXR1().GetPropellantMaxMass(ph);

    // If this tank is hidden, it should be EMPTY unless there is actually FUEL in the bay.
    // This is so that the engines will immediately stop when the bay tank empties or is jettesoned.
    // Granted, when the bay tanks runs out this will cause the last few kg of fuel to vanish
    // before being burned, but that's OK we can consider that last bit of fuel as being 
    // "stuck in the lines due to low fuel pressure" or something, so the engines shut down 
    // and the ship renders the fuel gauge as zero at that point.
    //
    // NOTE: we must ignore this check if refueling or cross-feeding is in progress: if there is no fuel tank in the bay
    // and RequirePayloadBayFuelTanks=0 or 1, the internal tank needs to fill in order for the refueling to stop.
    if ((!GetXR1().IsRefuelingOrCrossfeeding()) && isTankHidden && (GetXR1().m_pPayloadBay->GetPropellantMass(pt) <= 0))  // < 0 for sanity check
    {
        GetXR1().SetPropellantMass(ph, 0);  // no bay fuel: hidden internal tank is empty as well
        return 0;
    }

    const double requestedFlowQty = maxInternalTankQty - internalTankQty;
    _ASSERTE(requestedFlowQty >= 0);  // should never flowing in the other direction here!
    double fuelFlowedToMainTank = 0;
    if (requestedFlowQty > 0)
    {
        // internal tank is < 100% full; let's see if we can flow from the bay to the internal tank to fill it up
        // Note: flowFromBay may be zero here if bay tanks are empty
        const double flowFromBay = -GetXR1().AdjustBayPropellantMassWithMessages(pt, -requestedFlowQty);  // flow is negative, so negate it
        internalTankQty += flowFromBay;  // add to main internal tank
        fuelFlowedToMainTank = flowFromBay;    
        GetXR1().SetPropellantMass(ph, internalTankQty);  // ...and store the new quantity
    }
    return fuelFlowedToMainTank;
}

//---------------------------------------------------------------------------

RefreshSlotStatesPreStep::RefreshSlotStatesPreStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel), m_nextRefreshSimt(0)
{
}

// Rescan for bay slot changes once every second so we can detect and handle when some other vessel removes payload from our payload bay
// (forced detachment).  Otherwise the ship would think that an adjacent payload slot for a multi-slot payload would still be in use even 
// though the Orbiter core force-detached it (e.g., from a payload crane vessel).
void RefreshSlotStatesPreStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    if (simt >= m_nextRefreshSimt)
    {
        // time for a rescan
        GetXR1().m_pPayloadBay->RefreshSlotStates();

        // RefreshSlotStates is relatively expensive, so schedule next scan for one second from now
        m_nextRefreshSimt = simt + 1.0;
    }
}

//---------------------------------------------------------------------------

// Apply the parking brakes if they are set
ParkingBrakePreStep::ParkingBrakePreStep(DeltaGliderXR1 &vessel) :
XR1PrePostStep(vessel)
{
}

void ParkingBrakePreStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
	if (GetXR1().GetXR1Config()->EnableParkingBrakes)    // parking brake functionality enabled?
	{
		//
		// Set or unset the parking brakes
		//
		
		// engage the parking brakes if ship is at wheel-stop AND no thrust is applied AND (if the parking brakes are not already engagged) the APU is online
		// Note: the parking brakes do not require APU power once they are set.
		if (GetXR1().IsLanded() && ((GetXR1().apu_status == DOOR_OPEN) || GetXR1().m_parkingBrakesEngaged) &&
			!GetXR1().MainThrustApplied() && 
			!GetXR1().HoverThrustApplied() &&
			!GetXR1().RetroThrustApplied() &&
			!GetXR1().ScramThrustApplied() &&
			!GetXR1().RCSThrustApplied())
		{
			// sprintf(oapiDebugString(), "ParkingBrakePreStep: ship is landed and no thrust is applied and APU is online; setting parking brake");
			GetXR1().m_parkingBrakesEngaged = true;
		}
		// Note: because of an Orbiter 2016 core anomaly (or feature?) the ship can lose GroundContact and/or have spurious groundspeed on startup, so we give the ship 2 seconds to settle down first.
		else if (simt >= STARTUP_DELAY_BEFORE_ISLANDED_VALID)
		{
			// sprintf(oapiDebugString(), "ParkingBrakePreStep: ship is NOT landed OR thrust was applied OR APU was offline; UNSETTING parking brake");
			GetXR1().m_parkingBrakesEngaged = false;
		}

		// apply the parking brakes if set: this means the ship has reached (effective) wheel-stop
		if (GetXR1().m_parkingBrakesEngaged)
		{
			// apply brakes for this timestep only
			GetXR1().SetWheelbrakeLevel(1.0, 0, false);	

			///////////////////////////////////////////////////////////////
			// THIS IS A HACK WITHIN-A-HACK TO WORK AROUND AN ORBITER 2016 CORE BUG WHERE THE WHEEL BRAKES CANNOT STOP A VESSEL ON UNEVEN TERRAIN
			// TODO: COMMENT-OUT THIS HACK WHEN IT IS NO LONGER NEEDED
			// cheat and stop the vessel from moving
			VESSELSTATUS2 status;
			GetXR1().GetStatusSafe(status);
			if (status.status != 1)	// not already landed?
			{
				status.status = 1;		// hack #1: force LANDED to stop all motion

				if (oapiGetOrbiterVersion() < 160903)  // hack #2: work around Orbiter core bug with DefStateEx causing uncontrollable spins while landed.  This was fixed in Orbiter version 160903.
				{
					char planetName[256];
					oapiGetObjectName(GetXR1().GetSurfaceRef(), planetName, sizeof(planetName));  // "Earth", "Mars", etc.
					
					char landedStr[256];
					sprintf(landedStr, "Landed %s", planetName);  // "Landed Earth"
					
					const char *pVesselNameInScenario = GetXR1().GetName();	// "XR2-01", etc.

					CString filename;
					filename.Format("%s_temp", pVesselNameInScenario);   // file will be created in $ORBITER_HOME\config

					FILEHANDLE fh = oapiOpenFile(static_cast<const char *>(filename), FILE_OUT, CONFIG);
					oapiWriteScenario_string(fh, "STATUS", landedStr);
					oapiWriteScenario_float(fh, "HEADING", status.surf_hdg * DEG);

					char gearParams[256];   
					sprintf(gearParams, "%d %f", GetXR1().gear_status, GetXR1().gear_proc);   // must write out the landing gear status, too, or the Orbiter core will raise the landing gear on calling scenario load
					oapiWriteScenario_string(fh, "GEAR", gearParams);

					char position[256];
					sprintf(position, "%.20f %.20f", status.surf_lng * DEG, status.surf_lat * DEG);
					oapiWriteScenario_string(fh, "POS", position);
					oapiCloseFile(fh, FILE_OUT);
					fh = oapiOpenFile(static_cast<const char *>(filename), FILE_IN, CONFIG);
					GetXR1().clbkLoadStateEx(fh, &status);
					oapiCloseFile(fh, FILE_IN);
				}
				GetXR1().DefSetStateEx(&status);
			}
			
#if 0
			// section for debugging only
			VECTOR3 rVelToSurface;
			GetXR1().GetGroundspeedVector(FRAME_HORIZON, rVelToSurface);
			sprintf(oapiDebugString(), "ParkingBrakePreStep: rVelToSurface x=%lf, y=%lf, z=%lf, status=%d", rVelToSurface.x, rVelToSurface.y, rVelToSurface.z, status.status);
#endif
			///////////////////////////////////////////////////////////////

		}
		// else no brake override is applied, so normal Orbiter core wheelbrake keys apply for this timestep
	}
}
