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

#include "DeltaGliderXR1.h"
#include "XR1HUD.h"

// ==============================================================

//
// Constructor
// vessel = our vessel handle
// panelCoordinates = absolute coordinates of this area on the parent instrument panel
// areaID = unique Orbiter area ID
SecondaryHUDModeButtonsArea::SecondaryHUDModeButtonsArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID) :
    XR1Area(parentPanel, panelCoordinates, areaID)
{
}

// Activate this area
void SecondaryHUDModeButtonsArea::Activate()
{
    Area::Activate();  // invoke superclass method
    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(165, 15), PANEL_REDRAW_MOUSE, PANEL_MOUSE_LBDOWN | PANEL_MOUSE_ONREPLAY, PANEL_MAP_BACKGROUND);
    m_mainSurface = CreateSurface(IDB_LIGHT1);
}

// Redraw this area
// event = Orbiter event flags
// returns: true if area redrawn, false if not
bool SecondaryHUDModeButtonsArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    int mode = GetXR1().m_secondaryHUDMode;
    if (mode > 0)
        DeltaGliderXR1::SafeBlt(surf, m_mainSurface, (mode * 29) + 6, 0, 7, 0, 7, 7);

    return true;
}

// Handle mouse events for this area
// event = Orbiter event flags
// mx, my = mouse coordinates relative to the area
// returns: true if event processed, false if not
bool SecondaryHUDModeButtonsArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated or systems offline, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard() || GetXR1().m_internalSystemsFailure)
        return false;

    if (mx % 29 < 20)     // allow for spacing between buttons
        GetXR1().EnableAndSetSecondaryHUDMode(mx / 29);  // (0...5); will play sound as well

    return true;
}

//----------------------------------------------------------------------------------

//
// Constructor
// vessel = our vessel handle
// panelCoordinates = absolute coordinates of this area on the parent instrument panel
// areaID = unique Orbiter area ID
SecondaryHUDArea::SecondaryHUDArea(InstrumentPanel& parentPanel, const COORD2 panelCoordinates, const int areaID) :
    PopupHUDArea(parentPanel, panelCoordinates, areaID, 209, 82),
    m_lastHUDMode(0), m_mainFont(0)
{
    // no need to set colors or font here; they will be set by Activate()
    m_lineSpacing = 11;     // pixels between lines
}

SecondaryHUDArea::~SecondaryHUDArea()
{
    // clean up the last font we allocated, if any
    if (m_mainFont != 0)
        DeleteObject(m_mainFont);
}

// returns TRUE if HUD is on.  NOTE: the HUD is not necessarily fully deployed!
bool SecondaryHUDArea::isOn()
{
    return (GetXR1().m_secondaryHUDMode > 0);
}

// Set HUD colors; invoked by the superclass before HUD rendering begins
void SecondaryHUDArea::SetHUDColors()
{
    // NOTE: HUD may be (turning) off here; if so, don't change the colors
    int mode = GetXR1().m_secondaryHUDMode;  // mode 1-5
    if (mode > 0)
    {
        const XR1ConfigFileParser& config = *GetXR1().GetXR1Config();
        const SecondaryHUDMode secondaryHUD = config.SecondaryHUD[mode - 1];   // 0 < mode < 5

        // set the HUD colors 
        // there is no warning color, at least for now
        const COLORREF backgroundColor = secondaryHUD.GetBackgroundColor();
        SetColor(secondaryHUD.GetTextColor());    // normal color
        SetBackgroundColor(backgroundColor);

        // if the HUD mode has changed, recreate the font
        // We must do this here because we want an UNALIASED font if transparent
        // NOTE: do not use ANTIALIASED_QUALITY instead of '0' for the second parameter!  It looks better under Vista to leave it at 0 for some reason.
        if (mode != m_lastHUDMode)
        {
            const DWORD antialiasFlag = ((backgroundColor == 0xFFFFFF) ? NONANTIALIASED_QUALITY : 0);

            // release old font 
            if (m_mainFont != 0)
                DeleteObject(m_mainFont);

            // create new font
            m_mainFont = CreateFont(14, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, antialiasFlag, 0, "Arial");
        }
    }
}

// Render the contents of the HUD
// NOTE: the subclass MUST draw text from the supplied topY coordinate (plus some border gap space)
// The X coordinate is zero @ the border
// Returns: true if HUD was redrawn, false if not
bool SecondaryHUDArea::DrawHUD(const int event, const int topY, HDC hDC, COLORREF colorRef, bool forceRender)
{
    // NOTE: HUD may be off here if we are turning off!
    int mode = GetXR1().m_secondaryHUDMode;  // mode 1-5
    if (mode == 0)  // HUD off?
        mode = m_lastHUDMode;   // remember last active HUD mode
    else    // HUD is on
        m_lastHUDMode = mode;   // remember this

    const XR1ConfigFileParser& config = *GetXR1().GetXR1Config();
    SecondaryHUDMode secondaryHUD = config.SecondaryHUD[mode - 1];   // 0 < mode < 5

    // set the font
    HFONT prevFont = (HFONT)SelectObject(hDC, m_mainFont);   // save previous font and select new font

    // set the text fg and bg colors
    const COLORREF bgColor = secondaryHUD.GetBackgroundColor();
    ::SetTextColor(hDC, secondaryHUD.GetTextColor());
    SetBkColor(hDC, bgColor);

    // NOTE: area was registered with PANEL_MAP_BACKGROUND, so we don't need to always repaint it
    // fill the background area if not transparent; this is to make the background solid between letters
    if (bgColor != CWHITE)
    {
        RECT r = { 0, m_topYCoordinate, m_width, m_height };
        FillRect(hDC, &r, m_hBackgroundBrush);
    }

    // set the background mode
    SetBkMode(hDC, ((GetBackgroundColor() == CWHITE) ? TRANSPARENT : OPAQUE));

    // render each cell on the HUD
    // NOTE: must render from the BOTTOM-UP so that the descenders render on each row
    for (int row = SH_ROW_COUNT - 1; row >= 0; row--)
    {
        RenderCell(hDC, secondaryHUD, row, 0, topY);   // left side
        RenderCell(hDC, secondaryHUD, row, 1, topY);   // right side
    }

    SelectObject(hDC, prevFont);   // restore previously selected font

    // We always redraw here because 1) it would be almost impossible to accurately track what changes, and
    // 2) we are only invoked at a fixed interval anyway.
    return true;
}

// Render a single cell on the secondary HUD
// row and column are NOT validated here; they were validated before
void SecondaryHUDArea::RenderCell(HDC hDC, SecondaryHUDMode& secondaryHUD, const int row, const int column, const int topY)
{
    SecondaryHUDMode::Cell& cell = secondaryHUD.GetCell(row, column);
    if (cell.pField == nullptr)
        return;     // cell is empty!

    // Populate the value and valueText in this cell from our parent vessel
    PopulateCell(cell);

    const int xOffset = 34;             // # columns from left to render ":" in "Alt:"; splits each column between label and value
    const int xCenter = m_width / 2;    // horizontal center of HUD

    // Render the label; e.g., "Altitude:"
    SetTextAlign(hDC, TA_RIGHT);
    int x = ((column == 0) ? xOffset : (xCenter + xOffset));
    int y = topY + 2 + (row * m_lineSpacing);    // must render from current top of HUD, since it may be scrolling; also allow some spacing from the HUD top

    char temp[MAX_CELL_LABEL_LENGTH + 2];       // allow room for ":"
    sprintf(temp, "%s:", cell.pField->label);  // "Alt:"
    TextOut(hDC, x, y, temp, static_cast<int>(strlen(temp)));

    // Render the cell value
    SetTextAlign(hDC, TA_LEFT);
    x += 4;     // spacing between ":" and value
    const char* pStr = cell.valueStr;
    TextOut(hDC, x, y, pStr, static_cast<int>(strlen(pStr)));   // "102329 ft"
}

// Populate value and valueStr in the supplied cell
void SecondaryHUDArea::PopulateCell(SecondaryHUDMode::Cell& cell)
{
    const FieldID fieldID = cell.pField->id;
    const Units units = cell.units;
    double value = 0;   // reused below
    char valueStr[128];     // be sure that we never overrun the value buffer

    switch (fieldID)
    {
    case FieldID::Alt:
        value = GetXR1().GetAltitude(ALTMODE_GROUND); // in meters
        if (units == Units::u_met) // metric
        {
            // altitude will never be negative here
            if (value >= 1e7)   // >= 10 million meters (10,000 km)?
                sprintf(valueStr, "%.2lf mm", (value / 1e6));
            else if (value >= 3e4)   // >= 30 km?
                sprintf(valueStr, "%.3lf km", (value / 1e3));
            else
                sprintf(valueStr, "%.2lf m", value);
        }
        else    // imperial
        {
            value = MetersToFeet(value);
            // handle large mile distances here
            const double distInMiles = (value / 5280);
            if (fabs(distInMiles) >= 1e6)   // >= 1 million miles?
                sprintf(valueStr, "%.3lf mmi", (distInMiles / 1e6));  // do not clip
            else if (value > 407e3)  // > 407000 ft?
                sprintf(valueStr, "%.2lf mi", distInMiles);
            else
                sprintf(valueStr, "%.2lf ft", value);
        }
        break;

    case FieldID::Vel:
        value = GetXR1().GetGroundspeed();
        // velocity will never be negative 
        if (units == Units::u_met) // metric
        {
            sprintf(valueStr, "%.1lf m/s", value);
        }
        else if (units == Units::u_imp)   // imperial
        {
            value = MpsToMph(value);
            sprintf(valueStr, "%.1lf mph", value);
        }
        else if (units == Units::u_M)
        {
            value = GetXR1().GetMachNumber();
            sprintf(valueStr, "%.3lf Mach", value);  // cap @ 11 characters here b/c of clipping issue with "mach"
        }
        break;

    case FieldID::StatP:
    case FieldID::DynP:
        // in pascals
        value = ((fieldID == FieldID::StatP) ? GetXR1().GetAtmPressure() : GetXR1().GetDynPressure());
        if (units == Units::u_met) // metric
        {
            sprintf(valueStr, "%.4lf kPa", (value / 1000));
        }
        else // imperial
        {
            value = PaToPsi(value);
            sprintf(valueStr, "%.4lf psi", value);
        }
        break;

    case FieldID::OAT:
        value = GetXR1().GetExternalTemperature();   // Kelvin
        if (units == Units::u_K)
        {
            sprintf(valueStr, "%.4lf °K", value);
        }
        else if (units == Units::u_C)
        {
            value = KelvinToCelsius(value);
            sprintf(valueStr, "%.4lf °C", value);
        }
        else    // Fahrenheit
        {
            value = KelvinToFahrenheit(value);
            sprintf(valueStr, "%.4lf °F", value);
        }
        break;

    case FieldID::Hdg:
    {
        BOOL stat = oapiGetHeading(GetVessel().GetHandle(), &value);
        if (stat == FALSE)
            sprintf(valueStr, "---");
        else
        {
            sprintf(valueStr, "%.3lf°", (value * DEG));
        }
    }
    break;

    case FieldID::VS:
    {
        VECTOR3 v;
        GetXR1().GetAirspeedVector(FRAME_HORIZON, v);
        value = (GetXR1().GroundContact() ? 0 : v.y);      // in m/s
        if (units == Units::u_met) // metric
        {
            sprintf(valueStr, "%+.2lf m/s", value);
        }
        else // imperial
        {
            value = MetersToFeet(value);    // feet per second
            sprintf(valueStr, "%+.2lf fps", value);
        }
    }
    break;

    case FieldID::AccX:
    case FieldID::AccY:
    case FieldID::AccZ:
    {
        const VECTOR3& A = GetXR1().m_acceleration;
        if (fieldID == FieldID::AccX)
            value = A.x;
        else if (fieldID == FieldID::AccY)
            value = A.y;
        else
            value = A.z;

        if (units == Units::u_met)  // metric
        {
            sprintf(valueStr, "%.4lf m/s²", value);
        }
        else if (units == Units::u_imp)    // imperial
        {
            value = MetersToFeet(value);
            sprintf(valueStr, "%.4lf fps²", value);
        }
        else  // G
        {
            value = Mps2ToG(value);
            sprintf(valueStr, "%.6lf G", value);
        }
    }
    break;

    case FieldID::Mass:
        value = GetXR1().GetMass(); // in kg
        const char* pFormatStr;
        if (units == Units::u_met) // metric
        {
            if (value > 999999.9)
                pFormatStr = "%.1lf kg";
            else if (value > 99999.9)
                pFormatStr = "%.2lf kg";
            else
                pFormatStr = "%.3lf kg";

            sprintf(valueStr, "%.3lf kg", value);
        }
        else    // imperial
        {
            value = KgToPounds(value);

            if (value > 999999.9)
                pFormatStr = "%.1lf lb";
            else if (value > 99999.9)
                pFormatStr = "%.2lf lb";
            else
                pFormatStr = "%.3lf lb";

            sprintf(valueStr, pFormatStr, value);
        }
        break;

    case FieldID::Ecc:
    {
        ELEMENTS e;
        GetVessel().GetElements(nullptr, e, nullptr, 0, FRAME_EQU);  // this is only expensive on the first call to it in this frame
        value = e.e;
        sprintf(valueStr, "%.5lf", value);
    }
    break;

    case FieldID::Inc:
    {
        ELEMENTS e;
        GetVessel().GetElements(nullptr, e, nullptr, 0, FRAME_EQU);
        value = e.i * DEG;  // in degrees
        sprintf(valueStr, "%.4lf°", value);  // reduce to 11 chars for slight clipping issue
    }
    break;

    case FieldID::PeT:
    case FieldID::ApT:
    {
        ELEMENTS e;
        ORBITPARAM prm;
        GetVessel().GetElements(nullptr, e, &prm, 0, FRAME_EQU);
        value = ((fieldID == FieldID::PeT) ? prm.PeT : prm.ApT);

        // if value < 0, it means that it is N/A; i.e., we are not orbiting the object
        if (value <= 0)
        {
            sprintf(valueStr, "N/A");
            break;
        }

        if (fabs(value) >= 1e7)  // >= 10,000,000 seconds?
            sprintf(valueStr, "%.4lf M", (value / 1e6));
        else if (fabs(value) >= 1e4)  // >= 10,000 seconds?
            sprintf(valueStr, "%.4lf K", (value / 1e3));
        else
            sprintf(valueStr, "%.2lf", value);
    }
    break;

    case FieldID::PeR:
    case FieldID::ApR:
    case FieldID::PeA:
    case FieldID::ApA:
    {
        // These values operate on the primary G body at the moment
        ELEMENTS e;
        ORBITPARAM prm;
        GetVessel().GetElements(nullptr, e, &prm, 0, FRAME_EQU);
        value = (((fieldID == FieldID::PeR) || (fieldID == FieldID::PeA)) ? prm.PeD : prm.ApD);  // dist from body center in meters

        // if value <= 0, it means that it is N/A; i.e., we are not orbiting the object
        if (value <= 0)
        {
            sprintf(valueStr, "N/A");
            break;
        }

        // if we are displaying the ALTITUDE, we need to adjust for that
        if ((fieldID == FieldID::PeA) || (fieldID == FieldID::ApA))
        {
            const OBJHANDLE gRef = GetVessel().GetGravityRef();  // body we are orbiting
            double radius = oapiGetSize(gRef);  // radius of primary G body
            value -= radius;        // altitude in meters
        }

        // we have the distance in meters; display it
        if (units == Units::u_met)     // metric
        {
            if (fabs(value) >= 1e9)
                sprintf(valueStr, "%.2lf gm", (value / 1e9));
            else if (fabs(value) >= 1e7)   // >= 10,000 km?
                sprintf(valueStr, "%.2lf mm", (value / 1e6));
            else if (fabs(value) >= 1e3)
                sprintf(valueStr, "%.2lf km", (value / 1e3));
            else
                sprintf(valueStr, "%.2lf m", value);
        }
        else   // imperial
        {
            // convert to feet
            value = MetersToFeet(value);

            // handle large mile distances here
            const double distInMiles = (value / 5280);
            if (fabs(distInMiles) >= 1e9)   // >= 1 billion miles?
                sprintf(valueStr, "%.3lf gmi", (distInMiles / 1e9));  // do not clip
            else if (fabs(distInMiles) >= 1e6)   // >= 1 million miles?
                sprintf(valueStr, "%.3lf mmi", (distInMiles / 1e6));  // do not clip
            else if (fabs(value) >= 1e5)  // >= 100,000 feet?
                sprintf(valueStr, "%.2lf mi", distInMiles);
            else
                sprintf(valueStr, "%.2lf ft", value);
        }
    }
    break;

    case FieldID::Pitch:
    case FieldID::Bank:
    case FieldID::Slope:
    case FieldID::Slip:
    case FieldID::AOA:
        if (fieldID == FieldID::Pitch)
            value = GetVessel().GetPitch();
        else if (fieldID == FieldID::Bank)
            value = GetVessel().GetBank();
        else if (fieldID == FieldID::Slope)
            value = GetXR1().m_slope;
        else if (fieldID == FieldID::Slip)
            value = GetVessel().GetSlipAngle();
        else
            value = GetVessel().GetAOA();

        value *= DEG;   // convert to degrees
        sprintf(valueStr, "%+.3lf°", value);
        break;

    case FieldID::Long:
    case FieldID::Lat:
    {
        double longitude, latitude, radius;
        OBJHANDLE hObj = GetVessel().GetEquPos(longitude, latitude, radius);
        if (hObj == nullptr)
            sprintf(valueStr, "-----");     // no data available
        else
        {
            double pos = ((fieldID == FieldID::Long) ? longitude : latitude) * DEG;
            char dir;
            if (pos < 0)
                dir = ((fieldID == FieldID::Long) ? 'W' : 'S');
            else
                dir = ((fieldID == FieldID::Long) ? 'E' : 'N');

            sprintf(valueStr, "%.5lf° %c", fabs(pos), dir);
        }
    }
    break;

    case FieldID::LEng:
    case FieldID::REng:
    case FieldID::MEng:
    case FieldID::FHov:
    case FieldID::AHov:
    case FieldID::BHov:
    case FieldID::LScrm:
    case FieldID::RScrm:
    case FieldID::BScrm:
    case FieldID::rcs_1:
    case FieldID::rcs_2:
    case FieldID::rcs_3:
    case FieldID::rcs_4:
    case FieldID::rcs_5:
    case FieldID::rcs_6:
    case FieldID::rcs_7:
    case FieldID::rcs_8:
    case FieldID::rcs_9:
    case FieldID::rcs_10:
    case FieldID::rcs_11:
    case FieldID::rcs_12:
    case FieldID::rcs_13:
    case FieldID::rcs_14:
    {
#define GET_THRUST(handle) (GetXR1().GetThrusterLevel(GetXR1().handle) * GetXR1().GetThrusterMax(GetXR1().handle))
        if (fieldID == FieldID::LEng)
        {
            // test retro FIRST so we don't show "-0.00.." on the HUD
            value = -GET_THRUST(th_retro[0]);  // show as negative for retro thrust
            if (value == 0)
                value = GET_THRUST(th_main[0]);
        }
        else if (fieldID == FieldID::REng)
        {
            value = -GET_THRUST(th_retro[1]);  // retro
            if (value == 0)
                value = GET_THRUST(th_main[1]);
        }
        else if (fieldID == FieldID::MEng)   //  both main engines
        {
            value = -GET_THRUST(th_retro[0]);   // retro
            if (value == 0)
                value = GET_THRUST(th_main[0]);

            double e1 = -GET_THRUST(th_retro[1]);
            if (e1 == 0)
                e1 = GET_THRUST(th_main[1]);

            value += e1;
        }
        else if (fieldID == FieldID::FHov)
            value = GET_THRUST(th_hover[0]);
        else if (fieldID == FieldID::AHov)
            value = GET_THRUST(th_hover[1]);
        else if (fieldID == FieldID::BHov)
            value = GET_THRUST(th_hover[0]) + GET_THRUST(th_hover[1]);
        else if (fieldID == FieldID::LScrm)
            value = GetXR1().ramjet->GetMostRecentThrust(0);
        else if (fieldID == FieldID::RScrm)
            value = GetXR1().ramjet->GetMostRecentThrust(1);
        else if (fieldID == FieldID::BScrm)
            value = GetXR1().ramjet->GetMostRecentThrust(0) + GetXR1().ramjet->GetMostRecentThrust(1);
        else    // it's an RCS jet
            value = GET_THRUST(th_rcs[static_cast<int>(fieldID) - static_cast<int>(FieldID::rcs_1)]);

        if (units == Units::u_imp)
            value = NewtonsToPounds(value);

        if (value >= 1000)
        {
            if (units == Units::u_met)
                sprintf(valueStr, "%.3lf kN", value / 1000);
            else  // imperial
                sprintf(valueStr, "%.3lf kLb", value / 1000);
        }
        else    // RCS thrust is very small
        {
            if (units == Units::u_met)
                sprintf(valueStr, "%.3lf N", value);
            else  // imperial
                sprintf(valueStr, "%.3lf lb", NewtonsToPounds(value));
        }
    }
    break;

    case FieldID::LDtmp:
    case FieldID::LCtmp:
    case FieldID::LEtmp:
    case FieldID::RDtmp:
    case FieldID::RCtmp:
    case FieldID::REtmp:
    {
        if (fieldID == FieldID::LDtmp)
            value = GetXR1().ramjet->Temp(0, 0);
        else if (fieldID == FieldID::LCtmp)
            value = GetXR1().ramjet->Temp(0, 1);
        else if (fieldID == FieldID::LEtmp)
            value = GetXR1().ramjet->Temp(0, 2);
        else if (fieldID == FieldID::RDtmp)
            value = GetXR1().ramjet->Temp(1, 0);
        else if (fieldID == FieldID::RCtmp)
            value = GetXR1().ramjet->Temp(1, 1);
        else if (fieldID == FieldID::REtmp)
            value = GetXR1().ramjet->Temp(1, 2);

        if (units == Units::u_K)
        {
            sprintf(valueStr, "%.3lf °K", value);
        }
        else if (units == Units::u_C)
        {
            value = KelvinToCelsius(value);
            sprintf(valueStr, "%.3lf °C", value);
        }
        else    // Fahrenheit
        {
            value = KelvinToFahrenheit(value);
            sprintf(valueStr, "%.3lf °F", value);
        }
    }
    break;

    default:        // should never happen!
        sprintf(valueStr, "??????");        // let the user know something is wrong
        break;
    }

    // now copy the rendered string into the official string, TRUNCATING it if necessary to prevent buffer overruns and a CTD!
    strncpy(cell.valueStr, valueStr, MAX_CELL_VALUE_LENGTH);
    cell.valueStr[MAX_CELL_VALUE_LENGTH] = 0;   // terminate
}
