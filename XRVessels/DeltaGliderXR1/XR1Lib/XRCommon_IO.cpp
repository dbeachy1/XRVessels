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
// XRCommon_IO.cpp: XR I/O implementation class that parses scenario file settings common to all XR vessels.
// ==============================================================

#include "DeltaGliderXR1.h"
#include "XR1MultiDisplayArea.h"
#include "XRCommon_IO.h"

// --------------------------------------------------------------
// Parse the supplied line for a recognized XR status lines. 
// Subclasses should inovke this method from their clbkLoadStateEx oapiReadScenario_nextline loop.
//
// Parameters:
//   line: line to be parsed.
//
// Returns: true if line recognized and parsed, false otherwise
// --------------------------------------------------------------
bool DeltaGliderXR1::ParseXRCommonScenarioLine(char *line)
{
    // Note: 'line' is used by our parse macros
    int len;              // used by macros
    bool bFound = false;  // used by macros

    IF_FOUND(NOSECONE_SCN)   // 'NOSECONE' or 'DOCKINGPORT'
    {
        SSCANF2("%d%lf", &nose_status, &nose_proc);
    } 
    else IF_FOUND("APU_STATUS") 
    {
        SSCANF1("%d", &apu_status);  // no proc for this
    } 
    else IF_FOUND("EXTCOOLING_STATUS") 
    {
        SSCANF1("%d", &externalcooling_status);  // no proc for this
    } 
    else IF_FOUND("SECONDARY_HUD") 
    {
        SSCANF1("%d", &m_secondaryHUDMode);
    } 
    else IF_FOUND("ADCTRL_MODE")    // BUGFIX IN DEFAULT DG: preserve ADCTRL mode
    {      
        int adCtrlMode = 7;     // default to ALL ON
        SSCANF1("%d", &adCtrlMode);
        SetADCtrlMode(adCtrlMode);
    } 
    else IF_FOUND("LAST_ACTIVE_SECONDARY_HUD") 
    {
        SSCANF1("%d", &m_lastSecondaryHUDMode);
    } 
    else IF_FOUND("APU_FUEL_QTY") 
    {
        double frac = 1.0;  // default to full if invalid value found
        SSCANF1("%lf", &frac);
        ValidateFraction(frac);     // make sure it's in range
        m_apuFuelQty = frac * APU_FUEL_CAPACITY;
    } 
    else IF_FOUND("LOX_QTY") 
    {
        const double maxLOXQty = GetXR1Config()->GetMaxLoxMass();
        double frac = 1.0;  // default to full if invalid value found
        SSCANF1("%lf", &frac);
        ValidateFraction(frac);     // make sure it's in range
        m_loxQty = frac * GetXR1Config()->GetMaxLoxMass();  // set main tank qty ONLY
    } 
    else IF_FOUND("CABIN_O2_LEVEL") 
    {
        SSCANF1("%lf", &m_cabinO2Level);
        ValidateFraction(m_cabinO2Level);   // check range
    } 
    else IF_FOUND("COOLANT_TEMP") 
    {
        SSCANF1("%lf", &m_coolantTemp);
    } 
    else IF_FOUND("CREW_STATE") 
    {
        SSCANF1("%d", &m_crewState);
    } 
    else IF_FOUND("COGSHIFT_MODES") 
    {
        SSCANF_BOOL3(m_cogShiftAutoModeActive, m_cogShiftCenterModeActive, m_cogForceRecenter);
    } 
    else IF_FOUND("GIMBAL_BUTTON_STATES") 
    {
        SSCANF_BOOL6(m_mainPitchCenteringMode, m_mainYawCenteringMode, m_mainDivMode, m_mainAutoMode, m_hoverCenteringMode, m_scramCenteringMode);
    } 
    else IF_FOUND("INTERNAL_SYSTEMS_FAILURE") 
    {
        SSCANF_BOOL(m_internalSystemsFailure);
    } 
    else IF_FOUND("MWS_ACTIVE") 
    {
        SSCANF_BOOL(m_MWSActive);
    } 
    else IF_FOUND("TAKEOFF_LANDING_CALLOUTS") 
    {
        SSCANF5("%lf %lf %lf %lf %lf", &m_preStepPreviousAirspeed, &m_airborneTargetTime, &m_takeoffTime, &m_touchdownTime, &m_preStepPreviousVerticalSpeed);
    } 
    else IF_FOUND("IS_CRASHED") 
    {
        SSCANF_BOOL(m_isCrashed);
    } 
    else IF_FOUND("CRASH_MSG") 
    {
        SSCANF1("%s", &m_crashMessage);
        DecodeSpaces(m_crashMessage);   // Orbiter won't save or load spaces in params, so we work around it
    } 
    else IF_FOUND("ACTIVE_MDM") 
    {
        SSCANF1("%d", &m_activeMultiDisplayMode);
    } 
    else IF_FOUND("MET_STARTING_MJD") 
    {
        SSCANF1("%lf", &m_metMJDStartingTime);
    } 
    else IF_FOUND("INTERVAL1_ELAPSED_TIME") 
    {
        SSCANF1("%lf", &m_interval1ElapsedTime);
    } 
    else IF_FOUND("INTERVAL2_ELAPSED_TIME") 
    {
        SSCANF1("%lf", &m_interval2ElapsedTime);
    } 
    else IF_FOUND("MET_RUNNING") 
    {
        SSCANF_BOOL(m_metTimerRunning);
    } 
    else IF_FOUND("INTERVAL1_RUNNING") 
    {
        SSCANF_BOOL(m_interval1TimerRunning);
    } 
    else IF_FOUND("INTERVAL2_RUNNING") 
    {
        SSCANF_BOOL(m_interval2TimerRunning);
    } 
    else IF_FOUND("TEMP_SCALE") 
    {
        SSCANF1("%d", &m_activeTempScale);
    } 
    else IF_FOUND("CUSTOM_AUTOPILOT_MODE") 
    {
        AUTOPILOT ap;
        SSCANF1("%d", &ap);
        // must set the autopilot mode via the method so that RCS thrust levels are set correctly
        SetCustomAutopilotMode(ap, false, true);  // do not play sound; FORCE setting regardless of current door status (doors will be set elsewhere during the load)
    } 
    else IF_FOUND("AIRSPEED_HOLD_ENGAGED") 
    {
        SSCANF_BOOL(m_airspeedHoldEngaged);
    } 
    else IF_FOUND("ATTITUDE_HOLD_DATA") 
    {
        // NOTE: m_centerOfLift is a new field for XR1 version 1.3, so it will not be there for pre-existing scenarios.  This would only be a factor
        // if the scenario was saved with the autpilot engaged, but we need to handle this.  The default value in those cases will be NEUTRAL_CENTER_OF_LIFT.
        m_centerOfLift = NEUTRAL_CENTER_OF_LIFT; // this is the value used if no value is present in the scenario
        *(reinterpret_cast<unsigned char *>(&m_holdAOA)) = '0';    // default to FALSE if we read an old scenario file below and m_holdAOA is not parsed
		int i1, i2;
        SSCANF5("%lf %lf %d %d %lf", &m_setPitchOrAOA, &m_setBank, &i1, &i2, &m_centerOfLift);
		m_initialAHBankCompleted = (i1 != 0);  // convert to bool (0 or 1)
		m_holdAOA = (i2 != 0);  // convert to bool (0 or 1)
    } 
    else IF_FOUND("DESCENT_HOLD_DATA") 
    {
		char i1;
        SSCANF3("%lf %lf %c", &m_setDescentRate, &m_latchedAutoTouchdownMinDescentRate, &i1);
		m_autoLand = (i1 != 0);  // convert to bool (0 or 1)
    } 
    else IF_FOUND("AIRSPEED_HOLD_DATA") 
    {
        SSCANF1("%lf", &m_setAirspeed);
    } 
    else IF_FOUND("TERTIARY_HUD_ON") 
    {
        SSCANF_BOOL(m_tertiaryHUDOn);
    } 
    else IF_FOUND("CREW_DISPLAY_INDEX") 
    {
        SSCANF1("%d", &m_crewDisplayIndex);
        // range-check this
        if ((m_crewDisplayIndex < 0) || (m_crewDisplayIndex > MAX_PASSENGERS))  // includes room for pilot @ index 0
            m_crewDisplayIndex = 0;
    } 
    else IF_FOUND("GEAR") 
    {
        SSCANF2("%d%lf", &gear_status, &gear_proc);
    } 
    else IF_FOUND("OVERRIDE_INTERLOCKS") 
    {
        SSCANF_BOOL2(m_crewHatchInterlocksDisabled, m_airlockInterlocksDisabled);
    } 
    else IF_FOUND("RCOVER") 
    {
        SSCANF2("%d%lf", &rcover_status, &rcover_proc);
    } 
    else IF_FOUND("AIRLOCK") 
    {
        SSCANF2("%d%lf", &olock_status, &olock_proc);
    } 
    else IF_FOUND("IAIRLOCK") 
    {
        SSCANF2("%d%lf", &ilock_status, &ilock_proc);
    } 
    else IF_FOUND("CHAMBER") 
    {
        SSCANF2("%d%lf", &chamber_status, &chamber_proc);
    } 
    else IF_FOUND("AIRBRAKE") 
    {
        SSCANF2("%d%lf", &brake_status, &brake_proc);
    } 
    else IF_FOUND("RADIATOR") 
    {
        SSCANF2("%d%lf", &radiator_status, &radiator_proc);
    } 
    else IF_FOUND("LADDER")     // not used by some subclasses, but we can parse it just the same because we have a status and a proc for it in the base XR1 class 
    {
        SSCANF2("%d%lf", &ladder_status, &ladder_proc);
    } 
    else IF_FOUND("SCRAM_DOORS") 
    {
        SSCANF2("%d%lf", &scramdoor_status, &scramdoor_proc);
    } 
    else IF_FOUND("HOVER_DOORS") 
    {
        SSCANF2("%d%lf", &hoverdoor_status, &hoverdoor_proc);
    } 
    else IF_FOUND("HATCH")    // not used by some subclasses, but we can parse it just the same because we have a status and a proc for it in the base XR1 class 
    {
        SSCANF2("%d%lf", &hatch_status, &hatch_proc);
    } 
    else IF_FOUND("SCRAM0DIR") 
    {
        VECTOR3 dir;
        dir.z = -21769.5;   // sanity-check
        SSCANF3("%lf%lf%lf", &dir.x, &dir.y, &dir.z);
        if (dir.z != -21769.5)  // did we read in all three values?
            SetThrusterDir(th_scram[0], dir);
    } 
    else IF_FOUND("SCRAM1DIR") 
    {
        VECTOR3 dir;
        dir.z = -21769.5;   // sanity-check
        SSCANF3("%lf%lf%lf", &dir.x, &dir.y, &dir.z);
        if (dir.z != -21769.5)  // did we read in all three values?
            SetThrusterDir(th_scram[1], dir);
    } 
    else IF_FOUND("HOVER_BALANCE") 
    {
        SSCANF1("%lf", &m_hoverBalance);
    } 
    else IF_FOUND("MAIN0DIR") 
    {
        VECTOR3 dir;
        dir.z = -21769.5;   // sanity-check
        SSCANF3("%lf%lf%lf", &dir.x, &dir.y, &dir.z);
        if (dir.z != -21769.5)  // did we read in all three values?
            SetThrusterDir(th_main[0], dir);
    } 
    else IF_FOUND("MAIN1DIR") 
    {
        VECTOR3 dir;
        dir.z = -21769.5;   // sanity-check
        SSCANF3("%lf%lf%lf", &dir.x, &dir.y, &dir.z);
        if (dir.z != -21769.5)  // did we read in all three values?
            SetThrusterDir(th_main[1], dir);
    } 
    else IF_FOUND("TRIM") 
    {
        double trim;
        SSCANF1("%lf", &trim);
        // Note: cannot use ValidateFraction here, since valid range is -1.0 to +1.0
        // keep in range manually
        if (trim < -1.0)
            trim = -1.0;
        else if (trim > 1.0)
            trim = 1.0;
        SetControlSurfaceLevel (AIRCTRL_ELEVATORTRIM, trim);
    } 
    // NOTE: "SKIN" must be parsed by each subclass because the path, texture names, and texture count may vary between vessels
    else IF_FOUND("LIGHTS") 
    {
        int lgt[3];
        SSCANF3("%d%d%d", lgt+0, lgt+1, lgt+2);
        SetNavlight (lgt[0] != 0);
        SetBeacon (lgt[1] != 0);
        SetStrobe (lgt[2] != 0);
    } 
    else IF_FOUND("DMG_")  // starts with DMG_?
    {   
        int dmgIndex;
        double fracIntegrity;

        SSCANF2("%d %lf", &dmgIndex, &fracIntegrity);
        ValidateFraction(fracIntegrity);  // keep in range
        SetDamageStatus((DamageItem)dmgIndex, fracIntegrity);   // this may be overridden by subclasses
    }
#ifdef MMU
    else IF_FOUND("XR1UMMU_CREW_DATA_VALID") 
    {
        SSCANF_BOOL(m_UMmuCrewDataValid); 
    }
#endif
    else IF_FOUND("PAYLOAD_SCREENS_DATA")   // only applicable to payload-enabled vessels, but doesn't hurt to read it here
    {
        SSCANF4("%lf %d %d %d", &m_deployDeltaV, &m_grappleRangeIndex, &m_selectedSlotLevel, &m_selectedSlot);   // payload screen data
    }
	else IF_FOUND("PAYLOAD_BAY_DOORS")
	{
		SSCANF2("%d%lf", &bay_status, &bay_proc);
	}
    else IF_FOUND("GRAPPLE_TARGET")  // only applicable to payload-enabled vessels, but doesn't hurt to read it here
    {
        // Allocate space for grapple target vessel name; this is only necessary until the pilot selects another target vessel.
        // This memory is freed in the destructor.
        SSCANF1("%s", m_grappleTargetVesselName);
    }
	else IF_FOUND("PARKING_BRAKES")
	{
		SSCANF_BOOL(m_parkingBrakesEngaged);
	}

    //=================================================================
    // BEGIN configuration file overrides
    //=================================================================
    else IF_FOUND_CONFIG_OVERRIDE("MainFuelISP")
    {
        int val;
        SSCANF1("%d", &val);
        Validate(val, 0, MAX_MAINFUEL_ISP_CONFIG_OPTION);  // keep in range
        SET_CONFIG_OVERRIDE_INT(MainFuelISP, val);
    }
    else IF_FOUND_CONFIG_OVERRIDE("SCRAMFuelISP")
    {
        int val;
        SSCANF1("%d", &val);
        Validate(val, 0, 4);  // keep in range
        SET_CONFIG_OVERRIDE_INT(SCRAMFuelISP, val);
    }
    else IF_FOUND_CONFIG_OVERRIDE("LOXConsumptionMultiplier")
    {
        double val;
        SSCANF1("%lf", &val);
        Validate(val, 0.0, 10.0);  // keep in range
        SET_CONFIG_OVERRIDE_DOUBLE(LOXConsumptionMultiplier, val);
    }
    else IF_FOUND_CONFIG_OVERRIDE("APUFuelBurnRate")
    {
        int val;
        SSCANF1("%d", &val);
        Validate(val, 0, 5);  // keep in range
        SET_CONFIG_OVERRIDE_INT(APUFuelBurnRate, val);
    }
    else IF_FOUND_CONFIG_OVERRIDE("CoolantHeatingRate")
    {
        int val;
        SSCANF1("%d", &val);
        Validate(val, 0, 2);  // keep in range
        SET_CONFIG_OVERRIDE_INT(CoolantHeatingRate, val);
    }
    //=================================================================
    // END configuration file overrides
    //=================================================================
#ifdef MMU
    else if (UMmu.LoadAllMembersFromOrbiterScenario(line)) 
    {
        // sprintf(oapiDebugString(), "Loaded UMMu crew member data from scenario file.");  // DEBUG ONLY
		bFound = true;
    } 
#endif
    else 
    {
        // WARNING: if ALL fuel tanks depleted, PRPLEVEL is not present in the scenario file!
        IF_FOUND("PRPLEVEL") 
        {
            ParsePRPLevel(line, len);
            // fall through to Orbiter's default parser (do not set bFound = true)
        }
    }

    return bFound;     // set by macros
}

// --------------------------------------------------------------
// Write common XR status fields (including default vessel paramters in VESSEL2::clbkSaveState) to scenario file
// --------------------------------------------------------------
void DeltaGliderXR1::WriteXRCommonScenarioLines(FILEHANDLE scn)
{
	SaveOrbiterRenderWindowPosition();

    char cbuf[256];

    // Write default vessel parameters
    VESSEL2::clbkSaveState(scn);

    // Write NEW parameters common to all XR vessels
    oapiWriteScenario_int(scn, "SECONDARY_HUD", m_secondaryHUDMode);
    oapiWriteScenario_int(scn, "LAST_ACTIVE_SECONDARY_HUD", m_lastSecondaryHUDMode);
    oapiWriteScenario_int(scn, "ADCTRL_MODE", GetADCtrlMode());     // BUGFIX FOR DEFAULT DG

    sprintf(cbuf, "%lf %lf %lf %lf %lf", m_preStepPreviousAirspeed, m_airborneTargetTime, m_takeoffTime, m_touchdownTime, m_preStepPreviousVerticalSpeed);
    oapiWriteScenario_string(scn, "TAKEOFF_LANDING_CALLOUTS", cbuf);

    oapiWriteScenario_float(scn, "APU_FUEL_QTY", (m_apuFuelQty / APU_FUEL_CAPACITY)); // fraction of fuel remaining

    // need double precision for LOX qty
    sprintf(cbuf, "%lf", (m_loxQty / GetXR1Config()->GetMaxLoxMass()));  // save main tank qty ONLY
    oapiWriteScenario_string(scn, "LOX_QTY", cbuf);   // fraction of LOX remaining

    oapiWriteScenario_float(scn, "CABIN_O2_LEVEL", m_cabinO2Level);   // O2 level in cabin
    oapiWriteScenario_int(scn, "CREW_STATE", static_cast<int>(m_crewState));
    oapiWriteScenario_int(scn, "INTERNAL_SYSTEMS_FAILURE", m_internalSystemsFailure);

    sprintf(cbuf, "%d %d %d", m_cogShiftAutoModeActive, m_cogShiftCenterModeActive, m_cogForceRecenter);
    oapiWriteScenario_string(scn, "COGSHIFT_MODES", cbuf);   

    oapiWriteScenario_int(scn, "MWS_ACTIVE", m_MWSActive);   // there are a few cases where MWS is not automatically restarted (e.g., decompression)
    oapiWriteScenario_float(scn, "COOLANT_TEMP", m_coolantTemp);

    // for damage modeling: loop through each system and write status (0...1)
    // Write each surface so the user can manually disable one if he wants to
    // loop through all surfaces
    for (int i=0; i <= static_cast<int>(D_END); i++)      // Note: D_END is vessel-specific and is defined as a global
    {
        DamageStatus ds = GetDamageStatus((DamageItem)i);
        char name[32];
        char value[96];
        // NOTE: for cosmetic/manual editing reasons, append the FULL label to each name
        sprintf(name, "DMG_%d", i);
        sprintf(value, "%lf %s", ds.fracIntegrity, ds.label);
        oapiWriteScenario_string(scn, name, value);  // "DMG_1 1.000 Left Wing"
    }

    oapiWriteScenario_int(scn, "IS_CRASHED", m_isCrashed);

    if (*m_crashMessage)
    {
        // Orbiter won't save or load spaces in params, so we work around it
        EncodeSpaces(m_crashMessage);
        oapiWriteScenario_string(scn, "CRASH_MSG", m_crashMessage);
        DecodeSpaces(m_crashMessage);
    }

    // need maximum precision here, so format the string ourselves
    sprintf(cbuf, "%lf", m_metMJDStartingTime);
    oapiWriteScenario_string(scn, "MET_STARTING_MJD", cbuf);
    sprintf(cbuf, "%lf", m_interval1ElapsedTime);
    oapiWriteScenario_string(scn, "INTERVAL1_ELAPSED_TIME", cbuf);
    sprintf(cbuf, "%lf", m_interval2ElapsedTime);
    oapiWriteScenario_string(scn, "INTERVAL2_ELAPSED_TIME", cbuf);

    oapiWriteScenario_int(scn, "MET_RUNNING", m_metTimerRunning);
    oapiWriteScenario_int(scn, "INTERVAL1_RUNNING", m_interval1TimerRunning);
    oapiWriteScenario_int(scn, "INTERVAL2_RUNNING", m_interval2TimerRunning);

    oapiWriteScenario_int(scn, "ACTIVE_MDM", m_activeMultiDisplayMode);
    oapiWriteScenario_int(scn, "TEMP_SCALE", static_cast<int>(m_activeTempScale));
    oapiWriteScenario_int(scn, "CUSTOM_AUTOPILOT_MODE", static_cast<int>(m_customAutopilotMode));
    oapiWriteScenario_int(scn, "AIRSPEED_HOLD_ENGAGED", m_airspeedHoldEngaged);

    // scram gimbaling
    VECTOR3 scram0Dir, scram1Dir;
    GetThrusterDir(th_scram[0], scram0Dir);  
    GetThrusterDir(th_scram[1], scram1Dir);
    sprintf(cbuf, "%lf %lf %lf", scram0Dir.x, scram0Dir.y, scram0Dir.z);
    oapiWriteScenario_string(scn, "SCRAM0DIR", cbuf);
    sprintf(cbuf, "%lf %lf %lf", scram1Dir.x, scram1Dir.y, scram1Dir.z);
    oapiWriteScenario_string(scn, "SCRAM1DIR", cbuf);

    // hover balance
    oapiWriteScenario_float(scn, "HOVER_BALANCE", m_hoverBalance);

    // main engine gimbaling
    VECTOR3 main0Dir, main1Dir;
    GetThrusterDir(th_main[0], main0Dir);  
    GetThrusterDir(th_main[1], main1Dir);
    sprintf(cbuf, "%lf %lf %lf", main0Dir.x, main0Dir.y, main0Dir.z);
    oapiWriteScenario_string(scn, "MAIN0DIR", cbuf);
    sprintf(cbuf, "%lf %lf %lf", main1Dir.x, main1Dir.y, main1Dir.z);
    oapiWriteScenario_string(scn, "MAIN1DIR", cbuf);

    sprintf(cbuf, "%d %d %d %d %d %d", m_mainPitchCenteringMode, m_mainYawCenteringMode, m_mainDivMode, m_mainAutoMode, m_hoverCenteringMode, m_scramCenteringMode);
    oapiWriteScenario_string(scn, "GIMBAL_BUTTON_STATES", cbuf);

    // autopilot data
    sprintf(cbuf, "%lf %lf %d %d %lf", m_setPitchOrAOA, m_setBank, m_initialAHBankCompleted, m_holdAOA, m_centerOfLift);
    oapiWriteScenario_string(scn, "ATTITUDE_HOLD_DATA", cbuf);

    sprintf(cbuf, "%lf %lf %d", m_setDescentRate, m_latchedAutoTouchdownMinDescentRate, m_autoLand);
    oapiWriteScenario_string(scn, "DESCENT_HOLD_DATA", cbuf);

    sprintf(cbuf, "%lf", m_setAirspeed);
    oapiWriteScenario_string(scn, "AIRSPEED_HOLD_DATA", cbuf);

    sprintf(cbuf, "%d %d", m_crewHatchInterlocksDisabled, m_airlockInterlocksDisabled);
    oapiWriteScenario_string(scn, "OVERRIDE_INTERLOCKS", cbuf);

    oapiWriteScenario_int(scn, "TERTIARY_HUD_ON", m_tertiaryHUDOn);
    oapiWriteScenario_int(scn, "CREW_DISPLAY_INDEX", m_crewDisplayIndex);
    
    // Write custom parameters
    sprintf (cbuf, "%d %0.4f", gear_status, gear_proc);
    oapiWriteScenario_string (scn, "GEAR", cbuf);

    sprintf (cbuf, "%d %0.4f", rcover_status, rcover_proc);
    oapiWriteScenario_string (scn, "RCOVER", cbuf);

    sprintf (cbuf, "%d %0.4f", nose_status, nose_proc);
    oapiWriteScenario_string (scn, const_cast<char *>(NOSECONE_SCN), cbuf);  // 'NOSECONE' or 'DOCKINGPORT' (work around Orbiter bug with non-const param name)

    sprintf (cbuf, "%d %0.4f", olock_status, olock_proc);
    oapiWriteScenario_string (scn, "AIRLOCK", cbuf);

    sprintf (cbuf, "%d %0.4f", ilock_status, ilock_proc);
    oapiWriteScenario_string (scn, "IAIRLOCK", cbuf);

    sprintf (cbuf, "%d %0.4f", chamber_status, chamber_proc);
    oapiWriteScenario_string(scn, "CHAMBER", cbuf);

    sprintf (cbuf, "%d %0.4f", brake_status, brake_proc);
    oapiWriteScenario_string (scn, "AIRBRAKE", cbuf);

    sprintf (cbuf, "%d %0.4f", radiator_status, radiator_proc);
    oapiWriteScenario_string (scn, "RADIATOR", cbuf);

    sprintf (cbuf, "%d %0.4f", ladder_status, ladder_proc);
    oapiWriteScenario_string (scn, "LADDER", cbuf);

    sprintf (cbuf, "%d %0.4lf", hatch_status, hatch_proc);
    oapiWriteScenario_string (scn, "HATCH", cbuf);

    sprintf (cbuf, "%d %0.4f", scramdoor_status, scramdoor_proc);
    oapiWriteScenario_string (scn, "SCRAM_DOORS", cbuf);

    sprintf (cbuf, "%d %0.4f", hoverdoor_status, hoverdoor_proc);
    oapiWriteScenario_string (scn, "HOVER_DOORS", cbuf);

    oapiWriteScenario_int(scn, "APU_STATUS", static_cast<int>(apu_status));  // no proc for this
    oapiWriteScenario_int(scn, "EXTCOOLING_STATUS", static_cast<int>(externalcooling_status));  // no proc for this

    double trim = GetControlSurfaceLevel (AIRCTRL_ELEVATORTRIM);
    oapiWriteScenario_float (scn, "TRIM", trim);

    // save the custom skin, if any
    if (skinpath[0])
        oapiWriteScenario_string (scn, "SKIN", skinpath);

    // save the beacon status
    sprintf (cbuf, "%d %d %d", beacon[0].active, beacon[3].active, beacon[5].active);
    oapiWriteScenario_string (scn, "LIGHTS", cbuf);

	// save the parking brake status
	oapiWriteScenario_int(scn, "PARKING_BRAKES", m_parkingBrakesEngaged);  


    //=================================================================
    // BEGIN configuration file overrides
    //=================================================================
    // only write out fields that we read in
#define WRITE_CONFIG_OVERRIDE_INT(field)    if (m_configOverrideBitmask & CONFIG_OVERRIDE_##field) oapiWriteScenario_int(scn, "CONFIG_OVERRIDE_"#field, GetXR1Config()->field)
#define WRITE_CONFIG_OVERRIDE_FLOAT(field)  if (m_configOverrideBitmask & CONFIG_OVERRIDE_##field) oapiWriteScenario_float(scn, "CONFIG_OVERRIDE_"#field, GetXR1Config()->field)

    WRITE_CONFIG_OVERRIDE_INT(MainFuelISP);
    WRITE_CONFIG_OVERRIDE_INT(SCRAMFuelISP);
    WRITE_CONFIG_OVERRIDE_FLOAT(LOXConsumptionMultiplier);
    WRITE_CONFIG_OVERRIDE_INT(APUFuelBurnRate);
    WRITE_CONFIG_OVERRIDE_INT(CoolantHeatingRate);
    //=================================================================
    // END configuration file overrides
    //=================================================================

#ifdef MMU
    // UMMu data is valid for this scenario file
    oapiWriteScenario_int(scn, "XR1UMMU_CREW_DATA_VALID", true);   // always write 'true' here!

    // write passenger status via UMmu
    UMmu.SaveAllMembersInOrbiterScenarios(scn);
#endif

    // payload data (only written out if we have a payload bay)
    if (m_pPayloadBay)
    {
        sprintf(cbuf, "%0.1f %d %d %d", m_deployDeltaV, m_grappleRangeIndex, m_selectedSlotLevel, m_selectedSlot);   // payload screen data
        oapiWriteScenario_string (scn, "PAYLOAD_SCREENS_DATA", cbuf);
    
        if (*m_grappleTargetVesselName != 0)   // anything selected?
            oapiWriteScenario_string (scn, "GRAPPLE_TARGET", const_cast<char *>(m_grappleTargetVesselName));  // must cast away constness due to Orbiter API bug

		sprintf(cbuf, "%d %0.4f", bay_status, bay_proc);
		oapiWriteScenario_string(scn, "PAYLOAD_BAY_DOORS", cbuf);
    }
}

// parse the line for PRPLEVEL values and set original tank values
// line = line on which PRPLEVEL occurs in the scenario file
// nameLen = length of PRPLEVEL substring on line
void DeltaGliderXR1::ParsePRPLevel(const char *line, const int nameLen)
{
    // parse the propellant fractions; line format is:
    //    PRPLEVEL 0:0.100 1:0.200 2:0.300
    // Order is main, rcs, scram
    float level[3];
    level[0] = level[1] = level[2] = 0;    // if value is missing, it means that tank is EMPTY

    // find each of the three colons and parse the fraction
    // WARNING: the Orbiter core does not write "0:0", so we must check the tank index *before* the colon as well,
    // and not assume that the tanks are written in order.
    for (const char *p = line + nameLen; *p; p++)
    {
        if (*p == ':')
        {
            int levelIndex = -1;  // "no tank found yet"
            sscanf(p-1, "%d", &levelIndex);  // 0-2
            if ((levelIndex < 0) || (levelIndex > 2))
                continue;   // out-of-range!  Ignore and try to continue.

            sscanf(p+1, "%f", &level[levelIndex]);
            ValidateFraction(static_cast<float>(level[levelIndex]));
        }
    }

    // save our original propellant levels here before Orbiter has a chance to refuel us 
    // if we are sitting on a pad!
    m_startupMainFuelFrac  = level[0];
    m_startupRCSFuelFrac   = level[1];
    m_startupSCRAMFuelFrac = level[2];
}
