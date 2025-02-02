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
// XR1EngineDisplayComponent.cpp
// Handles main, hover, and scram throttle controls
// ==============================================================

#include "DeltaGliderXR1.h"
#include "XR1EngineDisplayComponent.h"
#include "AreaIDs.h"

// topLeft = top-left corner @ inside edge of screen
EngineDisplayComponent::EngineDisplayComponent(InstrumentPanel &parentPanel, COORD2 topLeft) :
    XR1Component(parentPanel, topLeft)
{
    AddArea(new EngineEfficiencyGaugeArea(parentPanel, GetAbsCoords(_COORD2( 40, 13)), AID_ENGINE_EFFICIENCY));  // -3 offset to handle the arrow's 3 pixes to the left of center

    // thrust bars
    AddArea(new MainRetroThrustBarArea    (parentPanel, GetAbsCoords(_COORD2( 43, 21)), AID_THROTTLEBAR_MAINL));
    AddArea(new MainRetroThrustBarArea    (parentPanel, GetAbsCoords(_COORD2( 43, 30)), AID_THROTTLEBAR_MAINR));
    AddArea(new NormalThrustBarArea       (parentPanel, GetAbsCoords(_COORD2( 43, 43)), AID_THROTTLEBAR_HOVER,  GetXR1().th_hover[0]));  // hover throttles are locked, so one bar is sufficient
    AddArea(new NormalThrustBarArea       (parentPanel, GetAbsCoords(_COORD2( 43, 56)), AID_THROTTLEBAR_SCRAML, GetXR1().th_scram[0]));
    AddArea(new NormalThrustBarArea       (parentPanel, GetAbsCoords(_COORD2( 43, 65)), AID_THROTTLEBAR_SCRAMR, GetXR1().th_scram[1]));

    // thrust numbers
    AddArea(new MainRetroThrustNumberArea (parentPanel, GetAbsCoords(_COORD2(132, 24)), AID_THRUSTMAIN_KN));
    AddArea(new HoverThrustNumberArea     (parentPanel, GetAbsCoords(_COORD2(132, 42)), AID_THRUSTHOVER_KN));
    AddArea(new ScramThrustNumberArea     (parentPanel, GetAbsCoords(_COORD2(132, 59)), AID_THRUSTSCRAM_KN));
    
    // G ACC indicators
    AddArea(new AccScaleArea              (parentPanel, GetAbsCoords(_COORD2( 39, 74)), AID_ACC_SCALE));
    AddArea(new AccHorizontalGaugeArea    (parentPanel, GetAbsCoords(_COORD2( 40, 87)), AID_ACCX_G, AccHorizontalGaugeArea::AXIS::X, false, HorizontalGaugeArea::SIDE::TOP));
    AddArea(new AccHorizontalGaugeArea    (parentPanel, GetAbsCoords(_COORD2( 40, 95)), AID_ACCY_G, AccHorizontalGaugeArea::AXIS::Y, true,  HorizontalGaugeArea::SIDE::BOTTOM));  // singleSide ignored here
    AddArea(new AccHorizontalGaugeArea    (parentPanel, GetAbsCoords(_COORD2( 40,108)), AID_ACCZ_G, AccHorizontalGaugeArea::AXIS::Z, false, HorizontalGaugeArea::SIDE::BOTTOM));

    // ACC m/s^2 numbers
    AddArea(new AccNumberArea             (parentPanel, GetAbsCoords(_COORD2(131, 83)), AID_ACCX_NUMBER, AccNumberArea::AXIS::X));
    AddArea(new AccNumberArea             (parentPanel, GetAbsCoords(_COORD2(131, 95)), AID_ACCY_NUMBER, AccNumberArea::AXIS::Y));
    AddArea(new AccNumberArea             (parentPanel, GetAbsCoords(_COORD2(131,107)), AID_ACCZ_NUMBER, AccNumberArea::AXIS::Z));
}   

//-------------------------------------------------------------------------

// NOTE: we need six extra pixels in width to accomodate 1/2 of the pointer sticking out over each end of the bar (3 pixels per side)
// Also, remember that the range of a gauge is INCLUSIVE, so if the range is 0-84, the size is actually 85 pixels, not 84.
EngineEfficiencyGaugeArea::EngineEfficiencyGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    PctHorizontalGaugeArea(parentPanel, panelCoordinates, areaID, false, 91, PANEL_REDRAW_ALWAYS)  // single gauge 91 (85+6) pixels wide
{
}

double EngineEfficiencyGaugeArea::GetFraction(const SIDE side, COLOR &color)
{
    double efficiency = GetVessel().GetThrusterIsp(GetXR1().th_main[0]) / GetVessel().GetThrusterIsp0(GetXR1().th_main[0]);   // 0...1
    if (efficiency < 0)     // in case we're landed on Venus, etc.
        efficiency = 0;

    color = COLOR::GREEN;  // set color of indicator arrow
    return efficiency;
}

//-------------------------------------------------------------------------

// used for hover and scram thrust bars only
NormalThrustBarArea::NormalThrustBarArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, THRUSTER_HANDLE th) :
    BarArea(parentPanel, panelCoordinates, areaID, 85, 7, ORIENTATION::HORIZONTAL),
    m_thrusterHandle(th)
{
}

BarArea::RENDERDATA NormalThrustBarArea::GetRenderData()
{
    double currentThrustLevel = GetVessel().GetThrusterLevel(m_thrusterHandle);  // 0...1
    return _RENDERDATA(COLOR::GREEN, currentThrustLevel, currentThrustLevel, 1.0);
}

//-------------------------------------------------------------------------

// handles main/retro thrust bar
MainRetroThrustBarArea::MainRetroThrustBarArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    BarArea(parentPanel, panelCoordinates, areaID, 85, 7, ORIENTATION::HORIZONTAL)
{
}

BarArea::RENDERDATA MainRetroThrustBarArea::GetRenderData()
{
    int thruster = ((GetAreaID() == AID_THROTTLEBAR_MAINL) ? 0 : 1);

    // Either the MAIN or the RETRO may be firing, but not BOTH.
    // get the thrust levels
    COLOR color = COLOR::GREEN;   // assume mains firing
    double level = GetVessel().GetThrusterLevel(GetXR1().th_main[thruster]);
    if (level == 0)
    {
        // main isn't firing; check the retro
        level = GetVessel().GetThrusterLevel(GetXR1().th_retro[thruster]);  // level may be 0 here
        if (level > 0)
        {
            color = COLOR::RED; // retros firing!
        }
    }
    
    return _RENDERDATA(color, level, level, 1.0);
}

//-------------------------------------------------------------------------

MainRetroThrustNumberArea::MainRetroThrustNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    ThrustNumberArea(parentPanel, panelCoordinates, areaID)
{
}

double MainRetroThrustNumberArea::GetThrust()
{
    // add up total thrust from both mains and both retros
    // NOTE: either one or the other, but not BOTH, may be firing
    double totalThrust = 0;

    // first the mains
    for (int i=0; i < 2; i++)
    {
        double currentMaxThrust = GetVessel().GetThrusterMax(GetXR1().th_main[i]);  // takes atmospheric pressure into account
        double throttleLevel = GetVessel().GetThrusterLevel(GetXR1().th_main[i]);
        totalThrust += (currentMaxThrust * throttleLevel);  // thrust in Newtons
    }

    // now the retros
    for (int i=0; i < 2; i++)
    {
        double currentMaxThrust = GetVessel().GetThrusterMax(GetXR1().th_retro[i]);  // takes atmospheric pressure into account
        double throttleLevel = GetVessel().GetThrusterLevel(GetXR1().th_retro[i]);
        totalThrust += (currentMaxThrust * throttleLevel);  // thrust in Newtons
    }
    
    return totalThrust / 1000;
}

//-------------------------------------------------------------------------

// pThrusters = THRUSTER_HANDLE[2] pointer (two engines)
HoverThrustNumberArea::HoverThrustNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    ThrustNumberArea(parentPanel, panelCoordinates, areaID)
{
}

double HoverThrustNumberArea::GetThrust()
{
    // add up total thrust from both engines
    double totalThrust = 0;
    for (int i=0; i < 2; i++)
    {
        double currentMaxThrust = GetVessel().GetThrusterMax(GetXR1().th_hover[i]);  // takes atmospheric pressure into account
        double throttleLevel = GetVessel().GetThrusterLevel(GetXR1().th_hover[i]);
        totalThrust += (currentMaxThrust * throttleLevel);  // thrust in Newtons
    }

    return totalThrust / 1000;
}

//-------------------------------------------------------------------------

// display scram thrust 
ScramThrustNumberArea::ScramThrustNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    ThrustNumberArea(parentPanel, panelCoordinates, areaID)
{
}

double ScramThrustNumberArea::GetThrust()
{
    // add up total thrust from both engines
    double totalThrust = 0;
    for (int i=0; i < 2; i++)
    {
        // SCRAM engines area special case; do not call GetThrusterMax for these
        double singleEngineThrust = GetXR1().ramjet->GetMostRecentThrust(i);  // thrust in newtons
        totalThrust += singleEngineThrust;
    }

    return totalThrust / 1000;
}
