// ==============================================================
// XR2Ravenstar implementation class
//
// Copyright 2008-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// ConfigFileParser.h
// Parse the XR2 configuration file.
// ==============================================================

#pragma once

#define _CRT_SECURE_NO_DEPRECATE
#include "windows.h"
#include "XR1ConfigFileParser.h"  // our base class
#include "SecondaryHUDData.h"
#include "XR2Globals.h"

class XR2ConfigFileParser : public XR1ConfigFileParser
{
public:
    XR2ConfigFileParser();

    virtual bool XR2ConfigFileParser::ParseLine(const char *pSection, const char *pPropertyName, const char *pValue, const bool bParsingOverrideFile);

    // parsed data values
    double PayloadScreensUpdateInterval;   // interval in seconds
    bool EnableHalloweenEasterEgg;
    bool ForceMarvinVisible;    
    bool EnableFuzzyDice;  
    bool EnableAFCtrlPerformanceModifier;
    double AFCtrlPerformanceModifier[2];  // [0] = "Pitch", [1] = "On"
    int RequirePayloadBayFuelTanks;
};
