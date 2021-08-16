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
// XR1HUD.h
// Handles all HUDs
// ==============================================================

#pragma once

#include "Orbitersdk.h"
#include "vessel3ext.h"
#include "Area.h"
#include "XR1Areas.h"
#include "TextBox.h"

static const int HudDeploySpeed = 90;     // pixels per second

//----------------------------------------------------------------------------------

class HudColorButtonArea : public TimedButtonArea
{
public:
    HudColorButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);
    virtual bool ProcessVCMouseEvent(const int event, const VECTOR3 &coords);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);
    virtual void ProcessTimedEvent(bool &isLit, const bool previousIsLit, const double simt, const double simdt, const double mjd);

protected:
    // data
    double m_lightShutoffTime; // time at which light will be turned off
};

//----------------------------------------------------------------------------------

class TertiaryHUDButtonArea : public XR1Area
{
public:
    TertiaryHUDButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);
};

//----------------------------------------------------------------------------------

class HudIntensitySwitchArea : public VerticalCenteringRockerSwitchArea
{
public:
    HudIntensitySwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID = VCPANEL_TEXTURE_NONE);

protected:
    virtual void ProcessSwitchEvent(SWITCHES switches, POSITION position);
};

//----------------------------------------------------------------------------------

class SecondaryHUDModeButtonsArea : public XR1Area
{
public:
    SecondaryHUDModeButtonsArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);
    // no VC handler for this area
};


//----------------------------------------------------------------------------------

// common base class for all popup HUDs
class PopupHUDArea : public XR1Area
{
public:
    enum class OnOffState { Off, TurningOn, On, TurningOff };  // used for scroll management

    PopupHUDArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int width, const int height);
    virtual ~PopupHUDArea();

    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

    // subclass must implement this
    virtual void SetHUDColors() = 0;    

    // NOTE: this is the caller's responsibility to delete this text box eventually
    void SetTextBox(TextBox *pTextBox) { m_pTextBox = pTextBox; }
    TextBox *GetTextBox() const { return m_pTextBox; }
    OnOffState GetState() const { return m_state; }  // Off, TurningOn, On, TurningOff
    
    // retrieve the background and highlight colors; if a TextBox is present, that value overrides any colors set manually
    COLORREF GetBackgroundColor() const { return m_bgColorRef; }
    COLORREF GetHighlightColor()  const { return m_hlColorRef; }
    void SetHighlightColor(COLORREF highlightColor) { m_hlColorRef = highlightColor; }
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    COLORREF GetColor() const { return m_colorRef; }
    
    void SetColor(COLORREF color);              // will create new pen, too
    void SetBackgroundColor(COLORREF bgColor);  // will create new brush, too

    // subclass must implement these methods
    // NOTE: the subclass MUST draw text from the supplied topY coordinate!
    virtual bool DrawHUD(const int event, const int topyY, HDC hDC, COLORREF colorRef, bool forceRender) = 0;
    virtual bool isOn() = 0;    // returns TRUE if HUD is turned on, FALSE if turned off

protected:
    OnOffState m_state;     // this is the currently DISPLAYED state
    int m_topYCoordinate;   // current top of HUD line; scrolled as HUD turns on or off
    int m_width;
    int m_height;
    COLORREF m_colorRef;
    COLORREF m_bgColorRef;
    COLORREF m_hlColorRef;
    HPEN m_pen0;
    HBRUSH m_hBackgroundBrush;
    TextBox *m_pTextBox;    // may be null
    int m_lastRenderedTopYCoordinate;

    // PostStep data
    double m_startScrollTime; // time when top of HUD started scrolling
    int m_startScrollY;       // Y coordinate of HUD top when scrolling started
    int m_movement;           // +1, -1, or 0; this determines whether we are scrolling up or down
};


//----------------------------------------------------------------------------------

// This object appears above the normal instrument panel; it handles all 5 modes
class SecondaryHUDArea: public PopupHUDArea
{
public:
    SecondaryHUDArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);
    virtual ~SecondaryHUDArea();
    virtual bool DrawHUD(const int event, const int topY, HDC hDC, COLORREF colorRef, bool forceRender);
    virtual bool isOn();    
    virtual void SetHUDColors();
    virtual void RenderCell(HDC hDC, SecondaryHUDMode &secondaryHUD, const int row, const int column, const int topY);
    virtual void PopulateCell(SecondaryHUDMode::Cell &cell);

protected:
    HFONT m_mainFont;
    int m_lineSpacing;  // pixels between text lines
    int m_lastHUDMode;  // 1-5
};

//----------------------------------------------------------------------------------

// This object appears above the normal instrument panel; it handles all 5 modes
class TertiaryHUDArea: public PopupHUDArea
{
public:
    TertiaryHUDArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);
    virtual ~TertiaryHUDArea();
    virtual bool DrawHUD(const int event, const int topY, HDC hDC, COLORREF colorRef, bool forceRender);
    virtual bool isOn();
    virtual void SetHUDColors();

protected:
    HFONT m_mainFont;
    int m_lineSpacing;  // pixels between text lines
};
