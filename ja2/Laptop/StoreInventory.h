// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __STORE_INVENTORY_H_
#define __STORE_INVENTORY_H_

#include "Tactical/ItemTypes.h"

struct STORE_INVENTORY {
  uint16_t usItemIndex;  // Index into the item table
  uint8_t ubQtyOnHand;
  uint8_t ubQtyOnOrder;         // The number of items on order
  uint8_t ubItemQuality;        // the % damaged listed from 0 to 100
  BOOLEAN fPreviouslyEligible;  // whether or not dealer has been eligible to
                                // sell this item in days prior to today
};

// Enums used for the access the std::max dealers array
enum {
  BOBBY_RAY_NEW,
  BOBBY_RAY_USED,

  BOBBY_RAY_LISTS,
};

extern uint8_t StoreInventory[MAXITEMS][BOBBY_RAY_LISTS];
extern int16_t WeaponROF[MAX_WEAPONS];

void SetupStoreInventory(STORE_INVENTORY *pInventoryArray, BOOLEAN fUsed);

#endif
