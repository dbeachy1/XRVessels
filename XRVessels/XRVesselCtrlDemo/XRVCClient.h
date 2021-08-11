//-------------------------------------------------------------------------
// XRVCClient.h : definition of XRVCClient class; all of our XRVesselCtrl calls
//                are made from this class.
//
// Copyright 2010-2018 Douglas E. Beachy; All Rights Reserved.
//
// This software is FREEWARE and may not be sold!
// 
// NOTE: You may not redistribute this file nor use it in any other project without
// express consent from the author.  
//
// http://www.alteaaerospace.com
// mailto:doug.beachy@outlook.com
//-------------------------------------------------------------------------

#pragma once

#include <windows.h>
#include <atlstr.h>

#include "orbitersdk.h"
#include "XRVesselCtrl.h"

class XRVCClient
{
public:
    XRVCClient();
    virtual ~XRVCClient();

    static bool IsXRVesselCtrl(const VESSEL *pVessel) { return XRVesselCtrl::IsXRVesselCtrl(pVessel); }

    void SetXRVessel(XRVesselCtrl *pVessel)       { m_pVessel = pVessel; }      // may be null
    XRVesselCtrl *GetXRVessel() const             { return m_pVessel; }         // may be null
    XREngineStateWrite &GetXREngineStateWrite()   { return m_xrEngineState; }   // working XREngineStateWrite structure
    XRSystemStatusWrite &GetXRSystemStatusWrite() { return m_xrSystemStatus; }  // working XRSystemStatusWrite structure

    // Status retrieval methods; each method sends output to a supplied CString 
    // that will contain formatted (i.e., space-padded) output.
    void RetrieveEngineState(CString &csOut, const XREngineID engineOne, const XREngineID engineTwo, const char *pLabelOne, const char *pLabelTwo) const;
    void RetrieveStatus(CString &csOut) const;
    void RetrieveDoorsState(CString &csOut) const;
    void RetrieveAutopilotsState(CString &csOut) const;
    void RetrieveOther(CString &csOut) const;

    // generic reusable enums/unions
    enum DataType { Double, Bool, Int};  // type of value to set
    union Value { double Double; bool Bool; int Int; };  // value to be written

    // State update methods; each methods writes a status message to statusOut
    bool UpdateEngineState(const XREngineID engineID, DataType dataType, Value &value, void *pValueToSet, CString &statusOut);
    bool UpdateDoorState(const XRDoorID doorID, const XRDoorState doorState, CString &statusOut) const;
    bool UpdateLightState(const int lightID, const bool state, CString &statusOut) const;
    bool SetSecondaryHUDMode(const int modeNumber, CString &statusOut) const;
    bool SetTertiaryHUDState(const BOOL on, CString &statusOut) const;
    bool SetRCSDockingMode(const BOOL on, CString &statusOut) const;
    bool SetElevatorEVAPortActive(const BOOL on, CString &statusOut) const;
    bool ShiftCenterOfGravity(const double requestedShift, CString &statusOut) const;
    bool SetStdAutopilotState(const int autopilotID, const bool state, CString &statusOut) const;
    bool SetAttitudeHold(const bool on, const bool *pHoldPitch = nullptr, const double *pTargetPitch = nullptr, const double *pTargetBank = nullptr) const;
    bool SetDescentHold(const bool on, const double *pTargetDescentRate = nullptr, const bool *pAutoLand = nullptr) const;
    bool SetAirspeedHold(const bool on, const double *pTargetAirspeed = nullptr) const;
    void ResetAutopilots() const { m_pVessel->KillAutopilots(); } 
    bool ResetMasterWarningAlarm() const { return m_pVessel->ResetMasterWarningAlarm(); }
    bool ResetDamage() const { return m_pVessel->ClearAllXRDamage(); }
    bool UpdateDamageState(DataType dataType, Value &value, void *pValueToSet, CString &statusOut);

protected:
    XRVesselCtrl *m_pVessel;      // active XR vessel, or NULL for none

    // static utility methods to format output; each returns a reference to csOut
    static CString &AppendPaddedInt(CString &csOut, const int val, const int width);
    static CString &AppendPaddedDouble(CString &csOut, const double val, const int width, const bool prependPlus = false);
    static CString &AppendPaddedBool(CString &csOut, const bool val, const int width);
    static CString &AppendPaddedString(CString &csOut, const CString &val, const int width);
    static CString &AppendString(CString &csOut, const CString &val) { return AppendPaddedString(csOut, val, 0); }

    // static enum -> string conversion methods
    static const char *GetDoorStateString(const XRDoorState state);
    static const char *GetDamageStateString(const XRDamageState state);
    static const char *GetWarningStateString(const XRWarningState state);
    static const char *GetAPStateString(const XRAutopilotState state);
    static const char *GetAttitudeHoldMode(const XRAttitudeHoldMode state);

private:
    // Working state data; callers specify a value to update in these structures.
    // Note: this must be XR___StateRead because we must read state before we update it.
    // However, we only expose the XR___StateWrite portion of it.
    XREngineStateRead  m_xrEngineState;
    XRSystemStatusRead m_xrSystemStatus;
};

