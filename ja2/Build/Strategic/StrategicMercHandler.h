// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef _STRATEGIC_MERC_HANDLER_H_
#define _STRATEGIC_MERC_HANDLER_H_

#include "JA2Types.h"
#include "SGP/Types.h"

void StrategicHandlePlayerTeamMercDeath(SOLDIERTYPE &);
void MercDailyUpdate();
void MercsContractIsFinished(SOLDIERTYPE *s);
void RPCWhineAboutNoPay(SOLDIERTYPE &);
void MercComplainAboutEquipment(uint8_t ubProfileID);
BOOLEAN SoldierHasWorseEquipmentThanUsedTo(SOLDIERTYPE *pSoldier);
void UpdateBuddyAndHatedCounters();
void HourlyCamouflageUpdate();
#endif
