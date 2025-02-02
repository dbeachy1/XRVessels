/**
  XR Vessel add-ons for OpenOrbiter Space Flight Simulator
  Copyright (C) 2006-2025 Douglas Beachy

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
#include <vector>

// Constructor
XRPayloadBay::XRPayloadBay(VESSEL &parentVessel) :
    m_parentVessel(parentVessel)
{
}

// Destructor
XRPayloadBay::~XRPayloadBay()
{
    // Free up the contents of the map, which were actually allocated by our caller before invoking
    // our constructor.
    auto it = m_allSlotsMap.begin();
    for (; it != m_allSlotsMap.end(); it++)
    {
        const XRPayloadBaySlot *pSlot = it->second;
        delete pSlot;   
    }
}

// Add (define) a slot for this payload bay.  Subclasses should invoke this to define the physical bay layout.
// pSlot will be freed when this object is freed: the caller should allocate it with 'new'.  May not be null.
void XRPayloadBay::AddSlot(XRPayloadBaySlot *pSlot)
{
    _ASSERTE(pSlot != nullptr);
    _ASSERTE(pSlot->GetSlotNumber() > 0);
    _ASSERTE(m_allSlotsMap.find(pSlot->GetSlotNumber()) == m_allSlotsMap.end());  // assert that the slot was not already added

    // add to our master map
    typedef pair<int, XRPayloadBaySlot *> Int_XRPayloadBaySlot_Pair;
    m_allSlotsMap.insert(Int_XRPayloadBaySlot_Pair(pSlot->GetSlotNumber(), pSlot));  // key = slot #, value=slot data
}

// Returns slot data for the specified slot number, or nullptr if slotNumber is invalid
// Valid slot numbers: 1...n
XRPayloadBaySlot *XRPayloadBay::GetSlot(int slotNumber) const
{
    XRPayloadBaySlot *pRetVal = nullptr;
    auto it = m_allSlotsMap.find(slotNumber);
    if (it != m_allSlotsMap.end())
        pRetVal = it->second;

    return pRetVal;
}

// Returns slot data for the slot specified by a level and its grid coordinates, or nullptr if no slot at the requested coordinates
// 0,0 = slot 1 on level 1; i.e., bottom-left of grid looking AFT
// No range checks are performed via asserts.
XRPayloadBaySlot *XRPayloadBay::GetSlotForGrid(const int level, const int gridX, const int gridY) const
{
    XRPayloadBaySlot *pRetVal = nullptr;

    // walk through each slot
    for (int slotNumber=1; slotNumber <= GetSlotCount(); slotNumber++)
    {
        XRPayloadBaySlot *pSlot = GetSlot(slotNumber);  // will never be null
        const COORD2 &coords = pSlot->GetLevelGridCoordinates();
        if ((coords.x == gridX) && (coords.y == gridY) && (pSlot->GetLevel() == level))
        {
            // found it
            pRetVal = pSlot;
            break;
        }
    }

    return pRetVal;
}

// Attach a child payload vessel to the specified slot.
// childObjHandle = handle of child object to be attached; must be XR-Payload-enabled, and may not be null.
// slotNumber = payload slot number
// Returns: true on success, false if the child refused to be attached, if no child attachment point found, or 
//          if the payload will not fit in the requested slot.
bool XRPayloadBay::AttachChild(OBJHANDLE childObjHandle, const int slotNumber)
{
    _ASSERTE(childObjHandle != nullptr);
    _ASSERTE(slotNumber > 0);

    // verify that the handle is still valid
    if (oapiIsVessel(childObjHandle) == false)
        return false;   // vessel deleted!

    // verify that the slot number is valid
    XRPayloadBaySlot *pPrimarySlot = GetSlot(slotNumber);
    if (pPrimarySlot == nullptr)
    {
        _ASSERTE(pPrimarySlot != nullptr);   // fail under debugger
        return false;   // invalid slot number passed in from caller!
    }

    // retrieve the child vessel and attach it
    VESSEL *pChildVessel = oapiGetVesselInterface(childObjHandle);  // will never be nullptr since we checked the handle already
    bool retVal = pPrimarySlot->AttachChild(*pChildVessel);

    return retVal;
}

// Detach a child from the specified slot using the specified delta-V (postive value = +Y velocity).
// slotNumber = payload slot number
// deltaV = deltaV in meters-per-second
// Returns: true on success, false if the child refused to be detached or if no child is in the specified slot
bool XRPayloadBay::DetachChild(const int slotNumber, const double deltaV)
{
    _ASSERTE(slotNumber > 0);

    XRPayloadBaySlot *pPrimarySlot = GetSlot(slotNumber);
    if (pPrimarySlot == nullptr)
    {
        _ASSERTE(pPrimarySlot != nullptr);   // fail under debugger
        return false;   // invalid slot number passed in from caller!
    }

    bool retVal = pPrimarySlot->DetachChild(deltaV);

    return retVal;
}

// Detach a child from the specified slot and unload it in LANDED mode; i.e., place it on the ground.
// slotNumber = payload slot number
// Returns: true on success, false if the child refused to be detached or if no child is in the specified slot
bool XRPayloadBay::DetachChildLanded(const int slotNumber)
{
    _ASSERTE(slotNumber > 0);

    bool retVal = false;

    // see if there is a child in the requested slot
    VESSEL *pChild = GetChild(slotNumber);
    if (pChild != nullptr)
    {
        // Must obtain "move-to" coordinates while the child is still attached!  The subclass needs the child's attachment point 
        // to compute the proper coordinates.
        VECTOR3 deployToCoords = GetLandedDeployToCoords(slotNumber);   // get from the subclass; these are ship-local coordinates

        // Detach the child vessel.
        DetachChild(slotNumber, 0.0);    // no delta-V

        // obtain the child's coordinates
        VESSELSTATUS2 childVS;
        VESSEL3_EXT::GetStatusSafe(*pChild, childVS, false);

        // move the child to the deployToCoordinates by converting them (as a delta) from parent-local to GLOBAL coordinates
        VECTOR3 globalChildDeltaCoords;
        GetParentVessel().GlobalRot(deployToCoords, globalChildDeltaCoords);

        // now take the parent's rpos, apply the delta, and store it in the child's VS
        VESSELSTATUS2 parentVS;
        VESSEL3_EXT::GetStatusSafe(GetParentVessel(), parentVS, false); 
        childVS.rpos = (parentVS.rpos + globalChildDeltaCoords);
        
        // WARNING: do not force status=1 (landed) here!  It will cause the "bounce bug" and crash Orbiter.
        childVS.status = 0;                  // set to FREEFLIGHT
        pChild->DefSetStateEx(&childVS);     // update the vessel's state with the new location

        retVal = true;
    }

    return retVal;
}

// Detach *all* children in the bay and deploy each at the specified deltaV.
// Returns: total count of children successfully detached
int XRPayloadBay::DetachAllChildren(const double deltaV)
{
    int detachedCount = 0;

    // loop through each slot and keep count 
    for (int i=1; i <= GetSlotCount(); i++)
    {
        if (DetachChild(i, deltaV))
            detachedCount++;
    }

    return detachedCount;
}

// Detach *all* children in the bay unload each in LANDED mode; i.e., place it on the ground.
// Returns: total count of children successfully detached
int XRPayloadBay::DetachAllChildrenLanded()
{
    int detachedCount = 0;

    // loop through each slot and keep count 
    for (int i=1; i <= GetSlotCount(); i++)
    {
        if (DetachChildLanded(i))
            detachedCount++;
    }

    return detachedCount;
}

// Returns the total payload mass in kg
double XRPayloadBay::GetPayloadMass() const
{
    double totalMass = 0;

    // iterate through each slot and add up the total payload mass
    for (int i=1; i <= GetSlotCount(); i++)     // slots are one-based
    {
        const VESSEL *pChild = GetChild(i);
        if (pChild != nullptr)        
        {
            // Only primary slots (slots to which a vessel was explicitly attached) have a child vessel present;
            // other surrounding slots will be marked as 'disabled' if the vessel occupies more than one slot, but all of the mass
            // will be tracked from the primary slot only.
            //
            // Note that we use the actual vessel MASS here instead of just the nominal initial mass tracked by the XRPayload object.  This is
            // so that the mass will be correct when "dynamic vessels" are docked in the payload bay.  In other words, if you have a ship docked 
            // in the payload bay that is burning consumables or venting mass, it will be reflected in real-time on the ship's mass readouts.
            totalMass += pChild->GetMass();
        }
    }

    return totalMass;
}

//-------------------------------------------------------------------------

// Create and attach the dummy XRPayload vessel to force Orbiter to render the mesh, plus perform any other final initialization tasks
// that must be performed after the simulation has fully initialized.
//
// NOTE: this must be called sometime AFTER clbkPostCreation because the vessel attached to the ship might not have been instantiated yet!
// Therefore it must be manually invoked from the subclass; it is not invoked by our constructor.
void XRPayloadBay::PerformFinalInitialization(ATTACHMENTHANDLE dummyAttachmentPoint)
{
    // create the vessel name; prepend our vessel name to ensure the child name is unique
    char dummyName[128];
    sprintf(dummyName, "%s_Bay", GetParentVessel().GetName());

    // Check whether we already have our dummy vessel attached; if so, we should not recreate it
    // NOTE: we should not try to delete and recreate the vessel here because Orbiter queues up the requests and
    // processes them when the frame ends, and delete trumps create.  Therefore, the new vessel is not created.
    OBJHANDLE hDummy = GetParentVessel().GetAttachmentStatus(dummyAttachmentPoint);

    // WARNING: for some reason Orbiter tends to keep vessels alive for at least one frame after they are deleted; i.e., the handles comes back
    // but it is now invalid!  Therefore, we have to handle that here.

    if (oapiIsVessel(hDummy) == false)
    {
        // create an instance of the dummy vessel
        VESSELSTATUS2 status;
        VESSEL3_EXT::GetStatusSafe(GetParentVessel(), status, true);  // clone our vessel's status
        // Zero out some stuff to be tidy; for example XRSound examines each vessel's thruster count to see if that vessel
        // should have default sounds, and obviously the _Bay dummy vessel should not.
        //
        // NOTE: from testing, however, it apears Orbiter assigns the parent vessel's thruster count
        // to any docked vessels anyway, because the thruster count for the dummy vessel is still 21 (for the XR2) even though
        // we zero out that value, among other settings, here.
        status.flag = 0;
        status.nfuel = 0;
        status.fuel = nullptr;
        status.nthruster = 0;
        status.thruster = nullptr;
        status.ndockinfo = 0;
        status.dockinfo = nullptr;

        hDummy = oapiCreateVesselEx(dummyName, XRPAYLOAD_BAY_CLASSNAME, &status);

        // now attach the vessel to our bay so Orbiter will always render the bay
        // retrieve attachment point of child
        VESSEL *pChildVessel = oapiGetVesselInterface(hDummy);
        ATTACHMENTHANDLE childAttPointHandle = pChildVessel->GetAttachmentHandle(true, 0);
        GetParentVessel().AttachChild(hDummy, dummyAttachmentPoint, childAttPointHandle);
    }

    // initialize/reset fuel tank size to fix "#IND00" from appearing in saved scenario files
    VESSEL *pDummyVessel = oapiGetVesselInterface(hDummy);
    _ASSERTE(pDummyVessel != nullptr);
    PROPELLANT_HANDLE ph = pDummyVessel->GetPropellantHandleByIndex(0);  // Orbiter only creates one tank by default
    if (ph != nullptr)   // should always succeed, but not an error if it does not
    {
        // since we have a tank, set its maximum capacity to non-zero to prevent division-by-zero causing "#IND00"
        pDummyVessel->SetPropellantMaxMass(ph, 0.1);
        VESSEL3_EXT::ResetAllFuelLevels(pDummyVessel, 0);
    }

    // initialize the enabled/disabled state of all slots
    RefreshSlotStates();
}

// Refresh the enabled/disabled state of all slots in the bay based on 
// payload in each slot.  This should be called on startup and whenever a new vessel is 
// attached or detached.
void XRPayloadBay::RefreshSlotStates()
{
    // First, reset all slots to ENABLED.
    for (int slotNumber=1; slotNumber <= GetSlotCount(); slotNumber++)
       GetSlot(slotNumber)->SetEnabled(true);

    // Second, locate and process *primary* slot with a child (i.e., a slot with a payload directly attached)
    // and disable any necessary slots.
    vector<XRPayloadBaySlot *> vOut;  // declared here for efficiency
    for (int slotNumber=1; slotNumber <= GetSlotCount(); slotNumber++)
    {
        XRPayloadBaySlot *pSlot = GetSlot(slotNumber);
        VESSEL *pChild = pSlot->GetChild();
        if (pChild != nullptr)
        {
            vOut.clear();       // reset

            // This is a primary slot with a child attached; process it and mark any surrounding slots as DISABLED if the 
            // payload is too large for one slot.
            // Note: we can safely typecast vOut to be a vector<const XRPayloadBaySlot *> for this call.
            pSlot->GetRequiredNeighborSlotsForCandidateVessel(*pChild, reinterpret_cast<vector<const XRPayloadBaySlot *> &>(vOut));  // ignore return code for 'clearsHull' status: it does not matter here

            // disable all occupied neighbor slots; the primary slot remains ENABLED
            for (UINT i=0; i < vOut.size(); i++)
            {
                XRPayloadBaySlot *pNeighborSlot = vOut[i];
                pNeighborSlot->SetEnabled(false);
            }
        }
    }
}

// Instantiate a new instance of a given payload vessel and attach it in the bay at the specified slot, provided there is room.
// Returns: true on success, false if vessel could not be instantiated or attached in the specified slot
bool XRPayloadBay::CreateAndAttachPayloadVessel(const char *pClassname, const int slotNumber)
{
    _ASSERTE(slotNumber > 0);

    const XRPayloadClassData &pcd = XRPayloadClassData::GetXRPayloadClassDataForClassname(pClassname);

    // WARNING: PAYLOAD VESSEL NAMES MUST BE UNIQUE!
    // Define the new vessel's name as: vesselClassname-slotNumber; e.g., XRPayloadTest-04-1
    // Loop until we find a unique name!
    char childName[256];
    for (int subIndex = 1; subIndex < 10000; subIndex++)   // 10000 is for sanity check
    {
        sprintf(childName, "%s-%02d-%d", pClassname, slotNumber, subIndex);

        // check whether vessel already exists
        OBJHANDLE hExistingVessel = oapiGetVesselByName(childName);
        if (oapiIsVessel(hExistingVessel) == false)
            break;      // name is unique in this scenario
    }

    // Instantiate a new instance of the payload vessel using our vessel's state as a template *EXCEPT* that 'base' and 'port' must 
    // be reset to zero!  Otherwise Orbiter will CTD when it tries to load an attached vessel that specifies a "base" in its scenario.
    VESSELSTATUS2 status;
    VESSEL3_EXT::GetStatusSafe(GetParentVessel(), status, true);  // reset extraneous fields to empty 

    OBJHANDLE childHandle = oapiCreateVesselEx (childName, pcd.GetClassname(), &status);
    if (childHandle == nullptr)
        return false;   // just in case (although the spec doesn't state what happens when vessel creation fails, or IF it can fail gracefully)
        // NOTE: it turns out that if the .cfg cannot be found, Orbiter terminates with a critical error in Orbiter log stating that no config file could be found for vessel 'foo'.

    // Check whether there is space for this vessel; this must be done AFTER the vessel is created because we need
    // to use the attachment points to determine whether it will fit.
    VESSEL *pChildVessel = oapiGetVesselInterface(childHandle);
    XRPayloadBaySlot *pSlot = GetSlot(slotNumber);

    // Vessel created successfully; try to attach it and update the enabled/disabled state of each slot.
    // This will also verify that there is sufficient space for the child.
    if (pSlot->AttachChild(*pChildVessel) == false)
    {
        // Attachment failed!  Delete the new vessel and exit.
        oapiDeleteVessel(childHandle);
        return false;
    }

    // Attach succeeded!  Update the enabled/disabled state of each slot since a new payload was added.
    RefreshSlotStates();
    
    // notify the subclasses
    clbkChildCreatedInBay(*pSlot);

    return true;
}

// Detach and remove the vessel in the specified slot, if any
// Returns: true if vessel was DETACHED successfully (although the delete should succeed, too), false if no vessel in slot or if it refused to detach
bool XRPayloadBay::DeleteAttachedPayloadVessel(const int slotNumber)
{
    bool retVal = false;

    // save the vessel handle in the bay so we can delete it after it detaches
    VESSEL *pChildVessel = GetChild(slotNumber);
    if (pChildVessel != nullptr)   // anything to remove?
    {
        retVal = DetachChild(slotNumber, 0.0);  // no delta-v
        if (retVal)     // success?
        {
            // Delete the vessel we just detached; ignore any error here since all we really care about is that
            // the slot is empty now.
            oapiDeleteVessel(pChildVessel->GetHandle());

            // since a slot was freed, update the enabled/disabled slot states
            RefreshSlotStates();  
        }
    }

    return retVal;
}   

// convenience method to return the child for the specified slot.
// WARNING: will return nullptr if child was deleted since it was attached, or if no payload is in this slot.
VESSEL *XRPayloadBay::GetChild(const int slotNumber) const
{
    _ASSERTE(slotNumber > 0);
    return GetSlot(slotNumber)->GetChild();  // we want to crash here anyway if the slot number is invalid
}

// Convenience method to return whether the given slot is enabled
bool XRPayloadBay::IsSlotEnabled(int slotNumber) const
{ 
    _ASSERTE(slotNumber > 0);
    return GetSlot(slotNumber)->IsEnabled();   // we want to crash here anyway if the slot number is invalid
}  

// Delete all child vessels in the bay
// Returns: # of vessels deleted
int XRPayloadBay::DeleteAllAttachedPayloadVessels()
{
    int count = 0;
    
    // walk through each slot 
    for (int i=0; i < GetSlotCount(); i++)
    {
        if (DeleteAttachedPayloadVessel(i+1))  
            count++;   // vessel was deleted
    }
    
    return count;
}

// Create a new vessel in each free slot (checking for room, of course).
// Returns: # of vessels created
int XRPayloadBay::CreateAndAttachPayloadVesselInAllSlots(const char *pClassname)
{
    int count = 0;
    
    // walk through each slot
    for (int i=0; i < GetSlotCount(); i++)
    {
        if (CreateAndAttachPayloadVessel(pClassname, i+1))
            count++;   // vessel was created
    }
    
    return count;
}

// Delete all child vessels in the bay of a given class type
// Returns: # of vessels deleted
int XRPayloadBay::DeleteAllAttachedPayloadVesselsOfClassname(const char *pClassname)
{
    int count = 0;
    
    // walk through each slot 
    for (int i=0; i < GetSlotCount(); i++)
    {
        // check whether this slot has payload
        XRPayloadBaySlot *pSlot = GetSlot(i+1);
        VESSEL *pChildVessel = pSlot->GetChild();
        if (pChildVessel != nullptr)
        {
            // child exists in this slot; check its classname
            if (strcmp(pChildVessel->GetClassName(), pClassname) == 0)
            {
                // classname matches; delete this child
                if (DeleteAttachedPayloadVessel(i+1))
                    count++;        // successfully deleted
            }
        }
    }
    
    return count;
}

// Returns the total number of child vessels attached in the bay.
int XRPayloadBay::GetChildCount() const
{
    int childCount = 0;
    for (int slotNumber = 1; slotNumber <= GetSlotCount(); slotNumber++)
    {
        if (GetChild(slotNumber) != nullptr)
            childCount++;
    }

    return childCount;
}

// Returns true if vessel attached in any bay slot, false otherwise
bool XRPayloadBay::IsChildVesselAttached(OBJHANDLE hVessel) const
{
    bool retVal = false;     // assume not attached

    // iterate through all slots
    for (int slotNumber = 1; slotNumber <= GetSlotCount(); slotNumber++)
    {
        const VESSEL *pChild = GetChild(slotNumber);
        if (pChild != nullptr)
        {
            // handles are globally unique, so let's check them
            if (pChild->GetHandle() == hVessel)
            {
                retVal = true;
                break;
            }
        }
    }

    return retVal;
}

// returns the maximum capacity of the indexed fuel tank for all tanks in the bay, if any
double XRPayloadBay::GetPropellantMaxMass(const PROP_TYPE propType) const
{
    double retVal = 0;

    // iterate through all slots
    for (int i=0; i < GetSlotCount(); i++)
    {
        const XRPayloadBaySlot *pSlot = GetSlot(i+1);  // will never be null
        if (propType == PROP_TYPE::PT_Main)
            retVal += pSlot->GetMainFuelMaxMass();
        else if (propType == PROP_TYPE::PT_SCRAM)
            retVal += pSlot->GetSCRAMFuelMaxMass();
        else if (propType == PROP_TYPE::PT_LOX)
            retVal += pSlot->GetLOXMaxMass();
        else if (propType == PROP_TYPE::PT_NONE)
        { 
            // e.g., RCS: a resource that has no corresponding bay tank, so fall through with zero
        }  
        else  // invalid enum (should never happen)
            _ASSERTE(false);  // break into debugger if debug build, else fall through with zero
    }
    
    return retVal;
}

// returns the *current quantity* of the indexed fuel tank for all tanks in the bay, if any
double XRPayloadBay::GetPropellantMass(const PROP_TYPE propType) const
{
    double retVal = 0;

    // iterate through all slots
    for (int i=0; i < GetSlotCount(); i++)
    {
        const XRPayloadBaySlot *pSlot = GetSlot(i+1);  // will never be null
        if (propType == PROP_TYPE::PT_Main)
            retVal += pSlot->GetMainFuelMass();
        else if (propType == PROP_TYPE::PT_SCRAM)
            retVal += pSlot->GetSCRAMFuelMass();
        else if (propType == PROP_TYPE::PT_LOX)
            retVal += pSlot->GetLOXMass();
        // else invalid enum! (should never happen), so fall through with zero
    }
    
    return retVal;
}

// returns the quantity of the fuel actually drained/added from/to the bay + any slots filled/drained
// The resource will be drained/added to/from lowest->highest numbered slots
const XRPayloadBay::SlotsDrainedFilled &XRPayloadBay::AdjustPropellantMass(const PROP_TYPE propType, const double quantityRequested)
{
    // reset any drained/filled slot data for the return value
    m_slotsDrainedFilled.quantityAdjusted = 0;
    m_slotsDrainedFilled.drainedList.clear();
    m_slotsDrainedFilled.filledList.clear();

    double deltaRemaining = quantityRequested;

    // iterate through all slots
    for (int i=0; i < GetSlotCount(); i++)
    {
        const int slotNumber = i+1;
        if (deltaRemaining == 0)  
            break;

        double qtyDrained = 0;
        const XRPayloadBaySlot *pSlot = GetSlot(slotNumber);  // will never be null
        double prevSlotQty = 0;  // set below
        double maxSlotQty = 0;   // ditto

        if (propType == PROP_TYPE::PT_Main)
        {
            maxSlotQty = pSlot->GetMainFuelMaxMass();
            prevSlotQty = pSlot->GetMainFuelMass();
            qtyDrained = pSlot->AdjustMainFuelMass(deltaRemaining);
        }
        else if (propType == PROP_TYPE::PT_SCRAM)
        {
            maxSlotQty = pSlot->GetSCRAMFuelMaxMass();
            prevSlotQty = pSlot->GetSCRAMFuelMass();
            qtyDrained = pSlot->AdjustSCRAMFuelMass(deltaRemaining);
        }
        else if (propType == PROP_TYPE::PT_LOX)
        {
            maxSlotQty = pSlot->GetLOXMaxMass();
            prevSlotQty = pSlot->GetLOXMass();
            qtyDrained = pSlot->AdjustLOXMass(deltaRemaining);
        }
        // else invalid enum! (should never happen), so fall through with zero

        m_slotsDrainedFilled.quantityAdjusted += qtyDrained;    
        deltaRemaining -= qtyDrained;

        // if quantity drained == all remaining fuel in slotanything was drained or added but deltaRemaining != 0, the tank either just filled up or emptied!
        const double currentSlotQty = prevSlotQty + qtyDrained;
        _ASSERTE(currentSlotQty >= 0);           // should never over-drain tanks!
        _ASSERTE(currentSlotQty <= maxSlotQty);  // should never over-fill tanks!

        if ((currentSlotQty == maxSlotQty) && (prevSlotQty < maxSlotQty))
            m_slotsDrainedFilled.filledList.push_back(slotNumber);  // tank just filled
        else if ((currentSlotQty == 0) && (prevSlotQty > 0))
            m_slotsDrainedFilled.drainedList.push_back(slotNumber);  // tank just emptied
    }
    
    return m_slotsDrainedFilled;
}
