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

// ==============================================================
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// XR1PayloadDialog.h
// Implements our common payload dialog handler class; this is NOT USED
// by the XR1: it for subclasses to use.
// ==============================================================

#pragma once

#include "DeltaGliderXR1.h"
#include "XR1PayloadDialog.h"
#include "resource_common.h"  // for our dialog controls that must match in each subclass that uses this dialog
#include "ScnEditorAPI.h"
#include "DlgCtrl.h"
#include "XRPayload.h"
#include "XRPayloadBaySlot.h"

// static font handles
HFONT XR1PayloadDialog::s_hOrgFont;      // normal button font handle
HFONT XR1PayloadDialog::s_hBoldFont;     // bold button font handle

// Invoked at initialization time when the user clicks our "payload" button; instantiates a 
// new XR1PayloadDialog instance and dispatches messages.  Returns when dialog is closed.
void XR1PayloadDialog::EditorFunc(OBJHANDLE hVessel)
{
    DeltaGliderXR1 *pXR1 = static_cast<DeltaGliderXR1 *>(oapiGetVesselInterface(hVessel));
    Launch(hVessel);
}

// method invoked by the Vanguard to open a new instance
// Returns: handle to new dialog
HWND XR1PayloadDialog::Launch(OBJHANDLE hVessel)
{
    DeltaGliderXR1 *pXR1 = static_cast<DeltaGliderXR1 *>(oapiGetVesselInterface(hVessel));
    HWND hDlg = oapiOpenDialogEx(g_hDLL, GLOBAL_IDD_PAYLOAD_EDITOR, Proc, DLG_CAPTIONCLOSE, pXR1);

    return hDlg;
}

#define TIMERID_REFRESH_MASS 1
#define TIMERID_REFRESH_BAY  2

// message proc that handles all our Windows messages
// Returns: TRUE if message handled, FALSE if message not handled; i.e., the next window in the chain should handle it
INT_PTR CALLBACK XR1PayloadDialog::Proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // handle bay slot buttons here via a for loop since there are an unknown number of them
    if (uMsg == WM_COMMAND)
    {
        const WORD loWord = LOWORD (wParam);    // get resource ID of button that was clicked
        for (int i=0; i < slotCount; i++)
        {
            if (loWord == slotResourceIDs[i])
            {
                const WORD notificationMsg = HIWORD(wParam);    // e.g., BN_CLICKED
                bool wasProcessed = ProcessSlotButtonMsg(hDlg, i+1, (HWND)lParam, notificationMsg);  // slots are one-based
                if (wasProcessed)
                    return TRUE;
                // else fall through to oapiDefDialogProc
            }
        }
    }

    switch (uMsg) 
    {
    /* Note: for some reason Orbiter appears to be trapping keystrokes, so this will not work.
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
            goto closeDialog;   // bye, bye
        break;  // pass it on
    */

	case WM_INITDIALOG:
        {
            // pointer to vessel instance was passed as dialog context
            DeltaGliderXR1 &xr1 = *reinterpret_cast<DeltaGliderXR1 *>(lParam);

            // Then walk through list of all vessels in the Orbiter config directory and 
            // add each XRPayload object to our payload combo box.
            HWND hListBox = ::GetDlgItem(hDlg, IDC_COMBO_SELECTED_PAYLOAD_OBJECT);
            const XRPayloadClassData **ppAllPayloadClasses = XRPayloadClassData::GetAllAvailableXRPayloads();  // this is a static global array that should not be freed by us
            for (const XRPayloadClassData **ppClassData = ppAllPayloadClasses; *ppClassData != nullptr; ppClassData++)
            {
                const char *pClassname = (*ppClassData)->GetClassname();
                ::SendMessage(hListBox, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(pClassname));
            }

            // now select the first one in the list
            // NOTE: it is possible that the list is empty, although that would only happen if the user deliberately deleted the sample payload container(s) 
            // installed with the Vanguard.
            ::SendMessage(hListBox, CB_SETCURSEL, 0, 0);    // index 0

            // set initial focus to our combo-box
            SetFocus(hListBox);

            // scan the payload bay and update the slot button states and the ship's mass fields
            RescanBayAndUpdateButtonStates(hDlg, &xr1);  // must pass in the XR1 ptr here
            ProcessSelectedPayloadChanged(hDlg, &xr1);   // ditto

            // create a timer for 1/20th-second so we can refresh the ship and payload mass values automatically
            SetTimer(hDlg, TIMERID_REFRESH_MASS, 50, NULL);

            // Create a time for 1/5th-second so we can refresh the bay contents automatically in case 
            // the user deploys or adds cargo via the ship's controls.
            SetTimer(hDlg, TIMERID_REFRESH_BAY, 200, NULL);

            return FALSE;  // we already set the focus 
        }  // case WM_INITDIALOG

    case WM_TIMER:
        switch (wParam)
        {
        case TIMERID_REFRESH_MASS:     // refresh the mass readout values
            UpdateMassValues(hDlg, GetXR1(hDlg));
            return 0;

        case TIMERID_REFRESH_BAY:     // refresh the bay contents
            RescanBayAndUpdateButtonStates(hDlg, &GetXR1(hDlg));
            return 0;
            
        // no default case
        }
        break;

	case WM_COMMAND:
		switch (LOWORD (wParam)) 
        {
        case IDC_COMBO_SELECTED_PAYLOAD_OBJECT:
            {
                if (HIWORD(wParam) == CBN_SELENDOK)   
                {
                    // value in combo box changed: read the selected classname and populate the payload values for it
                    ProcessSelectedPayloadChanged(hDlg);
                }
                return TRUE;
            }

        case IDC_BACK:
		case IDCANCEL:
            // WARNING: do not invoke GetXR1() here!  Orbiter has cleared the context pointer at this point!
            CloseDialog(hDlg);
			return TRUE;

		case IDHELP:
			// TODO: add help
			return FALSE;

        case IDC_EMPTY_BAY:
            // remove all payload in the bay
            GetXR1(hDlg).m_pPayloadBay->DeleteAllAttachedPayloadVessels();
            RescanBayAndUpdateButtonStates(hDlg);
            GetXR1(hDlg).PlaySound(GetXR1(hDlg).SwitchOff, DeltaGliderXR1::ST_Other, MED_CLICK);  // medium click 
            return TRUE;

        case IDC_FILL_PAYLOAD_BAY:
            {
                // Fill all open slots in the bay with the currently-selected payload;
                // this will walk through each slot in order and try to add a module to each.

                // retrieve selected vessel classname
                char pSelectedClassname[256];
                if (GetSelectedPayloadClassname(hDlg, pSelectedClassname, sizeof(pSelectedClassname)) > 0)  // anything selected in the box?
                {
                    DeltaGliderXR1 &xr1 = GetXR1(hDlg);
                    xr1.m_pPayloadBay->CreateAndAttachPayloadVesselInAllSlots(pSelectedClassname);
                    RescanBayAndUpdateButtonStates(hDlg);   // update the button enabled/disabled pushed/unpushed states based on the new payload in the bay
                }
                GetXR1(hDlg).PlaySound(GetXR1(hDlg).SwitchOn, DeltaGliderXR1::ST_Other, MED_CLICK);  // medium click 
                return TRUE;
            }

        case IDC_SELECTED_REMOVE_ALL:
            {
                // Remove all items of the currently-selected payload.

                // retrieve selected vessel classname
                char pSelectedClassname[256];
                if (GetSelectedPayloadClassname(hDlg, pSelectedClassname, sizeof(pSelectedClassname)) > 0)  // anything selected in the box?
                {
                    DeltaGliderXR1 &xr1 = GetXR1(hDlg);
                    xr1.m_pPayloadBay->DeleteAllAttachedPayloadVesselsOfClassname(pSelectedClassname);
                    RescanBayAndUpdateButtonStates(hDlg);   // update the button enabled/disabled pushed/unpushed states based on the new payload in the bay
                }
                GetXR1(hDlg).PlaySound(GetXR1(hDlg).SwitchOff, DeltaGliderXR1::ST_Other, MED_CLICK);  // medium click 
                return TRUE;
            }

		}  // case WM_COMMAND
		break;


    case WM_SETFOCUS:
        {
            // we just received the input focus; set focus to our selected payload combo-box
            HWND hListBox = ::GetDlgItem(hDlg, IDC_COMBO_SELECTED_PAYLOAD_OBJECT);
            SetFocus(hListBox);
            return TRUE;
        }

    case WM_TERMINATE:     // our custom message: Vanguard pilot wants us to close
        CloseDialog(hDlg);
        return TRUE;

    case WM_CLOSE:
         DeltaGliderXR1::s_hPayloadEditorDialog = 0;   // in case the sim is closing
         break;  // fall through to oapiDefDialogProc
	}
	return oapiDefDialogProc(hDlg, uMsg, wParam, lParam);
}

// close this dialog
void XR1PayloadDialog::CloseDialog(HWND hDlg)
{
    // WARNING: do not invoke GetXR1() in this method!  Orbiter may have cleared the context pointer at this point!

    // clean up dialog-specific resources
    KillTimer(hDlg, TIMERID_REFRESH_MASS);
    KillTimer(hDlg, TIMERID_REFRESH_BAY);

    if (s_hOrgFont != nullptr)
    {
        DeleteObject(s_hOrgFont);
        DeleteObject(s_hBoldFont);
        s_hOrgFont = s_hBoldFont = nullptr;  // reset for when the next dialog is instantiated
    }

    DeltaGliderXR1::s_hPayloadEditorDialog = 0;   // tell the ship we are closing

    // terminate
    oapiCloseDialog(hDlg);
}

// Update the payload fields on the dialog using the supplied vessel classname
void XR1PayloadDialog::UpdatePayloadFields(HWND hDlg, const char *pClassname)
{
    char msg[80];  // reused

    const XRPayloadClassData &pd = XRPayloadClassData::GetXRPayloadClassDataForClassname(pClassname);

    // description
    SetDlgItemText(hDlg, IDC_STATIC_DESCRIPTION, pd.GetDescription());
    
    // mass
    sprintf(msg, "%.3f", pd.GetMass());
    SetDlgItemText(hDlg, IDC_STATIC_MASS, msg);

    // dimensions
    VECTOR3 dim = pd.GetDimensions();
    sprintf(msg, "%.2f L x %.2f W x %.2f H", dim.z, dim.x, dim.y); 
    SetDlgItemText(hDlg, IDC_STATIC_DIMENSIONS, msg);

    // slots occupied
    VECTOR3 slots = pd.GetSlotsOccupied();  
    sprintf(msg, "%.1f L x %.1f W x %.1f H", slots.z, slots.x, slots.y); 
    SetDlgItemText(hDlg, IDC_STATIC_SLOTS_OCCUPIED, msg);

    // show the bitmap preview, if any (may be null)
    const HBITMAP hBmp = pd.GetThumbnailBitmapHandle();
    HWND hPictureCtrl = ::GetDlgItem(hDlg, IDC_STATIC_THUMBNAIL_BMP);
    ::SendMessage(hPictureCtrl, STM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(hBmp));   // TODO: figure out what a NULL hBmp here does to the image; is it blank?
}

// Refresh vessel and payload mass readouts
void XR1PayloadDialog::UpdateMassValues(HWND hDlg, const DeltaGliderXR1 &xr1)
{
    const double vesselMass = xr1.GetMass();
    const double payloadMass = xr1.GetPayloadMass();

    char msg[40];
    sprintf(msg, "%10.1f kg (%10.1lf lb)", payloadMass, payloadMass * 2.20462262);
    SetDlgItemText(hDlg, IDC_STATIC_PAYLOAD_MASS, msg);

    sprintf(msg, "%10.1f kg (%10.1lf lb)", vesselMass, vesselMass * 2.20462262);
    SetDlgItemText(hDlg, IDC_STATIC_VESSEL_MASS, msg);
}

// Process a payload button click message
// hDlg = main dialog window handle
// slotNumber = 1...36
// hButton = handle of button that was checked or unchecked
// notificationMsg = button window message ID; e.g., BN_CLICKED
// Returns: true if message processed, false if message not processed
bool XR1PayloadDialog::ProcessSlotButtonMsg(HWND hDlg, const int slotNumber, const HWND hButton, const WORD notificationMsg)
{
    bool retVal = false;

    // Note: disabled checkboxes (like buttons) do not send BN_CLICKED notifications
    if (notificationMsg == BN_CLICKED)
    {
        // retrieve the state of this button
        LRESULT buttonState = ::SendMessage(hButton, BM_GETCHECK, 0, 0);
        if (buttonState == BST_CHECKED)
        {
            AddPayloadToSlot(slotNumber, hDlg, hButton);
            GetXR1(hDlg).PlaySound(GetXR1(hDlg).SwitchOn, DeltaGliderXR1::ST_Other, MED_CLICK);  // medium click 
        }
        else
        {
            RemovePayloadFromSlot(slotNumber, hDlg, hButton);
            GetXR1(hDlg).PlaySound(GetXR1(hDlg).SwitchOff, DeltaGliderXR1::ST_Other, MED_CLICK);  // medium click 
        }

        retVal = true;
    }

    return retVal;
}

// Instantiate a new instance of the selected payload vessel and add it to the
// specified slot if there is room.
// Returns: true on success, false if slot is already occupied or if there is no room for the new vessel
bool XR1PayloadDialog::AddPayloadToSlot(const int slotNumber, HWND hDlg, HWND hButton)
{
    _ASSERTE(slotNumber > 0);
    bool retVal = false;

    // retrieve selected vessel classname
    char pSelectedClassname[256];
    if (GetSelectedPayloadClassname(hDlg, pSelectedClassname, sizeof(pSelectedClassname)) > 0)  // anything selected in the box?
    {
        DeltaGliderXR1 &xr1 = GetXR1(hDlg);
        retVal = xr1.m_pPayloadBay->CreateAndAttachPayloadVessel(pSelectedClassname, slotNumber);

        // update the button enabled/disabled pushed/unpushed states based on the new payload in the bay
        // We must always rescan in order to "unpush" any failed buttons.
        RescanBayAndUpdateButtonStates(hDlg);   
        if (retVal == false)     // no room for payload?
            xr1.PlaySound(DeltaGliderXR1::Error1, DeltaGliderXR1::ST_Other, ERROR1_VOL);      // error beep
    }
    
    return retVal;
}
  
// Delete the vessel in the selected slot
// Returns: true on success, false if no payload is in the selected slot.
bool XR1PayloadDialog::RemovePayloadFromSlot(const int slotNumber, HWND hDlg, HWND hButton)
{
    _ASSERTE(slotNumber > 0);
    bool retVal = false;

    DeltaGliderXR1 &xr1 = GetXR1(hDlg);
    XRPayloadBay *pBay = xr1.m_pPayloadBay;

    // delete the attached vessel
    retVal = pBay->DeleteAttachedPayloadVessel(slotNumber);
    if (retVal) // success?
    {
        // update the button enabled/disabled pushed/unpushed states based on the new payload in the bay
        RescanBayAndUpdateButtonStates(hDlg);   
    }
    
    return retVal;
}

// Rescan the payload bay and update button states based on the state of the payload bay;
// a slot that is occupied by a neighboring payload will be disabled.  Note that primary
// slots (slots to which a payload is attached) are *always* enabled, as are empty slots.
// pXR1: if NULL, look up XR1 via oapiGetDialogContext; WARNING: if WM_INITDIALOG, you must pass in pXR1 because the context is not set up yet!
// This method will also update the pushed/unpushed state based on whether this slot is a primary slot.  Note that 
// disabled slots do not need to be pushed or unpushed since it is irrelevant.
//
// This method must be invoked each time a payload is added to/removed from the bay.
void XR1PayloadDialog::RescanBayAndUpdateButtonStates(HWND hDlg, DeltaGliderXR1 *pXR1)
{
    if (pXR1 == nullptr)
        pXR1 = &GetXR1(hDlg);

    XR1PayloadBay *pBay = static_cast<XR1PayloadBay *>(pXR1->m_pPayloadBay);

    // retrieve selected vessel classname
    char pSelectedClassname[256];
    GetSelectedPayloadClassname(hDlg, pSelectedClassname, sizeof(pSelectedClassname));  // pSelectedClassname will be empty if no payload selected

    // walk through each slot and set the corresponding button's enabled/disabled pushed/unpushed state
    for (int i=0; i < pBay->GetSlotCount(); i++)
    {
        const int slotNumber = i+1;   // slot numbers are one-based
        const XRPayloadBaySlot *pSlot = pBay->GetSlot(slotNumber);  
        const bool isEnabled = pSlot->IsEnabled();    
        int buttonResourceID = slotResourceIDs[i];
        
        // Set pushed/unpushed based on whether payload is in this slot
        const HWND hButton = ::GetDlgItem(hDlg, buttonResourceID);
        ::SendMessage(hButton, BM_SETCHECK, (pSlot->IsOccupied() ? BST_CHECKED : BST_UNCHECKED), 0);

        // enable/disable the button for this slot
        ::EnableWindow(hButton, isEnabled);

        // Set the text to BOLD if this button contains content that is of the selected payload object class; 
        // otherwise, set it back to normal.
        bool isBold = false;
        VESSEL *pChildVessel = pSlot->GetChild();
        if (pChildVessel != nullptr)
        {
            if (strcmp(pSelectedClassname, pChildVessel->GetClassName()) == 0)
            {
                // classnames match; this button should be in BOLD font
                isBold = true;
            }
        }

        // Retrieve the button font.  NOTE: this font handle is tied to this dialog, so we cannot cache it statically.
        // However, there is only ever *one* instance of this dialog active at one time, so we can recreate it for each
        // dialog instance and free it when the dialog is destroyed.
        if (s_hOrgFont == nullptr)
        {
            // First time through; retrieve its original font and create the new bold font
            // These handles will persist until the dialog closes.
            s_hOrgFont = (HFONT)::SendMessage(hButton, WM_GETFONT, 0, 0);
            LOGFONT lf;
            GetObject(s_hOrgFont, sizeof(lf), &lf);   // fill in the LOGFONT value

            // Ensure that the org font is NOT bold; for some reason Windows seems to use a default BOLD 
            // font on very rare occasions.
            // do not free old font since Windows owns it
            lf.lfWeight = FW_NORMAL;
            s_hOrgFont = CreateFontIndirect(&lf);    // normal font

            // create a new bold font, set the button font, and force a redraw
            lf.lfWeight = FW_EXTRABOLD;
            s_hBoldFont = CreateFontIndirect(&lf);
        }        

        ::SendMessage(hButton, WM_SETFONT, (WPARAM)(isBold ? s_hBoldFont : s_hOrgFont), TRUE);
    }

    // update the ship's mass fields as well
    UpdateMassValues(hDlg, *pXR1);
}

// Retrieve the selected payload classname and store it in pOut.
// pOut will be empty if no payload is present.
// outLength = size of pOut buffer
// Returns # of characters read from the drop-down list
int XR1PayloadDialog::GetSelectedPayloadClassname(const HWND hDlg, char *pOut, const int outLength)
{
    // value in combo box changed: read the selected classname and populate the payload values for it
    HWND hListBox = ::GetDlgItem(hDlg, IDC_COMBO_SELECTED_PAYLOAD_OBJECT);
    LRESULT result = ::SendMessage(hListBox, WM_GETTEXT, outLength, reinterpret_cast<LPARAM>(pOut)); 

    return static_cast<int>(result);
}

// invoked whenever the selected payload type changed
// pXR1: pointer to our parent vessel; may be NULL only if not invoked from WM_INIT.
void XR1PayloadDialog::ProcessSelectedPayloadChanged(HWND hDlg, DeltaGliderXR1 *pXR1)
{
    // retrieve the new classname value from the combo box
    char pSelectedClassname[256];
    if (GetSelectedPayloadClassname(hDlg, pSelectedClassname, sizeof(pSelectedClassname)) > 0)  // anything selected in the box?
    {
        // we have the selected classname; update the payload data on the dialog
        UpdatePayloadFields(hDlg, pSelectedClassname);

        // also update button label font to show bold for slots with new selected payload
        RescanBayAndUpdateButtonStates(hDlg, pXR1);
    }
}
