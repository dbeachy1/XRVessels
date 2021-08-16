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
// XR1HUD.cpp
// Handles all HUDs
// ==============================================================

#include "resource.h"
#include "AreaIDs.h"

#include "DeltaGliderXR1.h"
#include "XR1HUD.h"

// --------------------------------------------------------------
// Respond to HUD mode change
// --------------------------------------------------------------

void DeltaGliderXR1::clbkHUDMode (int mode)
{
    // signal 2D area
    TriggerRedrawArea(AID_HUDMODE); 

    // signal 3D areas 
    TriggerRedrawArea(AID_HUDBUTTON1); 
    TriggerRedrawArea(AID_HUDBUTTON2); 
    TriggerRedrawArea(AID_HUDBUTTON3); 
    TriggerRedrawArea(AID_HUDBUTTON4); 
}

//----------------------------------------------------------------------------------
// Override the default clbkRenderHUD so we can prevent the Orbiter core from rendering the default HUD
//----------------------------------------------------------------------------------
void DeltaGliderXR1::clbkRenderHUD (int mode, const HUDPAINTSPEC *hps, SURFHANDLE hDefaultTex)
{
    // hide the default hud on systems failure or if the data HUD is active
    if ((!m_internalSystemsFailure) && (!m_dataHUDActive))
        VESSEL3::clbkRenderHUD(mode, hps, hDefaultTex);
}

//----------------------------------------------------------------------------------
// Draw the main HUD
//----------------------------------------------------------------------------------

// VESSEL3 implementation
// Returns: true on success, or false for the core to invoke the old VESSEL2 HUD renderer method, if any
bool DeltaGliderXR1::clbkDrawHUD (int mode, const HUDPAINTSPEC *hps, oapi::Sketchpad *skp)
{
    // for non-VC HUDs, save previous font and select new font that matches Orbiter 2010 new look
    oapi::Font *pOrgHUDFont = ((!IsCameraVC()) ? skp->SetFont(GetNormal2DHUDFont()) : NULL);   

    // center of the HUD
    const int cx = hps->CX;
    const int cy = hps->CY;  

    // main HUD is rendered only if internal systems are still OK
    // *** NOTE: IF YOU CHANGE THE HIDE/SHOW HUD BEHAVIOR, UPDATE clbkRenderHUD AS WELL: IT HANDLES HIDING/SHOWING THE DEFAULT HUD ***
    if (m_internalSystemsFailure == false)
    {
        // if data HUD is enabled, display the data HUD instead of the normal HUD
        if (m_dataHUDActive)
        {
            RenderDataHUD(hps, skp);
            return true;
        }

        // Note: we still need this call to render the default HUD in VC mode (but not in 2D mode, oddly)
        VESSEL3::clbkDrawHUD(mode, hps, skp);

        const int markerSize = hps->Markersize;

        int d = markerSize/2;
        const bool blinkOn = (fmod(GetAbsoluteSimTime(), 1.0) < 0.5);

        // default to LEFT alignment
        skp->SetTextAlign(oapi::Sketchpad::LEFT);

        // show Retro/Hover/SCRAM door open messages     
        // If we are rendering on the VC HUD or glass cockpit (i.e., anything but 2D panel), 
        // must render text lower (+Y) because there is no room at the top
        int startingYMarkerLine;
        if (IsCameraGeneric())
            startingYMarkerLine = 6;
        else if (IsCameraVC())
            startingYMarkerLine = 4;
        else    // 2D camera mode
            startingYMarkerLine = 2;

        const int doorIndicatorX = 10;  // common X coordinate; matches Orbiter top line
        const int doorIndicatorYBase = markerSize * startingYMarkerLine;      // starting Y coordinate
        const int doorIndicatorYDelta = (IsCameraVC() ? static_cast<int>(((static_cast<double>(markerSize)) / 1.5)) : m_pHudNormalFontSize);    // space between lines

#define RENDER_HUD_DOOR_TEXT(doorStatus, lineNumber, text)      \
    if ((doorStatus != DoorStatus::DOOR_CLOSED) && (doorStatus != DoorStatus::DOOR_FAILED))  \
    {                                                           \
        if ((doorStatus == DoorStatus::DOOR_OPEN) || blinkOn)               \
            skp->Text(doorIndicatorX, doorIndicatorYBase + (doorIndicatorYDelta * lineNumber), text, static_cast<int>(strlen(text)));  \
    }

        RENDER_HUD_DOOR_TEXT(rcover_status,    0, "Retro Doors");
        RENDER_HUD_DOOR_TEXT(hoverdoor_status, 1, "Hover Doors");
        RENDER_HUD_DOOR_TEXT(scramdoor_status, 2, "SCRAM Doors");
        RENDER_HUD_DOOR_TEXT(nose_status, 3, NOSECONE_LABEL);
        
        // render the bay door status if the ship has a bay
        if (m_pPayloadBay != nullptr)
            RENDER_HUD_DOOR_TEXT(bay_status, 4, "Bay Doors");

        // show gear deployment status
        if (gear_status == DoorStatus::DOOR_OPEN || (gear_status >= DoorStatus::DOOR_CLOSING && blinkOn))
        {
            if ((cx >= -d*3) && (cx < hps->W+d*3) && (cy >= d && cy < hps->H+d*5))
            {
                // we use a wider pen in 2D mode to make the indicators better match the look of the new HUD (default pen is 1 pixel wide)
                oapi::Pen *pOrgPen = nullptr;
                oapi::Pen *pNewPen = nullptr;
                if (!IsCameraVC())
                {
                    // NOTE: there seems to be no current pen, so I have to use GetTextColor instead.
                    // Retrieve the current HUD color: this is a hack to retrieve the current HUD color setting since there is no "GetTextColor" in the sketchpad API.
                    DWORD hudColor = skp->SetTextColor(0xFFFFFF);  // color being set doesn't matter here since we reset it anyway
                    skp->SetTextColor(hudColor);  

                    // create a wider pen based on the video mode resolution
                    pNewPen = oapiCreatePen(1, Get2DHUDGearIndicatorPenWidth(), hudColor);
                    pOrgPen = skp->SetPen(pNewPen);
                }

                // render the gear indicators
                skp->Rectangle (cx-(d/2), cy-(d*5), cx+(d/2), cy-(d*4));
                skp->Rectangle (cx-(d*3), cy-(d*2), cx-(d*2), cy-d);
                skp->Rectangle (cx+(d*2), cy-(d*2), cx+(d*3), cy-d);

                if (!IsCameraVC())
                {
                    // free the new pen
                    oapiReleasePen(pNewPen);

                    // restore the previous pen
                    skp->SetPen(pOrgPen);
                }
            }
        }

        // draw blinking "AIRBRAKE" on the HUD if airbrake deployed
        if ((brake_status != DoorStatus::DOOR_CLOSED) && (brake_status != DoorStatus::DOOR_FAILED) && blinkOn)
        {
            // render AIRBRAKE above and to the right of center
            int x = (IsCameraVC() ? cx : (cx + (markerSize * 2)));
            int y = cy - (markerSize * 4);

            char str[32];
            sprintf(str, "AIRBRAKE DEPLOYED");  
            skp->Text(x, y, str, static_cast<int>(strlen(str)));
        }

        // if grounded, render WHEEL BRAKES above and to the left and/or right of center
        // Always render brake message: if (GroundContact()
        {
            const double leftWheelBrakeLevel = GetWheelbrakeLevel(1);
            const double rightWheelBrakeLevel = GetWheelbrakeLevel(2);

            char str[64];
            int y = (IsCameraVC() ? (cy + (markerSize * 4)) : (cy - (markerSize * 2)));
            const bool apuOnline = (apu_status == DoorStatus::DOOR_OPEN);
            const char *pLeftWheelBrake;
            const char *pRightWheelBrake;
            const char *pNoHydraulicPressure;
            if (IsCameraVC())
            {
				if (m_parkingBrakesEngaged)
				{
					pLeftWheelBrake = "PBRAKE";
					pRightWheelBrake = "PBRAKE";
				}
				else
				{
					pLeftWheelBrake = "LBRAKE";
					pRightWheelBrake = "RBRAKE";
				}
                pNoHydraulicPressure = "NO HYD. PRESS.";
            }
            else  // 2D or glass HUD
            {
				if (m_parkingBrakesEngaged)
				{
					pLeftWheelBrake = "PARKING BRAKE";
					pRightWheelBrake = "PARKING BRAKE";
				}
				else
				{
					pLeftWheelBrake = "LEFT WHEEL BRAKE";
					pRightWheelBrake = "RIGHT WHEEL BRAKE";
				}
                pNoHydraulicPressure = "NO HYDRAULIC PRESSURE";
            }

            if (leftWheelBrakeLevel > 0)
            {
                skp->SetTextAlign(oapi::Sketchpad::RIGHT);
                const int x = (IsCameraVC() ? (cx - markerSize) : (cx - (markerSize * 2)));
                if (apuOnline || m_parkingBrakesEngaged)  // parking brakes do not require hydraulic pressure to remain engaged
                {
                    sprintf(str, "%s: %d%%", pLeftWheelBrake, static_cast<int>(leftWheelBrakeLevel * 100));
                    skp->Text(x, y, str, static_cast<int>(strlen(str)));
                }
                else    // blink warning
                {
                    if (blinkOn)
                    {
                        skp->Text(x, y, pNoHydraulicPressure, static_cast<int>(strlen(pNoHydraulicPressure)));
                    }
                }

                skp->SetTextAlign(oapi::Sketchpad::LEFT);  // preserve default alignment
            }

            if (rightWheelBrakeLevel > 0)
            {
                const int x = (IsCameraVC() ? (cx + markerSize) : (cx + (markerSize * 2)));
				if (apuOnline || m_parkingBrakesEngaged)   // parking brakes do not require hydraulic pressure to remain engaged
                {
                    sprintf(str, "%s: %d%%", pRightWheelBrake, static_cast<int>((rightWheelBrakeLevel * 100)));
                    skp->Text(x, y, str, static_cast<int>(strlen(str)));
                }
                else    // blink warning
                {
                    if (blinkOn)
                    {
                        skp->Text(x, y, pNoHydraulicPressure, static_cast<int>(strlen(pNoHydraulicPressure)));
                    }
                }
            }
        }

        // draw the vertical speed text if in SURFACE mode
        if (mode == HUD_SURFACE)
        {
            VECTOR3 v;
			GetAirspeedVector(FRAME_HORIZON, v);
            const double verticalSpeed = (GroundContact() ? 0 : v.y);      // in m/s
            
            // adjust alt. for landing gear if gear is down
            const double altitude = GetGearFullyUncompressedAltitude();   // show gear fully extended
            
            // values for altitude and v/s distance text
			int x = cx + (markerSize * 2);
            int y = cy + markerSize;
            const int deltaY = (IsCameraVC() ? static_cast<int>(markerSize * .75) : m_pHudNormalFontSize);

            // render altitude and v/s to the right and down of center if requested (initialized above)
            if (GetXR1Config()->ShowAltitudeAndVerticalSpeedOnHUD)
            {
                char str[128];
                // altitude
                CString altitudeStr;
                FormatDouble(altitude, altitudeStr, 1);  // format with commas
                sprintf(str, "%s meters", static_cast<const char *>(altitudeStr));   // e.g., "10,292.6 meters"
                skp->Text(x, y, str, static_cast<int>(strlen(str)));
                y += deltaY;  // next line down

                // vertical speed
                sprintf(str, "%+.1lf m/s", verticalSpeed);
                skp->Text(x, y, str, static_cast<int>(strlen(str)));  
                y += deltaY;
            }

            // show base distance if requested
            // 0 = always on, < 0 = always off, other = altitude threshold
            const double altitudeThreshold = GetXR1Config()->DistanceToBaseOnHUDAltitudeThreshold;
            if (altitudeThreshold >= 0)
            {
                if ((altitudeThreshold == 0) || ((altitude / 1000) <= altitudeThreshold))  // threshold is in kilometers
                {
					if (IsCameraVC())
						x = 10;		// show from left side of HUD (no room to render it to the right in the VC)
                    y += deltaY;	// blank line separator
                    char str[128];
                    // base distance; in Orbiter, this is the *closest* base to the ship
                    char baseName[60];
                    double baseDistance;  // initailized below
                    const bool baseFound = GetLandingTargetInfo(baseDistance, baseName, sizeof(baseName));
                    if (baseFound)
                    {
                        CString distanceString;
                        int precision;
                        // show "meters" if we are < 10 km away
                        if (baseDistance < 10e3)
                        {
                            if (baseDistance < 1e3)  // < 1 km
                                precision = 1;
                            else  // < 10 km
                                precision = 0;

                            FormatDouble(baseDistance, distanceString, precision);  // format with commas
                            sprintf(str, "%s: %s meters", baseName, static_cast<const char *>(distanceString));
                        }
                        else  // >= 10 km
                        {
                            if (baseDistance < 100e3)        // < 100 km
                                precision = 2;  // "n.## km"
                            else if (baseDistance < 1000e3)  // < 1000 km
                                precision = 1;  // "n.# km"
                            else                             // >= 1000 km
                                precision = 0;  // "n km"

                            FormatDouble((baseDistance / 1000), distanceString, precision);  // format with commas
                            sprintf(str, "%s: %s km", baseName, static_cast<const char *>(distanceString));
                        }
                    }
                    else  // no base found
                    {
                        sprintf(str, "[no base]");
                    }
                    skp->Text(x, y, str, static_cast<int>(strlen(str)));
                    y += deltaY;  // next line down
                }
            }
        }

        // show RCS mode if in the VC
        if (IsCameraVC())
        {
            const char *pStatus;
            switch (GetAttitudeMode())
            {
                case RCS_ROT:
                    pStatus = "RCS ROT";
                    break;

                case RCS_LIN:
                    pStatus = "RCS LIN";
                    break;

                default:
                    pStatus = "RCS OFF";
            }
            skp->Text(12, hps->H-13, pStatus, 7);
        }
    }   // end if (m_internalSystemsFailure == false)

    static char hudWarningText[MAX_MESSAGE_LENGTH];  // temporary buffer
    strcpy(hudWarningText, m_hudWarningText);  // may be empty
    
    // SPECIAL CHECK: if the ship is unflyable because no pilot is on board *and* there is
    // no existing HUD message (like "Crew is Dead!") AND we have not crashed, render temporary warning text.
    if ((*hudWarningText == 0) && (!IsPilotOnBoard()) && (!IsCrashed()))
        strcpy(hudWarningText, "NO PILOT ON BOARD");

    //
    // Show critical message, such as crash message, if any!
    // This is ALWAYS rendered since it is a warning and not part of the HUD per se.
    //
    if (*hudWarningText)
    {
        oapi::Font *prevFont = skp->SetFont(m_pHudWarningFont);   // save previous font and select new font
        
        // use RED for this
        skp->SetTextColor(CREF(BRIGHT_RED));
        skp->SetBackgroundMode(oapi::Sketchpad::BK_TRANSPARENT);
        skp->SetTextAlign(oapi::Sketchpad::CENTER);

        // parse string to honor newlines
        int coordY = cy - m_pHudWarningFontSize * 3; // just above center
        char *pStart = hudWarningText;
        bool cont = true;
        while (cont)
        {
            char *pEnd = strchr(pStart, '&');
            if (pEnd)   // found a newline?
            {
                *pEnd = 0;     // terminate line
            }
            else
                cont = false;   // this is the last line

            skp->Text(cx, coordY, pStart, static_cast<int>(strlen(pStart)));   // above center
            
            coordY += m_pHudWarningFontSize; // drop to next line

            if (pEnd)
                pStart = pEnd + 1;  // skip 1 char beyond newline
        }
        skp->SetFont(prevFont);   // restore previously selected font
    }


    if (pOrgHUDFont != nullptr)
        skp->SetFont(pOrgHUDFont);   // restore original HUD font

    return true;
}

//-------------------------------------------------------------------------
// Returns the pen width for the 2D HUD gear markers (wider for higher resolutions to match the HUD lines).
int DeltaGliderXR1::Get2DHUDGearIndicatorPenWidth()
{
    int retVal;
    // Returns: 1280, 1600, or 1920.
    int width = Get2DPanelWidth();
    switch (width)
    {
        case 1920:
            retVal = 4;  // Note: 5 is a little too wide here
            break;

        case 1600:
            retVal = 4;
            break;

        case 1280:
        default:        // 1280 is the default if unknown
            retVal = 3;
            break;
    }
    return retVal;
}

// render the Data HUD
void DeltaGliderXR1::RenderDataHUD(const HUDPAINTSPEC *hps, oapi::Sketchpad *skp)
{
    const int markerSize = hps->Markersize;

    // default to LEFT alignment
    skp->SetTextAlign(oapi::Sketchpad::LEFT);

    // save the existing font
    oapi::Font *pPrevFont = skp->SetFont(m_pDataHudFont);   // save previous font and select new font

    // NOTE: use active color; i.e., do not change it
    skp->SetBackgroundMode(oapi::Sketchpad::BK_TRANSPARENT);
    
    const int width = hps->W;
    const int height = hps->H;

    // determine how many lines to render
    int strCount = 0;
    for (const char **p = DATA_HUD_VALUES; *p; *p++)
        strCount++;

    // NOTE: although there will always be an even number of strings here, there may be an ODD number
    // of ROWS since we render two strings per row (per columnset).
    int rowCount = (strCount / 2);
    if (rowCount & 1)   // is rowCount odd?
        rowCount++;     // must go through an even number of strings per columnset so we don't get off-by-one!

    // first line on HUD
    skp->SetTextAlign(oapi::Sketchpad::CENTER);
    int coordY = static_cast<int>((static_cast<double>(height) * 0.03));
    // ORG: const char *pHeader = "CUSTOM SHORTCUT KEYS";
    char pHeader[200];
    sprintf(pHeader, "%s %s", VESSELNAME, VERSION);  
    skp->Text(hps->CX, coordY, pHeader, static_cast<int>(strlen(pHeader)));  // render text
    coordY += m_pDataHudFontSize * 2;    // leave blank line

    // render four columns of data on each row
    skp->SetTextAlign(oapi::Sketchpad::LEFT);
    const int tab[2][2] =   // two column sets to two columns per set
    { 
        { static_cast<int>((static_cast<double>(width) * .05)), static_cast<int>((static_cast<double>(width) * .20)) },
        { static_cast<int>((static_cast<double>(width) * .55)), static_cast<int>((static_cast<double>(width) * .70)) }
    };
    int tabIdx = 0;     // index into tab array
    const char **pStr = DATA_HUD_VALUES; // ptr into DATA_HUD_VALUES

    // render two sets of two colums per set
    // NOTE: we want to render VERTICALLY here rather than horizontally
    const int startingCoordY = coordY;
    for (int columnSet = 0; columnSet < 2; columnSet++)
    {
        // reset for this new columnset
        coordY = startingCoordY;
        tabIdx = 0;     

        for (int rowNum = 0; rowNum < rowCount; rowNum++)
        {
            if (*pStr == nullptr)
                break;  // end of text

            skp->Text(tab[columnSet][tabIdx], coordY, *pStr, static_cast<int>(strlen(*pStr)));  // render text

            // bump tabIndex
            tabIdx ^= 1;        // toggle 0->1 and 1->0

            if (tabIdx == 0)    // reset to start of next line?
                coordY += m_pDataHudFontSize;  // drop to next row

            pStr++;         // bump to next string to render
        }
    }

    skp->SetFont(pPrevFont);   // restore previously selected font
}

//-------------------------------------------------------------------------
// Returns a handle to the normal 2D HUD font; this varies based on the video mode width, so it 
// must be created later after we can determine the video mode width.
// This also sets m_pHudNormalFontSize, which the caller is free to use.
oapi::Font *DeltaGliderXR1::GetNormal2DHUDFont()
{
    // should not be called for VC modes
    _ASSERTE(!IsCameraVC());

    if (m_pHudNormalFont == nullptr)  // not cached yet?
    {
        int fontSize;  // set below

        // Returns: 1280, 1600, or 1920.
        int width = Get2DPanelWidth();
        switch (width)
        {
#if 0 // pre-XR1-1.9 (pre-sketchpad) sizes
            case 1920:
                fontSize = 22;
                break;

            case 1600:
                fontSize = 20;
                break;

            case 1280:
            default:        // 1280 is the default if unknown
                fontSize = 18;
                break;
#endif
            case 1920:
                fontSize = 24;
                break;

            case 1600:
                fontSize = 22;
                break;

            case 1280:
            default:        // 1280 is the default if unknown
                fontSize = 20;
                break;
        }
        // create HUD normal font (matches new look in Orbiter 2010)
        // Note: the new Orbiter 2010 core HUD text uses Arial bold; however, our custom text looks better in a fixed-width font,
        // so we use "Lucida Console".
        m_pHudNormalFont = oapiCreateFont(fontSize, false, "Lucida Console", FONT_BOLD);  // fixed-width (prop = false)
        m_pHudNormalFontSize = fontSize;  // includes spacing
    }
    return m_pHudNormalFont;
}

//-------------------------------------------------------------------------

HudIntensitySwitchArea::HudIntensitySwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID) :
    VerticalCenteringRockerSwitchArea(parentPanel, panelCoordinates, areaID, meshTextureID, false, true)     // this is a single switch, reverse rotation=true
{
    // must set this here after base class is initialized because GetXR1() is in the base class
    SetXRAnimationHandle(&GetXR1().anim_hudintens);  
}

// Process a mouse event that occurred on our switch
// switches = which switches moved (LEFT, RIGHT, BOTH, SINGLE, NA)
// position = current switch position (UP, DOWN, CENTER)
void HudIntensitySwitchArea::ProcessSwitchEvent(SWITCHES switches, POSITION position)
{
    if (position == POSITION::UP)
    {
        oapiIncHUDIntensity();
    }
    else if (position == POSITION::DOWN)
    {
        oapiDecHUDIntensity();
    }
}

//----------------------------------------------------------------------------------

HudColorButtonArea::HudColorButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    TimedButtonArea(parentPanel, panelCoordinates, areaID)
{
}

bool HudColorButtonArea::ProcessVCMouseEvent(const int event, const VECTOR3 &coords)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    // button does not light up in VC mode
    oapiToggleHUDColour();

    GetXR1().PlaySound(GetXR1().SwitchOn, DeltaGliderXR1::ST_Other, QUIET_CLICK);    // light click

    return true;
}

// override base class method because we don't want the light to turn off if clicked again
bool HudColorButtonArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    // always turn on the button
    *m_pIsLit = true;
    m_lightShutoffTime = GetAbsoluteSimTime() + 0.25;  // light turns off in 1/4-second

    oapiToggleHUDColour();

    // play sound
    GetXR1().PlaySound(GetXR1().SwitchOn, DeltaGliderXR1::ST_Other, QUIET_CLICK); // light click

    return true;
}

void HudColorButtonArea::ProcessTimedEvent(bool &isLit, const bool previousIsLit, const double simt, const double simdt, const double mjd)
{
    // {YYY} TODO: resolve this for the XR2's VC
    if (IsVC() == false)    // no action in 3D mode
    {
        // turn off the light if timeout reached
        if (isLit && (simt >= m_lightShutoffTime))
        {
            isLit = false;
            TriggerRedraw();
        }
    }
}

//----------------------------------------------------------------------------------

//
// Constructor
// vessel = our vessel handle
// panelCoordinates = absolute coordinates of this area on the parent instrument panel
// areaID = unique Orbiter area ID
SecondaryHUDModeButtonsArea::SecondaryHUDModeButtonsArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    XR1Area(parentPanel, panelCoordinates, areaID)
{
}

// Activate this area
void SecondaryHUDModeButtonsArea::Activate()
{
    Area::Activate();  // invoke superclass method
    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(165, 15), PANEL_REDRAW_MOUSE, PANEL_MOUSE_LBDOWN | PANEL_MOUSE_ONREPLAY, PANEL_MAP_BACKGROUND);
    m_mainSurface = CreateSurface(IDB_LIGHT1); 
}

// Redraw this area
// event = Orbiter event flags
// returns: true if area redrawn, false if not
bool SecondaryHUDModeButtonsArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    int mode = GetXR1().m_secondaryHUDMode;
    if (mode > 0)
        oapiBlt(surf, m_mainSurface, (mode * 29) + 6, 0, 7, 0, 7, 7);

    return true;
}

// Handle mouse events for this area
// event = Orbiter event flags
// mx, my = mouse coordinates relative to the area
// returns: true if event processed, false if not
bool SecondaryHUDModeButtonsArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated or systems offline, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard() || GetXR1().m_internalSystemsFailure)
        return false;

    if (mx%29 < 20)     // allow for spacing between buttons
        GetXR1().EnableAndSetSecondaryHUDMode(mx / 29);  // (0...5); will play sound as well
        
    return true;
}

//----------------------------------------------------------------------------------

TertiaryHUDButtonArea::TertiaryHUDButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    XR1Area(parentPanel, panelCoordinates, areaID)
{
}

void TertiaryHUDButtonArea::Activate()
{
    Area::Activate();  // invoke superclass method
    if (IsVC()) // 3D panel?
    {
        // 3D support N/A
    }
    else    // 2D panel
    {
        oapiRegisterPanelArea(GetAreaID(), GetRectForSize(12, 12), PANEL_REDRAW_MOUSE | PANEL_REDRAW_USER, PANEL_MOUSE_LBDOWN);
        m_mainSurface = CreateSurface(IDB_LIGHT2); 
    }
}

bool TertiaryHUDButtonArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    int srcX = (GetXR1().m_tertiaryHUDOn ? 12 : 0);
    oapiBlt(surf, m_mainSurface, 0, 0, srcX, 0, 12, 12);
    
    return true;
}

bool TertiaryHUDButtonArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated or systems offline, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard() || GetXR1().m_internalSystemsFailure)
        return false;

    GetXR1().m_tertiaryHUDOn = !GetXR1().m_tertiaryHUDOn;   // toggle
    GetXR1().PlaySound(GetXR1().SwitchOn, DeltaGliderXR1::ST_Other, QUIET_CLICK);

    return true;
}

//----------------------------------------------------------------------------------

// Base class for all popup HUDs
//
// Constructor
// vessel = our vessel handle
// panelCoordinates = absolute coordinates of this area on the parent instrument panel
// areaID = unique Orbiter area ID
// pTextLineGroup = lines to pass to new TextBox object; if null, no TextBox will be created
// hudTurnedOn = reference to hud "on/off" switch value
PopupHUDArea::PopupHUDArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int width, const int height) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_pen0(0), m_state(OnOffState::Off), m_startScrollTime(-1), m_startScrollY(-1), m_movement(0), m_hBackgroundBrush(0),
    m_width(width), m_height(height), m_colorRef(0), m_bgColorRef(0), m_hlColorRef(0),
    m_topYCoordinate(height),  // HUD is OFF (one pixel off-area)
    m_lastRenderedTopYCoordinate(-1)
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
            MoveToEx(hDC, 0, m_height, NULL);                 // bottom-left corner  
            LineTo(hDC, 0, m_topYCoordinate);           

            MoveToEx(hDC, 0, m_topYCoordinate, NULL);           // top-left corner
            LineTo(hDC, m_width, m_topYCoordinate);     

            MoveToEx(hDC, m_width-1, m_topYCoordinate, NULL);   // top-right corner
            LineTo(hDC, m_width-1, m_height);

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

//----------------------------------------------------------------------------------

//
// Constructor
// vessel = our vessel handle
// panelCoordinates = absolute coordinates of this area on the parent instrument panel
// areaID = unique Orbiter area ID
SecondaryHUDArea::SecondaryHUDArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    PopupHUDArea(parentPanel, panelCoordinates, areaID, 209, 82),
    m_lastHUDMode(0), m_mainFont(0)
{
    // no need to set colors or font here; they will be set by Activate()
    m_lineSpacing = 11;     // pixels between lines
}

SecondaryHUDArea::~SecondaryHUDArea()
{
    // clean up the last font we allocated, if any
	if (m_mainFont != 0)
		DeleteObject(m_mainFont);
}

// returns TRUE if HUD is on.  NOTE: the HUD is not necessarily fully deployed!
bool SecondaryHUDArea::isOn()
{
    return (GetXR1().m_secondaryHUDMode > 0);
}

// Set HUD colors; invoked by the superclass before HUD rendering begins
void SecondaryHUDArea::SetHUDColors()
{
    // NOTE: HUD may be (turning) off here; if so, don't change the colors
    int mode = GetXR1().m_secondaryHUDMode;  // mode 1-5
    if (mode > 0)
    {
        const XR1ConfigFileParser &config = *GetXR1().GetXR1Config();
        const SecondaryHUDMode secondaryHUD = config.SecondaryHUD[mode-1];   // 0 < mode < 5

        // set the HUD colors 
        // there is no warning color, at least for now
        const COLORREF backgroundColor = secondaryHUD.GetBackgroundColor();
        SetColor(secondaryHUD.GetTextColor());    // normal color
        SetBackgroundColor(backgroundColor);

        // if the HUD mode has changed, recreate the font
        // We must do this here because we want an UNALIASED font if transparent
        // NOTE: do not use ANTIALIASED_QUALITY instead of '0' for the second parameter!  It looks better under Vista to leave it at 0 for some reason.
        if (mode != m_lastHUDMode)
        {
            const DWORD antialiasFlag = ((backgroundColor == 0xFFFFFF) ? NONANTIALIASED_QUALITY : 0);

            // release old font 
            if (m_mainFont != 0)
                DeleteObject(m_mainFont);

            // create new font
            m_mainFont = CreateFont(14, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, antialiasFlag, 0, "Arial");
        }
    }
}

// Render the contents of the HUD
// NOTE: the subclass MUST draw text from the supplied topY coordinate (plus some border gap space)
// The X coordinate is zero @ the border
// Returns: true if HUD was redrawn, false if not
bool SecondaryHUDArea::DrawHUD(const int event, const int topY, HDC hDC, COLORREF colorRef, bool forceRender)
{
    // NOTE: HUD may be off here if we are turning off!
    int mode = GetXR1().m_secondaryHUDMode;  // mode 1-5
    if (mode == 0)  // HUD off?
        mode = m_lastHUDMode;   // remember last active HUD mode
    else    // HUD is on
        m_lastHUDMode = mode;   // remember this

    const XR1ConfigFileParser &config = *GetXR1().GetXR1Config();
    SecondaryHUDMode secondaryHUD = config.SecondaryHUD[mode-1];   // 0 < mode < 5
    
    // set the font
    HFONT prevFont = (HFONT)SelectObject(hDC, m_mainFont);   // save previous font and select new font

    // set the text fg and bg colors
    const COLORREF bgColor = secondaryHUD.GetBackgroundColor();
    ::SetTextColor(hDC, secondaryHUD.GetTextColor());
    SetBkColor(hDC, bgColor);

    // NOTE: area was registered with PANEL_MAP_BACKGROUND, so we don't need to always repaint it
    // fill the background area if not transparent; this is to make the background solid between letters
    if (bgColor != CWHITE)
    {
        RECT r = { 0, m_topYCoordinate, m_width, m_height };
        FillRect(hDC, &r, m_hBackgroundBrush);
    }

    // set the background mode
    SetBkMode(hDC, ((GetBackgroundColor() == CWHITE) ? TRANSPARENT : OPAQUE));

    // render each cell on the HUD
    // NOTE: must render from the BOTTOM-UP so that the descenders render on each row
    for (int row=SH_ROW_COUNT - 1; row >= 0; row--)
    {
        RenderCell(hDC, secondaryHUD, row, 0, topY);   // left side
        RenderCell(hDC, secondaryHUD, row, 1, topY);   // right side
    }

    SelectObject(hDC, prevFont);   // restore previously selected font

    // We always redraw here because 1) it would be almost impossible to accurately track what changes, and
    // 2) we are only invoked at a fixed interval anyway.
    return true;
}

// Render a single cell on the secondary HUD
// row and column are NOT validated here; they were validated before
void SecondaryHUDArea::RenderCell(HDC hDC, SecondaryHUDMode &secondaryHUD, const int row, const int column, const int topY)
{
    SecondaryHUDMode::Cell &cell = secondaryHUD.GetCell(row, column);
    if (cell.pField == nullptr)
        return;     // cell is empty!

    // Populate the value and valueText in this cell from our parent vessel
    PopulateCell(cell);

    const int xOffset = 34;             // # columns from left to render ":" in "Alt:"; splits each column between label and value
    const int xCenter = m_width / 2;    // horizontal center of HUD

    // Render the label; e.g., "Altitude:"
    SetTextAlign(hDC, TA_RIGHT);
    int x = ((column == 0) ? xOffset : (xCenter + xOffset));
    int y = topY + 2 + (row * m_lineSpacing);    // must render from current top of HUD, since it may be scrolling; also allow some spacing from the HUD top

    char temp[MAX_CELL_LABEL_LENGTH + 2];       // allow room for ":"
    sprintf(temp, "%s:", cell.pField->label);  // "Alt:"
    TextOut(hDC, x, y, temp, static_cast<int>(strlen(temp)));   

    // Render the cell value
    SetTextAlign(hDC, TA_LEFT);
    x += 4;     // spacing between ":" and value
    const char *pStr = cell.valueStr;
    TextOut(hDC, x, y, pStr, static_cast<int>(strlen(pStr)));   // "102329 ft"
}

// Populate value and valueStr in the supplied cell
void SecondaryHUDArea::PopulateCell(SecondaryHUDMode::Cell &cell)
{
    const FieldID fieldID = cell.pField->id;
    const Units units = cell.units;
    double value = 0;   // reused below
    char valueStr[128];     // be sure that we never overrun the value buffer

    switch (fieldID)
    {
    case FieldID::Alt:
        value = GetXR1().GetAltitude(ALTMODE_GROUND); // in meters
        if (units == Units::u_met) // metric
        {
            // altitude will never be negative here
            if (value >= 1e7)   // >= 10 million meters (10,000 km)?
                sprintf(valueStr, "%.2lf mm", (value / 1e6));
            else if (value >= 3e4)   // >= 30 km?
                sprintf(valueStr, "%.3lf km", (value / 1e3));
            else
                sprintf(valueStr, "%.2lf m", value);
        }
        else    // imperial
        {
            value = MetersToFeet(value);
            // handle large mile distances here
            const double distInMiles = (value / 5280);
            if (fabs(distInMiles) >= 1e6)   // >= 1 million miles?
                sprintf(valueStr, "%.3lf mmi", (distInMiles / 1e6));  // do not clip
            else if (value > 407e3)  // > 407000 ft?
                sprintf(valueStr, "%.2lf mi", distInMiles);
            else
                sprintf(valueStr, "%.2lf ft", value);
        }
        break;

    case FieldID::Vel:
		value = GetXR1().GetGroundspeed();
        // velocity will never be negative 
        if (units == Units::u_met) // metric
        {
            sprintf(valueStr, "%.1lf m/s", value);
        }
        else if (units == Units::u_imp)   // imperial
        {
            value = MpsToMph(value);
            sprintf(valueStr, "%.1lf mph", value);
        }
        else if (units == Units::u_M)
        {
            value = GetXR1().GetMachNumber();
            sprintf(valueStr, "%.3lf Mach", value);  // cap @ 11 characters here b/c of clipping issue with "mach"
        }
        break;
    
    case FieldID::StatP:
    case FieldID::DynP:
        // in pascals
        value = ((fieldID == FieldID::StatP) ? GetXR1().GetAtmPressure() : GetXR1().GetDynPressure());
        if (units == Units::u_met) // metric
        {
            sprintf(valueStr, "%.4lf kPa", (value / 1000));
        }
        else // imperial
        {
            value = PaToPsi(value);
            sprintf(valueStr, "%.4lf psi", value);
        }
        break;

    case FieldID::OAT:
        value = GetXR1().GetExternalTemperature();   // Kelvin
        if (units == Units::u_K)
        {
            sprintf(valueStr, "%.4lf °K", value);
        }
        else if (units == Units::u_C)
        {
            value = KelvinToCelsius(value);
            sprintf(valueStr, "%.4lf °C", value);
        }
        else    // Fahrenheit
        {
            value = KelvinToFahrenheit(value);
            sprintf(valueStr, "%.4lf °F", value);
        }
        break;

    case FieldID::Hdg:
        {
            BOOL stat = oapiGetHeading(GetVessel().GetHandle(), &value);
            if (stat == FALSE)
                sprintf(valueStr, "---");
            else
            {
                sprintf(valueStr, "%.3lf°", (value * DEG));
            }
        }
        break;

    case FieldID::VS:
        {
            VECTOR3 v;
			GetXR1().GetAirspeedVector(FRAME_HORIZON, v);
            value = (GetXR1().GroundContact() ? 0 : v.y);      // in m/s
            if (units == Units::u_met) // metric
            {
                sprintf(valueStr, "%+.2lf m/s", value);
            }
            else // imperial
            {
                value = MetersToFeet(value);    // feet per second
                sprintf(valueStr, "%+.2lf fps", value);
            }
        }
        break;
     
    case FieldID::AccX:
    case FieldID::AccY:
    case FieldID::AccZ:
        {
            const VECTOR3 &A = GetXR1().m_acceleration;
            if (fieldID == FieldID::AccX)
                value = A.x;
            else if (fieldID == FieldID::AccY)
                value = A.y;
            else
                value = A.z;

            if (units == Units::u_met)  // metric
            {
                sprintf(valueStr, "%.4lf m/s²", value);
            }
            else if (units == Units::u_imp)    // imperial
            {
                value = MetersToFeet(value);
                sprintf(valueStr, "%.4lf fps²", value);
            }
            else  // G
            {
                value = Mps2ToG(value);
                sprintf(valueStr, "%.6lf G", value);
            }
        }
        break;

    case FieldID::Mass:
        value = GetXR1().GetMass(); // in kg
        const char *pFormatStr;
        if (units == Units::u_met) // metric
        {
            if (value > 999999.9)
                pFormatStr = "%.1lf kg";
            else if (value > 99999.9)
                pFormatStr = "%.2lf kg";
            else 
                pFormatStr = "%.3lf kg";
            
            sprintf(valueStr, "%.3lf kg", value);
        }
        else    // imperial
        {
            value = KgToPounds(value);
            
            if (value > 999999.9)
                pFormatStr = "%.1lf lb";
            else if (value > 99999.9)
                pFormatStr = "%.2lf lb";
            else
                pFormatStr = "%.3lf lb";

            sprintf(valueStr, pFormatStr, value);
        }
        break;

    case FieldID::Ecc:
        {
            ELEMENTS e;
            GetVessel().GetElements(NULL, e, NULL, 0, FRAME_EQU);  // this is only expensive on the first call to it in this frame
            value = e.e;
            sprintf(valueStr, "%.5lf", value);
        }
        break;

    case FieldID::Inc:
        {
            ELEMENTS e;
            GetVessel().GetElements(NULL, e, NULL, 0, FRAME_EQU);
            value = e.i * DEG;  // in degrees
            sprintf(valueStr, "%.4lf°", value);  // reduce to 11 chars for slight clipping issue
        }
        break;

    case FieldID::PeT:
    case FieldID::ApT:
        {
            ELEMENTS e;
            ORBITPARAM prm;
            GetVessel().GetElements(NULL, e, &prm, 0, FRAME_EQU);
            value = ((fieldID == FieldID::PeT) ? prm.PeT : prm.ApT);

            // if value < 0, it means that it is N/A; i.e., we are not orbiting the object
            if (value <= 0)
            {
                sprintf(valueStr, "N/A");
                break;
            }

            if (fabs(value) >= 1e7)  // >= 10,000,000 seconds?
                sprintf(valueStr, "%.4lf M", (value / 1e6));
            else if (fabs(value) >= 1e4)  // >= 10,000 seconds?
                sprintf(valueStr, "%.4lf K", (value / 1e3));
            else
                sprintf(valueStr, "%.2lf", value);
        }
        break;

    case FieldID::PeR:
    case FieldID::ApR:
    case FieldID::PeA:
    case FieldID::ApA:
        {
            // These values operate on the primary G body at the moment
            ELEMENTS e;
            ORBITPARAM prm;
            GetVessel().GetElements(NULL, e, &prm, 0, FRAME_EQU);
            value = (((fieldID == FieldID::PeR) || (fieldID == FieldID::PeA)) ? prm.PeD : prm.ApD);  // dist from body center in meters
            
            // if value <= 0, it means that it is N/A; i.e., we are not orbiting the object
            if (value <= 0)
            {
                sprintf(valueStr, "N/A");
                break;
            }

            // if we are displaying the ALTITUDE, we need to adjust for that
            if ((fieldID == FieldID::PeA) || (fieldID == FieldID::ApA))
            {
                const OBJHANDLE gRef = GetVessel().GetGravityRef();  // body we are orbiting
                double radius = oapiGetSize(gRef);  // radius of primary G body
                value -= radius;        // altitude in meters
            }

            // we have the distance in meters; display it
            if (units == Units::u_met)     // metric
            {
                if (fabs(value) >= 1e9)
                    sprintf(valueStr, "%.2lf gm", (value / 1e9));
                else if (fabs(value) >= 1e7)   // >= 10,000 km?
                    sprintf(valueStr, "%.2lf mm", (value / 1e6));
                else if (fabs(value) >= 1e3)
                    sprintf(valueStr, "%.2lf km", (value / 1e3));
                else
                    sprintf(valueStr, "%.2lf m", value);
            }
            else   // imperial
            {
                // convert to feet
                value = MetersToFeet(value);

                // handle large mile distances here
                const double distInMiles = (value / 5280);
                if (fabs(distInMiles) >= 1e9)   // >= 1 billion miles?
                    sprintf(valueStr, "%.3lf gmi", (distInMiles / 1e9));  // do not clip
                else if (fabs(distInMiles) >= 1e6)   // >= 1 million miles?
                    sprintf(valueStr, "%.3lf mmi", (distInMiles / 1e6));  // do not clip
                else if (fabs(value) >= 1e5)  // >= 100,000 feet?
                    sprintf(valueStr, "%.2lf mi", distInMiles);
                else
                    sprintf(valueStr, "%.2lf ft", value);
            }
        }
        break;

    case FieldID::Pitch:
    case FieldID::Bank:
    case FieldID::Slope:
    case FieldID::Slip:
    case FieldID::AOA:
        if (fieldID == FieldID::Pitch)
            value = GetVessel().GetPitch();
        else if (fieldID == FieldID::Bank)
            value = GetVessel().GetBank();
        else if (fieldID == FieldID::Slope)
            value = GetXR1().m_slope;
        else if (fieldID == FieldID::Slip)
            value = GetVessel().GetSlipAngle();
        else
            value = GetVessel().GetAOA();

        value *= DEG;   // convert to degrees
        sprintf(valueStr, "%+.3lf°", value);
        break;

    case FieldID::Long:
    case FieldID::Lat:
        {
            double longitude, latitude, radius;
            OBJHANDLE hObj = GetVessel().GetEquPos(longitude, latitude, radius);
            if (hObj == nullptr)
                sprintf(valueStr, "-----");     // no data available
            else
            {
                double pos = ((fieldID == FieldID::Long) ? longitude : latitude) * DEG;
                char dir;
                if (pos < 0)
                    dir = ((fieldID == FieldID::Long) ? 'W' : 'S');
                else
                    dir = ((fieldID == FieldID::Long) ? 'E' : 'N');

                sprintf(valueStr, "%.5lf° %c", fabs(pos), dir);
            }
        }
        break;

    case FieldID::LEng:
    case FieldID::REng:
    case FieldID::MEng:
    case FieldID::FHov:
    case FieldID::AHov:
    case FieldID::BHov:
    case FieldID::LScrm:
    case FieldID::RScrm:
    case FieldID::BScrm:
    case FieldID::rcs_1:
    case FieldID::rcs_2:
    case FieldID::rcs_3:
    case FieldID::rcs_4:
    case FieldID::rcs_5:
    case FieldID::rcs_6:
    case FieldID::rcs_7:
    case FieldID::rcs_8:
    case FieldID::rcs_9:
    case FieldID::rcs_10:
    case FieldID::rcs_11:
    case FieldID::rcs_12:
    case FieldID::rcs_13:
    case FieldID::rcs_14:
        {
#define GET_THRUST(handle) (GetXR1().GetThrusterLevel(GetXR1().handle) * GetXR1().GetThrusterMax(GetXR1().handle))
            if (fieldID == FieldID::LEng)
            {
                // test retro FIRST so we don't show "-0.00.." on the HUD
                value = -GET_THRUST(th_retro[0]);  // show as negative for retro thrust
                if (value == 0)
                    value = GET_THRUST(th_main[0]); 
            }
            else if (fieldID == FieldID::REng)
            {
                value = -GET_THRUST(th_retro[1]);  // retro
                if (value == 0)
                    value = GET_THRUST(th_main[1]);
            }
            else if (fieldID == FieldID::MEng)   //  both main engines
            {
                value = -GET_THRUST(th_retro[0]);   // retro
                if (value == 0)
                    value = GET_THRUST(th_main[0]); 

                double e1 = -GET_THRUST(th_retro[1]);
                if (e1 == 0)
                    e1 = GET_THRUST(th_main[1]); 

                value += e1;
            }
            else if (fieldID == FieldID::FHov)
                value = GET_THRUST(th_hover[0]);
            else if (fieldID == FieldID::AHov)
                value = GET_THRUST(th_hover[1]);
            else if (fieldID == FieldID::BHov)
                value = GET_THRUST(th_hover[0]) + GET_THRUST(th_hover[1]);
            else if (fieldID == FieldID::LScrm)
                value = GetXR1().ramjet->GetMostRecentThrust(0);
            else if (fieldID == FieldID::RScrm)
                value = GetXR1().ramjet->GetMostRecentThrust(1);
            else if (fieldID == FieldID::BScrm)
                value = GetXR1().ramjet->GetMostRecentThrust(0) + GetXR1().ramjet->GetMostRecentThrust(1);
            else    // it's an RCS jet
                value = GET_THRUST(th_rcs[static_cast<int>(fieldID) - static_cast<int>(FieldID::rcs_1)]);

            if (units == Units::u_imp)
                value = NewtonsToPounds(value);

            if (value >= 1000)
            {
                if (units == Units::u_met)
                    sprintf(valueStr, "%.3lf kN", value / 1000);
                else  // imperial
                    sprintf(valueStr, "%.3lf kLb", value / 1000);
            }
            else    // RCS thrust is very small
            {
                if (units == Units::u_met)
                    sprintf(valueStr, "%.3lf N", value);
                else  // imperial
                    sprintf(valueStr, "%.3lf lb", NewtonsToPounds(value));
            }
        }
        break;

    case FieldID::LDtmp:
    case FieldID::LCtmp:
    case FieldID::LEtmp:
    case FieldID::RDtmp:
    case FieldID::RCtmp:
    case FieldID::REtmp:
        {
            if (fieldID == FieldID::LDtmp)
                value = GetXR1().ramjet->Temp(0, 0);
            else if (fieldID == FieldID::LCtmp)
                value = GetXR1().ramjet->Temp(0, 1);
            else if (fieldID == FieldID::LEtmp)
                value = GetXR1().ramjet->Temp(0, 2);
            else if (fieldID == FieldID::RDtmp)
                value = GetXR1().ramjet->Temp(1, 0);
            else if (fieldID == FieldID::RCtmp)
                value = GetXR1().ramjet->Temp(1, 1);
            else if (fieldID == FieldID::REtmp)
                value = GetXR1().ramjet->Temp(1, 2);

            if (units == Units::u_K)
            {
                sprintf(valueStr, "%.3lf °K", value);
            }
            else if (units == Units::u_C)
            {
                value = KelvinToCelsius(value);
                sprintf(valueStr, "%.3lf °C", value);
            }
            else    // Fahrenheit
            {
                value = KelvinToFahrenheit(value);
                sprintf(valueStr, "%.3lf °F", value);
            }
        }
        break;

    default:        // should never happen!
        sprintf(valueStr, "??????");        // let the user know something is wrong
        break;
    }

    // now copy the rendered string into the official string, TRUNCATING it if necessary to prevent buffer overruns and a CTD!
    strncpy(cell.valueStr, valueStr, MAX_CELL_VALUE_LENGTH);
    cell.valueStr[MAX_CELL_VALUE_LENGTH] = 0;   // terminate
}

//----------------------------------------------------------------------------------

//
// Constructor
// vessel = our vessel handle
// panelCoordinates = absolute coordinates of this area on the parent instrument panel
// areaID = unique Orbiter area ID
TertiaryHUDArea::TertiaryHUDArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    PopupHUDArea(parentPanel, panelCoordinates, areaID, 209, 82)
{
    const XR1ConfigFileParser &config = *GetXR1().GetXR1Config();

    const int width = GetWidth(), height = GetHeight();
    SetTextBox(new TextBox(width-2, height, config.TertiaryHUDNormalColor, config.TertiaryHUDWarningColor, config.TertiaryHUDBackgroundColor, 
        7, GetXR1().m_infoWarningTextLineGroup));

    // create our font
    // NOTE: we want an ALIASED font for a non-transparent background, or UNALIASED font for transparent background
    const DWORD antialiasFlag = ((config.TertiaryHUDBackgroundColor== 0xFFFFFF) ? NONANTIALIASED_QUALITY : 0);
    m_mainFont = CreateFont(14, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, antialiasFlag, 0, "Arial");
    m_lineSpacing = 11;     // pixels between lines
}

TertiaryHUDArea::~TertiaryHUDArea()
{
    // clean up the text box we allocated
    delete GetTextBox();

    // clean up the font we allocated
    DeleteObject(m_mainFont);
}

// returns TRUE if HUD is on
bool TertiaryHUDArea::isOn()
{
    return GetXR1().m_tertiaryHUDOn;
}

// Set HUD colors; invoked by the superclass before HUD rendering begins
void TertiaryHUDArea::SetHUDColors()
{
    const XR1ConfigFileParser &config = *GetXR1().GetXR1Config();

    SetColor(config.TertiaryHUDNormalColor);    // normal color
    SetHighlightColor(config.TertiaryHUDWarningColor);
    SetBackgroundColor(config.TertiaryHUDBackgroundColor);

}

// Render the contents of the HUD
// NOTE: the subclass MUST draw text from the supplied topY coordinate (plus some border gap space)
// The X coordinate is zero @ the border
// Returns: true if text re-rendered, false if not
bool TertiaryHUDArea::DrawHUD(const int event, const int topY, HDC hDC, COLORREF colorRef, bool forceRender)
{
    // NOTE: area was registered with PANEL_MAP_BACKGROUND, so we don't need to always repaint it
    // fill the background area if not transparent; this is to make the background solid between letters
    if (GetXR1().GetXR1Config()->TertiaryHUDBackgroundColor != CWHITE)
    {
        RECT r = { 0, m_topYCoordinate, m_width, m_height };
        FillRect(hDC, &r, m_hBackgroundBrush);
    }

    // invoke new TextBox handler to draw text using a TRANSPARENT background; this same TextBox handler
    // can also be used on the lower panel to render on a normal screen.
    // Note that our text box will never be null here.
    return m_pTextBox->Render(hDC, topY, m_mainFont, m_lineSpacing, forceRender);  // CWHITE = use transparent background
}
