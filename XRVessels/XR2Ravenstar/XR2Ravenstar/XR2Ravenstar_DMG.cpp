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
// XR2Ravenstar implementation class
//
// XR2Ravenstar_DMG.cpp
// Handles custom XR2 Damage; methods are invoked by the XR1 base class.
// ==============================================================

#include "XR2Ravenstar.h"
#include "XRCommon_DMG.h"
#include "AreaIDs.h"
#include "meshres.h"

// Perform crash damage; i.e., damage all systems.  This is invoked only once when a crash occurs.
void XR2Ravenstar::PerformCrashDamage()
{
    DeltaGliderXR1::PerformCrashDamage();  // handle all the common systems

    // set our custom systems to *crashed*
    bay_status = DOOR_FAILED;

    // blink our new warning light
    m_xr2WarningLights[wl2Bay] = true;
}

//
// Check for door-related heat and/or dynamic pressure damage here
// Note that a given damange type is only checked if it is enabled.
//
// Returns: true if any damage detected, false otherwise
bool XR2Ravenstar::CheckAllDoorDamage()
{
    bool newdamage = DeltaGliderXR1::CheckAllDoorDamage();  // check common systems

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
bool XR2Ravenstar::CheckHullHeatingDamage()
{
    bool newdamage = DeltaGliderXR1::CheckHullHeatingDamage();  // check common systems; includes payload bay doors open check

    // no additional damage checks at this time

    return newdamage;
}

// Note: base class IsDamagePresent() method is sufficient

// Check whether ANY warning is active.  Invoked on startup.
// Returns: true if any warning present, false if no warnings present
bool XR2Ravenstar::IsWarningPresent()
{
    // invoke the superclass
    bool retVal = DeltaGliderXR1::IsWarningPresent();

    if (retVal == false)
    {
        // loop through all new warning lights
        for (int i=0; i < XR2_WARNING_LIGHT_COUNT; i++)
        {
            bool warningLightActive = m_xr2WarningLights[i];
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
const DamageStatus &XR2Ravenstar::GetDamageStatus(DamageItem item) const
{
    double frac;
    const char *pLabel;
    const char *pShortLabel;
    bool onlineOffline = true;     // assume online/offline

    // check for our custom damage items first
    switch (item)
    {
    case BayDoors:
        frac = ((bay_status == DOOR_FAILED) ? 0 : 1);
        pLabel = "Bay Doors";
        pShortLabel = "BDor";
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
void XR2Ravenstar::SetDamageStatus(DamageItem item, double fracIntegrity)
{
// NOTE: because some warning lights can have multiple causes (e.g., left and right engines), we never CLEAR a warning flag here
#define SET_WARNING_LIGHT(wlIdx)  m_xr2WarningLights[wlIdx] |= (fracIntegrity < 1.0)

    // check for our custom damage items first
    switch (item)
    {   
    case BayDoors:
        UpdateDoorDamage(bay_status, bay_proc, fracIntegrity);
        SET_WARNING_LIGHT(wl2Bay);
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
bool XR2Ravenstar::CheckDoorFailure(DoorStatus *doorStatus)
{
    bool retVal = false;        // assume no damage

    // check for our new doors first

    // do not re-check or warn if door already failed
    bool doorOpen = ((*doorStatus != DOOR_CLOSED) && (*doorStatus != DOOR_FAILED));
    const double dt = oapiGetSimStep();

    if (doorOpen)
    {
        // Door is open!  Check for damage or failure.
        // NOTE: once a door fails, it can only be repaired via the damage dialog; therefore, we never reset it here

        // check NEW doors for the XR2 first
        if (doorStatus == &bay_status)
        {
            const double doorProc = bay_proc;
            if (IS_DOOR_FAILURE(m_topHullTemp, BAY_LIMIT))
            {
                ShowWarning("Warning Bay Door Failure.wav", DeltaGliderXR1::ST_WarningCallout, "Bay doors FAILED due to excessive&heat and/or dynamic pressure!", true); // force this
                *doorStatus = DOOR_FAILED;
                m_xr2WarningLights[wl2Bay] = true;
                retVal = true;   // new damage
            }
            else if (IS_DOOR_WARNING(m_topHullTemp, BAY_LIMIT))
            {
                ShowWarning("Warning Bay Doors Open.wav", DeltaGliderXR1::ST_WarningCallout, "Bay doors are open:&close them or reduce speed!");
                m_xr2WarningLights[wl2Bay] = true;
            }
            else if (IS_DOOR_FAILED() == false) 
                m_xr2WarningLights[wl2Bay] = false;   // reset light
        }
        else    // one of the unmodified doors, so let the base class handle it
            retVal = DeltaGliderXR1::CheckDoorFailure(doorStatus);    // let our superclass handle it
    }
    else   // door is either CLOSED or FAILED
    {
        if (IS_DOOR_FAILED() == false)
        {
            // only need to handle NEW doors here
            // door is closed; reset the warning light
            if (doorStatus == &bay_status)
                m_xr2WarningLights[wl2Bay] = false;  
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


// elevon mesh groups
static UINT LAileronGrp[] = {GRP_top_elevators_top01_port, GRP_top_elevators_bottom_port, GRP_bottom_elevators_bottom_port, GRP_bottom_elevators_bottom_port_fixup_1, GRP_bottom_elevators_bottom_port01, 
        GRP_top_elevators_bottom_starboard_fixup_4, /* This is actually *PORT TOP piece */
        GRP_top_elevators_bottom_starboard_fixup_3 /* This is actually the *PORT BOTTOM piece */ };

static UINT RAileronGrp[] = {GRP_top_elevators_top01_starboard, GRP_top_elevators_bottom_starboard, GRP_bottom_elevators_top_starboard, GRP_bottom_elevators_bottom_starboard, GRP_bottom_elevators_bottom_starboard_fixup_1,
                GRP_top_elevators_bottom_starboard_fixup_1, GRP_top_elevators_bottom_starboard_fixup_2 };

// size of a mesh group array
#define SizeOfGrp(grp) (sizeof(grp) / sizeof(UINT))

// invoked at startup and when a crash occurs
// Note: do not call the base class for this method: visuals are vessel-specific
void XR2Ravenstar::SetDamageVisuals()
{
    if (!exmesh) 
        return;
    
    // eleveons
    // order is: left, left, right, right
    for (int i = 0; i < 4; i += 2)   // 0, 2
    {
        if (i < 2)
            SetMeshGroupsVisibility(!aileronfail[i], exmesh, SizeOfGrp(LAileronGrp), LAileronGrp);  // left
        else
            SetMeshGroupsVisibility(!aileronfail[i], exmesh, SizeOfGrp(RAileronGrp), RAileronGrp);  // right
    }

    // top hatch
    if (hatch_status == DOOR_FAILED)
        SetXRAnimation(anim_hatch, 0.2);  // show partially deployed
}
