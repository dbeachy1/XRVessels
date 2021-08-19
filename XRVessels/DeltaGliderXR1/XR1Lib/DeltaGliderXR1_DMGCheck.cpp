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
// XR vessel damage checks
// ==============================================================

#include "DeltaGliderXR1.h"
#include "AreaIDs.h"
#include "XRCommon_DMG.h"

void DeltaGliderXR1::TestDamage()
{
    // work around Orbiter startup step bug: do not check for damage within the first two seconds of startup UNLESS we are crashed
    if ((IsCrashed() == false) && (GetAbsoluteSimTime() < 2.0))
        return;

    bool newdamage = false;
    double dt = oapiGetSimStep();

    // fail the ailerons/elevons if marked as damaged in the scenario file load
    FailAileronsIfDamaged();

    // if crashed, damage everything!
    if (IsCrashed())
    {
        // do not process crash logic more than once
        if (m_crashProcessed == false)
        {
            m_crashProcessed = true; // do not do this more than once
            PerformCrashDamage();
            newdamage = true;
        }
        goto exit;  // nothing else to check
    }

    // if crew incapacitated, temporarily (for this timestep) disable systems so ship is unflyable
    if (IsCrewIncapacitatedOrNoPilotOnBoard())
    {
        // kill all the engine throttles
        for (int i = 0; i < 2; i++)
        {
            SetThrusterLevel(th_scram[i], 0);
            scram_intensity[i] = 0;

            SetThrusterLevel(th_hover[i], 0);
            SetThrusterLevel(th_main[i], 0);
            SetThrusterLevel(th_retro[i], 0);
        }

        // same for RCS jets
        for (int i = 0; i < 14; i++)
            SetThrusterLevel(th_rcs[i], 0);

        // turn off the RCS and airfoil systems
        SetAttitudeMode(RCS_NONE);
        SetADCtrlMode(0);

        // APU check is handled manually elsewhere

        // ATC ON/OFF is handled at the point of crew being incapacitated; however, we must turn ATC off here in case the scenario was just reloaded.
        XRSoundOnOff(XRSound::RadioATCGroup, false);
        // cabin airflow still active
    }

    // airframe damage as a result of wingload stress
    // or excessive dynamic pressure

    double load = GetLift() / WING_AREA; // L/S
    double dynp = GetDynPressure();  // dynamic pressure

    if (GetXR1Config()->WingStressDamageEnabled && AllowDamageIfDockedCheck() && !Playback())
    {
        if (load > WINGLOAD_MAX || load < WINGLOAD_MIN || dynp > DYNP_MAX)
        {
            double alpha = max((dynp - DYNP_MAX) * 1e-5,         // amount over-limit * 100K
                (load > 0 ? load - WINGLOAD_MAX : WINGLOAD_MIN - load) * 5e-5);
            double p = 1.0 - exp(-alpha * dt); // probability of failure
            if (oapiRand() < p)
            {
                const char* pMsg;
                // simulate structural failure by distorting the airfoil definition
                int rfail = static_cast<int>(oapiRand() * RAND_MAX);      // use oapiRand() here since it's already seeded with a random value
                switch (rfail & 3)
                {
                case 0: // fail left wing
                    lwingstatus *= exp(-alpha * oapiRand());
                    pMsg = "Left Wing Failure!";
                    m_warningLights[static_cast<int>(WarningLight::wlLwng)] = true;
                    break;
                case 1: // fail right wing
                    rwingstatus *= exp(-alpha * oapiRand());
                    pMsg = "Right Wing Failure!";
                    m_warningLights[static_cast<int>(WarningLight::wlRwng)] = true;
                    break;
                case 2:
                {
                    pMsg = "Left Aileron Failure!";
                    aileronfail[0] = aileronfail[1] = true;     // delete both aileron mesh groups
                    m_warningLights[static_cast<int>(WarningLight::wlLail)] = true;
                    brake_status = DoorStatus::DOOR_FAILED;  // airbrake inoperable as well
                    m_warningLights[static_cast<int>(WarningLight::wlAirb)] = true;
                    FailAileronsIfDamaged();   // delete control surface
                }
                break;
                case 3:
                {
                    pMsg = "Right Aileron Failure!";
                    aileronfail[2] = aileronfail[3] = true;     // delete both aileron mesh groups
                    m_warningLights[static_cast<int>(WarningLight::wlRail)] = true;
                    brake_status = DoorStatus::DOOR_FAILED;  // airbrake inoperable as well
                    m_warningLights[static_cast<int>(WarningLight::wlAirb)] = true;
                    FailAileronsIfDamaged();   // delete control surface
                }
                break;
                }
                ShowWarning("Warning airframe damage.wav", ST_WarningCallout, pMsg);
            }   // if block

            newdamage = true;
        }
        else  // let's check for warnings
        {
#define CHECK_MAX_THRESHOLD(value, max, soundFilename, msg, lightIdx, lightIdx2, wingstatus, wingstatus2) \
    if (value > (max * warningThreshold))                      \
    {                                                          \
        ShowWarning(soundFilename, ST_WarningCallout, msg);    \
        m_warningLights[static_cast<int>(lightIdx)] = true;    \
        m_warningLights[static_cast<int>(lightIdx2)] = true;   \
        newdamage = true;                                      \
        wingWarnLightsOn = true;                               \
    }                                                          \
    else if (wingWarnLightsOn == false)                        \
    {                                                          \
        if (wingstatus == 1.0)  m_warningLights[static_cast<int>(lightIdx)] = false;  \
        if (wingstatus2 == 1.0) m_warningLights[static_cast<int>(lightIdx2)] = false; \
    }

#define CHECK_MIN_THRESHOLD(value, min, soundFilename, msg, lightIdx, lightIdx2, wingstatus, wingstatus2) \
    if (value < (min * warningThreshold))                      \
    {                                                          \
        ShowWarning(soundFilename, ST_WarningCallout, msg);    \
        m_warningLights[static_cast<int>(lightIdx)] = true;                      \
        m_warningLights[static_cast<int>(lightIdx2)] = true;                     \
        wingWarnLightsOn = true;                               \
        newdamage = true;                                      \
    }                                                          \
    else if (wingWarnLightsOn == false)                        \
    {                                                          \
        if (wingstatus == 1.0)  m_warningLights[static_cast<int>(lightIdx)] = false;  \
        if (wingstatus2 == 1.0) m_warningLights[static_cast<int>(lightIdx2)] = false; \
    }

            static const double warningThreshold = .85;     // 85%
            const char* pWingStress = "WARNING Wing Stress.wav";
            bool wingWarnLightsOn = false;  // true if either wing warning light is turned on below; we don't want to turn off
            CHECK_MAX_THRESHOLD(load, WINGLOAD_MAX, pWingStress, "Wing load over 85% of maximum.", WarningLight::wlRwng, WarningLight::wlLwng, rwingstatus, lwingstatus);
            CHECK_MIN_THRESHOLD(load, WINGLOAD_MIN, pWingStress, "Negative wing load over 85%&of maximum.", WarningLight::wlRwng, WarningLight::wlLwng, rwingstatus, lwingstatus);
            CHECK_MAX_THRESHOLD(dynp, DYNP_MAX, "Warning dynamic pressure.wav", "Dynamic pressure over 85%&of maximum.", WarningLight::wlDynp, WarningLight::wlDynp, 1.0, 1.0);  // always OK to turn off warning light for DynP
        }
    }   // if WingStressDamageEnabled

    //
    // Check for door-related heat and/or dynamic pressure damage here
    // Note that a given damange type is only checked if it is enabled.
    //
    newdamage |= CheckAllDoorDamage();     // returns true if any door is damaged

    // 
    // Check SCRAM engine temperature
    //
    if (GetXR1Config()->ScramEngineOverheatDamageEnabled && !Playback())
    {
        // NOTE: since doors always work in tandem, we only need to check the LEFT engine here
        const double alpha = CheckScramTemperature(ramjet->Temp(0, 0), MAX_SCRAM_TEMPERATURE);
        if (alpha != 0)
        {
            // ENGINE DAMAGE -- check for critical engine failure vs. just engine damage
            // NOTE: as an example, alpha values if pilot is over max temp:
            //  0% over = 0.00
            //  5% over = 0.20
            // 10% over = 0.42
            // 20% over = 0.88
            // 30% over = 1.38
            // NOTE: do not integrate dt here; dt was already taken into account by CheckTemperature
            // pick a random engine and damage it based on alpha delta
            int engineIndex = ((oapiRand() < 0.5) ? 0 : 1);
            const double engineFrac = max(0, (1.0 - alpha));
            ramjet->SetEngineIntegrity(engineIndex, ramjet->GetEngineIntegrity(engineIndex) * engineFrac);

            // NOTE: SCRAM warning light already handled by CheckScramTemperature

            const double engineInteg = ramjet->GetEngineIntegrity(engineIndex);
            const double mach = GetMachNumber();
            char temp[80];
            if (oapiRand() > engineInteg)
            {
                sprintf(temp, "#%d SCRAM ENGINE EXPLOSION at Mach %.1lf!", (engineIndex + 1), mach);
                DoCrash(temp, 0);
            }
            else
            {
                sprintf(temp, "SCRAM ENGINE #%d DAMAGE&at Mach %.1lf!&Engine Integrity=%.1lf%%", engineIndex, mach, engineInteg * 100);
                // NOTE: audio callout already occurred
                ShowWarning(nullptr, DeltaGliderXR1::ST_None, temp, true);   // force this
                newdamage = true;
            }
        }
    }

    //
    // Check hull temperatures
    // 
    if (GetXR1Config()->HullHeatingDamageEnabled && AllowDamageIfDockedCheck() && !Playback())
        newdamage |= CheckHullHeatingDamage();

exit:
    if (newdamage)
    {
        m_MWSActive = true;
        ApplyDamage();
        //UpdateDamageDialog (this);
    }

    // if no warning present, reset the MWS automatically
    if (!IsWarningPresent())
        m_MWSActive = false;    // it's all good now...
}

//
// Check all hull surfaces for heat damage.
// 
// NOTE: CheckTemperature will turn on warning lights for doors and hull temp as necessary; however
// we must reset the HTMP light ourselves since any surface can trigger it.  CheckTemperature will only 
// SET the light, never CLEAR it.  Therefore, we clear it here ourselves.
//
// Returns: true if any surface damaged, false otherwise
bool DeltaGliderXR1::CheckHullHeatingDamage()
{
    bool newdamage = false;
    double alpha = 0;
    char temp[128];
    const double mach = GetMachNumber();
    m_warningLights[static_cast<int>(WarningLight::wlHtmp)] = false;     // assume hull temp warning light OFF

    // check nosecone temperature using both nosecone and hover doors
    if (CheckTemperature(m_noseconeTemp, m_hullTemperatureLimits.noseCone, IS_DOOR_OPEN(nose_status)) != 0)
    {
        // HULL FAILURE - crew death!
        sprintf(temp, "NOSECONE BREACH at Mach %.1lf!", mach);
        DoCrash(temp, 0);
    }

    //
    // Note: checking these lower hull items separately will increase our chances of hull breach when more than one door is open; this is what we want!
    //
    if ((CheckTemperature(m_noseconeTemp, m_hullTemperatureLimits.noseCone, IS_DOOR_OPEN(hoverdoor_status)) != 0) ||
        (CheckTemperature(m_noseconeTemp, m_hullTemperatureLimits.noseCone, IS_DOOR_OPEN(gear_status)) != 0))
    {
        // LOWER HULL FAILURE - crew death!
        sprintf(temp, "LOWER HULL BREACH at Mach %.1lf!", mach);
        DoCrash(temp, 0);
    }

    // This check assumes the retro doors are related to the wings
    const bool retroDoorsOpen = IS_DOOR_OPEN(rcover_status);
    if ((alpha = CheckTemperature(m_leftWingTemp, m_hullTemperatureLimits.wings, retroDoorsOpen)) != 0)
    {
        // WING DAMAGE -- check for critical ship failure vs. just wing damage
        // NOTE: as an example, alphas values if pilot is over max temp:
        //  0% over = 0.00
        //  5% over = 0.10
        // 10% over = 0.21
        // 20% over = 0.44
        // 30% over = 0.69
        // 40% over = 0.96 
        // 50% over = 1.25
        // NOTE: do not integrate dt here; dt was already taken into account by CheckTemperature
        const double wingFrac = min(0, (1.0 - alpha));
        lwingstatus *= wingFrac;
        m_warningLights[static_cast<int>(WarningLight::wlLwng)] = true;   // warning light ON

        if (oapiRand() > lwingstatus)
        {
            sprintf(temp, "LEFT WING BREACH at Mach %.1lf!", mach);
            DoCrash(temp, 0);
        }
        else
        {
            sprintf(temp, "LEFT WING DAMAGE at Mach %.1lf!&Wing Integrity=%.1lf%%", mach, lwingstatus * 100);
            ShowWarning(nullptr, DeltaGliderXR1::ST_None, temp, true);   // force this 
            newdamage = true;
        }
    }

    // This check assumes the retro doors are related to the wings
    if ((alpha = CheckTemperature(m_rightWingTemp, m_hullTemperatureLimits.wings, retroDoorsOpen)) != 0)
    {
        const double wingFrac = min(0, (1.0 - alpha));
        rwingstatus *= wingFrac;
        m_warningLights[static_cast<int>(WarningLight::wlRwng)] = true;   // warning light ON

        // WING DAMAGE -- check for critical ship failure vs. just wing damage
        if (oapiRand() > rwingstatus)
        {
            sprintf(temp, "RIGHT WING BREACH at Mach %.1lf!", mach);
            DoCrash(temp, 0);
        }
        else
        {
            sprintf(temp, "RIGHT WING DAMAGE at Mach %.1lf&Wing Integrity=%.1lf%%", mach, rwingstatus * 100);
            ShowWarning(nullptr, DeltaGliderXR1::ST_None, temp, true); // force this
            newdamage = true;
        }
    }

    // This check assumes the escape hatch is close to the cockpit.
    if (CheckTemperature(m_cockpitTemp, m_hullTemperatureLimits.cockpit, IS_DOOR_OPEN(hatch_status)) != 0)
    {
        // HULL FAILURE - crew death!
        sprintf(temp, "COCKPIT BREACH at Mach %.1lf!", mach);
        DoCrash(temp, 0);
    }

    // Note: the XR1 does not have a payload bay, but it's OK to check it here for the purpose of subclasses
    // top hull max temp is tied to: 1) radiators, and 2) bay doors
    if ((CheckTemperature(m_topHullTemp, m_hullTemperatureLimits.topHull, IS_DOOR_OPEN(radiator_status)) != 0) ||
        (CheckTemperature(m_topHullTemp, m_hullTemperatureLimits.topHull, IS_DOOR_OPEN(bay_status)) != 0))
    {
        // HULL FAILURE - crew death!
        sprintf(temp, "TOP HULL BREACH at Mach %.1lf!", mach);
        DoCrash(temp, 0);
    }

    return newdamage;
}

//
// Returns the *effective* external temperature of the air molecules, taking static pressure into account.
double DeltaGliderXR1::GetExternalTemperature() const
{
    /* Orbiter 2009 new atmosphere models fix: we cannot just take OAT as a baseline anymore because the temperature
       reported by the core is very high in the upper atmosphere, even though dynamic pressure is practically non-existant.
       Empirical testing with the surface MFD shows that mach and temperature are valid at DYNAMIC PRESSURE 2.78 pascals and STATIC PRESSURE of about 0.014 pascal.

            100% OAT at staticPressure >= OAT_VALID_STATICP_THRESHOLD
              ...tapering smoothly down to...
            10% OAT at 0 kpa  (lower figure is arbitrary)
    */
    double effectiveOATFraction = (GetAtmPressure() / OAT_VALID_STATICP_THRESHOLD);

    // keep in range
    if (effectiveOATFraction > 1.0)
        effectiveOATFraction = 1.0;   // baseline temp is never greater than OAT
    else if (effectiveOATFraction < 0.1)
        effectiveOATFraction = 0.1;   // baseline temp is never less than 10% of OAT

    // WARNING: THIS SHOULD BE THE *ONLY* PLACE IN THE CODE WHERE GetAtmTemperature() IS INVOKED!  All other code should invoke GetExternalTemperature() instead.
    const double extTemp = GetAtmTemperature() * effectiveOATFraction;  // this is in Kelvin, so it is never negative
    return extTemp;
}

//
// Check for door-related heat and/or dynamic pressure damage here
// Note that a given damange type is only checked if it is enabled.
//
// Returns: true if any damage detected, false otherwise
bool DeltaGliderXR1::CheckAllDoorDamage()
{
    bool newdamage = false;
    newdamage |= CheckDoorFailure(&nose_status);
    newdamage |= CheckDoorFailure(&rcover_status);
    newdamage |= CheckDoorFailure(&hatch_status);
    newdamage |= CheckDoorFailure(&radiator_status);
    newdamage |= CheckDoorFailure(&gear_status);
    newdamage |= CheckDoorFailure(&hoverdoor_status);
    // SCRAM doors cannot fail for heat or pressure, so don't check them

    return newdamage;
}

// Check whether ANY system is damaged. Invoked when loading or saving state.
// Returns: true if any damage present, false if all systems green
bool DeltaGliderXR1::IsDamagePresent() const
{
    bool retVal = false;        // assume no damage

    // loop through all surfaces
    for (int i = 0; i <= static_cast<int>(D_END); i++)
    {
        DamageStatus ds = GetDamageStatus((DamageItem)i);
        if (ds.fracIntegrity < 1.0)
        {
            retVal = true;  // damage present
            break;
        }
    }

    return retVal;
}

// Check whether ANY warning is active.  Invoked on startup.
// Returns: true if any warning present, false if no warnings present
bool DeltaGliderXR1::IsWarningPresent() const
{
    bool retVal = false;        // assume no damage

    // loop through all warning lights
    for (int i = 0; i < WARNING_LIGHT_COUNT; i++)
    {
        bool warningLightActive = m_warningLights[i];
        if (warningLightActive)
        {
            retVal = true;  // warning present
            break;
        }
    }

    // check for APU warning
    if (m_apuWarning)
        retVal = true;

    // if crew is DEAD or incapacitated, that's worth a warning...
    if (IsCrewIncapacitatedOrNoPilotOnBoard())
        retVal = true;

    return retVal;
}

// returns DamageStatus (a static variable)
// This queries the actual SYSTEM STATE (e.g., current thrust output) to determine whether an item is damaged.
const DamageStatus& DeltaGliderXR1::GetDamageStatus(DamageItem item) const
{
    double frac;
    const char* pLabel = "???";
    const char* pShortLabel = "???";
    char tempLabel[8];
    bool onlineOffline = true;     // assume online/offline

    switch (item)
    {
    case DamageItem::LeftWing:
        frac = lwingstatus;
        pLabel = "Left Wing";
        pShortLabel = "LWng";
        onlineOffline = false;     // has partial failure
        break;

    case DamageItem::RightWing:
        frac = rwingstatus;
        pLabel = "Right Wing";
        pShortLabel = "RWng";
        onlineOffline = false;     // has partial failure
        break;

    case DamageItem::LeftAileron:
        frac = ((aileronfail[0] | aileronfail[1]) ? 0 : 1);    // either mesh index 0 or 1 could be marked FAILED, so we must check both
        pLabel = "Left Aileron";
        pShortLabel = "LAil";
        break;

    case DamageItem::RightAileron:
        frac = ((aileronfail[2] | aileronfail[3]) ? 0 : 1);    // either mesh index 2 or 3 could be marked FAILED, so we must check both
        pLabel = "Right Aileron";
        pShortLabel = "RAil";
        break;

    case DamageItem::LandingGear:
        frac = ((gear_status == DoorStatus::DOOR_FAILED) ? 0 : 1);
        pLabel = "Landing Gear";
        pShortLabel = "Gear";
        break;

    case DamageItem::Nosecone:
        frac = ((nose_status == DoorStatus::DOOR_FAILED) ? 0 : 1);
        pLabel = NOSECONE_LABEL;
        pShortLabel = NOSECONE_SHORT_LABEL;
        break;

    case DamageItem::RetroDoors:
        frac = ((rcover_status == DoorStatus::DOOR_FAILED) ? 0 : 1);
        pLabel = "Retro Doors";
        pShortLabel = "RDor";
        break;

    case DamageItem::Hatch:
        frac = ((hatch_status == DoorStatus::DOOR_FAILED) ? 0 : 1);
        pLabel = "Top Hatch";
        pShortLabel = "Htch";
        break;

    case DamageItem::Radiator:
        frac = ((radiator_status == DoorStatus::DOOR_FAILED) ? 0 : 1);
        pLabel = "Radiator";
        pShortLabel = "Rad";
        break;

    case DamageItem::Airbrake:
        frac = ((brake_status == DoorStatus::DOOR_FAILED) ? 0 : 1);
        pLabel = "Airbrake";
        pShortLabel = "Airb";
        break;

    case DamageItem::MainEngineLeft:
    {
        const double maxMainThrust = MAX_MAIN_THRUST[GetXR1Config()->MainEngineThrust];
        frac = (maxMainThrust > 0 ? (GetThrusterMax0(th_main[0]) / MAX_MAIN_THRUST[GetXR1Config()->MainEngineThrust]) : 1.0);  // if max main thrust set to zero via cheatcode, engines cannot fail (avoid divide-by-zero here as well)
        pLabel = "Left Main Engine";
        pShortLabel = "LEng";
        onlineOffline = false;     // has partial failure
        break;
    }

    case DamageItem::MainEngineRight:
    {
        const double maxMainThrust = MAX_MAIN_THRUST[GetXR1Config()->MainEngineThrust];
        frac = (maxMainThrust > 0 ? (GetThrusterMax0(th_main[1]) / MAX_MAIN_THRUST[GetXR1Config()->MainEngineThrust]) : 1.0);  // if max main thrust set to zero via cheatcode, engines cannot fail (avoid divide-by-zero here as well)
        pLabel = "Right Main Engine";
        pShortLabel = "REng";
        onlineOffline = false;     // has partial failure
        break;
    }

    case DamageItem::SCRAMEngineLeft:
        frac = ramjet->GetEngineIntegrity(0);
        pLabel = "Left SCRAM Engine";
        pShortLabel = "LScr";
        onlineOffline = false;     // has partial failure
        break;

    case DamageItem::SCRAMEngineRight:
        frac = ramjet->GetEngineIntegrity(1);
        pLabel = "Right SCRAM Engine";
        pShortLabel = "RScr";
        onlineOffline = false;     // has partial failure
        break;

    case DamageItem::HoverEngineFore:
        // must make explicit check for damage here because we can vary the max thrust based on gimbaling
        frac = m_hoverEngineIntegrity[0];
        pLabel = "Fore Hover Engine";
        pShortLabel = "FHov";
        onlineOffline = false;     // has partial failure
        break;

    case DamageItem::HoverEngineAft:
        // must make explicit check for damage here because we can vary the max thrust based on gimbaling
        frac = m_hoverEngineIntegrity[1];
        // can't do this: frac = GetThrusterMax0(th_hover[1]) / MAX_HOVER_THRUST[GetXR1Config()->HoverEngineThrust];
        pLabel = "Aft Hover Engine";
        pShortLabel = "AHov";
        onlineOffline = false;     // has partial failure
        break;

    case DamageItem::RetroEngineLeft:
        frac = (MAX_RETRO_THRUST > 0 ? (GetThrusterMax0(th_retro[0]) / MAX_RETRO_THRUST) : 1.0);    // if retro max thrust set to zero via cheatcode, engines cannot fail (avoid divide-by-zero here as well)
        pLabel = "Left Retro Engine";
        pShortLabel = "LRet";
        onlineOffline = false;     // has partial failure
        break;

    case DamageItem::RetroEngineRight:
        frac = (MAX_RETRO_THRUST > 0 ? (GetThrusterMax0(th_retro[1]) / MAX_RETRO_THRUST) : 1.0);    // if retro max thrust set to zero via cheatcode, engines cannot fail (avoid divide-by-zero here as well)
        pLabel = "Right Retro Engine";
        pShortLabel = "RRet";
        onlineOffline = false;     // has partial failure
        break;

    case DamageItem::RCS1:
    case DamageItem::RCS2:
    case DamageItem::RCS3:
    case DamageItem::RCS4:
    case DamageItem::RCS5:
    case DamageItem::RCS6:
    case DamageItem::RCS7:
    case DamageItem::RCS8:
    case DamageItem::RCS9:
    case DamageItem::RCS10:
    case DamageItem::RCS11:
    case DamageItem::RCS12:
    case DamageItem::RCS13:
    case DamageItem::RCS14:
    {
        int index = static_cast<int>(item) - static_cast<int>(DamageItem::RCS1);    // 0-13
        // these are display names for the MDA screens, so keep the length reasonable
        static const char* pLabels[] =
        {
            "Forward Lower RCS", "Aft Upper RCS", "Forward Upper RCS", "Aft Lower RCS",
            "Forward Star. RCS", "Aft Port RCS", "Forward Port RCS", "Aft Star. RCS",
            "Outboard Upper Port RCS", "Outboard Lower Star. RCS", "Outboard Upper Star. RCS", "Outboard Lower Port RCS",
            "Aft RCS", "Forward RCS"
        };

        // for simplicity, we do not use RCS thrust as a damage indicator; we use in internal RCS array instead
        frac = m_rcsIntegrityArray[index];  // internal array
        pLabel = pLabels[index];
        sprintf(tempLabel, "RCS%d", (index + 1));     // RCS1...RCS14
        pShortLabel = tempLabel;
        onlineOffline = false;     // has partial failure
        break;
    }

    default:        // should never happen!
        frac = 0;
        pLabel = "???????";
        pShortLabel = "????";
        break;
    }

    // populate the structure
    static DamageStatus dmgStatus;

    dmgStatus.fracIntegrity = frac;
    strcpy(dmgStatus.label, pLabel);
    strcpy(dmgStatus.shortLabel, pShortLabel);
    dmgStatus.onlineOffline = onlineOffline;

    return dmgStatus;   // return by reference
}

// check HULL temperature and issue warning if necessary
// returns 0 if OK, > 0 = % max temperature exceeded^2
// doorOpen = is door on this surface open?
double DeltaGliderXR1::CheckTemperature(double tempK, double limitK, bool doorOpen)
{
    double retVal = 0;

    // NOTE: do not reset hull temp light here; we only turn it ON in this method
    // NOTE: do not set or reset door warning lights here; the dynamic pressure code sets and resets them.

    // adjust temp limit if door is open
    if (doorOpen)
        limitK = m_hullTemperatureLimits.doorOpen;

    // check for over-limit
    if (tempK > limitK)
    {
        m_warningLights[static_cast<int>(WarningLight::wlHtmp)] |= true;       // turn light ON

        // fail the structure if necessary
        const double dt = oapiGetSimStep();     // # of seconds since last timestep
        double exceededLimitMult = pow((tempK / limitK), 2);  // e.g. 1.21 = 10% over limit

        // # of seconds at this temp / average terminal failure interval (8 secs)
        double failureTimeFrac = dt / 8.0;
        double failureProbability = failureTimeFrac * exceededLimitMult;
        // sprintf(oapiDebugString(), "Damage failureProbablity=%lf", failureProbability);

        if (oapiRand() <= failureProbability)
        {
            retVal = (exceededLimitMult - 1.0);
            ShowWarning("Warning heat damage.wav", ST_WarningCallout, "WARNING: HEAT DAMAGE!", true);  // OK to force this because it will not get called each frame
        }
        else  // no new damage (yet!)
        {
            ShowWarning("Warning airframe overheating.wav", ST_WarningCallout, "WARNING: AIRFRAME OVERHEATING!");  // can't force this because we're in a post-step and the sound would never get to play since this gets call for each frame
        }
    }
    else    // let's check for a warning 
    {
        double criticalK = m_hullTemperatureLimits.criticalFrac * limitK;
        if (tempK >= criticalK)
        {
            m_warningLights[static_cast<int>(WarningLight::wlHtmp)] |= true;       // turn hull temp light ON
            ShowWarning("Warning Hull Temperature.wav", ST_WarningCallout, "WARNING: HULL TEMP. CRITICAL!");  // can't force this because we're in a post-step and the sound would never get to play since this gets call for each frame
        }
    }

    return retVal;  // remaining fraction is fraction over max heat; e.g., 0.2 = 20% over max heat value
}

// Fail a door if dynamic pressure exceeds limits, or issue a warning if a door is 
// open and dynamic pressure is high enough, if heating == 25% of failure heat level.
// Returns: true if door FAILED, false otherwise
bool DeltaGliderXR1::CheckDoorFailure(DoorStatus* doorStatus)
{
    bool retVal = false;        // assume no damage

    // do not re-check or warn if door already failed
    bool doorOpen = ((*doorStatus != DoorStatus::DOOR_CLOSED) && (*doorStatus != DoorStatus::DOOR_FAILED));
    const double dt = oapiGetSimStep();

    if (doorOpen)
    {
        // Door is open!  Check for damage or failure.
        // NOTE: once a door fails, it can only be repaired via the damage dialog; therefore, we never reset it here
        double dynP = GetDynPressure();
        if (doorStatus == &nose_status)
        {
            const double doorProc = nose_proc;
            if (IS_DOOR_FAILURE(m_noseconeTemp, OPEN_NOSECONE_LIMIT))
            {
                char msg[128];
                sprintf(msg, "%s FAILED due to excessive&heat and/or dynamic pressure!", NOSECONE_LABEL);
                ShowWarning("Warning Nosecone Failure.wav", ST_WarningCallout, msg, true); // OK to force this here
                *doorStatus = DoorStatus::DOOR_FAILED;
                m_warningLights[static_cast<int>(WarningLight::wlNose)] = true;
                FailDoor(nose_proc, anim_nose);
                retVal = true;   // new damage
            }
            else if (IS_DOOR_WARNING(m_noseconeTemp, OPEN_NOSECONE_LIMIT))     // this will check temperature limit for hover doors as well
            {
                char msg[128];
                sprintf(msg, "%s is open:&close it or reduce speed!", NOSECONE_LABEL);
                ShowWarning(WARNING_NOSECONE_OPEN_WAV, ST_WarningCallout, msg);
                m_warningLights[static_cast<int>(WarningLight::wlNose)] = true;
            }
            else if (IS_DOOR_FAILED() == false)
                m_warningLights[static_cast<int>(WarningLight::wlNose)] = false;   // reset light
        }
        if (doorStatus == &hoverdoor_status)
        {
            // NOTE: the hover doors cannot fail due to dynamic pressure, so we only check them for temperature here via DOOR_WARNING
            if ((IS_DOOR_FAILED() == false) && (OPEN_DOOR_WARN_TEMP(m_noseconeTemp)))
            {
                ShowWarning("Warning Hover Doors Open.wav", ST_WarningCallout, "Hover doors are open:&close them or reduce speed!");
                // no warning light for hover doors since they can't be damaged for now
            }
        }
        else if (doorStatus == &rcover_status)
        {
            const double doorProc = rcover_proc;

            if (IS_DOOR_FAILURE(m_leftWingTemp, RETRO_DOOR_LIMIT) ||
                IS_DOOR_FAILURE(m_rightWingTemp, RETRO_DOOR_LIMIT))
            {
                ShowWarning("Warning Retro Door Failure.wav", ST_WarningCallout, "Retro Doors FAILED due to excessive&heat and/or dynamic pressure!", true); // force this
                *doorStatus = DoorStatus::DOOR_FAILED;
                m_warningLights[static_cast<int>(WarningLight::wlRdor)] = true;
                FailDoor(rcover_proc, anim_rcover);
                retVal = true;   // new damage
            }
            else if (IS_DOOR_WARNING(m_leftWingTemp, RETRO_DOOR_LIMIT) ||
                IS_DOOR_WARNING(m_rightWingTemp, RETRO_DOOR_LIMIT))
            {
                ShowWarning("Warning Retro Doors Open.wav", ST_WarningCallout, "Retro Doors are open:&close them or reduce speed!");
                m_warningLights[static_cast<int>(WarningLight::wlRdor)] = true;
            }
            else if (IS_DOOR_FAILED() == false)
                m_warningLights[static_cast<int>(WarningLight::wlRdor)] = false;   // reset light
        }
        else if (doorStatus == &hatch_status)
        {
            const double doorProc = hatch_proc;

            if (IS_DOOR_FAILURE(m_cockpitTemp, HATCH_OPEN_LIMIT))
            {
                ShowWarning("Warning Hatch Failure.wav", ST_WarningCallout, "Top Hatch FAILED due to excessive&heat and/or dynamic pressure!", true); // force this
                *doorStatus = DoorStatus::DOOR_FAILED;
                m_warningLights[static_cast<int>(WarningLight::wlHtch)] = true;
                FailDoor(hatch_proc, anim_hatch);
                retVal = true;   // new damage
            }
            else if (IS_DOOR_WARNING(m_cockpitTemp, HATCH_OPEN_LIMIT))
            {
                ShowWarning("Warning Hatch Open.wav", ST_WarningCallout, "Top Hatch is open:&close it or reduce speed!");
                m_warningLights[static_cast<int>(WarningLight::wlHtch)] = true;
            }
            else if (IS_DOOR_FAILED() == false)
                m_warningLights[static_cast<int>(WarningLight::wlHtch)] = false;   // reset light
        }
        else if (doorStatus == &radiator_status)
        {
            const double doorProc = radiator_proc;
            if (IS_DOOR_FAILURE(m_topHullTemp, RADIATOR_LIMIT))
            {
                ShowWarning("Warning Radiator Failure.wav", ST_WarningCallout, "Radiator FAILED due to excessive&heat and/or dynamic pressure!", true); // force this
                *doorStatus = DoorStatus::DOOR_FAILED;
                m_warningLights[static_cast<int>(WarningLight::wlRad)] = true;
                FailDoor(radiator_proc, anim_radiator);
                retVal = true;   // new damage
            }
            else if (IS_DOOR_WARNING(m_topHullTemp, RADIATOR_LIMIT))
            {
                ShowWarning("Warning Radiator Deployed.wav", ST_WarningCallout, "Radiator is deployed:&stow it or reduce speed!");
                m_warningLights[static_cast<int>(WarningLight::wlRad)] = true;
            }
            else if (IS_DOOR_FAILED() == false)
                m_warningLights[static_cast<int>(WarningLight::wlRad)] = false;   // reset light
        }
        else if (doorStatus == &gear_status)
        {
            const double doorProc = gear_proc;

            // use nosecone temps to check gear-down damage
            if (IS_DOOR_FAILURE(m_noseconeTemp, GEAR_LIMIT))
            {
                ShowWarning("Warning Gear Failure.wav", ST_WarningCallout, "Landing Gear FAILED due to excessive&heat and/or dynamic pressure!", true); // force this
                *doorStatus = DoorStatus::DOOR_FAILED;
                FailGear(true);     // also invoke FailDoor to show gear partially collapsed
                m_warningLights[static_cast<int>(WarningLight::wlGear)] = true;
                retVal = true;   // new damage
            }
            else if (IS_DOOR_WARNING(m_noseconeTemp, GEAR_LIMIT))
            {
                ShowWarning("Warning Gear Deployed.wav", ST_WarningCallout, "Gear is deployed:&retract it or reduce speed!");
                m_warningLights[static_cast<int>(WarningLight::wlGear)] = true;
            }
            else if (IS_DOOR_FAILED() == false)
                m_warningLights[static_cast<int>(WarningLight::wlGear)] = false;   // reset light
        }
    }
    else if (IS_DOOR_FAILED() == false)
    {
        // door is closed; reset the warning light
        if (doorStatus == &nose_status)
            m_warningLights[static_cast<int>(WarningLight::wlNose)] = false;
        else if (doorStatus == &rcover_status)
            m_warningLights[static_cast<int>(WarningLight::wlRdor)] = false;
        else if (doorStatus == &hatch_status)
            m_warningLights[static_cast<int>(WarningLight::wlHtch)] = false;
        else if (doorStatus == &radiator_status)
            m_warningLights[static_cast<int>(WarningLight::wlRad)] = false;
        else if (doorStatus == &gear_status)
            m_warningLights[static_cast<int>(WarningLight::wlGear)] = false;
    }

    return retVal;
}

// check SCRAM ENGINE temperature and issue warning if necessary
// returns 0 if OK, > 0 = % max temperature exceeded^2 * 2; e.g., 0.42 = 10% over limit
double DeltaGliderXR1::CheckScramTemperature(double tempK, double limitK)
{
    double retVal = 0;

    // turn on SCRAM light if engines over-temp or if either is damaged; otherwise, turn it off
    m_warningLights[static_cast<int>(WarningLight::wlScrm)] = ((tempK > limitK) || (ramjet->GetEngineIntegrity(0) < 1.0) || (ramjet->GetEngineIntegrity(1) < 1.0));

    // check for over-limit
    if (tempK > limitK)
    {
        // fail the engines if necessary
        const double dt = oapiGetSimStep();     // # of seconds since last timestep
        const double exceededLimitMult = pow((tempK / limitK), 2);  // e.g. 1.21 = 10% over limit

        // # of seconds at this temp / average engine failure interval (8 secs)
        double failureTimeFrac = dt / 8.0;
        double failureProbability = failureTimeFrac * exceededLimitMult;
        // sprintf(oapiDebugString(), "SCRAM Damage failureProbablity=%lf", failureProbability);

        if (oapiRand() <= failureProbability)
        {
            retVal = ((exceededLimitMult - 1.0) * 2); // e.g., 0.42 = 10% over limit
            ShowWarning("Warning SCRAM Engine Damage.wav", ST_WarningCallout, "WARNING: SCRAM ENGINE HEAT&DAMAGE! CLOSE THE SCRAM DOORS!", true);  // OK to force this because it will not get called each frame
        }
        else  // no new damage (yet!)
        {
            ShowWarning("Warning SCRAM Engines Overheating.wav", ST_WarningCallout, "WARNING: SCRAM TEMPERATURE&CRITICAL! CLOSE THE SCRAM DOORS!"); // can't force this because we're in a post-step and the sound would never get to play since this gets call for each frame
        }
    }
    else    // let's check for a warning 
    {
        // check for temps approaching limits
        if ((tempK / limitK) > 0.97)
            ShowWarning("Warning SCRAM Temperature.wav", ST_WarningCallout, "SCRAM engines approaching limit!&Close the SCRAM doors!");
        else    // let's check for special case where mach >= n and temperature is greater than ambient (need to signal the pilot ASAP during reentry)
        {
            const double extTemp = GetExternalTemperature();
            const double mach = GetMachNumber();
            // only play warning is SCRAM throttle is CLOSED
            const double throttleLevelX2 = GetThrusterLevel(th_scram[0]) + GetThrusterLevel(th_scram[1]);
            if ((throttleLevelX2 == 0.0) && (tempK > extTemp) && (mach >= MACH_REENTRY_WARNING_THRESHOLD) && ((scramdoor_status != DoorStatus::DOOR_CLOSED) && (scramdoor_status != DoorStatus::DOOR_CLOSING)))
            {
                ShowWarning("Warning SCRAM doors open.wav", ST_WarningCallout, "WARNING: SCRAM DOORS OPEN!");
            }
        }
    }

    return retVal;  // remaining fraction is fraction over max heat; e.g., 0.2 = 20% over max heat value
}
