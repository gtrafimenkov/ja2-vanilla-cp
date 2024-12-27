// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef WORLD_DAT_H
#define WORLD_DAT_H

#include "SGP/Types.h"
#include "TileEngine/TileDat.h"
#include "TileEngine/WorldTilesetEnums.h"

typedef void (*TILESET_CALLBACK)();

struct TILESET {
  wchar_t zName[32];
  char TileSurfaceFilenames[NUMBEROFTILETYPES][32];
  uint8_t ubAmbientID;
  TILESET_CALLBACK MovementCostFnc;
};

extern TILESET gTilesets[NUM_TILESETS];

void InitEngineTilesets();

// THESE FUNCTIONS WILL SET TERRAIN VALUES - CALL ONE FOR EACH TILESET
void SetTilesetOneTerrainValues();

#endif
