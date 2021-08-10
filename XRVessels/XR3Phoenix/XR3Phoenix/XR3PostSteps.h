// ==============================================================
// XR3Phoenix implementation class
//
// Copyright 2007-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR3PrePostSteps.cpp
// Class defining custom clbkPostStep callbacks for the XR3 Vanguard
// ==============================================================

#pragma once

#include "XR3Phoenix.h"
#include "XR3PrePostStep.h"
#include "XR1PostSteps.h"

//---------------------------------------------------------------------------

class XR3AnimationPostStep : public XR3PrePostStep
{
public:
    XR3AnimationPostStep(XR3Phoenix &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    void AnimateBayDoors    (const double simt, const double simdt, const double mjd);
    void AnimateElevator    (const double simt, const double simdt, const double mjd);
};

//---------------------------------------------------------------------------

// handles door opening/closing sounds
class XR3DoorSoundsPostStep : public DoorSoundsPostStep
{
public:
    XR3DoorSoundsPostStep(XR3Phoenix &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    DoorSound m_XR3doorSounds[2];   // custom doors
};

//---------------------------------------------------------------------------

// handles door opening/closing sounds
class HandleDockChangesForActiveAirlockPostStep : public XR3PrePostStep
{
public:
    HandleDockChangesForActiveAirlockPostStep(XR3Phoenix &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    bool m_wasDockedAtPreviousTimestep;  // true if we were docked during the previous timestep.
};

