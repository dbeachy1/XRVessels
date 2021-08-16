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
// Class implementing the XRVesselCtrl interface.
//
// Copyright 2007-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR3VesselCtrl.cpp
// ==============================================================

#include "XR3Phoenix.h"

// NOTE: none of these methods perform any significant operations themselves on the internal state of the XR3: they call internal XR3 methods
// to do any "heavy lifting."  None of the other XRn methods invoke any methods in this file; in other words, these methods are not required
// for operation of the XRn.  They are separate and stand-alone.

// Door State
// returns TRUE if door is valid for this ship
bool XR3Phoenix::SetDoorState(XRDoorID id, XRDoorState state)            
{
    bool retVal = true;

    switch (id)
    {
    case XRDoorID::XRD_CrewElevator:
        ActivateElevator(ToDoorStatus(state));
        break;

    case XRDoorID::XRD_PayloadBayDoors:
        ActivateBayDoors(ToDoorStatus(state));
        break;

    // airlock ladder is not supported by the XR3
    case XRDoorID::XRD_Ladder:
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
XRDoorState XR3Phoenix::GetDoorState(XRDoorID id, double *pProc) const  
{
    XRDoorState retVal;

    switch (id)
    {
    case XRDoorID::XRD_CrewElevator:
        retVal = ToXRDoorState(crewElevator_status);
        SET_IF_NOT_NULL(pProc, crewElevator_proc);
        break;

    case XRDoorID::XRD_PayloadBayDoors:
        retVal = ToXRDoorState(bay_status);
        SET_IF_NOT_NULL(pProc, bay_proc);
        break;

    // airlock ladder is not supported by the XR3
    case XRDoorID::XRD_Ladder:
        retVal = XRDoorState::XRDS_DoorNotSupported;
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
bool XR3Phoenix::SetXRSystemStatus(const XRSystemStatusWrite &status)
{
    // Invoke the superclass to handle all the normal fields
    DeltaGliderXR1::SetXRSystemStatus(status);

    // handle custom fields
    const double bayDoorState = ((status.PayloadBayDoors == XRDamageState::XRDMG_online) ? 1.0 : 0.0);
    SetDamageStatus(DamageItem::BayDoors, bayDoorState);

    const double elevatorState = ((status.CrewElevator == XRDamageState::XRDMG_online) ? 1.0 : 0.0);
    SetDamageStatus(DamageItem::Elevator, elevatorState);

    // check the user attempting to set any unsupported fields
    bool retVal = true;
    // (no unsupported fields at this time)

    return retVal;
}

// Read the status of the XR vessel
void XR3Phoenix::GetXRSystemStatus(XRSystemStatusRead &status) const
{
    // Invoke the superclass to fill in the base values; this must be invoked *before* we populate our custom values.
    DeltaGliderXR1::GetXRSystemStatus(status);

    status.PayloadBayDoors = ((GetDamageStatus(DamageItem::BayDoors).fracIntegrity == 1.0) ? XRDamageState::XRDMG_online : XRDamageState::XRDMG_offline);
    status.CrewElevator    = ((GetDamageStatus(DamageItem::Elevator).fracIntegrity == 1.0) ? XRDamageState::XRDMG_online : XRDamageState::XRDMG_offline);
}

// RCS Mode
// returns true if RCS DOCKING mode is active, false if RCS is in NORMAL mode
bool XR3Phoenix::IsRCSDockingMode() const
{
    return m_rcsDockingMode;
}

// NOTE: SetRCSDockingMode already defined in our class

// Active EVA port
// returns true if crew elevator is the active EVA port, false if the docking port is active
bool XR3Phoenix::IsElevatorEVAPortActive() const
{
    return (m_activeEVAPort == ACTIVE_EVA_PORT::CREW_ELEVATOR);
}

// true = crew elevator active, false = docking port active
// returns: true on success, false if crew elevator not supported
bool XR3Phoenix::SetElevatorEVAPortActive(bool on)
{
    ACTIVE_EVA_PORT newState = (on ? ACTIVE_EVA_PORT::CREW_ELEVATOR : ACTIVE_EVA_PORT::DOCKING_PORT);
    SetActiveEVAPort(newState);
    return true;
}
