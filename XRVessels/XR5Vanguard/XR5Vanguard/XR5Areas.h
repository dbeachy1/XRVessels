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
// XR5Areas.h
// Header for new areas for the XR5.
// ==============================================================

#pragma once

#include "orbitersdk.h"
#include "XR1Areas.h"
#include "XR1UpperPanelAreas.h"
#include "XR1MultiDisplayArea.h"

class XR5Vanguard;

// define this here so it works in XR1-extended classes as well
// XR5 areas should extend XR1Area
#define GetXR5() (static_cast<XR5Vanguard &>(GetVessel()))

//----------------------------------------------------------------------------------

class RCSDockingModeButtonArea : public XR1Area
{
public:
    RCSDockingModeButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);
};

//----------------------------------------------------------------------------------

class XR5CrewDisplayArea : public CrewDisplayArea
{
public:
    XR5CrewDisplayArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);
};

//----------------------------------------------------------------------------------

class ElevatorToggleSwitchArea : public ToggleSwitchArea
{
public:
    ElevatorToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID);

protected:
    virtual bool isOn();
    virtual bool ProcessSwitchEvent(bool switchIsOn);
};

//----------------------------------------------------------------------------------

class XR5ReentryCheckMultiDisplayMode : public ReentryCheckMultiDisplayMode
{
public:
    // Constructor
    XR5ReentryCheckMultiDisplayMode(int modeNumber);

    // Invoked by our parent's AddDisplayMode method immediately after we are attached to our parent MDA.
    // This is useful if an MDA needs to perform some one-time initialization.
    virtual void OnParentAttach();

protected:
    // subclass hooks
    virtual COORD2 GetStartingCoords()   { return _COORD2(85, 19); }  // text lines rendered here
    virtual COORD2 GetStatusLineCoords() { return _COORD2(80, 99); }  // "Reentry Check: ..."
    virtual int GetStartingCloseButtonYCoord() { return 22; }
    virtual int GetLinePitch() { return 10; }   // pitch between lines in pixels
    virtual int GetDoorCount() { return 8; }    // invoked by OnParentAttach
};
