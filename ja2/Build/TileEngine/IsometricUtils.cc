// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "TileEngine/IsometricUtils.h"

#include <algorithm>

#include "SGP/MouseSystem.h"
#include "SGP/Random.h"
#include "SysGlobals.h"
#include "Tactical/Interface.h"
#include "Tactical/Overhead.h"
#include "Tactical/PathAI.h"
#include "Tactical/StructureWrap.h"
#include "TileEngine/RenderWorld.h"
#include "TileEngine/Structure.h"
#include "TileEngine/WorldDef.h"
#include "TileEngine/WorldMan.h"
#include "math.h"

uint32_t guiForceRefreshMousePositionCalculation = 0;

// GLOBALS
const int16_t DirIncrementer[8] = {
    -MAPWIDTH,     // N
    1 - MAPWIDTH,  // NE
    1,             // E
    1 + MAPWIDTH,  // SE
    MAPWIDTH,      // S
    MAPWIDTH - 1,  // SW
    -1,            // W
    -MAPWIDTH - 1  // NW

};

//														DIRECTION
// FACING			 DIRECTION WE WANT TO GOTO
uint8_t const gPurpendicularDirection[NUM_WORLD_DIRECTIONS][NUM_WORLD_DIRECTIONS] = {
    {       // NORTH
     WEST,  // EITHER
     NORTHWEST, NORTH, NORTHEAST,
     EAST,  // EITHER
     NORTHWEST, NORTH, NORTHEAST},

    {// NORTH EAST
     NORTHWEST,
     NORTHWEST,  // EITHER
     SOUTH, NORTHEAST, EAST,
     SOUTHEAST,  // EITHER
     NORTH, NORTHEAST},

    {// EAST
     EAST, SOUTHEAST,
     NORTH,  // EITHER
     NORTHEAST, EAST, SOUTHEAST,
     NORTH,  // EITHER
     NORTHEAST},

    {
        // SOUTHEAST
        EAST, SOUTHEAST, SOUTH,
        SOUTHWEST,  // EITHER
        SOUTHWEST, SOUTHEAST, SOUTH,
        SOUTHWEST  // EITHER
    },

    {       // SOUTH
     WEST,  // EITHER
     SOUTHEAST, SOUTH, SOUTHWEST,
     EAST,  // EITHER
     SOUTHEAST, SOUTH, SOUTHWEST},

    {// SOUTHWEST
     WEST,
     NORTHWEST,  // EITHER
     SOUTH, SOUTHWEST, WEST,
     SOUTHEAST,  // EITHER
     SOUTH, SOUTHWEST},

    {// WEST
     WEST, NORTHWEST,
     NORTH,  // EITHER
     SOUTHWEST, WEST, NORTHWEST,
     SOUTH,  // EITHER
     SOUTHWEST},

    {
        // NORTHWEST
        WEST, NORTHWEST, NORTH,
        SOUTHWEST,  // EITHER
        SOUTHWEST, NORTHWEST, NORTH,
        NORTHEAST  // EITHER
    }};

void FromCellToScreenCoordinates(int16_t sCellX, int16_t sCellY, int16_t *psScreenX,
                                 int16_t *psScreenY) {
  *psScreenX = (2 * sCellX) - (2 * sCellY);
  *psScreenY = sCellX + sCellY;
}

void FromScreenToCellCoordinates(int16_t sScreenX, int16_t sScreenY, int16_t *psCellX,
                                 int16_t *psCellY) {
  *psCellX = ((sScreenX + (2 * sScreenY)) / 4);
  *psCellY = ((2 * sScreenY) - sScreenX) / 4;
}

// These two functions take into account that our world is projected and
// attached to the screen (0,0) in a specific way, and we MUSt take that into
// account then determining screen coords

void FloatFromCellToScreenCoordinates(float dCellX, float dCellY, float *pdScreenX,
                                      float *pdScreenY) {
  float dScreenX, dScreenY;

  dScreenX = (2 * dCellX) - (2 * dCellY);
  dScreenY = dCellX + dCellY;

  *pdScreenX = dScreenX;
  *pdScreenY = dScreenY;
}

BOOLEAN GetMouseXY(int16_t *psMouseX, int16_t *psMouseY) {
  int16_t sWorldX, sWorldY;

  if (!GetMouseWorldCoords(&sWorldX, &sWorldY)) {
    (*psMouseX) = 0;
    (*psMouseY) = 0;
    return (FALSE);
  }

  // Find start block
  (*psMouseX) = (sWorldX / CELL_X_SIZE);
  (*psMouseY) = (sWorldY / CELL_Y_SIZE);

  return (TRUE);
}

BOOLEAN GetMouseWorldCoords(int16_t *psMouseX, int16_t *psMouseY) {
  int16_t sOffsetX, sOffsetY;
  int16_t sTempPosX_W, sTempPosY_W;
  int16_t sStartPointX_W, sStartPointY_W;

  // Convert mouse screen coords into offset from center
  if (!(gViewportRegion.uiFlags & MSYS_MOUSE_IN_AREA)) {
    *psMouseX = 0;
    *psMouseY = 0;
    return (FALSE);
  }

  sOffsetX = gViewportRegion.MouseXPos -
             ((gsVIEWPORT_END_X - gsVIEWPORT_START_X) / 2);  // + gsRenderWorldOffsetX;
  sOffsetY = gViewportRegion.MouseYPos - ((gsVIEWPORT_END_Y - gsVIEWPORT_START_Y) / 2) +
             10;  // + gsRenderWorldOffsetY;

  // OK, Let's offset by a value if our interfac level is changed!
  if (gsInterfaceLevel != 0) {
    // sOffsetY -= 50;
  }

  FromScreenToCellCoordinates(sOffsetX, sOffsetY, &sTempPosX_W, &sTempPosY_W);

  // World start point is Render center plus this distance
  sStartPointX_W = gsRenderCenterX + sTempPosX_W;
  sStartPointY_W = gsRenderCenterY + sTempPosY_W;

  // check if we are out of bounds..
  if (sStartPointX_W < 0 || sStartPointX_W >= WORLD_COORD_ROWS || sStartPointY_W < 0 ||
      sStartPointY_W >= WORLD_COORD_COLS) {
    *psMouseX = 0;
    *psMouseY = 0;
    return (FALSE);
  }

  // Determine Start block and render offsets
  // Find start block
  // Add adjustment for render origin as well
  (*psMouseX) = sStartPointX_W;
  (*psMouseY) = sStartPointY_W;

  return (TRUE);
}

GridNo GetMouseMapPos() {
  static GridNo sSameCursorPos = NOWHERE;
  static uint32_t uiOldFrameNumber = 99999;

  // Check if this is the same frame as before, return already calculated value
  // if so!
  if (uiOldFrameNumber == guiGameCycleCounter && !guiForceRefreshMousePositionCalculation) {
    return sSameCursorPos;
  }

  uiOldFrameNumber = guiGameCycleCounter;
  guiForceRefreshMousePositionCalculation = FALSE;

  GridNo pos;
  int16_t sWorldX;
  int16_t sWorldY;
  if (GetMouseXY(&sWorldX, &sWorldY)) {
    pos = MAPROWCOLTOPOS(sWorldY, sWorldX);
  } else {
    pos = NOWHERE;
  }
  sSameCursorPos = pos;
  return pos;
}

void GetAbsoluteScreenXYFromMapPos(const GridNo pos, int16_t *const psWorldScreenX,
                                   int16_t *const psWorldScreenY) {
  int16_t sScreenCenterX, sScreenCenterY;

  int16_t sWorldCellX;
  int16_t sWorldCellY;
  ConvertGridNoToCellXY(pos, &sWorldCellX, &sWorldCellY);

  // Find the diustance from render center to true world center
  const int16_t sDistToCenterX = sWorldCellX - gCenterWorldX;
  const int16_t sDistToCenterY = sWorldCellY - gCenterWorldY;

  // From render center in world coords, convert to render center in "screen"
  // coords

  // ATE: We should call the fowllowing function but I'm putting it here
  // verbatim for speed
  // FromCellToScreenCoordinates( sDistToCenterX , sDistToCenterY,
  // &sScreenCenterX, &sScreenCenterY );
  sScreenCenterX = (2 * sDistToCenterX) - (2 * sDistToCenterY);
  sScreenCenterY = sDistToCenterX + sDistToCenterY;

  // Subtract screen center
  *psWorldScreenX = sScreenCenterX + gsCX - gsTLX;
  *psWorldScreenY = sScreenCenterY + gsCY - gsTLY;
}

GridNo GetMapPosFromAbsoluteScreenXY(const int16_t sWorldScreenX, const int16_t sWorldScreenY) {
  int16_t sWorldCenterX, sWorldCenterY;
  int16_t sDistToCenterY, sDistToCenterX;

  // Subtract screen center
  sDistToCenterX = sWorldScreenX - gsCX + gsTLX;
  sDistToCenterY = sWorldScreenY - gsCY + gsTLY;

  // From render center in world coords, convert to render center in "screen"
  // coords

  // ATE: We should call the fowllowing function but I'm putting it here
  // verbatim for speed
  // FromCellToScreenCoordinates( sDistToCenterX , sDistToCenterY,
  // &sScreenCenterX, &sScreenCenterY );
  sWorldCenterX = ((sDistToCenterX + (2 * sDistToCenterY)) / 4);
  sWorldCenterY = ((2 * sDistToCenterY) - sDistToCenterX) / 4;

  // Goto center again
  sWorldCenterX += gCenterWorldX;
  sWorldCenterY += gCenterWorldY;

  return GETWORLDINDEXFROMWORLDCOORDS(sWorldCenterY, sWorldCenterX);
}

// UTILITY FUNTIONS

int32_t OutOfBounds(int16_t sGridno, int16_t sProposedGridno) {
  int16_t sMod, sPropMod;

  // get modulas of our origin
  sMod = sGridno % MAXCOL;

  if (sMod != 0)                   // if we're not on leftmost grid
    if (sMod != RIGHTMOSTGRID)     // if we're not on rightmost grid
      if (sGridno < LASTROWSTART)  // if we're above bottom row
        if (sGridno > MAXCOL)      // if we're below top row
          // Everything's OK - we're not on the edge of the map
          return (FALSE);

  // if we've got this far, there's a potential problem - check it out!

  if (sProposedGridno < 0) return (TRUE);

  sPropMod = sProposedGridno % MAXCOL;

  if (sMod == 0 && sPropMod == RIGHTMOSTGRID)
    return (TRUE);
  else if (sMod == RIGHTMOSTGRID && sPropMod == 0)
    return (TRUE);
  else if (sGridno >= LASTROWSTART && sProposedGridno >= GRIDSIZE)
    return (TRUE);
  else
    return (FALSE);
}

int16_t NewGridNo(int16_t sGridno, int16_t sDirInc) {
  int16_t sProposedGridno = sGridno + sDirInc;

  // now check for out-of-bounds
  if (OutOfBounds(sGridno, sProposedGridno))
    // return ORIGINAL gridno to user
    sProposedGridno = sGridno;

  return (sProposedGridno);
}

int16_t DirectionInc(int16_t sDirection) {
  if ((sDirection < 0) || (sDirection > 7)) {
    // #ifdef BETAVERSION
    //    NumMessage("DirectionInc: Invalid direction received, = ",direction);
    // #endif

    // direction = random(8);	// replace garbage with random direction
    sDirection = 1;
  }

  return (DirIncrementer[sDirection]);
}

void CellXYToScreenXY(int16_t const sCellX, int16_t const sCellY, int16_t *const sScreenX,
                      int16_t *const sScreenY) {
  int16_t sDeltaCellX, sDeltaCellY;
  int16_t sDeltaScreenX, sDeltaScreenY;

  sDeltaCellX = sCellX - gsRenderCenterX;
  sDeltaCellY = sCellY - gsRenderCenterY;

  FromCellToScreenCoordinates(sDeltaCellX, sDeltaCellY, &sDeltaScreenX, &sDeltaScreenY);

  *sScreenX = (((gsVIEWPORT_END_X - gsVIEWPORT_START_X) / 2) + sDeltaScreenX);
  *sScreenY = (((gsVIEWPORT_END_Y - gsVIEWPORT_START_Y) / 2) + sDeltaScreenY);
}

void ConvertGridNoToXY(int16_t sGridNo, int16_t *sXPos, int16_t *sYPos) {
  *sYPos = sGridNo / WORLD_COLS;
  *sXPos = (sGridNo - (*sYPos * WORLD_COLS));
}

void ConvertGridNoToCellXY(int16_t sGridNo, int16_t *sXPos, int16_t *sYPos) {
  *sYPos = (sGridNo / WORLD_COLS);
  *sXPos = sGridNo - (*sYPos * WORLD_COLS);

  *sYPos = (*sYPos * CELL_Y_SIZE);
  *sXPos = (*sXPos * CELL_X_SIZE);
}

void ConvertGridNoToCenterCellXY(const int16_t gridno, int16_t *const x, int16_t *const y) {
  *x = gridno % WORLD_COLS * CELL_X_SIZE + CELL_X_SIZE / 2;
  *y = gridno / WORLD_COLS * CELL_Y_SIZE + CELL_Y_SIZE / 2;
}

int32_t GetRangeFromGridNoDiff(int16_t sGridNo1, int16_t sGridNo2) {
  int32_t uiDist;
  int16_t sXPos, sYPos, sXPos2, sYPos2;

  // Convert our grid-not into an XY
  ConvertGridNoToXY(sGridNo1, &sXPos, &sYPos);

  // Convert our grid-not into an XY
  ConvertGridNoToXY(sGridNo2, &sXPos2, &sYPos2);

  uiDist = (int16_t)sqrt(
      double((sXPos2 - sXPos) * (sXPos2 - sXPos) + (sYPos2 - sYPos) * (sYPos2 - sYPos)));

  return (uiDist);
}

int32_t GetRangeInCellCoordsFromGridNoDiff(int16_t sGridNo1, int16_t sGridNo2) {
  int16_t sXPos, sYPos, sXPos2, sYPos2;

  // Convert our grid-not into an XY
  ConvertGridNoToXY(sGridNo1, &sXPos, &sYPos);

  // Convert our grid-not into an XY
  ConvertGridNoToXY(sGridNo2, &sXPos2, &sYPos2);

  return ((int32_t)(sqrt(
              double((sXPos2 - sXPos) * (sXPos2 - sXPos) + (sYPos2 - sYPos) * (sYPos2 - sYPos)))) *
          CELL_X_SIZE);
}

bool IsPointInScreenRect(int16_t const x, int16_t const y, SGPRect const &r) {
  return r.iLeft <= x && x <= r.iRight && r.iTop <= y && y <= r.iBottom;
}

BOOLEAN IsPointInScreenRectWithRelative(int16_t sXPos, int16_t sYPos, SGPRect *pRect,
                                        int16_t *sXRel, int16_t *sYRel) {
  if ((sXPos >= pRect->iLeft) && (sXPos <= pRect->iRight) && (sYPos >= pRect->iTop) &&
      (sYPos <= pRect->iBottom)) {
    (*sXRel) = pRect->iLeft - sXPos;
    (*sYRel) = sYPos - (int16_t)pRect->iTop;

    return (TRUE);
  } else {
    return (FALSE);
  }
}

int16_t PythSpacesAway(int16_t sOrigin, int16_t sDest) {
  int16_t sRows, sCols, sResult;

  sRows = abs((sOrigin / MAXCOL) - (sDest / MAXCOL));
  sCols = abs((sOrigin % MAXROW) - (sDest % MAXROW));

  // apply Pythagoras's theorem for right-handed triangle:
  // dist^2 = rows^2 + cols^2, so use the square root to get the distance
  sResult = (int16_t)sqrt(double((sRows * sRows) + (sCols * sCols)));

  return (sResult);
}

int16_t SpacesAway(int16_t sOrigin, int16_t sDest) {
  int16_t sRows, sCols;

  sRows = abs((sOrigin / MAXCOL) - (sDest / MAXCOL));
  sCols = abs((sOrigin % MAXROW) - (sDest % MAXROW));

  return (std::max(sRows, sCols));
}

int16_t CardinalSpacesAway(int16_t sOrigin, int16_t sDest)
// distance away, ignoring diagonals!
{
  int16_t sRows, sCols;

  sRows = abs((sOrigin / MAXCOL) - (sDest / MAXCOL));
  sCols = abs((sOrigin % MAXROW) - (sDest % MAXROW));

  return ((int16_t)(sRows + sCols));
}

static int8_t FindNumTurnsBetweenDirs(int8_t sDir1, int8_t sDir2) {
  int16_t sDirection;
  int16_t sNumTurns = 0;

  sDirection = sDir1;

  do {
    sDirection = sDirection + QuickestDirection(sDir1, sDir2);

    if (sDirection > 7) {
      sDirection = 0;
    } else {
      if (sDirection < 0) {
        sDirection = 7;
      }
    }

    if (sDirection == sDir2) {
      break;
    }

    sNumTurns++;

    // SAFEGUARD ! - if we (somehow) do not get to were we want!
    if (sNumTurns > 100) {
      sNumTurns = 0;
      break;
    }
  } while (TRUE);

  return ((int8_t)sNumTurns);
}

bool FindHigherLevel(SOLDIERTYPE const *const s, int8_t *const out_direction) {
  if (s->bLevel > 0) return false;

  GridNo const grid_no = s->sGridNo;
  // If there is a roof over our heads, this is an ivalid
  if (FindStructure(grid_no, STRUCTURE_ROOF)) return false;

  bool found = false;
  uint8_t min_turns = 100;
  int8_t min_direction = 0;
  int8_t const starting_dir = s->bDirection;
  for (int32_t cnt = 0; cnt != 8; cnt += 2) {
    GridNo const new_grid_no = NewGridNo(grid_no, DirectionInc(cnt));
    if (!NewOKDestination(s, new_grid_no, TRUE, 1)) continue;

    // Check if this tile has a higher level
    if (!IsHeigherLevel(new_grid_no)) continue;

    // FInd how many turns we should go to get here
    int8_t const n_turns = FindNumTurnsBetweenDirs(cnt, starting_dir);
    if (min_turns <= n_turns) continue;

    found = true;
    min_turns = n_turns;
    min_direction = cnt;
  }

  if (!found) return false;

  if (out_direction) *out_direction = min_direction;
  return true;
}

bool FindLowerLevel(SOLDIERTYPE const *const s, int8_t *const out_direction) {
  if (s->bLevel == 0) return false;

  bool found = false;
  uint8_t min_turns = 100;
  int8_t min_direction = 0;
  GridNo const grid_no = s->sGridNo;
  int8_t const starting_dir = s->bDirection;
  for (int32_t dir = 0; dir != 8; dir += 2) {
    GridNo const new_grid_no = NewGridNo(grid_no, DirectionInc(dir));
    if (!NewOKDestination(s, new_grid_no, TRUE, 0)) continue;
    // Make sure there is NOT a roof here
    if (FindStructure(new_grid_no, STRUCTURE_ROOF)) continue;

    // Find how many turns we should go to get here
    int8_t const n_turns = FindNumTurnsBetweenDirs(dir, starting_dir);
    if (min_turns <= n_turns) continue;

    found = true;
    min_turns = n_turns;
    min_direction = dir;
  }

  if (!found) return false;

  if (out_direction) *out_direction = min_direction;
  return true;
}

int16_t QuickestDirection(int16_t origin, int16_t dest) {
  int16_t v1, v2;

  if (origin == dest) return (0);

  if ((abs(origin - dest)) == 4)
    return (1);  // this could be made random
  else if (origin > dest) {
    v1 = abs(origin - dest);
    v2 = (8 - origin) + dest;
    if (v1 > v2)
      return (1);
    else
      return (-1);
  } else {
    v1 = abs(origin - dest);
    v2 = (8 - dest) + origin;
    if (v1 > v2)
      return (-1);
    else
      return (1);
  }
}

int16_t ExtQuickestDirection(int16_t origin, int16_t dest) {
  int16_t v1, v2;

  if (origin == dest) return (0);

  if ((abs(origin - dest)) == 16)
    return (1);  // this could be made random
  else if (origin > dest) {
    v1 = abs(origin - dest);
    v2 = (32 - origin) + dest;
    if (v1 > v2)
      return (1);
    else
      return (-1);
  } else {
    v1 = abs(origin - dest);
    v2 = (32 - dest) + origin;
    if (v1 > v2)
      return (-1);
    else
      return (1);
  }
}

// Returns the (center ) cell coordinates in X
int16_t CenterX(int16_t sGridNo) {
  int16_t sYPos, sXPos;

  sYPos = sGridNo / WORLD_COLS;
  sXPos = (sGridNo - (sYPos * WORLD_COLS));

  return ((sXPos * CELL_X_SIZE) + (CELL_X_SIZE / 2));
}

// Returns the (center ) cell coordinates in Y
int16_t CenterY(int16_t sGridNo) {
  int16_t sYPos;

  sYPos = sGridNo / WORLD_COLS;

  return ((sYPos * CELL_Y_SIZE) + (CELL_Y_SIZE / 2));
}

BOOLEAN GridNoOnVisibleWorldTile(int16_t sGridNo) {
  int16_t sWorldX;
  int16_t sWorldY;
  GetAbsoluteScreenXYFromMapPos(sGridNo, &sWorldX, &sWorldY);

  if (sWorldX > 0 && sWorldX < (gsTRX - gsTLX - 20) && sWorldY > 20 &&
      sWorldY < (gsBLY - gsTLY - 20)) {
    return (TRUE);
  }

  return (FALSE);
}

BOOLEAN GridNoOnEdgeOfMap(int16_t sGridNo, int8_t *pbDirection) {
  int8_t bDir;

  // check NE, SE, SW, NW because of tilt of isometric display

  for (bDir = NORTHEAST; bDir < NUM_WORLD_DIRECTIONS; bDir += 2) {
    if (gubWorldMovementCosts[(sGridNo + DirectionInc(bDir))][bDir][0] == TRAVELCOST_OFF_MAP)
    // if ( !GridNoOnVisibleWorldTile( (int16_t) (sGridNo + DirectionInc( bDir ) )
    // ) )
    {
      *pbDirection = bDir;
      return (TRUE);
    }
  }
  return (FALSE);
}

BOOLEAN FindFenceJumpDirection(SOLDIERTYPE const *const pSoldier, int8_t *const out_direction) {
  int32_t cnt;
  int16_t sNewGridNo, sOtherSideOfFence;
  BOOLEAN fFound = FALSE;
  uint8_t bMinNumTurns = 100;
  int8_t bNumTurns;
  int8_t bMinDirection = 0;

  GridNo const sGridNo = pSoldier->sGridNo;
  // IF there is a fence in this gridno, return false!
  if (IsJumpableFencePresentAtGridno(sGridNo)) {
    return (FALSE);
  }

  // LOOP THROUGH ALL 8 DIRECTIONS
  int8_t const bStartingDir = pSoldier->bDirection;
  for (cnt = 0; cnt < 8; cnt += 2) {
    // go out *2* tiles
    sNewGridNo = NewGridNo((uint16_t)sGridNo, (uint16_t)DirectionInc((uint8_t)cnt));
    sOtherSideOfFence = NewGridNo((uint16_t)sNewGridNo, (uint16_t)DirectionInc((uint8_t)cnt));

    if (NewOKDestination(pSoldier, sOtherSideOfFence, TRUE, 0)) {
      // ATE: Check if there is somebody waiting here.....

      // Check if we have a fence here
      if (IsJumpableFencePresentAtGridno(sNewGridNo)) {
        fFound = TRUE;

        // FInd how many turns we should go to get here
        bNumTurns = FindNumTurnsBetweenDirs((int8_t)cnt, bStartingDir);

        if (bNumTurns < bMinNumTurns) {
          bMinNumTurns = bNumTurns;
          bMinDirection = (int8_t)cnt;
        }
      }
    }
  }

  if (fFound) {
    if (out_direction) *out_direction = bMinDirection;
    return (TRUE);
  }

  return (FALSE);
}

// Simply chooses a random gridno within valid boundaries (for dropping things
// in unloaded sectors)
int16_t RandomGridNo() {
  int32_t iMapXPos, iMapYPos, iMapIndex;
  do {
    iMapXPos = Random(WORLD_COLS);
    iMapYPos = Random(WORLD_ROWS);
    iMapIndex = iMapYPos * WORLD_COLS + iMapXPos;
  } while (!GridNoOnVisibleWorldTile((int16_t)iMapIndex));
  return (int16_t)iMapIndex;
}
