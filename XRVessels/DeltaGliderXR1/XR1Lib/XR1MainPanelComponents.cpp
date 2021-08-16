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
// XR1MainPanelComponents.cpp
// DG-XR1 components on the main panel
// ==============================================================

#include "orbitersdk.h"
#include "resource.h"
#include "AreaIDs.h"
#include "XR1InstrumentPanels.h"
#include "XR1Areas.h"
#include "XR1MainPanelComponents.h"

// Constructor
// parentPanel = parent instrument panel
// topLeft = top inside edge of frame, just on black screen
ScramPanelComponent::ScramPanelComponent(InstrumentPanel &parentPanel, COORD2 topLeft, const int meshTextureID) :
    XR1Component(parentPanel, topLeft, meshTextureID)
{
    AddArea(new ScramFlowGagueArea(parentPanel, GetAbsCoords(_COORD2(55, 14)), AID_SCRAMFLOW, meshTextureID));
    AddArea(new ScramTSFCGaugeArea(parentPanel, GetAbsCoords(_COORD2(18, 14)), AID_SCRAMTSFC, meshTextureID));
}   

//----------------------------------------------------------------------------------

// Constructor
// topLeft = top inside edge of frame, just on black screen
MainHoverPanelComponent::MainHoverPanelComponent(InstrumentPanel &parentPanel, COORD2 topLeft, const int meshTextureID) :
    XR1Component(parentPanel, topLeft, meshTextureID)
{
    AddArea(new MainTSFCGagueArea (parentPanel, GetAbsCoords(_COORD2(14, 17)), AID_MAINTSFC, meshTextureID));
    AddArea(new MainFlowGaugeArea (parentPanel, GetAbsCoords(_COORD2(42, 17)), AID_MAINFLOW, meshTextureID));
    AddArea(new HoverFlowGaugeArea(parentPanel, GetAbsCoords(_COORD2(66, 17)), AID_HOVERFLOW, meshTextureID));
}   

//----------------------------------------------------------------------------------

// Constructor
// topLeft = top inside edge of frame, just on black screen
DynamicPressurePanelComponent::DynamicPressurePanelComponent(InstrumentPanel &parentPanel, COORD2 topLeft) :
    XR1Component(parentPanel, topLeft)
{
    AddArea(new DynamicPressureNumberArea(parentPanel, GetAbsCoords(_COORD2(-1,  4)), AID_DYNPRESSURE_KPA));
    AddArea(new DynamicPressureGaugeArea (parentPanel, GetAbsCoords(_COORD2(21, 17)), AID_DYNPRESSURE_GAUGE));
}   

//----------------------------------------------------------------------------------

// Constructor
// parentPanel = parent instrument panel
// topLeft = top inside edge of frame, just on black screen
ScramTempPanelComponent::ScramTempPanelComponent(InstrumentPanel &parentPanel, COORD2 topLeft) :
    XR1Component(parentPanel, topLeft)
{
    AddArea(new ScramDiffuserTempHorizontalGaugeArea (parentPanel, GetAbsCoords(_COORD2( 0, 13)), AID_SCRAMTEMP_LBAR));
    AddArea(new ScramDiffuserTempHorizontalGaugeArea (parentPanel, GetAbsCoords(_COORD2( 0, 20)), AID_SCRAMTEMP_RBAR));
    AddArea(new ScramDiffuserTempNumberArea          (parentPanel, GetAbsCoords(_COORD2(91,  8)), AID_SCRAMTEMP_LTEXT));
    AddArea(new ScramDiffuserTempNumberArea          (parentPanel, GetAbsCoords(_COORD2(91, 20)), AID_SCRAMTEMP_RTEXT));
}   

//----------------------------------------------------------------------------------

// Constructor
// topLeft = top inside edge of frame, just on black screen
SlopePanelComponent::SlopePanelComponent(InstrumentPanel &parentPanel, COORD2 topLeft) :
    XR1Component(parentPanel, topLeft)
{
    AddArea(new SlopeNumberArea(parentPanel, GetAbsCoords(_COORD2(-1,  3)), AID_SLOPE_DEGREES));
    AddArea(new SlopeGaugeArea (parentPanel, GetAbsCoords(_COORD2(22, 17)), AID_SLOPE_GAUGE));
}   

//----------------------------------------------------------------------------------

// Constructor
// topLeft = top inside edge of frame, just on black screen
CenterOfGravityPanelComponent::CenterOfGravityPanelComponent(InstrumentPanel &parentPanel, COORD2 topLeft) :
    XR1Component(parentPanel, topLeft)
{
    AddArea(new CenterOfGravityNumberArea(parentPanel, GetAbsCoords(_COORD2( 0,  3)), AID_COG_NUMBER));
    AddArea(new CenterOfGravityGaugeArea (parentPanel, GetAbsCoords(_COORD2(27, 15)), AID_COG_GAUGE));

    AddArea(new CenterOfGravityRockerSwitchArea     (parentPanel, GetAbsCoords(_COORD2(58, 23)), AID_COG_ROCKER_SWITCH)); 
    AddArea(new CenterOfGravityAutoButtonArea       (parentPanel, GetAbsCoords(_COORD2(58,  4)), AID_COG_AUTO_LED));
    AddArea(new SimpleButtonArea                    (parentPanel, GetAbsCoords(_COORD2(60, 82)), AID_COG_CENTER_BUTTON, &GetXR1().m_cogShiftCenterModeActive));
}   

//----------------------------------------------------------------------------------

// Constructor
// topLeft = top inside edge of frame, just on black screen
AOAPanelComponent::AOAPanelComponent(InstrumentPanel &parentPanel, COORD2 topLeft) :
    XR1Component(parentPanel, topLeft)
{
    AddArea(new AOANumberArea(parentPanel, GetAbsCoords(_COORD2( 0,  3)), AID_AOA_DEGREES));
    AddArea(new AOAGaugeArea (parentPanel, GetAbsCoords(_COORD2(22, 17)), AID_AOA_GAUGE));
}   

//----------------------------------------------------------------------------------

// Constructor
// topLeft = top inside edge of frame, just on black screen
SlipPanelComponent::SlipPanelComponent(InstrumentPanel &parentPanel, COORD2 topLeft) :
    XR1Component(parentPanel, topLeft)
{
    AddArea(new SlipGaugeArea (parentPanel, GetAbsCoords(_COORD2( 2, 14)), AID_SLIP_GAUGE));
    AddArea(new SlipNumberArea(parentPanel, GetAbsCoords(_COORD2(95, 14)), AID_SLIP_TEXT));
}   

//----------------------------------------------------------------------------------

// Constructor
// topLeft = top inside edge of frame, just on black screen
APUPanelComponent::APUPanelComponent(InstrumentPanel &parentPanel, COORD2 topLeft) :
    XR1Component(parentPanel, topLeft)
{
    AddArea(new APUFuelNumberArea(parentPanel, GetAbsCoords(_COORD2( 2,  2)), AID_APU_FUEL_TEXT));
    AddArea(new APUFuelBarArea   (parentPanel, GetAbsCoords(_COORD2( 1, 15)), AID_APU_FUEL_GAUGE));
    AddArea(new APUButton        (parentPanel, GetAbsCoords(_COORD2(-3, 66)), AID_APU_BUTTON));
}   

//----------------------------------------------------------------------------------
// Begin Areas
//----------------------------------------------------------------------------------

// base class
FlowRateGauge::FlowRateGauge(VESSEL2 &parentVessel) :
    m_parentVessel(parentVessel)
{
}


//----------------------------------------------------------------------------------

ScramFlowGagueArea::ScramFlowGagueArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID) :
    VerticalGaugeArea(parentPanel, panelCoordinates, areaID, true, 73, PANEL_REDRAW_ALWAYS, meshTextureID)  // dual gauge 73 pixels high
{
}

// side: 0=left, 1=right
VerticalGaugeArea::RENDERDATA ScramFlowGagueArea::GetRenderData(const SIDE side)
{
    // show ACTUAL fuel flow here vs. EFFECTIVE flow
    const double actualDMF = GetXR1().ramjet->DMF(static_cast<unsigned int>(side)) / GetXR1().GetXR1Config()->GetScramISPMultiplier();
    int p = 66 - min(66, static_cast<int>(actualDMF / SCRAM_FLOW_GAUGE_MAX * 67.0));

    return _RENDERDATA(COLOR::GREEN, p);
}

//----------------------------------------------------------------------------------

ScramTSFCGaugeArea::ScramTSFCGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID) :
    VerticalGaugeArea(parentPanel, panelCoordinates, areaID, true, 73, PANEL_REDRAW_ALWAYS, meshTextureID)  // dual gauge 73 pixels high
{
}

VerticalGaugeArea::RENDERDATA ScramTSFCGaugeArea::GetRenderData(const SIDE side)
{
    // NOTE: must use UNSIGNED int here because TSFC can become very large, tripping the MSB
    int p = 66 - min(66, static_cast<unsigned int>(GetXR1().ramjet->TSFC(static_cast<unsigned int>(side))*(1e3 * 66.0 / SCRAM_TSFC_GAUGE_MAX)));

    // show in yellow if off-scale 
    COLOR color = ((p == 0) ? COLOR::YELLOW : COLOR::GREEN);

    return _RENDERDATA(color, p);  
}

//----------------------------------------------------------------------------------

MainTSFCGagueArea::MainTSFCGagueArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID) :
    VerticalGaugeArea(parentPanel, panelCoordinates, areaID, false, 73, PANEL_REDRAW_ALWAYS, meshTextureID, 4, 0),  // single gauge 73 pixels high, bump indicator right 4 pixels
    m_scaleMin(0.018), m_scaleMax(0.066)   // set gauge range
{
}

VerticalGaugeArea::RENDERDATA MainTSFCGagueArea::GetRenderData(const SIDE side)
{
    const double tsfc = 1000 / GetVessel().GetThrusterIsp(GetXR1().th_main[0]);

    int p = 66 - static_cast<int>((tsfc-m_scaleMin) / (m_scaleMax-m_scaleMin) * 67.0);

    // assume in range
    COLOR color = COLOR::GREEN;
    
    // keep in range
    if (p < 0)
    {
        p = 0;
        color = COLOR::YELLOW; // off gauge
    }
    else if (p > 66)
    {
        p = 66;
        color = COLOR::YELLOW; // off gauge
    }

    return _RENDERDATA(color, p);  
}

// must override this method because we need more width for the max eff. bar
void MainTSFCGagueArea::Activate()
{
    Area::Activate();  // invoke superclass method
    int sizeX = (m_isDual ? 13 : 6);    // need 13 pixels because of single-pixel separator between indicators
    if (IsVC()) // 3D panel?
    {
        oapiVCRegisterArea(GetAreaID(), GetRectForSize(sizeX + m_deltaX + 2, m_sizeY + m_deltaY), m_redrawFlag, PANEL_MOUSE_IGNORE, PANEL_MAP_BGONREQUEST, GetVCPanelTextureHandle());
    }
    else    // 2D panel
    {
        oapiRegisterPanelArea(GetAreaID(), GetRectForSize(sizeX + m_deltaX + 2, m_sizeY + m_deltaY), m_redrawFlag, PANEL_MOUSE_IGNORE, PANEL_MAP_BGONREQUEST);
    }

    m_mainSurface = CreateSurface(IDB_INDICATOR2);                    // green indicator arrows
    m_yellowIndicatorSurface = CreateSurface(IDB_YELLOW_INDICATOR2);  // yellow indicator arrows
    m_redIndicatorSurface = CreateSurface(IDB_RED_INDICATOR2);        // red indicator arrows
    
    DWORD white = 0xFFFFFF;           // set WHITE as transparent color; BLACK does not work for some reason!
    SetSurfaceColorKey(m_mainSurface, white);
    SetSurfaceColorKey(m_yellowIndicatorSurface, white);
    SetSurfaceColorKey(m_redIndicatorSurface, white);

    // reset state variables to force a repaint
    m_lastRenderData[0].Reset();
    m_lastRenderData[1].Reset();
}

// invoked after background painted, but before gauge
void MainTSFCGagueArea::Redraw2DFirstHook(const int event, SURFHANDLE surf)
{
    double tsfc = 1000 / GetVessel().GetThrusterIsp(GetXR1().th_main[0]);

    // draw a white bar @ max efficiency; we want to draw this FIRST so it is underneath the arrow we draw below
    // get tsfc in a vacuum
    double vacuumTSFC = 1000 / GetVessel().GetThrusterIsp0(GetXR1().th_main[0]);

    // do not adjust scale here
    int pMaxEfficiency = static_cast<int>((vacuumTSFC - m_scaleMin) / (m_scaleMax - m_scaleMin) * 67.0);

    // if off-scale, do not render the max efficiency bar!
    if ((pMaxEfficiency >= 0) && (pMaxEfficiency <= 66))
    {
        // get bright white color
        // NOTE: cannot use 255,255,255 here: that is the transparent color
        DWORD fillColor = BRIGHT_WHITE;

        // now render the white bar; must add 3 pixels here because the arrow is seven pixels high and we are one pixel high
        // NOTE: paintable area starts 3 pixes ABOVE bar so we have to adjust for that here by using 69 instead of 66
        oapiColourFill(surf, fillColor, 1, 69-pMaxEfficiency, 11, 1); // eleven pixels wide, one pixel high
    }
}

//----------------------------------------------------------------------------------

MainFlowGaugeArea::MainFlowGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID) :
    VerticalGaugeArea(parentPanel, panelCoordinates, areaID, false, 73, PANEL_REDRAW_ALWAYS, meshTextureID),  // single gauge 73 pixels high
    FlowRateGauge(parentPanel.GetVessel())
{
}

// side will always be LEFT for a single gauge
VerticalGaugeArea::RENDERDATA MainFlowGaugeArea::GetRenderData(const SIDE side)
{
    double totalFlowRate = 0;   // total for both main thrusters
    for (int i = 0; i < 2; i++)
        totalFlowRate += GetXR1().GetThrusterFlowRate(GetXR1().th_main[i]);  

    bool isRetro = false;     // assume main flow
    if (totalFlowRate == 0)   // no main flow?
    {
        // let's check for retro thrust
        for (int i = 0; i < 2; i++)
            totalFlowRate += GetXR1().GetThrusterFlowRate(GetXR1().th_retro[i]);

        if (totalFlowRate > 0)
            isRetro = true;
    }

    int p = 66 - static_cast<int>(min(totalFlowRate * 66 / MAIN_FLOW_GAUGE_MAX, 66));

    // use a green indicator for main engines or a red indicator for retro engines
    return _RENDERDATA(isRetro ? COLOR::RED : COLOR::GREEN, p);
}

//----------------------------------------------------------------------------------

HoverFlowGaugeArea::HoverFlowGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID) :
    VerticalGaugeArea(parentPanel, panelCoordinates, areaID, false, 73, PANEL_REDRAW_ALWAYS, meshTextureID),  // single gauge 73 pixels high
    FlowRateGauge(parentPanel.GetVessel())
{
}

// side will always be LEFT for a single gauge
VerticalGaugeArea::RENDERDATA HoverFlowGaugeArea::GetRenderData(const SIDE side)
{
    double gaugeSize = 66.99;  // pointer can move 66 pixels; also round up to next pixel
    double totalFlowRate = 0;
    for (int i = 0; i < 2; i++) // count flow in both hover thrusters
        totalFlowRate += GetXR1().GetThrusterFlowRate(GetXR1().th_hover[i]);

    int p = 66 - static_cast<int>(min(totalFlowRate * gaugeSize / HOVER_FLOW_GAUGE_MAX, gaugeSize));

    return _RENDERDATA(COLOR::GREEN, p);
}


//-------------------------------------------------------------------------

DynamicPressureNumberArea::DynamicPressureNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    NumberArea(parentPanel, panelCoordinates, areaID, 4, true)   // 4 chars plus decimal
{
}

bool DynamicPressureNumberArea::UpdateRenderData(RENDERDATA &renderData)
{
    bool redraw = false;
    const bool forceRedraw = renderData.forceRedraw;

    double dynamicPressure = GetVessel().GetDynPressure() / 1000;  // convert to kPa

     // round to nearest 1/10th
    dynamicPressure = (static_cast<double>(static_cast<int>((dynamicPressure + 0.05) * 10))) / 10;
    
    // check whether the value has changed since the last render
    if (forceRedraw || (dynamicPressure != renderData.value))
    {
        // Value has changed -- let's redo the string and see if that is different as well.
        // The goal here is to be as efficient as possible and only re-render when we absolutely have to.
        char pTemp[8];
        // ensure that value is in range
        if (dynamicPressure > 999.9)
            dynamicPressure = 999.9;   // trim to 3 leading digits
        else if (dynamicPressure < 0)   // dynp cannot be < 0
            dynamicPressure = 0;

        sprintf(pTemp, "%5.1f", dynamicPressure);   // 4 chars + the decimal = length 5
        if (forceRedraw || (strcmp(pTemp, renderData.pStrToRender) != 0))
        {
            // text has changed; signal the base class to render it
            renderData.value = dynamicPressure;   // remember for next time
            strcpy(renderData.pStrToRender, pTemp);
            redraw = true;
            renderData.forceRedraw = false;  // clear reset request
        }
    }

    // set font color based on pressure levels
    const double maxDynpKpa = DYNP_MAX / 1000;   // in kPa
    if (dynamicPressure >= maxDynpKpa)
        renderData.color = COLOR::WHITE;
    else if (dynamicPressure >= (maxDynpKpa * 0.93333))
        renderData.color = COLOR::RED;
    else if (dynamicPressure >= (maxDynpKpa * 0.80))
        renderData.color = COLOR::YELLOW;
    else
        renderData.color = COLOR::GREEN;

    return redraw;
}

//----------------------------------------------------------------------------------

DynamicPressureGaugeArea::DynamicPressureGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID) :
    VerticalGaugeArea(parentPanel, panelCoordinates, areaID, false, 73, PANEL_REDRAW_ALWAYS, meshTextureID)  // single gauge 73 pixels high
{
}

VerticalGaugeArea::RENDERDATA DynamicPressureGaugeArea::GetRenderData(const SIDE side)
{
    double dynamicPressure = GetVessel().GetDynPressure() / 1000;  // convert to kPa
    double frac = min(dynamicPressure / 150, 1.0); // gauge movement, 0...1
    int p = 66 - static_cast<int>((frac * 66) + 0.5);   // round to nearest pixel

    return _RENDERDATA(COLOR::GREEN, p);
}

//----------------------------------------------------------------------------------

// scram horizontal gauge area; one for L and R engines
// 91 pixels wide because gauge is 85 pixels, plus we need six extra pixels (three per side) for the triangle to stick out over the edges
ScramDiffuserTempHorizontalGaugeArea::ScramDiffuserTempHorizontalGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID) :
    HorizontalGaugeArea(parentPanel, panelCoordinates, areaID, false, 91, PANEL_REDRAW_ALWAYS, meshTextureID, 0, 0, 0, ((areaID == AID_SCRAMTEMP_LBAR) ? SIDE::TOP : SIDE::BOTTOM))
{
}

// side is not relevant to our rendering (since they match), so we can ignore it
HorizontalGaugeArea::RENDERDATA ScramDiffuserTempHorizontalGaugeArea::GetRenderData(const SIDE side)
{
    // if throttle off, reset temp to zero on gauge
    XR1Ramjet *pXR1Ramjet = GetXR1().ramjet;
    const int engine = ((GetAreaID() == AID_SCRAMTEMP_LBAR) ? 0 : 1);
    
    double thLevel = GetVessel().GetThrusterLevel(pXR1Ramjet->thdef[engine]->th); // throttle level
    double Td;  // diffuser temp

    // get the diffuser temp
    // which: 0=Td, 1=Tb, 2=Te
    Td = pXR1Ramjet->Temp(engine, 0);

    double fraction = min(1.0, Td / MAX_SCRAM_TEMPERATURE);    // 8000 deg max on gauge

    // compute pixel
    const int maxIndex = 84;   // total width = 85 pixels (index 0-84, inclusive)
    int index = static_cast<int>((maxIndex * fraction) + 0.5);  // round to nearest pixel

    // do not round pixels here if close to either edge

    return _RENDERDATA(COLOR::GREEN, index);
}

//-------------------------------------------------------------------------

ScramDiffuserTempNumberArea::ScramDiffuserTempNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    NumberArea(parentPanel, panelCoordinates, areaID, 5, true)   // 5 chars plus decimal
{
}

bool ScramDiffuserTempNumberArea::UpdateRenderData(RENDERDATA &renderData)
{
    bool redraw = false;
    const bool forceRedraw = renderData.forceRedraw;

    XR1Ramjet *pXR1Ramjet = GetXR1().ramjet;
    const int engine = ((GetAreaID() == AID_SCRAMTEMP_LTEXT) ? 0 : 1);

    double thLevel = GetVessel().GetThrusterLevel(pXR1Ramjet->thdef[engine]->th); // throttle level
    double Td;  // diffuser temp

    // get the diffuser temp
    // which: 0=Td, 1=Tb, 2=Te
    Td = pXR1Ramjet->Temp(engine, 0);

    // round to nearest 1/10th
    Td = (static_cast<double>(static_cast<int>((Td + 0.05) * 10))) / 10;

    // check whether the value has changed since the last render
    if (forceRedraw || (Td != renderData.value))
    {
        // Value has changed -- let's redo the string and see if that is different as well.
        // The goal here is to be as efficient as possible and only re-render when we absolutely have to.
        char pTemp[8];
        // ensure that value is in range
        if (Td > 9999.9)
            Td = 9999.9;   // trim
        else if (Td < -9999.9)
            Td = -9999.9;

        sprintf(pTemp, "%6.1f", Td);   // 5 chars + the decimal = length 6
        if (forceRedraw || (strcmp(pTemp, renderData.pStrToRender) != 0))
        {
            // text has changed; signal the base class to render it
            renderData.value = Td;   // remember for next time
            strcpy(renderData.pStrToRender, pTemp);
            redraw = true;
            renderData.forceRedraw = false;  // clear reset request
        }
    }

    // set font color based on temperature
    if (Td >= MAX_SCRAM_TEMPERATURE)
        renderData.color = COLOR::WHITE;
    else if (Td >= (MAX_SCRAM_TEMPERATURE * 0.97))
        renderData.color = COLOR::RED;
    else if (Td >= (MAX_SCRAM_TEMPERATURE * 0.94))
        renderData.color = COLOR::YELLOW;
    else
        renderData.color = COLOR::GREEN;

    return redraw;
}

//-------------------------------------------------------------------------

SlopeNumberArea::SlopeNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    NumberArea(parentPanel, panelCoordinates, areaID, 4, true)   // 4 chars plus decimal
{
}

bool SlopeNumberArea::UpdateRenderData(RENDERDATA &renderData)
{
    bool redraw = false;
    const bool forceRedraw = renderData.forceRedraw;

    // computing slope is relatively expensive (plus it should be available to other classes as well), so it is only done once per frame via a PostStep
    double slope = GetXR1().m_slope * DEG;  // convert to degrees

     // round to nearest 1/10th
    slope = (static_cast<double>(static_cast<int>((slope + 0.05) * 10))) / 10;
    
    // check whether the value has changed since the last render
    if (forceRedraw || (slope != renderData.value))
    {
        // Value has changed -- let's redo the string and see if that is different as well.
        // The goal here is to be as efficient as possible and only re-render when we absolutely have to.
        char pTemp[8];
        // ensure that value is in range
        if (slope > 99.9)
            slope = 99.9;   // trim to 2 leading digits
        else if (slope < -99.9)
            slope = -99.9;

        sprintf(pTemp, "%5.1f", slope);   // 4 chars + the decimal = length 5
        if (forceRedraw || (strcmp(pTemp, renderData.pStrToRender) != 0))
        {
            // text has changed; signal the base class to render it
            renderData.value = slope;   // remember for next time
            strcpy(renderData.pStrToRender, pTemp);
            redraw = true;
            renderData.forceRedraw = false;  // clear reset request
        }
    }

    // always render in the default green
    return redraw;
}

//----------------------------------------------------------------------------------

SlopeGaugeArea::SlopeGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    VerticalGaugeArea(parentPanel, panelCoordinates, areaID, false, 73, PANEL_REDRAW_ALWAYS)  // single gauge 73 pixels high
{
}

VerticalGaugeArea::RENDERDATA SlopeGaugeArea::GetRenderData(const SIDE side)
{
    // computing slope is relatively expensive (plus it should be available to other classes as well), so it is only done once per frame via a PostStep
    double slope = GetXR1().m_slope * DEG;  // convert to degrees

    // gauge can show 12 degrees: -6 to +6
    double frac = (slope + 6.0) / 12;
    
    COLOR color = COLOR::GREEN;    // assume in range
    if (frac < 0)
    {
        frac = 0;
        color = COLOR::YELLOW;     // show off-scale
    }
    else if (frac > 1.0)
    {
        frac = 1.0;
        color = COLOR::YELLOW;     // show off-scale
    }

    int p = 66 - static_cast<int>((frac * 66) + 0.5);   // round to nearest pixel

    return _RENDERDATA(color, p);  
}

//-------------------------------------------------------------------------

AOANumberArea::AOANumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    NumberArea(parentPanel, panelCoordinates, areaID, 4, true)   // 4 chars plus decimal
{
}

bool AOANumberArea::UpdateRenderData(RENDERDATA &renderData)
{
    bool redraw = false;
    const bool forceRedraw = renderData.forceRedraw;

    double aoa = (GetXR1().IsLanded() ? 0 : (GetVessel().GetAOA() * DEG));   // in degrees (always show 0 AoA if landed)

     // round to nearest 1/10th
    aoa = (static_cast<double>(static_cast<int>((aoa + 0.05) * 10))) / 10;
    
    // check whether the value has changed since the last render
    if (forceRedraw || (aoa != renderData.value))
    {
        // Value has changed -- let's redo the string and see if that is different as well.
        // The goal here is to be as efficient as possible and only re-render when we absolutely have to.
        char pTemp[8];
        // ensure that value is in range
        if (aoa > 99.9)
            aoa = 99.9;   // trim to 2 leading digits
        else if (aoa < -99.9)
            aoa = -99.9;

        sprintf(pTemp, "%5.1f", aoa);   // 4 chars + the decimal = length 5
        if (forceRedraw || (strcmp(pTemp, renderData.pStrToRender) != 0))
        {
            // text has changed; signal the base class to render it
            renderData.value = aoa;   // remember for next time
            strcpy(renderData.pStrToRender, pTemp);
            redraw = true;
            renderData.forceRedraw = false;  // clear reset request
        }
    }

    // always render in the default green
    return redraw;
}

//----------------------------------------------------------------------------------

AOAGaugeArea::AOAGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    VerticalGaugeArea(parentPanel, panelCoordinates, areaID, false, 73, PANEL_REDRAW_ALWAYS)  // single gauge 73 pixels high
{
}

VerticalGaugeArea::RENDERDATA AOAGaugeArea::GetRenderData(const SIDE side)
{
    double aoa = (GetXR1().IsLanded() ? 0 : (GetVessel().GetAOA() * DEG));   // in degrees.  Always show 0 AoA if wheel-stop.

    // gauge can show 60 degrees: -10 to +50
    double frac = (aoa + 10.0) / 60;
    COLOR color = COLOR::GREEN;    // assume gauge in range

    if (frac < 0)
    {
        frac = 0;
        color = COLOR::YELLOW;     // show off-scale
    }
    else if (frac > 1.0)
    {
        frac = 1.0;
        color = COLOR::YELLOW;     // show off-scale
    }

    int p = 66 - static_cast<int>((frac * 66) + 0.5);   // round to nearest pixel

    return _RENDERDATA(color, p);  
}

//----------------------------------------------------------------------------------

// slip indicator
SlipGaugeArea::SlipGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID) :
    XR1Area(parentPanel, panelCoordinates, areaID, meshTextureID),
    m_lastRenderedIndex(-1), m_yellowSurface(0), m_lastRenderedSrcSurface(0)
{
}

void SlipGaugeArea::Activate()
{
    Area::Activate();  // invoke superclass method
    // invoke the subclass to get the area size, excluding any delta values
    // we need 93 pixels wide because gauge is 85 pixels, plus we need eight extra pixels (four per side) for the triangle to stick out over the edges.
    const int sizeX = 93;
    const int sizeY = 9;

    if (IsVC()) // 3D panel?
    {
        oapiVCRegisterArea(GetAreaID(), GetRectForSize(sizeX, sizeY), PANEL_REDRAW_ALWAYS, PANEL_MOUSE_IGNORE, PANEL_MAP_BACKGROUND, GetVCPanelTextureHandle());
    }
    else    // 2D panel
    {
        oapiRegisterPanelArea(GetAreaID(), GetRectForSize(sizeX, sizeY), PANEL_REDRAW_ALWAYS, PANEL_MOUSE_IGNORE, PANEL_MAP_BACKGROUND);
    }

    m_mainSurface   = CreateSurface(IDB_INDICATOR4);
    m_yellowSurface = CreateSurface(IDB_INDICATOR4_YELLOW);
    
    DWORD white = 0xFFFFFF;           // set WHITE as transparent color; BLACK does not work for some reason!
    SetSurfaceColorKey(m_mainSurface, white);
    SetSurfaceColorKey(m_yellowSurface, white);

    // reset state variables to force a repaint
    m_lastRenderedIndex = -1;
}

void SlipGaugeArea::Deactivate()
{
    // clean up our extra resources
    DestroySurface(&m_yellowSurface);

    XR1Area::Deactivate();  // let superclass clean up
}

bool SlipGaugeArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    bool retVal = false;    // assume not re-rendered

    // compute the slip pixel index
    double slip = (GetXR1().IsLanded() ? 0 : (GetVessel().GetSlipAngle() * DEG)); // get slip in degrees (always show 0 slip if wheel-stop)

    // range is from +20 degrees to -20 degrees
    double frac = 1.0 - ((slip + 20) / 40);

    // keep in gauge range
    SURFHANDLE srcSurface = m_mainSurface;      // GREEN
    if (frac < 0)
    {
        frac = 0;
        srcSurface = m_yellowSurface;   // YELLOW: out of range!
    }
    else if (frac > 1.0)
    {
        frac = 1.0;
        srcSurface = m_yellowSurface;   // YELLOW: out of range!
    }

    // compute pixel
    const int maxIndex = 84;   // total width = 85 pixels (index 0-84, inclusive)
    int index = static_cast<int>((maxIndex * frac) + 0.5);  // round to nearest pixel

    // check whether either gauge needs to be repainted
    if ((index != m_lastRenderedIndex) || (srcSurface != m_lastRenderedSrcSurface))
    {
        // render the gauge
        //      tgt,  src,        tgtx,  tgty,srcx,srcy,w,h,<use predefined color key>
        oapiBlt(surf, srcSurface, index, 0, 0, 9, 9, 9, SURF_PREDEF_CK);

        // update last rendered index
        m_lastRenderedIndex = index;
        m_lastRenderedSrcSurface = srcSurface;
        retVal = true;      // we repainted the gauge
    }

    return retVal;
}

//-------------------------------------------------------------------------

SlipNumberArea::SlipNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    NumberArea(parentPanel, panelCoordinates, areaID, 4, true)   // 4 chars plus decimal
{
}

bool SlipNumberArea::UpdateRenderData(RENDERDATA &renderData)
{
    bool redraw = false;
    const bool forceRedraw = renderData.forceRedraw;

    double slip = (GetXR1().IsLanded() ? 0 : (GetVessel().GetSlipAngle() * DEG)); // get slip in degrees (always show 0 slip if wheel-stop)

    // check whether the value has changed since the last render
    if (forceRedraw || (slip != renderData.value))
    {
        // Value has changed -- let's redo the string and see if that is different as well.
        // The goal here is to be as efficient as possible and only re-render when we absolutely have to.
        char pTemp[8];
        // ensure that value is in range
        if (slip > 99.9)
            slip = 99.9;   // trim
        else if (slip < -99.9)
            slip = -99.9;

        sprintf(pTemp, "%5.1f", slip); 
        
        // DEBUG: sprintf(oapiDebugString(), "slip='%s' (len=%d)", pTemp, strlen(pTemp));
        
        if (forceRedraw || (strcmp(pTemp, renderData.pStrToRender) != 0))
        {
            // text has changed; signal the base class to render it
            renderData.value = slip;   // remember for next time
            strcpy(renderData.pStrToRender, pTemp);
            redraw = true;
            renderData.forceRedraw = false;  // clear reset request
        }
    }

    // font color is always green
    renderData.color = COLOR::GREEN;

    return redraw;
}

//-------------------------------------------------------------------------

APUFuelBarArea::APUFuelBarArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    BarArea(parentPanel, panelCoordinates, areaID, 32, 41, ORIENTATION::VERTICAL)   // 32 wide x 41 high, VERTICAL orientation
{
}

BarArea::RENDERDATA APUFuelBarArea::GetRenderData()
{
    const double remaining = GetXR1().m_apuFuelQty;
    return _RENDERDATA(COLOR::GREEN, remaining, remaining, APU_FUEL_CAPACITY);
}

//-------------------------------------------------------------------------

APUFuelNumberArea::APUFuelNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    NumberArea(parentPanel, panelCoordinates, areaID, 4, true)   // 4 chars plus decimal
{
}

bool APUFuelNumberArea::UpdateRenderData(RENDERDATA &renderData)
{
    bool redraw = false;
    const bool forceRedraw = renderData.forceRedraw;

    double currentFuelMass = GetXR1().m_apuFuelQty;

     // round to nearest 1/10th
    currentFuelMass = (static_cast<double>(static_cast<int>((currentFuelMass + 0.05) * 10))) / 10;
    
    // check whether the value has changed since the last render
    if (forceRedraw || (currentFuelMass != renderData.value))
    {
        // Value has changed -- let's redo the string and see if that is different as well.
        // The goal here is to be as efficient as possible and only re-render when we absolutely have to.
        char pTemp[12];
        // ensure that value is in range
        if (currentFuelMass > 9999)
            currentFuelMass = 9999;   
        else if (currentFuelMass < 0)      // should never happen, but just in case
            currentFuelMass = 0;

        // Note: pFormatStr must evaluate to exactly five characters, including exactly one decimal.
        const char *pFormatStr;
        if (currentFuelMass > 999.9)
            pFormatStr = "%4.0f.";    // four chars + trailing "." = 5 chars total
        else if (currentFuelMass > 99.9)
            pFormatStr = "%5.1f";
        else if (currentFuelMass > 9.9)    
            pFormatStr = "%5.2f";
        else
            pFormatStr = "%5.3f";

        sprintf(pTemp, pFormatStr, currentFuelMass); 
        if (forceRedraw || (strcmp(pTemp, renderData.pStrToRender) != 0))
        {
            // text has changed; signal the base class to render it
            renderData.value = currentFuelMass;   // remember for next time
            strcpy(renderData.pStrToRender, pTemp);
            redraw = true;
            renderData.forceRedraw = false;  // clear reset request
        }
    }

    return redraw;
}

//----------------------------------------------------------------------------------

APUButton::APUButton(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_lightState(LightState::UNPRESSED_DARK)
{
}

void APUButton::Activate()
{
    Area::Activate();  // invoke superclass method
    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(40, 29), PANEL_REDRAW_USER, PANEL_MOUSE_LBDOWN, PANEL_MAP_BGONREQUEST);
    m_mainSurface = CreateSurface(IDB_APU_BUTTON);
}

bool APUButton::Redraw2D(const int event, const SURFHANDLE surf)
{
    LightState lightState = m_lightState;

    // handle MWS TEST button
    if (GetXR1().m_mwsTestActive)
    {
        // light is always on for test button
        if ((lightState == LightState::UNPRESSED_DARK) || (lightState == LightState::UNPRESSED_BRIGHT))
            lightState = LightState::UNPRESSED_BRIGHT;
        else
            lightState = LightState::PRESSED_BRIGHT;
    }

    switch (lightState)
    {
    case LightState::UNPRESSED_DARK:
        oapiBltPanelAreaBackground(GetAreaID(), surf);
        break;

    case LightState::UNPRESSED_BRIGHT:
        oapiBlt(surf, m_mainSurface, 0, 0, 80, 0, 40, 29);  
        break;

    case LightState::PRESSED_DARK:
        oapiBlt(surf, m_mainSurface, 0, 0, 40, 0, 40, 29);  
        break;

    case LightState::PRESSED_BRIGHT:
        oapiBlt(surf, m_mainSurface, 0, 0, 0, 0, 40, 29);
        break;
    }

    // always return true because we are only drawn on request
    return true;    
}

bool APUButton::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    // toggle button state
    GetXR1().ToggleAPU();

    GetXR1().PlaySound(GetXR1().SwitchOn, DeltaGliderXR1::ST_Other);  // normal click

    return true;
}

// monitors APU button and fuel states
void APUButton::clbkPrePostStep(const double simt, const double simdt, const double mjd) 
{
    // set door status (pressed/unpressed)
    const DoorStatus doorStatus = GetXR1().apu_status;
    bool isPressed = ((doorStatus == DoorStatus::DOOR_OPEN) || (doorStatus == DoorStatus::DOOR_OPENING));
    bool isLit;   // set below

    // if startup or shutdown in progress, blink light rapidly
    if ((doorStatus == DoorStatus::DOOR_OPENING) || (doorStatus == DoorStatus::DOOR_CLOSING))
    {
        isLit = (fmod(simt, 0.5) < 0.25);  // blink twice a second to show startup/shutdown mode
    }
    else if (GetXR1().m_apuWarning)  // if warning active, set the blink state
    {
        double di;
        isLit = (modf(simt, &di) < 0.5);   // blink in sync w/MWS light in case MWS is flashing
    }
    else    // normal operation
    {
        isLit = (doorStatus == DoorStatus::DOOR_OPEN);
    }

    // set the light state
    LightState lightState;
    if (isPressed)
        lightState = (isLit ? LightState::PRESSED_BRIGHT : LightState::PRESSED_DARK);
    else
        lightState = (isLit ? LightState::UNPRESSED_BRIGHT : LightState::UNPRESSED_DARK);

    // update member var and trigger a redraw if state has changed
    if (lightState != m_lightState)
    {
        m_lightState = lightState;
        TriggerRedraw();
    }
}

//-------------------------------------------------------------------------

CenterOfGravityRockerSwitchArea::CenterOfGravityRockerSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID) :
    VerticalCenteringRockerSwitchArea(parentPanel, panelCoordinates, areaID, meshTextureID, false)     // this is a single switch
{
}

// Process a mouse event that occurred on our switch
// switches = which switches moved (LEFT, RIGHT, BOTH, SINGLE, NA)
// position = current switch position (UP, DOWN, CENTER)
void CenterOfGravityRockerSwitchArea::ProcessSwitchEvent(SWITCHES switches, POSITION position)
{
    if (GetXR1().VerifyManualCOGShiftAvailable() == false)   // plays a warning if unavailable
        return;

    // perform the COG shift
    if (position != POSITION::CENTER)   // is switch pressed in either direction?
    {
        // Note: to shift the center of gravity *forward* ("UP" on the switch), we must shift the center of lift *aft*
        double shift = oapiGetSimStep() * COL_MAX_SHIFT_RATE * (position == POSITION::UP ? -1.0 : 1.0);  // shift as a fraction of balance for this timestep

        // perform the shift, keeping it in range
        GetXR1().ShiftCenterOfLift(shift);
    }
}

//----------------------------------------------------------------------------------

// NOTE: auto mode is a read-only indicator
CenterOfGravityAutoButtonArea::CenterOfGravityAutoButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    XR1Area(parentPanel, panelCoordinates, areaID)
{
}

void CenterOfGravityAutoButtonArea::Activate()
{
    Area::Activate();  // invoke superclass method
    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(18, 15), PANEL_REDRAW_USER, PANEL_MOUSE_IGNORE);  // redrawn only on request from the poststep
    m_mainSurface = CreateSurface(IDB_GREEN_LED_TINY);
}

bool CenterOfGravityAutoButtonArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    // always render this since it is only drawn by request
    oapiBlt(surf, m_mainSurface, 0, 0, (GetXR1().m_cogShiftAutoModeActive ? 18 : 0), 0, 18, 15);
    return true;
}

#ifdef UNNECESSARY_NO_AUTO_MODE_FUNCTIONALITY
void CenterOfGravityAutoButtonArea::ProcessButtonAction(int event, double buttonDownSimt)
{
    // Note: auto mode can always be toggled UNLESS the autopilot has it locked
    if (GetXR1().m_customAutopilotMode == AP_ATTITUDEHOLD)
    {
        GetXR1().PlayErrorBeep();
        GetXR1().ShowWarning("Locked by Attitude Hold.wav", DeltaGliderXR1::ST_WarningCallout, "Center of Gravity shift locked&by Attitude Hold Autopilot.");
        return;
    }

    // OK to toggle auto mode
    if (event & PANEL_MOUSE_LBDOWN)
    {
        // toggle button state
        GetXR1().m_cogShiftAutoModeActive = !GetXR1().m_cogShiftAutoModeActive;
        GetXR1().PlaySound(IsLit() ? GetXR1().BeepHigh : GetXR1().BeepLow);

        // If AUTO mode engaged, enable CENTER mode to re-center to COG *if* the COG is off-center; this
        // will prevent the button from flickering for an instant if the COG is already centered.
        if (GetXR1().m_cogShiftAutoModeActive && (GetXR1().m_centerOfLift != NEUTRAL_CENTER_OF_LIFT))
        {
            GetXR1().SetRecenterCenterOfGravityMode(true);
            GetXR1().m_cogForceRecenter = true;     // override AUTO MODE check
        }
    }
}
#endif

//-------------------------------------------------------------------------

CenterOfGravityNumberArea::CenterOfGravityNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    NumberArea(parentPanel, panelCoordinates, areaID, 6, true)   // 6 chars plus decimal
{
}

bool CenterOfGravityNumberArea::UpdateRenderData(RENDERDATA &renderData)
{
    bool redraw = false;
    const bool forceRedraw = renderData.forceRedraw;

    // Note: "Center of Gravity" is really shown as a delta from the neutral center of lift on the wing; however, the effect is the same.
    double centerOfGravity = -(GetXR1().m_centerOfLift - NEUTRAL_CENTER_OF_LIFT);     // positive COL means COG is aft, negative means COG is forward

    // no need to round here; sprintf will do it
    
    // check whether the value has changed since the last render
    if (forceRedraw || (centerOfGravity != renderData.value))
    {
        // Value has changed -- let's redo the string and see if that is different as well.
        // The goal here is to be as efficient as possible and only re-render when we absolutely have to.
        char pTemp[8];
        // sanity-check ensure that value is in displayable range
        if (centerOfGravity > 99.999)
            centerOfGravity = 99.999;   // trim to max length
        else if (centerOfGravity < -99.999)
            centerOfGravity = -99.999;

        const char *pFormatStr;
        if (fabs(centerOfGravity) > 9.9999)
            pFormatStr = "%7.3f";
        else 
            pFormatStr = "%7.4f";

        sprintf(pTemp, pFormatStr, centerOfGravity);
        
        // remove the leading "-" if value shows as -0.0000
        if (strcmp(pTemp, "-0.0000") == 0)
            *pTemp = ' ';

        if (forceRedraw || (strcmp(pTemp, renderData.pStrToRender) != 0))
        {
            // text has changed; signal the base class to render it
            renderData.value = centerOfGravity;   // remember for next time
            strcpy(renderData.pStrToRender, pTemp);
            redraw = true;
            renderData.forceRedraw = false;  // clear reset request
        }
    }

    // always render in the default green
    return redraw;
}

//----------------------------------------------------------------------------------

CenterOfGravityGaugeArea::CenterOfGravityGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    VerticalGaugeArea(parentPanel, panelCoordinates, areaID, false, 73, PANEL_REDRAW_ALWAYS)  // single gauge 73 pixels high
{
}

VerticalGaugeArea::RENDERDATA CenterOfGravityGaugeArea::GetRenderData(const SIDE side)
{
    // Note: "Center of Gravity" is really shown as a delta from the neutral center of lift on the wing; however, the effect is the same.
    const double centerOfGravity = -(GetXR1().m_centerOfLift - NEUTRAL_CENTER_OF_LIFT);     // positive COL means COG is aft, negative means COG is forward

    // gauge can show COL_SHIFT_GAUGE_LIMIT degrees: -COL_SHIFT_GAUGE_LIMIT to +COL_SHIFT_GAUGE_LIMIT
    // Fraction is: distanceFromMidpoint / totalDistance = 0 < n < 1.0
    double frac = (centerOfGravity + COL_SHIFT_GAUGE_LIMIT) / (COL_SHIFT_GAUGE_LIMIT * 2);
    
    COLOR color = COLOR::GREEN;    // assume in range
    if (frac < 0)
    {
        frac = 0;
        color = COLOR::YELLOW;     // show off-scale
    }
    else if (frac > 1.0)
    {
        frac = 1.0;
        color = COLOR::YELLOW;     // show off-scale
    }

    int p = 66 - static_cast<int>((frac * 66) + 0.5);   // round to nearest pixel

    return _RENDERDATA(color, p);  
}

