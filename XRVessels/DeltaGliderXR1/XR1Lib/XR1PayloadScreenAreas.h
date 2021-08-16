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
// XR1PayloadScreenAreas.h
// Generic payload screen areas for all XR vessels;
// These are NOT used by the XR1! They are for sublcasses to use.
// ==============================================================

#pragma once

#include "XR1Areas.h"
#include "XRPayload.h"
#include "XR1Component.h"

// Note: SelectPayloadSlotArea is in each subclass that uses payload

//----------------------------------------------------------------------------------

class DeployPayloadArea : public XR1Area
{
public:
    DeployPayloadArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID,
                      const int idbDeployPayloadOrbit, const int idbDeployPayloadLanded);
    virtual void Activate();
    virtual void Deactivate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);

protected:
    static const COORD2 &s_screenSize;   // size of the screen in pixels

    int m_idbDeployPayloadOrbit;   // resource ID
    int m_idbDeployPayloadLanded;  // resource ID
    HFONT m_hFont;
    COORD2 m_deployButton;
    COORD2 m_deployAllButton;
    
    // Delta-V
    enum class RATE_ACTION { ACT_NONE, INCRATE1, DECRATE1, INCRATE5, DECRATE5, INCRATE25, DECRATE25 };
    COORD2 m_rateUp1ArrowCoord;     // actually 0.1
    COORD2 m_rateDown1ArrowCoord;
    COORD2 m_rateUp5ArrowCoord;
    COORD2 m_rateDown5ArrowCoord;
    COORD2 m_rateUp25ArrowCoord;
    COORD2 m_rateDown25ArrowCoord;
    COORD2 m_resetButtonCoord;
    double m_repeatSpeed;          // seconds between clicks if mouse held down
    double m_mouseHoldTargetSimt;  // simt at which next mouse click occurs
    RATE_ACTION m_lastAction;      // last rate change made
    int m_repeatCount;             // # of repeats this press (hold)

    SURFHANDLE m_hSurfaceForOrbit;  // surface loaded in ORBIT 
    SURFHANDLE m_hSurfaceForLanded; // surface loaded while LANDED
};

//----------------------------------------------------------------------------------

class PayloadThumbnailArea : public XR1Area
{
public:
    PayloadThumbnailArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID,
            const int idbPayloadThumbnailNone);
    virtual void Activate();
    virtual void Deactivate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);

protected:
    static const COORD2 &s_screenSize;   // size of the screen in pixels
    int m_idbPayloadThumbnailNone;  // resource ID

    SURFHANDLE m_hNoneSurface;
    const XRPayloadClassData *m_pLastRenderedPayloadThumbnailPCD;  // indicates which payload icon rendered on the screen
};

//----------------------------------------------------------------------------------

class GrapplePayloadArea : public XR1Area
{
public:
    GrapplePayloadArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int idbGrapplePayload);
    virtual void Activate();
    virtual void Deactivate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);

protected:
    static const COORD2 &s_screenSize;   // size of the screen in pixels

    int m_idbGrapplePayload;  // resource ID
    HFONT m_hFont;
    COORD2 m_grappleButton;
    COORD2 m_grappleAllButton;
    COORD2 m_rangeButton;
    COORD2 m_targetButtonUp;
    COORD2 m_targetButtonDown;
    COORD2 m_clearButton;
    // no way to do this: COORD2 m_trackButton;

    SURFHANDLE m_hSurface;  
};

//----------------------------------------------------------------------------------

class PayloadMassNumberArea : public MassNumberArea
{
public:
    PayloadMassNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const bool isMetric);
    
protected:
    virtual double GetMassInKG();
};

//----------------------------------------------------------------------------------

class PayloadMassDisplayComponent : public XR1Component
{
public:
    PayloadMassDisplayComponent(InstrumentPanel &parentPanel, COORD2 topLeft, const int lbAreaID, const int kgAreaID);
};

//----------------------------------------------------------------------------------

class PayloadEditorButtonArea : public MomentaryButtonArea
{
public:
    PayloadEditorButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual void ProcessButtonAction(int event, double buttonDownSimt);
};
