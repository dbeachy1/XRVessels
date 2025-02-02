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

// must be included BEFORE XR1Areas.h
#include "DeltaGliderXR1.h"
#include "XR1Areas.h"

//-------------------------------------------------------------------------

// isOn = reference to status variable: true = light on, false = light off
LEDArea::LEDArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID, const bool& isOn) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_isOn(isOn)
{
    m_color = BRIGHT_GREEN; // init here for efficiency
}

void LEDArea::Activate()
{
    Area::Activate();  // invoke superclass method
    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(28, 3), PANEL_REDRAW_USER, PANEL_MOUSE_IGNORE, PANEL_MAP_BACKGROUND);

    TriggerRedraw();    // render initial setting
}

bool LEDArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    if (m_isOn)
    {
        // fill the entire area
        oapiColourFill(surf, m_color);
    }

    return true;    // must always return true so either the background or the fill area is rendered
}

//----------------------------------------------------------------------------------

// isOn = reference to status variable: true = light on, false = light off
DoorMediumLEDArea::DoorMediumLEDArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID, DoorStatus& doorStatus, const bool redrawAlways) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_doorStatus(doorStatus), m_redrawAlways(redrawAlways), m_isOn(false)
{
}

void DoorMediumLEDArea::Activate()
{
    Area::Activate();  // invoke superclass method
    // we redraw the entire texture anyway, so map as PANEL_MAP_NONE
    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(29, 21), (m_redrawAlways ? PANEL_REDRAW_ALWAYS : PANEL_REDRAW_USER), PANEL_MOUSE_IGNORE, PANEL_MAP_NONE);
    m_mainSurface = CreateSurface(IDB_GREEN_LED_SMALL);

    TriggerRedraw();    // render initial setting
}

bool DoorMediumLEDArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    bool isOn = (m_doorStatus == DoorStatus::DOOR_OPEN);
    bool retVal = false;

    // always draw on panel init
    if ((event == PANEL_REDRAW_INIT) || (isOn != m_isOn))
    {
        int srcX = (isOn ? 29 : 0);
        DeltaGliderXR1::SafeBlt(surf, m_mainSurface, 0, 0, srcX, 0, 29, 21);
        m_isOn = isOn;
        retVal = true;
    }

    return retVal;
}
