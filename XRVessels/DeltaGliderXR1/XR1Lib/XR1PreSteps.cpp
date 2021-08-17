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
// XR1PreSteps.cpp
// Presteps that do not fall into prestep categories
// ==============================================================

#include "DeltaGliderXR1.h"
#include "XR1PreSteps.h"
#include "AreaIDs.h"
#include "XRPayloadBay.h"

//---------------------------------------------------------------------------

// Update any data values from this frame that we want to preserve for the NEXT frame
// This must be invoked LAST in the PreStep order; also, we cannot access these fields from a *PostStep* because the state has changed across the call
UpdatePreviousFieldsPreStep::UpdatePreviousFieldsPreStep(DeltaGliderXR1 &vessel) :
    XR1PrePostStep(vessel)
{
}

void UpdatePreviousFieldsPreStep::clbkPrePostStep(const double simt, const double simdt, const double mjd) 
{
    GetXR1().m_preStepPreviousGearFullyUncompressedAltitude = GetXR1().GetGearFullyUncompressedAltitude();   // adjust for gear down and/or GroundContact
    GetXR1().m_preStepPreviousAirspeed = GetXR1().GetAirspeed();   // this is used for airspeed callouts during takeoff & landing

    VECTOR3 asVector;
	GetXR1().GetAirspeedVector(FRAME_HORIZON, asVector);
    GetXR1().m_preStepPreviousVerticalSpeed = asVector.y;
}

//---------------------------------------------------------------------------

// Update vessel spotlight levels
UpdateVesselLightsPreStep::UpdateVesselLightsPreStep(DeltaGliderXR1 &vessel) :
    XR1PrePostStep(vessel)
{
}

void UpdateVesselLightsPreStep::clbkPrePostStep(const double simt, const double simdt, const double mjd) 
{
    // Keep the main and hover thruster variable levels in sync with the actual thrust levels;
    // the Orbiter core will automatically do the rest by varying the ligt intensity to match.
    GetXR1().m_mainThrusterLightLevel = GetVessel().GetThrusterGroupLevel(THGROUP_MAIN);
    GetXR1().m_hoverThrusterLightLevel = GetVessel().GetThrusterGroupLevel(THGROUP_HOVER);
}

//-------------------------------------------------------------------------

// Enable/disable nosewheel steering based on APU status.
// This does NOT handle any animation.
// Also fixes poor ground turning performance by "cheating" and rotating the ship based on
// wheel deflection.  Based on code here: http://orbiter-forum.com/showthread.php?t=8392
NosewheelSteeringPreStep::NosewheelSteeringPreStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel)
{
}

void NosewheelSteeringPreStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    if (GetXR1().IsCrashed())  
    {
        GetVessel().SetNosewheelSteering(false);
        return;     // nothing more to do
    }

    bool bSteeringEnabled = false;

    // gear must be operational and DOWN AND LOCKED for steering to be active
    if (GetXR1().gear_status == DoorStatus::DOOR_OPEN)
    {
        // check for ground contact and APU power
        if (GetVessel().GroundContact() && GetXR1().CheckHydraulicPressure(false, false))   // do not play a message or beep here: this is invoked each timestep
        {
            bSteeringEnabled = true;  // steering OK
        }
    }
    
    GetVessel().SetNosewheelSteering(bSteeringEnabled);
    GetXR1().AmplifyNosewheelSteering();  // rotate the ship to fix poor nosewheel steering performance inherent in all Orbiter vessels by default
}

//---------------------------------------------------------------------------

ScramjetSoundPreStep::ScramjetSoundPreStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel)
{
}

void ScramjetSoundPreStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // NOTE: engine sound is tied to thrust *produced*, not thrust *level*
    // The easiest way to check this is the flow rate
    double flowLeft  = GetXR1().ramjet->DMF(0);
    double flowRight = GetXR1().ramjet->DMF(1);
    double maxTotalFlow = GetXR1().GetXR1Config()->GetScramMaxDMF() * 2;
    double totalFlow = flowLeft + flowRight;
    double flowLevel = totalFlow / maxTotalFlow;    // 0...1

    // raise volume level earlier here b/c scram is too quiet at normal flow rates
    // flow should be from 127 (idle) to 255
    // OLD (before XRSound): int volume = 170 + static_cast<int>(flowLevel * 75);
    int volume = 127 + static_cast<int>(flowLevel * 128);
    
    // DEV DEBUGGING ONLY: sprintf(oapiDebugString(), "ScramjetSoundPreStep: flowLevel=%lf, volume=%d", flowLevel, volume);
    if (flowLevel == 0)
    {
        GetXR1().StopSound(GetXR1().ScramJet);  // no thrust
    }
    else  // flow > 0; play sound if not already started and/or set the volume level
    {
        // OK if sound already playing here
        GetXR1().PlaySound(GetXR1().ScramJet, DeltaGliderXR1::ST_Other, volume, true);  // loop forever
    }
}

//---------------------------------------------------------------------------

// Drain payload bay tanks to keep *main* tanks full.  This only affects main and SCRAM fuel tanks.
// This should be invoked FIRST in the PreStep order to ensure that the internal tanks stay full across the timestep.
DrainBayFuelTanksPreStep::DrainBayFuelTanksPreStep(DeltaGliderXR1 &vessel) :
    XR1PrePostStep(vessel)
{
}

void DrainBayFuelTanksPreStep::clbkPrePostStep(const double simt, const double simdt, const double mjd) 
{
    // sanity check, although this prestep should never be added to a vessel that does not have a payload bay
    if (GetXR1().m_pPayloadBay == nullptr)
        return; 

    // we track the amount of fuel flowed from bay -> main so that PreventAutoRefuelPostStep will know whether the fuel change came from us 
    GetXR1().m_MainFuelFlowedFromBayToMainThisTimestep = FlowBayFuel(GetXR1().ph_main, false);
    GetXR1().m_SCRAMFuelFlowedFromBayToMainThisTimestep = FlowBayFuel(GetXR1().ph_scram, GetXR1().m_SCRAMTankHidden);
    // Note: RCS internal tank is always standalone, and LOX is flowed manually separately
}

// flow fuel from the bay to the internal tank if possible
// isTankHidden: if true, empty the internal tank if there is no fuel in the bay; otherwise, flow normally.  false = flow normally
// Returns: amount of bay fuel flowed to main tank (in kg)
double DrainBayFuelTanksPreStep::FlowBayFuel(const PROPELLANT_HANDLE ph, const bool isTankHidden)
{
    // check whether the internal fuel tanks are less than full
    const PROP_TYPE pt = GetXR1().GetPropTypeForHandle(ph);
    double internalTankQty = GetXR1().GetPropellantMass(ph);
    const double maxInternalTankQty = GetXR1().GetPropellantMaxMass(ph);

    // If this tank is hidden, it should be EMPTY unless there is actually FUEL in the bay.
    // This is so that the engines will immediately stop when the bay tank empties or is jettesoned.
    // Granted, when the bay tanks runs out this will cause the last few kg of fuel to vanish
    // before being burned, but that's OK we can consider that last bit of fuel as being 
    // "stuck in the lines due to low fuel pressure" or something, so the engines shut down 
    // and the ship renders the fuel gauge as zero at that point.
    //
    // NOTE: we must ignore this check if refueling or cross-feeding is in progress: if there is no fuel tank in the bay
    // and RequirePayloadBayFuelTanks=0 or 1, the internal tank needs to fill in order for the refueling to stop.
    if ((!GetXR1().IsRefuelingOrCrossfeeding()) && isTankHidden && (GetXR1().m_pPayloadBay->GetPropellantMass(pt) <= 0))  // < 0 for sanity check
    {
        GetXR1().SetPropellantMass(ph, 0);  // no bay fuel: hidden internal tank is empty as well
        return 0;
    }

    const double requestedFlowQty = maxInternalTankQty - internalTankQty;
    _ASSERTE(requestedFlowQty >= 0);  // should never flowing in the other direction here!
    double fuelFlowedToMainTank = 0;
    if (requestedFlowQty > 0)
    {
        // internal tank is < 100% full; let's see if we can flow from the bay to the internal tank to fill it up
        // Note: flowFromBay may be zero here if bay tanks are empty
        const double flowFromBay = -GetXR1().AdjustBayPropellantMassWithMessages(pt, -requestedFlowQty);  // flow is negative, so negate it
        internalTankQty += flowFromBay;  // add to main internal tank
        fuelFlowedToMainTank = flowFromBay;    
        GetXR1().SetPropellantMass(ph, internalTankQty);  // ...and store the new quantity
    }
    return fuelFlowedToMainTank;
}

//---------------------------------------------------------------------------

RefreshSlotStatesPreStep::RefreshSlotStatesPreStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel), m_nextRefreshSimt(0)
{
}

// Rescan for bay slot changes once every second so we can detect and handle when some other vessel removes payload from our payload bay
// (forced detachment).  Otherwise the ship would think that an adjacent payload slot for a multi-slot payload would still be in use even 
// though the Orbiter core force-detached it (e.g., from a payload crane vessel).
void RefreshSlotStatesPreStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    if (simt >= m_nextRefreshSimt)
    {
        // time for a rescan
        GetXR1().m_pPayloadBay->RefreshSlotStates();

        // RefreshSlotStates is relatively expensive, so schedule next scan for one second from now
        m_nextRefreshSimt = simt + 1.0;
    }
}

//---------------------------------------------------------------------------

// Apply the parking brakes if they are set
ParkingBrakePreStep::ParkingBrakePreStep(DeltaGliderXR1 &vessel) :
XR1PrePostStep(vessel)
{
}

void ParkingBrakePreStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
	if (GetXR1().GetXR1Config()->EnableParkingBrakes)    // parking brake functionality enabled?
	{
		//
		// Set or unset the parking brakes
		//
		
		// engage the parking brakes if ship is at wheel-stop AND no thrust is applied AND (if the parking brakes are not already engagged) the APU is online
		// Note: the parking brakes do not require APU power once they are set.
		if (GetXR1().IsLanded() && ((GetXR1().apu_status == DoorStatus::DOOR_OPEN) || GetXR1().m_parkingBrakesEngaged) &&
			!GetXR1().MainThrustApplied() && 
			!GetXR1().HoverThrustApplied() &&
			!GetXR1().RetroThrustApplied() &&
			!GetXR1().ScramThrustApplied() &&
			!GetXR1().RCSThrustApplied())
		{
			// sprintf(oapiDebugString(), "ParkingBrakePreStep: ship is landed and no thrust is applied and APU is online; setting parking brake");
			GetXR1().m_parkingBrakesEngaged = true;
		}
		// Note: because of an Orbiter 2016 core anomaly (or feature?) the ship can lose GroundContact and/or have spurious groundspeed on startup, so we give the ship 2 seconds to settle down first.
		else if (simt >= STARTUP_DELAY_BEFORE_ISLANDED_VALID)
		{
			// sprintf(oapiDebugString(), "ParkingBrakePreStep: ship is NOT landed OR thrust was applied OR APU was offline; UNSETTING parking brake");
			GetXR1().m_parkingBrakesEngaged = false;
		}

		// apply the parking brakes if set: this means the ship has reached (effective) wheel-stop
		if (GetXR1().m_parkingBrakesEngaged)
		{
			// apply brakes for this timestep only
			GetXR1().SetWheelbrakeLevel(1.0, 0, false);	

			///////////////////////////////////////////////////////////////
			// THIS IS A HACK WITHIN-A-HACK TO WORK AROUND AN ORBITER 2016 CORE BUG WHERE THE WHEEL BRAKES CANNOT STOP A VESSEL ON UNEVEN TERRAIN
			// TODO: COMMENT-OUT THIS HACK WHEN IT IS NO LONGER NEEDED
			// cheat and stop the vessel from moving
			VESSELSTATUS2 status;
			GetXR1().GetStatusSafe(status);
			if (status.status != 1)	// not already landed?
			{
				status.status = 1;		// hack #1: force LANDED to stop all motion

				if (oapiGetOrbiterVersion() < 160903)  // hack #2: work around Orbiter core bug with DefStateEx causing uncontrollable spins while landed.  This was fixed in Orbiter version 160903.
				{
					char planetName[256];
					oapiGetObjectName(GetXR1().GetSurfaceRef(), planetName, sizeof(planetName));  // "Earth", "Mars", etc.
					
					char landedStr[256];
					sprintf(landedStr, "Landed %s", planetName);  // "Landed Earth"
					
					const char *pVesselNameInScenario = GetXR1().GetName();	// "XR2-01", etc.

					CString filename;
					filename.Format("%s_temp", pVesselNameInScenario);   // file will be created in $ORBITER_HOME\config

					FILEHANDLE fh = oapiOpenFile(static_cast<const char *>(filename), FILE_OUT, CONFIG);
					oapiWriteScenario_string(fh, "STATUS", landedStr);
					oapiWriteScenario_float(fh, "HEADING", status.surf_hdg * DEG);

					char gearParams[256];   
					sprintf(gearParams, "%d %f", GetXR1().gear_status, GetXR1().gear_proc);   // must write out the landing gear status, too, or the Orbiter core will raise the landing gear on calling scenario load
					oapiWriteScenario_string(fh, "GEAR", gearParams);

					char position[256];
					sprintf(position, "%.20f %.20f", status.surf_lng * DEG, status.surf_lat * DEG);
					oapiWriteScenario_string(fh, "POS", position);
					oapiCloseFile(fh, FILE_OUT);
					fh = oapiOpenFile(static_cast<const char *>(filename), FILE_IN, CONFIG);
					GetXR1().clbkLoadStateEx(fh, &status);
					oapiCloseFile(fh, FILE_IN);
				}
				GetXR1().DefSetStateEx(&status);
			}
			
#if 0
			// section for debugging only
			VECTOR3 rVelToSurface;
			GetXR1().GetGroundspeedVector(FRAME_HORIZON, rVelToSurface);
			sprintf(oapiDebugString(), "ParkingBrakePreStep: rVelToSurface x=%lf, y=%lf, z=%lf, status=%d", rVelToSurface.x, rVelToSurface.y, rVelToSurface.z, status.status);
#endif
			///////////////////////////////////////////////////////////////

		}
		// else no brake override is applied, so normal Orbiter core wheelbrake keys apply for this timestep
	}
}
