/**
  XR Vessel add-ons for OpenOrbiter Space Flight Simulator
  Copyright (C) 2006-2021 Douglas Beachy

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.

  Email: mailto:doug.beachy@outlook.com
  Web: https://www.alteaaerospace.com
**/

// Example usage:
//    RegKeyManager mgr;
//	  mgr.Initialize(HKEY_CURRENT_USER, ""AlteaAerospace\\XR\\WindowPos", NULL);
//    DWORD xOut;
//    mgr.ReadRegistryDWORD("X", &xOut);

#include "RegKeyManager.h"

// Constructor
RegKeyManager::RegKeyManager() :
    m_hKey(nullptr), m_bInitialized(false)
{
}

// Destructor
RegKeyManager::~RegKeyManager()
{
    if (m_hKey)
        RegCloseKey(m_hKey);
}

// Initialize this object; returns true on success.  No error message box is shown; the caller should handle that.
// hRootKey: root key; e.g., HKEY_CURRENT_USER
// pSubkeyPath: key from which to read values; e.g., "AlteaAerospace\\Orbiter\\XR\\WindowPos"
// hOwnerWindow: window that launched us; may be NULL
//
// Should only be called once.
bool RegKeyManager::Initialize(const HKEY hRootKey, const TCHAR *pSubkeyPath, const HWND hOwnerWindow)
{
    _ASSERTE(pSubkeyPath);
    _ASSERTE(!m_bInitialized);

    m_hOwnerWindow = hOwnerWindow;
    m_bInitialized = (RegCreateKeyEx(hRootKey, pSubkeyPath, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &m_hKey, NULL) == ERROR_SUCCESS);
    return m_bInitialized;
}

//
// Registry access methods; each returns true on success or false on error. 
//
// Writes a registry string value; returns false if error occurs
bool RegKeyManager::WriteRegistryString(const TCHAR *pValueName, const TCHAR *pValue) const
{
    _ASSERTE(m_bInitialized);

    // length includes the trailing zero byte
    bool bSuccess = (RegSetValueEx(m_hKey, pValueName, NULL, REG_SZ, reinterpret_cast<const BYTE *>(pValue), static_cast<DWORD>((_tcslen(pValue) + 1) * sizeof(TCHAR))) == ERROR_SUCCESS);

	return bSuccess;
}

// Reads a registry string value into valueOut; if error occurs (i.e., key does not exist), valueOut is unchanged.
// Returns false if error occurs
bool RegKeyManager::ReadRegistryString(const TCHAR *pValueName, CString &valueOut) const
{
    _ASSERTE(m_bInitialized);

    DWORD dwType;
	DWORD dwSize = 0;
	bool bSuccess = false;

    // get the size of the value
    if (RegQueryValueEx(m_hKey, pValueName, NULL, &dwType, NULL, &dwSize) == ERROR_SUCCESS)
    {
        if ((dwType == REG_SZ) && (dwSize > 0))
        {
            // now retrieve the actual value
            BYTE *pBuffer = reinterpret_cast<BYTE *>(valueOut.GetBufferSetLength(dwSize));  // dwSize is in bytes, but we allocate a minimum of n characters anyway in case the string value was stored as a non-Unicode value
            bSuccess = (RegQueryValueEx(m_hKey, pValueName, NULL, &dwType, pBuffer, &dwSize) == ERROR_SUCCESS);
            valueOut.ReleaseBuffer();
        }
        else
        {
            SetLastError(ERROR_BAD_LENGTH);
        }
    }
    return bSuccess;
}

// writes a blob out to a registry value; shows a messagebox if an error occurs.
// Returns false if error occurs
bool RegKeyManager::WriteRegistryBlob(const TCHAR *pValueName, const DATA_BLOB &blob) const
{
    _ASSERTE(m_bInitialized);

    // length includes the trailing zero byte
    bool bSuccess = (RegSetValueEx(m_hKey, pValueName, NULL, REG_BINARY, blob.pbData, blob.cbData) == ERROR_SUCCESS);

    return bSuccess;
}

// Reads a blog from the registry into blobOut; if error occurs, blobOut is unchanged.
// It is the caller's responsibility to eventually invoke LocalFree on blobOut.pbData.
// Returns false if error occurs.
bool RegKeyManager::ReadRegistryBlob(const TCHAR *pValueName, DATA_BLOB &blobOut) const
{
    _ASSERTE(m_bInitialized);

    DWORD dwType;
	bool bSuccess = false;

    // get the size of the value
    if (RegQueryValueEx(m_hKey, pValueName, NULL, &dwType, NULL, &blobOut.cbData) == ERROR_SUCCESS)
    {
        if ((dwType == REG_BINARY) && (blobOut.cbData > 0))
        {
            // now retrieve the actual value
            // Note: we must use LocalAlloc here since that is what the cryptography functions use when working with DATA_BLOB objects.
            blobOut.pbData = static_cast<BYTE *>(LocalAlloc(LMEM_FIXED, blobOut.cbData));
            bSuccess = (RegQueryValueEx(m_hKey, pValueName, NULL, &dwType, blobOut.pbData, &blobOut.cbData) == ERROR_SUCCESS);
        }
        else
        {
            SetLastError(ERROR_BAD_LENGTH);
        }
    }
    return bSuccess;
}

// Writes a registry DWORD value; shows a messagebox if an error occurs.
// Returns false if error occurs
bool RegKeyManager::WriteRegistryDWORD(const TCHAR *pValueName, const DWORD dwValue) const
{
    _ASSERTE(m_bInitialized);

    // length includes the trailing zero byte
    bool bSuccess = (RegSetValueEx(m_hKey, pValueName, NULL, REG_DWORD, reinterpret_cast<const BYTE *>(&dwValue), sizeof(DWORD)) == ERROR_SUCCESS);

    return bSuccess;
}

// Reads a registry DWORD value into valueOut; if error occurs (i.e., key does not exist), valueOut is unchanged.
// Returns false if error occurs
bool RegKeyManager::ReadRegistryDWORD(const TCHAR *pValueName, DWORD &valueOut) const
{
    _ASSERTE(m_bInitialized);

    DWORD dwType;
    DWORD dwSize = sizeof(DWORD);

    BYTE *pBuffer = reinterpret_cast<BYTE *>(&valueOut);  
    const bool bSuccess = (RegQueryValueEx(m_hKey, pValueName, NULL, &dwType, pBuffer, &dwSize) == ERROR_SUCCESS);

    return bSuccess;
}

// Writes a registry QWORD value; shows a messagebox if an error occurs.
// Returns false if error occurs
bool RegKeyManager::WriteRegistryQWORD(const TCHAR *pValueName, const ULONGLONG qwValue) const
{
    _ASSERTE(m_bInitialized);

    // length includes the trailing zero byte
    bool bSuccess = (RegSetValueEx(m_hKey, pValueName, NULL, REG_QWORD, reinterpret_cast<const BYTE *>(&qwValue), sizeof(ULONGLONG)) == ERROR_SUCCESS);

    return bSuccess;
}

// Reads a registry QWORD value into valueOut; if error occurs (i.e., key does not exist), valueOut is unchanged.
// Returns false if error occurs
bool RegKeyManager::ReadRegistryQWORD(const TCHAR *pValueName, ULONGLONG &valueOut) const
{
    _ASSERTE(m_bInitialized);

    DWORD dwType;
    DWORD dwSize = sizeof(ULONGLONG);

    BYTE *pBuffer = reinterpret_cast<BYTE *>(&valueOut);  
    const bool bSuccess = (RegQueryValueEx(m_hKey, pValueName, NULL, &dwType, pBuffer, &dwSize) == ERROR_SUCCESS);

    return bSuccess;
}

// Delete a registry value.  No error messagebox is displayed if the delete fails; the caller should
// check the return code from this call.
//
// pValueName: may be NULL or empty, in which case the default value for this manager's key is deleted
// 
// Returns: error code defined in Winerror.h (ala GetLastError())
LONG RegKeyManager::DeleteRegistryValue(const TCHAR *pValueName) const
{
    return RegDeleteValue(m_hKey, pValueName);
}

