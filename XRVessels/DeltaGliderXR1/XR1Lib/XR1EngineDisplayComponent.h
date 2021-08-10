// ==============================================================
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// Copyright 2006-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR1EngineDisplayComponent.h
// Handles main, hover, and scram throttle controls
// ==============================================================

#pragma once

#include "orbitersdk.h"
#include "XR1Component.h"
#include "XR1Areas.h"

class EngineDisplayComponent : public XR1Component
{
public:
    EngineDisplayComponent(InstrumentPanel &instrumentPanel, COORD2 topLeft);
};

//----------------------------------------------------------------------------------

class EngineEfficiencyGaugeArea : public PctHorizontalGaugeArea
{
public:
    EngineEfficiencyGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual double GetFraction(const SIDE side, COLOR &color);
};

//----------------------------------------------------------------------------------

class NormalThrustBarArea : public BarArea
{
public:
    NormalThrustBarArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, THRUSTER_HANDLE th);

protected:
    virtual RENDERDATA GetRenderData();

    // state data 
    THRUSTER_HANDLE m_thrusterHandle;   // handle of thruster being measured
};

//----------------------------------------------------------------------------------

class MainRetroThrustBarArea : public BarArea
{
public:
    MainRetroThrustBarArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual RENDERDATA GetRenderData();
};

//----------------------------------------------------------------------------------

class MainRetroThrustNumberArea : public ThrustNumberArea
{
public:
    MainRetroThrustNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual double GetThrust();  // thrust in kN
};

//----------------------------------------------------------------------------------

class HoverThrustNumberArea : public ThrustNumberArea
{
public:
    HoverThrustNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual double GetThrust();  // thrust in kN
};


//----------------------------------------------------------------------------------

class ScramThrustNumberArea : public ThrustNumberArea
{
public:
    ScramThrustNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual double GetThrust();  // thrust in kN
};
