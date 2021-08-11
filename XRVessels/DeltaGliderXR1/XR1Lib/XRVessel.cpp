// ==============================================================
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// Copyright 2007-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XRVessel.cpp
// Main vessel class for the XR1, which is the base class for other XR-class vessels.
// ==============================================================

// include common definitions
#include "DeltaGliderXR1.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "AreaIDs.h"  
#include "XR1InstrumentPanels.h"
#include "XR1PreSteps.h"
#include "XR1PostSteps.h"
#include "XR1FuelPostSteps.h"
#include "XR1MultiDisplayArea.h"
#include "XR1HUD.h"
#include "XR1AnimationPoststep.h"
#include "XRPayloadBay.h"    // necessary for destructor call
#include "XR1PayloadDialog.h"
#include "XR1ConfigFileParser.h"

// ==============================================================
// Message callback function for control dialog box
// ==============================================================

INT_PTR CALLBACK XR1Ctrl_DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DeltaGliderXR1 *dg = (uMsg == WM_INITDIALOG ? reinterpret_cast<DeltaGliderXR1 *>(lParam) : reinterpret_cast<DeltaGliderXR1 *>(oapiGetDialogContext(hWnd)));
    // pointer to vessel instance was passed as dialog context
    
    switch (uMsg) {
    case WM_INITDIALOG:
        dg->UpdateCtrlDialog (dg, hWnd);
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
        case IDC_NCONE_CLOSE:
            dg->ActivateNoseCone (DOOR_CLOSING);
            return 0;
        case IDC_NCONE_OPEN:
            dg->ActivateNoseCone (DOOR_OPENING);
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
        case IDC_LADDER_RETRACT:
            dg->ActivateLadder (DOOR_CLOSING);
            return 0;
        case IDC_LADDER_EXTEND:
            dg->ActivateLadder (DOOR_OPENING);
            return 0;
        case IDC_HATCH_CLOSE:
            dg->ActivateHatch (DOOR_CLOSING);
            return 0;
        case IDC_HATCH_OPEN:
            dg->ActivateHatch (DOOR_OPENING);
            return 0;
        case IDC_RADIATOR_RETRACT:
            dg->ActivateRadiator (DOOR_CLOSING);
            return 0;
        case IDC_RADIATOR_EXTEND:
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
            //case IDC_DAMAGE:
            //	oapiOpenDialog (g_hDLL, IDD_DAMAGE, Damage_DlgProc, dg);
            
        }
        break;
    }
    return oapiDefDialogProc(hWnd, uMsg, wParam, lParam);
}


// ==============================================================
// Airfoil coefficient functions
// Return lift, moment and zero-lift drag coefficients as a
// function of angle of attack (alpha or beta)
//
// Note: these must be static so name collisions will not occur in subclasses.
// ==============================================================

///#define PROFILE_DRAG 0.015
// NEW to fix "floaty" landings
// XR1 1.4 ORG: #define PROFILE_DRAG 0.030
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

// static data
HWND DeltaGliderXR1::s_hPayloadEditorDialog = 0;

// --------------------------------------------------------------
// Constructor
// --------------------------------------------------------------
// NOTE: fmodel is ignored for the XR1
DeltaGliderXR1::DeltaGliderXR1 (OBJHANDLE hObj, int fmodel, XR1ConfigFileParser *pConfigFileParser) : 
    VESSEL3_EXT(hObj, fmodel),
    m_secondaryHUDMode(3), m_preStepPreviousAirspeed(0), m_preStepPreviousGearFullyUncompressedAltitude(-1), m_airborneTargetTime(0), 
    m_takeoffTime(0), m_touchdownTime(0), m_preStepPreviousVerticalSpeed(0), m_forceWarning(false), m_accScale(NONE), m_maxGaugeAcc(0),
    m_isCrashed(false),
    m_noseconeTemp(0), m_leftWingTemp(0), m_rightWingTemp(0), m_cockpitTemp(0), m_topHullTemp(0),
    m_activeMultiDisplayMode(DEFAULT_MMID), m_activeTempScale(Celsius), m_pMDA(nullptr),
    m_tertiaryHUDOn(true), m_damagedWingBalance(0), m_crashProcessed(false),
    m_infoWarningTextLineGroup(INFO_WARNING_BUFFER_LINES), m_mwsTestActive(false),
    m_nextMDARefresh(0), m_nextSecondaryHUDRefresh(0), m_lastSecondaryHUDMode(0),
    m_metMJDStartingTime(-1), m_interval1ElapsedTime(-1), m_interval2ElapsedTime(-1),
    m_metTimerRunning(false), m_interval1TimerRunning(false), m_interval2TimerRunning(false),
    m_apuFuelQty(APU_FUEL_CAPACITY), m_mainFuelDumpInProgress(false), m_rcsFuelDumpInProgress(false),
    m_scramFuelDumpInProgress(false), m_apuFuelDumpInProgress(false), m_xfeedMode(XF_OFF),
    m_mainExtLinePressure(0), m_scramExtLinePressure(0), m_apuExtLinePressure(0), m_loxExtLinePressure(0),
    m_nominalMainExtLinePressure(0), m_nominalScramExtLinePressure(0), m_nominalApuExtLinePressure(0), m_nominalLoxExtLinePressure(0),
    m_mainSupplyLineStatus(false), m_scramSupplyLineStatus(false), m_apuSupplyLineStatus(false), m_loxSupplyLineStatus(false),
    m_mainFuelFlowSwitch(false), m_scramFuelFlowSwitch(false), m_apuFuelFlowSwitch(false), m_loxFlowSwitch(false),
    m_loxQty(-1), // set for real in clbkSetClassCaps
    m_loxDumpInProgress(false), m_oxygenRemainingTime(0), m_cabinO2Level(NORMAL_O2_LEVEL),
    m_crewState(OK), m_coolantTemp(NOMINAL_COOLANT_TEMP), m_internalSystemsFailure(false),
    m_customAutopilotMode(AP_OFF), m_airspeedHoldEngaged(false), m_setPitchOrAOA(0), m_setBank(0), m_initialAHBankCompleted(false), m_holdAOA(false),
    m_customAutopilotSuspended(false), m_airspeedHoldSuspended(false), m_setDescentRate(0), m_latchedAutoTouchdownMinDescentRate(-3), m_autoLand(false), m_maxShipHoverAcc(0),
    m_dataHUDActive(false), m_setAirspeed(0), m_maxMainAcc(0), m_nextTertiaryHUDRefresh(0), m_nextArtificialHorizonRefresh(0),
    m_crewHatchInterlocksDisabled(false), m_airlockInterlocksDisabled(false), m_isRetroEnabled(false), m_isHoverEnabled(false), m_isScramEnabled(false),
    m_startupMainFuelFrac(0), m_startupRCSFuelFrac(0), m_startupSCRAMFuelFrac(0),  // NOTE: these values must be 0 and not -1!
    m_crewDisplayIndex(0), m_parsedScenarioFile(false), m_mmuCrewDataValid(false), 
    m_hoverBalance(0),
    m_skipNextAFCallout(false), m_skipNextAPUWarning(false),
    m_centerOfLift(NEUTRAL_CENTER_OF_LIFT), m_cogShiftAutoModeActive(false), m_cogShiftCenterModeActive(false),
    m_mainPitchCenteringMode(false), m_mainYawCenteringMode(false), m_mainDivMode(false), m_mainAutoMode(false), m_hoverCenteringMode(false), m_scramCenteringMode(false),
    m_cogForceRecenter(false), m_MWSLit(false), m_wingBalance(0), m_lastActive2DPanelID(-1),
    m_externalCoolingSwitch(false), m_isExternalCoolantFlowing(false), m_selectedTurbopack(0), 
	m_configOverrideBitmask(0), m_backedOutOrbiterCoreAutoRefuelThisFrame(false), m_parkingBrakesEngaged(false),
    // initialize subclass-use-only variables; these are NOT used by the XR1
    m_dummyAttachmentPoint(nullptr), m_pPayloadBay(nullptr),
    m_deployDeltaV(0.2), m_grappleRangeIndex(0), m_selectedSlotLevel(1), m_selectedSlot(0),
    anim_bay(0), bay_status(DOOR_CLOSED), bay_proc(0), m_requestSwitchToTwoDPanelNumber(-1),
    m_animFrontTireRotation(0), m_animRearTireRotation(0),
    heatingmesh_tpl(nullptr), heatingmesh(nullptr),
    m_animNoseGearCompression(0), m_animRearGearCompression(0),
    m_noseGearProc(1.0), m_rearGearProc(1.0),  // Note: must default to gear *fully uncompressed* here because compression may not be implemented
    m_pFuelDumpParticleStreamSpec(nullptr), m_SCRAMTankHidden(false), m_pBoilOffExhaustParticleStreamSpec(nullptr),
    m_pHudNormalFont(nullptr), m_pHudNormalFontSize(0),
    hLeftAileron(0), hRightAileron(0), hElevator(0), hElevatorTrim(0),    // damageable control surfaces
    m_MainFuelFlowedFromBayToMainThisTimestep(0), m_SCRAMFuelFlowedFromBayToMainThisTimestep(0),
    m_mainThrusterLightLevel(0), m_hoverThrusterLightLevel(0), m_pXRSound(nullptr)
{
#ifdef _DEBUG
    m_tweakedInternalValue = 0;  
#endif

    // allocate and zero our spotlight pointer array
    m_pSpotlights = new SpotLight *[SPOTLIGHT_COUNT];
    for (int i=0; i < SPOTLIGHT_COUNT; i++)
        m_pSpotlights[i] = nullptr;

    // zero payload bay variables (unused by us)
    for (int i=0; i < 3; i++)
        m_nextPayloadScreensRefresh[i] = 0;
    *m_grappleTargetVesselName = 0;

    // normal initialization begins here
    SetModuleHandle(g_hDLL);  // plug in our DLL handle for panel and component classes to use

    // remember our config file parser
    // NOTE: do not read properties from this until clbkGetClassCaps is invoked: it is not parsed until then
    m_pConfig = pConfigFileParser;

    // SPECIAL CASE: just track this here so that hover engine gimbaling will work
    for (int i=0; i < 2; i++)
        m_hoverEngineIntegrity[i] = 1.0;    // default to no damage

    // SPECIAL CASE: track RCS damage separately
    for (int i=0; i < sizeof(m_rcsIntegrityArray) / sizeof(double); i++)
        m_rcsIntegrityArray[i] = 1.0;       // default to no damage

    // new vars for the XR1
    *m_lastWarningMessage = 0;
    *m_crashMessage = 0;
    *m_warningWavFilename = 0;
    m_warningWaveSoundType = ST_Other;  // will be set before first use anyway
    *m_lastWavLoaded = 0;
    *m_hudWarningText = 0;

    // always initalize these variables
    m_pXRSoundPath = "XRSound\\Default";        // installed by XRSound

    // these animation handles are not used by the XR1
    anim_fuelhatch = 0;
    anim_loxhatch = 0;

    // create HUD warning font
    m_pHudWarningFont = oapiCreateFont(34, true, "Tahoma", FONT_BOLD);
    m_pHudWarningFontSize = 42;      // includes spacing

    // NOTE: m_pHudNormalFont is created later once we know the dimensions of the video mode

    // create data HUD font
    m_pDataHudFont = oapiCreateFont(22, true, "Tahoma", FONT_BOLD);  // will be freed by the XR1's destructor
    m_pDataHudFontSize = 18;      // includes spacing

    gear_status       = DOOR_CLOSED;
    gear_proc         = 0.0;
    rcover_status     = DOOR_CLOSED;
    rcover_proc       = 0.0;
    nose_status       = DOOR_CLOSED;
    nose_proc         = 0.0;
    scramdoor_status  = DOOR_CLOSED;
    scramdoor_proc    = 0.0;
    hoverdoor_status  = DOOR_CLOSED;
    hoverdoor_proc    = 0.0;
    ladder_status     = DOOR_CLOSED;
    ladder_proc       = 0.0;
    olock_status      = DOOR_CLOSED;
    olock_proc        = 0.0;
    ilock_status      = DOOR_CLOSED;
    ilock_proc        = 0.0;
    chamber_status    = DOOR_CLOSED;  // closed = PRESSURIZED
    chamber_proc = 0.0;
    hatch_status      = DOOR_CLOSED;
    hatch_proc        = 0.0;
    brake_status      = DOOR_CLOSED;
    brake_proc        = 0.0;
    radiator_status   = DOOR_CLOSED;
    radiator_proc     = 0.0;

    // no proc for these; supply hatches are battery powered and "snap" open or closed
    fuelhatch_status = DOOR_CLOSED;
    loxhatch_status  = DOOR_CLOSED;
    externalcooling_status = DOOR_CLOSED;
    
    // NOTE: we treat the APU like a door here since it has spin-up and spin-down states
    // however, there is no proc for it
    apu_status        = DOOR_CLOSED;
    
    exmesh            = nullptr;
    vcmesh            = nullptr;
    vcmesh_tpl        = nullptr;
    ramjet            = nullptr;
    hatch_vent        = nullptr;
    campos            = CAM_GENERIC;

    // no custom skin loaded yet
    *skinpath = 0;  
    for (int i = 0; i < 3; i++)
        skin[i] = 0;

    for (int i = 0; i < 2; i++) {
        scram_max[i] = 0.0;
        scram_intensity[i] = 0.0;
    }

    // damage parameters
    m_MWSActive = false;  
    lwingstatus = rwingstatus = 1.0;
    
    for (int i = 0; i < 4; i++) 
        aileronfail[i] = false;

    // reset warning lights
    for (int i=0; i < WARNING_LIGHT_COUNT; i++)
        m_warningLights[i] = false;
    m_apuWarning = false;
}

// --------------------------------------------------------------
// Destructor
// --------------------------------------------------------------
DeltaGliderXR1::~DeltaGliderXR1 ()
{
    CleanUpAnimations();

    delete GetXR1Config();
    delete ramjet;

    // clean up sketchpad Font objects
    oapiReleaseFont(m_pHudWarningFont);
    oapiReleaseFont(m_pHudNormalFont);
    oapiReleaseFont(m_pDataHudFont);

    // clean up payload bay items in case our subclass used them; these are NOT used by the XR1
    delete m_pPayloadBay;

    // Reset our static payload editor flag to zero so we don't still think the dialog is
    // open after a restart IF the sim was closed with the dialog still open.  Of course, it is
    // possible that this vessel is being destroyed with the dialog open and the sim is still running,
    // but that is relatively unlikely and even if it happens, all that would happen is that toggling
    // the dialog again would send a OPEN command the first time instead of a close.
    s_hPayloadEditorDialog = 0;

    delete m_pFuelDumpParticleStreamSpec;
    delete m_pBoilOffExhaustParticleStreamSpec;
    ClearLightEmitters();
    delete m_pSpotlights;   // deletes the array of pointers, but Orbiter manages the objects themselves (no way for us to delete them)

    // free up custom skin textures, if any
    for (int i=0; i < 3; i++)
	    if (skin[i]) oapiReleaseTexture(skin[i]);

    delete m_pXRSound;
}

// --------------------------------------------------------------
// Set vessel mass excluding propellants
// NOTE: this is invoked automatically each frame by UpdateMassPostStep
// --------------------------------------------------------------
void DeltaGliderXR1::SetEmptyMass()
{
    double emass = EMPTY_MASS;
    
    // Retrieve passenger mass from MMU; we have to manage this ourselves since we have other things
    // that affect ship mass.
    for (int i=0; i < MAX_PASSENGERS; i++)
    {
#ifdef MMU
        const int crewMemberMass = GetCrewWeightBySlotNumber(i);
#else
        const int crewMemberMass = 68;      // 150 lb average
#endif
        if (crewMemberMass >= 0)
            emass += crewMemberMass;
    }

    // add APU fuel
    emass += m_apuFuelQty;

    // add LOX from the INTERNAL TANK ONLY
    emass += m_loxQty;

    // add payload
    emass += GetPayloadMass();
        
    VESSEL2::SetEmptyMass(emass);
}

// Close the fuel hatch and notify subordinate areas to re-render themselves; no warning or info is logged
// playSound = true to play hatch sound
void DeltaGliderXR1::CloseFuelHatch(bool playSound)
{
    fuelhatch_status = DOOR_CLOSED;

    /* DO NOT reset line pressures here: the PostStep will drop them to zero gradually
    // reset line pressures
    m_mainExtLinePressure = 0;
    m_scramExtLinePressure = 0;
    m_apuExtLinePressure = 0;
    */

    // reset 'pressure nominal' LED states 
    m_mainSupplyLineStatus = false;
    m_scramSupplyLineStatus = false;
    m_apuSupplyLineStatus = false;
    

    // reset fuel flow switches
    m_mainFuelFlowSwitch = false;
    m_scramFuelFlowSwitch = false;
    m_apuFuelFlowSwitch = false;

    if (playSound)
        PlaySound(SupplyHatch, ST_Other, SUPPLY_HATCH_VOL);

    // update animation
    SetXRAnimation(anim_fuelhatch, 0);  // closed

    TriggerRedrawArea(AID_FUELHATCHSWITCH);
    TriggerRedrawArea(AID_FUELHATCHLED);

    TriggerRedrawArea(AID_MAINSUPPLYLINE_SWITCH);
    TriggerRedrawArea(AID_MAINSUPPLYLINE_SWITCH_LED);

    TriggerRedrawArea(AID_SCRAMSUPPLYLINE_SWITCH);
    TriggerRedrawArea(AID_SCRAMSUPPLYLINE_SWITCH_LED);

    TriggerRedrawArea(AID_APUSUPPLYLINE_SWITCH);
    TriggerRedrawArea(AID_APUSUPPLYLINE_SWITCH_LED);
}

// Close the lox hatch and notify subordinate areas to re-render themselves; no warning or info is logged
// playSound = true to play hatch thump
void DeltaGliderXR1::CloseLoxHatch(bool playSound)
{
    loxhatch_status = DOOR_CLOSED;
    /* DO NOT reset line pressure here: the PostStep will drop them to zero gradually
    m_loxExtLinePressure = 0;
    */

    m_loxSupplyLineStatus = false;
    m_loxFlowSwitch = false;

    if (playSound)
        PlaySound(SupplyHatch, ST_Other, SUPPLY_HATCH_VOL);

    // update animation
    SetXRAnimation(anim_loxhatch, 0);  // closed; (close always works)

    TriggerRedrawArea(AID_LOXHATCHSWITCH);
    TriggerRedrawArea(AID_LOXHATCHLED);

    TriggerRedrawArea(AID_LOXSUPPLYLINE_SWITCH);
    TriggerRedrawArea(AID_LOXSUPPLYLINE_SWITCH_LED);
}

// Close the external cooling hatch and notify subordinate areas to re-render themselves; no warning or info is logged
// playSound = true to play hatch thump
void DeltaGliderXR1::CloseExternalCoolingHatch(bool playSound)
{
    externalcooling_status = DOOR_CLOSED;

    // reset external coolant switch
    m_externalCoolingSwitch = false;

    if (playSound)
        PlaySound(SupplyHatch, ST_Other, SUPPLY_HATCH_VOL);

    TriggerRedrawArea(AID_EXTERNAL_COOLING_SWITCH);
    TriggerRedrawArea(AID_EXTERNAL_COOLING_LED);
}


// redraw all 2D and 3D navmod buttons
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

    const char *pAction = (on ? "engaged" : "disengaged");

    char temp[60];
    sprintf(temp, "AIRSPEED HOLD autopilot %s.", pAction);
    ShowInfo(NULL, DeltaGliderXR1::ST_None, temp);

    if (on)     // turning autopilot on?
    {
        // if rate == 0, default to HOLD CURRENT airspeed
        if (m_setAirspeed == 0)
            m_setAirspeed = GetAirspeed();

        if (m_setAirspeed < 0)
            m_setAirspeed = 0;

        sprintf(temp, "Hold Airspeed %.1f m/s", m_setAirspeed);
        ShowInfo(NULL, DeltaGliderXR1::ST_None, temp);

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
    if (m_customAutopilotMode == AP_DESCENTHOLD)
        SetCustomAutopilotMode(AP_OFF, true);
    else
        SetCustomAutopilotMode(AP_DESCENTHOLD, true);
}

void DeltaGliderXR1::ToggleAttitudeHold()
{
    if (m_customAutopilotMode == AP_ATTITUDEHOLD)
        SetCustomAutopilotMode(AP_OFF, true);
    else
        SetCustomAutopilotMode(AP_ATTITUDEHOLD, true);
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
            SetAirspeedHold(false, AS_HOLDCURRENT, 0);  
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
    if ((force == false) && (mode == AP_DESCENTHOLD) && (m_isHoverEnabled == false))
    {
        PlaySound(HoverDoorsAreClosed, ST_WarningCallout);
        ShowWarning(NULL, DeltaGliderXR1::ST_None, "WARNING: Hover Doors are closed;&cannot engage DESCENT HOLD."); 
        SetCustomAutopilotMode(AP_OFF, false, false);   // kill any existing autopilot
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
    const char *pAction = (mode == AP_OFF ? "disengaged" : "engaged");
    char temp[50];
    
    // set mode being switched into or out of
    const AUTOPILOT actionMode = ((mode == AP_OFF) ? oldMode : mode);
    switch(actionMode)
    {
    case AP_ATTITUDEHOLD:
        sprintf(temp, "ATTITUDE HOLD autopilot %s.", pAction);
        ShowInfo(NULL, DeltaGliderXR1::ST_None,temp);

        if (mode != AP_OFF)     // autopilot on?
        {
            if (m_holdAOA)
                sprintf(temp, "Hold AOA=%+.1f°, Hold Bank=%+.1f°", m_setPitchOrAOA, m_setBank);
            else
                sprintf(temp, "Hold Pitch=%+.1f°, Hold Bank=%+.1f°", m_setPitchOrAOA, m_setBank);

            ShowInfo(NULL, DeltaGliderXR1::ST_None,temp);
            m_initialAHBankCompleted = false;  // defensive coding: reset just in case
        }
        else  // AP off now
        {
            m_initialAHBankCompleted = false;  // reset
        }
        break;

    case AP_DESCENTHOLD:
        sprintf(temp, "DESCENT HOLD autopilot %s.", pAction);
        ShowInfo(NULL, DeltaGliderXR1::ST_None,temp);

        if (mode != AP_OFF)     // turning autopilot on?
        {
            // if grounded and rate < 0.1, set rate = +0.1 m/s
            if (GroundContact() && (m_setDescentRate < 0.1))
                m_setDescentRate = 0.1;

            sprintf(temp, "Hold Rate=%+f m/s", m_setDescentRate);
            ShowInfo(NULL, DeltaGliderXR1::ST_None,temp);
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
    if (mode == AP_OFF)     
    {
        if (playSound)
            PlaySound(AutopilotOff, ST_Other, AUTOPILOT_VOL);
    }
    else
    {
        // must turn off normal autopilots here so the new one can take effect
        for (int i=0; i <= 7; i++)
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
    if (m_customAutopilotMode == AP_DESCENTHOLD)
        modeNumber = MDMID_DESCENT_HOLD;
    else if (m_customAutopilotMode == AP_ATTITUDEHOLD)
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
    for (int i=0; i < 14; i++)
        SetThrusterMax0(th_rcs[i], (GetRCSThrustMax(i) * m_rcsIntegrityArray[i]));
}

// kill all autopilots, including airspeed hold.  Sound will play automatically.
void DeltaGliderXR1::KillAllAutopilots()
{
    SetCustomAutopilotMode(AP_OFF, true); // turn off custom autopilot
    SetAirspeedHoldMode(false, false);    // turn off AIRSPEED HOLD; do not play sound again

    for (int i=0; i <= 7; i++)
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

    switch(mode)
    {
    case AS_HOLDCURRENT:
        // hold current airspeed
        m_setAirspeed = GetAirspeed(); 
        if (m_setAirspeed < 0)
            m_setAirspeed = 0;

        sound = BeepHigh;
        sprintf(msg, "Airspeed Hold: holding %.1lf m/s.", m_setAirspeed);
        break;

    case AS_RESET:
        m_setAirspeed = 0;
        sound = BeepLow;
        sprintf(msg, "Airspeed Hold: reset to 0 m/s.");
        break;

    case AS_ADJUST:
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
    
    ShowInfo(NULL, DeltaGliderXR1::ST_None, msg);
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

    if (mode != AD_AUTOLAND)
        m_autoLand = false;     // reset

    switch(mode)
    {
    case AD_LEVEL:
        m_setDescentRate = 0;
        sound = BeepLow;
        strcpy(msg, "Descent Hold: reset to HOVER.");
        break;

    case AD_ADJUST:
        m_setDescentRate += delta;
        if (m_setDescentRate > MAX_DESCENT_HOLD_RATE)
            m_setDescentRate = MAX_DESCENT_HOLD_RATE;
        else if (m_setDescentRate < -MAX_DESCENT_HOLD_RATE)
            m_setDescentRate = -MAX_DESCENT_HOLD_RATE;

        sound = ((delta >= 0) ? BeepHigh : BeepLow);
        sprintf(msg, "Descent Hold: set to %+.1f m/s.", m_setDescentRate);
        break;

    case AD_AUTOLAND:
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
    
    ShowInfo(NULL, DeltaGliderXR1::ST_None, msg);
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
    ShowInfo(NULL, DeltaGliderXR1::ST_None, msg);

    sprintf(msg, "Attitude Hold: Bank synced to %+4.1f°", m_setBank);
    ShowInfo(NULL, DeltaGliderXR1::ST_None, msg);
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
    if (m_customAutopilotMode == AP_ATTITUDEHOLD)
    {
        // perform an implicit sync
        SyncAttitudeHold(false, false);  // no sound for this, since we just beeped above; also, do not force PITCH mode
    }
    else  // Attitude Hold autopilot NOT engaged; do not change target values
    {
        char msg[50];
        sprintf(msg, "Attitude Hold: Holding %+4.1f° %s", m_setPitchOrAOA, (m_holdAOA ? "AOA" : "PITCH"));
        ShowInfo(NULL, DeltaGliderXR1::ST_None, msg);
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

    const char *pAxisMessage = nullptr;
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
        ShowInfo(NULL, DeltaGliderXR1::ST_None, msg);
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
void DeltaGliderXR1::LimitAttitudeHoldPitch(double &val, const double limit)
{
    if (val >  limit) 
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
void DeltaGliderXR1::LimitAttitudeHoldBank(const bool increment, double &val, const double limit)
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
    if (m_customAutopilotMode == AP_ATTITUDEHOLD)
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
        else if ((val <  maxInvertedAttitudeHoldNormal) && (val >= 90)) // <120, >=90 : quadrant 3 (8 o'clock)
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
            ShowWarning(NULL, ST_None, "As a flight safety measure&you must disengage Attitude Hold&before setting an inverted bank level.");
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
        else if ((val <  maxInvertedAttitudeHoldNormal) && (val >= 90))  // <120, >=90 : quadrant 3 (8 o'clock)
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
        ShowInfo(NULL, DeltaGliderXR1::ST_None,temp);
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
        ShowInfo(NULL, DeltaGliderXR1::ST_None,temp);
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
        ShowInfo(NULL, DeltaGliderXR1::ST_None,temp);
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
        ShowInfo(NULL, DeltaGliderXR1::ST_None,temp);
    }
}

// --------------------------------------------------------------
// Verify that hydraulic pressure is present
// playWarning: true = show warning if no hydraulic pressure present
// Returns: true if hydraulic pressure OK, false if not
// --------------------------------------------------------------
bool DeltaGliderXR1::CheckHydraulicPressure(bool showWarning, bool playErrorBeep)
{
    bool retVal = true;  // assume APU is running

        if (apu_status != DOOR_OPEN)   // APU not running?
        {
            retVal = false;

            if (showWarning)
            {
                if (m_skipNextAPUWarning == false)
                {
                    // only play error beep if requested
                    if (playErrorBeep)
                        PlayErrorBeep();

                    if (m_apuFuelQty <= 0.0)
                    {
                        ShowWarning("Warning APU Fuel Depleted No Hydraulic Pressure.wav", ST_WarningCallout, "APU fuel tanks depleted:&no hydraulic pressure!");
                    }
                    else    // fuel remaining, but APU is off
                    {
                        ShowWarning("APU Offline.wav", ST_WarningCallout, "WARNING: APU is offline; no hydraulic&pressure.");
                    }
                }
                else    // skip this warning and reset the flag, since we latched it
                {
                    m_skipNextAPUWarning = false;
                }
            }
        }
    
    return retVal;
}

// --------------------------------------------------------------
// Apply custom skin to the current mesh instance
// --------------------------------------------------------------
void DeltaGliderXR1::ApplySkin ()
{
    if (!exmesh) return;

    if (skin[0]) oapiSetTexture (exmesh, 1, skin[0]);
    if (skin[1]) oapiSetTexture (exmesh, 2, skin[1]);
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
                PopupHUDArea *pHud = static_cast<PopupHUDArea *>(GetArea(PANEL_MAIN, AID_SECONDARY_HUD));  // will never be null
                if (pHud->GetState() == PopupHUDArea::On)
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
                PopupHUDArea *pHud = static_cast<PopupHUDArea *>(GetArea(PANEL_MAIN, AID_TERTIARY_HUD));  // will never be null
                if (pHud->GetState() == PopupHUDArea::On)
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
                double *pNextAreaRefresh = nullptr;  // points to it->second in m_nextRedrawAlwaysRefreshMap 
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
                *pNextAreaRefresh = uptime + GetXR1Config()->PanelUpdateInterval;
                break;
            }
        }
        // DEBUG: sprintf(oapiDebugString(), "uptime=%lf, m_nextTertiaryHUDRefresh=%lf", uptime, m_nextTertiaryHUDRefresh);
    }

    // let the superclass dispatch the redraw event
    return VESSEL3_EXT::clbkPanelRedrawEvent(areaID, event, surf);
}

void DeltaGliderXR1::ScramjetThrust()
{
    int i;
    const double eps = 1e-8;
    const double Fnominal = 2.5 * MAX_MAIN_THRUST[GetXR1Config()->MainEngineThrust];
    
    double Fscram[2];
    ramjet->Thrust (Fscram);
    
    for (i = 0; i < 2; i++) 
    {
        double level = GetThrusterLevel (th_scram[i]);
        double Fmax  = Fscram[i]/(level+eps);
        SetThrusterMax0(th_scram[i], Fmax);
        
        // handle new configurable ISP
        const double isp = Fscram[i]/(ramjet->DMF(i)+eps) * GetXR1Config()->GetScramISPMultiplier();
        SetThrusterIsp(th_scram[i], max (1.0, isp)); // don't allow ISP=0
        
        // the following are used for calculating exhaust density
        scram_max[i] = min (Fmax/Fnominal, 1.0);
        scram_intensity[i] = level * scram_max[i];
    }
}

// returns true if MWS reset, false if cannot be reset
bool DeltaGliderXR1::ResetMWS()
{
    if (IsCrewIncapacitatedOrNoPilotOnBoard())  // covers IsCrashed() as well
        return false;     // cannot disable warning if crew incapacitated!

    m_MWSActive = false;    // reset "active" flag
    TriggerRedrawArea(AID_MWS);

    PlaySound(BeepLow, ST_Other);
    ShowInfo("System Reset.wav", ST_InformationCallout, "Master Warning System reset.");

    return true;
}

// handle instant jumps to open or closed here
#define CHECK_DOOR_JUMP(proc, anim) if (action == DOOR_OPEN) proc = 1.0;            \
                                    else if (action == DOOR_CLOSED) proc = 0.0;     \
                                    SetXRAnimation(anim, proc)

void DeltaGliderXR1::ActivateLandingGear (DoorStatus action)
{
    // check for failure
    if (gear_status == DOOR_FAILED)
    {
        PlayErrorBeep();
        ShowWarning("Warning Gear Failure.wav", ST_WarningCallout, "Landing Gear inoperative due to&excessive heat and/or dynamic&pressure.");
        return;  // cannot move
    }

    // We cannot raise or deploy the landing gear if 1) we are already sitting on the ground OR 2) if
    // the gear is up but we are at or below GEAR_FULLY_UNCOMPRESSED_DISTANCE in altitude.
    const double altitude = GetAltitude(ALTMODE_GROUND);
    if ((action == DOOR_OPENING) || (action == DOOR_CLOSING))
    {
        if (GroundContact())   // check #1
        {
            PlayErrorBeep();
            ShowWarning("Gear Locked.wav", ST_WarningCallout, "Ship is landed: cannot raise landing gear.");
            return;
        }
        else if (altitude <= GEAR_FULLY_UNCOMPRESSED_DISTANCE)  // would gear be below the ground?
        {
            if (action == DOOR_CLOSING)
            {
                PlayErrorBeep();
                ShowWarning("Gear Locked.wav", ST_WarningCallout, "Gear in contact with ground:&cannot raise landing gear.");
                return;
            }
            else if (action == DOOR_OPENING)
            {
                PlayErrorBeep();
                ShowWarning("Gear Locked.wav", ST_WarningCallout, "Insufficient altitude to lower&the landing gear.");
                return;
            }
        }
    }
    
    if (CheckHydraulicPressure(true, true) == false)   // show warning if no hydraulic pressure
        return;     // no hydraulic pressure

    bool close = (action == DOOR_CLOSED || action == DOOR_CLOSING);
    gear_status = action;

    CHECK_DOOR_JUMP(gear_proc, anim_gear);

    UpdateVCStatusIndicators();
    SetGearParameters(gear_proc);

    TriggerRedrawArea(AID_GEARSWITCH);
    TriggerRedrawArea(AID_GEARINDICATOR);
    SetXRAnimation(anim_gearlever, close ? 0:1);
    RecordEvent ("GEAR", close ? "UP" : "DOWN");

    // NOTE: sound is handled by GearCalloutsPostStep 
}

void DeltaGliderXR1::ToggleLandingGear ()
{
    ActivateLandingGear ( ((gear_status == DOOR_CLOSED || gear_status == DOOR_CLOSING) ? DOOR_OPENING : DOOR_CLOSING) );
    UpdateCtrlDialog (this);
}

// activate the bay doors 
// NOTE: not used by the XR1; this is here for subclasses only
void DeltaGliderXR1::ActivateBayDoors(DoorStatus action)
{
    // check for failure
    if (bay_status == DOOR_FAILED)
    {
        PlayErrorBeep();
        ShowWarning("Warning Bay Door Failure.wav", ST_WarningCallout, "Bay doors inoperative due to excessive&heat and/or dynamic pressure.");
        return;  // cannot move
    }

    if (CheckHydraulicPressure(true, true) == false)   // show warning if no hydraulic pressure
        return;     // no hydraulic pressure

    /* OK on the Ravenstar
    // cannot deploy or retract bay doors if the radiator is in motion
    // NOTE: allow for DOOR_FAILED here so that a radiator failure does not lock the bay doors
    if ((radiator_status == DOOR_OPENING) || (radiator_status == DOOR_CLOSING))
    {
        PlayErrorBeep();
        ShowWarning("Warning Radiator in Motion Bay Doors Are Locked.wav", "Cannot open/close bay doors while&radiator is in motion.");
        return;  // cannot move
    }
    */

    CHECK_DOOR_JUMP(bay_proc, anim_bay);

    bool close = (action == DOOR_CLOSING) || (action == DOOR_CLOSED);
    bay_status = action;
    TriggerRedrawArea(AID_BAYDOORSSWITCH);
    TriggerRedrawArea(AID_BAYDOORSINDICATOR);
    UpdateCtrlDialog(this);  // Note: CTRL dialog not used for the XR2
    RecordEvent("BAYDOORS", close ? "CLOSE" : "OPEN");
}

// invoked from key handler
// NOTE: not used by the XR1; this is here for subclasses only
void DeltaGliderXR1::ToggleBayDoors()
{
    ActivateBayDoors(bay_status == DOOR_CLOSED || bay_status == DOOR_CLOSING ?
            DOOR_OPENING : DOOR_CLOSING);
}

#ifdef NOT_USED_ANYMORE
// Invoked by all subclasses to set touchdown points; this method is necessary to support Orbiter 2016's new gear compression settings
void DeltaGliderXR1::SetXRTouchdownPoints(const VECTOR3 &pt1, const VECTOR3 &pt2, const VECTOR3 &pt3, const double mu_lng, const double mu_lat, const bool bIsGearDown) const
{
	// scale the gear stiffness by the default DG's so that each vessel has similar compression characteristics when it is fully loaded
	const double stiffness = (FULLY_LOADED_MASS / 26168.0) * (bIsGearDown ? 1e6 : 1e7);
	const double damping = (FULLY_LOADED_MASS / 26168.0) * 1e5;

	// compute the touchdown points for the top of the hull based on the ship's height when landed
	const double hull_radius = HEIGHT_WHEN_LANDED + pt1.y;		// e.g., 4.72 + -2.57 = 2.15 meters from the center of the ship; we assume the ship's centerpoint (0,0,0) bisects the hull in its exact center.
	const double hull_length_half = HULL_LENGTH / 2;
	const double hull_width_half = HULL_WIDTH / 2;
	const VECTOR3 forwardHullTouchdownPoint = { 0, 0, hull_length_half };   // since hull_radius is *positive*, the Y touchdown points for the hull will be positive, too
	const VECTOR3 aftHullTouchdownPoint1 = { -hull_width_half, hull_radius, -hull_length_half };
	const VECTOR3 aftHullTouchdownPoint2 = { hull_width_half, hull_radius, -hull_length_half };
	
	// We also define three touchdown points for the hull so the ship can sit upside-down; we assume 1/3 the bounce and 5x the friction of the corresponding landing gear point.  This won't be precise, but it will do.
	const double hull_stiffness = stiffness / 3;
	const double hull_damping = damping / 3;
	const double hull_mu_lat = mu_lat * 5;
	const double hull_mu_lng = mu_lng * 5;
	const TOUCHDOWNVTX vtxArray[6] = 
	{
		// gear
		{ pt1, stiffness, damping, mu_lat, mu_lng },		// forward landing gear
		{ pt2, stiffness, damping, mu_lat, mu_lng },		// aft landing gear 1
		{ pt3, stiffness, damping, mu_lat, mu_lng },		// aft landing gear 2

		// hull
		{ forwardHullTouchdownPoint, hull_stiffness, hull_damping, hull_mu_lat, hull_mu_lng },
		{ aftHullTouchdownPoint1, hull_stiffness, hull_damping, hull_mu_lat, hull_mu_lng },
		{ aftHullTouchdownPoint2, hull_stiffness, hull_damping, hull_mu_lat, hull_mu_lng }
	};
	SetTouchdownPoints(vtxArray, (sizeof(vtxArray) / sizeof(TOUCHDOWNVTX)));
}
#endif

// Invoked by all subclasses to set touchdown points; this method is necessary to support Orbiter 2016's new gear compression settings
void DeltaGliderXR1::SetXRTouchdownPoints(const VECTOR3 &pt1, const VECTOR3 &pt2, const VECTOR3 &pt3, const double mu_lng, const double mu_lat, const bool bIsGearDown) const
{
	// scale the gear stiffness by the default DG's so that each vessel has similar compression characteristics when it is fully loaded
    
	const double stiffness = (FULLY_LOADED_MASS / 26168.0) * (bIsGearDown ? 1e6 : 1e7);
	const double damping = (FULLY_LOADED_MASS / 26168.0) * 1e5;
    
	// for hull touchdown points, we assume 10x the stiffness and same damping of the corresponding landing gear point (which matches what the default DG does), with a friction coefficient of 1.0.
	const double hull_stiffness = stiffness * 10;
	const double hull_damping = damping;  
	const double hull_mu_lat = 3.0;

	const int vtxArrayElementCount = 3 + HULL_TOUCHDOWN_POINTS_COUNT;   // allow space for our three main touchdown points
	TOUCHDOWNVTX *pVtxArray = new TOUCHDOWNVTX[vtxArrayElementCount];
	pVtxArray[0] = { pt1, stiffness, damping, mu_lat, mu_lng };		    // forward landing gear
    // NOTE: we adjust these friction parameters for the rear the same as the DG does
    pVtxArray[1] = { pt2, stiffness, damping, mu_lat, mu_lng * 2 };		// aft landing gear 1 (left)  
	pVtxArray[2] = { pt3, stiffness, damping, mu_lat, mu_lng * 2 };		// aft landing gear 2 (right)

	// copy over all the hull touchdown points
	for (int i = 0; i < HULL_TOUCHDOWN_POINTS_COUNT; i++)
	{
		pVtxArray[3 + i] = { HULL_TOUCHDOWN_POINTS[i], hull_stiffness, hull_damping, hull_mu_lat };  // lng is not used for hull touchdown points (see Orbiter docs)
	}
	SetTouchdownPoints(pVtxArray, vtxArrayElementCount);
	delete pVtxArray;
}


// state: 0=fully retracted, 1.0 = fully deployed (this method is overridden by subclasses)
void DeltaGliderXR1::SetGearParameters(double state)
{
    if (state == 1.0) // fully deployed?
    {
        const double mainGearAdjustment = +2.0;  // move main gear forward to assist rotation
		SetXRTouchdownPoints(_V(0, -2.57, 10), _V(-3.5, -2.57, -3 + mainGearAdjustment), _V(3.5, -2.57, -3 + mainGearAdjustment), WHEEL_FRICTION_COEFF, WHEEL_LATERAL_COEFF, true);  // cheat and move touchdown points forward so the ship can rotate
        SetNosewheelSteering(true);  // not really necessary since we have have a prestep constantly checking this
    } 
    else  // not fully deployed
    {
        SetXRTouchdownPoints(_V(0,-1.5,9), _V(-6,-0.8,-5), _V(3,-1.2,-5), 3.0, 3.0, false);   // tilt the ship -- belly landing!
        SetNosewheelSteering(false);  // not really necessary since we have a prestep constantly checking this
    }

    // update the animation state
    gear_proc = state;
    SetXRAnimation(anim_gear, gear_proc);

    // redraw the gear indicator
    TriggerRedrawArea(AID_GEARINDICATOR);
}

void DeltaGliderXR1::ActivateHoverDoors(DoorStatus action)
{
    // NOTE: Hover doors (presently) cannot fail, so don't check for that here

    if (CheckHydraulicPressure(true, true) == false)   // show warning if no hydraulic pressure
        return;     // no hydraulic pressure

    bool close = (action == DOOR_CLOSED || action == DOOR_CLOSING);
    hoverdoor_status = action;

    CHECK_DOOR_JUMP(hoverdoor_proc, anim_hoverdoor);

    // No VC lights for these doors: UpdateVCStatusIndicators();

    EnableHoverEngines(action == DOOR_OPEN);
    TriggerRedrawArea(AID_HOVERDOORSWITCH);
    TriggerRedrawArea(AID_HOVERDOORINDICATOR);
    // no VC switch for this
    UpdateCtrlDialog (this);
    RecordEvent("HOVERDOORS", close ? "CLOSE" : "OPEN");
}

void DeltaGliderXR1::ActivateScramDoors(DoorStatus action)
{
    // NOTE: SCRAM doors (presently) cannot fail, so don't check for that here

    if (CheckHydraulicPressure(true, true) == false)   // show warning if no hydraulic pressure
        return;     // no hydraulic pressure

    bool close = (action == DOOR_CLOSED || action == DOOR_CLOSING);
    scramdoor_status = action;

    CHECK_DOOR_JUMP(scramdoor_proc, anim_scramdoor);

    // No VC lights for these doors: UpdateVCStatusIndicators();

    EnableScramEngines(action == DOOR_OPEN);
    TriggerRedrawArea(AID_SCRAMDOORSWITCH);
    TriggerRedrawArea(AID_SCRAMDOORINDICATOR);
    // no VC switch for this
    UpdateCtrlDialog(this);
    RecordEvent("SCRAMDOORS", close ? "CLOSE" : "OPEN");
}

void DeltaGliderXR1::ActivateRCover(DoorStatus action)
{
    // check for failure
    if (rcover_status == DOOR_FAILED)
    {
        PlayErrorBeep();
        ShowWarning("Warning Retro Door Failure.wav", ST_WarningCallout, "Retro Doors inoperative due to excessive&heat and/or dynamic pressure.");
        return;  // cannot move; NOTE: this will also disable the indicator lights, which is exactly what we want!
    }

    if (CheckHydraulicPressure(true, true) == false)   // show warning if no hydraulic pressure
        return;     // no hydraulic pressure

    bool close = (action == DOOR_CLOSED || action == DOOR_CLOSING);
    rcover_status = action;

    CHECK_DOOR_JUMP(rcover_proc, anim_rcover);
    /* {DEB} causes door to "jump"
    if (action <= DOOR_OPEN) {
        rcover_proc = (action == DOOR_CLOSED ? 0.0 : 1.0);
        SetXRAnimation (anim_rcover, rcover_proc);
    }
    */
    UpdateVCStatusIndicators();
    EnableRetroThrusters (action == DOOR_OPEN);
    TriggerRedrawArea(AID_RETRODOORSWITCH);
    TriggerRedrawArea(AID_RETRODOORINDICATOR);
    SetXRAnimation(anim_retroswitch, close ? 0:1);
    UpdateCtrlDialog (this);
    RecordEvent ("RCOVER", close ? "CLOSE" : "OPEN");
}

void DeltaGliderXR1::ActivateNoseCone(DoorStatus action)
{
    // check for failure
    if (nose_status == DOOR_FAILED)
    {
        PlayErrorBeep();
        char msg[128];
        sprintf(msg, "%s inoperative due to excessive&heat and/or dynamic pressure.", NOSECONE_LABEL);
        ShowWarning("Warning Nosecone Failure.wav", ST_WarningCallout, msg);
        return;  // cannot move
    }

    // if docked, cannot close nosecone
    if (IsDocked() && ((action == DOOR_CLOSING) || (action == DOOR_CLOSED)))
    {
        PlayErrorBeep();
        char msg[128];
        sprintf(msg, "Cannot close %s while&ship is docked!", NOSECONE_LABEL);
        ShowWarning("Warning Ship is Docked.wav", ST_WarningCallout, msg);
        return;
    }

    if (CheckHydraulicPressure(true, true) == false)   // show warning if no hydraulic pressure
        return;     // no hydraulic pressure

    // if outer airlock door is open and nosecone is CLOSING, close the outer airlock door as well
    if (((olock_status == DOOR_OPEN) || (olock_status == DOOR_OPENING)) &&
        ((action == DOOR_CLOSING) || (action == DOOR_CLOSED)))
    {
        // close the outer airlock door since it is OPEN or OPENING!
        ActivateOuterAirlock(DOOR_CLOSING);
    }

    bool close = (action == DOOR_CLOSED || action == DOOR_CLOSING);
    nose_status = action;
    
    CHECK_DOOR_JUMP(nose_proc, anim_nose);
    /* {DEB} causes door to "jump"
    if (action <= DOOR_OPEN) {
        nose_proc = (action == DOOR_CLOSED ? 0.0 : 1.0);
        SetXRAnimation (anim_nose, nose_proc);
        UpdateVCStatusIndicators();
    }
    */
    UpdateVCStatusIndicators();
    TriggerRedrawArea(AID_NOSECONESWITCH);
    TriggerRedrawArea(AID_NOSECONEINDICATOR);
    SetXRAnimation(anim_nconelever, close ? 0:1);
    
    if (close && ladder_status != DOOR_CLOSED)
        ActivateLadder (action); // retract ladder before closing the nose cone
    
    UpdateCtrlDialog (this);
    RecordEvent ("NOSECONE", close ? "CLOSE" : "OPEN");
}

// invoked from key handler
void DeltaGliderXR1::ToggleRCover()
{
    ActivateRCover(rcover_status == DOOR_CLOSED || rcover_status == DOOR_CLOSING ?
            DOOR_OPENING : DOOR_CLOSING);
}

// invoked from key handler
void DeltaGliderXR1::ToggleHoverDoors()
{
    ActivateHoverDoors(hoverdoor_status == DOOR_CLOSED || hoverdoor_status == DOOR_CLOSING ?
            DOOR_OPENING : DOOR_CLOSING);
}

// invoked from key handler
void DeltaGliderXR1::ToggleScramDoors()
{
    ActivateScramDoors(scramdoor_status == DOOR_CLOSED || scramdoor_status == DOOR_CLOSING ?
            DOOR_OPENING : DOOR_CLOSING);
}

// invoked from key handler
void DeltaGliderXR1::ToggleNoseCone ()
{
    ActivateNoseCone(nose_status == DOOR_CLOSED || nose_status == DOOR_CLOSING ?
            DOOR_OPENING : DOOR_CLOSING);
}

void DeltaGliderXR1::ActivateHatch(DoorStatus action)
{
    // check for failure
    if (hatch_status == DOOR_FAILED)
    {
        PlayErrorBeep();
        ShowWarning("Warning Hatch Failure.wav", ST_WarningCallout, "Top Hatch inoperative due to excessive&heat and/or dynamic pressure.");
        return;  // cannot move
    }

    // check for ATM pressure outside
    bool opening = ((action == DOOR_OPENING) || (action == DOOR_OPEN));
    if ((InEarthAtm() == false) && opening)
    {
        // check whether safety interlocks are still engaged
        if (m_crewHatchInterlocksDisabled == false)
        {
            PlayErrorBeep();
            ShowWarning("Warning Decompression Danger Hatch is Locked.wav", ST_WarningCallout, "WARNING: Crew Hatch LOCKED");
            return;
        }
    }

    if (CheckHydraulicPressure(true, true) == false)   // show warning if no hydraulic pressure
        return;     // no hydraulic pressure

    // NOTE: cabin decompression is handled by LOXConsumptionPostStep

    ForceActivateCabinHatch(action);
}

// Render hatch decompression exhaust stream
void DeltaGliderXR1::ShowHatchDecompression()
{
    static PARTICLESTREAMSPEC airvent = {
        0, 1.0, 15, 0.5, 0.3, 2, 0.3, 1.0, PARTICLESTREAMSPEC::EMISSIVE,
        PARTICLESTREAMSPEC::LVL_LIN, 0.1, 0.1,
        PARTICLESTREAMSPEC::ATM_FLAT, 0.1, 0.1
    };
    static VECTOR3 pos = {0,2,4};
    static VECTOR3 dir = {0,1,0};
    hatch_vent = new PSTREAM_HANDLE[1];   // this will be freed automatically for us later
    hatch_venting_lvl = new double[1];    // ditto
    hatch_venting_lvl[0] = 0.4;
    hatch_vent[0] = AddParticleStream (&airvent, pos, dir, hatch_venting_lvl);
    hatch_vent_t = GetAbsoluteSimTime();
}

// turn off hatch decompression exhaust stream; invoked form a PostStep
void DeltaGliderXR1::CleanUpHatchDecompression()
{
    DelExhaustStream(hatch_vent[0]);
}

// force the cabin hatch and don't do any checks
void DeltaGliderXR1::ForceActivateCabinHatch(DoorStatus action)
{
    hatch_status = action;
    UpdateVCStatusIndicators();

    CHECK_DOOR_JUMP(hatch_proc, anim_hatch);
    /* {DEB} causes door to "jump"
    if (action <= DOOR_OPEN) {
        hatch_proc = (action == DOOR_CLOSED ? 0.0 : 1.0);
        SetXRAnimation (anim_hatch, hatch_proc);
    }
    */
    UpdateVCStatusIndicators();
    TriggerRedrawArea(AID_HATCHSWITCH);
    TriggerRedrawArea(AID_HATCHINDICATOR);

    const bool close = (action == DOOR_CLOSED || action == DOOR_CLOSING);
    SetXRAnimation(anim_hatchswitch, close ? 0:1);
    UpdateCtrlDialog (this);
    RecordEvent ("HATCH", close ? "CLOSE" : "OPEN");
}

// decompress the cabin and kill the crew if necessary
void DeltaGliderXR1::DecompressCabin()
{
    // kill the crew if still alive and anyone on board
    char temp[60];
#ifdef MMU
    if ((m_crewState != DEAD) && (GetCrewMembersCount() > 0))
    {
        sprintf(temp, "DECOMPRESSION! CREW IS DEAD!!");
        KillCrew();  
    }
    else    // crew either dead or no one on board
#endif
    {
        sprintf(temp, "DECOMPRESSION!");
    }

    ShowWarning(NULL, DeltaGliderXR1::ST_None, temp);
    strcpy(m_crashMessage, temp);   // show on HUD
    PlaySound(Crash, ST_Other);
    m_cabinO2Level = 0;   // no atm in cabin now
    m_MWSActive = true;
}

// undock the ship intelligently
void DeltaGliderXR1::PerformUndocking()
{
    if (IsDocked() == false)
    {
        PlayErrorBeep();
        ShowWarning(NULL, DeltaGliderXR1::ST_None, "Ship is not docked.");
        return;
    }

    // safety check: prevent undocking if both airlock doors are open
    if ((olock_status != DOOR_CLOSED) && (ilock_status != DOOR_CLOSED))
    {
        PlayErrorBeep();
        ShowWarning("Warning Decompression Danger.wav", DeltaGliderXR1::ST_WarningCallout, "WARNING: DECOMPRESSION DANGER:&Both airlock doors open!");
        return;
    }

    Undock(0);

    // if ship is docked, set airlock pressure to EXTERNAL PRESSURE if outer door is not closed
    if (olock_status != DOOR_CLOSED)
    {
        DoorStatus newChamberStatus = (InEarthAtm() ? DOOR_CLOSED : DOOR_OPEN);
        ActivateChamber(newChamberStatus, true);  // instantly force pressure or vacuum
    }
}


// kill the crew and remove any passengers
// Returns: # of crew members on board who are now dead
int DeltaGliderXR1::KillCrew()
{
    int crewMembersKilled = 0;

    m_crewState = DEAD;   // do this even if nobody on board so that the controls will be disabled.
#ifdef MMU
    // remove all the crew members
    for (int i=0; i < MAX_PASSENGERS; i++)
    {
        // UMMu bug: cannot delcare pName 'const' here
        char *pName = GetCrewNameBySlotNumber(i);
        if (strlen(pName) > 0)  // is crew member on board?
        {
            UMmu.RemoveCrewMember(pName);   // he's dead now
            crewMembersKilled++;
        }
    }

    TriggerRedrawArea(AID_CREW_DISPLAY);   // update the crew display since they're all dead now...
    SetPassengerVisuals();     // update the VC mesh

    return crewMembersKilled;
#else
    return MAX_PASSENGERS;
#endif
}

void DeltaGliderXR1::ToggleHatch ()
{
    ActivateHatch (hatch_status == DOOR_CLOSED || hatch_status == DOOR_CLOSING ?
            DOOR_OPENING : DOOR_CLOSING);
}

void DeltaGliderXR1::ActivateLadder(DoorStatus action)
{
    // Note: this is never called by subclasses that do not have a nosecone, so there is no need to use NOSECONE_LABEL here.
    bool close = (action == DOOR_CLOSED || action == DOOR_CLOSING);
    
    // don't extend ladder if nose cone is closed
    if (!close && nose_status != DOOR_OPEN) 
    {
        PlayErrorBeep();
        ShowWarning("Warning Nosecone is Closed.wav", ST_WarningCallout, "Cannot deploy ladder while&nosecone is closed!");
        return;
    }

    // if docked, cannot deploy ladder
    if (IsDocked() && ((action == DOOR_OPENING) || (action == DOOR_OPEN)))
    {
        PlayErrorBeep();
        ShowWarning("Warning Ship is Docked.wav", ST_WarningCallout, "Cannot deploy ladder while&ship is docked!");
        return;
    }

    if (CheckHydraulicPressure(true, true) == false)   // show warning if no hydraulic pressure
        return;     // no hydraulic pressure
    
    ladder_status = action;

    CHECK_DOOR_JUMP(ladder_proc, anim_ladder);
    /* {DEB} causes door to "jump"
    if (action <= DOOR_OPEN) {
        ladder_proc = (action == DOOR_CLOSED ? 0.0 : 1.0);
        SetXRAnimation (anim_ladder, ladder_proc);
    }
    */
    TriggerRedrawArea(AID_LADDERSWITCH);    
    TriggerRedrawArea(AID_LADDERINDICATOR);

    SetXRAnimation(anim_ladderswitch, close ? 0:1);
    UpdateCtrlDialog (this);
    RecordEvent ("LADDER", close ? "CLOSE" : "OPEN");
}

// Not currently used, but keep it anyway
void DeltaGliderXR1::ToggleLadder ()
{
    ActivateLadder (ladder_status == DOOR_CLOSED || ladder_status == DOOR_CLOSING ?
            DOOR_OPENING : DOOR_CLOSING);
}

void DeltaGliderXR1::ActivateOuterAirlock (DoorStatus action)
{
    // door presently cannot fail, so don't bother to check for it here

    // if the nosecone is not open, cannot open outer airlock door;
    // HOWEVER, we can CLOSE it.
    if (((action != DOOR_CLOSING) && (action != DOOR_CLOSED)) && (nose_status != DOOR_OPEN))
    {
        PlayErrorBeep();
        char msg[128];
        sprintf(msg, "WARNING: %s is closed;&cannot open outer door", NOSECONE_LABEL);
        ShowWarning(WARNING_OUTER_DOOR_IS_LOCKED_WAV, ST_WarningCallout, msg);
        return;
    }

    // verify that pressure changes are not in progress
    if ((chamber_status != DOOR_CLOSED) && (chamber_status != DOOR_OPEN))
    {
        PlayErrorBeep();
        ShowWarning(((chamber_status == DOOR_CLOSING) ? 
            "WARNING Chamber Pressurizing Outer Door is Locked.wav" : 
            "WARNING Chamber Depressurizing Outer Door is Locked.wav"), ST_WarningCallout, 
            "WARNING: Airlock chamber pressure is&in flux; outer door is LOCKED.");
        return;
    }

    // check whether ATM pressure outside matches chamber pressure 
    // NOTE: always allow door to be CLOSED; this can be an issue if we DOCK with vacuum in the chamber and outer doors open
    if ((action != DOOR_CLOSING) && (action != DOOR_CLOSED))
    {
        if (chamber_status == DOOR_OPEN)      // vacuum in chamber?
        {
            if ((InEarthAtm() || IsDocked()) && (m_airlockInterlocksDisabled == false))
            {
                PlayErrorBeep();
                ShowWarning("Warning External Pressure Higher than Chamber Pressure.wav", ST_WarningCallout, 
                    "WARNING: External pressure is higher&than chamber pressure;&outer door is LOCKED.");
                return;
            }
        }
        else if (chamber_status == DOOR_CLOSED)    // ATM in chamber?
        {
            if (((InEarthAtm() == false) && (IsDocked() == false)) && (m_airlockInterlocksDisabled == false))
            {
                PlayErrorBeep();
                ShowWarning("Warning Decompression Danger Outer Door is Locked.wav", ST_WarningCallout, 
                    "WARNING: Chamber pressure exceeds&external pressure;&outer door is LOCKED.");
                return;
            }
        }
    }

    if (CheckHydraulicPressure(true, true) == false)   // show warning if no hydraulic pressure
        return;     // no hydraulic pressure

    bool close = (action == DOOR_CLOSED || action == DOOR_CLOSING);
    olock_status = action;

    CHECK_DOOR_JUMP(olock_proc, anim_olock);
    /* {DEB} causes door to "jump"
    if (action <= DOOR_OPEN) 
    {
        olock_proc = (action == DOOR_CLOSED ? 0.0 : 1.0);
        SetXRAnimation (anim_olock, olock_proc);
    }
    */

    // If door opening and atm in chamber, it means that the interlocks were disabled and the door
    // is opening: chamber pressure now matches external pressure!
    if (action == DOOR_OPENING)
    {
        if (InEarthAtm() || IsDocked())
            ActivateChamber(DOOR_CLOSED, true);   // force this
        else    // vacuum (or close enough to it)
            ActivateChamber(DOOR_OPEN, true);     // force this

        TriggerRedrawArea(AID_CHAMBERSWITCH);
        TriggerRedrawArea(AID_CHAMBERINDICATOR);        
    }

    UpdateVCStatusIndicators();
    TriggerRedrawArea(AID_OUTERDOORSWITCH);
    TriggerRedrawArea(AID_OUTERDOORINDICATOR);
    SetXRAnimation(anim_olockswitch, close ? 0:1);
    UpdateCtrlDialog (this);
    RecordEvent ("OLOCK", close ? "CLOSE" : "OPEN");
}

void DeltaGliderXR1::ToggleOuterAirlock ()
{
    ActivateOuterAirlock (olock_status == DOOR_CLOSED || olock_status == DOOR_CLOSING ?
            DOOR_OPENING : DOOR_CLOSING);
}

void DeltaGliderXR1::ActivateInnerAirlock(DoorStatus action)
{
    // inner door presently cannot fail, so don't bother to check for it here

    // verify that chamber is pressurized before opening it; always allow it to CLOSE, however!
    // NOTE: allow airlock interlock override to affect the INNER airlock door, too
    if (((action == DOOR_OPEN) || (action == DOOR_OPENING)) &&
        (chamber_status != DOOR_CLOSED) && (m_airlockInterlocksDisabled == false))    // chamber not fully pressurized?
    {
        PlayErrorBeep();
        ShowWarning("Warning Chamber Not Pressurized.wav", ST_WarningCallout, 
            "WARNING: Airlock chamber is&unpressurized; inner door is LOCKED.");
        return;
    }

    if (CheckHydraulicPressure(true, true) == false)   // show warning if no hydraulic pressure
        return;     // no hydraulic pressure

    ForceActivateInnerAirlock(action);
}

// force the inner airlock and don't do any checks
void DeltaGliderXR1::ForceActivateInnerAirlock(DoorStatus action)
{
    bool close = (action == DOOR_CLOSED || action == DOOR_CLOSING);
    ilock_status = action;

    CHECK_DOOR_JUMP(ilock_proc, anim_ilock);
    /* {DEB} causes door to "jump"
    if (action <= DOOR_OPEN) {
        ilock_proc = (action == DOOR_CLOSED ? 0.0 : 1.0);
        SetXRAnimation (anim_ilock, ilock_proc);
    }
    */

    // If door opening and chamber in vacuum, it means that the interlocks were disabled and the door
    // is opening: chamber is now fully pressurized!
    if (action == DOOR_OPENING)
    {
        ActivateChamber(DOOR_CLOSED, true);   // air in chamber (force this)
        TriggerRedrawArea(AID_CHAMBERSWITCH);
        TriggerRedrawArea(AID_CHAMBERINDICATOR);        
    }

    UpdateVCStatusIndicators();
    TriggerRedrawArea(AID_INNERDOORSWITCH);
    TriggerRedrawArea(AID_INNERDOORINDICATOR);
    SetXRAnimation(anim_ilockswitch, close ? 0:1);
    UpdateCtrlDialog (this);
    RecordEvent ("ILOCK", close ? "CLOSE" : "OPEN");
}

void DeltaGliderXR1::ToggleInnerAirlock ()
{
    ActivateInnerAirlock (ilock_status == DOOR_CLOSED || ilock_status == DOOR_CLOSING ?
            DOOR_OPENING : DOOR_CLOSING);
}

// pressurize or depressurize the airlock chamber
// door CLOSED = PRESSURIZED
// door OPEN   = VACUUM
// force: true to skip checks and just do it
void DeltaGliderXR1::ActivateChamber(DoorStatus action, bool force)
{
    if (force == false)
    {
        // verify that the chamber can change states; i.e., both doors are CLOSED
        if (ilock_status != DOOR_CLOSED)
        {
            ShowWarning("Inner Door is Open.wav", ST_WarningCallout, "Inner airlock door is open.");
            return;
        }

        if (olock_status != DOOR_CLOSED)
        {
            ShowWarning("Outer Door is Open.wav", ST_WarningCallout, "Outer airlock door is open.");
            return;
        }

        // chamber presently cannot fail, so don't bother to check for it here
    }

    bool close = (action == DOOR_CLOSED || action == DOOR_CLOSING);
    chamber_status = action;
    if (action == DOOR_CLOSED)
        chamber_proc = 0.0;
    else if (action == DOOR_OPEN)
        chamber_proc = 1.0;
    
    // no VC status indicator for this
    TriggerRedrawArea(AID_CHAMBERSWITCH);
    TriggerRedrawArea(AID_CHAMBERINDICATOR);
    // TODO: ANIMATE VC SWITCH (need mesh change from Donamy): SetXRAnimation(anim_chamberswitch, close ? 0:1);
    UpdateCtrlDialog(this);
    RecordEvent("CHAMBER", close ? "CLOSE" : "OPEN");
}

void DeltaGliderXR1::ActivateAirbrake(DoorStatus action)
{
    if (brake_status == DOOR_FAILED)
    {
        PlayErrorBeep();
        // TODO: if a new speech pack is created, create a "Warning: airbrake failure" callout.
        // no speech callout for this since none was scripted
        ShowWarning(NULL, ST_None, "Airbrake inoperative due to aileron&failure.");
        return;  // cannot move
    }

    if (CheckHydraulicPressure(true, true) == false)   // show warning if no hydraulic pressure
        return;     // no hydraulic pressure

    brake_status = action;
    RecordEvent ("AIRBRAKE", action == DOOR_CLOSING ? "CLOSE" : "OPEN");

    CHECK_DOOR_JUMP(brake_proc, anim_brake);
    TriggerRedrawArea(AID_AIRBRAKESWITCH);
    TriggerRedrawArea(AID_AIRBRAKEINDICATOR);
}

void DeltaGliderXR1::ToggleAirbrake (void)
{
    ActivateAirbrake (brake_status == DOOR_CLOSED || brake_status == DOOR_CLOSING ?
            DOOR_OPENING : DOOR_CLOSING);
}

void DeltaGliderXR1::ActivateRadiator(DoorStatus action)
{
    // check for failure
    if (radiator_status == DOOR_FAILED)
    {
        PlayErrorBeep();
        ShowWarning("Warning Radiator Failure.wav", ST_WarningCallout, "Radiator inoperative due to excessive&heat and/or dynamic pressure.");
        return;  // cannot move
    }

    if (CheckHydraulicPressure(true, true) == false)   // show warning if no hydraulic pressure
        return;     // no hydraulic pressure

    bool close = (action == DOOR_CLOSED || action == DOOR_CLOSING);
    radiator_status = action;

    CHECK_DOOR_JUMP(radiator_proc, anim_radiator);

    /* {DEB} causes door to "jump"
    if (action <= DOOR_OPEN) {
        radiator_proc = (action == DOOR_CLOSED ? 0.0 : 1.0);
        SetXRAnimation (anim_radiator, radiator_proc);
        UpdateVCStatusIndicators();
    }
    */
    UpdateVCStatusIndicators();
    TriggerRedrawArea(AID_RADIATORSWITCH);
    TriggerRedrawArea(AID_RADIATORINDICATOR);
    SetXRAnimation(anim_radiatorswitch, close ? 0:1);
    UpdateCtrlDialog (this);
    RecordEvent ("RADIATOR", close ? "CLOSE" : "OPEN");
}

void DeltaGliderXR1::ToggleRadiator (void)
{
    ActivateRadiator(radiator_status == DOOR_CLOSED || radiator_status == DOOR_CLOSING ?
        DOOR_OPENING : DOOR_CLOSING);
}

void DeltaGliderXR1::SetNavlight (bool on)
{
    // set the beacons
    beacon[0].active = beacon[1].active = beacon[2].active = on;

    // set all the spotlights as well
    for (int i=0; i < SPOTLIGHT_COUNT; i++)
        m_pSpotlights[i]->Activate(on);

    TriggerRedrawArea(AID_NAVLIGHTSWITCH);
    TriggerRedrawArea(AID_SWITCHLED_NAV);
    UpdateCtrlDialog (this);
    RecordEvent ("NAVLIGHT", on ? "ON" : "OFF");
}

void DeltaGliderXR1::SetBeacon (bool on)
{
    beacon[3].active = beacon[4].active = on;
    TriggerRedrawArea(AID_BEACONSWITCH);
    TriggerRedrawArea(AID_SWITCHLED_BEACON);  // repaint the new indicator as well
    UpdateCtrlDialog (this);
    RecordEvent ("BEACONLIGHT", on ? "ON" : "OFF");
}

void DeltaGliderXR1::SetStrobe (bool on)
{
    beacon[5].active = beacon[6].active = on;
    TriggerRedrawArea(AID_STROBESWITCH);
    TriggerRedrawArea(AID_SWITCHLED_STROBE);  // repaint the new indicator as well
    UpdateCtrlDialog (this);
    RecordEvent ("STROBELIGHT", on ? "ON" : "OFF");
}

void DeltaGliderXR1::EnableRetroThrusters (bool state)
{
    for (int i = 0; i < 2; i++)
        SetThrusterResource(th_retro[i], (state ? ph_main : NULL));

    // set flag denoting retro status so we can beep if necessary
    m_isRetroEnabled = state;
}

void DeltaGliderXR1::EnableHoverEngines(bool state)
{
    for(int i=0; i < 2; i++)
        SetThrusterResource(th_hover[i], (state ? ph_main : NULL));

    // set flag denoting hover status so we can beep if necessary
    m_isHoverEnabled = state;
}

void DeltaGliderXR1::EnableScramEngines(bool state)
{
    for (int i=0; i < 2; i++)
        SetThrusterResource(th_scram[i], (state ? ph_scram : NULL));

    // set flag denoting hover status so we can beep if necessary
    m_isScramEnabled = state;
}

void DeltaGliderXR1::ActivateAPU(DoorStatus action)
{
    // if crew incapacitated, cannot activate APU
    if (IsCrewIncapacitatedOrNoPilotOnBoard())
        return;  // cannot activate

    // TODO: add code to fail this or take out the failure check below
    // check for failure
    if (apu_status == DOOR_FAILED)
    {
        PlayErrorBeep();
        ShowWarning("Warning Aux Power Unit Failure.wav", ST_WarningCallout, "APU FAILED.");
        return;  // cannot activate
    }

    // check fuel level
    if ((m_apuFuelQty <= 0.0) && ((action == DOOR_OPEN) || (action == DOOR_OPENING)))
    {
        PlayErrorBeep();
        ShowWarning("Warning APU Fuel Depleted No Hydraulic Pressure.wav", ST_WarningCallout, "APU fuel depleted:&NO HYDRAULIC PRESSURE!");
        return;     // cannot activate
    }

    // update APU inactive timestamp for ALL actions (OK to reset even if door closing)
    MarkAPUActive();  // reset the APU idle warning callout time

    apu_status = action;
    RecordEvent("APU", ((action == DOOR_CLOSING) || (action == DOOR_CLOSED)) ? "CLOSE" : "OPEN");

    TriggerRedrawArea(AID_APU_BUTTON);
}

void DeltaGliderXR1::ToggleAPU(void)
{
    ActivateAPU((apu_status == DOOR_CLOSED || apu_status == DOOR_CLOSING) ? DOOR_OPENING : DOOR_CLOSING);
}

// Returns max configured thrust for the specified thruster BEFORE taking atmosphere or 
// damage into account.
// index = 0-13
double DeltaGliderXR1::GetRCSThrustMax(const int index) const
{
    // Attitude control system max thrust [N] per engine.
    const double MAX_FOREAFT_RCS_THRUST = (2 * MAX_RCS_THRUST);

    double retVal;

    if ((index == 12) || (index == 13))
        retVal = MAX_FOREAFT_RCS_THRUST;
    else
        retVal = MAX_RCS_THRUST;

    // For attitude hold or descent hold in an atmosphere, the pitch jets switch to a high-power mode.
    if (GetAtmPressure() > 1)
    {
        if ((m_customAutopilotMode == AP_ATTITUDEHOLD) || (m_customAutopilotMode == AP_DESCENTHOLD))
            retVal *= AP_ATTITUDE_HOLD_RCS_THRUST_MULTIPLIER;
    }

    return retVal;
}

// --------------------------------------------------------------
// Respond to MFD mode change
// --------------------------------------------------------------
void DeltaGliderXR1::clbkMFDMode (int mfd, int mode)
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
    if ((apu_status == DOOR_OPEN) && (IsCrewIncapacitatedOrNoPilotOnBoard() == false))
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
void DeltaGliderXR1::clbkNavMode (int mode, bool active)
{
    // redraw the navmode buttons
    TriggerNavButtonRedraw();

    const char *pAction = nullptr;    // set below
    if (active)
    {
        if (mode != NAVMODE_KILLROT)
        {
            PlaySound(AutopilotOn, ST_Other, AUTOPILOT_VOL);
            
            // disable any custom autopilot mode
            SetCustomAutopilotMode(AP_OFF, false);  // do not play sounds for this
        }

        pAction = "engaged";
    }
    else  // normal autopilot disabled now
    {
        // play the AutopilotOff sound for all modes except KILLROT, UNLESS custom autopilot is active now
        // (we don't want to play AutoPilotOff if custom autopilot is on now)
        if ((mode != NAVMODE_KILLROT) && (m_customAutopilotMode == AP_OFF))
            PlaySound(AutopilotOff, ST_Other, AUTOPILOT_VOL);

        pAction = "disengaged";
    }

    // set the corresponding label for all modes except killrot
    static const char *pNavModeLabels[] = 
    { 
        NULL, NULL, "LEVEL HORIZON", "PROGRADE", "RETROGRADE", "ORBIT-NORMAL",
        "ORBIT-ANTINORMAL", "HOLD ALTITUDE"
    };
    const char *pLabel = pNavModeLabels[mode];

    if (pLabel != nullptr)
    {
        char temp[40];
        sprintf(temp, "%s autopilot %s.", pLabel, pAction);
        ShowInfo(NULL, DeltaGliderXR1::ST_None,temp);
    }
}

bool DeltaGliderXR1::clbkLoadGenericCockpit ()
{
    SetCameraOffset (_V(0,1.467,6.782));
    oapiSetDefNavDisplay (1);
    oapiSetDefRCSDisplay (1);
    campos = CAM_GENERIC;
    return true;
}

// kill all attitude thrusters; usually invoked from autopilot handlers when autopilot switches off
void DeltaGliderXR1::KillAllAttitudeThrusters()
{
    SetThrusterGroupLevel(THGROUP_ATT_PITCHUP, 0);
    SetThrusterGroupLevel(THGROUP_ATT_PITCHDOWN, 0);
    SetThrusterGroupLevel(THGROUP_ATT_YAWLEFT, 0);
    SetThrusterGroupLevel(THGROUP_ATT_YAWRIGHT, 0);
    SetThrusterGroupLevel(THGROUP_ATT_BANKLEFT, 0);
    SetThrusterGroupLevel(THGROUP_ATT_BANKRIGHT, 0);
    SetThrusterGroupLevel(THGROUP_ATT_RIGHT, 0);
    SetThrusterGroupLevel(THGROUP_ATT_LEFT, 0);
    SetThrusterGroupLevel(THGROUP_ATT_UP, 0);
    SetThrusterGroupLevel(THGROUP_ATT_DOWN, 0);
    SetThrusterGroupLevel(THGROUP_ATT_FORWARD, 0);
    SetThrusterGroupLevel(THGROUP_ATT_BACK, 0);
}

// set all major control surfaces to neutral
// NOTE: this will NOT check for hydraulic pressure; it is assume the caller will have handled that already
void DeltaGliderXR1::NeutralAllControlSurfaces()
{
    SetControlSurfaceLevel(AIRCTRL_ELEVATOR, 0);
    SetControlSurfaceLevel(AIRCTRL_AILERON, 0);
    SetControlSurfaceLevel(AIRCTRL_RUDDER, 0);
}

// load a WAV file for XRSound to use
void DeltaGliderXR1::LoadXR1Sound(const Sound sound, const char *pFilename, XRSound::PlaybackType playbackType)
{
    if (!m_pXRSound->IsPresent())
        return;  

    // use member variable here so we can preserve the last file loaded for debugging purposes
    sprintf(m_lastWavLoaded, "%s\\%s", m_pXRSoundPath, pFilename);
    BOOL stat = m_pXRSound->LoadWav(sound, m_lastWavLoaded, playbackType);
#ifdef _DEBUG
    if (!stat)
        sprintf(oapiDebugString(), "ERROR: LoadXR1Sound: LoadWav failed, filename='%s'", pFilename);
#endif
}

// play a sound via the XRSound SDK
// volume default=255  -- NOTE: only applies if soundType == ST_Other; otherwise, volume is set AudioCalloutVolume config setting.
// loop default=NOLOOP (other value is LOOP)
void DeltaGliderXR1::PlaySound(Sound sound, const SoundType soundType, int volume, bool bLoop)
{
    if (!m_pXRSound->IsPresent())
        return;

    // if we are not in focus, do not play the sound since it would fail anyway
    if (HasFocus() == false)
        return;

#ifdef _DEBUG
    // sanity check if debugging
    if ((soundType == ST_None) && (sound != NO_SOUND))
    {
        char temp[512];
        sprintf(temp, "INTERNAL ERROR: PlaySound: ST_None specified for non-null sound=%d : m_lastWavLoaded=[%s]", sound, m_lastWavLoaded);
        strcpy(oapiDebugString(), temp);
        // fall through and play the sound
    }
#endif

    if (soundType != ST_Other)
        volume = GetXR1Config()->AudioCalloutVolume;  // overrides any requested audio callout volume

    // now check whether the user wants to play this type of callout
    bool playSound;
    switch (soundType)
    {
    case ST_AudioStatusGreeting:
        playSound = GetXR1Config()->EnableAudioStatusGreeting;
        break;

    case ST_VelocityCallout:
        playSound = GetXR1Config()->EnableVelocityCallouts;
        break;

    case ST_AltitudeCallout:
        playSound = GetXR1Config()->EnableAltitudeCallouts;
        break;

    case ST_DockingDistanceCallout:
        playSound = GetXR1Config()->EnableDockingDistanceCallouts;
        break;

    case ST_InformationCallout:
        playSound = GetXR1Config()->EnableInformationCallouts;
        break;

    case ST_RCSStatusCallout:
        playSound = GetXR1Config()->EnableRCSStatusCallouts;
        break;

    case ST_AFStatusCallout:
        playSound = GetXR1Config()->EnableAFStatusCallouts;
        break;

    case ST_WarningCallout:
        playSound = GetXR1Config()->EnableWarningCallouts;
        break;

    case ST_Other:
        playSound = true;  // sound effects *always* play
        break;

    default:
        // should never happen!  (ST_None should never come through here!)
        playSound = true;   // play the sound anyway

        // only show an error during development
#ifdef _DEBUG   
        char temp[512];
        sprintf(temp, "ERROR: PlaySound: Unknown Soundtype value (%d) for sound=%d : m_lastWavLoaded=[%s]", soundType, sound, m_lastWavLoaded);
        strcpy(oapiDebugString(), temp);
#endif
        break;
    }

    if (playSound == false)
        return;     // user doesn't want the sound to play

    // play the sound!
    const float volFrac = min(static_cast<float>(volume) / 255.0f, 1.0f);  // convert legacy volume 0-255 to 0-1.0.
    BOOL stat = m_pXRSound->PlayWav(sound, bLoop, volFrac);

    // We don't want "missing wave file" errors showing up for users; they may want to delete
    // some sound files because they don't like them, so we don't want to clutter the log with
    // useless messages.  We only need this during development.
#ifdef _DEBUG
    if (stat == FALSE)
    {
        char temp[512];
        sprintf(temp, "ERROR: PlaySound: PlayWav failed, sound=%d : m_lastWavLoaded=[%s]", sound, m_lastWavLoaded);
        strcpy(oapiDebugString(), temp);

        // also write to the log
        GetXR1Config()->WriteLog(temp);

        // now let's play an audible alert, too
        m_pXRSound->PlayWav(ErrorSoundFileMissing);
    }
#endif
}

// stop a sound via the XRSound SDK
void DeltaGliderXR1::StopSound(Sound sound)
{
    if (!m_pXRSound->IsPresent())
        return;

    // if we are not in focus, do not stop the sound since it would fail anyway
    if (HasFocus() == false)
        return;

    // OK if sound is already stopped here
    m_pXRSound->StopWav(sound);
}

// check whether the specified sound is playing
bool DeltaGliderXR1::IsPlaying(Sound sound) const
{
    if (!m_pXRSound->IsPresent())
        return false;

    return m_pXRSound->IsWavPlaying(sound);
}

// play a warning sound and display a warning message via the DisplayWarningPoststep
// pSoundFilename may be null or empty; pMessage may be null
// NOTE: specific component damage may be determined by polling the lwingstatus, etc.
// if force == true, always play the incoming wav
void DeltaGliderXR1::ShowWarning(const char *pSoundFilename, const SoundType soundType, const char *pMessage, bool force)
{
    if (IsCrashed())  // show if incapacitated
        return;       // no more warnings

    if (pMessage != nullptr)
    {
        // display warning message only IF it was not the last warning displayed
        if (strcmp(pMessage, m_lastWarningMessage) != 0)    // strings do not match?
        {
            // add to the info/warning text line vector
            m_infoWarningTextLineGroup.AddLines(pMessage, true);  // text is highlighted

            // save for check next time
            strcpy(m_lastWarningMessage, pMessage);
        }
    }

    // the poststep will pick this sound at the next timestep and play it within 5 seconds
    if (pSoundFilename != nullptr)
    {
        _ASSERTE(soundType != ST_None);
        strcpy(m_warningWavFilename, pSoundFilename);
        m_warningWaveSoundType = soundType;
    }
    else
    {
        _ASSERTE(soundType == ST_None);
    }
}

// play an info sound and display an info message via the DisplayWarningPoststep
// pSoundFilename and/or pMessage may be null
void DeltaGliderXR1::ShowInfo(const char *pSoundFilename, const SoundType soundType, const char *pMessage)
{
    if (IsCrashed())  // DO show if incapacitated
        return;     // no more messages

    // check whether a new info message has been set
    if (pMessage != nullptr)
    {
        // add to the info/warning text line vector
        m_infoWarningTextLineGroup.AddLines(pMessage, false);  // text is not highlighted
    }

    // play the info sound, if any
    // Info sounds are relatively infrequent, so no need for a PostStep to manage it
    if ((pSoundFilename != nullptr) && (*pSoundFilename != 0))
    {
        LoadXR1Sound(Info, pSoundFilename, XRSound::PlaybackType::Radio);
        PlaySound(Info, soundType);
    }

    // Clear the last warning message value so that the same warning can be displayed again;
    // this is so that the warning will always be printed again after an info message is displayed.
    *m_lastWarningMessage = 0;
}

// Play the error beep and kill any switch and key sounds in progress
void DeltaGliderXR1::PlayErrorBeep()
{
    // stop any switch or key sounds that may have been started
    m_pXRSound->StopWav(SwitchOn);
    m_pXRSound->StopWav(SwitchOff);
    m_pXRSound->StopWav(BeepHigh);
    m_pXRSound->StopWav(BeepLow);

    PlaySound(Error1, ST_Other, ERROR1_VOL);      // error beep
}

// Play a door opening/closing beep; usually invoked from key handlers
void DeltaGliderXR1::PlayDoorSound(DoorStatus doorStatus)
{
    if (doorStatus == DOOR_OPENING)
        PlaySound(_DoorOpening, ST_Other);
    else if (doorStatus == DOOR_CLOSING)
        PlaySound(_DoorClosing, ST_Other);
}

// perform an EVA for the specified crew member
// Returns: true on success, false on error (crew member not present or outer airlock door is closed)
bool DeltaGliderXR1::PerformEVA(const int ummuCrewMemberIndex)
{
    bool retVal = false;

    // NOTE: the crew member should always be onboard here because we only display members
    // that are onboard the ship; therefore, we don't need to check for that here.

    if (CheckEVADoor() == false)
        return false;     

#ifdef MMU
    // door is OK; retrieve data for crew member to perform EVA from UMMu
    const char *pCrewMemberNameUMMu = GetCrewNameBySlotNumber(ummuCrewMemberIndex);
    
    // must copy crew member name to our own buffer because UMmu will reuse the buffer below!
    char pCrewMemberName[40];       // matches UMmu name length
    strcpy(pCrewMemberName, pCrewMemberNameUMMu);

    if (strlen(pCrewMemberName) == 0)   // crew member not on board?
    {
        // should never happen!
        PlayErrorBeep();
        ShowWarning(NULL, DeltaGliderXR1::ST_None, "INTERNAL Mmu ERROR:&Crew member not on board!");
        return false;
    }

    // set the mesh for this crew member
    const char *pMesh = DEFAULT_CREW_MESH;  // assume custom mesh not set

    // UMmu bug: must cast away constness below
    const char *pMisc = GetCrewMiscIdByName(const_cast<char *>(pCrewMemberName));
    if (strlen(pMisc) > 0)
        pMesh = RetrieveMeshForUMmuMisc(pMisc); // use custom mesh

    UMmu.SetAlternateMeshToUseForEVASpacesuit(pMesh);

    // set O2 levels
    UMmu.SetO2ReserveWhenEvaing(100);   // default
    // Override from stupid default of 1000, which is CHEATING!  How can you suddenly jam 10x 
    // more O2 into your suit tanks just because you are ejecting?  It's not like you had 
    // time to "switch tanks."  Rubbish...
    UMmu.SetO2ReserveWhenEjecting(100); 

    // use EjectCrewMember here if ship in flight in an atmosphere
    int evaStatus;
    if ((IsLanded() == false) && (GetAtmPressure() >= 1e3) && (GetAltitude(ALTMODE_GROUND) >= 20))
    {
        evaStatus = UMmu.EjectCrewMember(const_cast<char *>(pCrewMemberName));
    }
    else    // normal EVA
        evaStatus = UMmu.EvaCrewMember(const_cast<char *>(pCrewMemberName));

    if ((evaStatus == TRANSFER_TO_DOCKED_SHIP_OK) || (evaStatus == EVA_OK))
    {
        // EVA successful!  No need to remove the crew member manually since UMmu will do it for us.

        SetPassengerVisuals();     // update the VC mesh

        if (IsDocked() && (m_pActiveAirlockDoorStatus == &olock_status))
        {
            char temp[100];
            sprintf(temp, "%s transferred&to docked vessel successfully!", pCrewMemberName);
            ShowInfo("Crew Member Transferred Successfully.wav", ST_InformationCallout, temp);
        }
        else  // not docked or docking port not active
        {
            char temp[100];
            sprintf(temp, "%s on EVA.", pCrewMemberName);
            ShowInfo("Egress Successful.wav", ST_InformationCallout, temp);
            retVal = true;   // success
        }
        retVal = true;
    }
    else if (evaStatus == ERROR_DOCKED_SHIP_HAVE_AIRLOCK_CLOSED)
    {
        PlayErrorBeep();
        ShowWarning("Warning Docked Ship's Outer Door is Closed.wav", ST_WarningCallout, "Crew transfer failed:&Docked ship's airlock is closed.");
    }
    else if (evaStatus == ERROR_DOCKED_SHIP_IS_FULL)
    {
        PlayErrorBeep();
        ShowWarning("Warning Docked Ship Has a Full Complement.wav", ST_WarningCallout, "Cannot transfer crew: Docked&ship has a full crew complement.");
    }
    else if (evaStatus == ERROR_DOCKEDSHIP_DONOT_USE_UMMU)
    {
        PlayErrorBeep();
        ShowWarning("Warning Crew Member Transfer Failed.wav", ST_WarningCallout, "Docked ship does not support UMmu!");
    }
    else    // other UMmu error
#endif
    {
        if (IsDocked())
        {
            PlayErrorBeep();
            ShowWarning("Warning Crew Member Transfer Failed.wav", ST_WarningCallout, "Crew member transfer failed.");
        }
        else    // internal error!  Should never happen!
        {
            PlayErrorBeep();
            ShowWarning(NULL, DeltaGliderXR1::ST_None, "INTERNAL Mmu ERROR: EVA FAILED");
        }
    }

    return retVal;
}

// returns: true if EVA doors are OK, false if not
// Note: this is also invoked for turbopack deployment/stowage 
bool DeltaGliderXR1::CheckEVADoor()
{
    // NOTE: we do not enforce the sequence of open inner door -> close inner door ->
    // depressurize -> open outer door for EVA because that is too tedious, plus the pilot
    // can still do that if he wants to.  We merely require that the outer door is open, which will require the
    // pilot to equalize the airlock pressure and open the outer door and nosecone first.
    
    // verify that the outer door and nosecone are both open
    // We really wouldn't have to check for nosecone here, since the outer door already requires that
    // the nosecone be open before the outer door can open; however, we want to give the pilot an accurate callout.
    if (nose_status != DOOR_OPEN)
    {
        PlayErrorBeep();
        char msg[128];
        sprintf(msg, "%s is closed.", NOSECONE_LABEL);
        ShowWarning(WARNING_NOSECONE_IS_CLOSED_WAV, ST_WarningCallout, msg);
        return false;
    }

    if (olock_status != DOOR_OPEN)
    {
        PlayErrorBeep();
        ShowWarning("Warning Outer Door is Closed.wav", ST_WarningCallout, "Outer door is closed.");
        return false;
    } 

    return true;
}

// Show a fatal error message box and terminate Orbiter
void DeltaGliderXR1::FatalError(const char *pMsg)
{
    // write to the log
    GetXR1Config()->WriteLog(pMsg);

    // close the main window so the dialog box will appear
    const HWND mainWindow = GetForegroundWindow();

    // show critical error, close the window, and exit
    MessageBox(mainWindow, pMsg, "Orbiter DG-XR1 Fatal Error", MB_OK | MB_SETFOREGROUND | MB_SYSTEMMODAL);
    CloseWindow(mainWindow);
    exit(-1);   // bye, bye
}

// extract a crew member's rank from the Mmu 'misc' field
// This returns a pointer to a static working buffer.  If pMisc is not from an
// XR1 crew member, the Misc ID itself is returned.
const char *DeltaGliderXR1::RetrieveRankForMmuMisc(const char *pMisc) const
{
    const char *retVal = pMisc;   // assume non-XR1 misc ID

    int index = ExtractIndexFromMmuMisc(pMisc);
    if (index >= 0)
    {
        CrewMember *pCM = GetXR1Config()->CrewMembers + index;
        retVal = pCM->rank;
    }

    return retVal;
}

// extract a crew member's mesh from the Mmu 'misc' field
// If pMisc is not from an XR1 crew member, the default Mmu mesh is returned.
const char *DeltaGliderXR1::RetrieveMeshForMmuMisc(const char *pMisc) const
{
    const char *retVal = DEFAULT_CREW_MESH;   // assume non-XR1 misc ID

    int index = ExtractIndexFromMmuMisc(pMisc);
    if (index >= 0)
    {
        CrewMember *pCM = GetXR1Config()->CrewMembers + index;
        retVal = pCM->mesh;
    }

    return retVal;
}

// extract a crew index (0..n) from the supplied Mmu 'misc' field, or -1 if pMisc is invalid
int DeltaGliderXR1::ExtractIndexFromMmuMisc(const char *pMisc)
{
    int index = -1;

    if ((strlen(pMisc) > 2) && (*pMisc == 'X') && (pMisc[1] == 'I'))  // "XI0", "XI1", etc.
        sscanf(pMisc+2, "%d", &index);   // skip leading "XR"

    return index;
}

// obtain the UMmu crew member slot number for the given name
// Returns: 0...n on success, or -1 if name is invalid
int DeltaGliderXR1::GetMmuSlotNumberForName(const char *pName) const
{
    int retVal = -1;    // assume not found

    for (int i=0; i < MAX_PASSENGERS; i++)
    {
#ifdef MMU
        const char *pUMmuName = CONST_UMMU(this).GetCrewNameBySlotNumber(i);
        if (strcmp(pName, pUMmuName) == 0)
        {
            retVal = i;     // found the slot!
            break;
        }
#endif
    }

    return retVal;
}

// returns true if Mmu crew member is on board or false if not
bool DeltaGliderXR1::IsCrewMemberOnBoard(const int index) const
{
#ifdef MMU
    const char *pName = CONST_UMMU(this).GetCrewNameBySlotNumber(index);
    bool retVal = (strlen(pName) > 0);

    return retVal;
#else
    return true;
#endif
}

void DeltaGliderXR1::UpdateCtrlDialog (DeltaGliderXR1 *dg, HWND hWnd)
{
    static int bstatus[2] = {BST_UNCHECKED, BST_CHECKED};
    
    if (!hWnd) hWnd = oapiFindDialog (g_hDLL, IDD_CTRL);
    if (!hWnd) return;
    
    int op;
    
    op = dg->gear_status & 1;
    SendDlgItemMessage (hWnd, IDC_GEAR_DOWN, BM_SETCHECK, bstatus[op], 0);
    SendDlgItemMessage (hWnd, IDC_GEAR_UP, BM_SETCHECK, bstatus[1-op], 0);
    
    op = dg->rcover_status & 1;
    SendDlgItemMessage (hWnd, IDC_RETRO_OPEN, BM_SETCHECK, bstatus[op], 0);
    SendDlgItemMessage (hWnd, IDC_RETRO_CLOSE, BM_SETCHECK, bstatus[1-op], 0);
    
    op = dg->nose_status & 1;
    SendDlgItemMessage (hWnd, IDC_NCONE_OPEN, BM_SETCHECK, bstatus[op], 0);
    SendDlgItemMessage (hWnd, IDC_NCONE_CLOSE, BM_SETCHECK, bstatus[1-op], 0);
    
    op = dg->olock_status & 1;
    SendDlgItemMessage (hWnd, IDC_OLOCK_OPEN, BM_SETCHECK, bstatus[op], 0);
    SendDlgItemMessage (hWnd, IDC_OLOCK_CLOSE, BM_SETCHECK, bstatus[1-op], 0);
    
    op = dg->ilock_status & 1;
    SendDlgItemMessage (hWnd, IDC_ILOCK_OPEN, BM_SETCHECK, bstatus[op], 0);
    SendDlgItemMessage (hWnd, IDC_ILOCK_CLOSE, BM_SETCHECK, bstatus[1-op], 0);
    
    op = dg->ladder_status & 1;
    SendDlgItemMessage (hWnd, IDC_LADDER_EXTEND, BM_SETCHECK, bstatus[op], 0);
    SendDlgItemMessage (hWnd, IDC_LADDER_RETRACT, BM_SETCHECK, bstatus[1-op], 0);
    
    op = dg->hatch_status & 1;
    SendDlgItemMessage (hWnd, IDC_HATCH_OPEN, BM_SETCHECK, bstatus[op], 0);
    SendDlgItemMessage (hWnd, IDC_HATCH_CLOSE, BM_SETCHECK, bstatus[1-op], 0);
    
    op = dg->radiator_status & 1;
    SendDlgItemMessage (hWnd, IDC_RADIATOR_EXTEND, BM_SETCHECK, bstatus[op], 0);
    SendDlgItemMessage (hWnd, IDC_RADIATOR_RETRACT, BM_SETCHECK, bstatus[1-op], 0);
    
    op = dg->beacon[0].active ? 1:0;
    SendDlgItemMessage (hWnd, IDC_NAVLIGHT, BM_SETCHECK, bstatus[op], 0);
    op = dg->beacon[3].active ? 1:0;
    SendDlgItemMessage (hWnd, IDC_BEACONLIGHT, BM_SETCHECK, bstatus[op], 0);
    op = dg->beacon[5].active ? 1:0;
    SendDlgItemMessage (hWnd, IDC_STROBELIGHT, BM_SETCHECK, bstatus[op], 0);
}

// --------------------------------------------------------------
// Convert spaces to a character that Orbiter can save
// --------------------------------------------------------------
void DeltaGliderXR1::EncodeSpaces(char *pStr)
{
    for (char *p = pStr; *p; p++)
    {
        if (*p == ' ')
            *p = '$';
    }
}

// --------------------------------------------------------------
// Decode a string saved in the scenario file
// --------------------------------------------------------------
void DeltaGliderXR1::DecodeSpaces(char *pStr)
{
    for (char *p = pStr; *p; p++)
    {
        if (*p == '$')
            *p = ' ';
    }
}

// Format a double with commas to a given number of decimal places
// e.g., "10,292.7"
void DeltaGliderXR1::FormatDouble(const double val, CString &out, const int decimalPlaces)
{
    CString format;
    _ASSERTE(decimalPlaces >= 0);
    format.Format("%%.%dlf", decimalPlaces);  // e.g., "%.2lf"
    
    out.Format(format, val);   // "10292.7"

    // now add in the commas; do not use a comma at three places for under 10,000
    int lowThreshold = ((val < 10000) ? 1 : 0);  //
    for (int index = out.Find('.') - 3; index > lowThreshold; index -= 3)
        out.Insert(index, ',');
}

// Returns the flow rate of a thruster in kg/sec
double DeltaGliderXR1::GetThrusterFlowRate(const THRUSTER_HANDLE th) const
{
    double level  = GetThrusterLevel(th); // throttle level
    double isp    = GetThrusterIsp0(th);  // must vacuum rating here since atmosphere does not affect flow rate
    double thrust = GetThrusterMax0(th);  // must use vacuum rating here since our ISP is a vacuum ISP
    double flow   = thrust * level / isp;
    
    return flow;
}

// all XR vessels should invoke this from clbkSetClassCaps to parse their configuration file(s)
void DeltaGliderXR1::ParseXRConfigFile()
{
    // NOTE: this should be the *only place* where ParseVesselConfig and ApplyCheatcodesIfEnabled are invoked
    m_pConfig->ParseVesselConfig(GetName());

    // now apply the cheatcodes if they are enabled
    // Note: cannot use GetXRConfig() here because we cannot make ApplyCheatcodesIfEnabled() const
    (static_cast<XR1ConfigFileParser *>(m_pConfig))->ApplyCheatcodesIfEnabled();
}

// ==============================================================
// Overloaded callback functions
// NOTE: normally you should override these if you subclass the XR1!
// ==============================================================

// --------------------------------------------------------------
// Set vessel class parameters
// --------------------------------------------------------------
void DeltaGliderXR1::clbkSetClassCaps (FILEHANDLE cfg)
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
    
    VESSEL2::SetEmptyMass(EMPTY_MASS);   // 12000 matches DG3
    SetSize (10.0);     // DG3 = 9.0 meters radius
    SetVisibilityLimit (7.5e-4, 1.5e-3);
    SetAlbedoRGB (_V(0.77,0.20,0.13));
    SetGravityGradientDamping (20.0);
    SetCW (0.09, 0.09, 2, 1.4);
    SetWingAspect (WING_ASPECT_RATIO);
    SetWingEffectiveness (2.5);
    SetCrossSections (_V(53.0,186.9,25.9));
    SetMaxWheelbrakeForce(MAX_WHEELBRAKE_FORCE);  
    SetPMI (_V(15.5,22.1,7.7));
    
    SetDockParams (_V(0,-0.49,10.076), _V(0,0,1), _V(0,1,0));
    SetGearParameters(1.0);  // NOTE: must init touchdown points here with gear DOWN!  This will be called again later by clbkPostCreation to init the "real" state from scenario file.
    EnableTransponder (true);
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
    max_scramfuel  = TANK2_CAPACITY;

    // NOTE: Orbiter seems to reset the 'current fuel mass' value to zero later, since it expects the scenario file to be read
    // WARNING: do NOT init 'fuel mass' value (optional second argument) to > 0, because Orbiter will NOT set the tank value if the fraction is 
    // zero in the scenario file.
    ph_main  = CreatePropellantResource(max_rocketfuel);       // main tank (fuel + oxydant)
    ph_rcs   = CreatePropellantResource(RCS_FUEL_CAPACITY);    // RCS tank  (fuel + oxydant)
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
    th_main[0] = CreateThruster (_V(-1,0.0,-7.7), _V(0,0,1), MAX_MAIN_THRUST[GetXR1Config()->MainEngineThrust], ph_main, mainISP, mainISP*ispscale);
    th_main[1] = CreateThruster (_V( 1,0.0,-7.7), _V(0,0,1), MAX_MAIN_THRUST[GetXR1Config()->MainEngineThrust], ph_main, mainISP, mainISP*ispscale);

    thg_main = CreateThrusterGroup (th_main, 2, THGROUP_MAIN);
    // increase thruster flame: stock was 12, 1
    SURFHANDLE mainExhaustTex = oapiRegisterExhaustTexture("dg-xr1\\ExhaustXR1");
    // Pre-1.9 release: length was 12
    AddXRExhaust (th_main[0], 10, 0.811, mainExhaustTex);
    AddXRExhaust (th_main[1], 10, 0.811, mainExhaustTex);

    // move exhaust smoke away from engines a bit
    // pre-1.9 release: const double mainDelta = -3;
    const double mainDelta = -1.5;
    AddExhaustStream (th_main[0], _V(-1,0,-15 + mainDelta), &contrail);
    AddExhaustStream (th_main[1], _V( 1,0,-15 + mainDelta), &contrail);
    AddExhaustStream (th_main[0], _V(-1,0,-10 + mainDelta), &exhaust_main);
    AddExhaustStream (th_main[1], _V( 1,0,-10 + mainDelta), &exhaust_main);
    
    // retro thrusters
    th_retro[0] = CreateThruster (_V(-3, 0, 5.3), _V(0, 0, -1.0), MAX_RETRO_THRUST, ph_main, mainISP, mainISP*ispscale);
    th_retro[1] = CreateThruster (_V( 3, 0, 5.3), _V(0, 0, -1.0), MAX_RETRO_THRUST, ph_main, mainISP, mainISP*ispscale);
    thg_retro = CreateThrusterGroup (th_retro, 2, THGROUP_RETRO);
    AddXRExhaust (th_retro[0], 1.5, 0.16, _V(-3, -0.300, 5.3), _V(0, 0, 1.0), mainExhaustTex);
    AddXRExhaust (th_retro[1], 1.5, 0.16, _V( 3, -0.300, 5.3), _V(0, 0, 1.0), mainExhaustTex);
    
    // hover thrusters (simplified)
    // The two aft hover engines are combined into a single "logical" thruster,
    // but exhaust is rendered separately for both
    th_hover[0] = CreateThruster (_V(0,0,3), _V(0,1,0),  MAX_HOVER_THRUST[GetXR1Config()->HoverEngineThrust], ph_main, mainISP, mainISP*ispscale);
    th_hover[1] = CreateThruster (_V(0,0,-3), _V(0,1,0), MAX_HOVER_THRUST[GetXR1Config()->HoverEngineThrust], ph_main, mainISP, mainISP*ispscale);
    thg_hover = CreateThrusterGroup (th_hover, 2, THGROUP_HOVER);
    // pre-1.9 version: increase thruster flame: was length 6
    AddXRExhaust (th_hover[0], 4.75, 0.5, _V( 0,-1.6, 3), _V(0,-1,0), mainExhaustTex);
    AddXRExhaust (th_hover[1], 4.75, 0.5, _V(-3,-1.3,-4.55), _V(0,-1,0), mainExhaustTex);
    AddXRExhaust (th_hover[1], 4.75, 0.5, _V( 3,-1.3,-4.55), _V(0,-1,0), mainExhaustTex);
    
    // move exhaust smoke away from engines a bit
    // pre-1.9 version: const double hoverDelta = -3;
    const double hoverDelta = -1.5;
    AddExhaustStream (th_hover[0], _V(0,-4 + hoverDelta,0), &contrail);
    AddExhaustStream (th_hover[0], _V(0,-2 + hoverDelta,3), &exhaust_hover);
    AddExhaustStream (th_hover[0], _V(-3,-2 + hoverDelta,-4.55), &exhaust_hover);
    AddExhaustStream (th_hover[0], _V( 3,-2 + hoverDelta,-4.55), &exhaust_hover);
    
    // set of attitude thrusters (idealised). The arrangement is such that no angular
    // momentum is created in linear mode, and no linear momentum is created in rotational mode.
    SURFHANDLE rcsExhaustTex = mainExhaustTex;
    THRUSTER_HANDLE th_att_rot[4], th_att_lin[4];
    // NOTE: save in th_rcs array so we can disable them later
    th_rcs[0] = th_att_rot[0] = th_att_lin[0] = CreateThruster (_V(0,0, 8), _V(0, 1,0), GetRCSThrustMax(0), ph_rcs, mainISP);
    th_rcs[1] = th_att_rot[1] = th_att_lin[3] = CreateThruster (_V(0,0,-8), _V(0,-1,0), GetRCSThrustMax(1), ph_rcs, mainISP);
    th_rcs[2] = th_att_rot[2] = th_att_lin[2] = CreateThruster (_V(0,0, 8), _V(0,-1,0), GetRCSThrustMax(2), ph_rcs, mainISP);
    th_rcs[3] = th_att_rot[3] = th_att_lin[1] = CreateThruster (_V(0,0,-8), _V(0, 1,0), GetRCSThrustMax(3), ph_rcs, mainISP);
    CreateThrusterGroup (th_att_rot,   2, THGROUP_ATT_PITCHUP);
    CreateThrusterGroup (th_att_rot+2, 2, THGROUP_ATT_PITCHDOWN);
    CreateThrusterGroup (th_att_lin,   2, THGROUP_ATT_UP);
    CreateThrusterGroup (th_att_lin+2, 2, THGROUP_ATT_DOWN);
    AddXRExhaust (th_att_rot[0], 0.6,  0.078, _V(-0.75,-0.7,  9.65), _V(0,-1,0), rcsExhaustTex);
    AddXRExhaust (th_att_rot[0], 0.6,  0.078, _V( 0.75,-0.7,  9.65), _V(0,-1,0), rcsExhaustTex);
    AddXRExhaust (th_att_rot[1], 0.79, 0.103, _V(-0.1 , 0.55,-7.3 ), _V(0, 1,0), rcsExhaustTex);
    AddXRExhaust (th_att_rot[1], 0.79, 0.103, _V( 0.1 , 0.55,-7.3 ), _V(0, 1,0), rcsExhaustTex);
    AddXRExhaust (th_att_rot[2], 0.6,  0.078, _V(-0.8,-0.25, 9.6), _V(0, 1,0),   rcsExhaustTex);
    AddXRExhaust (th_att_rot[2], 0.6,  0.078, _V( 0.8,-0.25, 9.6), _V(0, 1,0),   rcsExhaustTex);
    AddXRExhaust (th_att_rot[3], 0.79, 0.103, _V(-0.1, -0.55,-7.3 ), _V(0,-1,0), rcsExhaustTex);
    AddXRExhaust (th_att_rot[3], 0.79, 0.103, _V( 0.1, -0.55,-7.3 ), _V(0,-1,0), rcsExhaustTex);
    
    th_rcs[4] = th_att_rot[0] = th_att_lin[0] = CreateThruster (_V(0,0, 6), _V(-1,0,0), GetRCSThrustMax(4), ph_rcs, mainISP);
    th_rcs[5] = th_att_rot[1] = th_att_lin[3] = CreateThruster (_V(0,0,-6), _V( 1,0,0), GetRCSThrustMax(5), ph_rcs, mainISP);
    th_rcs[6] = th_att_rot[2] = th_att_lin[2] = CreateThruster (_V(0,0, 6), _V( 1,0,0), GetRCSThrustMax(6), ph_rcs, mainISP);
    th_rcs[7] = th_att_rot[3] = th_att_lin[1] = CreateThruster (_V(0,0,-6), _V(-1,0,0), GetRCSThrustMax(7), ph_rcs, mainISP);
    CreateThrusterGroup (th_att_rot,   2, THGROUP_ATT_YAWLEFT);
    CreateThrusterGroup (th_att_rot+2, 2, THGROUP_ATT_YAWRIGHT);
    CreateThrusterGroup (th_att_lin,   2, THGROUP_ATT_LEFT);
    CreateThrusterGroup (th_att_lin+2, 2, THGROUP_ATT_RIGHT);
    AddXRExhaust (th_att_rot[0], 0.6,  0.078, _V(1.0,-0.48,9.35), _V(1,0,0), rcsExhaustTex);
    AddXRExhaust (th_att_rot[1], 0.94, 0.122, _V(-2.2,0.2,-6.0), _V(-1,0,0), rcsExhaustTex);
    AddXRExhaust (th_att_rot[2], 0.6,  0.078, _V(-1.0,-0.48,9.35), _V(-1,0,0), rcsExhaustTex);
    AddXRExhaust (th_att_rot[3], 0.94, 0.122, _V(2.2,0.2,-6.0), _V(1,0,0), rcsExhaustTex);
    
    th_rcs[8] = th_att_rot[0] = CreateThruster (_V( 6,0,0), _V(0, 1,0), GetRCSThrustMax(8), ph_rcs, mainISP);
    th_rcs[9] = th_att_rot[1] = CreateThruster (_V(-6,0,0), _V(0,-1,0), GetRCSThrustMax(9), ph_rcs, mainISP);
    th_rcs[10] = th_att_rot[2] = CreateThruster (_V(-6,0,0), _V(0, 1,0), GetRCSThrustMax(10), ph_rcs, mainISP);
    th_rcs[11] = th_att_rot[3] = CreateThruster (_V( 6,0,0), _V(0,-1,0), GetRCSThrustMax(11), ph_rcs, mainISP);
    CreateThrusterGroup (th_att_rot, 2, THGROUP_ATT_BANKLEFT);
    CreateThrusterGroup (th_att_rot+2, 2, THGROUP_ATT_BANKRIGHT);
    AddXRExhaust (th_att_rot[0], 1.03, 0.134, _V(-5.1, 0.2,0.4), _V(0, 1,0), rcsExhaustTex);
    AddXRExhaust (th_att_rot[1], 1.03, 0.134, _V( 5.1,-0.8,0.4), _V(0,-1,0), rcsExhaustTex);
    AddXRExhaust (th_att_rot[2], 1.03, 0.134, _V( 5.1, 0.2,0.4), _V(0, 1,0), rcsExhaustTex);
    AddXRExhaust (th_att_rot[3], 1.03, 0.134, _V(-5.1,-0.8,0.4), _V(0,-1,0), rcsExhaustTex);
    
    th_rcs[12] = th_att_lin[0] = CreateThruster (_V(0,0,-7), _V(0,0, 1), GetRCSThrustMax(12), ph_rcs, mainISP);
    th_rcs[13] = th_att_lin[1] = CreateThruster (_V(0,0, 7), _V(0,0,-1), GetRCSThrustMax(13), ph_rcs, mainISP);
    CreateThrusterGroup (th_att_lin,   1, THGROUP_ATT_FORWARD);
    CreateThrusterGroup (th_att_lin+1, 1, THGROUP_ATT_BACK);
    AddXRExhaust (th_att_lin[0], 0.6, 0.078, _V(0,-0.2,-7.6), _V(0,0,-1), rcsExhaustTex);
    AddXRExhaust (th_att_lin[0], 0.6, 0.078, _V(0,0.22,-7.6), _V(0,0,-1), rcsExhaustTex);
    AddXRExhaust (th_att_lin[1], 0.6, 0.078, _V(-0.82,-0.49,9.8), _V(0,0,1), rcsExhaustTex);
    AddXRExhaust (th_att_lin[1], 0.6, 0.078, _V( 0.82,-0.49,9.8), _V(0,0,1), rcsExhaustTex);
    
    // **************** scramjet definitions ********************
    
    VECTOR3 dir = {0.0, sin(SCRAM_DEFAULT_DIR), cos(SCRAM_DEFAULT_DIR)};
    
    const double scramX = 0.9;  // distance from centerline
    for (int i = 0; i < 2; i++) 
    {
        th_scram[i] = CreateThruster (_V(i?scramX:-scramX, 0, -5.6), dir, 0, ph_scram, 0);
        ramjet->AddThrusterDefinition (th_scram[i], SCRAM_FHV[GetXR1Config()->SCRAMfhv],
            SCRAM_INTAKE_AREA, SCRAM_INTERNAL_TEMAX, GetXR1Config()->GetScramMaxEffectiveDMF());
    }
    
    // thrust rating and ISP for scramjet engines are updated continuously
    // move exhaust smoke away from engines a bit
    const double scramDelta = -3;
    PSTREAM_HANDLE ph;
    ph = AddExhaustStream (th_scram[0], _V(-1,-1.1,-5.4 + scramDelta), &exhaust_scram);
    // Note: ph will be null if exhaust streams are disabled
    if (ph != nullptr)
        oapiParticleSetLevelRef (ph, scram_intensity+0);

    ph = AddExhaustStream (th_scram[1], _V( 1,-1.1,-5.4 + scramDelta), &exhaust_scram);
    if (ph != nullptr)
        oapiParticleSetLevelRef (ph, scram_intensity+1);
    
    // ********************* aerodynamics ***********************
    
    // NOTE: org values were causing nasty downward pitch in the atmospehere: 
    // hwing = CreateAirfoil3 (LIFT_VERTICAL, _V(0,0,-0.3), VLiftCoeff, NULL, 5, 90, WING_ASPECT_RATIO);
    hwing = CreateAirfoil3 (LIFT_VERTICAL, _V(m_wingBalance,0,m_centerOfLift), VLiftCoeff, NULL, 5, WING_AREA, WING_ASPECT_RATIO);  // matches DG3 except for -0.15 (DG3=-0.20)

    ReinitializeDamageableControlSurfaces();  // create ailerons, elevators, and elevator trim

    // vertical stabiliser and body lift and drag components
    CreateAirfoil3 (LIFT_HORIZONTAL, _V(0,0,-4), HLiftCoeff, NULL, 5, 15, 1.5);
    CreateControlSurface (AIRCTRL_RUDDER,       0.8, 1.5, _V(   0,0,-7.2), AIRCTRL_AXIS_YPOS, anim_rudder);

    // Create a hidden elevator trim to fix the nose-up tendency on liftoff and allow the elevator trim to be truly neutral.
    // We have to use FLAP here because that is the only unused control surface type.  We could probably also duplicate this via CreateAirfoil3, but this
    // is easier to adjust and test.
    CreateControlSurface(AIRCTRL_FLAP, 0.3, 1.5, _V(   0,0, -7.2), AIRCTRL_AXIS_XPOS);  // no animation for this!
    m_hiddenElevatorTrimState = HIDDEN_ELEVATOR_TRIM_STATE;        // set to a member variable in case we want to change it in flight later during testing

	// we need these goofy const casts to force the linker to link with the 'const double *' version of CreateVariableDragElement instead of the legacy 'double *' version of the function (which still exists but is deprecated and causes warnings in orbiter.log)
    CreateVariableDragElement(const_cast<const double *>(&gear_proc), 0.8, _V(0, -1, 0));		// landing gear
	CreateVariableDragElement(const_cast<const double *>(&rcover_proc), 0.2, _V(0, -0.5, 6.5)); // retro covers
	CreateVariableDragElement(const_cast<const double *>(&nose_proc), 3, _V(0, 0, 8));			// nose cone
	CreateVariableDragElement(const_cast<const double *>(&radiator_proc), 1, _V(0, 1.5, -4));   // radiator
	CreateVariableDragElement(const_cast<const double *>(&brake_proc), 4, _V(0, 0, -8));        // airbrake
    
    SetRotDrag (_V(0.10,0.13,0.04));   // DG3 unchanged

    // define hull temperature limits
    m_hullTemperatureLimits.noseCone  = CTOK(2840);
    m_hullTemperatureLimits.wings     = CTOK(2380);
    m_hullTemperatureLimits.cockpit   = CTOK(1490);
    m_hullTemperatureLimits.topHull   = CTOK(1210);
    m_hullTemperatureLimits.warningFrac  = 0.80;   // yellow text
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
    static VECTOR3 beaconpos[8] = {{-8.6,0,-3.3}, {8.6,0,-3.3}, {0,0.5,-7.5}, {0,2.2,2}, {0,-1.8,2}, {-8.9,2.5,-5.4}, {8.9,2.5,-5.4}};
    static VECTOR3 beaconcol[7] = {{1.0,0.5,0.5}, {0.5,1.0,0.5}, {1,1,1}, {1,0.6,0.6}, {1,0.6,0.6}, {1,1,1}, {1,1,1}};
    for (int i = 0; i < 7; i++) 
    {  
        beacon[i].shape = (i < 3 ? BEACONSHAPE_DIFFUSE : BEACONSHAPE_STAR);
        beacon[i].pos = beaconpos+i;
        beacon[i].col = beaconcol+i;
        beacon[i].size = (i < 3 ? 0.3 : 0.55);
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

    // add a single light at the main engines since they are clustered together
    const double mainEnginePointLightPower = 100;
    const double zMainLightDelta = -1.0;
    if (GetXR1Config()->EnableEngineLightingEffects)
    {
	    LightEmitter *leMain = AddPointLight(_V(0, 0, -10 + zMainLightDelta), mainEnginePointLightPower * 2, 1e-3, 0, 2e-3, col_d, col_s, col_a);
	    leMain->SetIntensityRef(&m_mainThrusterLightLevel);
    }

    // add a light at each hover engine
    if (GetXR1Config()->EnableEngineLightingEffects)
    {
        const double hoverEnginePointLightPower = mainEnginePointLightPower * 0.6875;  // hovers are .6875 the thrust of the mains
        const double yHoverLightDelta = -1.0;
        LightEmitter *leForward      = AddPointLight (_V( 0, -1.6 + yHoverLightDelta,  3.00), hoverEnginePointLightPower, 1e-3, 0, 2e-3, col_d, col_s, col_a); 
        LightEmitter *leAftPort      = AddPointLight (_V( 3, -1.6 + yHoverLightDelta, -4.55), hoverEnginePointLightPower, 1e-3, 0, 2e-3, col_d, col_s, col_a); 
        LightEmitter *leAftStarboard = AddPointLight (_V(-3, -1.6 + yHoverLightDelta, -4.55), hoverEnginePointLightPower, 1e-3, 0, 2e-3, col_d, col_s, col_a); 
	    leForward->SetIntensityRef(&m_hoverThrusterLightLevel);
        leAftPort->SetIntensityRef(&m_hoverThrusterLightLevel);
        leAftStarboard->SetIntensityRef(&m_hoverThrusterLightLevel);
    }

    // add docking lights (our only 2 spotlights for now)
    m_pSpotlights[0] = static_cast<SpotLight *>(AddSpotLight(_V( 2.5,-0.5,6.5), _V(0,0,1), 150, 1e-3, 0, 1e-3, RAD*25, RAD*60, col_white, col_white, col_a));
    m_pSpotlights[1] = static_cast<SpotLight *>(AddSpotLight(_V(-2.5,-0.5,6.5), _V(0,0,1), 150, 1e-3, 0, 1e-3, RAD*25, RAD*60, col_white, col_white, col_a));

    // turn all spotlights off by default
    for (int i=0; i < SPOTLIGHT_COUNT; i++)
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
        oapiBlt (insignia_tex, hTex, 0, 0, 0, 0, 256, 256);
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
	UMmu.DefineAirLockShape(1,   -0.66f,0.66f,-1.65f,0.20f,8.0f,11.0f);  
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
    AddInstrumentPanel(new XR1MainInstrumentPanel1920 (*this), 1920);
    AddInstrumentPanel(new XR1UpperInstrumentPanel1920(*this), 1920);
    AddInstrumentPanel(new XR1LowerInstrumentPanel1920(*this), 1920);

    // 1600-pixel-wide panels
    AddInstrumentPanel(new XR1MainInstrumentPanel1600 (*this), 1600);
    AddInstrumentPanel(new XR1UpperInstrumentPanel1600(*this), 1600);
    AddInstrumentPanel(new XR1LowerInstrumentPanel1600(*this), 1600);

    // 1280-pixel-wide panels
    AddInstrumentPanel(new XR1MainInstrumentPanel1280 (*this), 1280);
    AddInstrumentPanel(new XR1UpperInstrumentPanel1280(*this), 1280);
    AddInstrumentPanel(new XR1LowerInstrumentPanel1280(*this), 1280);
    
    // add our VC panels (panel width MUST be zero for these!)
    AddInstrumentPanel(new XR1VCPilotInstrumentPanel     (*this, PANELVC_PILOT), 0);
    AddInstrumentPanel(new XR1VCPassenger1InstrumentPanel(*this, PANELVC_PSNGR1), 0);
    AddInstrumentPanel(new XR1VCPassenger2InstrumentPanel(*this, PANELVC_PSNGR2), 0);
    AddInstrumentPanel(new XR1VCPassenger3InstrumentPanel(*this, PANELVC_PSNGR3), 0);
    AddInstrumentPanel(new XR1VCPassenger4InstrumentPanel(*this, PANELVC_PSNGR4), 0);



    // NOTE: default crew data is set AFTER the scenario file is parsed
}

// Create control surfaces for any damageable control surface handles below that are zero (all are zero before vessel initialized).
// This is invoked from clbkSetClassCaps as well as ResetDamageStatus.
void DeltaGliderXR1::ReinitializeDamageableControlSurfaces()
{
    if (hElevator == 0)
    {
        // ORG:     CreateControlSurface (AIRCTRL_ELEVATOR,      1.4, 1.5, _V(   0,0,-7.2), AIRCTRL_AXIS_XPOS, anim_elevator);
        // DG3:     CreateControlSurface (AIRCTRL_ELEVATOR,      1.2, 1.4, _V(   0,0,-7.2), AIRCTRL_AXIS_XPOS, anim_elevator);  // matches DG3
        // BETA-1:  CreateControlSurface (AIRCTRL_ELEVATOR,      1.4, 1.4, _V(   0,0,-7.2), AIRCTRL_AXIS_XPOS, anim_elevator);  // COG point matches DG3, but keep stock area
        hElevator = CreateControlSurface2(AIRCTRL_ELEVATOR,      1.2, 1.4, _V(   0,0,-7.2), AIRCTRL_AXIS_XPOS, anim_elevator);  // MATCH DG3 FOR REENTRY TESTING
    }

    if (hLeftAileron == 0)
    {
        // ORG: hLeftAileron = CreateControlSurface2 (AIRCTRL_AILERON, 0.3, 1.5, _V( 7.5,0,-7.2), AIRCTRL_AXIS_XPOS, anim_raileron);
        hLeftAileron = CreateControlSurface2 (AIRCTRL_AILERON, 0.2, 1.5, _V( 7.5,0,-7.2), AIRCTRL_AXIS_XPOS, anim_raileron);  // matches DG3
    }

    if (hRightAileron == 0)
    {
        // ORG: hRightAileron = CreateControlSurface2 (AIRCTRL_AILERON, 0.3, 1.5, _V(-7.5,0,-7.2), AIRCTRL_AXIS_XNEG, anim_laileron);
        hRightAileron = CreateControlSurface2 (AIRCTRL_AILERON, 0.2, 1.5, _V(-7.5,0,-7.2), AIRCTRL_AXIS_XNEG, anim_laileron); // matches DG3
    }
    
    if (hElevatorTrim == 0)
        hElevatorTrim = CreateControlSurface2 (AIRCTRL_ELEVATORTRIM, 0.3, 1.5, _V(   0,0,-7.2), AIRCTRL_AXIS_XPOS, anim_elevatortrim);
}

// --------------------------------------------------------------
// Finalise vessel creation
// --------------------------------------------------------------
void DeltaGliderXR1::clbkPostCreation()
{
    // Invoke XR PostCreation code common to all XR vessels (code is in XRVessel.cpp)
    clbkPostCreationCommonXRCode();

    EnableRetroThrusters(rcover_status == DOOR_OPEN);
    EnableHoverEngines(hoverdoor_status == DOOR_OPEN);
    EnableScramEngines(scramdoor_status == DOOR_OPEN);

    // set initial animation states
    SetXRAnimation (anim_gear, gear_proc);
    SetXRAnimation (anim_rcover, rcover_proc);
    SetXRAnimation (anim_hoverdoor, hoverdoor_proc);
    SetXRAnimation (anim_scramdoor, scramdoor_proc);
    SetXRAnimation (anim_nose, nose_proc);
    SetXRAnimation (anim_ladder, ladder_proc);
    SetXRAnimation (anim_olock, olock_proc);
    SetXRAnimation (anim_ilock, ilock_proc);
    SetXRAnimation (anim_hatch, hatch_proc);
    SetXRAnimation (anim_radiator, radiator_proc);
    SetXRAnimation (anim_brake, brake_proc);
    SetXRAnimation (anim_gearlever, gear_status & 1);
    SetXRAnimation (anim_nconelever, nose_status & 1);
    SetXRAnimation (anim_olockswitch, olock_status & 1);
    SetXRAnimation (anim_ilockswitch, ilock_status & 1);
    SetXRAnimation (anim_retroswitch, rcover_status & 1);
    SetXRAnimation (anim_radiatorswitch, radiator_status & 1);
    SetXRAnimation (anim_hatchswitch, hatch_status & 1);
    SetXRAnimation (anim_ladderswitch, ladder_status & 1);
    
    // NOTE: instrument panel initialization moved to clbkSetClassCaps (earlier) because the Post-2010-P1 Orbiter Beta invokes clbkLoadPanel before invoking clbkPostCreation

    // add our PreStep objects; these are invoked in order
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
    AddPreStep(new NosewheelSteeringPreStep(*this)); 
    AddPreStep(new UpdateVesselLightsPreStep(*this));
	AddPreStep(new ParkingBrakePreStep     (*this));

    // WARNING: this must be invoked LAST in the prestep sequence so that behavior is consistent across all pre-step methods
    AddPreStep(new UpdatePreviousFieldsPreStep(*this));
 
    // add our PostStep objects; these are invoked in order
    AddPostStep(new PreventAutoRefuelPostStep   (*this));   // add this FIRST before our fuel callouts
    AddPostStep(new ComputeAccPostStep          (*this));   // used by acc areas; computed only once per frame for efficiency
    // XRSound: AddPostStep(new AmbientSoundsPostStep       (*this));
    AddPostStep(new ShowWarningPostStep         (*this));
    AddPostStep(new SetHullTempsPostStep        (*this));
    AddPostStep(new SetSlopePostStep            (*this));
    AddPostStep(new DoorSoundsPostStep          (*this));
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
    AddPostStep(new ResetAPUTimerForPolledSystemsPostStep(*this));
    AddPostStep(new ManageMWSPostStep            (*this));
#ifdef _DEBUG
    AddPostStep(new TestXRVesselCtrlPostStep     (*this));      // for manual testing of new XRVesselCtrl methods via the debugger
#endif

    // set hidden elevator trim level
    SetControlSurfaceLevel(AIRCTRL_FLAP, m_hiddenElevatorTrimState);  
}

// save the current Orbiter window coordinates; this is invoked when Orbiter exits or saves a scenario
void DeltaGliderXR1::SaveOrbiterRenderWindowPosition()
{
	// save the Orbiter render coordinates to the registry
	const HWND hOrbiterWnd = GetOrbiterRenderWindowHandle();
	if (hOrbiterWnd)    // will only be NULL for full-screen mode
	{
		// Get window coordinates
		RECT rect;
		if (::GetWindowRect(hOrbiterWnd, &rect))
		{
			// Get saved window coordinates
			CString xCoordValName, yCoordValName;
			xCoordValName.Format("x_window_coord_%u", GetVideoWindowWidth());
			yCoordValName.Format("y_window_coord_%u", GetVideoWindowHeight());
			m_regKeyManager.WriteRegistryDWORD(xCoordValName, rect.left);
			m_regKeyManager.WriteRegistryDWORD(yCoordValName, rect.top);

			char msg[256];
			sprintf(msg, "Saved Orbiter window coordinates x=%d, y=%d", rect.left, rect.top);   // NOTE: these are actually *signed* numbers since the coordinates can go negative with dual monitors.
			m_pConfig->WriteLog(msg);
		}
	}
}

// Move the Orbiter window to its previously saved coordinates.
void DeltaGliderXR1::RestoreOrbiterRenderWindowPosition()
{
	static bool s_isFirstRun = true;		// process-wide singleton

	// Restore the render window coordinates
	const HWND hOrbiterWnd = GetOrbiterRenderWindowHandle();
	if (hOrbiterWnd)    // will only be NULL for full-screen mode
	{
		// See if the restoring Orbiter window coordinates is allowed
		DWORD dwDisableWindowPosRestore = 0;
		m_regKeyManager.ReadRegistryDWORD("DisableWindowPosRestore", dwDisableWindowPosRestore);
		if (s_isFirstRun)	// skip next check silently if this is not the first run
		{
			if (dwDisableWindowPosRestore == 0)
			{
				// Get saved window coordinates
				CString xCoordValName, yCoordValName;
				xCoordValName.Format("x_window_coord_%u", GetVideoWindowWidth());
				yCoordValName.Format("y_window_coord_%u", GetVideoWindowHeight());
				int x, y;   // NOTE: coordinates must be treated as signed integers since they can go negative with dual monitors
				bool bSuccess = m_regKeyManager.ReadRegistryDWORD(xCoordValName, (DWORD &)x);
				bSuccess &= m_regKeyManager.ReadRegistryDWORD(yCoordValName, (DWORD &)y);
				if (bSuccess)
				{
					::SetWindowPos(hOrbiterWnd, 0, static_cast<int>(x), static_cast<int>(y), 0, 0, (SWP_NOSIZE | SWP_NOZORDER));
					char msg[256];
					sprintf(msg, "Restored Orbiter window to coordinates x=%d, y=%d for window size %u x %u", x, y, GetVideoWindowWidth(), GetVideoWindowHeight());
					m_pConfig->WriteLog(msg);
				}
				else
				{
					CString msg;
					msg.Format("No saved Orbiter render window coordinates found for window size %u x %u.", GetVideoWindowWidth(), GetVideoWindowHeight());
					m_pConfig->WriteLog(msg);
				}
			}
			else
			{
				m_pConfig->WriteLog("DisableWindowPosRestore is set in registry; Orbiter render window position will not be restored.");
			}
		}
	}
	s_isFirstRun = false;		// remember for next time
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
        for (int i=0; i < GetXR1Config()->DefaultCrewComplement; i++)
        {
            // can't use const here because of UMmu bug where it uses 'char *' instead of 'const char *'
            CrewMember *pCM = GetXR1Config()->CrewMembers + i;

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
        SetPropellantMass(ph_main,  TANK1_CAPACITY);
        SetPropellantMass(ph_scram, TANK2_CAPACITY);
        SetPropellantMass(ph_rcs,   RCS_FUEL_CAPACITY);

        // must init startup fuel fractions as well (internal tanks only)
        m_startupMainFuelFrac  = GetPropellantMass(ph_main) / GetPropellantMaxMass(ph_main);
        m_startupSCRAMFuelFrac = GetPropellantMass(ph_scram) / GetPropellantMaxMass(ph_scram);
        m_startupRCSFuelFrac   = GetPropellantMass(ph_rcs) / GetPropellantMaxMass(ph_rcs);
        
        // APU on
        ActivateAPU(DOOR_OPENING);

        // RCS off
        SetAttitudeMode(RCS_NONE);

        // Workaround for Orbiter core bug: must init gear parameters here in case gear status not present in the scenario file.
        // This is necessary because Orbiter requires the gear to be DOWN when the scenario first loads if the ship is landed; otherwise, a gruesome crash 
        // occurs due to the "bounce bug".
        gear_status = DOOR_CLOSED;
        gear_proc    = 0.0;
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

        for (int i=0; i < 14; i++)
            SetThrusterIsp(th_rcs[i], mainISP, GetISPScale());
    }

    // log a tertiary HUD message if an override config file was loaded
    static char msg[256];
    if ((*GetXR1Config()->GetOverrideFilename() != 0) && (!GetXR1Config()->ParseFailed()))  // any override set && load succeeded?
    {
        sprintf(msg, "Loaded configuration override file&'%s'.", GetXR1Config()->GetOverrideFilename());
        ShowInfo(NULL, ST_None, msg);
    }

    // log a tertiary HUD message if any scenario overrides found
    if (m_configOverrideBitmask != 0)
    {
        // count the number of '1' bits in the override bitmask
        sprintf(msg, "Loaded %d configuration override(s)&from scenario file.", CountOneBits(m_configOverrideBitmask));
        ShowInfo(NULL, ST_None, msg);
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
        ShowWarning(NULL, ST_None, msg);
    }
}

// --------------------------------------------------------------
// Respond to playback event
// NOTE: do not use spaces in any of these event ID strings.
// --------------------------------------------------------------
bool DeltaGliderXR1::clbkPlaybackEvent (double simt, double event_t, const char *event_type, const char *event)
{
    if (!_stricmp (event_type, "GEAR"))
    {
        ActivateLandingGear (!_stricmp (event, "UP") ? DOOR_CLOSING : DOOR_OPENING);
        return true;
    }
    else if (!_stricmp (event_type, "NOSECONE"))
    {
        ActivateNoseCone (!_stricmp (event, "CLOSE") ? DOOR_CLOSING : DOOR_OPENING);
        return true;
    }
    else if (!_stricmp (event_type, "RCOVER"))
    {
        ActivateRCover (!_stricmp (event, "CLOSE") ? DOOR_CLOSING : DOOR_OPENING);
        return true;
    }
    else if (!_stricmp (event_type, "RADIATOR"))
    {
        ActivateRadiator (!_stricmp (event, "CLOSE") ? DOOR_CLOSING : DOOR_OPENING);
        return true;
    }
    else if (!_stricmp (event_type, "AIRBRAKE"))
    {
        ActivateAirbrake(!_stricmp (event, "CLOSE") ? DOOR_CLOSING : DOOR_OPENING);
        return true;
    }
    else if (!_stricmp (event_type, "HATCH")) 
    {
        ActivateHatch (!_stricmp (event, "CLOSE") ? DOOR_CLOSING : DOOR_OPENING);
        return true;
    } 
    else if (!_stricmp (event_type, "OLOCK"))
    {
        ActivateOuterAirlock (!_stricmp (event, "CLOSE") ? DOOR_CLOSING : DOOR_OPENING);
        return true;
    } 
    else if (!_stricmp (event_type, "ILOCK"))
    {
        ActivateInnerAirlock (!_stricmp (event, "CLOSE") ? DOOR_CLOSING : DOOR_OPENING);
        return true;
    } 
    else if (!_stricmp (event_type, "LADDER"))
    {
        ActivateLadder (!_stricmp (event, "CLOSE") ? DOOR_CLOSING : DOOR_OPENING);
        return true;
    } 
    else if (!_stricmp (event_type, "APU")) 
    {
        ActivateAPU(!_stricmp (event, "CLOSE") ? DOOR_CLOSING : DOOR_OPENING);
        return true;
    }
    else if (!_stricmp (event_type, "HOVERDOORS")) 
    {
        ActivateHoverDoors(!_stricmp (event, "CLOSE") ? DOOR_CLOSING : DOOR_OPENING);
        return true;
    }
    else if (!_stricmp (event_type, "SCRAMDOORS")) 
    {
        ActivateScramDoors(!_stricmp (event, "CLOSE") ? DOOR_CLOSING : DOOR_OPENING);
        return true;
    }
    else if (!_stricmp (event_type, "BAYDOORS")) 
    {
        ActivateBayDoors(!_stricmp (event, "CLOSE") ? DOOR_CLOSING : DOOR_OPENING);
        return true;
    }
    else if (!_stricmp (event_type, "CHAMBER")) 
    {
        ActivateChamber((!_stricmp (event, "CLOSE") ? DOOR_CLOSING : DOOR_OPENING), true);  // OK to force here, although it shouldn't be necessary
        return true;
    }
    // new for the XR1-1.9 release group
    else if (!_stricmp (event_type, "NAVLIGHT")) 
    {
        SetNavlight(!_stricmp (event, "ON"));  // true = light on
        return true;
    }
    else if (!_stricmp (event_type, "BEACONLIGHT")) 
    {
        SetBeacon(!_stricmp (event, "ON"));  // true = light on
        return true;
    }
    else if (!_stricmp (event_type, "STROBELIGHT")) 
    {
        SetStrobe(!_stricmp (event, "ON"));  // true = light on
        return true;
    }
    else if (!_stricmp (event_type, "RESETMET")) 
    {
        ResetMET();   // event not used for this
        return true;
    }
    else if (!_stricmp (event_type, "XFEED")) 
    {
        XFEED_MODE mode;
        if (!_stricmp (event, "MAIN"))
            mode = XF_MAIN;
        else if (!_stricmp (event, "RCS"))
            mode = XF_RCS;
        else if (!_stricmp (event, "OFF"))
            mode = XF_OFF;
        else  // invalid mode, so ignore it
        {
            _ASSERTE(false);
            return false;
        }
        SetCrossfeedMode(mode, NULL);   // no optional message for this
        return true;
    }
    else if (!_stricmp (event_type, "MAINDUMP")) 
    {
        m_mainFuelDumpInProgress = (!strcmp(event, "ON"));
        return true;
    }
    else if (!_stricmp (event_type, "RCSDUMP")) 
    {
        m_rcsFuelDumpInProgress = (!strcmp(event, "ON"));
        return true;
    }
    else if (!_stricmp (event_type, "SCRAMDUMP")) 
    {
        m_scramFuelDumpInProgress = (!strcmp(event, "ON"));
        return true;
    }
    else if (!_stricmp (event_type, "APUDUMP")) 
    {
        m_apuFuelDumpInProgress = (!strcmp(event, "ON"));
        return true;
    }
    else if (!_stricmp (event_type, "LOXDUMP")) 
    {
        m_loxDumpInProgress = (!strcmp(event, "ON"));
        return true;
    }

    return false;
}

// --------------------------------------------------------------
// Create DG visual
// --------------------------------------------------------------
void DeltaGliderXR1::clbkVisualCreated (VISHANDLE vis, int refcount)
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
void DeltaGliderXR1::clbkVisualDestroyed (VISHANDLE vis, int refcount)
{
    exmesh = nullptr;
    vcmesh = nullptr;
}

// --------------------------------------------------------------
// PreStep Frame update; necessary to kill controls if ship crashed
// --------------------------------------------------------------
void DeltaGliderXR1::clbkPreStep (double simt, double simdt, double mjd)
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
void DeltaGliderXR1::clbkPostStep (double simt, double simdt, double mjd)
{
    // update VC warning lights
    UpdateVCStatusIndicators();

    // Invoke our superclass handler so our poststep Area and PostStep objects are executed
    VESSEL3_EXT::clbkPostStep(simt, simdt, mjd);
}

// ==============================================================
// End overloaded callback functions
// ==============================================================

// invoked during vessel initialization
// Returns: true if init successful, false if XRSound not loaded
bool DeltaGliderXR1::InitSound()
{
#if 0 // {XXX} ONLY SET THIS TO 1 IF YOU WANT TO TEST XRSOUND 2.0'S MODULE FUNCTIONALITY
#ifndef _DEBUG 
#error INVALID CONFIGURATION: fix your XRVessel.cpp's InitSound()
#endif
    m_pXRSound = XRSound::CreateInstance("XRSoundModuleTesting");
#else
	m_pXRSound = XRSound::CreateInstance(this);
#endif

    // check that XRSound is installed and warn the user if it is not
    if (!m_pXRSound->IsPresent())
    {
        // Note: do not blink a warning on the HUD or the debug line here because some users may want to 
        // fly without XRSound loaded.
        GetXR1Config()->WriteLog("WARNING: XRSound not installed or is a different XRSound.dll version from what this XR vessel version was built with: custom sound effects will not play.");
        ShowWarning(NULL, DeltaGliderXR1::ST_None, "WARNING: XRSound not installed!&Custom sounds will not play.", true);  // warn the user
        return false;
    }

    // write the XRSound version to the log
    float xrSoundVersion = m_pXRSound->GetVersion();
    char msg[256];
    sprintf(msg, "Using XRSound version: %.2f", xrSoundVersion);
    GetXR1Config()->WriteLog(msg);

    // disable any default XRSounds that we implement ourselves here via code
    XRSoundOnOff(XRSound::AudioGreeting, false);
    XRSoundOnOff(XRSound::SwitchOn, false);
    XRSoundOnOff(XRSound::SwitchOff, false);
    
    XRSoundOnOff(XRSound::Rotation, false);
    XRSoundOnOff(XRSound::Translation, false);
    XRSoundOnOff(XRSound::Off, false);
    
    XRSoundOnOff(XRSound::AFOff, false);
    XRSoundOnOff(XRSound::AFPitch, false);
    XRSoundOnOff(XRSound::AFOn, false);

    XRSoundOnOff(XRSound::Crash, false);
    XRSoundOnOff(XRSound::MetalCrunch, false);
    XRSoundOnOff(XRSound::Touchdown, false);
    XRSoundOnOff(XRSound::OneHundredKnots, false);
    XRSoundOnOff(XRSound::Liftoff, false);
    XRSoundOnOff(XRSound::WheelChirp, false);
    XRSoundOnOff(XRSound::WheelStop, false);
    XRSoundOnOff(XRSound::TiresRolling, false);
    XRSoundOnOff(XRSound::WarningGearIsUp, false);
    XRSoundOnOff(XRSound::YouAreClearedToLand, false);
    
    XRSoundOnOff(XRSound::MachCalloutsGroup, false);
    XRSoundOnOff(XRSound::AltitudeCalloutsGroup, false);
    XRSoundOnOff(XRSound::DockingDistanceCalloutsGroup, false);

    XRSoundOnOff(XRSound::DockingCallout, false);
    XRSoundOnOff(XRSound::UndockingCallout, false);

    XRSoundOnOff(XRSound::AutopilotOn, false);
    XRSoundOnOff(XRSound::AutopilotOff, false);

    XRSoundOnOff(XRSound::SubsonicCallout, false);
    XRSoundOnOff(XRSound::SonicBoom, false);

    // load sounds
    LoadXR1Sound(SwitchOn,               "SwitchOn1.wav",     XRSound::PlaybackType::InternalOnly);
    LoadXR1Sound(SwitchOff,              "SwitchOff1.wav",    XRSound::PlaybackType::InternalOnly);
    LoadXR1Sound(Off,                    "Off.wav",           XRSound::PlaybackType::Radio);
    LoadXR1Sound(Rotation,               "Rotation.wav",      XRSound::PlaybackType::Radio); // so it's always audible outside the ship 
    LoadXR1Sound(Translation,            "Translation.wav",   XRSound::PlaybackType::Radio); // so it's always audible outside the ship 
    LoadXR1Sound(Error1,                 "Error1.wav",        XRSound::PlaybackType::Radio); // so it's always audible outside the ship (just in case we use it for something)
    LoadXR1Sound(OneHundredKnots,        "100 Knots.wav",     XRSound::PlaybackType::Radio);
    LoadXR1Sound(V1,                     "V1.wav",            XRSound::PlaybackType::Radio);
    LoadXR1Sound(Rotate,                 "Rotate.wav",        XRSound::PlaybackType::Radio);
    LoadXR1Sound(GearUp,                 "Gear Up.wav",       XRSound::PlaybackType::Radio);     // 10
    LoadXR1Sound(GearDown,               "Gear Down.wav",     XRSound::PlaybackType::Radio);
    // GearLocked reused on-the-fly
    LoadXR1Sound(Pitch,                  "Pitch.wav",         XRSound::PlaybackType::Radio);
    LoadXR1Sound(On,                     "On.wav",            XRSound::PlaybackType::Radio);
    LoadXR1Sound(BeepHigh,               "BeepHigh.wav",      XRSound::PlaybackType::InternalOnly);
    LoadXR1Sound(BeepLow,                "BeepLow.wav",       XRSound::PlaybackType::InternalOnly);
    LoadXR1Sound(AutopilotOn,            "Autopilot On.wav",  XRSound::PlaybackType::InternalOnly);
    LoadXR1Sound(AutopilotOff,           "Autopilot Off.wav", XRSound::PlaybackType::InternalOnly);
    LoadXR1Sound(RetroDoorsAreClosed,    "Retro doors are closed.wav", XRSound::PlaybackType::InternalOnly);  
    // slot 20 = MachCallout
    // slot 21 = AltitudeCallout
    LoadXR1Sound(SonicBoom,              "Sonic Boom.wav",    XRSound::PlaybackType::BothViewFar); 
    // slot 23 = Ambient    (no longer used here; XRSound handles it)
    // slot 24 = Warning
    // slot 25 = Info
    LoadXR1Sound(ScramJet,               "ScramJet.wav",           XRSound::PlaybackType::BothViewFar);
    LoadXR1Sound(WarningBeep,            "Warning Beep.wav",       XRSound::PlaybackType::InternalOnly);
    LoadXR1Sound(GearWhine,              "Gear Whine.wav",         XRSound::PlaybackType::InternalOnly);
    LoadXR1Sound(GearLockedThump,        "Gear Locked Thump.wav",  XRSound::PlaybackType::InternalOnly);
    LoadXR1Sound(Crash,                  "Crash.wav",              XRSound::PlaybackType::BothViewFar);
    LoadXR1Sound(ErrorSoundFileMissing,  "Error Sound File Missing.wav",  XRSound::PlaybackType::BothViewFar);  // debugging only
    LoadXR1Sound(FuelResupply,           "Fuel Flow.wav",          XRSound::PlaybackType::InternalOnly);  
    LoadXR1Sound(FuelCrossFeed,          "Fuel Flow.wav",          XRSound::PlaybackType::InternalOnly);  
    LoadXR1Sound(FuelDump,               "Fuel Flow.wav",          XRSound::PlaybackType::InternalOnly);
    LoadXR1Sound(SupplyHatch,            "Door Opened Thump.wav",  XRSound::PlaybackType::InternalOnly);
    LoadXR1Sound(HoverDoorsAreClosed,    "Hover doors are closed.wav",  XRSound::PlaybackType::InternalOnly);
    LoadXR1Sound(ScramDoorsAreClosed,    "SCRAM doors are closed.wav",  XRSound::PlaybackType::InternalOnly);
    LoadXR1Sound(Chamber,                "Airlock.wav",                 XRSound::PlaybackType::InternalOnly);
    LoadXR1Sound(WheelChirp,             "Wheel Chirp.wav",         XRSound::PlaybackType::BothViewClose);
    LoadXR1Sound(TiresRolling,           "Tires Rolling.wav",       XRSound::PlaybackType::BothViewClose);

    return true;
}

//-------------------------------------------------------------------------
// Verify that a manual COG shift is available and play a warning beep and a voice callout if it is not.
// Returns: true if manual COG OK, false if locked or offline
//-------------------------------------------------------------------------
bool DeltaGliderXR1::VerifyManualCOGShiftAvailable()
{
    bool retCode = true;

    // can't move unless the APU is online
    if (CheckHydraulicPressure(false, true) == false)  // play error beep, but no wav for this; we will handle it below
    {
        PlayErrorBeep();
        ShowWarning("Warning Center of Gravity Shift Offline.wav", ST_WarningCallout, "Warning: APU offline; cannot&shift the center of gravity.");
        retCode = false;
    }
    else if (m_customAutopilotMode == AP_ATTITUDEHOLD)
    {
        PlayErrorBeep();
        ShowWarning("Locked by Attitude Hold.wav", ST_WarningCallout, "Center of Gravity shift locked&by Attitude Hold Autopilot.");
        retCode = false;
    }
    else if (m_cogShiftAutoModeActive)
    {
        PlayErrorBeep();
        ShowWarning("Locked by Auto Mode.wav", ST_WarningCallout, "Center of Gravity shift locked&by AUTO Mode.");
        retCode = false;
    }

    return retCode;
}

//-------------------------------------------------------------------------
// Shift the center-of-lift by a requested amount, verifying that the APU
// is running first.  
// 
// WARNING: this does NOT show a warning to the user if the APU is offline;
// it is the caller's responsibility to decide how to handle that.
//
// requestedShift = requested delta in meters from the current center-of-lift
// 
// Returns: true if shift successful, or false if the shift range was maxed out or 
// APU is offline.
//-------------------------------------------------------------------------
bool DeltaGliderXR1::ShiftCenterOfLift(double requestedShift)
{
    // the caller should have already checked this, but let's make sure...
    if (CheckHydraulicPressure(false, false) == false)   // no sound here
        return false;

    // unstable during reentry: requestedShift /= oapiGetTimeAcceleration();

    bool retVal = true;     // assume success
    m_centerOfLift += requestedShift;

    // never exceed the maximum shift allowed
    const double shiftDelta = m_centerOfLift - NEUTRAL_CENTER_OF_LIFT;
    if (shiftDelta < -COL_MAX_SHIFT_DISTANCE)
    {
        m_centerOfLift = NEUTRAL_CENTER_OF_LIFT - COL_MAX_SHIFT_DISTANCE;
        retVal = false;   // maxed out
    }
    else if (shiftDelta > COL_MAX_SHIFT_DISTANCE)
    {
        m_centerOfLift = NEUTRAL_CENTER_OF_LIFT + COL_MAX_SHIFT_DISTANCE;
        retVal = false;   // maxed out
    }

    EditAirfoil(hwing, 0x01, _V(m_wingBalance, 0, m_centerOfLift), NULL, 0, 0, 0);
    // debug: sprintf(oapiDebugString(), "requestedColShift=%lf, m_centerOfLift=%lf", requestedShift, m_centerOfLift);  

    MarkAPUActive();  // reset the APU idle warning callout time

    return retVal;
}

// Used for internal development testing only to tweak some internal value.
// This is invoked from the key handler as ALT-1 or ALT-2 are held down.  
// direction = true: increment value, false: decrement value
void DeltaGliderXR1::TweakInternalValue(bool direction)
{
// {ZZZ} TweakInternalValue
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
    // bump SCRAM gimbal up or down
    const double stepSize = (oapiGetSimStep() * 0.2);
    const double step = stepSize * (direction ? 1.0 : -1.0);

    XREngineStateRead state;
    GetEngineState(XRE_ScramLeft, state);   // populate the structure

    // bump the gimbal
    state.GimbalY += step;
    sprintf(oapiDebugString(), "GimbalY=%lf", state.GimbalY);  

    SetEngineState(XRE_ScramLeft, state);
#endif
#endif  // ifdef DEBUG
}

// Turn secondary HUD OFF
void DeltaGliderXR1::DisableSecondaryHUD()
{
    m_lastSecondaryHUDMode = m_secondaryHUDMode;  // remember mode for next reactivation
    m_secondaryHUDMode = 0; // turn HUD off
}

// Turn secondary HUD ON (if off), and set the mode
void DeltaGliderXR1::EnableAndSetSecondaryHUDMode(int mode)
{
    m_secondaryHUDMode = mode;
    
    PlaySound(SwitchOn, ST_Other, QUIET_CLICK);
    TriggerRedrawArea(AID_SECONDARY_HUD_BUTTONS);
}

// Set tertiary HUD on or off
void DeltaGliderXR1::SetTertiaryHUDEnabled(bool on)
{
    m_tertiaryHUDOn = on;

    PlaySound(SwitchOn, ST_Other, QUIET_CLICK);
    TriggerRedrawArea(AID_TERTIARY_HUD_BUTTON);
}

// hook whenever the 2D panel changes
bool DeltaGliderXR1::clbkLoadPanel(int panelID)
{ 
    m_lastActive2DPanelID = panelID; 
    return VESSEL3_EXT::clbkLoadPanel(panelID);  
}

// handle the Altea Aerospace logo click easter egg 
void DeltaGliderXR1::AlteaLogoClicked()
{
    // this callout file is camouflaged
    ShowInfo("ambl.wav", ST_Other, NULL);  // no text message for this; always play it (ST_Other)
}

// plays "Gear up and locked" or "Gear down and locked"
void DeltaGliderXR1::PlayGearLockedSound(bool isGearUp)
{
    const char *pFilename = (isGearUp ? "Gear Up And Locked.wav" : "Gear Down And Locked.wav");
    LoadXR1Sound(GearLocked, pFilename,  XRSound::PlaybackType::Radio); 
    PlaySound(GearLocked, ST_InformationCallout);
}

// NOTE: crew is treated as incapacitated if no one is on board!
// Returns true if crew is dead or cannot operate the ship, or false if at least one member is OK and can pilot the ship.
bool DeltaGliderXR1::IsCrewIncapacitatedOrNoPilotOnBoard() const
{ 
    // normal checks first
    bool retVal = IsCrewIncapacitated();

    // check whether a pilot must be on board in order to fly the ship
    if (GetXR1Config()->RequirePilotForShipControl)
    {
        // check for 'Commander' and 'Pilot' ranks
        if (!IsPilotOnBoard())
            retVal = true;    // nobody on board who can fly the ship
    }
    
    return retVal;
}

// Returns true if a pilot is on board *or* 'RequirePilotForShipControl=false' and at least
// *one* crew member is on board AND the crew is OK.
bool DeltaGliderXR1::IsPilotOnBoard() const
{ 
#ifdef MMU
    // normal checks first
    bool retVal = (IsCrewRankOnBoard("Commander") || IsCrewRankOnBoard("Pilot")); 

    // If 'RequirePilotForShipControl=false' the ship is *always* flyable as long as at least
    // on crew member is on board and the crew is still OK.
    if (GetXR1Config()->RequirePilotForShipControl == false)
        retVal = ((GetCrewMembersCount() > 0) && (m_crewState == OK));

    return retVal;
#else
    return true;
#endif
}

// Returns true if one or more crew members with the specified rank are on board, false otherwise
// pTargetRank is case-sensitive; e.g., "Commander"
bool DeltaGliderXR1::IsCrewRankOnBoard(const char *pTargetRank) const
{
#ifdef MMU
    bool retVal = false;   // assume not on board
    char pRank[CrewMemberRankLength+1];
    for (int i=0; i < MAX_PASSENGERS; i++) 
    {
        const char *pUmmuMisc = CONST_UMMU(this).GetCrewMiscIdBySlotNumber(i);
        const bool crewMemberOnBoard = (strlen(pUmmuMisc) > 0);

        if (crewMemberOnBoard)
        {
            // check the crew member's rank (case-sensitive)
            strncpy(pRank, RetrieveRankForUMmuMisc(pUmmuMisc), CrewMemberRankLength);
            // check for commander and pilot by RANK (case-sensitive)
            if (strcmp(pRank, pTargetRank) == 0)
            {
                retVal = true;
                break;
            }
        }
    }
    return retVal;
#else
    return true;
#endif
}

//-------------------------------------------------------------------------

// Gimbal SCRAM engine pitch
void DeltaGliderXR1::GimbalSCRAMPitch(const GIMBAL_SWITCH which, const DIRECTION dir)
{
    if (dir == DIR_NONE)
        return;   // nothing to do

    // warn the user if APU is offline
    if (CheckHydraulicPressure(true, true) == false)
        return;

    VECTOR3 dirVec;
    double phi, dphi = oapiGetSimStep() * SCRAM_GIMBAL_SPEED * (dir == UP_OR_LEFT ? -1.0 : 1.0);

    for (int i = 0; i < 2; i++)  // process both switches
    {
        if ((i == which) || (which == BOTH))   // is FOO or BOTH switches pressed?
        {
            GetThrusterDir(th_scram[i], dirVec);  
            phi = atan2 (dirVec.y, dirVec.z);
            phi = min (SCRAM_DEFAULT_DIR + SCRAM_GIMBAL_RANGE, max (SCRAM_DEFAULT_DIR - SCRAM_GIMBAL_RANGE, phi+dphi));
            SetThrusterDir(th_scram[i], _V(0, sin(phi), cos(phi)));

            MarkAPUActive();  // reset the APU idle warning callout time
        }
    }
}

//-------------------------------------------------------------------------

void DeltaGliderXR1::GimbalMainPitch(const GIMBAL_SWITCH which, const DIRECTION dir)
{
    if (dir == DIR_NONE)
        return;   // nothing to do

    // warn the user if APU is offline
    if (CheckHydraulicPressure(true, true) == false)
        return;

    VECTOR3 dirVec;
    double dy = oapiGetSimStep() * MAIN_PGIMBAL_SPEED * (dir == UP_OR_LEFT ? -1.0 : 1.0);

    for (int i = 0; i < 2; i++)  // process both switches
    {
        if ((i == which) || (which == BOTH))   // is FOO or BOTH switches pressed?
        {
            GetThrusterDir(th_main[i], dirVec);
            dirVec /= dirVec.z;
            dirVec.y = min (MAIN_PGIMBAL_RANGE, max (-MAIN_PGIMBAL_RANGE, dirVec.y+dy));
            SetThrusterDir(th_main[i], dirVec);

            MarkAPUActive();  // reset the APU idle warning callout time
        } 
    }
}

//-------------------------------------------------------------------------

void DeltaGliderXR1::GimbalMainYaw(const GIMBAL_SWITCH which, const DIRECTION dir)
{
    if (dir == DIR_NONE)
        return;   // nothing to do

    // warn the user if APU is offline
    if (CheckHydraulicPressure(true, true) == false)
        return;

    VECTOR3 dirVec;
    double dx = oapiGetSimStep() * MAIN_YGIMBAL_SPEED * (dir == UP_OR_LEFT ? 1.0 : -1.0);

    for (int i = 0; i < 2; i++)  // process both switches
    {
        if ((which == i) || (which == BOTH))  // is switch #i pressed?
        {
            GetThrusterDir(th_main[i], dirVec);
            dirVec /= dirVec.z;
            dirVec.x = min (MAIN_YGIMBAL_RANGE, max (-MAIN_YGIMBAL_RANGE, dirVec.x+dx));
            SetThrusterDir(th_main[i], dirVec);
            
            MarkAPUActive();  // reset the APU idle warning callout time
        } 
    }

}

//-------------------------------------------------------------------------

void DeltaGliderXR1::ShiftHoverBalance(const GIMBAL_SWITCH which, const DIRECTION dir)
{
    if (dir == DIR_NONE)
        return;   // nothing to do

    // warn the user if APU is offline
    if (CheckHydraulicPressure(true, true) == false)
        return;

    double shift = oapiGetSimStep() * HOVER_BALANCE_SPEED * (dir == UP_OR_LEFT ? 1.0 : -1.0);  // shift as a fraction of balance for this timestep
    m_hoverBalance += shift;       // adjust the balance

    // keep in range
    if (m_hoverBalance > MAX_HOVER_IMBALANCE)
        m_hoverBalance = MAX_HOVER_IMBALANCE;
    else if (m_hoverBalance < -MAX_HOVER_IMBALANCE)
        m_hoverBalance = -MAX_HOVER_IMBALANCE;

    // NOTE: must take damage into account here!
    const int hoverThrustIdx = GetXR1Config()->HoverEngineThrust;
    const double maxThrustFore = MAX_HOVER_THRUST[hoverThrustIdx] * GetDamageStatus(HoverEngineFore).fracIntegrity;
    const double maxThrustAft  = MAX_HOVER_THRUST[hoverThrustIdx] * GetDamageStatus(HoverEngineAft).fracIntegrity;

    SetThrusterMax0(th_hover[0], maxThrustFore * (1.0 + m_hoverBalance));
    SetThrusterMax0(th_hover[1], maxThrustAft *  (1.0 - m_hoverBalance));

    MarkAPUActive();  // reset the APU idle warning callout time
}

//------------------------------------------------------------------------

// gimbal recenter ALL engines
void DeltaGliderXR1::GimbalRecenterAll()
{
    m_mainPitchCenteringMode = m_mainYawCenteringMode = m_scramCenteringMode = true;
}

//------------------------------------------------------------------------

// mate: NULL = undocking event, otherwise vessel handle @ the docking port
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
        if (nose_status != DOOR_OPEN)
            Undock(dock);   // undo the dock
    }
    */
}

//------------------------------------------------------------------------

#ifdef MMU
// deploy a new instance of the currently-selected turbopack
void DeltaGliderXR1::DeployTurbopack()
{
    if (CheckEVADoor() == false)
        return;     // cannot deploy turbopack

    const Turbopack *pSelectedTurbopack = TURBOPACKS_ARRAY + m_selectedTurbopack;

    // WARNING: TURBOPACK VESSEL NAMES MUST BE UNIQUE!
    // Define the new vessel's name as: vesselClassname-index; e.g., XR2turbopackKara-1
    // Loop until we find a unique name.
    char childName[128];
    for (int subIndex = 1; subIndex < 10000; subIndex++)   // 10000 is for sanity check
    {
        sprintf(childName, "%s-%d", pSelectedTurbopack->Classname, subIndex);

        // check whether vessel already exists
        OBJHANDLE hExistingVessel = oapiGetVesselByName(childName);
        if (oapiIsVessel(hExistingVessel) == false)
            break;      // name is unique in this scenario
    }

    // Clone from our vessel's status initially.
    VESSELSTATUS2 childVS;
    GetStatusSafe(*this, childVS, true);  // reset fields

    // move the child (turbopack) to the deployToCoordinates by converting them (as a delta) from parent-local to GLOBAL coordinates
    VECTOR3 globalChildDeltaCoords;
    GlobalRot(TURBOPACK_SPAWN_COORDINATES, globalChildDeltaCoords);
    childVS.rpos += globalChildDeltaCoords;
    childVS.status = 0;                  // set to FREEFLIGHT

    OBJHANDLE hChild = oapiCreateVesselEx(childName, pSelectedTurbopack->Classname, &childVS);
    if (hChild == nullptr)
    {
        // should never happen!
        PlayErrorBeep();
        ShowWarning(NULL, DeltaGliderXR1::ST_None, "Warning: turbopack vessel&creation failed!");
        return;
    }

    // move the turbopack to its deploy location
    VESSEL *pChild = oapiGetVesselInterface(hChild);
    pChild->DefSetStateEx(&childVS);      

    char temp[64];
    sprintf(temp, "%s deployed.", childName);
    ShowInfo("BeepHigh.wav", ST_Other, temp);
}

//------------------------------------------------------------------------

// stow all turbopacks within STOW_TURBOPACK_DISTANCE meters of the ship
void DeltaGliderXR1::StowAllTurbopacks()
{
    if (CheckEVADoor() == false)
        return;     // cannot stow turbopack

    int stowedCount = 0;    // # of turbopacks stowed

    // loop through all vessels in the sim and check each vessel's classname and distance
    const DWORD dwVesselCount = oapiGetVesselCount();
    for (DWORD i=0; i < dwVesselCount; i++)
    {
        OBJHANDLE hVessel = oapiGetVesselByIndex(i);
        if (hVessel != nullptr)    // should never happen, but just in case
        {
            VESSEL *pVessel = oapiGetVesselInterface(hVessel);
            const char *pClassname = pVessel->GetClassName();
            // WARNING: some vessel classnames can be null, such as Mir!
            if (pClassname != nullptr)
            {
                // check this vessel's distance from our vessel
                const double candidateVesselDistance = GetDistanceToVessel(*pVessel);
                if (candidateVesselDistance <= STOW_TURBOPACK_DISTANCE)
                {
                    // candidate vessel is in range; check its class for a match with one of our turbopack types
                    for (int i=0; i < TURBOPACKS_ARRAY_SIZE; i++)
                    {
                        const Turbopack *pTurbopack = TURBOPACKS_ARRAY + i;
                        if (strcmp(pClassname, pTurbopack->Classname) == 0)
                        {
                            // classname is a match!  Delete ("stow") the vessel.
                            oapiDeleteVessel(hVessel);
                            stowedCount++;
                        }
                    }
                }
            }
        }
    }

    if (stowedCount == 0)
    {
        PlayErrorBeep();
        ShowWarning(NULL, DeltaGliderXR1::ST_None, "No turbopacks in range.");
    }
    else
    {
        char temp[64];
        const char *s = ((stowedCount == 1) ? "" : "s");
        sprintf(temp, "%d turbopack%s stowed.", stowedCount, s);
        ShowInfo("BeepHigh.wav", ST_Other, temp);
    }
}
#endif

// Note: this is used only by subclasses; it is not used by the XR1, although it is invoked by our key handler.
// toggle the payload editor dialog on/off
// Do not declare this 'const' because we are passing 'this' to a windows message proc.
void DeltaGliderXR1::TogglePayloadEditor()
{
    // sanity check
    if (m_pPayloadBay == nullptr)
        return;
    
    if (s_hPayloadEditorDialog != 0)
    {
        // editor is open: close it
        PlaySound(BeepLow, DeltaGliderXR1::ST_Other);
        ::SendMessage(s_hPayloadEditorDialog, WM_TERMINATE, 0, reinterpret_cast<LPARAM>(this));
        s_hPayloadEditorDialog = 0;
    }
    else
    {
        // editor is closed: open it
        PlaySound(BeepHigh, DeltaGliderXR1::ST_Other);
        s_hPayloadEditorDialog = XR1PayloadDialog::Launch(GetHandle()); 
    }
}

// returns the total payload mass in KG
double DeltaGliderXR1::GetPayloadMass() const
{
    if (m_pPayloadBay == nullptr)
        return 0;       // no payload bay for this vessel

    // if cheatcode is set, use it instead of the actual payload mass
    if (CARGO_MASS != -1.0)      // use exact match here instead of '< 0' so the users can cheat and make payload mass *negative* if the want to
        return CARGO_MASS;  

    return m_pPayloadBay->GetPayloadMass();
}

//
// Fuel/LOX quantity methods; these take any payload bay consumables into account
//

// returns the propellant type for the given propellant handle
// ph may not be null!
PROP_TYPE DeltaGliderXR1::GetPropTypeForHandle(PROPELLANT_HANDLE ph) const
{
    PROP_TYPE pt;

    // check each of our known propellant handle values
    if (ph == ph_main)
        pt = PT_Main;
    else if (ph == ph_scram)
        pt = PT_SCRAM;
    else if (ph == ph_rcs)
        pt = PT_NONE;  // no separate fuel tank for this
    else
    {
        // should never happen!
        _ASSERTE(false);
        pt = PT_NONE;
    }

    return pt;
}

// Returns the max capacity of this propellant, including payload tank(s) capacity.
//
// WARNING: this may return zero depending on how a given vessel configures its fuel tanks!
// If you need to divide by this return value, use the SAFE_FRACTION global inline method,
// which returns zero if the denominator is zero.  Of course, that macro should only be used
// for gauges where you need to render a percentage of fuel in the tank; i.e., if the max
// mass == 0, then the tank is always empty (0).
double DeltaGliderXR1::GetXRPropellantMaxMass(PROPELLANT_HANDLE ph) const
{
    double totalMass = oapiGetPropellantMaxMass(ph);
    if (m_pPayloadBay != nullptr)
    {
        const PROP_TYPE pt = GetPropTypeForHandle(ph);
        if (pt != PT_NONE)   // no extra capacity for RCS
            totalMass += m_pPayloadBay->GetPropellantMaxMass(pt);
    }
    
    return totalMass;
}

// returns the current quantity of this propellant, including payload tank(s) capacity
double DeltaGliderXR1::GetXRPropellantMass(PROPELLANT_HANDLE ph) const
{
    return (oapiGetPropellantMass(ph) + GetXRBayPropellantMass(ph));
}

// returns the current quantity of this propellant in the payload bay tanks *only*
double DeltaGliderXR1::GetXRBayPropellantMass(PROPELLANT_HANDLE ph) const
{
    double bayPropMass = 0;
    if (m_pPayloadBay != nullptr)
    {
        const PROP_TYPE pt = GetPropTypeForHandle(ph);
        if (pt != PT_NONE)   // no extra capacity for RCS
            bayPropMass = m_pPayloadBay->GetPropellantMass(pt);
    }
    
    return bayPropMass;
}


// sets propellant quantity, including payload tank(s).
// Note: internal tanks are always filled *first*
void DeltaGliderXR1::SetXRPropellantMass(PROPELLANT_HANDLE ph, const double mass)
{
    double deltaRemaining = mass;
    
    // fill the internal tank first
    const double internalTankQty = min(mass, oapiGetPropellantMaxMass(ph));
    SetPropellantMass(ph, internalTankQty);
    deltaRemaining -= internalTankQty; 

    // now store any remainder in the payload bay, if any bay exists
    if (m_pPayloadBay != nullptr)
    {
        // get the delta between the current payload bay quantity and the new quantity
        const PROP_TYPE pt = GetPropTypeForHandle(ph);
        if (pt != PT_NONE)   // no extra capacity for RCS
        {
            const double bayDeltaRequested = deltaRemaining - m_pPayloadBay->GetPropellantMass(pt);  // newBayMass - currentBayMass

            // apply the delta (as a *request*) to the bay tanks
            const double bayDeltaApplied = AdjustBayPropellantMassWithMessages(pt, bayDeltaRequested);

            // if the caller's code is correct, we should never overflow the bay quantity
            // Need to account for slight rounding error possibility here in the nth decimal place, so 0.01 is way overkill, but fine for an assert
            _ASSERTE(fabs(bayDeltaApplied - bayDeltaRequested) < 0.01);
        }
    }
    else 
    {
        // no payload bay, so it should all fit in the internal tank!
        // Need to account for slight rounding error possibility here in the nth decimal place, so 0.01 is way overkill, but fine for an assert
        _ASSERTE(fabs(deltaRemaining) < 0.01);
    }
}

// Adjust the propellant mass in the bay, displaying information messages if 
// bay slots empty or fill.
//
// Returns: quantity of propellant/LOX added to or removed from bay (negative = removed)
double DeltaGliderXR1::AdjustBayPropellantMassWithMessages(const PROP_TYPE pt, const double requestedFlowQty)
{
    // let's be efficient here...
    if (requestedFlowQty == 0)
        return 0;  

    const XRPayloadBay::SlotsDrainedFilled &slotsDrainedFilled = m_pPayloadBay->AdjustPropellantMass(pt, requestedFlowQty);

    // Note: although it is possible that multiple bay tanks will fill or empty here, it is
    // highly unlikey that more than one, or at the most, two, slots will fill or drain within
    // the same timestep.  Therefore, we won't worry about handling 32 slots numbers all at once
    // in the msg string.
    char msg[120];  // should be long enough for all slots, just in some bizarre case that we need it

    // first, construct a message listing each bay slot filled
    if (slotsDrainedFilled.filledList.size() > 0)
    {
        strcpy(msg, "Bay tank(s) full: ");  // slot numbers appended below
        for (unsigned int i=0; i < slotsDrainedFilled.filledList.size(); i++)
        {
            if (i > 0)  // more than one tank?
                strcat(msg, ", ");  // add separator

            const int msgIndex = static_cast<int>(strlen(msg));  // overwrite current null terminating byte
            sprintf(msg + msgIndex, "#%d", slotsDrainedFilled.filledList[i]);
        }

        PlaySound(BeepHigh, ST_Other);
        ShowInfo(NULL, ST_None, msg);  
    }
    else if (slotsDrainedFilled.drainedList.size() > 0)  // Note: we will *never* have tanks both full and drained in the same timestep!
    {
        strcpy(msg, "ALERT: Bay tank(s) empty: ");  // slot numbers appended below
        for (unsigned int i=0; i < slotsDrainedFilled.drainedList.size(); i++)
        {
            if (i > 0)  // more than one tank?
                strcat(msg, ", ");  // add separator

            const int msgIndex = static_cast<int>(strlen(msg));  // overwrite current null terminating byte
            sprintf(msg + msgIndex, "#%d", slotsDrainedFilled.drainedList[i]);
        }
        
        PlayErrorBeep();   // this is a warning, not a status message
        ShowWarning(NULL, ST_None, msg);  
    }

    return slotsDrainedFilled.quantityAdjusted;
}

// returns max capacity of LOX tanks, including any LOX tanks in the bay
// This will always be > 0.
double DeltaGliderXR1::GetXRLOXMaxMass() const
{
    double totalMass = GetXR1Config()->GetMaxLoxMass();
    if (m_pPayloadBay != nullptr)
        totalMass += m_pPayloadBay->GetPropellantMaxMass(PT_LOX);
    
    return totalMass;
}

// returns the current quantity of LOX, including any LOX in the bay
double DeltaGliderXR1::GetXRLOXMass() const
{
    return (m_loxQty + GetXRBayLOXMass());
}

// returns the current quantity of LOX in the BAY only
double DeltaGliderXR1::GetXRBayLOXMass() const
{
    double bayLoxMass = 0;

    if (m_pPayloadBay != nullptr)
        bayLoxMass = m_pPayloadBay->GetPropellantMass(PT_LOX);
    
    return bayLoxMass;
}

// set the quantity of LOX, including any LOX tank(s)
// Note: internal tanks are always filled *first*
void DeltaGliderXR1::SetXRLOXMass(const double mass)
{
    double deltaRemaining = mass;
    
    // fill the internal tank first
    const double internalTankQty = min(mass, GetXR1Config()->GetMaxLoxMass());
    m_loxQty = internalTankQty;
    deltaRemaining -= internalTankQty; 

    // now store any remainder in the payload bay, if any bay exists
    if (m_pPayloadBay != nullptr)
    {
        // get the delta between the current payload bay quantity and the new quantity
        const double bayDeltaRequested = deltaRemaining - m_pPayloadBay->GetPropellantMass(PT_LOX);  // newBayMass - currentBayMass

        // apply the delta (as a *request*) to the bay tanks
        const double bayDeltaApplied = AdjustBayPropellantMassWithMessages(PT_LOX, bayDeltaRequested);

        // if the caller's code is correct, we should never overflow the bay quantity
        // Need to account for slight rounding error possibility here in the nth decimal place, so 0.01 is way overkill, but fine for an assert
        _ASSERTE(fabs(bayDeltaApplied - bayDeltaRequested) < 0.01);
    }
    else 
    {
        // no payload bay, so it should all fit in the internal tank!
        // Need to account for slight rounding error possibility here in the nth decimal place, so 0.01 is way overkill, but fine for an assert
        _ASSERTE(fabs(deltaRemaining) < 0.01);
    }
} 

// Remove all existing Mmu crew members, if any
void DeltaGliderXR1::RemoveAllMmuCrewMembers()
{
#ifdef MMU
    const int crewCount = CONST_UMMU(this).GetCrewTotalNumber();
    for (int i=0; i < crewCount; i++)
    {
        // UMmu bug: cannot use const char * here because RemoveCrewMember takes a 'char *' even though it never modifies it
        char *pName = CONST_UMMU(this).GetCrewNameBySlotNumber(i);
        UMmu.RemoveCrewMember(pName);  // UMMU BUG: METHOD DOESN'T WORK!  
    }
#endif
}

// {ZZZ} You may need to update this method whenever the mesh is recreated (in case the texture indices changed): do not delete this comment
//
// Note: this method must reside here in in XRVessel.cpp rather than DeltaGliderXR1.cpp because even though it is never used by subclasses, it is 
// still the *superclass method* of all the XR1 vessel subclasses.  Therefore, it must be available at link time even though this superclass
// method it is never invoked from a subclass (all subclasses MUST override this method for correct vessel-specific functionality).
//
// meshTextureID = vessel-specific constant that is translated to a texture index specific to our vessel's .msh file.  meshTextureID 
// NOTE: meshTextureID=VCPANEL_TEXTURE_NONE = -1 = "no texture" (i.e., "not applicable"); defined in Area.h.
// hMesh = OUTPUT: will be set to the mesh handle of the mesh associated with meshTextureID.
DWORD DeltaGliderXR1::MeshTextureIDToTextureIndex(const int meshTextureID, MESHHANDLE &hMesh)
{
    // sanity check
    _ASSERTE(meshTextureID > VCPANEL_TEXTURE_NONE);

    // same mesh for all VC textures
    hMesh = vcmesh_tpl;  // assign by reference

    DWORD retVal = 0;
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

    return retVal;
}

// Also fixes poor ground turning performance by "cheating" and rotating the ship based on
// wheel deflection.  Based on code here: http://orbiter-forum.com/showthread.php?t=8392
// This should only be invoked from a PreStep.
// UPDATE: tweaked to handle turning in *reverse* as well.
void DeltaGliderXR1::AmplifyNosewheelSteering()
{
    // now rotate the ship to fix poor nosewheel steering performance inherent in all Orbiter vessels by default
    if (GetNosewheelSteering())  // can we steer the nose?
    {
        VECTOR3 pt1, pt2, pt3;

        const double groundspeed = GetGroundspeed();
        GetTouchdownPoints(pt1, pt2, pt3);

        const double wheelbase = pt1.z - (pt2.z + pt3.z) / 2.0;
        const double maxDeflectionAirspeedThreshold = 2;  // in m/s; (forum code had 10 here).  At this velocity, max deflection rate will be reached.  Lowering this will increase turning rates at low speeds.
        // ORG pre-Orbiter 2016 : const double deflectionLimit = 15;  // Note: code in forum had 15 for this
		const double deflectionLimit = 5;

        // ORG pre-Orbiter 2016: decrease deflection limit linearly between maxDeflectionAirspeedThreshold and 90 m/s; i.e., at 90 m/s no additional deflection will be applied here.
		// decrease deflection limit linearly between maxDeflectionAirspeedThreshold and 15 m/s; i.e., at 15 m/s no additional deflection will be applied here.
		double maxDeflection = ((groundspeed < maxDeflectionAirspeedThreshold) ? deflectionLimit : (deflectionLimit - (deflectionLimit * ((groundspeed - maxDeflectionAirspeedThreshold) / 15.0))));
        if (maxDeflection < 0.0)   // keep in range
            maxDeflection = 0.0;  
        double theta = -maxDeflection * GetControlSurfaceLevel(AIRCTRL_RUDDER);

        VECTOR3 avel;
        GetAngularVel(avel);

        VECTOR3 groundspeedVec;
		GetGroundspeedVector(FRAME_LOCAL, groundspeedVec);
		bool reverse = (groundspeedVec.z < 0);  // ship is backing up

		double newAngularVelocity = groundspeed / (wheelbase * tan((90 - theta) * PI / 180)) * (reverse ? -1.0 : 1.0);
        // DEBUG: sprintf(oapiDebugString(), "groundspeedVec: z=%lf, avel.y=%lf, newAngularVelocity=%lf", groundspeedVec.z, avel.y, newAngularVelocity);

        if (fabs(newAngularVelocity) > fabs(avel.y))  // never *reduce the rate* of our angular velocity
            avel.y = newAngularVelocity;

        SetAngularVel(avel);
    }
}

// static worker method that returns a pointer to a static exhaust spec
// pos and/or dir and/or tex may be null
// Note: contrary to the documentation note for AddExhaust(EXHAUSTSPEC) below, the thrusters *do* react to a change in thrust direction,
// at least in the D3D9 client.
//   >> "Exhaust positions and directions are fixed in this version, so they will not react to changes caused by SetThrusterRef and SetThrusterDir."
EXHAUSTSPEC *DeltaGliderXR1::GetExhaustSpec(const THRUSTER_HANDLE th, const double lscale, const double wscale, const VECTOR3 *pos, const VECTOR3 *dir, const SURFHANDLE tex)
{
    static EXHAUSTSPEC es;
    es.th = th;
    es.level = nullptr;  // core manages the level
    // work around Orbiter core bug here: EXHAUSTSPEC should specify lpos and ldir as const object pointers (they obviously are never changed by the core)
    es.lpos = const_cast<VECTOR3 *>(pos);    // may be null
    es.ldir = const_cast<VECTOR3 *>(dir);    // may be null
    es.lsize = lscale;
    es.wsize = wscale;
    es.lofs = 0;
    es.modulate = 0.20;  // modulates in brightness by this fraction
    es.tex = tex;        // may be null
    es.flags =  EXHAUST_CONSTANTPOS | EXHAUST_CONSTANTDIR;
    es.id = 0;           // reserved, so let's by tidy for our part

    return &es;
}

// XR gateway method for AddExhaust
unsigned int DeltaGliderXR1::AddXRExhaust(const THRUSTER_HANDLE th, const double lscale, const double wscale, const SURFHANDLE tex)
{
    return AddExhaust(GetExhaustSpec(th, lscale, wscale, NULL, NULL, tex));
}

// XR gateway method for AddExhaust
unsigned int DeltaGliderXR1::AddXRExhaust(const THRUSTER_HANDLE th, const double lscale, const double wscale, const VECTOR3 &pos, const VECTOR3 &dir, const SURFHANDLE tex)
{
    // Note: although not documented in the AddExhaust(EXHAUSTSPEC *) method, the exhaust direction must be *opposite*
    // what it is in the other AddExhaust versions, so we must flip it here.
    const VECTOR3 flippedDir = -dir;
    return AddExhaust(GetExhaustSpec(th, lscale, wscale, &pos, &flippedDir, tex));
}

// Reset the MET; invoked when ship is landed
void DeltaGliderXR1::ResetMET()
{
    ShowInfo("Mission Elapsed Time Reset.wav", DeltaGliderXR1::ST_InformationCallout, "Mission Elapsed Time reset; timer&will start at liftoff.");
    m_metMJDStartingTime = -1;     // reset timer
    m_metTimerRunning = false;     // not running now
    RecordEvent ("RESETMET", ".");
}

// Set crossfeed mode main/rcs/off
// pMsg = mode-specific infomation message; may be null
void DeltaGliderXR1::SetCrossfeedMode(const XFEED_MODE mode, const char *pMsg)
{
    char modeString[16];
    m_xfeedMode = mode;

    if (mode == XF_OFF)
    {
        char temp[80];
        if (pMsg != nullptr)
            sprintf(temp, "%s; cross-feed OFF.", pMsg);
        else
            strcpy(temp, "Fuel cross-feed OFF.");  // no optional reason
        ShowInfo("Cross-Feed Off.wav", DeltaGliderXR1::ST_InformationCallout, temp);
        strcpy(modeString, "OFF");
    }
    else if (mode == XF_MAIN)
    {
        ShowInfo("Cross-Feed Main.wav", DeltaGliderXR1::ST_InformationCallout, "Fuel cross-feed to MAIN.");
        strcpy(modeString, "MAIN");
    }
    else if (mode == XF_RCS)
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

void DeltaGliderXR1::SetFuelDumpState(bool &fuelDumpInProgress, const bool isDumping, const char *pFuelLabel)
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
        ShowInfo(NULL, DeltaGliderXR1::ST_None, temp);
    }

    // Convert the fueldump reference to a string; this is a bit of a hack since it checks pointer addresses instead
    // of a proper flag, but it should be fine since these will never change.
    const char *pEventName;
    const bool *pIsInProgress = &fuelDumpInProgress;  
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
        ShowInfo(NULL, DeltaGliderXR1::ST_None,"LOX dump halted.");
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
    externalcooling_status = (bEnableExternalCooling ? DOOR_OPEN : DOOR_CLOSED);
    
    // play door thump sound 
    PlaySound(SupplyHatch, DeltaGliderXR1::ST_Other, SUPPLY_HATCH_VOL);
    
    // log info message and play callout
    const char *pState = (bEnableExternalCooling ? "open" : "closed");
    char msg[40];
    char wavFilename[40];
    sprintf(msg, "External coolant hatch %s.", pState);
    sprintf(wavFilename, "Hatch %s.wav", pState);

    // NOTE: do not attempt to play a "Hatch Closed" callout since our FuelPostStep will play a proper "External Cooling Systems Offline" callout.
    ShowInfo((bEnableExternalCooling ? wavFilename : NULL), DeltaGliderXR1::ST_InformationCallout, msg);

    // show the new state on the panel
    TriggerRedrawArea(AID_EXTERNAL_COOLING_SWITCH);
    TriggerRedrawArea(AID_EXTERNAL_COOLING_LED);  

    return true;
}

// Enable or disable mode to reset the center-of-gravity
void DeltaGliderXR1::SetRecenterCenterOfGravityMode(const bool bEnabled)
{
    m_cogShiftCenterModeActive = bEnabled;
    TriggerRedrawArea(AID_COG_CENTER_BUTTON);
}


//
// New wrapper methods added for removing UMMU (11-Feb-2018)
//

int DeltaGliderXR1::GetCrewTotalNumber() const
{
#ifdef MMU
    // TODO: call MMU.GetCrewTotalNumber();
#else
    return MAX_PASSENGERS;
#endif
}

const char *DeltaGliderXR1::GetCrewNameBySlotNumber(const int index) const
{
#ifdef MMU
    // TODO: call MMU.GetCrewNameBySlotNumber();
#else
    return GetXR1Config()->CrewMembers[index].name;
#endif
}

int DeltaGliderXR1::GetCrewAgeByName(const char *pName) const
{
#ifdef MMU
    // TODO: call MMU.GetCrewTotalNumber();
#else
    int age = 0;
    for (int i = 0; i < MAX_PASSENGERS; i++)
    {
        CrewMember *pcm = GetXR1Config()->CrewMembers + i;
        if (_stricmp(pcm->name, pName) == 0)
        {
            age = pcm->age;
            break;
        }
    }

    return age;
#endif
}

const char *DeltaGliderXR1::GetCrewMiscIdByName(const char *pName) const
{
#ifdef MMU
    // TODO: call MMU.GetCrewMiscIdByName();
#else
    const char *pMiscID = "";
    for (int i = 0; i < MAX_PASSENGERS; i++)
    {
        CrewMember *pcm = GetXR1Config()->CrewMembers + i;
        if (_stricmp(pcm->name, pName) == 0)
        {
            pMiscID = pcm->miscID;  // "XI0", "XI1", etc.
            break;
        }
    }

    return pMiscID;
#endif
}
