// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __AIMSORT_H_
#define __AIMSORT_H_

#include <cstdint>

extern uint8_t gubCurrentSortMode;
extern uint8_t gubCurrentListMode;

#define AIM_ASCEND 6
#define AIM_DESCEND 7

void GameInitAimSort();
void EnterAimSort();
void ExitAimSort();
void RenderAimSort();

#endif
