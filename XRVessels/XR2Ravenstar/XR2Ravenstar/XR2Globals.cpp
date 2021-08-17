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
// XR2Ravenstar implementation class
//
// XR2Globals.cpp
// Contains global variable values for the XR2 Ravenstar.
// ==============================================================

#include "XR2Globals.h"  // verify that types agree
#include "XR2ConfigFileParser.h"
#include "meshres.h"
#include "XR1PayloadDialog.h"

//
// Version globals
//
const char *VESSELNAME = "XR2Ravenstar";

// VERSION ID
// {XXX} UPDATE THIS FOR THE CURRENT BUILD VERSION; DO NOT REMOVE THIS {XXX} COMMENT
const char* VERSION = "Version 2.0 Beta-1 [" ARCH_TYPE  " " BUILD_TYPE "], Build Date : " __DATE__;

// file is always written to the Orbiter directory
const char *XR_LOG_FILE = "XR2Ravenstar.log";

const char *XR_CONFIG_FILE = "Config\\XR2RavenstarPrefs.cfg";

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
    "CTRL-U",       "Payload Bay Doors",
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
    "ALT-/",        "AF CTRL Mode",
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

double EMPTY_MASS = 16080;  

double TANK1_CAPACITY = 13396.0;    // Main
double TANK2_CAPACITY =  3350.0;    // SCRAM
// Main fuel tank capacities [kg] (can be split between rocket
// fuel and scramjet fuel)

double RCS_FUEL_CAPACITY = 804.0;
// Max fuel capacity: RCS tank [kg]

double APU_FUEL_CAPACITY = 268.0;
// Max APU fuel capacity [kg]

double MAX_MAIN_THRUST[2] = {377.8e3, 302.3e3};
// Main engine max vacuum thrust [N] per engine. (x2 for total)
// NOTE: don't need to track main engine damage here since thrust is not set by gimbaling

double MAX_RETRO_THRUST = 64.24e3;
// Retro engine max vacuum thrust [N] per engine. (x2 for total)

double MAX_HOVER_THRUST[2] = {285.94e3, 207.83e3};
// Hover engine max vacuum thrust [N] (x2 for total)

double MAX_RCS_THRUST = 3.93e3;

double MAX_WHEELBRAKE_FORCE = 134.0e3;

// {DEB} Hydrogen FHV = 1.42e8 J/kg.
// This is the value to set if you want to change the SCRAM engines' ISP.
// Note: if you update this, update the pref file docs, too.
double SCRAM_FHV[2] = {4.2e8, 2.4e8};
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
// XR1 ORG: -0.341136
// TODO: tweak this if necessary
const double HIDDEN_ELEVATOR_TRIM_STATE = -0.341136;

//-------------------------------------------------------------------------

// XR1 mass fully loaded; only used for landing gear limit calculations
// NOTE: we assume a LOX mass here of a 14-day loadout @ 'realistic': 25.6 kg per passenger
// NOTE: max landing mass does NOT change with cheatcodes!
// we use an "average" passenger mass of 68 kg for landing limit calculations
const double FULLY_LOADED_MASS = (EMPTY_MASS + (68 * MAX_PASSENGERS) + TANK1_CAPACITY + TANK2_CAPACITY + RCS_FUEL_CAPACITY + APU_FUEL_CAPACITY + (25.6 * MAX_PASSENGERS));

// rate at which main fuel is dumped and loaded in kg/sec
const double FUEL_DUMP_RATE = 85 * 1.34;   // x XR1 modifier
const double FUEL_LOAD_RATE = 72 * 1.34;   // reloads slower than is dumped

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
const double WING_AREA = 125.6;      // Original DG was 90

// wing aspect ratio, which may be computed as: span^2 / wingArea

const double WING_ASPECT_RATIO = 18.95 * 18.95 / WING_AREA;  // accurate for the XR2

// wing efficiency factor
// The efficiency factor depends on the wing shape. The most efficient wings
// are elliptical, with e = 1. For all other shapes, e < 1.
const double WING_EFFICIENCY_FACTOR = 0.81;  // matches XR5's setting

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

const double RADIATOR_OPERATING_SPEED = 0.0625;
// Deployment speed of radiator (1/sec)
// XR1: 32 sec
// => cycle = 16 sec

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
const double MAX_COOLANT_TEMP = 117 + oapiRand();   // never exceeds this, although the crew would be dead by this time
const double MAX_COOLANT_GAUGE_TEMP = 110.0;
const double MIN_COOLANT_GAUGE_TEMP = 10.0;

// cooling rate fraction per second for ~67 minutes to cool to nominal @ realistic levels using only the radiator
const double COOLANT_COOLING_RATE_FRAC = 4.9751544513792169407956770249373e-4;
const double COOLANT_COOLING_RATE_MIN = 0.015;  // degrees C per second

// altitude adjustment when gear is down
// OLD: const double GEAR_FULLY_UNCOMPRESSED_DISTANCE = 2.128;  // no compression for now
// BETA-1a ORG: NOTE: this was slightly too high! : const double GEAR_FULLY_UNCOMPRESSED_DISTANCE = 2.628;  // gear is extended 0.5 meter now, but still no compression
// NO COMPRESSION UNTIL THE Mk II: const double GEAR_FULLY_UNCOMPRESSED_DISTANCE = 2.70;    // adjusted +0.15 for additional compression; the rest is a correction to the previous value
const double GEAR_FULLY_UNCOMPRESSED_DISTANCE = 2.60;  // no compression for now

// distance from centerpoint to bottom of tires, both front and rear
// OLD: const double GEAR_UNCOMPRESSED_YCOORD = -2.635;  
const double GEAR_UNCOMPRESSED_YCOORD = -GEAR_FULLY_UNCOMPRESSED_DISTANCE;

// distance the meters that the gear can travel during compression
// OLD: const double GEAR_COMPRESSION_DISTANCE = 0.40;
const double GEAR_COMPRESSION_DISTANCE = 0;  // no compression for the Mk I

// altitude at which gear is fully compressed and the hover engines may shut down from auto-descent.
const double GEAR_FULLY_COMPRESSED_DISTANCE = GEAR_FULLY_UNCOMPRESSED_DISTANCE - GEAR_COMPRESSION_DISTANCE;

// distance to center of nose wheel axle from centerpoint when the gear is *down*; used for compression and touchdown points
const double NOSE_GEAR_ZCOORD = 6.431;          

// distance to center of rear wheel axle from centerpoint when the gear is *down* and at full compression; used for compression and touchdown points
// NOTE: this must take final gear compression into account if the gear struts are angled.
// Since the XR2's struts are angled AFT, the touchdown point is moved FORWARD slightly.
// The best way to get this right is to rotate to +15 degrees pitch at Brighton Beach while stationary 
// and see if the rear wheels look right as the ship's nose comes up: the rear tires should stay on the ground.
const double REAR_GEAR_ZCOORD = -3.144;

// These factors will multiply front and rear gear translation distance for gear compression
// ("altitude") by our "angled strut" factor.
// i.e., if the strut deploys to a non-vertical angle we have to deploy slightly more than we would at 90 degrees;
// This is because the hypotenuse is always longer than the altitude of a triangle.
// 1.0 = "strut is vertical"
// For example: 11.2 degrees = 90-11.2 = 78.8 degrees angle A.  Hypotenuse is 1.019 X as long as the altitude: 
//      2.0 - sin (90-11.2) = 2.0 - 0.981 = 1.019
//      2.0 - sin (90-0)    = 2.0 - 1.0 = 1.0  (no angle)   
const double frontStrutAngle = 0 * RAD;      // 0 degrees deflection from vertical
const double FRONT_GEAR_COMPRESSION_TRANSLATION_FACTOR = 2.0 - sin((90.0 * RAD) - frontStrutAngle);
const double rearStrutAngle = 11.2 * RAD;   // XR2's rear struts angle 11.2 degrees from vertical
const double REAR_GEAR_COMPRESSION_TRANSLATION_FACTOR  = 2.0 - sin((90.0 * RAD) - rearStrutAngle);

// multipler for wing balance shift when a crash occurs
// XR1 value = 3.0
const double CRASH_WING_BALANCE_MULTIPLIER = 1.0;  // TODO: tweak this

// Mach number checked at reentry; if SCRAM doors open and internal temps are > ambient and mach 
// number is >= this value AND SCRAM throttle is zero, a "scram doors open" warning callout will sound.
const double MACH_REENTRY_WARNING_THRESHOLD = 22.0;  // TODO: tweak this: assumes mach 20 for SCRAM engines

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
const double SCRAM_INTERNAL_TEMAX = 20500;
// ORG: const double SCRAM_INTERNAL_TEMAX[2] = {3500.0, 3200.0};
// Max. scramjet exhaust temperature [K]

// new for XR1; used for display purposes only
const double SCRAM_COOLING = 2.5625;  // divisor: effective diffuser temps after new design's cooling

// max SCRAM diffuser temperature AFTER active cooling
// this is used for display purposes
const double MAX_SCRAM_TEMPERATURE = (SCRAM_INTERNAL_TEMAX / SCRAM_COOLING);  // degrees K

// SCRAM pressure recovery value; tune this to change the operating envelope of the SCRAM engines
const double SCRAM_PRESSURE_RECOVERY_MULT = 0.765;  // good for Mach 20 now

// SCRAM DMA scale value; tune this to change the operating envelope of the SCRAM engines
const double SCRAM_DMA_SCALE = 1.1475e-4; // good for Mach 20

const double SCRAM_INTAKE_AREA = 1.0 * 1.34;  // x XR2 multiplier over XR1: 1.34 would match main engines
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
const double SCRAM_FLOW_GAUGE_MAX = 9.0;    // NOTE: this should match the "Easy" value in m_scramMaxDMF
const double SCRAM_TSFC_GAUGE_MAX = 0.03;   // TSFC is unchanged

// Main/Hover fuel flow limits
const double MAIN_FLOW_GAUGE_MAX = 19.5 * 1.34;   // 26.1
const double HOVER_FLOW_GAUGE_MAX = 13.8 * 1.34;  // 18.49

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
const int MAX_PASSENGERS = 14;

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
const DamageItem D_END = DamageItem::BayDoors;

// Max. allowed dynamic pressure [Pa]
const double DYNP_MAX = 150e3;  // 150 kPa

// landing gear momentum limit
// limit is in kg-m/s units; original fully-loaded XR2 (excluding payload) could land at 3.7 m/s^2 descent max.
// XR1: 3.0 m/s^2
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
const double CREW_IMPACT_MINOR_INJURY_THRESHOLD = 2.7;

// multiplier used to add heat during reentry; this is multiplied by speed*pressure
// XR1 value: 3.1034e-10
const double HULL_HEATING_FACTOR = 3.1034e-10 * 1.40;

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
// XR1 value: 0.05
const double COL_KEY_SHIFT_RATE_FRACTION = 0.10;

// sound file customization
const char *SCRAMJET_WAV = "XR2ScramJet.wav";
const char *WELCOME_ABOARD_ALL_SYSTEMS_NOMINAL_WAV = "Welcome Aboard All Systems Nominal.wav";
const char *ALL_SYSTEMS_NOMINAL_WAV = "All Systems Nominal.wav";
const char *WARNING_OUTER_DOOR_IS_LOCKED_WAV = "Warning Nosecone is Closed Outer Door is Locked.wav";
const char *WARNING_NOSECONE_IS_CLOSED_WAV = "Warning Nosecone is Closed.wav";
const char *WARNING_NOSECONE_OPEN_WAV = "Warning Nosecone Open.wav";

// labels to handle nosecone and/or a docking port
const char *NOSECONE_LABEL = "Nosecone";
const char *NOSECONE_SHORT_LABEL = "Nose";  // used in the scenario file to show damage
const char *NOSECONE_SCN = "NOSECONE";      // tag value in scenario files

const int MAX_MAINFUEL_ISP_CONFIG_OPTION = 7;  // upper limit for MainFuelISP in config file

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

// elevator trim dead zone for COL shift, in meters.  If the current fabs(COL value) is > this value, an elevator trim step will be performed.
extern const double AP_ELEVATOR_TRIM_COL_DEAD_ZONE = 0.1;

// elevator trim fraction to move per second for manual movement
extern const double ELEVATOR_TRIM_SPEED = 0.20;

// elevator trim fraction to move per second for autopilot movement
// XR1: 0.20
extern const double AP_ELEVATOR_TRIM_SPEED = 0.20;  // tweaked for atm flight

// angular velocity degreesDelta fraction multiplier to reach target attitude in a reasonable time
// If this value is too large, the ship will roll too fast and "overshoot" the target at lower frame rates.
// If this value is too small, the ship will take too long to reach the target attitude, wasting RCS fuel in atmospheric flight.
// XR1 org: 0.5
extern const double AP_ANGULAR_VELOCITY_DEGREES_DELTA_FRAC = 0.5 / 2;

//-------------------------------------------------------------------------
// XR1ConfigFileParser data
//-------------------------------------------------------------------------

// Table of LOX mass in kg at REALISTIC consumption level; one entry for each config option.
static const double LOX_MODIFIER = (MAX_PASSENGERS / 5.0);    // adjust LOX for the XR5 vs. XR1 crew
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
// NOTE: DG default was 40000
const double XR1ConfigFileParser::m_mainFuelISPArray[] = 
{
    13943.1603938272,    // 0: Expert    (ISS Only w/expert use of SCRAM engines and expert deorbit/landing)
    20914.7405907408,    // 1: Realistic (ISS Only)
    25962.38443509765,   // 2: Default   (ISS and Moon)
    32981.19221754767,   // 3: Medium    (ISS and Moon w/reserve)
    40000.0,             // 4: Stock DG  (Moon w/large reserve; this is the original stock DG setting)
    52922.8282523788,    // 5: Big       (Mars)
    366251.528451608,    // 6: Huge      (Jupiter+)
    476126.9869870904    // 7: Massive   (Jupiter+ w/full payload)  : this was (Huge + 30%), which is the additional mass of a full bay against a fully-fueled vessel (23%) + 7% margin.
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
    9.0,    // 0 = 9.0 kg/sec (easy)
    6.0     // 1 = 6.0 kg/sec (realistic)
};

// APU fuel burn rate in kg/minute
static const double APU_MODIFIER = 1.34;   // should match APU 'Fuel Mass XR2:XR1 Ratio' setting in the spreadsheet
const double XR1ConfigFileParser::m_apuFuelBurnRate[] = 
{
    0.0         * APU_MODIFIER,     // 0 = unlimited (runs indefinitely)
    0.90718474  * APU_MODIFIER,     // 1 = very low  (2 lb/minute)    (3.7 hours runtime)
    1.81436948  * APU_MODIFIER,     // 2 = low       (4 lb/minute)    (110 minutes runtime)
    2.72155422  * APU_MODIFIER,     // 3 = moderate  (6 lb/minute)    (74 hours runtime)
    4.08233134  * APU_MODIFIER,     // 4 = realistic (9 lb/minute)    (49 minutes runtime)
    6.12349701  * APU_MODIFIER,     // 5 = expert    (13.5 lb/minute) (33 minutes runtime)
};

// docking port coordinates at the FRONT Z EDGE of the port in the center
const VECTOR3 &DOCKING_PORT_COORD = _V(0, 0.253, 10.55);

// welcome messages
const char *WELCOME_MSG = "Welcome aboard, Commander!&All systems nominal.";
const char *ALL_SYSTEMS_NOMINAL_MSG = "All systems nominal.";

// callout globals
// takeoff callouts in meters-per-second
// Note: these are calibrated for the DEFAULT (fixed) elevator settings
const double V1_CALLOUT_AIRSPEED = 105;
const double ROTATE_CALLOUT_AIRSPEED_EMPTY = 130;  // no payload
const double ROTATE_CALLOUT_AIRSPEED_HEAVY = 150;  // max payload
const double MAX_RECOMMENDED_PAYLOAD_MASS = 10795; // used for Vr callout calculation only

#ifdef TURBOPACKS
// turbopack data
const Turbopack TURBOPACKS_ARRAY[] =
{
    {
        "XR2 Turbopack (Kara)",
        "XR2turbopackKara"
    },
    {
        "XR2 Turbopack (Lee)",
        "XR2turbopackLee"
    },
    {
        "Standard Turbopack",
        "UMmuturbopack"
    }
};

// number of turbopacks in TURBOPACKS_ARRAY
const int TURBOPACKS_ARRAY_SIZE = sizeof(TURBOPACKS_ARRAY) / sizeof(Turbopack);

// vessel-relative coordinates where turbopacks spawn during deployment
const VECTOR3 &TURBOPACK_SPAWN_COORDINATES = _V(0, 0, 16.0);

// maximum distance in meters of turbopacks that will be auto-stowed
const double STOW_TURBOPACK_DISTANCE = 25;
#endif

// Panel ID of the first virtual cockpit panel (just beyond the last 2D panel).  2D panels start at ID 0.
const int VC_PANEL_ID_BASE = 4;

//
// New for XR2
//

// fuel/LOX dump particle stream coordinates
const double dumpZOffset = -0.10;   // Z axis offset for start of particle streams
const VECTOR3 &FUEL_DUMP_PARTICLE_STREAM_POS1 = _V(-4.072, 1.424, -9.969 + dumpZOffset);  // port side
const VECTOR3 &FUEL_DUMP_PARTICLE_STREAM_DIR1 = _V(0,0,-1.0);
const VECTOR3 &FUEL_DUMP_PARTICLE_STREAM_POS2 = _V( 4.072, 1.424, -9.969 + dumpZOffset);   // starboard side
const VECTOR3 &FUEL_DUMP_PARTICLE_STREAM_DIR2 = _V(0,0,-1.0);

// boil-off exhaust particle stream coordinates; not used by the XR1
const double boilYOffset = -0.10;   // Y axis offset for start of particle streams
const VECTOR3 &BOIL_OFF_PARTICLE_STREAM_POS1 = _V(-2.853, boilYOffset, -7.423);  // port
const VECTOR3 &BOIL_OFF_PARTICLE_STREAM_DIR1 = _V(0, -1, 0);
const VECTOR3 &BOIL_OFF_PARTICLE_STREAM_POS2 = _V( 2.853, boilYOffset, -7.423);  // starboard
const VECTOR3 &BOIL_OFF_PARTICLE_STREAM_DIR2 = _V(0, -1, 0);

// VC mesh groups for the HUD
const DWORD PILOT_HUD_MESHGRP = GRP_hudglass_pilot;
const DWORD COPILOT_HUD_MESHGRP = GRP_hudglass_copilot;

const double BAY_OPERATING_SPEED = 0.04545;    // 22 seconds
const double BAY_LIMIT           = 36e3;

// size of a single standard payload grid in meters: width (X), height (Y), length (Z)
// This must be defined BEFORE it is used below.
// This is only used for "slots occupied" display purposes, EXCEPT for the Y dimension, which must match for all slots.
// For the XR2 we use *slot 2 and 3* dimensions since they are smaller than slot 1 and must be considered "standard."
// Note: each slot's Y dimension matches the actual XR2 payload dimension in that slot so that the payloads will set on the floor of the bay.
//       The actual size of each slot in the bay is actually slightly taller (Y) and wider (X).
const VECTOR3 &PAYLOAD_SLOT1_DIMENSIONS = _V(3.452, 2.418, 2.060);  // y is to CENTER (highest point)
const VECTOR3 &PAYLOAD_SLOT_DIMENSIONS  = _V(3.452, 2.128, 1.454);  // Y is to CENTER (highest point)

// front and rear tire circumference; only used for wheel rotation animation
const double FRONT_TIRE_CIRCUMFERENCE = 0.717 * PI;
const double REAR_TIRE_CIRCUMFERENCE  = 1.128 * PI;

// Deceleration rate for wheel rotation to slow to a stop due to drag.
// Value is in meters per second @ the tire's outer edge.
// XR5 ORG: 7.6423
// STOPS TOO FAST: const double TIRE_DECELERATION_RATE = 7.6423;
const double TIRE_DECELERATION_RATE = 3.5;

// Ship-local delta in meters along the Y axis to the ground while the ship is landed, leaving 1/5th-meter as a safety margin
// to prevent the "bounce bug" if the altitude is too low.  In addition, this will show the container being "pulled down" by gravity for
// a tiny distance when it is deployed while landed, which looks cool.
//     Distance to ground + 1/2 payload slot height + 0.4 safety margin.
const double PAYLOAD_BAY_DELTAY_TO_GROUND = (GEAR_UNCOMPRESSED_YCOORD + GEAR_COMPRESSION_DISTANCE) + (PAYLOAD_SLOT_DIMENSIONS.y / 2) + 0.40;  // parens are for clarity only

// Payload bay delta in meters along the X axis to the center of the deployed payload grid while the
// ship is landed: this is 1/2 the bay width PLUS 1/2 the ship's width PLUS 3 meters of clearance on each side.
const double PAYLOAD_BAY_DELTAX_TO_GROUND = (3.452 / 2) + (18.95 / 2) + 3.0;

// default payload thumbnail path, relative to the Config directory
const char *DEFAULT_PAYLOAD_THUMBNAIL_PATH = "Vessels\\Altea_Default_Payload_Thumbnail.bmp";

// grapple display ranges in meters
const double GRAPPLE_DISPLAY_RANGES[] = { 50, 100, 250, 500, 1e3, 1e4, 1e5 };

// # of grapple display ranges
const int GRAPPLE_DISPLAY_RANGE_COUNT = sizeof(GRAPPLE_DISPLAY_RANGES) / sizeof(double);

// default grapple range index if not set in scenario file
const int DEFAULT_GRAPPLE_RANGE_INDEX = 4;

// # of payload slots
// NOTE: if you change this, you must also update the code in XR2PayloadBay to match!
#define PAYLOAD_BAY_SLOT_COUNT_CONSTANT 3    /* we need a constant value for an array size later in this file; this typedef is not used outside this file */
const double PAYLOAD_BAY_SLOT_COUNT = PAYLOAD_BAY_SLOT_COUNT_CONSTANT;

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
// array of button resource IDs in slot order (1-3)
const int XR1PayloadDialog::slotCount = PAYLOAD_BAY_SLOT_COUNT_CONSTANT;
const int XR1PayloadDialog::slotResourceIDs[PAYLOAD_BAY_SLOT_COUNT_CONSTANT] = 
    { 
        IDC_SLOT1,  IDC_SLOT2,  IDC_SLOT3
    };
const int GLOBAL_IDD_PAYLOAD_EDITOR = IDD_EDITOR_PG2;  // from resource.h

// resource ID globals used by common areas
const int RES_IDB_FUEL_GAUGE = IDB_FUEL_GAUGE;
const int RES_IDB_FUEL_GAUGE_DARK = IDB_FUEL_GAUGE_DARK;

const int RES_IDB_LOX_GAUGE = IDB_LOX_GAUGE;
const int RES_IDB_LOX_GAUGE_DARK = IDB_LOX_GAUGE_DARK;

const int RES_IDB_COOLANT_GAUGE = IDB_COOLANT_GAUGE;

// number of spotlights defined
const int SPOTLIGHT_COUNT = 2;

// values are in meters
const double HEIGHT_WHEN_LANDED = 8.60;
const double HULL_LENGTH = 23.91;
const double HULL_WIDTH = 18.95;

const VECTOR3 HULL_TOUCHDOWN_POINTS[] =
{
	_V(-9.421, 0.522, 10.026),		// wingtips
	_V( 9.421, 0.522, 10.026),
	_V(-5.196, 1.463, 0),			// wing midpoints
	_V( 5.196, 1.463, 0),
	_V(0, 2.309, 9.438),			// nose
	_V(0, 2.103, 0),				// top of canopy
	_V(-3.02, 4.936, -10.026),		// top of vertical stabilizers
	_V( 3.02, 4.936, -10.026),
	_V(-4.494, 0.479, -10.704),		// engine aft points
	_V( 4.494, 0.479, -10.704)
};
const int HULL_TOUCHDOWN_POINTS_COUNT = sizeof(HULL_TOUCHDOWN_POINTS) / sizeof(VECTOR3);
