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
// XR1 Base Class Library
// These classes extend and use the XR Framework classes
//
// XR1PayloadScreenAreas.h
//
// Generic payload screen areas and components for all XR vessels;
// These are NOT used by the XR1!  They are here for sublcasses to use.
// ==============================================================

#include "XR1PayloadBay.h"   // must be FIRST because of forward reference to XR1PayloadBay in DeltaGliderXR1.h
#include "DeltaGliderXR1.h"
#include "XR1PayloadScreenAreas.h"
#include "XRPayloadBaySlot.h"

//----------------------------------------------------------------------------------

const COORD2 &DeployPayloadArea::s_screenSize    =  _COORD2(210, 145);  // screen size in pixels
const COORD2 &PayloadThumbnailArea::s_screenSize =  _COORD2(154, 77);   // screen size in pixels
const COORD2 &GrapplePayloadArea::s_screenSize   =  _COORD2(210, 145);   // screen size in pixels

//=========================================================================

// isOn = reference to status variable: true = light on, false = light off
// idb* = resource IDs
DeployPayloadArea::DeployPayloadArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID,
                                     const int idbDeployPayloadOrbit, const int idbDeployPayloadLanded) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_hSurfaceForOrbit(nullptr), m_hSurfaceForLanded(nullptr), m_hFont(0),
    m_mouseHoldTargetSimt(-1), m_lastAction(RATE_ACTION::ACT_NONE), m_repeatCount(0),
    m_repeatSpeed(0.0625),   // seconds between clicks if mouse held down: 16 clicks per second
    m_idbDeployPayloadOrbit(idbDeployPayloadOrbit), m_idbDeployPayloadLanded(idbDeployPayloadLanded)
{
    m_deployButton    = _COORD2(  5, 129);
    m_deployAllButton = _COORD2(128, 129);

    // Delta-V buttons
    const int cyTop = 95;
    const int cyBot = 104;
    
    m_rateUp1ArrowCoord    = _COORD2(124 ,cyTop);
    m_rateDown1ArrowCoord  = _COORD2(124, cyBot);
    m_rateUp5ArrowCoord    = _COORD2(108, cyTop);
    m_rateDown5ArrowCoord  = _COORD2(108, cyBot);
    m_rateUp25ArrowCoord   = _COORD2( 92, cyTop);
    m_rateDown25ArrowCoord = _COORD2( 92, cyBot);
    m_resetButtonCoord     = _COORD2(141, 99);
}

void DeployPayloadArea::Activate()
{
    Area::Activate();  // invoke superclass method
    // register area
    // specify both PANEL_REDRAW_ALWAYS and PANEL_REDRAW_MOUSE because we need explicit mouse events
    // Note that refresh rates are managed above us by clbkPanelRedrawEvent.
    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(s_screenSize.x, s_screenSize.y), 
        PANEL_REDRAW_ALWAYS | PANEL_REDRAW_MOUSE, 
        PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBPRESSED | PANEL_MOUSE_LBUP, 
        PANEL_MAP_BGONREQUEST);

    m_hSurfaceForOrbit  = CreateSurface(m_idbDeployPayloadOrbit);
    m_hSurfaceForLanded = CreateSurface(m_idbDeployPayloadLanded);
    
    m_hFont = CreateFont(12, 0, 0, 0, 600, 0, 0, 0, 0, 0, 0, 0, FF_MODERN, "Microsoft Sans Serif");
}
 
void DeployPayloadArea::Deactivate()
{
    DestroySurface(&m_hSurfaceForOrbit);
    DestroySurface(&m_hSurfaceForLanded);

    DeleteObject(m_hFont);
    XR1Area::Deactivate();  // invoke superclass method
}

bool DeployPayloadArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    if (GetXR1().m_internalSystemsFailure)  
    {
        // Systems overheating!  Keep the screen black.
        // Note: given how rarely this condition occurs it is not worth tracking whether we already blitted a blank screen; 
        // therefore, we always re-blit it.
        oapiBltPanelAreaBackground(GetAreaID(), surf);
        return true;       
    }

    // Always re-render everything; it is too error-prone to try to track all values and clear any 
    // old data underneath from the previous render.
        
    // render the background based on whether the ship is landed and STOPPED
    const SURFHANDLE hSurface = (GetXR1().IsLanded() ? m_hSurfaceForLanded : m_hSurfaceForOrbit);
    DeltaGliderXR1::SafeBlt(surf, hSurface, 0, 0, 0, 0, s_screenSize.x, s_screenSize.y);

    // get the current selected slot, if any
    XRPayloadBay *pPayloadBay = GetXR1().m_pPayloadBay;
    const int selectedSlotNumber = GetXR1().m_selectedSlot; 
    const VESSEL *pChildVessel = ((selectedSlotNumber == 0) ? nullptr : pPayloadBay->GetChild(GetXR1().m_selectedSlot));
    const XRPayloadClassData *pChildVesselPCD = ((pChildVessel != nullptr) ? &XRPayloadClassData::GetXRPayloadClassDataForClassname(pChildVessel->GetClassName()) : nullptr);

    // obtain device context and save existing font
    HDC hDC = GetDC(surf);
    HFONT hPrevObject = (HFONT)SelectObject(hDC, m_hFont);

    SetBkMode(hDC, TRANSPARENT);
    SetTextColor(hDC, CREF(LIGHT_YELLOW));  // use CREF macro to convert to Windows' Blue, Green, Red COLORREF
    SetTextAlign(hDC, TA_LEFT);

    int textY = 2;
    const int pitch = 12;
    char msg[256];
    if (pChildVessel != nullptr)
    {
        // Note: pChildVesselPCD is never null here
        // DESC
        TextOut(hDC, 39, textY, pChildVesselPCD->GetDescription(), static_cast<int>(strlen(pChildVesselPCD->GetDescription())));  // length may exceed displayable area; this is OK

        // MASS
        textY += pitch;
        sprintf(msg, "%.2f kg", pChildVessel->GetMass());
        TextOut(hDC, 39, textY, msg, static_cast<int>(strlen(msg)));

        // DIMENSIONS
        textY += pitch;
        VECTOR3 dim = pChildVesselPCD->GetDimensions();
        sprintf(msg, "%.2f L x %.2f W x %.2f H", dim.z, dim.x, dim.y); 
        TextOut(hDC, 74, textY, msg, static_cast<int>(strlen(msg)));

        // MODULE NAME
        textY += pitch;
        const char *pName = pChildVessel->GetName();
        SetTextColor(hDC, CREF(CYAN));  // so user can find it instantly
        TextOut(hDC, 85, textY, pName, static_cast<int>(strlen(pName)));
        SetTextColor(hDC, CREF(LIGHT_YELLOW));  // restore default color

        // SLOTS OCCUPIED
        textY += pitch;
        VECTOR3 slots = pChildVesselPCD->GetSlotsOccupied();  
        sprintf(msg, "%.1f L x %.1f W x %.1f H", slots.z, slots.x, slots.y); 
        TextOut(hDC, 98, textY, msg, static_cast<int>(strlen(msg)));
    }
    else
        textY += (4 * pitch);

    // SELECTED BAY SLOT
    textY += pitch;
    if (pChildVessel != nullptr)
    {
        SetTextColor(hDC, MEDIUM_GREEN);
        _itoa(GetXR1().m_selectedSlot, msg, 10);
    }
    else
    {
        if (selectedSlotNumber == 0)
        {
            // no slot selected
            // use default color
            strcpy(msg, "NONE");
            
        }
        else   // slot selected but empty
        {
            sprintf(msg, "%d (EMPTY)", GetXR1().m_selectedSlot);
            strcpy(msg, msg);
            SetTextColor(hDC, CREF(LIGHT_RED));  // use CREF macro to convert to Windows' Blue, Green, Red COLORREF
        }
    }
    TextOut(hDC, 118, textY, msg, static_cast<int>(strlen(msg)));
    SetTextColor(hDC, LIGHT_YELLOW);     // restore default color

    if (GetXR1().IsLanded())
    {
        // only render this if we have selected a slot with cargo
        if (pChildVessel != nullptr)
        {
            // DEPLOY TO
            VECTOR3 c = pPayloadBay->GetLandedDeployToCoords(GetXR1().m_selectedSlot);
            sprintf(msg, "X: %.1f, Y: %.1f, Z: %.1f", c.x, c.y, c.z);
            TextOut(hDC, 69, 92, msg, static_cast<int>(strlen(msg)));
        }
    }
    else    // in orbit; always allow Delta-V to be set regardless of whether cargo is selected.
    {
        sprintf(msg, "%+.1f", GetXR1().m_deployDeltaV);
        SetTextColor(hDC, CREF(LIGHT_BLUE));  // use CREF macro to convert to Windows' Blue, Green, Red COLORREF
        SetTextAlign(hDC, TA_RIGHT);
        TextOut(hDC, 87, 96, msg, static_cast<int>(strlen(msg)));
    }

    // restore previous font and release device context
    SelectObject(hDC, hPrevObject);
    ReleaseDC(surf, hDC);

    return true;
}

// Handle mouse events for this area
// event = Orbiter event flags
// mx, my = mouse coordinates relative to the area
// returns: true if event processed, false if not
bool DeployPayloadArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated or systems failure, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard() || GetXR1().m_internalSystemsFailure)
        return false;

    const COORD2 c = { mx, my };

    // check DEPLOY and DEPLOY ALL buttons (active for both modes)
    if (event & PANEL_MOUSE_LBDOWN)
    {
        if (c.InBounds(m_deployButton, 9, 9))
        {
            GetXR1().DeployPayload(GetXR1().m_selectedSlot, true);   // ignore return code
            return true;
        }
        else if (c.InBounds(m_deployAllButton, 9, 9))
        {
            GetXR1().DeployAllPayload();    // selected slot is unchanged; ignore return code here
            return true;
        }
    }

    if (GetXR1().IsLanded() == false)
    {
        // ORBIT MODE
        if (event & PANEL_MOUSE_LBDOWN)
        {
            m_repeatCount = 0;  // reset just in case

            // check reset button
            if (c.InBounds(m_resetButtonCoord, 7, 7))
            {
                GetXR1().SetPayloadDeployDeltaV(0, true);
                return true;
            }
        }

        // check rate buttons
        bool showMessage = false;
        RATE_ACTION action = RATE_ACTION::ACT_NONE;
        if (event & (PANEL_MOUSE_LBDOWN | PANEL_MOUSE_LBPRESSED))
        {
            const double simt = GetAbsoluteSimTime();

            // if TRUE, process a button click
            bool doButtonClick = false;

            if (event & PANEL_MOUSE_LBDOWN)
            {
                // mouse just clicked; always process it immediately
                doButtonClick = true;
                showMessage = true;

                // next click if mouse held down is 0.75 second from now
                m_mouseHoldTargetSimt = simt + 0.75;
            }

            // check whether we reached our target hold time
            if ((m_mouseHoldTargetSimt > 0) && (simt >= m_mouseHoldTargetSimt))
            {
                doButtonClick = true;
                m_mouseHoldTargetSimt = simt + m_repeatSpeed;   // process another event if mouse held down long enough
                m_repeatCount++;        // remember this
            }

            // check rate arrows
            // Note: use NORMAL notation here; down arrow DECREMENTS rate and vice-versa
            if (c.InBounds(m_rateDown1ArrowCoord, 7, 6))
            {
                if (doButtonClick)
                {
                    m_lastAction = action = RATE_ACTION::DECRATE1;
                }
            }
            else if (c.InBounds(m_rateUp1ArrowCoord, 7, 6))
            {
                if (doButtonClick)
                {
                    m_lastAction = action = RATE_ACTION::INCRATE1;
                }
            }
            else if (c.InBounds(m_rateDown5ArrowCoord, 7, 6))
            {
                if (doButtonClick)
                {
                    m_lastAction = action = RATE_ACTION::DECRATE5;
                }
            }
            else if (c.InBounds(m_rateUp5ArrowCoord, 7, 6))
            {
                if (doButtonClick)
                {
                    m_lastAction = action = RATE_ACTION::INCRATE5;
                }
            }
            else if (c.InBounds(m_rateDown25ArrowCoord, 7, 6))
            {
                if (doButtonClick)
                {
                    m_lastAction = action = RATE_ACTION::DECRATE25;
                }
            }
            else if (c.InBounds(m_rateUp25ArrowCoord, 7, 6))
            {
                if (doButtonClick)
                {
                    m_lastAction = action = RATE_ACTION::INCRATE25;
                }
            }
            else
            {
                // mouse is outside of any buttons!
                // NOTE: technically we should check if the mouse moves off of the FIRST button clicked, but it's not worth the 
                // effort since there is blank space between the buttons anyway.
                m_mouseHoldTargetSimt = -1;
            }
        }
        else if (event & (PANEL_MOUSE_LBUP))
        {
            // mouse released; reset hold timer
            m_mouseHoldTargetSimt = -1;

            // re-issue the last action so a message is logged about the final state now IF we were repeating the button clicks
            if (m_repeatCount > 0)
            {
                action = m_lastAction;
                showMessage = true;  // show final message and play button up sound
                m_repeatCount = 0;   // reset
            }

            m_lastAction = RATE_ACTION::ACT_NONE;  // reset
        }

        if (action != RATE_ACTION::ACT_NONE)
        {
            switch (action)
            {
            case RATE_ACTION::INCRATE1:
                GetXR1().AdjustPayloadDeployDeltaV(+0.1, showMessage);
                return true;

            case RATE_ACTION::DECRATE1:
                GetXR1().AdjustPayloadDeployDeltaV(-0.1, showMessage);
                return true;

            case RATE_ACTION::INCRATE5:
                GetXR1().AdjustPayloadDeployDeltaV(+0.5, showMessage);
                return true;

            case RATE_ACTION::DECRATE5:
                GetXR1().AdjustPayloadDeployDeltaV(-0.5, showMessage);
                return true;

            case RATE_ACTION::INCRATE25:
                GetXR1().AdjustPayloadDeployDeltaV(+2.5, showMessage);
                return true;

            case RATE_ACTION::DECRATE25:
                GetXR1().AdjustPayloadDeployDeltaV(-2.5, showMessage);
                return true;

            default:
                // no action
                break;
            }
        }
    }  // ORBIT mode

    return false;
}

//=========================================================================

// Render the payload bitmap for the current slot, if any

// isOn = reference to status variable: true = light on, false = light off
// idbPayloadThumbnailNone = resource ID
PayloadThumbnailArea::PayloadThumbnailArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int idbPayloadThumbnailNone) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_hNoneSurface(nullptr), m_idbPayloadThumbnailNone(idbPayloadThumbnailNone)
{
}

void PayloadThumbnailArea::Activate()
{
    Area::Activate();  // invoke superclass method
    // register area
    // Note that refresh rates are managed above us by clbkPanelRedrawEvent.
    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(s_screenSize.x, s_screenSize.y), PANEL_REDRAW_ALWAYS, PANEL_MAP_BGONREQUEST);

    m_hNoneSurface = CreateSurface(m_idbPayloadThumbnailNone);  // "none" screen
}
 
void PayloadThumbnailArea::Deactivate()
{
    DestroySurface(&m_hNoneSurface);
    XR1Area::Deactivate();  // invoke superclass method
}

// Returns: true if area redrawn, false if no changes made
bool PayloadThumbnailArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    if (GetXR1().m_internalSystemsFailure)  
    {
        // Systems overheating!  Keep the screen black.
        // Note: given how rarely this condition occurs it is not worth tracking whether we already blitted a blank screen; 
        // therefore, we always re-blit it.
        oapiBltPanelAreaBackground(GetAreaID(), surf);
        return true;       
    }

    bool retVal = false;

    // get the current selected slot, if any
    XRPayloadBay *pPayloadBay = GetXR1().m_pPayloadBay;
    const VESSEL *pVesselForThumbnail = ((GetXR1().m_selectedSlot == 0) ? nullptr : pPayloadBay->GetChild(GetXR1().m_selectedSlot));
    
    // if EMPTY selected slot, try the grapple payload target...
    if (pVesselForThumbnail == nullptr)  // no selected slot with a vessel?
    {
        // check whether a slot is selected (which means the slot is empty)
        if (GetXR1().m_selectedSlot != 0)
        {
            // empty slot is selected
            // get the targeted vessel, if any; may be null!
            const XRGrappleTargetVessel *pGrappleTargetVessel = GetXR1().GetGrappleTargetVessel(GetXR1().m_grappleTargetVesselName);  // pulls data from cache (this logic is in the framework classes)
            const bool isGrappleTargetValidAndInRange = GetXR1().IsGrappleTargetVesselValidAndInDisplayRange(pGrappleTargetVessel);

            if (isGrappleTargetValidAndInRange)
                pVesselForThumbnail = pGrappleTargetVessel->GetTargetVessel();  // will never be null here
        }
    }

    // Note: if NO selected slot, pVesselForThumbnail == null and this screen will show "none"

    const XRPayloadClassData *pChildVesselPCD = ((pVesselForThumbnail != nullptr) ? &XRPayloadClassData::GetXRPayloadClassDataForClassname(pVesselForThumbnail->GetClassName()) : nullptr);

    // render the screen if it has changed since the last render OR if this is the inital render
    if ((pChildVesselPCD != m_pLastRenderedPayloadThumbnailPCD) || (event == PANEL_REDRAW_INIT))
    {
        if (pChildVesselPCD != nullptr)
        {
            const HBITMAP hThumb = pChildVesselPCD->GetThumbnailBitmapHandle();  // may be null, but normally should not be
            if (hThumb == nullptr)
            {
                // render a black screen so the user knows his thumbnail path is invalid
                // TODO: figure out why this does not work!  Oddly, it *does* work on system overheat (see at top of this method)
                oapiBltPanelAreaBackground(GetAreaID(), surf);
            }
            else  // thumbnail is OK
            {
                // WARNING: we cannot use SafeBlt to blit a bitmap here!  We must use the BitBlt Win32 call instead.
                // obtain device context and save existing object
                HDC hDC = GetDC(surf);
                
                HDC hMemDC = CreateCompatibleDC(hDC);   // create an in-memory DC
                SelectObject(hMemDC, hThumb);  // select the bitmap image into the in-memory DC
                BitBlt(hDC, 0, 0, s_screenSize.x, s_screenSize.y, hMemDC, 0, 0, SRCCOPY);  // copy new bitmap to the screen
                DeleteDC(hMemDC);       // clean up

                // release device context
                ReleaseDC(surf, hDC);
            }
        }
        else  // Blit the "none" screen
        {
            DeltaGliderXR1::SafeBlt(surf, m_hNoneSurface, 0, 0, 0, 0, s_screenSize.x, s_screenSize.y);
        }

        // save the PCD of the last rendered bitmap image; may be null
        m_pLastRenderedPayloadThumbnailPCD = pChildVesselPCD;
        
        retVal = true;
    }

    return retVal;
}

//=========================================================================

// isOn = reference to status variable: true = light on, false = light off
GrapplePayloadArea::GrapplePayloadArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const int idbGrapplePayload) :
    XR1Area(parentPanel, panelCoordinates, areaID),
    m_hSurface(nullptr), m_hFont(0), 
    m_idbGrapplePayload(idbGrapplePayload)
{
    m_rangeButton      = _COORD2(  4, 100);   
    m_grappleButton    = _COORD2(  4, 115);
    m_grappleAllButton = _COORD2(121, 115);
    m_targetButtonUp   = _COORD2( 40, 128);
    m_targetButtonDown = _COORD2( 40, 137);
    m_clearButton      = _COORD2(156, 100);
    // no way to do this: m_trackButton      = _COORD2(156,  86);
}

void GrapplePayloadArea::Activate()
{
    Area::Activate();  // invoke superclass method
    // register areaD
    // specify both PANEL_REDRAW_ALWAYS and PANEL_REDRAW_MOUSE because we need explicit mouse events
    // Note that refresh rates are managed above us by clbkPanelRedrawEvent.
    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(s_screenSize.x, s_screenSize.y), PANEL_REDRAW_ALWAYS | PANEL_REDRAW_MOUSE, PANEL_MOUSE_LBDOWN, PANEL_MAP_BGONREQUEST);

    m_hSurface = CreateSurface(m_idbGrapplePayload);
    
    m_hFont = CreateFont(12, 0, 0, 0, 600, 0, 0, 0, 0, 0, 0, 0, FF_MODERN, "Microsoft Sans Serif");
}
 
void GrapplePayloadArea::Deactivate()
{
    DestroySurface(&m_hSurface);

    DeleteObject(m_hFont);
    XR1Area::Deactivate();  // invoke superclass method
}

bool GrapplePayloadArea::Redraw2D(const int event, const SURFHANDLE surf)
{
    if (GetXR1().m_internalSystemsFailure)  
    {
        // Systems overheating!  Keep the screen black.
        // Note: given how rarely this condition occurs it is not worth tracking whether we already blitted a blank screen; 
        // therefore, we always re-blit it.
        oapiBltPanelAreaBackground(GetAreaID(), surf);  // background is a black screen (from the panel bitmap)
        return true;       
    }

    // Always re-render everything; it is too error-prone to try to track all values and clear any 
    // old data underneath from the previous render.
        
    // render the background based on whether the ship is landed and STOPPED (background changes)
    DeltaGliderXR1::SafeBlt(surf, m_hSurface, 0, 0, 0, 0, s_screenSize.x, s_screenSize.y);

    // get the targeted vessel, if any; may be null!
    const XRGrappleTargetVessel *pGrappleTargetVessel = GetXR1().GetGrappleTargetVessel(GetXR1().m_grappleTargetVesselName);  // pulls cached object w/updated distance, delta-V, etc. (this logic is in the framework classes)

    // obtain device context and save existing font
    HDC hDC = GetDC(surf);
    HFONT hPrevObject = (HFONT)SelectObject(hDC, m_hFont);

    SetBkMode(hDC, TRANSPARENT);
    SetTextColor(hDC, CREF(LIGHT_YELLOW));  // use CREF macro to convert to Windows' Blue, Green, Red COLORREF
    SetTextAlign(hDC, TA_LEFT);

    int textY = 0;
    const int pitch = 12;
    char msg[128];
    const double range = GetXR1().GetGrappleDisplayRange();  // this is always valid
    const bool isGrappleTargetValidAndInRange = GetXR1().IsGrappleTargetVesselValidAndInDisplayRange(pGrappleTargetVessel);  // pGrappleTargetVessel may be null here
    
    // only show the grapple target if it is in range
    if (isGrappleTargetValidAndInRange)  
    {
        // grapple target vessel is valid and in range
        VESSEL *pTargetVessel = pGrappleTargetVessel->GetTargetVessel();  // will never be null here
        const XRPayloadClassData &grappleTargetPCD = pGrappleTargetVessel->GetTargetPCD();  // will never be null

        // DESC
        TextOut(hDC, 39, textY, grappleTargetPCD.GetDescription(), static_cast<int>(strlen(grappleTargetPCD.GetDescription())));  // length may exceed displayable area; this is OK

        // MASS
        textY += pitch;
        sprintf(msg, "%.2f kg", pTargetVessel->GetMass());
        TextOut(hDC, 39, textY, msg, static_cast<int>(strlen(msg)));

        // DISTANCE
        textY += pitch;
        const double distance = pGrappleTargetVessel->GetDistance(); 
        const double grappleRangeLimit = GetXR1().GetPayloadGrappleRangeLimit();
        sprintf(msg, "%.1f m", distance);
        // render color depending on whether the target is in grapple range 
        DWORD dwColor;
        if (distance > grappleRangeLimit)
            dwColor = LIGHT_RED;     // out-of-range
        else if (distance >= (grappleRangeLimit * .80))
            dwColor = BRIGHT_YELLOW;
        else
            dwColor = MEDIUM_GREEN;
        
        SetTextColor(hDC, CREF(dwColor));
        TextOut(hDC, 61, textY, msg, static_cast<int>(strlen(msg)));
        SetTextColor(hDC, CREF(LIGHT_YELLOW));  // restore default color

        // DELTA-V
        textY += pitch;
        const double deltaV = pGrappleTargetVessel->GetDeltaV();
        sprintf(msg, "%.2f m/s", deltaV);
        // render color depending on whether the target is in delta-V range
        if (fabs(deltaV) > PAYLOAD_GRAPPLE_MAX_DELTAV)
            dwColor = LIGHT_RED;     // out-of-range
        else if (fabs(deltaV) >= (PAYLOAD_GRAPPLE_MAX_DELTAV * .80))
            dwColor = BRIGHT_YELLOW;
        else
            dwColor = MEDIUM_GREEN;
        
        SetTextColor(hDC, CREF(dwColor));  // restore default color
        TextOut(hDC, 53, textY, msg, static_cast<int>(strlen(msg)));
        SetTextColor(hDC, CREF(LIGHT_YELLOW));  // restore default color

        // DIMENSIONS
        textY += pitch;
        VECTOR3 dim = grappleTargetPCD.GetDimensions();
        sprintf(msg, "%.2f L x %.2f W x %.2f H", dim.z, dim.x, dim.y); 
        TextOut(hDC, 74, textY, msg, static_cast<int>(strlen(msg)));

        // MODULE NAME
        textY += pitch;
        const char *pName = pTargetVessel->GetName();
        SetTextColor(hDC, CREF(CYAN));  // so user can find it instantly
        TextOut(hDC, 85, textY, pName, static_cast<int>(strlen(pName)));
        SetTextColor(hDC, CREF(LIGHT_YELLOW));  // restore default color

        // SLOTS OCCUPIED
        textY += pitch;
        VECTOR3 slots = grappleTargetPCD.GetSlotsOccupied();  
        sprintf(msg, "%.1f L x %.1f W x %.1f H", slots.z, slots.x, slots.y); 
        TextOut(hDC, 98, textY, msg, static_cast<int>(strlen(msg)));
    }
    else
        textY += (6 * pitch);

    // SELECTED BAY SLOT
    // get the current selected slot, if any
    XRPayloadBay *pPayloadBay = GetXR1().m_pPayloadBay;
    const int selectedSlot = GetXR1().m_selectedSlot;  // 0 = NONE
    const VESSEL *pChildVessel = ((selectedSlot == 0) ? nullptr : pPayloadBay->GetChild(GetXR1().m_selectedSlot));

    if (selectedSlot > 0)
    {
        XRPayloadBaySlot *pSlot = pPayloadBay->GetSlot(selectedSlot);

        // check whether any grapple target vessel is selected and in range
        if (isGrappleTargetValidAndInRange)
        {
            // A grapple target is selected; check slot space and obtain list of slots required for the targeted grapple vessel
            VESSEL *pTargetVessel = pGrappleTargetVessel->GetTargetVessel();  // will never be null here
            vector<XRPayloadBaySlot *> vOut; // will contain list of slots required for targeted vessel
            // ORG: const bool wouldFit = pSlot->GetRequiredNeighborSlotsForCandidateVessel(*pTargetVessel, vOut);
            
            // check whether the slot itself or any of the *required* slots are occupied
            const bool wouldFit = pSlot->CheckSlotSpace(*pTargetVessel);
            if (pSlot->IsOccupied())
            {
                sprintf(msg, "%d (OCCUPIED)", selectedSlot);
                SetTextColor(hDC, CREF(LIGHT_RED));  // use CREF macro to convert to Windows' Blue, Green, Red COLORREF
            }
            else if (wouldFit == false)
            {
                sprintf(msg, "%d (NO ROOM)", selectedSlot);
                SetTextColor(hDC, CREF(LIGHT_RED));  // use CREF macro to convert to Windows' Blue, Green, Red COLORREF
            }
            else
            {
                // slot is OK; render in green
                SetTextColor(hDC, CREF(MEDIUM_GREEN));  // use CREF macro to convert to Windows' Blue, Green, Red COLORREF
                sprintf(msg, "%d (OK)", selectedSlot);
            }
        }
        else  // slot selected, but no grapple payload targeted OR target is out-of-range
        {
            // if slot is OCCUPIED, show a different message
            if (pSlot->IsOccupied())
            {
                // color is unchanged; this is not a serious issue since no payload is in range at this time.
                sprintf(msg, "%d (OCCUPIED)", selectedSlot);
            }
            else  // slot is free
            {
                // color is unchanged
                sprintf(msg, "%d", selectedSlot);   
            }
        }
    }
    else  // no slot selected
    {
        // color is unchanged
        strcpy(msg, "NONE");
    }
    
    // render the 'selected bay slot' value
    textY += pitch;
    TextOut(hDC, 118, textY, msg, static_cast<int>(strlen(msg)));
    SetTextColor(hDC, CREF(LIGHT_YELLOW));  // revert to default color

    // RANGE
    sprintf(msg, "%.0f m", range);    // e.g., "500 m"
    TextOut(hDC, 84, 98, msg, static_cast<int>(strlen(msg)));

    // target X of Y
    const int totalVesselsInRange = static_cast<int>(GetXR1().m_xrGrappleTargetVesselsInDisplayRange.size());
    if (isGrappleTargetValidAndInRange == false)   // no target selected or it is not in range?
    {
        if (totalVesselsInRange == 0)
        {
            sprintf(msg, "No modules in range.");
        }
        else    // at least one vessel in range
        {
            // color is unchanged
            sprintf(msg, "Click to select (%d in range)", totalVesselsInRange);
        }
    }
    else    // a target is selected and in range!
    {
        // Find the index of the selected grapple target; this will always succeed because we asserted that the target vessel is in range.
        int index;  // we need the value after the loop terminates
        for (index=0; index < totalVesselsInRange; index++)
        {
            const XRGrappleTargetVessel *pCandidate = GetXR1().m_xrGrappleTargetVesselsInDisplayRange[index];
            if (*pGrappleTargetVessel == *pCandidate)
            {
                break;  // found it; this should always succeed before the loop ends, so we don't add additional checks here
            }
        }

        // render in green
        SetTextColor(hDC, CREF(MEDIUM_GREEN));  // use CREF macro to convert to Windows' Blue, Green, Red COLORREF
        sprintf(msg, "%d of %d in range", index+1, totalVesselsInRange);
    }

    TextOut(hDC, 52, 128, msg, static_cast<int>(strlen(msg)));

    // restore previous font and release device context
    SelectObject(hDC, hPrevObject);
    ReleaseDC(surf, hDC);

    return true;
}

// Handle mouse events for this area
// event = Orbiter event flags
// mx, my = mouse coordinates relative to the area
// returns: true if event processed, false if not
bool GrapplePayloadArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated or systems failure, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard() || GetXR1().m_internalSystemsFailure)
        return false;

    const COORD2 c = { mx, my };

    // check buttons
    if (c.InBounds(m_grappleButton, 9, 9))
    {
        GetXR1().GrapplePayload(GetXR1().m_selectedSlot, true);   // ignore return code

        return true;
    }
    else if (c.InBounds(m_grappleAllButton, 9, 9))
    {
        GetXR1().GrappleAllPayload();    // selected slot is unchanged; ignore return code here
        return true;
    }
    else if (c.InBounds(m_rangeButton, 9, 9))
    {
        GetXR1().IncGrappleRange(true);
        return true;
    }
    else if (c.InBounds(m_targetButtonUp, 7, 6))
    {
        GetXR1().AdjustGrappleTarget(+1, true);
        return true;
    }
    else if (c.InBounds(m_targetButtonDown, 7, 6))
    {
        GetXR1().AdjustGrappleTarget(-1, true);
        return true;
    }
    else if (c.InBounds(m_clearButton, 9, 9))
    {
        GetXR1().ClearGrappleTarget(true);
        return true;
    }
#if 0 // no way to do this
    else if (c.InBounds(m_trackButton, 9, 9))
    {
        GetXR1().TrackGrappleTarget(true);
        return true;
    }
#endif

    return false;
}

//-------------------------------------------------------------------------

PayloadMassNumberArea::PayloadMassNumberArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID, const bool isMetric) :
        MassNumberArea(parentPanel, panelCoordinates, areaID, isMetric)
{
}

double PayloadMassNumberArea::GetMassInKG() 
{ 
    return GetXR1().GetPayloadMass();
}

//----------------------------------------------------------------------------------

// topLeft = top inside edge of frame, just on black screen
PayloadMassDisplayComponent::PayloadMassDisplayComponent(InstrumentPanel &parentPanel, COORD2 topLeft, const int lbAreaID, const int kgAreaID) :
    XR1Component(parentPanel, topLeft)
{
    AddArea(new PayloadMassNumberArea(parentPanel, GetAbsCoords(_COORD2(18,  2)), lbAreaID, false));  // pounds
    AddArea(new PayloadMassNumberArea(parentPanel, GetAbsCoords(_COORD2(18, 15)), kgAreaID, true));   // kg
}

//----------------------------------------------------------------------------------

// Launch the payload editor window
PayloadEditorButtonArea::PayloadEditorButtonArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    MomentaryButtonArea(parentPanel, panelCoordinates, areaID)
{
}

void PayloadEditorButtonArea::ProcessButtonAction(int event, double buttonDownSimt)
{
    // process PRESSED and UNPRESSED events
    if (event & (PANEL_MOUSE_LBDOWN))
    {
        GetXR1().TogglePayloadEditor();  // plays a beep as well
    }
}
