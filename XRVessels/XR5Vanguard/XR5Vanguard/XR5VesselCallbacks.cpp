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
// XR5 vessel non-startup callback methods invoked by Orbiter
// ==============================================================

#include "XR5Vanguard.h"
#include "XR5AreaIDs.h"  

// --------------------------------------------------------------
// Respond to playback event
// NOTE: do not use spaces in any of these event ID strings.
// --------------------------------------------------------------
bool XR5Vanguard::clbkPlaybackEvent(double simt, double event_t, const char* event_type, const char* event)
{
    // check for XR5-specific events
    if (!_stricmp(event_type, "ELEVATOR"))
    {
        ActivateElevator(!_stricmp(event, "CLOSE") ? DoorStatus::DOOR_CLOSING : DoorStatus::DOOR_OPENING);
        return true;
    }

    // else let our superclass have it
    return DeltaGliderXR1::clbkPlaybackEvent(simt, event_t, event_type, event);
}

// --------------------------------------------------------------
// Create visual
// --------------------------------------------------------------
void XR5Vanguard::clbkVisualCreated(VISHANDLE vis, int refcount)
{
    exmesh = GetDevMesh(vis, 0);
    // no VC: vcmesh = GetMesh (vis, 1);
    vcmesh = nullptr;
    SetPassengerVisuals();    // NOP for now, but let's invoke it anyway
    SetDamageVisuals();

    ApplySkin();

    // no VC: UpdateVCStatusIndicators();

    // redraw the navmode buttons
    TriggerNavButtonRedraw();

    // no VC: UpdateVCMesh();

    // show or hide the landing gear
    SetGearParameters(gear_proc);
}

// Invoked whenever the crew onboard change
void XR5Vanguard::SetPassengerVisuals()
{
    // nothing to do
}

// --------------------------------------------------------------
// Destroy visual
// --------------------------------------------------------------
void XR5Vanguard::clbkVisualDestroyed(VISHANDLE vis, int refcount)
{
    exmesh = nullptr;
    vcmesh = nullptr;
}

// --------------------------------------------------------------
// Respond to navmode change
// NOTE: this does NOT include any custom autopilots such as 
// ATTITUDE HOLD and DESCENT HOLD.
// --------------------------------------------------------------
void XR5Vanguard::clbkNavMode(int mode, bool active)
{
    if (mode == NAVMODE_KILLROT)
    {
        if (active)
        {
            m_rcsDockingModeAtKillrotStart = m_rcsDockingMode;
            ConfigureRCSJets(false);            // must revert for killrot to work properly
        }
        else  // killrot just disengaged
        {
            ConfigureRCSJets(m_rcsDockingModeAtKillrotStart);    // restore previous state
        }
    }
    else    // some other mode: disable docking config if mode is active
    {
        if (active)
            ConfigureRCSJets(false);  // must revert for autopilots to work properly
    }

    // propogate to the superclass
    DeltaGliderXR1::clbkNavMode(mode, active);
}

// override clbkPanelRedrawEvent so we can limit our refresh rates for our custom screens
bool XR5Vanguard::clbkPanelRedrawEvent(int areaID, int event, SURFHANDLE surf)
{
    const XR5ConfigFileParser* pConfig = GetXR5Config();

    // Only filter PANEL_REDRAW_ALWAYS events for timing!
    if (event == PANEL_REDRAW_ALWAYS)
    {
        // NOTE: we want to check *realtime* deltas, not *simulation time* here: repaint frequency should not
        // vary based on time acceleration.
        const double uptime = GetSystemUptime();  // will always count up

        // check for area IDs that have custom refresh rates
        int screenIndex = 0;  // for payload screens
        switch (areaID)
        {
        case AID_SELECT_PAYLOAD_BAY_SLOT_SCREEN:
            goto check;     // 0

        case AID_GRAPPLE_PAYLOAD_SCREEN:
            screenIndex = 1;
            goto check;

        case AID_DEPLOY_PAYLOAD_SCREEN:
            screenIndex = 2;
        check:
            if (uptime < m_nextPayloadScreensRefresh[screenIndex])
                return false;

            // update for next interval
            m_nextPayloadScreensRefresh[screenIndex] = uptime + pConfig->PayloadScreensUpdateInterval;

            // force the repaint to occur by invoking the VESSEL3 superclass directly; otherwise the XR1 impl would 
            // see each of these areas as just another area and limit it by PanelUpdateInterval yet, which we want to bypass.
            return VESSEL3_EXT::clbkPanelRedrawEvent(areaID, event, surf);
        }
    }

    // redraw is OK: invoke the superclass to dispatch the redraw event
    return DeltaGliderXR1::clbkPanelRedrawEvent(areaID, event, surf);
}


