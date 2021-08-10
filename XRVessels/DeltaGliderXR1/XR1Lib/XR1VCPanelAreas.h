// ==============================================================
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// Copyright 2006-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR1VCPanelAreas.h
// Handles non-component main panel areas
// ==============================================================

#pragma once

#include "Orbitersdk.h"
#include "vessel3ext.h"
#include "Area.h"
#include "XR1Areas.h"

//----------------------------------------------------------------------------------

class VCHudModeButtonArea : public XR1Area
{
public:
    // no redrawing here, so no meshTextureID required
    VCHudModeButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);  
    virtual void Activate();
    // VC-only: no Redraw2D for this area
    virtual bool Redraw3D(const int event, const SURFHANDLE surf);
    virtual bool ProcessVCMouseEvent(const int event, const VECTOR3 &coords);
};

//----------------------------------------------------------------------------------

class VCAutopilotButtonArea : public XR1Area
{
public:
    // no redrawing here, so no meshTextureID required
    VCAutopilotButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);
    virtual void Activate();
    // VC-only: no Redraw2D for this area
    virtual bool Redraw3D(const int event, const SURFHANDLE surf);
    virtual bool ProcessVCMouseEvent(const int event, const VECTOR3 &coords);
};

//----------------------------------------------------------------------------------

class VCToggleSwitchArea : public XR1Area
{
public:
    VCToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, void (DeltaGliderXR1::*pDoorHandler)(DoorStatus), const DoorStatus activatedStatus);
    virtual void Activate();
    virtual bool ProcessVCMouseEvent(const int event, const VECTOR3 &coords);

protected:
    // data
    DoorStatus m_activatedStatus;  // status to send pDoorHandler when activated
    void (DeltaGliderXR1::*m_pDoorHandler)(DoorStatus);  // handler to process door status for this switch

};

