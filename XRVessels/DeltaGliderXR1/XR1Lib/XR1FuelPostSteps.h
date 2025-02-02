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
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// XR1FuelPostSteps.h
// Class defining fuel-related PostSteps for the DG-XR1
// ==============================================================

#pragma once

#include "DeltaGliderXR1.h"
#include "XR1PrePostStep.h"

// handles fuel callouts (full/low/depleted)
class FuelCalloutsPostStep : public XR1PrePostStep
{
public:
    FuelCalloutsPostStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    void CheckFuelLevel(const char *pLabel, PROPELLANT_HANDLE ph, double &prevQty, WarningLight warningLight);
    void CheckLoxLevel();

    double m_prevMainFuelFrac;
    double m_prevRcsFuelFrac;
    double m_prevScramFuelFrac;
    double m_prevLoxFrac;
};

//---------------------------------------------------------------------------

class APUPostStep : public XR1PrePostStep
{
public:
    APUPostStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    void BurnAPUFuel(const double simt, const double simdt, const double mjd);
    void UpdateAPUDoorState(const double simt, const double simdt, const double mjd);

    /* DO NOT NEED THIS FOR NOW SINCE APU OFFLINE TURNS OFF CONTROL SURFACES
    bool CheckControlSurface(const int index, const AIRCTRL_TYPE type);
    double m_prevControlSurfaceState[4];
    */

    DoorStatus m_prevDoorStatus;  // from previous timestep
    double m_doorTargetSimt;      // time at which APU is fully operational/shutdown
    double m_prevQty;             // fuel quantity @ previous timestep
    bool m_firstTimeStep;         // true if this is still the first timestep
    bool m_poweringUpOrDown;      // true if APU is transitioning to a power-up or power-down state
};

//---------------------------------------------------------------------------

class UpdateMassPostStep : public XR1PrePostStep
{
public:
    UpdateMassPostStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);
};

//---------------------------------------------------------------------------

class DisableControlSurfForAPUPostStep : public XR1PrePostStep
{
public:
    DisableControlSurfForAPUPostStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    bool m_initialStartupComplete;  // true if > 1 timestep elapsed
};

//---------------------------------------------------------------------------

class FuelDumpPostStep : public XR1PrePostStep
{
public:
    FuelDumpPostStep(DeltaGliderXR1 &vessel);
    virtual ~FuelDumpPostStep();
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    bool DumpFuel(const PROPELLANT_HANDLE ph, const double simdt, bool &dumpInProgress, const double rateFraction);

    double m_nextWarningSimt;   // send next warning message

    PSTREAM_HANDLE m_fuelDumpStream1;
    PSTREAM_HANDLE m_fuelDumpStream2;
    double m_fuelDumpLevel;   // 0...1: used for particle streams: indicates relative strength of fuel flow being dumped
};

//---------------------------------------------------------------------------

class XFeedPostStep : public XR1PrePostStep
{
public:
    XFeedPostStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);
};

//---------------------------------------------------------------------------

// Worker class to handle line pressures (invoked by ResupplyPostStep)
class LinePressure
{
public:
    LinePressure(double &linePressure, double &nominalLinePressure, bool &pressureNominalLineStatusFlag, const bool &flowInProgress, double maxPressure, double pressureMovementRate, DeltaGliderXR1 &xr1);
    void AdjustPressure(const double simt, const double simdt, const double mjd);
    void Disconnected();

    double m_pressureTarget;        // in PSI; -1 = "target is nominal resupply pressure"

protected:
    DeltaGliderXR1 &m_xr1;          // our parent vessel
    double &m_linePressure;         // in PSI; resides in XR1 object
    double &m_nominalLinePressure;  // in PSI; resides in XR1 object
    bool &m_pressureNominalLineStatusFlag;  // resides in XR1 object
    const bool &m_flowInProgress;   // resides in XR1 object; usually a flow switch boolean
    const double m_maxPressure;     // in PSI
    double m_pressureMovementRate;  // fraction of max pressure to move in one second; e.g., 0.20 = 20% of max pressure
    double m_initialPressureTarget; // PSI when fuel line first attached (this will be nominal pressure until disconnect)
};

//---------------------------------------------------------------------------

// NOTE: this class handles external cooling logic as well
class ResupplyPostStep : public XR1PrePostStep
{
public:
    ResupplyPostStep(DeltaGliderXR1 &vessel);
    virtual ~ResupplyPostStep();
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    void PerformRefueling(const double simt, const double simdt, const double mjd);
    void PerformLoxResupply(const double simt, const double simdt, const double mjd);
    void DisconnectFuelLines();     // invoked when refueling lines disconnected
    void DisconnectLoxLine();       // invoked when LOX line disconnected
    
    void FlowMainFuel(const double simt, const double simdt, const double mjd);   // invoked only when refueling main tanks
    void FlowScramFuel(const double simt, const double simdt, const double mjd);  // invoked only when refueling scram tanks
    void FlowApuFuel(const double simt, const double simdt, const double mjd);    // invoked only when refueling apu tanks
    void FlowLox(const double simt, const double simdt, const double mjd);        // invoked only when refueling lox tanks

    // line pressure objects
    LinePressure *m_pMainLinePressure;
    LinePressure *m_pScramLinePressure;
    LinePressure *m_pApuLinePressure;
    LinePressure *m_pLoxLinePressure;

    // includes time for the external lines to latch to the ship; should be synced with sound effect
    const double m_resupplyStartupTime;

    // sequence timestamp data; -1 = disabled
    double m_refuelingSequenceStartSimt;         // simt when refueling enabled
    double m_loxSequenceStartSimt;               // simt when lox resupply enabled
    double m_externalCoolingSequenceStartSimt;   // simt when external cooling enabled
    double m_resupplyMovementFirstDetectedSimt;  // simt when the ship first started moving while resupply was enabled.

    // previous timestep data
    bool m_prevResupplyEnabledStatus;   // true = resupply was ENABLED 
    DoorStatus m_prevFuelHatchStatus;   
    DoorStatus m_prevLoxHatchStatus;    
    DoorStatus m_prevExternalCoolingStatus;
    double m_prevSimt;         
};

//---------------------------------------------------------------------------

// Handles LOX consumption
class LOXConsumptionPostStep : public XR1PrePostStep
{
public:
    LOXConsumptionPostStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    bool m_previousAmbientO2Available;  // from previous timestep
    double m_previousO2Level;           // cabin level
};

//---------------------------------------------------------------------------

class PreventAutoRefuelPostStep : public XR1PrePostStep
{
public:
    PreventAutoRefuelPostStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    void ResetFuelData()
    {
        for (int i=0; i < 3; i++)
            m_previousInternalFuelQty[i] = -1;
    }

    void DisableAutoRefuel(PROPELLANT_HANDLE ph, const int index, const bool bEnabled);

    // index 0=main, 1=rcs, 2=scram
    double m_previousInternalFuelQty[3]; // fuel qty *in the internal tank only* @ previous timestep in kg
    double m_previousBayFuelQty[3];      // fuel qty *in the payload bay only* @ previous timestep in kg
};

//---------------------------------------------------------------------------

class BoilOffPostStep : public XR1PrePostStep
{
public:
    BoilOffPostStep(DeltaGliderXR1 &vessel);
    virtual ~BoilOffPostStep();
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    PSTREAM_HANDLE m_stream1;
    PSTREAM_HANDLE m_stream2;
    double m_level;
};
