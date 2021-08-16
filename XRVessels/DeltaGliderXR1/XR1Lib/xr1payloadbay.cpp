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
// XR1Payload.cpp
// Class defining BASE XR VESSEL METHODS for a payload bay; this is not actually used
// by the XR1, but it is here for subclasses to use.
// ==============================================================

#include "DeltaGliderXR1.h"
#include "XR1PayloadBay.h"
#include "XRPayloadBaySlot.h"

//-------------------------------------------------------------------------
// XR1PayloadBay methods
// These are NOT USED by the XR1; they are here for subclasses.
//-------------------------------------------------------------------------

// Constructor
XR1PayloadBay::XR1PayloadBay(VESSEL &parentVessel) :
    XRPayloadBay(parentVessel)
{
    // nothing special to do here; the subclass must define the payload bay in its constructor.
}

// Callback invoked by the framework immediately after a child vessel is created and attached in a bay slot
// and the bay's slot states are refreshed.
//
// We need to hook this so we can unselect any selected slot since that is now disabled, which would
// circumvent the checks we have to prevent selecting a disabled slot.
// pSlotWithNewChild = slot containing new child vessel that was just created
void XR1PayloadBay::clbkChildCreatedInBay(XRPayloadBaySlot &slotWithNewChild)
{
    // If the selected slot is disabled and still SELECTED, then UNSELECT IT since you cannot select a DISABLED slot.
    const XRPayloadBaySlot *pSelectedSlot = GetSlot(GetXR1().m_selectedSlot);  // may be null
    if (pSelectedSlot != nullptr)
    {
        if (pSelectedSlot->IsEnabled() == false)
        {
            // slot is now disabled!  Unselect it.
            GetXR1().m_selectedSlot = 0;
        }
    }
}

//-------------------------------------------------------------------------
// Generic payload bay methods in the DeltaGliderXR1 base class
// These are NOT USED by the XR1; they are here for subclasses.
//-------------------------------------------------------------------------

// Deploy the active payload object; this handles landed and orbit modes automatically.
// Returns: true on success, false on error
// showMessage: true = show info message and play a callout; false = show no message and play no sound
bool DeltaGliderXR1::DeployPayload(const int slotNumber, const bool showMessage)
{
    bool retVal;

    // verify that a slot is selected
    if (VerifySlotSelected(showMessage) == false)
        return false;

    // Verify that bay doors are open AND there is payload in the requested slot.
    // This also will display an error message if requested.
    if (ValidateBayStatus(true, slotNumber, showMessage) == false)
        return false;

    // save the currently-attached vessel in the candidate slot
    const VESSEL *pChildForDetach = m_pPayloadBay->GetChild(slotNumber);  // may be null!

    // for wheel-stop, deploy in LANDED mode
    if (IsLanded())
    {
        retVal = m_pPayloadBay->DetachChildLanded(slotNumber);
        
        if (showMessage)
        {
            if (retVal)
            {
                PlaySound(BeepHigh, ST_Other);
                char msg[60];
                sprintf(msg, "Cargo in slot %d unloaded.", slotNumber);
                ShowInfo("Cargo Deployed.wav", DeltaGliderXR1::ST_InformationCallout, msg);
            }
            else
            {
                // really should never happen: this means the child vessel refused detachment
                PlaySound(Error1, DeltaGliderXR1::ST_Other, ERROR1_VOL);
                char msg[60];
                sprintf(msg, "Cargo deployment FAILED for slot %d.", slotNumber);
                ShowWarning("Cargo Deployment Failed.wav", DeltaGliderXR1::ST_WarningCallout, msg);
            }
        }
    }
    else  // deploy in ORBIT mode
    {
        retVal = m_pPayloadBay->DetachChild(slotNumber, -m_deployDeltaV);  // negative so we deploy UP out of the bay
        if (showMessage)
        {
            if (retVal)
            {
                PlaySound(BeepHigh, ST_Other);
                char msg[60];
                sprintf(msg, "Cargo in slot %d deployed at %.1f m/s.", slotNumber, m_deployDeltaV);
                ShowInfo("Cargo Deployed.wav", DeltaGliderXR1::ST_InformationCallout, msg);
            }
            else
            {
                // really should never happen: this means the child vessel refused detachment
                PlaySound(Error1, DeltaGliderXR1::ST_Other, ERROR1_VOL);
                char msg[60];
                sprintf(msg, "Cargo deployment FAILED for slot %d.", slotNumber);
                ShowWarning("Cargo Deployment Failed.wav", DeltaGliderXR1::ST_WarningCallout, msg);
            }
        }
    }

    // If deploy succeeded, bump DOWN to next slot automatically; this is so that cargo can be deployed from
    // top-to-bottom.  Note that we do not enforce a "room to deploy" rule since it could be tedious.
    // Also set the grapple target to the newly-deployed vessel.
    if (retVal)
    {
        // must refresh cargo in range since we just "added" another vessel by detaching one from the bay
        RefreshGrappleTargetsInDisplayRange(); 

        strcpy(m_grappleTargetVesselName, pChildForDetach->GetName());    // set new grapple target to name of child just detached

        // find the next DOWNWARD slot that contains cargo and select it
        const int orgSelectedSlot = m_selectedSlot;
        for (;;)
        {
            if (--m_selectedSlot < 1)
                m_selectedSlot = m_pPayloadBay->GetSlotCount();    // wrap around

            const XRPayloadBaySlot *pSlot = m_pPayloadBay->GetSlot(m_selectedSlot);
            if ((m_selectedSlot == orgSelectedSlot) || (pSlot->GetChild() != nullptr))
            {
                // set the active level for the new slot as well
                m_selectedSlotLevel = pSlot->GetLevel();
                break;
            }
        }
    }

    return retVal;
}

// Deploy *all* payload in the bay; this handles landed and orbit modes automatically.
// Returns: # of payload objects successfully deployed.
int DeltaGliderXR1::DeployAllPayload()
{
    // verify that bay doors are open AND there is payload in ANY slot
    if (ValidateBayStatus(true, 0, true) == false)
        return false;

    int retVal = 0;

    // for wheel-stop, deploy in LANDED mode
    if (IsLanded())
    {
        retVal = m_pPayloadBay->DetachAllChildrenLanded();

        if (retVal > 0)
        {
            PlaySound(BeepHigh, ST_Other);
            char msg[60];
            sprintf(msg, "%d cargo module(s) unloaded.", retVal);
            ShowInfo("Cargo Deployed.wav", DeltaGliderXR1::ST_InformationCallout, msg);
        }
        else
        {
            // really should never happen: this means all the child vessels refused detachment
            PlaySound(Error1, DeltaGliderXR1::ST_Other, ERROR1_VOL);
            ShowWarning("Cargo Deployment Failed.wav", DeltaGliderXR1::ST_WarningCallout, "Cargo deployment FAILED.");
        }
    }
    else  // deploy in ORBIT mode
    {
        retVal = m_pPayloadBay->DetachAllChildren(-m_deployDeltaV);  // negative so we deploy UP out of the bay

        if (retVal > 0)
        {
            PlaySound(BeepHigh, ST_Other);
            char msg[60];
            sprintf(msg, "%d cargo module(s) deployed at %.1f m/s.", retVal, m_deployDeltaV);
            ShowInfo("Cargo Deployed.wav", DeltaGliderXR1::ST_InformationCallout, msg);
        }
        else
        {
            // really should never happen: this means all the child vessels refused detachment
            PlaySound(Error1, DeltaGliderXR1::ST_Other, ERROR1_VOL);
            ShowWarning("Cargo Deployment Failed.wav", DeltaGliderXR1::ST_WarningCallout, "Cargo deployment FAILED.");
        }
    }

    // do not change the grapple target

    // on success, refresh cargo in range
    if (retVal)
        RefreshGrappleTargetsInDisplayRange(); 

    return retVal;
}

// Grapple the targeted payload object into the selected bay slot
// Returns: true on success, false on error
// showMessage: true = show info message and play a callout; false = show no message and play no sound
bool DeltaGliderXR1::GrapplePayload(const int slotNumber, const bool showMessage)
{
    bool retVal;

    // verify that a grapple target is selected
    const XRGrappleTargetVessel *pGrappleTargetVessel = GetGrappleTargetVessel(m_grappleTargetVesselName);  // pulls data from cache (this logic is in the framework classes)
    if (IsGrappleTargetVesselValidAndInDisplayRange(pGrappleTargetVessel) == false)
    {
        if (showMessage)
        {
            PlaySound(Error1, DeltaGliderXR1::ST_Other, ERROR1_VOL);
            ShowWarning("No Grapple Target Selected.wav", DeltaGliderXR1::ST_WarningCallout, "No grapple target selected.");
        }
        return false;
    }

    // verify that a slot is selected
    if (VerifySlotSelected(showMessage) == false)
        return false;

    // Verify that bay doors are open AND there is no payload in the requested slot.
    // This also will display an error message if requested.
    if (ValidateBayStatus(false, slotNumber, showMessage) == false)
        return false;

    // Validate that we are in RANGE to grapple the payload
    if (pGrappleTargetVessel->GetDistance() > GetPayloadGrappleRangeLimit())
    {
        if (showMessage)
        {
            PlaySound(Error1, DeltaGliderXR1::ST_Other, ERROR1_VOL);
            char msg[256];
            sprintf(msg, "Grapple target is out-of-range&(%.1f meters): grappling range is&%.1lf meters.", pGrappleTargetVessel->GetDistance(), GetPayloadGrappleRangeLimit());
            ShowWarning("Out of Range.wav", DeltaGliderXR1::ST_WarningCallout, msg);
        }
        return false;
    }

    // Validate that the Delta-V is OK to grapple the payload
    const double targetDeltaV = fabs(pGrappleTargetVessel->GetDeltaV());
    if (targetDeltaV > PAYLOAD_GRAPPLE_MAX_DELTAV)
    {
        if (showMessage)
        {
            PlaySound(Error1, DeltaGliderXR1::ST_Other, ERROR1_VOL);
            char msg[128];
            sprintf(msg, "Grapple target's delta-V is too high&(%.1f m/s): grappling limit is %.1lf m/s.", targetDeltaV, PAYLOAD_GRAPPLE_MAX_DELTAV);
            ShowWarning("Excess DeltaV.wav", DeltaGliderXR1::ST_WarningCallout, msg);
        }
        return false;
    }
    
    // Target is IN RANGE and DELTA-V is OK: try to grapple it
    VESSEL *pTargetVessel = pGrappleTargetVessel->GetTargetVessel();    // will never be null here
    retVal = m_pPayloadBay->AttachChild(pTargetVessel->GetHandle(), slotNumber);  

    // check whether the payload FITS into the slot
    if (retVal == false)
    {
        if (showMessage)
        {
            PlaySound(Error1, DeltaGliderXR1::ST_Other, ERROR1_VOL);
            ShowWarning("No Room In Selected Bay Slot.wav", DeltaGliderXR1::ST_WarningCallout, "Grapple target will not fit in the&selected bay slot.");
        }
        return false;
    }

    // success!
    if (showMessage)
    {
        PlaySound(BeepHigh, ST_Other);
        PlaySound(SupplyHatch, DeltaGliderXR1::ST_Other);   // use maximum volume here instead of SUPPLY_HATCH_VOL
        char msg[256];    
        sprintf(msg, "Cargo module %s&grappled and latched into slot %d.", pTargetVessel->GetName(), slotNumber);
        ShowInfo("Cargo Latched In Bay.wav", DeltaGliderXR1::ST_InformationCallout, msg);
    }

    // Since grapple was successful, set the grapple target to the NEXT available vessel, if any.
    // Also bump UP to next slot automatically; this is so that cargo can be latched from bottom-to-top.  
    AdjustGrappleTarget(+1, false);     // no sound for this

    // find the next UPWARD slot that is free and select it
    const int orgSelectedSlot = m_selectedSlot;
    for (;;)
    {
        if (++m_selectedSlot > m_pPayloadBay->GetSlotCount())
            m_selectedSlot = 1;    // wrap around

        const XRPayloadBaySlot *pSlot = m_pPayloadBay->GetSlot(m_selectedSlot);
        if ((m_selectedSlot == orgSelectedSlot) || (pSlot->IsOccupied() == false))
        {
            // set the active level for the new slot as well
            m_selectedSlotLevel = pSlot->GetLevel();
            break;
        }
    }

    return true;
}

// Bump the grapple range to the next value
void DeltaGliderXR1::IncGrappleRange(const bool playBeep)
{
    if (++m_grappleRangeIndex >= GRAPPLE_DISPLAY_RANGE_COUNT)
        m_grappleRangeIndex = 0;    // wrap around

    RefreshGrappleTargetsInDisplayRange();  // update vessels in range

    if (playBeep)
        PlaySound(BeepHigh, ST_Other);
}

// Adjust the selected grapple target vessel, wrapping around if necessary.
// adjustment: typically +1 or -1
// returns: true on success, false on error
bool DeltaGliderXR1::AdjustGrappleTarget(const int adjustment, const bool playBeep)
{
    RefreshGrappleTargetsInDisplayRange();  // ensure up-to-date state data

    const int vectorSize = static_cast<int>(m_xrGrappleTargetVesselsInDisplayRange.size());

    if (vectorSize == 0)
    {
        if (playBeep)
            PlaySound(Error1, DeltaGliderXR1::ST_Other, ERROR1_VOL);

        return false;
    }

    int newIndex = 0;       // index into m_xrGrappleTargetVesselsInDisplayRange vector

    // locate our current grapple target, if any
    int currentIndex = 0;
    bool found = false;
    for (; currentIndex < vectorSize; currentIndex++)
    {
        const XRGrappleTargetVessel *pGrappleTargetVessel = m_xrGrappleTargetVesselsInDisplayRange[currentIndex];   // will never be null
        VESSEL *pVessel = pGrappleTargetVessel->GetTargetVessel();   // will never be null since we refreshed grapple targets above
        if (strcmp(pVessel->GetName(), m_grappleTargetVesselName) == 0)
        {
            // found it; vector index is in currentIndex
            found = true;
            break;
        }
    }

    if (found)
    {
        newIndex = currentIndex + adjustment;
    }
    else
    {
        // current target not in vector, so reset to target zero
        newIndex = 0;
    }

    if (newIndex < 0)
        newIndex = vectorSize-1;    // wrap around
    else if (newIndex >= vectorSize)
        newIndex = 0;       // wrap around

    // update m_grappleTargetVesselName with the new grapple target
    const XRGrappleTargetVessel *pGrappleTargetVessel = m_xrGrappleTargetVesselsInDisplayRange[newIndex];
    strcpy(m_grappleTargetVesselName, pGrappleTargetVessel->GetTargetVessel()->GetName());  // GetTargetVessel will always be valid here

    if (playBeep)
    {
        if (adjustment > 0)
            PlaySound(BeepHigh, ST_Other);
        else if (adjustment < 0)
            PlaySound(BeepLow, ST_Other);
    }

    return true;
}

// Grapple *all* payload in range; this handles landed and orbit modes automatically.
// Returns: # of payload objects successfully attached.
int DeltaGliderXR1::GrappleAllPayload()
{
    RefreshGrappleTargetsInDisplayRange();     // ensure state of m_xrGrappleTargetVesselsInDisplayRange is current

    // verify that bay doors are open AND there is at least ONE free slot
    if (ValidateBayStatus(false, 0, true) == false)
        return false;

    // Iterate through all payload in grapple display
    // range and grapple into the bay into the first free slot for each on that is in GRAPPLE range.
    int vesselsAttached = 0;
    int vesselsInGrappleRange = 0;

    // process each targetVesselIndex once and only once; we need this for sorting
    vector<int> processedIndexes;
    while (processedIndexes.size() < m_xrGrappleTargetVesselsInDisplayRange.size())
    {
        double largestTotalSlots = -1;  // largest (length + width + height) value found in this loop
        int indexWithLargestChild = -1; 

        // loop through all vessels until we find the largest one we haven't processed yet
        for (unsigned int targetVesselIndex = 0; targetVesselIndex < m_xrGrappleTargetVesselsInDisplayRange.size(); targetVesselIndex++)
        {
            bool processed = false;
            for (int i=0; i < static_cast<int>(processedIndexes.size()); i++)
            {
                if (processedIndexes[i] == targetVesselIndex)
                {
                    processed = true;
                    break;
                }
            }

            if (processed)      // already processed this targetVesselIndex?
                continue;       // skip it

            // we didn't process targetVesselIndex yet: check its size
            const XRGrappleTargetVessel *pGrappleTarget = m_xrGrappleTargetVesselsInDisplayRange[targetVesselIndex];
            const VECTOR3 &dim = pGrappleTarget->GetTargetPCD().GetDimensions();
            const double totalSlots = dim.x + dim.y + dim.z;
            if (totalSlots > largestTotalSlots)
            {
                largestTotalSlots = totalSlots;
                indexWithLargestChild = targetVesselIndex;  // largest child so far
            }
        }

        // found the largest child to be grappled
        _ASSERTE(indexWithLargestChild >= 0);
        processedIndexes.push_back(indexWithLargestChild);  // remember this so we don't re-process it later

        // indexWithLargestChild now contains the child with the largest payload; try to grapple it into the 
        // first available slot.
        const XRGrappleTargetVessel *pGrappleTarget = m_xrGrappleTargetVesselsInDisplayRange[indexWithLargestChild];
        _ASSERTE(pGrappleTarget != nullptr);

        // check if vessel is in grappling range
        if (pGrappleTarget->GetDistance() <= GetPayloadGrappleRangeLimit())
        {
            vesselsInGrappleRange++;

            // Validate that the Delta-V is OK to grapple the payload
            const double targetDeltaV = fabs(pGrappleTarget->GetDeltaV());
            if (targetDeltaV <= PAYLOAD_GRAPPLE_MAX_DELTAV)
            {
                // vessel is OK to grapple!

                // try to attach this vessel using each slot until we find one that fits
                for (int slotNumber = 1; slotNumber <= m_pPayloadBay->GetSlotCount(); slotNumber++)
                {
                    if (m_pPayloadBay->AttachChild(pGrappleTarget->GetTargetVessel()->GetHandle(), slotNumber))
                    {
                        vesselsAttached++;
                        break;
                    }
                }
            }
        }
    }

    // verify at least ONE vessel was in grapple range
    if (vesselsInGrappleRange == 0)
    {
        PlaySound(Error1, DeltaGliderXR1::ST_Other, ERROR1_VOL);
        ShowWarning("No Cargo in Grapple Range.wav", DeltaGliderXR1::ST_WarningCallout, "No cargo in grapple range.");
        return false;
    }

    // check whether any were attached successfully
    if (vesselsAttached > 0)
    {
        char msg[60];
        sprintf(msg, "%d cargo module(s) attached successfully.", vesselsAttached);
        ShowInfo("Cargo Latched In Bay.wav", DeltaGliderXR1::ST_InformationCallout, msg);

        // must refresh grapple targets since we removed some vessels (which are now attached)
        RefreshGrappleTargetsInDisplayRange();

        // Note: current grapple target (m_grappleTargetVesselName) is unchanged
    }
    else
    {
        ShowWarning("Auto-Grapple Failed.wav", DeltaGliderXR1::ST_WarningCallout, "No modules within grappling limits.");
    }

    return vesselsAttached;
}

// Verify that a slot is selected; i.e., selected slot number is > 0.
// Returns: true of slot OK, false if no slot selected
// showMessage: true = show info message and play a callout; false = show no message and play no sound
bool DeltaGliderXR1::VerifySlotSelected(const bool showMessage)
{
    bool retVal = true;

    if (m_selectedSlot < 1)
    {
        if (showMessage)
        {
            PlaySound(Error1, DeltaGliderXR1::ST_Other, ERROR1_VOL);
            ShowWarning("No Slot Selected.wav", DeltaGliderXR1::ST_WarningCallout, "No payload bay slot selected.");
        }
        retVal = false;
    }

    return retVal;
}

// Validate that the bay doors are open and there is/isn't payload in the specified slot.
// slotNumber: 0 = validate there is at least ONE slot, > 0 = check only the specified slot
// showMessage: true = show info message and play a callout; false = show no message and play no sound
// Returns: true if bay is OK, false if not OK
bool DeltaGliderXR1::ValidateBayStatus(const bool isPayloadRequiredInSlot, const int slotNumber, const bool showMessage)
{
    char msg[60];

    // are the bay doors open?
    if (bay_status != DoorStatus::DOOR_OPEN)
    {
        if (showMessage)
        {
            PlaySound(Error1, DeltaGliderXR1::ST_Other, ERROR1_VOL);
            ShowWarning("Bay Doors Are Closed.wav", DeltaGliderXR1::ST_WarningCallout, "Bay doors are closed.");       
        }
        return false;
    }

    // is payload in/not it the requested slot/any slot?
    if (isPayloadRequiredInSlot)
    {
        if (slotNumber < 1)
        {
            // at least ONE slot must contain a payload module
            if (m_pPayloadBay->GetChildCount() == 0)
            {
                if (showMessage)
                {
                    PlaySound(Error1, DeltaGliderXR1::ST_Other, ERROR1_VOL);
                    ShowWarning("Bay is Empty.wav", DeltaGliderXR1::ST_WarningCallout, "No cargo in bay.");
                }
                return false;
            }
        }
        else 
        {
            // the specified slot must contain a payload module (i.e., it may be deployed)
            if (m_pPayloadBay->GetChild(slotNumber) == nullptr)
            {
                if (showMessage)
                {
                    PlaySound(Error1, DeltaGliderXR1::ST_Other, ERROR1_VOL);
                    // show a more informative message if the bay is empty
                    if (m_pPayloadBay->GetChildCount() == 0)
                    {
                        ShowWarning("Bay is Empty.wav", DeltaGliderXR1::ST_WarningCallout, "No cargo in bay.");
                    }
                    else    // bay is not empty
                    {
                        sprintf(msg, "No cargo in slot %d.", slotNumber);
                        PlaySound(Error1, DeltaGliderXR1::ST_Other, ERROR1_VOL);
                        ShowWarning("Slot is Empty.wav", DeltaGliderXR1::ST_WarningCallout, msg);
                    }
                }
                return false;
            }
        }

    }
    else    // slot (or at least (*one* slot) should be empty 
    {
        if (slotNumber < 1)
        {
            // at least ONE slot must be free
            if (m_pPayloadBay->GetChildCount() == m_pPayloadBay->GetSlotCount())
            {
                if (showMessage)
                {
                    PlaySound(Error1, DeltaGliderXR1::ST_Other, ERROR1_VOL);
                    ShowWarning("Bay is Full.wav", DeltaGliderXR1::ST_WarningCallout, "Payload bay is full.");
                }
                return false;
            }
        }
        else 
        {
            // the specified slot must be free for attaching a child.
            if (m_pPayloadBay->GetSlot(slotNumber)->IsOccupied())
            {
                if (showMessage)
                {
                    sprintf(msg, "Slot %d is occupied.", slotNumber);
                    PlaySound(Error1, DeltaGliderXR1::ST_Other, ERROR1_VOL);
                    ShowWarning("Slot Is Full.wav", DeltaGliderXR1::ST_WarningCallout, msg);
                }
                return false;
            }
        }
    }

    return true;
}

// Returns true if supplied vessel is valid and in grappling display range, false otherwise
// pGrappleTargetVessel : may be null
bool DeltaGliderXR1::IsGrappleTargetVesselValidAndInDisplayRange(const XRGrappleTargetVessel *pGrappleTargetVessel) const
{
    bool retVal = false;

    if (pGrappleTargetVessel != nullptr)  // any grapple target?
    {
        // check whether target vessel is in our 'valid target vessels' vector
        for (unsigned int i=0; i < m_xrGrappleTargetVesselsInDisplayRange.size(); i++)
        {
            if (m_xrGrappleTargetVesselsInDisplayRange[i] == pGrappleTargetVessel)
            {
                retVal = true;
                break;
            }
        }
    }
    return retVal;
}

// Iterate through all vessels and rebuild 'm_xrGrappleTargetVesselsInDisplayRange', which contains the 
// list of vessels in range of 'GRAPPLE_DISPLAY_RANGES[m_grappleRangeIndex]'.
// NOTE: this is a relatively expensive method, so you should only call it when necessary; i.e., not every frame.
// Also note that the currently-selected grapple target (m_grappleTargetVesselName), if any, is not changed.
void DeltaGliderXR1::RefreshGrappleTargetsInDisplayRange()
{
    const double range = GetGrappleDisplayRange();

    m_xrGrappleTargetVesselsInDisplayRange.clear();    // this will be rebuilt below

    // NOTE: Orbiter tends to keep vessels in a given order, so we don't need to worry about the
    // list being constructed out-of-order from the current order here.
    for (unsigned int i=0; i < oapiGetVesselCount(); i++)
    {
        const OBJHANDLE hVessel = oapiGetVesselByIndex(i);

        /* NOTE: check should not be required! 
        // If vessel deleted already, skip it!
        if (oapiIsVessel(hVessel) == false)
            continue;
        */

        const VESSEL *pVessel = oapiGetVesselInterface(hVessel);

        // If vessel is *us*, skip it!
        if (hVessel == GetHandle())
            continue;

        // check whether this vessel is in range
        const double distance = GetDistanceToVessel(*pVessel);
        if (distance <= range)
        {
            // vessel is in range; only show in list if vessel is NOT attached in the bay
            if (m_pPayloadBay->IsChildVesselAttached(hVessel) == false)
            {
                // vessel is in range and NOT attached in the bay: check whether it is an XR payload vessel
                const XRPayloadClassData &pcd = XRPayloadClassData::GetXRPayloadClassDataForClassname(pVessel->GetClassName());
                if (pcd.IsXRPayloadEnabled())
                {
                    // Note: this SHOULD never be null here since we know the vessel exists at this point, but
                    // Orbiter tends to keep just-deleted vessels around for a frame afterward, so we have to handle that.
                    const XRGrappleTargetVessel *pGrappleTarget = GetGrappleTargetVessel(pVessel->GetName());
                    // WARNING: if two Orbiter vessels exist with the same name, bad things happen here because a second vessel can exist!
                    // I added code to prevent that from happening, but we still want to do defensive coding here.
                    if (pGrappleTarget != nullptr)
                    {
                        // add vessel to the payload-in-range list
                        m_xrGrappleTargetVesselsInDisplayRange.push_back(pGrappleTarget);
                    }
                }
            }
        }
    }
}

const double maxDeployDeltaV = 100;  // used below

// Set a new payload deploy Delta-V; this is range-checked for limits.
// adjustment: after applying change, deltaV will be range-checked to 0 <= deltaV <= 100
// showMessage: true = show info message and play beep, false = no msg or beep
void DeltaGliderXR1::AdjustPayloadDeployDeltaV(double adjustment, const bool showMessage)
{
    m_deployDeltaV += adjustment;

    // range-check
    if (m_deployDeltaV < 0)
        m_deployDeltaV = 0;
    else if (m_deployDeltaV > maxDeployDeltaV)
        m_deployDeltaV = maxDeployDeltaV;

    if (showMessage)
    {
        char msg[50];
        sprintf(msg, "Deployment delta-V set to %.1f m/s", m_deployDeltaV);
        PlaySound(BeepHigh, ST_Other);
        ShowInfo(nullptr, DeltaGliderXR1::ST_None, msg);
    }
}

// Set a new payload deploy Delta-V; this is range-checked for limits
// deltaV: range-checked to 0 <= deltaV <= 100
// showMessage: true = show info message and play beep, false = no msg or beep
void DeltaGliderXR1::SetPayloadDeployDeltaV(double deltaV, const bool showMessage)
{
    m_deployDeltaV = deltaV;

    // range-check
    if (m_deployDeltaV < 0)
        m_deployDeltaV = 0;
    else if (m_deployDeltaV > maxDeployDeltaV)
        m_deployDeltaV = maxDeployDeltaV;

    if (showMessage)
    {
        char msg[50];
        sprintf(msg, "Deployment delta-V set to %.1f m/s", m_deployDeltaV);
        PlaySound(BeepHigh, ST_Other);
        ShowInfo(nullptr, DeltaGliderXR1::ST_None, msg);
    }
}

// clear the current grapple target, if any
void DeltaGliderXR1::ClearGrappleTarget(bool playBeep)
{
    *m_grappleTargetVesselName = 0;

    if (playBeep)
        PlaySound(BeepHigh, ST_Other);
}

#if 0 // NO WAY TO DO THIS
// Track the current grapple target, if any, on the docking HUD
// Returns: true on success, false on error
bool DeltaGliderXR1::TrackGrappleTarget(bool showMessage)
{
    const XRGrappleTargetVessel *pGrappleTargetVessel = GetGrappleTargetVessel(m_grappleTargetVesselName);  // pulls data from cache (this logic is in the framework classes)
    if (IsGrappleTargetVesselValidAndInDisplayRange(pGrappleTargetVessel) == false)
    {
        if (showMessage)
        {
            PlaySound(Error1, DeltaGliderXR1::ST_Other, ERROR1_VOL);
            ShowWarning("No Grapple Target Selected.wav", DeltaGliderXR1::ST_WarningCallout, "No grapple target selected.");
        }
        return false;
    }

    // Target is valid; do two things:
    // 1. Switch to docking HUD mode.
    // 2. Set our docking target in VESSELSTATUS.
    oapiSetHUDMode(HUD_DOCKING);
    const VESSEL *pVessel = pGrappleTargetVessel->GetTargetVessel();  // will never be null here
    VESSELSTATUS status;
    VESSEL3_EXT::GetStatusSafe(*pVessel, status);
    // UNKNOWN ORBITER CORE BEHAVIOR: status.base is always 0 for vessel targets on the HUD
    status.base = pVessel->GetHandle();     // set docking HUD to this target
    DefSetState(&status);

    if (showMessage)
    {
        PlaySound(BeepHigh, ST_Other);
        char msg[128];
        sprintf(msg, "Tracking cargo '%s'", pVessel->GetName());
        ShowInfo("Tracking Cargo.wav",  msg);
    }

    return true;
}
#endif

