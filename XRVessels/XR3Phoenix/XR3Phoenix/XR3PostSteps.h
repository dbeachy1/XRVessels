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
// XR3Phoenix implementation class
//
// XR3PrePostSteps.cpp
// Class defining custom clbkPostStep callbacks for the XR3 Phoenix
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

