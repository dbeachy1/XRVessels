/**
  XR Vessel add-ons for OpenOrbiter Space Flight Simulator
  Copyright (C) 2006-2021 Douglas Beachy

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

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
// XR1UpperPanelAreas.h
// Handles upper panel areas
// ==============================================================

#pragma once

#include "Orbitersdk.h"
#include "vessel3ext.h"
#include "Area.h"
#include "XR1Areas.h"

//----------------------------------------------------------------------------------

class NavLightToggleSwitchArea : public ToggleSwitchArea
{
public:
    NavLightToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID);

protected:
    virtual bool ProcessSwitchEvent(bool switchIsOn);
    virtual bool isOn();
};

//----------------------------------------------------------------------------------

class BeaconLightToggleSwitchArea : public ToggleSwitchArea
{
public:
    BeaconLightToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID);

protected:
    virtual bool ProcessSwitchEvent(bool switchIsOn);
    virtual bool isOn();
};

//----------------------------------------------------------------------------------

class StrobeLightToggleSwitchArea : public ToggleSwitchArea
{
public:
    StrobeLightToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID);

protected:
    virtual bool ProcessSwitchEvent(bool switchIsOn);
    virtual bool isOn();
};

//----------------------------------------------------------------------------------

class LadderToggleSwitchArea : public ToggleSwitchArea
{
public:
    LadderToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID);

protected:
    virtual bool isOn();
    virtual bool ProcessSwitchEvent(bool switchIsOn);
};

//----------------------------------------------------------------------------------

class NoseConeToggleSwitchArea : public ToggleSwitchArea
{
public:
    NoseConeToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID);

protected:
    virtual bool isOn();
    virtual bool ProcessSwitchEvent(bool switchIsOn);
};



//----------------------------------------------------------------------------------

class OuterDoorToggleSwitchArea : public ToggleSwitchArea
{
public:
    OuterDoorToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID);

protected:
    virtual bool isOn();
    virtual bool ProcessSwitchEvent(bool switchIsOn);
};

//----------------------------------------------------------------------------------

class InnerDoorToggleSwitchArea : public ToggleSwitchArea
{
public:
    InnerDoorToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID);

protected:
    virtual bool isOn();
    virtual bool ProcessSwitchEvent(bool switchIsOn);
};

//----------------------------------------------------------------------------------

class ChamberToggleSwitchArea : public ToggleSwitchArea
{
public:
    ChamberToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID);

protected:
    virtual bool isOn();
    virtual bool ProcessSwitchEvent(bool switchIsOn);
};

//----------------------------------------------------------------------------------

class HatchToggleSwitchArea : public ToggleSwitchArea
{
public:
    HatchToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID);

protected:
    virtual bool isOn();
    virtual bool ProcessSwitchEvent(bool switchIsOn);
};

//----------------------------------------------------------------------------------

class RadiatorToggleSwitchArea : public ToggleSwitchArea
{
public:
    RadiatorToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID);

protected:
    virtual bool isOn();
    virtual bool ProcessSwitchEvent(bool switchIsOn);
};

//----------------------------------------------------------------------------------

class RetroDoorToggleSwitchArea : public ToggleSwitchArea
{
public:
    RetroDoorToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID);

protected:
    virtual bool isOn();
    virtual bool ProcessSwitchEvent(bool switchIsOn);
};

//----------------------------------------------------------------------------------

class GearToggleSwitchArea : public ToggleSwitchArea
{
public:
    GearToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID);

protected:
    virtual bool isOn();
    virtual bool ProcessSwitchEvent(bool switchIsOn);
};

//----------------------------------------------------------------------------------

class AirbrakeToggleSwitchArea : public ToggleSwitchArea
{
public:
    AirbrakeToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID);

protected:
    virtual bool isOn();
    virtual bool ProcessSwitchEvent(bool switchIsOn);
};

//----------------------------------------------------------------------------------

class ScramTempGaugeArea : public XR1Area
{
public:
    ScramTempGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID = VCPANEL_TEXTURE_NONE);
    virtual ~ScramTempGaugeArea();
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);

protected:
    // data
    HPEN m_pen0;
    HPEN m_pen1;
};

//----------------------------------------------------------------------------------

class OverrideOuterAirlockToggleButtonArea : public RawButtonArea
{
public:
    OverrideOuterAirlockToggleButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual bool IsLit() { return GetXR1().m_airlockInterlocksDisabled; }
    virtual void ProcessButtonAction(int event, double buttonDownSimt);
};

//----------------------------------------------------------------------------------

class OverrideCrewHatchToggleButtonArea : public RawButtonArea
{
public:
    OverrideCrewHatchToggleButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual bool IsLit() { return GetXR1().m_crewHatchInterlocksDisabled; }
    virtual void ProcessButtonAction(int event, double buttonDownSimt);
};

//----------------------------------------------------------------------------------

class HoverDoorToggleSwitchArea : public ToggleSwitchArea
{
public:
    HoverDoorToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID);

protected:
    virtual bool isOn();
    virtual bool ProcessSwitchEvent(bool switchIsOn);
};

//----------------------------------------------------------------------------------

class ScramDoorToggleSwitchArea : public ToggleSwitchArea
{
public:
    ScramDoorToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID);

protected:
    virtual bool isOn();
    virtual bool ProcessSwitchEvent(bool switchIsOn);
};

//----------------------------------------------------------------------------------

class CrewDisplayArea : public XR1Area
{
public:
    CrewDisplayArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

    virtual void Activate();
    virtual void Deactivate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);

protected:
    HFONT m_font;
    HFONT m_numberFont;
    COORD2 m_evaButtonCoord;
    COORD2 m_prevArrowCoord;
    COORD2 m_nextArrowCoord;
    int m_crewMemberIndexX;
};

// NOTE: areas below here are used by subclasses only
//----------------------------------------------------------------------------------

class SwitchToPanelButtonArea : public MomentaryButtonArea
{
public:
    SwitchToPanelButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int targetPanelID);

protected:
    virtual void ProcessButtonAction(int event, double buttonDownSimt);

    int m_targetPanelID;
};

//----------------------------------------------------------------------------------

class BayDoorsToggleSwitchArea : public ToggleSwitchArea
{
public:
    BayDoorsToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID);

protected:
    virtual bool isOn();
    virtual bool ProcessSwitchEvent(bool switchIsOn);
};

