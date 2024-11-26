#ifndef __ENDGAME_H
#define __ENDGAME_H

#include "JA2Types.h"
#include "SGP/Types.h"

BOOLEAN DoesO3SectorStatueExistHere(INT16 sGridNo);
void ChangeO3SectorStatue(BOOLEAN fFromExplosion);

void BeginHandleDeidrannaDeath(SOLDIERTYPE *pKillerSoldier, INT16 sGridNo, INT8 bLevel);

void EndQueenDeathEndgameBeginEndCimenatic();
void EndQueenDeathEndgame();

void BeginHandleQueenBitchDeath(SOLDIERTYPE *pKillerSoldier, INT16 sGridNo, INT8 bLevel);

#endif
