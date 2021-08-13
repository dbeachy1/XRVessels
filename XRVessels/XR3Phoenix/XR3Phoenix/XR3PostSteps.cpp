/**
  XR Vessel add-ons for OpenOrbiter Space Flight Simulator
  Copyright (C) 2006-2021 Douglas Beachy

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

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
// XR3PostSteps.cpp
// Class defining custom clbkPostStep callbacks for the XR3 Phoenix
// ==============================================================

#include "XR3PayloadBay.h"
#include "XR3PostSteps.h"
#include "XR3AreaIDs.h"
#include "XR3Areas.h"
#include "XRPayload.h"

//---------------------------------------------------------------------------

XR3AnimationPostStep::XR3AnimationPostStep(XR3Phoenix &vessel) : 
    XR3PrePostStep(vessel)
{
}

void XR3AnimationPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // animate doors that require hydraulic pressure
    if (GetXR3().CheckHydraulicPressure(false, false))     // do not log a warning nor play an error beep here!  We are merely querying the state.
    {
        AnimateBayDoors(simt, simdt, mjd);
        AnimateElevator(simt, simdt, mjd);
    }
}

void XR3AnimationPostStep::AnimateBayDoors(const double simt, const double simdt, const double mjd)
{
    // animate the payload bay doors
    if (GetXR3().bay_status >= DOOR_CLOSING)  // closing or opening
    {
        double da = simdt * BAY_OPERATING_SPEED;
        if (GetXR3().bay_status == DOOR_CLOSING)
        {
            if (GetXR3().bay_proc > 0.0)
                GetXR3().bay_proc = max(0.0, GetXR3().bay_proc - da);
            else
            {
                GetXR3().bay_status = DOOR_CLOSED;
                GetVessel().TriggerRedrawArea(AID_BAYDOORSINDICATOR);
            }
        } 
        else    // door is opening or open
        {
            if (GetXR3().bay_proc < 1.0)
                GetXR3().bay_proc = min (1.0, GetXR3().bay_proc + da);
            else
            {
                GetXR3().bay_status = DOOR_OPEN;
                GetVessel().TriggerRedrawArea(AID_BAYDOORSINDICATOR);
            }
        }
        GetXR3().SetXRAnimation(GetXR3().anim_bay, GetXR3().bay_proc);
    }
}

void XR3AnimationPostStep::AnimateElevator(const double simt, const double simdt, const double mjd)
{
    // animate the elevator
    if (GetXR3().crewElevator_status >= DOOR_CLOSING)  // closing or opening
    {
        double da = simdt * ELEVATOR_OPERATING_SPEED;
        if (GetXR3().crewElevator_status == DOOR_CLOSING)
        {
            if (GetXR3().crewElevator_proc > 0.0)
                GetXR3().crewElevator_proc = max(0.0, GetXR3().crewElevator_proc - da);
            else
            {
                GetXR3().crewElevator_status = DOOR_CLOSED;
                GetVessel().TriggerRedrawArea(AID_ELEVATORINDICATOR);
            }
        } 
        else    // door is opening or open
        {
            if (GetXR3().crewElevator_proc < 1.0)
                GetXR3().crewElevator_proc = min (1.0, GetXR3().crewElevator_proc + da);
            else
            {
                GetXR3().crewElevator_status = DOOR_OPEN;
                GetVessel().TriggerRedrawArea(AID_ELEVATORINDICATOR);
            }
        }
        GetXR3().SetXRAnimation(GetXR3().anim_crewElevator, GetXR3().crewElevator_proc);
    }
}

//---------------------------------------------------------------------------

XR3DoorSoundsPostStep::XR3DoorSoundsPostStep(XR3Phoenix &vessel) : 
    DoorSoundsPostStep(vessel)
{
// set transition state processing to FALSE so we don't play an initial thump when a scenario loads
#define INIT_DOORSOUND(idx, doorStatus, xr1SoundID, label)      \
    m_XR3doorSounds[idx].pDoorStatus = &(GetXR3().doorStatus);  \
    m_XR3doorSounds[idx].prevDoorStatus = NOT_SET;              \
    m_XR3doorSounds[idx].soundID = GetXR1().xr1SoundID;         \
    m_XR3doorSounds[idx].processAPUTransitionState = false;     \
    m_XR3doorSounds[idx].pLabel = label
    
    // initialize door sound structures for all new doors
    INIT_DOORSOUND(0,  bay_status,          dPayloadBayDoors, "Bay Doors");
    INIT_DOORSOUND(1,  crewElevator_status, dElevator,        "Elevator");
}

void XR3DoorSoundsPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // call the superclass to handle all the normal doors
    DoorSoundsPostStep::clbkPrePostStep(simt, simdt, mjd);

    // handle all our custom door sounds
    const int doorCount = (sizeof(m_XR3doorSounds) / sizeof(DoorSound));
    for (int i=0; i < doorCount; i++)
        PlayDoorSound(m_XR3doorSounds[i], simt);
}

//---------------------------------------------------------------------------

// Detect docking status changes and force active airlock as necessary; this is required
// because Mmu assumes that each time you are docked you are transferring crew via the airlock.
HandleDockChangesForActiveAirlockPostStep::HandleDockChangesForActiveAirlockPostStep(XR3Phoenix &vessel) : 
    XR3PrePostStep(vessel),
    m_wasDockedAtPreviousTimestep(false)
{
}

void HandleDockChangesForActiveAirlockPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    if (GetXR3().IsCrashed())
        return;     // nothing to do

    const bool isDocked = GetXR3().IsDocked();
    if (isDocked && (!m_wasDockedAtPreviousTimestep))
        GetXR3().DefineMmuAirlock();       // we are docked, so lock the active airlock to be the docking port

    m_wasDockedAtPreviousTimestep = isDocked;  
}
