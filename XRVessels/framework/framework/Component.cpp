// ==============================================================
// XR Vessel Framework
//
// Copyright 2006-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// InstrumentPanel.cpp
// Abstract base class defining a 2D instrument panel
// ==============================================================

#include "Component.h"
#include "InstrumentPanel.h"
#include "Area.h"

// Constructor
// parentPanel = parent instrument panel
// topLeft = coordinates of top-left corner of component
Component::Component(InstrumentPanel &parentPanel, const COORD2 topLeft, const int meshTextureID, const int screenMeshGroup) :
    m_parentPanel(parentPanel), m_topLeft(topLeft), m_meshTextureID(meshTextureID), m_screenMeshGroup(screenMeshGroup)
{
    // subclass will invoke AddArea(...) to add each area to this component
}

// Destructor
Component::~Component()
{
}

bool Component::IsVC() const
{
    return m_parentPanel.IsVC();  // is component in virtual cockpit?
}

COORD2 Component::GetAbsCoords(const COORD2 relativeCoordinates)
{ 
    int absX = m_topLeft.x + relativeCoordinates.x;
    int absY = m_topLeft.y + relativeCoordinates.y;

    return _COORD2( absX, absY );
}

// Add an area to our parent InstrumentPanel and track the area in our object as well
void Component::AddArea(Area *pArea)
{
    // inform the area that we are its parent component 
    pArea->SetParentComponent(this);

    // add to end of vector
    m_areaVector.push_back(pArea);  

    // add to parent instrument panel
    m_parentPanel.AddArea(pArea);
}

// don't need to remove areas we added: our parent InstrumentPanel will clean them up for us

// can't inline this in the header file because of circular dependency
VESSEL3_EXT &Component::GetVessel() const
{
    return m_parentPanel.GetVessel();
}
