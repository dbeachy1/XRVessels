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
// XRPayloadBay.h
// Defines a payload bay for an XR-class vessel
// ==============================================================

#pragma once

#include "XRPayload.h"
#include "PropType.h"  // for enum
#include <unordered_map>

using namespace stdext;

// dummy vessel classname
#define XRPAYLOAD_BAY_CLASSNAME  "XRPayloadBay"

class XRPayloadBaySlot;

/*
    A payload bay is defined as a 3-dimensional grid of cargo slots, with each slot being
    a standard size (PAYLOAD_SLOT_DIMENSIONS).  Each payload bay slot (XRPayloadBaySlot object)
    has neighbors on each of its six sides: the neighbor may be another bay slot, or it may be the 
    edge of the payload bay.  The attachment point for each payload bay slot is in the center of its location
    (including the vertical (Y) dimension.)
*/

//-------------------------------------------------------------------------

// hashmap: int -> XRPayloadBaySlot object
typedef unordered_map<int, XRPayloadBaySlot *> HASHMAP_INT_XRPAYLOADBAYSLOT;

// Base XRPayload bay class that each XR vessel should extend or use
class XRPayloadBay
{
public:    
    // data structure returned by AdjustPropellantMass
    // this applies only to the *current timestep*
    struct SlotsDrainedFilled
    {
        double quantityAdjusted;  // quantity actually drained or filled (negative = drained)
        vector<int> drainedList;  // slot indexes are from 1...n
        vector<int> filledList;   // slot indexes are from 1...n
    };

    XRPayloadBay(VESSEL &parentVessel);
    virtual ~XRPayloadBay();

    void AddSlot(XRPayloadBaySlot *pSlot);
    XRPayloadBaySlot *GetSlot(int slotNumber) const;
    XRPayloadBaySlot *GetSlotForGrid(const int level, const int gridX, const int gridY) const; 
    bool AttachChild(OBJHANDLE payloadObjHandle, const int slotNumber);     // convenience method
    bool DetachChild(const int slotNumber, const double deltaV);            // convenience method
    bool DetachChildLanded(const int slotNumber);  
    int DetachAllChildren(const double deltaV);
    int DetachAllChildrenLanded();
    bool IsChildVesselAttached(OBJHANDLE hVessel) const;

    VESSEL *GetChild(const int slotNumber) const;  // convenience method: WARNING: will return nullptr if child was deleted since it was attached, or if no payload is in this slot.
    bool IsSlotEnabled(int slotNumber) const;      // convenience method
    double GetPayloadMass() const;
    void PerformFinalInitialization(ATTACHMENTHANDLE dummyAttachmentPoint);
    void RefreshSlotStates();  
    bool CreateAndAttachPayloadVessel(const char *pClassname, const int slotNumber);
    int CreateAndAttachPayloadVesselInAllSlots(const char *pClassname);
    bool DeleteAttachedPayloadVessel(const int slotNumber);
    int DeleteAllAttachedPayloadVessels();
    int DeleteAllAttachedPayloadVesselsOfClassname(const char *pClassname);
    int GetChildCount() const;

    int GetSlotCount() const                 { return static_cast<int>(m_allSlotsMap.size()); }
    VESSEL &GetParentVessel() const          { return m_parentVessel; }

    // fuel/lox management
    double GetPropellantMaxMass(const PROP_TYPE propType) const;
    double GetPropellantMass(const PROP_TYPE propType) const;
    const SlotsDrainedFilled &AdjustPropellantMass(const PROP_TYPE propType, const double quantityRequested);

    // virtual methods
    
    // Callback invoked by the framework immediately after a child vessel is created and attached in a bay slot
    // and the bay's slot states are refreshed.  The default implementation here does nothing.
    // pSlotWithNewChild = slot containing new child vessel that was just created
    virtual void clbkChildCreatedInBay(XRPayloadBaySlot &slotWithNewChild) { }

    // abstract methods
    virtual VECTOR3 GetLandedDeployToCoords(const int slotNumber) = 0;

protected:
    VESSEL &m_parentVessel;
    // map of slots numbers -> slot data: key=(int) slot #, value=(XRPayloadBaySlot) data
    HASHMAP_INT_XRPAYLOADBAYSLOT m_allSlotsMap;
    SlotsDrainedFilled m_slotsDrainedFilled;  // only updated by AdjustPropellantMass
};
