// ==============================================================
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// Copyright 2006-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR1PrePostStep.h
// Base class common to all our PostStep objects.
// ==============================================================

#pragma once

#include "PrePostStep.h"
#include "DeltaGliderXR1.h"

class XR1PrePostStep : public PrePostStep
{
public:
    // inline constructor
    XR1PrePostStep(DeltaGliderXR1 &vessel) :
        PrePostStep(vessel)
    {
    }

    // convenience methods
    DeltaGliderXR1 &GetXR1() const { return static_cast<DeltaGliderXR1 &>(GetVessel()); }
};
