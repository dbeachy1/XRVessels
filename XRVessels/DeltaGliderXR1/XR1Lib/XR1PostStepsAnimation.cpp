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

#include "XR1AnimationPostStep.h"
#include "AreaIDs.h"

//---------------------------------------------------------------------------

AnimationPostStep::AnimationPostStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel)
{
}

void AnimationPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // animate doors that require hydraulic pressure
    if (GetXR1().CheckHydraulicPressure(false, false))     // do not log a warning nor play an error beep here!  We are merely querying the state.
    {
        AnimateLadder      (simt, simdt, mjd);
        AnimateNosecone    (simt, simdt, mjd);
        AnimateOuterAirlock(simt, simdt, mjd);
        AnimateInnerAirlock(simt, simdt, mjd);
        AnimateHatch       (simt, simdt, mjd);
        AnimateRadiator    (simt, simdt, mjd);
        AnimateRetroDoors  (simt, simdt, mjd);
        AnimateHoverDoors  (simt, simdt, mjd);
        AnimateScramDoors  (simt, simdt, mjd);
        AnimateGear        (simt, simdt, mjd);
        AnimateAirbrake    (simt, simdt, mjd);
    }

    // animate doors that do NOT require hydraulic pressure
    ManageChamberPressure(simt, simdt, mjd);
}

void AnimationPostStep::AnimateLadder(const double simt, const double simdt, const double mjd)
{
    // animate escape ladder
    if (GetXR1().ladder_status >= DoorStatus::DOOR_CLOSING)  // closing or opening
    {
        double da = simdt * LADDER_OPERATING_SPEED;
        if (GetXR1().ladder_status == DoorStatus::DOOR_CLOSING)
        {
            if (GetXR1().ladder_proc > 0.0)
                GetXR1().ladder_proc = max(0.0, GetXR1().ladder_proc - da);
            else
            {
                GetXR1().ladder_status = DoorStatus::DOOR_CLOSED;
                GetVessel().TriggerRedrawArea(AID_LADDERINDICATOR);
            }
        } 
        else    // door is opening or open
        {
            if (GetXR1().ladder_proc < 1.0)
                GetXR1().ladder_proc = min (1.0, GetXR1().ladder_proc + da);
            else
            {
                GetXR1().ladder_status = DoorStatus::DOOR_OPEN;
                GetVessel().TriggerRedrawArea(AID_LADDERINDICATOR);
            }
        }
        GetXR1().SetXRAnimation(GetXR1().anim_ladder, GetXR1().ladder_proc);
    }
}

//---------------------------------------------------------------------------

void AnimationPostStep::AnimateNosecone(const double simt, const double simdt, const double mjd)
{
    if (GetXR1().nose_status >= DoorStatus::DOOR_CLOSING)
    {
        double da = simdt * NOSE_OPERATING_SPEED;
        if (GetXR1().nose_status == DoorStatus::DOOR_CLOSING)
        {
            if (GetXR1().nose_proc > 0.0)  // animation
                GetXR1().nose_proc = max (0.0, GetXR1().nose_proc-da);
            else
            {
                GetXR1().nose_status = DoorStatus::DOOR_CLOSED;
                GetVessel().TriggerRedrawArea(AID_NOSECONEINDICATOR);
            }
        } 
        else   // door opening
        {   
            if (GetXR1().nose_proc < 1.0)
                GetXR1().nose_proc = min (1.0, GetXR1().nose_proc+da);
            else 
            {
                GetXR1().nose_status = DoorStatus::DOOR_OPEN;
                GetVessel().TriggerRedrawArea(AID_NOSECONEINDICATOR);
            }
        }
        GetXR1().SetXRAnimation(GetXR1().anim_nose, GetXR1().nose_proc);
    }
}

//---------------------------------------------------------------------------

void AnimationPostStep::AnimateOuterAirlock(const double simt, const double simdt, const double mjd)
{
    if (GetXR1().olock_status >= DoorStatus::DOOR_CLOSING)
    {
        double da = simdt * AIRLOCK_OPERATING_SPEED;
        if (GetXR1().olock_status == DoorStatus::DOOR_CLOSING)
        {
            if (GetXR1().olock_proc > 0.0)
                GetXR1().olock_proc = max (0.0, GetXR1().olock_proc-da);
            else
            {
                GetXR1().olock_status = DoorStatus::DOOR_CLOSED;
                GetVessel().TriggerRedrawArea(AID_OUTERDOORINDICATOR);
            }
        } 
        else   // door opening
        { 
            if (GetXR1().olock_proc < 1.0)
                GetXR1().olock_proc = min (1.0, GetXR1().olock_proc+da);
            else
            {
                GetXR1().olock_status = DoorStatus::DOOR_OPEN;
                GetVessel().TriggerRedrawArea(AID_OUTERDOORINDICATOR);
            }
        }
        GetXR1().SetXRAnimation(GetXR1().anim_olock, GetXR1().olock_proc);
    }
}

//---------------------------------------------------------------------------

void AnimationPostStep::AnimateInnerAirlock(const double simt, const double simdt, const double mjd)
{
    if (GetXR1().ilock_status >= DoorStatus::DOOR_CLOSING)
    {
        double da = simdt * AIRLOCK_OPERATING_SPEED;
        if (GetXR1().ilock_status == DoorStatus::DOOR_CLOSING)
        {
            if (GetXR1().ilock_proc > 0.0)
                GetXR1().ilock_proc = max (0.0, GetXR1().ilock_proc-da);
            else
            {
                GetXR1().ilock_status = DoorStatus::DOOR_CLOSED;
                GetVessel().TriggerRedrawArea(AID_INNERDOORINDICATOR);
            }
        } 
        else  // door opening
        {  
            if (GetXR1().ilock_proc < 1.0)
                GetXR1().ilock_proc = min (1.0, GetXR1().ilock_proc+da);
            else
            {
                GetXR1().ilock_status = DoorStatus::DOOR_OPEN;
                GetVessel().TriggerRedrawArea(AID_INNERDOORINDICATOR);
            }
        }
        GetXR1().SetXRAnimation(GetXR1().anim_ilock, GetXR1().ilock_proc);
    }
}

//---------------------------------------------------------------------------
// NOTE: This is not actually animation; however, it does pressurize / depressurize at a fixed speed like a door and so it handled here
void AnimationPostStep::ManageChamberPressure(const double simt, const double simdt, const double mjd)
{
    if (GetXR1().chamber_status >= DoorStatus::DOOR_CLOSING)
    {
        double da = simdt * CHAMBER_OPERATING_SPEED;
        if (GetXR1().chamber_status == DoorStatus::DOOR_CLOSING)
        {
            if (GetXR1().chamber_proc > 0.0)
                GetXR1().chamber_proc = max (0.0, GetXR1().chamber_proc-da);
            else
            {
                GetXR1().chamber_status = DoorStatus::DOOR_CLOSED;
                GetVessel().TriggerRedrawArea(AID_CHAMBERINDICATOR);
            }
        } 
        else  // door opening
        {  
            if (GetXR1().chamber_proc < 1.0)
                GetXR1().chamber_proc = min (1.0, GetXR1().chamber_proc+da);
            else
            {
                GetXR1().chamber_status = DoorStatus::DOOR_OPEN;
                GetVessel().TriggerRedrawArea(AID_CHAMBERINDICATOR);
            }
        }
        // no animation to set
    }
}

//---------------------------------------------------------------------------

void AnimationPostStep::AnimateHatch(const double simt, const double simdt, const double mjd)
{
    if (GetXR1().hatch_status >= DoorStatus::DOOR_CLOSING)
    {
        double da = simdt * HATCH_OPERATING_SPEED;
        if (GetXR1().hatch_status == DoorStatus::DOOR_CLOSING)
        {
            if (GetXR1().hatch_proc > 0.0)
                GetXR1().hatch_proc = max(0.0, GetXR1().hatch_proc-da);
            else
            {
                GetXR1().hatch_status = DoorStatus::DOOR_CLOSED;
                GetVessel().TriggerRedrawArea(AID_HATCHINDICATOR);
            }
        } 
        else
        {
            if (GetXR1().hatch_proc < 1.0)
                GetXR1().hatch_proc = min(1.0, GetXR1().hatch_proc+da);
            else
            {
                GetXR1().hatch_status = DoorStatus::DOOR_OPEN;
                GetVessel().TriggerRedrawArea(AID_HATCHINDICATOR);
            }
        }
        GetXR1().SetXRAnimation(GetXR1().anim_hatch, GetXR1().hatch_proc);
    }

    if (GetXR1().hatch_vent && simt > GetXR1().hatch_vent_t + 4.0)    // vent for four seconds
    {
        GetXR1().CleanUpHatchDecompression();
        
        // clean up
        delete[] GetXR1().hatch_vent;        
        delete[] GetXR1().hatch_venting_lvl;
        GetXR1().hatch_vent = nullptr;
        GetXR1().hatch_venting_lvl = nullptr;
    }
}

//---------------------------------------------------------------------------

void AnimationPostStep::AnimateRadiator(const double simt, const double simdt, const double mjd)
{
    if (GetXR1().radiator_status >= DoorStatus::DOOR_CLOSING)
    {
        double da = simdt * RADIATOR_OPERATING_SPEED;
        if (GetXR1().radiator_status == DoorStatus::DOOR_CLOSING)
        { 
            // retract radiator
            if (GetXR1().radiator_proc > 0.0) 
            {
                GetXR1().radiator_proc = max (0.0, GetXR1().radiator_proc-da);
            }
            else                     
            {
                GetXR1().radiator_status = DoorStatus::DOOR_CLOSED;
                GetVessel().TriggerRedrawArea(AID_RADIATORINDICATOR);
            }
        } 
        else
        {                               // deploy radiator
            if (GetXR1().radiator_proc < 1.0)
            {
                GetXR1().radiator_proc = min (1.0, GetXR1().radiator_proc+da);
            }
            else
            {
                GetXR1().radiator_status = DoorStatus::DOOR_OPEN;
                GetVessel().TriggerRedrawArea(AID_RADIATORINDICATOR);
            }
        }
        GetXR1().SetXRAnimation(GetXR1().anim_radiator, GetXR1().radiator_proc);
    }
}

//---------------------------------------------------------------------------

void AnimationPostStep::AnimateRetroDoors (const double simt, const double simdt, const double mjd)
{
    if (GetXR1().rcover_status >= DoorStatus::DOOR_CLOSING)
    {
        double da = simdt * RCOVER_OPERATING_SPEED;
        if (GetXR1().rcover_status == DoorStatus::DOOR_CLOSING)
        {
            if (GetXR1().rcover_proc > 0.0)
                GetXR1().rcover_proc = max (0.0, GetXR1().rcover_proc-da);
            else
            {
                GetXR1().rcover_status = DoorStatus::DOOR_CLOSED;
                GetVessel().TriggerRedrawArea(AID_RETRODOORINDICATOR);
            }
        } 
        else
        {
            if (GetXR1().rcover_proc < 1.0)
                GetXR1().rcover_proc = min (1.0, GetXR1().rcover_proc+da);
            else
            {
                GetXR1().rcover_status = DoorStatus::DOOR_OPEN;
                GetXR1().EnableRetroThrusters (true);
                GetVessel().TriggerRedrawArea(AID_RETRODOORINDICATOR);
            }
        }

        GetXR1().SetXRAnimation(GetXR1().anim_rcover, GetXR1().rcover_proc);
    }
}

//---------------------------------------------------------------------------

void AnimationPostStep::AnimateHoverDoors(const double simt, const double simdt, const double mjd)
{
    if (GetXR1().hoverdoor_status >= DoorStatus::DOOR_CLOSING)
    {
        double da = simdt * HOVERDOOR_OPERATING_SPEED;
        if (GetXR1().hoverdoor_status == DoorStatus::DOOR_CLOSING)
        {
            if (GetXR1().hoverdoor_proc > 0.0)
                GetXR1().hoverdoor_proc = max (0.0, GetXR1().hoverdoor_proc-da);
            else
            {
                GetXR1().hoverdoor_status = DoorStatus::DOOR_CLOSED;
                GetVessel().TriggerRedrawArea(AID_HOVERDOORINDICATOR);
            }
        } 
        else
        {
            if (GetXR1().hoverdoor_proc < 1.0)
                GetXR1().hoverdoor_proc = min (1.0, GetXR1().hoverdoor_proc+da);
            else
            {
                GetXR1().hoverdoor_status = DoorStatus::DOOR_OPEN;
                GetXR1().EnableHoverEngines(true);
                GetVessel().TriggerRedrawArea(AID_SCRAMDOORINDICATOR);
            }
        }

        GetXR1().SetXRAnimation(GetXR1().anim_hoverdoor, GetXR1().hoverdoor_proc);
    }
}

//---------------------------------------------------------------------------

void AnimationPostStep::AnimateScramDoors(const double simt, const double simdt, const double mjd)
{
    if (GetXR1().scramdoor_status >= DoorStatus::DOOR_CLOSING)
    {
        double da = simdt * SCRAMDOOR_OPERATING_SPEED;
        if (GetXR1().scramdoor_status == DoorStatus::DOOR_CLOSING)
        {
            if (GetXR1().scramdoor_proc > 0.0)
                GetXR1().scramdoor_proc = max (0.0, GetXR1().scramdoor_proc-da);
            else
            {
                GetXR1().scramdoor_status = DoorStatus::DOOR_CLOSED;
                GetVessel().TriggerRedrawArea(AID_SCRAMDOORINDICATOR);
            }
        } 
        else
        {
            if (GetXR1().scramdoor_proc < 1.0)
                GetXR1().scramdoor_proc = min (1.0, GetXR1().scramdoor_proc+da);
            else
            {
                GetXR1().scramdoor_status = DoorStatus::DOOR_OPEN;
                GetXR1().EnableScramEngines(true);
                GetVessel().TriggerRedrawArea(AID_SCRAMDOORINDICATOR);
            }
        }

        GetXR1().SetXRAnimation(GetXR1().anim_scramdoor, GetXR1().scramdoor_proc);
    }
}

//---------------------------------------------------------------------------

void AnimationPostStep::AnimateGear(const double simt, const double simdt, const double mjd)
{
    if (GetXR1().gear_status >= DoorStatus::DOOR_CLOSING)
    {
        double da = simdt * GEAR_OPERATING_SPEED;
        if (GetXR1().gear_status == DoorStatus::DOOR_CLOSING)
        {
            if (GetXR1().gear_proc > 0.0)
                GetXR1().gear_proc = max (0.0, GetXR1().gear_proc-da);
            else
            {
                GetXR1().gear_status = DoorStatus::DOOR_CLOSED;
                GetVessel().TriggerRedrawArea(AID_GEARINDICATOR);
            }
        } 
        else  // door opening
        { 
            if (GetXR1().gear_proc < 1.0)
                GetXR1().gear_proc = min (1.0, GetXR1().gear_proc+da);
            else
            {
                GetXR1().gear_status = DoorStatus::DOOR_OPEN;
                GetVessel().TriggerRedrawArea(AID_GEARINDICATOR);
            }
        }
        GetXR1().SetGearParameters(GetXR1().gear_proc);  // will set animation state as well
    }
}

//---------------------------------------------------------------------------

void AnimationPostStep::AnimateAirbrake(const double simt, const double simdt, const double mjd)
{
    if (GetXR1().brake_status >= DoorStatus::DOOR_CLOSING)
    {
        double da = simdt * AIRBRAKE_OPERATING_SPEED;
        if (GetXR1().brake_status == DoorStatus::DOOR_CLOSING)  // retract brake
        { 
            if (GetXR1().brake_proc > 0.0) 
                GetXR1().brake_proc = max (0.0, GetXR1().brake_proc-da);
            else                  
                GetXR1().brake_status = DoorStatus::DOOR_CLOSED;
        } 
        else   // deploy brake
        {                            
            if (GetXR1().brake_proc < 1.0) 
                GetXR1().brake_proc = min (1.0, GetXR1().brake_proc+da);
            else                  
                GetXR1().brake_status = DoorStatus::DOOR_OPEN;
        }

        GetXR1().SetXRAnimation(GetXR1().anim_brake, GetXR1().brake_proc);
    }
}
