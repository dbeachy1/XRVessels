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

//-------------------------------------------------------------------------
// XRVesselCtrlDemo.cpp : Orbiter module that pops up a dialog to 
// remotely control/interface with vessels that implement XRVesselCtrl 2.0 or newer.
// This module was created as a way to properly test the XRVesselCtrl API.
//
// You can open this in Orbiter by pressing CTRL-F4 and selecting the "XRVesselCtrlDemo"
// entry in the list.
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
