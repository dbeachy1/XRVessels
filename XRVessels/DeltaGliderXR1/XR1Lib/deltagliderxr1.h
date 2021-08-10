// ==============================================================
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// Copyright 2006-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// DeltaGlider.h
// Class interface for DeltaGlider XR1 class module
// ==============================================================

#pragma once

#include "orbitersdk.h"
#include "XR1Ramjet.h"
#include "resource.h"
#include "InstrumentPanel.h"
#include "XRSound.h"
#include "XR1ConfigFileParser.h"
#include "TextBox.h"
#include "XR1Globals.h"

#ifdef MMU
#include "UMmuSDK.h"
#endif

#include <atlstr.h>

// Forward references
class MultiDisplayArea;
class XRPayloadBay;

#ifdef MMU
// Hack to work around UMMu bugs with none of its methods using const.
#define CONST_UMMU(xr1ptr) (const_cast<DeltaGliderXR1 *>(xr1ptr)->UMmu)
#endif

// ==========================================================
// Interface for derived vessel class: DeltaGliderXR1
// ==========================================================

class DeltaGliderXR1 : public VESSEL3_EXT
{
public:
	DeltaGliderXR1(OBJHANDLE hObj, int fmodel, XR1ConfigFileParser *pConfigFileParser);
	virtual ~DeltaGliderXR1();

    // gimbal switch definitions
    enum GIMBAL_SWITCH { LEFT, RIGHT, BOTH };
    enum DIRECTION { UP_OR_LEFT, DOWN_OR_RIGHT, DIR_NONE };

#ifdef MMU
    UMMUCREWMANAGMENT UMmu;  // Universal MMU
#endif

    // WARNING: EACH SUBCLASS *MUST* OVERRIDE THE FOLLOWING THREE METHODS 
    // EVEN IF YOU JUST MAKE THEM EMPTY.
    virtual void SetDamageVisuals();     
    virtual void SetPassengerVisuals();  
    virtual DWORD MeshTextureIDToTextureIndex(const int meshTextureID, MESHHANDLE &hMesh);

    // these methods are virtual so then can be overwritten by XR vessel subclasses
	virtual void UpdateVCMesh();  
    virtual void DefineAnimations();
    virtual void CleanUpAnimations();
    virtual void SetEmptyMass();
    virtual void PerformCrashDamage();
    virtual bool CheckAllDoorDamage();
    virtual bool CheckHullHeatingDamage();
    virtual bool IsOATValid() const { return (GetAtmPressure() >= OAT_VALID_STATICP_THRESHOLD); }  // returns true if OAT and Mach readings are valid
    virtual double GetExternalTemperature() const;
    
    // convenience method
    const XR1ConfigFileParser *GetXR1Config() const { return static_cast<const XR1ConfigFileParser *>(m_pConfig); }

    void EnableRetroThrusters(bool state);
    void EnableHoverEngines(bool state);
    void EnableScramEngines(bool state);
	void TestDamage ();
	void ApplyDamage ();
	void SaveOrbiterRenderWindowPosition();
	void RestoreOrbiterRenderWindowPosition();
    void clbkPostCreationCommonXRCode();              // not virtual since it should never be overridden
    bool ParseXRCommonScenarioLine(char *line);       // not virtual since it should never be overridden
    void WriteXRCommonScenarioLines(FILEHANDLE scn);  // not virtual since it should never be overridden

	// Overloaded callback functions
	virtual void clbkSetClassCaps(FILEHANDLE cfg);
	virtual void clbkLoadStateEx(FILEHANDLE scn, void *vs);
	virtual void clbkSaveState(FILEHANDLE scn);
	virtual void clbkPostCreation();
	virtual void clbkVisualCreated(VISHANDLE vis, int refcount);
	virtual void clbkVisualDestroyed(VISHANDLE vis, int refcount);
    virtual void clbkPreStep(double simt, double simdt, double mjd);
	virtual void clbkPostStep(double simt, double simdt, double mjd);
	virtual bool clbkPlaybackEvent(double simt, double event_t, const char *event_type, const char *event);
    virtual bool clbkDrawHUD(int mode, const HUDPAINTSPEC *hps, oapi::Sketchpad *skp);
    virtual void clbkRenderHUD(int mode, const HUDPAINTSPEC *hps, SURFHANDLE hDefaultTex);

	virtual void clbkRCSMode(int mode);
	virtual void clbkADCtrlMode(DWORD mode);
	virtual void clbkHUDMode(int mode);
	virtual void clbkMFDMode(int mfd, int mode);
	virtual void clbkNavMode(int mode, bool active);
	virtual int  clbkConsumeDirectKey(char *kstate);
	virtual int  clbkConsumeBufferedKey(DWORD key, bool down, char *kstate);
	virtual bool clbkLoadGenericCockpit();
    virtual void clbkFocusChanged(bool getfocus, OBJHANDLE hNewVessel, OBJHANDLE hOldVessel);

    virtual bool clbkLoadPanel(int panelID);
    virtual void clbkDockEvent(int dock, OBJHANDLE mate);
    
    virtual bool clbkLoadVC(int id);
    virtual void UpdateCtrlDialog (DeltaGliderXR1 *dg, HWND hWnd = nullptr);

    // remaining callback methods are implemented by our base class, VESSEL3_EXT

    // implement pure virtual methods from our base class
    virtual int GetVCPanelIDBase() const { return VC_PANEL_ID_BASE; } 

    void RenderDataHUD(const HUDPAINTSPEC *hps, oapi::Sketchpad *skp);

    double m_damagedWingBalance;       // used by ApplyDamage()

	// parameters for failure modeling in the mesh
	double lwingstatus, rwingstatus;
	bool aileronfail[4];   // order is: left, left, right, right

    void UpdateDoorDamage(DoorStatus &doorStatus, double &doorProc, const double fracIntegrity);

    // it is typically not necessary to override this method by subclasses, but we may need to.
    virtual void ResetDamageStatus();

    // virtual methods typically overridden by subclasses
    virtual void SetDamageStatus(DamageItem item, double fracIntegrity);
    virtual const DamageStatus &GetDamageStatus(DamageItem item) const;
    virtual bool IsDamagePresent() const;
    virtual bool IsWarningPresent() const;
    virtual double GetRCSThrustMax(const int index) const;
    virtual void ResetAllRCSThrustMaxLevels();
    virtual void TweakInternalValue(bool direction);  // used for developement testing only; usually an empty method
    virtual void ShowHatchDecompression();
    virtual void CleanUpHatchDecompression();
    virtual bool InAtm() const { return (GetAtmPressure() > 0.1); }  // returns true if we are in the atmosphere
    virtual void AlteaLogoClicked();   // Easter Egg
#ifdef TURBOPACKS
    virtual void DeployTurbopack();
    virtual void StowAllTurbopacks();
#endif

    void SetHoverThrusterMaxAndIntegrity(int engineIndex, double integrityFrac);  
    void SetRCSThrusterMaxAndIntegrity(int index, double integrityFrac);

    DoorStatus nose_status, scramdoor_status, hoverdoor_status, ladder_status, gear_status, rcover_status, olock_status, ilock_status, chamber_status, hatch_status, radiator_status, brake_status;

    // NOTE: we treat the APU like a door here since it has spin-up and spin-down states
    DoorStatus apu_status;
	bool ResetMWS();
    virtual void ActivateLandingGear (DoorStatus action);
    void ActivateHoverDoors(DoorStatus action);
    void ActivateScramDoors(DoorStatus action);
	void ActivateRCover (DoorStatus action);
	void ActivateNoseCone (DoorStatus action);
	virtual void ActivateLadder (DoorStatus action);
    virtual void ActivateRadiator (DoorStatus action);
	void ActivateOuterAirlock (DoorStatus action);
	void ActivateInnerAirlock (DoorStatus action);
    void ForceActivateInnerAirlock(DoorStatus action); // do the work for ActivateInnerAirlock
    void ForceActivateCabinHatch(DoorStatus action);   // do the work for ActivateCabinHatch
	void ActivateHatch (DoorStatus action);
	void ActivateAirbrake (DoorStatus action);
    void ActivateAPU(DoorStatus action);
	void ActivateChamber(DoorStatus action, bool force);
    virtual void ActivateBayDoors(DoorStatus action);  // overridden by the XR5
    void ToggleLandingGear ();
	void ToggleNoseCone();
	void ToggleLadder ();
	void ToggleOuterAirlock ();
	void ToggleInnerAirlock ();
	void ToggleHatch ();
    int KillCrew();
	void ToggleAirbrake ();
	void ToggleRadiator ();
    void ToggleRCover();
    void ToggleHoverDoors();
    void ToggleScramDoors();
    void ToggleAPU();
    void ToggleBayDoors();
	void SetXRTouchdownPoints(const VECTOR3 &pt1, const VECTOR3 &pt2, const VECTOR3 &pt3, const double mu_lng, const double mu_lat, const bool bIsGearDown) const;
	virtual void SetGearParameters (double state);
    void PerformUndocking();
    bool ShiftCenterOfLift(double requestedShift);
    void KillAllAutopilots();
    void DisableSecondaryHUD();
    void EnableAndSetSecondaryHUDMode(int mode);
    void SetTertiaryHUDEnabled(bool on);
    void GimbalSCRAMPitch(const GIMBAL_SWITCH which, const DIRECTION dir);
    void GimbalMainPitch(const GIMBAL_SWITCH which, const DIRECTION dir);
    void GimbalMainYaw(const GIMBAL_SWITCH which, const DIRECTION dir);
    void GimbalRecenterAll();
    void ShiftHoverBalance(const GIMBAL_SWITCH which, const DIRECTION dir);
    double AdjustBayPropellantMassWithMessages(const PROP_TYPE pt, const double requestedFlowQty);
    void RemoveAllMmuCrewMembers();
    void AmplifyNosewheelSteering();  // fixes poor ground turning performance; should be called from a PreStep
    int Get2DHUDGearIndicatorPenWidth();  // cannot be const because it invokes Get2DPanelWidth()
    oapi::Font *GetNormal2DHUDFont();
    bool AllowDamageIfDockedCheck() const { return (!IsDocked() || GetXR1Config()->EnableDamageWhileDocked); }  // returns true if damage allowed: should be invoked before each damage check
    bool AreElevatorsOperational() const { return !(aileronfail[0] || aileronfail[1] || aileronfail[2] || aileronfail[3]); }

    // invoked whenever a function that uses the APU is operating
    void MarkAPUActive() { m_latestHydraulicDoorRunningSimt = GetAbsoluteSimTime(); }  // use absolute simt so it never goes negative 
    
    // these methods include any fuel/LOX in the payload bay as well
    double GetXRPropellantMaxMass(PROPELLANT_HANDLE ph) const;
    double GetXRPropellantMass(PROPELLANT_HANDLE ph) const;
    double GetXRBayPropellantMass(PROPELLANT_HANDLE ph) const;
    void SetXRPropellantMass(PROPELLANT_HANDLE ph, const double mass);
    
    double GetXRLOXMaxMass() const; 
    double GetXRLOXMass() const;    
    double GetXRBayLOXMass() const;    
    void SetXRLOXMass(const double mass);
    PROP_TYPE GetPropTypeForHandle(PROPELLANT_HANDLE ph) const;
    
    // Note: XR1ConfigFileParser::GetMaxLoxMass only affects the SHIP'S INTERNAL LOX QTY (not payload)

    // Note: no proc for fuel or LOX hatches: they "snap" open or closed
	double nose_proc, scramdoor_proc, hoverdoor_proc, ladder_proc, gear_proc, rcover_proc, olock_proc, ilock_proc, chamber_proc, hatch_proc, radiator_proc, brake_proc;     // logical status

    // WARNING: All code should invoke SetXRAnimation instead of SetXRAnimation!  The reason is that 
    // SetXRAnimation always assumes that the handle is valid, and so SetXRAnimation is a virtual "gate" 
    // method that allows each subclass to determine whether that animation is valid or not for that vessel.
    virtual void SetXRAnimation(const UINT &anim, const double state) const;

	UINT anim_gear;         // handle for landing gear animation
	UINT anim_rcover;       // handle for retro cover animation
    UINT anim_hoverdoor;    // handle for hover doors animation
    UINT anim_scramdoor;    // handle for scram doors animation
	UINT anim_nose;         // handle for nose cone animation
	UINT anim_ladder;       // handle for front escape ladder animation
	UINT anim_olock;        // handle for outer airlock animation
	UINT anim_ilock;        // handle for inner airlock animation
	UINT anim_hatch;        // handle for top hatch animation
	UINT anim_radiator;     // handle for radiator animation
	UINT anim_rudder;       // handle for rudder animation
	UINT anim_elevator;     // handle for elevator animation
	UINT anim_elevatortrim; // handle for elevator trim animation
	UINT anim_laileron;     // handle for left aileron animation
	UINT anim_raileron;     // handle for right aileron animation
	UINT anim_brake;        // handle for airbrake animation
    
    // new animation handles for consumables hatches; these are driven by the code in XRVessel, but they are not used by the XR1.
    UINT anim_fuelhatch;
    UINT anim_loxhatch;

	UINT anim_mainthrottle[2];  // VC main/retro throttle levers (left and right)
	UINT anim_hoverthrottle;    // VC hover throttle
	UINT anim_scramthrottle[2]; // VC scram throttle levers (left and right)
	UINT anim_gearlever;        // VC gear lever
	UINT anim_nconelever;       // VC nose cone lever
	UINT anim_pmaingimbal[2];   // VC main engine pitch gimbal switch (left and right engine)
	UINT anim_ymaingimbal[2];   // VC main engine yaw gimbal switch (left and right engine)
	UINT anim_scramgimbal[2];   // VC scram engine pitch gimbal switch (left and right engine)
	UINT anim_hbalance;         // VC hover balance switch
	UINT anim_hudintens;        // VC HUD intensity switch
	UINT anim_rcsdial;          // VC RCS dial animation
	UINT anim_afdial;           // VC AF dial animation
	UINT anim_olockswitch;      // VC outer airlock switch animation
	UINT anim_ilockswitch;      // VC inner airlock switch animation
	UINT anim_retroswitch;      // VC retro cover switch animation
	UINT anim_ladderswitch;     // VC ladder switch animation
	UINT anim_hatchswitch;      // VC hatch switch animation
	UINT anim_radiatorswitch;   // VC radiator switch animation

    // Note: exmesh_tpl is now in VESSEL3_EXT base class
    MESHHANDLE vcmesh_tpl;      // this is a *template*, so it's a MESHHANDLE, not a DEVMESHHANDLE
    DEVMESHHANDLE exmesh;
	DEVMESHHANDLE vcmesh;          // local VC mesh and global template
    MESHHANDLE heatingmesh_tpl;    // global template: used for hull heating effects (not used on the XR1)
    DEVMESHHANDLE heatingmesh;     // used for hull heating effects (not used on the XR1)

	THGROUP_HANDLE thg_main;
	THGROUP_HANDLE thg_retro;
	THGROUP_HANDLE thg_hover;
    bool m_parsedScenarioFile;      // TRUE if we parsed a scenario file

	BEACONLIGHTSPEC beacon[7];      // light beacon definitions: NAV=0,1,2 : BEACON=3,4 : STROBE=5,6
    SpotLight **m_pSpotlights;             // size will be SPOTLIGHT_COUNT
    double     m_mainThrusterLightLevel;   // set to match GetThrusterGroupLevel (THGROUP_MAIN);
    double     m_hoverThrusterLightLevel;  // set to match GetThrusterGroupLevel (THGROUP_HOVER);
	void SetNavlight (bool on);
	void SetBeacon (bool on);
	void SetStrobe (bool on);

    bool IsDocked() const        { return ((GetFlightStatus() & 0x2) != 0) && (nose_status == DOOR_OPEN); }  // ignore docking if nosecone not open (we will auto-undock in the next timestep)
    bool IsCrashed() const       { return m_isCrashed; }
    bool IsCrewIncapacitated() const { return (IsCrashed() || (m_crewState == INCAPACITATED) || (m_crewState == DEAD) || (GetCrewMembersCount() == 0)); }
    bool IsCrewIncapacitatedOrNoPilotOnBoard() const;
    bool IsCrewRankOnBoard(const char *pRank) const;
    bool IsPilotOnBoard() const;  // true if ship is flyable
    bool IsRefuelingOrCrossfeeding() const   // returns true if the vessel is currently refueling or crossfeeding fuel
    {
        return (m_mainFuelFlowSwitch || 
        m_scramFuelFlowSwitch ||
        (m_xfeedMode == XF_MAIN) ||
        (m_xfeedMode == XF_RCS));
    }

    // Note: we check pitch as well in case gear compression is not implemented
    // As a reasonable simplification, we assume the front gear always leaves the ground first.
    // Therefore, if we are on the ground we assume that the rear gear is ALWAYS on the ground.
    virtual bool IsRearGearOnGround() const
    { 
        // if not fully uncompressed OR groundContact, gear is on ground
        return ((m_rearGearProc  < 1.0) || GroundContact()); 
    }  

    virtual bool IsNoseGearOnGround() const
    { 
        // nose gear is only on the ground if the *rear* gear is also on the ground
        // We need to handle ships that do not have gear compression, too, so that's
        // why we need the IsRearGearOnGround() check here.
		// DEBUG: sprintf(oapiDebugString(), "IsNoseGearOnGround: IsRearGearOnGround()=%d, pitch=%.1lf degrees", IsRearGearOnGround(), (GetPitch() * DEG));  
        return (IsRearGearOnGround() && 
                 ( (m_noseGearProc  < 1.0) || ((GetPitch() * DEG) < 1.5) )   // NOTE: used to be 0.4 degrees, but the ships sit more nose-up in Orbiter 2016
               ); 
    }

    // Returns the fraction of thrust efficiency at normal ATM pressure (1 atmosphere).
    double GetISPScale() const 
    {
        return (GetXR1Config()->EnableATMThrustReduction ? 0.8 : 1.0);
    }

    // This enum has values for ALL the XR subclasses; this is necessary to ensure that
    // the base class behaves correctly when using the enum.
    enum CAMERA_POSITION {CAM_GENERIC, CAM_PANELMAIN, CAM_PANELUP, CAM_PANELDN, CAM_PANELOVERHEAD, CAM_PANELPAYLOAD, 
                          CAM_VCPILOT, CAM_VCCOPILOT, 
                          CAM_VCPSNGR1, CAM_VCPSNGR2, CAM_VCAIRLOCK, CAM_VCPSNGR3, CAM_VCPSNGR4,
                          CAM_VCPSNGR5, CAM_VCPSNGR6, CAM_VCPSNGR7, CAM_VCPSNGR8, 
                          CAM_VCPSNGR9, CAM_VCPSNGR10, CAM_VCPSNGR11, CAM_VCPSNGR12};
    // Note: these methods are not virtual because subclasses should never override them!
    bool IsCameraGeneric() const { return (campos == CAM_GENERIC); }
    bool IsCameraVC() const { return (campos >= CAM_VCPILOT); }
    bool IsCamera2D() const { return ((campos != CAM_GENERIC) && (campos < CAM_VCPILOT)); }

    // Additional public data for Area objects to access
    bool m_mwsTestActive;           // TRUE if MWS test button pressed
    bool m_dataHUDActive;           // TRUE if Data HUD button pressed
 	THRUSTER_HANDLE th_main[2];     // main engine handles
	THRUSTER_HANDLE th_retro[2];    // retro engine handles
	THRUSTER_HANDLE th_hover[2];    // hover engine handles
	THRUSTER_HANDLE th_scram[2];    // scramjet handles
    THRUSTER_HANDLE th_rcs[14];     // RCS jets
  	double scram_intensity[2];
	double scram_max[2];            // max SCRAM thrust
    XR1Ramjet *ramjet;              // scramjet module (NULL = none)
	PROPELLANT_HANDLE ph_main, ph_rcs, ph_scram; // propellant resource handles
    double *hatch_venting_lvl;      // used for hatch decompression effects
    PSTREAM_HANDLE *hatch_vent;     // array of exhaust streams for decompression effects
	double hatch_vent_t;            // time when hatch venting began
    AccScale m_accScale;            // set by ComputeAccPostStep
    double m_maxGaugeAcc;           // 2.0, 4.0, 8.0
    int m_selectedTurbopack;        // 0 <= n < TURBOPACKS_ARRAY_SIZE

    // fuel/lox dump streams; this is *not* used by the XR1; it is
    // refereced by FuelDumpPostStep, however.
    PARTICLESTREAMSPEC *m_pFuelDumpParticleStreamSpec;  

    // boil-off exhaust effect; this is not used by the XR1; it is 
    // referenced by BoilOffExhaustPostStep, however.
    PARTICLESTREAMSPEC *m_pBoilOffExhaustParticleStreamSpec;  

    // external coolant flowing: this is NOT persisted
    bool m_isExternalCoolantFlowing;

    // external cooling: this is persisted
    DoorStatus externalcooling_status;       // not for refueling, but handled the same

    // refueling parameters; these are NOT persisted
    DoorStatus fuelhatch_status, loxhatch_status;   // doors locked unless docked or landed
    double m_mainExtLinePressure;           // PSI in refueling line
    double m_nominalMainExtLinePressure;    // nominal PSI in refueling line
    
    double m_scramExtLinePressure;
    double m_nominalScramExtLinePressure;
    
    double m_apuExtLinePressure;
    double m_nominalApuExtLinePressure;
    
    double m_loxExtLinePressure;
    double m_nominalLoxExtLinePressure;

    // external supply line states; these are NOT persisted
    bool m_mainSupplyLineStatus;    // true = pressure is nominal
    bool m_scramSupplyLineStatus;
    bool m_apuSupplyLineStatus;
    bool m_loxSupplyLineStatus;     

    // flow switch data
    bool m_mainFuelFlowSwitch;   // true = switch ON (refuel in progress)
    bool m_scramFuelFlowSwitch;
    bool m_apuFuelFlowSwitch;
    bool m_loxFlowSwitch;
    bool m_externalCoolingSwitch;  // handled as a refueling item

    // O2 remaining time in SECONDS.
    // This is NOT persisted; it is computed by a PostStep.
    double m_oxygenRemainingTime;   

    // thruster status; this is NOT persisted
    bool m_isRetroEnabled;   // true if retro thrusters enabled, false otherwise
    bool m_isHoverEnabled;   // true if hover thrusters enabled, false otherwise
    bool m_isScramEnabled;   // true if scram engines enabled, false otherwise

    // hover engine integrity; this is NOT persisted; it is set at load time
    double m_hoverEngineIntegrity[2];   // fore, aft
    double m_hoverBalance;              // +- MAX_HOVER_IMBALANCE: 0=balanced

    // temperatures in Kelvin
    double m_noseconeTemp;
    double m_leftWingTemp;
    double m_rightWingTemp;
    double m_cockpitTemp;
    double m_topHullTemp;

    // contains temperature limit data
    HullTemperatureLimits m_hullTemperatureLimits;

    // our active Multi-Display Area (MDA) for the current panel; if NULL, it means the MDA is invisible (not rendered)
    MultiDisplayArea *m_pMDA;   // NOTE: this object is freed automatically by InstrumentPanel; do not free it twice.  We just *point to an active area* here.

    // warning light panel data
    bool m_warningLights[WARNING_LIGHT_COUNT];
    bool m_apuWarning;  // true if APU is in warning state

    // airfoil handle for wings
    AIRFOILHANDLE hwing;

    // fuel dump state data; this is NOT persisted!
    bool m_mainFuelDumpInProgress;
    bool m_rcsFuelDumpInProgress;
    bool m_scramFuelDumpInProgress;
    bool m_apuFuelDumpInProgress;
    bool m_loxDumpInProgress;

    // x-feed state data is NOT persisted!
    XFEED_MODE m_xfeedMode;

    // active airlock door to be queried by the MMUPreStep 
    const DoorStatus *m_pActiveAirlockDoorStatus;

    // custom autopilot data that is NOT persisted
    bool m_customAutopilotSuspended;     // temporarily suspended due to time acc
    bool m_airspeedHoldSuspended;        // temporarily suspended airspeed hold
    double m_maxShipHoverAcc;            // max acc by hover engines w/o regard for atm
    double m_maxMainAcc;                 // max acc by main engines INCLUDING atm drag

    // misc state data the is NOT persisted
    bool   m_crashProcessed;             // true if engines already disabled
    double m_startupMainFuelFrac;        // initial fuel frac for *internal tank only* before first time step; used to prevent initial auto-refueling
    double m_startupRCSFuelFrac;         // initial fuel frac for *internal tank only* before first time step; used to prevent initial auto-refueling
    double m_startupSCRAMFuelFrac;       // initial fuel frac for *internal tank only* before first time step; used to prevent initial auto-refueling
    bool   m_skipNextAFCallout;          // true if clbkADCtrlMode should skip its upcoming callout (only set when an autopilot is turning on the APU)
    bool   m_skipNextAPUWarning;         // true to skip next APU offline warning (only set when an autopilot has already performed the warning callout)
    bool   m_MWSLit;                     // true if MWS light is currently lit
    double m_hiddenElevatorTrimState;    // fixes nose-up push

    // Internal RCS damage status array; unlike other damage systems, RCS integrity is updated in sync here 
    // so that we can easily change max RCS thrust without jumping through hoops.
    double m_rcsIntegrityArray[14];

    //
    // New PERSISTENT public state data to communicate between areas and the main vessel
    //

    bool m_MWSActive;             // master warning light and alarm flag
    int m_lastActive2DPanelID;    // last 2D panel active; saved with the scenario and updated in real-time.  -1 = NOT SET YET (this is for future use)

    // TRUE if MMU crew data is valid (only set for RC4 or newer versions)
    bool m_mmuCrewDataValid;

    // HUD data
    int m_secondaryHUDMode;       // 0-5, 0=off
    int m_lastSecondaryHUDMode;
    bool m_tertiaryHUDOn;         

    // MET data
    double m_metMJDStartingTime;    // MDJ when MET timer started running; -1 = TIMER WAS RESET
    double m_interval1ElapsedTime;  // elapsed time in days; -1 = TIMER WAS RESET
    double m_interval2ElapsedTime;  // elapsed time in days; -1 = TIMER WAS RESET
    bool m_metTimerRunning;
    bool m_interval1TimerRunning;
    bool m_interval2TimerRunning;

    // updated by TakeoffAndLandingCalloutsAndCrashPostStep area
    double m_airborneTargetTime;  // time after which we assume we are really airborne
    double m_takeoffTime;         // time wheels lifted off (0 = on ground [may be moving])
    double m_touchdownTime;       // time wheels touched down (0 = have not taken off yet)

    // updated by UpdatePreviousFieldsPostStep
    double m_preStepPreviousAirspeed;    // airspeed @ previous timestep
    double m_preStepPreviousGearFullyUncompressedAltitude;    // altitude @ previous timestep; < 0 = none.  Accounts for gear-down distance.
    double m_preStepPreviousVerticalSpeed;  // from previous frame

    // misc flags / state data
    bool    m_isCrashed;           // true = we have crashed (vessel disabled)
    int     m_activeMultiDisplayMode;  // 0...n, or -1 if no mode set
    double  m_slope;            // ascent/descent slope in radians
    TempScale m_activeTempScale;  // 0=K, 1=F, 2=C
    double  m_apuFuelQty;   // in kg
    double  m_loxQty;       // in kg  (INTERNAL TANKS ONLY!)
    double  m_cabinO2Level; // cabin level of O2
    double  m_coolantTemp;  // in degrees C
    bool    m_internalSystemsFailure;  // if true, internal systems failed due to overheating
    bool    m_crewHatchInterlocksDisabled;       // cabin hatch switch armed
    bool    m_airlockInterlocksDisabled;           // outer airlock switch armed

    // custom autopilot data
    AUTOPILOT  m_customAutopilotMode;
    bool       m_airspeedHoldEngaged;    // special case: AIRSPEED HOLD custom autpilot engaged
    bool       m_holdAOA;                // attitudeHold: if TRUE, hold AOA instead of pitch
    double     m_setPitchOrAOA;          // attitudeHold: in degrees
    double     m_setBank;                // attitudeHold: in degrees
    bool       m_initialAHBankCompleted; // attitudeHold: true if we reached our initial bank attitude right after engaged

    double     m_setDescentRate;         // descentHold: in m/s
    double     m_latchedAutoTouchdownMinDescentRate;  // descentHold: targetRate @ final auto-land phase
    bool       m_autoLand;               // descentHold: true = perform auto landing
    double     m_setAirspeed;            // airspeedHold: in m/s

    // crew status
    CrewState m_crewState;     // OK, INCAPACITATED, DEAD

    // engine gimbaling states; true = currently active
    bool    m_mainPitchCenteringMode;
    bool    m_mainYawCenteringMode;
    bool    m_mainDivMode;
    bool    m_mainAutoMode;
    bool    m_hoverCenteringMode;
    bool    m_scramCenteringMode;

    // crew display state
    int  m_crewDisplayIndex;   // 0 - MAX_CREW_COMPLEMENT : index into CrewMembers structures in config

    // center-of-gravity shift data
    bool    m_cogShiftAutoModeActive;  
    bool    m_cogShiftCenterModeActive; 
    bool    m_cogForceRecenter;         // set to TRUE to force the ship to recenter even if AUTO is engaged; e.g., when the autopilot wants to re-center
    double  m_centerOfLift;             // current center-of-lift on the wings; NOTE: this is persisted on the Attitude Hold data line in the scenario file
    double  m_wingBalance;              // necessary to re-create main airfoils
	bool	m_parkingBrakesEngaged;		// TRUE if the parking brakes are engaged

    // END persisted data section

#ifdef _DEBUG
    // for tweaking only
    double m_tweakedInternalValue;
#endif

    //--------------------------------------------------------------
    // XRVesselCtrl interface methods
    //--------------------------------------------------------------

    // Engine State: these methods return TRUE if XREngineID is valid for this ship and (for Set) the operation succeeded; otherwise, the methods do nothing.
    // Remember that not all engines support all fields in XREngineStateWrite and not all ships support all engine types in XREngineID.
    virtual bool SetEngineState(XREngineID id, const XREngineStateWrite &state);
    virtual bool GetEngineState(XREngineID id, XREngineStateRead &state) const;  
    
    // Door State
    virtual bool SetDoorState(XRDoorID id, XRDoorState state);            // returns TRUE if door is valid for this ship
    virtual XRDoorState GetDoorState(XRDoorID id, double *pProc = nullptr) const;  // returns XRDS_DoorNotSupported if door does not exist for this ship; if pProc != nullptr, proc is set to 0 <= n <= 1.0

    // Repairs all damaged systems
    virtual bool ClearAllXRDamage();  // returns true if this call is supported by this vessel

    // Get/set the status of the XR vessel
    virtual void GetXRSystemStatus(XRSystemStatusRead &status) const;
    bool SetXRSystemStatus(const XRSystemStatusWrite &status);

    // Kill all autopilots
    virtual void KillAutopilots();

    // Standard Autopilot status (on/off only)
    virtual XRAutopilotState SetStandardAP(XRStdAutopilot id, bool on);  // returns the new state of the autopilot, or XRAPSTATE_NotSupported if requested autopilot not supported
    virtual XRAutopilotState GetStandardAP(XRStdAutopilot id);  // cannot be const due to Orbiter core not declaring 'GetNavmodeState' const

    // Extended Autopilot methods
    virtual XRAutopilotState SetAttitudeHoldAP(const XRAttitudeHoldState &state);  // returns the new state of the autopilot, or XRAPSTATE_NotSupported if autopilot not supported
    virtual XRAutopilotState GetAttitudeHoldAP(XRAttitudeHoldState &state) const;

    virtual XRAutopilotState SetDescentHoldAP(const XRDescentHoldState &state);    // returns the new state of the autopilot, or XRAPSTATE_NotSupported if autopilot not supported
    virtual XRAutopilotState GetDescentHoldAP(XRDescentHoldState &state) const;

    virtual XRAutopilotState SetAirspeedHoldAP(const XRAirspeedHoldState &state);  // returns the new state of the autopilot, or XRAPSTATE_NotSupported if autopilot not supported
    virtual XRAutopilotState GetAirspeedHoldAP(XRAirspeedHoldState &state) const;

    // Exterior lights: true = ON, false = OFF
    virtual bool SetExteriorLight(XRLight light, bool state);
    virtual bool GetExteriorLight(XRLight light) const;
    
    // Secondary HUD mode (1-5) : 0 = OFF
    virtual bool SetSecondaryHUDMode(int modeNumber);
    virtual int GetSecondaryHUDMode() const;
    
    // Enable/disable tertiary HUD
    virtual bool SetTertiaryHUDState(bool on);
    virtual bool GetTertiaryHUDState() const;

    // Reset the MWS (Master Warning System) alarm; note that under certain conditions the MWS cannot be reset (e.g., after a vessel crash)
    virtual bool ResetMasterWarningAlarm();    // returns true if MWS alarm reset successfully, false if the alarm cannot be reset

    // Center-of-Gravity shift
    virtual double GetCenterOfGravity() const;   // 0.0 = centered; +/- max value varies by vessel
    virtual bool   ShiftCenterOfGravity(double requestedShift);  // requestedShift = requested delta in meters from the current center-of-gravity; returns true on success, false if APU offline or if shift is maxed out

    // RCS Mode
    virtual bool IsRCSDockingMode() const;    // returns true if RCS DOCKING mode is active, false if RCS is in NORMAL mode
    virtual bool SetRCSDockingMode(bool on);  // set or clear RCS docking mode; returns true on success, false if RCS docking mode not supported

    // Active EVA port
    virtual bool IsElevatorEVAPortActive() const;  // returns true if crew elevator is the active EVA port, false if the docking port is active
    virtual bool SetElevatorEVAPortActive(bool on);  // true = crew elevator active, false = docking port active.  Returns true on success, false if crew elevator not supported.

    // Retrieve info/warning lines on status screen and tertiary HUD
    virtual int GetStatusScreenText(char *pLinesOut, const int maxLinesToRetrieve) const;

    // Returns a pointer to this vessel's UMMUCREWMANAGMENT object
    virtual OMMUManagement *GetMMuObject()
    { 
#ifdef MMU
        return &UMmu; 
#else
        return nullptr;
#endif
    } 

    // Note: '&' character in the message string will generate a newline; tertiary HUD has approximately 38 characters per line.
    virtual void WriteTertiaryHudMessage(const char *pMessage, const bool isWarning);

    // Returns the name of the custom skin loaded for this vessel, if any.  NULL = no custom skin loaded.  
    // e.g., if "SKIN foobar" is in the scenario file for this XR vessel, GetCustomSkinName() returns a pointer to the string "foobar".
    virtual const char *GetCustomSkinName() const;

    // utilty methods
    static DoorStatus ToDoorStatus(XRDoorState state);
    static XRDoorState ToXRDoorState(DoorStatus status);
    static int GetNavmodeForXRStdAutopilot(XRStdAutopilot id);

    // API methods added in XRVesselCtrl version 3.0
    virtual int GetPayloadBaySlotCount() const;
    virtual bool IsPayloadBaySlotFree(const int slotNumber) const;
    virtual bool GetPayloadSlotData(const int slotNumber, XRPayloadSlotData &slotDataOut);
    virtual bool CanAttachPayload(const OBJHANDLE hPayloadVessel, const int slotNumber) const;
    virtual bool GrapplePayloadModuleIntoSlot(const OBJHANDLE hPayloadVessel, const int slotNumber);
    virtual bool DeployPayloadInFlight(const int slotNumber, const double deltaV);
    virtual bool DeployPayloadWhileLanded(const int slotNumber);
    virtual int DeployAllPayloadInFlight(const double deltaV);
    virtual int DeployAllPayloadWhileLanded();
    
    virtual bool SetMWSTest(bool bTestMode);
    virtual bool GetRecenterCOGMode() const;
    virtual bool SetRecenterCOGMode(const bool bEnableRecenterMode);
    virtual XRDoorState GetExternalCoolingState() const;
    virtual bool SetExternalCoolingState(const bool bEnabled);
    virtual bool SetCrossFeedMode(XRXFEED_STATE state);

    //=====================================================================

    //
    // XRSound (formerly OrbiterSound)
    //
    const char *m_pXRSoundPath;
    XRSound *m_pXRSound;

    // NOTE: sound IDs must start at 1, not 0!
    enum Sound
    { 
        NO_SOUND = 0,
        SwitchOn, 
        SwitchOff, 
        Off,
        Rotation,
        Translation,
        GearUp,
        GearDown,
        Error1,             // beep that plays when the user's input request could not be performed
        OneHundredKnots,
        V1,         // 10
        Rotate,    
        GearLocked,         // reloaded on demand; handles "gear up and locked" and "gear down and locked"
        WarningBeep,         
        Pitch,
        On,
        BeepHigh,
        BeepLow,
        AutopilotOn,
        AutopilotOff,
        RetroDoorsAreClosed, // 20
        MachCallout,         // slot is reloaded on demand
        AltitudeCallout,     // slot is reloaded on demand; also use for docking callouts
        SonicBoom,
        Ambient,             // slot is reloaded on demand
        Warning,             // slot is reloaded on demand
        Info,                // slot is reloaded on demand
        ScramJet,
        GearWhine,          
        GearLockedThump,
        Crash,               // 30
        ErrorSoundFileMissing,  // only invoked during debugging
        // Door sounds; these slots are loaded on demand
        // these are prefixed with 'd' so as not to conflict with other enums
        dAirlockLadder,
        dNosecone,
        dOuterDoor,
        dInnerDoor,
        dAirbrake,
        dCabinHatch,
        dRadiator,           
        dRetroDoors,
        dHoverDoors,        // 40
        dScramDoors,
        // End door sounds
        APU,                // reloaded on demand
        FuelResupply,       // fuel or lox flowing during resupply
        FuelCrossFeed,
        FuelDump,
        SupplyHatch,         // hatch opened/closed thump
        FuelResupplyLine,    // reloaded on demand: handles extend and thump
        LoxResupplyLine,     // reloaded on demand: handles extend and thump
        FuelLoad,
        HoverDoorsAreClosed, // 50
        ScramDoorsAreClosed, // 51
        Chamber,             // 52 airlock chamber pressurization/depressurization
        ExternalCoolingLine, // 53
        /* Not used anymore: handled by XRSound 
        MainEngines,         // 54 includes RETRO engine sounds as well
        HoverEngines,        // 55
        RCSAttack,           // 56
        RCSSustain,          // 57
        */
        // TODO: see if I can remove this workaround now that I've switched to XRSound
        // these sounds are for SUBCLASSES to use if desired; due to an OrbiterSound 3.5 bug, you have to work from low -> high
        Subclass1 = 58,     // 58
        Subclass2,          // 59
        Subclass3,          // 60

        // NEW since XRSound has no slot limit
        WheelChirp,         // 61
        TiresRolling        // 62
    };  

    // enum defining different classes of sounds
    enum SoundType { ST_AudioStatusGreeting, ST_VelocityCallout, ST_AltitudeCallout, ST_DockingDistanceCallout, ST_InformationCallout, ST_RCSStatusCallout, ST_AFStatusCallout, ST_WarningCallout, ST_Other, ST_None };

    void LoadXR1Sound(const Sound sound, const char *pFilename, XRSound::PlaybackType playbackType);
    void PlaySound(Sound sound, const SoundType soundType, int volume = 255, bool bLoop = false);
    void StopSound(Sound sound);
    bool IsPlaying(Sound sound) const;
    bool IsXRSoundLoaded() const { return m_pXRSound->IsPresent(); }
    
    void XRSoundOnOff(const XRSound::DefaultSoundID defaultSoundID, const bool bOn) 
    { 
        if (IsXRSoundLoaded()) 
            m_pXRSound->SetDefaultSoundEnabled(defaultSoundID, bOn);
    }

    void PlayErrorBeep();
    void PlayDoorSound(DoorStatus doorStatus);
    void PlayGearLockedSound(bool isGearUp);
    void DoCrash(const char *pMsg, double touchdownVerticalSpeed);
    void DoGearCollapse(const char *pMsg, double touchdownVerticalSpeed, bool setGearAnimState);
    void FailGear(bool setGearAnimState);
    void FailDoor(double &doorProc, UINT anim);
    bool CheckHydraulicPressure(bool playWarning, bool playErrorBeep);
    void CloseFuelHatch(bool playSound);
    void CloseLoxHatch(bool playSound);
    void CloseExternalCoolingHatch(bool playSound);
    void KillAllAttitudeThrusters();
    void NeutralAllControlSurfaces();
    void DecompressCabin();
    bool VerifyManualCOGShiftAvailable();
    DWORD GetLowerPanelMoveoutFlag() { return (GetXR1Config()->Lower2DPanelVerticalScrollingEnabled ? PANEL_MOVEOUT_TOP : 0); }  // convenience method
    void ResetMET();
    void SetCrossfeedMode(const XFEED_MODE mode, const char *pMsg);
    void SetFuelDumpState(bool &fuelDumpInProgress, const bool isDumping, const char *pFuelLabel);
    void SetLOXDumpState(const bool isDumping);
    bool RequestExternalCooling(const bool bEnableExternalCooling);
	
	bool MainThrustApplied() const
	{
		const double totalThrustLevel = GetThrusterLevel(th_main[0]) + GetThrusterLevel(th_main[1]);
		return (totalThrustLevel > 0);
	}

	bool HoverThrustApplied() const
	{
		const double totalThrustLevel = GetThrusterLevel(th_hover[0]) + GetThrusterLevel(th_hover[1]);
		return (totalThrustLevel > 0);
	}

	bool RetroThrustApplied() const
	{
		const double totalThrustLevel = GetThrusterLevel(th_retro[0]) + GetThrusterLevel(th_retro[1]);
		return (totalThrustLevel > 0);
	}

	bool ScramThrustApplied() const
	{
		const double totalThrustLevel = GetThrusterLevel(th_scram[0]) + GetThrusterLevel(th_scram[1]);
		return (totalThrustLevel > 0);
	}

	bool RCSThrustApplied() const
	{
		double totalThrustLevel = 0;
		for (int i = 0; i < 14; i++)
			totalThrustLevel += GetThrusterLevel(th_rcs[i]);
		return (totalThrustLevel > 0);
	}
    
    // autopilot button handlers
    void IncrementAttitudeHoldPitch(bool playSound, bool changeAxis, double stepSize);
    void DecrementAttitudeHoldPitch(bool playSound, bool changeAxis, double stepSize);
    void IncrementAttitudeHoldBank(bool playSound, bool changeAxis);   // bank left
    void DecrementAttitudeHoldBank(bool playSound, bool changeAxis);   // bank right
    void ResetAttitudeHoldToLevel(bool playSound, bool resetPitch, bool resetBank);
    void SyncAttitudeHold(bool playSound, bool forcePitchHoldMode);
    void ToggleAOAPitchAttitudeHold(bool playSound);
    void SetAirspeedHoldMode(bool on, bool playSound);
    virtual void SetCustomAutopilotMode(AUTOPILOT mode, bool playSound, bool force = false);  // this is virtual so the sublcass can hook it
    void SetAutoDescentRate(bool playSound, const AUTODESCENT_ADJUST mode, double delta);
    void SetAirspeedHold(bool playSound, const AIRSPEEDHOLD_ADJUST mode, double delta);
    void ToggleDescentHold();
    void ToggleAttitudeHold();
    void ToggleAirspeedHold(bool holdCurrent);
    virtual void ParsePRPLevel(const char *line, const int nameLen);
    void SetMDAModeForCustomAutopilot();
    void SetRecenterCenterOfGravityMode(const bool bEnabled);

    // utility methods
    // validate a fraction and keep it in-bounds (0...1)
    // fraction = value to test and adjust as necessary
    // Returns: true if fraction OK, false if it was adjusted
    template<class T> static bool ValidateFraction(T &frac)
    {
        return Validate(frac, static_cast<T>(0.0), static_cast<T>(1.0));
    }

    // validate a value and keep it in-bounds 
    // val = value to test and adjust as necessary
    // low,high = inclusive valid values
    // Returns: true if value OK, false if it was adjusted
    template<class T> static bool Validate(T &val, const T low, const T high)
    {
        bool retVal = false; // assume out-of-bounds

        if (val < low)
            val = low;
        else if (val > high)
            val = high;
        else
            retVal = true;  // value OK

        return retVal;
    }


    static void FormatDouble(const double val, CString &out, const int decimalPlaces);
    void TriggerNavButtonRedraw();
    void FatalError(const char *pMsg);   // show error message box and terminate
    double GetThrusterFlowRate(const THRUSTER_HANDLE th) const;

    // EVA/UMmu support methods
    virtual bool PerformEVA(const int mmuCrewMemberIndex);
    virtual bool CheckEVADoor();
    int GetMmuSlotNumberForName(const char *pName) const;
    bool IsCrewMemberOnBoard(const int index) const;

    // XR1 crew member 'Misc' format: "P0", "P1", etc.
    static int ExtractIndexFromMmuMisc(const char *pMisc);
    const char *RetrieveRankForMmuMisc(const char *pMisc) const;
    const char *RetrieveMeshForMmuMisc(const char *pMisc) const;
    // end section

    // shared data computed only once per frame for efficiency
    VECTOR3 m_acceleration;   // in m/s^2
    VECTOR3 m_F;              // force vector
    VECTOR3 m_W;              // weight vector

    void ShowWarning(const char *pSoundFilename, const SoundType soundType, const char *pMessage, bool force=false);
    void ShowInfo(const char *pSoundFilename, const SoundType soundType, const char *pMessage);

    // Info/Warning message lines; used primarily by the tertiary HUD
    TextLineGroup m_infoWarningTextLineGroup;
    
    char m_lastWarningMessage[MAX_MESSAGE_LENGTH];

    // the warning PostStep polls these values to see what to display
    bool m_forceWarning;    // if true, always display warning
    char m_warningWavFilename[256];
    SoundType m_warningWaveSoundType;
    
    // NOTE: this may be a real crash message, or it may be a (possibly) temporary condition
    // such as low O2 levels.
    char m_crashMessage[MAX_MESSAGE_LENGTH];

    char m_lastWavLoaded[MAX_PATH]; // last sound file loaded
    char m_hudWarningText[MAX_MESSAGE_LENGTH];  // displayed on the HUD 

    // warning font for critical HUD messages
    oapi::Font *m_pHudWarningFont;
    int   m_pHudWarningFontSize;   // vertical size in pixels incl. spacing

    // new HUD font for normal text (designed to match new HUD look in Orbiter 2010)
    oapi::Font *m_pHudNormalFont;
    int m_pHudNormalFontSize;

    // data HUD font
    oapi::Font *m_pDataHudFont;
    int   m_pDataHudFontSize;   // vertical size in pixels incl. spacing

    // timestamp that last hydraulic (APU-driven) door was running; NOTE: excludes AF CTRL surfaces
    double m_latestHydraulicDoorRunningSimt;

    // Set to true if our PreventAutoRefuelingPostStep just backed out and Orbiter core refueling this frame;
    // is only set for a single frame at a time.
    bool m_backedOutOrbiterCoreAutoRefuelThisFrame;

    // Orbiter won't save or load spaces in params, so we work around it
    void EncodeSpaces(char *pStr);
    void DecodeSpaces(char *pStr);

    // Inline method to retrieve crew member count
    int GetCrewMembersCount() const
    {
        if (m_crewState == DEAD)
            return 0;

        return GetCrewTotalNumber();
    }

    // New MMU wrapper methods added for removing MMU reliance
    int GetCrewTotalNumber() const;
    const char *GetCrewNameBySlotNumber(const int index) const;
    int GetCrewAgeByName(const char *pName) const;
    const char *GetCrewMiscIdByName(const char *pName) const;

    // retrieve the effective "gear down" altitude; i.e., this is "altitude to touchdown"
    const double GetGearFullyUncompressedAltitude()
    {
        double altitude = GetAltitude(ALTMODE_GROUND);
        
        if (GroundContact())
        {
            // if no gear compression, don't show "-0.0" as the altitude
            // otherwise, show altitude as negative since gear is fully compressed
            altitude = ((GEAR_COMPRESSION_DISTANCE == 0) ? 0 : -GEAR_COMPRESSION_DISTANCE);
        }
        else if (gear_status != DOOR_CLOSED)
            altitude -= GEAR_FULLY_UNCOMPRESSED_DISTANCE;   // adjust for gear down

        return altitude;
    }

    // retrieve the "hover engine cutoff gear down" altitude; i.e., this is "altitude full vertical stop"
    const double GetGearFullyCompressedAltitude()
    {
        double altitude = GetAltitude(ALTMODE_GROUND);
        
        if (GroundContact())
            altitude = 0;
        else if (gear_status != DOOR_CLOSED)
            altitude -= GEAR_FULLY_COMPRESSED_DISTANCE;   // adjust for gear down

        return altitude;
    }

    // overridden base class methods
    virtual bool clbkPanelRedrawEvent(int areaID, int event, SURFHANDLE surf);

    // payload bay methods for subclasses to use; these are not linked into the XR1
    virtual bool DeployPayload(const int slotNumber, const bool showMessage);
    virtual int DeployAllPayload();
    virtual bool ValidateBayStatus(const bool isPayloadRequiredInSlot, const int slotNumber, const bool showMessage);
    virtual bool VerifySlotSelected(const bool showMessage);
    virtual void RefreshGrappleTargetsInDisplayRange();        // update internal list of vessels in range of 'GRAPPLE_DISPLAY_RANGES[m_grappleRangeIndex]'
    virtual void AdjustPayloadDeployDeltaV(double adjustment, const bool showMessage);
    virtual void SetPayloadDeployDeltaV(double deltaV, const bool showMessage);
    virtual double GetGrappleDisplayRange() const { return GRAPPLE_DISPLAY_RANGES[m_grappleRangeIndex]; }
    virtual bool IsGrappleTargetVesselValidAndInDisplayRange(const XRGrappleTargetVessel *pGrappleTargetVessel) const;
    virtual bool GrapplePayload(const int slotNumber, const bool showMessage);
    virtual int GrappleAllPayload();
    virtual void IncGrappleRange(const bool playSound);
    virtual bool AdjustGrappleTarget(const int adjustment, const bool playBeep);
    virtual double GetPayloadGrappleRangeLimit() const { return (IsLanded() ? PAYLOAD_GRAPPLE_RANGE_LANDED : PAYLOAD_GRAPPLE_RANGE_ORBIT); }
    virtual void ClearGrappleTarget(bool playBeep);
    virtual double GetPayloadMass() const;
    void TogglePayloadEditor();
    // No way to do this: bool TrackGrappleTarget(bool showMessage);
    
    // TRANSIENT payload data; used only by subclasses!
    ATTACHMENTHANDLE m_dummyAttachmentPoint; 
    XRPayloadBay *m_pPayloadBay;
    double m_nextPayloadScreensRefresh[3];  // simt of next refresh for our three screens
    vector<const XRGrappleTargetVessel *> m_xrGrappleTargetVesselsInDisplayRange;   // list of XRGrappleTargetVessel objects; may be empty
    static HWND s_hPayloadEditorDialog;     // if non-zero, contains the window handle of the payload editor dialog; this is GLOBAL across all Ravenstar vessels since the dialog is a singleton
    // subclass bay doors, if any; these are not referenced by our class here
    UINT anim_bay;
    DoorStatus bay_status;
    double bay_proc;
    int m_requestSwitchToTwoDPanelNumber;
    CAMERA_POSITION campos;   // camera position; i.e., which instrument panel is active?
    bool m_SCRAMTankHidden;   // if true, internal SCRAM tank will be emptied by DrainBayFuelTanksPreStep unless a non-empty tank is in the payload bay
    double m_MainFuelFlowedFromBayToMainThisTimestep;   // updated every timestep: set by DrainBayFuelTanksPreStep
    double m_SCRAMFuelFlowedFromBayToMainThisTimestep;  // updated every timestep: set by DrainBayFuelTanksPreStep

    // PERSISTED payload data; used only by subclasses!
    double m_deployDeltaV;
    int m_grappleRangeIndex;   // index into array: 0-n
    char m_grappleTargetVesselName[128];  // Note: may be empty, or vessel may not exist!  Pass this name to GetGrappleTargetVessel(...) to obtain grapple target data.
    int m_selectedSlotLevel;   // 1 to level_count; this is valid regardless of whether any slot is selected
    int m_selectedSlot;        // 1 to slot_count, or 0 if NO slot selected

    // wheel rotation animation; used only by subclasses!
    UINT m_animFrontTireRotation;
    UINT m_animRearTireRotation;

    // gear compression animation; only used by subclasses!
    UINT m_animNoseGearCompression;     // animation handle
    UINT m_animRearGearCompression;     // both rear struts always compress as a pair 
    double m_noseGearProc, m_rearGearProc;  // set by GearCompressionPrestep; 1.0 = fully uncompressed, 0 = fully compressed

protected:
    virtual bool InitSound(); 
    virtual void UpdateVCStatusIndicators();  // moved here
    virtual void FailAileronsIfDamaged();
    void ParseXRConfigFile();  // all XR vessels should invoke this from the beginning of clbkSetClassCaps

    // invoked by AddXRExhaust methods below
    static EXHAUSTSPEC *GetExhaustSpec(const THRUSTER_HANDLE th, const double lscale, const double wscale, const VECTOR3 *pos, const VECTOR3 *dir, const SURFHANDLE tex = 0);

    // these gateway methods automatically set some flame magnitude variance (flickering); new for Orbiter 2010
    // Note: XR vessels should *only* invoke AddXRExhaust, not AddExhaust
    unsigned int AddXRExhaust(const THRUSTER_HANDLE th, const double lscale, const double wscale, const SURFHANDLE tex = 0);
    unsigned int AddXRExhaust(const THRUSTER_HANDLE th, const double lscale, const double wscale, const VECTOR3 &pos, const VECTOR3 &dir, const SURFHANDLE tex = 0);

    double CheckTemperature(double tempK, double limitK, bool doorOpen);
    double CheckScramTemperature(double tempK, double limitK);
    virtual bool CheckDoorFailure(DoorStatus *doorStatus);

	virtual void ApplySkin();                     // apply custom skin

    // attitude hold utility methods
    void LimitAttitudeHoldPitchAndBank(const bool incrementingBank);
    static void LimitAttitudeHoldPitch(double &val, const double limit);
    void LimitAttitudeHoldBank(const bool increment, double &val, const double limit);

	void ScramjetThrust();                       // scramjet thrust calculation

	double max_rocketfuel, max_scramfuel;        // max capacity for rocket and scramjet fuel
	SURFHANDLE skin[3];                          // custom skin textures, if applicable
	char skinpath[MAX_PATH];                     // skin directory, if applicable

    virtual void ReinitializeDamageableControlSurfaces();  // creates control surfaces for any handles below that are zero
	CTRLSURFHANDLE hLeftAileron, hRightAileron, hElevator, hElevatorTrim;         // control surface handles

    // custom refresh data
    double m_nextMDARefresh;            // simt for next PANEL_REDRAW_ALWAYS event
    double m_nextSecondaryHUDRefresh;   // simt for next PANEL_REDRAW_ALWAYS event
    double m_nextTertiaryHUDRefresh;    // simt for next PANEL_REDRAW_ALWAYS event
    double m_nextArtificialHorizonRefresh;  // simt for next PANEL_REDRAW_ALWAYS event

    // map of areaID -> simt of next update (only contains PANEL_REDRAW_ALWAYS areas)
    typedef unordered_map<unsigned int, double> HASHMAP_UINT_DOUBLE;
    typedef pair<unsigned int, double> uint_double_Pair;
    HASHMAP_UINT_DOUBLE m_nextRedrawAlwaysRefreshMap;  // key=area ID, value = simt of next update for this component

    // bitmask that tracks all config file overrides that were loaded with this scenario
#define CONFIG_OVERRIDE_MainFuelISP               0x00000001
#define CONFIG_OVERRIDE_SCRAMFuelISP              0x00000002
#define CONFIG_OVERRIDE_LOXConsumptionRate        0x00000004
#define CONFIG_OVERRIDE_LOXConsumptionMultiplier  0x00000008
#define CONFIG_OVERRIDE_APUFuelBurnRate           0x00000010
#define CONFIG_OVERRIDE_CoolantHeatingRate        0x00000020
    unsigned m_configOverrideBitmask;
};

// door sound structure; must be defined AFTER the XR1 class
struct DoorSound
{
    const DoorStatus *pDoorStatus;      // points to DeltaGliderXR1 member variable
    DoorStatus prevDoorStatus;          // value from previous timestep
    DeltaGliderXR1::Sound soundID;      // sound ID to play for this door
    bool processAPUTransitionState;     // true if we are armed to process an APU OFF transition
    const char *pLabel;                 // "Nosecone", "Retro Doors", etc.
};

