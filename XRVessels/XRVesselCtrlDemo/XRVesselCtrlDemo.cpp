//-------------------------------------------------------------------------
// XRVesselCtrlDemo.cpp : Orbiter module that pops up a dialog to 
// remotely control/interface with vessels that implement XRVesselCtrl 2.0 or newer.
// This module was created as a way to properly test the XRVesselCtrl API.
//
// You can open this in Orbiter by pressing CTRL-F4 and selecting the "XRVesselCtrlDemo"
// entry in the list.
//
// Copyright 2010-2018 Douglas E. Beachy; All Rights Reserved.
//
// This software is FREEWARE and may not be sold!
//
// NOTE: You may not redistribute this file nor use it in any other project without
// express consent from the author.  
//
// http://www.alteaaerospace.com
// mailto:doug.beachy@outlook.com
//-------------------------------------------------------------------------

#include <windows.h>

#define ORBITER_MODULE
#include "orbitersdk.h"

#include "XRVCMainDialog.h"

//==============================================================
// Global variables
//==============================================================

DWORD g_dwCmd;                  // custom function identifier

//==============================================================
// This function is called when Orbiter starts or when the module
// is activated.
//==============================================================
DLLCLBK void opcDLLInit(HINSTANCE hDLL)
{
    // create our singleton MainDialog object
    XRVCMainDialog *pDlg = new XRVCMainDialog(hDLL); 
    
    // This will create an entry in the "Custom Functions" list which is accessed
	// in Orbiter via Ctrl-F4.
    g_dwCmd = oapiRegisterCustomCmd(VERSION, "Demonstrates XRVesselCtrl remote interfacing to XR vessels.", XRVCMainDialog::OpenDialogClbk, reinterpret_cast<void *>(pDlg));

    char logMsg[256];
    sprintf_s(logMsg, "%s initialized.", VERSION);
    oapiWriteLog(logMsg);
}

//==============================================================
// This function is called when Orbiter shuts down or when the
// module is deactivated.
//==============================================================
DLLCLBK void opcDLLExit (HINSTANCE hDLL)
{
    char logMsg[256];

    // Unregister the custom function in Orbiter
	oapiUnregisterCustomCmd (g_dwCmd);

    // free our main dialog object
    delete XRVCMainDialog::s_pSingleton;

    sprintf_s(logMsg, "%s exiting.", VERSION);
    oapiWriteLog(logMsg);
}


// ==============================================================
// Write our parameters to the scenario file
// ==============================================================
DLLCLBK void opcSaveState(FILEHANDLE scn)
{
	oapiWriteScenario_int (scn, "EnableFullScreenMode", XRVCMainDialog::s_enableFullScreenMode);
}

// ==============================================================
// Read our parameters from the scenario file
// ==============================================================
#define IF_FOUND(name)  if (!_strnicmp(line, name, (len = static_cast<int>(strlen(name))))) 
#define SSCANF1(formatStr, a1) sscanf_s(line + len, formatStr, a1)
#define SSCANF_BOOL(var) { int temp = 0; SSCANF1("%d", &temp); var = (temp != 0); }

DLLCLBK void opcLoadState(FILEHANDLE scn)
{
    char *line; // used by macros
    int len;    // used by macros

	while (oapiReadScenario_nextline (scn, line)) 
    {
		IF_FOUND("EnableFullScreenMode")
        {
			SSCANF_BOOL(XRVCMainDialog::s_enableFullScreenMode);
		}
	}
}
