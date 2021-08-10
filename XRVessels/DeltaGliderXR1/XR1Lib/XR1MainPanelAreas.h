// ==============================================================
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// Copyright 2006-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR1MainPanelAreas.h
// Handles non-component main panel areas
// ==============================================================

#pragma once

#include "Orbitersdk.h"
#include "vessel3ext.h"
#include "Area.h"
#include "XR1Areas.h"

//----------------------------------------------------------------------------------

class HudModeButtonsArea : public XR1Area
{
public:
    HudModeButtonsArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);
    // no VC handler for this area
};

//----------------------------------------------------------------------------------

class ElevatorTrimArea : public XR1Area
{
public:
    ElevatorTrimArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID = VCPANEL_TEXTURE_NONE);
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool Redraw3D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);
    virtual bool ProcessVCMouseEvent(const int event, const VECTOR3 &coords);

protected:
    // state data 
    int m_elevTrimPos;  
};

//----------------------------------------------------------------------------------

class WingLoadAnalogGaugeArea : public AnalogGaugeArea
{
public:
    WingLoadAnalogGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID = VCPANEL_TEXTURE_NONE);

protected:
    virtual double GetDialAngle();

};

//----------------------------------------------------------------------------------

class AutopilotButtonsArea : public XR1Area
{
public:
    AutopilotButtonsArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);
};

//----------------------------------------------------------------------------------

class MWSArea : public XR1Area
{
public:
    MWSArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool Redraw3D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);

protected:
    bool m_MWSLit;  // TRUE if light is lit
};

//----------------------------------------------------------------------------------

class RCSModeArea : public XR1Area
{
public:
    RCSModeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const WORD resourceID = IDB_DIAL1);
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool Redraw3D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);

protected:
    WORD m_resourceID;
};

//----------------------------------------------------------------------------------

class AFCtrlArea : public XR1Area
{
public:
    AFCtrlArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool Redraw3D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);
};

//----------------------------------------------------------------------------------

class AutopilotLEDArea : public XR1Area
{
public:
    AutopilotLEDArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    DWORD m_color;
    bool m_isOn;    // is light on?
    bool m_enabled; // is any autopilot except KILLROT on?
};

//----------------------------------------------------------------------------------

class StaticPressureNumberArea : public NumberArea
{
public:
    StaticPressureNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual bool UpdateRenderData(RENDERDATA &renderData);
};

//----------------------------------------------------------------------------------

class MWSTestButtonArea : public MomentaryButtonArea
{
public:
    MWSTestButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual void ProcessButtonAction(int event, double buttonDownSimt);
};

//----------------------------------------------------------------------------------

class WarningLightsArea : public XR1Area
{
public:
    WarningLightsArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);

protected:
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

    // state data
    bool m_lightStateOn;  // true if light state (during blink) is ON
};

//----------------------------------------------------------------------------------

class DeployRadiatorButtonArea : public XR1Area
{
public:
    DeployRadiatorButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);

protected:
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

    // state data
    bool m_lastRenderedLightState;  
    bool m_lightState;  // true if light state is ON; set by PostStep
};

//----------------------------------------------------------------------------------

class DataHUDButtonArea : public MomentaryButtonArea
{
public:
    DataHUDButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual void ProcessButtonAction(int event, double buttonDownSimt);
    virtual bool IsLit();
};

