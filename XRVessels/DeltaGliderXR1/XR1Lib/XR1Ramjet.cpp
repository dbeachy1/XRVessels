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
// XR1Ramjet.cpp
// Implementation for the delta glider XR1Ramjet engine
// ==============================================================

#include "XR1Ramjet.h"
#include "DeltaGliderXR1.h"
#include "stdio.h"  

// constructor
XR1Ramjet::XR1Ramjet (DeltaGliderXR1 *_vessel): 
    vessel(_vessel)
{
	nthdef = 0;    // no thrusters associated yet

    for (int i=0; i < 2; i++)   // enable engines @ 100%
        m_integrity[i] = 1.0;
}

// destructor
XR1Ramjet::~XR1Ramjet ()
{
	if (nthdef) 
    {  
        // delete list of thruster definitions
		for (UINT i = 0; i < nthdef; i++)
			delete thdef[i];
		delete []thdef;
	}
}

// add new thruster definition to list
void XR1Ramjet::AddThrusterDefinition (THRUSTER_HANDLE th,
	double Qr, double Ai, double Tb_max, double dmf_max)
{
	THDEF *thd   = new THDEF;
	thd->th      = th;
	thd->Qr      = Qr;
	thd->Ai      = Ai;
	thd->Tb_max  = Tb_max;
	thd->dmf_max = dmf_max;
	thd->dmf     = 0.0;
	thd->F       = 0.0;
	for (int i = 0; i < 3; i++) thd->T[i] = 0.0;

	THDEF **tmp = new THDEF*[nthdef+1];
	if (nthdef) {
		memcpy (tmp, thdef, nthdef * sizeof(THDEF*));
		delete []thdef;
	}
	thdef = tmp;
	thdef[nthdef++] = thd;
}

// calculate current thrust force for all engines
void XR1Ramjet::Thrust (double *F) const
{
	const OBJHANDLE hBody = vessel->GetAtmRef();
	const ATMCONST *atm = (hBody ? oapiGetPlanetAtmConstants (hBody) : 0);

	if (atm)   // atmospheric parameters available
    { 
		double M, Fs, T0, Td, Tb, Tb0, Te, p0, pd, D, rho, cp, v0, ve, tr, lvl, dma, dmf, precov, dmafac;
		// ORG: const double dma_scale = 2.7e-4;
        const double dma_scale = SCRAM_DMA_SCALE;  // {DEB} tweaked for mach 17 (value is 1/2 original)

		M   = vessel->GetMachNumber();                     // Mach number
		T0  = vessel->GetExternalTemperature();                        // freestream temperature
		p0  = vessel->GetAtmPressure();                    // freestream pressure
		rho = vessel->GetAtmDensity();                     // freestream density
		cp  = atm->gamma * atm->R / (atm->gamma-1.0);      // specific heat (pressure)
		v0  = M * sqrt (atm->gamma * atm->R * T0);         // freestream velocity
		tr  = (1.0 + 0.5*(atm->gamma-1.0) * M*M);          // temperature ratio
		Td  = T0 * tr;                                     // diffuser temperature
		pd  = p0 * pow (Td/T0, atm->gamma/(atm->gamma-1.0)) * GetXR1().scramdoor_proc; // diffuser pressure; will be ZERO if SCRAM doors closed

        // {DEB} modified this for high-altitude flight: new limit is mach 17 (doubled)
		// ORG: precov = max (0.0, 1.0 - (0.075*pow(max(M,1.0)-1.0, 1.35)) ); // pressure recovery
        precov = max (0.0, 1.0 - (0.075*pow(max(M,1.0)-1.0, SCRAM_PRESSURE_RECOVERY_MULT)) ); // pressure recovery : good for Mach 17 now

        // NOTE: if the SCRAM doors are not fully open the throttle will be closed already, so no need to check the doors here

        dmafac = dma_scale*precov*pd;   // will be ZERO if SCRAM doors closed

        // DEBUG: sprintf(oapiDebugString(), "Td=%lf, precov=%lf, dmafac=%lf, pd=%lf" , Td, precov, dmafac, pd);

		for (UINT i = 0; i < nthdef; i++) 
        {
			Tb0 = thdef[i]->Tb_max;                        // max burner temperature

            lvl  = vessel->GetThrusterLevel(thdef[i]->th); // throttle level

            // NOTE: engine temp is checked in DMG file

            if ((pd > 0) & (Tb0 > Td))    // any diffuser pressure AND are we within operational range?
            {                                
				D    = (Tb0-Td) / (thdef[i]->Qr/cp - Tb0); // max fuel-to-air ratio (what if negative?)
                //dma  = rho * v0 * thdef[i]->Ai;          // air mass flow rate (DEB: Martin commented this out)
                dma = dmafac * thdef[i]->Ai;               // air mass flow rate [kg/s]

                // {DEB} reduce effective level based on dmf_max limit
                // FORMULA: throttleFrac = D * dma / max_dmf, where x = throttle fraction limit (0...n)
                double throttleFrac = D * dma / thdef[i]->dmf_max;
                
                // if throttleFrac > 1.0, it means that we need to reduce the throttle sensitivity by that fraction; i.e., reduce the effective throttle setting
                if (throttleFrac > 1.0)
                    lvl /= throttleFrac;   // reduce effective level so that 100% throttle == max possible fuel flow

				D   *= lvl;                                // actual fuel-to-air ratio
				dmf  = D * dma;                            // fuel mass flow rate
                
                // debug: if (i == 0) sprintf(oapiDebugString(), "throttleFrac=%lf, D=%lf, dma=%lf, dmf=%lf, dmf_max=%lf", throttleFrac, D, dma, dmf, thdef[i]->dmf_max);

				if (dmf > thdef[i]->dmf_max)               // max fuel rate exceeded
                {             
					dmf = thdef[i]->dmf_max;
					D = dmf/dma;
				}
				Tb   = (D*thdef[i]->Qr/cp + Td) / (1.0+D); // actual burner temperature
				Te   = Tb * pow (p0/pd, (atm->gamma-1.0)/atm->gamma); // exhaust temperature
                
                // bugfix: if exhaust temperature > burner temperature, we cannot continue
                if (Te > Tb)
                {
                    /* DEBUG ONLY
                    static double count = 0;
                    count++;
                    sprintf(oapiDebugString(), "Te > Tb count: %lf", count);
                    */
                    goto engines_off;
                }

				ve   = sqrt (2.0*cp*(Tb-Te));              // exhaust velocity
			    Fs  = (1.0+D)*ve - v0;                     // specific thrust

				thdef[i]->F = F[i] = max(0.0, Fs*dma * m_integrity[i]);    // thrust force * integrity fraction (0...1)

                // NEW CHECK: if no thrust, fuel flow is also zero
                if (thdef[i]->F == 0.0)
                    dmf = 0;    // no flow

				thdef[i]->dmf = dmf;
				thdef[i]->T[1] = Tb;
				thdef[i]->T[2] = Te;

			} 
            else   // overheating or SCRAM doors are closed!
            {                                       
engines_off:
				thdef[i]->F = F[i] = 0.0;
				thdef[i]->dmf = 0.0;
				thdef[i]->T[1] = thdef[i]->T[2] = Td;

			}
			thdef[i]->T[0] = Td;    // save diffuser temperature; may be very high, but we message the internal temp here for heat and display checks in the "Temp" method below
            thdef[i]->pd = pd;      // save diffuser pressure; will be ZERO if doors closed or out of atmosphere
		}
	} 
    else  // no atmospheric parameters or engines disabled
    {   
		for (UINT i = 0; i < nthdef; i++)
        {
            thdef[i]->dmf = 0.0;
			thdef[i]->F = F[i] = 0.0;
            thdef[i]->T[0] = thdef[i]->T[1] = thdef[i]->T[0] = vessel->GetExternalTemperature();  // set to external temperature
            thdef[i]->pd = 0;       // zero pressure
		}
	}
}

double XR1Ramjet::TSFC (UINT idx) const
{
	const double eps = 1e-5;
	return thdef[idx]->dmf/(thdef[idx]->F+eps);
}

// returns "visual" temperature used for display purposes and for heat checks
//   idx = 0 or 1 (left or right)
//   which = 0 (diffuser), 1 (burner), or 2 (exhaust)
// TODO: refactor 'which' to be an enum; the old integer constants are holdovers from the original DeltaGlider code
double XR1Ramjet::Temp(UINT idx, UINT which) const
{ 
    const double freestreamTemp = vessel->GetExternalTemperature();

    // DEFENSIVE CODE: clamp the temperature to freestream temp if SCRAM doors are closed.
    // This should never be necessary (i.e., is should be OK to fall through to the code below), but the code is complex and we want to be defensive here.
    if (vessel->scramdoor_status == DOOR_CLOSED)
        return freestreamTemp;

    double t = thdef[idx]->T[which] / SCRAM_COOLING; // adjusted for the XR1

    // Modify visual diffuser temperature based on diffuser pressure; this allows the temperature to rise gradually as the ship reenters the atmosphere,
    // giving the pilot time to close the SCRAM doors.
    const double mach = GetXR1().GetMachNumber();
    if (mach == 0)              // out of atmosphere?
    {
        // DEBUG: sprintf(oapiDebugString(), "XR1Ramjet::Temp: MACH=0");
        return freestreamTemp;  // return ext temp (avoid divide-by-zero below)
    }

    // NOTE: OK if pd is zero (or even negative, although that should never happen) 
    const double tdFrac = thdef[idx]->pd / (76923 * mach);  // once diffuser pressure reaches 2.0 million @ mach 26, temperature reaches full
    if (tdFrac < 1.0)
        t *= tdFrac;   // reduce temperature
    
    // DEBUG: sprintf(oapiDebugString(), "Td=%lf, tdFrac=%lf, pd=%lf, mach=%lf" , t, tdFrac, thdef[idx]->pd, mach);

    // if t < freestream temp, return the freestream temp
    if (t < freestreamTemp)
        t = freestreamTemp;

    return t;
}
