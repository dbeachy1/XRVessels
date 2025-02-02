/**
  XR Vessel add-ons for OpenOrbiter Space Flight Simulator
  Copyright (C) 2006-2025 Douglas Beachy

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
// XR5Vanguard implementation class
//
// XR5ConfigFileParser.cpp
// Parse the XR5 configuration file.
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

#include "XR5ConfigFileParser.h"
#include "ConfigFileParserMacros.h"
#include "XR5Globals.h"

#include <stdio.h>
#include <string.h>

// Constructor
// sets default values for memeber variables here
XR5ConfigFileParser::XR5ConfigFileParser() :
    XR1ConfigFileParser()
{
}

// Parse a line; invoked by our superclass
// returns: true if line OK, false if error
bool XR5ConfigFileParser::ParseLine(const char *pSection, const char *pPropertyName, const char *pValue, const bool bParsingOverrideFile)
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
