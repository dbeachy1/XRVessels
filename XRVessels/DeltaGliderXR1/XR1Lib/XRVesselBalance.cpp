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
// XR vessel balance-related methods
// ==============================================================

#include "DeltaGliderXR1.h"
#include "AreaIDs.h"

// Gimbal SCRAM engine pitch
void DeltaGliderXR1::GimbalSCRAMPitch(const GIMBAL_SWITCH which, const DIRECTION dir)
{
    if (dir == DIRECTION::DIR_NONE)
        return;   // nothing to do

    // warn the user if APU is offline
    if (CheckHydraulicPressure(true, true) == false)
        return;

    VECTOR3 dirVec;
    double phi, dphi = oapiGetSimStep() * SCRAM_GIMBAL_SPEED * (dir == DIRECTION::UP_OR_LEFT ? -1.0 : 1.0);

    for (int i = 0; i < 2; i++)  // process both switches
    {
        if ((i == static_cast<int>(which)) || (which == GIMBAL_SWITCH::BOTH))   // is one or BOTH switches pressed?
        {
            GetThrusterDir(th_scram[i], dirVec);
            phi = atan2(dirVec.y, dirVec.z);
            phi = min(SCRAM_DEFAULT_DIR + SCRAM_GIMBAL_RANGE, max(SCRAM_DEFAULT_DIR - SCRAM_GIMBAL_RANGE, phi + dphi));
            SetThrusterDir(th_scram[i], _V(0, sin(phi), cos(phi)));

            MarkAPUActive();  // reset the APU idle warning callout time
        }
    }
}

//-------------------------------------------------------------------------

void DeltaGliderXR1::GimbalMainPitch(const GIMBAL_SWITCH which, const DIRECTION dir)
{
    if (dir == DIRECTION::DIR_NONE)
        return;   // nothing to do

    // warn the user if APU is offline
    if (CheckHydraulicPressure(true, true) == false)
        return;

    VECTOR3 dirVec;
    double dy = oapiGetSimStep() * MAIN_PGIMBAL_SPEED * (dir == DIRECTION::UP_OR_LEFT ? -1.0 : 1.0);

    for (int i = 0; i < 2; i++)  // process both switches
    {
        if ((i == static_cast<int>(which)) || (which == GIMBAL_SWITCH::BOTH))   // is FOO or BOTH switches pressed?
        {
            GetThrusterDir(th_main[i], dirVec);
            dirVec /= dirVec.z;
            dirVec.y = min(MAIN_PGIMBAL_RANGE, max(-MAIN_PGIMBAL_RANGE, dirVec.y + dy));
            SetThrusterDir(th_main[i], dirVec);

            MarkAPUActive();  // reset the APU idle warning callout time
        }
    }
}

//-------------------------------------------------------------------------

void DeltaGliderXR1::GimbalMainYaw(const GIMBAL_SWITCH which, const DIRECTION dir)
{
    if (dir == DIRECTION::DIR_NONE)
        return;   // nothing to do

    // warn the user if APU is offline
    if (CheckHydraulicPressure(true, true) == false)
        return;

    VECTOR3 dirVec;
    double dx = oapiGetSimStep() * MAIN_YGIMBAL_SPEED * (dir == DIRECTION::UP_OR_LEFT ? 1.0 : -1.0);

    for (int i = 0; i < 2; i++)  // process both switches
    {
        if ((static_cast<int>(which) == i) || (which == GIMBAL_SWITCH::BOTH))  // is switch #i pressed?
        {
            GetThrusterDir(th_main[i], dirVec);
            dirVec /= dirVec.z;
            dirVec.x = min(MAIN_YGIMBAL_RANGE, max(-MAIN_YGIMBAL_RANGE, dirVec.x + dx));
            SetThrusterDir(th_main[i], dirVec);

            MarkAPUActive();  // reset the APU idle warning callout time
        }
    }
}

//-------------------------------------------------------------------------

void DeltaGliderXR1::ShiftHoverBalance(const GIMBAL_SWITCH which, const DIRECTION dir)
{
    if (dir == DIRECTION::DIR_NONE)
        return;   // nothing to do

    // warn the user if APU is offline
    if (CheckHydraulicPressure(true, true) == false)
        return;

    double shift = oapiGetSimStep() * HOVER_BALANCE_SPEED * (dir == DIRECTION::UP_OR_LEFT ? 1.0 : -1.0);  // shift as a fraction of balance for this timestep
    m_hoverBalance += shift;       // adjust the balance

    // keep in range
    if (m_hoverBalance > MAX_HOVER_IMBALANCE)
        m_hoverBalance = MAX_HOVER_IMBALANCE;
    else if (m_hoverBalance < -MAX_HOVER_IMBALANCE)
        m_hoverBalance = -MAX_HOVER_IMBALANCE;

    // NOTE: must take damage into account here!
    const int hoverThrustIdx = GetXR1Config()->HoverEngineThrust;
    const double maxThrustFore = MAX_HOVER_THRUST[hoverThrustIdx] * GetDamageStatus(DamageItem::HoverEngineFore).fracIntegrity;
    const double maxThrustAft = MAX_HOVER_THRUST[hoverThrustIdx] * GetDamageStatus(DamageItem::HoverEngineAft).fracIntegrity;

    SetThrusterMax0(th_hover[0], maxThrustFore * (1.0 + m_hoverBalance));
    SetThrusterMax0(th_hover[1], maxThrustAft * (1.0 - m_hoverBalance));

    MarkAPUActive();  // reset the APU idle warning callout time
}

//------------------------------------------------------------------------

// gimbal recenter ALL engines
void DeltaGliderXR1::GimbalRecenterAll()
{
    m_mainPitchCenteringMode = m_mainYawCenteringMode = m_scramCenteringMode = true;
}

//-------------------------------------------------------------------------
// Verify that a manual COG shift is available and play a warning beep and a voice callout if it is not.
// Returns: true if manual COG OK, false if locked or offline
//-------------------------------------------------------------------------
bool DeltaGliderXR1::VerifyManualCOGShiftAvailable()
{
    bool retCode = true;

    // can't move unless the APU is online
    if (CheckHydraulicPressure(false, true) == false)  // play error beep, but no wav for this; we will handle it below
    {
        PlayErrorBeep();
        ShowWarning("Warning Center of Gravity Shift Offline.wav", ST_WarningCallout, "Warning: APU offline; cannot&shift the center of gravity.");
        retCode = false;
    }
    else if (m_customAutopilotMode == AUTOPILOT::AP_ATTITUDEHOLD)
    {
        PlayErrorBeep();
        ShowWarning("Locked by Attitude Hold.wav", ST_WarningCallout, "Center of Gravity shift locked&by Attitude Hold Autopilot.");
        retCode = false;
    }
    else if (m_cogShiftAutoModeActive)
    {
        PlayErrorBeep();
        ShowWarning("Locked by Auto Mode.wav", ST_WarningCallout, "Center of Gravity shift locked&by AUTO Mode.");
        retCode = false;
    }

    return retCode;
}

// Enable or disable mode to reset the center-of-gravity
void DeltaGliderXR1::SetRecenterCenterOfGravityMode(const bool bEnabled)
{
    m_cogShiftCenterModeActive = bEnabled;
    TriggerRedrawArea(AID_COG_CENTER_BUTTON);
}

//-------------------------------------------------------------------------
// Shift the center-of-lift by a requested amount, verifying that the APU
// is running first.  
// 
// WARNING: this does NOT show a warning to the user if the APU is offline;
// it is the caller's responsibility to decide how to handle that.
//
// requestedShift = requested delta in meters from the current center-of-lift
// 
// Returns: true if shift successful, or false if the shift range was maxed out or 
// APU is offline.
//-------------------------------------------------------------------------
bool DeltaGliderXR1::ShiftCenterOfLift(double requestedShift)
{
    // the caller should have already checked this, but let's make sure...
    if (CheckHydraulicPressure(false, false) == false)   // no sound here
        return false;

    // unstable during reentry: requestedShift /= oapiGetTimeAcceleration();

    bool retVal = true;     // assume success
    m_centerOfLift += requestedShift;

    // never exceed the maximum shift allowed
    const double shiftDelta = m_centerOfLift - NEUTRAL_CENTER_OF_LIFT;
    if (shiftDelta < -COL_MAX_SHIFT_DISTANCE)
    {
        m_centerOfLift = NEUTRAL_CENTER_OF_LIFT - COL_MAX_SHIFT_DISTANCE;
        retVal = false;   // maxed out
    }
    else if (shiftDelta > COL_MAX_SHIFT_DISTANCE)
    {
        m_centerOfLift = NEUTRAL_CENTER_OF_LIFT + COL_MAX_SHIFT_DISTANCE;
        retVal = false;   // maxed out
    }

    EditAirfoil(hwing, 0x01, _V(m_wingBalance, 0, m_centerOfLift), nullptr, 0, 0, 0);
    // debug: sprintf(oapiDebugString(), "requestedColShift=%lf, m_centerOfLift=%lf", requestedShift, m_centerOfLift);  

    MarkAPUActive();  // reset the APU idle warning callout time

    return retVal;
}
