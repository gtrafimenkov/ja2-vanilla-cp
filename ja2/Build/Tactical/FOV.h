#ifndef __FOV_H
#define __FOV_H

#include "JA2Types.h"

void RevealRoofsAndItems(SOLDIERTYPE *pSoldier, BOOLEAN fShowLocators);

void ClearSlantRoofs();
void AddSlantRoofFOVSlot(INT16 sGridNo);
void ExamineSlantRoofFOVSlots();

#endif
