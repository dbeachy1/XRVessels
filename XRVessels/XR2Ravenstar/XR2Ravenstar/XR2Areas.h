// ==============================================================
// XR2Ravenstar implementation class
//
// Copyright 2008-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR2Areas.h
// Header for new areas for the XR2.
// ==============================================================

#pragma once

#include "orbitersdk.h"
#include "XR1Areas.h"
#include "XR1UpperPanelAreas.h"
#include "XR1MultiDisplayArea.h"

class XR2Ravenstar;

// define this here so it works in XR1-extended classes as well
// XR2 areas should extend XR1Area
#define GetXR2() (static_cast<XR2Ravenstar &>(GetVessel()))

//----------------------------------------------------------------------------------

class XR2ReentryCheckMultiDisplayMode : public ReentryCheckMultiDisplayMode
{
public:
    // Constructor
    XR2ReentryCheckMultiDisplayMode(int modeNumber);

    // Invoked by our parent's AddDisplayMode method immediately after we are attached to our parent MDA.
    // This is useful if an MDA needs to perform some one-time initialization.
    virtual void OnParentAttach();

protected:
    // subclass hooks
    virtual COORD2 GetStartingCoords()   { return _COORD2(85, 20); }  // first door status text line rendered here
    virtual COORD2 GetStatusLineCoords() { return _COORD2(80, 98); }  // "Reentry Check: ..."
    virtual int GetStartingCloseButtonYCoord() { return 23; }
    virtual int GetLinePitch() { return 11; }   // pitch between lines in pixels
    virtual int GetDoorCount() { return 7; }    // invoked by OnParentAttach
};

//----------------------------------------------------------------------------------

class XR2CrewDisplayArea : public CrewDisplayArea
{
public:
    XR2CrewDisplayArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);
};


