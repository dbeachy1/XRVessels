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

#include "DeltaGliderXR1.h"
#include "XR1HUD.h"

// ==============================================================

// Base class for all popup HUDs
//
// Constructor
// vessel = our vessel handle
// panelCoordinates = absolute coordinates of this area on the parent instrument panel
// areaID = unique Orbiter area ID
// pTextLineGroup = lines to pass to new TextBox object; if null, no TextBox will be created
// hudTurnedOn = reference to hud "on/off" switch value
PopupHUDArea::PopupHUDArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID, const int width, const int height) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_pen0(0), m_state(OnOffState::Off), m_startScrollTime(-1), m_startScrollY(-1), m_movement(0), m_hBackgroundBrush(0),
    m_width(width), m_height(height), m_colorRef(0), m_bgColorRef(0), m_hlColorRef(0),
    m_topYCoordinate(height),  // HUD is OFF (one pixel off-area)
    m_lastRenderedTopYCoordinate(-1), m_pTextBox(nullptr)
{
}

// Destructor
PopupHUDArea::~PopupHUDArea()
{
    // free up our pen and brush, if any
    // these are NOT deleted by Deactivate() because they are allocated BEFORE Activate() is called; i.e., outside of Activate()
    DeleteObject(m_pen0);
    DeleteObject(m_hBackgroundBrush);
}

// set main HUD color
void PopupHUDArea::SetColor(COLORREF color)
{
    // only recreate the pen if the color has actually changed
    if (color != m_colorRef)
    {
        m_colorRef = color;     // update

        // must recreate pen here because we can change colors without re-activating this area
        // delete any old pen
        DeleteObject(m_pen0);

        // create our pen to draw the frame
        m_pen0 = CreatePen(PS_SOLID, 1, m_colorRef);
    }
}

void PopupHUDArea::SetBackgroundColor(COLORREF color)
{
    // only recreate the brush if the color has actually changed
    if (color != m_bgColorRef)
    {
        m_bgColorRef = color;

        // must recreate brush here because we can change colors without re-activating this area
        // delete any old brush
        DeleteObject(m_hBackgroundBrush);

        // create background color brush
        m_hBackgroundBrush = CreateSolidBrush(color);
    }
}

// Activate this area
// NOTE: if you are not using a text box, remember to hook SetHUDColors() to set the colors correctly
void PopupHUDArea::Activate()
{
    Area::Activate();  // invoke superclass method
    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(m_width, m_height), PANEL_REDRAW_ALWAYS, PANEL_MOUSE_IGNORE, PANEL_MAP_BACKGROUND);
}

// Redraw this area
// event = Orbiter event flags
// returns: true if area redrawn, false if not
bool PopupHUDArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    // if systems offline, nothing to do here
    if (GetXR1().m_internalSystemsFailure)
        return true;    // erase any currently drawn text

    // NOTE: must always invoke the subclass even if HUD is off, because it still might be TURNING off

    // have the subclass set the HUD colors
    SetHUDColors();

    bool retVal = false;

    if (m_topYCoordinate < m_height) // is HUD not OFF; i.e., is the top of the HUD visible?
    {
        HDC hDC = GetDC(surf);

        // only render the HUD frame if we have not already rendered it at this topY coordinate OR if this is PANEL_REDRAW_INIT
        bool forceRender = (event == PANEL_REDRAW_INIT) || (m_lastRenderedTopYCoordinate != m_topYCoordinate);  // if frame has moved, we MUST re-render everything

        // Cool feature here: draw HUD even while it is deploying 
        // invoke the subclass to draw the HUD whether the HUD is on or off (it may just be TURNING off)
        retVal = DrawHUD(event, m_topYCoordinate, hDC, m_colorRef, forceRender);

        // re-render the frame if necessary
        // NOTE: we must check retVal here because the subclass may have rendered new data, too
        if (retVal)
        {
            m_lastRenderedTopYCoordinate = m_topYCoordinate;   // remember this
            retVal = true;  // must always render this frame

            // render the HUD frame, starting at the bottom-left corner
            HPEN prevPen = (HPEN)SelectObject(hDC, m_pen0);   // save previous pen

            // NOTE: LineTo draws up to, but not INCLUDING, the specified point
            // Also, it appears as though the FIRST POINT under MoveToEx is not drawn, either
            MoveToEx(hDC, 0, m_height, nullptr);                 // bottom-left corner  
            LineTo(hDC, 0, m_topYCoordinate);

            MoveToEx(hDC, 0, m_topYCoordinate, nullptr);           // top-left corner
            LineTo(hDC, m_width, m_topYCoordinate);

            MoveToEx(hDC, m_width - 1, m_topYCoordinate, nullptr);   // top-right corner
            LineTo(hDC, m_width - 1, m_height);

            SelectObject(hDC, prevPen);  // restore previous pen
        }

        ReleaseDC(surf, hDC);
    }
    else if (m_lastRenderedTopYCoordinate < m_height)   // HUD is now OFF: have we not erased the last frame top line yet?
    {
        retVal = true;  // erase the last frame top line
        m_lastRenderedTopYCoordinate = m_height;    // do not re-render since HUD is now off
    }

    return retVal;    // must always redraw so we erase any old lines
}

// scroll our HUD by moving its top coordinate smoothly
void PopupHUDArea::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // let's check the current TARGET state; i.e., is the HUD on or off?
    if (isOn())
    {
        // transition to the ON state if HUD display is OFF
        if ((m_state == OnOffState::Off) || (m_state == OnOffState::TurningOff))
        {
            m_state = OnOffState::TurningOn;
            m_startScrollTime = simt;
            m_movement = -1; // scroll UP
            m_startScrollY = m_topYCoordinate;  // remember where we started    
            // NOTE: no need to reset m_topYCoordinate here; it is always accurate
        }
    }
    else  // HUD is turned off
    {
        // transition to the OFF state if HUD display is ON
        if ((m_state == OnOffState::On) || (m_state == OnOffState::TurningOn))
        {
            m_state = OnOffState::TurningOff;
            m_startScrollTime = simt;
            m_movement = 1;   // scroll DOWN
            m_startScrollY = m_topYCoordinate;  // remember where we started    
            // NOTE: no need to reset m_topYCoordinate here; it is always accurate
        }
    }

    // move the top of the HUD if it's in motion
    if (m_movement != 0)
    {
        // compute how long it's been since we started scrolling
        double deltaT = simt - m_startScrollTime;

        // handle unlikely event that the user moved the sim date backwards while the panel is deploying (scrolling)
        if (deltaT < 0)
        {
            m_startScrollTime = simt;   // reset
            deltaT = 0;
        }

        // compute how many pixels we should have moved by now based on the scroll rate in pixels/second
        int pixelDelta = static_cast<int>((deltaT * HudDeploySpeed));

        // set the top of the HUD
        m_topYCoordinate = m_startScrollY + (m_movement * pixelDelta);

        // Check whether we are BEYOND the valid range; valid range is 0 to (height), 
        // where the top line is when the HUD is OFF.
        if (m_topYCoordinate < 0)
        {
            // we reached the top; HUD is now ON
            m_topYCoordinate = 0;
            m_movement = 0;
            m_state = OnOffState::On;
        }
        else if (m_topYCoordinate > m_height)  // NOTE: we want to scroll one pixel BEYOND the lower edge to we hide the top line entirely
        {
            // we reached the bottom; HUD is now OFF
            m_topYCoordinate = m_height;    // one pixel below visible area; line will not be rendered
            m_movement = 0;
            m_state = OnOffState::Off;
        }
    }
}

