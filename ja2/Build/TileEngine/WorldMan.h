#ifndef __WORLDMAN_H_
#define __WORLDMAN_H_

#include <exception>

#include "TileEngine/WorldDef.h"

// memory-accounting function
void CountLevelNodes();

class FailedToAddNode : public std::exception {
 public:
  virtual char const *what() const throw() { return "Failed to add node to world"; }
};

// Object manipulation functions
BOOLEAN RemoveObject(uint32_t iMapIndex, uint16_t usIndex);
LEVELNODE *AddObjectToTail(uint32_t iMapIndex, uint16_t usIndex);
LEVELNODE *AddObjectToHead(uint32_t iMapIndex, uint16_t usIndex);
LEVELNODE *FindTypeInObjectLayer(uint32_t map_idx, uint32_t type);
BOOLEAN RemoveAllObjectsOfTypeRange(uint32_t iMapIndex, uint32_t fStartType, uint32_t fEndType);
LEVELNODE *TypeRangeExistsInObjectLayer(uint32_t iMapIndex, uint32_t fStartType, uint32_t fEndType);

// Roof manipulation functions
BOOLEAN RemoveRoof(uint32_t iMapIndex, uint16_t usIndex);
LEVELNODE *AddRoofToTail(uint32_t iMapIndex, uint16_t usIndex);
LEVELNODE *AddRoofToHead(uint32_t iMapIndex, uint16_t usIndex);
LEVELNODE *FindTypeInRoofLayer(uint32_t map_idx, uint32_t type);
BOOLEAN RemoveAllRoofsOfTypeRange(uint32_t iMapIndex, uint32_t fStartType, uint32_t fEndType);
void RemoveRoofIndexFlagsFromTypeRange(uint32_t iMapIndex, uint32_t fStartType, uint32_t fEndType,
                                       LevelnodeFlags);
void SetRoofIndexFlagsFromTypeRange(uint32_t iMapIndex, uint32_t fStartType, uint32_t fEndType,
                                    LevelnodeFlags);
LEVELNODE *TypeRangeExistsInRoofLayer(uint32_t iMapIndex, uint32_t fStartType, uint32_t fEndType);
void SetWallLevelnodeFlags(uint16_t sGridNo, LevelnodeFlags);
void RemoveWallLevelnodeFlags(uint16_t sGridNo, LevelnodeFlags);
BOOLEAN IndexExistsInRoofLayer(int16_t sGridNo, uint16_t usIndex);

// OnRoof manipulation functions
BOOLEAN RemoveOnRoof(uint32_t iMapIndex, uint16_t usIndex);
LEVELNODE *AddOnRoofToTail(uint32_t iMapIndex, uint16_t usIndex);
LEVELNODE *AddOnRoofToHead(uint32_t iMapIndex, uint16_t usIndex);
BOOLEAN RemoveAllOnRoofsOfTypeRange(uint32_t iMapIndex, uint32_t fStartType, uint32_t fEndType);
BOOLEAN RemoveOnRoofFromLevelNode(uint32_t iMapIndex, LEVELNODE *pNode);

// Land manipulation functions
void RemoveLand(uint32_t map_idx, uint16_t idx);
LEVELNODE *AddLandToTail(uint32_t iMapIndex, uint16_t usIndex);
void AddLandToHead(uint32_t iMapIndex, uint16_t usIndex);
LEVELNODE *FindTypeInLandLayer(uint32_t map_idx, uint32_t type);
BOOLEAN RemoveAllLandsOfTypeRange(uint32_t iMapIndex, uint32_t fStartType, uint32_t fEndType);
BOOLEAN TypeRangeExistsInLandLayer(uint32_t iMapIndex, uint32_t fStartType, uint32_t fEndType);
void ReplaceLandIndex(uint32_t iMapIndex, uint16_t usOldIndex, uint16_t usNewIndex);
void DeleteAllLandLayers(uint32_t iMapIndex);
void InsertLandIndexAtLevel(uint32_t iMapIndex, uint16_t usIndex, uint8_t ubLevel);
void RemoveHigherLandLevels(uint32_t map_idx, uint32_t src_type, uint32_t *&higher_types,
                            uint8_t &n_higher_types);

uint8_t GetTerrainType(GridNo);
bool Water(GridNo);
bool DeepWater(GridNo);
bool WaterTooDeepForAttacks(GridNo);

// Structure manipulation routines
void RemoveStruct(uint32_t map_idx, uint16_t usIndex);
LEVELNODE *AddStructToTail(uint32_t iMapIndex, uint16_t usIndex);
LEVELNODE *ForceStructToTail(uint32_t iMapIndex, uint16_t usIndex);

void AddStructToHead(uint32_t iMapIndex, uint16_t usIndex);
BOOLEAN RemoveAllStructsOfTypeRange(uint32_t iMapIndex, uint32_t fStartType, uint32_t fEndType);
bool AddWallToStructLayer(int32_t map_idx, uint16_t idx, bool replace);
BOOLEAN ReplaceStructIndex(uint32_t iMapIndex, uint16_t usOldIndex, uint16_t usNewIndex);
void HideStructOfGivenType(uint32_t iMapIndex, uint32_t fType, BOOLEAN fHide);
void SetStructAframeFlags(uint32_t iMapIndex, LevelnodeFlags);
void RemoveStructFromLevelNode(uint32_t map_idx, LEVELNODE *);
BOOLEAN IndexExistsInStructLayer(GridNo, uint16_t tile_index);

void ForceRemoveStructFromTail(uint32_t iMapIndex);

// Shadow manipulation routines
void AddShadowToTail(uint32_t iMapIndex, uint16_t usIndex);
LEVELNODE *AddShadowToHead(uint32_t iMapIndex, uint16_t usIndex);
void AddExclusiveShadow(uint32_t iMapIndex, uint16_t usIndex);
BOOLEAN RemoveAllShadowsOfTypeRange(uint32_t iMapIndex, uint32_t fStartType, uint32_t fEndType);
BOOLEAN RemoveAllShadows(uint32_t iMapIndex);
BOOLEAN RemoveShadowFromLevelNode(uint32_t iMapIndex, LEVELNODE *pNode);

// Merc manipulation routines
// #################################################################

LEVELNODE *AddMercToHead(uint32_t iMapIndex, SOLDIERTYPE &, BOOLEAN fAddStructInfo);
void RemoveMerc(uint32_t map_idx, SOLDIERTYPE &, bool placeholder);
SOLDIERTYPE *WhoIsThere2(int16_t sGridNo, int8_t bLevel);
BOOLEAN AddMercStructureInfoFromAnimSurface(int16_t sGridNo, SOLDIERTYPE *pSoldier,
                                            uint16_t usAnimSurface, uint16_t usAnimState);
BOOLEAN UpdateMercStructureInfo(SOLDIERTYPE *pSoldier);
BOOLEAN OKToAddMercToWorld(SOLDIERTYPE *pSoldier, int8_t bDirection);

// TOPMOST manipulation functions
LEVELNODE *AddTopmostToTail(uint32_t iMapIndex, uint16_t usIndex);
LEVELNODE *AddTopmostToHead(uint32_t iMapIndex, uint16_t usIndex);
BOOLEAN RemoveTopmost(uint32_t iMapIndex, uint16_t usIndex);
LEVELNODE *FindTypeInTopmostLayer(uint32_t map_idx, uint32_t type);
BOOLEAN RemoveAllTopmostsOfTypeRange(uint32_t iMapIndex, uint32_t fStartType, uint32_t fEndType);
LEVELNODE *AddUIElem(uint32_t iMapIndex, uint16_t usIndex, int8_t sRelativeX, int8_t sRelativeY);
BOOLEAN RemoveTopmostFromLevelNode(uint32_t iMapIndex, LEVELNODE *pNode);

BOOLEAN IsHeigherLevel(int16_t sGridNo);
BOOLEAN IsRoofVisible(int16_t sMapPos);
BOOLEAN IsRoofVisible2(int16_t sMapPos);

LEVELNODE *FindLevelNodeBasedOnStructure(STRUCTURE const *);
LEVELNODE *FindShadow(int16_t sGridNo, uint16_t usStructIndex);

void WorldHideTrees();
void WorldShowTrees();

// this is found in editscreen.c
// Andrew, you had worldman.c checked out at the time, so I stuck it here.
// The best thing to do is toast it here, and include editscreen.h in
// worldman.c.
extern uint32_t gCurrentBackground;

void SetTreeTopStateForMap();

void DebugLevelNodePage();

#endif
