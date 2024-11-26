#ifndef _MAP_INTERFACE_MAP_INVEN_H
#define _MAP_INTERFACE_MAP_INVEN_H

#include "SGP/Types.h"
#include "Tactical/WorldItems.h"

// number of inventory slots
#define MAP_INVENTORY_POOL_SLOT_COUNT 45

// whether we are showing the inventory pool graphic
extern BOOLEAN fShowMapInventoryPool;

// load inventory pool graphic
void LoadInventoryPoolGraphic();

// remove inventory pool graphic
void RemoveInventoryPoolGraphic();

// blit the inventory graphic
void BlitInventoryPoolGraphic();

// which buttons in map invneotyr panel?
void HandleButtonStatesWhileMapInventoryActive();

// handle creation and destruction of map inventory pool buttons
void CreateDestroyMapInventoryPoolButtons(BOOLEAN fExitFromMapScreen);

// bail out of sector inventory mode if it is on
void CancelSectorInventoryDisplayIfOn(BOOLEAN fExitFromMapScreen);

// handle flash of inventory items
void HandleFlashForHighLightedItem();

// the list for the inventory
extern WORLDITEM *pInventoryPoolList;

// autoplace down object
void AutoPlaceObjectInInventoryStash(OBJECTTYPE *pItemPtr);

// the current inventory item
extern int32_t iCurrentlyHighLightedItem;
extern BOOLEAN fFlashHighLightInventoryItemOnradarMap;
extern int16_t sObjectSourceGridNo;
extern int32_t iCurrentInventoryPoolPage;
extern BOOLEAN fMapInventoryItemCompatable[];

BOOLEAN IsMapScreenWorldItemVisibleInMapInventory(const WORLDITEM *);

#endif
