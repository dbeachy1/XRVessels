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
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// XR1MultiDisplayArea.h
// Area class that manages all MultiDisplayMode objects
// ==============================================================

#pragma once

#include "Orbitersdk.h"
#include "vessel3ext.h"
#include "Area.h"
#include "XR1Areas.h"
#include "RollingArray.h"

class MultiDisplayMode;
class DeltaGliderXR1;

// Multi-display mode IDs
#define MDMID_AIRSPEED_HOLD     0
#define MDMID_DESCENT_HOLD      1
#define MDMID_ATTITUDE_HOLD     2
#define MDMID_HULL_TEMPS        3
#define MDMID_SYSTEMS_STATUS1   4
#define MDMID_SYSTEMS_STATUS2   5
#define MDMID_SYSTEMS_STATUS3   6
#define MDMID_SYSTEMS_STATUS4   7
#define MDMID_SYSTEMS_STATUS5   8
#define MDMID_REENTRY_CHECK     9


#define DEFAULT_MMID        MDMID_HULL_TEMPS   // enabled if no other ID set

//----------------------------------------------------------------------------------

class MultiDisplayArea : public XR1Area
{
public:
    MultiDisplayArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);
    virtual ~MultiDisplayArea();
    virtual void Activate();
    virtual void Deactivate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);
    virtual bool ProcessVCMouseEvent(const int event, const VECTOR3 &coords);

    const COORD2 &GetScreenSize() { return m_screenSize; }
    MultiDisplayMode *AddDisplayMode(MultiDisplayMode *pMultiDisplayMode);
    bool SetActiveMode(int modeNumber);
    bool TurnOn();     // turn screen on and enable last active mode
    void TurnOff();    // turn screen off
    
    enum class DIRECTION { UP, DOWN };
    int SwitchActiveMode(DIRECTION dir);

    // Allow our MultiDisplayMode objects to create surfaces for our vessel.
    // We make our base class methods public here.
    SURFHANDLE CreateSurface(const int resourceID) const { return XR1Area::CreateSurface(resourceID); }
    void DestroySurface(SURFHANDLE *pSurfHandle) { XR1Area::DestroySurface(pSurfHandle); }
    
protected:
    COORD2 m_screenSize;
    bool m_screenBlanked;  // true if screen currently blanked via oapiBltPanelAreaBackground
    MultiDisplayMode *m_pActiveDisplayMode;
    unordered_map<int, MultiDisplayMode *> m_modeMap;    // map of all display modes: key = mode ID, value = MultiDisplayMode *

    // active area coordinates
    COORD2 m_nextButtonCoord;
    COORD2 m_prevButtonCoord;
};

//----------------------------------------------------------------------------------

// base class for all multi-display mode objects
class MultiDisplayMode
{
public:
    MultiDisplayMode(int modeNumber) : 
        m_modeNumber(modeNumber)  { }

    // gateway methods to parent XR1Area methods that the MDM objects need
    VESSEL2 &GetVessel() const { return m_pParentMDA->GetVessel(); }
    DeltaGliderXR1 &GetXR1() const { return m_pParentMDA->GetXR1(); }
    double GetAbsoluteSimTime() const { return GetXR1().GetAbsoluteSimTime(); }  // convenience method
    SURFHANDLE CreateSurface(const int resourceID) const { return m_pParentMDA->CreateSurface(resourceID); }
    void DestroySurface(SURFHANDLE *pSurfHandle) { m_pParentMDA->DestroySurface(pSurfHandle); }
    const COORD2 &GetScreenSize() const { return m_pParentMDA->GetScreenSize(); }
    COLORREF GetTempCREF(double tempK, double limitK, DoorStatus doorStatus) const { return m_pParentMDA->GetTempCREF(tempK, limitK, doorStatus); }
    COLORREF GetValueCREF(double value, double warningLimit, double criticalLimit) const { return m_pParentMDA->GetValueCREF(value, warningLimit, criticalLimit); }

    void SetParent(MultiDisplayArea *pParentMDA) { m_pParentMDA = pParentMDA; }
    int GetModeNumber() const { return m_modeNumber; }

    // Invoked by our parent's AddDisplayMode method immediately after we are attached to our parent MDA.
    // This is useful if an MDA needs to perform some one-time initialization.
    virtual void OnParentAttach() { }

    // These methods are invoked by our parent MultiDisplayArea object.
    // You must override at least one of these methods in order for your mode 
    // to do anything useful.
    virtual void Activate() { }  // NOTE: do not invoke oapiRegisterPanelArea in this method; it will not be invoked!
    virtual void Deactivate() { } 
    virtual bool Redraw2D(const int event, const SURFHANDLE surf) { return false;  }
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my) { return false; }
    virtual bool ProcessVCMouseEvent(const int event, const VECTOR3 &coords) { return false; }

protected:
    int m_modeNumber;           // 0-n; this is the absolute mode number
    MultiDisplayArea *m_pParentMDA;
};

//----------------------------------------------------------------------------------

class HullTempsMultiDisplayMode : public MultiDisplayMode
{
public:
    HullTempsMultiDisplayMode(int modeNumber);

    // These methods are invoked by our parent MultiDisplayArea object.
    virtual void Activate();
    virtual void Deactivate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);

protected:
    virtual double GetHighestTempFrac();
    
    // if DoorStatus::DOOR_OPEN, temperature values will be displayed in yellow or red correctly since that door is open
    virtual DoorStatus GetNoseDoorStatus();
    virtual DoorStatus GetLeftWingDoorStatus();
    virtual DoorStatus GetRightWingDoorStatus();
    virtual DoorStatus GetCockpitDoorStatus();
    virtual DoorStatus GetTopHullDoorStatus();

    void GetTemperatureStr(double tempK, char *pStrOut);
    void GetCoolantTemperatureStr(double tempC, char *pStrOut);
    SURFHANDLE m_backgroundSurface;   // main screen background
    SURFHANDLE m_indicatorSurface;
    COORD2 m_kfcButtonCoord;

    // fonts
    // Note: as of D3D9 RC23 there is no difference in framerate between sketchpad and GetDC on this 
    // MDA area. In addition, the font control isn't quite as precise under sketchpad (FF_MODERN fonts looks a lot
    // better in the MDA), so I am keeping the GetDC version for now.
#if 1  // GetDC
    HFONT m_pKfcFont;
    HFONT m_pCoolantFont;   // coolant temps only
#else  // Sketchpad
    oapi::Font *m_pKfcFont;
    oapi::Font *m_pCoolantFont;   // coolant temps only    
#endif
};

//----------------------------------------------------------------------------------

class SystemsStatusMultiDisplayMode : public MultiDisplayMode
{
public:
    SystemsStatusMultiDisplayMode(int modeNumber);

    // These methods are invoked by our parent MultiDisplayArea object.
    virtual void Activate();
    virtual void Deactivate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);

protected:
    SURFHANDLE m_backgroundSurface;   // main screen background
    int m_fontPitch;
    int m_screenIndex;    // 0-n; this is the status screen index

    // fonts
    HFONT m_mainFont;
};

//----------------------------------------------------------------------------------

class AttitudeHoldMultiDisplayMode : public MultiDisplayMode
{
public:
    AttitudeHoldMultiDisplayMode(int modeNumber);

    // These methods are invoked by our parent MultiDisplayArea object.
    virtual void Activate();
    virtual void Deactivate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);

protected:
    enum class AXIS_ACTION { ACT_NONE, INCPITCH_SMALL, DECPITCH_SMALL, INCPITCH_LARGE, DECPITCH_LARGE, INCBANK, DECBANK };

    SURFHANDLE m_backgroundSurface;   // main screen background
    COORD2 m_engageButtonCoord;
    
    COORD2 m_toggleAOAPitchCoord;
    COORD2 m_pitchUpArrowSmallCoord;
    COORD2 m_pitchDownArrowSmallCoord;
    COORD2 m_pitchUpArrowLargeCoord;
    COORD2 m_pitchDownArrowLargeCoord;
    
    COORD2 m_bankLeftArrowCoord;
    COORD2 m_bankRightArrowCoord;
    
    COORD2 m_resetBankButtonCoord;
    COORD2 m_resetPitchButtonCoord;
    COORD2 m_resetBothButtonCoord;

    COORD2 m_syncButtonCoord;

    double m_repeatSpeed;          // seconds between clicks if mouse held down
    double m_mouseHoldTargetSimt;  // simt at which next mouse click occurs
    AXIS_ACTION m_lastAction; // last axis change made
    int m_repeatCount;        // # of repeats this press (hold)
    
    // fonts
    HFONT m_statusFont;
    HFONT m_numberFont;
    HFONT m_buttonFont;    // engage/disengage button
    HFONT m_aoaPitchFont;
};

//----------------------------------------------------------------------------------

class DescentHoldMultiDisplayMode : public MultiDisplayMode
{
public:
    DescentHoldMultiDisplayMode(int modeNumber);

    // These methods are invoked by our parent MultiDisplayArea object.
    virtual void Activate();
    virtual void Deactivate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);

protected:
    enum class RATE_ACTION { ACT_NONE, INCRATE1, DECRATE1, INCRATE5, DECRATE5, INCRATE25, DECRATE25 };

    SURFHANDLE m_backgroundSurface;   // main screen background
    COORD2 m_engageButtonCoord;
    COORD2 m_rateUp1ArrowCoord;
    COORD2 m_rateDown1ArrowCoord;
    COORD2 m_rateUp5ArrowCoord;
    COORD2 m_rateDown5ArrowCoord;
    COORD2 m_rateUp25ArrowCoord;
    COORD2 m_rateDown25ArrowCoord;
    COORD2 m_hoverButtonCoord;
    COORD2 m_autoLandButtonCoord;
    double m_repeatSpeed;          // seconds between clicks if mouse held down
    double m_mouseHoldTargetSimt;  // simt at which next mouse click occurs
    RATE_ACTION m_lastAction;      // last rate change made
    int m_repeatCount;             // # of repeats this press (hold)
    
    // fonts
    HFONT m_statusFont;
    HFONT m_numberFont;
    HFONT m_buttonFont;    // engage/disengage button
};

//----------------------------------------------------------------------------------

class AirspeedHoldMultiDisplayMode : public MultiDisplayMode
{
public:
    AirspeedHoldMultiDisplayMode(int modeNumber);
    virtual ~AirspeedHoldMultiDisplayMode();

    // These methods are invoked by our parent MultiDisplayArea object.
    virtual void Activate();
    virtual void Deactivate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);

protected:
    enum class RATE_ACTION { ACT_NONE, INCRATEP1, DECRATEP1, INCRATE1, DECRATE1, INCRATE5, DECRATE5, INCRATE25, DECRATE25 };

    SURFHANDLE m_backgroundSurface;   // main screen background
    COORD2 m_engageButtonCoord;
    COORD2 m_rateUpP1ArrowCoord;
    COORD2 m_rateDownP1ArrowCoord;
    COORD2 m_rateUp1ArrowCoord;
    COORD2 m_rateDown1ArrowCoord;
    COORD2 m_rateUp5ArrowCoord;
    COORD2 m_rateDown5ArrowCoord;
    COORD2 m_rateUp25ArrowCoord;
    COORD2 m_rateDown25ArrowCoord;
    COORD2 m_holdCurrentButtonCoord;
    COORD2 m_resetButtonCoord;
    double m_repeatSpeed;          // seconds between clicks if mouse held down
    double m_mouseHoldTargetSimt;  // simt at which next mouse click occurs
    RATE_ACTION m_lastAction;      // last rate change made
    int m_repeatCount;             // # of repeats this press (hold)

    RollingArray *m_pMaxMainAccRollingArray;  // smooths out the jumpy ACC values computed from the Orbiter core's force vectors
    
    // fonts
    HFONT m_statusFont;
    HFONT m_numberFont;
    HFONT m_buttonFont;    // engage/disengage button
};

//----------------------------------------------------------------------------------

class ReentryCheckMultiDisplayMode : public MultiDisplayMode
{
public:
    // Constructor
    ReentryCheckMultiDisplayMode(int modeNumber);
    virtual ~ReentryCheckMultiDisplayMode();

    // These methods are invoked by our parent MultiDisplayArea object.
    virtual void Activate();
    virtual void Deactivate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);
    
    // Invoked by our parent's AddDisplayMode method immediately after we are attached to our parent MDA.
    // This is useful if an MDA needs to perform some one-time initialization.
    virtual void OnParentAttach();

protected:
    // inner class to handle each door
    class DoorInfo
    {
    public:
        // pDoorHandler is a function pointer to the door handler to be invoked with the new door status
        DoorInfo(const char *pOpen, const char *pClosed, const DoorStatus &doorStatus, const COORD2 &closeButtonCoords, void (DeltaGliderXR1::*pDoorHandler)(DoorStatus)) :
            m_pOpen(pOpen), m_pClosed (pClosed), m_doorStatus(doorStatus), m_closeButtonCoords(closeButtonCoords), // copy by value
            m_pDoorHandler(pDoorHandler)
        {
        }

        const char *m_pOpen;                // "Open", "Deployed", etc.
        const char *m_pClosed;              // "Closed", "Stowed", etc.
        const DoorStatus &m_doorStatus; 
        const COORD2 m_closeButtonCoords;
        // pDoorHandler is a function pointer to the door handler to be invoked with the new door status
        void (DeltaGliderXR1::*m_pDoorHandler)(DoorStatus);  

        // convenience methods
        bool IsNotClosed() const { return (m_doorStatus != DoorStatus::DOOR_CLOSED); }
    };

    void PlayStatusCallout(const int openDoorCount);

    SURFHANDLE m_backgroundSurface;     // main screen background
    DoorInfo **m_pDoorInfo;             // one per door on screen
    bool m_prevReentryCheckStatus;      // check from previous render; true = OK

    // subclass hooks
    virtual COORD2 GetStartingCoords()   { return _COORD2(85, 23); }  // text lines rendered here
    virtual COORD2 GetStatusLineCoords() { return _COORD2(80, 95); }  // "Reentry Check: ..."
    virtual int GetStartingCloseButtonYCoord() { return 26; }
    virtual int GetCloseButtonXCoord() { return 136; }
    virtual int GetLinePitch() { return 12; }   // pitch between lines in pixels
    virtual int GetDoorCount() { return 6; }    // invoked by OnParentAttach

    // fonts
    HFONT m_mainFont;
};
