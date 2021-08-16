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
    AddArea(new RotationalVelocityArea(parentPanel, GetAbsCoords(_COORD2(148, 16)), AID_VPITCH, AngularDataArea::Type::PITCH, meshTextureID));
    AddArea(new RotationalVelocityArea(parentPanel, GetAbsCoords(_COORD2( 80, 16)), AID_VBANK,  AngularDataArea::Type::BANK,  meshTextureID));
    AddArea(new RotationalVelocityArea(parentPanel, GetAbsCoords(_COORD2( 14, 16)), AID_VYAW,   AngularDataArea::Type::YAW,   meshTextureID));

    // Acceleration
    AddArea(new RotationalAccArea(parentPanel, GetAbsCoords(_COORD2(148, 89)), AID_APITCH, AngularDataArea::Type::PITCH,      meshTextureID));
    AddArea(new RotationalAccArea(parentPanel, GetAbsCoords(_COORD2( 80, 89)), AID_ABANK,  AngularDataArea::Type::BANK,       meshTextureID));
    AddArea(new RotationalAccArea(parentPanel, GetAbsCoords(_COORD2( 14, 89)), AID_AYAW,   AngularDataArea::Type::YAW,        meshTextureID));

    // Torque
    AddArea(new RotationalTorqueArea(parentPanel, GetAbsCoords(_COORD2(148,162)), AID_MPITCH, AngularDataArea::Type::PITCH,   meshTextureID));
    AddArea(new RotationalTorqueArea(parentPanel, GetAbsCoords(_COORD2( 80,162)), AID_MBANK,  AngularDataArea::Type::BANK,    meshTextureID));
    AddArea(new RotationalTorqueArea(parentPanel, GetAbsCoords(_COORD2( 14,162)), AID_MYAW,   AngularDataArea::Type::YAW,     meshTextureID));
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
    case Type::PITCH:
        sizeX = 40;
        sizeY = 49;
        surfaceID = IDB_VPITCH;
        break;

    case Type::BANK:
        sizeX = 50;
        sizeY = 40;
        surfaceID = IDB_VBANK;
        break;

    case Type::YAW:
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

    v  = (m_type == Type::PITCH ? -vrot.x : m_type == Type::BANK ? -vrot.z : vrot.y);
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
    case Type::PITCH:
        DeltaGliderXR1::SafeBlt(surf, m_mainSurface, 0, 0, idx * 40, 0, 40, 49); 
        break;

    case Type::BANK:
    case Type::YAW:
        DeltaGliderXR1::SafeBlt(surf, m_mainSurface, 0, 0, idx * 50, 0, 50, 40); 
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
    a  = (m_type == Type::PITCH ? -arot.x : m_type == Type::BANK ? -arot.z : arot.y);
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
        case Type::PITCH:
            DeltaGliderXR1::SafeBlt(surf, m_mainSurface, 0, 0, idx*40, 0, 40, 49); 
            break;

        case Type::BANK:
        case Type::YAW:
            DeltaGliderXR1::SafeBlt(surf, m_mainSurface, 0, 0, idx*50, 0, 50, 40); 
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
    m  = (m_type == Type::PITCH ? -amom.x : m_type == Type::BANK ? -amom.z : amom.y);
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
        case Type::PITCH:
            DeltaGliderXR1::SafeBlt(surf, m_mainSurface, 0, 0, idx*40, 0, 40, 49); 
            break;

        case Type::BANK:
        case Type::YAW:
            DeltaGliderXR1::SafeBlt(surf, m_mainSurface, 0, 0, idx*50, 0, 50, 40); 
            break;
    }
    return true;
}
