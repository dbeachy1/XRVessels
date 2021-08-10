// ==============================================================
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// Copyright 2006-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// SubclassPreSteps.cpp
// Prestep methods only used by subclasses; these are not used by the XR1.
// ==============================================================

#include "DeltaGliderXR1.h"
#include "XR1PreSteps.h"
#include "AreaIDs.h"

//-------------------------------------------------------------------------

// Invokes RefreshGrappleTargetsInDisplayRange() method at regular intervals to handle the user 
// creating new vessels.  The vessel will invoke RefreshGrappleTargetsInDisplayRange() automatically each time 
// the pilot changes the range.
RefreshGrappleTargetsInDisplayRangePreStep::RefreshGrappleTargetsInDisplayRangePreStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel),
    m_lastUpdateSystemUptime(-1)
{
}

void RefreshGrappleTargetsInDisplayRangePreStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    if (GetXR1().IsCrashed())  
        return;     // nothing to do

    const double systemUptime = GetXR1().GetSystemUptime();  // *real-time*, not *simulation time*
    const double timeSinceLastUpdate = (systemUptime - m_lastUpdateSystemUptime);

    // For efficiency, only refresh once every second (Note: this does not affect how often the payload grapple screen is *udpdated*;
    // it only affects when new vessels are detected in range.
    if (timeSinceLastUpdate >= 1.0)
    {
        GetXR1().RefreshGrappleTargetsInDisplayRange();
        m_lastUpdateSystemUptime = systemUptime;
    }
}

// NOTE: remember to allow m_animFrontTireRotation and m_animRearTireRotation in your 
// subclass's SetXRAnimation method if you use this prestep

RotateWheelsPreStep::RotateWheelsPreStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel),
    m_noseWheelProc(0), m_rearWheelProc(0), 
    m_noseWheelRotationVelocity(0), m_rearWheelRotationVelocity(0)
{
}

void RotateWheelsPreStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // Note: must ALWAYS set animation here so that the wheels always follow the struts
    // even if the wheels are not rotating.  The animation states will be replaced later 
    // in this method if the wheels are actually rotating.
    GetXR1().SetXRAnimation(GetXR1().m_animFrontTireRotation, m_noseWheelProc);
    GetXR1().SetXRAnimation(GetXR1().m_animRearTireRotation,  m_rearWheelProc);

    if (GetXR1().IsCrashed())  
        return;     // nothing to do

    // Efficiency check: exit immediately if gear is retracted and has stopped spinning
    if ((GetXR1().gear_status == DOOR_CLOSED) && (m_noseWheelRotationVelocity == 0) && (m_rearWheelRotationVelocity == 0))
        return;  

    VECTOR3 gsVector;
	GetXR1().GetGroundspeedVector(FRAME_LOCAL, gsVector);
	const double groundSpeed = gsVector.z;  // in m/s; may be negative!

#if 0   // set to 1 for constant spin for testing
    m_noseWheelRotationVelocity = 50.0;
    m_rearWheelRotationVelocity = 30.0;
#else   // normal
    // figure out whether the wheels are on the ground
    const bool rearWheelsOnGround = GetXR1().IsRearGearOnGround();
    const bool frontWheelsOnGround = GetXR1().IsNoseGearOnGround();

    SetWheelRotVel(simdt, groundSpeed, frontWheelsOnGround, m_noseWheelRotationVelocity);
    SetWheelRotVel(simdt, groundSpeed, rearWheelsOnGround, m_rearWheelRotationVelocity);
#endif

    // animate the wheels if the rotation velocity has changed since the previous timestep
    SetXRAnimationForVelocity(simdt, GetXR1().m_animFrontTireRotation, m_noseWheelRotationVelocity, m_noseWheelProc, 1.0, FRONT_TIRE_CIRCUMFERENCE);
    SetXRAnimationForVelocity(simdt, GetXR1().m_animRearTireRotation,  m_rearWheelRotationVelocity, m_rearWheelProc, 1.0, REAR_TIRE_CIRCUMFERENCE); 

    // uncomment for debugging
    /*
    sprintf(oapiDebugString(), "groundSpeed=%f, fWheelVel: %lf, rWheelVel: %lf, rearWheelsOnGround=%d, frontWheelsOnGround=%d", static_cast<float>(groundSpeed),
        m_noseWheelRotationVelocity, m_rearWheelRotationVelocity, rearWheelsOnGround, frontWheelsOnGround);
    */
}

//-------------------------------------------------------------------------

// adjust/set wheel rotation velocity
// groundSpeed = meters/second; may be positive or negative
// isWheelOnGround = true if wheel is contacting the ground, false if not
// wheelRotationVelocity: value to be adjusted/set
void RotateWheelsPreStep::SetWheelRotVel(const double simdt, const double groundSpeed, const bool isWheelOnGround, double &wheelRotationVelocity)
{
    // add +/-20% randomness in here
    const double decelerationRate = TIRE_DECELERATION_RATE * (0.8 + (oapiRand() *.40));
    double tireSpinDecel = (decelerationRate * simdt);  // in m/s for this timestep
    if (wheelRotationVelocity < 0)
        tireSpinDecel = -tireSpinDecel;     // always move speed toward zero

    // Note: "rotation velocity" refers to the velocity of the edge of the tire; i.e., around its arc.
    // Also note that wheels may rotate forward or backward.

    // check whether the gear is down 
    if (isWheelOnGround)
    {
        wheelRotationVelocity = groundSpeed;     // on ground
    }
    else  // wheels are up -- let's see if they are still spinning down
    {
        // If wheel brakes engaged, stop rotation if airborne!  The brakes are very powerful and would stop rotation instantly.
        // Note: it would be nice to test left/right brakes here and just the one wheel, but we always rotate them in tandem.
        if ((GetVessel().GetWheelbrakeLevel(1) > 0) || (GetVessel().GetWheelbrakeLevel(2) > 0))
            wheelRotationVelocity = 0;      // brakes stop rotation instantly
        else if (fabs(wheelRotationVelocity) > 0)
        {
            // decelerate the wheels at a constant rate (drag is pretty much fixed)
            double orgRotVel = wheelRotationVelocity;
            wheelRotationVelocity -= tireSpinDecel;

            // if we just crossed zero, zero out rotation
            if (((orgRotVel > 0) && (wheelRotationVelocity < 0)) ||
                ((orgRotVel < 0) && (wheelRotationVelocity > 0)))
            {
                wheelRotationVelocity = 0;    // stopped rotating
            }
        }
    }
}

// Set a wheel's animation state based on the specified rotation velocity.
// animationHandle = handle of wheel to be rotated
// currentRotVel = current rotation velocity (may be negative)
// wheelProc = 0 < n < 1 : input/output: wheel animation state
// rotationFraction = 1.0 for normal speed, 0.5 = half-speed, etc.  (currently not used)
// wheelCircumference = just what it says...
void RotateWheelsPreStep::SetXRAnimationForVelocity(const double simdt, const UINT &animationHandle, const double currentRotVel, double &wheelProc, const double rotationFraction, const double wheelCircumference)
{
    // figure out how many tire revolutions have occured since the last timestep
    const double adjustedTireCircumference = wheelCircumference * 1.6523;     // correct for Orbiter not being 100% accurate in animating the mesh under the ship; i.e., "make the wheels look right when rotating"
    const double velocityDelta = currentRotVel * simdt;  // Note: may be negative!
    const double revolutionsDelta = velocityDelta / adjustedTireCircumference * rotationFraction; // tire revolutions since the previous timestep

    // add the revolutions to the current animation state; i.e., "spin the tire"
    wheelProc += revolutionsDelta;  // Note: revolutionsDelta may be negative

    // Now mod the wheelProc by 1.0 so we get a new tire rotation state between 0 and 1.
    // In other words, "throw out all the full revolutions of the tire and just render its current state".
    wheelProc = fmod(wheelProc, 1.0);

    // if wheelProc is negative, it means we are moving *backwards*, and so the state should be 1.0 + wheelProc.
    if (wheelProc < 0)
        wheelProc = 1.0 + wheelProc;

    GetXR1().SetXRAnimation(animationHandle, wheelProc);
}

//-------------------------------------------------------------------------

// animate the front and rear gear struts for touchdown compression
AnimateGearCompressionPreStep::AnimateGearCompressionPreStep(DeltaGliderXR1 &vessel) : 
    XR1PrePostStep(vessel),
    m_previousAltitude(-1)
{
}

void AnimateGearCompressionPreStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    if (GetXR1().IsCrashed())  
        return;     // nothing to do

    // sanity check, plus prevent compiler warning: "warning C4723: potential divide by 0" 
    if (GEAR_COMPRESSION_DISTANCE <= 0)
        return;

    // Only update animation state if gear is fully deployed.  Note: in theory it is possible for the pilot to
    // lower the gear just a few meters off the ground and to "push the gear below the ground" until they fully
    // extend, at which point they would "snap" up into the correct compression, but handling that absurdly rare
    // condition would make the already-complex math much worse since we would have to deal with angled and moving struts as well.
    if (GetXR1().gear_status != DOOR_OPEN)
    {
        GetXR1().m_noseGearProc = GetXR1().m_rearGearProc = 1.0;    // gear is fully uncompressed since gear not fully deployed yet
        return;
    }

    const double altitude = GetVessel().GetAltitude(ALTMODE_GROUND);  // altitude at the ship's centerpoint in meters

    // for efficiency, only recompute translation if the altitude has changed since the previous timestep
    if (altitude == m_previousAltitude)
        return;

    m_previousAltitude = altitude;
    const double pitch = GetVessel().GetPitch();  // in radians

    // Compute the length of the a and b legs of the front or rear strut triangle using a line parallel to the ground through the ship's centerpoint along the b leg
    // and the ship's centerline as the c leg (hypotenuse).  This will give us the all the data for the right triangle for these three lines:
    //   1) a line through the the ship's centerpoint (hypotenuse), or c
    //   2) a line through the front or rear strut extended through the centerpoint (altitude), or a
    //   3) a line parallel to the ground through the ship's centerpoint (base), or b
    const double c1 = NOSE_GEAR_ZCOORD;
    const double c2 = REAR_GEAR_ZCOORD;

    const double a1 = sin(pitch) * c1;
    // don't need this: const double b1 = cos(pitch) * c1;

    const double a2 = sin(pitch) * c2;   
    // don't need this: const double b2 = cos(-pitch) * c2;

    // At this point a1 and a2 will get us the delta from the ship's centerline (altitude) *assuming the strut projects straight toward the ground*, 
    // which is not the case unless the ship is perfectly level.  Therefore, we must solve for c in the adjacent right triangle.
    // Uppercase triangle leg varnames denote the second triangle we compute below.
    const double theta = (PI / 2) - pitch;   // A triangle's angles add up to 180 degrees (PI radians), so we can compute the other angle using the remainder.
    const double B1 = a1 / tan(theta);
    double C1 = sqrt((a1 * a1) + (B1 * B1));

    const double B2 = a2 / tan(theta);
    double C2 = sqrt((a2 * a2) + (B2 * B2));

    // Both C1 and C2 are positive at this point; however, if pitch is positive C2 needs to be *negative*
    // since it tilts the gear *down* toward the ground, and vice-versa.
    if (pitch > 0)
        C2 = -C2;
    else if (pitch < 0)
        C1 = -C1;
    
    // C1 and C2 contain the length of the line through the ship's centerpoint along it Z axis to the line through the ship's centerpoint parallel
    // to the ground for the front and rear struts, respectively.  Now we need to subtract the length of the fully uncompressed strut to get the distance that the 
    // bottom of the front and rear tires are away from ship's centerline along the Y axis.  
    // The distance remaining is the distance from the bottom of the tire to the ship's centerline.  
    // NOTE: one or both of these values may be (and probably will be!) negative, meaning the tires extend below the ship's centerline.
    const double frontGearDeltaYFromCenterline = C1 + GEAR_UNCOMPRESSED_YCOORD;   // GEAR_UNCOMPRESSED_YCOORD is negative, so we have to *add* here in order to subtract the distance
    const double rearGearDeltaYFromCenterline  = C2 + GEAR_UNCOMPRESSED_YCOORD;
    
    // Now we can get the distance the tire is from the ground by taking our altitude (at the centerpoint) plus the distance of the tire from the ship's centerline.
    // Remember that frontGearDeltaYFromCenterline and rearGearDeltaYFromCenterline will be negative unless the ship is pitched fairly high or low,
    // which would raise the gear above the line parallel to the ground through the centerpoint.
    double frontTireAltitude = altitude + frontGearDeltaYFromCenterline;
    double rearTireAltitude  = altitude + rearGearDeltaYFromCenterline;

    // Multiply front and rear gear translation distance ("altitude") by our "angled strut" factor.
    // i.e., if the strut deploys to a non-vertical angle we have to deploy slightly more than we would at 90 degrees;
    // This is because the hypotenuse is always longer than the altitude of a triangle.
    frontTireAltitude *= FRONT_GEAR_COMPRESSION_TRANSLATION_FACTOR;
    rearTireAltitude  *= REAR_GEAR_COMPRESSION_TRANSLATION_FACTOR;

    // distance in meters; limit to MAX gear compression
    double frontStrutCompressionDistance = min(-frontTireAltitude, GEAR_COMPRESSION_DISTANCE); 
    double rearStrutCompressionDistance  = min(-rearTireAltitude, GEAR_COMPRESSION_DISTANCE);

    // if no compression necessary (i.e., compression distance is negative), set to ZERO compression
    if (frontStrutCompressionDistance < 0)
        frontStrutCompressionDistance = 0;

    if (rearStrutCompressionDistance < 0)
        rearStrutCompressionDistance = 0;

    // One final note: technically we are still not 100% dead-accurate because the tire is round, and that will affect the contact 
    // distance slightly depending on how the ship is rotated.  The current calculations assume the touchdown point is directly in line with the
    // center strut, which will not be the case if the ship is pitched up or down.  However, this amount of error is very small and it is not 
    // worth the effort or computational time to compute the exact touchdown point along the arc of the tire.

    // Update the animation state fraction based on how far we need to compress the front and rear strut.  These states are stored in member variables 
    // so that other methods can read them. 1.0 = fully uncompressed, 0.0 = fully compressed.
    GetXR1().m_noseGearProc = 1.0 - (frontStrutCompressionDistance / GEAR_COMPRESSION_DISTANCE);
    GetXR1().m_rearGearProc = 1.0 - (rearStrutCompressionDistance  / GEAR_COMPRESSION_DISTANCE);

    // uncomment for debugging
    /*
    sprintf(oapiDebugString(), "altitude=%f, frontCompressionDist=%f, rearCompressionDist=%f, noseGearProc=%f, rearGearProc=%f", 
        static_cast<float>(altitude), static_cast<float>(frontStrutCompressionDistance), static_cast<float>(rearStrutCompressionDistance), static_cast<float>(GetXR1().m_noseGearProc), static_cast<float>(GetXR1().m_rearGearProc));
    */

    GetXR1().SetXRAnimation(GetXR1().m_animNoseGearCompression, GetXR1().m_noseGearProc);
    GetXR1().SetXRAnimation(GetXR1().m_animRearGearCompression, GetXR1().m_rearGearProc);
}

//-------------------------------------------------------------------------

// no longer used; handed by XRSound now
#if 0   
// This prestep will play main, hover, and RCS sounds to replace stock sounds.  
// Note: if you use this, you must disable the stock XRSound wav files in 
// your InitSound method.

// Constructor
// One or more of these Wav values may be null; only sounds whose values that are 
// non-null will be played.
EngineSoundsPreStep::EngineSoundsPreStep(DeltaGliderXR1 &vessel, const bool playMain, const bool playHover, const bool playRCS) : 
    XR1PrePostStep(vessel),
        m_playMain(playMain), m_playHover(playHover), m_playRCS(playRCS)
{
}

void EngineSoundsPreStep::clbkPrePostStep(const double simt, const double simdt, const double mjd)
{
    // main/retro engines
    if (m_playMain)
    {
        // main engines
        double totalThrustLevel = GetVessel().GetThrusterLevel(GetXR1().th_main[0]) + 
                                  GetVessel().GetThrusterLevel(GetXR1().th_main[1]);  // 0...2.0

        if (totalThrustLevel > 0)   // should sound be playing?
        {
            const double maxVolume = GetXR1().GetXR1Config()->CustomMainEngineSoundVolume;
            const double minVolume = maxVolume - 85;   // 170 at default max volume of 255
            const double variableVolume = (maxVolume - minVolume) * (totalThrustLevel / 2.0); 
            const double volume = minVolume + variableVolume;

            // OK if sound already playing here; if so, this call will merely change the volume
            GetXR1().PlaySound(GetXR1().MainEngines, DeltaGliderXR1::ST_Other, static_cast<int>(volume), true);  // loop forever
        }
        else    // sound should NOT be playing, so check the RETRO engines
        {
            // retro engines
            totalThrustLevel = GetVessel().GetThrusterLevel(GetXR1().th_retro[0]) + 
                GetVessel().GetThrusterLevel(GetXR1().th_retro[1]);  // 0...2.0

            if (totalThrustLevel > 0)   // should sound be playing?
            {
                // take 25 off max volume for retro engines
                const double minVolume = 150;
                const double variableVolume = (225 - minVolume) * (totalThrustLevel / 2.0); 
                const double volume = minVolume + variableVolume;

                // OK if sound already playing here; if so, this call will merely change the volume
                GetXR1().PlaySound(GetXR1().MainEngines, DeltaGliderXR1::ST_Other, static_cast<int>(volume), true);  // loop forever
            }
            else    // neither MAIN nor RETRO engines firing, so sound should NOT be playing
            {
                GetXR1().StopSound(GetXR1().MainEngines);
            }
        }
    }

    // hover engines
    if (m_playHover)
    {
        double totalThrustLevel = GetVessel().GetThrusterLevel(GetXR1().th_hover[0]) + 
                                  GetVessel().GetThrusterLevel(GetXR1().th_hover[1]);  // 0...2.0

        if (totalThrustLevel > 0)   // should sound be playing?
        {
            const double minVolume = 170;
            const double variableVolume = (255 - minVolume) * (totalThrustLevel / 2.0); 
            const double volume = minVolume + variableVolume;

            // OK if sound already playing here; if so, this call will merely change the volume
            GetXR1().PlaySound(GetXR1().HoverEngines, DeltaGliderXR1::ST_Other, static_cast<int>(volume), true);  // loop forever
        }
        else    // sound should NOT be playing
        {
            GetXR1().StopSound(GetXR1().HoverEngines);
        }
    }

    // RCS jets
    if (m_playRCS)
    {
        // Add up all the RCS thrusters: if TOTAL thrust >= 1.0, use full volume on RCS sustain.
        // RCS *attack*, however, always plays at full volume.
        double totalThrustLevel = 0;
        for (int rcsIndex = 0; rcsIndex < 14; rcsIndex++)
            totalThrustLevel += GetVessel().GetThrusterLevel(GetXR1().th_rcs[rcsIndex]); 

        if (totalThrustLevel > 1.0)
            totalThrustLevel = 1.0; // simultaneous RCS jets firing do not increase volume any further

        if (totalThrustLevel > 0)   // should sound be playing?
        {
            // if attack or sustain not ALREADY playing, play the RCS attack sound
            if ( (!GetXR1().IsPlaying(GetXR1().RCSAttack)) && (!GetXR1().IsPlaying(GetXR1().RCSSustain)) )
                GetXR1().PlaySound(GetXR1().RCSAttack, DeltaGliderXR1::ST_Other);  // max volume

            const double minVolume = 180; 
            const double variableVolume = (255 - minVolume) * totalThrustLevel; 
            const double volume = minVolume + variableVolume;

            // OK if sound already playing here; if so, this call will merely change the volume
            GetXR1().PlaySound(GetXR1().RCSSustain, DeltaGliderXR1::ST_Other, static_cast<int>(volume), true);  // loop forever
        }
        else    // sound should NOT be playing
        {
            // Note: we never terminate the RCS Attack sound: it always completes
            GetXR1().StopSound(GetXR1().RCSSustain);
        }
    }
}
#endif