// ==============================================================
// XR Vessel Framework
//
// Copyright 2006-2018 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
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