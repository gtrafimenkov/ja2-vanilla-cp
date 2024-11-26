#ifndef __FOV_H
#define __FOV_H

#include "JA2Types.h"

void RevealRoofsAndItems(SOLDIERTYPE *pSoldier, BOOLEAN fShowLocators);

void ClearSlantRoofs();
void AddSlantRoofFOVSlot(int16_t sGridNo);
void ExamineSlantRoofFOVSlots();

#endif
