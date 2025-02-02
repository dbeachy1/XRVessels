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

#include "resource.h"

#include "DeltaGliderXR1.h"
#include "XR1MultiDisplayArea.h"

//-------------------------------------------------------------------------

// Constructor
ReentryCheckMultiDisplayMode::ReentryCheckMultiDisplayMode(int modeNumber) :
    MultiDisplayMode(modeNumber),
    m_backgroundSurface(0), m_prevReentryCheckStatus(true)
{
    // NOTE: cannot accesss parent XR1 object yet because we have not yet been attached to a parent MDA object.
    // Therefore, one-time initialization that requires the our XR1 object will be done in OnParentAttach() below.
}

// Destructor
ReentryCheckMultiDisplayMode::~ReentryCheckMultiDisplayMode()
{
    for (int i = 0; i < GetDoorCount(); i++)
        delete m_pDoorInfo[i];

    delete[] m_pDoorInfo;
}

// Invoked by our parent's AddDisplayMode method immediately after we are attached to our parent MDA.
// This is useful if an MDA needs to perform some one-time initialization (which we do!)
void ReentryCheckMultiDisplayMode::OnParentAttach()
{
    m_pDoorInfo = new DoorInfo * [GetDoorCount()];  // NOTE: if a subclass is present, this may be > 6.

    // create a DoorInfo handler for each door
    const int cx = GetCloseButtonXCoord();       // X coord of close button
    int cy = GetStartingCloseButtonYCoord();
    m_pDoorInfo[0] = new DoorInfo("OPEN", "CLOSED", GetXR1().nose_status, _COORD2(cx, cy), &DeltaGliderXR1::ActivateNoseCone);
    m_pDoorInfo[1] = new DoorInfo("DEPLYD", "STOWED", GetXR1().radiator_status, _COORD2(cx, cy += GetLinePitch()), &DeltaGliderXR1::ActivateRadiator);
    m_pDoorInfo[2] = new DoorInfo("OPEN", "CLOSED", GetXR1().rcover_status, _COORD2(cx, cy += GetLinePitch()), &DeltaGliderXR1::ActivateRCover);
    m_pDoorInfo[3] = new DoorInfo("OPEN", "CLOSED", GetXR1().scramdoor_status, _COORD2(cx, cy += GetLinePitch()), &DeltaGliderXR1::ActivateScramDoors);
    m_pDoorInfo[4] = new DoorInfo("OPEN", "CLOSED", GetXR1().hoverdoor_status, _COORD2(cx, cy += GetLinePitch()), &DeltaGliderXR1::ActivateHoverDoors);
    m_pDoorInfo[5] = new DoorInfo("DOWN", "UP", GetXR1().gear_status, _COORD2(cx, cy += GetLinePitch()), &DeltaGliderXR1::ActivateLandingGear);
}

void ReentryCheckMultiDisplayMode::Activate()
{
    m_backgroundSurface = CreateSurface(IDB_REENTRY_CHECK_MULTI_DISPLAY);
    m_mainFont = CreateFont(12, 0, 0, 0, 700, 0, 0, 0, 0, 0, 0, 0, FF_MODERN, "Microsoft Sans Serif");

    // check doors and issue correct callout here
    int openDoorCount = 0;
    for (int i = 0; i < GetDoorCount(); i++)
    {
        if (m_pDoorInfo[i]->IsNotClosed())
            openDoorCount++;
    }

    PlayStatusCallout(openDoorCount);
}

void ReentryCheckMultiDisplayMode::Deactivate()
{
    DestroySurface(&m_backgroundSurface);
    DeleteObject(m_mainFont);
}

// play the status callout sound
void ReentryCheckMultiDisplayMode::PlayStatusCallout(const int openDoorCount)
{
    const double simt = GetAbsoluteSimTime();
    // do not play sound if simulation just started
    const bool bPlaySound = (simt > 2.0);
    if (openDoorCount > 0)
    {
        char msg[75];
        sprintf(msg, "WARNING: %d external door(s) open;&Reentry check FAILED.", openDoorCount);

        if (bPlaySound)
            GetXR1().ShowWarning("Warning Reentry Check Failed.wav", DeltaGliderXR1::ST_WarningCallout, msg);
        else
            GetXR1().ShowWarning(nullptr, DeltaGliderXR1::ST_None, msg);
    }
    else
    {
        if (bPlaySound)
            GetXR1().ShowInfo("Reentry Check All Systems Green.wav", DeltaGliderXR1::ST_InformationCallout, "Reentry Check: all systems green.");
        else
            GetXR1().ShowInfo(nullptr, DeltaGliderXR1::ST_None, "Reentry Check: all systems green.");
    }
}

bool ReentryCheckMultiDisplayMode::Redraw2D(const int event, const SURFHANDLE surf)
{
    // render the background
    const COORD2& screenSize = GetScreenSize();
    DeltaGliderXR1::SafeBlt(surf, m_backgroundSurface, 0, 0, 0, 0, screenSize.x, screenSize.y);

    // obtain device context and save existing font
    HDC hDC = m_pParentMDA->GetDC(surf);
    HFONT hPrevObject = (HFONT)SelectObject(hDC, m_mainFont);

    SetBkMode(hDC, TRANSPARENT);
    SetTextAlign(hDC, TA_LEFT);

    // set starting coordinates 
    COORD2 startingCoords = GetStartingCoords();
    int x = startingCoords.x;
    int y = startingCoords.y;

    // loop through and render each door's status
    int openDoorCount = 0;
    for (int i = 0; i < GetDoorCount(); i++)
    {
        const DoorInfo* pDI = m_pDoorInfo[i];

        COLORREF textColor;
        const char* pStatus;
        bool renderStatus = true;  // assume NOT in blink "off" state
        switch (pDI->m_doorStatus)
        {
        case DoorStatus::DOOR_OPEN:
            textColor = CREF(BRIGHT_RED);
            pStatus = pDI->m_pOpen;     // "Open", etc.
            openDoorCount++;
            break;

        case DoorStatus::DOOR_CLOSED:
            textColor = CREF(BRIGHT_GREEN);
            pStatus = pDI->m_pClosed;   // "Closed", etc.
            break;

        case DoorStatus::DOOR_FAILED:
            textColor = CREF(BRIGHT_RED);
            pStatus = "FAILED";
            openDoorCount++;
            break;

        case DoorStatus::DOOR_OPENING:
        case DoorStatus::DOOR_CLOSING:
            textColor = CREF(BRIGHT_YELLOW);
            pStatus = "In Transit";
            const double simt = GetAbsoluteSimTime();
            renderStatus = (fmod(simt, 0.75) < 0.375);  // blink once every 3/4-second
            openDoorCount++;
            break;
        }

        // render the door status if requested
        if (renderStatus)
        {
            SetTextColor(hDC, textColor);
            TextOut(hDC, x, y, pStatus, static_cast<int>(strlen(pStatus))); // "Left Wing", etc.
        }

        // drop to next line
        y += GetLinePitch();
    }

    // now render overall status on the bottom line
    const char* pStatus;
    COLORREF textColor;
    bool renderStatus = true;   // assume NOT blinking
    if (openDoorCount > 0)
    {
        pStatus = "Reentry Check FAILED";
        textColor = CREF(BRIGHT_RED);
        const double simt = GetAbsoluteSimTime();
        renderStatus = (fmod(simt, 2.0) < 1.5);  // on for 1.5 seconds, off for 0.5 second
    }
    else    // all doors closed
    {
        pStatus = "Reentry Check GREEN";
        textColor = CREF(BRIGHT_GREEN);
    }

    if (renderStatus)
    {
        SetTextAlign(hDC, TA_CENTER);
        SetTextColor(hDC, textColor);
        COORD2 c = GetStatusLineCoords();
        TextOut(hDC, c.x, c.y, pStatus, static_cast<int>(strlen(pStatus)));
    }

    // restore previous font and release device context
    SelectObject(hDC, hPrevObject);
    m_pParentMDA->ReleaseDC(surf, hDC);

    // play sound if our status changed from previous loop
    bool status = (openDoorCount == 0);     // true = OK
    if (status != m_prevReentryCheckStatus)
        PlayStatusCallout(openDoorCount);   // notify the pilot of the status change

    // save status for next frame
    m_prevReentryCheckStatus = status;

    return true;
}

bool ReentryCheckMultiDisplayMode::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    bool processed = false;

    COORD2 c = { mx, my };

    if (event & PANEL_MOUSE_LBDOWN)
    {
        // check the 'close' button for each door
        for (int i = 0; i < GetDoorCount(); i++)
        {
            const DoorInfo* pDI = m_pDoorInfo[i];

            if (c.InBounds(pDI->m_closeButtonCoords, 7, 7))
            {
                // invoke the door's handler to close the door if possible
                const DoorStatus ds = pDI->m_doorStatus;
                if (ds == DoorStatus::DOOR_CLOSED)
                {
                    GetXR1().PlaySound(GetXR1().Error1, DeltaGliderXR1::ST_Other, ERROR1_VOL);      // already closed
                }
                else    // either OPEN or FAILED
                {
                    // NOTE: this will display any applicable error message if the door cannot begin closing
                    (GetXR1().*pDI->m_pDoorHandler)(DoorStatus::DOOR_CLOSING);

                    GetXR1().PlaySound(GetXR1()._MDMButtonUp, DeltaGliderXR1::ST_Other);
                }
                processed = true;
                break;      // no sense in checking other coordinates
            }
        }
    }

    return processed;
}
