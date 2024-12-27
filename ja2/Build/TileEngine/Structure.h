// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef STRUCTURE_H
#define STRUCTURE_H

#include "JA2Types.h"
#include "SGP/AutoObj.h"
#include "Tactical/OverheadTypes.h"
#include "TileEngine/StructureInternals.h"
#include "Utils/SoundControl.h"

#define NOTHING_BLOCKING 0
#define BLOCKING_REDUCE_RANGE 1
#define BLOCKING_NEXT_TILE 10
#define BLOCKING_TOPLEFT_WINDOW 30
#define BLOCKING_TOPRIGHT_WINDOW 40
#define BLOCKING_TOPLEFT_DOOR 50
#define BLOCKING_TOPRIGHT_DOOR 60
#define FULL_BLOCKING 70
#define BLOCKING_TOPLEFT_OPEN_WINDOW 90
#define BLOCKING_TOPRIGHT_OPEN_WINDOW 100

// ATE: Increased to allow corpses to not collide with soldiers
// 100 == MAX_CORPSES
#define INVALID_STRUCTURE_ID (TOTAL_SOLDIERS + 100)
#define IGNORE_PEOPLE_STRUCTURE_ID (TOTAL_SOLDIERS + 101)

enum StructureDamageReason { STRUCTURE_DAMAGE_EXPLOSION = 1, STRUCTURE_DAMAGE_GUNFIRE = 2 };

// functions at the structure database level
STRUCTURE_FILE_REF *LoadStructureFile(const char *szFileName);
void FreeAllStructureFiles();
void FreeStructureFile(STRUCTURE_FILE_REF *);

//
// functions at the structure instance level
//
BOOLEAN OkayToAddStructureToWorld(int16_t sBaseGridNo, int8_t bLevel,
                                  const DB_STRUCTURE_REF *pDBStructureRef, int16_t sExclusionID);
BOOLEAN
InternalOkayToAddStructureToWorld(int16_t sBaseGridNo, int8_t bLevel,
                                  const DB_STRUCTURE_REF *pDBStructureRef, int16_t sExclusionID,
                                  BOOLEAN fIgnorePeople);

STRUCTURE *AddStructureToWorld(int16_t base_grid_no, int8_t level, DB_STRUCTURE_REF const *,
                               LEVELNODE *);
BOOLEAN DeleteStructureFromWorld(STRUCTURE *pStructure);

//
// functions to find a structure in a location
//

// Finds a structure that matches any of the given flags
STRUCTURE *FindStructure(int16_t sGridNo, StructureFlags);

STRUCTURE *FindNextStructure(const STRUCTURE *s, StructureFlags);
STRUCTURE *FindStructureByID(int16_t sGridNo, uint16_t structure_id);

#define FOR_EACH_STRUCTURE(iter, grid_no, flags)                  \
  for (STRUCTURE *iter = FindStructure((grid_no), (flags)); iter; \
       iter = FindNextStructure(iter, (flags)))

// Finds the base structure for any structure
STRUCTURE *FindBaseStructure(STRUCTURE *s);

//
// functions related to interactive tiles
//
STRUCTURE *SwapStructureForPartner(STRUCTURE *);
STRUCTURE *SwapStructureForPartnerAndStoreChangeInMap(STRUCTURE *);
//
// functions useful for AI that return info about heights
//
int8_t StructureHeight(STRUCTURE *pStructure);
int8_t StructureBottomLevel(STRUCTURE *pStructure);
int8_t GetTallestStructureHeight(int16_t sGridNo, BOOLEAN fOnRoof);
int8_t GetStructureTargetHeight(int16_t sGridNo, BOOLEAN fOnRoof);

BOOLEAN StructureDensity(STRUCTURE *pStructure, uint8_t *pubLevel0, uint8_t *pubLevel1,
                         uint8_t *pubLevel2, uint8_t *pubLevel3);

BOOLEAN FindAndSwapStructure(int16_t sGridNo);
//
// functions to work with the editor undo code
//

void DebugStructurePage1();

void AddZStripInfoToVObject(HVOBJECT, STRUCTURE_FILE_REF const *, BOOLEAN fFromAnimation,
                            int16_t sSTIStartIndex);

// FUNCTIONS FOR DETERMINING STUFF THAT BLOCKS VIEW FOR TILE_bASED LOS
int8_t GetBlockingStructureInfo(int16_t sGridNo, int8_t bDir, int8_t bNextDir, int8_t bLevel,
                                int8_t *pStructHeight, STRUCTURE **ppTallestStructure,
                                BOOLEAN fWallsBlock);

BOOLEAN DamageStructure(STRUCTURE *, uint8_t damage, StructureDamageReason, GridNo, int16_t x,
                        int16_t y, SOLDIERTYPE *owner);

// Material armour type enumeration
enum {
  MATERIAL_NOTHING,
  MATERIAL_WOOD_WALL,
  MATERIAL_PLYWOOD_WALL,
  MATERIAL_LIVE_WOOD,
  MATERIAL_LIGHT_VEGETATION,
  MATERIAL_FURNITURE,
  MATERIAL_PORCELAIN,
  MATERIAL_CACTUS,
  MATERIAL_NOTUSED1,
  MATERIAL_NOTUSED2,

  MATERIAL_NOTUSED3,
  MATERIAL_STONE,
  MATERIAL_CONCRETE1,
  MATERIAL_CONCRETE2,
  MATERIAL_ROCK,
  MATERIAL_RUBBER,
  MATERIAL_SAND,
  MATERIAL_CLOTH,
  MATERIAL_SANDBAG,
  MATERIAL_NOTUSED5,

  MATERIAL_NOTUSED6,
  MATERIAL_LIGHT_METAL,
  MATERIAL_THICKER_METAL,
  MATERIAL_HEAVY_METAL,
  MATERIAL_INDESTRUCTABLE_STONE,
  MATERIAL_INDESTRUCTABLE_METAL,
  MATERIAL_THICKER_METAL_WITH_SCREEN_WINDOWS,
  NUM_MATERIAL_TYPES
};

STRUCTURE *FindStructureBySavedInfo(GridNo, uint8_t type, uint8_t wall_orientation, int8_t level);
uint8_t StructureFlagToType(uint32_t uiFlag);

SoundID GetStructureOpenSound(STRUCTURE const *, bool closing);

extern const uint8_t gubMaterialArmour[];

typedef SGP::AutoObj<STRUCTURE_FILE_REF, FreeStructureFile> AutoStructureFileRef;

#endif
