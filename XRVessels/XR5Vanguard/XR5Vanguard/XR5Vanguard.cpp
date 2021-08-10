// ==============================================================
// XR5Vanguard implementation class
//
// Copyright 2007-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR5Vanguard.cpp
// ==============================================================

#define ORBITER_MODULE

#include "XR5Vanguard.h"
#include "DlgCtrl.h"
#include <stdio.h>

#include "XR5AreaIDs.h"  
#include "XR5InstrumentPanels.h"
#include "XR1PreSteps.h"
#include "XR1PostSteps.h"
#include "XR1FuelPostSteps.h"
#include "XR1AnimationPoststep.h"

#include "XR5Globals.h"
#include "XR5PreSteps.h"
#include "XR5PostSteps.h"
#include "XRPayload.h"
#include "XR5PayloadBay.h"

#include "meshres.h"

// ==============================================================
// API callback interface
// ==============================================================

// --------------------------------------------------------------
// Module initialisation
// --------------------------------------------------------------
DLLCLBK void InitModule (HINSTANCE hModule)
{
    g_hDLL = hModule;
    oapiRegisterCustomControls(hModule);
}

// --------------------------------------------------------------
// Module cleanup
// --------------------------------------------------------------
DLLCLBK void ExitModule (HINSTANCE hModule)
{
    oapiUnregisterCustomControls(hModule);
    XRPayloadClassData::Terminate();     // clean up global cache
}

// --------------------------------------------------------------
// Vessel initialisation
// --------------------------------------------------------------
DLLCLBK VESSEL *ovcInit (OBJHANDLE vessel, int flightmodel)
{
#ifdef _DEBUG
    // NOTE: _CRTDBG_CHECK_ALWAYS_DF is too slow
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF |
                   _CRTDBG_CHECK_CRT_DF | 
                   _CRTDBG_LEAK_CHECK_DF); 
#endif

    return new XR5Vanguard(vessel, flightmodel, new XR5ConfigFileParser());
}

// --------------------------------------------------------------
// Vessel cleanup
// --------------------------------------------------------------

// NOTE: must receive this as a VESSEL2 ptr because that's how Orbiter calls it
DLLCLBK void ovcExit(VESSEL2 *vessel)
{
    // NOTE: in order to free up VESSEL3 data, you must add an empty virtual destructor to the VESSEL2 class in VesselAPI.h
    
    // This is a hack so that the VESSEL3_EXT, XR5 and VESSEL3 destructors will be invoked.
    // Invokes XR5 destructor -> XR1 destructor -> VESSEL3_EXT destructor -> VESSEL3 destructor
    VESSEL3_EXT *pXR5 = reinterpret_cast<VESSEL3_EXT *>( (((void **)vessel)-1) );  // bump vptr to VESSEL3_EXT subclass, which has a virtual destructor
    delete pXR5;
}

// ==============================================================
// Airfoil coefficient functions
// Return lift, moment and zero-lift drag coefficients as a
// function of angle of attack (alpha or beta)
// ==============================================================

///#define PROFILE_DRAG 0.015
// NEW to fix "floaty" landings
// XR5 ORG: #define PROFILE_DRAG 0.030
// improve glide performance for the Vanguard
#define PROFILE_DRAG 0.015

// 1. vertical lift component (wings and body)
void VLiftCoeff (VESSEL *v, double aoa, double M, double Re, void *context, double *cl, double *cm, double *cd)
{
    const int nabsc = 9;
    // ORG: static const double AOA[nabsc] = {-180*RAD,-60*RAD,-30*RAD, -2*RAD, 15*RAD,20*RAD,25*RAD,60*RAD,180*RAD};
    // DG3 follows:
    static const double AOA[nabsc] =         {-180*RAD,-60*RAD,-30*RAD, -1*RAD, 15*RAD,20*RAD,25*RAD,50*RAD,180*RAD};

    // ORG: static const double CL[nabsc]  = {       0,      0,   -0.4,      0,    0.7,     1,   0.8,     0,      0};
    // DG3 follows:
    // NEW: static const double CL[nabsc]  = {       0,      0,   -0.4,      0,    0.7,     1,   0.2,     0,      0};
    // XR5 ORG: static const double CL[nabsc]  =         {       0,      0,   -0.4,      0,    0.7,     0.5, 0.2,     0,      0};
    static const double CL[nabsc]  =         {       0,      0,   -0.15,      0,    0.7,     0.5, 0.2,     0,      0};  // decrease negative lift to fix nose-down attitude hold problems

    // ORG: static const double CM[nabsc]  = {       0,      0,  0.014, 0.0039, -0.006,-0.008,-0.010,     0,      0};
    // DG3 follows:
    //static const double CM[nabsc]  =         {       0,  0.006,  0.014, 0.0034,-0.0054,-0.024,-0.00001,   0,      0};
    static const double CM[nabsc]  =         {       0,      0,  0.014, 0.0039, -0.006,-0.008,-0.010,     0,      0};

    int i=0;    
    for (i = 0; i < nabsc-1 && AOA[i+1] < aoa; i++);
    double f = (aoa-AOA[i]) / (AOA[i+1]-AOA[i]);
    *cl = CL[i] + (CL[i+1]-CL[i]) * f;  // aoa-dependent lift coefficient
    *cm = CM[i] + (CM[i+1]-CM[i]) * f;  // aoa-dependent moment coefficient
    double saoa = sin(aoa);
    double pd = PROFILE_DRAG + 0.4*saoa*saoa;  // profile drag
    *cd = pd + oapiGetInducedDrag (*cl, WING_ASPECT_RATIO, WING_EFFICIENCY_FACTOR) + oapiGetWaveDrag (M, 0.75, 1.0, 1.1, 0.04);
    // profile drag + (lift-)induced drag + transonic/supersonic wave (compressibility) drag
}

// 2. horizontal lift component (vertical stabilisers and body)

void HLiftCoeff (VESSEL *v, double beta, double M, double Re, void *context, double *cl, double *cm, double *cd)
{
    const int nabsc = 8;
    // DG3 BETA unchanged
    static const double BETA[nabsc] = {-180*RAD,-135*RAD,-90*RAD,-45*RAD,45*RAD,90*RAD,135*RAD,180*RAD};
    // DG3 CL unchanged
    static const double CL[nabsc]   = {       0,    +0.3,      0,   -0.3,  +0.3,     0,   -0.3,      0};

    int i=0;    
    for (i = 0; i < nabsc-1 && BETA[i+1] < beta; i++);
    *cl = CL[i] + (CL[i+1]-CL[i]) * (beta-BETA[i]) / (BETA[i+1]-BETA[i]);
    *cm = 0.0;
    *cd = PROFILE_DRAG + oapiGetInducedDrag (*cl, 1.5, 0.6) + oapiGetWaveDrag (M, 0.75, 1.0, 1.1, 0.04);
}

//
// Constructor
//
XR5Vanguard::XR5Vanguard(OBJHANDLE hObj, int fmodel, XR5ConfigFileParser *pConfigFileParser) : 
    DeltaGliderXR1(hObj, fmodel, pConfigFileParser),
    m_rcsDockingMode(false), m_rcsDockingModeAtKillrotStart(false),
    m_hiddenElevatorTrimState(0), m_activeEVAPort(DOCKING_PORT)
{
    // init new XR5 warning lights
    for (int i=0; i < XR5_WARNING_LIGHT_COUNT; i++)
        m_xr5WarningLights[i] = false;  // not lit

    // init new doors
    crewElevator_status = DOOR_CLOSED;
    crewElevator_proc   = 0.0;
    bay_status          = DOOR_CLOSED;
    bay_proc            = 0.0;

    // replace the data HUD font with a smaller one
    // XR1 ORG: m_pDataHudFont = CreateFont(20, 0, 0, 0, 700, 0, 0, 0, 0, 0, 0, NONANTIALIASED_QUALITY, 0, "Tahoma");
    // XR1 ORG: m_pDataHudFontSize = 22;      // includes spacing
    /* MATCHES XR1 FONT NOW 
    oapiReleaseFont(m_pDataHudFont);        // free existing front created by the XR1's constructor
    m_pDataHudFont = oapiCreateFont(22, true, "Tahoma", FONT_BOLD);  // will be freed by the XR1's destructor
    m_pDataHudFontSize = 18;      // includes spacing
    */
}

// 
// Destructor
//
XR5Vanguard::~XR5Vanguard()
{
    // Note: our superclass handles cleanup of m_pPayloadBay and s_hPayloadEditorDialog
}

// ==============================================================
// Overloaded callback functions
// ==============================================================

// --------------------------------------------------------------
// Set vessel class parameters
// --------------------------------------------------------------
void XR5Vanguard::clbkSetClassCaps (FILEHANDLE cfg)
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
    SetSize (38.335);    // 1/2 ship's width
    SetVisibilityLimit (7.5e-4, 1.5e-3);
    SetAlbedoRGB (_V(0.13,0.20,0.77));   // bluish
    SetGravityGradientDamping (20.0);    // ? same as DG for now
    
    SetCrossSections (_V(543.82, 1962.75, 330.97));     

    SetMaxWheelbrakeForce(MAX_WHEELBRAKE_FORCE);  

    SetPMI (_V(317.35, 305.08, 219.45));
    
    SetDockParams (DOCKING_PORT_COORD, _V(0,1,0), _V(0,0,-1));   // top-mounted port

    SetGearParameters(1.0);  // NOTE: must init touchdown points here with gear DOWN!  This will be called again later by clbkPostCreation to init the "real" state from scenario file.

    EnableTransponder (true);
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
    max_scramfuel  = TANK2_CAPACITY;

    // NOTE: Orbiter seems to reset this to zero later, since he expects the scenario file to be read.
    // WARNING: do NOT init these values to > 0, because Orbiter will NOT set the tank value if the fraction is 
    // zero in the scenario file.
    ph_main  = CreatePropellantResource(max_rocketfuel);      // main tank (fuel + oxydant)
    ph_rcs   = CreatePropellantResource(RCS_FUEL_CAPACITY);   // RCS tank  (fuel + oxydant)
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
    th_main[0] = CreateThruster(_V(-3.59, 0, mainEngineZ), _V(0, 0, 1), MAX_MAIN_THRUST[GetXR1Config()->MainEngineThrust], ph_main, mainISP, mainISP*ispscale);
    th_main[1] = CreateThruster(_V( 3.59, 0, mainEngineZ), _V(0, 0, 1), MAX_MAIN_THRUST[GetXR1Config()->MainEngineThrust], ph_main, mainISP, mainISP*ispscale);

    thg_main = CreateThrusterGroup (th_main, 2, THGROUP_MAIN);
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
    th_retro[0] = CreateThruster (_V(-retroXCoord, 0, retroZCoord), _V(0, 0, -1.0), MAX_RETRO_THRUST, ph_main, mainISP, mainISP*ispscale);
    th_retro[1] = CreateThruster (_V( retroXCoord, 0, retroZCoord), _V(0, 0, -1.0), MAX_RETRO_THRUST, ph_main, mainISP, mainISP*ispscale);

    const double retroLscale = 3.0;
    // const double retroWscale = 0.1315;  // too narrow
    // 1.0 ORG: const double retroWscale = 0.19;  
    // Pre-1.7 (XR1 1.9 group): const double retroWscale = 0.140;  
    const double retroWscale = 0.18; // show the texture better

#define ADD_RETRO_EXHAUST(th, x) \
    AddXRExhaust (th, retroLscale, retroWscale, _V(x, retroYCoord, retroZCoord), _V(0,0,1), mainExhaustTex);  \
    AddExhaustStream (th, _V(x, retroYCoord, retroZCoord + 0.5), &exhaust_retro)

    thg_retro = CreateThrusterGroup (th_retro, 2, THGROUP_RETRO);
    ADD_RETRO_EXHAUST(th_retro[0], -retroXCoord);
    ADD_RETRO_EXHAUST(th_retro[1], retroXCoord);
    
    // hover thrusters (simplified)
    // The two aft hover engines are combined into a single "logical" thruster,
    // but exhaust is rendered separately for both
    th_hover[0] = CreateThruster (_V(0, 0, 14.32), _V(0,1,0), MAX_HOVER_THRUST[GetXR1Config()->HoverEngineThrust], ph_main, mainISP, mainISP*ispscale);
    th_hover[1] = CreateThruster (_V(0, 0,-14.32), _V(0,1,0), MAX_HOVER_THRUST[GetXR1Config()->HoverEngineThrust], ph_main, mainISP, mainISP*ispscale);
    thg_hover = CreateThrusterGroup (th_hover, 2, THGROUP_HOVER);

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
    ADD_HOVER_EXHAUST(th_hover[0],  3.294, -1.46, 12.799);
    ADD_HOVER_EXHAUST(th_hover[0], -3.297, -1.46, 12.799);

    // aft starboard  (right-hand side looking forward)
    ADD_HOVER_EXHAUST(th_hover[1], -22.324, -1.091, -13.633);
    ADD_HOVER_EXHAUST(th_hover[1], -22.324, -1.091, -17.632);

    // aft port
    ADD_HOVER_EXHAUST(th_hover[1],  22.324, -1.091, -13.633);
    ADD_HOVER_EXHAUST(th_hover[1],  22.324, -1.091, -17.632);

    // set of attitude thrusters (idealised). The arrangement is such that no angular
    // momentum is created in linear mode, and no linear momentum is created in rotational mode.
    SURFHANDLE rcsExhaustTex = mainExhaustTex;

    // create RCS thrusters
    th_rcs[0] = CreateThruster (_V(0, 0, rcsZHullDistance), _V(0, 1,0), GetRCSThrustMax(0), ph_rcs, mainISP);  // fore bottom (i.e., pushes UP from the BOTTOM of the hull)
    th_rcs[1] = CreateThruster (_V(0, 0,-rcsZHullDistance), _V(0,-1,0), GetRCSThrustMax(1), ph_rcs, mainISP);  // aft top
    th_rcs[2] = CreateThruster (_V(0, 0, rcsZHullDistance), _V(0,-1,0), GetRCSThrustMax(2), ph_rcs, mainISP);  // fore top
    th_rcs[3] = CreateThruster (_V(0, 0,-rcsZHullDistance), _V(0, 1,0), GetRCSThrustMax(3), ph_rcs, mainISP);  // aft bottom

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

    ADD_RCS_EXHAUST (th_rcs[0], _V( 2.613, RCS_DCOORD(-0.284, -1), 25.532), _V(0,-1,0));  // fore bottom
    ADD_RCS_EXHAUST (th_rcs[0], _V( 2.411, RCS_DCOORD(-0.273, -1), 26.039), _V(0,-1,0));  
    ADD_RCS_EXHAUST (th_rcs[0], _V(-2.618, RCS_DCOORD(-0.284, -1), 25.532), _V(0,-1,0));
    ADD_RCS_EXHAUST (th_rcs[0], _V(-2.416, RCS_DCOORD(-0.273, -1), 26.039), _V(0,-1,0));

    ADD_RCS_EXHAUST(th_rcs[1], _V(-9.402, RCS_DCOORD(0.241, 1.5), -24.299), _V(0,1,0));  // aft top
    ADD_RCS_EXHAUST(th_rcs[1], _V(-9.485, RCS_DCOORD(0.241, 1.5), -23.936), _V(0,1,0)); 
    ADD_RCS_EXHAUST(th_rcs[1], _V( 9.493, RCS_DCOORD(0.241, 1.5), -23.936), _V(0,1,0)); 
    ADD_RCS_EXHAUST(th_rcs[1], _V( 9.410, RCS_DCOORD(0.241, 1.5), -24.299), _V(0,1,0)); 

    ADD_RCS_EXHAUST (th_rcs[2], _V( 2.646, NOSE_RCS_DCOORD(2.133, 1), 26.390), _V(0,1,0));  // fore top
    ADD_RCS_EXHAUST (th_rcs[2], _V( 2.510, NOSE_RCS_DCOORD(2.110, 1), 26.918), _V(0,1,0));
    ADD_RCS_EXHAUST (th_rcs[2], _V(-2.646, NOSE_RCS_DCOORD(2.133, 1), 26.390), _V(0,1,0));
    ADD_RCS_EXHAUST (th_rcs[2], _V(-2.510, NOSE_RCS_DCOORD(2.110, 1), 26.918), _V(0,1,0)); 

    ADD_RCS_EXHAUST(th_rcs[3], _V( 9.410, RCS_DCOORD(-0.04, -1), -24.572), _V(0,-1,0));  // aft bottom
    ADD_RCS_EXHAUST(th_rcs[3], _V( 9.410, RCS_DCOORD(-0.04, -1), -24.916), _V(0,-1,0)); 
    ADD_RCS_EXHAUST(th_rcs[3], _V(-9.402, RCS_DCOORD(-0.04, -1), -24.572), _V(0,-1,0)); 
    ADD_RCS_EXHAUST(th_rcs[3], _V(-9.402, RCS_DCOORD(-0.04, -1), -24.916), _V(0,-1,0)); 

    th_rcs[4] = CreateThruster (_V(0, 0, rcsZHullDistance), _V(-1,0,0), GetRCSThrustMax(4), ph_rcs, mainISP);  // fore right side
    th_rcs[5] = CreateThruster (_V(0, 0,-rcsZHullDistance), _V( 1,0,0), GetRCSThrustMax(5), ph_rcs, mainISP);  // aft left side
    th_rcs[6] = CreateThruster (_V(0, 0, rcsZHullDistance), _V( 1,0,0), GetRCSThrustMax(6), ph_rcs, mainISP);  // fore left side
    th_rcs[7] = CreateThruster (_V(0, 0,-rcsZHullDistance), _V(-1,0,0), GetRCSThrustMax(7), ph_rcs, mainISP);  // aft right side

    ADD_RCS_EXHAUST (th_rcs[4], _V( RCS_DCOORD(1.999, 1), 3.150, 26.150), _V(1,0,0));   // fore right side
    ADD_RCS_EXHAUST (th_rcs[4], _V( RCS_DCOORD(1.999, 1), 3.012, 26.658), _V(1,0,0)); 
    ADD_RCS_EXHAUST (th_rcs[4], _V( RCS_DCOORD(2.390, 1),-0.455, 25.789), _V(1,0,0)); 
    ADD_RCS_EXHAUST (th_rcs[4], _V( RCS_DCOORD(2.644, 1),-0.480, 25.276), _V(1,0,0)); 

    ADD_RCS_EXHAUST (th_rcs[5], _V( RCS_DCOORD(-8.559, -1.5), 0.891, -25.188), _V(-1,0,0));  // aft left side
    ADD_RCS_EXHAUST (th_rcs[5], _V( RCS_DCOORD(-8.664, -1.5), 0.891, -24.768), _V(-1,0,0));

    ADD_RCS_EXHAUST (th_rcs[6], _V(RCS_DCOORD(-1.999, -1), 3.012, 26.658), _V(-1,0,0));  // fore left side
    ADD_RCS_EXHAUST (th_rcs[6], _V(RCS_DCOORD(-1.999, -1), 3.150, 26.150), _V(-1,0,0));  
    ADD_RCS_EXHAUST (th_rcs[6], _V(RCS_DCOORD(-2.395, -1),-0.455, 25.789), _V(-1,0,0));  
    ADD_RCS_EXHAUST (th_rcs[6], _V(RCS_DCOORD(-2.650, -1),-0.480, 25.276), _V(-1,0,0));  

    ADD_RCS_EXHAUST (th_rcs[7], _V( RCS_DCOORD( 8.568, 1.5), 0.891, -25.188), _V(1,0,0));  // aft right side
    ADD_RCS_EXHAUST (th_rcs[7], _V( RCS_DCOORD( 8.673, 1.5), 0.891, -24.768), _V(1,0,0));

    th_rcs[8]  = CreateThruster (_V( rcsXWingDistance, 0, 0), _V(0, 1,0), GetRCSThrustMax(8),  ph_rcs, mainISP);    // right wing bottom
    th_rcs[9]  = CreateThruster (_V(-rcsXWingDistance, 0, 0), _V(0,-1,0), GetRCSThrustMax(9),  ph_rcs, mainISP);    // left wing top
    th_rcs[10] = CreateThruster (_V(-rcsXWingDistance, 0, 0), _V(0, 1,0), GetRCSThrustMax(10), ph_rcs, mainISP);    // left wing bottom
    th_rcs[11] = CreateThruster (_V( rcsXWingDistance, 0, 0), _V(0,-1,0), GetRCSThrustMax(11), ph_rcs, mainISP);    // right wing top
    
    // wing exhaust does not get depth adjustment
    ADD_RCS_EXHAUST (th_rcs[8], _V( 18.876, -0.816, -7.794), _V(0,-1,0));  // right wing bottom
    ADD_RCS_EXHAUST (th_rcs[9], _V(-18.886, 0.839, -7.493), _V(0, 1,0));   // left wing top
    ADD_RCS_EXHAUST (th_rcs[10], _V(-18.868,-0.816, -7.546), _V(0,-1,0));  // left wing bottom
    ADD_RCS_EXHAUST (th_rcs[11], _V( 18.886, 0.839, -7.493), _V(0, 1,0));  // right wing top
    
    // put the RCS directly on the Y centerline so we don't induce any rotation
    th_rcs[12] = CreateThruster (_V(0, 0,-rcsZHullDistance), _V(0, 0, 1), GetRCSThrustMax(12), ph_rcs, mainISP);   // aft
    th_rcs[13] = CreateThruster (_V(0, 0, rcsZHullDistance), _V(0, 0,-1), GetRCSThrustMax(13), ph_rcs, mainISP);   // fore

    ADD_RCS_EXHAUST (th_rcs[12],  _V( 9.581, 0.401, TAIL_RCS_DCOORD(-24.108, -1)), _V(0,0,-1));   // aft Z axis
    ADD_RCS_EXHAUST (th_rcs[12],  _V( 9.723, 0.074, TAIL_RCS_DCOORD(-24.108, -1)), _V(0,0,-1));
    ADD_RCS_EXHAUST (th_rcs[12],  _V(-9.714, 0.074, TAIL_RCS_DCOORD(-24.108, -1)), _V(0,0,-1));
    ADD_RCS_EXHAUST (th_rcs[12],  _V(-9.572, 0.401, TAIL_RCS_DCOORD(-24.108, -1)), _V(0,0,-1));

    ADD_RCS_EXHAUST (th_rcs[13], _V(-1.974, 2.546, RCS_DCOORD(27.685, 1)), _V(0,0,1));  // fore Z axis
    ADD_RCS_EXHAUST (th_rcs[13], _V(-2.121, 2.250, RCS_DCOORD(27.625, 1)), _V(0,0,1));  
    ADD_RCS_EXHAUST (th_rcs[13], _V( 1.974, 2.546, RCS_DCOORD(27.685, 1)), _V(0,0,1)); 
    ADD_RCS_EXHAUST (th_rcs[13], _V( 2.121, 2.250, RCS_DCOORD(27.625, 1)), _V(0,0,1));

    // NOTE: must invoke ConfigureRCSJets later after the scenario file is read

    // **************** scramjet definitions ********************
    
    const double scramX = 1.931;  // distance from centerline
    for (int i = 0; i < 2; i++) 
    {
        th_scram[i] = CreateThruster (_V((i ? scramX : -scramX), 0, -rcsZHullDistance), _V(0, 0, 1), 0, ph_scram, 0);
        ramjet->AddThrusterDefinition (th_scram[i], SCRAM_FHV[GetXR1Config()->SCRAMfhv],
            SCRAM_INTAKE_AREA, SCRAM_INTERNAL_TEMAX, GetXR1Config()->GetScramMaxEffectiveDMF());
    }
    
    // thrust rating and ISP for scramjet engines are updated continuously
    PSTREAM_HANDLE ph;
    const double scramDelta = -1.0;   // move particles back from the engines slightly
    // Note: ph will be null if exhaust streams are disabled
    ph = AddExhaustStream (th_scram[0], _V(-scramX, -2.121, -25.205 + scramDelta), &exhaust_scram);
    if (ph != nullptr)
        oapiParticleSetLevelRef (ph, scram_intensity+0);

    ph = AddExhaustStream (th_scram[1], _V( scramX, -2.121, -25.205 + scramDelta), &exhaust_scram);
    if (ph != nullptr)
        oapiParticleSetLevelRef (ph, scram_intensity+1);
    
#ifdef WONT_WORK
    // ********************* aerodynamics ***********************

    // NOTE: org values were causing nasty downward pitch in the atmospehere: 
    // hwing = CreateAirfoil3 (LIFT_VERTICAL, _V(0,0,-0.3), VLiftCoeff, NULL, 5, 90, 1.5);
    // Note: aspect ratio = total span squared divided by wing area: 76.67^2 / mesh_wing_area
    const double aspectRatio = (54.909154*54.909154) / 479.07;
    hwing = CreateAirfoil3 (LIFT_VERTICAL, _V(m_wingBalance,0,m_centerOfLift), VLiftCoeff, NULL, 27.68, WING_AREA, aspectRatio);
    
    // vertical stabiliser and body lift and drag components
    // location: effectively in center of vessel (because there are two), and just over 20 meters back from the center of the vessel
    // Note: aspect ratio = total span squared divided by wing area: 16.31.16.31^2 / mesh_wing_area
    const double verticalStabilWingArea = 96.68;
    const double vertAspectRatio = (16.30805*16.30805) / verticalStabilWingArea;
    CreateAirfoil3 (LIFT_HORIZONTAL, _V(0,0,-20.12), HLiftCoeff, NULL, 16.797632, verticalStabilWingArea, vertAspectRatio);
    
    
    // ref vector is 1 meter behind the vertical stabilizers
    hElevator = CreateControlSurface2(AIRCTRL_ELEVATOR,     40.32, 1.4, _V(   0,0,-21.2), AIRCTRL_AXIS_XPOS, anim_elevator);  // MATCH DG3 FOR REENTRY TESTING

    CreateControlSurface (AIRCTRL_RUDDER,      96.68, 1.5, _V(   0,0,-21.2), AIRCTRL_AXIS_YPOS, anim_rudder);

    hLeftAileron = CreateControlSurface2 (AIRCTRL_AILERON, 15.38, 1.5, _V( 31.96,0,-21.2), AIRCTRL_AXIS_XPOS, anim_raileron);
    hRightAileron = CreateControlSurface2 (AIRCTRL_AILERON, 15.38, 1.5, _V(-31.96,0,-21.2), AIRCTRL_AXIS_XNEG, anim_laileron);
    
    CreateControlSurface(AIRCTRL_ELEVATORTRIM, 10.08, 1.5, _V(   0,0,-21.2), AIRCTRL_AXIS_XPOS, anim_elevatortrim);
#endif

    // ********************* aerodynamics ***********************
    
    // NOTE: org values were causing nasty downward pitch in the atmospehere: 
    // hwing = CreateAirfoil3 (LIFT_VERTICAL, _V(0,0,-0.3), VLiftCoeff, NULL, 5, 90, 1.5);
    m_ctrlSurfacesDeltaZ = -21.2;       // distance from center of model to center of control surfaces, Z axis
    m_aileronDeltaX      = 31.962114;   // distance from center of ship to center of aileron, X direction
    XR1Multiplier        = 29.94;       // control surface area vs. the XR1  (5.99 = wing area delta w/area 479.07)

    // center of lift matches center of mass
    // NOTE: this airfoil's force attack point will be modified by the SetCenterOfLift PreStep 
    hwing = CreateAirfoil3 (LIFT_VERTICAL, _V(m_wingBalance, 0, m_centerOfLift), VLiftCoeff, NULL, 5 * XR1Multiplier, WING_AREA, WING_ASPECT_RATIO);  
    
    CreateAirfoil3 (LIFT_HORIZONTAL, _V(0, 0, m_ctrlSurfacesDeltaZ + 3.0), HLiftCoeff, NULL, 16.79, 15 * XR1Multiplier, 1.5);

    ReinitializeDamageableControlSurfaces();  // create ailerons, elevators, and elevator trim

    // vertical stabiliser and body lift and drag components
    CreateControlSurface(AIRCTRL_RUDDER,       0.8 * XR1Multiplier,     1.5, _V(   0,0, m_ctrlSurfacesDeltaZ), AIRCTRL_AXIS_YPOS, anim_rudder);

    // Create a hidden elevator trim to fix the nose-up tendency on liftoff and allow the elevator trim to be truly neutral.
    // We have to use FLAP here because that is the only unused control surface type.  We could probably also duplicate this via CreateAirfoil3, but this
    // is easier to adjust and test.
    CreateControlSurface(AIRCTRL_FLAP, 0.3 * XR1Multiplier * 7, 1.5, _V(   0,0, m_ctrlSurfacesDeltaZ), AIRCTRL_AXIS_XPOS);  // no animation for this!
    m_hiddenElevatorTrimState = HIDDEN_ELEVATOR_TRIM_STATE;        // set to a member variable in case we want to change it in flight later
    // Note: cannot set the level here; it is reset by Orbiter later.
    
    // TOO SMALL! const double xr1VariableDragModifier = 3.84;    // this is the avg ratio of the XR5:XR1 length, width, and height
    const double xr1VariableDragModifier = 22.2;    // this is the mass ratio of the XR5:XR1
	// we need these goofy const casts to force the linker to link with the 'const double *' version of CreateVariableDragElement instead of the legacy 'double *' version of the function (which still exists but is deprecated and causes warnings in orbiter.log)
	CreateVariableDragElement(const_cast<const double *>(&rcover_proc),   0.2 * xr1VariableDragModifier, _V(0,  0.581, 26.972));     // retro covers
    CreateVariableDragElement(const_cast<const double *>(&radiator_proc), 0.4 * xr1VariableDragModifier, _V(0, 3.274, -21.925));     // radiators
    CreateVariableDragElement(const_cast<const double *>(&bay_proc),      7.0 * xr1VariableDragModifier, _V(0, 8.01, -21.06));       // bay doors (drag is at rear of bay)
    CreateVariableDragElement(const_cast<const double *>(&gear_proc),     0.8 * xr1VariableDragModifier, _V(0, -9.539,  4.34));      // landing gear
    CreateVariableDragElement(const_cast<const double *>(&nose_proc),     2.1 * xr1VariableDragModifier, _V(0, 10.381,  6.515));     // docking port
    CreateVariableDragElement(const_cast<const double *>(&brake_proc),    4.0 * xr1VariableDragModifier, _V(0, 0, m_ctrlSurfacesDeltaZ));  // airbrake (do not induce a rotational moment here)
    CreateVariableDragElement(const_cast<const double *>(&crewElevator_proc), 6.0 * xr1VariableDragModifier, _V(-3.358, -6.51, 6.371));  // elevator (note: elevator is off-center)
    
    // XR5 ORG: const double dragMultiplier = 3.0;   
    const double dragMultiplier = 22.2;   // increased per c3p0: ship is easier to land now
    SetRotDrag (_V(0.10 * dragMultiplier, 0.13 * dragMultiplier, 0.04 * dragMultiplier));

    // define hull temperature limits (these match the XR1's limits for now)
    m_hullTemperatureLimits.noseCone  = CTOK(2840);
    m_hullTemperatureLimits.wings     = CTOK(2380);
    m_hullTemperatureLimits.cockpit   = CTOK(1490);
    m_hullTemperatureLimits.topHull   = CTOK(1210);
    m_hullTemperatureLimits.warningFrac  = 0.80;   // yellow text
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
        beacon[i].pos = beaconpos+i;
        beacon[i].col = beaconcol+i;
        beacon[i].size = (i < 3 ? 0.3 * sizeMultiplier : 0.55 * sizeMultiplier);
        beacon[i].falloff = (i < 3 ? 0.4 : 0.6);
        beacon[i].period = (i < 3 ? 0 : i < 5 ? 2 : 1.13);
        beacon[i].duration = (i < 5 ? 0.1 : 0.05);
        beacon[i].tofs = (6-i)*0.2;
        beacon[i].active = false;
        AddBeacon (beacon+i);
    }

    // light colors
    COLOUR4 col_d     = { 0.9f, 0.8f, 1.0f, 0.0f };  // diffuse
	COLOUR4 col_s     = { 1.9f, 0.8f, 1.0f ,0.0f };  // specular
	COLOUR4 col_a     = { 0.0f, 0.0f, 0.0f ,0.0f };  // ambient (black)
	COLOUR4 col_white = { 1.0f, 1.0f, 1.0f, 0.0f };  // white

    // add a light at each main engine set of 3
    const double mainEnginePointLightPower = 100 * 22.2;  // XR5 engines are 22.5 times as powerful as the XR1's
    const double zMainLightDelta = -5.0;  // need more delta here because the exhaust is sunk into the engine bell
    if (GetXR1Config()->EnableEngineLightingEffects)
    {
	    LightEmitter *leMainPort      = AddPointLight(_V(-4.1095, 2.871, mainExhaustZCoord + zMainLightDelta), mainEnginePointLightPower, 1e-3, 0, 2e-3, col_d, col_s, col_a);
        LightEmitter *leMainStarboard = AddPointLight(_V( 4.1095, 2.871, mainExhaustZCoord + zMainLightDelta), mainEnginePointLightPower, 1e-3, 0, 2e-3, col_d, col_s, col_a);
	    leMainPort->SetIntensityRef(&m_mainThrusterLightLevel);
        leMainStarboard->SetIntensityRef(&m_mainThrusterLightLevel);
    }

    // add a light at each set of hover engines
    if (GetXR1Config()->EnableEngineLightingEffects)
    {
        const double hoverEnginePointLightPower = mainEnginePointLightPower * 0.6875;  // hovers are .6875 the thrust of the mains
        const double yHoverLightDelta = -1.0;
        LightEmitter *leForward      = AddPointLight (_V(  0.000, -1.460 + yHoverLightDelta,  12.799), hoverEnginePointLightPower, 1e-3, 0, 2e-3, col_d, col_s, col_a); 
        LightEmitter *leAftPort      = AddPointLight (_V(-22.324, -1.091 + yHoverLightDelta, -15.633), hoverEnginePointLightPower, 1e-3, 0, 2e-3, col_d, col_s, col_a); 
        LightEmitter *leAftStarboard = AddPointLight (_V( 22.324, -1.091 + yHoverLightDelta, -15.633), hoverEnginePointLightPower, 1e-3, 0, 2e-3, col_d, col_s, col_a); 
	    leForward->SetIntensityRef(&m_hoverThrusterLightLevel);
        leAftPort->SetIntensityRef(&m_hoverThrusterLightLevel);
        leAftStarboard->SetIntensityRef(&m_hoverThrusterLightLevel);
    }

    // add docking lights (2 forward and 2 docking)
    // Note: XR1/XR2 rage was 150 meters
    // forward
    m_pSpotlights[0] = static_cast<SpotLight *>(AddSpotLight(_V( 10.628, -0.055, 3.586), _V(0,0,1), 250, 1e-3, 0, 1e-3, RAD*25, RAD*60, col_white, col_white, col_a));
    m_pSpotlights[1] = static_cast<SpotLight *>(AddSpotLight(_V(-10.628, -0.055, 3.586), _V(0,0,1), 250, 1e-3, 0, 1e-3, RAD*25, RAD*60, col_white, col_white, col_a));
    // docking port 
    m_pSpotlights[2] = static_cast<SpotLight *>(AddSpotLight(_V(-1.66, 7.475, 6.375), _V(0,1,0), 250, 1e-3, 0, 1e-3, RAD*25, RAD*60, col_white, col_white, col_a));
    m_pSpotlights[3] = static_cast<SpotLight *>(AddSpotLight(_V( 1.66, 7.475, 6.375), _V(0,1,0), 250, 1e-3, 0, 1e-3, RAD*25, RAD*60, col_white, col_white, col_a));

    // turn all spotlights off by default
    for (int i=0; i < SPOTLIGHT_COUNT; i++)
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
    AddInstrumentPanel(new XR5MainInstrumentPanel1920 (*this), 1920);
    AddInstrumentPanel(new XR5UpperInstrumentPanel1920(*this), 1920);
    AddInstrumentPanel(new XR5LowerInstrumentPanel1920(*this), 1920);
    AddInstrumentPanel(new XR5OverheadInstrumentPanel1920(*this), 1920);
    AddInstrumentPanel(new XR5PayloadInstrumentPanel1920(*this), 1920);

    // 1600-pixel-wide panels
    AddInstrumentPanel(new XR5MainInstrumentPanel1600 (*this), 1600);
    AddInstrumentPanel(new XR5UpperInstrumentPanel1600(*this), 1600);
    AddInstrumentPanel(new XR5LowerInstrumentPanel1600(*this), 1600);
    AddInstrumentPanel(new XR5OverheadInstrumentPanel1600(*this), 1600);
    AddInstrumentPanel(new XR5PayloadInstrumentPanel1600(*this), 1600);

    // 1280-pixel-wide panels
    AddInstrumentPanel(new XR5MainInstrumentPanel1280 (*this), 1280);
    AddInstrumentPanel(new XR5UpperInstrumentPanel1280(*this), 1280);
    AddInstrumentPanel(new XR5LowerInstrumentPanel1280(*this), 1280);
    AddInstrumentPanel(new XR5OverheadInstrumentPanel1280(*this), 1280);
    AddInstrumentPanel(new XR5PayloadInstrumentPanel1280(*this), 1280);

    // no VC (yet!) for the XR5
#ifdef UNDEF
    // add our VC panels
    AddInstrumentPanel(new VCPilotInstrumentPanel     (*this, PANELVC_PILOT), 0);
    AddInstrumentPanel(new VCPassenger1InstrumentPanel(*this, PANELVC_PSNGR1), 0);
    AddInstrumentPanel(new VCPassenger2InstrumentPanel(*this, PANELVC_PSNGR2), 0);
    AddInstrumentPanel(new VCPassenger3InstrumentPanel(*this, PANELVC_PSNGR3), 0);
    AddInstrumentPanel(new VCPassenger4InstrumentPanel(*this, PANELVC_PSNGR4), 0);
#endif

}

// Create control surfaces for any damageable control surface handles below that are zero (all are zero before vessel initialized).
// This is invoked from clbkSetClassCaps as well as ResetDamageStatus.
void XR5Vanguard::ReinitializeDamageableControlSurfaces()
{
    if (hElevator == 0)
    {
        // ORG:     CreateControlSurface (AIRCTRL_ELEVATOR,     1.4, 1.5, _V(   0,0,-7.2), AIRCTRL_AXIS_XPOS, anim_elevator);
        // DG3:     CreateControlSurface (AIRCTRL_ELEVATOR,     1.2, 1.4, _V(   0,0,-7.2), AIRCTRL_AXIS_XPOS, anim_elevator);  // matches DG3
        // BETA-1:  CreateControlSurface (AIRCTRL_ELEVATOR,     1.4, 1.4, _V(   0,0,-7.2), AIRCTRL_AXIS_XPOS, anim_elevator);  // COG point matches DG3, but keep stock area
        hElevator = CreateControlSurface2(AIRCTRL_ELEVATOR,     1.2 * XR1Multiplier * 3, 1.4, _V(   0,0, m_ctrlSurfacesDeltaZ), AIRCTRL_AXIS_XPOS, anim_elevator);
    }

    if (hLeftAileron == 0)
    {
        // ORG: hLeftAileron = CreateControlSurface2 (AIRCTRL_AILERON, 0.3, 1.5, _V( 7.5,0,-7.2), AIRCTRL_AXIS_XPOS, anim_raileron);
        hLeftAileron = CreateControlSurface2 (AIRCTRL_AILERON, 0.2 * XR1Multiplier * 2, 1.5, _V( m_aileronDeltaX, 0, m_ctrlSurfacesDeltaZ), AIRCTRL_AXIS_XPOS, anim_raileron);
    }

    if (hRightAileron == 0)
    {
        // ORG: hRightAileron = CreateControlSurface2 (AIRCTRL_AILERON, 0.3, 1.5, _V(-7.5,0,-7.2), AIRCTRL_AXIS_XNEG, anim_laileron);
        hRightAileron = CreateControlSurface2 (AIRCTRL_AILERON, 0.2 * XR1Multiplier * 2, 1.5, _V(-m_aileronDeltaX, 0, m_ctrlSurfacesDeltaZ), AIRCTRL_AXIS_XNEG, anim_laileron);
    }
    
    if (hElevatorTrim == 0)
    {
        // XR5 ORG: CreateControlSurface(AIRCTRL_ELEVATORTRIM, 0.3 * XR1Multiplier * 7, 1.5, _V(   0,0, controlSurfacesDeltaZ), AIRCTRL_AXIS_XPOS, anim_elevatortrim);
        // NOTE: increased this area to assist the autopilot in maintaining flight control in an atmosphere.
        hElevatorTrim = CreateControlSurface2 (AIRCTRL_ELEVATORTRIM, 0.3 * XR1Multiplier * 7, 1.5, _V(   0,0, m_ctrlSurfacesDeltaZ), AIRCTRL_AXIS_XPOS, anim_elevatortrim);
    }
}

// --------------------------------------------------------------
// Respond to playback event
// NOTE: do not use spaces in any of these event ID strings.
// --------------------------------------------------------------
bool XR5Vanguard::clbkPlaybackEvent (double simt, double event_t, const char *event_type, const char *event)
{
    // check for XR5-specific events
    if (!_stricmp (event_type, "ELEVATOR"))
    {
        ActivateElevator (!_stricmp (event, "CLOSE") ? DOOR_CLOSING : DOOR_OPENING);
        return true;
    } 

    // else let our superclass have it
    return DeltaGliderXR1::clbkPlaybackEvent(simt, event_t, event_type, event);
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

    EnableRetroThrusters(rcover_status == DOOR_OPEN);
    EnableHoverEngines(hoverdoor_status == DOOR_OPEN);
    EnableScramEngines(scramdoor_status == DOOR_OPEN);

    // set initial animation states
    SetXRAnimation(anim_gear,         gear_proc);
    SetXRAnimation(anim_rcover,       rcover_proc);
    SetXRAnimation(anim_hoverdoor,    hoverdoor_proc);
    SetXRAnimation(anim_scramdoor,    scramdoor_proc);
    SetXRAnimation(anim_nose,         nose_proc);
    SetXRAnimation(anim_ladder,       ladder_proc);
    SetXRAnimation(anim_olock,        olock_proc);
    SetXRAnimation(anim_ilock,        ilock_proc);
    SetXRAnimation(anim_hatch,        hatch_proc);
    SetXRAnimation(anim_radiator,     radiator_proc);
    SetXRAnimation(anim_brake,        brake_proc);
    SetXRAnimation(anim_bay,          bay_proc);
    SetXRAnimation(anim_crewElevator, crewElevator_proc);
    
    // NOTE: instrument panel initialization moved to clbkSetClassCaps (earlier) because the Post-2010-P1 Orbiter Beta invokes clbkLoadPanel before invoking clbkPostCreation

    // add our PreStep objects; these are invoked in order
    AddPreStep(new DrainBayFuelTanksPreStep(*this));  // need to do this *first* so the gauges are all correct later in the timestep (keeps main/SCRAM tanks full)
    AddPreStep(new RefreshSlotStatesPreStep(*this));  // do this early in case any other presteps look at the slot state
    AddPreStep(new AttitudeHoldPreStep     (*this));         
    AddPreStep(new DescentHoldPreStep      (*this));
    AddPreStep(new AirspeedHoldPreStep     (*this));
    AddPreStep(new ScramjetSoundPreStep    (*this));
    AddPreStep(new MmuPreStep              (*this));
    AddPreStep(new GearCalloutsPreStep     (*this));
    AddPreStep(new MachCalloutsPreStep     (*this));
    AddPreStep(new AltitudeCalloutsPreStep (*this));
    AddPreStep(new DockingCalloutsPreStep  (*this));
    AddPreStep(new TakeoffAndLandingCalloutsAndCrashPreStep(*this));
    AddPreStep(new AnimateGearCompressionPreStep           (*this));
    AddPreStep(new RotateWheelsPreStep                     (*this));  // NOTE: this must be *after* AnimateGearCompressionPreStep so that we can detect whether the wheels are touching the ground or not for this timestep
    AddPreStep(new XR5NosewheelSteeringPreStep             (*this));  // NOTE: this must be *after* AnimateGearCompressionPreStep so that we can detect whether the nosewheel is touching the ground or not for this timestep
    AddPreStep(new RefreshGrappleTargetsInDisplayRangePreStep     (*this));
    AddPreStep(new UpdateVesselLightsPreStep(*this));
	AddPreStep(new ParkingBrakePreStep	    (*this));

    // WARNING: this must be invoked LAST in the sequence so that behavior is consistent across all pre-step methods
    AddPreStep(new UpdatePreviousFieldsPreStep   (*this));

    // add our PostStep objects; these are invoked in order
    AddPostStep(new PreventAutoRefuelPostStep   (*this));   // add this FIRST before our fuel callouts
    AddPostStep(new ComputeAccPostStep          (*this));   // used by acc areas; computed only once per frame for efficiency
    // XRSound: AddPostStep(new AmbientSoundsPostStep       (*this));
    AddPostStep(new ShowWarningPostStep         (*this));
    AddPostStep(new SetHullTempsPostStep        (*this));
    AddPostStep(new SetSlopePostStep            (*this));
    // do not include DoorSoundsPostStep here; we replace it below
    AddPostStep(new FuelCalloutsPostStep        (*this));
    AddPostStep(new UpdateIntervalTimersPostStep(*this));
    AddPostStep(new APUPostStep                 (*this));
    AddPostStep(new UpdateMassPostStep          (*this));
    AddPostStep(new DisableControlSurfForAPUPostStep    (*this));
	AddPostStep(new OneShotInitializationPostStep(*this));
    AddPostStep(new AnimationPostStep           (*this));
    AddPostStep(new FuelDumpPostStep            (*this));
    AddPostStep(new XFeedPostStep               (*this));
    AddPostStep(new ResupplyPostStep            (*this));
    AddPostStep(new LOXConsumptionPostStep      (*this));
    AddPostStep(new UpdateCoolantTempPostStep   (*this));
    AddPostStep(new AirlockDecompressionPostStep(*this));
    AddPostStep(new AutoCenteringSimpleButtonAreasPostStep(*this));  // logic for all auto-centering button areas
    AddPostStep(new ResetAPUTimerForPolledSystemsPostStep (*this));
    AddPostStep(new ManageMWSPostStep                     (*this));

    // NEW poststeps specific to the XR5
    AddPostStep(new SwitchTwoDPanelPostStep                  (*this));
    AddPostStep(new XR5AnimationPostStep                     (*this));
    AddPostStep(new XR5DoorSoundsPostStep                    (*this));  // replaces the standard DoorSoundsPostStep in the XR1 class
    AddPostStep(new HandleDockChangesForActiveAirlockPostStep(*this));  // switch active airlock automatically as necessary

#ifdef _DEBUG
    AddPostStep(new TestXRVesselCtrlPostStep    (*this));      // for manual testing of new XRVesselCtrl methods via the debugger
#endif

    // set hidden elevator trim level
    SetControlSurfaceLevel(AIRCTRL_FLAP, m_hiddenElevatorTrimState);  
}

// --------------------------------------------------------------
// Create visual
// --------------------------------------------------------------
void XR5Vanguard::clbkVisualCreated (VISHANDLE vis, int refcount)
{
    exmesh = GetDevMesh (vis, 0);
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
void XR5Vanguard::clbkVisualDestroyed (VISHANDLE vis, int refcount)
{
    exmesh = nullptr;
    vcmesh = nullptr;
}


// Superclass' clbkPreStep and clbkPostStep are all we need

// ==============================================================
// Message callback function for control dialog box
// ==============================================================

BOOL CALLBACK XR5Ctrl_DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    XR5Vanguard *dg = (uMsg == WM_INITDIALOG ? reinterpret_cast<XR5Vanguard *>(lParam) : reinterpret_cast<XR5Vanguard *>(oapiGetDialogContext(hWnd)));
    // pointer to vessel instance was passed as dialog context
    
    switch (uMsg) {
    /* Note: for some reason Orbiter appears to be trapping keystrokes, so this will not work.
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
            oapiCloseDialog(hWnd);   // bye, bye
        break;  // pass it on
    */

    case WM_INITDIALOG:
        dg->UpdateCtrlDialog(dg, hWnd);
        return FALSE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            oapiCloseDialog (hWnd);
            return TRUE;

        case IDC_GEAR_UP:
            dg->ActivateLandingGear (DOOR_CLOSING);
            return 0;
        case IDC_GEAR_DOWN:
            dg->ActivateLandingGear (DOOR_OPENING);
            return 0;

        case IDC_RETRO_CLOSE:
            dg->ActivateRCover (DOOR_CLOSING);
            return 0;
        case IDC_RETRO_OPEN:
            dg->ActivateRCover (DOOR_OPENING);
            return 0;

        case IDC_BAY_CLOSE:
            dg->ActivateBayDoors(DOOR_CLOSING);
            return 0;
        case IDC_BAY_OPEN:
            dg->ActivateBayDoors (DOOR_OPENING);
            return 0;

        case IDC_OLOCK_CLOSE:
            dg->ActivateOuterAirlock (DOOR_CLOSING);
            return 0;
        case IDC_OLOCK_OPEN:
            dg->ActivateOuterAirlock (DOOR_OPENING);
            return 0;

        case IDC_ILOCK_CLOSE:
            dg->ActivateInnerAirlock (DOOR_CLOSING);
            return 0;
        case IDC_ILOCK_OPEN:
            dg->ActivateInnerAirlock (DOOR_OPENING);
            return 0;

        case IDC_HOVER_CLOSE:
            dg->ActivateHoverDoors(DOOR_CLOSING);
            return 0;
        case IDC_HOVER_OPEN:
            dg->ActivateHoverDoors (DOOR_OPENING);
            return 0;

        case IDC_DOCKING_STOW:
            dg->ActivateNoseCone (DOOR_CLOSING);
            return 0;
        case IDC_DOCKING_DEPLOY:
            dg->ActivateNoseCone (DOOR_OPENING);
            return 0;

        case IDC_ELEVATOR_STOW:
            dg->ActivateElevator(DOOR_CLOSING);
            return 0;
        case IDC_ELEVATOR_DEPLOY:
            dg->ActivateElevator (DOOR_OPENING);
            return 0;

        case IDC_SCRAM_CLOSE:
            dg->ActivateScramDoors(DOOR_CLOSING);
            return 0;
        case IDC_SCRAM_OPEN:
            dg->ActivateScramDoors (DOOR_OPENING);
            return 0;

        case IDC_HATCH_CLOSE:
            dg->ActivateHatch (DOOR_CLOSING);
            return 0;
        case IDC_HATCH_OPEN:
            dg->ActivateHatch (DOOR_OPENING);
            return 0;

        case IDC_RADIATOR_STOW:
            dg->ActivateRadiator (DOOR_CLOSING);
            return 0;
        case IDC_RADIATOR_DEPLOY:
            dg->ActivateRadiator (DOOR_OPENING);
            return 0;

        case IDC_NAVLIGHT:
            dg->SetNavlight (SendDlgItemMessage (hWnd, IDC_NAVLIGHT, BM_GETCHECK, 0, 0) == BST_CHECKED);
            return 0;
        case IDC_BEACONLIGHT:
            dg->SetBeacon (SendDlgItemMessage (hWnd, IDC_BEACONLIGHT, BM_GETCHECK, 0, 0) == BST_CHECKED);
            return 0;
        case IDC_STROBELIGHT:
            dg->SetStrobe (SendDlgItemMessage (hWnd, IDC_STROBELIGHT, BM_GETCHECK, 0, 0) == BST_CHECKED);
            return 0;
        }
        break;
    }
    return oapiDefDialogProc (hWnd, uMsg, wParam, lParam);
}

void XR5Vanguard::UpdateCtrlDialog(XR5Vanguard *dg, HWND hWnd)
{
    static int bstatus[2] = {BST_UNCHECKED, BST_CHECKED};
    
    if (hWnd == nullptr) 
        hWnd = oapiFindDialog (g_hDLL, IDD_CTRL);

    if (hWnd == nullptr) 
        return;

    int op;
    
    op = dg->gear_status & 1;
    SendDlgItemMessage (hWnd, IDC_GEAR_DOWN, BM_SETCHECK, bstatus[op], 0);
    SendDlgItemMessage (hWnd, IDC_GEAR_UP, BM_SETCHECK, bstatus[1-op], 0);
    
    op = dg->rcover_status & 1;
    SendDlgItemMessage (hWnd, IDC_RETRO_OPEN, BM_SETCHECK, bstatus[op], 0);
    SendDlgItemMessage (hWnd, IDC_RETRO_CLOSE, BM_SETCHECK, bstatus[1-op], 0);
    
    op = dg->bay_status & 1;
    SendDlgItemMessage (hWnd, IDC_BAY_OPEN, BM_SETCHECK, bstatus[op], 0);
    SendDlgItemMessage (hWnd, IDC_BAY_CLOSE, BM_SETCHECK, bstatus[1-op], 0);

    op = dg->olock_status & 1;
    SendDlgItemMessage (hWnd, IDC_OLOCK_OPEN, BM_SETCHECK, bstatus[op], 0);
    SendDlgItemMessage (hWnd, IDC_OLOCK_CLOSE, BM_SETCHECK, bstatus[1-op], 0);
    
    op = dg->ilock_status & 1;
    SendDlgItemMessage (hWnd, IDC_ILOCK_OPEN, BM_SETCHECK, bstatus[op], 0);
    SendDlgItemMessage (hWnd, IDC_ILOCK_CLOSE, BM_SETCHECK, bstatus[1-op], 0);

    op = dg->hoverdoor_status & 1;
    SendDlgItemMessage (hWnd, IDC_HOVER_OPEN, BM_SETCHECK, bstatus[op], 0);
    SendDlgItemMessage (hWnd, IDC_HOVER_CLOSE, BM_SETCHECK, bstatus[1-op], 0);

    op = dg->nose_status & 1;
    SendDlgItemMessage (hWnd, IDC_DOCKING_DEPLOY, BM_SETCHECK, bstatus[op], 0);
    SendDlgItemMessage (hWnd, IDC_DOCKING_STOW, BM_SETCHECK, bstatus[1-op], 0);
    
    op = dg->crewElevator_status & 1;
    SendDlgItemMessage (hWnd, IDC_ELEVATOR_DEPLOY, BM_SETCHECK, bstatus[op], 0);
    SendDlgItemMessage (hWnd, IDC_ELEVATOR_STOW, BM_SETCHECK, bstatus[1-op], 0);
    
    op = dg->scramdoor_status & 1;
    SendDlgItemMessage (hWnd, IDC_SCRAM_OPEN, BM_SETCHECK, bstatus[op], 0);
    SendDlgItemMessage (hWnd, IDC_SCRAM_CLOSE, BM_SETCHECK, bstatus[1-op], 0);

    op = dg->hatch_status & 1;
    SendDlgItemMessage (hWnd, IDC_HATCH_OPEN, BM_SETCHECK, bstatus[op], 0);
    SendDlgItemMessage (hWnd, IDC_HATCH_CLOSE, BM_SETCHECK, bstatus[1-op], 0);
    
    op = dg->radiator_status & 1;
    SendDlgItemMessage (hWnd, IDC_RADIATOR_DEPLOY, BM_SETCHECK, bstatus[op], 0);
    SendDlgItemMessage (hWnd, IDC_RADIATOR_STOW, BM_SETCHECK, bstatus[1-op], 0);
    
    op = dg->beacon[0].active ? 1:0;
    SendDlgItemMessage (hWnd, IDC_NAVLIGHT, BM_SETCHECK, bstatus[op], 0);
    op = dg->beacon[3].active ? 1:0;
    SendDlgItemMessage (hWnd, IDC_BEACONLIGHT, BM_SETCHECK, bstatus[op], 0);
    op = dg->beacon[5].active ? 1:0;
    SendDlgItemMessage (hWnd, IDC_STROBELIGHT, BM_SETCHECK, bstatus[op], 0);
}

// toggle RCS docking mode
// rcsMode: true = set docking mode, false = set normal mode
// Returns: true if mode switched successfully, false if mode switch was inhibited
bool XR5Vanguard::SetRCSDockingMode(bool dockingMode)
{
    // if enabling docking mode and any autopilot is engaged, prohibit the change
    bool autopilotEngaged = false;
    if (dockingMode)
    {
        // check whether any standard autopilot is engaged
        for (int i=1; i <= 7; i++)
        {
            if (GetNavmodeState(i))
            {
                autopilotEngaged = true;   
                break;
            }
        }

        // check for any custom autopilot except for Airspeed Hold
        autopilotEngaged |= (m_customAutopilotMode != AP_OFF);

        if (autopilotEngaged)
        {
            PlayErrorBeep();
            ShowWarning("RCS locked by Autopilot.wav", DeltaGliderXR1::ST_WarningCallout, "Autopilot is active: RCS mode is locked.");
            return false;
        }
    }

    ConfigureRCSJets(dockingMode);     
    PlaySound((dockingMode ? BeepHigh : BeepLow), DeltaGliderXR1::ST_Other);

    // play voice callout
    if (dockingMode)
        ShowInfo("RCS Config Docking.wav", DeltaGliderXR1::ST_InformationCallout, "RCS jets set to DOCKING configuration.");
    else
        ShowInfo("RCS Config Normal.wav", DeltaGliderXR1::ST_InformationCallout, "RCS jets set to NORMAL configuration.");
    
    return true;
}

// Configure RCS jets for docking or normal mode by configuring RCS thruster groups.  
// This method does NOT display any message or play any sounds; however, it does redraw then RCS mode light/switch.
// rcsMode: true = set docking mode, false = set normal mode
void XR5Vanguard::ConfigureRCSJets(bool dockingMode)
{
    // delete any existing RCS thruster groups
    DelThrusterGroup(THGROUP_ATT_PITCHUP);
    DelThrusterGroup(THGROUP_ATT_PITCHDOWN);
    DelThrusterGroup(THGROUP_ATT_UP);
    DelThrusterGroup(THGROUP_ATT_DOWN);

    DelThrusterGroup(THGROUP_ATT_YAWLEFT);
    DelThrusterGroup(THGROUP_ATT_YAWRIGHT);
    DelThrusterGroup(THGROUP_ATT_LEFT);
    DelThrusterGroup(THGROUP_ATT_RIGHT);

    DelThrusterGroup(THGROUP_ATT_BANKLEFT);
    DelThrusterGroup(THGROUP_ATT_BANKRIGHT);

    DelThrusterGroup(THGROUP_ATT_FORWARD);
    DelThrusterGroup(THGROUP_ATT_BACK);

    THRUSTER_HANDLE th_att_rot[4], th_att_lin[4];

    if (dockingMode == false)   
    {
        // NORMAL mode
        th_att_rot[0] = th_att_lin[0] = th_rcs[0];  // fore up
        th_att_rot[1] = th_att_lin[3] = th_rcs[1];  // aft down
        th_att_rot[2] = th_att_lin[2] = th_rcs[2];  // fore down
        th_att_rot[3] = th_att_lin[1] = th_rcs[3];  // aft up
        CreateThrusterGroup (th_att_rot,   2, THGROUP_ATT_PITCHUP);   // rotate UP on X axis (+x)
        CreateThrusterGroup (th_att_rot+2, 2, THGROUP_ATT_PITCHDOWN); // rotate DOWN on X axis (-x)
        CreateThrusterGroup (th_att_lin,   2, THGROUP_ATT_UP);        // translate UP along Y axis (+y)
        CreateThrusterGroup (th_att_lin+2, 2, THGROUP_ATT_DOWN);      // translate DOWN along Y axis (-y)

        th_att_rot[0] = th_att_lin[0] = th_rcs[4];  // fore left
        th_att_rot[1] = th_att_lin[3] = th_rcs[5];  // aft right
        th_att_rot[2] = th_att_lin[2] = th_rcs[6];  // fore right
        th_att_rot[3] = th_att_lin[1] = th_rcs[7];  // aft left
        CreateThrusterGroup (th_att_rot,   2, THGROUP_ATT_YAWLEFT);   // rotate LEFT on Y axis (-y)
        CreateThrusterGroup (th_att_rot+2, 2, THGROUP_ATT_YAWRIGHT);  // rotate RIGHT on Y axis (+y)
        CreateThrusterGroup (th_att_lin,   2, THGROUP_ATT_LEFT);      // translate LEFT along X axis (-x)
        CreateThrusterGroup (th_att_lin+2, 2, THGROUP_ATT_RIGHT);     // translate RIGHT along X axis (+x)

        th_att_rot[0] = th_rcs[8];     // right wing bottom
        th_att_rot[1] = th_rcs[9];     // left wing top
        th_att_rot[2] = th_rcs[10];    // left wing bottom
        th_att_rot[3] = th_rcs[11];    // right wing top
        CreateThrusterGroup (th_att_rot,   2, THGROUP_ATT_BANKLEFT);  // rotate LEFT on Z axis (-Z)
        CreateThrusterGroup (th_att_rot+2, 2, THGROUP_ATT_BANKRIGHT); // rotate RIGHT on Z axis (+Z)

        th_att_lin[0] = th_rcs[12];   // aft
        th_att_lin[1] = th_rcs[13];   // fore
        CreateThrusterGroup (th_att_lin,   1, THGROUP_ATT_FORWARD);   // translate FORWARD along Z axis (+z)
        CreateThrusterGroup (th_att_lin+1, 1, THGROUP_ATT_BACK);      // translate BACKWARD along Z axis (-z)
    }
    else  // DOCKING mode
    {
        // For DOCKING mode, the Z and Y axes are exchanged:
        // X axis remains UNCHANGED
        // +Y = +Z
        // -Y = -Z
        // +Z = +Y
        // -Z = -Y
        th_att_rot[0] = th_att_lin[0] = th_rcs[0];  // fore up
        th_att_rot[1] = th_att_lin[3] = th_rcs[1];  // aft down
        th_att_rot[2] = th_att_lin[2] = th_rcs[2];  // fore down
        th_att_rot[3] = th_att_lin[1] = th_rcs[3];  // aft up
        CreateThrusterGroup (th_att_rot,   2, THGROUP_ATT_PITCHUP);   // rotate UP on X axis (+x)
        CreateThrusterGroup (th_att_rot+2, 2, THGROUP_ATT_PITCHDOWN); // rotate DOWN on X axis (-x)
        CreateThrusterGroup (th_att_lin,   2, THGROUP_ATT_FORWARD);   // old: translate UP along Y axis (+y) = new: +Z
        CreateThrusterGroup (th_att_lin+2, 2, THGROUP_ATT_BACK);      // old: translate DOWN along Y axis (-y) = new: -Z

        th_att_rot[0] = th_att_lin[0] = th_rcs[4];  // fore left
        th_att_rot[1] = th_att_lin[3] = th_rcs[5];  // aft right
        th_att_rot[2] = th_att_lin[2] = th_rcs[6];  // fore right
        th_att_rot[3] = th_att_lin[1] = th_rcs[7];  // aft left
        CreateThrusterGroup (th_att_rot,   2, THGROUP_ATT_BANKRIGHT);  // old: rotate LEFT on Y axis (-y) = new: -Z
        CreateThrusterGroup (th_att_rot+2, 2, THGROUP_ATT_BANKLEFT);   // old: rotate RIGHT on Y axis (+y) = new: +Z
        CreateThrusterGroup (th_att_lin,   2, THGROUP_ATT_LEFT);       // translate LEFT along X axis (-x)
        CreateThrusterGroup (th_att_lin+2, 2, THGROUP_ATT_RIGHT);      // translate RIGHT along X axis (+x)

        th_att_rot[0] = th_rcs[8];     // right wing bottom
        th_att_rot[1] = th_rcs[9];     // left wing top
        th_att_rot[2] = th_rcs[10];    // left wing bottom
        th_att_rot[3] = th_rcs[11];    // right wing top
        CreateThrusterGroup (th_att_rot,   2, THGROUP_ATT_YAWLEFT);  // old: rotate LEFT on Z axis (+Z) = new: -Y
        CreateThrusterGroup (th_att_rot+2, 2, THGROUP_ATT_YAWRIGHT); // old: rotate RIGHT on Z axis (-Z) = new: +Z

        th_att_lin[0] = th_rcs[12];   // aft
        th_att_lin[1] = th_rcs[13];   // fore
        CreateThrusterGroup (th_att_lin,   1, THGROUP_ATT_DOWN);   // old: translate FORWARD along Z axis (+z) = new: -Y  
        CreateThrusterGroup (th_att_lin+1, 1, THGROUP_ATT_UP);     // old: translate BACKWARD along Z axis (-z) = new: +Y
    }

    // reset all thruster levels
    // NOTE: must take damage into account here!
    double rcsThrusterPowerFrac = (dockingMode? 0.40 : 1.0);  // reduce thruster power in docking mode
    for (int i=0; i < 14; i++)
    {
        // get integrity fraction
        int damageIntegrityIndex = static_cast<int>(RCS1) + i;    // 0 <= i <= 13
        DamageStatus ds = GetDamageStatus((DamageItem)damageIntegrityIndex);
        SetThrusterMax0(th_rcs[i], (GetRCSThrustMax(i) * rcsThrusterPowerFrac * ds.fracIntegrity));  
    }

    m_rcsDockingMode = dockingMode;     
    TriggerRedrawArea(AID_RCS_CONFIG_BUTTON);
}

// hook this so we can disable docking mode automatically
void XR5Vanguard::SetCustomAutopilotMode(AUTOPILOT mode, bool playSound, bool force)
{
    if (mode != AP_OFF)
        ConfigureRCSJets(false);    // revert to normal mode

    DeltaGliderXR1::SetCustomAutopilotMode(mode, playSound, force); // do the work
}

// set the active EVA port
void XR5Vanguard::SetActiveEVAPort(ACTIVE_EVA_PORT newState)
{
    m_activeEVAPort = newState;

    // update the UMMu port coordinates and repaint the LEDs and the switch
    DefineMmuAirlock();
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
    DeltaGliderXR1::clbkNavMode (mode, active);
}

// state: 0=fully retracted, 1.0 = fully deployed
void XR5Vanguard::SetGearParameters(double state)
{
    if (state == 1.0) // fully deployed?
    {
        const double touchdownDeltaX = 16.283;
        const double touchdownY = GEAR_UNCOMPRESSED_YCOORD + GEAR_COMPRESSION_DISTANCE; // gear height fully compressed

        SetXRTouchdownPoints(_V(               0, touchdownY, NOSE_GEAR_ZCOORD),    // front
                             _V(-touchdownDeltaX, touchdownY, REAR_GEAR_ZCOORD),    // left
                             _V( touchdownDeltaX, touchdownY, REAR_GEAR_ZCOORD),    // right
							 WHEEL_FRICTION_COEFF, WHEEL_LATERAL_COEFF, true);
        SetNosewheelSteering(true);  // not really necessary since we have have a prestep constantly checking this
    } 
    else  // not fully deployed (belly landing!)
    {
        const double touchdownDeltaX = 4.509;
        const double touchdownZRear = -17.754;

        SetXRTouchdownPoints(_V(               0, -1.248, 21.416),                // front
                             _V(-touchdownDeltaX, -3.666, touchdownZRear),        // left
                             _V( touchdownDeltaX, -3.150, touchdownZRear),        // right (tilt the ship)
							 3.0, 3.0, false);					// belly landing!
        SetNosewheelSteering(false);  // not really necessary since we have have a prestep constantly checking this
    }

    // update the animation state
    gear_proc = state;
    SetXRAnimation(anim_gear, gear_proc);

     // redraw the gear indicator
    TriggerRedrawArea(AID_GEARINDICATOR);

    // PERFORMANCE ENHANCEMENT: hide the gear if it is fully retracted; otherwise, render it
    static const UINT gearMeshGroups[] = 
    { 
        GRP_nose_oleo_piston, GRP_nose_axle_piston, GRP_nose_axle_cylinder, GRP_nose_axle, GRP_nose_oleo_piston, GRP_nose_gear_wheel_right, GRP_nose_gear_wheel_left,
        GRP_axle_left, GRP_axle_right, GRP_gear_main_oleo_cylinder_right, GRP_axle_piston_left, GRP_axle_cylinder_left, GRP_axle_cylinder_right, GRP_axle_piston_right, GRP_oleo_piston_right,
        GRP_oleo_piston_left, GRP_wheel_left_front_left_side, GRP_wheel_right_front_left_side, GRP_wheel_left_rear_left_side, GRP_wheel_right_rear_left_side, GRP_wheel_left_rear_right_side,
        GRP_wheel_right_rear_right_side, GRP_wheel_left_front_right_side, GRP_wheel_right_front_right_side, GRP_gear_main_oleo_cylinder_left, GRP_nose_oleo_cylinder 
    };

    SetMeshGroupsVisibility((state != 0.0), exmesh, SizeOfGrp(gearMeshGroups), gearMeshGroups);
}

// handle instant jumps to open or closed here
#define CHECK_DOOR_JUMP(proc, anim) if (action == DOOR_OPEN) proc = 1.0;            \
                                    else if (action == DOOR_CLOSED) proc = 0.0;     \
                                    SetXRAnimation(anim, proc)

// activate the bay doors (must override base class because of our radiator check)
void XR5Vanguard::ActivateBayDoors(DoorStatus action)
{
    // cannot deploy or retract bay doors if the radiator is in motion
    // NOTE: allow for DOOR_FAILED here so that a radiator failure does not lock the bay doors
    if ((radiator_status == DOOR_OPENING) || (radiator_status == DOOR_CLOSING))
    {
        PlayErrorBeep();
        ShowWarning("Warning Radiator in Motion Bay Doors Are Locked.wav", DeltaGliderXR1::ST_WarningCallout, "Cannot open/close bay doors while&radiator is in motion.");
        return;  // cannot move
    }

    // OK to move doors as far as the radiator is concerned; invoke the base class
    DeltaGliderXR1::ActivateBayDoors(action);
}

// activate the crew elevator
void XR5Vanguard::ActivateElevator(DoorStatus action)
{
    // check for failure
    if (crewElevator_status == DOOR_FAILED)
    {
        PlayErrorBeep();
        ShowWarning("Warning Elevator Failure.wav", DeltaGliderXR1::ST_WarningCallout, "Elevator inoperative due to excessive&heat and/or dynamic pressure.");
        return;  // cannot move
    }

    if (CheckHydraulicPressure(true, true) == false)   // show warning if no hydraulic pressure
        return;     // no hydraulic pressure

    // verify the gear has not collapsed!
    if (GetAltitude(ALTMODE_GROUND) < (GEAR_FULLY_COMPRESSED_DISTANCE-0.2))  // leave a 0.2-meter safety cushion
    {
        PlayErrorBeep();
        ShowWarning("Warning Elevator Failure.wav", DeltaGliderXR1::ST_WarningCallout, "Elevator inoperative: ground impact.");
        return;  // cannot move
    }

    bool close = (action == DOOR_CLOSING) || (action == DOOR_CLOSED);
    crewElevator_status = action;

    CHECK_DOOR_JUMP(crewElevator_proc, anim_crewElevator);

    TriggerRedrawArea(AID_ELEVATORSWITCH);
    TriggerRedrawArea(AID_ELEVATORINDICATOR);
    UpdateCtrlDialog(this);
    RecordEvent("ELEVATOR", close ? "CLOSE" : "OPEN");
}

// invoked from key handler
void XR5Vanguard::ToggleElevator()
{
    ActivateElevator(crewElevator_status == DOOR_CLOSED || crewElevator_status == DOOR_CLOSING ?
            DOOR_OPENING : DOOR_CLOSING);
}

// Override the base class method so we can perform some additional checks
void XR5Vanguard::ActivateRadiator(DoorStatus action)
{
    // check for failure
    if (radiator_status == DOOR_FAILED)
    {
        PlayErrorBeep();
        ShowWarning("Warning Radiator Failure.wav", DeltaGliderXR1::ST_WarningCallout, "Radiator inoperative due to excessive&heat and/or dynamic pressure.");
        return;  // cannot move
    }

    if (CheckHydraulicPressure(true, true) == false)   // show warning if no hydraulic pressure
        return;     // no hydraulic pressure

    // cannot deploy or retract radiator if the payload bay doors are in motion
    // NOTE: allow for DOOR_FAILED here so that a bay door failure does not lock the radiator
    if ((bay_status == DOOR_OPENING) || (bay_status == DOOR_CLOSING))
    {
        PlayErrorBeep();
        ShowWarning("Warning Bay Doors in Motion Radiator is Locked.wav", DeltaGliderXR1::ST_WarningCallout, "Cannot deploy/retract radiator&while bay doors are in motion.");
        return;  // cannot move
    }

    // cannot deploy or retract radiator if bay doors are OPEN (they would collide)
    if (bay_status == DOOR_OPEN)
    {
        PlayErrorBeep();
        ShowWarning("Warning Bay Doors Open Radiator is Locked.wav", DeltaGliderXR1::ST_WarningCallout, "Cannot deploy/retract radiator&while bay doors are open.");
        return;  // cannot move
    }

    bool close = (action == DOOR_CLOSED || action == DOOR_CLOSING);
    radiator_status = action;

    CHECK_DOOR_JUMP(radiator_proc, anim_radiator);

    TriggerRedrawArea(AID_RADIATORSWITCH);
    TriggerRedrawArea(AID_RADIATORINDICATOR);

    UpdateCtrlDialog(this);
    RecordEvent ("RADIATOR", close ? "CLOSE" : "OPEN");
}

// prevent landing gear from being raised if the gear is not yet fully uncompressed
void XR5Vanguard::ActivateLandingGear(DoorStatus action)
{
    if (((action == DOOR_OPENING) || (action == DOOR_CLOSING)) && ((m_noseGearProc != 1.0) || (m_rearGearProc != 1.0)))
    {
        PlayErrorBeep();
        ShowWarning("Gear Locked.wav", DeltaGliderXR1::ST_WarningCallout, "Gear is still in contact with the&ground: cannot raise landing gear.");
        return;
    }

    // propogate to the superclass
    DeltaGliderXR1::ActivateLandingGear(action);
}

// Used for internal development testing only to tweak some internal value.
// This is invoked from the key handler as ALT-1 or ALT-2 are held down.  
// direction = true: increment value, false: decrement value
void XR5Vanguard::TweakInternalValue(bool direction)
{
#ifdef _DEBUG  // debug only!
#if 0
    const double stepSize = (oapiGetSimStep() * ELEVATOR_TRIM_SPEED * 0.02);
    
    // tweak hidden elevator trim to fix nose-up push
    double step = stepSize * (direction ? 1.0 : -1.0);
    m_hiddenElevatorTrimState += step;

    if (m_hiddenElevatorTrimState < -1.0)
        m_hiddenElevatorTrimState = -1.0;
    else if (m_hiddenElevatorTrimState > 1.0)
        m_hiddenElevatorTrimState = 1.0;

    SetControlSurfaceLevel(AIRCTRL_FLAP, m_hiddenElevatorTrimState);     
    sprintf(oapiDebugString(), "Hidden trim=%lf", m_hiddenElevatorTrimState);
#endif
#if 0
    const double stepSize = (oapiGetSimStep() * 0.05);  // 20 seconds to go from 0...1
    m_tweakedInternalValue += (direction ? stepSize : -stepSize);
    sprintf(oapiDebugString(), "tweakedInternalValue=%lf", m_tweakedInternalValue);
#endif
#endif
}

// Render hatch decompression exhaust stream
void XR5Vanguard::ShowHatchDecompression()
{
    // NOTE: I assume this structure is actually treated as CONST by the Orbiter core, since the animation 
    // structures not declared CONST either.
    static PARTICLESTREAMSPEC airvent = {
        0, 1.0, 15, 0.5, 0.3, 2, 0.3, 1.0, PARTICLESTREAMSPEC::EMISSIVE,
        PARTICLESTREAMSPEC::LVL_LIN, 0.1, 0.1,
        PARTICLESTREAMSPEC::ATM_FLAT, 0.1, 0.1
    };
    /* Postions are:
         NOSE
          
         1  2

         3  4
    */
    static const VECTOR3 pos[4] = 
    {
        {-1.824, 6.285, 18.504},   // left-front
        { 1.824, 6.285, 18.504},   // right-front
        {-2.158, 7.838, 5.292},    // left-rear
        { 2.158, 7.838, 5.292}     // right-rear
    };

    static const VECTOR3 dir[4] = 
    {
        {-0.802,  0.597, 0},
        { 0.802,  0.597, 0},  
        {-0.050,  0.988, 0},
        { 0.050,  0.988, 0}
    };

    hatch_vent = new PSTREAM_HANDLE[4];   // this will be freed automatically for us later
    hatch_venting_lvl = new double[4];    // ditto
    for (int i=0; i < 4; i++)
    {
        hatch_venting_lvl[i] = 0.4;
        hatch_vent[i] = AddParticleStream(&airvent, pos[i], dir[i], hatch_venting_lvl + i);
    }

    hatch_vent_t = GetAbsoluteSimTime();
}

// turn off hatch decompression exhaust stream; invoked form a PostStep
void XR5Vanguard::CleanUpHatchDecompression()
{
    for (int i=0; i < 4; i++)
        DelExhaustStream(hatch_vent[i]);
}

void XR5Vanguard::DefineMmuAirlock()
{
    switch (m_activeEVAPort)
    {
    case DOCKING_PORT:
        {
            const float airlockY = static_cast<float>(DOCKING_PORT_COORD.y);
            const float airlockZ = static_cast<float>(DOCKING_PORT_COORD.z);

#ifdef MMU
            //                      state,MinX, MaxX, MinY,             MaxY,             MinZ,             MaxZ
            UMmu.DefineAirLockShape(1, -0.66f, 0.66f, airlockY - 3.00f, airlockY + 0.20f, airlockZ - 0.66f, airlockZ + 0.66f);  
            VECTOR3 pos = { 0.0, airlockY + 2.0, airlockZ }; // this is the position where the Mmu will appear relative to the ship's local coordinates
            VECTOR3 rot = { 0.0, 0.0, 0.0 };  // straight up, facing forward
            UMmu.SetMembersPosRotOnEVA(pos, rot);
            UMmu.SetEjectPosRotRelSpeed(pos, rot, _V(0, 4.0, 0));  // jumped UP to bail out @ 4 meters-per-second
            UMmu.SetActiveDockForTransfer(0);       // ship-to-ship transfer enabled
#endif
            m_pActiveAirlockDoorStatus = &olock_status;
            break;
        }

    case CREW_ELEVATOR:
        {
            // PRE-1.3 RC2: Port location (deployed): -3.116, -9.092, 6.35 
            // NEW: Port location (deployed):         -3.116 + 0.7, -9.092 - 0.7, 6.35 
            // add X 0.6 and Y 0.7 here for post-1.3 RC2 coordinates
            const float airlockX = -3.116f - 0.6f;
            const float airlockY = -7.299f + 0.7f;   // position coordinates refer to the TOP of the astronaut, so we have to allow space along the Y axis

            const float airlockZ =  6.35f; 
            const float xDim = 4.692f / 2;   // width from center
            const float yDim = 2.772f / 2;   // height from center
            const float zDim = 3.711f / 2;   // depth from center

#ifdef MMU
            //                      state,MinX,          MaxX,            MinY,           MaxY,            MinZ,            MaxZ
            UMmu.DefineAirLockShape(1, airlockX - xDim, airlockX + xDim, airlockY - yDim, airlockY + yDim, airlockZ - zDim, airlockZ + zDim);  
            VECTOR3 pos = { airlockX, airlockY, airlockZ + zDim + 1.0}; // this is the position where the Mmu will appear relative to the ship's local coordinates
            VECTOR3 rot = { 0.0, 0.0, 0.0 };  // straight up, facing forward
            UMmu.SetMembersPosRotOnEVA(pos, rot);
            UMmu.SetEjectPosRotRelSpeed(pos, rot, _V(0, -2.0, 0));  // jumped DOWN to bail out @ 2 meters-per-second
            UMmu.SetActiveDockForTransfer(-1);       // ship-to-ship transfer disabled
#endif
            m_pActiveAirlockDoorStatus = &crewElevator_status;
            break;
        }

        // NOTE: default case should never happen!
    }

#ifdef MMU
    // UMmu bug: must set this every time we reset the docking port AFTER the port is defined!
    UMmu.SetMaxSeatAvailableInShip(MAX_PASSENGERS); // includes the pilot
    UMmu.SetCrewWeightUpdateShipWeightAutomatically(FALSE);  // we handle crew member weight ourselves
#endif
    // repaint both LEDs and the switch
    TriggerRedrawArea(AID_EVA_DOCKING_PORT_ACTIVE_LED); 
    TriggerRedrawArea(AID_EVA_CREW_ELEVATOR_ACTIVE_LED); 
    TriggerRedrawArea(AID_ACTIVE_EVA_PORT_SWITCH); 
}

// returns: true if EVA doors are OK, false if not
bool XR5Vanguard::CheckEVADoor()
{
    if (m_activeEVAPort == DOCKING_PORT)
        return DeltaGliderXR1::CheckEVADoor();

    // else it's the crew elevator
    // NOTE: if the gear has collapsed, cannot EVA via the elevator!  Also note that we cannot use GetGearFullyCompressedAltitude here, since that will be 0
    // even after gear collapse since GroundContact will be true.
    if ((crewElevator_status == DOOR_FAILED) || (GetAltitude(ALTMODE_GROUND) < (GEAR_FULLY_COMPRESSED_DISTANCE-0.2)))  // leave a 0.2-meter safety cushion
    {
        PlayErrorBeep();
        ShowWarning("Warning Elevator Failure.wav", DeltaGliderXR1::ST_WarningCallout, "Crew Elevator is damanged.");
        return false;
    }
    else if (crewElevator_status != DOOR_OPEN)
    {
        PlayErrorBeep();
        ShowWarning("Warning Elevator is Closed.wav", DeltaGliderXR1::ST_WarningCallout, "Crew Elevator is stowed.");
        return false;
    }

    return true;
}

// Set the camera to its default payload bay postion.
void XR5Vanguard::ResetCameraToPayloadBay()
{
    /* ORG
    const VECTOR3 pos = { 0, 7.755, 4.077 };
    const VECTOR3 dir = { 0.0, -0.297, -0.955 };  // look down to rear bottom of bay
    */

    // PRE-1.7: const VECTOR3 pos = { 0, 8.755, 4.077 };
    const VECTOR3 pos = { 0, 8.755 + 1.0, 4.077 };  // so we don't clip under the D3D9 client
    const VECTOR3 dir = { 0.0, -0.297, -0.955 };  // look down to rear bottom of bay

    SetCameraOffset(pos);
    SetXRCameraDirection(dir); 
}

// override clbkPanelRedrawEvent so we can limit our refresh rates for our custom screens
bool XR5Vanguard::clbkPanelRedrawEvent(int areaID, int event, SURFHANDLE surf)
{
    const XR5ConfigFileParser *pConfig = GetXR5Config();

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


// Returns max configured thrust for the specified thruster BEFORE taking atmosphere or 
// damage into account.
// index = 0-13
double XR5Vanguard::GetRCSThrustMax(const int index) const
{
    // obtain the "normal" RCS jet power from the superclass
    double rcsThrustMax = DeltaGliderXR1::GetRCSThrustMax(index);

    // if holding attitude, adjust RCS max thrust based on payload in the bay
    if (InAtm())
    {
        if ((m_customAutopilotMode == AP_ATTITUDEHOLD) || (m_customAutopilotMode == AP_DESCENTHOLD))
        {
            const double withPayloadMass = GetEmptyMass();        // includes payload
            const double payloadMass = GetPayloadMass();
            const double noPayloadMass = withPayloadMass - payloadMass;  // total mass without any payload
            const double multiplier = withPayloadMass / noPayloadMass;   // 1.0 = no payload, etc.
            rcsThrustMax *= multiplier;
            // DEBUG: sprintf(oapiDebugString(), "RCS multiplier=%lf", multiplier);
        }
    }

    return rcsThrustMax;
}

// --------------------------------------------------------------
// Apply custom skin to the current mesh instance
// --------------------------------------------------------------
void XR5Vanguard::ApplySkin()
{
    if (!exmesh) return;
    
    if (skin[0])    // xr5t.dds
	{
		oapiSetTexture(exmesh, 1, skin[0]);
        oapiSetTexture(exmesh, 4, skin[0]);
	}

    if (skin[1])     // xr5b.dds
	{
		oapiSetTexture(exmesh,  2, skin[1]);
        oapiSetTexture(exmesh, 17, skin[1]);
	}
}

// meshTextureID = vessel-specific constant that is translated to a texture index specific to our vessel's .msh file.  meshTextureID 
// NOTE: meshTextureID=VCPANEL_TEXTURE_NONE = -1 = "no texture" (i.e., "not applicable"); defined in Area.h.
// hMesh = OUTPUT: will be set to the mesh handle of the mesh associated with meshTextureID.
DWORD XR5Vanguard::MeshTextureIDToTextureIndex(const int meshTextureID, MESHHANDLE &hMesh)
{
    _ASSERTE(false);  // should never reach here!

    hMesh = nullptr;      
    return MAXDWORD;   // bogus
}