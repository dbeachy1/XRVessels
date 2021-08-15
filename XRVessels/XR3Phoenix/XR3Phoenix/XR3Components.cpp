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
// XR3Phoenix implementation class
//
// XR1Components.cpp
// Custom XR3 components.
// ==============================================================

#include "orbitersdk.h"
#include "resource.h"
#include "XR3Phoenix.h"

#include "XR1MainPanelAreas.h"  
#include "XR3AreaIDs.h"
#include "XR3InstrumentPanels.h"
#include "XR3Areas.h"
#include "XR3Components.h"

// Constructor
// parentPanel = parent instrument panel
// topLeft = top inside edge of frame
XR3WarningLightsComponent::XR3WarningLightsComponent(InstrumentPanel &parentPanel, COORD2 topLeft) :
    XR1Component(parentPanel, topLeft)
{
    AddArea(new WarningLightsArea   (parentPanel, GetAbsCoords(_COORD2(  1,  1)), AID_WARNING_LIGHTS));
    AddArea(new XR3MWSTestButtonArea(parentPanel, GetAbsCoords(_COORD2(-18, 40)), AID_MWS_TEST_BUTTON));
    AddArea(new XR3WarningLightsArea(parentPanel, GetAbsCoords(_COORD2(-25, 56)), AID_XR3_WARNING_LIGHTS));
}   

// Constructor
// topLeft = top-left edge of docking port LED trim
XR3ActiveEVAPortComponent::XR3ActiveEVAPortComponent(InstrumentPanel &parentPanel, COORD2 topLeft) :
    XR1Component(parentPanel, topLeft)
{
    AddArea(new DockingPortActiveLEDArea (parentPanel, GetAbsCoords(_COORD2( 0, 0)), AID_EVA_DOCKING_PORT_ACTIVE_LED));
    AddArea(new ActiveEVAPortSwitchArea  (parentPanel, GetAbsCoords(_COORD2(27, 0)), AID_ACTIVE_EVA_PORT_SWITCH));
    AddArea(new CrewElevatorActiveLEDArea(parentPanel, GetAbsCoords(_COORD2(81, 0)), AID_EVA_CREW_ELEVATOR_ACTIVE_LED));
}   

//-------------------------------------------------------------------------
//
// Areas begin here
//
//-------------------------------------------------------------------------

XR3MWSTestButtonArea::XR3MWSTestButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    MomentaryButtonArea(parentPanel, panelCoordinates, areaID)
{
}

void XR3MWSTestButtonArea::ProcessButtonAction(int event, double buttonDownSimt)
{
    // process PRESSED and UNPRESSED events
    if (event & (PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBUP))
    {
        GetXR3().PlaySound(GetXR3().SwitchOn, DeltaGliderXR1::ST_Other, QUIET_CLICK);  // light click for both on and off
        GetXR3().m_mwsTestActive = ((event & PANEL_MOUSE_LBDOWN) == true);

        // redraw the MWS light and MWS warning panels
        GetVessel().TriggerRedrawArea(AID_MWS);
        GetVessel().TriggerRedrawArea(AID_WARNING_LIGHTS);
        GetVessel().TriggerRedrawArea(AID_XR3_WARNING_LIGHTS);
        GetVessel().TriggerRedrawArea(AID_APU_BUTTON);
    }

    // ignore PANEL_MOUSE_LBPRESSED events
}

//----------------------------------------------------------------------------------

XR3WarningLightsArea::XR3WarningLightsArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_lightStateOn(false)
{
}

void XR3WarningLightsArea::Activate()
{
    Area::Activate();  // invoke superclass method
    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(26, 22), PANEL_REDRAW_USER, PANEL_MOUSE_IGNORE, PANEL_MAP_BACKGROUND);
    m_mainSurface = CreateSurface(IDB_XR3_WARNING_LIGHTS);
}

bool XR3WarningLightsArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    // if TEST button pressed, all lights stay on regardless
    bool testModeActive = GetXR3().m_mwsTestActive;

    // check each light's status
    for (int i=0; i < XR3_WARNING_LIGHT_COUNT; i++)
    {   
        bool warningActive = GetXR3().m_XR3WarningLights[i];

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

void XR3WarningLightsArea::clbkPrePostStep(const double simt, const double simdt, const double mjd) 
{
    double di;
    // NOTE: must use fabs simt here since simt may be negative!
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
XR3HullTempsMultiDisplayMode::XR3HullTempsMultiDisplayMode(int modeNumber) :
    HullTempsMultiDisplayMode(modeNumber)
{
}

// returns the highest temperature fraction for any surface (0...n).  
double XR3HullTempsMultiDisplayMode::GetHighestTempFrac()
{
    XR3Phoenix &XR3 = GetXR3();      // for convenience

    const HullTemperatureLimits &limits = XR3.m_hullTemperatureLimits;
    double highestTempFrac = 0.0;    // max percentage of any hull temperature to its limit

    // if a surface's door is open, its limits will be lower
#define IS_DOOR_OPEN(status) (status != DOOR_CLOSED)   // includes DOOR_FAILED
#define LIMITK(limitK, doorStatus)  (IS_DOOR_OPEN(doorStatus) ? limits.doorOpen : limitK)
#define SET_MAX_PCT(tempK, limitK, doorStatus)  { double pct = (tempK / LIMITK(limitK, doorStatus)); if (pct > highestTempFrac) highestTempFrac = pct; }

    // nosecone temp is tied to hover doors, gear, elevator, and retro doors use nosecone limit
    SET_MAX_PCT(XR3.m_noseconeTemp,  limits.noseCone, XR3.hoverdoor_status);  
    SET_MAX_PCT(XR3.m_noseconeTemp,  limits.noseCone, XR3.gear_status);  
    SET_MAX_PCT(XR3.m_noseconeTemp,  limits.noseCone, XR3.crewElevator_status);  
    SET_MAX_PCT(XR3.m_noseconeTemp,  limits.noseCone, XR3.rcover_status);  

    // no doors on the wings
    SET_MAX_PCT(XR3.m_leftWingTemp,  limits.wings, DOOR_CLOSED);
    SET_MAX_PCT(XR3.m_rightWingTemp, limits.wings, DOOR_CLOSED);

    // cockpit temp is tied to the crew hatch
    SET_MAX_PCT(XR3.m_cockpitTemp,   limits.cockpit, XR3.hatch_status);

    // top hull temp is tied to docking port, radiator, and payload bay doors
    SET_MAX_PCT(XR3.m_topHullTemp,   limits.topHull, XR3.nose_status);  // this is the docking port
    SET_MAX_PCT(XR3.m_topHullTemp,   limits.topHull, XR3.radiator_status);
    SET_MAX_PCT(XR3.m_topHullTemp,   limits.topHull, XR3.bay_status);

    return highestTempFrac;
}


// determines which door(s) to use for temperature display warning colors
#define CHECK_AND_RETURN_DOOR(doorStatus) if (doorStatus != DOOR_CLOSED) return doorStatus
DoorStatus XR3HullTempsMultiDisplayMode::GetNoseDoorStatus()
{
    CHECK_AND_RETURN_DOOR(GetXR3().crewElevator_status);
    CHECK_AND_RETURN_DOOR(GetXR3().hoverdoor_status);
    CHECK_AND_RETURN_DOOR(GetXR3().rcover_status);
    CHECK_AND_RETURN_DOOR(GetXR3().gear_status);

    return DOOR_CLOSED;  // no open doors for this surface
}
 
DoorStatus XR3HullTempsMultiDisplayMode::GetLeftWingDoorStatus()
{
    return DOOR_CLOSED;  // no doors on the wings
}
 
DoorStatus XR3HullTempsMultiDisplayMode::GetRightWingDoorStatus()
{
    return DOOR_CLOSED;  // no doors on the wings
}

// base class behavior is fine for GetCockpitDoorStatus (only crew hatch to check)

DoorStatus XR3HullTempsMultiDisplayMode::GetTopHullDoorStatus()
{
    CHECK_AND_RETURN_DOOR(GetXR3().nose_status);    // docking port
    CHECK_AND_RETURN_DOOR(GetXR3().radiator_status);
    CHECK_AND_RETURN_DOOR(GetXR3().bay_status);
    
    return DOOR_CLOSED; 
}

//----------------------------------------------------------------------------------

// this light is read-only
DockingPortActiveLEDArea::DockingPortActiveLEDArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    XR1Area(parentPanel, panelCoordinates, areaID)
{
}

void DockingPortActiveLEDArea::Activate()
{
    Area::Activate();  // invoke superclass method
    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(18, 15), PANEL_REDRAW_USER, PANEL_MOUSE_IGNORE);  // redrawn only on request from the active switch area
    m_mainSurface = CreateSurface(IDB_GREEN_LED_TINY);
}

bool DockingPortActiveLEDArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    // always render this since it is only drawn by request
    oapiBlt(surf, m_mainSurface, 0, 0, ((GetXR3().m_activeEVAPort == XR3Phoenix::DOCKING_PORT) ? 18 : 0), 0, 18, 15);
    return true;
}

//----------------------------------------------------------------------------------

// this light is read-only
CrewElevatorActiveLEDArea::CrewElevatorActiveLEDArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    XR1Area(parentPanel, panelCoordinates, areaID)
{
}

void CrewElevatorActiveLEDArea::Activate()
{
    Area::Activate();  // invoke superclass method
    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(18, 15), PANEL_REDRAW_USER, PANEL_MOUSE_IGNORE);  // redrawn only on request from the active switch area
    m_mainSurface = CreateSurface(IDB_GREEN_LED_TINY);
}

bool CrewElevatorActiveLEDArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    // always render this since it is only drawn by request
    oapiBlt(surf, m_mainSurface, 0, 0, ((GetXR3().m_activeEVAPort == XR3Phoenix::CREW_ELEVATOR) ? 18 : 0), 0, 18, 15);
    return true;
}

//-------------------------------------------------------------------------

ActiveEVAPortSwitchArea::ActiveEVAPortSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
  HorizontalCenteringRockerSwitchArea(parentPanel, panelCoordinates, areaID, false, false, 
      // Note: we cannot call Area.GetVessel() here yet (since the class is not yet instantiated), so we have to do it the hard way...
      (( (static_cast<XR3Phoenix &>((parentPanel.GetVessel()))).m_activeEVAPort == XR3Phoenix::DOCKING_PORT) ? LEFT : RIGHT) )  // this is a SINGLE switch
{
}

// must hook Redraw here so we can keep the m_lastSwitchPosition in sync with the active docking port status
bool ActiveEVAPortSwitchArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    // this is a single switch, so we only need to set index 0 here
    m_lastSwitchPosition[0] = ((GetXR3().m_activeEVAPort == XR3Phoenix::DOCKING_PORT) ? LEFT : RIGHT);

    // now let the superclass method run
    return HorizontalCenteringRockerSwitchArea::Redraw2D(event, surf);
}


// Process a mouse event that occurred on our switch
// switches = which switches moved (SINGLE, NA); if NA, it means that no switch is pressed (i.e., button-up occurred and position == CENTER)
// position = current switch position (LEFT, RIGHT, CENTER)
void ActiveEVAPortSwitchArea::ProcessSwitchEvent(SWITCHES switches, POSITION position)
{
    // ignore switches NA (button-up events)
    if (switches == NA)
        return;
    
    XR3Phoenix::ACTIVE_EVA_PORT newState;
    switch (position)
    {
    case LEFT:
        newState = XR3Phoenix::DOCKING_PORT;
        break;

    case RIGHT:
        newState = XR3Phoenix::CREW_ELEVATOR;
        break;

    default:     // CENTER
        return;  // ignore
    };

    // perform the switch
    GetXR3().SetActiveEVAPort(newState);
}

