// ==============================================================
// XR5Vanguard implementation class
//
// Copyright 2007-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR5PrePostSteps.cpp
// Class defining custom clbkPostStep callbacks for the XR5 Vanguard
// ==============================================================

#pragma once

#include "XR5Vanguard.h"
#include "XR5PrePostStep.h"
#include "XR1PostSteps.h"

//---------------------------------------------------------------------------

class XR5AnimationPostStep : public XR5PrePostStep
{
public:
    XR5AnimationPostStep(XR5Vanguard &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    void AnimateBayDoors    (const double simt, const double simdt, const double mjd);
    void AnimateElevator    (const double simt, const double simdt, const double mjd);
};

//---------------------------------------------------------------------------

// handles door opening/closing sounds
class XR5DoorSoundsPostStep : public DoorSoundsPostStep
{
public:
    XR5DoorSoundsPostStep(XR5Vanguard &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    DoorSound m_xr5doorSounds[2];   // custom doors
};

//---------------------------------------------------------------------------

// handles door opening/closing sounds
class HandleDockChangesForActiveAirlockPostStep : public XR5PrePostStep
{
public:
    HandleDockChangesForActiveAirlockPostStep(XR5Vanguard &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    bool m_wasDockedAtPreviousTimestep;  // true if we were docked during the previous timestep.
};

