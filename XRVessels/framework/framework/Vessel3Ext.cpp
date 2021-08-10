// ==============================================================
// ORBITER MODULE: XR Vessel framework
//
// Copyright 2006-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// Vessel3Ext.cpp
// 
// Class defining an extended VESSEL3 class for use with the new cockpit framework.
// ==============================================================

#pragma once

#include "Orbitersdk.h"
#include "vessel3ext.h"
#include "XRTemplates.h"

#include "InstrumentPanel.h"
#include "PrePostStep.h"

// sneaky: this is set by the subclass if mesh encryption is enabled
NTVERTEX *VESSEL3_EXT::s_pFirstDecryptedVertex = nullptr;

// constructor
VESSEL3_EXT::VESSEL3_EXT(OBJHANDLE vessel, int fmodel) :
    XRVesselCtrl(vessel, fmodel),
    m_hModule(nullptr), m_hasFocus(false),
    m_pSecretKey(nullptr), m_secretKeyLength(0),
    m_pEncryptionEngine(nullptr), exmesh_tpl(nullptr), m_isExmeshTplEncrypted(false),
	m_videoWindowWidth(0), m_videoWindowHeight(0), m_lastVideoWindowWidth(-1), m_last2DPanelWidth(0),
    m_absoluteSimTime(0)
{
	m_regKeyManager.Initialize(HKEY_CURRENT_USER, XR_GLOBAL_SETTINGS_REG_KEY, NULL);   // should always succeed
}

// destructor
VESSEL3_EXT::~VESSEL3_EXT()
{
    // clean up each instrument panel in our list
    InstrumentPanelIterator it = GetPanelMap().begin();   // iterates over values
    for (; it != GetPanelMap().end(); it++)
    {
        InstrumentPanel *pPanel = it->second;  // get next panel in the map
        pPanel->Deactivate();                  // release surfaces
        delete pPanel;                         // ...and deallocate
    }

    // clean up each PostStep in our list
    PostStepIterator it2 = GetPostStepVector().begin();   // iterates over values
    for (; it2 != GetPostStepVector().end(); it2++)
    {
        PrePostStep *pStep = *it2;
        delete pStep;
    }

    // clean up our grapple target vessel cache; this will be empty for vessels that never invoke GetGrappleTargetVessel(...)
    auto it3 = m_grappleTargetMap.begin();   // iterates over values
    for (; it3 != m_grappleTargetMap.end(); it3++)
    {
        const string *pName = it3->first;
        XRGrappleTargetVessel *pGrappleTarget = it3->second;

        delete pName;
        delete pGrappleTarget;
    }

    // clean up mesh encryption data
    // note: do not delete m_pEncryptionEngine here; it was deleted in clbkVisualDestroyed
    delete m_pSecretKey;
}

// Add a new instrument panel to our map of panels
// Note: panelWidth must be zero for VC panels
void VESSEL3_EXT::AddInstrumentPanel(InstrumentPanel *pPanel, const int panelWidth)
{
    const int panelID = pPanel->GetPanelID();
    
    // sanity check
#ifdef _DEBUG
    if (panelID >= GetVCPanelIDBase()) // is this a VC panel?
        _ASSERTE(panelWidth == 0);
    else  // this is a 2D panel
        _ASSERTE(panelWidth > 0);
#endif

    // compute the panel hash
    const int panelHash = GetPanelKey(panelID, panelWidth);
    typedef pair<int, InstrumentPanel *> Int_InstrumentPanel_Pair;
    
    m_panelMap.insert(Int_InstrumentPanel_Pair(panelHash, pPanel));  // key = panel ID, value = panel *
}

// Add a new PostStep to our vector
void VESSEL3_EXT::AddPostStep(PrePostStep *pStep)
{
    GetPostStepVector().push_back(pStep);  // add to end of vector
}

// Add a new PreStep to our vector
void VESSEL3_EXT::AddPreStep(PrePostStep *pStep)
{
    GetPreStepVector().push_back(pStep);  // add to end of vector
}

// Returns the panel with the requested number (0-n), or NULL if panel number is invalid
// Note that each VC panel has a unique ID alongside the 2D panels
// vcPanelIDBase = VC_PANEL_ID_BASE from the subclass
InstrumentPanel *VESSEL3_EXT::GetInstrumentPanel(const int panelNumber)
{
    // obtain the current panel width, or 0 if this is a VC panel
    int panelWidth = (Is2DPanel(panelNumber) ? Get2DPanelWidth() : 0);

    // compute the panel hash
    const int panelHash = GetPanelKey(panelNumber, panelWidth);

    InstrumentPanel *retVal = nullptr;    // assume not found
    InstrumentPanelIterator it = m_panelMap.find(panelHash);
    
    if (it != m_panelMap.end())
        retVal = it->second;     // found a matching panel
    
    _ASSERTE(retVal != nullptr);
    return retVal;
}

// Trigger a redraw are for the supplied area ID by sending the request to each of our panels
bool VESSEL3_EXT::TriggerRedrawArea(const int areaID)
{
    // iterate through each of our instrument panels and send the request to each
    bool wasProcessed = false;

    InstrumentPanelIterator it = m_panelMap.begin();   // iterates over values

    // for efficiency, only send this redraw request to the active panel
    for (; it != m_panelMap.end(); it++)
    {
        InstrumentPanel *pPanel = it->second;  // get next panel in the map
        if (pPanel->IsActive())
        {
            wasProcessed = pPanel->TriggerRedrawArea(areaID);
            break;    // only one active panel at a time, so no need to check the others
        }
    }

    return wasProcessed;
}

// Note: this is called BEFORE clbkLoadPanel; this is sort of a hack to get the video mode width, but it's the only way to do it
// short of implementing bitmap-independent panels.
// TODO: look into use oapiGetViewportSize() instead of this.
bool VESSEL3_EXT::clbkLoadPanel2D(int panelID, PANELHANDLE hPanel, DWORD viewW, DWORD viewH)
{
	static bool s_isFirstRun = true;

	m_videoWindowWidth = viewW;  // save so clbkLoadPanel can get it
	m_videoWindowHeight = viewH;
	return false;       // so Orbiter core will invoke clbkLoadPanel next
}

// Activate the requested panel; returns true on success, 
// false on error (e.g., a bitmap failed to load)
// Implements VESSEL2 callback method.
// On entry: m_last2DPanelWidth = active 2D panel width
// NOTE: panelID may refer to a 3D (VC) panel.
bool VESSEL3_EXT::clbkLoadPanel(int panelID)
{
    // release any surfaces from any other panels, 2D and 3D
    DeactivateAllPanels();

    InstrumentPanel *pPanel = GetInstrumentPanel(panelID);   // retrieves cached panel of the correct resolution active video mode
    bool activationSuccessful = pPanel->Activate();   // if null here, the caller screwed up and we will (correctly) crash
    if (activationSuccessful)
        pPanel->SetActive(true);    // mark as active so the panel's Activate() method doesn't have to remember to do it

    return activationSuccessful;
}

// Deactivate all panels; i.e., release all surfaces
void VESSEL3_EXT::DeactivateAllPanels()
{
    InstrumentPanelIterator it = GetPanelMap().begin(); // key = panel ID, value = InstrumentPanel *
    for (; it != m_panelMap.end(); it++)
    {
        InstrumentPanel *pPanel = it->second;  // get next panel in the map
        pPanel->Deactivate();   // release all surfaces
    }
}


// Returns the optimal (or configured) panel width to use for m_videoWindowWidth.
// Returns: 1280, 1600, or 1920.
int VESSEL3_EXT::Get2DPanelWidth()
{
    // see if we can use the cached value
    // NOTE: normally, m_videoWindowWidth will *never* change after it is set once since the viewport (currently) does not change size.
    // However, we are set up to handle that here if and when it would ever happen.
    if (m_videoWindowWidth != m_lastVideoWindowWidth)
    {
        m_lastVideoWindowWidth = m_videoWindowWidth;  // remember for next time

        // video window width has changed or was never set, so let's set m_last2DPanelWidth below
        m_last2DPanelWidth = 0;    // for assertion check later
        switch (m_pConfig->GetTwoDPanelWidth())
        {
        case AUTODETECT:
            if (m_videoWindowWidth > 0)
            {
                char msg[256];
				sprintf(msg, "Autodetected video window width x height: %d x %d pixels", m_videoWindowWidth, m_videoWindowHeight);
                m_pConfig->WriteLog(msg);

                // figure out which panel size to use

                // Width in pixels to tolerate before dropping to lower panel size; this is because some newbie users will 
                // set their video window size to *exactly*, for example, 1600 instead of 1606, resulting in a render window of 1594
                // rather than 1600.  So rather than deal with mountains of newbie "bug reports" about how "auto-panel size isn't working",
                // we'll just let it clip for 3 pixels per side.
                const int tolerance = 6;       
                if (m_videoWindowWidth >= (1920 - tolerance))
                    m_last2DPanelWidth = 1920;
                else if (m_videoWindowWidth >= (1600 - tolerance))
                    m_last2DPanelWidth = 1600;
                else   // under 1600 pixels wide, so use the smallest panel
                    m_last2DPanelWidth = 1280;
            }
            else  // old Orbiter version; clbkLoadPanel2D was not invoked!
            {
                m_pConfig->WriteLog("WARNING: OLD ORBITER VERSION - unable to automatically determine video window width.  Falling back to 2D panel resolution of 1280 pixels.");
                m_last2DPanelWidth = 1280;
            }
            break;

        case USE1280:
            m_last2DPanelWidth = 1280;
            WriteForced2DResolutionLogMessage(m_last2DPanelWidth);
            break;

        case USE1600:
            m_last2DPanelWidth = 1600;
            WriteForced2DResolutionLogMessage(m_last2DPanelWidth);
            break;

        case USE1920:
            m_last2DPanelWidth = 1920;
            WriteForced2DResolutionLogMessage(m_last2DPanelWidth);
            break;

        default:    // should never happen!
            {
                m_last2DPanelWidth = 1280;    // fall back to the smallest panel
                static char msg[1024];  // static for efficiency
                sprintf(msg, "WARNING: invalid TwoDPanelWidth value in XR configuration file [%s]; defaulting to 2D panel width of %d pixels.", 
                    m_pConfig->GetConfigFilenames(), m_last2DPanelWidth);
                m_pConfig->WriteLog(msg);
                break;
            }
        }
    }

    _ASSERTE(m_last2DPanelWidth > 0);  // make sure we set the value

    return m_last2DPanelWidth;  // this is the active 2D panel width
}

void VESSEL3_EXT::WriteForced2DResolutionLogMessage(const int panelWidth) const
{
    static char msg[1024];  // static for efficiency
    sprintf(msg, "Forcing 2D panel width of %d pixels per XR configuration file [%s].", 
        panelWidth, m_pConfig->GetConfigFilenames());
    m_pConfig->WriteLog(msg);
}

// Process a 2D mouse event for all panels
// Implements VESSEL2 method
bool VESSEL3_EXT::clbkPanelMouseEvent(int areaID, int event, int mx, int my)
{
    bool wasProcessed = false;
    InstrumentPanelIterator it = GetPanelMap().begin(); // key = panel ID, value = InstrumentPanel *
    for (; (wasProcessed == false) && (it != m_panelMap.end()); it++)
    {
        // only send this event to the ACTIVE panel
        InstrumentPanel *pPanel = it->second;
        if (pPanel->IsActive())
        {
            wasProcessed = pPanel->ProcessMouseEvent(areaID, event, mx, my);
            break;  // only one active panel, so don't bother checking the rest
        }
    }

    return wasProcessed;
}

// Process a VC mouse event for all panels
// Implements VESSEL2 method
bool VESSEL3_EXT::clbkVCMouseEvent(int areaID, int event, VECTOR3 &coords)
{
    bool wasProcessed = false;
    InstrumentPanelIterator it = GetPanelMap().begin(); // key = panel ID, value = InstrumentPanel *
    for (; (wasProcessed == false) && (it != m_panelMap.end()); it++)
    {
        // only send this event to the ACTIVE panel
        InstrumentPanel *pPanel = it->second;
        if (pPanel->IsActive())
        {
            wasProcessed = pPanel->ProcessVCMouseEvent(areaID, event, coords);
            break;  // only one active panel, so don't bother checking the rest
        }
    }

    return wasProcessed;
}

// Implements VESSEL2 method
bool VESSEL3_EXT::clbkPanelRedrawEvent(int areaID, int event, SURFHANDLE surf)
{
    bool wasProcessed = false;
    InstrumentPanelIterator it = GetPanelMap().begin(); // key = panel ID, value = InstrumentPanel *
    
    // Only send this event to the ACTIVE panel; otherwise, beyond being less efficient, if an Area 
    // object is present on more than one panel the redraw event may be incorrectly sent to the wrong panel.
    for (; (wasProcessed == false) && (it != m_panelMap.end()); it++)
    {
        InstrumentPanel *pPanel = it->second;
        if (pPanel->IsActive())
        {
            wasProcessed = pPanel->ProcessRedrawEvent(areaID, event, surf);
            break;  // only one active panel, so don't bother checking the rest
        }
    }

    return wasProcessed;
}

// Retrieve an area by its ID for a given panel; remember that the same area can (and usually will!) have the same ID
// if it appears on multiple panels.
//
// This will return the area object for a given panel ID.
// Returns: requested Area object, or NULL if area not found on the specified panel
Area *VESSEL3_EXT::GetArea(const int panelID, const int areaID)
{
    Area *pArea = nullptr;
    InstrumentPanel *pPanel = GetInstrumentPanel(panelID);
    if (pPanel != nullptr)
        pArea = pPanel->GetArea(areaID);

    return pArea;
}

//
// Main Orbiter callback method, clbkPostStep.
//
// This event is sent to ALL panels; however, these callbacks should ONLY perform area-display-specific tasks (such as blinking a light).
// The default handler for each area does nothing.
// This event is also sent to all registered PostStep objects.
//
// You may override this method in your subclass, but remember to invoke the base class method here from it as well so that 
// Area and InstrumentPanel objects that hook clbkPostStep, as well as registered PostStep objects, will still be notified correctly.
// Normally, however, you should not need to override this.
//
// If your area needs to hook clbkPreStep, opcPreStep, or opcPostStep, hook it in your VESSEL3_EXT subclass and invoke
// GetArea(...) to retrieve the area and invoke your handler manually.  Those other three methods were not hooked
// here because they are rarely used by VESSEL3 objects and we want to minimize any performance hit from hooking them.
//
void VESSEL3_EXT::clbkPostStep(double simtDoNotUse, double simdt, double mjd)
{
    // ********************************************************************
    // NEW FOR XR1 1.9 release group: pass the absolute simt (is not affected by
    // adjustments to MJD) to all areas so that all MDJ wonkiness is no longer 
    // an issue.
    // ********************************************************************
    // Note: PostStep happens after the PreStep, so AbsoluteSimTime was already updated before here.
    const double simt = GetAbsoluteSimTime();

    // NEW BEHAVIOR for XR1 1.3: only invoke PostSteps on the ACTIVE panel, since they should not be doing any business logic anyway.
    InstrumentPanelIterator it = GetPanelMap().begin(); // key = panel ID, value = InstrumentPanel *
    for (; it != GetPanelMap().end(); it++)
    {
        InstrumentPanel *pPanel = it->second;
        if (pPanel->IsActive())
        {
            pPanel->clbkPrePostStep(simt, simdt, mjd);
            break;      // only one active panel, so no need to keep searching
        }
    }

    // invoke all registered PostStep objects
    PostStepIterator it2 = GetPostStepVector().begin();
    for (; it2 != GetPostStepVector().end(); it2++)
    {
        PrePostStep *pStep = *it2;
        pStep->clbkPrePostStep(simt, simdt, mjd);
    }
}

//
// Main Orbiter callback method, clbkPreStep.
//
// This event is sent to all registered PreStep handlers, in sequence.  It is currently NOT sent to panel areas for two reasons:
// 1) it is very rarely, if ever, used by a panel area (it is typically used for autopilots), and 
// 2) since it is so rarely used by panel areas, it is more efficient to not send it at all.
//
// You may override this method in your subclass, but remember to invoke the base class method here from it as well so that 
// Area and InstrumentPanel objects that hook clbkPostStep, as well as registered PostStep objects, will still be notified correctly.
//
// If your area needs to hook opcPreStep, or opcPostStep, hook it in your VESSEL3_EXT subclass and invoke
// GetArea(...) to retrieve the area and invoke your handler manually.  Those other two methods were not hooked
// here because they are rarely used by VESSEL3 objects and we want to minimize any performance hit from hooking them.
//
void VESSEL3_EXT::clbkPreStep(double simtDoNotUse, double simdt, double mjd)
{
    //**************************************************************************************************************
    // Update our absolute sim time counter: it is simt that always counts *up*, ignoring MDJ changes both positive and negative.
    // (The Orbiter core does not invoke clkbPreStep for MJD edits: it adjusts simt but not *simdt* on the next call, so that makes it easy.)
    //
    // Note: do NOT use simt in any way for this: simt adjusts with MJD, but simdt does not.
    // WARNING: XR CODE SHOULD *NEVER* INVOKE oapiGetSimTime(): it varies by MJD and so is unreliable for time deltas (which 
    // was the whole point of simt in the first place).  Instead, you should always use simt passed to Area objects (since we 
    // pass absoluteSimtTime in it instead), or invoke VESSEL3_EXT::GetAbsoluteSimTime() if a local simt is not available.  
    // There is also an Area::GetAbsoluteSimTime() convenience method. 
    //
    // I added a #define oapiGetSimTime error to Vessel3Ext.h to prevent XR code from accidentally trying to invoke it.
    //**************************************************************************************************************
    // Note: currently simdt never appears to go negative, but we're being defensive here anyway
    if (simdt > 0)
        m_absoluteSimTime += simdt;

    // DEBUG: sprintf(oapiDebugString(), "GetAbsoluteSimTime()=%lf, simtDoNotUse=%lf", GetAbsoluteSimTime(), simtDoNotUse);

    // ********************************************************************
    // NEW FOR XR1 1.9 release group: pass the absolute simt (is not affected by
    // adjustments to MJD) to all areas so that all MDJ wonkiness is no longer 
    // an issue.
    // ********************************************************************
    const double simt = GetAbsoluteSimTime();

    // invoke all registered PreStep objects
    PreStepIterator it2 = GetPreStepVector().begin();
    for (; it2 != GetPreStepVector().end(); it2++)
    {
        PrePostStep *pStep = *it2;
        pStep->clbkPrePostStep(simt, simdt, mjd);
    }
}

#if 0  // NOT IMPLEMENTED BECAUSE THIS CANNOT YET HANDLE FULL-SCREEN MODES : NOTE: we will not need this now, but let's keep the code in case we need to parse Orbiter.cfg later for any reason (sample code).
// 
// ALERT: THIS METHOD IS ORBITER-VERSION-SPECIFIC, although in theory it should rarely if ever need to be changed.
//
// Retrieve the currently configured video window width in pixels; Note that this is NOT CACHED, since the user may
// exit to the launch pad, change the video panel scale, and relaunch Orbiter.  This would not occur with the default
// 'fast shutdown' mode enabled, but some users may be running with the old shutdown method yet, and so we have to 
// handle that.
//
// NOTE: the *effective video window width* returned by this method is computed as follows:
//      effectiveWidth = actualWidth / panelScale
//
// For example, actualWidth of 1600 and panelScale of 1.25 would yield 1600/1.25 = 1280 pixels.
// Similarly, actualWidth of 1280 and panelScale of 0.75 would yield 1280/0.8 = 1600 pixels.
// 99% of the time, however, the user will be scaling his display UP (panelScale > 1.0) or not at all (panelScale=1.0)
//
// This will read Orbiter.cfg and parse out the video window width.
//
// Returns: > 0 = video window width in pixels, or 0 if the width could not be determined (i.e., incorrect Orbiter version!)
//
int VESSEL3_EXT::GetVideoWindowWidth()
{
    if (m_videoWindowWidth < 0) // not parsed yet?
    {
        // let's read the whole file into a buffer
        const int bufferSize = 8192;   // current Orbiter version's 'Orbiter.cfg' file size is only about 2.5K, so this should be plenty
        char orbiterConfigBuffer[bufferSize];        
        
        FILE *fOrbiterConfig = fopen("Orbiter.cfg", "rt");
        if (fOrbiterConfig == nullptr)
            return 0;       // file not found!

        size_t bytesRead = fread(orbiterConfigBuffer, sizeof(char), bufferSize-1, fOrbiterConfig);  // allow room for tailing zero
        if (ferror(fOrbiterConfig))
            return 0;       // error reading file!

        // Let's remove any zero bytes in the buffer so we don't stop searching for the string below before we reach the end of the buffer.
        // Then we append a trailing zero to terminate the buffer.
        for (char *pWork = orbiterConfigBuffer; pWork < orbiterConfigBuffer + bytesRead; pWork++)
        {
            if (*pWork == 0)
                *pWork = '\n';   // replace with newline
        }
        orbiterConfigBuffer[bytesRead] = 0; // terminate string @ end-of-buffer

        // Don't bother to check for buffer full here; let's search for the setting anyway in the buffer we have.
        const char *pTargetStr = "WindowWidth = ";
        char *pStrValue = strstr(orbiterConfigBuffer, pTargetStr);

        // returns 0 if value cannot be converted
        int actualVideoWidth = atoi(pStrValue + strlen(pTargetStr));      // skip over "WindowWidth = "
        if (actualVideoWidth <= 0)
            return 0;

        // now retrieve the panel scale
        pTargetStr = "PanelScale = ";
        pStrValue = strstr(orbiterConfigBuffer, pTargetStr);
        double panelScale = atof(pStrValue + strlen(pTargetStr));      // skip over "PanelScale = "
        if (panelScale <= 0)
            return 0;

        // effective window width = actual window width / panel scale
        m_videoWindowWidth = static_cast<int>(((static_cast<double>(actualVideoWidth / panelScale)) + 0.5));  // OK to round to the nearest pixel here
    }

    return m_videoWindowWidth;
}
#endif

// Returns the distance to another vessel in meters
double VESSEL3_EXT::GetDistanceToVessel(const VESSEL &targetVessel) const
{
    const VECTOR3 &zero = _V(0,0,0);

    VECTOR3 targetGlobalCoords;
    targetVessel.Local2Global(zero, targetGlobalCoords);

    VECTOR3 ourGlobalCoords;
    Local2Global(zero, ourGlobalCoords);

    return dist(ourGlobalCoords, targetGlobalCoords);
}

// Returns the grapple target vessel with the supplied name, or NULL if target name is invalid or no longer exists.
// NOTE: clients should test the returned object by invoking pGrappleTarget->IsStateDataValid() to ensure that the
// object is fully initialized: each object needs at least two frames in order for its state to be valid.
//
// This should be invoked periodically from your PreStep to obtain data on a given grapple target candidate or when
// you need to render the data.  
//
// Since the returned object is const, subclasses cannot (and *should not*) invoke Update() or any other method that alters the state of the
// returned object.  Also, subclasses should not free the returned object.
//
// NOTE: this method will *always* update the state of pTargetVesselName, such as distance, delta-V, etc. 
//
// pTargetVesselName: may not be null, but may be empty
// Returns: grapple target vessel on success, or NULL if target vessel name not found
const XRGrappleTargetVessel *VESSEL3_EXT::GetGrappleTargetVessel(const char *pTargetVesselName)
{
    XRGrappleTargetVessel *pRetVal = nullptr;

    // locate the vessel
    // Must cast away constness here until Martin fixes the API
    const OBJHANDLE hVessel = oapiGetVesselByName(const_cast<char *>(pTargetVesselName));  // will be NULL if vessel does not exist
    const string sTargetVesselName(pTargetVesselName);   // temporary copy used for lookups

    if (oapiIsVessel(hVessel))    // vessel is still valid?
    {
        // look up the XRGrappleTargetVessel in the cache
        // WARNING: it is possible that a DIFFERENT VESSEL WITH THE SAME NAME AS AN OLD VESSEL is occurring here!  
        // If that is the case the cache will contain stale data for it, so we have to double-check the handle.
        auto it = m_grappleTargetMap.find(&sTargetVesselName);
        if (it == m_grappleTargetMap.end())
        {
reload:
            // not in cache yet; instantiate it
            VESSEL *pTargetVessel = oapiGetVesselInterface(hVessel);  // will never be null
            pRetVal = new XRGrappleTargetVessel(*pTargetVessel, *this);

            // add it to cache; it will be updated below this 'if' block
            m_grappleTargetMap.insert(str_XRGrappleTargetVessel_Pair(new string(pTargetVesselName), pRetVal));   // the string is freed by us when we free the map
        }
        else    // vessel is in cache
        {
            pRetVal = it->second;   // use the cached object

            // Check whether the cache is stale for this object by confirming that both the HANDLES and the VESSEL POINTERS still match!
            // The reason we check for both is because Orbiter sometimes creates a new vessel using the same HANDLE as an old (now-deleted) vessel.
            const OBJHANDLE hCachedVessel = pRetVal->GetTargetHandle();
            const VESSEL *pCachedVessel = pRetVal->GetTargetVessel();
            const VESSEL *pCurrentVessel = oapiGetVesselInterface(hVessel);  // will always succeed because handle is valid
            if ((pCachedVessel != pCurrentVessel) || (hCachedVessel != hVessel))
            {
                // cache is stale!
                // free the map elements
                EraseIteratorItemFirstSecond(m_grappleTargetMap, it);
                goto reload;     // reload the cache element for this vessel
            }
        }

        //===============================================
        // pRetVal will never be null here
        if (pRetVal->Update() == false)    // Update the state of the grapple target vessel 
        {
            // target vessel deleted!
            // remove from cache since it is invalid now 
            // NOTE: we must keep the cache clean since it is possible for a *future* vessel to have the same handle!
            auto it = m_grappleTargetMap.find(&sTargetVesselName);
            if (it != m_grappleTargetMap.end()) // should always succeed
            {
                // free the map elements
                EraseIteratorItemFirstSecond(m_grappleTargetMap, it);
            }

            pRetVal = nullptr;  // object is invalid
        }
    }
    else    // vessel no longer exists!
    {
        // remove from cache since it is invalid now 
        // NOTE: we must keep the cache clean since it is possible for a *future* vessel to have the same handle!
        auto it = m_grappleTargetMap.find(&sTargetVesselName);
        if (it != m_grappleTargetMap.end())     // in cache?
        {
            // free the map elements
            EraseIteratorItemFirstSecond(m_grappleTargetMap, it);
        }
    }

    return pRetVal;
}

// WARNING: you must invoke this to work around Orbiter core bug:
// Orbiter uses data in flag[0] in DefSetState, but GetState() does not set those flags to zero!  
// They are unitialized!
void VESSEL3_EXT::GetStatusSafe(VESSELSTATUS2 &status) const
{
    VESSEL3_EXT::GetStatusSafe(*this, status, false);  // invoke static method
}

// Static version of GetStatusSafe that takes a non-XR-class vessel.
// 
// resetToDefault: if true, reset extraneous fields in structure to empty.
void VESSEL3_EXT::GetStatusSafe(const VESSEL &vessel, VESSELSTATUS2 &status, const bool resetToDefault)
{
    // Initialize entire structure to zero before invoking the read from the core.
    memset(&status, 0, sizeof(status));
    status.version = 2;   // retrieve version 2 = VESSELSTATUS2
    vessel.GetStatusEx(&status);

    if (resetToDefault)   // initialize to default settings?
    {
        // Note: there is no config file for this vessel (it was created dynamically), so we have to wing it here...

        // The FUELSPEC structure here is static and contains read-only values: it cannot be a local because it must remain valid after this
        // method exits so the caller can use it in an oapiCreateVesselEx call.
        const static VESSELSTATUS2::FUELSPEC s_pFuelSpecs[3] = 
        {
            // idx, level
            {0, 1.0},   // full main tank
            {1, 1.0},   // full RCS tank
            {2, 1.0}    // full SCRAM tank
        };
        
        // reset/initialize VESSELSTATUS2 fields 
        status.flag = VS_THRUSTRESET | VS_FUELLIST ;   // reset all thrusters to zero, set fuel levels
        status.nfuel = 3;  // three fuel tanks

        // NOTE: Orbiter core incorrectly specifies non-const structure here, so we have to hack it
        status.fuel = const_cast<VESSELSTATUS2::FUELSPEC *>(s_pFuelSpecs);         
        
        status.nthruster = 0;
        status.thruster = nullptr;

        status.ndockinfo = 0;
        status.dockinfo = nullptr;

        status.xpdr = 639;  // TODO: what should we set this to???  (range is 0-640)
        
        // WARNING: status.base must be 0 for all attached vessels or Orbiter will CTD/do weird things!
        status.base = 0;
        status.port = -1;
    }
}

// Returns true if the user's actual day is a match, or false if not.  This is useful for easter eggs.
// month=1-12, day=1-31
bool VESSEL3_EXT::IsToday(WORD wMonth, WORD wDay)  
{
    SYSTEMTIME st;
    GetLocalTime(&st);

    return ((wMonth == st.wMonth) && (wDay == st.wDay));
}

// Set a mesh group visible or invisible
// Note: dwMeshGroup is 0-based
void VESSEL3_EXT::SetMeshGroupVisible(DEVMESHHANDLE hMesh, DWORD dwMeshGroup, bool isVisible)
{
    // Note: for details on mesh group flags, refer to page 7 of 3DModel.pdf.
    /*
        Mesh type   Flag        Interpretation
        ---------   ----------  ----------------------------------------------------
        Vessel      0x00000001  Do not use this group to render ground shadows
        Vessel      0x00000002  Do not render this group
        Vessel      0x00000004  Do not apply lighting when rendering this group
        Vessel      0x00000008  Texture blending directive: additive with background
    */

    GROUPEDITSPEC geSpec;
    memset(&geSpec, 0, sizeof(GROUPEDITSPEC));  // init all to zero
    geSpec.UsrFlag = 0x00000003;    // toggle shadows as well; this will be ANDed or ORd with the group's flags

    // ORG: MESHGROUP *pGroup = oapiMeshGroup(hMesh, dwMeshGroup);  // should never be null!
    if (isVisible)  // assignment separated for clarity
        // ORG: pGroup->UsrFlag &= ~renderFlag; // clear the "do not render" bits
        geSpec.flags = GRPEDIT_DELUSERFLAG;  // clear the "do not render" bits
    else
        // ORG: pGroup->UsrFlag |= renderFlag;  // set the "do not render" bits
        geSpec.flags = GRPEDIT_ADDUSERFLAG;  // set the "do not render" bits

    oapiEditMeshGroup(hMesh, dwMeshGroup, &geSpec);
}

// Resets all the fuel levels in the supplied vessel to the supplied fraction (0...1)
// Returns the # of fuel tanks in the vessel
int VESSEL3_EXT::ResetAllFuelLevels(VESSEL *pVessel, const double levelFrac)
{
    _ASSERTE(pVessel != nullptr);
    _ASSERTE(levelFrac >= 0);
    _ASSERTE(levelFrac <= 1.0);

    DWORD dwPropCount = pVessel->GetPropellantCount();
    for (DWORD i = 0; i < dwPropCount; i++)
    {
        PROPELLANT_HANDLE ph = pVessel->GetPropellantHandleByIndex(i);
        const double maxPropMass = pVessel->GetPropellantMaxMass(ph);
        pVessel->SetPropellantMass(ph, maxPropMass * levelFrac);
    }

    return (int)dwPropCount;
}

// get distance and name of landing target (closest surface base) [vessel altitude is ignored]
// returns: true on success, false if no target available
bool VESSEL3_EXT::GetLandingTargetInfo(double &distanceOut, char * const pBaseNameOut, const int nameBufferLength)
{
    VESSELSTATUS2 shipStatus;
    GetStatusSafe(shipStatus);
    const OBJHANDLE hTarget = shipStatus.base;    
    if ((hTarget == nullptr) || (oapiGetObjectType(hTarget) != OBJTP_SURFBASE))  // WARNING: oapiGetObjectType will CTD if hTarget == nullptr!
        return false;  // no base in range

    oapiGetObjectName(hTarget, pBaseNameOut, nameBufferLength);  // gets the name

    // get base location
    double baseLng, baseLat, planetRadius;
    oapiGetBaseEquPos(hTarget, &baseLng, &baseLat, &planetRadius);

    // now compute the distance from our vessels surface position to the base (vessel altitude is ignored)
    // Note: all ship and planet latitude & longitude values are in radians
    const double theta = baseLng - shipStatus.surf_lng;
    distanceOut = acos( 
                        (sin(baseLat) * sin(shipStatus.surf_lat)) + 
                        (cos(baseLat) * cos(shipStatus.surf_lat) * cos(theta)) 
                      ) * planetRadius;  // multiply by radius in meters to get distance in meters

    return true;
}

// Static method that returns variable volume based on a level (0..1).
// Note: level may be outside range of 0..1; this is not an error, but it will be limited to between 0 and 1.
float VESSEL3_EXT::ComputeVariableVolume(const double minVolume, const double maxVolume, double level)
{
    _ASSERTE(minVolume >= 0);
    _ASSERTE(maxVolume <= 1.0);
    _ASSERTE(minVolume <= maxVolume);

    if (level < 0)
        level = 0;
    else if (level > 1.0)
        level = 1.0;

    const float variableVolume = (static_cast<float>(maxVolume) - static_cast<float>(minVolume)) * static_cast<float>(level);
    return (static_cast<float>(minVolume) + variableVolume);
}
