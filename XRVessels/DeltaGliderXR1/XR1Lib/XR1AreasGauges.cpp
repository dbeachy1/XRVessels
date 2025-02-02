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

// must be included BEFORE XR1Areas.h
#include "DeltaGliderXR1.h"
#include "XR1Areas.h"

//----------------------------------------------------------------------------------

IndicatorGaugeArea::IndicatorGaugeArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID, const bool isDual,
    const int redrawFlag, const int meshTextureID, const int deltaX, const int deltaY, const int gapSize) :
    XR1Area(parentPanel, panelCoordinates, areaID, meshTextureID),
    m_isDual(isDual), m_redIndicatorSurface(nullptr), m_yellowIndicatorSurface(nullptr), m_redrawFlag(redrawFlag),
    m_deltaX(deltaX), m_deltaY(deltaY), m_gapSize(gapSize)
{
}

void IndicatorGaugeArea::Activate()
{
    Area::Activate();  // invoke superclass method

    // invoke the subclass to get the area size, excluding any delta values
    COORD2 areaSize = GetAreaSize();
    const int sizeX = areaSize.x;
    const int sizeY = areaSize.y;

    if (IsVC()) // 3D panel?
    {
        oapiVCRegisterArea(GetAreaID(), GetRectForSize(sizeX + m_deltaX, sizeY + m_deltaY), m_redrawFlag, PANEL_MOUSE_IGNORE, PANEL_MAP_BGONREQUEST, GetVCPanelTextureHandle());
    }
    else    // 2D panel
    {
        oapiRegisterPanelArea(GetAreaID(), GetRectForSize(sizeX + m_deltaX, sizeY + m_deltaY), m_redrawFlag, PANEL_MOUSE_IGNORE, PANEL_MAP_BGONREQUEST);
    }

    m_mainSurface = CreateSurface(IDB_INDICATOR2);              // standard green indicator arrows
    m_redIndicatorSurface = CreateSurface(IDB_RED_INDICATOR2);  // red indicator arrows
    m_yellowIndicatorSurface = CreateSurface(IDB_YELLOW_INDICATOR2);  // yellow indicator arrows

    DWORD white = 0xFFFFFF;           // set WHITE as transparent color (Note: to use black, set 0xFF000000, not 0!)
    SetSurfaceColorKey(m_mainSurface, white);
    SetSurfaceColorKey(m_redIndicatorSurface, white);
    SetSurfaceColorKey(m_yellowIndicatorSurface, white);

    // reset state variables to force a repaint
    ResetRenderData();
    TriggerRedraw();
}

void IndicatorGaugeArea::Deactivate()
{
    DestroySurface(&m_redIndicatorSurface);        // clean up secondary surface
    DestroySurface(&m_yellowIndicatorSurface);     // clean up secondary surface
    XR1Area::Deactivate();  // invoke superclass method
}

SURFHANDLE IndicatorGaugeArea::GetSurfaceForColor(COLOR c)
{
    SURFHANDLE srcSurface;
    switch (c)
    {
    case COLOR::RED:
        srcSurface = m_redIndicatorSurface;
        break;

    case COLOR::YELLOW:
        srcSurface = m_yellowIndicatorSurface;
        break;

    default:  // GREEN or unknown...
        srcSurface = m_mainSurface;
        break;
    }

    return srcSurface;
}

//----------------------------------------------------------------------------------

// vertical gauge area; may be single or dual, and may render in green, red, or both
// registered area is either 6 pixels wide (single) or 13 pixels wide (dual)
// sizeY = vertical size of gauge in pixels
// deltaX, deltaY = offset from area start to draw pointer; default is 0
// gapSize = gap (in pixels) between dual indicators; default=1.  Has no effect on single-indicator gauges.
// singleSide = LEFT or RIGHT; default = LEFT (determines which indicator arrow to use for a single gauge)
VerticalGaugeArea::VerticalGaugeArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID, const bool isDual, const int sizeY,
    const int redrawFlag, const int meshTextureID, const int deltaX, const int deltaY, const int gapSize, const SIDE singleSide) :
    IndicatorGaugeArea(parentPanel, panelCoordinates, areaID, isDual, redrawFlag, meshTextureID, deltaX, deltaY, gapSize),
    m_sizeY(sizeY), m_singleSide(singleSide)
{
}

// get area size in pixels, excluding any delta size additions
COORD2 VerticalGaugeArea::GetAreaSize()
{
    int sizeX = (m_isDual ? (12 + m_gapSize) : 6);  // each indicator is 6 pixels side
    return _COORD2(sizeX, m_sizeY);
}

void VerticalGaugeArea::ResetRenderData()
{
    m_lastRenderData[0].Reset();
    m_lastRenderData[1].Reset();
}

bool VerticalGaugeArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    bool retVal = false;    // assume not re-rendered

    int gaugeCount = (m_isDual ? 2 : 1);

    // check whether either gauge needs to be repainted
    RENDERDATA renderData[2];
    bool doRender = false;  // assume no render required
    for (int i = 0; i < gaugeCount; i++)
    {
        // invoke subclass method to obtain color and indexY data for each gauge
        renderData[i] = GetRenderData((i == 0 ? SIDE::LEFT : SIDE::RIGHT));
        if (renderData[i] != m_lastRenderData[i])
        {
            doRender = true;
            // do not break here; allow renderData[1] to be initialized if present
        }
    }

    if (doRender)
    {
        // repaint the background
        oapiBltPanelAreaBackground(GetAreaID(), surf);

        // invoke the caller's hook in case he needs to paint something before we do
        Redraw2DFirstHook(event, surf);

        if (m_isDual)
        {
            // repaint the gauge
            for (int i = 0; i < gaugeCount; i++)
            {
                SIDE side = (m_isDual ? ((i == 0) ? SIDE::LEFT : SIDE::RIGHT) : m_singleSide);

                // render the gauge
                const SURFHANDLE srcSurface = GetSurfaceForColor(renderData[i].color);
                int tgtX = ((side == SIDE::LEFT) ? 0 : 6 + m_gapSize);  // if right side, bump right 6+gapSize pixels
                int srcX = ((side == SIDE::LEFT) ? 0 : 6);  // if right side, go right 6 pixels for source
                //      tgt,  src,        tgtx,            tgty,                            srcx,srcy,w,h, <use predefined color key>
                DeltaGliderXR1::SafeBlt(surf, srcSurface, tgtX + m_deltaX, renderData[i].indexY + m_deltaY, srcX, 0, 6, 7, SURF_PREDEF_CK);


                // update <last updated> render data
                m_lastRenderData[i] = renderData[i];   // default byte-for-byte copy here is fine
            }
        }
        else    // single gauge
        {
            // render the gauge
            const SURFHANDLE srcSurface = GetSurfaceForColor(renderData[0].color);
            int srcX = ((m_singleSide == SIDE::LEFT) ? 0 : 6);  // if right side, go right 6 pixels for source
            //      tgt,  src,        tgtx,         tgty,                            srcx,srcy,w,h, <use predefined color key>
            DeltaGliderXR1::SafeBlt(surf, srcSurface, 0 + m_deltaX, renderData[0].indexY + m_deltaY, srcX, 0, 6, 7, SURF_PREDEF_CK);

            // update <last updated> render data
            m_lastRenderData[0] = renderData[0];   // default byte-for-byte copy here is fine
        }
        retVal = true;
    }

    return retVal;
}

//----------------------------------------------------------------------------------

// horizontal gauge area; may be single or dual, and may render in green, red, or both
// registered area is either 6 pixels high (single) or 13 pixels high (dual)
// sizeX = horizontal size of gauge in pixels, including 3 pixels on each side of movable area auge for arrow to display
// redrawFlag = PANEL_REDRAW_USER, PANEL_REDRAW_ALWAYS, etc.
// deltaX, deltaY = offset from area start to draw pointer; default is 0
// gapSize = gap (in pixels) between dual indicators; default=1.  Has no effect on single-indicator gauges.
// singleSide = TOP or BOTTOM; default = BOTTOM  (determines which indicator arrow to use for a single gauge)
HorizontalGaugeArea::HorizontalGaugeArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID, const bool isDual, const int sizeX,
    const int redrawFlag, const int meshTextureID, const int deltaX, const int deltaY, const int gapSize, const SIDE singleSide) :
    IndicatorGaugeArea(parentPanel, panelCoordinates, areaID, isDual, redrawFlag, meshTextureID, deltaX, deltaY, gapSize),
    m_sizeX(sizeX), m_singleSide(singleSide)
{
}

// get area size in pixels, excluding any delta size additions
COORD2 HorizontalGaugeArea::GetAreaSize()
{
    int sizeY = (m_isDual ? (12 + m_gapSize) : 6);    // each indicator is 6 pixels side
    return _COORD2(m_sizeX, sizeY);
}

void HorizontalGaugeArea::ResetRenderData()
{
    m_lastRenderData[0].Reset();
    m_lastRenderData[1].Reset();
}

bool HorizontalGaugeArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    bool retVal = false;    // assume not re-rendered

    int gaugeCount = (m_isDual ? 2 : 1);

    // check whether either gauge needs to be repainted
    RENDERDATA renderData[2];
    bool doRender = false;  // assume no render required
    for (int i = 0; i < gaugeCount; i++)
    {
        // invoke subclass method to obtain color and indexY data for each gauge
        renderData[i] = GetRenderData((i == 0 ? SIDE::TOP : SIDE::BOTTOM));
        if (renderData[i] != m_lastRenderData[i])
        {
            doRender = true;
            // do not break here; allow renderData[1] to be initialized if present
        }
    }

    if (doRender)
    {
        // repaint the background
        oapiBltPanelAreaBackground(GetAreaID(), surf);

        // invoke the caller's hook in case he needs to paint something before we do
        Redraw2DFirstHook(event, surf);

        // repaint the gauge
        if (m_isDual)
        {
            for (int i = 0; i < gaugeCount; i++)
            {
                SIDE side = ((i == 0) ? SIDE::TOP : SIDE::BOTTOM);

                const SURFHANDLE srcSurface = GetSurfaceForColor(renderData[i].color);
                int tgtY = ((side == SIDE::TOP) ? 0 : 6 + m_gapSize);  // if bottom side, bump down 6+gapSize pixels
                int srcX = ((side == SIDE::TOP) ? 0 : 7);  // if bottom side, go down 7 pixels for source
                //      tgt,  src,        tgtx,                          tgty,          srcx,srcy,w,h, <use predefined color key>
                DeltaGliderXR1::SafeBlt(surf, srcSurface, renderData[i].indexX + m_deltaX, tgtY + m_deltaY, srcX, 8, 7, 6, SURF_PREDEF_CK);

                // update <last updated> render data
                m_lastRenderData[i] = renderData[i];   // default byte-for-byte copy here is fine
            }
        }
        else    // single gauge
        {
            // render the gauge
            const SURFHANDLE srcSurface = GetSurfaceForColor(renderData[0].color);
            int srcX = ((m_singleSide == SIDE::TOP) ? 0 : 7);  // if bottom side, go down 7 pixels for source
            //      tgt,  src,        tgtx,                            tgty,            srcx,srcy,w,h, <use predefined color key>
            DeltaGliderXR1::SafeBlt(surf, srcSurface, renderData[0].indexX + m_deltaX, 0 + m_deltaY, srcX, 8, 7, 6, SURF_PREDEF_CK);


            // update <last updated> render data
            m_lastRenderData[0] = renderData[0];   // default byte-for-byte copy here is fine
        }
        retVal = true;
    }

    return retVal;
}

//----------------------------------------------------------------------------------

// analog gauge area
AnalogGaugeArea::AnalogGaugeArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID, const double initialAngle, const int meshTextureID) :
    XR1Area(parentPanel, panelCoordinates, areaID, meshTextureID),
    m_initialAngle(initialAngle)
{
}

void AnalogGaugeArea::Activate()
{
    Area::Activate();  // invoke superclass method
    if (IsVC())  // 3D panel
    {
        oapiVCRegisterArea(GetAreaID(), GetRectForSize(56, 56), PANEL_REDRAW_ALWAYS, PANEL_MOUSE_IGNORE, PANEL_MAP_BGONREQUEST, GetVCPanelTextureHandle());
    }
    else    // 2D
    {
        oapiRegisterPanelArea(GetAreaID(), GetRectForSize(56, 56), PANEL_REDRAW_ALWAYS, PANEL_MOUSE_IGNORE, PANEL_MAP_BGONREQUEST);
    }
    m_lastIndicatorAngle = m_initialAngle;

    m_pen0 = CreatePen(PS_SOLID, 1, RGB(224, 224, 224));
    m_pen1 = CreatePen(PS_SOLID, 3, RGB(164, 164, 164));
}

void AnalogGaugeArea::Deactivate()
{
    // clean up our resources
    DeleteObject(m_pen0);
    DeleteObject(m_pen1);

    XR1Area::Deactivate();  // let superclass clean up
}

bool AnalogGaugeArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    bool retVal = false;    // assume not re-rendered
    bool forceRedraw = ((event & PANEL_REDRAW_INIT) != 0);
    static const double eps = 1e-2;  // sensitivity before redraw occurs; lower = more frequent redraws

    // invoke subclass method to obtain the dial angle
    double dialAngle = GetDialAngle();

    if (forceRedraw || (fabs(dialAngle - m_lastIndicatorAngle) > eps))
    {
        oapiBltPanelAreaBackground(GetAreaID(), surf);
        HDC hDC = GetDC(surf);
        DrawNeedle(hDC, 28, 28, 26.0, dialAngle);
        ReleaseDC(surf, hDC);
        retVal = true;
    }

    return retVal;
}

// angle = needle angle
// speed = how fast the dial moves in radians per second.  Default = PI radians per second, or 180 degrees per second
void AnalogGaugeArea::DrawNeedle(HDC hDC, int x, int y, double rad, double angle, double speed)
{
    // handle needle response delay
    double dt = oapiGetSimStep();  // delta time since last frame
    if ((fabs(angle - m_lastIndicatorAngle) / dt) >= speed)
    {
        if (angle > m_lastIndicatorAngle)
            angle = m_lastIndicatorAngle + (speed * dt);
        else
            angle = m_lastIndicatorAngle - (speed * dt);
    }

    m_lastIndicatorAngle = angle;

    double dx = rad * cos(angle), dy = rad * sin(angle);
    HGDIOBJ oldObject = SelectObject(hDC, m_pen1);
    MoveToEx(hDC, x, y, 0);
    LineTo(hDC, x + static_cast<int>(0.85 * dx + 0.5), y - static_cast<int>(0.85 * dy + 0.5));

    SelectObject(hDC, m_pen0);
    MoveToEx(hDC, x, y, 0);
    LineTo(hDC, x + static_cast<int>(dx + 0.5), y - static_cast<int>(dy + 0.5));

    SelectObject(hDC, oldObject);    // clean up
}

//----------------------------------------------------------------------------------

// acceleration horizontal gauge area
// 91 pixels wide because gauge is 85 pixels, plus we need six extra pixels (three per side) for the triangle to stick out over the edges
// maxG = maximum G value on gauge
AccHorizontalGaugeArea::AccHorizontalGaugeArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID, const AXIS axis, const bool isDual, const SIDE side, const int meshTextureID) :
    HorizontalGaugeArea(parentPanel, panelCoordinates, areaID, isDual, 91, PANEL_REDRAW_ALWAYS, meshTextureID, 0, 0, -1, side),  // gap of 0 for dual indicator
    m_axis(axis)
{
}

// override Activate so we can use a bright green arrow instead
void AccHorizontalGaugeArea::Activate()
{
    HorizontalGaugeArea::Activate();  // invoke superclass method
    DestroySurface(&m_mainSurface);

    m_mainSurface = CreateSurface(IDB_GREEN_INDICATOR2);  // bright green arrow
    DWORD white = 0xFFFFFF;           // set WHITE as transparent color; BLACK does not work for some reason!
    SetSurfaceColorKey(m_mainSurface, white);
}

// side = TOP or BOTTOM
HorizontalGaugeArea::RENDERDATA AccHorizontalGaugeArea::GetRenderData(const SIDE side)
{
    // use shared acceleration values set once per frame by ComputeAccPostStep object
    const VECTOR3& A = GetXR1().m_acceleration;
    double acc;
    switch (m_axis)
    {
    case AXIS::X:
        acc = A.x;
        break;

    case AXIS::Y:
        acc = A.y;
        break;

    case AXIS::Z:
        acc = A.z;
        break;
    }

    const double gravities = acc / G;     // acc in Gs
    double fraction = gravities / GetXR1().m_maxGaugeAcc;

    // to keep display clean, round fraction to nearest .001
    fraction = static_cast<double>(static_cast<int>((fraction + .0005) * 1000)) / 1000;
    double absFraction = fabs(fraction);
    bool isNegative = fraction < 0;

    COLOR color;
    if (absFraction > 1.0)
    {
        absFraction = 1.0;     // over-G!  Render in YELLOW.
        color = COLOR::YELLOW;
    }
    else    // in range
        color = (isNegative ? COLOR::RED : COLOR::GREEN);

    // compute pixel
    const int maxIndex = 84;    // total width = 85 pixels (index 0-84, inclusive)
    int index = static_cast<int>((maxIndex * absFraction) + 0.5);  // round to nearest pixel

    // do not round pixels here if close to either edge

    // set up RenderData
    return _RENDERDATA(color, index);
}

//-------------------------------------------------------------------------

AccScaleArea::AccScaleArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_accScale(AccScale::NONE)
{
}

void AccScaleArea::Activate()
{
    Area::Activate();  // invoke superclass method
    m_mainSurface = CreateSurface(IDB_GSCALE);

    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(92, 11), PANEL_REDRAW_ALWAYS, PANEL_MOUSE_IGNORE, PANEL_MAP_NONE);

    m_accScale = AccScale::NONE;  // force a redraw
}

bool AccScaleArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    bool redraw = false;

    // check whether the required ACC scale has changed since last render
    // also, don't render until the poststep has had a chance to run at least once
    if ((GetXR1().m_accScale != AccScale::NONE) && (GetXR1().m_accScale != m_accScale))
    {
        // need to redraw it
        // Y coordinate
        int y = static_cast<int>(GetXR1().m_accScale) * 11;       // 11 pixels high per row

        // redraw the entire area
        //                          tgX,tgY,srcX,srcY,width,height
        DeltaGliderXR1::SafeBlt(surf, m_mainSurface, 0, 0, 0, y, 92, 11);
        //oapiColourFill(surf, BRIGHT_YELLOW);

        redraw = true;
        m_accScale = GetXR1().m_accScale;     // remember new scale
    }

    return redraw;
}

//----------------------------------------------------------------------------------

// 'percentage' horizontal gauge area; may be single or dual, and may render in green, red, or both
// registered area is either 6 pixels high (single) or 13 pixels high (dual)
// sizeX = horizontal size of gauge in pixels
// redrawFlag = PANEL_REDRAW_USER, PANEL_REDRAW_ALWAYS, etc.
// deltaX, deltaY = offset from area start to draw pointer; default is 0
// gapSize = gap (in pixels) between dual indicators; default=1.  Has no effect on single-indicator gauges.
PctHorizontalGaugeArea::PctHorizontalGaugeArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID, const bool isDual, const int sizeX,
    const int redrawFlag, const int meshTextureID, const int deltaX, const int deltaY, const int gapSize) :
    HorizontalGaugeArea(parentPanel, panelCoordinates, areaID, isDual, sizeX, redrawFlag, meshTextureID, deltaX, deltaY, gapSize)
{
}

// side = LEFT or RIGHT
HorizontalGaugeArea::RENDERDATA PctHorizontalGaugeArea::GetRenderData(const SIDE side)
{
    // get the % to move on the gauge from the subclass
    COLOR color;
    double fraction = GetFraction(side, color);        // 0...1

    // compute pixel
    const int maxIndex = 84;    // total width = 85 pixels (index 0-84, inclusive)
    int index = static_cast<int>((maxIndex * fraction) + 0.5);  // round to nearest pixel

    // set up RenderData
    return _RENDERDATA(color, index);
}
