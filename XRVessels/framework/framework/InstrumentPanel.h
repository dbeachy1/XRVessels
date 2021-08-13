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
// InstrumentPanel.h
// Abstract base class defining a 2D instrument panel
// ==============================================================

#pragma once

#include "Orbitersdk.h"
#include "vessel3ext.h"
#include "Component.h"
#include "AreaGroup.h"

class InstrumentPanel : public AreaGroup
{
public:
    InstrumentPanel(VESSEL3_EXT &vessel, const int panelID, const int vcPanelID = -1, const WORD panelResourceID = -1, const bool force3DRedrawTo2D = false);
    virtual ~InstrumentPanel();

    VESSEL3_EXT &GetVessel() const { return m_vessel; }
    int GetPanelID() const { return m_panelID; }
    int GetVCPanelID() const { return m_vcPanelID; }
    bool IsForce3DRedrawTo2D() const { return m_force3DRedrawTo2D; }  // returns true if all 3D area redraw calls invoke Redraw2D instead (for glass panel VCs)
    // NOTE: this check whether the PANEL is a VC panel, NOT whether oapiCockpitMode() == COCKPIT_VIRTUAL
    // This is necessary so that the proper panel can be located for redraw events (2D vs. 3D)
    bool IsVC() const { return (m_vcPanelID >= 0); }
    bool IsActive() const { return m_isActive; }
    void SetActive(bool b) { m_isActive = b; }

    bool TriggerRedrawArea(const int areaID);
    void TriggerRedrawArea(Area *pArea);

    // returns resource ID of this panel in our DLL; e.g., IDB_PANEL1_1280
    WORD GetPanelResourceID() const
    {
        return m_panelResourceID;   // -1 = NONE
    }

    // methods that may be overridden by subclasses; however, be sure to call the base class method as well
    virtual void Deactivate();
    virtual bool ProcessRedrawEvent(const int areaID, const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int areaID, const int event, const int mx, const int my);
    virtual bool ProcessVCMouseEvent(const int areaID, const int event, const VECTOR3 &coords);

    // methods that must be implemented by the subclass
    virtual bool Activate() = 0;

protected:
    Component *AddComponent(Component *pComp);

    // data
    VESSEL3_EXT &m_vessel;
    int m_panelID;      // globally unique panel ID; also serves as Orbiter 2D panel ID
    int m_vcPanelID;    // Orbiter VC panel ID, or < 0 if this is a 2D panel
    HBITMAP m_hBmp;     // 2D bitmap resource, if any
    bool m_isActive;    // true if panel is activated, false if deactivated
    const bool m_force3DRedrawTo2D;  // if true, all area redraw calls in 3D (virtual cockpit) mode will invoke Redraw2D instead of Redraw3D

private:
    // data
    WORD m_panelResourceID; // resource ID of this panel in our DLL; e.g., IDB_PANEL1_1280
    vector<Component *> m_componentVector;    // list of all components on the panel
};
