// ==============================================================
// XR Vessel Framework
//
// Copyright 2007-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XRPayload.h
// Header file defining custom payload data for XR-class vessels
// ==============================================================

#pragma once

#include "OrbiterAPI.h"
#include "stringhasher.h"
#include <string>
#include <unordered_map>
#include <vector>

using namespace stdext;
using namespace std;

// define externs explicitly so we aren't tied to a specific XR-class vessel
extern const VECTOR3 &PAYLOAD_SLOT_DIMENSIONS;
extern const char *DEFAULT_PAYLOAD_THUMBNAIL_PATH;

class XRPayloadClassData;

// hashmap: string -> vector of integers 
typedef unordered_map<const string *, vector<int> *, stringhasher, stringhasher> HASHMAP_STR_VECINT;

// hashmap: string -> XRPayload object
typedef unordered_map<const string *, XRPayloadClassData *, stringhasher, stringhasher> HASHMAP_STR_XRPAYLOAD;

// vector of XRPayloadClassData objects
typedef vector<const XRPayloadClassData *> VECTOR_XRPAYLOAD;

//=========================================================================

// defines the dimensions of all XR Payload thumbnails
const int PAYLOAD_THUMBNAIL_DIMX = 154;
const int PAYLOAD_THUMBNAIL_DIMY = 77;

// immutable payload class data
class XRPayloadClassData
{
public:
    static const XRPayloadClassData &GetXRPayloadClassDataForClassname(const char *pClassname);
    static void Terminate();  // clients should invoke this from their ExitModule method
    static void InitializeXRPayloadClassData();  // clients must invoke this from a one-shot PostStep one second after the simulation starts so that all XR payload vessels are loaded
    static const XRPayloadClassData **GetAllAvailableXRPayloads();  // returns all XRPayloads available in the config\vessels directory
    static ATTACHMENTHANDLE GetAttachmentHandleForPayloadVessel(const VESSEL &childVessel);
    static double getLongestYTouchdownPoint(const VESSEL &vessel);

    //===========================================================
    
    void AddExplicitAttachmentSlot(const char *pParentVesselClassname, int slotNumber);
    bool AreAnyExplicitAttachmentSlotsDefined(const char *pParentVesselClassname) const;
    bool IsExplicitAttachmentSlotAllowed(const char *pParentVesselClassname, int slotNumber) const;

    const char *GetClassname() const         { return m_pClassname; }      // will never be null
    const char *GetConfigFilespec() const    { return m_pConfigFilespec; } // will never be null
    const char *GetDescription() const       { return m_pDescription; }    // will never be null
    const VECTOR3 &GetDimensions() const     { return m_dimensions; }
    const VECTOR3 &GetSlotsOccupied() const  { return m_slotsOccupied; }
    const VECTOR3 &GetPrimarySlotCenterOfMassOffset() const { return m_primarySlotCenterOfMassOffset; }
    bool IsXRPayloadEnabled() const          { return m_isXRPayloadEnabled; }
    bool IsXRConsumableTank() const          { return m_isXRConsumableTank; }
    double GetMass() const                   { return m_mass; }
    const VECTOR3 &GetGroundDeploymentAdjustment() const { return m_groundDeploymentAdjustment; }
    HBITMAP GetThumbnailBitmapHandle() const { return m_hThumbnailBitmap; }  // may be null
    
    // operator overloading
    bool operator==(const XRPayloadClassData &that) const { return (strcmp(m_pClassname, that.m_pClassname) == 0); }  // vessel classnames are unique

protected:
    const char *m_pClassname;       // vessel bare classname (no leading path)
    const char *m_pConfigFilespec;  // config-relative path of vessel's .cfg file; e.g., "Vessel\XRParts.cfg"
    char *m_pDescription;           // cosmetic description
    VECTOR3 m_dimensions;       // width (X), height (Y), length (Z)
    VECTOR3 m_slotsOccupied;    // width (X), height (Y), length (Z)
    VECTOR3 m_primarySlotCenterOfMassOffset;  // X,Y,Z
    HBITMAP m_hThumbnailBitmap; // will be NULL if bitmap is no defined or is invalid
    HASHMAP_STR_VECINT m_explicitAttachmentSlotsMap;   // key=vessel classname, value=list of ship bay slots to which this object may attach (assuming sufficient room).    
    bool m_isXRPayloadEnabled;  // true if this vessel is enabled for docking in the bay, false otherwise
    bool m_isXRConsumableTank;  // true if this vessel contains XR fuel consumable by the parent ship.
    double m_mass;              // nominal mass
    VECTOR3 m_groundDeploymentAdjustment;

private:
    // NOTE: these are 'private' by design to prevent incorrect instantiation: all client code should go through
    // the static GetXRPayloadClassDataForClassname to retrieve XRPayloadClassData data.
    XRPayloadClassData(const char *pConfigFilespec, const char *pClassname);
    virtual ~XRPayloadClassData();
    
    static HASHMAP_STR_XRPAYLOAD s_classnameToXRPayloadClassDataMap;
    static const XRPayloadClassData **s_allXRPayloadEnabledClassData;  // cached list of all XRPayload-enabled vessels objects in the Orbiter config directory, null-terminated
};
