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

#include "resource.h"

#include "XR1PostSteps.h"

//---------------------------------------------------------------------------

SetHullTempsPostStep::SetHullTempsPostStep(DeltaGliderXR1& vessel) :
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
        // Orbiter forumla: density / 2 * v^3
        // Note: messing with air density is a bitch, so we cheat and just use pressure here.
        double tweakedAtmPressure = atmPressure / 2;       // atmospheric pressure in pascals

        // NOTE: this was modified because the hull temperature was too high at low
        // altitudes (our boundary layer was artifically reducing hull temperatures at high velocities).
        // The boundary insulation layer been removed to keep the hull temp lower across the board and the workingHullHeatingFactor
        // was modified to keep the same target max hull temperature.  This tweak should not require any changes to the subclasses.
        // ORG prior to the XR2: const double tweakedAirspeed = (airspeed*airspeed*airspeed);  // use explicit mult for efficiency
        const double workingHullHeatingFactor = HULL_HEATING_FACTOR * 0.642; // tweaked very carefully...
        const double tweakedAirspeed = (airspeed * airspeed * airspeed);

        const double speedTimesPressure = tweakedAirspeed * tweakedAtmPressure;

        // NOTE: DO NOT SCALE THIS WITH simdt!  We are calculating an ABSOLUTE temperature, not DELTA temperature
        // this is the amount of heat to be added to the leading edges of the ship.
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
            double heatConductionPower = ((atmPressure - minHeatConductionPressure) / (maxHeatConductionPressure - minHeatConductionPressure));  // from 0.0 to 1.0 (higher number = more heat dropped due to conduction)
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
            double boundaryEffectivenessFrac = min(((airspeed - boundaryLayerLowerLimit) / (boundaryInsulatorSpeed - boundaryLayerLowerLimit)), 1.0);

            // heat fraction reaching the hull = minHeatFrac@max insulation + (fraction between minHeatFrac and 1.0 based on effectiveness)
            heatFrac = boundaryInsulatorFrac + ((1.0 - boundaryInsulatorFrac) * (1.0 - boundaryEffectivenessFrac));  // e.g., 0.29 + (0.71 * 0.5) = 0.29 + 0.355 = 0.645
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
void SetHullTempsPostStep::RemoveSurfaceHeat(const double simdt, double& temp)
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
        const MATERIAL* pSrcHeatingMaterial = oapiMeshMaterial(GetXR1().heatingmesh_tpl, GetHeatingMeshGroupIndex());
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
