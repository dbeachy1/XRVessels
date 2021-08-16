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
// Static / global / oapi wrapper functions
// ==============================================================

#include "DeltaGliderXR1.h"

// Safely fill a screen area: if width or height == 0, do not render anything
// Otherwise, oapiColourFill will (by design) render the entire area.
void DeltaGliderXR1::SafeColorFill(SURFHANDLE tgt, DWORD fillcolor, int tgtx, int tgty, int width, int height)
{
    if ((width > 0) && (height > 0))
        oapiColourFill(tgt, fillcolor, tgtx, tgty, width, height);
}

// Safely blit a screen area: if width or height == 0, do not render anything.
// Otherwise, Orbiter may throw an assertion failure in Orbiter.exe debug builds because the DirectX blit call fails
void DeltaGliderXR1::SafeBlt(SURFHANDLE tgt, SURFHANDLE src, int tgtx, int tgty, int srcx, int srcy, int width, int height, DWORD ck)
{
    if ((width > 0) && (height > 0))
        oapiBlt(tgt, src, tgtx, tgty, srcx, srcy, width, height, ck);
}

// ==============================================================
// Message callback function for control dialog box
// ==============================================================

INT_PTR CALLBACK XR1Ctrl_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DeltaGliderXR1* dg = (uMsg == WM_INITDIALOG ? reinterpret_cast<DeltaGliderXR1*>(lParam) : reinterpret_cast<DeltaGliderXR1*>(oapiGetDialogContext(hWnd)));
    // pointer to vessel instance was passed as dialog context

    switch (uMsg) {
    case WM_INITDIALOG:
        dg->UpdateCtrlDialog(dg, hWnd);
        return FALSE;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
            oapiCloseDialog(hWnd);
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
        case IDC_NCONE_CLOSE:
            dg->ActivateNoseCone(DoorStatus::DOOR_CLOSING);
            return 0;
        case IDC_NCONE_OPEN:
            dg->ActivateNoseCone(DoorStatus::DOOR_OPENING);
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
        case IDC_LADDER_RETRACT:
            dg->ActivateLadder(DoorStatus::DOOR_CLOSING);
            return 0;
        case IDC_LADDER_EXTEND:
            dg->ActivateLadder(DoorStatus::DOOR_OPENING);
            return 0;
        case IDC_HATCH_CLOSE:
            dg->ActivateHatch(DoorStatus::DOOR_CLOSING);
            return 0;
        case IDC_HATCH_OPEN:
            dg->ActivateHatch(DoorStatus::DOOR_OPENING);
            return 0;
        case IDC_RADIATOR_RETRACT:
            dg->ActivateRadiator(DoorStatus::DOOR_CLOSING);
            return 0;
        case IDC_RADIATOR_EXTEND:
            dg->ActivateRadiator(DoorStatus::DOOR_OPENING);
            return 0;
        case IDC_NAVLIGHT:
            dg->SetNavlight(SendDlgItemMessage(hWnd, IDC_NAVLIGHT, BM_GETCHECK, 0, 0) == BST_CHECKED);
            return 0;
        case IDC_BEACONLIGHT:
            dg->SetBeacon(SendDlgItemMessage(hWnd, IDC_BEACONLIGHT, BM_GETCHECK, 0, 0) == BST_CHECKED);
            return 0;
        case IDC_STROBELIGHT:
            dg->SetStrobe(SendDlgItemMessage(hWnd, IDC_STROBELIGHT, BM_GETCHECK, 0, 0) == BST_CHECKED);
            return 0;
            //case IDC_DAMAGE:
            //	oapiOpenDialog (g_hDLL, IDD_DAMAGE, Damage_DlgProc, dg);

        }
        break;
    }
    return oapiDefDialogProc(hWnd, uMsg, wParam, lParam);
}


// ==============================================================
// Airfoil coefficient functions
// Return lift, moment and zero-lift drag coefficients as a
// function of angle of attack (alpha or beta)
//
// Note: these must be static so name collisions will not occur in subclasses.
// ==============================================================

///#define PROFILE_DRAG 0.015
// NEW to fix "floaty" landings
// XR1 1.4 ORG: #define PROFILE_DRAG 0.030
#define PROFILE_DRAG 0.015

// 1. vertical lift component (wings and body)
void DeltaGliderXR1::VLiftCoeff(VESSEL* v, double aoa, double M, double Re, void* context, double* cl, double* cm, double* cd)
{
    const int nabsc = 9;
    static const double AOA[nabsc] = { -180 * RAD, -60 * RAD,-30 * RAD, -1 * RAD, 15 * RAD,20 * RAD,25 * RAD,50 * RAD,180 * RAD };
    
    static const double CL[nabsc] =  { 0, 0, -0.15, 0, 0.7, 0.5, 0.2, 0, 0 };  // decrease negative lift to better hold negative pitch
    
    static const double CM[nabsc] =  { 0, 0.006, 0.014, 0.0034,-0.0054,-0.024,-0.00001, 0, 0 };

    int i = 0;
    for (i = 0; i < nabsc - 1 && AOA[i + 1] < aoa; i++);
    double f = (aoa - AOA[i]) / (AOA[i + 1] - AOA[i]);
    *cl = CL[i] + (CL[i + 1] - CL[i]) * f;  // aoa-dependent lift coefficient
    *cm = CM[i] + (CM[i + 1] - CM[i]) * f;  // aoa-dependent moment coefficient
    double saoa = sin(aoa);
    double pd = PROFILE_DRAG + 0.4 * saoa * saoa;  // profile drag
    
    // profile drag + (lift-)induced drag + transonic/supersonic wave (compressibility) drag
    *cd = pd + oapiGetInducedDrag(*cl, WING_ASPECT_RATIO, WING_EFFICIENCY_FACTOR) + oapiGetWaveDrag(M, 0.75, 1.0, 1.1, 0.04);
}

// 2. horizontal lift component (vertical stabilisers and body)
void DeltaGliderXR1::HLiftCoeff(VESSEL* v, double beta, double M, double Re, void* context, double* cl, double* cm, double* cd)
{
    const int nabsc = 8;
    static const double BETA[nabsc] = { -180 * RAD,-135 * RAD,-90 * RAD,-45 * RAD,45 * RAD, 90 * RAD, 135 * RAD, 180 * RAD };

    static const double CL[nabsc] = { 0, +0.3, 0, -0.3, +0.3, 0, -0.3, 0 };

    int i = 0;
    for (i = 0; i < nabsc - 1 && BETA[i + 1] < beta; i++);
    *cl = CL[i] + (CL[i + 1] - CL[i]) * (beta - BETA[i]) / (BETA[i + 1] - BETA[i]);
    *cm = 0.0;
    *cd = PROFILE_DRAG + oapiGetInducedDrag(*cl, 1.5, 0.6) + oapiGetWaveDrag(M, 0.75, 1.0, 1.1, 0.04);
}

// static data
HWND DeltaGliderXR1::s_hPayloadEditorDialog = 0;

// --------------------------------------------------------------
// Convert spaces to a character that Orbiter can save
// --------------------------------------------------------------
void DeltaGliderXR1::EncodeSpaces(char* pStr)
{
    for (char* p = pStr; *p; p++)
    {
        if (*p == ' ')
            *p = '$';
    }
}

// --------------------------------------------------------------
// Decode a string saved in the scenario file
// --------------------------------------------------------------
void DeltaGliderXR1::DecodeSpaces(char* pStr)
{
    for (char* p = pStr; *p; p++)
    {
        if (*p == '$')
            *p = ' ';
    }
}

// Format a double with commas to a given number of decimal places
// e.g., "10,292.7"
void DeltaGliderXR1::FormatDouble(const double val, CString& out, const int decimalPlaces)
{
    CString format;
    _ASSERTE(decimalPlaces >= 0);
    format.Format("%%.%dlf", decimalPlaces);  // e.g., "%.2lf"

    out.Format(format, val);   // "10292.7"

    // now add in the commas; do not use a comma at three places for under 10,000
    int lowThreshold = ((val < 10000) ? 1 : 0);  //
    for (int index = out.Find('.') - 3; index > lowThreshold; index -= 3)
        out.Insert(index, ',');
}

// static worker method that returns a pointer to a static exhaust spec
// pos and/or dir and/or tex may be null
// Note: contrary to the documentation note for AddExhaust(EXHAUSTSPEC) below, the thrusters *do* react to a change in thrust direction,
// at least in the D3D9 client.
//   >> "Exhaust positions and directions are fixed in this version, so they will not react to changes caused by SetThrusterRef and SetThrusterDir."
EXHAUSTSPEC* DeltaGliderXR1::GetExhaustSpec(const THRUSTER_HANDLE th, const double lscale, const double wscale, const VECTOR3* pos, const VECTOR3* dir, const SURFHANDLE tex)
{
    static EXHAUSTSPEC es;
    es.th = th;
    es.level = nullptr;  // core manages the level
    // work around Orbiter core bug here: EXHAUSTSPEC should specify lpos and ldir as const object pointers (they obviously are never changed by the core)
    es.lpos = const_cast<VECTOR3*>(pos);    // may be null
    es.ldir = const_cast<VECTOR3*>(dir);    // may be null
    es.lsize = lscale;
    es.wsize = wscale;
    es.lofs = 0;
    es.modulate = 0.20;  // modulates in brightness by this fraction
    es.tex = tex;        // may be null
    es.flags = EXHAUST_CONSTANTPOS | EXHAUST_CONSTANTDIR;
    es.id = 0;           // reserved, so let's by tidy for our part

    return &es;
}

// XR gateway method for AddExhaust
unsigned int DeltaGliderXR1::AddXRExhaust(const THRUSTER_HANDLE th, const double lscale, const double wscale, const SURFHANDLE tex)
{
    return AddExhaust(GetExhaustSpec(th, lscale, wscale, nullptr, nullptr, tex));
}

// Overloaded XR gateway method for AddExhaust
unsigned int DeltaGliderXR1::AddXRExhaust(const THRUSTER_HANDLE th, const double lscale, const double wscale, const VECTOR3& pos, const VECTOR3& dir, const SURFHANDLE tex)
{
    // Note: although not documented in the AddExhaust(EXHAUSTSPEC *) method, the exhaust direction must be *opposite*
    // what it is in the other AddExhaust versions, so we must flip it here.
    const VECTOR3 flippedDir = -dir;
    return AddExhaust(GetExhaustSpec(th, lscale, wscale, &pos, &flippedDir, tex));
}
