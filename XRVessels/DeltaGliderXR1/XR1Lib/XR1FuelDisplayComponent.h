// ==============================================================
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// Copyright 2006-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR1FuelDisplayComponent.h
// Handles main, hover, and scram throttle controls
// ==============================================================

#pragma once

#include "orbitersdk.h"
#include "XR1Component.h"
#include "XR1Areas.h"

class FuelDisplayComponent : public XR1Component
{
public:
    FuelDisplayComponent(InstrumentPanel &instrumentPanel, COORD2 topLeft);
};

//----------------------------------------------------------------------------------

class FuelRemainingBarArea : public BarArea
{
public:
    FuelRemainingBarArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, PROPELLANT_HANDLE ph);

protected:
    virtual RENDERDATA GetRenderData();

    // state data 
    PROPELLANT_HANDLE m_propHandle;   // handle for fuel tank being measured
};

//----------------------------------------------------------------------------------

class FuelRemainingPCTNumberArea : public NumberArea
{
public:
    FuelRemainingPCTNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, PROPELLANT_HANDLE ph);

protected:
    virtual bool UpdateRenderData(RENDERDATA &renderData);

    // state data 
    PROPELLANT_HANDLE m_propHandle;   // handle for fuel tank being measured
};

//----------------------------------------------------------------------------------

class FuelRemainingKGNumberArea : public NumberArea
{
public:
    FuelRemainingKGNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, PROPELLANT_HANDLE ph);

protected:
    virtual bool UpdateRenderData(RENDERDATA &renderData);

    // state data 
    PROPELLANT_HANDLE m_propHandle;   // handle for fuel tank being measured
};
