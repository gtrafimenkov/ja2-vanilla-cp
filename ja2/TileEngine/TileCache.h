// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __TILE_CACHE_H
#define __TILE_CACHE_H

#include <stdlib.h>

#include "JA2Types.h"

#define TILE_CACHE_START_INDEX 36000

struct TILE_CACHE_ELEMENT {
  char zName[128];         // Name of tile (filename and directory here)
  TILE_IMAGERY *pImagery;  // Tile imagery
  int16_t sHits;
  uint8_t ubNumFrames;
  STRUCTURE_FILE_REF *struct_file_ref;
};

extern TILE_CACHE_ELEMENT *gpTileCache;

void InitTileCache();
void DeleteTileCache();

int32_t GetCachedTile(const char *cFilename);
void RemoveCachedTile(int32_t cached_tile);

STRUCTURE_FILE_REF *GetCachedTileStructureRefFromFilename(const char *cFilename);

void CheckForAndAddTileCacheStructInfo(LEVELNODE *pNode, int16_t sGridNo, uint16_t usIndex,
                                       uint16_t usSubIndex);
void CheckForAndDeleteTileCacheStructInfo(LEVELNODE *pNode, uint16_t usIndex);
void GetRootName(char *pDestStr, size_t n, char const *pSrcStr);

// OF COURSE, FOR SPEED, WE EXPORT OUR ARRAY
// ACCESS FUNCTIONS IN RENDERER IS NOT TOO NICE
// ATE

#endif
