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
// XR5Vanguard implementation class
//
// XR5PostSteps.cpp
// Class defining custom clbkPostStep callbacks for the XR5 Vanguard
// ==============================================================

#include "XR5PayloadBay.h"
#include "XR5PostSteps.h"
#include "XR5AreaIDs.h"
#include "XR5Areas.h"
#include "XRPayload.h"

//---------------------------------------------------------------------------

XR5AnimationPostStep::XR5AnimationPostStep(XR5Vanguard &vessel) : 
    XR5PrePostStep(vessel)
{
}

void XR5AnimationPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // animate doors that require hydraulic pressure
    if (GetXR5().CheckHydraulicPressure(false, false))     // do not log a warning nor play an error beep here!  We are merely querying the state.
    {
        AnimateBayDoors(simt, simdt, mjd);
        AnimateElevator(simt, simdt, mjd);
    }
}

void XR5AnimationPostStep::AnimateBayDoors(const double simt, const double simdt, const double mjd)
{
    // animate the payload bay doors
    if (GetXR5().bay_status >= DoorStatus::DOOR_CLOSING)  // closing or opening
    {
        double da = simdt * BAY_OPERATING_SPEED;
        if (GetXR5().bay_status == DoorStatus::DOOR_CLOSING)
        {
            if (GetXR5().bay_proc > 0.0)
                GetXR5().bay_proc = max(0.0, GetXR5().bay_proc - da);
            else
            {
                GetXR5().bay_status = DoorStatus::DOOR_CLOSED;
                GetVessel().TriggerRedrawArea(AID_BAYDOORSINDICATOR);
            }
        } 
        else    // door is opening or open
        {
            if (GetXR5().bay_proc < 1.0)
                GetXR5().bay_proc = min (1.0, GetXR5().bay_proc + da);
            else
            {
                GetXR5().bay_status = DoorStatus::DOOR_OPEN;
                GetVessel().TriggerRedrawArea(AID_BAYDOORSINDICATOR);
            }
        }
        GetXR5().SetXRAnimation(GetXR5().anim_bay, GetXR5().bay_proc);
    }
}

void XR5AnimationPostStep::AnimateElevator(const double simt, const double simdt, const double mjd)
{
    // animate the elevator
    if (GetXR5().crewElevator_status >= DoorStatus::DOOR_CLOSING)  // closing or opening
    {
        double da = simdt * ELEVATOR_OPERATING_SPEED;
        if (GetXR5().crewElevator_status == DoorStatus::DOOR_CLOSING)
        {
            if (GetXR5().crewElevator_proc > 0.0)
                GetXR5().crewElevator_proc = max(0.0, GetXR5().crewElevator_proc - da);
            else
            {
                GetXR5().crewElevator_status = DoorStatus::DOOR_CLOSED;
                GetVessel().TriggerRedrawArea(AID_ELEVATORINDICATOR);
            }
        } 
        else    // door is opening or open
        {
            if (GetXR5().crewElevator_proc < 1.0)
                GetXR5().crewElevator_proc = min (1.0, GetXR5().crewElevator_proc + da);
            else
            {
                GetXR5().crewElevator_status = DoorStatus::DOOR_OPEN;
                GetVessel().TriggerRedrawArea(AID_ELEVATORINDICATOR);
            }
        }
        GetXR5().SetXRAnimation(GetXR5().anim_crewElevator, GetXR5().crewElevator_proc);
    }
}

//---------------------------------------------------------------------------

XR5DoorSoundsPostStep::XR5DoorSoundsPostStep(XR5Vanguard &vessel) : 
    DoorSoundsPostStep(vessel)
{
// set transition state processing to FALSE so we don't play an initial thump when a scenario loads
#define INIT_DOORSOUND(idx, doorStatus, xr1SoundID, label)      \
    m_xr5doorSounds[idx].pDoorStatus = &(GetXR5().doorStatus);  \
    m_xr5doorSounds[idx].prevDoorStatus = DoorStatus::NOT_SET;  \
    m_xr5doorSounds[idx].soundID = GetXR1().xr1SoundID;         \
    m_xr5doorSounds[idx].processAPUTransitionState = false;     \
    m_xr5doorSounds[idx].pLabel = label
    
    // initialize door sound structures for all new doors
    INIT_DOORSOUND(0,  bay_status,          dPayloadBayDoors, "Bay Doors");
    INIT_DOORSOUND(1,  crewElevator_status, dElevator,        "Elevator");
}

void XR5DoorSoundsPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // call the superclass to handle all the normal doors
    DoorSoundsPostStep::clbkPrePostStep(simt, simdt, mjd);

    // handle all our custom door sounds
    const int doorCount = (sizeof(m_xr5doorSounds) / sizeof(DoorSound));
    for (int i=0; i < doorCount; i++)
        PlayDoorSound(m_xr5doorSounds[i], simt);
}

//---------------------------------------------------------------------------

// Detect docking status changes and force active airlock as necessary; this is required
// because Mmu assumes that each time you are docked you are transferring crew via the airlock.
HandleDockChangesForActiveAirlockPostStep::HandleDockChangesForActiveAirlockPostStep(XR5Vanguard &vessel) : 
    XR5PrePostStep(vessel),
    m_wasDockedAtPreviousTimestep(false)
{
}

void HandleDockChangesForActiveAirlockPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    if (GetXR5().IsCrashed())
        return;     // nothing to do

    const bool isDocked = GetXR5().IsDocked();
    if (isDocked && (!m_wasDockedAtPreviousTimestep))
        GetXR5().DefineMmuAirlock();       // we are docked, so lock the active airlock to be the docking port

    m_wasDockedAtPreviousTimestep = isDocked;  
}
