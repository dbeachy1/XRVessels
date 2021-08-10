// ==============================================================
// XR3Phoenix implementation class
//
// Copyright 2007-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
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
