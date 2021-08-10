// ==============================================================
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// Copyright 2007-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
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
