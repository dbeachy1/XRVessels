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
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// XR1InstrumentPanels.h
// Instrument panels for the DG-XR1.
// ==============================================================

#pragma once

#include "Orbitersdk.h"
#include "InstrumentPanel.h"
#include "Area.h"
#include "DeltaGliderXR1.h"

//
// Class definitions
//

//----------------------------------------------------------------------

// convenience base class for all our XR1 panels
class XR1InstrumentPanel : public InstrumentPanel
{
public:
    XR1InstrumentPanel(DeltaGliderXR1 &vessel, const int panelID, const WORD panelResourceID = -1);  // panelResourceID is not always used

    // convenience method to return our actual vessel object
    DeltaGliderXR1 &GetXR1() const { return static_cast<DeltaGliderXR1 &>(GetVessel()); }

    // methods shared among multiple instrument panels
    void InitMDA(MultiDisplayArea *pMDA);

private:
};

//----------------------------------------------------------------------
// Base classes for our different instrument panels; these classes contain
// code and data common for each panel regardless of its resolution.
//----------------------------------------------------------------------

class XR1MainInstrumentPanel : public XR1InstrumentPanel
{
public:
    XR1MainInstrumentPanel(DeltaGliderXR1 &vessel, const WORD panelResourceID) :
        XR1InstrumentPanel(vessel, PANEL_MAIN, panelResourceID)
    {
    }

    virtual bool Activate();
    virtual void Deactivate();
};

class XR1UpperInstrumentPanel : public XR1InstrumentPanel
{
public:
    XR1UpperInstrumentPanel(DeltaGliderXR1 &vessel, const WORD panelResourceID) :
        XR1InstrumentPanel(vessel, PANEL_UPPER, panelResourceID)
    {
    }

    virtual bool Activate();
};

class XR1LowerInstrumentPanel : public XR1InstrumentPanel
{
public:
    XR1LowerInstrumentPanel(DeltaGliderXR1 &vessel, const WORD panelResourceID) :
        XR1InstrumentPanel(vessel, PANEL_LOWER, panelResourceID)
    {
    }

    virtual bool Activate();
};


//----------------------------------------------------------------------
// 1280-pixel-wide panels
//----------------------------------------------------------------------

// Main 2D 1280-pixel instrument panel
class XR1MainInstrumentPanel1280 : public XR1MainInstrumentPanel
{
public:
    XR1MainInstrumentPanel1280(DeltaGliderXR1 &vessel);
};

//----------------------------------------------------------------------

// Upper 2D 1280-pixel instrument panel
class XR1UpperInstrumentPanel1280 : public XR1UpperInstrumentPanel
{
public:
    XR1UpperInstrumentPanel1280(DeltaGliderXR1 &vessel);
};

//----------------------------------------------------------------------

// Lower 2D 1280-pixel instrument panel
class XR1LowerInstrumentPanel1280 : public XR1LowerInstrumentPanel
{
public:
    XR1LowerInstrumentPanel1280(DeltaGliderXR1 &vessel);
};

//----------------------------------------------------------------------
// 1600-pixel-wide panels
//----------------------------------------------------------------------

// Main 2D 1600-pixel instrument panel
class XR1MainInstrumentPanel1600 : public XR1MainInstrumentPanel
{
public:
    XR1MainInstrumentPanel1600(DeltaGliderXR1 &vessel);
};

//----------------------------------------------------------------------

// Upper 2D 1600-pixel instrument panel
class XR1UpperInstrumentPanel1600 : public XR1UpperInstrumentPanel
{
public:
    XR1UpperInstrumentPanel1600(DeltaGliderXR1 &vessel);
};

//----------------------------------------------------------------------

// Lower 2D 1600-pixel instrument panel
class XR1LowerInstrumentPanel1600 : public XR1LowerInstrumentPanel
{
public:
    XR1LowerInstrumentPanel1600(DeltaGliderXR1 &vessel);
};

//----------------------------------------------------------------------
// 1920-pixel-wide panels
//----------------------------------------------------------------------

// Main 2D 1920-pixel instrument panel
class XR1MainInstrumentPanel1920 : public XR1MainInstrumentPanel
{
public:
    XR1MainInstrumentPanel1920(DeltaGliderXR1 &vessel);
};

//----------------------------------------------------------------------

// Upper 2D 1920-pixel instrument panel
class XR1UpperInstrumentPanel1920 : public XR1UpperInstrumentPanel
{
public:
    XR1UpperInstrumentPanel1920(DeltaGliderXR1 &vessel);
};

//----------------------------------------------------------------------

// Lower 2D 1920-pixel instrument panel
class XR1LowerInstrumentPanel1920 : public XR1LowerInstrumentPanel
{
public:
    XR1LowerInstrumentPanel1920(DeltaGliderXR1 &vessel);
};

//----------------------------------------------------------------------

//
// Virtual Cockpit panels
//

// Pilot

class XR1VCPilotInstrumentPanel : public XR1InstrumentPanel
{
public:
    XR1VCPilotInstrumentPanel(DeltaGliderXR1 &vessel, const int panelID);
    virtual bool Activate();
};

//----------------------------------------------------------------------
// Passenger #1

class XR1VCPassenger1InstrumentPanel : public XR1InstrumentPanel
{
public:
    XR1VCPassenger1InstrumentPanel(DeltaGliderXR1 &vessel, const int panelID);
    virtual bool Activate();
};

//----------------------------------------------------------------------
// Passenger #2

class XR1VCPassenger2InstrumentPanel : public XR1InstrumentPanel
{
public:
    XR1VCPassenger2InstrumentPanel(DeltaGliderXR1 &vessel, const int panelID);
    virtual bool Activate();
};

//----------------------------------------------------------------------
// Passenger #3

class XR1VCPassenger3InstrumentPanel : public XR1InstrumentPanel
{
public:
    XR1VCPassenger3InstrumentPanel(DeltaGliderXR1 &vessel, const int panelID);
    virtual bool Activate();
};

//----------------------------------------------------------------------
// Passenger #4

class XR1VCPassenger4InstrumentPanel : public XR1InstrumentPanel
{
public:
    XR1VCPassenger4InstrumentPanel(DeltaGliderXR1 &vessel, const int panelID);
    virtual bool Activate();
};
