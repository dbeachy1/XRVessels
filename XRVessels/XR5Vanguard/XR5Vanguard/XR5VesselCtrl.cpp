// ==============================================================
// Class implementing the XRVesselCtrl interface.
//
// Copyright 2007-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR5VesselCtrl.cpp
// ==============================================================

#include "XR5Vanguard.h"

// NOTE: none of these methods perform any significant operations themselves on the internal state of the XR5: they call internal XR5 methods
// to do any "heavy lifting."  None of the other XRn methods invoke any methods in this file; in other words, these methods are not required
// for operation of the XRn.  They are separate and stand-alone.

// Door State
// returns TRUE if door is valid for this ship
bool XR5Vanguard::SetDoorState(XRDoorID id, XRDoorState state)            
{
    bool retVal = true;

    switch (id)
    {
    case XRD_CrewElevator: 
        ActivateElevator(ToDoorStatus(state));
        break;

    case XRD_PayloadBayDoors:
        ActivateBayDoors(ToDoorStatus(state));
        break;

    // airlock ladder is not supported by the XR5
    case XRD_Ladder:
        return false;   // door not supported

    default:
        // let the superclass handle it
        retVal = DeltaGliderXR1::SetDoorState(id, state);   
        break;
    }

    return retVal;
}

#define SET_IF_NOT_NULL(ptr, value) if (ptr != nullptr) *ptr = value;
// returns XRDS_DoorNotSupported if door does not exist for this ship; if pProc != nullptr, proc is set to 0 <= n <= 1.0
XRDoorState XR5Vanguard::GetDoorState(XRDoorID id, double *pProc) const  
{
    XRDoorState retVal;

    switch (id)
    {
    case XRD_CrewElevator: 
        retVal = ToXRDoorState(crewElevator_status);
        SET_IF_NOT_NULL(pProc, crewElevator_proc);
        break;

    case XRD_PayloadBayDoors:
        retVal = ToXRDoorState(bay_status);
        SET_IF_NOT_NULL(pProc, bay_proc);
        break;

    // airlock ladder is not supported by the XR5
    case XRD_Ladder:
        retVal = XRDS_DoorNotSupported;
        SET_IF_NOT_NULL(pProc, -1);
        break;

    default:
        // let the superclass handle it
        retVal = DeltaGliderXR1::GetDoorState(id, pProc);
        break;
    }

    return retVal;
}

// Set the damage status of the XR Vessel; any unsupported fields in 'status' must be set to -1 (for doubles) or XRDMG_NotSupported (for XRDamageState)
bool XR5Vanguard::SetXRSystemStatus(const XRSystemStatusWrite &status)
{
    // Invoke the superclass to handle all the normal fields
    DeltaGliderXR1::SetXRSystemStatus(status);

    // handle custom fields
    const double bayDoorState = ((status.PayloadBayDoors == XRDMG_online) ? 1.0 : 0.0);
    SetDamageStatus(BayDoors, bayDoorState);

    const double elevatorState = ((status.CrewElevator == XRDMG_online) ? 1.0 : 0.0);
    SetDamageStatus(Elevator, elevatorState);

    // check the user attempting to set any unsupported fields
    bool retVal = true;
    // (no unsupported fields at this time)

    return retVal;
}

// Read the status of the XR vessel
void XR5Vanguard::GetXRSystemStatus(XRSystemStatusRead &status) const
{
    // Invoke the superclass to fill in the base values; this must be invoked *before* we populate our custom values.
    DeltaGliderXR1::GetXRSystemStatus(status);

    status.PayloadBayDoors = ((GetDamageStatus(BayDoors).fracIntegrity == 1.0) ? XRDMG_online : XRDMG_offline);
    status.CrewElevator    = ((GetDamageStatus(Elevator).fracIntegrity == 1.0) ? XRDMG_online : XRDMG_offline);
}

// RCS Mode
// returns true if RCS DOCKING mode is active, false if RCS is in NORMAL mode
bool XR5Vanguard::IsRCSDockingMode() const
{
    return m_rcsDockingMode;
}

// NOTE: SetRCSDockingMode already defined in our class

// Active EVA port
// returns true if crew elevator is the active EVA port, false if the docking port is active
bool XR5Vanguard::IsElevatorEVAPortActive() const
{
    return (m_activeEVAPort == CREW_ELEVATOR);
}

// true = crew elevator active, false = docking port active
// returns: true on success, false if crew elevator not supported
bool XR5Vanguard::SetElevatorEVAPortActive(bool on)
{
    ACTIVE_EVA_PORT newState = (on ? CREW_ELEVATOR : DOCKING_PORT);
    SetActiveEVAPort(newState);
    return true;
}
