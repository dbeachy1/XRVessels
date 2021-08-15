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
// XR2Ravenstar implementation class
//
// XR2InstrumentPanels.cpp
// Custom instrument panels for the XR2
// ==============================================================

#include "OrbiterSDK.h"
#include "resource.h"
#include "XR2AreaIDs.h"

#include "XR2InstrumentPanels.h"

#include "XR1MainPanelAreas.h"
#include "XR1UpperPanelAreas.h"
#include "XR1LowerPanelAreas.h"
#include "XR1VCPanelAreas.h"
#include "XR1HUD.h"
#include "XR1AngularDataComponent.h"
#include "XR1MFDComponent.h"
#include "XR1ThrottleQuadrantComponents.h"
#include "XR1MainPanelComponents.h"
#include "XR1FuelDisplayComponent.h"
#include "XR1EngineDisplayComponent.h"
#include "XR1MultiDisplayArea.h"
#include "XR1UpperPanelComponents.h"
#include "XR1LowerPanelComponents.h"

#include "XR2Areas.h"
#include "XR2Components.h"
#include "XR2PayloadScreenAreas.h"

// 2D cockpit coordinates for the eyepoint
static const VECTOR3 twoDCockpitCoordinates = _V(0, 1.946, 7.27);   // save as VC pilot view except that X == 0

// macros
#define ADD_SWITCH_AND_LED(switchClass, x, aidSwitch, aidLED, refLedIsOn)        \
    AddArea(new switchClass(*this, _COORD2( x,   switchY), aidSwitch, aidLED));  \
    AddArea(new LEDArea    (*this, _COORD2( x-1, ledY   ), aidLED, GetXR2().refLedIsOn))

#define ADD_SWITCH_AND_INDICATOR(switchClass, x, aidSwitch, aidIndicator, doorStatus, indicatorSurfaceIDB, animationState)  \
    AddArea(new switchClass      (*this, _COORD2( x,   switchY),    aidSwitch,    aidIndicator));                           \
    AddArea(new DoorIndicatorArea(*this, _COORD2( x-9, indicatorY), aidIndicator, VCPANEL_TEXTURE_NONE, &GetXR2().doorStatus, indicatorSurfaceIDB, &GetXR2().animationState))

#define ADD_SUPPLY_SWITCH_AND_LED(x, aidSwitch, aidLED, refSwitchState, refPressure)                                                              \
    AddArea(new ExtSupplyLineToggleSwitchArea(*this, _COORD2( x,   switchY), aidSwitch, aidLED, GetXR2().refSwitchState, GetXR2().refPressure));  \
    AddArea(new LEDArea                      (*this, _COORD2( x-1, ledY   ), aidLED, GetXR2().refSwitchState))

//----------------------------------------------------------------------

// base panel for the XR2 that all of our panels extend, including both 2D and 3D (VC) panels

// Constructor
// vessel = our parent vessel
// panelID = unique panel ID
// panelResourceID = resource ID of this panel in our DLL; e.g., IDB_PANEL1_1280.  -1 = NONE
XR2InstrumentPanel::XR2InstrumentPanel(XR2Ravenstar &vessel, const int panelID, const WORD panelResourceID, const bool force3DRedrawTo2D) :
    InstrumentPanel(vessel, panelID, (panelID - VC_PANEL_ID_BASE), panelResourceID, force3DRedrawTo2D)  // Orbiter VC panel ID is a delta from our globally unique panel ID)  
{
#ifdef _DEBUG
    const unsigned short NO_ID = (unsigned short)-1;
    if (IsVC())
        _ASSERTE(panelResourceID == NO_ID);
    else  // this is 2D panel, so panelResourceID should be valid
        _ASSERTE(panelResourceID != NO_ID);
#endif
}

// initialize a new MDA screen and all valid MultiDisplayModes
void XR2InstrumentPanel::InitMDA(MultiDisplayArea *pMDA)
{
    pMDA->AddDisplayMode(new AirspeedHoldMultiDisplayMode   (MDMID_AIRSPEED_HOLD));
    pMDA->AddDisplayMode(new DescentHoldMultiDisplayMode    (MDMID_DESCENT_HOLD));
    pMDA->AddDisplayMode(new AttitudeHoldMultiDisplayMode   (MDMID_ATTITUDE_HOLD));
    pMDA->AddDisplayMode(new XR2HullTempsMultiDisplayMode   (MDMID_HULL_TEMPS));
    pMDA->AddDisplayMode(new SystemsStatusMultiDisplayMode  (MDMID_SYSTEMS_STATUS1));
    pMDA->AddDisplayMode(new SystemsStatusMultiDisplayMode  (MDMID_SYSTEMS_STATUS2));
    pMDA->AddDisplayMode(new SystemsStatusMultiDisplayMode  (MDMID_SYSTEMS_STATUS3));
    pMDA->AddDisplayMode(new SystemsStatusMultiDisplayMode  (MDMID_SYSTEMS_STATUS4));
    pMDA->AddDisplayMode(new SystemsStatusMultiDisplayMode  (MDMID_SYSTEMS_STATUS5));
    pMDA->AddDisplayMode(new XR2ReentryCheckMultiDisplayMode(MDMID_REENTRY_CHECK));
}

//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Base classes for our different instrument panels; these classes contain
// code and data common for each panel regardless of its resolution.
//----------------------------------------------------------------------

// Activate and initialize the MAIN panel
// Invoked from VESSEL2's InitPanel method.
// Load our surface bitmaps. The inverse of this method is 'Deactivate'.
// Returns: true on success, false on error (e.g., a bitmap failed to load)
bool XR2MainInstrumentPanel::Activate() 
{
    const WORD panelResourceID = GetPanelResourceID();
    
    // load our bitmap
    m_hBmp = LoadBitmap(GetVessel().GetModuleHandle(), MAKEINTRESOURCE (panelResourceID));
    if (m_hBmp == nullptr)
        return false;       // should never happen

    GetVessel().SetCameraOffset(twoDCockpitCoordinates);
    GetVessel().SetXRCameraDirection (_V(0,0,1)); // look forward

    oapiRegisterPanelBackground(m_hBmp, PANEL_ATTACH_BOTTOM|PANEL_MOVEOUT_BOTTOM, 0xFFFFFF);  // white == transparent
    oapiSetPanelNeighbours (-1, -1, PANEL_UPPER, PANEL_LOWER);

    // initialize the XR vessel's m_pMDA to point to *this panel's* MDA object
    GetXR2().m_pMDA = static_cast<MultiDisplayArea *>(GetArea(AID_MULTI_DISPLAY));

    GetXR2().SetMDAModeForCustomAutopilot();  // update the MDA mode if MDA is visible

    // activate all our areas, including our components' areas
    ActivateAllAreas();

    GetXR2().campos = GetXR2().CAM_PANELMAIN;
    return true;
}

// Deactivate the MAIN panel; invoked when Orbiter invokes "ReleaseSurfaces"
void XR2MainInstrumentPanel::Deactivate()
{
    // mark the multi-display area as hidden now to prevent mode switching when invisible
    GetXR2().m_pMDA = nullptr;

    // now call the base class
    InstrumentPanel::Deactivate();
}

//-------------------------------------------------------------------------

// Activate and initialize the PAYLOAD panel.
// Invoked from VESSEL2's InitPanel method.
// Load our surface bitmaps. The inverse of this method is 'Deactivate'
// Returns: true on success, false on error (e.g., a bitmap failed to load)
bool XR2PayloadInstrumentPanel::Activate()
{
    const WORD panelResourceID = GetPanelResourceID();

    // load our bitmap
    m_hBmp = LoadBitmap(GetVessel().GetModuleHandle(), MAKEINTRESOURCE (panelResourceID));
    if (m_hBmp == nullptr)
        return false;       // should never happen
    
    oapiRegisterPanelBackground (m_hBmp, PANEL_ATTACH_BOTTOM | PANEL_ATTACH_LEFT | PANEL_MOVEOUT_BOTTOM, 0xFFFFFF);  // white is transparent

    // this panel is unique in that it is connected "one-way" to the upper panel to the right and the main panel below.
    oapiSetPanelNeighbours (-1, PANEL_UPPER, -1, PANEL_MAIN);

    // position the view at the top of the payload bay looking into it
    GetXR2().ResetCameraToPayloadBay();

    // enable the external mesh to be rendered in the external pass so that cargo containers may obscure it
    // external mesh is always index 0
    GetVessel().SetMeshVisibilityMode(0, (MESHVIS_EXTERNAL | MESHVIS_COCKPIT | MESHVIS_EXTPASS));

    // activate all our areas, including our components' areas
    ActivateAllAreas();

    GetXR2().campos = GetXR2().CAM_PANELPAYLOAD;

    return true;
}

// deactivate this panel
void XR2PayloadInstrumentPanel::Deactivate()
{
    // only reset if the panel is currently active
    if (IsActive())
    {
        // reset the mesh visibility to normal
        GetVessel().SetMeshVisibilityMode(0, MESHVIS_EXTERNAL | MESHVIS_VC);

        // Note: do not set camera apeture here: if we shut down in normal mode, the Orbiter core will crash if we invoke oapiCameraSetAperture
        // restore the camera apeture
        // Cannot do this: oapiCameraSetAperture(m_orgCameraAperture);
    }

    // do the work
    XR2InstrumentPanel::Deactivate();
}

// add areas common for all panel resolutions    
void XR2PayloadInstrumentPanel::AddCommonAreas()
{
    // no shift for this panel
    // add components
    AddComponent(new APUPanelComponent           (*this, _COORD2( 34,  241)));
    AddComponent(new PayloadMassDisplayComponent (*this, _COORD2(952,  193), AID_PAYLOADMASS_LB, AID_PAYLOADMASS_KG));
    AddComponent(new ShipMassDisplayComponent    (*this, _COORD2(1111, 193)));
    
    // add areas
    AddArea(new SwitchToPanelButtonArea          (*this, _COORD2(  32, 194), AID_RETURN_TO_UPPER_PANEL_VIEW, PANEL_UPPER));
    AddArea(new SystemsDisplayScreen             (*this, _COORD2(1026, 265), AID_SYSTEMS_DISPLAY_SCREEN));

    AddArea(new PayloadEditorButtonArea          (*this, _COORD2(  32, 173), AID_PAYLOAD_EDITOR_BUTTON));
    AddArea(new SelectPayloadSlotArea            (*this, _COORD2( 413, 200), AID_SELECT_PAYLOAD_BAY_SLOT_SCREEN));
    AddArea(new DeployPayloadArea                (*this, _COORD2( 598, 200), AID_DEPLOY_PAYLOAD_SCREEN, IDB_DEPLOY_PAYLOAD_ORBIT, IDB_DEPLOY_PAYLOAD_LANDED));
    AddArea(new PayloadThumbnailArea             (*this, _COORD2( 842, 267), AID_PAYLOAD_THUMBNAIL_SCREEN, IDB_PAYLOAD_THUMBNAIL_NONE));
    AddArea(new GrapplePayloadArea               (*this, _COORD2( 164, 200), AID_GRAPPLE_PAYLOAD_SCREEN, IDB_GRAPPLE_PAYLOAD));

    // add switches and indicators
    int switchY = 253;
    int indicatorY = 305;
    ADD_SWITCH_AND_INDICATOR(BayDoorsToggleSwitchArea, 99, AID_BAYDOORSSWITCH,  AID_BAYDOORSINDICATOR,  bay_status,      IDB_INDICATOR_OC, bay_proc);
    
}

//-------------------------------------------------------------------------

// add areas common to 1600-pixel-wide or wider panels
void XR2UpperInstrumentPanel::Add1600PlusAreas(const int width)
{
    // calibrated for 1600
    const int shift = (width - 1600) / 2;

    // add components
    AddComponent(new METTimerComponent        (*this, _COORD2(shift + 1150,  35)));
    AddComponent(new Interval1TimerComponent  (*this, _COORD2(shift + 1150,  81)));
    AddComponent(new Interval2TimerComponent  (*this, _COORD2(shift + 1150, 127)));

    // Note: cannot use XR2WarningLightsComponent here because the layout is slightly different for this panel
    AddArea(new MWSArea             (*this, _COORD2(shift + 14,   96), AID_MWS));
    AddArea(new WarningLightsArea   (*this, _COORD2(shift + 80,   49), AID_WARNING_LIGHTS));
    AddArea(new XR2MWSTestButtonArea(*this, _COORD2(shift + 22,   68), AID_MWS_TEST_BUTTON));
    AddArea(new XR2WarningLightsArea(*this, _COORD2(shift + 54,  115), AID_XR2_WARNING_LIGHTS));

    // add other areas
    AddArea(new SystemsDisplayScreen(*this, _COORD2(shift + 1370, 74), AID_SYSTEMS_DISPLAY_SCREEN));
}

// width = panel width: 1280, 1600, 1920
void XR2UpperInstrumentPanel::AddCommonAreas(const int width)
{
    const int shift = (width - 1280) / 2;

    // create our components
    AddComponent(new AngularDataComponent   (*this, _COORD2( shift + 541,   6)));
    AddComponent(new APUPanelComponent      (*this, _COORD2( shift +  19,  46)));

    //
    // Create our areas
    //

    // light switches and LEDs
    int switchY = 89;
    int ledY   = 140;
    ADD_SWITCH_AND_LED(NavLightToggleSwitchArea,    shift + 864, AID_NAVLIGHTSWITCH, AID_SWITCHLED_NAV,    beacon[0].active);
    ADD_SWITCH_AND_LED(BeaconLightToggleSwitchArea, shift + 900, AID_BEACONSWITCH,   AID_SWITCHLED_BEACON, beacon[4].active);
    ADD_SWITCH_AND_LED(StrobeLightToggleSwitchArea, shift + 937, AID_STROBESWITCH,   AID_SWITCHLED_STROBE, beacon[6].active);

    // main switches and indicators
    switchY = 59;
    int indicatorY = 111;
    ADD_SWITCH_AND_INDICATOR(BayDoorsToggleSwitchArea,  shift + 80,  AID_BAYDOORSSWITCH,  AID_BAYDOORSINDICATOR,  bay_status,      IDB_INDICATOR_OC, bay_proc);
    ADD_SWITCH_AND_INDICATOR(NoseConeToggleSwitchArea,  shift + 125, AID_NOSECONESWITCH,  AID_NOSECONEINDICATOR,  nose_status,     IDB_INDICATOR_OC, nose_proc);
    ADD_SWITCH_AND_INDICATOR(OuterDoorToggleSwitchArea, shift + 170, AID_OUTERDOORSWITCH, AID_OUTERDOORINDICATOR, olock_status,    IDB_INDICATOR_OC, olock_proc);
    ADD_SWITCH_AND_INDICATOR(InnerDoorToggleSwitchArea, shift + 215, AID_INNERDOORSWITCH, AID_INNERDOORINDICATOR, ilock_status,    IDB_INDICATOR_OC, ilock_proc);
    ADD_SWITCH_AND_INDICATOR(ChamberToggleSwitchArea,   shift + 260, AID_CHAMBERSWITCH,   AID_CHAMBERINDICATOR,   chamber_status,  IDB_INDICATOR_AV, chamber_proc);
    ADD_SWITCH_AND_INDICATOR(AirbrakeToggleSwitchArea,  shift + 316, AID_AIRBRAKESWITCH,  AID_AIRBRAKEINDICATOR,  brake_status,    IDB_INDICATOR_SD, brake_proc);
    ADD_SWITCH_AND_INDICATOR(HatchToggleSwitchArea,     shift + 361, AID_HATCHSWITCH,     AID_HATCHINDICATOR,     hatch_status,    IDB_INDICATOR_OC, hatch_proc);
    ADD_SWITCH_AND_INDICATOR(RadiatorToggleSwitchArea,  shift + 406, AID_RADIATORSWITCH,  AID_RADIATORINDICATOR,  radiator_status, IDB_INDICATOR_SD, radiator_proc);
    ADD_SWITCH_AND_INDICATOR(RetroDoorToggleSwitchArea, shift + 451, AID_RETRODOORSWITCH, AID_RETRODOORINDICATOR, rcover_status,   IDB_INDICATOR_OC, rcover_proc);
    ADD_SWITCH_AND_INDICATOR(HoverDoorToggleSwitchArea, shift + 496, AID_HOVERDOORSWITCH, AID_HOVERDOORINDICATOR, hoverdoor_status,IDB_INDICATOR_OC, hoverdoor_proc);
    
    // add bottom row of switches
    switchY = 177;
    indicatorY = 229;
    ADD_SWITCH_AND_INDICATOR(ScramDoorToggleSwitchArea, shift + 496, AID_SCRAMDOORSWITCH, AID_SCRAMDOORINDICATOR, scramdoor_status,IDB_INDICATOR_OC, scramdoor_proc);
    ADD_SWITCH_AND_INDICATOR(GearToggleSwitchArea,      shift + 757, AID_GEARSWITCH,      AID_GEARINDICATOR,      gear_status,     IDB_INDICATOR_UD, gear_proc);

    // door override interlock buttons
    AddArea(new OverrideOuterAirlockToggleButtonArea(*this, _COORD2( shift + 164, 148), AID_ARM_OUTER_AIRLOCK_DOOR));
    AddArea(new OverrideCrewHatchToggleButtonArea   (*this, _COORD2( shift + 355, 148), AID_ARM_CREW_HATCH));

    AddArea(new ScramTempGaugeArea(*this, _COORD2( shift + 758,  23), AID_SCRAMTEMPDISP));
    AddArea(new XR2CrewDisplayArea(*this, _COORD2( shift + 546, 242), AID_CREW_DISPLAY));

    // "Payload panel view"
    AddArea(new SwitchToPanelButtonArea(*this, _COORD2(shift + 16, 147), AID_SWITCH_TO_PAYLOAD_CAMERA_VIEW, PANEL_PAYLOAD));
}

//-------------------------------------------------------------------------

// Activate and initialize the UPPER panel
// Invoked from VESSEL2's InitPanel method.
// Load our surface bitmaps. The inverse of this method is 'Deactivate'
// Returns: true on success, false on error (e.g., a bitmap failed to load)
bool XR2UpperInstrumentPanel::Activate()
{
    const WORD panelResourceID = GetPanelResourceID();
    
    // load our bitmap
    m_hBmp = LoadBitmap(GetVessel().GetModuleHandle(), MAKEINTRESOURCE (panelResourceID));
    if (m_hBmp == nullptr)
        return false;       // should never happen
    
    oapiRegisterPanelBackground (m_hBmp, PANEL_ATTACH_TOP | PANEL_MOVEOUT_TOP, 0xFFFFFF);  // white is transparent
    oapiSetPanelNeighbours (PANEL_PAYLOAD, -1, -1, PANEL_MAIN);
    GetVessel().SetCameraOffset(twoDCockpitCoordinates);
    GetVessel().SetXRCameraDirection (_V(0,0.5,0.866)); // look up

    // activate all our areas, including our components' areas
    ActivateAllAreas();

    GetXR2().campos = GetXR2().CAM_PANELUP;
    return true;
}

//-------------------------------------------------------------------------

// Activate and initialize the LOWER panel
// Invoked from VESSEL2's InitPanel method.
// Load our surface bitmaps. The inverse of this method is 'Deactivate'
// Returns: true on success, false on error (e.g., a bitmap failed to load)
bool XR2LowerInstrumentPanel::Activate()
{
    const WORD panelResourceID = GetPanelResourceID();
    
    // load our bitmap
    m_hBmp = LoadBitmap(GetVessel().GetModuleHandle(), MAKEINTRESOURCE (panelResourceID));
    if (m_hBmp == nullptr)
        return false;       // should never happen

    GetVessel().SetCameraOffset(twoDCockpitCoordinates);
    GetVessel().SetXRCameraDirection(_V(0,-0.707,0.707)); // look down

    // NOTE: the lower panel is OPAQUE!  
    oapiRegisterPanelBackground (m_hBmp, PANEL_ATTACH_TOP | GetXR2().GetLowerPanelMoveoutFlag());  // do NOT attach at the bottom; this will prevent stretching
    oapiSetPanelNeighbours (-1, -1, PANEL_MAIN, -1);

    // activate all our areas, including our components' areas
    ActivateAllAreas();

    GetXR2().campos = GetXR2().CAM_PANELDN;
    return true;
}

// add areas common to all panels
// width = panel width
void XR2LowerInstrumentPanel::AddCommonAreas(const int width)
{
    const int shift = (width - 1600) / 2;   // calibrated below for 1600 pixels, but the end result is the same

    // create our components
    
    AddComponent(new METTimerComponent      (*this, _COORD2(shift +  588, 108)));

    AddComponent(new MainFuelGaugeComponent (*this, _COORD2(shift +  417, 193)));
    AddComponent(new RCSFuelGaugeComponent  (*this, _COORD2(shift +  522, 193)));
    AddComponent(new SCRAMFuelGaugeComponent(*this, _COORD2(shift +  628, 193)));
    AddComponent(new APUFuelGaugeComponent  (*this, _COORD2(shift +  732, 193)));
    AddComponent(new FuelHatchComponent     (*this, _COORD2(shift + 1053, 267)));
    AddComponent(new LoxHatchComponent      (*this, _COORD2(shift + 1120, 267)));

    AddComponent(new MainSupplyLineGaugeComponent (*this, _COORD2(shift +  830, 209)));
    AddComponent(new ScramSupplyLineGaugeComponent(*this, _COORD2(shift +  881, 209)));
    AddComponent(new ApuSupplyLineGaugeComponent  (*this, _COORD2(shift +  932, 209)));
    AddComponent(new LoxSupplyLineGaugeComponent  (*this, _COORD2(shift +  983, 209)));

    AddComponent(new ShipMassDisplayComponent     (*this, _COORD2(shift + 1043, 200)));
    AddComponent(new LoxGaugeComponent            (*this, _COORD2(shift + 1217, 178)));
    AddComponent(new OxygenRemainingPanelComponent(*this, _COORD2(shift + 1299,  87)));
    AddComponent(new CoolantGaugeComponent        (*this, _COORD2(shift + 1326, 178)));
    AddComponent(new XR2WarningLightsComponent    (*this, _COORD2(shift + 1058,  90)));  // Note: XR5 is +1057

    AddComponent(new ExternalCoolingComponent     (*this, _COORD2(shift + 1394, 207)));

    // create our areas
    AddArea(new DockReleaseButtonArea  (*this, _COORD2(shift +  187, 562), AID_DOCKRELEASE));
    AddArea(new AOAAnalogGaugeArea     (*this, _COORD2(shift +  982, 374), AID_AOAINSTR));
    AddArea(new SlipAnalogGaugeArea    (*this, _COORD2(shift + 1082, 374), AID_SLIPINSTR));
    AddArea(new ArtificialHorizonArea  (*this, _COORD2(shift +  837, 355), AID_HORIZON));
    AddArea(new MWSArea                (*this, _COORD2(shift + 1157,  94), AID_MWS));
    AddArea(new APUButton              (*this, _COORD2(shift + 1151, 138), AID_APU_BUTTON));
    AddArea(new XFeedKnobArea          (*this, _COORD2(shift +  473, 417), AID_XFEED_KNOB));
    AddArea(new SystemsDisplayScreen   (*this, _COORD2(shift + 1199, 409), AID_SYSTEMS_DISPLAY_SCREEN));
    AddArea(new DoorMediumLEDArea      (*this, _COORD2(shift + 1393, 323), AID_RADIATOR_DEPLOYED_LED, GetXR2().radiator_status, true));  // redraw always
    AddArea(new AlteaAerospaceArea     (*this, _COORD2(shift +  379,  88), AID_ALTEA_LOGO));

#ifdef TURBOPACKS
    AddArea(new TurbopackDisplayArea   (*this, _COORD2(shift +  362, 561), AID_TURBOPACK_MANAGEMENT_SCREEN));
#endif

    // add supply line switches and LEDs
    const int switchY = 421;
    const int ledY   = 467;
    ADD_SUPPLY_SWITCH_AND_LED(584 + shift, AID_MAINSUPPLYLINE_SWITCH,  AID_MAINSUPPLYLINE_SWITCH_LED,  m_mainFuelFlowSwitch,  m_mainSupplyLineStatus);
    ADD_SUPPLY_SWITCH_AND_LED(622 + shift, AID_SCRAMSUPPLYLINE_SWITCH, AID_SCRAMSUPPLYLINE_SWITCH_LED, m_scramFuelFlowSwitch, m_scramSupplyLineStatus);
    ADD_SUPPLY_SWITCH_AND_LED(660 + shift, AID_APUSUPPLYLINE_SWITCH,   AID_APUSUPPLYLINE_SWITCH_LED,   m_apuFuelFlowSwitch,   m_apuSupplyLineStatus);
    ADD_SUPPLY_SWITCH_AND_LED(698 + shift, AID_LOXSUPPLYLINE_SWITCH,   AID_LOXSUPPLYLINE_SWITCH_LED,   m_loxFlowSwitch,       m_loxSupplyLineStatus);
}

// add areas common to 1600-pixel-wide or wider panels
// width = panel width
void XR2LowerInstrumentPanel::Add1600PlusAreas(const int width)
{
    const int shift = (width - 1600) / 2;   // calibrated below for 1600 pixels, but the end result is the same

    AddComponent(new MainThrottleComponent  (*this, _COORD2(shift +  193,  71)));
    AddComponent(new HoverThrottleComponent (*this, _COORD2(shift +  193, 299)));
    AddComponent(new ScramThrottleComponent (*this, _COORD2(shift +  193, 400)));
    AddComponent(new MainHoverPanelComponent(*this, _COORD2(shift +  364, 402)));
    AddComponent(new Interval1TimerComponent(*this, _COORD2(shift +  827,  83)));
    AddComponent(new Interval2TimerComponent(*this, _COORD2(shift +  827, 125)));
    
}

//-------------------------------------------------------------------------
// Resolution-specific instrument panels begin here
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
// 1280-pixel-wide panels
//-------------------------------------------------------------------------

// Constructor
// vessel = our parent vessel
XR2MainInstrumentPanel1280::XR2MainInstrumentPanel1280(XR2Ravenstar &vessel) : 
    XR2MainInstrumentPanel(vessel, IDB_PANEL1_1280)
{
    // create our components
    AddComponent(new MFDComponent                 (*this, _COORD2(   0, 242), MFD_LEFT));
    AddComponent(new MFDComponent                 (*this, _COORD2( 879, 242), MFD_RIGHT));
    AddComponent(new ScramPanelComponent          (*this, _COORD2( 662, 368)));
    AddComponent(new EngineDisplayComponent       (*this, _COORD2( 471, 336)));
    AddComponent(new FuelDisplayComponent         (*this, _COORD2( 471, 252)));
    AddComponent(new DynamicPressurePanelComponent(*this, _COORD2( 748, 480)));
    AddComponent(new ScramTempPanelComponent      (*this, _COORD2( 745, 410)));
    AddComponent(new SlopePanelComponent          (*this, _COORD2( 746, 252)));
    AddComponent(new AOAPanelComponent            (*this, _COORD2( 792, 252)));
    AddComponent(new SlipPanelComponent           (*this, _COORD2( 745, 358)));
    AddComponent(new APUPanelComponent            (*this, _COORD2( 838, 252)));
    AddComponent(new CenterOfGravityPanelComponent(*this, _COORD2( 662, 484)));
    AddComponent(new XR2WarningLightsComponent    (*this, _COORD2(1040, 159)));

    // create our areas
    AddArea(new HudModeButtonsArea          (*this, _COORD2(  15,  128), AID_HUDMODE));
    AddArea(new ElevatorTrimArea            (*this, _COORD2( 188,  182), AID_ELEVATORTRIM));
    AddArea(new AutopilotButtonsArea        (*this, _COORD2(   5,  161), AID_AUTOPILOTBUTTONS));
    AddArea(new MWSArea                     (*this, _COORD2(1071,  116), AID_MWS));
    AddArea(new RCSModeArea                 (*this, _COORD2(1217,  182), AID_RCSMODE));
    AddArea(new AFCtrlArea                  (*this, _COORD2(1141,  182), AID_AFCTRLMODE));
    AddArea(new MainThrottleArea            (*this, _COORD2( 408,  242), AID_ENGINEMAIN));
    AddArea(new LargeHoverThrottleArea      (*this, _COORD2( 428,  429), AID_ENGINEHOVER));
    AddArea(new ScramThrottleArea           (*this, _COORD2( 688,  245), AID_ENGINESCRAM));
    AddArea(new HudIntensitySwitchArea      (*this, _COORD2( 216,  190), AID_HUDINTENSITY)); 
    AddArea(new HudColorButtonArea          (*this, _COORD2( 241,  222), AID_HUDCOLOR)); 
    AddArea(new AutopilotLEDArea            (*this, _COORD2( 134,  130), AID_AUTOPILOTLED));
    AddArea(new SecondaryHUDModeButtonsArea (*this, _COORD2(1110,  128), AID_SECONDARY_HUD_BUTTONS));
    AddArea(new SecondaryHUDArea            (*this, _COORD2(1064,   17), AID_SECONDARY_HUD));
    AddArea(new TertiaryHUDButtonArea       (*this, _COORD2( 181,  134), AID_TERTIARY_HUD_BUTTON)); 
    AddArea(new TertiaryHUDArea             (*this, _COORD2(   7,   17), AID_TERTIARY_HUD));
    AddArea(new WingLoadAnalogGaugeArea     (*this, _COORD2( 800,  497), AID_LOADINSTR));
    AddArea(new StaticPressureNumberArea    (*this, _COORD2( 794,  462), AID_STATIC_PRESSURE));
    AddArea(new DeployRadiatorButtonArea    (*this, _COORD2( 663,  337), AID_DEPLOY_RADIATOR_BUTTON));
    AddArea(new DataHUDButtonArea           (*this, _COORD2(1128,  226), AID_DATA_HUD_BUTTON));

    //
    // Intialize MultiDisplayArea 
    //
    MultiDisplayArea *pMDA = new MultiDisplayArea(*this, _COORD2( 471,  465), AID_MULTI_DISPLAY);
    InitMDA(pMDA);
    AddArea(pMDA);    // now add the Area to the panel
}

//----------------------------------------------------------------------

// Constructor
// vessel = our parent vessel
XR2UpperInstrumentPanel1280::XR2UpperInstrumentPanel1280(XR2Ravenstar &vessel) : 
    XR2UpperInstrumentPanel(vessel, IDB_PANEL2_1280)
{
    AddCommonAreas(1280);

    AddComponent(new METTimerComponent        (*this, _COORD2(990,  80)));
    AddComponent(new Interval1TimerComponent  (*this, _COORD2(990, 127)));
}

//-------------------------------------------------------------------------

// Constructor
// vessel = our parent vessel
XR2LowerInstrumentPanel1280::XR2LowerInstrumentPanel1280(XR2Ravenstar &vessel) : 
    XR2LowerInstrumentPanel(vessel, IDB_PANEL3_1280)
{
    AddCommonAreas(1280);

    AddComponent(new Interval2TimerComponent(*this, _COORD2( 667, 108)));
    AddComponent(new MainThrottleComponent  (*this, _COORD2(  22,  71)));
    AddComponent(new HoverThrottleComponent (*this, _COORD2(  22, 299)));
    AddComponent(new ScramThrottleComponent (*this, _COORD2(  22, 400)));
    AddComponent(new MainHoverPanelComponent(*this, _COORD2( 200, 402)));
}

//-------------------------------------------------------------------------
// 1600-pixel-wide panels
//-------------------------------------------------------------------------

// Constructor
// vessel = our parent vessel
XR2MainInstrumentPanel1600::XR2MainInstrumentPanel1600(XR2Ravenstar &vessel) : 
    XR2MainInstrumentPanel(vessel, IDB_PANEL1_1600)
{
    // create our components
    AddComponent(new MFDComponent                 (*this, _COORD2(   0, 242), MFD_LEFT));
    AddComponent(new MFDComponent                 (*this, _COORD2(1199, 242), MFD_RIGHT));
    AddComponent(new ScramPanelComponent          (*this, _COORD2( 963, 358)));
    AddComponent(new EngineDisplayComponent       (*this, _COORD2( 763, 336)));
    AddComponent(new FuelDisplayComponent         (*this, _COORD2( 750, 252)));
    AddComponent(new DynamicPressurePanelComponent(*this, _COORD2(1059, 480)));
    AddComponent(new ScramTempPanelComponent      (*this, _COORD2(1056, 410)));
    AddComponent(new SlopePanelComponent          (*this, _COORD2(1057, 252)));
    AddComponent(new AOAPanelComponent            (*this, _COORD2(1103, 252)));
    AddComponent(new SlipPanelComponent           (*this, _COORD2(1056, 358)));
    AddComponent(new APUPanelComponent            (*this, _COORD2(1149, 252)));
    AddComponent(new MainThrottleComponent        (*this, _COORD2( 420, 253)));
    AddComponent(new HoverThrottleComponent       (*this, _COORD2( 420, 481)));
    AddComponent(new ScramThrottleComponent       (*this, _COORD2( 592, 469)));
    AddComponent(new METTimerComponent            (*this, _COORD2( 598, 277)));
    AddComponent(new CenterOfGravityPanelComponent(*this, _COORD2( 964, 480)));
    AddComponent(new XR2WarningLightsComponent    (*this, _COORD2(1360, 159)));

    // create our areas
    AddArea(new HudModeButtonsArea          (*this, _COORD2(  15,  128), AID_HUDMODE));
    AddArea(new ElevatorTrimArea            (*this, _COORD2( 188,  182), AID_ELEVATORTRIM));
    AddArea(new AutopilotButtonsArea        (*this, _COORD2(   5,  161), AID_AUTOPILOTBUTTONS));
    AddArea(new MWSArea                     (*this, _COORD2(1391,  116), AID_MWS));
    AddArea(new RCSModeArea                 (*this, _COORD2(1537,  182), AID_RCSMODE));
    AddArea(new AFCtrlArea                  (*this, _COORD2(1461,  182), AID_AFCTRLMODE));

    AddArea(new HudIntensitySwitchArea      (*this, _COORD2( 216,  190), AID_HUDINTENSITY)); 
    AddArea(new HudColorButtonArea          (*this, _COORD2( 241,  222), AID_HUDCOLOR)); 
    AddArea(new AutopilotLEDArea            (*this, _COORD2( 134,  130), AID_AUTOPILOTLED));
    AddArea(new SecondaryHUDModeButtonsArea (*this, _COORD2(1430,  128), AID_SECONDARY_HUD_BUTTONS));
    AddArea(new SecondaryHUDArea            (*this, _COORD2(1384,   17), AID_SECONDARY_HUD));
    AddArea(new TertiaryHUDButtonArea       (*this, _COORD2( 181,  134), AID_TERTIARY_HUD_BUTTON)); 
    AddArea(new TertiaryHUDArea             (*this, _COORD2(   7,   17), AID_TERTIARY_HUD));
    AddArea(new WingLoadAnalogGaugeArea     (*this, _COORD2(1111,  497), AID_LOADINSTR));
    AddArea(new StaticPressureNumberArea    (*this, _COORD2(1105,  462), AID_STATIC_PRESSURE));
    AddArea(new DeployRadiatorButtonArea    (*this, _COORD2( 974,  323), AID_DEPLOY_RADIATOR_BUTTON));
    AddArea(new DataHUDButtonArea           (*this, _COORD2(1448,  226), AID_DATA_HUD_BUTTON));

    // add switches and indicators
    int switchY = 371;
    int indicatorY = 423;
    ADD_SWITCH_AND_INDICATOR(RetroDoorToggleSwitchArea, 596, AID_RETRODOORSWITCH, AID_RETRODOORINDICATOR, rcover_status,   IDB_INDICATOR_OC, rcover_proc);
    ADD_SWITCH_AND_INDICATOR(HoverDoorToggleSwitchArea, 654, AID_HOVERDOORSWITCH, AID_HOVERDOORINDICATOR, hoverdoor_status,IDB_INDICATOR_OC, hoverdoor_proc);
    ADD_SWITCH_AND_INDICATOR(ScramDoorToggleSwitchArea, 712, AID_SCRAMDOORSWITCH, AID_SCRAMDOORINDICATOR, scramdoor_status,IDB_INDICATOR_OC, scramdoor_proc);

    switchY = 256;
    indicatorY = 307;
    ADD_SWITCH_AND_INDICATOR(GearToggleSwitchArea,     1010, AID_GEARSWITCH,      AID_GEARINDICATOR,      gear_status,     IDB_INDICATOR_UD, gear_proc);
    
    //
    // Intialize MultiDisplayArea touch-screen
    //
    MultiDisplayArea *pMDA = new MultiDisplayArea(*this, _COORD2( 763,  465), AID_MULTI_DISPLAY);
    InitMDA(pMDA);
    AddArea(pMDA);
}

//----------------------------------------------------------------------

// Constructor
// vessel = our parent vessel
XR2UpperInstrumentPanel1600::XR2UpperInstrumentPanel1600(XR2Ravenstar &vessel) : 
    XR2UpperInstrumentPanel(vessel, IDB_PANEL2_1600)
{
    const int width = 1600;
    
    AddCommonAreas(width);
    Add1600PlusAreas(width);

    // logo is in a unique location on this panel
    AddArea(new AlteaAerospaceArea(*this, _COORD2(1375, 74), AID_ALTEA_LOGO));   
}
//-------------------------------------------------------------------------

// Constructor
// vessel = our parent vessel
XR2LowerInstrumentPanel1600::XR2LowerInstrumentPanel1600(XR2Ravenstar &vessel) : 
    XR2LowerInstrumentPanel(vessel, IDB_PANEL3_1600)
{
    const int width = 1600;

    AddCommonAreas(width);
    Add1600PlusAreas(width);
}

//-------------------------------------------------------------------------
// 1920-pixel-wide panels
//-------------------------------------------------------------------------

// Constructor
// vessel = our parent vessel
XR2MainInstrumentPanel1920::XR2MainInstrumentPanel1920(XR2Ravenstar &vessel) : 
    XR2MainInstrumentPanel(vessel, IDB_PANEL1_1920)
{
    // create our components
    AddComponent(new MFDComponent                 (*this, _COORD2(   0, 242), MFD_LEFT));
    AddComponent(new MFDComponent                 (*this, _COORD2(1519, 242), MFD_RIGHT));
    AddComponent(new MainThrottleComponent        (*this, _COORD2( 419, 253)));
    AddComponent(new HoverThrottleComponent       (*this, _COORD2( 419, 481)));
    AddComponent(new ScramThrottleComponent       (*this, _COORD2( 596, 469)));
    AddComponent(new METTimerComponent            (*this, _COORD2( 605, 277)));
    AddComponent(new Interval1TimerComponent      (*this, _COORD2( 771, 277)));
    AddComponent(new ScramPanelComponent          (*this, _COORD2( 767, 474)));
    AddComponent(new EngineDisplayComponent       (*this, _COORD2( 957, 336)));
    AddComponent(new FuelDisplayComponent         (*this, _COORD2( 943, 252)));
    AddComponent(new Interval2TimerComponent      (*this, _COORD2(1176, 277)));
    AddComponent(new DynamicPressurePanelComponent(*this, _COORD2(1323, 350)));
    AddComponent(new ScramTempPanelComponent      (*this, _COORD2(1372, 410)));
    AddComponent(new SlipPanelComponent           (*this, _COORD2(1372, 358)));
    AddComponent(new SlopePanelComponent          (*this, _COORD2(1373, 252)));
    AddComponent(new AOAPanelComponent            (*this, _COORD2(1419, 252)));
    AddComponent(new APUPanelComponent            (*this, _COORD2(1465, 252)));
    AddComponent(new CenterOfGravityPanelComponent(*this, _COORD2( 863, 478)));
    AddComponent(new XR2WarningLightsComponent    (*this, _COORD2(1680, 159)));

    // create our areas
    AddArea(new AutopilotButtonsArea        (*this, _COORD2(   5,  161), AID_AUTOPILOTBUTTONS));
    AddArea(new HudModeButtonsArea          (*this, _COORD2(  15,  128), AID_HUDMODE));
    AddArea(new ElevatorTrimArea            (*this, _COORD2( 188,  182), AID_ELEVATORTRIM));
    AddArea(new SystemsDisplayScreen        (*this, _COORD2(1169,  481), AID_SYSTEMS_DISPLAY_SCREEN));
    AddArea(new MWSArea                     (*this, _COORD2(1711,  116), AID_MWS));
    AddArea(new RCSModeArea                 (*this, _COORD2(1857,  182), AID_RCSMODE));
    AddArea(new AFCtrlArea                  (*this, _COORD2(1781,  182), AID_AFCTRLMODE));

    AddArea(new HudIntensitySwitchArea      (*this, _COORD2( 216,  190), AID_HUDINTENSITY)); 
    AddArea(new HudColorButtonArea          (*this, _COORD2( 241,  222), AID_HUDCOLOR)); 
    AddArea(new AutopilotLEDArea            (*this, _COORD2( 134,  130), AID_AUTOPILOTLED));
    AddArea(new SecondaryHUDModeButtonsArea (*this, _COORD2(1750,  128), AID_SECONDARY_HUD_BUTTONS));
    AddArea(new SecondaryHUDArea            (*this, _COORD2(1704,   17), AID_SECONDARY_HUD));
    AddArea(new TertiaryHUDButtonArea       (*this, _COORD2( 181,  134), AID_TERTIARY_HUD_BUTTON)); 
    AddArea(new TertiaryHUDArea             (*this, _COORD2(   7,   17), AID_TERTIARY_HUD));
    AddArea(new WingLoadAnalogGaugeArea     (*this, _COORD2(1427,  497), AID_LOADINSTR));
    AddArea(new StaticPressureNumberArea    (*this, _COORD2(1421,  462), AID_STATIC_PRESSURE));
    AddArea(new DataHUDButtonArea           (*this, _COORD2(1768,  226), AID_DATA_HUD_BUTTON));
    // OLD: AddArea(new AlteaAerospaceArea          (*this, _COORD2(1155,  368), AID_ALTEA_LOGO));
    AddArea(new ArtificialHorizonArea       (*this, _COORD2(1182,  346), AID_HORIZON));

    // add switches and indicators
    int switchY = 371;
    int indicatorY = 423;
    ADD_SWITCH_AND_INDICATOR(RetroDoorToggleSwitchArea, 603, AID_RETRODOORSWITCH, AID_RETRODOORINDICATOR, rcover_status,   IDB_INDICATOR_OC, rcover_proc);
    ADD_SWITCH_AND_INDICATOR(HoverDoorToggleSwitchArea, 661, AID_HOVERDOORSWITCH, AID_HOVERDOORINDICATOR, hoverdoor_status,IDB_INDICATOR_OC, hoverdoor_proc);
    ADD_SWITCH_AND_INDICATOR(ScramDoorToggleSwitchArea, 719, AID_SCRAMDOORSWITCH, AID_SCRAMDOORINDICATOR, scramdoor_status,IDB_INDICATOR_OC, scramdoor_proc);
    ADD_SWITCH_AND_INDICATOR(RadiatorToggleSwitchArea,  777, AID_RADIATORSWITCH,  AID_RADIATORINDICATOR,  radiator_status, IDB_INDICATOR_SD, radiator_proc);
    ADD_SWITCH_AND_INDICATOR(AirbrakeToggleSwitchArea,  835, AID_AIRBRAKESWITCH,  AID_AIRBRAKEINDICATOR,  brake_status,    IDB_INDICATOR_SD, brake_proc);
    ADD_SWITCH_AND_INDICATOR(GearToggleSwitchArea,      893, AID_GEARSWITCH,      AID_GEARINDICATOR,      gear_status,     IDB_INDICATOR_UD, gear_proc);

    //
    // Intialize MultiDisplayArea touch-screen
    //
    MultiDisplayArea *pMDA = new MultiDisplayArea(*this, _COORD2( 957,  465), AID_MULTI_DISPLAY);
    InitMDA(pMDA);
    AddArea(pMDA);
}

//----------------------------------------------------------------------

// Constructor
// vessel = our parent vessel
XR2UpperInstrumentPanel1920::XR2UpperInstrumentPanel1920(XR2Ravenstar &vessel) : 
    XR2UpperInstrumentPanel(vessel, IDB_PANEL2_1920)
{
    const int width = 1920;

    AddCommonAreas(width);     
    Add1600PlusAreas(width);

    // logo is in a unique location on this panel
    AddArea(new AlteaAerospaceArea(*this, _COORD2(15, 54), AID_ALTEA_LOGO));   
}

//-------------------------------------------------------------------------

// Constructor
// vessel = our parent vessel
XR2LowerInstrumentPanel1920::XR2LowerInstrumentPanel1920(XR2Ravenstar &vessel) : 
    XR2LowerInstrumentPanel(vessel, IDB_PANEL3_1920)
{
    AddCommonAreas(1920);
    Add1600PlusAreas(1920);
}

// glass cockpit
bool XR2Ravenstar::clbkLoadGenericCockpit()
{
    SetCameraOffset(twoDCockpitCoordinates);
    oapiSetDefNavDisplay (1);
    oapiSetDefRCSDisplay (1);
    campos = CAM_GENERIC;

    return true;
}
