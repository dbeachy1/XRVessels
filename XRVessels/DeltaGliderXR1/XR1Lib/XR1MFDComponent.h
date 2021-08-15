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

// ==============================================================
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// XR1MFDComponent.h
// Handles a single MFD
// ==============================================================

#pragma once

#include "orbitersdk.h"
#include "XR1Component.h"

class MFDComponent : public XR1Component
{
public:
    MFDComponent(InstrumentPanel &instrumentPanel, COORD2 topLeft, int mfdID, const int meshTextureID = VCPANEL_TEXTURE_NONE, const int screenMeshGroup = -1);

protected:
    int m_screenMeshGroup;  // mesh group ID of the MFD's screen
};

//----------------------------------------------------------------------------------

class MFDScreenArea : public XR1Area
{
public:
    MFDScreenArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int mfdID, const int meshGroup);
    virtual void Activate();

    int GetMfdID() const { return m_mfdID; }

protected:
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

    int m_mfdID;
    bool m_rebootMFD;  // true if systems failure occurred; reboot when systems restored
    DWORD m_meshGroup;
};

class MFDBottomButtonsArea : public XR1Area
{
public:
    MFDBottomButtonsArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int mfdID);
    virtual void Activate();
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);

    int GetMfdID() const { return m_mfdID; }

protected:
    int m_mfdID;
};

class MFDMainButtonsArea : public XR1Area
{
public:
    enum BUTTON_SIDE { LEFT, RIGHT };   // left side or right side MFD button row
    MFDMainButtonsArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int mfdID, const BUTTON_SIDE buttonSide, const int meshTextureID = VCPANEL_TEXTURE_NONE);
    virtual ~MFDMainButtonsArea();
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);
    virtual bool ProcessVCMouseEvent(const int event, const VECTOR3 &coords);

    int GetMfdID() const { return m_mfdID; }
    BUTTON_SIDE GetButtonSide() const { return m_buttonSide; }

protected:
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);
    bool m_justActivated;      // true if area was activated the previous frame

    HFONT m_font;
    int m_mfdID;
    BUTTON_SIDE m_buttonSide;
};

class VCMFDBottomButtonArea : public XR1Area
{
public:
    enum BUTTON_FUNC { PWR, SEL, MNU };
    VCMFDBottomButtonArea(InstrumentPanel &parentPanel, const int areaID, const int mfdID, const BUTTON_FUNC buttonFunc);
    virtual void Activate();
    virtual bool ProcessVCMouseEvent(const int event, const VECTOR3 &coords);

    int GetMfdID() const { return m_mfdID; }
    BUTTON_FUNC GetButtonFunc() const { return m_buttonFunc; }

protected:
    int m_mfdID;
    BUTTON_FUNC m_buttonFunc;
};

