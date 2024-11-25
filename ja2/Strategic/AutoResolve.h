// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __AUTO_RESOLVE_H
#define __AUTO_RESOLVE_H

#include "JA2Types.h"
#include "ScreenIDs.h"
#include "Tactical/ItemTypes.h"

void EnterAutoResolveMode(uint8_t ubSectorX, uint8_t ubSectorY);

// is the autoresolve active?
BOOLEAN IsAutoResolveActive();

void EliminateAllEnemies(uint8_t ubSectorX, uint8_t ubSectorY);

void ConvertTacticalBattleIntoStrategicAutoResolveBattle();

uint8_t GetAutoResolveSectorID();

extern BOOLEAN gfTransferTacticalOppositionToAutoResolve;

// Returns TRUE if autoresolve is active or a sector is loaded.
BOOLEAN GetCurrentBattleSectorXYZ(int16_t *psSectorX, int16_t *psSectorY, int16_t *psSectorZ);

uint32_t VirtualSoldierDressWound(SOLDIERTYPE *pSoldier, SOLDIERTYPE *pVictim, OBJECTTYPE *pKit,
                                  int16_t sKitPts, int16_t sStatus);

// Returns TRUE if a battle is happening ONLY
BOOLEAN GetCurrentBattleSectorXYZAndReturnTRUEIfThereIsABattle(int16_t *psSectorX,
                                                               int16_t *psSectorY,
                                                               int16_t *psSectorZ);

ScreenID AutoResolveScreenHandle();

#endif
