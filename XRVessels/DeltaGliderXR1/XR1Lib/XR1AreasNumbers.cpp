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

// must be included BEFORE XR1Areas.h
#include "DeltaGliderXR1.h"
#include "XR1Areas.h"

//-------------------------------------------------------------------------

// sizeInChars = # of characters in area to be painted
// e.g., "232.3": 4*7+3 = 31 wide, 9 high : sizeInChars = 4, hasDecimal=true
// fontResourceID: e.g., IDB_FONT2 (green version)
NumberArea::NumberArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID, int sizeInChars, bool hasDecimal) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_sizeInChars(sizeInChars), m_hasDecimal(hasDecimal), m_font2Yellow(0), m_font2Red(0), m_font2Blue(0), m_font2White(0)
{
    m_pRenderData = new RENDERDATA(sizeInChars + (hasDecimal ? 1 : 0));
}

NumberArea::~NumberArea()
{
    delete m_pRenderData;
}

void NumberArea::Activate()
{
    Area::Activate();  // invoke superclass method
    int sizeX = (m_sizeInChars * 7) + (m_hasDecimal ? 3 : 0);

    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(sizeX, 9), PANEL_REDRAW_ALWAYS, PANEL_MOUSE_IGNORE, PANEL_MAP_BGONREQUEST);

    m_mainSurface = CreateSurface(IDB_FONT2);  // our special numeric font (green)
    m_font2Yellow = CreateSurface(IDB_FONT2_YELLOW);
    m_font2Red = CreateSurface(IDB_FONT2_RED);
    m_font2Blue = CreateSurface(IDB_FONT2_BLUE);
    m_font2White = CreateSurface(IDB_FONT2_WHITE);

    // force a repaint and defult to normal color
    m_pRenderData->Reset();
}

void NumberArea::Deactivate()
{
    DestroySurface(&m_font2Yellow);
    DestroySurface(&m_font2Red);
    DestroySurface(&m_font2Blue);
    DestroySurface(&m_font2White);
    XR1Area::Deactivate();  // let superclass clean up
}

bool NumberArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    // invoke subclass method to update the render data
    bool redraw = UpdateRenderData(*m_pRenderData);

    if (redraw)   // has value changed?
    {
        // NOTE: no need to render background here; we will overwrite the entire area

        // each char is 7x9, except for '.' (last in the bitmap) which is 5x9
        // Order is: 0 1 2 3 4 5 6 7 8 9 ' ' .

        char* num = m_pRenderData->pStrToRender;
        int x = 0;  // X coordinate of next character render
        for (; *num; num++)
        {
            int srcX;           // X coord into font2.bmp
            int charWidth = 7;  // assume normal char
            char c = *num;
            switch (c)
            {
            case '-':
                srcX = 70;
                break;

            case ' ':   // blank space
                srcX = 77;
                break;

            case '.':   // special narrow '.' char
                srcX = 84;
                charWidth = 3;
                break;

            default:    // 0-9 digit
                srcX = (c - '0') * 7; // each digit is 7 pixels wide with spacing
            }

            SURFHANDLE srcSurface;
            switch (m_pRenderData->color)
            {
            case COLOR::RED:
                srcSurface = m_font2Red;
                break;

            case COLOR::YELLOW:
                srcSurface = m_font2Yellow;
                break;

            case COLOR::BLUE:
                srcSurface = m_font2Blue;
                break;

            case COLOR::WHITE:
                srcSurface = m_font2White;
                break;

            default:    // GREEN
                srcSurface = m_mainSurface;
                break;
            }

            // render separating spaces as well just in case anything underneath (since the font can vary in width now)
            //                                  srcX,srcY,width,   height
            DeltaGliderXR1::SafeBlt(surf, srcSurface, x, 0, srcX, 0, charWidth, 9);
            x += charWidth; // set up for next character
        }
    }

    return redraw;
}



//-------------------------------------------------------------------------

ThrustNumberArea::ThrustNumberArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID) :
    NumberArea(parentPanel, panelCoordinates, areaID, 6, true)   // 6 chars plus decimal
{
}

bool ThrustNumberArea::UpdateRenderData(RENDERDATA& renderData)
{
    bool redraw = false;
    const bool forceRedraw = renderData.forceRedraw;

    double thrust = GetThrust();  // retrieve from subclass (in kN)

    // no need to round here; sprintf will do it for us

    // check whether the value has changed since the last render
    if (forceRedraw || (thrust != renderData.value))
    {
        // Value has changed -- let's redo the string and see if that is different as well.
        // The goal here is to be as efficient as possible and only re-render when we absolutely have to.
        char pTemp[10];
        // ensure that value is in range
        if (thrust > 999999)
            thrust = 999999;   // trim to 6 digits
        else if (thrust < 0)
            thrust = 0;       // thrust cannot be negative!

        // note: pFormatStr must evaluate to exactly 7 characters for each case
        const char* pFormatStr;
        if (thrust > 99999.9)
            pFormatStr = "%6.0f.";
        else if (thrust > 9999.99)
            pFormatStr = "%5.1f";
        else if (thrust > 999.999)
            pFormatStr = "%4.2f";
        else if (thrust > 99.9999)
            pFormatStr = "%3.3f";
        else if (thrust > 9.99999)
            pFormatStr = "%2.4f";
        else  // <= 9.99999
            pFormatStr = "%1.5f";

        sprintf(pTemp, pFormatStr, thrust);
        if (forceRedraw || (strcmp(pTemp, renderData.pStrToRender) != 0))
        {
            // text has changed; signal the base class to render it
            renderData.value = thrust;   // remember for next time
            strcpy(renderData.pStrToRender, pTemp);
            redraw = true;
            renderData.forceRedraw = false;  // clear reset request
        }
    }

    return redraw;
}

//-------------------------------------------------------------------------

AccNumberArea::AccNumberArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID, const AXIS axis) :
    NumberArea(parentPanel, panelCoordinates, areaID, 6, true),   // 6 chars plus decimal
    m_axis(axis)
{
}
bool AccNumberArea::UpdateRenderData(RENDERDATA& renderData)
{
    bool redraw = false;
    const bool forceRedraw = renderData.forceRedraw;

    // use shared acceleration values set once per frame by ComputeAccPostStep object
    const VECTOR3& A = GetXR1().m_acceleration;
    double acc;
    switch (m_axis)
    {
    case AXIS::X:
        acc = A.x;
        break;

    case AXIS::Y:
        acc = A.y;
        break;

    case AXIS::Z:
        acc = A.z;
        break;
    }

    // round acc to nearest 1/1000th
    acc = (static_cast<double>(static_cast<int>((acc + 0.0005) * 1000))) / 1000;

    // check whether the value has changed since the last render
    if (forceRedraw || (acc != renderData.value))
    {
        // Value has changed -- let's redo the string and see if that is different as well.
        // The goal here is to be as efficient as possible and only re-render when we absolutely have to.
        char pTemp[10];   // add 2 extra chars in case we go way high on one frame for some reason
        // ensure that value is in range
        if (acc > 99.999)
            acc = 99.999;   // trim to 2 leading digits + possible minus sign
        else if (acc < -99.999)
            acc = -99.999;
        sprintf(pTemp, "%7.3f", acc);
        if (forceRedraw || (strcmp(pTemp, renderData.pStrToRender) != 0))
        {
            // text has changed; signal the base class to render it
            renderData.value = acc;   // remember for next time
            strcpy(renderData.pStrToRender, pTemp);
            redraw = true;
            renderData.forceRedraw = false;  // clear reset request
        }
    }

    return redraw;
}

//-------------------------------------------------------------------------

// base class for all timer number areas
TimerNumberArea::TimerNumberArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID, const int sizeInChars, TIMEUNITS timeUnits, NumberArea::COLOR color) :
    NumberArea(parentPanel, panelCoordinates, areaID, sizeInChars, false),   // no decimal
    m_timeUnits(timeUnits), m_color(color)
{
    switch (timeUnits)
    {
    case TIMEUNITS::DAYS:
        m_unitsInDay = 1;
        m_maxValue = 9999;
        break;

    case TIMEUNITS::HOURS:
        m_unitsInDay = 24;
        m_maxValue = 23;
        break;

    case TIMEUNITS::MINUTES:
        m_unitsInDay = 24.0 * 60;
        m_maxValue = 59;
        break;

    case TIMEUNITS::SECONDS:
        m_unitsInDay = 24.0 * 60 * 60;
        m_maxValue = 59;
        break;

    default:        // should never happen!
        m_unitsInDay = 1e-6;     // update each timestep so we see something is wrong
        break;
    }
}

bool TimerNumberArea::UpdateRenderData(RENDERDATA& renderData)
{
    bool redraw = false;
    const bool forceRedraw = renderData.forceRedraw;

    // invoke the subclass to return elapsed time in DAYS
    double time = GetTime();

    // render the string via a base class method
    redraw = RenderTimeValue(renderData, time);

    // render in the requested color
    renderData.color = m_color;

    return redraw;
}

// elapsedTime is in DAYS here
bool TimerNumberArea::RenderTimeValue(RENDERDATA& renderData, double time)
{
    bool redraw = false;
    const bool forceRedraw = renderData.forceRedraw;

    int value;
    if (m_timeUnits == TIMEUNITS::DAYS)
    {
        value = static_cast<int>(time);
    }
    else    // hours, minutes, or seconds
    {
        // compute the elapsed UNIT, rounded DOWN
        double elapsedUnitsTotal = time * m_unitsInDay;
        double elapsedUnitsInCurrentDay = fmod(elapsedUnitsTotal, m_unitsInDay);  // 0....(m_unitsInDay-1)
        value = static_cast<int>(elapsedUnitsInCurrentDay) % (m_maxValue + 1);   // 0...m_maxValue
    }

    // check whether the value has changed since the last render
    if (forceRedraw || (value != renderData.value))
    {
        char temp[10];

        // Value has changed -- since we are an integer value, the string will always be different as well

        // ensure that value is in range
        if (value > m_maxValue)
            value = m_maxValue;
        else if (value < 0)     // sanity check
            value = 0;

        if (m_sizeInChars == 4)     // days?
            sprintf(temp, "%4d", value);
        else        // hours, minutes, or seconds
            sprintf(temp, "%02d", value);

        // signal the base class to render the text
        renderData.value = value;   // remember for next time
        strcpy(renderData.pStrToRender, temp);
        redraw = true;
        renderData.forceRedraw = false;  // clear reset request
    }

    return redraw;
}

//-------------------------------------------------------------------------

// mdjStartTime = MJD that timer started running
// NOTE: if mjdStartTime is set to -1 while timer is running, timer is STOPPED and RESET here automatically; client classes need only set mdjStartTime=-1.
MJDTimerNumberArea::MJDTimerNumberArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID, bool& isRunning, const int sizeInChars, TIMEUNITS timeUnits, const double& mjdStartTime) :
    TimerNumberArea(parentPanel, panelCoordinates, areaID, sizeInChars, timeUnits),
    m_mjdStartTime(mjdStartTime), m_lastRenderedMJD(-1), m_isRunning(isRunning)
{
}

// returns: elapsed time in days
double MJDTimerNumberArea::GetTime()
{
    // check whether timer is reset
    if (m_mjdStartTime < 0)
    {
        m_isRunning = false;     // stop timer if still running
        m_lastRenderedMJD = -1;  // force retVal to be 0.0 below
    }
    else if (m_isRunning)  // update MJD to render if timer is running; otherwise it is paused or stopped 
    {
        // update MJD time to be rendered this frame
        m_lastRenderedMJD = oapiGetSimMJD();
    }

    // compute the elapsed time since timer start
    double retVal = max(m_lastRenderedMJD - m_mjdStartTime, 0.0);   // if negative delta, set to 0.0

    return retVal;
}

//-------------------------------------------------------------------------

ElapsedTimerNumberArea::ElapsedTimerNumberArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID, bool& isRunning, const int sizeInChars, TIMEUNITS timeUnits, double& elapsedTime) :
    TimerNumberArea(parentPanel, panelCoordinates, areaID, sizeInChars, timeUnits),
    m_elapsedTime(elapsedTime), m_isRunning(isRunning)
{
}

// returns: elapsed time in days
double ElapsedTimerNumberArea::GetTime()
{
    double retVal;

    // check whether timer is reset
    if (m_elapsedTime < 0)
    {
        m_isRunning = false;  // stop timer if still running
        retVal = 0;
    }
    else // timer running normally
    {
        retVal = m_elapsedTime;
    }

    return retVal;
}

//-------------------------------------------------------------------------

MassNumberArea::MassNumberArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID, const bool isMetric) :
    NumberArea(parentPanel, panelCoordinates, areaID, 8, true),   // 8 chars plus decimal
    m_isMetric(isMetric)
{
}

bool MassNumberArea::UpdateRenderData(RENDERDATA& renderData)
{
    bool redraw = false;
    const bool forceRedraw = renderData.forceRedraw;

    // invoke the subclass to retrieve the mass value in KG
    double mass = GetMassInKG();

    if (m_isMetric == false)
        mass = KgToPounds(mass);

    // do not round value

    // check whether the value has changed since the last render
    if (forceRedraw || (mass != renderData.value))
    {
        // Value has changed -- let's redo the string and see if that is different as well.
        // The goal here is to be as efficient as possible and only re-render when we absolutely have to.
        char pTemp[15];

        if (mass > 99999999)
            mass = 99999999;
        else if (mass < 0)      // sanity-check
            mass = 0;

        // Note: pFormatString must be exactly nine characters in length, with exactly one decimal.
        char* pFormatString;
        if (mass > 9999999.9)
            pFormatString = "%8.0lf.";  // eight because of "." appended = nine total
        else if (mass > 999999.9)
            pFormatString = "%9.1lf";   // includes the "."
        else if (mass > 99999.99)
            pFormatString = "%9.2lf";
        else
            pFormatString = "%9.3lf";

        sprintf(pTemp, pFormatString, mass);
        if (forceRedraw || (strcmp(pTemp, renderData.pStrToRender) != 0))
        {
            // text has changed; signal the base class to render it
            renderData.value = mass;   // remember for next time
            strcpy(renderData.pStrToRender, pTemp);
            redraw = true;
            renderData.forceRedraw = false;  // clear reset request
        }
    }

    // always render in the default green
    return redraw;
}

//-------------------------------------------------------------------------

ShipMassNumberArea::ShipMassNumberArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID, const bool isMetric) :
    MassNumberArea(parentPanel, panelCoordinates, areaID, isMetric)
{
}

double ShipMassNumberArea::GetMassInKG()
{
    return GetVessel().GetMass();
}


