#ifndef _ARMS_DEALER_INV_INIT__H_
#define _ARMS_DEALER_INV_INIT__H_

#include "SGP/Types.h"
#include "Tactical/ArmsDealer.h"

#define LAST_DEALER_ITEM -1
#define NO_DEALER_ITEM 0

// item suitability categories for dealer inventory initialization, virtual
// customer sales, and re-ordering
#define ITEM_SUITABILITY_NONE 0
#define ITEM_SUITABILITY_LOW 1
#define ITEM_SUITABILITY_MEDIUM 2
#define ITEM_SUITABILITY_HIGH 3
#define ITEM_SUITABILITY_ALWAYS 4

#define DEALER_BUYING 0
#define DEALER_SELLING 1

struct DEALER_POSSIBLE_INV {
  int16_t sItemIndex;
  uint8_t ubOptimalNumber;
};

int8_t GetDealersMaxItemAmount(ArmsDealerID, uint16_t usItemIndex);

DEALER_POSSIBLE_INV const *GetPointerToDealersPossibleInventory(ArmsDealerID);

uint8_t ChanceOfItemTransaction(ArmsDealerID, uint16_t usItemIndex, BOOLEAN fDealerSelling,
                                BOOLEAN fUsed);
BOOLEAN ItemTransactionOccurs(ArmsDealerID, uint16_t usItemIndex, BOOLEAN fDealerSelling,
                              BOOLEAN fUsed);
uint8_t DetermineInitialInvItems(ArmsDealerID, uint16_t usItemIndex, uint8_t ubChances,
                                 BOOLEAN fUsed);
uint8_t HowManyItemsAreSold(ArmsDealerID, uint16_t usItemIndex, uint8_t ubNumInStock,
                            BOOLEAN fUsed);
uint8_t HowManyItemsToReorder(uint8_t ubWanted, uint8_t ubStillHave);

int BobbyRayItemQsortCompare(const void *pArg1, const void *pArg2);
int ArmsDealerItemQsortCompare(const void *pArg1, const void *pArg2);
int CompareItemsForSorting(uint16_t usItem1Index, uint16_t usItem2Index, uint8_t ubItem1Quality,
                           uint8_t ubItem2Quality);
BOOLEAN CanDealerItemBeSoldUsed(uint16_t usItemIndex);

#endif
