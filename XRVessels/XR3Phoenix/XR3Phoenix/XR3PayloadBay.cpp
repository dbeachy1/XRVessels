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
// XR3Phoenix implementation class
//
// XR3Payload.cpp
// Class defining the XR3's payload bay
// ==============================================================

#include "XR3Phoenix.h"
#include "XR3PayloadBay.h"
#include "XRPayloadBaySlot.h"

// TODO: update this for the XR3's actual payload bay; this current file is simply cloned from the XR5 for template purposes

//-------------------------------------------------------------------------

// Constructor
XR3PayloadBay::XR3PayloadBay(VESSEL &parentVessel) :
    XR1PayloadBay(parentVessel)
{
    // Define our physical payload bay layout by creating each of our 36 payload slots and then linking them together.
    // NOTE: if you change this code you must update PAYLOAD_BAY_SLOT_COUNT in XR3Globals.cpp to match!
    
    // This is the forward-right lower corner of slots 1, 2, and 4.  The entire bay is based off of these coordinates.
    const VECTOR3 slot1ForwardBottomRightCorner = _V( 6.696994, -0.070681, 4.077764);   
    const VECTOR3 slot3ForwardBottomRightCorner = _V( 1.217,    -0.070681, 4.077764);   
    const VECTOR3 slot4ForwardBottomRightCorner = _V(-1.878,    -0.070681, 4.077764);   

    // defines the distance from the lower forward corner to the center of the slot
    // For that we need -X, +Y, and -Z adjustments
    const VECTOR3 deltaToCenter = _V(-PAYLOAD_SLOT_DIMENSIONS.x / 2, PAYLOAD_SLOT_DIMENSIONS.y / 2, -PAYLOAD_SLOT_DIMENSIONS.z / 2);    

    // these define the starting grid centerpoints 
    VECTOR3 slotCenter[5]; 
    slotCenter[0] = slot1ForwardBottomRightCorner + deltaToCenter;        // slot 1
    slotCenter[1] = slotCenter[0] + _V(-PAYLOAD_SLOT_DIMENSIONS.x, 0, 0); // slot 2: shift left one slot
    slotCenter[2] = slot3ForwardBottomRightCorner + deltaToCenter;        // slot 3
    slotCenter[3] = slot4ForwardBottomRightCorner + deltaToCenter;        // slot 4
    slotCenter[4] = slotCenter[3] + _V(-PAYLOAD_SLOT_DIMENSIONS.x, 0, 0); // slot 5: shift left one slot

    // define center slot width in meters; this is a special slot that is wider than standard slots
    const VECTOR3 CENTER_PAYLOAD_SLOT_DIMENSIONS = _V(3.650894, PAYLOAD_SLOT_DIMENSIONS.y, PAYLOAD_SLOT_DIMENSIONS.z);

// all slots are standard size except center slots, which are wider
// NOTE: slot columns are zero-based here
#define SLOTDIM(columnNumber) ((columnNumber == 2) ? CENTER_PAYLOAD_SLOT_DIMENSIONS : PAYLOAD_SLOT_DIMENSIONS)

    // Level 1: four rows of five slots each; the center slot has extra space on each side, 
    // but we do not leverage that in order to keep the payload logic manageable.

    // Rows go forward -> aft
    // NOTE: neighbors will be wired up later
    int slotNumber = 1; // index of each slot
    for (int rowNumber = 0; rowNumber < 4; rowNumber++)
    {
        // adjust with -Z * rowNumber (moving aft)
        const VECTOR3 slotRowDelta = _V(0, 0, -PAYLOAD_SLOT_DIMENSIONS.z * rowNumber);
        
        for (int columnNumber = 0; columnNumber < 5; columnNumber++)
            AddSlot(new XRPayloadBaySlot(slotNumber++, slotCenter[columnNumber] + slotRowDelta, *this, SLOTDIM(columnNumber), 1, _COORD2(columnNumber, rowNumber)));
    }

    // Level 2: four rows of three slots each
    slotNumber = 21;  
    for (int rowNumber = 0; rowNumber < 4; rowNumber++)
    {
        // adjust with -Z * rowNumber (moving aft) and +Y (move up one row)
        const VECTOR3 slotRowDelta = _V(0, PAYLOAD_SLOT_DIMENSIONS.y, -PAYLOAD_SLOT_DIMENSIONS.z * rowNumber);
        
        for (int columnNumber = 1; columnNumber <= 3; columnNumber++)
            AddSlot(new XRPayloadBaySlot(slotNumber++, slotCenter[columnNumber] + slotRowDelta, *this, SLOTDIM(columnNumber), 2, _COORD2(columnNumber, rowNumber)));
    }

     // Level 3: four rows of one slot
    slotNumber = 33;  
    for (int rowNumber = 0; rowNumber < 4; rowNumber++)
    {
        // adjust with -Z * rowNumber (moving aft) and +Y (move up one row)
        const VECTOR3 slotRowDelta = _V(0, PAYLOAD_SLOT_DIMENSIONS.y * 2, -PAYLOAD_SLOT_DIMENSIONS.z * rowNumber);
        AddSlot(new XRPayloadBaySlot(slotNumber++, slotCenter[2] + slotRowDelta, *this, SLOTDIM(2), 3, _COORD2(2, rowNumber)));
    }

    //----------------------------------------------------------------------------------
    // Wire up each slot's neighbors; each slot has exactly six neighbors.  
    // A nullptr neighbor means the edge of the bay.
    // By default all neighbors are nullptr, so we only have to fill in the adjacent slots.
    //----------------------------------------------------------------------------------

    // Level 1
    // Rows go forward -> aft; left/right is when facing forward (toward the nose)
    for (int rowNumber = 0; rowNumber < 4; rowNumber++)
    {
        for (int columnNumber = 1; columnNumber <= 5; columnNumber++)  // 1-based for slot clarity
        {
            const int slotNumber = (rowNumber * 5) + columnNumber;
            XRPayloadBaySlot *pSlot = GetSlot(slotNumber);

            // aft neighbor (-Z)
            if (rowNumber < 3)    // no aft neighbors for the last row
                pSlot->SetNeighbor(NEIGHBOR::MINUSZ, GetSlot(slotNumber + 5));

            // forward neighbor (+Z)
            if (rowNumber > 0)  // no forward neighbors for the first row
                pSlot->SetNeighbor(NEIGHBOR::PLUSZ, GetSlot(slotNumber - 5));

            // right neighbor (+X)
            if (columnNumber > 1) // no right neighbors for the first (right-most) column
                pSlot->SetNeighbor(NEIGHBOR::PLUSX, GetSlot(slotNumber - 1));

            // left neighbor (-X)
            if (columnNumber < 5) // no left neighbors for the last (left-most) column
                pSlot->SetNeighbor(NEIGHBOR::MINUSX, GetSlot(slotNumber + 1));

            // above neighbor (+Y)
            if ((columnNumber >= 2) && (columnNumber <= 4))  // three center columns only; 3 per row on the upper level (compared to 5 on the lower level)
                pSlot->SetNeighbor(NEIGHBOR::PLUSY, GetSlot(slotNumber + 19 - (rowNumber * 2)));

            // no below neighbor (-Y); we are on the bottom level
        }
    }

    // Level 2
    // Rows go forward -> aft; left/right is when facing forward (toward the nose)
    for (int rowNumber = 0; rowNumber < 4; rowNumber++)
    {
        for (int columnNumber = 2; columnNumber <= 4; columnNumber++)  // 2-based for slot clarity
        {
            const int slotNumber = 19 + (rowNumber * 3) + columnNumber;
            XRPayloadBaySlot *pSlot = GetSlot(slotNumber);

            // aft neighbor (-Z)
            if (rowNumber < 3)    // no aft neighbors for the last row
                pSlot->SetNeighbor(NEIGHBOR::MINUSZ, GetSlot(slotNumber + 3));

            // forward neighbor (+Z)
            if (rowNumber > 0)  // no forward neighbors for the first row
                pSlot->SetNeighbor(NEIGHBOR::PLUSZ, GetSlot(slotNumber - 3));

            // right neighbor (+X)
            if (columnNumber > 2)
                pSlot->SetNeighbor(NEIGHBOR::PLUSX, GetSlot(slotNumber - 1));

            // left neighbor (-X)
            if (columnNumber < 4)
                pSlot->SetNeighbor(NEIGHBOR::MINUSX, GetSlot(slotNumber + 1));

            // above neighbor (+Y)
            if (columnNumber == 3)  // center column only
                pSlot->SetNeighbor(NEIGHBOR::PLUSY, GetSlot(slotNumber + 11 - (rowNumber * 2)));
            
            // below neighbor (-Y)
            pSlot->SetNeighbor(NEIGHBOR::MINUSY, GetSlot(slotNumber - 19 + (rowNumber * 2)));
        }
    }

    // Level 3
    // Rows go forward -> aft; left/right is when facing forward (toward the nose)
    for (int rowNumber = 0; rowNumber < 4; rowNumber++)
    {
        const int slotNumber = 33 + rowNumber;
        XRPayloadBaySlot *pSlot = GetSlot(slotNumber);

        // aft neighbor (-Z)
        if (rowNumber < 3)    // no aft neighbors for the last row
            pSlot->SetNeighbor(NEIGHBOR::MINUSZ, GetSlot(slotNumber + 1));

        // forward neighbor (+Z)
        if (rowNumber > 0)  // no forward neighbors for the first row
            pSlot->SetNeighbor(NEIGHBOR::PLUSZ, GetSlot(slotNumber - 1));

        // no right for left neighbors on this row

        // no above neighbor (+Y); this is the top level

        // below neighbor (-Y)
        pSlot->SetNeighbor(NEIGHBOR::MINUSY, GetSlot(slotNumber - 11 + (rowNumber * 2)));
    }
}

// Returns the ship-local coordinates to deploy the selected slot payload while landed
VECTOR3 XR3PayloadBay::GetLandedDeployToCoords(const int slotNumber)
{
    _ASSERTE(slotNumber > 0);
    _ASSERTE(slotNumber <= GetSlotCount());

    double xFactor;
    double xAdjustment = 0;
    if (slotNumber <= 20)
    {
        // level 1 = LEFT SIDE of ship (facing forward)  (-X)
        xFactor = -1.0;
    }
    else if (slotNumber <= 32)
    {
        // level 2 = RIGHT SIDE of ship (facing forward) (+X)
        xFactor = 1.0;
    }
    else
    {
        // level 3 = RIGHT SIDE of ship (facing forward) (+X) + (3.095 * 2)
        // This will leave exactly a symmetrical gap between levels 2 and 3 payloads on the ground
        xFactor = 1.0;
        xAdjustment = (3.095 * 2);
    }

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

//-------------------------------------------------------------------------
// Vanguard class methods
//-------------------------------------------------------------------------

// create our payload bay; invoked by clbkSetClassCaps
void XR3Phoenix::CreatePayloadBay()
{
    // create our payload bay
    m_pPayloadBay = new XR3PayloadBay(*this);

    // NOTE: always create this LAST so that the payload indicies are consistent and zero-based; 
    // in any case, do NOT change it after the ship is out of Beta because the scenarios will break!
    //
    // Create our dummy bay vessel attachment point; we want this to be FIRST so that the payload bay slot
    // indices begin a 1 in the scenario file; i.e., the numbers will match the slots.
    VECTOR3 &attachVector = _V(0, 3.766, -23.537);
    m_dummyAttachmentPoint = CreateAttachment(false, attachVector, _V(0, -1.0, 0), _V(0, 0, 1.0), "XRDUMMY");
}

