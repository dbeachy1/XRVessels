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
// XR1ThrottleQuadrantComponents.h
// Handles main, hover, and scram throttle controls
// ==============================================================

#include "orbitersdk.h"
#include "resource.h"
#include "AreaIDs.h"
#include "XR1InstrumentPanels.h"
#include "XR1Areas.h"
#include "XR1ThrottleQuadrantComponents.h"

// Constructor
// parentPanel = parent instrument panel
// topLeft = for 2D: top inside edge of white far-left border.
MainThrottleComponent::MainThrottleComponent(InstrumentPanel &parentPanel, COORD2 topLeft) :
    XR1Component(parentPanel, topLeft)
{
    // there is no throttle component in the VC
    
    // 2D panel
    AddArea(new MainThrottleArea               (parentPanel, GetAbsCoords(_COORD2(104, -8)), AID_ENGINEMAIN));
    AddArea(new MainPitchSwitchArea            (parentPanel, GetAbsCoords(_COORD2( 57, 23)), AID_PGIMBALMAIN));
    AddArea(new MainPitchVerticalGaugeArea     (parentPanel, GetAbsCoords(_COORD2( 22,  6)), AID_PGIMBALMAINDISP));
    AddArea(new SimpleButtonArea               (parentPanel, GetAbsCoords(_COORD2( 51, 80)), AID_PGIMBALMAINCENTER, &GetXR1().m_mainPitchCenteringMode));
    AddArea(new MainYawSwitchArea              (parentPanel, GetAbsCoords(_COORD2( 49,119)), AID_YGIMBALMAIN));
    AddArea(new MainYawHorizontalGaugeArea     (parentPanel, GetAbsCoords(_COORD2( 15,181)), AID_YGIMBALMAINDISP));

    // add the three gimble mode buttons
    AddArea(new SimpleButtonArea    (parentPanel, GetAbsCoords(_COORD2(  2,106)), AID_YGIMBALMAINCENTER, &GetXR1().m_mainYawCenteringMode));
    AddArea(new SimpleButtonArea    (parentPanel, GetAbsCoords(_COORD2(  2,123)), AID_YGIMBALMAINDIV,    &GetXR1().m_mainDivMode));
    AddArea(new SimpleButtonArea    (parentPanel, GetAbsCoords(_COORD2(  2,140)), AID_YGIMBALMAINAUTO,   &GetXR1().m_mainAutoMode));
}   

// topLeft = for 2D: top inside edge of white far-left border.
HoverThrottleComponent::HoverThrottleComponent(InstrumentPanel &parentPanel, COORD2 topLeft) :
    XR1Component(parentPanel, topLeft)
{
    // there is no hover throttle component in the VC
    
    // 2D panel
    AddArea(new LargeHoverThrottleArea       (parentPanel, GetAbsCoords(_COORD2(130, -50)), AID_ENGINEHOVER));
    AddArea(new HoverBalanceSwitchArea       (parentPanel, GetAbsCoords(_COORD2( 57,  17)), AID_HOVERBALANCE));
    AddArea(new HoverBalanceVerticalGaugeArea(parentPanel, GetAbsCoords(_COORD2( 23,   9)), AID_HBALANCEDISP));  
    AddArea(new SimpleButtonArea             (parentPanel, GetAbsCoords(_COORD2( 51,  67)), AID_HBALANCECENTER, &GetXR1().m_hoverCenteringMode));
}   

// topLeft = for 2D: top inside edge of white far-left border.
ScramThrottleComponent::ScramThrottleComponent(InstrumentPanel &parentPanel, COORD2 topLeft) :
    XR1Component(parentPanel, topLeft)
{
    // there is no scram throttle component in the VC
    
    // 2D panel
    AddArea(new ScramThrottleArea              (parentPanel, GetAbsCoords(_COORD2(104, -5)), AID_ENGINESCRAM));
    AddArea(new ScramPitchSwitchArea           (parentPanel, GetAbsCoords(_COORD2( 57, 23)), AID_GIMBALSCRAM));
    AddArea(new ScramPitchVerticalGaugeArea    (parentPanel, GetAbsCoords(_COORD2( 22,  5)), AID_GIMBALSCRAMDISP));
    AddArea(new SimpleButtonArea               (parentPanel, GetAbsCoords(_COORD2( 51, 80)), AID_GIMBALSCRAMCENTER, &GetXR1().m_scramCenteringMode));
}   

//-------------------------------------------------------------------------

MainThrottleArea::MainThrottleArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    XR1Area(parentPanel, panelCoordinates, areaID), m_vcCtrl(0), m_vcMode(0), m_vcPY(0), m_engsliderpos{0}
{
}

MainThrottleArea::~MainThrottleArea()
{
}

void MainThrottleArea::Activate()
{
    Area::Activate();  // invoke superclass method
    if (IsVC()) // 3D panel?
    {
        oapiVCRegisterArea(GetAreaID(), PANEL_REDRAW_ALWAYS, PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBPRESSED);
    }
    else    // 2D panel
    {
        oapiRegisterPanelArea(GetAreaID(), GetRectForSize(49, 175), PANEL_REDRAW_ALWAYS, PANEL_MOUSE_LBPRESSED, PANEL_MAP_BGONREQUEST);
        m_mainSurface = CreateSurface(IDB_SLIDER1);     // engine slider knob
    }

    // reset state variables to force a repaint
    m_engsliderpos[0] = m_engsliderpos[1] = -1;
}

bool MainThrottleArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    UINT pos;
    bool redraw = false;

    for (int i = 0; i < 2; i++)
    {
        double level = GetVessel().GetThrusterLevel(GetXR1().th_main[i]);  // 0...1

        if (level > 0)
        {
            // main engines firing
            pos = (UINT)((1.0-level)*108.0);
        }
        else
        {
            // mains not firing -- let's check retro-rockets
            level = GetVessel().GetThrusterLevel(GetXR1().th_retro[i]); // levek is from 0...1
            if (level > 0) 
            {
                // retro rockets firing
                pos = 125+(UINT)(level*32.0);   // 125 = just off idle up to 32 more pixels (for 100% retro thrust)
            }
            else
            {
                pos = 116;  // engine thrust off -- set to center position
            }
        }
        if (pos != m_engsliderpos[i])   // has it moved since last update?
            m_engsliderpos[i] = pos, redraw = true;
    }

    if (redraw)
    {
        oapiBltPanelAreaBackground(GetAreaID(), surf);
        for (int i = 0; i < 2; i++)
        {
            //       target, source,      targetX, targetY,     srcX, srcY, width, height, (opt., not present) color key
            DeltaGliderXR1::SafeBlt (surf, m_mainSurface, i*26, m_engsliderpos[i], 0, 0, 23, 18);
        }
    }

    return redraw;

}

bool MainThrottleArea::Redraw3D(const int event, const SURFHANDLE surf)
{
    UINT pos;
    for (int i = 0; i < 2; i++)
    {
        double level = GetVessel().GetThrusterLevel(GetXR1().th_main[i]);

        if (level > 0) 
            pos = 150 + (UINT)(level*300.0);    // main thrust
        else
        { 
            // retro thrust
            level = GetVessel().GetThrusterLevel(GetXR1().th_retro[i]);
            pos = 150 - (UINT)(level*150.0);
        }
        if (pos != m_engsliderpos[i])
            GetXR1().SetXRAnimation(GetXR1().anim_mainthrottle[i], (m_engsliderpos[i] = pos)/450.0);
    }
    return true;
}

bool MainThrottleArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    int which;  // which engine(s)

    if      (mx <  12) which = 0; // left engine
    else if (mx >= 37) which = 1; // right engine
    else               which = 2; // both

    int wy = my;    // create working copy
    if ((wy -= 9) < 0) wy = 0;
    else if (wy > 157) wy = 157;
    
    double lmain = (wy <= 108 ? 1.0-wy/108.0 : 0.0);
    double lretro = (wy >= 125 ? (wy-125)/32.0 : 0.0);

    if (which == 2)  // both mains?
    {   
        GetVessel().SetThrusterGroupLevel(THGROUP_MAIN,  lmain); 
        GetVessel().SetThrusterGroupLevel(THGROUP_RETRO, lretro);
    } 
    else   // set individual engine
    {            
        GetVessel().SetThrusterLevel(GetXR1().th_main [which], lmain);
        GetVessel().SetThrusterLevel(GetXR1().th_retro[which], lretro);
    }

    // play error message if retro thrust requested but thrusters are disabled (thrusters already disabled, so OK to allow the thrust settings above)
    if ((GetXR1().m_isRetroEnabled == false) && (lretro != 0.0))
    {
        GetXR1().PlaySound(GetXR1().RetroDoorsAreClosed, DeltaGliderXR1::ST_WarningCallout);   // separate sound slot here so it ALWAYS plays (it is important)
        GetXR1().ShowWarning(nullptr, DeltaGliderXR1::ST_None, "Retro Doors are closed.");
    }

    return true;
}

bool MainThrottleArea::ProcessVCMouseEvent(const int event, const VECTOR3 &coords)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    if (event & PANEL_MOUSE_LBDOWN)   // record which slider to operate
    { 
        if      (coords.x < 0.3) m_vcCtrl = 0; // left engine
        else if (coords.x > 0.7) m_vcCtrl = 1; // right engine
        else                     m_vcCtrl = 2; // both
        m_vcMode = 2;
        m_vcPY = coords.y;
    } 
    else  // button up
    {
        for (int i = 0; i < 2; i++)
        {
            if (m_vcCtrl == i || m_vcCtrl == 2)
            {
                double lvl = GetVessel().GetThrusterLevel(GetXR1().th_main[i]) - GetVessel().GetThrusterLevel(GetXR1().th_retro[i]);
                if      (lvl > 0.0) m_vcMode = 0;
                else if (lvl < 0.0) m_vcMode = 1;
                double lmin = (m_vcMode == 0 ? 0.0 : -1.0); // prevent direct crossover from main to retro
                double lmax = (m_vcMode == 1 ? 0.0 :  1.0); // prevent direct crossover from retro to main
                lvl = max (lmin, min (lmax, lvl + 2.0*(coords.y-m_vcPY)));  
                if (fabs (lvl) < 0.01) lvl = 0.0;
                if (lvl >= 0.0)
                {
                    GetVessel().SetThrusterLevel(GetXR1().th_main[i], lvl);
                    GetVessel().SetThrusterLevel(GetXR1().th_retro[i], 0.0);
                } 
                else
                {
                    GetVessel().SetThrusterLevel(GetXR1().th_main[i], 0.0);
                    GetVessel().SetThrusterLevel(GetXR1().th_retro[i], -lvl);

                    // play error message if retro thrust requested but thrusters are disabled
                    if (GetXR1().m_isRetroEnabled == false)  
                    {
                        GetXR1().PlaySound(GetXR1().RetroDoorsAreClosed, DeltaGliderXR1::ST_WarningCallout);  // separate sound slot here so it ALWAYS plays (it is important)
                        GetXR1().ShowWarning(nullptr, DeltaGliderXR1::ST_None, "Retro Doors are closed.");
                    }
                }
            }
        }
        m_vcPY = coords.y;
    }
    return true;
}

//-------------------------------------------------------------------------

LargeHoverThrottleArea::LargeHoverThrottleArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    XR1Area(parentPanel, panelCoordinates, areaID), m_vcPY(0)
{
}

void LargeHoverThrottleArea::Activate()
{
    Area::Activate();  // invoke superclass method
    if (IsVC()) // 3D panel?
    {
        oapiVCRegisterArea(GetAreaID(), PANEL_REDRAW_ALWAYS, PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBPRESSED);
    }
    else    // 2D panel
    {
        oapiRegisterPanelArea(GetAreaID(), GetRectForSize(23, 134), PANEL_REDRAW_ALWAYS, PANEL_MOUSE_LBPRESSED, PANEL_MAP_BGONREQUEST);
        m_mainSurface = CreateSurface(IDB_SLIDER1);     // engine slider knob
    }

    // reset state variables to force a repaint
    m_engsliderpos = -1;
}

bool LargeHoverThrottleArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    bool retVal = false;

    double level = GetVessel().GetThrusterLevel(GetXR1().th_hover[0]);
    UINT pos = (UINT)((1.0-level)*116.0);
    if (pos != m_engsliderpos)
    {
        oapiBltPanelAreaBackground (GetAreaID(), surf);
        DeltaGliderXR1::SafeBlt (surf, m_mainSurface, 0, m_engsliderpos = pos, 0, 0, 23, 18);
        retVal = true;
    } 

    return retVal;
}

bool LargeHoverThrottleArea::Redraw3D(const int event, const SURFHANDLE surf)
{
    double level = GetVessel().GetThrusterLevel(GetXR1().th_hover[0]);
    UINT pos = (UINT)(level * 500.0);
    if (pos != m_engsliderpos)
    {
        GetXR1().SetXRAnimation(GetXR1().anim_hoverthrottle, level);
        m_engsliderpos = pos;
    }

    return true;
}

bool LargeHoverThrottleArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    int workY = my - 9; // make working copy

    if      (workY <   0) workY =   0;
    else if (workY > 116) workY = 116;

    GetVessel().SetThrusterGroupLevel(GetXR1().thg_hover, 1.0-workY/116.0);

    // play error message if retro thrust requested but thrusters are disabled (thrusters already disabled, so OK to allow the thrust settings above)
    if (GetXR1().m_isHoverEnabled == false)
    {
        GetXR1().PlaySound(GetXR1().HoverDoorsAreClosed, DeltaGliderXR1::ST_WarningCallout);   // separate sound slot here so it ALWAYS plays (it is important)
        GetXR1().ShowWarning(nullptr, DeltaGliderXR1::ST_None, "Hover Doors are closed.");
    }

    return true;
}

bool LargeHoverThrottleArea::ProcessVCMouseEvent(const int event, const VECTOR3 &coords)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    if (event & PANEL_MOUSE_LBDOWN) 
    { 
        m_vcPY = coords.y;
    } 
    else
    {
        double lvl = max (0.0, min (1.0, GetVessel().GetThrusterLevel(GetXR1().th_hover[0]) + (coords.y-m_vcPY)));
        if (lvl < 0.01) lvl = 0.0;
        for (int i = 0; i < 2; i++) 
            GetVessel().SetThrusterLevel(GetXR1().th_hover[i], lvl);

        m_vcPY = coords.y;
    }
    return true;
}

//-------------------------------------------------------------------------

// Note: this is currently not used
#if 0
SmallHoverThrottleArea::SmallHoverThrottleArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    XR1Area(parentPanel, panelCoordinates, areaID)
{
}

void SmallHoverThrottleArea::Activate()
{
    Area::Activate();  // invoke superclass method
    // No VC for this area
    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(23, 68), PANEL_REDRAW_ALWAYS, PANEL_MOUSE_LBPRESSED, PANEL_MAP_BGONREQUEST);
    m_mainSurface = CreateSurface(IDB_SLIDER1);     // engine slider knob

    // reset state variables to force a repaint
    m_engsliderpos = -1;
}

bool SmallHoverThrottleArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    bool retVal = false;

    double level = GetVessel().GetThrusterLevel(GetXR1().th_hover[0]);
    UINT pos = (UINT)((1.0-level) * 50.0);
    if (pos != m_engsliderpos)
    {
        oapiBltPanelAreaBackground (GetAreaID(), surf);
        DeltaGliderXR1::SafeBlt (surf, m_mainSurface, 0, m_engsliderpos = pos, 0, 0, 23, 18);
        retVal = true;
    } 

    return retVal;
}

bool SmallHoverThrottleArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    int workY = my - 9; // make working copy

    if      (workY <   0) workY =  0;
    else if (workY >  50) workY = 50;

    GetVessel().SetThrusterGroupLevel(GetXR1().thg_hover, 1.0-workY/50.0);

    // play error message if retro thrust requested but thrusters are disabled (thrusters already disabled, so OK to allow the thrust settings above)
    if (GetXR1().m_isHoverEnabled == false)
    {
        GetXR1().PlaySound(GetXR1().HoverDoorsAreClosed);   // separate sound slot here so it ALWAYS plays (it is important)
        GetXR1().ShowWarning(nullptr, DeltaGliderXR1::ST_None, "Hover Doors are closed.");
    }

    return true;
}
#endif

//-------------------------------------------------------------------------

ScramThrottleArea::ScramThrottleArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    XR1Area(parentPanel, panelCoordinates, areaID), m_vcCtrl(0), m_vcMode(0), m_vcPY(0)
{
}

void ScramThrottleArea::Activate()
{
    Area::Activate();  // invoke superclass method
    if (IsVC()) // 3D panel?
    {
        oapiVCRegisterArea(GetAreaID(), PANEL_REDRAW_ALWAYS, PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBPRESSED);
    }
    else    // 2D panel
    {
        oapiRegisterPanelArea(GetAreaID(), GetRectForSize(49, 102), PANEL_REDRAW_ALWAYS, PANEL_MOUSE_LBPRESSED, PANEL_MAP_BGONREQUEST);
        m_mainSurface = CreateSurface(IDB_SLIDER1);     // engine slider knob
    }

    // reset state variables to force a repaint
    m_engsliderpos[0] = m_engsliderpos[1] = -1;
}

bool ScramThrottleArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    UINT i, pos;
    bool redraw = false;

    for (i = 0; i < 2; i++)
    {
        double level = GetVessel().GetThrusterLevel(GetXR1().th_scram[i]);
        pos = (UINT)((1.0-level) * 84.0);
        if (pos != m_engsliderpos[i])
            m_engsliderpos[i] = pos, redraw = true;
    }

    if (redraw)
    {
        oapiBltPanelAreaBackground (AID_ENGINESCRAM, surf);
        for (i = 0; i < 2; i++)
            DeltaGliderXR1::SafeBlt(surf, m_mainSurface, i * 26, m_engsliderpos[i], 0, 0, 23, 18);
    }

    return redraw;
}

bool ScramThrottleArea::Redraw3D(const int event, const SURFHANDLE surf)
{
    for (int i = 0; i < 2; i++)
    {
        double level = GetVessel().GetThrusterLevel(GetXR1().th_scram[i]);
        UINT pos = (UINT)(level*500.0);
        if (pos != m_engsliderpos[i])
        {
            GetXR1().SetXRAnimation(GetXR1().anim_scramthrottle[i], level);
            m_engsliderpos[i] = pos;
        }
    }
    return true;
}

bool ScramThrottleArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    int which;

    if      (mx <  12) which = 0; // left engine
    else if (mx >= 37) which = 1; // right engine
    else               which = 2; // both
    
    double level = max (0.0, min(1.0, 1.0 - (my/84.0)));    // throttle level

    for (int i = 0; i < 2; i++)
    {
        if (which != 1-i)
        {
            GetVessel().SetThrusterLevel(GetXR1().th_scram[i], level);
            GetXR1().scram_intensity[i] = level * GetXR1().scram_max[i];
        }
    }

    // play error message if retro thrust requested but thrusters are disabled (thrusters already disabled, so OK to allow the thrust settings above)
    if (GetXR1().m_isScramEnabled == false)
    {
        GetXR1().PlaySound(GetXR1().ScramDoorsAreClosed, DeltaGliderXR1::ST_WarningCallout);   // separate sound slot here so it ALWAYS plays (it is important)
        GetXR1().ShowWarning(nullptr, DeltaGliderXR1::ST_None, "SCRAM Doors are closed.");
    }

    return true;
}

bool ScramThrottleArea::ProcessVCMouseEvent(const int event, const VECTOR3 &coords)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    if (event & PANEL_MOUSE_LBDOWN)  // record which slider to operate
    { 
        if      (coords.x < 0.3) m_vcCtrl = 0; // left engine
        else if (coords.x > 0.7) m_vcCtrl = 1; // right engine
        else                     m_vcCtrl = 2; // both
        m_vcPY = coords.y;
    }
    else
    {
        for (int i = 0; i < 2; i++)
        {
            if (m_vcCtrl == i || m_vcCtrl == 2)
            {
                double lvl = max (0.0, min (1.0, GetVessel().GetThrusterLevel(GetXR1().th_scram[i]) + (coords.y-m_vcPY)));
                if (lvl < 0.01) lvl = 0.0;
                GetVessel().SetThrusterLevel(GetXR1().th_scram[i], lvl);
            }
        }
        m_vcPY = coords.y;
    }
    return true;
}

//-------------------------------------------------------------------------

HoverBalanceSwitchArea::HoverBalanceSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID) :
    VerticalCenteringRockerSwitchArea(parentPanel, panelCoordinates, areaID, meshTextureID, false)     // this is a single switch
{
    // must set this here after base class is initialized because GetXR1() is in the base class
    SetXRAnimationHandle(&GetXR1().anim_hbalance);  
}

// Process a mouse event that occurred on our switch
// switches = which switches moved (LEFT, RIGHT, BOTH, SINGLE, NA)
// position = current switch position (UP, DOWN, CENTER)
void HoverBalanceSwitchArea::ProcessSwitchEvent(SWITCHES switches, POSITION position)
{
    GetXR1().ShiftHoverBalance(ToGIMBAL_SWITCH(switches), ToDIRECTION(position));
}

//----------------------------------------------------------------------------------

HoverBalanceVerticalGaugeArea::HoverBalanceVerticalGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID) :
    VerticalGaugeArea(parentPanel, panelCoordinates, areaID, false, 63, PANEL_REDRAW_ALWAYS, meshTextureID)  // single gauge 63 pixels high
{
}

// side will always be LEFT for a single gauge
VerticalGaugeArea::RENDERDATA HoverBalanceVerticalGaugeArea::GetRenderData(const SIDE side)
{
    int idx = static_cast<int>(28.4999 * (1.0 - (GetXR1().m_hoverBalance / MAX_HOVER_IMBALANCE)));

    return _RENDERDATA(COLOR::GREEN, idx);
}

//-------------------------------------------------------------------------

ScramPitchSwitchArea::ScramPitchSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID) :
    VerticalCenteringRockerSwitchArea(parentPanel, panelCoordinates, areaID, meshTextureID, true)     // this is a DUAL switch
{
    SetXRAnimationHandle(GetXR1().anim_scramgimbal);
}

// Process a mouse event that occurred on our switch
// switches = which switches moved (LEFT, RIGHT, BOTH, SINGLE, NA)
// position = current switch position (UP, DOWN, CENTER)
void ScramPitchSwitchArea::ProcessSwitchEvent(SWITCHES switches, POSITION position)
{
    GetXR1().GimbalSCRAMPitch(ToGIMBAL_SWITCH(switches), ToDIRECTION(position));
}

//----------------------------------------------------------------------------------

ScramPitchVerticalGaugeArea::ScramPitchVerticalGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID) :
    VerticalGaugeArea(parentPanel, panelCoordinates, areaID, true, 77, PANEL_REDRAW_ALWAYS, meshTextureID)  // dual gauge 77 pixels high
{
}

VerticalGaugeArea::RENDERDATA ScramPitchVerticalGaugeArea::GetRenderData(const SIDE side)
{
    VECTOR3 dir;
    const int thIndex = ((side == SIDE::LEFT) ? 0 : 1);

    GetVessel().GetThrusterDir(GetXR1().th_scram[thIndex], dir);  
    const double phi = atan2 (dir.y, dir.z);

    int idx = static_cast<int>(35*(phi-SCRAM_DEFAULT_DIR+SCRAM_GIMBAL_RANGE)/SCRAM_GIMBAL_RANGE);  // pixel index
    return _RENDERDATA(COLOR::GREEN, idx);
}

//----------------------------------------------------------------------------------

MainPitchSwitchArea::MainPitchSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID) :
    VerticalCenteringRockerSwitchArea(parentPanel, panelCoordinates, areaID, meshTextureID, true)     // this is a DUAL switch
{
    // must set this here after base class is initialized because GetXR1() is in the base class
    SetXRAnimationHandle(GetXR1().anim_pmaingimbal);  
}

// Process a mouse event that occurred on our switch
// switches = which switches moved (LEFT, RIGHT, BOTH, SINGLE, NA)
// position = current switch position (UP, DOWN, CENTER)
void MainPitchSwitchArea::ProcessSwitchEvent(SWITCHES switches, POSITION position)
{
    GetXR1().GimbalMainPitch(ToGIMBAL_SWITCH(switches), ToDIRECTION(position));
}

//----------------------------------------------------------------------------------

MainPitchVerticalGaugeArea::MainPitchVerticalGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID) :
    VerticalGaugeArea(parentPanel, panelCoordinates, areaID, true, 77, PANEL_REDRAW_ALWAYS, meshTextureID)  // dual gauge 77 pixels high
{
}

// side will always be LEFT for a single gauge
VerticalGaugeArea::RENDERDATA MainPitchVerticalGaugeArea::GetRenderData(const SIDE side)
{
    VECTOR3 dir;
    const int thIndex = ((side == SIDE::LEFT) ? 0 : 1);
    GetVessel().GetThrusterDir(GetXR1().th_main[thIndex], dir);

    int idx = static_cast<int>(35.4999*(dir.y/MAIN_PGIMBAL_RANGE+1.0));

    return _RENDERDATA(COLOR::GREEN, idx);
}

//-------------------------------------------------------------------------

MainYawSwitchArea::MainYawSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID) :
    HorizontalCenteringRockerSwitchArea(parentPanel, panelCoordinates, areaID, meshTextureID, true)     // this is a DUAL switch
{
    // must set this here after base class is initialized because GetXR1() is in the base class
    SetXRAnimationHandle(GetXR1().anim_ymaingimbal);  
}

// Process a mouse event that occurred on our switch
// switches = which switches moved (TOP, BOTTOM, BOTH, SINGLE, NA)
// position = current switch position (LEFT, RIGHT, CENTER)
void MainYawSwitchArea::ProcessSwitchEvent(SWITCHES switches, POSITION position)
{
    GetXR1().GimbalMainYaw(ToGIMBAL_SWITCH(switches), ToDIRECTION(position));
}

//----------------------------------------------------------------------------------

MainYawHorizontalGaugeArea::MainYawHorizontalGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID) :
    HorizontalGaugeArea(parentPanel, panelCoordinates, areaID, true, 77, PANEL_REDRAW_ALWAYS, meshTextureID)  // dual gauge 77 pixels wide
{
}

HorizontalGaugeArea::RENDERDATA MainYawHorizontalGaugeArea::GetRenderData(const SIDE side)
{
    VECTOR3 dir;
    const int thIndex = ((side == SIDE::TOP) ? 0 : 1);

    GetVessel().GetThrusterDir(GetXR1().th_main[thIndex], dir);

    // get pixel index
    int idx = static_cast<int>(35.4999*(1.0-dir.x/MAIN_YGIMBAL_RANGE));

    return _RENDERDATA(COLOR::GREEN, idx);
}
