// ==============================================================
// XR Vessel Framework
//
// Copyright 2009-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// EncryptionEngine.h
// Encrypt/decrypt float vertex values for Orbiter MSH files.
// ==============================================================

#pragma once

// table size in integers
#define KEYMANGLE_TABLE_SIZE 1024   

// XOR value used to cloak the secret key text so it won't be easily visible in the DLL
#define XOR_SECRET_KEY_VALUE 0x75

// Note: these methods are all virtual to make them harder to trace in a disassembler
class EncryptionEngine
{
public:
    EncryptionEngine(const unsigned char *pSecretKey, const int keyLength);
    EncryptionEngine(const EncryptionEngine &that);
    virtual ~EncryptionEngine();

    virtual void EncryptVertices(int valueCount, float *values);
    virtual void DecryptVertices(int valueCount, float *values);

    // always 3 ints for these methods
    virtual void ScrambleFaces  (unsigned int *values);
    virtual void UnscrambleFaces(unsigned int *values);

    virtual void EncryptInts(int valueCount, unsigned int *values);
    virtual void DecryptInts(int valueCount, unsigned int *values);

    static void XORInitialSecretKey(unsigned char *pSecretKey, const int keyLength);
    static const char *ToHexString(const unsigned char *pBytes, const int length);

    virtual unsigned char GetCurrentSecretKeyByte() const { return m_pSecretKey[m_keyByteIndex]; }

protected:
    static const unsigned int s_keymangleTable[];

    virtual void ScrambleOrUnscrambleFaces(unsigned int *values, bool scramble);
    virtual unsigned char LatchNextSecretKeyByte();
    virtual unsigned char LatchNextKeyMangleByte();

    unsigned char *m_pSecretKey;    // working key bytes; zero termination byte is NOT used
    int m_keyLength;                // # of bytes in the working key
    int m_keyByteIndex;             // 0...m_keyLength-1
    int m_keyMangleByteIndex;       // 0...4095
    unsigned char m_secretKeyBitmaskForFaceScrambling;  // bitmask with exactly FIVE bits set
};
