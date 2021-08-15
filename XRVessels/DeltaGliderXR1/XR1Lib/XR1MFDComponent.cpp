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
// XR1MFDComponent.cpp
// Handles a single MFD for a 2D panel
// ==============================================================

#include "orbitersdk.h"
#include "resource.h"
#include "AreaIDs.h"
#include "XR1InstrumentPanels.h"
#include "XR1Areas.h"
#include "XR1MFDComponent.h"

// Constructor
// parentPanel = parent instrument panel
// topLeft = for 2D: coordinates of top-left corner of first button (on gray border).  For 3D: top-left of AID_MFD1_LBUTTONS or AID_MFD2_LBUTTONS
// mfdID = MFD_LEFT, MFD_RIGHT, etc. (0, 1, ... 9)
// screenMeshGroup mesh group ID of the MFD screen
MFDComponent::MFDComponent(InstrumentPanel &parentPanel, COORD2 topLeft, int mfdID, const int meshTextureID, const int screenMeshGroup) :
    XR1Component(parentPanel, topLeft, meshTextureID, screenMeshGroup)
{
    if (IsVC())
    {
        // NOTE: 3D MFD component is not movable via a delta due to 3D coordinates
        // For this component in the VC, topLeft component coords are at the top-left of AID_MFD1_LBUTTONS or AID_MFD2_LBUTTONS
        AddArea(new MFDScreenArea        (parentPanel, _COORD2(-1,-1), AID_MFD1_SCREEN + mfdID, mfdID, screenMeshGroup));  // coords not used in VC mode

        AddArea(new VCMFDBottomButtonArea(parentPanel, AID_MFD1_PWR + mfdID, mfdID, VCMFDBottomButtonArea::PWR));  // top-left of PWR button face
        AddArea(new VCMFDBottomButtonArea(parentPanel, AID_MFD1_SEL + mfdID, mfdID, VCMFDBottomButtonArea::SEL));  // top-left of SEL button face
        AddArea(new VCMFDBottomButtonArea(parentPanel, AID_MFD1_MNU + mfdID, mfdID, VCMFDBottomButtonArea::MNU));  // top-left of MNU button face

        AddArea(new MFDMainButtonsArea   (parentPanel, GetAbsCoords(_COORD2(0,  0)), AID_MFD1_LBUTTONS + mfdID, mfdID, MFDMainButtonsArea::LEFT,  m_meshTextureID));  // left main button row (on top-left button face)
        AddArea(new MFDMainButtonsArea   (parentPanel, GetAbsCoords(_COORD2(0, 10)), AID_MFD1_RBUTTONS + mfdID, mfdID, MFDMainButtonsArea::RIGHT, m_meshTextureID));  // right main button row (on top-left button face)
    }
    else    // 2D panel
    {
        // Coordinates are relative to the very top-left outside MFD frame itself 
        // NOTE: for MFD screen area, area begins at where TEXT is drawn, which is two pixels below the top-left corner
        AddArea(new MFDScreenArea       (parentPanel, GetAbsCoords(_COORD2( 55,  14+2)), AID_MFD1_SCREEN + mfdID, mfdID, m_screenMeshGroup));  // top-left of screen + 2 pixels Y offset
        AddArea(new MFDBottomButtonsArea(parentPanel, GetAbsCoords(_COORD2( 66, 313)), AID_MFD1_BBUTTONS + mfdID, mfdID));  // top-left of PWR button face
        AddArea(new MFDMainButtonsArea  (parentPanel, GetAbsCoords(_COORD2( 13,  56)), AID_MFD1_LBUTTONS + mfdID, mfdID, MFDMainButtonsArea::LEFT));   // left main button row (on top-left button face)
        AddArea(new MFDMainButtonsArea  (parentPanel, GetAbsCoords(_COORD2(366,  56)), AID_MFD1_RBUTTONS + mfdID, mfdID, MFDMainButtonsArea::RIGHT));  // right main button row (on top-left button face)
    }
}   

//-------------------------------------------------------------------------
// Note: meshTextureID not needed for this area; instead, we pass a mesh group ID in
MFDScreenArea::MFDScreenArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int mfdID, const int meshGroup) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_mfdID(mfdID), m_rebootMFD(false), m_meshGroup(meshGroup)
{
}

// Activate this area
void MFDScreenArea::Activate()
{
    Area::Activate();  // invoke superclass method
    if (IsVC())
    {
        VCMFDSPEC mfdspec = { 1, m_meshGroup };
        oapiVCRegisterMFD(GetMfdID(), &mfdspec);
    }
    else    // 2D
    {
        MFDSPEC mfdspec  = {GetRectForSize(290, 290), 6, 6, 47, 41};
        oapiRegisterMFD(GetMfdID(),  mfdspec);  
    }
}

void MFDScreenArea::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // NOTE: MFD settings are GLOBAL TO ALL SHIPS, so we only want to turn off the MFDs if THIS ship has focus!
    if (GetXR1().HasFocus())
    {
        // if systems offline, ensure MFD is OFF
        if (GetXR1().m_internalSystemsFailure)
        {
            // if MFD still on, turn it off
            if (oapiGetMFDMode(GetMfdID()) != MFD_NONE)
            {
                oapiToggleMFD_on(GetMfdID());
                m_rebootMFD = true; // turn MFD on when systems restored
            }
        }
        else if (m_rebootMFD)   // systems online; ensure MFD is ON
        {
            // if MFD still off, turn it on
            if (oapiGetMFDMode(GetMfdID()) == MFD_NONE)
                oapiToggleMFD_on(GetMfdID());

            m_rebootMFD = false; // reset
        }
    }
}

//-------------------------------------------------------------------------

//
// Constructor
// vessel = our vessel handle
// panelCoordinates = absolute coordinates of this area on the parent instrument panel
// areaID = unique Orbiter area ID
// Note: this area is used for 2D panels only
MFDBottomButtonsArea::MFDBottomButtonsArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int mfdID) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_mfdID(mfdID)
{
}

// Activate this area
void MFDBottomButtonsArea::Activate()
{
    Area::Activate();  // invoke superclass method
    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(270, 18), PANEL_REDRAW_NEVER, PANEL_MOUSE_LBDOWN|PANEL_MOUSE_ONREPLAY);
}

// Handle mouse events for this area
// event = Orbiter event flags
// mx, my = mouse coordinates relative to the area
// returns: true if event processed, false if not
bool MFDBottomButtonsArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated or systems offline, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard() || GetXR1().m_internalSystemsFailure)
        return false;

    int mfd = GetMfdID();   // MFD_LEFT, MFD_RIGHT, etc.
    bool playClick = false;

    if (mx < 26)
    {
        oapiToggleMFD_on (mfd);  // PWR
        playClick = true;
    }
    else if (mx >= 214 && mx < 240)  
    {
        oapiSendMFDKey (mfd, OAPI_KEY_F1);  // SEL
        playClick = true;
    }
    else if (mx > 244)              
    {
        oapiSendMFDKey (mfd, OAPI_KEY_GRAVE); // MNU
        playClick = true;
    }
     
    if (playClick)
        GetXR1().PlaySound(GetXR1().SwitchOn, DeltaGliderXR1::ST_Other, MFD_CLICK);

    return true;
}

//-------------------------------------------------------------------------

// Handles both left side and right side main buttons for both 2D and VC panels
MFDMainButtonsArea::MFDMainButtonsArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int mfdID, const BUTTON_SIDE buttonSide, const int meshTextureID) :
    XR1Area(parentPanel, panelCoordinates, areaID, meshTextureID),
    m_mfdID(mfdID), m_buttonSide(buttonSide), m_justActivated(false)
{
    m_font = CreateFont(-10, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, 0, 0, "Arial");
}

MFDMainButtonsArea::~MFDMainButtonsArea()
{
    DeleteObject(m_font);   // clean up
}

void MFDMainButtonsArea::Activate()
{
    Area::Activate();  // invoke superclass method
    if (IsVC())  // 3D
    {
        // {YYY} need to make this size configurable for the XR2 VC
        oapiVCRegisterArea(GetAreaID(), GetRectForSize(143,10), PANEL_REDRAW_USER, PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBPRESSED | PANEL_MOUSE_ONREPLAY, PANEL_MAP_BACKGROUND, GetVCPanelTextureHandle());
    }
    else    // 2D
    {
        oapiRegisterPanelArea(GetAreaID(), GetRectForSize(25, 223), PANEL_REDRAW_USER,  PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBPRESSED | PANEL_MOUSE_ONREPLAY, PANEL_MAP_BACKGROUND);
    }

    m_justActivated = true;
}

bool MFDMainButtonsArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    HDC hDC = GetDC(surf);
    HFONT pFont = (HFONT)SelectObject(hDC, m_font);
    SetTextColor (hDC, RGB(196, 196, 196));
    SetTextAlign (hDC, TA_CENTER);
    SetBkMode (hDC, TRANSPARENT);
    const char *label;
    // {YYY} resolve this for the XR2 (need Redraw3D method, but it will be nearly identical to this one.  Perhaps create a common method with different VC x & y values passed in?)
    int x = (IsVC() ? 12 : 11);
    int y = (IsVC() ? 0 : 2);

    // draw text for each button
    for (int bt = 0; bt < 6; bt++)
    {
        // ORBITER BUG: for some reason, oapiMFDButtonLabel(0,0) returns NULL immediately after a vessel switch.
        // Workaround is via the PostStep which follows this method.
        if ((label = oapiMFDButtonLabel(GetMfdID(), bt + ((GetButtonSide() == LEFT) ? 0 : 6))) != nullptr)
        {
            TextOut (hDC, x, y, label, static_cast<int>(strlen(label)));
            m_justActivated = true;
            // {YYY} resolve this for the XR2
            if (IsVC()) 
                x += 24;
            else
                y += 41;
        } 
        else
        {
            break;
        }
    }

    SelectObject (hDC, pFont);
    ReleaseDC (surf, hDC);

    return true;
 }

void MFDMainButtonsArea::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // Work around weird redraw bug in Orbiter: if no text drawn yet, try again next frame.  
    // This problem only occurs on a vessel switch.
    if (m_justActivated)
    {
        TriggerRedraw();            // try again to work around bug
        m_justActivated = false;    // reset 
    }
}

bool MFDMainButtonsArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    bool retVal = false;

    if (my % 41 < 18)
    {
        int mfd = GetMfdID();   // 0...9
        int bt = (my / 41) + ((GetButtonSide() == LEFT) ? 0 : 6);
        oapiProcessMFDButton (mfd, bt, event);
        retVal = true;

        if (event & PANEL_MOUSE_LBDOWN)
            GetXR1().PlaySound(GetXR1().SwitchOn, DeltaGliderXR1::ST_Other, MFD_CLICK);
    }

    return retVal;
}

bool MFDMainButtonsArea::ProcessVCMouseEvent(const int event, const VECTOR3 &coords)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    bool retVal = false;

    double dp;
    if (modf(coords.y * 23.0 / 4.0, &dp) < 0.75)
    {
        int bt = static_cast<int>(dp + ((GetButtonSide() == LEFT) ? 0 : 6));
        oapiProcessMFDButton (GetMfdID(), bt, event);
        retVal = true;
        
        if (event & PANEL_MOUSE_LBDOWN)
            GetXR1().PlaySound(GetXR1().SwitchOn, DeltaGliderXR1::ST_Other, MFD_CLICK);
    }
    
    return retVal;
}

//-------------------------------------------------------------------------

// VC-only class to handle three bottom buttons on the MFD (one instance per button)
// Note: no dynamic texture updated required for this area, so no texture ID required.
VCMFDBottomButtonArea::VCMFDBottomButtonArea(InstrumentPanel &parentPanel, const int areaID, const int mfdID, const BUTTON_FUNC buttonFunc) :
    XR1Area(parentPanel, _COORD2(-1,-2), areaID),
    m_mfdID(mfdID), m_buttonFunc(buttonFunc)
{
}

void VCMFDBottomButtonArea::Activate()
{
    Area::Activate();  // invoke superclass method
    // Area ID is AID_MFD1_PWR, AID_MFD2_PWR, AID_MFD1_SEL, etc.
    // Note: the purpose of this call is to register an area ID and its callback for mouse events in the VC *only*.  The clickable area is defined in a separate call 
    // to oapiVCSetAreaClickmode_Spherical in XR1VCPilotInstrumentPanel::Activate().  
    oapiVCRegisterArea(GetAreaID(), PANEL_REDRAW_NEVER, PANEL_MOUSE_LBDOWN | PANEL_MOUSE_ONREPLAY);
}

bool VCMFDBottomButtonArea::ProcessVCMouseEvent(const int event, const VECTOR3 &coords)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    switch (GetButtonFunc())
    {
    case PWR:
        oapiToggleMFD_on(MFD_LEFT + GetMfdID());
        break;
        
    case SEL:
        oapiSendMFDKey(MFD_LEFT + GetMfdID(), OAPI_KEY_F1);
        break;

    case MNU:
        oapiSendMFDKey(MFD_LEFT + GetMfdID(), OAPI_KEY_GRAVE);
        break;

    default:
        _ASSERTE(false);  // should never happen!
        return false;
    }

    GetXR1().PlaySound(GetXR1().SwitchOn, DeltaGliderXR1::ST_Other, MFD_CLICK);

    return true;
}
