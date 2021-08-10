// ==============================================================
// XR2Ravenstar implementation class
//
// Copyright 2008-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR2Ravenstar_VC.cpp
// Virtual-cockpit-only methods
// ==============================================================

#include "XR2Ravenstar.h"
#include <stdio.h>
#include <math.h>

#include "Component.h"  
#include "Area.h"  
#include "AreaIDs.h"
#include "meshres.h"

// --------------------------------------------------------------
// WARNING: YOU *MUST* IMPLEMENT EACH OF THESE METHODS IN EACH
// VESSEL'S SUBCLASS!  ITS BEHAVIOR IS MESH-SPECIFIC!
// --------------------------------------------------------------

// --------------------------------------------------------------
// Load virtual cockpit mode
// --------------------------------------------------------------

bool XR2Ravenstar::clbkLoadVC (int id)
{
    // activate the requested panel
    const int activeVCPanel = VC_PANEL_ID_BASE + id;
    bool retCode = clbkLoadPanel(activeVCPanel);  // NOTE: this is a VC panel number!

    // Note: camera direction was already set by clbkLoadPanel; do not set it here

    return retCode;
}

void XR2Ravenstar::SetPassengerVisuals()
{
#ifdef MMU
    if (!exmesh)  
        return;

    // show or hide the commander and pilot
    bool commanderOnBoard = false;
    bool pilotOnBoard = false;
    char pRank[CrewMemberRankLength+1];
    for (int i=0; i < MAX_PASSENGERS; i++) 
    {
        const char *pUmmuMisc = CONST_UMMU_XR2(this).GetCrewMiscIdBySlotNumber(i);
        const bool crewMemberOnBoard = (strlen(pUmmuMisc) > 0);

        if (crewMemberOnBoard)
        {
            strncpy(pRank, RetrieveRankForUMmuMisc(pUmmuMisc), CrewMemberRankLength);
            // check for commander and pilot by RANK (case-sensitive)
            if (strcmp(pRank, "Commander") == 0)
                commanderOnBoard = true;
            else if (strcmp(pRank, "Pilot") == 0)
                pilotOnBoard = true;
        }
    }

#define SizeOfGrp(g) (sizeof(g) / sizeof(int))
    const UINT commanderGrp[] = { GRP_lee, GRP_lee_fixup_1, GRP_lee_fixup_2 };
    const UINT pilotGrp[] = { GRP_kara, GRP_kara_fixup_1, GRP_kara_fixup_2 };

    SetMeshGroupsVisibility(commanderOnBoard, exmesh, SizeOfGrp(commanderGrp), commanderGrp);
    SetMeshGroupsVisibility(pilotOnBoard, exmesh, SizeOfGrp(pilotGrp), pilotGrp);
#endif
}

// Update VC status indicators; invoked from clbkPostStep
void XR2Ravenstar::UpdateVCStatusIndicators()
{
    return;     // TODO: implement this method if we need it

    if (!exmesh) 
        return;     // mesh not loaded yet

    // for efficiency, exit immediately if not in VC mode
    if (!IsCameraVC())
        return;
}
