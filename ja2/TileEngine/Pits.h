// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __PITS_H
#define __PITS_H

#include "JA2Types.h"

void Add3X3Pit(int32_t iMapIndex);
void Add5X5Pit(int32_t iMapIndex);
void Remove3X3Pit(int32_t iMapIndex);
void Remove5X5Pit(int32_t iMapIndex);

void SearchForOtherMembersWithinPitRadiusAndMakeThemFall(int16_t sGridNo, int16_t sRadius);

void AddAllPits();
void RemoveAllPits();

extern BOOLEAN gfShowPits;

void HandleFallIntoPitFromAnimation(SOLDIERTYPE &);

#endif
