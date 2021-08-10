// ==============================================================
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// Copyright 2006-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR1Colors.h
// Defines colors for the XR1
// ==============================================================

#pragma once

// Colors
//
#define LIGHT_GREEN   oapiGetColour(206, 241, 153) // MDA status text
#define MEDIUM_GREEN  oapiGetColour( 26, 213, 64)  // autopilot LED
#define MEDIUM_YELLOW oapiGetColour(221, 221, 40) 
#define BRIGHT_GREEN  oapiGetColour(112, 236,   0)
#define BRIGHT_YELLOW oapiGetColour(249, 249,   0)
#define BRIGHT_WHITE  oapiGetColour(251, 251, 251)  // NOTE: 254,254,254 goes transparent on D3D9 client, so back it down a bit
#define BRIGHT_RED    oapiGetColour(243,  17,  17)
#define MEDB_RED      oapiGetColour(242,  64,  64)  // medium-bright red
#define DARK_BLUE     oapiGetColour(42,   58, 130)
#define OFF_WHITE192  oapiGetColour(192, 192, 192)
#define OFF_WHITE217  oapiGetColour(217, 217, 217)
#define LIGHT_RED     oapiGetColour(244, 117, 117)
#define LIGHT_YELLOW  oapiGetColour(237, 237, 103)
#define LIGHT_BLUE    oapiGetColour(162, 190, 255)
#define CYAN          oapiGetColour(26,  247, 255)
#define ORANGE        oapiGetColour(242, 173,  26)
#define CBLACK        0x000000
#define CWHITE        0xFFFFFF

// convert RGB value to BGR, a Windows' COLORREF
#define CREF(rgb) (((rgb & 0xFF) << 16) | (rgb & 0xFF00) | ((rgb & 0xFF0000) >> 16))
