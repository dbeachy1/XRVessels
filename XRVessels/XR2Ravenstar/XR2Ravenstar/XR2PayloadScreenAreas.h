// ==============================================================
// XR2Ravenstar implementation class
//
// Copyright 2008-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR2PayloadScreenAreas.h
// Header for new payload screen areas for the XR2.
// ==============================================================

#pragma once

#include "XR1PayloadScreenAreas.h"
#include "XR2Areas.h"

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
    static const COORD2 &s_blockOneSize;
    static const COORD2 &s_blockTwoAndThreeSize;
    SURFHANDLE m_hSurface;   // surface bitmap for each level
};

