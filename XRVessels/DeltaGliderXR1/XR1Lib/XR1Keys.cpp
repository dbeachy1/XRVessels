// ==============================================================
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// Copyright 2006-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR1Keys.cpp
// Contains DeltaGliderXR1 key handler methods
// ==============================================================

#include "DeltaGliderXR1.h"
#include "AreaIDs.h"
#include "XR1MultiDisplayArea.h"

// --------------------------------------------------------------
// Process direct key events
// --------------------------------------------------------------
int DeltaGliderXR1::clbkConsumeDirectKey(char *kstate)
{
    //
    // NOTE: if ATTITUDE HOLD or DESCENT HOLD autopilot engaged, we must "swallow" the normal kepress on the numpad
    //

#define RESET_KEY_IF_INCAP(keyCode)  if (KEYDOWN(kstate, keyCode) && IsCrewIncapacitatedOrNoPilotOnBoard()) RESETKEY(kstate, keyCode)

#define RESET_KEY_IF_PRESSED(keyCode)  if (KEYDOWN(kstate, keyCode)) RESETKEY(kstate, keyCode)

    // swallow these keys regardless of any alt/shift/ctrl pressed
    if (m_customAutopilotMode == AP_ATTITUDEHOLD)
    {
        RESET_KEY_IF_PRESSED(OAPI_KEY_NUMPAD2);
        RESET_KEY_IF_PRESSED(OAPI_KEY_NUMPAD8);
        RESET_KEY_IF_PRESSED(OAPI_KEY_NUMPAD4);
        RESET_KEY_IF_PRESSED(OAPI_KEY_NUMPAD6);
        RESET_KEY_IF_PRESSED(OAPI_KEY_NUMPAD9);
    }
    else if (m_customAutopilotMode == AP_DESCENTHOLD)
    {
        RESET_KEY_IF_PRESSED(OAPI_KEY_NUMPAD2);
        RESET_KEY_IF_PRESSED(OAPI_KEY_NUMPAD8);
        RESET_KEY_IF_PRESSED(OAPI_KEY_NUMPAD0);
        RESET_KEY_IF_PRESSED(OAPI_KEY_DECIMAL);
    }

    if (m_airspeedHoldEngaged)
    {
        RESET_KEY_IF_PRESSED(OAPI_KEY_ADD);
        RESET_KEY_IF_PRESSED(OAPI_KEY_SUBTRACT);
        RESET_KEY_IF_PRESSED(OAPI_KEY_MULTIPLY);
        RESET_KEY_IF_PRESSED(OAPI_KEY_NUMPADENTER);
    }

    // dev testing only!
    /*
    if (KEYMOD_ALT(kstate))
    {
        const double stepSize = (oapiGetSimStep() * 1.0);
        if (KEYDOWN(kstate, OAPI_KEY_SEMICOLON)) 
        { 
            m_centerOfLift -= stepSize;
            RESETKEY(kstate, OAPI_KEY_SEMICOLON);
        }

        if (KEYDOWN(kstate, OAPI_KEY_APOSTROPHE)) 
        { 
            m_centerOfLift += stepSize;
            RESETKEY(kstate, OAPI_KEY_APOSTROPHE);
        }
    }
    */

    if (KEYMOD_ALT(kstate))
    {
        {
            // These two keys are for development testing to tweak some internal value.
            if (KEYDOWN(kstate, OAPI_KEY_1))
            {
                TweakInternalValue(false);      // direction DOWN
                RESETKEY(kstate, OAPI_KEY_1);
            }

            if (KEYDOWN(kstate, OAPI_KEY_2))
            {
                TweakInternalValue(true);      // direction UP
                RESETKEY(kstate, OAPI_KEY_2);
            }

            // center-of-gravity shift keys
            const double cogShiftStep = (oapiGetSimStep() * COL_MAX_SHIFT_RATE * COL_KEY_SHIFT_RATE_FRACTION);
            RESET_KEY_IF_INCAP(OAPI_KEY_COMMA);
            if (KEYDOWN(kstate, OAPI_KEY_COMMA))   // shift COG aft
            { 
                // must shift center of lift *forward* to simulate a COG shift *aft*
                if (VerifyManualCOGShiftAvailable())    // plays warning if necessary
                    ShiftCenterOfLift(cogShiftStep);

                RESETKEY(kstate, OAPI_KEY_COMMA);
            }

            RESET_KEY_IF_INCAP(OAPI_KEY_PERIOD);
            if (KEYDOWN(kstate, OAPI_KEY_PERIOD))  // shift COG forward
            { 
                // must shift center of lift *aft* to simulate a COG shift *forward*
                if (VerifyManualCOGShiftAvailable())    // plays warning if necessary
                    ShiftCenterOfLift(-cogShiftStep);

                RESETKEY(kstate, OAPI_KEY_PERIOD);
            }
        }

        RESET_KEY_IF_INCAP(OAPI_KEY_M);
        if (KEYDOWN(kstate, OAPI_KEY_M))  // recenter COG
        { 
            SetRecenterCenterOfGravityMode(true);
            RESETKEY(kstate, OAPI_KEY_M);
        }

        RESET_KEY_IF_INCAP(OAPI_KEY_ADD);
        if (KEYDOWN(kstate, OAPI_KEY_ADD))  // increment scram thrust
        { 
            if (m_isScramEnabled == false)
            {
                PlaySound(ScramDoorsAreClosed, DeltaGliderXR1::ST_WarningCallout);
                ShowWarning(NULL, DeltaGliderXR1::ST_None, "SCRAM Doors are closed.");
            }
            else    // SCRAM engines enabled
            {
                for (int i = 0; i < 2; i++)
                {
                    IncThrusterLevel (th_scram[i], oapiGetSimStep() * 0.3);
                    scram_intensity[i] = GetThrusterLevel(th_scram[i]) * scram_max[i];
                }
            }
            RESETKEY(kstate, OAPI_KEY_ADD);
        }
        
        RESET_KEY_IF_INCAP(OAPI_KEY_SUBTRACT);
        if (KEYDOWN(kstate, OAPI_KEY_SUBTRACT))  // decrement scram thrust
        { 
            if (m_isScramEnabled == false)
            {
                PlaySound(ScramDoorsAreClosed, DeltaGliderXR1::ST_WarningCallout);
                ShowWarning(NULL, DeltaGliderXR1::ST_None, "SCRAM Doors are closed.");
            }
            else    // SCRAM engines enabled
            {
                for (int i = 0; i < 2; i++)
                {
                    IncThrusterLevel (th_scram[i], oapiGetSimStep() * -0.3);
                    scram_intensity[i] = GetThrusterLevel (th_scram[i]) * scram_max[i];
                }
            }
            RESETKEY(kstate, OAPI_KEY_SUBTRACT);
        }

        // rate is 3% throttle per second vs. normal rate of 30% (1/10th power)
        const double microRate = oapiGetSimStep() * THROTTLE_MICRO_FRAC;

        RESET_KEY_IF_INCAP(OAPI_KEY_EQUALS);
        if (KEYDOWN(kstate, OAPI_KEY_EQUALS))  // small inc SCRAM thrust
        { 
            if (m_isScramEnabled == false)
            {
                PlaySound(ScramDoorsAreClosed, DeltaGliderXR1::ST_WarningCallout);
                ShowWarning(NULL, DeltaGliderXR1::ST_None, "SCRAM Doors are closed.");
            }
            else    // SCRAM engines enabled
            {
                for (int i = 0; i < 2; i++)
                    IncThrusterLevel(th_scram[i], microRate);
            }
            RESETKEY(kstate, OAPI_KEY_EQUALS);
        }

        RESET_KEY_IF_INCAP(OAPI_KEY_MINUS);
        if (KEYDOWN(kstate, OAPI_KEY_MINUS))  // small dec SCRAM thrust
        { 
            if (m_isScramEnabled == false)
            {
                PlaySound(ScramDoorsAreClosed, DeltaGliderXR1::ST_WarningCallout);
                ShowWarning(NULL, DeltaGliderXR1::ST_None, "SCRAM Doors are closed.");
            }
            else    // SCRAM engines enabled
            {
                for (int i = 0; i < 2; i++)
                    IncThrusterLevel(th_scram[i], -microRate);
            }

            RESETKEY(kstate, OAPI_KEY_MINUS);
        }

        // allow if incap
        if (KEYDOWN(kstate, OAPI_KEY_Z))  // decrement HUD brightness
        { 
            oapiDecHUDIntensity();
            RESETKEY(kstate, OAPI_KEY_Z);
        }

        // allow if incap
        if (KEYDOWN(kstate, OAPI_KEY_X))  // increment HUD brightness
        { 
            oapiIncHUDIntensity();
            RESETKEY(kstate, OAPI_KEY_X);
        }

        // gimbal keys
        // Note: gauge is PANEL_REDRAW_ALWAYS, so we don't need to send redraw messages for these
        RESET_KEY_IF_INCAP(OAPI_KEY_SEMICOLON);
        if (KEYDOWN(kstate, OAPI_KEY_SEMICOLON))  // gimbal all up
        { 
            GimbalSCRAMPitch(BOTH, UP_OR_LEFT);
            GimbalMainPitch(BOTH, UP_OR_LEFT);
            RESETKEY(kstate, OAPI_KEY_SEMICOLON);
        }

        RESET_KEY_IF_INCAP(OAPI_KEY_L);
        if (KEYDOWN(kstate, OAPI_KEY_L))  // gimbal all right
        { 
            GimbalMainYaw(BOTH, DOWN_OR_RIGHT);  // only main engines gimbal left/right
            RESETKEY(kstate, OAPI_KEY_L);
        }

        RESET_KEY_IF_INCAP(OAPI_KEY_P);
        if (KEYDOWN(kstate, OAPI_KEY_P))  // gimbal all down
        { 
            GimbalSCRAMPitch(BOTH, DOWN_OR_RIGHT);
            GimbalMainPitch(BOTH, DOWN_OR_RIGHT);
            RESETKEY(kstate, OAPI_KEY_P);
        }

        RESET_KEY_IF_INCAP(OAPI_KEY_APOSTROPHE);
        if (KEYDOWN(kstate, OAPI_KEY_APOSTROPHE))  // gimbal all left
        { 
            GimbalMainYaw(BOTH, UP_OR_LEFT);  // only main engines gimbal left/right
            RESETKEY(kstate, OAPI_KEY_APOSTROPHE);
        }

        RESET_KEY_IF_INCAP(OAPI_KEY_0);
        if (KEYDOWN(kstate, OAPI_KEY_0))  // gimbal recenter all
        { 
            GimbalRecenterAll();
            RESETKEY(kstate, OAPI_KEY_0);
        }
    }

    //---------------------------------

    if (KEYMOD_CONTROL(kstate))
    {
        double delta = oapiGetSimStep() * ELEVATOR_TRIM_SPEED;
        double trimLevel = GetControlSurfaceLevel(AIRCTRL_ELEVATORTRIM);

        if (!AreElevatorsOperational())
        {
            // elevators offline; disable elevator movement keys
            RESETKEY(kstate, OAPI_KEY_COMMA);
            RESETKEY(kstate, OAPI_KEY_PERIOD);
        }

        RESET_KEY_IF_INCAP(OAPI_KEY_COMMA);
        if (KEYDOWN(kstate, OAPI_KEY_COMMA))  // inc elevator trim
        { 
            if (CheckHydraulicPressure(true, true))   // show warning if no hydraulic pressure
            {
                SetControlSurfaceLevel(AIRCTRL_ELEVATORTRIM, trimLevel + delta);
                MarkAPUActive();  // reset the APU idle warning callout time
            }
            RESETKEY(kstate, OAPI_KEY_COMMA);
        }

        RESET_KEY_IF_INCAP(OAPI_KEY_PERIOD);
        if (KEYDOWN(kstate, OAPI_KEY_PERIOD))  // dec elevator trim
        { 
            if (CheckHydraulicPressure(true, true))   // show warning if no hydraulic pressure
            {
                SetControlSurfaceLevel(AIRCTRL_ELEVATORTRIM, trimLevel - delta);
                MarkAPUActive();  // reset the APU idle warning callout time
            }
            RESETKEY(kstate, OAPI_KEY_PERIOD);
        }

        RESET_KEY_IF_INCAP(OAPI_KEY_EQUALS);
        if (KEYDOWN(kstate, OAPI_KEY_EQUALS))  // increment scram thrust
        { 
            if (m_isScramEnabled == false)
            {
                PlaySound(ScramDoorsAreClosed, DeltaGliderXR1::ST_WarningCallout);
                ShowWarning(NULL, DeltaGliderXR1::ST_None, "SCRAM Doors are closed.");
            }
            else    // SCRAM engines enabled
            {
                for (int i = 0; i < 2; i++)
                {
                    IncThrusterLevel(th_scram[i], oapiGetSimStep() * 0.3);
                    scram_intensity[i] = GetThrusterLevel (th_scram[i]) * scram_max[i];
                }
            }
            RESETKEY(kstate, OAPI_KEY_EQUALS);
        }

        RESET_KEY_IF_INCAP(OAPI_KEY_MINUS);
        if (KEYDOWN(kstate, OAPI_KEY_MINUS))  // decrement scram thrust
        { 
            if (m_isScramEnabled == false)
            {
                PlaySound(ScramDoorsAreClosed, DeltaGliderXR1::ST_WarningCallout);
                ShowWarning(NULL, DeltaGliderXR1::ST_None, "SCRAM Doors are closed.");
            }
            else    // SCRAM engines enabled
            {
                for (int i = 0; i < 2; i++)
                {
                    IncThrusterLevel(th_scram[i], oapiGetSimStep() * -0.3);
                    scram_intensity[i] = GetThrusterLevel (th_scram[i]) * scram_max[i];
                }
            }
            RESETKEY(kstate, OAPI_KEY_MINUS);
        }

    }

    //---------------------------------

    if (KEYMOD_SHIFT(kstate))
    {
        // rate is 3% throttle per second vs. normal rate of 30% (1/10th power)
        const double microRate = oapiGetSimStep() * THROTTLE_MICRO_FRAC;

        RESET_KEY_IF_INCAP(OAPI_KEY_NUMPAD0);
        if (KEYDOWN(kstate, OAPI_KEY_NUMPAD0))  // small inc hover thrust
        { 
            if (m_isHoverEnabled == false)
            {
                PlaySound(HoverDoorsAreClosed, DeltaGliderXR1::ST_WarningCallout);
                ShowWarning(NULL, DeltaGliderXR1::ST_None, "Hover Doors are closed.");
            }
            else    // Hover engines enabled
            {
                for (int i = 0; i < 2; i++)
                    IncThrusterLevel(th_hover[i], microRate);
            }
            RESETKEY(kstate, OAPI_KEY_NUMPAD0);
        }

        RESET_KEY_IF_INCAP(OAPI_KEY_DECIMAL);
        if (KEYDOWN(kstate, OAPI_KEY_DECIMAL))  // small dec hover thrust
        { 
            if (m_isHoverEnabled == false)
            {
                PlaySound(HoverDoorsAreClosed, DeltaGliderXR1::ST_WarningCallout);
                ShowWarning(NULL, DeltaGliderXR1::ST_None, "Hover Doors are closed.");
            }
            else    // Hover engines enabled
            {
                for (int i = 0; i < 2; i++)
                    IncThrusterLevel(th_hover[i], -microRate);
            }
            RESETKEY (kstate, OAPI_KEY_DECIMAL);
        }
    }

    //---------------------------------
    // keys that work regardless of KEYMOD state
    
    // check for hover doors here (sound only; Orbiter handles the code)
    RESET_KEY_IF_INCAP(OAPI_KEY_NUMPAD0);
    RESET_KEY_IF_INCAP(OAPI_KEY_DECIMAL);
    if (KEYDOWN(kstate, OAPI_KEY_NUMPAD0) || KEYDOWN(kstate, OAPI_KEY_DECIMAL))  // hover up or down
    { 
        if (m_isHoverEnabled == false)
        {
            PlaySound(HoverDoorsAreClosed, DeltaGliderXR1::ST_WarningCallout);
            ShowWarning(NULL, DeltaGliderXR1::ST_None, "Hover Doors are closed.");

            // reset both keys
            RESETKEY(kstate, OAPI_KEY_NUMPAD0);
            RESETKEY(kstate, OAPI_KEY_DECIMAL);

        }
        // else fall through and let Orbiter have the key
    }

    // detect elevator trim keys to reset APU active timer
    if (KEYDOWN(kstate, OAPI_KEY_INSERT) || KEYDOWN(kstate, OAPI_KEY_DELETE))
        MarkAPUActive();  // reset the APU idle warning callout time

    // reset Orbiter core default keys if crew incapacitated
    RESET_KEY_IF_INCAP(OAPI_KEY_ADD);
    RESET_KEY_IF_INCAP(OAPI_KEY_SUBTRACT);
    RESET_KEY_IF_INCAP(OAPI_KEY_INSERT);   // elevator trim
    RESET_KEY_IF_INCAP(OAPI_KEY_DELETE);   // elevator trim

    // check for APU; required since elevator trim (incorrectly!) works even if AF Ctrl is OFF.
    if (CheckHydraulicPressure(false, false) == false)
    {
        // check whether either trim key pressed
        if (KEYDOWN(kstate, OAPI_KEY_INSERT) || KEYDOWN(kstate, OAPI_KEY_DELETE))
        {
            CheckHydraulicPressure(true, true);     // show warning and play error beep
            RESETKEY(kstate, OAPI_KEY_INSERT);
            RESETKEY(kstate, OAPI_KEY_DELETE);
        }
    }

    return 0;
}

// --------------------------------------------------------------
// Process buffered key events
// --------------------------------------------------------------
#define RET_IF_INCAP() if (IsCrewIncapacitatedOrNoPilotOnBoard()) return 1

int DeltaGliderXR1::clbkConsumeBufferedKey (DWORD key, bool down, char *kstate)
{
    // defines keycodes allowed during playback; currently we do not need to check for CTRL or ALT flags, so we don't
    // bother to check for those at this time.
    static const int s_keysAllowedDuringPlayback[] =
    {
        OAPI_KEY_T,
        // numbers cover [0-9] for MDA as well as CTRL-[1-5] for secondary HUD mode selection
        OAPI_KEY_0,         
        OAPI_KEY_1,
        OAPI_KEY_2,
        OAPI_KEY_3,
        OAPI_KEY_4,
        OAPI_KEY_5,
        OAPI_KEY_6,
        OAPI_KEY_7,
        OAPI_KEY_8,
        OAPI_KEY_9,
        OAPI_KEY_H,     // covers H (hud mode), CTRL-H (hud on/off), and ALT-H (hud color)
        OAPI_KEY_W,     // CTRL-W (reset MWS)
        OAPI_KEY_D      // covers D (next MDA mode) and ALT-D (previous MDA mode)
    };
    static const int s_numKeysAllowedDuringPlayback = (sizeof(s_keysAllowedDuringPlayback) / sizeof(int));

    if (Playback()) 
    {
        // check if the key is allowed during playback
        bool bKeyAllowed = false;
        for (int i=0; i < s_numKeysAllowedDuringPlayback; i++)
        {
            if (key == s_keysAllowedDuringPlayback[i])
            {
                bKeyAllowed = true;
                break;
            }
        }

        if (!bKeyAllowed)
            return 0; // don't allow manual user input during a playback
        // else fall through and process the key normally
    }

    if (!down) 
    {
        // key is up; check for our special cases here
        // NOTE: ALT may not be down here, so don't require it!
        switch(key)
        {
        case OAPI_KEY_SPACE:
            if (m_dataHUDActive)    // data HUD currently active?
            {
                m_dataHUDActive = false;
                PlaySound(SwitchOn, DeltaGliderXR1::ST_Other, MED_CLICK);  // medium click for both on and off
                TriggerRedrawArea(AID_DATA_HUD_BUTTON);
                return 1;
            }
        }

        return 0; // ignore all other keyup events
    }

    if (KEYMOD_SHIFT (kstate)) 
    {
        // SHIFT key down
        // WARNING: ORBITER BUG: SHIFT-<ANY NUMPAD NUMBER KEY> DOES NOT COME THROUGH!  Key remains set to the SHIFT keycode, not the NUMPAD KEY CODE.

        // special autopilot keys
        if (m_airspeedHoldEngaged)
        {
            // OK to check for SHIFT NON-NUMBER keys here
            switch (key)
            {
            case OAPI_KEY_ADD:
                RET_IF_INCAP();
                SetAirspeedHold(true, AS_ADJUST, ASRATE_SMALL);
                return 1;

            case OAPI_KEY_SUBTRACT:
                RET_IF_INCAP();
                SetAirspeedHold(true, AS_ADJUST, -ASRATE_SMALL);
                return 1;
            }
        }

        /*
        switch (key)
        {
            // no entries yet; must be careful here to not step on MFD keystrokes!
        }
        */
    } 
    else if (KEYMOD_CONTROL(kstate)) 
    {
        // CTRL key down
        // autopilot keys
        if (m_customAutopilotMode == AP_DESCENTHOLD)
        {
            const bool invert = GetXR1Config()->InvertDescentHoldRateArrows;
            switch (key)
            {
            case OAPI_KEY_NUMPAD2:
                RET_IF_INCAP();
                if (invert) goto numpad8c_descent;
numpad2c_descent:
                SetAutoDescentRate(true, AD_ADJUST, ADRATE_LARGE);
                return 1;

            case OAPI_KEY_NUMPAD8:
                RET_IF_INCAP();
                if (invert) goto numpad2c_descent;
numpad8c_descent:
                SetAutoDescentRate(true, AD_ADJUST, -ADRATE_LARGE);
                return 1;
            }
        }
        else if (m_customAutopilotMode == AP_ATTITUDEHOLD)
        {
            switch (key)
            {
            case OAPI_KEY_NUMPAD3:   // reset bank to level
                ResetAttitudeHoldToLevel(true, true, false);
                return 1;

            case OAPI_KEY_NUMPAD7:   // reset pitch/aoa to level
                ResetAttitudeHoldToLevel(true, false, true);
                return 1;

            case OAPI_KEY_NUMPAD1:   // reset ship to level
                ResetAttitudeHoldToLevel(true, true, true);
                return 1;
            }
        }
        
        if (m_airspeedHoldEngaged)
        {
            switch (key)
            {
            case OAPI_KEY_ADD:
                RET_IF_INCAP();
                SetAirspeedHold(true, AS_ADJUST, ASRATE_LARGE);
                return 1;

            case OAPI_KEY_SUBTRACT:
                RET_IF_INCAP();
                SetAirspeedHold(true, AS_ADJUST, -ASRATE_LARGE);
                return 1;
            }
        }

        // normal mode (no autopilot engaged)
        switch (key) 
        {
            // swallow normal keys if crew incapacitated
        case OAPI_KEY_DIVIDE:
            RET_IF_INCAP();
            return 0;       // let Orbiter default handler handle it

        case OAPI_KEY_SLASH:
            {
            // WORKAROUND FOR JOY2KEY BUG: it sends numpad "/" as a normal "/", so handle it here.
            RET_IF_INCAP();
            int mode = GetAttitudeMode();
            SetAttitudeMode(mode != 0 ? 0 : 1);  // toggle between off and rotation
            return 1;
            }

        case OAPI_KEY_BACK:   // kill SCRAM thrust
            RET_IF_INCAP(); 
            for (int i = 0; i < 2; i++)
            {
                SetThrusterLevel(th_scram[i], 0);
                scram_intensity[i] = 0;
            }

            PlaySound(_KillThrust, DeltaGliderXR1::ST_Other);
            RESETKEY(kstate, OAPI_KEY_BACK);
            break;

        case OAPI_KEY_D:
            RET_IF_INCAP();
            // use our custom undocking routine
            PerformUndocking();
            return 1;   // we handled this key

        case OAPI_KEY_SPACE: // open control dialog
            // allow if crew incapacitated here
            oapiOpenDialogEx (g_hDLL, IDD_CTRL, XR1Ctrl_DlgProc, DLG_CAPTIONCLOSE, this);
            return 1;

            typedef INT_PTR(CALLBACK* DLGPROC)(HWND, UINT, WPARAM, LPARAM);

        case OAPI_KEY_A:    // toggle APU on/off
            RET_IF_INCAP();
            ToggleAPU();
            PlayDoorSound(apu_status);
            return 1;

        case OAPI_KEY_B:
            RET_IF_INCAP();
            ToggleAirbrake();
            PlayDoorSound(brake_status);
            return 1;

        case OAPI_KEY_K:  // "operate nose cone" (docking port)
            RET_IF_INCAP();
            ToggleNoseCone();
            PlayDoorSound(nose_status);
            return 1;

        case OAPI_KEY_O:  // "operate outer airlock"
            RET_IF_INCAP();
            ToggleOuterAirlock ();
            PlayDoorSound(olock_status);
            return 1;

        case OAPI_KEY_Y:  // operate top hatch
            RET_IF_INCAP();
            ToggleHatch();
            PlayDoorSound(hatch_status);
            return 1;

        case OAPI_KEY_H:  // toggle HUD on/off
            // allow if incap
            PlaySound(SwitchOn, DeltaGliderXR1::ST_Other);    // sound only
            return 0;               // let Orbiter handle it

        case OAPI_KEY_MULTIPLY:   // kill hover thrust
            RET_IF_INCAP();
            for (int i = 0; i < 2; i++)
                SetThrusterLevel(th_hover[i], 0);

            PlaySound(_KillThrust, DeltaGliderXR1::ST_Other);
            return 1;

        case OAPI_KEY_BACKSLASH:    // toggle retro doors
            RET_IF_INCAP();
            ToggleRCover();
            PlayDoorSound(rcover_status);
            return 1;

        case OAPI_KEY_V:        // toggle hover doors
            RET_IF_INCAP();
            ToggleHoverDoors();
            PlayDoorSound(hoverdoor_status);
            return 1;

        case OAPI_KEY_G:        // toggle scram doors
            RET_IF_INCAP();
            ToggleScramDoors();
            PlayDoorSound(scramdoor_status);
            return 1;

        case OAPI_KEY_1:        // set secondary HUD mode
        case OAPI_KEY_2:
        case OAPI_KEY_3:
        case OAPI_KEY_4:
        case OAPI_KEY_5:
            // allow if incap
            EnableAndSetSecondaryHUDMode(key - OAPI_KEY_1 + 1);
            
            return 1;
        
        case OAPI_KEY_T:        // toggle tertiary HUD
            // allow if incap
            SetTertiaryHUDEnabled(!m_tertiaryHUDOn); 
            return 1;

        case OAPI_KEY_W:   // Reset MWS 
            RET_IF_INCAP();
            ResetMWS();
            return 1;

        case OAPI_KEY_SUBTRACT:   // check for retro thrust here
            {
                RET_IF_INCAP();
                // If current throttle level == 0 for BOTH main engines, check the retro doors
                double mainThrottleLevel = GetThrusterLevel(th_main[0]) + GetThrusterLevel(th_main[1]);
                if ((mainThrottleLevel == 0) && (m_isRetroEnabled == false))
                {
                    PlaySound(RetroDoorsAreClosed, DeltaGliderXR1::ST_WarningCallout);
                    ShowWarning(NULL, DeltaGliderXR1::ST_None, "Retro Doors are closed.");
                    return 1;   // swallow this keypress
                }
                return 0;       // let the key be processed by Orbiter's default handler
            }

        case OAPI_KEY_L:  // engage ATTITUDE HOLD and sync to current attitude
            RET_IF_INCAP(); 
            SyncAttitudeHold(true, true); // play sound here and force PITCH mode

            // if autopilot not already engaged, turn it on
            if (m_customAutopilotMode != AP_ATTITUDEHOLD)
                ToggleAttitudeHold();  // use 'toggle' here because we don't have an explicit 'ActivateAttitudeHold' method

            return 1;
        } 
    } 
    else if (KEYMOD_ALT(kstate)) 
    {
        // ALT key down

        // special autopilot keys
        if (m_customAutopilotMode == AP_DESCENTHOLD)
        {
            const bool invert = GetXR1Config()->InvertDescentHoldRateArrows;
            switch (key)
            {
            case OAPI_KEY_NUMPAD2:
                RET_IF_INCAP();
                if (invert) goto numpad8_descent;
numpad2_descent:
                SetAutoDescentRate(true, AD_ADJUST, ADRATE_SMALL);
                return 1;

            case OAPI_KEY_NUMPAD8:
                RET_IF_INCAP();
                if (invert) goto numpad2_descent;
numpad8_descent:
                SetAutoDescentRate(true, AD_ADJUST, -ADRATE_SMALL);
                return 1;
            }
        }
        else if (m_customAutopilotMode == AP_ATTITUDEHOLD)
        {
            const bool invert = GetXR1Config()->InvertAttitudeHoldPitchArrows;
            switch (key)
            {
            case OAPI_KEY_NUMPAD2:
                RET_IF_INCAP(); 
                if (invert) goto numpad8;
numpad2:
                IncrementAttitudeHoldPitch(true, true, AP_PITCH_DELTA_SMALL);
                return 1;

            case OAPI_KEY_NUMPAD8:
                RET_IF_INCAP(); 
                if (invert) goto numpad2;
numpad8:
                DecrementAttitudeHoldPitch(true, true, AP_PITCH_DELTA_SMALL);
                return 1;
            }
        }

        if (m_airspeedHoldEngaged)
        {
            switch (key)
            {
            case OAPI_KEY_ADD:
                RET_IF_INCAP();
                SetAirspeedHold(true, AS_ADJUST, ASRATE_TINY);
                return 1;

            case OAPI_KEY_SUBTRACT:
                RET_IF_INCAP();
                SetAirspeedHold(true, AS_ADJUST, -ASRATE_TINY);
                return 1;
            }
        }

        // normal mode
        switch (key) 
        {
        case OAPI_KEY_R:  // "operate radiator"
            RET_IF_INCAP();
            ToggleRadiator();
            PlayDoorSound(radiator_status);
            return 1;

        case OAPI_KEY_T:        // toggle secondary HUD on/off
            // allow if incap
            if (m_secondaryHUDMode != 0)    // is HUD on?
                DisableSecondaryHUD();  // turn it off
            else    // HUD is off; turn it on
                EnableAndSetSecondaryHUDMode(m_lastSecondaryHUDMode);  // use the last active mode

            PlaySound(SwitchOn, DeltaGliderXR1::ST_Other, QUIET_CLICK);
            TriggerRedrawArea(AID_SECONDARY_HUD_BUTTONS);
            return 1;

            // NOTE: ALT-H will change HUD color; Orbiter core handles this; however, we want to play a sound
        case OAPI_KEY_H:    // Change HUD color
            // allow if incap
            // NOTE: by design, ALT-H is processed by the Orbiter core *before* any vessel-specific code,
            // so it is impossible for a vessel to capture ALT-H anyway; i.e., the Orbiter core *always* 
            // executes oapiToggleHUDColour() when ALT-H is invoked, even before our hook here is called.
            // Therefore, we do not want to invoke oapiToggleHUDColour() here and return 1, like we might expect.
            // We instead simply play a beep for the oapiToggleHUDColour() that the Orbiter core already invoked.
            // ORG CODE: oapiToggleHUDColour();
            PlaySound(BeepHigh, ST_Other);
            // ORG CODE: return 1;   // we handled this
            return 0;  // The core already handled this key, so returning 1 or 0 here makes no difference.

        case OAPI_KEY_D:   // Previous MDA mode
            // allow if incap
            if (!m_pMDA)  // MDA not rendered?
                PlayErrorBeep();
            else
            {
                m_pMDA->SwitchActiveMode(MultiDisplayArea::DOWN);
                PlaySound(BeepLow, ST_Other);
            }
            return 1;

        case OAPI_KEY_SLASH:
            {
                RET_IF_INCAP();
                // make / on main keyboard act the same as numeric keypad /
                DWORD mode = GetADCtrlMode();
                SetADCtrlMode(mode != 0 ? 0 : 7);  // toggle between off and on for all surfaces
                return 1;
            }

        case OAPI_KEY_MULTIPLY:   // kill scram thrust
            RET_IF_INCAP();
            for (int i = 0; i < 2; i++)
            {
                SetThrusterLevel(th_scram[i], 0);
                scram_intensity[i] = 0;
            }

            PlaySound(_KillThrust, DeltaGliderXR1::ST_Other);
            return 1;

        case OAPI_KEY_SPACE:    // show data HUD
            // allow if incap
            m_dataHUDActive = true;
            PlaySound(SwitchOn, DeltaGliderXR1::ST_Other, MED_CLICK);  // medium click for both on and off
            TriggerRedrawArea(AID_DATA_HUD_BUTTON);
            return 1;

        case OAPI_KEY_S:  // toggle AIRSPEED HOLD autopilot
            RET_IF_INCAP(); 
            ToggleAirspeedHold(true);   // hold current airspeed
            return 1;

        case OAPI_KEY_O:    // toggle INNER AIRLOCK DOOR
            RET_IF_INCAP();
            ToggleInnerAirlock();
            return 1;
        }
    }
    else   // normal key (not SHIFT, CTRL, or ALT)
    {    
        //
        // Handle custom autopilot mode-specific keys
        //
        if (m_customAutopilotMode == AP_ATTITUDEHOLD)
        {
            const bool invert = GetXR1Config()->InvertAttitudeHoldPitchArrows;
            switch (key)
            {
            case OAPI_KEY_NUMPAD2:
                RET_IF_INCAP(); 
                if (invert) goto numpad8n;
numpad2n:
                IncrementAttitudeHoldPitch(true, true, AP_PITCH_DELTA_LARGE);
                return 1;

            case OAPI_KEY_NUMPAD8:
                RET_IF_INCAP(); 
                if (invert) goto numpad2n;
numpad8n:
                DecrementAttitudeHoldPitch(true, true, AP_PITCH_DELTA_LARGE);
                return 1;

            case OAPI_KEY_NUMPAD4:
                RET_IF_INCAP(); 
                IncrementAttitudeHoldBank(true, true);
                return 1;

            case OAPI_KEY_NUMPAD6:
                RET_IF_INCAP(); 
                DecrementAttitudeHoldBank(true, true);
                return 1;

            case OAPI_KEY_NUMPAD9:
                RET_IF_INCAP(); 
                ToggleAOAPitchAttitudeHold(true);
                return 1;
            }
        }
        else if (m_customAutopilotMode == AP_DESCENTHOLD)
        {
            const bool invert = GetXR1Config()->InvertDescentHoldRateArrows;
            switch(key)
            {
            case OAPI_KEY_NUMPAD2:
                RET_IF_INCAP(); 
                if (invert) goto numpad8n_descent;
numpad2n_descent:
                SetAutoDescentRate(true, AD_ADJUST, ADRATE_MED);
                return 1;

            case OAPI_KEY_NUMPAD8:
                RET_IF_INCAP(); 
                if (invert) goto numpad2n_descent;
numpad8n_descent:
                SetAutoDescentRate(true, AD_ADJUST, -ADRATE_MED);
                return 1;

            case OAPI_KEY_NUMPAD0:
                RET_IF_INCAP(); 
                SetAutoDescentRate(true, AD_AUTOLAND, 0);
                return 1;

            case OAPI_KEY_DECIMAL:
                RET_IF_INCAP(); 
                SetAutoDescentRate(true, AD_LEVEL, 0);
                return 1;
            }
        }

        if (m_airspeedHoldEngaged)
        {
            switch(key)
            {
            case OAPI_KEY_ADD:
                RET_IF_INCAP(); 
                SetAirspeedHold(true, AS_ADJUST, ASRATE_MED);
                return 1;

            case OAPI_KEY_SUBTRACT:
                RET_IF_INCAP(); 
                SetAirspeedHold(true, AS_ADJUST, -ASRATE_MED);
                return 1;
            
            case OAPI_KEY_NUMPADENTER:
                RET_IF_INCAP(); 
                SetAirspeedHold(true, AS_HOLDCURRENT, 0);
                return 1;

            case OAPI_KEY_MULTIPLY:
                RET_IF_INCAP(); 
                SetAirspeedHold(true, AS_RESET, 0);
                return 1;
            }
        }

        // 
        // Perform normal key checks here
        //
        switch (key) 
        {
            // swallow normal keys if crew incapacitated
        case OAPI_KEY_DIVIDE:
            RET_IF_INCAP();
            return 0;       // let Orbiter default handler handle it

        // NOTE: this replaces the standard Orbiter "Level Horizon" autopilot key
        case OAPI_KEY_L:  // toggle ATTITUDE HOLD autopilot
            RET_IF_INCAP(); 
            ToggleAttitudeHold();
            return 1;

        // NOTE: this replaces the standard Orbiter "Hover Hold Alt" autopilot key
        case OAPI_KEY_A:  // toggle DESCENT HOLD autopilot
            RET_IF_INCAP(); 
            ToggleDescentHold();
            return 1;

        case OAPI_KEY_LBRACKET:
        case OAPI_KEY_RBRACKET:
        case OAPI_KEY_SEMICOLON:
        case OAPI_KEY_APOSTROPHE:
        case OAPI_KEY_NUMPAD5:   // killrot
            // swallow this key
            RET_IF_INCAP();    
            return 0;       // let Orbiter's default handler handle it

        case OAPI_KEY_0:
        case OAPI_KEY_1:
        case OAPI_KEY_2:
        case OAPI_KEY_3:
        case OAPI_KEY_4:
        case OAPI_KEY_5:
        case OAPI_KEY_6:
        case OAPI_KEY_7:
        case OAPI_KEY_8:
        case OAPI_KEY_9:
            // allow if incap
            if (!m_pMDA)  // MDA not rendered?
                PlayErrorBeep();
            else
            {
                int modeNumber = ((key == OAPI_KEY_0) ? 0 : (key-1));
                if (m_pMDA->SetActiveMode(modeNumber) == false)
                {
                    char temp[64];
                    sprintf(temp, "No such display mode: %d", modeNumber);
                    PlayErrorBeep();
                    ShowWarning(NULL, DeltaGliderXR1::ST_None, temp);
                }
                else
                    PlaySound(BeepHigh, ST_Other);
            }
            return 1;

        case OAPI_KEY_D:
            // allow if incap
            if (!m_pMDA)  // MDA not rendered?
                PlayErrorBeep();
            else
            {
                m_pMDA->SwitchActiveMode(MultiDisplayArea::UP);
                PlaySound(BeepHigh, ST_Other);
            }
            return 1;

        case OAPI_KEY_H:  // switch HUD mode
            // allow if incap
            PlaySound(SwitchOn, DeltaGliderXR1::ST_Other, MED_CLICK);
            return 0;               // let Orbiter handle it

        case OAPI_KEY_SLASH:
            RET_IF_INCAP();
            // WORKAROUND FOR JOY2KEY BUG: it sends numpad "/" as a normal "/", so handle it here.
            ToggleAttitudeMode();
            return 1;

        case OAPI_KEY_MULTIPLY:   // kill main thrust
            RET_IF_INCAP();
            PlaySound(_KillThrust, DeltaGliderXR1::ST_Other);
            return 0;   // let Orbiter default handler have the key
        
        case OAPI_KEY_SUBTRACT:
            RET_IF_INCAP();
            if (m_isRetroEnabled == false)
            {
                PlaySound(RetroDoorsAreClosed, DeltaGliderXR1::ST_WarningCallout);
                ShowWarning(NULL, DeltaGliderXR1::ST_None, "Retro Doors are closed.");
                return 1;   // swallow this keypress
            }
            return 0;       // let the key be processed by Orbiter's default handler

        case OAPI_KEY_G:  // "operate landing gear"
            RET_IF_INCAP();
            ToggleLandingGear();
            // do not play sound here; we have voice for this
            return 1;

        case OAPI_KEY_SPACE:  // disable autopilots
            RET_IF_INCAP();
            KillAllAutopilots();    // sound will play automatically
            return 1;
        }       // end of normal switch statement
    }
    return 0;
}

