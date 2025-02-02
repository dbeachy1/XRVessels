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
// Copyright 2006-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR1UpperPanelAreas.cpp
// Handles upper panel areas
// ==============================================================

#include "resource.h"
#include "AreaIDs.h"

#include "DeltaGliderXR1.h"
#include "XR1UpperPanelAreas.h"

//----------------------------------------------------------------------------------

NavLightToggleSwitchArea::NavLightToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID) :
    ToggleSwitchArea(parentPanel, panelCoordinates, areaID, indicatorAreaID)
{
}

bool NavLightToggleSwitchArea::ProcessSwitchEvent(bool switchIsOn)
{
    GetXR1().SetNavlight(switchIsOn);
    return true;
}

bool NavLightToggleSwitchArea::isOn()
{
    return GetXR1().beacon[0].active;
}

//----------------------------------------------------------------------------------

BeaconLightToggleSwitchArea::BeaconLightToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID) :
    ToggleSwitchArea(parentPanel, panelCoordinates, areaID, indicatorAreaID)
{
}

bool BeaconLightToggleSwitchArea::ProcessSwitchEvent(bool switchIsOn)
{
    GetXR1().SetBeacon(switchIsOn);
    return true;
}

bool BeaconLightToggleSwitchArea::isOn()
{
    return GetXR1().beacon[4].active;
}

//----------------------------------------------------------------------------------

StrobeLightToggleSwitchArea::StrobeLightToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID) :
    ToggleSwitchArea(parentPanel, panelCoordinates, areaID, indicatorAreaID)
{
}

bool StrobeLightToggleSwitchArea::ProcessSwitchEvent(bool switchIsOn)
{
    GetXR1().SetStrobe(switchIsOn);
    return true;
}

bool StrobeLightToggleSwitchArea::isOn()
{
    return GetXR1().beacon[6].active;
}

//----------------------------------------------------------------------------------

LadderToggleSwitchArea::LadderToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID) :
    ToggleSwitchArea(parentPanel, panelCoordinates, areaID, indicatorAreaID)
{
}

bool LadderToggleSwitchArea::isOn()
{
    // if switch is down ("on"), door is either opening or opened
    return ((GetXR1().ladder_status == DoorStatus::DOOR_OPENING) || (GetXR1().ladder_status == DoorStatus::DOOR_OPEN));
}


// only invoked when switch is CHANGING state
bool LadderToggleSwitchArea::ProcessSwitchEvent(bool switchIsOn)
{
    GetXR1().ActivateLadder(switchIsOn ? DoorStatus::DOOR_OPENING : DoorStatus::DOOR_CLOSING);
    
    return true;
}

//----------------------------------------------------------------------------------

NoseConeToggleSwitchArea::NoseConeToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID) :
    ToggleSwitchArea(parentPanel, panelCoordinates, areaID, indicatorAreaID)
{
}

bool NoseConeToggleSwitchArea::isOn()
{
    // if switch is down ("on"), door is either opening or opened
    return ((GetXR1().nose_status == DoorStatus::DOOR_OPENING) || (GetXR1().nose_status == DoorStatus::DOOR_OPEN));
}


// only invoked when switch is CHANGING state
bool NoseConeToggleSwitchArea::ProcessSwitchEvent(bool switchIsOn)
{
    GetXR1().ActivateNoseCone(switchIsOn ? DoorStatus::DOOR_OPENING : DoorStatus::DOOR_CLOSING);
    
    return true;
}

//----------------------------------------------------------------------------------

OuterDoorToggleSwitchArea::OuterDoorToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID) :
    ToggleSwitchArea(parentPanel, panelCoordinates, areaID, indicatorAreaID)
{
}

bool OuterDoorToggleSwitchArea::isOn()
{
    // if switch is down ("on"), door is either opening or opened
    return ((GetXR1().olock_status == DoorStatus::DOOR_OPENING) || (GetXR1().olock_status == DoorStatus::DOOR_OPEN));
}


// only invoked when switch is CHANGING state
bool OuterDoorToggleSwitchArea::ProcessSwitchEvent(bool switchIsOn)
{
    GetXR1().ActivateOuterAirlock(switchIsOn ? DoorStatus::DOOR_OPENING : DoorStatus::DOOR_CLOSING);
    
    return true;
}

//----------------------------------------------------------------------------------

InnerDoorToggleSwitchArea::InnerDoorToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID) :
    ToggleSwitchArea(parentPanel, panelCoordinates, areaID, indicatorAreaID)
{
}

bool InnerDoorToggleSwitchArea::isOn()
{
    // if switch is down ("on"), door is either opening or opened
    return ((GetXR1().ilock_status == DoorStatus::DOOR_OPENING) || (GetXR1().ilock_status == DoorStatus::DOOR_OPEN));
}


// only invoked when switch is CHANGING state
bool InnerDoorToggleSwitchArea::ProcessSwitchEvent(bool switchIsOn)
{
    GetXR1().ActivateInnerAirlock(switchIsOn ? DoorStatus::DOOR_OPENING : DoorStatus::DOOR_CLOSING);
    
    return true;
}

//----------------------------------------------------------------------------------

ChamberToggleSwitchArea::ChamberToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID) :
    ToggleSwitchArea(parentPanel, panelCoordinates, areaID, indicatorAreaID)
{
}

bool ChamberToggleSwitchArea::isOn()
{
    // if switch is down ("on"), door is either opening or opened; meaning, chamber is either VACUUM or DECOMPRESSING
    return ((GetXR1().chamber_status == DoorStatus::DOOR_OPENING) || (GetXR1().chamber_status == DoorStatus::DOOR_OPEN));
}


// only invoked when switch is CHANGING state
bool ChamberToggleSwitchArea::ProcessSwitchEvent(bool switchIsOn)
{
    GetXR1().ActivateChamber(switchIsOn ? DoorStatus::DOOR_OPENING : DoorStatus::DOOR_CLOSING, false);  // do not force
    
    return true;
}

//----------------------------------------------------------------------------------

HatchToggleSwitchArea::HatchToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID) :
    ToggleSwitchArea(parentPanel, panelCoordinates, areaID, indicatorAreaID)
{
}

bool HatchToggleSwitchArea::isOn()
{
    // if switch is down ("on"), door is either opening or opened
    return ((GetXR1().hatch_status == DoorStatus::DOOR_OPENING) || (GetXR1().hatch_status == DoorStatus::DOOR_OPEN));
}


// only invoked when switch is CHANGING state
bool HatchToggleSwitchArea::ProcessSwitchEvent(bool switchIsOn)
{
    GetXR1().ActivateHatch(switchIsOn ? DoorStatus::DOOR_OPENING : DoorStatus::DOOR_CLOSING);
    
    return true;
}

//----------------------------------------------------------------------------------

RadiatorToggleSwitchArea::RadiatorToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID) :
    ToggleSwitchArea(parentPanel, panelCoordinates, areaID, indicatorAreaID)
{
}

bool RadiatorToggleSwitchArea::isOn()
{
    // if switch is down ("on"), door is either opening or opened
    return ((GetXR1().radiator_status == DoorStatus::DOOR_OPENING) || (GetXR1().radiator_status == DoorStatus::DOOR_OPEN));
}


// only invoked when switch is CHANGING state
bool RadiatorToggleSwitchArea::ProcessSwitchEvent(bool switchIsOn)
{
    GetXR1().ActivateRadiator(switchIsOn ? DoorStatus::DOOR_OPENING : DoorStatus::DOOR_CLOSING);
    
    return true;
}

//----------------------------------------------------------------------------------

RetroDoorToggleSwitchArea::RetroDoorToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID) :
    ToggleSwitchArea(parentPanel, panelCoordinates, areaID, indicatorAreaID)
{
}

bool RetroDoorToggleSwitchArea::isOn()
{
    // if switch is down ("on"), door is either opening or opened
    return ((GetXR1().rcover_status == DoorStatus::DOOR_OPENING) || (GetXR1().rcover_status == DoorStatus::DOOR_OPEN));
}


// only invoked when switch is CHANGING state
bool RetroDoorToggleSwitchArea::ProcessSwitchEvent(bool switchIsOn)
{
    GetXR1().ActivateRCover(switchIsOn ? DoorStatus::DOOR_OPENING : DoorStatus::DOOR_CLOSING);
    
    return true;
}

//----------------------------------------------------------------------------------

GearToggleSwitchArea::GearToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID) :
    ToggleSwitchArea(parentPanel, panelCoordinates, areaID, indicatorAreaID)
{
}

bool GearToggleSwitchArea::isOn()
{
    // if switch is down ("on"), door is either opening or opened
    return ((GetXR1().gear_status == DoorStatus::DOOR_OPENING) || (GetXR1().gear_status == DoorStatus::DOOR_OPEN));
}


// only invoked when switch is CHANGING state
bool GearToggleSwitchArea::ProcessSwitchEvent(bool switchIsOn)
{
    GetXR1().ActivateLandingGear(switchIsOn ? DoorStatus::DOOR_OPENING : DoorStatus::DOOR_CLOSING);
    
    return true;
}

//----------------------------------------------------------------------------------

AirbrakeToggleSwitchArea::AirbrakeToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID) :
    ToggleSwitchArea(parentPanel, panelCoordinates, areaID, indicatorAreaID)
{
}

bool AirbrakeToggleSwitchArea::isOn()
{
    // if switch is down ("on"), door is either opening or opened
    return ((GetXR1().brake_status == DoorStatus::DOOR_OPENING) || (GetXR1().brake_status == DoorStatus::DOOR_OPEN));
}


// only invoked when switch is CHANGING state
bool AirbrakeToggleSwitchArea::ProcessSwitchEvent(bool switchIsOn)
{
    GetXR1().ActivateAirbrake(switchIsOn ? DoorStatus::DOOR_OPENING : DoorStatus::DOOR_CLOSING);
    
    return true;
}

//----------------------------------------------------------------------------------

ScramTempGaugeArea::ScramTempGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID) :
    XR1Area(parentPanel, panelCoordinates, areaID, meshTextureID)
{
    // create pens that we need
    m_pen0 = CreatePen(PS_SOLID, 1, RGB(224,224,224));
    m_pen1 = CreatePen(PS_SOLID, 3, RGB(164,164,164));
}

ScramTempGaugeArea::~ScramTempGaugeArea()
{
    DeleteObject(m_pen0);
    DeleteObject(m_pen1);
}


void ScramTempGaugeArea::Activate()
{
    Area::Activate();  // invoke superclass method
    if (IsVC())  // 3D
        oapiVCRegisterArea(GetAreaID(), GetRectForSize(81, 130), PANEL_REDRAW_ALWAYS, PANEL_MOUSE_IGNORE, PANEL_MAP_BACKGROUND, GetVCPanelTextureHandle());
    else
        oapiRegisterPanelArea(GetAreaID(), GetRectForSize(83, 130), PANEL_REDRAW_ALWAYS, PANEL_MOUSE_IGNORE, PANEL_MAP_BACKGROUND);
}

// this panel is ALWAYS redrawn
bool ScramTempGaugeArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    double T, phi;
    static const double rad = 19.0;
    int i, j, x0, y0, dx, dy;
    
    HDC hDC = GetDC(surf);
    SelectObject(hDC, m_pen0);
    for (j = 0; j < 3; j++) 
    {
        for (i = 0; i < 2; i++)
        {
            T = GetXR1().ramjet->Temp(i, j);
            phi = PI * min(T,7800.0)/4000.0;   // matches new engines
            dx = static_cast<int>(rad*sin(phi));
            dy = static_cast<int>(rad*cos(phi));
            // {YYY} resolve this for the XR2
            x0 = (IsVC() ? 20 : 22-j) + i*43;
            y0 = 19+j*46;
            MoveToEx(hDC, x0, y0, nullptr); 
            LineTo(hDC, x0+dx, y0-dy);
        }
    }
    SelectObject(hDC, GetStockObject(BLACK_PEN));
    ReleaseDC (surf, hDC);

    return true;
}

//-------------------------------------------------------------------------

OverrideOuterAirlockToggleButtonArea::OverrideOuterAirlockToggleButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    RawButtonArea(parentPanel, panelCoordinates, areaID)
{
}

void OverrideOuterAirlockToggleButtonArea::ProcessButtonAction(int event, double buttonDownSimt)
{
    if (event & PANEL_MOUSE_LBDOWN)
    {
        // toggle button state
        GetXR1().m_airlockInterlocksDisabled = !GetXR1().m_airlockInterlocksDisabled;

        DeltaGliderXR1::Sound sound;
        if (IsLit())
        {
            GetXR1().ShowWarning("Warning Airlock Safety Interlocks Disabled.wav", DeltaGliderXR1::ST_WarningCallout, "WARNING: Airlock safety&interlocks disabled.");
            sound = GetXR1().BeepHigh;
        }
        else
        {
            GetXR1().ShowInfo("Airlock Safety Interlocks Enabled.wav", DeltaGliderXR1::ST_InformationCallout, "Airlock safety interlocks enabled.");
            sound = GetXR1().BeepLow;
        }
        GetXR1().PlaySound(sound, DeltaGliderXR1::ST_Other);  // beep
    }
}

//-------------------------------------------------------------------------

OverrideCrewHatchToggleButtonArea::OverrideCrewHatchToggleButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    RawButtonArea(parentPanel, panelCoordinates, areaID)
{
}

void OverrideCrewHatchToggleButtonArea::ProcessButtonAction(int event, double buttonDownSimt)
{
   if (event & PANEL_MOUSE_LBDOWN)
    {
        // toggle button state
        GetXR1().m_crewHatchInterlocksDisabled = !GetXR1().m_crewHatchInterlocksDisabled;

        DeltaGliderXR1::Sound sound;
        if (IsLit())
        {
            GetXR1().ShowWarning("Warning Hatch Safety Interlocks Disabled.wav", DeltaGliderXR1::ST_WarningCallout, "WARNING: Crew hatch safety&interlocks disabled.");
            sound = GetXR1().BeepHigh;
        }
        else
        {
            GetXR1().ShowInfo("Hatch Safety Interlocks Enabled.wav", DeltaGliderXR1::ST_InformationCallout, "Crew hatch safety interlocks enabled.");
            sound = GetXR1().BeepLow;
        }
        GetXR1().PlaySound(sound, DeltaGliderXR1::ST_Other);  // beep
    }
}

//----------------------------------------------------------------------------------

HoverDoorToggleSwitchArea::HoverDoorToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID) :
    ToggleSwitchArea(parentPanel, panelCoordinates, areaID, indicatorAreaID)
{
}

bool HoverDoorToggleSwitchArea::isOn()
{
    // if switch is down ("on"), door is either opening or opened
    return ((GetXR1().hoverdoor_status == DoorStatus::DOOR_OPENING) || (GetXR1().hoverdoor_status == DoorStatus::DOOR_OPEN));
}


// only invoked when switch is CHANGING state
bool HoverDoorToggleSwitchArea::ProcessSwitchEvent(bool switchIsOn)
{
    GetXR1().ActivateHoverDoors(switchIsOn ? DoorStatus::DOOR_OPENING : DoorStatus::DOOR_CLOSING);
    
    return true;
}

//----------------------------------------------------------------------------------

ScramDoorToggleSwitchArea::ScramDoorToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID) :
    ToggleSwitchArea(parentPanel, panelCoordinates, areaID, indicatorAreaID)
{
}

bool ScramDoorToggleSwitchArea::isOn()
{
    // if switch is down ("on"), door is either opening or opened
    return ((GetXR1().scramdoor_status == DoorStatus::DOOR_OPENING) || (GetXR1().scramdoor_status == DoorStatus::DOOR_OPEN));
}


// only invoked when switch is CHANGING state
bool ScramDoorToggleSwitchArea::ProcessSwitchEvent(bool switchIsOn)
{
    GetXR1().ActivateScramDoors(switchIsOn ? DoorStatus::DOOR_OPENING : DoorStatus::DOOR_CLOSING);
    
    return true;
}

//-------------------------------------------------------------------------

// crew display panel showing crew members; also handles EVA requests
CrewDisplayArea::CrewDisplayArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_font(0), m_numberFont(0)
{
    m_evaButtonCoord.x = 154;
    m_evaButtonCoord.y = 6;

    m_prevArrowCoord.x = 157;
    m_prevArrowCoord.y = 21;
    
    m_nextArrowCoord.x = 174;
    m_nextArrowCoord.y = 21;

    m_crewMemberIndexX = 165;   // X coordinate at which number will be rendered
}

void CrewDisplayArea::Activate()
{
    Area::Activate();  // invoke superclass method
    m_font       = CreateFont(14, 0, 0, 0, 600, 0, 0, 0, 0, 0, 0, 0, FF_MODERN, "Microsoft Sans Serif");
    m_numberFont = CreateFont(12, 0, 0, 0, 600, 0, 0, 0, 0, 0, 0, 0, FF_MODERN, "Microsoft Sans Serif");

    // Note: this is 14 pixels wider than we need so that the subclasses have room for 2-digit crew indexes
    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(194, 32), PANEL_REDRAW_MOUSE | PANEL_REDRAW_USER, PANEL_MOUSE_LBDOWN, PANEL_MAP_BACKGROUND);
}

void CrewDisplayArea::Deactivate()
{
    DeleteObject(m_font);
    DeleteObject(m_numberFont);
    XR1Area::Deactivate();  // invoke superclass method
}

bool CrewDisplayArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    // obtain device context and save existing font
    HDC hDC = GetDC(surf);
    HFONT hPrevObject = (HFONT)SelectObject(hDC, m_font); // will render status text first
    SetBkMode(hDC, TRANSPARENT);
    SetTextAlign(hDC, TA_LEFT);  // set to LEFT alignment

    // if NO ONE on board, show a warning in RED
    if (GetXR1().GetCrewMembersCount() == 0)
    {
        SetTextColor(hDC, CREF(BRIGHT_RED));
        const char *pMsg = "NO CREW ON BOARD!";
        TextOut(hDC, 11, 9, pMsg, static_cast<int>(strlen(pMsg)));

        // restore previous font and release device context
        SelectObject(hDC, hPrevObject);
        ReleaseDC(surf, hDC);
        return true; 
    }

    int crewMemberIndex = GetXR1().m_crewDisplayIndex;    // 0-n

    ////////////////////////////////////////////////////
    // Defensive coding: sanity check
    // Check whether this crewman is on board, although in theory we should never need this if we change
    // m_crewDisplayIndex correctly whenever a crew member enters or leaves the ship.
    const char *pUmmuName = GetXR1().GetCrewNameBySlotNumber(crewMemberIndex);
    bool isOnBoard = (strlen(pUmmuName) > 0);

    if (isOnBoard == false) 
    {
        // current crew member for crewDisplayIndex is not on board; try switching to index #0
        crewMemberIndex = GetXR1().m_crewDisplayIndex = 0;
        pUmmuName = GetXR1().GetCrewNameBySlotNumber(crewMemberIndex);
        isOnBoard = (strlen(pUmmuName) > 0);
        if (isOnBoard == false) // still not on board? (should hever happen b/c UMmu always keeps slot #0 filled first, and GetCrewMembersCount() > 0 here)
            return true;       // crewman is not on board; display will be blank until pilot clicks button     
    }
        
    ////////////////////////////////////////////////////

    // copy UMmu values to known buffer sizes so we don't overflow the display, allowing 1 byte for the terminator
    char pName[CrewMemberNameLength+1];
    char pAge[4];   // age is limited to 3 digits
    char pRank[CrewMemberRankLength+1];

    strncpy(pName, pUmmuName, CrewMemberNameLength);
    pName[CrewMemberNameLength] = 0;    // in case name is max length

    int age = GetXR1().GetCrewAgeByName(const_cast<char *>(pUmmuName));
    // sanity-check the age
    if (age < 1)
        age = 1;
    else if (age > 99)
        age = 99;   // keep in range
    sprintf(pAge, "%2d", age);

    const char *pUmmuMisc = GetXR1().GetCrewMiscIdByName(pUmmuName);
    strncpy(pRank, GetXR1().RetrieveRankForMmuMisc(pUmmuMisc), CrewMemberRankLength);
    pRank[CrewMemberRankLength] = 0;    // in case rank is max length

    // render crew member's:
    //  name (age) 
    //  rank
    const int maxNameLineLength = CrewMemberNameLength + 2 + 3;   // includes 2 bytes for age + 3 extra bytes for " ()", excludes terminator
    char temp[maxNameLineLength+1]; // allow room for terminator
    const int fontPitch = 14;       // includes space between lines
    const int xCoord = 2;
    int yCoord = 2;
    SetTextColor(hDC, CREF(OFF_WHITE217));
    sprintf(temp, "%s (%s)", pName, pAge);
    TextOut(hDC, xCoord, yCoord, temp, min(static_cast<int>(strlen(temp)), maxNameLineLength)); // limit line length
    
    yCoord += fontPitch;
    TextOut(hDC, xCoord, yCoord, pRank, static_cast<int>(strlen(pRank)));  
        
    // render crew member index (0-n)
    SelectObject(hDC, m_numberFont);   // switch fonts
    SetTextColor(hDC, CREF(LIGHT_BLUE));
    sprintf(temp, "%d", crewMemberIndex);   // reuse existing buffer; size is plenty big enough
    TextOut(hDC, m_crewMemberIndexX, 18, temp, static_cast<int>(strlen(temp)));

    // restore previous font and release device context
    SelectObject(hDC, hPrevObject);
    ReleaseDC(surf, hDC);

    return true;
}

bool CrewDisplayArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    // NOTE: this also verifies that at least ONE crew member is on board!  
    // Also note that unlike other areas, this area is still functional if no pilot is on board.
    if (GetXR1().IsCrewIncapacitated())
        return false;

    bool retVal = false;

    // check whether button was just pressed
    if (event & PANEL_MOUSE_LBDOWN)
    {
        COORD2 c = { mx, my };

        if (c.InBounds(m_evaButtonCoord, 7, 7))    // EVA
        {
            retVal = true;  // always re-render
            bool success = GetXR1().PerformEVA(GetXR1().m_crewDisplayIndex);
            if (success)
                goto set_to_next_crew_member;   // since this one is gone now
        }
        else if (c.InBounds(m_prevArrowCoord, 6, 7))  // previous crew member
        {
            retVal = true;
            int &index = GetXR1().m_crewDisplayIndex;
            GetXR1().PlaySound(GetXR1().BeepLow, DeltaGliderXR1::ST_Other);

            // keep looking until previous passenger found
            bool passengerFound = false;
            while (index > 0) 
            {
                index--;    // >= 1 here
                if (GetXR1().IsCrewMemberOnBoard(index))
                {
                    passengerFound = true;
                    break;
                }
            }

            if (passengerFound == false)  
            {
                // No passengers found and we're counting down, so let's wrap around to the top of the index.
                // Find the highest slot with a passenger in it; note that this will re-check a few passenger slots (since we will check them
                // all here), but that is OK.
                for (index = MAX_PASSENGERS-1; index >= 0; index--)
                {
                    if (GetXR1().IsCrewMemberOnBoard(index))
                    {
                        passengerFound = true;
                        break;
                    }
                }
            }

            if (passengerFound == false)  // still haven't found a crew member?
            {
                // No crew on board! Should never happen (in theory) because we check for incapacitated crew at the beginning of this method.
                index = 0;  // reset to zero, although nobody on board...
            }
        }
        else if (c.InBounds(m_nextArrowCoord, 6, 7))  // next crew member
        {
set_to_next_crew_member:
            retVal = true;
            int &index = GetXR1().m_crewDisplayIndex;
            GetXR1().PlaySound(GetXR1().BeepHigh, DeltaGliderXR1::ST_Other);

            // keep looking until next passenger found
            bool passengerFound = false;
            while (++index < MAX_PASSENGERS)
            {
                if (GetXR1().IsCrewMemberOnBoard(index))
                {
                    passengerFound = true;
                    break;
                }
            }

            if (passengerFound == false)  
            {
                // we wrapped around, so let's try all the crew members counting up now (OK to try a few twice)
                for (index = 0; index < MAX_PASSENGERS; index++)
                {
                    if (GetXR1().IsCrewMemberOnBoard(index))
                    {
                        passengerFound = true;
                        break;
                    }
                }
            }

            // NOTE: it is only possible to hit this if we just EVA'd the last crew member
            if (passengerFound == false)    // still haven't found a crew member?
            {
                // No crew on board! Should never happen (in theory) because we check for incapacitated crew at the beginning of this method.
                index = 0;  // reset to zero, although nobody on board...
            }
        }
    }

    return retVal;
}

// Areas below here are used only by subclasses
//----------------------------------------------------------------------------------

// WARNING: do not make a direct call to oapiSetPanel from an area!  The area may be destroyed before all the area's events are dispatched!  
// Therefore, we request a panel switch via a custom PostStep in the main XR2 vessel object, where it is safe to switch panels.
// targetPanelID: PANEL_OVERHEAD, PANEL_UPPER, PANEL_OVERHEAD, etc.
SwitchToPanelButtonArea::SwitchToPanelButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int targetPanelID) :
    MomentaryButtonArea(parentPanel, panelCoordinates, areaID),
    m_targetPanelID(targetPanelID)
{
}

void SwitchToPanelButtonArea::ProcessButtonAction(int event, double buttonDownSimt)
{
    // process PRESSED and UNPRESSED events
    if (event & (PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBUP))
    {
        GetXR1().PlaySound(GetXR1().SwitchOn, DeltaGliderXR1::ST_Other, MED_CLICK);  // medium click for both on and off

        // if button is RELEASED, request a switch to the docking panel
        if (event & PANEL_MOUSE_LBUP)
        {
            DeltaGliderXR1 &xr1 = static_cast<DeltaGliderXR1 &>(GetVessel());  // downcast
            xr1.m_requestSwitchToTwoDPanelNumber = m_targetPanelID;
        }
    }
}

//----------------------------------------------------------------------------------

BayDoorsToggleSwitchArea::BayDoorsToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID) :
    ToggleSwitchArea(parentPanel, panelCoordinates, areaID, indicatorAreaID)
{
}

bool BayDoorsToggleSwitchArea::isOn()
{
    // if switch is down ("on"), door is either opening or opened
    return ((GetXR1().bay_status == DoorStatus::DOOR_OPENING) || (GetXR1().bay_status == DoorStatus::DOOR_OPEN));
}


// only invoked when switch is CHANGING state
bool BayDoorsToggleSwitchArea::ProcessSwitchEvent(bool switchIsOn)
{
    GetXR1().ActivateBayDoors(switchIsOn ? DoorStatus::DOOR_OPENING : DoorStatus::DOOR_CLOSING);
    
    return true;
}
