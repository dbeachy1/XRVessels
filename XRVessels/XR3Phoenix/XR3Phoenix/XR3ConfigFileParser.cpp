// ==============================================================
// XR3Phoenix implementation class
//
// Copyright 2007-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR3ConfigFileParser.cpp
// Parse the XR3 configuration file.
// Blank lines and lines beginning with "#" are ignored.
//
// Format is:
//
// [SECTION]
// name=value [,value2,value3,...]
//
// [SECTION-2]
// ...
// ==============================================================

#include "XR3ConfigFileParser.h"
#include "ConfigFileParserMacros.h"
#include "XR3Globals.h"

#include <stdio.h>
#include <string.h>

// Constructor
// sets default values for memeber variables here
XR3ConfigFileParser::XR3ConfigFileParser() :
    XR1ConfigFileParser()
{
}

// Parse a line; invoked by our superclass
// returns: true if line OK, false if error
bool XR3ConfigFileParser::ParseLine(const char *pSection, const char *pPropertyName, const char *pValue, const bool bParsingOverrideFile)
{
#ifdef UNUSED
    int len;        // used by macros
    char temp[MAX_LINE_LENGTH + 128];  // used for string scanning and error handling
#endif
    
    bool processed = false;     // set to 'true' by macros if parameter processed; the macros expect this variable to exist

    // Note: 'processed' is set by the parsing macros, so we do not need to set it manually below

    // parse [GENERAL] settings
    if (SECTION_MATCHES("GENERAL"))
    {
        if (PNAME_MATCHES("PayloadScreensUpdateInterval"))
        {
            SSCANF1("%lf", &PayloadScreensUpdateInterval);
            VALIDATE_DOUBLE(&PayloadScreensUpdateInterval, 0, 2.0, 0.05);
        }

    }
    
    // parse [CHEATCODES] settings
    
    // if we didn't process this line, pass it up to our superclass to try it...
    return (processed ? true : XR1ConfigFileParser::ParseLine(pSection, pPropertyName, pValue, bParsingOverrideFile));
}
