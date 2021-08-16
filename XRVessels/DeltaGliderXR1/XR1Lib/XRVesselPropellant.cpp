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
// XR vessel propellant- and LOX-related methods
// ==============================================================

#include "DeltaGliderXR1.h"
#include "XRPayloadBay.h"

//
// Fuel/LOX quantity methods; these take any payload bay consumables into account
//

// returns the propellant type for the given propellant handle
// ph may not be null!
PROP_TYPE DeltaGliderXR1::GetPropTypeForHandle(PROPELLANT_HANDLE ph) const
{
    PROP_TYPE pt;

    // check each of our known propellant handle values
    if (ph == ph_main)
        pt = PROP_TYPE::PT_Main;
    else if (ph == ph_scram)
        pt = PROP_TYPE::PT_SCRAM;
    else if (ph == ph_rcs)
        pt = PROP_TYPE::PT_NONE;  // no separate fuel tank for this
    else
    {
        // should never happen!
        _ASSERTE(false);
        pt = PROP_TYPE::PT_NONE;
    }

    return pt;
}

// Returns the max capacity of this propellant, including payload tank(s) capacity.
//
// WARNING: this may return zero depending on how a given vessel configures its fuel tanks!
// If you need to divide by this return value, use the SAFE_FRACTION global inline method,
// which returns zero if the denominator is zero.  Of course, that macro should only be used
// for gauges where you need to render a percentage of fuel in the tank; i.e., if the max
// mass == 0, then the tank is always empty (0).
double DeltaGliderXR1::GetXRPropellantMaxMass(PROPELLANT_HANDLE ph) const
{
    double totalMass = oapiGetPropellantMaxMass(ph);
    if (m_pPayloadBay != nullptr)
    {
        const PROP_TYPE pt = GetPropTypeForHandle(ph);
        if (pt != PROP_TYPE::PT_NONE)   // no extra capacity for RCS
            totalMass += m_pPayloadBay->GetPropellantMaxMass(pt);
    }

    return totalMass;
}

// returns the current quantity of this propellant, including payload tank(s) capacity
double DeltaGliderXR1::GetXRPropellantMass(PROPELLANT_HANDLE ph) const
{
    return (oapiGetPropellantMass(ph) + GetXRBayPropellantMass(ph));
}

// returns the current quantity of this propellant in the payload bay tanks *only*
double DeltaGliderXR1::GetXRBayPropellantMass(PROPELLANT_HANDLE ph) const
{
    double bayPropMass = 0;
    if (m_pPayloadBay != nullptr)
    {
        const PROP_TYPE pt = GetPropTypeForHandle(ph);
        if (pt != PROP_TYPE::PT_NONE)   // no extra capacity for RCS
            bayPropMass = m_pPayloadBay->GetPropellantMass(pt);
    }

    return bayPropMass;
}


// sets propellant quantity, including payload tank(s).
// Note: internal tanks are always filled *first*
void DeltaGliderXR1::SetXRPropellantMass(PROPELLANT_HANDLE ph, const double mass)
{
    double deltaRemaining = mass;

    // fill the internal tank first
    const double internalTankQty = min(mass, oapiGetPropellantMaxMass(ph));
    SetPropellantMass(ph, internalTankQty);
    deltaRemaining -= internalTankQty;

    // now store any remainder in the payload bay, if any bay exists
    if (m_pPayloadBay != nullptr)
    {
        // get the delta between the current payload bay quantity and the new quantity
        const PROP_TYPE pt = GetPropTypeForHandle(ph);
        if (pt != PROP_TYPE::PT_NONE)   // no extra capacity for RCS
        {
            const double bayDeltaRequested = deltaRemaining - m_pPayloadBay->GetPropellantMass(pt);  // newBayMass - currentBayMass

            // apply the delta (as a *request*) to the bay tanks
            const double bayDeltaApplied = AdjustBayPropellantMassWithMessages(pt, bayDeltaRequested);

            // if the caller's code is correct, we should never overflow the bay quantity
            // Need to account for slight rounding error possibility here in the nth decimal place, so 0.01 is way overkill, but fine for an assert
            _ASSERTE(fabs(bayDeltaApplied - bayDeltaRequested) < 0.01);
        }
    }
    else
    {
        // no payload bay, so it should all fit in the internal tank!
        // Need to account for slight rounding error possibility here in the nth decimal place, so 0.01 is way overkill, but fine for an assert
        _ASSERTE(fabs(deltaRemaining) < 0.01);
    }
}

// Adjust the propellant mass in the bay, displaying information messages if 
// bay slots empty or fill.
//
// Returns: quantity of propellant/LOX added to or removed from bay (negative = removed)
double DeltaGliderXR1::AdjustBayPropellantMassWithMessages(const PROP_TYPE pt, const double requestedFlowQty)
{
    // let's be efficient here...
    if (requestedFlowQty == 0)
        return 0;

    const XRPayloadBay::SlotsDrainedFilled& slotsDrainedFilled = m_pPayloadBay->AdjustPropellantMass(pt, requestedFlowQty);

    // Note: although it is possible that multiple bay tanks will fill or empty here, it is
    // highly unlikey that more than one, or at the most, two, slots will fill or drain within
    // the same timestep.  Therefore, we won't worry about handling 32 slots numbers all at once
    // in the msg string.
    char msg[120];  // should be long enough for all slots, just in some bizarre case that we need it

    // first, construct a message listing each bay slot filled
    if (slotsDrainedFilled.filledList.size() > 0)
    {
        strcpy(msg, "Bay tank(s) full: ");  // slot numbers appended below
        for (unsigned int i = 0; i < slotsDrainedFilled.filledList.size(); i++)
        {
            if (i > 0)  // more than one tank?
                strcat(msg, ", ");  // add separator

            const int msgIndex = static_cast<int>(strlen(msg));  // overwrite current null terminating byte
            sprintf(msg + msgIndex, "#%d", slotsDrainedFilled.filledList[i]);
        }

        PlaySound(BeepHigh, ST_Other);
        ShowInfo(nullptr, ST_None, msg);
    }
    else if (slotsDrainedFilled.drainedList.size() > 0)  // Note: we will *never* have tanks both full and drained in the same timestep!
    {
        strcpy(msg, "ALERT: Bay tank(s) empty: ");  // slot numbers appended below
        for (unsigned int i = 0; i < slotsDrainedFilled.drainedList.size(); i++)
        {
            if (i > 0)  // more than one tank?
                strcat(msg, ", ");  // add separator

            const int msgIndex = static_cast<int>(strlen(msg));  // overwrite current null terminating byte
            sprintf(msg + msgIndex, "#%d", slotsDrainedFilled.drainedList[i]);
        }

        PlayErrorBeep();   // this is a warning, not a status message
        ShowWarning(nullptr, ST_None, msg);
    }

    return slotsDrainedFilled.quantityAdjusted;
}

// returns max capacity of LOX tanks, including any LOX tanks in the bay
// This will always be > 0.
double DeltaGliderXR1::GetXRLOXMaxMass() const
{
    double totalMass = GetXR1Config()->GetMaxLoxMass();
    if (m_pPayloadBay != nullptr)
        totalMass += m_pPayloadBay->GetPropellantMaxMass(PROP_TYPE::PT_LOX);

    return totalMass;
}

// returns the current quantity of LOX, including any LOX in the bay
double DeltaGliderXR1::GetXRLOXMass() const
{
    return (m_loxQty + GetXRBayLOXMass());
}

// returns the current quantity of LOX in the BAY only
double DeltaGliderXR1::GetXRBayLOXMass() const
{
    double bayLoxMass = 0;

    if (m_pPayloadBay != nullptr)
        bayLoxMass = m_pPayloadBay->GetPropellantMass(PROP_TYPE::PT_LOX);

    return bayLoxMass;
}

// set the quantity of LOX, including any LOX tank(s)
// Note: internal tanks are always filled *first*
void DeltaGliderXR1::SetXRLOXMass(const double mass)
{
    double deltaRemaining = mass;

    // fill the internal tank first
    const double internalTankQty = min(mass, GetXR1Config()->GetMaxLoxMass());
    m_loxQty = internalTankQty;
    deltaRemaining -= internalTankQty;

    // now store any remainder in the payload bay, if any bay exists
    if (m_pPayloadBay != nullptr)
    {
        // get the delta between the current payload bay quantity and the new quantity
        const double bayDeltaRequested = deltaRemaining - m_pPayloadBay->GetPropellantMass(PROP_TYPE::PT_LOX);  // newBayMass - currentBayMass

        // apply the delta (as a *request*) to the bay tanks
        const double bayDeltaApplied = AdjustBayPropellantMassWithMessages(PROP_TYPE::PT_LOX, bayDeltaRequested);

        // if the caller's code is correct, we should never overflow the bay quantity
        // Need to account for slight rounding error possibility here in the nth decimal place, so 0.01 is way overkill, but fine for an assert
        _ASSERTE(fabs(bayDeltaApplied - bayDeltaRequested) < 0.01);
    }
    else
    {
        // no payload bay, so it should all fit in the internal tank!
        // Need to account for slight rounding error possibility here in the nth decimal place, so 0.01 is way overkill, but fine for an assert
        _ASSERTE(fabs(deltaRemaining) < 0.01);
    }
}