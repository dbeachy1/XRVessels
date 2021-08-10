// ==============================================================
// XR Vessel Framework
//
// Copyright 2007-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XRPayload.cpp
// Handles custom payload data for XR-class vessels.
// ==============================================================

#include "XRPayload.h"
#include "VesselAPI.h"
#include "XRPayloadBay.h"
#include "FileList.h"
#include <string>
#include <string.h>

// define static data
HASHMAP_STR_XRPAYLOAD XRPayloadClassData::s_classnameToXRPayloadClassDataMap;
const XRPayloadClassData **XRPayloadClassData::s_allXRPayloadEnabledClassData = nullptr;

// Static method to retrieve the cached XRPayloadClassData for a given Orbiter vessel classname.
// This is a static method shared between all XRn vessels in a given DLL; however, 
// multi-threading is not an issue since Orbiter is single-threaded.
// 
// Note: an XRPayloadClassData is returned for *each* valid vessel name, regardless of how many
// XRPayload data fields are defined in its config file (if *any*).  Any missing data fields will
// be filled in with default values.
//
// Note: clients should invoke Terminate() from the module's ExitModule method so that cached data is freed properly.
//
// pClassname = Orbiter vessel class name; i.e., pVessel->GetClassName() : MUST BE A VALID ORBITER VESSEL CLASSNAME!  
//              If NULL, a dummy PCD will be returned since it won't be an XR Payload vessel anyway.
//
// Returns: XRPayloadClassData for this classname; will never be null
const XRPayloadClassData &XRPayloadClassData::GetXRPayloadClassDataForClassname(const char *pClassname)
{
    // handle pClassname == nullptr here (e.g., for Mir)
    if (pClassname == nullptr)
        pClassname = XRPAYLOAD_BAY_CLASSNAME;    // use dummy bay object, which is a non-XR-payload object

    XRPayloadClassData *pRetVal = nullptr;

    // pull the data from cache, which was already pre-populated with all .cfg files in the system
    auto it = s_classnameToXRPayloadClassDataMap.find(&string(pClassname));
    if (it != s_classnameToXRPayloadClassDataMap.end())
    {
        // object is in cache: return it
        pRetVal = it->second; 
    }
    else   // something goofy is going on: there is no .cfg for this vessel under Config\Vessels
    {
        // return the default PCD 
        pRetVal = s_classnameToXRPayloadClassDataMap.find(&string(XRPAYLOAD_BAY_CLASSNAME))->second;  // will always succeed
    }

    return *pRetVal;
}

// Clean up all memory allocated by the global XRPayloadClassData cache; this should only be invoked from the 
// ExitModule method.
void XRPayloadClassData::Terminate()
{
    // walk through the global cache and free up memory
    // this will also free up all data objects referenced by s_allXRPayloadEnabledClassData
    auto it = s_classnameToXRPayloadClassDataMap.begin();
    for (; it != s_classnameToXRPayloadClassDataMap.end(); it++)
    {
        // NOTE: no reason to invoke erase() on the individual map items: they will be freed along with the hashmap object
        const string *pClassname = it->first;
        const XRPayloadClassData *pObj = it->second;

        delete pClassname;   
        delete pObj;
    }

    // delete the static s_allXRPayloadEnabledClassData array
    delete s_allXRPayloadEnabledClassData;      // do not use 'delete []' here; objects in the array were already freed above
}

// Payload vessles MUST invoke this static method before the simulation begins (typically from clbkPostCreation) so that all Orbiter vessel .cfg files are parsed.
void XRPayloadClassData::InitializeXRPayloadClassData()
{
    // don't re-scan for .cfg files more than once per simulation startup (it is unnecessary, and scanning is somewhat expensive)
    if (s_classnameToXRPayloadClassDataMap.size() > 0)
        return;   // we already parsed the config files and data is static, so nothing more to do

    // Custom FileList scanner to process .cfg files here
    class CfgFileList : public FileList
    {
    public:
        CfgFileList() : FileList("Config\\Vessels", true, ".cfg")
        {
        }

    protected:
        // Callback invoked for non-empty .cfg files.
        virtual void clbkProcessFile(const char *pConfigFilespec, const WIN32_FIND_DATA &fd) override
        {
            char pClassname[MAX_PATH];
            const int configVesselsPathPrefixLength = 15;     // "Config\Vessels\"
            // first, skip the leading "Config\Vessels\" in pConfigFilespec (e.g., "Config\Vessels\UCGO\foo.cfg")
            // The vessel's classname is everything between the leading prefix and the trailing ".cfg";
            // e.g., "UCGO\foo.cfg".
            const size_t classnameLength = strlen(pConfigFilespec) - configVesselsPathPrefixLength - 4;  // don't copy trailing ".cfg" either (4 bytes)
            strncpy(pClassname, pConfigFilespec + configVesselsPathPrefixLength, classnameLength);
            pClassname[classnameLength] = 0;  // zero-terminate string

            // Found a .cfg file, so create a new XRPayloadClassData for it and save it to our master s_classnameToXRPayloadClassDataMap.
            // Note that ALL vessels get a XRPayloadClassData object, even if they are not XRPayload-enabled.
            // NOTE: XRPayloadClassData requires a path relative to $ORBITER_ROOT\Config, so we have to skip over the leading "Config\" in pConfigFilespec here.
            const char *pConfigRelativePath = pConfigFilespec + 7;   // skip leanding "Config\"
            XRPayloadClassData *pPCD = new XRPayloadClassData(pConfigRelativePath, pClassname);

            // Now add it to the system-wide cache
            typedef pair<const string *, XRPayloadClassData *> Str_XRPayload_Pair;
            s_classnameToXRPayloadClassDataMap.insert(Str_XRPayload_Pair(new string(pClassname), pPCD));  // key = ship classname, value=XRPayloadClassData for that vessel class
        }
    };

    // recursively iterate through $ORBITER_HOME\Config\Vessels\... and parse each .cfg file for XRPayload data
    CfgFileList FileList;
    FileList.Scan();    // invokes our clbkProcessFile method above for each .cfg file found

    _ASSERTE(!FileList.GetScannedFilesList().empty());  // should have at least our XRPayloadBay.cfg in the list, plus the other vessels
}

//=========================================================================

// Constructor: create a new payload object from the supplied payload vessel; its config file 
// is parsed for custom configuration data.
// pConfigFilespec = path\filename under $ORBITER_HOME\Config of filename; e.g., "Vessels\XRParts.cfg".
// pClassname = vessel classname to which this payload object is tied; e.g., "XRParts", "UCGO\foo", etc.
XRPayloadClassData::XRPayloadClassData(const char *pConfigFilespec, const char *pClassname) :
    m_hThumbnailBitmap(nullptr)
{
    m_pClassname = _strdup(pClassname);
    m_pConfigFilespec = _strdup(pConfigFilespec);
    
    // set default values; these are overridden by values in the config file, if any
    m_isXRPayloadEnabled = false;     // default is FALSE
    m_isXRConsumableTank = false;     // default is FALSE
    m_pDescription = new char[128];
    strcpy(m_pDescription, "Unknown");
    m_dimensions = _V(1, 1, 1);   // UNKNOWN; should never happen!
    m_mass = 1.0;       // should never happen!
    m_primarySlotCenterOfMassOffset = _V(0, 0, 0);   // default to "mass centered in primary slot"
    m_groundDeploymentAdjustment = _V(0, 0, 0);      // default to "no adjustment"
    
    static char pThumbnailPath[1024];  // static for efficiency
    strcpy(pThumbnailPath, DEFAULT_PAYLOAD_THUMBNAIL_PATH);

    // Note: this should actually return nullptr if there is no vessel file defined in the Vessels directory
    // for the vessel; this would be possible if the the vessel's cfg file was incorrectly installed in
    // the Config directory instead of the correct Config\Vessels.  However, due to an Orbiter core bug
    // (or feature?) oapiOpenFile instead creates an empty file in Config\Vessels and returns a valid handle
    // to it.
    FILEHANDLE hConfigFile = oapiOpenFile(m_pConfigFilespec, FILE_IN, CONFIG);  // filespec is CONFIG-relative here
    if (hConfigFile != nullptr)  // Note: this will always be true with the current Orbiter version; this is here in case the core bug is fixed in the future
    {
        oapiReadItem_bool  (hConfigFile, "XRPayloadEnabled", m_isXRPayloadEnabled);
        oapiReadItem_string(hConfigFile, "Description", m_pDescription);
        oapiReadItem_vec   (hConfigFile, "Dimensions", m_dimensions);
        oapiReadItem_float (hConfigFile, "Mass", m_mass);
        // NOTE: no longer used: oapiReadItem_int   (hConfigFile, "AttachmentPointIndex", m_attachmentPointIndex);
        oapiReadItem_bool  (hConfigFile, "XRConsumableTank", m_isXRConsumableTank);
        oapiReadItem_vec   (hConfigFile, "PrimarySlotCenterOfMassOffset", m_primarySlotCenterOfMassOffset);
        oapiReadItem_string(hConfigFile, "ThumbnailPath", pThumbnailPath);
        oapiReadItem_vec   (hConfigFile, "GroundDeploymentAdjustment", m_groundDeploymentAdjustment);

        // explicit attachment points
        static char vesselsWithExplicitAttachmentSlotsDefined[2048];  // static for efficiency
        if (oapiReadItem_string(hConfigFile, "VesselsWithExplicitAttachmentSlotsDefined", vesselsWithExplicitAttachmentSlotsDefined))
        {
            // slot data exists for at least one vessel
            // Note: we cannot use strtok here in the outer loop, and it's a pain working with the 'string' class, so we'll just
            // parse it ourselves.
            char *pVesselClassname = vesselsWithExplicitAttachmentSlotsDefined;
            while ((*pVesselClassname == ' ') && (*pVesselClassname != 0))
                pVesselClassname++; // skip whitespace

            bool bExitClassLoop = false;   
            while ((*pVesselClassname != 0) && (bExitClassLoop == false))
            {
                char *pEnd;  // reused

                // locate the next space and zero-terminate on it
                for (pEnd = pVesselClassname; *pEnd != 0; pEnd++)
                {
                    if (*pEnd == ' ')
                        break;
                }
                if (*pEnd == 0)
                    bExitClassLoop = true;      // this is the last pass

                // we found a space, so zero-terminate on it
                *pEnd = 0;

                // retrieve the 'ExplicitAttachmentSlots' string of numbers for this vessel
                static char explicitAttachmentSlotsStr[1024];  // static for efficiency
                char prefName[512];
                sprintf(prefName, "%s_ExplicitAttachmentSlots", pVesselClassname);    // e.g., "XR5Vanguard_ExplicitAttachmentSlots"
                if (oapiReadItem_string(hConfigFile, prefName, explicitAttachmentSlotsStr))
                {
                    // slot data defined; parse out the space-delimited slot integer values and add each one to our new object
                    char *pSlotInt = strtok(explicitAttachmentSlotsStr, " ");
                    while (pSlotInt != nullptr)
                    {
                        // convert to an integer and save it in our new payload object
                        int slotNumber = atoi(pSlotInt);
                        if (slotNumber > 0)
                            AddExplicitAttachmentSlot(pVesselClassname, slotNumber);    // slot number is valid

                        pSlotInt = strtok(NULL, " ");   // read next slot value
                    }
                }

                // find the next token, which was replaced with zero, and move just beyond it for the next loop
                while (*pVesselClassname != 0)
                    pVesselClassname++;  // separate line for clarity
                pVesselClassname++;      // skip the zero
            }
        }
        // clean up
        oapiCloseFile(hConfigFile, FILE_IN);
    }  // if (hConfigFile != nullptr)

    // compute the # of slots occupied based on the dimensions (assigned by value)
    m_slotsOccupied = _V(m_dimensions.x / PAYLOAD_SLOT_DIMENSIONS.x,
                         m_dimensions.y / PAYLOAD_SLOT_DIMENSIONS.y,
                         m_dimensions.z / PAYLOAD_SLOT_DIMENSIONS.z);


    // load the thumbnail and save a handle to it
    // Note: our default path here is the Orbiter root directory: i.e., the directory from which
    // Orbiter.exe is running.
    static char pFullThumbnailPath[1024];    
    sprintf(pFullThumbnailPath, "Config\\%s", pThumbnailPath);
    m_hThumbnailBitmap = (HBITMAP)LoadImage(0, pFullThumbnailPath, IMAGE_BITMAP, PAYLOAD_THUMBNAIL_DIMX, PAYLOAD_THUMBNAIL_DIMY, LR_LOADFROMFILE);  // will be null if load failed
    if (m_hThumbnailBitmap == nullptr)
    {
        // Bad thumbnail path!  Switch to the default thumbnail.
        // Note: it is too early for oapiDebugString() to work, and we don't have access to the XR log, so we don't bother logging a message here.
        sprintf(pFullThumbnailPath, "Config\\%s", DEFAULT_PAYLOAD_THUMBNAIL_PATH);  // reset to default thumbnail
        m_hThumbnailBitmap = (HBITMAP)LoadImage(0, pFullThumbnailPath, IMAGE_BITMAP, PAYLOAD_THUMBNAIL_DIMX, PAYLOAD_THUMBNAIL_DIMY, LR_LOADFROMFILE);  // will be null if load failed, but default should always succeed
    }
}

// Destructor
XRPayloadClassData::~XRPayloadClassData()
{
    // must cast away constness for these
    free(const_cast<char *>(m_pClassname));       
    free(const_cast<char *>(m_pConfigFilespec));    
    
    delete m_pDescription;

    // free up the map
    auto it = m_explicitAttachmentSlotsMap.begin();
    for (; it != m_explicitAttachmentSlotsMap.end(); it++)
    {
        // NOTE: no reason to invoke erase() on the individual map items: they will be freed along with the hashmap object
        const string *pMapShipClassname = it->first;
        vector<int> *pSlotList = it->second;

        delete pMapShipClassname;   
        delete pSlotList;
    }

    // free the thumbnail bitmap, if any
    if (m_hThumbnailBitmap != nullptr)
        DeleteObject(m_hThumbnailBitmap);
}

// Add an explicit attachment point to which this object may dock.
// WARNING: this is invoked by the constructor, so be careful what you do in this method.
// pParentVesselClassname = Orbiter classname from the parent vessel's config file; e.g., XR5Vanguard, XR3Ravenwing, etc.
// slotNumber = slot number in the parent ship's bay to which this object may attach.
void XRPayloadClassData::AddExplicitAttachmentSlot(const char *pParentVesselClassname, int slotNumber)
{
    vector<int> *pSlotList = nullptr;  // assume not found
    
    auto it = m_explicitAttachmentSlotsMap.find(&string(pParentVesselClassname));
    
    // did we find an existing slot list of the specified vessel class?
    if (it != m_explicitAttachmentSlotsMap.end())
    {
        // add this slot number to the existing vector
        vector<int> *pSlotList = it->second;
        pSlotList->push_back(slotNumber);
    }
    else    
    {
        // map is empty; clone the incoming ship classname and create a new vector to hold its slot numbers
        vector<int> *pVec = new vector<int>();
        pVec->push_back(slotNumber);    // this is the first and only entry for now

        typedef pair<const string *, vector<int> *> Str_Vec_Pair;
        m_explicitAttachmentSlotsMap.insert(Str_Vec_Pair(new string(pParentVesselClassname), pVec));  // key = ship classname, value=vector<int> slot numbers
    }
}

// Returns true if any explicit bay slots are defined for the specified vessel classname.
bool XRPayloadClassData::AreAnyExplicitAttachmentSlotsDefined(const char *pParentVesselClassname) const
{
    auto it = m_explicitAttachmentSlotsMap.find(&string(pParentVesselClassname));
    return (it != m_explicitAttachmentSlotsMap.end());
}

// Returns true if the specified attachment slot in the bay is explicitly allowed for the specified vessel classname.
bool XRPayloadClassData::IsExplicitAttachmentSlotAllowed(const char *pParentVesselClassname, int slotNumber) const
{
    bool retVal = true;     // assume vessel not found

    auto it = m_explicitAttachmentSlotsMap.find(&string(pParentVesselClassname));
    if (it != m_explicitAttachmentSlotsMap.end())
    {
        retVal = false;     // slot denied now unless explicitly found in the slot list below

        vector<int> *pSlotList = it->second;
        // walk through all the slot numbers valid for this vessel class
        for (vector <int>::iterator slotIT = pSlotList->begin(); slotIT != pSlotList->end(); slotIT++)
        {
            if (slotNumber == *slotIT)
            {
                retVal = true;  // found it
                break;
            }
        }
    }

    return retVal;
}

//-------------------------------------------------------------------------


// Returns cached list of all XRPayload class objects parsed in the Orbiter\Config\Vessels directory.
// This returns a global static array that should not be freed by the caller!
const XRPayloadClassData **XRPayloadClassData::GetAllAvailableXRPayloads() 
{
    // check whether we already built this
    if (s_allXRPayloadEnabledClassData == nullptr)
    {       
        // this is the first call, so build the array...
        VECTOR_XRPAYLOAD allXRPayloads;

        // Walk through each XRPayloadClassData in our s_classnameToXRPayloadClassDataMap and copy all XRPayload-enabled ones to our master s_allXRPayloadEnabledClassData 
        unordered_map<const string *, XRPayloadClassData *>::const_iterator it = s_classnameToXRPayloadClassDataMap.begin();  // iterate over values
        for (; it != s_classnameToXRPayloadClassDataMap.end(); it++)
        {
            const XRPayloadClassData *pPCD = it->second;  // get next PCD
            if (pPCD->IsXRPayloadEnabled())
                allXRPayloads.push_back(pPCD);  // found one, so save it
        }

        // now allocate the static global array and copy each XRPayload-enabled vessel to it
        s_allXRPayloadEnabledClassData = new const XRPayloadClassData *[allXRPayloads.size() + 1];  // array of pointers plus room for null terminator
        int indexOut = 0;
        for (size_t i=0; i < allXRPayloads.size(); i++)
            s_allXRPayloadEnabledClassData[indexOut++] = allXRPayloads[i];   // save in our global cache array

        s_allXRPayloadEnabledClassData[indexOut] = nullptr;    // terminate the array
    }

    return s_allXRPayloadEnabledClassData;
}

// Static method that returns a child's attachment handle to be attached to the parent's
// payload bay, or NULL if no XRCARGO attachment point is defined OR if childVessel is not an XRPayload-enabled vessel.
ATTACHMENTHANDLE XRPayloadClassData::GetAttachmentHandleForPayloadVessel(const VESSEL &childVessel)
{
    const XRPayloadClassData &pcd = XRPayloadClassData::GetXRPayloadClassDataForClassname(childVessel.GetClassName());
    if (pcd.IsXRPayloadEnabled() == false)
        return nullptr;   // nothing to do: not an XR payload

    ATTACHMENTHANDLE retVal = nullptr;

    // Determine which attachment point to use on the child by iterating through all attachment points 
    // on the child vessel and locate the one with the "XRCARGO" tag on a PARENT attachment point.
    for (DWORD i=0; i < childVessel.AttachmentCount(true); i++)
    {
        ATTACHMENTHANDLE hAttachment = childVessel.GetAttachmentHandle(true, i);  // will never be null since index is in range
        const char *pAttachmentID = childVessel.GetAttachmentId(hAttachment);
        if ((pAttachmentID != nullptr) && (_stricmp(pAttachmentID, "XRCARGO") == 0))
        {
            retVal = hAttachment;  // found it
            break;
        }
    }
    return retVal;
}

// Static method that returns the LONGEST (i.e., most *negative*) Y leg of the specified vessel's touchdown points.
// Note: as of 4.5.2010, this method is not currently used.
double XRPayloadClassData::getLongestYTouchdownPoint(const VESSEL &vessel)
{
    VECTOR3 pt1, pt2, pt3;
    vessel.GetTouchdownPoints(pt1, pt2, pt3);
    
    // now figure out which one has the longest (most NEGATIVE) Y leg
    const double minY = min(min(pt1.y, pt2.y), pt3.y);
    return minY;
}