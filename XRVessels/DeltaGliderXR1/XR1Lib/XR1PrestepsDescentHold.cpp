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
DescentHoldPreStep::DescentHoldPreStep(DeltaGliderXR1& vessel) :
    XR1PrePostStep(vessel),
    m_prevCustomAutopilotMode(AUTOPILOT::AP_NOTSET)
{
}

void DescentHoldPreStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    if (GetXR1().IsCrashed())  // note: autopilot still works if crew is incapacitated!
        return;     // nothing to do

    // determine maximum hover thrust
    double maxHoverThrust = 0;
    for (int i = 0; i < 2; i++)
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
    if ((customAutopilotMode == AUTOPILOT::AP_DESCENTHOLD) && (m_prevCustomAutopilotMode == AUTOPILOT::AP_NOTSET))
        GetXR1().m_latchedAutoTouchdownMinDescentRate = -3;  // in case we are not set later before auto-land engages
    // check whether the DESCENT HOLD autopilot is engaged AND that we have already set the previous state correctly
    else if ((customAutopilotMode == AUTOPILOT::AP_DESCENTHOLD) && (m_prevCustomAutopilotMode != AUTOPILOT::AP_NOTSET))
    {
        // NOTE: 'suspend autpilot' checks are handled by the Attitude Hold autopilot code, since that is also enabled when we are enabled

        // verify that the hover doors are open
        if (GetXR1().m_isHoverEnabled == false)
        {
            GetXR1().PlaySound(GetXR1().HoverDoorsAreClosed, DeltaGliderXR1::ST_WarningCallout);
            GetXR1().ShowWarning(nullptr, DeltaGliderXR1::ST_None, "WARNING: Hover Doors are closed.");  // NOTE: "descent hold disengaged" will be displayed by SetCustomAutopilot
            GetXR1().SetCustomAutopilotMode(AUTOPILOT::AP_OFF, false);  // do not play sounds for this
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
            GetXR1().SetCustomAutopilotMode(AUTOPILOT::AP_OFF, false);
            goto exit;    // nothing more to do this timestep
        }

        // determine how much margin we have on hover thrust vs. weight
        // NOTE: should not use our m_acceleration data since that was computed in the previous frame's PreStep
        // We would like to make it a PreStep, but testing shows that the acc values kept fluctuating constantly, making the gauges jump.  Orbiter core bug???

        // determine the ship's current acceleration
        VECTOR3 F, D, L;
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

            sprintf(msg, "WARNING: insufficient hover thrust&available to maintain hover!&Ship mass %.0f%% of hover capacity.", massPctCapacity);
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
        if (m_prevCustomAutopilotMode == AUTOPILOT::AP_DESCENTHOLD)
        {
            // kill the hover engines if we just touched down
            if (altitude < 0.5)
            {
                for (int i = 0; i < 2; i++)
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
