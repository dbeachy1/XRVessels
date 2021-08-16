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
// XR vessel callback methods invoked by Orbiter
// ==============================================================

#include "DeltaGliderXR1.h"

#include <string.h>
#include <string.h>
#include "AreaIDs.h"
#include "XR1PayloadDialog.h"
#include "XR1HUD.h"
#include "XR1InstrumentPanels.h"
#include "XR1PreSteps.h"
#include "XR1PostSteps.h"
#include "XR1FuelPostSteps.h"
#include "XR1AnimationPoststep.h"

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

// ==============================================================
// Overloaded callback functions
// NOTE: normally you should override these if you subclass the XR1!
// ==============================================================

// --------------------------------------------------------------
// Set vessel class parameters
// --------------------------------------------------------------
void DeltaGliderXR1::clbkSetClassCaps(FILEHANDLE cfg)
{
    // parse the configuration file
    // If parse fails, we shouldn't display a MessageBox here because the Orbiter main window 
    // keeps putting itself in the foreground, covering it up and making Orbiter look like it's hung.
    // Therefore, TakeoffAndLandingCalloutsAndCrashPostStep will blink a warning message for us 
    // if the parse fails.
    ParseXRConfigFile();  // common XR code

    // Note: this must be invoked here instead of the constructor so the subclass may override it!
    DefineAnimations();

    // *************** physical parameters **********************

    ramjet = new XR1Ramjet(this);

    VESSEL2::SetEmptyMass(EMPTY_MASS);
    SetSize(10.0);
    SetVisibilityLimit(7.5e-4, 1.5e-3);
    SetAlbedoRGB(_V(0.77, 0.20, 0.13));
    SetGravityGradientDamping(20.0);
    SetCW(0.09, 0.09, 2, 1.4);
    SetWingAspect(WING_ASPECT_RATIO);
    SetWingEffectiveness(2.5);
    SetCrossSections(_V(53.0, 186.9, 25.9));
    SetMaxWheelbrakeForce(MAX_WHEELBRAKE_FORCE);
    SetPMI(_V(15.5, 22.1, 7.7));

    SetDockParams(_V(0, -0.49, 10.076), _V(0, 0, 1), _V(0, 1, 0));
    SetGearParameters(1.0);  // NOTE: must init touchdown points here with gear DOWN!  This will be called again later by clbkPostCreation to init the "real" state from scenario file.
    EnableTransponder(true);
    SetTransponderChannel(193); // XPDR = 117.65 MHz

    // init APU runtime callout timestamp
    MarkAPUActive();  // reset the APU idle warning callout time

    // NEW for XR1: enable IDS so we transmit a docking signal
    DOCKHANDLE hDock = GetDockHandle(0);    // primary docking port
    EnableIDS(hDock, true);
    SetIDSChannel(hDock, 199);  // DOCK = 117.95 MHz

    // ******************** Attachment points **************************

    // top-center (for lifter attachment)
    // SET IN CONFIG FILE: CreateAttachment(true, _V(0,0,0), _V(0,-1,0), _V(0,0,1), "XS");

    // ******************** NAV radios **************************

    InitNavRadios(4);

    // ****************** propellant specs **********************

    // set tank configuration
    max_rocketfuel = TANK1_CAPACITY;
    max_scramfuel = TANK2_CAPACITY;

    // NOTE: Orbiter seems to reset the 'current fuel mass' value to zero later, since it expects the scenario file to be read
    // WARNING: do NOT init 'fuel mass' value (optional second argument) to > 0, because Orbiter will NOT set the tank value if the fraction is 
    // zero in the scenario file.
    ph_main = CreatePropellantResource(max_rocketfuel);       // main tank (fuel + oxydant)
    ph_rcs = CreatePropellantResource(RCS_FUEL_CAPACITY);    // RCS tank  (fuel + oxydant)
    ph_scram = CreatePropellantResource(max_scramfuel);        // scramjet fuel

    // **************** thruster definitions ********************

    // Reduction of thrust efficiency at normal ATM pressure
    double ispscale = GetISPScale();

    // increase level, srcrate, and lifetime
    PARTICLESTREAMSPEC contrail = {
        0, 11.0, 6, 150, 0.3, 7.5, 4, 3.0, PARTICLESTREAMSPEC::DIFFUSE,
            PARTICLESTREAMSPEC::LVL_PSQRT, 0, 2,
            PARTICLESTREAMSPEC::ATM_PLOG, 1e-4, 1
    };
    // increase level
    PARTICLESTREAMSPEC exhaust_main = {
        0, 3.0, 16, 150, 0.1, 0.2, 16, 1.0, PARTICLESTREAMSPEC::EMISSIVE,
            PARTICLESTREAMSPEC::LVL_SQRT, 0, 1,
            PARTICLESTREAMSPEC::ATM_PLOG, 1e-5, 0.1
    };
    // increase level
    PARTICLESTREAMSPEC exhaust_hover = {
        0, 2.0, 20, 150, 0.1, 0.15, 16, 1.0, PARTICLESTREAMSPEC::EMISSIVE,
            PARTICLESTREAMSPEC::LVL_SQRT, 0, 1,
            PARTICLESTREAMSPEC::ATM_PLOG, 1e-5, 0.1
    };
    // increase level and particle lifetime
    PARTICLESTREAMSPEC exhaust_scram = {
        0, 3.0, 25, 150, 0.05, 15.0, 3, 1.0, PARTICLESTREAMSPEC::EMISSIVE,
            PARTICLESTREAMSPEC::LVL_SQRT, 0, 1,
            PARTICLESTREAMSPEC::ATM_PLOG, 1e-5, 0.1
    };

    // handle new configurable ISP
    const double mainISP = GetXR1Config()->GetMainISP();

    // main thrusters
    th_main[0] = CreateThruster(_V(-1, 0.0, -7.7), _V(0, 0, 1), MAX_MAIN_THRUST[GetXR1Config()->MainEngineThrust], ph_main, mainISP, mainISP * ispscale);
    th_main[1] = CreateThruster(_V(1, 0.0, -7.7), _V(0, 0, 1), MAX_MAIN_THRUST[GetXR1Config()->MainEngineThrust], ph_main, mainISP, mainISP * ispscale);

    thg_main = CreateThrusterGroup(th_main, 2, THGROUP_MAIN);
    // increase thruster flame: stock was 12, 1
    SURFHANDLE mainExhaustTex = oapiRegisterExhaustTexture("dg-xr1\\ExhaustXR1");
    // Pre-1.9 release: length was 12
    AddXRExhaust(th_main[0], 10, 0.811, mainExhaustTex);
    AddXRExhaust(th_main[1], 10, 0.811, mainExhaustTex);

    // move exhaust smoke away from engines a bit
    // pre-1.9 release: const double mainDelta = -3;
    const double mainDelta = -1.5;
    AddExhaustStream(th_main[0], _V(-1, 0, -15 + mainDelta), &contrail);
    AddExhaustStream(th_main[1], _V(1, 0, -15 + mainDelta), &contrail);
    AddExhaustStream(th_main[0], _V(-1, 0, -10 + mainDelta), &exhaust_main);
    AddExhaustStream(th_main[1], _V(1, 0, -10 + mainDelta), &exhaust_main);

    // retro thrusters
    th_retro[0] = CreateThruster(_V(-3, 0, 5.3), _V(0, 0, -1.0), MAX_RETRO_THRUST, ph_main, mainISP, mainISP * ispscale);
    th_retro[1] = CreateThruster(_V(3, 0, 5.3), _V(0, 0, -1.0), MAX_RETRO_THRUST, ph_main, mainISP, mainISP * ispscale);
    thg_retro = CreateThrusterGroup(th_retro, 2, THGROUP_RETRO);
    AddXRExhaust(th_retro[0], 1.5, 0.16, _V(-3, -0.300, 5.3), _V(0, 0, 1.0), mainExhaustTex);
    AddXRExhaust(th_retro[1], 1.5, 0.16, _V(3, -0.300, 5.3), _V(0, 0, 1.0), mainExhaustTex);

    // hover thrusters (simplified)
    // The two aft hover engines are combined into a single "logical" thruster,
    // but exhaust is rendered separately for both
    th_hover[0] = CreateThruster(_V(0, 0, 3), _V(0, 1, 0), MAX_HOVER_THRUST[GetXR1Config()->HoverEngineThrust], ph_main, mainISP, mainISP * ispscale);
    th_hover[1] = CreateThruster(_V(0, 0, -3), _V(0, 1, 0), MAX_HOVER_THRUST[GetXR1Config()->HoverEngineThrust], ph_main, mainISP, mainISP * ispscale);
    thg_hover = CreateThrusterGroup(th_hover, 2, THGROUP_HOVER);
    // pre-1.9 version: increase thruster flame: was length 6
    AddXRExhaust(th_hover[0], 4.75, 0.5, _V(0, -1.6, 3), _V(0, -1, 0), mainExhaustTex);
    AddXRExhaust(th_hover[1], 4.75, 0.5, _V(-3, -1.3, -4.55), _V(0, -1, 0), mainExhaustTex);
    AddXRExhaust(th_hover[1], 4.75, 0.5, _V(3, -1.3, -4.55), _V(0, -1, 0), mainExhaustTex);

    // move exhaust smoke away from engines a bit
    // pre-1.9 version: const double hoverDelta = -3;
    const double hoverDelta = -1.5;
    AddExhaustStream(th_hover[0], _V(0, -4 + hoverDelta, 0), &contrail);
    AddExhaustStream(th_hover[0], _V(0, -2 + hoverDelta, 3), &exhaust_hover);
    AddExhaustStream(th_hover[0], _V(-3, -2 + hoverDelta, -4.55), &exhaust_hover);
    AddExhaustStream(th_hover[0], _V(3, -2 + hoverDelta, -4.55), &exhaust_hover);

    // set of attitude thrusters (idealised). The arrangement is such that no angular
    // momentum is created in linear mode, and no linear momentum is created in rotational mode.
    SURFHANDLE rcsExhaustTex = mainExhaustTex;
    THRUSTER_HANDLE th_att_rot[4], th_att_lin[4];
    // NOTE: save in th_rcs array so we can disable them later
    th_rcs[0] = th_att_rot[0] = th_att_lin[0] = CreateThruster(_V(0, 0, 8), _V(0, 1, 0), GetRCSThrustMax(0), ph_rcs, mainISP);
    th_rcs[1] = th_att_rot[1] = th_att_lin[3] = CreateThruster(_V(0, 0, -8), _V(0, -1, 0), GetRCSThrustMax(1), ph_rcs, mainISP);
    th_rcs[2] = th_att_rot[2] = th_att_lin[2] = CreateThruster(_V(0, 0, 8), _V(0, -1, 0), GetRCSThrustMax(2), ph_rcs, mainISP);
    th_rcs[3] = th_att_rot[3] = th_att_lin[1] = CreateThruster(_V(0, 0, -8), _V(0, 1, 0), GetRCSThrustMax(3), ph_rcs, mainISP);
    CreateThrusterGroup(th_att_rot, 2, THGROUP_ATT_PITCHUP);
    CreateThrusterGroup(th_att_rot + 2, 2, THGROUP_ATT_PITCHDOWN);
    CreateThrusterGroup(th_att_lin, 2, THGROUP_ATT_UP);
    CreateThrusterGroup(th_att_lin + 2, 2, THGROUP_ATT_DOWN);
    AddXRExhaust(th_att_rot[0], 0.6, 0.078, _V(-0.75, -0.7, 9.65), _V(0, -1, 0), rcsExhaustTex);
    AddXRExhaust(th_att_rot[0], 0.6, 0.078, _V(0.75, -0.7, 9.65), _V(0, -1, 0), rcsExhaustTex);
    AddXRExhaust(th_att_rot[1], 0.79, 0.103, _V(-0.1, 0.55, -7.3), _V(0, 1, 0), rcsExhaustTex);
    AddXRExhaust(th_att_rot[1], 0.79, 0.103, _V(0.1, 0.55, -7.3), _V(0, 1, 0), rcsExhaustTex);
    AddXRExhaust(th_att_rot[2], 0.6, 0.078, _V(-0.8, -0.25, 9.6), _V(0, 1, 0), rcsExhaustTex);
    AddXRExhaust(th_att_rot[2], 0.6, 0.078, _V(0.8, -0.25, 9.6), _V(0, 1, 0), rcsExhaustTex);
    AddXRExhaust(th_att_rot[3], 0.79, 0.103, _V(-0.1, -0.55, -7.3), _V(0, -1, 0), rcsExhaustTex);
    AddXRExhaust(th_att_rot[3], 0.79, 0.103, _V(0.1, -0.55, -7.3), _V(0, -1, 0), rcsExhaustTex);

    th_rcs[4] = th_att_rot[0] = th_att_lin[0] = CreateThruster(_V(0, 0, 6), _V(-1, 0, 0), GetRCSThrustMax(4), ph_rcs, mainISP);
    th_rcs[5] = th_att_rot[1] = th_att_lin[3] = CreateThruster(_V(0, 0, -6), _V(1, 0, 0), GetRCSThrustMax(5), ph_rcs, mainISP);
    th_rcs[6] = th_att_rot[2] = th_att_lin[2] = CreateThruster(_V(0, 0, 6), _V(1, 0, 0), GetRCSThrustMax(6), ph_rcs, mainISP);
    th_rcs[7] = th_att_rot[3] = th_att_lin[1] = CreateThruster(_V(0, 0, -6), _V(-1, 0, 0), GetRCSThrustMax(7), ph_rcs, mainISP);
    CreateThrusterGroup(th_att_rot, 2, THGROUP_ATT_YAWLEFT);
    CreateThrusterGroup(th_att_rot + 2, 2, THGROUP_ATT_YAWRIGHT);
    CreateThrusterGroup(th_att_lin, 2, THGROUP_ATT_LEFT);
    CreateThrusterGroup(th_att_lin + 2, 2, THGROUP_ATT_RIGHT);
    AddXRExhaust(th_att_rot[0], 0.6, 0.078, _V(1.0, -0.48, 9.35), _V(1, 0, 0), rcsExhaustTex);
    AddXRExhaust(th_att_rot[1], 0.94, 0.122, _V(-2.2, 0.2, -6.0), _V(-1, 0, 0), rcsExhaustTex);
    AddXRExhaust(th_att_rot[2], 0.6, 0.078, _V(-1.0, -0.48, 9.35), _V(-1, 0, 0), rcsExhaustTex);
    AddXRExhaust(th_att_rot[3], 0.94, 0.122, _V(2.2, 0.2, -6.0), _V(1, 0, 0), rcsExhaustTex);

    th_rcs[8] = th_att_rot[0] = CreateThruster(_V(6, 0, 0), _V(0, 1, 0), GetRCSThrustMax(8), ph_rcs, mainISP);
    th_rcs[9] = th_att_rot[1] = CreateThruster(_V(-6, 0, 0), _V(0, -1, 0), GetRCSThrustMax(9), ph_rcs, mainISP);
    th_rcs[10] = th_att_rot[2] = CreateThruster(_V(-6, 0, 0), _V(0, 1, 0), GetRCSThrustMax(10), ph_rcs, mainISP);
    th_rcs[11] = th_att_rot[3] = CreateThruster(_V(6, 0, 0), _V(0, -1, 0), GetRCSThrustMax(11), ph_rcs, mainISP);
    CreateThrusterGroup(th_att_rot, 2, THGROUP_ATT_BANKLEFT);
    CreateThrusterGroup(th_att_rot + 2, 2, THGROUP_ATT_BANKRIGHT);
    AddXRExhaust(th_att_rot[0], 1.03, 0.134, _V(-5.1, 0.2, 0.4), _V(0, 1, 0), rcsExhaustTex);
    AddXRExhaust(th_att_rot[1], 1.03, 0.134, _V(5.1, -0.8, 0.4), _V(0, -1, 0), rcsExhaustTex);
    AddXRExhaust(th_att_rot[2], 1.03, 0.134, _V(5.1, 0.2, 0.4), _V(0, 1, 0), rcsExhaustTex);
    AddXRExhaust(th_att_rot[3], 1.03, 0.134, _V(-5.1, -0.8, 0.4), _V(0, -1, 0), rcsExhaustTex);

    th_rcs[12] = th_att_lin[0] = CreateThruster(_V(0, 0, -7), _V(0, 0, 1), GetRCSThrustMax(12), ph_rcs, mainISP);
    th_rcs[13] = th_att_lin[1] = CreateThruster(_V(0, 0, 7), _V(0, 0, -1), GetRCSThrustMax(13), ph_rcs, mainISP);
    CreateThrusterGroup(th_att_lin, 1, THGROUP_ATT_FORWARD);
    CreateThrusterGroup(th_att_lin + 1, 1, THGROUP_ATT_BACK);
    AddXRExhaust(th_att_lin[0], 0.6, 0.078, _V(0, -0.2, -7.6), _V(0, 0, -1), rcsExhaustTex);
    AddXRExhaust(th_att_lin[0], 0.6, 0.078, _V(0, 0.22, -7.6), _V(0, 0, -1), rcsExhaustTex);
    AddXRExhaust(th_att_lin[1], 0.6, 0.078, _V(-0.82, -0.49, 9.8), _V(0, 0, 1), rcsExhaustTex);
    AddXRExhaust(th_att_lin[1], 0.6, 0.078, _V(0.82, -0.49, 9.8), _V(0, 0, 1), rcsExhaustTex);

    // **************** scramjet definitions ********************

    VECTOR3 dir = { 0.0, sin(SCRAM_DEFAULT_DIR), cos(SCRAM_DEFAULT_DIR) };

    const double scramX = 0.9;  // distance from centerline
    for (int i = 0; i < 2; i++)
    {
        th_scram[i] = CreateThruster(_V(i ? scramX : -scramX, 0, -5.6), dir, 0, ph_scram, 0);
        ramjet->AddThrusterDefinition(th_scram[i], SCRAM_FHV[GetXR1Config()->SCRAMfhv],
            SCRAM_INTAKE_AREA, SCRAM_INTERNAL_TEMAX, GetXR1Config()->GetScramMaxEffectiveDMF());
    }

    // thrust rating and ISP for scramjet engines are updated continuously
    // move exhaust smoke away from engines a bit
    const double scramDelta = -3;
    PSTREAM_HANDLE ph;
    ph = AddExhaustStream(th_scram[0], _V(-1, -1.1, -5.4 + scramDelta), &exhaust_scram);
    // Note: ph will be null if exhaust streams are disabled
    if (ph != nullptr)
        oapiParticleSetLevelRef(ph, scram_intensity + 0);

    ph = AddExhaustStream(th_scram[1], _V(1, -1.1, -5.4 + scramDelta), &exhaust_scram);
    if (ph != nullptr)
        oapiParticleSetLevelRef(ph, scram_intensity + 1);

    // ********************* aerodynamics ***********************

    // NOTE: org values were causing nasty downward pitch in the atmospehere: 
    hwing = CreateAirfoil3(LIFT_VERTICAL, _V(m_wingBalance, 0, m_centerOfLift), DeltaGliderXR1::VLiftCoeff, nullptr, 5, WING_AREA, WING_ASPECT_RATIO);

    ReinitializeDamageableControlSurfaces();  // create ailerons, elevators, and elevator trim

    // vertical stabiliser and body lift and drag components
    CreateAirfoil3(LIFT_HORIZONTAL, _V(0, 0, -4), DeltaGliderXR1::HLiftCoeff, nullptr, 5, 15, 1.5);
    CreateControlSurface(AIRCTRL_RUDDER, 0.8, 1.5, _V(0, 0, -7.2), AIRCTRL_AXIS_YPOS, anim_rudder);

    // Create a hidden elevator trim to fix the nose-up tendency on liftoff and allow the elevator trim to be truly neutral.
    // We have to use FLAP here because that is the only unused control surface type.  We could probably also duplicate this via CreateAirfoil3, but this
    // is easier to adjust and test.
    CreateControlSurface(AIRCTRL_FLAP, 0.3, 1.5, _V(0, 0, -7.2), AIRCTRL_AXIS_XPOS);  // no animation for this!
    m_hiddenElevatorTrimState = HIDDEN_ELEVATOR_TRIM_STATE;        // set to a member variable in case we want to change it in flight later during testing

    // we need these goofy const casts to force the linker to link with the 'const double *' version of CreateVariableDragElement instead of the legacy 'double *' version of the function (which still exists but is deprecated and causes warnings in orbiter.log)
    CreateVariableDragElement(const_cast<const double*>(&gear_proc), 0.8, _V(0, -1, 0));		// landing gear
    CreateVariableDragElement(const_cast<const double*>(&rcover_proc), 0.2, _V(0, -0.5, 6.5)); // retro covers
    CreateVariableDragElement(const_cast<const double*>(&nose_proc), 3, _V(0, 0, 8));			// nose cone
    CreateVariableDragElement(const_cast<const double*>(&radiator_proc), 1, _V(0, 1.5, -4));   // radiator
    CreateVariableDragElement(const_cast<const double*>(&brake_proc), 4, _V(0, 0, -8));        // airbrake

    SetRotDrag(_V(0.10, 0.13, 0.04));

    // define hull temperature limits
    m_hullTemperatureLimits.noseCone = CTOK(2840);
    m_hullTemperatureLimits.wings = CTOK(2380);
    m_hullTemperatureLimits.cockpit = CTOK(1490);
    m_hullTemperatureLimits.topHull = CTOK(1210);
    m_hullTemperatureLimits.warningFrac = 0.80;   // yellow text
    m_hullTemperatureLimits.criticalFrac = 0.90;   // red text
    m_hullTemperatureLimits.doorOpenWarning = 0.75;
    // aluminum melts @ 660C and begins deforming below that
    m_hullTemperatureLimits.doorOpen = CTOK(480);

    // default to full LOX INTERNAL tank if not loaded from save file 
    if (m_loxQty < 0)
        m_loxQty = GetXR1Config()->GetMaxLoxMass();

    // angular damping

    // ************************* mesh ***************************

    // ********************* beacon lights **********************
    static VECTOR3 beaconpos[8] = { {-8.6,0,-3.3}, {8.6,0,-3.3}, {0,0.5,-7.5}, {0,2.2,2}, {0,-1.8,2}, {-8.9,2.5,-5.4}, {8.9,2.5,-5.4} };
    static VECTOR3 beaconcol[7] = { {1.0,0.5,0.5}, {0.5,1.0,0.5}, {1,1,1}, {1,0.6,0.6}, {1,0.6,0.6}, {1,1,1}, {1,1,1} };
    for (int i = 0; i < 7; i++)
    {
        beacon[i].shape = (i < 3 ? BEACONSHAPE_DIFFUSE : BEACONSHAPE_STAR);
        beacon[i].pos = beaconpos + i;
        beacon[i].col = beaconcol + i;
        beacon[i].size = (i < 3 ? 0.3 : 0.55);
        beacon[i].falloff = (i < 3 ? 0.4 : 0.6);
        beacon[i].period = (i < 3 ? 0 : i < 5 ? 2 : 1.13);
        beacon[i].duration = (i < 5 ? 0.1 : 0.05);
        beacon[i].tofs = (6 - i) * 0.2;
        beacon[i].active = false;
        AddBeacon(beacon + i);
    }

    // light colors
    COLOUR4 col_d = { 0.9f, 0.8f, 1.0f, 0.0f };  // diffuse
    COLOUR4 col_s = { 1.9f, 0.8f, 1.0f ,0.0f };  // specular
    COLOUR4 col_a = { 0.0f, 0.0f, 0.0f ,0.0f };  // ambient (black)
    COLOUR4 col_white = { 1.0f, 1.0f, 1.0f, 0.0f };  // white

    // add a single light at the main engines since they are clustered together
    const double mainEnginePointLightPower = 100;
    const double zMainLightDelta = -1.0;
    if (GetXR1Config()->EnableEngineLightingEffects)
    {
        LightEmitter* leMain = AddPointLight(_V(0, 0, -10 + zMainLightDelta), mainEnginePointLightPower * 2, 1e-3, 0, 2e-3, col_d, col_s, col_a);
        leMain->SetIntensityRef(&m_mainThrusterLightLevel);
    }

    // add a light at each hover engine
    if (GetXR1Config()->EnableEngineLightingEffects)
    {
        const double hoverEnginePointLightPower = mainEnginePointLightPower * 0.6875;  // hovers are .6875 the thrust of the mains
        const double yHoverLightDelta = -1.0;
        LightEmitter* leForward = AddPointLight(_V(0, -1.6 + yHoverLightDelta, 3.00), hoverEnginePointLightPower, 1e-3, 0, 2e-3, col_d, col_s, col_a);
        LightEmitter* leAftPort = AddPointLight(_V(3, -1.6 + yHoverLightDelta, -4.55), hoverEnginePointLightPower, 1e-3, 0, 2e-3, col_d, col_s, col_a);
        LightEmitter* leAftStarboard = AddPointLight(_V(-3, -1.6 + yHoverLightDelta, -4.55), hoverEnginePointLightPower, 1e-3, 0, 2e-3, col_d, col_s, col_a);
        leForward->SetIntensityRef(&m_hoverThrusterLightLevel);
        leAftPort->SetIntensityRef(&m_hoverThrusterLightLevel);
        leAftStarboard->SetIntensityRef(&m_hoverThrusterLightLevel);
    }

    // add docking lights (our only 2 spotlights for now)
    m_pSpotlights[0] = static_cast<SpotLight*>(AddSpotLight(_V(2.5, -0.5, 6.5), _V(0, 0, 1), 150, 1e-3, 0, 1e-3, RAD * 25, RAD * 60, col_white, col_white, col_a));
    m_pSpotlights[1] = static_cast<SpotLight*>(AddSpotLight(_V(-2.5, -0.5, 6.5), _V(0, 0, 1), 150, 1e-3, 0, 1e-3, RAD * 25, RAD * 60, col_white, col_white, col_a));

    // turn all spotlights off by default
    for (int i = 0; i < SPOTLIGHT_COUNT; i++)
        m_pSpotlights[i]->Activate(false);

    // load meshes
    vcmesh_tpl = oapiLoadMeshGlobal("dg-xr1\\deltaglidercockpit-xr1");  // VC mesh
    exmesh_tpl = oapiLoadMeshGlobal("dg-xr1\\deltaglider-xr1");         // exterior mesh
    SetMeshVisibilityMode(AddMesh(exmesh_tpl), MESHVIS_EXTERNAL);
    SetMeshVisibilityMode(AddMesh(vcmesh_tpl), MESHVIS_VC);

    // **************** vessel-specific insignia ****************

    /* NO UGLY LOGOS!
    insignia_tex = oapiCreateTextureSurface (256, 256);
    SURFHANDLE hTex = oapiGetTextureHandle (exmesh_tpl, 5);
    if (hTex)
        DeltaGliderXR1::SafeBlt (insignia_tex, hTex, 0, 0, 0, 0, 256, 256);
    */

#ifdef MMU
    ///////////////////////////////////////////////////////////////////////
    // Init UMmu
    ///////////////////////////////////////////////////////////////////////
    const int ummuStatus = UMmu.InitUmmu(GetHandle());  // returns 1 if ok and other number if not

    // RC4 AND NEWER: UMmu is REQUIRED!
    if (ummuStatus != 1)
        FatalError("UMmu not installed!  You must install Universal Mmu 3.0 or newer in order to use the XR1; visit http://www.alteaaerospace.com for more information.");

    // validate UMmu version and write it to the log file
    const float ummuVersion = CONST_UMMU(this).GetUserUMmuVersion();
    if (ummuVersion < 3.0)
    {
        char msg[256];
        sprintf(msg, "UMmu version %.2f is installed, but the XR1 requires Universal Mmu 3.0 or higher; visit http://www.alteaaerospace.com for more information.", ummuVersion);
        FatalError(msg);
    }

    char msg[256];
    sprintf(msg, "Using UMmu Version: %.2f", ummuVersion);
    GetXR1Config()->WriteLog(msg);

    //                      state,MinX, MaxX,  MinY, MaxY, MinZ,MaxZ
    UMmu.DefineAirLockShape(1, -0.66f, 0.66f, -1.65f, 0.20f, 8.0f, 11.0f);
    UMmu.SetCrewWeightUpdateShipWeightAutomatically(FALSE);  // we handle crew member weight ourselves
    // WARNING: default of 11.0 meters only works in space; on ground the astronaut reenters the ship immediately.
    VECTOR3 pos = { 0.0, 0.5, 12.5 }; // this is the position where the Mmu will appear relative to your ship's local coordinate (have to spawn at +0.5 meter Y because otherwise the crew member will be bounced into space if the ship is landed on ground)
    VECTOR3 rot = { 0.0, 0.0, 0.0 };  // straight up
    UMmu.SetMembersPosRotOnEVA(pos, rot);
    UMmu.SetEjectPosRotRelSpeed(pos, rot, _V(0, 4.0, 0));  // jumped UP to bail out @ 4 meters-per-second
    UMmu.SetMaxSeatAvailableInShip(MAX_PASSENGERS); // includes the pilot
#endif

    // there is only one active airlock, so initialize it now
    m_pActiveAirlockDoorStatus = &olock_status;

    //
    // Initialize and cache all instrument panels
    //

    // 1920-pixel-wide panels
    AddInstrumentPanel(new XR1MainInstrumentPanel1920(*this), 1920);
    AddInstrumentPanel(new XR1UpperInstrumentPanel1920(*this), 1920);
    AddInstrumentPanel(new XR1LowerInstrumentPanel1920(*this), 1920);

    // 1600-pixel-wide panels
    AddInstrumentPanel(new XR1MainInstrumentPanel1600(*this), 1600);
    AddInstrumentPanel(new XR1UpperInstrumentPanel1600(*this), 1600);
    AddInstrumentPanel(new XR1LowerInstrumentPanel1600(*this), 1600);

    // 1280-pixel-wide panels
    AddInstrumentPanel(new XR1MainInstrumentPanel1280(*this), 1280);
    AddInstrumentPanel(new XR1UpperInstrumentPanel1280(*this), 1280);
    AddInstrumentPanel(new XR1LowerInstrumentPanel1280(*this), 1280);

    // add our VC panels (panel width MUST be zero for these!)
    AddInstrumentPanel(new XR1VCPilotInstrumentPanel(*this, PANELVC_PILOT), 0);
    AddInstrumentPanel(new XR1VCPassenger1InstrumentPanel(*this, PANELVC_PSNGR1), 0);
    AddInstrumentPanel(new XR1VCPassenger2InstrumentPanel(*this, PANELVC_PSNGR2), 0);
    AddInstrumentPanel(new XR1VCPassenger3InstrumentPanel(*this, PANELVC_PSNGR3), 0);
    AddInstrumentPanel(new XR1VCPassenger4InstrumentPanel(*this, PANELVC_PSNGR4), 0);

    // NOTE: default crew data is set AFTER the scenario file is parsed
}

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

// --------------------------------------------------------------
// Finalise vessel creation
// --------------------------------------------------------------
void DeltaGliderXR1::clbkPostCreation()
{
    // Invoke XR PostCreation code common to all XR vessels (code is in XRVessel.cpp)
    clbkPostCreationCommonXRCode();

    EnableRetroThrusters(rcover_status == DoorStatus::DOOR_OPEN);
    EnableHoverEngines(hoverdoor_status == DoorStatus::DOOR_OPEN);
    EnableScramEngines(scramdoor_status == DoorStatus::DOOR_OPEN);

    // set initial animation states
    SetXRAnimation(anim_gear, gear_proc);
    SetXRAnimation(anim_rcover, rcover_proc);
    SetXRAnimation(anim_hoverdoor, hoverdoor_proc);
    SetXRAnimation(anim_scramdoor, scramdoor_proc);
    SetXRAnimation(anim_nose, nose_proc);
    SetXRAnimation(anim_ladder, ladder_proc);
    SetXRAnimation(anim_olock, olock_proc);
    SetXRAnimation(anim_ilock, ilock_proc);
    SetXRAnimation(anim_hatch, hatch_proc);
    SetXRAnimation(anim_radiator, radiator_proc);
    SetXRAnimation(anim_brake, brake_proc);
    SetXRAnimation(anim_gearlever, static_cast<int>(gear_status) & 1);
    SetXRAnimation(anim_nconelever, static_cast<int>(nose_status) & 1);
    SetXRAnimation(anim_olockswitch, static_cast<int>(olock_status) & 1);
    SetXRAnimation(anim_ilockswitch, static_cast<int>(ilock_status) & 1);
    SetXRAnimation(anim_retroswitch, static_cast<int>(rcover_status) & 1);
    SetXRAnimation(anim_radiatorswitch, static_cast<int>(radiator_status) & 1);
    SetXRAnimation(anim_hatchswitch, static_cast<int>(hatch_status) & 1);
    SetXRAnimation(anim_ladderswitch, static_cast<int>(ladder_status) & 1);

    // NOTE: instrument panel initialization moved to clbkSetClassCaps (earlier) because the Post-2010-P1 Orbiter Beta invokes clbkLoadPanel before invoking clbkPostCreation

    // add our PreStep objects; these are invoked in order
    AddPreStep(new AttitudeHoldPreStep(*this));
    AddPreStep(new DescentHoldPreStep(*this));
    AddPreStep(new AirspeedHoldPreStep(*this));
    AddPreStep(new ScramjetSoundPreStep(*this));
    AddPreStep(new MmuPreStep(*this));
    AddPreStep(new GearCalloutsPreStep(*this));
    AddPreStep(new MachCalloutsPreStep(*this));
    AddPreStep(new AltitudeCalloutsPreStep(*this));
    AddPreStep(new DockingCalloutsPreStep(*this));
    AddPreStep(new TakeoffAndLandingCalloutsAndCrashPreStep(*this));
    AddPreStep(new NosewheelSteeringPreStep(*this));
    AddPreStep(new UpdateVesselLightsPreStep(*this));
    AddPreStep(new ParkingBrakePreStep(*this));

    // WARNING: this must be invoked LAST in the prestep sequence so that behavior is consistent across all pre-step methods
    AddPreStep(new UpdatePreviousFieldsPreStep(*this));

    // add our PostStep objects; these are invoked in order
    AddPostStep(new PreventAutoRefuelPostStep(*this));   // add this FIRST before our fuel callouts
    AddPostStep(new ComputeAccPostStep(*this));   // used by acc areas; computed only once per frame for efficiency
    // XRSound: AddPostStep(new AmbientSoundsPostStep       (*this));
    AddPostStep(new ShowWarningPostStep(*this));
    AddPostStep(new SetHullTempsPostStep(*this));
    AddPostStep(new SetSlopePostStep(*this));
    AddPostStep(new DoorSoundsPostStep(*this));
    AddPostStep(new FuelCalloutsPostStep(*this));
    AddPostStep(new UpdateIntervalTimersPostStep(*this));
    AddPostStep(new APUPostStep(*this));
    AddPostStep(new UpdateMassPostStep(*this));
    AddPostStep(new DisableControlSurfForAPUPostStep(*this));
    AddPostStep(new OneShotInitializationPostStep(*this));
    AddPostStep(new AnimationPostStep(*this));
    AddPostStep(new FuelDumpPostStep(*this));
    AddPostStep(new XFeedPostStep(*this));
    AddPostStep(new ResupplyPostStep(*this));
    AddPostStep(new LOXConsumptionPostStep(*this));
    AddPostStep(new UpdateCoolantTempPostStep(*this));
    AddPostStep(new AirlockDecompressionPostStep(*this));
    AddPostStep(new AutoCenteringSimpleButtonAreasPostStep(*this));  // logic for all auto-centering button areas
    AddPostStep(new ResetAPUTimerForPolledSystemsPostStep(*this));
    AddPostStep(new ManageMWSPostStep(*this));
#ifdef _DEBUG
    AddPostStep(new TestXRVesselCtrlPostStep(*this));      // for manual testing of new XRVesselCtrl methods via the debugger
#endif

    // set hidden elevator trim level
    SetControlSurfaceLevel(AIRCTRL_FLAP, m_hiddenElevatorTrimState);
}

// --------------------------------------------------------------
// Contains clbkPostCreation code common to all XR vessels; invoked immediately after InitSound()
// in clbkPostCreation() from all XR subclasses.
// --------------------------------------------------------------
void DeltaGliderXR1::clbkPostCreationCommonXRCode()
{
    // initialize XRSound
    InitSound();

    SetGearParameters(gear_proc);

    SetEmptyMass();     // update mass for passengers, APU fuel, O2, etc.

    // set default crew members if no UMmu crew data loaded from scenario file
    if (m_mmuCrewDataValid == false)   // scenario file not saved with UMmu data?
    {
        // UMMU BUG: DOESN'T WORK!  RemoveAllUMmuCrewMembers();  // necessary in case some UMMU data is in the scenario file that we want to ignore here

        // set DEFAULT crew member data since this scenario file is old
        for (int i = 0; i < GetXR1Config()->DefaultCrewComplement; i++)
        {
            // can't use const here because of UMmu bug where it uses 'char *' instead of 'const char *'
            CrewMember* pCM = GetXR1Config()->CrewMembers + i;

            // set miscID hash string: "XI0" ..."XIn" equates to : rank="Commander", mesh="dg-xr1\EVAM1"
            char misc[5];   // size matches UMmu misc field size
            sprintf(misc, "XI%d", i);
#ifdef MMU
            UMmu.AddCrewMember(pCM->name, pCM->age, pCM->pulse, pCM->mass, misc);
#endif
        }
    }

    // ENHANCEMENT: init correct defaults if no scenario file loaded
    if (m_parsedScenarioFile == false)
    {
        // no scenario file parsed!  Set all INTERNAL tanks to 100%.  Do not set EXTERNAL tanks.
        SetPropellantMass(ph_main, TANK1_CAPACITY);
        SetPropellantMass(ph_scram, TANK2_CAPACITY);
        SetPropellantMass(ph_rcs, RCS_FUEL_CAPACITY);

        // must init startup fuel fractions as well (internal tanks only)
        m_startupMainFuelFrac = GetPropellantMass(ph_main) / GetPropellantMaxMass(ph_main);
        m_startupSCRAMFuelFrac = GetPropellantMass(ph_scram) / GetPropellantMaxMass(ph_scram);
        m_startupRCSFuelFrac = GetPropellantMass(ph_rcs) / GetPropellantMaxMass(ph_rcs);

        // APU on
        ActivateAPU(DoorStatus::DOOR_OPENING);

        // RCS off
        SetAttitudeMode(RCS_NONE);

        // Workaround for Orbiter core bug: must init gear parameters here in case gear status not present in the scenario file.
        // This is necessary because Orbiter requires the gear to be DOWN when the scenario first loads if the ship is landed; otherwise, a gruesome crash 
        // occurs due to the "bounce bug".
        gear_status = DoorStatus::DOOR_CLOSED;
        gear_proc = 0.0;
    }

    // update main fuel ISP if CONFIG_OVERRIDE_MainFuelISP is set
    if (m_configOverrideBitmask & CONFIG_OVERRIDE_MainFuelISP)
    {
        const double mainISP = GetXR1Config()->GetMainISP();  // this was updated from the override value in the scenario file
        SetThrusterIsp(th_main[0], mainISP, GetISPScale());
        SetThrusterIsp(th_main[1], mainISP, GetISPScale());

        SetThrusterIsp(th_retro[0], mainISP, GetISPScale());
        SetThrusterIsp(th_retro[1], mainISP, GetISPScale());

        SetThrusterIsp(th_hover[0], mainISP, GetISPScale());
        SetThrusterIsp(th_hover[1], mainISP, GetISPScale());

        for (int i = 0; i < 14; i++)
            SetThrusterIsp(th_rcs[i], mainISP, GetISPScale());
    }

    // log a tertiary HUD message if an override config file was loaded
    static char msg[256];
    if ((*GetXR1Config()->GetOverrideFilename() != 0) && (!GetXR1Config()->ParseFailed()))  // any override set && load succeeded?
    {
        sprintf(msg, "Loaded configuration override file&'%s'.", GetXR1Config()->GetOverrideFilename());
        ShowInfo(nullptr, ST_None, msg);
    }

    // log a tertiary HUD message if any scenario overrides found
    if (m_configOverrideBitmask != 0)
    {
        // count the number of '1' bits in the override bitmask
        sprintf(msg, "Loaded %d configuration override(s)&from scenario file.", CountOneBits(m_configOverrideBitmask));
        ShowInfo(nullptr, ST_None, msg);
    }

    // warn the user if parsing failed
    if (GetXR1Config()->ParseFailed())
    {
        sprintf(msg, "Error parsing configuration file(s)&'%s'.", GetXR1Config()->GetConfigFilenames());
        ShowWarning("Warning Conditions Detected.wav", ST_WarningCallout, msg);
    }
    else if ((GetXR1Config()->GetCheatcodesFoundCount() > 0) && (!GetXR1Config()->CheatcodesEnabled))  // warn the user if at least one cheatcode was set but then disabled by config
    {
        sprintf(msg, "%d cheatcode(s) ignored; cheatcodes are&disabled via the configuration file(s).", GetXR1Config()->GetCheatcodesFoundCount());
        ShowWarning(nullptr, ST_None, msg);
    }
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
