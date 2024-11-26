#ifndef __INTERFACE_ITEMS_H
#define __INTERFACE_ITEMS_H

#include <stdlib.h>

#include "SGP/ButtonSystem.h"
#include "SGP/MouseSystem.h"
#include "Tactical/Interface.h"
#include "Tactical/SoldierControl.h"

// DEFINES FOR ITEM SLOT SIZES IN PIXELS
#define BIG_INV_SLOT_WIDTH 61
#define BIG_INV_SLOT_HEIGHT 22
#define SM_INV_SLOT_WIDTH 30
#define SM_INV_SLOT_HEIGHT 23
#define VEST_INV_SLOT_WIDTH 43
#define VEST_INV_SLOT_HEIGHT 24
#define LEGS_INV_SLOT_WIDTH 43
#define LEGS_INV_SLOT_HEIGHT 24
#define HEAD_INV_SLOT_WIDTH 43
#define HEAD_INV_SLOT_HEIGHT 24

// USED TO SETUP REGION POSITIONS, ETC
struct INV_REGION_DESC {
  int16_t sX;
  int16_t sY;
};

// Itempickup stuff
void InitializeItemPickupMenu(SOLDIERTYPE *pSoldier, int16_t sGridNo, ITEM_POOL *pItemPool,
                              int8_t bZLevel);
void RenderItemPickupMenu();
void RemoveItemPickupMenu();
void SetItemPickupMenuDirty(BOOLEAN fDirtyLevel);
BOOLEAN HandleItemPickupMenu();

// FUNCTIONS FOR INTERFACEING WITH ITEM PANEL STUFF
void InitInvSlotInterface(INV_REGION_DESC const *pRegionDesc, INV_REGION_DESC const *pCamoRegion,
                          MOUSE_CALLBACK INVMoveCallback, MOUSE_CALLBACK INVClickCallback,
                          MOUSE_CALLBACK INVMoveCamoCallback, MOUSE_CALLBACK INVClickCamoCallback);
void ShutdownInvSlotInterface();
void HandleRenderInvSlots(SOLDIERTYPE const &, DirtyLevel);
void HandleNewlyAddedItems(SOLDIERTYPE &, DirtyLevel *);
void RenderInvBodyPanel(const SOLDIERTYPE *pSoldier, int16_t sX, int16_t sY);
void DisableInvRegions(BOOLEAN fDisable);

void DegradeNewlyAddedItems();
void CheckForAnyNewlyAddedItems(SOLDIERTYPE *pSoldier);

BOOLEAN HandleCompatibleAmmoUI(const SOLDIERTYPE *pSoldier, int8_t bInvPos, BOOLEAN fOn);

// THIS FUNCTION IS CALLED TO RENDER AN ITEM.
// uiBuffer - The Dest Video Surface - can only be FRAME_BUFFER or guiSAVEBUFFER
// pSoldier - used for determining whether burst mode needs display
// pObject	- Usually taken from pSoldier->inv[HANDPOS]
// sX, sY, Width, Height,  - Will Center it in the Width
// fDirtyLevel  if == DIRTYLEVEL2 will render everything
//							if == DIRTYLEVEL1 will render
// bullets and status only
//
//  Last parameter used mainly for when mouse is over item
void INVRenderItem(SGPVSurface *uiBuffer, SOLDIERTYPE const *pSoldier, OBJECTTYPE const &,
                   int16_t sX, int16_t sY, int16_t sWidth, int16_t sHeight, DirtyLevel,
                   uint8_t ubStatusIndex, int16_t sOutlineColor);

extern BOOLEAN gfInItemDescBox;

BOOLEAN InItemDescriptionBox();
void InitItemDescriptionBox(SOLDIERTYPE *pSoldier, uint8_t ubPosition, int16_t sX, int16_t sY,
                            uint8_t ubStatusIndex);
void InternalInitItemDescriptionBox(OBJECTTYPE *pObject, int16_t sX, int16_t sY,
                                    uint8_t ubStatusIndex, SOLDIERTYPE *pSoldier);
void InitKeyItemDescriptionBox(SOLDIERTYPE *pSoldier, uint8_t ubPosition, int16_t sX, int16_t sY);
void RenderItemDescriptionBox();
void HandleItemDescriptionBox(DirtyLevel *);
void DeleteItemDescriptionBox();

BOOLEAN InItemStackPopup();
void InitItemStackPopup(SOLDIERTYPE *pSoldier, uint8_t ubPosition, int16_t sInvX, int16_t sInvY,
                        int16_t sInvWidth, int16_t sInvHeight);
void RenderItemStackPopup(BOOLEAN fFullRender);

// keyring handlers
void InitKeyRingPopup(SOLDIERTYPE *pSoldier, int16_t sInvX, int16_t sInvY, int16_t sInvWidth,
                      int16_t sInvHeight);
void RenderKeyRingPopup(BOOLEAN fFullRender);
void InitKeyRingInterface(MOUSE_CALLBACK KeyRingClickCallback);
void InitMapKeyRingInterface(MOUSE_CALLBACK KeyRingClickCallback);
void DeleteKeyRingPopup();

void ShutdownKeyRingInterface();
BOOLEAN InKeyRingPopup();
void BeginKeyRingItemPointer(SOLDIERTYPE *pSoldier, uint8_t ubKeyRingPosition);

extern OBJECTTYPE *gpItemPointer;
extern OBJECTTYPE gItemPointer;
extern SOLDIERTYPE *gpItemPointerSoldier;
extern BOOLEAN gfItemPointerDifferentThanDefault;

void BeginItemPointer(SOLDIERTYPE *pSoldier, uint8_t ubHandPos);
void InternalBeginItemPointer(SOLDIERTYPE *pSoldier, OBJECTTYPE *pObject, int8_t bHandPos);
void EndItemPointer();
void DrawItemFreeCursor();
void DrawItemTileCursor();
BOOLEAN HandleItemPointerClick(uint16_t usMapPos);
SGPVObject const &GetInterfaceGraphicForItem(INVTYPE const &);
uint16_t GetTileGraphicForItem(INVTYPE const &);
SGPVObject *LoadTileGraphicForItem(INVTYPE const &);

void GetHelpTextForItem(wchar_t *buf, size_t length, OBJECTTYPE const &);

void CancelItemPointer();

void LoadItemCursorFromSavedGame(HWFILE);
void SaveItemCursorToSavedGame(HWFILE);

// handle compatable items for merc and map inventory
BOOLEAN HandleCompatibleAmmoUIForMapScreen(const SOLDIERTYPE *pSoldier, int32_t bInvPos,
                                           BOOLEAN fOn, BOOLEAN fFromMerc);
BOOLEAN HandleCompatibleAmmoUIForMapInventory(SOLDIERTYPE *pSoldier, int32_t bInvPos,
                                              int32_t iStartSlotNumber, BOOLEAN fOn,
                                              BOOLEAN fFromMerc);
void ResetCompatibleItemArray();

void CycleItemDescriptionItem();

void UpdateItemHatches();

extern BOOLEAN gfInKeyRingPopup;
extern BOOLEAN gfInItemPickupMenu;
extern SOLDIERTYPE *gpItemPopupSoldier;
extern int8_t gbCompatibleApplyItem;
extern int8_t gbInvalidPlacementSlot[NUM_INV_SLOTS];
extern MOUSE_REGION gInvDesc;
extern BOOLEAN gfAddingMoneyToMercFromPlayersAccount;
extern MOUSE_REGION gItemDescAttachmentRegions[MAX_ATTACHMENTS];
extern int8_t gbItemPointerSrcSlot;
extern BOOLEAN gfDontChargeAPsToPickup;
extern GUIButtonRef giMapInvDescButton;

void HandleAnyMercInSquadHasCompatibleStuff(const OBJECTTYPE *pObject);
BOOLEAN InternalHandleCompatibleAmmoUI(const SOLDIERTYPE *pSoldier, const OBJECTTYPE *pTestObject,
                                       BOOLEAN fOn);

void SetMouseCursorFromItem(uint16_t item_idx);
void SetMouseCursorFromCurrentItem();

void SetItemPointer(OBJECTTYPE *, SOLDIERTYPE *);

void LoadInterfaceItemsGraphics();
void DeleteInterfaceItemsGraphics();

#endif
