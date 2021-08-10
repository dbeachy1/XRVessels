// ==============================================================
// XR5Vanguard implementation class
//
// Copyright 2007-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR5PrePostSteps.h
// Class defining custom clbkPreStep callbacks for XR5 Vanguard.
// ==============================================================

#pragma once

#include "XR5Vanguard.h"
#include "XR5PrePostStep.h"

//---------------------------------------------------------------------------

#ifdef UNUSED
class SetCenterOfLiftPrePostStep : public XR5PrePostStep
{
public:
    SetCenterOfLiftPrePostStep(XR5Vanguard &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);
};
#endif

//---------------------------------------------------------------------------

class XR5NosewheelSteeringPreStep : public XR5PrePostStep
{
public:
    XR5NosewheelSteeringPreStep(XR5Vanguard &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    bool m_steeringActiveDuringPrevTimestep;  // true if steering was active during the previous timestep
};

