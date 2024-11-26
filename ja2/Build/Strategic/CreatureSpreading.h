#ifndef __CREATURE_SPREADING_H
#define __CREATURE_SPREADING_H

#include "JA2Types.h"
#include "SGP/Types.h"

void InitCreatureQuest();
void SpreadCreatures();
void ClearCreatureQuest();
void DeleteCreatureDirectives();

void SaveCreatureDirectives(HWFILE);
void LoadCreatureDirectives(HWFILE, uint32_t uiSavedGameVersion);

BOOLEAN PrepareCreaturesForBattle();
void CreatureNightPlanning();
void CreatureAttackTown(uint8_t ubSectorID, BOOLEAN fOverrideTest);

void CheckConditionsForTriggeringCreatureQuest(int16_t sSectorX, int16_t sSectorY, int8_t bSectorZ);

extern BOOLEAN gfUseCreatureMusic;

BOOLEAN MineClearOfMonsters(uint8_t ubMineIndex);

// Returns true if valid and creature quest over, false if creature quest active
// or not yet started
bool GetWarpOutOfMineCodes(int16_t *sector_x, int16_t *sector_y, int8_t *sector_z,
                           GridNo *insertion_grid_no);

extern int16_t gsCreatureInsertionCode;
extern int16_t gsCreatureInsertionGridNo;
extern uint8_t gubNumCreaturesAttackingTown;
extern uint8_t gubYoungMalesAttackingTown;
extern uint8_t gubYoungFemalesAttackingTown;
extern uint8_t gubAdultMalesAttackingTown;
extern uint8_t gubAdultFemalesAttackingTown;
extern uint8_t gubSectorIDOfCreatureAttack;
enum {
  CREATURE_BATTLE_CODE_NONE,
  CREATURE_BATTLE_CODE_TACTICALLYADD,
  CREATURE_BATTLE_CODE_TACTICALLYADD_WITHFOV,
  CREATURE_BATTLE_CODE_PREBATTLEINTERFACE,
  CREATURE_BATTLE_CODE_AUTORESOLVE,
};
extern uint8_t gubCreatureBattleCode;

void DetermineCreatureTownComposition(uint8_t ubNumCreatures, uint8_t *pubNumYoungMales,
                                      uint8_t *pubNumYoungFemales, uint8_t *pubNumAdultMales,
                                      uint8_t *pubNumAdultFemales);

void DetermineCreatureTownCompositionBasedOnTacticalInformation(uint8_t *pubNumCreatures,
                                                                uint8_t *pubNumYoungMales,
                                                                uint8_t *pubNumYoungFemales,
                                                                uint8_t *pubNumAdultMales,
                                                                uint8_t *pubNumAdultFemales);

BOOLEAN PlayerGroupIsInACreatureInfestedMine();

void EndCreatureQuest();

#endif
