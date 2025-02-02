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
// XR vessel autopilot utility methods
// ==============================================================

#include "DeltaGliderXR1.h"
#include "AreaIDs.h"
#include "XR1MultiDisplayArea.h"

// redraw all 2D and 3D navmode buttons
void DeltaGliderXR1::TriggerNavButtonRedraw()
{
    // signal 2D area
    TriggerRedrawArea(AID_AUTOPILOTBUTTONS);

    // signal 3D areas
    TriggerRedrawArea(AID_NAVBUTTON1);
    TriggerRedrawArea(AID_NAVBUTTON2);
    TriggerRedrawArea(AID_NAVBUTTON3);
    TriggerRedrawArea(AID_NAVBUTTON4);
    TriggerRedrawArea(AID_NAVBUTTON5);
    TriggerRedrawArea(AID_NAVBUTTON6);
}

void DeltaGliderXR1::SetAirspeedHoldMode(bool on, bool playSound)
{
    if (m_airspeedHoldEngaged == on)
    {
        SetMDAModeForCustomAutopilot();
        return;     // state is unchanged
    }

    m_airspeedHoldEngaged = on;

    const char* pAction = (on ? "engaged" : "disengaged");

    char temp[60];
    sprintf(temp, "AIRSPEED HOLD autopilot %s.", pAction);
    ShowInfo(nullptr, DeltaGliderXR1::ST_None, temp);

    if (on)     // turning autopilot on?
    {
        // if rate == 0, default to HOLD CURRENT airspeed
        if (m_setAirspeed == 0)
            m_setAirspeed = GetAirspeed();

        if (m_setAirspeed < 0)
            m_setAirspeed = 0;

        sprintf(temp, "Hold Airspeed %.1f m/s", m_setAirspeed);
        ShowInfo(nullptr, DeltaGliderXR1::ST_None, temp);

        if (playSound)
            PlaySound(AutopilotOn, ST_Other, AUTOPILOT_VOL);

        SetMDAModeForCustomAutopilot();
    }
    else    // AP off now
    {
        if (playSound)
            PlaySound(AutopilotOff, ST_Other, AUTOPILOT_VOL);
    }

    // repaint the autopilot buttons
    TriggerNavButtonRedraw();
}

//
// Toggle custom autopilot methods
//

void DeltaGliderXR1::ToggleDescentHold()
{
    if (m_customAutopilotMode == AUTOPILOT::AP_DESCENTHOLD)
        SetCustomAutopilotMode(AUTOPILOT::AP_OFF, true);
    else
        SetCustomAutopilotMode(AUTOPILOT::AP_DESCENTHOLD, true);
}

void DeltaGliderXR1::ToggleAttitudeHold()
{
    if (m_customAutopilotMode == AUTOPILOT::AP_ATTITUDEHOLD)
        SetCustomAutopilotMode(AUTOPILOT::AP_OFF, true);
    else
        SetCustomAutopilotMode(AUTOPILOT::AP_ATTITUDEHOLD, true);
}

// holdCurrent: if 'true', hold current airspeed
void DeltaGliderXR1::ToggleAirspeedHold(bool holdCurrent)
{
    if (m_airspeedHoldEngaged)
        SetAirspeedHoldMode(false, true);   // turn off
    else
    {
        if (holdCurrent)
        {
            // will hold current airspeed now (no sound for this since we just played one)
            SetAirspeedHold(false, AIRSPEEDHOLD_ADJUST::AS_HOLDCURRENT, 0);
        }
        SetAirspeedHoldMode(true, true);    // turn on
    }
}

// Turn a custom autopilot mode on or off; plays sound as well if requested.
// NOTE: unlike other custom autopilots, AIRSPEED HOLD does not disengage other autopilots
// This is also invoked at load time.
// force: true = always set autopilot mode regardless of doors, etc.; necessary at load time
void DeltaGliderXR1::SetCustomAutopilotMode(AUTOPILOT mode, bool playSound, bool force)
{
    if (IsCrashed())
        return;     // nothing to do

    // if descent hold, verify that the hover doors are open
    if ((force == false) && (mode == AUTOPILOT::AP_DESCENTHOLD) && (m_isHoverEnabled == false))
    {
        PlaySound(HoverDoorsAreClosed, ST_WarningCallout);
        ShowWarning(nullptr, DeltaGliderXR1::ST_None, "WARNING: Hover Doors are closed;&cannot engage DESCENT HOLD.");
        SetCustomAutopilotMode(AUTOPILOT::AP_OFF, false, false);   // kill any existing autopilot
        m_autoLand = false;   // reset just in case
        return;     // nothing to do            
    }

    m_customAutopilotSuspended = false;     // reset
    const AUTOPILOT oldMode = m_customAutopilotMode;  // mode being exited; may be AP_OFF

    // must set new autopilot mode FIRST since GetRCSThrustMax references it to determine the max RCS thrust
    m_customAutopilotMode = mode;

    // Update the MDA mode if the MDA is visible
    SetMDAModeForCustomAutopilot();

    // display the appropriate info message
    const char* pAction = (mode == AUTOPILOT::AP_OFF ? "disengaged" : "engaged");
    char temp[50];

    // set mode being switched into or out of
    const AUTOPILOT actionMode = ((mode == AUTOPILOT::AP_OFF) ? oldMode : mode);
    switch (actionMode)
    {
    case AUTOPILOT::AP_ATTITUDEHOLD:
        sprintf(temp, "ATTITUDE HOLD autopilot %s.", pAction);
        ShowInfo(nullptr, DeltaGliderXR1::ST_None, temp);

        if (mode != AUTOPILOT::AP_OFF)     // autopilot on?
        {
            if (m_holdAOA)
                sprintf(temp, "Hold AOA=%+.1f°, Hold Bank=%+.1f°", m_setPitchOrAOA, m_setBank);
            else
                sprintf(temp, "Hold Pitch=%+.1f°, Hold Bank=%+.1f°", m_setPitchOrAOA, m_setBank);

            ShowInfo(nullptr, DeltaGliderXR1::ST_None, temp);
            m_initialAHBankCompleted = false;  // defensive coding: reset just in case
        }
        else  // AP off now
        {
            m_initialAHBankCompleted = false;  // reset
        }
        break;

    case AUTOPILOT::AP_DESCENTHOLD:
        sprintf(temp, "DESCENT HOLD autopilot %s.", pAction);
        ShowInfo(nullptr, DeltaGliderXR1::ST_None, temp);

        if (mode != AUTOPILOT::AP_OFF)     // turning autopilot on?
        {
            // if grounded and rate < 0.1, set rate = +0.1 m/s
            if (GroundContact() && (m_setDescentRate < 0.1))
                m_setDescentRate = 0.1;

            sprintf(temp, "Hold Rate=%+f m/s", m_setDescentRate);
            ShowInfo(nullptr, DeltaGliderXR1::ST_None, temp);
            m_autoLand = false;   // defensive coding: reset just in case
        }
        else    // AP off now
        {
            m_autoLand = false; // reset
        }
        break;

        // no default handler; may be AP_OFF
    }

    // play the correct sound and deactivate normal navmode if set
    // NOTE: do not modify AIRSPEED HOLD autopilot here
    if (mode == AUTOPILOT::AP_OFF)
    {
        if (playSound)
            PlaySound(AutopilotOff, ST_Other, AUTOPILOT_VOL);
    }
    else
    {
        // must turn off normal autopilots here so the new one can take effect
        for (int i = 0; i <= 7; i++)
            DeactivateNavmode(i);

        if (playSound)
            PlaySound(AutopilotOn, ST_Other, AUTOPILOT_VOL);
    }

    // reset all thruster levels; levels may vary by autopilot mode.  This takes damage into account.
    ResetAllRCSThrustMaxLevels();

    // repaint the autopilot buttons
    TriggerNavButtonRedraw();
}

// Set the active MDA mode to the custom autopilot if any is active; this should be 
// invoked on panel creation if the panel contains an MDA screen and whenever the custom autopilot
// mode changes.
void DeltaGliderXR1::SetMDAModeForCustomAutopilot()
{
    int modeNumber = -1;
    if (m_customAutopilotMode == AUTOPILOT::AP_DESCENTHOLD)
        modeNumber = MDMID_DESCENT_HOLD;
    else if (m_customAutopilotMode == AUTOPILOT::AP_ATTITUDEHOLD)
        modeNumber = MDMID_ATTITUDE_HOLD;
    else if (m_airspeedHoldEngaged)
        modeNumber = MDMID_AIRSPEED_HOLD;

    // only set the active MDA mode if it is visible
    if ((modeNumber >= 0) && m_pMDA)
        m_pMDA->SetActiveMode(modeNumber);
}

// Resets all RCSthruster levels; this takes autopilot mode and damage into account.  
void DeltaGliderXR1::ResetAllRCSThrustMaxLevels()
{
    // NOTE: must take damage into account here!
    for (int i = 0; i < 14; i++)
        SetThrusterMax0(th_rcs[i], (GetRCSThrustMax(i) * m_rcsIntegrityArray[i]));
}

// kill all autopilots, including airspeed hold.  Sound will play automatically.
void DeltaGliderXR1::KillAllAutopilots()
{
    SetCustomAutopilotMode(AUTOPILOT::AP_OFF, true); // turn off custom autopilot
    SetAirspeedHoldMode(false, false);    // turn off AIRSPEED HOLD; do not play sound again

    for (int i = 0; i <= 7; i++)
        DeactivateNavmode(i);
}

//
// Adjust AIRSPEED HOLD autopilot values; will play a button sound and show info message
//
// Rules:
//  Rate cannot go negative, but has no UPPER limit.
//
// delta = delta for AD_ADJUST mode
void DeltaGliderXR1::SetAirspeedHold(bool playSound, const AIRSPEEDHOLD_ADJUST mode, double delta)
{
    Sound sound = NO_SOUND;    // set below
    char msg[50];

    switch (mode)
    {
    case AIRSPEEDHOLD_ADJUST::AS_HOLDCURRENT:
        // hold current airspeed
        m_setAirspeed = GetAirspeed();
        if (m_setAirspeed < 0)
            m_setAirspeed = 0;

        sound = BeepHigh;
        sprintf(msg, "Airspeed Hold: holding %.1lf m/s.", m_setAirspeed);
        break;

    case AIRSPEEDHOLD_ADJUST::AS_RESET:
        m_setAirspeed = 0;
        sound = BeepLow;
        sprintf(msg, "Airspeed Hold: reset to 0 m/s.");
        break;

    case AIRSPEEDHOLD_ADJUST::AS_ADJUST:
        m_setAirspeed += delta;
        if (m_setAirspeed < 0)
            m_setAirspeed = 0;

        sound = ((delta >= 0) ? BeepHigh : BeepLow);
        sprintf(msg, "Airspeed Hold: set to %.1lf m/s.", m_setAirspeed);
        break;

        // no default handler here
    };

    if (playSound)
        PlaySound(sound, ST_Other);

    ShowInfo(nullptr, DeltaGliderXR1::ST_None, msg);
}

//
// Adjust DESCENT HOLD autopilot values; will play a button sound and show info message
//
// Rules:
//  Rate is limited to +/- MAX_DESCENT_HOLD_RATE m/s
//
// delta = delta for AD_ADJUST mode
void DeltaGliderXR1::SetAutoDescentRate(bool playSound, const AUTODESCENT_ADJUST mode, double delta)
{
    Sound sound = NO_SOUND;    // set below
    char msg[128];

    if (mode != AUTODESCENT_ADJUST::AD_AUTOLAND)
        m_autoLand = false;     // reset

    switch (mode)
    {
    case AUTODESCENT_ADJUST::AD_LEVEL:
        m_setDescentRate = 0;
        sound = BeepLow;
        strcpy(msg, "Descent Hold: reset to HOVER.");
        break;

    case AUTODESCENT_ADJUST::AD_ADJUST:
        m_setDescentRate += delta;
        if (m_setDescentRate > MAX_DESCENT_HOLD_RATE)
            m_setDescentRate = MAX_DESCENT_HOLD_RATE;
        else if (m_setDescentRate < -MAX_DESCENT_HOLD_RATE)
            m_setDescentRate = -MAX_DESCENT_HOLD_RATE;

        sound = ((delta >= 0) ? BeepHigh : BeepLow);
        sprintf(msg, "Descent Hold: set to %+.1f m/s.", m_setDescentRate);
        break;

    case AUTODESCENT_ADJUST::AD_AUTOLAND:
        // TOGGLE auto-land 
        if (m_autoLand == false)
        {
            m_autoLand = true;
            sound = BeepHigh;
            strcpy(msg, "Descent Hold: AUTO-LAND engaged.");
        }
        else    // turn auto-land OFF and switch to HOVER mode
        {
            m_autoLand = false;
            m_setDescentRate = 0;   // hover
            sound = BeepLow;
            strcpy(msg, "Descent Hold: AUTO-LAND disengaged.");
        }
        break;

        // no default handler here
    };

    if (playSound)
        PlaySound(sound, ST_Other);

    ShowInfo(nullptr, DeltaGliderXR1::ST_None, msg);
}

#define ROUND(value, boundary)   \
    {                            \
        double mod = fmod(value, boundary);     \
        value -= mod;                           \
        if (fabs(mod) >= (boundary / 2)) value += boundary; \
    }

// 
// Sync ATTITUDE HOLD autopilot targets to current attitude, rounded to nearest
// 5 degrees for bank and 0.5 degree for pitch/aoa.
//
void DeltaGliderXR1::SyncAttitudeHold(bool playSound, bool forcePitchHoldMode)
{
    if (playSound)
        PlaySound(BeepHigh, ST_Other);

    // switch to PITCH HOLD if requested
    if (forcePitchHoldMode)
        m_holdAOA = false;

    // round pitch to the nearest AP_PITCH_DELTA_SMALL
    double newPitch = (m_holdAOA ? GetAOA() : GetPitch()) * DEG;
    ROUND(newPitch, AP_PITCH_DELTA_SMALL);

    // round bank to the nearest AP_BANK_DELTA
    double newBank = GetBank() * DEG;
    ROUND(newBank, AP_BANK_DELTA);

    // limit both axes to MAX_ATTITUDE_HOLD_NORMAL (since bank is not set to either 0 or 180 yet, so we must always limit to MAX_ATTITUDE_HOLD_NORMAL here)
    LimitAttitudeHoldPitch(newPitch, MAX_ATTITUDE_HOLD_NORMAL);
    LimitAttitudeHoldBank(false, newBank, MAX_ATTITUDE_HOLD_NORMAL);  // 'increment' flag doesn't really matter here, although technically a "snap to nearest edge" would be better.  It's not worth the (considerable) extra work, though.

    m_setPitchOrAOA = newPitch;
    m_setBank = newBank;

    char msg[50];
    sprintf(msg, "Attitude Hold: %s synced to %+4.1f°", (m_holdAOA ? "AOA" : "Pitch"), m_setPitchOrAOA);
    ShowInfo(nullptr, DeltaGliderXR1::ST_None, msg);

    sprintf(msg, "Attitude Hold: Bank synced to %+4.1f°", m_setBank);
    ShowInfo(nullptr, DeltaGliderXR1::ST_None, msg);
}

// 
// Toggle ATTITUDE HOLD autopilot holding AOA or PITCH.
//
void DeltaGliderXR1::ToggleAOAPitchAttitudeHold(bool playSound)
{
    m_holdAOA = !m_holdAOA;

    if (playSound)
        PlaySound((m_holdAOA ? BeepLow : BeepHigh), ST_Other);

    // if autopilot is current ENGAGED, perform an implicit SYNC as well so we don't pitch like crazy in some situations
    if (m_customAutopilotMode == AUTOPILOT::AP_ATTITUDEHOLD)
    {
        // perform an implicit sync
        SyncAttitudeHold(false, false);  // no sound for this, since we just beeped above; also, do not force PITCH mode
    }
    else  // Attitude Hold autopilot NOT engaged; do not change target values
    {
        char msg[50];
        sprintf(msg, "Attitude Hold: Holding %+4.1f° %s", m_setPitchOrAOA, (m_holdAOA ? "AOA" : "PITCH"));
        ShowInfo(nullptr, DeltaGliderXR1::ST_None, msg);
    }
}

//
// Adjust ATTITUDE HOLD autopilot values; will play a button sound and show info message
//
// Rules:
//  If pitch is level in attitude hold, you can bank up to 75.0 degrees.  Otherwise, limit is 60.
//  If bank is level in attitude hold, you can pitch up to 87.5 degrees.  Otherwise, limit is 60.
//
void DeltaGliderXR1::ResetAttitudeHoldToLevel(bool playSound, bool resetBank, bool resetPitch)
{
    if (playSound)
        PlaySound(BeepLow, ST_Other);

    const char* pAxisMessage = nullptr;
    if (resetBank)
    {
        pAxisMessage = "bank";

        // level the ship to either 0 roll or 180 roll depending on the ship's current attitude.
        const double currentBank = GetBank() * DEG;   // in degrees
        if (fabs(currentBank) <= 90)
            m_setBank = 0;    // ship is right-side-up, so level heads-up
        else
            m_setBank = 180;  // ship is upside-down, so level heads-down
    }

    if (resetPitch)
    {
        pAxisMessage = (m_holdAOA ? "AOA" : "pitch");
        m_setPitchOrAOA = 0;
    }

    if (resetBank && resetPitch)
        pAxisMessage = "ship";

    if (pAxisMessage != nullptr)
    {
        char msg[50];
        sprintf(msg, "Attitude Hold: %s reset to level.", pAxisMessage);
        ShowInfo(nullptr, DeltaGliderXR1::ST_None, msg);
    }
}

// ensure that m_setPitchOrAOA and m_setBank are within autopilot limits
// incrementingBank: true = incrementing bank value, false = decrementing bank value.  This determines what the 
//                   bank value will "snap to" if it is out-of-range and must be limited.
void DeltaGliderXR1::LimitAttitudeHoldPitchAndBank(const bool incrementingBank)
{
    const bool isShipLevel = ((m_setBank == 0) || (fabs(m_setBank) == 180));  // Note: 0, 180, and -180 are all level

    // limit pitch, accounting for a higher pitch limit if the ship is level
    LimitAttitudeHoldPitch(m_setPitchOrAOA, (isShipLevel ? MAX_ATTITUDE_HOLD_ABSOLUTE_PITCH_OR_AOA : MAX_ATTITUDE_HOLD_NORMAL));

    // limit bank, accounting for a higher bank limit if set pitch or AoA is zero
    LimitAttitudeHoldBank(incrementingBank, m_setBank, ((m_setPitchOrAOA == 0) ? MAX_ATTITUDE_HOLD_ABSOLUTE_BANK : MAX_ATTITUDE_HOLD_NORMAL));
}

// Static method: limit check will block out the "cones" around +90 and -90 degrees for pitch
// val = value to be limited
void DeltaGliderXR1::LimitAttitudeHoldPitch(double& val, const double limit)
{
    if (val > limit)
        val = limit;
    else if (val < -limit)
        val = -limit;
}

// Static method: limit check will block out the "cones" on both sides of +90 and -90 degrees for bank
// e.g., -60 to +60 and -120 to +120 (60-degree cones from either side of 0 & 180), 
// or    -75 to +75 and -105 to +105 (75-degree cones from either side of 0 & 180)
// onIncrement: true = incrementing bank value, false = decrementing bank value.  This determines what the 
//              bank value will "snap to" if it is out-of-range.
// val = value to be limited
// NOTE: If Attitude Hold is engaged, we disable the "snap to" functionality.  If *disengaged*, we enable the "snap-to" functionality.  
//       i.e., once you engage Attitude Hold you cannot cross a "snap-to" boundary.
//       This is by design so you do not flip the ship over accidentally during reentry or exceed autopilot hold limits.
void DeltaGliderXR1::LimitAttitudeHoldBank(const bool increment, double& val, const double limit)
{
    // Handle the +180 -> 179 and -180 -> +179 rollovers.
    // Note that both +180.0 and -180.0 are valid.
    if (val > 180)          // rolling over into -179 range
        val = -360 + val;   // result is > -180
    else if (val < -180)    // rolling over into +179 range
        val = 360 + val;    // result is < +180 now

    const double maxInvertedAttitudeHoldNormal = 180 - limit; // e.g., 120 = -120...180...+120

    // "Snap-to" clockwise quadrant sequence will be 1 -> 2 -> 3 -> 4 -> 1 ... (jump across quadrants), but *only if* the attitude hold autopilot is disengaged.
    //  i.e., 2 o'clock -> 4 o'clock -> 8 o'clock -> 10 o'clock
    // 0 degrees = midnight on a clock for our diagram purposes here
    if (m_customAutopilotMode == AUTOPILOT::AP_ATTITUDEHOLD)
    {
        bool limitedBank = false;  // set to true if we had to limit the bank setting below

        // Attitude Hold is engaged, so perform hard limit checks and do not cross quadrant boundaries
        // upper half (normal flight): -60...0...+60                                                      
        if ((val > limit) && (val <= 90))    // >60, <=90 : quadrant 4 (10 o'clock)
        {
            val = limit;                     // limit to +60
            limitedBank = true;
        }
        else if ((val < -limit) && (val >= -90))  // <-60, >=-90 : quadrant 1 (2 o'clock)
        {
            val = -limit;                         // limit to -60
            limitedBank = true;
        }
        // lower half (inverted flight): -120...180...+120
        else if ((val < maxInvertedAttitudeHoldNormal) && (val >= 90)) // <120, >=90 : quadrant 3 (8 o'clock)
        {
            val = maxInvertedAttitudeHoldNormal;
            limitedBank = true;
        }
        else if ((val > -maxInvertedAttitudeHoldNormal) && (val <= -90)) // >-120, <=-90 : quadrant 2 (4 o'clock)
        {
            val = -maxInvertedAttitudeHoldNormal;
            limitedBank = true;
        }

        // Notifiy the user here if we had to limit the bank (he may want to invert the ship, it may have been an accident, 
        // or he may have just wanted to rotate a little farther.  
        if (limitedBank)
        {
            PlaySound(Error1, ST_Other, ERROR1_VOL);
            ShowWarning(nullptr, ST_None, "As a flight safety measure&you must disengage Attitude Hold&before setting an inverted bank level.");
        }
    }
    else
    {
        // Attitude Hold is NOT engaged, so cross quadrant boundaries
        // upper half (normal flight): -60...0...+60                                                      
        if ((val > limit) && (val <= 90))    // >60, <=90 : quadrant 4 (10 o'clock)
            val = (increment ? maxInvertedAttitudeHoldNormal : -limit);  // snap to quadrant 3 (CCW) or quadrant 1 (CW)
        else if ((val < -limit) && (val >= -90))  // <-60, >=-90 : quadrant 1 (2 o'clock)
            val = (increment ? limit : -maxInvertedAttitudeHoldNormal);  // snap to quadrant 4 (CCW) or quadrant 2 (CW)
        // lower half (inverted flight): -120...180...+120
        else if ((val < maxInvertedAttitudeHoldNormal) && (val >= 90))  // <120, >=90 : quadrant 3 (8 o'clock)
            val = (increment ? -maxInvertedAttitudeHoldNormal : limit);  // snap to quadrant 2 (CCW) or quadrant 4 (CW)
        else if ((val > -maxInvertedAttitudeHoldNormal) && (val <= -90)) // >-120, <=-90 : quadrant 2 (4 o'clock)
            val = (increment ? -limit : maxInvertedAttitudeHoldNormal);  // snap to quadrant 1 (CCW) or quadrant 3 (CW)
    }
}

// Note: we need to check both pitch & bank limits in these methods because the absolute pitch limit can change depending on
// whether the bank just went from zero to non-zero (and vice-versa with bank vs. pitch).
void DeltaGliderXR1::IncrementAttitudeHoldPitch(bool playSound, bool changeAxis, double stepSize)
{
    if (changeAxis)
    {
        m_setPitchOrAOA += stepSize;
        LimitAttitudeHoldPitchAndBank(false);  // incrementBank flag doesn't matter here
    }

    // for performance reasons, only log message if playing sound
    // play button sound in slot separate from info message slot
    if (playSound)
    {
        PlaySound(BeepHigh, ST_Other);

        char temp[40];
        sprintf(temp, "Attitude Hold: %s %+4.1f°", (m_holdAOA ? "AOA" : "Pitch"), m_setPitchOrAOA);
        ShowInfo(nullptr, DeltaGliderXR1::ST_None, temp);
    }
}

void DeltaGliderXR1::DecrementAttitudeHoldPitch(bool playSound, bool changeAxis, double stepSize)
{
    if (changeAxis)
    {
        m_setPitchOrAOA -= stepSize;
        LimitAttitudeHoldPitchAndBank(false);  // incrementBank flag doesn't matter here
    }

    // for performance reasons, only log message if playing sound
    // play button sound in slot separate from info message slot
    if (playSound)
    {
        PlaySound(BeepLow, ST_Other);

        char temp[40];
        sprintf(temp, "Attitude Hold: %s %+4.1f°", (m_holdAOA ? "AOA" : "Pitch"), m_setPitchOrAOA);
        ShowInfo(nullptr, DeltaGliderXR1::ST_None, temp);
    }
}

void DeltaGliderXR1::IncrementAttitudeHoldBank(bool playSound, bool changeAxis)
{
    if (changeAxis)
    {
        m_setBank += AP_BANK_DELTA;
        LimitAttitudeHoldPitchAndBank(true);
    }

    // for performance reasons, only log message if playing sound
    // play button sound in slot separate from info message slot
    if (playSound)
    {
        PlaySound(BeepHigh, ST_Other);

        char temp[40];
        sprintf(temp, "Attitude Hold: Bank %+4.1f°", m_setBank);
        ShowInfo(nullptr, DeltaGliderXR1::ST_None, temp);
    }
}

void DeltaGliderXR1::DecrementAttitudeHoldBank(bool playSound, bool changeAxis)
{
    if (changeAxis)
    {
        m_setBank -= AP_BANK_DELTA;
        LimitAttitudeHoldPitchAndBank(false);
    }

    // for performance reasons, only log message if playing sound
    // play button sound in slot separate from info message slot
    if (playSound)
    {
        PlaySound(BeepLow, ST_Other);

        char temp[40];
        sprintf(temp, "Attitude Hold: Bank %+4.1f°", m_setBank);
        ShowInfo(nullptr, DeltaGliderXR1::ST_None, temp);
    }
}
