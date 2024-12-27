// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "TileEngine/TileCache.h"

#include <cstring>
#include <stdexcept>
#include <vector>

#include "Directories.h"
#include "Macro.h"
#include "SGP/FileMan.h"
#include "SGP/HImage.h"
#include "SGP/MemMan.h"
#include "SGP/SGPStrings.h"
#include "Tactical/AnimationCache.h"
#include "Tactical/AnimationData.h"
#include "TileEngine/Structure.h"
#include "TileEngine/TileDef.h"
#include "TileEngine/TileSurface.h"
#include "Utils/DebugControl.h"

struct TILE_CACHE_STRUCT {
  char zRootName[30];
  STRUCTURE_FILE_REF *pStructureFileRef;
};

static const uint32_t guiMaxTileCacheSize = 50;
static uint32_t guiCurTileCacheSize = 0;
static int32_t giDefaultStructIndex = -1;

TILE_CACHE_ELEMENT *gpTileCache;
static std::vector<TILE_CACHE_STRUCT> gpTileCacheStructInfo;

void InitTileCache() {
  gpTileCache = MALLOCN(TILE_CACHE_ELEMENT, guiMaxTileCacheSize);
  guiCurTileCacheSize = 0;

  // Zero entries
  for (uint32_t i = 0; i < guiMaxTileCacheSize; ++i) {
    gpTileCache[i].pImagery = 0;
    gpTileCache[i].struct_file_ref = 0;
  }

  // Look for JSD files in the tile cache directory and load any we find
  std::vector<std::string> jsdFiles =
      FindFilesInDir(FileMan::getTilecacheDirPath(), ".jsd", true, false);

  for (const std::string &file : jsdFiles) {
    TILE_CACHE_STRUCT tc;
    GetRootName(tc.zRootName, lengthof(tc.zRootName), file.c_str());
    tc.pStructureFileRef = LoadStructureFile(file.c_str());

    if (strcasecmp(tc.zRootName, "l_dead1") == 0) {
      giDefaultStructIndex = (int32_t)gpTileCacheStructInfo.size();
    }

    gpTileCacheStructInfo.push_back(tc);
  }
}

void DeleteTileCache() {
  uint32_t cnt;

  // Allocate entries
  if (gpTileCache != NULL) {
    // Loop through and delete any entries
    for (cnt = 0; cnt < guiMaxTileCacheSize; cnt++) {
      if (gpTileCache[cnt].pImagery != NULL) {
        DeleteTileSurface(gpTileCache[cnt].pImagery);
      }
    }
    MemFree(gpTileCache);
  }

  gpTileCacheStructInfo.clear();

  guiCurTileCacheSize = 0;
}

int32_t GetCachedTile(const char *const filename) {
  int32_t idx = -1;

  // Check to see if surface exists already
  for (uint32_t cnt = 0; cnt < guiCurTileCacheSize; ++cnt) {
    TILE_CACHE_ELEMENT *const i = &gpTileCache[cnt];
    if (i->pImagery == NULL) {
      if (idx == -1) idx = cnt;
      continue;
    }

    if (strcasecmp(i->zName, filename) != 0) continue;

    // Found surface, return
    ++i->sHits;
    return (int32_t)cnt;
  }

  if (idx == -1) {
    if (guiCurTileCacheSize < guiMaxTileCacheSize) {
      idx = guiCurTileCacheSize++;
    } else {
      // cache out least used file
      idx = 0;
      int16_t sMostHits = gpTileCache[idx].sHits;
      for (uint32_t cnt = 1; cnt < guiCurTileCacheSize; ++cnt) {
        const TILE_CACHE_ELEMENT *const i = &gpTileCache[cnt];
        if (i->sHits < sMostHits) {
          sMostHits = i->sHits;
          idx = cnt;
        }
      }

      // Bump off lowest index
      TILE_CACHE_ELEMENT *const del = &gpTileCache[idx];
      DeleteTileSurface(del->pImagery);
      del->sHits = 0;
      del->pImagery = 0;
      del->struct_file_ref = 0;
    }
  }

  TILE_CACHE_ELEMENT *const tce = &gpTileCache[idx];

  tce->pImagery = LoadTileSurface(filename);

  strcpy(tce->zName, filename);
  tce->sHits = 1;

  char root_name[30];
  GetRootName(root_name, lengthof(root_name), filename);
  STRUCTURE_FILE_REF *const sfr = GetCachedTileStructureRefFromFilename(root_name);
  tce->struct_file_ref = sfr;
  if (sfr) AddZStripInfoToVObject(tce->pImagery->vo, sfr, TRUE, 0);

  const AuxObjectData *const aux = tce->pImagery->pAuxData;
  tce->ubNumFrames = (aux != NULL ? aux->ubNumberOfFrames : 1);

  return idx;
}

void RemoveCachedTile(int32_t const cached_tile) {
  if ((uint32_t)cached_tile < guiCurTileCacheSize) {
    TILE_CACHE_ELEMENT &e = gpTileCache[cached_tile];
    if (e.pImagery) {
      if (--e.sHits != 0) return;

      DeleteTileSurface(e.pImagery);
      e.pImagery = 0;
      e.struct_file_ref = 0;
      return;
    }
  }
  throw std::logic_error("Trying to remove invalid cached tile");
}

static STRUCTURE_FILE_REF *GetCachedTileStructureRef(int32_t const idx) {
  return idx != -1 ? gpTileCache[idx].struct_file_ref : 0;
}

STRUCTURE_FILE_REF *GetCachedTileStructureRefFromFilename(char const *const filename) {
  size_t const n = gpTileCacheStructInfo.size();
  for (size_t i = 0; i != n; ++i) {
    TILE_CACHE_STRUCT &t = gpTileCacheStructInfo[i];
    if (strcasecmp(t.zRootName, filename) == 0) return t.pStructureFileRef;
  }
  return 0;
}

void CheckForAndAddTileCacheStructInfo(LEVELNODE *const pNode, int16_t const sGridNo,
                                       uint16_t const usIndex, uint16_t const usSubIndex) {
  STRUCTURE_FILE_REF *const sfr = GetCachedTileStructureRef(usIndex);
  if (!sfr) return;

  if (AddStructureToWorld(sGridNo, 0, &sfr->pDBStructureRef[usSubIndex], pNode)) return;

  if (giDefaultStructIndex == -1) return;

  STRUCTURE_FILE_REF *const def_sfr = gpTileCacheStructInfo[giDefaultStructIndex].pStructureFileRef;
  if (!def_sfr) return;

  AddStructureToWorld(sGridNo, 0, &def_sfr->pDBStructureRef[usSubIndex], pNode);
}

void CheckForAndDeleteTileCacheStructInfo(LEVELNODE *pNode, uint16_t usIndex) {
  STRUCTURE_FILE_REF *pStructureFileRef;

  if (usIndex >= TILE_CACHE_START_INDEX) {
    pStructureFileRef = GetCachedTileStructureRef((usIndex - TILE_CACHE_START_INDEX));

    if (pStructureFileRef != NULL) {
      DeleteStructureFromWorld(pNode->pStructureData);
    }
  }
}

void GetRootName(char *const pDestStr, size_t const n, char const *const pSrcStr) {
  // Remove path and extension
  ReplacePath(pDestStr, n, "", pSrcStr, "");
}
