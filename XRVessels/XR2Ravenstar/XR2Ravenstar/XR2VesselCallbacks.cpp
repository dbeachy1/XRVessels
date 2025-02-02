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

#include "XR2Ravenstar.h"
#include "XR2AreaIDs.h" 

#include "meshres.h"

// --------------------------------------------------------------
// Initialize sound; invoked on startup
// --------------------------------------------------------------
bool XR2Ravenstar::InitSound()
{
    // normal initialization
    bool initSuccessful = DeltaGliderXR1::InitSound();
    // no longer used; handed by XRSound now    
#if 0
    if (initSuccessful)
    {
        // If enabled, load our custom main, hover, and RCS engine sounds
        if (GetXR1Config()->EnableCustomMainEngineSound)
            m_pXRSound->LoadWav(XRSound::MainThrust, "rocketmain.wav", XRSound::PlaybackType::BothViewFar);

        if (GetXR1Config()->EnableCustomHoverEngineSound)
            m_pXRSound->LoadWav(XRSound::HoverThrust, "rocketmain.wav", XRSound::PlaybackType::BothViewFar);

        if (GetXR1Config()->EnableCustomRCSSound)
        {
            m_pXRSound->LoadWav(XRSound::RCSThrustHit, "attitudecontrolhit.wav", XRSound::BothViewMedium);
            m_pXRSound->LoadWav(XRSound::RCSThrustSustain, "attitudecontrolsustain.wav", XRSound::BothViewMedium);
        }
    }
#endif
    return initSuccessful;
}

// --------------------------------------------------------------
// Respond to playback event
// NOTE: do not use spaces in any of these event ID strings.
// Returns: true if handled, false if not
// --------------------------------------------------------------
bool XR2Ravenstar::clbkPlaybackEvent(double simt, double event_t, const char* event_type, const char* event)
{
    // handle our custom events first
    // TODO: write me if necessary

    // let our superclass have it
    return DeltaGliderXR1::clbkPlaybackEvent(simt, event_t, event_type, event);
}

// --------------------------------------------------------------
// Create visual
// --------------------------------------------------------------
void XR2Ravenstar::clbkVisualCreated(VISHANDLE vis, int refcount)
{
    exmesh = GetDevMesh(vis, 0);
    // Note: vcmesh should remain nullptr here! It is safer that way, since any vcmesh operations
    // performed by the XR1 base class are XR1-mesh-specific.

    heatingmesh = GetDevMesh(vis, 1);  // hull heating mesh; the single group in this mesh is HIDDEN by default

    SetPassengerVisuals();
    SetDamageVisuals();

    HideActiveVCHUDMesh();
    ApplySkin();

    const XR2ConfigFileParser* pConfig = GetXR2Config();

    //
    // Hide Marvin (the Halloween easter egg) UNLESS 1) the actual date is Halloween, 2) the user 
    // did not explicitly request that we hide him, and 3) the user did not FORCE Marvin enabled.
    // Note that 'ForceMarvinVisible=1' overrides 'EnableHalloweenEasterEgg=0'.
    // 
    if ((IsToday(10, 31) == false) || (pConfig->EnableHalloweenEasterEgg == false))
    {
        // hide Marvin *unless* 'ForceMarvinVisible=1'
        if (pConfig->ForceMarvinVisible == false)
        {
            SetMeshGroupVisible(exmesh, GRP_grey, false);
        }
    }

    // Hide the fuzzy dice unless explicitly enabled by the user
    if (pConfig->EnableFuzzyDice == false)
    {
        // hide the fuzzy dice 
        DEVMESHHANDLE devMesh = GetDevMesh(vis, 0);
        SetMeshGroupVisible(exmesh, GRP_furrydice, false);
        SetMeshGroupVisible(exmesh, GRP_furrydice01, false);
        SetMeshGroupVisible(exmesh, GRP_Line01, false);
    }
}

// Hide the active VC HUD mesh, if any, so it is not rendered twice; if we don't do this the HUD glass
// is rendered twice, making it twice as opaque.
void XR2Ravenstar::HideActiveVCHUDMesh()
{
    if (!exmesh)
        return;

    bool pilotHUDVisible = true;
    bool copilotHUDVisible = true;

    if (campos == CAMERA_POSITION::CAM_VCPILOT)
        pilotHUDVisible = false;
    else if (campos == CAMERA_POSITION::CAM_VCCOPILOT)
        copilotHUDVisible = false;

    SetMeshGroupVisible(exmesh, PILOT_HUD_MESHGRP, pilotHUDVisible);
    SetMeshGroupVisible(exmesh, COPILOT_HUD_MESHGRP, copilotHUDVisible);
}

// --------------------------------------------------------------
// Destroy visual
// --------------------------------------------------------------
void XR2Ravenstar::clbkVisualDestroyed(VISHANDLE vis, int refcount)
{
    exmesh = nullptr;
    heatingmesh = nullptr;

    // Note: vcmesh remains nullptr at all times with the XR2
}

// override clbkPanelRedrawEvent so we can limit our refresh rates for our custom screens
bool XR2Ravenstar::clbkPanelRedrawEvent(int areaID, int event, SURFHANDLE surf)
{
    const XR2ConfigFileParser* pConfig = GetXR2Config();

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

            // no default case here; fall through
        }
    }

    // redraw is OK: invoke the superclass to dispatch the redraw event
    return DeltaGliderXR1::clbkPanelRedrawEvent(areaID, event, surf);
}

// --------------------------------------------------------------
// Respond to control surface mode change
// We need to hook this to implement our dual-mode AF Ctrl logic.
// --------------------------------------------------------------
// mode: 0=disabled, 1=pitch, 7=on
void XR2Ravenstar::clbkADCtrlMode(DWORD mode)
{
    // invoke the superclass to do the work
    DeltaGliderXR1::clbkADCtrlMode(mode);

    ApplyElevatorAreaChanges();
}
