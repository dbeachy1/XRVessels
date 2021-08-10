// ==============================================================
// XR3Phoenix implementation class
//
// Copyright 2007-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR3InstrumentPanels.cpp
// Custom instrument panels for the XR3
// ==============================================================

#pragma once

#include "Orbitersdk.h"
#include "InstrumentPanel.h"
#include "Area.h"
#include "XR3Phoenix.h"

//
// Class definitions
//

//----------------------------------------------------------------------

class XR3InstrumentPanel : public InstrumentPanel
{
public:
    XR3InstrumentPanel(XR3Phoenix &vessel, const int panelID, const WORD panelResourceID = -1);

    // convenience method to return our actual vessel object
    XR3Phoenix &GetXR3() const { return static_cast<XR3Phoenix &>(GetVessel()); }

    // methods shared among multiple instrument panels
    void InitMDA(MultiDisplayArea *pMDA);

private:
};

//----------------------------------------------------------------------
// Base classes for our different instrument panels; these classes contain
// code and data common for each panel regardless of its resolution.
//----------------------------------------------------------------------

class XR3MainInstrumentPanel : public XR3InstrumentPanel
{
public:
    XR3MainInstrumentPanel(XR3Phoenix &vessel, const WORD panelResourceID) :
        XR3InstrumentPanel(vessel, PANEL_MAIN, panelResourceID)
    {
    }

    virtual bool Activate();
    virtual void Deactivate();
};

class XR3OverheadInstrumentPanel : public XR3InstrumentPanel
{
public:
    XR3OverheadInstrumentPanel(XR3Phoenix &vessel, const WORD panelResourceID) :
        XR3InstrumentPanel(vessel, PANEL_OVERHEAD, panelResourceID)
    {
        AddCommonAreas();
    }

    virtual bool Activate();

private:
    void AddCommonAreas();  
};

class XR3UpperInstrumentPanel : public XR3InstrumentPanel
{
public:
    XR3UpperInstrumentPanel::XR3UpperInstrumentPanel(XR3Phoenix &vessel, const WORD panelResourceID) :
        XR3InstrumentPanel(vessel, PANEL_UPPER, panelResourceID)
    {
    }

    virtual bool Activate();

protected:
    void AddCommonAreas(const int width);
    void Add1600PlusAreas(const int width);
};

class XR3LowerInstrumentPanel : public XR3InstrumentPanel
{
public:
    XR3LowerInstrumentPanel(XR3Phoenix &vessel, const WORD panelResourceID) :
        XR3InstrumentPanel(vessel, PANEL_LOWER, panelResourceID)
    {
    }

    virtual bool Activate();

protected:
    void AddCommonAreas(const int width);
    void Add1600PlusAreas(const int width);
};

class XR3PayloadInstrumentPanel : public XR3InstrumentPanel
{
public:
    XR3PayloadInstrumentPanel(XR3Phoenix &vessel, const WORD panelResourceID) :
        XR3InstrumentPanel(vessel, PANEL_PAYLOAD, panelResourceID)
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
class XR3PayloadInstrumentPanel1280 : public XR3PayloadInstrumentPanel
{
public:
    XR3PayloadInstrumentPanel1280::XR3PayloadInstrumentPanel1280(XR3Phoenix &vessel) : 
        XR3PayloadInstrumentPanel(vessel, IDB_PANEL5_1280)
    {
    }
};

// Overhead 2D 1280-pixel instrument panel
class XR3OverheadInstrumentPanel1280 : public XR3OverheadInstrumentPanel
{
public:
    XR3OverheadInstrumentPanel1280::XR3OverheadInstrumentPanel1280(XR3Phoenix &vessel) : 
        XR3OverheadInstrumentPanel(vessel, IDB_PANEL4_1280)
    {
    }
};

// Main 2D 1280-pixel instrument panel
class XR3MainInstrumentPanel1280 : public XR3MainInstrumentPanel
{
public:
    XR3MainInstrumentPanel1280(XR3Phoenix &vessel);
};

//----------------------------------------------------------------------

// Upper 2D 1280-pixel instrument panel
class XR3UpperInstrumentPanel1280 : public XR3UpperInstrumentPanel
{
public:
    XR3UpperInstrumentPanel1280(XR3Phoenix &vessel);
};

//----------------------------------------------------------------------

// Lower 2D 1280-pixel instrument panel
class XR3LowerInstrumentPanel1280 : public XR3LowerInstrumentPanel
{
public:
    XR3LowerInstrumentPanel1280(XR3Phoenix &vessel);
};

//----------------------------------------------------------------------
// 1600-pixel-wide panels
//----------------------------------------------------------------------

// Payload 2D 1600-pixel instrument panel
class XR3PayloadInstrumentPanel1600 : public XR3PayloadInstrumentPanel
{
public:
    XR3PayloadInstrumentPanel1600::XR3PayloadInstrumentPanel1600(XR3Phoenix &vessel) : 
        XR3PayloadInstrumentPanel(vessel, IDB_PANEL5_1600)
    {
    }
};

// Overhead 2D 1600-pixel instrument panel
class XR3OverheadInstrumentPanel1600 : public XR3OverheadInstrumentPanel
{
public:
    XR3OverheadInstrumentPanel1600::XR3OverheadInstrumentPanel1600(XR3Phoenix &vessel) : 
        XR3OverheadInstrumentPanel(vessel, IDB_PANEL4_1600)
    {
    }
};


// Main 2D 1600-pixel instrument panel
class XR3MainInstrumentPanel1600 : public XR3MainInstrumentPanel
{
public:
    XR3MainInstrumentPanel1600(XR3Phoenix &vessel);
};

//----------------------------------------------------------------------

// Upper 2D 1600-pixel instrument panel
class XR3UpperInstrumentPanel1600 : public XR3UpperInstrumentPanel
{
public:
    XR3UpperInstrumentPanel1600(XR3Phoenix &vessel);
};

//----------------------------------------------------------------------

// Lower 2D 1600-pixel instrument panel
class XR3LowerInstrumentPanel1600 : public XR3LowerInstrumentPanel
{
public:
    XR3LowerInstrumentPanel1600(XR3Phoenix &vessel);
};

//----------------------------------------------------------------------
// 1920-pixel-wide panels
//----------------------------------------------------------------------

// Payload 2D 1600-pixel instrument panel
class XR3PayloadInstrumentPanel1920 : public XR3PayloadInstrumentPanel
{
public:
    XR3PayloadInstrumentPanel1920::XR3PayloadInstrumentPanel1920(XR3Phoenix &vessel) : 
        XR3PayloadInstrumentPanel(vessel, IDB_PANEL5_1920)
    {
    }
};

// Overhead 2D 1920-pixel instrument panel
class XR3OverheadInstrumentPanel1920 : public XR3OverheadInstrumentPanel
{
public:
    XR3OverheadInstrumentPanel1920::XR3OverheadInstrumentPanel1920(XR3Phoenix &vessel) : 
        XR3OverheadInstrumentPanel(vessel, IDB_PANEL4_1920)
    {
    }
};

// Main 2D 1920-pixel instrument panel
class XR3MainInstrumentPanel1920 : public XR3MainInstrumentPanel
{
public:
    XR3MainInstrumentPanel1920(XR3Phoenix &vessel);
};

//----------------------------------------------------------------------

// Upper 2D 1920-pixel instrument panel
class XR3UpperInstrumentPanel1920 : public XR3UpperInstrumentPanel
{
public:
    XR3UpperInstrumentPanel1920(XR3Phoenix &vessel);
};

//----------------------------------------------------------------------

// Lower 2D 1920-pixel instrument panel
class XR3LowerInstrumentPanel1920 : public XR3LowerInstrumentPanel
{
public:
    XR3LowerInstrumentPanel1920(XR3Phoenix &vessel);
};

