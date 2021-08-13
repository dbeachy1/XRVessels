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

//-------------------------------------------------------------------------
// XRVCScriptThread.h : Class definition for our script thread.
//-------------------------------------------------------------------------

#pragma once

#include <windows.h>
#include <atlstr.h>  // we use CString instead of std::string primarily because we want the CString.Format method
#include <vector>

using namespace std;

class XRVCScriptThread
{
public:
    // public member methods
    XRVCScriptThread(const HWND hwndMainDialog);
    virtual ~XRVCScriptThread();
    
    // these methods interface with the thread
    bool OpenScriptFile();    // prompt the user for a filename and send the script data
    bool OpenScriptFile(const char *pFilename);  // open the requested file and send it 
    bool GetStatusMessage(CString &csOut);
    int GetScriptCommands(vector<CString> &commandListOut);
    bool IsThreadIdle() const { return (WaitForSingleObject(m_hEventScriptFile, 0) == WAIT_TIMEOUT); }

    // public data

protected:
    // static thread methods
    static unsigned __stdcall ScriptThread  (void *pParameter);
    static void ST_SendStatusMessage(XRVCScriptThread &instance, const char *pMsg);
    static bool ST_SendCommands(XRVCScriptThread &instance, const vector<CString> &commandList);
    static int  ST_ParseScriptFile(FILE *pFile, vector<CString> &commandListOut);
    
    // data
    HWND m_hwndMainDialog;                 // window that spawned us
    HANDLE m_hThread;
    HANDLE m_hEventScriptFile;             // our thread's WaitForSingleObject event
    CRITICAL_SECTION m_criticalSectionST;  // shared resource lock for our thread

    //****************************************************
    // Begin shared resources locked by m_csExecuteScript
    //****************************************************
    CString m_csExecuteScriptStatus;                // set by script thread: status message for display to the user
    vector<CString> m_csExecuteScriptCommandList;   // set by script thread: list of script commands to be executed
    CString m_csScriptToExecute;                    // set by main thread: if not empty, execute this script instead of prompting for one
    bool m_terminateEST;                            // set by main thread: true = script thread should exit
    //****************************************************
    // End shared resources locked by m_csExecuteScript
    //****************************************************
};
