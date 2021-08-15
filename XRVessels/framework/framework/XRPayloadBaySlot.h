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
// XRPayloadBaySlot.h
// Defines a payload bay for an XR-class vessel
// ==============================================================

#pragma once

#include "XRPayloadBay.h"
#include "vessel3ext.h"
#include <unordered_map>
#include <vector>

using namespace stdext;

class XRPayloadBaySlot;

// enum defining neighbor slot indicies; there are six sides to each cube-shaped slot
// NOTE: do not change the order of these items.
//              up,    down,   right, left,   forward, aft
enum NEIGHBOR { PLUSY, MINUSY, PLUSX, MINUSX, PLUSZ, MINUSZ };

// data class defining a single payload bay slot of standard size PAYLOAD_SLOT_DIMENSIONS
class XRPayloadBaySlot
{
public:
    static double GetVectorValueForAxis(const NEIGHBOR neighbor, const VECTOR3 &neighborVector);

    XRPayloadBaySlot(const int slotNumber, const VECTOR3 &localCoordinates, XRPayloadBay &parentBay, const VECTOR3 &slotDimensions, const int level, const COORD2 &levelCoordinates);

    bool AttachChild(VESSEL &childVessel);
    bool DetachChild(const double deltaV);
    VESSEL *GetChild() const;  // will return nullptr if child was deleted since it was attached or if no payload is in this slot.
    bool GetRequiredNeighborSlotsForCandidateVessel(const VESSEL &childVessel, vector<const XRPayloadBaySlot *> &vOut) const;  // populates slot ptrs in vOut; returns TRUE if hull edge check OK, or FALSE if vessel would hit the hull edge
    bool CheckSlotSpace(const VESSEL &childVessel) const;  // returns TRUE if there is room to latch the child in this slot; NOTE: may be via explicit-latch

    int GetSlotNumber() const                      { return m_slotNumber; }  // 1...n
    const VECTOR3 &GetLocalCoordinates() const     { return m_localCoordinates; }   // coordinates to the center of the slot
    void SetNeighbor(const NEIGHBOR n, XRPayloadBaySlot *pNeighbor) { m_neighbors[n] = pNeighbor; }
    XRPayloadBaySlot *GetNeighbor(const NEIGHBOR n) const { return m_neighbors[n]; }  // may be null
    
    void SetEnabled(bool b)                        { m_isEnabled = b; }
    bool IsEnabled() const                         { return m_isEnabled; }  // returns 'true' if this slot is enabled; i.e., it is available for explicit attach/detach operations by the pilot
    bool IsOccupied() const                        { return ((GetChild() != nullptr) || (IsEnabled() == false)); }  // slot is occupied by cargo or adjacent cargo
    XRPayloadBay &GetParentBay() const             { return m_parentBay; }
    VESSEL &GetParentVessel() const                { return GetParentBay().GetParentVessel(); }
    const VECTOR3 &GetDimensions() const           { return m_dimensions; }  // width (X), height (Y), length (Z)
    int GetLevel() const                           { return m_level; }
    const COORD2 &GetLevelGridCoordinates() const  { return m_levelGridCoordinates; }
    ATTACHMENTHANDLE GetAttachmentHandle() const   { return m_hAttachmentHandle; }   // our parent vessel's attachment handle in the bay for this slot (this is public b/c we need it for an XRVesselCtrl API call)

    // convenience methods dealing with consumable resouce payloads (if any) attached in the slot
    double GetMainFuelMaxMass() const    { return GetPropellantMaxMass(0); }
    double GetSCRAMFuelMaxMass() const   { return GetPropellantMaxMass(1); }
    double GetLOXMaxMass() const         { return GetPropellantMaxMass(2); }

    double GetMainFuelMass() const    { return GetPropellantMass(0); }
    double GetSCRAMFuelMass() const   { return GetPropellantMass(1); }
    double GetLOXMass() const         { return GetPropellantMass(2); }

    double AdjustMainFuelMass(const double delta) const    { return AdjustPropellantMass(0, delta); }
    double AdjustSCRAMFuelMass(const double delta) const   { return AdjustPropellantMass(1, delta); }
    double AdjustLOXMass(const double delta) const         { return AdjustPropellantMass(2, delta); }

protected:
    bool SweepSlots(const VECTOR3 &childCenterOfMass, const VECTOR3 &childDimensions, vector<const XRPayloadBaySlot *> &vOut) const ;
    bool SweepXAxisForSlots(vector<const XRPayloadBaySlot *> &zAxisOriginSlots, const bool addOriginSlotsToVout, const VECTOR3 &childCenterOfMass, const double xAxisLength, vector<const XRPayloadBaySlot *> &vOut) const;
    bool SweepAxis(const NEIGHBOR axisPlus, const NEIGHBOR axisMinus, const VECTOR3 &childCenterOfMass, const double axisLength, vector<const XRPayloadBaySlot *> &vOut) const;
    bool SweepHalfAxis(const NEIGHBOR axis, const double startingSlotAxisCoordinate, double distanceRequired, vector<const XRPayloadBaySlot *> &vOut) const;
    double GetPropellantMaxMass(const int index) const;
    double GetPropellantMass(const int index) const;
    double AdjustPropellantMass(const int index, const double delta) const;  // this is 'const' because only the *child vessel* is changed

    XRPayloadBay &m_parentBay;
    ATTACHMENTHANDLE m_hAttachmentHandle;   // parent vessel's attachment handle
    const int m_slotNumber;  // 1...n
    const VECTOR3 m_localCoordinates;       // coordinates of the CENTER of this slot
    const VECTOR3 m_dimensions;             // in meters
    XRPayloadBaySlot *m_neighbors[6];       // these are not freed by us; they reference other first-class objects
    int m_level;                            // bay level on which this slot resides (1..n)
    COORD2 m_levelGridCoordinates;          // x,y of the grid point on this level; 0,0 = bottom-left, n,n = top-right

    // If true, this slot is available for explicit attach/detach operations by the pilot; i.e., it is "enabled."
    // If false, this slot is occupied by a payload that was explicitly attached in a *neighboring* slot; i.e., it is "disabled" until the neighboring payload is detached.
    bool m_isEnabled; 
}; 
