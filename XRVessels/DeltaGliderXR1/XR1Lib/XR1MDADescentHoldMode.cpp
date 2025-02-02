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
DescentHoldMultiDisplayMode::DescentHoldMultiDisplayMode(int modeNumber) :
    MultiDisplayMode(modeNumber),
    m_backgroundSurface(0), m_mouseHoldTargetSimt(-1), m_lastAction(RATE_ACTION::ACT_NONE), m_repeatCount(0)
{
    m_engageButtonCoord.x = 6;
    m_engageButtonCoord.y = 42;

    m_rateUp1ArrowCoord.x = 159;
    m_rateUp1ArrowCoord.y = 47;

    m_rateDown1ArrowCoord.x = 159;
    m_rateDown1ArrowCoord.y = 56;

    m_rateUp5ArrowCoord.x = 143;
    m_rateUp5ArrowCoord.y = 47;

    m_rateDown5ArrowCoord.x = 143;
    m_rateDown5ArrowCoord.y = 56;

    m_rateUp25ArrowCoord.x = 127;
    m_rateUp25ArrowCoord.y = 47;

    m_rateDown25ArrowCoord.x = 127;
    m_rateDown25ArrowCoord.y = 56;

    m_hoverButtonCoord.x = 113;
    m_hoverButtonCoord.y = 77;

    m_autoLandButtonCoord.x = 113;
    m_autoLandButtonCoord.y = 88;

    m_repeatSpeed = 0.0625;  // seconds between clicks if mouse held down: 16 clicks per second
}


void DescentHoldMultiDisplayMode::Activate()
{
    m_backgroundSurface = CreateSurface(IDB_DESCENT_HOLD_MULTI_DISPLAY);

    m_statusFont = CreateFont(12, 0, 0, 0, 600, 0, 0, 0, 0, 0, 0, 0, FF_MODERN, "Microsoft Sans Serif");  // ENGAGED or DISENGAGED
    m_numberFont = CreateFont(12, 0, 0, 0, 600, 0, 0, 0, 0, 0, 0, 0, FF_MODERN, "Microsoft Sans Serif");  // bank/pitch number text
    m_buttonFont = CreateFont(12, 0, 0, 0, 600, 0, 0, 0, 0, 0, 0, 0, FF_MODERN, "Microsoft Sans Serif");  // engage/disengage button text
}

void DescentHoldMultiDisplayMode::Deactivate()
{
    DestroySurface(&m_backgroundSurface);
    DeleteObject(m_statusFont);
    DeleteObject(m_numberFont);
    DeleteObject(m_buttonFont);
}

bool DescentHoldMultiDisplayMode::Redraw2D(const int event, const SURFHANDLE surf)
{
    // Always re-render everything; it is too error-prone to try to track all values and clear any 
    // old data underneath from the previous render.

    // render the background
    const COORD2& screenSize = GetScreenSize();
    DeltaGliderXR1::SafeBlt(surf, m_backgroundSurface, 0, 0, 0, 0, screenSize.x, screenSize.y);

    // obtain device context and save existing font
    HDC hDC = m_pParentMDA->GetDC(surf);
    HFONT hPrevObject = (HFONT)SelectObject(hDC, m_statusFont); // will render status text first
    SetBkMode(hDC, TRANSPARENT);
    SetTextAlign(hDC, TA_LEFT);     // default to LEFT alignment

    // render autopilot status
    const char* pStatus;        // set below
    COLORREF statusColor;
    const bool engaged = (GetXR1().m_customAutopilotMode == AUTOPILOT::AP_DESCENTHOLD);
    if (engaged && (GetXR1().m_customAutopilotSuspended))
    {
        pStatus = "SUSPENDED";
        statusColor = CREF(BRIGHT_WHITE);
    }
    else  // normal operation
    {
        pStatus = (engaged ? "ENGAGED" : "DISENGAGED");
        statusColor = (engaged ? CREF(BRIGHT_GREEN) : CREF(BRIGHT_RED));  // use CREF macro to convert to Windows' Blue, Green, Red COLORREF

        // check for auto-land
        if (GetXR1().m_autoLand)
        {
            pStatus = "AUTO-LAND";
            statusColor = CREF(BRIGHT_YELLOW);
        }
    }
    SetTextColor(hDC, statusColor);
    TextOut(hDC, 46, 24, pStatus, static_cast<int>(strlen(pStatus)));

    // render button text
    SelectObject(hDC, m_buttonFont);
    const char* pEngageDisengage = (engaged ? "Disengage" : "Engage");
    SetTextColor(hDC, CREF(LIGHT_BLUE));
    TextOut(hDC, 27, 43, pEngageDisengage, static_cast<int>(strlen(pEngageDisengage)));

    SelectObject(hDC, m_numberFont);
    SetTextColor(hDC, CREF(OFF_WHITE217));
    char temp[15];

    // vertical speed
    VECTOR3 v;
    GetXR1().GetAirspeedVector(FRAME_HORIZON, v);
    double vs = (GetVessel().GroundContact() ? 0 : v.y); // in m/s

    // keep in range
    if (vs > 999.99)
        vs = 999.99;
    else if (vs < -999.99)
        vs = -999.99;
    sprintf(temp, "%-+7.2f", vs);
    TextOut(hDC, 49, 62, temp, static_cast<int>(strlen(temp)));

    // altitude
    double alt = GetXR1().GetGearFullyUncompressedAltitude();   // adjust for gear down and/or GroundContact

    if (alt > 999999.9)
        alt = 999999.9;
    else if (alt < -999999.9)
        alt = -999999.9;
    sprintf(temp, "%-8.1f", alt);
    TextOut(hDC, 49, 73, temp, static_cast<int>(strlen(temp)));

    // max hover engine acc based on ship mass
    const double maxHoverAcc = GetXR1().m_maxShipHoverAcc;
    if (fabs(maxHoverAcc) > 99.999)        // keep in range
        sprintf(temp, "------ m/s²");
    else
        sprintf(temp, "%.3f m/s²", maxHoverAcc);

    COLORREF cref;  // reused later as well
    if (maxHoverAcc <= 0)
        cref = CREF(MEDB_RED);
    else if (maxHoverAcc <= 1.0)
        cref = CREF(BRIGHT_YELLOW);
    else
        cref = CREF(BRIGHT_GREEN);
    SetTextColor(hDC, cref);
    TextOut(hDC, 61, 95, temp, static_cast<int>(strlen(temp)));

    // hover thrurst pct 
    double hoverThrustFrac = GetVessel().GetThrusterGroupLevel(THGROUP_HOVER);  // do not round this; sprintf will do it
    double hoverThrustPct = (hoverThrustFrac * 100.0);
    sprintf(temp, "%.3f%%", hoverThrustPct);
    if (hoverThrustPct >= 100)
        cref = CREF(MEDB_RED);
    else if (hoverThrustPct >= 90)
        cref = CREF(BRIGHT_YELLOW);
    else
        cref = CREF(BRIGHT_GREEN);

    SetTextColor(hDC, cref);
    TextOut(hDC, 61, 84, temp, static_cast<int>(strlen(temp)));

    // render the set ascent or descent rate
    sprintf(temp, "%+.1f", GetXR1().m_setDescentRate);
    SetTextAlign(hDC, TA_RIGHT);
    SetTextColor(hDC, CREF(LIGHT_BLUE));
    TextOut(hDC, 121, 48, temp, static_cast<int>(strlen(temp)));

    // restore previous font and release device context
    SelectObject(hDC, hPrevObject);
    m_pParentMDA->ReleaseDC(surf, hDC);

    return true;
}

bool DescentHoldMultiDisplayMode::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    bool processed = false;
    bool playSound = false;  // play sound in button processing

    COORD2 c = { mx, my };

    if (event & PANEL_MOUSE_LBDOWN)
    {
        m_repeatCount = 0;  // reset just in case

        // check engage/disengage button
        if (c.InBounds(m_engageButtonCoord, 14, 14))
        {
            // toggle autopilot status
            GetXR1().ToggleDescentHold();
            processed = true;
            playSound = true;
        }
        else if (c.InBounds(m_hoverButtonCoord, 7, 7))    // HOVER button
        {
            GetXR1().SetAutoDescentRate(true, AUTODESCENT_ADJUST::AD_LEVEL, 0);    // switch to HOVER mode
            processed = true;
        }
        else if (c.InBounds(m_autoLandButtonCoord, 7, 7))    // AUTO-LAND button
        {
            // only enabled if descent hold autopilot is currently ENGAGED 
            if (GetXR1().m_customAutopilotMode == AUTOPILOT::AP_DESCENTHOLD)
                GetXR1().SetAutoDescentRate(true, AUTODESCENT_ADJUST::AD_AUTOLAND, 0);
            else
            {
                // cannot enable auto-descent; autopilot not engaged
                GetXR1().PlaySound(GetXR1().Error1, DeltaGliderXR1::ST_Other, ERROR1_VOL);
                GetXR1().ShowWarning(nullptr, DeltaGliderXR1::ST_None, "Descent Hold autopilot not engaged.");
            }

            processed = true;
        }
    }

    // check rate buttons
    RATE_ACTION action = RATE_ACTION::ACT_NONE;
    if (event & (PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBPRESSED))
    {
        const double simt = GetAbsoluteSimTime();

        // if TRUE, process a button click
        bool doButtonClick = false;

        if (event & PANEL_MOUSE_LBDOWN)
        {
            // mouse just clicked; always process it immediately
            doButtonClick = true;
            playSound = true;

            // next click if mouse held down is 0.75 second from now
            m_mouseHoldTargetSimt = simt + 0.75;
        }

        // check whether we reached our target hold time
        if ((m_mouseHoldTargetSimt > 0) && (simt >= m_mouseHoldTargetSimt))
        {
            doButtonClick = true;
            m_mouseHoldTargetSimt = simt + m_repeatSpeed;   // process another event if mouse held down long enough
            m_repeatCount++;        // remember this
        }

        // check rate arrows
        // Note: by defaule we use PILOT notation here; down arrow INCREMENTS rate and vice-versa
        // However, the user can invert that behavior via the preference setting shown below
        const bool invertRateArrows = GetXR1().GetXR1Config()->InvertDescentHoldRateArrows;
        if (c.InBounds(m_rateUp1ArrowCoord, 6, 7))
        {
            if (doButtonClick)
            {
                if (invertRateArrows) goto incrate1;
            decrate1:
                m_lastAction = action = RATE_ACTION::DECRATE1;
            }
        }
        else if (c.InBounds(m_rateDown1ArrowCoord, 6, 7))
        {
            if (doButtonClick)
            {
                if (invertRateArrows) goto decrate1;
            incrate1:
                m_lastAction = action = RATE_ACTION::INCRATE1;
            }
        }
        else if (c.InBounds(m_rateUp5ArrowCoord, 6, 7))
        {
            if (doButtonClick)
            {
                if (invertRateArrows) goto incrate5;
            decrate5:
                m_lastAction = action = RATE_ACTION::DECRATE5;
            }
        }
        else if (c.InBounds(m_rateDown5ArrowCoord, 6, 7))
        {
            if (doButtonClick)
            {
                if (invertRateArrows) goto decrate5;
            incrate5:
                m_lastAction = action = RATE_ACTION::INCRATE5;
            }
        }
        else if (c.InBounds(m_rateUp25ArrowCoord, 6, 7))
        {
            if (doButtonClick)
            {
                if (invertRateArrows) goto incrate25;
            decrate25:
                m_lastAction = action = RATE_ACTION::DECRATE25;
            }
        }
        else if (c.InBounds(m_rateDown25ArrowCoord, 6, 7))
        {
            if (doButtonClick)
            {
                if (invertRateArrows) goto decrate25;
            incrate25:
                m_lastAction = action = RATE_ACTION::INCRATE25;
            }
        }
        else
        {
            // mouse is outside of any buttons!
            // NOTE: technically we should check if the mouse moves off of the FIRST button clicked, but it's not worth the 
            // effort since there is blank space between the buttons anyway.
            m_mouseHoldTargetSimt = -1;
        }
    }
    else if (event & (PANEL_MOUSE_LBUP))
    {
        // mouse released; reset hold timer
        m_mouseHoldTargetSimt = -1;

        // re-issue the last action so a message is logged about the final state now IF we were repeating the button clicks
        if (m_repeatCount > 0)
        {
            action = m_lastAction;
            playSound = true;   // show final message and play button up sound
            m_repeatCount = 0;  // reset
        }

        m_lastAction = RATE_ACTION::ACT_NONE;  // reset
    }

    if (action != RATE_ACTION::ACT_NONE)
    {
        switch (action)
        {
        case RATE_ACTION::INCRATE1:
            GetXR1().SetAutoDescentRate(playSound, AUTODESCENT_ADJUST::AD_ADJUST, ADRATE_SMALL);
            processed = true;
            break;

        case RATE_ACTION::DECRATE1:
            GetXR1().SetAutoDescentRate(playSound, AUTODESCENT_ADJUST::AD_ADJUST, -ADRATE_SMALL);
            processed = true;
            break;

        case RATE_ACTION::INCRATE5:
            GetXR1().SetAutoDescentRate(playSound, AUTODESCENT_ADJUST::AD_ADJUST, ADRATE_MED);
            processed = true;
            break;

        case RATE_ACTION::DECRATE5:
            GetXR1().SetAutoDescentRate(playSound, AUTODESCENT_ADJUST::AD_ADJUST, -ADRATE_MED);
            processed = true;
            break;

        case RATE_ACTION::INCRATE25:
            GetXR1().SetAutoDescentRate(playSound, AUTODESCENT_ADJUST::AD_ADJUST, ADRATE_LARGE);
            processed = true;
            break;

        case RATE_ACTION::DECRATE25:
            GetXR1().SetAutoDescentRate(playSound, AUTODESCENT_ADJUST::AD_ADJUST, -ADRATE_LARGE);
            processed = true;
            break;

        default:
            // no action
            break;
        }
    }

    return processed;
}
