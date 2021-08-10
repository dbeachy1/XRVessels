// ==============================================================
// XR3Phoenix implementation class
//
// Copyright 2007-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR3Payload.cpp
// Class defining the XR3's payload bay
// ==============================================================

#pragma once

#include "XR3Phoenix.h"
#include "XR1PayloadBay.h"  // cannot use forward reference here!  Otherwise inline code using XR3Phoenix object is generated incorrectly.

class XR3PayloadBay : public XR1PayloadBay 
{
public:
    XR3PayloadBay(VESSEL &parentVessel);

    VECTOR3 GetLandedDeployToCoords(const int slotNumber);
    XR3Phoenix &GetXR3() const { return static_cast<XR3Phoenix &>(GetParentVessel()); }
};
