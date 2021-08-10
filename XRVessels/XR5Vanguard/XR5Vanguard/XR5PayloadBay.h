// ==============================================================
// XR5Vanguard implementation class
//
// Copyright 2007-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR5Payload.cpp
// Class defining the XR5's payload bay
// ==============================================================

#pragma once

#include "XR5Vanguard.h"
#include "XR1PayloadBay.h"  // cannot use forward reference here!  Otherwise inline code using XR5Vanguard object is generated incorrectly.

class XR5PayloadBay : public XR1PayloadBay 
{
public:
    XR5PayloadBay(VESSEL &parentVessel);

    VECTOR3 GetLandedDeployToCoords(const int slotNumber);
    XR5Vanguard &GetXR5() const { return static_cast<XR5Vanguard &>(GetParentVessel()); }
};
