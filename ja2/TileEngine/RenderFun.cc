// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "TileEngine/RenderFun.h"

#include <string.h>

#include "SGP/Input.h"
#include "SGP/Random.h"
#include "Tactical/DialogueControl.h"
#include "Tactical/FOV.h"
#include "Tactical/HandleItems.h"
#include "TileEngine/Environment.h"
#include "TileEngine/FogOfWar.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/RenderWorld.h"
#include "TileEngine/Structure.h"
#include "TileEngine/SysUtil.h"
#include "TileEngine/TileDat.h"
#include "TileEngine/WorldDef.h"
#include "TileEngine/WorldMan.h"

// Room Information
uint8_t gubWorldRoomInfo[WORLD_MAX];
uint8_t gubWorldRoomHidden[MAX_ROOMS];

void InitRoomDatabase() {
  memset(gubWorldRoomInfo, NO_ROOM, sizeof(gubWorldRoomInfo));
  memset(gubWorldRoomHidden, TRUE, sizeof(gubWorldRoomHidden));
}

uint8_t GetRoom(uint16_t const gridno) { return gubWorldRoomInfo[gridno]; }

BOOLEAN InAHiddenRoom(uint16_t sGridNo, uint8_t *pubRoomNo) {
  if (gubWorldRoomInfo[sGridNo] != NO_ROOM) {
    if ((gubWorldRoomHidden[gubWorldRoomInfo[sGridNo]])) {
      *pubRoomNo = gubWorldRoomInfo[sGridNo];
      return (TRUE);
    }
  }

  return (FALSE);
}

// @@ATECLIP TO WORLD!
void SetRecalculateWireFrameFlagRadius(const GridNo pos, const int16_t sRadius) {
  int16_t pos_x_;
  int16_t pos_y_;
  ConvertGridNoToXY(pos, &pos_x_, &pos_y_);
  const int16_t pos_x = pos_x_;
  const int16_t pos_y = pos_y_;
  for (int16_t y = pos_y - sRadius; y < pos_y + sRadius + 2; ++y) {
    for (int16_t x = pos_x - sRadius; x < pos_x + sRadius + 2; ++x) {
      const uint32_t uiTile = MAPROWCOLTOPOS(y, x);
      gpWorldLevelData[uiTile].uiFlags |= MAPELEMENT_RECALCULATE_WIREFRAMES;
    }
  }
}

void SetGridNoRevealedFlag(uint16_t const grid_no) {
  // Set hidden flag, for any roofs
  SetRoofIndexFlagsFromTypeRange(grid_no, FIRSTROOF, FOURTHROOF, LEVELNODE_HIDDEN);

  // ATE: Do this only if we are in a room
  if (gubWorldRoomInfo[grid_no] != NO_ROOM) {
    SetStructAframeFlags(grid_no, LEVELNODE_HIDDEN);

    // Find gridno one east as well
    if (grid_no + WORLD_COLS < NOWHERE) {
      SetStructAframeFlags(grid_no + WORLD_COLS, LEVELNODE_HIDDEN);
    }

    if (grid_no + 1 < NOWHERE) {
      SetStructAframeFlags(grid_no + 1, LEVELNODE_HIDDEN);
    }
  }

  // Set gridno as revealed
  gpWorldLevelData[grid_no].uiFlags |= MAPELEMENT_REVEALED;
  if (gfCaves) RemoveFogFromGridNo(grid_no);

  // ATE: If there are any structs here, we can render them with the obscured
  // flag! Look for anything but walls pn this gridno!
  for (STRUCTURE *i = gpWorldLevelData[grid_no].pStructureHead; i; i = i->pNext) {
    if (!(i->fFlags & STRUCTURE_SLANTED_ROOF)) {
      if (i->sCubeOffset != STRUCTURE_ON_GROUND) continue;
      if (!(i->fFlags & STRUCTURE_OBSTACLE) || i->fFlags & (STRUCTURE_PERSON | STRUCTURE_CORPSE))
        continue;
    }

    STRUCTURE *const base = FindBaseStructure(i);
    LEVELNODE *const node = FindLevelNodeBasedOnStructure(base);
    node->uiFlags |= LEVELNODE_SHOW_THROUGH;

    if (i->fFlags & STRUCTURE_SLANTED_ROOF) {
      AddSlantRoofFOVSlot(base->sGridNo);
      node->uiFlags |= LEVELNODE_HIDDEN;
    }
  }

  gubWorldRoomHidden[gubWorldRoomInfo[grid_no]] = FALSE;
}

void ExamineGridNoForSlantRoofExtraGraphic(GridNo const check_grid_no) {
  // Check for a slanted roof here
  STRUCTURE *const s = FindStructure(check_grid_no, STRUCTURE_SLANTED_ROOF);
  if (!s) return;

  // We have a slanted roof here, find base and remove
  bool changed = false;
  STRUCTURE *const base = FindBaseStructure(s);
  LEVELNODE *const node = FindLevelNodeBasedOnStructure(base);
  bool const hidden = node->uiFlags & LEVELNODE_HIDDEN;
  GridNo const base_grid_no = base->sGridNo;
  DB_STRUCTURE_TILE *const *const tile = base->pDBStructureRef->ppTile;
  DB_STRUCTURE_TILE *const *const end = tile + base->pDBStructureRef->pDBStructure->ubNumberOfTiles;
  // Loop through each gridno and see if revealed
  for (DB_STRUCTURE_TILE *const *i = tile; i != end; ++i) {
    GridNo const grid_no = base_grid_no + (*i)->sPosRelToBase;
    if (grid_no < 0 || WORLD_MAX < grid_no) continue;

    if (gpWorldLevelData[grid_no].uiFlags &
        MAPELEMENT_REVEALED) {  // Remove any slant roof items if they exist
      if (LEVELNODE const *const roof = FindTypeInRoofLayer(grid_no, SLANTROOFCEILING)) {
        RemoveRoof(grid_no, roof->usIndex);
        changed = true;
      }
    } else {  // Add graphic if one does not already exist
      if (hidden && !FindTypeInRoofLayer(grid_no, SLANTROOFCEILING)) {
        AddRoofToHead(grid_no, SLANTROOFCEILING1);
        changed = true;
      }
    }
  }

  if (changed) {  // Dirty the world
    InvalidateWorldRedundency();
    SetRenderFlags(RENDER_FLAG_FULL);
  }
}

void RemoveRoomRoof(uint16_t sGridNo, uint8_t bRoomNum, SOLDIERTYPE *pSoldier) {
  uint32_t cnt;
  BOOLEAN fSaidItemSeenQuote = FALSE;

  //	STRUCTURE					*pStructure;//, *pBase;

  // LOOP THORUGH WORLD AND CHECK ROOM INFO
  for (cnt = 0; cnt < WORLD_MAX; cnt++) {
    if (gubWorldRoomInfo[cnt] == bRoomNum) {
      SetGridNoRevealedFlag((uint16_t)cnt);

      RemoveRoofIndexFlagsFromTypeRange(cnt, FIRSTROOF, SECONDSLANTROOF, LEVELNODE_REVEAL);

      // Reveal any items if here!
      if (SetItemsVisibilityOn(cnt, 0, INVISIBLE, TRUE)) {
        if (!fSaidItemSeenQuote) {
          fSaidItemSeenQuote = TRUE;

          if (pSoldier != NULL) {
            TacticalCharacterDialogue(pSoldier,
                                      (uint16_t)(QUOTE_SPOTTED_SOMETHING_ONE + Random(2)));
          }
        }
      }

      // OK, re-set writeframes ( in a radius )
      SetRecalculateWireFrameFlagRadius(cnt, 2);
    }
  }

  // for ( cnt = 0; cnt < WORLD_MAX; cnt++ )
  //{
  //	if ( gubWorldRoomInfo[ cnt ] == bRoomNum )
  //	{
  //		ExamineGridNoForSlantRoofExtraGraphic( (uint16_t)cnt );
  //	}
  //}

  // DIRTY THE WORLD!
  InvalidateWorldRedundency();
  SetRenderFlags(RENDER_FLAG_FULL);

  CalculateWorldWireFrameTiles(FALSE);
}
