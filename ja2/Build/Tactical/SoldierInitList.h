#ifndef __SOLDIER_INIT_LIST_H
#define __SOLDIER_INIT_LIST_H

#include "Tactical/SoldierCreate.h"

struct SOLDIERINITNODE {
  uint8_t ubNodeID;
  uint8_t ubSoldierID;
  BASIC_SOLDIERCREATE_STRUCT *pBasicPlacement;
  SOLDIERCREATE_STRUCT *pDetailedPlacement;
  SOLDIERTYPE *pSoldier;
  SOLDIERINITNODE *prev;
  SOLDIERINITNODE *next;
};

extern SOLDIERINITNODE *gSoldierInitHead;
extern SOLDIERINITNODE *gSoldierInitTail;

#define BASE_FOR_EACH_SOLDIERINITNODE(type, iter) \
  for (type *iter = gSoldierInitHead; iter != NULL; iter = iter->next)
#define FOR_EACH_SOLDIERINITNODE(iter) BASE_FOR_EACH_SOLDIERINITNODE(SOLDIERINITNODE, iter)
#define CFOR_EACH_SOLDIERINITNODE(iter) BASE_FOR_EACH_SOLDIERINITNODE(const SOLDIERINITNODE, iter)

#define FOR_EACH_SOLDIERINITNODE_SAFE(iter)                                                \
  for (SOLDIERINITNODE *iter = gSoldierInitHead, *iter##__next; iter; iter = iter##__next) \
    if (iter##__next = iter->next, false) {                                                \
    } else

// These serialization functions are assuming the passing of a valid file
// pointer to the beginning of the save/load area, at the correct part of the
// map file.
void LoadSoldiersFromMap(HWFILE, bool stracLinuxFormat);

BOOLEAN SaveSoldiersToMap(HWFILE fp);

// For the purpose of keeping track of which soldier belongs to which placement
// within the game, the only way we can do this properly is to save the soldier
// ID from the list and reconnect the soldier pointer whenever we load the game.
void SaveSoldierInitListLinks(HWFILE);
void LoadSoldierInitListLinks(HWFILE);
void NewWayOfLoadingEnemySoldierInitListLinks(HWFILE);
void NewWayOfLoadingCivilianInitListLinks(HWFILE);

void InitSoldierInitList();
void KillSoldierInitList();
SOLDIERINITNODE *AddBasicPlacementToSoldierInitList(BASIC_SOLDIERCREATE_STRUCT const &);
void RemoveSoldierNodeFromInitList(SOLDIERINITNODE *pNode);
SOLDIERINITNODE *FindSoldierInitNodeWithID(uint16_t usID);
SOLDIERINITNODE *FindSoldierInitNodeBySoldier(SOLDIERTYPE const &);

void AddSoldierInitListTeamToWorld(int8_t team);
void AddSoldierInitListEnemyDefenceSoldiers(uint8_t ubTotalAdmin, uint8_t ubTotalTroops,
                                            uint8_t ubTotalElite);
void AddSoldierInitListCreatures(BOOLEAN fQueen, uint8_t ubNumLarvae, uint8_t ubNumInfants,
                                 uint8_t ubNumYoungMales, uint8_t ubNumYoungFemales,
                                 uint8_t ubNumAdultMales, uint8_t ubNumAdultFemales);
void AddSoldierInitListMilitia(uint8_t ubNumGreen, uint8_t ubNumReg, uint8_t ubNumElites);

void AddSoldierInitListBloodcats();

void UseEditorOriginalList();
void UseEditorAlternateList();

/* Any killed people that used detailed placement information must prevent that
 * from occurring again in the future.  Otherwise, the sniper guy with 99
 * marksmanship could appear again if the map was loaded again! */
void EvaluateDeathEffectsToSoldierInitList(SOLDIERTYPE const &);

void AddProfilesUsingProfileInsertionData();
void AddProfilesNotUsingProfileInsertionData();

void StripEnemyDetailedPlacementsIfSectorWasPlayerLiberated();

bool AddPlacementToWorld(SOLDIERINITNODE *);

#endif
