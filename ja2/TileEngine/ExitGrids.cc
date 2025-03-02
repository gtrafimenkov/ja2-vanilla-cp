// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "TileEngine/ExitGrids.h"

#include <string.h>

#include "Editor/EditorUndo.h"
#include "Editor/Smooth.h"
#include "SGP/Debug.h"
#include "SGP/FileMan.h"
#include "SGP/Types.h"
#include "Strategic/Quests.h"
#include "Strategic/StrategicMap.h"
#include "Strategic/StrategicMovement.h"
#include "SysGlobals.h"
#include "Tactical/AnimationControl.h"
#include "Tactical/Overhead.h"
#include "Tactical/PathAI.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/SaveLoadMap.h"
#include "TileEngine/TileDat.h"
#include "TileEngine/WorldDef.h"
#include "TileEngine/WorldMan.h"
#include "Utils/FontControl.h"
#include "Utils/Message.h"
#include "Utils/Text.h"

BOOLEAN gfLoadingExitGrids = FALSE;

// used by editor.
EXITGRID gExitGrid = {0, 1, 1, 0};

BOOLEAN gfOverrideInsertionWithExitGrid = FALSE;

static int32_t ConvertExitGridToINT32(EXITGRID *pExitGrid) {
  int32_t iExitGridInfo;
  iExitGridInfo = (pExitGrid->ubGotoSectorX - 1) << 28;
  iExitGridInfo += (pExitGrid->ubGotoSectorY - 1) << 24;
  iExitGridInfo += pExitGrid->ubGotoSectorZ << 20;
  iExitGridInfo += pExitGrid->usGridNo & 0x0000ffff;
  return iExitGridInfo;
}

static void ConvertINT32ToExitGrid(int32_t iExitGridInfo, EXITGRID *pExitGrid) {
  // convert the int into 4 unsigned bytes.
  pExitGrid->ubGotoSectorX = (uint8_t)(((iExitGridInfo & 0xf0000000) >> 28) + 1);
  pExitGrid->ubGotoSectorY = (uint8_t)(((iExitGridInfo & 0x0f000000) >> 24) + 1);
  pExitGrid->ubGotoSectorZ = (uint8_t)((iExitGridInfo & 0x00f00000) >> 20);
  pExitGrid->usGridNo = (uint16_t)(iExitGridInfo & 0x0000ffff);
}

BOOLEAN GetExitGrid(uint16_t usMapIndex, EXITGRID *pExitGrid) {
  LEVELNODE *pShadow;
  pShadow = gpWorldLevelData[usMapIndex].pShadowHead;
  // Search through object layer for an exitgrid
  while (pShadow) {
    if (pShadow->uiFlags & LEVELNODE_EXITGRID) {
      ConvertINT32ToExitGrid(pShadow->iExitGridInfo, pExitGrid);
      return TRUE;
    }
    pShadow = pShadow->pNext;
  }
  pExitGrid->ubGotoSectorX = 0;
  pExitGrid->ubGotoSectorY = 0;
  pExitGrid->ubGotoSectorZ = 0;
  pExitGrid->usGridNo = 0;
  return FALSE;
}

BOOLEAN ExitGridAtGridNo(uint16_t usMapIndex) {
  LEVELNODE *pShadow;
  pShadow = gpWorldLevelData[usMapIndex].pShadowHead;
  // Search through object layer for an exitgrid
  while (pShadow) {
    if (pShadow->uiFlags & LEVELNODE_EXITGRID) {
      return TRUE;
    }
    pShadow = pShadow->pNext;
  }
  return FALSE;
}

void AddExitGridToWorld(int32_t const map_idx, EXITGRID *const xg) {
  // Search through object layer for an exitgrid
  for (LEVELNODE *i = gpWorldLevelData[map_idx].pShadowHead; i; i = i->pNext) {
    if (!(i->uiFlags & LEVELNODE_EXITGRID)) continue;
    // We have found an existing exitgrid in this node, so replace it with the
    // new information.
    i->iExitGridInfo = ConvertExitGridToINT32(xg);
    return;
  }

  LEVELNODE *const n = AddShadowToHead(map_idx, MOCKFLOOR1);
  // Fill in the information for the new exitgrid levelnode.
  n->iExitGridInfo = ConvertExitGridToINT32(xg);
  n->uiFlags |= LEVELNODE_EXITGRID | LEVELNODE_HIDDEN;

  // Add the exit grid to the sector, only if ApplyMapChangesToMapTempFile is
  // held.
  if (!gfEditMode && !gfLoadingExitGrids) {
    AddExitGridToMapTempFile(map_idx, xg, gWorldSectorX, gWorldSectorY, gbWorldSectorZ);
  }
}

void RemoveExitGridFromWorld(int32_t iMapIndex) {
  RemoveAllShadowsOfTypeRange(iMapIndex, MOCKFLOOR, MOCKFLOOR);
}

void SaveExitGrids(HWFILE fp, uint16_t usNumExitGrids) {
  EXITGRID exitGrid;
  uint16_t usNumSaved = 0;
  uint16_t x;
  FileWrite(fp, &usNumExitGrids, 2);
  for (x = 0; x < WORLD_MAX; x++) {
    if (GetExitGrid(x, &exitGrid)) {
      FileWrite(fp, &x, 2);
      FileWrite(fp, &exitGrid, 5);
      usNumSaved++;
    }
  }
  // If these numbers aren't equal, something is wrong!
  Assert(usNumExitGrids == usNumSaved);
}

void LoadExitGrids(HWFILE const f) {
  EXITGRID exitGrid;
  uint16_t x;
  uint16_t usNumSaved;
  uint16_t usMapIndex;
  gfLoadingExitGrids = TRUE;
  FileRead(f, &usNumSaved, sizeof(usNumSaved));
  for (x = 0; x < usNumSaved; x++) {
    FileRead(f, &usMapIndex, sizeof(usMapIndex));
    FileRead(f, &exitGrid, 5);
    AddExitGridToWorld(usMapIndex, &exitGrid);
  }
  gfLoadingExitGrids = FALSE;
}

void AttemptToChangeFloorLevel(int8_t const relative_z_level) {
  if (relative_z_level == -1) {
    if (gbWorldSectorZ == 0) {  // on ground level -- can't go up!
      ScreenMsg(FONT_DKYELLOW, MSG_INTERFACE, pMessageStrings[MSG_CANT_GO_UP]);
      return;
    }
  } else if (relative_z_level == 1) {
    if (gbWorldSectorZ == 3) {  // on bottom level -- can't go down!
      ScreenMsg(FONT_DKYELLOW, MSG_INTERFACE, pMessageStrings[MSG_CANT_GO_DOWN]);
      return;
    }
  } else {
    return;
  }

  uint8_t const look_for_level = gbWorldSectorZ + relative_z_level;
  for (uint16_t i = 0; i != WORLD_MAX; ++i) {
    if (!GetExitGrid(i, &gExitGrid)) continue;
    if (gExitGrid.ubGotoSectorZ != look_for_level) continue;
    // found an exit grid leading to the goal sector!

    gfOverrideInsertionWithExitGrid = TRUE;
    /* change all current mercs in the loaded sector, and move them to the new
     * sector. */
    MoveAllGroupsInCurrentSectorToSector(gWorldSectorX, gWorldSectorY, look_for_level);
    if (look_for_level) {
      ScreenMsg(FONT_YELLOW, MSG_INTERFACE, pMessageStrings[MSG_ENTERING_LEVEL], look_for_level);
    } else {
      ScreenMsg(FONT_YELLOW, MSG_INTERFACE, pMessageStrings[MSG_LEAVING_BASEMENT]);
    }
    SetCurrentWorldSector(gWorldSectorX, gWorldSectorY, look_for_level);
    gfOverrideInsertionWithExitGrid = FALSE;
  }
}

uint16_t FindGridNoFromSweetSpotCloseToExitGrid(const SOLDIERTYPE *const pSoldier,
                                                const int16_t sSweetGridNo, const int8_t ubRadius) {
  int16_t sTop, sBottom;
  int16_t sLeft, sRight;
  int16_t cnt1, cnt2;
  int16_t sGridNo;
  int32_t uiRange, uiLowestRange = 999999;
  int32_t leftmost;
  SOLDIERTYPE soldier;
  uint8_t ubSaveNPCAPBudget;
  uint8_t ubSaveNPCDistLimit;
  EXITGRID ExitGrid;
  uint8_t ubGotoSectorX, ubGotoSectorY, ubGotoSectorZ;

  // Turn off at end of function...
  gfPlotPathToExitGrid = TRUE;

  // Save AI pathing vars.  changing the distlimit restricts how
  // far away the pathing will consider.
  ubSaveNPCAPBudget = gubNPCAPBudget;
  ubSaveNPCDistLimit = gubNPCDistLimit;
  gubNPCAPBudget = 0;
  gubNPCDistLimit = ubRadius;

  // create dummy soldier, and use the pathing to determine which nearby slots
  // are reachable.
  memset(&soldier, 0, sizeof(SOLDIERTYPE));
  soldier.bLevel = 0;
  soldier.bTeam = 1;
  soldier.sGridNo = pSoldier->sGridNo;

  // OK, Get an exit grid ( if possible )
  if (!GetExitGrid(sSweetGridNo, &ExitGrid)) {
    return (NOWHERE);
  }

  // Copy our dest values.....
  ubGotoSectorX = ExitGrid.ubGotoSectorX;
  ubGotoSectorY = ExitGrid.ubGotoSectorY;
  ubGotoSectorZ = ExitGrid.ubGotoSectorZ;

  sTop = ubRadius;
  sBottom = -ubRadius;
  sLeft = -ubRadius;
  sRight = ubRadius;

  // clear the mapelements of potential residue MAPELEMENT_REACHABLE flags
  // in the square region.
  for (cnt1 = sBottom; cnt1 <= sTop; cnt1++) {
    for (cnt2 = sLeft; cnt2 <= sRight; cnt2++) {
      sGridNo = pSoldier->sGridNo + (WORLD_COLS * cnt1) + cnt2;
      if (sGridNo >= 0 && sGridNo < WORLD_MAX) {
        gpWorldLevelData[sGridNo].uiFlags &= (~MAPELEMENT_REACHABLE);
      }
    }
  }

  // Now, find out which of these gridnos are reachable
  //(use the fake soldier and the pathing settings)
  FindBestPath(&soldier, NOWHERE, 0, WALKING, COPYREACHABLE, PATH_THROUGH_PEOPLE);

  uiLowestRange = 999999;

  int16_t sLowestGridNo = NOWHERE;
  for (cnt1 = sBottom; cnt1 <= sTop; cnt1++) {
    leftmost = ((pSoldier->sGridNo + (WORLD_COLS * cnt1)) / WORLD_COLS) * WORLD_COLS;

    for (cnt2 = sLeft; cnt2 <= sRight; cnt2++) {
      sGridNo = pSoldier->sGridNo + (WORLD_COLS * cnt1) + cnt2;
      if (sGridNo >= 0 && sGridNo < WORLD_MAX && sGridNo >= leftmost &&
          sGridNo < (leftmost + WORLD_COLS) &&
          gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE) {
        // Go on sweet stop
        // ATE: Added this check because for all intensive purposes, cavewalls
        // will be not an OKDEST but we want thenm too...
        if (NewOKDestination(pSoldier, sGridNo, TRUE, pSoldier->bLevel)) {
          if (GetExitGrid(sGridNo, &ExitGrid)) {
            // Is it the same exitgrid?
            if (ExitGrid.ubGotoSectorX == ubGotoSectorX &&
                ExitGrid.ubGotoSectorY == ubGotoSectorY &&
                ExitGrid.ubGotoSectorZ == ubGotoSectorZ) {
              uiRange = GetRangeInCellCoordsFromGridNoDiff(pSoldier->sGridNo, sGridNo);

              if (uiRange < uiLowestRange) {
                sLowestGridNo = sGridNo;
                uiLowestRange = uiRange;
              }
            }
          }
        }
      }
    }
  }
  gubNPCAPBudget = ubSaveNPCAPBudget;
  gubNPCDistLimit = ubSaveNPCDistLimit;

  gfPlotPathToExitGrid = FALSE;

  return sLowestGridNo;
}

uint16_t FindClosestExitGrid(SOLDIERTYPE *pSoldier, int16_t sSrcGridNo, int8_t ubRadius) {
  int16_t sTop, sBottom;
  int16_t sLeft, sRight;
  int16_t cnt1, cnt2;
  int16_t sGridNo;
  int32_t uiRange, uiLowestRange = 999999;
  int32_t leftmost;
  EXITGRID ExitGrid;

  sTop = ubRadius;
  sBottom = -ubRadius;
  sLeft = -ubRadius;
  sRight = ubRadius;

  // clear the mapelements of potential residue MAPELEMENT_REACHABLE flags
  uiLowestRange = 999999;

  int16_t sLowestGridNo = NOWHERE;
  for (cnt1 = sBottom; cnt1 <= sTop; cnt1++) {
    leftmost = ((sSrcGridNo + (WORLD_COLS * cnt1)) / WORLD_COLS) * WORLD_COLS;

    for (cnt2 = sLeft; cnt2 <= sRight; cnt2++) {
      sGridNo = sSrcGridNo + (WORLD_COLS * cnt1) + cnt2;
      if (sGridNo >= 0 && sGridNo < WORLD_MAX && sGridNo >= leftmost &&
          sGridNo < (leftmost + WORLD_COLS)) {
        if (GetExitGrid(sGridNo, &ExitGrid)) {
          uiRange = GetRangeInCellCoordsFromGridNoDiff(sSrcGridNo, sGridNo);

          if (uiRange < uiLowestRange) {
            sLowestGridNo = sGridNo;
            uiLowestRange = uiRange;
          }
        }
      }
    }
  }

  return sLowestGridNo;
}

#include "gtest/gtest.h"

TEST(ExitGrids, asserts) { EXPECT_EQ(sizeof(EXITGRID), 6); }
