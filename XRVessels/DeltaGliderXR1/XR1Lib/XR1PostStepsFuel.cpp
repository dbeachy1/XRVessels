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
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// XR1FuelPostSteps.cpp
// Class defining fuel-related PostSteps for the DG-XR1
// ==============================================================

#include "XR1FuelPostSteps.h"
#include "AreaIDs.h"
#include "XRPayloadBay.h"

//---------------------------------------------------------------------------

// handles fuel and LOX callouts
FuelCalloutsPostStep::FuelCalloutsPostStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel),
    m_prevMainFuelFrac(-1), m_prevScramFuelFrac(-1), m_prevRcsFuelFrac(-1), m_prevLoxFrac(-1)
{
}

void FuelCalloutsPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard())  // covers IsCrashed() as well
        return;     

    CheckFuelLevel("Main", GetXR1().ph_main, m_prevMainFuelFrac, WarningLight::wlMfuel);
    CheckFuelLevel("RCS", GetXR1().ph_rcs, m_prevRcsFuelFrac, WarningLight::wlRfuel);
    CheckFuelLevel("SCRAM", GetXR1().ph_scram, m_prevScramFuelFrac, WarningLight::wlNONE);  // no light for SCRAM fuel; low SCRAM fuel is not a critical warning condition: it is normal
    
    // NOTE: APU fuel is checked in APUPostStep

    CheckLoxLevel();
}

void FuelCalloutsPostStep::CheckFuelLevel(const char *pLabel, PROPELLANT_HANDLE ph, double &prevQtyFrac, WarningLight warningLight)
{
    // NOTE: we need to detect whether we just backed out an Orbiter refuel and ignore the fuel level change.
    // If landed on a pad, the Orbiter core starts us auto-refuelled, and then when your fuel PreStep (correctly) backs out the 
    // fuel level to zero a frame later, the code here sees the level go from 1.0 to 0.0 and so throws a
    // "Foo Fuel Depleted" warning on startup (see XR2 Phobos/Deimos misson scenario startup). 
    if (GetXR1().m_backedOutOrbiterCoreAutoRefuelThisFrame)
    {
        // Force a reset to the current fuel level (level is zero for backed-out tanks now) so we 
        // don't throw a warning due to the level going from 1.0 to 0.
        prevQtyFrac = -1;   
    }

    // check the fuel level and see whether it is low or depleted, even if we are crashed
    char pSoundFilename[64];
    char pMsg[64];

#define SEND_FUEL_WARNING(level) \
        sprintf(pSoundFilename, "Warning %s Fuel " level ".wav", pLabel);  \
        sprintf(pMsg, "WARNING: %s Fuel " level, pLabel);                  \
        GetXR1().ShowWarning(pSoundFilename, DeltaGliderXR1::ST_WarningCallout, pMsg)

    const double currentPropMassFrac = SAFE_FRACTION(GetXR1().GetXRPropellantMass(ph), GetXR1().GetXRPropellantMaxMass(ph));
    const double warningFrac = 0.05;

    if (prevQtyFrac != -1)  // not first time through here?
    {
        if ((currentPropMassFrac >= 1.0) && (prevQtyFrac < 1.0))   // just hit full?
        {
            sprintf(pSoundFilename, "%s Fuel Tanks Full.wav", pLabel);
            sprintf(pMsg, "%s fuel tanks full.", pLabel);
            GetXR1().ShowInfo(pSoundFilename, DeltaGliderXR1::ST_InformationCallout, pMsg);
        }
        else if ((currentPropMassFrac <= 0) && (prevQtyFrac > 0))    // just hit 0%?
        {
            SEND_FUEL_WARNING("Depleted");
            if (warningLight != WarningLight::wlNONE)
                GetXR1().m_MWSActive = true;
        }
        else if ((currentPropMassFrac < warningFrac) && (prevQtyFrac >= warningFrac)) // just crossed below 5% remaining?
        {
            SEND_FUEL_WARNING("Low");
            if (warningLight != WarningLight::wlNONE)
                GetXR1().m_MWSActive = true;
        }

        // warning light always blinks regardless of main MWS light
        if (warningLight != WarningLight::wlNONE)
        {
            if (currentPropMassFrac < warningFrac)
                GetXR1().m_warningLights[static_cast<int>(warningLight)] = true;
            else
                GetXR1().m_warningLights[static_cast<int>(warningLight)] = false;  // fuel level OK
        }
    }

    // update prevQty for next loop
    prevQtyFrac = currentPropMassFrac;
}

void FuelCalloutsPostStep::CheckLoxLevel()
{
    // check the LOX level and see whether it is low or depleted, even if we are crashed
    // This take payload LOX into account as well
    const double currentQtyFrac = GetXR1().GetXRLOXMass() / GetXR1().GetXRLOXMaxMass();
    const double warningFrac = 0.10;

    if (m_prevLoxFrac != -1)  // not first time through here?
    {
        // must set a threshold < 1.0 here since LOX is constantly consumed
        const double fullThreshold = 0.99999;
        if ((currentQtyFrac >= fullThreshold) && (m_prevLoxFrac < fullThreshold))   // just hit full?
        {
            GetXR1().ShowInfo("LOX Tanks Full.wav", DeltaGliderXR1::ST_InformationCallout, "LOX tanks full.");
        }
        else if ((currentQtyFrac <= 0) && (m_prevLoxFrac > 0))    // just hit 0%?
        {
            GetXR1().ShowWarning("Warning Oxygen Depleted.wav", DeltaGliderXR1::ST_WarningCallout, "WARNING: OXYGEN DEPLETED");
            GetXR1().m_MWSActive = true;
        }
        else if ((currentQtyFrac < warningFrac) && (m_prevLoxFrac >= warningFrac)) // just crossed below 5% remaining?
        {
            GetXR1().ShowWarning("Warning Oxygen Low.wav", DeltaGliderXR1::ST_WarningCallout, "WARNING: Oxygen low.");
            GetXR1().m_MWSActive = true;
        }

        // warning light always blinks regardless of main MWS light
        if (currentQtyFrac < warningFrac)
            GetXR1().m_warningLights[static_cast<int>(WarningLight::wlLox)] = true;
        else
            GetXR1().m_warningLights[static_cast<int>(WarningLight::wlLox)] = false;  // LOX level OK
    }

    // update m_prevQty for next loop
    m_prevLoxFrac = currentQtyFrac;
}

//---------------------------------------------------------------------------

UpdateMassPostStep::UpdateMassPostStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel)
{
}

void UpdateMassPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // adjust mass for APU fuel, LOX, passengers, etc.
    GetXR1().SetEmptyMass();
}

//---------------------------------------------------------------------------

FuelDumpPostStep::FuelDumpPostStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel),
    m_nextWarningSimt(-1), m_fuelDumpStream1(nullptr), m_fuelDumpStream2(nullptr), 
    m_fuelDumpLevel(0)
{
    // create the particle streams if the parent vessel supports them
    if (GetXR1().m_pFuelDumpParticleStreamSpec != nullptr)
        m_fuelDumpStream1 = GetVessel().AddParticleStream(GetXR1().m_pFuelDumpParticleStreamSpec, FUEL_DUMP_PARTICLE_STREAM_POS1, FUEL_DUMP_PARTICLE_STREAM_DIR1, &m_fuelDumpLevel);

    if (GetXR1().m_pFuelDumpParticleStreamSpec != nullptr)
        m_fuelDumpStream2 = GetVessel().AddParticleStream(GetXR1().m_pFuelDumpParticleStreamSpec, FUEL_DUMP_PARTICLE_STREAM_POS2, FUEL_DUMP_PARTICLE_STREAM_DIR2, &m_fuelDumpLevel);
}

// destructor
FuelDumpPostStep::~FuelDumpPostStep()
{
    if (m_fuelDumpStream1 != nullptr)
        GetVessel().DelExhaustStream(m_fuelDumpStream1);

    if (m_fuelDumpStream2 != nullptr)
        GetVessel().DelExhaustStream(m_fuelDumpStream2);
}

void FuelDumpPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    m_fuelDumpLevel = 0.0;      // 0 -> 1.0; used for dump particle level
    // flow weights:
    //   Main:  50%
    //   SCRAM: 25%
    //   LOX: 15%
    //   RCS: 5%
    //   APU: 5%
    if (GetXR1().m_mainFuelDumpInProgress)
    {
        if (DumpFuel(GetXR1().ph_main, simdt, GetXR1().m_mainFuelDumpInProgress, 1.0))
            m_fuelDumpLevel += 0.50;
    }

    if (GetXR1().m_rcsFuelDumpInProgress)
    {
        if (DumpFuel(GetXR1().ph_rcs, simdt, GetXR1().m_rcsFuelDumpInProgress, RCS_FLOW_FRACTION))
            m_fuelDumpLevel += 0.05;
    }

    if (GetXR1().m_scramFuelDumpInProgress)
    {
        if (DumpFuel(GetXR1().ph_scram, simdt, GetXR1().m_scramFuelDumpInProgress, SCRAM_FLOW_FRACTION))
            m_fuelDumpLevel += 0.25;
    }

    if (GetXR1().m_apuFuelDumpInProgress)
    {
        // must dump APU fuel manually here
        if (GetXR1().m_apuFuelQty > 0)
        {
            GetXR1().m_apuFuelQty -= (FUEL_DUMP_RATE * simdt * APU_FLOW_FRACTION);
            if (GetXR1().m_apuFuelQty <= 0)    // underflow?
                GetXR1().m_apuFuelQty = 0;     
            else
                m_fuelDumpLevel += 0.05;
        }

        if (GetXR1().m_apuFuelQty <= 0)    // did tank reach empty?
        {
            GetXR1().PlayErrorBeep();   // alert the pilot
            GetXR1().m_apuFuelDumpInProgress = false;   // halt the dump
        }
    }

    if (GetXR1().m_loxDumpInProgress)
    {
        // must dump LOX manually here
        // This take payload LOX into account as well
        if (GetXR1().GetXRLOXMass() > 0)
        {
            // LOX flow fraction is based on tank capacity AND a minimum flow rate per second
            const double flowRate = max(GetXR1().GetXRLOXMaxMass() * LOX_DUMP_FRAC, LOX_MIN_DUMP_RATE);

            double qty = GetXR1().GetXRLOXMass();
            qty -= (flowRate * simdt);  // mass * dump rate in TANK FRACTION/SECOND
            if (qty <= 0)     // underflow?
                qty = 0;      // prevent underflow
            else
                m_fuelDumpLevel += 0.15; 
            GetXR1().SetXRLOXMass(qty);  // updates payload LOX as well
        }

        if (GetXR1().GetXRLOXMass() <= 0)  // did tank reach empty?
        {
            GetXR1().PlayErrorBeep();   // alert the pilot
            GetXR1().SetLOXDumpState(false);  // halt the dump
        }
    } 

    // update the dump particle stream rate
    // TESTING ONLY: m_fuelDumpLevel = 1.0;
    // DEBUG: sprintf(oapiDebugString(), "m_fuelDumpLevel=%lf", m_fuelDumpLevel);  

    // manage the fuel flow sound
    const int flowCount = static_cast<int>(GetXR1().m_mainFuelDumpInProgress) + 
            static_cast<int>(GetXR1().m_rcsFuelDumpInProgress) + 
            static_cast<int>(GetXR1().m_scramFuelDumpInProgress) + 
            static_cast<int>(GetXR1().m_apuFuelDumpInProgress) + 
            static_cast<int>(GetXR1().m_loxDumpInProgress);
    if (flowCount > 0)  
    {
        // handle fuel/lox flow sounds
        // determine volume level
        const int volume = FUEL_DUMP_BASE_VOL + ((flowCount-1) * FUEL_DUMP_INC_VOL);

        // always play this sound so we can adjust the volume
        GetXR1().PlaySound(GetXR1().FuelDump, DeltaGliderXR1::ST_Other, volume, true);   // loop this sound (although we keep playing it here anyway)

        // show a warning every 5 seconds while any fuel dump is in progress (this also plays immediately the first time)
        if (simt >= m_nextWarningSimt)  // NOTE: warning always plays the first time because m_nextWarningSimt == -1
        {
            char temp[45];
            const bool isLOX = GetXR1().m_loxDumpInProgress;
            
            // test LOX first; priority is HIGHEST -> LOWEST
            // NOTE: these messages must match the text in XR1LowerPanelComponents
            if (isLOX)
                sprintf(temp, "WARNING: LOX dump in progress.");
            else if (GetXR1().m_apuFuelDumpInProgress)
                sprintf(temp, "WARNING: APU fuel dump in progress.");
            else if (GetXR1().m_mainFuelDumpInProgress)
                sprintf(temp, "WARNING: Main fuel dump in progress.");
            else if (GetXR1().m_rcsFuelDumpInProgress)
                sprintf(temp, "WARNING: RCS fuel dump in progress.");
            else if (GetXR1().m_scramFuelDumpInProgress)
                sprintf(temp, "WARNING: SCRAM fuel dump in progress.");
            else    // should never happen!
            {
                _ASSERTE(false);
                *temp = 0;
            }

            if (isLOX)
                GetXR1().ShowWarning("Warning LOX Dump.wav", DeltaGliderXR1::ST_WarningCallout, temp);
            else
                GetXR1().ShowWarning("Warning Fuel Dump.wav", DeltaGliderXR1::ST_WarningCallout, temp);

            m_nextWarningSimt = simt + 5.0; // reset
        }
    }
    else    // fuel not flowing (flowCount == 0)
    {
        GetXR1().StopSound(GetXR1().FuelDump);
        m_nextWarningSimt = -1;   // reset for next time
        m_fuelDumpLevel = 0;      // halted
    }
}

// rateFraction = fraction of speed to dump this tank
// Returns: true if dump continuing, false if dump was halted
bool FuelDumpPostStep::DumpFuel(const PROPELLANT_HANDLE ph, const double simdt, bool &dumpInProgress, const double rateFraction)
{
    // NOTE: it is possible for remaining to be zero here already, so we have to check to end the dump *outside* the "remaining > 0" if block below
    double remaining = GetXR1().GetXRPropellantMass(ph);
    if (remaining > 0)
    {
        // add oapiRand to fuel dump rate so that kg mass goes down by a random fraction
        // (looks better on the lower panel's mass display)
        remaining -= ((FUEL_DUMP_RATE + oapiRand()) * simdt * rateFraction);
        if (remaining < 0)    // underflow?
            remaining = 0;

        // update fuel remaning in tank
        GetXR1().SetXRPropellantMass(ph, remaining);
    }
    
    if (remaining <= 0)    // is tank empty?
    {
        // tank either just reached empty or was empty on entry
        GetXR1().PlayErrorBeep();   // alert the pilot
        dumpInProgress = false;     // halt the dump
    }

    return dumpInProgress;
}

//---------------------------------------------------------------------------

XFeedPostStep::XFeedPostStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel)
{
}

void XFeedPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    double mainToRCSFlow = 0.0;

    // NOTE: flow is to or from RCS tank here, so use RCS_FLOW_FRACTION
    switch (GetXR1().m_xfeedMode)
    {
    case XFEED_MODE::XF_MAIN:
        // RCS -> MAIN
        mainToRCSFlow = -(FUEL_DUMP_RATE * simdt * RCS_FLOW_FRACTION); 
        break;

    case XFEED_MODE::XF_RCS:
        // MAIN -> RCS
        mainToRCSFlow = (FUEL_DUMP_RATE * simdt * RCS_FLOW_FRACTION); 
        break;

    default:
        // no default handler for this; fall through and do nothing
        break;
    }

    // flow the fuel
    if (mainToRCSFlow != 0)
    {
        double mainTankQty = GetXR1().GetXRPropellantMass(GetXR1().ph_main);
        double rcsTankQty = GetXR1().GetXRPropellantMass(GetXR1().ph_rcs);
        const double mainTankMaxQty = GetXR1().GetXRPropellantMaxMass(GetXR1().ph_main);
        const double rcsTankMaxQty = GetXR1().GetXRPropellantMaxMass(GetXR1().ph_rcs);

        mainTankQty -= mainToRCSFlow;
        rcsTankQty += mainToRCSFlow;

        // check limits
        bool haltFlow = false;
        const char *pMsg = nullptr;
        if (mainTankQty < 0)  // main tank underflow
        {
            // fuel flowing to RCS; remove excess fuel added
            rcsTankQty += mainTankQty;  // mainTankQty is negative

            mainTankQty = 0;
            haltFlow = true;
            pMsg = "MAIN fuel tanks empty";
        }
        else if (mainTankQty > mainTankMaxQty)  // main tank overflow
        {
            // fuel flowing from RCS; replace excess fuel removed
            rcsTankQty += (mainTankQty - mainTankMaxQty);

            mainTankQty = mainTankMaxQty;
            haltFlow = true;
            pMsg = "MAIN fuel tanks full";
        }

        if (rcsTankQty < 0)   // RCS tank underflow
        {
            // fuel flowing to main; remove excess fuel added
            mainTankQty += rcsTankQty;  // rcsTankQty is negative

            rcsTankQty = 0;
            haltFlow = true;
            pMsg = "RCS fuel tanks empty";
        }
        else if (rcsTankQty > rcsTankMaxQty)  // RCS tank overflow
        {
            // fuel flowing from main; replace excess fuel removed
            mainTankQty += (rcsTankQty - rcsTankMaxQty);

            rcsTankQty = rcsTankMaxQty;
            haltFlow = true;
            pMsg = "RCS fuel tanks full";
        }

        GetXR1().SetXRPropellantMass(GetXR1().ph_main, mainTankQty);
        GetXR1().SetXRPropellantMass(GetXR1().ph_rcs, rcsTankQty);

        if (haltFlow)
        {
            GetXR1().SetCrossfeedMode(XFEED_MODE::XF_OFF, pMsg);  // also triggers the knob to redraw
            // flow sound will stop next timestep 
        }
        else    // flow still in progress
        {
            // play sound if not already playing
            if (GetXR1().IsPlaying(GetXR1().FuelCrossFeed) == false)
                GetXR1().PlaySound(GetXR1().FuelCrossFeed, DeltaGliderXR1::ST_Other, FUEL_XFEED_VOL, true);   // loop this sound
        }
    }
    else    // fuel not flowing
    {
        GetXR1().StopSound(GetXR1().FuelCrossFeed);
    }
}



//---------------------------------------------------------------------------

// Handles LOX consumption
LOXConsumptionPostStep::LOXConsumptionPostStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel),
    m_previousAmbientO2Available(false), m_previousO2Level(-1)
{
}

void LOXConsumptionPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // if crew is DEAD, nothing to do here
    if (GetXR1().m_crewState == CrewState::DEAD)
        return;

    const int crewMembers = GetXR1().GetCrewMembersCount();

    // compensate for reduced oxygen consumption if configured as such
    const double consumptionFraction = GetXR1().GetXR1Config()->GetLOXConsumptionFraction();   // 0 < n <= 1.0
    const double loxConsumptionPerSecond = crewMembers * LOX_CONSUMPTION_RATE * consumptionFraction * GetXR1().GetXR1Config()->LOXConsumptionMultiplier;  // WARNING: MAY BE ZERO!
    const double loxConsumptionThisTimestep = loxConsumptionPerSecond * simdt;  // WARNING: MAY BE ZERO!

    // no LOX consumption if landed in earth ATM or docked and both airlocks and noscone open, OR if in earth ATM and hatch open, OR if external cooling active
    bool ambientO2Available = false;
    const bool bothAirlocksOpen = ((GetXR1().ilock_proc >= 0.25) && (GetXR1().olock_proc >= 0.25) && (GetXR1().nose_proc >= 0.25));
    const bool externalCoolingActive = (GetXR1().externalcooling_status == DoorStatus::DOOR_OPEN);
    const bool isHatchOpen = (GetXR1().hatch_proc > 0.25);  
    double loxQty = GetXR1().GetXRLOXMass();  // includes payload LOX as well
    double o2Level = GetXR1().m_cabinO2Level;   // fraction of O2 in cabin atm

    // check for cabin decompression due to open hatch
    if ((GetXR1().hatch_proc > 0.10) && (GetVessel().GetAtmPressure() < 50e3))
    {
        // decompression!
        GetXR1().ShowHatchDecompression();
        GetXR1().DecompressCabin();   // kills the crew as well
        return;     // all done for this step
    }

    // NOTE: airlock decompression is handled in a dedicated PostStep

    // NOTE: lox is NOT available if the cabin O2 level is 0, which means the cabin has decompressed!
    const bool loxAvailable = ((loxQty > 0) && (GetXR1().m_internalSystemsFailure == false) && (o2Level > 0));  // LOX flow fails internal systems failed

    if (GetXR1().InEarthAtm())
        ambientO2Available |= (isHatchOpen | bothAirlocksOpen);  // O2 available if hatch or both airlocks open

    if (GetXR1().IsDocked())  
        ambientO2Available |= bothAirlocksOpen; // O2 available if both airlocks open

    ambientO2Available |= externalCoolingActive; // O2 available if external cooling active (ambient oxygen feed on)

    // skip these checks the first time through here so that m_previousAmbientO2Available and m_previousO2Level have a chance to initialize
    if (m_previousO2Level > 0)
    {
        //
        // Use ambient O2 if available (e.g., docked w/open airlocks)
        //
        if (ambientO2Available)
        {
            // play an info message if we just reached this condition
            if (m_previousAmbientO2Available == false)
            {
                GetXR1().ShowInfo("Using External O2.wav", DeltaGliderXR1::ST_InformationCallout, "Using external oxygen;&internal O2 flow suspended.");
            }
            // Note: turn A/C sound on if using ambient air; we only want to turn off A/C sounds when LOX is depleted or unavailable (i.e., systems overheat)
            GetXR1().XRSoundOnOff(XRSound::AirConditioning, true);
        }
        else  // no ambient O2 available (using internal O2)
        {
            // play an info message if we just reached this condition
            if (m_previousAmbientO2Available)
                GetXR1().ShowInfo("Using Onboard O2.wav", DeltaGliderXR1::ST_InformationCallout, "Using onboard oxygen;&internal O2 flow resumed.");

            // consume oxygen if LOX available (delta will be 0.0 if LOX consumption disabled)
            if (loxAvailable)
            {
                loxQty -= loxConsumptionThisTimestep;
                if (loxQty < 0)
                    loxQty = 0;     // prevent underflow
            }

            // disable A/C sound if LOX exhausted or enable it if LOX available
            GetXR1().XRSoundOnOff(XRSound::AirConditioning, loxAvailable);  // no internal airflow if lox not available
        }

        //
        // Adjust ambient O2 level
        //
        if (ambientO2Available || loxAvailable)
        {
            // increment level if too low
            if (o2Level < NORMAL_O2_LEVEL)
            {
                o2Level += (AMBIENT_O2_REPLENTISHMENT_RATE * simdt);

                // NOTE: do not play callout here; callout already occurred when we crossed the LOC threshold
                if (o2Level > NORMAL_O2_LEVEL)     
                    o2Level = NORMAL_O2_LEVEL;  // avoid overrun
            }

            // level can never rise above normal, so no need to check it
        }
        else    // No O2 replentishment available; using existing cabin air only!
        {
            // only consume cabin air here if LOX consumption enabled 
            if (GetXR1().GetXR1Config()->GetLOXConsumptionFraction() > 0.0)
            {
                // level falls based on # of crew members AND whether crew is still alive
                const int crewMembers = GetXR1().GetCrewMembersCount();

                if (crewMembers > 0)
                {
                    o2Level -= (AMBIENT_O2_CONSUMPTION_RATE * crewMembers * simdt);
                }
            }
        }
        
        //
        // Check for crew unconsciousness or death UNLESS crew is already dead OR not on board (remember that the death threshold can vary slightly)
        //
        if ((GetXR1().m_crewState != CrewState::DEAD) && (GetXR1().GetCrewMembersCount() > 0))
        {
            if ((o2Level <= CREW_DEATH_O2_LEVEL) && (m_previousO2Level > CREW_DEATH_O2_LEVEL))
            {
                // no audio for this since no one is awake to hear it
                GetXR1().ShowWarning(nullptr, DeltaGliderXR1::ST_None, "CREW IS DEAD DUE TO HYPOXIA!");

                // blink this on the HUD was well
                sprintf(GetXR1().m_crashMessage, "OXYGEN DEPLETED!&CREW IS DEAD DUE TO HYPOXIA!");

                GetXR1().KillCrew();
                GetXR1().m_MWSActive = true;
            }
            else if ((o2Level <= CREW_LOC_O2_LEVEL) && (m_previousO2Level > CREW_LOC_O2_LEVEL))
            {
                // no audio for this since no one is awake to hear it
                GetXR1().ShowWarning(nullptr, DeltaGliderXR1::ST_None, "CREW IS UNCONSCIOUS!");

                // blink this on the HUD was well
                sprintf(GetXR1().m_crashMessage, "OXYGEN DEPLETED!&CREW IS UNCONSCIOUS -- DEATH IMMINENT!");

                GetXR1().m_crewState = CrewState::INCAPACITATED;
                GetXR1().m_MWSActive = true;
            }
            else if ((o2Level > CREW_LOC_O2_LEVEL) && (m_previousO2Level <= CREW_LOC_O2_LEVEL))   // is O2 level is now OK?
            {
                // crew is OK now unless DEAD

                // NOTE: this can only occur if some rescue crew member arrives, since the onboard crew will not be conscious to 
                // open the hatch or deploy the radiator, etc.; however, handle this anyway in case we implement external 
                // rescue ability someday.  
                if (GetXR1().m_crewState != CrewState::DEAD)
                {
                    GetXR1().m_crewState = CrewState::OK;

                    // reset HUD warning if msg begins with OXYGEN (bit of a hack, but suffices for now)
                    if (strncmp(GetXR1().m_crashMessage, "OXYGEN", 6) == 0)
                        *GetXR1().m_crashMessage = 0;      // reset

                    GetXR1().ShowInfo(nullptr, DeltaGliderXR1::ST_None, "O2 levels returning to normal;&Crew has regained consciousness.");
                }
            }
            else if ((o2Level <= CRITICAL_O2_LEVEL_WARNING) && (m_previousO2Level > CRITICAL_O2_LEVEL_WARNING))  // only play this once
            {
                GetXR1().ShowWarning("Warning Oxygen Levels Critical Hypoxia Imminent.wav", DeltaGliderXR1::ST_WarningCallout, "WARNING: OXYGEN LEVELS CRITICAL;&HYPOXIA IMMINENT!");
                // LOX light will already be blinking b/c tanks must be empty by this time
                GetXR1().m_MWSActive = true;
                
                // disable ATC until O2 returns to normal
                GetXR1().XRSoundOnOff(XRSound::RadioATCGroup, false);
            }
            else if ((o2Level > (CRITICAL_O2_LEVEL_WARNING + 0.01)) && (m_previousO2Level <= (CRITICAL_O2_LEVEL_WARNING + 0.01)))
            {
                GetXR1().ShowInfo("Normal Oxygen Levels Restored.wav", DeltaGliderXR1::ST_InformationCallout, "Oxygen levels returning to normal.");
                
                // re-enable ATC 
                GetXR1().XRSoundOnOff(XRSound::RadioATCGroup, true);
            }
        }
    }

    // set new O2 level
    GetXR1().m_cabinO2Level = o2Level;

    // DEBUG: sprintf(oapiDebugString(), "consumptionFraction=%lf, loxConsumptionPerSecond=%lf, loxQty=%lf, m_oxygenRemainingTime=%lf", consumptionFraction, loxConsumptionPerSecond, loxQty, GetXR1().m_oxygenRemainingTime);

    // update LOX remaining time in seconds and quantity
    // WARNING: must handle loxConsumptionPerSecond = 0 here!
    GetXR1().m_oxygenRemainingTime = ((loxConsumptionPerSecond <= 0) ? 0 : (loxQty / loxConsumptionPerSecond));
    GetXR1().SetXRLOXMass(loxQty);

    // save for next timestep
    m_previousAmbientO2Available = ambientO2Available;
    m_previousO2Level = o2Level;
}

//---------------------------------------------------------------------------
// NOTE: this must be a PostStep, instead of a PreStep as you might expect, because the Orbiter core seems to refuel the ship AFTER the PreSteps are fired.
// NOTE: take care to only check the ship's *internal* main fuel tank here, *not* the bay tanks (if any).
PreventAutoRefuelPostStep::PreventAutoRefuelPostStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel)
{
    // Since we just loaded, let's initialize the previous fuel qty values with the values loaded from the scenario file; otherwise,
    // Orbiter will auto-refuel us before we have a chance to read the original fuel levels.
    // NOTE: all of these values operate with *internal tank levels only*, since that is all that Orbiter refuels
    m_previousInternalFuelQty[0] = GetXR1().m_startupMainFuelFrac * oapiGetPropellantMaxMass(GetXR1().ph_main);
    m_previousInternalFuelQty[1] = GetXR1().m_startupRCSFuelFrac * oapiGetPropellantMaxMass(GetXR1().ph_rcs);  
    m_previousInternalFuelQty[2] = GetXR1().m_startupSCRAMFuelFrac * oapiGetPropellantMaxMass(GetXR1().ph_scram);

    // Initialize bay tank member variables to "not initialized yet": we must defer proper initialization of these
    // until clbkPrePostStep because the payload bay vessels are not attached yet in clbkPostCreation, from which we are called.
    for (int i=0; i < 3; i++)
        m_previousBayFuelQty[i] = -1;
}

void PreventAutoRefuelPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    GetXR1().m_backedOutOrbiterCoreAutoRefuelThisFrame = false;  // reset; this boolean flag is only ever set for one frame

    // perform one-time initialization if payload bay is present
    if ((GetXR1().m_pPayloadBay != nullptr) && (m_previousBayFuelQty[0] < 0))  // entire array is in sync, so checking just one is sufficient
    {
        m_previousBayFuelQty[0] = GetXR1().m_pPayloadBay->GetPropellantMaxMass(PROP_TYPE::PT_Main);
        m_previousBayFuelQty[1] = 0;  // no bay RCS-only tanks, and bay tanks never feed the RCS internal tank directly
        m_previousBayFuelQty[2] = GetXR1().m_pPayloadBay->GetPropellantMaxMass(PROP_TYPE::PT_SCRAM);
    }

    // allow auto-refueling if the user configured it in the prefs file OR if the ship is NOT landed (i.e., allow fuel MFD refueling in space)
    if (GetXR1().GetXR1Config()->OrbiterAutoRefuelingEnabled || (!GetXR1().GroundContact()))
        return;     // allow external refueling
    
    // Only disable refueling if:
    //   1) we are not actively refueling or cross-feeding, and 
    //   2) we did not just flow fuel from the bay tanks into the main tanks this timestep
    if (!GetXR1().IsRefuelingOrCrossfeeding())
    {
        // Note: we must always invoke DisableAutoRefuel here so that our m_previousInternalFuelQty and m_previousBayFuelQty values are always up-to-date
        DisableAutoRefuel(GetXR1().ph_main,  0, (GetXR1().m_MainFuelFlowedFromBayToMainThisTimestep == 0));
        DisableAutoRefuel(GetXR1().ph_rcs,   1, true);  // Note: there is no bay refuelling of the RCS tank
        DisableAutoRefuel(GetXR1().ph_scram, 2, (GetXR1().m_SCRAMFuelFlowedFromBayToMainThisTimestep == 0));
    }
    else    // we are refueling, so reset the fuel data to ensure we won't alter the fuel levels once refueling completes
    {
        ResetFuelData();
    }
}

// index = index into m_previousInternalFuelQty array
// bEnabled = true to enable fuel change to be backed out this timestep, false to not not change fuel levels this timestep
void PreventAutoRefuelPostStep::DisableAutoRefuel(PROPELLANT_HANDLE ph, const int index, const bool bEnabled)
{
    double internalFuelQty = oapiGetPropellantMass(ph);     // can't be const because we may set it below
    const double bayFuelQty = ((GetXR1().m_pPayloadBay != nullptr) ? GetXR1().m_pPayloadBay->GetPropellantMass(GetXR1().GetPropTypeForHandle(ph)) : 0);
    const double prevInternalFuelQty = m_previousInternalFuelQty[index];

    if (bEnabled && (prevInternalFuelQty >= 0))  // only check if we are enabled for this timestep AND we have valid data
    {
        // check whether fuel qty went UP since last timestep
        if (internalFuelQty > prevInternalFuelQty)
        {
            // We want to ALLOW payload tanks to refuel us.  Therefore, check whether the *bay fuel quantity* 
            // has changed as well.  If not, then Orbiter is refueling us.  If it *did* change,
            // however, then it means that a payload tank flowed the fuel, so we want to allow that.
            if (bayFuelQty == m_previousBayFuelQty[index])
            {
                // Orbiter is refueling us!  Back out the fuel change.
                // NOTE: this should only reset the *internal* tank: it should never affect the bay tanks
                GetVessel().SetPropellantMass(ph, prevInternalFuelQty);  
                internalFuelQty = prevInternalFuelQty;      // back out existing fuel qty as well (keep in sync w/new value)
                GetXR1().m_backedOutOrbiterCoreAutoRefuelThisFrame = true;
            }
        }
    }

    // remember these values for the next frame
    m_previousInternalFuelQty[index] = internalFuelQty;
    m_previousBayFuelQty[index] = bayFuelQty; 
}

//---------------------------------------------------------------------------

BoilOffPostStep::BoilOffPostStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel),
    m_stream1(nullptr), m_stream2(nullptr), m_level(0)
{
    // create the particle streams if the parent vessel supports them
    if (GetXR1().m_pBoilOffExhaustParticleStreamSpec != nullptr)
        m_stream1 = GetVessel().AddParticleStream(GetXR1().m_pBoilOffExhaustParticleStreamSpec, BOIL_OFF_PARTICLE_STREAM_POS1, BOIL_OFF_PARTICLE_STREAM_DIR1, &m_level);

    if (GetXR1().m_pBoilOffExhaustParticleStreamSpec != nullptr)
        m_stream2 = GetVessel().AddParticleStream(GetXR1().m_pBoilOffExhaustParticleStreamSpec, BOIL_OFF_PARTICLE_STREAM_POS2, BOIL_OFF_PARTICLE_STREAM_DIR2, &m_level);
}

// destructor
BoilOffPostStep::~BoilOffPostStep()
{
    if (m_stream1 != nullptr)
        GetVessel().DelExhaustStream(m_stream1);

    if (m_stream2 != nullptr)
        GetVessel().DelExhaustStream(m_stream2);
}

void BoilOffPostStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // sanity-check
    if (GetXR1().m_pBoilOffExhaustParticleStreamSpec == nullptr)
        return;

    // Boil-off only active if:
    // 1) ship is in GROUND CONTACT
    // 2) there is any MAIN FUEL remaining on board
    double newLevel = 0;  // assume disabled

    if (GetVessel().GroundContact() && (GetVessel().GetPropellantMass(GetXR1().ph_main) > 0))
    {
        // Note: if you don't want the exhaust to be visible outside of an atmosphere,
        // define the PARTICLESTREAMSPEC with PARTICLESTREAMSPEC::ATM_PLOG
        newLevel = 1.0;
    }

    m_level = newLevel;
}
