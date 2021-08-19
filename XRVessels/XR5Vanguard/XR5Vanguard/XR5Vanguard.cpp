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
// XR5Vanguard implementation class
//
// XR5Vanguard.cpp
// ==============================================================

#define ORBITER_MODULE

#include "XR5Vanguard.h"
#include "DlgCtrl.h"
#include <stdio.h>

#include "XR5AreaIDs.h"  
#include "XR5Globals.h"

#include "meshres.h"

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

    return new XR5Vanguard(vessel, flightmodel, new XR5ConfigFileParser());
}

// --------------------------------------------------------------
// Vessel cleanup
// --------------------------------------------------------------

// NOTE: must receive this as a VESSEL2 ptr because that's how Orbiter calls it
DLLCLBK void ovcExit(VESSEL2 *vessel)
{
    // NOTE: in order to free up VESSEL3 data, you must add an empty virtual destructor to the VESSEL2 class in VesselAPI.h
    
    // This is a hack so that the VESSEL3_EXT, XR5 and VESSEL3 destructors will be invoked.
    // Invokes XR5 destructor -> XR1 destructor -> VESSEL3_EXT destructor -> VESSEL3 destructor
    VESSEL3_EXT *pXR5 = reinterpret_cast<VESSEL3_EXT *>( (((void **)vessel)-1) );  // bump vptr to VESSEL3_EXT subclass, which has a virtual destructor
    delete pXR5;
}

// ==============================================================
// Airfoil coefficient functions
// Return lift, moment and zero-lift drag coefficients as a
// function of angle of attack (alpha or beta)
// ==============================================================

///#define PROFILE_DRAG 0.015
// NEW to fix "floaty" landings
// XR5 ORG: #define PROFILE_DRAG 0.030
// improve glide performance for the Vanguard
#define PROFILE_DRAG 0.015

// 1. vertical lift component (wings and body)
void VLiftCoeff (VESSEL *v, double aoa, double M, double Re, void *context, double *cl, double *cm, double *cd)
{
    const int nabsc = 9;
    static const double AOA[nabsc] =         {-180*RAD,-60*RAD,-30*RAD, -1*RAD, 15*RAD,20*RAD,25*RAD,50*RAD,180*RAD};

    static const double CL[nabsc]  =         {       0,      0,   -0.15,      0,    0.7,     0.5, 0.2,     0,      0};  // decrease negative lift to fix nose-down attitude hold problems

    static const double CM[nabsc]  =         {       0,      0,  0.014, 0.0039, -0.006,-0.008,-0.010,     0,      0};

    int i=0;    
    for (i = 0; i < nabsc-1 && AOA[i+1] < aoa; i++);
    double f = (aoa-AOA[i]) / (AOA[i+1]-AOA[i]);
    *cl = CL[i] + (CL[i+1]-CL[i]) * f;  // aoa-dependent lift coefficient
    *cm = CM[i] + (CM[i+1]-CM[i]) * f;  // aoa-dependent moment coefficient
    double saoa = sin(aoa);
    double pd = PROFILE_DRAG + 0.4*saoa*saoa;  // profile drag
    *cd = pd + oapiGetInducedDrag (*cl, WING_ASPECT_RATIO, WING_EFFICIENCY_FACTOR) + oapiGetWaveDrag (M, 0.75, 1.0, 1.1, 0.04);
    // profile drag + (lift-)induced drag + transonic/supersonic wave (compressibility) drag
}

// 2. horizontal lift component (vertical stabilisers and body)

void HLiftCoeff (VESSEL *v, double beta, double M, double Re, void *context, double *cl, double *cm, double *cd)
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
XR5Vanguard::XR5Vanguard(OBJHANDLE hObj, int fmodel, XR5ConfigFileParser *pConfigFileParser) : 
    DeltaGliderXR1(hObj, fmodel, pConfigFileParser),
    m_rcsDockingMode(false), m_rcsDockingModeAtKillrotStart(false),
    m_hiddenElevatorTrimState(0), m_activeEVAPort(ACTIVE_EVA_PORT::DOCKING_PORT)
{
    // init new XR5 warning lights
    for (int i=0; i < XR5_WARNING_LIGHT_COUNT; i++)
        m_xr5WarningLights[i] = false;  // not lit

    // init new doors
    crewElevator_status = DoorStatus::DOOR_CLOSED;
    crewElevator_proc   = 0.0;
    bay_status          = DoorStatus::DOOR_CLOSED;
    bay_proc            = 0.0;

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
XR5Vanguard::~XR5Vanguard()
{
    // Note: our superclass handles cleanup of m_pPayloadBay and s_hPayloadEditorDialog
}

// ==============================================================
// Overloaded callback functions
// ==============================================================

// Create control surfaces for any damageable control surface handles below that are zero (all are zero before vessel initialized).
// This is invoked from clbkSetClassCaps as well as ResetDamageStatus.
void XR5Vanguard::ReinitializeDamageableControlSurfaces()
{
    if (hElevator == 0)
    {
        hElevator = CreateControlSurface2(AIRCTRL_ELEVATOR,     1.2 * XR1Multiplier * 3, 1.4, _V(   0,0, m_ctrlSurfacesDeltaZ), AIRCTRL_AXIS_XPOS, anim_elevator);
    }

    if (hLeftAileron == 0)
    {
        hLeftAileron = CreateControlSurface2 (AIRCTRL_AILERON, 0.2 * XR1Multiplier * 2, 1.5, _V( m_aileronDeltaX, 0, m_ctrlSurfacesDeltaZ), AIRCTRL_AXIS_XPOS, anim_raileron);
    }

    if (hRightAileron == 0)
    {
        hRightAileron = CreateControlSurface2 (AIRCTRL_AILERON, 0.2 * XR1Multiplier * 2, 1.5, _V(-m_aileronDeltaX, 0, m_ctrlSurfacesDeltaZ), AIRCTRL_AXIS_XNEG, anim_laileron);
    }
    
    if (hElevatorTrim == 0)
    {
        hElevatorTrim = CreateControlSurface2 (AIRCTRL_ELEVATORTRIM, 0.3 * XR1Multiplier * 7, 1.5, _V(   0,0, m_ctrlSurfacesDeltaZ), AIRCTRL_AXIS_XPOS, anim_elevatortrim);
    }
}

// Superclass' clbkPreStep and clbkPostStep are all we need

// ==============================================================
// Message callback function for control dialog box
// ==============================================================

INT_PTR CALLBACK XR5Ctrl_DlgProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    XR5Vanguard *dg = (uMsg == WM_INITDIALOG ? reinterpret_cast<XR5Vanguard *>(lParam) : reinterpret_cast<XR5Vanguard *>(oapiGetDialogContext(hWnd)));
    // pointer to vessel instance was passed as dialog context
    
    switch (uMsg) {
    /* Note: for some reason Orbiter appears to be trapping keystrokes, so this will not work.
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
            oapiCloseDialog(hWnd);   // bye, bye
        break;  // pass it on
    */

    case WM_INITDIALOG:
        dg->UpdateCtrlDialog(dg, hWnd);
        return FALSE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            oapiCloseDialog (hWnd);
            return TRUE;

        case IDC_GEAR_UP:
            dg->ActivateLandingGear(DoorStatus::DOOR_CLOSING);
            return 0;
        case IDC_GEAR_DOWN:
            dg->ActivateLandingGear(DoorStatus::DOOR_OPENING);
            return 0;

        case IDC_RETRO_CLOSE:
            dg->ActivateRCover(DoorStatus::DOOR_CLOSING);
            return 0;
        case IDC_RETRO_OPEN:
            dg->ActivateRCover(DoorStatus::DOOR_OPENING);
            return 0;

        case IDC_BAY_CLOSE:
            dg->ActivateBayDoors(DoorStatus::DOOR_CLOSING);
            return 0;
        case IDC_BAY_OPEN:
            dg->ActivateBayDoors(DoorStatus::DOOR_OPENING);
            return 0;

        case IDC_OLOCK_CLOSE:
            dg->ActivateOuterAirlock(DoorStatus::DOOR_CLOSING);
            return 0;
        case IDC_OLOCK_OPEN:
            dg->ActivateOuterAirlock(DoorStatus::DOOR_OPENING);
            return 0;

        case IDC_ILOCK_CLOSE:
            dg->ActivateInnerAirlock(DoorStatus::DOOR_CLOSING);
            return 0;
        case IDC_ILOCK_OPEN:
            dg->ActivateInnerAirlock(DoorStatus::DOOR_OPENING);
            return 0;

        case IDC_HOVER_CLOSE:
            dg->ActivateHoverDoors(DoorStatus::DOOR_CLOSING);
            return 0;
        case IDC_HOVER_OPEN:
            dg->ActivateHoverDoors(DoorStatus::DOOR_OPENING);
            return 0;

        case IDC_DOCKING_STOW:
            dg->ActivateNoseCone(DoorStatus::DOOR_CLOSING);
            return 0;
        case IDC_DOCKING_DEPLOY:
            dg->ActivateNoseCone(DoorStatus::DOOR_OPENING);
            return 0;

        case IDC_ELEVATOR_STOW:
            dg->ActivateElevator(DoorStatus::DOOR_CLOSING);
            return 0;
        case IDC_ELEVATOR_DEPLOY:
            dg->ActivateElevator(DoorStatus::DOOR_OPENING);
            return 0;

        case IDC_SCRAM_CLOSE:
            dg->ActivateScramDoors(DoorStatus::DOOR_CLOSING);
            return 0;
        case IDC_SCRAM_OPEN:
            dg->ActivateScramDoors(DoorStatus::DOOR_OPENING);
            return 0;

        case IDC_HATCH_CLOSE:
            dg->ActivateHatch(DoorStatus::DOOR_CLOSING);
            return 0;
        case IDC_HATCH_OPEN:
            dg->ActivateHatch(DoorStatus::DOOR_OPENING);
            return 0;

        case IDC_RADIATOR_STOW:
            dg->ActivateRadiator(DoorStatus::DOOR_CLOSING);
            return 0;
        case IDC_RADIATOR_DEPLOY:
            dg->ActivateRadiator(DoorStatus::DOOR_OPENING);
            return 0;

        case IDC_NAVLIGHT:
            dg->SetNavlight (SendDlgItemMessage (hWnd, IDC_NAVLIGHT, BM_GETCHECK, 0, 0) == BST_CHECKED);
            return 0;
        case IDC_BEACONLIGHT:
            dg->SetBeacon (SendDlgItemMessage (hWnd, IDC_BEACONLIGHT, BM_GETCHECK, 0, 0) == BST_CHECKED);
            return 0;
        case IDC_STROBELIGHT:
            dg->SetStrobe (SendDlgItemMessage (hWnd, IDC_STROBELIGHT, BM_GETCHECK, 0, 0) == BST_CHECKED);
            return 0;
        }
        break;
    }
    return oapiDefDialogProc(hWnd, uMsg, wParam, lParam);
}

void XR5Vanguard::UpdateCtrlDialog(XR5Vanguard *dg, HWND hWnd)
{
    static int bstatus[2] = {BST_UNCHECKED, BST_CHECKED};
    
    if (hWnd == nullptr) 
        hWnd = oapiFindDialog (g_hDLL, IDD_CTRL);

    if (hWnd == nullptr) 
        return;

    int op;
    
    op = static_cast<int>(dg->gear_status) & 1;
    SendDlgItemMessage (hWnd, IDC_GEAR_DOWN, BM_SETCHECK, bstatus[op], 0);
    SendDlgItemMessage (hWnd, IDC_GEAR_UP, BM_SETCHECK, bstatus[1-op], 0);
    
    op = static_cast<int>(dg->rcover_status) & 1;
    SendDlgItemMessage (hWnd, IDC_RETRO_OPEN, BM_SETCHECK, bstatus[op], 0);
    SendDlgItemMessage (hWnd, IDC_RETRO_CLOSE, BM_SETCHECK, bstatus[1-op], 0);
    
    op = static_cast<int>(dg->bay_status) & 1;
    SendDlgItemMessage (hWnd, IDC_BAY_OPEN, BM_SETCHECK, bstatus[op], 0);
    SendDlgItemMessage (hWnd, IDC_BAY_CLOSE, BM_SETCHECK, bstatus[1-op], 0);

    op = static_cast<int>(dg->olock_status) & 1;
    SendDlgItemMessage (hWnd, IDC_OLOCK_OPEN, BM_SETCHECK, bstatus[op], 0);
    SendDlgItemMessage (hWnd, IDC_OLOCK_CLOSE, BM_SETCHECK, bstatus[1-op], 0);
    
    op = static_cast<int>(dg->ilock_status) & 1;
    SendDlgItemMessage (hWnd, IDC_ILOCK_OPEN, BM_SETCHECK, bstatus[op], 0);
    SendDlgItemMessage (hWnd, IDC_ILOCK_CLOSE, BM_SETCHECK, bstatus[1-op], 0);

    op = static_cast<int>(dg->hoverdoor_status) & 1;
    SendDlgItemMessage (hWnd, IDC_HOVER_OPEN, BM_SETCHECK, bstatus[op], 0);
    SendDlgItemMessage (hWnd, IDC_HOVER_CLOSE, BM_SETCHECK, bstatus[1-op], 0);

    op = static_cast<int>(dg->nose_status) & 1;
    SendDlgItemMessage (hWnd, IDC_DOCKING_DEPLOY, BM_SETCHECK, bstatus[op], 0);
    SendDlgItemMessage (hWnd, IDC_DOCKING_STOW, BM_SETCHECK, bstatus[1-op], 0);
    
    op = static_cast<int>(dg->crewElevator_status) & 1;
    SendDlgItemMessage (hWnd, IDC_ELEVATOR_DEPLOY, BM_SETCHECK, bstatus[op], 0);
    SendDlgItemMessage (hWnd, IDC_ELEVATOR_STOW, BM_SETCHECK, bstatus[1-op], 0);
    
    op = static_cast<int>(dg->scramdoor_status) & 1;
    SendDlgItemMessage (hWnd, IDC_SCRAM_OPEN, BM_SETCHECK, bstatus[op], 0);
    SendDlgItemMessage (hWnd, IDC_SCRAM_CLOSE, BM_SETCHECK, bstatus[1-op], 0);

    op = static_cast<int>(dg->hatch_status) & 1;
    SendDlgItemMessage (hWnd, IDC_HATCH_OPEN, BM_SETCHECK, bstatus[op], 0);
    SendDlgItemMessage (hWnd, IDC_HATCH_CLOSE, BM_SETCHECK, bstatus[1-op], 0);
    
    op = static_cast<int>(dg->radiator_status) & 1;
    SendDlgItemMessage (hWnd, IDC_RADIATOR_DEPLOY, BM_SETCHECK, bstatus[op], 0);
    SendDlgItemMessage (hWnd, IDC_RADIATOR_STOW, BM_SETCHECK, bstatus[1-op], 0);
    
    op = static_cast<int>(dg->beacon[0].active) ? 1:0;
    SendDlgItemMessage (hWnd, IDC_NAVLIGHT, BM_SETCHECK, bstatus[op], 0);
    op = static_cast<int>(dg->beacon[3].active) ? 1:0;
    SendDlgItemMessage (hWnd, IDC_BEACONLIGHT, BM_SETCHECK, bstatus[op], 0);
    op = static_cast<int>(dg->beacon[5].active) ? 1:0;
    SendDlgItemMessage (hWnd, IDC_STROBELIGHT, BM_SETCHECK, bstatus[op], 0);
}

// toggle RCS docking mode
// rcsMode: true = set docking mode, false = set normal mode
// Returns: true if mode switched successfully, false if mode switch was inhibited
bool XR5Vanguard::SetRCSDockingMode(bool dockingMode)
{
    // if enabling docking mode and any autopilot is engaged, prohibit the change
    bool autopilotEngaged = false;
    if (dockingMode)
    {
        // check whether any standard autopilot is engaged
        for (int i=1; i <= 7; i++)
        {
            if (GetNavmodeState(i))
            {
                autopilotEngaged = true;   
                break;
            }
        }

        // check for any custom autopilot except for Airspeed Hold
        autopilotEngaged |= (m_customAutopilotMode != AUTOPILOT::AP_OFF);

        if (autopilotEngaged)
        {
            PlayErrorBeep();
            ShowWarning("RCS locked by Autopilot.wav", DeltaGliderXR1::ST_WarningCallout, "Autopilot is active: RCS mode is locked.");
            return false;
        }
    }

    ConfigureRCSJets(dockingMode);     
    PlaySound((dockingMode ? BeepHigh : BeepLow), DeltaGliderXR1::ST_Other);

    // play voice callout
    if (dockingMode)
        ShowInfo("RCS Config Docking.wav", DeltaGliderXR1::ST_InformationCallout, "RCS jets set to DOCKING configuration.");
    else
        ShowInfo("RCS Config Normal.wav", DeltaGliderXR1::ST_InformationCallout, "RCS jets set to NORMAL configuration.");
    
    return true;
}

// Configure RCS jets for docking or normal mode by configuring RCS thruster groups.  
// This method does NOT display any message or play any sounds; however, it does redraw then RCS mode light/switch.
// rcsMode: true = set docking mode, false = set normal mode
void XR5Vanguard::ConfigureRCSJets(bool dockingMode)
{
    // delete any existing RCS thruster groups
    DelThrusterGroup(THGROUP_ATT_PITCHUP);
    DelThrusterGroup(THGROUP_ATT_PITCHDOWN);
    DelThrusterGroup(THGROUP_ATT_UP);
    DelThrusterGroup(THGROUP_ATT_DOWN);

    DelThrusterGroup(THGROUP_ATT_YAWLEFT);
    DelThrusterGroup(THGROUP_ATT_YAWRIGHT);
    DelThrusterGroup(THGROUP_ATT_LEFT);
    DelThrusterGroup(THGROUP_ATT_RIGHT);

    DelThrusterGroup(THGROUP_ATT_BANKLEFT);
    DelThrusterGroup(THGROUP_ATT_BANKRIGHT);

    DelThrusterGroup(THGROUP_ATT_FORWARD);
    DelThrusterGroup(THGROUP_ATT_BACK);

    THRUSTER_HANDLE th_att_rot[4], th_att_lin[4];

    if (dockingMode == false)   
    {
        // NORMAL mode
        th_att_rot[0] = th_att_lin[0] = th_rcs[0];  // fore up
        th_att_rot[1] = th_att_lin[3] = th_rcs[1];  // aft down
        th_att_rot[2] = th_att_lin[2] = th_rcs[2];  // fore down
        th_att_rot[3] = th_att_lin[1] = th_rcs[3];  // aft up
        CreateThrusterGroup (th_att_rot,   2, THGROUP_ATT_PITCHUP);   // rotate UP on X axis (+x)
        CreateThrusterGroup (th_att_rot+2, 2, THGROUP_ATT_PITCHDOWN); // rotate DOWN on X axis (-x)
        CreateThrusterGroup (th_att_lin,   2, THGROUP_ATT_UP);        // translate UP along Y axis (+y)
        CreateThrusterGroup (th_att_lin+2, 2, THGROUP_ATT_DOWN);      // translate DOWN along Y axis (-y)

        th_att_rot[0] = th_att_lin[0] = th_rcs[4];  // fore left
        th_att_rot[1] = th_att_lin[3] = th_rcs[5];  // aft right
        th_att_rot[2] = th_att_lin[2] = th_rcs[6];  // fore right
        th_att_rot[3] = th_att_lin[1] = th_rcs[7];  // aft left
        CreateThrusterGroup (th_att_rot,   2, THGROUP_ATT_YAWLEFT);   // rotate LEFT on Y axis (-y)
        CreateThrusterGroup (th_att_rot+2, 2, THGROUP_ATT_YAWRIGHT);  // rotate RIGHT on Y axis (+y)
        CreateThrusterGroup (th_att_lin,   2, THGROUP_ATT_LEFT);      // translate LEFT along X axis (-x)
        CreateThrusterGroup (th_att_lin+2, 2, THGROUP_ATT_RIGHT);     // translate RIGHT along X axis (+x)

        th_att_rot[0] = th_rcs[8];     // right wing bottom
        th_att_rot[1] = th_rcs[9];     // left wing top
        th_att_rot[2] = th_rcs[10];    // left wing bottom
        th_att_rot[3] = th_rcs[11];    // right wing top
        CreateThrusterGroup (th_att_rot,   2, THGROUP_ATT_BANKLEFT);  // rotate LEFT on Z axis (-Z)
        CreateThrusterGroup (th_att_rot+2, 2, THGROUP_ATT_BANKRIGHT); // rotate RIGHT on Z axis (+Z)

        th_att_lin[0] = th_rcs[12];   // aft
        th_att_lin[1] = th_rcs[13];   // fore
        CreateThrusterGroup (th_att_lin,   1, THGROUP_ATT_FORWARD);   // translate FORWARD along Z axis (+z)
        CreateThrusterGroup (th_att_lin+1, 1, THGROUP_ATT_BACK);      // translate BACKWARD along Z axis (-z)
    }
    else  // DOCKING mode
    {
        // For DOCKING mode, the Z and Y axes are exchanged:
        // X axis remains UNCHANGED
        // +Y = +Z
        // -Y = -Z
        // +Z = +Y
        // -Z = -Y
        th_att_rot[0] = th_att_lin[0] = th_rcs[0];  // fore up
        th_att_rot[1] = th_att_lin[3] = th_rcs[1];  // aft down
        th_att_rot[2] = th_att_lin[2] = th_rcs[2];  // fore down
        th_att_rot[3] = th_att_lin[1] = th_rcs[3];  // aft up
        CreateThrusterGroup (th_att_rot,   2, THGROUP_ATT_PITCHUP);   // rotate UP on X axis (+x)
        CreateThrusterGroup (th_att_rot+2, 2, THGROUP_ATT_PITCHDOWN); // rotate DOWN on X axis (-x)
        CreateThrusterGroup (th_att_lin,   2, THGROUP_ATT_FORWARD);   // old: translate UP along Y axis (+y) = new: +Z
        CreateThrusterGroup (th_att_lin+2, 2, THGROUP_ATT_BACK);      // old: translate DOWN along Y axis (-y) = new: -Z

        th_att_rot[0] = th_att_lin[0] = th_rcs[4];  // fore left
        th_att_rot[1] = th_att_lin[3] = th_rcs[5];  // aft right
        th_att_rot[2] = th_att_lin[2] = th_rcs[6];  // fore right
        th_att_rot[3] = th_att_lin[1] = th_rcs[7];  // aft left
        CreateThrusterGroup (th_att_rot,   2, THGROUP_ATT_BANKRIGHT);  // old: rotate LEFT on Y axis (-y) = new: -Z
        CreateThrusterGroup (th_att_rot+2, 2, THGROUP_ATT_BANKLEFT);   // old: rotate RIGHT on Y axis (+y) = new: +Z
        CreateThrusterGroup (th_att_lin,   2, THGROUP_ATT_LEFT);       // translate LEFT along X axis (-x)
        CreateThrusterGroup (th_att_lin+2, 2, THGROUP_ATT_RIGHT);      // translate RIGHT along X axis (+x)

        th_att_rot[0] = th_rcs[8];     // right wing bottom
        th_att_rot[1] = th_rcs[9];     // left wing top
        th_att_rot[2] = th_rcs[10];    // left wing bottom
        th_att_rot[3] = th_rcs[11];    // right wing top
        CreateThrusterGroup (th_att_rot,   2, THGROUP_ATT_YAWLEFT);  // old: rotate LEFT on Z axis (+Z) = new: -Y
        CreateThrusterGroup (th_att_rot+2, 2, THGROUP_ATT_YAWRIGHT); // old: rotate RIGHT on Z axis (-Z) = new: +Z

        th_att_lin[0] = th_rcs[12];   // aft
        th_att_lin[1] = th_rcs[13];   // fore
        CreateThrusterGroup (th_att_lin,   1, THGROUP_ATT_DOWN);   // old: translate FORWARD along Z axis (+z) = new: -Y  
        CreateThrusterGroup (th_att_lin+1, 1, THGROUP_ATT_UP);     // old: translate BACKWARD along Z axis (-z) = new: +Y
    }

    // reset all thruster levels
    // NOTE: must take damage into account here!
    double rcsThrusterPowerFrac = (dockingMode? 0.40 : 1.0);  // reduce thruster power in docking mode
    for (int i=0; i < 14; i++)
    {
        // get integrity fraction
        int damageIntegrityIndex = static_cast<int>(DamageItem::RCS1) + i;    // 0 <= i <= 13
        DamageStatus ds = GetDamageStatus((DamageItem)damageIntegrityIndex);
        SetThrusterMax0(th_rcs[i], (GetRCSThrustMax(i) * rcsThrusterPowerFrac * ds.fracIntegrity));  
    }

    m_rcsDockingMode = dockingMode;     
    TriggerRedrawArea(AID_RCS_CONFIG_BUTTON);
}

// hook this so we can disable docking mode automatically
void XR5Vanguard::SetCustomAutopilotMode(AUTOPILOT mode, bool playSound, bool force)
{
    if (mode != AUTOPILOT::AP_OFF)
        ConfigureRCSJets(false);    // revert to normal mode

    DeltaGliderXR1::SetCustomAutopilotMode(mode, playSound, force); // do the work
}

// set the active EVA port
void XR5Vanguard::SetActiveEVAPort(ACTIVE_EVA_PORT newState)
{
    m_activeEVAPort = newState;

    // update the UMMu port coordinates and repaint the LEDs and the switch
    DefineMmuAirlock();
}

// state: 0=fully retracted, 1.0 = fully deployed
void XR5Vanguard::SetGearParameters(double state)
{
    if (state == 1.0) // fully deployed?
    {
        const double touchdownDeltaX = 16.283;
        const double touchdownY = GEAR_UNCOMPRESSED_YCOORD + GEAR_COMPRESSION_DISTANCE; // gear height fully compressed

        SetXRTouchdownPoints(_V(               0, touchdownY, NOSE_GEAR_ZCOORD),    // front
                             _V(-touchdownDeltaX, touchdownY, REAR_GEAR_ZCOORD),    // left
                             _V( touchdownDeltaX, touchdownY, REAR_GEAR_ZCOORD),    // right
							 WHEEL_FRICTION_COEFF, WHEEL_LATERAL_COEFF, true);
        SetNosewheelSteering(true);  // not really necessary since we have have a prestep constantly checking this
    } 
    else  // not fully deployed (belly landing!)
    {
        const double touchdownDeltaX = 4.509;
        const double touchdownZRear = -17.754;

        SetXRTouchdownPoints(_V(               0, -1.248, 21.416),                // front
                             _V(-touchdownDeltaX, -3.666, touchdownZRear),        // left
                             _V( touchdownDeltaX, -3.150, touchdownZRear),        // right (tilt the ship)
							 3.0, 3.0, false);					// belly landing!
        SetNosewheelSteering(false);  // not really necessary since we have have a prestep constantly checking this
    }

    // update the animation state
    gear_proc = state;
    SetXRAnimation(anim_gear, gear_proc);

     // redraw the gear indicator
    TriggerRedrawArea(AID_GEARINDICATOR);

    // PERFORMANCE ENHANCEMENT: hide the gear if it is fully retracted; otherwise, render it
    static const UINT gearMeshGroups[] = 
    { 
        GRP_nose_oleo_piston, GRP_nose_axle_piston, GRP_nose_axle_cylinder, GRP_nose_axle, GRP_nose_oleo_piston, GRP_nose_gear_wheel_right, GRP_nose_gear_wheel_left,
        GRP_axle_left, GRP_axle_right, GRP_gear_main_oleo_cylinder_right, GRP_axle_piston_left, GRP_axle_cylinder_left, GRP_axle_cylinder_right, GRP_axle_piston_right, GRP_oleo_piston_right,
        GRP_oleo_piston_left, GRP_wheel_left_front_left_side, GRP_wheel_right_front_left_side, GRP_wheel_left_rear_left_side, GRP_wheel_right_rear_left_side, GRP_wheel_left_rear_right_side,
        GRP_wheel_right_rear_right_side, GRP_wheel_left_front_right_side, GRP_wheel_right_front_right_side, GRP_gear_main_oleo_cylinder_left, GRP_nose_oleo_cylinder 
    };

    SetMeshGroupsVisibility((state != 0.0), exmesh, SizeOfGrp(gearMeshGroups), gearMeshGroups);
}

// handle instant jumps to open or closed here
#define CHECK_DOOR_JUMP(proc, anim) if (action == DoorStatus::DOOR_OPEN) proc = 1.0;            \
                                    else if (action == DoorStatus::DOOR_CLOSED) proc = 0.0;     \
                                    SetXRAnimation(anim, proc)

// activate the bay doors (must override base class because of our radiator check)
void XR5Vanguard::ActivateBayDoors(DoorStatus action)
{
    // cannot deploy or retract bay doors if the radiator is in motion
    // NOTE: allow for DOOR_FAILED here so that a radiator failure does not lock the bay doors
    if ((radiator_status == DoorStatus::DOOR_OPENING) || (radiator_status == DoorStatus::DOOR_CLOSING))
    {
        PlayErrorBeep();
        ShowWarning("Warning Radiator in Motion Bay Doors Are Locked.wav", DeltaGliderXR1::ST_WarningCallout, "Cannot open/close bay doors while&radiator is in motion.");
        return;  // cannot move
    }

    // OK to move doors as far as the radiator is concerned; invoke the base class
    DeltaGliderXR1::ActivateBayDoors(action);
}

// activate the crew elevator
void XR5Vanguard::ActivateElevator(DoorStatus action)
{
    // check for failure
    if (crewElevator_status == DoorStatus::DOOR_FAILED)
    {
        PlayErrorBeep();
        ShowWarning("Warning Elevator Failure.wav", DeltaGliderXR1::ST_WarningCallout, "Elevator inoperative due to excessive&heat and/or dynamic pressure.");
        return;  // cannot move
    }

    if (CheckHydraulicPressure(true, true) == false)   // show warning if no hydraulic pressure
        return;     // no hydraulic pressure

    // verify the gear has not collapsed!
    if (GetAltitude(ALTMODE_GROUND) < (GEAR_FULLY_COMPRESSED_DISTANCE-0.2))  // leave a 0.2-meter safety cushion
    {
        PlayErrorBeep();
        ShowWarning("Warning Elevator Failure.wav", DeltaGliderXR1::ST_WarningCallout, "Elevator inoperative: ground impact.");
        return;  // cannot move
    }

    bool close = (action == DoorStatus::DOOR_CLOSING) || (action == DoorStatus::DOOR_CLOSED);
    crewElevator_status = action;

    CHECK_DOOR_JUMP(crewElevator_proc, anim_crewElevator);

    TriggerRedrawArea(AID_ELEVATORSWITCH);
    TriggerRedrawArea(AID_ELEVATORINDICATOR);
    UpdateCtrlDialog(this);
    RecordEvent("ELEVATOR", close ? "CLOSE" : "OPEN");
}

// invoked from key handler
void XR5Vanguard::ToggleElevator()
{
    ActivateElevator(crewElevator_status == DoorStatus::DOOR_CLOSED || crewElevator_status == DoorStatus::DOOR_CLOSING ?
        DoorStatus::DOOR_OPENING : DoorStatus::DOOR_CLOSING);
}

// Override the base class method so we can perform some additional checks
void XR5Vanguard::ActivateRadiator(DoorStatus action)
{
    // check for failure
    if (radiator_status == DoorStatus::DOOR_FAILED)
    {
        PlayErrorBeep();
        ShowWarning("Warning Radiator Failure.wav", DeltaGliderXR1::ST_WarningCallout, "Radiator inoperative due to excessive&heat and/or dynamic pressure.");
        return;  // cannot move
    }

    if (CheckHydraulicPressure(true, true) == false)   // show warning if no hydraulic pressure
        return;     // no hydraulic pressure

    // cannot deploy or retract radiator if the payload bay doors are in motion
    // NOTE: allow for DoorStatus::DOOR_FAILED here so that a bay door failure does not lock the radiator
    if ((bay_status == DoorStatus::DOOR_OPENING) || (bay_status == DoorStatus::DOOR_CLOSING))
    {
        PlayErrorBeep();
        ShowWarning("Warning Bay Doors in Motion Radiator is Locked.wav", DeltaGliderXR1::ST_WarningCallout, "Cannot deploy/retract radiator&while bay doors are in motion.");
        return;  // cannot move
    }

    // cannot deploy or retract radiator if bay doors are OPEN (they would collide)
    if (bay_status == DoorStatus::DOOR_OPEN)
    {
        PlayErrorBeep();
        ShowWarning("Warning Bay Doors Open Radiator is Locked.wav", DeltaGliderXR1::ST_WarningCallout, "Cannot deploy/retract radiator&while bay doors are open.");
        return;  // cannot move
    }

    bool close = (action == DoorStatus::DOOR_CLOSED || action == DoorStatus::DOOR_CLOSING);
    radiator_status = action;

    CHECK_DOOR_JUMP(radiator_proc, anim_radiator);

    TriggerRedrawArea(AID_RADIATORSWITCH);
    TriggerRedrawArea(AID_RADIATORINDICATOR);

    UpdateCtrlDialog(this);
    RecordEvent ("RADIATOR", close ? "CLOSE" : "OPEN");
}

// prevent landing gear from being raised if the gear is not yet fully uncompressed
void XR5Vanguard::ActivateLandingGear(DoorStatus action)
{
    if (((action == DoorStatus::DOOR_OPENING) || (action == DoorStatus::DOOR_CLOSING)) && ((m_noseGearProc != 1.0) || (m_rearGearProc != 1.0)))
    {
        PlayErrorBeep();
        ShowWarning("Gear Locked.wav", DeltaGliderXR1::ST_WarningCallout, "Gear is still in contact with the&ground: cannot raise landing gear.");
        return;
    }

    // propogate to the superclass
    DeltaGliderXR1::ActivateLandingGear(action);
}

// Used for internal development testing only to tweak some internal value.
// This is invoked from the key handler as ALT-1 or ALT-2 are held down.  
// direction = true: increment value, false: decrement value
void XR5Vanguard::TweakInternalValue(bool direction)
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
    const double stepSize = (oapiGetSimStep() * 0.05);  // 20 seconds to go from 0...1
    m_tweakedInternalValue += (direction ? stepSize : -stepSize);
    sprintf(oapiDebugString(), "tweakedInternalValue=%lf", m_tweakedInternalValue);
#endif
#endif
}

// Render hatch decompression exhaust stream
void XR5Vanguard::ShowHatchDecompression()
{
    // NOTE: I assume this structure is actually treated as CONST by the Orbiter core, since the animation 
    // structures not declared CONST either.
    static PARTICLESTREAMSPEC airvent = {
        0, 1.0, 15, 0.5, 0.3, 2, 0.3, 1.0, PARTICLESTREAMSPEC::EMISSIVE,
        PARTICLESTREAMSPEC::LVL_LIN, 0.1, 0.1,
        PARTICLESTREAMSPEC::ATM_FLAT, 0.1, 0.1
    };
    /* Postions are:
         NOSE
          
         1  2

         3  4
    */
    static const VECTOR3 pos[4] = 
    {
        {-1.824, 6.285, 18.504},   // left-front
        { 1.824, 6.285, 18.504},   // right-front
        {-2.158, 7.838, 5.292},    // left-rear
        { 2.158, 7.838, 5.292}     // right-rear
    };

    static const VECTOR3 dir[4] = 
    {
        {-0.802,  0.597, 0},
        { 0.802,  0.597, 0},  
        {-0.050,  0.988, 0},
        { 0.050,  0.988, 0}
    };

    hatch_vent = new PSTREAM_HANDLE[4];   // this will be freed automatically for us later
    hatch_venting_lvl = new double[4];    // ditto
    for (int i=0; i < 4; i++)
    {
        hatch_venting_lvl[i] = 0.4;
        hatch_vent[i] = AddParticleStream(&airvent, pos[i], dir[i], hatch_venting_lvl + i);
    }

    hatch_vent_t = GetAbsoluteSimTime();
}

// turn off hatch decompression exhaust stream; invoked form a PostStep
void XR5Vanguard::CleanUpHatchDecompression()
{
    for (int i=0; i < 4; i++)
        DelExhaustStream(hatch_vent[i]);
}

void XR5Vanguard::DefineMmuAirlock()
{
    switch (m_activeEVAPort)
    {
        case ACTIVE_EVA_PORT::DOCKING_PORT:
        {
            const float airlockY = static_cast<float>(DOCKING_PORT_COORD.y);
            const float airlockZ = static_cast<float>(DOCKING_PORT_COORD.z);

#ifdef MMU
            //                      state,MinX, MaxX, MinY,             MaxY,             MinZ,             MaxZ
            UMmu.DefineAirLockShape(1, -0.66f, 0.66f, airlockY - 3.00f, airlockY + 0.20f, airlockZ - 0.66f, airlockZ + 0.66f);  
            VECTOR3 pos = { 0.0, airlockY + 2.0, airlockZ }; // this is the position where the Mmu will appear relative to the ship's local coordinates
            VECTOR3 rot = { 0.0, 0.0, 0.0 };  // straight up, facing forward
            UMmu.SetMembersPosRotOnEVA(pos, rot);
            UMmu.SetEjectPosRotRelSpeed(pos, rot, _V(0, 4.0, 0));  // jumped UP to bail out @ 4 meters-per-second
            UMmu.SetActiveDockForTransfer(0);       // ship-to-ship transfer enabled
#endif
            m_pActiveAirlockDoorStatus = &olock_status;
            break;
        }

        case ACTIVE_EVA_PORT::CREW_ELEVATOR:
        {
            // PRE-1.3 RC2: Port location (deployed): -3.116, -9.092, 6.35 
            // NEW: Port location (deployed):         -3.116 + 0.7, -9.092 - 0.7, 6.35 
            // add X 0.6 and Y 0.7 here for post-1.3 RC2 coordinates
            const float airlockX = -3.116f - 0.6f;
            const float airlockY = -7.299f + 0.7f;   // position coordinates refer to the TOP of the astronaut, so we have to allow space along the Y axis

            const float airlockZ =  6.35f; 
            const float xDim = 4.692f / 2;   // width from center
            const float yDim = 2.772f / 2;   // height from center
            const float zDim = 3.711f / 2;   // depth from center

#ifdef MMU
            //                      state,MinX,          MaxX,            MinY,           MaxY,            MinZ,            MaxZ
            UMmu.DefineAirLockShape(1, airlockX - xDim, airlockX + xDim, airlockY - yDim, airlockY + yDim, airlockZ - zDim, airlockZ + zDim);  
            VECTOR3 pos = { airlockX, airlockY, airlockZ + zDim + 1.0}; // this is the position where the Mmu will appear relative to the ship's local coordinates
            VECTOR3 rot = { 0.0, 0.0, 0.0 };  // straight up, facing forward
            UMmu.SetMembersPosRotOnEVA(pos, rot);
            UMmu.SetEjectPosRotRelSpeed(pos, rot, _V(0, -2.0, 0));  // jumped DOWN to bail out @ 2 meters-per-second
            UMmu.SetActiveDockForTransfer(-1);       // ship-to-ship transfer disabled
#endif
            m_pActiveAirlockDoorStatus = &crewElevator_status;
            break;
        }

        default:
            // should never happen!
            _ASSERTE(false);
            break;
    }

#ifdef MMU
    // UMmu bug: must set this every time we reset the docking port AFTER the port is defined!
    UMmu.SetMaxSeatAvailableInShip(MAX_PASSENGERS); // includes the pilot
    UMmu.SetCrewWeightUpdateShipWeightAutomatically(FALSE);  // we handle crew member weight ourselves
#endif
    // repaint both LEDs and the switch
    TriggerRedrawArea(AID_EVA_DOCKING_PORT_ACTIVE_LED); 
    TriggerRedrawArea(AID_EVA_CREW_ELEVATOR_ACTIVE_LED); 
    TriggerRedrawArea(AID_ACTIVE_EVA_PORT_SWITCH); 
}

// returns: true if EVA doors are OK, false if not
bool XR5Vanguard::CheckEVADoor()
{
    if (m_activeEVAPort == ACTIVE_EVA_PORT::DOCKING_PORT)
        return DeltaGliderXR1::CheckEVADoor();

    // else it's the crew elevator
    // NOTE: if the gear has collapsed, cannot EVA via the elevator!  Also note that we cannot use GetGearFullyCompressedAltitude here, since that will be 0
    // even after gear collapse since GroundContact will be true.
    if ((crewElevator_status == DoorStatus::DOOR_FAILED) || (GetAltitude(ALTMODE_GROUND) < (GEAR_FULLY_COMPRESSED_DISTANCE-0.2)))  // leave a 0.2-meter safety cushion
    {
        PlayErrorBeep();
        ShowWarning("Warning Elevator Failure.wav", DeltaGliderXR1::ST_WarningCallout, "Crew Elevator is damanged.");
        return false;
    }
    else if (crewElevator_status != DoorStatus::DOOR_OPEN)
    {
        PlayErrorBeep();
        ShowWarning("Warning Elevator is Closed.wav", DeltaGliderXR1::ST_WarningCallout, "Crew Elevator is stowed.");
        return false;
    }

    return true;
}

// Set the camera to its default payload bay postion.
void XR5Vanguard::ResetCameraToPayloadBay()
{
    /* ORG
    const VECTOR3 pos = { 0, 7.755, 4.077 };
    const VECTOR3 dir = { 0.0, -0.297, -0.955 };  // look down to rear bottom of bay
    */

    // PRE-1.7: const VECTOR3 pos = { 0, 8.755, 4.077 };
    const VECTOR3 pos = { 0, 8.755 + 1.0, 4.077 };  // so we don't clip under the D3D9 client
    const VECTOR3 dir = { 0.0, -0.297, -0.955 };  // look down to rear bottom of bay

    SetCameraOffset(pos);
    SetXRCameraDirection(dir); 
}


// Returns max configured thrust for the specified thruster BEFORE taking atmosphere or 
// damage into account.
// index = 0-13
double XR5Vanguard::GetRCSThrustMax(const int index) const
{
    // obtain the "normal" RCS jet power from the superclass
    double rcsThrustMax = DeltaGliderXR1::GetRCSThrustMax(index);

    // if holding attitude, adjust RCS max thrust based on payload in the bay
    if (InAtm())
    {
        if ((m_customAutopilotMode == AUTOPILOT::AP_ATTITUDEHOLD) || (m_customAutopilotMode == AUTOPILOT::AP_DESCENTHOLD))
        {
            const double withPayloadMass = GetEmptyMass();        // includes payload
            const double payloadMass = GetPayloadMass();
            const double noPayloadMass = withPayloadMass - payloadMass;  // total mass without any payload
            const double multiplier = withPayloadMass / noPayloadMass;   // 1.0 = no payload, etc.
            rcsThrustMax *= multiplier;
            // DEBUG: sprintf(oapiDebugString(), "RCS multiplier=%lf", multiplier);
        }
    }

    return rcsThrustMax;
}

// --------------------------------------------------------------
// Apply custom skin to the current mesh instance
// --------------------------------------------------------------
void XR5Vanguard::ApplySkin()
{
    if (!exmesh) return;
    
    if (skin[0])    // xr5t.dds
	{
		oapiSetTexture(exmesh, 1, skin[0]);
        oapiSetTexture(exmesh, 4, skin[0]);
	}

    if (skin[1])     // xr5b.dds
	{
		oapiSetTexture(exmesh,  2, skin[1]);
        oapiSetTexture(exmesh, 17, skin[1]);
	}
}

// meshTextureID = vessel-specific constant that is translated to a texture index specific to our vessel's .msh file.  meshTextureID 
// NOTE: meshTextureID=VCPANEL_TEXTURE_NONE = -1 = "no texture" (i.e., "not applicable"); defined in Area.h.
// hMesh = OUTPUT: will be set to the mesh handle of the mesh associated with meshTextureID.
DWORD XR5Vanguard::MeshTextureIDToTextureIndex(const int meshTextureID, MESHHANDLE &hMesh)
{
    _ASSERTE(false);  // should never reach here!

    hMesh = nullptr;      
    return MAXDWORD;   // bogus
}
