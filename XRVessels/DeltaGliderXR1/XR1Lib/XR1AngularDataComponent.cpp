// ==============================================================
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// Copyright 2006-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR1AngularDataComponent.h
// Handles main, hover, and scram throttle controls
// ==============================================================

#include "DeltaGliderXR1.h"
#include "XR1AngularDataComponent.h"
#include "AreaIDs.h"

// topLeft = top-left corner @ inside edge of screen
AngularDataComponent::AngularDataComponent(InstrumentPanel &parentPanel, COORD2 topLeft, const int meshTextureID) :
    XR1Component(parentPanel, topLeft, meshTextureID)
{
    // Velocity
    AddArea(new RotationalVelocityArea(parentPanel, GetAbsCoords(_COORD2(148, 16)), AID_VPITCH, AngularDataArea::PITCH, meshTextureID));
    AddArea(new RotationalVelocityArea(parentPanel, GetAbsCoords(_COORD2( 80, 16)), AID_VBANK,  AngularDataArea::BANK,  meshTextureID));
    AddArea(new RotationalVelocityArea(parentPanel, GetAbsCoords(_COORD2( 14, 16)), AID_VYAW,   AngularDataArea::YAW,   meshTextureID));

    // Acceleration
    AddArea(new RotationalAccArea(parentPanel, GetAbsCoords(_COORD2(148, 89)), AID_APITCH, AngularDataArea::PITCH,      meshTextureID));
    AddArea(new RotationalAccArea(parentPanel, GetAbsCoords(_COORD2( 80, 89)), AID_ABANK,  AngularDataArea::BANK,       meshTextureID));
    AddArea(new RotationalAccArea(parentPanel, GetAbsCoords(_COORD2( 14, 89)), AID_AYAW,   AngularDataArea::YAW,        meshTextureID));

    // Torque
    AddArea(new RotationalTorqueArea(parentPanel, GetAbsCoords(_COORD2(148,162)), AID_MPITCH, AngularDataArea::PITCH,   meshTextureID));
    AddArea(new RotationalTorqueArea(parentPanel, GetAbsCoords(_COORD2( 80,162)), AID_MBANK,  AngularDataArea::BANK,    meshTextureID));
    AddArea(new RotationalTorqueArea(parentPanel, GetAbsCoords(_COORD2( 14,162)), AID_MYAW,   AngularDataArea::YAW,     meshTextureID));
}   

//-------------------------------------------------------------------------

// base class for all angular data areas
AngularDataArea::AngularDataArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const Type type, const int meshTextureID) :
    XR1Area(parentPanel, panelCoordinates, areaID, meshTextureID),
    m_type(type)
{
}

void AngularDataArea::Activate()
{
    Area::Activate();  // invoke superclass method

    int sizeX, sizeY;
    int surfaceID;
    
    switch (m_type)
    {
    case PITCH:
        sizeX = 40;
        sizeY = 49;
        surfaceID = IDB_VPITCH;
        break;

    case BANK:
        sizeX = 50;
        sizeY = 40;
        surfaceID = IDB_VBANK;
        break;

    case YAW:
        sizeX = 50;
        sizeY = 40;
        surfaceID = IDB_VYAW;
        break;

    default:
        throw "Unknown Type enum ID";
    }

    // load our source bitmap
    m_mainSurface = CreateSurface(surfaceID);

    if (IsVC()) // 3D panel?
    {
        oapiVCRegisterArea(GetAreaID(), GetRectForSize(sizeX, sizeY), PANEL_REDRAW_ALWAYS, PANEL_MOUSE_IGNORE, PANEL_MAP_NONE, GetVCPanelTextureHandle());
    }
    else    // 2D panel
    {
        oapiRegisterPanelArea(GetAreaID(), GetRectForSize(sizeX, sizeY), PANEL_REDRAW_ALWAYS, PANEL_MOUSE_IGNORE);
    }

    // force an initial repaint
    m_lastRenderedIndex = -1;
}

//-------------------------------------------------------------------------

RotationalVelocityArea::RotationalVelocityArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const Type type, const int meshTextureID) :
    AngularDataArea(parentPanel, panelCoordinates, areaID, type, meshTextureID)
{
}

bool RotationalVelocityArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    int idx;
    double v, av;
    VECTOR3 vrot;
    GetVessel().GetAngularVel(vrot);

    v  = (m_type == PITCH ? -vrot.x : m_type == BANK ? -vrot.z : vrot.y);
    av = fabs(v*DEG);

    if      (av <  1.0) idx = 0;
    else if (av < 11.0) idx = 1 + static_cast<int>((av-1.0)*0.4);
    else if (av < 45.0) idx = 5 + static_cast<int>((av-11.0)*3.0/34.0);
    else                idx = 8;
    if (v >= 0.0) idx  = 8-idx;
    else          idx += 8;

    if (idx == m_lastRenderedIndex) 
        return false;   // no change since previous frame

    m_lastRenderedIndex = idx;

    // render the surface
    switch (m_type)
    {
    case PITCH: 
        oapiBlt(surf, m_mainSurface, 0, 0, idx * 40, 0, 40, 49); 
        break;

    case BANK:  
    case YAW:   
        oapiBlt(surf, m_mainSurface, 0, 0, idx * 50, 0, 50, 40); 
        break;
    }

    return true;
}

//-------------------------------------------------------------------------

RotationalAccArea::RotationalAccArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const Type type, const int meshTextureID) :
    AngularDataArea(parentPanel, panelCoordinates, areaID, type, meshTextureID)
{
}

bool RotationalAccArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    int idx;
    double a, aa;
    VECTOR3 arot;
    GetVessel().GetAngularAcc(arot);
    a  = (m_type == PITCH ? -arot.x : m_type == BANK ? -arot.z : arot.y);
    a *= 2.0;
    aa = fabs(a*DEG);
    
    if      (aa <  1.0) idx = 0;
    else if (aa < 11.0) idx = 1 + static_cast<int>((aa-1.0)*0.4);
    else if (aa < 45.0) idx = 5 + static_cast<int>((aa-11.0)*3.0/34.0);
    else                idx = 8;
    if (a >= 0.0) idx  = 8-idx;
    else          idx += 8;
    
    if (idx == m_lastRenderedIndex) 
        return false;

    m_lastRenderedIndex = idx;
    
    switch (m_type)
    {
        case PITCH: 
            oapiBlt(surf, m_mainSurface, 0, 0, idx*40, 0, 40, 49); 
            break;

        case BANK:  
        case YAW:   
            oapiBlt(surf, m_mainSurface, 0, 0, idx*50, 0, 50, 40); 
            break;
    }
    return true;
}


//-------------------------------------------------------------------------

RotationalTorqueArea::RotationalTorqueArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const Type type, const int meshTextureID) :
    AngularDataArea(parentPanel, panelCoordinates, areaID, type, meshTextureID)
{
}

bool RotationalTorqueArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    int idx;
    double m, am;
    VECTOR3 amom;
    GetVessel().GetAngularMoment(amom);
    m  = (m_type == PITCH ? -amom.x : m_type == BANK ? -amom.z : amom.y);
    m *= 1e-3;
    am = fabs(m);
    
    if      (am <  1.0) idx = 0;
    else if (am < 11.0) idx = 1 + static_cast<int>(((am-1.0)*0.4));
    else if (am < 45.0) idx = 5 + static_cast<int>(((am-11.0)*3.0/34.0));
    else                idx = 8;
    if (m >= 0.0) idx  = 8-idx;
    else          idx += 8;
    
    if (idx == m_lastRenderedIndex) 
        return false;

    m_lastRenderedIndex = idx;

    switch (m_type) 
    {
        case PITCH:
            oapiBlt(surf, m_mainSurface, 0, 0, idx*40, 0, 40, 49); 
            break;

        case BANK:
        case YAW:   
            oapiBlt(surf, m_mainSurface, 0, 0, idx*50, 0, 50, 40); 
            break;
    }
    return true;
}
