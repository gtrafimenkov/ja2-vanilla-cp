#include "TileEngine/WorldMan.h"

#include <stdexcept>

#include "Editor/Smooth.h"
#include "GameSettings.h"
#include "SGP/Font.h"
#include "SGP/MemMan.h"
#include "SGP/Random.h"
#include "Tactical/AnimationControl.h"
#include "Tactical/AnimationData.h"
#include "Tactical/Overhead.h"
#include "TacticalAI/AI.h"
#include "TileEngine/Environment.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/Lighting.h"
#include "TileEngine/RenderFun.h"
#include "TileEngine/RenderWorld.h"
#include "TileEngine/SaveLoadMap.h"
#include "TileEngine/Structure.h"
#include "TileEngine/TileCache.h"
#include "TileEngine/TileDef.h"
#include "TileEngine/WorldDef.h"
#include "Utils/FontControl.h"
#include "Utils/Message.h"

static uint32_t guiLNCount[9];
static const wchar_t gzLevelString[][15] = {
    L"",           L"Land    %d", L"Object  %d", L"Struct  %d", L"Shadow  %d",
    L"Merc    %d", L"Roof    %d", L"Onroof  %d", L"Topmost %d",
};

// LEVEL NODE MANIPLULATION FUNCTIONS
static LEVELNODE *CreateLevelNode() {
  LEVELNODE *const Node = MALLOCZ(LEVELNODE);
  Node->ubShadeLevel = LightGetAmbient();
  Node->ubNaturalShadeLevel = LightGetAmbient();
  Node->pSoldier = NULL;
  Node->pNext = NULL;
  Node->sRelativeX = 0;
  Node->sRelativeY = 0;
  return Node;
}

void CountLevelNodes() {
  for (uint32_t uiLoop2 = 0; uiLoop2 < 9; uiLoop2++) {
    guiLNCount[uiLoop2] = 0;
  }

  FOR_EACH_WORLD_TILE(pME) {
    // start at 1 to skip land head ptr; 0 stores total
    for (uint32_t uiLoop2 = 1; uiLoop2 < 9; uiLoop2++) {
      for (const LEVELNODE *pLN = pME->pLevelNodes[uiLoop2]; pLN != NULL; pLN = pLN->pNext) {
        guiLNCount[uiLoop2]++;
        guiLNCount[0]++;
      }
    }
  }
}

#define LINE_HEIGHT 20
void DebugLevelNodePage() {
  SetFont(LARGEFONT1);
  gprintf(0, 0, L"DEBUG LEVELNODES PAGE 1 OF 1");

  for (uint32_t uiLoop = 1; uiLoop < 9; uiLoop++) {
    gprintf(0, LINE_HEIGHT * (uiLoop + 1), gzLevelString[uiLoop], guiLNCount[uiLoop]);
  }
  gprintf(0, LINE_HEIGHT * 12, L"%d land nodes in excess of world max (25600)",
          guiLNCount[1] - WORLD_MAX);
  gprintf(0, LINE_HEIGHT * 13, L"Total # levelnodes %d, %d bytes each", guiLNCount[0],
          sizeof(LEVELNODE));
  gprintf(0, LINE_HEIGHT * 14, L"Total memory for levelnodes %d",
          guiLNCount[0] * sizeof(LEVELNODE));
}

static LEVELNODE *FindTypeInLayer(LEVELNODE *const start_node, uint32_t const type) {
  // Look through all objects and Search for type
  for (LEVELNODE *i = start_node; i; i = i->pNext) {
    uint16_t const idx = i->usIndex;
    if (idx == NO_TILE || idx >= NUMBEROFTILES) continue;
    if (GetTileType(idx) != type) continue;
    return i;
  }
  return 0;
}

// First for object layer
// #################################################################

LEVELNODE *AddObjectToTail(const uint32_t iMapIndex, const uint16_t usIndex) {
  LEVELNODE *const n = CreateLevelNode();
  n->usIndex = usIndex;

  // Append node to list
  LEVELNODE **anchor = &gpWorldLevelData[iMapIndex].pObjectHead;
  while (*anchor != NULL) anchor = &(*anchor)->pNext;
  *anchor = n;

  ResetSpecificLayerOptimizing(TILES_DYNAMIC_OBJECTS);
  return n;
}

LEVELNODE *AddObjectToHead(const uint32_t iMapIndex, const uint16_t usIndex) {
  LEVELNODE *const n = CreateLevelNode();
  n->usIndex = usIndex;

  LEVELNODE **const head = &gpWorldLevelData[iMapIndex].pObjectHead;
  n->pNext = *head;
  *head = n;

  ResetSpecificLayerOptimizing(TILES_DYNAMIC_OBJECTS);
  AddObjectToMapTempFile(iMapIndex, usIndex);
  return n;
}

BOOLEAN RemoveObject(uint32_t iMapIndex, uint16_t usIndex) {
  // Look through all objects and remove index if found
  LEVELNODE *pOldObject = NULL;
  for (LEVELNODE *pObject = gpWorldLevelData[iMapIndex].pObjectHead; pObject != NULL;
       pObject = pObject->pNext) {
    if (pObject->usIndex == usIndex) {
      // OK, set links
      // Check for head or tail
      if (pOldObject == NULL) {
        // It's the head
        gpWorldLevelData[iMapIndex].pObjectHead = pObject->pNext;
      } else {
        pOldObject->pNext = pObject->pNext;
      }

      CheckForAndDeleteTileCacheStructInfo(pObject, usIndex);

      MemFree(pObject);

      // Add the index to the maps temp file so we can remove it after reloading
      // the map
      AddRemoveObjectToMapTempFile(iMapIndex, usIndex);

      return TRUE;
    }

    pOldObject = pObject;
  }

  // Could not find it
  return FALSE;
}

LEVELNODE *TypeRangeExistsInObjectLayer(uint32_t const iMapIndex, uint32_t const fStartType,
                                        uint32_t const fEndType) {
  // Look through all objects and Search for type
  for (LEVELNODE *pObject = gpWorldLevelData[iMapIndex].pObjectHead; pObject != NULL;
       pObject = pObject->pNext) {
    if (pObject->usIndex == NO_TILE || pObject->usIndex >= NUMBEROFTILES) continue;

    uint32_t const fTileType = GetTileType(pObject->usIndex);
    if (fTileType < fStartType || fEndType < fTileType) continue;

    return pObject;
  }

  // Could not find it
  return 0;
}

LEVELNODE *FindTypeInObjectLayer(uint32_t const map_idx, uint32_t const type) {
  return FindTypeInLayer(gpWorldLevelData[map_idx].pObjectHead, type);
}

BOOLEAN RemoveAllObjectsOfTypeRange(uint32_t iMapIndex, uint32_t fStartType, uint32_t fEndType) {
  BOOLEAN fRetVal = FALSE;

  // Look through all objects and Search for type
  for (LEVELNODE *pObject = gpWorldLevelData[iMapIndex].pObjectHead; pObject != NULL;) {
    LEVELNODE *Next = pObject->pNext;

    if (pObject->usIndex != NO_TILE && pObject->usIndex < NUMBEROFTILES) {
      const uint32_t fTileType = GetTileType(pObject->usIndex);
      if (fTileType >= fStartType && fTileType <= fEndType) {
        RemoveObject(iMapIndex, pObject->usIndex);
        fRetVal = TRUE;
      }
    }

    pObject = Next;
  }
  return fRetVal;
}

// #######################################################
// Land Piece Layer
// #######################################################

LEVELNODE *AddLandToTail(const uint32_t iMapIndex, const uint16_t usIndex) {
  LEVELNODE *const n = CreateLevelNode();
  n->usIndex = usIndex;

  // Append node to list
  LEVELNODE *prev = NULL;
  LEVELNODE **anchor = &gpWorldLevelData[iMapIndex].pLandHead;
  while (*anchor != NULL) {
    prev = *anchor;
    anchor = &prev->pNext;
  }
  *anchor = n;
  n->pPrevNode = prev;

  ResetSpecificLayerOptimizing(TILES_DYNAMIC_LAND);
  return n;
}

void AddLandToHead(const uint32_t iMapIndex, const uint16_t usIndex) {
  LEVELNODE *const n = CreateLevelNode();
  n->usIndex = usIndex;

  LEVELNODE **const head = &gpWorldLevelData[iMapIndex].pLandHead;
  if (*head != NULL) (*head)->pPrevNode = n;
  n->pNext = *head;
  n->pPrevNode = NULL;
  *head = n;

  if (usIndex < NUMBEROFTILES && gTileDatabase[usIndex].ubFullTile) {
    gpWorldLevelData[iMapIndex].pLandStart = n;
  }

  ResetSpecificLayerOptimizing(TILES_DYNAMIC_LAND);
}

static BOOLEAN AdjustForFullTile(uint32_t iMapIndex);
static void RemoveLandEx(uint32_t iMapIndex, uint16_t usIndex);

void RemoveLand(uint32_t const map_idx, uint16_t const idx) {
  RemoveLandEx(map_idx, idx);
  AdjustForFullTile(map_idx);
}

static void RemoveLandEx(uint32_t iMapIndex, uint16_t usIndex) {
  // Look through all Lands and remove index if found
  for (LEVELNODE *pLand = gpWorldLevelData[iMapIndex].pLandHead; pLand != NULL;
       pLand = pLand->pNext) {
    if (pLand->usIndex == usIndex) {
      // Check for head
      if (pLand->pPrevNode == NULL) {
        // It's the head
        gpWorldLevelData[iMapIndex].pLandHead = pLand->pNext;
      } else {
        pLand->pPrevNode->pNext = pLand->pNext;
      }

      // Check for tail
      if (pLand->pNext != NULL) {
        pLand->pNext->pPrevNode = pLand->pPrevNode;
      }

      MemFree(pLand);
      break;
    }
  }
}

static BOOLEAN AdjustForFullTile(uint32_t iMapIndex) {
  for (LEVELNODE *pLand = gpWorldLevelData[iMapIndex].pLandHead; pLand != NULL;
       pLand = pLand->pNext) {
    if (pLand->usIndex < NUMBEROFTILES) {
      // If this is a full tile, set new full tile
      if (gTileDatabase[pLand->usIndex].ubFullTile) {
        gpWorldLevelData[iMapIndex].pLandStart = pLand;
        return TRUE;
      }
    }
  }

  // Could not find a full tile
  // Set to tail, and convert it to a full tile!
  // Add a land piece to tail from basic land

  uint16_t NewIndex = Random(10);

  // Adjust for type
  NewIndex += gTileTypeStartIndex[gCurrentBackground];

  LEVELNODE *pNewNode = AddLandToTail(iMapIndex, NewIndex);
  gpWorldLevelData[iMapIndex].pLandStart = pNewNode;

  return FALSE;
}

void ReplaceLandIndex(uint32_t const iMapIndex, uint16_t const usOldIndex,
                      uint16_t const usNewIndex) {
  // Look through all Lands and remove index if found
  for (LEVELNODE *pLand = gpWorldLevelData[iMapIndex].pLandHead; pLand != NULL;
       pLand = pLand->pNext) {
    if (pLand->usIndex == usOldIndex) {
      // OK, set new index value
      pLand->usIndex = usNewIndex;
      AdjustForFullTile(iMapIndex);
      return;
    }
  }

  // Could not find it
  throw std::logic_error("Tried to replace non-existent land index");
}

LEVELNODE *FindTypeInLandLayer(uint32_t const map_idx, uint32_t const type) {
  return FindTypeInLayer(gpWorldLevelData[map_idx].pLandHead, type);
}

BOOLEAN TypeRangeExistsInLandLayer(uint32_t iMapIndex, uint32_t fStartType, uint32_t fEndType) {
  // Look through all objects and Search for type
  for (const LEVELNODE *pLand = gpWorldLevelData[iMapIndex].pLandHead; pLand != NULL;) {
    if (pLand->usIndex != NO_TILE) {
      const uint32_t fTileType = GetTileType(pLand->usIndex);

      pLand = pLand->pNext;  // XXX TODO0009 if pLand->usIndex == NO_TILE this is
                             // an endless loop

      if (fTileType >= fStartType && fTileType <= fEndType) {
        return TRUE;
      }
    }
  }

  // Could not find it
  return FALSE;
}

BOOLEAN RemoveAllLandsOfTypeRange(uint32_t iMapIndex, uint32_t fStartType, uint32_t fEndType) {
  const LEVELNODE *pLand = gpWorldLevelData[iMapIndex].pLandHead;
  BOOLEAN fRetVal = FALSE;

  // Look through all objects and Search for type
  while (pLand != NULL) {
    if (pLand->usIndex != NO_TILE) {
      const LEVELNODE *Next = pLand->pNext;

      const uint32_t fTileType = GetTileType(pLand->usIndex);
      if (fTileType >= fStartType && fTileType <= fEndType) {
        // Remove Item
        RemoveLand(iMapIndex, pLand->usIndex);
        fRetVal = TRUE;
      }

      pLand = Next;  // XXX TODO0009 if pLand->usIndex == NO_TILE this is an
                     // endless loop
    }
  }
  return fRetVal;
}

void DeleteAllLandLayers(uint32_t iMapIndex) {
  const LEVELNODE *pLand = gpWorldLevelData[iMapIndex].pLandHead;

  while (pLand != NULL) {
    const LEVELNODE *pOldLand = pLand;
    pLand = pLand->pNext;
    RemoveLandEx(iMapIndex, pOldLand->usIndex);
  }

  // Set world data values
  gpWorldLevelData[iMapIndex].pLandHead = NULL;
  gpWorldLevelData[iMapIndex].pLandStart = NULL;
}

void InsertLandIndexAtLevel(const uint32_t iMapIndex, const uint16_t usIndex,
                            const uint8_t ubLevel) {
  // If we want to insert at head;
  if (ubLevel == 0) {
    AddLandToHead(iMapIndex, usIndex);
    return;
  }

  // Move to index before insertion
  LEVELNODE *pLand = gpWorldLevelData[iMapIndex].pLandHead;
  for (uint8_t level = 0;; ++level) {
    if (!pLand) throw std::logic_error("Tried to insert land index at invalid level");

    if (level == ubLevel - 1) break;

    pLand = pLand->pNext;
  }

  LEVELNODE *const n = CreateLevelNode();
  n->usIndex = usIndex;

  // Set links, according to position!
  n->pPrevNode = pLand;
  n->pNext = pLand->pNext;
  pLand->pNext = n;

  // Check for tail
  if (n->pNext != NULL) n->pNext->pPrevNode = n;

  AdjustForFullTile(iMapIndex);

  ResetSpecificLayerOptimizing(TILES_DYNAMIC_LAND);
}

void RemoveHigherLandLevels(uint32_t const map_idx, uint32_t const src_type,
                            uint32_t *&out_higher_types, uint8_t &out_n_higher_types) {
  out_n_higher_types = 0;
  out_higher_types = 0;

  // Get tail
  LEVELNODE *tail = 0;
  for (LEVELNODE *i = gpWorldLevelData[map_idx].pLandHead; i; i = i->pNext) {
    tail = i;
  }

  uint8_t const src_log_height = GetTileTypeLogicalHeight(src_type);
  for (LEVELNODE *i = tail; i;) {
    LEVELNODE const &l = *i;
    i = i->pPrevNode;

    uint32_t const tile_type = GetTileType(l.usIndex);
    if (GetTileTypeLogicalHeight(tile_type) <= src_log_height) continue;

    RemoveLand(map_idx, l.usIndex);

    out_higher_types = REALLOC(out_higher_types, uint32_t, out_n_higher_types + 1);
    out_higher_types[out_n_higher_types] = tile_type;
    ++out_n_higher_types;
  }

  AdjustForFullTile(map_idx);
}

static LEVELNODE *AddNodeToWorld(uint32_t const iMapIndex, uint16_t const usIndex,
                                 int8_t const level) {
  LEVELNODE *const n = CreateLevelNode();
  n->usIndex = usIndex;

  if (usIndex >= NUMBEROFTILES) return n;

  const DB_STRUCTURE_REF *const sr = gTileDatabase[usIndex].pDBStructureRef;
  if (!sr) return n;

  if (AddStructureToWorld(iMapIndex, level, sr, n)) return n;

  MemFree(n);
  throw FailedToAddNode();
}

// Struct layer
// #################################################################

static LEVELNODE *AddStructToTailCommon(uint32_t const map_idx, uint16_t const idx,
                                        LEVELNODE *const n) {
  MAP_ELEMENT &me = gpWorldLevelData[map_idx];
  // Append node to list
  LEVELNODE **anchor = &me.pStructHead;
  while (*anchor) anchor = &(*anchor)->pNext;
  *anchor = n;

  if (idx < NUMBEROFTILES) {
    TILE_ELEMENT const &te = gTileDatabase[idx];
    // Check flags for tiledat and set a shadow if we have a buddy
    if (!GridNoIndoors(map_idx) && te.uiFlags & HAS_SHADOW_BUDDY && te.sBuddyNum != -1) {
      LEVELNODE *const n = AddShadowToHead(map_idx, te.sBuddyNum);
      n->uiFlags |= LEVELNODE_BUDDYSHADOW;
    }

    // Check for special flag to stop burn-through on same-tile structs
    if (DB_STRUCTURE_REF const *const sr = te.pDBStructureRef) {
      // If we are NOT a wall and NOT multi-tiles, set mapelement flag
      if (!FindStructure(map_idx, STRUCTURE_WALLSTUFF) &&
          sr->pDBStructure->ubNumberOfTiles == 1)  // XXX TODO0015
      {
        me.ubExtFlags[0] |= MAPELEMENT_EXT_NOBURN_STRUCT;
      } else {
        me.ubExtFlags[0] &= ~MAPELEMENT_EXT_NOBURN_STRUCT;
      }
    }
  }

  AddStructToMapTempFile(map_idx, idx);

  ResetSpecificLayerOptimizing(TILES_DYNAMIC_STRUCTURES);
  return n;
}

LEVELNODE *AddStructToTail(uint32_t const map_idx, uint16_t const idx) {
  LEVELNODE *const n = AddNodeToWorld(map_idx, idx, 0);
  return AddStructToTailCommon(map_idx, idx, n);
}

LEVELNODE *ForceStructToTail(uint32_t const map_idx, uint16_t const idx) {
  LEVELNODE *const n = CreateLevelNode();
  n->usIndex = idx;
  return AddStructToTailCommon(map_idx, idx, n);
}

void AddStructToHead(uint32_t const map_idx, uint16_t const idx) {
  LEVELNODE *const n = AddNodeToWorld(map_idx, idx, 0);

  MAP_ELEMENT &me = gpWorldLevelData[map_idx];
  // Prepend node to list
  LEVELNODE **const head = &me.pStructHead;
  n->pNext = *head;
  *head = n;

  if (idx < NUMBEROFTILES) {
    TILE_ELEMENT const &te = gTileDatabase[idx];
    // Check flags for tiledat and set a shadow if we have a buddy
    if (!GridNoIndoors(map_idx) && te.uiFlags & HAS_SHADOW_BUDDY && te.sBuddyNum != -1) {
      LEVELNODE *const n = AddShadowToHead(map_idx, te.sBuddyNum);
      n->uiFlags |= LEVELNODE_BUDDYSHADOW;
    }

    // Check for special flag to stop burn-through on same-tile structs
    if (DB_STRUCTURE_REF const *const sr = te.pDBStructureRef) {
      // If we are NOT a wall and NOT multi-tiles, set mapelement flag
      if (FindStructure(map_idx, STRUCTURE_WALLSTUFF) &&
          sr->pDBStructure->ubNumberOfTiles == 1)  // XXX TODO0015
      {
        me.ubExtFlags[0] |= MAPELEMENT_EXT_NOBURN_STRUCT;
      } else {
        me.ubExtFlags[0] &= ~MAPELEMENT_EXT_NOBURN_STRUCT;
      }
    }
  }

  AddStructToMapTempFile(map_idx, idx);

  ResetSpecificLayerOptimizing(TILES_DYNAMIC_STRUCTURES);
}

static void InsertStructIndex(const uint32_t iMapIndex, const uint16_t usIndex,
                              const uint8_t ubLevel) {
  // If we want to insert at head
  if (ubLevel == 0) {
    AddStructToHead(iMapIndex, usIndex);
    return;
  }

  // Move to index before insertion
  LEVELNODE *pStruct = gpWorldLevelData[iMapIndex].pStructHead;
  for (uint8_t level = 0;; ++level) {
    if (!pStruct) throw std::logic_error("Tried to insert struct at invalid level");

    if (level == ubLevel - 1) break;

    pStruct = pStruct->pNext;
  }

  LEVELNODE *const n = AddNodeToWorld(iMapIndex, usIndex, 0);

  // Set links, according to position!
  n->pNext = pStruct->pNext;
  pStruct->pNext = n;

  ResetSpecificLayerOptimizing(TILES_DYNAMIC_STRUCTURES);
}

static BOOLEAN RemoveShadow(uint32_t iMapIndex, uint16_t usIndex);

static void RemoveShadowBuddy(uint32_t iMapIndex, uint16_t usIndex) {
  if (usIndex >= NUMBEROFTILES) return;
  if (GridNoIndoors(iMapIndex)) return;

  const TILE_ELEMENT *const te = &gTileDatabase[usIndex];
  if (!(te->uiFlags & HAS_SHADOW_BUDDY)) return;
  if (te->sBuddyNum == -1) return;

  RemoveShadow(iMapIndex, te->sBuddyNum);
}

void ForceRemoveStructFromTail(uint32_t const iMapIndex) {
  LEVELNODE *pPrevStruct = NULL;

  // GOTO TAIL
  for (LEVELNODE *pStruct = gpWorldLevelData[iMapIndex].pStructHead; pStruct != NULL;
       pStruct = pStruct->pNext) {
    // AT THE TAIL
    if (pStruct->pNext == NULL) {
      if (pPrevStruct != NULL) {
        pPrevStruct->pNext = pStruct->pNext;
      } else {
        gpWorldLevelData[iMapIndex].pStructHead = pPrevStruct;
      }

      uint16_t usIndex = pStruct->usIndex;

      // XXX TODO000A It rather seems like a memory leak not to
      // DeleteStructureFromWorld() here. See InternalRemoveStruct()

      // If we have to, make sure to remove this node when we reload the map
      // from a saved game
      RemoveStructFromMapTempFile(iMapIndex, usIndex);

      MemFree(pStruct);

      RemoveShadowBuddy(iMapIndex, usIndex);
      return;
    }

    pPrevStruct = pStruct;
  }
}

static void InternalRemoveStruct(uint32_t const map_idx, LEVELNODE **const anchor) {
  LEVELNODE *const removee = *anchor;
  *anchor = removee->pNext;

  // Delete memory assosiated with item
  DeleteStructureFromWorld(removee->pStructureData);

  uint16_t const idx = removee->usIndex;

  // If we have to, make sure to remove this node when we reload the map from a
  // saved game
  RemoveStructFromMapTempFile(map_idx, idx);

  RemoveShadowBuddy(map_idx, idx);
  MemFree(removee);
}

void RemoveStruct(uint32_t const map_idx, uint16_t const idx) {
  // Look through all structs and remove index if found
  for (LEVELNODE **anchor = &gpWorldLevelData[map_idx].pStructHead;; anchor = &(*anchor)->pNext) {
    LEVELNODE *const i = *anchor;
    if (!i) return;  // XXX exception?
    if (i->usIndex != idx) continue;
    InternalRemoveStruct(map_idx, anchor);
    return;
  }
}

void RemoveStructFromLevelNode(uint32_t const map_idx, LEVELNODE *const n) {
  // Look through all structs and remove index if found
  for (LEVELNODE **anchor = &gpWorldLevelData[map_idx].pStructHead;; anchor = &(*anchor)->pNext) {
    LEVELNODE *const i = *anchor;
    if (!i) return;  // XXX exception?
    if (i != n) continue;
    InternalRemoveStruct(map_idx, anchor);
    return;
  }
}

BOOLEAN RemoveAllStructsOfTypeRange(uint32_t iMapIndex, uint32_t fStartType, uint32_t fEndType) {
  BOOLEAN fRetVal = FALSE;

  // Look through all structs and Search for type
  for (const LEVELNODE *pStruct = gpWorldLevelData[iMapIndex].pStructHead; pStruct != NULL;) {
    if (pStruct->uiFlags & LEVELNODE_CACHEDANITILE) {
      pStruct = pStruct->pNext;
      continue;
    }

    if (pStruct->usIndex != NO_TILE) {
      const uint32_t fTileType = GetTileType(pStruct->usIndex);

      // Advance to next
      const LEVELNODE *pOldStruct = pStruct;
      pStruct = pStruct->pNext;  // XXX TODO0009 if pStruct->usIndex == NO_TILE
                                 // this is an endless loop

      if (fTileType >= fStartType && fTileType <= fEndType) {
        uint16_t usIndex = pOldStruct->usIndex;
        if (usIndex < NUMBEROFTILES) {
          RemoveStruct(iMapIndex, usIndex);
          fRetVal = TRUE;
          RemoveShadowBuddy(iMapIndex, usIndex);
        }
      }
    }
  }
  return fRetVal;
}

// Kris:  This was a serious problem.  When saving the map and then reloading
// it, the structure
//  information was invalid if you changed the types, etc.  This is the
//  bulletproof way.
BOOLEAN ReplaceStructIndex(uint32_t iMapIndex, uint16_t usOldIndex, uint16_t usNewIndex) {
  RemoveStruct(iMapIndex, usOldIndex);
  AddWallToStructLayer(iMapIndex, usNewIndex, FALSE);
  return TRUE;
  //	LEVELNODE	*pStruct				= NULL;
  //	pStruct = gpWorldLevelData[ iMapIndex ].pStructHead;
  // Look through all Structs and remove index if found
  //	while( pStruct != NULL )
  //	{
  //		if ( pStruct->usIndex == usOldIndex )
  //		{
  //			// OK, set new index value
  //			pStruct->usIndex = usNewIndex;
  //			AdjustForFullTile( iMapIndex );
  //			return( TRUE );
  //		}
  //		// Advance
  //		pStruct = pStruct->pNext;
  //	}
  //	// Could not find it, return FALSE
  //	return( FALSE );
}

// When adding, put in order such that it's drawn before any walls of a
// lesser orientation value
bool AddWallToStructLayer(int32_t const map_idx, uint16_t const idx, bool const replace) {
  // Get orientation of piece we want to add
  uint16_t const wall_orientation = GetWallOrientation(idx);

  // Look through all objects and Search for orientation
  bool insert_found = false;
  bool roof_found = false;
  uint8_t roof_level = 0;
  uint8_t level = 0;
  for (LEVELNODE *i = gpWorldLevelData[map_idx].pStructHead; i; ++level, i = i->pNext) {
    if (i->uiFlags & LEVELNODE_CACHEDANITILE) continue;
    uint16_t const check_wall_orient = GetWallOrientation(i->usIndex);

    /* Kris: If placing a new wall which is at right angles to the current wall,
     * then we insert it. */
    if (check_wall_orient > wall_orientation) {
      if (((wall_orientation == INSIDE_TOP_RIGHT || wall_orientation == OUTSIDE_TOP_RIGHT) &&
           (check_wall_orient == INSIDE_TOP_LEFT || check_wall_orient == OUTSIDE_TOP_LEFT)) ||
          ((wall_orientation == INSIDE_TOP_LEFT || wall_orientation == OUTSIDE_TOP_LEFT) &&
           (check_wall_orient == INSIDE_TOP_RIGHT || check_wall_orient == OUTSIDE_TOP_RIGHT))) {
        insert_found = true;
      }
    }

    uint32_t const check_type = GetTileType(i->usIndex);
    if (FIRSTROOF <= check_type && check_type <= LASTROOF) {
      roof_found = true;
      roof_level = level;
    }

    /* Kris: We want to check for walls being parallel to each other.  If so,
     * then we we want to replace it.  This is because of an existing problem
     * with say, INSIDE_TOP_LEFT and OUTSIDE_TOP_LEFT walls coexisting. */
    if (((wall_orientation == INSIDE_TOP_RIGHT || wall_orientation == OUTSIDE_TOP_RIGHT) &&
         (check_wall_orient == INSIDE_TOP_RIGHT || check_wall_orient == OUTSIDE_TOP_RIGHT)) ||
        ((wall_orientation == INSIDE_TOP_LEFT || wall_orientation == OUTSIDE_TOP_LEFT) &&
         (check_wall_orient == INSIDE_TOP_LEFT || check_wall_orient == OUTSIDE_TOP_LEFT))) {
      // Same, if replace, replace here
      return replace ? ReplaceStructIndex(map_idx, i->usIndex, idx) : false;
    }
  }

  // Check if we found an insert position, otherwise set to head
  if (insert_found) {
    AddStructToHead(map_idx, idx);
  } else if (roof_found)  // Make sure it's ALWAYS after the roof (if any)
  {
    InsertStructIndex(map_idx, idx, roof_level);
  } else {
    AddStructToTail(map_idx, idx);
  }

  ResetSpecificLayerOptimizing(TILES_DYNAMIC_STRUCTURES);
  return true;
}

static bool IndexExistsInLayer(LEVELNODE const *n, uint16_t const tile_index) {
  for (; n; n = n->pNext) {
    if (n->usIndex == tile_index) return true;
  }
  return false;
}

BOOLEAN IndexExistsInStructLayer(GridNo const grid_no, uint16_t const tile_index) {
  return IndexExistsInLayer(gpWorldLevelData[grid_no].pStructHead, tile_index);
}

void HideStructOfGivenType(uint32_t const iMapIndex, uint32_t const fType, BOOLEAN const fHide) {
  if (fHide) {
    SetRoofIndexFlagsFromTypeRange(iMapIndex, fType, fType, LEVELNODE_HIDDEN);
  } else {
    // ONLY UNHIDE IF NOT REAVEALED ALREADY
    if (!(gpWorldLevelData[iMapIndex].uiFlags & MAPELEMENT_REVEALED)) {
      RemoveRoofIndexFlagsFromTypeRange(iMapIndex, fType, fType, LEVELNODE_HIDDEN);
    }
  }
}

// Shadow layer
// #################################################################

void AddShadowToTail(uint32_t const iMapIndex, uint16_t const usIndex) {
  LEVELNODE *const n = CreateLevelNode();
  n->usIndex = usIndex;

  // Append node to list
  LEVELNODE **anchor = &gpWorldLevelData[iMapIndex].pShadowHead;
  while (*anchor != NULL) anchor = &(*anchor)->pNext;
  *anchor = n;

  ResetSpecificLayerOptimizing(TILES_DYNAMIC_SHADOWS);
}

// Kris:  identical shadows can exist in the same gridno, though it makes no
// sense 		because it actually renders the shadows darker than the others.  This is
// an 	  undesirable effect with walls and buildings so I added this function to
// make 		sure there isn't already a shadow before placing it.
void AddExclusiveShadow(uint32_t iMapIndex, uint16_t usIndex) {
  for (LEVELNODE *pShadow = gpWorldLevelData[iMapIndex].pShadowHead; pShadow;
       pShadow = pShadow->pNext) {
    if (pShadow->usIndex == usIndex) return;
  }
  AddShadowToHead(iMapIndex, usIndex);
}

LEVELNODE *AddShadowToHead(const uint32_t iMapIndex, const uint16_t usIndex) {
  LEVELNODE *const n = CreateLevelNode();
  n->usIndex = usIndex;

  // Prepend node to list
  LEVELNODE **const head = &gpWorldLevelData[iMapIndex].pShadowHead;
  n->pNext = *head;
  *head = n;

  ResetSpecificLayerOptimizing(TILES_DYNAMIC_SHADOWS);
  return n;
}

static BOOLEAN RemoveShadow(uint32_t iMapIndex, uint16_t usIndex) {
  // Look through all shadows and remove index if found
  LEVELNODE *pOldShadow = NULL;
  for (LEVELNODE *pShadow = gpWorldLevelData[iMapIndex].pShadowHead; pShadow != NULL;
       pShadow = pShadow->pNext) {
    if (pShadow->usIndex == usIndex) {
      // OK, set links
      // Check for head
      if (pOldShadow == NULL) {
        // It's the head
        gpWorldLevelData[iMapIndex].pShadowHead = pShadow->pNext;
      } else {
        pOldShadow->pNext = pShadow->pNext;
      }

      MemFree(pShadow);
      return TRUE;
    }

    pOldShadow = pShadow;
  }

  // Could not find it
  return FALSE;
}

BOOLEAN RemoveShadowFromLevelNode(uint32_t iMapIndex, LEVELNODE *pNode) {
  LEVELNODE *pOldShadow = NULL;
  for (LEVELNODE *pShadow = gpWorldLevelData[iMapIndex].pShadowHead; pShadow != NULL;
       pShadow = pShadow->pNext) {
    if (pShadow == pNode) {
      // OK, set links
      // Check for head
      if (pOldShadow == NULL) {
        // It's the head
        gpWorldLevelData[iMapIndex].pShadowHead = pShadow->pNext;
      } else {
        pOldShadow->pNext = pShadow->pNext;
      }

      MemFree(pShadow);
      return TRUE;
    }

    pOldShadow = pShadow;
  }

  // Could not find it
  return FALSE;
}

BOOLEAN RemoveAllShadowsOfTypeRange(uint32_t iMapIndex, uint32_t fStartType, uint32_t fEndType) {
  BOOLEAN fRetVal = FALSE;

  // Look through all shadows and Search for type
  for (const LEVELNODE *pShadow = gpWorldLevelData[iMapIndex].pShadowHead; pShadow != NULL;) {
    if (pShadow->usIndex != NO_TILE) {
      const uint32_t fTileType = GetTileType(pShadow->usIndex);

      // Advance to next
      const LEVELNODE *pOldShadow = pShadow;
      pShadow = pShadow->pNext;

      if (fTileType >= fStartType && fTileType <= fEndType) {
        RemoveShadow(iMapIndex, pOldShadow->usIndex);
        fRetVal = TRUE;
      }
    }
  }
  return fRetVal;
}

BOOLEAN RemoveAllShadows(uint32_t iMapIndex) {
  BOOLEAN fRetVal = FALSE;

  for (LEVELNODE *pShadow = gpWorldLevelData[iMapIndex].pShadowHead; pShadow != NULL;) {
    if (pShadow->usIndex != NO_TILE) {
      // Advance to next
      const LEVELNODE *pOldShadow = pShadow;
      pShadow = pShadow->pNext;

      RemoveShadow(iMapIndex, pOldShadow->usIndex);
      fRetVal = TRUE;
    }
  }
  return fRetVal;
}

// Merc layer
// #################################################################

static void AddMercStructureInfo(int16_t sGridNo, SOLDIERTYPE *pSoldier);

LEVELNODE *AddMercToHead(uint32_t const iMapIndex, SOLDIERTYPE &s, BOOLEAN const fAddStructInfo) {
  LEVELNODE *pMerc = gpWorldLevelData[iMapIndex].pMercHead;

  LEVELNODE *pNextMerc = CreateLevelNode();
  pNextMerc->pNext = pMerc;
  pNextMerc->pSoldier = &s;
  pNextMerc->uiFlags |= LEVELNODE_SOLDIER;

  // Add structure info if we want
  if (fAddStructInfo) {
    // Set soldier's levelnode
    s.pLevelNode = pNextMerc;
    AddMercStructureInfo(iMapIndex, &s);
  }

  gpWorldLevelData[iMapIndex].pMercHead = pNextMerc;

  ResetSpecificLayerOptimizing(TILES_DYNAMIC_MERCS | TILES_DYNAMIC_STRUCT_MERCS |
                               TILES_DYNAMIC_HIGHMERCS);
  return pNextMerc;
}

static void AddMercStructureInfo(int16_t sGridNo, SOLDIERTYPE *pSoldier) {
  uint16_t const usAnimSurface = GetSoldierAnimationSurface(pSoldier);
  AddMercStructureInfoFromAnimSurface(sGridNo, pSoldier, usAnimSurface, pSoldier->usAnimState);
}

BOOLEAN AddMercStructureInfoFromAnimSurface(const int16_t sGridNo, SOLDIERTYPE *const s,
                                            const uint16_t usAnimSurface,
                                            const uint16_t usAnimState) {
  s->uiStatusFlags &= ~SOLDIER_MULTITILE;

  LEVELNODE *const n = s->pLevelNode;
  if (n == NULL || usAnimSurface == INVALID_ANIMATION_SURFACE) return FALSE;

  // Remove existing structs
  DeleteStructureFromWorld(n->pStructureData);
  n->pStructureData = NULL;

  const STRUCTURE_FILE_REF *const sfr = GetAnimationStructureRef(s, usAnimSurface, usAnimState);
  if (sfr == NULL) return TRUE;  // XXX why TRUE?

  const DB_STRUCTURE_REF *const sr = s->ubBodyType == QUEENMONSTER
                                         ?  // Queen uses only one direction
                                         &sfr->pDBStructureRef[0]
                                         : &sfr->pDBStructureRef[OneCDirection(s->bDirection)];

  bool const success = AddStructureToWorld(sGridNo, s->bLevel, sr, n);
  if (!success) {
    ScreenMsg(MSG_FONT_RED, MSG_DEBUG,
              L"FAILED: add struct info for merc %d (%ls), at %d direction %d", s->ubID, s->name,
              sGridNo, s->bDirection);
  }

  // Turn on if we are multi-tiled
  if (sr->pDBStructure->ubNumberOfTiles > 1) s->uiStatusFlags |= SOLDIER_MULTITILE;

  return success;
}

BOOLEAN OKToAddMercToWorld(SOLDIERTYPE *pSoldier, int8_t bDirection) {
  // if (pSoldier->uiStatusFlags & SOLDIER_MULTITILE)
  {
    // Get surface data
    uint16_t const usAnimSurface = GetSoldierAnimationSurface(pSoldier);
    if (usAnimSurface == INVALID_ANIMATION_SURFACE) {
      return FALSE;
    }

    // Now check if we have multi-tile info!
    const STRUCTURE_FILE_REF *const pStructFileRef =
        GetAnimationStructureRef(pSoldier, usAnimSurface, pSoldier->usAnimState);
    if (pStructFileRef != NULL) {
      // Try adding struct to this location, if we can it's good!
      uint16_t usOKToAddStructID;
      if (pSoldier->pLevelNode && pSoldier->pLevelNode->pStructureData != NULL) {
        usOKToAddStructID = pSoldier->pLevelNode->pStructureData->usStructureID;
      } else {
        usOKToAddStructID = INVALID_STRUCTURE_ID;
      }

      if (!OkayToAddStructureToWorld(pSoldier->sGridNo, pSoldier->bLevel,
                                     &pStructFileRef->pDBStructureRef[OneCDirection(bDirection)],
                                     usOKToAddStructID)) {
        return FALSE;
      }
    }
  }

  return TRUE;
}

BOOLEAN UpdateMercStructureInfo(SOLDIERTYPE *pSoldier) {
  if (pSoldier->pLevelNode == NULL) {
    return FALSE;
  }

  AddMercStructureInfo(pSoldier->sGridNo, pSoldier);
  return TRUE;
}

void RemoveMerc(uint32_t const map_idx, SOLDIERTYPE &s, bool const placeholder) {
  if (map_idx == NOWHERE) return;  // XXX exception?

  for (LEVELNODE **anchor = &gpWorldLevelData[map_idx].pMercHead;; anchor = &(*anchor)->pNext) {
    LEVELNODE *const merc = *anchor;
    if (!merc) break;

    if (merc->pSoldier != &s) continue;
    if (placeholder ^ ((merc->uiFlags & LEVELNODE_MERCPLACEHOLDER) != 0)) continue;

    *anchor = merc->pNext;

    if (!placeholder) {
      s.pLevelNode = 0;
      DeleteStructureFromWorld(merc->pStructureData);
    }

    MemFree(merc);
    break;
  }
  // XXX exception?
}

// Roof layer
// #################################################################

static LEVELNODE *AddRoof(const uint32_t iMapIndex, const uint16_t usIndex) {
  LEVELNODE *const n = AddNodeToWorld(iMapIndex, usIndex, 1);
  ResetSpecificLayerOptimizing(TILES_DYNAMIC_ROOF);
  return n;
}

LEVELNODE *AddRoofToTail(const uint32_t iMapIndex, const uint16_t usIndex) {
  LEVELNODE *const n = AddRoof(iMapIndex, usIndex);

  // Append node to list
  LEVELNODE **anchor = &gpWorldLevelData[iMapIndex].pRoofHead;
  while (*anchor != NULL) anchor = &(*anchor)->pNext;
  *anchor = n;

  return n;
}

LEVELNODE *AddRoofToHead(const uint32_t iMapIndex, const uint16_t usIndex) {
  LEVELNODE *const n = AddRoof(iMapIndex, usIndex);

  // Prepend node to list
  LEVELNODE **const head = &gpWorldLevelData[iMapIndex].pRoofHead;
  n->pNext = *head;
  *head = n;

  return n;
}

BOOLEAN RemoveRoof(uint32_t iMapIndex, uint16_t usIndex) {
  // Look through all Roofs and remove index if found
  LEVELNODE *pOldRoof = NULL;
  for (LEVELNODE *pRoof = gpWorldLevelData[iMapIndex].pRoofHead; pRoof != NULL;
       pRoof = pRoof->pNext) {
    if (pRoof->usIndex == usIndex) {
      // OK, set links
      // Check for head
      if (pOldRoof == NULL) {
        // It's the head
        gpWorldLevelData[iMapIndex].pRoofHead = pRoof->pNext;
      } else {
        pOldRoof->pNext = pRoof->pNext;
      }

      DeleteStructureFromWorld(pRoof->pStructureData);
      MemFree(pRoof);
      return TRUE;
    }

    pOldRoof = pRoof;
  }

  // Could not find it
  return FALSE;
}

LEVELNODE *FindTypeInRoofLayer(uint32_t const map_idx, uint32_t const type) {
  return FindTypeInLayer(gpWorldLevelData[map_idx].pRoofHead, type);
}

LEVELNODE *TypeRangeExistsInRoofLayer(uint32_t iMapIndex, uint32_t fStartType, uint32_t fEndType) {
  // Look through all objects and Search for type
  for (LEVELNODE *pRoof = gpWorldLevelData[iMapIndex].pRoofHead; pRoof;) {
    if (pRoof->usIndex != NO_TILE) {
      const uint32_t fTileType = GetTileType(pRoof->usIndex);
      if (fStartType <= fTileType && fTileType <= fEndType) {
        return pRoof;
      }
      pRoof = pRoof->pNext;  // XXX TODO0009 if pRoof->usIndex == NO_TILE this is
                             // an endless loop
    }
  }

  // Could not find it
  return 0;
}

BOOLEAN IndexExistsInRoofLayer(int16_t const sGridNo, uint16_t const usIndex) {
  return IndexExistsInLayer(gpWorldLevelData[sGridNo].pRoofHead, usIndex);
}

BOOLEAN RemoveAllRoofsOfTypeRange(uint32_t iMapIndex, uint32_t fStartType, uint32_t fEndType) {
  BOOLEAN fRetVal = FALSE;

  // Look through all Roofs and Search for type
  for (const LEVELNODE *pRoof = gpWorldLevelData[iMapIndex].pRoofHead; pRoof != NULL;) {
    if (pRoof->usIndex != NO_TILE) {
      const uint32_t fTileType = GetTileType(pRoof->usIndex);

      // Advance to next
      const LEVELNODE *pOldRoof = pRoof;
      pRoof = pRoof->pNext;  // XXX TODO0009 if pRoof->usIndex == NO_TILE this is
                             // an endless loop

      if (fTileType >= fStartType && fTileType <= fEndType) {
        RemoveRoof(iMapIndex, pOldRoof->usIndex);
        fRetVal = TRUE;
      }
    }
  }

  // Could not find it
  return fRetVal;
}

void RemoveRoofIndexFlagsFromTypeRange(uint32_t const iMapIndex, uint32_t const fStartType,
                                       uint32_t const fEndType, LevelnodeFlags const uiFlags) {
  // Look through all Roofs and Search for type
  for (LEVELNODE *pRoof = gpWorldLevelData[iMapIndex].pRoofHead; pRoof != NULL;) {
    if (pRoof->usIndex != NO_TILE) {
      const uint32_t fTileType = GetTileType(pRoof->usIndex);
      if (fTileType >= fStartType && fTileType <= fEndType) {
        pRoof->uiFlags &= ~uiFlags;
      }
      pRoof = pRoof->pNext;  // XXX TODO0009 if pRoof->usIndex == NO_TILE this is
                             // an endless loop
    }
  }
}

void SetRoofIndexFlagsFromTypeRange(uint32_t const iMapIndex, uint32_t const fStartType,
                                    uint32_t const fEndType, LevelnodeFlags const uiFlags) {
  // Look through all Roofs and Search for type
  for (LEVELNODE *pRoof = gpWorldLevelData[iMapIndex].pRoofHead; pRoof != NULL;) {
    if (pRoof->usIndex != NO_TILE) {
      const uint32_t fTileType = GetTileType(pRoof->usIndex);
      if (fTileType >= fStartType && fTileType <= fEndType) {
        pRoof->uiFlags |= uiFlags;
      }
      pRoof = pRoof->pNext;  // XXX TODO0009 if pRoof->usIndex == NO_TILE this is
                             // an endless loop
    }
  }
}

// OnRoof layer
// #################################################################

static LEVELNODE *AddOnRoof(const uint32_t iMapIndex, const uint16_t usIndex) {
  LEVELNODE *const n = AddNodeToWorld(iMapIndex, usIndex, 1);
  ResetSpecificLayerOptimizing(TILES_DYNAMIC_ONROOF);
  return n;
}

LEVELNODE *AddOnRoofToTail(const uint32_t iMapIndex, const uint16_t usIndex) {
  LEVELNODE *const n = AddOnRoof(iMapIndex, usIndex);

  // Append the node to the list
  LEVELNODE **anchor = &gpWorldLevelData[iMapIndex].pOnRoofHead;
  while (*anchor != NULL) anchor = &(*anchor)->pNext;
  *anchor = n;

  return n;
}

LEVELNODE *AddOnRoofToHead(const uint32_t iMapIndex, const uint16_t usIndex) {
  LEVELNODE *const n = AddOnRoof(iMapIndex, usIndex);

  // Prepend the node to the list
  LEVELNODE **const head = &gpWorldLevelData[iMapIndex].pOnRoofHead;
  n->pNext = *head;
  *head = n;

  return n;
}

BOOLEAN RemoveOnRoof(uint32_t iMapIndex, uint16_t usIndex) {
  LEVELNODE *pOldOnRoof = NULL;

  // Look through all OnRoofs and remove index if found
  for (LEVELNODE *pOnRoof = gpWorldLevelData[iMapIndex].pOnRoofHead; pOnRoof != NULL;
       pOnRoof = pOnRoof->pNext) {
    if (pOnRoof->usIndex == usIndex) {
      // OK, set links
      // Check for head
      if (pOldOnRoof == NULL) {
        // It's the head
        gpWorldLevelData[iMapIndex].pOnRoofHead = pOnRoof->pNext;
      } else {
        pOldOnRoof->pNext = pOnRoof->pNext;
      }

      MemFree(pOnRoof);
      return TRUE;
    }

    pOldOnRoof = pOnRoof;
  }

  // Could not find it
  return FALSE;
}

BOOLEAN RemoveOnRoofFromLevelNode(uint32_t iMapIndex, LEVELNODE *pNode) {
  LEVELNODE *pOldOnRoof = NULL;

  for (LEVELNODE *pOnRoof = gpWorldLevelData[iMapIndex].pOnRoofHead; pOnRoof != NULL;
       pOnRoof = pOnRoof->pNext) {
    if (pOnRoof == pNode) {
      // OK, set links
      // Check for head
      if (pOldOnRoof == NULL) {
        // It's the head
        gpWorldLevelData[iMapIndex].pOnRoofHead = pOnRoof->pNext;
      } else {
        pOldOnRoof->pNext = pOnRoof->pNext;
      }

      MemFree(pOnRoof);
      return TRUE;
    }

    pOldOnRoof = pOnRoof;
  }

  // Could not find it
  return FALSE;
}

BOOLEAN RemoveAllOnRoofsOfTypeRange(uint32_t iMapIndex, uint32_t fStartType, uint32_t fEndType) {
  BOOLEAN fRetVal = FALSE;

  // Look through all OnRoofs and Search for type
  for (const LEVELNODE *pOnRoof = gpWorldLevelData[iMapIndex].pOnRoofHead; pOnRoof != NULL;) {
    if (pOnRoof->uiFlags & LEVELNODE_CACHEDANITILE) {
      pOnRoof = pOnRoof->pNext;
      continue;
    }

    if (pOnRoof->usIndex != NO_TILE) {
      const uint32_t fTileType = GetTileType(pOnRoof->usIndex);

      // Advance to next
      const LEVELNODE *pOldOnRoof = pOnRoof;
      pOnRoof = pOnRoof->pNext;  // XXX TODO0009 if pOnRoof->usIndex == NO_TILE
                                 // this is an endless loop

      if (fTileType >= fStartType && fTileType <= fEndType) {
        RemoveOnRoof(iMapIndex, pOldOnRoof->usIndex);
        fRetVal = TRUE;
      }
    }
  }
  return fRetVal;
}

// Topmost layer
// #################################################################

LEVELNODE *AddTopmostToTail(const uint32_t iMapIndex, const uint16_t usIndex) {
  LEVELNODE *const n = CreateLevelNode();
  n->usIndex = usIndex;

  // Append node to list
  LEVELNODE **anchor = &gpWorldLevelData[iMapIndex].pTopmostHead;
  while (*anchor != NULL) anchor = &(*anchor)->pNext;
  *anchor = n;

  ResetSpecificLayerOptimizing(TILES_DYNAMIC_TOPMOST);
  return n;
}

LEVELNODE *AddUIElem(uint32_t iMapIndex, uint16_t usIndex, int8_t sRelativeX, int8_t sRelativeY) {
  LEVELNODE *pTopmost = AddTopmostToTail(iMapIndex, usIndex);

  // Set flags
  pTopmost->uiFlags |= LEVELNODE_USERELPOS;
  pTopmost->sRelativeX = sRelativeX;
  pTopmost->sRelativeY = sRelativeY;

  ResetSpecificLayerOptimizing(TILES_DYNAMIC_TOPMOST);
  return pTopmost;
}

LEVELNODE *AddTopmostToHead(const uint32_t iMapIndex, const uint16_t usIndex) {
  LEVELNODE *const n = CreateLevelNode();
  n->usIndex = usIndex;

  // Prepend node to list
  LEVELNODE **const head = &gpWorldLevelData[iMapIndex].pTopmostHead;
  n->pNext = *head;
  *head = n;

  ResetSpecificLayerOptimizing(TILES_DYNAMIC_TOPMOST);
  return n;
}

BOOLEAN RemoveTopmost(uint32_t iMapIndex, uint16_t usIndex) {
  // Look through all topmosts and remove index if found
  LEVELNODE *pOldTopmost = NULL;
  for (LEVELNODE *pTopmost = gpWorldLevelData[iMapIndex].pTopmostHead; pTopmost != NULL;
       pTopmost = pTopmost->pNext) {
    if (pTopmost->usIndex == usIndex) {
      // OK, set links
      // Check for head
      if (pOldTopmost == NULL) {
        // It's the head
        gpWorldLevelData[iMapIndex].pTopmostHead = pTopmost->pNext;
      } else {
        pOldTopmost->pNext = pTopmost->pNext;
      }

      MemFree(pTopmost);
      return TRUE;
    }

    pOldTopmost = pTopmost;
  }

  // Could not find it
  return FALSE;
}

BOOLEAN RemoveTopmostFromLevelNode(uint32_t iMapIndex, LEVELNODE *pNode) {
  // Look through all topmosts and remove index if found
  LEVELNODE *pOldTopmost = NULL;
  for (LEVELNODE *pTopmost = gpWorldLevelData[iMapIndex].pTopmostHead; pTopmost != NULL;
       pTopmost = pTopmost->pNext) {
    if (pTopmost == pNode) {
      // OK, set links
      // Check for head or tail
      if (pOldTopmost == NULL) {
        // It's the head
        gpWorldLevelData[iMapIndex].pTopmostHead = pTopmost->pNext;
      } else {
        pOldTopmost->pNext = pTopmost->pNext;
      }

      MemFree(pTopmost);
      return TRUE;
    }

    pOldTopmost = pTopmost;
  }

  // Could not find it
  return FALSE;
}

BOOLEAN RemoveAllTopmostsOfTypeRange(uint32_t iMapIndex, uint32_t fStartType, uint32_t fEndType) {
  BOOLEAN fRetVal = FALSE;

  // Look through all topmosts and Search for type
  for (const LEVELNODE *pTopmost = gpWorldLevelData[iMapIndex].pTopmostHead; pTopmost != NULL;) {
    const LEVELNODE *pOldTopmost = pTopmost;
    pTopmost = pTopmost->pNext;

    if (pOldTopmost->usIndex != NO_TILE && pOldTopmost->usIndex < NUMBEROFTILES) {
      const uint32_t fTileType = GetTileType(pOldTopmost->usIndex);
      if (fTileType >= fStartType && fTileType <= fEndType) {
        // Remove Item
        RemoveTopmost(iMapIndex, pOldTopmost->usIndex);
        fRetVal = TRUE;
      }
    }
  }
  return fRetVal;
}

LEVELNODE *FindTypeInTopmostLayer(uint32_t const map_idx, uint32_t const type) {
  return FindTypeInLayer(gpWorldLevelData[map_idx].pTopmostHead, type);
}

BOOLEAN IsHeigherLevel(int16_t sGridNo) {
  const STRUCTURE *pStructure = FindStructure(sGridNo, STRUCTURE_NORMAL_ROOF);
  return pStructure != NULL;
}

BOOLEAN IsRoofVisible(int16_t sMapPos) {
  if (gfBasement) return TRUE;

  const STRUCTURE *pStructure = FindStructure(sMapPos, STRUCTURE_ROOF);
  return pStructure != NULL && !(gpWorldLevelData[sMapPos].uiFlags & MAPELEMENT_REVEALED);
}

BOOLEAN IsRoofVisible2(int16_t sMapPos) {
  if (!gfBasement) {
    const STRUCTURE *pStructure = FindStructure(sMapPos, STRUCTURE_ROOF);
    if (pStructure == NULL) return FALSE;
  }

  return !(gpWorldLevelData[sMapPos].uiFlags & MAPELEMENT_REVEALED);
}

SOLDIERTYPE *WhoIsThere2(int16_t const gridno, int8_t const level) {
  if (!GridNoOnVisibleWorldTile(gridno)) return NULL;

  for (STRUCTURE const *structure = gpWorldLevelData[gridno].pStructureHead; structure;
       structure = structure->pNext) {
    if (!(structure->fFlags & STRUCTURE_PERSON)) continue;

    SOLDIERTYPE &tgt = GetMan(structure->usStructureID);
    // person must either have their pSoldier->sGridNo here or be non-passable
    if (structure->fFlags & STRUCTURE_PASSABLE && tgt.sGridNo != gridno) continue;

    if ((level == 0 && structure->sCubeOffset == 0) || (level > 0 && structure->sCubeOffset > 0)) {
      // found a person, on the right level!
      // structure ID and merc ID are identical for merc structures
      return &tgt;
    }
  }

  return NULL;
}

uint8_t GetTerrainType(GridNo const grid_no) { return gpWorldLevelData[grid_no].ubTerrainID; }

bool Water(GridNo const grid_no) {
  if (grid_no == NOWHERE) return false;

  uint8_t const terrain = GetTerrainType(grid_no);
  return terrain == LOW_WATER || terrain == MED_WATER || terrain == DEEP_WATER;
}

bool DeepWater(GridNo const grid_no) { return GetTerrainType(grid_no) == DEEP_WATER; }

bool WaterTooDeepForAttacks(GridNo const grid_no) { return DeepWater(grid_no); }

void SetStructAframeFlags(uint32_t const iMapIndex, LevelnodeFlags const uiFlags) {
  // Look through all Roofs and Search for type
  for (LEVELNODE *pStruct = gpWorldLevelData[iMapIndex].pRoofHead; pStruct != NULL;) {
    if (pStruct->usIndex != NO_TILE) {
      if (GetTileFlags(pStruct->usIndex) & AFRAME_TILE) {
        pStruct->uiFlags |= uiFlags;
      }
      pStruct = pStruct->pNext;  // XXX TODO0009 if pStruct->usIndex == NO_TILE
                                 // this is an endless loop
    }
  }
}

LEVELNODE *FindLevelNodeBasedOnStructure(STRUCTURE const *const s) {
  Assert(s->fFlags & STRUCTURE_BASE_TILE);
  MAP_ELEMENT const &me = gpWorldLevelData[s->sGridNo];

  // ATE: First look on the struct layer
  for (LEVELNODE *i = me.pStructHead; i; i = i->pNext) {
    if (i->pStructureData == s) return i;
  }

  // Next the roof layer
  for (LEVELNODE *i = me.pRoofHead; i; i = i->pNext) {
    if (i->pStructureData == s) return i;
  }

  // Then the object layer
  for (LEVELNODE *i = me.pObjectHead; i; i = i->pNext) {
    if (i->pStructureData == s) return i;
  }

  // Finally the onroof layer
  for (LEVELNODE *i = me.pOnRoofHead; i; i = i->pNext) {
    if (i->pStructureData == s) return i;
  }

  throw std::logic_error("FindLevelNodeBasedOnStruct failed");
}

LEVELNODE *FindShadow(int16_t sGridNo, uint16_t usStructIndex) {
  if (usStructIndex < FIRSTOSTRUCT1 || usStructIndex >= FIRSTSHADOW1) {
    return NULL;
  }

  uint16_t usShadowIndex = usStructIndex - FIRSTOSTRUCT1 + FIRSTSHADOW1;
  LEVELNODE *pLevelNode;
  for (pLevelNode = gpWorldLevelData[sGridNo].pShadowHead; pLevelNode != NULL;
       pLevelNode = pLevelNode->pNext) {
    if (pLevelNode->usIndex == usShadowIndex) {
      break;
    }
  }
  return pLevelNode;
}

void WorldHideTrees() {
  FOR_EACH_WORLD_TILE(i) {
    for (LEVELNODE *pNode = i->pStructHead; pNode != NULL; pNode = pNode->pNext) {
      if (pNode->uiFlags & LEVELNODE_ANIMATION) continue;
      if (GetTileFlags(pNode->usIndex) & FULL3D_TILE) {
        pNode->uiFlags |= LEVELNODE_REVEALTREES;
      }
    }
  }

  SetRenderFlags(RENDER_FLAG_FULL);
}

void WorldShowTrees() {
  FOR_EACH_WORLD_TILE(i) {
    for (LEVELNODE *pNode = i->pStructHead; pNode != NULL; pNode = pNode->pNext) {
      if (pNode->uiFlags & LEVELNODE_ANIMATION) continue;
      if (GetTileFlags(pNode->usIndex) & FULL3D_TILE) {
        pNode->uiFlags &= ~LEVELNODE_REVEALTREES;
      }
    }
  }

  SetRenderFlags(RENDER_FLAG_FULL);
}

void SetWallLevelnodeFlags(uint16_t const sGridNo, LevelnodeFlags const uiFlags) {
  for (LEVELNODE *pStruct = gpWorldLevelData[sGridNo].pStructHead; pStruct != NULL;
       pStruct = pStruct->pNext) {
    if (pStruct->pStructureData != NULL &&
        pStruct->pStructureData->fFlags & STRUCTURE_WALLSTUFF)  // See if we are a wall!
    {
      pStruct->uiFlags |= uiFlags;
    }
  }
}

void RemoveWallLevelnodeFlags(uint16_t const sGridNo, LevelnodeFlags const uiFlags) {
  for (LEVELNODE *pStruct = gpWorldLevelData[sGridNo].pStructHead; pStruct != NULL;
       pStruct = pStruct->pNext) {
    if (pStruct->pStructureData != NULL &&
        pStruct->pStructureData->fFlags & STRUCTURE_WALLSTUFF)  // See if we are a wall!
    {
      pStruct->uiFlags &= ~uiFlags;
    }
  }
}

void SetTreeTopStateForMap() {
  if (!gGameSettings.fOptions[TOPTION_TOGGLE_TREE_TOPS]) {
    WorldHideTrees();
    gTacticalStatus.uiFlags |= NOHIDE_REDUNDENCY;
  } else {
    WorldShowTrees();
    gTacticalStatus.uiFlags &= ~NOHIDE_REDUNDENCY;
  }

  // FOR THE NEXT RENDER LOOP, RE-EVALUATE REDUNDENT TILES
  InvalidateWorldRedundency();
}
