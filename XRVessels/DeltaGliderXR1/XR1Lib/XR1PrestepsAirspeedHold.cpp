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

#include "DeltaGliderXR1.h"
#include "XR1PreSteps.h"
#include "AreaIDs.h"

//---------------------------------------------------------------------------

// NOTE: requires AttitudeHoldPreStep as well to hold ship level during descent
AirspeedHoldPreStep::AirspeedHoldPreStep(DeltaGliderXR1& vessel) :
    XR1PrePostStep(vessel),
    m_prevAirspeedHold(PREV_AIRSPEED_HOLD::PAH_NOTSET)
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
    for (int i = 0; i < 2; i++)
        maxMainThrust += GetVessel().GetThrusterMax(GetXR1().th_main[i]);  // takes atmospheric pressure into account

    // determine maximum retro engine thrust
    double maxRetroThrust = 0;
    for (int i = 0; i < 2; i++)
        maxRetroThrust += GetVessel().GetThrusterMax(GetXR1().th_retro[i]);  // takes atmospheric pressure into account

    // NOTE: must take ATM lift and drag into account here to reduce error amount, since ATM drag is a large part of the airspeed hold equation
    // Also, unlike DESCENT HOLD, the rate at which we reach our target airspeed is not critical.
    const double mass = GetVessel().GetMass();
    VECTOR3 W, F, T;
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
        VECTOR3 L, D;
        GetVessel().GetLiftVector(L);
        GetVessel().GetDragVector(D);
        negEffectiveShipWeight = W.z + L.z + D.z;  // sum of all lift and drag forces on the ship, EXCLUDING wheel drag, w/o any thrust
    }

    // NOTE: this cannot be const because we may need to reset it to zero later
    double planetAcc = negEffectiveShipWeight / mass;   // planetary acc on ship in m/s/s, including atm drag and lift

    // save maxAcc for use by MDA display area as well, which includes ATM data
    GetXR1().m_maxMainAcc = (maxMainThrust + negEffectiveShipWeight) / mass;  // weight (including drag) is NEGATIVE

    // check whether the AIRSPEED HOLD autopilot is engaged AND that we have already set the previous state correctly
    if ((GetXR1().m_airspeedHoldEngaged) && (m_prevAirspeedHold != PREV_AIRSPEED_HOLD::PAH_NOTSET))
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

        if (retroThLevel > 0)     // let's use the retros to reach (lower) target velocity
        {
            if (GetXR1().rcover_status != DoorStatus::DOOR_OPEN)
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
    m_prevAirspeedHold = (GetXR1().m_airspeedHoldEngaged ? PREV_AIRSPEED_HOLD::PAH_ON : PREV_AIRSPEED_HOLD::PAH_OFF);
}

