// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef _LAPTOP_SAVE_H_
#define _LAPTOP_SAVE_H_

#include "Laptop/StoreInventory.h"
#include "Tactical/ItemTypes.h"

#define MAX_BOOKMARKS 20

#define MAX_PURCHASE_AMOUNT 10

#define SPECK_QUOTE__ALREADY_TOLD_PLAYER_THAT_LARRY_RELAPSED 0x00000001
#define SPECK_QUOTE__SENT_EMAIL_ABOUT_LACK_OF_PAYMENT 0x00000002

struct LIFE_INSURANCE_PAYOUT {
  BOOLEAN fActive;
  uint8_t ubSoldierID;
  uint8_t ubMercID;
  int32_t iPayOutPrice;
};

struct LAST_HIRED_MERC_STRUCT {
  BOOLEAN fHaveDisplayedPopUpInLaptop;  // Is set when the popup gets displayed,
                                        // reset when entering laptop again.
  int32_t iIdOfMerc;
  uint32_t uiArrivalTime;
};

struct BobbyRayPurchaseStruct {
  uint16_t usItemIndex;
  uint8_t ubNumberPurchased;
  int8_t bItemQuality;
  uint16_t usBobbyItemIndex;  // Item number in the BobbyRayInventory structure
  BOOLEAN fUsed;              // Indicates wether or not the item is from the used inventory
                              // or the regular inventory
};

struct BobbyRayOrderStruct {
  BOOLEAN fActive;
  BobbyRayPurchaseStruct BobbyRayPurchase[MAX_PURCHASE_AMOUNT];
  uint8_t ubNumberPurchases;
};

// used when the player goes to bobby rays when it is still down
enum {
  BOBBYR_NEVER_BEEN_TO_SITE,
  BOBBYR_BEEN_TO_SITE_ONCE,
  BOBBYR_ALREADY_SENT_EMAIL,
};

struct LaptopSaveInfoStruct {
  // General Laptop Info
  BOOLEAN gfNewGameLaptop;              // Is it the firs time in Laptop
  BOOLEAN fVisitedBookmarkAlready[20];  // have we visitied this site already?
  int32_t iBookMarkList[MAX_BOOKMARKS];

  int32_t iCurrentBalance;  // current players balance

  // IMP Information
  BOOLEAN fIMPCompletedFlag;       // Has the player Completed the IMP process
  BOOLEAN fSentImpWarningAlready;  // Has the Imp email warning already been sent

  // Personnel Info
  int16_t ubDeadCharactersList[256];
  int16_t ubLeftCharactersList[256];
  int16_t ubOtherCharactersList[256];

  // MERC site info
  uint8_t gubPlayersMercAccountStatus;
  uint32_t guiPlayersMercAccountNumber;
  uint8_t gubLastMercIndex;

  // Aim Site

  // BobbyRay Site
  STORE_INVENTORY BobbyRayInventory[MAXITEMS];
  STORE_INVENTORY BobbyRayUsedInventory[MAXITEMS];

  BobbyRayOrderStruct *BobbyRayOrdersOnDeliveryArray;
  uint8_t usNumberOfBobbyRayOrderItems;  // The number of elements in the array
  uint8_t usNumberOfBobbyRayOrderUsed;   // The number of items in the array that
                                         // are used

  // Flower Site
  // NONE

  // Insurance Site
  LIFE_INSURANCE_PAYOUT *pLifeInsurancePayouts;
  uint8_t ubNumberLifeInsurancePayouts;     // The number of elements in the array
  uint8_t ubNumberLifeInsurancePayoutUsed;  // The number of items in the array
                                            // that are used

  BOOLEAN fBobbyRSiteCanBeAccessed;

  uint8_t ubPlayerBeenToMercSiteStatus;
  BOOLEAN fFirstVisitSinceServerWentDown;
  BOOLEAN fNewMercsAvailableAtMercSite;
  BOOLEAN fSaidGenericOpeningInMercSite;
  BOOLEAN fSpeckSaidFloMarriedCousinQuote;
  BOOLEAN fHasAMercDiedAtMercSite;

  uint16_t usInventoryListLength[BOBBY_RAY_LISTS];

  int32_t iVoiceId;

  uint8_t ubHaveBeenToBobbyRaysAtLeastOnceWhileUnderConstruction;

  BOOLEAN fMercSiteHasGoneDownYet;

  uint8_t ubSpeckCanSayPlayersLostQuote;

  LAST_HIRED_MERC_STRUCT sLastHiredMerc;

  int32_t iCurrentHistoryPage;
  int32_t iCurrentFinancesPage;
  int32_t iCurrentEmailPage;

  uint32_t uiSpeckQuoteFlags;

  uint32_t uiFlowerOrderNumber;

  uint32_t uiTotalMoneyPaidToSpeck;

  uint8_t ubLastMercAvailableId;
};

extern LaptopSaveInfoStruct LaptopSaveInfo;

extern BobbyRayPurchaseStruct BobbyRayPurchases[MAX_PURCHASE_AMOUNT];

void LoadLaptopInfoFromSavedGame(HWFILE);
void SaveLaptopInfoToSavedGame(HWFILE);

#endif
