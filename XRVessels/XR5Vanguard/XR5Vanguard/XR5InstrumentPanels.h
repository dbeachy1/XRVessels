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
// XR5InstrumentPanels.cpp
// Custom instrument panels for the XR5
// ==============================================================

#pragma once

#include "Orbitersdk.h"
#include "InstrumentPanel.h"
#include "Area.h"
#include "XR5Vanguard.h"

//
// Class definitions
//

//----------------------------------------------------------------------

class XR5InstrumentPanel : public InstrumentPanel
{
public:
    XR5InstrumentPanel(XR5Vanguard &vessel, const int panelID, const WORD panelResourceID = -1);

    // convenience method to return our actual vessel object
    XR5Vanguard &GetXR5() const { return static_cast<XR5Vanguard &>(GetVessel()); }

    // methods shared among multiple instrument panels
    void InitMDA(MultiDisplayArea *pMDA);

private:
};

//----------------------------------------------------------------------
// Base classes for our different instrument panels; these classes contain
// code and data common for each panel regardless of its resolution.
//----------------------------------------------------------------------

class XR5MainInstrumentPanel : public XR5InstrumentPanel
{
public:
    XR5MainInstrumentPanel(XR5Vanguard &vessel, const WORD panelResourceID) :
        XR5InstrumentPanel(vessel, PANEL_MAIN, panelResourceID)
    {
    }

    virtual bool Activate();
    virtual void Deactivate();
};

class XR5OverheadInstrumentPanel : public XR5InstrumentPanel
{
public:
    XR5OverheadInstrumentPanel(XR5Vanguard &vessel, const WORD panelResourceID) :
        XR5InstrumentPanel(vessel, PANEL_OVERHEAD, panelResourceID)
    {
        AddCommonAreas();
    }

    virtual bool Activate();

private:
    void AddCommonAreas();  
};

class XR5UpperInstrumentPanel : public XR5InstrumentPanel
{
public:
    XR5UpperInstrumentPanel::XR5UpperInstrumentPanel(XR5Vanguard &vessel, const WORD panelResourceID) :
        XR5InstrumentPanel(vessel, PANEL_UPPER, panelResourceID)
    {
    }

    virtual bool Activate();

protected:
    void AddCommonAreas(const int width);
    void Add1600PlusAreas(const int width);
};

class XR5LowerInstrumentPanel : public XR5InstrumentPanel
{
public:
    XR5LowerInstrumentPanel(XR5Vanguard &vessel, const WORD panelResourceID) :
        XR5InstrumentPanel(vessel, PANEL_LOWER, panelResourceID)
    {
    }

    virtual bool Activate();

protected:
    void AddCommonAreas(const int width);
    void Add1600PlusAreas(const int width);
};

class XR5PayloadInstrumentPanel : public XR5InstrumentPanel
{
public:
    XR5PayloadInstrumentPanel(XR5Vanguard &vessel, const WORD panelResourceID) :
        XR5InstrumentPanel(vessel, PANEL_PAYLOAD, panelResourceID)
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
class XR5PayloadInstrumentPanel1280 : public XR5PayloadInstrumentPanel
{
public:
    XR5PayloadInstrumentPanel1280::XR5PayloadInstrumentPanel1280(XR5Vanguard &vessel) : 
        XR5PayloadInstrumentPanel(vessel, IDB_PANEL5_1280)
    {
    }
};

// Overhead 2D 1280-pixel instrument panel
class XR5OverheadInstrumentPanel1280 : public XR5OverheadInstrumentPanel
{
public:
    XR5OverheadInstrumentPanel1280::XR5OverheadInstrumentPanel1280(XR5Vanguard &vessel) : 
        XR5OverheadInstrumentPanel(vessel, IDB_PANEL4_1280)
    {
    }
};

// Main 2D 1280-pixel instrument panel
class XR5MainInstrumentPanel1280 : public XR5MainInstrumentPanel
{
public:
    XR5MainInstrumentPanel1280(XR5Vanguard &vessel);
};

//----------------------------------------------------------------------

// Upper 2D 1280-pixel instrument panel
class XR5UpperInstrumentPanel1280 : public XR5UpperInstrumentPanel
{
public:
    XR5UpperInstrumentPanel1280(XR5Vanguard &vessel);
};

//----------------------------------------------------------------------

// Lower 2D 1280-pixel instrument panel
class XR5LowerInstrumentPanel1280 : public XR5LowerInstrumentPanel
{
public:
    XR5LowerInstrumentPanel1280(XR5Vanguard &vessel);
};

//----------------------------------------------------------------------
// 1600-pixel-wide panels
//----------------------------------------------------------------------

// Payload 2D 1600-pixel instrument panel
class XR5PayloadInstrumentPanel1600 : public XR5PayloadInstrumentPanel
{
public:
    XR5PayloadInstrumentPanel1600::XR5PayloadInstrumentPanel1600(XR5Vanguard &vessel) : 
        XR5PayloadInstrumentPanel(vessel, IDB_PANEL5_1600)
    {
    }
};

// Overhead 2D 1600-pixel instrument panel
class XR5OverheadInstrumentPanel1600 : public XR5OverheadInstrumentPanel
{
public:
    XR5OverheadInstrumentPanel1600::XR5OverheadInstrumentPanel1600(XR5Vanguard &vessel) : 
        XR5OverheadInstrumentPanel(vessel, IDB_PANEL4_1600)
    {
    }
};


// Main 2D 1600-pixel instrument panel
class XR5MainInstrumentPanel1600 : public XR5MainInstrumentPanel
{
public:
    XR5MainInstrumentPanel1600(XR5Vanguard &vessel);
};

//----------------------------------------------------------------------

// Upper 2D 1600-pixel instrument panel
class XR5UpperInstrumentPanel1600 : public XR5UpperInstrumentPanel
{
public:
    XR5UpperInstrumentPanel1600(XR5Vanguard &vessel);
};

//----------------------------------------------------------------------

// Lower 2D 1600-pixel instrument panel
class XR5LowerInstrumentPanel1600 : public XR5LowerInstrumentPanel
{
public:
    XR5LowerInstrumentPanel1600(XR5Vanguard &vessel);
};

//----------------------------------------------------------------------
// 1920-pixel-wide panels
//----------------------------------------------------------------------

// Payload 2D 1600-pixel instrument panel
class XR5PayloadInstrumentPanel1920 : public XR5PayloadInstrumentPanel
{
public:
    XR5PayloadInstrumentPanel1920::XR5PayloadInstrumentPanel1920(XR5Vanguard &vessel) : 
        XR5PayloadInstrumentPanel(vessel, IDB_PANEL5_1920)
    {
    }
};

// Overhead 2D 1920-pixel instrument panel
class XR5OverheadInstrumentPanel1920 : public XR5OverheadInstrumentPanel
{
public:
    XR5OverheadInstrumentPanel1920::XR5OverheadInstrumentPanel1920(XR5Vanguard &vessel) : 
        XR5OverheadInstrumentPanel(vessel, IDB_PANEL4_1920)
    {
    }
};

// Main 2D 1920-pixel instrument panel
class XR5MainInstrumentPanel1920 : public XR5MainInstrumentPanel
{
public:
    XR5MainInstrumentPanel1920(XR5Vanguard &vessel);
};

//----------------------------------------------------------------------

// Upper 2D 1920-pixel instrument panel
class XR5UpperInstrumentPanel1920 : public XR5UpperInstrumentPanel
{
public:
    XR5UpperInstrumentPanel1920(XR5Vanguard &vessel);
};

//----------------------------------------------------------------------

// Lower 2D 1920-pixel instrument panel
class XR5LowerInstrumentPanel1920 : public XR5LowerInstrumentPanel
{
public:
    XR5LowerInstrumentPanel1920(XR5Vanguard &vessel);
};

