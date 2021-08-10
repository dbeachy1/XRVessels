// ==============================================================
// XR2Ravenstar implementation class
//
// Copyright 2007-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR2Payload.cpp
// Class defining the XR2's payload bay
// ==============================================================

#pragma once

#include "XR2Ravenstar.h"
#include "XR1PayloadBay.h"  // cannot use forward reference here!  Otherwise inline code using XR2Ravenstar object is generated incorrectly.

class XR2PayloadBay : public XR1PayloadBay 
{
public:
    XR2PayloadBay(VESSEL &parentVessel);

    VECTOR3 GetLandedDeployToCoords(const int slotNumber);
    XR2Ravenstar &GetXR2() const { return static_cast<XR2Ravenstar &>(GetParentVessel()); }
};
