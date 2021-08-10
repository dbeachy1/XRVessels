// ==============================================================
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// Copyright 2006-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR1PreSteps.h
// Class defining custom clbkPreStep callbacks for the DG-XR1
// ==============================================================

#pragma once

#include "DeltaGliderXR1.h"
#include "XR1PrePostStep.h"

//---------------------------------------------------------------------------

class AttitudeHoldPreStep : public XR1PrePostStep
{
public:
    AttitudeHoldPreStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    enum AXIS { PITCH, ROLL, YAW };
    // inner class to manage learning autopilot data in an atmosphere
    class LearningData
    {
    public:
        // constructor
        LearningData() { Reset(); }

        void Reset()
        {
            m_thrustFrac = 0;
            m_lastLearningThrustStep = 0;
            m_reverseLastLearningThrustStep = false;
        }

        double m_thrustFrac;      // 0...1: delta to be applied to computed thrust along one direction
        double m_lastLearningThrustStep;  // last applied learning thrust step
        bool m_reverseLastLearningThrustStep;  // if true, m_lastLearningThrustStep will be subtracted from m_thrustFrac the next time these jets fire
    };

    void ResetLastYawThrusterLevels() { m_lastSetYawThrusterGroupLevels[0] = m_lastSetYawThrusterGroupLevels[1] = 2; }   // > 1 means "not set yet"
    void ResetCenterOfLift();
    void ResetLearningData();
    void ResetAutopilot();
    double FireThrusterGroups(const double targetValue, const double currentValue, double angularVelocity, THGROUP_TYPE thgPositive, THGROUP_TYPE thgNegative, const double simdt, const double angVelLimit, const bool reverseRotation, const bool isShipInverted, const AXIS axis, const double masterThrustFrac = 1.0);
    void KillRotation(const double angularVelocity, const THGROUP_TYPE thgPositive, const THGROUP_TYPE thgNegative, const double simdt, const bool reverseRotation, double * const pOutSetThrusterGroupsLevels = nullptr, const double masterThrustFrac = 1.0);
    AUTOPILOT m_prevCustomAutopilotMode;
    LearningData m_pitchLearningData;
    double m_lastSetYawThrusterGroupLevels[2];  // last LEFT and RIGHT group levels set by the autopilot
    bool m_performedAPUWarningCallout;          // true if we warned the pilot that the APU is offline since the autopilot was engaged
    bool m_apuRanOnceWhileAPActive;             // true if the APU was running at least once while the autopilot was engaged
    bool m_forceOnlineCallout;                  // true to force "COG shift online" callout when APU autostart complete
};

//---------------------------------------------------------------------------

class DescentHoldPreStep : public XR1PrePostStep
{
public:
    DescentHoldPreStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    AUTOPILOT m_prevCustomAutopilotMode;
};

//---------------------------------------------------------------------------

class AirspeedHoldPreStep : public XR1PrePostStep
{
public:
    enum PREV_AIRSPEED_HOLD { PAH_NOTSET, PAH_OFF, PAH_ON };
    AirspeedHoldPreStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    PREV_AIRSPEED_HOLD m_prevAirspeedHold;
};

//---------------------------------------------------------------------------

class MmuPreStep : public XR1PrePostStep
{
public:
    MmuPreStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);
};

//---------------------------------------------------------------------------

class TakeoffAndLandingCalloutsAndCrashPreStep : public XR1PrePostStep
{
public:
    TakeoffAndLandingCalloutsAndCrashPreStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    // state variables are in the XR1 class since they are persisted
};

//---------------------------------------------------------------------------

class GearCalloutsPreStep : public XR1PrePostStep
{
public:
    GearCalloutsPreStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    int m_previousGearStatus;
};

//---------------------------------------------------------------------------

class MachCalloutsPreStep : public XR1PrePostStep
{
public:
    MachCalloutsPreStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);
    
protected:
    void PlayMach(const double simt, const char *pFilename);

    double m_previousMach; // mach number @ last step; < 0 = none
    double m_nextMinimumCalloutTime;
};

//---------------------------------------------------------------------------

class AltitudeCalloutsPreStep : public XR1PrePostStep
{
public:
    AltitudeCalloutsPreStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);
    
protected:
    void PlayAltitude(const double simt, const char *pFilename);

    double m_nextMinimumCalloutTime;
};

//---------------------------------------------------------------------------

class DockingCalloutsPreStep : public XR1PrePostStep
{
public:
    DockingCalloutsPreStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);
    
protected:
    void PlayDistance(const double simt, const char *pFilename);
    double GetDockingDistance();  // distance in meters

    double m_previousDistance; // Distance @ last step; < 0 = none
    double m_intervalStartTime;      // simt when m_intervalStartDistance was set
    double m_intervalStartDistance;  // distance when measuring interval started
    double m_nextMinimumCalloutTime;
    double m_previousSimt;
    double m_undockingMsgTime;
    bool m_previousWasDocked;   // true if we were docked during the previous timestep
};

//---------------------------------------------------------------------------

class UpdatePreviousFieldsPreStep : public XR1PrePostStep
{
public:
    UpdatePreviousFieldsPreStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);
};

//---------------------------------------------------------------------------

class UpdateVesselLightsPreStep : public XR1PrePostStep
{
public:
    UpdateVesselLightsPreStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);
};

//---------------------------------------------------------------------------

class NosewheelSteeringPreStep : public XR1PrePostStep
{
public:
    NosewheelSteeringPreStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);
};

//---------------------------------------------------------------------------

class ParkingBrakePreStep : public XR1PrePostStep
{
public:
	ParkingBrakePreStep(DeltaGliderXR1 &vessel);
	virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);
};


//------------------------------------------------------------------------
// PAYLOAD presteps NOT USED by the XR1; they are here for subclasses to use
//-------------------------------------------------------------------------

class RefreshGrappleTargetsInDisplayRangePreStep : public XR1PrePostStep
{
public:
    RefreshGrappleTargetsInDisplayRangePreStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    double m_lastUpdateSystemUptime;  // used to manage refresh intervals
};

//-----------------------------------------------------------------------------
// ROTATING WHEEL prestep NOT USED by the XR1; it is here for subclasses to use
//-----------------------------------------------------------------------------

class RotateWheelsPreStep : public XR1PrePostStep
{
public:
    RotateWheelsPreStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    void SetWheelRotVel(const double simdt, const double groundSpeed, const bool isWheelOnGround, double &wheelRotationVelocity);
    void SetXRAnimationForVelocity(const double simdt, const UINT &animationHandle, const double currentRotVel, double &wheelProc, const double rotationFraction, const double wheelCircumference);

    double m_noseWheelProc;         // nose wheels animation state: 0 <= n <= 1
    double m_rearWheelProc;         // rear wheels animation state: 0 <= n <= 1

    double m_noseWheelRotationVelocity;  // velocity in m/s at the tire's edge; this will match ground speed while on ground
    double m_rearWheelRotationVelocity;  // velocity in m/s at the tire's edge; this will match ground speed while on ground
};

//---------------------------------------------------------------------------
// NOT USED by the XR1; this is here for subclasses to use
//---------------------------------------------------------------------------

class AnimateGearCompressionPreStep : public XR1PrePostStep
{
public:
    AnimateGearCompressionPreStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    double m_previousAltitude;  // altitude at previous timestep
};

//---------------------------------------------------------------------------

class ScramjetSoundPreStep : public XR1PrePostStep
{
public:
    ScramjetSoundPreStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);
    
protected:
    void PlayAmbient();
    void ResetTimer();

    double m_nextPlayTime;
    int m_previousAmbientIndex;
};

//---------------------------------------------------------------------------
// NOT USED by the XR1; this is here for subclasses to use
//---------------------------------------------------------------------------

class EngineSoundsPreStep : public XR1PrePostStep
{
public:
    EngineSoundsPreStep(DeltaGliderXR1 &vessel, 
        const bool playMain,
        const bool playHover,
        const bool playRCS);

    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    const bool m_playMain;
    const bool m_playHover;
    const bool m_playRCS;
};

//---------------------------------------------------------------------------
// NOT USED by the XR1; is here for subclasses to use
//---------------------------------------------------------------------------

class DrainBayFuelTanksPreStep : public XR1PrePostStep
{
public:
    DrainBayFuelTanksPreStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);
    double FlowBayFuel(const PROPELLANT_HANDLE ph, const bool isTankHidden);
};

//---------------------------------------------------------------------------
// NOT USED by the XR1; is here for subclasses to use
//---------------------------------------------------------------------------

class RefreshSlotStatesPreStep : public XR1PrePostStep
{
public:
    RefreshSlotStatesPreStep(DeltaGliderXR1 &vessel);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

private:
    double m_nextRefreshSimt;   // simt when we should perform the next rescan 
};
