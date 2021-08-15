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
// XR2Ravenstar implementation class
//
// XR2PrePostSteps.cpp
// Class defining custom clbkPostStep callbacks for the XR2 Ravenstar
// ==============================================================

#include "XR2PostSteps.h"
#include "XR2AreaIDs.h"
#include "XR2Areas.h"

//---------------------------------------------------------------------------

XR2AnimationPostStep::XR2AnimationPostStep(XR2Ravenstar &vessel) : 
    XR2PrePostStep(vessel)
{
}

void XR2AnimationPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // animate doors that require hydraulic pressure
    if (GetXR2().CheckHydraulicPressure(false, false))     // do not log a warning nor play an error beep here!  We are merely querying the state.
    {
        AnimateBayDoors(simt, simdt, mjd);
    }
}

void XR2AnimationPostStep::AnimateBayDoors(const double simt, const double simdt, const double mjd)
{
    // animate the payload bay doors
    if (GetXR2().bay_status >= DOOR_CLOSING)  // closing or opening
    {
        double da = simdt * BAY_OPERATING_SPEED;
        if (GetXR2().bay_status == DOOR_CLOSING)
        {
            if (GetXR2().bay_proc > 0.0)
                GetXR2().bay_proc = max(0.0, GetXR2().bay_proc - da);
            else
            {
                GetXR2().bay_status = DOOR_CLOSED;
                GetVessel().TriggerRedrawArea(AID_BAYDOORSINDICATOR);
            }
        } 
        else    // door is opening or open
        {
            if (GetXR2().bay_proc < 1.0)
                GetXR2().bay_proc = min (1.0, GetXR2().bay_proc + da);
            else
            {
                GetXR2().bay_status = DOOR_OPEN;
                GetVessel().TriggerRedrawArea(AID_BAYDOORSINDICATOR);
            }
        }
        GetXR2().SetXRAnimation(GetXR2().anim_bay, GetXR2().bay_proc);
    }
}

//---------------------------------------------------------------------------

XR2DoorSoundsPostStep::XR2DoorSoundsPostStep(XR2Ravenstar &vessel) : 
    DoorSoundsPostStep(vessel)
{
// set transition state processing to FALSE so we don't play an initial thump when a scenario loads
#define INIT_DOORSOUND(idx, doorStatus, xr1SoundID, label)      \
    m_doorSounds[idx].pDoorStatus = &(GetXR2().doorStatus);  \
    m_doorSounds[idx].prevDoorStatus = NOT_SET;              \
    m_doorSounds[idx].soundID = GetXR1().xr1SoundID;         \
    m_doorSounds[idx].processAPUTransitionState = false;     \
    m_doorSounds[idx].pLabel = label
    
    // initialize door sound structures for all new doors
    INIT_DOORSOUND(0, bay_status, dPayloadBayDoors, "Bay Doors");
}

void XR2DoorSoundsPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // call the superclass to handle all the normal doors
    DoorSoundsPostStep::clbkPrePostStep(simt, simdt, mjd);

    // handle all our custom door sounds
    const int doorCount = (sizeof(m_doorSounds) / sizeof(DoorSound));
    for (int i=0; i < doorCount; i++)
        PlayDoorSound(m_doorSounds[i], simt);
}
