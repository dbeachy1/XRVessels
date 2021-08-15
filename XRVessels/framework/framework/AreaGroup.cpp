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
// AreaGroup.cpp
// Class to manage a group of areas; used as a base class
// ==============================================================

#include "OrbiterSDK.h"
#include "AreaGroup.h"
#include "Area.h"

// Constructor
AreaGroup::AreaGroup()
{
}

// Destructor
AreaGroup::~AreaGroup()
{
    // free all areas in the map so our subclass won't have to
    unordered_map<int, Area *>::const_iterator it = m_areaMap.begin();   // iterates over values
    for (; it != m_areaMap.end(); it++)
    {
        Area *pArea = it->second;      // get next area value in the map
        delete pArea;
    }
}

// Add a new area to this area group
// pArea = new area object
// Returns: pArea
Area *AreaGroup::AddArea(Area *pArea)
{
    typedef pair<int, Area *> Int_Area_Pair;
    int areaID = pArea->GetAreaID();
    m_areaMap.insert(Int_Area_Pair(areaID, pArea));  // key = area ID, value = Area *

    return pArea;
}

void AreaGroup::ActivateAllAreas()
{
    // loop through each area and activate it
    unordered_map<int, Area *>::const_iterator it = m_areaMap.begin();   // iterates over values
    for (; it != m_areaMap.end(); it++)
    {
        Area *pArea = it->second;      // get next area value in the map
        pArea->Activate();
    }
}

void AreaGroup::DeactivateAllAreas()
{
    // loop through each area and deactivate it
    unordered_map<int, Area *>::const_iterator it = m_areaMap.begin();   // iterates over values
    for (; it != m_areaMap.end(); it++)
    {
        Area *pArea = it->second;      // get next area value in the map
        pArea->Deactivate();
    }
}

// Retrieve an area with the supplied area ID.
// Returns: Area object if found, or NULL if area with the supplied ID does not exist in this area group
Area *AreaGroup::GetArea(const int areaID)
{
    Area *retVal = nullptr;    // assume not found
    unordered_map<int, Area *>::const_iterator it = m_areaMap.find(areaID);
    
    if (it != m_areaMap.end())
        retVal = it->second;     // found a matching area

    return retVal;
}

// Invoke each area's clbkPostStep callback method, if any
void AreaGroup::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // loop through each area
    unordered_map<int, Area *>::const_iterator it = m_areaMap.begin();   // iterates over values
    for (; it != m_areaMap.end(); it++)
    {
        Area *pArea = it->second;               // get next area value in the map
        pArea->clbkPrePostStep(simt, simdt, mjd);  // default handler for each area is empty
    }
}

