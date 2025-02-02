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
// Handles XR vessel hatch logic
// ==============================================================

#include "DeltaGliderXR1.h"
#include "AreaIDs.h"  

// Close the fuel hatch and notify subordinate areas to re-render themselves; no warning or info is logged
// playSound = true to play hatch sound
void DeltaGliderXR1::CloseFuelHatch(bool playSound)
{
    fuelhatch_status = DoorStatus::DOOR_CLOSED;

    /* DO NOT reset line pressures here: the PostStep will drop them to zero gradually
    // reset line pressures
    m_mainExtLinePressure = 0;
    m_scramExtLinePressure = 0;
    m_apuExtLinePressure = 0;
    */

    // reset 'pressure nominal' LED states 
    m_mainSupplyLineStatus = false;
    m_scramSupplyLineStatus = false;
    m_apuSupplyLineStatus = false;


    // reset fuel flow switches
    m_mainFuelFlowSwitch = false;
    m_scramFuelFlowSwitch = false;
    m_apuFuelFlowSwitch = false;

    if (playSound)
        PlaySound(SupplyHatch, ST_Other, SUPPLY_HATCH_VOL);

    // update animation
    SetXRAnimation(anim_fuelhatch, 0);  // closed

    TriggerRedrawArea(AID_FUELHATCHSWITCH);
    TriggerRedrawArea(AID_FUELHATCHLED);

    TriggerRedrawArea(AID_MAINSUPPLYLINE_SWITCH);
    TriggerRedrawArea(AID_MAINSUPPLYLINE_SWITCH_LED);

    TriggerRedrawArea(AID_SCRAMSUPPLYLINE_SWITCH);
    TriggerRedrawArea(AID_SCRAMSUPPLYLINE_SWITCH_LED);

    TriggerRedrawArea(AID_APUSUPPLYLINE_SWITCH);
    TriggerRedrawArea(AID_APUSUPPLYLINE_SWITCH_LED);
}

// Close the lox hatch and notify subordinate areas to re-render themselves; no warning or info is logged
// playSound = true to play hatch thump
void DeltaGliderXR1::CloseLoxHatch(bool playSound)
{
    loxhatch_status = DoorStatus::DOOR_CLOSED;
    /* DO NOT reset line pressure here: the PostStep will drop them to zero gradually
    m_loxExtLinePressure = 0;
    */

    m_loxSupplyLineStatus = false;
    m_loxFlowSwitch = false;

    if (playSound)
        PlaySound(SupplyHatch, ST_Other, SUPPLY_HATCH_VOL);

    // update animation
    SetXRAnimation(anim_loxhatch, 0);  // closed; (close always works)

    TriggerRedrawArea(AID_LOXHATCHSWITCH);
    TriggerRedrawArea(AID_LOXHATCHLED);

    TriggerRedrawArea(AID_LOXSUPPLYLINE_SWITCH);
    TriggerRedrawArea(AID_LOXSUPPLYLINE_SWITCH_LED);
}

// Close the external cooling hatch and notify subordinate areas to re-render themselves; no warning or info is logged
// playSound = true to play hatch thump
void DeltaGliderXR1::CloseExternalCoolingHatch(bool playSound)
{
    externalcooling_status = DoorStatus::DOOR_CLOSED;

    // reset external coolant switch
    m_externalCoolingSwitch = false;

    if (playSound)
        PlaySound(SupplyHatch, ST_Other, SUPPLY_HATCH_VOL);

    TriggerRedrawArea(AID_EXTERNAL_COOLING_SWITCH);
    TriggerRedrawArea(AID_EXTERNAL_COOLING_LED);
}

// Render hatch decompression exhaust stream
void DeltaGliderXR1::ShowHatchDecompression()
{
    static PARTICLESTREAMSPEC airvent = {
        0, 1.0, 15, 0.5, 0.3, 2, 0.3, 1.0, PARTICLESTREAMSPEC::EMISSIVE,
        PARTICLESTREAMSPEC::LVL_LIN, 0.1, 0.1,
        PARTICLESTREAMSPEC::ATM_FLAT, 0.1, 0.1
    };
    static VECTOR3 pos = { 0,2,4 };
    static VECTOR3 dir = { 0,1,0 };
    hatch_vent = new PSTREAM_HANDLE[1];   // this will be freed automatically for us later
    hatch_venting_lvl = new double[1];    // ditto
    hatch_venting_lvl[0] = 0.4;
    hatch_vent[0] = AddParticleStream(&airvent, pos, dir, hatch_venting_lvl);
    hatch_vent_t = GetAbsoluteSimTime();
}

// turn off hatch decompression exhaust stream; invoked form a PostStep
void DeltaGliderXR1::CleanUpHatchDecompression()
{
    DelExhaustStream(hatch_vent[0]);
}
