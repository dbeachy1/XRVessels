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
// XR1MainPanelAreas.cpp
// Handles non-component 2D and 2D/3D shared main panel areas
// ==============================================================

#include "resource.h"
#include "AreaIDs.h"

#include "DeltaGliderXR1.h"
#include "XR1MainPanelAreas.h"

//----------------------------------------------------------------------------------

//
// Constructor
// vessel = our vessel handle
// panelCoordinates = absolute coordinates of this area on the parent instrument panel
// areaID = unique Orbiter area ID
HudModeButtonsArea::HudModeButtonsArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    XR1Area(parentPanel, panelCoordinates, areaID)
{
}

// Activate this area
void HudModeButtonsArea::Activate()
{
    Area::Activate();  // invoke superclass method
    // register area
    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(107, 15), PANEL_REDRAW_USER, PANEL_MOUSE_LBDOWN | PANEL_MOUSE_ONREPLAY, PANEL_MAP_BACKGROUND);

    // load surface(s); most areas only have one surface
    m_mainSurface = CreateSurface(IDB_LIGHT1);     // HUD mode LED at top-left

    // NOTE: if you activate any additional surfaces you must override the Deactivate() method and free them up.  
    // The default Deactivate() method only frees m_mainSurface.
}

// Redraw this area
// event = Orbiter event flags
// returns: true if area redrawn, false if not
bool HudModeButtonsArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    int mode = oapiGetHUDMode();
    if (mode > HUD_NONE)
        DeltaGliderXR1::SafeBlt (surf, m_mainSurface, mode*29+6, 0, 7, 0, 7, 7);

    return true;
}

// Handle mouse events for this area
// event = Orbiter event flags
// mx, my = mouse coordinates relative to the area
// returns: true if event processed, false if not
bool HudModeButtonsArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    if (mx%29 < 20)
    {
        oapiSetHUDMode (HUD_NONE+(mx/29));
        
        // play sound
        GetXR1().PlaySound(GetXR1().SwitchOn, DeltaGliderXR1::ST_Other, QUIET_CLICK);    // light click
    }
        
    return true;
}

//----------------------------------------------------------------------------------

ElevatorTrimArea::ElevatorTrimArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID) :
    XR1Area(parentPanel, panelCoordinates, areaID, meshTextureID)
{
}

void ElevatorTrimArea::Activate()
{
    Area::Activate();  // invoke superclass method
    if (IsVC()) // 3D panel?
    {
        oapiVCRegisterArea(GetAreaID(), GetRectForSize(3, 52), PANEL_REDRAW_ALWAYS, PANEL_MOUSE_LBPRESSED, PANEL_MAP_NONE, GetVCPanelTextureHandle());
    }
    else    // 2D panel
    {
        oapiRegisterPanelArea(GetAreaID(), GetRectForSize(16, 52), PANEL_REDRAW_ALWAYS, PANEL_MOUSE_LBPRESSED);
        m_mainSurface = CreateSurface(IDB_LIGHT1);     // HUD mode LED at top-left (2D mode only for now)
    }

    // reset state variables to force a repaint
    m_elevTrimPos = -1;
}

bool ElevatorTrimArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    double level = GetVessel().GetControlSurfaceLevel(AIRCTRL_ELEVATORTRIM);
    UINT pos = (UINT)((1.0+level)*23.0);
    
    if (pos != m_elevTrimPos) // has trim moved since last redraw?
    {
        const int w = 15;  // 2D width
        oapiColourFill (surf, 0);  // repaint to black
        oapiColourFill (surf, oapiGetColour (210,210,210), 1, pos, w, 6);
        m_elevTrimPos = pos;    // save update location
        return true;
    }
    
    return false;
}

// Note: this is identical to Redraw2D except for the width of the second ColourFill (const int w = X)
// This is necessary in order to keep the VC code completely separate from 2D code.
bool ElevatorTrimArea::Redraw3D(const int event, const SURFHANDLE surf)
{
    double level = GetVessel().GetControlSurfaceLevel(AIRCTRL_ELEVATORTRIM);
    UINT pos = (UINT)((1.0+level)*23.0);
    
    if (pos != m_elevTrimPos) // has trim moved since last redraw?
    {
        const int w = 2;  // 3D width
        oapiColourFill (surf, 0);  // repaint to black
        oapiColourFill (surf, oapiGetColour (210,210,210), 1, pos, w, 6);
        m_elevTrimPos = pos;    // save update location
        return true;
    }
    
    return false;
}


bool ElevatorTrimArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    if (GetXR1().CheckHydraulicPressure(true, true) == false)   // show warning if no hydraulic pressure
        return false; 

    if (!GetXR1().AreElevatorsOperational())
        return false;   // elevators offline, so elevator trim is offline as well

    double newLevel = GetVessel().GetControlSurfaceLevel(AIRCTRL_ELEVATORTRIM) + (oapiGetSimStep() * (my < 22 ? -ELEVATOR_TRIM_SPEED : ELEVATOR_TRIM_SPEED));

    GetVessel().SetControlSurfaceLevel(AIRCTRL_ELEVATORTRIM, newLevel);
    GetXR1().MarkAPUActive();  // reset the APU idle warning callout time

    // no sound for this control

    return true;
}

bool ElevatorTrimArea::ProcessVCMouseEvent(const int event, const VECTOR3 &coords)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    if (GetXR1().CheckHydraulicPressure(true, true) == false)   // show warning if no hydraulic pressure
        return false; 

    if (!GetXR1().AreElevatorsOperational())
        return false;   // elevators offline, so elevator trim is offline as well

    VESSEL3_EXT &vessel = GetVessel();
    double newLevel = vessel.GetControlSurfaceLevel(AIRCTRL_ELEVATORTRIM) + 
        (oapiGetSimStep() * (coords.y < 0.5 ? -ELEVATOR_TRIM_SPEED : ELEVATOR_TRIM_SPEED));

    vessel.SetControlSurfaceLevel(AIRCTRL_ELEVATORTRIM, newLevel);
        
    // no sound for this control

    return true;
}

//----------------------------------------------------------------------------------

WingLoadAnalogGaugeArea::WingLoadAnalogGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID) :
    AnalogGaugeArea(parentPanel, panelCoordinates, areaID, PI, meshTextureID)  // init @ 180 degrees (0 degrees points to the right)
{
}

double WingLoadAnalogGaugeArea::GetDialAngle()
{
    static const double dial_min = -123.0*RAD;
    static const double dial_max =  217.0*RAD;
    
    double load = GetVessel().GetLift() / WING_AREA; // L/S
    double dialAngle = PI - min (dial_max, max (dial_min, load/15.429e3*PI));

    return dialAngle;
}


//----------------------------------------------------------------------------------

AutopilotButtonsArea::AutopilotButtonsArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    XR1Area(parentPanel, panelCoordinates, areaID)
{
}

void AutopilotButtonsArea::Activate()
{
    Area::Activate();  // invoke superclass method
    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(154, 76), PANEL_REDRAW_USER,  PANEL_MOUSE_LBDOWN, PANEL_MAP_BACKGROUND);

    m_mainSurface = CreateSurface(IDB_NAVBUTTON);     // autopilot buttons
}

bool AutopilotButtonsArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    // Button coordinates for panel buttons, indexed from top-right corner of area.  
    // Order follows lighted buttons in NavButton.bmp; order is: 
    //  Kill Rotation, Attitude Hold, Prograde, Retrograde,
    //  Orbit Normal+, Orbit Normal-, Descent Hold, Airspeed Hold
    static const int navx[] = {  0, 3, 1, 1, 2, 2, 3, 0 };
    static const int navy[] = {  0, 0, 0, 1, 0, 1, 1, 1 };

    // process buttons 0-7 in NavButton.bmp from left to right
    for (int i=0; i < 8; i++)
    {
        bool isLit = false;
        switch (i)
        {
        case 0:     
            isLit = GetVessel().GetNavmodeState(NAVMODE_KILLROT);
            break;

        case 1:
            isLit = (GetXR1().m_customAutopilotMode == AUTOPILOT::AP_ATTITUDEHOLD);
            break;

        case 2:
            isLit = GetVessel().GetNavmodeState(NAVMODE_PROGRADE);
            break;

        case 3:
            isLit = GetVessel().GetNavmodeState(NAVMODE_RETROGRADE);
            break;

        case 4:
            isLit = GetVessel().GetNavmodeState(NAVMODE_NORMAL);
            break;

        case 5:
            isLit = GetVessel().GetNavmodeState(NAVMODE_ANTINORMAL);
            break;

        case 6:
            isLit = (GetXR1().m_customAutopilotMode == AUTOPILOT::AP_DESCENTHOLD);
            break;

        case 7:
            isLit = GetXR1().m_airspeedHoldEngaged;
            break;

            // no default for this case
        }

        if (isLit)
        {
            // dest coordinate blocks are 39x39, but source blocks are 37x37 since we don't repaint the border
            DeltaGliderXR1::SafeBlt(surf, m_mainSurface, navx[i] * 39, navy[i] * 39, (i * 37), 0, 37, 37);
        }
    }

    return true;
}

bool AutopilotButtonsArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    // get the block coordinates of the block clicked; grid is 4x2 (8 buttons total)
    const int x = (mx / 39);
    const int y = (my / 39);

    // invoke handler for each block 
    bool isOn = false;      // TRUE if new autopilot button is LIT (engaged)
    if (y == 0)
    {
        if (x == 0)
        {
            GetVessel().ToggleNavmode(NAVMODE_KILLROT);
            isOn = GetVessel().GetNavmodeState(NAVMODE_KILLROT);
        }
        else if (x == 1)
        {
            GetVessel().ToggleNavmode(NAVMODE_PROGRADE);
            isOn = GetVessel().GetNavmodeState(NAVMODE_PROGRADE);
        }
        else if (x == 2)
        {
            GetVessel().ToggleNavmode(NAVMODE_NORMAL);
            isOn = GetVessel().GetNavmodeState(NAVMODE_NORMAL);
        }
        else if (x == 3)
        {
            GetXR1().ToggleAttitudeHold();
            isOn = (GetXR1().m_customAutopilotMode == AUTOPILOT::AP_ATTITUDEHOLD);
        }
    }
    else  // y == 1
    {
        if (x == 0)
        {
            GetXR1().ToggleAirspeedHold(false);   // do not hold current airspeed
            isOn = GetXR1().m_airspeedHoldEngaged;
        }
        else if (x == 1)
        {
            GetVessel().ToggleNavmode(NAVMODE_RETROGRADE);
            isOn = GetVessel().GetNavmodeState(NAVMODE_RETROGRADE);
        }
        else if (x == 2)
        {
            GetVessel().ToggleNavmode(NAVMODE_ANTINORMAL);
            isOn = GetVessel().GetNavmodeState(NAVMODE_ANTINORMAL);
        }
        else if (x == 3)
        {
            GetXR1().ToggleDescentHold();
            isOn = (GetXR1().m_customAutopilotMode == AUTOPILOT::AP_DESCENTHOLD);
        }
    }

    if (event & PANEL_MOUSE_LBDOWN)     // do not play for INIT events
        GetXR1().PlaySound((isOn ? GetXR1().SwitchOn : GetXR1().SwitchOff), DeltaGliderXR1::ST_Other);

    return true;
}

//----------------------------------------------------------------------------------

AutopilotLEDArea::AutopilotLEDArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_isOn(false)       // initially off
{
    m_color = MEDIUM_GREEN; // init here for efficiency
}

void AutopilotLEDArea::Activate()
{
    Area::Activate();  // invoke superclass method
    // 2D-only for now
    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(32, 10), PANEL_REDRAW_USER, PANEL_MOUSE_IGNORE, PANEL_MAP_BACKGROUND);

    m_isOn = false;     
    TriggerRedraw();    // draw initial state
}

bool AutopilotLEDArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    if (m_isOn)
        oapiColourFill(surf, m_color);  // fill the entire area
    
    // must always return 'true' so either the background or the fill will be painted
    return true;    

}

void AutopilotLEDArea::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // check if any normal autopilot is on, including killrot
    m_enabled = false;          // assume disabled
    for (int i=1; i <= 7; i++)
    {
        if (GetVessel().GetNavmodeState(i))
        {
            m_enabled = true;   // light should blink
            break;
        }
    }

    // now check whether any CUSTOM autopilot mode is engaged
    if ((GetXR1().m_customAutopilotMode != AUTOPILOT::AP_OFF) || GetXR1().m_airspeedHoldEngaged)
        m_enabled = true;

    // if enabled, set the light to its correct blink state
    if (m_enabled)
    {
        if (fmod(simt, 3.5) <= 3.0)   // on for 3 seconds, off for 1/2-second
        {
            // turn LED on if it's currently off
            if (m_isOn == false)
            {
                m_isOn = true;
                TriggerRedraw();
            }
        }
        else
        {
            // turn LED off if it's currently on
            if (m_isOn)
            {
                m_isOn = false;
                TriggerRedraw();
            }
        }
    }
    else    // LED disabled -- turn it off if necessary
    {
        if (m_isOn)
        {
            m_isOn = false;
            TriggerRedraw();
        }
    }
}

//----------------------------------------------------------------------------------

MWSArea::MWSArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    XR1Area(parentPanel, panelCoordinates, areaID)
{
}

void MWSArea::Activate()
{
    Area::Activate();  // invoke superclass method
    if (IsVC())
    {
        oapiVCRegisterArea(GetAreaID(), PANEL_REDRAW_USER | PANEL_REDRAW_MOUSE, PANEL_MOUSE_LBDOWN | PANEL_MOUSE_ONREPLAY);
    }
    else
    {
        oapiRegisterPanelArea(GetAreaID(), GetRectForSize(29, 29), PANEL_REDRAW_USER | PANEL_REDRAW_MOUSE, PANEL_MOUSE_LBDOWN | PANEL_MOUSE_ONREPLAY);
        m_mainSurface = CreateSurface(IDB_WARN);     // master warning system alarm
    }
}

bool MWSArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    bool lightOn = GetXR1().m_MWSLit;
    
    // if TEST button pressed, light stays on regardless
    if (GetXR1().m_mwsTestActive)
        lightOn = true;

    DeltaGliderXR1::SafeBlt(surf, m_mainSurface, 0, 0, lightOn ? 29 : 0, 0, 29, 29);

    return true;
}

bool MWSArea::Redraw3D(const int event, const SURFHANDLE surf)
{
    bool lightOn = GetXR1().m_MWSLit;
    
    // if TEST button pressed, light stays on regardless
    if (GetXR1().m_mwsTestActive)
        lightOn = true;

    if (GetXR1().vcmesh != nullptr)
    {
        NTVERTEX vtx[4];
		static WORD vidx[4] = {32,33,34,35};
		GROUPEDITSPEC ges;
		ges.flags = GRPEDIT_VTXTEXU;
		ges.nVtx = 4;
		ges.vIdx = vidx;
		ges.Vtx = vtx;
		float xofs = 0.2246f + (lightOn ? 0.12891f : 0.0f);
		vtx[0].tu = vtx[1].tu = xofs;
		vtx[2].tu = vtx[3].tu = xofs + 0.125f;
		oapiEditMeshGroup (GetXR1().vcmesh, MESHGRP_VC_STATUSIND, &ges);
    }

    return true;
}

bool MWSArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

#ifdef INITIAL_TESTING_ONLY
    // NOTE: if you enable this block you must modify the MWS auto-reset code at the end of the TestDamage method as well!
    // toggle the light for testing
    bool newValue = !GetXR1().m_MWSActive;
    
    if (newValue == false)      // turning off MWS?
        GetXR1().ResetMWS();    // reset via method so the beep and audio alerts play
    else
        GetXR1().m_MWSActive = true;
#else
    // user turned off the warning light
    GetXR1().ResetMWS();
#endif

    return true;
}

//----------------------------------------------------------------------------------

RCSModeArea::RCSModeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const WORD resourceID) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_resourceID(resourceID)

{
}

// Activate this area
void RCSModeArea::Activate()
{
    Area::Activate();  // invoke superclass method
    if (IsVC())
        oapiVCRegisterArea(GetAreaID(), PANEL_REDRAW_USER, PANEL_MOUSE_DOWN);
    else
        oapiRegisterPanelArea(GetAreaID(), GetRectForSize(40, 44), PANEL_REDRAW_MOUSE, PANEL_MOUSE_DOWN);

    // load surface(s); most areas only have one surface
    m_mainSurface = CreateSurface(m_resourceID);     // rotary dial
}

bool RCSModeArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    int mode = GetVessel().GetAttitudeMode();

    DeltaGliderXR1::SafeBlt(surf, m_mainSurface, 0, 0, mode * 40, 0, 40, 44);

    return true;
}

bool RCSModeArea::Redraw3D(const int event, const SURFHANDLE surf)
{
    int mode = GetVessel().GetAttitudeMode();

    GetXR1().SetXRAnimation(GetXR1().anim_rcsdial, mode * 0.5);

    return true;
}

bool RCSModeArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    bool retVal = false;
    int mode = GetVessel().GetAttitudeMode();

    int newMode = -1;
    if (event & PANEL_MOUSE_LBDOWN)
    {
        // rotate left if not already @ mode 0
        if (mode)
        {
            newMode = mode - 1;
        }
    }
    else if (event & PANEL_MOUSE_RBDOWN) 
    {
        // rotate right if not already @ mode 2
        if (mode < 2)
        {
            newMode = mode + 1;
        }
    }

    if (newMode > -1)
    {
        GetVessel().SetAttitudeMode(newMode);
        retVal = true;
    }

    // always play sound
    GetXR1().PlaySound(GetXR1().SwitchOn, DeltaGliderXR1::ST_Other); // medium click

    return retVal;
}

//----------------------------------------------------------------------------------

AFCtrlArea::AFCtrlArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    XR1Area(parentPanel, panelCoordinates, areaID)
{
}

// Activate this area
void AFCtrlArea::Activate()
{
    Area::Activate();  // invoke superclass method
    if (IsVC())
        oapiVCRegisterArea(GetAreaID(), PANEL_REDRAW_USER, PANEL_MOUSE_DOWN);
    else
        oapiRegisterPanelArea(GetAreaID(), GetRectForSize(40, 44), PANEL_REDRAW_MOUSE, PANEL_MOUSE_DOWN);

    // load surface(s); most areas only have one surface
    m_mainSurface = CreateSurface(IDB_DIAL1);     // rotary dial
}

bool AFCtrlArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    int mode = GetVessel().GetAttitudeMode();

    // NOTE: if current switch value is ON but APU is OFF, don't redraw this switch!  This fixes the
    // brief "jump" that occurs when the pilot tries to enable AFCtrl with the APU already off.
    if ((GetXR1().apu_status != DoorStatus::DOOR_OPEN) && (GetVessel().GetADCtrlMode() != 0))
        return false;       // don't paint the "jumping" switch

    DeltaGliderXR1::SafeBlt(surf, m_mainSurface, 0, 0, min(GetVessel().GetADCtrlMode(), 2) * 40, 0, 40, 44);

    return true;
}

bool AFCtrlArea::Redraw3D(const int event, const SURFHANDLE surf)
{
    int mode = GetVessel().GetAttitudeMode();

    // NOTE: if current switch value is ON but APU is OFF, don't redraw this switch!  This fixes the
    // brief "jump" that occurs when the pilot tries to enable AFCtrl with the APU already off.
    if ((GetXR1().apu_status != DoorStatus::DOOR_OPEN) && (GetVessel().GetADCtrlMode() != 0))
        return false;       // don't paint the "jumping" switch

    GetXR1().SetXRAnimation(GetXR1().anim_afdial, min(GetVessel().GetADCtrlMode(), 2) * 0.5);

    return true;
}

bool AFCtrlArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    bool retVal = false;
    int mode = min(GetVessel().GetADCtrlMode(), 2);

    int newMode = -1;
    if (event & PANEL_MOUSE_LBDOWN)
    {
        // rotate left if not already @ mode 0
        if (mode)
        {
            newMode = mode - 1;
        }
    }
    else if (event & PANEL_MOUSE_RBDOWN) 
    {
        // rotate right if not already @ mode 2
        if (mode < 2)
        {
            newMode = (mode ? 7 : 1);
        }
    }

    if (newMode > -1)
    {
        // NOTE: warning will be played and mode reset in PostStep if APU offline
        GetVessel().SetADCtrlMode(newMode);
        retVal = true;
    }

    // always play sound
    GetXR1().PlaySound(GetXR1().SwitchOn, DeltaGliderXR1::ST_Other); // medium click

    return retVal;
}

//-------------------------------------------------------------------------

StaticPressureNumberArea::StaticPressureNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    NumberArea(parentPanel, panelCoordinates, areaID, 8, true)   // 8 chars plus decimal
{
}

bool StaticPressureNumberArea::UpdateRenderData(RENDERDATA &renderData)
{
    bool redraw = false;
    const bool forceRedraw = renderData.forceRedraw;

    double pressure = GetVessel().GetAtmPressure() / 1000; // in kPa

    // round to nearest 1/10000th
    pressure = (static_cast<double>(static_cast<int>(((pressure + 0.00005) * 10000.0)))) / 10000.0;

    // check whether the value has changed since the last render
    if (forceRedraw || (pressure != renderData.value))
    {
        // Value has changed -- let's redo the string and see if that is different as well.
        // The goal here is to be as efficient as possible and only re-render when we absolutely have to.
        char pTemp[16];
        // ensure that value is in range
        if (pressure > 9999.9999)
            pressure = 9999.9999;   // trim

        // pressure should never be < 0, but let's sanity-check it anyway
        if (pressure < 0)
            pressure = 0;

        sprintf(pTemp, "%9.4f", pressure); 
        if (forceRedraw || (strcmp(pTemp, renderData.pStrToRender) != 0))
        {
            // text has changed; signal the base class to render it
            renderData.value = pressure;   // remember for next time
            strcpy(renderData.pStrToRender, pTemp);
            redraw = true;
            renderData.forceRedraw = false;  // clear reset request
        }
    }

    return redraw;
}

//----------------------------------------------------------------------------------

MWSTestButtonArea::MWSTestButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    MomentaryButtonArea(parentPanel, panelCoordinates, areaID)
{
}

void MWSTestButtonArea::ProcessButtonAction(int event, double buttonDownSimt)
{
    // process PRESSED and UNPRESSED events
    if (event & (PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBUP))
    {
        GetXR1().PlaySound(GetXR1().SwitchOn, DeltaGliderXR1::ST_Other, QUIET_CLICK);  // light click for both on and off
        GetXR1().m_mwsTestActive = ((event & PANEL_MOUSE_LBDOWN) == true);

        // redraw the MWS light and MWS warning panels
        GetVessel().TriggerRedrawArea(AID_MWS);
        GetVessel().TriggerRedrawArea(AID_WARNING_LIGHTS);
        GetVessel().TriggerRedrawArea(AID_APU_BUTTON);
    }

    // ignore PANEL_MOUSE_LBPRESSED events
}

//----------------------------------------------------------------------------------

WarningLightsArea::WarningLightsArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_lightStateOn(false)
{
}

void WarningLightsArea::Activate()
{
    Area::Activate();  // invoke superclass method
    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(78, 77), PANEL_REDRAW_USER, PANEL_MOUSE_IGNORE, PANEL_MAP_BACKGROUND);
    m_mainSurface = CreateSurface(IDB_WARNING_LIGHTS);
}

bool WarningLightsArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    // if TEST button pressed, all lights stay on regardless
    bool testModeActive = GetXR1().m_mwsTestActive;

    // check each light's status
    for (int i=0; i < WARNING_LIGHT_COUNT; i++)
    {   
        bool warningActive = GetXR1().m_warningLights[i];

        // light is ON if 1) test mode, or 2) warning is active and blink state is ON
        if (testModeActive || (warningActive && m_lightStateOn))
        {
            if (m_lightStateOn || testModeActive)
            {
                // render the "lit up" texture
                int x = (i % 3) * 26;    // column
                int y = (i/3) * 11;      // row 

                DeltaGliderXR1::SafeBlt(surf, m_mainSurface, x, y, x, y, 26, 11);
            }
        }
    }
        
    // always return 'true' here so we are sure to turn off any now-off-but-previously-lit lights
    return true;
}

void WarningLightsArea::clbkPrePostStep(const double simt, const double simdt, const double mjd) 
{
    double di;
    // NOTE: must use fabs simt here since simt may be negative!
    bool lightStateOn = (modf(simt, &di) < 0.5);   // blink twice a second
    if (lightStateOn != m_lightStateOn)  // has state switched?
    {
        // toggle the state and request a repaint
        m_lightStateOn = lightStateOn;
        TriggerRedraw();

        // no sound with these lights
    }
}

//----------------------------------------------------------------------------------

DeployRadiatorButtonArea::DeployRadiatorButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_lightState(false), m_lastRenderedLightState(false)
{
}

void DeployRadiatorButtonArea::Activate()
{
    Area::Activate();  // invoke superclass method
    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(18, 15), PANEL_REDRAW_ALWAYS, PANEL_MOUSE_LBDOWN);
    m_mainSurface = CreateSurface(IDB_GREEN_LED_TINY);
}

bool DeployRadiatorButtonArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    bool retVal = false;

    if ((event == PANEL_REDRAW_INIT) || (m_lastRenderedLightState != m_lightState))
    {
        DeltaGliderXR1::SafeBlt(surf, m_mainSurface, 0, 0, m_lightState ? 18 : 0, 0, 18, 15);
        m_lastRenderedLightState = m_lightState;
        retVal = true;
    }

    return retVal;
}

bool DeployRadiatorButtonArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    // hydraulic pressure will be checked by ActivateRadiator below

    GetXR1().PlaySound(GetXR1().SwitchOn, DeltaGliderXR1::ST_Other);    // normal click
    GetXR1().ToggleRadiator();

    return true;
}

void DeployRadiatorButtonArea::clbkPrePostStep(const double simt, const double simdt, const double mjd) 
{
    const DoorStatus ds = GetXR1().radiator_status;

    if (ds == DoorStatus::DOOR_OPEN)
        m_lightState = true;
    else if ((ds == DoorStatus::DOOR_OPENING) || (ds == DoorStatus::DOOR_CLOSING))
        m_lightState = (fmod(simt, 0.75) < 0.375);  // blink once every 3/4-second
    else  // door closed or FAILED
        m_lightState = false;
}

//----------------------------------------------------------------------------------

DataHUDButtonArea::DataHUDButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    MomentaryButtonArea(parentPanel, panelCoordinates, areaID)
{
}

void DataHUDButtonArea::ProcessButtonAction(int event, double buttonDownSimt)
{
    // process PRESSED and UNPRESSED events
    if (event & (PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBUP))
    {
        GetXR1().PlaySound(GetXR1().SwitchOn, DeltaGliderXR1::ST_Other, MED_CLICK);  // medium click for both on and off
        GetXR1().m_dataHUDActive = ((event & PANEL_MOUSE_LBDOWN) == true);

        // HUD is redrawn automatically each frame, so no redraw areas to trigger here
    }

    // ignore PANEL_MOUSE_LBPRESSED events
}

// override default IsLit behavior so the button lights up when the HUD is activated via the shortcut key as well
bool DataHUDButtonArea::IsLit() 
{ 
    return GetXR1().m_dataHUDActive;
}
