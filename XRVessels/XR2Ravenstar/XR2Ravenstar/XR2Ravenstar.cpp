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
// XR2Ravenstar implementation class
//
// XR2Ravenstar.cpp
// ==============================================================

#define ORBITER_MODULE

#include "XR2Ravenstar.h"
#include "DlgCtrl.h"

#include <stdio.h>
#include "XR2AreaIDs.h"  
#include "XR2InstrumentPanels.h"
#include "XR2Globals.h"
#include "XRPayload.h"
#include "XR2PayloadBay.h"
// TODO: #include "XR2PayloadDialog.h"

// ==============================================================
// API callback interface
// ==============================================================

// --------------------------------------------------------------
// Module initialisation
// --------------------------------------------------------------
DLLCLBK void InitModule (HINSTANCE hModule)
{
    g_hDLL = hModule;
    oapiRegisterCustomControls(hModule);
}

// --------------------------------------------------------------
// Module cleanup
// NOTE: this is called even if fast shutdown is enabled.
// --------------------------------------------------------------
DLLCLBK void ExitModule (HINSTANCE hModule)
{
    oapiUnregisterCustomControls(hModule);
    XRPayloadClassData::Terminate();     // clean up global cache
}

// --------------------------------------------------------------
// Vessel initialisation
// --------------------------------------------------------------
DLLCLBK VESSEL *ovcInit (OBJHANDLE vessel, int flightmodel)
{
#ifdef _DEBUG
    // NOTE: _CRTDBG_CHECK_ALWAYS_DF is too slow
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF |
                   _CRTDBG_CHECK_CRT_DF | 
                   _CRTDBG_LEAK_CHECK_DF); 
#endif

    return new XR2Ravenstar(vessel, flightmodel, new XR2ConfigFileParser());
}

// --------------------------------------------------------------
// Vessel cleanup
// Note: this is only called if fast shutdown is DISABLED.
// --------------------------------------------------------------

// NOTE: must receive this as a VESSEL2 ptr because that's how Orbiter calls it
DLLCLBK void ovcExit(VESSEL2 *vessel)
{
    // NOTE: in order to free up VESSEL3 data without this hack you would need to add an empty virtual destructor to the VESSEL2 class in VesselAPI.h.
    // Unfortunately that cannot occur now (from Martin) because it would break all existing add-ons linked against VESSEL2 (virtual table change).
    
    // This is a hack so that the VESSEL3_EXT, XR2Ravenstar, and VESSEL3 destructors will be invoked: it must
    // "skip over" the VESSEL2 class that does not have a virtual destructor and jump down to ours.
    // Invokes XR2Ravenstar destructor -> VESSEL3_EXT destructor -> VESSEL3 destructor
    VESSEL3_EXT *pXR2 = reinterpret_cast<VESSEL3_EXT *>( (((void **)vessel)-1) );  // bump vptr to VESSEL3_EXT subclass, which has a virtual destructor
    delete pXR2;
}

// ==============================================================
// Airfoil coefficient functions
// Return lift, moment and zero-lift drag coefficients as a
// function of angle of attack (alpha or beta)
//
// Note: these must be static so name collisions will not occur in subclasses.
// ==============================================================

// DG org: 0.015
// XR1 org: 0.030
#define PROFILE_DRAG 0.015

// 1. vertical lift component (wings and body)
static void VLiftCoeff (VESSEL *v, double aoa, double M, double Re, void *context, double *cl, double *cm, double *cd)
{
    const int nabsc = 9;
    static const double AOA[nabsc] =         {-180*RAD,-60*RAD,-30*RAD, -1*RAD, 15*RAD,20*RAD,25*RAD,50*RAD,180*RAD};

    static const double CL[nabsc]  =         {       0,      0,   -0.15,      0,    0.7,     0.5, 0.2,     0,      0};  // decrease negative lift to better hold negative pitch

    static const double CM[nabsc]  =         {       0,  0.006,  0.014, 0.0034,-0.0054,-0.024,-0.00001,   0,      0};

    int i=0;    
    for (i = 0; i < nabsc-1 && AOA[i+1] < aoa; i++);
    double f = (aoa-AOA[i]) / (AOA[i+1]-AOA[i]);
    *cl = CL[i] + (CL[i+1]-CL[i]) * f;  // aoa-dependent lift coefficient
    *cm = CM[i] + (CM[i+1]-CM[i]) * f;  // aoa-dependent moment coefficient
    double saoa = sin(aoa);
    double pd = PROFILE_DRAG + 0.4*saoa*saoa;  // profile drag
    // profile drag + (lift-)induced drag + transonic/supersonic wave (compressibility) drag
    *cd = pd + oapiGetInducedDrag (*cl, WING_ASPECT_RATIO, WING_EFFICIENCY_FACTOR) + oapiGetWaveDrag (M, 0.75, 1.0, 1.1, 0.04);
}

// 2. horizontal lift component (vertical stabilisers and body)
static void HLiftCoeff (VESSEL *v, double beta, double M, double Re, void *context, double *cl, double *cm, double *cd)
{
    const int nabsc = 8;
    static const double BETA[nabsc] = {-180*RAD,-135*RAD,-90*RAD,-45*RAD,45*RAD,90*RAD,135*RAD,180*RAD};
    static const double CL[nabsc]   = {       0,    +0.3,      0,   -0.3,  +0.3,     0,   -0.3,      0};

    int i=0;    
    for (i = 0; i < nabsc-1 && BETA[i+1] < beta; i++);
    *cl = CL[i] + (CL[i+1]-CL[i]) * (beta-BETA[i]) / (BETA[i+1]-BETA[i]);
    *cm = 0.0;
    *cd = PROFILE_DRAG + oapiGetInducedDrag (*cl, 1.5, 0.6) + oapiGetWaveDrag (M, 0.75, 1.0, 1.1, 0.04);
}

//
// Constructor
//
XR2Ravenstar::XR2Ravenstar(OBJHANDLE hObj, int fmodel, XR2ConfigFileParser *pConfigFileParser) : 
    DeltaGliderXR1(hObj, fmodel, pConfigFileParser)
{
    // init new XR2 warning lights
    for (int i=0; i < XR2_WARNING_LIGHT_COUNT; i++)
        m_xr2WarningLights[i] = false;  // not lit

    // init new doors
    bay_status = DoorStatus::DOOR_CLOSED;
    bay_proc   = 0.0;

    // replace the data HUD font with a smaller one
    // XR1 ORG: m_pDataHudFont = CreateFont(20, 0, 0, 0, 700, 0, 0, 0, 0, 0, 0, NONANTIALIASED_QUALITY, 0, "Tahoma");
    // XR1 ORG: m_pDataHudFontSize = 22;      // includes spacing
    /* MATCHES XR1 FONT NOW 
    oapiReleaseFont(m_pDataHudFont);        // free existing front created by the XR1's constructor
    m_pDataHudFont = oapiCreateFont(22, true, "Tahoma", FONT_BOLD);  // will be freed by the XR1's destructor
    m_pDataHudFontSize = 18;      // includes spacing
    */
}

// 
// Destructor
//
XR2Ravenstar::~XR2Ravenstar()
{
    // Note: payload bay is cleaned up in our XRVessel base class
}

// Create control surfaces for any damageable control surface handles below that are zero (all are zero before vessel initialized).
// This is invoked from clbkSetClassCaps as well as ResetDamageStatus.
void XR2Ravenstar::ReinitializeDamageableControlSurfaces()
{
    // We have to cheat a little and move the ailerons out farther to improve roll performance in the atmosphere
    // so that our roll performance is comparable with the XR1.
    // AS MEASURED: const double aileronDeltaX         = 7.782;     // distance from center of ship to center of aileron, X direction
    const double aileronDeltaX         = 7.782 + 2.0; // distance from center of ship to center of aileron, X direction

    if (hElevator == 0)
    {
        // ORG: CreateControlSurface(AIRCTRL_ELEVATOR,     1.2 * XR1Multiplier, 1.4, _V(   0,0, controlSurfacesDeltaZ), AIRCTRL_AXIS_XPOS, anim_elevator);
        hElevator = CreateControlSurface2(AIRCTRL_ELEVATOR, m_baselineElevatorArea, m_elevatorCL, _V(   0,0, m_ctrlSurfacesDeltaZ), AIRCTRL_AXIS_XPOS, anim_elevator);
    }

    if (hLeftAileron == 0)
    {
        hLeftAileron = CreateControlSurface2 (AIRCTRL_AILERON, 0.2 * XR1Multiplier * 1.50, 1.5, _V( aileronDeltaX, 0, m_ctrlSurfacesDeltaZ), AIRCTRL_AXIS_XPOS, anim_raileron);
    }

    if (hRightAileron == 0)
    {
        hRightAileron = CreateControlSurface2 (AIRCTRL_AILERON, 0.2 * XR1Multiplier * 1.50, 1.5, _V(-aileronDeltaX, 0, m_ctrlSurfacesDeltaZ), AIRCTRL_AXIS_XNEG, anim_laileron);
    }
    
    if (hElevatorTrim == 0)
    {
        // Note: XR5 was 0.3 * XR1Multiplier * 7 to help autopilot in the atmosphere.
        hElevatorTrim = CreateControlSurface2 (AIRCTRL_ELEVATORTRIM, 0.3 * XR1Multiplier, 1.5, _V(   0,0, m_ctrlSurfacesDeltaZ), AIRCTRL_AXIS_XPOS, anim_elevatortrim);
    }
}

// Used for internal development testing only to tweak some internal value.
// This is invoked from the key handler as ALT-1 or ALT-2 are held down.  
// direction = true: increment value, false: decrement value
void XR2Ravenstar::TweakInternalValue(bool direction)
{
#ifdef _DEBUG  // debug only!
#if 0
    const double stepSize = (oapiGetSimStep() * ELEVATOR_TRIM_SPEED * 0.02);
    
    // tweak hidden elevator trim to fix nose-up push
    double step = stepSize * (direction ? 1.0 : -1.0);
    m_hiddenElevatorTrimState += step;

    if (m_hiddenElevatorTrimState < -1.0)
        m_hiddenElevatorTrimState = -1.0;
    else if (m_hiddenElevatorTrimState > 1.0)
        m_hiddenElevatorTrimState = 1.0;

    SetControlSurfaceLevel(AIRCTRL_FLAP, m_hiddenElevatorTrimState);     
    sprintf(oapiDebugString(), "Hidden trim=%lf", m_hiddenElevatorTrimState);
#endif
#if 0
    const double stepSize = (oapiGetSimStep() * 0.5);  // 2 seconds to go from 0...1
    m_tweakedInternalValue += (direction ? stepSize : -stepSize);
    sprintf(oapiDebugString(), "tweakedInternalValue=%lf", m_tweakedInternalValue);
#endif
#if 0  
    // heating mesh testing #1
    if (heatingmesh)
    {
        const double stepSize = (oapiGetSimStep() * 0.1);  // 10 seconds to go from 0...1
        m_tweakedInternalValue += (direction ? stepSize : -stepSize);
        sprintf(oapiDebugString(), "tweakedInternalValue=%lf", m_tweakedInternalValue); 

        oapiSetMeshProperty(heatingmesh, MESHPROPERTY_MODULATEMATALPHA, 1); // use material alpha w/texture alpha
        MATERIAL *pHeatingMaterial = oapiMeshMaterial(heatingmesh, 0);
        pHeatingMaterial->diffuse.a = static_cast<float>(m_tweakedInternalValue);
        pHeatingMaterial->ambient.a = static_cast<float>(m_tweakedInternalValue);
        pHeatingMaterial->specular.a = static_cast<float>(m_tweakedInternalValue);
        pHeatingMaterial->emissive.a = static_cast<float>(m_tweakedInternalValue);
    }
#endif
#if 0 
    // heating mesh testing #2 (set nosecone temp in tweaked value (must update SetHullTempsPostStep as well))
    const double stepSize = (oapiGetSimStep() * 100);  // 100 degrees per second
    m_tweakedInternalValue += (direction ? stepSize : -stepSize);
    sprintf(oapiDebugString(), "tweakedInternalValue=%lf", m_tweakedInternalValue); 
#endif
#if 0
    // play demo sound
    ShowInfo("rocketmain.wav", "Playing rocket sound!");
#endif
#if 0
    // 2 = enable value, 1 = disable value
    m_tweakedInternalValue = static_cast<double>(direction);
#endif

#endif
}

// --------------------------------------------------------------
// Apply custom skin to the current mesh instance
// --------------------------------------------------------------
void XR2Ravenstar::ApplySkin ()
{
    if (!exmesh) return;

    // NOTE: this core bug is fixed in the new Orbiter version (2009), so this workaround is no longer necessary
    // TODO FOR 1.1: test workaround for CTD after vessel switching
    // THIS WORKS, but I need to figure out how to delete the old texture handle in clbkVisualDestroyed:
    /*
    SURFHANDLE hTexture = oapiLoadTexture("XR2Ravenstar\\Skins\\Red\\top_hull_colour_XR2_paint.dds");
    oapiSetTexture(exmesh, 3, hTexture);
    */

    // TODO: {ZZZ} update these texture indexes with each new mesh version!
    if (skin[0]) oapiSetTexture (exmesh, 3, skin[0]);   // top_hull_colour_XR2_paint.dds
    if (skin[1]) oapiSetTexture (exmesh, 1, skin[1]);   // bottom_hull.dds
}

// Set the camera to its default payload bay postion.
void XR2Ravenstar::ResetCameraToPayloadBay()
{
    /* ORG
    const VECTOR3 pos = { 0.0, 2.168, 3.153 };
    const VECTOR3 dir = { 0.0, -0.461, -0.887 };  // look down to rear bottom of bay
    */
    // pre-1.4: const VECTOR3 pos = { 0.0, 2.168 + 3.0, 3.153 + 2.0};    // above the bay
    const VECTOR3 pos = { 0.0, 2.168 + 3.0, 3.153 + 2.0};    // above the bay
    // BETA-1d: const VECTOR3 dir = { 0.0, -0.461, -0.887 };          
    const VECTOR3 dir = { 0.0, -0.624, -0.781 };             // look down to rear bottom of bay

    SetCameraOffset(pos);
    SetXRCameraDirection(dir); 
}

// handle instant jumps to open or closed here
#define CHECK_DOOR_JUMP(proc, anim) if (action == DoorStatus::DOOR_OPEN) proc = 1.0;            \
                                    else if (action == DoorStatus::DOOR_CLOSED) proc = 0.0;     \
                                    SetXRAnimation(anim, proc)

// state: 0=fully retracted, 1.0 = fully deployed
void XR2Ravenstar::SetGearParameters(double state)
{
    if (state == 1.0) // fully deployed?
    {
        const double touchdownDeltaX = 4.615;
        const double touchdownY = GEAR_UNCOMPRESSED_YCOORD + GEAR_COMPRESSION_DISTANCE; // gear height fully compressed
        // Orbiter 2010 P1 and earlier: const double touchdownRearZ = REAR_GEAR_ZCOORD;
        const double touchdownRearZ = REAR_GEAR_ZCOORD + 2.5;  // move main gear forward to assist rotation

        SetXRTouchdownPoints(_V(               0, touchdownY, NOSE_GEAR_ZCOORD),  // front
                             _V(-touchdownDeltaX, touchdownY, touchdownRearZ),    // left
                             _V( touchdownDeltaX, touchdownY, touchdownRearZ),    // right
						     WHEEL_FRICTION_COEFF, WHEEL_LATERAL_COEFF, true);
        SetNosewheelSteering(true);  // not really necessary since we have have a prestep constantly checking this
    } 
    else  // not fully deployed (belly landing!)
    {
        const double touchdownDeltaX = 1.701;
        const double touchdownZRear = -3.263;

        SetXRTouchdownPoints(_V(               0, -1.490, 2.847),               // front
                             _V(-touchdownDeltaX, -1.261, touchdownZRear),      // left
                             _V( touchdownDeltaX, -1.0, touchdownZRear),        // right (tilt the ship)
							 3.0, 3.0, false);		// belly landing!
        SetNosewheelSteering(false);  // not really necessary since we have have a prestep constantly checking this
    }

    // update the animation state
    gear_proc = state;
    SetXRAnimation(anim_gear, gear_proc);

     // redraw the gear indicator
    TriggerRedrawArea(AID_GEARINDICATOR);
}

// perform an EVA for the specified crew member
// Returns: true on success, false on error (crew member not present or outer airlock door is closed)
bool XR2Ravenstar::PerformEVA(const int mmuCrewMemberIndex)
{
    // set coordinates depending on whether we are landed or not
    VECTOR3 pos = { 0.0, DOCKING_PORT_COORD.y, DOCKING_PORT_COORD.z + 2.0 }; // this is the position where the Mmu will appear relative to the ship's local coordinates
    VECTOR3 rot = { 0.0, 0.0, 0.0 };  // straight up, facing forward
#ifdef MMU
    UMmu.SetEjectPosRotRelSpeed(pos, rot, _V(0, 4.0, 0));  // jumped UP to bail out @ 4 meters-per-second

    if (IsLanded())
        pos.y -= 1.0;   // so we are just above the ground when we EVA
        
    UMmu.SetMembersPosRotOnEVA(pos, rot);
#endif

    // perform the EVA
    return DeltaGliderXR1::PerformEVA(mmuCrewMemberIndex);  
}

// Now modify the elevator area if elevators are enabled and "dual-mode"
// elevator performance is enabled.
void XR2Ravenstar::ApplyElevatorAreaChanges()
{
    const DWORD mode = GetADCtrlMode();
    const XR2ConfigFileParser *parser = GetXR2Config();
    if (parser->EnableAFCtrlPerformanceModifier)
    {
        double modifier = 1.0;
        if (mode == 1)  // pitch?
            modifier = parser->AFCtrlPerformanceModifier[0];
        else if (mode == 7)  // on?
            modifier = parser->AFCtrlPerformanceModifier[1];

        // recreate (i.e., modify) the elevator control surface area to simulate limited deflection
        if (hElevator)   // not damaged?
        {
            DelControlSurface(hElevator);
            hElevator = CreateControlSurface2(AIRCTRL_ELEVATOR, m_baselineElevatorArea * modifier, m_elevatorCL, _V(0,0, m_ctrlSurfacesDeltaZ), AIRCTRL_AXIS_XPOS, anim_elevator);
        }

        // DEBUG: sprintf(oapiDebugString(), "elevator modifier: %lf", modifier);
    }
}

// {ZZZ} You may need to update this method whenever the mesh is recreated (in case the texture indices changed): do not delete this comment
// meshTextureID = vessel-specific constant that is translated to a texture index specific to our vessel's .msh file.  meshTextureID 
// NOTE: meshTextureID=VCPANEL_TEXTURE_NONE = -1 = "no texture" (i.e., "not applicable"); defined in Area.h.
// hMesh = OUTPUT: will be set to the mesh handle of the mesh associated with meshTextureID.
DWORD XR2Ravenstar::MeshTextureIDToTextureIndex(const int meshTextureID, MESHHANDLE &hMesh)
{
    // sanity check
    _ASSERTE(meshTextureID > VCPANEL_TEXTURE_NONE);

    DWORD retVal = 0;
#if 0   // {YYY} implement this method when the VC is implemented
    // same mesh for all VC textures
    hMesh = vcmesh_tpl;  // assign by reference

    switch (meshTextureID)
    {
    case XR1_VCPANEL_TEXTURE_LEFT:
        retVal = 18;      // was "tex2" in original DG code
        break;

    case XR1_VCPANEL_TEXTURE_CENTER:
        retVal = 16;      // was "tex1" in original DG code
        break;

    case XR1_VCPANEL_TEXTURE_RIGHT:
        retVal = 14;      // was "tex3" in original DG code
        break;

    default:   // should never happen!
        _ASSERTE(false);
        // fall through with retVal 0
        break;
    }

     // validate return values
    _ASSERTE(retVal >= 0);
    _ASSERTE(hMesh != nullptr);
#endif
    return retVal;
}
