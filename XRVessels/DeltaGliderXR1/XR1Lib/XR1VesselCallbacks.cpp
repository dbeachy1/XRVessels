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
// XR1 vessel non-startup callback methods invoked by Orbiter
// ==============================================================

#include "DeltaGliderXR1.h"

#include <string.h>
#include <string.h>
#include "AreaIDs.h"
#include "XR1PayloadDialog.h"
#include "XR1HUD.h"

// ==============================================================
// Overloaded callback functions
// NOTE: normally you should override these if you subclass the XR1!
// ==============================================================

// --------------------------------------------------------------
// Respond to MFD mode change
// --------------------------------------------------------------
void DeltaGliderXR1::clbkMFDMode(int mfd, int mode)
{
    TriggerRedrawArea(AID_MFD1_LBUTTONS + mfd);
    TriggerRedrawArea(AID_MFD1_RBUTTONS + mfd);
}

// --------------------------------------------------------------
// Respond to RCS mode change
// --------------------------------------------------------------
// mode: 0=disabled, 1=rotation, 2=translation
void DeltaGliderXR1::clbkRCSMode(int mode)
{
    TriggerRedrawArea(AID_RCSMODE);

    // play our custom sound IF the crew is not incapacitated!
    if (IsCrewIncapacitatedOrNoPilotOnBoard())
        return;

    Sound s;
    if (mode == 0)
        s = Off;
    else if (mode == 1)
        s = Rotation;
    else
        s = Translation;

    PlaySound(s, ST_RCSStatusCallout);
}

// --------------------------------------------------------------
// Respond to control surface mode change
// --------------------------------------------------------------
// mode: 0=disabled, 1=pitch, 7=on
void DeltaGliderXR1::clbkADCtrlMode(DWORD mode)
{
    TriggerRedrawArea(AID_AFCTRLMODE);

    // play our custom sound IF the APU is running and IF the crew is not incapacitated
    // otherwise, the AD ctrls may have just been turned off automatically
    if ((apu_status == DoorStatus::DOOR_OPEN) && (IsCrewIncapacitatedOrNoPilotOnBoard() == false))
    {
        Sound s;
        if (mode == 0)
            s = Off;
        else if (mode == 1)
            s = Pitch;
        else
            s = On;

        // SPECIAL CHECK: do not play the callout if the "no AF callout" flag is set
        if (m_skipNextAFCallout == false)
            PlaySound(s, ST_AFStatusCallout);
        else
            m_skipNextAFCallout = false;   // reset; we only want to skip one call
    }
}

// --------------------------------------------------------------
// Respond to navmode change
// NOTE: this does NOT include any custom autopilots such as 
// ATTITUDE HOLD and DESCENT HOLD.
// --------------------------------------------------------------
void DeltaGliderXR1::clbkNavMode(int mode, bool active)
{
    // redraw the navmode buttons
    TriggerNavButtonRedraw();

    const char* pAction = nullptr;    // set below
    if (active)
    {
        if (mode != NAVMODE_KILLROT)
        {
            PlaySound(AutopilotOn, ST_Other, AUTOPILOT_VOL);

            // disable any custom autopilot mode
            SetCustomAutopilotMode(AUTOPILOT::AP_OFF, false);  // do not play sounds for this
        }

        pAction = "engaged";
    }
    else  // normal autopilot disabled now
    {
        // play the AutopilotOff sound for all modes except KILLROT, UNLESS custom autopilot is active now
        // (we don't want to play AutoPilotOff if custom autopilot is on now)
        if ((mode != NAVMODE_KILLROT) && (m_customAutopilotMode == AUTOPILOT::AP_OFF))
            PlaySound(AutopilotOff, ST_Other, AUTOPILOT_VOL);

        pAction = "disengaged";
    }

    // set the corresponding label for all modes except killrot
    static const char* pNavModeLabels[] =
    {
        nullptr, nullptr, "LEVEL HORIZON", "PROGRADE", "RETROGRADE", "ORBIT-NORMAL",
        "ORBIT-ANTINORMAL", "HOLD ALTITUDE"
    };
    const char* pLabel = pNavModeLabels[mode];

    if (pLabel != nullptr)
    {
        char temp[40];
        sprintf(temp, "%s autopilot %s.", pLabel, pAction);
        ShowInfo(nullptr, DeltaGliderXR1::ST_None, temp);
    }
}

bool DeltaGliderXR1::clbkLoadGenericCockpit()
{
    SetCameraOffset(_V(0, 1.467, 6.782));
    oapiSetDefNavDisplay(1);
    oapiSetDefRCSDisplay(1);
    campos = DeltaGliderXR1::CAMERA_POSITION::CAM_GENERIC;
    return true;
}



// hook focus switch; we must be sure to call our superclass so VESSEL3_EXT will work properly.
void DeltaGliderXR1::clbkFocusChanged(bool getfocus, OBJHANDLE hNewVessel, OBJHANDLE hOldVessel)
{
    // are we losing focus?
    if (getfocus == false)
    {
        // close the payload editor if it is open: otherwise a stale dialog will remain open
        if (s_hPayloadEditorDialog != 0)
        {
            // editor is open: close it
            // do not beep here; this is automatic
            ::SendMessage(s_hPayloadEditorDialog, WM_TERMINATE, 0, reinterpret_cast<LPARAM>(this));
            s_hPayloadEditorDialog = 0;
        }
    }

    // propagate up
    VESSEL3_EXT::clbkFocusChanged(getfocus, hNewVessel, hOldVessel);
}

// override clbkPanelRedrawEvent so we can limit our refresh rates
bool DeltaGliderXR1::clbkPanelRedrawEvent(const int areaID, const int event, SURFHANDLE surf)
{
    // Only filter PANEL_REDRAW_ALWAYS events for timing!
    if (event == PANEL_REDRAW_ALWAYS)
    {
        // NOTE: we want to check *realtime* deltas, not *simulation time* here: repaint frequency should not
        // vary based on time acceleration.
        const double uptime = GetSystemUptime();  // will always count up

        // check for area IDs that have custom refresh rates
        switch (areaID)
        {
        case AID_MULTI_DISPLAY:
            if (uptime < m_nextMDARefresh)
                return false;

            // update for next interval
            m_nextMDARefresh = uptime + GetXR1Config()->MDAUpdateInterval;
            break;

        case AID_SECONDARY_HUD:
        {
            // only delay rendering if the HUD is fully deployed!
            PopupHUDArea* pHud = static_cast<PopupHUDArea*>(GetArea(PANEL_MAIN, AID_SECONDARY_HUD));  // will never be null
            if (pHud->GetState() == PopupHUDArea::OnOffState::On)
            {
                if (uptime < m_nextSecondaryHUDRefresh)
                    return false;
            }
            else
            {
                // else HUD is not fully deployed, so let's refresh it according to the default panel refresh rate rather than each frame so we don't cause a framerate stutter while the HUD is deploying
                goto panel_default_refresh;
            }

            // update for next interval
            m_nextSecondaryHUDRefresh = uptime + GetXR1Config()->SecondaryHUDUpdateInterval;
            break;
        }

        case AID_TERTIARY_HUD:
        {
            // only delay rendering if the HUD is fully deployed!
            PopupHUDArea* pHud = static_cast<PopupHUDArea*>(GetArea(PANEL_MAIN, AID_TERTIARY_HUD));  // will never be null
            if (pHud->GetState() == PopupHUDArea::OnOffState::On)
            {
                if (uptime < m_nextTertiaryHUDRefresh)
                    return false;
            }
            else
            {
                // else HUD is not fully deployed, so let's refresh it according to the default panel refresh rate rather than each frame so we don't cause a framerate stutter while the HUD is deploying
                goto panel_default_refresh;
            }

            // update for next interval
            m_nextTertiaryHUDRefresh = uptime + GetXR1Config()->TertiaryHUDUpdateInterval;
            break;
        }

        case AID_HORIZON:
        {
            if (uptime < m_nextArtificialHorizonRefresh)
                return false;

            // update for next interval
            m_nextArtificialHorizonRefresh = uptime + GetXR1Config()->ArtificialHorizonUpdateInterval;
            break;
        }

        default:
        panel_default_refresh:
            // defensive code: if GetXR1Config()->PanelUpdateInterval == 0, skip all these checks and just update each frame
            if (GetXR1Config()->PanelUpdateInterval > 0)
            {
                // for all other PANEL_REDRAW_ALWAYS components, limit them to a master framerate for the sake of performance (e.g., 60 fps)
                // retrieve the next uptime for this particular component
                auto it = m_nextRedrawAlwaysRefreshMap.find(areaID);
                double* pNextAreaRefresh = nullptr;  // points to it->second in m_nextRedrawAlwaysRefreshMap 
                if (it != m_nextRedrawAlwaysRefreshMap.end())
                {
                    // area has a uptime in the map, so let's test against that
                    pNextAreaRefresh = &(it->second);
                }
                else
                {
                    // no refresh uptime in the map yet for this area, so let's add one with a refresh uptime of now (for immediate update)
                    // Note: pr.first = new map entry, pr.second = insertion status: true if inserted successfully, false if already in map (should never happen here!)
                    pair<HASHMAP_UINT_DOUBLE::iterator, bool> pr = m_nextRedrawAlwaysRefreshMap.insert(uint_double_Pair(areaID, uptime));
                    _ASSERTE(pr.second);  // the above insert should always succeed
                    pNextAreaRefresh = &((pr.first)->second);  // points into uptime of map entry
                }

                if (uptime < *pNextAreaRefresh)
                    return false;   // not time to update this area yet
#if 0 // debug
                if (areaID == AID_ACC_SCALE)  // arbitrary
                    sprintf(oapiDebugString(), "uptime=%lf, next AID_ACC_SCALE refresh=%lf", uptime, *pNextAreaRefresh);
#endif
                // update this area's nextUpdateSimt for next interval
                * pNextAreaRefresh = uptime + GetXR1Config()->PanelUpdateInterval;
                break;
            }
        }
        // DEBUG: sprintf(oapiDebugString(), "uptime=%lf, m_nextTertiaryHUDRefresh=%lf", uptime, m_nextTertiaryHUDRefresh);
    }

    // let the superclass dispatch the redraw event
    return VESSEL3_EXT::clbkPanelRedrawEvent(areaID, event, surf);
}

// --------------------------------------------------------------
// Respond to playback event
// NOTE: do not use spaces in any of these event ID strings.
// --------------------------------------------------------------
bool DeltaGliderXR1::clbkPlaybackEvent(double simt, double event_t, const char* event_type, const char* event)
{
    if (!_stricmp(event_type, "GEAR"))
    {
        ActivateLandingGear(!_stricmp(event, "UP") ? DoorStatus::DOOR_CLOSING : DoorStatus::DOOR_OPENING);
        return true;
    }
    else if (!_stricmp(event_type, "NOSECONE"))
    {
        ActivateNoseCone(!_stricmp(event, "CLOSE") ? DoorStatus::DOOR_CLOSING : DoorStatus::DOOR_OPENING);
        return true;
    }
    else if (!_stricmp(event_type, "RCOVER"))
    {
        ActivateRCover(!_stricmp(event, "CLOSE") ? DoorStatus::DOOR_CLOSING : DoorStatus::DOOR_OPENING);
        return true;
    }
    else if (!_stricmp(event_type, "RADIATOR"))
    {
        ActivateRadiator(!_stricmp(event, "CLOSE") ? DoorStatus::DOOR_CLOSING : DoorStatus::DOOR_OPENING);
        return true;
    }
    else if (!_stricmp(event_type, "AIRBRAKE"))
    {
        ActivateAirbrake(!_stricmp(event, "CLOSE") ? DoorStatus::DOOR_CLOSING : DoorStatus::DOOR_OPENING);
        return true;
    }
    else if (!_stricmp(event_type, "HATCH"))
    {
        ActivateHatch(!_stricmp(event, "CLOSE") ? DoorStatus::DOOR_CLOSING : DoorStatus::DOOR_OPENING);
        return true;
    }
    else if (!_stricmp(event_type, "OLOCK"))
    {
        ActivateOuterAirlock(!_stricmp(event, "CLOSE") ? DoorStatus::DOOR_CLOSING : DoorStatus::DOOR_OPENING);
        return true;
    }
    else if (!_stricmp(event_type, "ILOCK"))
    {
        ActivateInnerAirlock(!_stricmp(event, "CLOSE") ? DoorStatus::DOOR_CLOSING : DoorStatus::DOOR_OPENING);
        return true;
    }
    else if (!_stricmp(event_type, "LADDER"))
    {
        ActivateLadder(!_stricmp(event, "CLOSE") ? DoorStatus::DOOR_CLOSING : DoorStatus::DOOR_OPENING);
        return true;
    }
    else if (!_stricmp(event_type, "APU"))
    {
        ActivateAPU(!_stricmp(event, "CLOSE") ? DoorStatus::DOOR_CLOSING : DoorStatus::DOOR_OPENING);
        return true;
    }
    else if (!_stricmp(event_type, "HOVERDOORS"))
    {
        ActivateHoverDoors(!_stricmp(event, "CLOSE") ? DoorStatus::DOOR_CLOSING : DoorStatus::DOOR_OPENING);
        return true;
    }
    else if (!_stricmp(event_type, "SCRAMDOORS"))
    {
        ActivateScramDoors(!_stricmp(event, "CLOSE") ? DoorStatus::DOOR_CLOSING : DoorStatus::DOOR_OPENING);
        return true;
    }
    else if (!_stricmp(event_type, "BAYDOORS"))
    {
        ActivateBayDoors(!_stricmp(event, "CLOSE") ? DoorStatus::DOOR_CLOSING : DoorStatus::DOOR_OPENING);
        return true;
    }
    else if (!_stricmp(event_type, "CHAMBER"))
    {
        ActivateChamber((!_stricmp(event, "CLOSE") ? DoorStatus::DOOR_CLOSING : DoorStatus::DOOR_OPENING), true);  // OK to force here, although it shouldn't be necessary
        return true;
    }
    // new for the XR1-1.9 release group
    else if (!_stricmp(event_type, "NAVLIGHT"))
    {
        SetNavlight(!_stricmp(event, "ON"));  // true = light on
        return true;
    }
    else if (!_stricmp(event_type, "BEACONLIGHT"))
    {
        SetBeacon(!_stricmp(event, "ON"));  // true = light on
        return true;
    }
    else if (!_stricmp(event_type, "STROBELIGHT"))
    {
        SetStrobe(!_stricmp(event, "ON"));  // true = light on
        return true;
    }
    else if (!_stricmp(event_type, "RESETMET"))
    {
        ResetMET();   // event not used for this
        return true;
    }
    else if (!_stricmp(event_type, "XFEED"))
    {
        XFEED_MODE mode;
        if (!_stricmp(event, "MAIN"))
            mode = XFEED_MODE::XF_MAIN;
        else if (!_stricmp(event, "RCS"))
            mode = XFEED_MODE::XF_RCS;
        else if (!_stricmp(event, "OFF"))
            mode = XFEED_MODE::XF_OFF;
        else  // invalid mode, so ignore it
        {
            _ASSERTE(false);
            return false;
        }
        SetCrossfeedMode(mode, nullptr);   // no optional message for this
        return true;
    }
    else if (!_stricmp(event_type, "MAINDUMP"))
    {
        m_mainFuelDumpInProgress = (!strcmp(event, "ON"));
        return true;
    }
    else if (!_stricmp(event_type, "RCSDUMP"))
    {
        m_rcsFuelDumpInProgress = (!strcmp(event, "ON"));
        return true;
    }
    else if (!_stricmp(event_type, "SCRAMDUMP"))
    {
        m_scramFuelDumpInProgress = (!strcmp(event, "ON"));
        return true;
    }
    else if (!_stricmp(event_type, "APUDUMP"))
    {
        m_apuFuelDumpInProgress = (!strcmp(event, "ON"));
        return true;
    }
    else if (!_stricmp(event_type, "LOXDUMP"))
    {
        m_loxDumpInProgress = (!strcmp(event, "ON"));
        return true;
    }

    return false;
}

// --------------------------------------------------------------
// Create DG visual
// --------------------------------------------------------------
void DeltaGliderXR1::clbkVisualCreated(VISHANDLE vis, int refcount)
{
    exmesh = GetDevMesh(vis, 0);
    vcmesh = GetDevMesh(vis, 1);
    SetPassengerVisuals();
    SetDamageVisuals();

    ApplySkin();

    // set VC state
    UpdateVCStatusIndicators();

    // redraw the navmode buttons
    TriggerNavButtonRedraw();

    // signal other 2D or 2D/3D shared areas
    // signal 3D areas
    TriggerRedrawArea(AID_HUDBUTTON1);
    TriggerRedrawArea(AID_HUDBUTTON2);
    TriggerRedrawArea(AID_HUDBUTTON3);
    TriggerRedrawArea(AID_HUDBUTTON4);

    UpdateVCMesh();
}

// --------------------------------------------------------------
// Destroy DG visual
// --------------------------------------------------------------
void DeltaGliderXR1::clbkVisualDestroyed(VISHANDLE vis, int refcount)
{
    exmesh = nullptr;
    vcmesh = nullptr;
}

// --------------------------------------------------------------
// PreStep Frame update; necessary to kill controls if ship crashed
// --------------------------------------------------------------
void DeltaGliderXR1::clbkPreStep(double simt, double simdt, double mjd)
{
    // calculate max scramjet thrust
    ScramjetThrust();

    // damage/failure system
    TestDamage();

    // Invoke our superclass handler so our prestep Area and PreStep objects are executed
    VESSEL3_EXT::clbkPreStep(simt, simdt, mjd);
}

// --------------------------------------------------------------
// PostStep Frame update
// --------------------------------------------------------------
void DeltaGliderXR1::clbkPostStep(double simt, double simdt, double mjd)
{
    // update VC warning lights
    UpdateVCStatusIndicators();

    // Invoke our superclass handler so our poststep Area and PostStep objects are executed
    VESSEL3_EXT::clbkPostStep(simt, simdt, mjd);
}

// hook whenever the 2D panel changes
bool DeltaGliderXR1::clbkLoadPanel(int panelID)
{
    m_lastActive2DPanelID = panelID;
    return VESSEL3_EXT::clbkLoadPanel(panelID);
}

// mate: nullptr = undocking event, otherwise vessel handle @ the docking port
void DeltaGliderXR1::clbkDockEvent(int dock, OBJHANDLE mate)
{
    // WARNING: cannot invoke Undock in this method or it will CTD Orbiter on exit, plus
    // the docking port will not work anymore after that.
    // if nosecone not open, PREVENT the dock event
    /* CANNOT DO THIS
    if (mate != nullptr)   // docking event?
    {
        // Note: a separate PreStep enables/disables docking callouts
        // depending on whether nosecone is open/closed.
        if (nose_status != DoorStatus::DOOR_OPEN)
            Undock(dock);   // undo the dock
    }
    */
}
