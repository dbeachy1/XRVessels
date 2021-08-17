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
// XR3Phoenix implementation class
//
// XR3Animations.cpp
// Contains animation code for the XR3
// ==============================================================

#include "XR3Phoenix.h"
#include "XR3Globals.h"   // for SizeOfGrp
#include "meshres.h"


// Virtual Gateway method that decides which animations are valid for this vessel; if the incoming animation handle is valid, 
// the call is propogated up to SetAnimation.  Otherwise, this method returns without changing the animation state.
#define ALLOW(handlePtr)  if (&anim == &(handlePtr)) { SetAnimation(anim, state); return; }
void XR3Phoenix::SetXRAnimation(const UINT &anim, const double state) const
{
    // TODO: enable these as they are added to the XR3 code
#if 0
    ALLOW(anim_gear);         // handle for landing gear animation
	ALLOW(anim_rcover);       // handle for retro cover animation
    ALLOW(anim_hoverdoor);    // handle for hover doors animation
    ALLOW(anim_scramdoor);    // handle for scram doors animation
	ALLOW(anim_nose);         // handle for docking port animation
	ALLOW(anim_hatch);        // handle for top hatch animation 
	ALLOW(anim_radiator);     // handle for radiator animation
	ALLOW(anim_rudder);       // handle for rudder animation
	ALLOW(anim_elevator);     // handle for elevator animation
	ALLOW(anim_elevatortrim); // handle for elevator trim animation
	ALLOW(anim_laileron);     // handle for left aileron animation
	ALLOW(anim_raileron);     // handle for right aileron animation
	ALLOW(anim_brake);        // handle for airbrake animation
    ALLOW(anim_olock);        // handle for outer airlock door animation
    ALLOW(anim_ilock);        // handle for inner airlock door animation

    // new for XR3
    ALLOW(anim_crewElevator);
    ALLOW(anim_bay);
    ALLOW(m_animNoseGearCompression);
    ALLOW(m_animRearGearCompression);
    ALLOW(m_animFrontTireRotation);
    ALLOW(m_animRearTireRotation);
    ALLOW(m_animNosewheelSteering);
#endif
}

// --------------------------------------------------------------
// Define animation sequences for moving parts
// Invoked by our constructor.
// --------------------------------------------------------------
void XR3Phoenix::DefineAnimations()
{
/* TODO: use these coordinates and vectors from Loru's SC3 test settings for the XR3:
[DOCK_0]
POS=(0,-0.49,9.84)
DIR=(0,0,1)
ROT=(0,1,0)


;Exhaust-definitions.........................................................

[EX_MAIN_0]
OFF=(5.75,0,-13.5)
DIR=(0,0,-1)
LENGTH=12
WIDTH=1.2

[EX_MAIN_1]
OFF=(-5.75,0,-13.5)
DIR=(0,0,-1)
LENGTH=12
WIDTH=1.2

[EX_MAIN_2]
OFF=(-7.25,0,-13.5)
DIR=(0,0,-1)
LENGTH=12
WIDTH=1.2

[EX_MAIN_3]
OFF=(7.25,0,-13.5)
DIR=(0,0,-1)
LENGTH=12
WIDTH=1.2

[EX_HOVER_0]
OFF=(1.6,-1.1,10.6)
DIR=(0,-1,0)
LENGTH=2
WIDTH=0.8

[EX_HOVER_1]
OFF=(1.6,-1.1,9.4)
DIR=(0,-1,0)
LENGTH=2
WIDTH=0.8

[EX_HOVER_2]
OFF=(-1.6,-1.1,10.6)
DIR=(0,-1,0)
LENGTH=2
WIDTH=0.8

[EX_HOVER_3]
OFF=(-1.6,-1.1,9.4)
DIR=(0,-1,0)
LENGTH=2
WIDTH=0.8

[EX_HOVER_4]
OFF=(6.5,-0.9,-8.35)
DIR=(0,-1,0)
LENGTH=2
WIDTH=0.8

[EX_HOVER_5]
OFF=(6.5,-0.9,-9.5)
DIR=(0,-1,0)
LENGTH=2
WIDTH=0.8

[EX_HOVER_6]
OFF=(-6.5,-0.9,-8.35)
DIR=(0,-1,0)
LENGTH=2
WIDTH=0.8

[EX_HOVER_7]
OFF=(-6.5,-0.9,-9.5)
DIR=(0,-1,0)
LENGTH=2
WIDTH=0.8

[EX_RETRO_0]
OFF=(3.946,0.25,13.347)
DIR=(0.174,0,0.985)
LENGTH=3
WIDTH=0.5

[EX_RETRO_1]
OFF=(3.946,-0.25,13.347)
DIR=(0.174,0,0.985)
LENGTH=3
WIDTH=0.5

[EX_RETRO_2]
OFF=(-3.946,0.25,13.347)
DIR=(-0.174,0,0.985)
LENGTH=3
WIDTH=0.5

[EX_RETRO_3]
OFF=(-3.946,-0.25,13.347)
DIR=(-0.174,0,0.985)
LENGTH=3
WIDTH=0.5

;animations.........................................................

[ANIM_SEQ_0]
; landing gear
KEY=G
DURATION=6.7

[ANIM_SEQ_1]
; cargo bay door
KEY=K
DURATION=6.7

[ANIM_SEQ_2]
; radiators
KEY=0
DURATION=6.7

[ANIM_SEQ_3]
; radiators
KEY=1
DURATION=2

[ANIM_SEQ_4]
; scramdoor
KEY=2
DURATION=2

[ANIM_SEQ_5]
; docking_port
KEY=3
DURATION=5

[ANIM_SEQ_6]
; side EVA hatch
KEY=4
DURATION=5

[ANIM_SEQ_7]
; bottom EVA hatch
KEY=5
DURATION=5

[ANIM_SEQ_8]
; hover door
KEY=6
DURATION=2

[ANIM_SEQ_9]
; airbrake
KEY=7
DURATION=1

[ANIM_SEQ_10]
; rudder
INIT_POS=0.5

[ANIM_SEQ_11]
; elevator
INIT_POS=0.5

[ANIM_SEQ_12]
; left aileron
INIT_POS=0.5

[ANIM_SEQ_13]
; right aileron
INIT_POS=0.5

[ANIM_SEQ_14]
; airlock inner door
KEY=8
DURATION=1

[ANIM_SEQ_15]
; top docking port inner door inner door
KEY=9
DURATION=1



; -----------------------------components
;front gear

[ANIM_COMP_0]
;front-gear
SEQ=0
GROUPS=32,33,34
RANGE=(0.05,0.7)
ROT_PNT=(0,-0.5,15)
ROT_AXIS=(1,0,0)
ANGLE=90.

[ANIM_COMP_1]
;front-flap
SEQ=0
GROUPS=39
RANGE=(0.7,0.95)
ROT_PNT=(-0.8,-1.3,15)
ROT_AXIS=(0,0,1)
ANGLE=95.

[ANIM_COMP_2]
;front-flap
SEQ=0
GROUPS=40
RANGE=(0.7,0.95)
ROT_PNT=(0.8,-1.3,15)
ROT_AXIS=(0,0,1)
ANGLE=-95.

;rear gear

[ANIM_COMP_3]
;rear gear-parent
SEQ=0
GROUPS=28,29,86,87
RANGE=(0.05,0.7)
ROT_PNT=(0,0,-3)
ROT_AXIS=(1,0,0)
ANGLE=100.

[ANIM_COMP_4]
;rear gear-child
SEQ=0
GROUPS=25,26,27,30,31,85
RANGE=(0.05,0.7)
ROT_PNT=(0,-2.6,-3)
ROT_AXIS=(1,0,0)
ANGLE=-90.
PARENT=3

;outboard rear gear flaps

[ANIM_COMP_5]
SEQ=0
GROUPS=35
RANGE=(0.7,0.95)
ROT_PNT=(7.5,-1,-3)
ROT_AXIS=(0,0,1)
ANGLE=-95.

[ANIM_COMP_6]
SEQ=0
GROUPS=37
RANGE=(0.7,0.95)
ROT_PNT=(-7.5,-1,-3)
ROT_AXIS=(0,0,1)
ANGLE=95.

;inboard rear gear flaps

[ANIM_COMP_7]
SEQ=0
GROUPS=36
RANGE=(0.7,0.95)
ROT_PNT=(5.5,-1,-3)
ROT_AXIS=(0,0,1)
ANGLE=95.

[ANIM_COMP_8]
SEQ=0
GROUPS=38
RANGE=(0.7,0.95)
ROT_PNT=(-5.5,-1,-3)
ROT_AXIS=(0,0,1)
ANGLE=-95.

; cargobay door

[ANIM_COMP_9]
SEQ=1
GROUPS=89
RANGE=(0.1,0.90)
ROT_PNT=(-3.475,1.7,-3)
ROT_AXIS=(0,0,1)
ANGLE=150.

[ANIM_COMP_10]
SEQ=1
GROUPS=88
RANGE=(0.1,0.90)
ROT_PNT=(3.475,1.7,-3)
ROT_AXIS=(0,0,1)
ANGLE=-150.

;Radiators

[ANIM_COMP_11]
SEQ=2
GROUPS=47,73
RANGE=(0.1,0.90)
ROT_PNT=(-6.647,1.757,-6)
ROT_AXIS=(0,0,1)
ANGLE=80.

[ANIM_COMP_12]
SEQ=2
GROUPS=72,74
RANGE=(0.1,0.90)
ROT_PNT=(6.647,1.757,-6)
ROT_AXIS=(0,0,1)
ANGLE=-80.

;retros
[ANIM_COMP_13]
SEQ=3
GROUPS=62,65,71
RANGE=(0.1,0.90)
ROT_PNT=(-4,0,10.2)
ROT_AXIS=(0,1,0)
ANGLE=-10.

[ANIM_COMP_14]
SEQ=3
GROUPS=63,66,70
RANGE=(0.1,0.90)
ROT_PNT=(4,0,10.2)
ROT_AXIS=(0,1,0)
ANGLE=10.

;scram door
[ANIM_COMP_15]
SEQ=4
GROUPS=84
RANGE=(0.1,0.90)
ROT_PNT=(0,-1.3,3)
ROT_AXIS=(1,0,0)
ANGLE=23.

;dockport

;1 liftdown-parent
[ANIM_COMP_16]
SEQ=5
GROUPS=80
RANGE=(0.1,0.4)
TYPE=TRANSLATE
SHIFT=(0,-0.75,0)

;arm1
[ANIM_COMP_17]
SEQ=5
GROUPS=81
RANGE=(0.1,0.4)
ROT_PNT=(-.79,2.77,0)
ROT_AXIS=(0,0,1)
ANGLE=90.
PARENT=16

;arm2
[ANIM_COMP_18]
SEQ=5
GROUPS=79
RANGE=(0.1,0.4)
ROT_PNT=(.79,2.77,0)
ROT_AXIS=(0,0,1)
ANGLE=-90.
PARENT=16

;docport-door1
[ANIM_COMP_19]
SEQ=5
GROUPS=82
RANGE=(0.1,0.4)
ROT_PNT=(-0.01,2.77,0)
ROT_AXIS=(0,0,1)
ANGLE=-90.
PARENT=17

;docport-door2
[ANIM_COMP_20]
SEQ=5
GROUPS=83
RANGE=(0.1,0.4)
ROT_PNT=(0.01,2.77,0)
ROT_AXIS=(0,0,1)
ANGLE=90.
PARENT=18

;docking-ring
[ANIM_COMP_21]
SEQ=5
GROUPS=78
RANGE=(0.4,0.95)
TYPE=TRANSLATE
SHIFT=(0,0.5,0)

;docking-tunel
[ANIM_COMP_22]
SEQ=5
GROUPS=75
RANGE=(0.4,0.95)
TYPE=SCALE
SCALE=(1,2,1)
REF=(0,2,8.6)

;side hatch up
[ANIM_COMP_23]
SEQ=6
GROUPS=67
RANGE=(0.1,0.9)
ROT_PNT=(-2.7,2.3,0)
ROT_AXIS=(0,0,1)
ANGLE=-120.

;side hatch down
[ANIM_COMP_24]
SEQ=6
GROUPS=41
RANGE=(0.1,0.9)
ROT_PNT=(-3.9,0.9,0)
ROT_AXIS=(0,0,1)
ANGLE=120.

;bottom-hatch
[ANIM_COMP_25]
SEQ=7
GROUPS=64,91
RANGE=(0.1,0.7)
ROT_PNT=(0,-1.3,9.2)
ROT_AXIS=(1,0,0)
ANGLE=-90.

;ladder-extension
[ANIM_COMP_26]
SEQ=7
GROUPS=90
RANGE=(0.7,0.95)
ROT_PNT=(0,-1.09,7.28)
ROT_AXIS=(1,0,0)
ANGLE=-180.
PARENT=25

; hover door section 8 animations

[ANIM_COMP_27]
SEQ=8
GROUPS=54
RANGE=(0.25,0.95)
ROT_PNT=(-2.2,-1.3,0)
ROT_AXIS=(0,0,1)
ANGLE=-90.

[ANIM_COMP_28]
SEQ=8
GROUPS=55
RANGE=(0.25,0.95)
ROT_PNT=(-1,-1.3,0)
ROT_AXIS=(0,0,1)
ANGLE=90.

[ANIM_COMP_29]
SEQ=8
GROUPS=57
RANGE=(0.25,0.95)
ROT_PNT=(2.2,-1.3,0)
ROT_AXIS=(0,0,1)
ANGLE=90.

[ANIM_COMP_30]
SEQ=8
GROUPS=56
RANGE=(0.25,0.95)
ROT_PNT=(1,-1.3,0)
ROT_AXIS=(0,0,1)
ANGLE=-90.

[ANIM_COMP_31]
SEQ=8
GROUPS=58
RANGE=(0.25,0.95)
ROT_PNT=(-7.1,-1,0)
ROT_AXIS=(0,0,1)
ANGLE=-90.

[ANIM_COMP_32]
SEQ=8
GROUPS=61
RANGE=(0.25,0.95)
ROT_PNT=(7.1,-1,0)
ROT_AXIS=(0,0,1)
ANGLE=90.

[ANIM_COMP_33]
SEQ=8
GROUPS=59
RANGE=(0.25,0.95)
ROT_PNT=(-5.9,-1,0)
ROT_AXIS=(0,0,1)
ANGLE=90.

[ANIM_COMP_34]
SEQ=8
GROUPS=60
RANGE=(0.25,0.95)
ROT_PNT=(5.9,-1,0)
ROT_AXIS=(0,0,1)
ANGLE=-90.

; Airbrake

[ANIM_COMP_35]
;bottom
SEQ=9
GROUPS=48,50
RANGE=(0.05,0.95)
ROT_PNT=(0,0,-10.1)
ROT_AXIS=(1,0,0)
ANGLE=-20.

[ANIM_COMP_36]
;top
SEQ=9
GROUPS=49,51
RANGE=(0.05,0.95)
ROT_PNT=(0,0,-10.1)
ROT_AXIS=(1,0,0)
ANGLE=20.

; rudder

[ANIM_COMP_37]
;rudder1
SEQ=10
GROUPS=45
RANGE=(0.0,1.0)
ROT_PNT=(-6.485,1.486,-11.45)
ROT_AXIS=(0.342,0.94,0)
ANGLE=-40.

[ANIM_COMP_38]
;rudder2
SEQ=10
GROUPS=46
RANGE=(0.0,1.0)
ROT_PNT=(6.485,1.486,-11.45)
ROT_AXIS=(-0.342,0.94,0)
ANGLE=-40.

; elevator

[ANIM_COMP_39]
;elevators all
SEQ=11
GROUPS=48,49,50,51
RANGE=(0.05,0.95)
ROT_PNT=(0,0,-10.1)
ROT_AXIS=(1,0,0)
ANGLE=30.

; aileron
[ANIM_COMP_40]
;aileron left
SEQ=12
GROUPS=50,51
RANGE=(0.05,0.95)
ROT_PNT=(0,0,-10.1)
ROT_AXIS=(1,0,0)
ANGLE=-30

[ANIM_COMP_41]
;aleron right
SEQ=13
GROUPS=48,49
RANGE=(0.05,0.95)
ROT_PNT=(0,0,-10.1)
ROT_AXIS=(1,0,0)
ANGLE=30


[ANIM_COMP_42]
;airlock - cabin door
SEQ=14
GROUPS=93
RANGE=(0.1,0.9)
TYPE=TRANSLATE
SHIFT=(1,0,0)

;the same will be for door to cargo bay tunel but I've run out of keys - according groups are 94 and 95 and shift is (-1,0,0)

[ANIM_COMP_43]
;top cabin door
SEQ=15
GROUPS=77
RANGE=(0.1,0.9)
TYPE=TRANSLATE
SHIFT=(0,0,0.45)

[ANIM_COMP_44]
;top cabin door
SEQ=15
GROUPS=76
RANGE=(0.1,0.9)
TYPE=TRANSLATE
SHIFT=(0,0,-0.45)

*/

// The code below that is #if'd out is a copy of the XR5's code for reference / template purposes
#if 0
    //----------landing gear start-----------landing gear start-------------landing gear start----------landing gear start----------
    //----------landing gear start-----------landing gear start-------------landing gear start----------landing gear start----------
    //----------------nose gear doors front start--------------------
    // right
    static UINT nose_gear_door_front_rightGrp[1] = {GRP_nose_door_front_right};
    static MGROUP_ROTATE nose_gear_door_front_right (0, nose_gear_door_front_rightGrp, 1,
        _V(2.188115, -1.265304, 21.500756), _V(0,0,1), static_cast<float>(90*RAD));  
    // left
    static UINT nose_gear_door_front_leftGrp[1] = {GRP_nose_door_front_left};
    static MGROUP_ROTATE nose_gear_door_front_left (0, nose_gear_door_front_leftGrp, 1,
        _V(-2.195979, -1.265305, 21.500757), _V(0,0,1), static_cast<float>(-90*RAD));
    

    //------------------------------------------------------nose gear doors front end--------------------
    //----------------nose gear doors back down start--------------------
    // right
    static UINT nose_gear_door_back_right_dGrp[1] = {GRP_nose_door_back_right};
    static MGROUP_ROTATE nose_gear_door_back_right_d (0, nose_gear_door_back_right_dGrp, 1,
        _V(2.188115, -1.265304, 21.500756), _V(0,0,1), static_cast<float>(-90*RAD));

    // left
    static UINT nose_gear_door__back_left_dGrp[1] = {GRP_nose_door_back_left};
    static MGROUP_ROTATE nose_gear_door_back_left_d (0, nose_gear_door__back_left_dGrp, 1,
        _V(-2.195979, -1.265305, 21.500757), _V(0,0,1), static_cast<float>(90*RAD));
    //------------------------------------------------------nose gear doors back down end--------------------
    //----------------nose gear doors back up start--------------------
    // right
    static UINT nose_gear_door_back_right_uGrp[1] = {GRP_nose_door_back_right};
    static MGROUP_ROTATE nose_gear_door_back_right_u (0, nose_gear_door_back_right_uGrp, 1,
        _V(2.188115, -1.265304, 21.500756), _V(0,0,1), static_cast<float>(90*RAD));
    // left
    static UINT nose_gear_door__back_left_uGrp[1] = {GRP_nose_door_back_left};
    static MGROUP_ROTATE nose_gear_door_back_left_u (0, nose_gear_door__back_left_uGrp, 1,
        _V(-2.195979, -1.265305, 21.500757), _V(0,0,1), static_cast<float>(-90*RAD));
    //--------------------------------------------------------nose gear doors back up end--------------------
    //----------------left rear gear door inside start--------------------
    //down
    static UINT gear_door_left_insideGrp[1] = {GRP_gear_door_left_inside,};
    static MGROUP_ROTATE gear_door_left_inside_d (0, gear_door_left_insideGrp,1,
        _V(-7.317603, -2.370169, -7.618354), _V(0,0,1), static_cast<float>(-90*RAD));
    // up
    static UINT gear_door_left_inside1Grp[1] = {GRP_gear_door_left_inside,};
    static MGROUP_ROTATE gear_door_left_inside_u (0, gear_door_left_insideGrp,1,
        _V(-7.317603, -2.370169, -7.618354), _V(0,0,1), static_cast<float>(90*RAD));
    //---------------------------------------------------left rear gear door inside end--------------------
    //----------------left rear gear door outside start--------------------
    //top
    static UINT gear_door_left_outside_1Grp[1] = {GRP_gear_door_left_outside_1,};
    static MGROUP_ROTATE gear_door_left_outside_1 (0, gear_door_left_outside_1Grp,1,
        _V(-17.231083, -1.243402, -13.627935), _V(0,0,1), static_cast<float>(-94.4 * RAD));
    
    //bottom
    static UINT gear_door_left_outside_2Grp[1] = {GRP_gear_door_left_outside_2,};
    m_gear_door_left_outside_2 = new MGROUP_ROTATE (0, gear_door_left_outside_2Grp,1,
        _V(-17.86693, -3.671284, -7.622968), _V(0,0,1), static_cast<float>(170*RAD));
    //--------------------------------------------------left rear gear doors outside end--------------------                            
    //----------------right rear gear door inside start--------------------
    //down
    static UINT gear_door_right_insideGrp[1] = {GRP_gear_door_right_inside,};
    static MGROUP_ROTATE gear_door_right_inside_d (0, gear_door_right_insideGrp,1,
        _V(7.326452, -2.370169, -7.618353), _V(0,0,1), static_cast<float>(90*RAD));
    //up
    static UINT gear_door_right_inside1Grp[1] = {GRP_gear_door_right_inside,};
    static MGROUP_ROTATE gear_door_right_inside_u (0, gear_door_right_insideGrp,1,
        _V(7.326452, -2.370169, -7.618353), _V(0,0,1), static_cast<float>(-90*RAD));
    //----------------------------------------------------right rear gear door inside end--------------------
    //----------------right rear gear doors outside start--------------------
    // top
    static UINT gear_door_right_outside_1Grp[1] = {GRP_gear_door_right_outside_1,};
    static MGROUP_ROTATE gear_door_right_outside_1 (0, gear_door_right_outside_1Grp,1,
        _V(17.239931, -1.243401, -13.627935), _V(0,0,1), static_cast<float>(94.5 *RAD));
    //bottom
    static UINT gear_door_right_outside_2Grp[1] = {GRP_gear_door_right_outside_2,};
    m_gear_door_right_outside_2 = new MGROUP_ROTATE (0, gear_door_right_outside_2Grp,1,
        _V(17.868013, -3.669825, -13.44978), _V(0,0,1), static_cast<float>(-170*RAD));

    //--------------------------------------------------right rear gear doors outside end--------------------
    // --------------gear doors start -----------------------------------
    anim_gear = CreateAnimation(1);   // mesh shows gear DEPLOYED in its initial state

    AddAnimationComponent (anim_gear, 0, 0.15, &nose_gear_door_front_right);
    AddAnimationComponent (anim_gear, 0, 0.15, &nose_gear_door_front_left);

    AddAnimationComponent (anim_gear, 0.6, 1.0, &nose_gear_door_back_right_d);  // this door is always closed. only opens to let gear pass
    AddAnimationComponent (anim_gear, 0.6, 1.0, &nose_gear_door_back_left_d);   // this door is always closed. only opens to let gear pass
    AddAnimationComponent (anim_gear, 0, 0.15, &nose_gear_door_back_right_u);   // this door is always closed. only opens to let gear pass
    AddAnimationComponent (anim_gear, 0, 0.15, &nose_gear_door_back_left_u);    // this door is always closed. only opens to let gear pass

    AddAnimationComponent (anim_gear, 0.725, 1.0, &gear_door_left_inside_d);    // this door is always closed. only opens to let gear pass
    AddAnimationComponent (anim_gear, 0, 0.2, &gear_door_left_inside_u);        // this door is always closed. only opens to let gear pass
    AddAnimationComponent (anim_gear, 0.725, 1.0, &gear_door_right_inside_d);   // this door is always closed. only opens to let gear pass
    AddAnimationComponent (anim_gear, 0, 0.2, &gear_door_right_inside_u);       // this door is always closed. only opens to let gear pass
    
    //---------------left rear gear doors outside start--------------------
    ANIMATIONCOMPONENT_HANDLE gearDoorLeftOutside1Handle = AddAnimationComponent (anim_gear, 0, 0.4, &gear_door_left_outside_1);
    AddAnimationComponent (anim_gear, 0, 0.4, m_gear_door_left_outside_2, gearDoorLeftOutside1Handle);
    //---------------------------------------------------left rear gear doors outside end--------------------
    //----------------right rear gear doors outside start--------------------
    ANIMATIONCOMPONENT_HANDLE gearDoorrightOutside1Handle = AddAnimationComponent (anim_gear, 0, 0.4, &gear_door_right_outside_1);
    AddAnimationComponent (anim_gear, 0, 0.4, m_gear_door_right_outside_2, gearDoorrightOutside1Handle);

    //---------------------------------------------------right rear gear doors outside end--------------------
    // ---------------------------------------------------------gear doors end -----------------------------------

    //------------------------- main nose gear ---------------------------
    static UINT noseCylinderGroup[1] = {GRP_nose_oleo_cylinder};
    // determines how far the cylinder is in the ship when horizontal (stowed)
    static MGROUP_ROTATE rotateNoseCylinder(0, noseCylinderGroup, 1, _V(0, 0.5, 22.680), _V(1,0,0), static_cast<float>(-90*RAD));  
    
    // This group contains every group attached to the main nose cylinder *except* for the wheels.  The reason
    // is because each mesh group must only appear "once" in the heirarchical group tree.  The wheels will be added a child group to this group,
    // and therefore they must not be included here in the parent group as well, or they will move "twice as much" in the animation.
    static UINT noseGearGrp[] = {GRP_nose_axle, GRP_nose_axle_cylinder, GRP_nose_axle_piston, GRP_nose_oleo_piston};

    // this fully compresses the front gear for storage; it is not used for landing compression, which is handled separately
    m_noseGearTranslation = new MGROUP_TRANSLATE(0, noseGearGrp, SizeOfGrp(noseGearGrp), _V(0,-2.7,0));

    // this group is only used to attach the main gear to its parent cylinder
    m_noseGearNoMovement = new MGROUP_TRANSLATE(0, noseGearGrp, SizeOfGrp(noseGearGrp), _V(0, 0, 0));

    static MGROUP_TRANSLATE translate_all_nose_gear_forward (0, noseCylinderGroup, SizeOfGrp(noseCylinderGroup), _V(0,-2.5,0));  // move gear FORWARD toward the nose
    static MGROUP_TRANSLATE translate_all_nose_gear_in (0, noseCylinderGroup, SizeOfGrp(noseCylinderGroup), _V(0,0,-0.75));      // moves gear UP into the ship

    ANIMATIONCOMPONENT_HANDLE noseCylinderHandle  = AddAnimationComponent (anim_gear, 0.1, 0.5, &rotateNoseCylinder);  
    ANIMATIONCOMPONENT_HANDLE noseGearHandle      = AddAnimationComponent (anim_gear, 0.35, 0.7, m_noseGearTranslation, noseCylinderHandle);
    
    // these translate the parent cylinder only, to which the nose strut and gear are attached
    ANIMATIONCOMPONENT_HANDLE noseCylinderParent1 = AddAnimationComponent (anim_gear, 0.35, 0.7, &translate_all_nose_gear_in);
    ANIMATIONCOMPONENT_HANDLE noseCylinderParent2 = AddAnimationComponent (anim_gear, 0.7,  1.0, &translate_all_nose_gear_forward);

    // Attach nose gear to its parent cylinder; we need multiple parent animations so that the nose gear is moved along with each phase of
    // its parents' animations.  These next lines really mean, "Apply any noseCylinderParent1 and noseCylinderParent2 transformations 
    // to noseGearGrp and its children as well."
    AddAnimationComponent (anim_gear, 0, 1.0, m_noseGearNoMovement, noseCylinderParent1);   
    AddAnimationComponent (anim_gear, 0, 1.0, m_noseGearNoMovement, noseCylinderParent2);
    
    //------------------------ rear gear ---------------------------

    // left side groups
    static UINT leftRearCylinderGrp[1] = {GRP_gear_main_oleo_cylinder_left};
    static MGROUP_ROTATE rotateLeftRearCylinder (0, leftRearCylinderGrp, 1, _V(-16.309,0,-10.781), _V(0,0,1), static_cast<float>(-90*RAD));  

    // This group contains every group attached to the main rear left cylinder *except* for the wheels.  The reason
    // is because each mesh group must only appear "once" in the heirarchical group tree.  The wheels will be added a child group to this group,
    // and therefore they must not be included here in the parent group as well, or they will move "twice as much" in the animation.
    static UINT rearLeftGearGrp[] = { GRP_axle_cylinder_left, GRP_axle_left, GRP_axle_piston_left, GRP_oleo_piston_left };
    // this fully compresses the left rear gear for storage, and handles landing compression as well
    m_rearGearLeftTranslation = new MGROUP_TRANSLATE(0, rearLeftGearGrp, SizeOfGrp(rearLeftGearGrp), _V(0, -GEAR_COMPRESSION_DISTANCE, 0)  );

     // right side groups
    static UINT rightRearCylinderGrp[1] = {GRP_gear_main_oleo_cylinder_right};
    static MGROUP_ROTATE rotateRightRearCylinder (0, rightRearCylinderGrp, 1, _V(16.309, 0, -10.781), _V(0,0,1), static_cast<float>(90*RAD));  

    static UINT rearRightGearGrp[] = { GRP_axle_cylinder_right, GRP_axle_right, GRP_axle_piston_right, GRP_oleo_piston_right };
    m_rearGearRightTranslation = new MGROUP_TRANSLATE(0, rearRightGearGrp, SizeOfGrp(rearRightGearGrp), _V(0, -GEAR_COMPRESSION_DISTANCE, 0)  );

    // animations
    ANIMATIONCOMPONENT_HANDLE leftRearCylinderHandle  = AddAnimationComponent (anim_gear, 0.1, 1, &rotateLeftRearCylinder);      // main cylinder @ top of strut assembly
    ANIMATIONCOMPONENT_HANDLE leftRearGearHandle      = AddAnimationComponent (anim_gear, 0.4, 1.0, m_rearGearLeftTranslation, leftRearCylinderHandle);   // compress/decompress strut 
    ANIMATIONCOMPONENT_HANDLE rightRearCylinderHandle = AddAnimationComponent (anim_gear, 0.1, 1, &rotateRightRearCylinder);      
    ANIMATIONCOMPONENT_HANDLE rightRearGearHandle     = AddAnimationComponent (anim_gear, 0.4, 1.0, m_rearGearRightTranslation, rightRearCylinderHandle);   

    //---------------------------------------------------------------------
    // Landing strut compression
    //---------------------------------------------------------------------
    static MGROUP_TRANSLATE noseCompressionTranslate(0, noseGearGrp, SizeOfGrp(noseGearGrp), _V(0, -GEAR_COMPRESSION_DISTANCE, 0));
    m_animNoseGearCompression = CreateAnimation(1);
    ANIMATIONCOMPONENT_HANDLE frontCompressionHandle = AddAnimationComponent(m_animNoseGearCompression, 0, 1, &noseCompressionTranslate);

    // rear gear compression; both gear struts always move in sync as a pair
    m_animRearGearCompression = CreateAnimation(1);
    ANIMATIONCOMPONENT_HANDLE rearLeftCompressionHandle  = AddAnimationComponent(m_animRearGearCompression, 0, 1, m_rearGearLeftTranslation);
    ANIMATIONCOMPONENT_HANDLE rearRightCompressionHandle = AddAnimationComponent(m_animRearGearCompression, 0, 1, m_rearGearRightTranslation);   

    //---------------------------------------------------
    // Gear wheel rotation
    //---------------------------------------------------
    static UINT frontRotationGrp[] = { GRP_nose_gear_wheel_left, GRP_nose_gear_wheel_right };

    // these must be in different groups because they each have different parent/axis combinations
    // Groups are named TIRE_SHIP_side, which seems bass-ackwards to me...
    static UINT rearLeftFgrp[]  = { GRP_wheel_left_front_left_side,  GRP_wheel_right_front_left_side  };
    static UINT rearRightFgrp[] = { GRP_wheel_left_front_right_side, GRP_wheel_right_front_right_side };
    static UINT rearLeftBgrp[]  = { GRP_wheel_left_rear_left_side,   GRP_wheel_right_rear_left_side   };
    static UINT rearRightBgrp[] = { GRP_wheel_left_rear_right_side,  GRP_wheel_right_rear_right_side  };

    m_frontWheelRotation = new MGROUP_ROTATE(0, frontRotationGrp,  2, _V(0, -9.576,  21.436), _V(1,0,0), static_cast<float>(360 * RAD));
    
    m_rearLeftRearRotationF  = new MGROUP_ROTATE(0, rearLeftFgrp,  2, _V(0, -9.576,  -9.199), _V(1,0,0), static_cast<float>(360 * RAD));
    m_rearRightRearRotationF = new MGROUP_ROTATE(0, rearRightFgrp, 2, _V(0, -9.576,  -9.199), _V(1,0,0), static_cast<float>(360 * RAD));
    
    m_rearLeftRearRotationB  = new MGROUP_ROTATE(0, rearLeftBgrp,  2, _V(0, -9.576, -12.397), _V(1,0,0), static_cast<float>(360 * RAD));
    m_rearRightRearRotationB = new MGROUP_ROTATE(0, rearRightBgrp, 2, _V(0, -9.576, -12.397), _V(1,0,0), static_cast<float>(360 * RAD));

    m_animFrontTireRotation = CreateAnimation(0);
    // Note: we need multiple parents here so that we move with each phase of the gear animation AND for gear compression
    // WARNING: by necessity, the front wheels are in this sequence twice, and so we must spin them 1/2 as fast as the rears!
    // This is handled in the PreStep for tire rotation.
    AddAnimationComponent(m_animFrontTireRotation, 0, 1, m_frontWheelRotation, noseGearHandle);       // attached to nose gear
    AddAnimationComponent(m_animFrontTireRotation, 0, 1, m_frontWheelRotation, noseCylinderParent1);  
    AddAnimationComponent(m_animFrontTireRotation, 0, 1, m_frontWheelRotation, noseCylinderParent2);    // this adds the wheels twice, but we have no choice...
    AddAnimationComponent(m_animFrontTireRotation, 0, 1, m_frontWheelRotation, frontCompressionHandle);

    m_animRearTireRotation = CreateAnimation(0);
    AddAnimationComponent(m_animRearTireRotation, 0, 1, m_rearLeftRearRotationF,  leftRearGearHandle);   
    AddAnimationComponent(m_animRearTireRotation, 0, 1, m_rearLeftRearRotationB,  leftRearGearHandle); 
    AddAnimationComponent(m_animRearTireRotation, 0, 1, m_rearLeftRearRotationF,  rearLeftCompressionHandle);   
    AddAnimationComponent(m_animRearTireRotation, 0, 1, m_rearLeftRearRotationB,  rearLeftCompressionHandle); 
    
    AddAnimationComponent(m_animRearTireRotation, 0, 1, m_rearRightRearRotationF, rightRearGearHandle);  
    AddAnimationComponent(m_animRearTireRotation, 0, 1, m_rearRightRearRotationB, rightRearGearHandle);
    AddAnimationComponent(m_animRearTireRotation, 0, 1, m_rearRightRearRotationF, rearRightCompressionHandle);   
    AddAnimationComponent(m_animRearTireRotation, 0, 1, m_rearRightRearRotationB, rearRightCompressionHandle); 

    //---------------------------------------------------------------------
    // Nosewheel steering
    //---------------------------------------------------------------------
    static MGROUP_ROTATE noseCylinderSteering(0, noseCylinderGroup, SizeOfGrp(noseCylinderGroup), _V(0, 0, 22.680), _V(0,1,0), static_cast<float>(50*RAD));

    m_animNosewheelSteering = CreateAnimation(0.5);
    ANIMATIONCOMPONENT_HANDLE noseCylinderSteeringHandle = AddAnimationComponent(m_animNosewheelSteering, 0, 1.0, &noseCylinderSteering);

    // Attach nose gear to its parent cylinder; we need multiple parent animations so that the nose gear is moved along with each phase of
    // its parents' animations.  These next lines really mean, "Apply any noseCylinderSteeringHandle transformations 
    // to m_noseGearNoMovement and m_frontWheelRotation (plus any of their children)."
    AddAnimationComponent (m_animNosewheelSteering, 0, 1.0, m_noseGearNoMovement, noseCylinderSteeringHandle);   
    AddAnimationComponent (m_animNosewheelSteering, 0, 1.0, m_frontWheelRotation, noseCylinderSteeringHandle);   

    //----------landing gear end---------------------landing gear end---------------------landing gear end-----------


    //------------------------dock port start----------------------------------------------------------------------
    // docking port doors
    static UINT dock_door_rightGrp[1] = {GRP_dock_door_right};
    static MGROUP_ROTATE dock_door_right (0, dock_door_rightGrp, 1,
        _V(1.687, 7.934, 4.304), _V(-0.022, -0.034, 0.999), static_cast<float>(-140*RAD));

    static UINT dock_door_leftGrp[1] = {GRP_dock_door_left};
    static MGROUP_ROTATE dock_door_left (0, dock_door_leftGrp, 1,  
        _V(-1.729, 7.942 ,4.304), _V(0.022, -0.034, 0.999), static_cast<float>(140*RAD));

    // docking port itself
    static UINT dock_port00Grp[1] = {GRP_dockport_00};
    m_dock_port00 = new MGROUP_TRANSLATE (0, dock_port00Grp, 1, _V(0,.7,0)  );

    static UINT dock_port01Grp[1] = {GRP_dockport_01};
    m_dock_port01 = new MGROUP_TRANSLATE (0, dock_port01Grp, 1, _V(0,.7,0)  );

    static UINT dockport_ringGrp[1] = {GRP_dockport_ring};
    m_dock_port_ring = new MGROUP_TRANSLATE (0, dockport_ringGrp, 1, _V(0,.2,0)  );  


    anim_nose = CreateAnimation (0);   // this is treated as the docking port by the XR3 code, even though it uses the base class "anim_nose" member variable
    AddAnimationComponent (anim_nose, 0, .8, &dock_door_right);
    AddAnimationComponent (anim_nose, 0, .8, &dock_door_left);  
    ANIMATIONCOMPONENT_HANDLE dock_port00_handle = AddAnimationComponent (anim_nose, .3, .6, m_dock_port00);
    ANIMATIONCOMPONENT_HANDLE dock_port01_handle = AddAnimationComponent (anim_nose, .6, .8, m_dock_port01,dock_port00_handle);
    ANIMATIONCOMPONENT_HANDLE dock_port_ring_handle = AddAnimationComponent (anim_nose, .8, 1, m_dock_port_ring,dock_port01_handle);


    //------------------------dock port end----------------------------------------------------------------------

    //------------------------ outer airlock door start ----------------------------------------------------------------------

    // NOTE: these petals must be children of dock_port01_handle since they translate with it
    const double doorPetalYCoord = 7.475;
    static UINT door_petal_001Grp[1] = {GRP_door_petal_001};
    m_door_petal[0] = new MGROUP_ROTATE (0, door_petal_001Grp, 1,
        _V(-0.955727, doorPetalYCoord, 6.00584), _V(0.70710745818873111, 1.0832703945355475E-5, -0.70710610410073838), static_cast<float>(90*RAD));

    static UINT door_petal_002Grp[1] = {GRP_door_petal_002};
    m_door_petal[1] = new MGROUP_ROTATE (0, door_petal_002Grp, 1,
        _V(0.424622, doorPetalYCoord, 5.483639), _V(0.999999999999321, 1.1653015161028343E-6, 0.0), static_cast<float>(90*RAD));

    static UINT door_petal_003Grp[1] = {GRP_door_petal_003};
    m_door_petal[2]= new MGROUP_ROTATE (0, door_petal_003Grp, 1,
        _V(0.424622, doorPetalYCoord ,5.483639 ), _V(0.70710813524277616, -9.4786250277924277E-6, 0.7071054270641964), static_cast<float>(90*RAD));

    static UINT door_petal_004Grp[1] = {GRP_door_petal_004};
    m_door_petal[3]= new MGROUP_ROTATE (0, door_petal_004Grp, 1,
        _V(0.946823, doorPetalYCoord, 6.744339  ), _V(-1.3540979744412952E-6, 1.3540979745916302E-6, 0.99999999999816), static_cast<float>(90*RAD));

    static UINT door_petal_005Grp[1] = {GRP_door_petal_005};
    m_door_petal[4]= new MGROUP_ROTATE (0, door_petal_005Grp, 1,
        _V(0.946823, doorPetalYCoord, 6.744338 ), _V(-0.70710813525120342, 8.1245357380328129E-6, 0.7071054270726238), static_cast<float>(90*RAD));

    static UINT door_petal_006Grp[1] = {GRP_door_petal_006};
    m_door_petal[5]= new MGROUP_ROTATE (0, door_petal_006Grp, 1,
        _V(2.634096, doorPetalYCoord, 7.266539 ), _V(-0.99999999998642064, -4.6612114950330311E-6, 2.3306057480340142E-6), static_cast<float>(90*RAD));

    static UINT door_petal_007Grp[1] = {GRP_door_petal_007};
    m_door_petal[6]= new MGROUP_ROTATE (0, door_petal_007Grp, 1,
        _V(-0.433525, doorPetalYCoord, 7.266538 ), _V(-0.70710813526870664, -4.0622678697182969E-6, -0.7071054270901268), static_cast<float>(90*RAD));

    static UINT door_petal_008Grp[1] = {GRP_door_petal_008};
    m_door_petal[7]= new MGROUP_ROTATE (0, door_petal_008Grp, 1,
        _V(-0.955727, doorPetalYCoord, 6.744339  ), _V(0.0, -4.0622994245002438E-6, -0.99999999999174893), static_cast<float>(90*RAD));

    anim_olock = CreateAnimation(0);
    for (int i=0; i < 8; i++)
        AddAnimationComponent (anim_olock, 0, 1, m_door_petal[i], dock_port01_handle);

    //------------------------ outer airlock door end ----------------------------------------------------------------------

    //--------------- inner airlock door start -----------------------
    static UINT    inner_airlock_doorGrp[1] = {GRP_dock_port_inner_door};
    static MGROUP_ROTATE inner_airlock_door (0, inner_airlock_doorGrp, 1,
        _V(-1.032187, 6.178692, 5.976163), _V(0,0,1), static_cast<float>(-90*RAD));

    anim_ilock = CreateAnimation(0);
    AddAnimationComponent(anim_ilock, 0, 1, &inner_airlock_door);

    //--------------- inner airlock door end -----------------------


    //---------------right bay door start-----------------------
    static UINT    bay_door_rightGrp[1] = {GRP_bay_door_right};
    static MGROUP_ROTATE bay_door_right (0, bay_door_rightGrp, 1,
        _V(7.103, 4.569, -1.089), _V(0,0,1), static_cast<float>(-160*RAD));

    //---------------right bay door end-----------------------


    //---------------left bay door start-----------------------
    static UINT    bay_door_leftGrp[1] = {GRP_bay_door_left};
    static MGROUP_ROTATE bay_door_left (0, bay_door_leftGrp, 1,
        _V(-7.121, 4.569, -1.099), _V(0,0,1), static_cast<float>(160*RAD));

    //---------------left bay door end-----------------------

    //----------scram_door start ----------scram_door start ----------scram_door start ----------scram_door start ----------scram_door start
    static UINT scram_doorGrp[1] = {GRP_scram_door1};
    static MGROUP_ROTATE scram_door (0, scram_doorGrp, 1,
        _V(-6.832061, -1.315524, 3.450493), _V(1,0,0), static_cast<float>(15.5*RAD));

    anim_scramdoor = CreateAnimation(0);
    AddAnimationComponent(anim_scramdoor, 0, 1, &scram_door);

    // ------ Retro system animation ----
    static UINT retro_arm_leftGrp[] = {GRP_retro_arm_left, GRP_retro_rocket_inside_left, GRP_retro_rocket_outside_left};
    static MGROUP_ROTATE retro_arm_left (0, retro_arm_leftGrp, SizeOfGrp(retro_arm_leftGrp),
        _V(-4.526704, 0.685707, 23.725811), _V(0,1,0), static_cast<float>(-9.8*RAD));
    static UINT retro_arm_rightGrp[] = {GRP_retro_arm_right, GRP_retro_rocket_inside_right, GRP_retro_rocket_outside_right};
    static MGROUP_ROTATE retro_arm_right (0, retro_arm_rightGrp, SizeOfGrp(retro_arm_rightGrp),
        _V(4.526704, 0.685707, 23.725811), _V(0,1,0), static_cast<float>(9.8*RAD));

    static UINT retro_rockets_Grp[] = {GRP_retro_rocket_inside_left, GRP_retro_rocket_outside_left, GRP_retro_rocket_inside_right, GRP_retro_rocket_outside_right};
    static MGROUP_TRANSLATE retro_rockets (0, retro_rockets_Grp, SizeOfGrp(retro_rockets_Grp), _V(0,0,.3)  );

    anim_rcover = CreateAnimation(0);
    AddAnimationComponent (anim_rcover, 0, .7, &retro_arm_left);
    AddAnimationComponent (anim_rcover, 0, .7, &retro_arm_right);
    AddAnimationComponent (anim_rcover, .7, 1, &retro_rockets);

    // -------------- Payload Bay Doors -------------------
    anim_bay = CreateAnimation(0);
    AddAnimationComponent (anim_bay, 0, 1, &bay_door_right);
    AddAnimationComponent (anim_bay, 0, 1, &bay_door_left);

    // ------------------hover system--------------------------
    //-------------------------------all doors TRANSLATE up start------------------
    static UINT hover_doorsGrp[12] = {GRP_hover_door_left_front,GRP_hover_door_right_front,GRP_hover_door_left_rear,GRP_hover_door_right_aft,GRP_hover_door_right_1_left_wing,
        GRP_hover_door_left_1_left_wing,GRP_hover_door_right_2_left_wing,GRP_hover_door_left_2_left_wing,GRP_hover_door_left_1_right_wing,GRP_hover_door_right_1_right_wing,
        GRP_hover_door_left_2_right_wing,GRP_hover_door_right_2_right_wing};
    static MGROUP_TRANSLATE hover_doors (0, hover_doorsGrp, 12,
        _V(0,.2,0)  );
    //-------------------------------all doors TRANSLATE up end------------------	

    //---------------aft doors TRANSLATE back start------------------------- 

    static UINT front_hover_doors_aftGrp[2] = {GRP_hover_door_right_aft,GRP_hover_door_left_rear};
    static MGROUP_TRANSLATE front_hover_doors_aft (0, front_hover_doors_aftGrp, 2,
        _V(0,0,-1.6)  );

    //--------------------------------------------------aft doors TRANSLATE back end------------------------- 
    //---------------front doors TRANSLATE forward start------------------------- 

    static UINT front_hover_doors_frontGrp[2] = {GRP_hover_door_right_front,GRP_hover_door_left_front};
    static MGROUP_TRANSLATE front_hover_doors_front (0, front_hover_doors_frontGrp, 2,
        _V(0,0,1.6)  );
    //----------------------------------------------------front doors TRANSLATE forward end-------------------------
    //-----------hover_outlet_front TRANSLATE down start-----------------------------------------------------
    static UINT hover_outlet_front_Grp[2] = {GRP_hover_outlet_front_left,GRP_hover_outlet_front_right};
    static MGROUP_TRANSLATE hover_outlet_front (0, hover_outlet_front_Grp, 2,
        _V(0,-1,0)  );
    //------------------------------------------------------hover_outlet_front TRANSLATE down end-----------------------------------------------------


    //----------------------- left wing hovers start------------------------------------------------	

    //----------------hover_door_right_left_wing start---------------------------------------------
    static UINT hover_door_right_left_wingGrp[2] = {GRP_hover_door_right_1_left_wing,GRP_hover_door_right_2_left_wing};
    static MGROUP_TRANSLATE hover_door_right_left_wing (0, hover_door_right_left_wingGrp, 2,
        _V(1.6,0,0)  );     
    //--------------------------------------------------hover_door_right_left_wing end-------------------------------------------
    //------------------hover_door_left_left_wing start-------------------------------------------
    static UINT hover_door_left_left_wingGrp[2] = {GRP_hover_door_left_1_left_wing,GRP_hover_door_left_2_left_wing};
    static MGROUP_TRANSLATE hover_door_left_left_wing (0, hover_door_left_left_wingGrp, 2,
        _V(-1.6,0,0)  );   
    //----------------------------------------------------------------hover_door_left_left_wing end----
    //----------------hover_oulet_left_wing start---------------------------	
    static UINT hover_oulet_left_wingGrp[2] = {GRP_hover_outlet_left_forward,GRP_hover_outlet_left_aft};
    static MGROUP_TRANSLATE hover_oulet_left_wing (0, hover_oulet_left_wingGrp, 2,
        _V(0,-1,0)  );

    //----------------hover_oulet_left_wing end---------------------------	

    //----------------------- right wing hovers start------------------------------------------------	

    //----------------hover_door_right_left_wing start---------------------------------------------
    static UINT hover_door_right_right_wingGrp[2] = {GRP_hover_door_right_1_right_wing,GRP_hover_door_right_2_right_wing};
    static MGROUP_TRANSLATE hover_door_right_right_wing (0, hover_door_right_right_wingGrp, 2,
        _V(1.6,0,0)  );     
    //--------------------------------------------------hover_door_right_left_wing end-------------------------------------------
    //------------------hover_door_left_left_wing start-------------------------------------------
    static UINT hover_door_left_right_wingGrp[2] = {GRP_hover_door_left_1_right_wing,GRP_hover_door_left_2_right_wing};
    static MGROUP_TRANSLATE hover_door_left_right_wing (0, hover_door_left_right_wingGrp, 2,
        _V(-1.6,0,0)  );   
    //----------------------------------------------------------------hover_door_left_left_wing end----
    //----------------hover_oulet_left_wing start---------------------------	
    static UINT hover_oulet_right_wingGrp[2] = {GRP_hover_outlet_right_forward,GRP_hover_outlet_right_aft};
    static MGROUP_TRANSLATE hover_oulet_right_wing (0, hover_oulet_right_wingGrp, 2,
        _V(0,-1,0)  );

    //----------------hover_oulet_left_wing end---------------------------		

    anim_hoverdoor = CreateAnimation (0);
    AddAnimationComponent (anim_hoverdoor, 0, .2, &hover_doors);
    AddAnimationComponent (anim_hoverdoor, .2, .5, &front_hover_doors_aft);
    AddAnimationComponent (anim_hoverdoor, .2, .5, &front_hover_doors_front);
    AddAnimationComponent (anim_hoverdoor, .5, 1, &hover_outlet_front);
    AddAnimationComponent (anim_hoverdoor, .2, .5, &hover_door_right_left_wing);
    AddAnimationComponent (anim_hoverdoor, .2, .5, &hover_door_left_left_wing);
    AddAnimationComponent (anim_hoverdoor, .5, 1, &hover_oulet_left_wing);
    AddAnimationComponent (anim_hoverdoor, .2, .5, &hover_door_right_right_wing);
    AddAnimationComponent (anim_hoverdoor, .2, .5, &hover_door_left_right_wing);
    AddAnimationComponent (anim_hoverdoor, .5, 1, &hover_oulet_right_wing);

    //---------- Radiator animation start ------
    //---------- ----------------------Radiator animation right side start ------
    // ------------------door right bottom side start---------------------
    static UINT radiator_door_bottom_rightGrp[1] = {GRP_radiator_door_bottom_right};
    static MGROUP_ROTATE radiator_door_bottom_right (0, radiator_door_bottom_rightGrp, 1,
        _V(8.51313, 2.366325, -23.27969), _V(-0.061839935624561214, 0.0, -0.99808607963539386), static_cast<float>(160*RAD));
    // --------------------------------------------door right bottom side end---------------------
    // ------------------door right top side start---------------------
    static UINT radiator_door_top_rightGrp[1] = {GRP_radiator_door_top_right};
    m_radiator_door_top_right = new MGROUP_ROTATE (0, radiator_door_top_rightGrp, 1,
        _V(7.760109, 3.218705, -23.28654), _V ( 0.055688639405021563, 1.1587554948842699E-4, 0.9984481769296164), static_cast<float>(177*RAD));
    // --------------------------------------------door right bottom side end---------------------
    // ----------rad_panel_right_001 start-------------------------------------------
    static UINT rad_panel_right_001Grp[1] = {GRP_rad_panel_right_001};
    static MGROUP_ROTATE rad_panel_right_001 (0, rad_panel_right_001Grp, 1,
        _V(8.024,2.393,0), _V(0,0,1), static_cast<float>(-120*RAD));
    // -----------------------------------------------------rad_panel_right_001 end-----
    // ----------rad_panel_right_002 start-------------------------------------------
    static UINT rad_panel_right_002Grp[1] = {GRP_rad_panel_right_002};
    m_rad_panel_right_002 = new MGROUP_ROTATE (0, rad_panel_right_002Grp, 1,
        _V(6.741,3.779,0), _V(0,0,1), static_cast<float>(150*RAD));
    // ----------rad_panel_right_002 end-------------------------------------------
    // ----------rad_panel_right_003 start-------------------------------------------
    static UINT rad_panel_right_003Grp[1] = {GRP_rad_panel_right_003};
    m_rad_panel_right_003 = new MGROUP_ROTATE (0, rad_panel_right_003Grp, 1,
        _V(8.062,2.428,0), _V(0,0,1), static_cast<float>(-160*RAD));
    // ----------rad_panel_right_003 end-------------------------------------------
    // ----------rad_panel_right_004 start-------------------------------------------
    static UINT rad_panel_right_004Grp[1] = {GRP_rad_panel_right_004};
    m_rad_panel_right_004 = new MGROUP_ROTATE (0, rad_panel_right_004Grp, 1,
        _V(6.779,3.815,0), _V(0,0,1), static_cast<float>(160*RAD));
    // ----------rad_panel_right_004 end-------------------------------------------
    //---------- ----------------------Radiator animation right side end ------

    //--------Radiator animation left side start ------

    // ------------------door left bottom side start---------------------
    static UINT radiator_door_bottom_leftGrp[1] = {GRP_radiator_door_bottom_left};
    static MGROUP_ROTATE radiator_door_bottom_left (0, radiator_door_bottom_leftGrp, 1,
        _V(-8.513, 2.366, -23.279), _V(0.0618, 0.0, -0.998), static_cast<float>(-160*RAD));
    // --------------------------------------------door left bottom side end---------------------
    // ------------------door left top side start---------------------
    static UINT radiator_door_top_leftGrp[1] = {GRP_radiator_door_top_left};
    m_radiator_door_top_left = new MGROUP_ROTATE (0, radiator_door_top_leftGrp, 1,
        _V(-7.760109, 3.218705, -23.28654), _V (-0.055688053003507147, 1.1678323006870412E-4, 0.998448209530347), static_cast<float>(-177*RAD));
    // --------------------------------------------door left top side end---------------------
    // ----------rad_panel_left_001 start-------------------------------------------
    static UINT rad_panel_left_001Grp[1] = {GRP_rad_panel_left_001};
    static MGROUP_ROTATE rad_panel_left_001 (0, rad_panel_left_001Grp, 1,
        _V(-8.024,2.393,0), _V(0,0,1), static_cast<float>(120*RAD));
    // -----------------------------------------------------rad_panel_right_001 end-----
    // ----------rad_panel_left_002 start-------------------------------------------
    static UINT rad_panel_left_002Grp[1] = {GRP_rad_panel_left_002};
    m_rad_panel_left_002 = new MGROUP_ROTATE (0, rad_panel_left_002Grp, 1,
        _V(-6.741,3.779,0), _V(0,0,1), static_cast<float>(-150*RAD));
    // ----------rad_panel_left_002 end-------------------------------------------
    // ----------rad_panel_left_003 start-------------------------------------------
    static UINT rad_panel_left_003Grp[1] = {GRP_rad_panel_left_003};
    m_rad_panel_left_003 = new MGROUP_ROTATE (0, rad_panel_left_003Grp, 1,
        _V(-8.062,2.428,0), _V(0,0,1), static_cast<float>(160*RAD));
    // ----------rad_panel_left_003 end-------------------------------------------
    // ----------rad_panel_left_004 start-------------------------------------------
    static UINT rad_panel_left_004Grp[1] = {GRP_rad_panel_left_004};
    m_rad_panel_left_004 = new MGROUP_ROTATE (0, rad_panel_left_004Grp, 1,
        _V(-6.779,3.815,0), _V(0,0,1), static_cast<float>(-160*RAD));
    // ----------rad_panel_left_004 end-------------------------------------------

    //--------Radiator animation left side end ------


    anim_radiator = CreateAnimation(0);
    //---------------- radiator system right side start--------------------
    ANIMATIONCOMPONENT_HANDLE radiator_door_bottom_right_Handle = AddAnimationComponent (anim_radiator, 0, .3, &radiator_door_bottom_right);
    AddAnimationComponent (anim_radiator, 0, .3, m_radiator_door_top_right,radiator_door_bottom_right_Handle);

    ANIMATIONCOMPONENT_HANDLE rad_panel_right_001_Handle = AddAnimationComponent (anim_radiator, .27, 1, &rad_panel_right_001);
    ANIMATIONCOMPONENT_HANDLE rad_panel_right_002_Handle = AddAnimationComponent (anim_radiator, .4, 1, m_rad_panel_right_002, rad_panel_right_001_Handle);
    ANIMATIONCOMPONENT_HANDLE rad_panel_right_003_Handle = AddAnimationComponent (anim_radiator, .4, 1, m_rad_panel_right_003, rad_panel_right_002_Handle);
    AddAnimationComponent (anim_radiator, .4, 1, m_rad_panel_right_004,rad_panel_right_003_Handle);
    //---------------------------------------------------- radiator system right side end--------------------

    //---------------- radiator system left side start--------------------
    ANIMATIONCOMPONENT_HANDLE radiator_door_bottom_left_Handle = AddAnimationComponent (anim_radiator, 0, .3, &radiator_door_bottom_left);
    AddAnimationComponent (anim_radiator, 0, .3, m_radiator_door_top_left,radiator_door_bottom_left_Handle);

    ANIMATIONCOMPONENT_HANDLE rad_panel_left_001_Handle = AddAnimationComponent (anim_radiator, .3, 1, &rad_panel_left_001);
    ANIMATIONCOMPONENT_HANDLE rad_panel_left_002_Handle = AddAnimationComponent (anim_radiator, .4, 1, m_rad_panel_left_002, rad_panel_left_001_Handle);
    ANIMATIONCOMPONENT_HANDLE rad_panel_left_003_Handle = AddAnimationComponent (anim_radiator, .4, 1, m_rad_panel_left_003, rad_panel_left_002_Handle);
    AddAnimationComponent(anim_radiator, .4, 1, m_rad_panel_left_004, rad_panel_left_003_Handle);
    //---------------------------------------------------- radiator system left side end--------------------
    //---------- Radiator animation end ------

    // ---------- Elevator animation ---------
    static UINT ElevatorGrp[2] = {GRP_elevator_left,GRP_elevator_right};
    static MGROUP_ROTATE Elevator (0, ElevatorGrp, 2,
        _V(10.917201, -0.0924926, -23.058558), _V(1,0,0), static_cast<float>(40*RAD));
    anim_elevator = CreateAnimation (0.5);

    AddAnimationComponent (anim_elevator, 0, 1, &Elevator);

    // ---------- Elevator trim animation ---------
    static MGROUP_ROTATE ElevatorTrim (0, ElevatorGrp, 2,
        _V(10.917201, -0.0924926, -23.058558), _V(1,0,0), static_cast<float>(20*RAD));  // was 10 degrees, but increased to 20 because trim range increased
    anim_elevatortrim = CreateAnimation (0.5);
    AddAnimationComponent (anim_elevatortrim, 0, 1, &ElevatorTrim);

    // -------------- Aileron animation --------------
    const float aileronSpread = static_cast<float>(37 * RAD);
    /* ORG to use elevons
    static UINT LAileronGrp[1] = {GRP_elevator_left};
    static MGROUP_ROTATE LAileron (0, LAileronGrp, 1,
        _V(10.917201, -0.0924926, -23.058558), _V(1,0,0), -aileronSpread);
    */
    static UINT LAileronGrp[2] = {GRP_upper_brake_left, GRP_lower_brake_left};
    static MGROUP_ROTATE LAileron (0, LAileronGrp, SizeOfGrp(LAileronGrp),
        _V(-36.839389,-0.0826128,-24.137964), _V(1,0,0), -aileronSpread);

    anim_laileron = CreateAnimation (0.5);
    AddAnimationComponent (anim_laileron, 0, 1, &LAileron);

    /* ORG to use elevons
    static UINT RAileronGrp[1] = {GRP_elevator_right};
    static MGROUP_ROTATE RAileron (0, RAileronGrp, 1,
        _V(10.917201, -0.0924926, -23.058558), _V(1,0,0), aileronSpread);
    */
    static UINT RAileronGrp[2] = {GRP_upper_brake_right, GRP_lower_brake_right};
    static MGROUP_ROTATE RAileron (0, RAileronGrp, SizeOfGrp(RAileronGrp),
        _V(36.861465,-0.0826143,-24.09242), _V(1,0,0), aileronSpread);

    anim_raileron = CreateAnimation (0.5);
    AddAnimationComponent (anim_raileron, 0, 1, &RAileron);

    // ***** Airbrake ("speedbrake") animation *****
    const float brakeSpread = static_cast<float>(50 * RAD);   // too much spread shows red in-between
    static UINT UpperBrakeGrp[2] = {GRP_upper_brake_right,GRP_upper_brake_left};
    static MGROUP_ROTATE UpperBrake (0, UpperBrakeGrp, 2,
        _V(36.839, -0.074, -24.138), _V(1,0,0), brakeSpread);
    static UINT LowerBrakeGrp[2] = {GRP_lower_brake_right,GRP_lower_brake_left};
    static MGROUP_ROTATE LowerBrake (0, LowerBrakeGrp, 2,
        _V(36.839, -0.074, -24.138), _V(1,0,0), -brakeSpread);

    anim_brake = CreateAnimation (0);
    AddAnimationComponent (anim_brake, 0, 1, &UpperBrake);
    AddAnimationComponent (anim_brake, 0, 1, &LowerBrake);

    // ----------------- Rudder animation -------------
    const float rudderSpread = static_cast<float>(37 * RAD);
    static UINT RRudderGrpTop[1] = {GRP_upper_brake_right};
    static MGROUP_ROTATE RRudderTop (0, RRudderGrpTop, 1,
        _V( -36.861465, -0.0826143, -24.09242), _V( 1,0,0), rudderSpread);

    static UINT RRudderGrpBot[1] = {GRP_lower_brake_right};
    static MGROUP_ROTATE RRudderBot (0, RRudderGrpBot, 1,
        _V( -36.861465, -0.0826143, -24.09242), _V( 1,0,0), -rudderSpread);

    static UINT LRudderGrpTop[1] = {GRP_upper_brake_left};
    static MGROUP_ROTATE LRudderTop (0, LRudderGrpTop, 1,
        _V( -36.861465, -0.0826143, -24.09242), _V( 1,0,0), -rudderSpread);

    static UINT LRudderGrpBot[1] = {GRP_lower_brake_left};
    static MGROUP_ROTATE LRudderBot (0, LRudderGrpBot, 1,
        _V( -36.861465, -0.0826143, -24.09242), _V( 1,0,0), rudderSpread);

    anim_rudder = CreateAnimation (0.5);
    AddAnimationComponent (anim_rudder, 0.5, 1.0, &RRudderTop);
    AddAnimationComponent (anim_rudder, 0.5, 1.0, &RRudderBot);
    AddAnimationComponent (anim_rudder, 0.0, 0.5, &LRudderTop);
    AddAnimationComponent (anim_rudder, 0.0, 0.5, &LRudderBot);

    // D. Beachy: new code
    // crew hatch animations
    const float hatchRotationFront = static_cast<float>(150 * RAD);
    const float hatchRotationRear = static_cast<float>(180 * RAD);
    static UINT HatchLeftFrontGrp[1] = {GRP_Hatch_left_front};
    static MGROUP_ROTATE RotHatchLeftFront(0, HatchLeftFrontGrp, 1,
        _V(-2.041, 5.956, 18.809), _V(0.0230, -0.153, 0.988), hatchRotationFront);

    static UINT HatchRightFrontGrp[1] = {GRP_Hatch_right_front};
    static MGROUP_ROTATE RotHatchRightFront(0, HatchRightFrontGrp, 1,
        _V(2.058, 6.069, 18.078), _V(0.0230, 0.153, -0.988), hatchRotationFront);

    static UINT HatchRightRearGrp[1] = {GRP_Hatch_right_back};
    static MGROUP_ROTATE RotHatchRightRear(0, HatchRightRearGrp, 1,
        _V(2.457, 7.745, 5.666), _V(0.015, -0.041, 0.999), -hatchRotationRear);

    static UINT HatchLeftRearGrp[1] = {GRP_hatch_left_back};
    static MGROUP_ROTATE RotHatchLeftRear(0, HatchLeftRearGrp, 1,
        _V(-2.461, 7.744, 5.666), _V(0.015, -0.041, 0.999), hatchRotationRear);

    anim_hatch = CreateAnimation(0);
    AddAnimationComponent (anim_hatch, 0, 1.0, &RotHatchLeftFront);
    AddAnimationComponent (anim_hatch, 0, 1.0, &RotHatchRightFront);
    AddAnimationComponent (anim_hatch, 0, 1.0, &RotHatchRightRear);
    AddAnimationComponent (anim_hatch, 0, 1.0, &RotHatchLeftRear);

    //-----------------------------------
    // Crew elevator animation
    //-----------------------------------

    // translate main elevator doors UP
    static UINT TranslateElevatorDoorsUpGrp[2] = { GRP_elevator_door_aft, GRP_elevator_door_forward };
    static MGROUP_TRANSLATE TranslateElevatorDoorsUp(0, TranslateElevatorDoorsUpGrp, SizeOfGrp(TranslateElevatorDoorsUpGrp), _V(0, 0.2, 0));   // translate UP

    // rotate arm elevator doors OPEN
    static UINT RotateAftElevatorArmDoorGrp[1] = { GRP_elevator_arm_door_aft };
    static MGROUP_ROTATE RotateAftElevatorArmDoor(0, RotateAftElevatorArmDoorGrp, 1, _V(-0.191, -1.373, 5.164), _V(-1.0, -0.014, -0.002), static_cast<float>(-175 * RAD));
    
    static UINT RotateForwardElevatorArmDoorGrp[1] = { GRP_elevator_arm_door_forward };
    m_forwardElevatorArmDoor = new MGROUP_ROTATE(0, RotateForwardElevatorArmDoorGrp, 1, _V(4.895, -1.253, 6.122), _V(1.0, 0.009, 0.002), static_cast<float>(-175 * RAD));

    // translate main doors BACK
    const double elevatorDoorTranslationZ = 2.18;
    static UINT TranslateForwardElevatorDoorGrp[1] = { GRP_elevator_door_forward };
    static MGROUP_TRANSLATE TranslateForwardElevatorDoor(0, TranslateForwardElevatorDoorGrp, 1, _V(0, 0, elevatorDoorTranslationZ));   // translate FORWARD

    static UINT TranslateAftElevatorDoorGrp[1] = { GRP_elevator_door_aft };
    static MGROUP_TRANSLATE TranslateAftElevatorDoor(0, TranslateAftElevatorDoorGrp, 1, _V(0, 0, -elevatorDoorTranslationZ));   // translate AFT

    // translate the elevator DOWN 
    const double elevatorTranslationY = -1.0;
    static UINT CrewElevatorGrp[] = { GRP_elevator_pod1, GRP_el_door_left_inside, GRP_el_door_right_inside, GRP_el_door_left_inside_glass, GRP_el_door_right_inside_glass,
                                      GRP_el_door_left_outside, GRP_el_door_right_outside, GRP_el_door_left_outside_glass, GRP_el_door_right_outside_glass, 
                                      GRP_elevator_pod_glass };
    static MGROUP_TRANSLATE TranslateElevator(0, CrewElevatorGrp, SizeOfGrp(CrewElevatorGrp), _V(0, elevatorTranslationY, 0)); 

    // rotate parent elevator arm down
    // PRE-1.3 RC2 (had 2.0-meter gear compression): const float elevatorRotation = static_cast<float>(39.5 * RAD);
    const float elevatorRotation = static_cast<float>(35.4 * RAD);   // TWEAKED for v1.5: was *slightly* off  the ground before
    static UINT ParentElevatorArmGrp[] = { GRP_el_arm_pistion_01, GRP_el_arm_cylinder_01 };
    static MGROUP_ROTATE RotateParentElevatorArm(0, ParentElevatorArmGrp, SizeOfGrp(ParentElevatorArmGrp), _V(0.190, -1.062, 6.105), _V(0, 0, 1.0), -elevatorRotation);

    // rotate child elevator arm down, which will be parented to the parent arm
    static UINT ChildElevatorArmGrp[] = { GRP_el_arm_pistion_00, GRP_el_arm_cylinder_00 };
    m_rotateChildElevatorArm = new MGROUP_ROTATE(0, ChildElevatorArmGrp, SizeOfGrp(ChildElevatorArmGrp), _V(5.525, -1.075, 5.407), _V(0, 0, 1.0), elevatorRotation * 2);

    // rotate elevator up, which will be parented to the child arm
    // Note: must account for the new rotation point here since the elevator translated and we don't want to deal with more parent components...
    m_rotateElevator = new MGROUP_ROTATE(0, CrewElevatorGrp, SizeOfGrp(CrewElevatorGrp), _V(-0.497, 0.105, 6.007), _V(0, 0, 1.0), static_cast<float>(-elevatorRotation));

    // 
    // Translate doors OUT to open them
    //
    // Note: the right/left group names are backwards in the mesh.
    //
    const float elevatorDoorWidth = 0.819701f;
    const float elevatorDoorZ = 0.1f;

    // translate inner right-side door and glass 1) +z elevatorDoorZ meter, and 2) OUT
    static UINT InnerRightElevatorDoorGrp[2] = { GRP_el_door_right_outside, GRP_el_door_right_outside_glass };
    static MGROUP_TRANSLATE InnerRightElevatorDoorZ(0, InnerRightElevatorDoorGrp, 2, _V(0, 0, elevatorDoorZ));
    static MGROUP_TRANSLATE InnerRightElevatorDoor (0, InnerRightElevatorDoorGrp, 2, _V(elevatorDoorWidth * 2, 0, 0));

    // translate outer right-side door and glass 
    static UINT OuterRightElevatorDoorGrp[2] = { GRP_el_door_right_inside, GRP_el_door_right_inside_glass };
    static MGROUP_TRANSLATE OuterRightElevatorDoor(0, OuterRightElevatorDoorGrp, 2, _V(elevatorDoorWidth, 0, 0));

    // translate inner left-side door and glass 1) +z elevatorDoorZ meter, and 2) OUT
    static UINT InnerLeftElevatorDoorGrp[2] = { GRP_el_door_left_outside, GRP_el_door_left_outside_glass };
    static MGROUP_TRANSLATE InnerLeftElevatorDoorZ(0, InnerLeftElevatorDoorGrp, 2, _V(0, 0, elevatorDoorZ));
    static MGROUP_TRANSLATE InnerLeftElevatorDoor (0, InnerLeftElevatorDoorGrp, 2, _V(-elevatorDoorWidth * 2, 0, 0));

    // translate outer left-side door and glass 
    static UINT OuterLeftElevatorDoorGrp[2] = { GRP_el_door_left_inside, GRP_el_door_left_inside_glass };
    static MGROUP_TRANSLATE OuterLeftElevatorDoor(0, OuterLeftElevatorDoorGrp, 2, _V(-elevatorDoorWidth, 0, 0));

    //
    // Define the crew elevator animation
    //
    anim_crewElevator = CreateAnimation(0);

    // translate the elevator DOWN
    AddAnimationComponent(anim_crewElevator, 0.125, 0.25, &TranslateElevator);

    // open/translate the doors
    AddAnimationComponent(anim_crewElevator, 0, 0.0925, &TranslateElevatorDoorsUp);        // translate main doors UP

    ANIMATIONCOMPONENT_HANDLE parentArmDoorHandle = AddAnimationComponent(anim_crewElevator, 0, 0.25, &RotateAftElevatorArmDoor);  // rotate aft arm door OPEN
    AddAnimationComponent(anim_crewElevator, 0, 0.25, m_forwardElevatorArmDoor, parentArmDoorHandle);   // rotate forward door OPEN by folding it onto parent door 

    AddAnimationComponent(anim_crewElevator, 0.0925, 0.185, &TranslateForwardElevatorDoor);  // translate main doors BACK/FORWARD
    AddAnimationComponent(anim_crewElevator, 0.0925, 0.185, &TranslateAftElevatorDoor); 

    // rotate the parent arm, child arm, and elevator pod down
    ANIMATIONCOMPONENT_HANDLE parentArmElevatorArmHandle = AddAnimationComponent(anim_crewElevator, 0.25, 0.85, &RotateParentElevatorArm);   
    ANIMATIONCOMPONENT_HANDLE childArmElevatorArmHandle  = AddAnimationComponent(anim_crewElevator, 0.25, 0.85, m_rotateChildElevatorArm, parentArmElevatorArmHandle);
    ANIMATIONCOMPONENT_HANDLE elevatorPodHandle          = AddAnimationComponent(anim_crewElevator, 0.25, 0.85, m_rotateElevator, childArmElevatorArmHandle);

    // translate the inner doors +Z
    AddAnimationComponent(anim_crewElevator, 0.85, 0.88, &InnerRightElevatorDoorZ);
    AddAnimationComponent(anim_crewElevator, 0.85, 0.88, &InnerLeftElevatorDoorZ);

    // translate the inner doors OUT
    AddAnimationComponent(anim_crewElevator, 0.88, 1.0, &InnerRightElevatorDoor);
    AddAnimationComponent(anim_crewElevator, 0.88, 1.0, &InnerLeftElevatorDoor);

    // translate the outer doors OUT
    AddAnimationComponent(anim_crewElevator, 0.94, 1.0, &OuterRightElevatorDoor);
    AddAnimationComponent(anim_crewElevator, 0.94, 1.0, &OuterLeftElevatorDoor);
#endif
}

// delete any child animation objects; invoked by our destructor
void XR3Phoenix::CleanUpAnimations()
{
    // TODO: enable these as animations are added
#if 0
    delete m_rad_panel_right_002;
	delete m_rad_panel_right_003;
    delete m_rad_panel_right_004;
    delete m_rad_panel_left_002;
	delete m_rad_panel_left_003;
    delete m_rad_panel_left_004;

    delete m_radiator_door_top_right;
    delete m_radiator_door_top_left;

    delete m_gear_door_left_outside_2;
	delete m_gear_door_right_outside_2;

    delete m_noseGearNoMovement;

    delete m_noseGearTranslation;
    delete m_rearGearLeftTranslation;
    delete m_rearGearRightTranslation;

    delete m_rearLeftRearRotationF;
    delete m_rearRightRearRotationF;
    delete m_rearLeftRearRotationB;
    delete m_rearRightRearRotationB;
    
    delete m_frontWheelRotation;

    delete m_forwardElevatorArmDoor;
    delete m_rotateChildElevatorArm;
    delete m_rotateElevator;

    delete m_dock_port00;
    delete m_dock_port01;
    delete m_dock_port_ring;

    // door petals
    for (int i=0; i < 8; i++)
        delete m_door_petal[i];

    // no need to invoke DelAnimation; all the data handles are going away anyway
#endif
}
