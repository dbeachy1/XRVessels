// ==============================================================
// XR3Phoenix implementation class
//
// Copyright 2007-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR3AreaIDs.h
// Area IDs for the XR3
// ==============================================================

#pragma once

#include "AreaIDs.h"   // XR1 areas

// main panel
#define AID_RCS_CONFIG_BUTTON   2000
#define AID_XR3_WARNING_LIGHTS  2001

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