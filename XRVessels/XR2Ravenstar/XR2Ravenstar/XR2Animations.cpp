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
// XR2Ravenstar implementation class
//
// Animations.cpp
// Contains animation code for the XR2Ravenstar
// ==============================================================

#include "XR2Ravenstar.h"
#include "meshres.h"

// size of a mesh group array
#define SizeOfGrp(grp) (sizeof(grp) / sizeof(UINT))

// Virtual Gateway method that decides which animations are valid for this vessel; if the incoming animation handle is valid, 
// the call is propogated up to SetAnimation.  Otherwise, this method returns without changing the animation state.
#define ALLOW(handlePtr)  if (&anim == &(handlePtr)) { SetAnimation(anim, state); return; }
void XR2Ravenstar::SetXRAnimation(const UINT &anim, const double state) const
{
    ALLOW(anim_rcover);       // handle for retro cover animation
    ALLOW(anim_hoverdoor);    // handle for hover doors animation
    ALLOW(anim_scramdoor);    // handle for scram doors animation
    ALLOW(anim_nose);         // handle for nose cone animation
    ALLOW(anim_olock);        // handle for outer airlock animation
    ALLOW(anim_ilock);        // handle for inner airlock animation
    ALLOW(anim_hatch);        // handle for top hatch animation
    ALLOW(anim_radiator);     // handle for radiator animation
    ALLOW(anim_rudder);       // handle for rudder animation
	ALLOW(anim_elevator);     // handle for elevator animation
	ALLOW(anim_elevatortrim); // handle for elevator trim animation
	ALLOW(anim_laileron);     // handle for left aileron animation
	ALLOW(anim_raileron);     // handle for right aileron animation
	ALLOW(anim_brake);        // handle for airbrake animation
    ALLOW(anim_fuelhatch);    // handle for fuel hatch animation
    ALLOW(anim_loxhatch);     // handle for LOX hatch animation
    ALLOW(anim_gear);         // handle for landing gear animation

    // New for XR2
    ALLOW(anim_bay);          // handle for bay doors animation
    ALLOW(m_animFrontTireRotation);
    ALLOW(m_animRearTireRotation);
    /* Not until the MK II
    ALLOW(m_animNoseGearCompression);
    ALLOW(m_animRearGearCompression);
    */

    // NO: ALLOW(m_animNosewheelSteering); 
	// NO: ALLOW(anim_ladder);       // handle for front escape ladder animation
}

// --------------------------------------------------------------
// Define animation sequences for moving parts
// Invoked by our constructor.
// --------------------------------------------------------------
void XR2Ravenstar::DefineAnimations()
{
    // ***** Landing gear animation *****  
    // nose gear strut
    static UINT NWheelStrutGrp[] = {GRP_Object34};
    static MGROUP_ROTATE NWheelStrut (0, NWheelStrutGrp, SizeOfGrp(NWheelStrutGrp),
        _V(0.0, -0.697, 6.431), _V(1,0,0), static_cast<float>(-98.93 * RAD));

    // inner front gear strut; parented to main strut
    // TRANSLATE the gear struts down to extend them for landing
    const double gearStrutTranslation = 0.5;     // applies to rear as well
    // for Mk II when compression added: const double gearStrutTranslation = 0.60;     // applies to rear as well
    static UINT FrontInnerStrutGrp[] = {GRP_front_inner_strut};
    m_frontInnerStrut = new MGROUP_TRANSLATE(0, FrontInnerStrutGrp, SizeOfGrp(FrontInnerStrutGrp),
        _V(0.0, 0.157 * gearStrutTranslation, -0.988 * gearStrutTranslation));

    // nose gear doors
    static UINT NWheelFCoverGrp[] = {GRP_central_front_gear_door, GRP_central_front_gear_door_inner};
    static MGROUP_ROTATE NWheelFCover (0, NWheelFCoverGrp, SizeOfGrp(NWheelFCoverGrp),
        _V(0, -0.992, 6.560), _V(1,0,0), -static_cast<float>(110.0 * RAD));  // must open farther to clear the tires
    static MGROUP_ROTATE NWheelFCoverClose (0, NWheelFCoverGrp, SizeOfGrp(NWheelFCoverGrp),
        _V(0, -0.992, 6.560), _V(1,0,0), static_cast<float>(20 * RAD));  // close back to 90 degrees
    
    const float noseDoorRotation = static_cast<float>(90 * RAD);
    static UINT NWheelLCoverGrp[] = {GRP_Port_front_gear_door, GRP_Port_front_gear_door_inner};
    static MGROUP_ROTATE NWheelLCover (0, NWheelLCoverGrp, SizeOfGrp(NWheelLCoverGrp),
        _V(-0.368, -0.921, 4.987), _V(0.0, 0.028, 1.0), -noseDoorRotation);
    static MGROUP_ROTATE NWheelLCoverClose (0, NWheelLCoverGrp, SizeOfGrp(NWheelLCoverGrp),
        _V(-0.368, -0.921, 4.987), _V(0.0, 0.028, 1.0), noseDoorRotation);
    
    static UINT NWheelRCoverGrp[] = {GRP_starboard_front_gear_door, GRP_starboard_front_gear_door_inner};
    static MGROUP_ROTATE NWheelRCover (0, NWheelRCoverGrp, SizeOfGrp(NWheelRCoverGrp),
        _V(0.368, -0.921, 4.987), _V(0.0, 0.029, 1.0), noseDoorRotation);
    static MGROUP_ROTATE NWheelRCoverClose (0, NWheelRCoverGrp, SizeOfGrp(NWheelRCoverGrp),
        _V(0.368, -0.921, 4.987), _V(0.0, 0.029, 1.0), -noseDoorRotation);

    // rear gear struts (main)
    const float rearStrutsRotation = static_cast<float>(87 * RAD);  // do not rotate to vertical
    static UINT AftStrutsGrp[] = {GRP_Cylinder16};  // outer strut only
    static MGROUP_ROTATE AftStruts(0, AftStrutsGrp, SizeOfGrp(AftStrutsGrp),
        _V(0.0, -0.145, -2.918), _V(1,0,0), -rearStrutsRotation);   

    // inner aft gear struts; parented to main struts
    // TRANSLATE the gear struts down to extend them for landing
    // rear gear translation factor due to rear sweep angle (11.0 degrees)
    // Note: should be this, but rear gear is taller than front to start with: const double REAR_GEAR_TRANSLATION_FACTOR = 1.01872146;
    const double rearGearTranslationFactor = 0.67;   // tuned by hand for our current touchdown points
    static UINT AftInnerStrutsGrp[] = {GRP_Object10};
    m_aftInnerStruts = new MGROUP_TRANSLATE(0, AftInnerStrutsGrp, SizeOfGrp(AftInnerStrutsGrp),
        _V(0.0, 0.138 * gearStrutTranslation * rearGearTranslationFactor, -0.99 * gearStrutTranslation * rearGearTranslationFactor));

    // rear swingarms
    static UINT RearSwingarmsGrp[] = {GRP_Object11};
    m_rearSwingarms = new MGROUP_ROTATE(0, RearSwingarmsGrp, SizeOfGrp(RearSwingarmsGrp), 
        _V(0.0, 0.242, -3.569), _V(1, 0, 0), rearStrutsRotation * 1.3f);  // must rotate slightly more than gear, plus we must clear the closed aft gear doors

    const float aftDoorRotation = static_cast<float>(90 * RAD);
    // rear gear port doors
    static UINT LWheelAftCoverGrp[] = {GRP_gearflap4_outer, GRP_gearflap_4_inner};
    static MGROUP_ROTATE LWheelAftCover (0, LWheelAftCoverGrp, SizeOfGrp(LWheelAftCoverGrp),
        _V(-4.753, -0.733, -5.090), _V(0.0, 0.007, 1.0), -aftDoorRotation);
    static MGROUP_ROTATE LWheelAftCoverClose (0, LWheelAftCoverGrp, SizeOfGrp(LWheelAftCoverGrp),
        _V(-4.753, -0.733, -5.090), _V(0.0, 0.007, 1.0), aftDoorRotation);
    
    static UINT LWheelForwardCoverGrp[] = {GRP_gearflap_1_outer, GRP_gearflap_1_inner};
    static MGROUP_ROTATE LWheelForwardCover(0, LWheelForwardCoverGrp, SizeOfGrp(LWheelForwardCoverGrp),
        _V(-3.930, -0.591, -3.726), _V(0.0, 0.04, 0.999), aftDoorRotation);

    // rear gear starboard doors
    static UINT RWheelAftCoverGrp[] = {GRP_gearflap3_outer, GRP_gearflap3_inner};
    static MGROUP_ROTATE RWheelAftCover (0, RWheelAftCoverGrp, SizeOfGrp(RWheelAftCoverGrp),
        _V(4.753, -0.733, -5.090), _V(0.0, 0.007, 1.0), aftDoorRotation);
    static MGROUP_ROTATE RWheelAftCoverClose (0, RWheelAftCoverGrp, SizeOfGrp(RWheelAftCoverGrp),
        _V(4.753, -0.733, -5.090), _V(0.0, 0.007, 1.0), -aftDoorRotation);
    
    static UINT RWheelForwardCoverGrp[] = {GRP_gearflap_2outer, GRP_gearflap_2outer_fixup_1, GRP_gearflap_2outer_fixup_2};
    static MGROUP_ROTATE RWheelForwardCover(0, RWheelForwardCoverGrp, SizeOfGrp(RWheelForwardCoverGrp),
        _V(3.930, -0.591, -3.726), _V(0.0, 0.04, 0.999), -aftDoorRotation);

    anim_gear = CreateAnimation(0);
    ANIMATIONCOMPONENT_HANDLE hFrontStrut      = AddAnimationComponent (anim_gear, 0.3, 0.7, &NWheelStrut);
    ANIMATIONCOMPONENT_HANDLE hFrontInnerStrut = AddAnimationComponent (anim_gear, 0.5, 1.0, m_frontInnerStrut, hFrontStrut);
    AddAnimationComponent (anim_gear, 0.3, 0.7, &NWheelFCover);
    AddAnimationComponent (anim_gear, 0, 0.3, &NWheelLCover);
    AddAnimationComponent (anim_gear, 0, 0.3, &NWheelRCover);

    AddAnimationComponent (anim_gear, 0, 0.3, &LWheelAftCover);
    AddAnimationComponent (anim_gear, 0, 0.3, &RWheelAftCover);
    AddAnimationComponent (anim_gear, 0, 0.3, &LWheelForwardCover);
    AddAnimationComponent (anim_gear, 0, 0.3, &RWheelForwardCover);
    ANIMATIONCOMPONENT_HANDLE hAftStruts = AddAnimationComponent (anim_gear, 0.3, 0.7, &AftStruts);

    // close the doors again
    AddAnimationComponent (anim_gear, 0.85, 1.0, &NWheelFCoverClose);  // close back to 90 degrees
    AddAnimationComponent (anim_gear, 0.7, 1.0, &NWheelLCoverClose);
    AddAnimationComponent (anim_gear, 0.7, 1.0, &NWheelRCoverClose);
    AddAnimationComponent (anim_gear, 0.7, 1.0, &LWheelAftCoverClose);
    AddAnimationComponent (anim_gear, 0.7, 1.0, &RWheelAftCoverClose);

    ANIMATIONCOMPONENT_HANDLE hAftInnerStruts = AddAnimationComponent (anim_gear, 0.6, 1.0, m_aftInnerStruts, hAftStruts);

    // rear swingarms (sequence must match gear motion)
    AddAnimationComponent(anim_gear, 0.3, 0.7, m_rearSwingarms, hAftStruts); 

    // Note: wheels are attached during wheel rotation setup below

#if 0  // not until the MK II when Steve rebuilds the gear in the fully-down position
    // **** Landing strut compression ****
    // NOTE: the intial state of the gear before compression is applied is always *fully deployed*.
    // front gear compression
    static MGROUP_TRANSLATE noseCompressionTranslate (0, FrontInnerStrutGrp, SizeOfGrp(FrontInnerStrutGrp), 
        _V(0.0, -GEAR_COMPRESSION_DISTANCE, 0));

    // must slightly open the nose door to clear the wheel
    static MGROUP_ROTATE NWheelFCoverCompressionOpen (0, NWheelFCoverGrp, SizeOfGrp(NWheelFCoverGrp),
        _V(0, -0.992, 6.560), _V(1,0,0), static_cast<float>(20 * RAD));  // Open to 110 degrees

    m_animNoseGearCompression = CreateAnimation(1);  // fully uncompressed when gear is down
    ANIMATIONCOMPONENT_HANDLE hFrontCompressionHandle = AddAnimationComponent(m_animNoseGearCompression, 0, 1, &noseCompressionTranslate);    // parent to inner strut
    AddAnimationComponent(m_animNoseGearCompression, 0, 0.55, &NWheelFCoverCompressionOpen);    

    // rear gear compression; both gear struts always move in sync as a pair
    const double rearStrutTranslationDistance = -GEAR_COMPRESSION_DISTANCE * REAR_GEAR_COMPRESSION_TRANSLATION_FACTOR;
    static MGROUP_TRANSLATE rearCompressionTranslate (0, AftInnerStrutsGrp, SizeOfGrp(AftInnerStrutsGrp), 
        _V(0.0, 0.982 * rearStrutTranslationDistance, 0.191 * rearStrutTranslationDistance));
    m_animRearGearCompression = CreateAnimation(1);  // fully uncompressed when gear is down
    ANIMATIONCOMPONENT_HANDLE hRearCompressionHandle  = AddAnimationComponent(m_animRearGearCompression, 0, 1, &rearCompressionTranslate);
#endif

    // **** Wheel rotation ****

    // TEMP for the Mk I release: set rotation to 720 degrees so they rotate twice as fast since the PreStep expects this.
    // This will be removed for the Mk II release when we add the wheels twice anyway for compression.

    // front wheels
    static UINT FrontWheelsGrp[] = {GRP_Forward_Wheels};
    m_frontWheels = new MGROUP_ROTATE(0, FrontWheelsGrp, SizeOfGrp(FrontWheelsGrp), 
        _V(0.0, -0.5295, 5.369), _V(1, 0, 0), static_cast<float>(720 * RAD));  // TEMP

    // rear wheels
    static UINT RearWheelsGrp[] = {GRP_Rear_Wheel_Port, GRP_Rear_Wheel_Starboard};
    m_rearWheels = new MGROUP_ROTATE(0, RearWheelsGrp, SizeOfGrp(RearWheelsGrp), 
        _V(0.0, 0.068, -4.5375), _V(1, 0, 0), static_cast<float>(720 * RAD));  // TEMP

    // Note: we need to parent the wheels to the struts here so the rotation points are modified as the struts move
    m_animFrontTireRotation = CreateAnimation(0);
    AddAnimationComponent(m_animFrontTireRotation, 0, 1, m_frontWheels, hFrontInnerStrut);

    m_animRearTireRotation = CreateAnimation(0);
    AddAnimationComponent(m_animRearTireRotation, 0, 1, m_rearWheels, hAftInnerStruts);

    // PROBLEM: UNCOMMENTING THE NEXT 2 LINES CAUSES THE COMPRESSION BUG WITH ROTATION
    /*  Root cause of view-switch rotation bug: the mesh originates with the gear UP, and so gear compression relies on the gear deploy animation, which won't work.  
        In order for compression to work, the struts must be able to compress without relying on the main gear animation; e.g., they must be fully deployed in the original mesh.

        To see the bug:
        1. Load up an external view landed at Brighton Beach.
        2. Switch to SH-01 and back.
        3. The wheels will be off-center.

        This has something to do with how the Orbiter core applies the transformations to the copy of the global
        template mesh, but I haven't figured out why.  Bottom line: it works in the XR5 with wheels down
    */
    // Now we need to parent the wheels to the COMPRESSION STRUT so they move with gear compression, too.
    // This line really means, "Apply any transformations for my parent (compression strut) to ME as well."
    // Note: the wheels are in here TWICE, so they rotate twice as fast as they should; however, we account for this in our rotation prestep.
    /* NO COMPRESSION until the MK II
    AddAnimationComponent(m_animFrontTireRotation, 0, 1, m_frontWheels, hFrontCompressionHandle);
    AddAnimationComponent(m_animRearTireRotation,  0, 1, m_rearWheels,  hRearCompressionHandle);
    */

    // **** Nosewheel steering ****
    // Note: for simplicity here, we use a separate animation for this; this is OK since
    /* NO: TOO COMPLEX, AND DOESN'T LOOK NICE ANYWAY
    // we never rotate the nose gear unless it is on the ground.  Technically we should
    // integrate this into the nose gear animation and attach the wheels *here* so there is 
    // only one "animation chain" for the nose gear.  However, that would be pretty complex,
    // and the shortcut we take here is sufficient.
    static MGROUP_ROTATE noseCylinderSteering(0, FrontInnerStrutGrp, SizeOfGrp(FrontInnerStrutGrp), _V(0.0, 0.0, 6.431), _V(0,1,0), static_cast<float>(50*RAD));

    m_animNosewheelSteering = CreateAnimation(0.5);
    ANIMATIONCOMPONENT_HANDLE hNoseCylinderSteering = AddAnimationComponent(m_animNosewheelSteering, 0, 1.0, &noseCylinderSteering);

    // Apply steering transformation to the front inner strut; this next lines really mean, 
    // "Apply any noseCylinderSteeringHandle transformations to hFrontInnerStrut, m_frontWheels,
    // plus all of their children."  
    AddAnimationComponent (m_animNosewheelSteering, 0, 1.0, m_frontInnerStrut, hNoseCylinderSteering);
    AddAnimationComponent (m_animNosewheelSteering, 0, 1.0, m_frontWheels, hNoseCylinderSteering);
    */

    // ***** Retro cover animation *****
    const float retroRotation = static_cast<float>(16 * RAD);
    static UINT RCoverLGrp[] = { GRP_retrocover_port, GRP_retrocover_port_inner, GRP_retro_nozzle2, GRP_retromachinery1 };
    static MGROUP_ROTATE RCoverL (0, RCoverLGrp, SizeOfGrp(RCoverLGrp),
        _V(-4.698, 0.467, -1.08), _V(0, -1.0, 0), retroRotation);

    static UINT RCoverRGrp[] = { GRP_retrocover_starboard, GRP_retrocover_starboard_inner, GRP_retronozzle1, GRP_retromachinery2 };
    static MGROUP_ROTATE RCoverR (0, RCoverRGrp, SizeOfGrp(RCoverRGrp),
        _V(4.698, 0.467, -1.08), _V(0, 1.0, 0), retroRotation);

    anim_rcover = CreateAnimation (0);
    AddAnimationComponent (anim_rcover, 0, 1, &RCoverL);
    AddAnimationComponent (anim_rcover, 0, 1, &RCoverR);

    // ***** Hover Doors animation *****
    const float forwardHoverDoorRotation = static_cast<float>(110*RAD);
    static UINT HoverDoorsFLGrp[] = { GRP_takeoffflap1, GRP_takeoffflap1_inner };     // Forward left door
    static MGROUP_ROTATE HoverDoorsFL (0, HoverDoorsFLGrp, SizeOfGrp(HoverDoorsFLGrp),     
        _V(-0.582, -1.487, 1.649), _V(0,0,1), -forwardHoverDoorRotation);
    
    static UINT HoverDoorsFRGrp[] = { GRP_takeoffflap2, GRP_takeoffflap2_fixup_1 };          // Forward right door
    static MGROUP_ROTATE HoverDoorsFR (0, HoverDoorsFRGrp, SizeOfGrp(HoverDoorsFRGrp),     
        _V(0.582, -1.487, 1.649), _V(0,0,1), forwardHoverDoorRotation);

    static UINT HoverDoorsRearGrp[] = { GRP_aft_takeoff_cover_inside, GRP_aft_takeoff_cover };  // Aft doors (both)
    static MGROUP_ROTATE HoverDoorsRear (0, HoverDoorsRearGrp, SizeOfGrp(HoverDoorsRearGrp),     
        _V(-4.481, -0.756, -6.787), _V(1,0,0), static_cast<float>(-90.0*RAD));

    anim_hoverdoor = CreateAnimation(0);
    AddAnimationComponent(anim_hoverdoor, 0, 1, &HoverDoorsFL);
    AddAnimationComponent(anim_hoverdoor, 0, 1, &HoverDoorsFR);
    AddAnimationComponent(anim_hoverdoor, 0, 1, &HoverDoorsRear);
    
    // ***** SCRAM Doors animation *****
    const float scramDoorsRotation = static_cast<float>(15.475*RAD);
    static UINT ScramDoorsLeftGrp[] = { GRP_Port_scram_door };           // left SCRAM door (port)
    static MGROUP_ROTATE ScramDoorsLeft (0, ScramDoorsLeftGrp, SizeOfGrp(ScramDoorsLeftGrp),     
        _V(-2.191, -0.555, 3.162), _V(-0.78, 0.145, -0.609), -scramDoorsRotation);    

    static UINT ScramDoorsRightGrp[] = { GRP_starboard_scram_door };      // right SCRAM door (starboard)
    static MGROUP_ROTATE ScramDoorsRight (0, ScramDoorsRightGrp, 1,     
        _V(2.191, -0.555, 3.162), _V(0.78, 0.145, -0.609), scramDoorsRotation);    
    
    anim_scramdoor = CreateAnimation(0);
    AddAnimationComponent(anim_scramdoor, 0, 1, &ScramDoorsLeft);
    AddAnimationComponent(anim_scramdoor, 0, 1, &ScramDoorsRight);

    // ***** Nose cone animation *****
    const float noseconeRotation = -static_cast<float>(180*RAD);
    static UINT NConeTRGrp[] = {GRP_starboard_top_petal, GRP_starboard_top_petal_inner};
    static MGROUP_ROTATE NConeTR (0, NConeTRGrp, SizeOfGrp(NConeTRGrp),
        _V(0.813, 0.913, 10.5), _V(0.663, -0.748, 0.0), noseconeRotation);

    static UINT NConeTLGrp[] = {GRP_port_top_petal, GRP_port_top_petal_inner};
    static MGROUP_ROTATE NConeTL (0, NConeTLGrp, SizeOfGrp(NConeTLGrp),
        _V(-0.813, 0.913, 10.5), _V(0.679, 0.734, 0.014), noseconeRotation);

    static UINT NConeBRGrp[] = {GRP_starboard_bottom_petal, GRP_starboard_bottom_petal_inner};
    static MGROUP_ROTATE NConeBR (0, NConeBRGrp, SizeOfGrp(NConeBRGrp),
        _V( 0.841, -0.503, 10.492), _V(-0.9, -0.436, -0.028), noseconeRotation);

    static UINT NConeBLGrp[] = {GRP_port_bottom_petal, GRP_port_bottom_petal_inner};
    static MGROUP_ROTATE NConeBL (0, NConeBLGrp, SizeOfGrp(NConeBLGrp),
        _V(-0.841, -0.503, 10.492), _V(0.9, -0.436, -0.028), -noseconeRotation);
    
    /* no
    static UINT NConeDockGrp[] = {GRP_docking_collar, GRP_docking_collar_grabber};
    static MGROUP_TRANSLATE NConeDock (0, NConeDockGrp, SizeOfGrp(NConeDockGrp), _V(0, 0, 0.200));
    */

    anim_nose = CreateAnimation (0);
    AddAnimationComponent (anim_nose, 0, 1.0, &NConeTR);    // includes the nose piece
    AddAnimationComponent (anim_nose, 0.02, 1.0, &NConeTL);
    AddAnimationComponent (anim_nose, 0.02, 1.0, &NConeBR);
    AddAnimationComponent (anim_nose, 0.02, 1.0, &NConeBL);
    
    // no: AddAnimationComponent (anim_nose, 0.6, 1.0, &NConeDock);  // translate collar forward

    // ***** Outer airlock animation *****
    const double oLockTransDistance = 0.547;  // flush with airlock edge
    static UINT OLockLeftGrp[] = {GRP_port_outerdoor, GRP_port_outerdoor_fixup_1, GRP_port_outerdoor_fixup_2 };
    static MGROUP_TRANSLATE oLockLeft (0, OLockLeftGrp, SizeOfGrp(OLockLeftGrp), _V(-oLockTransDistance, 0, 0));

    static UINT OLockRightGrp[] = {GRP_starboard_outerdoor, GRP_starboard_outerdoor_fixup_1, GRP_starboard_outerdoor_fixup_2 };
    static MGROUP_TRANSLATE oLockRight (0, OLockRightGrp, SizeOfGrp(OLockRightGrp), _V(oLockTransDistance, 0, 0));

    anim_olock = CreateAnimation (0);
    AddAnimationComponent (anim_olock, 0, 1, &oLockLeft);
    AddAnimationComponent (anim_olock, 0, 1, &oLockRight);

    // ***** Inner airlock animation *****
    static UINT ILockGrp[] = { GRP_airlock_innerdoor, GRP_airlock_innerdoor_fixup_1 };
    static MGROUP_ROTATE ILock (0, ILockGrp, SizeOfGrp(ILockGrp),
        _V( 0.721, 0.544, 8.463), _V(1,0,0), static_cast<float>(110*RAD));  // ORG was 85 degrees

    anim_ilock = CreateAnimation(0);
    AddAnimationComponent (anim_ilock, 0, 1, &ILock);

    /* NO LADDER
    // ***** Escape ladder animation *****
    static UINT LadderGrp[2] = {GRP_Ladder1,GRP_Ladder2};
    static MGROUP_TRANSLATE Ladder1 (0, LadderGrp, 2, _V(0,0,1.1));
    static MGROUP_ROTATE Ladder2 (0, LadderGrp, 2,
        _V(0,-1.05,9.85), _V(1,0,0), static_cast<float>(80*RAD));
    anim_ladder = CreateAnimation (0);
    AddAnimationComponent (anim_ladder, 0, 0.5, &Ladder1);
    AddAnimationComponent (anim_ladder, 0.5, 1, &Ladder2);
    */
    
    // ***** Top hatch animation *****
    static UINT HatchGrp[] = {GRP_upperhatchtop, GRP_Upper_hatch_bottom};
    static MGROUP_ROTATE Hatch (0, HatchGrp, SizeOfGrp(HatchGrp),
        _V(0.0, 2.214, 4.124), _V(1,0,0), static_cast<float>(110*RAD));

    static UINT HatchInnerDoorGrp[] = {GRP_top_hatch_inner_door};
    static MGROUP_ROTATE HatchInnerDoor (0, HatchInnerDoorGrp, SizeOfGrp(HatchInnerDoorGrp),
        _V(-0.475, 1.916, 3.228), _V(1,0,0), static_cast<float>(90*RAD));

    anim_hatch = CreateAnimation (0);
    AddAnimationComponent (anim_hatch, 0, 1, &Hatch);
    AddAnimationComponent (anim_hatch, 0, 1, &HatchInnerDoor);

    // ***** Radiator animation *****
    const float radDoorRotation = static_cast<float>(90*RAD);
    const float radRotation = static_cast<float>(45*RAD);
    static UINT LeftRadDoorGrp[] = {GRP_port_radiator_panel, GRP_port_radiator_panel_inner};
    static MGROUP_ROTATE LeftRadDoor (0, LeftRadDoorGrp, SizeOfGrp(LeftRadDoorGrp),
        _V(-0.121, 1.556, -3.797), _V(0.0, 0.155, 0.988), -radDoorRotation);

    static UINT RightRadDoorGrp[] = {GRP_starboard_radiator_panel, GRP_starboard_radiator_panel_inner};
    static MGROUP_ROTATE RightRadDoor (0, RightRadDoorGrp, SizeOfGrp(RightRadDoorGrp),
        _V(0.121, 1.556, -3.797), _V(0.0, 0.155, 0.988), radDoorRotation);

    static UINT LeftRadGrp[] = {GRP_port_rad};
    static MGROUP_ROTATE LeftRad (0, LeftRadGrp, SizeOfGrp(LeftRadGrp),
        _V(-0.133, 1.216, -4.755), _V(-0.012, 0.098, 0.995), -radRotation);

    static UINT RightRadGrp[] = {GRP_starboard_rad};
    static MGROUP_ROTATE RightRad(0, RightRadGrp, SizeOfGrp(RightRadGrp),
        _V(0.133, 1.216, -4.755), _V(0.012, 0.098, 0.995), radRotation);

    anim_radiator = CreateAnimation(0);
    AddAnimationComponent (anim_radiator, 0, 1.0, &LeftRadDoor);
    AddAnimationComponent (anim_radiator, 0, 1.0, &RightRadDoor);
    AddAnimationComponent (anim_radiator, 0.25, 1.0, &LeftRad);
    AddAnimationComponent (anim_radiator, 0.25, 1.0, &RightRad);

    // ***** Rudder animation *****
    const float rudderRotation = static_cast<float>(60*RAD);
    static UINT RRudderGrp[] = {GRP_starboard_rudder};
    static MGROUP_ROTATE RRudder (0, RRudderGrp, SizeOfGrp(RRudderGrp),
        _V(4.021, 1.584, -9.445), _V(0.352, -0.907, 0.233), rudderRotation);

    static UINT LRudderGrp[] = {GRP_port_rudder};
    static MGROUP_ROTATE LRudder (0, LRudderGrp, SizeOfGrp(LRudderGrp),
        _V(-4.021, 1.584, -9.445), _V(-0.352, -0.907, 0.233), rudderRotation);

    anim_rudder = CreateAnimation (0.5);
    AddAnimationComponent (anim_rudder, 0, 1, &RRudder);
    AddAnimationComponent (anim_rudder, 0, 1, &LRudder);

    // ***** Elevator animation *****
    const VECTOR3 &elevatorRotationPoint = _V(0, 0, -9.581);
    static UINT ElevatorGrp[] = {GRP_top_elevators_bottom_starboard, GRP_top_elevators_bottom_starboard_fixup_1, GRP_top_elevators_bottom_starboard_fixup_2, GRP_top_elevators_bottom_starboard_fixup_3, GRP_top_elevators_bottom_starboard_fixup_4,
            GRP_top_elevators_top01_starboard, GRP_bottom_elevators_top_starboard, GRP_bottom_elevators_bottom_starboard, GRP_bottom_elevators_bottom_starboard_fixup_1, GRP_top_elevators_top01_port, GRP_bottom_elevators_bottom_port, GRP_bottom_elevators_bottom_port_fixup_1, GRP_top_elevators_bottom_port, GRP_bottom_elevators_bottom_port01};
    static MGROUP_ROTATE Elevator (0, ElevatorGrp, SizeOfGrp(ElevatorGrp),
        elevatorRotationPoint, _V(1,0,0), static_cast<float>(40*RAD));
    anim_elevator = CreateAnimation (0.5);
    AddAnimationComponent (anim_elevator, 0, 1, &Elevator);

    // ***** Elevator trim animation *****
    static MGROUP_ROTATE ElevatorTrim (0, ElevatorGrp, SizeOfGrp(ElevatorGrp),
        _V(0, 0, -9.581), _V(1,0,0), static_cast<float>(10*RAD));
    anim_elevatortrim = CreateAnimation (0.5);
    AddAnimationComponent (anim_elevatortrim, 0, 1, &ElevatorTrim);

    // ***** Aileron animation *****
    static UINT LAileronGrp[] = {GRP_top_elevators_top01_port, GRP_top_elevators_bottom_port, GRP_bottom_elevators_bottom_port, GRP_bottom_elevators_bottom_port_fixup_1, GRP_bottom_elevators_bottom_port01, 
        GRP_top_elevators_bottom_starboard_fixup_4, /* This is actually *PORT TOP piece */
        GRP_top_elevators_bottom_starboard_fixup_3 /* This is actually the *PORT BOTTOM piece */ };
    static MGROUP_ROTATE LAileron (0, LAileronGrp, SizeOfGrp(LAileronGrp),
        elevatorRotationPoint, _V(1,0,0), static_cast<float>(-20*RAD));
    anim_laileron = CreateAnimation (0.5);
    AddAnimationComponent (anim_laileron, 0, 1, &LAileron);
    
    static UINT RAileronGrp[] = {GRP_top_elevators_top01_starboard, GRP_top_elevators_bottom_starboard, GRP_bottom_elevators_top_starboard, GRP_bottom_elevators_bottom_starboard, GRP_bottom_elevators_bottom_starboard_fixup_1,
                GRP_top_elevators_bottom_starboard_fixup_1, GRP_top_elevators_bottom_starboard_fixup_2 };
    static MGROUP_ROTATE RAileron (0, RAileronGrp, SizeOfGrp(RAileronGrp),
        elevatorRotationPoint, _V(1,0,0), static_cast<float>(20*RAD));
    anim_raileron = CreateAnimation (0.5);
    AddAnimationComponent (anim_raileron, 0, 1, &RAileron);

    // ***** Airbrake animation *****
    static UINT UpperBrakeGrp[] = {GRP_top_elevators_top01_port, GRP_top_elevators_bottom_port, GRP_top_elevators_top01_starboard, GRP_top_elevators_bottom_starboard, 
        GRP_top_elevators_bottom_starboard_fixup_1, GRP_top_elevators_bottom_starboard_fixup_2, GRP_top_elevators_bottom_starboard_fixup_3, GRP_top_elevators_bottom_starboard_fixup_4 };
    static MGROUP_ROTATE UpperBrake (0, UpperBrakeGrp, SizeOfGrp(UpperBrakeGrp),
        elevatorRotationPoint, _V(1,0,0), static_cast<float>(30*RAD));
    static UINT LowerBrakeGrp[] = {GRP_bottom_elevators_bottom_port, GRP_bottom_elevators_bottom_port_fixup_1, GRP_bottom_elevators_bottom_port01, 
            GRP_bottom_elevators_top_starboard, GRP_bottom_elevators_bottom_starboard, GRP_bottom_elevators_bottom_starboard_fixup_1};
    static MGROUP_ROTATE LowerBrake (0, LowerBrakeGrp, SizeOfGrp(LowerBrakeGrp),
        elevatorRotationPoint, _V(1,0,0), static_cast<float>(-30*RAD));
    
    anim_brake = CreateAnimation (0);
    AddAnimationComponent (anim_brake, 0, 1, &UpperBrake);
    AddAnimationComponent (anim_brake, 0, 1, &LowerBrake);

    // ***** Fuel hatch animation (red, on left side) *****
    const float consumablesHatchesRotation = static_cast<float>(110*RAD);
    static UINT FuelHatchGrp[] = {GRP_propellant_Flap, GRP_propellant_Flap_inner};
    static MGROUP_ROTATE FuelHatch (0, FuelHatchGrp, SizeOfGrp(FuelHatchGrp),
        _V(-0.876, 1.41, -2.781), _V(0.0, -0.103, -0.995), consumablesHatchesRotation);

    anim_fuelhatch = CreateAnimation (0);
    AddAnimationComponent (anim_fuelhatch, 0, 1, &FuelHatch);
    
    // ***** LOX hatch animation (blue, on right side) *****
    static UINT LOXHatchGrp[] = {GRP_LOX_flap, GRP_LOX_flap_inner};
    static MGROUP_ROTATE LOXHatch (0, LOXHatchGrp, SizeOfGrp(LOXHatchGrp),
        _V(0.876, 1.41, -2.781), _V(0.0, -0.103, -0.995), -consumablesHatchesRotation);

    anim_loxhatch = CreateAnimation (0);
    AddAnimationComponent (anim_loxhatch, 0, 1, &LOXHatch);

    // ***** Payload Bay Doors *****
    // Note: the middle and rear doors rotate slightly more than the forward doors
    const float bayDoorsRotation = static_cast<float>(170*RAD);  // max rotation (forward doors only)
    const float staggerRotation = static_cast<float>(4.0*RAD);
    static UINT PortBayGrp1[] = {GRP_portfore_pbd_top, GRP_portfore_pbd_bottom};
    static UINT PortBayGrp2[] = {GRP_portmid_pbd_top, GRP_portmid_pbd_bottom};
    static UINT PortBayGrp3[] = {GRP_portaft_pbd_top, GRP_portaft_pbd_bottom};
    static MGROUP_ROTATE PortBayDoors1 (0, PortBayGrp1, SizeOfGrp(PortBayGrp1),
        _V(-1.726, 1.258, 1.483), _V(0.0, 0.007, 1.0), bayDoorsRotation);
    static MGROUP_ROTATE PortBayDoors2 (0, PortBayGrp2, SizeOfGrp(PortBayGrp2),
        _V(-1.726, 1.166, -0.503), _V(0.0, 0.046, 0.999), bayDoorsRotation - staggerRotation);
    static MGROUP_ROTATE PortBayDoors3 (0, PortBayGrp3, SizeOfGrp(PortBayGrp3),
        _V(-1.726, 1.057, -1.926), _V(0.0, 0.076, 0.997), bayDoorsRotation - (staggerRotation*2));

    static UINT StarboardBayGrp1[] = {GRP_starboardfore_pbd_top, GRP_starboardfore_pbd_bottom};
    static UINT StarboardBayGrp2[] = {GRP_starboardmid_pbd_top, GRP_starboardmid_pbd_bottom};
    static UINT StarboardBayGrp3[] = {GRP_starboardaft_pbd_top, GRP_starboardaft_pbd_bottom};
    static MGROUP_ROTATE StarboardBayDoors1(0, StarboardBayGrp1, SizeOfGrp(StarboardBayGrp1),
        _V(1.726, 1.258, 1.483), _V(0.0, 0.007, 1.0), -bayDoorsRotation);
    static MGROUP_ROTATE StarboardBayDoors2(0, StarboardBayGrp2, SizeOfGrp(StarboardBayGrp2),
        _V(1.726, 1.166, -0.503), _V(0.0, 0.046, 0.999), -bayDoorsRotation + staggerRotation);
    static MGROUP_ROTATE StarboardBayDoors3(0, StarboardBayGrp3, SizeOfGrp(StarboardBayGrp3),
        _V(1.726, 1.057, -1.926), _V(0.0, 0.076, 0.997), -bayDoorsRotation + (staggerRotation*2));

    anim_bay = CreateAnimation(0);
    AddAnimationComponent(anim_bay, 0, 1, &PortBayDoors1);
    AddAnimationComponent(anim_bay, 0, 1, &PortBayDoors2);
    AddAnimationComponent(anim_bay, 0, 1, &PortBayDoors3);
    AddAnimationComponent(anim_bay, 0, 1, &StarboardBayDoors1);
    AddAnimationComponent(anim_bay, 0, 1, &StarboardBayDoors2);
    AddAnimationComponent(anim_bay, 0, 1, &StarboardBayDoors3);
}

// delete any child animation objects; invoked by our destructor
void XR2Ravenstar::CleanUpAnimations()
{
    delete m_frontWheels;
    delete m_rearWheels;
    delete m_rearSwingarms;
    delete m_aftInnerStruts;
    delete m_frontInnerStrut;
}
