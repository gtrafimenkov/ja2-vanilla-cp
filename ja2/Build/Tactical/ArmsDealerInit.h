// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef _ARMS_DEALERS_INIT__H_
#define _ARMS_DEALERS_INIT__H_

#include "JA2Types.h"
#include "Tactical/ArmsDealer.h"
#include "Tactical/ItemTypes.h"

// the enums for the different kinds of arms dealers
enum {
  ARMS_DEALER_BUYS_SELLS,
  ARMS_DEALER_SELLS_ONLY,
  ARMS_DEALER_BUYS_ONLY,
  ARMS_DEALER_REPAIRS,
};

// The following defines indicate what items can be sold by the arms dealer
#define ARMS_DEALER_HANDGUNCLASS 0x00000001
#define ARMS_DEALER_SMGCLASS 0x00000002
#define ARMS_DEALER_RIFLECLASS 0x00000004
#define ARMS_DEALER_MGCLASS 0x00000008
#define ARMS_DEALER_SHOTGUNCLASS 0x00000010

#define ARMS_DEALER_KNIFECLASS 0x00000020

#define ARMS_DEALER_BLADE 0x00000040
#define ARMS_DEALER_LAUNCHER 0x00000080

#define ARMS_DEALER_ARMOUR 0x00000100
#define ARMS_DEALER_MEDKIT 0x00000200
#define ARMS_DEALER_MISC 0x00000400
#define ARMS_DEALER_AMMO 0x00000800

#define ARMS_DEALER_GRENADE 0x00001000
#define ARMS_DEALER_BOMB 0x00002000
#define ARMS_DEALER_EXPLOSV 0x00004000

#define ARMS_DEALER_KIT 0x00008000

#define ARMS_DEALER_FACE 0x00010000
// #define		ARMS_DEALER_THROWN
//  0x00020000 #define		ARMS_DEALER_KEY
//  0x00040000

// #define		ARMS_DEALER_VIDEO_CAMERA 0x00020000

#define ARMS_DEALER_DETONATORS 0x00040000

#define ARMS_DEALER_ATTACHMENTS 0x00080000

#define ARMS_DEALER_ALCOHOL 0x00100000
#define ARMS_DEALER_ELECTRONICS 0x00200000
#define ARMS_DEALER_HARDWARE 0x00400000 | ARMS_DEALER_KIT

#define ARMS_DEALER_MEDICAL 0x00800000 | ARMS_DEALER_MEDKIT

// #define		ARMS_DEALER_EMPTY_JAR
//  0x01000000
#define ARMS_DEALER_CREATURE_PARTS 0x02000000
#define ARMS_DEALER_ROCKET_RIFLE 0x04000000

#define ARMS_DEALER_ONLY_USED_ITEMS 0x08000000
#define ARMS_DEALER_GIVES_CHANGE \
  0x10000000  // The arms dealer will give the required change when doing a
              // transaction
#define ARMS_DEALER_ACCEPTS_GIFTS \
  0x20000000  // The arms dealer is the kind of person who will accept gifts
#define ARMS_DEALER_SOME_USED_ITEMS \
  0x40000000  // The arms dealer can have used items in his inventory
#define ARMS_DEALER_HAS_NO_INVENTORY 0x80000000  // The arms dealer does not carry any inventory

#define ARMS_DEALER_ALL_GUNS                                                                       \
  ARMS_DEALER_HANDGUNCLASS | ARMS_DEALER_SMGCLASS | ARMS_DEALER_RIFLECLASS | ARMS_DEALER_MGCLASS | \
      ARMS_DEALER_SHOTGUNCLASS

#define ARMS_DEALER_BIG_GUNS \
  ARMS_DEALER_SMGCLASS | ARMS_DEALER_RIFLECLASS | ARMS_DEALER_MGCLASS | ARMS_DEALER_SHOTGUNCLASS

#define ARMS_DEALER_ALL_WEAPONS \
  ARMS_DEALER_ALL_GUNS | ARMS_DEALER_BLADE | ARMS_DEALER_LAUNCHER | ARMS_DEALER_KNIFECLASS

//
// Specific Dealer Flags
// NOTE: Each dealer has 8 flags, but different dealers can and SHOULD share the
// same flag #s!
//

// Alex Fredo
#define ARMS_DEALER_FLAG__FREDO_HAS_SAID_ROCKET_RIFLE_QUOTE \
  0x00000001  // Alex Fredo has already repaired the Rocket Rifle
// Franz Hinkle
#define ARMS_DEALER_FLAG__FRANZ_HAS_SOLD_VIDEO_CAMERA_TO_PLAYER \
  0x00000001  // Franz Hinkle has sold the video camera to the player

// THIS STRUCTURE HAS UNCHANGING INFO THAT DOESN'T GET SAVED/RESTORED/RESET
struct ARMS_DEALER_INFO {
  union {
    struct {
      float buy;   // The price modifier used when this dealer is BUYING something.
      float sell;  // The price modifier used when this dealer is SELLING
                   // something.
    } price;
    struct {
      float speed;  // Modifier to the speed at which a repairman repairs things
      float cost;   // Modifier to the price a repairman charges for repairs
    } repair;
  } u;

  uint8_t ubShopKeeperID;      // Merc Id for the dealer
  uint8_t ubTypeOfArmsDealer;  // Whether he buys/sells, sells, buys, or repairs
  int32_t iInitialCash;        // How much cash dealer starts with (we now reset to this
                               // amount once / day)
  uint32_t uiFlags;            // various flags which control the dealer's operations
};

// THIS STRUCTURE GETS SAVED/RESTORED/RESET
struct ARMS_DEALER_STATUS {
  uint32_t uiArmsDealersCash;  // How much money the arms dealer currently has

  uint8_t ubSpecificDealerFlags;  // Misc state flags for specific dealers
  BOOLEAN fOutOfBusiness;         // Set when a dealer has been killed, etc.
  BOOLEAN fRepairDelayBeenUsed;   // Set when a repairman has missed his repair
                                  // time estimate & given his excuse for it
  BOOLEAN fUnusedKnowsPlayer;     // Set if the shopkeeper has met with the player
                                  // before [UNUSED] // XXX HACK000B

  uint32_t uiTimePlayerLastInSKI;  // game time (in total world minutes) when
                                   // player last talked to this dealer in SKI

  uint8_t ubPadding[8];  // XXX HACK000B
};

struct SPECIAL_ITEM_INFO {
  uint16_t usAttachment[MAX_ATTACHMENTS];  // item index of any attachments on the
                                           // item

  int8_t bItemCondition;  // if 0, no item is stored
                          // from 1 to 100 indicates an item with that status
                          // -1 to -100 means the item is in for repairs, flip sign
                          // for the actual status

  uint8_t ubImprintID;  // imprint ID for imprinted items (during repair!)

  int8_t bAttachmentStatus[MAX_ATTACHMENTS];  // status of any attachments on the
                                              // item

  uint8_t ubPadding[2];  // filler // XXX HACK000B
};

struct DEALER_SPECIAL_ITEM {
  // Individual "special" items are stored here as needed, *one* per slot
  // An item is special if it is used (status < 100), has been imprinted, or has
  // a permanent attachment

  SPECIAL_ITEM_INFO Info;

  uint32_t uiRepairDoneTime;  // If the item is in for repairs, this holds the time
                              // when it will be repaired (in min)

  BOOLEAN fActive;  // TRUE means an item is stored here (empty elements may not
                    // always be freed immediately)

  uint8_t ubOwnerProfileId;  // stores which merc previously owned an item being
                             // repaired

  uint8_t ubPadding[6];  // filler // XXX HACK000B
};

struct DEALER_ITEM_HEADER {
  // Non-special items are all the identical and are totaled inside
  // ubPerfectItems. Items being repaired are also stored here, with a negative
  // condition. NOTE: special item elements may remain allocated long after item
  // has been removed, to reduce memory fragmentation!!!

  uint8_t ubTotalItems;    // sum of all the items (all perfect ones + all special
                           // ones)
  uint8_t ubPerfectItems;  // non-special (perfect) items held by dealer
  uint8_t ubStrayAmmo;     // partially-depleted ammo mags are stored here as
                           // #bullets, and can be converted to full packs

  uint8_t ubElementsAlloced;  // number of DEALER_SPECIAL_ITEM array elements
                              // alloced for the special item array
  DEALER_SPECIAL_ITEM
  *SpecialItem;  // dynamic array of special items with this same item index

  uint32_t uiOrderArrivalTime;  // Day the items ordered will arrive on.  It's
                                // uint32_t in case we change this to minutes.
  uint8_t ubQtyOnOrder;         // The number of items currently on order
  BOOLEAN fPreviouslyEligible;  // whether or not dealer has been eligible to
                                // sell this item in days prior to today
};

extern const ARMS_DEALER_INFO ArmsDealerInfo[NUM_ARMS_DEALERS];
extern ARMS_DEALER_STATUS gArmsDealerStatus[NUM_ARMS_DEALERS];
extern DEALER_ITEM_HEADER gArmsDealersInventory[NUM_ARMS_DEALERS][MAXITEMS];

void InitAllArmsDealers();
void ShutDownArmsDealers();

// Count only the # of "distinct" item types (for shopkeeper purposes)
uint32_t CountDistinctItemsInArmsDealersInventory(ArmsDealerID);
uint16_t CountTotalItemsRepairDealerHasInForRepairs(ArmsDealerID);

void AddObjectToArmsDealerInventory(ArmsDealerID, OBJECTTYPE *);

void RemoveItemFromArmsDealerInventory(ArmsDealerID, uint16_t usItemIndex,
                                       SPECIAL_ITEM_INFO *pSpclItemInfo, uint8_t ubHowMany);
void RemoveSpecialItemFromArmsDealerInventoryAtElement(ArmsDealerID, uint16_t usItemIndex,
                                                       uint8_t ubElement);

BOOLEAN IsMercADealer(uint8_t ubMercID);
ArmsDealerID GetArmsDealerIDFromMercID(uint8_t ubMercID);

void SaveArmsDealerInventoryToSaveGameFile(HWFILE);
void LoadArmsDealerInventoryFromSavedGameFile(HWFILE, uint32_t savegame_version);

void DailyUpdateOfArmsDealersInventory();

uint8_t GetTypeOfArmsDealer(uint8_t ubDealerID);

BOOLEAN DoesDealerDoRepairs(ArmsDealerID);
BOOLEAN RepairmanIsFixingItemsButNoneAreDoneYet(uint8_t ubProfileID);

BOOLEAN CanDealerTransactItem(ArmsDealerID, uint16_t usItemIndex, BOOLEAN fPurchaseFromPlayer);
BOOLEAN CanDealerRepairItem(ArmsDealerID, uint16_t usItemIndex);

BOOLEAN AddDeadArmsDealerItemsToWorld(SOLDIERTYPE const *);

void MakeObjectOutOfDealerItems(uint16_t usItemIndex, SPECIAL_ITEM_INFO *pSpclItemInfo,
                                OBJECTTYPE *pObject, uint8_t ubHowMany);

void GiveObjectToArmsDealerForRepair(ArmsDealerID, OBJECTTYPE const *pObject,
                                     uint8_t ubOwnerProfileId);

uint32_t CalculateObjectItemRepairTime(ArmsDealerID, OBJECTTYPE const *pItemObject);

uint32_t CalculateObjectItemRepairCost(ArmsDealerID, OBJECTTYPE const *pItemObject);

void SetSpecialItemInfoToDefaults(SPECIAL_ITEM_INFO *pSpclItemInfo);
void SetSpecialItemInfoFromObject(SPECIAL_ITEM_INFO *pSpclItemInfo, const OBJECTTYPE *pObject);

uint16_t CalcValueOfItemToDealer(ArmsDealerID, uint16_t usItemIndex, BOOLEAN fDealerSelling);

BOOLEAN DealerItemIsSafeToStack(uint16_t usItemIndex);

uint32_t CalculateOvernightRepairDelay(ArmsDealerID, uint32_t uiTimeWhenFreeToStartIt,
                                       uint32_t uiMinutesToFix);
uint32_t CalculateMinutesClosedBetween(ArmsDealerID, uint32_t uiStartTime, uint32_t uiEndTime);

void GuaranteeAtLeastXItemsOfIndex(ArmsDealerID, uint16_t usItemIndex, uint8_t ubHowMany);
BOOLEAN ItemIsARocketRifle(int16_t sItemIndex);

extern uint8_t gubLastSpecialItemAddedAtElement;

#endif
