// ==============================================================
// DeltaGliderXR1_IO.cpp: Parses XR1 scenario file settings
//
// Copyright 2006-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
// ==============================================================

#include "DeltaGliderXR1.h"
#include "XR1MultiDisplayArea.h"
#include "XRCommon_IO.h"

// --------------------------------------------------------------
// Read status from scenario file
// --------------------------------------------------------------
void DeltaGliderXR1::clbkLoadStateEx(FILEHANDLE scn, void *vs)
{
    char *line; // used by macros
    int len;    // used by macros
    bool bFound = false;    // used by macros

    // remember that we parsed a scenario file now
    m_parsedScenarioFile = true;

    // Workaround for Orbiter core bug: must init gear parameters here in case gear status not present in the scenario file.
    // This is necessary because Orbiter requires the gear to be DOWN when the scenario first loads if the ship is landed; otherwise, a gruesome crash 
    // occurs due to the "bounce bug".
    gear_status = DOOR_CLOSED;
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
            strcpy (fname, "DG-XR1\\Skins\\");
            strcat (fname, skinpath);
            int n = static_cast<int>(strlen(fname)); fname[n++] = '\\';
            strcpy (fname+n, "dgxr1_1.dds");  skin[0] = oapiLoadTexture (fname);
            strcpy (fname+n, "dgxr1_2.dds");  skin[1] = oapiLoadTexture (fname);
        } 
        else
        {
            // unrecognized option - pass to Orbiter's generic parser
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
void DeltaGliderXR1::clbkSaveState (FILEHANDLE scn)
{
    WriteXRCommonScenarioLines(scn);        // save common data
}
    