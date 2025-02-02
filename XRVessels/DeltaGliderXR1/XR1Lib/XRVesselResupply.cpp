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
// XR vessel resupply utility methods
// ==============================================================

#include "DeltaGliderXR1.h"
#include "AreaIDs.h"

// Set crossfeed mode main/rcs/off
// pMsg = mode-specific infomation message; may be null
void DeltaGliderXR1::SetCrossfeedMode(const XFEED_MODE mode, const char* pMsg)
{
    char modeString[16];
    m_xfeedMode = mode;

    if (mode == XFEED_MODE::XF_OFF)
    {
        char temp[80];
        if (pMsg != nullptr)
            sprintf(temp, "%s; cross-feed OFF.", pMsg);
        else
            strcpy(temp, "Fuel cross-feed OFF.");  // no optional reason
        ShowInfo("Cross-Feed Off.wav", DeltaGliderXR1::ST_InformationCallout, temp);
        strcpy(modeString, "OFF");
    }
    else if (mode == XFEED_MODE::XF_MAIN)
    {
        ShowInfo("Cross-Feed Main.wav", DeltaGliderXR1::ST_InformationCallout, "Fuel cross-feed to MAIN.");
        strcpy(modeString, "MAIN");
    }
    else if (mode == XFEED_MODE::XF_RCS)
    {
        ShowInfo("Cross-Feed RCS.wav", DeltaGliderXR1::ST_InformationCallout, "Fuel cross-feed to RCS.");
        strcpy(modeString, "RCS");
    }
    else  // invalid mode!  (should never happen)
    {
        _ASSERTE(false);
        return;
    }

    // refresh the xfeed knob in case it wasn't a mouse event that triggered our status change
    TriggerRedrawArea(AID_XFEED_KNOB);

    // save a replay event
    RecordEvent("XFEED", modeString);
}

void DeltaGliderXR1::SetFuelDumpState(bool& fuelDumpInProgress, const bool isDumping, const char* pFuelLabel)
{
    if (isDumping)  // fuel dump in progress?
    {
        // start the fuel dump
        fuelDumpInProgress = true;
        PlaySound(BeepHigh, DeltaGliderXR1::ST_Other);
        // NOTE: do not display a warning message here: it is handled by the DumpFuel poststep for technical reasons
    }
    else   // fuel dump halted
    {
        char temp[45];
        fuelDumpInProgress = false;       // fuel dump halted now
        PlaySound(BeepLow, DeltaGliderXR1::ST_Other);
        sprintf(temp, "%s fuel dump halted.", pFuelLabel);
        ShowInfo(nullptr, DeltaGliderXR1::ST_None, temp);
    }

    // Convert the fueldump reference to a string; this is a bit of a hack since it checks pointer addresses instead
    // of a proper flag, but it should be fine since these will never change.
    const char* pEventName;
    const bool* pIsInProgress = &fuelDumpInProgress;
    if (pIsInProgress == &m_mainFuelDumpInProgress)
        pEventName = "MAINDUMP";
    else if (pIsInProgress == &m_rcsFuelDumpInProgress)
        pEventName = "RCSDUMP";
    else if (pIsInProgress == &m_scramFuelDumpInProgress)
        pEventName = "SCRAMDUMP";
    else if (pIsInProgress == &m_apuFuelDumpInProgress)
        pEventName = "APUDUMP";

    // save a replay event
    RecordEvent(pEventName, (isDumping ? "ON" : "OFF"));
}

void DeltaGliderXR1::SetLOXDumpState(const bool isDumping)
{
    if (isDumping)
    {
        // start the lox dump
        m_loxDumpInProgress = true;
        PlaySound(BeepHigh, DeltaGliderXR1::ST_Other);
        // NOTE: do not display a warning message here: it is handled by the DumpFuel poststep
    }
    else
    {
        m_loxDumpInProgress = false;       // halted now
        PlaySound(BeepLow, DeltaGliderXR1::ST_Other);
        ShowInfo(nullptr, DeltaGliderXR1::ST_None, "LOX dump halted.");
    }

    // save a replay event
    RecordEvent("LOXDUMP", (isDumping ? "ON" : "OFF"));
}

// Request that external cooling be enabled or disabled.
// Shows a success or failure message on the secondary HUD and plays a beep. 
// Returns: true on success, false on failure
bool DeltaGliderXR1::RequestExternalCooling(const bool bEnableExternalCooling)
{
    // may use external coolant if landed OR if docked
    const bool doorUnlocked = (IsLanded() || IsDocked());
    if (doorUnlocked == false)
    {
        PlaySound(Error1, DeltaGliderXR1::ST_Other, ERROR1_VOL);
        ShowWarning("Hatch is Locked.wav", DeltaGliderXR1::ST_WarningCallout, "External cooling hatch is locked&while in flight.");
        return false;
    }

    // set door state
    externalcooling_status = (bEnableExternalCooling ? DoorStatus::DOOR_OPEN : DoorStatus::DOOR_CLOSED);

    // play door thump sound 
    PlaySound(SupplyHatch, DeltaGliderXR1::ST_Other, SUPPLY_HATCH_VOL);

    // log info message and play callout
    const char* pState = (bEnableExternalCooling ? "open" : "closed");
    char msg[40];
    char wavFilename[40];
    sprintf(msg, "External coolant hatch %s.", pState);
    sprintf(wavFilename, "Hatch %s.wav", pState);

    // NOTE: do not attempt to play a "Hatch Closed" callout since our FuelPostStep will play a proper "External Cooling Systems Offline" callout.
    ShowInfo((bEnableExternalCooling ? wavFilename : nullptr), DeltaGliderXR1::ST_InformationCallout, msg);

    // show the new state on the panel
    TriggerRedrawArea(AID_EXTERNAL_COOLING_SWITCH);
    TriggerRedrawArea(AID_EXTERNAL_COOLING_LED);

    return true;
}
