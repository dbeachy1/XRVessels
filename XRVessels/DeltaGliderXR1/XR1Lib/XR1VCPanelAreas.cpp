// ==============================================================
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// Copyright 2006-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR1VCPanelAreas.cpp
// Handles non-component main panel areas
// ==============================================================

#include "resource.h"
#include "AreaIDs.h"

#include "DeltaGliderXR1.h"
#include "XR1VCPanelAreas.h"

//----------------------------------------------------------------------------------

// NOTE: there are four of these buttons defined, each with a different area ID

//
// Constructor
// vessel = our vessel handle
// panelCoordinates = absolute coordinates of this area on the parent instrument panel
// areaID = unique Orbiter area ID
// no redrawing here, so no meshTextureID required
VCHudModeButtonArea::VCHudModeButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    XR1Area(parentPanel, panelCoordinates, areaID)
{
}

// Activate this area
void VCHudModeButtonArea::Activate()
{
    Area::Activate();  // invoke superclass method
    oapiVCRegisterArea(GetAreaID(), PANEL_REDRAW_MOUSE, PANEL_MOUSE_LBDOWN | PANEL_MOUSE_ONREPLAY);
}

// Redraw this area
// event = Orbiter event flags
// returns: true if area redrawn, false if not
bool VCHudModeButtonArea::Redraw3D(const int event, const SURFHANDLE surf)
{
    if (GetXR1().vcmesh == nullptr) 
        return false;       // no mesh to change

    // redraw all four buttons since we send in all 4 sets of vertices
    NTVERTEX vtx[12];
	GROUPEDITSPEC ges;
	ges.flags = GRPEDIT_VTXTEXU;
	ges.Vtx = vtx;
	ges.nVtx = 12;
	ges.vIdx = nullptr;

	for (int i = 0; i < 3; i++) 
    {
		bool hilight = (oapiGetHUDMode() == 3-i);
		vtx[i*4  ].tu = vtx[i*4+1].tu = (hilight ? 0.1543f : 0.0762f);
		vtx[i*4+2].tu = vtx[i*4+3].tu = (hilight ? 0.0801f : 0.0020f);
	}
	oapiEditMeshGroup (GetXR1().vcmesh, MESHGRP_VC_HUDMODE, &ges);

    return true;
}

// Handle mouse events for this area
// event = Orbiter event flags
// coords = mouse coordinates relative to the area
// returns: true if event processed, false if not
bool VCHudModeButtonArea::ProcessVCMouseEvent(const int event, const VECTOR3 &coords)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    // AID index delta is HUD mode (0-n)
    oapiSetHUDMode(HUD_NONE + GetAreaID() - AID_HUDBUTTON1);

    GetXR1().PlaySound(GetXR1().SwitchOn, DeltaGliderXR1::ST_Other, QUIET_CLICK);    // light click

    return true;
}

//----------------------------------------------------------------------------------

VCAutopilotButtonArea::VCAutopilotButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    XR1Area(parentPanel, panelCoordinates, areaID)
{
}
void VCAutopilotButtonArea::Activate()
{
    Area::Activate();  // invoke superclass method
    // no redrawing here, so no meshTextureID required
    oapiVCRegisterArea(GetAreaID(), PANEL_REDRAW_MOUSE, PANEL_MOUSE_LBDOWN);
}

bool VCAutopilotButtonArea::Redraw3D(const int event, const SURFHANDLE surf)
{
    if (GetXR1().vcmesh == nullptr) 
        return false;       // no mesh to change

    NTVERTEX vtx[24];
	GROUPEDITSPEC ges;
	ges.flags = GRPEDIT_VTXTEXU;
	ges.Vtx = vtx;
	ges.nVtx = 24;
	ges.vIdx = nullptr;

	for (int i = 0; i < 6; i++) 
    {
		bool hilight = GetXR1().GetNavmodeState (i + NAVMODE_KILLROT);
		vtx[i*4  ].tu = vtx[i*4+1].tu = (hilight ? 0.1172f : 0.0f);
		vtx[i*4+2].tu = vtx[i*4+3].tu = (hilight ? 0.2344f : 0.1172f);
	}
	oapiEditMeshGroup (GetXR1().vcmesh, MESHGRP_VC_NAVMODE, &ges);

    return true;
}
bool VCAutopilotButtonArea::ProcessVCMouseEvent(const int event, const VECTOR3 &coords)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    // AID index delta is Nav mode index (0-5)
    int mode = GetAreaID() - AID_NAVBUTTON1 + 1;
    GetVessel().ToggleNavmode(mode);

   if (event & PANEL_MOUSE_LBDOWN)
        GetXR1().PlaySound((GetVessel().GetNavmodeState(mode) ? GetXR1().SwitchOn : GetXR1().SwitchOff), DeltaGliderXR1::ST_Other);

    return true;
}

//----------------------------------------------------------------------------------

// reused for many VC switches; no redraw necessary
// pDoorHandler is a function pointer to the door handler to be invoked with the new door status
// no redrawing here, so no meshTextureID required
VCToggleSwitchArea::VCToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, void (DeltaGliderXR1::*pDoorHandler)(DoorStatus), const DoorStatus activatedStatus) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_pDoorHandler(pDoorHandler), m_activatedStatus(activatedStatus)
{
}
void VCToggleSwitchArea::Activate()
{
    Area::Activate();  // invoke superclass method
    oapiVCRegisterArea(GetAreaID(), PANEL_REDRAW_NEVER, PANEL_MOUSE_LBDOWN);
}

bool VCToggleSwitchArea::ProcessVCMouseEvent(const int event, const VECTOR3 &coords)
{
    // if crew is incapacitated, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())
        return false;

    // invoke handler to process this event
    (GetXR1().*m_pDoorHandler)(m_activatedStatus);   

    return true;
}
