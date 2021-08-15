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
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// XR1PrePostSteps.h
// Class defining custom clbkPostStep callbacks for the DG-XR1
// ==============================================================

#pragma once

#include "DeltaGliderXR1.h"
#include "XR1PrePostStep.h"
#include "RollingArray.h"

//---------------------------------------------------------------------------

class ShowWarningPostStep : public XR1PrePostStep
{
public:
    ShowWarningPostStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);
    
protected:
    bool m_performedStartupCheck;
    bool m_warningSoundPlayingPreviousStep; // true if warning WAV was playing during the previous timestep
    char m_lastWarningWavFilename[256]; // last WAV file played
    double m_minimumRepeatSimt; // minimum time before a repeat warning can be played
};

//---------------------------------------------------------------------------

class ComputeAccPostStep : public XR1PrePostStep
{
public:
    ComputeAccPostStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    AccScale m_activeGaugeScale;
    double m_gaugeScaleExpiration;        // time after which gauge scale may be reduced
    double m_peakAccOnCurrentGaugeScale;  // max acc of any axis on the current gauge
};

//---------------------------------------------------------------------------

class SetHullTempsPostStep : public XR1PrePostStep
{
public:
    SetHullTempsPostStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    void RemoveSurfaceHeat(const double simdt, double &temp);

    virtual void AddHeat(const double simdt);
    virtual void RemoveHeat(const double simdt);
    virtual void UpdateHullHeatingMesh(const double simdt);
    virtual int GetHeatingMeshGroupIndex() { return 0; }  // typical heating mesh will only have one group anyway

    bool m_forceTempUpdate;
};

//---------------------------------------------------------------------------

class SetSlopePostStep : public XR1PrePostStep
{
public:
    SetSlopePostStep(DeltaGliderXR1 &vessel);
    virtual ~SetSlopePostStep();
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    RollingArray *m_pAltitudeDeltaRollingArray;  // to smooth out the jitter
    RollingArray *m_pDistanceRollingArray;       // to smooth out the jitter
    double m_refreshRate;
    double m_nextUpdateTime;          // NOTE: may be negative if user moved sim date backwards
    double m_lastUpdateTime;          // simt of last update
    double m_lastUpdateAltitude;      // altitude of last update
    bool   m_isNextUpdateTimeValid;   // false before m_nextUpdateTime is set the first time
};

//---------------------------------------------------------------------------

// handles door opening/closing sounds
class DoorSoundsPostStep : public XR1PrePostStep
{
public:
    DoorSoundsPostStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    void PlayDoorSound(DoorSound &doorSound, const double simt);
    void ShowDoorInfoMsg(DoorSound doorSound);
    DoorSound m_doorSounds[10];

    DoorStatus m_prevChamberStatus;
};

//---------------------------------------------------------------------------

class UpdateIntervalTimersPostStep : public XR1PrePostStep
{
public:
    UpdateIntervalTimersPostStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);
};

//---------------------------------------------------------------------------

class UpdateCoolantTempPostStep : public XR1PrePostStep
{
public:
    UpdateCoolantTempPostStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    double m_prevCoolantTemp;       // from previous timestep
};

//---------------------------------------------------------------------------

class AirlockDecompressionPostStep : public XR1PrePostStep
{
public:
    AirlockDecompressionPostStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    PSTREAM_HANDLE m_decompressionStream;
    PARTICLESTREAMSPEC m_particleStreamSpec; 
    double m_ventTime;  // simt @ start of decompression
    double m_streamLevel;
    PARTICLESTREAMSPEC m_airvent;
};

//---------------------------------------------------------------------------

class AutoCenteringSimpleButtonAreasPostStep : public XR1PrePostStep
{
public:
    AutoCenteringSimpleButtonAreasPostStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    // enum for DoMainYawCenter; do not change the order of these values.
    enum BUTTON { CENTER, DIVERGENT, AUTO };

    // handlers for each different button
    void DoCenterOfGravityCenter(const double simt, const double simdt, const double mjd);
    void DoHoverCenter          (const double simt, const double simdt, const double mjd);
    void DoScramCenter          (const double simt, const double simdt, const double mjd);
    void DoMainPitchCenter      (const double simt, const double simdt, const double mjd);
    void DoMainYawCenter        (const double simt, const double simdt, const double mjd);
};

//---------------------------------------------------------------------------

class ResetAPUTimerForPolledSystemsPostStep : public XR1PrePostStep
{
public:
    ResetAPUTimerForPolledSystemsPostStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);
};

//---------------------------------------------------------------------------

class ManageMWSPostStep : public XR1PrePostStep
{
public:
    ManageMWSPostStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);
};

//---------------------------------------------------------------------------

// generic one-shot delayed initializion PostStep (e.g., initialize dummy payload vessel)
class OneShotInitializationPostStep : public XR1PrePostStep
{
public:
    OneShotInitializationPostStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    bool m_done;  // if true, we are done
};

//---------------------------------------------------------------------------

class SwitchTwoDPanelPostStep : public XR1PrePostStep
{
public:
    SwitchTwoDPanelPostStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    double m_targetSwitchSimt;  // switch when simt reaches here; 0 = no switch
    int m_target2DPanel;        // panel ID
};

//---------------------------------------------------------------------------
#ifdef _DEBUG
class TestXRVesselCtrlPostStep : public XR1PrePostStep
{
public:
    TestXRVesselCtrlPostStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);
};
#endif
