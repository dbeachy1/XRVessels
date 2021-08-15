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
// XRPayload.cpp
// Defines a vessel targeted by a parent vessel for grappling.
// ==============================================================

#include "XRGrappleTargetVessel.h"
#include "vessel3ext.h"

// Constructor
XRGrappleTargetVessel::XRGrappleTargetVessel(VESSEL &targetVessel, VESSEL3_EXT &parentVessel) :
    m_pTargetVessel(&targetVessel), m_parentVessel(parentVessel), m_prevSimt(-1),
    m_deltaV(0), m_distance(0), m_prevRetVal(true), m_lastComputedDeltaVSimt(-1), m_lastComputedDeltaVDistance(-1),
    m_isLastComputedValid(false)
{
    m_hTargetHandle = m_pTargetVessel->GetHandle();
    m_targetPCD = &XRPayloadClassData::GetXRPayloadClassDataForClassname(m_pTargetVessel->GetClassName());  // this will never change over the vessel's life

    // Note: don't make the sample size too high, or the values may "lag" a bit when delta-V or distance changes abruptly:
    // e.g., 30 samples / 60 samples-per-second = displayed rolling average is over the last 0.5 second
    const int rollingAvgSampleSize = 30;
    m_pDistanceRollingArray = new RollingArray(rollingAvgSampleSize);
    m_pTimeRollingArray = new RollingArray(rollingAvgSampleSize);
}

// Destructor
XRGrappleTargetVessel::~XRGrappleTargetVessel()
{
    delete m_pDistanceRollingArray;
    delete m_pTimeRollingArray;
}

// Update the state data for this vessel.  NOTE: you MUST call this at least *twice* across separate frames before the state data is valid.
//
// Returns: true on success, false if target vessel no longer exists.  If this happens, the caller should delete this object
//          since it will no longer return useful data.
bool XRGrappleTargetVessel::Update()
{
    // NOTE: it is possible that the subclass called this method more than once in a given frame; therefore,
    // we need to detect that here for two reasons 1) the deltaV is computed as NaN for this frame, and 
    // 2) it is inefficient to compute it twice in a single frame.
    const double simt = m_parentVessel.GetAbsoluteSimTime();
    if (simt == m_prevSimt)   // will never be true for the first frame, and we are OK after two or more frames
        return m_prevRetVal;

    bool retVal = false;    // assume failure

    // check whether the target vessel still exists
    if (oapiIsVessel(m_hTargetHandle))
    {
        // target vessel is still valid
        retVal = true;

        const VESSEL *pTargetVessel = GetTargetVessel();    // will never be null
        const double distance = m_parentVessel.GetDistanceToVessel(*m_pTargetVessel);    // will never be negative

        //
        // NEW for XR1 1.9 release group: show rolling averages of the last n entries (see constructor) for both delta-V and distance
        // so that the display can update at 60 fps (0.0167 delta below) instead of just 5 fps.
        //

        // initialize the 'last computed' distance and simt values on startup (the first frame)
        if (!m_isLastComputedValid)
        {
            m_lastComputedDeltaVDistance = distance;
            m_lastComputedDeltaVSimt = simt;
            m_isLastComputedValid = true;
        }

        // OLD: update deltaV and distance every 1/5th-second: anything smaller creates jump values b/c the sample is so tiny that rounding errors creep in
        // NEW: update deltaV and distance every 1/60th-second since we use a one-second rolling average now
        // NOTE: we don't want to add a sample every frame here because it would make the number of samples
        // over time vary, which would make accuracy (and lag) dependent on the framerate.  So we sync at 60 fps instead.
        const double timeDelta = (simt - m_lastComputedDeltaVSimt);  // will never be negative
        if (timeDelta >= 0.0167)   // 1/60th-second
        {
            // distance is always up-to-date (it won't jump around like delta-V does with small time steps, since delta-V involves distance over *time*)
            m_distance = m_parentVessel.GetDistanceToVessel(*m_pTargetVessel);    // will never be negative
            const double distanceDelta = m_distance - m_lastComputedDeltaVDistance; 
            
            // add new distance and elapsed time samples to the rolling arrays (oldest sample in each is bumped out) so we can calculate delta-V later
            m_pDistanceRollingArray->AddSample(distanceDelta);
            m_pTimeRollingArray->AddSample(timeDelta);

            // save last computed values (the current values!)
            m_lastComputedDeltaVDistance = m_distance;
            m_lastComputedDeltaVSimt = simt;
        }
        
        //
        // Update delta-V using sample data over the last N seconds
        //

        // if both parent and target landed, lock delta-V to zero
        // Note: this assumes that the vessel and the payload are landed on the same body (which is higly likely since the payload is 
        // in range), so it's not worth handling the other case.  We don't want delta-V jumping around when both are landed.
        if (m_parentVessel.IsLanded() && pTargetVessel->GroundContact())
        {
            m_deltaV = 0.0;
        }
        else
        {
            m_deltaV = m_pDistanceRollingArray->GetSum() / m_pTimeRollingArray->GetSum();  // total distance / total time (meters / seconds)
        }
    }
    else    // target deleted!
    {
        // reset state data to "unknown/invalid"
        m_pTargetVessel = nullptr;
        m_deltaV = 0;
        m_distance = -1;
    }

    // needed if we are called more than once per frame
    m_prevSimt = simt;
    m_prevRetVal = retVal;      

    return retVal;
}
