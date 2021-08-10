// ==============================================================
// XR2Ravenstar implementation class
//
// Copyright 2008-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR2Ravenstar.cpp
//
// Altea Aerospace: The Future Is Now
// ==============================================================

#define ORBITER_MODULE

#include "XR2Ravenstar.h"
#include "DlgCtrl.h"

#include <stdio.h>

#include "XR2AreaIDs.h"  
#include "XR2InstrumentPanels.h"
#include "XR1PreSteps.h"
#include "XR1PostSteps.h"
#include "XR1FuelPostSteps.h"
#include "XR1AnimationPoststep.h"

#include "XR2Globals.h"
#include "XR2PreSteps.h"
#include "XR2PostSteps.h"
#include "XRPayload.h"
#include "XR2PayloadBay.h"
// TODO: #include "XR2PayloadDialog.h"

#include "meshres.h"

// ==============================================================
// Static callback invoked by oapiLoadMeshGlobal; this will DECRYPT
// the XR2's global mesh once and only once when it is loaded.
// ==============================================================

static XR2Ravenstar *s_pVessel;   // initialized right before our callback is invoked

static void LoadMeshGlobalCallback(const MESHHANDLE hMesh, const bool firstload)
{
    // Note: we do not use 'firstload' here since 1) we don't need it, and 2) it will be harder to trace this code if we don't rely on it.
    s_pVessel->exmesh_tpl = hMesh;  // save it in the global so it's harder to trace since we aren't passing it around to the decryption code; this will be overwritten by the assignment from oapiLoadMeshGlobal when this call returns anyway

    // Note: this invokes code in the XR2 itself for two reasons: 1) to reuse existing code
    // that was present in the 1.0 release, and 2) to make it harder to trace.

    // parse the encrypted mesh in exmesh_tpl, but do not decrypt it yet
    s_pVessel->ParseEncryptedMesh();

    // Note: originally I had encryption designed to only decrypt the mesh inside clbkVisualCreated, but 
    // unfortunately that can only work if the mesh is still in its original state: Orbiter sets the 
    // animation state of the mesh before invoking clbkVisualCreated, and it will no longer decrypt 
    // correctly if any animation states have changed from the original state.

    // instantiate s_pVessel->m_pEncryptionEngine and decrypt the global mesh data via the handle in exmesh_tpl
    s_pVessel->DecryptMeshData();   // {ZZZ} comment this line out to disable mesh decryption (FOR TESTING ONLY!)
    s_pVessel->m_meshGroupVector.clear();    // so the hacker can't grab the data
    delete s_pVessel->m_pEncryptionEngine;   // so it won't hang around for examination via the debugger
}


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
// NOTE: this is called even if fast shutdown is enabled.
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

    return new XR2Ravenstar(vessel, flightmodel, new XR2ConfigFileParser());
}

// --------------------------------------------------------------
// Vessel cleanup
// Note: this is only called if fast shutdown is DISABLED.
// --------------------------------------------------------------

// NOTE: must receive this as a VESSEL2 ptr because that's how Orbiter calls it
DLLCLBK void ovcExit(VESSEL2 *vessel)
{
    // NOTE: in order to free up VESSEL3 data without this hack you would need to add an empty virtual destructor to the VESSEL2 class in VesselAPI.h.
    // Unfortunately that cannot occur now (from Martin) because it would break all existing add-ons linked against VESSEL2 (virtual table change).
    
    // This is a hack so that the VESSEL3_EXT, XR2Ravenstar, and VESSEL3 destructors will be invoked: it must
    // "skip over" the VESSEL2 class that does not have a virtual destructor and jump down to ours.
    // Invokes XR2Ravenstar destructor -> VESSEL3_EXT destructor -> VESSEL3 destructor
    VESSEL3_EXT *pXR2 = reinterpret_cast<VESSEL3_EXT *>( (((void **)vessel)-1) );  // bump vptr to VESSEL3_EXT subclass, which has a virtual destructor
    delete pXR2;
}

// ==============================================================
// Airfoil coefficient functions
// Return lift, moment and zero-lift drag coefficients as a
// function of angle of attack (alpha or beta)
//
// Note: these must be static so name collisions will not occur in subclasses.
// ==============================================================

// DG org: 0.015
// XR1 org: 0.030
#define PROFILE_DRAG 0.015

// 1. vertical lift component (wings and body)
static void VLiftCoeff (VESSEL *v, double aoa, double M, double Re, void *context, double *cl, double *cm, double *cd)
{
    const int nabsc = 9;
    // ORG: static const double AOA[nabsc] = {-180*RAD,-60*RAD,-30*RAD, -2*RAD, 15*RAD,20*RAD,25*RAD,60*RAD,180*RAD};
    // DG3 follows:
    static const double AOA[nabsc] =         {-180*RAD,-60*RAD,-30*RAD, -1*RAD, 15*RAD,20*RAD,25*RAD,50*RAD,180*RAD};

    // ORG: static const double CL[nabsc]  = {       0,      0,   -0.4,      0,    0.7,     1,   0.8,     0,      0};
    // DG3 follows:
    // NEW: static const double CL[nabsc]  = {       0,      0,   -0.4,      0,    0.7,     1,   0.2,     0,      0};
    // XR1 ORG: static const double CL[nabsc]  =         {       0,      0,   -0.4,      0,    0.7,     0.5, 0.2,     0,      0};
    static const double CL[nabsc]  =         {       0,      0,   -0.15,      0,    0.7,     0.5, 0.2,     0,      0};  // decrease negative lift to better hold negative pitch

    // ORG: static const double CM[nabsc]  = {       0,      0,  0.014, 0.0039, -0.006,-0.008,-0.010,     0,      0};
    // DG3 follows:
    static const double CM[nabsc]  =         {       0,  0.006,  0.014, 0.0034,-0.0054,-0.024,-0.00001,   0,      0};

    int i=0;    
    for (i = 0; i < nabsc-1 && AOA[i+1] < aoa; i++);
    double f = (aoa-AOA[i]) / (AOA[i+1]-AOA[i]);
    *cl = CL[i] + (CL[i+1]-CL[i]) * f;  // aoa-dependent lift coefficient
    *cm = CM[i] + (CM[i+1]-CM[i]) * f;  // aoa-dependent moment coefficient
    double saoa = sin(aoa);
    double pd = PROFILE_DRAG + 0.4*saoa*saoa;  // profile drag
    // DG3: these values are unchanged
    *cd = pd + oapiGetInducedDrag (*cl, WING_ASPECT_RATIO, WING_EFFICIENCY_FACTOR) + oapiGetWaveDrag (M, 0.75, 1.0, 1.1, 0.04);
    // profile drag + (lift-)induced drag + transonic/supersonic wave (compressibility) drag
}

// 2. horizontal lift component (vertical stabilisers and body)

static void HLiftCoeff (VESSEL *v, double beta, double M, double Re, void *context, double *cl, double *cm, double *cd)
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
XR2Ravenstar::XR2Ravenstar(OBJHANDLE hObj, int fmodel, XR2ConfigFileParser *pConfigFileParser) : 
    DeltaGliderXR1(hObj, fmodel, pConfigFileParser)
{
    // init new XR2 warning lights
    for (int i=0; i < XR2_WARNING_LIGHT_COUNT; i++)
        m_xr2WarningLights[i] = false;  // not lit

    // init new doors
    bay_status = DOOR_CLOSED;
    bay_proc   = 0.0;

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
XR2Ravenstar::~XR2Ravenstar()
{
    // Note: payload bay is cleaned up in our XRVessel base class
}

// ==============================================================
// Overridden callback functions
// ==============================================================

char mesh_name[256];
// --------------------------------------------------------------
// Set vessel class parameters
// -------------------------------- ------------------------------
void XR2Ravenstar::clbkSetClassCaps (FILEHANDLE cfg)
{
    // parse the configuration file
    // If parse fails, we shouldn't display a MessageBox here because the Orbiter main window 
    // keeps putting itself in the foreground, covering it up and making Orbiter look like it's hung.
    // Therefore, TakeoffAndLandingCalloutsAndCrashPostStep will blink a warning message for us 
    // if the parse fails.
    ParseXRConfigFile();  // common XR code

    // Note: this must be invoked here instead of the constructor so the subclass may override it!
    DefineAnimations();

    // define our payload bay and attachment points
    CreatePayloadBay();

    // *************** physical parameters **********************
    ramjet = new XR1Ramjet(this);

    VESSEL2::SetEmptyMass(EMPTY_MASS);   // 12000 matches DG3

    // slightly too small for bow shock: SetSize (9.475);     // DG3 = 9.0 meters radius
    SetSize (11.955);     // length / 2
    SetVisibilityLimit (7.5e-4, 1.5e-3);
    SetAlbedoRGB (_V(0.77,0.77,0.77));  // gray
    SetGravityGradientDamping (20.0);   // ? same as DG and XR5 for now
    
    SetCrossSections (_V(77.46, 238.98, 30.14));  

    SetMaxWheelbrakeForce(MAX_WHEELBRAKE_FORCE);  
    // OLD (incorrect!): SetPMI (_V(53.0, 186.9, 25.9));  
    SetPMI (_V(32.04, 42.56, 13.17));  // this is from the INERTIA TENSOR diagonal values in Shipedit.exe (77,000 samples)
    
    SetDockParams (DOCKING_PORT_COORD, _V(0,0,1), _V(0,1,0));
    SetGearParameters(1.0);  // NOTE: must init touchdown points here with gear DOWN!  This will be called again later by clbkPostCreation to init the "real" state from scenario file.
    EnableTransponder (true);
    SetTransponderChannel(201); // XPDR = 118.05 MHz

    // init APU runtime callout timestamp
    MarkAPUActive();  // reset the APU idle warning callout time

    // enable IDS so we transmit a docking signal
    DOCKHANDLE hDock = GetDockHandle(0);    // primary docking port
    EnableIDS(hDock, true);
    SetIDSChannel(hDock, 203);  // DOCK = 118.15 MHz
    
    // ******************** Attachment points **************************

    // top-center (for lifter attachment)
    // SET IN CONFIG FILE: CreateAttachment(true, _V(0,0,0), _V(0,-1,0), _V(0,0,1), "XS");

    // ******************** NAV radios **************************
    
    InitNavRadios(4);
    
    // ****************** propellant specs **********************
    
    const XR2ConfigFileParser *pConfig = GetXR2Config();

    // set tank configuration

    // Note: we need to keep the hidden SCRAM tank as small as possible, but still large enough
    // to handle full flow (9 kg/sec) at 30 fps @ 10x time acceleration (3 fps effective).  
    // That means 9 kg/sec / 3 fps = 3 kg flowed per timestep.  Therefore, a 5-kg hidden 
    // tank should be sufficient (66% safety margin).
    const double hiddenSCRAMTankSize = 5;
    switch (pConfig->RequirePayloadBayFuelTanks)
    {
    case 0:  // 0 = Internal fuel tanks are sized at 100% of normal capacity.  (default)
        max_rocketfuel = TANK1_CAPACITY;
        max_scramfuel  = TANK2_CAPACITY;    
        break;

        // NOTE: in order to flow fuel to the SCRAM engines we must have at least a small internal 
        // "buffer" tank from which the Orbiter core can draw fuel.
    case 1:  // 1 = There is no internal SCRAM tank, and main tanks only hold 75% of normal capacity.
        max_rocketfuel = TANK1_CAPACITY * 0.75;
        max_scramfuel  = hiddenSCRAMTankSize;
        m_SCRAMTankHidden = true;   // tank only contains fuel if there is a NON-EMPTY SCRAM TANK in the bay
        break;

    case 2:  // 2 = There is no internal SCRAM tank, and main tanks only hold 50% of normal capacity.
        max_rocketfuel = TANK1_CAPACITY * 0.50;
        max_scramfuel  = hiddenSCRAMTankSize;
        m_SCRAMTankHidden = true;   // tank only contains fuel if there is a NON-EMPTY SCRAM TANK in the bay
        break;

    default:     
        // should never happen!
        _ASSERTE(false);
        // use default config
        max_rocketfuel = TANK1_CAPACITY;
        max_scramfuel  = TANK2_CAPACITY;    
        break;
    }

    // NOTE: Orbiter seems to reset the 'current fuel mass' value this to zero later, since it expects the scenario file to be read
    // WARNING: do NOT init 'fuel mass' value (optional second argument) to > 0, because Orbiter will NOT set the tank value if the fraction is 
    // zero in the scenario file.
    ph_main  = CreatePropellantResource(max_rocketfuel);      // main tank (fuel + oxydant)
    ph_rcs   = CreatePropellantResource(RCS_FUEL_CAPACITY);   // RCS tank  (fuel + oxydant)
    ph_scram = CreatePropellantResource(max_scramfuel);       // scramjet fuel
    // ph_scram = CreatePropellantResource(max_scramfuel, max_scramfuel);         // scramjet fuel

    // **************** thruster definitions ********************
    
    double ispscale = (GetXR1Config()->EnableATMThrustReduction ? 0.8 : 1.0);
    // Reduction of thrust efficiency at normal pressure
    
    // increase level, srcrate, and lifetime
    PARTICLESTREAMSPEC contrail = {
        0,          // flags (unused)                              
        // BETA-1 ORG: 11.0,       // srcsize (meters)                            
        5.5,        // srcsize (meters)                            
        6,          // srcrate (Hz)                                
        150,        // emission velocity (m/s)                     
        // BETA-1b ORG: 0.3,        // emission velocity distribution randomization
        0.1,        // emission velocity distribution randomization
        7.5,        // average particle lifetime (seconds)         
        // BETA-1 ORG: 4,          // particle growth rate (m/s)                  
        2,          // particle growth rate (m/s)                  
        3.0,        // deceleration rate in atmosphere             
        // BETA-1 ORG: PARTICLESTREAMSPEC::DIFFUSE,   // lighting type                                  
        PARTICLESTREAMSPEC::EMISSIVE,  // lighting type                                  
        PARTICLESTREAMSPEC::LVL_PSQRT, // mapping between level and opacity              
        0, 2,                          // min/max levels for alpha mapping               
        PARTICLESTREAMSPEC::ATM_PLOG,  // mapping between atm params and particle opacity
        1e-4, 1                        // min/max values for alpha mapping               
    };
    // increase level
    PARTICLESTREAMSPEC exhaust_main = {
        0,          // flags (unused)                              
        // BETA-1 ORG: 3.0,        // srcsize (meters)                            
        1.5,        // srcsize (meters)                            
        16,         // srcrate (Hz)                                
        150,        // emission velocity (m/s)                     
        0.1,        // emission velocity distribution randomization
        0.2,        // average particle lifetime (seconds)         
        // BETA-1 ORG: 16,         // particle growth rate (m/s)                  
        8,          // particle growth rate (m/s)                  
        1.0,        // deceleration rate in atmosphere             
        PARTICLESTREAMSPEC::EMISSIVE,  // lighting type                                  
        PARTICLESTREAMSPEC::LVL_SQRT,  // mapping between level and opacity              
        0, 1,                          // min/max levels for alpha mapping               
        PARTICLESTREAMSPEC::ATM_PLOG,  // mapping between atm params and particle opacity
        1e-5, 0.1                      // min/max values for alpha mapping               
    };
    // increase level
    PARTICLESTREAMSPEC exhaust_hover = {
        0,          // flags (unused)                              
        // BETA-1 ORG: 2.0,        // srcsize (meters)                            
        1.0,        // srcsize (meters)                            
        20,         // srcrate (Hz)                                
        150,        // emission velocity (m/s)                     
        0.1,        // emission velocity distribution randomization
        0.15,       // average particle lifetime (seconds)         
        // BETA-1 ORG: 16,         // particle growth rate (m/s)                  
        8,          // particle growth rate (m/s)                  
        1.0,        // deceleration rate in atmosphere             
        PARTICLESTREAMSPEC::EMISSIVE,  // lighting type                                  
        PARTICLESTREAMSPEC::LVL_SQRT,  // mapping between level and opacity              
        0, 1,                          // min/max levels for alpha mapping               
        PARTICLESTREAMSPEC::ATM_PLOG,  // mapping between atm params and particle opacity
        1e-5, 0.1                      // min/max values for alpha mapping               
    };
    // increase level and particle lifetime
    PARTICLESTREAMSPEC exhaust_scram = {
        0,          // flags (unused)
        3.0,        // srcsize (meters)  [ NOTE: keeping BETA-1 size here ]
        25,         // srcrate (Hz)
        150,        // emission velocity (m/s)
        0.05,       // emission velocity distribution randomization
        15.0,       // average particle lifetime (seconds)
        // BETA-1 ORG: 3,          // particle growth rate (m/s)
        1.5,        // particle growth rate (m/s)
        1.0,        // deceleration rate in atmosphere
        PARTICLESTREAMSPEC::EMISSIVE,  // lighting type
        PARTICLESTREAMSPEC::LVL_SQRT,  // mapping between level and opacity
        0, 1,                          // min/max levels for alpha mapping
        PARTICLESTREAMSPEC::ATM_PLOG,  // mapping between atm params and particle opacity
        1e-5, 0.1                      // min/max values for alpha mapping
    };

    // NEW for XR2: retros  (NOT USED CURRENTLY)
    /* NOT USED
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
    */

    // NEW for XR2: LOX and Fuel dump particle streams; used by FuelDumpPostStep
    PARTICLESTREAMSPEC dump = {
        0,          // flags (unused)
        0.175,      // srcsize (meters) (as measured)
        100,        // srcrate (Hz)
        60,         // emission velocity (m/s)
        0.01,       // emission velocity distribution randomization
        4.0,        // average particle lifetime (seconds)
        0.45,       // particle growth rate (m/s)
        1.0,        // deceleration rate in atmosphere
        PARTICLESTREAMSPEC::EMISSIVE,  // lighting type
        PARTICLESTREAMSPEC::LVL_LIN,   // mapping between level and opacity
        0.8, 1.0,                      // min and max levels for level PLIN and PSQRT mapping types
        PARTICLESTREAMSPEC::ATM_FLAT,  // mapping between atm params and particle opacity
        0.8, 1.0                       // min/max values for alpha mapping
    };

    // clone the local variable in our member var
    m_pFuelDumpParticleStreamSpec = new PARTICLESTREAMSPEC;
    memcpy(m_pFuelDumpParticleStreamSpec, &dump, sizeof(PARTICLESTREAMSPEC));

#ifdef TODO_LATER
    if (pConfig->EnableBoilOffExhaustEffect)
    {
        // NEW for XR2: EnableBoilOffExhaustEffect 
        PARTICLESTREAMSPEC boilOffExhaust = {
            0,          // flags (unused)
            0.06,       // srcsize (meters)
            5.0,        // srcrate (Hz)
            1.6,        // emission velocity (m/s)
            0.01,       // emission velocity distribution randomization
            2.5,        // average particle lifetime (seconds)
            0.02,       // particle growth rate (m/s)
            6.0,        // deceleration rate in atmosphere
            PARTICLESTREAMSPEC::DIFFUSE,   // lighting type
            PARTICLESTREAMSPEC::LVL_SQRT,  // mapping between level and opacity
            0.0, 1.0,                      // min and max levels for level PLIN and PSQRT mapping types
            PARTICLESTREAMSPEC::ATM_PLOG,  // mapping between atm params and particle opacity
            1e-5, 0.1                      // min/max values for alpha mapping
        };

        // clone the local variable in our member var
        m_pBoilOffExhaustParticleStreamSpec = new PARTICLESTREAMSPEC;
        memcpy(m_pBoilOffExhaustParticleStreamSpec, &boilOffExhaust, sizeof(PARTICLESTREAMSPEC));
    }
#endif

    // handle new configurable ISP
    const double mainISP = GetXR1Config()->GetMainISP();
    // main thrusters
    th_main[0] = CreateThruster (_V(-4.488, 0, -10.254), _V(0,0,1), MAX_MAIN_THRUST[GetXR1Config()->MainEngineThrust], ph_main, mainISP, mainISP*ispscale);
    th_main[1] = CreateThruster (_V( 4.488, 0, -10.254), _V(0,0,1), MAX_MAIN_THRUST[GetXR1Config()->MainEngineThrust], ph_main, mainISP, mainISP*ispscale);

    thg_main = CreateThrusterGroup (th_main, 2, THGROUP_MAIN);
    // increase thruster flame: stock was 12, 1
    SURFHANDLE mainExhaustTex = oapiRegisterExhaustTexture("XR2Ravenstar\\ExhaustXR2");

    // XR1 org was 12 long, 0.811 wide
    // BETA-1 ORG: const double mainLscale = 14;
    const double mainLscale = 10;
    // BETA-1 ORG: const double mainWscale = 0.85;   // 1.18 as measured, but that is way too wide in the sim
    // NOTE: THIS IS *RADIUS*, not "Width" as the SDK docs say!  1.18/2 = 0.59
    const double mainWscale = 0.59;   // 1.18 as measured, but that is way too wide in the sim
    // BETA-1 ORG: const double mainExhaustZCoord = -10.254;
    // BETA-1d ORG: const double mainExhaustZCoord = -9.960;   // sink into engines more per Steve's request
    // needed to adjust this slightly for Steve's new engine cones
    const double mainExhaustZCoord = -9.960 - 0.18;  

// BETA-1 ORG: AddExhaustStream (th, _V(x, y, mainExhaustZCoord - 4), &exhaust_main);
    /* 
      Visual exhaust gimbaling note:

      There are two conditions in order for the Orbiter core to vector the exhaust flames to match the 
      thruster's gimbal direction:
        1) There must be one physical/logical thruster defined for each exhaust definition; i.e., exactly two in the XRs case
        2) For cosmetic reasons, the Y-coordinate center of the engine in the mesh must be exactly at 0.0; i.e., it must match the thruster location.

        Since the physical thruster must be set on the ship's centerline (Y coordinate = 0.0) to prevent the ship from pitching up 
        or down under acceleration, the ship's mesh must be shifted vertically such that the vertical (up-down) center of the engines 
        must be at Y=0.0 in Orbiter.  Since the XR2's physical engines are actually at Y=0.477, we cannot tie them directly to the 
        thruster location because then the flames would render 0.477-meter below the engine bells.  Visual exhaust gimbaling 
        tested and working if I do this:
            AddXRExhaust (th, mainLscale, mainWscale, mainExhaustTex); 
        ...but that makes the flames appear 0.477-meter below the engine bells.  Therefore, if we want visual engine gimbaling
        in the Mk II we will need the model to be shifted vertically such that the center of the engine bells are at 0.0 vertically.

        The XR5 flames cannot be visually gimbaled for two reasons: 
            1) there are *six* engines mapped to two logical engines (left/right), and
            2) none of the six engines are aligned with the centerline of the mesh anyway.
    */
#define ADD_MAIN_EXHAUST(th, x, y)  \
    AddXRExhaust (th, mainLscale, mainWscale, _V(x, y, mainExhaustZCoord), _V(0,0,-1), mainExhaustTex); \
    AddExhaustStream (th, _V(x, y, mainExhaustZCoord - 6), &exhaust_main);                            \
    AddExhaustStream (th, _V(x, y, mainExhaustZCoord - 12), &contrail)

    ADD_MAIN_EXHAUST(th_main[0], -4.488, 0.477);  // port
    ADD_MAIN_EXHAUST(th_main[1],  4.488, 0.477);  // starboard

    // retro thrusters (Y coord is always zero so we don't induce rotation)
    const double retroXCoord = 5.075;  // 5.068 as measured, but not quite enough
    const double retroZCoord = 0.659;
    th_retro[0] = CreateThruster (_V(-retroXCoord, 0, retroZCoord), _V(0, 0, -1.0), MAX_RETRO_THRUST, ph_main, mainISP, mainISP*ispscale);
    th_retro[1] = CreateThruster (_V( retroXCoord, 0, retroZCoord), _V(0, 0, -1.0), MAX_RETRO_THRUST, ph_main, mainISP, mainISP*ispscale);

    const double retroLscale = 1.5;    // XR1 was 1.5
    // BETA-1 ORG: const double retroWscale = 0.20;   // 0.431 as measured, but way too wide!  Is this actually radius???
    const double retroWscale = 0.2155;   // 0.431 is *radius*

    SURFHANDLE retroExhaustTex = oapiRegisterExhaustTexture("XR2Ravenstar\\ExhaustXR2-rcs");  // orange texture for this
#define ADD_RETRO_EXHAUST(th, x) \
    AddXRExhaust (th, retroLscale, retroWscale, _V(x, 0.461, retroZCoord), _V(0,0,1), retroExhaustTex)
    /* NO: AddExhaustStream (th, _V(x, 0, retroZCoord + 0.3), &exhaust_retro) */

    thg_retro = CreateThrusterGroup (th_retro, 2, THGROUP_RETRO);
    ADD_RETRO_EXHAUST(th_retro[0], -retroXCoord);
    ADD_RETRO_EXHAUST(th_retro[1], retroXCoord);

    // hover thrusters (simplified)
    // The two aft hover engines are combined into a single "logical" thruster,
    // but exhaust is rendered separately for both
    th_hover[0] = CreateThruster (_V(0, 0, 14.32), _V(0,1,0), MAX_HOVER_THRUST[GetXR1Config()->HoverEngineThrust], ph_main, mainISP, mainISP*ispscale);
    th_hover[1] = CreateThruster (_V(0, 0,-14.32), _V(0,1,0), MAX_HOVER_THRUST[GetXR1Config()->HoverEngineThrust], ph_main, mainISP, mainISP*ispscale);
    thg_hover = CreateThrusterGroup (th_hover, 2, THGROUP_HOVER);

    // XR1 was 6, 0.5
    // BETA-1 ORG: const double hoverLscale = 7;
    const double hoverLscale = 5.5;
    // BETA-1 ORG: const double hoverWscale = 0.65;   // 0.892 as measured
    const double hoverWscale = 0.446;   // 0.892 DIAMETER as measured
    const double sinkIntoHover = 0.2;  // sink exhaust flame into hover engines

#define ADD_HOVER_EXHAUST(th, x, y, z)  \
    AddXRExhaust (th, hoverLscale, hoverWscale, _V(x, y + sinkIntoHover, z), _V(0,-1,0), mainExhaustTex); \
    AddExhaustStream (th, _V(x, y - 2.5, z), &exhaust_hover);                                           \
    AddExhaustStream (th, _V(x, y - 5, z), &contrail)

    // forward
    ADD_HOVER_EXHAUST(th_hover[0],  0.0, -1.430, 1.447);

    // aft 
    ADD_HOVER_EXHAUST(th_hover[1],  4.481, -0.659, -7.41);
    ADD_HOVER_EXHAUST(th_hover[1], -4.481, -0.659, -7.41);

    // define thruster locations in meters from the ship's centerpoint
    const double shipLength = 23.91;
    const double rcsZHullDistance = (shipLength / 2) - 1.0;   // distance from Z centerline -> RCS fore and aft
    const double rcsXWingDistance = 8.0;                      // distance from X centerline -> RCS on wings  (cheat a bit...)

    // set of attitude thrusters (idealised). The arrangement is such that no angular
    // momentum is created in linear mode, and no linear momentum is created in rotational mode.
    SURFHANDLE rcsExhaustTex = retroExhaustTex;  // orange texture for this, too
    THRUSTER_HANDLE th_att_rot[4], th_att_lin[4];
    
    // create RCS thrusters
    // NOTE: save in th_rcs array so we can disable them later
    th_rcs[0] = th_att_rot[0] = th_att_lin[0] = CreateThruster (_V(0, 0, rcsZHullDistance), _V(0, 1,0), GetRCSThrustMax(0), ph_rcs, mainISP);  // fore bottom (i.e., pushes UP from the BOTTOM of the hull)
    th_rcs[1] = th_att_rot[1] = th_att_lin[3] = CreateThruster (_V(0, 0,-rcsZHullDistance), _V(0,-1,0), GetRCSThrustMax(1), ph_rcs, mainISP);  // aft top
    th_rcs[2] = th_att_rot[2] = th_att_lin[2] = CreateThruster (_V(0, 0, rcsZHullDistance), _V(0,-1,0), GetRCSThrustMax(2), ph_rcs, mainISP);  // fore top
    th_rcs[3] = th_att_rot[3] = th_att_lin[1] = CreateThruster (_V(0, 0,-rcsZHullDistance), _V(0, 1,0), GetRCSThrustMax(3), ph_rcs, mainISP);  // aft bottom
    CreateThrusterGroup (th_att_rot,   2, THGROUP_ATT_PITCHUP);
    CreateThrusterGroup (th_att_rot+2, 2, THGROUP_ATT_PITCHDOWN);
    CreateThrusterGroup (th_att_lin,   2, THGROUP_ATT_UP);
    CreateThrusterGroup (th_att_lin+2, 2, THGROUP_ATT_DOWN);

    // BETA-1d ORG: const double rcsLscale = 0.7;    // XR1 was 0.6 or 0.79
    const double rcsLscale = 0.6;    // XR1 was 0.6 or 0.79
    // BETA-1 ORG: const double rcsWscale = 0.200;  // 0.221 as measured; XR1 was 0.078 or 0.103
    // BETA-1d ORG: const double rcsWscale = 0.1105;  // 0.221 as measured (diameter); XR1 was 0.078 or 0.103
    // const double rcsWscale = 0.0885;  // as measured (radius)
    const double rcsWscale = 0.07;  // slighly narrower
    // Note: actual rcs pit depth is 0.184 meter for standard hull-mounted jets
    const double rcsDepthModifier = 0.170;     // leave slightly below thruster surface

// Note: no exhaust smoke for RCS jets.
#define ADD_RCS_EXHAUST_YPAIR(th, x, y, z, upOrDown, directionV)                                     \
        AddXRExhaust (th, rcsLscale, rcsWscale, _V( x, RCS_DCOORD(y, upOrDown), z), directionV, rcsExhaustTex);   \
        AddXRExhaust (th, rcsLscale, rcsWscale, _V(-(x), RCS_DCOORD(y, upOrDown), z), directionV, rcsExhaustTex)

#define ADD_RCS_EXHAUST(th, coordsV, directionV)                                     \
        AddXRExhaust (th, rcsLscale, rcsWscale, coordsV, directionV, rcsExhaustTex)

// compute actual RCS depth coordinate; this is necessary for hull-mounted RCS jets
#define RCS_DCOORD(c, dir) (c + (dir * rcsDepthModifier))

    // Note: these four exhausts are angled, but they rotate the ship around the Y axis
    const VECTOR3 &forwardRCSYVectorLeft  = _V(-0.527, -0.844, 0.102);
    const VECTOR3 &forwardRCSYVectorRight = _V( 0.527, -0.844, 0.102);
    ADD_RCS_EXHAUST(th_rcs[0], _V(-1.417, -0.339, 8.696), forwardRCSYVectorLeft);   // nose bottom forward port
    ADD_RCS_EXHAUST(th_rcs[0], _V( 1.417, -0.339, 8.696), forwardRCSYVectorRight);  // nose bottom forward starboard

    ADD_RCS_EXHAUST(th_rcs[0], _V(-1.461, -0.339, 8.411), forwardRCSYVectorLeft);   // nose bottom aft port
    ADD_RCS_EXHAUST(th_rcs[0], _V( 1.461, -0.339, 8.411), forwardRCSYVectorRight);  // nose bottom aft starboard

    ADD_RCS_EXHAUST_YPAIR (th_rcs[1], 4.655, 1.287, -8.309, 1, _V(0,1,0));  // aft top forward
    ADD_RCS_EXHAUST_YPAIR (th_rcs[1], 4.654, 1.280, -8.563, 1, _V(0,1,0));  // aft top middle 
    ADD_RCS_EXHAUST_YPAIR (th_rcs[1], 4.655, 1.250, -8.815, 1, _V(0,1,0));  // aft top aft

    ADD_RCS_EXHAUST_YPAIR (th_rcs[2], 0.267, 1.163,  9.670, 1, _V(0,1,0));  // nose top right/left
    ADD_RCS_EXHAUST       (th_rcs[2], _V( 0.0, RCS_DCOORD(1.185, 1), 9.675), _V(0,1,0));  // nose top center

    ADD_RCS_EXHAUST_YPAIR (th_rcs[3], 4.486, -0.475, -8.299, -0.80, _V(0,-1,0));  // aft bottom forward
    ADD_RCS_EXHAUST_YPAIR (th_rcs[3], 4.487, -0.436, -8.560, -0.80, _V(0,-1,0));  // aft bottom middle
    ADD_RCS_EXHAUST_YPAIR (th_rcs[3], 4.489, -0.395, -8.815, -0.80, _V(0,-1,0));  // aft bottom aft
    
    th_rcs[4] = th_att_rot[0] = th_att_lin[0] = CreateThruster (_V(0, 0, rcsZHullDistance), _V(-1,0,0), GetRCSThrustMax(4), ph_rcs, mainISP);  // fore right side
    th_rcs[5] = th_att_rot[1] = th_att_lin[3] = CreateThruster (_V(0, 0,-rcsZHullDistance), _V( 1,0,0), GetRCSThrustMax(5), ph_rcs, mainISP);  // aft left side
    th_rcs[6] = th_att_rot[2] = th_att_lin[2] = CreateThruster (_V(0, 0, rcsZHullDistance), _V( 1,0,0), GetRCSThrustMax(6), ph_rcs, mainISP);  // fore left side
    th_rcs[7] = th_att_rot[3] = th_att_lin[1] = CreateThruster (_V(0, 0,-rcsZHullDistance), _V(-1,0,0), GetRCSThrustMax(7), ph_rcs, mainISP);  // aft right side
    CreateThrusterGroup (th_att_rot,   2, THGROUP_ATT_YAWLEFT);   // rotate LEFT on Y axis (-y)
    CreateThrusterGroup (th_att_rot+2, 2, THGROUP_ATT_YAWRIGHT);  // rotate RIGHT on Y axis (+y)
    CreateThrusterGroup (th_att_lin,   2, THGROUP_ATT_LEFT);      // translate LEFT along X axis (-x)
    CreateThrusterGroup (th_att_lin+2, 2, THGROUP_ATT_RIGHT);     // translate RIGHT along X axis (+x)

    ADD_RCS_EXHAUST (th_rcs[4], _V(RCS_DCOORD(1.081, 1.5), 0.444, 9.658), _V(1,0,0));   // fore right side forward (X axis)
    ADD_RCS_EXHAUST (th_rcs[4], _V(RCS_DCOORD(1.146, 1.5), 0.443, 9.386), _V(1,0,0));   // fore right side aft

    ADD_RCS_EXHAUST (th_rcs[5], _V(RCS_DCOORD(-5.365, -1), 0.464, -8.309), _V(-1,0,0));  // aft left side forward
    ADD_RCS_EXHAUST (th_rcs[5], _V(RCS_DCOORD(-5.350, -1), 0.467, -8.562), _V(-1,0,0));  // aft left side middle
    ADD_RCS_EXHAUST (th_rcs[5], _V(RCS_DCOORD(-5.321, -1), 0.465, -8.814), _V(-1,0,0));  // aft left side aft

    ADD_RCS_EXHAUST (th_rcs[6], _V(RCS_DCOORD(-1.081, -1.5), 0.444, 9.658), _V(-1,0,0));  // fore left side forward
    ADD_RCS_EXHAUST (th_rcs[6], _V(RCS_DCOORD(-1.146, -1.5), 0.443, 9.386), _V(-1,0,0));  // fore left side aft

    ADD_RCS_EXHAUST (th_rcs[7], _V( RCS_DCOORD( 5.365, 1), 0.464, -8.309), _V(1,0,0));  // aft right side forward
    ADD_RCS_EXHAUST (th_rcs[7], _V( RCS_DCOORD( 5.350, 1), 0.467, -8.562), _V(1,0,0));  // aft right side middle
    ADD_RCS_EXHAUST (th_rcs[7], _V( RCS_DCOORD( 5.321, 1), 0.465, -8.814), _V(1,0,0));  // aft right side aft

    th_rcs[8]  = th_att_rot[0] = CreateThruster (_V( rcsXWingDistance, 0, 0), _V(0, 1,0), GetRCSThrustMax(8),  ph_rcs, mainISP);    // right wing bottom
    th_rcs[9]  = th_att_rot[1] = CreateThruster (_V(-rcsXWingDistance, 0, 0), _V(0,-1,0), GetRCSThrustMax(9),  ph_rcs, mainISP);    // left wing top
    th_rcs[10] = th_att_rot[2] = CreateThruster (_V(-rcsXWingDistance, 0, 0), _V(0, 1,0), GetRCSThrustMax(10), ph_rcs, mainISP);    // left wing bottom
    th_rcs[11] = th_att_rot[3] = CreateThruster (_V( rcsXWingDistance, 0, 0), _V(0,-1,0), GetRCSThrustMax(11), ph_rcs, mainISP);    // right wing top
    CreateThrusterGroup (th_att_rot,   2, THGROUP_ATT_BANKLEFT);  // rotate LEFT on Z axis (-Z)
    CreateThrusterGroup (th_att_rot+2, 2, THGROUP_ATT_BANKRIGHT); // rotate RIGHT on Z axis (+Z)

    // mesh decryption: invoke InitEncryptedMeshHandler with our secret key
    // sneaky: we put this down here so it will be less visible in this method.
    const unsigned char secretKey[] = { 0x5C, 0xF9, 0xF9, 0x5F, 0x99, 0xDA, 0x3A, 0x3C, 0x58, 0x1A, 0xD8, 0x5C, 0x3A, 0xD8, 0x3C, 0xD8, 0xF9, 0x99, 0xB9, 0x98, 0xFE, 0x99, 0xDE, 0x1A, 0x32, 0x52, 0x92, 0xB2, 0x53 };
    InitEncryptedMeshHandler(secretKey, sizeof(secretKey));  // this will clone the secretKey and store it in our base class

    // Rotation exhaust: note that these exhausts share coordinates with other thrusters, since they do "double-duty."
    // These are logically mounted on the wings, but we re-use three jets per side to rotate the ship along the Z axis
    ADD_RCS_EXHAUST (th_rcs[8], _V( 4.486, RCS_DCOORD(-0.475, -1), -8.299), _V(0,-1,0));  // right side bottom fore
    ADD_RCS_EXHAUST (th_rcs[8], _V( 4.487, RCS_DCOORD(-0.436, -1), -8.560), _V(0,-1,0));  // right side bottom middle
    ADD_RCS_EXHAUST (th_rcs[8], _V( 4.489, RCS_DCOORD(-0.395, -1), -8.815), _V(0,-1,0));  // right side bottom aft

    ADD_RCS_EXHAUST (th_rcs[9], _V(-4.655, RCS_DCOORD(1.287, 1), -8.309), _V(0,1,0));  // left side top fore
    ADD_RCS_EXHAUST (th_rcs[9], _V(-4.654, RCS_DCOORD(1.280, 1), -8.563), _V(0,1,0));  // left side top middle
    ADD_RCS_EXHAUST (th_rcs[9], _V(-4.655, RCS_DCOORD(1.250, 1), -8.815), _V(0,1,0));  // left side top aft

    ADD_RCS_EXHAUST (th_rcs[10], _V(-4.486, RCS_DCOORD(-0.475, -1), -8.299), _V(0,-1,0));  // left side bottom fore
    ADD_RCS_EXHAUST (th_rcs[10], _V(-4.487, RCS_DCOORD(-0.436, -1), -8.560), _V(0,-1,0));  // left side bottom middle
    ADD_RCS_EXHAUST (th_rcs[10], _V(-4.489, RCS_DCOORD(-0.395, -1), -8.815), _V(0,-1,0));  // left side bottom aft

    ADD_RCS_EXHAUST (th_rcs[11], _V( 4.655, RCS_DCOORD(1.287, 1), -8.309), _V(0,1,0));  // right side top fore
    ADD_RCS_EXHAUST (th_rcs[11], _V( 4.654, RCS_DCOORD(1.280, 1), -8.563), _V(0,1,0));  // right side top middle
    ADD_RCS_EXHAUST (th_rcs[11], _V( 4.655, RCS_DCOORD(1.250, 1), -8.815), _V(0,1,0));  // right side top aft

    // put the RCS directly on the X and Y centerlines so we don't induce any rotation while translating fore/aft
    th_rcs[12] = th_att_lin[0] = CreateThruster (_V(0, 0,-rcsZHullDistance), _V(0, 0, 1), GetRCSThrustMax(12), ph_rcs, mainISP);   // aft
    th_rcs[13] = th_att_lin[1] = CreateThruster (_V(0, 0, rcsZHullDistance), _V(0, 0,-1), GetRCSThrustMax(13), ph_rcs, mainISP);   // fore
    CreateThrusterGroup (th_att_lin,   1, THGROUP_ATT_FORWARD);   // translate FORWARD along Z axis (+z)
    CreateThrusterGroup (th_att_lin+1, 1, THGROUP_ATT_BACK);      // translate BACKWARD along Z axis (-z)

    // use longer exhaust here because rear RCS jets are stronger than others
    // BETA-1D ORG: rcsLscale * 2.5
#define ADD_RCS_EXHAUST_ZPAIR(th, x, y, z, foreOrAft, directionV)                                     \
        AddXRExhaust (th, rcsLscale * 2.3, rcsWscale * 1.2, _V(  x , y, RCS_DCOORD(z, foreOrAft)), directionV, rcsExhaustTex);   \
        AddXRExhaust (th, rcsLscale * 2.3, rcsWscale * 1.2, _V(-(x), y, RCS_DCOORD(z, foreOrAft)), directionV, rcsExhaustTex)

    // BETA-1D ORG: const double zShiftAft = -0.435;  // delta from endpoint of aft Z thrusters
    const double zShiftAft = -0.25;  // delta from endpoint of aft Z thrusters
    ADD_RCS_EXHAUST_ZPAIR (th_rcs[12], 0.162, 0.478, -8.39 + zShiftAft , -1, _V(0,0,-1));   // aft Z axis 

    // forward translation jets (Z axis) on the side of the nose
    // Note: these vectors are not straight along the Z axis because the jets fire on an angle
    const VECTOR3 &forwardRCSZVectorLeft  = _V(-0.313, 0.0, 0.95);
    const VECTOR3 &forwardRCSZVectorRight = _V( 0.313, 0.0, 0.95);

#define ADD_RCS_EXHAUST_Z(th, coordsV, directionV)                                     \
        AddXRExhaust (th, rcsLscale * 2.0, rcsWscale, coordsV, directionV, rcsExhaustTex)

    ADD_RCS_EXHAUST_Z(th_rcs[13], _V(-1.484, 0.444, 8.802), forwardRCSZVectorLeft);   // forward-port
    ADD_RCS_EXHAUST_Z(th_rcs[13], _V( 1.484, 0.444, 8.802), forwardRCSZVectorRight);  // forward-starboard

    ADD_RCS_EXHAUST_Z(th_rcs[13], _V(-1.603, 0.442, 8.383), forwardRCSZVectorLeft);  // aft-port
    ADD_RCS_EXHAUST_Z(th_rcs[13], _V( 1.603, 0.442, 8.383), forwardRCSZVectorRight); // aft-starboard

    // **************** scramjet definitions ********************
    
    VECTOR3 dir = {0.0, sin(SCRAM_DEFAULT_DIR), cos(SCRAM_DEFAULT_DIR)};
    
    const double scramX = 1.931;  // X distance from centerline
    const double scramY = -0.523;
    const double scramZ = -6.141;
    for (int i = 0; i < 2; i++) 
    {
        // use zero for SCRAM y coordinate so we never induce rotation
        th_scram[i] = CreateThruster (_V((i ? scramX : -scramX), 0, scramZ), _V(0, 0, 1), 0, ph_scram, 0);
        ramjet->AddThrusterDefinition (th_scram[i], SCRAM_FHV[GetXR1Config()->SCRAMfhv],
            SCRAM_INTAKE_AREA, SCRAM_INTERNAL_TEMAX, GetXR1Config()->GetScramMaxEffectiveDMF());
    }
    
    // thrust rating and ISP for scramjet engines are updated continuously
    // move exhaust smoke away from engines a bit
    const double scramDelta = -8;  // XR1 org: 3
    PSTREAM_HANDLE ph;
    ph = AddExhaustStream (th_scram[0], _V(-scramX, scramY, scramZ + scramDelta), &exhaust_scram);
    // Note: ph will be null if exhaust streams are disabled
    if (ph != nullptr)
        oapiParticleSetLevelRef (ph, scram_intensity+0);

    ph = AddExhaustStream (th_scram[1], _V( scramX, scramY, scramZ + scramDelta), &exhaust_scram);
    if (ph != nullptr)
        oapiParticleSetLevelRef (ph, scram_intensity+1);
    
    // ********************* aerodynamics ***********************
    
    // NOTE: org values were causing nasty downward pitch in the atmospehere: 
    // hwing = CreateAirfoil3 (LIFT_VERTICAL, _V(0,0,-0.3), VLiftCoeff, NULL, 5, 90, 1.5);

    XR1Multiplier = 1.34;        // control surface area vs. the XR1  
    
    // Note: XR5 was XR1Multiplier * 3
    // XR2 TWEAK: REDUCE PITCH "twitchiness" BY 40%
    // saves baseline elevator values in globals so we can read them later
    // BETA-1: m_baselineElevatorArea = 1.2 * XR1Multiplier * 0.6;
    m_baselineElevatorArea = 1.2 * XR1Multiplier * 0.7;  // BETA-1b: decrease default power since gear moved farther forward
    m_ctrlSurfacesDeltaZ = -10.133;   // distance from center of model to center of control surfaces, Z axis
    m_elevatorCL = 1.4;

    // center of lift matches center of mass
    // NOTE: this airfoil's force attack point will be modified by the SetCenterOfLift PreStep 
    hwing = CreateAirfoil3 (LIFT_VERTICAL, _V(m_wingBalance, 0, m_centerOfLift), VLiftCoeff, NULL, 5 * XR1Multiplier, WING_AREA, WING_ASPECT_RATIO);  
    
    ReinitializeDamageableControlSurfaces();  // create ailerons, elevators, and elevator trim

    // vertical stabiliser and body lift and drag components
    CreateAirfoil3 (LIFT_HORIZONTAL, _V(0, 0, m_ctrlSurfacesDeltaZ), HLiftCoeff, NULL, 5 * XR1Multiplier, 15 * XR1Multiplier, 1.5);
    CreateControlSurface(AIRCTRL_RUDDER,       0.8 * XR1Multiplier, 1.5, _V(   0,0, m_ctrlSurfacesDeltaZ), AIRCTRL_AXIS_YPOS, anim_rudder);

    // Create a hidden elevator trim to fix the nose-up tendency on liftoff and allow the elevator trim to be truly neutral.
    // We have to use FLAP here because that is the only unused control surface type.  We could probably also duplicate this via CreateAirfoil3, but this
    // is easier to adjust and test.
    CreateControlSurface(AIRCTRL_FLAP, 0.3 * XR1Multiplier, 1.5, _V(   0,0, m_ctrlSurfacesDeltaZ), AIRCTRL_AXIS_XPOS);  // no animation for this!
    m_hiddenElevatorTrimState = HIDDEN_ELEVATOR_TRIM_STATE;        // set to a member variable in case we want to change it in flight later
    // Note: cannot set the level here; it is reset by Orbiter later.

    const double xr1VariableDragModifier = 1.34;    // this is the empty mass ratio of the XR2:XR1
	// we need these goofy const casts to force the linker to link with the 'const double *' version of CreateVariableDragElement instead of the legacy 'double *' version of the function (which still exists but is deprecated and causes warnings in orbiter.log)
    CreateVariableDragElement(const_cast<const double *>(&rcover_proc),    0.3 * xr1VariableDragModifier, _V(0, 0.461,  0.659));      // retro covers : XR1 was 0.2
    CreateVariableDragElement(const_cast<const double *>(&radiator_proc),  0.4 * xr1VariableDragModifier, _V(0, 1.216, -4.755));      // radiators
    CreateVariableDragElement(const_cast<const double *>(&bay_proc),       6.0 * xr1VariableDragModifier, _V(0, 1.751, -1.876));      // bay doors (drag is at rear of bay)
    CreateVariableDragElement(const_cast<const double *>(&gear_proc),      0.8 * 1.0                    , _V(0, -2.635, -0.329));     // landing gear (gear same size as XR1).  Z coord is average for all three touchdown points.
    CreateVariableDragElement(const_cast<const double *>(&nose_proc),      3.1 * xr1VariableDragModifier, _V(0, 0.134, 10.500));      // nosecone open (against airlock door)
    CreateVariableDragElement(const_cast<const double *>(&brake_proc),     4.0 * xr1VariableDragModifier, _V(0, 0, m_ctrlSurfacesDeltaZ));  // airbrake (do not induce a rotational moment here)
    CreateVariableDragElement(const_cast<const double *>(&hoverdoor_proc), 1.5 * xr1VariableDragModifier, _V(0.0, -0.756-0.63, -6.786));     // center of deployed rear hover doors
    
    // BETA-1: const double dragMultiplier = 1.34;  
    const double dragMultiplier = 2.0;  
    SetRotDrag (_V(0.10 * dragMultiplier, 0.13 * dragMultiplier, 0.04 * dragMultiplier));

    // define hull temperature limits (matches XR1 and XR5 for now)
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
    // sneaky: save the first decrypted vertex values so the superclass can tell whether the mesh is encrypted
    if (s_pFirstDecryptedVertex == nullptr)    // not set yet?
    {
       s_pFirstDecryptedVertex = new NTVERTEX;   // allocate memory; this is never freed explicitly: it happens automatically when the DLL is unloaded
       // {ZZZ} CHECK/UPDATE THIS EACH TIME THE MESH CHANGES
       NTVERTEX temp = { 5.3994f, -0.0735f, -9.5987f, 0.0153f, -0.9996f, -0.0251f, 0.1468f, 0.9257f };
       memcpy(s_pFirstDecryptedVertex, &temp, sizeof(temp));
    }
    
    // ********************* beacon lights **********************
    const double bd = 0.15;  // beacon delta from the mesh edge
    static VECTOR3 beaconpos[7] = 
    {
        {-9.43, -0.207 + bd, -7.24 - bd},     // nav: left wing
        { 9.43, -0.207 + bd, -7.24 + bd},     // nav: right wing
        { 0.0,  -0.033 + bd,-11.243 + bd},    // nav: aft center  (NOTE: move FORWARD slightly)
        { 0.0,   2.461 + bd,  6.113},         // beacon: top hull, bottom hull
        { 0.0,  -1.376 - bd, -1.095},         // beacon: bottom hull
        {-2.697, 4.997 + bd, -9.112},         // strobe: left rudder top
        { 2.697, 4.997 + bd, -9.112},         // strobe: right rudder top
    };
    
    // RGB colors
    static VECTOR3 beaconcol[7] = 
    {
        {1.0,0.5,0.5}, {0.5,1.0,0.5}, {1,1,1},  // nav RGB colors
        {1,0.6,0.6}, {1,0.6,0.6},               // beacon
        {1,1,1}, {1,1,1}                        // strobe
    };  

    // ORG: const double sizeMultiplier = 1.34;
    const double sizeMultiplier = 1.0;
    for (int i = 0; i < 7; i++) 
    {  
        beacon[i].shape = (i < 3 ? BEACONSHAPE_DIFFUSE : BEACONSHAPE_STAR);
        beacon[i].pos = beaconpos+i;
        beacon[i].col = beaconcol+i;
        // ORG: beacon[i].size = (i < 3 ? 0.3 * sizeMultiplier : 0.55 * sizeMultiplier);
        beacon[i].size = (i < 3 ? 0.2 * sizeMultiplier : 0.55 * sizeMultiplier);
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

    // add a light at each main engine
    const double mainEnginePointLightPower = 100 * 1.57;  // XR2 engines are 1.57 times as powerful as the XR1's
    const double zMainLightDelta = -1.0;
    if (GetXR1Config()->EnableEngineLightingEffects)
    {
	    LightEmitter *leMainPort      = AddPointLight(_V(-4.488, 0.477, mainExhaustZCoord + zMainLightDelta), mainEnginePointLightPower, 1e-3, 0, 2e-3, col_d, col_s, col_a);
        LightEmitter *leMainStarboard = AddPointLight(_V( 4.488, 0.477, mainExhaustZCoord + zMainLightDelta), mainEnginePointLightPower, 1e-3, 0, 2e-3, col_d, col_s, col_a);
	    leMainPort->SetIntensityRef(&m_mainThrusterLightLevel);
        leMainStarboard->SetIntensityRef(&m_mainThrusterLightLevel);
    }

    // add a light at each hover engine
    if (GetXR1Config()->EnableEngineLightingEffects)
    {
        const double hoverEnginePointLightPower = mainEnginePointLightPower * 0.6875;  // hovers are .6875 the thrust of the mains
        const double yHoverLightDelta = -1.0;
        LightEmitter *leForward      = AddPointLight (_V( 0.000, -1.430 + yHoverLightDelta,  1.447), hoverEnginePointLightPower, 1e-3, 0, 2e-3, col_d, col_s, col_a); 
        LightEmitter *leAftPort      = AddPointLight (_V(-4.481, -0.659 + yHoverLightDelta, -7.410), hoverEnginePointLightPower, 1e-3, 0, 2e-3, col_d, col_s, col_a); 
        LightEmitter *leAftStarboard = AddPointLight (_V( 4.481, -0.659 + yHoverLightDelta, -7.410), hoverEnginePointLightPower, 1e-3, 0, 2e-3, col_d, col_s, col_a); 
	    leForward->SetIntensityRef(&m_hoverThrusterLightLevel);
        leAftPort->SetIntensityRef(&m_hoverThrusterLightLevel);
        leAftStarboard->SetIntensityRef(&m_hoverThrusterLightLevel);
    }

    // add docking lights (our only 2 spotlights for now)
    m_pSpotlights[0] = static_cast<SpotLight *>(AddSpotLight(_V( 2.190, 0.0035, 8.053), _V(0,0,1), 150, 1e-3, 0, 1e-3, RAD*25, RAD*60, col_white, col_white, col_a));
    m_pSpotlights[1] = static_cast<SpotLight *>(AddSpotLight(_V(-2.190, 0.0035, 8.053), _V(0,0,1), 150, 1e-3, 0, 1e-3, RAD*25, RAD*60, col_white, col_white, col_a));

    // turn all spotlights off by default
    for (int i=0; i < SPOTLIGHT_COUNT; i++)
        m_pSpotlights[i]->Activate(false);

    //
    // Load meshes
    //

    // NOTE: this will be decrypted once and only once in LoadMeshGlobalCallback.
    s_pVessel = this;   // save so static callback can reference it
    exmesh_tpl = oapiLoadMeshGlobal("XR2Ravenstar\\XR2Ravenstar", LoadMeshGlobalCallback);     // exterior mesh

    // load the heating mesh; this mesh is NOT encrypted
    heatingmesh_tpl = oapiLoadMeshGlobal("XR2Ravenstar\\RavenstarHeatShield");

    // this call associates a global mesh with a vessel; it does not actually instantiate a copy of the mesh yet
    // Note: the encrypted mesh must be mesh #0, so it must be added *first*
    // This mesh is used both for VC and external views.
    SetMeshVisibilityMode(AddMesh(exmesh_tpl), MESHVIS_EXTERNAL | MESHVIS_VC);    
    SetMeshVisibilityMode(AddMesh(heatingmesh_tpl), MESHVIS_EXTERNAL); // not visible from VC anyway

#ifdef MMU
    ///////////////////////////////////////////////////////////////////////
    // Init UMmu
    ///////////////////////////////////////////////////////////////////////
    const int ummuStatus = UMmu.InitUmmu(GetHandle());  // returns 1 if ok and other number if not

    // RC4 AND NEWER: UMmu is REQUIRED!
    if (ummuStatus != 1)
        FatalError("UMmu not installed!  You must install Universal Mmu 3.0 or newer in order to use the XR2; visit http://www.alteaaerospace.com for more information.");

    // validate UMmu version and write it to the log file
    const float ummuVersion = CONST_UMMU_XR2(this).GetUserUMmuVersion();
    if (ummuVersion < 3.0)
    {
        char msg[256];
        sprintf(msg, "UMmu version %.2f is installed, but the XR2 requires Universal Mmu 3.0 or higher; visit http://www.alteaaerospace.com for more information.", ummuVersion);
        FatalError(msg);
    }

    char msg[256];
    sprintf(msg, "Using UMmu Version: %.2f", ummuVersion);
    GetXR1Config()->WriteLog(msg);

    // define the docking port coordinates and size
    const float airlockY = static_cast<float>(DOCKING_PORT_COORD.y);
    const float airlockZ = static_cast<float>(DOCKING_PORT_COORD.z);

    // Note: we had to extend the airlock size vertically by (2-GEARCOMPRESSION) meters DOWN to accommodate ingress from the ground.
    const float extraVerticalSize = 2.0f - static_cast<float>(GEAR_COMPRESSION_DISTANCE);
    //                      state,MinX, MaxX, MinY,                                 MaxY,             MinZ,             MaxZ
    UMmu.DefineAirLockShape(1, -0.66f, 0.66f, airlockY - 0.66f - extraVerticalSize, airlockY + 0.66f, airlockZ - 3.0f, airlockZ + 1.2f);

    // Note EVA and ejection coordinates are set inside our PerformEVA method.
    
    m_pActiveAirlockDoorStatus = &olock_status;
    UMmu.SetActiveDockForTransfer(0);       // ship-to-ship transfer enabled
    
    UMmu.SetMaxSeatAvailableInShip(MAX_PASSENGERS); // includes the pilot
    UMmu.SetCrewWeightUpdateShipWeightAutomatically(FALSE);  // we handle crew member weight ourselves
#endif

    // there is only one active airlock, so initialize it now
    m_pActiveAirlockDoorStatus = &olock_status;

    //
    // Initialize and cache all instrument panels
    //
    
    // 1920-pixel-wide panels
    AddInstrumentPanel(new XR2MainInstrumentPanel1920 (*this), 1920);
    AddInstrumentPanel(new XR2UpperInstrumentPanel1920(*this), 1920);
    AddInstrumentPanel(new XR2LowerInstrumentPanel1920(*this), 1920);
    AddInstrumentPanel(new XR2PayloadInstrumentPanel1920(*this), 1920);

    // 1600-pixel-wide panels
    AddInstrumentPanel(new XR2MainInstrumentPanel1600 (*this), 1600);
    AddInstrumentPanel(new XR2UpperInstrumentPanel1600(*this), 1600);
    AddInstrumentPanel(new XR2LowerInstrumentPanel1600(*this), 1600);
    AddInstrumentPanel(new XR2PayloadInstrumentPanel1600(*this), 1600);

    // 1280-pixel-wide panels
    AddInstrumentPanel(new XR2MainInstrumentPanel1280 (*this), 1280);
    AddInstrumentPanel(new XR2UpperInstrumentPanel1280(*this), 1280);
    AddInstrumentPanel(new XR2LowerInstrumentPanel1280(*this), 1280);
    AddInstrumentPanel(new XR2PayloadInstrumentPanel1280(*this), 1280);
    
    // add our VC panels (panel width MUST be zero for these!)
    // TODO: after overhauling the XR1 base class for isVC handling, add a VC panel ID (3rd argument) to each VC panel here
    AddInstrumentPanel(new XR2VCPilotInstrumentPanel     (*this, PANELVC_PILOT), 0);
    AddInstrumentPanel(new XR2VCCopilotInstrumentPanel   (*this, PANELVC_COPILOT), 0);
    AddInstrumentPanel(new XR2VCPassenger1InstrumentPanel(*this, PANELVC_PSNGR1), 0);
    AddInstrumentPanel(new XR2VCPassenger2InstrumentPanel(*this, PANELVC_PSNGR2), 0);
    AddInstrumentPanel(new XR2VCAirlockInstrumentPanel   (*this, PANELVC_AIRLOCK), 0);
    AddInstrumentPanel(new XR2VCPassenger3InstrumentPanel(*this, PANELVC_PSNGR3), 0);
    AddInstrumentPanel(new XR2VCPassenger4InstrumentPanel(*this, PANELVC_PSNGR4), 0);
    AddInstrumentPanel(new XR2VCPassenger5InstrumentPanel(*this, PANELVC_PSNGR5), 0);
    AddInstrumentPanel(new XR2VCPassenger6InstrumentPanel(*this, PANELVC_PSNGR6), 0);
    AddInstrumentPanel(new XR2VCPassenger7InstrumentPanel(*this, PANELVC_PSNGR7), 0);
    AddInstrumentPanel(new XR2VCPassenger8InstrumentPanel(*this, PANELVC_PSNGR8), 0);
    AddInstrumentPanel(new XR2VCPassenger9InstrumentPanel(*this, PANELVC_PSNGR9), 0);
    AddInstrumentPanel(new XR2VCPassenger10InstrumentPanel(*this, PANELVC_PSNGR10), 0);
    AddInstrumentPanel(new XR2VCPassenger11InstrumentPanel(*this, PANELVC_PSNGR11), 0);
    AddInstrumentPanel(new XR2VCPassenger12InstrumentPanel(*this, PANELVC_PSNGR12), 0);

    // NOTE: default crew data is set AFTER the scenario file is parsed
}

// Create control surfaces for any damageable control surface handles below that are zero (all are zero before vessel initialized).
// This is invoked from clbkSetClassCaps as well as ResetDamageStatus.
void XR2Ravenstar::ReinitializeDamageableControlSurfaces()
{
    // We have to cheat a little and move the ailerons out farther to improve roll performance in the atmosphere
    // so that our roll performance is comparable with the XR1.
    // AS MEASURED: const double aileronDeltaX         = 7.782;     // distance from center of ship to center of aileron, X direction
    const double aileronDeltaX         = 7.782 + 2.0; // distance from center of ship to center of aileron, X direction

    if (hElevator == 0)
    {
        // ORG: CreateControlSurface(AIRCTRL_ELEVATOR,     1.2 * XR1Multiplier, 1.4, _V(   0,0, controlSurfacesDeltaZ), AIRCTRL_AXIS_XPOS, anim_elevator);
        hElevator = CreateControlSurface2(AIRCTRL_ELEVATOR, m_baselineElevatorArea, m_elevatorCL, _V(   0,0, m_ctrlSurfacesDeltaZ), AIRCTRL_AXIS_XPOS, anim_elevator);
    }

    if (hLeftAileron == 0)
    {
        hLeftAileron = CreateControlSurface2 (AIRCTRL_AILERON, 0.2 * XR1Multiplier * 1.50, 1.5, _V( aileronDeltaX, 0, m_ctrlSurfacesDeltaZ), AIRCTRL_AXIS_XPOS, anim_raileron);
    }

    if (hRightAileron == 0)
    {
        hRightAileron = CreateControlSurface2 (AIRCTRL_AILERON, 0.2 * XR1Multiplier * 1.50, 1.5, _V(-aileronDeltaX, 0, m_ctrlSurfacesDeltaZ), AIRCTRL_AXIS_XNEG, anim_laileron);
    }
    
    if (hElevatorTrim == 0)
    {
        // Note: XR5 was 0.3 * XR1Multiplier * 7 to help autopilot in the atmosphere.
        hElevatorTrim = CreateControlSurface2 (AIRCTRL_ELEVATORTRIM, 0.3 * XR1Multiplier, 1.5, _V(   0,0, m_ctrlSurfacesDeltaZ), AIRCTRL_AXIS_XPOS, anim_elevatortrim);
    }
}

// --------------------------------------------------------------
// Finalise vessel creation
// --------------------------------------------------------------
void XR2Ravenstar::clbkPostCreation()
{
    // Invoke XR PostCreation code common to all XR vessels (code is in XRVessel.cpp)
    clbkPostCreationCommonXRCode();

    // Initialize XR payload vessel data
    XRPayloadClassData::InitializeXRPayloadClassData();

    ApplyElevatorAreaChanges();   // apply "dual-mode" AF Ctrl elevator settings
    EnableRetroThrusters(rcover_status == DOOR_OPEN);
    EnableHoverEngines(hoverdoor_status == DOOR_OPEN);
    EnableScramEngines(scramdoor_status == DOOR_OPEN);

    // set initial animation states
    SetXRAnimation (anim_gear, gear_proc);
    SetXRAnimation (anim_rcover, rcover_proc);
    SetXRAnimation (anim_hoverdoor, hoverdoor_proc);
    SetXRAnimation (anim_scramdoor, scramdoor_proc);
    SetXRAnimation (anim_nose, nose_proc);
    // NONE: SetXRAnimation (anim_ladder, ladder_proc);
    SetXRAnimation (anim_olock, olock_proc);
    SetXRAnimation (anim_ilock, ilock_proc);
    SetXRAnimation (anim_hatch, hatch_proc);
    SetXRAnimation (anim_radiator, radiator_proc);
    SetXRAnimation (anim_brake, brake_proc);
    SetXRAnimation (anim_bay, bay_proc);
    
    // Note: LOX and Fuel hatch states are not persisted
    
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
    AddPreStep(new NosewheelSteeringPreStep(*this));  // NOTE: REMOVE THIS LINE IF AND WHEN XR2NosewheelSteeringPreStep IS IMPLEMENTED!
    AddPreStep(new UpdateVesselLightsPreStep(*this));
	AddPreStep(new ParkingBrakePreStep     (*this));

    // NO: AddPreStep(new XR2NosewheelSteeringPreStep             (*this)); 
    // not until the Mk II: AddPreStep(new AnimateGearCompressionPrePostStep(*this));
    AddPreStep(new RotateWheelsPreStep     (*this));
    // handled by XRSound now: AddPreStep(new EngineSoundsPreStep     (*this, GetXR1Config()->EnableCustomMainEngineSound, GetXR1Config()->EnableCustomHoverEngineSound, GetXR1Config()->EnableCustomRCSSound));

    // WARNING: this must be invoked LAST in the AddPreStep sequence so that behavior is consistent across all pre-step methods
    AddPreStep(new UpdatePreviousFieldsPreStep(*this));
 
    // add our PostStep objects; these are invoked in order
    AddPostStep(new PreventAutoRefuelPostStep   (*this));   // add this FIRST before our fuel callouts
    AddPostStep(new ComputeAccPostStep          (*this));   // used by acc areas; computed only once per frame for efficiency
    // XRSound: AddPostStep(new AmbientSoundsPostStep       (*this));
    AddPostStep(new ShowWarningPostStep         (*this));
    AddPostStep(new SetHullTempsPostStep        (*this));
    AddPostStep(new SetSlopePostStep            (*this));
    AddPostStep(new FuelCalloutsPostStep        (*this));
    AddPostStep(new UpdateIntervalTimersPostStep(*this));
    AddPostStep(new APUPostStep                 (*this));
    AddPostStep(new UpdateMassPostStep          (*this));
    
    AddPostStep(new SwitchTwoDPanelPostStep     (*this));
    AddPostStep(new AnimationPostStep           (*this));  // Note: this should be BEFORE XR2AnimationPostStep so the gear animates correctly
    AddPostStep(new XR2AnimationPostStep        (*this));
    AddPostStep(new XR2DoorSoundsPostStep       (*this));

    AddPostStep(new OneShotInitializationPostStep(*this));
    AddPostStep(new DisableControlSurfForAPUPostStep    (*this));
    AddPostStep(new FuelDumpPostStep            (*this));
    AddPostStep(new XFeedPostStep               (*this));
    AddPostStep(new ResupplyPostStep            (*this));
    AddPostStep(new LOXConsumptionPostStep      (*this));
    AddPostStep(new UpdateCoolantTempPostStep   (*this));
    AddPostStep(new AirlockDecompressionPostStep(*this));
    AddPostStep(new AutoCenteringSimpleButtonAreasPostStep(*this));  // logic for all auto-centering button areas
    AddPostStep(new ResetAPUTimerForPolledSystemsPostStep(*this));
    AddPostStep(new ManageMWSPostStep           (*this));
    if (GetXR1Config()->EnableBoilOffExhaustEffect)  // user wants boil-off effect?
        AddPostStep(new BoilOffPostStep(*this));

#ifdef _DEBUG
    AddPostStep(new TestXRVesselCtrlPostStep    (*this));      // for manual testing of new XRVesselCtrl methods via the debugger
#endif

    // set hidden elevator trim level
    SetControlSurfaceLevel(AIRCTRL_FLAP, m_hiddenElevatorTrimState);  
}

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
            m_pXRSound->LoadWav(XRSound::RCSThrustHit,     "attitudecontrolhit.wav",     XRSound::BothViewMedium);
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
bool XR2Ravenstar::clbkPlaybackEvent (double simt, double event_t, const char *event_type, const char *event)
{
    // handle our custom events first
    // TODO: write me if necessary

    // let our superclass have it
    return DeltaGliderXR1::clbkPlaybackEvent(simt, event_t, event_type, event);
}

// --------------------------------------------------------------
// Create visual
// --------------------------------------------------------------
void XR2Ravenstar::clbkVisualCreated (VISHANDLE vis, int refcount)
{
    exmesh = GetDevMesh(vis, 0);
    // Note: vcmesh should remain NULL here! It is safer that way, since any vcmesh operations
    // performed by the XR1 base class are XR1-mesh-specific.

    heatingmesh = GetDevMesh(vis, 1);  // hull heating mesh; the single group in this mesh is HIDDEN by default
    
    SetPassengerVisuals();
    SetDamageVisuals();

    HideActiveVCHUDMesh();
    ApplySkin();
    
    const XR2ConfigFileParser *pConfig = GetXR2Config();

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

    if (campos == CAM_VCPILOT)
        pilotHUDVisible = false;
    else if (campos == CAM_VCCOPILOT)
        copilotHUDVisible = false;

    SetMeshGroupVisible(exmesh, PILOT_HUD_MESHGRP, pilotHUDVisible);  
    SetMeshGroupVisible(exmesh, COPILOT_HUD_MESHGRP, copilotHUDVisible);  
}

// --------------------------------------------------------------
// Destroy visual
// --------------------------------------------------------------
void XR2Ravenstar::clbkVisualDestroyed (VISHANDLE vis, int refcount)
{
    exmesh = nullptr;
    heatingmesh = nullptr;

    // Note: vcmesh remains NULL at all times with the XR2
}

// ==============================================================
// End overridden callback functions
// ==============================================================

// Used for internal development testing only to tweak some internal value.
// This is invoked from the key handler as ALT-1 or ALT-2 are held down.  
// direction = true: increment value, false: decrement value
void XR2Ravenstar::TweakInternalValue(bool direction)
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
    const double stepSize = (oapiGetSimStep() * 0.5);  // 2 seconds to go from 0...1
    m_tweakedInternalValue += (direction ? stepSize : -stepSize);
    sprintf(oapiDebugString(), "tweakedInternalValue=%lf", m_tweakedInternalValue);
#endif
#if 0  
    // heating mesh testing #1
    if (heatingmesh)
    {
        const double stepSize = (oapiGetSimStep() * 0.1);  // 10 seconds to go from 0...1
        m_tweakedInternalValue += (direction ? stepSize : -stepSize);
        sprintf(oapiDebugString(), "tweakedInternalValue=%lf", m_tweakedInternalValue); 

        oapiSetMeshProperty(heatingmesh, MESHPROPERTY_MODULATEMATALPHA, 1); // use material alpha w/texture alpha
        MATERIAL *pHeatingMaterial = oapiMeshMaterial(heatingmesh, 0);
        pHeatingMaterial->diffuse.a = static_cast<float>(m_tweakedInternalValue);
        pHeatingMaterial->ambient.a = static_cast<float>(m_tweakedInternalValue);
        pHeatingMaterial->specular.a = static_cast<float>(m_tweakedInternalValue);
        pHeatingMaterial->emissive.a = static_cast<float>(m_tweakedInternalValue);
    }
#endif
#if 0 
    // heating mesh testing #2 (set nosecone temp in tweaked value (must update SetHullTempsPostStep as well))
    const double stepSize = (oapiGetSimStep() * 100);  // 100 degrees per second
    m_tweakedInternalValue += (direction ? stepSize : -stepSize);
    sprintf(oapiDebugString(), "tweakedInternalValue=%lf", m_tweakedInternalValue); 
#endif
#if 0
    // play demo sound
    ShowInfo("rocketmain.wav", "Playing rocket sound!");
#endif
#if 0
    // 2 = enable value, 1 = disable value
    m_tweakedInternalValue = static_cast<double>(direction);
#endif

#endif
}

// --------------------------------------------------------------
// Apply custom skin to the current mesh instance
// --------------------------------------------------------------
void XR2Ravenstar::ApplySkin ()
{
    if (!exmesh) return;

    // NOTE: this core bug is fixed in the new Orbiter version (2009), so this workaround is no longer necessary
    // TODO FOR 1.1: test workaround for CTD after vessel switching
    // THIS WORKS, but I need to figure out how to delete the old texture handle in clbkVisualDestroyed:
    /*
    SURFHANDLE hTexture = oapiLoadTexture("XR2Ravenstar\\Skins\\Red\\top_hull_colour_XR2_paint.dds");
    oapiSetTexture(exmesh, 3, hTexture);
    */

    // TODO: {ZZZ} update these texture indexes with each new mesh version!
    if (skin[0]) oapiSetTexture (exmesh, 3, skin[0]);   // top_hull_colour_XR2_paint.dds
    if (skin[1]) oapiSetTexture (exmesh, 1, skin[1]);   // bottom_hull.dds
}

// Set the camera to its default payload bay postion.
void XR2Ravenstar::ResetCameraToPayloadBay()
{
    /* ORG
    const VECTOR3 pos = { 0.0, 2.168, 3.153 };
    const VECTOR3 dir = { 0.0, -0.461, -0.887 };  // look down to rear bottom of bay
    */
    // pre-1.4: const VECTOR3 pos = { 0.0, 2.168 + 3.0, 3.153 + 2.0};    // above the bay
    const VECTOR3 pos = { 0.0, 2.168 + 3.0, 3.153 + 2.0};    // above the bay
    // BETA-1d: const VECTOR3 dir = { 0.0, -0.461, -0.887 };          
    const VECTOR3 dir = { 0.0, -0.624, -0.781 };             // look down to rear bottom of bay

    SetCameraOffset(pos);
    SetXRCameraDirection(dir); 
}

// handle instant jumps to open or closed here
#define CHECK_DOOR_JUMP(proc, anim) if (action == DOOR_OPEN) proc = 1.0;            \
                                    else if (action == DOOR_CLOSED) proc = 0.0;     \
                                    SetXRAnimation(anim, proc)

// state: 0=fully retracted, 1.0 = fully deployed
void XR2Ravenstar::SetGearParameters(double state)
{
    if (state == 1.0) // fully deployed?
    {
        const double touchdownDeltaX = 4.615;
        const double touchdownY = GEAR_UNCOMPRESSED_YCOORD + GEAR_COMPRESSION_DISTANCE; // gear height fully compressed
        // Orbiter 2010 P1 and earlier: const double touchdownRearZ = REAR_GEAR_ZCOORD;
        const double touchdownRearZ = REAR_GEAR_ZCOORD + 2.5;  // move main gear forward to assist rotation

        SetXRTouchdownPoints(_V(               0, touchdownY, NOSE_GEAR_ZCOORD),  // front
                             _V(-touchdownDeltaX, touchdownY, touchdownRearZ),    // left
                             _V( touchdownDeltaX, touchdownY, touchdownRearZ),    // right
						     WHEEL_FRICTION_COEFF, WHEEL_LATERAL_COEFF, true);
        SetNosewheelSteering(true);  // not really necessary since we have have a prestep constantly checking this
    } 
    else  // not fully deployed (belly landing!)
    {
        const double touchdownDeltaX = 1.701;
        const double touchdownZRear = -3.263;

        SetXRTouchdownPoints(_V(               0, -1.490, 2.847),               // front
                             _V(-touchdownDeltaX, -1.261, touchdownZRear),      // left
                             _V( touchdownDeltaX, -1.0, touchdownZRear),        // right (tilt the ship)
							 3.0, 3.0, false);		// belly landing!
        SetNosewheelSteering(false);  // not really necessary since we have have a prestep constantly checking this
    }

    // update the animation state
    gear_proc = state;
    SetXRAnimation(anim_gear, gear_proc);

     // redraw the gear indicator
    TriggerRedrawArea(AID_GEARINDICATOR);
}

// perform an EVA for the specified crew member
// Returns: true on success, false on error (crew member not present or outer airlock door is closed)
bool XR2Ravenstar::PerformEVA(const int mmuCrewMemberIndex)
{
    // set coordinates depending on whether we are landed or not
    VECTOR3 pos = { 0.0, DOCKING_PORT_COORD.y, DOCKING_PORT_COORD.z + 2.0 }; // this is the position where the Mmu will appear relative to the ship's local coordinates
    VECTOR3 rot = { 0.0, 0.0, 0.0 };  // straight up, facing forward
#ifdef MMU
    UMmu.SetEjectPosRotRelSpeed(pos, rot, _V(0, 4.0, 0));  // jumped UP to bail out @ 4 meters-per-second

    if (IsLanded())
        pos.y -= 1.0;   // so we are just above the ground when we EVA
        
    UMmu.SetMembersPosRotOnEVA(pos, rot);
#endif

    // perform the EVA
    return DeltaGliderXR1::PerformEVA(mmuCrewMemberIndex);  
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

// Now modify the elevator area if elevators are enabled and "dual-mode"
// elevator performance is enabled.
void XR2Ravenstar::ApplyElevatorAreaChanges()
{
    const DWORD mode = GetADCtrlMode();
    const XR2ConfigFileParser *parser = GetXR2Config();
    if (parser->EnableAFCtrlPerformanceModifier)
    {
        double modifier = 1.0;
        if (mode == 1)  // pitch?
            modifier = parser->AFCtrlPerformanceModifier[0];
        else if (mode == 7)  // on?
            modifier = parser->AFCtrlPerformanceModifier[1];

        // recreate (i.e., modify) the elevator control surface area to simulate limited deflection
        if (hElevator)   // not damaged?
        {
            DelControlSurface(hElevator);
            hElevator = CreateControlSurface2(AIRCTRL_ELEVATOR, m_baselineElevatorArea * modifier, m_elevatorCL, _V(0,0, m_ctrlSurfacesDeltaZ), AIRCTRL_AXIS_XPOS, anim_elevator);
        }

        // DEBUG: sprintf(oapiDebugString(), "elevator modifier: %lf", modifier);
    }
}

// override clbkPanelRedrawEvent so we can limit our refresh rates for our custom screens
bool XR2Ravenstar::clbkPanelRedrawEvent(int areaID, int event, SURFHANDLE surf)
{
    const XR2ConfigFileParser *pConfig = GetXR2Config();

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

// {ZZZ} You may need to update this method whenever the mesh is recreated (in case the texture indices changed): do not delete this comment
// meshTextureID = vessel-specific constant that is translated to a texture index specific to our vessel's .msh file.  meshTextureID 
// NOTE: meshTextureID=VCPANEL_TEXTURE_NONE = -1 = "no texture" (i.e., "not applicable"); defined in Area.h.
// hMesh = OUTPUT: will be set to the mesh handle of the mesh associated with meshTextureID.
DWORD XR2Ravenstar::MeshTextureIDToTextureIndex(const int meshTextureID, MESHHANDLE &hMesh)
{
    // sanity check
    _ASSERTE(meshTextureID > VCPANEL_TEXTURE_NONE);

    DWORD retVal = 0;
#if 0   // {YYY} implement this method when the VC is implemented
    // same mesh for all VC textures
    hMesh = vcmesh_tpl;  // assign by reference

    switch (meshTextureID)
    {
    case XR1_VCPANEL_TEXTURE_LEFT:
        retVal = 18;      // was "tex2" in original DG code
        break;

    case XR1_VCPANEL_TEXTURE_CENTER:
        retVal = 16;      // was "tex1" in original DG code
        break;

    case XR1_VCPANEL_TEXTURE_RIGHT:
        retVal = 14;      // was "tex3" in original DG code
        break;

    default:   // should never happen!
        _ASSERTE(false);
        // fall through with retVal 0
        break;
    }

     // validate return values
    _ASSERTE(retVal >= 0);
    _ASSERTE(hMesh != nullptr);
#endif
    return retVal;
}