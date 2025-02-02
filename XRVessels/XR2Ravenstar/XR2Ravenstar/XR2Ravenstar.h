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
// XR2Ravenstar implementation class
//
// XR2Ravenstar.h
// Class interface for the XR2 Ravenstar
// ==============================================================

#pragma once

#include "DeltaGliderXR1.h"
#include "XR2ConfigFileParser.h"

// Forward references
class MultiDisplayArea;
class XR2PayloadBay;

// ==========================================================
// Interface for derived vessel class: XR2Ravenstar
// ==========================================================

#define MESH_SECRET_KEY 
class XR2Ravenstar : public DeltaGliderXR1
{
public:
	XR2Ravenstar(OBJHANDLE hObj, int fmodel, XR2ConfigFileParser *pConfigFileParser);
	virtual ~XR2Ravenstar();

    // Mandatory overridden methods
    virtual void SetDamageVisuals();     
    virtual void SetPassengerVisuals();

    // Mandatory if vessel has a VC
    virtual void UpdateVCStatusIndicators();
    virtual DWORD MeshTextureIDToTextureIndex(const int meshTextureID, MESHHANDLE &hMesh);  

    // overridden callback functions
    virtual void clbkSetClassCaps (FILEHANDLE cfg);
    virtual void clbkPostCreation ();
    virtual bool clbkPlaybackEvent (double simt, double event_t, const char *event_type, const char *event);
    virtual void clbkVisualCreated (VISHANDLE vis, int refcount);
    virtual void clbkVisualDestroyed (VISHANDLE vis, int refcount);
    virtual bool clbkLoadVC(int id);  // NOTE: OVERRIDING THIS IS MANDATORY!
    virtual void clbkLoadStateEx(FILEHANDLE scn, void *vs);
    virtual void clbkSaveState (FILEHANDLE scn);
    virtual int  clbkConsumeBufferedKey(DWORD key, bool down, char *kstate);
    virtual bool clbkLoadGenericCockpit();
    virtual void clbkADCtrlMode(DWORD mode);
    virtual bool clbkPanelRedrawEvent(int areaID, int event, SURFHANDLE surf);

    // overridden superclass methods
    virtual void SetXRAnimation(const UINT &anim, const double state) const;
    virtual void DefineAnimations();
    virtual void CleanUpAnimations();
    virtual void TweakInternalValue(bool direction);  // used for developement testing only; usually an empty method
    virtual void ApplySkin();
    virtual void PerformCrashDamage();
    virtual bool CheckAllDoorDamage();
    virtual bool CheckHullHeatingDamage();
    virtual bool IsWarningPresent();
    virtual const DamageStatus &GetDamageStatus(DamageItem item) const;
    virtual void SetDamageStatus(DamageItem item, double fracIntegrity);
    virtual bool CheckDoorFailure(DoorStatus *doorStatus);
    virtual void SetGearParameters(double state);
    virtual bool PerformEVA(const int mmuCrewMemberIndex);

    virtual bool InitSound();
    virtual void ReinitializeDamageableControlSurfaces();  // creates control surfaces for any handles below that are zero

    // convenience method
    XR2ConfigFileParser *GetXR2Config() { return static_cast<XR2ConfigFileParser *>(m_pConfig);  }

    // payload methods
    void ResetCameraToPayloadBay();
    void CreatePayloadBay();
    
    // other methods
    void HideActiveVCHUDMesh();
    void ApplyElevatorAreaChanges();

    // new state data that is NOT persisted
    bool m_xr2WarningLights[XR2_WARNING_LIGHT_COUNT];

    // nosewheel steering animation
    // NO: UINT m_animNosewheelSteering;       // animation handle
    // no proc necessary for this; follows rudder

    // We use these to mimic "limited deflection" for dual-mode AF Ctrl settings.
    double m_baselineElevatorArea;
    double m_ctrlSurfacesDeltaZ;
    double m_elevatorCL;   // elevator coefficient of lift
    double XR1Multiplier;  // control surface area vs. the XR1   (used to be a local; only used by subclasses)

    //--------------------------------------------------------------
    // Vessel-specific XRVesselCtrl interface methods
    //--------------------------------------------------------------

    virtual bool SetDoorState(XRDoorID id, XRDoorState state);
    virtual XRDoorState GetDoorState(XRDoorID id, double *pProc) const;
    virtual bool SetXRSystemStatus(const XRSystemStatusWrite &status);
    virtual void GetXRSystemStatus(XRSystemStatusRead &status);

protected:
    // child mesh groups
    MGROUP_ROTATE *m_frontWheels;
    MGROUP_ROTATE *m_rearWheels;
    MGROUP_ROTATE *m_rearSwingarms;
    MGROUP_TRANSLATE *m_aftInnerStruts;
    MGROUP_TRANSLATE *m_frontInnerStrut;
};

// Define custom sound slots we need from the XR1's Sound enum
// TODO: see if I can remove this workaround now that I've switched to XRSound
// WARNING: OrbiterSound Bug: it expects all loaded sequences to be *sequential*; i.e., no gaps.
// Therefore, work from low -> high
#define dPayloadBayDoors Subclass1
