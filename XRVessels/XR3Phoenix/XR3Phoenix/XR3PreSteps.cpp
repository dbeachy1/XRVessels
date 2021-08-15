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
// XR3Phoenix implementation class
//
// Copyright 2007-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR3PreSteps.cpp
// Class defining custom clbkPreStep callbacks for the XR3 Phoenix
// ==============================================================

#include "XR3PreSteps.h"
#include <math.h>

#ifdef UNUSED
// modifies the wings' center of lift to assist in landing; in essence, this acts like automatic flaps
SetCenterOfLiftPreStep::SetCenterOfLiftPreStep(XR3Phoenix &vessel) : 
    XR3PrePostStep(vessel)
{
}

void SetCenterOfLiftPreStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // Note: this works even if the ship is crashed (to maintain consistency w/non-crashed behavior), so we 
    // don't check for isCrashed here.

    // only function if in an atmosphere
    const double pressure = GetVessel().GetAtmPressure() / 1000; // in kPa
    if (pressure < 1.0e-6)
        return;     // no atm to speak of

    // check whether ATTITUDE HOLD is engaged; if so, don't shift the center of lift, since the autopilot has priority
    if (GetXR3().m_customAutopilotMode == AP_ATTITUDEHOLD)
        return;     // don't change anything

    // don't do anything unless AUTO COG mode or RECENTER mode is engaged
    if ((GetXR3().m_cogShiftAutoModeActive == false) || GetXR3().m_cogShiftCenterModeActive)
        return;     // don't change anything

    // Requires APU to operate; however, since we are flying in the atmosphere it is highly likely the ship is running with
    // the APU.  If not, the elevators will not operate, and so the lack of COG shift functionality is the least of the pilot's problems...
    // Also, when the ship flies at a very high altitude a COG shift is of much lesser importance, so we don't want to sound out a warning.
    if (GetXR3().CheckHydraulicPressure(false, false) == false)     // no sounds here -- we get called at each timestep!
        return;

    //
    // We're in Earth-pressure atm and the APU is online, so let's shift the COG if necessary
    //
    const double deploymentSpeedRange = FLAPS_FULLY_RETRACTED_SPEED - FLAPS_FULLY_DEPLOYED_SPEED;

    // get our velocity
    const double airspeed = GetXR3().GetAirspeed();     // in meters-per-second

    // center of lift will vary between LOWSPEED_CENTER_OF_LIFT at fullyDeployedSpeed and HIGHSPEED_CENTER_OF_LIFT at fullyRetractedSpeed
    double movementRange = fabs(HIGHSPEED_CENTER_OF_LIFT - NEUTRAL_CENTER_OF_LIFT);    // in meters

    // get the fraction between fully retracted and fully deployed, based on our current velocity
    const double adjustedAirspeed = (airspeed - FLAPS_FULLY_DEPLOYED_SPEED);    // <=0 means fully deployed

    // now compute what fraction of our deploymentSpeedRange our adjustedAirspeed is
    double deployFrac = 1.0 - (adjustedAirspeed / deploymentSpeedRange);  // <=0 means fully retracted; >= 1 means fully deployed

    // keep in range
    if (deployFrac < 0)
        deployFrac = 0;
    else if (deployFrac > 1.0)
        deployFrac = 1.0;

    // update the new center of lift for the wings
    // NOTE: if this function is ever enabled, do not set the airfoil directly!  Invoke ShiftCenterOfLift(delta) instead.
    // OLD: GetXR3().m_centerOfLift = HIGHSPEED_CENTER_OF_LIFT + (deployFrac * movementRange);  // lift is moved FORWARD (+Z axis) as flaps are DEPLOYED
    // OLD: GetVessel().EditAirfoil(GetXR3().hwing, 0x01, _V(0, 0, GetXR3().m_centerOfLift), NULL, 0, 0, 0);

    sprintf(oapiDebugString(), "airspeed=%lf, deployFrac=%lf, centerOfLift=%lf", airspeed, deployFrac, GetXR3().m_centerOfLift);
}
#endif

//-------------------------------------------------------------------------

// animate the front and rear gear struts for touchdown compression
XR3NosewheelSteeringPreStep::XR3NosewheelSteeringPreStep(XR3Phoenix &vessel) : 
    XR3PrePostStep(vessel),
    m_steeringActiveDuringPrevTimestep(false)
{
}

void XR3NosewheelSteeringPreStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    if (GetXR3().IsCrashed()) 
    {
        GetVessel().SetNosewheelSteering(false);
        return;     // nothing more to do (do not recenter steering either)
    }

    // exit immediately if gear is not down and locked OR if the nosewheel is not touching the ground (i.e., fully uncompressed)
    if ((GetXR3().gear_status != DOOR_OPEN) || (GetXR3().m_noseGearProc == 1.0))
    {
        // reset the steering to centered if we just deactivated nosewheel steering
        // NOTE: we have to do this to ensure that the gear retraction animation works properly!
        if (m_steeringActiveDuringPrevTimestep)
        {
            GetXR3().SetXRAnimation(GetXR3().m_animNosewheelSteering, 0.5);  // center
            m_steeringActiveDuringPrevTimestep = false;     // reset
        }
        return;  
    }

    // if we reach here, OK to have nosewheel steering UNLESS the APU if offline
    if (GetXR3().CheckHydraulicPressure(false, false) == false)      // no sound or message here: this is invoked each timestep
    {
        GetVessel().SetNosewheelSteering(false);
        GetXR3().SetXRAnimation(GetXR3().m_animNosewheelSteering, 0.5);    // recenter since steering is inactive
        return;
    }
    else if (GetVessel().GroundContact())   // do a sanity check for ground contact
    {
        GetVessel().SetNosewheelSteering(true);
    }

    // OK to animate nosewheel steering: nosewheel steering state matches rudder state
    const double animState = 0.5 + (GetVessel().GetControlSurfaceLevel(AIRCTRL_RUDDER) * 0.5);  // 0...1
    GetXR3().SetXRAnimation(GetXR3().m_animNosewheelSteering, animState);
    
    m_steeringActiveDuringPrevTimestep = true;

    GetXR3().AmplifyNosewheelSteering();  // rotate the ship to fix poor nosewheel steering performance inherent in all Orbiter vessels by default
}

