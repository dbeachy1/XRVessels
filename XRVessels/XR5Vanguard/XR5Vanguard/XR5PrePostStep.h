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
// XR5Vanguard implementation class
//
// XR5PrePostStep.h
// Class common to all our Pre/PostStep objects.
// ==============================================================

#pragma once

#include "PrePostStep.h"
#include "XR5Vanguard.h"
#include "XR1PrePostStep.h"

class XR5PrePostStep : public XR1PrePostStep
{
public:
    // inline constructor
    XR5PrePostStep(XR5Vanguard &vessel) :
        XR1PrePostStep(vessel)
    {
    }

    // convenience methods
    XR5Vanguard &GetXR5() const { return static_cast<XR5Vanguard &>(GetVessel()); }
};
