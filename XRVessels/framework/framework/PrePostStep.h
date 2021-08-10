// ==============================================================
// XR Vessel Framework
//
// Copyright 2006-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// Callbacks.h
// Class defining PreStep/PostStep objects, which are invoked from 
// clbkPreStep/clbkPostStep at each Orbiter timestep.
// ==============================================================

#pragma once

#include "Orbitersdk.h"
#include "vessel3ext.h"

// A prestep or a poststep class
class PrePostStep
{
public:
    PrePostStep(VESSEL3_EXT &vessel) : m_vessel(vessel) { } 
    VESSEL3_EXT &GetVessel() const { return m_vessel; }

    // subclass must implement this method
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd) = 0;
    
private:
    VESSEL3_EXT &m_vessel;
};
