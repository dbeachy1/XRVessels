/**
  XR Vessel add-ons for OpenOrbiter Space Flight Simulator
  Copyright (C) 2006-2021 Douglas Beachy

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.

  Email: mailto:doug.beachy@outlook.com
  Web: https://www.alteaaerospace.com
**/

#include "DeltaGliderXR1.h"
#include "ScnEditorAPI.h"
#include "DlgCtrl.h"

static HELPCONTEXT g_hc = {
    "html/vessels/deltaglider.chm",
        0,
        "html/vessels/deltaglider.chm::/deltaglider.hhc",
        "html/vessels/deltaglider.chm::/deltaglider.hhk"
};

// static helper methods
static void UpdateDamage (HWND hTab, DeltaGliderXR1 *dg)
{
    int i;
    char cbuf[256];
    i = static_cast<int>((dg->lwingstatus*100.0+0.5));
    sprintf (cbuf, "%d %%", i);
    SetWindowText (GetDlgItem (hTab, IDC_LEFTWING_STATUS), cbuf);
    oapiSetGaugePos (GetDlgItem (hTab, IDC_LEFTWING_SLIDER), i);
    i = static_cast<int>((dg->rwingstatus*100.0+0.5));
    sprintf (cbuf, "%d %%", i);
    SetWindowText (GetDlgItem (hTab, IDC_RIGHTWING_STATUS), cbuf);
    oapiSetGaugePos (GetDlgItem (hTab, IDC_RIGHTWING_SLIDER), i);
}

// ==============================================================
// Scenario editor interface
// ==============================================================

DeltaGliderXR1 *GetDG (HWND hDlg)
{
    // retrieve DG interface from scenario editor
    OBJHANDLE vessel;
    SendMessage (hDlg, WM_SCNEDITOR, SE_GETVESSEL, reinterpret_cast<LPARAM>(&vessel));
    return static_cast<DeltaGliderXR1 *>(oapiGetVesselInterface(vessel));
}

// --------------------------------------------------------------
// Message procedure for editor page 1 (animation settings)
// --------------------------------------------------------------
INT_PTR CALLBACK EdPg1Proc (HWND hTab, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_COMMAND:
        switch (LOWORD (wParam)) {
        case IDHELP:
            g_hc.topic = "/SE_Anim.htm";
            oapiOpenHelp (&g_hc);
            return TRUE;
        case IDC_GEAR_UP:
            GetDG(hTab)->ActivateLandingGear (DOOR_CLOSED);
            return TRUE;
        case IDC_GEAR_DOWN:
            GetDG(hTab)->ActivateLandingGear (DOOR_OPEN);
            return TRUE;
        case IDC_RETRO_CLOSE:
            GetDG(hTab)->ActivateRCover (DOOR_CLOSED);
            return TRUE;
        case IDC_RETRO_OPEN:
            GetDG(hTab)->ActivateRCover (DOOR_OPEN);
            return TRUE;
        case IDC_OLOCK_CLOSE:
            GetDG(hTab)->ActivateOuterAirlock (DOOR_CLOSED);
            return TRUE;
        case IDC_OLOCK_OPEN:
            GetDG(hTab)->ActivateOuterAirlock (DOOR_OPEN);
            return TRUE;
        case IDC_ILOCK_CLOSE:
            GetDG(hTab)->ActivateInnerAirlock (DOOR_CLOSED);
            return TRUE;
        case IDC_ILOCK_OPEN:
            GetDG(hTab)->ActivateInnerAirlock (DOOR_OPEN);
            return TRUE;
        case IDC_NCONE_CLOSE:
            GetDG(hTab)->ActivateOuterAirlock(DOOR_CLOSED);  // NOTE: outer airlock must close, too!
            GetDG(hTab)->ActivateNoseCone (DOOR_CLOSED);
            return TRUE;
        case IDC_NCONE_OPEN:
            GetDG(hTab)->ActivateNoseCone (DOOR_OPEN);
            return TRUE;
        case IDC_LADDER_RETRACT:
            GetDG(hTab)->ActivateLadder (DOOR_CLOSED);
            return TRUE;
        case IDC_LADDER_EXTEND:
            GetDG(hTab)->ActivateLadder (DOOR_OPEN);
            return TRUE;
        case IDC_HATCH_CLOSE:
            GetDG(hTab)->ActivateHatch (DOOR_CLOSED);
            return TRUE;
        case IDC_HATCH_OPEN:
            GetDG(hTab)->ActivateHatch (DOOR_OPEN);
            return TRUE;
        case IDC_RADIATOR_RETRACT:
            GetDG(hTab)->ActivateRadiator (DOOR_CLOSED);
            return TRUE;
        case IDC_RADIATOR_EXTEND:
            GetDG(hTab)->ActivateRadiator (DOOR_OPEN);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

#if 0  // this was not compatible with UMMU to begin with
// --------------------------------------------------------------
// Message procedure for editor page 2 (passengers)
// --------------------------------------------------------------
INT_PTR CALLBACK EdPg2Proc (HWND hTab, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DeltaGliderXR1 *dg;
    
    switch (uMsg) {
    case WM_INITDIALOG: 
        {
            char cbuf[256];
            dg = static_cast<DeltaGliderXR1 *>(oapiGetVesselInterface((OBJHANDLE)lParam));
            for (int i=0; i < MAX_PASSENGERS; i++)  
            {
                const char *pName = dg->GetCrewNameBySlotNumber(i);
                bool passengerOnBoard = dg->IsCrewMemberOnBoard(i);
                SendDlgItemMessage (hTab, IDC_CHECK1+i, BM_SETCHECK, passengerOnBoard ? BST_CHECKED : BST_UNCHECKED, 0);
            }
            sprintf (cbuf, "%0.2f kg", dg->GetMass());
            SetWindowText (GetDlgItem (hTab, IDC_MASS), cbuf);
            break;
        } 
    case WM_COMMAND:
        switch (LOWORD (wParam))
        {
        case IDC_CHECK1:
        case IDC_CHECK2:
        case IDC_CHECK3:
        case IDC_CHECK4: 
        case IDC_CHECK5: 
            {
                // WARNING: this is not compatible with UMmu crew management because UMmu reorders the slots (0-4) each time a crew member is added or removed.
                char cbuf[256];
                LRESULT lResult = SendDlgItemMessage (hTab, LOWORD(wParam), BM_GETCHECK, 0, 0);
                dg = GetDG(hTab);

                // get current crew member for this slot
                const int index = LOWORD(wParam)-IDC_CHECK1; // 0...n
                const char *pName = dg->GetCrewNameBySlotNumber(index);
                if (lResult)    // adding crew member?
                {
                    if (strlen(pName) == 0)     // not already on board?
                    {
                        // add default crew member for this slot
                        CrewMember *pCM = dg->GetXR1Config()->CrewMembers + index;
                        dg->UMmu.AddCrewMember(pCM->name, pCM->age, pCM->pulse, pCM->mass, pCM->rank);
                    }
                }
                else    // removing crew member
                {
                    if (strlen(pName) > 0)  // is crew member on board?
                    {
                        // UMmu bug: must cast away constness here
                        dg->UMmu.RemoveCrewMember(const_cast<char *>(pName));   // remove him
                    }
                }

                dg->SetPassengerVisuals();
                dg->SetEmptyMass();
                sprintf (cbuf, "%0.2f kg", dg->GetMass());
                SetWindowText (GetDlgItem (hTab, IDC_MASS), cbuf);
                break;
            } 
        }
        break;
    }
    return FALSE;
}
#endif

// --------------------------------------------------------------
// Message procedure for editor page 3 (damage)
// --------------------------------------------------------------
INT_PTR CALLBACK EdPg3Proc (HWND hTab, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DeltaGliderXR1 *dg;
    
    switch (uMsg) {
    case WM_INITDIALOG: {
        dg = static_cast<DeltaGliderXR1 *>(oapiGetVesselInterface(reinterpret_cast<OBJHANDLE>(lParam)));
        GAUGEPARAM gp = { 0, 100, GAUGEPARAM::LEFT, GAUGEPARAM::BLACK };
        oapiSetGaugeParams (GetDlgItem (hTab, IDC_LEFTWING_SLIDER), &gp);
        oapiSetGaugeParams (GetDlgItem (hTab, IDC_RIGHTWING_SLIDER), &gp);
        UpdateDamage (hTab, dg);
                        } break;
    case WM_COMMAND:
        /* NOT IMPLEMENTED
        switch (LOWORD (wParam)) {
        case IDC_REPAIR:
            dg = GetDG(hTab);
            dg->RepairDamage ();
            UpdateDamage (hTab, dg);
            return TRUE;
        }
        break;
        */
        case WM_HSCROLL:
            dg = GetDG(hTab);
            int id = GetDlgCtrlID ((HWND)lParam);
            switch (id) {
            case IDC_LEFTWING_SLIDER:
            case IDC_RIGHTWING_SLIDER:
                switch (LOWORD (wParam)) {
                case SB_THUMBTRACK:
                case SB_LINELEFT:
                case SB_LINERIGHT:
                    if (id == IDC_LEFTWING_SLIDER)
                        dg->lwingstatus = HIWORD(wParam)*0.01;
                    else
                        dg->rwingstatus = HIWORD(wParam)*0.01;
                    dg->ApplyDamage ();
                    UpdateDamage (hTab, dg);
                    return TRUE;
                }
                break;
            }
            break;
    }
    return FALSE;
}

// --------------------------------------------------------------
// Add vessel-specific pages into scenario editor
// --------------------------------------------------------------
DLLCLBK void secInit (HWND hEditor, OBJHANDLE vessel)
{
    DeltaGliderXR1 *dg = static_cast<DeltaGliderXR1 *>(oapiGetVesselInterface(vessel));
    
    EditorPageSpec eps1 = {"Animations", g_hDLL, IDD_EDITOR_PG1, EdPg1Proc};
    SendMessage (hEditor, WM_SCNEDITOR, SE_ADDPAGEBUTTON, reinterpret_cast<LPARAM>(&eps1));
    EditorPageSpec eps3 = {"Damage", g_hDLL, IDD_EDITOR_PG3, EdPg3Proc};
    SendMessage (hEditor, WM_SCNEDITOR, SE_ADDPAGEBUTTON, reinterpret_cast<LPARAM>(&eps3));
}

