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

// NOTE: this is a simple toggle button that stays lit until you change it externally.
// Remember that an area can exist on more than one panel, although each will have a different unique ID.

// lighted button area for a timed event, such as auto-centering a control
// pIsLit ptr to boolean to track IsLit status; if null, the class will use an internal IsLit variable and it will be set to FALSE here.  Otherwise, the value referenced by the pointer will not be changed.
// buttonMeshGroup = mesh group for 3D button; default == -1 (no VC button)
SimpleButtonArea::SimpleButtonArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID, bool* pIsLit, const int buttonMeshGroup) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_buttonMeshGroup(buttonMeshGroup), m_defaultIsLit(false)
{
    if (pIsLit == nullptr)
        pIsLit = &m_defaultIsLit;   // use default built-in bool

    m_pIsLit = pIsLit;
}

void SimpleButtonArea::Activate()
{
    Area::Activate();  // invoke superclass method
    m_mainSurface = CreateSurface(IDB_LIGHT2);                  // lighted green button

    if (IsVC()) // 3D panel?
    {
        oapiVCRegisterArea(GetAreaID(), PANEL_REDRAW_USER | PANEL_REDRAW_MOUSE, PANEL_MOUSE_LBDOWN);
    }
    else    // 2D panel
    {
        oapiRegisterPanelArea(GetAreaID(), GetRectForSize(12, 12), PANEL_REDRAW_USER | PANEL_REDRAW_MOUSE, PANEL_MOUSE_LBDOWN);
    }

    // do not reset m_pIsLit value

    // no need to redraw here; Orbiter will do it for us
}

bool SimpleButtonArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    // always re-render this since it is always performed on request
    int srcX = (*m_pIsLit ? 12 : 0);    // texture X coord; 12 = lit, 0 = not lit
    DeltaGliderXR1::SafeBlt(surf, m_mainSurface, 0, 0, srcX, 0, 12, 12);

    return true;
}

bool SimpleButtonArea::Redraw3D(const int event, const SURFHANDLE surf)
{
    static const float tv0[8] = { 0,0,0.0469f,0.0469f,0,0,0.0469f,0.0469f };

    if ((GetXR1().vcmesh == nullptr) || (m_buttonMeshGroup < 0))
        return false;       // nothing to draw

    static NTVERTEX vtx[8];   // this is OK because Orbiter is single-threaded; i.e., only one XR vessel is active at a time
    float ofs = (*m_pIsLit ? 0.0469f : 0);
    GROUPEDITSPEC ges;
    ges.flags = GRPEDIT_VTXTEXV;
    ges.nVtx = 8;
    ges.vIdx = nullptr;
    ges.Vtx = vtx;

    for (int j = 0; j < 8; j++)
        vtx[j].tv = tv0[j] + ofs;
    oapiEditMeshGroup(GetXR1().vcmesh, m_buttonMeshGroup, &ges);

    return true;
}

bool SimpleButtonArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    // toggle the button state
    *m_pIsLit = !*m_pIsLit;

    // play sound if the mouse was just clicked
    if (event & PANEL_MOUSE_LBDOWN)
        GetXR1().PlaySound(GetXR1().SwitchOn, DeltaGliderXR1::ST_Other, QUIET_CLICK);    // light click

    return true;
}

//----------------------------------------------------------------------------------

// lighted button area for a timed event, such as auto-centering a control
// pIsLit ptr to boolean to track IsLit status; if null, the class will use an internal IsLit variable and it will be set to FALSE here.  Otherwise, the value referenced by the pointer will not be changed.
// buttonMeshGroup = mesh group for 3D button; default == -1 (no VC button)
TimedButtonArea::TimedButtonArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID, bool* pIsLit, const int buttonMeshGroup) :
    SimpleButtonArea(parentPanel, panelCoordinates, areaID, pIsLit, buttonMeshGroup),
    m_previousIsLit(false)
{
}

// invoked once per timestep
void TimedButtonArea::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // invoke the subclass method to process the switch action 
    ProcessTimedEvent(*m_pIsLit, m_previousIsLit, simt, simdt, mjd);    // subclass will take some action here

    m_previousIsLit = *m_pIsLit;  // remember for next time
}

//-------------------------------------------------------------------------

// lighted button area that is lit as long as the mouse button is held down
// buttonMeshGroup = mesh group for 3D button; default == -1 (no VC button)
MomentaryButtonArea::MomentaryButtonArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID, const int buttonMeshGroup) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_buttonMeshGroup(buttonMeshGroup), m_isLit(false), m_buttonDownSimt(-1)
{
}

void MomentaryButtonArea::Activate()
{
    Area::Activate();  // invoke superclass method
    m_mainSurface = CreateSurface(IDB_LIGHT2);                  // lighted green button

    if (IsVC()) // 3D panel?
    {
        oapiVCRegisterArea(GetAreaID(), PANEL_REDRAW_USER | PANEL_REDRAW_MOUSE, PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBPRESSED | PANEL_MOUSE_LBUP);
    }
    else    // 2D panel
    {
        oapiRegisterPanelArea(GetAreaID(), GetRectForSize(12, 12), PANEL_REDRAW_USER | PANEL_REDRAW_MOUSE, PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBPRESSED | PANEL_MOUSE_LBUP);
    }
}

bool MomentaryButtonArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    int srcX = (IsLit() ? 12 : 0);    // texture X coord; 12 = lit, 0 = not lit
    DeltaGliderXR1::SafeBlt(surf, m_mainSurface, 0, 0, srcX, 0, 12, 12);

    return true;
}

bool MomentaryButtonArea::Redraw3D(const int event, const SURFHANDLE surf)
{
    static const float tv0[8] = { 0,0,0.0469f,0.0469f,0,0,0.0469f,0.0469f };

    if ((GetXR1().vcmesh == nullptr) || (m_buttonMeshGroup < 0))
        return false;       // nothing to draw

    static NTVERTEX vtx[8];   // this is OK because Orbiter is single-threaded; i.e., only one XR vessel is active at a time
    float ofs = (IsLit() ? 0.0469f : 0);
    GROUPEDITSPEC ges;
    ges.flags = GRPEDIT_VTXTEXV;
    ges.nVtx = 8;
    ges.vIdx = nullptr;
    ges.Vtx = vtx;

    for (int j = 0; j < 8; j++)
        vtx[j].tv = tv0[j] + ofs;
    oapiEditMeshGroup(GetXR1().vcmesh, m_buttonMeshGroup, &ges);

    return true;
}

bool MomentaryButtonArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    // check whether button was just pressed
    if (event & PANEL_MOUSE_LBDOWN)
    {
        m_buttonDownSimt = GetAbsoluteSimTime();
        m_isLit = true;
    }

    // let the subclass take some action base on the click/hold action
    ProcessButtonAction(event, m_buttonDownSimt);

    // check whether button was just unpressed
    if (event & PANEL_MOUSE_LBUP)
    {
        m_buttonDownSimt = -1;
        m_isLit = false;
    }

    return true;
}

//-------------------------------------------------------------------------

// lighted button area whose raw mouse events are passed to the sublcass
// buttonMeshGroup = mesh group for 3D button; default == -1 (no VC button)
RawButtonArea::RawButtonArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID, const int buttonMeshGroup) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_buttonMeshGroup(buttonMeshGroup), m_buttonDownSimt(-1)
{
}

void RawButtonArea::Activate()
{
    Area::Activate();  // invoke superclass method
    m_mainSurface = CreateSurface(IDB_LIGHT2);                  // lighted green button

    if (IsVC()) // 3D panel?
    {
        oapiVCRegisterArea(GetAreaID(), PANEL_REDRAW_USER | PANEL_REDRAW_MOUSE, PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBPRESSED | PANEL_MOUSE_LBUP);
    }
    else    // 2D panel
    {
        oapiRegisterPanelArea(GetAreaID(), GetRectForSize(12, 12), PANEL_REDRAW_USER | PANEL_REDRAW_MOUSE, PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBPRESSED | PANEL_MOUSE_LBUP);
    }
}

bool RawButtonArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    int srcX = (IsLit() ? 12 : 0);    // texture X coord; 12 = lit, 0 = not lit
    DeltaGliderXR1::SafeBlt(surf, m_mainSurface, 0, 0, srcX, 0, 12, 12);

    return true;
}

bool RawButtonArea::Redraw3D(const int event, const SURFHANDLE surf)
{
    static const float tv0[8] = { 0,0,0.0469f,0.0469f,0,0,0.0469f,0.0469f };

    if ((GetXR1().vcmesh == nullptr) || (m_buttonMeshGroup < 0))
        return false;       // nothing to draw

    static NTVERTEX vtx[8];   // this is OK because Orbiter is single-threaded; i.e., only one XR vessel is active at a time
    float ofs = (IsLit() ? 0.0469f : 0);
    GROUPEDITSPEC ges;
    ges.flags = GRPEDIT_VTXTEXV;
    ges.nVtx = 8;
    ges.vIdx = nullptr;
    ges.Vtx = vtx;

    for (int j = 0; j < 8; j++)
        vtx[j].tv = tv0[j] + ofs;
    oapiEditMeshGroup(GetXR1().vcmesh, m_buttonMeshGroup, &ges);

    return true;
}

bool RawButtonArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    // check whether button was just pressed
    if (event & PANEL_MOUSE_LBDOWN)
    {
        // record button down time
        m_buttonDownSimt = GetAbsoluteSimTime();
    }

    // let the subclass take some action based on the click/hold action
    ProcessButtonAction(event, m_buttonDownSimt);

    return true;
}

//----------------------------------------------------------------------------------

// NOTE: fuel dump status will NOT be preserved in the save file; we never want to boot up and resume dumping fuel automatically
// fuelDumpInProgress = reference to bool flag denoting fuel dump status for a given tank
FuelDumpButtonArea::FuelDumpButtonArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID, bool& fuelDumpInProgress, const char* pFuelLabel) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_fuelDumpInProgress(fuelDumpInProgress), m_isLit(false), m_buttonDownSimt(-1),
    m_buttonPressProcessed(false), m_isButtonDown(false)
{
    strcpy(m_fuelLabel, pFuelLabel);
}

void FuelDumpButtonArea::Activate()
{
    Area::Activate();  // invoke superclass method
    m_mainSurface = CreateSurface(IDB_LIGHT2);                  // lighted green button

    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(12, 12), PANEL_REDRAW_MOUSE, PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBPRESSED | PANEL_MOUSE_LBUP);

    // reset to NOT lit
    m_isLit = false;

    TriggerRedraw();
}

bool FuelDumpButtonArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    // always re-render this since it is always performed on request
    int srcX = (m_isLit ? 12 : 0);    // texture X coord; 12 = lit, 0 = not lit
    DeltaGliderXR1::SafeBlt(surf, m_mainSurface, 0, 0, srcX, 0, 12, 12);

    return true;
}

bool FuelDumpButtonArea::ProcessMouseEvent(const int event, const int mx, const int my)
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

void FuelDumpButtonArea::ProcessButtonPressed(const int event)
{
    if (event & (PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBUP))
        GetXR1().PlaySound(GetXR1().SwitchOn, DeltaGliderXR1::ST_Other, QUIET_CLICK);  // light click for both on and off

    if (event & PANEL_MOUSE_LBDOWN)
    {
        m_buttonPressProcessed = false;     // reset for this new press

        if (m_fuelDumpInProgress)
        {
            GetXR1().SetFuelDumpState(m_fuelDumpInProgress, false, m_fuelLabel);   // displays warning & plays callout

            // nothing more to do for this press
            m_buttonPressProcessed = true;
            return;
        }
    }

    if (m_buttonPressProcessed)
        return;     // ignore this event; button press already processed

    const double RESET_TIME = 2.5;      // button must be held this long to initiate fuel dump
    const double buttonHoldTime = GetAbsoluteSimTime() - m_buttonDownSimt;

    if (event & PANEL_MOUSE_LBPRESSED)
    {
        if (buttonHoldTime >= RESET_TIME)
        {
            GetXR1().SetFuelDumpState(m_fuelDumpInProgress, true, m_fuelLabel);  // displays warning & plays callout
            // Note: we cannot easily determine whether to play an error beep here since we do not know about our tank level, so just play an
            // acknowledgement beep: we will play an error beep if the tank empties (or *is* empy) in the FuelDumpPostStep.
            m_buttonPressProcessed = true;   // ignore any further events
        }
    }
    else    // button was released before fuel dump was initiated
    {
        GetXR1().ShowWarning("Hold to Dump Fuel.wav", DeltaGliderXR1::ST_WarningCallout, "You must hold down the dump&button to initiate fuel dump.");
        m_buttonPressProcessed = true;
    }
}

void FuelDumpButtonArea::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    if (m_fuelDumpInProgress)
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

