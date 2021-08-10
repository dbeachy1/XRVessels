// ==============================================================
// XR3Phoenix implementation class
//
// Copyright 2007-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR3Globals.h
// Contains externs for global variables and constants for the XR3.
// ==============================================================

#pragma once

#define XR1_MULTIPLIER 5.05     /* from the spreadsheet */

// extends XR1Globals
#include "XR1Globals.h"   

// size of a mesh group array
#define SizeOfGrp(grp) (sizeof(grp) / sizeof(UINT))

// unique panel IDs; standard panels are 0,1,2 and are defined in xr1globals.h
#define PANEL_OVERHEAD  3   // Orbiter 2D panel ID
#define PANEL_PAYLOAD   4   // Orbiter 2D panel ID

#ifdef UNUSED
extern const double HIGHSPEED_CENTER_OF_LIFT;
extern const double FLAPS_FULLY_RETRACTED_SPEED;
extern const double FLAPS_FULLY_DEPLOYED_SPEED;
#endif

extern const double GEAR_COMPRESSION_DISTANCE;
extern const double NOSE_GEAR_ZCOORD;
extern const double REAR_GEAR_ZCOORD;
extern const double GEAR_UNCOMPRESSED_YCOORD;

// addtional warning light values
#define XR3_WARNING_LIGHT_COUNT  2
enum XR3WarningLight 
{ 
    wl5NONE = -1,    // no light
    wl5Elev,
    wl5Bay
};

// new globals for the XR3
extern const double BAY_OPERATING_SPEED;
extern const double ELEVATOR_OPERATING_SPEED;

extern const double BAY_LIMIT;
extern const double ELEVATOR_LIMIT;
extern const VECTOR3 &DOCKING_PORT_COORD;

// new damage enum values we need from the XR1's DamageItem enum
// WARNING: if you add or remove values here, update the D_END global as well!
#define BayDoors DISubclass1
#define Elevator DISubclass2   /* XR3TODO: either remove this or use it for crew ladder */
extern const DamageItem D_END;

// ==============================================================
// Global callback prototypes 

BOOL CALLBACK XR3Ctrl_DlgProc(HWND, UINT, WPARAM, LPARAM);

