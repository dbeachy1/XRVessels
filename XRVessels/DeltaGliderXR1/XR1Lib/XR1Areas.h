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
// XR1Area.h
// Abstract area base class that each of our Areas extend
// Also includes additional base classes that add functionality
// ==============================================================

#pragma once

#include "Orbitersdk.h"
#include "vessel3ext.h"
#include "Area.h"
#include "XR1Colors.h"

class DeltaGliderXR1;

// Define XR1 VC mesh texture IDs; these are converted to actual texture indices in the XR1's mesh by 
// our MeshTextureIDToTextureIndex method.  These constants are arbitrary and are ONLY used by the XR1 (no subclasses).
// NOTE: VCPANEL_TEXTURE_NONE = -1 = "no texture" (i.e., "not applicable"), so these texture IDs here must start at zero or higher.
#define XR1_VCPANEL_TEXTURE_LEFT    0
#define XR1_VCPANEL_TEXTURE_CENTER  1
#define XR1_VCPANEL_TEXTURE_RIGHT   2

class XR1Area : public Area
{
public:
    XR1Area(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID = VCPANEL_TEXTURE_NONE);
    virtual ~XR1Area();

    // convenience methods
    DeltaGliderXR1 &GetXR1() const { return static_cast<DeltaGliderXR1 &>(GetVessel()); }

    // utility methods
    COLORREF XR1Area::GetTempCREF(const double tempK, double limitK, const DoorStatus doorStatus) const;
    COLORREF GetValueCREF(double value, double warningLimit, double criticalLimit) const;

    // static worker methods to convert values
    static double MetersToFeet(const double meters) { return (meters * 3.2808399); }
    static double MetersToMiles(const double meters) { return (meters * 3.2808399); }
    static double MpsToMph(const double mps) { return (mps * 2.23693629); }  // meters per second to MPH
    static double PaToPsi(const double pa) { return (pa * 1.45037738e-4); }  // pascals to PSI
    static double KelvinToFahrenheit(const double k) { return (((k - 273.15) * (9.0/5.0)) + 32); }
    static double KelvinToCelsius(const double k) { return (k - 273.15); }
    static double CelsiusToKelvin(const double c) { return (c + 273.15); }
    static double CelsiusToFahrenheit(const double c) { return ((c * (9.0/5.0)) + 32); }
    static double MpsToFpm(const double mps) { return (mps * 196.850394); }  // meters per second to feet per minute
    static double Mps2ToG(const double mps2) { return (mps2 / G); }  // meters/second^2 to G's (acc)
    static double KgToPounds(const double kg) { return (kg * 2.20462262); } // kilograms to pounds
    static double NewtonsToPounds(const double n) { return n * 0.224808943; }
};

//----------------------------------------------------------------------------------

class VerticalCenteringRockerSwitchArea : public XR1Area
{
public:
    enum class POSITION { UP, DOWN, CENTER };               // switch position
    enum class SWITCHES { LEFT, RIGHT, BOTH, SINGLE, NA };  // which switch(es) moved?   NOTE: do not change the order of these values!
    VerticalCenteringRockerSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID, bool isDual, bool reverseRotation = false, POSITION initialPosition = POSITION::CENTER);
    void SetXRAnimationHandle(UINT * const pAnimationHandle) { m_pAnimationHandle = pAnimationHandle; }  // VC 3D switch; defaults to NULL
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool Redraw3D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);
    virtual bool ProcessVCMouseEvent(const int event, const VECTOR3 &coords);

protected:
    DeltaGliderXR1::GIMBAL_SWITCH ToGIMBAL_SWITCH(SWITCHES switches)
    {
        if (switches == SWITCHES::LEFT)
            return DeltaGliderXR1::GIMBAL_SWITCH::LEFT;
        else if (switches == SWITCHES::RIGHT)
            return DeltaGliderXR1::GIMBAL_SWITCH::RIGHT;
        else
            return DeltaGliderXR1::GIMBAL_SWITCH::BOTH;   // SINGLE or NA should never happen here
    }

    DeltaGliderXR1::DIRECTION ToDIRECTION(POSITION position)
    {
        if (position == POSITION::UP)
            return DeltaGliderXR1::DIRECTION::UP_OR_LEFT;
        else if (position == POSITION::DOWN)
            return DeltaGliderXR1::DIRECTION::DOWN_OR_RIGHT;
        else 
            return DeltaGliderXR1::DIRECTION::DIR_NONE;
    }

    virtual bool DispatchSwitchEvent(const int event, SWITCHES &switches, POSITION &position);  // common mouse code for 2D and 3D

    // the subclass must hook this to process the switch event
    virtual void ProcessSwitchEvent(SWITCHES switches, POSITION position) = 0;

    // data
    UINT *m_pAnimationHandle;
    POSITION m_initialPosition;         // switch is set to this on activation
    POSITION m_lastSwitchPosition[2];   // UP, DOWN, CENTER
    bool m_isDual;
    bool m_reverseRotation;       // if true, reverses animation rotation actions
};

//----------------------------------------------------------------------------------

class HorizontalCenteringRockerSwitchArea : public XR1Area
{
public:
    enum class POSITION { LEFT, RIGHT, CENTER };          // switch position
    enum class SWITCHES { TOP, BOTTOM, BOTH, SINGLE, NA };  // which switch(es) moved?   NOTE: do not change the order of these values!
    HorizontalCenteringRockerSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID, bool isDual, bool reverseRotation = false, POSITION initialPosition = POSITION::CENTER);
    void SetXRAnimationHandle(UINT * const pAnimationHandle) { m_pAnimationHandle = pAnimationHandle; }  // VC 3D switch; defaults to NULL
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool Redraw3D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);
    virtual bool ProcessVCMouseEvent(const int event, const VECTOR3 &coords);
    
protected:
    DeltaGliderXR1::GIMBAL_SWITCH ToGIMBAL_SWITCH(SWITCHES switches)
    {
        if (switches == SWITCHES::TOP)  // top switch is LEFT engine
            return DeltaGliderXR1::GIMBAL_SWITCH::LEFT;
        else if (switches == SWITCHES::BOTTOM)
            return DeltaGliderXR1::GIMBAL_SWITCH::RIGHT;
        else
            return DeltaGliderXR1::GIMBAL_SWITCH::BOTH;   // SINGLE or NA should never happen here
    }

    DeltaGliderXR1::DIRECTION ToDIRECTION(HorizontalCenteringRockerSwitchArea::POSITION position)
    {
        if (position == HorizontalCenteringRockerSwitchArea::POSITION::LEFT)
            return DeltaGliderXR1::DIRECTION::UP_OR_LEFT;
        else if (position == HorizontalCenteringRockerSwitchArea::POSITION::RIGHT)
            return DeltaGliderXR1::DIRECTION::DOWN_OR_RIGHT;
        else 
            return DeltaGliderXR1::DIRECTION::DIR_NONE;
    }

    virtual bool DispatchSwitchEvent(const int event, SWITCHES &switches, HorizontalCenteringRockerSwitchArea::POSITION &position);  // common mouse code for 2D and 3D

    // the subclass must hook this to process the switch event
    virtual void ProcessSwitchEvent(SWITCHES switches, HorizontalCenteringRockerSwitchArea::POSITION position) = 0;

    // data
    UINT *m_pAnimationHandle;
    HorizontalCenteringRockerSwitchArea::POSITION m_initialPosition;         // switch is set to this on activation
    HorizontalCenteringRockerSwitchArea::POSITION m_lastSwitchPosition[2];   // LEFT, RIGHT, CENTER
    bool m_isDual;
    bool m_reverseRotation;       // if true, reverses animation rotation actions
};

//----------------------------------------------------------------------------------

// base class for all indicator gauge areas
class IndicatorGaugeArea : public XR1Area
{
public:
    enum class COLOR { GREEN, RED, YELLOW, WHITE, NONE };  // color of indicator arrow
    IndicatorGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const bool isDual, 
                                       const int redrawFlag, const int meshTextureID, const int deltaX, const int deltaY, const int gapSize);

    virtual void Activate();
    virtual void Deactivate();

protected:
    SURFHANDLE GetSurfaceForColor(COLOR c);  // subclasses should not override this
    
    // subclass must implement these
    virtual COORD2 GetAreaSize() = 0;
    virtual void ResetRenderData() = 0;
    
    // data
    SURFHANDLE m_redIndicatorSurface;     // secondary surface
    SURFHANDLE m_yellowIndicatorSurface;  // secondary surface
    bool m_isDual;
    int m_redrawFlag;
    int m_deltaX, m_deltaY;
    int m_gapSize;
};

//----------------------------------------------------------------------------------

class VerticalGaugeArea : public IndicatorGaugeArea
{
public:
    enum class SIDE { LEFT, RIGHT };        // which gauge to render

    // render data passed back from subclass
    struct RENDERDATA 
    { 
        COLOR color; 
        int indexY;

        void Reset() { color = COLOR::NONE; indexY = -1; }
        bool operator!=(const RENDERDATA &that) const
        {
             return ((color != that.color) || (indexY != that.indexY));
        }
        
        bool operator==(const RENDERDATA &that) const
        {
             return ((color == that.color) & (indexY == that.indexY));
        }

        // no need to implement operator=; default byte-for-byte copy is fine
    }; 
    
    // convenience inline
    inline RENDERDATA _RENDERDATA(COLOR color, int indexY)
    {
        RENDERDATA rd = { color, indexY }; 
        return rd;
    };

    VerticalGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const bool isDual, const int sizeY, 
                      const int redrawFlag, const int meshTextureID = VCPANEL_TEXTURE_NONE, const int deltaX = 0, const int deltaY = 0, const int gapSize = 1, const SIDE singleSide = SIDE::LEFT);
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);

protected:
    virtual COORD2 GetAreaSize();
    virtual void ResetRenderData();

    // the subclass must implement this method 
    virtual RENDERDATA GetRenderData(const SIDE side) = 0;

    // the subclass may override this method; default impl does nothing
    virtual void Redraw2DFirstHook(const int event, SURFHANDLE surf) { }

    // data
    int m_sizeY;    // height of registered area in pixels
    SIDE m_singleSide;  // for a single gauge
    RENDERDATA m_lastRenderData[2]; // one for each indicator 
};

//----------------------------------------------------------------------------------

class HorizontalGaugeArea : public IndicatorGaugeArea
{
public:
    enum class SIDE { TOP, BOTTOM };         // which gauge to render

    // render data passed back from subclass
    struct RENDERDATA 
    { 
        IndicatorGaugeArea::COLOR color;
        int indexX;

        void Reset() { color = IndicatorGaugeArea::COLOR::NONE; indexX = -1; }
        bool operator!=(const RENDERDATA &that) const
        {
             return ((color != that.color) || (indexX != that.indexX));
        }
        
        bool operator==(const RENDERDATA &that) const
        {
             return ((color == that.color) & (indexX == that.indexX));
        }

        // no need to implement operator=; default byte-for-byte copy is fine
    }; 
    
    // convenience inline
    inline RENDERDATA _RENDERDATA(COLOR color, int indexX)
    {
        RENDERDATA rd = { color, indexX }; 
        return rd;
    };

    // NOTE: we need six extra pixels in width to accomodate 1/2 of the pointer sticking out over each end of the bar (3 pixels per side)
    HorizontalGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const bool isDual, const int sizeX, 
                        const int redrawFlag, const int meshTextureID = 1, const int deltaX = 0, const int deltaY = 0, const int gapSize = 1, const SIDE singleSide = SIDE::BOTTOM);
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);

protected:
    virtual COORD2 GetAreaSize();
    virtual void ResetRenderData();

    // the subclass must implement this method 
    virtual RENDERDATA GetRenderData(const SIDE side) = 0;

    // the subclass may override this method; default impl does nothing
    virtual void Redraw2DFirstHook(const int event, SURFHANDLE surf) { }

    // data
    int m_sizeX;    // width of registered area in pixels
    SIDE m_singleSide;  // for a single gauge
    RENDERDATA m_lastRenderData[2]; // one for each indicator 
};

//----------------------------------------------------------------------------------

class SimpleButtonArea : public XR1Area
{
public:
    SimpleButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, bool *pIsLit = nullptr, const int buttonMeshGroup = -1);

    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool Redraw3D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);

protected:
    // data
    int m_buttonMeshGroup;  // 3D button mesh group, or -1 if none
    bool *m_pIsLit;         // true if button is lit up (green)

private:
    bool m_defaultIsLit;    // for subclasses that pass NULL to pIsLit in the constructor
};

//----------------------------------------------------------------------------------

class TimedButtonArea : public SimpleButtonArea
{
public:
    TimedButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, bool *pIsLit = nullptr, const int buttonMeshGroup = -1);

    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

    // invoked at each timestep
    virtual void ProcessTimedEvent(bool &isLit, const bool previousIsLit, const double simt, const double simdt, const double mjd) = 0;

protected:
    // data
    bool m_previousIsLit;   // previous value in clbkPostStep
};

//----------------------------------------------------------------------------------

class AnalogGaugeArea : public XR1Area
{
public:
    AnalogGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const double initialAngle, const int meshTextureID = VCPANEL_TEXTURE_NONE);
    virtual void Activate();
    virtual void Deactivate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
   
protected:
    void DrawNeedle(HDC hDC, int x, int w, double rad, double angle, double speed = 3.0);
    // the subclass must hook this to determine the needle angle
    virtual double GetDialAngle() = 0;

    // data
    double m_initialAngle;        // angle on initial render, in radians
    double m_lastIndicatorAngle;  // angle in radians
    HPEN m_pen0, m_pen1;
};

//----------------------------------------------------------------------------------

class ToggleSwitchArea : public XR1Area
{
public:
    ToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID);
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);

protected:
    // the subclass must hook the next two methods
    virtual bool ProcessSwitchEvent(bool switchIsOn) = 0;
    virtual bool isOn() = 0;

    // data
    int m_indicatorAreaID;
};

//----------------------------------------------------------------------------------

class LEDArea : public XR1Area
{
public:
    LEDArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const bool &isOn);
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);

protected:
    const bool &m_isOn;
    DWORD m_color;
};

//----------------------------------------------------------------------------------

class DoorIndicatorArea : public XR1Area
{
public:
    DoorIndicatorArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int meshTextureID, const DoorStatus *pDoorStatus, const int surfaceIDB, const double *pAnimationState);
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

protected:
    const DoorStatus *m_pDoorStatus;
    int   m_indicatorAreaID;
    int   m_surfaceIDB;
    int   m_transitIndex;     // pixel index of transit markers
    DWORD m_transitColor;     // current color of transit markers
    const double *m_pAnimationState;  // may be null
    bool  m_isTransitVisible; // true if 'Transit' is visible during blinking
};

//----------------------------------------------------------------------------------

class BarArea : public XR1Area
{
public:
    enum class COLOR { GREEN, RED, YELLOW, WHITE, NONE };  // color of bar
    enum class ORIENTATION { HORIZONTAL, VERTICAL };  // orientation of bar
    enum class BARPORTION { BRIGHT, DARK };  

    // render data passed back from subclass
    class RENDERDATA
    { 
    public:
        RENDERDATA() :  // only used during initialization
          m_pBarArea(nullptr), color(COLOR::NONE), startingDarkValue(0), value(0), maxValue(0) { }

        // startingDarkValue = value at which a darker bar is rendered; will match maxValue if no dark bar present
        RENDERDATA(BarArea *pBarArea, COLOR color, double startingDarkValue, double value, double maxValue) : 
          m_pBarArea(pBarArea), color(color), value(value), maxValue(maxValue), startingDarkValue(startingDarkValue) { }

        COLOR color; 
        // startingDarkValue <= value <= maxValue
        double startingDarkValue;  // this is really the edge of the *internal tanks* qty
        double value;              // this is the top edge of the dark portion, which includes bay qty
        double maxValue;           // this is the gauge size

        // wouldn't really need to reset values here, but it won't hurt
        // NOTE: remember that startingDarkValue must always be <= value
        void Reset() { color = COLOR::NONE; value = startingDarkValue = 0; }

        // compute X or Y size (i.e., TOP) of the DARK or BRIGHT portion of the gauge bar (depends on orientation)
        int GetIndex(BARPORTION bp) const 
        { 
            // Note: if this is invoked from an operator method, m_pBarArea may be NULL.
            // In that case, we don't have an index to compute, so return -1.
            if (m_pBarArea == nullptr)     // no actual render for this object before?
                return -1;

            _ASSERTE(startingDarkValue <= value);
            _ASSERTE(value <= maxValue);
            const double workingValue = ((bp == BARPORTION::DARK) ? value : startingDarkValue);
            double fraction = SAFE_FRACTION(workingValue, maxValue);  // 0...1
            if (fraction > 1.0)
            {
                _ASSERTE(false);  // code bug
                fraction = 1.0;   
            }
            else if (fraction < 0)
            {
                _ASSERTE(false);  // code bug
                fraction = 0;   
            }

            int barLength;
            if (m_pBarArea->m_orientation == ORIENTATION::HORIZONTAL)
                barLength = static_cast<int>(((m_pBarArea->m_sizeX * fraction) + 0.5));  // round to nearest pixel
            else
                barLength = static_cast<int>(((m_pBarArea->m_sizeY * fraction) + 0.5));  // round to nearest pixel

            return barLength;
        }

        // equality is based on LAST RENDERED WIDTH and COLOR, *not* value.
        bool operator!=(const RENDERDATA &that) const
        {
             return ((color != that.color) || 
                     (GetIndex(BARPORTION::DARK) != that.GetIndex(BARPORTION::DARK)) ||
                     (GetIndex(BARPORTION::BRIGHT) != that.GetIndex(BARPORTION::BRIGHT))
                    );
        }
        
        bool operator==(const RENDERDATA &that) const
        {
             return ((color == that.color) && 
                     (GetIndex(BARPORTION::DARK) == that.GetIndex(BARPORTION::DARK)) &&
                     (GetIndex(BARPORTION::BRIGHT) == that.GetIndex(BARPORTION::BRIGHT))
                    );
        }

        // no need to implement operator=; default byte-for-byte copy is fine

    private:
        BarArea *m_pBarArea; // our parent object
    }; 
    
    // convenience inline
    inline RENDERDATA _RENDERDATA(const COLOR color, const double startingDarkValue, double value, double maxValue)
    {
        // copy new object by value
        return RENDERDATA(this, color, startingDarkValue, value, maxValue);
    };

    // Constructor
    BarArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, int sizeX, int sizeY, ORIENTATION orientation);
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);

protected:
    // the subclass must implement this method 
    virtual RENDERDATA GetRenderData() = 0;

    // the subclass may override this method; default impl does nothing
    // it is invoked AFTER the main bar is rendered
    virtual void RedrawAfterHook(const int event, SURFHANDLE surf) { }

    // state data 
    int m_sizeX, m_sizeY;   // width and height of bar
    RENDERDATA m_lastRenderData;
    ORIENTATION m_orientation;  // VERTICAL or HORIZONTAL
};

//----------------------------------------------------------------------------------

class NumberArea : public XR1Area
{
public:
    enum class COLOR { GREEN, YELLOW, RED, BLUE, WHITE };

    // render data updated by the subclass
    class RENDERDATA
    { 
    public:
        RENDERDATA(int sizeInChars) : forceRedraw(false), value(0), color(COLOR::GREEN) { pStrToRender = new char[sizeInChars + 1]; }
        virtual ~RENDERDATA() { delete pStrToRender; }
        // NOTE: do not set value=-999 here!  The string might not be long enough to render it, resulting in a heap overrun.
        void Reset() { forceRedraw = true; value=0; color = COLOR::GREEN; }
        double value;
        char *pStrToRender;  // initialized in constructor
        bool forceRedraw;
        COLOR color;   // defaults to GREEN on initialization
    }; 

    // Constructor
    NumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, int sizeInChars, bool hasDecimal);
    virtual ~NumberArea();
    virtual void Activate();
    virtual void Deactivate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);

protected:
    // the subclass must implement this method 
    virtual bool UpdateRenderData(RENDERDATA &renderData) = 0;

    // data
    SURFHANDLE m_font2Yellow;
    SURFHANDLE m_font2Red;
    SURFHANDLE m_font2Blue;
    SURFHANDLE m_font2White;

    // state data 
    int m_sizeInChars;
    bool m_hasDecimal;
    RENDERDATA *m_pRenderData;
};

//----------------------------------------------------------------------------------

class PctHorizontalGaugeArea : public HorizontalGaugeArea
{
public:
    // NOTE: we need six extra pixels in width to accomodate 1/2 of the pointer sticking out over each end of the bar (3 pixels per side)
    PctHorizontalGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const bool isDual, const int sizeX, const int redrawFlag, const int meshTextureID = VCPANEL_TEXTURE_NONE, const int deltaX = 0, const int deltaY = 0, const int gapSize = 1);

protected:
    virtual RENDERDATA GetRenderData(const SIDE side);

    // subclass must hook this method
    virtual double GetFraction(const SIDE side, COLOR &color) = 0;
};

//----------------------------------------------------------------------------------

class ThrustNumberArea : public NumberArea
{
public:
    ThrustNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

protected:
    virtual bool UpdateRenderData(RENDERDATA &renderData);
    
    // subclass must implement this method
    virtual double GetThrust() = 0;  // thrust in kN
};

//----------------------------------------------------------------------------------

class AccNumberArea : public NumberArea
{
public:
    enum class AXIS { X, Y, Z };
    AccNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const AXIS axis);

protected:
    virtual bool UpdateRenderData(RENDERDATA &renderData);
    AXIS m_axis;
};

//----------------------------------------------------------------------------------

class AccHorizontalGaugeArea : public HorizontalGaugeArea
{
public:
    enum class AXIS { X, Y, Z };
    // NOTE: we need six extra pixels in width to accomodate 1/2 of the pointer sticking out over each end of the bar (3 pixels per side)
    AccHorizontalGaugeArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const AXIS axis, const bool isDual, const SIDE side, const int meshTextureID = VCPANEL_TEXTURE_NONE);

protected:
    virtual void Activate();
    virtual RENDERDATA GetRenderData(const SIDE side);
    AXIS m_axis;
};

//----------------------------------------------------------------------------------

class AccScaleArea : public XR1Area
{
public:
    AccScaleArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);

    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);

protected:
    AccScale m_accScale;            // current rendered Acc scale
};


//----------------------------------------------------------------------------------

class MomentaryButtonArea : public XR1Area
{
public:
    MomentaryButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int buttonMeshGroup = -1);

    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool Redraw3D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);

protected:
    // subclass hooks this method 
    virtual void ProcessButtonAction(int event, double buttonDownSimt) = 0;

    // subclass may override this method if desired to handle special button lighting conditions
    virtual bool IsLit() { return m_isLit; }

    // data
    int m_buttonMeshGroup;  // 3D button mesh group, or -1 if none
    bool m_isLit;           // true if button is lit as processed by the BUTTON state
    double m_buttonDownSimt;    // simt of when button was pressed
};

//----------------------------------------------------------------------------------

class RawButtonArea : public XR1Area
{
public:
    RawButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int buttonMeshGroup = -1);

    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool Redraw3D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);

protected:
    // subclass hooks this method 
    virtual void ProcessButtonAction(int event, double buttonDownSimt) = 0;

    // subclass must override this method 
    virtual bool IsLit() = 0;

    // data
    int m_buttonMeshGroup;  // 3D button mesh group, or -1 if none
    double m_buttonDownSimt;    // simt of when button was pressed
};

//----------------------------------------------------------------------------------

class TimerNumberArea : public NumberArea
{
public:
    enum class TIMEUNITS { DAYS, HOURS, MINUTES, SECONDS };
    TimerNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int sizeInChars, TIMEUNITS timeUnits, NumberArea::COLOR color = COLOR::GREEN);

protected:
    virtual bool UpdateRenderData(RENDERDATA &renderData);
    virtual bool RenderTimeValue(RENDERDATA &renderData, double time);

    // subclass must implement this method
    virtual double GetTime() = 0;

    COLOR m_color;  // font color
    double m_unitsInDay;
    TIMEUNITS m_timeUnits;
    int m_maxValue;
};

//----------------------------------------------------------------------------------

class MJDTimerNumberArea : public TimerNumberArea
{
public:
    MJDTimerNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, bool &isRunning, const int sizeInChars, TIMEUNITS timeUnits, const double &mjdStartTime);

protected:
    // returns time in DAYS
    virtual double GetTime();

    const double &m_mjdStartTime;   // -1 = RESET
    double m_lastRenderedMJD;       // full MDJ of last rendered value
    bool &m_isRunning;
};

//----------------------------------------------------------------------------------

class ElapsedTimerNumberArea : public TimerNumberArea
{
public:
    ElapsedTimerNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, bool &isRunning, const int sizeInChars, TIMEUNITS timeUnits, double &elapsedTime);

protected:
    virtual double GetTime();

    double &m_elapsedTime;  
    bool &m_isRunning;
};


//----------------------------------------------------------------------------------

class LargeBarArea : public BarArea
{
public:
    // Constructor
    LargeBarArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, int sizeX, int sizeY, int resourceID, int darkResourceID);
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual void Deactivate();

protected:
    virtual RENDERDATA GetRenderData() = 0;  // subclasses must implement this

    // state data 
    int m_resourceID;
    int m_darkResourceID;

    SURFHANDLE m_darkSurface;   // our dark surface handle
};

//----------------------------------------------------------------------------------

class LargeFuelBarArea : public LargeBarArea
{
public:
    LargeFuelBarArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, double maxFuelQty, const double *pFuelRemaining, const int resourceID,  const int darkResourceID, const double gaugeMinValue = 0);
    LargeFuelBarArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, PROPELLANT_HANDLE ph, const int resourceID, const int darkResourceID, const double gaugeMinValue = 0);

protected:
    virtual RENDERDATA GetRenderData();

    const double m_maxFuelQty;       // will be -1 if not used
    const double m_gaugeMinValue;    // minimum value on gauge
    const double *m_pFuelRemaining;  // may be null (exactly one of these two will be null)
    PROPELLANT_HANDLE m_propHandle;  // may be null (exactly one of these two will be null)
};

//----------------------------------------------------------------------------------

class LargeLOXBarArea : public LargeBarArea
{
public:
    LargeLOXBarArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int resourceID, const int darkResourceID);

protected:
    virtual RENDERDATA GetRenderData();
};

//----------------------------------------------------------------------------------

class FuelDumpButtonArea : public XR1Area
{
public:
    FuelDumpButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, bool &fuelDumpInProgress, const char *pFuelLabel);
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);
    virtual void clbkPrePostStep(const double simt, const double simdt, const double mjd);

    // invoked repeatedly when button is held down
    virtual void ProcessButtonPressed(const int event);

protected:
    bool m_isLit;
    bool m_buttonPressProcessed;
    bool &m_fuelDumpInProgress;
    double m_buttonDownSimt;
    int m_isButtonDown;
    
    char m_fuelLabel[10];   // Main, RCS, SCRAM, APU
};

//----------------------------------------------------------------------------------

class SupplyHatchToggleSwitchArea : public ToggleSwitchArea
{
public:
    SupplyHatchToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID, DoorStatus &doorStatus, const char *pHatchName, const UINT &animHandle);

protected:
    virtual bool ProcessSwitchEvent(bool switchIsOn);
    virtual bool isOn();
    
    const UINT &m_animHandle;   // animation handle for this hatch; 0 == none
    DoorStatus &m_doorStatus;
    char m_hatchName[20]; // e.g., "Fuel", "LOX", etc.; used to construct wav filename
};

//----------------------------------------------------------------------------------

class DoorMediumLEDArea : public XR1Area
{
public:
    DoorMediumLEDArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, DoorStatus &doorStatus, const bool redrawAlways = false);
    virtual void Activate();
    virtual bool Redraw2D(const int event, const SURFHANDLE surf);

protected:
    DoorStatus &m_doorStatus;
    bool m_isOn;    // true if LED currently lit
    const bool m_redrawAlways;
};

//----------------------------------------------------------------------------------

class BoolToggleSwitchArea : public ToggleSwitchArea
{
public:
    BoolToggleSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID, bool &switchState);

protected:
    virtual bool ProcessSwitchEvent(bool switchIsOn);
    virtual bool isOn();

    bool &m_switchState;
};

//----------------------------------------------------------------------------------
// abstract base class for a mass area
class MassNumberArea : public NumberArea
{
public:
    MassNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const bool isMetric);

protected:
    virtual double GetMassInKG() = 0;
    virtual bool UpdateRenderData(RENDERDATA &renderData);

    const bool m_isMetric;
};

//----------------------------------------------------------------------------------

class ShipMassNumberArea : public MassNumberArea
{
public:
    ShipMassNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const bool isMetric);
    
protected:
    virtual double GetMassInKG();
};

//----------------------------------------------------------------------------------

class AlteaAerospaceArea : public XR1Area
{
public:
    AlteaAerospaceArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID);
    
protected:
    virtual void Activate();
    virtual bool ProcessMouseEvent(const int event, const int mx, const int my);
};

//----------------------------------------------------------------------------------

class ExternalCoolingSwitchArea : public ToggleSwitchArea
{
public:
    ExternalCoolingSwitchArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int indicatorAreaID);

protected:
    virtual bool ProcessSwitchEvent(bool switchIsOn);
    virtual bool isOn();
};
