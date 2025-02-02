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

//-------------------------------------------------------------------------
// XRVCMainDialog.cpp : Implementation of main XRVCMainDialog class methods.
//-------------------------------------------------------------------------

#include <windows.h>
#include <process.h>

#include "XRVCMainDialog.h"

// define storage for static class data
XRVCMainDialog *XRVCMainDialog::s_pSingleton;
void *XRVCMainDialog::s_pCommandBoxOldMessageProc;
bool XRVCMainDialog::s_enableFullScreenMode;

const int XRVCMainDialog::MODE_GROUP_LEFT_IDCs[]  = { IDC_CHECK_MAIN, IDC_CHECK_RETRO, IDC_CHECK_HOVER, IDC_CHECK_SCRAM };
const int XRVCMainDialog::MODE_GROUP_RIGHT_IDCs[] = { IDC_CHECK_STATUS, IDC_CHECK_DOORS, IDC_CHECK_AUTOPILOTS, IDC_CHECK_OTHER };

//==============================================================
// Static method to open the main dialog window
// pContext = XRVCMainDialog * 
//==============================================================
void XRVCMainDialog::OpenDialogClbk(void *pContext)
{
    XRVCMainDialog *pDlg = static_cast<XRVCMainDialog *>(pContext);

    // Note: don't use a standard Windows function like CreateWindow to
	// open the dialog box because that wouldn't work in fullscreen mode.
    pDlg->m_hwndDlg = oapiOpenDialog(pDlg->m_hDLL, IDD_MAINDIALOG, MsgProcMain, pDlg);
}

// Constructor
XRVCMainDialog::XRVCMainDialog(const HINSTANCE hDLL) :
    m_hwndDlg(0), m_hDLL(hDLL), m_hwndHelpDlg(0), m_pScriptThread(nullptr)
{
    // construct our fixed-width courier font for our output edit boxes
    m_hCourierFontSmall  = CreateFont(-10, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, 0, FIXED_PITCH | FF_MODERN, "Courier New");
    m_hCourierFontNormal = CreateFont(-12, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, 0, FIXED_PITCH | FF_MODERN, "Courier New");

    m_pxrvcClientCommandParser = new XRVCClientCommandParser(m_xrvcClient);
}

// Destructor
XRVCMainDialog::~XRVCMainDialog()
{
    delete m_pxrvcClientCommandParser;

    // free our fixed-width fonts
    if (m_hCourierFontSmall)
        DeleteObject(m_hCourierFontSmall);

    if (m_hCourierFontNormal)
        DeleteObject(m_hCourierFontNormal);
}

//==============================================================
// Static windows message handler for our help dialog box
//==============================================================

INT_PTR CALLBACK XRVCMainDialog::MsgProcHelp(const HWND hDlg, const UINT uMsg, const WPARAM wParam, const LPARAM lParam)
{
    switch (uMsg) 
    {
        case WM_INITDIALOG:     // first-time initialization
        {
            // populate the help text
            static const char *pHelpText = 
                "XRVesselCtrl Demo Command Help:\r\n"
                "\r\n"
                "Left/Right/Home/End = Move cursor\r\n"
                "CTRL-left/right     = Jump to previous/next word\r\n"
                "Up/Down Arrow       = Recall previous/next command\r\n"
                "Tab/SHIFT-Tab       = Autocomplete command token\r\n"
                "Enter               = Execute command\r\n"
                "CTRL-Tab            = Delete last word\r\n"
                "Esc                 = Clear command line\r\n"
                "\r\n"
                "F1/'Help' button will toggle this window open/closed.\r\n"
                "The 'Available Params' box shows valid command tokens as you type.\r\n"
                "Commands are case-insensitive.\r\n"
                "Example: \"Set Door HoverDoors Opening\"\r\n";
                ;
            
            // let's use a fixed-width font for this.
            const HWND hwndText = GetDlgItem(hDlg, IDC_STATIC_HELP_TEXT);
            SendMessage(hwndText, WM_SETFONT, (WPARAM)s_pSingleton->m_hCourierFontNormal, FALSE);
            SetWindowText(hwndText, pHelpText);

            // position our help window relative to the main XRVC demo dialog
            const HWND hwndParent = s_pSingleton->m_hwndDlg;
            WINDOWINFO wi;
            wi.cbSize = sizeof(WINDOWINFO);
            if (GetWindowInfo(hwndParent, &wi))  // should always succeed
            {
                int newX = wi.rcWindow.left;         // left-align with our parent window
                int newY = wi.rcWindow.bottom + 30;  // 30 pixels below our parent window  
                
                // obtain our window size so we can maintain it when we move it
                GetWindowInfo(hDlg, &wi);
                const int width = wi.rcWindow.right - wi.rcWindow.left;
                const int height = wi.rcWindow.bottom - wi.rcWindow.top;
                MoveWindow(hDlg, newX, newY, width, height, TRUE);
            }
            return TRUE;
        }

        case WM_COMMAND:
        {
            const int controlID = LOWORD (wParam);          // dialog item ID of button, drop-down, etc.
            switch (controlID)
            {
                case IDHELP_OK:
                    goto close_window;
            }
            break;
        }

        case WM_CLOSE:
        case WM_DESTROY:
close_window:
            // notify our parent that we are closing
            s_pSingleton->clbkHelpWindowClosed();
            oapiCloseDialog(hDlg);
            return TRUE;
    }
    return oapiDefDialogProc(hDlg, uMsg, wParam, lParam);
}

//==============================================================
// Static windows message handler for our main dialog box
//==============================================================
INT_PTR CALLBACK XRVCMainDialog::MsgProcMain(const HWND hDlg, const UINT uMsg, const WPARAM wParam, const LPARAM lParam)
{
	switch (uMsg) 
    {
        case WM_INITDIALOG:     // first-time initialization
        {
            // let's save our dialog object and window handle
            s_pSingleton = reinterpret_cast<XRVCMainDialog *>(lParam);  // this is our context, passed here from: oapiRegisterCustomCmd -> OpenDialogClbk -> MsgProcMain
            s_pSingleton->m_hwndDlg = hDlg;
            
            // create our thread to handle a file dialog so that we don't block Orbiter's main thread
            s_pSingleton->m_pScriptThread = new XRVCScriptThread(hDlg);

            // initialize the dialog
	        s_pSingleton->RefreshVesselList();       // populate the vessel list
            s_pSingleton->RefreshDataSection();      // show XRVesselCtrl data
            s_pSingleton->UpdateFromStaticFields();  // sync with state loaded from the scenario

            // create a timer for 1/20th-second to refresh the data displays automatically and execute script commands
            SetTimer(hDlg, TIMERID_20_TICKS_A_SECOND, 50, nullptr);

            // create a timer for 1/10th-second to refresh the available parameters box automatically
            SetTimer(hDlg, TIMERID_UDPATE_AVAILABLE_PARAMS, 100, nullptr);

            // hook into our command window edit line so we can trap keystrokes, saving the address of its existing message loop
            s_pCommandBoxOldMessageProc = reinterpret_cast<void *>(SetWindowLongPtr(GetDlgItem(hDlg, IDC_COMMANDBOX), GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(CommandBoxMsgProc)));
            return TRUE;
        }

        case WM_DESTROY:
            s_pSingleton->CloseHelpWindow();
            delete s_pSingleton->m_pScriptThread;    // destructor signals the thread to terminate gracefully and block until it does so
	        return TRUE;

        case WM_TIMER:
            switch (wParam)
            {
                case TIMERID_20_TICKS_A_SECOND:     
                    s_pSingleton->RefreshDataSection();  // refresh on-screen ship status data
                    s_pSingleton->HandleExecuteScript();
                    return 0;
                
                case TIMERID_UDPATE_AVAILABLE_PARAMS:
                    s_pSingleton->UpdateAvailableParams();
                    s_pSingleton->EnableDisableButtons();
                    return 0;
                // no default case
            }
            break;

        // Note: looking for WM_KEYDOWN messages doesn't work here in dialogs under Orbiter, 
        // so we have to create our own message loop for our edit control instead (elsewhere).
        
        case WM_COMMAND:
        {
            const int controlID = LOWORD(wParam);          // dialog item ID of button, drop-down, etc.
            const WORD notificationMsg = HIWORD(wParam);    // e.g., BN_CLICKED

            // check BN_CLICKED messages for left-hand and right-hand panel mode buttons
            if (notificationMsg == BN_CLICKED)   
            {
                // check for left-hand panel mode buttons
                for (int i=0; i < MODE_GROUP_LEFT_COUNT; i++)
                {
                    if (controlID == MODE_GROUP_LEFT_IDCs[i]) 
                    {
                        s_pSingleton->ProcessModeSwitchLeft(controlID);
                        return TRUE;
                    }
                }

                // check for right-hand panel mode buttons
                for (int i=0; i < MODE_GROUP_RIGHT_COUNT; i++)
                {
                    if (controlID == MODE_GROUP_RIGHT_IDCs[i])
                    {
                        s_pSingleton->ProcessModeSwitchRight(controlID);
                        return TRUE;
                    }
                }
            }

            // handle the rest of the controls
            switch (controlID) 
            {
                case IDC_COMBO_VESSEL:      // vessel drop-down (combo-box) selection changed
                    if (notificationMsg == CBN_SELENDOK)
                    {
                        // item has changed
                        s_pSingleton->ComboVesselChanged();   // re-parse vessel and update our XRVCClient object
                        s_pSingleton->RefreshDataSection();   // show XRVesselCtrl data, if any
                        return TRUE;
                    }
                    break;  // some other message

                case IDC_CHECK_HIDE_NON_XRVESSELS:  // just fall through and refresh the drop-down
                case IDC_BUTTON_REFRESH_LIST:       // "Refresh List" button
                    s_pSingleton->RefreshVesselList();      // repopulate the vessel list and select the focus vessel in the drop-down
                    s_pSingleton->RefreshDataSection();     // show XRVesselCtrl data
                    return TRUE;

                case IDC_BUTTON_SET_FOCUS:
                    s_pSingleton->SetFocusToSelectedVessel();
                    return TRUE;

                case IDC_EXECUTE_COMMAND:
                        s_pSingleton->ExecuteCommand();
                    return TRUE;

                case IDC_EXECUTE_SCRIPT:   
                        s_pSingleton->ExecuteScriptFile();  
                    return TRUE;

                case IDHELP:
                    s_pSingleton->ToggleHelp();
                    return TRUE;

                case IDC_FULL_SCREEN_MODE:
                    s_pSingleton->ToggleFullScreenMode();
                    return TRUE;

                case IDCANCEL:  // dialog closed by user
                    s_pSingleton->Close();
                    return TRUE;
            }
            break;
        }
        
        case WM_SETFOCUS:
            SetFocus(GetDlgItem(s_pSingleton->m_hwndDlg, IDC_COMMANDBOX));  // we want the command box to have focus by default
            return TRUE;      
    }
	return oapiDefDialogProc(hDlg, uMsg, wParam, lParam);
}

// Static utility method that constructs the combo box entry from the vessel name and class; 
// returns pointer to static buffer.  Returns nullptr if the supplied vessel should be ignored and not placed in the drop-down.
const char *XRVCMainDialog::GetComboLineForVessel(const VESSEL *pVessel)
{
    const char *pClassName = pVessel->GetClassName();
    if (pClassName == nullptr)   // in theory should never happen, but just in case...
        pClassName = "";        

    if (_stricmp(pClassName, "XRPAYLOADBAY") == 0)
        return nullptr;        // ignore XR payload bay phantom vessels

    static char comboLine[256];  // must be static so calling function can read it
    sprintf_s(comboLine, "%s [%s]", pVessel->GetName(), pClassName);
    return comboLine;
}

//========================================================
// member methods
//========================================================

// buttonIDC = IDC of new button pressed
void XRVCMainDialog::ProcessModeSwitchLeft(const int buttonIDC)
{
    // uncheck all the other left-hand mode buttons and then check this new one
    UncheckAllModeButtons(TextPanel::TEXTPANEL_LEFT);
    SendMessage(GetDlgItem(m_hwndDlg, buttonIDC), BM_SETCHECK, BST_CHECKED, 0);

    if (m_xrvcClient.GetXRVessel() != nullptr)   // repaint only if valid XR vessel selected
    {
        // clear the left panel since we've switched modes now
        SetWindowTextSmart(GetDlgItem(m_hwndDlg, IDC_MAINBOX_LEFT), "");
    }
}

// buttonIDC = IDC of new button pressed
void XRVCMainDialog::ProcessModeSwitchRight(const int buttonIDC)
{
    UncheckAllModeButtons(TextPanel::TEXTPANEL_RIGHT);
    SendMessage(GetDlgItem(m_hwndDlg, buttonIDC), BM_SETCHECK, BST_CHECKED, 0);

    if (m_xrvcClient.GetXRVessel() != nullptr)   // repaint only if valid XR vessel selected
    {
        // clear the left panel since we've switched modes now
        SetWindowTextSmart(GetDlgItem(m_hwndDlg, IDC_MAINBOX_RIGHT), "");
    }
}

// Refresh the vessel list in the drop-down and set input focus to it
void XRVCMainDialog::RefreshVesselList()
{
    const HWND hListBox = GetDlgItem(m_hwndDlg, IDC_COMBO_VESSEL);  // handle to our vessel combo box

    // first remove all vessels in the drop-down
    SendMessage(hListBox, CB_RESETCONTENT, 0, 0);

    // now add all vessels into the drop-down list
    for (UINT i=0; i < oapiGetVesselCount(); i++)
    {
        const OBJHANDLE hVessel = oapiGetVesselByIndex(i);        // will never be null
        const VESSEL *pVessel = oapiGetVesselInterface(hVessel);  // will never be null

        // filter out non-XRVesselCtrl vessels if requested
        const LRESULT buttonState = SendMessage(GetDlgItem(m_hwndDlg, IDC_CHECK_HIDE_NON_XRVESSELS), BM_GETCHECK, 0, 0);
        if (buttonState == BST_CHECKED)
        {
            if (!XRVCClient::IsXRVesselCtrl(pVessel))
                continue;   // skip this vessel
        }

        // construct the combo box line for this vessel and send it to the combo box
        const char *pComboLine = GetComboLineForVessel(pVessel);
        if (pComboLine == nullptr)    
            continue;       // ignore this vessel

        SendMessage(hListBox, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(pComboLine));
    }

    // select the focus vessel in the drop-down and set the dialog input focus to the drop-down control
    SelectFocusVessel();
    SetFocus(hListBox);

    // parse the selected vessel and update our XRVCClient object
    ComboVesselChanged();      

    // select default left and right modes if necessary
    EnsureLeftRightModesSet();
}

// Ensure that a mode is set for both the left-hand and right-hand panels
void XRVCMainDialog::EnsureLeftRightModesSet()
{
    // see if any of the left panel buttons are set
    bool leftPanelEmpty = true;  // assume no mode set
    for (int i=0; i < MODE_GROUP_LEFT_COUNT; i++)
    {
        const HWND hButton = GetDlgItem(m_hwndDlg, MODE_GROUP_LEFT_IDCs[i]); // "Main", "Retro", etc.
        const LRESULT buttonState = SendMessage(hButton, BM_GETCHECK, 0, 0);
        if (buttonState == BST_CHECKED)
        {
            leftPanelEmpty = false;
            break;
        }
    }
    
    // if no button pressed, default to the first one
    if (leftPanelEmpty)
        SendMessage(GetDlgItem(m_hwndDlg, MODE_GROUP_LEFT_IDCs[0]), BM_SETCHECK, BST_CHECKED, 0);   

    // now check the right panel
    bool rightPanelEmpty = true;  // assume no mode set
    for (int i=0; i < MODE_GROUP_RIGHT_COUNT; i++)
    {
        const HWND hButton = GetDlgItem(m_hwndDlg, MODE_GROUP_RIGHT_IDCs[i]); // "Status", "Doors", etc.
        const LRESULT buttonState = SendMessage(hButton, BM_GETCHECK, 0, 0);
        if (buttonState == BST_CHECKED)
        {
            rightPanelEmpty = false;
            break;
        }
    }

    // if no button pressed, default to the first one
    if (rightPanelEmpty)
        SendMessage(GetDlgItem(m_hwndDlg, MODE_GROUP_RIGHT_IDCs[0]), BM_SETCHECK, BST_CHECKED, 0);   
}

// Uncheck all mode buttons
// PanelID = TEXTPANEL_LEFT, TEXTPANEL_RIGHT, TEXTPANEL_BOTH
void XRVCMainDialog::UncheckAllModeButtons(TextPanel panelID)
{
    // left panel
    if ((panelID == TextPanel::TEXTPANEL_LEFT) || (panelID == TextPanel::TEXTPANEL_BOTH))
    {
        for (int i=0; i < MODE_GROUP_LEFT_COUNT; i++)
        {
            HWND hButton = GetDlgItem(m_hwndDlg, MODE_GROUP_LEFT_IDCs[i]); // "Main", "Retro", etc.
            SendMessage(hButton, BM_SETCHECK, BST_UNCHECKED, 0);
        }
    }
    
    // right panel
    if ((panelID == TextPanel::TEXTPANEL_RIGHT) || (panelID == TextPanel::TEXTPANEL_BOTH))
    {
        for (int i=0; i < MODE_GROUP_RIGHT_COUNT; i++)
        {
            HWND hButton = GetDlgItem(m_hwndDlg, MODE_GROUP_RIGHT_IDCs[i]); // "Status", "Doors", etc.
            SendMessage(hButton, BM_SETCHECK, BST_UNCHECKED, 0);
        }
    }
}

// Sets the selected vessel in our drop-down to the target vessel
void XRVCMainDialog::SelectFocusVessel() const
{
    bool selectedVessel = false;
    OBJHANDLE hFocusObject = oapiGetFocusObject();
    if (oapiIsVessel(hFocusObject))
    {
        const VESSEL *pVessel = oapiGetVesselInterface(hFocusObject);  // will never be null
        const char *pComboLine = GetComboLineForVessel(pVessel);
        if (pComboLine != nullptr) 
        {
            LRESULT result = SendMessage(GetDlgItem(m_hwndDlg, IDC_COMBO_VESSEL), CB_SELECTSTRING, -1, reinterpret_cast<LPARAM>(pComboLine));
            selectedVessel = (result != CB_ERR);  // it is possible that the focus vessel is no longer in the list (e.g., if "Hide Non-XRVesselCtrl Vessels" wsa just checked) 
        }
    }

    if (!selectedVessel)  
        SendMessage(GetDlgItem(m_hwndDlg, IDC_COMBO_VESSEL), CB_SETCURSEL, 0, 0);  // default to first item in the list
}

// Switches the vessel focus in Orbiter to the vessel selected in the drop-down
void XRVCMainDialog::SetFocusToSelectedVessel() const
{
    const char *pVesselName = GetSelectedVesselName();
    if (pVesselName != nullptr)  // should always succeed
    {
        // Note: must cast away constness here to work around oapiGetVesselByName not taking const char * 
        const OBJHANDLE hVessel = oapiGetVesselByName(const_cast<char *>(pVesselName));  // should never be null unless the vessel was deleted
        if (hVessel != nullptr)
        {
            // vessel is still valid, so let's set focus to it
            oapiSetFocusObject(hVessel);
        }
    }
}

// Returns the name of the vessel selected in the drop-down, or nullptr if no vessel selected.
// this returns a pointer into a static buffer.
const char *XRVCMainDialog::GetSelectedVesselName() const
{
    char vesselLine[256];
    
    // retrieve the vessel's name and class from the vessel drop-down; format is "vesselName [classname]"
    int charsRead = static_cast<int>(SendMessage(GetDlgItem(m_hwndDlg, IDC_COMBO_VESSEL), WM_GETTEXT, sizeof(vesselLine), reinterpret_cast<LPARAM>(vesselLine))); 
    if (charsRead == 0)  // should never happen since a vessel should always be selected
        return nullptr;

    // parse the vessel entry from the drop-down and extract the name, which is everything up to the leading "["
    CString csVesselLine(vesselLine);
    const int bracketIndex = csVesselLine.Find('[');  // will always succeed
    CString csVesselName(csVesselLine.Left(bracketIndex).Trim());  // trim the leading and trailing whitespace as well

    // Note: this buffer must be static because we return a pointer to it to the caller.
    // This is safe to do here because 1) we only ever have one of these dialogs open, and 2) we are not multi-threaded.
    static char buff[128];
    strcpy_s(buff, static_cast<const char *>(csVesselName));
    return buff;
}

// Invoked whenever the vessel selected in the drop-down box changes
void XRVCMainDialog::ComboVesselChanged()
{
    char xrVesselCtrlVersionStr[20];
    strcpy_s(xrVesselCtrlVersionStr, "NONE"); // assume not XRVesselCtrl

    const char *pVesselName = GetSelectedVesselName();
    // retrieve the vessel's name and class from the vessel drop-down; format is "vesselName [classname]"
    if (pVesselName == nullptr)
    {
        m_xrvcClient.SetXRVessel(nullptr);
no_vessel:
        // update the XRVesselCtrl version box
        SetWindowTextSmart(GetDlgItem(m_hwndDlg, IDC_XRVC_VERSION), xrVesselCtrlVersionStr);

        // let's clear both main text boxes
        SetWindowTextSmart(GetDlgItem(m_hwndDlg, IDC_MAINBOX_LEFT), "");
        SetWindowTextSmart(GetDlgItem(m_hwndDlg, IDC_MAINBOX_RIGHT), "");
        return;
    }

    // retrieve the vessel object from Orbiter
    // Note: must cast away constness here to work around oapiGetVesselByName not taking const char *
    const OBJHANDLE hVessel = oapiGetVesselByName(const_cast<char *>(pVesselName));  // should never be null
    if (hVessel == nullptr)
        goto no_vessel;   // should never happen!

    VESSEL *pOrbiterVessel = oapiGetVesselInterface(hVessel);  // will never be null
    if (XRVesselCtrl::IsXRVesselCtrl(pOrbiterVessel))
    {
        // this vessel implements XRVesselCtrl, so it is safe to downcast to XRVesselCtrl
        XRVesselCtrl *pVessel = static_cast<XRVesselCtrl *>(pOrbiterVessel);
        const float xrVesselCtrlVersion = pVessel->GetCtrlAPIVersion();
        sprintf_s(xrVesselCtrlVersionStr, "%.1f", xrVesselCtrlVersion);  

        if (xrVesselCtrlVersion < THIS_XRVESSELCTRL_API_VERSION)
        {
            // old version, so let's clear both main text boxes
            SetWindowTextSmart(GetDlgItem(m_hwndDlg, IDC_MAINBOX_LEFT), "");
            SetWindowTextSmart(GetDlgItem(m_hwndDlg, IDC_MAINBOX_RIGHT), "");
            goto exit;
        }

        // this vessel implements XRVesselCtrl and the version is OK: show the XR state data for the selected modes
        m_xrvcClient.SetXRVessel(pVessel);
    }
    else   // this is a non-XR vessel, so reset the font and text in our edit boxes
    {
        m_xrvcClient.SetXRVessel(nullptr);
        static const char *pNonXRText = "Vessel does not implement the XRVesselCtrl interface.";
        SendMessage(GetDlgItem(m_hwndDlg, IDC_MAINBOX_LEFT), WM_SETFONT, (WPARAM)m_hCourierFontNormal, FALSE);
        SendMessage(GetDlgItem(m_hwndDlg, IDC_MAINBOX_RIGHT), WM_SETFONT, (WPARAM)m_hCourierFontNormal, FALSE);
    
        SetWindowTextSmart(GetDlgItem(m_hwndDlg, IDC_MAINBOX_LEFT), pNonXRText);
        SetWindowTextSmart(GetDlgItem(m_hwndDlg, IDC_MAINBOX_RIGHT), pNonXRText);
    }

exit:
    // write XRVesselCtrl version string to the dialog (may be "None")
    SetWindowTextSmart(GetDlgItem(m_hwndDlg, IDC_XRVC_VERSION), xrVesselCtrlVersionStr);
}

// Refreshes the XRVesselCtrl Data section of our main dialogng data section text, which was 
// already set by ComboVesselChanged.
void XRVCMainDialog::RefreshDataSection()
{
    XRVesselCtrl *pXRVessel = m_xrvcClient.GetXRVessel();
    if (pXRVessel == nullptr)
        return;   // nothing to update

    // this vessel implements XRVesselCtrl and the version is OK: show the XR state data for the selected modes
    XRStatusOut(IDC_MAINBOX_LEFT, GetActiveModeLeftIDC());
    XRStatusOut(IDC_MAINBOX_RIGHT, GetActiveModeRightIDC());
}

// Returns IDC_CHECK_MAIN, IDC_CHECK_RETRO, etc.
int XRVCMainDialog::GetActiveModeLeftIDC() const
{
    int retVal = -1;
    // find the button (checkbox) that is currently checked
    for (int i=0; i < MODE_GROUP_LEFT_COUNT; i++)
    {
        const int modeIDC = MODE_GROUP_LEFT_IDCs[i];
        const HWND hButton = GetDlgItem(m_hwndDlg, modeIDC); // "Main", "Retro", etc.
        const LRESULT buttonState = SendMessage(hButton, BM_GETCHECK, 0, 0);
        if (buttonState == BST_CHECKED)
        {
            retVal = modeIDC;  // found it
            break;
        }
    }
    _ASSERTE(retVal >= 0);
    return retVal;
}

// Returns IDC_CHECK_STATUS, IDC_CHECK_DOORS, etc
int XRVCMainDialog::GetActiveModeRightIDC() const
{
    int retVal = -1;
    // find the button (checkbox) that is currently checked
    for (int i=0; i < MODE_GROUP_RIGHT_COUNT; i++)
    {
        const int modeIDC = MODE_GROUP_RIGHT_IDCs[i];
        const HWND hButton = GetDlgItem(m_hwndDlg, modeIDC); // "Status", "Doors", etc.
        const LRESULT buttonState = SendMessage(hButton, BM_GETCHECK, 0, 0);
        if (buttonState == BST_CHECKED)
        {
            retVal = modeIDC;  // found it
            break;
        }
    }
    _ASSERTE(retVal >= 0);
    return retVal;
}

// Returns true if selected vessel implements XRVesselCtrl and version is OK.  Otherwise,
// displays an error message to the status box and returns false.
bool XRVCMainDialog::CheckXRVesselForCommand()
{ 
    const bool isVesselOK = (m_xrvcClient.GetXRVessel() != nullptr);
    if (!isVesselOK)
    {
        CString msg;
        msg.Format("Error: selected vessel does not implement XRVesselCtrl %.1f or newer.", THIS_XRVESSELCTRL_API_VERSION);
        SetStatusText(msg);
    }
    return isVesselOK;
}   

// Process a keystroke from our command edit box
// keycode = VK_RETURN, VK_ESCAPE, etc.
// wMsg = WM_KEYDOWN, WM_KEYUP, WM_CHAR
// Returns: true if key processed and should be ignored, false if the default Windows key handler should process it.
bool XRVCMainDialog::ProcessCommandKeystroke(WPARAM keycode, const UINT wMsg)
{
// macro to swallow all KEYUP messages so we don't get beeps on certain keypresses such as RETURN, ESCAPE, and TAB
#define IGNORE_IF_KEYUP() if (wMsg == WM_KEYUP) return true

    if ((wMsg == WM_KEYDOWN) && (keycode != VK_TAB) && (keycode != VK_SHIFT) && (keycode != VK_CONTROL))
        m_pxrvcClientCommandParser->ResetAutocompletionState();     // allow SHIFT and CTRL keys to not disrupt autocompletion cycling

    bool retVal = false;
    if (wMsg == WM_CHAR)     // character code 
    {
        switch (keycode)
        {
            case 27:  // ESC
            case 13:  // Enter
            case 9:   // Tab      
                retVal = true;     // swallow key to prevent system beep
                break;
            // Note: the other keys that we hook do not generate WM_CHAR messages
        }
    }
    else    // virtual key code
    {
        switch (keycode)
        {
            case VK_RETURN:
                IGNORE_IF_KEYUP();
                ExecuteCommand();
                SetCommandText("");  // clear the command line
                retVal = true;       // we processed the key
                break;

            case VK_UP:
            {
                IGNORE_IF_KEYUP();
                CString csNewText = m_pxrvcClientCommandParser->RetrieveCommand(false);  // get previous command text
                SetCommandText(csNewText);
                retVal = true;
                break;
            }

            case VK_DOWN:
            {
                IGNORE_IF_KEYUP();
                CString csNewText = m_pxrvcClientCommandParser->RetrieveCommand(true);  // get next command text
                SetCommandText(csNewText);
                retVal = true;
                break;
            }

            case VK_ESCAPE:
            {
                IGNORE_IF_KEYUP();
                SetCommandText("");  // clear the command line
                m_pxrvcClientCommandParser->ResetCommandRecallIndex();
                retVal = true;
                break;
            }

            case VK_TAB:
            {
                IGNORE_IF_KEYUP();

                // check for CTRL-TAB, which deletes the last token on the command line
                if (GetKeyState(VK_CONTROL) & 0x8000)
                {
                    RemoveLastTokenFromCommandLine();
                    m_pxrvcClientCommandParser->ResetAutocompletionState();
                }
                else   // this is either TAB or SHIFT-TAB
                {
                    bool direction = !(GetKeyState(VK_SHIFT) & 0x8000);  // if key is NOT DOWN, direction is *forward*
                    AutoCompleteCommand(direction);
                }
                retVal = true;
                break;
            }

            case VK_F1:
            {
                IGNORE_IF_KEYUP();
                ToggleHelp();
                retVal = true;
                break;
            }
            // no default case; ignore all other keycodes
        }
    }
    return retVal;
}

// Enable/Disable the "Execute Command" and "Execute Script File" buttons
void XRVCMainDialog::EnableDisableButtons() const
{
    CString csCommand;
    GetCommandText(csCommand);
    // Note: we want to allow the command button be enabled even if the selected vessels is not an XR vessel
    // so the user can play around with the command line to see how it works.
    EnableWindow(GetDlgItem(m_hwndDlg, IDC_EXECUTE_COMMAND), (csCommand.GetLength() > 0));

    // Only enable "Execute Script File" button if 1) the selected vessel is an XR vessel, 
    // 2) the thread is idle and waiting for work, and 3) if we are not in full-screen mode.
    const bool isExecuteScriptEnabled = ((m_xrvcClient.GetXRVessel() != nullptr) && m_pScriptThread->IsThreadIdle()  && !s_enableFullScreenMode);
    EnableWindow(GetDlgItem(m_hwndDlg, IDC_EXECUTE_SCRIPT), isExecuteScriptEnabled);
}

// update the "Available Params" options based on the text in the command box
void XRVCMainDialog::UpdateAvailableParams() const
{
    CString csCommand;
    GetCommandText(csCommand);
    
    // retrive the available arguments for current command parameters
    vector<CString> argsOut;
    const int paramLevel = m_pxrvcClientCommandParser->GetAvailableArgumentsForCommand(csCommand, argsOut);

    CString csLine;
    csLine.Format("(%d) ", paramLevel);
    for (unsigned int i=0; i < argsOut.size(); i++)
    {
        if (i > 0)
            csLine += "  ";
        csLine += argsOut[i];
    }
    
    // now update the text in the 'available params' read-only edit box *only if the text has changed*

    SetWindowTextSmart(GetDlgItem(m_hwndDlg, IDC_AVAILABLE_PARAMS), csLine);
}

// Removes the last token on the command line, but add a trailing space if the command is not empty
void XRVCMainDialog::RemoveLastTokenFromCommandLine()
{
    CString csCommand;
    GetCommandText(csCommand);

    if (csCommand.IsEmpty())
    {
        AutocompleteBeep();
        return;   // nothing to whack
    }

    // locate the last space in the command line and truncate the string there
    int lastSpaceIndex = csCommand.ReverseFind(' ');
    if (lastSpaceIndex >= 0)
        csCommand = csCommand.Left(lastSpaceIndex + 1);  // leave the trailing space 
    else  // no spaces, so just whack any text (single argument) on the line
        csCommand.Empty();
    
    SetCommandText(csCommand);
}

// Retrieves the trimmed text from the command box and stores it in csOut.
void XRVCMainDialog::GetCommandText(CString &csOut) const
{
    const int maxCmdLength = 256;
    char *pBuff = csOut.GetBufferSetLength(maxCmdLength);  // allocate space for the command text
    GetWindowText(GetDlgItem(m_hwndDlg, IDC_COMMANDBOX), pBuff, maxCmdLength);
    csOut.ReleaseBuffer();

    bool success = true;

    // trim leading and trailing whitespace; if the command is empty, there is nothing to execute
    csOut = csOut.Trim();
}

// Resets the text in the command box to the supplied value; may not be null.
void XRVCMainDialog::SetCommandText(const char *pNewText) const
{
    _ASSERTE(pNewText != nullptr);
    const HWND hCommandBox = GetDlgItem(m_hwndDlg, IDC_COMMANDBOX);
    SetWindowTextSmart(hCommandBox, pNewText);

    // reset the cursor to the end of the text in the box
    const size_t textLength = strlen(pNewText);
    SendMessage(hCommandBox, EM_SETSEL, textLength, textLength);
}

// Executes the command in the command box.  Command status will be written to the status box.
// Returns true if command executed successfully, false if error occurred.
bool XRVCMainDialog::ExecuteCommand()
{
    // retrieve the trimmed text from the command box
    CString csCommand;
    GetCommandText(csCommand);

#ifdef _DEBUG
    if (csCommand.CompareNoCase("dumptree") == 0)
        return DumpCommandTree("c:\\temp\\xrvctree.txt");
#endif

    return ExecuteCommand(csCommand);
}


// Autocomplete and execute the supplied command.  Command status will be written to the status box.
// Returns true if command executed successfully, false if error occurred.
bool XRVCMainDialog::ExecuteCommand(CString &csCommand)
{
    bool success = true;   // assume empty command

    // NOTE: we do not want to invoke CheckXRVesselForCommand here yet because we want to 
    // commands to always be added to the command history regardless of whether an XR vessel is selected.

    // ignore empty command
    if (!csCommand.IsEmpty())
    {
        // auto-complete/clean up extra whitespace in the command
        m_pxrvcClientCommandParser->AutoCompleteCommand(csCommand, true);   // direction is moot here since we only call this once

        // execute the command
        CString csStatus;
        success = m_pxrvcClientCommandParser->ExecuteCommand(csCommand, csStatus);
        if (!success)
            ErrorBeep();

        // override the normal error message if this is not an XR vessel
        if (CheckXRVesselForCommand())
            SetStatusText(csStatus);   // vessel OK, so update the status window text with the result
    }
    // else fall through and return true for the empty command

    // clear the command box and focus set to it
    SetCommandText("");
    SetFocus(GetDlgItem(m_hwndDlg, IDC_COMMANDBOX));
    
    return success;
}

// Pops up or closes a non-blocking help box showing command shortcuts
void XRVCMainDialog::ToggleHelp()
{
    // see if it's already open
    if (m_hwndHelpDlg != 0)
    {
        // already open, so close it
        oapiCloseDialog(m_hwndHelpDlg);
        m_hwndHelpDlg = 0;
    }
    else
    {
        // Note: don't use a standard Windows function like CreateWindow to
	    // open the dialog box because that wouldn't work in fullscreen mode.
        m_hwndHelpDlg = oapiOpenDialog(m_hDLL, IDD_HELP, MsgProcHelp, nullptr);
    }
}

// Attempt to auto-complete the command in the command box; if successful, the completed command will be written to the status box.
// direction: true = tab direction forward, false = tab direction backward
// Returns true if all tokens were autocompleted, false otherwise
bool XRVCMainDialog::AutoCompleteCommand(const bool direction)
{
    // retrieve the trimmed text from the command box
    CString csCommand;
    GetCommandText(csCommand);
    
    // attempt to auto-complete the command
    bool autoCompletedAllTokens = m_pxrvcClientCommandParser->AutoCompleteCommand(csCommand, direction);
    // update the command window text with the autocompleted/cleaned-up result
    SetCommandText(csCommand);

    if (!autoCompletedAllTokens)
    {
        // could not autocomplete all tokens
        AutocompleteBeep();
    }

    // focus set to the command box (should be moot for the moment, but let's be tidy)
    SetFocus(GetDlgItem(m_hwndDlg, IDC_COMMANDBOX));
    
    return autoCompletedAllTokens;
}

//=========================================================================
// Static message proc for our command edit window; this is necessary in 
// order to capture key events in a dialog under Orbiter.
//=========================================================================
LRESULT CALLBACK XRVCMainDialog::CommandBoxMsgProc(const HWND hWnd, const UINT uMsg, const WPARAM wParam, const LPARAM lParam)
{
    switch (uMsg)
    {
        // Inspect keystrokes; we want to hook KEYDOWN, KEYUP, and CHAR here so we 
        // can get cursor keys properly as well as prevent the default system beeps when 
        // ENTER, TAB, and ESC are pressed in our edit box.
        case WM_KEYDOWN:
        case WM_KEYUP:  
        case WM_CHAR:
        {
            const bool processed = s_pSingleton->ProcessCommandKeystroke(wParam, uMsg);  // wParam = VK_RETURN, etc.
            if (processed)
                return TRUE;    
            break;
        }
        // check for lost focus as well
        case WM_KILLFOCUS:
            s_pSingleton->m_pxrvcClientCommandParser->ResetAutocompletionState();
            break;

    }
    // send this as-yet-unprocessd message on to the window's old message proc
    return CallWindowProc((WNDPROC)s_pCommandBoxOldMessageProc, hWnd, uMsg, wParam, lParam);
}

// return a font handle appropriate for the specified mode
HFONT XRVCMainDialog::GetFontForMode(const int modeIDC) const
{
    HFONT hRetVal = 0;  // not set yet

    switch (modeIDC)
    {
        // for ENGINE and STATUS modes, use the small font
        case IDC_CHECK_MAIN:
        case IDC_CHECK_RETRO:
        case IDC_CHECK_HOVER:
        case IDC_CHECK_SCRAM:
        case IDC_CHECK_STATUS:
            hRetVal = m_hCourierFontSmall;
            break;

        // use normal font for these modes
        case IDC_CHECK_DOORS:
        case IDC_CHECK_AUTOPILOTS:
        case IDC_CHECK_OTHER:
            hRetVal = m_hCourierFontNormal;
            break;

        default:  // should never happen!
            // break into the debugger for debug builds
            _ASSERTE(false);  // incorrect mode IDC!
    }

    return hRetVal;
}

// Send formatted text for the active mode to the specified edit box
// editBoxOutIDC = IDC of edit box to which formatted text will be sent
// modeIDC = IDC of active mode button (IDC_CHECK_MAIN, IDC_CHECK_RETRO, etc.)
// hFont = font with which to render the output
void XRVCMainDialog::XRStatusOut(const int editBoxOutIDC, const int modeIDC)
{
    CString csOut;  // holds text output to be sent to the edit box

    // set the font in output edit box to the correct width for this mode
    const HFONT hFont = GetFontForMode(modeIDC);
    SendMessage(GetDlgItem(m_hwndDlg, editBoxOutIDC), WM_SETFONT, (WPARAM)hFont, FALSE);

    switch (modeIDC)
    {
        case IDC_CHECK_MAIN:
            m_xrvcClient.RetrieveEngineState(csOut, XREngineID::XRE_MainLeft, XREngineID::XRE_MainRight, "Port Main Engine", "Starboard Main Engine");
            break;

        case IDC_CHECK_RETRO:
            m_xrvcClient.RetrieveEngineState(csOut, XREngineID::XRE_RetroLeft, XREngineID::XRE_RetroRight, "Port Retro Engine", "Starboard Retro Engine");
            break;
        
        case IDC_CHECK_HOVER:
            m_xrvcClient.RetrieveEngineState(csOut, XREngineID::XRE_HoverFore, XREngineID::XRE_HoverAft, "Forward Hover Engine", "Aft Hover Engine");
            break;
        
        case IDC_CHECK_SCRAM:
            m_xrvcClient.RetrieveEngineState(csOut, XREngineID::XRE_ScramLeft, XREngineID::XRE_ScramRight, "Port SCRAM Engine", "Starboard SCRAM Engine");
            break;
        
        case IDC_CHECK_STATUS:
            m_xrvcClient.RetrieveStatus(csOut);
            break;
        
        case IDC_CHECK_DOORS:
            m_xrvcClient.RetrieveDoorsState(csOut);
            break;
        
        case IDC_CHECK_AUTOPILOTS:
            m_xrvcClient.RetrieveAutopilotsState(csOut);
            break;

        case IDC_CHECK_OTHER:
            m_xrvcClient.RetrieveOther(csOut);
            break;

        default:    // should never happen!
            _ASSERTE(false);  // break into debugger under debug builds
            csOut.Format("INTERNAL ERROR: INVALID modeIDC: %d", modeIDC);
            break;
    }

    // Send the formatted text to the edit control
    SetWindowTextSmart(GetDlgItem(m_hwndDlg, editBoxOutIDC), (LPCTSTR)csOut);
}

// Our static smart SetWindowText that only updates the window's text if the contents have changed;
// This is to prevent flickering in the text field from it being erased and re-drawn unnecessarily.
// Returns true on success, false on error
bool XRVCMainDialog::SetWindowTextSmart(HWND hWnd, const char *pString) const
{
    // Send the formatted text to the edit control *only if the text has changed*.  
    // This is necessary in order to prevent text flickering because the box 
    // is erased right before the text inside replaced.

    // first we retrieve the existing window text
    CString csExistingText;
    const int buffLen = 2048;
    char *pBuff = csExistingText.GetBufferSetLength(buffLen);
    GetWindowText(hWnd, pBuff, buffLen);  
    csExistingText.ReleaseBuffer();
    
    // now see the current text is different from what we've just constructed
    bool retVal = true;  // no change == success
    if (strcmp(csExistingText, pString) != 0) 
        retVal = (::SetWindowText(hWnd, pString) == TRUE);
    
    return retVal;
}

// Initialize the dialog with data read from our scenario file
void XRVCMainDialog::UpdateFromStaticFields()
{ 
    SendMessage(GetDlgItem(m_hwndDlg, IDC_FULL_SCREEN_MODE), BM_SETCHECK, (s_enableFullScreenMode ? BST_CHECKED : BST_UNCHECKED), 0); 
}

//=========================================================================================
// Interfaces with our ScriptThread and executes a group of script commands if ready.
// Returns true if script executed, or false if no work was available.
//=========================================================================================
bool XRVCMainDialog::HandleExecuteScript()
{
    // check for a status message from the ExecuteScript thread
    CString statusMsg;
    if (m_pScriptThread->GetStatusMessage(statusMsg))
        SetStatusText(statusMsg);    // update the dialog

    // check for a list of script commands from the ExecuteScript thread    
    bool scriptExecuted = false;
    vector<CString> latchedCommandList;
    m_pScriptThread->GetScriptCommands(latchedCommandList);   // latch command list from thread
    
    if (latchedCommandList.size() == 0)
        return false;     // no script work to do

    //
    // User wants to run a list of script commands.
    //

    if (!CheckXRVesselForCommand())
        return false;       // not an XR vessel

    // execute all latched script commands
    for (unsigned int i=0; i < latchedCommandList.size(); i++)
    {
        CString &csLatchedCommand = latchedCommandList[i];
        if (csLatchedCommand.GetLength() == 0)
            continue;       // line is empty

        scriptExecuted = true;

        // Write the script command to the command line and feed then invoke our ExecuteCommand so that
        // the command will be auto-completed, etc., just as if the user had typed it.
        SetCommandText(csLatchedCommand);
        const bool commandStatus = ExecuteCommand(csLatchedCommand);   // this will store the command in the user's history as well
        if (!commandStatus)
        {
            CString msg;
            msg.Format("Script Error - command failed: [%s]", csLatchedCommand);
            SetStatusText(msg);
            break;    // command failed, so halt script execution
        }
    }
    return scriptExecuted;
}

// This quick-and-dirty method is only used for debugging to dump the tree to a file.
bool XRVCMainDialog::DumpCommandTree(const char *pFilename)
{
    CString csCommands;
    BuildCommandHelpTree(csCommands);

    FILE *pFile = nullptr;
    if (fopen_s(&pFile, pFilename, "wb") != 0)
        return false;

    if (fputs(csCommands, pFile) == EOF)
        return false;

    fclose(pFile);
    
    CString msg;
    msg.Format("Dumped command list to '%s'", pFilename);
    SetStatusText(msg);

    return true;
}
