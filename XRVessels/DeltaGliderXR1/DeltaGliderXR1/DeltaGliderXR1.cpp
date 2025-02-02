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

#define ORBITER_MODULE

#include "DeltaGliderXR1.h"
#include "DlgCtrl.h"
#include "XR1Areas.h"   // for XR1_VCPANEL_TEXTURE_xxx definitions

// ==============================================================
// API callback interface
// ==============================================================

// --------------------------------------------------------------
// Module initialisation
// --------------------------------------------------------------
DLLCLBK void InitModule (HINSTANCE hModule)
{
    g_hDLL = hModule;
    oapiRegisterCustomControls(hModule);
}

// --------------------------------------------------------------
// Module cleanup
// NOTE: this is called even if fast shutdown is enabled.
// --------------------------------------------------------------
DLLCLBK void ExitModule (HINSTANCE hModule)
{
    oapiUnregisterCustomControls(hModule);
}

// --------------------------------------------------------------
// Vessel initialisation
// --------------------------------------------------------------
DLLCLBK VESSEL *ovcInit (OBJHANDLE vessel, int flightmodel)
{
#ifdef _DEBUG
    // NOTE: _CRTDBG_CHECK_ALWAYS_DF is too slow
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF |
                   _CRTDBG_CHECK_CRT_DF | 
                   _CRTDBG_LEAK_CHECK_DF); 
#endif

    return new DeltaGliderXR1(vessel, flightmodel, new XR1ConfigFileParser());
}

// --------------------------------------------------------------
// Vessel cleanup
// Note: this is only called if fast shutdown is DISABLED.
// --------------------------------------------------------------

// NOTE: must receive this as a VESSEL2 ptr because that's how Orbiter calls it
DLLCLBK void ovcExit(VESSEL2 *vessel)
{
    // NOTE: in order to free up VESSEL2 data, you must add an empty virtual destructor to the VESSEL2 class in VesselAPI.h
    
    // This is a hack so that the VESSEL3_EXT, DeltaGliderXR1, and VESSEL3 destructors will be invoked.
    // Invokes DeltaGliderXR1 destructor -> VESSEL3_EXT destructor -> VESSEL3 destructor
    VESSEL3_EXT *pXR1 = reinterpret_cast<VESSEL3_EXT *>((reinterpret_cast<void **>(vessel))-1);  // bump vptr to VESSEL3_EXT subclass, which has a virtual destructor
    delete pXR1;
}
