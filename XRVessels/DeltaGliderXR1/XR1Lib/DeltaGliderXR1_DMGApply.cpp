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
// Applies damage to XR vessels; e.g,. when loading a scenario
// ==============================================================

#include "DeltaGliderXR1.h"
#include "AreaIDs.h"
#include "XRCommon_DMG.h"

// Perform crash damage; i.e., damage all systems.  This is invoked only once when a crash occurs.
void DeltaGliderXR1::PerformCrashDamage()
{
    // WARNING: do not set m_cabinO2Level = 0 here: it will trigger a "crew dead due to hypoxia" message on the HUD

    // turn on ALL warning lights
    for (int i=0; i < WARNING_LIGHT_COUNT; i++)
        m_warningLights[i] = true;

    // disable any autopilots
    m_customAutopilotMode = AUTOPILOT::AP_OFF;
    for (int i=0; i <=7; i++)
        DeactivateNavmode(i);

    // disable ATC and cabin airflow sounds from now on
    XRSoundOnOff(XRSound::RadioATCGroup, false);  
    XRSoundOnOff(XRSound::AirConditioning, false);

    // fail gear
    FailGear(true);

    // fail left wing
    if (lwingstatus == 1.0)     // not already damaged?
        lwingstatus = oapiRand() * 0.5;

    // fail right wing
    if (rwingstatus == 1.0)     // not already damaged?
        rwingstatus = oapiRand() * 0.5;

    // fail all ailerons 
    aileronfail[0] = aileronfail[1] = true;
    aileronfail[2] = aileronfail[3] = true;
    FailAileronsIfDamaged();

    // Deactivate doors
    hoverdoor_status = nose_status = hatch_status = radiator_status = brake_status = rcover_status = brake_status = DoorStatus::DOOR_FAILED;

    //
    // deactivate engines 
    //

    // kill all the engines
    for (int i = 0; i < 2; i++)
    {
        SetThrusterLevel(th_scram[i], 0);
        scram_intensity[i] = 0;

        SetThrusterLevel(th_hover[i], 0);
        SetThrusterLevel(th_main[i], 0);
        SetThrusterLevel(th_retro[i], 0);
    }

    // NOTE: do not delete thrusters here!  Orbiter doesn't like it.
    // So we'll set max thrust to zero and empty the fuel tanks instead.
    for (int i=0; i < 2; i++)
    {
        SetThrusterMax0(th_main[i], 0);
        SetThrusterMax0(th_retro[i], 0);                
        SetHoverThrusterMaxAndIntegrity(i, 0);
    }

    // zero fuel in the bay tanks as well
    SetXRPropellantMass(ph_main, 0);
    SetXRPropellantMass(ph_rcs, 0);
    SetXRPropellantMass(ph_scram, 0);

    // same for RCS jets
    for (int i=0; i < 14; i++)
        SetRCSThrusterMaxAndIntegrity(i, 0);

    // we have to disable the SCRAM engines manually
    ramjet->SetEngineIntegrity(0, 0);
    ramjet->SetEngineIntegrity(1, 0);

    // fuel lights will come on automatically via the fuel PostStep 

    // fail all remaining control surfaces
    ClearControlSurfaceDefinitions();

    // kill the APU
    apu_status = DoorStatus::DOOR_FAILED;   // this will deactivate all doors as well
    m_apuWarning = true;
    m_apuFuelQty = 0;
    StopSound(APU);
}

// This method will reset (repair) all damaged systems and clear all warning lights. Note that it is not invoked by 
// XR systems -- it is only used by XRVesselCtrl calls.
// This should normally NOT need to be overridden by vessel subclasses.
void DeltaGliderXR1::ResetDamageStatus()
{
    // first, clear all damage states
    for (int i=0; i <= static_cast<int>(D_END); i++)
        SetDamageStatus((DamageItem)i, 1.0);  // this will not reset warning lights, however

    // second, clear all warning lights
    for (int i=0; i < WARNING_LIGHT_COUNT; i++)
        m_warningLights[i] = false;

    // third, reset (recreate) any damaged control surfaces (i.e., surfaces with a 0 handle)
    ReinitializeDamageableControlSurfaces();

    // finally, restore any possibly-deleted mesh items
    SetDamageVisuals();
}

// Sets system damage based on an integrity value; invoked at load time AND from XRVesselCtrl whenever a damageable item is altered.
// Note that this is not called at runtime from the ship because the code merely needs to set the system settings
// (max engine thrust, etc.) to create damage.  In fact, that is what we do in this method.
// Also note that this method never *clear* warning lights, so you should first invoke ResetDamageStatus before invoking this
// method in a loop to set the status of each DamageItem.
void DeltaGliderXR1::SetDamageStatus(DamageItem item, double fracIntegrity)
{
// NOTE: because some warning lights can have multiple causes (e.g., left and right engines), we never CLEAR a warning flag here
#define SET_WARNING_LIGHT(wlIdx)  m_warningLights[static_cast<int>(wlIdx)] |= (fracIntegrity < 1.0)
#define UPDATE_DOOR_DMG(name)  UpdateDoorDamage(name##_status, name##_proc, fracIntegrity)

    switch (item)
    {   
    case DamageItem::LeftWing:
        lwingstatus = fracIntegrity;
        SET_WARNING_LIGHT(WarningLight::wlLwng);
        break;

    case DamageItem::RightWing:
        rwingstatus = fracIntegrity;
        SET_WARNING_LIGHT(WarningLight::wlRwng);
        break;

    case DamageItem::LeftAileron:
        aileronfail[0] = aileronfail[1] = ((fracIntegrity < 1.0) ? true : false);  // affect both aileron mesh groups here
        // NOTE: the control surfaces are failed later in our main damange method
        SET_WARNING_LIGHT(WarningLight::wlLail);
        break;

    case DamageItem::RightAileron:
        aileronfail[2] = aileronfail[3] = ((fracIntegrity < 1.0) ? true : false);  // affect both aileron mesh groups here
        // NOTE: the control surfaces are failed later in our main damange method
        SET_WARNING_LIGHT(WarningLight::wlRail);
        break;

    case DamageItem::LandingGear:
        UPDATE_DOOR_DMG(gear);
        SET_WARNING_LIGHT(WarningLight::wlGear);
        break;

    case DamageItem::Nosecone:
        UPDATE_DOOR_DMG(nose);
        SET_WARNING_LIGHT(WarningLight::wlNose);
        break;

    case DamageItem::RetroDoors:
        UPDATE_DOOR_DMG(rcover);
        SET_WARNING_LIGHT(WarningLight::wlRdor);
        break;

    case DamageItem::Hatch:
        UPDATE_DOOR_DMG(hatch);
        SET_WARNING_LIGHT(WarningLight::wlHtch);
        break;

    case DamageItem::Radiator:
        UPDATE_DOOR_DMG(radiator);
        SET_WARNING_LIGHT(WarningLight::wlRad);
        break;

    case DamageItem::Airbrake:
        UPDATE_DOOR_DMG(brake);
        SET_WARNING_LIGHT(WarningLight::wlAirb);
        break;

    case DamageItem::MainEngineLeft:
        SetThrusterMax0(th_main[0], MAX_MAIN_THRUST[GetXR1Config()->MainEngineThrust] * fracIntegrity);
        SET_WARNING_LIGHT(WarningLight::wlMain);
        break;

    case DamageItem::MainEngineRight:
        SetThrusterMax0(th_main[1], MAX_MAIN_THRUST[GetXR1Config()->MainEngineThrust] * fracIntegrity);
        SET_WARNING_LIGHT(WarningLight::wlMain);
        break;

    case DamageItem::SCRAMEngineLeft:
        ramjet->SetEngineIntegrity(0, fracIntegrity);
        SET_WARNING_LIGHT(WarningLight::wlScrm);
        break;

    case DamageItem::SCRAMEngineRight:
        ramjet->SetEngineIntegrity(1, fracIntegrity);
        SET_WARNING_LIGHT(WarningLight::wlScrm);
        break;

    case DamageItem::HoverEngineFore:
        SetHoverThrusterMaxAndIntegrity(0, fracIntegrity);
        SET_WARNING_LIGHT(WarningLight::wlHovr);
        break;

    case DamageItem::HoverEngineAft:
        SetHoverThrusterMaxAndIntegrity(1, fracIntegrity);
        SET_WARNING_LIGHT(WarningLight::wlHovr);
        break;

    case DamageItem::RetroEngineLeft:
        SetThrusterMax0(th_retro[0], MAX_RETRO_THRUST * fracIntegrity);
        SET_WARNING_LIGHT(WarningLight::wlRtro);
        break;

    case DamageItem::RetroEngineRight:
        SetThrusterMax0(th_retro[1], MAX_RETRO_THRUST * fracIntegrity);
        SET_WARNING_LIGHT(WarningLight::wlRtro);
        break;

    case DamageItem::RCS1:
    case DamageItem::RCS2:
    case DamageItem::RCS3:
    case DamageItem::RCS4:
    case DamageItem::RCS5:
    case DamageItem::RCS6:
    case DamageItem::RCS7:
    case DamageItem::RCS8:
    case DamageItem::RCS9:
    case DamageItem::RCS10:
    case DamageItem::RCS11:
    case DamageItem::RCS12:
    case DamageItem::RCS13:
    case DamageItem::RCS14:
        {
            int index = static_cast<int>(item) - static_cast<int>(DamageItem::RCS1);    // 0-13
            SetRCSThrusterMaxAndIntegrity(index, fracIntegrity);
            SET_WARNING_LIGHT(WarningLight::wlRcs);
        }
        break;

    default:        // should never happen!
        sprintf(oapiDebugString(), "WARNING: invalid damage ID in scenario file: %d", item);
        break;
    }

    // if any damage present, let's apply it (also calls SetDamageVisuals)
    if (IsDamagePresent())   
    {
        m_MWSActive = true;
        ApplyDamage();
        //UpdateDamageDialog (this);
    }
}

// Update a given door's state based on its integrity and proc (percentage-open state)
// Note: this method does not update the corresponding warning light state
void DeltaGliderXR1::UpdateDoorDamage(DoorStatus &doorStatus, double &doorProc, const double fracIntegrity)
{
    if (fracIntegrity < 1.0)
        doorStatus = DoorStatus::DOOR_FAILED;
    else 
    {
        // door is OK, so let's see if we are restoring this door from an OFFLINE state
        if (doorStatus == DoorStatus::DOOR_FAILED)
        {
            // door just came back online
            if (doorProc == 0.0)
                doorStatus = DoorStatus::DOOR_CLOSED;
            else if (doorProc == 1.0)
                doorStatus = DoorStatus::DOOR_OPEN;
            else  // door was halfway open or closed, but we have no way of knowing which...
            {
                doorStatus = DoorStatus::DOOR_CLOSING;   // ...so let's mark it 'closing'
            }
        }
    }
}

// Update the vessel performance (e.g., wing lift) to reflect any damage
// Note: items like RCS/engine thrust is computed internally, and should not be applied here.
void DeltaGliderXR1::ApplyDamage()
{
    // if crashed, use balance previously set by DoCrash()
    m_wingBalance = (IsCrashed() ? m_damagedWingBalance : ((rwingstatus-lwingstatus) * CRASH_WING_BALANCE_MULTIPLIER));
    const double minWingAreaPct = 0.2222;   // if crashed, will be 22.2% lift
    // total wing status is from 0...2, so we have to divide the wingAreaPct by 2 as well
    double wingArea = ((rwingstatus+lwingstatus) * (WING_AREA * ((1.0 - minWingAreaPct) / 2))) + (WING_AREA * minWingAreaPct);  // square meters 
    
    EditAirfoil(hwing, 0x09, _V(m_wingBalance, 0, m_centerOfLift), 0, 0, wingArea, 0);  // reset wing area and attack point

    if (rwingstatus < 1.0 || lwingstatus < 1.0) 
        m_MWSActive = true;

    SetDamageVisuals();
}

static UINT AileronGrp[8] = {29,51,30,52,35,55,36,54};

// Note: do not call this base class method from sub: visuals are vessel-specific
void DeltaGliderXR1::SetDamageVisuals()
{
    if (!exmesh) return;
    
    int i, j;
    
    // ailerons
    for (i = 0; i < 4; i++) 
    {
        for (j = 0; j < 2; j++)
            SetMeshGroupVisible(exmesh, AileronGrp[i*2+j], !aileronfail[i]);  // hide or show aileron 
    }
    
    // top hatch
    if (hatch_status == DoorStatus::DOOR_FAILED)
        SetXRAnimation(anim_hatch, 0.2);  // show partially deployed
}

// Ship crashed!
// pMsg = crash reason
// touchdownVerticalSpeed : if > 0, will be appended to the crash message.  NOTE: this should never be negative unless you want to ALWAYS kill the crew on impact!
void DeltaGliderXR1::DoCrash(const char *pMsg, double touchdownVerticalSpeed)
{
    // Note: allow crash during playback
    if (!GetXR1Config()->CrashDamageEnabled || !AllowDamageIfDockedCheck() || m_isCrashed)
        return;

    // msg will be blinked on the HUD by TakeoffAndLandingCalloutPostStep
    
    sprintf(m_crashMessage, "%s", pMsg);

    if (touchdownVerticalSpeed > 0)
    {
        char temp[128];
        sprintf(temp, "&Vertical Impact Velocity = %.2f m/s", touchdownVerticalSpeed);
        strcat(m_crashMessage, temp);
    }

    if (GetCrewMembersCount() > 0)   // anyone on board?
    {
        // at least one crew member is on board
        // NOTE: it is possible that the pilot did a soft belly landing, so we check that here
        // NOTE #2: it is possible to have a NEGATIVE touchdownVerticalSpeed here at this point if landing under light gravity conditions; this is
        // due to the Orbiter "bounce bug", where it bounces the ship.
        // NOTE #3: if crew is already dead, always display this message.  
        // Also note that if the crew is dead GetCrewMembersCount() returns 0, so be careful when modifying this section
        if ((touchdownVerticalSpeed <= 0) || (touchdownVerticalSpeed > CREW_IMPACT_DEATH_THRESHOLD) || (m_crewState == CrewState::DEAD))
        {
            strcat(m_crashMessage, "&You and the crew are DEAD!");
            KillCrew();
        }
        else   // the crew is NOT dead
        {
            if (touchdownVerticalSpeed > CREW_IMPACT_SEVERE_INJURY_THRESHOLD)
            {
                strcat(m_crashMessage, "&You and the crew&sustained SEVERE INJURIES,&but you survived!");
                m_crewState = CrewState::INCAPACITATED;
            }
            else if (touchdownVerticalSpeed > CREW_IMPACT_MODERATE_INJURY_THRESHOLD)
                strcat(m_crashMessage, "&You and the crew&sustained MODERATE INJURIES,&but you survived!");
            else if (touchdownVerticalSpeed > CREW_IMPACT_MINOR_INJURY_THRESHOLD)
                strcat(m_crashMessage, "&You and the crew&sustained MINOR INJURIES.");
            else    // light impact
            {
                strcat(m_crashMessage, "&You and the crew are UNINJURED.");

                // NOTE: cannot check for gear failed here, because gear fails on crash if wheels up
            }
        }
    }

    // play crash sound separately so it always plays immediately
    PlaySound(Crash, ST_Other);
    ShowWarning(nullptr, DeltaGliderXR1::ST_None, m_crashMessage, true);  // OK force this message because DoCrash() is only called once

    // set random new wing balance to make ship spiral
    m_damagedWingBalance = (oapiRand() * 6.0) + 3.0;  // was 8.0, but induced excessive spins sometime

    // now set left vs. right
    if (oapiRand() < 0.5)
        m_damagedWingBalance = -m_damagedWingBalance;

    // damage will be applied by the TestDemage routine since IsCrashed() == true now

    // must set this LAST so our last CRASHED warning gets through
    m_isCrashed = true;
}

// Ship crashed or had a hard landing!
// pMsg may be null; if null, default GEAR COLLAPSED message is used.
// setGearAnimState: true = set gear to random deployment, 0-0.25 or so.  false = don't change the gear's state
void DeltaGliderXR1::DoGearCollapse(const char *pMsg, double touchdownVerticalSpeed, bool setGearAnimState)
{
    // allow gear collapse during playback
    if (!GetXR1Config()->HardLandingsDamageEnabled || !AllowDamageIfDockedCheck())
        return;

    // play the gear collapse sound
    char temp[MAX_MESSAGE_LENGTH];
    if (pMsg == nullptr)
    {
        if (touchdownVerticalSpeed > CREW_IMPACT_MINOR_INJURY_THRESHOLD)
            sprintf(temp, "GEAR COLLAPSED!  Impact=%.3f m/s&You and the crew&sustained MINOR INJURIES.", touchdownVerticalSpeed);
        else    // light impact
            sprintf(temp, "GEAR COLLAPSED!  Impact=%.3f m/s&You and the crew&are UNINJURED.", touchdownVerticalSpeed);

        pMsg = temp;
    }
    
    ShowWarning("Gear Collapse.wav", ST_Other, pMsg, true);  // OK to force this becase we only do it once

    // show gear collapsed
    FailGear(setGearAnimState);

    // NOTE: do NOT delete the thruster group here; Orbiter does not like it!
    // hover thrusters damanged to random percentage of power
    for (int i=0; i < 2; i++)
    {
        // reduce the power somewhat
        double currentIntegrity = m_hoverEngineIntegrity[i];
        double frac = (oapiRand() + 0.20);  // thruster is still at least 20% functional
        if (frac > 0.89)
            frac = 0.89;  // hard cap
        double newIntegrity = currentIntegrity * frac;  // reduce max power
        SetHoverThrusterMaxAndIntegrity(i, newIntegrity);
    }

    // kill the hover engine thrust since we damaged them
    for (int i = 0; i < 2; i++)
        SetThrusterLevel(th_hover[i], 0);
}

// fail a door
// doorProc = nose_proc, ladder_proc, etc.
// anim = anim_gear, anim_rcover, etc.
void DeltaGliderXR1::FailDoor(double &doorProc, UINT anim)
{
    doorProc = fmod(oapiRand(), 0.3) + 0.2;     // damage range is 0.2 - 0.5
    SetXRAnimation(anim, doorProc);
}

// Set gear as FAILED
// setGearAnimState: true = show gear partially deployed if crashed into ground, false = don't change gear animation state
void DeltaGliderXR1::FailGear(bool setGearAnimState)
{
    if (setGearAnimState)
    {
        // fully compress the gear (only applies to subclasses)
        SetXRAnimation(m_animNoseGearCompression, 0);
        SetXRAnimation(m_animRearGearCompression, 0);

        // if ship CRASHED into the ground, show gear as partially deployed
        if (GetAltitude(ALTMODE_GROUND) < 100)  // close to planet's surface?
            FailDoor(gear_proc, anim_gear);
    }

    SetGearParameters(gear_proc);       // sets friction coeff and nosewheel steering
    SetMaxWheelbrakeForce(0);           // brakes disabled
    m_MWSActive = true;
    gear_status = DoorStatus::DOOR_FAILED;
    m_warningLights[static_cast<int>(WarningLight::wlGear)] = true;
}

//
// Perform SetThrusterMax0 for a hover engine.  This method is necessary because we must track hover engine
// damage separately via m_hoverEngineIntegrity: the hover engine max thrust is set when the engines are gimbaled,
// and so we cannot rely on that to track engine damage.
// 
// engineIndex: 0=fore, 1=aft
// integrityFrac: 0...1
//
void DeltaGliderXR1::SetHoverThrusterMaxAndIntegrity(int engineIndex, double integrityFrac)
{
    const double maxThrustInKN = MAX_HOVER_THRUST[GetXR1Config()->HoverEngineThrust] * integrityFrac;

    SetThrusterMax0(th_hover[engineIndex], maxThrustInKN);
    m_hoverEngineIntegrity[engineIndex] = integrityFrac;   // update internal damage status too
}

void DeltaGliderXR1::SetRCSThrusterMaxAndIntegrity(int index, double integrityFrac)
{
    double maxThrust = GetRCSThrustMax(index);
    SetThrusterMax0(th_rcs[index], maxThrust * integrityFrac);
    m_rcsIntegrityArray[index] = integrityFrac;     // set internal damage array as well
}

// Fail the ailerons/elevons/elevator trim if damaged
void DeltaGliderXR1::FailAileronsIfDamaged()
{
    // delete the aileron and elevator control surfaces if there are marked as damaged
    // For the purposes of XR vessels, anytime an aileron is damaged the elevator is damaged as well,
    // regardless of whether they are the same control surface.
    if (aileronfail[0] || aileronfail[1])
    {
        // fail left aileron
        if (hLeftAileron)
        {
            DelControlSurface (hLeftAileron);
            hLeftAileron = 0;
        }
    }

    if (aileronfail[2] || aileronfail[3])
    {
        // fail right aileron
        if (hRightAileron)
        {
            DelControlSurface (hRightAileron);
            hRightAileron = 0;
        }
    }

    // if any aileron damaged, fail the elevators and elevator trim as well
    if (!AreElevatorsOperational())
    {
        if (hElevator)  // not already failed
        {
            DelControlSurface(hElevator);
            hElevator = 0;
        }
        
        if (hElevatorTrim)
        {
            DelControlSurface(hElevatorTrim);
            hElevatorTrim = 0;
        }
    }
}
