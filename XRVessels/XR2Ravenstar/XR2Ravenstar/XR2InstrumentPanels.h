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
// XR2Ravenstar implementation class
//
// XR2InstrumentPanels.cpp
// Custom instrument panels for the XR2
// ==============================================================

#pragma once

#include "Orbitersdk.h"
#include "InstrumentPanel.h"
#include "Area.h"
#include "XR2Ravenstar.h"

//
// Class definitions
//

//----------------------------------------------------------------------

class XR2InstrumentPanel : public InstrumentPanel
{
public:
    XR2InstrumentPanel(XR2Ravenstar &vessel, const int panelID, const WORD panelResourceID = -1, const bool force3DRedrawTo2D = true);  // NOTE: force3DRedrawTo2D = true for the XR2

    // convenience method to return our actual vessel object
    XR2Ravenstar &GetXR2() const { return static_cast<XR2Ravenstar &>(GetVessel()); }

    // methods shared among multiple instrument panels
    void InitMDA(MultiDisplayArea *pMDA);

private:
};

//----------------------------------------------------------------------
// Base classes for our different instrument panels; these classes contain
// code and data common for each panel regardless of its resolution.
//----------------------------------------------------------------------

class XR2MainInstrumentPanel : public XR2InstrumentPanel
{
public:
    XR2MainInstrumentPanel(XR2Ravenstar &vessel, const WORD panelResourceID) :
        XR2InstrumentPanel(vessel, PANEL_MAIN, panelResourceID)
    {
    }

    virtual bool Activate();
    virtual void Deactivate();
};

class XR2UpperInstrumentPanel : public XR2InstrumentPanel
{
public:
    XR2UpperInstrumentPanel::XR2UpperInstrumentPanel(XR2Ravenstar &vessel, const WORD panelResourceID) :
        XR2InstrumentPanel(vessel, PANEL_UPPER, panelResourceID)
    {
    }

    virtual bool Activate();

protected:
    void AddCommonAreas(const int width);
    void Add1600PlusAreas(const int width);
};

class XR2LowerInstrumentPanel : public XR2InstrumentPanel
{
public:
    XR2LowerInstrumentPanel(XR2Ravenstar &vessel, const WORD panelResourceID) :
        XR2InstrumentPanel(vessel, PANEL_LOWER, panelResourceID)
    {
    }

    virtual bool Activate();

protected:
    void AddCommonAreas(const int width);
    void Add1600PlusAreas(const int width);
};

class XR2PayloadInstrumentPanel : public XR2InstrumentPanel
{
public:
    XR2PayloadInstrumentPanel(XR2Ravenstar &vessel, const WORD panelResourceID) :
        XR2InstrumentPanel(vessel, PANEL_PAYLOAD, panelResourceID)
    {
        AddCommonAreas();       // no shift for this panel
    }

    virtual bool Activate();
    virtual void Deactivate();

protected:
    void AddCommonAreas();
};

//----------------------------------------------------------------------
// 1280-pixel-wide panels
//----------------------------------------------------------------------

// Payload 2D 1280-pixel instrument panel
class XR2PayloadInstrumentPanel1280 : public XR2PayloadInstrumentPanel
{
public:
    XR2PayloadInstrumentPanel1280::XR2PayloadInstrumentPanel1280(XR2Ravenstar &vessel) : 
        XR2PayloadInstrumentPanel(vessel, IDB_PANEL4_1280)
    {
    }
};

// Main 2D 1280-pixel instrument panel
class XR2MainInstrumentPanel1280 : public XR2MainInstrumentPanel
{
public:
    XR2MainInstrumentPanel1280(XR2Ravenstar &vessel);
};

//----------------------------------------------------------------------

// Upper 2D 1280-pixel instrument panel
class XR2UpperInstrumentPanel1280 : public XR2UpperInstrumentPanel
{
public:
    XR2UpperInstrumentPanel1280(XR2Ravenstar &vessel);
};

//----------------------------------------------------------------------

// Lower 2D 1280-pixel instrument panel
class XR2LowerInstrumentPanel1280 : public XR2LowerInstrumentPanel
{
public:
    XR2LowerInstrumentPanel1280(XR2Ravenstar &vessel);
};

//----------------------------------------------------------------------
// 1600-pixel-wide panels
//----------------------------------------------------------------------

// Payload 2D 1600-pixel instrument panel
class XR2PayloadInstrumentPanel1600 : public XR2PayloadInstrumentPanel
{
public:
    XR2PayloadInstrumentPanel1600::XR2PayloadInstrumentPanel1600(XR2Ravenstar &vessel) : 
        XR2PayloadInstrumentPanel(vessel, IDB_PANEL4_1600)
    {
    }
};

// Main 2D 1600-pixel instrument panel
class XR2MainInstrumentPanel1600 : public XR2MainInstrumentPanel
{
public:
    XR2MainInstrumentPanel1600(XR2Ravenstar &vessel);
};

//----------------------------------------------------------------------

// Upper 2D 1600-pixel instrument panel
class XR2UpperInstrumentPanel1600 : public XR2UpperInstrumentPanel
{
public:
    XR2UpperInstrumentPanel1600(XR2Ravenstar &vessel);
};

//----------------------------------------------------------------------

// Lower 2D 1600-pixel instrument panel
class XR2LowerInstrumentPanel1600 : public XR2LowerInstrumentPanel
{
public:
    XR2LowerInstrumentPanel1600(XR2Ravenstar &vessel);
};

//----------------------------------------------------------------------
// 1920-pixel-wide panels
//----------------------------------------------------------------------

// Payload 2D 1600-pixel instrument panel
class XR2PayloadInstrumentPanel1920 : public XR2PayloadInstrumentPanel
{
public:
    XR2PayloadInstrumentPanel1920::XR2PayloadInstrumentPanel1920(XR2Ravenstar &vessel) : 
        XR2PayloadInstrumentPanel(vessel, IDB_PANEL4_1920)
    {
    }
};

// Main 2D 1920-pixel instrument panel
class XR2MainInstrumentPanel1920 : public XR2MainInstrumentPanel
{
public:
    XR2MainInstrumentPanel1920(XR2Ravenstar &vessel);
};

//----------------------------------------------------------------------

// Upper 2D 1920-pixel instrument panel
class XR2UpperInstrumentPanel1920 : public XR2UpperInstrumentPanel
{
public:
    XR2UpperInstrumentPanel1920(XR2Ravenstar &vessel);
};

//----------------------------------------------------------------------

// Lower 2D 1920-pixel instrument panel
class XR2LowerInstrumentPanel1920 : public XR2LowerInstrumentPanel
{
public:
    XR2LowerInstrumentPanel1920(XR2Ravenstar &vessel);
};

//----------------------------------------------------------------------

//
// Virtual Cockpit panels
//

// base class for all VC panels; this will initialize all the instruments in the VC
class XR2VCInstrumentPanel : public XR2InstrumentPanel
{
public:
    XR2VCInstrumentPanel(XR2Ravenstar &vessel, const int panelID);
    virtual bool Activate();
};

class XR2VCPilotInstrumentPanel : public XR2VCInstrumentPanel
{
public:
    // inline constructor
    XR2VCPilotInstrumentPanel(XR2Ravenstar &vessel, const int panelID) :
        XR2VCInstrumentPanel(vessel, panelID)
    {
    }

    virtual bool Activate();
};

class XR2VCCopilotInstrumentPanel : public XR2VCInstrumentPanel
{
public:
    // inline constructor
    XR2VCCopilotInstrumentPanel(XR2Ravenstar &vessel, const int panelID) :
        XR2VCInstrumentPanel(vessel, panelID)
    {
    }

    virtual bool Activate();
};

class XR2VCPassenger1InstrumentPanel : public XR2VCInstrumentPanel
{
public:
    // inline constructor
    XR2VCPassenger1InstrumentPanel(XR2Ravenstar &vessel, const int panelID) :
        XR2VCInstrumentPanel(vessel, panelID)
    {
    }

    virtual bool Activate();
};

class XR2VCPassenger2InstrumentPanel : public XR2VCInstrumentPanel
{
public:
    // inline constructor
    XR2VCPassenger2InstrumentPanel(XR2Ravenstar &vessel, const int panelID) :
        XR2VCInstrumentPanel(vessel, panelID)
    {
    }

    virtual bool Activate();
};

class XR2VCPassenger3InstrumentPanel : public XR2VCInstrumentPanel
{
public:
    // inline constructor
    XR2VCPassenger3InstrumentPanel(XR2Ravenstar &vessel, const int panelID) :
        XR2VCInstrumentPanel(vessel, panelID)
    {
    }

    virtual bool Activate();
};

class XR2VCPassenger4InstrumentPanel : public XR2VCInstrumentPanel
{
public:
    // inline constructor
    XR2VCPassenger4InstrumentPanel(XR2Ravenstar &vessel, const int panelID) :
        XR2VCInstrumentPanel(vessel, panelID)
    {
    }

    virtual bool Activate();
};

class XR2VCAirlockInstrumentPanel : public XR2VCInstrumentPanel
{
public:
    // inline constructor
    XR2VCAirlockInstrumentPanel(XR2Ravenstar &vessel, const int panelID) :
        XR2VCInstrumentPanel(vessel, panelID)
    {
    }

    virtual bool Activate();
};

class XR2VCPassenger5InstrumentPanel : public XR2VCInstrumentPanel
{
public:
    // inline constructor
    XR2VCPassenger5InstrumentPanel(XR2Ravenstar &vessel, const int panelID) :
        XR2VCInstrumentPanel(vessel, panelID)
    {
    }

    virtual bool Activate();
};

class XR2VCPassenger6InstrumentPanel : public XR2VCInstrumentPanel
{
public:
    // inline constructor
    XR2VCPassenger6InstrumentPanel(XR2Ravenstar &vessel, const int panelID) :
        XR2VCInstrumentPanel(vessel, panelID)
    {
    }

    virtual bool Activate();
};

class XR2VCPassenger7InstrumentPanel : public XR2VCInstrumentPanel
{
public:
    // inline constructor
    XR2VCPassenger7InstrumentPanel(XR2Ravenstar &vessel, const int panelID) :
        XR2VCInstrumentPanel(vessel, panelID)
    {
    }

    virtual bool Activate();
};

class XR2VCPassenger8InstrumentPanel : public XR2VCInstrumentPanel
{
public:
    // inline constructor
    XR2VCPassenger8InstrumentPanel(XR2Ravenstar &vessel, const int panelID) :
        XR2VCInstrumentPanel(vessel, panelID)
    {
    }

    virtual bool Activate();
};

class XR2VCPassenger9InstrumentPanel : public XR2VCInstrumentPanel
{
public:
    // inline constructor
    XR2VCPassenger9InstrumentPanel(XR2Ravenstar &vessel, const int panelID) :
        XR2VCInstrumentPanel(vessel, panelID)
    {
    }

    virtual bool Activate();
};

class XR2VCPassenger10InstrumentPanel : public XR2VCInstrumentPanel
{
public:
    // inline constructor
    XR2VCPassenger10InstrumentPanel(XR2Ravenstar &vessel, const int panelID) :
        XR2VCInstrumentPanel(vessel, panelID)
    {
    }

    virtual bool Activate();
};

class XR2VCPassenger11InstrumentPanel : public XR2VCInstrumentPanel
{
public:
    // inline constructor
    XR2VCPassenger11InstrumentPanel(XR2Ravenstar &vessel, const int panelID) :
        XR2VCInstrumentPanel(vessel, panelID)
    {
    }

    virtual bool Activate();
};

class XR2VCPassenger12InstrumentPanel : public XR2VCInstrumentPanel
{
public:
    // inline constructor
    XR2VCPassenger12InstrumentPanel(XR2Ravenstar &vessel, const int panelID) :
        XR2VCInstrumentPanel(vessel, panelID)
    {
    }

    virtual bool Activate();
};

