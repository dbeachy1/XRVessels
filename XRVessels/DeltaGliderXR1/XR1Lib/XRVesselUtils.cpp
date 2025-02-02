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
// XR vessel utility methods
// ==============================================================

#include "DeltaGliderXR1.h"
#include "AreaIDs.h"
#include "XRPayloadBay.h"
#include "XR1PayloadDialog.h"
#include "XR1Areas.h"

// save the current Orbiter window coordinates; this is invoked when Orbiter exits or saves a scenario
void DeltaGliderXR1::SaveOrbiterRenderWindowPosition()
{
	// save the Orbiter render coordinates to the registry
	const HWND hOrbiterWnd = GetOrbiterRenderWindowHandle();
	if (hOrbiterWnd)    // will only be nullptr for full-screen mode
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
	if (hOrbiterWnd)    // will only be nullptr for full-screen mode
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
				bool bSuccess = m_regKeyManager.ReadRegistryDWORD(xCoordValName, (DWORD&)x);
				bSuccess &= m_regKeyManager.ReadRegistryDWORD(yCoordValName, (DWORD&)y);
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
// Verify that hydraulic pressure is present
// playWarning: true = show warning if no hydraulic pressure present
// Returns: true if hydraulic pressure OK, false if not
// --------------------------------------------------------------
bool DeltaGliderXR1::CheckHydraulicPressure(bool showWarning, bool playErrorBeep)
{
	bool retVal = true;  // assume APU is running

	if (apu_status != DoorStatus::DOOR_OPEN)   // APU not running?
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
void DeltaGliderXR1::ApplySkin()
{
	if (!exmesh) return;

	if (skin[0]) oapiSetTexture(exmesh, 1, skin[0]);
	if (skin[1]) oapiSetTexture(exmesh, 2, skin[1]);
}

#ifdef NOT_USED_ANYMORE
// Invoked by all subclasses to set touchdown points; this method is necessary to support Orbiter 2016's new gear compression settings
void DeltaGliderXR1::SetXRTouchdownPoints(const VECTOR3& pt1, const VECTOR3& pt2, const VECTOR3& pt3, const double mu_lng, const double mu_lat, const bool bIsGearDown) const
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
#else
// Invoked by all subclasses to set touchdown points; this method is necessary to support Orbiter 2016's new gear compression settings
void DeltaGliderXR1::SetXRTouchdownPoints(const VECTOR3& pt1, const VECTOR3& pt2, const VECTOR3& pt3, const double mu_lng, const double mu_lat, const bool bIsGearDown) const
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
	delete []pVtxArray;
}
#endif

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
		SetXRTouchdownPoints(_V(0, -1.5, 9), _V(-6, -0.8, -5), _V(3, -1.2, -5), 3.0, 3.0, false);   // tilt the ship -- belly landing!
		SetNosewheelSteering(false);  // not really necessary since we have a prestep constantly checking this
	}

	// update the animation state
	gear_proc = state;
	SetXRAnimation(anim_gear, gear_proc);

	// redraw the gear indicator
	TriggerRedrawArea(AID_GEARINDICATOR);
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

// Show a fatal error message box and terminate Orbiter
void DeltaGliderXR1::FatalError(const char* pMsg)
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

// Returns the flow rate of a thruster in kg/sec
double DeltaGliderXR1::GetThrusterFlowRate(const THRUSTER_HANDLE th) const
{
	double level = GetThrusterLevel(th); // throttle level
	double isp = GetThrusterIsp0(th);  // must vacuum rating here since atmosphere does not affect flow rate
	double thrust = GetThrusterMax0(th);  // must use vacuum rating here since our ISP is a vacuum ISP
	double flow = thrust * level / isp;

	return flow;
}

// all XR vessels should invoke this from clbkSetClassCaps to parse their configuration file(s)
void DeltaGliderXR1::ParseXRConfigFile()
{
	// NOTE: this should be the *only place* where ParseVesselConfig and ApplyCheatcodesIfEnabled are invoked
	m_pConfig->ParseVesselConfig(GetName());

	// now apply the cheatcodes if they are enabled
	// Note: cannot use GetXRConfig() here because we cannot make ApplyCheatcodesIfEnabled() const
	(static_cast<XR1ConfigFileParser*>(m_pConfig))->ApplyCheatcodesIfEnabled();
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
#if 0
	// adjust XRSound air conditioning pan sound (XRSound 3.0 test)
	const double stepSize = (oapiGetSimStep() * 0.2);
	const double step = stepSize * (direction ? 1.0 : -1.0);

	const float pan = m_pXRSound->GetPan(XRSound::DefaultSoundID::AirConditioning) + static_cast<float>(step);
	sprintf(oapiDebugString(), "Pan=%f", pan);

	m_pXRSound->SetPan(XRSound::DefaultSoundID::AirConditioning, pan);
#endif
#if 0
	// adjust XRSound air conditioning playback speed (XRSound 3.0 test)
	const double stepSize = (oapiGetSimStep() * 0.2);
	const double step = stepSize * (direction ? 1.0 : -1.0);

	const float speed = m_pXRSound->GetPlaybackSpeed(XRSound::DefaultSoundID::AirConditioning) + static_cast<float>(step);
	sprintf(oapiDebugString(), "PlaybackSpeed=%f", speed);

	m_pXRSound->SetPlaybackSpeed(XRSound::DefaultSoundID::AirConditioning, speed);
#endif
#if 0
	// adjust XRSound retro doors sound PlayPosition (XRSound 3.0 test)
	const int stepSize = static_cast<int>(oapiGetSimStep() * 200);
	const int step = stepSize * (direction ? 1 : -1);

	const int position = m_pXRSound->GetPlayPosition(Sound::RetroDoorsAreClosed) + step;
	sprintf(oapiDebugString(), "PlayPosition=%d", position);

	m_pXRSound->SetPlayPosition(Sound::RetroDoorsAreClosed, position);
#endif
#endif  // ifdef DEBUG
}

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


// handle the Altea Aerospace logo click easter egg 
void DeltaGliderXR1::AlteaLogoClicked()
{
	// this callout file is camouflaged
	ShowInfo("ambl.wav", ST_Other, nullptr);  // no text message for this; always play it (ST_Other)
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
	for (int i = 0; i < MAX_PASSENGERS; i++)
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

void DeltaGliderXR1::ScramjetThrust()
{
	int i;
	const double eps = 1e-8;
	const double Fnominal = 2.5 * MAX_MAIN_THRUST[GetXR1Config()->MainEngineThrust];

	double Fscram[2];
	ramjet->Thrust(Fscram);

	for (i = 0; i < 2; i++)
	{
		double level = GetThrusterLevel(th_scram[i]);
		double Fmax = Fscram[i] / (level + eps);
		SetThrusterMax0(th_scram[i], Fmax);

		// handle new configurable ISP
		const double isp = Fscram[i] / (ramjet->DMF(i) + eps) * GetXR1Config()->GetScramISPMultiplier();
		SetThrusterIsp(th_scram[i], max(1.0, isp)); // don't allow ISP=0

		// the following are used for calculating exhaust density
		scram_max[i] = min(Fmax / Fnominal, 1.0);
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

// undock the ship intelligently
void DeltaGliderXR1::PerformUndocking()
{
	if (IsDocked() == false)
	{
		PlayErrorBeep();
		ShowWarning(nullptr, DeltaGliderXR1::ST_None, "Ship is not docked.");
		return;
	}

	// safety check: prevent undocking if both airlock doors are open
	if ((olock_status != DoorStatus::DOOR_CLOSED) && (ilock_status != DoorStatus::DOOR_CLOSED))
	{
		PlayErrorBeep();
		ShowWarning("Warning Decompression Danger.wav", DeltaGliderXR1::ST_WarningCallout, "WARNING: DECOMPRESSION DANGER:&Both airlock doors open!");
		return;
	}

	Undock(0);

	// if ship is docked, set airlock pressure to EXTERNAL PRESSURE if outer door is not closed
	if (olock_status != DoorStatus::DOOR_CLOSED)
	{
		DoorStatus newChamberStatus = (InEarthAtm() ? DoorStatus::DOOR_CLOSED : DoorStatus::DOOR_OPEN);
		ActivateChamber(newChamberStatus, true);  // instantly force pressure or vacuum
	}
}

void DeltaGliderXR1::SetNavlight(bool on)
{
	// set the beacons
	beacon[0].active = beacon[1].active = beacon[2].active = on;

	// set all the spotlights as well
	for (int i = 0; i < SPOTLIGHT_COUNT; i++)
		m_pSpotlights[i]->Activate(on);

	TriggerRedrawArea(AID_NAVLIGHTSWITCH);
	TriggerRedrawArea(AID_SWITCHLED_NAV);
	UpdateCtrlDialog(this);
	RecordEvent("NAVLIGHT", on ? "ON" : "OFF");
}

void DeltaGliderXR1::SetBeacon(bool on)
{
	beacon[3].active = beacon[4].active = on;
	TriggerRedrawArea(AID_BEACONSWITCH);
	TriggerRedrawArea(AID_SWITCHLED_BEACON);  // repaint the new indicator as well
	UpdateCtrlDialog(this);
	RecordEvent("BEACONLIGHT", on ? "ON" : "OFF");
}

void DeltaGliderXR1::SetStrobe(bool on)
{
	beacon[5].active = beacon[6].active = on;
	TriggerRedrawArea(AID_STROBESWITCH);
	TriggerRedrawArea(AID_SWITCHLED_STROBE);  // repaint the new indicator as well
	UpdateCtrlDialog(this);
	RecordEvent("STROBELIGHT", on ? "ON" : "OFF");
}

void DeltaGliderXR1::EnableRetroThrusters(bool state)
{
	for (int i = 0; i < 2; i++)
		SetThrusterResource(th_retro[i], (state ? ph_main : nullptr));

	// set flag denoting retro status so we can beep if necessary
	m_isRetroEnabled = state;
}

void DeltaGliderXR1::EnableHoverEngines(bool state)
{
	for (int i = 0; i < 2; i++)
		SetThrusterResource(th_hover[i], (state ? ph_main : nullptr));

	// set flag denoting hover status so we can beep if necessary
	m_isHoverEnabled = state;
}

void DeltaGliderXR1::EnableScramEngines(bool state)
{
	for (int i = 0; i < 2; i++)
		SetThrusterResource(th_scram[i], (state ? ph_scram : nullptr));

	// set flag denoting hover status so we can beep if necessary
	m_isScramEnabled = state;
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
		if ((m_customAutopilotMode == AUTOPILOT::AP_ATTITUDEHOLD) || (m_customAutopilotMode == AUTOPILOT::AP_DESCENTHOLD))
			retVal *= AP_ATTITUDE_HOLD_RCS_THRUST_MULTIPLIER;
	}

	return retVal;
}

// Create control surfaces for any damageable control surface handles below that are zero (all are zero before vessel initialized).
// This is invoked from clbkSetClassCaps as well as ResetDamageStatus.
void DeltaGliderXR1::ReinitializeDamageableControlSurfaces()
{
	if (hElevator == 0)
	{
		hElevator = CreateControlSurface2(AIRCTRL_ELEVATOR, 1.2, 1.4, _V(0, 0, -7.2), AIRCTRL_AXIS_XPOS, anim_elevator);
	}

	if (hLeftAileron == 0)
	{
		hLeftAileron = CreateControlSurface2(AIRCTRL_AILERON, 0.2, 1.5, _V(7.5, 0, -7.2), AIRCTRL_AXIS_XPOS, anim_raileron);
	}

	if (hRightAileron == 0)
	{
		hRightAileron = CreateControlSurface2(AIRCTRL_AILERON, 0.2, 1.5, _V(-7.5, 0, -7.2), AIRCTRL_AXIS_XNEG, anim_laileron);
	}

	if (hElevatorTrim == 0)
		hElevatorTrim = CreateControlSurface2(AIRCTRL_ELEVATORTRIM, 0.3, 1.5, _V(0, 0, -7.2), AIRCTRL_AXIS_XPOS, anim_elevatortrim);
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
DWORD DeltaGliderXR1::MeshTextureIDToTextureIndex(const int meshTextureID, MESHHANDLE& hMesh)
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

// Reset the MET; invoked when ship is landed
void DeltaGliderXR1::ResetMET()
{
	ShowInfo("Mission Elapsed Time Reset.wav", DeltaGliderXR1::ST_InformationCallout, "Mission Elapsed Time reset; timer&will start at liftoff.");
	m_metMJDStartingTime = -1;     // reset timer
	m_metTimerRunning = false;     // not running now
	RecordEvent("RESETMET", ".");
}

void DeltaGliderXR1::UpdateCtrlDialog(DeltaGliderXR1 *dg, HWND hWnd)
{
	static int bstatus[2] = { BST_UNCHECKED, BST_CHECKED };

	if (!hWnd) hWnd = oapiFindDialog(g_hDLL, IDD_CTRL);
	if (!hWnd) return;

	int op;

	op = static_cast<int>(dg->gear_status) & 1;
	SendDlgItemMessage(hWnd, IDC_GEAR_DOWN, BM_SETCHECK, bstatus[op], 0);
	SendDlgItemMessage(hWnd, IDC_GEAR_UP, BM_SETCHECK, bstatus[1 - op], 0);

	op = static_cast<int>(dg->rcover_status) & 1;
	SendDlgItemMessage(hWnd, IDC_RETRO_OPEN, BM_SETCHECK, bstatus[op], 0);
	SendDlgItemMessage(hWnd, IDC_RETRO_CLOSE, BM_SETCHECK, bstatus[1 - op], 0);

	op = static_cast<int>(dg->nose_status) & 1;
	SendDlgItemMessage(hWnd, IDC_NCONE_OPEN, BM_SETCHECK, bstatus[op], 0);
	SendDlgItemMessage(hWnd, IDC_NCONE_CLOSE, BM_SETCHECK, bstatus[1 - op], 0);

	op = static_cast<int>(dg->olock_status) & 1;
	SendDlgItemMessage(hWnd, IDC_OLOCK_OPEN, BM_SETCHECK, bstatus[op], 0);
	SendDlgItemMessage(hWnd, IDC_OLOCK_CLOSE, BM_SETCHECK, bstatus[1 - op], 0);

	op = static_cast<int>(dg->ilock_status) & 1;
	SendDlgItemMessage(hWnd, IDC_ILOCK_OPEN, BM_SETCHECK, bstatus[op], 0);
	SendDlgItemMessage(hWnd, IDC_ILOCK_CLOSE, BM_SETCHECK, bstatus[1 - op], 0);

	op = static_cast<int>(dg->ladder_status) & 1;
	SendDlgItemMessage(hWnd, IDC_LADDER_EXTEND, BM_SETCHECK, bstatus[op], 0);
	SendDlgItemMessage(hWnd, IDC_LADDER_RETRACT, BM_SETCHECK, bstatus[1 - op], 0);

	op = static_cast<int>(dg->hatch_status) & 1;
	SendDlgItemMessage(hWnd, IDC_HATCH_OPEN, BM_SETCHECK, bstatus[op], 0);
	SendDlgItemMessage(hWnd, IDC_HATCH_CLOSE, BM_SETCHECK, bstatus[1 - op], 0);

	op = static_cast<int>(dg->radiator_status) & 1;
	SendDlgItemMessage(hWnd, IDC_RADIATOR_EXTEND, BM_SETCHECK, bstatus[op], 0);
	SendDlgItemMessage(hWnd, IDC_RADIATOR_RETRACT, BM_SETCHECK, bstatus[1 - op], 0);

	op = static_cast<int>(dg->beacon[0].active) ? 1 : 0;
	SendDlgItemMessage(hWnd, IDC_NAVLIGHT, BM_SETCHECK, bstatus[op], 0);
	op = static_cast<int>(dg->beacon[3].active) ? 1 : 0;
	SendDlgItemMessage(hWnd, IDC_BEACONLIGHT, BM_SETCHECK, bstatus[op], 0);
	op = static_cast<int>(dg->beacon[5].active) ? 1 : 0;
	SendDlgItemMessage(hWnd, IDC_STROBELIGHT, BM_SETCHECK, bstatus[op], 0);
}
