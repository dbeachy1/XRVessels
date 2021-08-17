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
// XR3Phoenix implementation class
//
// XR3Globals.h
// Contains global variable values for the XR3.
// ==============================================================

#include "XR3Globals.h"    // verify that types agree
#include "XR3ConfigFileParser.h"
#include "XR1PayloadDialog.h"

//
// Version globals
//

const char *VESSELNAME = "XR3Phoenix";

// VERSION ID
// {XXX} UPDATE THIS FOR THE CURRENT BUILD VERSION; DO NOT REMOVE THIS {XXX} COMMENT
const char* VERSION = "Version 0.1 ALPHA-1 [" ARCH_TYPE  " " BUILD_TYPE "], Build Date : " __DATE__;

// file is always written to the Orbiter directory
const char *XR_LOG_FILE = "XR3Phoenix.log";

const char *XR_CONFIG_FILE = "Config\\XR3PhoenixPrefs.cfg";

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
    "ALT-U",        "Deploy Selected Payload",
    "CTRL-ALT-U",   "Deploy All Payload",
    "ALT-G",        "Grapple Selected Payload",
    "CTRL-ALT-G",   "Grapple All Payload",
    "ALT-B",        "Toggle Payload Editor Window",

    "ALT-H",        "Change HUD Color",
    "ALT-Z",        "Dec HUD Brightness",
    "ALT-X",        "Inc HUD Brightness",
    "CTRL-\\",      "Retro Doors",
    "CTRL-G",       "SCRAM Doors",
    "CTRL-V",       "Hover Doors",
    "CTRL-E",       "Elevator",  //     XR3TODO: make this the crew ladder to the ground
    "CTRL-U",       "Payload Bay Doors",
    "ALT-,",        "Shift Center of Gravity Aft",
    "ALT-.",        "Shift Center of Gravity Forward",
    "ALT-M",        "Recenter Center of Gravity",
    "ALT-J",        "Toggle DOCKING/NORMAL RCS config",
    "CTRL-ALT-R",   "Set visual docking target (docking HUD)",
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
    "CTRL-K",       "Docking Port",
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

    nullptr, nullptr   // null-terminate the array
};

// ==============================================================
// Some vessel class caps
// Where an entry consists of two values, the first refers to the
// "easy", the second to the "complex" flight model.
// ==============================================================

// ==============================================================
// CHEATCODE globals
// ==============================================================

double EMPTY_MASS = 60629;  // mass w/o fuel
double TANK1_CAPACITY = 52545;   // Main fuel
double TANK2_CAPACITY = 12631;   // SCRAMjet fuel

double RCS_FUEL_CAPACITY = 3031;  // Max fuel capacity: RCS tank [kg]

double APU_FUEL_CAPACITY = 1010;  // Max APU fuel capacity [kg]

double MAX_MAIN_THRUST[2] = { 1424.79e3, 1139.83e3 };   // EASY, REALISTIC thrust levels in newtons
// Main engine max vacuum thrust [N] per engine. (x2 for total)
// NOTE: don't need to track main engine damage here since thrust is not set by gimbaling

double MAX_RETRO_THRUST = 905.76e3;  // Retro engine max vacuum thrust [N] per engine. (x2 for total)

double MAX_HOVER_THRUST[2] = { 1078.15e3, 783.64e3 };  // Hover engine max vacuum thrust [N] (x2 for total) at EASY, REALISTIC 

double MAX_RCS_THRUST = 14.84e3;

double MAX_WHEELBRAKE_FORCE = 505245.07;

// {DEB} Hydrogen FHV = 1.42e8 J/kg.
// This is the value to set if you want to change the SCRAM engines' ISP.
// XR1 ORG: double SCRAM_FHV[2] = {3.5e8, 2.0e8};
double SCRAM_FHV[2] = { 4.2e8, 2.4e8 };       // EASY, REALISTIC  XR3TODO: matches XR2 for now; tweak this as necessary to reach orbit on nominal SCRAM ascent with ~5% fuel remaining
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
const double HIDDEN_ELEVATOR_TRIM_STATE = -0.598760;   //   XR3TODO: tweak this as necessary per testing

//-------------------------------------------------------------------------

// XR3 mass fully loaded WITHOUT payload; only used for landing gear limit calculations
// NOTE: we assume a LOX mass here // NOTE: we assume a LOX mass here of a 14-day loadout @ 'realistic': 25.6 kg per passenger
// NOTE: max landing mass does NOT change with cheatcodes!
// we use an "average" passenger mass of 68 kg for landing limit calculations
const double FULLY_LOADED_MASS = (EMPTY_MASS + (68.0 * MAX_PASSENGERS) + TANK1_CAPACITY + TANK2_CAPACITY + RCS_FUEL_CAPACITY + APU_FUEL_CAPACITY + (25.6 * MAX_PASSENGERS));

// rate at which main fuel is dumped and loaded in kg/sec
const double FUEL_DUMP_RATE = 85 * 5.94;   // x XR1Multiplier
const double FUEL_LOAD_RATE = 72 * 5.94;   // reloads slower than is dumped

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
const double WING_AREA = 437.65;   // XR3TODO: tweak this to achieve desired lift & reentry characteristics

// wing aspect ratio, which may be computed as: span^2 / wingArea
const double WING_ASPECT_RATIO = (29.49 * 29.49) / WING_AREA; 

// wing efficiency factor
// The efficiency factor depends on the wing shape. The most efficient wings
// are elliptical, with e = 1. For all other shapes, e < 1.
const double WING_EFFICIENCY_FACTOR = 0.81;   // XR1/DG is 0.7; 0.81 matches XR2 & XR5

// NOTE: max RCS thrust is mutable, and is also assigned/tweaked in the GetRCSThrustMax method

// these values are the rate to fraction to move in one second; e.g., for 35 seconds: 1/35 = 0.0285714285714286
const double BAY_OPERATING_SPEED = 1.0 / 27.0;    // XR2=22 seconds, XR5=35 seconds

const double ELEVATOR_OPERATING_SPEED = 1.0 / 8.0;  // XR3TODO: delete this; use LADDER_OPERATING_SPEED instead (defined later in this method)

const double GEAR_OPERATING_SPEED = 0.15;   // matches other XR vessels
// Opening/closing speed of landing gear (1/sec)
// => gear cycle ~ 6.7 sec

const double NOSE_OPERATING_SPEED = 0.05;
// Opening/closing speed of the docking port mechanism (1/sec)
// => cycle = 20 sec

const double AIRLOCK_OPERATING_SPEED = 0.1;
// Opening/closing speed of outer airlock (1/sec)
// => cycle = 10 sec

const double CHAMBER_OPERATING_SPEED = 0.035714285;
// Pressurize/Depressurize speed of airlock chamber (1/sec)
// => cycle = 28 sec

const double RADIATOR_OPERATING_SPEED = 1.0 / 20.0;    // XR2 was 16 seconds, XR5 was 32 seconds
// Deployment speed of radiator (1/sec)

const double AIRBRAKE_OPERATING_SPEED = 1.0 / 3.3;  // matches other XR vessels
// Deployment speed of airbrakes

const double LADDER_OPERATING_SPEED = 1.0 / 10.0;
// Deployment speed of escape ladder

const double HATCH_OPERATING_SPEED = 1.0 / 1.67;
// Opening/closing speed of top hatch

const double RCOVER_OPERATING_SPEED = 1.0 / 3.33;
// Retro cover opening/closing speed

const double HOVERDOOR_OPERATING_SPEED = 1.0 / 5.0;
// Hover door opening/closing speed 1/speed; e.g., 1/.2 = 5 seconds (20% per second)

const double SCRAMDOOR_OPERATING_SPEED = 1.0 / 3.0;
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
const double MAX_COOLANT_TEMP = 117 + oapiRand();   // never exceeds this, although the crew would be dead by this time
const double MAX_COOLANT_GAUGE_TEMP = 110.0;
const double MIN_COOLANT_GAUGE_TEMP = 10.0;

// cooling rate fraction per second for ~67 minutes to cool to nominal @ realistic levels
const double COOLANT_COOLING_RATE_FRAC = 4.9751544513792169407956770249373e-4;
const double COOLANT_COOLING_RATE_MIN = 0.015;  // degrees C per second

// altitude adjustment when gear is down, in meters.  This for FULLY UNCOMPRESSED gear.
const double GEAR_FULLY_UNCOMPRESSED_DISTANCE = -GEAR_UNCOMPRESSED_YCOORD;

// altitude at which gear is fully compressed and the hover engines may shut down for auto-descent.
const double GEAR_FULLY_COMPRESSED_DISTANCE = -GEAR_UNCOMPRESSED_YCOORD - GEAR_COMPRESSION_DISTANCE;

// distance the meters that the gear can travel during compression
// PRE-1.3 RC2: const double GEAR_COMPRESSION_DISTANCE = 1.97;    // in meters; this affects the touchdown point distance along the Y axis
const double GEAR_COMPRESSION_DISTANCE = 0;    // in meters; this affects the touchdown point distance along the Y axis  XR3TODO: set this if and when gear compression is added

// multipler for wing balance shift when a crash occurs
// XR1 value = 3.0
const double CRASH_WING_BALANCE_MULTIPLIER = 0;  // anything > 0 induces a wild spin

// Mach number checked at reentry; if SCRAM doors open and internal temps are > ambient and mach 
// number is >= this value, a "scram doors open" warning callout will sound.
const double MACH_REENTRY_WARNING_THRESHOLD = 22.0;

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

// upgraded to fly to ~Mach 20.5 (actuals display temps are reduced by a divisor)
// This value is only used for internal SCRAMJET thrust calculations; this determines
// the upper velocity limit of the SCRAM engines.
const double SCRAM_INTERNAL_TEMAX = 20500; 
// XR1 org: 16000
// ORG: const double SCRAM_INTERNAL_TEMAX[2] = {3500.0, 3200.0};
// Max. scramjet exhaust temperature [K]

// new for XR1; used for display purposes only
// XR1 org: 2.0.  New value still tops out at 8000K.
const double SCRAM_COOLING = 2.5625;  // divisor: effective diffuser temps after new design's cooling

// max SCRAM diffuser temperature AFTER active cooling
// this is used for display purposes
const double MAX_SCRAM_TEMPERATURE = (SCRAM_INTERNAL_TEMAX / SCRAM_COOLING);  // degrees K

// NOTE: this is 1.0 XR1 times the XR3:XR1 multiplier
const double SCRAM_INTAKE_AREA = 1.0 * XR1_MULTIPLIER;       
// scramjet intake cross section (per engine) [m^2]

// SCRAM pressure recovery value; tune this to change the operating envelope of the SCRAM engines
// XR1 org value: 0.9 (mach 17)
// DG org value: 1.35 (mach 8)
const double SCRAM_PRESSURE_RECOVERY_MULT = 0.765;  // good for Mach 20 now

// SCRAM DMA scale value; tune this to change the operating envelope of the SCRAM engines
// XR1 org value: 1.35e-4 (mach 17)
// DG org value: 2.7e-4 (mach 8)
const double SCRAM_DMA_SCALE = 1.1475e-4; // good for Mach 20

const double SCRAM_DEFAULT_DIR = 0.0;  // XR3 simulates balancing the scrams by mounting them on the centerline
// Default scramjet thrust angle (rad)

const double SCRAM_GIMBAL_RANGE = (5.0*RAD);
// scramjet gimbal range (rad)

const double SCRAM_GIMBAL_SPEED = SCRAM_GIMBAL_RANGE/3.0;
// Operating speed of scramjet pitch gimbals (rad/s)

// SCRAM gauge limits
const double SCRAM_FLOW_GAUGE_MAX = 66.0;
const double SCRAM_TSFC_GAUGE_MAX = 0.015;

// Main/Hover fuel flow limits
// XR3TODO: update the numbers on the gauges to match these values
const double MAIN_FLOW_GAUGE_MAX = 19.5 * XR1_MULTIPLIER;   // = 98.5  
const double HOVER_FLOW_GAUGE_MAX = 13.8 * XR1_MULTIPLIER;  // = 69.7

// cabin O2 data
const double NORMAL_O2_LEVEL = .209;    // 20.9% O2 level
const double CRITICAL_O2_LEVEL_WARNING = .16;   // hypoxia effects below this level

// this level will replentish O2 levels from .10 to .20 in about 20 seconds
const double AMBIENT_O2_REPLENTISHMENT_RATE = .00526;  // rate per second cabin O2 replentished due to LOX available now
// This level will yield 7 minutes to go from .209 to .10 (unconsciousness) in 7 minutes
// Note: original value was calibrated for 5 crew members in the XR1, so we must adjust the level DOWN for that below (MAXP/5) to accomodate the large crew (and cabin).
// In other words, this consumption rate is *per crew member*, so that is why we adjust it for MAX_PASSENGERS: a full crew will still have seven minutes to unconsciousness.
// For less-than-full crews, the larger MAX_PASSENGERS is the longer each crew member has before becoming unconscious. 
const double AMBIENT_O2_CONSUMPTION_RATE = 5.1904761904761904761904761904762e-5 / (MAX_PASSENGERS / 5);  // O2 pct level consumption per crew member per second
const double CREW_LOC_O2_LEVEL = .10 + (oapiRand() * .01);   // crew unconscious at this O2 level
const double CREW_DEATH_O2_LEVEL = .09 - (oapiRand() * .01); // crew death at this O2 level

// maximum crew complement, including pilot
const int MAX_PASSENGERS = 18;      // XR5 also had a crew complement of 18

// ============ Damage parameters ==============

// Max. allowed positive and negative wing load [N/m^2]
const double WINGLOAD_MAX           =  17e3;  // 17000 N/m^2
const double WINGLOAD_MIN           = -11e3;  // 11000 N/m^2
const double ELEVATOR_LIMIT         = 9e3;      // XR3TODO: convert this to ladder limit (or just make ladder not damageable)
const double RADIATOR_LIMIT         = 16e3;   // pascals dynamic pressure
const double HATCH_OPEN_LIMIT       = 20e3;   // 20 kPa will damage top hatch if open
const double OPEN_NOSECONE_LIMIT    = 32e3;   // Note: this is actually docking port limit!
const double BAY_LIMIT              = 36e3;
const double GEAR_LIMIT             = 39e3;
const double RETRO_DOOR_LIMIT       = 41e3; 

const double DOOR_DYNAMIC_PRESSURE_WARNING_THRESHOLD = 0.75; // issue "door open" warning for pressure

// defines the LAST VALID damage item for this vessel
const DamageItem D_END = DamageItem::Elevator;    // XR3TODO: either use this for crew ladder or remove it

// Max. allowed dynamic pressure [Pa]
const double DYNP_MAX = 150e3;  // 150 kPa

// landing gear momentum limit
// limit is in kg-m/s units; a fully-loaded XR3 could land at 3.7 m/s descent max: this is up from the XR1's 3.0 m/s; matches XR2's 3.7.
const double LANDING_GEAR_MAX_MOMEMTUM = (FULLY_LOADED_MASS * 3.7);
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
const double CREW_IMPACT_MINOR_INJURY_THRESHOLD = 2.7;  // matches XR2's value

// multiplier used to add heat during reentry; this is multiplied by speed*pressure
// XR1 VALUE: 3.1034e-10
const double HULL_HEATING_FACTOR = 3.1034e-10 * 1.479167;   // reach temp limit @ 1 degree slope w/full fuel and cargo  XR3TODO: tweak this to get desired hull heating performance under full cargo load

// static pressure threshold at which OAT and Mach values are valid. 
// (APPROX) AS SEEN ON SURFACE MFD, BUT TOO RISKY TO USE IN PRODUCTION: const double OAT_VALID_STATICP_THRESHOLD = 0.014;  // in pascals
const double OAT_VALID_STATICP_THRESHOLD = 0.02;  // in pascals

// end section  

// Panel ID of the first virtual cockpit panel (just beyond the last 2D panel).  2D panels start at ID 0.
const int VC_PANEL_ID_BASE = 100;   // XR3TODO: set this if and when we add a VC

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

// normal COL for the wings
const double NEUTRAL_CENTER_OF_LIFT = 0.0;  // in meters

// # of meters to shift the center of lift per second as the autopilot or the user adjustes it.
// This will help to ship to maintain a nose-up attitude during reentry while expending very little RCS fuel.
//
// Note: if this value is too large, the autopilot will keep "hunting" for the optimum COL and the 
// upper and lower RCS thrusters will keep firing in alternate groups.  If the value is too small, the autopilot
// will take too long to adjust to pitch/AOA target changes.  
// Also note that the actual step value will be based on the percentage of thrust fired by the RCS jets, up to
// a maximum of this value.  
// XR1: value set to reach 40-degree AOA target shift of 0.23288 meter in two seconds assuming RCS jets are firing at 10% (0.23288 * 10 / 2).
const double COL_MAX_SHIFT_RATE = 1.1644 * 1.2;    // 20% faster than the XR1's rate   XR3TODO: tweak this when tuning autopilot performance

// absolute limit the autopilot or the user is allowed to shift the center-of-lift (+/-)
// OLD (too much): const double COL_MAX_SHIFT_DISTANCE = 15.675;
// OLD #2 (still too much): const double COL_MAX_SHIFT_DISTANCE = 9.275;
const double COL_MAX_SHIFT_DISTANCE = 4.115;    // matches XR1 & XR2 for now  XR3TODO: tune this

// the limit of the COG shift slider gauge; this is usually close to COL_MAX_SHIFT_DISTANCE
const double COL_SHIFT_GAUGE_LIMIT = 3.9;  // matches XR1 & XR2 for now  XR3TODO: tune this

// the fractional rate of COL_MAX_SHIFT_RATE that the COG shift keys move the COG; 0 < n < 1.0
const double COL_KEY_SHIFT_RATE_FRACTION = 0.10;    // matches XR1 & XR2 for now   XR3TODO: tune this

const char *SCRAMJET_WAV = "XR2ScramJet.wav";
const char *WELCOME_ABOARD_ALL_SYSTEMS_NOMINAL_WAV = "Welcome Aboard All Systems Nominal.wav";
const char *ALL_SYSTEMS_NOMINAL_WAV = "All Systems Nominal.wav";
const char *WARNING_OUTER_DOOR_IS_LOCKED_WAV = "Warning Docking Port is Closed Outer Door is Locked.wav";
const char *WARNING_NOSECONE_IS_CLOSED_WAV = "Warning Docking Port is Closed.wav";
const char *WARNING_NOSECONE_OPEN_WAV = "Warning Docking Port Deployed.wav";

// labels to handle nosecone and/or a docking port
const char *NOSECONE_LABEL = "Docking Port";
const char *NOSECONE_SHORT_LABEL = "Dock";  // used in the scenario file to show damage
const char *NOSECONE_SCN = "DOCKINGPORT";   // tag value in scenario files

const int MAX_MAINFUEL_ISP_CONFIG_OPTION = 7;  // upper limit for MainFuelISP in config file

//
// Autopilot constants
//

// attitude hold: pitch and bank deltas per mouse click or key press
const double AP_PITCH_DELTA_SMALL = 0.5;       // in degrees
const double AP_PITCH_DELTA_LARGE = 2.5;       // in degrees
const double AP_BANK_DELTA  = 5.0;             // in degrees

// Thruster level dead zone for positive (nose-up) RCS thrust; if the thrust exceeds this level
// a center-of-lift shift will be performed.  
// XR1 value: 0.04
// NOTE: keep this value very small to keep the ship stable!
const double AP_COL_DEAD_ZONE = 0.04;   // allow up to 4% RCS thrust before a COL shift is performed  (matches XR1 & XR2 for now)  XR3TODO: tune this

// Ratio of thruster level (0...1) to shift step strength (0..1).  For example, 1.0 means that at RCS thrust level 1.0 (100%) a full shift step will be used.
// Similarly, 2.0 means that at RCS thrust level 1.0/2=0.50 (50%) a full shift step will be used.
// A value other than 1.0 allows you to use larger step sizes for a given thrust level without increasing the step size and causing
// the COL shift to lose precision.  Higher values mean that a full step will be used sooner (i.e., at a lower RCS thrust level).
// XR1 value: 4.0
const double AP_COL_THRUSTLEVEL_TO_SHIFTSTEP_RATIO = 4.0;  // full step @ (1.0/4.0=0.25) 25% RCS thrust

// Multiplier for max RCS thrust while attitude hold active
// XR1 value: 5.0
const double AP_ATTITUDE_HOLD_RCS_THRUST_MULTIPLIER = 5.0;    

// elevator trim dead zone for center-of-lift (COL) shift, in meters.  If the current fabs(COL value) is > this value, an elevator trim step will be performed.
// XR1 ORG: extern const double AP_ELEVATOR_TRIM_COL_DEAD_ZONE = 1.0;
// TOO FAST: extern const double AP_ELEVATOR_TRIM_COL_DEAD_ZONE = 1.0 / 2;
// XR1 & XR2: 0.1
extern const double AP_ELEVATOR_TRIM_COL_DEAD_ZONE = 0.1;    // XR3TODO: tune this

// elevator trim fraction to move per second for manual movement
extern const double ELEVATOR_TRIM_SPEED = .20;  // matches other XR vessels

// elevator trim fraction to move per second for autopilot movement
// TOO SLOW: extern const double AP_ELEVATOR_TRIM_SPEED = .10;  // 1/2 XR1 value
extern const double AP_ELEVATOR_TRIM_SPEED = .20;    // matches XR2 for now   XR3TODO: tune this

// angular velocity degreesDelta fraction multiplier to reach target attitude in a reasonable time
// If this value is too large, the ship will roll too fast and "overshoot" the target at lower frame rates.
// If this value is too small, the ship will take too long to reach the target attitude, wasting RCS fuel in atmospheric flight.
// XR1 org value: 0.5, which means targetAngVel = degreesDelta * 0.5.  e.g., 5 degrees delta * 0.5 = 2.5 degrees-per-second target rate.
// ORG: extern const double AP_ANGULAR_VELOCITY_DEGREES_DELTA_FRAC = 0.5 / 4;  // slow down rotation rates for the Vanguard
extern const double AP_ANGULAR_VELOCITY_DEGREES_DELTA_FRAC = 0.5 / 2.5;  // XR2 was / 2.0   XR3TODO: tune this

//-------------------------------------------------------------------------
// XR3ConfigFileParser/XR1ConfigFileParser data
//-------------------------------------------------------------------------

// Table of LOX mass in kg at REALISTIC consumption level; one entry for each config option.
static const double LOX_MODIFIER = (MAX_PASSENGERS / 5.0);    // adjust LOX for the XR3 vs. XR1 crew
const double XR1ConfigFileParser::m_loxLoadoutArray[] = 
{
    65      * LOX_MODIFIER,  // 7 days
    130     * LOX_MODIFIER,  // 14 days
    283     * LOX_MODIFIER,  // one month
    848     * LOX_MODIFIER,  // three months
    1695    * LOX_MODIFIER,  // six months
    3389    * LOX_MODIFIER,  // one year
    6777    * LOX_MODIFIER,  // two years
    10165   * LOX_MODIFIER,  // three years
    13553   * LOX_MODIFIER,  // four years
    16942   * LOX_MODIFIER   // five years
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
const double XR1ConfigFileParser::m_mainFuelISPArray[] = 
{
    13943.1603938272,    // 0: Expert    (ISS Only w/expert use of SCRAM engines and expert deorbit/landing)
    20914.7405907408,    // 1: Realistic (ISS Only)
    25962.38443509765,   // 2: Default   (ISS and Moon)
    32981.19221754767,   // 3: Medium    (ISS and Moon w/reserve)
    40000.0,             // 4: Stock DG  (Moon w/large reserve; this is the original stock DG setting)
    52922.8282523788,    // 5: Big       (Mars)
    366251.528451608,    // 6: Huge      (Jupiter+)
    549377.292677412     // 7: Massive   (Jupiter+ w/full payload)  : this was (Huge + 50%), which is the additional mass of a full bay against a fully-fueled vessel (43%) + 7% margin.
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
    66.0,    // 0 = 66 kg/sec (easy)
    44.0     // 1 = 44 kg/sec (realistic)
};

// APU fuel burn rate in kg/minute
static const double APU_MODIFIER = 22.2;   // should match APU 'Fuel Mass XR3:XR1 Ratio' setting in the spreadsheet
const double XR1ConfigFileParser::m_apuFuelBurnRate[] = 
{
    0.0         * APU_MODIFIER,     // 0 = unlimited (runs indefinitely)
    0.90718474  * APU_MODIFIER,     // 1 = very low  (2 lb/minute)    (3.7 hours runtime)
    1.81436948  * APU_MODIFIER,     // 2 = low       (4 lb/minute)    (110 minutes runtime)
    2.72155422  * APU_MODIFIER,     // 3 = moderate  (6 lb/minute)    (74 hours runtime)
    4.08233134  * APU_MODIFIER,     // 4 = realistic (9 lb/minute)    (49 minutes runtime)
    6.12349701  * APU_MODIFIER,     // 5 = expert    (13.5 lb/minute) (33 minutes runtime)
};

// fuel/LOX dump particle stream coordinates; not used by the XR3
const VECTOR3 &FUEL_DUMP_PARTICLE_STREAM_POS1 = _V(0,0,0);
const VECTOR3 &FUEL_DUMP_PARTICLE_STREAM_DIR1 = _V(0,0,0);
const VECTOR3 &FUEL_DUMP_PARTICLE_STREAM_POS2 = _V(0,0,0);
const VECTOR3 &FUEL_DUMP_PARTICLE_STREAM_DIR2 = _V(0,0,0);

// boil-off exhaust particle stream coordinates; not used by the XR3
const VECTOR3 &BOIL_OFF_PARTICLE_STREAM_POS1 = _V(0,0,0);
const VECTOR3 &BOIL_OFF_PARTICLE_STREAM_DIR1 = _V(0,0,0);
const VECTOR3 &BOIL_OFF_PARTICLE_STREAM_POS2 = _V(0,0,0);
const VECTOR3 &BOIL_OFF_PARTICLE_STREAM_DIR2 = _V(0,0,0);

//
// XR3-specific globals
//

#ifdef UNUSED
const double HIGHSPEED_CENTER_OF_LIFT = -4.97;   
const double FLAPS_FULLY_RETRACTED_SPEED = 350;   // meters-per-second; just over Mach 1
const double FLAPS_FULLY_DEPLOYED_SPEED  = 90.0;  // meters-per-second
#endif

const double NOSE_GEAR_ZCOORD =  15.0;         // distance to center of nose gear strut from centerpoint
const double REAR_GEAR_ZCOORD = -3.9;          // distance to center of rear gear strut from centerpoint
const double GEAR_UNCOMPRESSED_YCOORD = -3.8;  // distance from centerpoint to bottom of tires, both front and rear

// These factors will multiply front and rear gear translation distance for gear compression
// ("altitude") by our "angled strut" factor.
// i.e., if the strut deploys to a non-vertical angle we have to deploy slightly more than we would at 90 degrees;
// This is because the hypotenuse is always longer than the altitude of a triangle.
// 1.0 = "strut is vertical"
// XR3TODO: set these if and when gear compression is added
const double FRONT_GEAR_COMPRESSION_TRANSLATION_FACTOR = 1.0;
const double REAR_GEAR_COMPRESSION_TRANSLATION_FACTOR = 1.0;

// tire diameter and circumference in meters
const double TIRE_DIAMETER = 2.546;     // XR3TODO: get this value from Loru
const double FRONT_TIRE_CIRCUMFERENCE = TIRE_DIAMETER * PI;
const double REAR_TIRE_CIRCUMFERENCE = TIRE_DIAMETER * PI;

// Deceleration rate for wheel rotation to slow to a stop due to drag.
// Value is in meters per second @ the tire's outer edge.
// ORG (stops too fast): const double TIRE_DECELERATION_RATE = 7.6423;
const double TIRE_DECELERATION_RATE = 3.5;

// size of a single standard payload grid in meters: width (X), height (Y), length (Z)
// This must be defined BEFORE it is used below.
// This is only used for "slots occupied" display purposes, EXCEPT for the Y dimension, which must match for all slots.
// For the XR2 we use *slot 2 and 3* dimensions since they are smaller than slot 1 and must be considered "standard."
// Note: each slot's Y dimension matches the actual XR2 payload dimension in that slot so that the payloads will set on the floor of the bay.
//       The actual size of each slot in the bay is actually slightly taller (Y) and wider (X).
const VECTOR3 &PAYLOAD_SLOT1_DIMENSIONS = _V(3.452, 2.418, 2.060);  // y is to CENTER (highest point)  // XR3TODO: add payload bay code for PAYLOAD_SLOT1_DIMENSIONS like the XR2 does
const VECTOR3 &PAYLOAD_SLOT_DIMENSIONS = _V(3.452, 2.128, 1.454);  // Y is to CENTER (highest point)

// Ship-local delta in meters along the Y axis to the ground while the ship is landed, leaving 1/5th-meter as a safety margin
// to prevent the "bounce bug" if the altitude is too low.  In addition, this will show the container being "pulled down" by gravity for
// a tiny distance when it is deployed while landed, which looks cool.
//     Distance to ground + 1/2 payload slot height + 0.2 safety margin.
const double PAYLOAD_BAY_DELTAY_TO_GROUND = (GEAR_UNCOMPRESSED_YCOORD + GEAR_COMPRESSION_DISTANCE) + (PAYLOAD_SLOT_DIMENSIONS.y / 2) + 0.20;  // parens are for clarity only

// Payload bay delta in meters along the X axis to the center of the deployed payload grid while the
// ship is landed: this is 1/2 the bay width PLUS 1/2 the ship's width PLUS 5 meters of clearance on each side.
const double PAYLOAD_BAY_DELTAX_TO_GROUND = (7.0 / 2) + (29.49 / 2) + 5.0;

// default payload thumbnail path, relative to the Config directory
const char *DEFAULT_PAYLOAD_THUMBNAIL_PATH = "Vessels\\Altea_Default_Payload_Thumbnail.bmp";

// grapple display ranges in meters
const double GRAPPLE_DISPLAY_RANGES[] = { 50, 100, 250, 500, 1e3, 1e4, 1e5 };

// # of grapple display ranges
const int GRAPPLE_DISPLAY_RANGE_COUNT = sizeof(GRAPPLE_DISPLAY_RANGES) / sizeof(double);

// default grapple range index if not set in scenario file
const int DEFAULT_GRAPPLE_RANGE_INDEX = 4;

// # of payload slots
// NOTE: if you change this, you must also update the code in XR3PayloadBay to match!
// The XR3 has 7 slots (6 normal slots + 1 double-wide CHM in front) compared to XR3's three slots.
#define PAYLOAD_BAY_SLOT_COUNT_CONSTANT 7    /* we need a constant value for an array size later in this file; this typedef is not used outside this file */
const double PAYLOAD_BAY_SLOT_COUNT = PAYLOAD_BAY_SLOT_COUNT_CONSTANT;

// *Deployed* docking port coordinates
const VECTOR3 &DOCKING_PORT_COORD = _V(0, 3.060, 8.60); 

// welcome messages
const char *WELCOME_MSG = "Welcome aboard, Commander!&All systems nominal.";
const char *ALL_SYSTEMS_NOMINAL_MSG = "All systems nominal.";

// callout globals
// takeoff callouts in meters-per-second
// XR3TODO: tune these three airspeed callout thresholds; they match the XR2's values for now
const double V1_CALLOUT_AIRSPEED = 105;  
const double ROTATE_CALLOUT_AIRSPEED_EMPTY = 130;  // no payload  
const double ROTATE_CALLOUT_AIRSPEED_HEAVY = 150;  // max payload
const double MAX_RECOMMENDED_PAYLOAD_MASS = 28763; // used for Vr callout calculation only

#ifdef TURBOPACKS
// turbopack data
const Turbopack TURBOPACKS_ARRAY[] =   // XR3TODO: do we have non-UMMu turbopacks for XR3 crew?  Other XR vessels did, but I don't remember where they came from.
{
    {
        "Standard Turbopack",
        "UMmuturbopack"
    }
};

// number of turbopacks in TURBOPACKS_ARRAY
const int TURBOPACKS_ARRAY_SIZE = sizeof(TURBOPACKS_ARRAY) / sizeof(Turbopack);

// vessel-relative coordinates where turbopacks spawn during deployment
const VECTOR3 &TURBOPACK_SPAWN_COORDINATES = _V(0, 0, 15.0);    // front of the XR3 is at 18 meters   XR3TODO: tweak this to be close to where the crew ladder is deployed

// maximum distance in meters of turbopacks that will be auto-stowed
const double STOW_TURBOPACK_DISTANCE = 20;
#endif

////////////////
// CHEATCODES //
////////////////

// cargo mass (may change as cargo is loaded/unloaded)
double CARGO_MASS = -1.0;   // -1.0 = "not set"

// the maximum range that a payload module may be grappled in orbit
double PAYLOAD_GRAPPLE_RANGE_ORBIT = 22.0;

// the maximum range that a payload module may be grappled while landed
double PAYLOAD_GRAPPLE_RANGE_LANDED = 400.0;

// the maximum deltaV at which a payload module may be grappled, in m/s
double PAYLOAD_GRAPPLE_MAX_DELTAV = 0.5;

////////////////////////////////
// payload dialog static data //
////////////////////////////////
// array of button resource IDs in slot order (1-7)
const int XR1PayloadDialog::slotCount = PAYLOAD_BAY_SLOT_COUNT_CONSTANT;
const int XR1PayloadDialog::slotResourceIDs[PAYLOAD_BAY_SLOT_COUNT_CONSTANT] =
    { 
        IDC_SLOT1,  IDC_SLOT2,  IDC_SLOT3,  IDC_SLOT4,  IDC_SLOT5,  IDC_SLOT6,  IDC_SLOT7  // XR3TODO: remove extra slot buttons and IDC_SLOTn definitions from resource.h
    };
        
const int GLOBAL_IDD_PAYLOAD_EDITOR = IDD_EDITOR_PG2;  // from resource.h

// resource ID globals used by common areas
const int RES_IDB_FUEL_GAUGE = IDB_FUEL_GAUGE;
const int RES_IDB_FUEL_GAUGE_DARK = IDB_FUEL_GAUGE_DARK;

const int RES_IDB_LOX_GAUGE = IDB_LOX_GAUGE;
const int RES_IDB_LOX_GAUGE_DARK = IDB_LOX_GAUGE_DARK;

const int RES_IDB_COOLANT_GAUGE = IDB_COOLANT_GAUGE;

// number of spotlights defined
const int SPOTLIGHT_COUNT = 4;

// values are in meters
const double HEIGHT_WHEN_LANDED = 10.57;
const double HULL_LENGTH = 36.75;
const double HULL_WIDTH = 29.49;

const VECTOR3 HULL_TOUCHDOWN_POINTS[] =
{
	// TODO: fill these in
	0
};
// TODO: const int HULL_TOUCHDOWN_POINTS_COUNT = sizeof(HULL_TOUCHDOWN_POINTS) / sizeof(VECTOR3);
const int HULL_TOUCHDOWN_POINTS_COUNT = 3; // TEMP until real touchdown points are defined

