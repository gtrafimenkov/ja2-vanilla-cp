// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __FOV_H
#define __FOV_H

#include "JA2Types.h"

void RevealRoofsAndItems(SOLDIERTYPE *pSoldier, BOOLEAN fShowLocators);

void ClearSlantRoofs();
void AddSlantRoofFOVSlot(int16_t sGridNo);
void ExamineSlantRoofFOVSlots();

#endif
