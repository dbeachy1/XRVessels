// ==============================================================
// XR2Ravenstar implementation class
//
// Copyright 2008-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR2PrePostSteps.cpp
// Class defining custom clbkPreStep callbacks for the XR2 Ravenstar
// ==============================================================

#include "XR2PreSteps.h"
#include <math.h>

//-------------------------------------------------------------------------

/* NO
// animate the front and rear gear struts for touchdown compression
XR2NosewheelSteeringPreStep::XR2NosewheelSteeringPrePostStep(XR2Ravenstar &vessel) : 
    XR2PrePostStep(vessel),
    m_steeringActiveDuringPrevTimestep(false)
{
}

void XR2NosewheelSteeringPreStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    if (GetXR2().IsCrashed()) 
    {
        GetVessel().SetNosewheelSteering(false);
        return;     // nothing more to do (do not recenter steering either)
    }

    // exit immediately if gear is not down and locked OR if the nosewheel is not touching the ground
    if ((GetXR2().gear_status != DOOR_OPEN) || (GetXR2().IsNoseGearOnGround() == false))
    {
        // reset the steering to centered if we just deactivated nosewheel steering
        // NOTE: we have to do this to ensure that the gear retraction animation works properly!
        if (m_steeringActiveDuringPrevTimestep)
        {
            GetXR2().SetXRAnimation(GetXR2().m_animNosewheelSteering, 0.5);  // center
            m_steeringActiveDuringPrevTimestep = false;     // reset
        }
        return;  
    }

    // if we reach here, OK to have nosewheel steering UNLESS the APU if offline
    if (GetXR2().CheckHydraulicPressure(false, false) == false)      // no sound or message here: this is invoked each timestep
    {
        GetVessel().SetNosewheelSteering(false);
        GetXR2().SetXRAnimation(GetXR2().m_animNosewheelSteering, 0.5);    // recenter since steering is inactive
        return;
    }
    else if (GetVessel().GroundContact() && (GetVessel().GetADCtrlMode() & 0x02))   // do a sanity check for ground contact and only enable nosewheel steering if rudder AF Ctrl surface is enabled (since anim tied to rudder)
    {
        GetVessel().SetNosewheelSteering(true);
    }

    // OK to animate nosewheel sttering: nosewheel steering state matches rudder state
    const double animState = 0.5 + (GetVessel().GetControlSurfaceLevel(AIRCTRL_RUDDER) * 0.5);  // 0...1
    GetXR2().SetXRAnimation(GetXR2().m_animNosewheelSteering, animState);
    
    m_steeringActiveDuringPrevTimestep = true;

    GetXR2().AmplifyNosewheelSteering();  // rotate the ship to fix poor nosewheel steering performance inherent in all Orbiter vessels by default
}
*/

