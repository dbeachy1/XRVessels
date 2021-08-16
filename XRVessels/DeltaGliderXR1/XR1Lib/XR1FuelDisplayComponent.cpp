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
// XR1FuelDisplayComponent.cpp
// Handles main, hover, and scram throttle controls
// ==============================================================

#include "DeltaGliderXR1.h"
#include "XR1FuelDisplayComponent.h"
#include "AreaIDs.h"

// topLeft = top-left corner @ inside edge of screen
FuelDisplayComponent::FuelDisplayComponent(InstrumentPanel &parentPanel, COORD2 topLeft) :
    XR1Component(parentPanel, topLeft)
{
    // PCT remaining bars
    AddArea(new FuelRemainingBarArea(parentPanel, GetAbsCoords(_COORD2( 34, 23)), AID_MAINPROPMASS_BAR,  GetXR1().ph_main));
    AddArea(new FuelRemainingBarArea(parentPanel, GetAbsCoords(_COORD2( 34, 35)), AID_RCSPROPMASS_BAR,   GetXR1().ph_rcs));
    AddArea(new FuelRemainingBarArea(parentPanel, GetAbsCoords(_COORD2( 34, 47)), AID_SCRAMPROPMASS_BAR, GetXR1().ph_scram));

    // PCT remaining digits
    AddArea(new FuelRemainingPCTNumberArea(parentPanel, GetAbsCoords(_COORD2(121, 21)), AID_MAINPROPMASS_PCT,  GetXR1().ph_main));
    AddArea(new FuelRemainingPCTNumberArea(parentPanel, GetAbsCoords(_COORD2(121, 33)), AID_RCSPROPMASS_PCT,   GetXR1().ph_rcs));
    AddArea(new FuelRemainingPCTNumberArea(parentPanel, GetAbsCoords(_COORD2(121, 45)), AID_SCRAMPROPMASS_PCT, GetXR1().ph_scram));

    // KG remaining digits
    // figure out whether to use large-capacity tanks or not
    AddArea(new FuelRemainingKGNumberArea(parentPanel, GetAbsCoords(_COORD2(162, 21)), AID_MAINPROPMASS_KG,  GetXR1().ph_main));
    AddArea(new FuelRemainingKGNumberArea(parentPanel, GetAbsCoords(_COORD2(162, 33)), AID_RCSPROPMASS_KG,   GetXR1().ph_rcs));
    AddArea(new FuelRemainingKGNumberArea(parentPanel, GetAbsCoords(_COORD2(162, 45)), AID_SCRAMPROPMASS_KG, GetXR1().ph_scram));
}   

//-------------------------------------------------------------------------

// used for all three fuel remaining bars
FuelRemainingBarArea::FuelRemainingBarArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, PROPELLANT_HANDLE ph) :
    BarArea(parentPanel, panelCoordinates, areaID, 85, 7, ORIENTATION::HORIZONTAL),  // 84 pixels wide, inclusive (so we add 1)
    m_propHandle(ph)
{
}

BarArea::RENDERDATA FuelRemainingBarArea::GetRenderData()
{
    const double maxPropMass = GetXR1().GetXRPropellantMaxMass(m_propHandle);  // includes the bay qty
    const double totalPropMass = GetXR1().GetXRPropellantMass(m_propHandle);   // includes bay qty, if any
    const double startingDarkValue = GetVessel().GetPropellantMass(m_propHandle);  // any qty shown over what is currently in the INTERNAL TANK must be from the BAY

    return _RENDERDATA(COLOR::GREEN, startingDarkValue, totalPropMass, maxPropMass);
}

//-------------------------------------------------------------------------

// used for all three fuel remaining PCT lines
FuelRemainingPCTNumberArea::FuelRemainingPCTNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, PROPELLANT_HANDLE ph) :
    NumberArea(parentPanel, panelCoordinates, areaID, 4, true),   // 4 chars plus decimal
    m_propHandle(ph)
{
}

bool FuelRemainingPCTNumberArea::UpdateRenderData(RENDERDATA &renderData)
{
    bool redraw = false;
    const bool forceRedraw = renderData.forceRedraw;

    // check whether the value has changed since the last render
    double currentPropMassPct = SAFE_FRACTION(GetXR1().GetXRPropellantMass(m_propHandle), GetXR1().GetXRPropellantMaxMass(m_propHandle)) * 100;

    // round to nearest 1/10th
    currentPropMassPct = (static_cast<double>(static_cast<int>((currentPropMassPct + 0.05) * 10.0))) / 10.0;

    if (forceRedraw || (currentPropMassPct != renderData.value))
    {
        // Value has changed -- let's redo the string and see if that is different as well.
        // The goal here is to be as efficient as possible and only re-render when we absolutely have to.
        char pTemp[10];
        // ensure that value is in range
        if (currentPropMassPct > 999.9)
            currentPropMassPct = 999.9;   // trim to 3 leading digits
        else if (currentPropMassPct < -999.9)
            currentPropMassPct = -999.9;

        sprintf(pTemp, "%5.1f", currentPropMassPct);   // 4 chars + decimal = 5 chars total
        if (forceRedraw || (strcmp(pTemp, renderData.pStrToRender) != 0))
        {
            // text has changed; signal the base class to render it
            renderData.value = currentPropMassPct;   // remember for next time
            strcpy(renderData.pStrToRender, pTemp);
            redraw = true;
            renderData.forceRedraw = false;  // clear reset request
        }
    }

    return redraw;
}

//-------------------------------------------------------------------------

// used for all three fuel remaining KG lines
// Automatically adjusts for max fuel levels
// largeTank: true = don't show 1/10th units
FuelRemainingKGNumberArea::FuelRemainingKGNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, PROPELLANT_HANDLE ph) :
    NumberArea(parentPanel, panelCoordinates, areaID, 6, true),   // 6 chars plus decimal (e.g., 230000. or 10400.0)
    m_propHandle(ph)
{
}

bool FuelRemainingKGNumberArea::UpdateRenderData(RENDERDATA &renderData)
{
    bool redraw = false;
    const bool forceRedraw = renderData.forceRedraw;

    double currentPropMass = GetXR1().GetXRPropellantMass(m_propHandle);

     // round to nearest 1/10th
    currentPropMass = (static_cast<double>((static_cast<int>(((currentPropMass + 0.05)) * 10.0)))) / 10.0;
    
    // check whether the value has changed since the last render
    if (forceRedraw || (currentPropMass != renderData.value))
    {
        // Value has changed -- let's redo the string and see if that is different as well.
        // The goal here is to be as efficient as possible and only re-render when we absolutely have to.
        char pTemp[12];
        // ensure that value is in range
        if (currentPropMass > 999999)
            currentPropMass = 999999;   
        else if (currentPropMass < 0)   // sanity-check
            currentPropMass = 0;  

        sprintf(pTemp, ((currentPropMass > 99999.9) ? "%6.0f." : "%7.1f"), currentPropMass);  // use trailing dot to pad the display area
        if (forceRedraw || (strcmp(pTemp, renderData.pStrToRender) != 0))
        {
            // text has changed; signal the base class to render it
            renderData.value = currentPropMass;   // remember for next time
            strcpy(renderData.pStrToRender, pTemp);
            redraw = true;
            renderData.forceRedraw = false;  // clear reset request
        }
    }

    return redraw;
}
