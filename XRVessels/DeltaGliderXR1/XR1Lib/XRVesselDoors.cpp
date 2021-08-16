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
// XR vessel door state methods
// ==============================================================

#include "DeltaGliderXR1.h"
#include "AreaIDs.h"

// handle instant jumps to open or closed here
#define CHECK_DOOR_JUMP(proc, anim) if (action == DoorStatus::DOOR_OPEN) proc = 1.0;            \
                                    else if (action == DoorStatus::DOOR_CLOSED) proc = 0.0;     \
                                    SetXRAnimation(anim, proc)

void DeltaGliderXR1::ActivateLandingGear(DoorStatus action)
{
    // check for failure
    if (gear_status == DoorStatus::DOOR_FAILED)
    {
        PlayErrorBeep();
        ShowWarning("Warning Gear Failure.wav", ST_WarningCallout, "Landing Gear inoperative due to&excessive heat and/or dynamic&pressure.");
        return;  // cannot move
    }

    // We cannot raise or deploy the landing gear if 1) we are already sitting on the ground OR 2) if
    // the gear is up but we are at or below GEAR_FULLY_UNCOMPRESSED_DISTANCE in altitude.
    const double altitude = GetAltitude(ALTMODE_GROUND);
    if ((action == DoorStatus::DOOR_OPENING) || (action == DoorStatus::DOOR_CLOSING))
    {
        if (GroundContact())   // check #1
        {
            PlayErrorBeep();
            ShowWarning("Gear Locked.wav", ST_WarningCallout, "Ship is landed: cannot raise landing gear.");
            return;
        }
        else if (altitude <= GEAR_FULLY_UNCOMPRESSED_DISTANCE)  // would gear be below the ground?
        {
            if (action == DoorStatus::DOOR_CLOSING)
            {
                PlayErrorBeep();
                ShowWarning("Gear Locked.wav", ST_WarningCallout, "Gear in contact with ground:&cannot raise landing gear.");
                return;
            }
            else if (action == DoorStatus::DOOR_OPENING)
            {
                PlayErrorBeep();
                ShowWarning("Gear Locked.wav", ST_WarningCallout, "Insufficient altitude to lower&the landing gear.");
                return;
            }
        }
    }

    if (CheckHydraulicPressure(true, true) == false)   // show warning if no hydraulic pressure
        return;     // no hydraulic pressure

    bool close = (action == DoorStatus::DOOR_CLOSED || action == DoorStatus::DOOR_CLOSING);
    gear_status = action;

    CHECK_DOOR_JUMP(gear_proc, anim_gear);

    UpdateVCStatusIndicators();
    SetGearParameters(gear_proc);

    TriggerRedrawArea(AID_GEARSWITCH);
    TriggerRedrawArea(AID_GEARINDICATOR);
    SetXRAnimation(anim_gearlever, close ? 0 : 1);
    RecordEvent("GEAR", close ? "UP" : "DOWN");

    // NOTE: sound is handled by GearCalloutsPostStep 
}

void DeltaGliderXR1::ToggleLandingGear()
{
    ActivateLandingGear(((gear_status == DoorStatus::DOOR_CLOSED || gear_status == DoorStatus::DOOR_CLOSING) ? DoorStatus::DOOR_OPENING : DoorStatus::DOOR_CLOSING));
    UpdateCtrlDialog(this);
}

// activate the bay doors 
// NOTE: not used by the XR1; this is here for subclasses only
void DeltaGliderXR1::ActivateBayDoors(DoorStatus action)
{
    // check for failure
    if (bay_status == DoorStatus::DOOR_FAILED)
    {
        PlayErrorBeep();
        ShowWarning("Warning Bay Door Failure.wav", ST_WarningCallout, "Bay doors inoperative due to excessive&heat and/or dynamic pressure.");
        return;  // cannot move
    }

    if (CheckHydraulicPressure(true, true) == false)   // show warning if no hydraulic pressure
        return;     // no hydraulic pressure

    /* OK on the Ravenstar, so we handle this check in the subclasses
    // cannot deploy or retract bay doors if the radiator is in motion
    // NOTE: allow for DoorStatus::DOOR_FAILED here so that a radiator failure does not lock the bay doors
    if ((radiator_status == DoorStatus::DOOR_OPENING) || (radiator_status == DoorStatus::DOOR_CLOSING))
    {
        PlayErrorBeep();
        ShowWarning("Warning Radiator in Motion Bay Doors Are Locked.wav", "Cannot open/close bay doors while&radiator is in motion.");
        return;  // cannot move
    }
    */

    CHECK_DOOR_JUMP(bay_proc, anim_bay);

    bool close = (action == DoorStatus::DOOR_CLOSING) || (action == DoorStatus::DOOR_CLOSED);
    bay_status = action;
    TriggerRedrawArea(AID_BAYDOORSSWITCH);
    TriggerRedrawArea(AID_BAYDOORSINDICATOR);
    UpdateCtrlDialog(this);  // Note: CTRL dialog not used for the XR2
    RecordEvent("BAYDOORS", close ? "CLOSE" : "OPEN");
}

// invoked from key handler
// NOTE: not used by the XR1; this is here for subclasses only
void DeltaGliderXR1::ToggleBayDoors()
{
    ActivateBayDoors(bay_status == DoorStatus::DOOR_CLOSED || bay_status == DoorStatus::DOOR_CLOSING ?
        DoorStatus::DOOR_OPENING : DoorStatus::DOOR_CLOSING);
}

void DeltaGliderXR1::ActivateHoverDoors(DoorStatus action)
{
    // NOTE: Hover doors (presently) cannot fail, so don't check for that here

    if (CheckHydraulicPressure(true, true) == false)   // show warning if no hydraulic pressure
        return;     // no hydraulic pressure

    bool close = (action == DoorStatus::DOOR_CLOSED || action == DoorStatus::DOOR_CLOSING);
    hoverdoor_status = action;

    CHECK_DOOR_JUMP(hoverdoor_proc, anim_hoverdoor);

    // No VC lights for these doors: UpdateVCStatusIndicators();

    EnableHoverEngines(action == DoorStatus::DOOR_OPEN);
    TriggerRedrawArea(AID_HOVERDOORSWITCH);
    TriggerRedrawArea(AID_HOVERDOORINDICATOR);
    // no VC switch for this
    UpdateCtrlDialog(this);
    RecordEvent("HOVERDOORS", close ? "CLOSE" : "OPEN");
}

void DeltaGliderXR1::ActivateScramDoors(DoorStatus action)
{
    // NOTE: SCRAM doors (presently) cannot fail, so don't check for that here

    if (CheckHydraulicPressure(true, true) == false)   // show warning if no hydraulic pressure
        return;     // no hydraulic pressure

    bool close = (action == DoorStatus::DOOR_CLOSED || action == DoorStatus::DOOR_CLOSING);
    scramdoor_status = action;

    CHECK_DOOR_JUMP(scramdoor_proc, anim_scramdoor);

    // No VC lights for these doors: UpdateVCStatusIndicators();

    EnableScramEngines(action == DoorStatus::DOOR_OPEN);
    TriggerRedrawArea(AID_SCRAMDOORSWITCH);
    TriggerRedrawArea(AID_SCRAMDOORINDICATOR);
    // no VC switch for this
    UpdateCtrlDialog(this);
    RecordEvent("SCRAMDOORS", close ? "CLOSE" : "OPEN");
}

void DeltaGliderXR1::ActivateRCover(DoorStatus action)
{
    // check for failure
    if (rcover_status == DoorStatus::DOOR_FAILED)
    {
        PlayErrorBeep();
        ShowWarning("Warning Retro Door Failure.wav", ST_WarningCallout, "Retro Doors inoperative due to excessive&heat and/or dynamic pressure.");
        return;  // cannot move; NOTE: this will also disable the indicator lights, which is exactly what we want!
    }

    if (CheckHydraulicPressure(true, true) == false)   // show warning if no hydraulic pressure
        return;     // no hydraulic pressure

    bool close = (action == DoorStatus::DOOR_CLOSED || action == DoorStatus::DOOR_CLOSING);
    rcover_status = action;

    CHECK_DOOR_JUMP(rcover_proc, anim_rcover);
    /* {DEB} causes door to "jump"
    if (action <= DoorStatus::DoorStatus::DOOR_OPEN) {
        rcover_proc = (action == DoorStatus::DOOR_CLOSED ? 0.0 : 1.0);
        SetXRAnimation (anim_rcover, rcover_proc);
    }
    */
    UpdateVCStatusIndicators();
    EnableRetroThrusters(action == DoorStatus::DOOR_OPEN);
    TriggerRedrawArea(AID_RETRODOORSWITCH);
    TriggerRedrawArea(AID_RETRODOORINDICATOR);
    SetXRAnimation(anim_retroswitch, close ? 0 : 1);
    UpdateCtrlDialog(this);
    RecordEvent("RCOVER", close ? "CLOSE" : "OPEN");
}

void DeltaGliderXR1::ActivateNoseCone(DoorStatus action)
{
    // check for failure
    if (nose_status == DoorStatus::DOOR_FAILED)
    {
        PlayErrorBeep();
        char msg[128];
        sprintf(msg, "%s inoperative due to excessive&heat and/or dynamic pressure.", NOSECONE_LABEL);
        ShowWarning("Warning Nosecone Failure.wav", ST_WarningCallout, msg);
        return;  // cannot move
    }

    // if docked, cannot close nosecone
    if (IsDocked() && ((action == DoorStatus::DOOR_CLOSING) || (action == DoorStatus::DOOR_CLOSED)))
    {
        PlayErrorBeep();
        char msg[128];
        sprintf(msg, "Cannot close %s while&ship is docked!", NOSECONE_LABEL);
        ShowWarning("Warning Ship is Docked.wav", ST_WarningCallout, msg);
        return;
    }

    if (CheckHydraulicPressure(true, true) == false)   // show warning if no hydraulic pressure
        return;     // no hydraulic pressure

    // if outer airlock door is open and nosecone is CLOSING, close the outer airlock door as well
    if (((olock_status == DoorStatus::DOOR_OPEN) || (olock_status == DoorStatus::DOOR_OPENING)) &&
        ((action == DoorStatus::DOOR_CLOSING) || (action == DoorStatus::DOOR_CLOSED)))
    {
        // close the outer airlock door since it is OPEN or OPENING!
        ActivateOuterAirlock(DoorStatus::DOOR_CLOSING);
    }

    bool close = (action == DoorStatus::DOOR_CLOSED || action == DoorStatus::DOOR_CLOSING);
    nose_status = action;

    CHECK_DOOR_JUMP(nose_proc, anim_nose);
    /* {DEB} causes door to "jump"
    if (action <= DoorStatus::DOOR_OPEN) {
        nose_proc = (action == DoorStatus::DOOR_CLOSED ? 0.0 : 1.0);
        SetXRAnimation (anim_nose, nose_proc);
        UpdateVCStatusIndicators();
    }
    */
    UpdateVCStatusIndicators();
    TriggerRedrawArea(AID_NOSECONESWITCH);
    TriggerRedrawArea(AID_NOSECONEINDICATOR);
    SetXRAnimation(anim_nconelever, close ? 0 : 1);

    if (close && ladder_status != DoorStatus::DOOR_CLOSED)
        ActivateLadder(action); // retract ladder before closing the nose cone

    UpdateCtrlDialog(this);
    RecordEvent("NOSECONE", close ? "CLOSE" : "OPEN");
}

// invoked from key handler
void DeltaGliderXR1::ToggleRCover()
{
    ActivateRCover(rcover_status == DoorStatus::DOOR_CLOSED || rcover_status == DoorStatus::DOOR_CLOSING ?
        DoorStatus::DOOR_OPENING : DoorStatus::DOOR_CLOSING);
}

// invoked from key handler
void DeltaGliderXR1::ToggleHoverDoors()
{
    ActivateHoverDoors(hoverdoor_status == DoorStatus::DOOR_CLOSED || hoverdoor_status == DoorStatus::DOOR_CLOSING ?
        DoorStatus::DOOR_OPENING : DoorStatus::DOOR_CLOSING);
}

// invoked from key handler
void DeltaGliderXR1::ToggleScramDoors()
{
    ActivateScramDoors(scramdoor_status == DoorStatus::DOOR_CLOSED || scramdoor_status == DoorStatus::DOOR_CLOSING ?
        DoorStatus::DOOR_OPENING : DoorStatus::DOOR_CLOSING);
}

// invoked from key handler
void DeltaGliderXR1::ToggleNoseCone()
{
    ActivateNoseCone(nose_status == DoorStatus::DOOR_CLOSED || nose_status == DoorStatus::DOOR_CLOSING ?
        DoorStatus::DOOR_OPENING : DoorStatus::DOOR_CLOSING);
}

void DeltaGliderXR1::ActivateHatch(DoorStatus action)
{
    // check for failure
    if (hatch_status == DoorStatus::DOOR_FAILED)
    {
        PlayErrorBeep();
        ShowWarning("Warning Hatch Failure.wav", ST_WarningCallout, "Top Hatch inoperative due to excessive&heat and/or dynamic pressure.");
        return;  // cannot move
    }

    // check for ATM pressure outside
    bool opening = ((action == DoorStatus::DOOR_OPENING) || (action == DoorStatus::DOOR_OPEN));
    if ((InEarthAtm() == false) && opening)
    {
        // check whether safety interlocks are still engaged
        if (m_crewHatchInterlocksDisabled == false)
        {
            PlayErrorBeep();
            ShowWarning("Warning Decompression Danger Hatch is Locked.wav", ST_WarningCallout, "WARNING: Crew Hatch LOCKED");
            return;
        }
    }

    if (CheckHydraulicPressure(true, true) == false)   // show warning if no hydraulic pressure
        return;     // no hydraulic pressure

    // NOTE: cabin decompression is handled by LOXConsumptionPostStep

    ForceActivateCabinHatch(action);
}

// force the cabin hatch and don't do any checks
void DeltaGliderXR1::ForceActivateCabinHatch(DoorStatus action)
{
    hatch_status = action;
    UpdateVCStatusIndicators();

    CHECK_DOOR_JUMP(hatch_proc, anim_hatch);
    /* {DEB} causes door to "jump"
    if (action <= DoorStatus::DOOR_OPEN) {
        hatch_proc = (action == DoorStatus::DOOR_CLOSED ? 0.0 : 1.0);
        SetXRAnimation (anim_hatch, hatch_proc);
    }
    */
    UpdateVCStatusIndicators();
    TriggerRedrawArea(AID_HATCHSWITCH);
    TriggerRedrawArea(AID_HATCHINDICATOR);

    const bool close = (action == DoorStatus::DOOR_CLOSED || action == DoorStatus::DOOR_CLOSING);
    SetXRAnimation(anim_hatchswitch, close ? 0 : 1);
    UpdateCtrlDialog(this);
    RecordEvent("HATCH", close ? "CLOSE" : "OPEN");
}

// decompress the cabin and kill the crew if necessary
void DeltaGliderXR1::DecompressCabin()
{
    // kill the crew if still alive and anyone on board
    char temp[60];
#ifdef MMU
    if ((m_crewState != DEAD) && (GetCrewMembersCount() > 0))
    {
        sprintf(temp, "DECOMPRESSION! CREW IS DEAD!!");
        KillCrew();
    }
    else    // crew either dead or no one on board
#endif
    {
        sprintf(temp, "DECOMPRESSION!");
    }

    ShowWarning(nullptr, DeltaGliderXR1::ST_None, temp);
    strcpy(m_crashMessage, temp);   // show on HUD
    PlaySound(Crash, ST_Other);
    m_cabinO2Level = 0;   // no atm in cabin now
    m_MWSActive = true;
}

void DeltaGliderXR1::ToggleHatch()
{
    ActivateHatch(hatch_status == DoorStatus::DOOR_CLOSED || hatch_status == DoorStatus::DOOR_CLOSING ?
        DoorStatus::DOOR_OPENING : DoorStatus::DOOR_CLOSING);
}

void DeltaGliderXR1::ActivateLadder(DoorStatus action)
{
    // Note: this is never called by subclasses that do not have a nosecone, so there is no need to use NOSECONE_LABEL here.
    bool close = (action == DoorStatus::DOOR_CLOSED || action == DoorStatus::DOOR_CLOSING);

    // don't extend ladder if nose cone is closed
    if (!close && nose_status != DoorStatus::DOOR_OPEN)
    {
        PlayErrorBeep();
        ShowWarning("Warning Nosecone is Closed.wav", ST_WarningCallout, "Cannot deploy ladder while&nosecone is closed!");
        return;
    }

    // if docked, cannot deploy ladder
    if (IsDocked() && ((action == DoorStatus::DOOR_OPENING) || (action == DoorStatus::DOOR_OPEN)))
    {
        PlayErrorBeep();
        ShowWarning("Warning Ship is Docked.wav", ST_WarningCallout, "Cannot deploy ladder while&ship is docked!");
        return;
    }

    if (CheckHydraulicPressure(true, true) == false)   // show warning if no hydraulic pressure
        return;     // no hydraulic pressure

    ladder_status = action;

    CHECK_DOOR_JUMP(ladder_proc, anim_ladder);
    /* {DEB} causes door to "jump"
    if (action <= DoorStatus::DOOR_OPEN) {
        ladder_proc = (action == DoorStatus::DOOR_CLOSED ? 0.0 : 1.0);
        SetXRAnimation (anim_ladder, ladder_proc);
    }
    */
    TriggerRedrawArea(AID_LADDERSWITCH);
    TriggerRedrawArea(AID_LADDERINDICATOR);

    SetXRAnimation(anim_ladderswitch, close ? 0 : 1);
    UpdateCtrlDialog(this);
    RecordEvent("LADDER", close ? "CLOSE" : "OPEN");
}

// Not currently used, but keep it anyway
void DeltaGliderXR1::ToggleLadder()
{
    ActivateLadder(ladder_status == DoorStatus::DOOR_CLOSED || ladder_status == DoorStatus::DOOR_CLOSING ?
        DoorStatus::DOOR_OPENING : DoorStatus::DOOR_CLOSING);
}

void DeltaGliderXR1::ActivateOuterAirlock(DoorStatus action)
{
    // door presently cannot fail, so don't bother to check for it here

    // if the nosecone is not open, cannot open outer airlock door;
    // HOWEVER, we can CLOSE it.
    if (((action != DoorStatus::DOOR_CLOSING) && (action != DoorStatus::DOOR_CLOSED)) && (nose_status != DoorStatus::DOOR_OPEN))
    {
        PlayErrorBeep();
        char msg[128];
        sprintf(msg, "WARNING: %s is closed;&cannot open outer door", NOSECONE_LABEL);
        ShowWarning(WARNING_OUTER_DOOR_IS_LOCKED_WAV, ST_WarningCallout, msg);
        return;
    }

    // verify that pressure changes are not in progress
    if ((chamber_status != DoorStatus::DOOR_CLOSED) && (chamber_status != DoorStatus::DOOR_OPEN))
    {
        PlayErrorBeep();
        ShowWarning(((chamber_status == DoorStatus::DOOR_CLOSING) ?
            "WARNING Chamber Pressurizing Outer Door is Locked.wav" :
            "WARNING Chamber Depressurizing Outer Door is Locked.wav"), ST_WarningCallout,
            "WARNING: Airlock chamber pressure is&in flux; outer door is LOCKED.");
        return;
    }

    // check whether ATM pressure outside matches chamber pressure 
    // NOTE: always allow door to be CLOSED; this can be an issue if we DOCK with vacuum in the chamber and outer doors open
    if ((action != DoorStatus::DOOR_CLOSING) && (action != DoorStatus::DOOR_CLOSED))
    {
        if (chamber_status == DoorStatus::DOOR_OPEN)      // vacuum in chamber?
        {
            if ((InEarthAtm() || IsDocked()) && (m_airlockInterlocksDisabled == false))
            {
                PlayErrorBeep();
                ShowWarning("Warning External Pressure Higher than Chamber Pressure.wav", ST_WarningCallout,
                    "WARNING: External pressure is higher&than chamber pressure;&outer door is LOCKED.");
                return;
            }
        }
        else if (chamber_status == DoorStatus::DOOR_CLOSED)    // ATM in chamber?
        {
            if (((InEarthAtm() == false) && (IsDocked() == false)) && (m_airlockInterlocksDisabled == false))
            {
                PlayErrorBeep();
                ShowWarning("Warning Decompression Danger Outer Door is Locked.wav", ST_WarningCallout,
                    "WARNING: Chamber pressure exceeds&external pressure;&outer door is LOCKED.");
                return;
            }
        }
    }

    if (CheckHydraulicPressure(true, true) == false)   // show warning if no hydraulic pressure
        return;     // no hydraulic pressure

    bool close = (action == DoorStatus::DOOR_CLOSED || action == DoorStatus::DOOR_CLOSING);
    olock_status = action;

    CHECK_DOOR_JUMP(olock_proc, anim_olock);
    /* {DEB} causes door to "jump"
    if (action <= DoorStatus::DOOR_OPEN)
    {
        olock_proc = (action == DoorStatus::DOOR_CLOSED ? 0.0 : 1.0);
        SetXRAnimation (anim_olock, olock_proc);
    }
    */

    // If door opening and atm in chamber, it means that the interlocks were disabled and the door
    // is opening: chamber pressure now matches external pressure!
    if (action == DoorStatus::DOOR_OPENING)
    {
        if (InEarthAtm() || IsDocked())
            ActivateChamber(DoorStatus::DOOR_CLOSED, true);   // force this
        else    // vacuum (or close enough to it)
            ActivateChamber(DoorStatus::DOOR_OPEN, true);     // force this

        TriggerRedrawArea(AID_CHAMBERSWITCH);
        TriggerRedrawArea(AID_CHAMBERINDICATOR);
    }

    UpdateVCStatusIndicators();
    TriggerRedrawArea(AID_OUTERDOORSWITCH);
    TriggerRedrawArea(AID_OUTERDOORINDICATOR);
    SetXRAnimation(anim_olockswitch, close ? 0 : 1);
    UpdateCtrlDialog(this);
    RecordEvent("OLOCK", close ? "CLOSE" : "OPEN");
}

void DeltaGliderXR1::ToggleOuterAirlock()
{
    ActivateOuterAirlock(olock_status == DoorStatus::DOOR_CLOSED || olock_status == DoorStatus::DOOR_CLOSING ?
        DoorStatus::DOOR_OPENING : DoorStatus::DOOR_CLOSING);
}

void DeltaGliderXR1::ActivateInnerAirlock(DoorStatus action)
{
    // inner door presently cannot fail, so don't bother to check for it here

    // verify that chamber is pressurized before opening it; always allow it to CLOSE, however!
    // NOTE: allow airlock interlock override to affect the INNER airlock door, too
    if (((action == DoorStatus::DOOR_OPEN) || (action == DoorStatus::DOOR_OPENING)) &&
        (chamber_status != DoorStatus::DOOR_CLOSED) && (m_airlockInterlocksDisabled == false))    // chamber not fully pressurized?
    {
        PlayErrorBeep();
        ShowWarning("Warning Chamber Not Pressurized.wav", ST_WarningCallout,
            "WARNING: Airlock chamber is&unpressurized; inner door is LOCKED.");
        return;
    }

    if (CheckHydraulicPressure(true, true) == false)   // show warning if no hydraulic pressure
        return;     // no hydraulic pressure

    ForceActivateInnerAirlock(action);
}

// force the inner airlock and don't do any checks
void DeltaGliderXR1::ForceActivateInnerAirlock(DoorStatus action)
{
    bool close = (action == DoorStatus::DOOR_CLOSED || action == DoorStatus::DOOR_CLOSING);
    ilock_status = action;

    CHECK_DOOR_JUMP(ilock_proc, anim_ilock);
    /* {DEB} causes door to "jump"
    if (action <= DoorStatus::DOOR_OPEN) {
        ilock_proc = (action == DoorStatus::DOOR_CLOSED ? 0.0 : 1.0);
        SetXRAnimation (anim_ilock, ilock_proc);
    }
    */

    // If door opening and chamber in vacuum, it means that the interlocks were disabled and the door
    // is opening: chamber is now fully pressurized!
    if (action == DoorStatus::DOOR_OPENING)
    {
        ActivateChamber(DoorStatus::DOOR_CLOSED, true);   // air in chamber (force this)
        TriggerRedrawArea(AID_CHAMBERSWITCH);
        TriggerRedrawArea(AID_CHAMBERINDICATOR);
    }

    UpdateVCStatusIndicators();
    TriggerRedrawArea(AID_INNERDOORSWITCH);
    TriggerRedrawArea(AID_INNERDOORINDICATOR);
    SetXRAnimation(anim_ilockswitch, close ? 0 : 1);
    UpdateCtrlDialog(this);
    RecordEvent("ILOCK", close ? "CLOSE" : "OPEN");
}

void DeltaGliderXR1::ToggleInnerAirlock()
{
    ActivateInnerAirlock(ilock_status == DoorStatus::DOOR_CLOSED || ilock_status == DoorStatus::DOOR_CLOSING ?
        DoorStatus::DOOR_OPENING : DoorStatus::DOOR_CLOSING);
}

// pressurize or depressurize the airlock chamber
// door CLOSED = PRESSURIZED
// door OPEN   = VACUUM
// force: true to skip checks and just do it
void DeltaGliderXR1::ActivateChamber(DoorStatus action, bool force)
{
    if (force == false)
    {
        // verify that the chamber can change states; i.e., both doors are CLOSED
        if (ilock_status != DoorStatus::DOOR_CLOSED)
        {
            ShowWarning("Inner Door is Open.wav", ST_WarningCallout, "Inner airlock door is open.");
            return;
        }

        if (olock_status != DoorStatus::DOOR_CLOSED)
        {
            ShowWarning("Outer Door is Open.wav", ST_WarningCallout, "Outer airlock door is open.");
            return;
        }

        // chamber presently cannot fail, so don't bother to check for it here
    }

    bool close = (action == DoorStatus::DOOR_CLOSED || action == DoorStatus::DOOR_CLOSING);
    chamber_status = action;
    if (action == DoorStatus::DOOR_CLOSED)
        chamber_proc = 0.0;
    else if (action == DoorStatus::DOOR_OPEN)
        chamber_proc = 1.0;

    // no VC status indicator for this
    TriggerRedrawArea(AID_CHAMBERSWITCH);
    TriggerRedrawArea(AID_CHAMBERINDICATOR);
    // TODO: ANIMATE VC SWITCH (need mesh change from Donamy): SetXRAnimation(anim_chamberswitch, close ? 0:1);
    UpdateCtrlDialog(this);
    RecordEvent("CHAMBER", close ? "CLOSE" : "OPEN");
}

void DeltaGliderXR1::ActivateAirbrake(DoorStatus action)
{
    if (brake_status == DoorStatus::DOOR_FAILED)
    {
        PlayErrorBeep();
        // TODO: if a new speech pack is created, create a "Warning: airbrake failure" callout.
        // no speech callout for this since none was scripted
        ShowWarning(nullptr, ST_None, "Airbrake inoperative due to aileron&failure.");
        return;  // cannot move
    }

    if (CheckHydraulicPressure(true, true) == false)   // show warning if no hydraulic pressure
        return;     // no hydraulic pressure

    brake_status = action;
    RecordEvent("AIRBRAKE", action == DoorStatus::DOOR_CLOSING ? "CLOSE" : "OPEN");

    CHECK_DOOR_JUMP(brake_proc, anim_brake);
    TriggerRedrawArea(AID_AIRBRAKESWITCH);
    TriggerRedrawArea(AID_AIRBRAKEINDICATOR);
}

void DeltaGliderXR1::ToggleAirbrake(void)
{
    ActivateAirbrake(brake_status == DoorStatus::DOOR_CLOSED || brake_status == DoorStatus::DOOR_CLOSING ?
        DoorStatus::DOOR_OPENING : DoorStatus::DOOR_CLOSING);
}

void DeltaGliderXR1::ActivateRadiator(DoorStatus action)
{
    // check for failure
    if (radiator_status == DoorStatus::DOOR_FAILED)
    {
        PlayErrorBeep();
        ShowWarning("Warning Radiator Failure.wav", ST_WarningCallout, "Radiator inoperative due to excessive&heat and/or dynamic pressure.");
        return;  // cannot move
    }

    if (CheckHydraulicPressure(true, true) == false)   // show warning if no hydraulic pressure
        return;     // no hydraulic pressure

    bool close = (action == DoorStatus::DOOR_CLOSED || action == DoorStatus::DOOR_CLOSING);
    radiator_status = action;

    CHECK_DOOR_JUMP(radiator_proc, anim_radiator);

    /* {DEB} causes door to "jump"
    if (action <= DoorStatus::DOOR_OPEN) {
        radiator_proc = (action == DoorStatus::DOOR_CLOSED ? 0.0 : 1.0);
        SetXRAnimation (anim_radiator, radiator_proc);
        UpdateVCStatusIndicators();
    }
    */
    UpdateVCStatusIndicators();
    TriggerRedrawArea(AID_RADIATORSWITCH);
    TriggerRedrawArea(AID_RADIATORINDICATOR);
    SetXRAnimation(anim_radiatorswitch, close ? 0 : 1);
    UpdateCtrlDialog(this);
    RecordEvent("RADIATOR", close ? "CLOSE" : "OPEN");
}

void DeltaGliderXR1::ToggleRadiator(void)
{
    ActivateRadiator(radiator_status == DoorStatus::DOOR_CLOSED || radiator_status == DoorStatus::DOOR_CLOSING ?
        DoorStatus::DOOR_OPENING : DoorStatus::DOOR_CLOSING);
}

void DeltaGliderXR1::ActivateAPU(DoorStatus action)
{
    // if crew incapacitated, cannot activate APU
    if (IsCrewIncapacitatedOrNoPilotOnBoard())
        return;  // cannot activate

    // TODO: add code to fail this or take out the failure check below
    // check for failure
    if (apu_status == DoorStatus::DOOR_FAILED)
    {
        PlayErrorBeep();
        ShowWarning("Warning Aux Power Unit Failure.wav", ST_WarningCallout, "APU FAILED.");
        return;  // cannot activate
    }

    // check fuel level
    if ((m_apuFuelQty <= 0.0) && ((action == DoorStatus::DOOR_OPEN) || (action == DoorStatus::DOOR_OPENING)))
    {
        PlayErrorBeep();
        ShowWarning("Warning APU Fuel Depleted No Hydraulic Pressure.wav", ST_WarningCallout, "APU fuel depleted:&NO HYDRAULIC PRESSURE!");
        return;     // cannot activate
    }

    // update APU inactive timestamp for ALL actions (OK to reset even if door closing)
    MarkAPUActive();  // reset the APU idle warning callout time

    apu_status = action;
    RecordEvent("APU", ((action == DoorStatus::DOOR_CLOSING) || (action == DoorStatus::DOOR_CLOSED)) ? "CLOSE" : "OPEN");

    TriggerRedrawArea(AID_APU_BUTTON);
}

void DeltaGliderXR1::ToggleAPU(void)
{
    ActivateAPU((apu_status == DoorStatus::DOOR_CLOSED || apu_status == DoorStatus::DOOR_CLOSING) ? DoorStatus::DOOR_OPENING : DoorStatus::DOOR_CLOSING);
}
