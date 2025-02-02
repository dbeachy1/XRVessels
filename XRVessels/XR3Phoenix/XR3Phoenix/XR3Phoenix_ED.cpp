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
// XR3Phoenix implementation class
//
// XR3Phoenix_ED.cpp
// Contains Vessel Editor methods
// ==============================================================

#include "XR3Phoenix.h"
#include "ScnEditorAPI.h"
#include "DlgCtrl.h"
#include "XR1PayloadDialog.h"

// ==============================================================
// Scenario editor interface
// ==============================================================

XR3Phoenix *GetXR3 (HWND hDlg)
{
    // retrieve vessel interface from scenario editor
    OBJHANDLE vessel;
    SendMessage (hDlg, WM_SCNEDITOR, SE_GETVESSEL, reinterpret_cast<LPARAM>(&vessel));
    return static_cast<XR3Phoenix *>(oapiGetVesselInterface (vessel));
}

// --------------------------------------------------------------
// Message procedure for editor page 1 (animation settings)
// lParam = Orbiter vessel handle
// --------------------------------------------------------------
INT_PTR CALLBACK EdPg1Proc (HWND hTab, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    /* Note: for some reason Orbiter appears to be trapping keystrokes, so this will not work.
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
            oapiCloseDialog(hTab);   // bye, bye
        break;  // pass it on
    */

    case WM_COMMAND:
        switch (LOWORD (wParam)) {
        case IDHELP:
            // TODO: add editor help
            return FALSE;
        case IDC_GEAR_UP:
            GetXR3(hTab)->ActivateLandingGear(DoorStatus::DOOR_CLOSED);
            return TRUE;
        case IDC_GEAR_DOWN:
            GetXR3(hTab)->ActivateLandingGear(DoorStatus::DOOR_OPEN);
            return TRUE;

        case IDC_RETRO_CLOSE:
            GetXR3(hTab)->ActivateRCover(DoorStatus::DOOR_CLOSED);
            return TRUE;
        case IDC_RETRO_OPEN:
            GetXR3(hTab)->ActivateRCover(DoorStatus::DOOR_OPEN);
            return TRUE;

        case IDC_OLOCK_CLOSE:
            GetXR3(hTab)->ActivateOuterAirlock(DoorStatus::DOOR_CLOSED);
            return TRUE;
        case IDC_OLOCK_OPEN:
            GetXR3(hTab)->ActivateOuterAirlock(DoorStatus::DOOR_OPEN);
            return TRUE;

        case IDC_ILOCK_CLOSE:
            GetXR3(hTab)->ActivateInnerAirlock(DoorStatus::DOOR_CLOSED);
            return TRUE;
        case IDC_ILOCK_OPEN:
            GetXR3(hTab)->ActivateInnerAirlock(DoorStatus::DOOR_OPEN);
            return TRUE;

        case IDC_DOCKING_STOW:
            GetXR3(hTab)->ActivateOuterAirlock(DoorStatus::DOOR_CLOSED);  // NOTE: outer airlock must close, too!
            GetXR3(hTab)->ActivateNoseCone(DoorStatus::DOOR_CLOSED);
            return TRUE;
        case IDC_DOCKING_DEPLOY:
            GetXR3(hTab)->ActivateNoseCone(DoorStatus::DOOR_OPEN);
            return TRUE;

        case IDC_ELEVATOR_STOW:
            GetXR3(hTab)->ActivateElevator(DoorStatus::DOOR_CLOSED);
            return TRUE;
        case IDC_ELEVATOR_DEPLOY:
            GetXR3(hTab)->ActivateElevator(DoorStatus::DOOR_OPEN);
            return TRUE;

        case IDC_HATCH_CLOSE:
            GetXR3(hTab)->ActivateHatch(DoorStatus::DOOR_CLOSED);
            return TRUE;
        case IDC_HATCH_OPEN:
            GetXR3(hTab)->ActivateHatch(DoorStatus::DOOR_OPEN);
            return TRUE;

        case IDC_RADIATOR_STOW:
            GetXR3(hTab)->ActivateRadiator(DoorStatus::DOOR_CLOSED);
            return TRUE;
        case IDC_RADIATOR_DEPLOY:
            GetXR3(hTab)->ActivateRadiator(DoorStatus::DOOR_OPEN);
            return TRUE;

        case IDC_SCRAM_CLOSE:
            GetXR3(hTab)->ActivateScramDoors(DoorStatus::DOOR_CLOSED);
            return TRUE;
        case IDC_SCRAM_OPEN:
            GetXR3(hTab)->ActivateScramDoors(DoorStatus::DOOR_OPEN);
            return TRUE;

        case IDC_HOVER_CLOSE:
            GetXR3(hTab)->ActivateHoverDoors(DoorStatus::DOOR_CLOSED);
            return TRUE;
        case IDC_HOVER_OPEN:
            GetXR3(hTab)->ActivateHoverDoors(DoorStatus::DOOR_OPEN);
            return TRUE;

        case IDC_BAY_CLOSE:
            GetXR3(hTab)->ActivateBayDoors(DoorStatus::DOOR_CLOSED);
            return TRUE;
        case IDC_BAY_OPEN:
            GetXR3(hTab)->ActivateBayDoors(DoorStatus::DOOR_OPEN);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// --------------------------------------------------------------
// Add vessel-specific pages into scenario editor
// --------------------------------------------------------------
DLLCLBK void secInit (HWND hEditor, OBJHANDLE hVessel)
{
    // animation page
    EditorPageSpec eps1 = {"Animations", g_hDLL, IDD_EDITOR_PG1, EdPg1Proc};
    SendMessage (hEditor, WM_SCNEDITOR, SE_ADDPAGEBUTTON, reinterpret_cast<LPARAM>(&eps1));

    // payload page
    EditorFuncSpec efs = { "Payload", XR1PayloadDialog::EditorFunc};
    SendMessage (hEditor, WM_SCNEDITOR, SE_ADDFUNCBUTTON, reinterpret_cast<LPARAM>(&efs));
}
