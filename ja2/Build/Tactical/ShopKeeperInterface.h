// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef _SHOPKEEPER_INTERFACE__H_
#define _SHOPKEEPER_INTERFACE__H_

#include "JA2Types.h"
#include "MessageBoxScreen.h"
#include "ScreenIDs.h"
#include "Tactical/ArmsDealer.h"
#include "Tactical/ItemTypes.h"

// Enums used for when the user clicks on an item and the item goes to..
enum {
  ARMS_DEALER_INVENTORY,
  ARMS_DEALER_OFFER_AREA,
  PLAYERS_OFFER_AREA,
  PLAYERS_INVENTORY,
};

#define ARMS_INV_ITEM_SELECTED 0x00000001  // The item has been placed into the offer area
// #define	ARMS_INV_PLAYERS_ITEM_SELECTED
//  0x00000002			// The source location for the item has been
//  selected
#define ARMS_INV_PLAYERS_ITEM_HAS_VALUE \
  0x00000004  // The Players item is worth something to this dealer
// #define	ARMS_INV_ITEM_HIGHLIGHTED
//  0x00000008			// If the items is highlighted
#define ARMS_INV_ITEM_NOT_REPAIRED_YET \
  0x00000010                                // The item is in for repairs but not repaired yet
#define ARMS_INV_ITEM_REPAIRED 0x00000020   // The item is repaired
#define ARMS_INV_JUST_PURCHASED 0x00000040  // The item was just purchased
#define ARMS_INV_PLAYERS_ITEM_HAS_BEEN_EVALUATED 0x00000080  // The Players item has been evaluated

struct INVENTORY_IN_SLOT {
  BOOLEAN fActive;
  int16_t sItemIndex;
  uint32_t uiFlags;
  OBJECTTYPE ItemObject;
  uint8_t ubLocationOfObject;  // An enum value for the location of the item (
                               // either in the arms dealers inventory, one of the
                               // offer areas or in the users inventory)
  int8_t bSlotIdInOtherLocation;

  uint8_t ubIdOfMercWhoOwnsTheItem;
  uint32_t uiItemPrice;  // Only used for the players item that have been evaluated

  int16_t sSpecialItemElement;  // refers to which special item element an item in
                                // a dealer's inventory area occupies.  -1 Means
                                // the item is "perfect" and has no associated
                                // special item.
};

enum {
  SKI_DIRTY_LEVEL0,  // no redraw
  SKI_DIRTY_LEVEL1,  // redraw only items
  SKI_DIRTY_LEVEL2,  // redraw everything
};

extern uint8_t gubSkiDirtyLevel;

extern const OBJECTTYPE *gpHighLightedItemObject;

extern INVENTORY_IN_SLOT gMoveingItem;

extern OBJECTTYPE *pShopKeeperItemDescObject;

void ShopKeeperScreenInit();
ScreenID ShopKeeperScreenHandle();
void ShopKeeperScreenShutdown();

void EnterShopKeeperInterfaceScreen(uint8_t ubArmsDealer);

void DrawHatchOnInventory(SGPVSurface *dst, uint16_t usPosX, uint16_t usPosY, uint16_t usWidth,
                          uint16_t usHeight);
BOOLEAN ShouldSoldierDisplayHatchOnItem(uint8_t ubProfileID, int16_t sSlotNum);
void ConfirmToDeductMoneyFromPlayersAccountMessageBoxCallBack(MessageBoxReturnValue);
void ConfirmDontHaveEnoughForTheDealerMessageBoxCallBack(MessageBoxReturnValue);

void SetSkiCursor(uint16_t usCursor);

void InitShopKeeperSubTitledText(const wchar_t *pString);

void AddItemToPlayersOfferAreaAfterShopKeeperOpen(OBJECTTYPE *pItemObject, int8_t bPreviousInvPos);

void BeginSkiItemPointer(uint8_t ubSource, int8_t bSlotNum, BOOLEAN fOfferToDealerFirst);

void DeleteShopKeeperItemDescBox();

BOOLEAN CanMercInteractWithSelectedShopkeeper(const SOLDIERTYPE *s);

void RestrictSkiMouseCursor();

void DoSkiMessageBox(wchar_t const *zString, ScreenID uiExitScreen, MessageBoxFlags,
                     MSGBOX_CALLBACK ReturnCallback);
void StartSKIDescriptionBox();

#endif
