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
// XR Vessel Framework
//
// AreaGroup.h
// Abstract base class to manage a group of areas
// ==============================================================

#pragma once

#include <unordered_map>

using namespace std;
using namespace stdext;

// must use forward reference here to avoid circular dependencies
class Area;

class AreaGroup
{
public:
    AreaGroup();
    virtual ~AreaGroup();

    // returns map of all Areas in this group
    unordered_map<int, Area *> &GetAreaMap() { return m_areaMap; };

    Area *AddArea(Area *pArea);
    void ActivateAllAreas();
    void DeactivateAllAreas();
    Area *GetArea(const int areaID);
    
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

private:
    // data
    unordered_map<int, Area *> m_areaMap;    // map of all areas in this group: key = area ID, value = Area *
};
