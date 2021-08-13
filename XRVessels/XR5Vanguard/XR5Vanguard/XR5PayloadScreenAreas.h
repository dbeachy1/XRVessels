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
// XR5Vanguard implementation class
//
// XR5PayloadScreenAreas.h
// Header for new payload screen areas for the XR5.
// ==============================================================

#pragma once

#include "XR1PayloadScreenAreas.h"
#include "XR5Areas.h"

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
    static const COORD2 &s_blockSize;   // size of each block in pixels
    
    COORD2 m_levelButton; 
    SURFHANDLE m_hSurfaceForLevel[3];   // surface bitmap for each level
};
