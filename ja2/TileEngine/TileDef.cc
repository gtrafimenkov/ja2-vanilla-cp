// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "TileEngine/TileDef.h"

#include <stdexcept>
#include <string.h>

#include "Editor/EditSys.h"
#include "Macro.h"
#include "SGP/Debug.h"
#include "SGP/HImage.h"
#include "SGP/MemMan.h"
#include "SGP/VObject.h"
#include "Tactical/PathAI.h"
#include "TileEngine/Structure.h"
#include "TileEngine/TileSurface.h"
#include "TileEngine/WorldDef.h"
#include "TileEngine/WorldMan.h"

// GLobals
TILE_ELEMENT gTileDatabase[NUMBEROFTILES];
static uint16_t gTileDatabaseSize;
uint16_t gusNumAnimatedTiles = 0;
uint16_t gusAnimatedTiles[MAX_ANIMATED_TILES];

uint16_t gTileTypeStartIndex[NUMBEROFTILETYPES];

// These values coorespond to TerrainTypeDefines order
uint8_t gTileTypeMovementCost[NUM_TERRAIN_TYPES] = {
    TRAVELCOST_FLAT,         // NO_TERRAIN
    TRAVELCOST_FLAT,         // FLAT GROUND
    TRAVELCOST_FLATFLOOR,    // FLAT FLOOR
    TRAVELCOST_PAVEDROAD,    // PAVED ROAD
    TRAVELCOST_DIRTROAD,     // DIRT ROAD
    TRAVELCOST_FLAT,         // LOW_GRASS
    TRAVELCOST_FLAT,         // HIGH GRASS
    TRAVELCOST_TRAINTRACKS,  // TRAIN TRACKS
    TRAVELCOST_SHORE,        // LOW WATER
    TRAVELCOST_SHORE,        // MED WATER
    TRAVELCOST_SHORE,        // DEEP WATER
};

void CreateTileDatabase() {
  // Loop through all surfaces and tiles and build database
  for (uint32_t cnt1 = 0; cnt1 < NUMBEROFTILETYPES; ++cnt1) {
    TILE_IMAGERY const *const TileSurf = gTileSurfaceArray[cnt1];
    if (!TileSurf) continue;

    // Build start index list
    gTileTypeStartIndex[cnt1] = (uint16_t)gTileDatabaseSize;

    uint32_t NumRegions = TileSurf->vo->SubregionCount();

    // Handle overflow
    if (NumRegions > gNumTilesPerType[cnt1]) {
      NumRegions = gNumTilesPerType[cnt1];
    }

    DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
             String("Type: %s Size: %d Index: %d", gTileSurfaceName[cnt1], gNumTilesPerType[cnt1],
                    gTileDatabaseSize));

    uint32_t cnt2;
    for (cnt2 = 0; cnt2 < NumRegions; ++cnt2) {
      TILE_ELEMENT TileElement;
      memset(&TileElement, 0, sizeof(TileElement));
      TileElement.usRegionIndex = (uint16_t)cnt2;
      TileElement.hTileSurface = TileSurf->vo;
      TileElement.sBuddyNum = -1;

      // Check for multi-z stuff
      ZStripInfo *const *const zsi = TileSurf->vo->ppZStripInfo;
      if (zsi && zsi[cnt2]) TileElement.uiFlags |= MULTI_Z_TILE;

      // Structure database stuff!
      STRUCTURE_FILE_REF const *const sfr = TileSurf->pStructureFileRef;
      if (sfr && sfr->pubStructureData /* XXX testing wrong attribute? */) {
        DB_STRUCTURE_REF *const sr = &sfr->pDBStructureRef[cnt2];
        if (sr->pDBStructure) TileElement.pDBStructureRef = sr;
      }

      TileElement.fType = (uint16_t)TileSurf->fType;
      TileElement.ubTerrainID = TileSurf->ubTerrainID;
      TileElement.usWallOrientation = NO_ORIENTATION;

      if (TileSurf->pAuxData) {
        AuxObjectData const &aux = TileSurf->pAuxData[cnt2];
        if (aux.fFlags & AUX_FULL_TILE) {
          TileElement.ubFullTile = 1;
        }
        if (aux.fFlags & AUX_ANIMATED_TILE) {
          AllocateAnimTileData(&TileElement, aux.ubNumberOfFrames);

          TileElement.pAnimData->bCurrentFrame = aux.ubCurrentFrame;
          for (uint8_t ubLoop = 0; ubLoop < TileElement.pAnimData->ubNumFrames; ++ubLoop) {
            TileElement.pAnimData->pusFrames[ubLoop] =
                gTileDatabaseSize - TileElement.pAnimData->bCurrentFrame + ubLoop;
          }

          // set into animation controller array
          Assert(gusNumAnimatedTiles < lengthof(gusAnimatedTiles));
          gusAnimatedTiles[gusNumAnimatedTiles++] = gTileDatabaseSize;

          TileElement.uiFlags |= ANIMATED_TILE;
        }
        TileElement.usWallOrientation = aux.ubWallOrientation;
        if (aux.ubNumberOfTiles > 0) {
          TileElement.ubNumberOfTiles = aux.ubNumberOfTiles;
          TileElement.pTileLocData = TileSurf->pTileLocData + aux.usTileLocIndex;
        }
      }

      SetSpecificDatabaseValues(cnt1, gTileDatabaseSize, TileElement, TileSurf->bRaisedObjectType);

      gTileDatabase[gTileDatabaseSize++] = TileElement;
    }

    // Handle underflow
    for (; cnt2 < gNumTilesPerType[cnt1]; ++cnt2) {
      TILE_ELEMENT TileElement;
      memset(&TileElement, 0, sizeof(TileElement));
      TileElement.usRegionIndex = 0;
      TileElement.hTileSurface = TileSurf->vo;
      TileElement.fType = (uint16_t)TileSurf->fType;
      TileElement.ubFullTile = 0;
      TileElement.uiFlags |= UNDERFLOW_FILLER;

      gTileDatabase[gTileDatabaseSize++] = TileElement;
    }
  }

  // Calculate mem usgae
  DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
           String("Database Sizes: %d vs %d", gTileDatabaseSize, NUMBEROFTILES));
  DebugMsg(TOPIC_JA2, DBG_LEVEL_3, String("Database Types: %d", NUMBEROFTILETYPES));
  DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
           String("Database Item Mem:		%d", gTileDatabaseSize * sizeof(TILE_ELEMENT)));
}

static void FreeAnimTileData(TILE_ELEMENT *pTileElem);

void DeallocateTileDatabase() {
  int32_t cnt;

  for (cnt = 0; cnt < NUMBEROFTILES; cnt++) {
    // Check if an existing set of animated tiles are in place, remove if found
    if (gTileDatabase[cnt].pAnimData != NULL) {
      FreeAnimTileData(&gTileDatabase[cnt]);
    }
  }

  gTileDatabaseSize = 0;
  gusNumAnimatedTiles = 0;
}

void SetLandIndex(int32_t const iMapIndex, uint16_t const usIndex, uint32_t const uiNewType) {
  uint8_t ubLastHighLevel;
  if (LEVELNODE const *const land = FindTypeInLandLayer(iMapIndex, uiNewType)) {
    ReplaceLandIndex(iMapIndex, land->usIndex, usIndex);
  } else if (AnyHeigherLand(iMapIndex, uiNewType, &ubLastHighLevel)) {
    InsertLandIndexAtLevel(iMapIndex, usIndex, ubLastHighLevel + 1);
  } else {
    AddLandToHead(iMapIndex, usIndex);
  }
}

bool GetTypeLandLevel(uint32_t const map_idx, uint32_t const new_type, uint8_t *const out_level) {
  uint8_t level = 0;
  for (LEVELNODE *i = gpWorldLevelData[map_idx].pLandHead; i; ++level, i = i->pNext) {
    if (i->usIndex == NO_TILE) continue;
    if (GetTileType(i->usIndex) != new_type) continue;
    *out_level = level;
    return true;
  }
  return false;
}

uint16_t GetSubIndexFromTileIndex(const uint16_t usTileIndex) {
  const uint32_t uiType = GetTileType(usTileIndex);
  return usTileIndex - gTileTypeStartIndex[uiType] + 1;
}

uint16_t GetTypeSubIndexFromTileIndex(uint32_t const uiCheckType, uint16_t const usIndex) {
  // Tile database is zero-based, Type indecies are 1-based!
  if (uiCheckType >= NUMBEROFTILETYPES) {
    throw std::logic_error("Tried to get sub-index from invalid tile type");
  }
  return usIndex - gTileTypeStartIndex[uiCheckType] + 1;
}

uint16_t GetTileIndexFromTypeSubIndex(uint32_t uiCheckType, uint16_t usSubIndex) {
  Assert(uiCheckType < NUMBEROFTILETYPES);
  // Tile database is zero-based, Type indices are 1-based!
  return usSubIndex + gTileTypeStartIndex[uiCheckType] - 1;
}

// Database access functions
uint32_t GetTileType(const uint16_t usIndex) {
  Assert(usIndex < lengthof(gTileDatabase));
  return gTileDatabase[usIndex].fType;
}

uint32_t GetTileFlags(const uint16_t usIndex) {
  Assert(usIndex < lengthof(gTileDatabase));
  return gTileDatabase[usIndex].uiFlags;
}

uint8_t GetTileTypeLogicalHeight(uint32_t const type) {
  Assert(type < lengthof(gTileTypeLogicalHeight));
  return gTileTypeLogicalHeight[type];
}

bool AnyHeigherLand(uint32_t const map_idx, uint32_t const src_type,
                    uint8_t *const out_last_level) {
  // Check that src type is not head
  uint8_t src_type_level = 0;
  if (GetTypeLandLevel(map_idx, src_type, &src_type_level) && src_type_level == LANDHEAD) {
    return false;
  }

  uint8_t level = 0;
  bool found = false;
  uint8_t src_log_height = GetTileTypeLogicalHeight(src_type);
  for (LEVELNODE *i = gpWorldLevelData[map_idx].pLandHead; i; ++level, i = i->pNext) {
    // Get type and height
    uint32_t const tile_type = GetTileType(i->usIndex);
    if (GetTileTypeLogicalHeight(tile_type) > src_log_height) {
      *out_last_level = level;
      found = TRUE;
    }
  }
  return found;
}

static BOOLEAN AnyLowerLand(uint32_t iMapIndex, uint32_t uiSrcType, uint8_t *pubLastLevel) {
  LEVELNODE *pLand = NULL;
  uint8_t level = 0;
  uint8_t ubSrcTypeLevel;
  TILE_ELEMENT TileElem;

  pLand = gpWorldLevelData[iMapIndex].pLandHead;

  uint8_t ubSrcLogHeight = GetTileTypeLogicalHeight(uiSrcType);

  GetTypeLandLevel(iMapIndex, uiSrcType, &ubSrcTypeLevel);

  // Look through all objects and Search for type
  while (pLand != NULL) {
    // Get type and height
    const uint32_t fTileType = GetTileType(pLand->usIndex);

    if (GetTileTypeLogicalHeight(fTileType) < ubSrcLogHeight) {
      *pubLastLevel = level;
      return (TRUE);
    }

    // Get tile element
    TileElem = gTileDatabase[pLand->usIndex];

    // Get full tile flag
    if (TileElem.ubFullTile && fTileType != uiSrcType) {
      return (FALSE);
    }

    // Advance to next
    pLand = pLand->pNext;

    level++;
  }

  // Could not find it, return FALSE
  return (FALSE);
}

uint16_t GetWallOrientation(uint16_t usIndex) {
  Assert(usIndex < lengthof(gTileDatabase));
  return gTileDatabase[usIndex].usWallOrientation;
}

void AllocateAnimTileData(TILE_ELEMENT *const pTileElem, uint8_t const ubNumFrames) {
  pTileElem->pAnimData = MALLOC(TILE_ANIMATION_DATA);
  pTileElem->pAnimData->pusFrames = MALLOCN(uint16_t, ubNumFrames);

  // Set # if frames!
  pTileElem->pAnimData->ubNumFrames = ubNumFrames;
}

static void FreeAnimTileData(TILE_ELEMENT *pTileElem) {
  if (pTileElem->pAnimData != NULL) {
    // Free frames list
    MemFree(pTileElem->pAnimData->pusFrames);

    // Free frames
    MemFree(pTileElem->pAnimData);
  }
}
