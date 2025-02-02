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
// Animations.cpp
// Contains animation code for the DeltaGliderXR1
// ==============================================================

#include "DeltaGliderXR1.h"
#include "meshres.h"

// Virtual Gateway method that decides which animations are valid for this vessel; if the incoming animation handle is valid, 
// the call is propogated up to SetAnimation.  Otherwise, this method returns without changing the animation state.
// [This check is necessary because if we call SetAnimation with an invalid handle (e.g., 0) the Orbiter core animates the wrong groups or crashes.]
#define ALLOW(handlePtr)  if (&anim == &(handlePtr)) { SetAnimation(anim, state); return; }
void DeltaGliderXR1::SetXRAnimation(const UINT &anim, const double state) const
{
    ALLOW(anim_gear);         // handle for landing gear animation
	ALLOW(anim_rcover);       // handle for retro cover animation
    ALLOW(anim_hoverdoor);    // handle for hover doors animation
    ALLOW(anim_scramdoor);    // handle for scram doors animation
	ALLOW(anim_nose);         // handle for nose cone animation
	ALLOW(anim_ladder);       // handle for front escape ladder animation
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

	ALLOW(anim_mainthrottle[0]);   // VC main/retro throttle levers (left and right)
    ALLOW(anim_mainthrottle[1]);   // VC main/retro throttle levers (left and right)
	ALLOW(anim_hoverthrottle);     // VC hover throttle
	ALLOW(anim_scramthrottle[0]);  // VC scram throttle levers (left and right)
    ALLOW(anim_scramthrottle[1]);  // VC scram throttle levers (left and right)
	ALLOW(anim_gearlever);         // VC gear lever
	ALLOW(anim_nconelever);        // VC nose cone lever
	ALLOW(anim_pmaingimbal[0]);    // VC main engine pitch gimbal switch (left and right engine)
    ALLOW(anim_pmaingimbal[1]);    // VC main engine pitch gimbal switch (left and right engine)
	ALLOW(anim_ymaingimbal[0]);    // VC main engine yaw gimbal switch (left and right engine)
    ALLOW(anim_ymaingimbal[1]);    // VC main engine yaw gimbal switch (left and right engine)
	ALLOW(anim_scramgimbal[0]);    // VC scram engine pitch gimbal switch (left and right engine)
    ALLOW(anim_scramgimbal[1]);    // VC scram engine pitch gimbal switch (left and right engine)
	ALLOW(anim_hbalance);          // VC hover balance switch
	ALLOW(anim_hudintens);         // VC HUD intensity switch
	ALLOW(anim_rcsdial);           // VC RCS dial animation
	ALLOW(anim_afdial);            // VC AF dial animation
	ALLOW(anim_olockswitch);       // VC outer airlock switch animation
	ALLOW(anim_ilockswitch);       // VC inner airlock switch animation
	ALLOW(anim_retroswitch);       // VC retro cover switch animation
	ALLOW(anim_ladderswitch);      // VC ladder switch animation
	ALLOW(anim_hatchswitch);       // VC hatch switch animation
	ALLOW(anim_radiatorswitch);    // VC radiator switch animation
}

// --------------------------------------------------------------
// Define animation sequences for moving parts
// Invoked by our constructor.
// --------------------------------------------------------------
void DeltaGliderXR1::DefineAnimations()
{
    // ***** Landing gear animation *****
    static UINT NWheelStrutGrp[2] = {GRP_NWheelStrut1,GRP_NWheelStrut2};
    static MGROUP_ROTATE NWheelStrut (0, NWheelStrutGrp, 2,
        _V(0,-1.048,8.561), _V(1,0,0), static_cast<float>(-95*RAD));
    static UINT NWheelFCoverGrp[2] = {GRP_NWheelFCover1,GRP_NWheelFCover2};
    static MGROUP_ROTATE NWheelFCover (0, NWheelFCoverGrp, 2,
        _V(0,-1.145,8.65), _V(1,0,0), static_cast<float>(-90*RAD));
    static UINT NWheelLCoverGrp[2] = {GRP_NWheelLCover1,GRP_NWheelLCover2};
    static MGROUP_ROTATE NWheelLCover1 (0, NWheelLCoverGrp, 2,
        _V(-0.3,-1.222,7.029), _V(0,0.052,0.999), static_cast<float>(-90*RAD));
    static MGROUP_ROTATE NWheelLCover2 (0, NWheelLCoverGrp, 2,
        _V(-0.3,-1.222,7.029), _V(0,0.052,0.999), static_cast<float>( 90*RAD));
    static UINT NWheelRCoverGrp[2] = {GRP_NWheelRCover1,GRP_NWheelRCover2};
    static MGROUP_ROTATE NWheelRCover1 (0, NWheelRCoverGrp, 2,
        _V( 0.3,-1.222,7.029), _V(0,0.052,0.999), static_cast<float>( 90*RAD));
    static MGROUP_ROTATE NWheelRCover2 (0, NWheelRCoverGrp, 2,
        _V( 0.3,-1.222,7.029), _V(0,0.052,0.999), static_cast<float>(-90*RAD));
    static UINT LWheelStrutGrp[2] = {GRP_LWheelStrut1,GRP_LWheelStrut2};
    static MGROUP_ROTATE LWheelStrut (0, LWheelStrutGrp, 2,
        _V(-3.607,-1.137,-3.08), _V(0,0,1), static_cast<float>(-90*RAD));
    static UINT RWheelStrutGrp[2] = {GRP_RWheelStrut1,GRP_RWheelStrut2};
    static MGROUP_ROTATE RWheelStrut (0, RWheelStrutGrp, 2,
        _V( 3.607,-1.137,-3.08), _V(0,0,1), static_cast<float>(90*RAD));
    static UINT LWheelOCoverGrp[4] = {GRP_LWheelOCover1,GRP_LWheelOCover2,GRP_LWheelOCover3,GRP_LWheelOCover4};
    static MGROUP_ROTATE LWheelOCover (0, LWheelOCoverGrp, 4,
        _V(-3.658,-1.239,-3.038), _V(0,0,1), static_cast<float>(-110*RAD));
    static UINT LWheelICoverGrp[2] = {GRP_LWheelICover1,GRP_LWheelICover2};
    static MGROUP_ROTATE LWheelICover1 (0, LWheelICoverGrp, 2,
        _V(-2.175,-1.178,-3.438), _V(0,0,1), static_cast<float>(90*RAD));
    static MGROUP_ROTATE LWheelICover2 (0, LWheelICoverGrp, 2,
        _V(-2.175,-1.178,-3.438), _V(0,0,1), static_cast<float>(-90*RAD));
    static UINT RWheelOCoverGrp[4] = {GRP_RWheelOCover1,GRP_RWheelOCover2,GRP_RWheelOCover3,GRP_RWheelOCover4};
    static MGROUP_ROTATE RWheelOCover (0, RWheelOCoverGrp, 4,
        _V( 3.658,-1.239,-3.038), _V(0,0,1), static_cast<float>( 110*RAD));
    static UINT RWheelICoverGrp[2] = {GRP_RWheelICover1,GRP_RWheelICover2};
    static MGROUP_ROTATE RWheelICover1 (0, RWheelICoverGrp, 2,
        _V( 2.175,-1.178,-3.438), _V(0,0,1), static_cast<float>(-90*RAD));
    static MGROUP_ROTATE RWheelICover2 (0, RWheelICoverGrp, 2,
        _V( 2.175,-1.178,-3.438), _V(0,0,1), static_cast<float>( 90*RAD));
    anim_gear = CreateAnimation (1);
    AddAnimationComponent (anim_gear, 0.3, 1, &NWheelStrut);
    AddAnimationComponent (anim_gear, 0.3, 0.9, &NWheelFCover);
    AddAnimationComponent (anim_gear, 0, 0.3, &NWheelLCover1);
    AddAnimationComponent (anim_gear, 0.7, 1.0, &NWheelLCover2);
    AddAnimationComponent (anim_gear, 0, 0.3, &NWheelRCover1);
    AddAnimationComponent (anim_gear, 0.7, 1.0, &NWheelRCover2);
    AddAnimationComponent (anim_gear, 0, 1, &LWheelStrut);
    AddAnimationComponent (anim_gear, 0, 1, &RWheelStrut);
    AddAnimationComponent (anim_gear, 0, 1, &LWheelOCover);
    AddAnimationComponent (anim_gear, 0, 0.3, &LWheelICover1);
    AddAnimationComponent (anim_gear, 0.7, 1, &LWheelICover2);
    AddAnimationComponent (anim_gear, 0, 1, &RWheelOCover);
    AddAnimationComponent (anim_gear, 0, 0.3, &RWheelICover1);
    AddAnimationComponent (anim_gear, 0.7, 1, &RWheelICover2);
    
    // ***** Retro cover animation *****
    static UINT RCoverTLGrp[2] = {GRP_RCoverTL1,GRP_RCoverTL2};
    static MGROUP_ROTATE RCoverTL (0, RCoverTLGrp, 2,
        _V(-2.156,-0.49,6.886), _V(-0.423,0.23,-0.877), static_cast<float>( 70*RAD));
    static UINT RCoverBLGrp[2] = {GRP_RCoverBL1,GRP_RCoverBL2};
    static MGROUP_ROTATE RCoverBL (0, RCoverBLGrp, 2,
        _V(-2.156,-0.49,6.886), _V(-0.434,-0.037,-0.9), static_cast<float>(-70*RAD));
    static UINT RCoverTRGrp[2] = {GRP_RCoverTR1,GRP_RCoverTR2};
    static MGROUP_ROTATE RCoverTR (0, RCoverTRGrp, 2,
        _V( 2.156,-0.49,6.886), _V( 0.423,0.23,-0.877), static_cast<float>(-70*RAD));
    static UINT RCoverBRGrp[2] = {GRP_RCoverBR1,GRP_RCoverBR2};
    static MGROUP_ROTATE RCoverBR (0, RCoverBRGrp, 2,
        _V( 2.156,-0.49,6.886), _V( 0.434,-0.037,-0.9), static_cast<float>( 70*RAD));
    anim_rcover = CreateAnimation (0);
    AddAnimationComponent (anim_rcover, 0, 1, &RCoverTL);
    AddAnimationComponent (anim_rcover, 0, 1, &RCoverBL);
    AddAnimationComponent (anim_rcover, 0, 1, &RCoverTR);
    AddAnimationComponent (anim_rcover, 0, 1, &RCoverBR);

    // ***** Hover Doors animation *****
    const float hoverDoorRotation = static_cast<float>(180*RAD);
    static UINT HoverDoorsFLGrp[1] = { GRP_frhovleft };           // Forward left door
    static MGROUP_ROTATE HoverDoorsFL (0, HoverDoorsFLGrp, 1,     
        _V(-0.60,-1.62,2.96), _V(0,0,1), -hoverDoorRotation);
    
    static UINT HoverDoorsFRGrp[1] = { GRP_frhovright };          // Forward right door
    static MGROUP_ROTATE HoverDoorsFR (0, HoverDoorsFRGrp, 1,     
        _V(0.60,-1.62,2.96), _V(0,0,1), hoverDoorRotation);

    static UINT HoverDoorsPLGrp[1] = { GRP_Lefthovleft };         // Port left door
    static MGROUP_ROTATE HoverDoorsPL (0, HoverDoorsPLGrp, 1,     
        _V(-3.57,-1.25,-4.75), _V(0,0,1), -hoverDoorRotation);

    static UINT HoverDoorsPRGrp[1] = { GRP_Lefthovright };        // Port right door
    static MGROUP_ROTATE HoverDoorsPR (0, HoverDoorsPRGrp, 1,     
        _V(-2.42,-1.25,-4.75), _V(0,0,1), hoverDoorRotation);

    static UINT HoverDoorsSLGrp[1] = { GRP_Righthovleft };        // Starboard left door
    static MGROUP_ROTATE HoverDoorsSL (0, HoverDoorsSLGrp, 1,     
        _V(2.42,-1.25,-4.75), _V(0,0,1), -hoverDoorRotation);

    static UINT HoverDoorsSRGrp[1] = { GRP_Righthovright };       // Starboard right door
    static MGROUP_ROTATE HoverDoorsSR (0, HoverDoorsSRGrp, 1,     
        _V(3.57,-1.25,-4.75), _V(0,0,1), hoverDoorRotation);

    anim_hoverdoor = CreateAnimation(0);
    AddAnimationComponent(anim_hoverdoor, 0, 1, &HoverDoorsFL);
    AddAnimationComponent(anim_hoverdoor, 0, 1, &HoverDoorsFR);
    AddAnimationComponent(anim_hoverdoor, 0, 1, &HoverDoorsPL);
    AddAnimationComponent(anim_hoverdoor, 0, 1, &HoverDoorsPR);
    AddAnimationComponent(anim_hoverdoor, 0, 1, &HoverDoorsSL);
    AddAnimationComponent(anim_hoverdoor, 0, 1, &HoverDoorsSR);
    
    // ***** SCRAM Doors animation *****
    static UINT ScramDoorsGrp[1] = { GRP_scramclose };           // single SCRAM door mesh
    static MGROUP_ROTATE ScramDoors (0, ScramDoorsGrp, 1,     
        _V(0,-0.87,2.95), _V(-1,0,0), static_cast<float>(-95*RAD));    // _V(-1...) here makes the doors rotate toward the FRONT of the ship when they close instead of the back [_V(1...)]
    
    anim_scramdoor = CreateAnimation(0);
    AddAnimationComponent(anim_scramdoor, 0, 1, &ScramDoors);

    // ***** Nose cone animation *****
    static UINT NConeTLGrp[2] = {GRP_NConeTL1,GRP_NConeTL2};
    static MGROUP_ROTATE NConeTL (0, NConeTLGrp, 2,
        _V(-0.424,-0.066,9.838), _V(-0.707,-0.707,0), static_cast<float>(150*RAD));
    static UINT NConeTRGrp[2] = {GRP_NConeTR1,GRP_NConeTR2};
    static MGROUP_ROTATE NConeTR (0, NConeTRGrp, 2,
        _V( 0.424,-0.066,9.838), _V(-0.707, 0.707,0), static_cast<float>(150*RAD));
    static UINT NConeBLGrp[2] = {GRP_NConeBL1,GRP_NConeBL2};
    static MGROUP_ROTATE NConeBL (0, NConeBLGrp, 2,
        _V(-0.424,-0.914,9.838), _V( 0.707,-0.707,0), static_cast<float>(150*RAD));
    static UINT NConeBRGrp[2] = {GRP_NConeBR1,GRP_NConeBR2};
    static MGROUP_ROTATE NConeBR (0, NConeBRGrp, 2,
        _V( 0.424,-0.914,9.838), _V( 0.707, 0.707,0), static_cast<float>(150*RAD));
    static UINT NConeDockGrp[1] = {GRP_NConeDock};
    static MGROUP_TRANSLATE NConeDock (0, NConeDockGrp, 1, _V(0,0,0.06));
    // virtual cockpit mesh animation (nose cone visible from cockpit)
    static UINT VCNConeTLGrp[1] = {106};
    static MGROUP_ROTATE VCNConeTL (1, VCNConeTLGrp, 1,
        _V(-0.424,-0.066,9.838), _V(-0.707,-0.707,0), static_cast<float>(150*RAD));
    static UINT VCNConeTRGrp[1] = {107};
    static MGROUP_ROTATE VCNConeTR (1, VCNConeTRGrp, 1,
        _V( 0.424,-0.066,9.838), _V(-0.707, 0.707,0), static_cast<float>(150*RAD));
    anim_nose = CreateAnimation (0);
    AddAnimationComponent (anim_nose, 0.01, 0.92, &NConeTL);
    AddAnimationComponent (anim_nose, 0.01, 0.92, &VCNConeTL);
    AddAnimationComponent (anim_nose, 0.03, 0.91, &NConeTR);
    AddAnimationComponent (anim_nose, 0.03, 0.91, &VCNConeTR);
    AddAnimationComponent (anim_nose, 0, 0.89, &NConeBL);
    AddAnimationComponent (anim_nose, 0.03, 0.94, &NConeBR);
    AddAnimationComponent (anim_nose, 0.8, 1, &NConeDock);
    
    // ***** Outer airlock animation *****
    static UINT OLockGrp[2] = {GRP_OLock1,GRP_OLock2};
    static MGROUP_ROTATE OLock (0, OLockGrp, 2,
        _V(0,-0.080,9.851), _V(1,0,0), static_cast<float>(110*RAD));
    static UINT VCOLockGrp[1] = {13};
    static MGROUP_ROTATE VCOLock (1, VCOLockGrp, 1,
        _V(0,-0.080,9.851), _V(1,0,0), static_cast<float>(110*RAD));
    anim_olock = CreateAnimation (0);
    AddAnimationComponent (anim_olock, 0, 1, &OLock);
    AddAnimationComponent (anim_olock, 0, 1, &VCOLock);
    
    // ***** Inner airlock animation *****
    static UINT ILockGrp[2] = {GRP_ILock1,GRP_ILock2};
    static MGROUP_ROTATE ILock (0, ILockGrp, 2,
        _V(0,-0.573,7.800), _V(1,0,0), static_cast<float>(85*RAD));
    // virtual cockpit mesh animation (inner airlock visible from cockpit)
    static UINT VCILockGrp[4] = {10,28,11,127};
    static MGROUP_ROTATE VCILock (1, VCILockGrp, 4,
        _V(0,-0.573,7.800), _V(1,0,0), static_cast<float>(85*RAD));
    anim_ilock = CreateAnimation (0);
    AddAnimationComponent (anim_ilock, 0, 1, &ILock);
    AddAnimationComponent (anim_ilock, 0, 1, &VCILock);
    
    // ***** Escape ladder animation *****
    static UINT LadderGrp[2] = {GRP_Ladder1,GRP_Ladder2};
    static MGROUP_TRANSLATE Ladder1 (0, LadderGrp, 2, _V(0,0,1.1));
    static MGROUP_ROTATE Ladder2 (0, LadderGrp, 2,
        _V(0,-1.05,9.85), _V(1,0,0), static_cast<float>(80*RAD));
    anim_ladder = CreateAnimation (0);
    AddAnimationComponent (anim_ladder, 0, 0.5, &Ladder1);
    AddAnimationComponent (anim_ladder, 0.5, 1, &Ladder2);
    
    // ***** Top hatch animation *****
    static UINT HatchGrp[2] = {GRP_Hatch1,GRP_Hatch2};
    static MGROUP_ROTATE Hatch (0, HatchGrp, 2,
        _V(0,2.069,5.038), _V(1,0,0), static_cast<float>(110*RAD));
    static UINT VCHatchGrp[1] = {14};
    static MGROUP_ROTATE VCHatch (1, VCHatchGrp, 1,
        _V(0,2.069,5.038), _V(1,0,0), static_cast<float>(110*RAD));
    static UINT RearLadderGrp[2] = {GRP_RearLadder1,GRP_RearLadder2};
    static MGROUP_ROTATE RearLadder1 (0, RearLadderGrp, 2,
        _V(0,1.7621,4.0959), _V(1,0,0), static_cast<float>(-20*RAD));
    static MGROUP_ROTATE RearLadder2 (0, RearLadderGrp+1, 1,
        _V(0,1.1173,4.1894), _V(1,0,0), static_cast<float>(180*RAD));
    // virtual cockpit ladder animation
    static UINT VCRearLadderGrp[2] = {29,30};
    static MGROUP_ROTATE VCRearLadder1 (1, VCRearLadderGrp, 2,
        _V(0,1.7621,4.0959), _V(1,0,0), static_cast<float>(-20*RAD));
    static MGROUP_ROTATE VCRearLadder2 (1, VCRearLadderGrp+1, 1,
        _V(0,1.1173,4.1894), _V(1,0,0), static_cast<float>(180*RAD));
    anim_hatch = CreateAnimation (0);
    AddAnimationComponent (anim_hatch, 0, 1, &Hatch);
    AddAnimationComponent (anim_hatch, 0, 1, &VCHatch);
    AddAnimationComponent (anim_hatch, 0, 0.25, &RearLadder1);
    AddAnimationComponent (anim_hatch, 0.25, 0.8, &RearLadder2);
    AddAnimationComponent (anim_hatch, 0, 0.25, &VCRearLadder1);
    AddAnimationComponent (anim_hatch, 0.25, 0.8, &VCRearLadder2);
    
    // ***** Radiator animation *****
    static UINT RaddoorGrp[2] = {GRP_Raddoor1,GRP_Raddoor2};
    static MGROUP_ROTATE Raddoor (0, RaddoorGrp, 2,
        _V(0,1.481,-3.986), _V(1,0,0), static_cast<float>(170*RAD));
    static UINT RadiatorGrp[3] = {GRP_Radiator1,GRP_Radiator2,GRP_Radiator3};
    static MGROUP_TRANSLATE Radiator (0, RadiatorGrp, 3,
        _V(0,0.584,-0.157));
    static UINT LRadiatorGrp[1] = {GRP_Radiator1};
    static MGROUP_ROTATE LRadiator (0, LRadiatorGrp, 1,
        _V(-0.88,1.94,-4.211), _V(0,0.260,0.966), static_cast<float>(135*RAD));
    static UINT RRadiatorGrp[1] = {GRP_Radiator2};
    static MGROUP_ROTATE RRadiator (0, RRadiatorGrp, 1,
        _V(0.93,1.91,-4.211), _V(0,0.260,0.966), static_cast<float>(-135*RAD));
    anim_radiator = CreateAnimation (0);
    AddAnimationComponent (anim_radiator, 0, 0.33, &Raddoor);
    AddAnimationComponent (anim_radiator, 0.25, 0.5, &Radiator);
    AddAnimationComponent (anim_radiator, 0.5, 0.75, &RRadiator);
    AddAnimationComponent (anim_radiator, 0.75, 1, &LRadiator);
    
    // ***** Rudder animation *****
    static UINT RRudderGrp[2] = {GRP_RRudder1,GRP_RRudder2};
    static MGROUP_ROTATE RRudder (0, RRudderGrp, 2,
        _V( 8.668,0.958,-6.204), _V( 0.143,0.975,-0.172), static_cast<float>(-60*RAD));
    static UINT LRudderGrp[2] = {GRP_LRudder1,GRP_LRudder2};
    static MGROUP_ROTATE LRudder (0, LRudderGrp, 2,
        _V(-8.668,0.958,-6.204), _V(-0.143,0.975,-0.172), static_cast<float>(-60*RAD));
    anim_rudder = CreateAnimation (0.5);
    AddAnimationComponent (anim_rudder, 0, 1, &RRudder);
    AddAnimationComponent (anim_rudder, 0, 1, &LRudder);
    
    // ***** Elevator animation *****
    static UINT ElevatorGrp[8] = {29,30,35,36,51,52,54,55};
    static MGROUP_ROTATE Elevator (0, ElevatorGrp, 8,
        _V(0,-0.4,-6.0), _V(1,0,0), static_cast<float>(40*RAD));
    anim_elevator = CreateAnimation (0.5);
    AddAnimationComponent (anim_elevator, 0, 1, &Elevator);
    
    // ***** Elevator trim animation *****
    static MGROUP_ROTATE ElevatorTrim (0, ElevatorGrp, 8,
        _V(0,-0.4,-6.0), _V(1,0,0), static_cast<float>(10*RAD));
    anim_elevatortrim = CreateAnimation (0.5);
    AddAnimationComponent (anim_elevatortrim, 0, 1, &ElevatorTrim);
    
    // ***** Aileron animation *****
    static UINT LAileronGrp[4] = {29,30,51,52};
    static MGROUP_ROTATE LAileron (0, LAileronGrp, 4,
        _V(0,-0.4,-6.0), _V(1,0,0), static_cast<float>(-20*RAD));
    anim_laileron = CreateAnimation (0.5);
    AddAnimationComponent (anim_laileron, 0, 1, &LAileron);
    
    static UINT RAileronGrp[4] = {35,36,54,55};
    static MGROUP_ROTATE RAileron (0, RAileronGrp, 4,
        _V(0,-0.4,-6.0), _V(1,0,0), static_cast<float>(20*RAD));
    anim_raileron = CreateAnimation (0.5);
    AddAnimationComponent (anim_raileron, 0, 1, &RAileron);
    
    // ***** Airbrake animation *****
    static UINT UpperBrakeGrp[4] = {35,30,52,55};
    static MGROUP_ROTATE UpperBrake (0, UpperBrakeGrp, 4,
        _V(0,-0.4,-6.0), _V(1,0,0), static_cast<float>(30*RAD));
    static UINT LowerBrakeGrp[4] = {29,36,51,54};
    static MGROUP_ROTATE LowerBrake (0, LowerBrakeGrp, 4,
        _V(0,-0.4,-6.0), _V(1,0,0), static_cast<float>(-30*RAD));
    static MGROUP_ROTATE RRudderBrake (0, RRudderGrp, 2,
        _V( 8.668,0.958,-6.204), _V( 0.143,0.975,-0.172), static_cast<float>( 25*RAD));
    static MGROUP_ROTATE LRudderBrake (0, LRudderGrp, 2,
        _V(-8.668,0.958,-6.204), _V(-0.143,0.975,-0.172), static_cast<float>(-25*RAD));
    
    anim_brake = CreateAnimation (0);
    AddAnimationComponent (anim_brake, 0, 1, &UpperBrake);
    AddAnimationComponent (anim_brake, 0, 1, &LowerBrake);
    AddAnimationComponent (anim_brake, 0, 1, &RRudderBrake);
    AddAnimationComponent (anim_brake, 0, 1, &LRudderBrake);
    
    // ======================================================
    // VC animation definitions
    // ======================================================
    
    static UINT MainThrottleLGrp[2] = {32,53};
    static MGROUP_ROTATE MainThrottleL (1, MainThrottleLGrp, 2,
        _V(0,0.72,6.9856), _V(1,0,0), static_cast<float>(50*RAD));
    anim_mainthrottle[0] = CreateAnimation (0.4);
    AddAnimationComponent (anim_mainthrottle[0], 0, 1, &MainThrottleL);
    
    static UINT MainThrottleRGrp[2] = {37,54};
    static MGROUP_ROTATE MainThrottleR (1, MainThrottleRGrp, 2,
        _V(0,0.72,6.9856), _V(1,0,0), static_cast<float>(50*RAD));
    anim_mainthrottle[1] = CreateAnimation (0.4);
    AddAnimationComponent (anim_mainthrottle[1], 0, 1, &MainThrottleR);
    
    static UINT HoverThrottleGrp[2] = {38,60};
    static MGROUP_ROTATE HoverThrottle (1, HoverThrottleGrp, 2,
        _V(-0.41,0.8222,6.9226), _V(1,0,0), static_cast<float>(50*RAD));
    anim_hoverthrottle = CreateAnimation (0);
    AddAnimationComponent (anim_hoverthrottle, 0, 1, &HoverThrottle);
    
    static UINT ScramThrottleLGrp[2] = {39,61};
    static MGROUP_ROTATE ScramThrottleL (1, ScramThrottleLGrp, 2,
        _V(0,0.7849,6.96), _V(1,0,0), static_cast<float>(30*RAD));
    anim_scramthrottle[0] =  CreateAnimation (0);
    AddAnimationComponent (anim_scramthrottle[0], 0, 1, &ScramThrottleL);
    
    static UINT ScramThrottleRGrp[2] = {40,62};
    static MGROUP_ROTATE ScramThrottleR (1, ScramThrottleRGrp, 2,
        _V(0,0.7849,6.96), _V(1,0,0), static_cast<float>(30*RAD));
    anim_scramthrottle[1] =  CreateAnimation (0);
    AddAnimationComponent (anim_scramthrottle[1], 0, 1, &ScramThrottleR);
    
    static UINT GearLeverGrp[2] = {42,63};
    static MGROUP_ROTATE GearLever (1, GearLeverGrp, 2,
        _V(0.3314,0.9542,7.1764), _V(-0.7590,-0.231,0.6087), static_cast<float>(110*RAD));
    anim_gearlever = CreateAnimation (1);
    AddAnimationComponent (anim_gearlever, 0, 1, &GearLever);
    
    static UINT NoseconeLeverGrp[2] = {43,64};
    static MGROUP_ROTATE NoseconeLever (1, NoseconeLeverGrp, 2,
        _V(0.35,1.0594,7.1995), _V(-0.7590,-0.231,0.6087), static_cast<float>(110*RAD));
    anim_nconelever = CreateAnimation (0);
    AddAnimationComponent (anim_nconelever, 0, 1, &NoseconeLever);
    
    static UINT ScramGimbalLGrp = 69;
    static MGROUP_ROTATE ScramGimbalL (1, &ScramGimbalLGrp, 1,
        _V(-0.2620,1.0515,7.2433), _V(0.9439,-0.0828,0.3197), static_cast<float>(31*RAD));
    anim_scramgimbal[0] = CreateAnimation (0.5);
    AddAnimationComponent (anim_scramgimbal[0], 0, 1, &ScramGimbalL);
    
    static UINT ScramGimbalRGrp = 70;
    static MGROUP_ROTATE ScramGimbalR (1, &ScramGimbalRGrp, 1,
        _V(-0.2501,1.0504,7.2474), _V(0.9439,-0.0828,0.3197), static_cast<float>(31*RAD));
    anim_scramgimbal[1] = CreateAnimation (0.5);
    AddAnimationComponent (anim_scramgimbal[1], 0, 1, &ScramGimbalR);
    
    static UINT PMainGimbalLGrp = 72;
    static MGROUP_ROTATE PMainGimbalL (1, &PMainGimbalLGrp, 1,
        _V(-0.3682,1.0986,7.1452), _V(0.7139,-0.1231,0.6893), static_cast<float>(31*RAD));
    anim_pmaingimbal[0] = CreateAnimation (0.5);
    AddAnimationComponent (anim_pmaingimbal[0], 0, 1, &PMainGimbalL);
    
    static UINT PMainGimbalRGrp = 73;
    static MGROUP_ROTATE PMainGimbalR (1, &PMainGimbalRGrp, 1,
        _V(-0.3587,1.0970,7.1543), _V(0.7139,-0.1231,0.6893), static_cast<float>(31*RAD));
    anim_pmaingimbal[1] = CreateAnimation (0.5);
    AddAnimationComponent (anim_pmaingimbal[1], 0, 1, &PMainGimbalR);
    
    static UINT YMainGimbalLGrp = 74;
    static MGROUP_ROTATE YMainGimbalL (1, &YMainGimbalLGrp, 1,
        _V(-0.3638,1.0479,7.1364), _V(-0.0423,0.9733,0.2257), static_cast<float>(31*RAD));
    anim_ymaingimbal[0] = CreateAnimation (0.5);
    AddAnimationComponent (anim_ymaingimbal[0], 0, 1, &YMainGimbalL);
    
    static UINT YMainGimbalRGrp = 75;
    static MGROUP_ROTATE YMainGimbalR (1, &YMainGimbalRGrp, 1,
        _V(-0.3633,1.0355,7.1336), _V(-0.0423,0.9733,0.2257), static_cast<float>(31*RAD));
    anim_ymaingimbal[1] = CreateAnimation (0.5);
    AddAnimationComponent (anim_ymaingimbal[1], 0, 1, &YMainGimbalR);
    
    static UINT HBalanceGrp = 68;
    static MGROUP_ROTATE HBalance (1, &HBalanceGrp, 1,
        _V(-0.2561,1.1232,7.2678), _V(0.9439,-0.0828,0.3197), static_cast<float>(31*RAD));
    anim_hbalance = CreateAnimation (0.5);
    AddAnimationComponent (anim_hbalance, 0, 1, &HBalance);
    
    static UINT HUDIntensGrp = 78;
    static MGROUP_ROTATE HUDIntens (1, &HUDIntensGrp, 1,
        _V(0.2427,1.1504,7.3136), _V(-0.7590,-0.231,0.6087), static_cast<float>(31*RAD));
    anim_hudintens = CreateAnimation (0.5);
    AddAnimationComponent (anim_hudintens, 0, 1, &HUDIntens);
    
    static UINT RCSDialGrp = 79;
    static MGROUP_ROTATE RCSDial (1, &RCSDialGrp, 1,
        _V(-0.3358,1.0683,7.2049), _V(0.3310,0.2352,-0.9138), static_cast<float>(100*RAD));
    anim_rcsdial = CreateAnimation (0.5);
    AddAnimationComponent (anim_rcsdial, 0, 1, &RCSDial);
    
    static UINT AFDialGrp = 83;
    static MGROUP_ROTATE AFDial (1, &AFDialGrp, 1,
        _V(-0.3361,1.1152,7.2179), _V(0.3310,0.2352,-0.9138), static_cast<float>(100*RAD));
    anim_afdial = CreateAnimation (0.5);
    AddAnimationComponent (anim_afdial, 0, 1, &AFDial);
    
    static UINT OLockSwitchGrp = 90;
    static MGROUP_ROTATE OLockSwitch (1, &OLockSwitchGrp, 1,
        _V(0.2506,1.0969,7.2866), _V(-0.7590,-0.231,0.6087), static_cast<float>(31*RAD));
    anim_olockswitch = CreateAnimation (1);
    AddAnimationComponent (anim_olockswitch, 0, 1, &OLockSwitch);
    
    static UINT ILockSwitchGrp = 93;
    static MGROUP_ROTATE ILockSwitch (1, &ILockSwitchGrp, 1,
        _V(0.2824,1.1066,7.2611), _V(-0.7590,-0.231,0.6087), static_cast<float>(31*RAD));
    anim_ilockswitch = CreateAnimation (1);
    AddAnimationComponent (anim_ilockswitch, 0, 1, &ILockSwitch);
    
    static UINT RetroSwitchGrp = 95;
    static MGROUP_ROTATE RetroSwitch (1, &RetroSwitchGrp, 1,
        _V(0.2508,1.0505,7.2694), _V(-0.7590,-0.231,0.6087), static_cast<float>(31*RAD));
    anim_retroswitch = CreateAnimation (1);
    AddAnimationComponent (anim_retroswitch, 0, 1, &RetroSwitch);
    
    static UINT LadderSwitchGrp = 96;
    static MGROUP_ROTATE LadderSwitch (1, &LadderSwitchGrp, 1,
        _V(0.2889,1.0622,7.2388), _V(-0.7590,-0.231,0.6087), static_cast<float>(31*RAD));
    anim_ladderswitch = CreateAnimation (1);
    AddAnimationComponent (anim_ladderswitch, 0, 1, &LadderSwitch);
    
    static UINT HatchSwitchGrp = 97;
    static MGROUP_ROTATE HatchSwitch (1, &HatchSwitchGrp, 1,
        _V(0.2511,1.0006,7.2507), _V(-0.7590,-0.231,0.6087), static_cast<float>(31*RAD));
    anim_hatchswitch = CreateAnimation (1);
    AddAnimationComponent (anim_hatchswitch, 0, 1, &HatchSwitch);
    
    static UINT RadiatorSwitchGrp = 98;
    static MGROUP_ROTATE RadiatorSwitch (1, &RadiatorSwitchGrp, 1,
        _V(0.2592,0.9517,7.2252), _V(-0.7590,-0.231,0.6087), static_cast<float>(31*RAD));
    anim_radiatorswitch = CreateAnimation (1);
    AddAnimationComponent (anim_radiatorswitch, 0, 1, &RadiatorSwitch);
}

// delete any child animation objects; invoked by our destructor
void DeltaGliderXR1::CleanUpAnimations()
{
    // Note: there are no child animations for the XR1, so this method is empty
}