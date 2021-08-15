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
// DeltaGliderXR1_VC.cpp
// Virtual cockpit-only methods
// ==============================================================

#include "DeltaGliderXR1.h"
#include "meshres.h"
#include <stdio.h>
#include <math.h>

#include "Component.h"  
#include "Area.h"  
#include "AreaIDs.h"

// --------------------------------------------------------------
// WARNING: YOU *MUST* OVERRIDE EACH OF THESE METHODS IN EACH
// VESSEL'S SUBCLASS!  ITS BEHAVIOR IS MESH-SPECIFIC!
// --------------------------------------------------------------

// --------------------------------------------------------------
// Load virtual cockpit mode
// --------------------------------------------------------------

bool DeltaGliderXR1::clbkLoadVC (int id)
{
    // activate the requested panel
    bool retCode = clbkLoadPanel(VC_PANEL_ID_BASE + id);  // NOTE: this is a VC panel number!

    SetXRCameraDirection (_V(0,0,1)); // forward

    if (retCode)
        UpdateVCMesh();

    return retCode;
}

void DeltaGliderXR1::UpdateVCMesh()
{
    if (vcmesh)
    {
        // hide pilot head if in VCPILOT position
        SetMeshGroupVisible(vcmesh, 138, !(campos == CAM_VCPILOT));
    }
 }

// Update VC status indicators; invoked from clbkPostStep
void DeltaGliderXR1::UpdateVCStatusIndicators()
{
    if (!vcmesh) 
        return;     // mesh not loaded yet

    // for efficiency, exit immediately if not in VC mode
    if (oapiCockpitMode() != COCKPIT_VIRTUAL)
        return;

    float x, xon = 0.845f, xoff = 0.998f;
	double d;

	static NTVERTEX vtx[16];
	static WORD vidx[16] = {0,1,4,5,20,21,8,9,24,25,16,17,12,13,28,29};
	GROUPEDITSPEC ges;
	ges.flags = GRPEDIT_VTXTEXU;
	ges.nVtx = 16;
	ges.vIdx = vidx;
	ges.Vtx = vtx;
    const double simt = GetAbsoluteSimTime();
	// gear indicator
	x = (gear_status == DOOR_CLOSED ? xoff : gear_status == DOOR_OPEN ? xon : modf(simt, &d) < 0.5 ? xon : xoff);
	vtx[0].tu = vtx[1].tu = x;

	// retro cover indicator
	x = (rcover_status == DOOR_CLOSED ? xoff : rcover_status == DOOR_OPEN ? xon : modf(simt, &d) < 0.5 ? xon : xoff);
	vtx[2].tu = vtx[3].tu = x;

	// airbrake indicator
	x = (brake_status == DOOR_CLOSED ? xoff : brake_status == DOOR_OPEN ? xon : modf(simt, &d) < 0.5 ? xon : xoff);
	vtx[4].tu = vtx[5].tu = x;

	// nose cone indicator
	x = (nose_status == DOOR_CLOSED ? xoff : nose_status == DOOR_OPEN ? xon : modf(simt, &d) < 0.5 ? xon : xoff);
	vtx[6].tu = vtx[7].tu = x;

	// top hatch indicator
	x = (hatch_status == DOOR_CLOSED ? xoff : hatch_status == DOOR_OPEN ? xon : modf(simt, &d) < 0.5 ? xon : xoff);
	vtx[8].tu = vtx[9].tu = x;

	// radiator indicator
	x = (radiator_status == DOOR_CLOSED ? xoff : radiator_status == DOOR_OPEN ? xon : modf(simt, &d) < 0.5 ? xon : xoff);
	vtx[10].tu = vtx[11].tu = x;

	// outer airlock indicator
	x = (olock_status == DOOR_CLOSED ? xoff : olock_status == DOOR_OPEN ? xon : modf(simt, &d) < 0.5 ? xon : xoff);
	vtx[12].tu = vtx[13].tu = x;

	// inner airlock indicator
	x = (ilock_status == DOOR_CLOSED ? xoff : ilock_status == DOOR_OPEN ? xon : modf (simt, &d) < 0.5 ? xon : xoff);
	vtx[14].tu = vtx[15].tu = x;

	oapiEditMeshGroup (vcmesh, MESHGRP_VC_STATUSIND, &ges);
}

void DeltaGliderXR1::SetPassengerVisuals ()
{
    // Note: in the DG mesh the passengers in external (non-VC) view
    // are in the same group as the external mesh itself.  Therefore, we can only 
    // hide/show passengers meshes in VC mode.
    if (!(vcmesh && exmesh))   // VC must be visible or nothing to do
        return;

    static int vcpsngridx[4] = {123, 124, 125, 126};
    static int vcvisoridx[4] = {130, 131, 132, 133};

    // TODO: add code to add/remove pilot mesh here

    // start @ index 1 to skip pilot at index 0
    for (int i = 1; i < MAX_PASSENGERS; i++) 
    {
#ifdef MMU
        const char *pName = GetCrewNameBySlotNumber(i);
        const bool crewMemberOnBoard = (strlen(pName) > 0);

        const int meshIdx = i-1;    // make 0-based
        SetMeshGroupVisible(vcmesh, vcpsngridx[meshIdx], crewMemberOnBoard);  // make passenger visible if crew member on board
        SetMeshGroupVisible(vcmesh, vcvisoridx[meshIdx], crewMemberOnBoard);  // make visor visible if crew member on board
#endif
    }
}
