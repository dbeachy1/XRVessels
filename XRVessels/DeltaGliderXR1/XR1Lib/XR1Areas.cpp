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
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// XR1Area.cpp
// Abstract area base class that each of our Areas extend
// Also includes additional base classes that add functionality
// ==============================================================

// must be included BEFORE XR1Areas.h
#include "DeltaGliderXR1.h"
#include "XR1Areas.h"
#include "resource.h"

//
// Constructor
// vessel = our vessel handle
// panelCoordinates = absolute coordinates of this area on the parent instrument panel
// areaID = unique Orbiter area ID
XR1Area::XR1Area(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID) :
    Area(parentPanel, panelCoordinates, areaID, meshTextureID)
{
}

// Destructor
XR1Area::~XR1Area()
{
}

//----------------------------------------------------------------------------------

// Process a vertical self-centering rocker switch
// isDual: true = is dual switches, false = single switch
// pAnimHandle = animation handle for 3D switch; may be null.
// initialPosition: defaults to CENTER if not set; if not CENTER, switch will not auto-center
VerticalCenteringRockerSwitchArea::VerticalCenteringRockerSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID, bool isDual, bool reverseRotation, POSITION initialPosition) :
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
        for (int i=0; i < 2; i++)
        {
            const POSITION lastPos = m_lastSwitchPosition[i];
            if (lastPos == POSITION::CENTER)
                srcX = 0;
            else if (lastPos == POSITION::UP)
                srcX = 16;
            else  // DOWN
                srcX = 32;

            DeltaGliderXR1::SafeBlt (surf, m_mainSurface, i * 19, 0, srcX, 0, 16, 44);
        }
    }

    return true;
}

bool VerticalCenteringRockerSwitchArea::Redraw3D(const int event, const SURFHANDLE surf)
{
    bool retCode = false;

    if (m_pAnimationHandle) // any 3D switch to animate?
    {
        for (int i=0; i < (m_isDual ? 2 : 1); i++)
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
            if      (mx <  10) switches = SWITCHES::LEFT;
            else if (mx >= 25) switches = SWITCHES::RIGHT;
            else               switches = SWITCHES::BOTH;

            if      (my <  22) position = POSITION::UP;
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

bool VerticalCenteringRockerSwitchArea::ProcessVCMouseEvent(const int event, const VECTOR3 &coords)
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
            if      (coords.x < 0.25)  switches = SWITCHES::LEFT;
            else if (coords.x >= 0.75) switches = SWITCHES::RIGHT;
            else                       switches = SWITCHES::BOTH;

            if      (coords.y <  0.5) position = POSITION::UP;
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
bool VerticalCenteringRockerSwitchArea::DispatchSwitchEvent(const int event, SWITCHES &switches, POSITION &position)
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
HorizontalCenteringRockerSwitchArea::HorizontalCenteringRockerSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID, bool isDual, bool reverseRotation, POSITION initialPosition) :
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
        for (int i=0; i < 2; i++)
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
        for (int i=0; i < (m_isDual ? 2 : 1); i++)
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
            if      (my <  10) switches = SWITCHES::TOP;
            else if (my >= 25) switches = SWITCHES::BOTTOM;
            else               switches = SWITCHES::BOTH;

            if      (mx <  22) position = POSITION::LEFT;
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

bool HorizontalCenteringRockerSwitchArea::ProcessVCMouseEvent(const int event, const VECTOR3 &coords)
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
            if      (coords.y < 0.25)  switches = SWITCHES::TOP;
            else if (coords.y >= 0.75) switches = SWITCHES::BOTTOM;
            else                       switches = SWITCHES::BOTH;

            if      (coords.x <  0.5) position = POSITION::LEFT;
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
bool HorizontalCenteringRockerSwitchArea::DispatchSwitchEvent(const int event, SWITCHES &switches, POSITION &position)
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

IndicatorGaugeArea::IndicatorGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const bool isDual, 
                                       const int redrawFlag, const int meshTextureID, const int deltaX, const int deltaY, const int gapSize) :
    XR1Area(parentPanel, panelCoordinates, areaID, meshTextureID), 
    m_isDual(isDual), m_redIndicatorSurface(nullptr), m_yellowIndicatorSurface(nullptr), m_redrawFlag(redrawFlag), 
    m_deltaX(deltaX), m_deltaY(deltaY), m_gapSize(gapSize)
{
}

void IndicatorGaugeArea::Activate()
{
    Area::Activate();  // invoke superclass method

    // invoke the subclass to get the area size, excluding any delta values
    COORD2 areaSize = GetAreaSize();
    const int sizeX = areaSize.x;
    const int sizeY = areaSize.y;

    if (IsVC()) // 3D panel?
    {
        oapiVCRegisterArea(GetAreaID(), GetRectForSize(sizeX + m_deltaX, sizeY + m_deltaY), m_redrawFlag, PANEL_MOUSE_IGNORE, PANEL_MAP_BGONREQUEST, GetVCPanelTextureHandle());
    }
    else    // 2D panel
    {
        oapiRegisterPanelArea(GetAreaID(), GetRectForSize(sizeX + m_deltaX, sizeY + m_deltaY), m_redrawFlag, PANEL_MOUSE_IGNORE, PANEL_MAP_BGONREQUEST);
    }

    m_mainSurface = CreateSurface(IDB_INDICATOR2);              // standard green indicator arrows
    m_redIndicatorSurface = CreateSurface(IDB_RED_INDICATOR2);  // red indicator arrows
    m_yellowIndicatorSurface = CreateSurface(IDB_YELLOW_INDICATOR2);  // yellow indicator arrows
    
    DWORD white = 0xFFFFFF;           // set WHITE as transparent color (Note: to use black, set 0xFF000000, not 0!)
    SetSurfaceColorKey(m_mainSurface, white);
    SetSurfaceColorKey(m_redIndicatorSurface, white);
    SetSurfaceColorKey(m_yellowIndicatorSurface, white);
    
    // reset state variables to force a repaint
    ResetRenderData();
    TriggerRedraw();
}

void IndicatorGaugeArea::Deactivate()
{
    DestroySurface(&m_redIndicatorSurface);        // clean up secondary surface
    DestroySurface(&m_yellowIndicatorSurface);     // clean up secondary surface
    XR1Area::Deactivate();  // invoke superclass method
}

SURFHANDLE IndicatorGaugeArea::GetSurfaceForColor(COLOR c)
{
    SURFHANDLE srcSurface;
    switch (c)
    {
    case COLOR::RED:
        srcSurface = m_redIndicatorSurface;
        break;

    case COLOR::YELLOW:
        srcSurface = m_yellowIndicatorSurface;
        break;

    default:  // GREEN or unknown...
        srcSurface = m_mainSurface;
        break;
    }

    return srcSurface;
}

//----------------------------------------------------------------------------------

// vertical gauge area; may be single or dual, and may render in green, red, or both
// registered area is either 6 pixels wide (single) or 13 pixels wide (dual)
// sizeY = vertical size of gauge in pixels
// deltaX, deltaY = offset from area start to draw pointer; default is 0
// gapSize = gap (in pixels) between dual indicators; default=1.  Has no effect on single-indicator gauges.
// singleSide = LEFT or RIGHT; default = LEFT (determines which indicator arrow to use for a single gauge)
VerticalGaugeArea::VerticalGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const bool isDual, const int sizeY, 
                                     const int redrawFlag, const int meshTextureID, const int deltaX, const int deltaY, const int gapSize, const SIDE singleSide) :
    IndicatorGaugeArea(parentPanel, panelCoordinates, areaID, isDual, redrawFlag, meshTextureID, deltaX, deltaY, gapSize),
    m_sizeY(sizeY), m_singleSide(singleSide)
{
}

// get area size in pixels, excluding any delta size additions
COORD2 VerticalGaugeArea::GetAreaSize()
{
    int sizeX = (m_isDual ? (12 + m_gapSize) : 6);  // each indicator is 6 pixels side
    return _COORD2(sizeX, m_sizeY);
}

void VerticalGaugeArea::ResetRenderData()
{
    m_lastRenderData[0].Reset();
    m_lastRenderData[1].Reset();
}

bool VerticalGaugeArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    bool retVal = false;    // assume not re-rendered

    int gaugeCount = (m_isDual ? 2 : 1);

    // check whether either gauge needs to be repainted
    RENDERDATA renderData[2];
    bool doRender = false;  // assume no render required
    for (int i=0; i < gaugeCount; i++)
    {
        // invoke subclass method to obtain color and indexY data for each gauge
        renderData[i] = GetRenderData((i == 0 ? SIDE::LEFT : SIDE::RIGHT));
        if (renderData[i] != m_lastRenderData[i])
        {
            doRender = true;
            // do not break here; allow renderData[1] to be initialized if present
        }
    }

    if (doRender)
    {
        // repaint the background
        oapiBltPanelAreaBackground (GetAreaID(), surf);

        // invoke the caller's hook in case he needs to paint something before we do
        Redraw2DFirstHook(event, surf);

        if (m_isDual)
        {
            // repaint the gauge
            for (int i=0; i < gaugeCount; i++)
            {
                SIDE side = (m_isDual ? ((i == 0) ? SIDE::LEFT : SIDE::RIGHT) : m_singleSide);

                // render the gauge
                const SURFHANDLE srcSurface = GetSurfaceForColor(renderData[i].color);
                int tgtX = ((side == SIDE::LEFT) ? 0 : 6 + m_gapSize);  // if right side, bump right 6+gapSize pixels
                int srcX = ((side == SIDE::LEFT) ? 0 : 6);  // if right side, go right 6 pixels for source
                //      tgt,  src,        tgtx,            tgty,                            srcx,srcy,w,h, <use predefined color key>
                DeltaGliderXR1::SafeBlt(surf, srcSurface, tgtX + m_deltaX, renderData[i].indexY + m_deltaY, srcX, 0, 6, 7, SURF_PREDEF_CK);


                // update <last updated> render data
                m_lastRenderData[i] = renderData[i];   // default byte-for-byte copy here is fine
            }
        }
        else    // single gauge
        {
                // render the gauge
                const SURFHANDLE srcSurface = GetSurfaceForColor(renderData[0].color);
                int srcX = ((m_singleSide == SIDE::LEFT) ? 0 : 6);  // if right side, go right 6 pixels for source
                //      tgt,  src,        tgtx,         tgty,                            srcx,srcy,w,h, <use predefined color key>
                DeltaGliderXR1::SafeBlt(surf, srcSurface, 0 + m_deltaX, renderData[0].indexY + m_deltaY, srcX, 0, 6, 7, SURF_PREDEF_CK);

                // update <last updated> render data
                m_lastRenderData[0] = renderData[0];   // default byte-for-byte copy here is fine
        }
        retVal = true;
    }

    return retVal;
}

//----------------------------------------------------------------------------------

// horizontal gauge area; may be single or dual, and may render in green, red, or both
// registered area is either 6 pixels high (single) or 13 pixels high (dual)
// sizeX = horizontal size of gauge in pixels, including 3 pixels on each side of movable area auge for arrow to display
// redrawFlag = PANEL_REDRAW_USER, PANEL_REDRAW_ALWAYS, etc.
// deltaX, deltaY = offset from area start to draw pointer; default is 0
// gapSize = gap (in pixels) between dual indicators; default=1.  Has no effect on single-indicator gauges.
// singleSide = TOP or BOTTOM; default = BOTTOM  (determines which indicator arrow to use for a single gauge)
HorizontalGaugeArea::HorizontalGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const bool isDual, const int sizeX, 
                                         const int redrawFlag, const int meshTextureID, const int deltaX, const int deltaY, const int gapSize, const SIDE singleSide) :
    IndicatorGaugeArea(parentPanel, panelCoordinates, areaID, isDual, redrawFlag, meshTextureID, deltaX, deltaY, gapSize),
    m_sizeX(sizeX), m_singleSide(singleSide)
{
}

// get area size in pixels, excluding any delta size additions
COORD2 HorizontalGaugeArea::GetAreaSize()
{
    int sizeY = (m_isDual ? (12 + m_gapSize) : 6);    // each indicator is 6 pixels side
    return _COORD2(m_sizeX, sizeY);
}

void HorizontalGaugeArea::ResetRenderData()
{
    m_lastRenderData[0].Reset();
    m_lastRenderData[1].Reset();
}

bool HorizontalGaugeArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    bool retVal = false;    // assume not re-rendered

    int gaugeCount = (m_isDual ? 2 : 1);

    // check whether either gauge needs to be repainted
    RENDERDATA renderData[2];
    bool doRender = false;  // assume no render required
    for (int i=0; i < gaugeCount; i++)
    {
        // invoke subclass method to obtain color and indexY data for each gauge
        renderData[i] = GetRenderData((i == 0 ? SIDE::TOP : SIDE::BOTTOM));
        if (renderData[i] != m_lastRenderData[i])
        {
            doRender = true;
            // do not break here; allow renderData[1] to be initialized if present
        }
    }

    if (doRender)
    {
        // repaint the background
        oapiBltPanelAreaBackground (GetAreaID(), surf);

        // invoke the caller's hook in case he needs to paint something before we do
        Redraw2DFirstHook(event, surf);

        // repaint the gauge
        if (m_isDual)
        {
            for (int i=0; i < gaugeCount; i++)
            {
                SIDE side = ((i == 0) ? SIDE::TOP : SIDE::BOTTOM);
                
                const SURFHANDLE srcSurface = GetSurfaceForColor(renderData[i].color);
                int tgtY = ((side == SIDE::TOP) ? 0 : 6 + m_gapSize);  // if bottom side, bump down 6+gapSize pixels
                int srcX = ((side == SIDE::TOP) ? 0 : 7);  // if bottom side, go down 7 pixels for source
                //      tgt,  src,        tgtx,                          tgty,          srcx,srcy,w,h, <use predefined color key>
                DeltaGliderXR1::SafeBlt(surf, srcSurface, renderData[i].indexX + m_deltaX, tgtY + m_deltaY, srcX, 8, 7, 6, SURF_PREDEF_CK);

                // update <last updated> render data
                m_lastRenderData[i] = renderData[i];   // default byte-for-byte copy here is fine
            }
        }
        else    // single gauge
        {
            // render the gauge
            const SURFHANDLE srcSurface = GetSurfaceForColor(renderData[0].color);
            int srcX = ((m_singleSide == SIDE::TOP) ? 0 : 7);  // if bottom side, go down 7 pixels for source
            //      tgt,  src,        tgtx,                            tgty,            srcx,srcy,w,h, <use predefined color key>
            DeltaGliderXR1::SafeBlt(surf, srcSurface, renderData[0].indexX + m_deltaX, 0 + m_deltaY,    srcX, 8, 7, 6, SURF_PREDEF_CK);


            // update <last updated> render data
            m_lastRenderData[0] = renderData[0];   // default byte-for-byte copy here is fine
        }
        retVal = true;
    }

    return retVal;
}

//----------------------------------------------------------------------------------

// NOTE: this is a simple toggle button that stays lit until you change it externally.
// Remember that an area can exist on more than one panel, although each will have a different unique ID.

// lighted button area for a timed event, such as auto-centering a control
// pIsLit ptr to boolean to track IsLit status; if null, the class will use an internal IsLit variable and it will be set to FALSE here.  Otherwise, the value referenced by the pointer will not be changed.
// buttonMeshGroup = mesh group for 3D button; default == -1 (no VC button)
SimpleButtonArea::SimpleButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, bool *pIsLit, const int buttonMeshGroup) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_buttonMeshGroup(buttonMeshGroup), m_defaultIsLit(false)
{
    if (pIsLit == nullptr)
        pIsLit = &m_defaultIsLit;   // use default built-in bool

    m_pIsLit = pIsLit;
}

void SimpleButtonArea::Activate()
{
    Area::Activate();  // invoke superclass method
    m_mainSurface = CreateSurface(IDB_LIGHT2);                  // lighted green button

    if (IsVC()) // 3D panel?
    {
        oapiVCRegisterArea(GetAreaID(), PANEL_REDRAW_USER | PANEL_REDRAW_MOUSE, PANEL_MOUSE_LBDOWN);
    }
    else    // 2D panel
    {
        oapiRegisterPanelArea(GetAreaID(), GetRectForSize(12, 12), PANEL_REDRAW_USER | PANEL_REDRAW_MOUSE, PANEL_MOUSE_LBDOWN);
    }

    // do not reset m_pIsLit value
    
    // no need to redraw here; Orbiter will do it for us
}

bool SimpleButtonArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    // always re-render this since it is always performed on request
    int srcX = (*m_pIsLit ? 12 : 0);    // texture X coord; 12 = lit, 0 = not lit
    DeltaGliderXR1::SafeBlt(surf, m_mainSurface, 0, 0, srcX, 0, 12, 12);

    return true;
}

bool SimpleButtonArea::Redraw3D(const int event, const SURFHANDLE surf)
{
    static const float tv0[8] = {0,0,0.0469f,0.0469f,0,0,0.0469f,0.0469f};

    if ((GetXR1().vcmesh == nullptr) || (m_buttonMeshGroup < 0))
        return false;       // nothing to draw

    static NTVERTEX vtx[8];   // this is OK because Orbiter is single-threaded; i.e., only one XR vessel is active at a time
    float ofs = (*m_pIsLit ? 0.0469f : 0);
    GROUPEDITSPEC ges;
    ges.flags = GRPEDIT_VTXTEXV;
    ges.nVtx = 8;
    ges.vIdx = nullptr;
    ges.Vtx = vtx;

    for (int j = 0; j < 8; j++)
	    vtx[j].tv = tv0[j]+ofs;
    oapiEditMeshGroup (GetXR1().vcmesh, m_buttonMeshGroup, &ges);

    return true;
}

bool SimpleButtonArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    // toggle the button state
    *m_pIsLit = !*m_pIsLit;

    // play sound if the mouse was just clicked
    if (event & PANEL_MOUSE_LBDOWN)
        GetXR1().PlaySound(GetXR1().SwitchOn, DeltaGliderXR1::ST_Other, QUIET_CLICK);    // light click

    return true;
}

//----------------------------------------------------------------------------------

// lighted button area for a timed event, such as auto-centering a control
// pIsLit ptr to boolean to track IsLit status; if null, the class will use an internal IsLit variable and it will be set to FALSE here.  Otherwise, the value referenced by the pointer will not be changed.
// buttonMeshGroup = mesh group for 3D button; default == -1 (no VC button)
TimedButtonArea::TimedButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, bool *pIsLit, const int buttonMeshGroup) :
    SimpleButtonArea(parentPanel, panelCoordinates, areaID, pIsLit, buttonMeshGroup),
    m_previousIsLit(false)
{
}

// invoked once per timestep
void TimedButtonArea::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // invoke the subclass method to process the switch action 
    ProcessTimedEvent(*m_pIsLit, m_previousIsLit, simt, simdt, mjd);    // subclass will take some action here

    m_previousIsLit = *m_pIsLit;  // remember for next time
}

//----------------------------------------------------------------------------------

// analog gauge area
AnalogGaugeArea::AnalogGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const double initialAngle, const int meshTextureID) :
    XR1Area(parentPanel, panelCoordinates, areaID, meshTextureID),
    m_initialAngle(initialAngle)
{
}

void AnalogGaugeArea::Activate()
{
    Area::Activate();  // invoke superclass method
    if (IsVC())  // 3D panel
    {
        oapiVCRegisterArea(GetAreaID(), GetRectForSize(56, 56), PANEL_REDRAW_ALWAYS, PANEL_MOUSE_IGNORE, PANEL_MAP_BGONREQUEST, GetVCPanelTextureHandle());
    }
    else    // 2D
    {
        oapiRegisterPanelArea(GetAreaID(), GetRectForSize(56, 56), PANEL_REDRAW_ALWAYS, PANEL_MOUSE_IGNORE, PANEL_MAP_BGONREQUEST);
    }
    m_lastIndicatorAngle = m_initialAngle;

    m_pen0 = CreatePen(PS_SOLID, 1, RGB(224,224,224));
    m_pen1 = CreatePen(PS_SOLID, 3, RGB(164,164,164));
}

void AnalogGaugeArea::Deactivate()
{
    // clean up our resources
    DeleteObject(m_pen0);
    DeleteObject(m_pen1);

    XR1Area::Deactivate();  // let superclass clean up
}

bool AnalogGaugeArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    bool retVal = false;    // assume not re-rendered
    bool forceRedraw = ((event & PANEL_REDRAW_INIT) != 0);    
    static const double eps = 1e-2;  // sensitivity before redraw occurs; lower = more frequent redraws

    // invoke subclass method to obtain the dial angle
    double dialAngle = GetDialAngle();

    if (forceRedraw || (fabs(dialAngle - m_lastIndicatorAngle) > eps))
    {
        oapiBltPanelAreaBackground (GetAreaID(), surf);
        HDC hDC = GetDC (surf);
        DrawNeedle(hDC, 28, 28, 26.0, dialAngle);
        ReleaseDC(surf, hDC);
        retVal = true;
    }

    return retVal;
}

// angle = needle angle
// speed = how fast the dial moves in radians per second.  Default = PI radians per second, or 180 degrees per second
void AnalogGaugeArea::DrawNeedle(HDC hDC, int x, int y, double rad, double angle, double speed)
{
    // handle needle response delay
    double dt = oapiGetSimStep();  // delta time since last frame
    if ((fabs(angle - m_lastIndicatorAngle) / dt) >= speed)
    {
        if (angle > m_lastIndicatorAngle)
            angle = m_lastIndicatorAngle + (speed * dt);
        else
            angle = m_lastIndicatorAngle - (speed * dt);
    }

    m_lastIndicatorAngle = angle;

    double dx = rad * cos(angle), dy = rad * sin(angle);
    HGDIOBJ oldObject = SelectObject(hDC, m_pen1);
    MoveToEx(hDC, x, y, 0); 
    LineTo(hDC, x + static_cast<int>(0.85*dx+0.5), y - static_cast<int>(0.85*dy+0.5));
    
    SelectObject(hDC, m_pen0);
    MoveToEx(hDC, x, y, 0); 
    LineTo(hDC, x + static_cast<int>(dx+0.5), y - static_cast<int>(dy+0.5));

    SelectObject(hDC, oldObject);    // clean up
}

//----------------------------------------------------------------------------------

// 2D-only for now
// indicatorAreaID = areaID of status light, etc.  -1 = none.
ToggleSwitchArea::ToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID) :
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

// isOn = reference to status variable: true = light on, false = light off
LEDArea::LEDArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const bool &isOn) :
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

// doorStatus = ptr to status enum: DoorStatus::DOOR_OPEN, DoorStatus::DOOR_CLOSED, DoorStatus::DOOR_OPENING, DoorStatus::DOOR_CLOSING 
// surfaceIDB = resource ID of source surface
// pAnimationState = ptr to animation state (0...1).  May be null.
DoorIndicatorArea::DoorIndicatorArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID, const DoorStatus *pDoorStatus, const int surfaceIDB, const double *pAnimationState) :
    XR1Area(parentPanel, panelCoordinates, areaID, meshTextureID),
    m_pDoorStatus(pDoorStatus), m_surfaceIDB(surfaceIDB), m_isTransitVisible(true), m_transitIndex(-1), m_pAnimationState(pAnimationState), m_transitColor(0) // black transit color for now
{
}

void DoorIndicatorArea::Activate()
{
    Area::Activate();  // invoke superclass method
    if (IsVC())
    {
        oapiVCRegisterArea (GetAreaID(), GetRectForSize(43, 31), PANEL_REDRAW_USER, PANEL_MOUSE_IGNORE, PANEL_MAP_BACKGROUND, GetVCPanelTextureHandle());
    }
    else  // 2D
    {
        oapiRegisterPanelArea(GetAreaID(), GetRectForSize(43, 31), PANEL_REDRAW_USER, PANEL_MOUSE_IGNORE, PANEL_MAP_BACKGROUND);
    }

    m_mainSurface = CreateSurface(m_surfaceIDB);

    TriggerRedraw();    // render initial setting
}

// Note: for the XR1, this is also invoked for Redraw3D via the default behavior
bool DoorIndicatorArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    int yCoordToPaint = -1;      // Y coordinate of texture to paint; -1 = do not paint
    switch (*m_pDoorStatus)
    {
    case DoorStatus::DOOR_CLOSED:
        yCoordToPaint = 4;
        m_isTransitVisible = true;  // reset so "Transit" will always be visible for at least a little bit when you first click the switch
        break;

    case DoorStatus::DOOR_OPENING:
    case DoorStatus::DOOR_CLOSING:
        if (m_isTransitVisible)     // only paint if "Transit" is supposed to be visible
            yCoordToPaint = 13;
        break;

    case DoorStatus::DOOR_OPEN:
        yCoordToPaint = 22;
        m_isTransitVisible = true;  // reset 
        break;

    default:    // should never happen!
        return false;
    }

    // {YYY} resolve this for the XR2
    // NOTE: if in the VC, skip over first part of src texture and shorten the width to adjust for the smaller display area in the VC
    const int srcX = (IsVC() ? 7 : 0);
    const int width = 43 - (srcX * 2);  // skip trailing trim as well

    if (yCoordToPaint >= 0)   // should we repaint the text line?
    {
        //      tgt,  src,           tX, tY,            srcX, srcY,          w,     h
        DeltaGliderXR1::SafeBlt(surf, m_mainSurface, 0,  yCoordToPaint, srcX, yCoordToPaint, width, 6); 
    }

    // repaint the transit indicator lines IF door is not fully open or closed
    if (m_transitIndex >= 0)
    {
        const int w=3, h=1;
        
        // left-hand bar
        oapiColourFill(surf, m_transitColor, 4,  m_transitIndex, w, h); // three pixels wide, one pixel high

        // right-hand bar
        oapiColourFill(surf, m_transitColor, 37, m_transitIndex, w, h); // three pixels wide, one pixel high
    }

    return true;
}

// Blink "Transit"
void DoorIndicatorArea::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // compute door transit index if we have animation in progress
    if (m_pAnimationState != nullptr)
    {
        int newTransitIndex = 2 + static_cast<int>(27 * *m_pAnimationState);    // pixel index
        if (newTransitIndex != m_transitIndex)  // has it changed?
        {
            m_transitIndex = newTransitIndex;
            TriggerRedraw();    // repaint it
        }
    }

    if (*m_pDoorStatus >= DoorStatus::DOOR_CLOSING) // in transit?
    {
        m_transitColor = BRIGHT_YELLOW; 
        const double blinkDelay = 0.75;   // blink once every 3/4-second
        double modTime = fmod(simt, blinkDelay);
        if (m_isTransitVisible) // "Transit" currently visible?
        {
            // see if it's time to blank it
            if (modTime < (blinkDelay/2))
            {
                // signal redraw method to leave area blank
                m_isTransitVisible = false;   
                TriggerRedraw();
            }
        }
        else   // "Transit" currently invisible
        {
            // see if it's time to show it
            if (modTime >= (blinkDelay/2))
            {
                // signal redraw method to repaint "Transit"
                m_isTransitVisible = true;
                TriggerRedraw();
            }
        }
    }
    else    // door not in transit
    {
        /* Do not render bars if door fully open or closed; it is cleaner that way.
        DWORD oldTransitColor = m_transitColor;
        m_transitColor = BRIGHT_GREEN;
        
        // since we don't know which animation state corresponds to which option (open/closed), just "round" the current transit index to one edge
        int newTransitIndex = ((m_transitIndex >= 15) ? 29 : 2);
        if ((newTransitIndex != m_transitIndex) || (oldTransitColor != m_transitColor))   // has pixel index or color changed?
        {
            m_transitIndex = newTransitIndex;
            TriggerRedraw();    // repaint
        }*/

        m_transitIndex = -1;        // do not render transit index when door open or closed
        TriggerRedraw();
    }
}

//----------------------------------------------------------------------------------

// sizeX, sizeY = size of bar
// maxValue = bar value for 100%
// orientation = HORIZONTAL or VERTICAL; default = HORIZONTAL
BarArea::BarArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, int sizeX, int sizeY, ORIENTATION orientation) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_sizeX(sizeX), m_sizeY(sizeY), m_orientation(orientation)
{
}

void BarArea::Activate()
{
    Area::Activate();  // invoke superclass method
    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(m_sizeX, m_sizeY), PANEL_REDRAW_ALWAYS, PANEL_MOUSE_IGNORE, PANEL_MAP_BGONREQUEST);

    m_lastRenderData.Reset();  // force a repaint on first call to Redraw
}

bool BarArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    bool retVal = false;

    // invoke subclass method to obtain color and indexY data for each gauge
    RENDERDATA renderData = GetRenderData();

    if (renderData != m_lastRenderData)
    {
        // remember this so we don't re-render each time
        m_lastRenderData = renderData;  // byte-for-byte copy

        // NOTE: 0 <= brightIndex <= darkIndex
        const int brightIndex = renderData.GetIndex(BARPORTION::BRIGHT);  // first part of bar
        const int darkIndex   = renderData.GetIndex(BARPORTION::DARK);    // second part of bar

        // reset background
        oapiBltPanelAreaBackground(GetAreaID(), surf);

        // now paint the bar IF there is anything to paint
        // NOTE: cannot pass barSize of 0 here, or entire width is painted!
        if ((brightIndex > 0) || (darkIndex > 0))  // anything on the gauge at all?
        {
            // create the color based on the enum
            DWORD color;
            DWORD darkColor;
            switch (renderData.color)
            {
            case COLOR::GREEN:
                color = BRIGHT_GREEN;
                darkColor = MEDIUM_GREEN;
                break;

            case COLOR::RED:
                color = BRIGHT_RED;
                darkColor = MEDB_RED;
                break;

            case COLOR::YELLOW:
                color = BRIGHT_YELLOW;
                darkColor = MEDIUM_YELLOW;
                break;

            case COLOR::WHITE:
                color = BRIGHT_WHITE;
                darkColor = OFF_WHITE192;
                break;

            default:
                color = darkColor = 0;   // something is wrong with the code, so paint it black to let the user know
                break;
            }

            // Note: we cannot use '0' for any width, or the entire area is painted.
            // Therefore, we use SafeColorFill.
            if (m_orientation == ORIENTATION::HORIZONTAL)
            {
                // horizontal                                  X            Y  width        height                
                DeltaGliderXR1::SafeColorFill(surf, color,     0,           0, brightIndex, m_sizeY);  // first part  (bright)
                DeltaGliderXR1::SafeColorFill(surf, darkColor, brightIndex, 0, (darkIndex - brightIndex), m_sizeY);  // second part (dark)
            }
            else
            {
                // vertical                                    X   Y                         width    height
                DeltaGliderXR1::SafeColorFill(surf, color,     0, (m_sizeY - brightIndex),   m_sizeX, brightIndex);                // bottom (first) part: bright
                DeltaGliderXR1::SafeColorFill(surf, darkColor, 0, (m_sizeY - darkIndex),     m_sizeX, (darkIndex - brightIndex));  // top (second) part: dark
            }
        }

        // invoke the post-drawing hook in case the subclass wants to overlay something
        RedrawAfterHook(event, surf);

        retVal = true;
    }

    return retVal;
}

//----------------------------------------------------------------------------------

// sizeInChars = # of characters in area to be painted
// e.g., "232.3": 4*7+3 = 31 wide, 9 high : sizeInChars = 4, hasDecimal=true
// fontResourceID: e.g., IDB_FONT2 (green version)
NumberArea::NumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, int sizeInChars, bool hasDecimal) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_sizeInChars(sizeInChars), m_hasDecimal(hasDecimal), m_font2Yellow(0), m_font2Red(0), m_font2Blue(0), m_font2White(0)
{
    m_pRenderData = new RENDERDATA(sizeInChars + (hasDecimal ? 1 : 0));
}

NumberArea::~NumberArea()
{
    delete m_pRenderData;
}

void NumberArea::Activate()
{
    Area::Activate();  // invoke superclass method
    int sizeX = (m_sizeInChars * 7) + (m_hasDecimal ? 3 : 0);
    
    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(sizeX, 9), PANEL_REDRAW_ALWAYS, PANEL_MOUSE_IGNORE, PANEL_MAP_BGONREQUEST);

    m_mainSurface = CreateSurface(IDB_FONT2);  // our special numeric font (green)
    m_font2Yellow = CreateSurface(IDB_FONT2_YELLOW);
    m_font2Red    = CreateSurface(IDB_FONT2_RED);
    m_font2Blue   = CreateSurface(IDB_FONT2_BLUE);
    m_font2White  = CreateSurface(IDB_FONT2_WHITE);

    // force a repaint and defult to normal color
    m_pRenderData->Reset();
}

void NumberArea::Deactivate()
{
    DestroySurface(&m_font2Yellow);
    DestroySurface(&m_font2Red);
    DestroySurface(&m_font2Blue);
    DestroySurface(&m_font2White);
    XR1Area::Deactivate();  // let superclass clean up
}

bool NumberArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    // invoke subclass method to update the render data
    bool redraw = UpdateRenderData(*m_pRenderData);

    if (redraw)   // has value changed?
    {
        // NOTE: no need to render background here; we will overwrite the entire area

        // each char is 7x9, except for '.' (last in the bitmap) which is 5x9
        // Order is: 0 1 2 3 4 5 6 7 8 9 ' ' .
        
        char *num = m_pRenderData->pStrToRender;
        int x = 0;  // X coordinate of next character render
        for (; *num; num++)
        {     
            int srcX;           // X coord into font2.bmp
            int charWidth = 7;  // assume normal char
            char c = *num;
            switch (c)
            {
            case '-':
                srcX = 70;
                break;

            case ' ':   // blank space
                srcX = 77;
                break;

            case '.':   // special narrow '.' char
                srcX = 84;
                charWidth = 3;
                break;

            default:    // 0-9 digit
                srcX = (c - '0') * 7; // each digit is 7 pixels wide with spacing
            }

            SURFHANDLE srcSurface;
            switch (m_pRenderData->color)
            {
            case COLOR::RED:
                srcSurface = m_font2Red;
                break;

            case COLOR::YELLOW:
                srcSurface = m_font2Yellow;
                break;

            case COLOR::BLUE:
                srcSurface = m_font2Blue;
                break;

            case COLOR::WHITE:
                srcSurface = m_font2White;
                break;

            default:    // GREEN
                srcSurface = m_mainSurface;
                break;
            }

            // render separating spaces as well just in case anything underneath (since the font can vary in width now)
            //                                  srcX,srcY,width,   height
            DeltaGliderXR1::SafeBlt (surf, srcSurface, x, 0, srcX, 0, charWidth, 9);
            x += charWidth; // set up for next character
        }
    }

    return redraw;
}

//----------------------------------------------------------------------------------

// 'percentage' horizontal gauge area; may be single or dual, and may render in green, red, or both
// registered area is either 6 pixels high (single) or 13 pixels high (dual)
// sizeX = horizontal size of gauge in pixels
// redrawFlag = PANEL_REDRAW_USER, PANEL_REDRAW_ALWAYS, etc.
// deltaX, deltaY = offset from area start to draw pointer; default is 0
// gapSize = gap (in pixels) between dual indicators; default=1.  Has no effect on single-indicator gauges.
PctHorizontalGaugeArea::PctHorizontalGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const bool isDual, const int sizeX, 
                                         const int redrawFlag, const int meshTextureID, const int deltaX, const int deltaY, const int gapSize) :
    HorizontalGaugeArea(parentPanel, panelCoordinates, areaID, isDual, sizeX, redrawFlag, meshTextureID, deltaX, deltaY, gapSize)
{
}

// side = LEFT or RIGHT
HorizontalGaugeArea::RENDERDATA PctHorizontalGaugeArea::GetRenderData(const SIDE side)
{
    // get the % to move on the gauge from the subclass
    COLOR color;
    double fraction = GetFraction(side, color);        // 0...1
    
    // compute pixel
    const int maxIndex = 84;    // total width = 85 pixels (index 0-84, inclusive)
    int index = static_cast<int>((maxIndex * fraction) + 0.5);  // round to nearest pixel
    
    // set up RenderData
    return _RENDERDATA(color, index);
}

//-------------------------------------------------------------------------

ThrustNumberArea::ThrustNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    NumberArea(parentPanel, panelCoordinates, areaID, 6, true)   // 6 chars plus decimal
{
}

bool ThrustNumberArea::UpdateRenderData(RENDERDATA &renderData)
{
    bool redraw = false;
    const bool forceRedraw = renderData.forceRedraw;

    double thrust = GetThrust();  // retrieve from subclass (in kN)

    // no need to round here; sprintf will do it for us

    // check whether the value has changed since the last render
    if (forceRedraw || (thrust != renderData.value))
    {
        // Value has changed -- let's redo the string and see if that is different as well.
        // The goal here is to be as efficient as possible and only re-render when we absolutely have to.
        char pTemp[10];
        // ensure that value is in range
        if (thrust > 999999)
            thrust = 999999;   // trim to 6 digits
        else if (thrust < 0)
            thrust = 0;       // thrust cannot be negative!

        // note: pFormatStr must evaluate to exactly 7 characters for each case
        const char *pFormatStr;
        if (thrust > 99999.9)
            pFormatStr = "%6.0f.";
        else if (thrust > 9999.99)
            pFormatStr = "%5.1f";
        else if (thrust > 999.999)
            pFormatStr = "%4.2f";
        else if (thrust > 99.9999)
            pFormatStr = "%3.3f";
        else if (thrust > 9.99999)
            pFormatStr = "%2.4f";
        else  // <= 9.99999
            pFormatStr = "%1.5f";

        sprintf(pTemp, pFormatStr, thrust); 
        if (forceRedraw || (strcmp(pTemp, renderData.pStrToRender) != 0))
        {
            // text has changed; signal the base class to render it
            renderData.value = thrust;   // remember for next time
            strcpy(renderData.pStrToRender, pTemp);
            redraw = true;
            renderData.forceRedraw = false;  // clear reset request
        }
    }

    return redraw;
}

//-------------------------------------------------------------------------

AccNumberArea::AccNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const AXIS axis) :
    NumberArea(parentPanel, panelCoordinates, areaID, 6, true),   // 6 chars plus decimal
    m_axis(axis)
{
}
bool AccNumberArea::UpdateRenderData(RENDERDATA &renderData)
{
    bool redraw = false;
    const bool forceRedraw = renderData.forceRedraw;

    // use shared acceleration values set once per frame by ComputeAccPostStep object
    const VECTOR3 &A = GetXR1().m_acceleration;
    double acc;
    switch (m_axis)
    {
    case AXIS::X:
        acc = A.x;
        break;

    case AXIS::Y:
        acc = A.y;
        break;

    case AXIS::Z:
        acc = A.z;
        break;
    }

    // round acc to nearest 1/1000th
    acc = (static_cast<double>(static_cast<int>((acc + 0.0005) * 1000))) / 1000;

    // check whether the value has changed since the last render
    if (forceRedraw || (acc != renderData.value))
    {
        // Value has changed -- let's redo the string and see if that is different as well.
        // The goal here is to be as efficient as possible and only re-render when we absolutely have to.
        char pTemp[10];   // add 2 extra chars in case we go way high on one frame for some reason
        // ensure that value is in range
        if (acc > 99.999)
            acc = 99.999;   // trim to 2 leading digits + possible minus sign
        else if (acc < -99.999)
            acc = -99.999;
        sprintf(pTemp, "%7.3f", acc); 
        if (forceRedraw || (strcmp(pTemp, renderData.pStrToRender) != 0))
        {
            // text has changed; signal the base class to render it
            renderData.value = acc;   // remember for next time
            strcpy(renderData.pStrToRender, pTemp);
            redraw = true;
            renderData.forceRedraw = false;  // clear reset request
        }
    }

    return redraw;
}

//----------------------------------------------------------------------------------

// acceleration horizontal gauge area
// 91 pixels wide because gauge is 85 pixels, plus we need six extra pixels (three per side) for the triangle to stick out over the edges
// maxG = maximum G value on gauge
AccHorizontalGaugeArea::AccHorizontalGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const AXIS axis, const bool isDual, const SIDE side, const int meshTextureID) :
    HorizontalGaugeArea(parentPanel, panelCoordinates, areaID, isDual, 91, PANEL_REDRAW_ALWAYS, meshTextureID, 0, 0, -1, side),  // gap of 0 for dual indicator
    m_axis(axis)
{
}

// override Activate so we can use a bright green arrow instead
void AccHorizontalGaugeArea::Activate()
{
    HorizontalGaugeArea::Activate();  // invoke superclass method
    DestroySurface(&m_mainSurface);

    m_mainSurface = CreateSurface(IDB_GREEN_INDICATOR2);  // bright green arrow
    DWORD white = 0xFFFFFF;           // set WHITE as transparent color; BLACK does not work for some reason!
    SetSurfaceColorKey(m_mainSurface, white);
}

// side = TOP or BOTTOM
HorizontalGaugeArea::RENDERDATA AccHorizontalGaugeArea::GetRenderData(const SIDE side)
{
    // use shared acceleration values set once per frame by ComputeAccPostStep object
    const VECTOR3 &A = GetXR1().m_acceleration;
    double acc;
    switch (m_axis)
    {
    case AXIS::X:
        acc = A.x;
        break;

    case AXIS::Y:
        acc = A.y;
        break;

    case AXIS::Z:
        acc = A.z;
        break;
    }

    const double gravities = acc / G;     // acc in Gs
    double fraction = gravities / GetXR1().m_maxGaugeAcc;

    // to keep display clean, round fraction to nearest .001
    fraction = static_cast<double>(static_cast<int>((fraction + .0005) * 1000)) / 1000;
    double absFraction = fabs(fraction);
    bool isNegative = fraction < 0;
    
    COLOR color;
    if (absFraction > 1.0)
    {
        absFraction = 1.0;     // over-G!  Render in YELLOW.
        color = COLOR::YELLOW;
    }
    else    // in range
        color = (isNegative ? COLOR::RED : COLOR::GREEN);

    // compute pixel
    const int maxIndex = 84;    // total width = 85 pixels (index 0-84, inclusive)
    int index = static_cast<int>((maxIndex * absFraction) + 0.5);  // round to nearest pixel

    // do not round pixels here if close to either edge

    // set up RenderData
    return _RENDERDATA(color, index);
}

//-------------------------------------------------------------------------

AccScaleArea::AccScaleArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_accScale(AccScale::NONE)
{
}

void AccScaleArea::Activate()
{
    Area::Activate();  // invoke superclass method
    m_mainSurface = CreateSurface(IDB_GSCALE);
    
    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(92, 11), PANEL_REDRAW_ALWAYS, PANEL_MOUSE_IGNORE, PANEL_MAP_NONE);

    m_accScale = AccScale::NONE;  // force a redraw
}

bool AccScaleArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    bool redraw = false;

    // check whether the required ACC scale has changed since last render
    // also, don't render until the poststep has had a chance to run at least once
    if ((GetXR1().m_accScale != AccScale::NONE) && (GetXR1().m_accScale != m_accScale))
    {
        // need to redraw it
        // Y coordinate
        int y = static_cast<int>(GetXR1().m_accScale) * 11;       // 11 pixels high per row

        // redraw the entire area
        //                          tgX,tgY,srcX,srcY,width,height
        DeltaGliderXR1::SafeBlt(surf, m_mainSurface, 0, 0, 0, y, 92, 11);
        //oapiColourFill(surf, BRIGHT_YELLOW);

        redraw = true;
        m_accScale = GetXR1().m_accScale;     // remember new scale
    }
    
    return redraw;
}

//-------------------------------------------------------------------------

// utility method to retrieve a Windows COLORREF for a given temperature
// This is used by both the temperature MDM and the pop-up HUD, so it is defined here in the base class.
COLORREF XR1Area::GetTempCREF(const double tempK, double limitK, const DoorStatus doorStatus) const
{
    if (doorStatus != DoorStatus::DOOR_CLOSED)
        limitK = GetXR1().m_hullTemperatureLimits.doorOpen;  // we have a door open; lower the limit

    double warningTemp  = limitK * GetXR1().m_hullTemperatureLimits.warningFrac;
    double criticalTemp = limitK * GetXR1().m_hullTemperatureLimits.criticalFrac;

    COLORREF retVal;
    if (tempK >= limitK)
        retVal = CREF(BRIGHT_WHITE);
    else if (tempK >= criticalTemp)
        retVal = CREF(BRIGHT_RED);
    else if (tempK >= warningTemp)
        retVal = CREF(BRIGHT_YELLOW);
    else
        retVal = CREF(BRIGHT_GREEN);

    return retVal;
}

// utility method to retrieve a Windows COLORREF for a given value
COLORREF XR1Area::GetValueCREF(double value, double warningLimit, double criticalLimit) const
{
    COLORREF retVal;
    if (value >= criticalLimit)
        retVal = CREF(BRIGHT_RED);
    else if (value >= warningLimit)
        retVal = CREF(BRIGHT_YELLOW);
    else
        retVal = CREF(BRIGHT_GREEN);

    return retVal;
}

//-------------------------------------------------------------------------

// lighted button area that is lit as long as the mouse button is held down
// buttonMeshGroup = mesh group for 3D button; default == -1 (no VC button)
MomentaryButtonArea::MomentaryButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int buttonMeshGroup) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_buttonMeshGroup(buttonMeshGroup), m_isLit(false), m_buttonDownSimt(-1)
{
}

void MomentaryButtonArea::Activate()
{
    Area::Activate();  // invoke superclass method
    m_mainSurface = CreateSurface(IDB_LIGHT2);                  // lighted green button

    if (IsVC()) // 3D panel?
    {
        oapiVCRegisterArea(GetAreaID(), PANEL_REDRAW_USER | PANEL_REDRAW_MOUSE, PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBPRESSED | PANEL_MOUSE_LBUP);
    }
    else    // 2D panel
    {
        oapiRegisterPanelArea(GetAreaID(), GetRectForSize(12, 12), PANEL_REDRAW_USER | PANEL_REDRAW_MOUSE, PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBPRESSED | PANEL_MOUSE_LBUP);
    }
}

bool MomentaryButtonArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    int srcX = (IsLit() ? 12 : 0);    // texture X coord; 12 = lit, 0 = not lit
    DeltaGliderXR1::SafeBlt(surf, m_mainSurface, 0, 0, srcX, 0, 12, 12);

    return true;
}

bool MomentaryButtonArea::Redraw3D(const int event, const SURFHANDLE surf)
{
    static const float tv0[8] = {0,0,0.0469f,0.0469f,0,0,0.0469f,0.0469f};

    if ((GetXR1().vcmesh == nullptr) || (m_buttonMeshGroup < 0))
        return false;       // nothing to draw

    static NTVERTEX vtx[8];   // this is OK because Orbiter is single-threaded; i.e., only one XR vessel is active at a time
    float ofs = (IsLit() ? 0.0469f : 0);
    GROUPEDITSPEC ges;
    ges.flags = GRPEDIT_VTXTEXV;
    ges.nVtx = 8;
    ges.vIdx = nullptr;
    ges.Vtx = vtx;

    for (int j = 0; j < 8; j++)
	    vtx[j].tv = tv0[j]+ofs;
    oapiEditMeshGroup (GetXR1().vcmesh, m_buttonMeshGroup, &ges);
    
    return true;
}

bool MomentaryButtonArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    // check whether button was just pressed
    if (event & PANEL_MOUSE_LBDOWN)
    {
        m_buttonDownSimt = GetAbsoluteSimTime();    
        m_isLit = true;
    }

    // let the subclass take some action base on the click/hold action
    ProcessButtonAction(event, m_buttonDownSimt); 

    // check whether button was just unpressed
    if (event & PANEL_MOUSE_LBUP)
    {
        m_buttonDownSimt = -1;
        m_isLit = false;
    }

    return true;
}

//-------------------------------------------------------------------------

// lighted button area whose raw mouse events are passed to the sublcass
// buttonMeshGroup = mesh group for 3D button; default == -1 (no VC button)
RawButtonArea::RawButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int buttonMeshGroup) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_buttonMeshGroup(buttonMeshGroup), m_buttonDownSimt(-1)
{
}

void RawButtonArea::Activate()
{
    Area::Activate();  // invoke superclass method
    m_mainSurface = CreateSurface(IDB_LIGHT2);                  // lighted green button

    if (IsVC()) // 3D panel?
    {
        oapiVCRegisterArea(GetAreaID(), PANEL_REDRAW_USER | PANEL_REDRAW_MOUSE, PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBPRESSED | PANEL_MOUSE_LBUP);
    }
    else    // 2D panel
    {
        oapiRegisterPanelArea(GetAreaID(), GetRectForSize(12, 12), PANEL_REDRAW_USER | PANEL_REDRAW_MOUSE, PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBPRESSED | PANEL_MOUSE_LBUP);
    }
}

bool RawButtonArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    int srcX = (IsLit() ? 12 : 0);    // texture X coord; 12 = lit, 0 = not lit
    DeltaGliderXR1::SafeBlt(surf, m_mainSurface, 0, 0, srcX, 0, 12, 12);

    return true;
}

bool RawButtonArea::Redraw3D(const int event, const SURFHANDLE surf)
{
    static const float tv0[8] = {0,0,0.0469f,0.0469f,0,0,0.0469f,0.0469f};

    if ((GetXR1().vcmesh == nullptr) || (m_buttonMeshGroup < 0))
        return false;       // nothing to draw

    static NTVERTEX vtx[8];   // this is OK because Orbiter is single-threaded; i.e., only one XR vessel is active at a time
    float ofs = (IsLit() ? 0.0469f : 0);
    GROUPEDITSPEC ges;
    ges.flags = GRPEDIT_VTXTEXV;
    ges.nVtx = 8;
    ges.vIdx = nullptr;
    ges.Vtx = vtx;

    for (int j = 0; j < 8; j++)
	    vtx[j].tv = tv0[j]+ofs;
    oapiEditMeshGroup (GetXR1().vcmesh, m_buttonMeshGroup, &ges);

    return true;
}

bool RawButtonArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    // check whether button was just pressed
    if (event & PANEL_MOUSE_LBDOWN)
    {
        // record button down time
        m_buttonDownSimt = GetAbsoluteSimTime();    
    }

    // let the subclass take some action based on the click/hold action
    ProcessButtonAction(event, m_buttonDownSimt); 

    return true;
}

//-------------------------------------------------------------------------

// base class for all timer number areas
TimerNumberArea::TimerNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int sizeInChars, TIMEUNITS timeUnits, NumberArea::COLOR color) :
    NumberArea(parentPanel, panelCoordinates, areaID, sizeInChars, false),   // no decimal
    m_timeUnits(timeUnits), m_color(color)
{
    switch (timeUnits)
    {
    case TIMEUNITS::DAYS:
        m_unitsInDay = 1;
        m_maxValue = 9999;
        break;

    case TIMEUNITS::HOURS:
        m_unitsInDay = 24;
        m_maxValue = 23;
        break;

    case TIMEUNITS::MINUTES:
        m_unitsInDay = 24.0 * 60;
        m_maxValue = 59;
        break;

    case TIMEUNITS::SECONDS:
        m_unitsInDay = 24.0 * 60 * 60;
        m_maxValue = 59;
        break;

    default:        // should never happen!
        m_unitsInDay = 1e-6;     // update each timestep so we see something is wrong
        break;
    }
}

bool TimerNumberArea::UpdateRenderData(RENDERDATA &renderData)
{
    bool redraw = false;
    const bool forceRedraw = renderData.forceRedraw;

    // invoke the subclass to return elapsed time in DAYS
    double time = GetTime();

    // render the string via a base class method
    redraw = RenderTimeValue(renderData, time);

    // render in the requested color
    renderData.color = m_color;

    return redraw;
}

// elapsedTime is in DAYS here
bool TimerNumberArea::RenderTimeValue(RENDERDATA &renderData, double time)
{
    bool redraw = false;
    const bool forceRedraw = renderData.forceRedraw;

    int value;
    if (m_timeUnits == TIMEUNITS::DAYS)
    {
        value = static_cast<int>(time);
    }
    else    // hours, minutes, or seconds
    {
        // compute the elapsed UNIT, rounded DOWN
        double elapsedUnitsTotal = time * m_unitsInDay;
        double elapsedUnitsInCurrentDay = fmod(elapsedUnitsTotal, m_unitsInDay);  // 0....(m_unitsInDay-1)
        value = static_cast<int>(elapsedUnitsInCurrentDay) % (m_maxValue + 1);   // 0...m_maxValue
    }

    // check whether the value has changed since the last render
    if (forceRedraw || (value != renderData.value))
    {
        char temp[10];

        // Value has changed -- since we are an integer value, the string will always be different as well

        // ensure that value is in range
        if (value > m_maxValue)
            value = m_maxValue; 
        else if (value < 0)     // sanity check
            value = 0;

        if (m_sizeInChars == 4)     // days?
            sprintf(temp, "%4d", value); 
        else        // hours, minutes, or seconds
            sprintf(temp, "%02d", value); 

        // signal the base class to render the text
        renderData.value = value;   // remember for next time
        strcpy(renderData.pStrToRender, temp);
        redraw = true;
        renderData.forceRedraw = false;  // clear reset request
    }

    return redraw;
}

//-------------------------------------------------------------------------

// mdjStartTime = MJD that timer started running
// NOTE: if mjdStartTime is set to -1 while timer is running, timer is STOPPED and RESET here automatically; client classes need only set mdjStartTime=-1.
MJDTimerNumberArea::MJDTimerNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, bool &isRunning, const int sizeInChars, TIMEUNITS timeUnits, const double &mjdStartTime) :
    TimerNumberArea(parentPanel, panelCoordinates, areaID, sizeInChars, timeUnits),
    m_mjdStartTime(mjdStartTime), m_lastRenderedMJD(-1), m_isRunning(isRunning)
{
}

// returns: elapsed time in days
double MJDTimerNumberArea::GetTime()
{
    // check whether timer is reset
    if (m_mjdStartTime < 0)
    {
        m_isRunning = false;     // stop timer if still running
        m_lastRenderedMJD = -1;  // force retVal to be 0.0 below
    }
    else if (m_isRunning)  // update MJD to render if timer is running; otherwise it is paused or stopped 
    {
        // update MJD time to be rendered this frame
        m_lastRenderedMJD = oapiGetSimMJD();
    }

    // compute the elapsed time since timer start
    double retVal = max(m_lastRenderedMJD - m_mjdStartTime, 0.0);   // if negative delta, set to 0.0

    return retVal;
}

//-------------------------------------------------------------------------

ElapsedTimerNumberArea::ElapsedTimerNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, bool &isRunning, const int sizeInChars, TIMEUNITS timeUnits, double &elapsedTime) :
    TimerNumberArea(parentPanel, panelCoordinates, areaID, sizeInChars, timeUnits),
    m_elapsedTime(elapsedTime), m_isRunning(isRunning)
{
}

// returns: elapsed time in days
double ElapsedTimerNumberArea::GetTime()
{
    double retVal;

    // check whether timer is reset
    if (m_elapsedTime < 0)
    {
        m_isRunning = false;  // stop timer if still running
        retVal = 0;
    }
    else // timer running normally
    {
        retVal = m_elapsedTime;
    }

    return retVal;
}

//-------------------------------------------------------------------------

// sizeX, sizeY = size of bar
// resourceID = texture to use for bar
LargeBarArea::LargeBarArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, int sizeX, int sizeY, int resourceID, int darkResourceID) :
    BarArea(parentPanel, panelCoordinates, areaID, sizeX, sizeY, ORIENTATION::VERTICAL),
    m_resourceID(resourceID), m_darkResourceID(darkResourceID), m_darkSurface(nullptr)
{
}

void LargeBarArea::Activate()
{
    Area::Activate();  // invoke superclass method
    m_mainSurface = CreateSurface(m_resourceID);   
    
    if (m_darkResourceID > 0)  // dark resource is optional
        m_darkSurface = CreateSurface(m_darkResourceID);

    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(m_sizeX, m_sizeY), PANEL_REDRAW_ALWAYS, PANEL_MOUSE_IGNORE, PANEL_MAP_BGONREQUEST);

    m_lastRenderData.Reset();  // force a repaint on first call to Redraw
}

void LargeBarArea::Deactivate()
{
    if (m_darkSurface != nullptr)
        DestroySurface(&m_darkSurface);  // free our dark surface as well

    BarArea::Deactivate();           // invoke superclass method to free main surface
}

// override the base method so we will render a texture rather than a colored bar
bool LargeBarArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    bool retVal = false;

    // invoke subclass method to obtain color and indexY data for each gauge
    RENDERDATA renderData = GetRenderData();

    if (renderData != m_lastRenderData)
    {
        // remember this so we don't re-render each time
        m_lastRenderData = renderData;  // byte-for-byte copy

        // NOTE: 0 <= brightIndex <= darkIndex
        const int brightIndex = renderData.GetIndex(BARPORTION::BRIGHT);  // first part of bar
        const int darkIndex   = renderData.GetIndex(BARPORTION::DARK);    // second part of bar

        // reset background
        oapiBltPanelAreaBackground(GetAreaID(), surf);

        // render portions of both textures as necessary: bright and dark
        const int brightYCoord = (m_sizeY - brightIndex);
        const int darkYCoord = (m_sizeY - darkIndex);
        //            texture        tgtX  tgtY       srcX  srcY       width    height
        DeltaGliderXR1::SafeBlt(surf, m_mainSurface, 0, brightYCoord, 0, brightYCoord, m_sizeX, brightIndex);                // bottom (first) part: bright
        
        if (m_darkSurface != nullptr)  // any dark surface defined?
        {
            //            texture        tgtX  tgtY       srcX  srcY       width    height
            DeltaGliderXR1::SafeBlt(surf, m_darkSurface, 0, darkYCoord,   0, darkYCoord,   m_sizeX, (darkIndex - brightIndex));  // top (second) part: dark
        }
        
        // invoke the post-drawing hook in case the subclass wants to overlay something
        RedrawAfterHook(event, surf);

        retVal = true;
    }

    return retVal;
}


//----------------------------------------------------------------------------------

// darkResourceID: -1 = none
LargeFuelBarArea::LargeFuelBarArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, PROPELLANT_HANDLE ph, const int resourceID, const int darkResourceID, const double gaugeMinValue) :
    LargeBarArea(parentPanel, panelCoordinates, areaID, 49, 141, resourceID, darkResourceID),   // 49 wide x 141 high
    m_maxFuelQty(-1), m_pFuelRemaining(nullptr), m_propHandle(ph), m_gaugeMinValue(gaugeMinValue)
{
}


LargeFuelBarArea::LargeFuelBarArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, double maxFuelQty, const double *pFuelRemaining, const int resourceID, const int darkResourceID, const double gaugeMinValue) :
    LargeBarArea(parentPanel, panelCoordinates, areaID, 49, 141, resourceID, darkResourceID),   // 49 wide x 141 high, VERTICAL orientation
    m_maxFuelQty(maxFuelQty), m_pFuelRemaining(pFuelRemaining), m_propHandle(nullptr), m_gaugeMinValue(gaugeMinValue)
{
}

BarArea::RENDERDATA LargeFuelBarArea::GetRenderData()
{
    double maxPropMass;
    double totalPropMass;
    double startingDarkValue;

    if (m_propHandle != nullptr)
    {
        // propellant resource
        maxPropMass = GetXR1().GetXRPropellantMaxMass(m_propHandle);  // includes the bay qty
        totalPropMass = GetXR1().GetXRPropellantMass(m_propHandle);   // includes bay qty, if any
        startingDarkValue = GetVessel().GetPropellantMass(m_propHandle);  // any qty shown over what is currently in the INTERNAL TANK must be from the BAY
    }
    else    
    {
        // non-propellant resource
        maxPropMass = m_maxFuelQty;
        totalPropMass = *m_pFuelRemaining;
        startingDarkValue = totalPropMass;  // no bay tanks for non-propellant resource
    }

    // adjust for minimum gauge value; this affects all values
    // This is currently only used for the coolant gauge
    maxPropMass -= m_gaugeMinValue;
    totalPropMass -= m_gaugeMinValue;
    startingDarkValue -= m_gaugeMinValue;

    return _RENDERDATA(COLOR::NONE, startingDarkValue, totalPropMass, maxPropMass);
}

//----------------------------------------------------------------------------------

LargeLOXBarArea::LargeLOXBarArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int resourceID, const int darkResourceID) :
    LargeBarArea(parentPanel, panelCoordinates, areaID, 49, 141, resourceID, darkResourceID)   // 49 wide x 141 high
{
}

BarArea::RENDERDATA LargeLOXBarArea::GetRenderData()
{
    const double maxLoxMass = GetXR1().GetXRLOXMaxMass();  // includes the bay qty
    const double totalLoxMass = GetXR1().GetXRLOXMass();   // includes bay qty, if any
    const double startingDarkValue = GetXR1().m_loxQty;    // internal LOX tank quantity

    return _RENDERDATA(COLOR::NONE, startingDarkValue, totalLoxMass, maxLoxMass);
}

//----------------------------------------------------------------------------------

// NOTE: fuel dump status will NOT be preserved in the save file; we never want to boot up and resume dumping fuel automatically
// fuelDumpInProgress = reference to bool flag denoting fuel dump status for a given tank
FuelDumpButtonArea::FuelDumpButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, bool &fuelDumpInProgress, const char *pFuelLabel) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_fuelDumpInProgress(fuelDumpInProgress), m_isLit(false), m_buttonDownSimt(-1), 
    m_buttonPressProcessed(false), m_isButtonDown(false)
{
    strcpy(m_fuelLabel, pFuelLabel);
}

void FuelDumpButtonArea::Activate()
{
    Area::Activate();  // invoke superclass method
    m_mainSurface = CreateSurface(IDB_LIGHT2);                  // lighted green button

    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(12, 12), PANEL_REDRAW_MOUSE, PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBPRESSED | PANEL_MOUSE_LBUP);

    // reset to NOT lit
    m_isLit = false;
    
    TriggerRedraw();
}

bool FuelDumpButtonArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    // always re-render this since it is always performed on request
    int srcX = (m_isLit ? 12 : 0);    // texture X coord; 12 = lit, 0 = not lit
    DeltaGliderXR1::SafeBlt(surf, m_mainSurface, 0, 0, srcX, 0, 12, 12);

    return true;
}

bool FuelDumpButtonArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    // check whether button was just pressed
    if (event & PANEL_MOUSE_LBDOWN)
    {
        m_buttonDownSimt = GetAbsoluteSimTime();    
        m_isLit = true;
        m_isButtonDown = true;
    }

    // main processing is here
    ProcessButtonPressed(event);

    // check whether button was just unpressed
    if (event & PANEL_MOUSE_LBUP)
    {
        m_buttonDownSimt = -1;
        
        // do not turn off button light here; our PostStep manages that

        m_isButtonDown = false;     // reset
    }

    return true;
}

void FuelDumpButtonArea::ProcessButtonPressed(const int event)
{
    if (event & (PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBUP))
        GetXR1().PlaySound(GetXR1().SwitchOn, DeltaGliderXR1::ST_Other, QUIET_CLICK);  // light click for both on and off

    if (event & PANEL_MOUSE_LBDOWN)
    {
        m_buttonPressProcessed = false;     // reset for this new press

        if (m_fuelDumpInProgress)
        {
            GetXR1().SetFuelDumpState(m_fuelDumpInProgress, false, m_fuelLabel);   // displays warning & plays callout

            // nothing more to do for this press
            m_buttonPressProcessed = true;  
            return;     
        }
    }

    if (m_buttonPressProcessed)
        return;     // ignore this event; button press already processed

    const double RESET_TIME = 2.5;      // button must be held this long to initiate fuel dump
    const double buttonHoldTime = GetAbsoluteSimTime() - m_buttonDownSimt;

    if (event & PANEL_MOUSE_LBPRESSED)
    {
        if (buttonHoldTime >= RESET_TIME)
        {
            GetXR1().SetFuelDumpState(m_fuelDumpInProgress, true, m_fuelLabel);  // displays warning & plays callout
            // Note: we cannot easily determine whether to play an error beep here since we do not know about our tank level, so just play an
            // acknowledgement beep: we will play an error beep if the tank empties (or *is* empy) in the FuelDumpPostStep.
            m_buttonPressProcessed = true;   // ignore any further events
        }
    }
    else    // button was released before fuel dump was initiated
    {
        GetXR1().ShowWarning("Hold to Dump Fuel.wav", DeltaGliderXR1::ST_WarningCallout, "You must hold down the dump&button to initiate fuel dump.");
        m_buttonPressProcessed = true;
    }
}

void FuelDumpButtonArea::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    if (m_fuelDumpInProgress)
    {
        // blink the light twice a second
        bool isLit = (fmod(simt, 0.5) < 0.25);

        if (isLit != m_isLit)
        {
            m_isLit = isLit;
            TriggerRedraw();
        }
    }
    else    // dump is NOT in progress; turn off the light if it is lit UNLESS button is down
    {
        if (m_isLit && (m_isButtonDown == false))
        {
            m_isLit = false;
            TriggerRedraw();
        }
    }
}

//----------------------------------------------------------------------------------

SupplyHatchToggleSwitchArea::SupplyHatchToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID, DoorStatus &doorStatus, const char *pHatchName, const UINT &animHandle) :
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
    const char *pState = (switchIsOn ? "open" : "closed");
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

// isOn = reference to status variable: true = light on, false = light off
DoorMediumLEDArea::DoorMediumLEDArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, DoorStatus &doorStatus, const bool redrawAlways) :
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

//----------------------------------------------------------------------------------

// switchState = ref to bool switch state
BoolToggleSwitchArea::BoolToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID, bool &switchState) :
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

//-------------------------------------------------------------------------

MassNumberArea::MassNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const bool isMetric) :
    NumberArea(parentPanel, panelCoordinates, areaID, 8, true),   // 8 chars plus decimal
    m_isMetric(isMetric)
{
}

bool MassNumberArea::UpdateRenderData(RENDERDATA &renderData)
{
    bool redraw = false;
    const bool forceRedraw = renderData.forceRedraw;

    // invoke the subclass to retrieve the mass value in KG
    double mass = GetMassInKG();

    if (m_isMetric == false)
        mass = KgToPounds(mass);

    // do not round value
    
    // check whether the value has changed since the last render
    if (forceRedraw || (mass != renderData.value))
    {
        // Value has changed -- let's redo the string and see if that is different as well.
        // The goal here is to be as efficient as possible and only re-render when we absolutely have to.
        char pTemp[15];

        if (mass > 99999999)
            mass = 99999999;
        else if (mass < 0)      // sanity-check
            mass = 0;           

        // Note: pFormatString must be exactly nine characters in length, with exactly one decimal.
        char *pFormatString;
        if (mass > 9999999.9)
            pFormatString = "%8.0lf.";  // eight because of "." appended = nine total
        else if (mass > 999999.9)
            pFormatString = "%9.1lf";   // includes the "."
        else if (mass > 99999.99)
            pFormatString = "%9.2lf";
        else 
            pFormatString = "%9.3lf";

        sprintf(pTemp, pFormatString, mass);
        if (forceRedraw || (strcmp(pTemp, renderData.pStrToRender) != 0))
        {
            // text has changed; signal the base class to render it
            renderData.value = mass;   // remember for next time
            strcpy(renderData.pStrToRender, pTemp);
            redraw = true;
            renderData.forceRedraw = false;  // clear reset request
        }
    }

    // always render in the default green
    return redraw;
}

//-------------------------------------------------------------------------

ShipMassNumberArea::ShipMassNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const bool isMetric) :
        MassNumberArea(parentPanel, panelCoordinates, areaID, isMetric)
{
}

double ShipMassNumberArea::GetMassInKG() 
{ 
    return GetVessel().GetMass(); 
}

//----------------------------------------------------------------------------------

// This is an Easter Egg to handle mouse clicks on the Altea Aerospace logo.
AlteaAerospaceArea::AlteaAerospaceArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    XR1Area(parentPanel, panelCoordinates, areaID)
{
}

void AlteaAerospaceArea::Activate()
{
    Area::Activate();  // invoke superclass method
    // OLD LOGO: (149,52)
    // NEW LOGO: (149,54)
    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(149, 54), PANEL_REDRAW_NEVER, PANEL_MOUSE_LBDOWN);
}

bool AlteaAerospaceArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    // play sound if the mouse was just clicked
    if (event & PANEL_MOUSE_LBDOWN)
        GetXR1().AlteaLogoClicked();

    return true;
}
