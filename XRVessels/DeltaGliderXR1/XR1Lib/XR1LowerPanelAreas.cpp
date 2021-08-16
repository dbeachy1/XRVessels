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
// XR1LowerPanelAreas.cpp
// Handles non-component 2D and 2D/3D shared lower panel areas
// ==============================================================

#include "resource.h"
#include "AreaIDs.h"

#include "DeltaGliderXR1.h"
#include "XR1LowerPanelAreas.h"

#include <math.h>

//----------------------------------------------------------------------------------

DockReleaseButtonArea::DockReleaseButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_buttonPressed(false)
{
}

void DockReleaseButtonArea::Activate()
{
    Area::Activate();  // invoke superclass method
    if (IsVC()) // 3D panel?
    {
        // {YYY} add this for the XR2's VC
        // Doesn't exist in the VC
    }
    else    // 2D panel
    {
        oapiRegisterPanelArea(GetAreaID(), GetRectForSize(40, 53), PANEL_REDRAW_MOUSE, PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBUP);
        m_mainSurface = CreateSurface(IDB_SWITCH3);     // HUD mode LED at top-left (2D mode only for now)
    }

    // reset state variables to force a repaint
    m_buttonPressed = false;
}

bool DockReleaseButtonArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    DeltaGliderXR1::SafeBlt(surf, m_mainSurface, 0, 0, (m_buttonPressed ? 40 : 0), 0, 40, 53);
    
    return true;
}

bool DockReleaseButtonArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    if (event & PANEL_MOUSE_LBDOWN)
        GetXR1().PerformUndocking();

    m_buttonPressed = (event & PANEL_MOUSE_LBDOWN);

    return true;
}

//----------------------------------------------------------------------------------

AOAAnalogGaugeArea::AOAAnalogGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID) :
    AnalogGaugeArea(parentPanel, panelCoordinates, areaID, PI, meshTextureID)  // init @ 180 degrees (0 degrees points to the right)
{
}

double AOAAnalogGaugeArea::GetDialAngle()
{
    static const double dial_max = RAD*165.0;
    
    double aoa = (GetXR1().IsLanded() ? 0 : GetVessel().GetAOA());    // always show 0 AoA if wheel-stop
    double dialAngle = PI - min (dial_max, max (-dial_max, aoa*7.7));

    return dialAngle;
}

//----------------------------------------------------------------------------------

SlipAnalogGaugeArea::SlipAnalogGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID) :
    AnalogGaugeArea(parentPanel, panelCoordinates, areaID, (PI/2), meshTextureID)  // init @ 90 degrees (0 degrees points to the right)
    
{
}

double SlipAnalogGaugeArea::GetDialAngle()
{
    static const double dial_max = RAD*165.0;

    // REVERSE slip angle so we match slip indicators in other aircraft
    double slip = (GetXR1().IsLanded() ? 0 : -GetVessel().GetSlipAngle());   // always show 0 slip if wheel-stop
    double dialAngle = PI05 - min (dial_max, max (-dial_max, slip*7.7));

    return dialAngle;
}

//----------------------------------------------------------------------------------

//
// Constructor
// vessel = our vessel handle
// panelCoordinates = absolute coordinates of this area on the parent instrument panel
// areaID = unique Orbiter area ID
ArtificialHorizonArea::ArtificialHorizonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID) :
    XR1Area(parentPanel, panelCoordinates, areaID, meshTextureID)
{
}

// Activate this area
void ArtificialHorizonArea::Activate()
{
    Area::Activate();  // invoke superclass method
    if (IsVC())
    {
        oapiVCRegisterArea (GetAreaID(), GetRectForSize(96, 96), PANEL_REDRAW_ALWAYS, PANEL_MOUSE_IGNORE, PANEL_MAP_NONE, GetVCPanelTextureHandle());
    }
    else
    {
        oapiRegisterPanelArea(GetAreaID(), GetRectForSize(96, 96), PANEL_REDRAW_ALWAYS, PANEL_MOUSE_IGNORE);
    }

    m_mainSurface = CreateSurface(IDB_HORIZON);
    // NOTE: cannot use zero here b/c zero means "none" with the D3D9 client (SURF_PREDEF_CK flag is not passed to graphics clients)
    SetSurfaceColorKey(m_mainSurface, 0xFF000000);  // black = transparent

    // load brushes, pens, and colors
    m_brush2 = CreateSolidBrush(RGB(80,80,224));  // blue
    m_brush3 = CreateSolidBrush(RGB(160,120,64)); // brown
    m_pen0   = CreatePen(PS_SOLID, 1, RGB(224,224,224));
    m_color2 = oapiGetColour(80,80,224);
    m_color3 = oapiGetColour(160,120,64);
}

// Deactivate this area
void ArtificialHorizonArea::Deactivate()
{
    DeleteObject(m_brush2);
    DeleteObject(m_brush3);
    DeleteObject(m_pen0);
    // do not delete colors

    XR1Area::Deactivate();  // let our superclass clean up
}

// Redraw this area
// event = Orbiter event flags
// returns: true if area redrawn, false if not
bool ArtificialHorizonArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    POINT pt[4];
    static double prange = RAD*30.0;
    static int size = 48, size2 = size*2;
    static int extent = static_cast<int>(size*prange);
    double bank = GetVessel().GetBank();
    double pitch = GetVessel().GetPitch();
    double pfrac = pitch/prange;
    double sinb = sin(bank), cosb = cos(bank);
    double a = tan(bank);
    double yl, yr, xb, xt, xlr, xll, ylr, yll;
    int i, iphi, n = 0;
    bool bl, br, bb, bt, bblue;

    if (cosb)   // horizon not vertical
    { 
        double b = pfrac/cosb;
        bl = (fabs(yl = -a+b) < 1.0); // left edge
        br = (fabs(yr =  a+b) < 1.0);  // right edge
        if (a)   // horizon not horizontal
        { 
            bb = (fabs(xb = ( 1.0-b)/a) < 1.0); // bottom edge
            bt = (fabs(xt = (-1.0-b)/a) < 1.0); // top edge
        } 
        else  // horizon horizontal
        { 
            bb = bt = false;
        }
    } 
    else   // horizon vertical
    { 
        bl = br = false;
        bb = bt = (fabs(xb = xt = pfrac) < 1.0);
    }
    if (bl)
    {
        pt[0].x = 0;
        pt[0].y = static_cast<int>(yl*size)+size;
        if (bt)
        {
            pt[1].x = static_cast<int>(xt*size)+size;
            pt[1].y = 0;
            pt[2].x = 0;
            pt[2].y = 0;
            n = 3;
            bblue = (cosb > 0.0);
        } 
        else if (br)
        {
            pt[1].x = size2;
            pt[1].y = static_cast<int>(yr*size)+size;
            pt[2].x = size2;
            pt[2].y = 0;
            pt[3].x = 0;
            pt[3].y = 0;
            n = 4;
            bblue = (cosb > 0.0);
        } 
        else if (bb)
        {
            pt[1].x = static_cast<int>(xb*size)+size;
            pt[1].y = size2;
            pt[2].x = 0;
            pt[2].y = size2;
            n = 3;
            bblue = (cosb < 0.0);
        }
    } 
    else if (br)
    {
        pt[0].x = size2;
        pt[0].y = static_cast<int>(yr*size)+size;
        if (bt)
        {
            pt[1].x = static_cast<int>(xt*size)+size;
            pt[1].y = 0;
            pt[2].x = size2;
            pt[2].y = 0;
            n = 3;
            bblue = (cosb > 0.0);
        } 
        else if (bb) 
        {
            pt[1].x = static_cast<int>(xb*size)+size;
            pt[1].y = size2;
            pt[2].x = size2;
            pt[2].y = size2;
            n = 3;
            bblue = (cosb < 0.0);
        }
    } 
    else if (bt && bb)
    {
        pt[0].x = static_cast<int>(xt*size)+size;
        pt[0].y = 0;
        pt[1].x = static_cast<int>(xb*size)+size;
        pt[1].y = size2;
        pt[2].x = 0;
        pt[2].y = size2;
        pt[3].x = 0;
        pt[3].y = 0;
        n = 4;
        bblue = ((xt-xb)*cosb > 0.0);
    }

    if (!n) 
        bblue = (pitch < 0.0);

    // NOTE: invoking oapiClearSurface improves performance for GetDC on the D3D9 client 
    oapiClearSurface (surf, bblue ? m_color3 : m_color2);

    HDC hDC = GetDC (surf);
    SelectObject (hDC, GetStockObject (BLACK_PEN));

    if (n >= 3)
    {
        SelectObject (hDC, (bblue ? m_brush2 : m_brush3));
        Polygon (hDC, pt, n);
        SelectObject (hDC, m_pen0);
        MoveToEx (hDC, pt[0].x, pt[0].y, NULL); LineTo (hDC, pt[1].x, pt[1].y);
    }

    // bank indicator
    SelectObject (hDC, m_pen0);
    SelectObject (hDC, GetStockObject (NULL_BRUSH));
    static double r1 = 40, r2 = 35;
    double sinb1 = sin(bank-0.1), cosb1 = cos(bank-0.1);
    double sinb2 = sin(bank+0.1), cosb2 = cos(bank+0.1);
    pt[0].x = static_cast<int>(r2*sinb1+0.5)+size; pt[0].y = -static_cast<int>(r2*cosb1+0.5)+size;
    pt[1].x = static_cast<int>(r1*sinb+0.5)+size;  pt[1].y = -static_cast<int>(r1*cosb+0.5)+size;
    pt[2].x = static_cast<int>(r2*sinb2+0.5)+size; pt[2].y = -static_cast<int>(r2*cosb2+0.5)+size;
    Polygon (hDC, pt, 3);

    // pitch ladder
    static double d = size*(10.0*RAD)/prange;
    static double ladderw = 14.0;
    double lwcosa = ladderw*cosb, lwsina = ladderw*sinb;
    double dsinb = d*sinb, dcosb = d*cosb;
    double phi0 = floor(pitch*DEG*0.1);
    double d0 = (pitch*DEG*0.1-phi0) * d, d1 = d0-4*d;

    // ladder
    xlr = lwcosa-d1*sinb, xll = -lwcosa-d1*sinb;
    ylr = lwsina+d1*cosb, yll = -lwsina+d1*cosb;
    for (iphi = static_cast<int>(phi0+4), i = 0; i < 8; i++, iphi--)
    {
        if (iphi)
        {
            MoveToEx (hDC, size+static_cast<int>(xll), size+static_cast<int>(yll), NULL);
            LineTo   (hDC, size+static_cast<int>(xlr), size+static_cast<int>(ylr));
        }
        xlr -= dsinb, ylr += dcosb;
        xll -= dsinb, yll += dcosb;
    }
    ReleaseDC (surf, hDC);

    // labels
    lwcosa *= 1.6, lwsina *= 1.6;
    xlr = lwcosa-d1*sinb, xll = -lwcosa-d1*sinb;
    ylr = lwsina+d1*cosb, yll = -lwsina+d1*cosb;
    for (iphi = static_cast<int>(phi0+4), i = 0; i < 8; i++, iphi--)
    {
        if (iphi)
        {
            int lb = abs(iphi)-1; if (lb >= 9) lb = 16-lb;
            DeltaGliderXR1::SafeBlt (surf, m_mainSurface, size-5+static_cast<int>(xlr), size-3+static_cast<int>(ylr), 9*lb, 96, 9, 7, SURF_PREDEF_CK);
            DeltaGliderXR1::SafeBlt (surf, m_mainSurface, size-5+static_cast<int>(xll), size-3+static_cast<int>(yll), 9*lb, 96, 9, 7, SURF_PREDEF_CK);
        }
        xlr -= dsinb, ylr += dcosb;
        xll -= dsinb, yll += dcosb;
    }

    // now overlay markings with transparent blt
    DeltaGliderXR1::SafeBlt (surf, m_mainSurface, 0, 0, 0, 0, 96, 96, SURF_PREDEF_CK);

    return true;
}

//----------------------------------------------------------------------------------

XFeedKnobArea::XFeedKnobArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    XR1Area(parentPanel, panelCoordinates, areaID)
{
}

// Activate this area
void XFeedKnobArea::Activate()
{
    Area::Activate();  // invoke superclass method
    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(40, 44), PANEL_REDRAW_MOUSE, PANEL_MOUSE_DOWN);

    m_mainSurface = CreateSurface(IDB_DIAL2);    // rotary dial #2 (different bg color)
}

bool XFeedKnobArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    const int mode = static_cast<int>(GetXR1().m_xfeedMode);

    DeltaGliderXR1::SafeBlt(surf, m_mainSurface, 0, 0, mode * 40, 0, 40, 44);

    return true;
}

bool XFeedKnobArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    bool retVal = false;
    
    const int mode = static_cast<int>(GetXR1().m_xfeedMode);
    int newMode = static_cast<int>(XFEED_MODE::XF_NOT_SET);
    if (event & PANEL_MOUSE_LBDOWN)
    {
        // rotate left if not already @ mode 0
        if (mode > 0)
        {
            newMode = mode - 1;
        }
    }
    else if (event & PANEL_MOUSE_RBDOWN) 
    {
        // rotate right if not already @ mode 2
        if (mode < 2)
        {
            newMode = mode + 1;
        }
    }

    if (newMode != static_cast<int>(XFEED_MODE::XF_NOT_SET))
    {
        GetXR1().SetCrossfeedMode(static_cast<XFEED_MODE>(newMode), NULL);  // will show a message & play sound effect as well
        retVal = true;
    }

    // always play click sound
    GetXR1().PlaySound(GetXR1().SwitchOn, DeltaGliderXR1::ST_Other); // medium click

    return retVal;
}

//----------------------------------------------------------------------------------

//
// Constructor
// vessel = our vessel handle
// panelCoordinates = absolute coordinates of this area on the parent instrument panel
// areaID = unique Orbiter area ID
SystemsDisplayScreen::SystemsDisplayScreen(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_width(207), m_height(82)
{
    // use transparent background
    // m_pTextBox = new TextBox(m_width, m_height, CREF(BRIGHT_GREEN), CREF(BRIGHT_RED), CREF(CWHITE), 7, GetXR1().m_infoWarningTextLineGroup);
    m_pTextBox = new TextBox(m_width, m_height, CREF(BRIGHT_GREEN), CREF(BRIGHT_RED), CREF(CWHITE), 7, GetXR1().m_infoWarningTextLineGroup);

    // create our font
    m_mainFont = CreateFont(14, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, 0, 0, "Arial");
    m_lineSpacing = 11;     // pixels between lines
}

SystemsDisplayScreen::~SystemsDisplayScreen()
{
    // clean up the text box we allocated
    delete m_pTextBox;

    // clean up the font we allocated
    DeleteObject(m_mainFont);
}

// Activate this area
void SystemsDisplayScreen::Activate()
{
    Area::Activate();  // invoke superclass method
    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(m_width, m_height), PANEL_REDRAW_ALWAYS, PANEL_MOUSE_IGNORE, PANEL_MAP_BACKGROUND);
}

// Render the contents of the systems display screen
// NOTE: the subclass MUST draw text from the supplied topY coordinate (plus some border gap space)
// The X coordinate is zero @ the border
// Returns: true if text re-rendered, false if not
bool SystemsDisplayScreen::Redraw2D(const int event, const SURFHANDLE surf)
{
    // NOTE: area was registered with PANEL_MAP_BACKGROUND, so we don't need to repaint the background

    // invoke new TextBox handler to draw text
    // Note that our text box will never be null here.
    HDC hDC = GetDC(surf);
    
    bool retVal = m_pTextBox->Render(hDC, 0, m_mainFont, m_lineSpacing, (event == PANEL_REDRAW_INIT));  // CWHITE = use transparent background
    
    ReleaseDC(surf, hDC);

    return retVal;
}

//----------------------------------------------------------------------------------

// switchState = ref to bool switch state
ExtSupplyLineToggleSwitchArea::ExtSupplyLineToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID, bool &switchState, const bool &pressureNominal) :
    ToggleSwitchArea(parentPanel, panelCoordinates, areaID, indicatorAreaID),
    m_switchState(switchState), pressureNominal(pressureNominal)
{
}

bool ExtSupplyLineToggleSwitchArea::ProcessSwitchEvent(bool switchIsOn)
{
    // if turning switch on, check the line pressure
    if (switchIsOn)
    {
        if (pressureNominal == false)
        {
            GetXR1().ShowWarning("No External Line Pressure.wav", DeltaGliderXR1::ST_WarningCallout, "Cannot resupply:&no external line pressure.");
            GetXR1().PlaySound(GetXR1().Error1, DeltaGliderXR1::ST_Other, ERROR1_VOL);
            return false;
        }
    }

    // Note: switch sound is played by the base class

    m_switchState = switchIsOn;   // ref to XR1 variable
    return true;
}

bool ExtSupplyLineToggleSwitchArea::isOn()
{
    return m_switchState;       // ref to XR1 variable
}

//-------------------------------------------------------------------------

#ifdef TURBOPACKS
// crew display panel showing crew members; also handles EVA requests
TurbopackDisplayArea::TurbopackDisplayArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_font(0)
{
    m_deployButtonCoord.x = 6;
    m_deployButtonCoord.y = 19;

    m_stowAllButtonCoord.x = 124;
    m_stowAllButtonCoord.y = 19;

    m_prevArrowCoord.x = 164;
    m_prevArrowCoord.y = 6;
    
    m_nextArrowCoord.x = 176;
    m_nextArrowCoord.y = 6;
}

void TurbopackDisplayArea::Activate()
{
    Area::Activate();  // invoke superclass method
    m_font = CreateFont(14, 0, 0, 0, 600, 0, 0, 0, 0, 0, 0, 0, FF_MODERN, "Microsoft Sans Serif");

    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(182, 26), PANEL_REDRAW_MOUSE, PANEL_MOUSE_LBDOWN, PANEL_MAP_BACKGROUND);
}

void TurbopackDisplayArea::Deactivate()
{
    DeleteObject(m_font);
    XR1Area::Deactivate();  // invoke superclass method
}

bool TurbopackDisplayArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    // obtain device context and save existing font
    HDC hDC = GetDC(surf);
    HFONT hPrevObject = (HFONT)SelectObject(hDC, m_font); // will render status text first
    SetBkMode(hDC, TRANSPARENT);
    SetTextAlign(hDC, TA_LEFT);  // set to LEFT alignment

    // render turbopack display name
    const Turbopack *pSelectedTurbopack = TURBOPACKS_ARRAY + GetXR1().m_selectedTurbopack;

    SetTextColor(hDC, CREF(OFF_WHITE217));
    TextOut(hDC, 6, 2, pSelectedTurbopack->DisplayName, static_cast<int>(strlen(pSelectedTurbopack->DisplayName)));
    
    // restore previous font and release device context
    SelectObject(hDC, hPrevObject);
    ReleaseDC(surf, hDC);

    return true;
}

bool TurbopackDisplayArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    // NOTE: this also verifies that at least ONE crew member is on board!  
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    bool retVal = false;  // set to TRUE to re-render the area; i.e., when a turbopack arrow is clicked

    // check whether the left button was just pressed
    if (event & PANEL_MOUSE_LBDOWN)
    {
        COORD2 c = { mx, my };

        if (c.InBounds(m_deployButtonCoord, 7, 7))    
        {
            // the beep/info message will be handled by the deploy method
            GetXR1().DeployTurbopack();  // deploy currently-selected turbopack
        }
        else if (c.InBounds(m_stowAllButtonCoord, 7, 7))    
        {
            // the beep/info message will be handled by the stowall method
            GetXR1().StowAllTurbopacks();  
        }
        // Note: arrows are only present for those vessels that support more than one turbopack type
        else if ((TURBOPACKS_ARRAY_SIZE > 1) && c.InBounds(m_prevArrowCoord, 6, 7))  // previous turbopack
        {
            retVal = true;
            int &index = GetXR1().m_selectedTurbopack;
            GetXR1().PlaySound(GetXR1().BeepLow, DeltaGliderXR1::ST_Other);

            index--;
            if (index < 0)
                index = TURBOPACKS_ARRAY_SIZE-1;    // wrap around
        }
        else if ((TURBOPACKS_ARRAY_SIZE > 1) && c.InBounds(m_nextArrowCoord, 6, 7))  // next turbopack
        {
            retVal = true;
            int &index = GetXR1().m_selectedTurbopack;
            GetXR1().PlaySound(GetXR1().BeepHigh, DeltaGliderXR1::ST_Other);

            index++;
            if (index >= TURBOPACKS_ARRAY_SIZE)
                index = 0;  // wrap around
        }
    }

    return retVal;
}
#endif
