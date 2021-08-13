/**
  XR Vessel add-ons for OpenOrbiter Space Flight Simulator
  Copyright (C) 2006-2021 Douglas Beachy

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

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
// XR3Phoenix implementation class
//
// XR3Phoenix.h
// Class interface for XR3 Phoenix vessel class module
// ==============================================================

#pragma once

// include base class
#include "DeltaGliderXR1.h"

#include "XR3ConfigFileParser.h"

class XR3PayloadBay;

class XR3Phoenix : public DeltaGliderXR1
{
public:
	XR3Phoenix(OBJHANDLE hObj, int fmodel, XR3ConfigFileParser *pConfigFileParser);
	virtual ~XR3Phoenix();

    // Mandatory overridden methods
    virtual void SetDamageVisuals();     
    virtual void SetPassengerVisuals();
    DWORD MeshTextureIDToTextureIndex(const int meshTextureID, MESHHANDLE &hMesh);  // method is empty

    // overloaded callback functions
    virtual void clbkSetClassCaps(FILEHANDLE cfg);
    virtual void clbkPostCreation();
    virtual bool clbkPlaybackEvent(double simt, double event_t, const char *event_type, const char *event);
	virtual void clbkVisualCreated(VISHANDLE vis, int refcount);
	virtual void clbkVisualDestroyed(VISHANDLE vis, int refcount);
    virtual void clbkLoadStateEx(FILEHANDLE scn, void *vs);
    virtual void clbkSaveState (FILEHANDLE scn);
    virtual int clbkConsumeDirectKey (char *kstate);
    virtual int clbkConsumeBufferedKey(DWORD key, bool down, char *kstate);
    virtual bool clbkPanelRedrawEvent(int areaID, int event, SURFHANDLE surf);
    virtual bool clbkLoadGenericCockpit();

    virtual void UpdateCtrlDialog(XR3Phoenix *dg, HWND hWnd = nullptr);
    virtual void DefineAnimations();

    virtual bool clbkLoadVC(int id) { return false; }   // Must KEEP THIS LINE to prevent VC from loading!
    virtual void ActivateLadder(DoorStatus action) { }  // prevent bogus activation
    virtual void ShowHatchDecompression();
    virtual void CleanUpHatchDecompression();
    virtual bool CheckEVADoor();

    // active EVA port enum
    enum ACTIVE_EVA_PORT { DOCKING_PORT, CREW_ELEVATOR };

    // overidden XR1 methods
#ifdef TOO_MESSY
    virtual double GetRCSThrustMax(const int index);
#endif
    virtual void ActivateBayDoors(DoorStatus action); 
    virtual void SetCustomAutopilotMode(AUTOPILOT mode, bool playSound, bool force = false);
    virtual void clbkNavMode (int mode, bool active);
    virtual void SetGearParameters (double state);
    virtual void PerformCrashDamage();
    virtual bool CheckAllDoorDamage();
    virtual bool CheckHullHeatingDamage();
    virtual bool IsWarningPresent();
    virtual const DamageStatus &GetDamageStatus(DamageItem item) const;
    virtual void SetDamageStatus(DamageItem item, double fracIntegrity);
    virtual bool CheckDoorFailure(DoorStatus *doorStatus);
    virtual void CleanUpAnimations();   // invoked by XR1's destructor
    virtual void ActivateRadiator(DoorStatus action);
    virtual void ActivateLandingGear(DoorStatus action);
    virtual void TweakInternalValue(bool direction);  // used for developement testing only; usually an empty method
    virtual double GetRCSThrustMax(const int index) const;
	virtual void ApplySkin();
    virtual void ReinitializeDamageableControlSurfaces();  // creates control surfaces for any handles below that are zero

    // convenience method
    XR3ConfigFileParser *GetXR3Config() { return static_cast<XR3ConfigFileParser *>(m_pConfig);  }

    // new methods
    bool SetRCSDockingMode(bool dockingMode);
    void ConfigureRCSJets(bool dockingMode);
    void ActivateElevator(DoorStatus action);
    void ToggleElevator();
    void DefineMmuAirlock();
    void ResetCameraToPayloadBay();
    void CreatePayloadBay();
    void SetActiveEVAPort(ACTIVE_EVA_PORT newState);

    // WARNING: All code should invoke SetXRAnimation instead of SetXRAnimation!  The reason is that 
    // SetXRAnimation always assumes that the handle is valid, and so SetXRAnimation is a virtual "gateway" 
    // method that allows each subclass to determine whether that animation is valid or not for that vessel.
    virtual void SetXRAnimation(const UINT &anim, const double state) const;

    // mesh indicies
    UINT m_exteriorMeshIndex;

    // our custom doors
    UINT anim_crewElevator;
    DoorStatus crewElevator_status;
    double crewElevator_proc;

    // new PERSISTED state data
    bool m_rcsDockingMode;  // true = docking mode, false = normal mode
    ACTIVE_EVA_PORT m_activeEVAPort;

    // new state data that that is NOT persisted
    bool m_rcsDockingModeAtKillrotStart;
    bool m_XR3WarningLights[XR3_WARNING_LIGHT_COUNT];
    double m_hiddenElevatorTrimState;   // fixes nose-up push

    // nosewheel steering animation
    UINT m_animNosewheelSteering;       // animation handle
    // no proc necessary for this; follows rudder

    //--------------------------------------------------------------
    // XRVesselCtrl interface methods
    //--------------------------------------------------------------

    // Door State
    virtual bool SetDoorState(XRDoorID id, XRDoorState state);            // returns TRUE if door is valid for this ship
    virtual XRDoorState GetDoorState(XRDoorID id, double *pProc = nullptr) const;  // returns XRDS_DoorNotSupported if door does not exist for this ship; if pProc != nullptr, proc is set to 0 <= n <= 1.0

    // Set/Read the status of the XR vessel
    virtual bool SetXRSystemStatus(const XRSystemStatusWrite &status);
    virtual void GetXRSystemStatus(XRSystemStatusRead &status) const; 

    // RCS Mode
    virtual bool IsRCSDockingMode() const;    // returns true if RCS DOCKING mode is active, false if RCS is in NORMAL mode
    // SetRCSDockingMode already defined earlier

    // Active EVA port
    virtual bool IsElevatorEVAPortActive() const;  // returns true if crew elevator is the active EVA port, false if the docking port is active
    virtual bool SetElevatorEVAPortActive(bool on);  // true = crew elevator active, false = docking port active.  Returns true on success, false if crew elevator not supported.

protected:
    double m_ctrlSurfacesDeltaZ;  // distance from center of model to center of control surfaces, Z axis
    double m_aileronDeltaX;       // distance from center of ship to center of aileron, X direction
    double XR1Multiplier;         // control surface area vs. the XR1   (used to be a local; only used by subclasses)

    // child animation groups; initialized by DefineAnimations
	MGROUP_ROTATE    *m_rad_panel_right_002;
    MGROUP_ROTATE    *m_rad_panel_right_003;
	MGROUP_ROTATE    *m_rad_panel_right_004;
    MGROUP_ROTATE    *m_rad_panel_left_002;
    MGROUP_ROTATE    *m_rad_panel_left_003;
	MGROUP_ROTATE    *m_rad_panel_left_004;

    MGROUP_ROTATE    *m_radiator_door_top_right;
    MGROUP_ROTATE    *m_radiator_door_top_left;

    MGROUP_ROTATE    *m_gear_door_left_outside_2;
    MGROUP_ROTATE    *m_gear_door_right_outside_2;

    MGROUP_TRANSLATE *m_noseGearNoMovement;

    MGROUP_TRANSLATE *m_noseGearTranslation;     // contains every group attached to the main nose cylinder
    MGROUP_TRANSLATE *m_rearGearLeftTranslation;
    MGROUP_TRANSLATE *m_rearGearRightTranslation;

    // wheel rotation (5 different axles total)
    MGROUP_ROTATE    *m_rearLeftRearRotationF;
    MGROUP_ROTATE    *m_rearRightRearRotationF;
    MGROUP_ROTATE    *m_rearLeftRearRotationB;
    MGROUP_ROTATE    *m_rearRightRearRotationB;
    
    MGROUP_ROTATE    *m_frontWheelRotation;

    // elevator animation
    MGROUP_ROTATE    *m_forwardElevatorArmDoor;
    MGROUP_ROTATE    *m_rotateChildElevatorArm;
    MGROUP_ROTATE    *m_rotateElevator;

    // outer docking port door animation
    MGROUP_TRANSLATE *m_dock_port00;
    MGROUP_TRANSLATE *m_dock_port01;
    MGROUP_TRANSLATE *m_dock_port_ring;

    // outer airlock door petals
    MGROUP_ROTATE *m_door_petal[8];
};

// Define custom sound slots we need from the XR1's Sound enum
// TODO: see if I can remove this workaround now that I've switched to XRSound
// WARNING: OrbiterSound Bug: it expects all loaded sequences to be *sequential*; i.e., no gaps.
// Therefore, work from low -> high
#define dPayloadBayDoors Subclass1
#define dElevator        Subclass2
