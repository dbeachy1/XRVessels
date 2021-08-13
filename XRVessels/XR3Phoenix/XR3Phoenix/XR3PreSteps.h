/**
  XR Vessel add-ons for OpenOrbiter Space Flight Simulator
  Copyright (C) 2006-2021 Douglas Beachy

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

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
// XR3PrePostSteps.h
// Class defining custom clbkPreStep callbacks for XR3 Phoenix.
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

