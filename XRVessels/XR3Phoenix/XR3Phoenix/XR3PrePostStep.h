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
// XR3Phoenix implementation class
//
// XR3PrePostStep.h
// Class common to all our Pre/PostStep objects.
// ==============================================================

#pragma once

#include "PrePostStep.h"
#include "XR3Phoenix.h"
#include "XR1PrePostStep.h"

class XR3PrePostStep : public XR1PrePostStep
{
public:
    // inline constructor
    XR3PrePostStep(XR3Phoenix &vessel) :
        XR1PrePostStep(vessel)
    {
    }

    // convenience methods
    XR3Phoenix &GetXR3() const { return static_cast<XR3Phoenix &>(GetVessel()); }
};
