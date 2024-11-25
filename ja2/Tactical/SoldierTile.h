// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __SOLDIER_TILE_H
#define __SOLDIER_TILE_H

#include "JA2Types.h"

#define MOVE_TILE_CLEAR 1
#define MOVE_TILE_TEMP_BLOCKED -1
#define MOVE_TILE_STATIONARY_BLOCKED -2

void UnMarkMovementReserved(SOLDIERTYPE &);

BOOLEAN HandleNextTile(SOLDIERTYPE *pSoldier, int8_t bDirection, int16_t sGridNo,
                       int16_t sFinalDestTile);

void HandleNextTileWaiting(SOLDIERTYPE *pSoldier);

bool TeleportSoldier(SOLDIERTYPE &, GridNo, bool force);

void SwapMercPositions(SOLDIERTYPE &s1, SOLDIERTYPE &s2);

void SetDelayedTileWaiting(SOLDIERTYPE *pSoldier, int16_t sCauseGridNo, int8_t bValue);

BOOLEAN CanExchangePlaces(SOLDIERTYPE *pSoldier1, SOLDIERTYPE *pSoldier2, BOOLEAN fShow);

#endif
