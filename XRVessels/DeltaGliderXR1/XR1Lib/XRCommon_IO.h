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
// XRCommon_IO.h: Header file for base XR I/O implementation class that parses scenario file settings common to all XR vessels.
// ==============================================================

#pragma once

//
// Utility macros
//
// NOTE: you should define the following local variables in your clbkLoadStateEx method to use these macros:
//    char *line;
//    int len;
//    bool found;   // set to true if any SSCANF macro was invoked

#define IF_FOUND(name)  if (!_strnicmp(line, name, (len = static_cast<int>(strlen(name))))) 
#define IF_FOUND_CONFIG_OVERRIDE(name)  IF_FOUND("CONFIG_OVERRIDE_"##name)
#define SET_CONFIG_OVERRIDE_INT(field, val)                             \
{                                                                       \
    const_cast<XR1ConfigFileParser *>(GetXR1Config())->field = val;     \
    char msg[256];                                                      \
    sprintf(msg, "INFO: scenario data overriding default XR configuration file setting: %s = %d", #field, val);  \
    GetXR1Config()->WriteLog(msg);                                      \
    m_configOverrideBitmask |= CONFIG_OVERRIDE_##field;                 \
}

#define SET_CONFIG_OVERRIDE_DOUBLE(field, val)                          \
{                                                                       \
    const_cast<XR1ConfigFileParser *>(GetXR1Config())->field = val;     \
    char msg[256];                                                      \
    sprintf(msg, "INFO: scenario data overriding default XR configuration file setting: %s = %lf", #field, val);  \
    GetXR1Config()->WriteLog(msg);                                      \
    m_configOverrideBitmask |= CONFIG_OVERRIDE_##field;                 \
}

#define SSCANF1(formatStr, a1) bFound = true; sscanf(line + len, formatStr, a1)
#define SSCANF2(formatStr, a1, a2) bFound = true; sscanf(line + len, formatStr, a1, a2)
#define SSCANF3(formatStr, a1, a2, a3) bFound = true; sscanf(line + len, formatStr, a1, a2, a3)
#define SSCANF4(formatStr, a1, a2, a3, a4) bFound = true; sscanf(line + len, formatStr, a1, a2, a3, a4)
#define SSCANF5(formatStr, a1, a2, a3, a4, a5) bFound = true; sscanf(line + len, formatStr, a1, a2, a3, a4, a5)
#define SSCANF6(formatStr, a1, a2, a3, a4, a5, a6) bFound = true; sscanf(line + len, formatStr, a1, a2, a3, a4, a5, a6)
#define SSCANF7(formatStr, a1, a2, a3, a4, a5, a6, a7) bFound = true; sscanf(line + len, formatStr, a1, a2, a3, a4, a5, a6, a7)

#define SSCANF_BOOL(var) bFound = true; { int temp = 0; SSCANF1("%d", &temp); var = (temp != 0); }
#define SSCANF_BOOL2(var1,var2) bFound = true; { int temp[2]; temp[0] = temp[1] = 0; SSCANF2("%d %d", temp, temp+1); var1 = (temp[0] != 0); var2 = (temp[1] != 0); }
#define SSCANF_BOOL3(var1,var2,var3) bFound = true; { int temp[3]; temp[0] = temp[1] = temp[2] = 0; SSCANF3("%d %d %d", temp, temp+1, temp+2); var1 = (temp[0] != 0); var2 = (temp[1] != 0); var3 = (temp[2] != 0); }
#define SSCANF_BOOL6(var1,var2,var3,var4,var5,var6) bFound = true; { int temp[6]; memset(temp, 0, sizeof(temp)); SSCANF6("%d%d%d%d%d%d", temp, temp+1, temp+2, temp+3, temp+4, temp+5); var1 = (temp[0] != 0); var2 = (temp[1] != 0); var3 = (temp[2] != 0); var4 = (temp[3] != 0); var5 = (temp[4] != 0); var6 = (temp[5] != 0);}
