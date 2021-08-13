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
// Area.cpp
// Abstract base class defining an updatable or interactive area on
// a 2D or 3D panel.  Typically, an Area will be created by a Component
// object, but this is not a requirement.
// ==============================================================

#include "Area.h"

// Constructor
// Note: default for m_vcPanelTextureID = -1, which means "none"
Area::Area(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID) : 
    m_parentPanel(parentPanel), m_panelCoordinates(panelCoordinates), 
    m_areaID(areaID), m_mainSurface(0),
    m_pParentComponent(nullptr), m_meshTextureID(meshTextureID), m_sizeX(-1), m_sizeY(-1), m_isActive(false)
{
}

// Destructor
Area::~Area()
{
}

// Informs the Area object of its size (may be cached for use elsewhere)
// and returns a rectangle of a given size using our base panel coordinates as the top-left corner.
// This cannot be 'const' because we save the size of this area.
RECT Area::GetRectForSize(const int sizeX, const int sizeY)
{
    // save the size for use later 
    m_sizeX = sizeX;
    m_sizeY = sizeY;

    COORD2 c = GetPanelCoordinates();
    // NOTE: lower and right coordinates are EXCLUSIVE here; they are not part of the rectangle
    return _R(c.x, c.y, c.x + sizeX, c.y + sizeY);
}

// Load a bitmap resource and return an Orbiter surface handle.  
SURFHANDLE Area::CreateSurface(const int resourceID) const
{
    const HINSTANCE hDLL = GetVessel().GetModuleHandle();
    return oapiCreateSurface(LoadBitmap(hDLL, MAKEINTRESOURCE (resourceID)));
}

// Destroy (free) an Orbiter surface and set the variable containing the surface value to 0
void Area::DestroySurface(SURFHANDLE *pSurfHandle)
{
    // NOTE: surface may have already been freed (or not yet allocated), so check for 0 here
    if (*pSurfHandle != 0)
    {
        oapiDestroySurface(*pSurfHandle);
        *pSurfHandle = 0;    // clear so we don't free it again
    }
}

// Request a redraw of ourself
void Area::TriggerRedraw()
{
    // get our parent panel and invoke a redraw
    GetParentPanel().TriggerRedrawArea(this);
}

// the default activate method currently only sets our active flag, which is mainly used by assertion checks
void Area::Activate()
{
    _ASSERTE(!IsActive());  // ensure that the subclass remembered to invoke its superclass's Deactivate method
    m_isActive = true;
}

// the default deactivate method currently only frees m_mainSurface and our private, cached, m_cachedGDISurface 
// and clears the active flag, which is mainly used by assertion checks
void Area::Deactivate()
{
    _ASSERTE(IsActive());  // ensure that the subclass remembered to invoke its superclass's Activate method
    m_isActive = false;

    // destroy surfaces and set handles to 0 (either or both of these may be zero)
    DestroySurface(&m_mainSurface);       
}

// Retrieve a mesh texture handle (usually for VC panel textures)
// meshTextureID = vessel-specific constant that is translated by the parent vessel's MeshTextureIDToTextureIndex method to a texture index specific to the vessel's .msh file; it is also associated with a given vessel mesh, again determined by 
// hMesh typically the VC mesh containing the panel mesh to be retrieved; may not be null.
SURFHANDLE Area::GetMeshTextureHandle(const int meshTextureID) const
{
    // We never use a valid meshTextureID of < 0, so it is good to trap that here because
    // it means we tried to use an area in the VC that does not have a mesh texture handle passed in to
    // the Area constructor.
    _ASSERTE(meshTextureID >= 0);

    // Have our parent vessel translate the arbitrary, vessel-specific meshTextureID to an actual 
    // texture index from the vessel's .msh file.
    MESHHANDLE hMesh;  // initialized by MeshTextureIDToTextureIndex below (passed by ref)
    const DWORD dwTextureIndex = GetVessel().MeshTextureIDToTextureIndex(meshTextureID, hMesh);
    SURFHANDLE hSurf = oapiGetTextureHandle(hMesh, dwTextureIndex);

    _ASSERTE(hSurf != nullptr);
    return hSurf;
}

//---------------------------------------------------------------------------------
// These were prototype D3D9 performance enhancement methods; see method comments in Area.h for details
//---------------------------------------------------------------------------------
/* Per jarmonik:
    
    1) m_mainSurface = CreateSurface(IDB_HORIZON);
    2) oapiSetSurfaceColourKey(m_mainSurface, 0xFF000000);  // black = transparent
    
    3) SURFHANDLE GDISurface = oapiCreateSurface(96, 96);  // make empty surface for entire area
    
    4) HDC hDC = oapiGetDC(GDISurface)
    5) Draw GDI stuff
    6) oapiReleaseDC(GDISurface, hDC);
    
    7) oapiBlt (surf, GDISurface, ...)  // When a GDI surface is blitted no scaling is allowed during the blit.
    8) oapiBlt (surf, m_mainSurface,... )   // for every m_mainSurface blit

    Unfortunately, that does not render correctly, so I decided to just port the MDA temperature display
    and the artificial horizon to use the sketchpad API instead.
*/

// replaces oapiSetSurfaceColourKey; as a sanity check, we should never call this with ck == 0 (no transparency)
void Area::SetSurfaceColorKey(const SURFHANDLE surf, const DWORD ck)
{
    oapiSetSurfaceColourKey(surf, ck);  // we set transparency on the NORMAL surface here, never the CACHED surface
    _ASSERTE(ck != 0);
}

// replaces oapiGetDC
HDC Area::GetDC(const SURFHANDLE surf)
{
    return oapiGetDC(surf);
}

// replaces oapiReleaseDC
void Area::ReleaseDC(const SURFHANDLE surf, const HDC hDC)
{
    oapiReleaseDC(surf, hDC);
}
