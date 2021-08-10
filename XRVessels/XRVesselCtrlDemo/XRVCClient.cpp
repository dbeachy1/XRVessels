//-------------------------------------------------------------------------
// XRVCClient.cpp : implementation of main XRVCClient class methods; contains
// client code interfacing to XRVesselCtrl.  This is the class you should refer
// to for sample code invoking XRVesselCtrl interface methods.
//
// Copyright 2010-2018 Douglas E. Beachy; All Rights Reserved.
//
// This software is FREEWARE and may not be sold!
// 
// NOTE: You may not redistribute this file nor use it in any other project without
// express consent from the author.  
//
// http://www.alteaaerospace.com
// mailto:dbeachy@speakeasy.net
//
//-------------------------------------------------------------------------

/*****************************************************************************************/
/* NOTE: This is the class that contains sample code for XRVesselCtrl interface methods. */
/*****************************************************************************************/

#include <windows.h>
#include "XRVCClient.h"

// Constructor
XRVCClient::XRVCClient() : 
    m_pVessel(nullptr)
{ 
}

// Destructor
XRVCClient::~XRVCClient()
{
}

//-------------------------------------------------------------------------
// Sets the XR vessel's engine states and writes a result message to statusOut.
// Returns true if engine set successfully, false on error.
//
// Note: Range checks for *pValueToSet are the caller's responsibility.
//-------------------------------------------------------------------------
bool XRVCClient::UpdateEngineState(const XREngineID engineID, XRVCClient::DataType dataType, XRVCClient::Value &value, void *pValueToSet, CString &statusOut)
{
    // sanity-check the pointer to be within the XREngineStateWrite part of the EngineStateRead structure
    _ASSERTE(pValueToSet >= &m_xrEngineState.ThrottleLevel);
    _ASSERTE(pValueToSet <= &m_xrEngineState.DivergentMode);

    // retrieve the current engine state so we can update it 
    m_pVessel->GetEngineState(engineID, m_xrEngineState);

    // write the updated value
    if (dataType == Double)
        *static_cast<double *>(pValueToSet) = value.Double;
    else if (dataType == Bool)
        *static_cast<bool *>(pValueToSet) = value.Bool;
    else  // should never happen!
    {
        _ASSERTE(false);  
        statusOut.Format("Internal Error: invalid dataType (%d)", dataType);
        return false;
    }

    // write the updated the engine state
    bool success = m_pVessel->SetEngineState(engineID, m_xrEngineState);
    if (success)
        statusOut = "Successfully updated engine state.";
    else
        statusOut = "Could not update engine state.";

    return success;
}

//-------------------------------------------------------------------------
// Updates the damage state of an XR system and writes a result message to statusOut.
// Returns true if damage item set successfully, false on error.
//
// Note: Range checks for *pValueToSet are the caller's responsibility.
//-------------------------------------------------------------------------
bool XRVCClient::UpdateDamageState(DataType dataType, Value &value, void *pValueToSet, CString &statusOut)
{
    // sanity-check the pointer to be within the XRSystemStatusWrite part of the XRSystemStatusRead structure
    _ASSERTE(pValueToSet >= &m_xrSystemStatus.LeftWing);
    _ASSERTE(pValueToSet <= &m_xrSystemStatus.CrewElevator);

    // read the damage state
    m_pVessel->GetXRSystemStatus(m_xrSystemStatus);

    // write the updated value
    if (dataType == Double)
        *static_cast<double *>(pValueToSet) = value.Double;
    else if (dataType == Int)
        *static_cast<int *>(pValueToSet) = value.Int;
    else  // should never happen!
    {
        _ASSERTE(false);  
        statusOut.Format("Internal Error: invalid dataType (%d)", dataType);
        return false;
    }

    // write the updated the system state
    bool success = m_pVessel->SetXRSystemStatus(m_xrSystemStatus);
    if (success)
        statusOut = "Successfully updated damage state.";
    else
        statusOut = "Could not update damage state.";

    return success;
}

//-------------------------------------------------------------------------
// Sets the XR vessel's door states and writes a result message to statusOut.
// Returns true if door set successfully, false on error.
//-------------------------------------------------------------------------
bool XRVCClient::UpdateDoorState(const XRDoorID doorID, const XRDoorState doorState, CString &statusOut) const
{
    const bool success = m_pVessel->SetDoorState(doorID, doorState);
    if (success)
        statusOut = "Successfully set door state.";
    else
        statusOut = "Could not set door state.";

    return success;
}

//-------------------------------------------------------------------------
// Sets the XR vessel's light states and writes a result message to statusOut.
// Returns true if light set successfully, false on error.
// lightID is type 'int' here to make this callback compatible with EnumBoolLeafHandler in XRVCClientCommandParser.
//-------------------------------------------------------------------------
bool XRVCClient::UpdateLightState(const int lightID, const bool state, CString &statusOut) const
{
    const bool success = m_pVessel->SetExteriorLight(static_cast<XRLight>(lightID), state);
    if (success)
        statusOut = "Successfully set exterior light state.";
    else
        statusOut = "Could not set exterior light state.";

    return success;
}

//-------------------------------------------------------------------------
// Sets the XR vessel's secondary HUD mode and writes a result message to statusOut.
// Returns true if mode set successfully, false if mode not supported.
//-------------------------------------------------------------------------
bool XRVCClient::SetSecondaryHUDMode(const int modeNumber, CString &statusOut) const
{
    const bool success = m_pVessel->SetSecondaryHUDMode(modeNumber);
    if (success)
        statusOut.Format("Successfully set secondary HUD to mode %d.", modeNumber);
    else
        statusOut.Format("Invalid secondary HUD mode: %d.", modeNumber);

    return success;
}

//-------------------------------------------------------------------------
// Sets the XR vessel's secondary HUD on/off and writes a result message to statusOut.
// Returns true if mode set successfully, false if mode not supported.
//-------------------------------------------------------------------------
bool XRVCClient::SetTertiaryHUDState(const BOOL on, CString &statusOut) const
{
    const bool success = m_pVessel->SetTertiaryHUDState(on == TRUE);
    if (success)
        statusOut = "Successfully set tertiary HUD state.";
    else
        statusOut = "Tertiary HUD not supported by this vessel.";

    return success;
}

//-------------------------------------------------------------------------
// Enables/disables the XR vessel's docking mode and writes a result message to statusOut.
// Returns true if mode set successfully, false if RCS docking mode not supported.
//-------------------------------------------------------------------------
bool XRVCClient::SetRCSDockingMode(const BOOL on, CString &statusOut) const
{
    const bool success = m_pVessel->SetRCSDockingMode(on == TRUE);
    if (success)
        statusOut = "Successfully set RCS docking mode state.";
    else
        statusOut = "RCS docking mode not supported by this vessel.";

    return success;
}

//-------------------------------------------------------------------------
// Enables/disables the XR vessel's elevator EVA port and writes a result message to statusOut.
// Returns true if mode set successfully, false if elevator EVA not supported.
//-------------------------------------------------------------------------
bool XRVCClient::SetElevatorEVAPortActive(const BOOL on, CString &statusOut) const
{
    bool success = m_pVessel->SetElevatorEVAPortActive(on == TRUE);
    if (success)
        statusOut = "Successfully set elevator EVA port active state.";
    else
        statusOut = "Elevator EVA port not supported by this vessel.";

    return success;
}

//-------------------------------------------------------------------------
// Shifts the ship's center-of-gravity by the requested amount and writes a result message to statusOut.
// Returns true of shift successful, or false if shift is maxed-out.
//-------------------------------------------------------------------------
bool XRVCClient::ShiftCenterOfGravity(const double requestedShift, CString &statusOut) const
{
    bool success = m_pVessel->ShiftCenterOfGravity(requestedShift);
    if (success)
        statusOut.Format("Successfully shifted center-of-gravity by %.3lf meters.", requestedShift);
    else
        statusOut.Format("ShiftCenterOfGravity call failed.");

    return success;
}

//-------------------------------------------------------------------------
// Sets an XR vessel's standard autopilot state and writes a result message to statusOut.
// Returns true if AP state set successfully, false on error.
// autopilotID is type 'int' here to make this callback compatible with EnumBoolLeafHandler in XRVCClientCommandParser.
//-------------------------------------------------------------------------
bool XRVCClient::SetStdAutopilotState(const int autopilotID, const bool state, CString &statusOut) const
{
    XRAutopilotState newState = m_pVessel->SetStandardAP(static_cast<XRStdAutopilot>(autopilotID), state);
    const bool success = (newState != XRAPSTATE_NotSupported);
    if (success)
        statusOut = "Successfully set standard autopilot state.";
    else
        statusOut = "Autopilot is not supported by the target vessel.";

    return success;
}

//-------------------------------------------------------------------------
// Sets an XR vessel's Attitude Hold autopilot state and writes a result message to statusOut.
// Returns true if AP state set successfully, false on error.
//-------------------------------------------------------------------------
bool XRVCClient::SetAttitudeHold(const bool on, const bool *pHoldPitch, const double *pTargetPitch, const double *pTargetBank) const
{
    XRAttitudeHoldState state;
    if ((pHoldPitch == nullptr) || (pTargetPitch == nullptr) || (pTargetBank == nullptr))
    {
        // user is only setting on/off, so retrieve existing state 
        if (m_pVessel->GetAttitudeHoldAP(state) == XRAPSTATE_NotSupported)
            return false;
    }
    else  // user is setting all parameters
    {
        state.mode = (*pHoldPitch ? XRAH_HoldPitch : XRAH_HoldAOA);
        state.TargetPitch = *pTargetPitch;
        state.TargetBank = *pTargetBank;
    }

    state.on = on;
    return (m_pVessel->SetAttitudeHoldAP(state) != XRAPSTATE_NotSupported);
}


//-------------------------------------------------------------------------
// Sets an XR vessel's Attitude Hold autopilot state and writes a result message to statusOut.
// Returns true if AP state set successfully, false on error.
//-------------------------------------------------------------------------
bool XRVCClient::SetDescentHold(const bool on, const double *pTargetDescentRate, const bool *pAutoLand) const
{
    XRDescentHoldState state;
    if ((pTargetDescentRate == nullptr) || (pAutoLand == nullptr))
    {
        // user is only setting on/off, so retrieve existing state 
        if (m_pVessel->GetDescentHoldAP(state) == XRAPSTATE_NotSupported)
            return false;
    }
    else  // user is setting all parameters
    {
        state.TargetDescentRate = *pTargetDescentRate;
        state.AutoLandMode = *pAutoLand;
    }

    state.on = on;
    return (m_pVessel->SetDescentHoldAP(state) != XRAPSTATE_NotSupported);
}

//-------------------------------------------------------------------------
// Sets an XR vessel's Attitude Hold autopilot state and writes a result message to statusOut.
// Returns true if AP state set successfully, false on error.
//-------------------------------------------------------------------------
bool XRVCClient::SetAirspeedHold(const bool on, const double *pTargetAirspeed) const
{
    XRAirspeedHoldState state;
    if (pTargetAirspeed == nullptr)
    {
        // user is only setting on/off, so retrieve existing state 
        if (m_pVessel->GetAirspeedHoldAP(state) == XRAPSTATE_NotSupported)
            return false;
    }
    else  // user is setting both parameters
        state.TargetAirspeed = *pTargetAirspeed;

    state.on = on;
    return (m_pVessel->SetAirspeedHoldAP(state) != XRAPSTATE_NotSupported);
}


//-------------------------------------------------------------------------
// Methods below here retrieve XRVesselCtrl information and convert it 
// to string data.
//-------------------------------------------------------------------------

// Note: these methods expect a display area of ~68 characters wide x ~23 lines high.
// set right column at halfway
#define RIGHT_COLUMN_INDEX 34

// convenience macros
#define STR_FOR_BOOL(VAL)       ((VAL) ? "True (on)" : "False (off)")
#define WRITE_LABEL(LABEL)      AppendPaddedString(csOut, LABEL, nameWidth)
#define WRITE_INT(VAL)          AppendPaddedInt(csOut, VAL, valueWidth)
#define WRITE_DOUBLE(VAL)       AppendPaddedDouble(csOut, VAL, valueWidth)
#define WRITE_DOUBLE_PLUS(VAL)  AppendPaddedDouble(csOut, VAL, valueWidth, true)
#define WRITE_BOOL(VAL)         AppendPaddedBool(csOut, VAL, valueWidth)
#define WRITE_STR(VAL)          AppendPaddedString(csOut, VAL, valueWidth)
#define WRITE_CRLF()            csOut += "\r\n";

// dual-column convenience macros
#define WRITE_DOUBLE_PAIR(FIELDNAME)                 \
    WRITE_LABEL(#FIELDNAME ":");  WRITE_DOUBLE(state1.FIELDNAME);    \
    WRITE_LABEL(#FIELDNAME ":");  WRITE_DOUBLE(state2.FIELDNAME);    \
    WRITE_CRLF()

#define WRITE_BOOL_PAIR(FIELDNAME)                   \
    WRITE_LABEL(#FIELDNAME ":");  WRITE_BOOL(state1.FIELDNAME);      \
    WRITE_LABEL(#FIELDNAME ":");  WRITE_BOOL(state2.FIELDNAME);      \
    WRITE_CRLF()

//-------------------------------------------------------------------------
// Status retrieval methods; each of these methods sends output to a supplied CString 
// that will contain formatted (i.e., space-padded) output, appended to the end of csOut.
//-------------------------------------------------------------------------
void XRVCClient::RetrieveEngineState(CString &csOut, const XREngineID engineOne, const XREngineID engineTwo, const char *pLabelOne, const char *pLabelTwo) const
{
    _ASSERTE(m_pVessel != nullptr);

    // we will build two columns here: engineOne engineTwo
    XREngineStateRead state1;
    XREngineStateRead state2;
    m_pVessel->GetEngineState(engineOne, state1);
    m_pVessel->GetEngineState(engineTwo, state2);

    // write out two columns of values: name: val    name: val
    const int nameWidth = 22;     
    const int valueWidth = RIGHT_COLUMN_INDEX - nameWidth;

    // write the column headers and a blank line
    AppendPaddedString(csOut, pLabelOne, RIGHT_COLUMN_INDEX);
    csOut += pLabelTwo;   // no need to pad the last value on the line
    csOut += "\r\n\r\n";  // must use CR/NL, NOT just NL

    WRITE_DOUBLE_PAIR(ThrottleLevel);
    WRITE_DOUBLE_PAIR(GimbalX);
    WRITE_DOUBLE_PAIR(GimbalY);
    WRITE_DOUBLE_PAIR(Balance);     
    WRITE_BOOL_PAIR(CenteringModeX);
    WRITE_BOOL_PAIR(CenteringModeY);
    WRITE_BOOL_PAIR(CenteringModeBalance);
    WRITE_BOOL_PAIR(AutoMode);
    WRITE_BOOL_PAIR(DivergentMode);

    WRITE_DOUBLE_PAIR(TSFC);
    WRITE_DOUBLE_PAIR(FlowRate);
    WRITE_DOUBLE_PAIR(Thrust);
    WRITE_DOUBLE_PAIR(FuelLevel);
    WRITE_DOUBLE_PAIR(MaxFuelMass);
    WRITE_DOUBLE_PAIR(BayFuelMass);  // new for API 2.1

    WRITE_DOUBLE_PAIR(DiffuserTemp);
    WRITE_DOUBLE_PAIR(BurnerTemp);
    WRITE_DOUBLE_PAIR(ExhaustTemp);
}

//-------------------------------------------------------------------------
// Writes formatted ship status text to csOut
//-------------------------------------------------------------------------
void XRVCClient::RetrieveStatus(CString &csOut) const
{
    _ASSERTE(m_pVessel != nullptr);

    XRSystemStatusRead status;
    m_pVessel->GetXRSystemStatus(status);

    // write out two columns of values: name: val    name: val
    const int nameWidth = 26;     
    const int valueWidth = RIGHT_COLUMN_INDEX - nameWidth + 1;  // due to smaller font, we have more chars per line available

// ID = LeftWing, RightWing, etc.
#define WRITE_STATUS_DOUBLE_PAIR(ID1, ID2)  \
    WRITE_LABEL(#ID1 ":");                  \
    WRITE_DOUBLE(status.##ID1);             \
    WRITE_LABEL(#ID2 ":");                  \
    WRITE_DOUBLE(status.##ID2);             \
    WRITE_CRLF()

// ID = LeftAileron, RightAileron, etc.
#define WRITE_DAMAGE_STATE_PAIR(ID1, ID2)    \
    WRITE_LABEL(#ID1 ":");                   \
    WRITE_STR(GetDamageStateString(status.##ID1));  \
    WRITE_LABEL(#ID2 ":");                   \
    WRITE_STR(GetDamageStateString(status.##ID2));  \
    WRITE_CRLF()

// ID = LeftAileron, RightAileron, etc.
#define WRITE_WARNING_STATE_PAIR(ID1, ID2)    \
    WRITE_LABEL(#ID1 ":");                   \
    WRITE_STR(GetWarningStateString(status.##ID1));  \
    WRITE_LABEL(#ID2 ":");                   \
    WRITE_STR(GetWarningStateString(status.##ID2));  \
    WRITE_CRLF()

    // items that support partial failure
    WRITE_STATUS_DOUBLE_PAIR(LeftWing, RightWing);
    WRITE_STATUS_DOUBLE_PAIR(LeftMainEngine, RightMainEngine);
    WRITE_STATUS_DOUBLE_PAIR(LeftSCRAMEngine, RightSCRAMEngine);
    WRITE_STATUS_DOUBLE_PAIR(ForeHoverEngine, AftHoverEngine);
    WRITE_STATUS_DOUBLE_PAIR(LeftRetroEngine, RightRetroEngine);
    WRITE_STATUS_DOUBLE_PAIR(ForwardLowerRCS, AftUpperRCS);
    WRITE_STATUS_DOUBLE_PAIR(ForwardUpperRCS, AftLowerRCS);
    WRITE_STATUS_DOUBLE_PAIR(ForwardStarboardRCS, AftPortRCS);
    WRITE_STATUS_DOUBLE_PAIR(ForwardPortRCS, AftStarboardRCS);
    WRITE_STATUS_DOUBLE_PAIR(OutboardUpperPortRCS, OutboardLowerStarboardRCS);
    WRITE_STATUS_DOUBLE_PAIR(OutboardUpperStarboardRCS, OutboardLowerPortRCS);
    WRITE_STATUS_DOUBLE_PAIR(AftRCS, ForwardRCS);

    // online/failed damage state items
    WRITE_DAMAGE_STATE_PAIR(LeftAileron, RightAileron);
    WRITE_DAMAGE_STATE_PAIR(LandingGear, DockingPort);
    WRITE_DAMAGE_STATE_PAIR(RetroDoors, TopHatch);
    WRITE_DAMAGE_STATE_PAIR(Radiator, Speedbrake);
    WRITE_DAMAGE_STATE_PAIR(PayloadBayDoors, CrewElevator);

    // warning states
    WRITE_WARNING_STATE_PAIR(HullTemperatureWarning, MainFuelWarning);
    WRITE_WARNING_STATE_PAIR(RCSFuelWarning, APUFuelWarning);
    WRITE_WARNING_STATE_PAIR(LOXWarning, DynamicPressureWarning);
    WRITE_WARNING_STATE_PAIR(CoolantWarning, MasterWarning);

    // master warning light state (blinks)
    WRITE_LABEL("MWSLightState:");                  
    WRITE_STR((status.MWSLightState ? "ON" : "off"));
    WRITE_CRLF();

    // New for API 2.1: RCS/APU/LOX levels
    WRITE_STATUS_DOUBLE_PAIR(RCSFuelLevel, RCSMaxFuelMass);
    WRITE_STATUS_DOUBLE_PAIR(APUFuelLevel, APUMaxFuelMass);
    WRITE_STATUS_DOUBLE_PAIR(LOXLevel, LOXMaxMass);
    
    WRITE_LABEL("BayLOXMass:");
    WRITE_DOUBLE(status.BayLOXMass);

    WRITE_CRLF();
}

//-------------------------------------------------------------------------
// Writes formatted door state text to csOut
//-------------------------------------------------------------------------
void XRVCClient::RetrieveDoorsState(CString &csOut) const
{
    _ASSERTE(m_pVessel != nullptr);

    // write out one pair of values: name: val 
    const int nameWidth = 17;     
    const int valueWidth = RIGHT_COLUMN_INDEX - nameWidth;

    // define variables use our macro
    XRDoorState state;
    double doorProc;    // 0 <= n <= 1
    CString csValue;

// ID = DockingPort, ScramDoors, etc.
#define WRITE_DOOR_STATE(ID)                              \
    state = m_pVessel->GetDoorState(XRD_##ID, &doorProc); \
    WRITE_LABEL(#ID ":");                                 \
    csValue.Format("%s (%0.3lf)", GetDoorStateString(state), doorProc);  \
    WRITE_STR(csValue);                                    \
    WRITE_CRLF()

    WRITE_DOOR_STATE(DockingPort);
    WRITE_DOOR_STATE(ScramDoors);
    WRITE_DOOR_STATE(HoverDoors);
    WRITE_DOOR_STATE(Ladder);
    WRITE_DOOR_STATE(Gear);

    WRITE_DOOR_STATE(RetroDoors);
    WRITE_DOOR_STATE(OuterAirlock);
    WRITE_DOOR_STATE(InnerAirlock);
    WRITE_DOOR_STATE(AirlockChamber);

    WRITE_DOOR_STATE(CrewHatch);
    WRITE_DOOR_STATE(Radiator);
    WRITE_DOOR_STATE(Speedbrake);
    WRITE_DOOR_STATE(APU);

    WRITE_DOOR_STATE(CrewElevator);
    WRITE_DOOR_STATE(PayloadBayDoors);
}

//-------------------------------------------------------------------------
// Writes formatted autopilot state text to csOut
//-------------------------------------------------------------------------
void XRVCClient::RetrieveAutopilotsState(CString &csOut) const
{
    _ASSERTE(m_pVessel != nullptr);
    
    // write out one pair of values: name: val 
    const int nameWidth = 15;    // leave two spaces separating the two columns instead of just one
    const int valueWidth = RIGHT_COLUMN_INDEX - nameWidth;

    // define variables use our macro
    XRAutopilotState state;
    CString csValue;

// ID = KillRot, Prograde, etc.
#define WRITE_STDAP_STATE(ID)                       \
    state = m_pVessel->GetStandardAP(XRSAP_##ID);   \
    WRITE_LABEL(#ID ":");                           \
    WRITE_STR(GetAPStateString(state));             \
    WRITE_CRLF()

    // show the standard Orbiter autopilots
    WRITE_STDAP_STATE(KillRot);
    WRITE_STDAP_STATE(Prograde);
    WRITE_STDAP_STATE(Retrograde);
    WRITE_STDAP_STATE(Normal);
    WRITE_STDAP_STATE(AntiNormal);
    WRITE_STDAP_STATE(LevelHorizon);
    WRITE_STDAP_STATE(Hover);

    // show the custom XR autopilots

    WRITE_CRLF();  // separator line

    // AttitudeHold
    {       // braces are to hide local variable in this block
        XRAttitudeHoldState ahState;
        state = m_pVessel->GetAttitudeHoldAP(ahState);
        WRITE_LABEL("AttitudeHold:");
        csValue.Format("%s, %s, on = %s", GetAPStateString(state), GetAttitudeHoldMode(ahState.mode), STR_FOR_BOOL(ahState.on));
        WRITE_STR(csValue);
        WRITE_CRLF();
        WRITE_LABEL("");  // indent
        csValue.Format("TargetPitch = %+0.1lf, TargetBank = %+0.1lf", ahState.TargetPitch, ahState.TargetBank);
        WRITE_STR(csValue);
        WRITE_CRLF();
    }

    WRITE_CRLF();  // separator line

    // DescentHold
    {       // braces are to hide local variable in this block
        XRDescentHoldState dhState;
        state = m_pVessel->GetDescentHoldAP(dhState);
        WRITE_LABEL("DescentHold:");
        csValue.Format("%s, TargetDescentRate = %+0.1lf", GetAPStateString(state), dhState.TargetDescentRate);
        WRITE_STR(csValue);
        WRITE_CRLF();
        WRITE_LABEL("");  // indent
        csValue.Format("AutoLandMode = %s, on = %s", STR_FOR_BOOL(dhState.AutoLandMode), STR_FOR_BOOL(dhState.on));
        WRITE_STR(csValue);
        WRITE_CRLF();
    }

    WRITE_CRLF();  // separator line

    // AirspeedHold
    {       // braces are to hide local variable in this block
        XRAirspeedHoldState ashState;
        state = m_pVessel->GetAirspeedHoldAP(ashState);
        WRITE_LABEL("AirspeedHold:");
        csValue.Format("%s, TargetAirspeed = %0.1lf", GetAPStateString(state), ashState.TargetAirspeed);
        WRITE_STR(csValue);
        WRITE_CRLF();
        WRITE_LABEL("");  // indent
        csValue.Format("on = %s", STR_FOR_BOOL(ashState.on));
        WRITE_STR(csValue);
        WRITE_CRLF();
    }
}

//-------------------------------------------------------------------------
// Write formatted misc XRVC state to csOut; this method includes everything that does not fit into
// any of the normal categories.
//-------------------------------------------------------------------------
void XRVCClient::RetrieveOther(CString &csOut) const
{
    _ASSERTE(m_pVessel != nullptr);
       
    // write out one pair of values: name: val 
    const int nameWidth = 26;    
    const int valueWidth = RIGHT_COLUMN_INDEX - nameWidth;

    CString csValue;
    
    WRITE_LABEL("SecondaryHUDMode:");
    WRITE_INT(m_pVessel->GetSecondaryHUDMode());
    WRITE_CRLF();
    
    WRITE_LABEL("TertiaryHUDState:");
    WRITE_BOOL(m_pVessel->GetTertiaryHUDState());
    WRITE_CRLF();

    WRITE_LABEL("CenterOfGravity:");
    WRITE_DOUBLE_PLUS(m_pVessel->GetCenterOfGravity());
    WRITE_CRLF();

    WRITE_LABEL("IsRCSDockingMode:");
    WRITE_BOOL(m_pVessel->IsRCSDockingMode());
    WRITE_CRLF();

    WRITE_LABEL("IsElevatorEVAPortActive:");
    WRITE_BOOL(m_pVessel->IsElevatorEVAPortActive());
    WRITE_CRLF();

    OMMUManagement *pUMMu = m_pVessel->GetMMuObject();
    CString temp;
    temp.Format("0x%X", pUMMu);
    WRITE_LABEL("UMMU Object Address:");
    WRITE_STR(temp);
    WRITE_CRLF();

    // GetStatusScreenText
    const int maxLinesToRetrieve = 10;  // NOTE: XR vessels retain the 64 most-recent lines, but only the seven most-recent are displayed on the tertiary HUD
    char statusText[maxLinesToRetrieve * 50];
    const int lineCount = m_pVessel->GetStatusScreenText(statusText, maxLinesToRetrieve);
    csValue.Format("GetStatusScreenText: newest %d line(s) retrieved: >>>>", lineCount);
    WRITE_LABEL(csValue);
    WRITE_CRLF();
    AppendString(csOut, statusText); // this is from 0-7 lines
    WRITE_LABEL("<<<  end  <<<");
    WRITE_CRLF();
    
    // New for API 2.2: Custom skin label
    const char *pCustomSkinName = m_pVessel->GetCustomSkinName();
    WRITE_LABEL("CustomSkinName:");
    WRITE_STR((pCustomSkinName ? pCustomSkinName : "<none>"));
    WRITE_CRLF();
}

//-------------------------------------------------------------------------
// Static utility methods to format output; each returns a reference to csOut.
//-------------------------------------------------------------------------

CString &XRVCClient::AppendPaddedInt(CString &csOut, const int val, const int width)
{
    CString csLine;
    csLine.Format("%d", val);
    return AppendPaddedString(csOut, csLine, width);  // pad with spaces to proper length
}

// prependPlus: true = prepend '+' to number, false = do not. (Default = false)
CString &XRVCClient::AppendPaddedDouble(CString &csOut, const double val, const int width, const bool prependPlus)
{
    CString csLine;
    CString csFormat = (prependPlus ? "%+0.3lf" : "%0.3lf");
    csLine.Format(csFormat, val);
    return AppendPaddedString(csOut, csLine, width);  // pad with spaces to proper length
}

CString &XRVCClient::AppendPaddedBool(CString &csOut, const bool val, const int width)
{
    const char *pBoolStr = STR_FOR_BOOL(val);
    return AppendPaddedString(csOut, pBoolStr, width);  // pad with spaces to proper length
}

// Note: if width == 0 no padding will be performed
CString &XRVCClient::AppendPaddedString(CString &csOut, const CString &val, const int width)
{
    csOut += val;

    // now pad with spaces so width is fixed
    for (int i = val.GetLength(); i < width; i++)
        csOut += ' ';

    return csOut;
}

//-------------------------------------------------------------------------
// Returns a reference to the text label for a given XRDoorState
//-------------------------------------------------------------------------
const char *XRVCClient::GetDoorStateString(const XRDoorState state)
{
    const char *pRetVal;

    switch (state)
    {
    case XRDS_Opening:
        pRetVal = "Opening";
        break;
        
    case XRDS_Open:
        pRetVal = "Open";
        break;
        
    case XRDS_Closing:
        pRetVal = "Closing";
        break;
        
    case XRDS_Closed:
        pRetVal = "Closed";
        break;
        
    case XRDS_Failed:
        pRetVal = "FAILED";
        break;
        
    case XRDS_DoorNotSupported:
        pRetVal = "[Not Supported]";
        break;

    default:    // should never happen!
        _ASSERTE(false);    // break into debugger if debug build
        pRetVal = "ERROR: INVALID STATE";
    }

    return pRetVal;
}

//-------------------------------------------------------------------------
// Returns a reference to the text label for a given XRDamageState
//-------------------------------------------------------------------------
const char *XRVCClient::GetDamageStateString(const XRDamageState state)
{
    const char *pRetVal;

    switch (state)
    {
    case XRDMG_offline:
        pRetVal = "OFFLINE";
        break;
        
    case XRDMG_online:
        pRetVal = "Online";
        break;
        
    case XRDMG_NotSupported:
        pRetVal = "[N/A]";   
        break;
        
    default:    // should never happen!
        _ASSERTE(false);    // break into debugger if debug build
        pRetVal = "ERROR: INVALID STATE";
    }

    return pRetVal;
}

//-------------------------------------------------------------------------
// Returns a reference to the text label for a given XRWarningState
//-------------------------------------------------------------------------
const char *XRVCClient::GetWarningStateString(const XRWarningState state)
{
    const char *pRetVal;

    switch (state)
    {
    case XRW_warningActive:
        pRetVal = "ACTIVE";
        break;
        
    case XRW_warningInactive:
        pRetVal = "Inactive";
        break;
        
    default:    // should never happen!
        _ASSERTE(false);    
        pRetVal = "ERROR: INVALID STATE";
    }

    return pRetVal;
}

//-------------------------------------------------------------------------
// Returns a reference to the text label for a given XRAutopilotState
//-------------------------------------------------------------------------
const char *XRVCClient::GetAPStateString(const XRAutopilotState state)
{
    const char *pRetVal;

    switch (state)
    {
    case XRAPSTATE_Engaged:
        pRetVal = "ENGAGED";
        break;
        
    case XRAPSTATE_Disengaged:
        pRetVal = "Disengaged";
        break;

    case XRAPSTATE_NotSupported:
        pRetVal = "[Not Supported]";
        break;

    default:    // should never happen!
        _ASSERTE(false);    
        pRetVal = "ERROR: INVALID STATE";
    }

    return pRetVal;
}

//-------------------------------------------------------------------------
// Returns a reference to the text label for a given XRAutopilotState
//-------------------------------------------------------------------------
const char *XRVCClient::GetAttitudeHoldMode(const XRAttitudeHoldMode state)
{
    const char *pRetVal;

    switch (state)
    {
    case XRAH_HoldPitch:
        pRetVal = "HoldPitch";
        break;
        
    case XRAH_HoldAOA:
        pRetVal = "HoldAOA";
        break;

    default:    // should never happen!
        _ASSERTE(false);    
        pRetVal = "ERROR: INVALID STATE";
    }

    return pRetVal;
}
