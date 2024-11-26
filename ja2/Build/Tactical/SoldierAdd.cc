#include "Tactical/SoldierAdd.h"

#include <math.h>
#include <string.h>

#include "SGP/Debug.h"
#include "SGP/Random.h"
#include "Strategic/Meanwhile.h"
#include "Strategic/Strategic.h"
#include "Strategic/StrategicMap.h"
#include "Tactical/AnimationControl.h"
#include "Tactical/FOV.h"
#include "Tactical/InterfacePanels.h"
#include "Tactical/MapInformation.h"
#include "Tactical/Overhead.h"
#include "Tactical/OverheadTypes.h"
#include "Tactical/PathAI.h"
#include "Tactical/SoldierControl.h"
#include "Tactical/SoldierCreate.h"
#include "Tactical/SoldierInitList.h"
#include "Tactical/SoldierMacros.h"
#include "TileEngine/ExitGrids.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/RenderFun.h"
#include "TileEngine/Structure.h"

// SO, STEPS IN CREATING A MERC!

// 1 ) Setup the SOLDIERCREATE_STRUCT
//			Among other things, this struct needs a sSectorX, sSectorY,
// and a valid InsertionDirection 			and InsertionGridNo. 			This
// GridNo will be determined by a prevouis function that will examine the sector Infomration
// regarding placement positions and pick one 2 ) Call TacticalCreateSoldier() which will create our
// soldier
//			What it does is:	Creates a soldier in the Menptr[]
// array.
// Allocates the Animation cache for this merc Loads up the intial aniamtion file Creates initial
// palettes, etc And other cool things. Now we have an allocated soldier, we just need to set him in
// the world! 3) When we want them in the world, call AddSoldierToSector().
//			This function sets the graphic in the world, lighting
// effects, etc 			It also formally adds it to the TacticalSoldier slot and
// interface panel slot.

// Kris:  modified to actually path from sweetspot to gridno.  Previously, it
// only checked if the destination was sittable (though it was possible that that
// location would be trapped.
uint16_t FindGridNoFromSweetSpot(const SOLDIERTYPE *const pSoldier, const int16_t sSweetGridNo,
                                 const int8_t ubRadius) {
  int16_t sTop, sBottom;
  int16_t sLeft, sRight;
  int16_t cnt1, cnt2;
  int16_t sGridNo;
  int32_t uiRange, uiLowestRange = 999999;
  int32_t leftmost;
  SOLDIERTYPE soldier;
  uint8_t ubSaveNPCAPBudget;
  uint8_t ubSaveNPCDistLimit;

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
  soldier.sGridNo = sSweetGridNo;

  sTop = ubRadius;
  sBottom = -ubRadius;
  sLeft = -ubRadius;
  sRight = ubRadius;

  // clear the mapelements of potential residue MAPELEMENT_REACHABLE flags
  // in the square region.
  // ATE: CHECK FOR BOUNDARIES!!!!!!
  for (cnt1 = sBottom; cnt1 <= sTop; cnt1++) {
    leftmost = ((sSweetGridNo + (WORLD_COLS * cnt1)) / WORLD_COLS) * WORLD_COLS;

    for (cnt2 = sLeft; cnt2 <= sRight; cnt2++) {
      sGridNo = sSweetGridNo + (WORLD_COLS * cnt1) + cnt2;
      if (sGridNo >= 0 && sGridNo < WORLD_MAX && sGridNo >= leftmost &&
          sGridNo < (leftmost + WORLD_COLS)) {
        gpWorldLevelData[sGridNo].uiFlags &= (~MAPELEMENT_REACHABLE);
      }
    }
  }

  // Now, find out which of these gridnos are reachable
  //(use the fake soldier and the pathing settings)
  FindBestPath(&soldier, NOWHERE, 0, WALKING, COPYREACHABLE,
               (PATH_IGNORE_PERSON_AT_DEST | PATH_THROUGH_PEOPLE));

  uiLowestRange = 999999;

  int16_t sLowestGridNo = NOWHERE;
  for (cnt1 = sBottom; cnt1 <= sTop; cnt1++) {
    leftmost = ((sSweetGridNo + (WORLD_COLS * cnt1)) / WORLD_COLS) * WORLD_COLS;

    for (cnt2 = sLeft; cnt2 <= sRight; cnt2++) {
      sGridNo = sSweetGridNo + (WORLD_COLS * cnt1) + cnt2;
      if (sGridNo >= 0 && sGridNo < WORLD_MAX && sGridNo >= leftmost &&
          sGridNo < (leftmost + WORLD_COLS) &&
          gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE) {
        // Go on sweet stop
        if (NewOKDestination(pSoldier, sGridNo, TRUE, pSoldier->bLevel)) {
          // ATE: INstead of using absolute range, use the path cost!
          // uiRange = PlotPath(&soldier, sGridNo, NO_COPYROUTE, NO_PLOT,
          // WALKING, 50);
          uiRange = CardinalSpacesAway(sSweetGridNo, sGridNo);

          //	if ( uiRange == 0 )
          //	{
          //		uiRange = 999999;
          //	}

          if (uiRange < uiLowestRange) {
            sLowestGridNo = sGridNo;
            uiLowestRange = uiRange;
          }
        }
      }
    }
  }
  gubNPCAPBudget = ubSaveNPCAPBudget;
  gubNPCDistLimit = ubSaveNPCDistLimit;
  return sLowestGridNo;
}

uint16_t FindGridNoFromSweetSpotThroughPeople(const SOLDIERTYPE *const pSoldier,
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
  soldier.bTeam = pSoldier->bTeam;
  soldier.sGridNo = sSweetGridNo;

  sTop = ubRadius;
  sBottom = -ubRadius;
  sLeft = -ubRadius;
  sRight = ubRadius;

  // clear the mapelements of potential residue MAPELEMENT_REACHABLE flags
  // in the square region.
  // ATE: CHECK FOR BOUNDARIES!!!!!!
  for (cnt1 = sBottom; cnt1 <= sTop; cnt1++) {
    leftmost = ((sSweetGridNo + (WORLD_COLS * cnt1)) / WORLD_COLS) * WORLD_COLS;

    for (cnt2 = sLeft; cnt2 <= sRight; cnt2++) {
      sGridNo = sSweetGridNo + (WORLD_COLS * cnt1) + cnt2;
      if (sGridNo >= 0 && sGridNo < WORLD_MAX && sGridNo >= leftmost &&
          sGridNo < (leftmost + WORLD_COLS)) {
        gpWorldLevelData[sGridNo].uiFlags &= (~MAPELEMENT_REACHABLE);
      }
    }
  }

  // Now, find out which of these gridnos are reachable
  //(use the fake soldier and the pathing settings)
  FindBestPath(&soldier, NOWHERE, 0, WALKING, COPYREACHABLE,
               (PATH_IGNORE_PERSON_AT_DEST | PATH_THROUGH_PEOPLE));

  uiLowestRange = 999999;

  int16_t sLowestGridNo = NOWHERE;
  for (cnt1 = sBottom; cnt1 <= sTop; cnt1++) {
    leftmost = ((sSweetGridNo + (WORLD_COLS * cnt1)) / WORLD_COLS) * WORLD_COLS;

    for (cnt2 = sLeft; cnt2 <= sRight; cnt2++) {
      sGridNo = sSweetGridNo + (WORLD_COLS * cnt1) + cnt2;
      if (sGridNo >= 0 && sGridNo < WORLD_MAX && sGridNo >= leftmost &&
          sGridNo < (leftmost + WORLD_COLS) &&
          gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE) {
        // Go on sweet stop
        if (NewOKDestination(pSoldier, sGridNo, TRUE, pSoldier->bLevel)) {
          uiRange = GetRangeInCellCoordsFromGridNoDiff(sSweetGridNo, sGridNo);

          {
            if (uiRange < uiLowestRange) {
              sLowestGridNo = sGridNo;
              uiLowestRange = uiRange;
            }
          }
        }
      }
    }
  }
  gubNPCAPBudget = ubSaveNPCAPBudget;
  gubNPCDistLimit = ubSaveNPCDistLimit;
  return sLowestGridNo;
}

// Kris:  modified to actually path from sweetspot to gridno.  Previously, it
// only checked if the destination was sittable (though it was possible that that
// location would be trapped.
uint16_t FindGridNoFromSweetSpotWithStructData(SOLDIERTYPE *pSoldier, uint16_t usAnimState,
                                               int16_t sSweetGridNo, int8_t ubRadius,
                                               uint8_t *pubDirection, BOOLEAN fClosestToMerc) {
  int16_t sTop, sBottom;
  int16_t sLeft, sRight;
  int16_t cnt1, cnt2, cnt3;
  int16_t sGridNo;
  int32_t uiRange, uiLowestRange = 999999;
  int16_t sLowestGridNo = -1;
  int32_t leftmost;
  BOOLEAN fFound = FALSE;
  SOLDIERTYPE soldier;
  uint8_t ubSaveNPCAPBudget;
  uint8_t ubSaveNPCDistLimit;
  uint8_t ubBestDirection = 0;

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
  soldier.sGridNo = sSweetGridNo;

  sTop = ubRadius;
  sBottom = -ubRadius;
  sLeft = -ubRadius;
  sRight = ubRadius;

  // If we are already at this gridno....
  if (pSoldier->sGridNo == sSweetGridNo && !(pSoldier->uiStatusFlags & SOLDIER_VEHICLE)) {
    *pubDirection = pSoldier->bDirection;
    return (sSweetGridNo);
  }

  // clear the mapelements of potential residue MAPELEMENT_REACHABLE flags
  // in the square region.
  // ATE: CHECK FOR BOUNDARIES!!!!!!
  for (cnt1 = sBottom; cnt1 <= sTop; cnt1++) {
    leftmost = ((sSweetGridNo + (WORLD_COLS * cnt1)) / WORLD_COLS) * WORLD_COLS;

    for (cnt2 = sLeft; cnt2 <= sRight; cnt2++) {
      sGridNo = sSweetGridNo + (WORLD_COLS * cnt1) + cnt2;
      if (sGridNo >= 0 && sGridNo < WORLD_MAX && sGridNo >= leftmost &&
          sGridNo < (leftmost + WORLD_COLS)) {
        gpWorldLevelData[sGridNo].uiFlags &= (~MAPELEMENT_REACHABLE);
      }
    }
  }

  // Now, find out which of these gridnos are reachable
  //(use the fake soldier and the pathing settings)
  FindBestPath(&soldier, NOWHERE, 0, WALKING, COPYREACHABLE,
               (PATH_IGNORE_PERSON_AT_DEST | PATH_THROUGH_PEOPLE));

  uiLowestRange = 999999;

  for (cnt1 = sBottom; cnt1 <= sTop; cnt1++) {
    leftmost = ((sSweetGridNo + (WORLD_COLS * cnt1)) / WORLD_COLS) * WORLD_COLS;

    for (cnt2 = sLeft; cnt2 <= sRight; cnt2++) {
      sGridNo = sSweetGridNo + (WORLD_COLS * cnt1) + cnt2;
      if (sGridNo >= 0 && sGridNo < WORLD_MAX && sGridNo >= leftmost &&
          sGridNo < (leftmost + WORLD_COLS) &&
          gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE) {
        // Go on sweet stop
        if (NewOKDestination(pSoldier, sGridNo, TRUE, pSoldier->bLevel)) {
          BOOLEAN fDirectionFound = FALSE;
          uint16_t usOKToAddStructID;
          uint16_t usAnimSurface;

          if (pSoldier->pLevelNode != NULL) {
            if (pSoldier->pLevelNode->pStructureData != NULL) {
              usOKToAddStructID = pSoldier->pLevelNode->pStructureData->usStructureID;
            } else {
              usOKToAddStructID = INVALID_STRUCTURE_ID;
            }
          } else {
            usOKToAddStructID = INVALID_STRUCTURE_ID;
          }

          // Get animation surface...
          usAnimSurface = DetermineSoldierAnimationSurface(pSoldier, usAnimState);
          // Get structure ref...
          const STRUCTURE_FILE_REF *const pStructureFileRef =
              GetAnimationStructureRef(pSoldier, usAnimSurface, usAnimState);
          Assert(pStructureFileRef);

          // Check each struct in each direction
          for (cnt3 = 0; cnt3 < 8; cnt3++) {
            if (OkayToAddStructureToWorld(sGridNo, pSoldier->bLevel,
                                          &pStructureFileRef->pDBStructureRef[OneCDirection(cnt3)],
                                          usOKToAddStructID)) {
              fDirectionFound = TRUE;
              break;
            }
          }

          if (fDirectionFound) {
            if (fClosestToMerc) {
              uiRange = FindBestPath(pSoldier, sGridNo, pSoldier->bLevel,
                                     pSoldier->usUIMovementMode, NO_COPYROUTE, 0);

              if (uiRange == 0) {
                uiRange = 999;
              }
            } else {
              uiRange = GetRangeInCellCoordsFromGridNoDiff(sSweetGridNo, sGridNo);
            }

            if (uiRange < uiLowestRange) {
              ubBestDirection = (uint8_t)cnt3;
              sLowestGridNo = sGridNo;
              uiLowestRange = uiRange;
              fFound = TRUE;
            }
          }
        }
      }
    }
  }
  gubNPCAPBudget = ubSaveNPCAPBudget;
  gubNPCDistLimit = ubSaveNPCDistLimit;
  if (fFound) {
    // Set direction we chose...
    *pubDirection = ubBestDirection;

    return (sLowestGridNo);
  } else {
    return (NOWHERE);
  }
}

static uint16_t FindGridNoFromSweetSpotWithStructDataUsingGivenDirectionFirst(
    SOLDIERTYPE *pSoldier, uint16_t usAnimState, int16_t sSweetGridNo, int8_t ubRadius,
    uint8_t *pubDirection, BOOLEAN fClosestToMerc, int8_t bGivenDirection) {
  int16_t sTop, sBottom;
  int16_t sLeft, sRight;
  int16_t cnt1, cnt2, cnt3;
  int16_t sGridNo;
  int32_t uiRange, uiLowestRange = 999999;
  int16_t sLowestGridNo = -1;
  int32_t leftmost;
  BOOLEAN fFound = FALSE;
  SOLDIERTYPE soldier;
  uint8_t ubSaveNPCAPBudget;
  uint8_t ubSaveNPCDistLimit;
  uint8_t ubBestDirection = 0;

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
  soldier.sGridNo = sSweetGridNo;

  sTop = ubRadius;
  sBottom = -ubRadius;
  sLeft = -ubRadius;
  sRight = ubRadius;

  // If we are already at this gridno....
  if (pSoldier->sGridNo == sSweetGridNo && !(pSoldier->uiStatusFlags & SOLDIER_VEHICLE)) {
    *pubDirection = pSoldier->bDirection;
    return (sSweetGridNo);
  }

  // clear the mapelements of potential residue MAPELEMENT_REACHABLE flags
  // in the square region.
  // ATE: CHECK FOR BOUNDARIES!!!!!!
  for (cnt1 = sBottom; cnt1 <= sTop; cnt1++) {
    leftmost = ((sSweetGridNo + (WORLD_COLS * cnt1)) / WORLD_COLS) * WORLD_COLS;

    for (cnt2 = sLeft; cnt2 <= sRight; cnt2++) {
      sGridNo = sSweetGridNo + (WORLD_COLS * cnt1) + cnt2;
      if (sGridNo >= 0 && sGridNo < WORLD_MAX && sGridNo >= leftmost &&
          sGridNo < (leftmost + WORLD_COLS)) {
        gpWorldLevelData[sGridNo].uiFlags &= (~MAPELEMENT_REACHABLE);
      }
    }
  }

  // Now, find out which of these gridnos are reachable
  //(use the fake soldier and the pathing settings)
  FindBestPath(&soldier, NOWHERE, 0, WALKING, COPYREACHABLE,
               (PATH_IGNORE_PERSON_AT_DEST | PATH_THROUGH_PEOPLE));

  uiLowestRange = 999999;

  for (cnt1 = sBottom; cnt1 <= sTop; cnt1++) {
    leftmost = ((sSweetGridNo + (WORLD_COLS * cnt1)) / WORLD_COLS) * WORLD_COLS;

    for (cnt2 = sLeft; cnt2 <= sRight; cnt2++) {
      sGridNo = sSweetGridNo + (WORLD_COLS * cnt1) + cnt2;
      if (sGridNo >= 0 && sGridNo < WORLD_MAX && sGridNo >= leftmost &&
          sGridNo < (leftmost + WORLD_COLS) &&
          gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE) {
        // Go on sweet stop
        if (NewOKDestination(pSoldier, sGridNo, TRUE, pSoldier->bLevel)) {
          BOOLEAN fDirectionFound = FALSE;
          uint16_t usOKToAddStructID;
          uint16_t usAnimSurface;

          if (pSoldier->pLevelNode != NULL) {
            if (pSoldier->pLevelNode->pStructureData != NULL) {
              usOKToAddStructID = pSoldier->pLevelNode->pStructureData->usStructureID;
            } else {
              usOKToAddStructID = INVALID_STRUCTURE_ID;
            }
          } else {
            usOKToAddStructID = INVALID_STRUCTURE_ID;
          }

          // Get animation surface...
          usAnimSurface = DetermineSoldierAnimationSurface(pSoldier, usAnimState);
          // Get structure ref...
          const STRUCTURE_FILE_REF *const pStructureFileRef =
              GetAnimationStructureRef(pSoldier, usAnimSurface, usAnimState);
          Assert(pStructureFileRef);

          // OK, check the perfered given direction first
          if (OkayToAddStructureToWorld(
                  sGridNo, pSoldier->bLevel,
                  &pStructureFileRef->pDBStructureRef[OneCDirection(bGivenDirection)],
                  usOKToAddStructID)) {
            fDirectionFound = TRUE;
            cnt3 = bGivenDirection;
          } else {
            // Check each struct in each direction
            for (cnt3 = 0; cnt3 < 8; cnt3++) {
              if (cnt3 != bGivenDirection) {
                if (OkayToAddStructureToWorld(
                        sGridNo, pSoldier->bLevel,
                        &pStructureFileRef->pDBStructureRef[OneCDirection(cnt3)],
                        usOKToAddStructID)) {
                  fDirectionFound = TRUE;
                  break;
                }
              }
            }
          }

          if (fDirectionFound) {
            if (fClosestToMerc) {
              uiRange = FindBestPath(pSoldier, sGridNo, pSoldier->bLevel,
                                     pSoldier->usUIMovementMode, NO_COPYROUTE, 0);

              if (uiRange == 0) {
                uiRange = 999;
              }
            } else {
              uiRange = GetRangeInCellCoordsFromGridNoDiff(sSweetGridNo, sGridNo);
            }

            if (uiRange < uiLowestRange) {
              ubBestDirection = (uint8_t)cnt3;
              sLowestGridNo = sGridNo;
              uiLowestRange = uiRange;
              fFound = TRUE;
            }
          }
        }
      }
    }
  }
  gubNPCAPBudget = ubSaveNPCAPBudget;
  gubNPCDistLimit = ubSaveNPCDistLimit;
  if (fFound) {
    // Set direction we chose...
    *pubDirection = ubBestDirection;

    return (sLowestGridNo);
  } else {
    return (NOWHERE);
  }
}

uint16_t FindGridNoFromSweetSpotWithStructDataFromSoldier(const SOLDIERTYPE *const pSoldier,
                                                          const uint16_t usAnimState,
                                                          const int8_t ubRadius,
                                                          const BOOLEAN fClosestToMerc,
                                                          const SOLDIERTYPE *const pSrcSoldier) {
  int16_t sTop, sBottom;
  int16_t sLeft, sRight;
  int16_t cnt1, cnt2, cnt3;
  int16_t sGridNo;
  int32_t uiRange, uiLowestRange = 999999;
  int32_t leftmost;
  uint8_t ubSaveNPCAPBudget;
  uint8_t ubSaveNPCDistLimit;
  int16_t sSweetGridNo;
  SOLDIERTYPE soldier;

  sSweetGridNo = pSrcSoldier->sGridNo;

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
  soldier.sGridNo = sSweetGridNo;

  sTop = ubRadius;
  sBottom = -ubRadius;
  sLeft = -ubRadius;
  sRight = ubRadius;

  // clear the mapelements of potential residue MAPELEMENT_REACHABLE flags
  // in the square region.
  // ATE: CHECK FOR BOUNDARIES!!!!!!
  for (cnt1 = sBottom; cnt1 <= sTop; cnt1++) {
    leftmost = ((sSweetGridNo + (WORLD_COLS * cnt1)) / WORLD_COLS) * WORLD_COLS;

    for (cnt2 = sLeft; cnt2 <= sRight; cnt2++) {
      sGridNo = sSweetGridNo + (WORLD_COLS * cnt1) + cnt2;
      if (sGridNo >= 0 && sGridNo < WORLD_MAX && sGridNo >= leftmost &&
          sGridNo < (leftmost + WORLD_COLS)) {
        gpWorldLevelData[sGridNo].uiFlags &= (~MAPELEMENT_REACHABLE);
      }
    }
  }

  // Now, find out which of these gridnos are reachable
  FindBestPath(&soldier, NOWHERE, 0, WALKING, COPYREACHABLE,
               (PATH_IGNORE_PERSON_AT_DEST | PATH_THROUGH_PEOPLE));

  uiLowestRange = 999999;

  int16_t sLowestGridNo = NOWHERE;
  for (cnt1 = sBottom; cnt1 <= sTop; cnt1++) {
    leftmost = ((sSweetGridNo + (WORLD_COLS * cnt1)) / WORLD_COLS) * WORLD_COLS;

    for (cnt2 = sLeft; cnt2 <= sRight; cnt2++) {
      sGridNo = sSweetGridNo + (WORLD_COLS * cnt1) + cnt2;
      if (sGridNo >= 0 && sGridNo < WORLD_MAX && sGridNo >= leftmost &&
          sGridNo < (leftmost + WORLD_COLS) &&
          gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE) {
        // Go on sweet stop
        if (NewOKDestination(pSoldier, sGridNo, TRUE, pSoldier->bLevel)) {
          BOOLEAN fDirectionFound = FALSE;
          uint16_t usOKToAddStructID;
          uint16_t usAnimSurface;

          if (fClosestToMerc != 3) {
            if (pSoldier->pLevelNode != NULL && pSoldier->pLevelNode->pStructureData != NULL) {
              usOKToAddStructID = pSoldier->pLevelNode->pStructureData->usStructureID;
            } else {
              usOKToAddStructID = INVALID_STRUCTURE_ID;
            }

            // Get animation surface...
            usAnimSurface = DetermineSoldierAnimationSurface(pSoldier, usAnimState);
            // Get structure ref...
            const STRUCTURE_FILE_REF *const pStructureFileRef =
                GetAnimationStructureRef(pSoldier, usAnimSurface, usAnimState);

            // Check each struct in each direction
            for (cnt3 = 0; cnt3 < 8; cnt3++) {
              if (OkayToAddStructureToWorld(
                      sGridNo, pSoldier->bLevel,
                      &pStructureFileRef->pDBStructureRef[OneCDirection(cnt3)],
                      usOKToAddStructID)) {
                fDirectionFound = TRUE;
                break;
              }
            }
          } else {
            fDirectionFound = TRUE;
            cnt3 = (uint8_t)Random(8);
          }

          if (fDirectionFound) {
            if (fClosestToMerc == 1) {
              uiRange = GetRangeInCellCoordsFromGridNoDiff(pSoldier->sGridNo, sGridNo);
            } else if (fClosestToMerc == 2) {
              uiRange = GetRangeInCellCoordsFromGridNoDiff(pSoldier->sGridNo, sGridNo) +
                        GetRangeInCellCoordsFromGridNoDiff(sSweetGridNo, sGridNo);
            } else {
              // uiRange = GetRangeInCellCoordsFromGridNoDiff( sSweetGridNo,
              // sGridNo );
              uiRange = abs((sSweetGridNo / MAXCOL) - (sGridNo / MAXCOL)) +
                        abs((sSweetGridNo % MAXROW) - (sGridNo % MAXROW));
            }

            if (uiRange < uiLowestRange || (uiRange == uiLowestRange &&
                                            PythSpacesAway(pSoldier->sGridNo, sGridNo) <
                                                PythSpacesAway(pSoldier->sGridNo, sLowestGridNo))) {
              sLowestGridNo = sGridNo;
              uiLowestRange = uiRange;
            }
          }
        }
      }
    }
  }
  gubNPCAPBudget = ubSaveNPCAPBudget;
  gubNPCDistLimit = ubSaveNPCDistLimit;
  return sLowestGridNo;
}

uint16_t FindGridNoFromSweetSpotExcludingSweetSpot(const SOLDIERTYPE *const pSoldier,
                                                   const int16_t sSweetGridNo,
                                                   const int8_t ubRadius) {
  int16_t sTop, sBottom;
  int16_t sLeft, sRight;
  int16_t cnt1, cnt2;
  int16_t sGridNo;
  int32_t uiRange, uiLowestRange = 999999;
  int32_t leftmost;

  sTop = ubRadius;
  sBottom = -ubRadius;
  sLeft = -ubRadius;
  sRight = ubRadius;

  uiLowestRange = 999999;

  int16_t sLowestGridNo = NOWHERE;
  for (cnt1 = sBottom; cnt1 <= sTop; cnt1++) {
    leftmost = ((sSweetGridNo + (WORLD_COLS * cnt1)) / WORLD_COLS) * WORLD_COLS;

    for (cnt2 = sLeft; cnt2 <= sRight; cnt2++) {
      sGridNo = sSweetGridNo + (WORLD_COLS * cnt1) + cnt2;

      if (sSweetGridNo == sGridNo) {
        continue;
      }

      if (sGridNo >= 0 && sGridNo < WORLD_MAX && sGridNo >= leftmost &&
          sGridNo < (leftmost + WORLD_COLS)) {
        // Go on sweet stop
        if (NewOKDestination(pSoldier, sGridNo, TRUE, pSoldier->bLevel)) {
          uiRange = GetRangeInCellCoordsFromGridNoDiff(sSweetGridNo, sGridNo);

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

uint16_t FindGridNoFromSweetSpotExcludingSweetSpotInQuardent(const SOLDIERTYPE *const pSoldier,
                                                             const int16_t sSweetGridNo,
                                                             const int8_t ubRadius,
                                                             const int8_t ubQuardentDir) {
  int16_t sTop, sBottom;
  int16_t sLeft, sRight;
  int16_t cnt1, cnt2;
  int16_t sGridNo;
  int32_t uiRange, uiLowestRange = 999999;
  int32_t leftmost;

  sTop = ubRadius;
  sBottom = -ubRadius;
  sLeft = -ubRadius;
  sRight = ubRadius;

  // Switch on quadrent
  if (ubQuardentDir == SOUTHEAST) {
    sBottom = 0;
    sLeft = 0;
  }

  uiLowestRange = 999999;

  int16_t sLowestGridNo = NOWHERE;
  for (cnt1 = sBottom; cnt1 <= sTop; cnt1++) {
    leftmost = ((sSweetGridNo + (WORLD_COLS * cnt1)) / WORLD_COLS) * WORLD_COLS;

    for (cnt2 = sLeft; cnt2 <= sRight; cnt2++) {
      sGridNo = sSweetGridNo + (WORLD_COLS * cnt1) + cnt2;

      if (sSweetGridNo == sGridNo) {
        continue;
      }

      if (sGridNo >= 0 && sGridNo < WORLD_MAX && sGridNo >= leftmost &&
          sGridNo < (leftmost + WORLD_COLS)) {
        // Go on sweet stop
        if (NewOKDestination(pSoldier, sGridNo, TRUE, pSoldier->bLevel)) {
          uiRange = GetRangeInCellCoordsFromGridNoDiff(sSweetGridNo, sGridNo);

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

BOOLEAN CanSoldierReachGridNoInGivenTileLimit(SOLDIERTYPE *pSoldier, int16_t sGridNo,
                                              int16_t sMaxTiles, int8_t bLevel) {
  int32_t iNumTiles;
  int16_t sActionGridNo;

  if (pSoldier->bLevel != bLevel) {
    return (FALSE);
  }

  sActionGridNo = FindAdjacentGridEx(pSoldier, sGridNo, NULL, NULL, FALSE, FALSE);

  if (sActionGridNo == -1) {
    sActionGridNo = sGridNo;
  }

  if (sActionGridNo == pSoldier->sGridNo) {
    return (TRUE);
  }

  iNumTiles = FindBestPath(pSoldier, sActionGridNo, pSoldier->bLevel, WALKING, NO_COPYROUTE,
                           PATH_IGNORE_PERSON_AT_DEST);

  if (iNumTiles <= sMaxTiles && iNumTiles != 0) {
    return (TRUE);
  } else {
    return (FALSE);
  }
}

uint16_t FindRandomGridNoFromSweetSpot(const SOLDIERTYPE *const pSoldier,
                                       const int16_t sSweetGridNo, const int8_t ubRadius) {
  int16_t sX, sY;
  int16_t sGridNo;
  int32_t leftmost;
  BOOLEAN fFound = FALSE;
  uint32_t cnt = 0;
  SOLDIERTYPE soldier;
  uint8_t ubSaveNPCAPBudget;
  uint8_t ubSaveNPCDistLimit;
  int16_t sTop, sBottom;
  int16_t sLeft, sRight;
  int16_t cnt1, cnt2;

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
  soldier.sGridNo = sSweetGridNo;

  sTop = ubRadius;
  sBottom = -ubRadius;
  sLeft = -ubRadius;
  sRight = ubRadius;

  // ATE: CHECK FOR BOUNDARIES!!!!!!
  for (cnt1 = sBottom; cnt1 <= sTop; cnt1++) {
    leftmost = ((sSweetGridNo + (WORLD_COLS * cnt1)) / WORLD_COLS) * WORLD_COLS;

    for (cnt2 = sLeft; cnt2 <= sRight; cnt2++) {
      sGridNo = sSweetGridNo + (WORLD_COLS * cnt1) + cnt2;
      if (sGridNo >= 0 && sGridNo < WORLD_MAX && sGridNo >= leftmost &&
          sGridNo < (leftmost + WORLD_COLS)) {
        gpWorldLevelData[sGridNo].uiFlags &= (~MAPELEMENT_REACHABLE);
      }
    }
  }

  // Now, find out which of these gridnos are reachable
  //(use the fake soldier and the pathing settings)
  FindBestPath(&soldier, NOWHERE, 0, WALKING, COPYREACHABLE,
               (PATH_IGNORE_PERSON_AT_DEST | PATH_THROUGH_PEOPLE));

  do {
    sX = (uint16_t)Random(ubRadius);
    sY = (uint16_t)Random(ubRadius);

    leftmost = ((sSweetGridNo + (WORLD_COLS * sY)) / WORLD_COLS) * WORLD_COLS;

    sGridNo = sSweetGridNo + (WORLD_COLS * sY) + sX;

    if (sGridNo >= 0 && sGridNo < WORLD_MAX && sGridNo >= leftmost &&
        sGridNo < (leftmost + WORLD_COLS) &&
        gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE) {
      // Go on sweet stop
      if (NewOKDestination(pSoldier, sGridNo, TRUE, pSoldier->bLevel)) {
        // If we are a crow, we need this additional check
        if (pSoldier->ubBodyType == CROW) {
          if (GetRoom(sGridNo) == NO_ROOM) {
            fFound = TRUE;
          }
        } else {
          fFound = TRUE;
        }
      }
    }

    cnt++;

    if (cnt > 2000) {
      return (NOWHERE);
    }

  } while (!fFound);

  gubNPCAPBudget = ubSaveNPCAPBudget;
  gubNPCDistLimit = ubSaveNPCDistLimit;

  return (sGridNo);
}

static void AddSoldierToSectorGridNo(SOLDIERTYPE *pSoldier, int16_t sGridNo, uint8_t ubDirection,
                                     BOOLEAN fUseAnimation, uint16_t usAnimState,
                                     uint16_t usAnimCode);

static void InternalAddSoldierToSector(SOLDIERTYPE *const s, BOOLEAN calculate_direction,
                                       BOOLEAN const use_animation, uint16_t const anim_state,
                                       uint16_t const anim_code) {
  if (!s->bActive) return;

  // ATE: Make sure life of elliot is OK if from a meanwhile
  if (AreInMeanwhile() && s->ubProfile == ELLIOT && s->bLife < OKLIFE) {
    s->bLife = 25;
  }

  // ADD SOLDIER TO SLOT!
  if (s->uiStatusFlags & SOLDIER_OFF_MAP) {
    AddAwaySlot(s);
    // Guy is NOT "in sector"
    s->bInSector = FALSE;
  } else {
    AddMercSlot(s);
    // Add guy to sector flag
    s->bInSector = TRUE;
  }

  // If a driver or passenger - stop here!
  if (s->uiStatusFlags & SOLDIER_DRIVER) return;
  if (s->uiStatusFlags & SOLDIER_PASSENGER) return;

  CheckForAndAddMercToTeamPanel(s);

  s->usQuoteSaidFlags &= ~SOLDIER_QUOTE_SAID_SPOTTING_CREATURE_ATTACK;
  s->usQuoteSaidFlags &= ~SOLDIER_QUOTE_SAID_SMELLED_CREATURE;
  s->usQuoteSaidFlags &= ~SOLDIER_QUOTE_SAID_WORRIED_ABOUT_CREATURES;

  int16_t gridno;
  uint8_t direction;
  uint8_t calculated_direction;
  if (s->bTeam == CREATURE_TEAM) {
    gridno = FindGridNoFromSweetSpotWithStructData(s, STANDING, s->sInsertionGridNo, 7,
                                                   &calculated_direction, FALSE);
    direction = calculate_direction ? calculated_direction : s->ubInsertionDirection;
  } else {
    if (s->sInsertionGridNo == NOWHERE) {  // Add the soldier to the respective entrypoint.  This is
                                           // an error condition.
    }

    if (s->uiStatusFlags & SOLDIER_VEHICLE) {
      gridno = FindGridNoFromSweetSpotWithStructDataUsingGivenDirectionFirst(
          s, STANDING, s->sInsertionGridNo, 12, &calculated_direction, FALSE,
          s->ubInsertionDirection);
      // ATE: Override insertion direction
      s->ubInsertionDirection = calculated_direction;
    } else {
      gridno = FindGridNoFromSweetSpot(s, s->sInsertionGridNo, 7);
      if (gridno == NOWHERE) {  // ATE: Error condition - if nowhere use insertion gridno!
        // FIXME: calculate_direction is left uninitialized
        gridno = s->sInsertionGridNo;
      } else {
        calculated_direction = GetDirectionToGridNoFromGridNo(gridno, CENTER_GRIDNO);
      }
    }

    // Override calculated direction if we were told to....
    if (s->ubInsertionDirection > 100) {
      s->ubInsertionDirection -= 100;
      calculate_direction = FALSE;
    }

    if (calculate_direction) {
      direction = calculated_direction;

      // Check if we need to get direction from exit grid
      if (s->bUseExitGridForReentryDirection) {
        s->bUseExitGridForReentryDirection = FALSE;

        // OK, we know there must be an exit gridno SOMEWHERE close
        int16_t const sExitGridNo = FindClosestExitGrid(s, gridno, 10);
        if (sExitGridNo != NOWHERE) {
          // We found one, calculate direction
          direction = (uint8_t)GetDirectionToGridNoFromGridNo(sExitGridNo, gridno);
        }
      }
    } else {
      direction = s->ubInsertionDirection;
    }
  }

  if (gTacticalStatus.uiFlags & LOADING_SAVED_GAME) direction = s->bDirection;
  AddSoldierToSectorGridNo(s, gridno, direction, use_animation, anim_state, anim_code);

  CheckForPotentialAddToBattleIncrement(s);
}

void AddSoldierToSector(SOLDIERTYPE *const s) { InternalAddSoldierToSector(s, TRUE, FALSE, 0, 0); }

void AddSoldierToSectorNoCalculateDirection(SOLDIERTYPE *const s) {
  InternalAddSoldierToSector(s, FALSE, FALSE, 0, 0);
}

void AddSoldierToSectorNoCalculateDirectionUseAnimation(SOLDIERTYPE *const s,
                                                        uint16_t const usAnimState,
                                                        uint16_t const usAnimCode) {
  InternalAddSoldierToSector(s, FALSE, TRUE, usAnimState, usAnimCode);
}

static void PlaceSoldierNearSweetSpot(SOLDIERTYPE *const s, const uint16_t anim,
                                      const GridNo sweet_spot) {
  // OK, look for suitable placement....
  uint8_t new_direction;
  const GridNo good_pos =
      FindGridNoFromSweetSpotWithStructData(s, anim, sweet_spot, 5, &new_direction, FALSE);
  EVENT_SetSoldierPosition(s, good_pos, SSP_NONE);
  EVENT_SetSoldierDirection(s, new_direction);
  EVENT_SetSoldierDesiredDirection(s, new_direction);
}

static void InternalSoldierInSectorSleep(SOLDIERTYPE *const s, int16_t const gridno) {
  if (!s->bInSector) return;
  uint16_t const anim = AM_AN_EPC(s) ? STANDING : SLEEPING;
  PlaceSoldierNearSweetSpot(s, anim, gridno);
  EVENT_InitNewSoldierAnim(s, anim, 1, TRUE);
}

static void SoldierInSectorIncompaciated(SOLDIERTYPE *const s, int16_t const gridno) {
  if (!s->bInSector) return;
  PlaceSoldierNearSweetSpot(s, STAND_FALLFORWARD_STOP, gridno);
  EVENT_InitNewSoldierAnim(s, STAND_FALLFORWARD_STOP, 1, TRUE);
}

static void SoldierInSectorAnim(SOLDIERTYPE *const s, int16_t const gridno, uint16_t anim_state) {
  if (!s->bInSector) return;
  PlaceSoldierNearSweetSpot(s, anim_state, gridno);
  if (!IS_MERC_BODY_TYPE(s)) anim_state = STANDING;
  EVENT_InitNewSoldierAnim(s, anim_state, 1, TRUE);
}

void SoldierInSectorPatient(SOLDIERTYPE *const s, int16_t const gridno) {
  SoldierInSectorAnim(s, gridno, BEING_PATIENT);
}

void SoldierInSectorDoctor(SOLDIERTYPE *const s, int16_t const gridno) {
  SoldierInSectorAnim(s, gridno, BEING_DOCTOR);
}

void SoldierInSectorRepair(SOLDIERTYPE *const s, int16_t const gridno) {
  SoldierInSectorAnim(s, gridno, BEING_REPAIRMAN);
}

static void AddSoldierToSectorGridNo(SOLDIERTYPE *const s, int16_t const sGridNo,
                                     uint8_t const ubDirection, BOOLEAN const fUseAnimation,
                                     uint16_t const usAnimState, uint16_t const usAnimCode) {
  // Add merc to gridno

  // Set reserved location!
  s->sReservedMovementGridNo = NOWHERE;

  // Save OLD insertion code.. as this can change...
  uint8_t const insertion_code = s->ubStrategicInsertionCode;

  // Remove any pending animations
  s->usPendingAnimation = NO_PENDING_ANIMATION;
  s->ubPendingDirection = NO_PENDING_DIRECTION;
  s->ubPendingAction = NO_PENDING_ACTION;

  // If we are not loading a saved game
  SetSoldierPosFlags set_pos_flags = SSP_NONE;
  if (gTacticalStatus.uiFlags & LOADING_SAVED_GAME) {
    // Set final dest to be the same...
    set_pos_flags = SSP_NO_DEST | SSP_NO_FINAL_DEST;
  }

  // If this is a special insertion location, get path!
  if (insertion_code == INSERTION_CODE_ARRIVING_GAME) {
    EVENT_SetSoldierPosition(s, sGridNo, set_pos_flags);
    EVENT_SetSoldierDirection(s, ubDirection);
    EVENT_SetSoldierDesiredDirection(s, ubDirection);
  } else if (insertion_code != INSERTION_CODE_CHOPPER) {
    EVENT_SetSoldierPosition(s, sGridNo, set_pos_flags);

    // if we are loading, dont set the direction (they are already set)
    if (!(gTacticalStatus.uiFlags & LOADING_SAVED_GAME)) {
      EVENT_SetSoldierDirection(s, ubDirection);
      EVENT_SetSoldierDesiredDirection(s, ubDirection);
    }
  }

  if (gTacticalStatus.uiFlags & LOADING_SAVED_GAME) return;

  if (!(s->uiStatusFlags & SOLDIER_DEAD) && s->bTeam == OUR_TEAM) {
    RevealRoofsAndItems(s, FALSE);

    // ATE: Patch fix: If we are in an non-interruptable animation, stop!
    if (s->usAnimState == HOPFENCE) {
      s->fInNonintAnim = FALSE;
      SoldierGotoStationaryStance(s);
    }

    EVENT_StopMerc(s, sGridNo, ubDirection);
  }

  // If just arriving, set a destination to walk into from!
  if (insertion_code == INSERTION_CODE_ARRIVING_GAME) {
    // Find a sweetspot near...
    int16_t const new_gridno = FindGridNoFromSweetSpot(s, gMapInformation.sNorthGridNo, 4);
    EVENT_GetNewSoldierPath(s, new_gridno, WALKING);
  }

  // If he's an enemy... set presence
  // ATE: Added if not bloodcats, only do this once they are seen
  if (!s->bNeutral && s->bSide != OUR_TEAM && s->ubBodyType != BLOODCAT) {
    SetEnemyPresence();
  }

  if (s->uiStatusFlags & SOLDIER_DEAD) return;

  // ATE: Double check if we are on the roof that there is a roof there!
  if (s->bLevel == 1 && !FindStructure(s->sGridNo, STRUCTURE_ROOF)) {
    SetSoldierHeight(s, 0.0);
  }

  if (insertion_code == INSERTION_CODE_ARRIVING_GAME) return;

  // default to standing on arrival
  if (s->usAnimState != HELIDROP) {
    if (fUseAnimation) {
      EVENT_InitNewSoldierAnim(s, usAnimState, usAnimCode, TRUE);
    } else if (s->ubBodyType != CROW) {
      EVENT_InitNewSoldierAnim(s, STANDING, 1, TRUE);
    }
  }

  // ATE: if we are below OK life, make them lie down!
  if (s->bLife < OKLIFE) {
    SoldierInSectorIncompaciated(s, s->sInsertionGridNo);
  } else if (s->fMercAsleep) {
    InternalSoldierInSectorSleep(s, s->sInsertionGridNo);
  } else
    switch (s->bAssignment) {
      case PATIENT:
        SoldierInSectorPatient(s, s->sInsertionGridNo);
        break;
      case DOCTOR:
        SoldierInSectorDoctor(s, s->sInsertionGridNo);
        break;
      case REPAIR:
        SoldierInSectorRepair(s, s->sInsertionGridNo);
        break;
    }

  // ATE: Make sure movement mode is up to date!
  s->usUIMovementMode = GetMoveStateBasedOnStance(s, gAnimControl[s->usAnimState].ubEndHeight);
}

// IsMercOnTeam() checks to see if the passed in Merc Profile ID is currently on
// the player's team
BOOLEAN IsMercOnTeam(uint8_t ubMercID) {
  const SOLDIERTYPE *const s = FindSoldierByProfileIDOnPlayerTeam(ubMercID);
  return s != NULL;
}

BOOLEAN IsMercOnTeamAndInOmertaAlreadyAndAlive(uint8_t ubMercID) {
  const SOLDIERTYPE *const s = FindSoldierByProfileIDOnPlayerTeam(ubMercID);
  return s != NULL && s->bAssignment != IN_TRANSIT && s->bLife > 0;
}
