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
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// XRVessel.cpp
// Main vessel class for the XR1, which is the base class for other XR-class vessels.
// ==============================================================

// include common definitions
#include "DeltaGliderXR1.h"
#include "XRPayloadBay.h"        // necessary for destructor
#include "XR1MultiDisplayArea.h" // necessary for constructor


// --------------------------------------------------------------
// Constructor
// --------------------------------------------------------------
// NOTE: fmodel is ignored for the XR1
DeltaGliderXR1::DeltaGliderXR1 (OBJHANDLE hObj, int fmodel, XR1ConfigFileParser *pConfigFileParser) : 
    VESSEL3_EXT(hObj, fmodel),
    m_secondaryHUDMode(3), m_preStepPreviousAirspeed(0), m_preStepPreviousGearFullyUncompressedAltitude(-1), m_airborneTargetTime(0), 
    m_takeoffTime(0), m_touchdownTime(0), m_preStepPreviousVerticalSpeed(0), m_forceWarning(false), m_accScale(AccScale::NONE), m_maxGaugeAcc(0),
    m_isCrashed(false),
    m_noseconeTemp(0), m_leftWingTemp(0), m_rightWingTemp(0), m_cockpitTemp(0), m_topHullTemp(0),
    m_activeMultiDisplayMode(DEFAULT_MMID), m_activeTempScale(TempScale::Celsius), m_pMDA(nullptr),
    m_tertiaryHUDOn(true), m_damagedWingBalance(0), m_crashProcessed(false),
    m_infoWarningTextLineGroup(INFO_WARNING_BUFFER_LINES), m_mwsTestActive(false),
    m_nextMDARefresh(0), m_nextSecondaryHUDRefresh(0), m_lastSecondaryHUDMode(0),
    m_metMJDStartingTime(-1), m_interval1ElapsedTime(-1), m_interval2ElapsedTime(-1),
    m_metTimerRunning(false), m_interval1TimerRunning(false), m_interval2TimerRunning(false),
    m_apuFuelQty(APU_FUEL_CAPACITY), m_mainFuelDumpInProgress(false), m_rcsFuelDumpInProgress(false),
    m_scramFuelDumpInProgress(false), m_apuFuelDumpInProgress(false), m_xfeedMode(XFEED_MODE::XF_OFF),
    m_mainExtLinePressure(0), m_scramExtLinePressure(0), m_apuExtLinePressure(0), m_loxExtLinePressure(0),
    m_nominalMainExtLinePressure(0), m_nominalScramExtLinePressure(0), m_nominalApuExtLinePressure(0), m_nominalLoxExtLinePressure(0),
    m_mainSupplyLineStatus(false), m_scramSupplyLineStatus(false), m_apuSupplyLineStatus(false), m_loxSupplyLineStatus(false),
    m_mainFuelFlowSwitch(false), m_scramFuelFlowSwitch(false), m_apuFuelFlowSwitch(false), m_loxFlowSwitch(false),
    m_loxQty(-1), // set for real in clbkSetClassCaps
    m_loxDumpInProgress(false), m_oxygenRemainingTime(0), m_cabinO2Level(NORMAL_O2_LEVEL),
    m_crewState(CrewState::OK), m_coolantTemp(NOMINAL_COOLANT_TEMP), m_internalSystemsFailure(false),
    m_customAutopilotMode(AUTOPILOT::AP_OFF), m_airspeedHoldEngaged(false), m_setPitchOrAOA(0), m_setBank(0), m_initialAHBankCompleted(false), m_holdAOA(false),
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
    anim_bay(0), bay_status(DoorStatus::DOOR_CLOSED), bay_proc(0), m_requestSwitchToTwoDPanelNumber(-1),
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

    gear_status       = DoorStatus::DOOR_CLOSED;
    gear_proc         = 0.0;
    rcover_status     = DoorStatus::DOOR_CLOSED;
    rcover_proc       = 0.0;
    nose_status       = DoorStatus::DOOR_CLOSED;
    nose_proc         = 0.0;
    scramdoor_status  = DoorStatus::DOOR_CLOSED;
    scramdoor_proc    = 0.0;
    hoverdoor_status  = DoorStatus::DOOR_CLOSED;
    hoverdoor_proc    = 0.0;
    ladder_status     = DoorStatus::DOOR_CLOSED;
    ladder_proc       = 0.0;
    olock_status      = DoorStatus::DOOR_CLOSED;
    olock_proc        = 0.0;
    ilock_status      = DoorStatus::DOOR_CLOSED;
    ilock_proc        = 0.0;
    chamber_status    = DoorStatus::DOOR_CLOSED;  // closed = PRESSURIZED
    chamber_proc = 0.0;
    hatch_status      = DoorStatus::DOOR_CLOSED;
    hatch_proc        = 0.0;
    brake_status      = DoorStatus::DOOR_CLOSED;
    brake_proc        = 0.0;
    radiator_status   = DoorStatus::DOOR_CLOSED;
    radiator_proc     = 0.0;

    // no proc for these; supply hatches are battery powered and "snap" open or closed
    fuelhatch_status = DoorStatus::DOOR_CLOSED;
    loxhatch_status  = DoorStatus::DOOR_CLOSED;
    externalcooling_status = DoorStatus::DOOR_CLOSED;
    
    // NOTE: we treat the APU like a door here since it has spin-up and spin-down states
    // however, there is no proc for it
    apu_status        = DoorStatus::DOOR_CLOSED;
    
    exmesh            = nullptr;
    vcmesh            = nullptr;
    vcmesh_tpl        = nullptr;
    ramjet            = nullptr;
    hatch_vent        = nullptr;
    campos            = DeltaGliderXR1::CAMERA_POSITION::CAM_GENERIC;

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
