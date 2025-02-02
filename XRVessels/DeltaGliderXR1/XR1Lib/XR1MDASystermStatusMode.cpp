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

#include "resource.h"

#include "DeltaGliderXR1.h"
#include "XR1MultiDisplayArea.h"

// This class handles all systems status screens, using the delta from MDMID_SYSTEMS_STATUS1 to determine which screen we are
// Constructor
SystemsStatusMultiDisplayMode::SystemsStatusMultiDisplayMode(int modeNumber) :
    MultiDisplayMode(modeNumber),
    m_backgroundSurface(0)
{
    m_screenIndex = modeNumber - MDMID_SYSTEMS_STATUS1;   // index 0...n
}

void SystemsStatusMultiDisplayMode::Activate()
{
    static const int resourceIDs[] = { IDB_SYSTEMS_STATUS1_MULTI_DISPLAY, IDB_SYSTEMS_STATUS2_MULTI_DISPLAY,
                                       IDB_SYSTEMS_STATUS3_MULTI_DISPLAY, IDB_SYSTEMS_STATUS4_MULTI_DISPLAY,
                                       IDB_SYSTEMS_STATUS5_MULTI_DISPLAY };

    m_backgroundSurface = CreateSurface(resourceIDs[m_screenIndex]);
    m_mainFont = CreateFont(14, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, 0, FF_MODERN, "Microsoft Sans Serif");
    m_fontPitch = 11;
}

void SystemsStatusMultiDisplayMode::Deactivate()
{
    DestroySurface(&m_backgroundSurface);
    DeleteObject(m_mainFont);
}

bool SystemsStatusMultiDisplayMode::Redraw2D(const int event, const SURFHANDLE surf)
{
    // render the background
    const COORD2& screenSize = GetScreenSize();
    DeltaGliderXR1::SafeBlt(surf, m_backgroundSurface, 0, 0, 0, 0, screenSize.x, screenSize.y);

    // obtain device context and save existing font
    HDC hDC = m_pParentMDA->GetDC(surf);
    HFONT hPrevObject = (HFONT)SelectObject(hDC, m_mainFont);

    SetBkMode(hDC, TRANSPARENT);
    SetTextAlign(hDC, TA_LEFT);

    // set starting coordinates 
    int x = 5;
    int y = 20;
    int statusX = 136;  // "OK", "OFFLINE", "32%", etc.

    static const int linesPerScreen = 7;
    DamageItem start = static_cast<DamageItem>(static_cast<int>(DamageItem::LeftWing) + (m_screenIndex * linesPerScreen));
    char temp[64];
    for (int i = 0; i < linesPerScreen; i++)
    {
        DamageItem damageItem = static_cast<DamageItem>(static_cast<int>(start) + i);
        if (damageItem > D_END)
            break;  // no more items

        DamageStatus damageStatus = GetXR1().GetDamageStatus(damageItem);

        double integrity = damageStatus.fracIntegrity;

        if (integrity == 1.0)
            SetTextColor(hDC, CREF(MEDIUM_GREEN));
        else
            SetTextColor(hDC, CREF(BRIGHT_RED));

        sprintf(temp, "%s:", damageStatus.label);
        TextOut(hDC, x, y, temp, static_cast<int>(strlen(temp))); // "Left Wing", etc.

        if (damageStatus.onlineOffline)
        {
            if (integrity < 1.0)
                strcpy(temp, "OFFLINE");
            else
                strcpy(temp, "ONLINE");
        }
        else    // can have partial failure
        {
            sprintf(temp, "%d%%", static_cast<int>(integrity * 100));
        }

        TextOut(hDC, statusX, y, temp, static_cast<int>(strlen(temp)));

        // drop to next line
        y += m_fontPitch;
    }

    // restore previous font and release device context
    SelectObject(hDC, hPrevObject);
    m_pParentMDA->ReleaseDC(surf, hDC);

    return true;
}
