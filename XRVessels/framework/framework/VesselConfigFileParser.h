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
// VesselConfigFileParser.h
// Abstract base class to parse a vessel's configuration file
// ==============================================================

#pragma once

#include "ConfigFileParser.h"  

// 2D panel width flags
// NOTE: if you add additional widths, be sure to update VESSEL3_EXT::Get2DPanelWidth() as well.
enum TWO_D_PANEL_WIDTH { AUTODETECT, USE1280, USE1600, USE1920 };

class VesselConfigFileParser : public ConfigFileParser
{
public:
    VesselConfigFileParser(const char *pDefaultFilename, const char *pLogFilename);
    virtual ~VesselConfigFileParser();

    bool ParseVesselConfig(const char *pVesselName);    // e.g., pVesselName = "XR5-01"
    TWO_D_PANEL_WIDTH GetTwoDPanelWidth() const { return TwoDPanelWidth; }

protected:
    // parsed data values required for the framework
    // NOTE: THE SUBCLASS *MUST* POPULATE THESE VALUES!
    TWO_D_PANEL_WIDTH TwoDPanelWidth;

private:
};