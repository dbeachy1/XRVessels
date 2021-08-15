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
// XR1Component.h
// Common base class for all XR1 components
// ==============================================================

#pragma once

#include "Orbitersdk.h"
#include "vessel3ext.h"
#include "DeltaGliderXR1.h"
#include "Component.h"

class XR1Component : public Component
{
public:
    // Constructor
    // screenMeshGroup = -1 = NONE
    XR1Component(InstrumentPanel &parentPanel, const COORD2 topLeft, const int meshTextureID = VCPANEL_TEXTURE_NONE, const int screenMeshGroup = -1) : 
        Component(parentPanel, topLeft, meshTextureID, screenMeshGroup) { }

    // convenience method to retrive our XR1 vessel
    DeltaGliderXR1 &GetXR1() const { return static_cast<DeltaGliderXR1 &>(GetVessel()); }
};
