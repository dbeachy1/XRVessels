// ==============================================================
// XR2Ravenstar implementation class
//
// Copyright 2008-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR2Keys.cpp
// Class defining custom keys for the XR2 Ravenstar
// ==============================================================

#include "XR2Ravenstar.h"
#include "XR1PayloadDialog.h"

// --------------------------------------------------------------
// Process buffered key events
// --------------------------------------------------------------
#define RET_IF_INCAP() if (IsCrewIncapacitatedOrNoPilotOnBoard()) return 1

int XR2Ravenstar::clbkConsumeBufferedKey(DWORD key, bool down, char *kstate)
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
            // OVERRIDE SUPERCLASS BEHAVIOR: switch between Off -> Pitch -> On
            case OAPI_KEY_SLASH:
            {
                RET_IF_INCAP();
                // make / on main keyboard act the same as numeric keypad /
                // Bits: 0 = elevator, 1 = rudder, 2 = ailerons
                DWORD mode = GetADCtrlMode();
                if (mode == 0)  // off?
                    mode = 1;   // set to "Pitch" mode (elevators only)
                else if (mode == 1)  // pitch?
                    mode = 7;        // set to "On" mode (all surfaces on)
                else // on
                    mode = 0;       // set to "Off"

                SetADCtrlMode(mode);  // Note: a prestep handles APU checks for this
                return 1;
            }

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
            case OAPI_KEY_U: 
                RET_IF_INCAP();
                ToggleBayDoors(); 
                return 1;

            case OAPI_KEY_SPACE:   // DISABLE control dialog -- not implemented for the XR2 (redundant)
                PlayErrorBeep();
                // NO: oapiOpenDialogEx (g_hDLL, IDD_CTRL, XR2Ctrl_DlgProc, DLG_CAPTIONCLOSE, this);
            return 1;
            }
        }
    }

    // this is not an XR2 keypress; send it up to the superclass
    return DeltaGliderXR1::clbkConsumeBufferedKey(key, down, kstate);
}
