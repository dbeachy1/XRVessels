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
// XRGrappleTargetVessel.h
// Header file defining a vessel targeted by a parent vessel for grappling
// ==============================================================

#pragma once

#include "XRPayload.h"
#include "RollingArray.h"

class VESSEL3_EXT;

class XRGrappleTargetVessel
{
public:
    XRGrappleTargetVessel(VESSEL &targetVessel, VESSEL3_EXT &parentVessel);
    virtual ~XRGrappleTargetVessel();

    bool Update();      // update the state data for this vessel; this should be invoked from your vessel's PreStep.  If this returns false, delete this object since it is invalid.

    VESSEL *GetTargetVessel() const { return m_pTargetVessel; } // NULL = "target invalid"; will never be null if IsStateDataValid() == true.
    OBJHANDLE GetTargetHandle() const { return m_hTargetHandle; }
    const XRPayloadClassData &GetTargetPCD() const { return *m_targetPCD; }
    double GetDeltaV() const        { return m_deltaV; }        // may by positive or negative
    double GetDistance() const      { return m_distance; }      // -1 = "unknown"
    bool IsStateDataValid() const   { return ((m_pTargetVessel != nullptr) && (m_lastComputedDeltaVSimt >= 0)); }   // invoke this before invoking Get* methods

    // operator overloading
    bool operator==(const XRGrappleTargetVessel &that) const { return (m_hTargetHandle == that.m_hTargetHandle); }  // vessel handles are unique

protected:
    VESSEL *m_pTargetVessel;
    OBJHANDLE m_hTargetHandle;
    VESSEL3_EXT &m_parentVessel;
    const XRPayloadClassData *m_targetPCD;
    bool m_prevRetVal;
    bool m_isLastComputedValid;  // true if m_lastComputed* values are valid
    double m_distance;          // distance at last timestep (i.e., last call to 'Update')
    double m_deltaV;            // computed deltaV over the last second or so
    double m_prevSimt;          // simt of last call to Update(); necessary to detect multiple calls per timestep
    
    double m_lastComputedDeltaVSimt;      // simt of timestep when m_prevDistance was last calculated (not necessarily the last frame!)
    double m_lastComputedDeltaVDistance;  // distance at timestep when m_prevDistance was last calculated (not necessarily the last frame!)

    // tracks the last n distances and times so we can smoothly update the display at 20 fps instead of just 5 fps (which would be the smallest single stable sample we could show without the value "jumping around" a bit)
    RollingArray *m_pDistanceRollingArray;
    RollingArray *m_pTimeRollingArray;
};