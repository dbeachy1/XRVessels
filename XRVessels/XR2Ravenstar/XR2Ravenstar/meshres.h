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

//----------------------------------------------------------------------------
// Created by Obj2Msh 3.0, 07.10.2011 10:23:48 PM
//
// Mesh Filename: deltaglider221_45.msh
//----------------------------------------------------------------------------

#pragma once

// Number of mesh groups
#define NGRP 133

// Number of materials
#define NMAT 24

// Number of textures
#define NTEX 21

// Named mesh groups
#define GRP_bottom_elevators_bottom_starboard 0
#define GRP_bottom_elevators_bottom_starboard_fixup_1 1
#define GRP_starboard_rudder 2
#define GRP_port_radiator_panel 3
#define GRP_starboard_radiator_panel 4
#define GRP_port_rudder 5
#define GRP_starboard_scram_door 6
#define GRP_Port_scram_door 7
#define GRP_retromachinery1 8
#define GRP_retrocover_port 9
#define GRP_chair_pilot 10
#define GRP_port_top_petal 11
#define GRP_port_bottom_petal 12
#define GRP_starboard_top_petal 13
#define GRP_starboard_bottom_petal 14
#define GRP_docking_collar 15
#define GRP_passengerwindowframes 16
#define GRP_cockpitcanopyframe_outer 17
#define GRP_chair_copilot 18
#define GRP_touchscreens 19
#define GRP_cockpitcanopyframe 20
#define GRP_upperhull 21
#define GRP_upperhull_fixup_1 22
#define GRP_upperhull_fixup_2 23
#define GRP_interior 24
#define GRP_Upper_hatch_bottom 25
#define GRP_propellant_Flap 26
#define GRP_LOX_flap 27
#define GRP_top_elevators_bottom_starboard 28
#define GRP_top_elevators_bottom_starboard_fixup_1 29
#define GRP_top_elevators_bottom_starboard_fixup_2 30
#define GRP_top_elevators_bottom_starboard_fixup_3 31
#define GRP_top_elevators_bottom_starboard_fixup_4 32
#define GRP_retromachinery2 33
#define GRP_retrocover_starboard 34
#define GRP_airlock_innerdoor 35
#define GRP_airlock_innerdoor_fixup_1 36
#define GRP_Port_front_gear_door 37
#define GRP_starboard_front_gear_door 38
#define GRP_central_front_gear_door 39
#define GRP_takeoffflap2 40
#define GRP_takeoffflap2_fixup_1 41
#define GRP_takeoffflap1 42
#define GRP_starboard_rad 43
#define GRP_port_rad 44
#define GRP_aft_takeoff_cover 45
#define GRP_gearflap3_inner 46
#define GRP_gearflap_4_inner 47
#define GRP_gearflap_1_inner 48
#define GRP_top_hatch_inner_door 49
#define GRP_port_outerdoor 50
#define GRP_port_outerdoor_fixup_1 51
#define GRP_port_outerdoor_fixup_2 52
#define GRP_starboard_outerdoor 53
#define GRP_starboard_outerdoor_fixup_1 54
#define GRP_starboard_outerdoor_fixup_2 55
#define GRP_docking_collar_grabber 56
#define GRP_passengerchairs 57
#define GRP_retro_nozzle2 58
#define GRP_portfore_pbd_top 59
#define GRP_portmid_pbd_bottom 60
#define GRP_portaft_pbd_bottom 61
#define GRP_CARGOBAY 62
#define GRP_starboardaft_pbd_top 63
#define GRP_starboardmid_pbd_top 64
#define GRP_starboardfore_pbd_bottom 65
#define GRP_lowerhull 66
#define GRP_upperhatchtop 67
#define GRP_starboardfore_pbd_top 68
#define GRP_starboardmid_pbd_bottom 69
#define GRP_starboardaft_pbd_bottom 70
#define GRP_portaft_pbd_top 71
#define GRP_portmid_pbd_top 72
#define GRP_portfore_pbd_bottom 73
#define GRP_top_elevators_top01_starboard 74
#define GRP_bottom_elevators_top_starboard 75
#define GRP_aft_takeoff_cover_inside 76
#define GRP_gearflap_1_outer 77
#define GRP_gearflap_2outer 78
#define GRP_gearflap_2outer_fixup_1 79
#define GRP_gearflap_2outer_fixup_2 80
#define GRP_gearflap3_outer 81
#define GRP_gearflap4_outer 82
#define GRP_retronozzle1 83
#define GRP_takeoffflap1_inner 84
#define GRP_LOX_flap_inner 85
#define GRP_propellant_Flap_inner 86
#define GRP_port_radiator_panel_inner 87
#define GRP_starboard_radiator_panel_inner 88
#define GRP_retrocover_starboard_inner 89
#define GRP_retrocover_port_inner 90
#define GRP_starboard_front_gear_door_inner 91
#define GRP_Port_front_gear_door_inner 92
#define GRP_central_front_gear_door_inner 93
#define GRP_port_bottom_petal_inner 94
#define GRP_starboard_bottom_petal_inner 95
#define GRP_port_top_petal_inner 96
#define GRP_starboard_top_petal_inner 97
#define GRP_furrydice 98
#define GRP_furrydice01 99
#define GRP_Line01 100
#define GRP_Cylinder16 101
#define GRP_Object10 102
#define GRP_Object11 103
#define GRP_Object34 104
#define GRP_kara 105
#define GRP_kara_fixup_1 106
#define GRP_kara_fixup_2 107
#define GRP_lee 108
#define GRP_lee_fixup_1 109
#define GRP_lee_fixup_2 110
#define GRP_grey 111
#define GRP_top_elevators_top01_port 112
#define GRP_bottom_elevators_bottom_port 113
#define GRP_bottom_elevators_bottom_port_fixup_1 114
#define GRP_Rear_Wheel_Port 115
#define GRP_Rear_Wheel_Starboard 116
#define GRP_Forward_Wheels 117
#define GRP_top_elevators_bottom_port 118
#define GRP_bottom_elevators_bottom_port01 119
#define GRP_front_inner_strut 120
#define GRP_cockpit 121
#define GRP_fueldumpers 122
#define GRP_rcs 123
#define GRP_starboard_nozzle 124
#define GRP_port_nozzle 125
#define GRP_starboard_engine_cone 126
#define GRP_port_engine_cone 127
#define GRP_passengerwindows 128
#define GRP_cockpitglass01 129
#define GRP_hudglass 130
#define GRP_hudglass_pilot 131
#define GRP_hudglass_copilot 132
// end of file
