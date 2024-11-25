// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __BOBBYRMAILORDER_H
#define __BOBBYRMAILORDER_H

#include "Laptop/LaptopSave.h"

// enums for the various destinations that are available in the bobbyR dest drop
// down box
enum {
  BR_AUSTIN,
  BR_BAGHDAD,
  BR_DRASSEN,
  BR_HONG_KONG,
  BR_BEIRUT,
  BR_LONDON,
  BR_LOS_ANGELES,
  BR_MEDUNA,
  BR_METAVIRA,
  BR_MIAMI,
  BR_MOSCOW,
  BR_NEW_YORK,
  BR_OTTAWA,
  BR_PARIS,
  BR_TRIPOLI,
  BR_TOKYO,
  BR_VANCOUVER,
};

void GameInitBobbyRMailOrder();
void EnterBobbyRMailOrder();
void ExitBobbyRMailOrder();
void HandleBobbyRMailOrder();
void RenderBobbyRMailOrder();

void BobbyRayMailOrderEndGameShutDown();
void EnterInitBobbyRayOrder();
void AddJohnsGunShipment();

void CreateBobbyRayOrderTitle();
void DestroyBobbyROrderTitle();
void DrawBobbyROrderTitle();

void DisplayPurchasedItems(BOOLEAN fCalledFromOrderPage, uint16_t usGridX, uint16_t usGridY,
                           BobbyRayPurchaseStruct *pBobbyRayPurchase, BOOLEAN fJustDisplayTitles,
                           int32_t iOrderNum);

struct NewBobbyRayOrderStruct {
  BOOLEAN fActive;
  uint8_t ubDeliveryLoc;     // the city the shipment is going to
  uint8_t ubDeliveryMethod;  // type of delivery: next day, 2 days ...
  BobbyRayPurchaseStruct BobbyRayPurchase[MAX_PURCHASE_AMOUNT];
  uint8_t ubNumberPurchases;

  uint32_t uiPackageWeight;
  uint32_t uiOrderedOnDayNum;

  BOOLEAN fDisplayedInShipmentPage;

  uint8_t ubFiller[7];  // XXX HACK000B
};

extern NewBobbyRayOrderStruct *gpNewBobbyrShipments;
extern int32_t giNumberOfNewBobbyRShipment;

uint16_t CountNumberOfBobbyPurchasesThatAreInTransit();

void NewWayOfLoadingBobbyRMailOrdersToSaveGameFile(HWFILE);
void NewWayOfSavingBobbyRMailOrdersToSaveGameFile(HWFILE);

#endif
