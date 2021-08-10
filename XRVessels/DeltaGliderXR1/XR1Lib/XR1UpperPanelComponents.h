// ==============================================================
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// Copyright 2006-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR1UpperPanelComponents.h
// Handles upper panel components and associated areas.
// ==============================================================

#pragma once

#include "Orbitersdk.h"
#include "XR1Component.h"
#include "XR1Areas.h"

class METTimerComponent : public XR1Component
{
public:
    METTimerComponent(InstrumentPanel &instrumentPanel, COORD2 topLeft);
};

class Interval1TimerComponent : public XR1Component
{
public:
    Interval1TimerComponent(InstrumentPanel &parentPanel, COORD2 topLeft);
};

class CrewDisplayComponent : public XR1Component
{
public:
    CrewDisplayComponent(InstrumentPanel &parentPanel, COORD2 topLeft);
};

//----------------------------------------------------------------------------------

class METResetButtonArea : public MomentaryButtonArea
{
public:
    METResetButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual void ProcessButtonAction(int event, double buttonDownSimt);
    bool m_buttonPressProcessed;    // true if button press already processed; ignore remaining events
};

class IntervalResetButtonArea : public MomentaryButtonArea
{
public:
    IntervalResetButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, bool &intervalTimerRunning, double &intervalStartingTime, const char timerNumberChar);

protected:
    virtual void ProcessButtonAction(int event, double buttonDownSimt);
    bool m_buttonPressProcessed;    // true if button press already processed; ignore remaining events
    bool &m_intervalTimerRunning;
    double &m_intervalStartingTime;
    char m_timerNumberChar;
    bool m_disableTimerStartForThisClick;
};

