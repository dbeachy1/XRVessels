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
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// AreaID.h
// Defines Orbiter area IDs for the DG-XR1
// ==============================================================

#pragma once

//
// Panel area identifiers
//
// NOTE: the framework expects each area to have a globally unique ID!
//


//
// Panel 0  (main)
//
#define AID_ENGINEMAIN           2
#define AID_ENGINEHOVER          3
#define AID_ENGINESCRAM          4

#define AID_RCSMODE              5  // off, rot, lin
#define AID_AFCTRLMODE           6  // off, pitch, on
#define AID_AUTOPILOTBUTTONS     7  // kill rot, prograde, etc.
#define AID_HUDMODE              8
#define AID_ALTEA_LOGO           9  /* shared across several panels */
#define AID_LOADINSTR           11

// MFD area IDs must be sequential for each type
#define AID_MFD1_BBUTTONS       14
#define AID_MFD2_BBUTTONS       15

#define AID_MFD1_LBUTTONS       16
#define AID_MFD2_LBUTTONS       17

#define AID_MFD1_RBUTTONS       18
#define AID_MFD2_RBUTTONS       19

#define AID_ELEVATORTRIM        20
#define AID_MAINFLOW            32
#define AID_HOVERFLOW           34   
#define AID_MAINTSFC            35
#define AID_SCRAMFLOW           36
#define AID_SCRAMTSFC           37

// TEXT: fuel remaining in KG
#define AID_MAINPROPMASS_KG     39
#define AID_RCSPROPMASS_KG      41
#define AID_SCRAMPROPMASS_KG    43

// TEXT: fuel remaining in %
#define AID_MAINPROPMASS_PCT    44
#define AID_RCSPROPMASS_PCT     45
#define AID_SCRAMPROPMASS_PCT   46

// BAR: fuel remaining in %
#define AID_MAINPROPMASS_BAR    47
#define AID_RCSPROPMASS_BAR     48
#define AID_SCRAMPROPMASS_BAR   49

// TEXT: thrust generated in kN
#define AID_THRUSTMAIN_KN       51
#define AID_THRUSTHOVER_KN      52
#define AID_THRUSTSCRAM_KN      53

// INDICATOR2: engine efficiency
#define AID_ENGINE_EFFICIENCY   54

// BAR: engine throttles
#define AID_THROTTLEBAR_MAINL   55
#define AID_THROTTLEBAR_MAINR   56
#define AID_THROTTLEBAR_HOVER   57
#define AID_THROTTLEBAR_SCRAML  58
#define AID_THROTTLEBAR_SCRAMR  59

#define AID_MWS                 60

// TEXT: acc
#define AID_ACCX_NUMBER         61
#define AID_ACCY_NUMBER         62
#define AID_ACCZ_NUMBER         63

// INDICATOR2: acc in Gs
#define AID_ACCX_G              64
#define AID_ACCY_G              65
#define AID_ACCZ_G              66

// Dynamic pressure panel
#define AID_DYNPRESSURE_KPA     67
#define AID_DYNPRESSURE_GAUGE   68

// acc scale on engine panel
#define AID_ACC_SCALE           69

// SCRAM temp panel
#define AID_SCRAMTEMP_LBAR      70      // left engine bar
#define AID_SCRAMTEMP_RBAR      71      // right engine bar
#define AID_SCRAMTEMP_LTEXT     72      // left engine text
#define AID_SCRAMTEMP_RTEXT     73      // right engine text

#define AID_STATIC_PRESSURE     74

// Slope panel
#define AID_SLOPE_DEGREES       75
#define AID_SLOPE_GAUGE         76

#define AID_MULTI_DISPLAY       77

#define AID_HUDCOLOR                    78
#define AID_HUDINTENSITY                79
#define AID_SECONDARY_HUD_BUTTONS       80
#define AID_SECONDARY_HUD               81
#define AID_TERTIARY_HUD_BUTTON         82
#define AID_TERTIARY_HUD                83

// AOA panel
#define AID_AOA_DEGREES                 84
#define AID_AOA_GAUGE                   85

#define AID_MWS_TEST_BUTTON             86
#define AID_WARNING_LIGHTS              87

// Slip panel
#define AID_SLIP_GAUGE                  88
#define AID_SLIP_TEXT                   89

// APU panel
#define AID_APU_FUEL_TEXT               90
#define AID_APU_FUEL_GAUGE              91
#define AID_APU_BUTTON                  92

#define AID_DEPLOY_RADIATOR_BUTTON      93
#define AID_DATA_HUD_BUTTON             94

// Center Of Gravity panel
#define AID_COG_NUMBER                  95
#define AID_COG_GAUGE                   96
#define AID_COG_ROCKER_SWITCH           97
#define AID_COG_AUTO_LED                98
#define AID_COG_CENTER_BUTTON           99

// WARNING: be careful adding IDs here: subclasses (i.e., the Vanguard) are using 2100, for example

//
// Panel 1  (upper)
//
#define AID_OUTERDOORSWITCH             100
#define AID_INNERDOORSWITCH             101
#define AID_NAVLIGHTSWITCH              102
#define AID_BEACONSWITCH                103
#define AID_STROBESWITCH                104
#define AID_VPITCH                      105
#define AID_VBANK                       106
#define AID_VYAW                        107
#define AID_APITCH                      108
#define AID_ABANK                       109
#define AID_AYAW                        110
#define AID_MPITCH                      111
#define AID_MBANK                       112
#define AID_MYAW                        113
#define AID_SCRAMTEMPDISP               114
#define AID_NOSECONESWITCH              115  // formerly panel 3
#define AID_NOSECONEINDICATOR           116  // formerly panel 3
#define AID_GEARSWITCH                  117  // formerly panel 3
#define AID_GEARINDICATOR               118  // formerly panel 3
#define AID_RETRODOORSWITCH             119  // formerly panel 0
#define AID_RETRODOORINDICATOR          120  // new
#define AID_HATCHINDICATOR              121  // new
#define AID_LADDERINDICATOR             122  // new   
#define AID_OUTERDOORINDICATOR          123  // new
#define AID_INNERDOORINDICATOR          124  // new
#define AID_RADIATORINDICATOR           125  // new
#define AID_RADIATORSWITCH              126  // formerly panel 0
#define AID_HATCHSWITCH                 127  // formerly panel 0
#define AID_LADDERSWITCH                128  // formerly panel 0
#define AID_SWITCHLED_NAV               129  // green LED under a switch
#define AID_SWITCHLED_BEACON            130  // green LED under a switch
#define AID_SWITCHLED_STROBE            131  // green LED under a switch
#define AID_MET_DAYS                    132  // days numbers on MET timer
#define AID_MET_HOURS                   133  // hours numbers on MET timer
#define AID_MET_MINUTES                 134  // minutes numbers on MET timer
#define AID_MET_SECONDS                 135  // seconds numbers on MET timer
#define AID_MET_RESETBUTTON             136  // days numbers on MET timer
#define AID_INTERVAL1_DAYS              137  // Interval1 timer
#define AID_INTERVAL1_HOURS             138  
#define AID_INTERVAL1_MINUTES           139  
#define AID_INTERVAL1_SECONDS           140  
#define AID_INTERVAL1_RESETBUTTON       141  
#define AID_INTERVAL2_DAYS              142  // Interval2 timer
#define AID_INTERVAL2_HOURS             143  
#define AID_INTERVAL2_MINUTES           144
#define AID_INTERVAL2_SECONDS           145  
#define AID_INTERVAL2_RESETBUTTON       146
#define AID_ARM_OUTER_AIRLOCK_DOOR      147
#define AID_ARM_CREW_HATCH              148
#define AID_HOVERDOORSWITCH             149
#define AID_HOVERDOORINDICATOR          150
#define AID_SCRAMDOORSWITCH             151
#define AID_SCRAMDOORINDICATOR          152
#define AID_CHAMBERSWITCH               153
#define AID_CHAMBERINDICATOR            154
#define AID_CREW_DISPLAY                155

// upper panel area IDs only used by subclasses
#define AID_BAYDOORSSWITCH              156
#define AID_BAYDOORSINDICATOR           157
#define AID_SWITCH_TO_PAYLOAD_CAMERA_VIEW   158

// Panel 2 (lower)
#define AID_DOCKRELEASE        200
#define AID_PGIMBALMAIN        201
#define AID_PGIMBALMAINCENTER  202
#define AID_YGIMBALMAIN        203

#define AID_GIMBALSCRAM        205
#define AID_GIMBALSCRAMCENTER  206
#define AID_PGIMBALMAINDISP    207
#define AID_YGIMBALMAINDISP    208
#define AID_GIMBALSCRAMDISP    209

#define AID_HOVERBALANCE       210
#define AID_HBALANCECENTER     211
#define AID_HBALANCEDISP       212

#define AID_HORIZON            213
#define AID_AOAINSTR           214
#define AID_SLIPINSTR          215

#define AID_MAIN_FUELBAR       216
#define AID_RCS_FUELBAR        217
#define AID_SCRAM_FUELBAR      218
#define AID_APU_FUELBAR        219

#define AID_MAIN_FUELDUMP_BUTTON    220
#define AID_RCS_FUELDUMP_BUTTON     221
#define AID_SCRAM_FUELDUMP_BUTTON   222
#define AID_APU_FUELDUMP_BUTTON     223

#define AID_XFEED_KNOB              224
#define AID_SYSTEMS_DISPLAY_SCREEN  225

// Resupply Hatches
#define AID_FUELHATCHSWITCH         226
#define AID_FUELHATCHLED            227
#define AID_LOXHATCHSWITCH          228
#define AID_LOXHATCHLED             229

// External Supply Line Pressure
#define AID_MAINSUPPLYLINE_PSI      230
#define AID_SCRAMSUPPLYLINE_PSI     231
#define AID_APUSUPPLYLINE_PSI       232
#define AID_LOXSUPPLYLINE_PSI       233

#define AID_MAINSUPPLYLINE_GAUGE    234
#define AID_SCRAMSUPPLYLINE_GAUGE   235
#define AID_APUSUPPLYLINE_GAUGE     236
#define AID_LOXSUPPLYLINE_GAUGE     237

#define AID_MAINSUPPLYLINE_LED      238
#define AID_SCRAMSUPPLYLINE_LED     239
#define AID_APUSUPPLYLINE_LED       240
#define AID_LOXSUPPLYLINE_LED       241

// External Supply Lines
#define AID_MAINSUPPLYLINE_SWITCH   242
#define AID_SCRAMSUPPLYLINE_SWITCH  243
#define AID_APUSUPPLYLINE_SWITCH    244
#define AID_LOXSUPPLYLINE_SWITCH    245 

#define AID_MAINSUPPLYLINE_SWITCH_LED   246
#define AID_SCRAMSUPPLYLINE_SWITCH_LED  247
#define AID_APUSUPPLYLINE_SWITCH_LED    248
#define AID_LOXSUPPLYLINE_SWITCH_LED    249 

#define AID_SHIPMASS_LB         250
#define AID_SHIPMASS_KG         251

// LOX Areas
#define AID_LOX_BAR             252
#define AID_LOX_TEXT            253
#define AID_LOX_DUMP_BUTTON     254

// Oxygen Remaining Panel Areas
#define AID_OXYGEN_REM_PCT      255
#define AID_OXYGEN_REM_DAYS     256
#define AID_OXYGEN_REM_HOURS    257
#define AID_OXYGEN_REM_MINUTES  258
#define AID_OXYGEN_REM_SECONDS  259
#define AID_CREW_MEMBERS_TEXT   260
#define AID_CABIN_O2_PCT        261

// Coolant Temp Gauge
#define AID_COOLANT_BAR             262
#define AID_COOLANT_TEXT            263
#define AID_RADIATOR_DEPLOYED_LED   264

// External Cooling Areas
#define AID_EXTERNAL_COOLING_SWITCH 265
#define AID_EXTERNAL_COOLING_LED    266

// Turbopack Management screen
#define AID_TURBOPACK_MANAGEMENT_SCREEN 267
//
// Other IDs
//
#define AID_MFD1_SCREEN          500
#define AID_MFD2_SCREEN          501
#define AID_YGIMBALMAINCENTER    502
#define AID_YGIMBALMAINDIV       503
#define AID_YGIMBALMAINAUTO      504
#define AID_AIRBRAKESWITCH       505
#define AID_AIRBRAKEINDICATOR    506
#define AID_AUTOPILOTLED         507

// VC-only MFD buttons
// IDs must be paired for each MFD, like with 2D MFD button IDs
#define AID_MFD1_PWR          1024
#define AID_MFD2_PWR          1025

#define AID_MFD1_SEL          1026
#define AID_MFD2_SEL          1027

#define AID_MFD1_MNU          1028
#define AID_MFD2_MNU          1029

// VC-only HUD and NAV buttons
#define AID_HUDBUTTON1        1030
#define AID_HUDBUTTON2        1031
#define AID_HUDBUTTON3        1032
#define AID_HUDBUTTON4        1033
#define AID_NAVBUTTON1        1037
#define AID_NAVBUTTON2        1038
#define AID_NAVBUTTON3        1039
#define AID_NAVBUTTON4        1040
#define AID_NAVBUTTON5        1041
#define AID_NAVBUTTON6        1042

// VC levers and switches
#define AID_RADIATOREX        1043 
#define AID_RADIATORIN        1044
#define AID_HATCHOPEN         1045
#define AID_HATCHCLOSE        1046
#define AID_LADDEREX          1047
#define AID_LADDERIN          1048
#define AID_RCOVEROPEN        1049
#define AID_RCOVERCLOSE       1050
#define AID_ILOCKOPEN         1051
#define AID_ILOCKCLOSE        1052
#define AID_OLOCKOPEN         1053
#define AID_OLOCKCLOSE        1054
#define AID_NCONEOPEN         1055
#define AID_NCONECLOSE        1056
#define AID_GEARDOWN          1057
#define AID_GEARUP            1058

