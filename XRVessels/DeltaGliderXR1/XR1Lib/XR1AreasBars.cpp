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

// must be included BEFORE XR1Areas.h
#include "DeltaGliderXR1.h"
#include "XR1Areas.h"

//-------------------------------------------------------------------------

// sizeX, sizeY = size of bar
// maxValue = bar value for 100%
// orientation = HORIZONTAL or VERTICAL; default = HORIZONTAL
BarArea::BarArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID, int sizeX, int sizeY, ORIENTATION orientation) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_sizeX(sizeX), m_sizeY(sizeY), m_orientation(orientation)
{
}

void BarArea::Activate()
{
    Area::Activate();  // invoke superclass method
    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(m_sizeX, m_sizeY), PANEL_REDRAW_ALWAYS, PANEL_MOUSE_IGNORE, PANEL_MAP_BGONREQUEST);

    m_lastRenderData.Reset();  // force a repaint on first call to Redraw
}

bool BarArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    bool retVal = false;

    // invoke subclass method to obtain color and indexY data for each gauge
    RENDERDATA renderData = GetRenderData();

    if (renderData != m_lastRenderData)
    {
        // remember this so we don't re-render each time
        m_lastRenderData = renderData;  // byte-for-byte copy

        // NOTE: 0 <= brightIndex <= darkIndex
        const int brightIndex = renderData.GetIndex(BARPORTION::BRIGHT);  // first part of bar
        const int darkIndex = renderData.GetIndex(BARPORTION::DARK);    // second part of bar

        // reset background
        oapiBltPanelAreaBackground(GetAreaID(), surf);

        // now paint the bar IF there is anything to paint
        // NOTE: cannot pass barSize of 0 here, or entire width is painted!
        if ((brightIndex > 0) || (darkIndex > 0))  // anything on the gauge at all?
        {
            // create the color based on the enum
            DWORD color;
            DWORD darkColor;
            switch (renderData.color)
            {
            case COLOR::GREEN:
                color = BRIGHT_GREEN;
                darkColor = MEDIUM_GREEN;
                break;

            case COLOR::RED:
                color = BRIGHT_RED;
                darkColor = MEDB_RED;
                break;

            case COLOR::YELLOW:
                color = BRIGHT_YELLOW;
                darkColor = MEDIUM_YELLOW;
                break;

            case COLOR::WHITE:
                color = BRIGHT_WHITE;
                darkColor = OFF_WHITE192;
                break;

            default:
                color = darkColor = 0;   // something is wrong with the code, so paint it black to let the user know
                break;
            }

            // Note: we cannot use '0' for any width, or the entire area is painted.
            // Therefore, we use SafeColorFill.
            if (m_orientation == ORIENTATION::HORIZONTAL)
            {
                // horizontal                                  X            Y  width        height                
                DeltaGliderXR1::SafeColorFill(surf, color, 0, 0, brightIndex, m_sizeY);  // first part  (bright)
                DeltaGliderXR1::SafeColorFill(surf, darkColor, brightIndex, 0, (darkIndex - brightIndex), m_sizeY);  // second part (dark)
            }
            else
            {
                // vertical                                    X   Y                         width    height
                DeltaGliderXR1::SafeColorFill(surf, color, 0, (m_sizeY - brightIndex), m_sizeX, brightIndex);                // bottom (first) part: bright
                DeltaGliderXR1::SafeColorFill(surf, darkColor, 0, (m_sizeY - darkIndex), m_sizeX, (darkIndex - brightIndex));  // top (second) part: dark
            }
        }

        // invoke the post-drawing hook in case the subclass wants to overlay something
        RedrawAfterHook(event, surf);

        retVal = true;
    }

    return retVal;
}

//-------------------------------------------------------------------------

// sizeX, sizeY = size of bar
// resourceID = texture to use for bar
LargeBarArea::LargeBarArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID, int sizeX, int sizeY, int resourceID, int darkResourceID) :
    BarArea(parentPanel, panelCoordinates, areaID, sizeX, sizeY, ORIENTATION::VERTICAL),
    m_resourceID(resourceID), m_darkResourceID(darkResourceID), m_darkSurface(nullptr)
{
}

void LargeBarArea::Activate()
{
    Area::Activate();  // invoke superclass method
    m_mainSurface = CreateSurface(m_resourceID);

    if (m_darkResourceID > 0)  // dark resource is optional
        m_darkSurface = CreateSurface(m_darkResourceID);

    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(m_sizeX, m_sizeY), PANEL_REDRAW_ALWAYS, PANEL_MOUSE_IGNORE, PANEL_MAP_BGONREQUEST);

    m_lastRenderData.Reset();  // force a repaint on first call to Redraw
}

void LargeBarArea::Deactivate()
{
    if (m_darkSurface != nullptr)
        DestroySurface(&m_darkSurface);  // free our dark surface as well

    BarArea::Deactivate();           // invoke superclass method to free main surface
}

// override the base method so we will render a texture rather than a colored bar
bool LargeBarArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    bool retVal = false;

    // invoke subclass method to obtain color and indexY data for each gauge
    RENDERDATA renderData = GetRenderData();

    if (renderData != m_lastRenderData)
    {
        // remember this so we don't re-render each time
        m_lastRenderData = renderData;  // byte-for-byte copy

        // NOTE: 0 <= brightIndex <= darkIndex
        const int brightIndex = renderData.GetIndex(BARPORTION::BRIGHT);  // first part of bar
        const int darkIndex = renderData.GetIndex(BARPORTION::DARK);    // second part of bar

        // reset background
        oapiBltPanelAreaBackground(GetAreaID(), surf);

        // render portions of both textures as necessary: bright and dark
        const int brightYCoord = (m_sizeY - brightIndex);
        const int darkYCoord = (m_sizeY - darkIndex);
        //            texture        tgtX  tgtY       srcX  srcY       width    height
        DeltaGliderXR1::SafeBlt(surf, m_mainSurface, 0, brightYCoord, 0, brightYCoord, m_sizeX, brightIndex);                // bottom (first) part: bright

        if (m_darkSurface != nullptr)  // any dark surface defined?
        {
            //            texture        tgtX  tgtY       srcX  srcY       width    height
            DeltaGliderXR1::SafeBlt(surf, m_darkSurface, 0, darkYCoord, 0, darkYCoord, m_sizeX, (darkIndex - brightIndex));  // top (second) part: dark
        }

        // invoke the post-drawing hook in case the subclass wants to overlay something
        RedrawAfterHook(event, surf);

        retVal = true;
    }

    return retVal;
}


//----------------------------------------------------------------------------------

// darkResourceID: -1 = none
LargeFuelBarArea::LargeFuelBarArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID, PROPELLANT_HANDLE ph, const int resourceID, const int darkResourceID, const double gaugeMinValue) :
    LargeBarArea(parentPanel, panelCoordinates, areaID, 49, 141, resourceID, darkResourceID),   // 49 wide x 141 high
    m_maxFuelQty(-1), m_pFuelRemaining(nullptr), m_propHandle(ph), m_gaugeMinValue(gaugeMinValue)
{
}


LargeFuelBarArea::LargeFuelBarArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID, double maxFuelQty, const double* pFuelRemaining, const int resourceID, const int darkResourceID, const double gaugeMinValue) :
    LargeBarArea(parentPanel, panelCoordinates, areaID, 49, 141, resourceID, darkResourceID),   // 49 wide x 141 high, VERTICAL orientation
    m_maxFuelQty(maxFuelQty), m_pFuelRemaining(pFuelRemaining), m_propHandle(nullptr), m_gaugeMinValue(gaugeMinValue)
{
}

BarArea::RENDERDATA LargeFuelBarArea::GetRenderData()
{
    double maxPropMass;
    double totalPropMass;
    double startingDarkValue;

    if (m_propHandle != nullptr)
    {
        // propellant resource
        maxPropMass = GetXR1().GetXRPropellantMaxMass(m_propHandle);  // includes the bay qty
        totalPropMass = GetXR1().GetXRPropellantMass(m_propHandle);   // includes bay qty, if any
        startingDarkValue = GetVessel().GetPropellantMass(m_propHandle);  // any qty shown over what is currently in the INTERNAL TANK must be from the BAY
    }
    else
    {
        // non-propellant resource
        maxPropMass = m_maxFuelQty;
        totalPropMass = *m_pFuelRemaining;
        startingDarkValue = totalPropMass;  // no bay tanks for non-propellant resource
    }

    // adjust for minimum gauge value; this affects all values
    // This is currently only used for the coolant gauge
    maxPropMass -= m_gaugeMinValue;
    totalPropMass -= m_gaugeMinValue;
    startingDarkValue -= m_gaugeMinValue;

    return _RENDERDATA(COLOR::NONE, startingDarkValue, totalPropMass, maxPropMass);
}

//----------------------------------------------------------------------------------

LargeLOXBarArea::LargeLOXBarArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID, const int resourceID, const int darkResourceID) :
    LargeBarArea(parentPanel, panelCoordinates, areaID, 49, 141, resourceID, darkResourceID)   // 49 wide x 141 high
{
}

BarArea::RENDERDATA LargeLOXBarArea::GetRenderData()
{
    const double maxLoxMass = GetXR1().GetXRLOXMaxMass();  // includes the bay qty
    const double totalLoxMass = GetXR1().GetXRLOXMass();   // includes bay qty, if any
    const double startingDarkValue = GetXR1().m_loxQty;    // internal LOX tank quantity

    return _RENDERDATA(COLOR::NONE, startingDarkValue, totalLoxMass, maxLoxMass);
}
