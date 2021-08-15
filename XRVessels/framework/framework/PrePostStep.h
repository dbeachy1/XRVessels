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
// Callbacks.h
// Class defining PreStep/PostStep objects, which are invoked from 
// clbkPreStep/clbkPostStep at each Orbiter timestep.
// ==============================================================

#pragma once

#include "Orbitersdk.h"
#include "vessel3ext.h"

// A prestep or a poststep class
class PrePostStep
{
public:
    PrePostStep(VESSEL3_EXT &vessel) : m_vessel(vessel) { } 
    VESSEL3_EXT &GetVessel() const { return m_vessel; }

    // subclass must implement this method
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd) = 0;
    
private:
    VESSEL3_EXT &m_vessel;
};
