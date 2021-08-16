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
// Parse the XR1 configuration file.
// Blank lines and lines beginning with "#" are ignored.
//
// Format is:
//
// [SECTION]
// name=value [,value2,value3,...]
//
// [SECTION-2]
// ...
// ==============================================================

#include "XR1ConfigFileParser.h"
#include "ConfigFileParserMacros.h"
#include "XR1Globals.h"

#include <stdio.h>
#include <string.h>

// Constructor
// sets default values for memeber variables here
XR1ConfigFileParser::XR1ConfigFileParser() :
    VesselConfigFileParser(XR_CONFIG_FILE, XR_LOG_FILE),
    m_cheatISP(0),
    MainEngineThrust(1), HoverEngineThrust(1), SCRAMfhv(1), SCRAMdmf(1),
    WingStressDamageEnabled(true), HullHeatingDamageEnabled(true), 
    HardLandingsDamageEnabled(true), CrashDamageEnabled(true),
    DoorStressDamageEnabled(true), TertiaryHUDNormalColor(DEFAULT_POPUP_HUD_COLOR),
    EnableVelocityCallouts(true), EnableAltitudeCallouts(true), EnableDockingDistanceCallouts(true), 
    EnableInformationCallouts(true), EnableRCSStatusCallouts(true), EnableAFStatusCallouts(true), 
    EnableWarningCallouts(true), EnableAudioStatusGreeting(true), DistanceToBaseOnHUDAltitudeThreshold(200),
    MDAUpdateInterval(0.05), SecondaryHUDUpdateInterval(0.05), TertiaryHUDUpdateInterval(0.05), 
    ArtificialHorizonUpdateInterval(0.05), PanelUpdateInterval(0.0167),
    APUFuelBurnRate(2), APUIdleRuntimeCallouts(20), LOXLoadout(1), LOXConsumptionRate(1),
    CoolantHeatingRate(1), MainFuelISP(2), SCRAMFuelISP(0),
    ClearedToLandCallout(1500), EnableSonicBoom(true), ScramEngineOverheatDamageEnabled(true), EnableDamageWhileDocked(true),
    OrbiterAutoRefuelingEnabled(false), APUAutoShutdown(true), APUAutostartForCOGShift(true),
    EnableManualFlightControlsForAttitudeHold(false), InvertAttitudeHoldPitchArrows(false), InvertDescentHoldRateArrows(false), 
    Lower2DPanelVerticalScrollingEnabled(false),
    DefaultCrewComplement(MAX_PASSENGERS), ShowAltitudeAndVerticalSpeedOnHUD(true), EnableEngineLightingEffects(true),
	CheatcodesEnabled(true), EnableParkingBrakes(true),
    // Values below here are NOT used by the XR1; there are here for subclasses
    EnableResupplyHatchAnimationsWhileDocked(true),
    AudioCalloutVolume(255), PayloadScreensUpdateInterval(0.05),  // 20 times/second
    LOXConsumptionMultiplier(1.0), EnableBoilOffExhaustEffect(true)
{
    // set callout defaults
    strcpy(LiftoffCallout, "Wheels Up.wav");
    strcpy(TouchdownCallout, "Wheels Down.wav");

    //
    // Set default refuel/resupply tank options
    //

    // first, reset all to false
    for (int i=0; i <= TANK_LAST; i++)
    {
        AllowGroundResupply[i] = false;
        AllowDockResupply[i] = false;
        AllowEarthOnlyResupply[i] = false;
    }

    // second, enable default refueling/resupply settings
    AllowGroundResupply[TANK_MAIN] = true;
    AllowGroundResupply[TANK_LOX] = true;
    
    AllowDockResupply[TANK_MAIN] = true;
    AllowDockResupply[TANK_LOX] = true;

    AllowEarthOnlyResupply[TANK_APU] = true;

    // set passenger defaults, which should always be overridden from settings in the config file
    CrewMembers = new CrewMember[MAX_PASSENGERS];
    for (int i=0; i < MAX_PASSENGERS; i++)
    {
        CrewMember *pCrewMember = CrewMembers + i;
        sprintf(pCrewMember->name, "Passenger %d", i);
        pCrewMember->age = 37;
        pCrewMember->pulse = 72;
        pCrewMember->mass = 68;
        strcpy(pCrewMember->rank, "Civilian");
        strcpy(pCrewMember->mesh, DEFAULT_CREW_MESH);
        pCrewMember->miscID.Format("XI%d", i);      // "XI0", "XI1", etc.
    }

    // write the XR1 version to the log to aid in debugging
    char msg[256];
    sprintf(msg, "Loading %s: %s", VESSELNAME, VERSION);
    WriteLog(msg);
}

// Destructor
XR1ConfigFileParser::~XR1ConfigFileParser()
{
    delete[] CrewMembers;

    // clean up our list of cheatcodes (may be empty)
    CheatcodeConstIterator it = m_cheatcodeVector.begin();   // iterates over values
    for (; it != m_cheatcodeVector.end(); it++)
        delete *it;
}

// Parse a line; invoked by our superclass
// returns: true if line OK, false if error
bool XR1ConfigFileParser::ParseLine(const char *pSection, const char *pPropertyName, const char *pValue, const bool bParsingOverrideFile)
{
    int len;        // used by macros
    char temp[MAX_LINE_LENGTH + 128];  // used for string scanning and error handling
    char temp2[MAX_LINE_LENGTH + 128]; // used for string scanning and error handling
    bool processed = false;     // set to 'true' by macros if parameter processed; primarily used by subclasses, so the macros expect this variable to exist

    // parse [SYSTEM] settings
    if (SECTION_MATCHES("SYSTEM"))
    {
        if (PNAME_MATCHES("2DPanelWidth"))
        {
            SSCANF1("%d", &TwoDPanelWidth);
            VALIDATE_INT(reinterpret_cast<int *>(&TwoDPanelWidth), 0, 3, 0);  // OK to cast enum * to int * here
        }
    }
    // parse [PASSENGERx] settings
    else if (SECTION_STARTSWITH("PASSENGER"))
    {
        int passengerNumber = -1;
        sscanf(pSection + 9, "%d", &passengerNumber);   // 0 to (MAX_PASSENGERS-1)
        const int lastValidPassengerIndex = MAX_PASSENGERS - 1; 
        if ((passengerNumber < 0) || (passengerNumber > lastValidPassengerIndex))
        {
            char msg[80];
            sprintf(msg, "Invalid PASSENGER section name: valid values are PASSENGER0 - PASSENGER%d", lastValidPassengerIndex);
            WriteLog(msg);
            return false;
        }

        CrewMember *pCrewMember = CrewMembers + passengerNumber;
        
        // passenger section OK; let's parse the values
        if (PNAME_MATCHES("Name"))
        {
            STRNCPY(pCrewMember->name, CrewMemberNameLength);  // length does NOT include the terminator
        }
        else if (PNAME_MATCHES("Age"))
        {
            SSCANF1("%d", &pCrewMember->age);
            VALIDATE_INT(&pCrewMember->age, 1, 99, 37);
        }
        else if (PNAME_MATCHES("Pulse"))
        {
            SSCANF1("%d", &pCrewMember->pulse);
            VALIDATE_INT(&pCrewMember->pulse, 60, 120, 37);
        }
        else if (PNAME_MATCHES("Mass"))
        {
            SSCANF1("%d", &pCrewMember->mass);
            VALIDATE_INT(&pCrewMember->mass, 10, 181, 68);
        }
        else if (PNAME_MATCHES("Rank"))
        {
            STRNCPY(pCrewMember->rank, CrewMemberRankLength);
        }
        else if (PNAME_MATCHES("Mesh"))
        {
            STRNCPY(pCrewMember->mesh, CrewMemberMeshLength);
        }
        else  // invalid value!
        {
            goto invalid_name;
        }
    }
    // parse [GENERAL] settings
    else if (SECTION_MATCHES("GENERAL"))
    {
        if (PNAME_MATCHES("DefaultCrewComplement"))
        {
            SSCANF1("%d", &DefaultCrewComplement);
            VALIDATE_INT(&DefaultCrewComplement, 0, MAX_PASSENGERS, MAX_PASSENGERS);
        }
        else if (PNAME_MATCHES("EnableEngineLightingEffects"))
        {
			SSCANF_BOOL("%c", &EnableEngineLightingEffects);
        }
        else if (PNAME_MATCHES("EnableParkingBrakes"))
        {
			SSCANF_BOOL("%c", &EnableParkingBrakes);
        }
		else if (PNAME_MATCHES("CheatcodesEnabled"))
		{
			SSCANF_BOOL("%c", &CheatcodesEnabled);
		}
        else if (PNAME_MATCHES("ShowAltitudeAndVerticalSpeedOnHUD"))
        {
			SSCANF_BOOL("%c", &ShowAltitudeAndVerticalSpeedOnHUD);
        }
        else if (PNAME_MATCHES("RequirePilotForShipControl"))
        {
			SSCANF_BOOL("%c", &RequirePilotForShipControl);
        }
        else if (PNAME_MATCHES("MainFuelISP"))
        {
            SSCANF1("%d", &MainFuelISP);
            VALIDATE_INT(&MainFuelISP, 0, MAX_MAINFUEL_ISP_CONFIG_OPTION, 2);
        }
        else if (PNAME_MATCHES("SCRAMFuelISP"))
        {
            SSCANF1("%d", &SCRAMFuelISP);
            VALIDATE_INT(&SCRAMFuelISP, 0, 4, 0);
        }
        else if (PNAME_MATCHES("MainEngineThrust"))
        {
            SSCANF1("%d", &MainEngineThrust);
            VALIDATE_INT(&MainEngineThrust, 0, 1, 1);
        }
        else if (PNAME_MATCHES("HoverEngineThrust"))
        {
            SSCANF1("%d", &HoverEngineThrust);
            VALIDATE_INT(&HoverEngineThrust, 0, 1, 1);
        }
        else if (PNAME_MATCHES("SCRAMfhv"))
        {
            SSCANF1("%d", &SCRAMfhv);
            VALIDATE_INT(&SCRAMfhv, 0, 1, 1);
        }
        else if (PNAME_MATCHES("SCRAMdmf"))
        {
            SSCANF1("%d", &SCRAMdmf);
            VALIDATE_INT(&SCRAMdmf, 0, 1, 1);
        }
        else if (PNAME_MATCHES("LOXLoadout"))
        {
            SSCANF1("%d", &LOXLoadout);
            VALIDATE_INT(&LOXLoadout, 0, MAX_LOX_LOADOUT_INDEX, 1);
        }
        else if (PNAME_MATCHES("LOXConsumptionRate"))
        {
            SSCANF1("%d", &LOXConsumptionRate);
            VALIDATE_INT(&LOXConsumptionRate, -1, 4, -1);
        }
        else if (PNAME_MATCHES("CoolantHeatingRate"))
        {
            SSCANF1("%d", &CoolantHeatingRate);
            VALIDATE_INT(&CoolantHeatingRate, 0, 2, 1);
        }
        else if (PNAME_MATCHES("WingStressDamageEnabled"))
        {
			SSCANF_BOOL("%c", &WingStressDamageEnabled);
        }
        else if (PNAME_MATCHES("HullHeatingDamageEnabled"))
        {
			SSCANF_BOOL("%c", &HullHeatingDamageEnabled);
        }
        else if (PNAME_MATCHES("HardLandingsDamageEnabled"))
        {
			SSCANF_BOOL("%c", &HardLandingsDamageEnabled);
        }
        else if (PNAME_MATCHES("DoorStressDamageEnabled"))
        {
			SSCANF_BOOL("%c", &DoorStressDamageEnabled);
        }
        else if (PNAME_MATCHES("CrashDamageEnabled"))
        {
			SSCANF_BOOL("%c", &CrashDamageEnabled);
        }
        else if (PNAME_MATCHES("ScramEngineOverheatDamageEnabled"))
        {
			SSCANF_BOOL("%c", &ScramEngineOverheatDamageEnabled);
        }
        else if (PNAME_MATCHES("EnableDamageWhileDocked"))
        {
			SSCANF_BOOL("%c", &EnableDamageWhileDocked);
        }
        else if (PNAME_MATCHES("EnableATMThrustReduction"))
        {
			SSCANF_BOOL("%c", &EnableATMThrustReduction);
        }
        else if (PNAME_MATCHES("EnableManualFlightControlsForAttitudeHold"))
        {
			SSCANF_BOOL("%c", &EnableManualFlightControlsForAttitudeHold);
        }
        else if (PNAME_MATCHES("InvertAttitudeHoldPitchArrows"))
        {
			SSCANF_BOOL("%c", &InvertAttitudeHoldPitchArrows);
        }
        else if (PNAME_MATCHES("InvertDescentHoldRateArrows"))
        {
			SSCANF_BOOL("%c", &InvertDescentHoldRateArrows);
        }
        else if (PNAME_MATCHES("EnableAudioStatusGreeting"))
        {
			SSCANF_BOOL("%c", &EnableAudioStatusGreeting);
        }
        else if (PNAME_MATCHES("EnableVelocityCallouts"))
        {
			SSCANF_BOOL("%c", &EnableVelocityCallouts);
        }
        else if (PNAME_MATCHES("EnableAltitudeCallouts"))
        {
			SSCANF_BOOL("%c", &EnableAltitudeCallouts);
        }
        else if (PNAME_MATCHES("EnableDockingDistanceCallouts"))
        {
			SSCANF_BOOL("%c", &EnableDockingDistanceCallouts);
        }
        else if (PNAME_MATCHES("EnableInformationCallouts"))
        {
			SSCANF_BOOL("%c", &EnableInformationCallouts);
        }
        else if (PNAME_MATCHES("EnableRCSStatusCallouts"))
        {
			SSCANF_BOOL("%c", &EnableRCSStatusCallouts);
        }
        else if (PNAME_MATCHES("EnableAudioStatusGreeting"))
        {
			SSCANF_BOOL("%c", &EnableAudioStatusGreeting);
        }
        else if (PNAME_MATCHES("EnableAFStatusCallouts"))
        {
			SSCANF_BOOL("%c", &EnableAFStatusCallouts);
        }
        else if (PNAME_MATCHES("EnableWarningCallouts"))
        {
			SSCANF_BOOL("%c", &EnableWarningCallouts);
        }
        else if (PNAME_MATCHES("OrbiterAutoRefuelingEnabled"))
        {
			SSCANF_BOOL("%c", &OrbiterAutoRefuelingEnabled);
        }
        else if (PNAME_MATCHES("TertiaryHUDNormalColor"))
        {
            int r,g,b = 128;    // fall back to gray if bytes invalid
            SSCANF3("%d,%d,%d", &r, &g, &b);
            TertiaryHUDNormalColor = CREF3(r,g,b); // convert to Windows CREF
        }
        else if (PNAME_MATCHES("TertiaryHUDWarningColor"))
        {
            int r,g,b = 128;    // fall back to gray if bytes invalid
            SSCANF3("%d,%d,%d", &r, &g, &b);
            TertiaryHUDWarningColor = CREF3(r,g,b); // convert to Windows CREF
        }
        else if (PNAME_MATCHES("TertiaryHUDBackgroundColor"))
        {
            int r,g,b = 128;    // fall back to gray if bytes invalid
            SSCANF3("%d,%d,%d", &r, &g, &b);
            TertiaryHUDBackgroundColor = CREF3(r,g,b); // convert to Windows CREF
        }
        else if (PNAME_MATCHES("DistanceToBaseOnHUDAltitudeThreshold"))
        {
            SSCANF1("%lf", &DistanceToBaseOnHUDAltitudeThreshold);
            // no validation on this: all double values are valid
        }
        else if (PNAME_MATCHES("MDAUpdateInterval"))
        {
            SSCANF1("%lf", &MDAUpdateInterval);
            VALIDATE_DOUBLE(&MDAUpdateInterval, 0, 2.0, 0.05);
        }
        else if (PNAME_MATCHES("SecondaryHUDUpdateInterval"))
        {
            SSCANF1("%lf", &SecondaryHUDUpdateInterval);
            VALIDATE_DOUBLE(&SecondaryHUDUpdateInterval, 0, 2.0, 0.05);
        }
        else if (PNAME_MATCHES("TertiaryHUDUpdateInterval"))
        {
            SSCANF1("%lf", &TertiaryHUDUpdateInterval);
            VALIDATE_DOUBLE(&TertiaryHUDUpdateInterval, 0, 2.0, 0.05);
        }
        else if (PNAME_MATCHES("ArtificialHorizonUpdateInterval"))
        {
            SSCANF1("%lf", &ArtificialHorizonUpdateInterval);
            VALIDATE_DOUBLE(&ArtificialHorizonUpdateInterval, 0, 2.0, 0.05);
        }
        else if (PNAME_MATCHES("PanelUpdateInterval"))
        {
            SSCANF1("%lf", &PanelUpdateInterval);
            VALIDATE_DOUBLE(&PanelUpdateInterval, 0, 2.0, 0.0167);
        }
        else if (PNAME_MATCHES("APUFuelBurnRate"))
        {
            SSCANF1("%d", &APUFuelBurnRate);
            VALIDATE_INT(&APUFuelBurnRate, 0, 5, 2);
        }
        else if (PNAME_MATCHES("APUIdleRuntimeCallouts"))
        {
            SSCANF1("%d", &APUIdleRuntimeCallouts);
            if (APUIdleRuntimeCallouts != 0)    // 0 is permitted
            {
                // non-zero value
                VALIDATE_INT(&APUIdleRuntimeCallouts, 5, 600, 20);
            }
        }
        else if (PNAME_MATCHES("APUAutoShutdown"))
        {
			SSCANF_BOOL("%c", &APUAutoShutdown);
        }
        else if (PNAME_MATCHES("APUAutostartForCOGShift"))
        {
			SSCANF_BOOL("%c", &APUAutostartForCOGShift);
        }
        else if (PNAME_MATCHES("AllowGroundResupply"))
        {
            if (ParseFuelTanks(pValue, AllowGroundResupply) == false)
                return false;
        }
        else if (PNAME_MATCHES("AllowDockResupply"))
        {
            if (ParseFuelTanks(pValue, AllowDockResupply) == false)
                return false;
        }
        else if (PNAME_MATCHES("AllowEarthOnlyResupply"))
        {
            if (ParseFuelTanks(pValue, AllowEarthOnlyResupply) == false)
                return false;
        }
        else if (PNAME_MATCHES("LiftoffCallout"))
        {
            if (strcmp(pValue, "NONE") == 0)
                *LiftoffCallout = 0;    // callout disabled
            else
                strncpy(LiftoffCallout, pValue, MAX_FILENAME_LEN);
        }
        else if (PNAME_MATCHES("TouchdownCallout"))
        {
            if (strcmp(pValue, "NONE") == 0)
                *TouchdownCallout = 0;    // callout disabled
            else
                strncpy(TouchdownCallout, pValue, MAX_FILENAME_LEN);
        }
        else if (PNAME_MATCHES("ClearedToLandCallout"))
        {
            SSCANF1("%d", &ClearedToLandCallout);
            VALIDATE_INT(&ClearedToLandCallout, 0, 10000, 1500);
        }
        else if (PNAME_MATCHES("EnableSonicBoom"))
        {
			SSCANF_BOOL("%c", &EnableSonicBoom);
        }
        // Note: parameters below here are NOT used by the XR1; they are here for subclasses
        else if (PNAME_MATCHES("EnableResupplyHatchAnimationsWhileDocked"))
        {
			SSCANF_BOOL("%c", &EnableResupplyHatchAnimationsWhileDocked);
        }
        else if (PNAME_MATCHES("EnableCustomMainEngineSound"))
        {
			SSCANF_BOOL("%c", &EnableCustomMainEngineSound);
        }
        else if (PNAME_MATCHES("EnableCustomHoverEngineSound"))
        {
			SSCANF_BOOL("%c", &EnableCustomHoverEngineSound);
        }
        else if (PNAME_MATCHES("EnableCustomRCSSound"))
        {
			SSCANF_BOOL("%c", &EnableCustomRCSSound);
        }
        else if (PNAME_MATCHES("AudioCalloutVolume"))
        {
            SSCANF1("%d", &AudioCalloutVolume);
            VALIDATE_INT(&AudioCalloutVolume, 0, 255, 255);
        }
        else if (PNAME_MATCHES("CustomMainEngineSoundVolume"))
        {
            SSCANF1("%d", &CustomMainEngineSoundVolume);
            VALIDATE_INT(&CustomMainEngineSoundVolume, 0, 255, 255);
        }
        else if (PNAME_MATCHES("MainFuelISP"))  
        {
            SSCANF1("%d", &MainFuelISP);
            VALIDATE_INT(&MainFuelISP, 0, MAX_MAINFUEL_ISP_CONFIG_OPTION, 2);  // upper limit varies by vessel global
        }
        else if (PNAME_MATCHES("PayloadScreensUpdateInterval"))
        {
            SSCANF1("%lf", &PayloadScreensUpdateInterval);
            VALIDATE_DOUBLE(&PayloadScreensUpdateInterval, 0, 2.0, 0.05);
        }
        else if (PNAME_MATCHES("LOXConsumptionMultiplier"))
        {
            SSCANF1("%lf", &LOXConsumptionMultiplier);
            VALIDATE_DOUBLE(&LOXConsumptionMultiplier, 0.0, 10.0, 1.0);
        }
        else if (PNAME_MATCHES("EnableBoilOffExhaustEffect"))
        {
			SSCANF_BOOL("%c", &EnableBoilOffExhaustEffect);
        }
        else if (PNAME_MATCHES("Lower2DPanelVerticalScrollingEnabled"))
        {
			SSCANF_BOOL("%c", &Lower2DPanelVerticalScrollingEnabled);
        }
        else    // unknown parameter name
        {
            goto invalid_name;
        }
    }
    else if ((strlen(pSection) == 14) && (SECTION_STARTSWITH("SECONDARYHUD-")))
    {
        // validate final character 
        int hudIndex= pSection[13] - '1';
        if ((hudIndex < 0) || (hudIndex > 4))
            goto invalid_section;

        // secondary HUD mode OK
        SecondaryHUDMode &hud = SecondaryHUD[hudIndex];
        
        if (PNAME_MATCHES("TextColor"))
        {
            int r,g,b = 128;    // fall back to gray if bytes invalid
            SSCANF3("%d,%d,%d", &r, &g, &b);
            hud.SetTextColor(CREF3(r,g,b));
        }
        else if (PNAME_MATCHES("BackgroundColor"))
        {
            int r,g,b = 128;    // fall back to gray if bytes invalid
            SSCANF3("%d,%d,%d", &r, &g, &b);
            hud.SetBackgroundColor(CREF3(r,g,b));
        }
        else    // let's check for row values: e.g., "row1L=Alt,imp"
        {
            if ((strlen(pPropertyName) != 5) || (PNAME_STARTSWITH("row") == false))
                goto invalid_name;  

            // get the row number
            // NOTE: do not validate it here; the SecondaryHUDMode class will do it below
            const int rowIndex = pPropertyName[3] - '1';    // 0 to SH_ROW_COUNT

            // validate the L/R side
            int sideIndex;
            char sideChar = pPropertyName[4];
            if (sideChar == 'L')
                sideIndex = 0;
            else if (sideChar = 'R')
                sideIndex = 1;
            else
                goto invalid_name;

            // row and side ID are OK; parse the fieldID and units now
            SSCANF2("%s %s", temp, temp2);  // e.g., "Alt", "imp"
            bool cellOK = hud.SetCell(rowIndex, sideIndex, temp, temp2);
            if (cellOK == false)
                goto invalid_value;
        }
    }
    // parse [CHEATCODES] settings
    else if (SECTION_MATCHES("CHEATCODES"))
    {
        PARSE_CHEATCODE_DOUBLE("EmptyMass",         EMPTY_MASS);
        PARSE_CHEATCODE_DOUBLE("MainTankCapacity",  TANK1_CAPACITY);
        PARSE_CHEATCODE_DOUBLE("ScramTankCapacity", TANK2_CAPACITY);
        PARSE_CHEATCODE_DOUBLE("RCSTankCapacity",   RCS_FUEL_CAPACITY);
        PARSE_CHEATCODE_DOUBLE("APUTankCapacity",   APU_FUEL_CAPACITY);
        PARSE_CHEATCODE_DOUBLE("MainFuelISP",       m_cheatISP); // special case: set our member variable
        PARSE_CHEATCODE_DOUBLE2("MaxMainThrust",    MAX_MAIN_THRUST);
        PARSE_CHEATCODE_DOUBLE2("MaxHoverThrust",   MAX_HOVER_THRUST);
        PARSE_CHEATCODE_DOUBLE("MaxRetroThrust",    MAX_RETRO_THRUST);
        PARSE_CHEATCODE_DOUBLE("MaxRCSThrust",      MAX_RCS_THRUST);
        PARSE_CHEATCODE_DOUBLE2("ScramFHV",         SCRAM_FHV);
        PARSE_CHEATCODE_DOUBLE("MaxWheelbrakeForce",                MAX_WHEELBRAKE_FORCE);
        PARSE_CHEATCODE_DOUBLE("WheelSurfaceFrictionCoeff",         WHEEL_FRICTION_COEFF);
        PARSE_CHEATCODE_DOUBLE("MaxAttitudeHoldNormal",             MAX_ATTITUDE_HOLD_NORMAL);
        PARSE_CHEATCODE_DOUBLE("MaxAttitudeHoldAbsoluteBank",       MAX_ATTITUDE_HOLD_ABSOLUTE_BANK);
        PARSE_CHEATCODE_DOUBLE("MaxAttitudeHoldAbsolutePitchOrAOA", MAX_ATTITUDE_HOLD_ABSOLUTE_PITCH_OR_AOA);

        // Payload items: not used by the XR1
        PARSE_CHEATCODE_DOUBLE("CargoMass", CARGO_MASS);
        PARSE_CHEATCODE_DOUBLE("PayloadGrappleRangeOrbit",  PAYLOAD_GRAPPLE_RANGE_ORBIT);
        PARSE_CHEATCODE_DOUBLE("PayloadGrappleRangeLanded", PAYLOAD_GRAPPLE_RANGE_LANDED);
        PARSE_CHEATCODE_DOUBLE("PayloadGrappleMaxDeltaV",   PAYLOAD_GRAPPLE_MAX_DELTAV);
    }
    else  // invalid section!
        goto invalid_section;

    // success!
    return true;

    // invalid section handler
invalid_section:
    // our base class will provide more details in the line following this one, so we don't need to do it...
    if (*pSection == 0)
        strcpy(temp, "Missing [section] line (e.g., '[GENERAL]')");
    else
        sprintf(temp, "Invalid [section] value: '%s'", pSection);
    WriteLog(temp);
    return false;

    // invalid name handler
invalid_name:
    // our base class will provide more details in the line following this one, so we don't need to do it...
    sprintf(temp, "Invalid property name: '%s' in section [%s]" , pPropertyName, pSection);
    WriteLog(temp);
    return false;

    // invalid value handler
invalid_value:
    // our base class will provide more details in the line following this one, so we don't need to do it...
    sprintf(temp, "Invalid property value: '%s'", pValue);
    WriteLog(temp);
    return false;
}

// Add a cheatcode to the pending cheatcode list; this will not be *applied* until later (and then only if cheatcodes are enabled)
//  ptr2 defaults to nullptr
void XR1ConfigFileParser::AddCheatcode(const char *pName, const double value, double *ptr1, double *ptr2)
{
    Cheatcode *pCheatcode = new Cheatcode(pName, value, ptr1, ptr2);
    m_cheatcodeVector.push_back(pCheatcode);
}

// apply all parsed cheatcodes if they are enabled, or log a warning if cheatcodes are disabled
// Note: this method cannot be const because the vector.begin() method is not const, and the vector is our member variable
void XR1ConfigFileParser::ApplyCheatcodesIfEnabled()
{
    // static buffer for efficiency
    static char msg[200]; 

    if (CheatcodesEnabled)
    {
        CheatcodeConstIterator it = m_cheatcodeVector.begin();   // iterates over values
        for (; it != m_cheatcodeVector.end(); it++)
        {
            const Cheatcode *pCheatcode = *it;
            pCheatcode->Apply();
            sprintf(msg, ">>> CHEATCODE ENABLED: %s = %lf", pCheatcode->GetName(), pCheatcode->GetValue()); 
            WriteLog(msg);
        }
    }
    else  // cheatcodes disabled, so display a warning if any are set
    {
        if (GetCheatcodesFoundCount() > 0)
        {
            sprintf(msg, "*** WARNING: %d CHEATCODE(S) set but ignored: cheatcodes are disabled (check 'CheatcodesEnabled' setting)", static_cast<int>(m_cheatcodeVector.size()));
            WriteLog(msg);
        }
    }
}

// Parse fuel tank values and populate bool[TANK_LAST] array such as AllowGroundResupply
// pValue = property value; i.e., "main,scram"
// Returns: true if values OK, false on error
bool XR1ConfigFileParser::ParseFuelTanks(const char *pValue, bool *pConfigArray)
{
    char temp[256];     // for error messages
    char val[TANK_LAST][6];     // n tank values of 6 characters each, including null terminator

    // scanf does not handle this, so we have to parse the line ourselves
    int valueNumber = 0;    // 0-TANK_LAST
    int valueCharIndex = 0; // 0-5
    bool foundAValue = false;
    for (; *pValue; pValue++)
    {
        char c = *pValue;

        // skip any whitespace
        if ((c <= 32) || (c >= 127))
            continue;

        if (c == ',')
        {
            // terminate existing value string
            val[valueNumber][valueCharIndex] = 0;

            // set to next value
            if (valueNumber == 3)
                break;  // no more values to parse

            valueNumber++;
            valueCharIndex = 0;     // reset to start of next value

            // fall through and skip ',' character
        }
        else  // normal character
        {
            foundAValue = true;         // at least one valid value found
            if (valueCharIndex > 5)     // too many chars in this value?
            {
                val[valueNumber][valueCharIndex] = 0;   // terminate string
                sprintf(temp, "Tank ID value is too long; begins with '%s'; valid values are 'main', 'scram', 'apu', and 'lox'", val[valueNumber]);
                WriteLog(temp);
                return false;           // no joy
            }

            val[valueNumber][valueCharIndex++] = c;
        }
    }

    int argCount = 0;   // assume no values found
    if (foundAValue)
    {
        // terminate final string value
        val[valueNumber][valueCharIndex] = 0;
        argCount = valueNumber + 1;   // 1-4
    }

    // since we found this parameter, if it is empty we want to PREVENT ground refueling, so overwrite any defaults to FALSE
    for (int i=0; i <= TANK_LAST; i++)
        pConfigArray[i] = false;

    for (int i=0; i < argCount; i++)
    {
        const char *pArg = val[i];

        // these are case-insensitive values
        if (_stricmp(pArg, "main") == 0)
            pConfigArray[TANK_MAIN] = true;
        else if (_stricmp(pArg, "scram") == 0)
            pConfigArray[TANK_SCRAM] = true;
        else if (_stricmp(pArg, "apu") == 0)
            pConfigArray[TANK_APU] = true;
        else if (_stricmp(pArg, "lox") == 0)
            pConfigArray[TANK_LOX] = true;
        else    // invalid tank ID
        {
            char temp[256];
            sprintf(temp, "Invalid tank ID: '%s'; valid values are 'main', 'scram', 'apu', and 'lox'", pArg);
            WriteLog(temp); 
            return false;
        }
    }

    return true;
}