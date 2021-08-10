// ==============================================================
// XR Vessel Framework
//
// Copyright 2006-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
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
