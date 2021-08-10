// ==============================================================
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// Copyright 2008-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// resource_common.h
// Defines resource IDs that must be common between the XR1 and all
// subclasses.  In order for this to work with Visual Studio's 
// resource editor, you must manually add these #define values
// to your project's resource.h file: including it will not work
// because resource.h and foo.rc are both regenerated each time
// the dialog is saved from the editor, and '#include' lines are not
// preserved.
// ==============================================================

#pragma once

// dialog resource ID constants
#define IDC_EMPTY_BAY                   4003
#define IDC_COMBO_SELECTED_PAYLOAD_OBJECT 4004
#define IDC_FILL_PAYLOAD_BAY            4005
#define IDC_SELECTED_REMOVE_ALL         4006
#define IDC_STATIC_THUMBNAIL_BMP        4007
#define IDC_STATIC_DESCRIPTION          4008
#define IDC_STATIC_MASS                 4009
#define IDC_STATIC_DIMENSIONS           4010
#define IDC_STATIC_SLOTS_OCCUPIED       4011
#define IDC_STATIC_PAYLOAD_MASS         4012
#define IDC_STATIC_VESSEL_MASS          4013
#define IDC_REMOVE_ALL                  4014
#define IDC_EMPTY_BAY2                  4015
#define IDC_BAY_REMOVE_ALL              4016
