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
// XR2Globals.h
// Contains global variable values for the XR2 Ravenstar.
// ==============================================================

#pragma once

#include "XR1Globals.h"  // we "extend" the XR1 globals

#define PANEL_PAYLOAD   3   // Orbiter 2D panel ID (0-based)

// undef VC_PANEL_ID_BASE and other VC globals so we can redefine as necessary below
#undef VC_PANEL_ID_BASE
#undef PANELVC_PILOT
#undef PANELVC_PSNGR1
#undef PANELVC_PSNGR2
#undef PANELVC_PSNGR3
#undef PANELVC_PSNGR4
#undef ORBITER_VC_NUMBER

// define VC panel IDs
// VC_PANEL_ID_BASE is defined in XR2Globals.cpp
#define PANELVC_PILOT    (VC_PANEL_ID_BASE + 0)
#define PANELVC_COPILOT  (VC_PANEL_ID_BASE + 1)
#define PANELVC_PSNGR1   (VC_PANEL_ID_BASE + 2)
#define PANELVC_PSNGR2   (VC_PANEL_ID_BASE + 3)
#define PANELVC_AIRLOCK  (VC_PANEL_ID_BASE + 4)
#define PANELVC_PSNGR3   (VC_PANEL_ID_BASE + 5)
#define PANELVC_PSNGR4   (VC_PANEL_ID_BASE + 6)
#define PANELVC_PSNGR5   (VC_PANEL_ID_BASE + 7)
#define PANELVC_PSNGR6   (VC_PANEL_ID_BASE + 8)
#define PANELVC_PSNGR7   (VC_PANEL_ID_BASE + 9)
#define PANELVC_PSNGR8   (VC_PANEL_ID_BASE + 10)
#define PANELVC_PSNGR9   (VC_PANEL_ID_BASE + 11)
#define PANELVC_PSNGR10  (VC_PANEL_ID_BASE + 12)
#define PANELVC_PSNGR11  (VC_PANEL_ID_BASE + 13)
#define PANELVC_PSNGR12  (VC_PANEL_ID_BASE + 14)
// convert a unique VC panel ID into an Orbiter VC number (0-n)
#define ORBITER_VC_NUMBER(panelID) (panelID - VC_PANEL_ID_BASE)

extern const DWORD PILOT_HUD_MESHGRP;
extern const DWORD COPILOT_HUD_MESHGRP;
extern const double BAY_OPERATING_SPEED;
extern const double BAY_LIMIT;

extern const VECTOR3 &DOCKING_PORT_COORD;
extern const VECTOR3 &PAYLOAD_SLOT1_DIMENSIONS;

// addtional warning light values
#define XR2_WARNING_LIGHT_COUNT  1
enum XR2WarningLight 
{ 
    wl2NONE = -1,    // no light
    wl2Bay
};

// new damage enum values we need from the XR1's DamageItem enum
// WARNING: if you add or remove values here, update the D_END global as well!
#define BayDoors DISubclass1

