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
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// XR1VCPanelAreas.h
// Handles non-component main panel areas
// ==============================================================

#pragma once

#include "Orbitersdk.h"
#include "vessel3ext.h"
#include "Area.h"
#include "XR1Areas.h"

//----------------------------------------------------------------------------------

class VCHudModeButtonArea : public XR1Area
{
public:
    // no redrawing here, so no meshTextureID required
    VCHudModeButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);  
    virtual void Activate();
    // VC-only: no Redraw2D for this area
    virtual bool Redraw3D(const int event, const SURFHANDLE surf);
    virtual bool ProcessVCMouseEvent(const int event, const VECTOR3 &coords);
};

//----------------------------------------------------------------------------------

class VCAutopilotButtonArea : public XR1Area
{
public:
    // no redrawing here, so no meshTextureID required
    VCAutopilotButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);
    virtual void Activate();
    // VC-only: no Redraw2D for this area
    virtual bool Redraw3D(const int event, const SURFHANDLE surf);
    virtual bool ProcessVCMouseEvent(const int event, const VECTOR3 &coords);
};

//----------------------------------------------------------------------------------

class VCToggleSwitchArea : public XR1Area
{
public:
    VCToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, void (DeltaGliderXR1::*pDoorHandler)(DoorStatus), const DoorStatus activatedStatus);
    virtual void Activate();
    virtual bool ProcessVCMouseEvent(const int event, const VECTOR3 &coords);

protected:
    // data
    DoorStatus m_activatedStatus;  // status to send pDoorHandler when activated
    void (DeltaGliderXR1::*m_pDoorHandler)(DoorStatus);  // handler to process door status for this switch

};

