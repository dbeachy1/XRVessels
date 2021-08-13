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
// XR1Templates.h
// Contains template classes, which are expensive to include everywhere.
// ==============================================================

#pragma once

// Utility class that is used to average values over a number of renders;
// typically only useful when updated each frame.
template<class T>
class Averager 
{
protected:
    const int m_bufferSize; // max # of samples in sample window
    int m_sampleCount;      // current # of samples in window: 0 <= n <= m_windowSize
    int m_sampleIndex;      // index to where the NEXT sample will be added to the sample buffer
    T *m_pSampleBuffer;     // array of values to be averaged

public:
    // Constructor
    // bufferSize = # of samples in the average buffer
    // NOTE: if bufferSize == 1, average will always be the last value set via AddSample
    Averager(const int bufferSize) :
        m_bufferSize(bufferSize), m_sampleCount(0), m_sampleIndex(0)
    {
        m_pSampleBuffer = new T[bufferSize];
    }

    // Destructor
    ~Averager()
    {
        delete[] m_pSampleBuffer;
    }

    // Add a sample to the buffer, overwriting the oldest value if necessary
    void AddSample(T value)
    {
        m_pSampleBuffer[m_sampleIndex] = value;

        // increment the sample index
        if (m_sampleIndex == m_bufferSize-1)
            m_sampleIndex = 0;  // wrap around
        else
            m_sampleIndex++;

        // update the sample count
        if (m_sampleCount < m_bufferSize)
            m_sampleCount++;
    }

    // Returns the MEAN of all samples in the buffer
    // Throws fatal error if no samples added yet.
    T Averager<T>::GetMean()
    {
        if (m_sampleCount == 0)
            throw "Averager.GetMean: no samples in buffer!";

        // add up all the samples in the buffer
        T sum = 0;
        for (int i=0; i < m_sampleCount; i++)
            sum += m_pSampleBuffer[i];

        // now divide it by the total number of samples
        T retVal = sum / (T)m_sampleCount;

        return retVal;
    }

    // Returns the MEDIAN of all samples in the buffer
    // Throws fatal error if no samples added yet.
    // WARNING: this is relatively expensive with a large sample count.
    T Averager<T>::GetMedian()
    {
        if (m_sampleCount == 0)
            throw "Averager.GetAverage: no samples in buffer!";

        // find the MEDIAN value via a bubble sort
        bool cont = true;
        while (cont)
        {
            cont = false;   // reset
            for (int i=0; i < m_sampleCount-1; i++)
            {
                if (m_pSampleBuffer[i] > m_pSampleBuffer[i+1])  
                {
                    // they're in the wrong order; swap them
                    T temp = m_pSampleBuffer[i];
                    m_pSampleBuffer[i] = m_pSampleBuffer[i+1];
                    m_pSampleBuffer[i+1] = temp;
                    cont = true;    // must loop again
                }
            }
        }

        // return the MEDIAN value
        T retVal = m_pSampleBuffer[(m_sampleCount/2)];

        return retVal;
    }

    // reset average window to empty
    void Reset() { m_sampleCount = m_sampleIndex = 0; }  
};

//----------------------------------------------------------------------------------

// global template utility method to free an iterator entry as well as it->First & it->Second pointer blocks
template <class MAP, class ITERATOR>
void EraseIteratorItemFirstSecond(MAP &map, ITERATOR &it)
{
    // WARNING: must erase the map entry *before* we free the *contents* of the hashmap object item (first & second).
    // Based on debugging, the hashmap code appears to allocate extra data in the it->first block, because if we free it 
    // *first*, we CTD inside the erase(it) call.
    const void *pFirst = it->first;     // e.g., string *
    const void *pSecond = it->second;   // e.g., XRGrappleTargetVessel *
    map.erase(it);
    delete pFirst;
    delete pSecond;
}

//----------------------------------------------------------------------------------

