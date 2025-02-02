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
// XR1Area.cpp
// Abstract area base class that each of our Areas extend
// Also includes areas that do not fall into defined categories.
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
