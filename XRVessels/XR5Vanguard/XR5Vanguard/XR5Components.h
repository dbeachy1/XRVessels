// ==============================================================
// XR5Vanguard implementation class
//
// Copyright 2006-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR5Components.h
// XR5Vanguard components.
// ==============================================================

#pragma once

#include "orbitersdk.h"
#include "XR1Component.h"
#include "XR5Areas.h"
#include "XR1Areas.h"
#include "XR1MultiDisplayArea.h"

// 
// Components
//

class XR5WarningLightsComponent : public XR1Component
{
public:
    XR5WarningLightsComponent(InstrumentPanel &instrumentPanel, COORD2 topLeft);
};

class XR5ActiveEVAPortComponent : public XR1Component
{
public:
    XR5ActiveEVAPortComponent(InstrumentPanel &instrumentPanel, COORD2 topLeft);
};

//
// Areas begin here
//

//----------------------------------------------------------------------------------

class XR5MWSTestButtonArea : public MomentaryButtonArea
{
public:
    XR5MWSTestButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual void ProcessButtonAction(int event, double buttonDownSimt);
};

//----------------------------------------------------------------------------------

class XR5WarningLightsArea : public XR1Area
{
public:
    XR5WarningLightsArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);

protected:
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

    // state data
    bool m_lightStateOn;  // true if light state (during blink) is ON
};


//----------------------------------------------------------------------------------
// our custom hull temps multi-display mode
class XR5HullTempsMultiDisplayMode : public HullTempsMultiDisplayMode
{
public:
    XR5HullTempsMultiDisplayMode(int modeNumber);

protected:
    virtual double GetHighestTempFrac();

    // if DOOR_OPEN, temperature values will be displayed in yellow or red correctly since that door is open
    virtual DoorStatus GetNoseDoorStatus();
    virtual DoorStatus GetLeftWingDoorStatus();
    virtual DoorStatus GetRightWingDoorStatus();
    virtual DoorStatus GetTopHullDoorStatus();
};

//----------------------------------------------------------------------------------

class DockingPortActiveLEDArea : public XR1Area
{
public:
    DockingPortActiveLEDArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
};

//----------------------------------------------------------------------------------

class CrewElevatorActiveLEDArea : public XR1Area
{
public:
    CrewElevatorActiveLEDArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
};

//----------------------------------------------------------------------------------

class ActiveEVAPortSwitchArea : public HorizontalCenteringRockerSwitchArea
{
public:
    ActiveEVAPortSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual void ProcessSwitchEvent(SWITCHES switches, POSITION position);
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
};

