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

#include "resource.h"

#include "DeltaGliderXR1.h"
#include "XR1MultiDisplayArea.h"

// Note: as of D3D9 RC23 there is no difference in framerate between sketchpad and GetDC on this 
// MDA area. In addition, the font control isn't quite as precise under sketchpad (FF_MODERN fonts looks a lot
// better in the MDA), so I am keeping the GetDC version for now.
#if 1  // GetDC
// Constructor
HullTempsMultiDisplayMode::HullTempsMultiDisplayMode(int modeNumber) :
    MultiDisplayMode(modeNumber),
    m_backgroundSurface(0), m_indicatorSurface(0)
{
    m_kfcButtonCoord.x = 24;
    m_kfcButtonCoord.y = 25;
}

void HullTempsMultiDisplayMode::Activate()
{
    m_backgroundSurface = CreateSurface(IDB_HULL_TEMP_MULTI_DISPLAY);
    m_indicatorSurface = CreateSurface(IDB_INDICATOR2);
    m_pParentMDA->SetSurfaceColorKey(m_indicatorSurface, CWHITE);

    m_pKfcFont = CreateFont(14, 0, 0, 0, 600, 0, 0, 0, 0, 0, 0, 0, FF_MODERN, "Microsoft Sans Serif");
    m_pCoolantFont = CreateFont(12, 0, 0, 0, 600, 0, 0, 0, 0, 0, 0, 0, FF_MODERN, "Microsoft Sans Serif");
}

void HullTempsMultiDisplayMode::Deactivate()
{
    DestroySurface(&m_backgroundSurface);
    DestroySurface(&m_indicatorSurface);
    DeleteObject(m_pKfcFont);
    DeleteObject(m_pCoolantFont);
}

bool HullTempsMultiDisplayMode::Redraw2D(const int event, const SURFHANDLE surf)
{
    // Always re-render everything; it is too error-prone to try to track all values and clear any 
    // old data underneath from the previous render.

    // 
    // Render the graphics
    // NOTE: must render these BEFORE any text, or the graphics will not paint because of the SelectObject call.
    //

    // render the background
    const COORD2& screenSize = GetScreenSize();
    DeltaGliderXR1::SafeBlt(surf, m_backgroundSurface, 0, 0, 0, 0, screenSize.x, screenSize.y);

    // detect the highest temperature percentage of all surfaces
    double highestTempFrac = GetHighestTempFrac();  // max percentage of any hull temperature to its limit

    // Render the hull temperature limits gauge; cannot go negative since it is in degrees K
    if (highestTempFrac > 1.0)
        highestTempFrac = 1.0;   // keep gauge in range

    int maxIndex = 83;   // total height = 84 pixels (index 0-83, inclusive)
    int index = static_cast<int>((maxIndex * highestTempFrac) + 0.5);  // round to nearest pixel
    int tgtY = 102 - index;   // center-3 pixels
    //      tgt,  src,                tgtx,tgty,srcx,srcy,w,h, <use predefined color key>
    DeltaGliderXR1::SafeBlt(surf, m_indicatorSurface, 8, tgtY, 0, 0, 6, 7, SURF_PREDEF_CK);

    // Render the coolant temperature gauge
    // round to nearest pixel
    const double coolantTemp = GetXR1().m_coolantTemp;  // in degrees C
    double frac = (coolantTemp - MIN_COOLANT_GAUGE_TEMP) / (MAX_COOLANT_GAUGE_TEMP - MIN_COOLANT_GAUGE_TEMP);

    // keep gauge in range
    if (frac < 0.0)
        frac = 0;
    else if (frac > 1.0)
        frac = 1.0;

    maxIndex = 72;      // 0-72 inclusive
    index = static_cast<int>((maxIndex * frac) + 0.5);
    tgtY = 91 - index;   // center-3 pixels
    //      tgt,  src,                tgtx,tgty,srcx,srcy,w,h, <use predefined color key>
    DeltaGliderXR1::SafeBlt(surf, m_indicatorSurface, 165, tgtY, 6, 0, 6, 7, SURF_PREDEF_CK);

    // 
    // Now draw the text
    //

    // NOTE: according to jarmonik invoking oapiClearSurface before a GetDC improves performance on the D3D9 client,
    // but unfortunately we can't do that here since we are rendering on top of a background that we blitted.

    // obtain device context and save existing font
    HDC hDC = m_pParentMDA->GetDC(surf);
    HFONT hPrevObject = (HFONT)SelectObject(hDC, m_pKfcFont);

    // render our K/F/C button temp label if necessary
    SetBkMode(hDC, TRANSPARENT);
    SetTextColor(hDC, CREF(LIGHT_BLUE));  // use CREF macro to convert to Windows' Blue, Green, Red COLORREF
    SetTextAlign(hDC, TA_LEFT);
    char* pScale;
    if (GetXR1().m_activeTempScale == TempScale::Kelvin)
        pScale = "°K";
    else if (GetXR1().m_activeTempScale == TempScale::Celsius)
        pScale = "°C";
    else
        pScale = "°F";

    TextOut(hDC, 35, 22, pScale, static_cast<int>(strlen(pScale)));

    char tempStr[12];   // temperature string; reused for each temperature; includes 1 extra char

    // EXT 
    SetTextColor(hDC, CREF(OFF_WHITE192));
    SetTextAlign(hDC, TA_CENTER);
    GetTemperatureStr(GetXR1().GetExternalTemperature(), tempStr);
    TextOut(hDC, 142, 36, tempStr, static_cast<int>(strlen(tempStr)));

    const HullTemperatureLimits& limits = GetXR1().m_hullTemperatureLimits;

    // NOSECONE 
    SetTextColor(hDC, GetTempCREF(GetXR1().m_noseconeTemp, limits.noseCone, GetNoseDoorStatus()));
    SetTextAlign(hDC, TA_CENTER);
    GetTemperatureStr(GetXR1().m_noseconeTemp, tempStr);
    TextOut(hDC, 91, 22, tempStr, static_cast<int>(strlen(tempStr)));

    // LEFT WING
    const int wingY = 57;
    SetTextColor(hDC, GetTempCREF(GetXR1().m_leftWingTemp, limits.wings, GetLeftWingDoorStatus()));
    SetTextAlign(hDC, TA_RIGHT);
    GetTemperatureStr(GetXR1().m_leftWingTemp, tempStr);
    TextOut(hDC, 65, wingY, tempStr, static_cast<int>(strlen(tempStr)));

    // RIGHT WING
    SetTextColor(hDC, GetTempCREF(GetXR1().m_rightWingTemp, limits.wings, GetRightWingDoorStatus()));
    SetTextAlign(hDC, TA_LEFT);
    GetTemperatureStr(GetXR1().m_rightWingTemp, tempStr);
    TextOut(hDC, 119, wingY, tempStr, static_cast<int>(strlen(tempStr)));

    // COCKPIT
    SetTextColor(hDC, GetTempCREF(GetXR1().m_cockpitTemp, limits.cockpit, GetCockpitDoorStatus()));
    SetTextAlign(hDC, TA_RIGHT);
    GetTemperatureStr(GetXR1().m_cockpitTemp, tempStr);
    TextOut(hDC, 78, 38, tempStr, static_cast<int>(strlen(tempStr)));

    // TOP HULL
    SetTextColor(hDC, GetTempCREF(GetXR1().m_topHullTemp, limits.topHull, GetTopHullDoorStatus()));
    SetTextAlign(hDC, TA_CENTER);
    GetTemperatureStr(GetXR1().m_topHullTemp, tempStr);
    TextOut(hDC, 91, 75, tempStr, static_cast<int>(strlen(tempStr)));

    // COOL (coolant temperature)
    SelectObject(hDC, m_pCoolantFont);       // use smaller font
    SetTextColor(hDC, GetValueCREF(coolantTemp, WARN_COOLANT_TEMP, CRITICAL_COOLANT_TEMP));  // do not round value
    SetTextAlign(hDC, TA_LEFT);
    GetCoolantTemperatureStr(coolantTemp, tempStr);
    TextOut(hDC, 134, 82, tempStr, static_cast<int>(strlen(tempStr)));

    // restore previous font and release device context
    SelectObject(hDC, hPrevObject);
    m_pParentMDA->ReleaseDC(surf, hDC);

    return true;
}
#else // SKETCHPAD INTERFACE

//
// Refactored to use the new sketchpad interface for improved performance under the D3D9 client
//

// Constructor
HullTempsMultiDisplayMode::HullTempsMultiDisplayMode(int modeNumber) :
    MultiDisplayMode(modeNumber),
    m_backgroundSurface(0), m_indicatorSurface(0)
{
    m_kfcButtonCoord.x = 24;
    m_kfcButtonCoord.y = 25;
}

void HullTempsMultiDisplayMode::Activate()
{
    m_backgroundSurface = CreateSurface(IDB_HULL_TEMP_MULTI_DISPLAY);
    m_indicatorSurface = CreateSurface(IDB_INDICATOR2);
    m_pParentMDA->SetSurfaceColorKey(m_indicatorSurface, CWHITE);

    m_pKfcFont = oapiCreateFont(15, true, "Microsoft Sans Serif", FONT_BOLD);  // was 14 for GetDC
    m_pCoolantFont = oapiCreateFont(13, true, "Microsoft Sans Serif", FONT_BOLD);  // was 12 for GetDC
}

void HullTempsMultiDisplayMode::Deactivate()
{
    DestroySurface(&m_backgroundSurface);
    DestroySurface(&m_indicatorSurface);
    oapiReleaseFont(m_pKfcFont);
    oapiReleaseFont(m_pCoolantFont);
}

bool HullTempsMultiDisplayMode::Redraw2D(const int event, const SURFHANDLE surf)
{
    // Always re-render everything; it is too error-prone to try to track all values and clear any 
    // old data underneath from the previous render.

    // 
    // Render the graphics
    // NOTE: must render these BEFORE any text, or the graphics will not paint because of the SelectObject call.
    //

    // render the background
    const COORD2& screenSize = GetScreenSize();
    DeltaGliderXR1::SafeBlt(surf, m_backgroundSurface, 0, 0, 0, 0, screenSize.x, screenSize.y);

    // detect the highest temperature percentage of all surfaces
    double highestTempFrac = GetHighestTempFrac();  // max percentage of any hull temperature to its limit

    // Render the hull temperature limits gauge; cannot go negative since it is in degrees K
    if (highestTempFrac > 1.0)
        highestTempFrac = 1.0;   // keep gauge in range

    int maxIndex = 83;   // total height = 84 pixels (index 0-83, inclusive)
    int index = static_cast<int>((maxIndex * highestTempFrac) + 0.5);  // round to nearest pixel
    int tgtY = 102 - index;   // center-3 pixels
    //      tgt,  src,                tgtx,tgty,srcx,srcy,w,h, <use predefined color key>
    DeltaGliderXR1::SafeBlt(surf, m_indicatorSurface, 8, tgtY, 0, 0, 6, 7, SURF_PREDEF_CK);

    // Render the coolant temperature gauge
    // round to nearest pixel
    const double coolantTemp = GetXR1().m_coolantTemp;  // in degrees C
    double frac = (coolantTemp - MIN_COOLANT_GAUGE_TEMP) / (MAX_COOLANT_GAUGE_TEMP - MIN_COOLANT_GAUGE_TEMP);

    // keep gauge in range
    if (frac < 0.0)
        frac = 0;
    else if (frac > 1.0)
        frac = 1.0;

    maxIndex = 72;      // 0-72 inclusive
    index = static_cast<int>((maxIndex * frac) + 0.5);
    tgtY = 91 - index;   // center-3 pixels
    //      tgt,  src,                tgtx,tgty,srcx,srcy,w,h, <use predefined color key>
    DeltaGliderXR1::SafeBlt(surf, m_indicatorSurface, 165, tgtY, 6, 0, 6, 7, SURF_PREDEF_CK);

    // obtain sketchpad and save existing font
    oapi::Sketchpad* skp = oapiGetSketchpad(surf);
    oapi::Font* pPrevFont = skp->SetFont(m_pKfcFont);

    // render our K/F/C button temp label if necessary
    skp->SetBackgroundMode(oapi::Sketchpad::BK_TRANSPARENT);
    skp->SetTextColor(CREF(LIGHT_BLUE));
    skp->SetTextAlign(oapi::Sketchpad::LEFT);

    const int fontSizeDelta = -1;  // text Y coordinate adjustment for larger font size for sketchpad

    char* pScale;
    if (GetXR1().m_activeTempScale == Kelvin)
        pScale = "°K";
    else if (GetXR1().m_activeTempScale == Celsius)
        pScale = "°C";
    else
        pScale = "°F";

    skp->Text(35, 22 + fontSizeDelta, pScale, static_cast<int>(strlen(pScale)));

    char tempStr[12];   // temperature string; reused for each temperature; includes 1 extra char

    // EXT 
    skp->SetTextColor(CREF(OFF_WHITE192));
    skp->SetTextAlign(oapi::Sketchpad::CENTER);
    GetTemperatureStr(GetXR1().GetExternalTemperature(), tempStr);
    skp->Text(142, 36 + fontSizeDelta, tempStr, static_cast<int>(strlen(tempStr)));

    const HullTemperatureLimits& limits = GetXR1().m_hullTemperatureLimits;

    // NOSECONE 
    skp->SetTextColor(GetTempCREF(GetXR1().m_noseconeTemp, limits.noseCone, GetNoseDoorStatus()));
    skp->SetTextAlign(oapi::Sketchpad::CENTER);
    GetTemperatureStr(GetXR1().m_noseconeTemp, tempStr);
    skp->Text(91, 22 + fontSizeDelta, tempStr, static_cast<int>(strlen(tempStr)));

    // LEFT WING
    const int wingY = 57;
    skp->SetTextColor(GetTempCREF(GetXR1().m_leftWingTemp, limits.wings, GetLeftWingDoorStatus()));
    skp->SetTextAlign(oapi::Sketchpad::RIGHT);
    GetTemperatureStr(GetXR1().m_leftWingTemp, tempStr);
    skp->Text(65, wingY + fontSizeDelta, tempStr, static_cast<int>(strlen(tempStr)));

    // RIGHT WING
    skp->SetTextColor(GetTempCREF(GetXR1().m_rightWingTemp, limits.wings, GetRightWingDoorStatus()));
    skp->SetTextAlign(oapi::Sketchpad::LEFT);
    GetTemperatureStr(GetXR1().m_rightWingTemp, tempStr);
    skp->Text(119, wingY + fontSizeDelta, tempStr, static_cast<int>(strlen(tempStr)));

    // COCKPIT
    skp->SetTextColor(GetTempCREF(GetXR1().m_cockpitTemp, limits.cockpit, GetCockpitDoorStatus()));
    skp->SetTextAlign(oapi::Sketchpad::RIGHT);
    GetTemperatureStr(GetXR1().m_cockpitTemp, tempStr);
    skp->Text(78, 38 + fontSizeDelta, tempStr, static_cast<int>(strlen(tempStr)));

    // TOP HULL
    skp->SetTextColor(GetTempCREF(GetXR1().m_topHullTemp, limits.topHull, GetTopHullDoorStatus()));
    skp->SetTextAlign(oapi::Sketchpad::CENTER);
    GetTemperatureStr(GetXR1().m_topHullTemp, tempStr);
    skp->Text(91, 75 + fontSizeDelta, tempStr, static_cast<int>(strlen(tempStr)));

    // COOL (coolant temperature)
    skp->SetFont(m_pCoolantFont);       // use smaller font
    skp->SetTextColor(GetValueCREF(coolantTemp, WARN_COOLANT_TEMP, CRITICAL_COOLANT_TEMP));  // do not round value
    skp->SetTextAlign(oapi::Sketchpad::LEFT);
    GetCoolantTemperatureStr(coolantTemp, tempStr);
    skp->Text(134, 82 + fontSizeDelta, tempStr, static_cast<int>(strlen(tempStr)));

    // restore previous font and release sketchpad
    skp->SetFont(pPrevFont);
    oapiReleaseSketchpad(skp);

    return true;
}
#endif  // GetDC / Sketchpad impls

// returns the highest temperature fraction for any surface (0...n).  
double HullTempsMultiDisplayMode::GetHighestTempFrac()
{
    const HullTemperatureLimits& limits = GetXR1().m_hullTemperatureLimits;
    double highestTempFrac = 0.0;    // max percentage of any hull temperature to its limit

    // if a surface's door is open, its limits will be lower
#define IS_DOOR_OPEN(status) (status != DoorStatus::DOOR_CLOSED)   // includes DOOR_FAILED
#define LIMITK(limitK, doorStatus)  (IS_DOOR_OPEN(doorStatus) ? limits.doorOpen : limitK)
#define SET_MAX_PCT(tempK, limitK, doorStatus)  { double pct = (tempK / LIMITK(limitK, doorStatus)); if (pct > highestTempFrac) highestTempFrac = pct; }

    // nosecone, hover doors, and gear use nosecone limit
    SET_MAX_PCT(GetXR1().m_noseconeTemp, limits.noseCone, GetXR1().nose_status);
    SET_MAX_PCT(GetXR1().m_noseconeTemp, limits.noseCone, GetXR1().hoverdoor_status);
    SET_MAX_PCT(GetXR1().m_noseconeTemp, limits.noseCone, GetXR1().gear_status);

    SET_MAX_PCT(GetXR1().m_leftWingTemp, limits.wings, GetXR1().rcover_status);
    SET_MAX_PCT(GetXR1().m_rightWingTemp, limits.wings, GetXR1().rcover_status);
    SET_MAX_PCT(GetXR1().m_cockpitTemp, limits.cockpit, GetXR1().hatch_status);
    SET_MAX_PCT(GetXR1().m_topHullTemp, limits.topHull, GetXR1().radiator_status);

    return highestTempFrac;
}

// convert coolant temperature in C to a displayable string 
// NOTE: pStrOut must be at least 7 characters in length; that is the max characters rendered including the null
void HullTempsMultiDisplayMode::GetCoolantTemperatureStr(double tempC, char* pStrOut)
{
    double tempConverted;

    // sanity-check the incoming value so we never exceed five (123.45) characters + decimal, regardless of the temp scale
    if (tempC < 0)
        tempC = 0;
    else if (tempC > MAX_COOLANT_TEMP)
        tempC = MAX_COOLANT_TEMP;

    switch (GetXR1().m_activeTempScale)
    {
    case TempScale::Kelvin:
        tempConverted = m_pParentMDA->CelsiusToKelvin(tempC);
        break;

    case TempScale::Fahrenheit:
        tempConverted = m_pParentMDA->CelsiusToFahrenheit(tempC);
        break;

    default:    // Celsius
        tempConverted = tempC;
        break;
    }

    // keep in range
    if (tempConverted > 999.9)
        tempConverted = 999.9;
    else if (tempConverted < -99.9)
        tempConverted = -99.9;

    // Do not round the value!  We need to match the warning PostStep exactly, and rounding up makes us arrive early.
    // HOWEVER: NOTE THAT sprintf will round the value!
    sprintf(pStrOut, "%.1lf°", tempConverted);
}

// convert temperature in K to a displayable string 
// NOTE: pStrOut must be at least 11 characters in length; that is the max characters rendered including the null
void HullTempsMultiDisplayMode::GetTemperatureStr(double tempK, char* pStrOut)
{
    double tempConverted;

    // sanity-check the incoming value in case something make the temps go nuts
    if (tempK < 0)
        tempK = 0;  // cannot be colder than absolute zero

    switch (GetXR1().m_activeTempScale)
    {
    case TempScale::Kelvin:
        tempConverted = tempK;
        break;

    case TempScale::Fahrenheit:
        tempConverted = m_pParentMDA->KelvinToFahrenheit(tempK);
        break;

    default:    // Celsius
        tempConverted = m_pParentMDA->KelvinToCelsius(tempK);
        break;
    }

    // keep in range; no need to check for NEGATIVE values here because of absolute zero
    // must do range check BEFORE doing the rounding; otherwise it is converted to an integer first, which may overflow
    if (tempConverted > 99999.9)
        tempConverted = 99999.9;

    // Do not round the value!  We want to match the damage code exactly (although it is technically not critical), and rounding up makes us arrive early.

    sprintf(pStrOut, "%.1lf°", tempConverted);
}

bool HullTempsMultiDisplayMode::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    bool processed = false;

    COORD2 c = { mx, my };

    if (event & PANEL_MOUSE_LBDOWN)
    {
        // check K/F/C button
        if (c.InBounds(m_kfcButtonCoord, 7, 7))
        {
            if (GetXR1().m_activeTempScale == TempScale::Celsius)
                GetXR1().m_activeTempScale = TempScale::Fahrenheit;
            else if (GetXR1().m_activeTempScale == TempScale::Fahrenheit)
                GetXR1().m_activeTempScale = TempScale::Kelvin;
            else    // it's Kelvin
                GetXR1().m_activeTempScale = TempScale::Celsius;

            GetXR1().PlaySound(GetXR1()._MDMButtonUp, DeltaGliderXR1::ST_Other);
            processed = true;
        }
    }

    return processed;
}

//----------------------------------------------------------------------------------

// determines which door(s) to use for temperature display warning colors
#define CHECK_AND_RETURN_DOOR(doorStatus) if (doorStatus != DoorStatus::DOOR_CLOSED) return doorStatus
DoorStatus HullTempsMultiDisplayMode::GetNoseDoorStatus()
{
    CHECK_AND_RETURN_DOOR(GetXR1().nose_status);
    CHECK_AND_RETURN_DOOR(GetXR1().hoverdoor_status);
    CHECK_AND_RETURN_DOOR(GetXR1().gear_status);

    return DoorStatus::DOOR_CLOSED;  // no open doors for this surface
}

DoorStatus HullTempsMultiDisplayMode::GetLeftWingDoorStatus()
{
    return GetXR1().rcover_status;
}

DoorStatus HullTempsMultiDisplayMode::GetRightWingDoorStatus()
{
    return GetXR1().rcover_status;
}

DoorStatus HullTempsMultiDisplayMode::GetCockpitDoorStatus()
{
    return GetXR1().hatch_status;
}

DoorStatus HullTempsMultiDisplayMode::GetTopHullDoorStatus()
{
    return GetXR1().radiator_status;
}
