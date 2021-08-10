// ==============================================================
// XR3Phoenix implementation class
//
// Copyright 2007-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR3PrePostSteps.h
// Class defining custom clbkPreStep callbacks for XR3 Vanguard.
// ==============================================================

#pragma once

#include "XR3Phoenix.h"
#include "XR3PrePostStep.h"

//---------------------------------------------------------------------------

#ifdef UNUSED
class SetCenterOfLiftPrePostStep : public XR3PrePostStep
{
public:
    SetCenterOfLiftPrePostStep(XR3Phoenix &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);
};
#endif

//---------------------------------------------------------------------------

class XR3NosewheelSteeringPreStep : public XR3PrePostStep
{
public:
    XR3NosewheelSteeringPreStep(XR3Phoenix &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    bool m_steeringActiveDuringPrevTimestep;  // true if steering was active during the previous timestep
};

