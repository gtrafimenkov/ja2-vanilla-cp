// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Tactical/DisplayCover.h"

#include <algorithm>
#include <string.h>

#include "GameSettings.h"
#include "Macro.h"
#include "SGP/Debug.h"
#include "Strategic/GameClock.h"
#include "Strategic/StrategicMap.h"
#include "Tactical/AnimationControl.h"
#include "Tactical/Interface.h"
#include "Tactical/Items.h"
#include "Tactical/LOS.h"
#include "Tactical/OppList.h"
#include "Tactical/Overhead.h"
#include "Tactical/PathAI.h"
#include "Tactical/SoldierFind.h"
#include "Tactical/Weapons.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/RenderFun.h"
#include "TileEngine/RenderWorld.h"
#include "TileEngine/TileDat.h"
#include "TileEngine/WorldMan.h"
#include "Utils/FontControl.h"
#include "Utils/Message.h"
#include "Utils/Text.h"

#define DC_MAX_COVER_RANGE 31

#define DC__SOLDIER_VISIBLE_RANGE 31

#define DC__MIN_SIZE 4
#define DC__MAX_SIZE 11

struct BEST_COVER_STRUCT {
  int16_t sGridNo;
  int8_t bCover;  //% chance that the gridno is fully covered.  ie 100 if safe, 0
                  // is has no cover
};

struct VISIBLE_TO_SOLDIER_STRUCT {
  int16_t sGridNo;
  int8_t bVisibleToSoldier;
  BOOLEAN fRoof;
};

enum { DC__SEE_NO_STANCES, DC__SEE_1_STANCE, DC__SEE_2_STANCE, DC__SEE_3_STANCE };

static BEST_COVER_STRUCT gCoverRadius[DC_MAX_COVER_RANGE][DC_MAX_COVER_RANGE];
static int16_t gsLastCoverGridNo = NOWHERE;
static int16_t gsLastSoldierGridNo = NOWHERE;
static int8_t gbLastStance = -1;

static VISIBLE_TO_SOLDIER_STRUCT gVisibleToSoldierStruct[DC__SOLDIER_VISIBLE_RANGE]
                                                        [DC__SOLDIER_VISIBLE_RANGE];
static int16_t gsLastVisibleToSoldierGridNo = NOWHERE;

static void AddCoverTileToEachGridNo();
static void CalculateCoverInRadiusAroundGridno(int16_t sTargetGridNo, int8_t bSearchRange);
static int8_t GetCurrentMercForDisplayCoverStance();

void DisplayCoverOfSelectedGridNo() {
  SOLDIERTYPE const *const sel = GetSelectedMan();
  // Only allowed in if there is someone selected
  if (!sel) return;

  // if the cursor is in a the tactical map
  GridNo const sGridNo = GetMouseMapPos();
  if (sGridNo == NOWHERE) return;

  int8_t const bStance = GetCurrentMercForDisplayCoverStance();

  // if the gridno is different then the last one that was displayed
  if (gsLastCoverGridNo == sGridNo && gbLastStance == bStance &&
      gsLastCoverGridNo == sel->sGridNo) {
    return;
  }

  // if the cover is currently being displayed
  if (gsLastCoverGridNo != NOWHERE || gbLastStance != -1 || gsLastSoldierGridNo != NOWHERE) {
    // remove the gridnos
    RemoveCoverOfSelectedGridNo();
  } else {
    // if it is the first time in here

    // pop up a message to say we are in the display cover routine
    ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, zNewTacticalMessages[TCTL_MSG__DISPLAY_COVER]);
  }

  gbLastStance = bStance;
  gsLastCoverGridNo = sGridNo;
  gsLastSoldierGridNo = sel->sGridNo;

  // Fill the array of gridno and cover values
  CalculateCoverInRadiusAroundGridno(sGridNo, gGameSettings.ubSizeOfDisplayCover);

  // Add the graphics to each gridno
  AddCoverTileToEachGridNo();

  // Re-render the scene!
  SetRenderFlags(RENDER_FLAG_FULL);
}

static void AddCoverObjectToWorld(int16_t sGridNo, uint16_t usGraphic, BOOLEAN fRoof);

static void AddCoverTileToEachGridNo() {
  BOOLEAN const roof = gsInterfaceLevel != I_GROUND_LEVEL;
  for (uint32_t y = 0; y < DC_MAX_COVER_RANGE; ++y) {
    for (uint32_t x = 0; x < DC_MAX_COVER_RANGE; ++x) {
      BEST_COVER_STRUCT const &cr = gCoverRadius[x][y];
      int8_t const cover = cr.bCover;
      if (cover == -1) continue;  // Valid cover?
      Assert(0 <= cover && cover <= 100);

      uint16_t const gfx = cover <= 20   ? SPECIALTILE_COVER_1
                           : cover <= 40 ? SPECIALTILE_COVER_2
                           : cover <= 60 ? SPECIALTILE_COVER_3
                           : cover <= 80 ? SPECIALTILE_COVER_4
                                         : SPECIALTILE_COVER_5;
      AddCoverObjectToWorld(cr.sGridNo, gfx, roof);
    }
  }
}

static void RemoveCoverObjectFromWorld(int16_t sGridNo, uint16_t usGraphic, BOOLEAN fRoof);

void RemoveCoverOfSelectedGridNo() {
  if (gsLastCoverGridNo == NOWHERE) return;

  BOOLEAN const roof = gsInterfaceLevel != I_GROUND_LEVEL;
  for (uint32_t y = 0; y < DC_MAX_COVER_RANGE; ++y) {
    for (uint32_t x = 0; x < DC_MAX_COVER_RANGE; ++x) {
      BEST_COVER_STRUCT const &cr = gCoverRadius[x][y];
      int8_t const cover = cr.bCover;
      if (cover == -1) continue;  // Valid cover?
      Assert(0 <= cover && cover <= 100);

      uint16_t const gfx = cover <= 20   ? SPECIALTILE_COVER_1
                           : cover <= 40 ? SPECIALTILE_COVER_2
                           : cover <= 60 ? SPECIALTILE_COVER_3
                           : cover <= 80 ? SPECIALTILE_COVER_4
                                         : SPECIALTILE_COVER_5;
      RemoveCoverObjectFromWorld(cr.sGridNo, gfx, roof);
    }
  }

  // Re-render the scene!
  SetRenderFlags(RENDER_FLAG_FULL);

  gsLastCoverGridNo = NOWHERE;
  gbLastStance = -1;
  gsLastSoldierGridNo = NOWHERE;
}

static int8_t CalcCoverForGridNoBasedOnTeamKnownEnemies(const SOLDIERTYPE *pSoldier,
                                                        int16_t sTargetGridNo, int8_t bStance);
static SOLDIERTYPE *GetCurrentMercForDisplayCover();

static void CalculateCoverInRadiusAroundGridno(int16_t const sTargetGridNo, int8_t search_range) {
  // clear out the array first
  for (int16_t y = 0; y < DC_MAX_COVER_RANGE; ++y) {
    for (int16_t x = 0; x < DC_MAX_COVER_RANGE; ++x) {
      BEST_COVER_STRUCT &cr = gCoverRadius[x][y];
      cr.sGridNo = -1;
      cr.bCover = -1;
    }
  }

  if (search_range > DC_MAX_COVER_RANGE / 2) search_range = DC_MAX_COVER_RANGE / 2;

  // Determine maximum horizontal and vertical limits
  int16_t const max_left = std::min((int16_t)search_range, (int16_t)(sTargetGridNo % MAXCOL));
  int16_t const max_right =
      std::min((int16_t)search_range, (int16_t)(MAXCOL - 1 - sTargetGridNo % MAXCOL));
  int16_t const max_up = std::min((int16_t)search_range, (int16_t)(sTargetGridNo / MAXROW));
  int16_t const max_down =
      std::min((int16_t)search_range, (int16_t)(MAXROW - 1 - sTargetGridNo / MAXROW));

  // Find out which tiles around the location are reachable
  LocalReachableTest(sTargetGridNo, search_range);

  SOLDIERTYPE const *const pSoldier = GetCurrentMercForDisplayCover();

  int16_t x = 0;
  int16_t y = 0;

  // Determine the stance to use
  int8_t const stance = GetCurrentMercForDisplayCoverStance();

  // loop through all the gridnos that we are interested in
  for (int16_t sYOffset = -max_up; sYOffset <= max_down; ++sYOffset) {
    for (int16_t sXOffset = -max_left; sXOffset <= max_right; ++sXOffset) {
      int16_t const sGridNo = sTargetGridNo + sXOffset + (MAXCOL * sYOffset);

      gCoverRadius[x][y].sGridNo = sGridNo;

      if (!GridNoOnScreen(sGridNo)) continue;

      // if we are to display cover for the roofs, and there is a roof above us
      if (gsInterfaceLevel == I_ROOF_LEVEL && !FlatRoofAboveGridNo(sGridNo)) {
        continue;
      }

      if (!(gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE)) {
        // skip to the next gridno
        ++x;
        continue;
      }

      // if someone (visible) is there, skip
      // Check both bottom level, and top level
      SOLDIERTYPE const *tgt = WhoIsThere2(sGridNo, 0);
      if (!tgt) tgt = WhoIsThere2(sGridNo, 1);
      // if someone is here, and they are an enemy, skip over them
      if (tgt && tgt->bVisible == TRUE && tgt->bTeam != pSoldier->bTeam) {
        continue;
      }

      // Calculate the cover for this gridno
      gCoverRadius[x][y].bCover =
          CalcCoverForGridNoBasedOnTeamKnownEnemies(pSoldier, sGridNo, stance);
      ++x;
    }
    ++y;
    x = 0;
  }
}

static int8_t CalcCoverForGridNoBasedOnTeamKnownEnemies(SOLDIERTYPE const *const pSoldier,
                                                        int16_t const sTargetGridNo,
                                                        int8_t const bStance) {
  // loop through all the enemies and determine the cover
  int32_t iTotalCoverPoints = 0;
  int8_t bNumEnemies = 0;
  int32_t iHighestValue = 0;
  FOR_EACH_MERC(i) {
    SOLDIERTYPE *const pOpponent = *i;

    if (pOpponent->bLife < OKLIFE) continue;

    // if this man is neutral / on the same side, he's not an opponent
    if (CONSIDERED_NEUTRAL(pSoldier, pOpponent)) continue;
    if (pSoldier->bSide == pOpponent->bSide) continue;

    int8_t const *const pbPersOL = pSoldier->bOppList + pOpponent->ubID;
    int8_t const *const pbPublOL = gbPublicOpplist[OUR_TEAM] + pOpponent->ubID;

    // if this opponent is unknown personally and publicly
    if (*pbPersOL != SEEN_CURRENTLY && *pbPersOL != SEEN_THIS_TURN && *pbPublOL != SEEN_CURRENTLY &&
        *pbPublOL != SEEN_THIS_TURN) {
      continue;
    }

    uint16_t const usRange = GetRangeInCellCoordsFromGridNoDiff(pOpponent->sGridNo, sTargetGridNo);
    uint16_t const usSightLimit = DistanceVisible(
        pOpponent, DIRECTION_IRRELEVANT, DIRECTION_IRRELEVANT, sTargetGridNo, pSoldier->bLevel);

    if (usRange > usSightLimit * CELL_X_SIZE) continue;

    // if actual LOS check fails, then chance to hit is 0, ignore this guy
    if (SoldierToVirtualSoldierLineOfSightTest(pOpponent, sTargetGridNo, pSoldier->bLevel, bStance,
                                               usSightLimit, TRUE) == 0) {
      continue;
    }

    int32_t const iGetThrough = SoldierToLocationChanceToGetThrough(
        pOpponent, sTargetGridNo, pSoldier->bLevel, bStance, NULL);
    uint16_t const usMaxRange =
        WeaponInHand(pOpponent) ? GunRange(pOpponent->inv[HANDPOS]) : Weapon[GLOCK_18].usRange;
    int32_t const iBulletGetThrough = std::min(
        std::max((int32_t)(((usMaxRange - usRange) / (float)usMaxRange + .3) * 100), 0), 100);
    if (iBulletGetThrough > 5 && iGetThrough > 0) {
      int32_t const iCover = iGetThrough * iBulletGetThrough / 100;
      if (iHighestValue < iCover) iHighestValue = iCover;

      iTotalCoverPoints += iCover;
      ++bNumEnemies;
    }
  }

  int8_t bPercentCoverForGridno;
  if (bNumEnemies == 0) {
    bPercentCoverForGridno = 100;
  } else {
    bPercentCoverForGridno = iTotalCoverPoints / bNumEnemies;
    int32_t const iTemp = bPercentCoverForGridno - (iHighestValue / bNumEnemies) + iHighestValue;
    bPercentCoverForGridno = 100 - std::min(iTemp, 100);
  }
  return bPercentCoverForGridno;
}

static void AddCoverObjectToWorld(int16_t const sGridNo, uint16_t const usGraphic,
                                  BOOLEAN const fRoof) {
  LEVELNODE *const n =
      fRoof ? AddOnRoofToHead(sGridNo, usGraphic) : AddObjectToHead(sGridNo, usGraphic);

  n->uiFlags |= LEVELNODE_REVEAL;

  if (NightTime()) {
    n->ubShadeLevel = DEFAULT_SHADE_LEVEL;
    n->ubNaturalShadeLevel = DEFAULT_SHADE_LEVEL;
  }
}

static void RemoveCoverObjectFromWorld(int16_t sGridNo, uint16_t usGraphic, BOOLEAN fRoof) {
  if (fRoof) {
    RemoveOnRoof(sGridNo, usGraphic);
  } else {
    RemoveObject(sGridNo, usGraphic);
  }
}

static SOLDIERTYPE *GetCurrentMercForDisplayCover() { return GetSelectedMan(); }

static int8_t GetCurrentMercForDisplayCoverStance() {
  const SOLDIERTYPE *const pSoldier = GetCurrentMercForDisplayCover();
  int8_t bStance;

  switch (pSoldier->usUIMovementMode) {
    case PRONE:
    case CRAWLING:
      bStance = ANIM_PRONE;
      break;

    case KNEEL_DOWN:
    case SWATTING:
    case CROUCHING:
      bStance = ANIM_CROUCH;
      break;

    case WALKING:
    case RUNNING:
    case STANDING:
      bStance = ANIM_STAND;
      break;

    default:
      bStance = ANIM_CROUCH;
      break;
  }

  return (bStance);
}

void DisplayRangeToTarget(SOLDIERTYPE const *const s, int16_t const sTargetGridNo) {
  if (sTargetGridNo == NOWHERE || sTargetGridNo == 0) return;

  // Get the range to the target location
  uint16_t const usRange = GetRangeInCellCoordsFromGridNoDiff(s->sGridNo, sTargetGridNo) / 10;

  if (WeaponInHand(s)) {
    // display a string with the weapons range, then range to target
    ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE,
              zNewTacticalMessages[TCTL_MSG__RANGE_TO_TARGET_AND_GUN_RANGE],
              Weapon[s->inv[HANDPOS].usItem].usRange / 10, usRange);
  } else {
    // display a string with the range to target
    ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, zNewTacticalMessages[TCTL_MSG__RANGE_TO_TARGET],
              usRange);
  }

  // if the target is out of the mercs gun range or knife
  if (!InRange(s, sTargetGridNo)) {
    uint16_t const item_class = Item[s->inv[HANDPOS].usItem].usItemClass;
    if (item_class == IC_GUN || item_class == IC_THROWING_KNIFE) {
      // Display a warning saying so
      ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, TacticalStr[OUT_OF_RANGE_STRING]);
    }
  }
}

static void AddVisibleToSoldierToEachGridNo();
static void CalculateVisibleToSoldierAroundGridno(int16_t sTargetGridNo, int8_t bSearchRange);

void DisplayGridNoVisibleToSoldierGrid() {
  //	int8_t	bStance;

  const SOLDIERTYPE *const sel = GetSelectedMan();
  // Only allowed in if there is someone selected
  if (sel == NULL) return;

  // if the cursor is in a the tactical map
  const GridNo sGridNo = GetMouseMapPos();
  if (sGridNo != NOWHERE) {
    // if the gridno is different then the last one that was displayed
    if (sGridNo != gsLastVisibleToSoldierGridNo || sel->sGridNo != gsLastSoldierGridNo) {
      // if the cover is currently being displayed
      if (gsLastVisibleToSoldierGridNo != NOWHERE || gsLastSoldierGridNo != NOWHERE) {
        // remove the gridnos
        RemoveVisibleGridNoAtSelectedGridNo();
      } else {
        // pop up a message to say we are in the display cover routine
        ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, zNewTacticalMessages[TCTL_MSG__LOS]);
        // increment the display LOS counter ( just seeing how many times people
        // use it ) gJa25SaveStruct.uiDisplayLosCounter++;
      }

      gsLastVisibleToSoldierGridNo = sGridNo;
      gsLastSoldierGridNo = sel->sGridNo;

      // Fill the array of gridno and cover values
      CalculateVisibleToSoldierAroundGridno(sGridNo, gGameSettings.ubSizeOfLOS);

      // Add the graphics to each gridno
      AddVisibleToSoldierToEachGridNo();

      // Re-render the scene!
      SetRenderFlags(RENDER_FLAG_FULL);
    }
  }
}

static int8_t CalcIfSoldierCanSeeGridNo(const SOLDIERTYPE *pSoldier, int16_t sTargetGridNo,
                                        BOOLEAN fRoof);
static BOOLEAN IsTheRoofVisible(int16_t sGridNo);

static void CalculateVisibleToSoldierAroundGridno(int16_t sTargetGridNo, int8_t bSearchRange) {
  int16_t sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;
  int16_t sGridNo;
  int16_t sCounterX, sCounterY;
  BOOLEAN fRoof = FALSE;

  // clear out the struct
  memset(gVisibleToSoldierStruct, 0,
         sizeof(VISIBLE_TO_SOLDIER_STRUCT) * DC__SOLDIER_VISIBLE_RANGE * DC__SOLDIER_VISIBLE_RANGE);

  if (bSearchRange > (DC_MAX_COVER_RANGE / 2)) bSearchRange = (DC_MAX_COVER_RANGE / 2);

  // determine maximum horizontal limits
  sMaxLeft = std::min((int16_t)bSearchRange, (int16_t)(sTargetGridNo % MAXCOL));
  sMaxRight = std::min((int16_t)bSearchRange, (int16_t)(MAXCOL - ((sTargetGridNo % MAXCOL) + 1)));

  // determine maximum vertical limits
  sMaxUp = std::min((int16_t)bSearchRange, (int16_t)(sTargetGridNo / MAXROW));
  sMaxDown = std::min((int16_t)bSearchRange, (int16_t)(MAXROW - ((sTargetGridNo / MAXROW) + 1)));

  const SOLDIERTYPE *const pSoldier = GetCurrentMercForDisplayCover();

  sCounterX = 0;
  sCounterY = 0;

  // loop through all the gridnos that we are interested in
  for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++) {
    sCounterX = 0;
    for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++) {
      sGridNo = sTargetGridNo + sXOffset + (MAXCOL * sYOffset);
      fRoof = FALSE;

      // record the gridno
      gVisibleToSoldierStruct[sCounterX][sCounterY].sGridNo = sGridNo;

      // if the gridno is NOT on screen
      if (!GridNoOnScreen(sGridNo)) {
        continue;
      }

      // is there a roof above this gridno
      if (FlatRoofAboveGridNo(sGridNo)) {
        if (IsTheRoofVisible(sGridNo) && gbWorldSectorZ == 0) {
          fRoof = TRUE;
        }

        // if wer havent explored the area yet and we are underground, dont show
        // cover
        else if (!(gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REVEALED) &&
                 gbWorldSectorZ != 0) {
          continue;
        }
      }

      /*
                              //if we are to display cover for the roofs, and
         there is a roof above us if( gsInterfaceLevel == I_ROOF_LEVEL &&
         !FlatRoofAboveGridNo( sGridNo ) )
                              {
                                      continue;
                              }
      */
      /*
                              // if someone (visible) is there, skip
                              //Check both bottom level, and top level
                              SOLDIERTYPE* tgt = WhoIsThere2(sGridNo, 0);
                              if (tgt == NULL) tgt = WhoIsThere2(sGridNo, 1);
                              //if someone is here, and they are an enemy, skip
         over them if (tgt != NULL && tgt->bVisible == TRUE && tgt->bTeam !=
         pSoldier->bTeam)
                              {
                                      continue;
                              }

                              //Calculate the cover for this gridno
                              gCoverRadius[ sCounterX ][ sCounterY ].bCover =
         CalcCoverForGridNoBasedOnTeamKnownEnemies( pSoldier, sGridNo, bStance
         );
      */

      gVisibleToSoldierStruct[sCounterX][sCounterY].bVisibleToSoldier =
          CalcIfSoldierCanSeeGridNo(pSoldier, sGridNo, fRoof);
      gVisibleToSoldierStruct[sCounterX][sCounterY].fRoof = fRoof;
      sCounterX++;
    }

    sCounterY++;
  }
}

static void AddVisibleToSoldierToEachGridNo() {
  uint32_t uiCntX, uiCntY;
  int8_t bVisibleToSoldier = 0;
  BOOLEAN fRoof;
  int16_t sGridNo;

  // loop through all the gridnos
  for (uiCntY = 0; uiCntY < DC_MAX_COVER_RANGE; uiCntY++) {
    for (uiCntX = 0; uiCntX < DC_MAX_COVER_RANGE; uiCntX++) {
      bVisibleToSoldier = gVisibleToSoldierStruct[uiCntX][uiCntY].bVisibleToSoldier;
      if (bVisibleToSoldier == -1) {
        continue;
      }

      fRoof = gVisibleToSoldierStruct[uiCntX][uiCntY].fRoof;
      sGridNo = gVisibleToSoldierStruct[uiCntX][uiCntY].sGridNo;

      // if the soldier can easily see this gridno.  Can see all 3 positions
      if (bVisibleToSoldier == DC__SEE_3_STANCE) {
        AddCoverObjectToWorld(sGridNo, SPECIALTILE_COVER_5, fRoof);
      }

      // cant see a thing
      else if (bVisibleToSoldier == DC__SEE_NO_STANCES) {
        AddCoverObjectToWorld(gVisibleToSoldierStruct[uiCntX][uiCntY].sGridNo, SPECIALTILE_COVER_1,
                              fRoof);
      }

      // can only see prone
      else if (bVisibleToSoldier == DC__SEE_1_STANCE) {
        AddCoverObjectToWorld(gVisibleToSoldierStruct[uiCntX][uiCntY].sGridNo, SPECIALTILE_COVER_2,
                              fRoof);
      }

      // can see crouch or prone
      else if (bVisibleToSoldier == DC__SEE_2_STANCE) {
        AddCoverObjectToWorld(gVisibleToSoldierStruct[uiCntX][uiCntY].sGridNo, SPECIALTILE_COVER_3,
                              fRoof);
      }

      else {
        Assert(0);
      }
    }
  }
}

void RemoveVisibleGridNoAtSelectedGridNo() {
  uint32_t uiCntX, uiCntY;
  int8_t bVisibleToSoldier;
  BOOLEAN fRoof;

  // make sure to only remove it when its right
  if (gsLastVisibleToSoldierGridNo == NOWHERE) {
    return;
  }

  // loop through all the gridnos
  for (uiCntY = 0; uiCntY < DC_MAX_COVER_RANGE; uiCntY++) {
    for (uiCntX = 0; uiCntX < DC_MAX_COVER_RANGE; uiCntX++) {
      bVisibleToSoldier = gVisibleToSoldierStruct[uiCntX][uiCntY].bVisibleToSoldier;
      fRoof = gVisibleToSoldierStruct[uiCntX][uiCntY].fRoof;

      // if there is a valid cover at this gridno
      if (bVisibleToSoldier == DC__SEE_3_STANCE) {
        RemoveCoverObjectFromWorld(gVisibleToSoldierStruct[uiCntX][uiCntY].sGridNo,
                                   SPECIALTILE_COVER_5, fRoof);
      }

      // cant see a thing
      else if (bVisibleToSoldier == DC__SEE_NO_STANCES) {
        RemoveCoverObjectFromWorld(gVisibleToSoldierStruct[uiCntX][uiCntY].sGridNo,
                                   SPECIALTILE_COVER_1, fRoof);
      }

      // can only see prone
      else if (bVisibleToSoldier == DC__SEE_1_STANCE) {
        RemoveCoverObjectFromWorld(gVisibleToSoldierStruct[uiCntX][uiCntY].sGridNo,
                                   SPECIALTILE_COVER_2, fRoof);
      }

      // can see crouch or prone
      else if (bVisibleToSoldier == DC__SEE_2_STANCE) {
        RemoveCoverObjectFromWorld(gVisibleToSoldierStruct[uiCntX][uiCntY].sGridNo,
                                   SPECIALTILE_COVER_3, fRoof);
      }

      else {
        Assert(0);
      }
    }
  }

  // Re-render the scene!
  SetRenderFlags(RENDER_FLAG_FULL);

  gsLastVisibleToSoldierGridNo = NOWHERE;
  gsLastSoldierGridNo = NOWHERE;
}

static int8_t CalcIfSoldierCanSeeGridNo(const SOLDIERTYPE *pSoldier, int16_t sTargetGridNo,
                                        BOOLEAN fRoof) {
  int8_t bRetVal = 0;
  int32_t iLosForGridNo = 0;
  uint16_t usSightLimit = 0;
  BOOLEAN bAware = FALSE;

  const SOLDIERTYPE *const tgt = WhoIsThere2(sTargetGridNo, fRoof ? 1 : 0);
  if (tgt != NULL) {
    const int8_t *const pPersOL = &pSoldier->bOppList[tgt->ubID];
    const int8_t *const pbPublOL = &gbPublicOpplist[pSoldier->bTeam][tgt->ubID];

    // if soldier is known about (SEEN or HEARD within last few turns)
    if (*pPersOL || *pbPublOL) {
      bAware = TRUE;
    }
  }

  usSightLimit =
      DistanceVisible(pSoldier, DIRECTION_IRRELEVANT, DIRECTION_IRRELEVANT, sTargetGridNo, fRoof);

  //
  // Prone
  //
  iLosForGridNo = SoldierToVirtualSoldierLineOfSightTest(pSoldier, sTargetGridNo, fRoof, ANIM_PRONE,
                                                         (uint8_t)usSightLimit, bAware);
  if (iLosForGridNo != 0) {
    bRetVal++;
  }

  //
  // Crouch
  //
  iLosForGridNo = SoldierToVirtualSoldierLineOfSightTest(
      pSoldier, sTargetGridNo, fRoof, ANIM_CROUCH, (uint8_t)usSightLimit, bAware);
  if (iLosForGridNo != 0) {
    bRetVal++;
  }

  //
  // Standing
  //
  iLosForGridNo = SoldierToVirtualSoldierLineOfSightTest(pSoldier, sTargetGridNo, fRoof, ANIM_STAND,
                                                         (uint8_t)usSightLimit, bAware);
  if (iLosForGridNo != 0) {
    bRetVal++;
  }

  return (bRetVal);
}

static BOOLEAN IsTheRoofVisible(int16_t sGridNo) {
  if (GetRoom(sGridNo) == NO_ROOM) return FALSE;

  if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REVEALED) {
    if (gTacticalStatus.uiFlags & SHOW_ALL_ROOFS)
      return (TRUE);
    else
      return (FALSE);
  } else {
    return (TRUE);
  }
}

void ChangeSizeOfDisplayCover(int32_t iNewSize) {
  // if the new size is smaller or greater, scale it
  if (iNewSize < DC__MIN_SIZE) {
    iNewSize = DC__MIN_SIZE;
  } else if (iNewSize > DC__MAX_SIZE) {
    iNewSize = DC__MAX_SIZE;
  }

  // Set new size
  gGameSettings.ubSizeOfDisplayCover = (uint8_t)iNewSize;

  // redisplay the cover
  RemoveCoverOfSelectedGridNo();
  DisplayCoverOfSelectedGridNo();
}

void ChangeSizeOfLOS(int32_t iNewSize) {
  // if the new size is smaller or greater, scale it
  if (iNewSize < DC__MIN_SIZE) {
    iNewSize = DC__MIN_SIZE;
  } else if (iNewSize > DC__MAX_SIZE) {
    iNewSize = DC__MAX_SIZE;
  }

  // Set new size
  gGameSettings.ubSizeOfLOS = (uint8_t)iNewSize;

  // ReDisplay the los
  RemoveVisibleGridNoAtSelectedGridNo();
  DisplayGridNoVisibleToSoldierGrid();
}
