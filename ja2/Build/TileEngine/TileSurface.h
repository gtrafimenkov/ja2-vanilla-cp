// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef _TILE_SURFACE_H
#define _TILE_SURFACE_H

#include "TileEngine/TileDat.h"
#include "TileEngine/WorldDef.h"

extern TILE_IMAGERY *gTileSurfaceArray[NUMBEROFTILETYPES];

TILE_IMAGERY *LoadTileSurface(const char *cFilename);

void DeleteTileSurface(TILE_IMAGERY *pTileSurf);

void SetRaisedObjectFlag(char const *filename, TILE_IMAGERY *);

#endif
