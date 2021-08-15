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
// XR1EngineDisplayComponent.h
// Handles main, hover, and scram throttle controls
// ==============================================================

#pragma once

#include "orbitersdk.h"
#include "XR1Component.h"
#include "XR1Areas.h"

class EngineDisplayComponent : public XR1Component
{
public:
    EngineDisplayComponent(InstrumentPanel &instrumentPanel, COORD2 topLeft);
};

//----------------------------------------------------------------------------------

class EngineEfficiencyGaugeArea : public PctHorizontalGaugeArea
{
public:
    EngineEfficiencyGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual double GetFraction(const SIDE side, COLOR &color);
};

//----------------------------------------------------------------------------------

class NormalThrustBarArea : public BarArea
{
public:
    NormalThrustBarArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, THRUSTER_HANDLE th);

protected:
    virtual RENDERDATA GetRenderData();

    // state data 
    THRUSTER_HANDLE m_thrusterHandle;   // handle of thruster being measured
};

//----------------------------------------------------------------------------------

class MainRetroThrustBarArea : public BarArea
{
public:
    MainRetroThrustBarArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual RENDERDATA GetRenderData();
};

//----------------------------------------------------------------------------------

class MainRetroThrustNumberArea : public ThrustNumberArea
{
public:
    MainRetroThrustNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual double GetThrust();  // thrust in kN
};

//----------------------------------------------------------------------------------

class HoverThrustNumberArea : public ThrustNumberArea
{
public:
    HoverThrustNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual double GetThrust();  // thrust in kN
};


//----------------------------------------------------------------------------------

class ScramThrustNumberArea : public ThrustNumberArea
{
public:
    ScramThrustNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual double GetThrust();  // thrust in kN
};
