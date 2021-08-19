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
// XR5Vanguard implementation class
//
// XR5Vanguard.cpp
// ==============================================================

#include "XR5Vanguard.h"

#include "XR5InstrumentPanels.h"
#include "XR1PreSteps.h"
#include "XR1PostSteps.h"
#include "XR1FuelPostSteps.h"
#include "XR1AnimationPoststep.h"
#include "XR5PreSteps.h"
#include "XR5PostSteps.h"
#include "XRPayload.h"
#include "XR5PayloadBay.h"

// --------------------------------------------------------------
// Set vessel class parameters
// --------------------------------------------------------------
void XR5Vanguard::clbkSetClassCaps(FILEHANDLE cfg)
{
    // parse the configuration file
    // If parse fails, we shouldn't display a MessageBox here because the Orbiter main window 
    // keeps putting itself in the foreground, covering it up and making Orbiter look like it's hung.
    // Therefore, TakeoffAndLandingCalloutsAndCrashPostStep will blink a warning message for us 
    // if the parse fails.
    ParseXRConfigFile();   // common XR code

    // Note: this must be invoked here instead of the constructor so the we may override it!
    DefineAnimations();

    // define our payload bay and attachment points
    CreatePayloadBay();

    // *************** physical parameters **********************

    ramjet = new XR1Ramjet(this);

    VESSEL2::SetEmptyMass(EMPTY_MASS);
    SetSize(38.335);    // 1/2 ship's width
    SetVisibilityLimit(7.5e-4, 1.5e-3);
    SetAlbedoRGB(_V(0.13, 0.20, 0.77));   // bluish
    SetGravityGradientDamping(20.0);    // ? same as DG for now

    SetCrossSections(_V(543.82, 1962.75, 330.97));

    SetMaxWheelbrakeForce(MAX_WHEELBRAKE_FORCE);

    SetPMI(_V(317.35, 305.08, 219.45));

    SetDockParams(DOCKING_PORT_COORD, _V(0, 1, 0), _V(0, 0, -1));   // top-mounted port

    SetGearParameters(1.0);  // NOTE: must init touchdown points here with gear DOWN!  This will be called again later by clbkPostCreation to init the "real" state from scenario file.

    EnableTransponder(true);
    SetTransponderChannel(195); // XPDR = 117.75 MHz

    // init APU runtime callout timestamp
    MarkAPUActive();  // reset the APU idle warning callout time

    // enable IDS so we transmit a docking signal
    DOCKHANDLE hDock = GetDockHandle(0);    // primary docking port
    EnableIDS(hDock, true);
    SetIDSChannel(hDock, 197);  // DOCK = 117.85 MHz

    // ******************** Attachment points **************************

    // top-center (for lifter attachment)
    // SET IN CONFIG FILE: CreateAttachment(true, _V(0,0,0), _V(0,-1,0), _V(0,0,1), "XS");

    // ******************** NAV radios **************************

    InitNavRadios(4);

    // ****************** propellant specs **********************

    // set tank configuration
    max_rocketfuel = TANK1_CAPACITY;
    max_scramfuel = TANK2_CAPACITY;

    // NOTE: Orbiter seems to reset this to zero later, since he expects the scenario file to be read.
    // WARNING: do NOT init these values to > 0, because Orbiter will NOT set the tank value if the fraction is 
    // zero in the scenario file.
    ph_main = CreatePropellantResource(max_rocketfuel);      // main tank (fuel + oxydant)
    ph_rcs = CreatePropellantResource(RCS_FUEL_CAPACITY);   // RCS tank  (fuel + oxydant)
    ph_scram = CreatePropellantResource(max_scramfuel);       // scramjet fuel

    // **************** thruster definitions ********************

    double ispscale = (GetXR1Config()->EnableATMThrustReduction ? 0.8 : 1.0);
    // Reduction of thrust efficiency at normal pressure

    // increase level, srcrate, and lifetime
    const double particleMult = 1.5;
    PARTICLESTREAMSPEC contrail = {
        0, 11.0 * particleMult, 6 * particleMult, 150, 0.3, 7.5, 4, 3.0, PARTICLESTREAMSPEC::DIFFUSE,
            PARTICLESTREAMSPEC::LVL_PSQRT, 0, 2,
            PARTICLESTREAMSPEC::ATM_PLOG, 1e-4, 1
    };

    // increase level
    PARTICLESTREAMSPEC exhaust_main = {
        0, 3.0 * particleMult, 10 * particleMult, 150, 0.1, 0.2, 16, 1.0, PARTICLESTREAMSPEC::EMISSIVE,
            PARTICLESTREAMSPEC::LVL_SQRT, 0, 1,
            PARTICLESTREAMSPEC::ATM_PLOG, 1e-5, 0.1
    };
    // increase level
    PARTICLESTREAMSPEC exhaust_hover = {
        0, 2.0 * particleMult, 10 * particleMult, 150, 0.1, 0.15, 16, 1.0, PARTICLESTREAMSPEC::EMISSIVE,
            PARTICLESTREAMSPEC::LVL_SQRT, 0, 1,
            PARTICLESTREAMSPEC::ATM_PLOG, 1e-5, 0.1
    };
    // increase level and particle lifetime
    PARTICLESTREAMSPEC exhaust_scram = {
       0, 3.0 * particleMult, 25 * particleMult, 150, 0.05, 15.0, 3, 1.0, PARTICLESTREAMSPEC::EMISSIVE,
           PARTICLESTREAMSPEC::LVL_SQRT, 0, 1,
           PARTICLESTREAMSPEC::ATM_PLOG, 1e-5, 0.1
    };
    // NEW for XR5: RCS
    PARTICLESTREAMSPEC exhaust_rcs = {
        0,          // flags
        0.01,       // particle size
        15,         // particle create rate (Hz)
        25,         // emission velocity
        0.03,       // velocity spread during creation
        0.55,       // average particle lifetime
        1.5,        // particle growth rate
        0.10,       // slowdown in ATM (m/s)
        PARTICLESTREAMSPEC::EMISSIVE, PARTICLESTREAMSPEC::LVL_SQRT, 0, 1, PARTICLESTREAMSPEC::ATM_PLOG, 1e-5, 0.1
    };
    // NEW for XR5: retros
    PARTICLESTREAMSPEC exhaust_retro = {
        0,          // flags
        0.19,       // particle size
        65,         // particle creation rate (Hz)
        60,         // emission velocity
        0.13,       // velocity spread during creation
        1.50,       // average particle lifetime
        2.0,        // particle growth rate
        0.40,       // slowdown in ATM (m/s)
        PARTICLESTREAMSPEC::EMISSIVE, PARTICLESTREAMSPEC::LVL_SQRT, 0, 1, PARTICLESTREAMSPEC::ATM_PLOG, 1e-5, 0.1
    };


    // handle new configurable ISP
    const double mainISP = GetXR1Config()->GetMainISP();

    /* From API Guide:
        Vessel coordinates are always defined so that the CG is at the origin (0,0,0). Therefore, a
        thruster located at (0,0,-10) and generating thrust in direction (0,0,1) would not generate
        torque.
    */

    // define thruster locations in meters from the ship's centerpoint
    const double shipLength = 60.47;
    const double rcsZHullDistance = (shipLength / 2) - 4.0;   // distance from Z centerline -> RCS fore and aft
    const double rcsXWingDistance = 19.0;                     // distance from X centerline -> RCS on wings

    // main thrusters
    const double mainEngineZ = -(shipLength / 2) - 1;
    th_main[0] = CreateThruster(_V(-3.59, 0, mainEngineZ), _V(0, 0, 1), MAX_MAIN_THRUST[GetXR1Config()->MainEngineThrust], ph_main, mainISP, mainISP * ispscale);
    th_main[1] = CreateThruster(_V(3.59, 0, mainEngineZ), _V(0, 0, 1), MAX_MAIN_THRUST[GetXR1Config()->MainEngineThrust], ph_main, mainISP, mainISP * ispscale);

    thg_main = CreateThrusterGroup(th_main, 2, THGROUP_MAIN);
    SURFHANDLE mainExhaustTex = oapiRegisterExhaustTexture("XR5Vanguard\\ExhaustXR5Vanguard");
    // 1.0 ORG: const double mainLscale = 33;
    // PRE-1.7 (XR1 1.9 group) VERSIONS: const double mainLscale = 29;
    const double mainLscale = 20;
    // const double mainWscale = 2.445;  // outside actual size (too big)
    // const double mainWscale = 1.546;  // inside actual size (still too big??)
    // 1.0 ORG: const double mainWscale = 1.35; 
    // Pre-1.7 (XR1 1.9 group): const double mainWscale = 1.2225;   // RADIUS
    const double mainWscale = 1.3;   // RADIUS
    // Pre-1.7 (XR1 1.9 group): const double mainExhaustZCoord = -27.367;
    const double mainExhaustZCoord = -29.5;  // to show the exhaust texture better

#define ADD_MAIN_EXHAUST(th, x, y)  \
    AddXRExhaust (th, mainLscale, mainWscale, _V(x, y, mainExhaustZCoord), _V(0,0,-1), mainExhaustTex); \
    AddExhaustStream (th, _V(x, y, mainExhaustZCoord - 13), &exhaust_main);                           \
    AddExhaustStream (th, _V(x, y, mainExhaustZCoord - 20), &contrail)

    // left side (viewed from rear)
    ADD_MAIN_EXHAUST(th_main[0], -4.222, 4.514);  // top
    ADD_MAIN_EXHAUST(th_main[0], -6.103, 1.227);  // outboard
    ADD_MAIN_EXHAUST(th_main[0], -2.116, 1.227);  // inboard

    // right side (viewed from rear)
    ADD_MAIN_EXHAUST(th_main[1], 4.170, 4.502);  // top
    ADD_MAIN_EXHAUST(th_main[1], 6.111, 1.227);  // outboard
    ADD_MAIN_EXHAUST(th_main[1], 2.119, 1.227);  // inboard

    // retro thrusters
    const double retroXCoord = 2.827;
    const double retroYCoord = 0.636;
    // Pre-1.7 (XR1 1.9 group): const double retroZCoord = 27.309;
    const double retroZCoord = 27.8;  // show the texture better
    th_retro[0] = CreateThruster(_V(-retroXCoord, 0, retroZCoord), _V(0, 0, -1.0), MAX_RETRO_THRUST, ph_main, mainISP, mainISP * ispscale);
    th_retro[1] = CreateThruster(_V(retroXCoord, 0, retroZCoord), _V(0, 0, -1.0), MAX_RETRO_THRUST, ph_main, mainISP, mainISP * ispscale);

    const double retroLscale = 3.0;
    // const double retroWscale = 0.1315;  // too narrow
    // 1.0 ORG: const double retroWscale = 0.19;  
    // Pre-1.7 (XR1 1.9 group): const double retroWscale = 0.140;  
    const double retroWscale = 0.18; // show the texture better

#define ADD_RETRO_EXHAUST(th, x) \
    AddXRExhaust (th, retroLscale, retroWscale, _V(x, retroYCoord, retroZCoord), _V(0,0,1), mainExhaustTex);  \
    AddExhaustStream (th, _V(x, retroYCoord, retroZCoord + 0.5), &exhaust_retro)

    thg_retro = CreateThrusterGroup(th_retro, 2, THGROUP_RETRO);
    ADD_RETRO_EXHAUST(th_retro[0], -retroXCoord);
    ADD_RETRO_EXHAUST(th_retro[1], retroXCoord);

    // hover thrusters (simplified)
    // The two aft hover engines are combined into a single "logical" thruster,
    // but exhaust is rendered separately for both
    th_hover[0] = CreateThruster(_V(0, 0, 14.32), _V(0, 1, 0), MAX_HOVER_THRUST[GetXR1Config()->HoverEngineThrust], ph_main, mainISP, mainISP * ispscale);
    th_hover[1] = CreateThruster(_V(0, 0, -14.32), _V(0, 1, 0), MAX_HOVER_THRUST[GetXR1Config()->HoverEngineThrust], ph_main, mainISP, mainISP * ispscale);
    thg_hover = CreateThrusterGroup(th_hover, 2, THGROUP_HOVER);

    // 1.0: const double hoverLscale = 15;
    // PRE-1.7 (XR1 1.9 group) VERSIONS: const double hoverLscale = 13;
    const double hoverLscale = 11;  // shorter (old were too long)
    // const double hoverWscale = 1.5515;  // as measured  (too narrow)
    // 1.0: const double hoverWscale = 1.35;  // looks better with mains
    const double hoverWscale = mainWscale;  // matches the mains

#define ADD_HOVER_EXHAUST(th, x, y, z)  \
    AddXRExhaust (th, hoverLscale, hoverWscale, _V(x, y, z), _V(0,-1,0), mainExhaustTex); \
    AddExhaustStream (th, _V(x, y - 8, z), &exhaust_hover);                             \
    AddExhaustStream (th, _V(x, y - 13, z), &contrail)

    // forward
    ADD_HOVER_EXHAUST(th_hover[0], 3.294, -1.46, 12.799);
    ADD_HOVER_EXHAUST(th_hover[0], -3.297, -1.46, 12.799);

    // aft starboard  (right-hand side looking forward)
    ADD_HOVER_EXHAUST(th_hover[1], -22.324, -1.091, -13.633);
    ADD_HOVER_EXHAUST(th_hover[1], -22.324, -1.091, -17.632);

    // aft port
    ADD_HOVER_EXHAUST(th_hover[1], 22.324, -1.091, -13.633);
    ADD_HOVER_EXHAUST(th_hover[1], 22.324, -1.091, -17.632);

    // set of attitude thrusters (idealised). The arrangement is such that no angular
    // momentum is created in linear mode, and no linear momentum is created in rotational mode.
    SURFHANDLE rcsExhaustTex = mainExhaustTex;

    // create RCS thrusters
    th_rcs[0] = CreateThruster(_V(0, 0, rcsZHullDistance), _V(0, 1, 0), GetRCSThrustMax(0), ph_rcs, mainISP);  // fore bottom (i.e., pushes UP from the BOTTOM of the hull)
    th_rcs[1] = CreateThruster(_V(0, 0, -rcsZHullDistance), _V(0, -1, 0), GetRCSThrustMax(1), ph_rcs, mainISP);  // aft top
    th_rcs[2] = CreateThruster(_V(0, 0, rcsZHullDistance), _V(0, -1, 0), GetRCSThrustMax(2), ph_rcs, mainISP);  // fore top
    th_rcs[3] = CreateThruster(_V(0, 0, -rcsZHullDistance), _V(0, 1, 0), GetRCSThrustMax(3), ph_rcs, mainISP);  // aft bottom

    // 1.0 ORG: const double rcsLscale = 1.2;    
    // Pre-1.7 (XR1 1.9 group): const double rcsLscale = 1.4;    
    const double rcsLscale = 1.6;   // so it shows up the new textures better
    // 1.0 ORG: const double rcsWscale = 0.127;  // as measured
    // Pre-1.7 (XR1 1.9 group): const double rcsWscale = 0.100;  
    const double rcsWscale = 0.127;  // back to as-measured

    // Note: actual rcs pit depth is 0.5 meter for standard hull-mounted jets
    const double rcsDepthModifier = 0.4;       // reduce depth of thruster flame firing so it shows up better
    const double rcsNoseDepthModifier = 0.71;  // top-mounted Y-axis nose RCS jets are deeper than standard jets
    // Pre-1.7 (XR1 1.9 group): const double rcsTailDepthModifier = 0.71;  // rear-mounted Z-axis RCS jets are deeper than standard jets
    const double rcsTailDepthModifier = 0.9;  // rear-mounted Z-axis RCS jets are deeper than standard jets

    const double exhaustDistance = 1.4;        // exhaust distance from thruster coordinates

// move exhaust smoke away from the RCS jet by a fixed distance
#define GetRCSExhaustStreamCoords(coordsV, directionV) (coordsV + (directionV * exhaustDistance))

#define ADD_RCS_EXHAUST(th, coordsV, directionV)                                     \
        AddXRExhaust (th, rcsLscale, rcsWscale, coordsV, directionV, rcsExhaustTex)

// NO: AddExhaustStream (th, GetRCSExhaustStreamCoords(coordsV, directionV), &exhaust_rcs)

// compute actual RCS depth coordinate; this is necessary for hull-mounted RCS jets
#define RCS_DCOORD(c, dir) (c + (dir * rcsDepthModifier))
#define NOSE_RCS_DCOORD(c, dir) (c + (dir * rcsNoseDepthModifier))
#define TAIL_RCS_DCOORD(c, dir) (c + (dir * rcsTailDepthModifier))

    ADD_RCS_EXHAUST(th_rcs[0], _V(2.613, RCS_DCOORD(-0.284, -1), 25.532), _V(0, -1, 0));  // fore bottom
    ADD_RCS_EXHAUST(th_rcs[0], _V(2.411, RCS_DCOORD(-0.273, -1), 26.039), _V(0, -1, 0));
    ADD_RCS_EXHAUST(th_rcs[0], _V(-2.618, RCS_DCOORD(-0.284, -1), 25.532), _V(0, -1, 0));
    ADD_RCS_EXHAUST(th_rcs[0], _V(-2.416, RCS_DCOORD(-0.273, -1), 26.039), _V(0, -1, 0));

    ADD_RCS_EXHAUST(th_rcs[1], _V(-9.402, RCS_DCOORD(0.241, 1.5), -24.299), _V(0, 1, 0));  // aft top
    ADD_RCS_EXHAUST(th_rcs[1], _V(-9.485, RCS_DCOORD(0.241, 1.5), -23.936), _V(0, 1, 0));
    ADD_RCS_EXHAUST(th_rcs[1], _V(9.493, RCS_DCOORD(0.241, 1.5), -23.936), _V(0, 1, 0));
    ADD_RCS_EXHAUST(th_rcs[1], _V(9.410, RCS_DCOORD(0.241, 1.5), -24.299), _V(0, 1, 0));

    ADD_RCS_EXHAUST(th_rcs[2], _V(2.646, NOSE_RCS_DCOORD(2.133, 1), 26.390), _V(0, 1, 0));  // fore top
    ADD_RCS_EXHAUST(th_rcs[2], _V(2.510, NOSE_RCS_DCOORD(2.110, 1), 26.918), _V(0, 1, 0));
    ADD_RCS_EXHAUST(th_rcs[2], _V(-2.646, NOSE_RCS_DCOORD(2.133, 1), 26.390), _V(0, 1, 0));
    ADD_RCS_EXHAUST(th_rcs[2], _V(-2.510, NOSE_RCS_DCOORD(2.110, 1), 26.918), _V(0, 1, 0));

    ADD_RCS_EXHAUST(th_rcs[3], _V(9.410, RCS_DCOORD(-0.04, -1), -24.572), _V(0, -1, 0));  // aft bottom
    ADD_RCS_EXHAUST(th_rcs[3], _V(9.410, RCS_DCOORD(-0.04, -1), -24.916), _V(0, -1, 0));
    ADD_RCS_EXHAUST(th_rcs[3], _V(-9.402, RCS_DCOORD(-0.04, -1), -24.572), _V(0, -1, 0));
    ADD_RCS_EXHAUST(th_rcs[3], _V(-9.402, RCS_DCOORD(-0.04, -1), -24.916), _V(0, -1, 0));

    th_rcs[4] = CreateThruster(_V(0, 0, rcsZHullDistance), _V(-1, 0, 0), GetRCSThrustMax(4), ph_rcs, mainISP);  // fore right side
    th_rcs[5] = CreateThruster(_V(0, 0, -rcsZHullDistance), _V(1, 0, 0), GetRCSThrustMax(5), ph_rcs, mainISP);  // aft left side
    th_rcs[6] = CreateThruster(_V(0, 0, rcsZHullDistance), _V(1, 0, 0), GetRCSThrustMax(6), ph_rcs, mainISP);  // fore left side
    th_rcs[7] = CreateThruster(_V(0, 0, -rcsZHullDistance), _V(-1, 0, 0), GetRCSThrustMax(7), ph_rcs, mainISP);  // aft right side

    ADD_RCS_EXHAUST(th_rcs[4], _V(RCS_DCOORD(1.999, 1), 3.150, 26.150), _V(1, 0, 0));   // fore right side
    ADD_RCS_EXHAUST(th_rcs[4], _V(RCS_DCOORD(1.999, 1), 3.012, 26.658), _V(1, 0, 0));
    ADD_RCS_EXHAUST(th_rcs[4], _V(RCS_DCOORD(2.390, 1), -0.455, 25.789), _V(1, 0, 0));
    ADD_RCS_EXHAUST(th_rcs[4], _V(RCS_DCOORD(2.644, 1), -0.480, 25.276), _V(1, 0, 0));

    ADD_RCS_EXHAUST(th_rcs[5], _V(RCS_DCOORD(-8.559, -1.5), 0.891, -25.188), _V(-1, 0, 0));  // aft left side
    ADD_RCS_EXHAUST(th_rcs[5], _V(RCS_DCOORD(-8.664, -1.5), 0.891, -24.768), _V(-1, 0, 0));

    ADD_RCS_EXHAUST(th_rcs[6], _V(RCS_DCOORD(-1.999, -1), 3.012, 26.658), _V(-1, 0, 0));  // fore left side
    ADD_RCS_EXHAUST(th_rcs[6], _V(RCS_DCOORD(-1.999, -1), 3.150, 26.150), _V(-1, 0, 0));
    ADD_RCS_EXHAUST(th_rcs[6], _V(RCS_DCOORD(-2.395, -1), -0.455, 25.789), _V(-1, 0, 0));
    ADD_RCS_EXHAUST(th_rcs[6], _V(RCS_DCOORD(-2.650, -1), -0.480, 25.276), _V(-1, 0, 0));

    ADD_RCS_EXHAUST(th_rcs[7], _V(RCS_DCOORD(8.568, 1.5), 0.891, -25.188), _V(1, 0, 0));  // aft right side
    ADD_RCS_EXHAUST(th_rcs[7], _V(RCS_DCOORD(8.673, 1.5), 0.891, -24.768), _V(1, 0, 0));

    th_rcs[8] = CreateThruster(_V(rcsXWingDistance, 0, 0), _V(0, 1, 0), GetRCSThrustMax(8), ph_rcs, mainISP);    // right wing bottom
    th_rcs[9] = CreateThruster(_V(-rcsXWingDistance, 0, 0), _V(0, -1, 0), GetRCSThrustMax(9), ph_rcs, mainISP);    // left wing top
    th_rcs[10] = CreateThruster(_V(-rcsXWingDistance, 0, 0), _V(0, 1, 0), GetRCSThrustMax(10), ph_rcs, mainISP);    // left wing bottom
    th_rcs[11] = CreateThruster(_V(rcsXWingDistance, 0, 0), _V(0, -1, 0), GetRCSThrustMax(11), ph_rcs, mainISP);    // right wing top

    // wing exhaust does not get depth adjustment
    ADD_RCS_EXHAUST(th_rcs[8], _V(18.876, -0.816, -7.794), _V(0, -1, 0));  // right wing bottom
    ADD_RCS_EXHAUST(th_rcs[9], _V(-18.886, 0.839, -7.493), _V(0, 1, 0));   // left wing top
    ADD_RCS_EXHAUST(th_rcs[10], _V(-18.868, -0.816, -7.546), _V(0, -1, 0));  // left wing bottom
    ADD_RCS_EXHAUST(th_rcs[11], _V(18.886, 0.839, -7.493), _V(0, 1, 0));  // right wing top

    // put the RCS directly on the Y centerline so we don't induce any rotation
    th_rcs[12] = CreateThruster(_V(0, 0, -rcsZHullDistance), _V(0, 0, 1), GetRCSThrustMax(12), ph_rcs, mainISP);   // aft
    th_rcs[13] = CreateThruster(_V(0, 0, rcsZHullDistance), _V(0, 0, -1), GetRCSThrustMax(13), ph_rcs, mainISP);   // fore

    ADD_RCS_EXHAUST(th_rcs[12], _V(9.581, 0.401, TAIL_RCS_DCOORD(-24.108, -1)), _V(0, 0, -1));   // aft Z axis
    ADD_RCS_EXHAUST(th_rcs[12], _V(9.723, 0.074, TAIL_RCS_DCOORD(-24.108, -1)), _V(0, 0, -1));
    ADD_RCS_EXHAUST(th_rcs[12], _V(-9.714, 0.074, TAIL_RCS_DCOORD(-24.108, -1)), _V(0, 0, -1));
    ADD_RCS_EXHAUST(th_rcs[12], _V(-9.572, 0.401, TAIL_RCS_DCOORD(-24.108, -1)), _V(0, 0, -1));

    ADD_RCS_EXHAUST(th_rcs[13], _V(-1.974, 2.546, RCS_DCOORD(27.685, 1)), _V(0, 0, 1));  // fore Z axis
    ADD_RCS_EXHAUST(th_rcs[13], _V(-2.121, 2.250, RCS_DCOORD(27.625, 1)), _V(0, 0, 1));
    ADD_RCS_EXHAUST(th_rcs[13], _V(1.974, 2.546, RCS_DCOORD(27.685, 1)), _V(0, 0, 1));
    ADD_RCS_EXHAUST(th_rcs[13], _V(2.121, 2.250, RCS_DCOORD(27.625, 1)), _V(0, 0, 1));

    // NOTE: must invoke ConfigureRCSJets later after the scenario file is read

    // **************** scramjet definitions ********************

    const double scramX = 1.931;  // distance from centerline
    for (int i = 0; i < 2; i++)
    {
        th_scram[i] = CreateThruster(_V((i ? scramX : -scramX), 0, -rcsZHullDistance), _V(0, 0, 1), 0, ph_scram, 0);
        ramjet->AddThrusterDefinition(th_scram[i], SCRAM_FHV[GetXR1Config()->SCRAMfhv],
            SCRAM_INTAKE_AREA, SCRAM_INTERNAL_TEMAX, GetXR1Config()->GetScramMaxEffectiveDMF());
    }

    // thrust rating and ISP for scramjet engines are updated continuously
    PSTREAM_HANDLE ph;
    const double scramDelta = -1.0;   // move particles back from the engines slightly
    // Note: ph will be null if exhaust streams are disabled
    ph = AddExhaustStream(th_scram[0], _V(-scramX, -2.121, -25.205 + scramDelta), &exhaust_scram);
    if (ph != nullptr)
        oapiParticleSetLevelRef(ph, scram_intensity + 0);

    ph = AddExhaustStream(th_scram[1], _V(scramX, -2.121, -25.205 + scramDelta), &exhaust_scram);
    if (ph != nullptr)
        oapiParticleSetLevelRef(ph, scram_intensity + 1);

#ifdef WONT_WORK
    // ********************* aerodynamics ***********************

    // NOTE: org values were causing nasty downward pitch in the atmospehere: 
    // hwing = CreateAirfoil3 (LIFT_VERTICAL, _V(0,0,-0.3), VLiftCoeff, nullptr, 5, 90, 1.5);
    // Note: aspect ratio = total span squared divided by wing area: 76.67^2 / mesh_wing_area
    const double aspectRatio = (54.909154 * 54.909154) / 479.07;
    hwing = CreateAirfoil3(LIFT_VERTICAL, _V(m_wingBalance, 0, m_centerOfLift), VLiftCoeff, nullptr, 27.68, WING_AREA, aspectRatio);

    // vertical stabiliser and body lift and drag components
    // location: effectively in center of vessel (because there are two), and just over 20 meters back from the center of the vessel
    // Note: aspect ratio = total span squared divided by wing area: 16.31.16.31^2 / mesh_wing_area
    const double verticalStabilWingArea = 96.68;
    const double vertAspectRatio = (16.30805 * 16.30805) / verticalStabilWingArea;
    CreateAirfoil3(LIFT_HORIZONTAL, _V(0, 0, -20.12), HLiftCoeff, nullptr, 16.797632, verticalStabilWingArea, vertAspectRatio);


    // ref vector is 1 meter behind the vertical stabilizers
    hElevator = CreateControlSurface2(AIRCTRL_ELEVATOR, 40.32, 1.4, _V(0, 0, -21.2), AIRCTRL_AXIS_XPOS, anim_elevator);

    CreateControlSurface(AIRCTRL_RUDDER, 96.68, 1.5, _V(0, 0, -21.2), AIRCTRL_AXIS_YPOS, anim_rudder);

    hLeftAileron = CreateControlSurface2(AIRCTRL_AILERON, 15.38, 1.5, _V(31.96, 0, -21.2), AIRCTRL_AXIS_XPOS, anim_raileron);
    hRightAileron = CreateControlSurface2(AIRCTRL_AILERON, 15.38, 1.5, _V(-31.96, 0, -21.2), AIRCTRL_AXIS_XNEG, anim_laileron);

    CreateControlSurface(AIRCTRL_ELEVATORTRIM, 10.08, 1.5, _V(0, 0, -21.2), AIRCTRL_AXIS_XPOS, anim_elevatortrim);
#endif

    // ********************* aerodynamics ***********************

    // NOTE: org values were causing nasty downward pitch in the atmospehere: 
    // hwing = CreateAirfoil3 (LIFT_VERTICAL, _V(0,0,-0.3), VLiftCoeff, nullptr, 5, 90, 1.5);
    m_ctrlSurfacesDeltaZ = -21.2;       // distance from center of model to center of control surfaces, Z axis
    m_aileronDeltaX = 31.962114;   // distance from center of ship to center of aileron, X direction
    XR1Multiplier = 29.94;       // control surface area vs. the XR1  (5.99 = wing area delta w/area 479.07)

    // center of lift matches center of mass
    // NOTE: this airfoil's force attack point will be modified by the SetCenterOfLift PreStep 
    hwing = CreateAirfoil3(LIFT_VERTICAL, _V(m_wingBalance, 0, m_centerOfLift), VLiftCoeff, nullptr, 5 * XR1Multiplier, WING_AREA, WING_ASPECT_RATIO);

    CreateAirfoil3(LIFT_HORIZONTAL, _V(0, 0, m_ctrlSurfacesDeltaZ + 3.0), HLiftCoeff, nullptr, 16.79, 15 * XR1Multiplier, 1.5);

    ReinitializeDamageableControlSurfaces();  // create ailerons, elevators, and elevator trim

    // vertical stabiliser and body lift and drag components
    CreateControlSurface(AIRCTRL_RUDDER, 0.8 * XR1Multiplier, 1.5, _V(0, 0, m_ctrlSurfacesDeltaZ), AIRCTRL_AXIS_YPOS, anim_rudder);

    // Create a hidden elevator trim to fix the nose-up tendency on liftoff and allow the elevator trim to be truly neutral.
    // We have to use FLAP here because that is the only unused control surface type.  We could probably also duplicate this via CreateAirfoil3, but this
    // is easier to adjust and test.
    CreateControlSurface(AIRCTRL_FLAP, 0.3 * XR1Multiplier * 7, 1.5, _V(0, 0, m_ctrlSurfacesDeltaZ), AIRCTRL_AXIS_XPOS);  // no animation for this!
    m_hiddenElevatorTrimState = HIDDEN_ELEVATOR_TRIM_STATE;        // set to a member variable in case we want to change it in flight later
    // Note: cannot set the level here; it is reset by Orbiter later.

    // TOO SMALL! const double xr1VariableDragModifier = 3.84;    // this is the avg ratio of the XR5:XR1 length, width, and height
    const double xr1VariableDragModifier = 22.2;    // this is the mass ratio of the XR5:XR1
    // we need these goofy const casts to force the linker to link with the 'const double *' version of CreateVariableDragElement instead of the legacy 'double *' version of the function (which still exists but is deprecated and causes warnings in orbiter.log)
    CreateVariableDragElement(const_cast<const double*>(&rcover_proc), 0.2 * xr1VariableDragModifier, _V(0, 0.581, 26.972));     // retro covers
    CreateVariableDragElement(const_cast<const double*>(&radiator_proc), 0.4 * xr1VariableDragModifier, _V(0, 3.274, -21.925));     // radiators
    CreateVariableDragElement(const_cast<const double*>(&bay_proc), 7.0 * xr1VariableDragModifier, _V(0, 8.01, -21.06));       // bay doors (drag is at rear of bay)
    CreateVariableDragElement(const_cast<const double*>(&gear_proc), 0.8 * xr1VariableDragModifier, _V(0, -9.539, 4.34));      // landing gear
    CreateVariableDragElement(const_cast<const double*>(&nose_proc), 2.1 * xr1VariableDragModifier, _V(0, 10.381, 6.515));     // docking port
    CreateVariableDragElement(const_cast<const double*>(&brake_proc), 4.0 * xr1VariableDragModifier, _V(0, 0, m_ctrlSurfacesDeltaZ));  // airbrake (do not induce a rotational moment here)
    CreateVariableDragElement(const_cast<const double*>(&crewElevator_proc), 6.0 * xr1VariableDragModifier, _V(-3.358, -6.51, 6.371));  // elevator (note: elevator is off-center)

    // XR5 ORG: const double dragMultiplier = 3.0;   
    const double dragMultiplier = 22.2;   // increased per c3p0: ship is easier to land now
    SetRotDrag(_V(0.10 * dragMultiplier, 0.13 * dragMultiplier, 0.04 * dragMultiplier));

    // define hull temperature limits (these match the XR1's limits for now)
    m_hullTemperatureLimits.noseCone = CTOK(2840);
    m_hullTemperatureLimits.wings = CTOK(2380);
    m_hullTemperatureLimits.cockpit = CTOK(1490);
    m_hullTemperatureLimits.topHull = CTOK(1210);
    m_hullTemperatureLimits.warningFrac = 0.80;   // yellow text
    m_hullTemperatureLimits.criticalFrac = 0.90;   // red text
    m_hullTemperatureLimits.doorOpenWarning = 0.75;
    // aluminum melts @ 660C and begins deforming below that
    m_hullTemperatureLimits.doorOpen = CTOK(480);

    // default to full LOX tank if not loaded from save file 
    if (m_loxQty < 0)
        m_loxQty = GetXR1Config()->GetMaxLoxMass();

    // ************************* mesh ***************************

    // ********************* beacon lights **********************
    const double bd = 0.4;  // beacon delta from the mesh edge
    static VECTOR3 beaconpos[7] = {
        {-37.605, 0.561 + bd, -18.939 + bd}, {37.605, 0.561 + bd, -18.939 + bd}, {0, 3.241, -30.489 - bd},  // nav: left wing, right wing, aft center
        {0, 7.958 + bd, 8.849}, {0, -1.26 - bd, 8.823},                     // beacon: top hull, bottom hull
        {-37.605, 7.932 + bd, -28.304}, {37.605, 7.932 + bd, -28.304},      // strobe: left rudder top, right rudder top
    };

    // RGB colors
    static VECTOR3 beaconcol[7] = {
        {1.0,0.5,0.5}, {0.5,1.0,0.5}, {1,1,1},  // nav RGB colors
        {1,0.6,0.6}, {1,0.6,0.6},               // beacon
        {1,1,1}, {1,1,1}                        // strobe
    };

    const double sizeMultiplier = 3.0;
    for (int i = 0; i < 7; i++)
    {
        beacon[i].shape = (i < 3 ? BEACONSHAPE_DIFFUSE : BEACONSHAPE_STAR);
        beacon[i].pos = beaconpos + i;
        beacon[i].col = beaconcol + i;
        beacon[i].size = (i < 3 ? 0.3 * sizeMultiplier : 0.55 * sizeMultiplier);
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

    // add a light at each main engine set of 3
    const double mainEnginePointLightPower = 100 * 22.2;  // XR5 engines are 22.5 times as powerful as the XR1's
    const double zMainLightDelta = -5.0;  // need more delta here because the exhaust is sunk into the engine bell
    if (GetXR1Config()->EnableEngineLightingEffects)
    {
        LightEmitter* leMainPort = AddPointLight(_V(-4.1095, 2.871, mainExhaustZCoord + zMainLightDelta), mainEnginePointLightPower, 1e-3, 0, 2e-3, col_d, col_s, col_a);
        LightEmitter* leMainStarboard = AddPointLight(_V(4.1095, 2.871, mainExhaustZCoord + zMainLightDelta), mainEnginePointLightPower, 1e-3, 0, 2e-3, col_d, col_s, col_a);
        leMainPort->SetIntensityRef(&m_mainThrusterLightLevel);
        leMainStarboard->SetIntensityRef(&m_mainThrusterLightLevel);
    }

    // add a light at each set of hover engines
    if (GetXR1Config()->EnableEngineLightingEffects)
    {
        const double hoverEnginePointLightPower = mainEnginePointLightPower * 0.6875;  // hovers are .6875 the thrust of the mains
        const double yHoverLightDelta = -1.0;
        LightEmitter* leForward = AddPointLight(_V(0.000, -1.460 + yHoverLightDelta, 12.799), hoverEnginePointLightPower, 1e-3, 0, 2e-3, col_d, col_s, col_a);
        LightEmitter* leAftPort = AddPointLight(_V(-22.324, -1.091 + yHoverLightDelta, -15.633), hoverEnginePointLightPower, 1e-3, 0, 2e-3, col_d, col_s, col_a);
        LightEmitter* leAftStarboard = AddPointLight(_V(22.324, -1.091 + yHoverLightDelta, -15.633), hoverEnginePointLightPower, 1e-3, 0, 2e-3, col_d, col_s, col_a);
        leForward->SetIntensityRef(&m_hoverThrusterLightLevel);
        leAftPort->SetIntensityRef(&m_hoverThrusterLightLevel);
        leAftStarboard->SetIntensityRef(&m_hoverThrusterLightLevel);
    }

    // add docking lights (2 forward and 2 docking)
    // Note: XR1/XR2 rage was 150 meters
    // forward
    m_pSpotlights[0] = static_cast<SpotLight*>(AddSpotLight(_V(10.628, -0.055, 3.586), _V(0, 0, 1), 250, 1e-3, 0, 1e-3, RAD * 25, RAD * 60, col_white, col_white, col_a));
    m_pSpotlights[1] = static_cast<SpotLight*>(AddSpotLight(_V(-10.628, -0.055, 3.586), _V(0, 0, 1), 250, 1e-3, 0, 1e-3, RAD * 25, RAD * 60, col_white, col_white, col_a));
    // docking port 
    m_pSpotlights[2] = static_cast<SpotLight*>(AddSpotLight(_V(-1.66, 7.475, 6.375), _V(0, 1, 0), 250, 1e-3, 0, 1e-3, RAD * 25, RAD * 60, col_white, col_white, col_a));
    m_pSpotlights[3] = static_cast<SpotLight*>(AddSpotLight(_V(1.66, 7.475, 6.375), _V(0, 1, 0), 250, 1e-3, 0, 1e-3, RAD * 25, RAD * 60, col_white, col_white, col_a));

    // turn all spotlights off by default
    for (int i = 0; i < SPOTLIGHT_COUNT; i++)
        m_pSpotlights[i]->Activate(false);

    // load meshes
    // no VC: vcmesh_tpl = oapiLoadMeshGlobal("dg-xr1\\deltaglidercockpit-xr1");  // VC mesh
    vcmesh_tpl = nullptr;  // no VC; must set this to null so the superclass won't try to use it
    exmesh_tpl = oapiLoadMeshGlobal("XR5Vanguard\\XR5Vanguard");         // exterior mesh

    m_exteriorMeshIndex = AddMesh(exmesh_tpl);  // save so we can modify the mesh later
    SetMeshVisibilityMode(m_exteriorMeshIndex, MESHVIS_EXTERNAL);
    // no VC: SetMeshVisibilityMode(AddMesh(vcmesh_tpl), MESHVIS_VC);

#ifdef MMU
    ///////////////////////////////////////////////////////////////////////
    // Init UMmu
    ///////////////////////////////////////////////////////////////////////
    const int ummuStatus = UMmu.InitUmmu(GetHandle());  // returns 1 if ok and other number if not

    // UMmu is REQUIRED!
    if (ummuStatus != 1)
        FatalError("UMmu not installed!  You must install Universal Mmu 3.0 or newer in order to use the XR5; visit http://www.alteaaerospace.com for more information.");

    // validate UMmu version and write it to the log file
    const float ummuVersion = CONST_UMMU_XR5(this).GetUserUMmuVersion();
    if (ummuVersion < 3.0)
    {
        char msg[256];
        sprintf(msg, "UMmu version %.2f is installed, but the XR5 requires Universal Mmu 3.0 or higher; visit http://www.alteaaerospace.com for more information.", ummuVersion);
        FatalError(msg);
    }

    char msg[256];
    sprintf(msg, "Using UMmu Version: %.2f", ummuVersion);
    GetXR1Config()->WriteLog(msg);
#endif

    // UMMU Bug: must invoke SetMaxSeatAvailableInShip and SetCrewWeightUpdateShipWeightAutomatically each time we redefine the airlock
    // NOTE: UMmu airlock definition and default crew data will be set again later AFTER the scenario file is parsed.
    DefineMmuAirlock();  // required here so that UMMu loads the crew from the scenario file!

    //
    // Initialize and cache all instrument panels
    //

    // 1920-pixel-wide panels
    AddInstrumentPanel(new XR5MainInstrumentPanel1920(*this), 1920);
    AddInstrumentPanel(new XR5UpperInstrumentPanel1920(*this), 1920);
    AddInstrumentPanel(new XR5LowerInstrumentPanel1920(*this), 1920);
    AddInstrumentPanel(new XR5OverheadInstrumentPanel1920(*this), 1920);
    AddInstrumentPanel(new XR5PayloadInstrumentPanel1920(*this), 1920);

    // 1600-pixel-wide panels
    AddInstrumentPanel(new XR5MainInstrumentPanel1600(*this), 1600);
    AddInstrumentPanel(new XR5UpperInstrumentPanel1600(*this), 1600);
    AddInstrumentPanel(new XR5LowerInstrumentPanel1600(*this), 1600);
    AddInstrumentPanel(new XR5OverheadInstrumentPanel1600(*this), 1600);
    AddInstrumentPanel(new XR5PayloadInstrumentPanel1600(*this), 1600);

    // 1280-pixel-wide panels
    AddInstrumentPanel(new XR5MainInstrumentPanel1280(*this), 1280);
    AddInstrumentPanel(new XR5UpperInstrumentPanel1280(*this), 1280);
    AddInstrumentPanel(new XR5LowerInstrumentPanel1280(*this), 1280);
    AddInstrumentPanel(new XR5OverheadInstrumentPanel1280(*this), 1280);
    AddInstrumentPanel(new XR5PayloadInstrumentPanel1280(*this), 1280);

    // no VC (yet!) for the XR5
#ifdef UNDEF
    // add our VC panels
    AddInstrumentPanel(new VCPilotInstrumentPanel(*this, PANELVC_PILOT), 0);
    AddInstrumentPanel(new VCPassenger1InstrumentPanel(*this, PANELVC_PSNGR1), 0);
    AddInstrumentPanel(new VCPassenger2InstrumentPanel(*this, PANELVC_PSNGR2), 0);
    AddInstrumentPanel(new VCPassenger3InstrumentPanel(*this, PANELVC_PSNGR3), 0);
    AddInstrumentPanel(new VCPassenger4InstrumentPanel(*this, PANELVC_PSNGR4), 0);
#endif

}

// --------------------------------------------------------------
// Finalize vessel creation
// --------------------------------------------------------------
void XR5Vanguard::clbkPostCreation()
{
    // Invoke XR PostCreation code common to all XR vessels (code is in XRVessel.cpp)
    clbkPostCreationCommonXRCode();

    // configure RCS thruster groups and override the max thrust values if necessary
    ConfigureRCSJets(m_rcsDockingMode);

    // Initialize XR payload vessel data
    XRPayloadClassData::InitializeXRPayloadClassData();

    DefineMmuAirlock();    // update Mmu airlock data based on current active EVA port

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
    SetXRAnimation(anim_bay, bay_proc);
    SetXRAnimation(anim_crewElevator, crewElevator_proc);

    // NOTE: instrument panel initialization moved to clbkSetClassCaps (earlier) because the Post-2010-P1 Orbiter Beta invokes clbkLoadPanel before invoking clbkPostCreation

    // add our PreStep objects; these are invoked in order
    AddPreStep(new DrainBayFuelTanksPreStep(*this));  // need to do this *first* so the gauges are all correct later in the timestep (keeps main/SCRAM tanks full)
    AddPreStep(new RefreshSlotStatesPreStep(*this));  // do this early in case any other presteps look at the slot state
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
    AddPreStep(new AnimateGearCompressionPreStep(*this));
    AddPreStep(new RotateWheelsPreStep(*this));  // NOTE: this must be *after* AnimateGearCompressionPreStep so that we can detect whether the wheels are touching the ground or not for this timestep
    AddPreStep(new XR5NosewheelSteeringPreStep(*this));  // NOTE: this must be *after* AnimateGearCompressionPreStep so that we can detect whether the nosewheel is touching the ground or not for this timestep
    AddPreStep(new RefreshGrappleTargetsInDisplayRangePreStep(*this));
    AddPreStep(new UpdateVesselLightsPreStep(*this));
    AddPreStep(new ParkingBrakePreStep(*this));

    // WARNING: this must be invoked LAST in the sequence so that behavior is consistent across all pre-step methods
    AddPreStep(new UpdatePreviousFieldsPreStep(*this));

    // add our PostStep objects; these are invoked in order
    AddPostStep(new PreventAutoRefuelPostStep(*this));   // add this FIRST before our fuel callouts
    AddPostStep(new ComputeAccPostStep(*this));   // used by acc areas; computed only once per frame for efficiency
    // XRSound: AddPostStep(new AmbientSoundsPostStep       (*this));
    AddPostStep(new ShowWarningPostStep(*this));
    AddPostStep(new SetHullTempsPostStep(*this));
    AddPostStep(new SetSlopePostStep(*this));
    // do not include DoorSoundsPostStep here; we replace it below
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

    // NEW poststeps specific to the XR5
    AddPostStep(new SwitchTwoDPanelPostStep(*this));
    AddPostStep(new XR5AnimationPostStep(*this));
    AddPostStep(new XR5DoorSoundsPostStep(*this));  // replaces the standard DoorSoundsPostStep in the XR1 class
    AddPostStep(new HandleDockChangesForActiveAirlockPostStep(*this));  // switch active airlock automatically as necessary

#ifdef _DEBUG
    AddPostStep(new TestXRVesselCtrlPostStep(*this));      // for manual testing of new XRVesselCtrl methods via the debugger
#endif

    // set hidden elevator trim level
    SetControlSurfaceLevel(AIRCTRL_FLAP, m_hiddenElevatorTrimState);
}

