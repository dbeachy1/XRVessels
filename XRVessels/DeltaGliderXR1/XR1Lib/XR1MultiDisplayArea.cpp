// ==============================================================
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// Copyright 2006-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
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
    if (dir == UP)
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
                SwitchActiveMode(UP);
                GetXR1().PlaySound(GetXR1()._MDMButtonUp, DeltaGliderXR1::ST_Other);
            }
            else if (mouseOverPrevButton)
            {
                SwitchActiveMode(DOWN);
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

//----------------------------------------------------------------------------------
// MULTI-DISPLAY MODES BEGIN HERE
//----------------------------------------------------------------------------------

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

    m_pKfcFont     = CreateFont(14, 0, 0, 0, 600, 0, 0, 0, 0, 0, 0, 0, FF_MODERN, "Microsoft Sans Serif");
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
    const COORD2 &screenSize = GetScreenSize();
    oapiBlt(surf, m_backgroundSurface, 0, 0, 0, 0, screenSize.x, screenSize.y);

    // detect the highest temperature percentage of all surfaces
    double highestTempFrac = GetHighestTempFrac();  // max percentage of any hull temperature to its limit

    // Render the hull temperature limits gauge; cannot go negative since it is in degrees K
    if (highestTempFrac > 1.0)
        highestTempFrac = 1.0;   // keep gauge in range

    int maxIndex = 83;   // total height = 84 pixels (index 0-83, inclusive)
    int index = static_cast<int>((maxIndex * highestTempFrac) + 0.5);  // round to nearest pixel
    int tgtY = 102 - index;   // center-3 pixels
    //      tgt,  src,                tgtx,tgty,srcx,srcy,w,h, <use predefined color key>
    oapiBlt(surf, m_indicatorSurface, 8,   tgtY, 0,  0,   6, 7, SURF_PREDEF_CK);

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
    oapiBlt(surf, m_indicatorSurface, 165, tgtY, 6,  0,   6, 7, SURF_PREDEF_CK);

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
    char *pScale;
    if (GetXR1().m_activeTempScale == Kelvin)
        pScale = "°K";
    else if (GetXR1().m_activeTempScale == Celsius)
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

    const HullTemperatureLimits &limits = GetXR1().m_hullTemperatureLimits;

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

    m_pKfcFont     = oapiCreateFont(15, true, "Microsoft Sans Serif", FONT_BOLD);  // was 14 for GetDC
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
    const COORD2 &screenSize = GetScreenSize();
    oapiBlt(surf, m_backgroundSurface, 0, 0, 0, 0, screenSize.x, screenSize.y);

    // detect the highest temperature percentage of all surfaces
    double highestTempFrac = GetHighestTempFrac();  // max percentage of any hull temperature to its limit

    // Render the hull temperature limits gauge; cannot go negative since it is in degrees K
    if (highestTempFrac > 1.0)
        highestTempFrac = 1.0;   // keep gauge in range

    int maxIndex = 83;   // total height = 84 pixels (index 0-83, inclusive)
    int index = static_cast<int>((maxIndex * highestTempFrac) + 0.5);  // round to nearest pixel
    int tgtY = 102 - index;   // center-3 pixels
    //      tgt,  src,                tgtx,tgty,srcx,srcy,w,h, <use predefined color key>
    oapiBlt(surf, m_indicatorSurface, 8,   tgtY, 0,  0,   6, 7, SURF_PREDEF_CK);

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
    oapiBlt(surf, m_indicatorSurface, 165, tgtY, 6,  0,   6, 7, SURF_PREDEF_CK);

    // obtain sketchpad and save existing font
    oapi::Sketchpad *skp = oapiGetSketchpad(surf);
    oapi::Font *pPrevFont = skp->SetFont(m_pKfcFont);

    // render our K/F/C button temp label if necessary
    skp->SetBackgroundMode(oapi::Sketchpad::BK_TRANSPARENT);
    skp->SetTextColor(CREF(LIGHT_BLUE));
    skp->SetTextAlign(oapi::Sketchpad::LEFT);

    const int fontSizeDelta = -1;  // text Y coordinate adjustment for larger font size for sketchpad

    char *pScale;
    if (GetXR1().m_activeTempScale == Kelvin)
        pScale = "°K";
    else if (GetXR1().m_activeTempScale == Celsius)
        pScale = "°C";
    else
        pScale = "°F";

    skp->Text(35, 22+fontSizeDelta, pScale, static_cast<int>(strlen(pScale)));

    char tempStr[12];   // temperature string; reused for each temperature; includes 1 extra char

    // EXT 
    skp->SetTextColor(CREF(OFF_WHITE192));
    skp->SetTextAlign(oapi::Sketchpad::CENTER);
    GetTemperatureStr(GetXR1().GetExternalTemperature(), tempStr);
    skp->Text(142, 36+fontSizeDelta, tempStr, static_cast<int>(strlen(tempStr)));

    const HullTemperatureLimits &limits = GetXR1().m_hullTemperatureLimits;

    // NOSECONE 
    skp->SetTextColor(GetTempCREF(GetXR1().m_noseconeTemp, limits.noseCone, GetNoseDoorStatus()));
    skp->SetTextAlign(oapi::Sketchpad::CENTER);
    GetTemperatureStr(GetXR1().m_noseconeTemp, tempStr);
    skp->Text(91, 22+fontSizeDelta, tempStr, static_cast<int>(strlen(tempStr)));

    // LEFT WING
    const int wingY = 57;
    skp->SetTextColor(GetTempCREF(GetXR1().m_leftWingTemp, limits.wings, GetLeftWingDoorStatus()));
    skp->SetTextAlign(oapi::Sketchpad::RIGHT);
    GetTemperatureStr(GetXR1().m_leftWingTemp, tempStr);
    skp->Text(65, wingY+fontSizeDelta, tempStr, static_cast<int>(strlen(tempStr)));

    // RIGHT WING
    skp->SetTextColor(GetTempCREF(GetXR1().m_rightWingTemp, limits.wings, GetRightWingDoorStatus()));
    skp->SetTextAlign(oapi::Sketchpad::LEFT);
    GetTemperatureStr(GetXR1().m_rightWingTemp, tempStr);
    skp->Text(119, wingY+fontSizeDelta, tempStr, static_cast<int>(strlen(tempStr)));

    // COCKPIT
    skp->SetTextColor(GetTempCREF(GetXR1().m_cockpitTemp, limits.cockpit, GetCockpitDoorStatus()));
    skp->SetTextAlign(oapi::Sketchpad::RIGHT);
    GetTemperatureStr(GetXR1().m_cockpitTemp, tempStr);
    skp->Text(78, 38+fontSizeDelta, tempStr, static_cast<int>(strlen(tempStr)));

    // TOP HULL
    skp->SetTextColor(GetTempCREF(GetXR1().m_topHullTemp, limits.topHull, GetTopHullDoorStatus()));
    skp->SetTextAlign(oapi::Sketchpad::CENTER);
    GetTemperatureStr(GetXR1().m_topHullTemp, tempStr);
    skp->Text(91, 75+fontSizeDelta, tempStr, static_cast<int>(strlen(tempStr)));
    
    // COOL (coolant temperature)
    skp->SetFont(m_pCoolantFont);       // use smaller font
    skp->SetTextColor(GetValueCREF(coolantTemp, WARN_COOLANT_TEMP, CRITICAL_COOLANT_TEMP));  // do not round value
    skp->SetTextAlign(oapi::Sketchpad::LEFT);
    GetCoolantTemperatureStr(coolantTemp, tempStr);
    skp->Text(134, 82+fontSizeDelta, tempStr, static_cast<int>(strlen(tempStr)));

    // restore previous font and release sketchpad
    skp->SetFont(pPrevFont);
    oapiReleaseSketchpad(skp);   

    return true;
}
#endif  // GetDC / Sketchpad impls

// returns the highest temperature fraction for any surface (0...n).  
double HullTempsMultiDisplayMode::GetHighestTempFrac()
{
    const HullTemperatureLimits &limits = GetXR1().m_hullTemperatureLimits;
    double highestTempFrac = 0.0;    // max percentage of any hull temperature to its limit

    // if a surface's door is open, its limits will be lower
#define IS_DOOR_OPEN(status) (status != DOOR_CLOSED)   // includes DOOR_FAILED
#define LIMITK(limitK, doorStatus)  (IS_DOOR_OPEN(doorStatus) ? limits.doorOpen : limitK)
#define SET_MAX_PCT(tempK, limitK, doorStatus)  { double pct = (tempK / LIMITK(limitK, doorStatus)); if (pct > highestTempFrac) highestTempFrac = pct; }

    // nosecone, hover doors, and gear use nosecone limit
    SET_MAX_PCT(GetXR1().m_noseconeTemp,  limits.noseCone, GetXR1().nose_status);
    SET_MAX_PCT(GetXR1().m_noseconeTemp,  limits.noseCone, GetXR1().hoverdoor_status);  
    SET_MAX_PCT(GetXR1().m_noseconeTemp,  limits.noseCone, GetXR1().gear_status);  

    SET_MAX_PCT(GetXR1().m_leftWingTemp,  limits.wings, GetXR1().rcover_status);
    SET_MAX_PCT(GetXR1().m_rightWingTemp, limits.wings, GetXR1().rcover_status);
    SET_MAX_PCT(GetXR1().m_cockpitTemp,   limits.cockpit, GetXR1().hatch_status);
    SET_MAX_PCT(GetXR1().m_topHullTemp,   limits.topHull, GetXR1().radiator_status);

    return highestTempFrac;
}

// convert coolant temperature in C to a displayable string 
// NOTE: pStrOut must be at least 7 characters in length; that is the max characters rendered including the null
void HullTempsMultiDisplayMode::GetCoolantTemperatureStr(double tempC, char *pStrOut)
{
    double tempConverted;

    // sanity-check the incoming value so we never exceed five (123.45) characters + decimal, regardless of the temp scale
    if (tempC < 0)
        tempC = 0; 
    else if (tempC > MAX_COOLANT_TEMP)
        tempC = MAX_COOLANT_TEMP;

    switch(GetXR1().m_activeTempScale)
    {
    case Kelvin:
        tempConverted = m_pParentMDA->CelsiusToKelvin(tempC);
        break;

    case Fahrenheit:
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
void HullTempsMultiDisplayMode::GetTemperatureStr(double tempK, char *pStrOut)
{
    double tempConverted;

    // sanity-check the incoming value in case something make the temps go nuts
    if (tempK < 0)
        tempK = 0;  // cannot be colder than absolute zero

    switch(GetXR1().m_activeTempScale)
    {
    case Kelvin:
        tempConverted = tempK;
        break;

    case Fahrenheit:
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
            if (GetXR1().m_activeTempScale == Celsius)
                GetXR1().m_activeTempScale = Fahrenheit;
            else if (GetXR1().m_activeTempScale == Fahrenheit)
                GetXR1().m_activeTempScale = Kelvin;
            else    // it's Kelvin
                GetXR1().m_activeTempScale = Celsius;

            GetXR1().PlaySound(GetXR1()._MDMButtonUp, DeltaGliderXR1::ST_Other);
            processed = true;
        }
    }

    return processed;
}

//----------------------------------------------------------------------------------

// This class handles all systems status screens, using the delta from MDMID_SYSTEMS_STATUS1 to determine which screen we are
// Constructor
SystemsStatusMultiDisplayMode::SystemsStatusMultiDisplayMode(int modeNumber) :
    MultiDisplayMode(modeNumber), 
    m_backgroundSurface(0)
{
    m_screenIndex = modeNumber - MDMID_SYSTEMS_STATUS1;   // index 0...n
}

void SystemsStatusMultiDisplayMode::Activate()
{
    static const int resourceIDs[] = { IDB_SYSTEMS_STATUS1_MULTI_DISPLAY, IDB_SYSTEMS_STATUS2_MULTI_DISPLAY, 
                                       IDB_SYSTEMS_STATUS3_MULTI_DISPLAY, IDB_SYSTEMS_STATUS4_MULTI_DISPLAY,
                                       IDB_SYSTEMS_STATUS5_MULTI_DISPLAY };

    m_backgroundSurface = CreateSurface(resourceIDs[m_screenIndex]);
    m_mainFont = CreateFont(14, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, 0, FF_MODERN, "Microsoft Sans Serif");
    m_fontPitch = 11;
}
 
void SystemsStatusMultiDisplayMode::Deactivate()
{
    DestroySurface(&m_backgroundSurface);
    DeleteObject(m_mainFont);
}

bool SystemsStatusMultiDisplayMode::Redraw2D(const int event, const SURFHANDLE surf)
{
    // render the background
    const COORD2 &screenSize = GetScreenSize();
    oapiBlt(surf, m_backgroundSurface, 0, 0, 0, 0, screenSize.x, screenSize.y);

    // obtain device context and save existing font
    HDC hDC = m_pParentMDA->GetDC(surf);
    HFONT hPrevObject = (HFONT)SelectObject(hDC, m_mainFont);

    SetBkMode(hDC, TRANSPARENT);
    SetTextAlign(hDC, TA_LEFT);

    // set starting coordinates 
    int x = 5;
    int y = 20;
    int statusX = 136;  // "OK", "OFFLINE", "32%", etc.

    static const int linesPerScreen = 7; 
    DamageItem start = (DamageItem)(LeftWing + (m_screenIndex * linesPerScreen));
    char temp[64];
    for (int i=0; i < linesPerScreen; i++)
    {
        DamageItem damageItem = (DamageItem)(start + i);
        if (damageItem > D_END)
            break;  // no more items

        DamageStatus damageStatus = GetXR1().GetDamageStatus(damageItem);

        double integrity = damageStatus.fracIntegrity;

        if (integrity == 1.0)
            SetTextColor(hDC, CREF(MEDIUM_GREEN));
        else
            SetTextColor(hDC, CREF(BRIGHT_RED));

        sprintf(temp, "%s:", damageStatus.label);
        TextOut(hDC, x, y, temp, static_cast<int>(strlen(temp))); // "Left Wing", etc.

        if (damageStatus.onlineOffline)
        {
            if (integrity < 1.0)
                strcpy(temp, "OFFLINE");
            else 
                strcpy(temp, "ONLINE");
        }
        else    // can have partial failure
        {
            sprintf(temp, "%d%%", static_cast<int>(integrity * 100));
        }

        TextOut(hDC, statusX, y, temp, static_cast<int>(strlen(temp)));

        // drop to next line
        y += m_fontPitch;
    }

    // restore previous font and release device context
    SelectObject(hDC, hPrevObject);
    m_pParentMDA->ReleaseDC(surf, hDC);

    return true;
}

//-------------------------------------------------------------------------

AttitudeHoldMultiDisplayMode::AttitudeHoldMultiDisplayMode(int modeNumber) :
    MultiDisplayMode(modeNumber), 
    m_backgroundSurface(0), m_mouseHoldTargetSimt(-1), m_lastAction(ACT_NONE), m_repeatCount(0)
{
    m_engageButtonCoord.x = 6;
    m_engageButtonCoord.y = 42;

    m_toggleAOAPitchCoord.x = 169;
    m_toggleAOAPitchCoord.y = 28;

    m_pitchUpArrowSmallCoord.x = 166;
    m_pitchUpArrowSmallCoord.y  = 41;

    m_pitchUpArrowLargeCoord.x = 149;
    m_pitchUpArrowLargeCoord.y  = 41;
    
    m_pitchDownArrowSmallCoord.x = 166;
    m_pitchDownArrowSmallCoord.y = 50;

    m_pitchDownArrowLargeCoord.x = 149;
    m_pitchDownArrowLargeCoord.y = 50;

    m_bankLeftArrowCoord.x  = 124;
    m_bankLeftArrowCoord.y  = 86;

    m_bankRightArrowCoord.x = 169;
    m_bankRightArrowCoord.y = 86;

    m_resetBankButtonCoord.x = 78;
    m_resetBankButtonCoord.y = 99;

    m_resetPitchButtonCoord.x = 6;
    m_resetPitchButtonCoord.y = 88;

    m_resetBothButtonCoord.x = 6;
    m_resetBothButtonCoord.y = 99;

    m_syncButtonCoord.x = 78;
    m_syncButtonCoord.y = 88;

    m_repeatSpeed = 0.125;  // seconds between clicks if mouse held down
}

void AttitudeHoldMultiDisplayMode::Activate()
{
    m_backgroundSurface = CreateSurface(IDB_ATTITUDE_HOLD_MULTI_DISPLAY);

    m_statusFont =   CreateFont(12, 0, 0, 0, 600, 0, 0, 0, 0, 0, 0, 0, FF_MODERN, "Microsoft Sans Serif");  // ENGAGED or DISENGAGED
    m_numberFont =   CreateFont(12, 0, 0, 0, 600, 0, 0, 0, 0, 0, 0, 0, FF_MODERN, "Microsoft Sans Serif");  // bank/pitch number text
    m_buttonFont =   CreateFont(12, 0, 0, 0, 600, 0, 0, 0, 0, 0, 0, 0, FF_MODERN, "Microsoft Sans Serif");  // engage/disengage button text
    m_aoaPitchFont = CreateFont(10, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, 0, FF_MODERN, "Arial");  // "Hold Pitch", "Hold AOA" text
}
 
void AttitudeHoldMultiDisplayMode::Deactivate()
{
    DestroySurface(&m_backgroundSurface);
    DeleteObject(m_statusFont);
    DeleteObject(m_numberFont);
    DeleteObject(m_buttonFont);
    DeleteObject(m_aoaPitchFont);
}

bool AttitudeHoldMultiDisplayMode::Redraw2D(const int event, const SURFHANDLE surf)
{
    // Always re-render everything; it is too error-prone to try to track all values and clear any 
    // old data underneath from the previous render.
    
    const bool holdAOA = GetXR1().m_holdAOA;        // for convenience

    // render the background
    const COORD2 &screenSize = GetScreenSize();
    oapiBlt(surf, m_backgroundSurface, 0, 0, 0, 0, screenSize.x, screenSize.y);

    // obtain device context and save existing font
    HDC hDC = m_pParentMDA->GetDC(surf);
    HFONT hPrevObject = (HFONT)SelectObject(hDC, m_statusFont); // will render status text first
    SetBkMode(hDC, TRANSPARENT);
    SetTextAlign(hDC, TA_LEFT);     // default to LEFT alignment

    // render autopilot status
    const char *pStatus;        // set below
    COLORREF statusColor;
    const bool engaged = (GetXR1().m_customAutopilotMode == AP_ATTITUDEHOLD);
    if (engaged && (GetXR1().m_customAutopilotSuspended))
    {
        pStatus = "SUSPENDED";
        statusColor = CREF(BRIGHT_WHITE);
    }
    else  // normal operation
    {
        pStatus = (engaged ? "ENGAGED" : "DISENGAGED");
        statusColor = (engaged ? CREF(BRIGHT_GREEN) : CREF(BRIGHT_RED));  // use CREF macro to convert to Windows' Blue, Green, Red COLORREF
    }
    SetTextColor(hDC, statusColor);
    TextOut(hDC, 46, 24, pStatus, static_cast<int>(strlen(pStatus)));

    // render "Set Pitch" or "Set AOA" text
    SelectObject(hDC, m_aoaPitchFont); 
    SetTextAlign(hDC, TA_RIGHT);     // RIGHT alignment
    const char *pSetText = (holdAOA ? "SET AOA" : "SET PITCH");
    SetTextColor(hDC, CREF((holdAOA ? BRIGHT_YELLOW : BRIGHT_GREEN)));
    TextOut(hDC, 165, 26, pSetText, static_cast<int>(strlen(pSetText)));
    SetTextAlign(hDC, TA_LEFT);     // restore to default to LEFT alignment

    // render button text
    SelectObject(hDC, m_buttonFont); 
    const char *pEngageDisengage = (engaged ? "Disengage" : "Engage");
    SetTextColor(hDC, CREF(LIGHT_BLUE));
    TextOut(hDC, 27, 43, pEngageDisengage, static_cast<int>(strlen(pEngageDisengage)));

    // render ship's current pitch, bank, and AOA
    SelectObject(hDC, m_numberFont); 
    SetTextColor(hDC, CREF(OFF_WHITE217));
    char temp[15];
    sprintf(temp, "%+7.2f°", GetVessel().GetPitch() * DEG);
    TextOut(hDC, 31, 61, temp, static_cast<int>(strlen(temp)));
    
    sprintf(temp, "%+7.2f°", GetVessel().GetBank() * DEG);
    TextOut(hDC, 31, 72, temp, static_cast<int>(strlen(temp)));

    sprintf(temp, "%+7.2f°", GetVessel().GetAOA() * DEG);
    TextOut(hDC, 98, 61, temp, static_cast<int>(strlen(temp)));

    // render "ZERO PITCH" or "ZERO AOA"
    SelectObject(hDC, m_aoaPitchFont); 
    const char *pZeroText = (holdAOA ? "ZERO AOA" : "ZERO PITCH");
    SetTextColor(hDC, CREF((holdAOA ? BRIGHT_YELLOW : BRIGHT_GREEN)));
    TextOut(hDC, 18, 86, pZeroText, static_cast<int>(strlen(pZeroText)));

    // render SET pitch/aoa and bank values; these values will be limited to +-90 degrees at the most
    SelectObject(hDC, m_numberFont); 
    
    SetTextAlign(hDC, TA_RIGHT);
    SetTextColor(hDC, engaged ? CREF((holdAOA ? BRIGHT_YELLOW : BRIGHT_GREEN)) : CREF(LIGHT_BLUE));
    sprintf(temp, "%+5.1f°", GetXR1().m_setPitchOrAOA);  // already in degrees
    TextOut(hDC, 143, 41, temp, static_cast<int>(strlen(temp)));
    
    SetTextAlign(hDC, TA_CENTER);
    SetTextColor(hDC, engaged ? CREF(BRIGHT_GREEN) : CREF(LIGHT_BLUE));
    sprintf(temp, "%+5.1f°", GetXR1().m_setBank);  // already in degrees
    TextOut(hDC, 151, 83, temp, static_cast<int>(strlen(temp)));

    // restore previous font and release device context
    SelectObject(hDC, hPrevObject);
    m_pParentMDA->ReleaseDC(surf, hDC);

    return true;
}

bool AttitudeHoldMultiDisplayMode::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    bool processed = false;
    bool playSound = false;  // play sound in button processing
    bool changeAxis = true;  // change axis value in button processing

    COORD2 c = { mx, my };

    if (event & PANEL_MOUSE_LBDOWN)
    {
        m_repeatCount = 0;  // reset just in case

        // check engage/disengage button
        if (c.InBounds(m_engageButtonCoord, 14, 14))
        {
            // toggle autopilot status
            GetXR1().ToggleAttitudeHold();
            processed = true;
            playSound = true;
        }
        else if (c.InBounds(m_toggleAOAPitchCoord, 7, 7))   // toggle AOA / Pitch hold
        {
            GetXR1().ToggleAOAPitchAttitudeHold(true);
            processed = true;
        }
        else if (c.InBounds(m_resetBankButtonCoord, 7, 7))    // reset bank button
        {
            GetXR1().ResetAttitudeHoldToLevel(true, true, false);
            processed = true;
        }
        else if (c.InBounds(m_resetBankButtonCoord, 7, 7))    // reset bank button
        {
            GetXR1().ResetAttitudeHoldToLevel(true, true, false);
            processed = true;
        }
        else if (c.InBounds(m_resetPitchButtonCoord, 7, 7))    // reset pitch/aoa button
        {
            GetXR1().ResetAttitudeHoldToLevel(true, false, true);
            processed = true;
        }
        else if (c.InBounds(m_syncButtonCoord, 7, 7))    // sync to current attitude
        {
            GetXR1().SyncAttitudeHold(true, false);  // do not force PITCH mode here
            processed = true;
        }
        else if (c.InBounds(m_resetBothButtonCoord, 7, 7))    // reset BOTH button
        {
            GetXR1().ResetAttitudeHoldToLevel(true, true, true);
            processed = true;
        }
    }

    // check axis buttons
    AXIS_ACTION action = ACT_NONE;
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

            // next click if mouse held down is one second from now
            m_mouseHoldTargetSimt = simt + 1.0;
        }

        // check whether we reached our target hold time
        if ((m_mouseHoldTargetSimt > 0) && (simt >= m_mouseHoldTargetSimt))
        {
            doButtonClick = true;
            m_mouseHoldTargetSimt = simt + m_repeatSpeed;   // process another event if mouse held down long enough
            m_repeatCount++;        // remember this
        }

        // check pitch and bank arrows
        if (c.InBounds(m_pitchUpArrowSmallCoord, 6, 7))
        {
            if (doButtonClick)
            {
                // use PILOT controls (reverse up/down arrows)
                m_lastAction = action = DECPITCH_SMALL;
            }
        }
        else if (c.InBounds(m_pitchDownArrowSmallCoord, 6, 7))
        {
            if (doButtonClick)
            {
                // use PILOT controls (reverse up/down arrows)
                m_lastAction = action = INCPITCH_SMALL;
            }
        }
        else if (c.InBounds(m_pitchUpArrowLargeCoord, 6, 7))
        {
            if (doButtonClick)
            {
                // use PILOT controls (reverse up/down arrows)
                m_lastAction = action = DECPITCH_LARGE;
            }
        }
        else if (c.InBounds(m_pitchDownArrowLargeCoord, 6, 7))
        {
            if (doButtonClick)
            {
                // use PILOT controls (reverse up/down arrows)
                m_lastAction = action = INCPITCH_LARGE;
            }
        }
        else if (c.InBounds(m_bankLeftArrowCoord, 6, 7))
        {
            if (doButtonClick)
            {
                m_lastAction = action = INCBANK;
            }
        }
        else if (c.InBounds(m_bankRightArrowCoord, 6, 7))
        {
            if (doButtonClick)
            {
                m_lastAction = action = DECBANK;
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
            changeAxis = false; // ...but don't actually change the value
            m_repeatCount = 0;  // reset
        }
        
        m_lastAction = ACT_NONE;  // reset
    }

    if (action != ACT_NONE)
    {
        const bool invertPitchArrows = GetXR1().GetXR1Config()->InvertAttitudeHoldPitchArrows;
        switch (action)
        {
        case INCPITCH_SMALL:
            if (invertPitchArrows) goto decpitch_small;
incpitch_small:
            GetXR1().IncrementAttitudeHoldPitch(playSound, changeAxis, AP_PITCH_DELTA_SMALL);
            processed = true;
            break;

        case DECPITCH_SMALL:
            if (invertPitchArrows) goto incpitch_small;
decpitch_small:
            GetXR1().DecrementAttitudeHoldPitch(playSound, changeAxis, AP_PITCH_DELTA_SMALL);
            processed = true;
            break;

        case INCPITCH_LARGE:
            if (invertPitchArrows) goto decpitch_large;
incpitch_large:
            GetXR1().IncrementAttitudeHoldPitch(playSound, changeAxis, AP_PITCH_DELTA_LARGE);
            processed = true;
            break;

        case DECPITCH_LARGE:
            if (invertPitchArrows) goto incpitch_large;
decpitch_large:
            GetXR1().DecrementAttitudeHoldPitch(playSound, changeAxis, AP_PITCH_DELTA_LARGE);
            processed = true;
            break;

        case INCBANK:
            GetXR1().IncrementAttitudeHoldBank(playSound, changeAxis);
            processed = true;
            break;

        case DECBANK:
            GetXR1().DecrementAttitudeHoldBank(playSound, changeAxis);
            processed = true;
            break;

        default:
            // no action
            break;
        }
    }
                    
    return processed;
}

//-------------------------------------------------------------------------

// Constructor
DescentHoldMultiDisplayMode::DescentHoldMultiDisplayMode(int modeNumber) :
    MultiDisplayMode(modeNumber), 
    m_backgroundSurface(0), m_mouseHoldTargetSimt(-1), m_lastAction(ACT_NONE), m_repeatCount(0)
{
    m_engageButtonCoord.x = 6;
    m_engageButtonCoord.y = 42;

    m_rateUp1ArrowCoord.x = 159;
    m_rateUp1ArrowCoord.y  = 47;
    
    m_rateDown1ArrowCoord.x = 159;
    m_rateDown1ArrowCoord.y = 56;

    m_rateUp5ArrowCoord.x = 143;
    m_rateUp5ArrowCoord.y  = 47;
    
    m_rateDown5ArrowCoord.x = 143;
    m_rateDown5ArrowCoord.y = 56;

    m_rateUp25ArrowCoord.x = 127;
    m_rateUp25ArrowCoord.y  = 47;
    
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
    const COORD2 &screenSize = GetScreenSize();
    oapiBlt(surf, m_backgroundSurface, 0, 0, 0, 0, screenSize.x, screenSize.y);

    // obtain device context and save existing font
    HDC hDC = m_pParentMDA->GetDC(surf);
    HFONT hPrevObject = (HFONT)SelectObject(hDC, m_statusFont); // will render status text first
    SetBkMode(hDC, TRANSPARENT);
    SetTextAlign(hDC, TA_LEFT);     // default to LEFT alignment

    // render autopilot status
    const char *pStatus;        // set below
    COLORREF statusColor;
    const bool engaged = (GetXR1().m_customAutopilotMode == AP_DESCENTHOLD);
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
    const char *pEngageDisengage = (engaged ? "Disengage" : "Engage");
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
            GetXR1().SetAutoDescentRate(true, AD_LEVEL, 0);    // switch to HOVER mode
            processed = true;
        }
        else if (c.InBounds(m_autoLandButtonCoord, 7, 7))    // AUTO-LAND button
        {
            // only enabled if descent hold autopilot is currently ENGAGED 
            if (GetXR1().m_customAutopilotMode == AP_DESCENTHOLD)
                GetXR1().SetAutoDescentRate(true, AD_AUTOLAND, 0);
            else
            {
                // cannot enable auto-descent; autopilot not engaged
                GetXR1().PlaySound(GetXR1().Error1, DeltaGliderXR1::ST_Other, ERROR1_VOL);
                GetXR1().ShowWarning(NULL, DeltaGliderXR1::ST_None, "Descent Hold autopilot not engaged.");
            }

            processed = true;
        }
    }

    // check rate buttons
    RATE_ACTION action = ACT_NONE;
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
                m_lastAction = action = DECRATE1;
            }
        }
        else if (c.InBounds(m_rateDown1ArrowCoord, 6, 7))
        {
            if (doButtonClick)
            {
                if (invertRateArrows) goto decrate1;
incrate1:
                m_lastAction = action = INCRATE1;
            }
        }
        else if (c.InBounds(m_rateUp5ArrowCoord, 6, 7))
        {
            if (doButtonClick)
            {
                if (invertRateArrows) goto incrate5;
decrate5:
                m_lastAction = action = DECRATE5;
            }
        }
        else if (c.InBounds(m_rateDown5ArrowCoord, 6, 7))
        {
            if (doButtonClick)
            {
                if (invertRateArrows) goto decrate5;
incrate5:
                m_lastAction = action = INCRATE5;
            }
        }
        else if (c.InBounds(m_rateUp25ArrowCoord, 6, 7))
        {
            if (doButtonClick)
            {
                if (invertRateArrows) goto incrate25; 
decrate25:
                m_lastAction = action = DECRATE25;
            }
        }
        else if (c.InBounds(m_rateDown25ArrowCoord, 6, 7))
        {
            if (doButtonClick)
            {
                if (invertRateArrows) goto decrate25;
incrate25:
                m_lastAction = action = INCRATE25;
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
        
        m_lastAction = ACT_NONE;  // reset
    }

    if (action != ACT_NONE)
    {
        switch (action)
        {
        case INCRATE1:
            GetXR1().SetAutoDescentRate(playSound, AD_ADJUST, ADRATE_SMALL);
            processed = true;
            break;

        case DECRATE1:
            GetXR1().SetAutoDescentRate(playSound, AD_ADJUST, -ADRATE_SMALL);
            processed = true;
            break;

        case INCRATE5:
            GetXR1().SetAutoDescentRate(playSound, AD_ADJUST, ADRATE_MED);
            processed = true;
            break;

        case DECRATE5:
            GetXR1().SetAutoDescentRate(playSound, AD_ADJUST, -ADRATE_MED);
            processed = true;
            break;

        case INCRATE25:
            GetXR1().SetAutoDescentRate(playSound, AD_ADJUST, ADRATE_LARGE);
            processed = true;
            break;

        case DECRATE25:
            GetXR1().SetAutoDescentRate(playSound, AD_ADJUST, -ADRATE_LARGE);
            processed = true;
            break;

        default:
            // no action
            break;
        }
    }
                    
    return processed;
}

//-------------------------------------------------------------------------

#define ROLLING_AVG_SIZE ((sizeof(m_maxMainAccRollingAvg) / sizeof(double)))

// Constructor
AirspeedHoldMultiDisplayMode::AirspeedHoldMultiDisplayMode(int modeNumber) :
    MultiDisplayMode(modeNumber), 
    m_backgroundSurface(0), m_mouseHoldTargetSimt(-1), m_lastAction(ACT_NONE), m_repeatCount(0)
{
    m_engageButtonCoord.x = 6;
    m_engageButtonCoord.y = 42;

    m_rateUpP1ArrowCoord.x = 166;
    m_rateUpP1ArrowCoord.y  = 47;

    m_rateDownP1ArrowCoord.x = 166;
    m_rateDownP1ArrowCoord.y = 56;

    m_rateUp1ArrowCoord.x = 153;
    m_rateUp1ArrowCoord.y  = 47;
    
    m_rateDown1ArrowCoord.x = 153;
    m_rateDown1ArrowCoord.y = 56;

    m_rateUp5ArrowCoord.x = 140;
    m_rateUp5ArrowCoord.y  = 47;
    
    m_rateDown5ArrowCoord.x = 140;
    m_rateDown5ArrowCoord.y = 56;

    m_rateUp25ArrowCoord.x = 127;
    m_rateUp25ArrowCoord.y  = 47;
    
    m_rateDown25ArrowCoord.x = 127;
    m_rateDown25ArrowCoord.y = 56;
    
    m_holdCurrentButtonCoord.x = 113;
    m_holdCurrentButtonCoord.y = 77;

    m_resetButtonCoord.x = 113;
    m_resetButtonCoord.y = 88;

    m_repeatSpeed = 0.0625;  // seconds between clicks if mouse held down: 16 clicks per second

    // Note: 10 frames is not enough here: it still jumps in the thousanth's place
    m_pMaxMainAccRollingArray = new RollingArray(20);  // average last 20 frame values
}

// Destructor
AirspeedHoldMultiDisplayMode::~AirspeedHoldMultiDisplayMode()
{
    delete m_pMaxMainAccRollingArray;
}

void AirspeedHoldMultiDisplayMode::Activate()
{
    m_backgroundSurface = CreateSurface(IDB_AIRSPEED_HOLD_MULTI_DISPLAY);

    m_statusFont = CreateFont(12, 0, 0, 0, 600, 0, 0, 0, 0, 0, 0, 0, FF_MODERN, "Microsoft Sans Serif");  // ENGAGED or DISENGAGED
    m_numberFont = CreateFont(12, 0, 0, 0, 600, 0, 0, 0, 0, 0, 0, 0, FF_MODERN, "Microsoft Sans Serif");  // set airspeed number text
    m_buttonFont = CreateFont(12, 0, 0, 0, 600, 0, 0, 0, 0, 0, 0, 0, FF_MODERN, "Microsoft Sans Serif");  // engage/disengage button text
}
 
void AirspeedHoldMultiDisplayMode::Deactivate()
{
    DestroySurface(&m_backgroundSurface);
    DeleteObject(m_statusFont);
    DeleteObject(m_numberFont);
    DeleteObject(m_buttonFont);
}

bool AirspeedHoldMultiDisplayMode::Redraw2D(const int event, const SURFHANDLE surf)
{
    // Always re-render everything; it is too error-prone to try to track all values and clear any 
    // old data underneath from the previous render.
        
    // render the background
    const COORD2 &screenSize = GetScreenSize();
    oapiBlt(surf, m_backgroundSurface, 0, 0, 0, 0, screenSize.x, screenSize.y);

    // obtain device context and save existing font
    HDC hDC = m_pParentMDA->GetDC(surf);
    HFONT hPrevObject = (HFONT)SelectObject(hDC, m_statusFont); // will render status text first
    SetBkMode(hDC, TRANSPARENT);
    SetTextAlign(hDC, TA_LEFT);     // default to LEFT alignment

    // render autopilot status
    const char *pStatus;        // set below
    COLORREF statusColor;
    const bool engaged = GetXR1().m_airspeedHoldEngaged;
    if (engaged && (GetXR1().m_airspeedHoldSuspended))
    {
        pStatus = "SUSPENDED";
        statusColor = CREF(BRIGHT_WHITE);
    }
    else  // normal operation
    {
        pStatus = (engaged ? "ENGAGED" : "DISENGAGED");
        statusColor = (engaged ? CREF(BRIGHT_GREEN) : CREF(BRIGHT_RED));  // use CREF macro to convert to Windows' Blue, Green, Red COLORREF
    }
    SetTextColor(hDC, statusColor);
    TextOut(hDC, 46, 24, pStatus, static_cast<int>(strlen(pStatus)));

    // render button text
    SelectObject(hDC, m_buttonFont); 
    const char *pEngageDisengage = (engaged ? "Disengage" : "Engage");
    SetTextColor(hDC, CREF(LIGHT_BLUE));
    TextOut(hDC, 27, 43, pEngageDisengage, static_cast<int>(strlen(pEngageDisengage)));

    SelectObject(hDC, m_numberFont); 
    SetTextColor(hDC, CREF(OFF_WHITE217));
    char temp[15];

    // airspeed 
    double airspeed = GetXR1().GetAirspeed();  // in m/s; we are holding KIAS here, NOT groundspeed!

    // keep in range
    if (airspeed > 99999.9)
        airspeed = 99999.9;
    else if (airspeed < 0)
        airspeed = 0;     // sanity-check
    sprintf(temp, "%-.1f m/s", airspeed);
    TextOut(hDC, 48, 62, temp, static_cast<int>(strlen(temp)));

    // imperial airspeed 
    double airspeedImp = XR1Area::MpsToMph(airspeed);
    if (airspeedImp > 99999.9)
        airspeedImp = 99999.9;
    else if (airspeedImp < 0)
        airspeedImp = 0;     // sanity-check
    sprintf(temp, "%-.1f mph", airspeedImp);
    TextOut(hDC, 48, 73, temp, static_cast<int>(strlen(temp)));

    // max main engine acc based on ship mass + atm drag
    // NOTE: this is a ROLLING AVERAGE over the last n frames to help the jumping around the Orbiter does with the acc values
    m_pMaxMainAccRollingArray->AddSample(GetXR1().m_maxMainAcc);
    const double maxMainAcc = m_pMaxMainAccRollingArray->GetAverage();   // overall average for all samples

    if (fabs(maxMainAcc) > 99.999)        // keep in range
        sprintf(temp, "------ m/s²");
    else
        sprintf(temp, "%.3f m/s²", maxMainAcc);
    COLORREF cref;  // reused below as well
    if (maxMainAcc <= 0)
        cref = CREF(MEDB_RED);
    else if (maxMainAcc < 1.0)
        cref = CREF(BRIGHT_YELLOW);
    else 
        cref = CREF(BRIGHT_GREEN);
    SetTextColor(hDC, cref);
    TextOut(hDC, 62, 95, temp, static_cast<int>(strlen(temp)));

    // main thrust pct 
    double mainThrustFrac = GetVessel().GetThrusterGroupLevel(THGROUP_MAIN);  // do not round this; sprintf will do it
    double mainThrustPct = (mainThrustFrac * 100.0);
    sprintf(temp, "%.3f%%", mainThrustPct);
    if (mainThrustPct >= 100)
        cref = CREF(MEDB_RED);
    else if (mainThrustPct >= 90)
        cref = CREF(BRIGHT_YELLOW);
    else 
        cref = CREF(BRIGHT_GREEN);

    SetTextColor(hDC, cref);
    TextOut(hDC, 62, 84, temp, static_cast<int>(strlen(temp)));    

    // render the set airspeed
    sprintf(temp, "%.1lf", GetXR1().m_setAirspeed);
    SetTextAlign(hDC, TA_RIGHT);
    SetTextColor(hDC, CREF(LIGHT_BLUE));
    TextOut(hDC, 121, 48, temp, static_cast<int>(strlen(temp)));
    
    // restore previous font and release device context
    SelectObject(hDC, hPrevObject);
    m_pParentMDA->ReleaseDC(surf, hDC);

    return true;
}

bool AirspeedHoldMultiDisplayMode::ProcessMouseEvent(const int event, const int mx, const int my)
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
            GetXR1().SetAirspeedHoldMode(!GetXR1().m_airspeedHoldEngaged, true);
            
            processed = true;
            playSound = true;
        }
        else if (c.InBounds(m_holdCurrentButtonCoord, 7, 7))   // HOLD CURRENT button
        {
            GetXR1().SetAirspeedHold(true, AS_HOLDCURRENT, 0);
            processed = true;
        }
        else if (c.InBounds(m_resetButtonCoord, 7, 7))   // RESET button
        {
            GetXR1().SetAirspeedHold(true, AS_RESET, 0);
            processed = true;
        }
    }

    // check rate buttons
    RATE_ACTION action = ACT_NONE;
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
        if (c.InBounds(m_rateUpP1ArrowCoord, 6, 7))
        {
            if (doButtonClick)
            {
                m_lastAction = action = INCRATEP1;
            }
        }
        else if (c.InBounds(m_rateDownP1ArrowCoord, 6, 7))
        {
            if (doButtonClick)
            {
                m_lastAction = action = DECRATEP1;
            }
        }
        else if (c.InBounds(m_rateUp1ArrowCoord, 6, 7))
        {
            if (doButtonClick)
            {
                m_lastAction = action = INCRATE1;
            }
        }
        else if (c.InBounds(m_rateDown1ArrowCoord, 6, 7))
        {
            if (doButtonClick)
            {
                m_lastAction = action = DECRATE1;
            }
        }
        else if (c.InBounds(m_rateUp5ArrowCoord, 6, 7))
        {
            if (doButtonClick)
            {
                m_lastAction = action = INCRATE5;
            }
        }
        else if (c.InBounds(m_rateDown5ArrowCoord, 6, 7))
        {
            if (doButtonClick)
            {
                m_lastAction = action = DECRATE5;
            }
        }
        else if (c.InBounds(m_rateUp25ArrowCoord, 6, 7))
        {
            if (doButtonClick)
            {
                m_lastAction = action = INCRATE25;
            }
        }
        else if (c.InBounds(m_rateDown25ArrowCoord, 6, 7))
        {
            if (doButtonClick)
            {
                m_lastAction = action = DECRATE25;
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
        
        m_lastAction = ACT_NONE;  // reset
    }

    if (action != ACT_NONE)
    {
        switch (action)
        {
        case INCRATEP1:
            GetXR1().SetAirspeedHold(playSound, AS_ADJUST, ASRATE_TINY);
            processed = true;
            break;

        case DECRATEP1:
            GetXR1().SetAirspeedHold(playSound, AS_ADJUST, -ASRATE_TINY);
            processed = true;
            break;

        case INCRATE1:
            GetXR1().SetAirspeedHold(playSound, AS_ADJUST, ASRATE_SMALL);
            processed = true;
            break;

        case DECRATE1:
            GetXR1().SetAirspeedHold(playSound, AS_ADJUST, -ASRATE_SMALL);
            processed = true;
            break;

        case INCRATE5:
            GetXR1().SetAirspeedHold(playSound, AS_ADJUST, ASRATE_MED);
            processed = true;
            break;

        case DECRATE5:
            GetXR1().SetAirspeedHold(playSound, AS_ADJUST, -ASRATE_MED);
            processed = true;
            break;

        case INCRATE25:
            GetXR1().SetAirspeedHold(playSound, AS_ADJUST, ASRATE_LARGE);
            processed = true;
            break;

        case DECRATE25:
            GetXR1().SetAirspeedHold(playSound, AS_ADJUST, -ASRATE_LARGE);
            processed = true;
            break;

        default:
            // no action
            break;
        }
    }
                    
    return processed;
}

//----------------------------------------------------------------------------------

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
    for (int i=0; i < GetDoorCount(); i++)
        delete m_pDoorInfo[i];

    delete[] m_pDoorInfo;
}

// Invoked by our parent's AddDisplayMode method immediately after we are attached to our parent MDA.
// This is useful if an MDA needs to perform some one-time initialization (which we do!)
void ReentryCheckMultiDisplayMode::OnParentAttach() 
{ 
    m_pDoorInfo = new DoorInfo *[GetDoorCount()];  // NOTE: if a subclass is present, this may be > 6.

    // create a DoorInfo handler for each door
    const int cx = GetCloseButtonXCoord();       // X coord of close button
    int cy = GetStartingCloseButtonYCoord();
    m_pDoorInfo[0] = new DoorInfo("OPEN",     "CLOSED", GetXR1().nose_status,      _COORD2(cx, cy),                   &DeltaGliderXR1::ActivateNoseCone);
    m_pDoorInfo[1] = new DoorInfo("DEPLYD",   "STOWED", GetXR1().radiator_status,  _COORD2(cx, cy += GetLinePitch()), &DeltaGliderXR1::ActivateRadiator);
    m_pDoorInfo[2] = new DoorInfo("OPEN",     "CLOSED", GetXR1().rcover_status,    _COORD2(cx, cy += GetLinePitch()), &DeltaGliderXR1::ActivateRCover);
    m_pDoorInfo[3] = new DoorInfo("OPEN",     "CLOSED", GetXR1().scramdoor_status, _COORD2(cx, cy += GetLinePitch()), &DeltaGliderXR1::ActivateScramDoors);
    m_pDoorInfo[4] = new DoorInfo("OPEN",     "CLOSED", GetXR1().hoverdoor_status, _COORD2(cx, cy += GetLinePitch()), &DeltaGliderXR1::ActivateHoverDoors);
    m_pDoorInfo[5] = new DoorInfo("DOWN",     "UP",     GetXR1().gear_status,      _COORD2(cx, cy += GetLinePitch()), &DeltaGliderXR1::ActivateLandingGear);
}

void ReentryCheckMultiDisplayMode::Activate()
{
    m_backgroundSurface = CreateSurface(IDB_REENTRY_CHECK_MULTI_DISPLAY);
    m_mainFont = CreateFont(12, 0, 0, 0, 700, 0, 0, 0, 0, 0, 0, 0, FF_MODERN, "Microsoft Sans Serif");

    // check doors and issue correct callout here
    int openDoorCount = 0;
    for (int i=0; i < GetDoorCount(); i++)
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
			GetXR1().ShowWarning(NULL, DeltaGliderXR1::ST_None, msg);
    }
    else
    {
		if (bPlaySound)
			GetXR1().ShowInfo("Reentry Check All Systems Green.wav", DeltaGliderXR1::ST_InformationCallout, "Reentry Check: all systems green.");
		else
			GetXR1().ShowInfo(NULL, DeltaGliderXR1::ST_None, "Reentry Check: all systems green.");
    }
}

bool ReentryCheckMultiDisplayMode::Redraw2D(const int event, const SURFHANDLE surf)
{
    // render the background
    const COORD2 &screenSize = GetScreenSize();
    oapiBlt(surf, m_backgroundSurface, 0, 0, 0, 0, screenSize.x, screenSize.y);

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
    for (int i=0; i < GetDoorCount(); i++)
    {
        const DoorInfo *pDI = m_pDoorInfo[i];
        
        COLORREF textColor;
        const char *pStatus;
        bool renderStatus = true;  // assume NOT in blink "off" state
        switch (pDI->m_doorStatus)
        {
        case DOOR_OPEN:
            textColor = CREF(BRIGHT_RED);
            pStatus = pDI->m_pOpen;     // "Open", etc.
            openDoorCount++;
            break;

        case DOOR_CLOSED:
            textColor = CREF(BRIGHT_GREEN);
            pStatus = pDI->m_pClosed;   // "Closed", etc.
            break;

        case DOOR_FAILED:
            textColor = CREF(BRIGHT_RED);
            pStatus = "FAILED";
            openDoorCount++;
            break;

        case DOOR_OPENING:
        case DOOR_CLOSING:
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
    const char *pStatus;
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
        for (int i=0; i < GetDoorCount(); i++)
        {
            const DoorInfo *pDI = m_pDoorInfo[i];

            if (c.InBounds(pDI->m_closeButtonCoords, 7, 7))
            {
                // invoke the door's handler to close the door if possible
                const DoorStatus ds = pDI->m_doorStatus;
                if (ds == DOOR_CLOSED)
                {
                    GetXR1().PlaySound(GetXR1().Error1, DeltaGliderXR1::ST_Other, ERROR1_VOL);      // already closed
                }
                else    // either OPEN or FAILED
                {
                    // NOTE: this will display any applicable error message if the door cannot begin closing
                    (GetXR1().*pDI->m_pDoorHandler)(DOOR_CLOSING); 
                    
                    GetXR1().PlaySound(GetXR1()._MDMButtonUp, DeltaGliderXR1::ST_Other);
                }
                processed = true;
                break;      // no sense in checking other coordinates
            }
        }
    }

    return processed;
}

// determines which door(s) to use for temperature display warning colors
#define CHECK_AND_RETURN_DOOR(doorStatus) if (doorStatus != DOOR_CLOSED) return doorStatus
DoorStatus HullTempsMultiDisplayMode::GetNoseDoorStatus()
{
    CHECK_AND_RETURN_DOOR(GetXR1().nose_status);    
    CHECK_AND_RETURN_DOOR(GetXR1().hoverdoor_status);
    CHECK_AND_RETURN_DOOR(GetXR1().gear_status);

    return DOOR_CLOSED;  // no open doors for this surface
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
