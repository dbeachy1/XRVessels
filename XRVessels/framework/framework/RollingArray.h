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
// RollingArray.h
// Utility class that manages a rolling array of double values
// ==============================================================

#pragma once

#include <crtdbg.h>   // for _ASSERTE

class RollingArray
{
public:
    // Constructor
    RollingArray(const int maxSampleCount) : m_maxSampleCount(maxSampleCount), m_sampleIndex(0), m_sampleCount(0)
    {
        m_pSampleArray = new double[maxSampleCount];
    }

    // Destructor
    virtual ~RollingArray()
    {
        delete []m_pSampleArray;
    }

    // Add a new sample data point
    void AddSample(const double value)
    {
        m_pSampleArray[m_sampleIndex] = value;
        
        if (m_sampleCount < m_maxSampleCount)  // sampleCount is 0 -> m_maxSampleCount, inclusive
            m_sampleCount++;        // still filling the array

        if (++m_sampleIndex >= m_maxSampleCount)
            m_sampleIndex = 0;      // wrap around
    }

    // Returns the rolling average value of all data points in the buffer
    double GetAverage() const
    {
        const int sampleCount = GetSampleCount();
        if (sampleCount == 0)   // no data yet?
        {
            // this is likely a program bug!!
            _ASSERTE(false);
            return 0;  // try to continue
        }

        return GetSum() / sampleCount;
    }

    // Returns the newest sample in the array
    double GetNewest() const
    {
        if (GetSampleCount() < 1)
        {
            _ASSERTE(false);  // program bug!  no data in array yet
            return 0;   // try to continue
        }

        // locate the last value added (m_sampleIndex - 1)
        // we sampleIndex == 0 here, it means we wrapped around because we know we have at least one sample
        int idx = ((m_sampleIndex == 0) ? (GetSampleCount() - 1) : (m_sampleIndex - 1));
        return m_pSampleArray[idx];
    }

    // Returns the oldest sample in the array
    double GetOldest() const
    {
        if (GetSampleCount() < 1)
        {
            _ASSERTE(false);  // program bug!  no data in array yet
            return 0;   // try to continue
        }

        // The oldest value added is sitting at m_sampleIndex, which always points to the array entry where the 
        // next value will be added.
        return m_pSampleArray[m_sampleIndex];
    }

    // Returns the number of data points in the buffer
    // (Will start at zero and grow to m_maxSampleCount, where it will stay from then on.)
    int GetSampleCount() const
    {
        return m_sampleCount;
    }

    // Returns the sum of all data points in the buffer
    double GetSum() const
    {
        double sum = 0;
        for (int i=0; i < GetSampleCount(); i++)
            sum += m_pSampleArray[i];
        return sum;
    }

    // Resets the sample array to empty
    void Clear()
    {
        m_sampleIndex = m_sampleCount = 0;
    }

private:
    int m_maxSampleCount;   // maximum # of samples in this array
    int m_sampleIndex;      // index into NEXT FREE ENTRY in m_sampleArray (i.e., entry that will be overwritten next)
    int m_sampleCount;      // total # of samples in the array so far; grows on startup from 0 -> m_maxSampleCount, then stays there
    double *m_pSampleArray; // size 'm_maxSampleCount'

};