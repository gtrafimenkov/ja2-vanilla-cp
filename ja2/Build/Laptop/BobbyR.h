// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __BOBBYR_H
#define __BOBBYR_H

#include "Laptop/Laptop.h"
#include "Laptop/StoreInventory.h"
#include "Tactical/ItemTypes.h"

void EnterBobbyR();
void ExitBobbyR();
void HandleBobbyR();
void RenderBobbyR();

#define BOBBYR_BACKGROUND_WIDTH 125
#define BOBBYR_BACKGROUND_HEIGHT 100
#define BOBBYR_NUM_HORIZONTAL_TILES 4
#define BOBBYR_NUM_VERTICAL_TILES 4

#define BOBBYR_GRIDLOC_X LAPTOP_SCREEN_UL_X + 4
#define BOBBYR_GRIDLOC_Y LAPTOP_SCREEN_WEB_UL_Y + 45

extern LaptopMode guiLastBobbyRayPage;

void DrawBobbyRWoodBackground();
void DeleteBobbyRWoodBackground();
void InitBobbyRWoodBackground();
void DailyUpdateOfBobbyRaysNewInventory();
void DailyUpdateOfBobbyRaysUsedInventory();
void AddFreshBobbyRayInventory(uint16_t usItemIndex);
void InitBobbyRayInventory();
void CancelAllPendingBRPurchaseOrders();
int16_t GetInventorySlotForItem(STORE_INVENTORY *pInventoryArray, uint16_t usItemIndex,
                                BOOLEAN fUsed);

#endif
