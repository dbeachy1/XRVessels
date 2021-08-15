/**
  XR Vessel add-ons for OpenOrbiter Space Flight Simulator
  Copyright (C) 2006-2021 Douglas Beachy

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
// XR1Components.h
// Main panel DG-XR1 components.
// ==============================================================

#pragma once

#include "orbitersdk.h"
#include "XR1Component.h"
#include "XR1Areas.h"

class ScramPanelComponent : public XR1Component
{
public:
    ScramPanelComponent(InstrumentPanel &instrumentPanel, COORD2 topLeft, const int meshTextureID = VCPANEL_TEXTURE_NONE);
};

//----------------------------------------------------------------------------------

class MainHoverPanelComponent : public XR1Component
{
public:
    MainHoverPanelComponent(InstrumentPanel &instrumentPanel, COORD2 topLeft, const int meshTextureID = VCPANEL_TEXTURE_NONE);
};

//----------------------------------------------------------------------------------

class DynamicPressurePanelComponent : public XR1Component
{
public:
    DynamicPressurePanelComponent(InstrumentPanel &instrumentPanel, COORD2 topLeft);
};

class ScramTempPanelComponent : public XR1Component
{
public:
    ScramTempPanelComponent(InstrumentPanel &instrumentPanel, COORD2 topLeft);
};

class SlopePanelComponent : public XR1Component
{
public:
    SlopePanelComponent(InstrumentPanel &instrumentPanel, COORD2 topLeft);
};

class AOAPanelComponent : public XR1Component
{
public:
    AOAPanelComponent(InstrumentPanel &instrumentPanel, COORD2 topLeft);
};

class SlipPanelComponent : public XR1Component
{
public:
    SlipPanelComponent(InstrumentPanel &instrumentPanel, COORD2 topLeft);
};

class APUPanelComponent : public XR1Component
{
public:
    APUPanelComponent(InstrumentPanel &instrumentPanel, COORD2 topLeft);
};

class CenterOfGravityPanelComponent : public XR1Component
{
public:
    CenterOfGravityPanelComponent(InstrumentPanel &instrumentPanel, COORD2 topLeft);
};

//==================================================================================

// base worker class; used via multiple inheritence
class FlowRateGauge
{
public:
    FlowRateGauge(VESSEL2 &parentVessel);
    VESSEL2 &GetVessel() const { return m_parentVessel; }

protected:
    VESSEL2 &m_parentVessel;
};

//----------------------------------------------------------------------------------

class ScramFlowGagueArea : public VerticalGaugeArea
{
public:
    ScramFlowGagueArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID = VCPANEL_TEXTURE_NONE);

protected:
    virtual RENDERDATA GetRenderData(const SIDE side);
};

//----------------------------------------------------------------------------------

class ScramTSFCGaugeArea : public VerticalGaugeArea
{
public:
    ScramTSFCGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID = VCPANEL_TEXTURE_NONE);

protected:
    virtual RENDERDATA GetRenderData(const SIDE side);
};

//----------------------------------------------------------------------------------

class MainTSFCGagueArea : public VerticalGaugeArea
{
public:
    MainTSFCGagueArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID = VCPANEL_TEXTURE_NONE);
    void Activate();   // needed because we need a wider area
    virtual void Redraw2DFirstHook(const int event, SURFHANDLE surf);  // invoked after background painted, but before gauge

protected:
    virtual RENDERDATA GetRenderData(const SIDE side);

    // data
    double m_scaleMin, m_scaleMax;
};

//----------------------------------------------------------------------------------

class MainFlowGaugeArea : public VerticalGaugeArea, public FlowRateGauge
{
public:
    MainFlowGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID = VCPANEL_TEXTURE_NONE);

protected:
    virtual RENDERDATA GetRenderData(const SIDE side);
};

//----------------------------------------------------------------------------------

class HoverFlowGaugeArea : public VerticalGaugeArea, public FlowRateGauge
{
public:
    HoverFlowGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID = VCPANEL_TEXTURE_NONE);

protected:
    virtual RENDERDATA GetRenderData(const SIDE side);
};

//----------------------------------------------------------------------------------

class DynamicPressureNumberArea : public NumberArea
{
public:
    DynamicPressureNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual bool UpdateRenderData(RENDERDATA &renderData);
};

//----------------------------------------------------------------------------------

class DynamicPressureGaugeArea : public VerticalGaugeArea
{
public:
    DynamicPressureGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID = VCPANEL_TEXTURE_NONE);

protected:
    virtual RENDERDATA GetRenderData(const SIDE side);
};

//----------------------------------------------------------------------------------

class ScramDiffuserTempHorizontalGaugeArea : public HorizontalGaugeArea
{
public:
    ScramDiffuserTempHorizontalGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID = VCPANEL_TEXTURE_NONE);

protected:
    virtual RENDERDATA GetRenderData(const SIDE side);
};

//----------------------------------------------------------------------------------

class ScramDiffuserTempNumberArea : public NumberArea
{
public:
    ScramDiffuserTempNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual bool UpdateRenderData(RENDERDATA &renderData);
};

//----------------------------------------------------------------------------------

class SlopeNumberArea : public NumberArea
{
public:
    SlopeNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual bool UpdateRenderData(RENDERDATA &renderData);
};

//----------------------------------------------------------------------------------

class SlopeGaugeArea : public VerticalGaugeArea
{
public:
    SlopeGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual RENDERDATA GetRenderData(const SIDE side);
};

//----------------------------------------------------------------------------------

class CenterOfGravityNumberArea : public NumberArea
{
public:
    CenterOfGravityNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual bool UpdateRenderData(RENDERDATA &renderData);
};

//----------------------------------------------------------------------------------

class CenterOfGravityGaugeArea : public VerticalGaugeArea
{
public:
    CenterOfGravityGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual RENDERDATA GetRenderData(const SIDE side);
};

//----------------------------------------------------------------------------------

class CenterOfGravityRockerSwitchArea : public VerticalCenteringRockerSwitchArea
{
public:
    CenterOfGravityRockerSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID = VCPANEL_TEXTURE_NONE);

protected:
    virtual void ProcessSwitchEvent(SWITCHES switches, POSITION position);
};

//----------------------------------------------------------------------------------

class CenterOfGravityAutoButtonArea : public XR1Area
{
public:
    CenterOfGravityAutoButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
};

//----------------------------------------------------------------------------------

class AOANumberArea : public NumberArea
{
public:
    AOANumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual bool UpdateRenderData(RENDERDATA &renderData);
};

//----------------------------------------------------------------------------------

class AOAGaugeArea : public VerticalGaugeArea
{
public:
    AOAGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual RENDERDATA GetRenderData(const SIDE side);
};

//----------------------------------------------------------------------------------

class SlipGaugeArea : public XR1Area
{
public:
    SlipGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID = VCPANEL_TEXTURE_NONE);

    virtual void Activate();
    virtual void Deactivate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);

protected:
    SURFHANDLE m_yellowSurface;
    SURFHANDLE m_lastRenderedSrcSurface;
    int m_lastRenderedIndex;    // last rendered pixel index; -1 = none
};

//----------------------------------------------------------------------------------

class SlipNumberArea : public NumberArea
{
public:
    SlipNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual bool UpdateRenderData(RENDERDATA &renderData);
};

//----------------------------------------------------------------------------------

class APUFuelNumberArea : public NumberArea
{
public:
    APUFuelNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual bool UpdateRenderData(RENDERDATA &renderData);
};

//----------------------------------------------------------------------------------

class APUFuelBarArea : public BarArea
{
public:
    APUFuelBarArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual RENDERDATA GetRenderData();
};

//----------------------------------------------------------------------------------

class APUButton : public XR1Area
{
public:
    APUButton(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    // state data
    enum LightState { UNPRESSED_DARK, UNPRESSED_BRIGHT, PRESSED_DARK, PRESSED_BRIGHT };
    LightState m_lightState;
};
