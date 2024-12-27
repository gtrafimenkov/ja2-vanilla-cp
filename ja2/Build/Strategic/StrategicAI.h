// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __STRATEGIC_AI_H
#define __STRATEGIC_AI_H

#include "Strategic/StrategicMovement.h"

void InitStrategicAI();
void KillStrategicAI();

void SaveStrategicAI(HWFILE);
void LoadStrategicAI(HWFILE);

// NPC ACTION TRIGGERS SPECIAL CASE AI
enum {
  STRATEGIC_AI_ACTION_WAKE_QUEEN = 1,
  STRATEGIC_AI_ACTION_KINGPIN_DEAD,
  STRATEGIC_AI_ACTION_QUEEN_DEAD,

};

void ExecuteStrategicAIAction(uint16_t usActionCode, int16_t sSectorX, int16_t sSectorY);

void CheckEnemyControlledSector(uint8_t ubSectorID);
void EvaluateQueenSituation();

extern BOOLEAN gfUseAlternateQueenPosition;

// returns TRUE if the group was deleted.
BOOLEAN StrategicAILookForAdjacentGroups(GROUP *pGroup);
void RemoveGroupFromStrategicAILists(GROUP const &);
void RecalculateSectorWeight(uint8_t ubSectorID);
void RecalculateGroupWeight(GROUP const &);

BOOLEAN OkayForEnemyToMoveThroughSector(uint8_t ubSectorID);

void StrategicHandleQueenLosingControlOfSector(int16_t sSectorX, int16_t sSectorY,
                                               int16_t sSectorZ);

void WakeUpQueen();

void StrategicHandleMineThatRanOut(uint8_t ubSectorID);

int16_t FindPatrolGroupIndexForGroupID(uint8_t ubGroupID);
int16_t FindPatrolGroupIndexForGroupIDPending(uint8_t ubGroupID);
int16_t FindGarrisonIndexForGroupIDPending(uint8_t ubGroupID);

GROUP *FindPendingGroupInSector(uint8_t ubSectorID);

void RepollSAIGroup(GROUP *pGroup);

extern BOOLEAN gfDisplayStrategicAILogs;
extern BOOLEAN gfFirstBattleMeanwhileScenePending;

extern uint8_t gubSAIVersion;

// These enumerations define all of the various types of stationary garrison
// groups, and index their compositions for forces, etc.
enum {
  QUEEN_DEFENCE,       // The most important sector, the queen's palace.
  MEDUNA_DEFENCE,      // The town surrounding the queen's palace.
  MEDUNA_SAMSITE,      // A sam site within Meduna (higher priority)
  LEVEL1_DEFENCE,      // The sectors immediately adjacent to Meduna (defence and
                       // spawning area)
  LEVEL2_DEFENCE,      // Two sectors away from Meduna (defence and spawning area)
  LEVEL3_DEFENCE,      // Three sectors away from Meduna (defence and spawning area)
  ORTA_DEFENCE,        // The top secret military base containing lots of elites
  EAST_GRUMM_DEFENCE,  // The most-industrial town in Arulco (more mine income)
  WEST_GRUMM_DEFENCE,  // The most-industrial town in Arulco (more mine income)
  GRUMM_MINE,
  OMERTA_WELCOME_WAGON,  // Small force that greets the player upon arrival in
                         // game.
  BALIME_DEFENCE,        // Rich town, paved roads, close to Meduna (in queen's favor)
  TIXA_PRISON,           // Prison, well defended, but no point in retaking
  TIXA_SAMSITE,          // The central-most sam site (important for queen to keep)
  ALMA_DEFENCE,          // The military town of Meduna.  Also very important for queen.
  ALMA_MINE,             // Mine income AND administrators
  CAMBRIA_DEFENCE,       // Medical town, large, central.
  CAMBRIA_MINE,
  CHITZENA_DEFENCE,  // Small town, small mine, far away.
  CHITZENA_MINE,
  CHITZENA_SAMSITE,  // Sam site near Chitzena.
  DRASSEN_AIRPORT,   // Very far away, a supply depot of little importance.
  DRASSEN_DEFENCE,   // Medium town, normal.
  DRASSEN_MINE,
  DRASSEN_SAMSITE,  // Sam site near Drassen (least importance to queen of all
                    // samsites)
  ROADBLOCK,        // General outside city roadblocks -- enhance chance of ambush?
  SANMONA_SMALL,
  NUM_ARMY_COMPOSITIONS
};

struct ARMY_COMPOSITION {
  int32_t iReadability;  // contains the enumeration which is useless, but helps
                         // readability.
  int8_t bPriority;
  int8_t bElitePercentage;
  int8_t bTroopPercentage;
  int8_t bAdminPercentage;
  int8_t bDesiredPopulation;
  int8_t bStartPopulation;
  int8_t bPadding[10];  // XXX HACK000B
};

// Defines the patrol groups -- movement groups.
struct PATROL_GROUP {
  int8_t bSize;
  int8_t bPriority;
  uint8_t ubSectorID[4];
  int8_t bFillPermittedAfterDayMod100;
  uint8_t ubGroupID;
  int8_t bWeight;
  uint8_t ubPendingGroupID;
  int8_t bPadding[10];  // XXX HACK000B
};

// Defines all stationary defence forces.
struct GARRISON_GROUP {
  uint8_t ubSectorID;
  uint8_t ubComposition;
  int8_t bWeight;
  uint8_t ubPendingGroupID;
  int8_t bPadding[10];  // XXX HACK000B
};

#endif
