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
// XRPayloadBay.cpp
// Models a payload bay for an XR-class vessel
// ==============================================================

#include "XRPayloadBay.h"
#include "XRPayloadBaySlot.h"
#include "VesselAPI.h"

//-------------------------------------------------------------------------
// XRPayloadBaySlot
//-------------------------------------------------------------------------

// Standard Constructor
// slotNumber = numerical slot # used as a reference by the pilot; 1...n
// localCoordinates = SHIP-RELATIVE coordinates to the center of the slot in the bay.
// slotDimensions : typically PAYLOAD_SLOT_DIMENSIONS; however, non-standard slots may be defined, too
// level = bay level; for most ships this will always be 1
// levelCoordinates: 0-based grid coordinates on this level: 0,0 = bottom-left (facing rearwards), n,n = top-right
//
// Note: slot is empty and enabled on creation
XRPayloadBaySlot::XRPayloadBaySlot(const int slotNumber, const VECTOR3 &localCoordinates, XRPayloadBay &parentBay, const VECTOR3 &slotDimensions, const int level, const COORD2 &levelGridCoordinates) :
    m_parentBay(parentBay), m_hAttachmentHandle(0), m_slotNumber(slotNumber), m_localCoordinates(localCoordinates), 
    m_isEnabled(true), m_dimensions(slotDimensions), m_level(level), m_levelGridCoordinates(levelGridCoordinates)
{
    // all neighbors are null
    for (int i=0; i < 6; i++)
        m_neighbors[i] = nullptr;

    // create an attachment point on our parent vessel: attachment point is in the *center* of the slot
    m_hAttachmentHandle = parentBay.GetParentVessel().CreateAttachment(false, localCoordinates, _V(0, -1.0, 0), _V(0, 0, 1.0), "XRCARGO");
}

//-------------------------------------------------------------------------

// Attach a child to this primary slot.
// Note: this does NOT do any distance/bearing checks; that should be handled by the caller before invoking this method.
//
// Returns: true on success, false if child vessel is invalid or if there is insufficient room at this slot
// for the child vessel to fit.
bool XRPayloadBaySlot::AttachChild(VESSEL &childVessel)
{
     // reserve space for this object using this as the primary slot
    if (CheckSlotSpace(childVessel) == false)
        return false;   // no room in bay!

    //
    // Object will fit; attach it.
    //

    const XRPayloadClassData &pcd = XRPayloadClassData::GetXRPayloadClassDataForClassname(childVessel.GetClassName());
    ATTACHMENTHANDLE childAttPointHandle = XRPayloadClassData::GetAttachmentHandleForPayloadVessel(childVessel);

    // attach in this slot, which is the primary slot
    const bool retVal = GetParentVessel().AttachChild(childVessel.GetHandle(), GetAttachmentHandle(), childAttPointHandle);

    // if the attach succeeded, refresh the slot states in the bay
    if (retVal)
        GetParentBay().RefreshSlotStates();  // enable/disable slots based on payload in bay

    return retVal;
}

// Detach the child vessel attached to this primary attachment point, if any.
// Note: the child is detached using the supplied Delta-V and the bay space deallocated.
// Returns true on success, false if no child present or the child refused to detach.
bool XRPayloadBaySlot::DetachChild(const double deltaV)
{
    VESSEL *pChild = GetChild();
    if (pChild == nullptr)
        return false;   // nothing to detach

    // detach the child vessel
    // NOTE: we must use our PARENT VESSEL'S attachment point here
    const bool retVal = GetParentVessel().DetachChild(GetAttachmentHandle(), deltaV);

    // if the detach succeeded, refresh the slot states in the bay
    if (retVal)
        GetParentBay().RefreshSlotStates();  // enable/disable slots based on payload in bay

    return retVal;
}

// Will return nullptr if child was deleted since it was attached, or if no payload is in this slot.
VESSEL *XRPayloadBaySlot::GetChild() const
{
    VESSEL *retVal = nullptr;

    // see if we have a parent vessel attached to our attachment point
    ATTACHMENTHANDLE hAttachmentHandle = GetAttachmentHandle();  // should never be null!
    OBJHANDLE hChildVessel = GetParentVessel().GetAttachmentStatus(hAttachmentHandle);  // will be NULL if no child vessel attached

    // WARNING: for some reason Orbiter tends to keep vessels alive for at least one frame after they are deleted; i.e., the handle still comes back
    // but it is now invalid!  Therefore, we have to handle that here.
    if (oapiIsVessel(hChildVessel))
        retVal = oapiGetVesselInterface(hChildVessel);

    return retVal;
}   

// Allocate bay slot space.
// Returns true of the specified payload object will fit in this slot, false otherwise
bool XRPayloadBaySlot::CheckSlotSpace(const VESSEL &childVessel) const
{
    // verify that this (the primary slot) is free
    if ((GetChild() != nullptr) || (IsEnabled() == false))
        return false;   // slot occupied!

    // If explicit attachment slots are defined for this child object, ignore hull boundary checks and only check for other attached payloads.
    // Disable each surrounding slot that is occupied by this payload; this primay slot remains enabled, however.
    const char *pParentVesselClassname = GetParentVessel().GetClassName();
    const XRPayloadClassData &pcd = XRPayloadClassData::GetXRPayloadClassDataForClassname(childVessel.GetClassName());

    // validate that childVessel is an XR payload vessel: necessary because this method is exposed via XRVesselCtrl API call
    if (!pcd.IsXRPayloadEnabled())
        return false;
    
    bool isExplicitAttachmentSlot = false;      // assume normal attachment mode
    if (pcd.AreAnyExplicitAttachmentSlotsDefined(pParentVesselClassname))
    {
        // attachment is ONLY permitted if this slot is listed in the explicit attachement slot list for this payload module
        isExplicitAttachmentSlot = pcd.IsExplicitAttachmentSlotAllowed(pParentVesselClassname, GetSlotNumber());
        if (isExplicitAttachmentSlot == false)
            return false;   // explicit slots specified, but this slot is not in the list!
    }

    // This slot (the primary slot) is OK; retrieve the surrounding slots occupied by this candidate vessel.
    vector<const XRPayloadBaySlot *> vOut;  
    bool childClearsHull = GetRequiredNeighborSlotsForCandidateVessel(childVessel, vOut);  // if 'true', the child clears the hull; if 'false', the child IMPACTS the hull

    // If the child impacts the hull, we may ignore it ONLY if "explicit attachment slot" mode is enabled, which assumes that the vessel mesh was explicitly 
    // taylored to fit in this slot.
    if ((childClearsHull == false) && (isExplicitAttachmentSlot == false))
        return false;       // child vessel would impact the hull edge and explicit-latch is not set!

    // If we reach here, the child will clear the hull!  Let's check the neighboring slots next...

    // Iterate through each neighbor slot occupied; each occupied neighbor slot 
    // must be be FREE in order for this candidate vessel to fit.
    for (UINT i=0; i < vOut.size(); i++)
    {
        const XRPayloadBaySlot *pSlot = vOut[i];
        if ((pSlot->GetChild() != nullptr) || (pSlot->IsEnabled() == false))
            return false;   // neighbor slot itself is occupied
    }

    // If we reach here, there is room to attach the candidate vessel!
    return true;
}

// Retrieve a list of all neighboring slots that would be occupied by the supplied candidate vessel.
// Note that we do not check whether the slots are *occupied*; we merely return the slots required if the candidate
// vessel were to be attached in the requested slots.
// childVessel = candidate vessel to be tested in this slot
// vOut = OUTPUT: on exit, will contain a list of slots; if empty, no neighboring slots are occupied
// Returns: returns 'true' if hull edge check OK, or 'false' if vessel would hit the hull edge.
bool XRPayloadBaySlot::GetRequiredNeighborSlotsForCandidateVessel(const VESSEL &childVessel, vector<const XRPayloadBaySlot *> &vOut) const
{
    bool retVal = false;        // assume we do NOT impact the hull along any axis
    
    // Step 1: obtain the child vessel's attachment point, direction, and rotation
    ATTACHMENTHANDLE hChildAttachment = XRPayloadClassData::GetAttachmentHandleForPayloadVessel(childVessel);  // will be null if vessel is not XRPayload-enabled or does not have an attachment point defined
    if (hChildAttachment == nullptr)
        return true;        // no slot data available, so assume edge is OK, too

    // Step 2: obtain the size of the vessel in X,Y,Z lengths (meters)
    const XRPayloadClassData &pcd = XRPayloadClassData::GetXRPayloadClassDataForClassname(childVessel.GetClassName());
    const VECTOR3 &childDimensions = pcd.GetDimensions();

    // Step 3: set the point from which the distance dimensions will be measured (the center of the child's mass), as defined in payload-slot-center coordinates.
    // +X = right (starboard), +Y = straight up, +Z = forward
    // NOTE: the actual *attachment point coordinates* have nothing to do with the center of the payload's mass in its primary slot (this slot!)  
    // That is determined by the 'PrimarySlotCenterOfMassOffset' coordinates.
    const VECTOR3 childCenterOfMass = pcd.GetPrimarySlotCenterOfMassOffset();

    // Step 4: we now have 1) the length of the three vectors for the payload module, and 2) the centerpoint where all 
    // three axes converge, shifted correctly to adjust for the attachment point.  
    // Next we need to compute the endpoints of the three length vectors (X,Y,Z) in *payload-slot-local-coordinates*
    // based on the direction and rotation of the child vessel.
    // However, to make things simpler we can cheat here because we know that the rotation and direction of the payload is locked to the 
    // bay's main axes; i.e, each payload is "latched" into the bay in straight lines, which means the payload slot's vectors are locked with the 
    // payload vessel's vectors.  Therefore, we can simply traverse through the adjacent slots in straight lines along the payload vectors,
    // without have to adjust for angles due to rotated cargo.

    // When checking for collisions, we sweep through each slot we touch in three dimensions; i.e.,
    //       up/down : forward/aft/left/right
    //
    // We must check each slot along each up/down level (or "layer") all the way out; i.e., we must "sweep" all the slots we touch.
    retVal = SweepSlots(childCenterOfMass, childDimensions, vOut);

    return retVal;
}

// Sweep each slot in a cube from supplied the childCenterOfMass centerpoint, using each slot's dimensions (including *this* slot).  
// We mark each slot we touch as occupied in the vOut vector by storing a pointer to it.
//
// NOTE: this code assumes that each LAYER is consistent with other layers; i.e., each vertical layer must have the same number and size of slots.
// However, *within* a given layer each slot may vary in length, width, and height as long as all layers have the same slot dimensions for a 
// given X and Z coordinate pair.
//
// childCenterOfMass = center of the candidate child vessel in slot-local coordinates
// childDimensions = X,Y,Z dimensions of the child vessel; we treat all vessels as cube-shaped
// vOut = OUTPUT: will be populated with neighboring slots that would be occupied
// Returns: 'true' if hull edge check OK, or 'false' if vessel would hit the hull edge along any axis
bool XRPayloadBaySlot::SweepSlots(const VECTOR3 &childCenterOfMass, const VECTOR3 &childDimensions, vector<const XRPayloadBaySlot *> &vOut) const
{
    bool retVal = true;     // no hull impact yet

    // Note: we sweep all axes even if we hit the hull: we need the full data if explict-latch is specified, in which case hull 
    // impacts are ignored.  This allows custom-shaped non-rectangular meshes to latch into the bay in pre-designed bay slots.

    // Also note that we must REVERSE the startingSlotAccessCoordinate (argument #2) for MINUS half-axis sweeps because the value
    // *increases* the adjacent slot space required instead of *decreasing* it.
    
    // sweep vertically to obtain a list of all vertical slots required; each slot will be on a different level
    retVal &= SweepAxis(NEIGHBOR::PLUSY, NEIGHBOR::MINUSY, childCenterOfMass, childDimensions.y, vOut);   // UP/DOWN
    
    // Now walk through each neighboring slot above and below us and then sweep the X and Z vectors for each level.
    // For each level, we must sweep left and right (along the X axis) *for each slot along the Z axis*.
    // Note: vOut will be empty here if no slots on neighboring levels are required.
    const size_t latchedVoutSize = vOut.size();   // must latch the size of just our *vertical* slots here because vOut will grow below
    for (UINT i=0; i < latchedVoutSize; i++)
    {
        // Note: vOut contains the origin slot on each level
        const XRPayloadBaySlot *pLayerOriginSlot = vOut[i];
        
        // obtain the list of slots along the Z axis (forward/aft) for this slot
        vector<const XRPayloadBaySlot *>zAxisVout;  // will contain the *neighbors* required along the Z axis for this slot
        retVal &= pLayerOriginSlot->SweepAxis(NEIGHBOR::PLUSZ, NEIGHBOR::MINUSZ, childCenterOfMass, childDimensions.z, zAxisVout);   // FORWARD/AFT

        // NOTE: the list of Z axis slots here does *NOT* include the origin slot on the origin level.
        // sweep all Z axis slots along the X axis (left/right) as well.  Also add the Z axis slot itself to vOut.
        retVal &= SweepXAxisForSlots(zAxisVout, true, childCenterOfMass, childDimensions.x, vOut);   // RIGHT/LEFT for each slot in zAxisVout

        // Now sweep this Z axis origin slot itself along the X axis
        retVal &= pLayerOriginSlot->SweepAxis(NEIGHBOR::PLUSX, NEIGHBOR::MINUSX, childCenterOfMass, childDimensions.x, vOut);   // RIGHT/LEFT for this layer's *origin* slot
    }

    // Lastly, sweep the origin slot (us!) along the X and Z vectors.  This sweeps the origin level.
    // obtain the list of slots along the Z axis (forward/aft) for this slot
    vector<const XRPayloadBaySlot *>zAxisVout;  // will contain the *neighbors* required along the Z axis for this slot
    retVal &= SweepAxis(NEIGHBOR::PLUSZ, NEIGHBOR::MINUSZ, childCenterOfMass, childDimensions.z, zAxisVout);   // FORWARD/AFT

    // Sweep all neighboring Z axis slots along the X axis (left and right).  Also add the neighboring Z axis slot itself to vOut.
    retVal &= SweepXAxisForSlots(zAxisVout, true, childCenterOfMass, childDimensions.x, vOut);   // RIGHT/LEFT for each slot in zAxisVout

    // now sweep the X axis from *this* slot (the origin)
    retVal &= SweepAxis(NEIGHBOR::PLUSX, NEIGHBOR::MINUSX, childCenterOfMass, childDimensions.x, vOut);   // RIGHT/LEFT

    return retVal;
}

// Sweep the full X axis for the supplied list of slots
// orginSlots = list of origin slots that will be swept along a full axis.  
// addOriginSlotsToVout: true = origin slot itself is added to the vOut vector, false = origin slot itself is *not* added to vOut vector.
// See doc below for more param descriptions.
bool XRPayloadBaySlot::SweepXAxisForSlots(vector<const XRPayloadBaySlot *> &zAxisOriginSlots, const bool addOriginSlotsToVout, const VECTOR3 &childCenterOfMass, const double xAxisLength, vector<const XRPayloadBaySlot *> &vOut) const
{
    bool retVal = true;     // assume no hull contact

    // sweep each slot along the X axis in this Z axis slot
    for (UINT i=0; i < zAxisOriginSlots.size(); i++)
    {
        const XRPayloadBaySlot *pZaxisOriginSlot = zAxisOriginSlots[i];

        // add this slot found in the Z Axis sweep to our master vector as well if requested
        if (addOriginSlotsToVout)
            vOut.push_back(pZaxisOriginSlot);

        // sweep along the X axis from this Z axis slot
        retVal &= pZaxisOriginSlot->SweepAxis(NEIGHBOR::PLUSX, NEIGHBOR::MINUSX, childCenterOfMass, xAxisLength, vOut);   // RIGHT/LEFT
    }

    return retVal;
}

// Sweep a full axis
// axisPlus = +axis side to sweep
// axisMinus = -axis side to sweep
// childCenterOfMass = center of the candidate child vessel in slot-local coordinates
// axisLength = full length of axis to sweep
// vOut = OUTPUT: will contain slots that are required for this axis
// returns: true of hull boundary OK, false if hull boundary reached
bool XRPayloadBaySlot::SweepAxis(const NEIGHBOR axisPlus, const NEIGHBOR axisMinus, const VECTOR3 &childCenterOfMass, const double axisLength, vector<const XRPayloadBaySlot *> &vOut) const
{
    bool retVal = true;     // hull boundary OK

    const double startingAxisCoordinate = GetVectorValueForAxis(axisPlus, childCenterOfMass);
    const double halfAxisLength = axisLength / 2;

    retVal &= SweepHalfAxis(axisPlus,   startingAxisCoordinate, halfAxisLength, vOut);
    retVal &= SweepHalfAxis(axisMinus, -startingAxisCoordinate, halfAxisLength, vOut);

    return retVal;
}

// Sweep 1/2 of an axis
// axis = axis to sweep
// startingSlotAxisCoordinate = distance to center-of-mass from the center of this slot along the supplied axis.  This is in slot-local coordinates.
// distanceRequired = distance required from centerpoint to the edge of the incoming payload cube object
// vOut = OUTPUT: will contain slots that are required for this axis
// returns: true of hull boundary OK, false if hull boundary reached
bool XRPayloadBaySlot::SweepHalfAxis(const NEIGHBOR axis, const double startingSlotAxisCoordinate, double distanceRequired, vector<const XRPayloadBaySlot *> &vOut) const
{
    bool retVal = true;     // no impact with hull yet

    // Reduce the distance required by 1/2 the space in the ORIGIN slot (us!) *adjusted* for the starting axis coordinate.
    // Compute how much distance is required along the direction of the axis being swept.
    // Adjust the distance remaining by the starting slot coordinate; for example, if the center shifted LEFT (-X), we will need MORE distance along
    // the +X axis, but LESS distance along the -X axis. 
    const double originSlotDistanceForAxis = GetVectorValueForAxis(axis, GetDimensions()) / 2;  // 1/2 total length (or width or height) of the origin slot (us)
    distanceRequired -= (originSlotDistanceForAxis - startingSlotAxisCoordinate);   // startingSlotAxisCoordinate may be positive or negative 

    const XRPayloadBaySlot *pSlot = this;
    for (;;)
    {
        // check whether we need another slot
        if (distanceRequired <= 0.01)  // allow cheating of 1/100th-meter (1 cm) in case someone makes a payload and rounds up 1/100th
            break;  // we have room

        // we need another slot; walk to it
        pSlot = pSlot->GetNeighbor(axis);
        if (pSlot == nullptr)
        {
            // we reached the hull!
            retVal = false;
            break;
        }

        // We have another valid slot; subtract its total length along the current axis from the distancePlus required 
        // and add it to our "occupied neighbor slots" vector.
        const VECTOR3 &slotDimensions = pSlot->GetDimensions();
        distanceRequired -= GetVectorValueForAxis(axis, slotDimensions);
        vOut.push_back(pSlot);
    }  // end of main loop

    return retVal;
}

// static utility method that returns the vector value of a given neighbor axis
double XRPayloadBaySlot::GetVectorValueForAxis(const NEIGHBOR neighbor, const VECTOR3 &neighborVector)
{
    double retVal;

    switch (neighbor)
    {
    case NEIGHBOR::PLUSY:
    case NEIGHBOR::MINUSY:
        retVal = neighborVector.y;
        break;

    case NEIGHBOR::PLUSX:
    case NEIGHBOR::MINUSX:
        retVal = neighborVector.x;
        break;

    case NEIGHBOR::PLUSZ:
    case NEIGHBOR::MINUSZ:
        retVal = neighborVector.z;
        break;
    
    default:
        // should never happen!
        retVal = 0;
        break;
    }

    return retVal;
}

// returns the maximum capacity of the indexed fuel tank for this payload, if any is attached in this slot (0 = PropellantResource1) AND it contains XR fuel
double XRPayloadBaySlot::GetPropellantMaxMass(const int index) const
{
    double retVal = 0;
    VESSEL *pChild = GetChild();
    if (pChild != nullptr)
    {
        const XRPayloadClassData &pcd = XRPayloadClassData::GetXRPayloadClassDataForClassname(pChild->GetClassName());
        if (pcd.IsXRConsumableTank())
        {
            const PROPELLANT_HANDLE ph = pChild->GetPropellantHandleByIndex(index);
            if (ph != nullptr)
                retVal = pChild->GetPropellantMaxMass(ph);
        }
    }
    
    return retVal;
}

// returns the *current quantity* of the indexed fuel tank for this payload, if any is attached in this slot (0 = PropellantResource1) AND it contains XR fuel
double XRPayloadBaySlot::GetPropellantMass(const int index) const
{
    double retVal = 0;
    VESSEL *pChild = GetChild();
    if (pChild != nullptr)
    {
        const XRPayloadClassData &pcd = XRPayloadClassData::GetXRPayloadClassDataForClassname(pChild->GetClassName());
        if (pcd.IsXRConsumableTank())
        {
            const PROPELLANT_HANDLE ph = pChild->GetPropellantHandleByIndex(index);
            if (ph != nullptr)
                retVal = pChild->GetPropellantMass(ph);
        }
    }
    
    return retVal;
}

// returns quantity actually adjusted in this slot (takes empty/full into account)
// delta = amount in kg to add/remove
// this is 'const' because only the *child vessel* is changed
// If the payload in this slot does not contain XR fuel, no change is made.
double XRPayloadBaySlot::AdjustPropellantMass(const int index, const double delta) const
{
    double retVal = 0;
    
    VESSEL *pChild = GetChild();
    if (pChild != nullptr)
    {
        const PROPELLANT_HANDLE ph = pChild->GetPropellantHandleByIndex(index);
        if (ph != nullptr)
        {
            const XRPayloadClassData &pcd = XRPayloadClassData::GetXRPayloadClassDataForClassname(pChild->GetClassName());
            if (pcd.IsXRConsumableTank())
            {
                double qty = pChild->GetPropellantMass(ph);
                const double orgQuantity = qty;
                const double capacity = pChild->GetPropellantMaxMass(ph); 
                qty += delta;  // adjust

                // range-check
                if (qty < 0)
                    qty = 0;
                else if (qty > capacity)
                    qty = capacity;
                
                pChild->SetPropellantMass(ph, qty);
                retVal = qty - orgQuantity;  // delta from original fill level
            }
        }
    }
    
    return retVal;
}
