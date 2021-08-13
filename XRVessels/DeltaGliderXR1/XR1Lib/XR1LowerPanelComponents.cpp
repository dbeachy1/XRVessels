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
// XR1LowerPanelComponents.cpp
// Handles lower panel components and associated areas.
// ==============================================================

#include "resource.h"
#include "AreaIDs.h"
#include "DeltaGliderXR1.h"

#include "XR1UpperPanelComponents.h"   // for IntervalResetButtonArea
#include "XR1MainPanelComponents.h"    // for APUFuelNumberArea
#include "XR1FuelDisplayComponent.h"   // for fuel qty areas
#include "XR1LowerPanelComponents.h"

//----------------------------------------------------------------------------------

// topLeft = top inside edge of frame, just on black screen
Interval2TimerComponent::Interval2TimerComponent(InstrumentPanel &parentPanel, COORD2 topLeft) :
    XR1Component(parentPanel, topLeft)
{
    DeltaGliderXR1 &xr1 = GetXR1();

    AddArea(new ElapsedTimerNumberArea (parentPanel, GetAbsCoords(_COORD2(  2,  1)), AID_INTERVAL2_DAYS,        xr1.m_interval2TimerRunning, 4, TimerNumberArea::DAYS,    xr1.m_interval2ElapsedTime));
    AddArea(new ElapsedTimerNumberArea (parentPanel, GetAbsCoords(_COORD2( 58,  1)), AID_INTERVAL2_HOURS,       xr1.m_interval2TimerRunning, 2, TimerNumberArea::HOURS,   xr1.m_interval2ElapsedTime));
    AddArea(new ElapsedTimerNumberArea (parentPanel, GetAbsCoords(_COORD2( 77,  1)), AID_INTERVAL2_MINUTES,     xr1.m_interval2TimerRunning, 2, TimerNumberArea::MINUTES, xr1.m_interval2ElapsedTime));
    AddArea(new ElapsedTimerNumberArea (parentPanel, GetAbsCoords(_COORD2( 96,  1)), AID_INTERVAL2_SECONDS,     xr1.m_interval2TimerRunning, 2, TimerNumberArea::SECONDS, xr1.m_interval2ElapsedTime));
    AddArea(new IntervalResetButtonArea(parentPanel, GetAbsCoords(_COORD2(125, -1)), AID_INTERVAL2_RESETBUTTON, xr1.m_interval2TimerRunning, xr1.m_interval2ElapsedTime, '2'));
}

//----------------------------------------------------------------------------------

// NOTE: FuelDumpButtonArea labels must match the labels in FuelDumpPostStep::clbkPostStep

// topLeft = top inside edge of frame, just on black screen
MainFuelGaugeComponent::MainFuelGaugeComponent(InstrumentPanel &parentPanel, COORD2 topLeft) :
    XR1Component(parentPanel, topLeft)
{
    AddArea(new LargeFuelBarArea         (parentPanel, GetAbsCoords(_COORD2(  0,  23)), AID_MAIN_FUELBAR,          GetXR1().ph_main, RES_IDB_FUEL_GAUGE, RES_IDB_FUEL_GAUGE_DARK)); 
    AddArea(new FuelRemainingKGNumberArea(parentPanel, GetAbsCoords(_COORD2(  2,   4)), AID_MAINPROPMASS_KG,       GetXR1().ph_main));
    AddArea(new FuelDumpButtonArea       (parentPanel, GetAbsCoords(_COORD2( -5, 175)), AID_MAIN_FUELDUMP_BUTTON,  GetXR1().m_mainFuelDumpInProgress, "Main"));
}

//----------------------------------------------------------------------------------

// topLeft = top inside edge of frame, just on black screen
RCSFuelGaugeComponent::RCSFuelGaugeComponent(InstrumentPanel &parentPanel, COORD2 topLeft) :
    XR1Component(parentPanel, topLeft)
{
    AddArea(new LargeFuelBarArea         (parentPanel, GetAbsCoords(_COORD2(  0,  23)), AID_RCS_FUELBAR,         GetXR1().ph_rcs, RES_IDB_FUEL_GAUGE, RES_IDB_FUEL_GAUGE_DARK));
    AddArea(new FuelRemainingKGNumberArea(parentPanel, GetAbsCoords(_COORD2(  2,   4)), AID_RCSPROPMASS_KG,      GetXR1().ph_rcs));
    AddArea(new FuelDumpButtonArea       (parentPanel, GetAbsCoords(_COORD2( -5, 175)), AID_RCS_FUELDUMP_BUTTON, GetXR1().m_rcsFuelDumpInProgress, "RCS"));
}

//----------------------------------------------------------------------------------

// topLeft = top inside edge of frame, just on black screen
SCRAMFuelGaugeComponent::SCRAMFuelGaugeComponent(InstrumentPanel &parentPanel, COORD2 topLeft) :
    XR1Component(parentPanel, topLeft)
{
    AddArea(new LargeFuelBarArea         (parentPanel, GetAbsCoords(_COORD2(  0,  23)), AID_SCRAM_FUELBAR,          GetXR1().ph_scram, RES_IDB_FUEL_GAUGE, RES_IDB_FUEL_GAUGE_DARK));
    AddArea(new FuelRemainingKGNumberArea(parentPanel, GetAbsCoords(_COORD2(  2,   4)), AID_SCRAMPROPMASS_KG,       GetXR1().ph_scram));
    AddArea(new FuelDumpButtonArea       (parentPanel, GetAbsCoords(_COORD2( -5, 175)), AID_SCRAM_FUELDUMP_BUTTON,  GetXR1().m_scramFuelDumpInProgress, "SCRAM"));
}

//----------------------------------------------------------------------------------

// topLeft = top inside edge of frame, just on black screen
APUFuelGaugeComponent::APUFuelGaugeComponent(InstrumentPanel &parentPanel, COORD2 topLeft) :
    XR1Component(parentPanel, topLeft)
{
    AddArea(new LargeFuelBarArea  (parentPanel, GetAbsCoords(_COORD2(  0,  23)), AID_APU_FUELBAR, APU_FUEL_CAPACITY, &(GetXR1().m_apuFuelQty), RES_IDB_FUEL_GAUGE, RES_IDB_FUEL_GAUGE_DARK));
    AddArea(new APUFuelNumberArea (parentPanel, GetAbsCoords(_COORD2( 16,   4)), AID_APU_FUEL_TEXT));
    AddArea(new FuelDumpButtonArea(parentPanel, GetAbsCoords(_COORD2( -5, 175)), AID_APU_FUELDUMP_BUTTON,  GetXR1().m_apuFuelDumpInProgress, "APU"));
}

//----------------------------------------------------------------------------------

// topLeft = top-left of switch
FuelHatchComponent::FuelHatchComponent(InstrumentPanel &parentPanel, COORD2 topLeft) :
    XR1Component(parentPanel, topLeft)
{
    AddArea(new SupplyHatchToggleSwitchArea(parentPanel, GetAbsCoords(_COORD2( 0,  0)), AID_FUELHATCHSWITCH, AID_FUELHATCHLED, GetXR1().fuelhatch_status, "Fuel", GetXR1().anim_fuelhatch));
    AddArea(new DoorMediumLEDArea          (parentPanel, GetAbsCoords(_COORD2(-1, 56)), AID_FUELHATCHLED, GetXR1().fuelhatch_status));
}

//----------------------------------------------------------------------------------

// topLeft = top-left of switch
LoxHatchComponent::LoxHatchComponent(InstrumentPanel &parentPanel, COORD2 topLeft) :
    XR1Component(parentPanel, topLeft)
{
    AddArea(new SupplyHatchToggleSwitchArea(parentPanel, GetAbsCoords(_COORD2( 0,  0)), AID_LOXHATCHSWITCH, AID_LOXHATCHLED, GetXR1().loxhatch_status, "LOX", GetXR1().anim_loxhatch));
    AddArea(new DoorMediumLEDArea          (parentPanel, GetAbsCoords(_COORD2(-1, 56)), AID_LOXHATCHLED, GetXR1().loxhatch_status));
}

//----------------------------------------------------------------------------------

// topLeft = top inside edge of frame, just on black screen
MainSupplyLineGaugeComponent::MainSupplyLineGaugeComponent(InstrumentPanel &parentPanel, COORD2 topLeft) :
    XR1Component(parentPanel, topLeft)
{
    const double limit = MAIN_SUPPLY_PSI_LIMIT;  // in psi
    AddArea(new SupplyLinePressureNumberArea(parentPanel, GetAbsCoords(_COORD2( 4,   4)), AID_MAINSUPPLYLINE_PSI, limit, GetXR1().m_mainExtLinePressure));
    AddArea(new SupplyLinePressureGaugeArea (parentPanel, GetAbsCoords(_COORD2(21,  17)), AID_MAINSUPPLYLINE_GAUGE, limit, GetXR1().m_mainExtLinePressure));
    AddArea(new SupplyLineMediumLEDArea     (parentPanel, GetAbsCoords(_COORD2( 2, 102)), AID_MAINSUPPLYLINE_LED, GetXR1().m_mainSupplyLineStatus));
}   

//----------------------------------------------------------------------------------

// topLeft = top inside edge of frame, just on black screen
ScramSupplyLineGaugeComponent::ScramSupplyLineGaugeComponent(InstrumentPanel &parentPanel, COORD2 topLeft) :
    XR1Component(parentPanel, topLeft)
{
    const double limit = SCRAM_SUPPLY_PSI_LIMIT;
    AddArea(new SupplyLinePressureNumberArea(parentPanel, GetAbsCoords(_COORD2( 4,   4)), AID_SCRAMSUPPLYLINE_PSI, limit, GetXR1().m_scramExtLinePressure));
    AddArea(new SupplyLinePressureGaugeArea (parentPanel, GetAbsCoords(_COORD2(21,  17)), AID_SCRAMSUPPLYLINE_GAUGE, limit, GetXR1().m_scramExtLinePressure));
    AddArea(new SupplyLineMediumLEDArea     (parentPanel, GetAbsCoords(_COORD2( 2, 102)), AID_SCRAMSUPPLYLINE_LED, GetXR1().m_scramSupplyLineStatus));
}   

//----------------------------------------------------------------------------------

// topLeft = top inside edge of frame, just on black screen
ApuSupplyLineGaugeComponent::ApuSupplyLineGaugeComponent(InstrumentPanel &parentPanel, COORD2 topLeft) :
    XR1Component(parentPanel, topLeft)
{
    const double limit = APU_SUPPLY_PSI_LIMIT;  // in psi
    AddArea(new SupplyLinePressureNumberArea(parentPanel, GetAbsCoords(_COORD2( 4,   4)), AID_APUSUPPLYLINE_PSI, limit, GetXR1().m_apuExtLinePressure));
    AddArea(new SupplyLinePressureGaugeArea (parentPanel, GetAbsCoords(_COORD2(21,  17)), AID_APUSUPPLYLINE_GAUGE, limit, GetXR1().m_apuExtLinePressure));
    AddArea(new SupplyLineMediumLEDArea     (parentPanel, GetAbsCoords(_COORD2( 2, 102)), AID_APUSUPPLYLINE_LED, GetXR1().m_apuSupplyLineStatus));    
}   

//----------------------------------------------------------------------------------

// topLeft = top inside edge of frame, just on black screen
LoxSupplyLineGaugeComponent::LoxSupplyLineGaugeComponent(InstrumentPanel &parentPanel, COORD2 topLeft) :
    XR1Component(parentPanel, topLeft)
{
    const double limit = LOX_SUPPLY_PSI_LIMIT;  // in psi
    AddArea(new SupplyLinePressureNumberArea(parentPanel, GetAbsCoords(_COORD2( 4,   4)), AID_LOXSUPPLYLINE_PSI, limit, GetXR1().m_loxExtLinePressure));
    AddArea(new SupplyLinePressureGaugeArea (parentPanel, GetAbsCoords(_COORD2(21,  17)), AID_LOXSUPPLYLINE_GAUGE, limit, GetXR1().m_loxExtLinePressure));
    AddArea(new SupplyLineMediumLEDArea     (parentPanel, GetAbsCoords(_COORD2( 2, 102)), AID_LOXSUPPLYLINE_LED, GetXR1().m_loxSupplyLineStatus));
}

//----------------------------------------------------------------------------------

// topLeft = top inside edge of frame, just on black screen
ShipMassDisplayComponent::ShipMassDisplayComponent(InstrumentPanel &parentPanel, COORD2 topLeft) :
    XR1Component(parentPanel, topLeft)
{
    AddArea(new ShipMassNumberArea(parentPanel, GetAbsCoords(_COORD2(18,  2)), AID_SHIPMASS_LB, false));  // pounds
    AddArea(new ShipMassNumberArea(parentPanel, GetAbsCoords(_COORD2(18, 15)), AID_SHIPMASS_KG, true));   // kg
}

//----------------------------------------------------------------------------------

// topLeft = top inside edge of frame, just on black screen
LoxGaugeComponent::LoxGaugeComponent(InstrumentPanel &parentPanel, COORD2 topLeft) :
    XR1Component(parentPanel, topLeft)
{
    AddArea(new LargeLOXBarArea  (parentPanel, GetAbsCoords(_COORD2(  0,  23)), AID_LOX_BAR, RES_IDB_LOX_GAUGE, RES_IDB_LOX_GAUGE_DARK));
    AddArea(new LoxNumberArea    (parentPanel, GetAbsCoords(_COORD2(  2,   4)), AID_LOX_TEXT));
    AddArea(new LoxDumpButtonArea(parentPanel, GetAbsCoords(_COORD2( -5, 175)), AID_LOX_DUMP_BUTTON));
}

//----------------------------------------------------------------------------------

// topLeft = top inside edge of frame, just on black screen
OxygenRemainingPanelComponent::OxygenRemainingPanelComponent(InstrumentPanel &parentPanel, COORD2 topLeft) :
    XR1Component(parentPanel, topLeft)
{
    AddArea(new OxygenRemainingPctNumberArea(parentPanel, GetAbsCoords(_COORD2(  21,  2)), AID_OXYGEN_REM_PCT));
    
    const DeltaGliderXR1 &xr1 = GetXR1();
    AddArea(new OxygenRemainingTimerNumberArea(parentPanel, GetAbsCoords(_COORD2(  2,  15)), AID_OXYGEN_REM_DAYS,        4, TimerNumberArea::DAYS));
    AddArea(new OxygenRemainingTimerNumberArea(parentPanel, GetAbsCoords(_COORD2( 59,  15)), AID_OXYGEN_REM_HOURS,       2, TimerNumberArea::HOURS));
    AddArea(new OxygenRemainingTimerNumberArea(parentPanel, GetAbsCoords(_COORD2( 78,  15)), AID_OXYGEN_REM_MINUTES,     2, TimerNumberArea::MINUTES));
    AddArea(new OxygenRemainingTimerNumberArea(parentPanel, GetAbsCoords(_COORD2( 97,  15)), AID_OXYGEN_REM_SECONDS,     2, TimerNumberArea::SECONDS));

    AddArea(new CrewMembersNumberArea(parentPanel, GetAbsCoords(_COORD2( 85, 28)), AID_CREW_MEMBERS_TEXT));
    AddArea(new CabinO2PctNumberArea (parentPanel, GetAbsCoords(_COORD2( 78, 41)), AID_CABIN_O2_PCT));
}

//----------------------------------------------------------------------------------

// topLeft = top inside edge of frame, just on black screen
CoolantGaugeComponent::CoolantGaugeComponent(InstrumentPanel &parentPanel, COORD2 topLeft) :
    XR1Component(parentPanel, topLeft)
{
    AddArea(new LargeFuelBarArea (parentPanel, GetAbsCoords(_COORD2(  0,  23)), AID_COOLANT_BAR, MAX_COOLANT_GAUGE_TEMP, &(GetXR1().m_coolantTemp), RES_IDB_COOLANT_GAUGE, -1, MIN_COOLANT_GAUGE_TEMP));
    AddArea(new CoolantNumberArea(parentPanel, GetAbsCoords(_COORD2(  6,   4)), AID_COOLANT_TEXT));
    AddArea(new DoorMediumLEDArea(parentPanel, GetAbsCoords(_COORD2( -1,  56)), AID_FUELHATCHLED, GetXR1().fuelhatch_status));
}

//----------------------------------------------------------------------------------

// topLeft = top-left of switch
ExternalCoolingComponent::ExternalCoolingComponent(InstrumentPanel &parentPanel, COORD2 topLeft) :
    XR1Component(parentPanel, topLeft)
{
    AddArea(new ExternalCoolingSwitchArea(parentPanel, GetAbsCoords(_COORD2( 0,  0)), AID_EXTERNAL_COOLING_SWITCH, AID_EXTERNAL_COOLING_LED));
    AddArea(new DoorMediumLEDArea        (parentPanel, GetAbsCoords(_COORD2(-1, 56)), AID_EXTERNAL_COOLING_LED, GetXR1().externalcooling_status));
}


//-------------------------------------------------------------------------
// Areas
//-------------------------------------------------------------------------

SupplyLinePressureNumberArea::SupplyLinePressureNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, double limit, double &pressure) :
    NumberArea(parentPanel, panelCoordinates, areaID, 3, true),   // 3 chars plus decimal
    m_limit(limit), m_pressure(pressure)
{
}

bool SupplyLinePressureNumberArea::UpdateRenderData(RENDERDATA &renderData)
{
    bool redraw = false;
    const bool forceRedraw = renderData.forceRedraw;

     // round to nearest 1/10th
    double pressure = (static_cast<double>(static_cast<int>((m_pressure   + 0.05) * 10.0))) / 10.0;
    
    // check whether the value has changed since the last render
    if (forceRedraw || (pressure != renderData.value))
    {
        // Value has changed -- let's redo the string and see if that is different as well.
        // The goal here is to be as efficient as possible and only re-render when we absolutely have to.
        char pTemp[8];
        // ensure that value is in range
        if (pressure > 99.9)
            pressure = 99.9;   // trim to 3 leading digits
        else if (pressure < 0)
            pressure = 0;

        sprintf(pTemp, "%4.1f", pressure);   // 3 chars + the decimal = length 4
        if (forceRedraw || (strcmp(pTemp, renderData.pStrToRender) != 0))
        {
            // text has changed; signal the base class to render it
            renderData.value = pressure;   // remember for next time
            strcpy(renderData.pStrToRender, pTemp);
            redraw = true;
            renderData.forceRedraw = false;  // clear reset request
        }
    }

    // set font color based on pressure levels
    // calibrated to match colors on gauge
    const double redLevel = m_limit * 0.94;
    const double yellowLevel = m_limit * 0.79;    
    if (pressure >= redLevel)
        renderData.color = RED;
    else if (pressure >= yellowLevel)
        renderData.color = YELLOW;
    else
        renderData.color = GREEN;

    return redraw;
}

//----------------------------------------------------------------------------------

SupplyLinePressureGaugeArea::SupplyLinePressureGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, double limit, double &pressure) :
    VerticalGaugeArea(parentPanel, panelCoordinates, areaID, false, 73, PANEL_REDRAW_ALWAYS),  // single gauge 73 pixels high
    m_limit(limit), m_pressure(pressure)
{
}

VerticalGaugeArea::RENDERDATA SupplyLinePressureGaugeArea::GetRenderData(const SIDE side)
{
    double frac = min(m_pressure / m_limit, 1.0); // gauge movement, 0...1
    int p = 66 - static_cast<int>((frac * 66) + 0.5);   // round to nearest pixel

    return _RENDERDATA(GREEN, p);  
}

//----------------------------------------------------------------------------------

// lightStatus = reference to status variable: true = light on, false = light off
SupplyLineMediumLEDArea::SupplyLineMediumLEDArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, bool &lightStatus) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_lightStatus(lightStatus), m_lastRenderedState(lightStatus)
{
}

void SupplyLineMediumLEDArea::Activate()
{
    Area::Activate();  // invoke superclass method
    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(29, 21), PANEL_REDRAW_ALWAYS, PANEL_MOUSE_IGNORE, PANEL_MAP_BACKGROUND);
    m_mainSurface = CreateSurface(IDB_GREEN_LED_SMALL);

    TriggerRedraw();    // render initial setting
}

bool SupplyLineMediumLEDArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    bool retVal = false;

    if ((event == PANEL_REDRAW_INIT) || (m_lastRenderedState != m_lightStatus))
    {
        int srcX = (m_lightStatus ? 29 : 0);
        oapiBlt(surf, m_mainSurface, 0, 0, srcX, 0, 29, 21);    
        m_lastRenderedState = m_lightStatus;      // remember what we rendered
        retVal = true;
    }

    return retVal;
}

//----------------------------------------------------------------------------------

// NOTE: LOX dump status will NOT be preserved in the save file; we never want to boot up and resume dumping LOX automatically
LoxDumpButtonArea::LoxDumpButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_isLit(false), m_buttonDownSimt(-1), 
    m_buttonPressProcessed(false), m_isButtonDown(false)
{
}

void LoxDumpButtonArea::Activate()
{
    Area::Activate();  // invoke superclass method
    m_mainSurface = CreateSurface(IDB_LIGHT2);                  // lighted green button

    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(12, 12), PANEL_REDRAW_MOUSE, PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBPRESSED | PANEL_MOUSE_LBUP);

    // reset to NOT lit
    m_isLit = false;
    
    TriggerRedraw();
}

bool LoxDumpButtonArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    // always re-render this since it is always performed on request
    int srcX = (m_isLit ? 12 : 0);    // texture X coord; 12 = lit, 0 = not lit
    oapiBlt(surf, m_mainSurface, 0, 0, srcX, 0, 12, 12);

    return true;
}

bool LoxDumpButtonArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    // check whether button was just pressed
    if (event & PANEL_MOUSE_LBDOWN)
    {
        m_buttonDownSimt = GetAbsoluteSimTime();    
        m_isLit = true;
        m_isButtonDown = true;
    }

    // main processing is here
    ProcessButtonPressed(event);

    // check whether button was just unpressed
    if (event & PANEL_MOUSE_LBUP)
    {
        m_buttonDownSimt = -1;
        
        // do not turn off button light here; our PostStep manages that

        m_isButtonDown = false;     // reset
    }

    return true;
}

void LoxDumpButtonArea::ProcessButtonPressed(const int event)
{
    if (event & (PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBUP))
        GetXR1().PlaySound(GetXR1().SwitchOn, DeltaGliderXR1::ST_Other, QUIET_CLICK);  // light click for both on and off

    if (event & PANEL_MOUSE_LBDOWN)
    {
        m_buttonPressProcessed = false;     // reset for this new press

        // if LOX consumption set to zero, cannot dump LOX
        if (GetXR1().GetXR1Config()->GetLOXConsumptionFraction() == 0.0)
        {
            GetXR1().ShowWarning("LOX Consumption Disabled.wav", DeltaGliderXR1::ST_WarningCallout, "Cannot dump LOX when&LOX consumption disabled.");

            // nothing more to do for this press
            m_buttonPressProcessed = true;  
        }

        if (GetXR1().m_loxDumpInProgress)
        {
            GetXR1().SetLOXDumpState(false);
            // nothing more to do for this press
            m_buttonPressProcessed = true;  
            return;     
        }
    }

    if (m_buttonPressProcessed)
        return;     // ignore this event; button press already processed

    const double RESET_TIME = 2.5;      // button must be held this long to initiate LOX dump
    const double buttonHoldTime = GetAbsoluteSimTime() - m_buttonDownSimt;

    if (event & PANEL_MOUSE_LBPRESSED)
    {
        if (buttonHoldTime >= RESET_TIME)
        {
            GetXR1().SetLOXDumpState(true);  // start the LOX dump
            m_buttonPressProcessed = true;   // ignore any further events
        }
    }
    else    // button was released before fuel dump was initiated
    {
        GetXR1().ShowWarning("Hold to Dump LOX.wav", DeltaGliderXR1::ST_WarningCallout, "You must hold down the dump&button to initiate LOX dump.");
        m_buttonPressProcessed = true;
    }
}

void LoxDumpButtonArea::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    if (GetXR1().m_loxDumpInProgress)
    {
        // blink the light twice a second
        bool isLit = (fmod(simt, 0.5) < 0.25);

        if (isLit != m_isLit)
        {
            m_isLit = isLit;
            TriggerRedraw();
        }
    }
    else    // dump is NOT in progress; turn off the light if it is lit UNLESS button is down
    {
        if (m_isLit && (m_isButtonDown == false))
        {
            m_isLit = false;
            TriggerRedraw();
        }
    }
}

//-------------------------------------------------------------------------

LoxNumberArea::LoxNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    NumberArea(parentPanel, panelCoordinates, areaID, 6, true)   // 6 chars plus decimal
{
}

bool LoxNumberArea::UpdateRenderData(RENDERDATA &renderData)
{
    bool redraw = false;
    const bool forceRedraw = renderData.forceRedraw;

    double currentLoxMass = GetXR1().GetXRLOXMass();  // takes payload LOX into account as well

     // round to nearest 1/10th
    currentLoxMass = (static_cast<double>(static_cast<int>((currentLoxMass + 0.05) * 10.0))) / 10.0;
    
    // check whether the value has changed since the last render
    if (forceRedraw || (currentLoxMass != renderData.value))
    {
        // Value has changed -- let's redo the string and see if that is different as well.
        // The goal here is to be as efficient as possible and only re-render when we absolutely have to.
        char pTemp[12];
        // ensure that value is in range
        if (currentLoxMass > 99999.9)
            currentLoxMass = 99999.9;   
        else if (currentLoxMass < -99999.9)
            currentLoxMass = -99999.9;
        sprintf(pTemp, "%7.1f", currentLoxMass); 
        if (forceRedraw || (strcmp(pTemp, renderData.pStrToRender) != 0))
        {
            // text has changed; signal the base class to render it
            renderData.value = currentLoxMass;   // remember for next time
            strcpy(renderData.pStrToRender, pTemp);
            redraw = true;
            renderData.forceRedraw = false;  // clear reset request
        }
    }

    // always render in BLUE 
    renderData.color = BLUE;

    return redraw;
}

//-------------------------------------------------------------------------

OxygenRemainingPctNumberArea::OxygenRemainingPctNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    NumberArea(parentPanel, panelCoordinates, areaID, 8, true)   // 8 chars plus decimal
{
}

bool OxygenRemainingPctNumberArea::UpdateRenderData(RENDERDATA &renderData)
{
    bool redraw = false;
    const bool forceRedraw = renderData.forceRedraw;

    const double fracRemaining = GetXR1().GetXRLOXMass() / GetXR1().GetXRLOXMaxMass();   // 0...1
    const double pctRemaining = fracRemaining * 100;

    // do not round value
    
    // check whether the value has changed since the last render
    if (forceRedraw || (pctRemaining != renderData.value))
    {
        // Value has changed -- let's redo the string and see if that is different as well.
        // The goal here is to be as efficient as possible and only re-render when we absolutely have to.
        char pTemp[15];

        sprintf(pTemp, "%9.5lf", pctRemaining);
        if (forceRedraw || (strcmp(pTemp, renderData.pStrToRender) != 0))
        {
            // text has changed; signal the base class to render it
            renderData.value = pctRemaining;   // remember for next time
            strcpy(renderData.pStrToRender, pTemp);
            redraw = true;
            renderData.forceRedraw = false;  // clear reset request
        }
    }

    // render in blue unless oxygen depleted
    renderData.color = ((pctRemaining <= 0.0) ? RED : BLUE);

    return redraw;
}

//-------------------------------------------------------------------------

OxygenRemainingTimerNumberArea::OxygenRemainingTimerNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int sizeInChars, TIMEUNITS timeUnits) :
    TimerNumberArea(parentPanel, panelCoordinates, areaID, sizeInChars, timeUnits, BLUE)
{
}

// returns: elapsed time in days
double OxygenRemainingTimerNumberArea::GetTime()
{
    const double oxygenRemainingInDays = GetXR1().m_oxygenRemainingTime / 86400;

    // Note: this value is range-checked by our base class, so no need to do it here.

    return oxygenRemainingInDays;
}

//-------------------------------------------------------------------------

CrewMembersNumberArea::CrewMembersNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    NumberArea(parentPanel, panelCoordinates, areaID, 2, false)   // 2 chars, no decimal
{
}

bool CrewMembersNumberArea::UpdateRenderData(RENDERDATA &renderData)
{
    bool redraw = false;
    const bool forceRedraw = renderData.forceRedraw;

    const int crewMembers = GetXR1().GetCrewMembersCount();

    // check whether the value has changed since the last render
    if (forceRedraw || (crewMembers != renderData.value))
    {
        char pTemp[3];

        // since we are an integer value, the string will always be different here from the previous render
        sprintf(pTemp, "%-2d", crewMembers);  // left-align
        
        // signal the base class to render the new value
        renderData.value = crewMembers;   // remember for next time
        strcpy(renderData.pStrToRender, pTemp);
        redraw = true;
        renderData.forceRedraw = false;  // clear reset request
    }

    // render in BLUE
    renderData.color = BLUE;

    return redraw;
}

//-------------------------------------------------------------------------

CabinO2PctNumberArea::CabinO2PctNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    NumberArea(parentPanel, panelCoordinates, areaID, 3, true)   // 3 chars plus decimal
{
}

bool CabinO2PctNumberArea::UpdateRenderData(RENDERDATA &renderData)
{
    bool redraw = false;
    const bool forceRedraw = renderData.forceRedraw;

    double o2Pct = GetXR1().m_cabinO2Level * 100;

    // round to nearest tenth
    o2Pct = (static_cast<double>(static_cast<int>((o2Pct   + 0.05) * 10))) / 10;    

    // check whether the value has changed since the last render
    if (forceRedraw || (o2Pct != renderData.value))
    {
        // Value has changed -- let's redo the string and see if that is different as well.
        // The goal here is to be as efficient as possible and only re-render when we absolutely have to.
        char pTemp[10];

        sprintf(pTemp, "%4.1lf", o2Pct);
        if (forceRedraw || (strcmp(pTemp, renderData.pStrToRender) != 0))
        {
            // text has changed; signal the base class to render it
            renderData.value = o2Pct;   // remember for next time
            strcpy(renderData.pStrToRender, pTemp);
            redraw = true;
            renderData.forceRedraw = false;  // clear reset request
        }
    }

    // render in blue if o2Level OK, or red if crew is incapacitated or dead
    // NOTE: do not invoke IsCrewIncapacitatedOrNoPilotOnBoard here, since that also checks whether all the crew are outside the ship
    renderData.color = (((GetXR1().m_crewState == INCAPACITATED) || (GetXR1().m_crewState == DEAD)) ? RED : BLUE);

    return redraw;
}

//-------------------------------------------------------------------------

CoolantNumberArea::CoolantNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    NumberArea(parentPanel, panelCoordinates, areaID, 5, true)   // 5 chars plus decimal
{
}

bool CoolantNumberArea::UpdateRenderData(RENDERDATA &renderData)
{
    bool redraw = false;
    const bool forceRedraw = renderData.forceRedraw;

    double coolantTemp = GetXR1().m_coolantTemp;

     // round to nearest 1/100th
    coolantTemp = (static_cast<double>((static_cast<int>((coolantTemp + 0.005) * 100.0)))) / 100.0;
    
    // check whether the value has changed since the last render
    if (forceRedraw || (coolantTemp != renderData.value))
    {
        // Value has changed -- let's redo the string and see if that is different as well.
        // The goal here is to be as efficient as possible and only re-render when we absolutely have to.
        char pTemp[12];
        // ensure that value is in range
        if (coolantTemp > 999.99)
            coolantTemp = 999.99;   
        else if (coolantTemp < -999.99)
            coolantTemp = -999.99;
        sprintf(pTemp, "%6.2f", coolantTemp); 
        if (forceRedraw || (strcmp(pTemp, renderData.pStrToRender) != 0))
        {
            // text has changed; signal the base class to render it
            renderData.value = coolantTemp;   // remember for next time
            strcpy(renderData.pStrToRender, pTemp);
            redraw = true;
            renderData.forceRedraw = false;  // clear reset request
        }

        // render color is based on temperature
        if (coolantTemp > CRITICAL_COOLANT_TEMP)
            renderData.color = RED;
        else if (coolantTemp > WARN_COOLANT_TEMP)
            renderData.color = YELLOW;
        else    // temperature normal
            renderData.color = GREEN;
    }

    return redraw;
}

//----------------------------------------------------------------------------------

ExternalCoolingSwitchArea::ExternalCoolingSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID) :
    ToggleSwitchArea(parentPanel, panelCoordinates, areaID, indicatorAreaID)
{
}

bool ExternalCoolingSwitchArea::ProcessSwitchEvent(bool switchIsOn)
{
    // must delegate call to the main class so XRVesselCtrl 3.0 APIs can call it, too
    return GetXR1().RequestExternalCooling(switchIsOn);
}

bool ExternalCoolingSwitchArea::isOn()
{
    return (GetXR1().externalcooling_status == DOOR_OPEN);
}
