/**
  XR Vessel add-ons for OpenOrbiter Space Flight Simulator
  Copyright (C) 2006-2021 Douglas Beachy

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.

  Email: mailto:doug.beachy@outlook.com
  Web: https://www.alteaaerospace.com
**/

// ==============================================================
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
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
