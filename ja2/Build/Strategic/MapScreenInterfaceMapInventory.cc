// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Strategic/MapScreenInterfaceMapInventory.h"

#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "Directories.h"
#include "Macro.h"
#include "MessageBoxScreen.h"
#include "SGP/Buffer.h"
#include "SGP/ButtonSystem.h"
#include "SGP/CursorControl.h"
#include "SGP/Debug.h"
#include "SGP/English.h"
#include "SGP/Font.h"
#include "SGP/HImage.h"
#include "SGP/MemMan.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "ScreenIDs.h"
#include "Strategic/AutoResolve.h"
#include "Strategic/MapScreen.h"
#include "Strategic/MapScreenInterface.h"
#include "Strategic/MapScreenInterfaceBorder.h"
#include "Strategic/MapScreenInterfaceBottom.h"
#include "Strategic/MapScreenInterfaceMap.h"
#include "Strategic/StrategicMap.h"
#include "Tactical/ArmsDealerInvInit.h"
#include "Tactical/HandleItems.h"
#include "Tactical/Interface.h"
#include "Tactical/InterfaceItems.h"
#include "Tactical/InterfacePanels.h"
#include "Tactical/InterfaceUtils.h"
#include "Tactical/Items.h"
#include "Tactical/Overhead.h"
#include "Tactical/ShopKeeperInterface.h"
#include "Tactical/SoldierControl.h"
#include "Tactical/TacticalSave.h"
#include "Tactical/WorldItems.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/RadarScreen.h"
#include "TileEngine/RenderDirty.h"
#include "TileEngine/SysUtil.h"
#include "Utils/FontControl.h"
#include "Utils/Message.h"
#include "Utils/MultiLanguageGraphicUtils.h"
#include "Utils/Text.h"
#include "Utils/TimerControl.h"
#include "Utils/WordWrap.h"

// status bar colors
#define DESC_STATUS_BAR FROMRGB(201, 172, 133)
#define DESC_STATUS_BAR_SHADOW FROMRGB(140, 136, 119)

// delay for flash of item
#define DELAY_FOR_HIGHLIGHT_ITEM_FLASH 200

// inventory slot font
#define MAP_IVEN_FONT SMALLCOMPFONT

// inventory pool slot positions and sizes
#define MAP_INV_SLOT_ROWS 9

static const SGPBox g_sector_inv_box = {261, 0, 379, 360};
static const SGPBox g_sector_inv_title_box = {266, 5, 370, 29};
static const SGPBox g_sector_inv_slot_box = {274, 37, 72, 32};
static const SGPBox g_sector_inv_region_box = {0, 0, 67, 31};  // relative to g_sector_inv_slot_box
static const SGPBox g_sector_inv_item_box = {6, 0, 61, 24};    // relative to g_sector_inv_slot_box
static const SGPBox g_sector_inv_bar_box = {2, 2, 2, 20};      // relative to g_sector_inv_slot_box
static const SGPBox g_sector_inv_name_box = {0, 24, 67, 7};    // relative to g_sector_inv_slot_box
static const SGPBox g_sector_inv_loc_box = {326, 337, 39, 10};
static const SGPBox g_sector_inv_count_box = {437, 337, 39, 10};
static const SGPBox g_sector_inv_page_box = {505, 337, 50, 10};

// the current highlighted item
int32_t iCurrentlyHighLightedItem = -1;
BOOLEAN fFlashHighLightInventoryItemOnradarMap = FALSE;

// whether we are showing the inventory pool graphic
BOOLEAN fShowMapInventoryPool = FALSE;

// the v-object index value for the background
static SGPVObject *guiMapInventoryPoolBackground;

// inventory pool list
WORLDITEM *pInventoryPoolList = NULL;

// current page of inventory
int32_t iCurrentInventoryPoolPage = 0;
static int32_t iLastInventoryPoolPage = 0;

// total number of slots allocated
static int32_t iTotalNumberOfSlots = 0;

int16_t sObjectSourceGridNo = 0;

// number of unseen items in sector
static uint32_t uiNumberOfUnSeenItems = 0;

// the inventory slots
static MOUSE_REGION MapInventoryPoolSlots[MAP_INVENTORY_POOL_SLOT_COUNT];
static MOUSE_REGION MapInventoryPoolMask;
BOOLEAN fMapInventoryItemCompatable[MAP_INVENTORY_POOL_SLOT_COUNT];
static BOOLEAN fChangedInventorySlots = FALSE;

// the unseen items list...have to save this
static WORLDITEM *pUnSeenItems = NULL;

int32_t giFlashHighlightedItemBaseTime = 0;
int32_t giCompatibleItemBaseTime = 0;

static GUIButtonRef guiMapInvenButton[3];

static BOOLEAN gfCheckForCursorOverMapSectorInventoryItem = FALSE;

// load the background panel graphics for inventory
void LoadInventoryPoolGraphic() {
  // add to V-object index
  guiMapInventoryPoolBackground = AddVideoObjectFromFile(INTERFACEDIR "/sector_inventory.sti");
}

// remove background panel graphics for inventory
void RemoveInventoryPoolGraphic() {
  // remove from v-object index
  if (guiMapInventoryPoolBackground) {
    DeleteVideoObject(guiMapInventoryPoolBackground);
    guiMapInventoryPoolBackground = 0;
  }
}

static void CheckAndUnDateSlotAllocation();
static void DisplayCurrentSector();
static void DisplayPagesForMapInventoryPool();
static void DrawNumberOfIventoryPoolItems();
static void DrawTextOnMapInventoryBackground();
static void RenderItemsForCurrentPageOfInventoryPool();
static void UpdateHelpTextForInvnentoryStashSlots();

// blit the background panel for the inventory
void BlitInventoryPoolGraphic() {
  const SGPBox *const box = &g_sector_inv_box;
  BltVideoObject(guiSAVEBUFFER, guiMapInventoryPoolBackground, 0, box->x, box->y);

  // resize list
  CheckAndUnDateSlotAllocation();

  // now the items
  RenderItemsForCurrentPageOfInventoryPool();

  // now update help text
  UpdateHelpTextForInvnentoryStashSlots();

  // show which page and last page
  DisplayPagesForMapInventoryPool();

  // draw number of items in current inventory
  DrawNumberOfIventoryPoolItems();

  // display current sector inventory pool is at
  DisplayCurrentSector();

  DrawTextOnMapInventoryBackground();

  // re render buttons
  MarkButtonsDirty();

  // which buttons will be active and which ones not
  HandleButtonStatesWhileMapInventoryActive();
}

static BOOLEAN RenderItemInPoolSlot(int32_t iCurrentSlot, int32_t iFirstSlotOnPage);

static void RenderItemsForCurrentPageOfInventoryPool() {
  int32_t iCounter = 0;

  // go through list of items on this page and place graphics to screen
  for (iCounter = 0; iCounter < MAP_INVENTORY_POOL_SLOT_COUNT; iCounter++) {
    RenderItemInPoolSlot(iCounter, (iCurrentInventoryPoolPage * MAP_INVENTORY_POOL_SLOT_COUNT));
  }
}

static BOOLEAN RenderItemInPoolSlot(int32_t iCurrentSlot, int32_t iFirstSlotOnPage) {
  // render item in this slot of the list
  const WORLDITEM *const item = &pInventoryPoolList[iCurrentSlot + iFirstSlotOnPage];

  // check if anything there
  if (item->o.ubNumberOfObjects == 0) return FALSE;

  const SGPBox *const slot_box = &g_sector_inv_slot_box;
  const int32_t dx = slot_box->x + slot_box->w * (iCurrentSlot / MAP_INV_SLOT_ROWS);
  const int32_t dy = slot_box->y + slot_box->h * (iCurrentSlot % MAP_INV_SLOT_ROWS);

  SetFontDestBuffer(guiSAVEBUFFER);
  const SGPBox *const item_box = &g_sector_inv_item_box;
  const uint16_t outline = fMapInventoryItemCompatable[iCurrentSlot]
                               ? Get16BPPColor(FROMRGB(255, 255, 255))
                               : SGP_TRANSPARENT;
  INVRenderItem(guiSAVEBUFFER, NULL, item->o, dx + item_box->x, dy + item_box->y, item_box->w,
                item_box->h, DIRTYLEVEL2, 0, outline);

  // draw bar for condition
  const uint16_t col0 = Get16BPPColor(DESC_STATUS_BAR);
  const uint16_t col1 = Get16BPPColor(DESC_STATUS_BAR_SHADOW);
  const SGPBox *const bar_box = &g_sector_inv_bar_box;
  DrawItemUIBarEx(item->o, 0, dx + bar_box->x, dy + bar_box->y + bar_box->h - 1, bar_box->h, col0,
                  col1, guiSAVEBUFFER);

  // if the item is not reachable, or if the selected merc is not in the current
  // sector
  const SOLDIERTYPE *const s = GetSelectedInfoChar();
  if (!(item->usFlags & WORLD_ITEM_REACHABLE) || s == NULL || s->sSectorX != sSelMapX ||
      s->sSectorY != sSelMapY || s->bSectorZ != iCurrentMapSectorZ) {
    // Shade the item
    DrawHatchOnInventory(guiSAVEBUFFER, dx + item_box->x, dy + item_box->y, item_box->w,
                         item_box->h);
  }

  // the name
  const SGPBox *const name_box = &g_sector_inv_name_box;
  wchar_t sString[SIZE_SHORT_ITEM_NAME];
  wcscpy(sString, ShortItemNames[item->o.usItem]);
  ReduceStringLength(sString, lengthof(sString), name_box->w, MAP_IVEN_FONT);

  SetFontAttributes(MAP_IVEN_FONT, FONT_WHITE);

  int16_t x;
  int16_t y;
  FindFontCenterCoordinates(dx + name_box->x, dy + name_box->y, name_box->w, name_box->h, sString,
                            MAP_IVEN_FONT, &x, &y);
  MPrint(x, y, sString);

  SetFontDestBuffer(FRAME_BUFFER);

  return TRUE;
}

static void UpdateHelpTextForInvnentoryStashSlots() {
  wchar_t pStr[512];
  int32_t iCounter = 0;
  int32_t iFirstSlotOnPage = (iCurrentInventoryPoolPage * MAP_INVENTORY_POOL_SLOT_COUNT);

  // run through list of items in slots and update help text for mouse regions
  for (iCounter = 0; iCounter < MAP_INVENTORY_POOL_SLOT_COUNT; iCounter++) {
    wchar_t const *help = L"";
    OBJECTTYPE const &o = pInventoryPoolList[iCounter + iFirstSlotOnPage].o;
    if (o.ubNumberOfObjects > 0) {
      GetHelpTextForItem(pStr, lengthof(pStr), o);
      help = pStr;
    }
    MapInventoryPoolSlots[iCounter].SetFastHelpText(help);
  }
}

static void BuildStashForSelectedSector(int16_t sMapX, int16_t sMapY, int16_t sMapZ);
static void CreateMapInventoryButtons();
static void CreateMapInventoryPoolDoneButton();
static void CreateMapInventoryPoolSlots();
static void DestroyInventoryPoolDoneButton();
static void DestroyMapInventoryButtons();
static void DestroyMapInventoryPoolSlots();
static void DestroyStash();
static void HandleMapSectorInventory();
static void SaveSeenAndUnseenItems();

// create and remove buttons for inventory
void CreateDestroyMapInventoryPoolButtons(BOOLEAN fExitFromMapScreen) {
  static BOOLEAN fCreated = FALSE;

  /* player can leave items underground, no?
          if( iCurrentMapSectorZ )
          {
                  fShowMapInventoryPool = FALSE;
          }
  */

  if (fShowMapInventoryPool && !fCreated) {
    if ((gWorldSectorX == sSelMapX) && (gWorldSectorY == sSelMapY) &&
        (gbWorldSectorZ == iCurrentMapSectorZ)) {
      // handle all reachable before save
      HandleAllReachAbleItemsInTheSector(gWorldSectorX, gWorldSectorY, gbWorldSectorZ);
    }

    // destroy buttons for map border
    DeleteMapBorderButtons();

    fCreated = TRUE;

    // also create the inventory slot
    CreateMapInventoryPoolSlots();

    // create buttons
    CreateMapInventoryButtons();

    // build stash
    BuildStashForSelectedSector(sSelMapX, sSelMapY, (int16_t)(iCurrentMapSectorZ));

    CreateMapInventoryPoolDoneButton();

    fMapPanelDirty = TRUE;
    fMapScreenBottomDirty = TRUE;
  } else if (!fShowMapInventoryPool && fCreated) {
    // check fi we are in fact leaving mapscreen
    if (!fExitFromMapScreen) {
      // recreate mapborder buttons
      CreateButtonsForMapBorder();
    }
    fCreated = FALSE;

    // destroy the map inventory slots
    DestroyMapInventoryPoolSlots();

    // destroy map inventory buttons
    DestroyMapInventoryButtons();

    DestroyInventoryPoolDoneButton();

    // now save results
    SaveSeenAndUnseenItems();

    DestroyStash();

    fMapPanelDirty = TRUE;
    fTeamPanelDirty = TRUE;
    fCharacterInfoPanelDirty = TRUE;

    // DEF: added to remove the 'item blip' from staying on the radar map
    iCurrentlyHighLightedItem = -1;

    // re render radar map
    RenderRadarScreen();
  }

  // do our handling here
  HandleMapSectorInventory();
}

void CancelSectorInventoryDisplayIfOn(BOOLEAN fExitFromMapScreen) {
  if (fShowMapInventoryPool) {
    // get rid of sector inventory mode & buttons
    fShowMapInventoryPool = FALSE;
    CreateDestroyMapInventoryPoolButtons(fExitFromMapScreen);
  }
}

static int32_t GetTotalNumberOfItems();
static void ReBuildWorldItemStashForLoadedSector(int32_t iNumberSeenItems,
                                                 int32_t iNumberUnSeenItems,
                                                 const WORLDITEM *pSeenItemsList,
                                                 const WORLDITEM *pUnSeenItemsList);

static void SaveSeenAndUnseenItems() {
  const int32_t iTotalNumberItems = GetTotalNumberOfItems();

  // if there are seen items, build a temp world items list of them and save
  // them
  int32_t iItemCount = 0;
  SGP::Buffer<WORLDITEM> pSeenItemsList;
  if (iTotalNumberItems > 0) {
    pSeenItemsList.Allocate(iTotalNumberItems);

    // copy
    for (int32_t iCounter = 0; iCounter < iTotalNumberOfSlots; ++iCounter) {
      const WORLDITEM *const pi = &pInventoryPoolList[iCounter];
      if (pi->o.ubNumberOfObjects == 0) continue;

      WORLDITEM *const si = &pSeenItemsList[iItemCount++];
      *si = *pi;

      // Check if item actually lives at a gridno
      if (si->sGridNo == 0) {
        // Use gridno of predecessor, if there is one
        if (si != pSeenItemsList) {
          // borrow from predecessor
          si->sGridNo = si[-1].sGridNo;
        } else {
          // get entry grid location
        }
      }
      si->fExists = TRUE;
      si->bVisible = TRUE;
    }
  }

  // if this is the loaded sector handle here
  if (gWorldSectorX == sSelMapX && gWorldSectorY == sSelMapY &&
      gbWorldSectorZ == (int8_t)iCurrentMapSectorZ) {
    ReBuildWorldItemStashForLoadedSector(iItemCount, uiNumberOfUnSeenItems, pSeenItemsList,
                                         pUnSeenItems);
  } else {
    // now copy over unseen and seen
    SaveWorldItemsToTempItemFile(sSelMapX, sSelMapY, iCurrentMapSectorZ, uiNumberOfUnSeenItems,
                                 pUnSeenItems);
    AddWorldItemsToUnLoadedSector(sSelMapX, sSelMapY, iCurrentMapSectorZ, iItemCount,
                                  pSeenItemsList);
  }
}

static void InventoryNextPage() {
  if (iCurrentInventoryPoolPage < iLastInventoryPoolPage) {
    ++iCurrentInventoryPoolPage;
    fMapPanelDirty = TRUE;
  }
}

static void InventoryPrevPage() {
  if (iCurrentInventoryPoolPage > 0) {
    --iCurrentInventoryPoolPage;
    fMapPanelDirty = TRUE;
  }
}

// the screen mask bttn callaback...to disable the inventory and lock out the
// map itself
static void MapInvenPoolScreenMaskCallback(MOUSE_REGION *pRegion, int32_t iReason) {
  if ((iReason & MSYS_CALLBACK_REASON_RBUTTON_UP)) {
    fShowMapInventoryPool = FALSE;
  } else if (iReason & MSYS_CALLBACK_REASON_WHEEL_UP) {
    InventoryPrevPage();
  } else if (iReason & MSYS_CALLBACK_REASON_WHEEL_DOWN) {
    InventoryNextPage();
  }
}

static void MapInvenPoolSlots(MOUSE_REGION *pRegion, int32_t iReason);
static void MapInvenPoolSlotsMove(MOUSE_REGION *pRegion, int32_t iReason);

static void CreateMapInventoryPoolSlots() {
  {
    const SGPBox *const inv_box = &g_sector_inv_box;
    uint16_t const x = inv_box->x;
    uint16_t const y = inv_box->y;
    uint16_t const w = inv_box->w;
    uint16_t const h = inv_box->h;
    MSYS_DefineRegion(&MapInventoryPoolMask, x, y, x + w - 1, y + h - 1, MSYS_PRIORITY_HIGH,
                      MSYS_NO_CURSOR, MSYS_NO_CALLBACK, MapInvenPoolScreenMaskCallback);
  }

  const SGPBox *const slot_box = &g_sector_inv_slot_box;
  const SGPBox *const reg_box = &g_sector_inv_region_box;
  for (uint32_t i = 0; i < MAP_INVENTORY_POOL_SLOT_COUNT; ++i) {
    uint16_t const sx = i / MAP_INV_SLOT_ROWS;
    uint16_t const sy = i % MAP_INV_SLOT_ROWS;
    uint16_t const x = reg_box->x + slot_box->x + sx * slot_box->w;
    uint16_t const y = reg_box->y + slot_box->y + sy * slot_box->h;
    uint16_t const w = reg_box->w;
    uint16_t const h = reg_box->h;
    MOUSE_REGION *const r = &MapInventoryPoolSlots[i];
    MSYS_DefineRegion(r, x, y, x + w - 1, y + h - 1, MSYS_PRIORITY_HIGH, MSYS_NO_CURSOR,
                      MapInvenPoolSlotsMove, MapInvenPoolSlots);
    MSYS_SetRegionUserData(r, 0, i);
  }
}

static void DestroyMapInventoryPoolSlots() {
  FOR_EACH(MOUSE_REGION, i, MapInventoryPoolSlots) MSYS_RemoveRegion(&*i);
  MSYS_RemoveRegion(&MapInventoryPoolMask);
}

static void MapInvenPoolSlotsMove(MOUSE_REGION *pRegion, int32_t iReason) {
  int32_t iCounter = 0;

  iCounter = MSYS_GetRegionUserData(pRegion, 0);

  if (iReason & MSYS_CALLBACK_REASON_GAIN_MOUSE) {
    iCurrentlyHighLightedItem = iCounter;
    fChangedInventorySlots = TRUE;
    gfCheckForCursorOverMapSectorInventoryItem = TRUE;
  } else if (iReason & MSYS_CALLBACK_REASON_LOST_MOUSE) {
    iCurrentlyHighLightedItem = -1;
    fChangedInventorySlots = TRUE;
    gfCheckForCursorOverMapSectorInventoryItem = FALSE;

    // re render radar map
    RenderRadarScreen();
  }
}

static void BeginInventoryPoolPtr(OBJECTTYPE *pInventorySlot);
static BOOLEAN CanPlayerUseSectorInventory();
static BOOLEAN PlaceObjectInInventoryStash(OBJECTTYPE *pInventorySlot, OBJECTTYPE *pItemPtr);

static void MapInvenPoolSlots(MOUSE_REGION *const pRegion, const int32_t iReason) {
  // btn callback handler for assignment screen mask region
  if (iReason & MSYS_CALLBACK_REASON_RBUTTON_UP) {
    if (gpItemPointer == NULL) fShowMapInventoryPool = FALSE;
  } else if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    // check if item in cursor, if so, then swap, and no item in curor, pick up,
    // if item in cursor but not box, put in box
    int32_t const slot_idx = MSYS_GetRegionUserData(pRegion, 0);
    WORLDITEM *const slot =
        &pInventoryPoolList[iCurrentInventoryPoolPage * MAP_INVENTORY_POOL_SLOT_COUNT + slot_idx];

    // Return if empty
    if (gpItemPointer == NULL && slot->o.usItem == NOTHING) return;

    // is this item reachable
    if (slot->o.usItem != NOTHING && !(slot->usFlags & WORLD_ITEM_REACHABLE)) {
      // not reachable
      DoMapMessageBox(MSG_BOX_BASIC_STYLE, gzLateLocalizedString[STR_LATE_38], MAP_SCREEN,
                      MSG_BOX_FLAG_OK, NULL);
      return;
    }

    // Valid character?
    const SOLDIERTYPE *const s = GetSelectedInfoChar();
    if (s == NULL) {
      DoMapMessageBox(MSG_BOX_BASIC_STYLE, pMapInventoryErrorString[0], MAP_SCREEN, MSG_BOX_FLAG_OK,
                      NULL);
      return;
    }

    // Check if selected merc is in this sector, if not, warn them and leave
    if (s->sSectorX != sSelMapX || s->sSectorY != sSelMapY || s->bSectorZ != iCurrentMapSectorZ ||
        s->fBetweenSectors) {
      const wchar_t *const msg =
          (gpItemPointer == NULL ? pMapInventoryErrorString[1] : pMapInventoryErrorString[4]);
      wchar_t buf[128];
      swprintf(buf, lengthof(buf), msg, s->name);
      DoMapMessageBox(MSG_BOX_BASIC_STYLE, buf, MAP_SCREEN, MSG_BOX_FLAG_OK, NULL);
      return;
    }

    // If in battle inform player they will have to do this in tactical
    if (!CanPlayerUseSectorInventory()) {
      const wchar_t *const msg =
          (gpItemPointer == NULL ? pMapInventoryErrorString[2] : pMapInventoryErrorString[3]);
      DoMapMessageBox(MSG_BOX_BASIC_STYLE, msg, MAP_SCREEN, MSG_BOX_FLAG_OK, NULL);
      return;
    }

    // If we do not have an item in hand, start moving it
    if (gpItemPointer == NULL) {
      sObjectSourceGridNo = slot->sGridNo;
      BeginInventoryPoolPtr(&slot->o);
    } else {
      const int32_t iOldNumberOfObjects = slot->o.ubNumberOfObjects;

      // Else, try to place here
      if (PlaceObjectInInventoryStash(&slot->o, gpItemPointer)) {
        // nothing here before, then place here
        if (iOldNumberOfObjects == 0) {
          slot->sGridNo = sObjectSourceGridNo;
          slot->ubLevel = s->bLevel;
          slot->usFlags = 0;
          slot->bRenderZHeightAboveLevel = 0;

          if (sObjectSourceGridNo == NOWHERE) {
            slot->usFlags |= WORLD_ITEM_GRIDNO_NOT_SET_USE_ENTRY_POINT;
          }
        }

        slot->usFlags |= WORLD_ITEM_REACHABLE;

        // Check if it's the same now!
        if (gpItemPointer->ubNumberOfObjects == 0) {
          MAPEndItemPointer();
        } else {
          SetMapCursorItem();
        }
      }
    }

    // dirty region, force update
    fMapPanelDirty = TRUE;
  } else if (iReason & MSYS_CALLBACK_REASON_WHEEL_UP) {
    InventoryPrevPage();
  } else if (iReason & MSYS_CALLBACK_REASON_WHEEL_DOWN) {
    InventoryNextPage();
  }
}

static void MapInventoryPoolPrevBtn(GUI_BUTTON *btn, int32_t reason);
static void MapInventoryPoolNextBtn(GUI_BUTTON *btn, int32_t reason);

static void CreateMapInventoryButtons() {
  guiMapInvenButton[0] =
      QuickCreateButtonImg(INTERFACEDIR "/map_screen_bottom_arrows.sti", 10, 1, -1, 3, -1, 559, 336,
                           MSYS_PRIORITY_HIGHEST, MapInventoryPoolNextBtn);
  guiMapInvenButton[1] =
      QuickCreateButtonImg(INTERFACEDIR "/map_screen_bottom_arrows.sti", 9, 0, -1, 2, -1, 487, 336,
                           MSYS_PRIORITY_HIGHEST, MapInventoryPoolPrevBtn);

  // reset the current inventory page to be the first page
  iCurrentInventoryPoolPage = 0;
}

static void DestroyMapInventoryButtons() {
  RemoveButton(guiMapInvenButton[0]);
  RemoveButton(guiMapInvenButton[1]);
}

static void CheckGridNoOfItemsInMapScreenMapInventory();
static void SortSectorInventory(WORLDITEM *pInventory, uint32_t uiSizeOfArray);

static void BuildStashForSelectedSector(const int16_t sMapX, const int16_t sMapY,
                                        const int16_t sMapZ) {
  WORLDITEM *items;
  const WORLDITEM *items_end;
  if (sMapX == gWorldSectorX && sMapY == gWorldSectorY && sMapZ == gbWorldSectorZ) {
    items = gWorldItems;
    items_end = gWorldItems + guiNumWorldItems;
  } else {
    uint32_t item_count;
    LoadWorldItemsFromTempItemFile(sMapX, sMapY, sMapZ, &item_count, &items);
    items_end = items + item_count;
  }

  uint32_t visible_count = 0;
  uint32_t unseen_count = 0;
  for (const WORLDITEM *wi = items; wi != items_end; ++wi) {
    if (!wi->fExists) continue;
    if (IsMapScreenWorldItemVisibleInMapInventory(wi)) {
      ++visible_count;
    } else {
      ++unseen_count;
    }
  }

  const uint32_t slot_count =
      visible_count - visible_count % MAP_INVENTORY_POOL_SLOT_COUNT + MAP_INVENTORY_POOL_SLOT_COUNT;
  iLastInventoryPoolPage = (slot_count - 1) / MAP_INVENTORY_POOL_SLOT_COUNT;

  WORLDITEM *visible_item = MALLOCNZ(WORLDITEM, slot_count);
  WORLDITEM *unseen_item = (unseen_count != 0 ? MALLOCN(WORLDITEM, unseen_count) : NULL);

  iTotalNumberOfSlots = slot_count;
  pInventoryPoolList = visible_item;
  uiNumberOfUnSeenItems = unseen_count;
  pUnSeenItems = unseen_item;

  for (const WORLDITEM *wi = items; wi != items_end; ++wi) {
    if (!wi->fExists) continue;
    if (IsMapScreenWorldItemVisibleInMapInventory(wi)) {
      *visible_item++ = *wi;
      Assert(visible_item <= pInventoryPoolList + visible_count);
    } else {
      *unseen_item++ = *wi;
      Assert(unseen_item <= pUnSeenItems + unseen_count);
    }
  }
  Assert(visible_item == pInventoryPoolList + visible_count);
  Assert(unseen_item == pUnSeenItems + unseen_count);

  if (items != gWorldItems && items != NULL) MemFree(items);

  CheckGridNoOfItemsInMapScreenMapInventory();
  SortSectorInventory(pInventoryPoolList, visible_count);
}

static void ReBuildWorldItemStashForLoadedSector(const int32_t iNumberSeenItems,
                                                 const int32_t iNumberUnSeenItems,
                                                 const WORLDITEM *const pSeenItemsList,
                                                 const WORLDITEM *const pUnSeenItemsList) {
  TrashWorldItems();

  // get total number of items
  int32_t iTotalNumberOfItems = iNumberUnSeenItems + iNumberSeenItems;

  const int32_t iRemainder = iTotalNumberOfItems % 10;

  // if there is a remainder, then add onto end of list
  if (iRemainder) iTotalNumberOfItems += 10 - iRemainder;

  // allocate space for items
  WORLDITEM *const pTotalList = MALLOCNZ(WORLDITEM, iTotalNumberOfItems);

  int32_t iCurrentItem = 0;
  // place seen items in the world
  for (int32_t i = 0; i < iNumberSeenItems; ++i) {
    pTotalList[iCurrentItem++] = pSeenItemsList[i];
  }

  // now store the unseen item list
  for (int32_t i = 0; i < iNumberUnSeenItems; ++i) {
    pTotalList[iCurrentItem++] = pUnSeenItemsList[i];
  }

  RefreshItemPools(pTotalList, iTotalNumberOfItems);

  // Count the total number of visible items
  uint32_t uiTotalNumberOfVisibleItems = 0;
  for (int32_t i = 0; i < iNumberSeenItems; ++i) {
    uiTotalNumberOfVisibleItems += pSeenItemsList[i].o.ubNumberOfObjects;
  }

  // reset the visible item count in the sector info struct
  SetNumberOfVisibleWorldItemsInSectorStructureForSector(
      gWorldSectorX, gWorldSectorY, gbWorldSectorZ, uiTotalNumberOfVisibleItems);

  // clear out allocated space for total list
  MemFree(pTotalList);
}

static void ReSizeStashListByThisAmount(int32_t iNumberOfItems) {
  const int32_t count = iTotalNumberOfSlots;
  iTotalNumberOfSlots += iNumberOfItems;
  pInventoryPoolList = REALLOC(pInventoryPoolList, WORLDITEM, iTotalNumberOfSlots);
  memset(pInventoryPoolList + count, 0, sizeof(*pInventoryPoolList) * iNumberOfItems);
}

static void DestroyStash() {
  // clear out stash
  MemFree(pInventoryPoolList);

  if (pUnSeenItems != NULL) {
    MemFree(pUnSeenItems);
    pUnSeenItems = NULL;
  }
  uiNumberOfUnSeenItems = 0;
}

static BOOLEAN GetObjFromInventoryStashSlot(OBJECTTYPE *pInventorySlot, OBJECTTYPE *pItemPtr);
static BOOLEAN RemoveObjectFromStashSlot(OBJECTTYPE *pInventorySlot, OBJECTTYPE *pItemPtr);

static void BeginInventoryPoolPtr(OBJECTTYPE *pInventorySlot) {
  BOOLEAN fOk = FALSE;

  // If not null return
  if (gpItemPointer != NULL) {
    return;
  }

  // if shift key get all

  if (IsKeyDown(SHIFT)) {
    // Remove all from soldier's slot
    fOk = RemoveObjectFromStashSlot(pInventorySlot, &gItemPointer);
  } else {
    GetObjFromInventoryStashSlot(pInventorySlot, &gItemPointer);
    fOk = (gItemPointer.ubNumberOfObjects == 1);
  }

  if (fOk) {
    // Dirty interface
    fMapPanelDirty = TRUE;
    SetItemPointer(&gItemPointer, 0);
    SetMapCursorItem();

    if (fShowInventoryFlag) {
      SOLDIERTYPE *const s = GetSelectedInfoChar();
      if (s != NULL) {
        ReevaluateItemHatches(s, FALSE);
        fTeamPanelDirty = TRUE;
      }
    }
  }
}

// get this item out of the stash slot
static BOOLEAN GetObjFromInventoryStashSlot(OBJECTTYPE *pInventorySlot, OBJECTTYPE *pItemPtr) {
  // item ptr
  if (!pItemPtr) {
    return (FALSE);
  }

  // if there are only one item in slot, just copy
  if (pInventorySlot->ubNumberOfObjects == 1) {
    *pItemPtr = *pInventorySlot;
    DeleteObj(pInventorySlot);
  } else {
    // take one item
    pItemPtr->usItem = pInventorySlot->usItem;

    // find first unempty slot
    pItemPtr->bStatus[0] = pInventorySlot->bStatus[0];
    pItemPtr->ubNumberOfObjects = 1;
    pItemPtr->ubWeight = CalculateObjectWeight(pItemPtr);
    RemoveObjFrom(pInventorySlot, 0);
    pInventorySlot->ubWeight = CalculateObjectWeight(pInventorySlot);
  }

  return (TRUE);
}

static BOOLEAN RemoveObjectFromStashSlot(OBJECTTYPE *pInventorySlot, OBJECTTYPE *pItemPtr) {
  if (pInventorySlot->ubNumberOfObjects == 0) {
    return (FALSE);
  } else {
    *pItemPtr = *pInventorySlot;
    DeleteObj(pInventorySlot);
    return (TRUE);
  }
}

static BOOLEAN PlaceObjectInInventoryStash(OBJECTTYPE *pInventorySlot, OBJECTTYPE *pItemPtr) {
  uint8_t ubNumberToDrop, ubSlotLimit, ubLoop;

  // if there is something there, swap it, if they are of the same type and
  // stackable then add to the count

  ubSlotLimit = Item[pItemPtr->usItem].ubPerPocket;

  if (pInventorySlot->ubNumberOfObjects == 0) {
    // placement in an empty slot
    ubNumberToDrop = pItemPtr->ubNumberOfObjects;

    if (ubNumberToDrop > ubSlotLimit && ubSlotLimit != 0) {
      // drop as many as possible into pocket
      ubNumberToDrop = ubSlotLimit;
    }

    // could be wrong type of object for slot... need to check...
    // but assuming it isn't
    *pInventorySlot = *pItemPtr;

    if (ubNumberToDrop != pItemPtr->ubNumberOfObjects) {
      // in the InSlot copy, zero out all the objects we didn't drop
      for (ubLoop = ubNumberToDrop; ubLoop < pItemPtr->ubNumberOfObjects; ubLoop++) {
        pInventorySlot->bStatus[ubLoop] = 0;
      }
    }
    pInventorySlot->ubNumberOfObjects = ubNumberToDrop;

    // remove a like number of objects from pObj
    RemoveObjs(pItemPtr, ubNumberToDrop);
  } else {
    // replacement/reloading/merging/stacking

    // placement in an empty slot
    ubNumberToDrop = pItemPtr->ubNumberOfObjects;

    if (pItemPtr->usItem == pInventorySlot->usItem) {
      if (pItemPtr->usItem == MONEY) {
        // always allow money to be combined!
        // average out the status values using a weighted average...
        pInventorySlot->bStatus[0] =
            (int8_t)(((uint32_t)pInventorySlot->bMoneyStatus * pInventorySlot->uiMoneyAmount +
                      (uint32_t)pItemPtr->bMoneyStatus * pItemPtr->uiMoneyAmount) /
                     (pInventorySlot->uiMoneyAmount + pItemPtr->uiMoneyAmount));
        pInventorySlot->uiMoneyAmount += pItemPtr->uiMoneyAmount;

        DeleteObj(pItemPtr);
      } else if (ubSlotLimit < 2) {
        // swapping
        SwapObjs(pItemPtr, pInventorySlot);
      } else {
        // stacking
        if (ubNumberToDrop > ubSlotLimit - pInventorySlot->ubNumberOfObjects) {
          ubNumberToDrop = ubSlotLimit - pInventorySlot->ubNumberOfObjects;
        }

        StackObjs(pItemPtr, pInventorySlot, ubNumberToDrop);
      }
    } else {
      SwapObjs(pItemPtr, pInventorySlot);
    }
  }
  return (TRUE);
}

void AutoPlaceObjectInInventoryStash(OBJECTTYPE *pItemPtr) {
  uint8_t ubNumberToDrop, ubSlotLimit, ubLoop;
  OBJECTTYPE *pInventorySlot;

  // if there is something there, swap it, if they are of the same type and
  // stackable then add to the count
  pInventorySlot = &(pInventoryPoolList[iTotalNumberOfSlots].o);

  // placement in an empty slot
  ubNumberToDrop = pItemPtr->ubNumberOfObjects;

  ubSlotLimit = ItemSlotLimit(pItemPtr->usItem, BIGPOCK1POS);

  if (ubNumberToDrop > ubSlotLimit && ubSlotLimit != 0) {
    // drop as many as possible into pocket
    ubNumberToDrop = ubSlotLimit;
  }

  // could be wrong type of object for slot... need to check...
  // but assuming it isn't
  *pInventorySlot = *pItemPtr;

  if (ubNumberToDrop != pItemPtr->ubNumberOfObjects) {
    // in the InSlot copy, zero out all the objects we didn't drop
    for (ubLoop = ubNumberToDrop; ubLoop < pItemPtr->ubNumberOfObjects; ubLoop++) {
      pInventorySlot->bStatus[ubLoop] = 0;
    }
  }
  pInventorySlot->ubNumberOfObjects = ubNumberToDrop;

  // remove a like number of objects from pObj
  RemoveObjs(pItemPtr, ubNumberToDrop);
}

static void MapInventoryPoolNextBtn(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    InventoryNextPage();
  }
}

static void MapInventoryPoolPrevBtn(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    InventoryPrevPage();
  }
}

static void MapInventoryPoolDoneBtn(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    fShowMapInventoryPool = FALSE;
  }
}

static void DisplayPagesForMapInventoryPool() {
  // get the current and last pages and display them
  wchar_t sString[32];
  int16_t sX, sY;

  SetFontAttributes(COMPFONT, 183);
  SetFontDestBuffer(guiSAVEBUFFER);

  // grab current and last pages
  swprintf(sString, lengthof(sString), L"%d / %d", iCurrentInventoryPoolPage + 1,
           iLastInventoryPoolPage + 1);

  // grab centered coords
  const SGPBox *const box = &g_sector_inv_page_box;
  FindFontCenterCoordinates(box->x, box->y, box->w, box->h, sString, COMPFONT, &sX, &sY);
  MPrint(sX, sY, sString);

  SetFontDestBuffer(FRAME_BUFFER);
}

static int32_t GetTotalNumberOfItemsInSectorStash() {
  int32_t iCounter, iCount = 0;

  // run through list of items and find out how many are there
  for (iCounter = 0; iCounter < iTotalNumberOfSlots; iCounter++) {
    if (pInventoryPoolList[iCounter].o.ubNumberOfObjects > 0) {
      iCount += pInventoryPoolList[iCounter].o.ubNumberOfObjects;
    }
  }

  return iCount;
}

// get total number of items in sector
static int32_t GetTotalNumberOfItems() {
  int32_t iCounter, iCount = 0;

  // run through list of items and find out how many are there
  for (iCounter = 0; iCounter < iTotalNumberOfSlots; iCounter++) {
    if (pInventoryPoolList[iCounter].o.ubNumberOfObjects > 0) {
      iCount++;
    }
  }

  return iCount;
}

static void DrawNumberOfIventoryPoolItems() {
  int32_t iNumberOfItems = 0;
  wchar_t sString[32];
  int16_t sX, sY;

  iNumberOfItems = GetTotalNumberOfItemsInSectorStash();

  // get number of items
  swprintf(sString, lengthof(sString), L"%d", iNumberOfItems);

  SetFontAttributes(COMPFONT, 183);
  SetFontDestBuffer(guiSAVEBUFFER);

  // grab centered coords
  const SGPBox *const box = &g_sector_inv_count_box;
  FindFontCenterCoordinates(box->x, box->y, box->w, box->h, sString, COMPFONT, &sX, &sY);
  MPrint(sX, sY, sString);

  SetFontDestBuffer(FRAME_BUFFER);
}

static void CreateMapInventoryPoolDoneButton() {
  // create done button
  guiMapInvenButton[2] = QuickCreateButtonImg(INTERFACEDIR "/done_button.sti", 0, 1, 587, 333,
                                              MSYS_PRIORITY_HIGHEST, MapInventoryPoolDoneBtn);
}

static void DestroyInventoryPoolDoneButton() {
  // destroy ddone button
  RemoveButton(guiMapInvenButton[2]);
}

static void DisplayCurrentSector() {
  // grab current sector being displayed
  wchar_t sString[32];
  int16_t sX, sY;

  swprintf(sString, lengthof(sString), L"%ls%ls%ls", pMapVertIndex[sSelMapY],
           pMapHortIndex[sSelMapX], pMapDepthIndex[iCurrentMapSectorZ]);

  SetFontAttributes(COMPFONT, 183);
  SetFontDestBuffer(guiSAVEBUFFER);

  // grab centered coords
  const SGPBox *const box = &g_sector_inv_loc_box;
  FindFontCenterCoordinates(box->x, box->y, box->w, box->h, sString, COMPFONT, &sX, &sY);
  MPrint(sX, sY, sString);

  SetFontDestBuffer(FRAME_BUFFER);
}

static void CheckAndUnDateSlotAllocation() {
  // will check number of available slots, if less than half a page, allocate a
  // new page
  int32_t iNumberOfTakenSlots = 0;

  // get number of taken slots
  iNumberOfTakenSlots = GetTotalNumberOfItems();

  if ((iTotalNumberOfSlots - iNumberOfTakenSlots) < 2) {
    // not enough space
    // need to make more space
    ReSizeStashListByThisAmount(MAP_INVENTORY_POOL_SLOT_COUNT);
  }

  iLastInventoryPoolPage = ((iTotalNumberOfSlots - 1) / MAP_INVENTORY_POOL_SLOT_COUNT);
}

static void DrawTextOnSectorInventory();

static void DrawTextOnMapInventoryBackground() {
  //	wchar_t sString[ 64 ];
  uint16_t usStringHeight;

  SetFontDestBuffer(guiSAVEBUFFER);

  int xPos = 268;
  int yPos = 342;

  // Calculate the height of the string, as it needs to be vertically centered.
  usStringHeight =
      DisplayWrappedString(xPos, yPos, 53, 1, MAP_IVEN_FONT, FONT_BEIGE, pMapInventoryStrings[0],
                           FONT_BLACK, RIGHT_JUSTIFIED | DONT_DISPLAY_TEXT);
  DisplayWrappedString(xPos, yPos - (usStringHeight / 2), 53, 1, MAP_IVEN_FONT, FONT_BEIGE,
                       pMapInventoryStrings[0], FONT_BLACK, RIGHT_JUSTIFIED);

  xPos = 369;

  // Calculate the height of the string, as it needs to be vertically centered.
  usStringHeight =
      DisplayWrappedString(xPos, yPos, 65, 1, MAP_IVEN_FONT, FONT_BEIGE, pMapInventoryStrings[1],
                           FONT_BLACK, RIGHT_JUSTIFIED | DONT_DISPLAY_TEXT);
  DisplayWrappedString(xPos, yPos - (usStringHeight / 2), 65, 1, MAP_IVEN_FONT, FONT_BEIGE,
                       pMapInventoryStrings[1], FONT_BLACK, RIGHT_JUSTIFIED);

  DrawTextOnSectorInventory();

  SetFontDestBuffer(FRAME_BUFFER);
}

void HandleButtonStatesWhileMapInventoryActive() {
  // are we even showing the amp inventory pool graphic?
  if (!fShowMapInventoryPool) return;

  // first page, can't go back any
  EnableButton(guiMapInvenButton[1], iCurrentInventoryPoolPage != 0);
  // last page, go no further
  EnableButton(guiMapInvenButton[0], iCurrentInventoryPoolPage != iLastInventoryPoolPage);
  // item picked up ..disable button
  EnableButton(guiMapInvenButton[2], !fMapInventoryItem);
}

static void DrawTextOnSectorInventory() {
  SetFontDestBuffer(guiSAVEBUFFER);
  SetFontAttributes(FONT14ARIAL, FONT_WHITE);

  int16_t x;
  int16_t y;
  const SGPBox *const box = &g_sector_inv_title_box;
  const wchar_t *const title = zMarksMapScreenText[11];
  FindFontCenterCoordinates(box->x, box->y, box->w, box->h, title, FONT14ARIAL, &x, &y);
  MPrint(x, y, title);

  SetFontDestBuffer(FRAME_BUFFER);
}

void HandleFlashForHighLightedItem() {
  int32_t iCurrentTime = 0;
  int32_t iDifference = 0;

  // if there is an invalid item, reset
  if (iCurrentlyHighLightedItem == -1) {
    fFlashHighLightInventoryItemOnradarMap = FALSE;
    giFlashHighlightedItemBaseTime = 0;
  }

  // get the current time
  iCurrentTime = GetJA2Clock();

  // if there basetime is uninit
  if (giFlashHighlightedItemBaseTime == 0) {
    giFlashHighlightedItemBaseTime = iCurrentTime;
  }

  iDifference = iCurrentTime - giFlashHighlightedItemBaseTime;

  if (iDifference > DELAY_FOR_HIGHLIGHT_ITEM_FLASH) {
    // reset timer
    giFlashHighlightedItemBaseTime = iCurrentTime;

    // flip flag
    fFlashHighLightInventoryItemOnradarMap = !fFlashHighLightInventoryItemOnradarMap;

    // re render radar map
    RenderRadarScreen();
  }
}

static void ResetMapSectorInventoryPoolHighLights();

static void HandleMouseInCompatableItemForMapSectorInventory(int32_t iCurrentSlot) {
  SOLDIERTYPE *pSoldier = NULL;
  static BOOLEAN fItemWasHighLighted = FALSE;

  if (iCurrentSlot == -1) {
    giCompatibleItemBaseTime = 0;
  }

  if (fChangedInventorySlots) {
    giCompatibleItemBaseTime = 0;
    fChangedInventorySlots = FALSE;
  }

  // reset the base time to the current game clock
  if (giCompatibleItemBaseTime == 0) {
    giCompatibleItemBaseTime = GetJA2Clock();

    if (fItemWasHighLighted) {
      fTeamPanelDirty = TRUE;
      fMapPanelDirty = TRUE;
      fItemWasHighLighted = FALSE;
    }
  }

  ResetCompatibleItemArray();
  ResetMapSectorInventoryPoolHighLights();

  if (iCurrentSlot == -1) {
    return;
  }

  // given this slot value, check if anything in the displayed sector inventory
  // or on the mercs inventory is compatable
  if (fShowInventoryFlag) {
    // check if any compatable items in the soldier inventory matches with this
    // item
    if (gfCheckForCursorOverMapSectorInventoryItem) {
      const SOLDIERTYPE *const pSoldier = GetSelectedInfoChar();
      if (pSoldier) {
        if (HandleCompatibleAmmoUIForMapScreen(
                pSoldier,
                iCurrentSlot + (iCurrentInventoryPoolPage * MAP_INVENTORY_POOL_SLOT_COUNT), TRUE,
                FALSE)) {
          if (GetJA2Clock() - giCompatibleItemBaseTime > 100) {
            if (!fItemWasHighLighted) {
              fTeamPanelDirty = TRUE;
              fItemWasHighLighted = TRUE;
            }
          }
        }
      }
    } else {
      giCompatibleItemBaseTime = 0;
    }
  }

  // now handle for the sector inventory
  if (fShowMapInventoryPool) {
    // check if any compatable items in the soldier inventory matches with this
    // item
    if (gfCheckForCursorOverMapSectorInventoryItem) {
      if (HandleCompatibleAmmoUIForMapInventory(
              pSoldier, iCurrentSlot, (iCurrentInventoryPoolPage * MAP_INVENTORY_POOL_SLOT_COUNT),
              TRUE, FALSE)) {
        if (GetJA2Clock() - giCompatibleItemBaseTime > 100) {
          if (!fItemWasHighLighted) {
            fItemWasHighLighted = TRUE;
            fMapPanelDirty = TRUE;
          }
        }
      }
    } else {
      giCompatibleItemBaseTime = 0;
    }
  }
}

static void ResetMapSectorInventoryPoolHighLights() {  // Reset the highlight list for the
                                                       // map sector inventory.
  FOR_EACH(BOOLEAN, i, fMapInventoryItemCompatable) *i = FALSE;
}

static void HandleMapSectorInventory() {
  // handle mouse in compatable item map sectors inventory
  HandleMouseInCompatableItemForMapSectorInventory(iCurrentlyHighLightedItem);
}

// CJC look here to add/remove checks for the sector inventory
BOOLEAN
IsMapScreenWorldItemVisibleInMapInventory(const WORLDITEM *const pWorldItem) {
  if (pWorldItem->fExists && pWorldItem->bVisible == VISIBLE && pWorldItem->o.usItem != SWITCH &&
      pWorldItem->o.usItem != ACTION_ITEM && pWorldItem->o.bTrap <= 0) {
    return (TRUE);
  }

  return (FALSE);
}

// Check to see if any of the items in the list have a gridno of NOWHERE and the
// entry point flag NOT set
static void CheckGridNoOfItemsInMapScreenMapInventory() {
  int32_t iCnt;
  uint32_t uiNumFlagsNotSet = 0;
  int32_t iTotalNumberItems = GetTotalNumberOfItems();

  for (iCnt = 0; iCnt < iTotalNumberItems; iCnt++) {
    if (pInventoryPoolList[iCnt].sGridNo == NOWHERE &&
        !(pInventoryPoolList[iCnt].usFlags & WORLD_ITEM_GRIDNO_NOT_SET_USE_ENTRY_POINT)) {
      // set the flag
      pInventoryPoolList[iCnt].usFlags |= WORLD_ITEM_GRIDNO_NOT_SET_USE_ENTRY_POINT;

      // count the number
      uiNumFlagsNotSet++;
    }
  }

  // loop through all the UNSEEN items
  for (iCnt = 0; iCnt < (int32_t)uiNumberOfUnSeenItems; iCnt++) {
    if (pUnSeenItems[iCnt].sGridNo == NOWHERE &&
        !(pUnSeenItems[iCnt].usFlags & WORLD_ITEM_GRIDNO_NOT_SET_USE_ENTRY_POINT)) {
      // set the flag
      pUnSeenItems[iCnt].usFlags |= WORLD_ITEM_GRIDNO_NOT_SET_USE_ENTRY_POINT;

      // count the number
      uiNumFlagsNotSet++;
    }
  }
}

static int32_t MapScreenSectorInventoryCompare(const void *pNum1, const void *pNum2);

static void SortSectorInventory(WORLDITEM *pInventory, uint32_t uiSizeOfArray) {
  qsort(pInventory, uiSizeOfArray, sizeof(WORLDITEM), MapScreenSectorInventoryCompare);
}

static int32_t MapScreenSectorInventoryCompare(const void *pNum1, const void *pNum2) {
  WORLDITEM *pFirst = (WORLDITEM *)pNum1;
  WORLDITEM *pSecond = (WORLDITEM *)pNum2;
  uint16_t usItem1Index;
  uint16_t usItem2Index;
  uint8_t ubItem1Quality;
  uint8_t ubItem2Quality;

  usItem1Index = pFirst->o.usItem;
  usItem2Index = pSecond->o.usItem;

  ubItem1Quality = pFirst->o.bStatus[0];
  ubItem2Quality = pSecond->o.bStatus[0];

  return (CompareItemsForSorting(usItem1Index, usItem2Index, ubItem1Quality, ubItem2Quality));
}

static BOOLEAN CanPlayerUseSectorInventory() {
  int16_t x;
  int16_t y;
  int16_t z;
  return !GetCurrentBattleSectorXYZAndReturnTRUEIfThereIsABattle(&x, &y, &z) || sSelMapX != x ||
         sSelMapY != y || iCurrentMapSectorZ != z;
}
