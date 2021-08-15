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
// XR2Components.cpp
// Custom XR2 components.
// ==============================================================

#include "orbitersdk.h"
#include "resource.h"
#include "XR2Ravenstar.h"

#include "XR1MainPanelAreas.h"  
#include "XR2AreaIDs.h"
#include "XR2InstrumentPanels.h"
#include "XR2Areas.h"
#include "XR2Components.h"

// Constructor
// parentPanel = parent instrument panel
// topLeft = top inside edge of frame
XR2WarningLightsComponent::XR2WarningLightsComponent(InstrumentPanel &parentPanel, COORD2 topLeft) :
    XR1Component(parentPanel, topLeft)
{
    AddArea(new WarningLightsArea   (parentPanel, GetAbsCoords(_COORD2(  1,  1)), AID_WARNING_LIGHTS));
    AddArea(new XR2MWSTestButtonArea(parentPanel, GetAbsCoords(_COORD2(-18, 50)), AID_MWS_TEST_BUTTON));
    AddArea(new XR2WarningLightsArea(parentPanel, GetAbsCoords(_COORD2(-25, 67)), AID_XR2_WARNING_LIGHTS));
}   

//-------------------------------------------------------------------------
//
// Areas begin here
//
//-------------------------------------------------------------------------

XR2MWSTestButtonArea::XR2MWSTestButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    MomentaryButtonArea(parentPanel, panelCoordinates, areaID)
{
}

void XR2MWSTestButtonArea::ProcessButtonAction(int event, double buttonDownSimt)
{
    // process PRESSED and UNPRESSED events
    if (event & (PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBUP))
    {
        GetXR2().PlaySound(GetXR2().SwitchOn, DeltaGliderXR1::ST_Other, QUIET_CLICK);  // light click for both on and off
        GetXR2().m_mwsTestActive = ((event & PANEL_MOUSE_LBDOWN) == true);

        // redraw the MWS light and MWS warning panels
        GetVessel().TriggerRedrawArea(AID_MWS);
        GetVessel().TriggerRedrawArea(AID_WARNING_LIGHTS);
        GetVessel().TriggerRedrawArea(AID_XR2_WARNING_LIGHTS);
        GetVessel().TriggerRedrawArea(AID_APU_BUTTON);
    }

    // ignore PANEL_MOUSE_LBPRESSED events
}

//----------------------------------------------------------------------------------

XR2WarningLightsArea::XR2WarningLightsArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_lightStateOn(false)
{
}

void XR2WarningLightsArea::Activate()
{
    Area::Activate();  // invoke superclass method
    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(26, 11), PANEL_REDRAW_USER, PANEL_MOUSE_IGNORE, PANEL_MAP_BACKGROUND);
    m_mainSurface = CreateSurface(IDB_XR2_WARNING_LIGHTS);
}

bool XR2WarningLightsArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    // if TEST button pressed, all lights stay on regardless
    bool testModeActive = GetXR2().m_mwsTestActive;

    // check each light's status
    for (int i=0; i < XR2_WARNING_LIGHT_COUNT; i++)
    {   
        bool warningActive = GetXR2().m_xr2WarningLights[i];

        // light is ON if 1) test mode, or 2) warning is active and blink state is ON
        if (testModeActive || (warningActive && m_lightStateOn))
        {
            if (m_lightStateOn || testModeActive)
            {
                // render the "lit up" texture
                int x = 0;          // column
                int y = i * 11;     // row 

                oapiBlt(surf, m_mainSurface, x, y, x, y, 26, 11);
            }
        }
    }
        
    // always return 'true' here so we are sure to turn off any now-off-but-previously-lit lights
    return true;
}

void XR2WarningLightsArea::clbkPrePostStep(const double simt, const double simdt, const double mjd) 
{
    double di;
    bool lightStateOn = (modf(simt, &di) < 0.5);   // blink twice a second; NOTE: this must match the XR1's WarningLightsArea time
    if (lightStateOn != m_lightStateOn)  // has state switched?
    {
        // toggle the state and request a repaint
        m_lightStateOn = lightStateOn;
        TriggerRedraw();

        // no sound with these lights
    }
}


//----------------------------------------------------------------------------------
// our custom hull temps multi-display mode

// Constructor
XR2HullTempsMultiDisplayMode::XR2HullTempsMultiDisplayMode(int modeNumber) :
    HullTempsMultiDisplayMode(modeNumber)
{
}

// returns the highest temperature fraction for any surface (0...n).  
double XR2HullTempsMultiDisplayMode::GetHighestTempFrac()
{
    XR2Ravenstar &xr2 = GetXR2();      // for convenience

    const HullTemperatureLimits &limits = xr2.m_hullTemperatureLimits;
    double highestTempFrac = 0.0;    // max percentage of any hull temperature to its limit

    // if a surface's door is open, its limits will be lower
#define IS_DOOR_OPEN(status) (status != DOOR_CLOSED)   // includes DOOR_FAILED
#define LIMITK(limitK, doorStatus)  (IS_DOOR_OPEN(doorStatus) ? limits.doorOpen : limitK)
#define SET_MAX_PCT(tempK, limitK, doorStatus)  { double pct = (tempK / LIMITK(limitK, doorStatus)); if (pct > highestTempFrac) highestTempFrac = pct; }

    // nosecone temp is tied to hover doors, gear, and retro doors use nosecone limit
    SET_MAX_PCT(xr2.m_noseconeTemp,  limits.noseCone, xr2.nose_status);
    SET_MAX_PCT(xr2.m_noseconeTemp,  limits.noseCone, xr2.hoverdoor_status);  
    SET_MAX_PCT(xr2.m_noseconeTemp,  limits.noseCone, xr2.gear_status);  

    // both wing temps are affected by retro doors
    SET_MAX_PCT(xr2.m_leftWingTemp,  limits.wings, xr2.rcover_status);  
    SET_MAX_PCT(xr2.m_rightWingTemp, limits.wings, xr2.rcover_status);  

    // cockpit temp is tied to the crew hatch
    SET_MAX_PCT(xr2.m_cockpitTemp,   limits.cockpit, xr2.hatch_status);

    // top hull temp is tied to radiator and payload bay doors
    SET_MAX_PCT(xr2.m_topHullTemp,   limits.topHull, xr2.radiator_status);
    SET_MAX_PCT(xr2.m_topHullTemp,   limits.topHull, xr2.bay_status);

    return highestTempFrac;
}


// determines which door(s) to use for temperature display warning colors
#define CHECK_AND_RETURN_DOOR(doorStatus) if (doorStatus != DOOR_CLOSED) return doorStatus
DoorStatus XR2HullTempsMultiDisplayMode::GetNoseDoorStatus()
{
    CHECK_AND_RETURN_DOOR(GetXR2().nose_status);
    CHECK_AND_RETURN_DOOR(GetXR2().hoverdoor_status);
    CHECK_AND_RETURN_DOOR(GetXR2().gear_status);

    return DOOR_CLOSED;
}
 
DoorStatus XR2HullTempsMultiDisplayMode::GetLeftWingDoorStatus()
{
    CHECK_AND_RETURN_DOOR(GetXR2().rcover_status);
    return DOOR_CLOSED;  
}
 
DoorStatus XR2HullTempsMultiDisplayMode::GetRightWingDoorStatus()
{
    CHECK_AND_RETURN_DOOR(GetXR2().rcover_status);
    return DOOR_CLOSED;  
}

// base class behavior is fine for GetCockpitDoorStatus (only crew hatch to check)

DoorStatus XR2HullTempsMultiDisplayMode::GetTopHullDoorStatus()
{
    CHECK_AND_RETURN_DOOR(GetXR2().radiator_status);
    CHECK_AND_RETURN_DOOR(GetXR2().bay_status);
    
    return DOOR_CLOSED; 
}

