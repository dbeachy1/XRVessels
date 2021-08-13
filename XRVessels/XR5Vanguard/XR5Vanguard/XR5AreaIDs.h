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
// XR5Vanguard implementation class
//
// XR5AreaIDs.h
// Area IDs for the XR5
// ==============================================================

#pragma once

#include "AreaIDs.h"   // XR1 areas

// main panel
#define AID_RCS_CONFIG_BUTTON   2000
#define AID_XR5_WARNING_LIGHTS  2001

// upper panel
#define AID_SWITCH_TO_DOCKING_CAMERA_VIEW   2100
// Note: bay doors switch and indicator are in XR1
#define AID_ELEVATORSWITCH                  2103
#define AID_ELEVATORINDICATOR               2104
#define AID_ACTIVE_EVA_PORT_SWITCH          2105
#define AID_EVA_DOCKING_PORT_ACTIVE_LED     2106
#define AID_EVA_CREW_ELEVATOR_ACTIVE_LED    2107
#define AID_PAYLOADMASS_LB                  2108
#define AID_PAYLOADMASS_KG                  2109

// overhead (docking) panel
#define AID_RETURN_TO_UPPER_PANEL_VIEW  2200  /* shared with payload panel */

// payload panel
#define AID_SELECT_PAYLOAD_BAY_SLOT_SCREEN  2300
#define AID_GRAPPLE_PAYLOAD_SCREEN          2301
#define AID_DEPLOY_PAYLOAD_SCREEN           2302
#define AID_PAYLOAD_THUMBNAIL_SCREEN        2303
#define AID_PAYLOAD_EDITOR_BUTTON           2304