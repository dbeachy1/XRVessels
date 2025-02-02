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
