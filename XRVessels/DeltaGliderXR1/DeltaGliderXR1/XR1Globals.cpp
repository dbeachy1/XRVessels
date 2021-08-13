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
// Contains global variable values for the XR1.
// ==============================================================

#include "XR1Globals.h"  // verify that types agree
#include "XR1ConfigFileParser.h"
#include "XR1PayloadDialog.h"

//
// Version globals
//
const char *VESSELNAME = "DeltagliderXR1";

// VERSION ID
// {XXX} UPDATE THIS FOR THE CURRENT BUILD VERSION; DO NOT REMOVE THIS {XXX} COMMENT
const char *VERSION = "Version 2.0 RC1 [" ARCH_TYPE  "], Build Date : " __DATE__;

// file is always written to the Orbiter directory
const char *XR_LOG_FILE = "DeltaGliderXR1.log";

const char *XR_CONFIG_FILE = "Config\\DeltaGliderXR1Prefs.cfg";

// data hud text strings
const char *DATA_HUD_VALUES[] =
{
    "F1",           "Exterior View",
    "F8",           "Virtual Cockpit View",
    "{0-9}",        "Select MDA Mode Directly",
    "D",            "Next MDA Mode",
    "ALT-D",        "Previous MDA Mode",
    "H",            "Select HUD Mode",
    "G",            "Landing Gear",
    "SPACE",        "Disengage Autopilot",
    "ALT-SPACE (hold)", "Show Data HUD (this HUD)",

    "ALT-H",        "Change HUD Color",
    "ALT-Z",        "Dec HUD Brightness",
    "ALT-X",        "Inc HUD Brightness",
    "CTRL-\\",      "Retro Doors",
    "CTRL-G",       "SCRAM Doors",
    "CTRL-V",       "Hover Doors",
    "ALT-,",        "Shift Center of Gravity Aft",
    "ALT-.",        "Shift Center of Gravity Forward",
    "ALT-M",        "Recenter Center of Gravity",
    "CTRL-,",       "Inc Elevator Trim",
    "CTRL-.",       "Dec Elevator Trim",
    "INS",          "Inc Elevator Trim",
    "DEL",          "Dec Elevator Trim",
    "NUM0",         "Inc Hover Thrust",
    "NUM.",         "Dec Hover Thrust",
    "CTRL-=",       "Inc SCRAM Thrust",
    "CTRL--",       "Dec SCRAM Thrust",
    "CTRL-BACKSPACE","Kill SCRAM Thrust",
    "ALT-=",        "Small Inc SCRAM Thrust",
    "ALT--",        "Small Dec SCRAM Thrust",
    "SHIFT-NUM0",   "Small Inc Hover Thrust",
    "SHIFT-NUM.",   "Small Dec Hover Thrust",

    "/",            "RCS Translation/Rotation",
    "ALT-/",        "AF CTRL On/Off",
    "CTRL-/",       "RCS On/Off",
    "CTRL-SPACE",   "Open Control Window",
    "CTRL-A",       "Auxiliary Power Unit (APU)",
    "CTRL-B",       "Airbrake",
    "ALT-R",        "Radiator",
    "CTRL-K",       "Nosecone",
    "CTRL-O",       "Outer Airlock",
    "ALT-O",        "Inner Airlock",
    "CTRL-Y",       "Top Hatch",
    "CTRL-H",       "Main HUD On/Off",
    "CTRL-NUM*",    "Kill Hover Thrust",
    "ALT-T",        "Secondary HUD On/Off",
    "CTRL-{1-5}",   "Secondary HUD Mode",
    "CTRL-T",       "Tertiary HUD On/Off",
    "CTRL-W",       "Silence MWS Alarm",
    "CTRL-NUM+",    "Inc Main Thrust",
    "CTRL-NUM-",    "Dec Main Thrust",
    "NUM*",         "Kill Main Thrust",

    "L",            "Attitude Hold Autopilot",
    "CTRL-L",       "Engage Attitude Hold and Sync",
    "A",            "Descent Hold Autopilot",
    "ALT-S",        "Airspeed Hold Autopilot",

    "[",            "Prograde Autopilot",
    "]",            "Retrograde Autopilot",
    ";",            "Orbit-Normal Autopilot",
    "'",            "Orbit-AntiNormal Autopilot",

    "ALT-;",        "Gimbal All Up (nose up)",
    "ALT-P",        "Gimbal All Down (nose down)",
    "ALT-L",        "Gimbal Main Right (nose left)",
    "ALT-'",        "Gimbal Main Left (nose right)",
    "ALT-0",        "Gimbal Recenter All",

    "CTRL-NUMPAD3", "Attitude Hold: Reset Bank",
    "CTRL-NUMPAD7", "Attitude Hold: Reset Pitch/AOA",
    "CTRL-NUMPAD1", "Attitude Hold: Reset Both (level)",
    "NUMPAD9",      "Attitude Hold: Toggle AOA/Pitch Hold",
    "NUMPAD2",      "Attitude Hold: Inc Pitch/AOA 2.5°",
    "NUMPAD8",      "Attitude Hold: Dec Pitch/AOA 2.5°",
    "ALT-NUMPAD2",  "Attitude Hold: Inc Pitch/AOA 0.5°",
    "ALT-NUMPAD8",  "Attitude Hold: Dec Pitch/AOA 0.5°",
    "NUMPAD4",      "Attitude Hold: Bank Left 5°",
    "NUMPAD6",      "Attitude Hold: Bank Right 5°",

    "CTRL-NUMPAD8",  "Descent Hold: Increase Rate 2.5 m/s",
    "CTRL-NUMPAD2",  "Descent Hold: Decrease Rate 2.5 m/s",
    "NUMPAD8",       "Descent Hold: Increase Rate 0.5 m/s",
    "NUMPAD2",       "Descent Hold: Decrease Rate 0.5 m/s",
    "ALT-NUMPAD8",   "Descent Hold: Increase Rate 0.1 m/s",
    "ALT-NUMPAD2",   "Descent Hold: Decrease Rate 0.1 m/s",
    "NUMPAD-0",      "Descent Hold: Toggle Auto-Land mode",
    "NUMPAD-.",      "Descent Hold: Hold Altitude (hover)",

    "CTRL-NUMPAD+",  "Airspeed Hold: Increase Rate 25 m/s",
    "CTRL-NUMPAD-",  "Airspeed Hold: Decrease Rate 25 m/s",
    "NUMPAD+",       "Airspeed Hold: Increase Rate 5 m/s",
    "NUMPAD-",       "Airspeed Hold: Decrease Rate 5 m/s",
    "SHIFT-NUMPAD+", "Airspeed Hold: Increase Rate 1 m/s",
    "SHIFT-NUMPAD-", "Airspeed Hold: Decrease Rate 1 m/s",
    "ALT-NUMPAD+",   "Airspeed Hold: Increase Rate 0.1 m/s",
    "ALT-NUMPAD-",   "Airspeed Hold: Decrease Rate 0.1 m/s",
    "NUMPAD_ENTER",  "Airspeed Hold: Hold Current Airspeed",
    "NUMPAD*",       "Airspeed Hold: Reset Rate to 0 m/s",

    NULL, NULL   // null-terminate the array
};

// ==============================================================
// Some vessel class caps
// Where an entry consists of two values, the first refers to the
// "easy", the second to the "complex" flight model.
// ==============================================================

// ==============================================================
// CHEATCODE globals
// ==============================================================

// NOT USED FOR XR1: const double EMPTY_MASS    = 11000.0;  // standard configuration
// ORG: const double EMPTY_MASS_SC = 13000.0;  // XR1Ramjet configuration
double EMPTY_MASS = 12000.0;  // XR1Ramjet configuration (NEW)
// DG mass w/o fuel

double TANK1_CAPACITY = 10400.0;
double TANK2_CAPACITY =  2500.0;
// Main fuel tank capacities [kg] (can be split between rocket
// fuel and scramjet fuel)

double RCS_FUEL_CAPACITY = 600.0;
// Max fuel capacity: RCS tank [kg]

double APU_FUEL_CAPACITY = 200.0;
// Max APU fuel capacity [kg]

// DG3: thrust w/turbopump = 1.6e5
// XR1: thrust increased 20% for LOX loadout @ 5 years @ 25% consumption
double MAX_MAIN_THRUST[2] = {2.4e5, 1.92e5};
// Main engine max vacuum thrust [N] per engine. (x2 for total)
// NOTE: don't need to track main engine damage here since thrust is not set by gimbaling

// STOCK: 3.4e4
double MAX_RETRO_THRUST = 4.08e4;
// Retro engine max vacuum thrust [N] per engine. (x2 for total)

// DG3: hover thrust = 1.35e5
// ORG: const double MAX_HOVER_THRUST[2] = {1.4e5, 1.1e5};
// XR1: thrust increased 20% for LOX loadout @ 5 years @ 25% consumption
// XR1: ...PLUS an extra 8.1% of that for the hovers for the simple flight model to allow vertical takeoff on Earth @ full LOX load on AUTO setting
double MAX_HOVER_THRUST[2] = {1.81608e5, 1.32e5};
// Hover engine max vacuum thrust [N] (x2 for total)

// DG3: 2.375
double MAX_RCS_THRUST = 2.5e3;

// DG3 level: 70000 : XR1 100000 : stock 200000
double MAX_WHEELBRAKE_FORCE = 1.0e5;

// {DEB} Hydrogen FHV = 1.42e8 J/kg.
// const double SCRAM_FHV[2] = { 2.13e8, 1.42e8 };  // 50% higher for simple model
// This is the value to set if you want to change the SCRAM engines' ISP.
double SCRAM_FHV[2] = {3.5e8, 2.0e8};
// scramjet fuel heating value [J/kg]: Amount of heat energy
// obtained from burning 1kg of propellant
// NOTE: SCRAM engine integrity is already tracked separately

// this is how much friction the wheels have rolling on the ground
// ORG: 0.05
// BEFORE ORBITER 2016: double WHEEL_FRICTION_COEFF = 0.025;
double WHEEL_FRICTION_COEFF = 0.10;   // DG has 0.1 front, 0.2 rear
double WHEEL_LATERAL_COEFF = 1.6;     // DG has 1.6 front, 3.0 rear

// ATTITUDE HOLD autopilot limits
// max pitch/AOA or bank that can be held when the other axis is non-zero
// NOTE: this must be evenly divisible by 5!
double MAX_ATTITUDE_HOLD_NORMAL = 60.0;

// max bank that can be held at zero pitch or AOA
double MAX_ATTITUDE_HOLD_ABSOLUTE_BANK = 75.0;

// max pitch or AOA that can be held at zero pitch or AOA
double MAX_ATTITUDE_HOLD_ABSOLUTE_PITCH_OR_AOA = 87.5;

// Always active to fix nose-up push; cannot be modified by the pilot.
const double HIDDEN_ELEVATOR_TRIM_STATE = -0.341136;

//-------------------------------------------------------------------------

// XR1 mass fully loaded; only used for landing gear limit calculations
// NOTE: we assume a LOX mass here of a 14-day loadout @ 'realistic': 25.6 kg per passenger (=128 kg)
// NOTE: max landing mass does NOT change with cheatcodes!
// we use an "average" passenger mass of 68 kg for landing limit calculations
const double FULLY_LOADED_MASS = (EMPTY_MASS + (68 * MAX_PASSENGERS) + TANK1_CAPACITY + TANK2_CAPACITY + RCS_FUEL_CAPACITY + APU_FUEL_CAPACITY + (25.6 * MAX_PASSENGERS));

// rate at which main fuel is dumped and loaded in kg/sec
const double FUEL_DUMP_RATE = 85;
const double FUEL_LOAD_RATE = 72;   // reloads slower than is dumped

// fuel flow fractions; this is a fraction of FUEL_DUMP_RATE or FUEL_LOAD_RATE for each indicated system
const double RCS_FLOW_FRACTION =    0.12;
const double SCRAM_FLOW_FRACTION =  0.40;
const double APU_FLOW_FRACTION =    0.04;
// NOTE: main flow fraction is always 1.0 since it is set by FUEL_DUMP_RATE

// LOX consumption rate in kg/second/crew member
// This is based on consumption of 0.54 kg for seven hours per crew member, 
// using C02 scrubbers to reclaim all oxygen.
// Note: this is NOT alterable via cheat codes.
const double LOX_CONSUMPTION_RATE = 2.1428571428571428571428571428571e-5; 

// LOX load and dump rates in TANK FRACTION/SECOND
// NOTE: this is adjusted depending on tank capacity to prevent slow resupply when tank capacity is small
const double LOX_DUMP_FRAC = .0081;     // empty in 123 seconds (similar to main tank)
const double LOX_LOAD_FRAC = .0069;     // fill in 144 seconds
const double LOX_MIN_DUMP_RATE = 2.262; // in kg/sec; affects flow when tank is small
const double LOX_MIN_FLOW_RATE = 1.927; // in kg/sec; affects flow when tank is small

// wing area in square meters
const double WING_AREA = 80.0;      // Original DG was 90

// wing aspect ratio, which may be computed as: span^2 / wingArea
const double WING_ASPECT_RATIO = 1.5;   // this is not actually accurate, but that's the way the stock DG was set

// wing efficiency factor
// The efficiency factor depends on the wing shape. The most efficient wings
// are elliptical, with e = 1. For all other shapes, e < 1.
const double WING_EFFICIENCY_FACTOR = 0.70;

// NOTE: max RCS thrust is mutable, and is also assigned/tweaked in the GetRCSThrustMax method

const double GEAR_OPERATING_SPEED = 0.15;
// Opening/closing speed of landing gear (1/sec)
// => gear cycle ~ 6.7 sec

const double NOSE_OPERATING_SPEED = 0.05;
// Opening/closing speed of nose cone docking mechanism (1/sec)
// => cycle = 20 sec

const double AIRLOCK_OPERATING_SPEED = 0.1;
// Opening/closing speed of outer airlock (1/sec)
// => cycle = 10 sec

const double CHAMBER_OPERATING_SPEED = 0.035714285;
// Pressurize/Depressurize speed of airlock chamber (1/sec)
// => cycle = 28 sec

const double RADIATOR_OPERATING_SPEED = 0.03125;
// Deployment speed of radiator (1/sec)
// => cycle = 32 sec

const double AIRBRAKE_OPERATING_SPEED = 0.3;
// Deployment speed of airbrakes
// => cycle = 3.3 sec

const double LADDER_OPERATING_SPEED = 0.1;
// Deployment speed of escape ladder

const double HATCH_OPERATING_SPEED = 0.15;
// Opening/closing speed of top hatch

const double RCOVER_OPERATING_SPEED = 0.3;
// Retro cover opening/closing speed

const double HOVERDOOR_OPERATING_SPEED = 0.2;
// Hover door opening/closing speed 1/speed; e.g., 1/.2 = 5 seconds (20% per second)

const double SCRAMDOOR_OPERATING_SPEED = 0.33;
// Hover door opening/closing speed 1/speed; e.g., 1/.33 = 3 seconds

// resupply line pressure data
const double MAIN_SUPPLY_PSI_LIMIT =  30.0;
const double SCRAM_SUPPLY_PSI_LIMIT = 21.0;
const double APU_SUPPLY_PSI_LIMIT =    6.0;
const double LOX_SUPPLY_PSI_LIMIT =   15.0;

// Pressure build rate fraction per second; e.g., .20 per second = 5 seconds to full pressure.
// this also determines how quickly the pressure varies while flowing.
const double PRESSURE_MOVEMENT_RATE = .20;

// factors affecting resupply pressures; this is multiplied by the LIMIT
// also, factor can vary in either direction during resupply.
const double RESUPPLY_RANDOM_LIMIT = 0.02;

// Define absolute upper and lower pressure limits while fuel flowing.
// This is a fraction of MAX pressure.
// Note: yellow zone begins @ 79%; red zone begins @ 94%
const double RESUPPLY_UPPER_LIMIT = 0.84;      // 84% max upper limit
const double RESUPPLY_LOWER_LIMIT = 0.25;      // 25% max lower limit

// defines fraction of max pressure to be the initial refueling target pressure
// NOTE: added 19% to these original values to compensate for pressure drop when fueling
const double RESUPPLY_GROUND_PSI_FACTOR = 0.741;
const double RESUPPLY_DOCKED_PSI_FACTOR = 0.530;

// coolant settings
// NOTE: lowered NOMINAL_COOLANT_TEMP by about 5 degrees due to heating balancing cooling at low values.  
// This also works out nicely with the radiator deployed and external coolant line connected, since the temperature will settle below the "normal minimum" when both lines are hooked up.
const double NOMINAL_COOLANT_TEMP  = 31.2;   // w/radiator deployed AND external cooling, stops here
const double WARN_COOLANT_TEMP     = 80.0;
const double CRITICAL_COOLANT_TEMP = 90.0;
const double COOLANT_HEATING_RATE[] = { 0.00690887811812889, 0.01515104849, 0.0 };  // 125 (113) min, 52 min, OFF : degrees C per second
const double MAX_COOLANT_TEMP = 108 + oapiRand();   // never exceeds this, although the crew would be dead by this time
const double MAX_COOLANT_GAUGE_TEMP = 110.0;
const double MIN_COOLANT_GAUGE_TEMP = 10.0;

// cooling rate fraction per second for ~67 minutes to cool to nominal @ realistic levels using only the radiator
const double COOLANT_COOLING_RATE_FRAC = 4.9751544513792169407956770249373e-4;
const double COOLANT_COOLING_RATE_MIN = 0.015;  // degrees C per second

// altitude adjustment when gear is down
const double GEAR_FULLY_UNCOMPRESSED_DISTANCE = 2.58;

// altitude at which gear is fully compressed and the hover engines may shut down for auto-descent.
const double GEAR_FULLY_COMPRESSED_DISTANCE = GEAR_FULLY_UNCOMPRESSED_DISTANCE;  // no compression for the XR1

// distance the meters that the gear can travel during compression
const double GEAR_COMPRESSION_DISTANCE = 0;    // no compression for the XR1

// multipler for wing balance shift when a crash occurs
// XR1 value = 3.0
const double CRASH_WING_BALANCE_MULTIPLIER = 3.0;

// Mach number checked at reentry; if SCRAM doors open and internal temps are > ambient and mach 
// number is >= this value, a "scram doors open" warning callout will sound.
const double MACH_REENTRY_WARNING_THRESHOLD = 19.0;

// ========= Main engine parameters ============

// 3% throttle per second
const double THROTTLE_MICRO_FRAC = 0.03;

const double MAIN_PGIMBAL_RANGE = tan (1.0*RAD);
const double MAIN_YGIMBAL_RANGE = 1.0/7.7;
// main engine pitch and yaw gimbal range (tan)

const double MAIN_PGIMBAL_SPEED = 0.007;
const double MAIN_YGIMBAL_SPEED = 0.035;
// operating speed of main engine pitch and yaw gimbals

const double MAX_HOVER_IMBALANCE = .035;    // 3.5% max imbalance
// max imbalance between front and aft hover engines in percent

const double HOVER_BALANCE_SPEED = .02;     // % imbalance per second
// operating speed of hover balance shift control

// ========== scramjet parameters ==============

// upgraded to fly to ~Mach 17 (actuals display temps are cut in half)
// This value is only used for internal SCRAMJET thrust calculations; this determines
// the upper velocity limit of the SCRAM engines.
const double SCRAM_INTERNAL_TEMAX = 16000;
// ORG: const double SCRAM_INTERNAL_TEMAX[2] = {3500.0, 3200.0};
// Max. scramjet exhaust temperature [K]

// new for XR1; used for display purposes only
const double SCRAM_COOLING = 2.0;  // divisor: effective diffuser temps after new design's cooling

// max SCRAM diffuser temperature AFTER active cooling
// this is used for display purposes
const double MAX_SCRAM_TEMPERATURE = (SCRAM_INTERNAL_TEMAX / SCRAM_COOLING);  // degrees K

// SCRAM pressure recovery value; tune this to change the operating envelope of the SCRAM engines
const double SCRAM_PRESSURE_RECOVERY_MULT = 0.9;  // good for Mach 17 now; org DG value was 1.35

// SCRAM DMA scale value; tune this to change the operating envelope of the SCRAM engines
const double SCRAM_DMA_SCALE = 1.35e-4; // good for Mach 17; org DG value was 2.7e-4

const double SCRAM_INTAKE_AREA = 1.0;
// scramjet intake cross section (per engine) [m^2]

// ORG: const double SCRAM_DEFAULT_DIR = 9.0*RAD;
// XR1 ORG: const double SCRAM_DEFAULT_DIR = (8.0*RAD);  // {DEB} level it out
const double SCRAM_DEFAULT_DIR = (0.0*RAD);  // completely flat for version 1.3
// Default scramjet thrust angle (rad)

const double SCRAM_GIMBAL_RANGE = (5.0*RAD);
// scramjet gimbal range (rad)

const double SCRAM_GIMBAL_SPEED = SCRAM_GIMBAL_RANGE/3.0;
// Operating speed of scramjet pitch gimbals (rad/s)

// SCRAM gauge limits
const double SCRAM_FLOW_GAUGE_MAX = 3.0;
const double SCRAM_TSFC_GAUGE_MAX = 0.03;

// Main/Hover fuel flow limits
const double MAIN_FLOW_GAUGE_MAX = 19.5;
const double HOVER_FLOW_GAUGE_MAX = 13.8;

// cabin O2 data
const double NORMAL_O2_LEVEL = .209;    // 20.9% O2 level
const double CRITICAL_O2_LEVEL_WARNING = .16;   // hypoxia effects below this level

// this level will replentish O2 levels from .10 to .20 in about 20 seconds
const double AMBIENT_O2_REPLENTISHMENT_RATE = .00526;  // rate per second cabin O2 replentished due to LOX available now
// this level will yield 7 minutes to go from .209 to .10 (unconsciousness) in 7 minutes
const double AMBIENT_O2_CONSUMPTION_RATE = 5.1904761904761904761904761904762e-5;  // O2 pct level consumption per crew member per second
const double CREW_LOC_O2_LEVEL = .10 + (oapiRand() * .01);   // crew unconscious at this O2 level
const double CREW_DEATH_O2_LEVEL = .09 - (oapiRand() * .01); // crew death at this O2 level

// maximum crew complement, including pilot
const int MAX_PASSENGERS = 5;

// ============ Damage parameters ==============

// Max. allowed positive and negative wing load [N/m^2]
const double WINGLOAD_MAX           =  17e3;  // 17000 N/m^2
const double WINGLOAD_MIN           = -11e3;  // 11000 N/m^2
const double RADIATOR_LIMIT         = 16e3;   // pascals dynamic pressure
const double HATCH_OPEN_LIMIT       = 20e3;   // 20 kPa will damage top hatch if open
const double OPEN_NOSECONE_LIMIT    = 32e3;
const double GEAR_LIMIT             = 39e3;
const double RETRO_DOOR_LIMIT       = 41e3; 

const double DOOR_DYNAMIC_PRESSURE_WARNING_THRESHOLD = 0.75; // issue "door open" warning for pressure

// defines the LAST VALID damage item for this vessel
const DamageItem D_END = RCS14;

// Max. allowed dynamic pressure [Pa]
const double DYNP_MAX = 150e3;  // 150 kPa

// NEW SECTION for XR1

// landing gear momentum limit
// limit is in kg-m/s units; a fully-loaded XR1 could land at 3 m/s^2 descent max.
const double LANDING_GEAR_MAX_MOMEMTUM = (FULLY_LOADED_MASS * 3.0);
const double FULL_CRASH_THRESHOLD = (LANDING_GEAR_MAX_MOMEMTUM * 3);  // above this limit, full crash occurs (as opposed to just gear collapse)

// pitch and bank touchdown limits; exceeding these will result in a crash
const double TOUCHDOWN_BANK_LIMIT = (15 * RAD); 
const double TOUCHDOWN_MAX_PITCH = (16 * RAD);
const double TOUCHDOWN_MIN_PITCH = (-5 * RAD);  // -5 degrees is lenient

// m/s vertical impact velocity above which the crew will not survive
const double CREW_IMPACT_DEATH_THRESHOLD = 39.0;  

// m/s vertical impact velocity above which the crew will sustain SEVERELY injuries during a belly-landing or gear-collapse
const double CREW_IMPACT_SEVERE_INJURY_THRESHOLD = 29.0;

// m/s vertical impact velocity above which the crew will sustain MODERATE injuries during a belly-landing or gear-collapse
const double CREW_IMPACT_MODERATE_INJURY_THRESHOLD = 12.0;

// m/s vertical impact velocity above which the crew will sustain MINOR injuries during a belly-landing or gear-collapse
const double CREW_IMPACT_MINOR_INJURY_THRESHOLD = 2.7;

// multiplier used to add heat during reentry; this is multiplied by speed*pressure
const double HULL_HEATING_FACTOR = 3.1034e-10;

// static pressure threshold at which OAT and Mach values are valid. 
// (APPROX) AS SEEN ON SURFACE MFD, BUT TOO RISKY TO USE IN PRODUCTION: const double OAT_VALID_STATICP_THRESHOLD = 0.014;  // in pascals
const double OAT_VALID_STATICP_THRESHOLD = 0.02;  // in pascals

// end section

// normal COL for the wings
// ORG: const double NEUTRAL_CENTER_OF_LIFT = -0.15;  // in meters
const double NEUTRAL_CENTER_OF_LIFT = 0.0;  // in meters  (makes ship stable landing in atmosphere)

// =============================================

//
// Globals
//
HMODULE g_hDLL;  // our DLL handle

const double MAX_DESCENT_HOLD_RATE = 990;   // in m/s (1.22 and earlier: was 250 m/s)
const double ADRATE_SMALL = 0.1;
const double ADRATE_MED   = 0.5;
const double ADRATE_LARGE = 2.5;

const double ASRATE_TINY  = 0.1;
const double ASRATE_SMALL = 1.0;
const double ASRATE_MED   = 5.0;
const double ASRATE_LARGE = 25.0;

// volume constants
const int QUIET_CLICK = 200;
const int MED_CLICK = 225;
const int MFD_CLICK = 210;
const int AUTOPILOT_VOL = 220;
const int WARNING_BEEP_VOL = 230;
const int GEAR_WHINE_VOL = 210;
const int DOOR_WHINE_VOL = 255;
const int ERROR1_VOL = 220;
const int APU_VOL = 130;
const int FUEL_XFEED_VOL = 180;
const int FUEL_DUMP_BASE_VOL = 205;      // volume for ONE fuel/LOX line flowing (may be 5)
const int FUEL_DUMP_INC_VOL = 10;        // * 3 = max increment; one per open fuel line
const int FUEL_RESUPPLY_BASE_VOL = 215;  // volume for ONE fuel/LOX line flowing (may be 4)
const int FUEL_RESUPPLY_INC_VOL = 10;    // * 3 = max increment; one per open fuel line
const int SUPPLY_HATCH_VOL = 220;
const int RESUPPLY_LINE_EXTEND_VOL = 220;
const int AIRLOCK_CHAMBER_VOLUME = 64;  // sound of air whooshing

// # of meters to shift the center of lift per second as the autopilot or the user adjustes it.
// This will help to ship to maintain a nose-up attitude during reentry while expending very little RCS fuel.
//
// Note: if this value is too large, the autopilot will keep "hunting" for the optimum COL and the 
// upper and lower RCS thrusters will keep firing in alternate groups.  If the value is too small, the autopilot
// will take too long to adjust to pitch/AOA target changes.  
// Also note that the actual step value will be based on the percentage of thrust fired by the RCS jets, up to
// a maximum of this value.  
// XR1: value set to reach 40-degree AOA target shift of 0.23288 meter in two seconds assuming RCS jets are firing at 10% (0.23288 * 10 / 2).
const double COL_MAX_SHIFT_RATE = 1.1644;

// absolute limit the autopilot or the user is allowed to shift the center-of-lift (+/-)
const double COL_MAX_SHIFT_DISTANCE = 4.115;

// the limit of the COG shift slider gauge; this is usually close to COL_MAX_SHIFT_DISTANCE
const double COL_SHIFT_GAUGE_LIMIT = 3.9;

// the fractional rate of COL_MAX_SHIFT_RATE that the COG shift keys move the COG; 0 < n < 1.0
const double COL_KEY_SHIFT_RATE_FRACTION = 0.10;    // 1/20th maximum speed; the XR1 is very sensitive to it

// sound file customization
const char *SCRAMJET_WAV = "ScramJet.wav";
const char *WELCOME_ABOARD_ALL_SYSTEMS_NOMINAL_WAV = "Welcome Aboard All XR1 Systems Nominal.wav";
const char *ALL_SYSTEMS_NOMINAL_WAV = "All XR1 Systems Nominal.wav";
const char *WARNING_OUTER_DOOR_IS_LOCKED_WAV = "Warning Nosecone is Closed Outer Door is Locked.wav";
const char *WARNING_NOSECONE_IS_CLOSED_WAV = "Warning Nosecone is Closed.wav";
const char *WARNING_NOSECONE_OPEN_WAV = "Warning Nosecone Open.wav";

// labels to handle nosecone and/or a docking port
const char *NOSECONE_LABEL = "Nosecone";
const char *NOSECONE_SHORT_LABEL = "Nose";  // used in the scenario file to show damage
const char *NOSECONE_SCN = "NOSECONE";      // tag value in scenario files

const int MAX_MAINFUEL_ISP_CONFIG_OPTION = 6;  // upper limit for MainFuelISP in config file

//
// Autopilot constants
//

// attitude hold: pitch and bank deltas per mouse click or key press
const double AP_PITCH_DELTA_SMALL = 0.5;       // in degrees
const double AP_PITCH_DELTA_LARGE = 2.5;       // in degrees
const double AP_BANK_DELTA  = 5.0;             // in degrees

// thruster level dead zone for RCS thrust; if the thrust exceeds this level
// a center-of-lift shift will be performed.  If the ship is alternately firing the RCS up/down
// jets, try increasing this until only the positive jets fire.
const double AP_COL_DEAD_ZONE = 0.04;   // allow up to 4.0% RCS thrust before a COL shift is performed

// Ratio of thruster level (0...1) to shift step strength (0..1).  For example, 1.0 means that at RCS thrust level 1.0 (100%) a full shift step will be used.
// Similarly, 2.0 means that at RCS thrust level 1.0/2=0.50 (50%) a full shift step will be used.
// A value other than 1.0 allows you to use larger step sizes for a given thrust level without increasing the step size and causing
// the COL shift to lose precision.  Higher values mean that a full step will be used sooner (i.e., at a lower RCS thrust level).
const double AP_COL_THRUSTLEVEL_TO_SHIFTSTEP_RATIO = 4.0;

// Multiplier for max RCS thrust while attitude hold active
const double AP_ATTITUDE_HOLD_RCS_THRUST_MULTIPLIER = 5.0;    // 5x power on all jets to hold in high AOA situations

// Panel ID of the first virtual cockpit panel (just beyond the last 2D panel).  2D panels start at ID 0.
const int VC_PANEL_ID_BASE = 3;

// elevator trim dead zone for COL shift, in meters.  If the current fabs(COL value) is > this value, an elevator trim step will be performed.
extern const double AP_ELEVATOR_TRIM_COL_DEAD_ZONE = 0.1;

// elevator trim fraction to move per second for manual movement
extern const double ELEVATOR_TRIM_SPEED = 0.20;

// elevator trim fraction to move per second for autopilot movement
extern const double AP_ELEVATOR_TRIM_SPEED = 0.20;

// angular velocity degreesDelta fraction multiplier to reach target attitude in a reasonable time
// If this value is too large, the ship will roll too fast and "overshoot" the target at lower frame rates.
// If this value is too small, the ship will take too long to reach the target attitude, wasting RCS fuel in atmospheric flight.
extern const double AP_ANGULAR_VELOCITY_DEGREES_DELTA_FRAC = 0.5;

//-------------------------------------------------------------------------
// XR1ConfigFileParser data
//-------------------------------------------------------------------------

// Table of LOX mass in kg at REALISTIC consumption level; one entry for each config option.
const double XR1ConfigFileParser::m_loxLoadoutArray[] = 
{
    65,     // 7 days
    130,    // 14 days
    283,    // one month
    848,    // three months
    1695,   // six months
    3389,   // one year
    6777,   // two years
    10165,  // three years
    13553,  // four years
    16942   // five years
};

// LOX consumption fractions for AUTO mode; one elemement for each value in m_loxLoadoutArray
const double XR1ConfigFileParser::m_autoLoxConsumptionArray[] = 
{
    1.0,    // 7 days
    1.0,    // 14 days
    1.0,    // one month
    0.75,   // three months
    0.60,   // six months
    0.38,   // one year
    0.222,  // two years
    0.1682, // three years
    0.1411, // four years
    0.125   // five years
};

// Table LOX consumption fractions, from 0 (NONE) to 4 (REALISTIC)
const double XR1ConfigFileParser::m_loxConsumptionArray[] = 
{
    0.0,    // 0: disabled
    0.25,   // 1: very low
    0.50,   // 2: low
    0.75,   // 3: medium
    1.0     // 4: realistic
};

// Main fuel ISP table
// NOTE: DG default was 40000
const double XR1ConfigFileParser::m_mainFuelISPArray[] = 
{
    13943.1603938272,    // 0: Expert    (ISS Only w/expert use of SCRAM engines and expert deorbit/landing)
    20914.7405907408,    // 1: Realistic (ISS Only)
    25962.38443509765,   // 2: Default   (ISS and Moon)
    32981.19221754767,   // 3: Medium    (ISS and Moon w/reserve)
    40000.0,             // 4: Stock DG  (Moon w/large reserve; this is the original stock DG setting)
    52922.8282523788,    // 5: Big       (Mars)
    366251.528451608     // 6: Huge      (Jupiter+)
};

// SCRAM fuel ISP multiplier table
const double XR1ConfigFileParser::m_scramFuelISPArray[] = 
{
    1.0,    // 0: realistic
    1.5,    // 1: 1.5x normal
    3.0,    // 2: 3x normal
    6.0,    // 3: 6x normal
    10.0    // 4: 10x normal
};

// max fuel flow for a single SCRAM engine in kg/sec
const double XR1ConfigFileParser::m_scramMaxDMF[] =
{
    3.0,    // 0 = 3.0 kg/sec (easy)
    2.0     // 1 = 2.0 kg/sec (realistic)
};

// APU fuel burn rate in kg/minute
const double XR1ConfigFileParser::m_apuFuelBurnRate[] = 
{
    0.0,            // 0 = unlimited (runs indefinitely)
    0.90718474,     // 1 = very low  (2 lb/minute)    (3.7 hours runtime)
    1.81436948,     // 2 = low       (4 lb/minute)    (110 minutes runtime)
    2.72155422,     // 3 = moderate  (6 lb/minute)    (74 hours runtime)
    4.08233134,     // 4 = realistic (9 lb/minute)    (49 minutes runtime)
    6.12349701,     // 5 = expert    (13.5 lb/minute) (33 minutes runtime)
};

// these are required by the framework in order to link, but the XR1 does not use them because it has no payload
const VECTOR3 &PAYLOAD_SLOT_DIMENSIONS = _V(0, 0, 0);
const char *DEFAULT_PAYLOAD_THUMBNAIL_PATH = "";

// welcome messages
const char *WELCOME_MSG = "Welcome aboard, Commander!&All XR1 systems nominal.";
const char *ALL_SYSTEMS_NOMINAL_MSG = "All XR1 systems nominal.";

// callout globals
// takeoff callouts in meters-per-second
const double V1_CALLOUT_AIRSPEED = 85;
const double ROTATE_CALLOUT_AIRSPEED_EMPTY = 110;  // no payload  (actually rotates at 100 m/s, but rotation is too slow at that speed)
const double ROTATE_CALLOUT_AIRSPEED_HEAVY = 110;  // (N/A for the XR1)
const double MAX_RECOMMENDED_PAYLOAD_MASS = 0;     // used for Vr callout calculation only

#ifdef TURBOPACKS
// turbopack data
const Turbopack TURBOPACKS_ARRAY[] =
{
    {
        "Standard Turbopack",
        "UMmuturbopack"
    }
};

// number of turbopacks in TURBOPACKS_ARRAY
const int TURBOPACKS_ARRAY_SIZE = sizeof(TURBOPACKS_ARRAY) / sizeof(Turbopack);

// vessel-relative coordinates where turbopacks spawn during deployment
const VECTOR3 &TURBOPACK_SPAWN_COORDINATES = _V(0, 0, 15.0);

// maximum distance in meters of turbopacks that will be auto-stowed
const double STOW_TURBOPACK_DISTANCE = 20;
#endif

////////////////////////////////////////////////////////////////
// UNUSED by XR1: globals to satisfy the linker

// cargo mass (may change as cargo is loaded/unloaded)
double CARGO_MASS = -1.0;   // -1.0 = "not set"

// grapple display ranges in meters
const double GRAPPLE_DISPLAY_RANGES[] = { 50, 100, 250, 500, 1e3, 1e4, 1e5 };

// # of grapple display ranges
const int GRAPPLE_DISPLAY_RANGE_COUNT = sizeof(GRAPPLE_DISPLAY_RANGES) / sizeof(double);

// the maximum range that a payload module may be grappled in orbit
double PAYLOAD_GRAPPLE_RANGE_ORBIT = 22.0;

// the maximum range that a payload module may be grappled while landed
double PAYLOAD_GRAPPLE_RANGE_LANDED = 400.0;

// the maximum deltaV at which a payload module may be grappled, in m/s
double PAYLOAD_GRAPPLE_MAX_DELTAV = 0.5;

// front and rear tire circumference; only used for wheel rotation animation
const double FRONT_TIRE_CIRCUMFERENCE = 0.717 * PI;
const double REAR_TIRE_CIRCUMFERENCE  = 1.128 * PI;

// Deceleration rate for wheel rotation to slow to a stop due to drag.
// Value is in meters per second @ the tire's outer edge.
// XR5 ORG: 7.6423
const double TIRE_DECELERATION_RATE = 7.6423;

// Gear Compression: These are NOT USED by the XR1.

// distance to center of nose gear strut from centerpoint
const double NOSE_GEAR_ZCOORD = 0;          

// distance to center of rear gear strut from centerpoint
const double REAR_GEAR_ZCOORD = 0;          

// distance from centerpoint to bottom of tires, both front and rear
const double GEAR_UNCOMPRESSED_YCOORD = 0;  

// These factors will multiply front and rear gear translation distance for gear compression
// ("altitude") by our "angled strut" factor.
// i.e., if the strut deploys to a non-vertical angle we have to deploy slightly more than we would at 90 degrees;
// This is because the hypotenuse is always longer than the altitude of a triangle.
// 1.0 = "strut is vertical"
const double FRONT_GEAR_COMPRESSION_TRANSLATION_FACTOR = 1.0;
const double REAR_GEAR_COMPRESSION_TRANSLATION_FACTOR = 1.0;

// fuel/LOX dump particle stream coordinates; not used by the XR1
const VECTOR3 &FUEL_DUMP_PARTICLE_STREAM_POS1 = _V(0,0,0);
const VECTOR3 &FUEL_DUMP_PARTICLE_STREAM_DIR1 = _V(0,0,0);
const VECTOR3 &FUEL_DUMP_PARTICLE_STREAM_POS2 = _V(0,0,0);
const VECTOR3 &FUEL_DUMP_PARTICLE_STREAM_DIR2 = _V(0,0,0);

// boil-off exhaust particle stream coordinates; not used by the XR1
const VECTOR3 &BOIL_OFF_PARTICLE_STREAM_POS1 = _V(0,0,0);
const VECTOR3 &BOIL_OFF_PARTICLE_STREAM_DIR1 = _V(0,0,0);
const VECTOR3 &BOIL_OFF_PARTICLE_STREAM_POS2 = _V(0,0,0);
const VECTOR3 &BOIL_OFF_PARTICLE_STREAM_DIR2 = _V(0,0,0);

////////////////////////////////////////////////////////////////

// resource ID globals used by common areas
// TODO: some of the remaining XR1 base class areas still use constants from resource.h;
// these currently work with subclasses only because the resource.h happens to match, since
// the subclasses copied the XR1's resource.h as a base.  The real fix, however, is to abstract
// out each shared resource ID here as a global.
const int RES_IDB_FUEL_GAUGE = IDB_FUEL_GAUGE;
const int RES_IDB_FUEL_GAUGE_DARK = -1;  // no payload in the XR1, so no dark gauge textures

const int RES_IDB_LOX_GAUGE = IDB_LOX_GAUGE;
const int RES_IDB_LOX_GAUGE_DARK = -1;  // no payload in the XR1, so no dark gauge textures

const int RES_IDB_COOLANT_GAUGE = IDB_COOLANT_GAUGE;

////////////////////////////////
// payload dialog static data //
////////////////////////////////

// these are here only so the DLL can link; there is no payload in the XR1
const int XR1PayloadDialog::slotCount = 0;
const int XR1PayloadDialog::slotResourceIDs[1] = { 0 };  // not used
const int GLOBAL_IDD_PAYLOAD_EDITOR = -1;  // not used

// number of spotlights defined
const int SPOTLIGHT_COUNT = 2;

// values are in meters
const double HEIGHT_WHEN_LANDED = 4.72;
const double HULL_LENGTH = 17.76;
const double HULL_WIDTH = 17.86;

// these are cloned from the DeltaGlider in the Orbiter 2016 source; list is null-terminated
const VECTOR3 HULL_TOUCHDOWN_POINTS[] =
{
	_V(-8.5, -0.3, -7.05),
	_V(8.5, -0.3, -7.05),
	_V(-8.5, -0.4, -3),
	_V(8.5, -0.4, -3),
	_V(-8.85, 2.3, -5.05),
	_V(8.85, 2.3, -5.05),
	_V(-8.85, 2.3, -7.05),
	_V(8.85, 2.3, -7.05),
	_V(0, 2, 6.2),
	_V(0, -0.6, 10.65)
};
const int HULL_TOUCHDOWN_POINTS_COUNT = sizeof(HULL_TOUCHDOWN_POINTS) / sizeof(VECTOR3);
