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
// XR2Ravenstar_ED.cpp
// Contains Vessel Editor methods
// ==============================================================

#include "XR2Ravenstar.h"
#include "ScnEditorAPI.h"
#include "DlgCtrl.h"
#include "XR1PayloadDialog.h"

// ==============================================================
// Scenario editor interface
// ==============================================================

XR2Ravenstar *GetDG (HWND hDlg)
{
    // retrieve DG interface from scenario editor
    OBJHANDLE vessel;
    SendMessage (hDlg, WM_SCNEDITOR, SE_GETVESSEL, reinterpret_cast<LPARAM>(&vessel));
    return static_cast<XR2Ravenstar *>(oapiGetVesselInterface (vessel));
}

// --------------------------------------------------------------
// Message procedure for editor page 1 (animation settings)
// --------------------------------------------------------------
INT_PTR CALLBACK EdPg1Proc (HWND hTab, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL retVal = FALSE;

    switch (uMsg) 
    {
    case WM_COMMAND:
        XR2Ravenstar *pXR = static_cast<XR2Ravenstar *>(GetDG(hTab));
        // save the original APU state so we can restore it later
        DoorStatus orgAPUState = pXR->apu_status;

        // hotwire the apu to ON so we can move the doors by "cheating" here
        pXR->apu_status = DOOR_OPEN;

        switch (LOWORD (wParam)) 
        {
        /* NO ONLINE HEP
        case IDHELP:

            g_hc.topic = "/SE_Anim.htm";
            oapiOpenHelp (&g_hc);
            return TRUE;
         */
        
        case IDC_GEAR_UP:
            pXR->ActivateLandingGear (DOOR_CLOSED);
            retVal = TRUE;
            break;
        case IDC_GEAR_DOWN:
            pXR->ActivateLandingGear (DOOR_OPEN);
            retVal = TRUE;
            break;

        case IDC_AIRBRAKE_STOWED:
            pXR->ActivateAirbrake(DOOR_CLOSED);
            retVal = TRUE;
            break;
        case IDC_AIRBRAKE_DEPLOYED:
            pXR->ActivateAirbrake (DOOR_OPEN);
            retVal = TRUE;
            break;

        case IDC_OLOCK_CLOSE:
            pXR->ActivateOuterAirlock (DOOR_CLOSED);
            retVal = TRUE;
            break;
        case IDC_OLOCK_OPEN:
            pXR->ActivateOuterAirlock (DOOR_OPEN);
            retVal = TRUE;
            break;

        case IDC_ILOCK_CLOSE:
            pXR->ActivateInnerAirlock (DOOR_CLOSED);
            retVal = TRUE;
            break;
        case IDC_ILOCK_OPEN:
            pXR->ActivateInnerAirlock (DOOR_OPEN);
            retVal = TRUE;
            break;

        case IDC_NCONE_CLOSE:
            pXR->ActivateOuterAirlock (DOOR_CLOSED);  // NOTE: outer door must be closed as well, BEFORE the nosecone!
            pXR->ActivateNoseCone (DOOR_CLOSED);
            retVal = TRUE;
            break;
        case IDC_NCONE_OPEN:
            pXR->ActivateNoseCone (DOOR_OPEN);
            retVal = TRUE;
            break;

        case IDC_HATCH_CLOSE:
            pXR->ActivateHatch (DOOR_CLOSED);
            retVal = TRUE;
            break;
        case IDC_HATCH_OPEN:
            pXR->ActivateHatch (DOOR_OPEN);
            retVal = TRUE;
            break;

        case IDC_RADIATOR_RETRACT:
            pXR->ActivateRadiator (DOOR_CLOSED);
            retVal = TRUE;
            break;
        case IDC_RADIATOR_EXTEND:
            pXR->ActivateRadiator (DOOR_OPEN);
            retVal = TRUE;
            break;

        case IDC_SCRAM_CLOSED:
            pXR->ActivateScramDoors(DOOR_CLOSED);
            retVal = TRUE;
            break;
        case IDC_SCRAM_OPEN:
            pXR->ActivateScramDoors (DOOR_OPEN);
            retVal = TRUE;
            break;

        case IDC_HOVER_CLOSED:
            pXR->ActivateHoverDoors(DOOR_CLOSED);
            retVal = TRUE;
            break;
        case IDC_HOVER_OPEN:
            pXR->ActivateHoverDoors (DOOR_OPEN);
            retVal = TRUE;
            break;

        case IDC_BAY_CLOSED:
            pXR->ActivateBayDoors(DOOR_CLOSED);
            retVal = TRUE;
            break;
        case IDC_BAY_OPEN:
            pXR->ActivateBayDoors (DOOR_OPEN);
            retVal = TRUE;
            break;

        case IDC_RETRO_CLOSE:
            pXR->ActivateRCover (DOOR_CLOSED);
            retVal = TRUE;
            break;
        case IDC_RETRO_OPEN:
            pXR->ActivateRCover (DOOR_OPEN);
            retVal = TRUE;
            break;
        }

        // restore original APU state
        pXR->apu_status = orgAPUState;

        break;
    }

    return retVal;
}




// --------------------------------------------------------------
// Add vessel-specific pages into scenario editor
// --------------------------------------------------------------
DLLCLBK void secInit (HWND hEditor, OBJHANDLE vessel)
{
    XR2Ravenstar *dg = static_cast<XR2Ravenstar *>(oapiGetVesselInterface (vessel));
    
    EditorPageSpec eps1 = {"Animations", g_hDLL, IDD_EDITOR_PG1, EdPg1Proc};
    SendMessage (hEditor, WM_SCNEDITOR, SE_ADDPAGEBUTTON, reinterpret_cast<LPARAM>(&eps1));

    // payload page
    EditorFuncSpec efs = { "Payload", XR1PayloadDialog::EditorFunc};
    SendMessage (hEditor, WM_SCNEDITOR, SE_ADDFUNCBUTTON, reinterpret_cast<LPARAM>(&efs));
}
