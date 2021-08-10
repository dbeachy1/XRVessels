// ==============================================================
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// Copyright 2006-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR1LowerPanelAreas.h
// Handles non-component lower panel areas
// ==============================================================

#pragma once

#include "Orbitersdk.h"
#include "vessel3ext.h"
#include "Area.h"
#include "XR1Areas.h"

//----------------------------------------------------------------------------------

class DockReleaseButtonArea : public XR1Area
{
public:
    DockReleaseButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);

protected:
    // state data 
    bool m_buttonPressed;  
};

//----------------------------------------------------------------------------------

class AOAAnalogGaugeArea : public AnalogGaugeArea
{
public:
    AOAAnalogGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID = VCPANEL_TEXTURE_NONE);

protected:
    virtual double GetDialAngle();

};

//----------------------------------------------------------------------------------

class SlipAnalogGaugeArea : public AnalogGaugeArea
{
public:
    SlipAnalogGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID = VCPANEL_TEXTURE_NONE);

protected:
    virtual double GetDialAngle();

};

//----------------------------------------------------------------------------------

class ArtificialHorizonArea : public XR1Area
{
public:
    ArtificialHorizonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID = VCPANEL_TEXTURE_NONE);
    virtual void Activate();
    virtual void Deactivate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);

protected:
    // additional resources
    HBRUSH m_brush2, m_brush3;
    HPEN m_pen0;
    DWORD m_color2, m_color3;
};

//----------------------------------------------------------------------------------

class XFeedKnobArea : public XR1Area
{
public:
    XFeedKnobArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);
};

//----------------------------------------------------------------------------------

class SystemsDisplayScreen : public XR1Area
{
public:
    SystemsDisplayScreen(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);
    virtual ~SystemsDisplayScreen();
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);

protected:
    HFONT m_mainFont;
    int m_lineSpacing;  // pixels between text lines
    bool m_forceRender;
    TextBox *m_pTextBox;
    const int m_width, m_height;
};

//----------------------------------------------------------------------------------

class ExtSupplyLineToggleSwitchArea : public ToggleSwitchArea
{
public:
    ExtSupplyLineToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID, bool &switchState, const bool &pressureNominal);

protected:
    virtual bool ProcessSwitchEvent(bool switchIsOn);
    virtual bool isOn();

    bool &m_switchState;     // reference to an XR1's m_fooFuelFlowSwitch variable
    const bool &pressureNominal; // true if line is ready to flow
};

//----------------------------------------------------------------------------------

#ifdef TURBOPACKS
class TurbopackDisplayArea : public XR1Area
{
public:
    TurbopackDisplayArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

    virtual void Activate();
    virtual void Deactivate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);

protected:
    HFONT m_font;
    COORD2 m_deployButtonCoord;
    COORD2 m_stowAllButtonCoord;
    COORD2 m_prevArrowCoord;
    COORD2 m_nextArrowCoord;
};
#endif
