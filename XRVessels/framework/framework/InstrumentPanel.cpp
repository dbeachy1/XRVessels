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
// InstrumentPanel.cpp
// Abstract base class defining a 2D or 3D instrument panel
// ==============================================================

#include <memory.h>
#include "InstrumentPanel.h"
#include "Area.h"

// Constructor
// vessel = parent vessel
// panelID = globally unique panel ID; also serves as Orbiter 2D panel ID
// vcPanelID = Orbiter VC panel ID, or -1 if this is a 2D panel
// panelResourceID = resource ID of this panel in our DLL; e.g., IDB_PANEL1_1280.  -1 = NONE
// force3DRedrawTo2D = true to force all 3D area redraws to invoke Redraw2D instead of Redraw3D (e.g., for glass panel virtual cockpits that have no animation)
//     NOTE: force3DRedrawTo2D is useful because it allows a glass-cockpit VC to use and/or inherit the base class XR1 areas without having to override all the existing Redraw3D methods just so call Redraw2D for each one.
InstrumentPanel::InstrumentPanel(VESSEL3_EXT &vessel, const int panelID, const int vcPanelID, const WORD panelResourceID, const bool force3DRedrawTo2D) :
        AreaGroup(), 
        m_vessel(vessel), m_panelID(panelID), m_vcPanelID(vcPanelID), m_hBmp(nullptr), m_isActive(false), 
        m_panelResourceID(panelResourceID), m_force3DRedrawTo2D(force3DRedrawTo2D)
{
    // NOTE: m_hBitmap must be reloaded inside Activate on each call because Orbiter seems to free the 
    // panel-associated bitmap memory itself each time the panel is deactivated.
}

// Destructor
InstrumentPanel::~InstrumentPanel()
{
    // free all components in the vector so our subclass won't have to
    vector<Component *>::const_iterator it = m_componentVector.begin();
    for (; it != m_componentVector.end(); it++)
    {
        Component *pComponent = *it;
        delete pComponent;
    }

    Deactivate();   // free up surfaces 
}

// Add a new component to this instrument panel
// pComp = new component object
// Returns: pComp
Component *InstrumentPanel::AddComponent(Component *pComp)
{
    m_componentVector.push_back(pComp);  // add to end of vector

    return pComp;
}

// release our surfaces; invoked when Orbiter invokes "ReleaseSurfaces"
void InstrumentPanel::Deactivate()
{
    // do not deactivate areas unless we are currently active
    if (IsActive())
    {
        // deactivate all our areas, including our component's areas
        DeactivateAllAreas();

        // free our bitmap, if any
        if (m_hBmp != nullptr)
            DeleteObject(m_hBmp);

        SetActive(false);   // panel deactivated now
    }
}

// Process a redraw event for the requested area ID if it is on our panel.
// Returns: true if event was processed, false if it was not
bool InstrumentPanel::ProcessRedrawEvent(const int areaID, const int event, const SURFHANDLE surf)
{
    bool retVal = false;

    Area *pArea = GetArea(areaID);
    if (pArea != nullptr)
        retVal = pArea->Redraw(event, surf);

    return retVal;
}

// Process a mouse event for the requested area ID if it is on our panel.
// Returns: true if event was processed, false if it was not
bool InstrumentPanel::ProcessMouseEvent(const int areaID, const int event, const int mx, const int my)
{
    bool retVal = false;

    Area *pArea = GetArea(areaID);
    if (pArea != nullptr)
        retVal = pArea->ProcessMouseEvent(event, mx, my);

    return retVal;
}

// Process a virtual cockpit mouse event for the requested area ID if it is on our panel.
// Returns: true if event was processed, false if it was not
bool InstrumentPanel::ProcessVCMouseEvent(const int areaID, const int event, const VECTOR3 &coords)
{
    bool retVal = false;

    Area *pArea = GetArea(areaID);
    if (pArea != nullptr)
        retVal = pArea->ProcessVCMouseEvent(event, coords);

    return retVal;
}

// Process a 'trigger redraw' request for the given area ID
// areaID = unique Orbiter area ID
// Returns: true if request processed, false if requested area ID is not on our panel
bool InstrumentPanel::TriggerRedrawArea(const int areaID)
{
    bool retVal = false;

    Area *pArea = GetArea(areaID);
    if (pArea != nullptr)
    {
        TriggerRedrawArea(pArea);  // do the work
        retVal = true;
    }

    return retVal;
}

// Redraw an Area
// This is primarily invoked from child Area classes
// Note: this is the worker method that handles all redraw requests; all other redraw methods end up here.
void InstrumentPanel::TriggerRedrawArea(Area *pArea)
{
    // Work around Orbiter bug: calling TriggerRedrawArea from a PostStep when we are not in focus will crash the Orbiter core.
    if (GetVessel().HasFocus() == false)
        return;     // nothing to do

    // trigger either a 2D or 3D redraw depending on the current panel
    if (IsVC())
    {
        // 3D panel
        oapiVCTriggerRedrawArea(GetVCPanelID(), pArea->GetAreaID());
    }
    else
    {
        // 2D panel
        GetVessel().TriggerPanelRedrawArea(GetPanelID(), pArea->GetAreaID());
    }
}
