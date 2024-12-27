// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef _STRATEGIC_EVENT_HANDLER_H_
#define _STRATEGIC_EVENT_HANDLER_H_

#include "SGP/Types.h"

#define KINGPIN_MONEY_SECTOR_X 5
#define KINGPIN_MONEY_SECTOR_Y MAP_ROW_D
#define KINGPIN_MONEY_SECTOR_Z 1

#define HOSPITAL_SECTOR_X 8
#define HOSPITAL_SECTOR_Y MAP_ROW_F
#define HOSPITAL_SECTOR_Z 0

extern uint8_t gubCambriaMedicalObjects;

void CheckForKingpinsMoneyMissing(BOOLEAN fFirstCheck);
void CheckForMissingHospitalSupplies();

void BobbyRayPurchaseEventCallback(uint8_t ubOrderID);

void HandleStolenItemsReturned();

void AddSecondAirportAttendant();

void HandleNPCSystemEvent(uint32_t uiEvent);
void HandleEarlyMorningEvents();

void MakeCivGroupHostileOnNextSectorEntrance(uint8_t ubCivGroup);

void RemoveAssassin(uint8_t ubProfile);

#endif
