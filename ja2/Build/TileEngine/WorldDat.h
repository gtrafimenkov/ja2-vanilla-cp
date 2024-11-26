#ifndef WORLD_DAT_H
#define WORLD_DAT_H

#include "SGP/Types.h"
#include "TileEngine/TileDat.h"
#include "TileEngine/WorldTilesetEnums.h"

typedef void (*TILESET_CALLBACK)();

struct TILESET {
  wchar_t zName[32];
  CHAR8 TileSurfaceFilenames[NUMBEROFTILETYPES][32];
  UINT8 ubAmbientID;
  TILESET_CALLBACK MovementCostFnc;
};

extern TILESET gTilesets[NUM_TILESETS];

void InitEngineTilesets();

// THESE FUNCTIONS WILL SET TERRAIN VALUES - CALL ONE FOR EACH TILESET
void SetTilesetOneTerrainValues();

#endif
