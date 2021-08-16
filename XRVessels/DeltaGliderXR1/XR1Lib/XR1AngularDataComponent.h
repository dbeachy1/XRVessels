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
// XR1AngularDataComponent.h
// Handles main, hover, and scram throttle controls
// ==============================================================

#pragma once

#include "orbitersdk.h"
#include "XR1Component.h"
#include "XR1Areas.h"

class AngularDataComponent : public XR1Component
{
public:
    AngularDataComponent(InstrumentPanel &instrumentPanel, COORD2 topLeft, const int meshTextureID = VCPANEL_TEXTURE_NONE);
};

//----------------------------------------------------------------------------------

class AngularDataArea : public XR1Area
{
public:
    enum class Type { PITCH, BANK, YAW };
    AngularDataArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const Type type, const int meshTextureID = VCPANEL_TEXTURE_NONE);
    virtual void Activate();

protected:
    Type m_type;
    int m_lastRenderedIndex;
};

//----------------------------------------------------------------------------------

class RotationalVelocityArea : public AngularDataArea
{
public:
    RotationalVelocityArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const Type type, const int meshTextureID);
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
};

//----------------------------------------------------------------------------------

class RotationalAccArea : public AngularDataArea
{
public:
    RotationalAccArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const Type type, const int meshTextureID);
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
};

//----------------------------------------------------------------------------------

class RotationalTorqueArea : public AngularDataArea
{
public:
    RotationalTorqueArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const Type type, const int meshTextureID);
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
};
