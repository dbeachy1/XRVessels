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
// XR1PrePostSteps.cpp
// Class defining custom clbkPostStep callbacks for the DG-XR1
// ==============================================================

#include "XR1PostSteps.h"
#include "AreaIDs.h"
#include "XRPayloadBay.h"   // not used by the XR1

//---------------------------------------------------------------------------

// Compute acceleration values for all three axes and store it in our parent vessel object.
// This is relatively expensive, so it is only performed once per frame for efficiency.
ComputeAccPostStep::ComputeAccPostStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel),
    m_gaugeScaleExpiration(-1), m_peakAccOnCurrentGaugeScale(0), m_activeGaugeScale(NONE)
{
    // init to zero
    VECTOR3 &A = GetXR1().m_acceleration;
    A.x = A.y = A.z = 0;
}

void ComputeAccPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    VECTOR3 &F = GetXR1().m_F;
    VECTOR3 &W = GetXR1().m_W;
    GetVessel().GetForceVector(F);
    GetVessel().GetWeightVector(W);
    
    // ORBITER BUG: if docked, Orbiter shows ~1/2 G along all three axes
    // the workaround is to set acc to zero if docked
    VECTOR3 acc;
    if (GetXR1().IsDocked())
        acc.x = acc.y = acc.z = 0;    
    else
        acc = GetXR1().m_acceleration = (F-W) / GetVessel().GetMass();

    const static double GAUGE_EXPIRATION_TIME = 3.0;      // minimum of 3 seconds before gauge scale is lowered

    // compute the max acc (positive or negative) along any axis
    double accX = fabs(acc.x);
    double accY = fabs(acc.y);
    double accZ = fabs(acc.z);
    double maxAcc = fabs(max(accX, max(accY, accZ))) / G;  // convert m/s to Gs

    // Gauge scale can be increased at any time; however, it may only be reset to a lower
    // scale after a set timeout period.  This will prevent "trashing" back and forth.
    // We allow a 5% cushion.  
    // NOTE: if the vessel is still in contact with the ground, lock the scale to TwoG since sometimes 
    // the G "bouncing" during roll can jump it to 4G, which is pointless.
    if (GetVessel().GroundContact() ||
        (maxAcc > (GetXR1().m_maxGaugeAcc * 1.05)) ||   // has maxAcc exceeded current gauge by 5%?
        (simt >= m_gaugeScaleExpiration))   // OK to lower gauge scale if necessary?
    {
        // rescale the panel based on the highest G received on the current gauge since it was last expired
        AccScale as;
        if (m_peakAccOnCurrentGaugeScale <= 2.1)   // 5% cushion
        {
            as = TwoG;
            GetXR1().m_maxGaugeAcc = 2.0;
        }
        else if (m_peakAccOnCurrentGaugeScale <= 4.2)
        {
            as = FourG;
            GetXR1().m_maxGaugeAcc = 4.0;
        }
        else
        {
            as = EightG;
            GetXR1().m_maxGaugeAcc = 8.0;
        }

        m_gaugeScaleExpiration = simt + GAUGE_EXPIRATION_TIME; // reset
        m_peakAccOnCurrentGaugeScale = -1;  // force it to be set below
        
        GetXR1().m_accScale = as;   // everybody will use the new scale
    }

    // update peak Acc for this gauge scale
    if (maxAcc > m_peakAccOnCurrentGaugeScale)
        m_peakAccOnCurrentGaugeScale = maxAcc;      // set abs value
}

//---------------------------------------------------------------------------

// notify the pilot by flashing warning lights, printing warning or info messages, and playing
// warning and info wav files
ShowWarningPostStep::ShowWarningPostStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel),
    m_performedStartupCheck(false), m_minimumRepeatSimt(0), m_warningSoundPlayingPreviousStep(false)
{
    *m_lastWarningWavFilename = 0;
}

void ShowWarningPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // if crashed, don't play any more warnings (but DO play if incapacitated)
    if (GetXR1().IsCrashed())
        return;

    // check whether a warning wav file is playing
    bool warningWavPlayingNow = GetXR1().IsPlaying(GetXR1().Warning);

    // check whether the warning wav just STOPPED playing
    if ((m_warningSoundPlayingPreviousStep == true) && (warningWavPlayingNow == false))
    {
        // if the same WAV is already queued up, wait a minimum time before playing it; otherwise it can play immediately
        if (*GetXR1().m_warningWavFilename != 0)
        {
            // determine whether the queued message is the same as the one that just finished playing
            bool isRepeat = (strcmp(GetXR1().m_warningWavFilename, m_lastWarningWavFilename) == 0);
            if (isRepeat)
                m_minimumRepeatSimt = simt + 3.0;   // wait 3 seconds after the previous warning ends before playing the duplicate message
        }
    }

    // no need to stop previous warning sound here; we are replaying in the same slot anyway, which will stop the current sound
    if (*GetXR1().m_warningWavFilename != 0)  // check whether a new warning sound has been set
    {
        // determine whether this is a repeat message of the last one played
        bool isRepeat = (strcmp(GetXR1().m_warningWavFilename, m_lastWarningWavFilename) == 0);

        // do not interrupt a playing warning UNLESS this was forced
        if (GetXR1().m_forceWarning || (warningWavPlayingNow == false))
        {
            // warning wav file is NOT playing; play it now 1) if it was forced, 2) if it is NOT a repeat, or 3) if it is a repeat but sufficient time has elapsed
            if (GetXR1().m_forceWarning || (isRepeat == false) || (simt > m_minimumRepeatSimt))
            {
                // play the new warning sound
                GetXR1().LoadXR1Sound(GetXR1().Warning, GetXR1().m_warningWavFilename, XRSound::PlaybackType::Radio);
                GetXR1().PlaySound(GetXR1().Warning, DeltaGliderXR1::ST_WarningCallout, GetXR1().m_warningWaveSoundType);

                strcpy(m_lastWarningWavFilename, GetXR1().m_warningWavFilename);   // remember that we played this warning
                *GetXR1().m_warningWavFilename = 0;   // reset WAV play request (we played it now)
                GetXR1().m_warningWaveSoundType = DeltaGliderXR1::ST_Other;    // will be set again later anyway, but reset it now anyway
            }
        }

        // If the active warning file is a repeat, do NOT leave it queued up; we don't ever want to play a "late" warning.  
        // For example, if the dynp light goes out we don't want to play a "Dynamic Pressure" warning again a few seconds later.  If the 
        // warning is still active after the current warning ends and we reach m_minimumRepeatSimt, the warning will play then.
        // Note, however, that we DO want to queue up a DIFFERENT warning to play if one occurs; therefore, we only reset the wave request
        // here if it is a repeat.
        if (isRepeat)
        {
            *GetXR1().m_warningWavFilename = 0;   // prevent "late playing" of a warning message
            GetXR1().m_warningWaveSoundType = DeltaGliderXR1::ST_Other;
        }
        
        GetXR1().m_forceWarning = false;    // reset force flag
    }

    // save warning play status for next step
    m_warningSoundPlayingPreviousStep = warningWavPlayingNow;

    // If we just started up, check for damage; if present, activate MWS and print a warning.
    // We must do this here in order for the greeting sound to play; it cannot play before the main loop begins.
    // ALSO, we need to wait at least one frame so that GroundContact() can return true.  We wait 1 second.
    if ((simt > 1.0) && (m_performedStartupCheck == false))
    {
        m_performedStartupCheck = true;
        if (GetXR1().m_crewState == DEAD)
        {
            // no audio
            GetXR1().ShowWarning(NULL, DeltaGliderXR1::ST_None, "CREW IS DEAD!");
        }
        else if (GetXR1().GetCrewMembersCount() == 0)
        {
            // no audio
            GetXR1().ShowWarning(NULL, DeltaGliderXR1::ST_None, "NO CREW ON BOARD!");
        }
        else if (GetXR1().IsCrewIncapacitated())
        {
            // no audio
            GetXR1().ShowWarning(NULL, DeltaGliderXR1::ST_None, "CREW IS UNCONSCIOUS!");
        }
        else if (!GetXR1().IsPilotOnBoard())
        {
            // no audio
            GetXR1().ShowWarning(NULL, DeltaGliderXR1::ST_None, "NO PILOT ON BOARD!");
        }
        else if (GetXR1().IsDamagePresent())
        {
            GetXR1().m_MWSActive = true;
            GetXR1().ShowWarning("Warning Ship Damage Detected.wav", DeltaGliderXR1::ST_WarningCallout, "Ship damage detected!&Check MDA status screens.");
        }
        else if (GetXR1().IsWarningPresent())  // NOTE: does *not* include scenario parse warnings: this is ship warnings only
        {
            GetXR1().m_MWSActive = true;
            GetXR1().ShowWarning("Warning Conditions Detected.wav", DeltaGliderXR1::ST_WarningCallout, "Warning condition(s) detected!&Check MWS lights.");
        }
        else  // no ship damage or warnings
        {
            // only show the "all systems nominal" if the parsing also succeeded
            if (!GetXR1().GetXR1Config()->ParseFailed())
            {
                // only use "welcome aboard" if ship is grounded or docked
				// DEBUG: sprintf(oapiDebugString(), "GroundContact()=%d, airspeed=%lf, groundspeed=%lf", GetVessel().GroundContact(), GetXR1().GetAirspeed(), GetXR1().GetGroundspeed());
				// NOTE: because of a glitch (or feature?) of the Orbiter 2016 core on startup, the ship often has a ~0.3 meter-per-second ground speed when the ship first loads, 
				//       so we have to account for that by only checking if with a hack here by checking if the parking brake is enabled (i.e., was the ship stopped when the scenario was saved?)
                bool showWelcome = (GetXR1().IsLanded() || GetXR1().IsDocked() || GetXR1().m_parkingBrakesEngaged);
                if (showWelcome) 
                    GetXR1().ShowInfo(WELCOME_ABOARD_ALL_SYSTEMS_NOMINAL_WAV, DeltaGliderXR1::ST_AudioStatusGreeting, WELCOME_MSG);
                else    
                    GetXR1().ShowInfo(ALL_SYSTEMS_NOMINAL_WAV, DeltaGliderXR1::ST_AudioStatusGreeting, ALL_SYSTEMS_NOMINAL_MSG);
            }
        }
    }
}

//---------------------------------------------------------------------------

SetHullTempsPostStep::SetHullTempsPostStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel),
    m_forceTempUpdate(true) // force update on first frame through to init hull temps
{
}

void SetHullTempsPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    AddHeat(simdt);
    RemoveHeat(simdt);
    UpdateHullHeatingMesh(simdt);  
}

void SetHullTempsPostStep::AddHeat(const double simdt)
{
    //
    // ADD HEAT if atmPressure is present
    //
    if (m_forceTempUpdate || GetXR1().IsOATValid())
    {
        const double atmPressure = GetVessel().GetAtmPressure();
        const double airspeed = GetVessel().GetAirspeed();   // check *airspeed* here, not ground speed

        // compute total heat to be added to the ship

        // NOTE: the hull temperatures displayed are too high at low-altitude subsonic flight to be realistic.
        // The root issue is the algorithm is scaling linearly with 0.5 * pressure * velocity^3, and that value is too high at low altitude / high mach.
        // The only way to fix that is to compute the true air density as it relates to OAT in kelvin and the specific gas constant of air, and I haven't figure out how to do that yet.

        // tweakedAtmPressure called leftWingHeatFrac in ASM code
        // DG3: double tweakedAtmPressure = atmPressure;       // atmospheric pressure in pascals
        // Orbiter forumla: density / 2 * v^3
        // Note: messing with air density is a bitch, so we cheat and just use pressure here.
        double tweakedAtmPressure = atmPressure / 2;       // atmospheric pressure in pascals

        // NOTE: this was modified because the hull temperature was too high at low
        // altitudes (our boundary layer was artifically reducing hull temperatures at high velocities).
        // The boundary insulation layer been removed to keep the hull temp lower across the board and the workingHullHeatingFactor
        // was modified to keep the same target max hull temperature.  This tweak should not require any changes to the subclasses.
        // DG3: const double tweakedAirspeed = pow(airspeed, 3) * 1.5;
        // ORG prior to the XR2: const double tweakedAirspeed = (airspeed*airspeed*airspeed);  // use explicit mult for efficiency
        const double workingHullHeatingFactor = HULL_HEATING_FACTOR * 0.642; // tweaked very carefully...
        const double tweakedAirspeed = (airspeed*airspeed*airspeed);  
        
        const double speedTimesPressure = tweakedAirspeed * tweakedAtmPressure; 

        // NOTE: DO NOT SCALE THIS WITH simdt!  We are calculating an ABSOLUTE temperature, not DELTA temperature
        // this is the amount of heat to be added to the leading edges of the ship.
        // DG3: double degreesK = speedTimesPressure * 1.694915180695206e-10;    
        // ORG: double degreesK = speedTimesPressure * 1.8e-10;
        // NEW value for boundary layer heating support
        double degreesK = speedTimesPressure * workingHullHeatingFactor;
        // DEBUG: sprintf(oapiDebugString(), "degreesK = %lf", degreesK);

        /*
            New for XR1 1.11 / XR2 1.6 / XR5 1.9: add heat transfer by conduction logic to bleed heat away from the 
            ship's hull based on atmospheric density (static pressure).
        
            heatConductionFraction denotes the fraction of normally-computed heat added to the hull after some fraction is conducted away by the atmosphere rushing over it; depends on static pressure only.
    
            Max heat reduction: heatConductionFraction = 0.0949622 for static pressure 97700 pascals  (1000 ft altitude on Earth)
            Min heat reduction: heatConductionFraction = 1.0       for static pressure  7000 pascals  (~18 km altitude on Earth)
        */
        double heatConductionFraction = 1.0;      // assume no conductive cooling
        const double minHeatConductionPressure = 7000;      // below this pressure no heat conduction cooling occurs: this pressure is ~18 km above sea level on Earth
        const double maxHeatConductionPressure = 97700;     // above this pressure no *additional* heat conduction cooling occurs: this pressure is 1000 ft above sea level on Earth
        const double minHeatConductionFraction = 0.0949622; // carefully set so that our just-subsonic flight at just above sea level comes out to ~40 C above OAT per http://www.aerospaceweb.org/design/scripts/atmosphere/

        if (atmPressure > minHeatConductionPressure)   // is ATM pressure above the minimum where conductive cooling occurs?
        {
            const double maxHeatConductionFraction = 1.0 - minHeatConductionFraction;  // the max fraction of heat that is dropped due to conduction
            double heatConductionPower = ((atmPressure - minHeatConductionPressure)  / (maxHeatConductionPressure - minHeatConductionPressure));  // from 0.0 to 1.0 (higher number = more heat dropped due to conduction)
            if (heatConductionPower > 1.0)   
                heatConductionPower = 1.0;   // never add extra heat if atmPressure > maxHeatConductionPressure
            _ASSERTE(heatConductionPower >= 0.0);

            heatConductionFraction = 1.0 - (maxHeatConductionFraction * heatConductionPower);
            // cannot use due to tiny rounding error at boundary: _ASSERTE(heatConductionFraction >= minHeatConductionFraction);

            // DEBUG: sprintf(oapiDebugString(), "heatConductionFraction = %lf, heatConductionPower = %lf, normal degreesK = %lf, adjusted degreesK = %lf", heatConductionFraction, heatConductionPower, degreesK, (degreesK * heatConductionFraction));
        }

        degreesK *= heatConductionFraction;

#if 0 // NO BOUNDARY LAYER SINCE XR2
        // now adjust for boundary layer insulation
        // NOTE: we had to tweak this to fix low-altitude heating but still have upper-altitude heating be OK
        // ORG before the XR2: const double boundaryInsulatorFrac = (1600.0 / 5500.0);  // hull temp vs. freestream temp for the shuttle @ maximum heating
        const double boundaryInsulatorFrac = (5500.0 / 5500.0);  // hull temp vs. freestream temp for the shuttle @ maximum heating

        const double boundaryInsulatorSpeed = 7800;   // m/s     maximum shuttle heating
        const double boundaryLayerLowerLimit = 514;   // m/s  mach 1.5

        double heatFrac = 1.0;                 // assume no boundary layer
        
        if (airspeed > boundaryLayerLowerLimit)
        {
            // effectiveness is determined by speed over boundary's lower limit
            // at half the speed (over the lower limit), twice the heat gets through
            // 0 < n < 1.0 : how effective is the boundary layer insulator?
            double boundaryEffectivenessFrac = min( ((airspeed - boundaryLayerLowerLimit) / (boundaryInsulatorSpeed - boundaryLayerLowerLimit)), 1.0 );  

            // heat fraction reaching the hull = minHeatFrac@max insulation + (fraction between minHeatFrac and 1.0 based on effectiveness)
            heatFrac = boundaryInsulatorFrac + ((1.0-boundaryInsulatorFrac) * (1.0 - boundaryEffectivenessFrac));  // e.g., 0.29 + (0.71 * 0.5) = 0.29 + 0.355 = 0.645
            // sprintf(oapiDebugString(), "boundaryEffectivenessFrac=%.4lf, heatFrac=%.4lf", boundaryEffectivenessFrac, heatFrac);
        }

        // boundaryInsulatorFrac < heatFrac <= 1.0
        degreesK *= heatFrac;   // hull heating reduced by boundary layer
#endif

        //
        // Add heat if there is any to add OR if this is the first frame since we loaded
        //
        // Note: degreesK should never be < 0 here since neither velocity nor pressure can go negative.  It can, however be zero.
        if (m_forceTempUpdate || (degreesK > 0.0))
        {
            const double extTemp = GetXR1().GetExternalTemperature();
            const double slipAngle = GetVessel().GetSlipAngle();
            const double altitude = GetVessel().GetAltitude(ALTMODE_GROUND);
            const double aoa = GetVessel().GetAOA();

            // NOSECONE
            // since we have TWO factors affecting the nosecone, cut each effect into pieces
            
            double noseconeSlipHeatFrac;
            double noseconeAOAHeatFrac;
            // TODO: Orbiter sets slip to be 0.0 whether ship is pointing forward or backward: find a way to determine this
            if (fabs(slipAngle) <= 90.0)    // going FORWARD?
            {
                // the smaller the slip, the HIGHER the heat
                // changing slip has 1/5 effect of sine angle change
                noseconeSlipHeatFrac = 1.0 - (sin(fabs(slipAngle)) / 5 / 2);

                // changing AOA has 1/3 effect of sine angle change
                noseconeAOAHeatFrac = 1.0 - (sin(fabs(aoa)) / 3 / 2);
            }
            else   // going BACKWARDS
            {
                // the smaller the slip, the LOWER the heat
                noseconeSlipHeatFrac = (sin(fabs(slipAngle)) / 5 / 2);
                noseconeAOAHeatFrac = (sin(fabs(aoa)) / 3 / 2);
            }

            // no need to check for fractions > 1.0 here since the sine of a positive number is always positive
            // now combine both fractions to get the overall fraction
            double noseconeHeatFrac = noseconeSlipHeatFrac * noseconeAOAHeatFrac;

            // WINGS
            // no need to reduce angles here; if slip > 90 degrees left or 
            // to reduce heat for right wing, slip must be POSITIVE, meaning positive slip == right turn
            // Minimum heat is is 10% of total wing heat.
            double rightWingHeatFrac = (1.0 - (sin(slipAngle) * 0.9));

            // to reduce heat for left wing, slip must be NEGATIVE, meaning negative slip == left turn
            // Minimum heat is is 10% of total wing heat.
            double leftWingHeatFrac = (1.0 - (sin(-slipAngle) * 0.9));

            // heating factor can never exceed total heat on the leading edge, so cap it at 1.0
            if (leftWingHeatFrac > 1.0)
                leftWingHeatFrac = 1.0;

            if (rightWingHeatFrac > 1.0)
                rightWingHeatFrac = 1.0;      

            // COCKPIT
            double cockpitHeatFrac = (1.0 - sin(aoa));

            // cap it at 1.20 (in case the pilot pitches down), in which case the cockpit can get as hot as the nose
            if (cockpitHeatFrac > 1.20)
                cockpitHeatFrac = 1.20;

            // NOSECONE
            // NOTE: this variable is reused for each surface
            double newTemp = extTemp + (noseconeHeatFrac * degreesK);

            // Don't ever LOWER the nosecone temp in the "add heat" phase here
            if (newTemp > GetXR1().m_noseconeTemp)
                GetXR1().m_noseconeTemp = newTemp;

            // LEFT WING
            newTemp = extTemp + ((leftWingHeatFrac * degreesK) * 0.75);  // nose gets 25% hotter than wings
            if (newTemp > GetXR1().m_leftWingTemp)
                GetXR1().m_leftWingTemp = newTemp;

            // RIGHT WING
            newTemp = extTemp + ((rightWingHeatFrac * degreesK) * 0.75);
            if (newTemp > GetXR1().m_rightWingTemp)
                GetXR1().m_rightWingTemp = newTemp;

            // COCKPIT
            double cockpitDeltaTemp = (cockpitHeatFrac * degreesK) * .73;  // nose gets 27% hotter than cockpit (max)
            newTemp = extTemp + cockpitDeltaTemp;
            if (newTemp > GetXR1().m_cockpitTemp)
                GetXR1().m_cockpitTemp = newTemp;

            // TOP HULL
            // top hull gets 80% of the heat that the cockpit does
            newTemp = extTemp + (cockpitDeltaTemp * 0.80);
            if (newTemp > GetXR1().m_topHullTemp)
            GetXR1().m_topHullTemp = newTemp;
        }
    }
    m_forceTempUpdate = false;      // reset
}

void SetHullTempsPostStep::RemoveHeat(const double simdt)
{
    // heat dissipation rates are the same for each surface
    RemoveSurfaceHeat(simdt, GetXR1().m_noseconeTemp);
    RemoveSurfaceHeat(simdt, GetXR1().m_leftWingTemp);
    RemoveSurfaceHeat(simdt, GetXR1().m_rightWingTemp);
    RemoveSurfaceHeat(simdt, GetXR1().m_cockpitTemp);
    RemoveSurfaceHeat(simdt, GetXR1().m_topHullTemp);
}

// remove heat from a single surface
// temp = temperature of surface
void SetHullTempsPostStep::RemoveSurfaceHeat(const double simdt, double &temp)
{
    const double extTemp = GetXR1().GetExternalTemperature();
    const double delta = fabs(temp - extTemp);

    // Each surface drops 2% or .1 degree of its heat ABOVE AMBIENT per second, whichever is greater
    double heatDropped = max((delta * .02), 0.1) * simdt;  // amount of heat dropped in this fraction of a second
    
    double newTemp = temp - heatDropped;
    if (newTemp > extTemp)
        temp = newTemp;
    else
        temp = extTemp;   // external temps reached ambient
}

// update the transparency of the hull heating mesh, if any
void SetHullTempsPostStep::UpdateHullHeatingMesh(const double simdt)
{
    if (!GetXR1().heatingmesh)
        return;     // no hull heating mesh

    // DEBUG: heating mesh testing: GetXR1().m_noseconeTemp = GetXR1().m_tweakedInternalValue;

    // We check temperature of the nosecone only; set the limits at which the mesh becomes barely visible
    // to where it is at its maximum opacity (maxHeatingAlpha).
    const double minVisibilityTemp = GetXR1().m_hullTemperatureLimits.noseCone * 0.387;  // coincides with Orbiter visual plasma
    const double maxVisibilityTemp = GetXR1().m_hullTemperatureLimits.noseCone * 0.80; 
    
    // Orbiter core bug: we should only modulate alpha when the heating mesh should
    // actually be *visible* because the Orbiter core applies the alpha setting to *all*
    // transparent meshes in the sim, including the Sun!  This makes the sun disappear.
    const bool bHeatingMeshVisible = (GetXR1().m_noseconeTemp >= minVisibilityTemp);
    GetXR1().SetMeshGroupVisible(GetXR1().heatingmesh, GetHeatingMeshGroupIndex(), bHeatingMeshVisible);  // show or hide the group
    oapiSetMeshProperty(GetXR1().heatingmesh, MESHPROPERTY_MODULATEMATALPHA, (DWORD)bHeatingMeshVisible); // use material alpha w/texture alpha

    if (bHeatingMeshVisible)
    {
        // hull heat is visible!  Update the alpha for the material.
        // get the fraction between minVisiblityTemp (0 and maxVisibilityTemp (1.0)
        double alphaFrac = ((GetXR1().m_noseconeTemp - minVisibilityTemp) / (maxVisibilityTemp - minVisibilityTemp));
        if (alphaFrac > 1.0)
            alphaFrac = 1.0;    // keep in range

        // now get the alpha to be applied
        // min heating alpha is 0.0
        // BETA-1 ORG: const double maxHeatingAlpha = 0.475;
        const double maxHeatingAlpha = 1.0;  // new heating mesh is 4-bit alpha
        const float heatingMeshAlpha = static_cast<float>(alphaFrac * maxHeatingAlpha);

        // read the original material from the *global* mesh and clone it, since we cannot read material from the active ship's mesh in Orbiter_ng
        const MATERIAL *pSrcHeatingMaterial = oapiMeshMaterial(GetXR1().heatingmesh_tpl, GetHeatingMeshGroupIndex());
        MATERIAL clonedMaterial;
        memcpy(&clonedMaterial, pSrcHeatingMaterial, sizeof(MATERIAL));  // make working copy
        
        // now set the new alpha in the working copy
        clonedMaterial.diffuse.a = heatingMeshAlpha;
        clonedMaterial.ambient.a = heatingMeshAlpha;
        clonedMaterial.specular.a = heatingMeshAlpha;
        clonedMaterial.emissive.a = heatingMeshAlpha;

        // apply the modified material to the heating mesh
        oapiSetMaterial(GetXR1().heatingmesh, GetHeatingMeshGroupIndex(), &clonedMaterial);
        
        // DEBUG: sprintf(oapiDebugString(), "NoseconeTemp: %.3lf heatingMeshAlpha=%f", GetXR1().m_noseconeTemp, heatingMeshAlpha);
    }
    else
    {
        // DEBUG: sprintf(oapiDebugString(), "NoseconeTemp: %.3lf : Heating mesh invisible", GetXR1().m_noseconeTemp);
    }
}

//---------------------------------------------------------------------------

// compute descent or ascent slope
SetSlopePostStep::SetSlopePostStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel), m_lastUpdateTime(0), m_lastUpdateAltitude(0),
    // OLD: m_refreshRate(0.10),     // update time in seconds; larger values are more stable but refresh at a lower rate
    m_refreshRate(0.0167),     // 60 fps OK now
    m_nextUpdateTime(0), m_isNextUpdateTimeValid(false)
{
    // 30 samples / 60 samples-per-second = average over the last 0.5-second
    const int sampleSize = 30;  
    m_pAltitudeDeltaRollingArray = new RollingArray(sampleSize);  // the altitude for the last n timesteps
    m_pDistanceRollingArray = new RollingArray(sampleSize);  // the distance traveled for the last n timesteps
}

// destructor
SetSlopePostStep::~SetSlopePostStep()
{
    delete m_pAltitudeDeltaRollingArray;
    delete m_pDistanceRollingArray;
}

void SetSlopePostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    if (GetXR1().GroundContact())
    {
        m_isNextUpdateTimeValid = false;      // reset
        GetXR1().m_slope = 0;       // no slope when on ground
    }
    else  // ship is airborne, so slope is valid
    {
        if (!m_isNextUpdateTimeValid)
            goto reset;  // enable slope calculation
    }

    // check if the sim date was moved *backward*: m_nextUpdateTime should never normally be > 0.10 second from current simt
    if (m_isNextUpdateTimeValid && (m_nextUpdateTime - simt) > 1.0)  // 0.10 would work, but 1.0 is fine (allow for changes to m_refreshRate sometime w/o having to change this, too)
        goto reset;     // reset for for 1/10th second from now with new data

    // NOTE: we don't want to add a sample every frame here because it would make the number of samples
    // over time vary, which would make accuracy (and lag) dependent on the framerate.  So we sync at 60 fps instead (see m_refreshRate value).
    if (m_isNextUpdateTimeValid && (simt >= m_nextUpdateTime))
    {
        double altitude = GetVessel().GetAltitude(ALTMODE_GROUND);
		double groundspeed = GetVessel().GetGroundspeed();

        const double timeDeltaSinceLastUpdate = simt - m_lastUpdateTime;
        m_pAltitudeDeltaRollingArray->AddSample(altitude - m_lastUpdateAltitude);       // altitude delta for this timestep
		m_pDistanceRollingArray->AddSample(groundspeed * timeDeltaSinceLastUpdate);   // distance traveled for this timestep

        // NOTE: the total sample size is very small until the data builds up, so the slope may be pretty far out for 
        // the first few frames, but that's OK.

        // update slope variables
        // compute triangle's 'a' leg (total altitude delta over for the last N timesteps)
        const double a = m_pAltitudeDeltaRollingArray->GetSum();

        // compute triangle's hypotenuse (distance traveled along velocity vector over the last N timesteps)
        const double c = m_pDistanceRollingArray->GetSum();  // total distance traveled over the last N frames

        // compute the triangle's 'b' leg (ground distance traveled)
        // b = sqrt( c^2 - a^2 )
        const double b = sqrt((c*c) - (a*a));

        // how we have the 'a' and 'b' legs; compute the slope angle
        // A = arctan(a / b)
        GetXR1().m_slope = atan(a / b);     // slope in radians
        // DEBUG: sprintf(oapiDebugString(), "a=%0.4lf, b=%0.4lf, c=%0.4lf, slope=%lf, velocity=%lf, timeDeltaSinceLastUpdate=%lf", a, b, c, GetXR1().m_slope * DEG, airspeed, timeDeltaSinceLastUpdate);

reset:
        // reset for next sample
        m_nextUpdateTime        = simt + m_refreshRate;
        m_lastUpdateTime        = simt;
        m_lastUpdateAltitude    = altitude;
        m_isNextUpdateTimeValid = true;
    }
}

//---------------------------------------------------------------------------

DoorSoundsPostStep::DoorSoundsPostStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel),
    m_prevChamberStatus(NOT_SET)
{
// set transition state processing to FALSE so we don't play an initial thump when a scenario loads
#define INIT_DOORSOUND(idx, doorStatus, xr1SoundID, label)   \
    m_doorSounds[idx].pDoorStatus = &(GetXR1().doorStatus);  \
    m_doorSounds[idx].prevDoorStatus = NOT_SET;              \
    m_doorSounds[idx].soundID = GetXR1().xr1SoundID;         \
    m_doorSounds[idx].processAPUTransitionState = false;     \
    m_doorSounds[idx].pLabel = label
    
    // initialize door sound structures for all doors
    INIT_DOORSOUND(0,  ladder_status,    dAirlockLadder, "Airlock Ladder");
    INIT_DOORSOUND(1,  nose_status,      dNosecone,      NOSECONE_LABEL);
    INIT_DOORSOUND(2,  olock_status,     dOuterDoor,     "Airlock Outer Door");
    INIT_DOORSOUND(3,  ilock_status,     dInnerDoor,     "Airlock Inner Door");
    INIT_DOORSOUND(4,  brake_status,     dAirbrake,      "Airbrake");
    INIT_DOORSOUND(5,  hatch_status,     dCabinHatch,    "Cabin Hatch");
    INIT_DOORSOUND(6,  radiator_status,  dRadiator,      "Radiator");
    INIT_DOORSOUND(7,  rcover_status,    dRetroDoors,    "Retro Doors");
    INIT_DOORSOUND(8,  hoverdoor_status, dHoverDoors,    "Hover Doors");
    INIT_DOORSOUND(9,  scramdoor_status, dScramDoors,    "SCRAM Doors");
}

void DoorSoundsPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // walk through all doors
    const int doorCount = (sizeof(m_doorSounds) / sizeof(DoorSound));
    for (int i=0; i < doorCount; i++)
        PlayDoorSound(m_doorSounds[i], simt);

    // MANUAL STEP: manage airlock chamber pressure sound
    const DoorStatus ds = GetXR1().chamber_status;
    const DeltaGliderXR1::Sound soundID = GetXR1().Chamber;

    // skip the first frame through here so we can initialize the previous gear status properly
    if (m_prevChamberStatus != NOT_SET)
    {
        // no hydraulic pressure required for this
        // check whether this "door" is in motion (i.e, is chamber pressurizing/depressurizing)
        if ((ds == DOOR_OPENING) || (ds == DOOR_CLOSING))
        {
            // if door sound is not playing, restart it since the door is moving again
            if (GetXR1().IsPlaying(soundID) == false)
            {
                // sound is already loaded
                GetXR1().PlaySound(soundID, DeltaGliderXR1::ST_Other, AIRLOCK_CHAMBER_VOLUME);
            }
        }

        if (ds != m_prevChamberStatus)
        {
            // door changed state
            if ((ds == DOOR_OPENING) || (ds == DOOR_CLOSING))
            {
                // no need to stop sound here; PlaySound will reset it
                // sound is loaded already
                GetXR1().PlaySound(soundID, DeltaGliderXR1::ST_Other, AIRLOCK_CHAMBER_VOLUME);
                if (ds == DOOR_OPENING)
                    GetXR1().ShowInfo("Airlock Depressurizing.wav", DeltaGliderXR1::ST_InformationCallout, "Airlock depressurizing.");
                else
                    GetXR1().ShowInfo("Airlock Pressurizing.wav", DeltaGliderXR1::ST_InformationCallout, "Airlock pressurizing.");
            }
            else    // door is either OPENED, CLOSED, or FAILED
            {
                GetXR1().StopSound(soundID);
                if (ds != DOOR_FAILED)
                {
                    // door finished shutting or opening; let's show a message and play a callout
                    if (ds == DOOR_OPEN)
                        GetXR1().ShowInfo("Airlock Pressure Zero PSI.wav", DeltaGliderXR1::ST_InformationCallout, "Airlock fully depressurized.");
                    else
                        GetXR1().ShowInfo("Airlock Pressure Fourteen Point Seven PSI.wav", DeltaGliderXR1::ST_InformationCallout, "Airlock pressure nominal.");
                }
                // chamber can never fail, so don't play a sound
            }
        }
    }
    m_prevChamberStatus = GetXR1().chamber_status;    // save for next frame
}

// play the sound for a single door
void DoorSoundsPostStep::PlayDoorSound(DoorSound &doorSound, const double simt)
{
    const DoorStatus ds = *doorSound.pDoorStatus;
    const DeltaGliderXR1::Sound soundID = doorSound.soundID;
    const bool apuOnline = GetXR1().CheckHydraulicPressure(false, false);

    // skip the first frame through here so we can initialize the previous door status properly
    if (doorSound.prevDoorStatus != NOT_SET)
    {
        // handle APU transition states
        if (!apuOnline)
        {
            // No hydraulic pressure!  Check whether this door is in motion.
            // Don't check this more than once.
            if (((ds == DOOR_OPENING) || (ds == DOOR_CLOSING)) && doorSound.processAPUTransitionState)
            {
                // play a thump since this door stopped abruptly
                // this will also kill any hydraulic sound in progress
                GetXR1().LoadXR1Sound(soundID, "Door Opened Thump.wav", XRSound::PlaybackType::InternalOnly);
                GetXR1().PlaySound(soundID, DeltaGliderXR1::ST_Other);
                doorSound.processAPUTransitionState = false;   // don't play the thump again until reset
            }
        }
        else  // hydraulic pressure OK
        {
            // check whether this door is in motion
            if ((ds == DOOR_OPENING) || (ds == DOOR_CLOSING))
            {
                // door requiring APU is active, so update our timestamp to reflect this
                GetXR1().MarkAPUActive();  // reset the APU idle warning callout time

                // if door (hydraulic) sound is not playing, restart it since the door is moving again
                if (GetXR1().IsPlaying(soundID) == false)
                {
                    GetXR1().LoadXR1Sound(soundID, "Hydraulics1.wav", XRSound::PlaybackType::InternalOnly);  // audible outside ship as well
                    GetXR1().PlaySound(soundID, DeltaGliderXR1::ST_Other);
                }
                doorSound.processAPUTransitionState = true;  // arm the system so we play a thump if the door stops due to APU shutdown
            }
        }

        if (apuOnline && (ds != doorSound.prevDoorStatus))   // APU online AND door just started moving is moving 
        {
            // door changed state
            if ((ds == DOOR_OPENING) || (ds == DOOR_CLOSING))
            {
                // no need to stop sound here; PlaySound will reset it
                GetXR1().LoadXR1Sound(soundID, "Hydraulics1.wav", XRSound::PlaybackType::InternalOnly);  // audible outside ship as well
                GetXR1().PlaySound(soundID, DeltaGliderXR1::ST_Other);
                ShowDoorInfoMsg(doorSound);       // show info message
            }
            else    // door is either OPENED, CLOSED, or FAILED
            {
                GetXR1().StopSound(soundID);
                if (ds != DOOR_FAILED)
                {
                    // door finished shutting or opening
                    GetXR1().LoadXR1Sound(soundID, "Door Opened Thump.wav", XRSound::PlaybackType::InternalOnly);
                    GetXR1().PlaySound(soundID, DeltaGliderXR1::ST_Other);
                }
                ShowDoorInfoMsg(doorSound);       // show info message
            }
        }
    }
    doorSound.prevDoorStatus = *doorSound.pDoorStatus;    // save for next frame
}

// show an info message for a door's change in status
// NOTE: this does NOT handle DOOR_FAILED events; those are logged manually at the point of failure
void DoorSoundsPostStep::ShowDoorInfoMsg(DoorSound doorSound)
{
    const char *pActionStr = nullptr;
    const DoorStatus action = *doorSound.pDoorStatus;

    if (action == DOOR_OPENING)
        pActionStr = "opening";
    else if (action == DOOR_CLOSING)
        pActionStr = "closing";
    else if (action == DOOR_OPEN)
        pActionStr = "open";
    else if (action == DOOR_CLOSED)
        pActionStr = "closed";

    if (pActionStr != nullptr)
    {
        char msg[60];
        sprintf(msg, "%s %s.", doorSound.pLabel, pActionStr);
        GetXR1().ShowInfo(NULL, DeltaGliderXR1::ST_None, msg);
    }
}

//---------------------------------------------------------------------------

UpdateIntervalTimersPostStep::UpdateIntervalTimersPostStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel)
{
}

void UpdateIntervalTimersPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    double daysDT = simdt / 86400;

    // update time for each of our interval timers
    if (GetXR1().m_interval1TimerRunning)
        GetXR1().m_interval1ElapsedTime += daysDT;

    if (GetXR1().m_interval2TimerRunning)
        GetXR1().m_interval2ElapsedTime += daysDT;
}

//---------------------------------------------------------------------------

UpdateCoolantTempPostStep::UpdateCoolantTempPostStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel),
    m_prevCoolantTemp(-1)
{
}

void UpdateCoolantTempPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // if crashed, nothing more to do
    if (GetXR1().IsCrashed())
        return;

    double coolantTemp = GetXR1().m_coolantTemp;
    const int heatingRateSetting = GetXR1().GetXR1Config()->CoolantHeatingRate;
    double heatingModifier = 1.0;   // assume no extra heating occurring
    
    // if APU is running, this generates 5% extra heat
    if ((GetXR1().apu_status == DOOR_OPEN) || (GetXR1().apu_status == DOOR_OPENING))
        heatingModifier += 0.05;

    // add heat
    coolantTemp += (COOLANT_HEATING_RATE[heatingRateSetting] * simdt * heatingModifier);

    // heat is capped at max temp
    if (coolantTemp > MAX_COOLANT_TEMP)
        coolantTemp = MAX_COOLANT_TEMP;

    // remove heat if radiator open
    if (GetXR1().radiator_status == DOOR_OPEN)
    {
        // cool at a percentage OR at a minimum rate, whichever is higher
        coolantTemp -= max(COOLANT_COOLING_RATE_FRAC * coolantTemp, COOLANT_COOLING_RATE_MIN) * simdt;
    }

    // remove heat if external cooling is flowing; this "stacks" with the radiator as well
    if (GetXR1().m_isExternalCoolantFlowing)
    {
        // cool at a percentage OR at a minimum rate, whichever is higher
        // NOTE: ground cooling is 27% more efficient than radiators, so effective total cooling with both active is 127% of normal.
        coolantTemp -= max((COOLANT_COOLING_RATE_FRAC * 1.27) * coolantTemp, COOLANT_COOLING_RATE_MIN) * simdt;
    }

    // do not drop below nominal
    if (coolantTemp < NOMINAL_COOLANT_TEMP)
        coolantTemp = NOMINAL_COOLANT_TEMP;

    // check for warnings or failure
    if (coolantTemp >= CRITICAL_COOLANT_TEMP)
    {
        if (GetXR1().m_internalSystemsFailure)
        {
            GetXR1().m_MWSActive = true;    // keep warning light blinking
            GetXR1().m_warningLights[wlCool] = true;    // in case we just reloaded the scenario
        }
        else    // not failed yet
        {
            GetXR1().ShowWarning("Warning Systems Overheating.wav", DeltaGliderXR1::ST_WarningCallout, "WARNING: coolant temperature critical!");

            const double dt = oapiGetSimStep();     // # of seconds since last timestep
            double exceededLimitMult = pow((coolantTemp / CRITICAL_COOLANT_TEMP), 2);  // e.g. 1.21 = 10% over limit

            // # of seconds at this temp / average terminal failure interval (20 secs)
            double failureTimeFrac = dt / 20.0;
            double failureProbability = failureTimeFrac * exceededLimitMult;

            if (oapiRand() <= failureProbability)
            {
                GetXR1().m_internalSystemsFailure = true;   // systems offline
                GetXR1().m_MWSActive = true;
                GetXR1().ShowWarning("Warning Systems Failure Oxygen Flow Offline.wav", DeltaGliderXR1::ST_WarningCallout, "WARNING: SYSTEMS FAILURE!&Environmental systems offline;&DEPLOY THE RADIATOR!", true);  // force this
            }

            GetXR1().m_warningLights[wlCool] = true;

            // if this just occurred, activate MWS
            if (m_prevCoolantTemp < CRITICAL_COOLANT_TEMP)
                GetXR1().m_MWSActive = true;
        }
    }
    else if (coolantTemp >= WARN_COOLANT_TEMP)
    {
        GetXR1().m_warningLights[wlCool] = true;

        // if this just occurred, display a warning and activate MWS
        if (m_prevCoolantTemp < WARN_COOLANT_TEMP)
        {
            GetXR1().ShowWarning("Warning Coolant Temperature.wav", DeltaGliderXR1::ST_WarningCallout, "WARNING: deploy radiator!&Check coolant temperature.");
            GetXR1().m_MWSActive = true;
        }

        // if systems just reactivated, show a message
        if (m_prevCoolantTemp >= CRITICAL_COOLANT_TEMP)
        {
            GetXR1().ShowInfo("Systems Rebooted Oxygen Flow Restored.wav", DeltaGliderXR1::ST_InformationCallout, "Systems rebooted;&Environmental systems online.");
        }
    }
    else  // coolant temperature OK
    {
        GetXR1().m_warningLights[wlCool] = false;
    }

    if (coolantTemp < CRITICAL_COOLANT_TEMP)
    {
        // since we are still below fatal temperature, internal systems are still OK
        GetXR1().m_internalSystemsFailure = false;   // systems online
    }

    // set updated coolant temp
    GetXR1().m_coolantTemp = coolantTemp;
    m_prevCoolantTemp = coolantTemp;    // save for next timestep
}

//---------------------------------------------------------------------------

AirlockDecompressionPostStep::AirlockDecompressionPostStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel),
    m_decompressionStream(nullptr), m_ventTime(-1), m_streamLevel(0.4)
{
    // NOTE: m_particleStreamSpec is not initialized here; it is write-only

    // initialize our airvent data
    m_airvent.flags = 0;        //     streamspec bitflags
	m_airvent.srcsize = 1.0;    //     particle size at creation [m]
	m_airvent.srcrate = 15;     //     average particle creation rate [Hz]
	m_airvent.v0 = 0.5;         //     emission velocity [m/s]
	m_airvent.srcspread = 0.3;  //     velocity spread during creation
	m_airvent.lifetime = 2;     //     average particle lifetime
	m_airvent.growthrate = 0.3; //     particle growth rate [m/s]
	m_airvent.atmslowdown = 1.0;//     slowdown rate in atmosphere
	m_airvent.ltype = PARTICLESTREAMSPEC::EMISSIVE;     // render lighting method
	m_airvent.levelmap = PARTICLESTREAMSPEC::LVL_LIN;   // mapping from level to alpha
	m_airvent.lmin = 0.1;
    m_airvent.lmax = 0.1;       //     min and max levels for level PLIN and PSQRT mapping types
	m_airvent.atmsmap = PARTICLESTREAMSPEC::ATM_FLAT;    // mapping from atmospheric params to alpha
	m_airvent.amin = 0.1;
    m_airvent.amax = 0.1;       //     min and max densities for atms PLIN mapping
	m_airvent.tex = nullptr;       //     particle texture handle (NULL for default)
}

void AirlockDecompressionPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // check for both airlock doors open and low atmospheric pressure AND we are not docked
    // doors are open if >= 10% ajar
    const bool doorsOpen = ((GetXR1().olock_status > 0.10) && (GetXR1().ilock_status > 0.10));
    if (doorsOpen && (GetXR1().m_cabinO2Level > 0) && (GetXR1().GetAtmPressure() < 50e3) && (GetXR1().IsDocked() == false))
    {
        // decompression!
        // obtain our docking port params
        VECTOR3 pos;    // docking port position in local coordinates; will be start of decompression stream
        VECTOR3 dir;
        VECTOR3 rot;    // not used, but we need to pass it anyway
        DOCKHANDLE hDock = GetVessel().GetDockHandle(0);
        GetVessel().GetDockParams(hDock, pos, dir, rot);  // stream will move AWAY from the airlock along its vector

        m_decompressionStream = GetVessel().AddParticleStream(&m_airvent, pos, dir, &m_streamLevel);
        m_ventTime = simt;

        GetXR1().DecompressCabin();
    }

    // halt the stream if time expired
    if ((m_decompressionStream != nullptr) && (simt > m_ventTime + 4.0))    // vent for four seconds
    {
        GetVessel().DelExhaustStream(m_decompressionStream);
        m_decompressionStream = nullptr;
    }
}

//---------------------------------------------------------------------------

AutoCenteringSimpleButtonAreasPostStep::AutoCenteringSimpleButtonAreasPostStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel)
{
}

void AutoCenteringSimpleButtonAreasPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    DoCenterOfGravityCenter(simt, simdt, mjd);
    DoHoverCenter(simt, simdt, mjd);
    DoScramCenter(simt, simdt, mjd);
    DoMainPitchCenter(simt, simdt, mjd);
    DoMainYawCenter(simt, simdt, mjd);
}

// button handlers begin here
//---------------
void AutoCenteringSimpleButtonAreasPostStep::DoCenterOfGravityCenter(const double simt, const double simdt, const double mjd)
{
    bool &isLit = GetXR1().m_cogShiftCenterModeActive;
    const int areaID = AID_COG_CENTER_BUTTON;

    if (isLit)
    {
        if (GetXR1().m_cogForceRecenter == false)       // OK to do normal checks?
        {
            if (GetXR1().VerifyManualCOGShiftAvailable() == false)
            {
                isLit = false;
                GetXR1().TriggerRedrawArea(areaID);    // redraw ourselves
                return;
            }
        }
        else // autopilot wants to re-center; only do a (SILENT!) APU check
        {
            if (GetXR1().CheckHydraulicPressure(false, false) == false)
            {
                isLit = false;
                GetXR1().TriggerRedrawArea(areaID);    // redraw ourselves
                return;
            }
            // else fall through and perform the recenter
        }

        // move the center-of-lift toward the neutral point
        // shift it ourselves instead of calling GetXR1().ShiftCenterOfLift() because we need to check if we cross the center boundary
        const double shift = oapiGetSimStep() * COL_MAX_SHIFT_RATE;
        if (GetXR1().m_centerOfLift > NEUTRAL_CENTER_OF_LIFT)
        {
            GetXR1().m_centerOfLift -= shift;
            if (GetXR1().m_centerOfLift < NEUTRAL_CENTER_OF_LIFT)   // crossed the center boundary?
                GetXR1().m_centerOfLift = NEUTRAL_CENTER_OF_LIFT;
        }
        else if (GetXR1().m_centerOfLift < NEUTRAL_CENTER_OF_LIFT)
        {
            GetXR1().m_centerOfLift += shift;
            if (GetXR1().m_centerOfLift > NEUTRAL_CENTER_OF_LIFT)   // crossed the center boundary?
                GetXR1().m_centerOfLift = NEUTRAL_CENTER_OF_LIFT;
        }

        // sanity-check: keep in range, although in theory it should always be in range since we are moving toward center
        GetXR1().ShiftCenterOfLift(0);      // just range-check

        if (GetXR1().m_centerOfLift == NEUTRAL_CENTER_OF_LIFT)  // reached the center?
        {
            isLit = false;     // all done recentering
            GetXR1().SetRecenterCenterOfGravityMode(false);
        }
    }
    else    // not lit
    {
        // reset request to force recentering
        GetXR1().m_cogForceRecenter = false;
    }
}

void AutoCenteringSimpleButtonAreasPostStep::DoHoverCenter(const double simt, const double simdt, const double mjd)
{
    bool &isLit = GetXR1().m_hoverCenteringMode;
    const int areaID = AID_HBALANCECENTER;

    if (isLit)      // are we recentering?
    {
        // can't move unless the APU is online
        if (GetXR1().CheckHydraulicPressure(isLit, isLit) == false)  // play warning and beep only if light is lit, meaning it was just pressed
        {
            isLit = false;
            GetXR1().TriggerRedrawArea(areaID);    // redraw ourselves
            return;
        }

        double shift = oapiGetSimStep() * HOVER_BALANCE_SPEED;
        if (GetXR1().m_hoverBalance > 0)
        {
            GetXR1().m_hoverBalance -= shift;
            if (GetXR1().m_hoverBalance < 0)   // crossed the center boundary?
                GetXR1().m_hoverBalance = 0;
        }
        else if (GetXR1().m_hoverBalance < 0)
        {
            GetXR1().m_hoverBalance += shift;
            if (GetXR1().m_hoverBalance > 0)   // crossed the center boundary?
                GetXR1().m_hoverBalance = 0;
        }

        // sanity-check: keep in range, although in theory it should always be in range since we are moving toward center
        if (GetXR1().m_hoverBalance > MAX_HOVER_IMBALANCE)
            GetXR1().m_hoverBalance = MAX_HOVER_IMBALANCE;
        else if (GetXR1().m_hoverBalance < -MAX_HOVER_IMBALANCE)
            GetXR1().m_hoverBalance = -MAX_HOVER_IMBALANCE;

        // NOTE: must take damage into account here!
        const int hoverThrustIdx = GetXR1().GetXR1Config()->HoverEngineThrust;
        const double maxThrustFore = MAX_HOVER_THRUST[hoverThrustIdx] * GetXR1().GetDamageStatus(HoverEngineFore).fracIntegrity;
        const double maxThrustAft  = MAX_HOVER_THRUST[hoverThrustIdx] * GetXR1().GetDamageStatus(HoverEngineAft).fracIntegrity;
        GetVessel().SetThrusterMax0(GetXR1().th_hover[0], maxThrustFore * (1.0 + GetXR1().m_hoverBalance));
        GetVessel().SetThrusterMax0(GetXR1().th_hover[1], maxThrustAft *  (1.0 - GetXR1().m_hoverBalance));

        if (GetXR1().m_hoverBalance == 0.0)  // reached the center?
        {
            isLit = false;     // all done recentering
            GetXR1().TriggerRedrawArea(areaID);    // redraw ourselves since the light is out now
        }
    }
}

void AutoCenteringSimpleButtonAreasPostStep::DoScramCenter(const double simt, const double simdt, const double mjd)
{
    bool &isLit = GetXR1().m_scramCenteringMode;
    const int areaID = AID_GIMBALSCRAMCENTER;

    if (isLit)      // are we recentering?
    {
        // can't move unless the APU is online
        if (GetXR1().CheckHydraulicPressure(true, true) == false)  // play warning and beep if APU offline
        {
            isLit = false;
            GetXR1().TriggerRedrawArea(areaID);    // redraw ourselves
            return;
        }

        // move the SCRAMs back toward center
        VECTOR3 dir;
        double phi, dphi = oapiGetSimStep() * SCRAM_GIMBAL_SPEED;
        bool keep_going = false;

        for (int i = 0; i < 2; i++)
        {
            GetVessel().GetThrusterDir(GetXR1().th_scram[i], dir);
            phi = atan2 (dir.y, dir.z);
            if (phi >= SCRAM_DEFAULT_DIR) 
                phi = max(SCRAM_DEFAULT_DIR, phi - dphi);
            else                          
                phi = min(SCRAM_DEFAULT_DIR, phi + dphi);

            GetVessel().SetThrusterDir(GetXR1().th_scram[i], _V(0, sin(phi), cos(phi)));

            if (phi != SCRAM_DEFAULT_DIR) 
                keep_going = true;
        }

        if (!keep_going)
        {
            isLit = false;      // we reached the center
            GetXR1().TriggerRedrawArea(areaID);    // redraw ourselves since the light is out now
        }
    }
}

void AutoCenteringSimpleButtonAreasPostStep::DoMainPitchCenter(const double simt, const double simdt, const double mjd)
{
    bool &isLit = GetXR1().m_mainPitchCenteringMode;
    const int areaID = AID_PGIMBALMAINCENTER;

    if (isLit)   // are we recentering?
    {
        // can't move unless the APU is online
        if (GetXR1().CheckHydraulicPressure(true, true) == false)  // play warning and beep 
        {
            isLit = false;
            GetXR1().TriggerRedrawArea(areaID);    // redraw ourselves
            return;
        }

        VECTOR3 dir;
        double dy = oapiGetSimStep() * MAIN_PGIMBAL_SPEED;
        bool keep_going = false;

        for (int i = 0; i < 2; i++)
        {
            GetVessel().GetThrusterDir(GetXR1().th_main[i], dir);
            dir /= dir.z;
            if (dir.y >= 0.0) dir.y = max(0.0, dir.y - dy);
            else              dir.y = min(0.0, dir.y + dy);
            GetVessel().SetThrusterDir(GetXR1().th_main[i], dir);

            if (dir.y) 
                keep_going = true;
        }

        if (!keep_going)
        {
            isLit = false;      // we reached the center
            GetXR1().TriggerRedrawArea(areaID);  // redraw ourselves since the light is out now
        }
    }
}

void AutoCenteringSimpleButtonAreasPostStep::DoMainYawCenter(const double simt, const double simdt, const double mjd)
{
    // loop through and process all three buttons
    for (int buttonIndex=CENTER; buttonIndex <= AUTO; buttonIndex++)  // CENTER, DIVERGENT, AUTO
    {
        bool *pIsLit;
        int areaID;

        if (buttonIndex == CENTER)   
        {
            pIsLit = &GetXR1().m_mainYawCenteringMode;
            areaID = AID_YGIMBALMAINCENTER;
        }
        else if (buttonIndex == DIVERGENT) 
        {
            pIsLit = &GetXR1().m_mainDivMode;
            areaID = AID_YGIMBALMAINDIV;
        }
        else    // AUTO
        {
            pIsLit = &GetXR1().m_mainAutoMode;
            areaID = AID_YGIMBALMAINAUTO;
        }

        if (*pIsLit)
        {
            // can't move unless the APU is online
            if (GetXR1().CheckHydraulicPressure(true, true) == false)  // play warning and beep
            {
                *pIsLit = false;
                GetXR1().TriggerRedrawArea(areaID);    // redraw our button
                return;
            }

            VECTOR3 dir;
            double tgtx[2], dx = oapiGetSimStep() * MAIN_YGIMBAL_SPEED;
            bool keep_going = false;

            switch (buttonIndex)
            {
            case CENTER:
                tgtx[0] = tgtx[1] = 0.0;    // set target coordinates to center
                break;

            case DIVERGENT:  
                tgtx[1] = -(tgtx[0] = MAIN_YGIMBAL_RANGE); // opposite ends
                break;

            case AUTO: 
                { 
                    // auto-adjusts based on differing engine thrust to keep vector straight ahead
                    double t0 = GetVessel().GetThrusterLevel(GetXR1().th_main[0]);
                    double t1 = GetVessel().GetThrusterLevel(GetXR1().th_main[1]);
                    double tt = t0 + t1;
                    tgtx[0] = tgtx[1] = (tt ? (MAIN_YGIMBAL_RANGE * (t0-t1)/tt) : 0.0);
                }
                break;
            }

            // adjust thrusters to match gimble effect
            for (int i = 0; i < 2; i++)
            {
                GetVessel().GetThrusterDir(GetXR1().th_main[i], dir);
                dir /= dir.z;

                if (dir.x >= tgtx[i]) dir.x = max(tgtx[i], dir.x - dx);
                else                  dir.x = min(tgtx[i], dir.x + dx);

                if ((dir.x != tgtx[i]) || (buttonIndex == AUTO))   // AUTO mode never terminates until button pressed
                    keep_going = true;    

                GetVessel().SetThrusterDir(GetXR1().th_main[i], dir);
            }

            if (!keep_going)
            {
                *pIsLit = false;    // reached target coordinates
                GetXR1().TriggerRedrawArea(areaID);    // redraw this button
            }
        }
    }
}

//---------------------------------------------------------------------------

// simple PostStep to reset the APU inactive timestamp anytime for systems that must be polled
ResetAPUTimerForPolledSystemsPostStep::ResetAPUTimerForPolledSystemsPostStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel)
{
}

void ResetAPUTimerForPolledSystemsPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    bool resetAPUTimer = false;

    // if AUTO COG shift mode engage, APU is allowed to always run without triggering a warning
    resetAPUTimer |= GetXR1().m_cogShiftAutoModeActive;

    // check wheel brakes
    resetAPUTimer |= ((GetVessel().GetWheelbrakeLevel(1) + GetVessel().GetWheelbrakeLevel(2)) > 0.0);

    if (resetAPUTimer)
        GetXR1().MarkAPUActive();  // reset the APU idle warning callout time
}

//---------------------------------------------------------------------------

// simple PostStep to reset the APU inactive timestamp anytime that COG shift is set to AUTO mode
ManageMWSPostStep::ManageMWSPostStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel)
{
}

// Hook the timestep we can flash our light if necessary
void ManageMWSPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd) 
{
    if (GetXR1().m_MWSActive)    // is light enabled?
    {
        double di;
        bool mwson = (modf(simt, &di) < 0.5);   // toggle twice a second
        if (mwson != GetXR1().m_MWSLit)  // not updated the light yet?
        {
            // toggle the state and request a repaint
            GetXR1().m_MWSLit = mwson;
            GetXR1().TriggerRedrawArea(AID_MWS);

            // if light just turned on, play the warning beep
            if (mwson)
                GetXR1().PlaySound(GetXR1().WarningBeep, DeltaGliderXR1::ST_Other, WARNING_BEEP_VOL);
        }
    }
    else    // light disabled
    {
        // MWS is now inactive; turn the light OFF if it is on
        if (GetXR1().m_MWSLit)
        {
            GetXR1().m_MWSLit = false;
            GetXR1().TriggerRedrawArea(AID_MWS);
        }
    }
}

//---------------------------------------------------------------------------
// Special debug PostStep to test new XRVesselCtrl API methods via the debugger
//---------------------------------------------------------------------------
#ifdef _DEBUG
TestXRVesselCtrlPostStep::TestXRVesselCtrlPostStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel)
{
}

void TestXRVesselCtrlPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // wait 2 seconds before running tests
    if (simt < 2.0)
        return;

    XRVesselCtrl *pXRvc = &GetXR1();

    double di;
    const bool mode = (modf(simt, &di) < 0.5);   // toggle twice a second
#if 0
    //-------------------------
    // API 2.3 testing
    //-------------------------
    // negative testing
    bool bBadSlotFree = pXRvc->IsPayloadBaySlotFree(0);
    bool bBadXR2SlotFree = pXRvc->IsPayloadBaySlotFree(4);
    bool bBadXR5SlotFree = pXRvc->IsPayloadBaySlotFree(37);

    int payloadBaySlotCount = pXRvc->GetPayloadBaySlotCount();
    bool bSlot0Free = pXRvc->IsPayloadBaySlotFree(1);
    bool bSlot1Free = pXRvc->IsPayloadBaySlotFree(2);

    XRPayloadSlotData sd = {0};
    bool bSlotDataValid = pXRvc->GetPayloadSlotData(2, sd);

    // attach a payload object if not already attached
    OBJHANDLE hPayloadVessel = oapiGetVesselByName("payload0");
    int deployedCount = -1;
    if (bSlotDataValid && oapiIsVessel(hPayloadVessel))
    {
        // see if it's already attached in slot 2
        if (!sd.IsOccupied)
        {
            // attach it
            bool bCanAttach = pXRvc->CanAttachPayload(hPayloadVessel, 2);
            bool bAttached = false;
            if (bCanAttach)
                bAttached = pXRvc->GrapplePayloadModuleIntoSlot(hPayloadVessel, 2);
        }
        else  // it's attached in the slot, so detach it
        {
            return;  // use 'set next statement' to test one of these below
            deployedCount = pXRvc->DeployPayloadInFlight(2, 1.0);
            deployedCount = pXRvc->DeployPayloadWhileLanded(2);

            deployedCount = pXRvc->DeployAllPayloadInFlight(1.0);
            deployedCount = pXRvc->DeployAllPayloadWhileLanded();
        }
    }
    //-------------------------
#endif

#if 0   // {ZZZ} uncomment this block for manual testing
    //-------------------------
    // API 3.0 testing
    //-------------------------
    XRSystemStatusRead status;
    pXRvc->GetXRSystemStatus(status);

    //pXRvc->SetMWSTest(mode);

    bool bRecenterCoGMode = pXRvc->GetRecenterCOGMode();
    //pXRvc->SetRecenterCOGMode(mode);
    if (simt < 5) pXRvc->SetRecenterCOGMode(true);

    XRDoorState extCoolingState = pXRvc->GetExternalCoolingState();
    pXRvc->SetExternalCoolingState(mode);

    //pXRvc->SetCrossFeedMode((mode ? XRXF_MAIN : XRXF_OFF));
    //pXRvc->SetCrossFeedMode((mode ? XRXF_RCS : XRXF_OFF));
#endif
}
#endif

//---------------------------------------------------------------------------

// Generic delayed initializion poststep; it will 1) Restore the Orbiter render window position, and 2) initialize the dummy payload vessel and XRPayload cache.
// Code in this PostStep will run only once exactly 1/2-second after simulation startup.
OneShotInitializationPostStep::OneShotInitializationPostStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel),
    m_done(false)
{
}

void OneShotInitializationPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // this is a one-shot occurrence
    if (m_done)
        return;

    // NOTE: since we only reach here on startup and startup always starts at simt=0, we don't need to worry about
    // simt ever being negative here (because of the m_done check above).
    _ASSERTE(simt >= 0);

    // wait 0.5 second before initializing
    if (simt < 0.5)
        return;

    // finish initializing the payload bay, if any
	if (GetXR1().m_pPayloadBay)
	{
		GetXR1().m_pPayloadBay->PerformFinalInitialization(GetXR1().m_dummyAttachmentPoint);

		// initialize the grapple target data
		GetXR1().RefreshGrappleTargetsInDisplayRange();
	}
    
	// restore the Orbiter window to its save coordinates if we haven't already done so once 
	// NOTE: this call was originally invoked from clbkPostCreation, but for some reason repositioning the D3D9 client window does not work from there.  So we work around it by doing it after the simulation has been running for 0.5 second.
	GetXR1().RestoreOrbiterRenderWindowPosition();  

    // disable further runs
    m_done = true;
}

//-------------------------------------------------------------------------

// Switch to a new 2D panel after a 0.10-second delay; this will give Orbiter time to finish dispatching panel events before the panel
// is switched "out from under" any active area objects.
SwitchTwoDPanelPostStep::SwitchTwoDPanelPostStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel),
    m_targetSwitchSimt(0), m_target2DPanel(-1)
{
}

void SwitchTwoDPanelPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // Note: this works even if the ship is crashed, so we don't check for isCrashed here.
    if (GetXR1().m_requestSwitchToTwoDPanelNumber >= 0)
    {
        // panel switch requested; set the target time
        m_targetSwitchSimt = simt + 0.10;   // switch 1/10-second from now

        // latch and reset the target panel request
        m_target2DPanel = GetXR1().m_requestSwitchToTwoDPanelNumber;  
        GetXR1().m_requestSwitchToTwoDPanelNumber = -1;               
    }
    else if (m_target2DPanel >= 0)    // is a panel switch request in progress?
    {
        if (simt >= m_targetSwitchSimt)
        {
            // switch panels and reset
            oapiSetPanel(m_target2DPanel);
            m_target2DPanel = -1;   
        }
    }
}
