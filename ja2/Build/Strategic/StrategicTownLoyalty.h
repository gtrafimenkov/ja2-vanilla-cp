#ifndef __STRATEGIC_TOWN_LOYALTY_H
#define __STRATEGIC_TOWN_LOYALTY_H

#include "JA2Types.h"

// gain pts per real loyalty pt
#define GAIN_PTS_PER_LOYALTY_PT 500

// --- LOYALTY BONUSES ---
// Omerta
#define LOYALTY_BONUS_MIGUEL_READS_LETTER \
  (10 * GAIN_PTS_PER_LOYALTY_PT)  // multiplied by 4.5 due to Omerta's high
                                  // seniment, so it's 45%
// Drassen
#define LOYALTY_BONUS_CHILDREN_FREED_DOREEN_KILLED \
  (10 * GAIN_PTS_PER_LOYALTY_PT)  // +50% bonus for Drassen
#define LOYALTY_BONUS_CHILDREN_FREED_DOREEN_SPARED \
  (20 * GAIN_PTS_PER_LOYALTY_PT)  // +50% bonus for Drassen
// Cambria
#define LOYALTY_BONUS_MARTHA_WHEN_JOEY_RESCUED \
  (15 * GAIN_PTS_PER_LOYALTY_PT)  // -25% for low Cambria sentiment
#define LOYALTY_BONUS_KEITH_WHEN_HILLBILLY_SOLVED \
  (15 * GAIN_PTS_PER_LOYALTY_PT)  // -25% for low Cambria sentiment
// Chitzena
#define LOYALTY_BONUS_YANNI_WHEN_CHALICE_RETURNED_LOCAL \
  (20 * GAIN_PTS_PER_LOYALTY_PT)  // +75% higher in Chitzena
#define LOYALTY_BONUS_YANNI_WHEN_CHALICE_RETURNED_GLOBAL \
  (10 * GAIN_PTS_PER_LOYALTY_PT)  // for ALL towns!
// Alma
#define LOYALTY_BONUS_AUNTIE_WHEN_BLOODCATS_KILLED \
  (20 * GAIN_PTS_PER_LOYALTY_PT)  // Alma's increases reduced by half due to low
                                  // rebel sentiment
#define LOYALTY_BONUS_MATT_WHEN_DYNAMO_FREED \
  (20 * GAIN_PTS_PER_LOYALTY_PT)  // Alma's increases reduced by half due to low
                                  // rebel sentiment
#define LOYALTY_BONUS_FOR_SERGEANT_KROTT \
  (20 * GAIN_PTS_PER_LOYALTY_PT)  // Alma's increases reduced by half due to low
                                  // rebel sentiment
// Everywhere
#define LOYALTY_BONUS_TERRORISTS_DEALT_WITH (5 * GAIN_PTS_PER_LOYALTY_PT)
#define LOYALTY_BONUS_KILL_QUEEN_MONSTER (10 * GAIN_PTS_PER_LOYALTY_PT)
// Anywhere
// loyalty bonus for completing town training
#define LOYALTY_BONUS_FOR_TOWN_TRAINING (2 * GAIN_PTS_PER_LOYALTY_PT)  // 2%

// --- LOYALTY PENALTIES ---
// Cambria
#define LOYALTY_PENALTY_MARTHA_HEART_ATTACK (20 * GAIN_PTS_PER_LOYALTY_PT)
#define LOYALTY_PENALTY_JOEY_KILLED (10 * GAIN_PTS_PER_LOYALTY_PT)
// Balime
#define LOYALTY_PENALTY_ELDIN_KILLED (20 * GAIN_PTS_PER_LOYALTY_PT)  // effect is double that!
// Any mine
#define LOYALTY_PENALTY_HEAD_MINER_ATTACKED \
  (20 * GAIN_PTS_PER_LOYALTY_PT)  // exact impact depends on rebel sentiment in
                                  // that town
// Loyalty penalty for being inactive, per day after the third
#define LOYALTY_PENALTY_INACTIVE (10 * GAIN_PTS_PER_LOYALTY_PT)

enum GlobalLoyaltyEventTypes {
  // There are only for distance-adjusted global loyalty effects.  Others go
  // into list above instead!
  GLOBAL_LOYALTY_BATTLE_WON,
  GLOBAL_LOYALTY_BATTLE_LOST,
  GLOBAL_LOYALTY_ENEMY_KILLED,
  GLOBAL_LOYALTY_NATIVE_KILLED,
  GLOBAL_LOYALTY_GAIN_TOWN_SECTOR,
  GLOBAL_LOYALTY_LOSE_TOWN_SECTOR,
  GLOBAL_LOYALTY_LIBERATE_WHOLE_TOWN,  // awarded only the first time it happens
  GLOBAL_LOYALTY_ABANDON_MILITIA,
  GLOBAL_LOYALTY_GAIN_MINE,
  GLOBAL_LOYALTY_LOSE_MINE,
  GLOBAL_LOYALTY_GAIN_SAM,
  GLOBAL_LOYALTY_LOSE_SAM,
  GLOBAL_LOYALTY_QUEEN_BATTLE_WON,
};

struct TOWN_LOYALTY {
  uint8_t ubRating;
  int16_t sChange;
  BOOLEAN fStarted;  // starting loyalty of each town is initialized only when
                     // player first enters that town
  BOOLEAN fLiberatedAlready;
};

// the loyalty variables for each town
extern TOWN_LOYALTY gTownLoyalty[];

// whether town maintains/displays loyalty or not
extern BOOLEAN gfTownUsesLoyalty[];

struct TownSectorInfo {
  uint8_t town;
  uint8_t sector;
};

extern TownSectorInfo g_town_sectors[];

#define FOR_EACH_TOWN_SECTOR(iter) \
  for (TownSectorInfo const *iter = g_town_sectors; iter->town != BLANK_SECTOR; ++iter)

#define FOR_EACH_SECTOR_IN_TOWN(iter, town_)                      \
  FOR_EACH_TOWN_SECTOR(iter) if (iter->town != (town_)) continue; \
  else

// initialize a specific town's loyalty if it hasn't already been
void StartTownLoyaltyIfFirstTime(int8_t bTownId);

// set a speciafied town's loyalty rating
void SetTownLoyalty(int8_t bTownId, uint8_t ubLoyaltyValue);

// increment the town loyalty rating (hundredths!)
void IncrementTownLoyalty(int8_t bTownId, uint32_t uiLoyaltyIncrease);

// decrement the town loyalty rating (hundredths!)
void DecrementTownLoyalty(int8_t bTownId, uint32_t uiLoyaltyDecrease);

// init town loyalty lists
void InitTownLoyalty();

// handle the death of a civ
void HandleMurderOfCivilian(const SOLDIERTYPE *pSoldier);

// handle town loyalty adjustment for recruitment
void HandleTownLoyaltyForNPCRecruitment(SOLDIERTYPE *pSoldier);

// remove random item from this sector
void RemoveRandomItemsInSector(int16_t sSectorX, int16_t sSectorY, int16_t sSectorZ,
                               uint8_t ubChance);

// build list of town sectors
void BuildListOfTownSectors();

void LoadStrategicTownLoyaltyFromSavedGameFile(HWFILE);
void SaveStrategicTownLoyaltyToSaveGameFile(HWFILE);

void ReduceLoyaltyForRebelsBetrayed();

// how many towns under player control?
int32_t GetNumberOfWholeTownsUnderControl();

// is all the sectors of this town under control by the player
int32_t IsTownUnderCompleteControlByPlayer(int8_t bTownId);

// used when monsters attack a town sector without going through tactical and
// they win
void AdjustLoyaltyForCivsEatenByMonsters(int16_t sSectorX, int16_t sSectorY, uint8_t ubHowMany);

// these are used to handle global loyalty events (ones that effect EVERY town
// on the map)
void IncrementTownLoyaltyEverywhere(uint32_t uiLoyaltyIncrease);
void DecrementTownLoyaltyEverywhere(uint32_t uiLoyaltyDecrease);
void HandleGlobalLoyaltyEvent(uint8_t ubEventType, int16_t sSectorX, int16_t sSectorY,
                              int8_t bSectorZ);

// handle a town being liberated for the first time
void CheckIfEntireTownHasBeenLiberated(int8_t bTownId, int16_t sSectorX, int16_t sSectorY);
void CheckIfEntireTownHasBeenLost(int8_t bTownId, int16_t sSectorX, int16_t sSectorY);

void HandleLoyaltyChangeForNPCAction(uint8_t ubNPCProfileId);

bool DidFirstBattleTakePlaceInThisTown(int8_t town);
void SetTheFirstBattleSector(int16_t sSectorValue);

// gte number of whole towns but exclude this one
int32_t GetNumberOfWholeTownsUnderControlButExcludeCity(int8_t bCityToExclude);

// Function assumes that mercs have retreated already.  Handles two cases, one
// for general merc retreat which slightly demoralizes the mercs, the other
// handles abandonment of militia forces which poses as a serious loyalty
// penalty.

#define RETREAT_TACTICAL_TRAVERSAL 0
#define RETREAT_PBI 1
#define RETREAT_AUTORESOLVE 2
void HandleLoyaltyImplicationsOfMercRetreat(int8_t bRetreatCode, int16_t sSectorX, int16_t sSectorY,
                                            int16_t sSectorZ);

void MaximizeLoyaltyForDeidrannaKilled();

#endif
