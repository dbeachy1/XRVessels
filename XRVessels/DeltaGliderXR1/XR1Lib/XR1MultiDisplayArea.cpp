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
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// XR1MultiDisplayArea.cpp
// Area class that manages all MultiDisplayMode objects
// ==============================================================

#include "resource.h"
#include "AreaIDs.h"

#include "DeltaGliderXR1.h"
#include "XR1MultiDisplayArea.h"

//----------------------------------------------------------------------------------

//
// Constructor
// vessel = our vessel handle
// panelCoordinates = absolute coordinates of this area on the parent instrument panel; added at upper-left corner, just inside the frame
// areaID = unique Orbiter area ID
MultiDisplayArea::MultiDisplayArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_pActiveDisplayMode(nullptr), m_screenBlanked(true)
{
    // define active area top-left coordinates
    m_nextButtonCoord.x = 169;
    m_nextButtonCoord.y = 99;
    
    m_prevButtonCoord.x = 152;
    m_prevButtonCoord.y = 99;

    m_screenSize.x = 179;
    m_screenSize.y = 110;
}

// Destructor
MultiDisplayArea::~MultiDisplayArea()
{
    // free up our MultiDisplayMode objects so our subclass won't have to
    // free all areas in the vector 
    unordered_map<int, MultiDisplayMode *>::const_iterator it = m_modeMap.begin();   // iterates over values
    for (; it != m_modeMap.end(); it++)
    {
        MultiDisplayMode *pMode = it->second;   // get next MultiDisplayMode value in the map
        delete pMode;
    }
}

// Activate this area
void MultiDisplayArea::Activate()
{
    _ASSERTE(!IsActive());
    Area::Activate();  // invoke superclass method
    // register area
    // specify both PANEL_REDRAW_ALWAYS and PANEL_REDRAW_MOUSE because we need explicit mouse events
    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(m_screenSize.x, m_screenSize.y), PANEL_REDRAW_ALWAYS | PANEL_REDRAW_MOUSE, PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBPRESSED | PANEL_MOUSE_LBUP | PANEL_MOUSE_ONREPLAY, PANEL_MAP_BGONREQUEST);

    TurnOn();   // re-enable last active mode
}

// Deactivate this area
void MultiDisplayArea::Deactivate()
{
    _ASSERTE(IsActive());
    // Deactivate the active mode, if any
    TurnOff();

    // invoke our superclass method so it can clean up its resources, too
    XR1Area::Deactivate();
}

// Invoked at instantiation time to add any number of display modes.
// These are automatically freed by our destructor, so the caller does not need to free them manually.
// Returns: pMultiDisplayMode
MultiDisplayMode *MultiDisplayArea::AddDisplayMode(MultiDisplayMode *pMultiDisplayMode)
{
    pMultiDisplayMode->SetParent(this);

    // add to our list of valid modes
    typedef pair<int, MultiDisplayMode *> Int_Mode_Pair;
    int modeNumber = pMultiDisplayMode->GetModeNumber();
    m_modeMap.insert(Int_Mode_Pair(modeNumber, pMultiDisplayMode));  // key = area ID, value = MultiDisplayMode *

    // invoke the MDM OnParentAttach hook now to allow it to perform any one-time initialization
    pMultiDisplayMode->OnParentAttach();

    return pMultiDisplayMode;
}


// switch to a new mode
// returns: ID of new mode, or -1 if screen is off
int MultiDisplayArea::SwitchActiveMode(DIRECTION dir)
{
    if (m_pActiveDisplayMode == nullptr)
        return -1;  // screen is off

    // get the active mode number
    int activeMode = m_pActiveDisplayMode->GetModeNumber();   // 0...n
    
    int lowestMode = MAXLONG;
    int highestMode = -1;
    int closestMatchUp = MAXLONG;
    int closestMatchDown = -1;
    int newMode = -1;
    
    // walk the entire list of valid modes and find best matches
    unordered_map<int, MultiDisplayMode *>::const_iterator it = m_modeMap.begin();   // iterates over values
    for (; it != m_modeMap.end(); it++)
    {
        MultiDisplayMode *pMode = it->second;   // get next MultiDisplayMode value in the map
        int id = pMode->GetModeNumber();
        
        if ((id > activeMode) && (id < closestMatchUp))
            closestMatchUp = id;

        if ((id < activeMode) && (id > closestMatchDown))
            closestMatchDown = id;

        if (id < lowestMode)
            lowestMode = id;

        if (id > highestMode)
            highestMode = id;
    }

    // now figure out the new mode, wrapping around if necessary
    if (dir == DIRECTION::UP)
    {
        if (closestMatchUp == MAXLONG)
            newMode = lowestMode;   // wrap around
        else
            newMode = closestMatchUp;   // move up one step
    }
    else    // dir == DOWN
    {
        if (closestMatchDown < 0)
            newMode = highestMode;  // wrap around
        else
            newMode = closestMatchDown; // move down one step
    }

    // set the new mode; this will deactivate the old mode and activate the new mode
    SetActiveMode(newMode);

    return newMode;
}

// Invoked to switch the active mode and turn on the screen.
// This is the ONLY method that switches to or activates a new mode
// Returns: true on success, false if no such mode
bool MultiDisplayArea::SetActiveMode(int modeNumber)
{
    if (modeNumber < 0)
        return false;       // screen disabled

    bool retVal = false;

    // locate mode handler for this mode number
    MultiDisplayMode *pDisplayMode = nullptr;

    unordered_map<int, MultiDisplayMode *>::const_iterator it = m_modeMap.find(modeNumber);
    if (it != m_modeMap.end())
        pDisplayMode = it->second;     // found a matching area

    // if new mode is valid, switch to it
    if (pDisplayMode != nullptr)
    {
        // deactivate the OLD (existing) mode
        TurnOff();

        // now activate the new mode handler
        m_pActiveDisplayMode = pDisplayMode;
        m_pActiveDisplayMode->Activate();          

        GetXR1().m_activeMultiDisplayMode = pDisplayMode->GetModeNumber();  // update persisted state
        m_screenBlanked = false;  // screen is active now
        retVal = true;
    }

    return retVal;
}

// Reenable the previously active mode.
// NOTE: this method must not be invoked before the parent MultiDisplayArea is activated.
//
// returns: true if successfully activated, false otherwise
bool MultiDisplayArea::TurnOn()
{
    return SetActiveMode(GetXR1().m_activeMultiDisplayMode);
}

// Disable the display for the active mode, if any; this will turn off the screen
// NOTE: this method must not be invoked before the parent MultiDisplayArea is activated.
void MultiDisplayArea::TurnOff()
{
    if (m_pActiveDisplayMode != nullptr)  // not already turned off?
    {
        m_pActiveDisplayMode->Deactivate();
        m_pActiveDisplayMode = nullptr;
        // do not clear GetXR1().m_activeMultiDisplayMode variable here; a mode stays set until changed
    }

    // NOTE: do not set m_screenBlanked here; the next redraw of the area will blank the screen and then set the flag
}

// Redraw this area
// event = Orbiter event flags
// returns: true if area redrawn, false if not
bool MultiDisplayArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    // if systems offline or no active display mode, nothing more to render
    if ((m_pActiveDisplayMode == nullptr) || (GetXR1().m_internalSystemsFailure))
    {
        // blt the area background to blank the screen IF we haven't done it before
        if (m_screenBlanked == false)
        {
            oapiBltPanelAreaBackground(GetAreaID(), surf);
            m_screenBlanked = true;   // remember this so we don't keep re-blitting the area
            return true;
        }
        return false;   // screen is currently off and was already blanked
    }

    // screen is active; pass the redraw command down the the active mode handler
    bool redraw = m_pActiveDisplayMode->Redraw2D(event, surf);

    return redraw;
}

// Handle mouse events for this area
// event = Orbiter event flags
// mx, my = mouse coordinates relative to the area
// returns: true if event processed, false if not
bool MultiDisplayArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated or systems failure, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard() || GetXR1().m_internalSystemsFailure)
        return false;

    if (m_pActiveDisplayMode == nullptr)
        return false;

    // sprintf(oapiDebugString(), "mx=%d, my=%d, event=0x%X", mx, my, event);
    COORD2 c = { mx, my };

    // set button mouse-over states
    bool mouseOverNextButton = c.InBounds(m_nextButtonCoord, 7, 6);  // 7 pixels wide x 6 pixels high
    bool mouseOverPrevButton = c.InBounds(m_prevButtonCoord, 7, 6);
    
    // process active areas common to all modes
    if (mouseOverNextButton || mouseOverPrevButton)
    {
        if (event & PANEL_MOUSE_LBDOWN)
        {
            if (mouseOverNextButton)
            {
                SwitchActiveMode(DIRECTION::UP);
                GetXR1().PlaySound(GetXR1()._MDMButtonUp, DeltaGliderXR1::ST_Other);
            }
            else if (mouseOverPrevButton)
            {
                SwitchActiveMode(DIRECTION::DOWN);
                GetXR1().PlaySound(GetXR1()._MDMButtonDown, DeltaGliderXR1::ST_Other);
            }
        }
        return true;    // we processed this event; the active mode handler should not receive mouse events that we already processed
    }

    // pass the mouse event on to the subclass for processing
    return m_pActiveDisplayMode->ProcessMouseEvent(event, mx, my);
}

bool MultiDisplayArea::ProcessVCMouseEvent(const int event, const VECTOR3 &coords)
{
    // if crew is incapacitated or systems failure, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard() || GetXR1().m_internalSystemsFailure)
        return false;

    if (m_pActiveDisplayMode == nullptr)
        return false;

    // invoke the subclass
    return m_pActiveDisplayMode->ProcessVCMouseEvent(event, coords);
}
