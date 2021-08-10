// ==============================================================
// XRCommon_DMG.h: Header file containing common macros and data for XR damage modeling
//
// Copyright 2013-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
// ==============================================================

#pragma once

// Door heat FAILURE depends on how far the door is opened (doorProc)
#define OPEN_DOOR_OVER_TEMP(tempK)          (GetXR1Config()->HullHeatingDamageEnabled && AllowDamageIfDockedCheck() && !Playback() && ((tempK * (0.75 + (doorProc / 4.0))) > m_hullTemperatureLimits.doorOpen))
#define OPEN_DOOR_WARN_TEMP(tempK)          (GetXR1Config()->HullHeatingDamageEnabled && AllowDamageIfDockedCheck() && !Playback() && (tempK > (m_hullTemperatureLimits.doorOpen * m_hullTemperatureLimits.doorOpenWarning)))
#define OPEN_DOOR_OVER_PRESSURE_WARN(maxDynP)    (GetXR1Config()->DoorStressDamageEnabled && AllowDamageIfDockedCheck() && !Playback() && (GetDynPressure() > maxDynP))
// Door dynp FAILURE depends on how far the door is opened (doorProc)
#define OPEN_DOOR_OVER_PRESSURE_FAIL(maxDynP)    (GetXR1Config()->DoorStressDamageEnabled && AllowDamageIfDockedCheck() && !Playback() && (((GetDynPressure() * (0.20 + (doorProc / 1.25))) > maxDynP )))

// do not issue warnings if door already failed
#define IS_DOOR_OPEN(status)  (status != DOOR_CLOSED)   // includes DOOR_FAILED
#define IS_DOOR_FAILED() (*doorStatus == DOOR_FAILED)
#define IS_DOOR_WARNING(tempK, maxDynP)        ((IS_DOOR_FAILED() == false) && (OPEN_DOOR_WARN_TEMP(tempK) || OPEN_DOOR_OVER_PRESSURE_WARN((maxDynP * DOOR_DYNAMIC_PRESSURE_WARNING_THRESHOLD))))
#define IS_DOOR_FAILURE(tempK, maxDynP)        ((IS_DOOR_FAILED() == false) && (OPEN_DOOR_OVER_TEMP(tempK) || OPEN_DOOR_OVER_PRESSURE_FAIL(maxDynP)))


