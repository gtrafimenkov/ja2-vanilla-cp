// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __WORLD_ITEMS
#define __WORLD_ITEMS

#include <stdlib.h>

#include "SGP/Debug.h"
#include "Tactical/ItemTypes.h"

#define WORLD_ITEM_DONTRENDER 0x0001
#define WOLRD_ITEM_FIND_SWEETSPOT_FROM_GRIDNO 0x0002
#define WORLD_ITEM_ARMED_BOMB 0x0040
#define WORLD_ITEM_SCIFI_ONLY 0x0080
#define WORLD_ITEM_REALISTIC_ONLY 0x0100
#define WORLD_ITEM_REACHABLE 0x0200
#define WORLD_ITEM_GRIDNO_NOT_SET_USE_ENTRY_POINT 0x0400

struct WORLDITEM {
  BOOLEAN fExists;
  int16_t sGridNo;
  uint8_t ubLevel;
  OBJECTTYPE o;
  uint16_t usFlags;
  int8_t bRenderZHeightAboveLevel;

  int8_t bVisible;

  // This is the chance associated with an item or a trap not-existing in the
  // world.  The reason why this is reversed (10 meaning item has 90% chance of
  // appearing, is because the order that the map is saved, we don't know if the
  // version is older or not until after the items are loaded and added. Because
  // this value is zero in the saved maps, we can't change it to 100, hence the
  // reversal method. This check is only performed the first time a map is
  // loaded.  Later, it is entirely skipped.
  uint8_t ubNonExistChance;
};

extern WORLDITEM *gWorldItems;

// number of items in currently loaded sector
extern uint32_t guiNumWorldItems;

static inline WORLDITEM &GetWorldItem(size_t const idx) {
  Assert(idx < guiNumWorldItems);
  return gWorldItems[idx];
}

#define BASE_FOR_EACH_WORLD_ITEM(type, iter)                                          \
  for (type *iter = gWorldItems, *const end__##iter = gWorldItems + guiNumWorldItems; \
       iter != end__##iter; ++iter)                                                   \
    if (!iter->fExists)                                                               \
      continue;                                                                       \
    else
#define FOR_EACH_WORLD_ITEM(iter) BASE_FOR_EACH_WORLD_ITEM(WORLDITEM, iter)
#define CFOR_EACH_WORLD_ITEM(iter) BASE_FOR_EACH_WORLD_ITEM(const WORLDITEM, iter)

int32_t AddItemToWorld(int16_t sGridNo, const OBJECTTYPE *pObject, uint8_t ubLevel,
                       uint16_t usFlags, int8_t bRenderZHeightAboveLevel, int8_t bVisible);
void RemoveItemFromWorld(int32_t iItemIndex);
int32_t FindWorldItem(uint16_t usItem);

void LoadWorldItemsFromMap(HWFILE);

void SaveWorldItemsToMap(HWFILE fp);

void TrashWorldItems();

struct WORLDBOMB {
  BOOLEAN fExists;
  int32_t iItemIndex;
};

extern WORLDBOMB *gWorldBombs;
extern uint32_t guiNumWorldBombs;

#define BASE_FOR_EACH_WORLD_BOMB(type, iter)                                          \
  for (type *iter = gWorldBombs, *const end__##iter = gWorldBombs + guiNumWorldBombs; \
       iter != end__##iter; ++iter)                                                   \
    if (!iter->fExists)                                                               \
      continue;                                                                       \
    else
#define FOR_EACH_WORLD_BOMB(iter) BASE_FOR_EACH_WORLD_BOMB(WORLDBOMB, iter)
#define CFOR_EACH_WORLD_BOMB(iter) BASE_FOR_EACH_WORLD_BOMB(const WORLDBOMB, iter)

extern void FindPanicBombsAndTriggers();
extern int32_t FindWorldItemForBombInGridNo(int16_t sGridNo, int8_t bLevel);

void RefreshWorldItemsIntoItemPools(const WORLDITEM *pItemList, int32_t iNumberOfItems);

#endif
