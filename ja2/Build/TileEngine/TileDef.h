#ifndef __TILEDEF_H
#define __TILEDEF_H

#include "JA2Types.h"
#include "TileEngine/TileDat.h"

// CATEGORY TYPES
#define NO_TILE 64000
#define REQUIRES_SMOOTHING_TILE 19

enum TileElementFlags {
  WALL_TILE = 1U << 0,
  ANIMATED_TILE = 1U << 1,
  DYNAMIC_TILE = 1U << 2,
  IGNORE_WORLD_HEIGHT = 1U << 3,
  FULL3D_TILE = 1U << 4,
  MULTI_Z_TILE = 1U << 5,
  OBJECTLAYER_USEZHEIGHT = 1U << 6,
  ROOFSHADOW_TILE = 1U << 7,
  ROOF_TILE = 1U << 8,
  HAS_SHADOW_BUDDY = 1U << 9,
  AFRAME_TILE = 1U << 10,
  CLIFFHANG_TILE = 1U << 11,
  UNDERFLOW_FILLER = 1U << 12
};
ENUM_BITSET(TileElementFlags)

#define MAX_ANIMATED_TILES 200
#define WALL_HEIGHT 50

enum WallOrientationDefines {
  NO_ORIENTATION,
  INSIDE_TOP_LEFT,
  INSIDE_TOP_RIGHT,
  OUTSIDE_TOP_LEFT,
  OUTSIDE_TOP_RIGHT
};

// TERRAIN ID VALUES.
enum TerrainTypeDefines {
  NO_TERRAIN,
  FLAT_GROUND,
  FLAT_FLOOR,
  PAVED_ROAD,
  DIRT_ROAD,
  LOW_GRASS,
  HIGH_GRASS,
  TRAIN_TRACKS,
  LOW_WATER,
  MED_WATER,
  DEEP_WATER,
  NUM_TERRAIN_TYPES
};

// These structures are placed in a list and used for all tile imagery
struct TILE_IMAGERY {
  HVOBJECT vo;
  uint32_t fType;
  AuxObjectData *pAuxData;
  RelTileLoc *pTileLocData;
  STRUCTURE_FILE_REF *pStructureFileRef;
  uint8_t ubTerrainID;
  uint8_t bRaisedObjectType;

  // Reserved for added room and 32-byte boundaries
  uint8_t bReserved[2];
};

struct TILE_ANIMATION_DATA {
  uint16_t *pusFrames;
  int8_t bCurrentFrame;
  uint8_t ubNumFrames;
};

// Tile data element
struct TILE_ELEMENT {
  HVOBJECT hTileSurface;
  DB_STRUCTURE_REF *pDBStructureRef;
  RelTileLoc *pTileLocData;
  TileElementFlags uiFlags;
  uint16_t fType;
  uint16_t usRegionIndex;
  int16_t sBuddyNum;
  uint8_t ubTerrainID;
  uint8_t ubNumberOfTiles;

  // Land and overlay type
  uint16_t usWallOrientation;
  uint8_t ubFullTile;

  // For animated tiles
  TILE_ANIMATION_DATA *pAnimData;
};

// Globals used
extern TILE_ELEMENT gTileDatabase[NUMBEROFTILES];
extern uint16_t gTileTypeStartIndex[NUMBEROFTILETYPES];

static inline const TILE_ELEMENT *TileElemFromTileType(const uint16_t tile_type) {
  return &gTileDatabase[gTileTypeStartIndex[tile_type]];
}

extern uint16_t gusNumAnimatedTiles;
extern uint16_t gusAnimatedTiles[MAX_ANIMATED_TILES];
extern uint8_t gTileTypeMovementCost[NUM_TERRAIN_TYPES];

void CreateTileDatabase();

// Land level manipulation functions

void SetLandIndex(int32_t iMapIndex, uint16_t usIndex, uint32_t uiNewType);

bool GetTypeLandLevel(uint32_t map_idx, uint32_t new_type, uint8_t *out_level);

// Database access functions
uint16_t GetSubIndexFromTileIndex(uint16_t usIndex);

uint16_t GetTypeSubIndexFromTileIndex(uint32_t uiCheckType, uint16_t usIndex);

uint16_t GetTileIndexFromTypeSubIndex(uint32_t uiCheckType, uint16_t usSubIndex);
uint32_t GetTileType(uint16_t usIndex);
uint32_t GetTileFlags(uint16_t usIndex);

uint8_t GetTileTypeLogicalHeight(uint32_t type);
bool AnyHeigherLand(uint32_t map_idx, uint32_t src_type, uint8_t *out_last_level);
uint16_t GetWallOrientation(uint16_t usIndex);

void SetSpecificDatabaseValues(uint16_t type, uint16_t database_elem, TILE_ELEMENT &,
                               bool use_raised_object_type);

void AllocateAnimTileData(TILE_ELEMENT *pTileElem, uint8_t ubNumFrames);
void DeallocateTileDatabase();

#endif
