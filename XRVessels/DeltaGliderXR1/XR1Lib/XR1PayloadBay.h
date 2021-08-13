/**
  XR Vessel add-ons for OpenOrbiter Space Flight Simulator
  Copyright (C) 2006-2021 Douglas Beachy

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

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
// XR1Payload.cpp
// Class defining the XR1's payload bay
// ==============================================================

#pragma once

#include "DeltaGliderXR1.h"
#include "XRPayloadBay.h"  // cannot use forward reference here!  Otherwise inline code using XR1Ravenstar object is generated incorrectly.

class XR1PayloadBay : public XRPayloadBay 
{
public:
    XR1PayloadBay(VESSEL &parentVessel);

    DeltaGliderXR1 &GetXR1() const { return static_cast<DeltaGliderXR1 &>(GetParentVessel()); }

    // hook the base class callback
    virtual void clbkChildCreatedInBay(XRPayloadBaySlot &slotWithNewChild);
};
