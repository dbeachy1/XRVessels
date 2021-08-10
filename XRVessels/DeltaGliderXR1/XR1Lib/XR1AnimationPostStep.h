// ==============================================================
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// Copyright 2006-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR1AnimationPostStep.h
// Class containing animation poststep code.
// ==============================================================

#pragma once

#include "XR1PrePostStep.h"

class DeltaGliderXR1;

class AnimationPostStep : public XR1PrePostStep
{
public:
    AnimationPostStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    void AnimateLadder      (const double simt, const double simdt, const double mjd);
    void AnimateNosecone    (const double simt, const double simdt, const double mjd);
    void AnimateOuterAirlock(const double simt, const double simdt, const double mjd);
    void AnimateInnerAirlock(const double simt, const double simdt, const double mjd);
    void AnimateHatch       (const double simt, const double simdt, const double mjd);
    void AnimateRadiator    (const double simt, const double simdt, const double mjd);
    void AnimateRetroDoors  (const double simt, const double simdt, const double mjd);
    void AnimateHoverDoors  (const double simt, const double simdt, const double mjd);
    void AnimateScramDoors  (const double simt, const double simdt, const double mjd);
    void AnimateGear        (const double simt, const double simdt, const double mjd);
    void AnimateAirbrake    (const double simt, const double simdt, const double mjd);
    void ManageChamberPressure(const double simt, const double simdt, const double mjd);
    void AnimateFuelHatch   (const double simt, const double simdt, const double mjd);
    void AnimateLOXHatch    (const double simt, const double simdt, const double mjd);
};
