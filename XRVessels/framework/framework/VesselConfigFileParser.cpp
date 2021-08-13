/**
  XR Vessel add-ons for OpenOrbiter Space Flight Simulator
  Copyright (C) 2006-2021 Douglas Beachy

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

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
// ConfigFileParser.cpp
// Base class to parse a vessel's configuration file.
// ==============================================================

#include "VesselConfigFileParser.h"

// Constructor
// pFilename = path to config file; may be relative to Orbiter root or absolute
// pLogFilename = path to optional (but highly recommended) log file; may be null
VesselConfigFileParser::VesselConfigFileParser(const char *pDefaultFilename, const char *pLogFilename) :
    ConfigFileParser(pDefaultFilename, pLogFilename),
    TwoDPanelWidth(USE1280)  // default to the smallest panel
{
}

// Destructor
VesselConfigFileParser::~VesselConfigFileParser()
{
}

// Begin parsing the vessel config file(s)
//
//  pVesselName: "XR5-01", etc: GetName() from the parent XR vessel.  Will be used to read & parse optional "Config\XR5-01.xrcfg" override file, if it exists.
//               Applied *after* the default file is read. 
//
// Returns: true on success, false if I/O error occurs or if default preference file does not exist
bool VesselConfigFileParser::ParseVesselConfig(const char *pVesselName)
{
    SetLogPrefix(pVesselName);
    
    m_csOverrideFilename.Format("Config\\%s.xrcfg", pVesselName);  // e.g., "Config\XR5-01.xrcfg"
    const BOOL overrideFileExists = PathFileExists(m_csOverrideFilename);

    if (overrideFileExists)
        m_csConfigFilenames.Format("%s + %s", GetDefaultFilename(), GetOverrideFilename());
    else
    {
        m_csConfigFilenames.Format("%s (no override found [%s])", GetDefaultFilename(), GetOverrideFilename());
        m_csOverrideFilename.Empty();  // empty the filename to indicate it does not exist
    }

    // log the filenames
    CString csTemp;
    csTemp.Format("Using configuration file(s): %s", GetConfigFilenames());
    WriteLog(csTemp);

    // parse the default config file first
    bool retVal = ParseFile();  // ignore error here (any errors were already logged)

    // now parse the override file if any exists
    if (overrideFileExists)
        retVal &= ParseFile(GetOverrideFilename());  // clear flag if error occurs

    return retVal;
}
