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
// XRCommon_DMG.h: Header file containing common macros and data for XR damage modeling
// ==============================================================

#pragma once

// Door heat FAILURE depends on how far the door is opened (doorProc)
#define OPEN_DOOR_OVER_TEMP(tempK)          (GetXR1Config()->HullHeatingDamageEnabled && AllowDamageIfDockedCheck() && !Playback() && ((tempK * (0.75 + (doorProc / 4.0))) > m_hullTemperatureLimits.doorOpen))
#define OPEN_DOOR_WARN_TEMP(tempK)          (GetXR1Config()->HullHeatingDamageEnabled && AllowDamageIfDockedCheck() && !Playback() && (tempK > (m_hullTemperatureLimits.doorOpen * m_hullTemperatureLimits.doorOpenWarning)))
#define OPEN_DOOR_OVER_PRESSURE_WARN(maxDynP)    (GetXR1Config()->DoorStressDamageEnabled && AllowDamageIfDockedCheck() && !Playback() && (GetDynPressure() > maxDynP))
// Door dynp FAILURE depends on how far the door is opened (doorProc)
#define OPEN_DOOR_OVER_PRESSURE_FAIL(maxDynP)    (GetXR1Config()->DoorStressDamageEnabled && AllowDamageIfDockedCheck() && !Playback() && (((GetDynPressure() * (0.20 + (doorProc / 1.25))) > maxDynP )))

// do not issue warnings if door already failed
#define IS_DOOR_OPEN(status)  (status != DoorStatus::DOOR_CLOSED)   // includes DoorStatus::DOOR_FAILED
#define IS_DOOR_FAILED() (*doorStatus == DoorStatus::DOOR_FAILED)
#define IS_DOOR_WARNING(tempK, maxDynP)        ((IS_DOOR_FAILED() == false) && (OPEN_DOOR_WARN_TEMP(tempK) || OPEN_DOOR_OVER_PRESSURE_WARN((maxDynP * DOOR_DYNAMIC_PRESSURE_WARNING_THRESHOLD))))
#define IS_DOOR_FAILURE(tempK, maxDynP)        ((IS_DOOR_FAILED() == false) && (OPEN_DOOR_OVER_TEMP(tempK) || OPEN_DOOR_OVER_PRESSURE_FAIL(maxDynP)))


