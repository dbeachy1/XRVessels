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
// SecondaryHUDData.cpp
//
// Defines data classes to the secondary HUD.
// ==============================================================

#include "DeltaGliderXR1.h"
#include "SecondaryHUDData.h"

// define all valid Secondary HUD fields
const SHField SHField::allFields[] = 
{ 
    { FieldID::Alt,      {  Units::u_met, Units::u_imp },               "Alt" },
    { FieldID::Vel,      {  Units::u_met, Units::u_imp, Units::u_M},    "Vel" },
    { FieldID::StatP,    {  Units::u_met, Units::u_imp },               "StatP" },
    { FieldID::DynP,     {  Units::u_met, Units::u_imp },               "DynP" },
    { FieldID::OAT,      {  Units::u_K, Units::u_F, Units::u_C },       "OAT" },
    { FieldID::Hdg,      {  Units::u_NA },                              "Hdg" },
    { FieldID::VS,       {  Units::u_met, Units::u_imp },                "v/s" },
    { FieldID::AccX,     {  Units::u_G, Units::u_met, Units::u_imp },   "AccX" },
    { FieldID::AccY,     {  Units::u_G, Units::u_met, Units::u_imp },   "AccY" },
    { FieldID::AccZ,     {  Units::u_G, Units::u_met, Units::u_imp },   "AccZ" },
    { FieldID::Mass,     {  Units::u_met, Units::u_imp },               "Mass" },
    { FieldID::Ecc,      {  Units::u_NA },                              "Ecc" },
    { FieldID::Inc,      {  Units::u_NA },                              "Inc" },
    { FieldID::PeT,      {  Units::u_NA },                              "PeT" },
    { FieldID::ApT,      {  Units::u_NA },                              "ApT" },
    { FieldID::PeA,      {  Units::u_met, Units::u_imp },               "PeA" },
    { FieldID::ApA,      {  Units::u_met, Units::u_imp },               "ApA" },
    { FieldID::PeR,      {  Units::u_met, Units::u_imp },               "PeR" },
    { FieldID::ApR,      {  Units::u_met, Units::u_imp },               "ApR" },
    { FieldID::Pitch,    {  Units::u_NA },                              "Pitch" },
    { FieldID::Bank,     {  Units::u_NA },                              "Bank" },
    { FieldID::Slope,    {  Units::u_NA },                              "Slope" },
    { FieldID::Slip,     {  Units::u_NA },                              "Slip" },
    { FieldID::AOA,      {  Units::u_NA },                              "AOA" },
    { FieldID::Long,     {  Units::u_NA },                              "Long" },
    { FieldID::Lat,      {  Units::u_NA },                              "Lat" },
    { FieldID::LEng,     {  Units::u_met, Units::u_imp },               "LEng" },
    { FieldID::REng,     {  Units::u_met, Units::u_imp },               "REng" },
    { FieldID::MEng,     {  Units::u_met, Units::u_imp },               "MEng" },
    { FieldID::FHov,     {  Units::u_met, Units::u_imp },               "FHov" },
    { FieldID::AHov,     {  Units::u_met, Units::u_imp },               "AHov" },
    { FieldID::BHov,     {  Units::u_met, Units::u_imp },               "BHov" },
    { FieldID::LScrm,    {  Units::u_met, Units::u_imp },               "LScrm" },
    { FieldID::RScrm,    {  Units::u_met, Units::u_imp },               "RScrm" },
    { FieldID::BScrm,    {  Units::u_met, Units::u_imp },               "BScrm" },
    { FieldID::rcs_1,    {  Units::u_met, Units::u_imp },               "rcs1" },
    { FieldID::rcs_2,    {  Units::u_met, Units::u_imp },               "rcs2" },
    { FieldID::rcs_3,    {  Units::u_met, Units::u_imp },               "rcs3" },
    { FieldID::rcs_4,    {  Units::u_met, Units::u_imp },               "rcs4" },
    { FieldID::rcs_5,    {  Units::u_met, Units::u_imp },               "rcs5" },
    { FieldID::rcs_6,    {  Units::u_met, Units::u_imp },               "rcs6" },
    { FieldID::rcs_7,    {  Units::u_met, Units::u_imp },               "rcs7" },
    { FieldID::rcs_8,    {  Units::u_met, Units::u_imp },               "rcs8" },
    { FieldID::rcs_9,    {  Units::u_met, Units::u_imp },               "rcs9" },
    { FieldID::rcs_10,   {  Units::u_met, Units::u_imp },               "rcs10" },
    { FieldID::rcs_11,   {  Units::u_met, Units::u_imp },               "rcs11" },
    { FieldID::rcs_12,   {  Units::u_met, Units::u_imp },               "rcs12" },
    { FieldID::rcs_13,   {  Units::u_met, Units::u_imp },               "rcs13" },
    { FieldID::rcs_14,   {  Units::u_met, Units::u_imp },               "rcs14" },
    { FieldID::LDtmp,    {  Units::u_K, Units::u_F, Units::u_C },       "LDtmp" },
    { FieldID::LCtmp,    {  Units::u_K, Units::u_F, Units::u_C },       "LCtmp" },
    { FieldID::LEtmp,    {  Units::u_K, Units::u_F, Units::u_C },       "LEtmp" },
    { FieldID::RDtmp,    {  Units::u_K, Units::u_F, Units::u_C },       "RDtmp" },
    { FieldID::RCtmp,    {  Units::u_K, Units::u_F, Units::u_C },       "RCtmp" },
    { FieldID::REtmp,    {  Units::u_K, Units::u_F, Units::u_C },       "REtmp" },
    { FieldID::f_END,    { },                     nullptr }     // marks the END of the array
};

// define all valid ParseUnits
const SHParseUnit SHParseUnit::allParseUnits[] =
{
    { Units::u_met, "met" },   // (metric)     : km/meters/kg, etc.
    { Units::u_imp, "imp" },   // (imperial)   : ft/miles/pounds, etc.
    { Units::u_G,   "G" },     // (gravities)  : related to acceleration
    { Units::u_M,   "M" },     // (mach)       : related to speed
    { Units::u_K,   "K" },     // (Kelvin)     : temperature
    { Units::u_F,   "F" },     // (Fahrenheit) : temperature
    { Units::u_C,   "C" },     // (Celsiuis)   : temperature
    { Units::u_NA,  "-" },     // "N/A"; used for fields where only one unit is valid, such as "degrees" for angles
    { Units::u_END, nullptr},     // marks the END of the array
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
        if (field.id == FieldID::f_END)
            break;  // end of fields

        if (strcmp(pFieldName, field.label) == 0)
        {
            // we have a field ID match; now validate the units requested
            for (int unitsIndex=0; ; unitsIndex++)
            {
                const SHParseUnit &parseUnit = SHParseUnit::allParseUnits[unitsIndex];
                if (parseUnit.units == Units::u_END)
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

