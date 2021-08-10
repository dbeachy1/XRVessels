// ==============================================================
// ORBITER MODULE: XR Vessel framework
//
// Copyright 2006-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// Vessel3ExtMeshEncryption.cpp
// 
// Methods for Vessel3Ext handling mesh encryption.
// ==============================================================

#include "Vessel3Ext.h"

// STEP 1: invoked from the subclass's clbkSetClassCaps to enable encrypted mesh handling
void VESSEL3_EXT::InitEncryptedMeshHandler(const unsigned char *pSecretKey, const int keyLength)
{
    // remember that the mesh is encrypted
    m_isExmeshTplEncrypted = true;

    // clone the secret key and save its length as well
    unsigned char *pOrgKeyBuffer = new unsigned char[keyLength];
    memcpy(pOrgKeyBuffer, pSecretKey, keyLength);

    m_pSecretKey = pOrgKeyBuffer;
    m_secretKeyLength = keyLength;

    // Note: m_pEncryptionEngine is initialized later
}

// STEP 2: invoked from the subclass's clbkSetClassCaps method sometime after exmesh_tpl is initialized
// if encryption enabled, parse our global mesh data in exmesh_tpl but do not parse it yet
void VESSEL3_EXT::ParseEncryptedMesh()
{
    if (exmesh_tpl == nullptr)   // sanity check
        return; 

    if (m_isExmeshTplEncrypted)
    {
        // mesh encryption enabled -- recreate the decryption engine to reset it
        // Note: m_meshGroupVector was reset and any existing m_pEncryptionEngine was freed in clbkVisualDestroyed
        m_pEncryptionEngine = new EncryptionEngine(m_pSecretKey, m_secretKeyLength);

        // Sneaky: use m_meshGroupVector vector to hold MESHGROUP data so we can decrypt it later in clbkPostCreation.
        // This will make it harder to trace.
        //
        // WARNING: we must step through each group in order so the keys will be correct
        // when we decrypt the mesh vertices and faces.

        // Also note that there is no sense in trying to hide this method with a JumpGateBarrier: 
        // 'idag' would pinpoint the call to the DLL method "GetMesh" and the hacker would find this code anyway.
        DWORD dwMeshCount = oapiMeshGroupCount(exmesh_tpl);
        m_meshGroupVector.clear();       // just in case; should not be necessary
        for (DWORD i = 0; i < dwMeshCount; i++)
        {
            MESHGROUP *pMeshGroup = oapiMeshGroup(exmesh_tpl, i);
            if (pMeshGroup == nullptr) // just in case
                break;

            // save the group in our list
            m_meshGroupVector.push_back(pMeshGroup);
        }

        // Note: we decrypt the mesh later in cblkPostCreation to make this harder to trace.
    }
}

// Decrypt the mesh previously parsed by ParseEncryptedMesh UNLESS it is already decypted.
void VESSEL3_EXT::DecryptMeshData()
{
    // sanity check: if mesh group data already cleared, nothing to do
    if (m_meshGroupVector.size() == 0)
        return;     // should never happen!

    // iterate through each mesh group and decrypt it
    bool firstLoop = true;
    MESHGROUPIterator it2 = m_meshGroupVector.begin();
    for (; it2 != m_meshGroupVector.end(); it2++)
    {
        MESHGROUP *pMeshGroup = *it2;
        
        // decrypt all the vertices for this group 
        NTVERTEX *pVertex = pMeshGroup->Vtx;  // working index pointer
        if (firstLoop)
        {
            firstLoop = false;
            // check whether the mesh is already decrypted; if so, nothing more to do
            // WARNING: we cannot do a simple memory compare here because of small rounding errors that
            // occur when reading the ASCII values in from the scenario file; therefore, to be safe
            // we will round each float to the nearest 1/100th and compare each one.
            bool vtxMatches = true;
            const float *pArray1 = reinterpret_cast<const float *>(s_pFirstDecryptedVertex);
            const float *pArray2 = reinterpret_cast<const float *>(pVertex);
            for (int i=0; i < 8; i++)   // compare all 8 floats
            {
                if (CompareFloatsLoose(pArray1[i], pArray2[i]) == false)
                {
                    vtxMatches = false;
                    break;      // found mismatch; no sense in checking more
                }
            }
            if (vtxMatches)
                return;     // first vertex matches, so mesh is already decypted.
        }
        
        for (DWORD i=0; i < pMeshGroup->nVtx; i++)
        {
            // Note: the mesh we encrypted always has 8 bytes per vertex, zero-padded if necessary
            m_pEncryptionEngine->DecryptVertices(8, reinterpret_cast<float *>(pVertex));  // structure is 8 floats long
            pVertex++;
        }

        // unscramble all the faces for this group
        WORD *pFace = pMeshGroup->Idx;  // working index pointer
        for (DWORD i=0; i < (pMeshGroup->nIdx / 3); i++)   // process one triplet at a time
        {
            // Since Orbiter stores these as shorts, we need to copy them to/from an 
            // unsigned integer array so the encryption engine can deal with them.
            unsigned int f[3];
            for (int j=0; j < 3; j++)
                f[j] = pFace[j];

            m_pEncryptionEngine->UnscrambleFaces(f);  // we always have exactly 3 integers here

            // copy the unscrambled face values back to our mesh copy
            for (int j=0; j < 3; j++)
                pFace[j] = static_cast<WORD>(f[j]);   // OK to truncate back to 16 bits here

            pFace += 3;   // set to next triplet
        }
    }
}

// --------------------------------------------------------------
// compare two floats after rounding to the nearest 1/100th
// Returns true if floats match, or false if not
bool VESSEL3_EXT::CompareFloatsLoose(float f1, float f2) const
{
    /* NO (conversion to introduce tiny errors)
    // we should always match in at least the first two decimal places, so round to that
    int loose1 = static_cast<int>(((f1 + 0.005) * 100));
    int loose2 = static_cast<int>(((f2 + 0.005) * 100));

    return (loose1 == loose2);
    */

    return (f1 == f2);
}
