// ==============================================================
// XR3Phoenix implementation class
//
// Copyright 2007-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// ConfigFileParser.h
// Parse the XR3 configuration file.
// ==============================================================

#pragma once

#define _CRT_SECURE_NO_DEPRECATE
#include "windows.h"
#include "XR1ConfigFileParser.h"  // our base class
#include "SecondaryHUDData.h"
#include "XR3Globals.h"

class XR3ConfigFileParser : public XR1ConfigFileParser
{
public:
    XR3ConfigFileParser();

    virtual bool XR3ConfigFileParser::ParseLine(const char *pSection, const char *pPropertyName, const char *pValue, const bool bParsingOverrideFile);

    // NOTE: common payload items moved up to XR1 class; nothing custom for now.
};
