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
// XR3Phoenix implementation class
//
// XR3Phoenix_DMG.cpp
// Handles custom XR3 Damage; methods are invoked by the XR1 base class.
// ==============================================================

#include "XR3Phoenix.h"
#include "XRCommon_DMG.h"
#include "AreaIDs.h"
#include "meshres.h"

// Perform crash damage; i.e., damage all systems.  This is invoked only once when a crash occurs.
void XR3Phoenix::PerformCrashDamage()
{
    DeltaGliderXR1::PerformCrashDamage();  // handle all the common systems

    // set our custom systems to *crashed*
    bay_status          = DoorStatus::DOOR_FAILED;
    crewElevator_status = DoorStatus::DOOR_FAILED;

    // blink our new warning lights
    m_XR3WarningLights[static_cast<int>(XR3WarningLight::wl3Elev)] = true;
    m_XR3WarningLights[static_cast<int>(XR3WarningLight::wl3Bay)] = true;
}

//
// Check for door-related heat and/or dynamic pressure damage here
// Note that a given damange type is only checked if it is enabled.
//
// Returns: true if any damage detected, false otherwise
bool XR3Phoenix::CheckAllDoorDamage()
{
    bool newdamage = DeltaGliderXR1::CheckAllDoorDamage();  // check common systems; includes docking port check (nose_status)

    newdamage |= CheckDoorFailure(&crewElevator_status);
    newdamage |= CheckDoorFailure(&bay_status);

    return newdamage;  
}


//
// Check all hull surfaces for heat damage.
// 
// NOTE: CheckTemperature will turn on warning lights for doors and hull temp as necessary; however
// we must reset the HTMP light ourselves since any surface can trigger it.  CheckTemperature will only 
// SET the light, never CLEAR it.  Therefore, we clear it here ourselves.
//
// Returns: true if any surface damaged, false otherwise
bool XR3Phoenix::CheckHullHeatingDamage()
{
    double alpha = 0;
    char temp[128];
    const double mach = GetMachNumber();

    // NOTE: since the retro doors are not on the wings for this ship, mark the retro doors as CLOSED for the default wing checks in the base class method.
    // This is a bit of a hack, but it's safe and still cleaner than duplicating the wing damange check code.
    const DoorStatus orgRcoverStatus = rcover_status;
    rcover_status = DoorStatus::DOOR_CLOSED;            // disable door-open damage checks for the wings
    bool newdamage = DeltaGliderXR1::CheckHullHeatingDamage();  // check common systems; includes payload bay doors open check
    rcover_status = orgRcoverStatus;        // restore

    // nosecone max temp is tied to the retro doors and our crew elevator
    if ( (CheckTemperature(m_noseconeTemp,  m_hullTemperatureLimits.noseCone, IS_DOOR_OPEN(crewElevator_status)) != 0) ||
         (CheckTemperature(m_noseconeTemp,  m_hullTemperatureLimits.noseCone, IS_DOOR_OPEN(rcover_status)) != 0) )
    {
        // LOWER HULL FAILURE - crew death!
        sprintf(temp, "LOWER HULL BREACH at Mach %.1lf!", mach);
        DoCrash(temp, 0);       
    }

    // cockpit max temp is tied to the XR3's escape hatch
    if (CheckTemperature(m_cockpitTemp, m_hullTemperatureLimits.cockpit, IS_DOOR_OPEN(hatch_status)) != 0)
    {
        // HULL FAILURE - crew death!
        sprintf(temp, "COCKPIT BREACH at Mach %.1lf!", mach);
        DoCrash(temp, 0);
    }

    // top hull max temp is tied to: 1) radiators, 2) bay doors, and 3) docking port (uses NOSECONE animation and status)
    // 1) and 2) were already checked by the base class method, so we just need to check 3) here.
    if (CheckTemperature(m_topHullTemp, m_hullTemperatureLimits.topHull, IS_DOOR_OPEN(nose_status)) != 0)
    {
        // HULL FAILURE - crew death!
        sprintf(temp, "TOP HULL BREACH at Mach %.1lf!", mach);
        DoCrash(temp, 0);
    }

    return newdamage;
}

// Note: base class IsDamagePresent() method is sufficient

// Check whether ANY warning is active.  Invoked on startup.
// Returns: true if any warning present, false if no warnings present
bool XR3Phoenix::IsWarningPresent()
{
    // invoke the superclass
    bool retVal = DeltaGliderXR1::IsWarningPresent();

    if (retVal == false)
    {
        // loop through all new warning lights
        for (int i=0; i < XR3_WARNING_LIGHT_COUNT; i++)
        {
            bool warningLightActive = m_XR3WarningLights[i];
            if (warningLightActive)
            {
                retVal = true;  // warning present
                break;
            }
        }
    }

    return retVal;
}

// returns DamageStatus (a static variable)
// This queries the actual SYSTEM STATE (e.g., current thrust output) to determine whether an item is damaged.
const DamageStatus &XR3Phoenix::GetDamageStatus(DamageItem item) const
{
    double frac;
    const char *pLabel;
    const char *pShortLabel;
    bool onlineOffline = true;     // assume online/offline

    // check for our custom damage items first
    switch (item)
    {
    case DamageItem::BayDoors:
        frac = ((bay_status == DoorStatus::DOOR_FAILED) ? 0 : 1);
        pLabel = "Bay Doors";
        pShortLabel = "BDor";
        break;

    case DamageItem::Elevator:
        frac = ((crewElevator_status == DoorStatus::DOOR_FAILED) ? 0 : 1);
        pLabel = "Elevator";
        pShortLabel = "Elev";
        break;

    default:
        return DeltaGliderXR1::GetDamageStatus(item);  // let the superclass handle it
    }

    // populate the structure
    static DamageStatus dmgStatus;
    
    dmgStatus.fracIntegrity = frac;
    strcpy(dmgStatus.label, pLabel);
    strcpy(dmgStatus.shortLabel, pShortLabel);
    dmgStatus.onlineOffline = onlineOffline;

    return dmgStatus;   // return by reference
}

// Sets system damage based on an integrity value; invoked at load time
// Note that this is not called at runtime because the code merely needs to set the system settings
// (max engine thrust, etc.) to create damage.  In fact, that is what we do in this method.
void XR3Phoenix::SetDamageStatus(DamageItem item, double fracIntegrity)
{
// NOTE: because some warning lights can have multiple causes (e.g., left and right engines), we never CLEAR a warning flag here
#define SET_WARNING_LIGHT(wlIdx)  m_XR3WarningLights[static_cast<int>(XR3WarningLight::wlIdx)] |= (fracIntegrity < 1.0)

    // check for our custom damage items first
    switch (item)
    {   
    case DamageItem::BayDoors:
        UpdateDoorDamage(bay_status, bay_proc, fracIntegrity);
        SET_WARNING_LIGHT(wl3Bay);
        break;

    case DamageItem::Elevator:
        UpdateDoorDamage(crewElevator_status, crewElevator_proc, fracIntegrity);
        if (fracIntegrity < 1.0)
            crewElevator_status = DoorStatus::DOOR_FAILED;

        SET_WARNING_LIGHT(wl3Elev);
        break;

    default:
        // let the superclass handle it
        DeltaGliderXR1::SetDamageStatus(item, fracIntegrity);
        return;
    }

    // if any damage present, let's apply it (also calls SetDamageVisuals)
    if (IsDamagePresent())   
    {
        m_MWSActive = true;
        ApplyDamage();
    }
}

// Fail a door if dynamic pressure exceeds limits, or issue a warning if a door is 
// open and dynamic pressure is high enough, if heating == 25% of failure heat level.
// Returns: true if door FAILED, false otherwise
bool XR3Phoenix::CheckDoorFailure(DoorStatus *doorStatus)
{
    bool retVal = false;        // assume no damage

    // check for our new doors first

    // do not re-check or warn if door already failed
    bool doorOpen = ((*doorStatus != DoorStatus::DOOR_CLOSED) && (*doorStatus != DoorStatus::DOOR_FAILED));
    const double dt = oapiGetSimStep();

    if (doorOpen)
    {
        // Door is open!  Check for damage or failure.
        // NOTE: once a door fails, it can only be repaired via the damage dialog; therefore, we never reset it here

        // check NEW doors for the XR3 first
        if (doorStatus == &crewElevator_status)
        {
            const double doorProc = crewElevator_proc;
            if (IS_DOOR_FAILURE(m_noseconeTemp, ELEVATOR_LIMIT))
            {
                ShowWarning("Warning Elevator Failure.wav", DeltaGliderXR1::ST_WarningCallout, "Elevator FAILED due to excessive&heat and/or dynamic pressure!", true); // force this
                *doorStatus = DoorStatus::DOOR_FAILED;
                m_XR3WarningLights[static_cast<int>(XR3WarningLight::wl3Elev)] = true;
                FailDoor(crewElevator_proc, anim_crewElevator);
                retVal = true;   // new damage
            }
            else if (IS_DOOR_WARNING(m_noseconeTemp, ELEVATOR_LIMIT))
            {
                ShowWarning("Warning Elevator Deployed.wav", DeltaGliderXR1::ST_WarningCallout, "Elevator is deployed:&retract it or reduce speed!");
                m_XR3WarningLights[static_cast<int>(XR3WarningLight::wl3Elev)] = true;
            }
            else if (IS_DOOR_FAILED() == false) 
                m_XR3WarningLights[static_cast<int>(XR3WarningLight::wl3Elev)] = false;   // reset light
        }
        else if (doorStatus == &bay_status)
        {
            const double doorProc = bay_proc;
            if (IS_DOOR_FAILURE(m_topHullTemp, BAY_LIMIT))
            {
                ShowWarning("Warning Bay Door Failure.wav", DeltaGliderXR1::ST_WarningCallout, "Bay doors FAILED due to excessive&heat and/or dynamic pressure!", true); // force this
                *doorStatus = DoorStatus::DOOR_FAILED;
                m_XR3WarningLights[static_cast<int>(XR3WarningLight::wl3Bay)] = true;
                retVal = true;   // new damage
            }
            else if (IS_DOOR_WARNING(m_topHullTemp, BAY_LIMIT))
            {
                ShowWarning("Warning Bay Doors Open.wav", DeltaGliderXR1::ST_WarningCallout, "Bay doors are open:&close them or reduce speed!");
                m_XR3WarningLights[static_cast<int>(XR3WarningLight::wl3Bay)] = true;
            }
            else if (IS_DOOR_FAILED() == false) 
                m_XR3WarningLights[static_cast<int>(XR3WarningLight::wl3Bay)] = false;   // reset light
        }
        else    // one of the unmodified doors
            retVal = DeltaGliderXR1::CheckDoorFailure(doorStatus);    // let our superclass handle it
    }
    else   // door is either CLOSED or FAILED
    {
        if (IS_DOOR_FAILED() == false)
        {
            // only need to handle NEW doors here
            // door is closed; reset the warning light
            if (doorStatus == &crewElevator_status)
                m_XR3WarningLights[static_cast<int>(XR3WarningLight::wl3Elev)] = false;
            else if (doorStatus == &bay_status)
                m_XR3WarningLights[static_cast<int>(XR3WarningLight::wl3Bay)] = false;
            else
            {
                // notify our superclass
                retVal = DeltaGliderXR1::CheckDoorFailure(doorStatus);
            }
        }
        else  // door is failed; notify our superclass
            retVal = DeltaGliderXR1::CheckDoorFailure(doorStatus);
    }

    return retVal;
}


// alieron mesh groups
static UINT AileronGrp[4] = {GRP_upper_brake_left, GRP_lower_brake_left, GRP_lower_brake_right, GRP_upper_brake_right};

// elevator mesh groups
static UINT ElevatorGrp[2] = {GRP_elevator_left, GRP_elevator_right};

// invoked at startup and when a crash occurs
void XR3Phoenix::SetDamageVisuals()
{
    if (!exmesh) return;
    
    // ailerons
    for (int i = 0; i < 4; i++) 
        SetMeshGroupVisible(exmesh, AileronGrp[i], !aileronfail[i]);  

    // elevators
    for (int i=0; i < 2; i++)
    {
        // elevator damage follows upper aileron surface damage
        SetMeshGroupVisible(exmesh, ElevatorGrp[i], !aileronfail[i*2]);  
    }
    
    if (hatch_status == DoorStatus::DOOR_FAILED)
        SetXRAnimation(anim_hatch, 0.2);  // show partially deployed
}
