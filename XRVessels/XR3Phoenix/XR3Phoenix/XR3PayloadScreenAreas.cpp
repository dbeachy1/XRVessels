// ==============================================================
// XR3Phoenix implementation class
//
// Copyright 2007-2016 Douglas E. Beachy
// All rights reserved.
//
// This software is FREEWARE and may not be sold!
//
// XR3PayloadScreenAreas.h
// Header for payload screen areas for the XR3.
// ==============================================================

#include "XR3PayloadBay.h"   // must be FIRST because of forward reference to XR3PayloadBay in XR3Phoenix.h
#include "XR3Phoenix.h"
#include "XR3PayloadScreenAreas.h"
#include "XRPayloadBaySlot.h"

//----------------------------------------------------------------------------------

// STATIC DATA
const COORD2 &SelectPayloadSlotArea::s_blockSize  = _COORD2(25, 30);    // size of each block in pixels
const COORD2 &SelectPayloadSlotArea::s_screenSize = _COORD2(149, 144);  // screen size in pixels

// isOn = reference to status variable: true = light on, false = light off
SelectPayloadSlotArea::SelectPayloadSlotArea(InstrumentPanel &parentPanel, const COORD2 panelCoordinates, const int areaID) :
    XR1Area(parentPanel, panelCoordinates, areaID)
{
    // reset all three level handles
    for (int i=0; i < 3; i++)
        m_hSurfaceForLevel[i] = nullptr;

    // define LEVEL button coordinates
    m_levelButton = _COORD2(12, 133);
}

void SelectPayloadSlotArea::Activate()
{
    Area::Activate();  // invoke superclass method
    // register area
    // specify both PANEL_REDRAW_ALWAYS and PANEL_REDRAW_MOUSE because we need explicit mouse events
    // Note that refresh rates are managed above us by clbkPanelRedrawEvent.
    oapiRegisterPanelArea(GetAreaID(), GetRectForSize(s_screenSize.x, s_screenSize.y), PANEL_REDRAW_ALWAYS | PANEL_REDRAW_MOUSE, PANEL_MOUSE_LBDOWN, PANEL_MAP_BGONREQUEST);

    m_hSurfaceForLevel[0] = CreateSurface(IDB_SELECT_BAY_SLOT_1);
    m_hSurfaceForLevel[1] = CreateSurface(IDB_SELECT_BAY_SLOT_2);
    m_hSurfaceForLevel[2] = CreateSurface(IDB_SELECT_BAY_SLOT_3);
}
 
void SelectPayloadSlotArea::Deactivate()
{
    Area::Deactivate();  // invoke superclass method
    for (int i=0; i < 3; i++)
        DestroySurface(m_hSurfaceForLevel + i);  // must pass pointer here because DestroySurface sets it to NULL
}

bool SelectPayloadSlotArea::Redraw2D(const int event, const SURFHANDLE surf)
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
        
    // render the background based on the currently active level
    const int selectedSlot = GetXR3().m_selectedSlot;  
    const int activeLevel = GetXR3().m_selectedSlotLevel;  
    oapiBlt(surf, m_hSurfaceForLevel[activeLevel-1], 0, 0, 0, 0, s_screenSize.x, s_screenSize.y);

    // check whether any vessel is targeted for grappling
    const XRPayloadClassData *pGrappleTargetVesselPCD = nullptr;

    if (!GetXR3().m_grappleTargetVesselName != 0)  // anything there?
    {
        // TODO: remove cast when Martin fixes the API
        OBJHANDLE hTarget = oapiGetVesselByName(const_cast<char *>(GetXR3().m_grappleTargetVesselName));   // will be NULL if vessel no longer in range
        if (hTarget != nullptr)
        {
            VESSEL *pGrappleTargetVessel = oapiGetVesselInterface(hTarget);  // will never be null
            pGrappleTargetVesselPCD = &XRPayloadClassData::GetXRPayloadClassDataForClassname(pGrappleTargetVessel->GetClassName());
        }
    }

    // Render the border around each square based on its status.  In order of precedence:
    //   Cyan = SELECTED EMPTY (clicking will toggle it)
    //   Orange = SELECTED OCCUPIED (clicking will toggle it)
    //   Light Green = Cargo attached, type matches cargo selected in grapple screen
    //   Medium Green = Cargo attached (centerpoint)
    //   Gray = Occupied by cargo (slot disabled in dialog)
    //   None (black) = empty

    for (int slotNumber = 1; slotNumber <= PAYLOAD_BAY_SLOT_COUNT; slotNumber++)
    {
        XRPayloadBaySlot *pSlot = GetXR3().m_pPayloadBay->GetSlot(slotNumber);   // will never be null

        // only process the active level's slots
        if (pSlot->GetLevel() != activeLevel)
            continue;
        
        DWORD dwBorderColor = 0;    // NONE
        const VESSEL *pChild = pSlot->GetChild();     // may be null

        // NOTE: we apply these tests in order of precedence

        if (slotNumber == GetXR3().m_selectedSlot)
        {
            // Note: this slot is never disabled because we prevent selecting a disabled slot.
            // slot is selected; check whether it is occupied
            dwBorderColor = ((pChild != nullptr) ? ORANGE : CYAN);
        }
        else if (pChild != nullptr)
        {
            // does cargo match the type of the grapple target?
            if (pGrappleTargetVesselPCD != nullptr)    // anything targeted for grappling?
            {
                const XRPayloadClassData &childPCD = XRPayloadClassData::GetXRPayloadClassDataForClassname(pChild->GetClassName());
                if (childPCD == *pGrappleTargetVesselPCD)
                {
                    // slot contains a child of the same type as the cargo targeted for grappling
                    dwBorderColor = LIGHT_GREEN;
                }
            }

            if (dwBorderColor == 0)   // not already set?
            {
                // slot contains a child that is a different type from the cargo targeted for grappling
                dwBorderColor = MEDIUM_GREEN;
            }
        }
        else if (pSlot->IsEnabled() == false)
        {
            // slot is DISABLED due to adjacent payload
            dwBorderColor = OFF_WHITE192; 
        }

        // only render this slot's border if one is set
        if (dwBorderColor != 0)
        {
            // compute the coordinates of this slot
            const COORD2 &slotGridCoordinates = pSlot->GetLevelGridCoordinates();   // 0,0 to n,n
            COORD2 &slot = _COORD2(14, 98);  // upper-left corder of slot where line border starts for SLOT 1
            slot += _COORD2((slotGridCoordinates.x * s_blockSize.x), -(slotGridCoordinates.y * s_blockSize.y));  // Y is negative

            // draw the line around the inside of the border
            // Note: the 3 and 4 in the following lines is necessary for the 3 and 4 grid pixel lines that separate columns and rows, respectively.
            // these are static for efficiency
            static const int borderBarWidth = 3;   // in pixels
            static const int blockInsideWidth = s_blockSize.x - 3;   // full-width
            static const int blockInsideHeight = s_blockSize.y - 4 - (borderBarWidth * 2);  // draw inside the full-width top and bottom lines
            //                                  X,                                           Y,                                           width,            height
            oapiColourFill(surf, dwBorderColor, slot.x,                                      slot.y,                                      blockInsideWidth, borderBarWidth);    // top horizontal line
            oapiColourFill(surf, dwBorderColor, slot.x,                                      slot.y + s_blockSize.y - 4 - borderBarWidth, blockInsideWidth, borderBarWidth);    // bottom horizontal line
            oapiColourFill(surf, dwBorderColor, slot.x,                                      slot.y + borderBarWidth,                     borderBarWidth,   blockInsideHeight); // left vertical line
            oapiColourFill(surf, dwBorderColor, slot.x + s_blockSize.x - 3 - borderBarWidth, slot.y + borderBarWidth,                     borderBarWidth,   blockInsideHeight); // right vertical line
        }
    }

    return true;
}

// Handle mouse events for this area
// event = Orbiter event flags
// mx, my = mouse coordinates relative to the area
// returns: true if event processed, false if not
bool SelectPayloadSlotArea::ProcessMouseEvent(const int event, const int mx, const int my)
{
    // if crew is incapacitated or systems failure, nothing to do here
    if (GetXR1().IsCrewIncapacitatedOrNoPilotOnBoard() || GetXR1().m_internalSystemsFailure)
        return false;

    const COORD2 c = { mx, my };

    // check LEVEL button
    if (c.InBounds(m_levelButton, 9, 9))
    {
        // bump to next level; do not reset the selected slot
        GetXR3().m_selectedSlotLevel++;
        if (GetXR3().m_selectedSlotLevel > 3)
            GetXR3().m_selectedSlotLevel = 1;

        GetXR3().PlaySound(DeltaGliderXR1::BeepHigh, DeltaGliderXR1::ST_Other);
        return true;
    }

    // check whether mouse is in range of our grid
    if ((mx >= 11) && (mx <= 138) &&
        (my >= 5)  && (my <= 126))
    {
        // compute the grid slot in which the mouse was clicked: 0,0 = lower-left corner (slot 1)
        int gridX = (mx - 11) / s_blockSize.x;    
        int gridY = (s_blockSize.y + 96 - my) / s_blockSize.y;   // must go from BOTTOM edge: y is negative as slot number grows

        // determine whether the slot clicked is valid for the active level
        XRPayloadBaySlot *pSlot = GetXR3().m_pPayloadBay->GetSlotForGrid(GetXR3().m_selectedSlotLevel, gridX, gridY);  // will be NULL if no slot at requested coordinates
        if (pSlot != nullptr)
        {
            // slot cannot be selected if it is DISABLED
            if (pSlot->IsEnabled() == false)
            {
                GetXR3().PlaySound(DeltaGliderXR1::Error1, DeltaGliderXR1::ST_Other, ERROR1_VOL);
                return false;   // no joy
            }
            else    // slot OK
            {
                // if the slot is already selected, de-select it; otherwise, select it
                const int slotNumber = pSlot->GetSlotNumber();
                if (GetXR3().m_selectedSlot == slotNumber)
                {
                    // deselect
                    GetXR3().PlaySound(DeltaGliderXR1::BeepLow, DeltaGliderXR1::ST_Other);
                    GetXR3().m_selectedSlot = 0;
                }
                else
                {
                    // select
                    GetXR3().PlaySound(DeltaGliderXR1::BeepHigh, DeltaGliderXR1::ST_Other);
                    GetXR3().m_selectedSlot = pSlot->GetSlotNumber();
                }

                // DEBUG: sprintf(oapiDebugString(), "mx=%d, my=%d, gridX=%d, gridY=%d, selectedSlot=%d", mx, my, gridX, gridY, GetXR3().m_selectedSlot);
                return true;
            }
        }
    }
    // DEBUG: else sprintf(oapiDebugString(), "NO CLICK");

    return false;
}
