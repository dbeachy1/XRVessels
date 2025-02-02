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
// XR5Vanguard_IO.cpp: Parses XR5 scenario file settings
// ==============================================================

#include "XR5Vanguard.h"
#include "XR1MultiDisplayArea.h"
#include "XRCommon_IO.h"

// --------------------------------------------------------------
// Read status from scenario file
// --------------------------------------------------------------
void XR5Vanguard::clbkLoadStateEx(FILEHANDLE scn, void *vs)
{
    char *line; // used by macros
    int len;    // used by macros
    bool bFound = false;  // used by macros

    // remember that we parsed a scenario file now
    m_parsedScenarioFile = true;

    // Workaround for Orbiter core bug: must init gear parameters here in case gear status not present in the scenario file.
    // This is necessary because Orbiter requires the gear to be DOWN when the scenario first loads if the ship is landed; otherwise, a gruesome crash 
    // occurs due to the "bounce bug".
    gear_status = DoorStatus::DOOR_CLOSED;
    gear_proc   = 0.0;
    
    while (oapiReadScenario_nextline (scn, line)) 
    {   
        const bool bParsedCommonLine = ParseXRCommonScenarioLine(line);
        if (bParsedCommonLine)
            continue;

        // parse vessel-specific fields
        IF_FOUND("SKIN") 
        {
            SSCANF1("%s", skinpath);
            char fname[256];
            strcpy (fname, "XR5Vanguard\\Skins\\");
            strcat (fname, skinpath);
            int n = static_cast<int>(strlen(fname)); fname[n++] = '\\';
            strcpy (fname+n, "XR5T.dds");  skin[0] = oapiLoadTexture (fname);
            strcpy (fname+n, "XR5B.dds");  skin[1] = oapiLoadTexture (fname);
        }
        else IF_FOUND("RCS_DOCKING_MODE") 
        {
            SSCANF_BOOL(m_rcsDockingMode);
        }
        else IF_FOUND("ACTIVE_EVA_PORT") 
        {
            SSCANF1("%d", &m_activeEVAPort);
        }
		else IF_FOUND("CREW_ELEVATOR")
		{
			SSCANF2("%d%lf", &crewElevator_status, &crewElevator_proc);
		}
        else
        {
            // unrecognized option - pass to Orbiter's default parser
            ParseScenarioLineEx(line, vs);
        }
    }

    // set default MDM mode if not set 
    if (m_activeMultiDisplayMode < 0)
        m_activeMultiDisplayMode = MDMID_HULL_TEMPS;
}

// --------------------------------------------------------------
// Write status to scenario file
// --------------------------------------------------------------
void XR5Vanguard::clbkSaveState (FILEHANDLE scn)
{
    WriteXRCommonScenarioLines(scn);        // save common data

    // XR5-specific data
	char cbuf[256];
    oapiWriteScenario_int(scn, "RCS_DOCKING_MODE", m_rcsDockingMode);
    oapiWriteScenario_int(scn, "ACTIVE_EVA_PORT", static_cast<int>(m_activeEVAPort));
	sprintf(cbuf, "%d %0.4f", crewElevator_status, crewElevator_proc);
	oapiWriteScenario_string(scn, "CREW_ELEVATOR", cbuf);
}
