// ==============================================================
// XR3Phoenix implementation class
//
// Copyright 2007-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR3PayloadScreenAreas.h
// Header for new payload screen areas for the XR3.
// ==============================================================

#pragma once

#include "XR1PayloadScreenAreas.h"
#include "XR3Areas.h"

//----------------------------------------------------------------------------------

class SelectPayloadSlotArea : public XR1Area
{
public:
    SelectPayloadSlotArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);
    virtual void Activate();
    virtual void Deactivate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);

protected:
    static const COORD2 &s_screenSize;  // size of the screen in pixels
    static const COORD2 &s_blockSize;   // size of each block in pixels
    
    COORD2 m_levelButton; 
    SURFHANDLE m_hSurfaceForLevel[3];   // surface bitmap for each level
};
