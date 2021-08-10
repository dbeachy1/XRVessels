// ==============================================================
// XR2Ravenstar implementation class
//
// Copyright 2008-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR2PrePostStep.h
// Class common to all our Pre/PostStep objects.
// ==============================================================

#pragma once

#include "PrePostStep.h"
#include "XR2Ravenstar.h"
#include "XR1PrePostStep.h"

class XR2PrePostStep : public XR1PrePostStep
{
public:
    // inline constructor
    XR2PrePostStep(XR2Ravenstar &vessel) :
        XR1PrePostStep(vessel)
    {
    }

    // convenience methods
    XR2Ravenstar &GetXR2() const { return static_cast<XR2Ravenstar &>(GetVessel()); }
};
