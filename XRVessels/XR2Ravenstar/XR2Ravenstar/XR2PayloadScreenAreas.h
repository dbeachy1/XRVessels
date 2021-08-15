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
// XR2PayloadScreenAreas.h
// Header for new payload screen areas for the XR2.
// ==============================================================

#pragma once

#include "XR1PayloadScreenAreas.h"
#include "XR2Areas.h"

//----------------------------------------------------------------------------------

class SelectPayloadSlotArea : public XR1Area
{
public:
    SelectPayloadSlotArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);
    virtual void Activate();
    virtual void Deactivate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);

protected:
    static const COORD2 &s_screenSize;  // size of the screen in pixels
    static const COORD2 &s_blockOneSize;
    static const COORD2 &s_blockTwoAndThreeSize;
    SURFHANDLE m_hSurface;   // surface bitmap for each level
};

