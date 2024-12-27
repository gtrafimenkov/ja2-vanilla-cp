// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __STRATEGIC_MINES_H
#define __STRATEGIC_MINES_H

// the .h to the mine management system

#include "SGP/Types.h"

// the mines
enum MineID {
  MINE_SAN_MONA = 0,
  MINE_DRASSEN,
  MINE_ALMA,
  MINE_CAMBRIA,
  MINE_CHITZENA,
  MINE_GRUMM,
  MAX_NUMBER_OF_MINES,
};

enum {
  MINER_FRED = 0,
  MINER_MATT,
  MINER_OSWALD,
  MINER_CALVIN,
  MINER_CARL,
  NUM_HEAD_MINERS,
};

// different types of mines
enum {
  SILVER_MINE = 0,
  GOLD_MINE,
  NUM_MINE_TYPES,
};

// head miner quote types
enum HeadMinerQuote {
  HEAD_MINER_STRATEGIC_QUOTE_RUNNING_OUT = 0,
  HEAD_MINER_STRATEGIC_QUOTE_CREATURES_ATTACK,
  HEAD_MINER_STRATEGIC_QUOTE_CREATURES_GONE,
  HEAD_MINER_STRATEGIC_QUOTE_CREATURES_AGAIN,
  NUM_HEAD_MINER_STRATEGIC_QUOTES
};

// the strategic mine structures
struct MINE_LOCATION_TYPE {
  uint8_t sector;          // Sector the mine is in
  int8_t bAssociatedTown;  // Associated town of this mine
};

struct MINE_STATUS_TYPE {
  uint8_t ubMineType;         // type of mine (silver or gold)
  uint32_t uiMaxRemovalRate;  // fastest rate we can move ore from this mine in period

  uint32_t uiRemainingOreSupply;  // the total value left to this mine (-1 means
                                  // unlimited)
  uint32_t uiOreRunningOutPoint;  // when supply drop below this, workers tell
                                  // player the mine is running out of ore

  BOOLEAN fEmpty;                     // whether no longer minable
  BOOLEAN fRunningOut;                // whether mine is beginning to run out
  BOOLEAN fWarnedOfRunningOut;        // whether mine foreman has already told player
                                      // the mine's running out
  BOOLEAN fShutDownIsPermanent;       // means will never produce again in the game
                                      // (head miner was attacked & died/quit)
  BOOLEAN fShutDown;                  // TRUE means mine production has been shut off
  BOOLEAN fPrevInvadedByMonsters;     // whether or not mine has been previously
                                      // invaded by monsters
  BOOLEAN fSpokeToHeadMiner;          // player doesn't receive income from mine without
                                      // speaking to the head miner first
  BOOLEAN fMineHasProducedForPlayer;  // player has earned income from this mine
                                      // at least once

  BOOLEAN fQueenRetookProducingMine;       // whether or not queen ever retook a mine
                                           // after a player had produced from it
  BOOLEAN fAttackedHeadMiner;              // player has attacked the head miner, shutting
                                           // down mine & decreasing loyalty
  uint32_t uiTimePlayerProductionStarted;  // time in minutes when
                                           // 'fMineHasProducedForPlayer' was first
                                           // set
};

// init mines
void InitializeMines();

void HourlyMinesUpdate();

// get total left in this mine
int32_t GetTotalLeftInMine(int8_t bMineIndex);

// get max rates for this mine (per period, per day)
uint32_t GetMaxPeriodicRemovalFromMine(int8_t bMineIndex);

// Get the max amount that can be mined in one day.
uint32_t GetMaxDailyRemovalFromMine(int8_t mine_id);

// which town does this mine belong to?
int8_t GetTownAssociatedWithMine(int8_t bMineIndex);

// posts the actual mine production events daily
void PostEventsForMineProduction();

// the periodic checking for income from mines
void HandleIncomeFromMines();

// predict income from mines
int32_t PredictIncomeFromPlayerMines();

// predict income from a mine
uint32_t PredictDailyIncomeFromAMine(int8_t mine_id);

/* Calculate how much player could make daily if he owned all mines with 100%
 * control and 100% loyalty. */
int32_t CalcMaxPlayerIncomeFromMines();

// get index of this mine, return -1 if no mine found
int8_t GetMineIndexForSector(uint8_t sector);

// get the sector value for the mine associated with this town
int16_t GetMineSectorForTown(int8_t town_id);

// Save the mine status to the save game file
void SaveMineStatusToSaveGameFile(HWFILE);

// Load the mine status from the saved game file
void LoadMineStatusFromSavedGameFile(HWFILE);

// if the player controls a given mine
bool PlayerControlsMine(int8_t mine_id);

void ShutOffMineProduction(int8_t bMineIndex);
void RestartMineProduction(int8_t bMineIndex);

BOOLEAN IsMineShutDown(int8_t bMineIndex);

// Find the sector location of a mine
uint8_t GetMineSector(uint8_t ubMineIndex);

void IssueHeadMinerQuote(int8_t bMineIndex, HeadMinerQuote);

uint8_t GetHeadMinersMineIndex(uint8_t ubMinerProfileId);

void PlayerSpokeToHeadMiner(uint8_t ubMinerProfile);

BOOLEAN IsHisMineRunningOut(uint8_t ubMinerProfileId);
BOOLEAN IsHisMineEmpty(uint8_t ubMinerProfileId);
BOOLEAN IsHisMineDisloyal(uint8_t ubMinerProfileId);
BOOLEAN IsHisMineInfested(uint8_t ubMinerProfileId);
BOOLEAN IsHisMineLostAndRegained(uint8_t ubMinerProfileId);
BOOLEAN IsHisMineAtMaxProduction(uint8_t ubMinerProfileId);
void ResetQueenRetookMine(uint8_t ubMinerProfileId);

void QueenHasRegainedMineSector(int8_t bMineIndex);

BOOLEAN HasAnyMineBeenAttackedByMonsters();

void PlayerAttackedHeadMiner(uint8_t ubMinerProfileId);

BOOLEAN HasHisMineBeenProducingForPlayerForSomeTime(uint8_t ubMinerProfileId);

// Get the id of the mine for this sector x,y,z; -1 is invalid
int8_t GetIdOfMineForSector(int16_t x, int16_t y, int8_t z);

// use this to determine whether or not to place miners into a underground mine
// level
BOOLEAN AreThereMinersInsideThisMine(uint8_t ubMineIndex);

// use this to determine whether or not the player has spoken to a head miner
BOOLEAN SpokenToHeadMiner(uint8_t ubMineIndex);

extern MINE_LOCATION_TYPE const gMineLocation[MAX_NUMBER_OF_MINES];
extern MINE_STATUS_TYPE gMineStatus[MAX_NUMBER_OF_MINES];

#endif
