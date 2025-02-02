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

// Process a vertical self-centering rocker switch
// isDual: true = is dual switches, false = single switch
// pAnimHandle = animation handle for 3D switch; may be null.
// initialPosition: defaults to CENTER if not set; if not CENTER, switch will not auto-center
VerticalCenteringRockerSwitchArea::VerticalCenteringRockerSwitchArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID, bool isDual, bool reverseRotation, POSITION initialPosition) :
    XR1Area(parentPanel, panelCoordinates, areaID, meshTextureID),
    m_isDual(isDual), m_pAnimationHandle(nullptr), m_reverseRotation(reverseRotation), m_initialPosition(initialPosition)
{
}

void VerticalCenteringRockerSwitchArea::Activate()
{
    Area::Activate();  // invoke superclass method
    if (IsVC())  // 3D
    {
        oapiVCRegisterArea(GetAreaID(), PANEL_REDRAW_MOUSE, PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBPRESSED | PANEL_MOUSE_LBUP);
    }
    else   // 2D
    {
        int sizeX = 16;     // width
        int sizeY = 44;     // height

        if (m_isDual)
            sizeX = 35;

        // note: PANEL_MOUSE_LBPRESSED is sent repeatedly when the mouse button is HELD down
        oapiRegisterPanelArea(GetAreaID(), GetRectForSize(sizeX, sizeY),
            PANEL_REDRAW_MOUSE, PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBUP | PANEL_MOUSE_LBPRESSED, PANEL_MAP_CURRENT);

        m_mainSurface = CreateSurface(IDB_SWITCH4);
    }

    // intialize state variables
    m_lastSwitchPosition[0] = m_lastSwitchPosition[1] = m_initialPosition;

    TriggerRedraw();
}

bool VerticalCenteringRockerSwitchArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    int srcX;

    if (m_isDual == FALSE)
    {
        // single switch
        const POSITION lastPos = m_lastSwitchPosition[0];
        if (lastPos == POSITION::CENTER)
            srcX = 0;
        else if (lastPos == POSITION::UP)
            srcX = 16;
        else  // DOWN
            srcX = 32;

        DeltaGliderXR1::SafeBlt(surf, m_mainSurface, 0, 0, srcX, 0, 16, 44);
    }
    else  // dual switch
    {
        for (int i = 0; i < 2; i++)
        {
            const POSITION lastPos = m_lastSwitchPosition[i];
            if (lastPos == POSITION::CENTER)
                srcX = 0;
            else if (lastPos == POSITION::UP)
                srcX = 16;
            else  // DOWN
                srcX = 32;

            DeltaGliderXR1::SafeBlt(surf, m_mainSurface, i * 19, 0, srcX, 0, 16, 44);
        }
    }

    return true;
}

bool VerticalCenteringRockerSwitchArea::Redraw3D(const int event, const SURFHANDLE surf)
{
    bool retCode = false;

    if (m_pAnimationHandle) // any 3D switch to animate?
    {
        for (int i = 0; i < (m_isDual ? 2 : 1); i++)
        {
            double animationState;
            if (m_lastSwitchPosition[i] == POSITION::CENTER)
                animationState = 0.5;
            else if (m_lastSwitchPosition[i] == POSITION::DOWN)
                animationState = 0;
            else        // UP
                animationState = 1;

            // reverse rotation if requested
            if (m_reverseRotation)
                animationState = 1 - animationState;

            GetXR1().SetXRAnimation(m_pAnimationHandle[i], animationState);
        }
        retCode = true;
    }
    return retCode;
}

bool VerticalCenteringRockerSwitchArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitated())
        return false;

    SWITCHES switches = SWITCHES::NA;  // which switches moved
    POSITION position = POSITION::CENTER;  // up, down, center

    // true if switch is pressed in any direction
    bool isPressed = ((event & (PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBPRESSED)) != 0);

    if (m_isDual == FALSE)
    {
        if (isPressed)
        {
            if (my < 22) position = POSITION::UP;
            else         position = POSITION::DOWN;
        }

        switches = SWITCHES::SINGLE;
    }
    else  // dual switch
    {
        if (isPressed)
        {
            if (mx < 10) switches = SWITCHES::LEFT;
            else if (mx >= 25) switches = SWITCHES::RIGHT;
            else               switches = SWITCHES::BOTH;

            if (my < 22) position = POSITION::UP;
            else               position = POSITION::DOWN;
        }
    }

    // play sound if the mouse was just clicked
    if ((position != POSITION::CENTER) && (event & PANEL_MOUSE_LBDOWN))
    {
        // play a quiet click if this is auto-centering, or a normal click if not auto-centering
        if (m_initialPosition == POSITION::CENTER)
            GetXR1().PlaySound(GetXR1().SwitchOn, DeltaGliderXR1::ST_Other, QUIET_CLICK);    // light click
        else
            GetXR1().PlaySound(GetXR1().SwitchOn, DeltaGliderXR1::ST_Other);  // normal click
    }

    return DispatchSwitchEvent(event, switches, position);
}

bool VerticalCenteringRockerSwitchArea::ProcessVCMouseEvent(const int event, const VECTOR3& coords)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    SWITCHES switches;  // which switches moved
    POSITION position = POSITION::CENTER;  // up, down, center

    // true if switch is pressed in any direction
    bool isPressed = ((event & (PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBPRESSED)) != 0);

    if (m_isDual == FALSE)
    {
        if (isPressed)
        {
            if (coords.y < 0.5) position = POSITION::UP;
            else                position = POSITION::DOWN;
        }

        switches = SWITCHES::SINGLE;
    }
    else  // dual switch
    {
        if (isPressed)
        {
            if (coords.x < 0.25)  switches = SWITCHES::LEFT;
            else if (coords.x >= 0.75) switches = SWITCHES::RIGHT;
            else                       switches = SWITCHES::BOTH;

            if (coords.y < 0.5) position = POSITION::UP;
            else                      position = POSITION::DOWN;
        }
    }

    // play sound if the mouse was just clicked
    if ((position != POSITION::CENTER) && (event & PANEL_MOUSE_LBDOWN))
        GetXR1().PlaySound(GetXR1().SwitchOn, DeltaGliderXR1::ST_Other, QUIET_CLICK);    // light click

    return DispatchSwitchEvent(event, switches, position);
}

// common handler to dispatch a switch event
// return: true event was handled, false if not handled
bool VerticalCenteringRockerSwitchArea::DispatchSwitchEvent(const int event, SWITCHES& switches, POSITION& position)
{
    if (event & PANEL_MOUSE_LBUP)
    {
        // no movement, but we still need to repaint the switch texture
        switches = SWITCHES::NA;
        position = POSITION::CENTER;
    }

    // save "last rendered" state
    switch (switches)
    {
    case SWITCHES::SINGLE:
    case SWITCHES::LEFT:
        m_lastSwitchPosition[0] = position;
        break;

    case SWITCHES::RIGHT:
        m_lastSwitchPosition[1] = position;
        break;

    case SWITCHES::BOTH:
    case SWITCHES::NA:   // on button-up, reset to center if centering mode enabled
        if (m_initialPosition == POSITION::CENTER)
            m_lastSwitchPosition[0] = m_lastSwitchPosition[1] = position;
        break;
    }

    // invoke the subclass's handler to process the switch event
    ProcessSwitchEvent(switches, position);

    return true;
}

//-------------------------------------------------------------------------

// Process a horizontal self-centering rocker switch
// isDual: true = is dual switches, false = single switch
// initialPosition: defaults to CENTER if not set; if not CENTER, switch will not auto-center
HorizontalCenteringRockerSwitchArea::HorizontalCenteringRockerSwitchArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID, bool isDual, bool reverseRotation, POSITION initialPosition) :
    XR1Area(parentPanel, panelCoordinates, areaID, meshTextureID),
    m_isDual(isDual), m_pAnimationHandle(nullptr), m_reverseRotation(reverseRotation), m_initialPosition(initialPosition)
{
}

void HorizontalCenteringRockerSwitchArea::Activate()
{
    Area::Activate();  // invoke superclass method
    if (IsVC())  // 3D
    {
        oapiVCRegisterArea(GetAreaID(), PANEL_REDRAW_MOUSE, PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBPRESSED | PANEL_MOUSE_LBUP);
    }
    else   // 2D
    {
        int sizeX = 44;     // width
        int sizeY = 16;     // height

        if (m_isDual)
            sizeY = 35;     // twice the height plus a few pixels separation

        // note: PANEL_MOUSE_LBPRESSED is sent repeatedly when the mouse button is HELD down
        oapiRegisterPanelArea(GetAreaID(), GetRectForSize(sizeX, sizeY),
            PANEL_REDRAW_MOUSE, PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBUP | PANEL_MOUSE_LBPRESSED, PANEL_MAP_CURRENT);

        m_mainSurface = CreateSurface(IDB_SWITCH4R);    // horizontal switches
    }

    // initialize state variables
    m_lastSwitchPosition[0] = m_lastSwitchPosition[1] = m_initialPosition;

    TriggerRedraw();
}

bool HorizontalCenteringRockerSwitchArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    int srcY;

    if (m_isDual == FALSE)
    {
        // single switch
        const POSITION lastPos = m_lastSwitchPosition[static_cast<int>(POSITION::LEFT)];
        if (lastPos == POSITION::CENTER)
            srcY = 0;
        else if (lastPos == POSITION::LEFT)
            srcY = 16;
        else  // RIGHT
            srcY = 32;

        DeltaGliderXR1::SafeBlt(surf, m_mainSurface, 0, 0, 0, srcY, 44, 16);
    }
    else  // dual switch
    {
        for (int i = 0; i < 2; i++)
        {
            const POSITION lastPos = m_lastSwitchPosition[i];
            if (lastPos == POSITION::CENTER)
                srcY = 0;
            else if (lastPos == POSITION::LEFT)
                srcY = 16;
            else  // DOWN
                srcY = 32;

            DeltaGliderXR1::SafeBlt(surf, m_mainSurface, 0, i * 19, 0, srcY, 44, 16);
        }
    }

    return true;
}

bool HorizontalCenteringRockerSwitchArea::Redraw3D(const int event, const SURFHANDLE surf)
{
    bool retCode = false;

    if (m_pAnimationHandle) // any 3D switch to animate?
    {
        for (int i = 0; i < (m_isDual ? 2 : 1); i++)
        {
            double animationState;
            if (m_lastSwitchPosition[i] == POSITION::CENTER)
                animationState = 0.5;
            else if (m_lastSwitchPosition[i] == POSITION::RIGHT)
                animationState = 0;
            else        // LEFT
                animationState = 1;

            // reverse rotation if requested
            if (m_reverseRotation)
                animationState = 1 - animationState;

            GetXR1().SetXRAnimation(m_pAnimationHandle[i], animationState);
        }
        retCode = true;
    }
    return retCode;
}

bool HorizontalCenteringRockerSwitchArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    SWITCHES switches = SWITCHES::NA;      // which switches moved
    POSITION position = POSITION::CENTER;  // LEFT, RIGHT, CENTER

    // true if switch is pressed in any direction
    bool isPressed = ((event & (PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBPRESSED)) != 0);
    if (m_isDual == FALSE)
    {
        if (isPressed)
        {
            if (mx < 22) position = POSITION::LEFT;
            else         position = POSITION::RIGHT;
        }

        switches = SWITCHES::SINGLE;
    }
    else  // dual switch
    {
        if (isPressed)
        {
            if (my < 10) switches = SWITCHES::TOP;
            else if (my >= 25) switches = SWITCHES::BOTTOM;
            else               switches = SWITCHES::BOTH;

            if (mx < 22) position = POSITION::LEFT;
            else               position = POSITION::RIGHT;
        }
    }

    // play sound if the mouse was just clicked    
    if ((position != POSITION::CENTER) && (event & PANEL_MOUSE_LBDOWN))
    {
        // play a quiet click if this is auto-centering, or a normal click if not auto-centering
        if (m_initialPosition == POSITION::CENTER)
            GetXR1().PlaySound(GetXR1().SwitchOn, DeltaGliderXR1::ST_Other, QUIET_CLICK);    // light click
        else
            GetXR1().PlaySound(GetXR1().SwitchOff, DeltaGliderXR1::ST_Other);  // normal click; SwitchOff is slight louder, so let's use that
    }

    return DispatchSwitchEvent(event, switches, position);
}

bool HorizontalCenteringRockerSwitchArea::ProcessVCMouseEvent(const int event, const VECTOR3& coords)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    SWITCHES switches;  // which switches moved
    POSITION position = POSITION::CENTER;  // up, down, center

    // true if switch is pressed in any direction
    bool isPressed = ((event & (PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBPRESSED)) != 0);

    if (m_isDual == FALSE)
    {
        if (isPressed)
        {
            if (coords.x < 0.5) position = POSITION::LEFT;
            else                position = POSITION::RIGHT;
        }

        switches = SWITCHES::SINGLE;
    }
    else  // dual switch
    {
        if (isPressed)
        {
            if (coords.y < 0.25)  switches = SWITCHES::TOP;
            else if (coords.y >= 0.75) switches = SWITCHES::BOTTOM;
            else                       switches = SWITCHES::BOTH;

            if (coords.x < 0.5) position = POSITION::LEFT;
            else                      position = POSITION::RIGHT;
        }
    }

    // play sound if the mouse was just clicked
    if ((position != POSITION::CENTER) && (event & PANEL_MOUSE_LBDOWN))
        GetXR1().PlaySound(GetXR1().SwitchOn, DeltaGliderXR1::ST_Other, QUIET_CLICK);    // light click

    return DispatchSwitchEvent(event, switches, position);
}

// common handler to dispatch a switch event
// return: true event was handled, false if not handled
bool HorizontalCenteringRockerSwitchArea::DispatchSwitchEvent(const int event, SWITCHES& switches, POSITION& position)
{
    if (event & PANEL_MOUSE_LBUP)
    {
        // no movement, but we still need to repaint the switch texture
        switches = SWITCHES::NA;
        position = POSITION::CENTER;
    }

    // save "last rendered" state
    switch (switches)
    {
    case SWITCHES::SINGLE:
    case SWITCHES::TOP:
        m_lastSwitchPosition[0] = position;
        break;

    case SWITCHES::BOTTOM:
        m_lastSwitchPosition[1] = position;
        break;

    case SWITCHES::BOTH:
    case SWITCHES::NA:   // on button-up, reset to center if centering mode enabled
        if (m_initialPosition == POSITION::CENTER)
            m_lastSwitchPosition[0] = m_lastSwitchPosition[1] = position;
        break;
    }

    // invoke the subclass's handler to process the switch event
    ProcessSwitchEvent(switches, position);

    return true;
}

//----------------------------------------------------------------------------------

// 2D-only for now
// indicatorAreaID = areaID of status light, etc.  -1 = none.
ToggleSwitchArea::ToggleSwitchArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_indicatorAreaID(indicatorAreaID)
{
}

void ToggleSwitchArea::Activate()
{
    Area::Activate();  // invoke superclass method
    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(25, 38), PANEL_REDRAW_MOUSE | PANEL_REDRAW_USER, PANEL_MOUSE_LBDOWN);
    m_mainSurface = CreateSurface(IDB_SWITCH1);     // gray rocker switch

    TriggerRedraw();    // render initial switch setting

    if (m_indicatorAreaID >= 0)
        GetVessel().TriggerRedrawArea(m_indicatorAreaID); // render indicator too, if any
}

bool ToggleSwitchArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    DeltaGliderXR1::SafeBlt(surf, m_mainSurface, 0, 0, (isOn() ? 0 : 25), 0, 25, 38);

    return true;
}

bool ToggleSwitchArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    bool switchIsOn = (my >= 19);

    // check current state to see whether switch is changing the state
    if (isOn() == switchIsOn)
    {
        return false;   // switch already in that position
    }

    // play sound if the mouse was just clicked
    if (event & PANEL_MOUSE_LBDOWN)
        GetXR1().PlaySound((switchIsOn ? GetXR1().SwitchOn : GetXR1().SwitchOff), DeltaGliderXR1::ST_Other);

    // invoke the subclass to handle the mouse event
    bool retVal = ProcessSwitchEvent(switchIsOn);

    // notify the indicator if the switch changed state
    if (retVal && (m_indicatorAreaID >= 0))
        GetVessel().TriggerRedrawArea(m_indicatorAreaID); // render indicator, if any

    return retVal;
}

//----------------------------------------------------------------------------------

SupplyHatchToggleSwitchArea::SupplyHatchToggleSwitchArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID, DoorStatus& doorStatus, const char* pHatchName, const UINT& animHandle) :
    ToggleSwitchArea(parentPanel, panelCoordinates, areaID, indicatorAreaID),
    m_doorStatus(doorStatus), m_animHandle(animHandle)
{
    strcpy(m_hatchName, pHatchName);
}

bool SupplyHatchToggleSwitchArea::ProcessSwitchEvent(bool switchIsOn)
{
    // may resupply if grounded and stopped or if docked
    const bool doorUnlocked = (GetXR1().IsLanded() || GetXR1().IsDocked());
    if (doorUnlocked == false)
    {
        GetXR1().PlaySound(GetXR1().Error1, DeltaGliderXR1::ST_Other, ERROR1_VOL);
        GetXR1().ShowWarning("Resupply Hatches Locked.wav", DeltaGliderXR1::ST_WarningCallout, "Resupply hatches locked while in flight.");
        return false;
    }

    // set XR1 state via ref
    m_doorStatus = (switchIsOn ? DoorStatus::DOOR_OPEN : DoorStatus::DOOR_CLOSED);

    // update the hatch animation state *if* the vessel currently allows it; this hatch "snaps" open or closed
    if (GetXR1().GetXR1Config()->EnableResupplyHatchAnimationsWhileDocked)
        GetXR1().SetXRAnimation(m_animHandle, (switchIsOn ? 1.0 : 0.0));

    // play door thump sound 
    GetXR1().PlaySound(GetXR1().SupplyHatch, DeltaGliderXR1::ST_Other, SUPPLY_HATCH_VOL);

    // log info message and play callout
    const char* pState = (switchIsOn ? "open" : "closed");
    char msg[40];
    char wavFilename[40];
    sprintf(msg, "%s hatch %s.", m_hatchName, pState);
    sprintf(wavFilename, "%s hatch %s.wav", m_hatchName, pState);
    GetXR1().ShowInfo(wavFilename, DeltaGliderXR1::ST_InformationCallout, msg);

    return true;
}

bool SupplyHatchToggleSwitchArea::isOn()
{
    return (m_doorStatus == DoorStatus::DOOR_OPEN);
}

//----------------------------------------------------------------------------------

// switchState = ref to bool switch state
BoolToggleSwitchArea::BoolToggleSwitchArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID, bool& switchState) :
    ToggleSwitchArea(parentPanel, panelCoordinates, areaID, indicatorAreaID),
    m_switchState(switchState)
{
}

bool BoolToggleSwitchArea::ProcessSwitchEvent(bool switchIsOn)
{
    m_switchState = switchIsOn;   // ref to XR1 variable
    return true;
}

bool BoolToggleSwitchArea::isOn()
{
    return m_switchState;       // ref to XR1 variable
}
