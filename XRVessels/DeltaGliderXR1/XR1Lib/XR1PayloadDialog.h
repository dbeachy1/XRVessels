// ==============================================================
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// Copyright 2007-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR1PayloadDialog.h
// Defines our common payload dialog handler class; this is NOT USED by
// the XR1; it is here for sublcasses to use.
// ==============================================================

#pragma once

#include "DeltaGliderXR1.h"
#include "XR1PayloadBay.h"

// custom messages
#define WM_XR1_UPDATEMASSVALUES (WM_USER+100) /* so we don't step on Orbiter's message IDs */
#define WM_TERMINATE            (WM_USER+101) /* clean up and close dialog gracefully: LPARAM = DeltaGliderXR1 * requesting the close. */

// static payload handler class
class XR1PayloadDialog
{
public:
    static void EditorFunc(OBJHANDLE hVessel);
    static HWND Launch(OBJHANDLE hVessel);
    
protected:
    static DeltaGliderXR1 &GetXR1(HWND hDlg) { return *static_cast<DeltaGliderXR1 *>(oapiGetDialogContext(hDlg)); }  // WARNING: not valid during WM_INITDIALOG! 
    static BOOL CALLBACK Proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);  // message handler
    static void UpdateMassValues(HWND hDlg, const DeltaGliderXR1 &xr1);
    static void UpdatePayloadFields(HWND hDlg, const char *pClassname);
    static bool ProcessSlotButtonMsg(HWND hDlg, const int slotNumber, const HWND hButton, const WORD notificationMsg);

    static bool AddPayloadToSlot(const int slotNumber, HWND hDlg, HWND hButton);
    static bool RemovePayloadFromSlot(const int slotNumber, HWND hDlg, HWND hButton);

    static void RescanBayAndUpdateButtonStates(HWND hDlg, DeltaGliderXR1 *pXR1 = nullptr);
    static void ProcessSelectedPayloadChanged(HWND hDlg, DeltaGliderXR1 *pXR1 = nullptr);

    static void CloseDialog(HWND hDlg);

    // utility methods
    static int GetSelectedPayloadClassname(const HWND hDlg, char *pOut, const int outLength);

    // data
    static const int slotCount;          // total # of slots in the bay
    static const int slotResourceIDs[];  // array of button resource IDs in slot order
    static HFONT s_hOrgFont;      // normal button font handle
    static HFONT s_hBoldFont;     // bold button font handle
};
