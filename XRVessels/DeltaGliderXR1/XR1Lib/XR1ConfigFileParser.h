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
// XR1ConfigFileParser.h
// Parse the XR1 configuration file.
// ==============================================================

#pragma once

#define _CRT_SECURE_NO_DEPRECATE
#include "windows.h"
#include <vector>
#include "VesselConfigFileParser.h"
#include "SecondaryHUDData.h"
#include "XR1Globals.h"

using namespace std;

// convert RGB value to BGR, a Windows' COLORREF
#define CREF3(r,g,b) ((b << 16) | (g << 8) | r)

// default colors
#define DEFAULT_POPUP_HUD_COLOR  CREF3(217,217,217)  /* RGB->CREF format */

// resupply tank indexes
#define TANK_MAIN   0
#define TANK_SCRAM  1
#define TANK_APU    2
#define TANK_LOX    3
#define TANK_LAST   TANK_LOX   /* update if you ever add tanks; it is used for array sizes & iteration */

#define MAX_LOX_LOADOUT_INDEX 9
#define MAX_FILENAME_LEN  80

// These max lengths are based on the Crew Display on the upper panel; they do NOT include the terminator
// NOTE: the font is varied in width, so there is no way to get this width exact
const int CrewMemberNameLength = 25;
const int CrewMemberRankLength = 30;
const int CrewMemberMeshLength = 255; 

// contains data about a single cheatcode
// Note: currently all cheatcodes are doubles
class Cheatcode
{
public:
    Cheatcode(const char *pName, double value, double *ptr1, double *ptr2) :
        m_ptr1(ptr1), m_ptr2(ptr2), m_value(value)
    {
        strcpy(m_name, pName);
    }

    // apply this cheatcode
    void Apply() const
    {
        *m_ptr1 = m_value;
        if (m_ptr2 != nullptr)    // 2nd pointer is optional
            *m_ptr2 = m_value;  
    }

    const char *GetName() const { return m_name; }
    double GetValue() const { return m_value; }

private:
    char m_name[80];  // e.g., "EmptyMass"
    double *m_ptr1;         // variable #1 to be altered; is never null
    double *m_ptr2;         // variable #2 to be altered; may be null
    double m_value;         // value itself
};


// contains data about a single crew member
class CrewMember
{
public:
    CrewMember() { *name = age = pulse = mass = *rank = *mesh = 0; }

    char name[CrewMemberNameLength+1];  // allow room for terminator
    int age;                // limited to 2 digits
    int pulse;              // base pulse
    int mass;               // mass in kg
    char rank[CrewMemberRankLength+1];  // stored as MiscID in UMmu
    char mesh[CrewMemberMeshLength+1];
    CString miscID;         // "XI0", "XI1", etc.
};

#define PARSE_CHEATCODE_DOUBLE(name, variable)     \
        if (PNAME_MATCHES(name))                   \
        {                                          \
            double val = 0;                        \
            SSCANF1("%lf", &val);                  \
            AddCheatcode(name, val, &variable);    \
            processed = true;                      \
        }

// sets both values in an array of 2 values to be the parsed value; e.g., for custom ISP value
// Note: since 'arrayPtr' is a double[] here, it's a pointer.
#define PARSE_CHEATCODE_DOUBLE2(name, arrayPtr)    \
        if (PNAME_MATCHES(name))                   \
        {                                          \
            double val = 0;                        \
            SSCANF1("%lf", &val);                  \
            AddCheatcode(name, val, arrayPtr, arrayPtr+1);  \
            processed = true;                      \
        }

typedef vector<const Cheatcode *>::iterator CheatcodeConstIterator;

// main parser class; this is also a base class for other XR vessels
class XR1ConfigFileParser : public VesselConfigFileParser
{
public:
    XR1ConfigFileParser();
    virtual ~XR1ConfigFileParser();

    // parsed data values
    int MainEngineThrust;
    int HoverEngineThrust;
    int SCRAMfhv;
    int SCRAMdmf;
    bool WingStressDamageEnabled;
    bool HullHeatingDamageEnabled;
    bool HardLandingsDamageEnabled;
    bool DoorStressDamageEnabled;
    bool CrashDamageEnabled;
    bool EnableATMThrustReduction;
    bool EnableManualFlightControlsForAttitudeHold;
    bool InvertAttitudeHoldPitchArrows;
    bool InvertDescentHoldRateArrows;
    
    bool EnableAudioStatusGreeting;
    bool EnableVelocityCallouts;
    bool EnableAltitudeCallouts;
    bool EnableDockingDistanceCallouts;
    bool EnableInformationCallouts;
    bool EnableRCSStatusCallouts;
    bool EnableAFStatusCallouts;
    bool EnableWarningCallouts;

    COLORREF TertiaryHUDNormalColor;
    COLORREF TertiaryHUDWarningColor;
    COLORREF TertiaryHUDBackgroundColor;
    SecondaryHUDMode SecondaryHUD[5];
    double DistanceToBaseOnHUDAltitudeThreshold;
    double MDAUpdateInterval;
    double SecondaryHUDUpdateInterval;
    double TertiaryHUDUpdateInterval;
    double ArtificialHorizonUpdateInterval;
    double PanelUpdateInterval;
    int APUFuelBurnRate;
    int APUIdleRuntimeCallouts; 
    bool APUAutoShutdown;
    bool APUAutostartForCOGShift;
    bool AllowGroundResupply[TANK_LAST+1];    // this is a *size*, not an *index*
    bool AllowDockResupply[TANK_LAST+1];      // this is a *size*, not an *index*
    bool AllowEarthOnlyResupply[TANK_LAST+1]; // this is a *size*, not an *index*
    int LOXLoadout;
    int LOXConsumptionRate;
    int CoolantHeatingRate;
    int MainFuelISP;
    int SCRAMFuelISP;
    char LiftoffCallout[MAX_FILENAME_LEN+1];
    char TouchdownCallout[MAX_FILENAME_LEN+1];
    int ClearedToLandCallout;
    bool EnableSonicBoom;
    bool ScramEngineOverheatDamageEnabled;
    bool EnableDamageWhileDocked;
    bool OrbiterAutoRefuelingEnabled;
    bool RequirePilotForShipControl;
    bool EnableCustomMainEngineSound;
    bool EnableCustomHoverEngineSound;
    bool EnableCustomRCSSound;
    int AudioCalloutVolume;
    int CustomMainEngineSoundVolume;
    bool Lower2DPanelVerticalScrollingEnabled;
    // payload items; not used by the XR1
    double PayloadScreensUpdateInterval;   // interval in seconds

    CrewMember *CrewMembers;  // array MAX_PASSENGERS in length; includes pilot
    int DefaultCrewComplement;
    bool ShowAltitudeAndVerticalSpeedOnHUD;
    bool EnableEngineLightingEffects;
    bool CheatcodesEnabled;
	bool EnableParkingBrakes;

    // this is NOT used by the XR1; it is here for subclasses
    bool EnableResupplyHatchAnimationsWhileDocked;
    double LOXConsumptionMultiplier;
    bool EnableBoilOffExhaustEffect;

    // invoked only from XRVessel::ParseXRConfigFile()
    // Note: this method cannot be const because the vector.begin() method is not const, and the vector is our member variable
    void ApplyCheatcodesIfEnabled();

    //
    // Worker methods
    //

    // LOX tank capacity @ 100% capacity; based on GetLOXConsumptionFraction()
    // (prevent 0 mass when consumption set to 0)
    double GetMaxLoxMass() const { return max(10.0, (m_loxLoadoutArray[LOXLoadout] * GetLOXConsumptionFraction())); }

    // Consumption fraction from 0...1
    double GetLOXConsumptionFraction() const 
    { 
        double retVal;
        if (LOXConsumptionRate == -1)
        {
            // use AUTO table
            retVal = m_autoLoxConsumptionArray[LOXLoadout];
        }
        else
        {
            // normal
            retVal = m_loxConsumptionArray[LOXConsumptionRate]; 
        }

        return retVal;
    }

    // Retrieve ISP for main/hover/rcs engine fuel
    // NOTE: must check for cheatcode here; allow negative cheatcode ISP just to see what happens...
    double GetMainISP() const { return ((m_cheatISP != 0) ? m_cheatISP : m_mainFuelISPArray[MainFuelISP]); }

    // Retrieve ISP multiplier for SCRAM engine fuel
    double GetScramISPMultiplier() const { return m_scramFuelISPArray[SCRAMFuelISP]; }

    // Retrieve SCRAM DMF (max fuel flow)
    // Adjust DMF for SCRAM ISP multiplier; this is to keep the flow value correct.
    double GetScramMaxDMF() const { return m_scramMaxDMF[SCRAMdmf] / GetScramISPMultiplier(); }

    // this is used for engine thrust calculations ONLY!
    double GetScramMaxEffectiveDMF() const { return m_scramMaxDMF[SCRAMdmf]; }

    // get APU fuel burn rate
    double GetAPUFuelBurnRate() const { return m_apuFuelBurnRate[APUFuelBurnRate]; }

    // returns the number of cheatcodes found in the config file(s)
    int GetCheatcodesFoundCount() const { return static_cast<int>(m_cheatcodeVector.size()); }

protected:
    void AddCheatcode(const char *pName, const double value, double *ptr1, double *ptr2 = nullptr);

    virtual bool ParseLine(const char *pSection, const char *pName, const char *pValue, const bool bParsingOverrideFile);
    bool ParseFuelTanks(const char *pValue, bool *pConfigArray);

    // special cheat code values that cannot be set directly in the XR1 object
    double m_cheatISP;    // -1 = NOT SET

    // data tables; defined in XR1Globals.cpp
    static const double m_loxLoadoutArray[];     // LOX mass at REALISTIC consumption rate
    static const double m_loxConsumptionArray[]; // LOX consumption fractions (0...1)
    static const double m_autoLoxConsumptionArray[];    // LOX consumption fractions for AUTO mode; one elemement for each value in m_loxLoadoutArray
    static const double m_mainFuelISPArray[];    // ISP values
    static const double m_scramFuelISPArray[];   // ISP values
    static const double m_scramMaxDMF[];         // max fuel flow for a single SCRAM engine
    static const double m_apuFuelBurnRate[];     // kg/minute

    vector<const Cheatcode *> m_cheatcodeVector; // list of all parsed Cheatcode objects; may be empty
};

