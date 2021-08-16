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
// Handles XR vessel MMU logic
// ==============================================================

#include "DeltaGliderXR1.h"

// perform an EVA for the specified crew member
// Returns: true on success, false on error (crew member not present or outer airlock door is closed)
bool DeltaGliderXR1::PerformEVA(const int ummuCrewMemberIndex)
{
    bool retVal = false;

    // NOTE: the crew member should always be onboard here because we only display members
    // that are onboard the ship; therefore, we don't need to check for that here.

    if (CheckEVADoor() == false)
        return false;

#ifdef MMU
    // door is OK; retrieve data for crew member to perform EVA from UMMu
    const char* pCrewMemberNameUMMu = GetCrewNameBySlotNumber(ummuCrewMemberIndex);

    // must copy crew member name to our own buffer because UMmu will reuse the buffer below!
    char pCrewMemberName[40];       // matches UMmu name length
    strcpy(pCrewMemberName, pCrewMemberNameUMMu);

    if (strlen(pCrewMemberName) == 0)   // crew member not on board?
    {
        // should never happen!
        PlayErrorBeep();
        ShowWarning(NULL, DeltaGliderXR1::ST_None, "INTERNAL Mmu ERROR:&Crew member not on board!");
        return false;
    }

    // set the mesh for this crew member
    const char* pMesh = DEFAULT_CREW_MESH;  // assume custom mesh not set

    // UMmu bug: must cast away constness below
    const char* pMisc = GetCrewMiscIdByName(const_cast<char*>(pCrewMemberName));
    if (strlen(pMisc) > 0)
        pMesh = RetrieveMeshForUMmuMisc(pMisc); // use custom mesh

    UMmu.SetAlternateMeshToUseForEVASpacesuit(pMesh);

    // set O2 levels
    UMmu.SetO2ReserveWhenEvaing(100);   // default
    // Override from stupid default of 1000, which is CHEATING!  How can you suddenly jam 10x 
    // more O2 into your suit tanks just because you are ejecting?  It's not like you had 
    // time to "switch tanks."  Rubbish...
    UMmu.SetO2ReserveWhenEjecting(100);

    // use EjectCrewMember here if ship in flight in an atmosphere
    int evaStatus;
    if ((IsLanded() == false) && (GetAtmPressure() >= 1e3) && (GetAltitude(ALTMODE_GROUND) >= 20))
    {
        evaStatus = UMmu.EjectCrewMember(const_cast<char*>(pCrewMemberName));
    }
    else    // normal EVA
        evaStatus = UMmu.EvaCrewMember(const_cast<char*>(pCrewMemberName));

    if ((evaStatus == TRANSFER_TO_DOCKED_SHIP_OK) || (evaStatus == EVA_OK))
    {
        // EVA successful!  No need to remove the crew member manually since UMmu will do it for us.

        SetPassengerVisuals();     // update the VC mesh

        if (IsDocked() && (m_pActiveAirlockDoorStatus == &olock_status))
        {
            char temp[100];
            sprintf(temp, "%s transferred&to docked vessel successfully!", pCrewMemberName);
            ShowInfo("Crew Member Transferred Successfully.wav", ST_InformationCallout, temp);
        }
        else  // not docked or docking port not active
        {
            char temp[100];
            sprintf(temp, "%s on EVA.", pCrewMemberName);
            ShowInfo("Egress Successful.wav", ST_InformationCallout, temp);
            retVal = true;   // success
        }
        retVal = true;
    }
    else if (evaStatus == ERROR_DOCKED_SHIP_HAVE_AIRLOCK_CLOSED)
    {
        PlayErrorBeep();
        ShowWarning("Warning Docked Ship's Outer Door is Closed.wav", ST_WarningCallout, "Crew transfer failed:&Docked ship's airlock is closed.");
    }
    else if (evaStatus == ERROR_DOCKED_SHIP_IS_FULL)
    {
        PlayErrorBeep();
        ShowWarning("Warning Docked Ship Has a Full Complement.wav", ST_WarningCallout, "Cannot transfer crew: Docked&ship has a full crew complement.");
    }
    else if (evaStatus == ERROR_DOCKEDSHIP_DONOT_USE_UMMU)
    {
        PlayErrorBeep();
        ShowWarning("Warning Crew Member Transfer Failed.wav", ST_WarningCallout, "Docked ship does not support UMmu!");
    }
    else    // other UMmu error
#endif
    {
        if (IsDocked())
        {
            PlayErrorBeep();
            ShowWarning("Warning Crew Member Transfer Failed.wav", ST_WarningCallout, "Crew member transfer failed.");
        }
        else    // internal error!  Should never happen!
        {
            PlayErrorBeep();
            ShowWarning(NULL, DeltaGliderXR1::ST_None, "INTERNAL Mmu ERROR: EVA FAILED");
        }
    }

    return retVal;
}

// returns: true if EVA doors are OK, false if not
// Note: this is also invoked for turbopack deployment/stowage 
bool DeltaGliderXR1::CheckEVADoor()
{
    // NOTE: we do not enforce the sequence of open inner door -> close inner door ->
    // depressurize -> open outer door for EVA because that is too tedious, plus the pilot
    // can still do that if he wants to.  We merely require that the outer door is open, which will require the
    // pilot to equalize the airlock pressure and open the outer door and nosecone first.

    // verify that the outer door and nosecone are both open
    // We really wouldn't have to check for nosecone here, since the outer door already requires that
    // the nosecone be open before the outer door can open; however, we want to give the pilot an accurate callout.
    if (nose_status != DoorStatus::DOOR_OPEN)
    {
        PlayErrorBeep();
        char msg[128];
        sprintf(msg, "%s is closed.", NOSECONE_LABEL);
        ShowWarning(WARNING_NOSECONE_IS_CLOSED_WAV, ST_WarningCallout, msg);
        return false;
    }

    if (olock_status != DoorStatus::DOOR_OPEN)
    {
        PlayErrorBeep();
        ShowWarning("Warning Outer Door is Closed.wav", ST_WarningCallout, "Outer door is closed.");
        return false;
    }

    return true;
}

// extract a crew member's rank from the Mmu 'misc' field
// This returns a pointer to a static working buffer.  If pMisc is not from an
// XR1 crew member, the Misc ID itself is returned.
const char* DeltaGliderXR1::RetrieveRankForMmuMisc(const char* pMisc) const
{
    const char* retVal = pMisc;   // assume non-XR1 misc ID

    int index = ExtractIndexFromMmuMisc(pMisc);
    if (index >= 0)
    {
        CrewMember* pCM = GetXR1Config()->CrewMembers + index;
        retVal = pCM->rank;
    }

    return retVal;
}

// extract a crew member's mesh from the Mmu 'misc' field
// If pMisc is not from an XR1 crew member, the default Mmu mesh is returned.
const char* DeltaGliderXR1::RetrieveMeshForMmuMisc(const char* pMisc) const
{
    const char* retVal = DEFAULT_CREW_MESH;   // assume non-XR1 misc ID

    int index = ExtractIndexFromMmuMisc(pMisc);
    if (index >= 0)
    {
        CrewMember* pCM = GetXR1Config()->CrewMembers + index;
        retVal = pCM->mesh;
    }

    return retVal;
}

// extract a crew index (0..n) from the supplied Mmu 'misc' field, or -1 if pMisc is invalid
int DeltaGliderXR1::ExtractIndexFromMmuMisc(const char* pMisc)
{
    int index = -1;

    if ((strlen(pMisc) > 2) && (*pMisc == 'X') && (pMisc[1] == 'I'))  // "XI0", "XI1", etc.
        sscanf(pMisc + 2, "%d", &index);   // skip leading "XR"

    return index;
}

// obtain the UMmu crew member slot number for the given name
// Returns: 0...n on success, or -1 if name is invalid
int DeltaGliderXR1::GetMmuSlotNumberForName(const char* pName) const
{
    int retVal = -1;    // assume not found

    for (int i = 0; i < MAX_PASSENGERS; i++)
    {
#ifdef MMU
        const char* pUMmuName = CONST_UMMU(this).GetCrewNameBySlotNumber(i);
        if (strcmp(pName, pUMmuName) == 0)
        {
            retVal = i;     // found the slot!
            break;
        }
#endif
    }

    return retVal;
}

// returns true if Mmu crew member is on board or false if not
bool DeltaGliderXR1::IsCrewMemberOnBoard(const int index) const
{
#ifdef MMU
    const char* pName = CONST_UMMU(this).GetCrewNameBySlotNumber(index);
    bool retVal = (strlen(pName) > 0);

    return retVal;
#else
    return true;
#endif
}

// NOTE: crew is treated as incapacitated if no one is on board!
// Returns true if crew is dead or cannot operate the ship, or false if at least one member is OK and can pilot the ship.
bool DeltaGliderXR1::IsCrewIncapacitatedOrNoPilotOnBoard() const
{
    // normal checks first
    bool retVal = IsCrewIncapacitated();

    // check whether a pilot must be on board in order to fly the ship
    if (GetXR1Config()->RequirePilotForShipControl)
    {
        // check for 'Commander' and 'Pilot' ranks
        if (!IsPilotOnBoard())
            retVal = true;    // nobody on board who can fly the ship
    }

    return retVal;
}

// Returns true if a pilot is on board *or* 'RequirePilotForShipControl=false' and at least
// *one* crew member is on board AND the crew is OK.
bool DeltaGliderXR1::IsPilotOnBoard() const
{
#ifdef MMU
    // normal checks first
    bool retVal = (IsCrewRankOnBoard("Commander") || IsCrewRankOnBoard("Pilot"));

    // If 'RequirePilotForShipControl=false' the ship is *always* flyable as long as at least
    // on crew member is on board and the crew is still OK.
    if (GetXR1Config()->RequirePilotForShipControl == false)
        retVal = ((GetCrewMembersCount() > 0) && (m_crewState == OK));

    return retVal;
#else
    return true;
#endif
}

// Returns true if one or more crew members with the specified rank are on board, false otherwise
// pTargetRank is case-sensitive; e.g., "Commander"
bool DeltaGliderXR1::IsCrewRankOnBoard(const char* pTargetRank) const
{
#ifdef MMU
    bool retVal = false;   // assume not on board
    char pRank[CrewMemberRankLength + 1];
    for (int i = 0; i < MAX_PASSENGERS; i++)
    {
        const char* pUmmuMisc = CONST_UMMU(this).GetCrewMiscIdBySlotNumber(i);
        const bool crewMemberOnBoard = (strlen(pUmmuMisc) > 0);

        if (crewMemberOnBoard)
        {
            // check the crew member's rank (case-sensitive)
            strncpy(pRank, RetrieveRankForUMmuMisc(pUmmuMisc), CrewMemberRankLength);
            // check for commander and pilot by RANK (case-sensitive)
            if (strcmp(pRank, pTargetRank) == 0)
            {
                retVal = true;
                break;
            }
        }
    }
    return retVal;
#else
    return true;
#endif
}

#ifdef MMU
// deploy a new instance of the currently-selected turbopack
void DeltaGliderXR1::DeployTurbopack()
{
    if (CheckEVADoor() == false)
        return;     // cannot deploy turbopack

    const Turbopack* pSelectedTurbopack = TURBOPACKS_ARRAY + m_selectedTurbopack;

    // WARNING: TURBOPACK VESSEL NAMES MUST BE UNIQUE!
    // Define the new vessel's name as: vesselClassname-index; e.g., XR2turbopackKara-1
    // Loop until we find a unique name.
    char childName[128];
    for (int subIndex = 1; subIndex < 10000; subIndex++)   // 10000 is for sanity check
    {
        sprintf(childName, "%s-%d", pSelectedTurbopack->Classname, subIndex);

        // check whether vessel already exists
        OBJHANDLE hExistingVessel = oapiGetVesselByName(childName);
        if (oapiIsVessel(hExistingVessel) == false)
            break;      // name is unique in this scenario
    }

    // Clone from our vessel's status initially.
    VESSELSTATUS2 childVS;
    GetStatusSafe(*this, childVS, true);  // reset fields

    // move the child (turbopack) to the deployToCoordinates by converting them (as a delta) from parent-local to GLOBAL coordinates
    VECTOR3 globalChildDeltaCoords;
    GlobalRot(TURBOPACK_SPAWN_COORDINATES, globalChildDeltaCoords);
    childVS.rpos += globalChildDeltaCoords;
    childVS.status = 0;                  // set to FREEFLIGHT

    OBJHANDLE hChild = oapiCreateVesselEx(childName, pSelectedTurbopack->Classname, &childVS);
    if (hChild == nullptr)
    {
        // should never happen!
        PlayErrorBeep();
        ShowWarning(NULL, DeltaGliderXR1::ST_None, "Warning: turbopack vessel&creation failed!");
        return;
    }

    // move the turbopack to its deploy location
    VESSEL* pChild = oapiGetVesselInterface(hChild);
    pChild->DefSetStateEx(&childVS);

    char temp[64];
    sprintf(temp, "%s deployed.", childName);
    ShowInfo("BeepHigh.wav", ST_Other, temp);
}

//------------------------------------------------------------------------

// stow all turbopacks within STOW_TURBOPACK_DISTANCE meters of the ship
void DeltaGliderXR1::StowAllTurbopacks()
{
    if (CheckEVADoor() == false)
        return;     // cannot stow turbopack

    int stowedCount = 0;    // # of turbopacks stowed

    // loop through all vessels in the sim and check each vessel's classname and distance
    const DWORD dwVesselCount = oapiGetVesselCount();
    for (DWORD i = 0; i < dwVesselCount; i++)
    {
        OBJHANDLE hVessel = oapiGetVesselByIndex(i);
        if (hVessel != nullptr)    // should never happen, but just in case
        {
            VESSEL* pVessel = oapiGetVesselInterface(hVessel);
            const char* pClassname = pVessel->GetClassName();
            // WARNING: some vessel classnames can be null, such as Mir!
            if (pClassname != nullptr)
            {
                // check this vessel's distance from our vessel
                const double candidateVesselDistance = GetDistanceToVessel(*pVessel);
                if (candidateVesselDistance <= STOW_TURBOPACK_DISTANCE)
                {
                    // candidate vessel is in range; check its class for a match with one of our turbopack types
                    for (int i = 0; i < TURBOPACKS_ARRAY_SIZE; i++)
                    {
                        const Turbopack* pTurbopack = TURBOPACKS_ARRAY + i;
                        if (strcmp(pClassname, pTurbopack->Classname) == 0)
                        {
                            // classname is a match!  Delete ("stow") the vessel.
                            oapiDeleteVessel(hVessel);
                            stowedCount++;
                        }
                    }
                }
            }
        }
    }

    if (stowedCount == 0)
    {
        PlayErrorBeep();
        ShowWarning(NULL, DeltaGliderXR1::ST_None, "No turbopacks in range.");
    }
    else
    {
        char temp[64];
        const char* s = ((stowedCount == 1) ? "" : "s");
        sprintf(temp, "%d turbopack%s stowed.", stowedCount, s);
        ShowInfo("BeepHigh.wav", ST_Other, temp);
    }
}
#endif

// Remove all existing Mmu crew members, if any
void DeltaGliderXR1::RemoveAllMmuCrewMembers()
{
#ifdef MMU
    const int crewCount = CONST_UMMU(this).GetCrewTotalNumber();
    for (int i = 0; i < crewCount; i++)
    {
        // UMmu bug: cannot use const char * here because RemoveCrewMember takes a 'char *' even though it never modifies it
        char* pName = CONST_UMMU(this).GetCrewNameBySlotNumber(i);
        UMmu.RemoveCrewMember(pName);  // UMMU BUG: METHOD DOESN'T WORK!  
    }
#endif
}


//
// New wrapper methods added for removing UMMU (11-Feb-2018)
//

int DeltaGliderXR1::GetCrewTotalNumber() const
{
#ifdef MMU
    // TODO: call MMU.GetCrewTotalNumber();
#else
    return MAX_PASSENGERS;
#endif
}

const char* DeltaGliderXR1::GetCrewNameBySlotNumber(const int index) const
{
#ifdef MMU
    // TODO: call MMU.GetCrewNameBySlotNumber();
#else
    return GetXR1Config()->CrewMembers[index].name;
#endif
}

int DeltaGliderXR1::GetCrewAgeByName(const char* pName) const
{
#ifdef MMU
    // TODO: call MMU.GetCrewTotalNumber();
#else
    int age = 0;
    for (int i = 0; i < MAX_PASSENGERS; i++)
    {
        CrewMember* pcm = GetXR1Config()->CrewMembers + i;
        if (_stricmp(pcm->name, pName) == 0)
        {
            age = pcm->age;
            break;
        }
    }

    return age;
#endif
}

const char* DeltaGliderXR1::GetCrewMiscIdByName(const char* pName) const
{
#ifdef MMU
    // TODO: call MMU.GetCrewMiscIdByName();
#else
    const char* pMiscID = "";
    for (int i = 0; i < MAX_PASSENGERS; i++)
    {
        CrewMember* pcm = GetXR1Config()->CrewMembers + i;
        if (_stricmp(pcm->name, pName) == 0)
        {
            pMiscID = pcm->miscID;  // "XI0", "XI1", etc.
            break;
        }
    }

    return pMiscID;
#endif
}

// kill the crew and remove any passengers
// Returns: # of crew members on board who are now dead
int DeltaGliderXR1::KillCrew()
{
    int crewMembersKilled = 0;

    m_crewState = CrewState::DEAD;   // do this even if nobody on board so that the controls will be disabled.
#ifdef MMU
    // remove all the crew members
    for (int i = 0; i < MAX_PASSENGERS; i++)
    {
        // UMMu bug: cannot delcare pName 'const' here
        char* pName = GetCrewNameBySlotNumber(i);
        if (strlen(pName) > 0)  // is crew member on board?
        {
            UMmu.RemoveCrewMember(pName);   // he's dead now
            crewMembersKilled++;
        }
    }

    TriggerRedrawArea(AID_CREW_DISPLAY);   // update the crew display since they're all dead now...
    SetPassengerVisuals();     // update the VC mesh

    return crewMembersKilled;
#else
    return MAX_PASSENGERS;
#endif
}
