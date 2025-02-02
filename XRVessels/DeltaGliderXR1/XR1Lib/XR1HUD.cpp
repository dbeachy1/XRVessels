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
    oapi::Font *pOrgHUDFont = ((!IsCameraVC()) ? skp->SetFont(GetNormal2DHUDFont()) : nullptr);   

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
    for (const char **p = DATA_HUD_VALUES; *p; p++)
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
    TimedButtonArea(parentPanel, panelCoordinates, areaID),
    m_lightShutoffTime(-1)
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
