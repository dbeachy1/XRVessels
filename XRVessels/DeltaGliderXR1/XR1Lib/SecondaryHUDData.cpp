/**
  XR Vessel add-ons for OpenOrbiter Space Flight Simulator
  Copyright (C) 2006-2021 Douglas Beachy

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

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
// SecondaryHUDData.cpp
//
// Defines data classes to the secondary HUD.
// ==============================================================

#include "DeltaGliderXR1.h"
#include "SecondaryHUDData.h"

// define all valid Secondary HUD fields
const SHField SHField::allFields[] = 
{ 
    { Alt,      { u_met, u_imp },        "Alt" },
    { Vel,      { u_met, u_imp, u_M},    "Vel" },
    { StatP,    { u_met, u_imp },        "StatP" },
    { DynP,     { u_met, u_imp },        "DynP" },
    { OAT,      { u_K, u_F, u_C },       "OAT" },
    { Hdg,      { u_NA },                "Hdg" },
    { VS,       { u_met, u_imp },        "v/s" },
    { AccX,     { u_G, u_met, u_imp },   "AccX" },
    { AccY,     { u_G, u_met, u_imp },   "AccY" },
    { AccZ,     { u_G, u_met, u_imp },   "AccZ" },
    { Mass,     { u_met, u_imp },        "Mass" },
    { Ecc,      { u_NA },                "Ecc" },
    { Inc,      { u_NA },                "Inc" },
    { PeT,      { u_NA },                "PeT" },
    { ApT,      { u_NA },                "ApT" },
    { PeA,      { u_met, u_imp },        "PeA" },
    { ApA,      { u_met, u_imp },        "ApA" },
    { PeR,      { u_met, u_imp },        "PeR" },
    { ApR,      { u_met, u_imp },        "ApR" },
    { Pitch,    { u_NA },                "Pitch" },
    { Bank,     { u_NA },                "Bank" },
    { Slope,    { u_NA },                "Slope" },
    { Slip,     { u_NA },                "Slip" },
    { AOA,      { u_NA },                "AOA" },
    { Long,     { u_NA },                "Long" },
    { Lat,      { u_NA },                "Lat" },
    { LEng,     { u_met, u_imp },        "LEng" },
    { REng,     { u_met, u_imp },        "REng" },
    { MEng,     { u_met, u_imp },        "MEng" },
    { FHov,     { u_met, u_imp },        "FHov" },
    { AHov,     { u_met, u_imp },        "AHov" },
    { BHov,     { u_met, u_imp },        "BHov" },
    { LScrm,    { u_met, u_imp },        "LScrm" },
    { RScrm,    { u_met, u_imp },        "RScrm" },
    { BScrm,    { u_met, u_imp },        "BScrm" },
    { rcs_1,    { u_met, u_imp },        "rcs1" },
    { rcs_2,    { u_met, u_imp },        "rcs2" },
    { rcs_3,    { u_met, u_imp },        "rcs3" },
    { rcs_4,    { u_met, u_imp },        "rcs4" },
    { rcs_5,    { u_met, u_imp },        "rcs5" },
    { rcs_6,    { u_met, u_imp },        "rcs6" },
    { rcs_7,    { u_met, u_imp },        "rcs7" },
    { rcs_8,    { u_met, u_imp },        "rcs8" },
    { rcs_9,    { u_met, u_imp },        "rcs9" },
    { rcs_10,   { u_met, u_imp },        "rcs10" },
    { rcs_11,   { u_met, u_imp },        "rcs11" },
    { rcs_12,   { u_met, u_imp },        "rcs12" },
    { rcs_13,   { u_met, u_imp },        "rcs13" },
    { rcs_14,   { u_met, u_imp },        "rcs14" },
    { LDtmp,    { u_K, u_F, u_C },       "LDtmp" },
    { LCtmp,    { u_K, u_F, u_C },       "LCtmp" },
    { LEtmp,    { u_K, u_F, u_C },       "LEtmp" },
    { RDtmp,    { u_K, u_F, u_C },       "RDtmp" },
    { RCtmp,    { u_K, u_F, u_C },       "RCtmp" },
    { REtmp,    { u_K, u_F, u_C },       "REtmp" },
    { f_END,    { },                     NULL }     // marks the END of the array
};

// define all valid ParseUnits
const SHParseUnit SHParseUnit::allParseUnits[] =
{
    { u_met, "met" },   // (metric)     : km/meters/kg, etc.
    { u_imp, "imp" },   // (imperial)   : ft/miles/pounds, etc.
    { u_G,   "G" },     // (gravities)  : related to acceleration
    { u_M,   "M" },     // (mach)       : related to speed
    { u_K,   "K" },     // (Kelvin)     : temperature
    { u_F,   "F" },     // (Fahrenheit) : temperature
    { u_C,   "C" },     // (Celsiuis)   : temperature
    { u_NA,  "-" },     // "N/A"; used for fields where only one unit is valid, such as "degrees" for angles
    { u_END, NULL},     // marks the END of the array
};

// Set a cell via text IDs; use this when parsing values from the config file
// returns: true if cell set successfully, false if one or more parameters invalid
// pFieldName = "Alt", "Vel", etc.
// pUnits = "met", "imp", etc.
bool SecondaryHUDMode::SetCell(int row, int column, const char *pFieldName, const char *pUnits)
{
    // row and column will be verified later

    // walk through each valid field searching for a match
    for (int fieldIndex = 0; ; fieldIndex++)
    {
        const SHField &field = SHField::allFields[fieldIndex];
        if (field.id == f_END)
            break;  // end of fields

        if (strcmp(pFieldName, field.label) == 0)
        {
            // we have a field ID match; now validate the units requested
            for (int unitsIndex=0; ; unitsIndex++)
            {
                const SHParseUnit &parseUnit = SHParseUnit::allParseUnits[unitsIndex];
                if (parseUnit.units == u_END)
                    return false;   // invalid units specified for this field
 
                if (strcmp(pUnits, parseUnit.parseTag) == 0)
                {
                    // both field and unit are valid; attempt to set the cell values
                    // this call will also validate the row and column
                    bool retVal = SetCell(row, column, field, parseUnit.units);
                    return retVal;
                }
            }
        }
    }

    // invalid field name
    return false;
}
// Set a cell using verfied enum values
// row, column are zero-based: 0-(SH_ROW_COUNT-1), 0-1
// returns: true if cell set successfully, false if one or more parameters invalid
bool SecondaryHUDMode::SetCell(const int row, const int column, const SHField &field, const Units units)
{
    if ((row < 0) || (row >= SH_ROW_COUNT) || (column < 0) || (column > 1))
        return false;

    // cell data OK
    Cell &cell = m_cells[row][column];
    cell.pField = &field;
    cell.units = units;
    // value and valueStr are only set at render time

    return true;
}

