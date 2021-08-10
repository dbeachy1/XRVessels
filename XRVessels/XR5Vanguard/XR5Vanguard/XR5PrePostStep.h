// ==============================================================
// XR5Vanguard implementation class
//
// Copyright 2007-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
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
