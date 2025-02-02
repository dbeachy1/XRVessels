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
// XR1FuelDisplayComponent.h
// Handles main, hover, and scram throttle controls
// ==============================================================

#pragma once

#include "orbitersdk.h"
#include "XR1Component.h"
#include "XR1Areas.h"

class FuelDisplayComponent : public XR1Component
{
public:
    FuelDisplayComponent(InstrumentPanel &instrumentPanel, COORD2 topLeft);
};

//----------------------------------------------------------------------------------

class FuelRemainingBarArea : public BarArea
{
public:
    FuelRemainingBarArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, PROPELLANT_HANDLE ph);

protected:
    virtual RENDERDATA GetRenderData();

    // state data 
    PROPELLANT_HANDLE m_propHandle;   // handle for fuel tank being measured
};

//----------------------------------------------------------------------------------

class FuelRemainingPCTNumberArea : public NumberArea
{
public:
    FuelRemainingPCTNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, PROPELLANT_HANDLE ph);

protected:
    virtual bool UpdateRenderData(RENDERDATA &renderData);

    // state data 
    PROPELLANT_HANDLE m_propHandle;   // handle for fuel tank being measured
};

//----------------------------------------------------------------------------------

class FuelRemainingKGNumberArea : public NumberArea
{
public:
    FuelRemainingKGNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, PROPELLANT_HANDLE ph);

protected:
    virtual bool UpdateRenderData(RENDERDATA &renderData);

    // state data 
    PROPELLANT_HANDLE m_propHandle;   // handle for fuel tank being measured
};
