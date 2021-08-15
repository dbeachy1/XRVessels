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
// XR2Areas.cpp
// Contains new areas for the XR2.
// ==============================================================

#include "XR2Ravenstar.h"
#include "AreaIDs.h"
#include "XR2Areas.h"

//----------------------------------------------------------------------------------

// Constructor
XR2ReentryCheckMultiDisplayMode::XR2ReentryCheckMultiDisplayMode(int modeNumber) :
    ReentryCheckMultiDisplayMode(modeNumber)
{
}

// Invoked by our parent's AddDisplayMode method immediately after we are attached to our parent MDA.
// This is useful if an MDA needs to perform some one-time initialization.
void XR2ReentryCheckMultiDisplayMode::OnParentAttach()
{
    // allocate the door array and add all the standard doors
    ReentryCheckMultiDisplayMode::OnParentAttach();

    // now add our custom doors beginning @ door index 6
    const int cx = GetCloseButtonXCoord();
    int cy = GetStartingCloseButtonYCoord() + (6 * GetLinePitch());
    m_pDoorInfo[6] = new DoorInfo("OPEN", "CLOSED", GetXR2().bay_status, _COORD2(cx, cy), (void (DeltaGliderXR1::*)(DoorStatus))(&XR2Ravenstar::ActivateBayDoors));
}

//----------------------------------------------------------------------------------

// Override XR1 crew display panel showing crew members; also handles EVA requests
XR2CrewDisplayArea::XR2CrewDisplayArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    CrewDisplayArea(parentPanel, panelCoordinates, areaID)
{
    // override X coordinate for the "next" arrow to allow for 2-digit-wide crew indexes
    m_nextArrowCoord.x = 181;
}

