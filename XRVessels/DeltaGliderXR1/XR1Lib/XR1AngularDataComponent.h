// ==============================================================
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// Copyright 2006-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR1AngularDataComponent.h
// Handles main, hover, and scram throttle controls
// ==============================================================

#pragma once

#include "orbitersdk.h"
#include "XR1Component.h"
#include "XR1Areas.h"

class AngularDataComponent : public XR1Component
{
public:
    AngularDataComponent(InstrumentPanel &instrumentPanel, COORD2 topLeft, const int meshTextureID = VCPANEL_TEXTURE_NONE);
};

//----------------------------------------------------------------------------------

class AngularDataArea : public XR1Area
{
public:
    enum Type { PITCH, BANK, YAW };
    AngularDataArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const Type type, const int meshTextureID = VCPANEL_TEXTURE_NONE);
    virtual void Activate();

protected:
    Type m_type;
    int m_lastRenderedIndex;
};

//----------------------------------------------------------------------------------

class RotationalVelocityArea : public AngularDataArea
{
public:
    RotationalVelocityArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const Type type, const int meshTextureID);
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
};

//----------------------------------------------------------------------------------

class RotationalAccArea : public AngularDataArea
{
public:
    RotationalAccArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const Type type, const int meshTextureID);
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
};

//----------------------------------------------------------------------------------

class RotationalTorqueArea : public AngularDataArea
{
public:
    RotationalTorqueArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const Type type, const int meshTextureID);
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
};
