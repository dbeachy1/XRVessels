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
// XR3 Orbiter startup callbacks
// ==============================================================

#include "XR3Phoenix.h"

#include "XR3InstrumentPanels.h"
#include "XR1PreSteps.h"
#include "XR1PostSteps.h"
#include "XR1FuelPostSteps.h"
#include "XR1AnimationPoststep.h"
#include "XR3PreSteps.h"
#include "XR3PostSteps.h"

// ==============================================================
// Overloaded callback functions
// ==============================================================

// --------------------------------------------------------------
// Set vessel class parameters
// --------------------------------------------------------------
void XR3Phoenix::clbkSetClassCaps(FILEHANDLE cfg)
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
    SetSize(14.745);    // 1/2 ship's total width
    SetVisibilityLimit(7.5e-4, 1.5e-3);
    SetAlbedoRGB(_V(0.13, 0.20, 0.77));   // bluish
    SetGravityGradientDamping(20.0);    // ? same as DG for now

    SetCrossSections(_V(147.97, 486.33, 63.01));

    SetMaxWheelbrakeForce(MAX_WHEELBRAKE_FORCE);

    SetPMI(_V(88.20, 107.35, 27.03));

    SetDockParams(DOCKING_PORT_COORD, _V(0, 1, 0), _V(0, 0, -1));   // top-mounted port

    SetGearParameters(1.0);  // NOTE: must init touchdown points here with gear DOWN!  This will be called again later by clbkPostCreation to init the "real" state from scenario file.

    EnableTransponder(true);
    SetTransponderChannel(207); // XPDR = 118.35 MHz

    // init APU runtime callout timestamp
    MarkAPUActive();  // reset the APU idle warning callout time

    // enable IDS so we transmit a docking signal
    DOCKHANDLE hDock = GetDockHandle(0);    // primary docking port
    EnableIDS(hDock, true);
    SetIDSChannel(hDock, 209);  // DOCK = 113.45 MHz

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
    // XR5: const double particleMult = 1.5;
    const double particleMult = 1.0;    // XR3TODO: tweak this
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

#if 0  // not used
    // RCS
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
    // retros
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
#endif


    // handle new configurable ISP
    const double mainISP = GetXR1Config()->GetMainISP();

    /* From API Guide:
        Vessel coordinates are always defined so that the CG is at the origin (0,0,0). Therefore, a
        thruster located at (0,0,-10) and generating thrust in direction (0,0,1) would not generate
        torque.
    */

    // define thruster locations in meters from the ship's centerpoint
    const double shipLength = 36.75;
    const double rcsZHullDistance = (shipLength / 2) - 4.0;   // distance from Z centerline -> RCS fore and aft
    // XR3TODO: tweak this via rotation testing
    const double rcsXWingDistance = 12.0;                     // distance from X centerline -> simulated RCS on wings (not modeled visually) [XR2 was 8.0] : we cheat a bit here to improve rotation performance

    // main thrusters
    const double mainEngineZ = -(shipLength / 2) - 1;
    th_main[0] = CreateThruster(_V(-3.59, 0, mainEngineZ), _V(0, 0, 1), MAX_MAIN_THRUST[GetXR1Config()->MainEngineThrust], ph_main, mainISP, mainISP * ispscale);
    th_main[1] = CreateThruster(_V(3.59, 0, mainEngineZ), _V(0, 0, 1), MAX_MAIN_THRUST[GetXR1Config()->MainEngineThrust], ph_main, mainISP, mainISP * ispscale);

    thg_main = CreateThrusterGroup(th_main, 2, THGROUP_MAIN);
    SURFHANDLE mainExhaustTex = oapiRegisterExhaustTexture("XR3Phoenix\\ExhaustXR3");
    const double mainLscale = 12;
    const double mainWscale = 1.2;   // RADIUS
    const double mainExhaustZCoord = -13.5;  // to show the exhaust texture better

#define ADD_MAIN_EXHAUST(th, x, y)  \
    AddXRExhaust (th, mainLscale, mainWscale, _V(x, y, mainExhaustZCoord), _V(0,0,-1), mainExhaustTex); \
    AddExhaustStream (th, _V(x, y, mainExhaustZCoord - 13), &exhaust_main);                           \
    AddExhaustStream (th, _V(x, y, mainExhaustZCoord - 20), &contrail)

    // left side (viewed from rear)
    ADD_MAIN_EXHAUST(th_main[0], -7.25, 0);  // outboard
    ADD_MAIN_EXHAUST(th_main[0], -5.75, 0);  // inboard

    // right side (viewed from rear)
    ADD_MAIN_EXHAUST(th_main[1], 7.25, 0);  // outboard
    ADD_MAIN_EXHAUST(th_main[1], 5.75, 0);  // inboard

    // retro thrusters
    const double retroXCoord = 3.946;
    const double retroYCoord = 0.25;
    const double retroZCoord = 13.347;
    // Note: we use zero for the engine Y coordinate here to balance the thrust; this has nothing to do with the four visible retro engine exhausts
    th_retro[0] = CreateThruster(_V(-retroXCoord, 0, retroZCoord), _V(0, 0, -1.0), MAX_RETRO_THRUST, ph_main, mainISP, mainISP * ispscale);
    th_retro[1] = CreateThruster(_V(retroXCoord, 0, retroZCoord), _V(0, 0, -1.0), MAX_RETRO_THRUST, ph_main, mainISP, mainISP * ispscale);

    const double retroLscale = 3.0;
    const double retroWscale = 0.5;

#define ADD_RETRO_EXHAUST(th, x, y) \
    AddXRExhaust (th, retroLscale, retroWscale, _V(x, retroYCoord, retroZCoord), _V(0,0,1), mainExhaustTex);  \
    /* not used: AddExhaustStream (th, _V(x, retroYCoord, retroZCoord + 0.5), &exhaust_retro) */

    thg_retro = CreateThrusterGroup(th_retro, 2, THGROUP_RETRO);
    // add the four retro exhaust flames
    ADD_RETRO_EXHAUST(th_retro[0], -retroXCoord, retroYCoord);
    ADD_RETRO_EXHAUST(th_retro[0], -retroXCoord, -retroYCoord);
    ADD_RETRO_EXHAUST(th_retro[1], retroXCoord, retroYCoord);
    ADD_RETRO_EXHAUST(th_retro[1], retroXCoord, -retroYCoord);

    // hover thrusters (simplified)
    // The two aft hover engines are combined into a single "logical" thruster,
    // but exhaust is rendered separately for both
    const double hoverZ = 10.6;
    th_hover[0] = CreateThruster(_V(0, 0, hoverZ), _V(0, 1, 0), MAX_HOVER_THRUST[GetXR1Config()->HoverEngineThrust], ph_main, mainISP, mainISP * ispscale);
    th_hover[1] = CreateThruster(_V(0, 0, -hoverZ), _V(0, 1, 0), MAX_HOVER_THRUST[GetXR1Config()->HoverEngineThrust], ph_main, mainISP, mainISP * ispscale);
    thg_hover = CreateThrusterGroup(th_hover, 2, THGROUP_HOVER);

    const double hoverLscale = 2.0;  // shorter (old were too long)
    const double hoverWscale = 0.8;

#define ADD_HOVER_EXHAUST(th, x, y, z)  \
    AddXRExhaust (th, hoverLscale, hoverWscale, _V(x, y, z), _V(0,-1,0), mainExhaustTex); \
    AddExhaustStream (th, _V(x, y - 4.5, z), &exhaust_hover);                             \
    AddExhaustStream (th, _V(x, y - 7, z), &contrail)

    // define eight hover engine flames & particle streams
    // forward
    ADD_HOVER_EXHAUST(th_hover[0], 1.6, -1.1, 10.6);
    ADD_HOVER_EXHAUST(th_hover[0], -1.6, -1.1, 10.6);
    ADD_HOVER_EXHAUST(th_hover[0], 1.6, -1.1, 9.4);
    ADD_HOVER_EXHAUST(th_hover[0], -1.6, -1.1, 9.4);

    // aft 
    ADD_HOVER_EXHAUST(th_hover[1], 6.5, -0.9, -8.35);
    ADD_HOVER_EXHAUST(th_hover[1], -6.5, -0.9, -8.35);
    ADD_HOVER_EXHAUST(th_hover[1], 6.5, -0.9, -9.5);
    ADD_HOVER_EXHAUST(th_hover[1], -6.5, -0.9, -9.5);

    // set of attitude thrusters (idealised). The arrangement is such that no angular
    // momentum is created in linear mode, and no linear momentum is created in rotational mode.
    SURFHANDLE rcsExhaustTex = mainExhaustTex;

    // create RCS thrusters (not related to RCS exhaust)
    th_rcs[0] = CreateThruster(_V(0, 0, rcsZHullDistance), _V(0, 1, 0), GetRCSThrustMax(0), ph_rcs, mainISP);  // fore bottom (i.e., pushes UP from the BOTTOM of the hull)
    th_rcs[1] = CreateThruster(_V(0, 0, -rcsZHullDistance), _V(0, -1, 0), GetRCSThrustMax(1), ph_rcs, mainISP);  // aft top
    th_rcs[2] = CreateThruster(_V(0, 0, rcsZHullDistance), _V(0, -1, 0), GetRCSThrustMax(2), ph_rcs, mainISP);  // fore top
    th_rcs[3] = CreateThruster(_V(0, 0, -rcsZHullDistance), _V(0, 1, 0), GetRCSThrustMax(3), ph_rcs, mainISP);  // aft bottom

    const double rcsLscale = 1.0;      // XR2 was 0.6
    const double rcsWscale = 0.11;      // XR2 was 0.07

    // these are for the larger RCS jets
    const double rcsLscaleLarge = 1.5;
    const double rcsWscaleLarge = 0.16;

    const double rcsDepthModifier = 0;      // reduce depth of thruster flame firing so it shows up better
    const double rcsNoseDepthModifier = 0;  // top-mounted Y-axis nose RCS jets are deeper than standard jets
    const double rcsTailDepthModifier = 0;  // rear-mounted Z-axis RCS jets are deeper than standard jets

    const double exhaustDistance = 1.4;        // exhaust distance from thruster coordinates; XR2 was 1.4

// move exhaust smoke away from the RCS jet by a fixed distance
#define ADD_RCS_EXHAUST(th, coordsV, directionV)                                     \
        AddXRExhaust (th, rcsLscale, rcsWscale, coordsV, directionV, rcsExhaustTex)

#define ADD_LARGE_RCS_EXHAUST(th, coordsV, directionV)                                \
    AddXRExhaust (th, rcsLscaleLarge, rcsWscaleLarge, coordsV, directionV, rcsExhaustTex)

// compute actual RCS depth coordinate; this is necessary for hull-mounted RCS jets
#define RCS_DCOORD(c, dir) (c + (dir * rcsDepthModifier))
#define NOSE_RCS_DCOORD(c, dir) (c + (dir * rcsNoseDepthModifier))
#define TAIL_RCS_DCOORD(c, dir) (c + (dir * rcsTailDepthModifier))

    // fore bottom 
    // Note: the direction for these thrusters is a little wonky (not (0,-1,0) as normal), I think because Loru combined rotate ("bank") and pitch/translation in one thruster.
    ADD_LARGE_RCS_EXHAUST(th_rcs[0], _V(2.097, RCS_DCOORD(0.333, -1), 19.032), _V(0.643, -0.766, 0));     // ---->>> Front set: Pitch up / Bank Right / translation up
    ADD_LARGE_RCS_EXHAUST(th_rcs[0], _V(2.221, RCS_DCOORD(0.333, -1), 18.556), _V(0.643, -0.766, 0));

    ADD_LARGE_RCS_EXHAUST(th_rcs[0], _V(-2.097, RCS_DCOORD(0.333, -1), 19.032), _V(-0.643, -0.766, 0));   // ---->>> Front set: Pitch up / Bank left / translation up
    ADD_LARGE_RCS_EXHAUST(th_rcs[0], _V(-2.221, RCS_DCOORD(0.333, -1), 18.556), _V(-0.643, -0.766, 0));

    // aft top
    const double aftPitchXDelta = 8.25;  // Loru's supplied RCS coordinates of 8.5 were off for these jets, so I had to adjust them manually; hence the variable. 
    ADD_RCS_EXHAUST(th_rcs[1], _V(-aftPitchXDelta, RCS_DCOORD(0.45, 1), -10.693), _V(0, 1, 0));    // ---->>> Rear Top set: Pitch up / translation down / Bank Left
    ADD_RCS_EXHAUST(th_rcs[1], _V(-aftPitchXDelta, RCS_DCOORD(0.45, 1), -11.077), _V(0, 1, 0));

    ADD_RCS_EXHAUST(th_rcs[1], _V(aftPitchXDelta, RCS_DCOORD(0.45, 1), -10.693), _V(0, 1, 0));     // ---->>> Rear Top set: Pitch UP / translation down / Bank Right
    ADD_RCS_EXHAUST(th_rcs[1], _V(aftPitchXDelta, RCS_DCOORD(0.45, 1), -11.077), _V(0, 1, 0));

    // fore top
    ADD_RCS_EXHAUST(th_rcs[2], _V(-0.23, NOSE_RCS_DCOORD(0.95, 1), 20.248), _V(0, 1, 0));      // ---->>> Front set: Pitch down / translation down
    ADD_RCS_EXHAUST(th_rcs[2], _V(0.23, NOSE_RCS_DCOORD(0.95, 1), 20.248), _V(0, 1, 0));
    // XR3TODO: we may be missing a pair of RCS definitions here; need to test visually

    // aft bottom
    ADD_RCS_EXHAUST(th_rcs[3], _V(aftPitchXDelta, RCS_DCOORD(-0.4, -1), -10.693), _V(0, -1, 0));     // ---->>> Rear Bottom set: Pitch down /translation up / Bank left
    ADD_RCS_EXHAUST(th_rcs[3], _V(aftPitchXDelta, RCS_DCOORD(-0.4, -1), -11.077), _V(0, -1, 0));

    ADD_RCS_EXHAUST(th_rcs[3], _V(-aftPitchXDelta, RCS_DCOORD(-0.4, -1), -10.693), _V(0, -1, 0));     // ---->>> Rear Bottom set: Pitch down /translation up / Bank Right
    ADD_RCS_EXHAUST(th_rcs[3], _V(-aftPitchXDelta, RCS_DCOORD(-0.4, -1), -11.077), _V(0, -1, 0));

    th_rcs[4] = CreateThruster(_V(0, 0, rcsZHullDistance), _V(-1, 0, 0), GetRCSThrustMax(4), ph_rcs, mainISP);  // fore right side
    th_rcs[5] = CreateThruster(_V(0, 0, -rcsZHullDistance), _V(1, 0, 0), GetRCSThrustMax(5), ph_rcs, mainISP);  // aft left side
    th_rcs[6] = CreateThruster(_V(0, 0, rcsZHullDistance), _V(1, 0, 0), GetRCSThrustMax(6), ph_rcs, mainISP);  // fore left side
    th_rcs[7] = CreateThruster(_V(0, 0, -rcsZHullDistance), _V(-1, 0, 0), GetRCSThrustMax(7), ph_rcs, mainISP);  // aft right side

    // fore right side
    ADD_RCS_EXHAUST(th_rcs[4], _V(RCS_DCOORD(2.55, 1), 0.167, 17.949), _V(1, 0, 0));   // ---->>> Front set: Yaw Left / Translation Left
    ADD_RCS_EXHAUST(th_rcs[4], _V(RCS_DCOORD(2.55, 1), -0.224, 17.949), _V(1, 0, 0));

    // aft left side
    ADD_RCS_EXHAUST(th_rcs[5], _V(RCS_DCOORD(-7.9, -1.0), 0.7, -10.9), _V(-1, 0, 0));  // ---->>> Rear side set: Yaw left / translation right
    ADD_RCS_EXHAUST(th_rcs[5], _V(RCS_DCOORD(-7.9, -1.0), 0.7, -10.6), _V(-1, 0, 0));
    ADD_RCS_EXHAUST(th_rcs[5], _V(RCS_DCOORD(-7.9, -1.0), 0.7, -10.3), _V(-1, 0, 0));

    // fore left side
    ADD_RCS_EXHAUST(th_rcs[6], _V(RCS_DCOORD(-2.55, -1), 0.167, 17.949), _V(-1, 0, 0));  // ---->>> Front set: Yaw Right / Translation Right
    ADD_RCS_EXHAUST(th_rcs[6], _V(RCS_DCOORD(-2.55, -1), -0.224, 17.949), _V(-1, 0, 0));

    // aft right side
    ADD_RCS_EXHAUST(th_rcs[7], _V(RCS_DCOORD(7.9, 1.0), 0.7, -10.9), _V(1, 0, 0));  // ---->>> Rear side set: Yaw right / translation left
    ADD_RCS_EXHAUST(th_rcs[7], _V(RCS_DCOORD(7.9, 1.0), 0.7, -10.6), _V(1, 0, 0));
    ADD_RCS_EXHAUST(th_rcs[7], _V(RCS_DCOORD(7.9, 1.0), 0.7, -10.3), _V(1, 0, 0));

    // Define rotation thrusters (we cheat a bit here and put the rotation thrusters out on the wings, even though they aren't there on the mesh.  :)
    th_rcs[8] = CreateThruster(_V(rcsXWingDistance, 0, 0), _V(0, 1, 0), GetRCSThrustMax(8), ph_rcs, mainISP);    // right wing bottom
    th_rcs[9] = CreateThruster(_V(-rcsXWingDistance, 0, 0), _V(0, -1, 0), GetRCSThrustMax(9), ph_rcs, mainISP);    // left wing top
    th_rcs[10] = CreateThruster(_V(-rcsXWingDistance, 0, 0), _V(0, 1, 0), GetRCSThrustMax(10), ph_rcs, mainISP);    // left wing bottom
    th_rcs[11] = CreateThruster(_V(rcsXWingDistance, 0, 0), _V(0, -1, 0), GetRCSThrustMax(11), ph_rcs, mainISP);    // right wing top

    // Rotation exhaust: note that these exhausts share coordinates with other thrusters, since they do "double-duty."
    // These are logically mounted on the wings, but we re-use hull jets on the side to rotate the ship along the Z axis
    ADD_RCS_EXHAUST(th_rcs[8], _V(8.5, -0.4, -10.693), _V(0, -1, 0));  // right side bottom : ---->>> Rear Bottom set: Pitch down / translation up / Bank left
    ADD_RCS_EXHAUST(th_rcs[8], _V(8.5, -0.4, -11.077), _V(0, -1, 0));

    ADD_RCS_EXHAUST(th_rcs[9], _V(-8.5, 0.45, -10.693), _V(0, 1, 0));   // left side top : ----->>> Rear Top set: Pitch up / translation down / Bank Left
    ADD_RCS_EXHAUST(th_rcs[9], _V(-8.5, 0.45, -11.077), _V(0, 1, 0));

    ADD_RCS_EXHAUST(th_rcs[10], _V(-8.5, -0.4, -10.693), _V(0, -1, 0));  // left side bottom : ---->>> Rear Bottom set: Pitch down / translation up / Bank Right
    ADD_RCS_EXHAUST(th_rcs[10], _V(-8.5, -0.4, -11.077), _V(0, -1, 0));

    ADD_RCS_EXHAUST(th_rcs[10], _V(8.5, 0.45, -10.693), _V(0, -1, 0));  // right side top : ---->>> Rear Top set: Pitch UP / translation down / Bank Right
    ADD_RCS_EXHAUST(th_rcs[10], _V(8.5, 0.45, -11.077), _V(0, -1, 0));

    // put the RCS directly on the Y centerline so we don't induce any rotation
    th_rcs[12] = CreateThruster(_V(0, 0, -rcsZHullDistance), _V(0, 0, 1), GetRCSThrustMax(12), ph_rcs, mainISP);   // aft
    th_rcs[13] = CreateThruster(_V(0, 0, rcsZHullDistance), _V(0, 0, -1), GetRCSThrustMax(13), ph_rcs, mainISP);   // fore

    // Translation exhausts
    // aft Z axis : ---->>> Rear set: Translation forward
    ADD_LARGE_RCS_EXHAUST(th_rcs[12], _V(4.25, 0.25, TAIL_RCS_DCOORD(-11.8, -1)), _V(0, 0, -1));
    ADD_LARGE_RCS_EXHAUST(th_rcs[12], _V(4.25, -0.25, TAIL_RCS_DCOORD(-11.8, -1)), _V(0, 0, -1));
    ADD_LARGE_RCS_EXHAUST(th_rcs[12], _V(-4.25, 0.25, TAIL_RCS_DCOORD(-11.8, -1)), _V(0, 0, -1));
    ADD_LARGE_RCS_EXHAUST(th_rcs[12], _V(-4.25, -0.25, TAIL_RCS_DCOORD(-11.8, -1)), _V(0, 0, -1));

    // fore Z axis : ---->>> Front set: Translation back
    ADD_LARGE_RCS_EXHAUST(th_rcs[13], _V(0.4, 0.915, RCS_DCOORD(20.66, 1)), _V(0, 0, 1));
    ADD_LARGE_RCS_EXHAUST(th_rcs[13], _V(0.0, 0.915, RCS_DCOORD(20.66, 1)), _V(0, 0, 1));
    ADD_LARGE_RCS_EXHAUST(th_rcs[13], _V(-0.4, 0.915, RCS_DCOORD(20.66, 1)), _V(0, 0, 1));

    // NOTE: must invoke ConfigureRCSJets later after the scenario file is read

    // **************** scramjet definitions ********************

    const double scramX = 1.0;  // distance from centerline
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

    // XR3TODO: test this visually once the mesh is animated 
    const double scramY = 1.54;
    ph = AddExhaustStream(th_scram[0], _V(-scramX, -1.54, -9.0 + scramDelta), &exhaust_scram);
    if (ph != nullptr)
        oapiParticleSetLevelRef(ph, scram_intensity + 0);

    ph = AddExhaustStream(th_scram[1], _V(scramX, -1.54, -9.0 + scramDelta), &exhaust_scram);
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
    m_ctrlSurfacesDeltaZ = -rcsZHullDistance;     // distance from center of model to center of control surfaces, Z axis
    m_aileronDeltaX = 13.0;      // distance from center of ship to center of aileron, X direction : this is appoximate, I don't have an exact number from Loru
    XR1Multiplier = XR1_MULTIPLIER;        // control surface area vs. the XR1 

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
    // XR3TODO: tweak this as necessary to fix nose-up push
    CreateControlSurface(AIRCTRL_FLAP, 0.3 * XR1Multiplier * 7, 1.5, _V(0, 0, m_ctrlSurfacesDeltaZ), AIRCTRL_AXIS_XPOS);  // no animation for this!
    m_hiddenElevatorTrimState = HIDDEN_ELEVATOR_TRIM_STATE;        // set to a member variable in case we want to change it in flight later
    // Note: cannot set the level here; it is reset by Orbiter later.

    const double xr1VariableDragModifier = XR1_MULTIPLIER;    // this is the empty mass ratio of the XR3:XR1
    // XR3TODO:  tweak these drag element coordinates
    // we need these goofy const casts to force the linker to link with the 'const double *' version of CreateVariableDragElement instead of the legacy 'double *' version of the function (which still exists but is deprecated and causes warnings in orbiter.log)
    CreateVariableDragElement(const_cast<const double*>(&rcover_proc), 0.2 * xr1VariableDragModifier, _V(0, 0.0, 26.972));     // retro covers
    CreateVariableDragElement(const_cast<const double*>(&radiator_proc), 0.4 * xr1VariableDragModifier, _V(0, 3.274, -rcsZHullDistance + 5));     // radiators
    CreateVariableDragElement(const_cast<const double*>(&bay_proc), 7.0 * xr1VariableDragModifier, _V(0, 8.01, -rcsZHullDistance + 8));       // bay doors (drag is at rear of bay)
    CreateVariableDragElement(const_cast<const double*>(&gear_proc), 0.8 * xr1VariableDragModifier, _V(0, -4, 4.34));          // landing gear
    CreateVariableDragElement(const_cast<const double*>(&nose_proc), 2.1 * xr1VariableDragModifier, _V(0, 3.06, 8.6));          // docking port
    CreateVariableDragElement(const_cast<const double*>(&brake_proc), 4.0 * xr1VariableDragModifier, _V(0, 0, m_ctrlSurfacesDeltaZ));  // airbrake (do not induce a rotational moment here)
    // XR3TODO: convert crewElevator_proc and associated code into ladder to the ground: CreateVariableDragElement(&crewElevator_proc, 6.0 * xr1VariableDragModifier, _V(-3.358, -6.51, 6.371));  // elevator (note: elevator is off-center)

    const double dragMultiplier = XR1_MULTIPLIER;
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
    // TODO: get these beacon coordinates from Loru
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

    const double sizeMultiplier = 1.5;  // XR3TODO: tweak this beacon size
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
    const double mainEnginePointLightPower = 100 * 5.94;  // XR3 engines are 5.94 times as powerful as the XR1's
    const double zMainLightDelta = -3.0;  // need more delta here because the exhaust is sunk into the engine bell  XR3TODO: tweak this value
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
        const double hoverEnginePointLightPower = mainEnginePointLightPower * 0.7567;  // hovers are .7567 the thrust of the mains (different engine count notwithstanding)
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
    // XR3TODO: get these two sets of landing light coordinates from Loru (these are on the wings)
    m_pSpotlights[0] = static_cast<SpotLight*>(AddSpotLight(_V(10.628, -0.055, 3.586), _V(0, 0, 1), 250, 1e-3, 0, 1e-3, RAD * 25, RAD * 60, col_white, col_white, col_a));
    m_pSpotlights[1] = static_cast<SpotLight*>(AddSpotLight(_V(-10.628, -0.055, 3.586), _V(0, 0, 1), 250, 1e-3, 0, 1e-3, RAD * 25, RAD * 60, col_white, col_white, col_a));
    // docking port 
    // XR3TODO: tweak the Y coordinate here so the lights are against the hull
    m_pSpotlights[2] = static_cast<SpotLight*>(AddSpotLight(_V(-1.66, 3.060, 8.60), _V(0, 1, 0), 250, 1e-3, 0, 1e-3, RAD * 25, RAD * 60, col_white, col_white, col_a));
    m_pSpotlights[3] = static_cast<SpotLight*>(AddSpotLight(_V(1.66, 3.060, 8.60), _V(0, 1, 0), 250, 1e-3, 0, 1e-3, RAD * 25, RAD * 60, col_white, col_white, col_a));

    // turn all spotlights off by default
    for (int i = 0; i < SPOTLIGHT_COUNT; i++)
        m_pSpotlights[i]->Activate(false);

    // load meshes
    // no VC: vcmesh_tpl = oapiLoadMeshGlobal("dg-xr1\\deltaglidercockpit-xr1");  // VC mesh
    vcmesh_tpl = nullptr;  // no VC; must set this to null so the superclass won't try to use it
    exmesh_tpl = oapiLoadMeshGlobal("XR3Phoenix\\XR3Phoenix");         // exterior mesh

    m_exteriorMeshIndex = AddMesh(exmesh_tpl);  // save so we can modify the mesh later
    SetMeshVisibilityMode(m_exteriorMeshIndex, MESHVIS_EXTERNAL);
    // no VC: SetMeshVisibilityMode(AddMesh(vcmesh_tpl), MESHVIS_VC);

    ///////////////////////////////////////////////////////////////////////
    // Init UMmu
    ///////////////////////////////////////////////////////////////////////
#ifdef MMU
    const int ummuStatus = UMmu.InitUmmu(GetHandle());  // returns 1 if ok and other number if not

    // UMmu is REQUIRED!
    if (ummuStatus != 1)
        FatalError("UMmu not installed!  You must install Universal Mmu 3.0 or newer in order to use the XR3; visit http://www.alteaaerospace.com for more information.");

    // validate UMmu version and write it to the log file
    const float ummuVersion = CONST_UMMU_XR3(this).GetUserUMmuVersion();
    if (ummuVersion < 2.0)
    {
        char msg[256];
        sprintf(msg, "UMmu version %.2f is installed, but the XR3 requires Universal Mmu 3.0 or higher; visit http://www.alteaaerospace.com for more information.", ummuVersion);
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
    AddInstrumentPanel(new XR3MainInstrumentPanel1920(*this), 1920);
    AddInstrumentPanel(new XR3UpperInstrumentPanel1920(*this), 1920);
    AddInstrumentPanel(new XR3LowerInstrumentPanel1920(*this), 1920);
    AddInstrumentPanel(new XR3OverheadInstrumentPanel1920(*this), 1920);
    AddInstrumentPanel(new XR3PayloadInstrumentPanel1920(*this), 1920);

    // 1600-pixel-wide panels
    AddInstrumentPanel(new XR3MainInstrumentPanel1600(*this), 1600);
    AddInstrumentPanel(new XR3UpperInstrumentPanel1600(*this), 1600);
    AddInstrumentPanel(new XR3LowerInstrumentPanel1600(*this), 1600);
    AddInstrumentPanel(new XR3OverheadInstrumentPanel1600(*this), 1600);
    AddInstrumentPanel(new XR3PayloadInstrumentPanel1600(*this), 1600);

    // 1280-pixel-wide panels
    AddInstrumentPanel(new XR3MainInstrumentPanel1280(*this), 1280);
    AddInstrumentPanel(new XR3UpperInstrumentPanel1280(*this), 1280);
    AddInstrumentPanel(new XR3LowerInstrumentPanel1280(*this), 1280);
    AddInstrumentPanel(new XR3OverheadInstrumentPanel1280(*this), 1280);
    AddInstrumentPanel(new XR3PayloadInstrumentPanel1280(*this), 1280);

    // XR3TODO: uncomment this for VC
    // no VC yet for the XR3
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
void XR3Phoenix::clbkPostCreation()
{
    // Invoke XR PostCreation code common to all XR vessels (code is in XRVessel.cpp)
    clbkPostCreationCommonXRCode();

    // configure RCS thruster groups and override the max thrust values if necessary
    ConfigureRCSJets(m_rcsDockingMode);

    // Initialize XR payload vessel data
    XRPayloadClassData::InitializeXRPayloadClassData();

    DefineMmuAirlock();    // update UMmu airlock data based on current active EVA port

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
    // XR3TODO: convert crewElevator_proc and associated code into ladder to the ground: SetXRAnimation(anim_crewElevator, crewElevator_proc);

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
    AddPreStep(new XR3NosewheelSteeringPreStep(*this));  // NOTE: this must be *after* AnimateGearCompressionPreStep so that we can detect whether the nosewheel is touching the ground or not for this timestep
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

    // NEW poststeps specific to the XR3
    AddPostStep(new SwitchTwoDPanelPostStep(*this));
    AddPostStep(new XR3AnimationPostStep(*this));
    AddPostStep(new XR3DoorSoundsPostStep(*this));  // replaces the standard DoorSoundsPostStep in the XR1 class
    AddPostStep(new HandleDockChangesForActiveAirlockPostStep(*this));  // switch active airlock automatically as necessary

#ifdef _DEBUG
    AddPostStep(new TestXRVesselCtrlPostStep(*this));      // for manual testing of new XRVesselCtrl methods via the debugger
#endif

    // set hidden elevator trim level
    SetControlSurfaceLevel(AIRCTRL_FLAP, m_hiddenElevatorTrimState);
}
