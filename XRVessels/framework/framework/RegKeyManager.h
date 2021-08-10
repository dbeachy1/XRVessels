//
// RegKeyManager.h
//
// Copyright 2016 Douglas E.Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
#pragma once

#include <windows.h>
#include <WinCrypt.h>	// for DATA_BLOB
#include <atlstr.h>		// for CString

// Reads & writes our values to the registry
class RegKeyManager
{
public:
    RegKeyManager();
    virtual ~RegKeyManager();

    bool Initialize(const HKEY hRootKey, const TCHAR *pSubkeyPath, const HWND hOwnerWindow);
    bool IsInitialized() const { return m_bInitialized; }

    bool WriteRegistryString(const TCHAR *pValueName, const TCHAR *pValue) const;
    bool ReadRegistryString(const TCHAR *pValueName, CString &valueOut) const;
    
    bool WriteRegistryBlob(const TCHAR *pValueName, const DATA_BLOB &blob) const;
    bool ReadRegistryBlob(const TCHAR *pValueName, DATA_BLOB &blobOut) const;
    
    bool WriteRegistryDWORD(const TCHAR *pValueName, const DWORD dwValue) const;
    bool ReadRegistryDWORD(const TCHAR *pValueName, DWORD &valueOut) const;
    
    bool WriteRegistryQWORD(const TCHAR *pValueName, const ULONGLONG dwValue) const;
    bool ReadRegistryQWORD(const TCHAR *pValueName, ULONGLONG &valueOut) const;

    LONG DeleteRegistryValue(const TCHAR *pValueName) const;

private:
    HKEY m_hKey;
    HWND m_hOwnerWindow;  
    bool m_bInitialized;

    TCHAR *m_pSubkeyPath;  // preserved for error messages
};
