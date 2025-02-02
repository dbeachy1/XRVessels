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
// XR5Vanguard implementation class
//
// XR5Keys.cpp
// Class defining custom keys for the XR5 Vanguard
// ==============================================================

#include "XR5Vanguard.h"
#include "XR1PayloadDialog.h"

// --------------------------------------------------------------
// Process direct key events
// --------------------------------------------------------------
int XR5Vanguard::clbkConsumeDirectKey(char *kstate)
{
    // handle any keys we want to override

    /* FOR DEVELOPMENT 
    if (KEYMOD_ALT(kstate))
    {
        if (KEYDOWN(kstate, OAPI_KEY_COMMA)) 
        { 
            NEUTRAL_CENTER_OF_LIFT -= (oapiGetSimStep() * 0.001);
            RESETKEY(kstate, OAPI_KEY_COMMA);
        }

        if (KEYDOWN(kstate, OAPI_KEY_PERIOD)) 
        { 
            NEUTRAL_CENTER_OF_LIFT += (oapiGetSimStep() * 0.001); 
            RESETKEY(kstate, OAPI_KEY_PERIOD);
        }
    }
    */

    // allow our superclass to handle any keys we didn't process
    DeltaGliderXR1::clbkConsumeDirectKey(kstate);

    return 0;
}

// --------------------------------------------------------------
// Process buffered key events
// --------------------------------------------------------------
#define RET_IF_INCAP() if (IsCrewIncapacitatedOrNoPilotOnBoard()) return 1

int XR5Vanguard::clbkConsumeBufferedKey(DWORD key, bool down, char *kstate)
{
    if (Playback()) 
        return 0; // don't allow manual user input during a playback

    // we only want KEYDOWN events
    if (down)
    {
        if (KEYMOD_ALT(kstate)) 
        {
            // ALT key down
            switch(key)
            {
            case OAPI_KEY_J: 
                RET_IF_INCAP();
                SetRCSDockingMode(!m_rcsDockingMode);   // toggle mode
                return 1;

            // GRAPPLE targeted payload
            case OAPI_KEY_G:
                RET_IF_INCAP();
                // handle CTRL-ALT-G for GRAPPLE ALL
                if (KEYMOD_CONTROL(kstate))
                    GrappleAllPayload();
                else
                    GrapplePayload(m_selectedSlot, true);  // beep and show message
                return 1;

            // UNLOAD (DEPLOY) selected payload
            case OAPI_KEY_U:
                RET_IF_INCAP();
                // handle CTRL-ALT-U for DEPLOY ALL
                if (KEYMOD_CONTROL(kstate))
                    DeployAllPayload();
                else
                    DeployPayload(m_selectedSlot, true);   // beep and show message
                return 1;

            // launch payload editor
            case OAPI_KEY_B: 
                // allow if crew incapactiated
                TogglePayloadEditor();
                return 1;
            }
        }

        if (KEYMOD_CONTROL (kstate)) 
        {
            // CTRL key down
            switch(key)
            {
            case OAPI_KEY_E: 
                RET_IF_INCAP();
                ToggleElevator(); 
                return 1;

            case OAPI_KEY_U: 
                RET_IF_INCAP();
                ToggleBayDoors(); 
                return 1;

            case OAPI_KEY_SPACE:   // open control dialog
                oapiOpenDialogEx (g_hDLL, IDD_CTRL, XR5Ctrl_DlgProc, DLG_CAPTIONCLOSE, this);
            return 1;
            }
        }
    }

    // this is not an XR5 keypress; send it up to the superclass
    return DeltaGliderXR1::clbkConsumeBufferedKey(key, down, kstate);
}
