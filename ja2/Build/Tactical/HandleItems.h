#ifndef __HANDLE_ITEMS_H
#define __HANDLE_ITEMS_H

#include "JA2Types.h"
#include "Tactical/WorldItems.h"

enum ItemHandleResult {
  ITEM_HANDLE_OK = 1,
  ITEM_HANDLE_RELOADING = -1,
  ITEM_HANDLE_UNCONSCIOUS = -2,
  ITEM_HANDLE_NOAPS = -3,
  ITEM_HANDLE_NOAMMO = -4,
  ITEM_HANDLE_CANNOT_GETTO_LOCATION = -5,
  ITEM_HANDLE_BROKEN = -6,
  ITEM_HANDLE_NOROOM = -7,
  ITEM_HANDLE_REFUSAL = -8
};

// Define for code to try and pickup all items....
#define ITEM_PICKUP_ACTION_ALL 32000
#define ITEM_PICKUP_SELECTION 31000

#define ITEM_IGNORE_Z_LEVEL -1

enum Visibility {
  ANY_VISIBILITY_VALUE = -10,
  HIDDEN_ITEM = -4,
  BURIED = -3,
  HIDDEN_IN_OBJECT = -2,
  INVISIBLE = -1,
  VISIBILITY_0 = 0,  // XXX investigate
  VISIBLE = 1
};

#define ITEM_LOCATOR_LOCKED 0x02

/* Check if at least one item in the item pool is visible */
bool IsItemPoolVisible(ITEM_POOL const *);

struct ITEM_POOL {
  ITEM_POOL *pNext;
  int32_t iItemIndex;
  LEVELNODE *pLevelNode;
};

ItemHandleResult HandleItem(SOLDIERTYPE *pSoldier, int16_t usGridNo, int8_t bLevel,
                            uint16_t usHandItem, BOOLEAN fFromUI);

/* iItemIndex is ignored for player soldiers */
void SoldierPickupItem(SOLDIERTYPE *pSoldier, int32_t iItemIndex, int16_t sGridNo, int8_t bZLevel);

void HandleSoldierPickupItem(SOLDIERTYPE *pSoldier, int32_t iItemIndex, int16_t sGridNo,
                             int8_t bZLevel);
void HandleFlashingItems();

void SoldierDropItem(SOLDIERTYPE *, OBJECTTYPE *);

void HandleSoldierThrowItem(SOLDIERTYPE *pSoldier, int16_t sGridNo);
SOLDIERTYPE *VerifyGiveItem(SOLDIERTYPE *pSoldier);
void SoldierGiveItemFromAnimation(SOLDIERTYPE *pSoldier);
void SoldierGiveItem(SOLDIERTYPE *pSoldier, SOLDIERTYPE *pTargetSoldier, OBJECTTYPE *pObject,
                     int8_t bInvPos);

void NotifySoldiersToLookforItems();
void AllSoldiersLookforItems();

void SoldierGetItemFromWorld(SOLDIERTYPE *pSoldier, int32_t iItemIndex, int16_t sGridNo,
                             int8_t bZLevel, const BOOLEAN *pfSelectionList);

int32_t AddItemToPool(int16_t sGridNo, OBJECTTYPE *pObject, Visibility, uint8_t ubLevel,
                      uint16_t usFlags, int8_t bRenderZHeightAboveLevel);
int32_t InternalAddItemToPool(int16_t *psGridNo, OBJECTTYPE *pObject, Visibility, uint8_t ubLevel,
                              uint16_t usFlags, int8_t bRenderZHeightAboveLevel);

GridNo AdjustGridNoForItemPlacement(SOLDIERTYPE *, GridNo);
ITEM_POOL *GetItemPool(uint16_t usMapPos, uint8_t ubLevel);
void DrawItemPoolList(const ITEM_POOL *pItemPool, int8_t bZLevel, int16_t sXPos, int16_t sYPos);
void RemoveItemFromPool(WORLDITEM *);
void MoveItemPools(int16_t sStartPos, int16_t sEndPos);

BOOLEAN SetItemsVisibilityOn(GridNo, uint8_t level, Visibility bAllGreaterThan,
                             BOOLEAN fSetLocator);

void SetItemsVisibilityHidden(GridNo, uint8_t level);

void RenderTopmostFlashingItems();

void RemoveAllUnburiedItems(int16_t sGridNo, uint8_t ubLevel);

BOOLEAN DoesItemPoolContainAnyHiddenItems(const ITEM_POOL *pItemPool);

void HandleSoldierDropBomb(SOLDIERTYPE *pSoldier, int16_t sGridNo);
void HandleSoldierUseRemote(SOLDIERTYPE *pSoldier, int16_t sGridNo);

BOOLEAN ItemPoolOKForDisplay(const ITEM_POOL *pItemPool, int8_t bZLevel);

void SoldierHandleDropItem(SOLDIERTYPE *pSoldier);

int8_t GetZLevelOfItemPoolGivenStructure(int16_t sGridNo, uint8_t ubLevel,
                                         const STRUCTURE *pStructure);

int8_t GetLargestZLevelOfItemPool(const ITEM_POOL *pItemPool);

BOOLEAN NearbyGroundSeemsWrong(SOLDIERTYPE *pSoldier, int16_t sGridNo, BOOLEAN fCheckAroundGridno,
                               int16_t *psProblemGridNo);
void MineSpottedDialogueCallBack();

extern int16_t gsBoobyTrapGridNo;
extern SOLDIERTYPE *gpBoobyTrapSoldier;
void RemoveBlueFlag(int16_t sGridNo, int8_t bLevel);

// check if item is booby trapped
BOOLEAN ContinuePastBoobyTrapInMapScreen(OBJECTTYPE *pObject, SOLDIERTYPE *pSoldier);

void RefreshItemPools(const WORLDITEM *pItemList, int32_t iNumberOfItems);

BOOLEAN ItemTypeExistsAtLocation(int16_t sGridNo, uint16_t usItem, uint8_t ubLevel,
                                 int32_t *piItemIndex);

int16_t FindNearestAvailableGridNoForItem(int16_t sSweetGridNo, int8_t ubRadius);

void MakeNPCGrumpyForMinorOffense(SOLDIERTYPE *pSoldier, const SOLDIERTYPE *pOffendingSoldier);

BOOLEAN AnyItemsVisibleOnLevel(const ITEM_POOL *pItemPool, int8_t bZLevel);

void RemoveFlashItemSlot(ITEM_POOL const *);

void ToggleItemGlow(BOOLEAN fOn);

BOOLEAN HandleCheckForBadChangeToGetThrough(SOLDIERTYPE *pSoldier,
                                            const SOLDIERTYPE *pTargetSoldier,
                                            int16_t sTargetGridNo, int8_t bLevel);

#endif
