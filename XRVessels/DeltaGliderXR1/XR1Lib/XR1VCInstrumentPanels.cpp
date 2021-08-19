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
// XR1InstrumentPanels.cpp
// Instrument panels for the DG-XR1.
// ==============================================================

#include "OrbiterSDK.h"
#include "resource.h"
#include "AreaIDs.h"

#include "XR1InstrumentPanels.h"
#include "XR1VCPanelAreas.h"
#include "XR1MainPanelAreas.h"
#include "XR1UpperPanelAreas.h"
#include "XR1LowerPanelAreas.h"
#include "XR1AngularDataComponent.h"
#include "XR1HUD.h"
#include "XR1MFDComponent.h"
#include "XR1ThrottleQuadrantComponents.h"
#include "XR1MainPanelComponents.h"

// 3D cockpit coordinates
static const VECTOR3 threeDCockpktCoordinates = _V(0, 1.467, 6.782);

// ------------------------------------------------------------------------

// VC Convenience macros
#define ADD_TOGGLE_SWITCH(areaID, pHandler, activatedStatus)  AddArea(new VCToggleSwitchArea(*this, _COORD2(-1, -1), areaID, pHandler, activatedStatus))

//
// Construct our Virtual Cockpit Pilot panel and passenger views
//

// Constructor
// vessel = our parent vessel
// panelID = unique panel ID
XR1VCPilotInstrumentPanel::XR1VCPilotInstrumentPanel(DeltaGliderXR1& vessel, int panelID) :
    XR1InstrumentPanel(vessel, panelID)
{
    // NOTE: THE PANEL TEXTURE IDS DON'T REALLY MAKE SENSE FROM A LOGICAL GROUPING STANDPOINT: THE ORIGINAL DG VC TEXTURES ARE SORT OF SCREWED UP IN
    // THAT THEY ARE NOT NECESSARY LOCATED IN THE SAME SECTION OF THE COCKPIT.  So do not take the "left", "right", and "center" sections as
    // always making sense.  They are merely the best approximation I can use for a name.  

    // create our components
    AddComponent(new MFDComponent(*this, _COORD2(112, 214), MFD_LEFT, XR1_VCPANEL_TEXTURE_CENTER, MESHGRP_VC_LMFDDISP));
    AddComponent(new MFDComponent(*this, _COORD2(112, 234), MFD_RIGHT, XR1_VCPANEL_TEXTURE_CENTER, MESHGRP_VC_RMFDDISP));
    AddComponent(new AngularDataComponent(*this, _COORD2(56, 32), XR1_VCPANEL_TEXTURE_RIGHT));
    AddComponent(new ScramPanelComponent(*this, _COORD2(141, 2), XR1_VCPANEL_TEXTURE_CENTER));
    AddComponent(new MainHoverPanelComponent(*this, _COORD2(1, 0), XR1_VCPANEL_TEXTURE_CENTER));

    //
    // Create our areas
    //

    // create the four HUD mode button areas
    // Note: no redrawing for these areas, so no meshTextureID required
    for (int i = 0; i < 4; i++)
        AddArea(new VCHudModeButtonArea(*this, _COORD2(-1, -1), AID_HUDBUTTON1 + i));    // panelCoordinates are ignored for these areas

    // create the six autopilot (NAV mode) button areas
    // Note: no redrawing for these areas, so no meshTextureID required
    for (int i = 0; i < 6; i++)
        AddArea(new VCAutopilotButtonArea(*this, _COORD2(-1, -1), AID_NAVBUTTON1 + i)); // panelCoordinates are ignored for these areas

    AddArea(new HoverBalanceVerticalGaugeArea(*this, _COORD2(97, 95), AID_HBALANCEDISP, XR1_VCPANEL_TEXTURE_CENTER));
    AddArea(new ScramPitchVerticalGaugeArea(*this, _COORD2(236, 86), AID_GIMBALSCRAMDISP, XR1_VCPANEL_TEXTURE_CENTER));
    AddArea(new MainPitchVerticalGaugeArea(*this, _COORD2(227, 2), AID_PGIMBALMAINDISP, XR1_VCPANEL_TEXTURE_CENTER));
    AddArea(new MainYawHorizontalGaugeArea(*this, _COORD2(6, 107), AID_YGIMBALMAINDISP, XR1_VCPANEL_TEXTURE_CENTER));
    AddArea(new ScramTempGaugeArea(*this, _COORD2(6, 10), AID_SCRAMTEMPDISP, XR1_VCPANEL_TEXTURE_LEFT));

    AddArea(new AOAAnalogGaugeArea(*this, _COORD2(17, 181), AID_AOAINSTR, XR1_VCPANEL_TEXTURE_LEFT));
    AddArea(new SlipAnalogGaugeArea(*this, _COORD2(109, 181), AID_SLIPINSTR, XR1_VCPANEL_TEXTURE_LEFT));
    AddArea(new WingLoadAnalogGaugeArea(*this, _COORD2(111, 17), AID_LOADINSTR, XR1_VCPANEL_TEXTURE_LEFT));
    AddArea(new ArtificialHorizonArea(*this, _COORD2(0, 159), AID_HORIZON, XR1_VCPANEL_TEXTURE_CENTER));

    AddArea(new ElevatorTrimArea(*this, _COORD2(252, 0), AID_ELEVATORTRIM, XR1_VCPANEL_TEXTURE_CENTER));

    AddArea(new DoorIndicatorArea(*this, _COORD2(1, 127), AID_GEARINDICATOR, XR1_VCPANEL_TEXTURE_CENTER, &GetXR1().gear_status, IDB_INDICATOR_UD, &GetXR1().gear_proc));
    AddArea(new DoorIndicatorArea(*this, _COORD2(32, 127), AID_NOSECONEINDICATOR, XR1_VCPANEL_TEXTURE_CENTER, &GetXR1().nose_status, IDB_INDICATOR_OC, &GetXR1().nose_proc));

    // panelCoordinates are ignored for these interactive areas; coordinates for these clickable areas are defined in our Activate() method.
    AddArea(new MWSArea(*this, _COORD2(-1, -1), AID_MWS));             // has a custom Redraw3D method, so no VC panel texture necessary                
    AddArea(new RCSModeArea(*this, _COORD2(-1, -1), AID_RCSMODE));         // has a custom Redraw3D method, so no VC panel texture necessary                
    AddArea(new AFCtrlArea(*this, _COORD2(-1, -1), AID_AFCTRLMODE));      // has a custom Redraw3D method, so no VC panel texture necessary                
    AddArea(new MainThrottleArea(*this, _COORD2(-1, -1), AID_ENGINEMAIN));      // has a custom Redraw3D method, so no VC panel texture necessary                
    AddArea(new LargeHoverThrottleArea(*this, _COORD2(-1, -1), AID_ENGINEHOVER));     // has a custom Redraw3D method, so no VC panel texture necessary                
    AddArea(new ScramThrottleArea(*this, _COORD2(-1, -1), AID_ENGINESCRAM));     // has a custom Redraw3D method, so no VC panel texture necessary
    AddArea(new MainPitchSwitchArea(*this, _COORD2(-1, -1), AID_PGIMBALMAIN));     // no redrawing, so no VC panel texture necessary
    AddArea(new ScramPitchSwitchArea(*this, _COORD2(-1, -1), AID_GIMBALSCRAM));     // no redrawing, so no VC panel texture necessary
    AddArea(new HoverBalanceSwitchArea(*this, _COORD2(-1, -1), AID_HOVERBALANCE));    // no redrawing, so no VC panel texture necessary
    AddArea(new MainYawSwitchArea(*this, _COORD2(-1, -1), AID_YGIMBALMAIN));     // no redrawing, so no VC panel texture necessary
    // SimpleButtonArea has a custom Redraw3D method, so no VC panel texture necessary                
    AddArea(new SimpleButtonArea(*this, _COORD2(-1, -1), AID_HBALANCECENTER, &GetXR1().m_hoverCenteringMode, MESHGRP_VC_HBALANCECNT));
    AddArea(new SimpleButtonArea(*this, _COORD2(-1, -1), AID_GIMBALSCRAMCENTER, &GetXR1().m_scramCenteringMode, MESHGRP_VC_SCRAMGIMBALCNT));
    AddArea(new SimpleButtonArea(*this, _COORD2(-1, -1), AID_PGIMBALMAINCENTER, &GetXR1().m_mainPitchCenteringMode, MESHGRP_VC_PGIMBALCNT));

    AddArea(new SimpleButtonArea(*this, _COORD2(-1, -1), AID_YGIMBALMAINCENTER, &GetXR1().m_mainYawCenteringMode, MESHGRP_VC_YGIMBALCNT));
    AddArea(new SimpleButtonArea(*this, _COORD2(-1, -1), AID_YGIMBALMAINDIV, &GetXR1().m_mainDivMode, MESHGRP_VC_YGIMBALDIV));
    AddArea(new SimpleButtonArea(*this, _COORD2(-1, -1), AID_YGIMBALMAINAUTO, &GetXR1().m_mainAutoMode, MESHGRP_VC_YGIMBALAUTO));

    AddArea(new HudIntensitySwitchArea(*this, _COORD2(-1, -1), AID_HUDINTENSITY));    // no redrawing, so no VC panel texture necessary
    AddArea(new HudColorButtonArea(*this, _COORD2(-1, -1), AID_HUDCOLOR));        // no redrawing, so no VC panel texture necessary

    // add toggle switches (panelCoordinates are ignored for these)
    // Note: the animation for each of these switches in the VC is handled by a call to 'SetXRAnimation' inside each switch handler, so no VC panel texture is necessary
    ADD_TOGGLE_SWITCH(AID_GEARDOWN, &DeltaGliderXR1::ActivateLandingGear, DoorStatus::DOOR_OPENING);
    ADD_TOGGLE_SWITCH(AID_GEARUP, &DeltaGliderXR1::ActivateLandingGear, DoorStatus::DOOR_CLOSING);
    ADD_TOGGLE_SWITCH(AID_NCONEOPEN, &DeltaGliderXR1::ActivateNoseCone, DoorStatus::DOOR_OPENING);
    ADD_TOGGLE_SWITCH(AID_NCONECLOSE, &DeltaGliderXR1::ActivateNoseCone, DoorStatus::DOOR_CLOSING);
    ADD_TOGGLE_SWITCH(AID_OLOCKOPEN, &DeltaGliderXR1::ActivateOuterAirlock, DoorStatus::DOOR_OPENING);
    ADD_TOGGLE_SWITCH(AID_OLOCKCLOSE, &DeltaGliderXR1::ActivateOuterAirlock, DoorStatus::DOOR_CLOSING);
    ADD_TOGGLE_SWITCH(AID_ILOCKOPEN, &DeltaGliderXR1::ActivateInnerAirlock, DoorStatus::DOOR_OPENING);
    ADD_TOGGLE_SWITCH(AID_ILOCKCLOSE, &DeltaGliderXR1::ActivateInnerAirlock, DoorStatus::DOOR_CLOSING);
    // TODO: ADD VC SWITCH FOR AIRLOCK PRESSURIZE / DEPRESSUREIZE
    ADD_TOGGLE_SWITCH(AID_RCOVEROPEN, &DeltaGliderXR1::ActivateRCover, DoorStatus::DOOR_OPENING);
    ADD_TOGGLE_SWITCH(AID_RCOVERCLOSE, &DeltaGliderXR1::ActivateRCover, DoorStatus::DOOR_CLOSING);
    ADD_TOGGLE_SWITCH(AID_RADIATOREX, &DeltaGliderXR1::ActivateRadiator, DoorStatus::DOOR_OPENING);
    ADD_TOGGLE_SWITCH(AID_RADIATORIN, &DeltaGliderXR1::ActivateRadiator, DoorStatus::DOOR_CLOSING);
    ADD_TOGGLE_SWITCH(AID_HATCHOPEN, &DeltaGliderXR1::ActivateHatch, DoorStatus::DOOR_OPENING);
    ADD_TOGGLE_SWITCH(AID_HATCHCLOSE, &DeltaGliderXR1::ActivateHatch, DoorStatus::DOOR_CLOSING);
    ADD_TOGGLE_SWITCH(AID_LADDEREX, &DeltaGliderXR1::ActivateLadder, DoorStatus::DOOR_OPENING);
    ADD_TOGGLE_SWITCH(AID_LADDERIN, &DeltaGliderXR1::ActivateLadder, DoorStatus::DOOR_CLOSING);
}

// Activate and initialize this panel
// Invoked from VESSEL2's InitPanel method.
// Load our surface bitmaps. The inverse of this method is 'Deactivate'
// Returns: true on success, false on error (e.g., a bitmap failed to load)
bool XR1VCPilotInstrumentPanel::Activate()
{
    const double buttonRadius = .005;   // radius of small buttons

    GetVessel().SetXRCameraDirection(_V(0, 0, 1)); // center, facing forward

    // set for Pilot
    GetVessel().SetCameraOffset(threeDCockpktCoordinates);
    GetVessel().SetCameraShiftRange(_V(0, 0, 0.1), _V(-0.2, 0, 0), _V(0.2, 0, 0));
    oapiVCSetNeighbours(1, 2, -1, -1);

    // activate all our areas, including our components' areas; this will invoke oapiVCRegisterArea, etc.
    ActivateAllAreas();

    //
    // Define interactive (clickable) areas in the VC.  This is done here rather than in the normal areas above 
    //
    oapiVCSetAreaClickmode_Quadrilateral(AID_ELEVATORTRIM, _V(0.2873, 1.0276, 7.2286), _V(0.3040, 1.0327, 7.2151), _V(0.2873, 0.9957, 7.2165), _V(0.3040, 1.0008, 7.2030));
    oapiVCSetAreaClickmode_Spherical(AID_MWS, _V(0.0755, 1.2185, 7.3576), 0.013);
    oapiVCSetAreaClickmode_Spherical(AID_RCSMODE, _V(-0.3358, 1.0683, 7.2049), 0.02);
    oapiVCSetAreaClickmode_Spherical(AID_AFCTRLMODE, _V(-0.3351, 1.1153, 7.2131), 0.02);
    oapiVCSetAreaClickmode_Quadrilateral(AID_ENGINEMAIN, _V(-0.372, 0.918, 6.905), _V(-0.279, 0.918, 6.905), _V(-0.372, 0.885, 7.11), _V(-0.279, 0.885, 7.11));
    oapiVCSetAreaClickmode_Quadrilateral(AID_ENGINEHOVER, _V(-0.44, 0.87, 6.81), _V(-0.35, 0.87, 6.81), _V(-0.44, 0.95, 6.91), _V(-0.35, 0.95, 6.91));
    oapiVCSetAreaClickmode_Quadrilateral(AID_ENGINESCRAM, _V(-0.45, 0.98, 6.94), _V(-0.39, 0.98, 6.94), _V(-0.45, 0.95, 7.07), _V(-0.39, 0.95, 7.07));


    oapiVCSetAreaClickmode_Quadrilateral(AID_HOVERBALANCE, _V(-0.2691, 1.1353, 7.27), _V(-0.2606, 1.1346, 7.2729), _V(-0.2691, 1.1065, 7.2625), _V(-0.2606, 1.1058, 7.2654));
    oapiVCSetAreaClickmode_Quadrilateral(AID_PGIMBALMAIN, _V(-0.3739, 1.1105, 7.1478), _V(-0.3593, 1.108, 7.1618), _V(-0.3728, 1.0875, 7.1426), _V(-0.3582, 1.085, 7.1566));
    oapiVCSetAreaClickmode_Quadrilateral(AID_GIMBALSCRAM, _V(-0.2666, 1.0629, 7.2484), _V(-0.248, 1.0613, 7.2548), _V(-0.2666, 1.04, 7.2425), _V(-0.248, 1.0384, 7.2488));
    oapiVCSetAreaClickmode_Quadrilateral(AID_YGIMBALMAIN, _V(-0.3728, 1.0522, 7.1301), _V(-0.3566, 1.0494, 7.1460), _V(-0.3720, 1.0324, 7.1259), _V(-0.3558, 1.0293, 7.1416));
    oapiVCSetAreaClickmode_Quadrilateral(AID_HUDINTENSITY, _V(0.2327, 1.1682, 7.3136), _V(0.2500, 1.1682, 7.3136), _V(0.2327, 1.1300, 7.3134), _V(0.2500, 1.1300, 7.3134));
    oapiVCSetAreaClickmode_Spherical(AID_HUDCOLOR, _V(0.2511, 1.1456, 7.3031), buttonRadius);

    // Gimble buttons
    oapiVCSetAreaClickmode_Spherical(AID_YGIMBALMAINCENTER, _V(-0.393, 1.065, 7.1074), buttonRadius);
    oapiVCSetAreaClickmode_Spherical(AID_YGIMBALMAINDIV, _V(-0.394, 1.053, 7.1073), buttonRadius);
    oapiVCSetAreaClickmode_Spherical(AID_YGIMBALMAINAUTO, _V(-0.395, 1.040, 7.1072), buttonRadius);
    oapiVCSetAreaClickmode_Spherical(AID_PGIMBALMAINCENTER, _V(-0.3708, 1.0743, 7.1357), buttonRadius);
    oapiVCSetAreaClickmode_Spherical(AID_HBALANCECENTER, _V(-0.2684, 1.0972, 7.2555), buttonRadius);
    oapiVCSetAreaClickmode_Spherical(AID_GIMBALSCRAMCENTER, _V(-0.2672, 1.0256, 7.2336), buttonRadius);

    // Left MFD clickable areas
    oapiVCSetAreaClickmode_Quadrilateral(AID_MFD1_LBUTTONS, _V(-0.2301, 1.1592, 7.3322), _V(-0.2161, 1.1592, 7.3322), _V(-0.2301, 1.0302, 7.2852), _V(-0.2161, 1.0302, 7.2852));
    oapiVCSetAreaClickmode_Quadrilateral(AID_MFD1_RBUTTONS, _V(-0.023942, 1.1592, 7.3322), _V(-0.009927, 1.1592, 7.3322), _V(-0.023942, 1.0302, 7.2852), _V(-0.009927, 1.0302, 7.2852));
    oapiVCSetAreaClickmode_Spherical(AID_MFD1_PWR, _V(-0.1914, 1.009, 7.2775), 0.01);
    oapiVCSetAreaClickmode_Spherical(AID_MFD1_SEL, _V(-0.0670, 1.009, 7.2775), 0.01);
    oapiVCSetAreaClickmode_Spherical(AID_MFD1_MNU, _V(-0.0485, 1.009, 7.2775), 0.01);

    // Right MFD clickable areas
    oapiVCSetAreaClickmode_Quadrilateral(AID_MFD2_LBUTTONS, _V(0.009927, 1.1592, 7.3322), _V(0.023942, 1.1592, 7.3322), _V(0.009927, 1.0302, 7.2852), _V(0.023942, 1.0302, 7.2852));
    oapiVCSetAreaClickmode_Quadrilateral(AID_MFD2_RBUTTONS, _V(0.216058, 1.1592, 7.3322), _V(0.230072, 1.1592, 7.3322), _V(0.216058, 1.0302, 7.2852), _V(0.230072, 1.0302, 7.2852));
    oapiVCSetAreaClickmode_Spherical(AID_MFD2_PWR, _V(0.0483, 1.009, 7.2775), 0.01);
    oapiVCSetAreaClickmode_Spherical(AID_MFD2_SEL, _V(0.1726, 1.009, 7.2775), 0.01);
    oapiVCSetAreaClickmode_Spherical(AID_MFD2_MNU, _V(0.1913, 1.009, 7.2775), 0.01);

    // Switches and toggle levers
    oapiVCSetAreaClickmode_Spherical(AID_GEARDOWN, _V(0.3008, 1.0197, 7.1656), 0.02);
    oapiVCSetAreaClickmode_Spherical(AID_GEARUP, _V(0.3052, 0.9061, 7.1280), 0.02);

    oapiVCSetAreaClickmode_Spherical(AID_NCONEOPEN, _V(0.3317, 1.1078, 7.1968), 0.02);
    oapiVCSetAreaClickmode_Spherical(AID_NCONECLOSE, _V(0.3281, 1.0302, 7.1630), 0.02);

    oapiVCSetAreaClickmode_Spherical(AID_OLOCKOPEN, _V(0.2506, 1.0884, 7.2866), 0.01);
    oapiVCSetAreaClickmode_Spherical(AID_OLOCKCLOSE, _V(0.2506, 1.1054, 7.2866), 0.01);

    oapiVCSetAreaClickmode_Spherical(AID_ILOCKOPEN, _V(0.2824, 1.0981, 7.2611), 0.01);
    oapiVCSetAreaClickmode_Spherical(AID_ILOCKCLOSE, _V(0.2824, 1.1151, 7.2611), 0.01);

    oapiVCSetAreaClickmode_Spherical(AID_RCOVEROPEN, _V(0.2508, 1.0420, 7.2694), 0.01);
    oapiVCSetAreaClickmode_Spherical(AID_RCOVERCLOSE, _V(0.2508, 1.0590, 7.2694), 0.01);

    oapiVCSetAreaClickmode_Spherical(AID_RADIATOREX, _V(0.2582, 0.9448, 7.22), 0.01);
    oapiVCSetAreaClickmode_Spherical(AID_RADIATORIN, _V(0.2582, 0.9618, 7.22), 0.01);

    oapiVCSetAreaClickmode_Spherical(AID_HATCHOPEN, _V(0.2511, 0.9921, 7.2507), 0.01);
    oapiVCSetAreaClickmode_Spherical(AID_HATCHCLOSE, _V(0.2511, 1.0091, 7.2507), 0.01);

    oapiVCSetAreaClickmode_Spherical(AID_LADDEREX, _V(0.2889, 1.0537, 7.2388), 0.01);
    oapiVCSetAreaClickmode_Spherical(AID_LADDERIN, _V(0.2889, 1.0707, 7.2388), 0.01);

    // define the four HUD clickable buttons and trigger a redraw of each in case the mode changed in 2D mode
    for (int i = 0; i < 4; i++)
    {
        int buttonAreaID = AID_HUDBUTTON1 + i;
        oapiVCSetAreaClickmode_Spherical(buttonAreaID, _V(-0.1094, 1.4174 + 0.0101 * i, 7.0406 + i * 0.0070), 0.0065);
        TriggerRedrawArea(buttonAreaID);
    }

    // define the HUD display in the VC
    static VCHUDSPEC huds = { 1, MESHGRP_VC_HUDDISP, {0,1.462,7.09}, 0.15 };
    oapiVCRegisterHUD(&huds);  // HUD parameters

    // define the six autopilot (NAV mode) clickable buttons and trigger a redraw of each in case the mode changed in 2D mode
    for (int i = 0; i < 6; i++)
    {
        int buttonAreaID = AID_NAVBUTTON1 + i;
        oapiVCSetAreaClickmode_Spherical(AID_NAVBUTTON1 + i, _V(0.11264, 1.461821 - 0.0132572 * i, 7.071551 - 0.0090569 * i), 0.0065);
        TriggerRedrawArea(buttonAreaID);
    }

    // all finished; set current camera position flag
    GetXR1().campos = DeltaGliderXR1::CAMERA_POSITION::CAM_VCPILOT;
    return true;
}

//
// Construct our Virtual Cockpit Passenger views ("panels", as far as Orbiter is concerned)
//

// Passenger #1 (left-front)

// Constructor
// vessel = our parent vessel
// panelID = unique panel ID
XR1VCPassenger1InstrumentPanel::XR1VCPassenger1InstrumentPanel(DeltaGliderXR1& vessel, int panelID) :
    XR1InstrumentPanel(vessel, panelID)
{
}

// Activate and initialize this panel
// Invoked from VESSEL2's InitPanel method.
// Load our surface bitmaps. The inverse of this method is 'Deactivate'
// Returns: true on success, false on error (e.g., a bitmap failed to load)
bool XR1VCPassenger1InstrumentPanel::Activate()
{
    GetVessel().SetXRCameraDirection(_V(0, 0, 1)); // center, facing forward
    GetVessel().SetCameraOffset(_V(-0.7, 1.15, 5.55));
    GetVessel().SetCameraMovement(_V(0.2, -0.05, 0.3), -10 * RAD, 10 * RAD, _V(-0.3, 0, 0), 80 * RAD, 0, _V(0.4, 0, 0), -90 * RAD, 0);
    GetXR1().campos = DeltaGliderXR1::CAMERA_POSITION::CAM_VCPSNGR1;
    oapiVCSetNeighbours(-1, 2, 0, 3);

    return true;
}

// ------------------------------------------------------------------------

//
// Construct our Passenger #2 panel (right-front)
//

// Constructor
// vessel = our parent vessel
// panelID = unique panel ID

XR1VCPassenger2InstrumentPanel::XR1VCPassenger2InstrumentPanel(DeltaGliderXR1& vessel, int panelID) :
    XR1InstrumentPanel(vessel, panelID)
{
}

// Activate and initialize this panel
// Invoked from VESSEL2's InitPanel method.
// Load our surface bitmaps. The inverse of this method is 'Deactivate'
// Returns: true on success, false on error (e.g., a bitmap failed to load)
bool XR1VCPassenger2InstrumentPanel::Activate()
{
    GetVessel().SetXRCameraDirection(_V(0, 0, 1)); // center, facing forward
    GetVessel().SetCameraOffset(_V(0.7, 1.15, 5.55));
    GetVessel().SetCameraMovement(_V(-0.2, -0.05, 0.3), 10 * RAD, 10 * RAD, _V(-0.4, 0, 0), 90 * RAD, 0, _V(0.3, 0, 0), -80 * RAD, 0);
    GetXR1().campos = DeltaGliderXR1::CAMERA_POSITION::CAM_VCPSNGR2;
    oapiVCSetNeighbours(1, -1, 0, 4);

    return true;
}

//
// Construct our Virtual Cockpit Passenger #3 panel
//

// Constructor
// vessel = our parent vessel
// panelID = unique panel ID

XR1VCPassenger3InstrumentPanel::XR1VCPassenger3InstrumentPanel(DeltaGliderXR1& vessel, int panelID) :
    XR1InstrumentPanel(vessel, panelID)
{
}

// Activate and initialize this panel
// Invoked from VESSEL2's InitPanel method.
// Load our surface bitmaps. The inverse of this method is 'Deactivate'
// Returns: true on success, false on error (e.g., a bitmap failed to load)
bool XR1VCPassenger3InstrumentPanel::Activate()
{
    GetVessel().SetXRCameraDirection(_V(0, 0, 1)); // center, facing forward
    GetVessel().SetCameraOffset(_V(-0.8, 1.2, 4.4));
    GetVessel().SetCameraMovement(_V(0.4, 0, 0), 0, 0, _V(-0.3, 0, 0), 70 * RAD, 0, _V(0.4, 0, 0), -90 * RAD, 0);
    GetXR1().campos = DeltaGliderXR1::CAMERA_POSITION::CAM_VCPSNGR3;
    oapiVCSetNeighbours(-1, 4, 1, -1);

    return true;
}


//
// Construct our Virtual Cockpit Passenger #4 panel
//

// Constructor
// vessel = our parent vessel
// panelID = unique panel ID

XR1VCPassenger4InstrumentPanel::XR1VCPassenger4InstrumentPanel(DeltaGliderXR1& vessel, int panelID) :
    XR1InstrumentPanel(vessel, panelID)
{
}

// Activate and initialize this panel
// Invoked from VESSEL2's InitPanel method.
// Load our surface bitmaps. The inverse of this method is 'Deactivate'
// Returns: true on success, false on error (e.g., a bitmap failed to load)
bool XR1VCPassenger4InstrumentPanel::Activate()
{
    GetVessel().SetXRCameraDirection(_V(0, 0, 1)); // center, facing forward
    GetVessel().SetCameraOffset(_V(0.8, 1.2, 4.4));
    GetVessel().SetCameraMovement(_V(-0.4, 0, 0), 0, 0, _V(-0.4, 0, 0), 90 * RAD, 0, _V(0.3, 0, 0), -70 * RAD, 0);
    GetXR1().campos = DeltaGliderXR1::CAMERA_POSITION::CAM_VCPSNGR4;
    oapiVCSetNeighbours(3, -1, 2, -1);

    return true;
}

