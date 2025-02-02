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
// XR1LowerPanelComponents.h
// Handles lower panel components and associated areas.
// ==============================================================

#pragma once

#include "Orbitersdk.h"
#include "XR1Component.h"
#include "XR1Areas.h"

class Interval2TimerComponent : public XR1Component
{
public:
    Interval2TimerComponent(InstrumentPanel &parentPanel, COORD2 topLeft);
};

//----------------------------------------------------------------------------------

class MainFuelGaugeComponent : public XR1Component
{
public:
    MainFuelGaugeComponent(InstrumentPanel &parentPanel, COORD2 topLeft);
};

//----------------------------------------------------------------------------------

class LoxGaugeComponent : public XR1Component
{
public:
    LoxGaugeComponent(InstrumentPanel &parentPanel, COORD2 topLeft);
};


//----------------------------------------------------------------------------------

class RCSFuelGaugeComponent : public XR1Component
{
public:
    RCSFuelGaugeComponent(InstrumentPanel &parentPanel, COORD2 topLeft);
};

//----------------------------------------------------------------------------------

class SCRAMFuelGaugeComponent : public XR1Component
{
public:
    SCRAMFuelGaugeComponent(InstrumentPanel &parentPanel, COORD2 topLeft);
};

//----------------------------------------------------------------------------------

class APUFuelGaugeComponent : public XR1Component
{
public:
    APUFuelGaugeComponent(InstrumentPanel &parentPanel, COORD2 topLeft);
};

//----------------------------------------------------------------------------------

class FuelHatchComponent : public XR1Component
{
public:
    FuelHatchComponent(InstrumentPanel &parentPanel, COORD2 topLeft);
};

//----------------------------------------------------------------------------------

class LoxHatchComponent : public XR1Component
{
public:
    LoxHatchComponent(InstrumentPanel &parentPanel, COORD2 topLeft);
};

//----------------------------------------------------------------------------------

class MainSupplyLineGaugeComponent : public XR1Component
{
public:
    MainSupplyLineGaugeComponent(InstrumentPanel &parentPanel, COORD2 topLeft);
};

class ScramSupplyLineGaugeComponent : public XR1Component
{
public:
    ScramSupplyLineGaugeComponent(InstrumentPanel &parentPanel, COORD2 topLeft);
};

class ApuSupplyLineGaugeComponent : public XR1Component
{
public:
    ApuSupplyLineGaugeComponent(InstrumentPanel &parentPanel, COORD2 topLeft);
};

class LoxSupplyLineGaugeComponent : public XR1Component
{
public:
    LoxSupplyLineGaugeComponent(InstrumentPanel &parentPanel, COORD2 topLeft);
};

//----------------------------------------------------------------------------------

class ShipMassDisplayComponent : public XR1Component
{
public:
    ShipMassDisplayComponent(InstrumentPanel &parentPanel, COORD2 topLeft);
};

//----------------------------------------------------------------------------------

class OxygenRemainingPanelComponent : public XR1Component
{
public:
    OxygenRemainingPanelComponent(InstrumentPanel &parentPanel, COORD2 topLeft);
};

//----------------------------------------------------------------------------------

class CoolantGaugeComponent : public XR1Component
{
public:
    CoolantGaugeComponent(InstrumentPanel &parentPanel, COORD2 topLeft);
};


//----------------------------------------------------------------------------------
// AREAS
//----------------------------------------------------------------------------------

class SupplyLinePressureNumberArea : public NumberArea
{
public:
    SupplyLinePressureNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, double limit, double &pressure);

protected:
    virtual bool UpdateRenderData(RENDERDATA &renderData);
    double m_limit;
    double &m_pressure;
};

//----------------------------------------------------------------------------------

class SupplyLinePressureGaugeArea : public VerticalGaugeArea
{
public:
    SupplyLinePressureGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, double limit, double &pressure);

protected:
    virtual RENDERDATA GetRenderData(const SIDE side);
    double m_limit;
    double &m_pressure;
};

//----------------------------------------------------------------------------------

class SupplyLineMediumLEDArea : public XR1Area
{
public:
    SupplyLineMediumLEDArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, bool &lightStatus);
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);

protected:
    bool &m_lightStatus;
    bool m_lastRenderedState;   // true of light on, false if light off
};

//----------------------------------------------------------------------------------

class LoxNumberArea : public NumberArea
{
public:
    LoxNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual bool UpdateRenderData(RENDERDATA &renderData);
};

//----------------------------------------------------------------------------------

class LoxDumpButtonArea : public XR1Area
{
public:
    LoxDumpButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

    virtual void ProcessButtonPressed(const int event);

protected:
    bool m_isLit;
    bool m_buttonPressProcessed;
    double m_buttonDownSimt;
    int m_isButtonDown;
};

//----------------------------------------------------------------------------------

class OxygenRemainingPctNumberArea : public NumberArea
{
public:
    OxygenRemainingPctNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual bool UpdateRenderData(RENDERDATA &renderData);
};

//----------------------------------------------------------------------------------

class OxygenRemainingTimerNumberArea : public TimerNumberArea
{
public:
    OxygenRemainingTimerNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int sizeInChars, TIMEUNITS timeUnits);

protected:
    virtual double GetTime();   // returns time in DAYS
};

//----------------------------------------------------------------------------------

class CrewMembersNumberArea : public NumberArea
{
public:
    CrewMembersNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual bool UpdateRenderData(RENDERDATA &renderData);
};

//----------------------------------------------------------------------------------

class CabinO2PctNumberArea : public NumberArea
{
public:
    CabinO2PctNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual bool UpdateRenderData(RENDERDATA &renderData);
};

//----------------------------------------------------------------------------------

class CoolantNumberArea : public NumberArea
{
public:
    CoolantNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual bool UpdateRenderData(RENDERDATA &renderData);
};

//----------------------------------------------------------------------------------

class ExternalCoolingComponent : public XR1Component
{
public:
    ExternalCoolingComponent(InstrumentPanel &parentPanel, COORD2 topLeft);
};
