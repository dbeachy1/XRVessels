/**
  XR Vessel add-ons for OpenOrbiter Space Flight Simulator
  Copyright (C) 2006-2025 Douglas Beachy

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
// XR2Components.h
// XR2Ravenstar components.
// ==============================================================

#pragma once

#include "orbitersdk.h"
#include "XR1Component.h"
#include "XR2Areas.h"
#include "XR1MultiDisplayArea.h"

// 
// Components
//

class XR2WarningLightsComponent : public XR1Component
{
public:
    XR2WarningLightsComponent(InstrumentPanel &instrumentPanel, COORD2 topLeft);
};

//
// Areas begin here
//

//----------------------------------------------------------------------------------

class XR2MWSTestButtonArea : public MomentaryButtonArea
{
public:
    XR2MWSTestButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual void ProcessButtonAction(int event, double buttonDownSimt);
};

//----------------------------------------------------------------------------------

class XR2WarningLightsArea : public XR1Area
{
public:
    XR2WarningLightsArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);

protected:
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

    // state data
    bool m_lightStateOn;  // true if light state (during blink) is ON
};


//----------------------------------------------------------------------------------
// our custom hull temps multi-display mode
class XR2HullTempsMultiDisplayMode : public HullTempsMultiDisplayMode
{
public:
    XR2HullTempsMultiDisplayMode(int modeNumber);

protected:
    virtual double GetHighestTempFrac();

    // if DoorStatus::DOOR_OPEN, temperature values will be displayed in yellow or red correctly since that door is open
    virtual DoorStatus GetNoseDoorStatus();
    virtual DoorStatus GetLeftWingDoorStatus();
    virtual DoorStatus GetRightWingDoorStatus();
    virtual DoorStatus GetTopHullDoorStatus();
};

