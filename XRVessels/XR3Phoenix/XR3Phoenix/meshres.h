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
// Mesh Filename: XR3Phoenix.msh
//----------------------------------------------------------------------------

#pragma once

// Number of mesh groups
#define NGRP 223

// Number of materials
#define NMAT 27

// Number of textures
#define NTEX 23

// Named mesh groups
#define GRP_right_wing_tip 0
#define GRP_left_wing_tip 1
#define GRP_hover_door_left_2_left_wing 2
#define GRP_hover_door_right_2_left_wing 3
#define GRP_hover_door_left_1_left_wing 4
#define GRP_hover_door_right_1_left_wing 5
#define GRP_hover_door_left_2_right_wing 6
#define GRP_hover_door_right_2_right_wing 7
#define GRP_hover_door_left_1_right_wing 8
#define GRP_hover_door_right_1_right_wing 9
#define GRP_hover_door_left_rear 10
#define GRP_hover_door_left_front 11
#define GRP_hover_door_right_aft 12
#define GRP_hover_door_right_front 13
#define GRP_gear_bay2 14
#define GRP_Hatch_left_front 15
#define GRP_hatch_left_back 16
#define GRP_Hatch_right_back 17
#define GRP_Hatch_right_front 18
#define GRP_gear_bay1 19
#define GRP_retro_bay_right 20
#define GRP_retro_bay_left 21
#define GRP_gear_door_left_inside 22
#define GRP_gear_door_right_inside 23
#define GRP_elevator_left 24
#define GRP_lower_brake_right 25
#define GRP_upper_brake_right 26
#define GRP_upper_brake_left 27
#define GRP_lower_brake_left 28
#define GRP_elevator_right 29
#define GRP_polySurface9109 30
#define GRP_polySurface9113 31
#define GRP_cockpit_door 32
#define GRP_nose_door_back_left 33
#define GRP_nose_door_front_left 34
#define GRP_nose_door_back_right 35
#define GRP_nose_door_front_right 36
#define GRP_HAL_9000 37
#define GRP_gear_door_right_outside_1 38
#define GRP_gear_door_right_outside_2 39
#define GRP_cockpit_floor1 40
#define GRP_cabin_left 41
#define GRP_cabin_right 42
#define GRP_radiator_door_bottom_right 43
#define GRP_radiator_door_top_right 44
#define GRP_radiator_door_bottom_left 45
#define GRP_radiator_door_top_left 46
#define GRP_pCube5 47
#define GRP_pCube7 48
#define GRP_pCube8 49
#define GRP_pCube10 50
#define GRP_el_door_right_inside 51
#define GRP_el_door_right_outside 52
#define GRP_el_door_left_outside 53
#define GRP_el_door_left_inside 54
#define GRP_hull_back1 55
#define GRP_radiator_bay_left 56
#define GRP_radiator_bay_right 57
#define GRP_elevator_door_aft 58
#define GRP_elevator_door_forward 59
#define GRP_inside_door 60
#define GRP_hull 61
#define GRP_scram_door1 62
#define GRP_polySurface8521 63
#define GRP_elevator_pod1 64
#define GRP_el_arm_pistion_00 65
#define GRP_el_arm_cylinder_00 66
#define GRP_el_arm_cylinder_01 67
#define GRP_el_arm_pistion_01 68
#define GRP_dock_door_right 69
#define GRP_dock_door_left 70
#define GRP_bay_door_left 71
#define GRP_bay_door_right 72
#define GRP_elevator_arm_door_aft 73
#define GRP_elevator_arm_door_forward 74
#define GRP_scram_scoop1 75
#define GRP_polySurface8612 76
#define GRP_polySurface8616 77
#define GRP_polySurface8617 78
#define GRP_polySurface8619 79
#define GRP_polySurface8621 80
#define GRP_polySurface8623 81
#define GRP_polySurface8625 82
#define GRP_polySurface8645 83
#define GRP_polySurface8660 84
#define GRP_polySurface8665 85
#define GRP_polySurface8672 86
#define GRP_polySurface8677 87
#define GRP_polySurface8678 88
#define GRP_polySurface8679 89
#define GRP_polySurface8680 90
#define GRP_polySurface8685 91
#define GRP_polySurface8690 92
#define GRP_polySurface8695 93
#define GRP_polySurface8700 94
#define GRP_polySurface8705 95
#define GRP_polySurface8710 96
#define GRP_polySurface8715 97
#define GRP_nose_gear_wheel_right 98
#define GRP_nose_gear_wheel_left 99
#define GRP_wheel_left_front_left_side 100
#define GRP_wheel_right_front_left_side 101
#define GRP_wheel_left_rear_left_side 102
#define GRP_wheel_right_rear_left_side 103
#define GRP_wheel_left_rear_right_side 104
#define GRP_wheel_right_rear_right_side 105
#define GRP_wheel_left_front_right_side 106
#define GRP_wheel_right_front_right_side 107
#define GRP_polySurface8813 108
#define GRP_polySurface8823 109
#define GRP_polySurface8826 110
#define GRP_polySurface8827 111
#define GRP_polySurface8828 112
#define GRP_polySurface8829 113
#define GRP_polySurface8830 114
#define GRP_polySurface8840 115
#define GRP_polySurface8843 116
#define GRP_polySurface8873 117
#define GRP_polySurface8923 118
#define GRP_polySurface8924 119
#define GRP_retro_rocket_outside_right 120
#define GRP_retro_rocket_inside_right 121
#define GRP_retro_arm_right 122
#define GRP_retro_rocket_outside_left 123
#define GRP_retro_rocket_inside_left 124
#define GRP_retro_arm_left 125
#define GRP_rad_panel_right_002 126
#define GRP_rad_panel_right_003 127
#define GRP_rad_panel_right_004 128
#define GRP_rad_panel_right_001 129
#define GRP_rad_panel_left_002 130
#define GRP_rad_panel_left_003 131
#define GRP_rad_panel_left_004 132
#define GRP_rad_panel_left_001 133
#define GRP_polySurface8945 134
#define GRP_polySurface8966 135
#define GRP_polySurface8985 136
#define GRP_polySurface8997 137
#define GRP_polySurface9019 138
#define GRP_gear_door_left_outside_1 139
#define GRP_gear_door_left_outside_2 140
#define GRP_gear_bay3 141
#define GRP_polySurface9041 142
#define GRP_polySurface9040 143
#define GRP_polySurface9022 144
#define GRP_dockport_00 145
#define GRP_polySurface9044 146
#define GRP_nose_axle 147
#define GRP_nose_oleo_piston 148
#define GRP_nose_oleo_cylinder 149
#define GRP_axle_right 150
#define GRP_oleo_piston_right 151
#define GRP_axle_cylinder_right 152
#define GRP_axle_piston_right 153
#define GRP_axle_left 154
#define GRP_oleo_piston_left 155
#define GRP_gear_main_oleo_cylinder_left 156
#define GRP_axle_cylinder_left 157
#define GRP_axle_piston_left 158
#define GRP_polySurface8570 159
#define GRP_pilot_right 160
#define GRP_polySurface9074 161
#define GRP_pilot_left 162
#define GRP_nose_axle_cylinder 163
#define GRP_nose_axle_piston 164
#define GRP_polySurface9083 165
#define GRP_polySurface9086 166
#define GRP_polySurface9089 167
#define GRP_polySurface9092 168
#define GRP_polySurface9098 169
#define GRP_polySurface9102 170
#define GRP_gear_main_oleo_cylinder_right 171
#define GRP_polySurface9120 172
#define GRP_polySurface9121 173
#define GRP_polySurface9128 174
#define GRP_polySurface9132 175
#define GRP_polySurface9133 176
#define GRP_polySurface9134 177
#define GRP_hover_outlet_right_aft 178
#define GRP_hover_outlet_right_forward 179
#define GRP_hover_outlet_left_forward 180
#define GRP_hover_outlet_left_aft 181
#define GRP_hover_outlet_front_left 182
#define GRP_hover_outlet_front_right 183
#define GRP_polySurface9135 184
#define GRP_dockport_ring 185
#define GRP_door_petal_008 186
#define GRP_door_petal_001 187
#define GRP_door_petal_002 188
#define GRP_door_petal_003 189
#define GRP_door_petal_004 190
#define GRP_door_petal_005 191
#define GRP_door_petal_007 192
#define GRP_door_petal_006 193
#define GRP_polySurface9151 194
#define GRP_dock_port_base 195
#define GRP_dock_port_inner_door 196
#define GRP_dockport_01 197
#define GRP_cockpit_door_glass 198
#define GRP_el_door_right_inside_glass 199
#define GRP_el_door_right_outside_glass 200
#define GRP_el_door_left_outside_glass 201
#define GRP_el_door_left_inside_glass 202
#define GRP_elevator_pod_glass 203
#define GRP_polySurface8684 204
#define GRP_polySurface8689 205
#define GRP_polySurface8694 206
#define GRP_polySurface8699 207
#define GRP_polySurface8704 208
#define GRP_polySurface8714 209
#define GRP_polySurface8664 210
#define GRP_polySurface8659 211
#define GRP_polySurface8655 212
#define GRP_polySurface8709 213
#define GRP_polySurface8648 214
#define GRP_polySurface8676 215
#define GRP_polySurface8513 216
#define GRP_polySurface8528 217
#define GRP_polySurface8652 218
#define GRP_polySurface8671 219
#define GRP_pilot_right_visor 220
#define GRP_pilot_left_visor 221
#define GRP_polySurface9142 222

// end of file
