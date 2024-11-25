// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef _DISPLAY_COVER__H_
#define _DISPLAY_COVER__H_

#include "JA2Types.h"

void DisplayCoverOfSelectedGridNo();
void RemoveCoverOfSelectedGridNo();

void DisplayRangeToTarget(const SOLDIERTYPE *pSoldier, int16_t sTargetGridNo);

void RemoveVisibleGridNoAtSelectedGridNo();
void DisplayGridNoVisibleToSoldierGrid();

void ChangeSizeOfDisplayCover(int32_t iNewSize);

void ChangeSizeOfLOS(int32_t iNewSize);

#endif
