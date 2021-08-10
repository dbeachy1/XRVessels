// ==============================================================
// XR2Ravenstar implementation class
//
// Copyright 2008-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR2InstrumentPanels.cpp
// Custom instrument panels for the XR2
// ==============================================================

#include "OrbiterSDK.h"
#include "resource.h"
#include "XR2AreaIDs.h"

#include "XR2InstrumentPanels.h"

// ------------------------------------------------------------------------

// VC Convenience macros
// TODO: #define ADD_TOGGLE_SWITCH(areaID, pHandler, activatedStatus)  AddArea(new VCToggleSwitchArea(*this, _COORD2(-1, -1), areaID, pHandler, activatedStatus))

//
// Construct our Virtual Cockpit Pilot panel and passenger views
//

// This is the base class, which is invoked in *all* VC views

// Constructor
// vessel = our parent vessel
// panelID = unique panel ID
XR2VCInstrumentPanel::XR2VCInstrumentPanel(XR2Ravenstar &vessel, int panelID) : 
    XR2InstrumentPanel(vessel, panelID)  // NOTE: force3DRedrawTo2D = true for the XR2's VC (default value defined in class definition)
{
    /* TODO
    // create our components
    AddComponent(new MFDComponent           (*this, _COORD2( 112, 214), MFD_LEFT));
    AddComponent(new MFDComponent           (*this, _COORD2( 112, 234), MFD_RIGHT));
    AddComponent(new AngularDataComponent   (*this, _COORD2(  56,  32)));
    AddComponent(new ScramPanelComponent    (*this, _COORD2( 141,   2)));
    AddComponent(new MainHoverPanelComponent(*this, _COORD2(   1,   0)));

    //
    // Create our areas
    //

    // create the four HUD mode button areas
    for (int i = 0; i < 4; i++)
        AddArea(new VCHudModeButtonArea(*this, _COORD2(-1, -1), AID_HUDBUTTON1+i));    // panelCoordinates are ignored for these areas

    // create the six autopilot (NAV mode) button areas
    for (int i=0; i < 6; i++)
        AddArea(new VCAutopilotButtonArea(*this, _COORD2(-1, -1), AID_NAVBUTTON1 + i)); // panelCoordinates are ignored for these areas
    
    AddArea(new ElevatorTrimArea             (*this, _COORD2(252,   0), AID_ELEVATORTRIM));
    AddArea(new HoverBalanceVerticalGaugeArea(*this, _COORD2( 97,  95), AID_HBALANCEDISP));  
    AddArea(new ScramPitchVerticalGaugeArea  (*this, _COORD2(236,  86), AID_GIMBALSCRAMDISP));
    AddArea(new MainPitchVerticalGaugeArea   (*this, _COORD2(227,   2), AID_PGIMBALMAINDISP));
    AddArea(new MainYawHorizontalGaugeArea   (*this, _COORD2(  6, 107), AID_YGIMBALMAINDISP));
    AddArea(new AOAAnalogGaugeArea           (*this, _COORD2( 17, 181), AID_AOAINSTR));
    AddArea(new SlipAnalogGaugeArea          (*this, _COORD2(109, 181), AID_SLIPINSTR));
    AddArea(new WingLoadAnalogGaugeArea      (*this, _COORD2(111,  17), AID_LOADINSTR));
    AddArea(new ArtificialHorizonArea        (*this, _COORD2(  0, 159), AID_HORIZON));
    AddArea(new ScramTempGaugeArea           (*this, _COORD2(  6,  10), AID_SCRAMTEMPDISP));
    AddArea(new DoorIndicatorArea            (*this, _COORD2(  1, 127), AID_GEARINDICATOR,     &GetXR2().gear_status, IDB_INDICATOR_UD, &GetXR2().gear_proc));
    AddArea(new DoorIndicatorArea            (*this, _COORD2( 32, 127), AID_NOSECONEINDICATOR, &GetXR2().nose_status, IDB_INDICATOR_OC, &GetXR2().nose_proc));

    // panelCoordinates are ignored for these interactive areas
    AddArea(new MWSArea                     (*this, _COORD2(-1, -1), AID_MWS));
    AddArea(new RCSModeArea                 (*this, _COORD2(-1, -1), AID_RCSMODE));
    AddArea(new AFCtrlArea                  (*this, _COORD2(-1, -1), AID_AFCTRLMODE));
    AddArea(new MainThrottleArea            (*this, _COORD2(-1, -1), AID_ENGINEMAIN)); 
    AddArea(new LargeHoverThrottleArea           (*this, _COORD2(-1, -1), AID_ENGINEHOVER)); 
    AddArea(new ScramThrottleArea           (*this, _COORD2(-1, -1), AID_ENGINESCRAM)); 
    AddArea(new MainPitchSwitchArea         (*this, _COORD2(-1, -1), AID_PGIMBALMAIN)); 
    AddArea(new ScramPitchSwitchArea        (*this, _COORD2(-1, -1), AID_GIMBALSCRAM)); 
    AddArea(new HoverBalanceSwitchArea      (*this, _COORD2(-1, -1), AID_HOVERBALANCE)); 
    AddArea(new MainYawSwitchArea           (*this, _COORD2(-1, -1), AID_YGIMBALMAIN)); 
    AddArea(new SimpleButtonArea            (*this, _COORD2(-1, -1), AID_HBALANCECENTER,    &GetXR2().m_hoverCenteringMode,      MESHGRP_VC_HBALANCECNT)); 
    AddArea(new SimpleButtonArea            (*this, _COORD2(-1, -1), AID_GIMBALSCRAMCENTER, &GetXR2().m_scramCenteringMode,      MESHGRP_VC_SCRAMGIMBALCNT));
    AddArea(new SimpleButtonArea            (*this, _COORD2(-1, -1), AID_PGIMBALMAINCENTER, &GetXR2().m_mainPitchCenteringMode,  MESHGRP_VC_PGIMBALCNT)); 

    AddArea(new SimpleButtonArea            (*this, _COORD2(-1, -1), AID_YGIMBALMAINCENTER, &GetXR2().m_mainYawCenteringMode,   MESHGRP_VC_YGIMBALCNT));
    AddArea(new SimpleButtonArea            (*this, _COORD2(-1, -1), AID_YGIMBALMAINDIV,    &GetXR2().m_mainDivMode,            MESHGRP_VC_YGIMBALDIV));
    AddArea(new SimpleButtonArea            (*this, _COORD2(-1, -1), AID_YGIMBALMAINAUTO,   &GetXR2().m_mainAutoMode,           MESHGRP_VC_YGIMBALAUTO));

    AddArea(new HudIntensitySwitchArea      (*this, _COORD2(-1, -1), AID_HUDINTENSITY)); 
    AddArea(new HudColorButtonArea          (*this, _COORD2(-1, -1), AID_HUDCOLOR));

    // add toggle switches (panelCoordinates are ignored for these)
    ADD_TOGGLE_SWITCH(AID_GEARDOWN,    &XR2Ravenstar::ActivateLandingGear,  DOOR_OPENING);
    ADD_TOGGLE_SWITCH(AID_GEARUP,      &XR2Ravenstar::ActivateLandingGear,  DOOR_CLOSING);
    ADD_TOGGLE_SWITCH(AID_NCONEOPEN,   &XR2Ravenstar::ActivateNoseCone,     DOOR_OPENING);
    ADD_TOGGLE_SWITCH(AID_NCONECLOSE,  &XR2Ravenstar::ActivateNoseCone,     DOOR_CLOSING);
    ADD_TOGGLE_SWITCH(AID_OLOCKOPEN,   &XR2Ravenstar::ActivateOuterAirlock, DOOR_OPENING);
    ADD_TOGGLE_SWITCH(AID_OLOCKCLOSE,  &XR2Ravenstar::ActivateOuterAirlock, DOOR_CLOSING);
    ADD_TOGGLE_SWITCH(AID_ILOCKOPEN,   &XR2Ravenstar::ActivateInnerAirlock, DOOR_OPENING);
    ADD_TOGGLE_SWITCH(AID_ILOCKCLOSE,  &XR2Ravenstar::ActivateInnerAirlock, DOOR_CLOSING);
    // TODO: ADD VC SWITCH FOR AIRLOCK PRESSURIZE / DEPRESSUREIZE
    ADD_TOGGLE_SWITCH(AID_RCOVEROPEN,  &XR2Ravenstar::ActivateRCover,       DOOR_OPENING);
    ADD_TOGGLE_SWITCH(AID_RCOVERCLOSE, &XR2Ravenstar::ActivateRCover,       DOOR_CLOSING);
    ADD_TOGGLE_SWITCH(AID_RADIATOREX,  &XR2Ravenstar::ActivateRadiator,     DOOR_OPENING);
    ADD_TOGGLE_SWITCH(AID_RADIATORIN,  &XR2Ravenstar::ActivateRadiator,     DOOR_CLOSING);
    ADD_TOGGLE_SWITCH(AID_HATCHOPEN,   &XR2Ravenstar::ActivateHatch,        DOOR_OPENING);
    ADD_TOGGLE_SWITCH(AID_HATCHCLOSE,  &XR2Ravenstar::ActivateHatch,        DOOR_CLOSING);
    ADD_TOGGLE_SWITCH(AID_LADDEREX,    &XR2Ravenstar::ActivateLadder,       DOOR_OPENING);
    ADD_TOGGLE_SWITCH(AID_LADDERIN,    &XR2Ravenstar::ActivateLadder,       DOOR_CLOSING);
    */
}

// Activate and initialize this panel
// Invoked from VESSEL2's InitPanel method.
// Load our surface bitmaps. The inverse of this method is 'Deactivate'
// Returns: true on success, false on error (e.g., a bitmap failed to load)
bool XR2VCInstrumentPanel::Activate()
{
    // activate all our areas, including our components' areas; this will invoke oapiVCRegisterArea, etc.
    ActivateAllAreas();

    // Hide the active VC HUD mesh so we don't render it twice; this is also invoked in clbkVisualCreated
    GetXR2().HideActiveVCHUDMesh();

    /* {YYY} TODO
    //
    // Define interactive (clickable) areas in the VC
    //
    const double buttonRadius = .005;   // radius of small buttons
    
    oapiVCSetAreaClickmode_Quadrilateral(AID_ELEVATORTRIM,      _V(0.2873,1.0276,7.2286), _V(0.3040,1.0327,7.2151), _V(0.2873,0.9957,7.2165), _V(0.3040,1.0008,7.2030));
    oapiVCSetAreaClickmode_Spherical    (AID_MWS,               _V(0.0755,1.2185,7.3576), 0.013);
    oapiVCSetAreaClickmode_Spherical    (AID_RCSMODE,           _V(-0.3358,1.0683,7.2049),0.02);
    oapiVCSetAreaClickmode_Spherical    (AID_AFCTRLMODE,        _V(-0.3351,1.1153,7.2131),0.02);
    oapiVCSetAreaClickmode_Quadrilateral(AID_ENGINEMAIN,        _V(-0.372,0.918,6.905), _V(-0.279,0.918,6.905), _V(-0.372,0.885,7.11), _V(-0.279,0.885,7.11));
    oapiVCSetAreaClickmode_Quadrilateral(AID_ENGINEHOVER,       _V(-0.44,0.87,6.81), _V(-0.35,0.87,6.81), _V(-0.44,0.95,6.91), _V(-0.35,0.95,6.91));
    oapiVCSetAreaClickmode_Quadrilateral(AID_ENGINESCRAM,       _V(-0.45,0.98,6.94), _V(-0.39,0.98,6.94), _V(-0.45,0.95,7.07), _V(-0.39,0.95,7.07));
    oapiVCSetAreaClickmode_Quadrilateral(AID_HOVERBALANCE,      _V(-0.2691,1.1353,7.27), _V(-0.2606,1.1346,7.2729), _V(-0.2691,1.1065,7.2625), _V(-0.2606,1.1058,7.2654));
    oapiVCSetAreaClickmode_Quadrilateral(AID_PGIMBALMAIN,       _V(-0.3739,1.1105,7.1478), _V(-0.3593,1.108,7.1618), _V(-0.3728,1.0875,7.1426), _V(-0.3582,1.085,7.1566));
    oapiVCSetAreaClickmode_Quadrilateral(AID_GIMBALSCRAM,       _V(-0.2666,1.0629,7.2484), _V(-0.248,1.0613,7.2548), _V(-0.2666,1.04,7.2425), _V(-0.248,1.0384,7.2488));
    oapiVCSetAreaClickmode_Quadrilateral(AID_YGIMBALMAIN,       _V(-0.3728,1.0522,7.1301), _V(-0.3566,1.0494,7.1460), _V(-0.3720,1.0324,7.1259), _V(-0.3558,1.0293,7.1416));
    oapiVCSetAreaClickmode_Quadrilateral(AID_HUDINTENSITY,      _V(0.2327,1.1682,7.3136), _V(0.2500,1.1682,7.3136), _V(0.2327,1.1300,7.3134), _V(0.2500,1.1300,7.3134));
    oapiVCSetAreaClickmode_Spherical    (AID_HUDCOLOR,          _V(0.2511,1.1456,7.3031), buttonRadius);
     
    // Gimble buttons
    oapiVCSetAreaClickmode_Spherical(AID_YGIMBALMAINCENTER, _V(-0.393, 1.065, 7.1074), buttonRadius);
    oapiVCSetAreaClickmode_Spherical(AID_YGIMBALMAINDIV,    _V(-0.394, 1.053, 7.1073), buttonRadius);
    oapiVCSetAreaClickmode_Spherical(AID_YGIMBALMAINAUTO,   _V(-0.395, 1.040, 7.1072), buttonRadius);
    oapiVCSetAreaClickmode_Spherical(AID_PGIMBALMAINCENTER, _V(-0.3708,1.0743,7.1357), buttonRadius);
    oapiVCSetAreaClickmode_Spherical(AID_HBALANCECENTER,    _V(-0.2684,1.0972,7.2555), buttonRadius);
    oapiVCSetAreaClickmode_Spherical(AID_GIMBALSCRAMCENTER, _V(-0.2672,1.0256,7.2336), buttonRadius);

    // Left MFD clickable areas
    oapiVCSetAreaClickmode_Quadrilateral(AID_MFD1_LBUTTONS, _V(-0.2301,1.1592,7.3322), _V(-0.2161,1.1592,7.3322), _V(-0.2301,1.0302,7.2852), _V(-0.2161,1.0302,7.2852));
    oapiVCSetAreaClickmode_Quadrilateral(AID_MFD1_RBUTTONS, _V(-0.023942,1.1592,7.3322), _V(-0.009927,1.1592,7.3322), _V(-0.023942,1.0302,7.2852), _V(-0.009927,1.0302,7.2852));
    oapiVCSetAreaClickmode_Spherical(AID_MFD1_PWR, _V(-0.1914,1.009,7.2775), 0.01);
    oapiVCSetAreaClickmode_Spherical(AID_MFD1_SEL, _V(-0.0670,1.009,7.2775), 0.01);
    oapiVCSetAreaClickmode_Spherical(AID_MFD1_MNU, _V(-0.0485,1.009,7.2775), 0.01);

    // Right MFD clickable areas
    oapiVCSetAreaClickmode_Quadrilateral(AID_MFD2_LBUTTONS, _V(0.009927,1.1592,7.3322), _V(0.023942,1.1592,7.3322), _V(0.009927,1.0302,7.2852), _V(0.023942,1.0302,7.2852));
    oapiVCSetAreaClickmode_Quadrilateral(AID_MFD2_RBUTTONS, _V(0.216058,1.1592,7.3322), _V(0.230072,1.1592,7.3322), _V(0.216058,1.0302,7.2852), _V(0.230072,1.0302,7.2852));
    oapiVCSetAreaClickmode_Spherical(AID_MFD2_PWR, _V(0.0483,1.009,7.2775), 0.01);
    oapiVCSetAreaClickmode_Spherical(AID_MFD2_SEL, _V(0.1726,1.009,7.2775), 0.01);
    oapiVCSetAreaClickmode_Spherical(AID_MFD2_MNU, _V(0.1913,1.009,7.2775), 0.01);

    // Switches and toggle levers
    oapiVCSetAreaClickmode_Spherical (AID_GEARDOWN, _V(0.3008,1.0197,7.1656),0.02);
    oapiVCSetAreaClickmode_Spherical (AID_GEARUP, _V(0.3052,0.9061,7.1280),0.02);

    oapiVCSetAreaClickmode_Spherical (AID_NCONEOPEN, _V(0.3317,1.1078,7.1968),0.02);
    oapiVCSetAreaClickmode_Spherical (AID_NCONECLOSE, _V(0.3281,1.0302,7.1630),0.02);

    oapiVCSetAreaClickmode_Spherical (AID_OLOCKOPEN, _V(0.2506,1.0884,7.2866),0.01);
    oapiVCSetAreaClickmode_Spherical (AID_OLOCKCLOSE, _V(0.2506,1.1054,7.2866),0.01);

    oapiVCSetAreaClickmode_Spherical (AID_ILOCKOPEN, _V(0.2824,1.0981,7.2611),0.01);
    oapiVCSetAreaClickmode_Spherical (AID_ILOCKCLOSE, _V(0.2824,1.1151,7.2611),0.01);

    oapiVCSetAreaClickmode_Spherical (AID_RCOVEROPEN, _V(0.2508,1.0420,7.2694),0.01);
    oapiVCSetAreaClickmode_Spherical (AID_RCOVERCLOSE, _V(0.2508,1.0590,7.2694),0.01);

    oapiVCSetAreaClickmode_Spherical (AID_RADIATOREX, _V(0.2582,0.9448,7.22),0.01);
    oapiVCSetAreaClickmode_Spherical (AID_RADIATORIN, _V(0.2582,0.9618,7.22),0.01);

    oapiVCSetAreaClickmode_Spherical (AID_HATCHOPEN, _V(0.2511,0.9921,7.2507), 0.01);
    oapiVCSetAreaClickmode_Spherical (AID_HATCHCLOSE, _V(0.2511,1.0091,7.2507), 0.01);

    oapiVCSetAreaClickmode_Spherical (AID_LADDEREX, _V(0.2889,1.0537,7.2388), 0.01);
    oapiVCSetAreaClickmode_Spherical (AID_LADDERIN, _V(0.2889,1.0707,7.2388), 0.01);

    // define the four HUD clickable buttons and trigger a redraw of each in case the mode changed in 2D mode
    for (int i=0; i < 4; i++)
    {
        int buttonAreaID = AID_HUDBUTTON1 + i;
        oapiVCSetAreaClickmode_Spherical(buttonAreaID, _V(-0.1094,1.4174+0.0101*i,7.0406+i*0.0070), 0.0065);
        TriggerRedrawArea(buttonAreaID);
    }

    // define the HUD display in the VC
    static VCHUDSPEC huds = {1, MESHGRP_VC_HUDDISP, {0,1.462,7.09}, 0.15};
    oapiVCRegisterHUD(&huds);  // HUD parameters

    // define the six autopilot (NAV mode) clickable buttons and trigger a redraw of each in case the mode changed in 2D mode
    for (int i=0; i < 6; i++)
    {
        int buttonAreaID = AID_NAVBUTTON1 + i;
        oapiVCSetAreaClickmode_Spherical (AID_NAVBUTTON1+i, _V(0.11264,1.461821-0.0132572*i,7.071551-0.0090569*i), 0.0065);
        TriggerRedrawArea(buttonAreaID);
    }
    */

    return true;
}

//=========================================================================

// Activate and initialize pilot instrument panel
// Invoked from VESSEL2's InitPanel method.
// Load our surface bitmaps. The inverse of this method is 'Deactivate'
// Returns: true on success, false on error (e.g., a bitmap failed to load)
bool XR2VCPilotInstrumentPanel::Activate()
{
    // define the HUD display in the VC
    // Note: Orbiter only supports ONE active HUD surface in the VC
    // NOTE #2: the "size" value seems to be ignored by the Orbiter core
    static VCHUDSPEC huds = {0, PILOT_HUD_MESHGRP, {-0.414, 1.946, 8.011}, 0.127};  // X,Y match eyepoint
    oapiVCRegisterHUD(&huds);  // HUD parameters

    GetVessel().SetXRCameraDirection (_V(0,0,1)); // center, facing forward

    // set for Pilot
    GetVessel().SetCameraOffset(_V(-0.414, 1.946, 7.27));  // pilot's eyes
    GetVessel().SetCameraShiftRange (_V(0,0,0.32), _V(-0.1,0,0), _V(0.25,0,0));

    oapiVCSetNeighbours(-1, ORBITER_VC_NUMBER(PANELVC_COPILOT), -1, ORBITER_VC_NUMBER(PANELVC_PSNGR2));

    // set current camera position flag
    GetXR2().campos = GetXR2().CAM_VCPILOT;

    // invoke the superclass to activate all VC areas
    return XR2VCInstrumentPanel::Activate();
}

// Activate and initialize copilot instrument panel
bool XR2VCCopilotInstrumentPanel::Activate()
{
    // define the HUD display in the VC
    // Note: Orbiter only supports ONE active HUD surface in the VC
    // NOTE #2: the "size" value seems to be ignored by the Orbiter core
    static VCHUDSPEC huds = {0, COPILOT_HUD_MESHGRP, {0.407, 1.922, 8.011}, 0.1325};  // X,Y match eyepoint
    oapiVCRegisterHUD(&huds);  // HUD parameters  

    GetVessel().SetXRCameraDirection (_V(0,0,1)); // center, facing forward

    // set for Copilot
    // as measured: GetVessel().SetCameraOffset(_V(0.397, 1.922, 7.461));  // copilot's eyes
    GetVessel().SetCameraOffset(_V(0.407, 1.922, 7.461));  // moved directly in front of HUD

    GetVessel().SetCameraShiftRange (_V(0,0,0.25), _V(-0.25,0,0), _V(0.1,0,0));

    oapiVCSetNeighbours(ORBITER_VC_NUMBER(PANELVC_PILOT), -1, -1, ORBITER_VC_NUMBER(PANELVC_PSNGR3));

    // set current camera position flag
    GetXR2().campos = GetXR2().CAM_VCCOPILOT;

    // invoke the superclass to activate all VC areas
    return XR2VCInstrumentPanel::Activate();
}

// Note: passengers are here (looking FORWARD):
// pilot  copilot
//
//       A       = airlock
//  1 2    3 4
//  5 6    7 8
//  9 10  11 12

// shift each headrest coordinate forward by this amount
#define PASSENGER_ZSHIFT 0.25

// Activate and initialize passenger #1 instrument panel
bool XR2VCPassenger1InstrumentPanel::Activate()
{
    GetVessel().SetXRCameraDirection (_V(0,0,1)); // center, facing forward
    GetVessel().SetCameraOffset(_V(-1.222, 1.004, 5.973 + PASSENGER_ZSHIFT));  
    GetVessel().SetCameraShiftRange (_V(0,0,0.25), _V(-0.10,0,0), _V(0.25,0,0));

    oapiVCSetNeighbours(-1, ORBITER_VC_NUMBER(PANELVC_PSNGR2), ORBITER_VC_NUMBER(PANELVC_PILOT), ORBITER_VC_NUMBER(PANELVC_PSNGR5));

    // set current camera position flag
    GetXR2().campos = GetXR2().CAM_VCPSNGR1;

    // invoke the superclass to activate all VC areas
    return XR2VCInstrumentPanel::Activate();
}

// Activate and initialize passenger #2 instrument panel
bool XR2VCPassenger2InstrumentPanel::Activate()
{
    GetVessel().SetXRCameraDirection (_V(0,0,1)); // center, facing forward
    GetVessel().SetCameraOffset(_V(-0.59, 1.004, 5.973 + PASSENGER_ZSHIFT));  
    GetVessel().SetCameraShiftRange (_V(0,0,0.25), _V(-0.25,0,0), _V(0.25,0,0));

    oapiVCSetNeighbours(ORBITER_VC_NUMBER(PANELVC_PSNGR1), ORBITER_VC_NUMBER(PANELVC_AIRLOCK), ORBITER_VC_NUMBER(PANELVC_PILOT), ORBITER_VC_NUMBER(PANELVC_PSNGR6));

    // set current camera position flag
    GetXR2().campos = GetXR2().CAM_VCPSNGR2;

    // invoke the superclass to activate all VC areas
    return XR2VCInstrumentPanel::Activate();
}

// Activate and initialize airlock instrument panel
bool XR2VCAirlockInstrumentPanel::Activate()
{
    GetVessel().SetXRCameraDirection (_V(0,0,-1)); // center, facing AFT
    GetVessel().SetCameraOffset(_V(0.0, 0.253, 9.24));  
    GetVessel().SetCameraShiftRange (_V(0,0,0.75), _V(-0.25,0,0), _V(0.25,0,0));
    GetVessel().SetCameraRotationRange(PI*0.99, PI*0.99, PI*0.4, PI*0.4);  // allow extra rotation range in airlock

    oapiVCSetNeighbours(ORBITER_VC_NUMBER(PANELVC_PSNGR2), ORBITER_VC_NUMBER(PANELVC_PSNGR3), ORBITER_VC_NUMBER(PANELVC_PILOT), -1);

    // set current camera position flag
    GetXR2().campos = GetXR2().CAM_VCAIRLOCK;

    // invoke the superclass to activate all VC areas
    return XR2VCInstrumentPanel::Activate();
}

// Activate and initialize passenger #3 instrument panel
bool XR2VCPassenger3InstrumentPanel::Activate()
{
    GetVessel().SetXRCameraDirection (_V(0,0,1)); // center, facing forward
    GetVessel().SetCameraOffset(_V(0.551, 1.004, 5.973 + PASSENGER_ZSHIFT));  
    GetVessel().SetCameraShiftRange (_V(0,0,0.25), _V(-0.25,0,0), _V(0.25,0,0));

    oapiVCSetNeighbours(ORBITER_VC_NUMBER(PANELVC_AIRLOCK), ORBITER_VC_NUMBER(PANELVC_PSNGR4), ORBITER_VC_NUMBER(PANELVC_COPILOT), ORBITER_VC_NUMBER(PANELVC_PSNGR7));

    // set current camera position flag
    GetXR2().campos = GetXR2().CAM_VCPSNGR3;

    // invoke the superclass to activate all VC areas
    return XR2VCInstrumentPanel::Activate();
}


// Activate and initialize passenger #4 instrument panel
bool XR2VCPassenger4InstrumentPanel::Activate()
{
    GetVessel().SetXRCameraDirection (_V(0,0,1)); // center, facing forward
    GetVessel().SetCameraOffset(_V(1.183, 1.004, 5.973 + PASSENGER_ZSHIFT));  
    GetVessel().SetCameraShiftRange (_V(0,0,0.25), _V(-0.25,0,0), _V(0.10,0,0));

    oapiVCSetNeighbours(ORBITER_VC_NUMBER(PANELVC_PSNGR3), -1, ORBITER_VC_NUMBER(PANELVC_COPILOT), ORBITER_VC_NUMBER(PANELVC_PSNGR8));

    // set current camera position flag
    GetXR2().campos = GetXR2().CAM_VCPSNGR4;

    // invoke the superclass to activate all VC areas
    return XR2VCInstrumentPanel::Activate();
}

// Activate and initialize passenger #5 instrument panel
bool XR2VCPassenger5InstrumentPanel::Activate()
{
    GetVessel().SetXRCameraDirection (_V(0,0,1)); // center, facing forward
    GetVessel().SetCameraOffset(_V(-1.222, 1.004, 4.873 + PASSENGER_ZSHIFT));  
    GetVessel().SetCameraShiftRange (_V(0,0,0.25), _V(-0.10,0,0), _V(0.25,0,0));

    oapiVCSetNeighbours(-1, ORBITER_VC_NUMBER(PANELVC_PSNGR6), ORBITER_VC_NUMBER(PANELVC_PSNGR1), ORBITER_VC_NUMBER(PANELVC_PSNGR9));

    // set current camera position flag
    GetXR2().campos = GetXR2().CAM_VCPSNGR5;

    // invoke the superclass to activate all VC areas
    return XR2VCInstrumentPanel::Activate();
}

// Activate and initialize passenger #6 instrument panel
bool XR2VCPassenger6InstrumentPanel::Activate()
{
    GetVessel().SetXRCameraDirection (_V(0,0,1)); // center, facing forward
    GetVessel().SetCameraOffset(_V(-0.59, 1.004, 4.873 + PASSENGER_ZSHIFT));  
    GetVessel().SetCameraShiftRange (_V(0,0,0.25), _V(-0.25,0,0), _V(0.25,0,0));

    oapiVCSetNeighbours(ORBITER_VC_NUMBER(PANELVC_PSNGR5), ORBITER_VC_NUMBER(PANELVC_PSNGR7), ORBITER_VC_NUMBER(PANELVC_PSNGR2), ORBITER_VC_NUMBER(PANELVC_PSNGR10));

    // set current camera position flag
    GetXR2().campos = GetXR2().CAM_VCPSNGR6;

    // invoke the superclass to activate all VC areas
    return XR2VCInstrumentPanel::Activate();
}

// Activate and initialize passenger #7 instrument panel
bool XR2VCPassenger7InstrumentPanel::Activate()
{
    GetVessel().SetXRCameraDirection (_V(0,0,1)); // center, facing forward
    GetVessel().SetCameraOffset(_V(0.551, 1.004, 4.873 + PASSENGER_ZSHIFT));  
    GetVessel().SetCameraShiftRange (_V(0,0,0.25), _V(-0.25,0,0), _V(0.25,0,0));

    oapiVCSetNeighbours(ORBITER_VC_NUMBER(PANELVC_PSNGR6), ORBITER_VC_NUMBER(PANELVC_PSNGR8), ORBITER_VC_NUMBER(PANELVC_PSNGR3), ORBITER_VC_NUMBER(PANELVC_PSNGR11));

    // set current camera position flag
    GetXR2().campos = GetXR2().CAM_VCPSNGR7;

    // invoke the superclass to activate all VC areas
    return XR2VCInstrumentPanel::Activate();
}


// Activate and initialize passenger #8 instrument panel
bool XR2VCPassenger8InstrumentPanel::Activate()
{
    GetVessel().SetXRCameraDirection (_V(0,0,1)); // center, facing forward
    GetVessel().SetCameraOffset(_V(1.183, 1.004, 4.873 + PASSENGER_ZSHIFT));  
    GetVessel().SetCameraShiftRange (_V(0,0,0.25), _V(-0.25,0,0), _V(0.10,0,0));

    oapiVCSetNeighbours(ORBITER_VC_NUMBER(PANELVC_PSNGR7), -1, ORBITER_VC_NUMBER(PANELVC_PSNGR4), ORBITER_VC_NUMBER(PANELVC_PSNGR12));

    // set current camera position flag
    GetXR2().campos = GetXR2().CAM_VCPSNGR8;

    // invoke the superclass to activate all VC areas
    return XR2VCInstrumentPanel::Activate();
}

// Activate and initialize passenger #9 instrument panel
bool XR2VCPassenger9InstrumentPanel::Activate()
{
    GetVessel().SetXRCameraDirection (_V(0,0,1)); // center, facing forward
    GetVessel().SetCameraOffset(_V(-1.222, 1.004, 3.773 + PASSENGER_ZSHIFT));  
    GetVessel().SetCameraShiftRange (_V(0,0,0.25), _V(-0.10,0,0), _V(0.25,0,0));

    oapiVCSetNeighbours(-1, ORBITER_VC_NUMBER(PANELVC_PSNGR10), ORBITER_VC_NUMBER(PANELVC_PSNGR5), -1);

    // set current camera position flag
    GetXR2().campos = GetXR2().CAM_VCPSNGR9;

    // invoke the superclass to activate all VC areas
    return XR2VCInstrumentPanel::Activate();
}

// Activate and initialize passenger #10 instrument panel
bool XR2VCPassenger10InstrumentPanel::Activate()
{
    GetVessel().SetXRCameraDirection (_V(0,0,1)); // center, facing forward
    GetVessel().SetCameraOffset(_V(-0.59, 1.004, 3.773 + PASSENGER_ZSHIFT));  
    GetVessel().SetCameraShiftRange (_V(0,0,0.25), _V(-0.25,0,0), _V(0.25,0,0));

    oapiVCSetNeighbours(ORBITER_VC_NUMBER(PANELVC_PSNGR9), ORBITER_VC_NUMBER(PANELVC_PSNGR11), ORBITER_VC_NUMBER(PANELVC_PSNGR6), -1);

    // set current camera position flag
    GetXR2().campos = GetXR2().CAM_VCPSNGR10;

    // invoke the superclass to activate all VC areas
    return XR2VCInstrumentPanel::Activate();
}

// Activate and initialize passenger #11 instrument panel
bool XR2VCPassenger11InstrumentPanel::Activate()
{
    GetVessel().SetXRCameraDirection (_V(0,0,1)); // center, facing forward
    GetVessel().SetCameraOffset(_V(0.551, 1.004, 3.773 + PASSENGER_ZSHIFT));  
    GetVessel().SetCameraShiftRange (_V(0,0,0.25), _V(-0.25,0,0), _V(0.25,0,0));

    oapiVCSetNeighbours(ORBITER_VC_NUMBER(PANELVC_PSNGR10), ORBITER_VC_NUMBER(PANELVC_PSNGR12), ORBITER_VC_NUMBER(PANELVC_PSNGR7), -1);

    // set current camera position flag
    GetXR2().campos = GetXR2().CAM_VCPSNGR11;

    // invoke the superclass to activate all VC areas
    return XR2VCInstrumentPanel::Activate();
}


// Activate and initialize passenger #12 instrument panel
bool XR2VCPassenger12InstrumentPanel::Activate()
{
    GetVessel().SetXRCameraDirection (_V(0,0,1)); // center, facing forward
    GetVessel().SetCameraOffset(_V(1.183, 1.004, 3.773 + PASSENGER_ZSHIFT));  
    GetVessel().SetCameraShiftRange (_V(0,0,0.25), _V(-0.25,0,0), _V(0.10,0,0));

    oapiVCSetNeighbours(ORBITER_VC_NUMBER(PANELVC_PSNGR11), -1, ORBITER_VC_NUMBER(PANELVC_PSNGR8), -1);

    // set current camera position flag
    GetXR2().campos = GetXR2().CAM_VCPSNGR8;

    // invoke the superclass to activate all VC areas
    return XR2VCInstrumentPanel::Activate();
}

