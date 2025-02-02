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
// Component.h
// Abstract base class defining a panel component on a 2D or 3D panel
// ==============================================================

#pragma once

#include "Orbitersdk.h"
#include "vessel3ext.h"

#include <vector>

using namespace std; 

// use forward references to avoid circular dependency
class InstrumentPanel;
class Area;

class Component
{
public:
    // Constructor
    Component(InstrumentPanel &parentPanel, const COORD2 topLeft, const int meshTextureID = VCPANEL_TEXTURE_NONE, const int screenMeshGroup = -1);
    virtual ~Component();

    InstrumentPanel &GetParentPanel() { return m_parentPanel; };
    COORD2 GetTopLeft() { return m_topLeft; };
    void AddArea(Area *pArea);
    bool IsVC() const;
    VESSEL3_EXT &GetVessel() const;
    const vector<Area *> &GetAreas() { return m_areaVector; }

protected:
    // methods
    COORD2 GetAbsCoords(const COORD2 relativeCoordinates);

    // data
    vector<Area *> m_areaVector;    // list of all areas on the component
    const int m_meshTextureID;      // arbitrary vessel-specific constant denoting which VC mesh texture on which this component's areas reside
    const int m_screenMeshGroup;    // mesh group ID from our parent vessel's mesh file

private:
    // data
    InstrumentPanel &m_parentPanel;
    COORD2 m_topLeft;     // coordinates of top-left corner of component
};
