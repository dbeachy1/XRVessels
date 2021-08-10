// ==============================================================
// XR Vessel Framework
//
// Copyright 2006-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// Component.h
// Abstract base class defining a panel component on a 2D or 3D panel
// ==============================================================

#pragma once

#include "Orbitersdk.h"
#include "vessel3ext.h"

#include <vector>

using namespace std; 

// use forward references to avoid circular dependency
class InstrumentPanel;
class Area;

class Component
{
public:
    // Constructor
    Component(InstrumentPanel &parentPanel, const COORD2 topLeft, const int meshTextureID = VCPANEL_TEXTURE_NONE, const int screenMeshGroup = -1);
    virtual ~Component();

    InstrumentPanel &GetParentPanel() { return m_parentPanel; };
    COORD2 GetTopLeft() { return m_topLeft; };
    void AddArea(Area *pArea);
    bool IsVC() const;
    VESSEL3_EXT &GetVessel() const;
    const vector<Area *> &GetAreas() { return m_areaVector; }

protected:
    // methods
    COORD2 GetAbsCoords(const COORD2 relativeCoordinates);

    // data
    vector<Area *> m_areaVector;    // list of all areas on the component
    const int m_meshTextureID;      // arbitrary vessel-specific constant denoting which VC mesh texture on which this component's areas reside
    const int m_screenMeshGroup;    // mesh group ID from our parent vessel's mesh file

private:
    // data
    InstrumentPanel &m_parentPanel;
    COORD2 m_topLeft;     // coordinates of top-left corner of component
};
