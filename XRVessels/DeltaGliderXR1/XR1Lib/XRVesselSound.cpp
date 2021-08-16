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
// Handles XR vessel custom sound and info messages
// ==============================================================

#include "DeltaGliderXR1.h"

// invoked during vessel initialization
// Returns: true if init successful, false if XRSound not loaded
bool DeltaGliderXR1::InitSound()
{
#if 0 // {XXX} ONLY SET THIS TO 1 IF YOU WANT TO TEST XRSOUND 2.0'S MODULE FUNCTIONALITY
#ifndef _DEBUG 
#error INVALID CONFIGURATION: fix your XRVessel.cpp's InitSound()
#endif
    m_pXRSound = XRSound::CreateInstance("XRSoundModuleTesting");
#else
    m_pXRSound = XRSound::CreateInstance(this);
#endif

    // check that XRSound is installed and warn the user if it is not
    if (!m_pXRSound->IsPresent())
    {
        // Note: do not blink a warning on the HUD or the debug line here because some users may want to 
        // fly without XRSound loaded.
        GetXR1Config()->WriteLog("WARNING: XRSound not installed or is a different XRSound.dll version from what this XR vessel version was built with: custom sound effects will not play.");
        ShowWarning(nullptr, DeltaGliderXR1::ST_None, "WARNING: XRSound not installed!&Custom sounds will not play.", true);  // warn the user
        return false;
    }

    // write the XRSound version to the log
    float xrSoundVersion = m_pXRSound->GetVersion();
    char msg[256];
    sprintf(msg, "Using XRSound version: %.2f", xrSoundVersion);
    GetXR1Config()->WriteLog(msg);

    // disable any default XRSounds that we implement ourselves here via code
    XRSoundOnOff(XRSound::AudioGreeting, false);
    XRSoundOnOff(XRSound::SwitchOn, false);
    XRSoundOnOff(XRSound::SwitchOff, false);

    XRSoundOnOff(XRSound::Rotation, false);
    XRSoundOnOff(XRSound::Translation, false);
    XRSoundOnOff(XRSound::Off, false);

    XRSoundOnOff(XRSound::AFOff, false);
    XRSoundOnOff(XRSound::AFPitch, false);
    XRSoundOnOff(XRSound::AFOn, false);

    XRSoundOnOff(XRSound::Crash, false);
    XRSoundOnOff(XRSound::MetalCrunch, false);
    XRSoundOnOff(XRSound::Touchdown, false);
    XRSoundOnOff(XRSound::OneHundredKnots, false);
    XRSoundOnOff(XRSound::Liftoff, false);
    XRSoundOnOff(XRSound::WheelChirp, false);
    XRSoundOnOff(XRSound::WheelStop, false);
    XRSoundOnOff(XRSound::TiresRolling, false);
    XRSoundOnOff(XRSound::WarningGearIsUp, false);
    XRSoundOnOff(XRSound::YouAreClearedToLand, false);

    XRSoundOnOff(XRSound::MachCalloutsGroup, false);
    XRSoundOnOff(XRSound::AltitudeCalloutsGroup, false);
    XRSoundOnOff(XRSound::DockingDistanceCalloutsGroup, false);

    XRSoundOnOff(XRSound::DockingCallout, false);
    XRSoundOnOff(XRSound::UndockingCallout, false);

    XRSoundOnOff(XRSound::AutopilotOn, false);
    XRSoundOnOff(XRSound::AutopilotOff, false);

    XRSoundOnOff(XRSound::SubsonicCallout, false);
    XRSoundOnOff(XRSound::SonicBoom, false);

    // load sounds
    LoadXR1Sound(SwitchOn, "SwitchOn1.wav", XRSound::PlaybackType::InternalOnly);
    LoadXR1Sound(SwitchOff, "SwitchOff1.wav", XRSound::PlaybackType::InternalOnly);
    LoadXR1Sound(Off, "Off.wav", XRSound::PlaybackType::Radio);
    LoadXR1Sound(Rotation, "Rotation.wav", XRSound::PlaybackType::Radio); // so it's always audible outside the ship 
    LoadXR1Sound(Translation, "Translation.wav", XRSound::PlaybackType::Radio); // so it's always audible outside the ship 
    LoadXR1Sound(Error1, "Error1.wav", XRSound::PlaybackType::Radio); // so it's always audible outside the ship (just in case we use it for something)
    LoadXR1Sound(OneHundredKnots, "100 Knots.wav", XRSound::PlaybackType::Radio);
    LoadXR1Sound(V1, "V1.wav", XRSound::PlaybackType::Radio);
    LoadXR1Sound(Rotate, "Rotate.wav", XRSound::PlaybackType::Radio);
    LoadXR1Sound(GearUp, "Gear Up.wav", XRSound::PlaybackType::Radio);     // 10
    LoadXR1Sound(GearDown, "Gear Down.wav", XRSound::PlaybackType::Radio);
    // GearLocked reused on-the-fly
    LoadXR1Sound(Pitch, "Pitch.wav", XRSound::PlaybackType::Radio);
    LoadXR1Sound(On, "On.wav", XRSound::PlaybackType::Radio);
    LoadXR1Sound(BeepHigh, "BeepHigh.wav", XRSound::PlaybackType::InternalOnly);
    LoadXR1Sound(BeepLow, "BeepLow.wav", XRSound::PlaybackType::InternalOnly);
    LoadXR1Sound(AutopilotOn, "Autopilot On.wav", XRSound::PlaybackType::InternalOnly);
    LoadXR1Sound(AutopilotOff, "Autopilot Off.wav", XRSound::PlaybackType::InternalOnly);
    LoadXR1Sound(RetroDoorsAreClosed, "Retro doors are closed.wav", XRSound::PlaybackType::InternalOnly);
    // slot 20 = MachCallout
    // slot 21 = AltitudeCallout
    LoadXR1Sound(SonicBoom, "Sonic Boom.wav", XRSound::PlaybackType::BothViewFar);
    // slot 23 = Ambient    (no longer used here; XRSound handles it)
    // slot 24 = Warning
    // slot 25 = Info
    LoadXR1Sound(ScramJet, "ScramJet.wav", XRSound::PlaybackType::BothViewFar);
    LoadXR1Sound(WarningBeep, "Warning Beep.wav", XRSound::PlaybackType::InternalOnly);
    LoadXR1Sound(GearWhine, "Gear Whine.wav", XRSound::PlaybackType::InternalOnly);
    LoadXR1Sound(GearLockedThump, "Gear Locked Thump.wav", XRSound::PlaybackType::InternalOnly);
    LoadXR1Sound(Crash, "Crash.wav", XRSound::PlaybackType::BothViewFar);
    LoadXR1Sound(ErrorSoundFileMissing, "Error Sound File Missing.wav", XRSound::PlaybackType::BothViewFar);  // debugging only
    LoadXR1Sound(FuelResupply, "Fuel Flow.wav", XRSound::PlaybackType::InternalOnly);
    LoadXR1Sound(FuelCrossFeed, "Fuel Flow.wav", XRSound::PlaybackType::InternalOnly);
    LoadXR1Sound(FuelDump, "Fuel Flow.wav", XRSound::PlaybackType::InternalOnly);
    LoadXR1Sound(SupplyHatch, "Door Opened Thump.wav", XRSound::PlaybackType::InternalOnly);
    LoadXR1Sound(HoverDoorsAreClosed, "Hover doors are closed.wav", XRSound::PlaybackType::InternalOnly);
    LoadXR1Sound(ScramDoorsAreClosed, "SCRAM doors are closed.wav", XRSound::PlaybackType::InternalOnly);
    LoadXR1Sound(Chamber, "Airlock.wav", XRSound::PlaybackType::InternalOnly);
    LoadXR1Sound(WheelChirp, "Wheel Chirp.wav", XRSound::PlaybackType::BothViewClose);
    LoadXR1Sound(TiresRolling, "Tires Rolling.wav", XRSound::PlaybackType::BothViewClose);

    return true;
}

// load a WAV file for XRSound to use
void DeltaGliderXR1::LoadXR1Sound(const Sound sound, const char* pFilename, XRSound::PlaybackType playbackType)
{
    if (!m_pXRSound->IsPresent())
        return;

    // use member variable here so we can preserve the last file loaded for debugging purposes
    sprintf(m_lastWavLoaded, "%s\\%s", m_pXRSoundPath, pFilename);
    BOOL stat = m_pXRSound->LoadWav(sound, m_lastWavLoaded, playbackType);
#ifdef _DEBUG
    if (!stat)
        sprintf(oapiDebugString(), "ERROR: LoadXR1Sound: LoadWav failed, filename='%s'", pFilename);
#endif
}

// play a sound via the XRSound SDK
// volume default=255  -- NOTE: only applies if soundType == ST_Other; otherwise, volume is set AudioCalloutVolume config setting.
// loop default=NOLOOP (other value is LOOP)
void DeltaGliderXR1::PlaySound(Sound sound, const SoundType soundType, int volume, bool bLoop)
{
    if (!m_pXRSound->IsPresent())
        return;

    // if we are not in focus, do not play the sound since it would fail anyway
    if (HasFocus() == false)
        return;

#ifdef _DEBUG
    // sanity check if debugging
    if ((soundType == ST_None) && (sound != NO_SOUND))
    {
        char temp[512];
        sprintf(temp, "INTERNAL ERROR: PlaySound: ST_None specified for non-null sound=%d : m_lastWavLoaded=[%s]", sound, m_lastWavLoaded);
        strcpy(oapiDebugString(), temp);
        // fall through and play the sound
    }
#endif

    if (soundType != ST_Other)
        volume = GetXR1Config()->AudioCalloutVolume;  // overrides any requested audio callout volume

    // now check whether the user wants to play this type of callout
    bool playSound;
    switch (soundType)
    {
    case ST_AudioStatusGreeting:
        playSound = GetXR1Config()->EnableAudioStatusGreeting;
        break;

    case ST_VelocityCallout:
        playSound = GetXR1Config()->EnableVelocityCallouts;
        break;

    case ST_AltitudeCallout:
        playSound = GetXR1Config()->EnableAltitudeCallouts;
        break;

    case ST_DockingDistanceCallout:
        playSound = GetXR1Config()->EnableDockingDistanceCallouts;
        break;

    case ST_InformationCallout:
        playSound = GetXR1Config()->EnableInformationCallouts;
        break;

    case ST_RCSStatusCallout:
        playSound = GetXR1Config()->EnableRCSStatusCallouts;
        break;

    case ST_AFStatusCallout:
        playSound = GetXR1Config()->EnableAFStatusCallouts;
        break;

    case ST_WarningCallout:
        playSound = GetXR1Config()->EnableWarningCallouts;
        break;

    case ST_Other:
        playSound = true;  // sound effects *always* play
        break;

    default:
        // should never happen!  (ST_None should never come through here!)
        playSound = true;   // play the sound anyway

        // only show an error during development
#ifdef _DEBUG   
        char temp[512];
        sprintf(temp, "ERROR: PlaySound: Unknown Soundtype value (%d) for sound=%d : m_lastWavLoaded=[%s]", soundType, sound, m_lastWavLoaded);
        strcpy(oapiDebugString(), temp);
#endif
        break;
    }

    if (playSound == false)
        return;     // user doesn't want the sound to play

    // play the sound!
    const float volFrac = min(static_cast<float>(volume) / 255.0f, 1.0f);  // convert legacy volume 0-255 to 0-1.0.
    BOOL stat = m_pXRSound->PlayWav(sound, bLoop, volFrac);

    // We don't want "missing wave file" errors showing up for users; they may want to delete
    // some sound files because they don't like them, so we don't want to clutter the log with
    // useless messages.  We only need this during development.
#ifdef _DEBUG
    if (stat == FALSE)
    {
        char temp[512];
        sprintf(temp, "ERROR: PlaySound: PlayWav failed, sound=%d : m_lastWavLoaded=[%s]", sound, m_lastWavLoaded);
        strcpy(oapiDebugString(), temp);

        // also write to the log
        GetXR1Config()->WriteLog(temp);

        // now let's play an audible alert, too
        m_pXRSound->PlayWav(ErrorSoundFileMissing);
    }
#endif
}

// stop a sound via the XRSound SDK
void DeltaGliderXR1::StopSound(Sound sound)
{
    if (!m_pXRSound->IsPresent())
        return;

    // if we are not in focus, do not stop the sound since it would fail anyway
    if (HasFocus() == false)
        return;

    // OK if sound is already stopped here
    m_pXRSound->StopWav(sound);
}

// check whether the specified sound is playing
bool DeltaGliderXR1::IsPlaying(Sound sound) const
{
    if (!m_pXRSound->IsPresent())
        return false;

    return m_pXRSound->IsWavPlaying(sound);
}

// play a warning sound and display a warning message via the DisplayWarningPoststep
// pSoundFilename may be null or empty; pMessage may be null
// NOTE: specific component damage may be determined by polling the lwingstatus, etc.
// if force == true, always play the incoming wav
void DeltaGliderXR1::ShowWarning(const char* pSoundFilename, const SoundType soundType, const char* pMessage, bool force)
{
    if (IsCrashed())  // show if incapacitated
        return;       // no more warnings

    if (pMessage != nullptr)
    {
        // display warning message only IF it was not the last warning displayed
        if (strcmp(pMessage, m_lastWarningMessage) != 0)    // strings do not match?
        {
            // add to the info/warning text line vector
            m_infoWarningTextLineGroup.AddLines(pMessage, true);  // text is highlighted

            // save for check next time
            strcpy(m_lastWarningMessage, pMessage);
        }
    }

    // the poststep will pick this sound at the next timestep and play it within 5 seconds
    if (pSoundFilename != nullptr)
    {
        _ASSERTE(soundType != ST_None);
        strcpy(m_warningWavFilename, pSoundFilename);
        m_warningWaveSoundType = soundType;
    }
    else
    {
        _ASSERTE(soundType == ST_None);
    }
}

// play an info sound and display an info message via the DisplayWarningPoststep
// pSoundFilename and/or pMessage may be null
void DeltaGliderXR1::ShowInfo(const char* pSoundFilename, const SoundType soundType, const char* pMessage)
{
    if (IsCrashed())  // DO show if incapacitated
        return;     // no more messages

    // check whether a new info message has been set
    if (pMessage != nullptr)
    {
        // add to the info/warning text line vector
        m_infoWarningTextLineGroup.AddLines(pMessage, false);  // text is not highlighted
    }

    // play the info sound, if any
    // Info sounds are relatively infrequent, so no need for a PostStep to manage it
    if ((pSoundFilename != nullptr) && (*pSoundFilename != 0))
    {
        LoadXR1Sound(Info, pSoundFilename, XRSound::PlaybackType::Radio);
        PlaySound(Info, soundType);
    }

    // Clear the last warning message value so that the same warning can be displayed again;
    // this is so that the warning will always be printed again after an info message is displayed.
    *m_lastWarningMessage = 0;
}

// Play the error beep and kill any switch and key sounds in progress
void DeltaGliderXR1::PlayErrorBeep()
{
    // stop any switch or key sounds that may have been started
    m_pXRSound->StopWav(SwitchOn);
    m_pXRSound->StopWav(SwitchOff);
    m_pXRSound->StopWav(BeepHigh);
    m_pXRSound->StopWav(BeepLow);

    PlaySound(Error1, ST_Other, ERROR1_VOL);      // error beep
}

// Play a door opening/closing beep; usually invoked from key handlers
void DeltaGliderXR1::PlayDoorSound(DoorStatus doorStatus)
{
    if (doorStatus == DoorStatus::DOOR_OPENING)
        PlaySound(_DoorOpening, ST_Other);
    else if (doorStatus == DoorStatus::DOOR_CLOSING)
        PlaySound(_DoorClosing, ST_Other);
}

// plays "Gear up and locked" or "Gear down and locked"
void DeltaGliderXR1::PlayGearLockedSound(bool isGearUp)
{
    const char* pFilename = (isGearUp ? "Gear Up And Locked.wav" : "Gear Down And Locked.wav");
    LoadXR1Sound(GearLocked, pFilename, XRSound::PlaybackType::Radio);
    PlaySound(GearLocked, ST_InformationCallout);
}
