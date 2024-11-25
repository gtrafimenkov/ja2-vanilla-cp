// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __ENDGAME_H
#define __ENDGAME_H

#include "JA2Types.h"
#include "SGP/Types.h"

BOOLEAN DoesO3SectorStatueExistHere(int16_t sGridNo);
void ChangeO3SectorStatue(BOOLEAN fFromExplosion);

void BeginHandleDeidrannaDeath(SOLDIERTYPE *pKillerSoldier, int16_t sGridNo, int8_t bLevel);

void EndQueenDeathEndgameBeginEndCimenatic();
void EndQueenDeathEndgame();

void BeginHandleQueenBitchDeath(SOLDIERTYPE *pKillerSoldier, int16_t sGridNo, int8_t bLevel);

#endif
