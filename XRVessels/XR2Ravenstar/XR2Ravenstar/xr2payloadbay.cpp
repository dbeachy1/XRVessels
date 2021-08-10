// ==============================================================
// XR2Ravenstar implementation class
//
// Copyright 2008-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR2Payload.cpp
// Class defining the XR2's payload bay
// ==============================================================

#include "XR2Ravenstar.h"
#include "XR2PayloadBay.h"
#include "XRPayloadBaySlot.h"

// Note: all the common payload bay code for XR vessels is in the XR1PayloadBay class

//-------------------------------------------------------------------------

// Constructor
XR2PayloadBay::XR2PayloadBay(VESSEL &parentVessel) :
    XR1PayloadBay(parentVessel)
{
    // Define our physical payload bay layout by creating each of our 3 payload slots and then linking them together.
    // NOTE: if you change this code you must update PAYLOAD_BAY_SLOT_COUNT in XR2Globals.cpp to match!
    
    // Note: these directions are facing REARWARD; i.e., on the payload camera
    // This is the forward-right (starboard) lower corner of slot #1.  The entire bay is based off of these coordinates.
    // ORG: const VECTOR3 slot1ForwardBottomRightCorner = _V(1.726, -0.444, 3.154);   
    // Note: we must explicitly define all 3 slot locations here because of the (tiny) gap between slot 1 and slots 2-3, and we
    // want slots 2-3->aft end of bay to butt up against each other.
    const VECTOR3 slot1ForwardBottomRightCorner = _V(1.726, -0.444, 3.104);   // Z adjusted to aft edge of crew hatch ring.
    const VECTOR3 slot2ForwardBottomRightCorner = _V(1.726, -0.444, 1.032);   // Z dimension is against slot 3, but not slot 1
    const VECTOR3 slot3ForwardBottomRightCorner = _V(1.726, -0.444, -0.422);  // slot reaches rear of bay

    // Three rows of one slot each; only one level.
    // NOTE: neighbors will be wired up later
    // We need the distance to the CENTER of each slot; for that we need -X, +Y, and -Z adjustments.
    const VECTOR3 deltaToCenterSlot1      = _V(-PAYLOAD_SLOT1_DIMENSIONS.x, PAYLOAD_SLOT1_DIMENSIONS.y, -PAYLOAD_SLOT1_DIMENSIONS.z) / 2;    
    const VECTOR3 deltaToCenterSlots2and3 = _V(-PAYLOAD_SLOT_DIMENSIONS.x,  PAYLOAD_SLOT_DIMENSIONS.y,  -PAYLOAD_SLOT_DIMENSIONS.z) / 2;    

    // slot 1
    VECTOR3 slotCenter = slot1ForwardBottomRightCorner + deltaToCenterSlot1;
    AddSlot(new XRPayloadBaySlot(1, slotCenter, *this, PAYLOAD_SLOT1_DIMENSIONS, 1, _COORD2(0, 0)));

    // slot 2
    slotCenter = slot2ForwardBottomRightCorner + deltaToCenterSlots2and3;
    AddSlot(new XRPayloadBaySlot(2, slotCenter, *this, PAYLOAD_SLOT_DIMENSIONS, 1, _COORD2(0, 0)));

    // slot 3
    slotCenter = slot3ForwardBottomRightCorner + deltaToCenterSlots2and3;
    AddSlot(new XRPayloadBaySlot(3, slotCenter, *this, PAYLOAD_SLOT_DIMENSIONS, 1, _COORD2(0, 0)));
    
    //----------------------------------------------------------------------------------
    // Wire up each slot's neighbors
    // A NULL neighbor means the edge of the bay.
    // By default all neighbors are NULL, so we only have to fill in the adjacent slots.
    //----------------------------------------------------------------------------------

    XRPayloadBaySlot *pSlot1 = GetSlot(1);
    XRPayloadBaySlot *pSlot2 = GetSlot(2);
    XRPayloadBaySlot *pSlot3 = GetSlot(3);

    pSlot1->SetNeighbor(MINUSZ, pSlot2);
    pSlot2->SetNeighbor(PLUSZ, pSlot1);
    pSlot2->SetNeighbor(MINUSZ, pSlot3);
    pSlot3->SetNeighbor(PLUSZ, pSlot2);
}

// Returns the ship-local coordinates to deploy the selected slot payload while landed
VECTOR3 XR2PayloadBay::GetLandedDeployToCoords(const int slotNumber)
{
    _ASSERTE(slotNumber > 0);
    _ASSERTE(slotNumber <= GetSlotCount());

    double xFactor = -1.0;  // LEFT SIDE of ship (facing forward)
    double xAdjustment = 0;
    
    // Replicate the layout of the payload bay on the left and right side of the ship;
    // each slot is "locked" to a fixed deploy position.

    // Use a delta from the level-1 slot's attachment point to deploy the payload.
    XRPayloadBaySlot *pSlot = GetSlot(slotNumber);  // will never be null
    VECTOR3 slotAttachmentPoint = pSlot->GetLocalCoordinates();

    // Must add in the *child vessel's* attachment point coordinates as well, if any
    VECTOR3 childAttachmentPoint = _V(0,0,0);
    VESSEL *pChild = pSlot->GetChild();  // may be null
    VECTOR3 deployCoordinates = _V(0, 0, 0);
    if (pChild != nullptr)
    {
        const XRPayloadClassData &pcd = XRPayloadClassData::GetXRPayloadClassDataForClassname(pChild->GetClassName());
        ATTACHMENTHANDLE childAttHandle = pcd.GetAttachmentHandleForPayloadVessel(*pChild);  // will always be valid
        _ASSERTE(childAttHandle != nullptr);  // sanity check
        VECTOR3 dir, rot;  // not used here
        pChild->GetAttachmentParams(childAttHandle, childAttachmentPoint, dir, rot);

        // add GroundDeploymentAdjustment payload setting, if any
        deployCoordinates += pcd.GetGroundDeploymentAdjustment();
    }

    deployCoordinates += _V(
        ((slotAttachmentPoint.x + PAYLOAD_BAY_DELTAX_TO_GROUND) * xFactor) + xAdjustment, 
        PAYLOAD_BAY_DELTAY_TO_GROUND,   // all slots are level on ground, regardless of their attachment point's Y value
        slotAttachmentPoint.z);

    // now subtract the child attachment point so it remains lined up correctly when deployed
    // this is necessary because, for example, a +Z attachement point will push the vessel AFT in the bay (-Z)
    deployCoordinates -= childAttachmentPoint;      // childAttachmentPoint may be 0,0,0

    return deployCoordinates;
}

//=========================================================================
// XR2Ravenstar methods
//=========================================================================

// create our payload bay; invoked by clbkSetClassCaps
void XR2Ravenstar::CreatePayloadBay()
{
    // create our payload bay
    m_pPayloadBay = new XR2PayloadBay(*this);

    // NOTE: always create this LAST so that the payload indicies are consistent and zero-based; 
    // in any case, do NOT change it after the ship is out of Beta because the scenarios will break!
    //
    // Create our dummy bay vessel attachment point; we want this to be FIRST so that the payload bay slot
    // indices begin at 1 in the scenario file; i.e., the numbers will match the slots.
    VECTOR3 &attachVector = _V(0.0, 1.079, -2.977);
    m_dummyAttachmentPoint = CreateAttachment(false, attachVector, _V(0, -1.0, 0), _V(0, 0, 1.0), "XRDUMMY");
}
