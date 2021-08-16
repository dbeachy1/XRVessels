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
// XR1Globals.h
// Contains externs for global variables and constants for the XR1.
// ==============================================================

#pragma once

#include "orbitersdk.h"

class DeltaGliderXR1;

// for use by build version strings
#ifdef _WIN64
#define ARCH_TYPE "64-bit"
#else
#define ARCH_TYPE "32-bit"
#endif

// version globals
extern const char *VESSELNAME;
extern const char *VERSION;
extern const char *XR_LOG_FILE;
extern const char *XR_CONFIG_FILE;

// for docs for each value, refer to XR1Globals.cpp

// data hud text strings
extern const char *DATA_HUD_VALUES[];
    
// [CHEATCODE] globals
extern double EMPTY_MASS;
extern double TANK1_CAPACITY;
extern double TANK2_CAPACITY;
extern double RCS_FUEL_CAPACITY;
extern double APU_FUEL_CAPACITY;
extern double MAX_MAIN_THRUST[2];
extern double MAX_RETRO_THRUST;
extern double MAX_HOVER_THRUST[2];
extern double MAX_RCS_THRUST;
extern double MAX_WHEELBRAKE_FORCE;
extern double WHEEL_FRICTION_COEFF;
extern double WHEEL_LATERAL_COEFF;
extern double SCRAM_FHV[2];
extern double MAX_ATTITUDE_HOLD_NORMAL;
extern double MAX_ATTITUDE_HOLD_ABSOLUTE_BANK;
extern double MAX_ATTITUDE_HOLD_ABSOLUTE_PITCH_OR_AOA;

extern const double HIDDEN_ELEVATOR_TRIM_STATE;
extern const double FULLY_LOADED_MASS; 

extern const double FUEL_DUMP_RATE;
extern const double FUEL_LOAD_RATE;

extern const double RCS_FLOW_FRACTION;
extern const double SCRAM_FLOW_FRACTION;
extern const double APU_FLOW_FRACTION;

extern const double LOX_CONSUMPTION_RATE;

extern const double LOX_DUMP_FRAC;
extern const double LOX_LOAD_FRAC;
extern const double LOX_MIN_DUMP_RATE;
extern const double LOX_MIN_FLOW_RATE;

extern const double WING_AREA;
extern const double WING_ASPECT_RATIO;
extern const double WING_EFFICIENCY_FACTOR;

extern const double GEAR_OPERATING_SPEED;
extern const double NOSE_OPERATING_SPEED;
extern const double AIRLOCK_OPERATING_SPEED;
extern const double CHAMBER_OPERATING_SPEED;
extern const double RADIATOR_OPERATING_SPEED;
extern const double AIRBRAKE_OPERATING_SPEED;
extern const double LADDER_OPERATING_SPEED;
extern const double HATCH_OPERATING_SPEED;
extern const double RCOVER_OPERATING_SPEED;
extern const double HOVERDOOR_OPERATING_SPEED;
extern const double SCRAMDOOR_OPERATING_SPEED;

extern const double MAIN_SUPPLY_PSI_LIMIT;
extern const double SCRAM_SUPPLY_PSI_LIMIT;
extern const double APU_SUPPLY_PSI_LIMIT;
extern const double LOX_SUPPLY_PSI_LIMIT;

extern const double PRESSURE_MOVEMENT_RATE;

extern const double RESUPPLY_RANDOM_LIMIT;

extern const double RESUPPLY_UPPER_LIMIT;
extern const double RESUPPLY_LOWER_LIMIT;

extern const double RESUPPLY_GROUND_PSI_FACTOR;
extern const double RESUPPLY_DOCKED_PSI_FACTOR;

extern const double NOMINAL_COOLANT_TEMP;
extern const double WARN_COOLANT_TEMP;
extern const double CRITICAL_COOLANT_TEMP;
extern const double COOLANT_HEATING_RATE[];
extern const double MAX_COOLANT_TEMP;
extern const double MAX_COOLANT_GAUGE_TEMP;
extern const double MIN_COOLANT_GAUGE_TEMP;

extern const double COOLANT_COOLING_RATE_FRAC;
extern const double COOLANT_COOLING_RATE_MIN;

extern const double GEAR_FULLY_UNCOMPRESSED_DISTANCE;
extern const double GEAR_FULLY_COMPRESSED_DISTANCE;
extern const double GEAR_COMPRESSION_DISTANCE;
extern const double CRASH_WING_BALANCE_MULTIPLIER;

extern const double MACH_REENTRY_WARNING_THRESHOLD;

// ========= Main engine parameters ============

extern const double THROTTLE_MICRO_FRAC;

extern const double MAIN_PGIMBAL_RANGE;
extern const double MAIN_YGIMBAL_RANGE;

extern const double MAIN_PGIMBAL_SPEED;
extern const double MAIN_YGIMBAL_SPEED;

extern const double MAX_HOVER_IMBALANCE;
extern const double HOVER_BALANCE_SPEED;

// ========== scramjet parameters ==============

extern const double SCRAM_INTERNAL_TEMAX;
extern const double SCRAM_COOLING;
extern const double MAX_SCRAM_TEMPERATURE;
extern const double SCRAM_PRESSURE_RECOVERY_MULT;
extern const double SCRAM_DMA_SCALE;

extern const double SCRAM_INTAKE_AREA;
extern const double SCRAM_DEFAULT_DIR;

extern const double SCRAM_GIMBAL_RANGE;

extern const double SCRAM_GIMBAL_SPEED;

extern const double NORMAL_O2_LEVEL;
extern const double CRITICAL_O2_LEVEL_WARNING;

extern const double AMBIENT_O2_REPLENTISHMENT_RATE;
extern const double AMBIENT_O2_CONSUMPTION_RATE;
extern const double CREW_LOC_O2_LEVEL;
extern const double CREW_DEATH_O2_LEVEL;

// maximum crew complement, including pilot
extern const int MAX_PASSENGERS;

// SCRAM gauge limits
extern const double SCRAM_FLOW_GAUGE_MAX;
extern const double SCRAM_TSFC_GAUGE_MAX;

// Main/Hover fuel flow limits
extern const double MAIN_FLOW_GAUGE_MAX;
extern const double HOVER_FLOW_GAUGE_MAX;

// ============ Damage parameters ==============

extern const double WINGLOAD_MAX;
extern const double WINGLOAD_MIN;
extern const double RADIATOR_LIMIT;
extern const double HATCH_OPEN_LIMIT;
extern const double OPEN_NOSECONE_LIMIT;
extern const double GEAR_LIMIT;
extern const double RETRO_DOOR_LIMIT;

extern const double DOOR_DYNAMIC_PRESSURE_WARNING_THRESHOLD;

extern const double DYNP_MAX;

extern const double LANDING_GEAR_MAX_MOMEMTUM;
extern const double FULL_CRASH_THRESHOLD;

extern const double TOUCHDOWN_BANK_LIMIT;
extern const double TOUCHDOWN_MAX_PITCH;
extern const double TOUCHDOWN_MIN_PITCH;

extern const double CREW_IMPACT_DEATH_THRESHOLD;
extern const double CREW_IMPACT_SEVERE_INJURY_THRESHOLD;
extern const double CREW_IMPACT_MODERATE_INJURY_THRESHOLD;
extern const double CREW_IMPACT_MINOR_INJURY_THRESHOLD;

extern const double HULL_HEATING_FACTOR;
extern const double OAT_VALID_STATICP_THRESHOLD;

extern const char *WELCOME_MSG;
extern const char *ALL_SYSTEMS_NOMINAL_MSG;

// =============================================

//
// Globals
//
extern HMODULE g_hDLL;

// Global enum; NOT_SET is only used by GearCalloutsPostStep
enum class DoorStatus { NOT_SET = -2, DOOR_FAILED, DOOR_CLOSED, DOOR_OPEN, DOOR_CLOSING, DOOR_OPENING };

#ifdef MMU
// define default mesh (the UMmu mesh)
#define DEFAULT_CREW_MESH "UMmu\\Ummu"
#else
#define DEFAULT_CREW_MESH ""
#endif

// warning light values
#define WARNING_LIGHT_COUNT  21
enum class WarningLight
{ 
    wlNONE = -1,    // no light
    wlMain, wlHovr, wlScrm, 
    wlRtro, wlLwng, wlRwng,
    wlLail, wlRail, wlGear,
    wlNose, wlRdor, wlHtch,
    wlRad,  wlAirb, wlRcs,
    wlHtmp, wlMfuel, wlRfuel,
    wlLox,  wlDynp, wlCool
};

// custom autopilot modes
// NOTE: airspeed hold may be engaged with ANY other AP mode, so it is not defined here
enum class AUTOPILOT { AP_NOTSET = -1, AP_OFF, AP_ATTITUDEHOLD, AP_DESCENTHOLD };
enum class AUTODESCENT_ADJUST { AD_NONE, AD_LEVEL, AD_ADJUST, AD_AUTOLAND };
enum class AIRSPEEDHOLD_ADJUST { AS_NONE, AS_HOLDCURRENT, AS_RESET, AS_ADJUST };

// autopilot constants
extern const double MAX_DESCENT_HOLD_RATE;
extern const double ADRATE_SMALL;
extern const double ADRATE_MED;
extern const double ADRATE_LARGE;

extern const double ASRATE_TINY;
extern const double ASRATE_SMALL;
extern const double ASRATE_MED;
extern const double ASRATE_LARGE;

// volume constants
extern const int QUIET_CLICK;
extern const int MED_CLICK;
extern const int MFD_CLICK;
extern const int AUTOPILOT_VOL;
extern const int WARNING_BEEP_VOL;
extern const int GEAR_WHINE_VOL;
extern const int DOOR_WHINE_VOL;
extern const int ERROR1_VOL;
extern const int APU_VOL;
extern const int FUEL_XFEED_VOL;
extern const int FUEL_DUMP_BASE_VOL;
extern const int FUEL_DUMP_INC_VOL;
extern const int FUEL_RESUPPLY_BASE_VOL;
extern const int FUEL_RESUPPLY_INC_VOL;
extern const int SUPPLY_HATCH_VOL;
extern const int RESUPPLY_LINE_EXTEND_VOL;
extern const int AIRLOCK_CHAMBER_VOLUME;

extern const double NEUTRAL_CENTER_OF_LIFT;
extern const double COL_MAX_SHIFT_RATE;
extern const double COL_MAX_SHIFT_DISTANCE;
extern const double COL_SHIFT_GAUGE_LIMIT;
extern const double COL_KEY_SHIFT_RATE_FRACTION;
extern const double AP_ELEVATOR_TRIM_COL_DEAD_ZONE;
extern const double ELEVATOR_TRIM_SPEED;
extern const double AP_ELEVATOR_TRIM_SPEED;
extern const double AP_ANGULAR_VELOCITY_DEGREES_DELTA_FRAC;

extern const char *SCRAMJET_WAV;
extern const char *WELCOME_ABOARD_ALL_SYSTEMS_NOMINAL_WAV;
extern const char *ALL_SYSTEMS_NOMINAL_WAV;
extern const char *WARNING_OUTER_DOOR_IS_LOCKED_WAV;
extern const char *WARNING_NOSECONE_IS_CLOSED_WAV;
extern const char *WARNING_NOSECONE_OPEN_WAV;

extern const char *NOSECONE_LABEL;
extern const char *NOSECONE_SHORT_LABEL;
extern const char *NOSECONE_SCN;

extern const int MAX_MAINFUEL_ISP_CONFIG_OPTION;

//
// Autopilot constants
//

extern const double AP_PITCH_DELTA_SMALL;
extern const double AP_PITCH_DELTA_LARGE;
extern const double AP_BANK_DELTA;

extern const double AP_COL_DEAD_ZONE;
extern const double AP_COL_THRUSTLEVEL_TO_SHIFTSTEP_RATIO;
extern const double AP_ATTITUDE_HOLD_RCS_THRUST_MULTIPLIER;

// other constants
#define MAX_MESSAGE_LENGTH  512       // max # of characters on the tertiary HUD (~50 chars * 7 lines = 350 + safety margin)
#define INFO_WARNING_BUFFER_LINES 64  // # of lines to preserve in info/warning buffer
#define STARTUP_DELAY_BEFORE_ISLANDED_VALID 4.0   // # of seconds to wait before the ship settles down enough for IsLanded() checks to be valid on startup

// sound aliases, denoted by leading underscore
#define _DoorOpening    BeepHigh
#define _DoorClosing    BeepLow
#define _KillThrust     BeepLow
#define _MDMButtonUp    BeepHigh
#define _MDMButtonDown  BeepLow

// unique panel IDs 
#define PANEL_MAIN  0       // Orbiter 2D panel ID
#define PANEL_UPPER 1       // Orbiter 2D panel ID
#define PANEL_LOWER 2       // Orbiter 2D panel ID

extern const int VC_PANEL_ID_BASE;  // ID of first VC panel
#define PANELVC_PILOT    (VC_PANEL_ID_BASE + 0)   // this is not sent to Orbiter; it is VC #0
#define PANELVC_PSNGR1   (VC_PANEL_ID_BASE + 1)   
#define PANELVC_PSNGR2   (VC_PANEL_ID_BASE + 2)   
#define PANELVC_PSNGR3   (VC_PANEL_ID_BASE + 3)   
#define PANELVC_PSNGR4   (VC_PANEL_ID_BASE + 4)   

// convert a unique VC panel ID into an Orbiter VC number (0-n)
#define ORBITER_VC_NUMBER(panelID) (panelID - VC_PANEL_ID_BASE)

// global enums
enum class AccScale  { EightG, FourG, TwoG, NONE };
enum class TempScale { Kelvin, Fahrenheit, Celsius };

// Note: DamageItem has subclass-usable values defined beginning with "DISubclass"
// NOTE: these enum values must match the XRDamageItem enum in XRVesselCtrl.h (including vessel subclasses)
enum class DamageItem { LeftWing = 0, RightWing, LeftAileron, RightAileron, LandingGear,
                  Nosecone, RetroDoors, Hatch, Radiator, Airbrake, MainEngineLeft, MainEngineRight,
                  SCRAMEngineLeft, SCRAMEngineRight, HoverEngineFore, HoverEngineAft,
                  RetroEngineLeft, RetroEngineRight, RCS1, RCS2, RCS3, RCS4, 
                  RCS5, RCS6, RCS7, RCS8, RCS9, RCS10, RCS11, RCS12, RCS13,
                  RCS14, DISubclass1, DISubclass2, DISubclass3, DISubclass4, DISubclass5, 
                  DISubclass6, DISubclass7, DISubclass8, DISubclass9, DISubclass10 };
extern const DamageItem D_END;   // points to the LAST VALID damage enum for this vessel

enum class CrewState { OK, INCAPACITATED, DEAD };

// NOTE: do not change the order of these values
enum class XFEED_MODE { XF_NOT_SET = -1, XF_MAIN, XF_OFF, XF_RCS };

// Damage status structure; constains status about a single surface
struct DamageStatus
{
    double fracIntegrity;   // 0-1
    char label[64];         // cosmetic label ("Left Wing", etc.)
    char shortLabel[5];     // abbreviated label ("LWng")
    bool onlineOffline;     // if true, status is "ONLINE/OFFLINE" vs. "100%, 0%"
};

// hull temperature limits in degrees K
#define CTOK(c) (c + 273)
#define KTOC(k) (k - 273)
struct HullTemperatureLimits
{
    int noseCone;
    int wings;
    int cockpit;
    int topHull;
    // these two are fractional values denoting a fraction of the limit temp
    double warningFrac;   // yellow text
    double criticalFrac;  // red text
    double doorOpenWarning; // temperature warning issued @ this level (earlier!) if door open
    int doorOpen;           // heat limit if door is open on that surface
};


// Some mesh groups referenced in the code
#define MESHGRP_VC_HUDMODE          0
#define MESHGRP_VC_HBALANCECNT     18
#define MESHGRP_VC_SCRAMGIMBALCNT  19
#define MESHGRP_VC_PGIMBALCNT      20
#define MESHGRP_VC_YGIMBALCNT      21
#define MESHGRP_VC_YGIMBALDIV      22   // NEW
#define MESHGRP_VC_YGIMBALAUTO     23   // NEW
#define MESHGRP_VC_NAVMODE         59
#define MESHGRP_VC_LMFDDISP       109
#define MESHGRP_VC_RMFDDISP       110
#define MESHGRP_VC_STATUSIND      118
#define MESHGRP_VC_HORIZON        120
#define MESHGRP_VC_HUDDISP        136

// ==============================================================
// Global callback prototypes 

INT_PTR CALLBACK XR1Ctrl_DlgProc(HWND, UINT, WPARAM, LPARAM);

// callout globals
extern const double V1_CALLOUT_AIRSPEED;
extern const double ROTATE_CALLOUT_AIRSPEED_EMPTY;
extern const double ROTATE_CALLOUT_AIRSPEED_HEAVY;
extern const double MAX_RECOMMENDED_PAYLOAD_MASS;

// turbopacks available
struct Turbopack
{
    char DisplayName[28];
    char Classname[64];
};

extern const Turbopack TURBOPACKS_ARRAY[];
extern const int TURBOPACKS_ARRAY_SIZE;
extern const VECTOR3 &TURBOPACK_SPAWN_COORDINATES;
extern const double STOW_TURBOPACK_DISTANCE;

// resource ID globals used by common areas
// TODO: some of the XR1 base class areas still use constants from resource.h;
// these currently work with subclasses only because the resource.h happens to match, since
// the subclasses copied the XR1's resource.h as a base.  The real fix, however, is to abstract
// out each shared resource ID here as a global.
extern const int RES_IDB_FUEL_GAUGE;
extern const int RES_IDB_FUEL_GAUGE_DARK;
extern const int RES_IDB_LOX_GAUGE;
extern const int RES_IDB_LOX_GAUGE_DARK;
extern const int RES_IDB_COOLANT_GAUGE;

///////////////////////////////////////////////////////////////////////////

// payload bay globals; these are not used by the XR1
extern const VECTOR3 &PAYLOAD_SLOT_DIMENSIONS;
extern const char *DEFAULT_PAYLOAD_THUMBNAIL_PATH;
extern const double PAYLOAD_BAY_DELTAY_TO_GROUND;
extern const double PAYLOAD_BAY_DELTAX_TO_GROUND;
extern const double GRAPPLE_DISPLAY_RANGES[];
extern const int GRAPPLE_DISPLAY_RANGE_COUNT;
extern const int DEFAULT_GRAPPLE_RANGE_INDEX;
extern const double PAYLOAD_BAY_SLOT_COUNT;
extern const int GLOBAL_IDD_PAYLOAD_EDITOR;  // from resource.h

// payload bay [CHEATCODE] globals; not used by the XR1
extern double CARGO_MASS;
extern double PAYLOAD_GRAPPLE_RANGE_ORBIT;
extern double PAYLOAD_GRAPPLE_RANGE_LANDED;
extern double PAYLOAD_GRAPPLE_MAX_DELTAV;

// wheel rotation globals; not used by the XR1
extern const double FRONT_TIRE_CIRCUMFERENCE;
extern const double REAR_TIRE_CIRCUMFERENCE;
extern const double TIRE_DECELERATION_RATE;

// gear compression globals; not used by the XR1
extern const double GEAR_COMPRESSION_DISTANCE;
extern const double NOSE_GEAR_ZCOORD;
extern const double REAR_GEAR_ZCOORD;
extern const double GEAR_UNCOMPRESSED_YCOORD;
extern const double FRONT_GEAR_COMPRESSION_TRANSLATION_FACTOR;
extern const double REAR_GEAR_COMPRESSION_TRANSLATION_FACTOR;

// fuel/LOX dump particle stream coordinates; not used by the XR1
// If you implement fuel/LOX dump streams, remember to initialize
// XR1.m_pFuelDumpParticleStreamSpec.
extern const VECTOR3 &FUEL_DUMP_PARTICLE_STREAM_POS1;
extern const VECTOR3 &FUEL_DUMP_PARTICLE_STREAM_DIR1;
extern const VECTOR3 &FUEL_DUMP_PARTICLE_STREAM_POS2;
extern const VECTOR3 &FUEL_DUMP_PARTICLE_STREAM_DIR2;

// boil-off exhaust particle stream coordinates; not used by the XR1
// If you implement boil-off streams, remember to initialize
// XR1.m_pBoilOffExhaustParticleStreamSpec.
extern const VECTOR3 &BOIL_OFF_PARTICLE_STREAM_POS1;
extern const VECTOR3 &BOIL_OFF_PARTICLE_STREAM_DIR1;
extern const VECTOR3 &BOIL_OFF_PARTICLE_STREAM_POS2;
extern const VECTOR3 &BOIL_OFF_PARTICLE_STREAM_DIR2;

// number of spotlights defined
extern const int SPOTLIGHT_COUNT;

extern const double HEIGHT_WHEN_LANDED;  
extern const double HULL_LENGTH;
extern const double HULL_WIDTH;

// explicit hull touchdown points (mirror -X for the other opposite sides as well)
extern const VECTOR3 HULL_TOUCHDOWN_POINTS[];
extern const int HULL_TOUCHDOWN_POINTS_COUNT;
