#ifndef _DISPLAY_COVER__H_
#define _DISPLAY_COVER__H_

#include "JA2Types.h"

void DisplayCoverOfSelectedGridNo();
void RemoveCoverOfSelectedGridNo();

void DisplayRangeToTarget(const SOLDIERTYPE *pSoldier, INT16 sTargetGridNo);

void RemoveVisibleGridNoAtSelectedGridNo();
void DisplayGridNoVisibleToSoldierGrid();

void ChangeSizeOfDisplayCover(INT32 iNewSize);

void ChangeSizeOfLOS(INT32 iNewSize);

#endif
