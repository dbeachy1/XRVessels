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
// Public XR-Class Vessel Control Header File.
// 
// XRVesselControl Version: 3.2
// Release Date: 11-Aug-2021
//
// XR vessels implementing this API version: XR1 2.0, XR2 2.0, XR5 2.0
//
// XRVesselCtrl.h : Main header file defining the public XR-class vessel control API.
//
// NOTE: Refer to the sample 'XRVesselCtrlDemo' module for sample code on how to use 
//       this API.
// ==============================================================

#pragma once

#include "orbitersdk.h"
#include <vector>

using namespace std;

// Calling applications should '#include "oMMU_API.h"' in order to use UMMUCREWMANAGMENT
class OMMUManagement;

class XRVesselCtrl;

// Use this floating point constant when implementing your ship's GetCtrlAPIVersion method; also, you should compare each vessel's API 
// version against this version when you are writing interface code.
#define THIS_XRVESSELCTRL_API_VERSION 3.2f

/*
  Here is an example of how to use the XRVesselCtrl API:

  if (shipHandle == nullptr)
  {
    // handle error: target vessel not found
  }

  // test whether the target vessel implements XRVesselCtrl
  VESSEL *pCandidateVessel = oapiGetVesselInterface(shipHandle);
  if (!XRVesselCtrl::IsXRVesselCtrl(pCandidateVessel))
  {
    // handle error: target vessel does not support XRVesselCtrl
  }

  XRVesselCtrl *pVessel = static_cast<XRVesselCtrl *>(pCandidateVessel);  // downcast to actual object type

  // We have access to the XR vessel; now let's check the API version
  float apiVersion = pVessel->GetCtrlAPIVersion();
  if (apiVersion < THIS_XRVESSELCTRL_API_VERSION)     // verify that the ship's API version is >= the API version we linked with
  {
      // handle error: API too old
  }
  
  // API version is OK.  Let's retrieve the ship's current status...
  XRSystemStatusRead status;
  pVessel->GetXRSystemStatus(status);

  // ... examine the status structure and continue...
*/

//-------------------------------------------------------------------------

// Defines XR engine IDs
// Note: "Left" is port, "Right" is starboard.
enum class XREngineID 
{ 
    XRE_MainLeft,  XRE_MainRight, 
    XRE_HoverFore, XRE_HoverAft,
    XRE_ScramLeft, XRE_ScramRight,
    XRE_RetroLeft, XRE_RetroRight
};

// Writable values for an engine's state.
struct XREngineStateWrite
{
    double     ThrottleLevel;   // 0 <= n <= 1.0
    double     GimbalX;         // -1.0 <= n <= 1.0 : 0 = centered horizontally (yaw)
    double     GimbalY;         // -1.0 <= n <= 1.0 : 0 = centered vertically (pitch)
    double     Balance;         // -1.0 <= n <= 1.0 : 0 = equally balanced between fore/aft or left/right engines
    bool       CenteringModeX;  // true if pitch centering mode is set
    bool       CenteringModeY;  // true if yaw centering mode is set
    bool       CenteringModeBalance; // true if balance centering mode is set
    bool       AutoMode;        // true if auto mode is set
    bool       DivergentMode;   // true if divergent mode is set
};

// Readable values for an engine's state, which include all the writable values plus the values below.  
// This allows you to do things like this:
/*
    XREngineStateRead state;
    vessel.GetEngineState(XRE_MainLeft, state);   // populate the structure
    state.ThrottleLevel = 1.0;                    // set to full throttle...
    vessel.SetEngineState(XRE_MainLeft, state);   // ...and write the new state back to the vessel
*/
struct XREngineStateRead : public XREngineStateWrite
{
    double     TSFC;            // 0 <= n
    double     FlowRate;        // kg/sec
    double     Thrust;          // kN
    double     FuelLevel;       // 0 <= n <= 1.0
    double     MaxFuelMass;     // max fuel mass for this tank in kg (includes bay fuel tanks, if any)

    // Some or all of these values may not be available for all engines; any unsupported value will be set to -1.
    double DiffuserTemp;        // degrees K
    double BurnerTemp;          // degrees K
    double ExhaustTemp;         // degrees K

    // Field added in API Version 2.1
    double     BayFuelMass;     // quantity of fuel in kg currently in PAYLOAD BAY TANKS ONLY (may be zero)
};

//-------------------------------------------------------------------------

// Door States; a "door" has five normal states: OPEN, CLOSED, OPENING, CLOSING, and FAILED
// In addtion, a door has a 'proc' showing how far it is open or closed, from 0 (close) to 1 (open) (-1 = proc not supported).
// Also note that XRD_AirlockChamber refers to the state of the air pressure in the airlock: 
// OPEN = pressurized, CLOSED = vacuum, OPENING = pressurizing, CLOSING = depressurizing
enum class XRDoorID 
{ 
    XRD_DockingPort, XRD_ScramDoors, XRD_HoverDoors, XRD_Ladder, XRD_Gear, 
    XRD_RetroDoors, XRD_OuterAirlock, XRD_InnerAirlock, XRD_AirlockChamber, 
    XRD_CrewHatch, XRD_Radiator, XRD_Speedbrake, XRD_APU, 
    XRD_CrewElevator, XRD_PayloadBayDoors
};

enum class XRDoorState { XRDS_Opening, XRDS_Open, XRDS_Closing, XRDS_Closed, XRDS_Failed, XRDS_DoorNotSupported };

//-------------------------------------------------------------------------

enum class XRDamageState  { XRDMG_offline, XRDMG_online, XRDMG_NotSupported };
enum class XRWarningState { XRW_warningActive, XRW_warningInactive };

// Warning, damage, and system status: XR-class vessels support incremental damage, from 1.0 (100% working)
// to 0.0 (0% working).  For example, a wing that is 50% damaged will only provide 50% of 
// its normal lift.
//
// Some systems, such as doors, are either online or offline (working or failed), so those systems return an XRDamageState enum.
// For warning status, each system returns an XRWarningState enum.
// Damage may also be set by editing the XR Vessel's scenario file.
//
// Note: any systems not supported by this XR vessel will be set to -1 for double fields 
// and XRDMG_NotSupported for XRDamageState fields.

struct XRSystemStatusWrite
{
    double LeftWing;
    double RightWing;
    double LeftMainEngine;
    double RightMainEngine;
    double LeftSCRAMEngine;
    double RightSCRAMEngine;
    double ForeHoverEngine;   // these are *logical* engines
    double AftHoverEngine;
    double LeftRetroEngine;
    double RightRetroEngine;
    double ForwardLowerRCS;
    double AftUpperRCS;
    double ForwardUpperRCS;
    double AftLowerRCS;
    double ForwardStarboardRCS;
    double AftPortRCS;
    double ForwardPortRCS;
    double AftStarboardRCS;
    double OutboardUpperPortRCS;
    double OutboardLowerStarboardRCS;
    double OutboardUpperStarboardRCS;
    double OutboardLowerPortRCS;
    double AftRCS;
    double ForwardRCS;
    XRDamageState LeftAileron;     // includes left elevator if a separate elevator surface is present
    XRDamageState RightAileron;    // includes right elevator if a separate elevator surface is present
    XRDamageState LandingGear;     
    XRDamageState DockingPort;     // "nosecone" on some vessels
    XRDamageState RetroDoors;      
    XRDamageState TopHatch;        // "crew hatch" on some vessels        
    XRDamageState Radiator;        
    XRDamageState Speedbrake;      // "airbrake" on some vessels
    XRDamageState PayloadBayDoors;
    XRDamageState CrewElevator;   
};

struct XRSystemStatusRead : public XRSystemStatusWrite
{
    // The warning states below are not persisted in the scenario file; they are constantly
    // recalculated dynamically.
    XRWarningState HullTemperatureWarning;
    XRWarningState MainFuelWarning;
    XRWarningState RCSFuelWarning;
    XRWarningState APUFuelWarning;
    XRWarningState LOXWarning;
    XRWarningState DynamicPressureWarning;
    XRWarningState CoolantWarning;
    XRWarningState MasterWarning;     // warning active if *any* other warning active
    bool           MWSLightState;     // updated as the MWS light blinks: true = lit, false = not lit

    // Fields added in API version 2.1
    double RCSFuelLevel;        // 0 <= n <= 1.0
    double RCSMaxFuelMass;      // max fuel mass for RCS in kg (includes bay fuel tanks, if any)
    double APUFuelLevel;        // 0 <= n <= 1.0
    double APUMaxFuelMass;      // max fuel mass for APU in kg
    double LOXLevel;            // 0 <= n <= 1.0
    double LOXMaxMass;          // max internal LOX mass in kg (includes bay LOX tanks, if any)
    double BayLOXMass;          // quantity of LOX in kg currently in PAYLOAD BAY TANKS ONLY (may be zero)

    // Fields added in API version 3.0
    bool MWSAlarmState;         // true = MWS alarm active, false = alarm silenced via alarm reset or no warning active
    bool COGAutoMode;           // true = center-of-gravity shift in auto-mode because the Attitude Hold or Descent Hold autopilot is engaged
    bool InternalSystemsFailure;// true = internal systems failed due to coolant overheat
    double CenterOfGravity;     // CoG shift in meters: 0 = centered, < 0 is forward, > 0 is aft
    double CabinO2Level;        // Oxygen fraction in cabin; nominal = .209 (20.9%)
    
    double CoolantTemp;         // in degrees C
    double NoseconeTemp;        // in Kelvin
    double LeftWingTemp;        // in Kelvin
    double RightWingTemp;       // in Kelvin
    double CockpitTemp;         // external cockpit hull temperature in Kelvin
    double TopHullTemp;         // in Kelvin

    double MaxSafeNoseconeTemp; // in Kelvin
    double MaxSafeWingTemp;     // in Kelvin
    double MaxSafeCockpitTemp;  // in Kelvin
    double MaxSafeTopHullTemp;  // in Kelvin
};

// contains data about an XR payload bay slot
struct XRPayloadSlotData
{
    VESSEL *hCargoModuleVessel; // Orbiter vessel handle of payload module attached in this slot; will be NULL if slot is empty
    XRVesselCtrl *pParentXRVessel;        // Orbiter vessel handle of the XR vessel to which this bay slot belongs
    ATTACHMENTHANDLE hXRAttachmentHandle; // XR vessel's attachment handle for this bay slot; will never be NULL
    int SlotNumber;             // 1-n
    VECTOR3 localCoordinates;   // XR-vessel-relative coordinates to the center of this payload slot
    bool IsOccupied;            // true if a payload module occupies this slot
    VECTOR3 Dimensions;         // width (X), height (Y), length (Z)
    int BayLevel;               // 1-n (will be one except for multi-level payload bays such as the XR5's)
};

//-------------------------------------------------------------------------

// Define exterior lights
enum class XRLight { XRL_Nav, XRL_Beacon, XRL_Strobe };

//-------------------------------------------------------------------------

// Define standard autopilot modes
enum class XRStdAutopilot 
{ 
    XRSAP_KillRot, XRSAP_Prograde, XRSAP_Retrograde, 
    XRSAP_Normal, XRSAP_AntiNormal, XRSAP_LevelHorizon, XRSAP_Hover
};

// Define autopilot state return values.  Not all vessels will support all autopilot modes; in
// that case, XRAPSTATE_NotSupported is returned.
enum class XRAutopilotState
{
    XRAPSTATE_Engaged, XRAPSTATE_Disengaged, XRAPSTATE_NotSupported
};

// Attitude Hold modes
enum class XRAttitudeHoldMode { XRAH_HoldPitch, XRAH_HoldAOA };

// Define extended autopilot status structures
struct XRAttitudeHoldState 
{
    bool                on;
    XRAttitudeHoldMode  mode;   // XRAH_HoldPitch, etc.
    double              TargetPitch;   // in degrees (this is also the target AOA if AOA mode engaged)
    double              TargetBank;    // in degrees
};

struct XRDescentHoldState
{
    bool    on;
    double  TargetDescentRate;  // in m/s; this is negative for descending and positive for ascending
    bool    AutoLandMode;       // true = ENGAGED
};

struct XRAirspeedHoldState
{
    bool    on;
    double  TargetAirspeed;   // m/s
};

// This exported symbol indicates that the vessel that linked with code in this header file; if set to 'true', it means this vessel class implemented by this DLL implements the XRVesselCtrl interface.
DLLCLBK bool XRVesselCtrlFlag;

#ifdef ORBITER_MODULE   // Only implement this code once per vessel.  Variable value is declared here so we don't need to bother including a separate XRVesselCtrl.lib for just one exported variable.
bool XRVesselCtrlFlag = true;
#endif

// added in XRVesselCtrl API version 3.0
enum class XRXFEED_STATE { XRXF_MAIN, XRXF_OFF, XRXF_RCS };

//=========================================================================
// Each vessel that supports this API will extend this abstract 
// base class.  This need not be limited to only XR-class vessels; it is up
// to the vessel developer to implement each of the virtual methods below.
//=========================================================================
class XRVesselCtrl : public VESSEL4
{
public:
    // Returns true if the supplied vessel is an XR vessel that supports XRVesselCtrl 1.5 or later
    static bool IsXRVesselCtrl(const VESSEL *pVessel)
    {
        // Let's figure out if the supplied vessel implements XRVesselCtrl.
        // Since we can't rely on RTTI here to test vessel objects that do not contain RTTI, we instead check 
        // the status of an exported XRVesselCtrlFlag, if any.
        bool retVal = false;  // assume not XRVesselCtrl
        const HMODULE hDLL = GetModuleHandle(pVessel->GetClassName());  // should always succeed
        if (hDLL != nullptr)
        {
            const bool *pFlag = reinterpret_cast<const bool *>(GetProcAddress(hDLL, "XRVesselCtrlFlag"));
            if (pFlag != nullptr)
                retVal = *pFlag;  // will be 'true' for vessels that implement XRVesselCtrl
            // Do not free hDLL handle here! GetModuleHandle does not increment the reference count.
        }
        return retVal;
    }

    // Constructor
    XRVesselCtrl(OBJHANDLE vessel, int fmodel) : VESSEL4(vessel, fmodel) { } 

#ifdef VSEXPRESS_2005
    // DUMMY FUNCTION TO COMPENSATE FOR VISUAL STUDIO EXPRESS 2005 INCOMPATIBILITY: this must be placed here at the top of the file.
    // This will force the compiler to bump up the virtual function pointer lookups by four bytes, generating correct code to invoke
    // the XRVesselCtrl methods.  This line *must not* be included for other Visual Studio versions.
    //
    // If you are compiling your application under Visual Studio 2005, add a line "#define VSEXPRESS_2005" to a header file included before this one.
    virtual void dummy() { }
#endif

    // API version implemented by this vessel
    // Note: this must be virtual so that the correct version will be returned by each vessel instance.
    virtual float GetCtrlAPIVersion() const { return THIS_XRVESSELCTRL_API_VERSION; }

    //--------------------------------------------------------------
    // These methods must be implemented by the XR vessel subclass.
    //--------------------------------------------------------------

    // Engine State: these methods return true if XREngineID is valid for this ship and (for Set) the operation succeeded; otherwise, the methods do nothing.
    // Remember that not all engines support all fields in XREngineStateWrite and not all vessels support all engine types in XREngineID.
    virtual bool SetEngineState(XREngineID id, const XREngineStateWrite &state) = 0;
    virtual bool GetEngineState(XREngineID id, XREngineStateRead &state) const = 0;  
    
    // Door State
    virtual bool SetDoorState(XRDoorID id, XRDoorState state) = 0;   // returns true if door/state combination is valid for this ship
    virtual XRDoorState GetDoorState(XRDoorID id, double *pProc = nullptr) const = 0;  // returns XRDS_DoorNotSupported if door does not exist for this ship; if pProc != nullptr, proc is set to 0 <= n <= 1.0

    // Set/Read the damage/system state
    // Remember that not all vessels support all systems in XRSystemStatusWrite and XRSystemStatusRead.
    // NOTE: it is currently not possible to repair the ship after it is destroyed (e.g., exploded or crashed into the ground)
    virtual bool SetXRSystemStatus(const XRSystemStatusWrite &status) = 0;  // returns true if vessel successfully set all damage states
    virtual void GetXRSystemStatus(XRSystemStatusRead &status) const = 0;

    // Repairs all damaged systems
    // NOTE: it is currently not possible to repair the ship after it is destroyed (e.g., exploded or crashed into the ground)
    virtual bool ClearAllXRDamage() = 0;  // returns true if this call is supported by this vessel

    // Kill all autopilots
    virtual void KillAutopilots() = 0;

    // Standard Autopilot status (on/off only)
    virtual XRAutopilotState SetStandardAP(XRStdAutopilot id, bool on) = 0;  // returns the new state of the autopilot, or XRAPSTATE_NotSupported if requested autopilot not supported
    virtual XRAutopilotState GetStandardAP(XRStdAutopilot id) = 0;     // cannot be const due to Orbiter core not declaring 'GetNavmodeState' const

    // Extended Autopilot methods
    virtual XRAutopilotState SetAttitudeHoldAP(const XRAttitudeHoldState &state) = 0;  // returns the new state of the autopilot, or XRAPSTATE_NotSupported if autopilot not supported
    virtual XRAutopilotState GetAttitudeHoldAP(XRAttitudeHoldState &state) const = 0;

    virtual XRAutopilotState SetDescentHoldAP(const XRDescentHoldState &state) = 0;    // returns the new state of the autopilot, or XRAPSTATE_NotSupported if autopilot not supported
    virtual XRAutopilotState GetDescentHoldAP(XRDescentHoldState &state) const = 0;

    virtual XRAutopilotState SetAirspeedHoldAP(const XRAirspeedHoldState &state) = 0;  // returns the new state of the autopilot, or XRAPSTATE_NotSupported if autopilot not supported
    virtual XRAutopilotState GetAirspeedHoldAP(XRAirspeedHoldState &state) const = 0;

    // Exterior lights: true = ON, false = OFF or not supported
    virtual bool SetExteriorLight(XRLight light, bool state) = 0;  // returns true on success, false if not supported
    virtual bool GetExteriorLight(XRLight light) const = 0; 
    
    // Secondary HUD mode (1-5) : 0 = OFF
    virtual bool SetSecondaryHUDMode(int modeNumber) = 0;  // Returns: true on success, false if mode is unsupported
    virtual int GetSecondaryHUDMode() const = 0;
    
    // Enable/disable tertiary HUD
    virtual bool SetTertiaryHUDState(bool on) = 0;  
    virtual bool GetTertiaryHUDState() const = 0;

    // Reset the MWS (Master Warning System) alarm; note that under certain conditions the MWS cannot be reset (e.g., after a vessel crash)
    virtual bool ResetMasterWarningAlarm() = 0;    // returns true if MWS alarm reset successfully, false if the alarm cannot be reset

    // Center-of-Gravity shift
    virtual bool   ShiftCenterOfGravity(double requestedShift) = 0;  // requestedShift = requested delta in meters from the current center-of-gravity; returns true on success, false if APU offline or if shift is maxed out
    virtual double GetCenterOfGravity() const = 0;                   // 0.0 = centered; +/- max value varies by vessel

    // RCS Mode
    virtual bool SetRCSDockingMode(bool on) = 0;  // set or clear RCS docking mode; returns true on success, false if RCS docking mode not supported or mode switch inhibited
    virtual bool IsRCSDockingMode() const = 0;    // returns true if RCS DOCKING mode is active, false if RCS is in NORMAL mode

    // Active EVA port
    virtual bool SetElevatorEVAPortActive(bool on) = 0;  // true = crew elevator active, false = docking port active.  Returns true on success, false if crew elevator not supported.
    virtual bool IsElevatorEVAPortActive() const = 0;    // returns true if crew elevator is the active EVA port, false if the docking port is active

    // Retrieve info/warning lines on status screen and tertiary HUD
    // linesOut will be populated with the lines on the status screen/tertiary HUD, delimited by \r\n
    // maxLinesToRetrieve = 0-64.
    // NOTE: linesOut should contain space for at least 50 bytes per line retrieved.  XR HUDs display only the seven most-recent lines.
    // Returns: # of lines copied to linesOut
    virtual int GetStatusScreenText(char *pLinesOut, const int maxLinesToRetrieve) const = 0;

    //=====================================================================
    // Methods added in API version 2.01
    //=====================================================================
    // Returns a pointer to this vessel's OMMUManagement object; will only be NULL if this vessel does not support MMu.
    virtual OMMUManagement *GetMMuObject() = 0;

    //=====================================================================
    // Methods added in API version 2.1
    //=====================================================================
    // Writes a method to the tertiary HUD.  If isWarning == true, message will be in red.
    virtual void WriteTertiaryHudMessage(const char *pMessage, const bool isWarning) = 0;   // Note: '&' character in the message string will generate a newline; tertiary HUD has approximately 38 characters per line.

    //=====================================================================
    // Methods added in API version 2.2
    //=====================================================================
    // Returns the name of the custom skin loaded for this vessel, if any.  NULL = no custom skin loaded.  
    // e.g., if "SKIN foobar" is in the scenario file for this XR vessel, GetCustomSkinName() returns a pointer to the string "foobar".
    virtual const char *GetCustomSkinName() const = 0;

    //=====================================================================
    // Methods added in API version 3.0
    //=====================================================================
    // Returns the total number of payload bay slots in this XR vessel (1-n)
    virtual int GetPayloadBaySlotCount() const = 0;

    // Returns true if the specified payload bay slot is free
    //   slotNumber: 1 <= n <= GetPayloadBaySlotCount()
    virtual bool IsPayloadBaySlotFree(const int slotNumber) const = 0;

    // Returns details about an XR payload bay slot.  Returns NULL if slotNumber is invalid.
    //   slotNumber: 1 <= n <= GetPayloadBaySlotCount()
    //   slotDataOut: will be populated with data for the specified slot if slotNumber is valid.  If slotNumber is invalied, the contents of slotDataOut are not changed.
    // Returns: true if slotDataOut successfully populated (i.e., slotNumber was valid)
    virtual bool GetPayloadSlotData(const int slotNumber, XRPayloadSlotData &slotDataOut) = 0;

    // Returns true if the supplied vessel 1) is an XR-compatible payload vessel, and 2) can be latched into the specified slot (i.e., it will fit)
    // Note: this does NOT check the status of the payload bay doors: it is the caller's responsibility to do that if desired.
    //   slotNumber: 1 <= n <= GetPayloadBaySlotCount()
    virtual bool CanAttachPayload(const OBJHANDLE hPayloadVessel, const int slotNumber) const = 0;

    // Attempts to grapple the given payload vessel into the specified slot; there is no distance checks, although there is a size check.
    // Note: hPayloadVessel must be an XR payload module in order to be gappled into the payload bay.
    // Note: this does NOT check the status of the payload bay doors: it is the caller's responsibility to do that if desired.
    //   slotNumber: 1 <= n <= GetPayloadBaySlotCount()
    // Returns: true on success, false if payload could not be grappled into the requested slot
    virtual bool GrapplePayloadModuleIntoSlot(const OBJHANDLE hPayloadVessel, const int slotNumber) = 0;

    // Detaches a payload vessel from the specified slot at the specified delta-V along the Y axis (up out of the bay).
    // You should normally only call this on vessels that are in space.
    // Note: this does NOT check the status of the payload bay doors: it is the caller's responsibility to do that if desired.
    //   slotNumber: 1 <= n <= GetPayloadBaySlotCount()
    //   deltaV = +Y deployment velocity in meters-per-second (e.g., 0.2)
    // Returns: true on success, false if slot is invalid or no payload is attached in the specified slot
    virtual bool DeployPayloadInFlight(const int slotNumber, const double deltaV) = 0;

    // Detaches a payload vessel from the specified slot and moves it to alongside the ship on the ground.
    // You should normally only call this on vessels that are landed and stationary.
    // Note: this does NOT check the status of the payload bay doors: it is the caller's responsibility to do that if desired.
    //   slotNumber: 1 <= n <= GetPayloadBaySlotCount()
    // Returns: true on success, false if slot is invalid or no payload is attached in the specified slot
    virtual bool DeployPayloadWhileLanded(const int slotNumber) = 0;

    // Detaches all payload vessels at the specified delta-V along the Y axis (up out of the bay).
    // You should normally only call this on vessels that are in space.
    // Note: this does NOT check the status of the payload bay doors: it is the caller's responsibility to do that if desired.
    //   deltaV = +Y deployment velocity in meters-per-second (e.g., 0.2)
    // Returns: number of payload vessels deployed
    virtual int DeployAllPayloadInFlight(const double deltaV) = 0;

    // Detaches all payload vessels and moves them to alongside the ship on the ground.
    // You should normally only call this on vessels that are landed and stationary.
    // Note: this does NOT check the status of the payload bay doors: it is the caller's responsibility to do that if desired.
    // Returns: number of payload vessels deployed
    virtual int DeployAllPayloadWhileLanded() = 0;

    // Enables or disables MWS test mode (i.e., same as pressing or releasing the 'Test' MWS button)
    // Returns: previous state of MWS test mode
    virtual bool SetMWSTest(bool bTestMode) = 0;
    
    // Returns true if 'recenter the center-of-gravity' mode is enabled
    virtual bool GetRecenterCOGMode() const = 0;

    // Enable or disable 'recenter the center-of-gravity' mode.
    // Returns: true on success, false if mode could not be set because no pilot is on board
    virtual bool SetRecenterCOGMode(const bool bEnableRecenterMode) = 0;

    // Returns the status of the external cooling line.
    virtual XRDoorState GetExternalCoolingState() const = 0;

    // Deploys or retracts the external cooling line and shows a success or failure message on the secondary HUD.
    // Returns: true on success, false on error
    virtual bool SetExternalCoolingState(const bool bEnabled) = 0;

    // Sets fuel cross-feed mode and shows a status message on the secondary HUD
    //   state: XF_MAIN, XF_OFF, or XF_RCS
    // Returns: true on success, false if state is invalid or no crew members on board
    virtual bool SetCrossFeedMode(XRXFEED_STATE state) = 0;

    //=====================================================================

    // TODO: add resupply / refueling support later as necessary
};
