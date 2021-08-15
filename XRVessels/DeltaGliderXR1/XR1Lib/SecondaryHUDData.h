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
// SecondaryHUDData.h
//
// Defines data classes for the secondary HUD.
// ==============================================================

#pragma once

class DeltaGliderXR1;

// # of rows on the secondary HUD
#define SH_ROW_COUNT 7

//----------------------------------------------------------------------------------
// Secondary HUD fields
//----------------------------------------------------------------------------------

enum Units
{
    u_NONE = 0,   // not set
    u_met,    //      ("metric")   : km/meters/kg, etc.
    u_imp,    //      ("imperial") : ft/miles/pounds, etc.
    u_G,      //      ("gravities"): related to acceleration
    u_M,      //      ("Mach")     : related to speed)
    u_K,      //      (Kelvin)     : temperature
    u_F,      //      (Fahrenheit) : temperature
    u_C,      //      (Celsiuis)   : temperature
    u_NA,     //      "N/A"; used for fields where only one unit is valid, such as "degrees" for angles
    u_END     // marks the END of an array
};

enum FieldID 
{
    f_NONE = 0, // not set
    Alt,        //       met/imp         Altitude
    Vel,        //       met/imp/M       Velocity
    StatP,      //       met/imp         Atm Static Pressure
    DynP,       //       met/imp         Atm Dynamic Pressure
    OAT,        //       K/F/C           Outside Air Temperature
    Hdg,        //       -               Heading
    VS,         //       met/imp         Vertical Speed
    AccX,       //       G/met/imp       Acceleration along X axis
    AccY,       //       G/met/imp       Acceleration along Y axis
    AccZ,       //       G/met/imp       Acceleration along Z axis
    Mass,       //       met/imp         Ship mass
    Ecc,        //       -               Orbit Eccentricity
    Inc,        //       -               Orbit Inclination relative to the equator
    PeT,        //       -               Time to Periapsis
    ApT,        //       -               Time to Apoapsis
    PeA,        //       met/imp         Periapsis Altitude
    ApA,        //       met/imp         Apoapsis Altitude
    PeR,        //       met/imp         Periapsis Radius
    ApR,        //       met/imp         Apoapsis Radius
    Pitch,      //       -               Pitch
    Bank,       //       -               Bank
    Slope,      //       -               Glide Slope 
    Slip,       //       -               Slip Angle
    AOA,        //       -               Angle of Attack
    Long,       //       -               Longitude
    Lat,        //       -               Latitude
    LEng,       //       met/imp         Left Engine/Retro Thrust
    REng,       //       met/imp         Right Engine/Retro Thrust
    MEng,       //       met/imp         Both Main Engines' Thrust
    FHov,       //       met/imp         Forward Hover Engine Thrust
    AHov,       //       met/imp         Aft Hover Engine Thrust
    BHov,       //       met/imp         Both Hover Engines' Thrust
    LScrm,      //       met/imp         Left SCRAM Engine Thrust
    RScrm,      //       met/imp         Right SCRAM Engine Thrust
    BScrm,      //       met/imp         Both SCRAM Engines' Thrust
    rcs_1,      //       met/imp         Forward Lower RCS Thrust
    rcs_2,      //       met/imp         Aft Upper RCS Thrust
    rcs_3,      //       met/imp         Forward Upper RCS Thrust
    rcs_4,      //       met/imp         Aft Lower RCS Thrust
    rcs_5,      //       met/imp         Forward Starboard RCS Thruster
    rcs_6,      //       met/imp         Aft Port RCS Thruster
    rcs_7,      //       met/imp         Forward Port RCS Thruster
    rcs_8,      //       met/imp         Aft Starboard RCS Thruster
    rcs_9,      //       met/imp         Outboard Upper Port RCS Thrust
    rcs_10,     //       met/imp         Outboard Lower Starboard RCS Thrust
    rcs_11,     //       met/imp         Outboard Upper Starboard RCS Thrust
    rcs_12,     //       met/imp         Outboard Lower Port RCS Thrust
    rcs_13,     //       met/imp         Aft RCS Thrust (+Z axis)
    rcs_14,     //       met/imp         Forward RCS Thrust (-Z axis)
    LDtmp,      //       K/F/C           Left SCRAM Diffuser Temp
    LCtmp,      //       K/F/C           Left SCRAM Combustion Chamber Temp
    LEtmp,      //       K/F/C           Left SCRAM Exhaust Temp
    RDtmp,      //       K/F/C           Right SCRAM Diffuser Temp
    RCtmp,      //       K/F/C           Right SCRAM Combustion Chamber Temp
    REtmp,      //       K/F/C           Right SCRAM Exhaust Temp
    f_END       // marks the END of the list
};

struct SHField
{
    FieldID id;             // Alt
    Units   validUnits[3];  // up to 3 valid units for this field
    const char *label;      // display label; e.g., "Alt"  (do NOT delete this ptr; it points to a static string)

    const static SHField allFields[]; // array of all valid fields
};

// this structure is only used for parsing
struct SHParseUnit
{
    Units units;            // u_met, u_imp, etc.
    const char *parseTag;   // tag in config file for parser to check

    const static SHParseUnit allParseUnits[]; // array of all valid parse units
};

#define MAX_CELL_LABEL_LENGTH 5
#define MAX_CELL_VALUE_LENGTH 12

// contains information for a group of fields on the secondary HUD
class SecondaryHUDMode
{
public:
    // denotes a single cell on the HUD (field and value)
    struct Cell
    {
        // inline constructor 
        Cell() : pField(nullptr), units(u_NONE)
        {
            *valueStr = 0;
        }

        // NOTE: do NOT delete pField; it is const * to a static string

        const SHField *pField;  // e.g., Alt field structure   ("Altitude")
        Units units;            // e.g., u_met  (metric)
        char valueStr[MAX_CELL_VALUE_LENGTH + 1];  // "212000 ft", etc.; leave some extra padding here in case we go over somewhere
    };

    // inline constructor
    SecondaryHUDMode() : m_textColor(0), m_backgroundColor(0) { }

    bool SetCell(int row, int column, const char *pFieldName, const char *pUnits);
    bool SetCell(int row, int column, const SHField &field, const Units units);
    Cell &GetCell(int x, int y) { return m_cells[x][y]; }

    void SetTextColor(COLORREF color) { m_textColor = color; }
    COLORREF GetTextColor() const { return m_textColor; }

    void SetBackgroundColor(COLORREF color) { m_backgroundColor = color; }
    COLORREF GetBackgroundColor() const { return m_backgroundColor; }
    
private:
    Cell m_cells[SH_ROW_COUNT][2];  // 7 rows, 2 column per row
    COLORREF m_textColor;
    COLORREF m_backgroundColor;
};

