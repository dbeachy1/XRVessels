/**
  XR Vessel add-ons for OpenOrbiter Space Flight Simulator
  Copyright (C) 2006-2025 Douglas Beachy

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.

  Email: mailto:doug.beachy@outlook.com
  Web: https://www.alteaaerospace.com
**/

// ==============================================================
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
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

