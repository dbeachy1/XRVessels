// ==============================================================
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// Copyright 2006-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR1UpperPanelComponents.cpp
// Handles upper panel components and associated areas.
// ==============================================================

#include "resource.h"
#include "AreaIDs.h"
#include "DeltaGliderXR1.h"

#include "XR1UpperPanelComponents.h"

//----------------------------------------------------------------------------------

// topLeft = top inside edge of frame, just on black screen
METTimerComponent::METTimerComponent(InstrumentPanel &parentPanel, COORD2 topLeft) :
    XR1Component(parentPanel, topLeft)
{
    DeltaGliderXR1 &xr1 = static_cast<DeltaGliderXR1 &>(GetVessel());

    AddArea(new MJDTimerNumberArea(parentPanel, GetAbsCoords(_COORD2(  2,  1)), AID_MET_DAYS,    xr1.m_metTimerRunning, 4, TimerNumberArea::DAYS,    xr1.m_metMJDStartingTime));
    AddArea(new MJDTimerNumberArea(parentPanel, GetAbsCoords(_COORD2( 58,  1)), AID_MET_HOURS,   xr1.m_metTimerRunning, 2, TimerNumberArea::HOURS,   xr1.m_metMJDStartingTime));
    AddArea(new MJDTimerNumberArea(parentPanel, GetAbsCoords(_COORD2( 77,  1)), AID_MET_MINUTES, xr1.m_metTimerRunning, 2, TimerNumberArea::MINUTES, xr1.m_metMJDStartingTime));
    AddArea(new MJDTimerNumberArea(parentPanel, GetAbsCoords(_COORD2( 96,  1)), AID_MET_SECONDS, xr1.m_metTimerRunning, 2, TimerNumberArea::SECONDS, xr1.m_metMJDStartingTime));
    AddArea(new METResetButtonArea(parentPanel, GetAbsCoords(_COORD2(125, -1)), AID_MET_RESETBUTTON));
}   

// topLeft = top inside edge of frame, just on black screen
Interval1TimerComponent::Interval1TimerComponent(InstrumentPanel &parentPanel, COORD2 topLeft) :
    XR1Component(parentPanel, topLeft)
{
    DeltaGliderXR1 &xr1 = static_cast<DeltaGliderXR1 &>(GetVessel());

    AddArea(new ElapsedTimerNumberArea (parentPanel, GetAbsCoords(_COORD2(  2,  1)), AID_INTERVAL1_DAYS,        xr1.m_interval1TimerRunning, 4, TimerNumberArea::DAYS,    xr1.m_interval1ElapsedTime));
    AddArea(new ElapsedTimerNumberArea (parentPanel, GetAbsCoords(_COORD2( 58,  1)), AID_INTERVAL1_HOURS,       xr1.m_interval1TimerRunning, 2, TimerNumberArea::HOURS,   xr1.m_interval1ElapsedTime));
    AddArea(new ElapsedTimerNumberArea (parentPanel, GetAbsCoords(_COORD2( 77,  1)), AID_INTERVAL1_MINUTES,     xr1.m_interval1TimerRunning, 2, TimerNumberArea::MINUTES, xr1.m_interval1ElapsedTime));
    AddArea(new ElapsedTimerNumberArea (parentPanel, GetAbsCoords(_COORD2( 96,  1)), AID_INTERVAL1_SECONDS,     xr1.m_interval1TimerRunning, 2, TimerNumberArea::SECONDS, xr1.m_interval1ElapsedTime));
    AddArea(new IntervalResetButtonArea(parentPanel, GetAbsCoords(_COORD2(125, -1)), AID_INTERVAL1_RESETBUTTON, xr1.m_interval1TimerRunning, xr1.m_interval1ElapsedTime, '1'));
}   

//----------------------------------------------------------------------------------
// Areas
//----------------------------------------------------------------------------------

METResetButtonArea::METResetButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    MomentaryButtonArea(parentPanel, panelCoordinates, areaID),
    m_buttonPressProcessed(false)
{
}

// buttonDownTime = simt button was initally pressed
void METResetButtonArea::ProcessButtonAction(int event, double buttonDownSimt)
{
    if (event & (PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBUP))
        GetXR1().PlaySound(GetXR1().SwitchOn, DeltaGliderXR1::ST_Other, QUIET_CLICK);  // light click for both on and off

    if (event & PANEL_MOUSE_LBDOWN)
        m_buttonPressProcessed = false;     // reset for this new press
    else if (m_buttonPressProcessed)
        return;     // ignore this event; button press already processed

    const double RESET_TIME = 2.5;      // button must be held this long to reset 
    const double buttonHoldTime = GetAbsoluteSimTime() - buttonDownSimt;

    if (event & PANEL_MOUSE_LBPRESSED)
    {
        // reset only works if grounded
        if (GetXR1().GroundContact())
        {
            if (buttonHoldTime >= RESET_TIME)
            {
                GetXR1().ResetMET();
                m_buttonPressProcessed = true;          // ignore any further events
            }
        }
        else    // not landed!
        {
            GetXR1().PlayErrorBeep();
            GetXR1().ShowWarning("Must be landed to reset MET.wav", DeltaGliderXR1::ST_WarningCallout, "Ship must be landed to reset&the MET timer.");
            m_buttonPressProcessed = true;          // ignore any further events
        }
    }
    else    // button was released before MET was reset
    {
        GetXR1().ShowWarning("Hold to Reset.wav", DeltaGliderXR1::ST_WarningCallout, "You must hold down the reset&button to reset the MET timer.");
        m_buttonPressProcessed = true;
    }
}

//----------------------------------------------------------------------------------

IntervalResetButtonArea::IntervalResetButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, bool &intervalTimerRunning, double &intervalStartingTime, const char timerNumberChar) :
    MomentaryButtonArea(parentPanel, panelCoordinates, areaID),
    m_buttonPressProcessed(false), m_intervalStartingTime(intervalStartingTime), m_intervalTimerRunning(intervalTimerRunning),
    m_timerNumberChar(timerNumberChar), m_disableTimerStartForThisClick(false)
{
}

// buttonDownTime = simt button was initally pressed
void IntervalResetButtonArea::ProcessButtonAction(int event, double buttonDownSimt)
{
    if (event & (PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBUP))
        GetXR1().PlaySound(GetXR1().SwitchOn, DeltaGliderXR1::ST_Other, QUIET_CLICK);  // light click for both on and off

    char temp[40];
    if (event & PANEL_MOUSE_LBDOWN)
    {
        m_buttonPressProcessed = false;     // reset for this new press

        if (m_intervalTimerRunning)
        {
            m_intervalTimerRunning = false;
            GetXR1().PlaySound(GetXR1().BeepLow, DeltaGliderXR1::ST_Other);
            sprintf(temp, "Interval Timer #%c stopped.", m_timerNumberChar);

            // NOTE: we want to allow the pilot to stop and reset the timer in a single click, so we can't just set m_buttonPressProcessed = true here.
            m_disableTimerStartForThisClick = true;     // allow reset but not timer start
        }
    }

    if (m_buttonPressProcessed)
        return;     // ignore this event; button press already processed

    // we want the timer to start when the mouse is RELEASED
    if (event & PANEL_MOUSE_LBUP)
    {
        if ((m_intervalTimerRunning == false) && (m_disableTimerStartForThisClick == false))
        {
            // start the timer
            m_intervalTimerRunning = true;
            GetXR1().PlaySound(GetXR1().BeepHigh, DeltaGliderXR1::ST_Other);
            sprintf(temp, "Interval Timer #%c started.", m_timerNumberChar);

            // if timer is currently reset, init timer to 0
            if (m_intervalStartingTime < 0)
                m_intervalStartingTime = 0;
        }

        m_disableTimerStartForThisClick = false;    // reset
    }

    const double RESET_TIME = 2.5;      // button must be held this long to reset 
    const double buttonHoldTime = GetAbsoluteSimTime() - buttonDownSimt;

    if (event & PANEL_MOUSE_LBPRESSED)
    {
        if (buttonHoldTime >= RESET_TIME)
        {
            sprintf(temp, "Interval Timer #%c reset.", m_timerNumberChar);
            GetXR1().ShowInfo("Interval Timer Reset.wav", DeltaGliderXR1::ST_InformationCallout, temp);

            m_intervalStartingTime = -1;     // reset timer
            m_intervalTimerRunning = false;  // not running now
            m_buttonPressProcessed = true;   // ignore any further events
            m_disableTimerStartForThisClick = false;    // must RESET this because no further events will be processed for this click
        }
    }
}

