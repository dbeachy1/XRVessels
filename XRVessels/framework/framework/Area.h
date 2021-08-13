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
// XR Vessel Framework
//
// Area.h
// Abstract base class defining an updatable or interactive area on
// a 2D or 3D panel. Typically, an Area will be created by a Component
// object, but this is not a requirement.  Areas always deal with
// absolute panel coordinates, whereas a Component will construct Area
// objects relative to the Component's top-left corner coordinates.
// ==============================================================

#pragma once

#include "Orbitersdk.h"
#include "Vessel3Ext.h"
#include "InstrumentPanel.h"

class Area
{
public:
    // vcPanelTextureID = VC panel texture ID constant from XRxAreas.h or VCPANEL_TEXTURE_NONE if no VC texture for this area
    Area(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID = VCPANEL_TEXTURE_NONE);
    virtual ~Area();

    VESSEL3_EXT &GetVessel() const { return m_parentPanel.GetVessel(); }
    double GetAbsoluteSimTime() const { return GetVessel().GetAbsoluteSimTime(); }  // convenience method
    InstrumentPanel &GetParentPanel() const { return m_parentPanel; }
    bool IsVC() const { return m_parentPanel.IsVC(); }  // is area in virtual cockpit?
    COORD2 GetPanelCoordinates() const { return m_panelCoordinates; }
    RECT GetRectForSize(const int sizeX, const int sizeY);  // this also informs the area object of its *size* on the panel as well
    
    int GetAreaID() const { return m_areaID; };
    void TriggerRedraw();
    void SetParentComponent(Component *pComponent) { m_pParentComponent = pComponent; }
    Component *GetParentComponent() const { return m_pParentComponent; }
    // Note: these two handles do not need to be cleaned up later: Orbiter does it automatically
    SURFHANDLE GetMeshTextureHandle(const int meshTextureID) const;
    SURFHANDLE GetVCPanelTextureHandle() const { return GetMeshTextureHandle(m_meshTextureID); }  // may be null
    int GetSizeX() const { return m_sizeX; }
    int GetSizeY() const { return m_sizeY; }

    // returns true if this area is active (mainly used for assertion checks)
    bool IsActive() const { return m_isActive; }  

    // NOTE: this method should be overridden by the subclasses, but you MUST invoke the superclass method here as well
    virtual void Activate();

    // NOTE: you must override this method if you allocate more than one surface (m_mainSurface)
    // in your Activate() method.  If you do, be sure to still invoke this base class method, however.
    virtual void Deactivate();

    // this is NOT virtual because subclasses should never override it!
    bool Redraw(const int event, const SURFHANDLE surf) 
    { 
        // set to true if area redrawn, false if not
        // Note: 'isForce3DRedrawTo2D' is here so we can force a base-class custom Redraw3D implementation to be ignored for subclasses that
        // have a 'glass panel' VC and always want the 2D renderer to execute.
        return ((IsVC() && !m_parentPanel.IsForce3DRedrawTo2D()) ? Redraw3D(event, surf) : Redraw2D(event, surf));
    }

    // NOTE: at least one of these three methods AND/OR the protected Redraw2D/Redraw3D methods must be overidden by the subclass in order the Area to do anything useful
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my) { return false; };  // returns true if event processed, false if not
    
    // Invokes 2D ProcessMouseEvent by default; if you need the VC coordinates of the mouse click, override the method
    virtual bool ProcessVCMouseEvent(const int event, const VECTOR3 &coords) { return ProcessMouseEvent(event, -1, -1); }

    // NOTE: remember that these methods will be invoked even when the panel is deactivated, so 
    // be sure not to access any member data freed or invalidated by the Deactivate method.

    // NEW BEHAVIOR FOR 1.3: this is only invoked for the ACTIVE panel to ensure that no business logic is performed in one of these PostSteps!
    // Also remember that an area can exist on multiple panels, so this callback should ONLY perform area-display-specific tasks (such as blinking a light).
    // Otherwise you will perform duplicate (and possibly corrupted!) work at each frame.
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd) { return; }  // invoked at each timestep from Orbiter

    //*************************************************************************************************
    // These three methods were originally created to handle automatic creation of a GDI surface for any area this blits transparently
    // as a performance enhancement for the D3D9 graphics client.  However, that method of blitting didn't work, so 
    // while it is now moot I have left these methods in (and XR code invokes them) for the sake of future flexibility.
    //
    // These methods are non-virtual because they should never be overloaded by subclasses.
    // Note: for the sake of consistency and future enhancements XR areas should ALWAYS use the methods below instead of their oapi counterparts.
    // Note: these methods must be public instead of protected because we need to access them from MultiDisplayArea classes.
    //*************************************************************************************************
    void SetSurfaceColorKey(const SURFHANDLE surf, const DWORD ck);  // replaces oapiSetSurfaceColourKey
    HDC GetDC(const SURFHANDLE surf);                                // replaces oapiGetDC
    void ReleaseDC(const SURFHANDLE surf, const HDC hDC);            // replaces oapiReleaseDC

protected:
    // NOTE: subclasses should always override one or both of these two methods
    // NOTE: in general, Redraw2D implementations are inheritable by subclsses and work fine unchanged, but Redraw3D implementations are vessel-specific (e.g., animating switches, etc.)
    virtual bool Redraw2D(const int event, const SURFHANDLE surf) { _ASSERTE(false); return false; }  // should never reach here, because it means no handler was implemented for a 2D area in 2D panel mode!
    virtual bool Redraw3D(const int event, const SURFHANDLE surf) { return Redraw2D(event, surf); }   // by default, perform same action as 2D (necessary for 'glass panel' VC panels)

    SURFHANDLE CreateSurface(const int resourceID) const;
    void DestroySurface(SURFHANDLE *pSurfHandle);  

    // surface data
    SURFHANDLE m_mainSurface;       // our main surface handle; most areas only have one surface

    // data
    InstrumentPanel &m_parentPanel;
    Component *m_pParentComponent;   // may be null
    COORD2 m_panelCoordinates;
    int m_areaID;
    // Note: subclasses should normally not access this directly, but should invoke GetVCPanelTextureHandle() instead
    const int m_meshTextureID;    // vessel-specific pseudo-enum value (see XR1Areas.h, for example) that is mapped to a VC texture index by VESSEL3_EXT::MeshTextureIDToTextureIndex

private:
    int m_sizeX;          // -1 = not set via GetRectForSize yet
    int m_sizeY;          // -1 = not set via GetRectForSize yet
    bool m_isActive;      // true if area is active; mainly used by assertion checks
};
