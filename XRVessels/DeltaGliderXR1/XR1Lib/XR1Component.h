// ==============================================================
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// Copyright 2006-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR1Component.h
// Common base class for all XR1 components
// ==============================================================

#pragma once

#include "Orbitersdk.h"
#include "vessel3ext.h"
#include "DeltaGliderXR1.h"
#include "Component.h"

class XR1Component : public Component
{
public:
    // Constructor
    // screenMeshGroup = -1 = NONE
    XR1Component(InstrumentPanel &parentPanel, const COORD2 topLeft, const int meshTextureID = VCPANEL_TEXTURE_NONE, const int screenMeshGroup = -1) : 
        Component(parentPanel, topLeft, meshTextureID, screenMeshGroup) { }

    // convenience method to retrive our XR1 vessel
    DeltaGliderXR1 &GetXR1() const { return static_cast<DeltaGliderXR1 &>(GetVessel()); }
};



