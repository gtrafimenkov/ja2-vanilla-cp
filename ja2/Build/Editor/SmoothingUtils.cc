#include "Editor/SmoothingUtils.h"

#include <stdlib.h>

#include "Editor/EditSys.h"
#include "Editor/EditorDefines.h"
#include "Editor/EditorUndo.h"
#include "Editor/NewSmooth.h"
#include "Editor/SmartMethod.h"
#include "TileEngine/Environment.h"
#include "TileEngine/IsometricUtils.h"  //for GridNoOnVisibleWorldTile()
#include "TileEngine/TileDef.h"
#include "TileEngine/WorldDef.h"  //for LEVELNODE def
#include "TileEngine/WorldMan.h"  //for RemoveXXXX()

// This method isn't foolproof, but because erasing large areas of buildings
// could result in multiple wall types for each building.  When processing the
// region, it is necessary to calculate the roof type by searching for the
// nearest roof tile.
uint16_t SearchForWallType(uint32_t iMapIndex) {
  LEVELNODE *pWall;
  int16_t sOffset;
  int16_t x, y, sRadius = 0;
  if (gfBasement) {
    uint16_t usWallType;
    usWallType = GetRandomIndexByRange(FIRSTWALL, LASTWALL);
    if (usWallType == 0xffff) usWallType = FIRSTWALL;
    return usWallType;
  }
  while (sRadius < 32) {
    // NOTE:  start at the higher y value and go negative because it is possible
    // to have another
    // structure type one tile north, but not one tile south -- so it'll find
    // the correct wall first.
    for (y = sRadius; y >= -sRadius; y--)
      for (x = -sRadius; x <= sRadius; x++) {
        if (abs(x) == sRadius || abs(y) == sRadius) {
          sOffset = y * WORLD_COLS + x;
          if (!GridNoOnVisibleWorldTile((int16_t)(iMapIndex + sOffset))) {
            continue;
          }
          pWall = gpWorldLevelData[iMapIndex + sOffset].pStructHead;
          while (pWall) {
            const uint32_t uiTileType = GetTileType(pWall->usIndex);
            if (uiTileType >= FIRSTWALL &&
                uiTileType <= LASTWALL) {  // found a roof, so return its type.
              return (uint16_t)uiTileType;
            }
            // if( uiTileType >= FIRSTWINDOW && uiTileType <= LASTWINDOW )
            //{	//Window types can be converted to a wall type.
            //	return (uint16_t)(FIRSTWALL + uiTileType - FIRSTWINDOW );
            //}
            pWall = pWall->pNext;
          }
        }
      }
    sRadius++;
  }
  return 0xffff;
}

/* This method isn't foolproof, because erasing large areas of buildings could
 * result in multiple roof types for each building. When processing the region,
 * it is necessary to calculate the roof type by searching for the nearest roof
 * tile. */
uint16_t SearchForRoofType(uint32_t const map_idx) {
  for (int16_t radius = 0; radius != 32; ++radius) {
    for (int16_t y = -radius; y <= radius; ++y) {
      for (int16_t x = -radius; x <= radius; ++x) {
        if (abs(x) != radius && abs(y) != radius) continue;

        GridNo const grid_no = map_idx + y * WORLD_COLS + x;
        if (!GridNoOnVisibleWorldTile(grid_no)) continue;

        for (LEVELNODE const *i = gpWorldLevelData[grid_no].pRoofHead; i; i = i->pNext) {
          uint32_t const tile_type = GetTileType(i->usIndex);
          if (tile_type < FIRSTROOF && LASTROOF < tile_type) continue;
          // found a roof, so return its type.
          return tile_type;
        }
      }
    }
  }
  return 0xFFFF;
}

static bool RoofAtGridNo(uint32_t const map_idx) {
  for (LEVELNODE const *i = gpWorldLevelData[map_idx].pRoofHead; i;) {
    if (i->usIndex == NO_TILE) continue;

    uint32_t const tile_type = GetTileType(i->usIndex);
    if (FIRSTROOF <= tile_type && tile_type <= SECONDSLANTROOF) return true;
    i = i->pNext;  // XXX TODO0009 if i->usIndex == NO_TILE this is an endless
                   // loop
  }
  return false;
}

bool BuildingAtGridNo(uint32_t const map_idx) {
  return RoofAtGridNo(map_idx) || FloorAtGridNo(map_idx);
}

static LEVELNODE *GetHorizontalFence(uint32_t map_idx);
static LEVELNODE *GetVerticalFence(uint32_t map_idx);

bool ValidDecalPlacement(uint32_t const map_idx) {
  return GetVerticalWall(map_idx) || GetHorizontalWall(map_idx) || GetVerticalFence(map_idx) ||
         GetHorizontalFence(map_idx);
}

LEVELNODE *GetVerticalWall(uint32_t const map_idx) {
  for (LEVELNODE *i = gpWorldLevelData[map_idx].pStructHead; i; i = i->pNext) {
    if (i->usIndex == NO_TILE) continue;

    uint32_t const tile_type = GetTileType(i->usIndex);
    if ((FIRSTWALL <= tile_type && tile_type <= LASTWALL) ||
        (FIRSTDOOR <= tile_type && tile_type <= LASTDOOR)) {
      uint16_t const wall_orientation = GetWallOrientation(i->usIndex);
      if (wall_orientation != INSIDE_TOP_RIGHT && wall_orientation != OUTSIDE_TOP_RIGHT) continue;
      return i;
    }
  }
  return 0;
}

LEVELNODE *GetHorizontalWall(uint32_t const map_idx) {
  for (LEVELNODE *i = gpWorldLevelData[map_idx].pStructHead; i; i = i->pNext) {
    if (i->usIndex == NO_TILE) continue;

    uint32_t const tile_type = GetTileType(i->usIndex);
    if ((FIRSTWALL <= tile_type && tile_type <= LASTWALL) ||
        (FIRSTDOOR <= tile_type && tile_type <= LASTDOOR)) {
      uint16_t const wall_orientation = GetWallOrientation(i->usIndex);
      if (wall_orientation != INSIDE_TOP_LEFT && wall_orientation != OUTSIDE_TOP_LEFT) continue;
      return i;
    }
  }
  return 0;
}

uint16_t GetVerticalWallType(uint32_t const map_idx) {
  LEVELNODE const *const wall = GetVerticalWall(map_idx);
  return wall ? GetWallType(wall, map_idx) : 0;
}

uint16_t GetHorizontalWallType(uint32_t const map_idx) {
  LEVELNODE const *const wall = GetHorizontalWall(map_idx);
  return wall ? GetWallType(wall, map_idx) : 0;
}

static LEVELNODE *GetVerticalFence(uint32_t const map_idx) {
  for (LEVELNODE *i = gpWorldLevelData[map_idx].pStructHead; i; i = i->pNext) {
    if (i->usIndex == NO_TILE) continue;
    if (GetTileType(i->usIndex) != FENCESTRUCT) continue;

    uint16_t const wall_orientation = GetWallOrientation(i->usIndex);
    if (wall_orientation != INSIDE_TOP_RIGHT && wall_orientation != OUTSIDE_TOP_RIGHT) continue;
    return i;
  }
  return 0;
}

static LEVELNODE *GetHorizontalFence(uint32_t const map_idx) {
  for (LEVELNODE *i = gpWorldLevelData[map_idx].pStructHead; i; i = i->pNext) {
    if (i->usIndex == NO_TILE) continue;
    if (GetTileType(i->usIndex) != FENCESTRUCT) continue;

    uint16_t const wall_orientation = GetWallOrientation(i->usIndex);
    if (wall_orientation != INSIDE_TOP_LEFT && wall_orientation != OUTSIDE_TOP_LEFT) continue;
    return i;
  }
  return 0;
}

void EraseHorizontalWall(uint32_t iMapIndex) {
  LEVELNODE *pWall;
  pWall = GetHorizontalWall(iMapIndex);
  if (pWall) {
    AddToUndoList(iMapIndex);
    RemoveStruct(iMapIndex, pWall->usIndex);
    RemoveAllShadowsOfTypeRange(iMapIndex, FIRSTWALL, LASTWALL);
  }
}

void EraseVerticalWall(uint32_t iMapIndex) {
  LEVELNODE *pWall;
  pWall = GetVerticalWall(iMapIndex);
  if (pWall) {
    AddToUndoList(iMapIndex);
    RemoveStruct(iMapIndex, pWall->usIndex);
    RemoveAllShadowsOfTypeRange(iMapIndex, FIRSTWALL, LASTWALL);
  }
}

static void ChangeWall(LEVELNODE const *const wall, uint32_t const map_idx,
                       uint16_t const new_piece) {
  if (!wall) return;

  uint32_t const tile_type = GetTileType(wall->usIndex);
  if (tile_type < FIRSTWALL || LASTWALL < tile_type) return;

  // We have the wall, now change its type
  int16_t const idx = PickAWallPiece(new_piece);
  AddToUndoList(map_idx);
  uint16_t const tile_idx = GetTileIndexFromTypeSubIndex(tile_type, idx);
  ReplaceStructIndex(map_idx, wall->usIndex, tile_idx);
}

static void ChangeHorizontalWall(uint32_t const map_idx, uint16_t const new_piece) {
  ChangeWall(GetHorizontalWall(map_idx), map_idx, new_piece);
}

void ChangeVerticalWall(uint32_t const map_idx, uint16_t const new_piece) {
  ChangeWall(GetVerticalWall(map_idx), map_idx, new_piece);
}

uint16_t GetWallType(LEVELNODE const *const wall, uint32_t const map_idx) {
  uint32_t const tile_type = GetTileType(wall->usIndex);
  /* Doors do not contain the wall type, so search for the nearest wall to
   * extract it */
  return FIRSTDOOR <= tile_type && tile_type <= LASTDOOR ? SearchForWallType(map_idx)
                                                         : (uint16_t)tile_type;
}

void RestoreWalls(uint32_t const map_idx) {
  bool done = false;

  if (LEVELNODE const *const wall = GetHorizontalWall(map_idx)) {
    uint16_t const wall_type = GetWallType(wall, map_idx);
    uint16_t const wall_orientation = GetWallOrientation(wall->usIndex);
    AddToUndoList(map_idx);
    RemoveStruct(map_idx, wall->usIndex);
    RemoveAllShadowsOfTypeRange(map_idx, FIRSTWALL, LASTWALL);
    switch (wall_orientation) {
      case OUTSIDE_TOP_LEFT:
        BuildWallPiece(map_idx, INTERIOR_BOTTOM, wall_type);
        break;
      case INSIDE_TOP_LEFT:
        BuildWallPiece(map_idx, EXTERIOR_BOTTOM, wall_type);
        break;
    }
    done = true;
  }

  if (LEVELNODE const *const wall = GetVerticalWall(map_idx)) {
    uint16_t const wall_type = GetWallType(wall, map_idx);
    uint16_t const wall_orientation = GetWallOrientation(wall->usIndex);
    AddToUndoList(map_idx);
    RemoveStruct(map_idx, wall->usIndex);
    RemoveAllShadowsOfTypeRange(map_idx, FIRSTWALL, LASTWALL);
    switch (wall_orientation) {
      case OUTSIDE_TOP_RIGHT:
        BuildWallPiece(map_idx, INTERIOR_RIGHT, wall_type);
        break;
      case INSIDE_TOP_RIGHT:
        BuildWallPiece(map_idx, EXTERIOR_RIGHT, wall_type);
        break;
    }
    done = true;
  }

  if (done) return;

  /* We are in a special case here. The user is attempting to restore a wall,
   * though nothing is here. We will hook into the smart wall method by tricking
   * it into using the local wall type, but only if we have adjacent walls. */
  LEVELNODE *wall = GetHorizontalWall(map_idx - 1);
  if (!wall) wall = GetHorizontalWall(map_idx + 1);
  if (!wall) wall = GetVerticalWall(map_idx - WORLD_COLS);
  if (!wall) wall = GetVerticalWall(map_idx + WORLD_COLS);
  if (!wall) return;

  /* Found a wall.  Let's back up the current wall value, and restore it after
   * pasting a smart wall. */
  uint16_t const wall_type = GetWallType(wall, map_idx);
  if (wall_type == 0xFFFF) return;

  uint8_t const save_wall_ui_value = gubWallUIValue;  // Save the wall UI value
  gubWallUIValue = (uint8_t)wall_type;                // Trick the UI value
  PasteSmartWall(map_idx);                            // Paste smart wall with fake UI value
  gubWallUIValue = save_wall_ui_value;                // Restore the real UI value
}
