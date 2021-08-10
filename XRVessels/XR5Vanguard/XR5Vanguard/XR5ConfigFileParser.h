// ==============================================================
// XR5Vanguard implementation class
//
// Copyright 2007-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// ConfigFileParser.h
// Parse the XR5 configuration file.
// ==============================================================

#pragma once

#define _CRT_SECURE_NO_DEPRECATE
#include "windows.h"
#include "XR1ConfigFileParser.h"  // our base class
#include "SecondaryHUDData.h"
#include "XR5Globals.h"

class XR5ConfigFileParser : public XR1ConfigFileParser
{
public:
    XR5ConfigFileParser();

    virtual bool XR5ConfigFileParser::ParseLine(const char *pSection, const char *pPropertyName, const char *pValue, const bool bParsingOverrideFile);

    // NOTE: common payload items moved up to XR1 class; nothing custom for now.
};
