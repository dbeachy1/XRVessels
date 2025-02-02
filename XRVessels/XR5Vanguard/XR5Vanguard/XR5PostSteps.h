/**
  XR Vessel add-ons for OpenOrbiter Space Flight Simulator
  Copyright (C) 2006-2025 Douglas Beachy

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
// XR5Vanguard implementation class
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

