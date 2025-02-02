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
// XR2Ravenstar implementation class
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

