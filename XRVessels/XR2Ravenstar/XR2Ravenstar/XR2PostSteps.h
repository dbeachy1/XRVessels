// ==============================================================
// XR2Ravenstar implementation class
//
// Copyright 2008-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR2PostSteps.cpp
// Class defining custom clbkPostStep callbacks for the XR2 Ravenstar
// ==============================================================

#pragma once

#include "XR2Ravenstar.h"
#include "XR2PrePostStep.h"
#include "XR1PostSteps.h"

//---------------------------------------------------------------------------

class XR2AnimationPostStep : public XR2PrePostStep
{
public:
    XR2AnimationPostStep(XR2Ravenstar &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    void AnimateBayDoors  (const double simt, const double simdt, const double mjd);
};

//---------------------------------------------------------------------------

// handles door opening/closing sounds
class XR2DoorSoundsPostStep : public DoorSoundsPostStep
{
public:
    XR2DoorSoundsPostStep(XR2Ravenstar &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    DoorSound m_doorSounds[1];   // custom doors
};

