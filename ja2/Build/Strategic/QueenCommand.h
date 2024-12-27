// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __QUEEN_COMMAND_H
#define __QUEEN_COMMAND_H

#include "JA2Types.h"
#include "Strategic/CampaignTypes.h"
#include "Strategic/StrategicMovement.h"

extern BOOLEAN gfPendingEnemies;

// Counts enemies and crepitus, but not bloodcats.
uint8_t NumHostilesInSector(int16_t sSectorX, int16_t sSectorY, int16_t sSectorZ);

uint8_t NumEnemiesInAnySector(int16_t sSectorX, int16_t sSectorY, int16_t sSectorZ);

uint8_t NumEnemiesInSector(int16_t sSectorX, int16_t sSectorY);
uint8_t NumStationaryEnemiesInSector(int16_t sSectorX, int16_t sSectorY);
uint8_t NumMobileEnemiesInSector(int16_t sSectorX, int16_t sSectorY);
void GetNumberOfEnemiesInSector(int16_t sSectorX, int16_t sSectorY, uint8_t *pubNumAdmins,
                                uint8_t *pubNumTroops, uint8_t *pubNumElites);

/* Called when entering a sector so the campaign AI can automatically insert the
 * correct number of troops of each type based on the current number in the
 * sector in global focus (gWorldSectorX/Y) */
void PrepareEnemyForSectorBattle();

void AddPossiblePendingEnemiesToBattle();
void EndTacticalBattleForEnemy();

void ProcessQueenCmdImplicationsOfDeath(const SOLDIERTYPE *);

void SaveUnderGroundSectorInfoToSaveGame(HWFILE);
void LoadUnderGroundSectorInfoFromSavedGame(HWFILE);

// Finds and returns the specified underground structure ( DONT MODIFY IT ).
// Else returns NULL
UNDERGROUND_SECTORINFO *FindUnderGroundSector(int16_t sMapX, int16_t sMapY, uint8_t bMapZ);

void EnemyCapturesPlayerSoldier(SOLDIERTYPE *pSoldier);
void BeginCaptureSquence();
void EndCaptureSequence();

BOOLEAN PlayerSectorDefended(uint8_t ubSectorID);

BOOLEAN OnlyHostileCivsInSector();

extern int16_t gsInterrogationGridNo[3];

#endif
