// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Macro.h"
/*
        Filename        :       pathai.c
        Author          :       Ray E. Bornert II
        Date            :       1992-MAR-15

        Skip list additions
        Author          :       Chris Camfield
        Date            :       1997-NOV
*/

#include <algorithm>
#include <string.h>

#include "GameSettings.h"
#include "SGP/English.h"
#include "SGP/Input.h"
#include "SGP/MemMan.h"
#include "SGP/Random.h"
#include "Tactical/AnimationCache.h"
#include "Tactical/AnimationControl.h"
#include "Tactical/AnimationData.h"
#include "Tactical/Interface.h"
#include "Tactical/Keys.h"
#include "Tactical/Overhead.h"
#include "Tactical/OverheadTypes.h"
#include "Tactical/PathAI.h"
#include "Tactical/PathAIDebug.h"
#include "Tactical/Points.h"
#include "Tactical/SoldierControl.h"
#include "Tactical/StructureWrap.h"
#include "TacticalAI/AI.h"
#include "TileEngine/Buildings.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/Structure.h"
#include "TileEngine/TileDef.h"
#include "TileEngine/WorldDef.h"
#include "TileEngine/WorldMan.h"
#include "Utils/FontControl.h"
#include "Utils/Message.h"

// skiplist has extra level of pointers every 4 elements, so a level 5is
// optimized for 4 to the power of 5 elements, or 2 to the power of 10, 1024

// #define PATHAI_VISIBLE_DEBUG

// #define PATHAI_SKIPLIST_DEBUG

#ifdef PATHAI_VISIBLE_DEBUG
#include "SGP/Video.h"

extern int16_t gsCoverValue[WORLD_MAX];
BOOLEAN gfDisplayCoverValues = TRUE;
static BOOLEAN gfDrawPathPoints = FALSE;
#endif

BOOLEAN gfPlotPathToExitGrid = FALSE;
BOOLEAN gfRecalculatingExistingPathCost = FALSE;
uint8_t gubGlobalPathFlags = 0;

uint8_t gubBuildingInfoToSet;

// ABSOLUTE maximums
#define ABSMAX_SKIPLIST_LEVEL 5
#define ABSMAX_TRAIL_TREE (16384)
#define ABSMAX_PATHQ (512)

// STANDARD maximums... configurable!
#define MAX_SKIPLIST_LEVEL 5
#define MAX_TRAIL_TREE (4096)
#define MAX_PATHQ (512)

int32_t iMaxSkipListLevel = MAX_SKIPLIST_LEVEL;
int32_t iMaxTrailTree = MAX_TRAIL_TREE;
int32_t iMaxPathQ = MAX_PATHQ;

extern BOOLEAN gfGeneratingMapEdgepoints;

#define VEHICLE

#define TRAILCELLTYPE uint16_t

// OLD PATHAI STUFF
/////////////////////////////////////////////////
struct path_t {
  int32_t iLocation;                     // 4
  path_t *pNext[ABSMAX_SKIPLIST_LEVEL];  // 4 * MAX_SKIPLIST_LEVEL (5) = 20
  int16_t sPathNdx;                      // 2
  TRAILCELLTYPE usCostSoFar;             // 2
  TRAILCELLTYPE usCostToGo;              // 2
  TRAILCELLTYPE usTotalCost;             // 2
  int8_t bLevel;                         // 1
  uint8_t ubTotalAPCost;                 // 1
  uint8_t ubLegDistance;                 // 1
};

struct trail_t {
  int16_t nextLink;
  int8_t stepDir;
  int8_t fFlags;
  int16_t sGridNo;
};

enum TrailFlags { STEP_BACKWARDS = 0x01 };

#define EASYWATERCOST TRAVELCOST_FLAT / 2
#define ISWATER(t) (((t) == TRAVELCOST_KNEEDEEP) || ((t) == TRAVELCOST_DEEPWATER))
#define NOPASS (TRAVELCOST_BLOCKED)

static path_t *pathQ;
static uint16_t gusPathShown, gusAPtsToMove;
static int32_t queRequests;
static int32_t iSkipListSize;
static int32_t iClosedListSize;
static int8_t bSkipListLevel;
static int32_t iSkipListLevelLimit[8] = {0, 4, 16, 64, 256, 1024, 4192, 16384};

#define ESTIMATE0 ((dx > dy) ? (dx) : (dy))
#define ESTIMATE1 ((dx < dy) ? ((dx * 14) / 10 + dy) : ((dy * 14) / 10 + dx))
#define ESTIMATE2 FLATCOST *((dx < dy) ? ((dx * 14) / 10 + dy) : ((dy * 14) / 10 + dx))
#define ESTIMATEn ((int)(FLATCOST * sqrt(dx * dx + dy * dy)))
#define ESTIMATEC                                             \
  (((dx < dy) ? (TRAVELCOST_BUMPY * (dx * 14 + dy * 10) / 10) \
              : (TRAVELCOST_BUMPY * (dy * 14 + dx * 10) / 10)))
// #define ESTIMATEC (((dx<dy) ? ( (TRAVELCOST_FLAT * dx * 14) / 10 + dy) :
//(TRAVELCOST_FLAT * dy * 14 ) / 10 + dx) ) )
#define ESTIMATE ESTIMATEC

#define MAXCOST (9990)
// #define MAXCOST (255)
// #define TOTALCOST( pCurrPtr ) (pCurrPtr->usCostSoFar + pCurrPtr->usCostToGo)
#define TOTALCOST(ptr) (ptr->usTotalCost)
#define XLOC(a) (a % MAPWIDTH)
#define YLOC(a) (a / MAPWIDTH)
// #define LEGDISTANCE(a,b) ( abs( XLOC(b)-XLOC(a) ) + abs( YLOC(b)-YLOC(a) ) )
#define LEGDISTANCE(x1, y1, x2, y2) (abs(x2 - x1) + abs(y2 - y1))
// #define FARTHER(ndx,NDX) ( LEGDISTANCE( ndx->sLocation,sDestination) >
//  LEGDISTANCE(NDX->sLocation,sDestination) )
#define FARTHER(ndx, NDX) (ndx->ubLegDistance > NDX->ubLegDistance)

#define SETLOC(str, loc) \
  { (str).iLocation = loc; }

static TRAILCELLTYPE *trailCost;
static uint8_t *trailCostUsed;
static uint8_t gubGlobalPathCount = 0;
static trail_t *trailTree;

static short trailTreeNdx = 0;

#define QHEADNDX (0)
#define QPOOLNDX (iMaxPathQ - 1)

static path_t *pQueueHead;
static path_t *pClosedHead;

#define pathQNotEmpty (pQueueHead->pNext[0] != NULL)
#define pathFound (pQueueHead->pNext[0]->iLocation == iDestination)
#define pathNotYetFound (!pathFound)

// Note, the closed list is maintained as a doubly-linked list;
// it's a regular queue, essentially, as we always add to the end
// and remove from the front

// pNext[0] is used for the next pointers
// and pNext[1] is used for prev pointers

/*
#define ClosedListAdd( pNew ) \
{\
        pNew->pNext[0] = pClosedHead;\
        pNew->pNext[1] = pClosedHead->pNext[1];\
        pClosedHead->pNext[1]->pNext[0] = pNew;\
        pClosedHead->pNext[1] = pNew;\
        pNew->iLocation = -1;\
        iClosedListSize++;\
}

#define ClosedListGet( pNew )\
{\
        if (queRequests<QPOOLNDX)\
        {\
                pNew = pathQ + (queRequests);\
                queRequests++;\
                memset( pNew->pNext, 0, sizeof( path_t *) *
ABSMAX_SKIPLIST_LEVEL );\
                pNew->bLevel = RandomSkipListLevel();\
        }\
        else if (iClosedListSize > 0)\
        {\
                pNew = pClosedHead->pNext[0];\
                pNew->pNext[1]->pNext[0] = pNew->pNext[0];\
                pNew->pNext[0]->pNext[1] = pNew->pNext[1];\
                iClosedListSize--;\
                queRequests++;\
                memset( pNew->pNext, 0, sizeof( path_t *) *
ABSMAX_SKIPLIST_LEVEL );\
                pNew->bLevel = RandomSkipListLevel();\
        }\
        else\
        {\
                pNew = NULL;\
        }\
}
*/

// experiment 1, seemed to fail
#define ClosedListAdd(pNew)                 \
  {                                         \
    pNew->pNext[0] = pClosedHead->pNext[0]; \
    pClosedHead->pNext[0] = pNew;           \
    pNew->iLocation = -1;                   \
    iClosedListSize++;                      \
  }

#define ClosedListGet(pNew)                                             \
  {                                                                     \
    if (queRequests < QPOOLNDX) {                                       \
      pNew = pathQ + (queRequests);                                     \
      queRequests++;                                                    \
      pNew->bLevel = RandomSkipListLevel();                             \
    } else if (iClosedListSize > 0) {                                   \
      pNew = pClosedHead->pNext[0];                                     \
      pClosedHead->pNext[0] = pNew->pNext[0];                           \
      iClosedListSize--;                                                \
      queRequests++;                                                    \
      memset(pNew->pNext, 0, sizeof(path_t *) * ABSMAX_SKIPLIST_LEVEL); \
      pNew->bLevel = RandomSkipListLevel();                             \
    } else {                                                            \
      pNew = NULL;                                                      \
    }                                                                   \
  }

/*
#define ClosedListAdd( pNew ) \
{\
        pNew->pNext[0] = pClosedHead;\
        pNew->pNext[1] = pClosedHead->pNext[1];\
        pClosedHead->pNext[1]->pNext[0] = pNew;\
        pClosedHead->pNext[1] = pNew;\
        pNew->iLocation = -1;\
        iClosedListSize++;\
}

#define ClosedListGet( pNew )\
{\
        if (queRequests<QPOOLNDX)\
        {\
                pNew = pathQ + (queRequests);\
                queRequests++;\
                memset( pNew->pNext, 0, sizeof( path_t *) *
ABSMAX_SKIPLIST_LEVEL );\
                pNew->bLevel = RandomSkipListLevel();\
        }\
        else if (iClosedListSize > 0)\
        {\
                pNew = pClosedHead->pNext[0];\
                pNew->pNext[1]->pNext[0] = pNew->pNext[0];\
                pNew->pNext[0]->pNext[1] = pNew->pNext[1];\
                iClosedListSize--;\
                queRequests++;\
                memset( pNew->pNext, 0, sizeof( path_t *) *
ABSMAX_SKIPLIST_LEVEL );\
                pNew->bLevel = RandomSkipListLevel();\
        }\
        else\
        {\
                pNew = NULL;\
        }\
}
*/

#define SkipListRemoveHead()                                                   \
  {                                                                            \
    pDel = pQueueHead->pNext[0];                                               \
    for (iLoop = 0; iLoop < std::min(bSkipListLevel, pDel->bLevel); iLoop++) { \
      pQueueHead->pNext[iLoop] = pDel->pNext[iLoop];                           \
    }                                                                          \
    iSkipListSize--;                                                           \
    ClosedListAdd(pDel);                                                       \
  }

#define SkipListInsert(pNew)                                                                     \
  {                                                                                              \
    pCurr = pQueueHead;                                                                          \
    uiCost = TOTALCOST(pNew);                                                                    \
    memset(pUpdate, 0, MAX_SKIPLIST_LEVEL * sizeof(path_t *));                                   \
    for (iCurrLevel = bSkipListLevel - 1; iCurrLevel >= 0; iCurrLevel--) {                       \
      pNext = pCurr->pNext[iCurrLevel];                                                          \
      while (pNext) {                                                                            \
        if (uiCost > TOTALCOST(pNext) || (uiCost == TOTALCOST(pNext) && FARTHER(pNew, pNext))) { \
          pCurr = pNext;                                                                         \
          pNext = pCurr->pNext[iCurrLevel];                                                      \
        } else {                                                                                 \
          break;                                                                                 \
        }                                                                                        \
      }                                                                                          \
      pUpdate[iCurrLevel] = pCurr;                                                               \
    }                                                                                            \
    pCurr = pCurr->pNext[0];                                                                     \
    for (iCurrLevel = 0; iCurrLevel < pNew->bLevel; iCurrLevel++) {                              \
      if (!(pUpdate[iCurrLevel])) {                                                              \
        break;                                                                                   \
      }                                                                                          \
      pNew->pNext[iCurrLevel] = pUpdate[iCurrLevel]->pNext[iCurrLevel];                          \
      pUpdate[iCurrLevel]->pNext[iCurrLevel] = pNew;                                             \
    }                                                                                            \
    iSkipListSize++;                                                                             \
    if (iSkipListSize > iSkipListLevelLimit[bSkipListLevel]) {                                   \
      pCurr = pQueueHead;                                                                        \
      pNext = pQueueHead->pNext[bSkipListLevel - 1];                                             \
      while (pNext) {                                                                            \
        if (pNext->bLevel > bSkipListLevel) {                                                    \
          pCurr->pNext[bSkipListLevel] = pNext;                                                  \
          pCurr = pNext;                                                                         \
        }                                                                                        \
        pNext = pNext->pNext[bSkipListLevel - 1];                                                \
      }                                                                                          \
      pCurr->pNext[bSkipListLevel] = pNext;                                                      \
      bSkipListLevel++;                                                                          \
    }                                                                                            \
  }

#define REMQUEHEADNODE() SkipListRemoveHead();

#define DELQUENODE(ndx) SkipListRemoveHead()

#define REMAININGCOST(ptr) ((dy = abs(iDestY - iLocY)), (dx = abs(iDestX - iLocX)), ESTIMATE)
/*
#define REMAININGCOST(ptr)					\
(								\
        (locY = (ptr)->iLocation/MAPWIDTH),			\
        (locX = (ptr)->iLocation%MAPWIDTH),			\
        (dy = abs(iDestY-locY)),					\
        (dx = abs(iDestX-locX)),					\
        ESTIMATE						\
)
*/

#define NEWQUENODE ClosedListGet(pNewPtr)

#define QUEINSERT(ndx) SkipListInsert(ndx)

#define GREENSTEPSTART 0
#define REDSTEPSTART 16
#define PURPLESTEPSTART 32
#define BLUESTEPSTART 48
#define ORANGESTEPSTART 64

uint8_t gubNPCAPBudget = 0;
uint8_t gubNPCDistLimit = 0;
BOOLEAN gfNPCCircularDistLimit = FALSE;
uint8_t gubNPCPathCount;

BOOLEAN gfPlotDirectPath = FALSE;
BOOLEAN gfEstimatePath = FALSE;
BOOLEAN gfPathAroundObstacles = TRUE;

static uint32_t guiPlottedPath[256];
uint32_t guiPathingData[256];
static int32_t giPathDataSize;
static int32_t giPlotCnt;

static int32_t dirDelta[8] = {
    -MAPWIDTH,     // N
    1 - MAPWIDTH,  // NE
    1,             // E
    1 + MAPWIDTH,  // SE
    MAPWIDTH,      // S
    MAPWIDTH - 1,  // SW
    -1,            // W
    -MAPWIDTH - 1  // NW
};

#define LOOPING_CLOCKWISE 0
#define LOOPING_COUNTERCLOCKWISE 1
#define LOOPING_REVERSE 2

#ifdef COUNT_PATHS
uint32_t guiSuccessfulPathChecks = 0;
uint32_t guiTotalPathChecks = 0;
uint32_t guiFailedPathChecks = 0;
uint32_t guiUnsuccessfulPathChecks = 0;
#endif

static int8_t RandomSkipListLevel() {
  int8_t bLevel = 1;

  while (Random(4) == 0 && bLevel < iMaxSkipListLevel - 1) {
    bLevel++;
  }
  return (bLevel);
}

void InitPathAI() {
  pathQ = MALLOCN(path_t, ABSMAX_PATHQ);
  trailCost = MALLOCN(TRAILCELLTYPE, MAPLENGTH);
  trailCostUsed = MALLOCNZ(uint8_t, MAPLENGTH);
  trailTree = MALLOCN(trail_t, ABSMAX_TRAIL_TREE);
  pQueueHead = &(pathQ[QHEADNDX]);
  pClosedHead = &(pathQ[QPOOLNDX]);
}

void ShutDownPathAI() {
  MemFree(pathQ);
  MemFree(trailCostUsed);
  MemFree(trailCost);
  MemFree(trailTree);
}

static void ReconfigurePathAI(int32_t iNewMaxSkipListLevel, int32_t iNewMaxTrailTree,
                              int32_t iNewMaxPathQ) {
  // make sure the specified parameters are reasonable
  iNewMaxSkipListLevel = std::max(iNewMaxSkipListLevel, ABSMAX_SKIPLIST_LEVEL);
  iNewMaxTrailTree = std::max(iNewMaxTrailTree, ABSMAX_TRAIL_TREE);
  iNewMaxPathQ = std::max(iNewMaxPathQ, ABSMAX_PATHQ);
  // assign them
  iMaxSkipListLevel = iNewMaxSkipListLevel;
  iMaxTrailTree = iNewMaxTrailTree;
  iMaxPathQ = iNewMaxPathQ;
  // relocate the head of the closed list to the end of the array portion being
  // used
  pClosedHead = &(pathQ[QPOOLNDX]);
  memset(pClosedHead, 0, sizeof(path_t));
}

static void RestorePathAIToDefaults() {
  iMaxSkipListLevel = MAX_SKIPLIST_LEVEL;
  iMaxTrailTree = MAX_TRAIL_TREE;
  iMaxPathQ = MAX_PATHQ;
  // relocate the head of the closed list to the end of the array portion being
  // used
  pClosedHead = &(pathQ[QPOOLNDX]);
  memset(pClosedHead, 0, sizeof(path_t));
}

///////////////////////////////////////////////////////////////////////
//	FINDBESTPATH                                                   /
////////////////////////////////////////////////////////////////////////
int32_t FindBestPath(SOLDIERTYPE *s, int16_t sDestination, int8_t ubLevel, int16_t usMovementMode,
                     int8_t bCopy, uint8_t fFlags) {
  int32_t iDestination = sDestination, iOrigination;
  int32_t iCnt = -1, iStructIndex;
  int32_t iLoopStart = 0, iLoopEnd = 0;
  int8_t bLoopState = LOOPING_CLOCKWISE;
  // BOOLEAN fLoopForwards = FALSE;
  BOOLEAN fCheckedBehind = FALSE;
  int32_t iDestX, iDestY, iLocX, iLocY, dx, dy;
  int32_t newLoc, curLoc;
  // int32_t curY;
  int32_t curCost, newTotCost, nextCost;
  int16_t sCurPathNdx;
  int32_t prevCost;
  int32_t iWaterToWater;
  uint8_t ubCurAPCost, ubAPCost;
  uint8_t ubNewAPCost = 0;
#ifdef VEHICLE
  // BOOLEAN fTurnSlow = FALSE;
  // BOOLEAN fReverse = FALSE; // stuff for vehicles turning
  BOOLEAN fMultiTile, fVehicle;
  // int32_t iLastDir, iPrevToLastDir;
  // int8_t bVehicleCheckDir;
  // uint16_t adjLoc;
  STRUCTURE_FILE_REF *pStructureFileRef = NULL;
  uint16_t usAnimSurface;
  // int32_t iCnt2, iCnt3;
#endif

  int32_t iLastDir = 0;

  path_t *pNewPtr;
  path_t *pCurrPtr;

  path_t *pUpdate[ABSMAX_SKIPLIST_LEVEL];
  path_t *pCurr;
  path_t *pNext;
  path_t *pDel;
  uint32_t uiCost;
  int32_t iCurrLevel, iLoop;

  uint16_t usOKToAddStructID = 0;

  BOOLEAN fCopyReachable;
  BOOLEAN fCopyPathCosts;
  BOOLEAN fVisitSpotsOnlyOnce;
  int32_t iOriginationX, iOriginationY, iX, iY;

  BOOLEAN fTurnBased;
  BOOLEAN fPathingForPlayer;
  int32_t iDoorGridNo = -1;
  BOOLEAN fDoorIsObstacleIfClosed = 0;  // if false, door is obstacle if it is open
  DOOR_STATUS *pDoorStatus;
  DOOR *pDoor;
  STRUCTURE *pDoorStructure;
  BOOLEAN fDoorIsOpen = FALSE;
  BOOLEAN fNonFenceJumper;
  BOOLEAN fNonSwimmer;
  BOOLEAN fPathAroundPeople;
  BOOLEAN fGoingThroughDoor = FALSE;  // for one tile
  BOOLEAN fContinuousTurnNeeded;
  BOOLEAN fCloseGoodEnough;
  uint16_t usMovementModeToUseForAPs;
  int16_t sClosePathLimit = -1;  // XXX HACK000E

#ifdef PATHAI_SKIPLIST_DEBUG
  char zTempString[1000], zTS[50];
#endif

#ifdef PATHAI_VISIBLE_DEBUG
  uint16_t usCounter = 0;
#endif

  fVehicle = FALSE;
  iOriginationX = iOriginationY = 0;
  iOrigination = (int32_t)s->sGridNo;

  if (iOrigination < 0 || iOrigination > WORLD_MAX) {
    return (0);
  } else if (!GridNoOnVisibleWorldTile((int16_t)iOrigination)) {
    return (0);
  } else if (s->bLevel != ubLevel) {
    // pathing to a different level... bzzzt!
    return (0);
  }

  if (gubGlobalPathFlags) {
    fFlags |= gubGlobalPathFlags;
  }

  fTurnBased = ((gTacticalStatus.uiFlags & TURNBASED) && (gTacticalStatus.uiFlags & INCOMBAT));

  fPathingForPlayer = ((s->bTeam == OUR_TEAM) && (!gTacticalStatus.fAutoBandageMode) &&
                       !(s->uiStatusFlags & SOLDIER_PCUNDERAICONTROL));
  fNonFenceJumper = !(IS_MERC_BODY_TYPE(s));
  fNonSwimmer = !(IS_MERC_BODY_TYPE(s));
  if (fNonSwimmer) {
    if (Water(sDestination)) {
      return (0);
    }
  }
  fPathAroundPeople = ((fFlags & PATH_THROUGH_PEOPLE) == 0);
  fCloseGoodEnough = ((fFlags & PATH_CLOSE_GOOD_ENOUGH) != 0);
  if (fCloseGoodEnough) {
    sClosePathLimit =
        std::min(PythSpacesAway((int16_t)s->sGridNo, sDestination) - 1, PATH_CLOSE_RADIUS);
    if (sClosePathLimit <= 0) {
      return (0);
    }
  }

  BOOLEAN const fConsiderPersonAtDestAsObstacle =
      fPathingForPlayer && fPathAroundPeople && !(fFlags & PATH_IGNORE_PERSON_AT_DEST);

  if (bCopy >= COPYREACHABLE) {
    fCopyReachable = TRUE;
    fCopyPathCosts = (bCopy == COPYREACHABLE_AND_APS);
    fVisitSpotsOnlyOnce = (bCopy == COPYREACHABLE);
    // make sure we aren't trying to copy path costs for an area greater than
    // the AI array...
    if (fCopyPathCosts && gubNPCDistLimit > AI_PATHCOST_RADIUS) {
      // oy!!!! dis no supposed to happen!
      gubNPCDistLimit = AI_PATHCOST_RADIUS;
    }
  } else {
    fCopyReachable = FALSE;
    fCopyPathCosts = FALSE;
    fVisitSpotsOnlyOnce = FALSE;
  }

  gubNPCPathCount++;

  if (gubGlobalPathCount == 255) {
    // reset arrays!
    memset(trailCostUsed, 0, MAPLENGTH);
    gubGlobalPathCount = 1;
  } else {
    gubGlobalPathCount++;
  }

  // only allow nowhere destination if distance limit set
  if (sDestination == NOWHERE) {
    /*
    if (gubNPCDistLimit == 0)
    {
            return( FALSE );
    }
    */
  } else {
    // the very first thing to do is make sure the destination tile is reachable
    if (!NewOKDestination(s, sDestination, fConsiderPersonAtDestAsObstacle, ubLevel)) {
      gubNPCAPBudget = 0;
      gubNPCDistLimit = 0;
      return (FALSE);
    }

    if (sDestination == s->sGridNo) {
      return (FALSE);
    }
  }

  if (gubNPCAPBudget) {
    ubAPCost = MinAPsToStartMovement(s, usMovementMode);
    if (ubAPCost > gubNPCAPBudget) {
      gubNPCAPBudget = 0;
      gubNPCDistLimit = 0;
      return (0);
    } else {
      gubNPCAPBudget -= ubAPCost;
    }
  }

#ifdef COUNT_PATHS
  guiTotalPathChecks++;
#endif

#ifdef VEHICLE

  fMultiTile = ((s->uiStatusFlags & SOLDIER_MULTITILE) != 0);
  if (fMultiTile) {
    // Get animation surface...
    // Chris_C... change this to use parameter.....
    usAnimSurface = DetermineSoldierAnimationSurface(s, usMovementMode);
    // Get structure ref...
    pStructureFileRef = GetAnimationStructureRef(s, usAnimSurface, usMovementMode);

    if (pStructureFileRef) {
      fVehicle = ((s->uiStatusFlags & SOLDIER_VEHICLE) != 0);

      fContinuousTurnNeeded =
          ((s->uiStatusFlags & (SOLDIER_MONSTER | SOLDIER_ANIMAL | SOLDIER_VEHICLE)) != 0);

      /*
      if (fVehicle && s->bReverse)
      {
              fReverse = TRUE;
      }
      */
      /*
      if (fVehicle || s->ubBodyType == COW || s->ubBodyType == BLOODCAT) // or a
      vehicle
      {
              fTurnSlow = TRUE;
      }
      */
      if (gfEstimatePath) {
        usOKToAddStructID = IGNORE_PEOPLE_STRUCTURE_ID;
      } else if (s->pLevelNode != NULL && s->pLevelNode->pStructureData != NULL) {
        usOKToAddStructID = s->pLevelNode->pStructureData->usStructureID;
      } else {
        usOKToAddStructID = INVALID_STRUCTURE_ID;
      }

    } else {
      // turn off multitile pathing
      fMultiTile = FALSE;
      fContinuousTurnNeeded = FALSE;
    }

  } else {
    fContinuousTurnNeeded = FALSE;
  }

#endif

  if (!fContinuousTurnNeeded) {
    iLoopStart = 0;
    iLoopEnd = 0;
    bLoopState = LOOPING_CLOCKWISE;
  }

  ubCurAPCost = 0;
  queRequests = 2;

  // initialize the path data structures
  memset(pathQ, 0, iMaxPathQ * sizeof(path_t));
  memset(trailTree, 0, iMaxTrailTree * sizeof(trail_t));

#if defined(PATHAI_VISIBLE_DEBUG)
  if (gfDisplayCoverValues && gfDrawPathPoints) {
    memset(gsCoverValue, 0x7F, sizeof(int16_t) * WORLD_MAX);
  }
#endif

  bSkipListLevel = 1;
  iSkipListSize = 0;
  iClosedListSize = 0;

  trailTreeNdx = 0;

  // set up common info
  if (fCopyPathCosts) {
    iOriginationY = (iOrigination / MAPWIDTH);
    iOriginationX = (iOrigination % MAPWIDTH);
  }

  iDestY = (iDestination / MAPWIDTH);
  iDestX = (iDestination % MAPWIDTH);

  // if origin and dest is water, then user wants to stay in water!
  // so, check and set waterToWater flag accordingly
  if (iDestination == NOWHERE) {
    iWaterToWater = 0;
  } else {
    if (ISWATER(gubWorldMovementCosts[iOrigination][0][ubLevel]) &&
        ISWATER(gubWorldMovementCosts[iDestination][0][ubLevel]))
      iWaterToWater = 1;
    else
      iWaterToWater = 0;
  }

  // setup Q and first path record

  SETLOC(*pQueueHead, iOrigination);
  pQueueHead->usCostSoFar = MAXCOST;
  pQueueHead->bLevel = iMaxSkipListLevel - 1;

  pClosedHead->pNext[0] = pClosedHead;
  pClosedHead->pNext[1] = pClosedHead;

  // setup first path record
  iLocY = iOrigination / MAPWIDTH;
  iLocX = iOrigination % MAPWIDTH;

  SETLOC(pathQ[1], iOrigination);
  pathQ[1].sPathNdx = 0;
  pathQ[1].usCostSoFar = 0;
  if (fCopyReachable) {
    pathQ[1].usCostToGo = 100;
  } else {
    pathQ[1].usCostToGo = REMAININGCOST(&(pathQ[1]));
  }
  pathQ[1].usTotalCost = pathQ[1].usCostSoFar + pathQ[1].usCostToGo;
  pathQ[1].ubLegDistance = LEGDISTANCE(iLocX, iLocY, iDestX, iDestY);
  pathQ[1].bLevel = 1;
  pQueueHead->pNext[0] = &(pathQ[1]);
  iSkipListSize++;

  trailTreeNdx = 0;
  trailCost[iOrigination] = 0;
  pCurrPtr = pQueueHead->pNext[0];
  pCurrPtr->sPathNdx = trailTreeNdx;
  trailTreeNdx++;

  do {
    // remove the first and best path so far from the que
    pCurrPtr = pQueueHead->pNext[0];
    curLoc = pCurrPtr->iLocation;
    curCost = pCurrPtr->usCostSoFar;
    sCurPathNdx = pCurrPtr->sPathNdx;

    // remember the cost used to get here...
    prevCost = gubWorldMovementCosts[trailTree[sCurPathNdx].sGridNo][trailTree[sCurPathNdx].stepDir]
                                    [ubLevel];

#if defined(PATHAI_VISIBLE_DEBUG)
    if (gfDisplayCoverValues && gfDrawPathPoints) {
      if (gsCoverValue[curLoc] > 0) {
        gsCoverValue[curLoc] *= -1;
      }
    }
#endif

#ifdef VEHICLE
    /*
    if (fTurnSlow)
    {
            if (pCurrPtr->pNext[0] == 0)
            {
                    if (fReverse)
                    {
                            iLastDir = OppositeDirection(s->bDirection);
                    }
                    else
                    {
                            iLastDir = s->bDirection;
                    }
                    // start prev-to-last dir at same as current (could cause a
    problem) iPrevToLastDir = iLastDir;
            }
            else
            {
                    iPrevToLastDir =
    trailTree[trailTree[pCurrPtr->sPathNdx].nextLink].dirDelta; iLastDir =
    trailTree[pCurrPtr->sPathNdx].dirDelta;
            }

    }
    */
#endif

    if (gubNPCAPBudget) {
      ubCurAPCost = pCurrPtr->ubTotalAPCost;
    }
    if (fCopyReachable && prevCost != TRAVELCOST_FENCE) {
      gpWorldLevelData[curLoc].uiFlags |= MAPELEMENT_REACHABLE;
      if (gubBuildingInfoToSet > 0) {
        gubBuildingInfo[curLoc] = gubBuildingInfoToSet;
      }
    }

    DELQUENODE(pCurrPtr);

    if (trailCostUsed[curLoc] == gubGlobalPathCount && trailCost[curLoc] < curCost) goto NEXTDIR;

    // DebugMsg( TOPIC_JA2, DBG_LEVEL_3, String( "PATHAI %d", curLoc ) );

    if (fContinuousTurnNeeded) {
      if (trailTreeNdx < 2) {
        iLastDir = s->bDirection;
      } else if (trailTree[pCurrPtr->sPathNdx].fFlags & STEP_BACKWARDS) {
        iLastDir = OppositeDirection(trailTree[pCurrPtr->sPathNdx].stepDir);
      } else {
        iLastDir = trailTree[pCurrPtr->sPathNdx].stepDir;
      }
      iLoopStart = iLastDir;
      iLoopEnd = iLastDir;
      bLoopState = LOOPING_CLOCKWISE;
      fCheckedBehind = FALSE;
    }

    // contemplate a new path in each direction
    // for ( iCnt = iLoopStart; iCnt != iLoopEnd; iCnt = (iCnt + iLoopIncrement)
    // % MAXDIR )
    for (iCnt = iLoopStart;;) {
#ifdef VEHICLE
      /*
      if (fTurnSlow)
      {
              if (iLastDir == iPrevToLastDir)
              {
                      if (iCnt != iLastDir &&
                                      iCnt != OneCDirection(iLastDir) &&
                                      iCnt != OneCCDirection(iLastDir))
                      {
                              goto NEXTDIR;
                      }
              }
              else
              {
                      if ( iCnt != iLastDir )
                      {
                              goto NEXTDIR;
                      }
              }
      }
      */

      if (bLoopState == LOOPING_REVERSE) {
        iStructIndex = OppositeDirection(OneCDirection(iCnt));
      } else {
        iStructIndex = OneCDirection(iCnt);
      }

      if (fMultiTile) {
        if (fContinuousTurnNeeded) {
          if (iCnt != iLastDir) {
            if (!OkayToAddStructureToWorld((int16_t)curLoc, ubLevel,
                                           &(pStructureFileRef->pDBStructureRef[iStructIndex]),
                                           usOKToAddStructID)) {
              // we have to abort this loop and possibly reset the loop
              // conditions to search in the other direction (if we haven't
              // already done the other dir)
              if (bLoopState == LOOPING_CLOCKWISE) {
                iLoopStart = iLastDir;
                iLoopEnd = iCnt;
                bLoopState = LOOPING_COUNTERCLOCKWISE;  // backwards
                // when we go to the bottom of the loop, iLoopIncrement will be
                // added to iCnt which is good since it avoids duplication of
                // effort
                iCnt = iLoopStart;
                goto NEXTDIR;
              } else if (bLoopState == LOOPING_COUNTERCLOCKWISE && !fCheckedBehind) {
                // check rear dir
                bLoopState = LOOPING_REVERSE;

                // NB we're stuck with adding 1 to the loop counter down below
                // so configure to accomodate...
                // iLoopStart = (iLastDir + (MAXDIR / 2) - 1) % MAXDIR;
                iLoopStart = OppositeDirection(OneCCDirection(iLastDir));
                iLoopEnd = (iLoopStart + 2) % MAXDIR;
                iCnt = iLoopStart;
                fCheckedBehind = TRUE;
                goto NEXTDIR;
              } else {
                // done
                goto ENDOFLOOP;
              }
            }
          }
        } else if (pStructureFileRef) {
          // check to make sure it's okay for us to turn to the new direction in
          // our current tile
          if (!OkayToAddStructureToWorld((int16_t)curLoc, ubLevel,
                                         &(pStructureFileRef->pDBStructureRef[iStructIndex]),
                                         usOKToAddStructID)) {
            goto NEXTDIR;
          }
        }
      }

#endif

      newLoc = curLoc + dirDelta[iCnt];

      if (fVisitSpotsOnlyOnce && trailCostUsed[newLoc] == gubGlobalPathCount) {
        // on a "reachable" test, never revisit locations!
        goto NEXTDIR;
      }

      // if (gpWorldLevelData[newLoc].sHeight != ubLevel)
      // ATE: Movement onto cliffs? Check vs the soldier's gridno height
      // CJC: PREVIOUS LOCATION's height
      if (gpWorldLevelData[newLoc].sHeight != gpWorldLevelData[curLoc].sHeight) {
        goto NEXTDIR;
      }

      if (gubNPCDistLimit) {
        if (gfNPCCircularDistLimit) {
          if (PythSpacesAway((int16_t)iOrigination, (int16_t)newLoc) > gubNPCDistLimit) {
            goto NEXTDIR;
          }
        } else {
          if (SpacesAway((int16_t)iOrigination, (int16_t)newLoc) > gubNPCDistLimit) {
            goto NEXTDIR;
          }
        }
      }

      // AI check for mines
      if (gpWorldLevelData[newLoc].uiFlags & MAPELEMENT_ENEMY_MINE_PRESENT && s->bSide != 0) {
        goto NEXTDIR;
      }

      /*
      if ( gpWorldLevelData[newLoc].uiFlags & (MAPELEMENT_ENEMY_MINE_PRESENT |
      MAPELEMENT_PLAYER_MINE_PRESENT) )
      {
              if (s->bSide == 0)
              {
                      // For our team, skip a location with a known mines unless
      it is the end of our
                      // path; for others on our side, skip such locations
      completely; if (s->bTeam != OUR_TEAM || newLoc != iDestination)
                      {
                              if (gpWorldLevelData[newLoc].uiFlags &
      MAPELEMENT_PLAYER_MINE_PRESENT)
                              {
                                      goto NEXTDIR;
                              }
                      }
              }
              else
              {
                      // For the enemy, always skip known mines
                      if (gpWorldLevelData[newLoc].uiFlags &
      MAPELEMENT_ENEMY_MINE_PRESENT)
                      {
                              goto NEXTDIR;
                      }
              }
      }
      */

      // how much is admission to the next tile
      if (gfPathAroundObstacles) {
        nextCost = gubWorldMovementCosts[newLoc][iCnt][ubLevel];

        // ATE:	Check for differences from reality
        if (nextCost == TRAVELCOST_NOT_STANDING) {
          // for path plotting purposes, use the terrain value
          nextCost = gTileTypeMovementCost[gpWorldLevelData[newLoc].ubTerrainID];
        } else if (nextCost == TRAVELCOST_EXITGRID) {
          if (gfPlotPathToExitGrid) {
            // replace with terrain cost so that we can plot path, otherwise is
            // obstacle
            nextCost = gTileTypeMovementCost[gpWorldLevelData[newLoc].ubTerrainID];
          }
        } else if (nextCost == TRAVELCOST_FENCE && fNonFenceJumper) {
          goto NEXTDIR;
        } else if (IS_TRAVELCOST_DOOR(nextCost)) {
          // don't let anyone path diagonally through doors!
          if (iCnt & 1) {
            goto NEXTDIR;
          }

          switch (nextCost) {
            case TRAVELCOST_DOOR_CLOSED_HERE:
              fDoorIsObstacleIfClosed = TRUE;
              iDoorGridNo = newLoc;
              break;
            case TRAVELCOST_DOOR_CLOSED_N:
              fDoorIsObstacleIfClosed = TRUE;
              iDoorGridNo = newLoc + dirDelta[NORTH];
              break;
            case TRAVELCOST_DOOR_CLOSED_W:
              fDoorIsObstacleIfClosed = TRUE;
              iDoorGridNo = newLoc + dirDelta[WEST];
              break;
            case TRAVELCOST_DOOR_OPEN_HERE:
              fDoorIsObstacleIfClosed = FALSE;
              iDoorGridNo = newLoc;
              break;
            case TRAVELCOST_DOOR_OPEN_N:
              fDoorIsObstacleIfClosed = FALSE;
              iDoorGridNo = newLoc + dirDelta[NORTH];
              break;
            case TRAVELCOST_DOOR_OPEN_NE:
              fDoorIsObstacleIfClosed = FALSE;
              iDoorGridNo = newLoc + dirDelta[NORTHEAST];
              break;
            case TRAVELCOST_DOOR_OPEN_E:
              fDoorIsObstacleIfClosed = FALSE;
              iDoorGridNo = newLoc + dirDelta[EAST];
              break;
            case TRAVELCOST_DOOR_OPEN_SE:
              fDoorIsObstacleIfClosed = FALSE;
              iDoorGridNo = newLoc + dirDelta[SOUTHEAST];
              break;
            case TRAVELCOST_DOOR_OPEN_S:
              fDoorIsObstacleIfClosed = FALSE;
              iDoorGridNo = newLoc + dirDelta[SOUTH];
              break;
            case TRAVELCOST_DOOR_OPEN_SW:
              fDoorIsObstacleIfClosed = FALSE;
              iDoorGridNo = newLoc + dirDelta[SOUTHWEST];
              break;
            case TRAVELCOST_DOOR_OPEN_W:
              fDoorIsObstacleIfClosed = FALSE;
              iDoorGridNo = newLoc + dirDelta[WEST];
              break;
            case TRAVELCOST_DOOR_OPEN_NW:
              fDoorIsObstacleIfClosed = FALSE;
              iDoorGridNo = newLoc + dirDelta[NORTHWEST];
              break;
            case TRAVELCOST_DOOR_OPEN_N_N:
              fDoorIsObstacleIfClosed = FALSE;
              iDoorGridNo = newLoc + dirDelta[NORTH] + dirDelta[NORTH];
              break;
            case TRAVELCOST_DOOR_OPEN_NW_N:
              fDoorIsObstacleIfClosed = FALSE;
              iDoorGridNo = newLoc + dirDelta[NORTHWEST] + dirDelta[NORTH];
              break;
            case TRAVELCOST_DOOR_OPEN_NE_N:
              fDoorIsObstacleIfClosed = FALSE;
              iDoorGridNo = newLoc + dirDelta[NORTHEAST] + dirDelta[NORTH];
              break;
            case TRAVELCOST_DOOR_OPEN_W_W:
              fDoorIsObstacleIfClosed = FALSE;
              iDoorGridNo = newLoc + dirDelta[WEST] + dirDelta[WEST];
              break;
            case TRAVELCOST_DOOR_OPEN_SW_W:
              fDoorIsObstacleIfClosed = FALSE;
              iDoorGridNo = newLoc + dirDelta[SOUTHWEST] + dirDelta[WEST];
              break;
            case TRAVELCOST_DOOR_OPEN_NW_W:
              fDoorIsObstacleIfClosed = FALSE;
              iDoorGridNo = newLoc + dirDelta[NORTHWEST] + dirDelta[WEST];
              break;
            default:
              break;
          }

          if (fPathingForPlayer &&
              gpWorldLevelData[iDoorGridNo].ubExtFlags[0] & MAPELEMENT_EXT_DOOR_STATUS_PRESENT) {
            // check door status
            pDoorStatus = GetDoorStatus((int16_t)iDoorGridNo);
            if (pDoorStatus) {
              fDoorIsOpen = (pDoorStatus->ubFlags & DOOR_PERCEIVED_OPEN) != 0;
            } else {
              // door destroyed?
              nextCost = gTileTypeMovementCost[gpWorldLevelData[newLoc].ubTerrainID];
            }
          } else {
            // check door structure
            pDoorStructure = FindStructure((int16_t)iDoorGridNo, STRUCTURE_ANYDOOR);
            if (pDoorStructure) {
              fDoorIsOpen = (pDoorStructure->fFlags & STRUCTURE_OPEN) != 0;
            } else {
              // door destroyed?
              nextCost = gTileTypeMovementCost[gpWorldLevelData[newLoc].ubTerrainID];
            }
          }

          // now determine movement cost... if it hasn't been changed already
          if (IS_TRAVELCOST_DOOR(nextCost)) {
            if (fDoorIsOpen) {
              if (fDoorIsObstacleIfClosed) {
                nextCost = gTileTypeMovementCost[gpWorldLevelData[newLoc].ubTerrainID];
              } else {
                nextCost = TRAVELCOST_OBSTACLE;
              }
            } else {
              if (fDoorIsObstacleIfClosed) {
                // door is closed and this should be an obstacle, EXCEPT if we
                // are calculating a path for an enemy or NPC with keys
                if (fPathingForPlayer || (s && (s->uiStatusFlags & SOLDIER_MONSTER ||
                                                s->uiStatusFlags & SOLDIER_ANIMAL))) {
                  nextCost = TRAVELCOST_OBSTACLE;
                } else {
                  // have to check if door is locked and NPC does not have keys!
                  pDoor = FindDoorInfoAtGridNo(iDoorGridNo);
                  if (pDoor) {
                    if (!pDoor->fLocked || s->bHasKeys) {
                      // add to AP cost
                      if (gubNPCAPBudget) {
                        fGoingThroughDoor = TRUE;
                      }
                      nextCost = gTileTypeMovementCost[gpWorldLevelData[newLoc].ubTerrainID];
                    } else {
                      nextCost = TRAVELCOST_OBSTACLE;
                    }
                  } else {
                    nextCost = gTileTypeMovementCost[gpWorldLevelData[newLoc].ubTerrainID];
                  }
                }
              } else {
                nextCost = gTileTypeMovementCost[gpWorldLevelData[newLoc].ubTerrainID];
              }
            }
          }
        } else if ((nextCost == TRAVELCOST_SHORE || nextCost == TRAVELCOST_KNEEDEEP ||
                    nextCost == TRAVELCOST_DEEPWATER) &&
                   fNonSwimmer) {
          // creatures and animals can't go in water
          nextCost = TRAVELCOST_OBSTACLE;
        }

        // Apr. '96 - moved up be ahead of AP_Budget stuff
        if ((nextCost >= NOPASS))  // || ( nextCost == TRAVELCOST_DOOR ) )
          goto NEXTDIR;

      } else {
        nextCost = TRAVELCOST_FLAT;
      }

      if (newLoc > GRIDSIZE) {
        // WHAT THE??? hack.
        goto NEXTDIR;
      }

      // if contemplated tile is NOT final dest and someone there, disqualify
      // route when doing a reachable test, ignore all locations with people in
      // them
      if (fPathAroundPeople && ((newLoc != iDestination) || fCopyReachable)) {
        // ATE: ONLY cancel if they are moving.....
        const SOLDIERTYPE *const tgt = WhoIsThere2(newLoc, s->bLevel);
        if (tgt != NULL && tgt != s) {
          // Check for movement....
          // if (fTurnBased || tgt->sFinalDestination == tgt->sGridNo ||
          // tgt->fDelayedMovement)
          //{
          goto NEXTDIR;
          //}
          //	else
          //{
          //	nextCost += 50;
          //}
        }
      }

#ifdef VEHICLE
      if (fMultiTile) {
        // vehicle test for obstacles: prevent movement to next tile if
        // a tile covered by the vehicle in that position & direction
        // has an obstacle in it

        // because of the order in which animations are stored (dir 7 first,
        // then 0 1 2 3 4 5 6), we must subtract 1 from the direction
        // ATE: Send in our existing structure ID so it's ignored!

        if (!OkayToAddStructureToWorld((int16_t)newLoc, ubLevel,
                                       &(pStructureFileRef->pDBStructureRef[iStructIndex]),
                                       usOKToAddStructID)) {
          goto NEXTDIR;
        }

        /*
        // vehicles aren't moving any more....
        if (fVehicle)
        {
                // transmogrify pathing costs for vehicles!
                switch(nextCost)
                {
                        case TRAVELCOST_THICK		:	nextCost =
        TRAVELCOST_GRASS; break; case TRAVELCOST_SHORE		: case
        TRAVELCOST_KNEEDEEP: case TRAVELCOST_DEEPWATER:
                        //case TRAVELCOST_DOOR		:
                        case TRAVELCOST_FENCE		:	nextCost =
        TRAVELCOST_OBSTACLE; break;

                        default
        :	break;
                }
        }
        */
      }
#endif

      // NEW Apr 21 by Ian: abort if cost exceeds budget
      if (gubNPCAPBudget) {
        switch (nextCost) {
          case TRAVELCOST_NONE:
            ubAPCost = 0;
            break;

          case TRAVELCOST_DIRTROAD:
          case TRAVELCOST_FLAT:
            ubAPCost = AP_MOVEMENT_FLAT;
            break;
          // case TRAVELCOST_BUMPY	:
          case TRAVELCOST_GRASS:
            ubAPCost = AP_MOVEMENT_GRASS;
            break;
          case TRAVELCOST_THICK:
            ubAPCost = AP_MOVEMENT_BUSH;
            break;
          case TRAVELCOST_DEBRIS:
            ubAPCost = AP_MOVEMENT_RUBBLE;
            break;
          case TRAVELCOST_SHORE:
            ubAPCost = AP_MOVEMENT_SHORE;  // wading shallow water
            break;
          case TRAVELCOST_KNEEDEEP:
            ubAPCost = AP_MOVEMENT_LAKE;  // wading waist/chest deep - very slow
            break;

          case TRAVELCOST_DEEPWATER:
            ubAPCost = AP_MOVEMENT_OCEAN;  // can swim, so it's faster than wading
            break;

            // case TRAVELCOST_DOOR		:	ubAPCost =
            // AP_MOVEMENT_FLAT;
            // break;

          case TRAVELCOST_FENCE:
            ubAPCost = AP_JUMPFENCE;

            /*
                                    if ( sSwitchValue == TRAVELCOST_FENCE )
                                    {
                                            sPoints += sTileCost;

                                            // If we are changeing stance ( either
               before or after getting there....
                                            // We need to reflect that...
                                            switch(usMovementMode)
                                            {
                                                    case RUNNING:
                                                    case WALKING :

                                                            // Add here cost to go
               from crouch to stand AFTER fence hop....
                                                            // Since it's AFTER..
               make sure we will be moving after jump... if ( ( iCnt + 2 ) <
               iLastGrid )
                                                            {
                                                                    sPoints +=
               AP_CROUCH;
                                                            }
                                                            break;

                                                    case SWATTING:

                                                            // Add cost to stand
               once there BEFORE.... sPoints += AP_CROUCH; break;

                                                    case CRAWLING:

                                                            // Can't do it
               here..... break;

                                            }
                                    }

            */

            break;

          case TRAVELCOST_OBSTACLE:
          default:
            goto NEXTDIR;  // Cost too much to be considered!
        }

        // don't make the mistake of adding directly to
        // ubCurAPCost, that must be preserved for remaining dirs!
        if (iCnt & 1) {
          ubAPCost = (ubAPCost * 14) / 10;
        }

        usMovementModeToUseForAPs = usMovementMode;

        // ATE: if water, force to be walking always!
        if (nextCost == TRAVELCOST_SHORE || nextCost == TRAVELCOST_KNEEDEEP ||
            nextCost == TRAVELCOST_DEEPWATER) {
          usMovementModeToUseForAPs = WALKING;
        }

        // adjust AP cost for movement mode
        switch (usMovementModeToUseForAPs) {
          case RUNNING:
          case ADULTMONSTER_WALKING:
            // save on casting
            ubAPCost = ubAPCost * 10 / ((uint8_t)(RUNDIVISOR * 10));
            // ubAPCost = (int16_t)(double)( (sTileCost / RUNDIVISOR) );	break;
            break;
          case WALKING:
          case ROBOT_WALK:
            ubAPCost = (ubAPCost + WALKCOST);
            break;
          case SWATTING:
            ubAPCost = (ubAPCost + SWATCOST);
            break;
          case CRAWLING:
            ubAPCost = (ubAPCost + CRAWLCOST);
            break;
        }

        if (nextCost == TRAVELCOST_FENCE) {
          switch (usMovementModeToUseForAPs) {
            case RUNNING:
            case WALKING:
              // Here pessimistically assume the path will continue after hopping
              // the fence
              ubAPCost += AP_CROUCH;
              break;

            case SWATTING:

              // Add cost to stand once there BEFORE jumping....
              ubAPCost += AP_CROUCH;
              break;

            case CRAWLING:

              // Can't do it here.....
              goto NEXTDIR;
          }
        } else if (nextCost == TRAVELCOST_NOT_STANDING) {
          switch (usMovementModeToUseForAPs) {
            case RUNNING:
            case WALKING:
              // charge crouch APs for ducking head!
              ubAPCost += AP_CROUCH;
              break;

            default:
              break;
          }
        } else if (fGoingThroughDoor) {
          ubAPCost += AP_OPEN_DOOR;
          fGoingThroughDoor = FALSE;
        }

        ubNewAPCost = ubCurAPCost + ubAPCost;

        if (ubNewAPCost > gubNPCAPBudget) goto NEXTDIR;
      }

      if (fCloseGoodEnough) {
        if (PythSpacesAway((int16_t)newLoc, sDestination) <= sClosePathLimit) {
          // stop the path here!
          iDestination = newLoc;
          sDestination = (int16_t)newLoc;
          fCloseGoodEnough = FALSE;
        }
      }
      // make the destination look very attractive
      if (newLoc == iDestination)
        nextCost = 0;
      else if (gfPlotDirectPath && nextCost < NOPASS)
        nextCost = TRAVELCOST_FLAT;

      // make water cost attractive for water to water paths
      if (iWaterToWater) {
        if (ISWATER(prevCost)) prevCost = EASYWATERCOST;
        if (ISWATER(nextCost)) nextCost = EASYWATERCOST;
      }

      // NOTE: on September 24, 1997, Chris went back to a diagonal bias system
      if (iCnt & 1) {
        // moving on a diagonal
        nextCost = nextCost * 14 / 10;
      }

      if (bLoopState == LOOPING_REVERSE) {
        // penalize moving backwards to encourage turning sooner
        nextCost += 50;
      }

      newTotCost = curCost + nextCost;

      // have we found a path to the current location that
      // costs less than the best so far to the same location?
      if (trailCostUsed[newLoc] != gubGlobalPathCount || newTotCost < trailCost[newLoc]) {
#if defined(PATHAI_VISIBLE_DEBUG)

        if (gfDisplayCoverValues && gfDrawPathPoints) {
          if (gsCoverValue[newLoc] == 0x7F7F) {
            gsCoverValue[newLoc] = usCounter++;
          }
          /*
          else if (gsCoverValue[newLoc] >= 0)
          {
                  gsCoverValue[newLoc]++;
          }
          else
          {
                  gsCoverValue[newLoc]--;
          }
          */
        }
#endif

        // NEWQUENODE;
        {
          if (queRequests < QPOOLNDX) {
            pNewPtr = pathQ + (queRequests);
            queRequests++;
            memset(pNewPtr->pNext, 0, sizeof(path_t *) * ABSMAX_SKIPLIST_LEVEL);
            pNewPtr->bLevel = RandomSkipListLevel();
          } else if (iClosedListSize > 0) {
            pNewPtr = pClosedHead->pNext[0];
            pClosedHead->pNext[0] = pNewPtr->pNext[0];
            iClosedListSize--;
            queRequests++;
            memset(pNewPtr->pNext, 0, sizeof(path_t *) * ABSMAX_SKIPLIST_LEVEL);
            pNewPtr->bLevel = RandomSkipListLevel();
          } else {
            pNewPtr = NULL;
          }
        }

        if (pNewPtr == NULL) {
#ifdef COUNT_PATHS
          guiFailedPathChecks++;
#endif
          gubNPCAPBudget = 0;
          gubNPCDistLimit = 0;
          return (0);
        }

        // make new path to current location
        trailTree[trailTreeNdx].nextLink = sCurPathNdx;
        trailTree[trailTreeNdx].stepDir = (int8_t)iCnt;
        if (bLoopState == LOOPING_REVERSE) {
          trailTree[trailTreeNdx].fFlags = STEP_BACKWARDS;
        } else {
          trailTree[trailTreeNdx].fFlags = 0;
        }
        trailTree[trailTreeNdx].sGridNo = (int16_t)newLoc;
        pNewPtr->sPathNdx = trailTreeNdx;
        trailTreeNdx++;

        if (trailTreeNdx >= iMaxTrailTree) {
#ifdef COUNT_PATHS
          guiFailedPathChecks++;
#endif
          gubNPCAPBudget = 0;
          gubNPCDistLimit = 0;
          return (0);
        }

        iLocY = newLoc / MAPWIDTH;
        iLocX = newLoc % MAPWIDTH;
        SETLOC(*pNewPtr, newLoc);
        pNewPtr->usCostSoFar = (uint16_t)newTotCost;
        pNewPtr->usCostToGo = (uint16_t)REMAININGCOST(pNewPtr);
        if (fCopyReachable) {
          pNewPtr->usCostToGo = 100;
        } else {
          pNewPtr->usCostToGo = (uint16_t)REMAININGCOST(pNewPtr);
        }

        pNewPtr->usTotalCost = newTotCost + pNewPtr->usCostToGo;
        pNewPtr->ubLegDistance = LEGDISTANCE(iLocX, iLocY, iDestX, iDestY);

        if (gubNPCAPBudget) {
          // save the AP cost so far along this path
          pNewPtr->ubTotalAPCost = ubNewAPCost;
          // update the AP costs in the AI array of path costs if necessary...
          if (fCopyPathCosts) {
            iX = AI_PATHCOST_RADIUS + iLocX - iOriginationX;
            iY = AI_PATHCOST_RADIUS + iLocY - iOriginationY;
            gubAIPathCosts[iX][iY] = ubNewAPCost;
          }
        }

        // update the trail map to reflect the newer shorter path
        trailCost[newLoc] = (uint16_t)newTotCost;
        trailCostUsed[newLoc] = gubGlobalPathCount;

        // do a sorted que insert of the new path
        // COMMENTED OUT TO DO BOUNDS CHECKER CC JAN 18 99
        // QUEINSERT(pNewPtr);
        // #define SkipListInsert( pNewPtr )
        {
          pCurr = pQueueHead;
          uiCost = TOTALCOST(pNewPtr);
          memset(pUpdate, 0, MAX_SKIPLIST_LEVEL * sizeof(path_t *));
          for (iCurrLevel = bSkipListLevel - 1; iCurrLevel >= 0; iCurrLevel--) {
            pNext = pCurr->pNext[iCurrLevel];
            while (pNext) {
              if (uiCost > TOTALCOST(pNext) ||
                  (uiCost == TOTALCOST(pNext) && FARTHER(pNewPtr, pNext))) {
                pCurr = pNext;
                pNext = pCurr->pNext[iCurrLevel];
              } else {
                break;
              }
            }
            pUpdate[iCurrLevel] = pCurr;
          }
          pCurr = pCurr->pNext[0];
          for (iCurrLevel = 0; iCurrLevel < pNewPtr->bLevel; iCurrLevel++) {
            if (!(pUpdate[iCurrLevel])) {
              break;
            }
            pNewPtr->pNext[iCurrLevel] = pUpdate[iCurrLevel]->pNext[iCurrLevel];
            pUpdate[iCurrLevel]->pNext[iCurrLevel] = pNewPtr;
          }
          iSkipListSize++;
          if (iSkipListSize > iSkipListLevelLimit[bSkipListLevel]) {
            pCurr = pQueueHead;
            pNext = pQueueHead->pNext[bSkipListLevel - 1];
            while (pNext) {
              if (pNext->bLevel > bSkipListLevel) {
                pCurr->pNext[bSkipListLevel] = pNext;
                pCurr = pNext;
              }
              pNext = pNext->pNext[bSkipListLevel - 1];
            }
            pCurr->pNext[bSkipListLevel] = pNext;
            bSkipListLevel++;
          }
        }

#ifdef PATHAI_SKIPLIST_DEBUG
        // print out contents of queue
        {
          int32_t iLoop;
          int8_t bTemp;

          zTempString[0] = '\0';
          pCurr = pQueueHead;
          iLoop = 0;
          while (pCurr) {
            sprintf(zTS, "\t%ld", pCurr->sPathNdx);
            if (pCurr == pNewPtr) {
              strcat(zTS, "*");
            }
            strcat(zTempString, zTS);
            pCurr = pCurr->pNext[0];
            iLoop++;
            if (iLoop > 50) {
              break;
            }
          }
          DebugMsg(TOPIC_JA2, DBG_LEVEL_3, zTempString);

          zTempString[0] = '\0';
          pCurr = pQueueHead;
          iLoop = 0;
          while (pCurr) {
            sprintf(zTS, "\t%d", pCurr->bLevel);
            strcat(zTempString, zTS);
            pCurr = pCurr->pNext[0];
            iLoop++;
            if (iLoop > 50) {
              break;
            }
          }
          DebugMsg(TOPIC_JA2, DBG_LEVEL_3, zTempString);

          zTempString[0] = '\0';
          bTemp = pQueueHead->bLevel;
          pCurr = pQueueHead;
          iLoop = 0;
          while (pCurr) {
            bTemp = pQueueHead->bLevel;
            while (pCurr->pNext[bTemp - 1] == NULL) {
              bTemp--;
            }
            sprintf(zTS, "\t%d", bTemp);
            strcat(zTempString, zTS);
            pCurr = pCurr->pNext[0];
            iLoop++;
            if (iLoop > 50) {
              break;
            }
          }
          DebugMsg(TOPIC_JA2, DBG_LEVEL_3, zTempString);
          DebugMsg(TOPIC_JA2, DBG_LEVEL_3, "------");
        }
#endif
      }

    NEXTDIR:
      if (bLoopState == LOOPING_CLOCKWISE)  // backwards
      {
        iCnt = OneCCDirection(iCnt);
      } else {
        iCnt = OneCDirection(iCnt);
      }
      if (iCnt == iLoopEnd) {
      ENDOFLOOP:
        break;
      } else if (fContinuousTurnNeeded && iCnt == OppositeDirection(iLoopStart)) {
        fCheckedBehind = TRUE;
      }
    }
  } while (pathQNotEmpty && pathNotYetFound);

#if defined(PATHAI_VISIBLE_DEBUG)
  if (gfDisplayCoverValues && gfDrawPathPoints) {
    SetRenderFlags(RENDER_FLAG_FULL);
    if (guiCurrentScreen == GAME_SCREEN) {
      RenderWorld();
      RenderCoverDebug();
      InvalidateScreen();
      RefreshScreen();
    }
  }
#endif

  // work finished. Did we find a path?
  if (pathQNotEmpty && pathFound) {
    int16_t z, _z, _nextLink;  //,tempgrid;

    _z = 0;
    z = (int16_t)pQueueHead->pNext[0]->sPathNdx;

    while (z) {
      _nextLink = trailTree[z].nextLink;
      trailTree[z].nextLink = _z;
      _z = z;
      z = _nextLink;
    }

    // if this function was called because a solider is about to embark on an
    // actual route (as opposed to "test" path finding (used by cursor, etc),
    // then grab all pertinent data and copy into soldier's database
    if (bCopy == COPYROUTE) {
      z = _z;

      for (iCnt = 0; z && (iCnt < MAX_PATH_LIST_SIZE); iCnt++) {
        s->usPathingData[iCnt] = (int16_t)trailTree[z].stepDir;

        z = trailTree[z].nextLink;
      }

      s->usPathIndex = 0;
      s->usPathDataSize = (uint16_t)iCnt;

    } else if (bCopy == NO_COPYROUTE) {
      z = _z;

      for (iCnt = 0; z != 0; iCnt++) {
        Assert(iCnt < lengthof(guiPathingData));  // XXX TODO001C
        guiPathingData[iCnt] = trailTree[z].stepDir;

        z = trailTree[z].nextLink;
      }

      giPathDataSize = (uint16_t)iCnt;
    }

#if defined(PATHAI_VISIBLE_DEBUG)
    if (gfDisplayCoverValues && gfDrawPathPoints) {
      SetRenderFlags(RENDER_FLAG_FULL);
      RenderWorld();
      RenderCoverDebug();
      InvalidateScreen();
      RefreshScreen();
    }
#endif

// return path length : serves as a "successful" flag and a path length counter
#ifdef COUNT_PATHS
    guiSuccessfulPathChecks++;
#endif
    gubNPCAPBudget = 0;
    gubNPCDistLimit = 0;

    // TEMP:  This is returning zero when I am generating edgepoints, so I am
    // force returning 1 until
    //       this is fixed?
    if (gfGeneratingMapEdgepoints) {
      return TRUE;
    }

    return (iCnt);
  }

#ifdef COUNT_PATHS
  if (fCopyReachable) {
    // actually this is a success
    guiSuccessfulPathChecks++;
  } else {
    guiUnsuccessfulPathChecks++;
  }
#endif

  // failed miserably, report...
  gubNPCAPBudget = 0;
  gubNPCDistLimit = 0;
  return (0);
}

void GlobalReachableTest(int16_t sStartGridNo) {
  SOLDIERTYPE s;

  memset(&s, 0, sizeof(SOLDIERTYPE));
  s.sGridNo = sStartGridNo;
  s.bLevel = 0;
  s.bTeam = 1;

  // reset the flag for gridno's
  FOR_EACH_WORLD_TILE(i) { i->uiFlags &= ~MAPELEMENT_REACHABLE; }

  ReconfigurePathAI(ABSMAX_SKIPLIST_LEVEL, ABSMAX_TRAIL_TREE, ABSMAX_PATHQ);
  FindBestPath(&s, NOWHERE, 0, WALKING, COPYREACHABLE, PATH_THROUGH_PEOPLE);
  RestorePathAIToDefaults();
}

void LocalReachableTest(int16_t sStartGridNo, int8_t bRadius) {
  SOLDIERTYPE s;
  int32_t iCurrentGridNo = 0;
  int32_t iX, iY;

  memset(&s, 0, sizeof(SOLDIERTYPE));
  s.sGridNo = sStartGridNo;

  // if we are moving on the gorund level
  if (gsInterfaceLevel == I_ROOF_LEVEL) {
    s.bLevel = 1;
  } else {
    s.bLevel = 0;
  }

  s.bTeam = OUR_TEAM;

  // reset the flag for gridno's
  for (iY = -bRadius; iY <= bRadius; iY++) {
    for (iX = -bRadius; iX <= bRadius; iX++) {
      iCurrentGridNo = sStartGridNo + iX + iY * MAXCOL;
      if (iCurrentGridNo >= 0 && iCurrentGridNo <= WORLD_MAX) {
        gpWorldLevelData[iCurrentGridNo].uiFlags &= ~(MAPELEMENT_REACHABLE);
      }
    }
  }

  // set the dist limit
  gubNPCDistLimit = bRadius;
  // make the function call
  FindBestPath(&s, NOWHERE, s.bLevel, WALKING, COPYREACHABLE, PATH_THROUGH_PEOPLE);
  // reset dist limit
  gubNPCDistLimit = 0;
}

void GlobalItemsReachableTest(int16_t sStartGridNo1, int16_t sStartGridNo2) {
  SOLDIERTYPE s;

  memset(&s, 0, sizeof(SOLDIERTYPE));
  s.sGridNo = sStartGridNo1;
  s.bLevel = 0;
  s.bTeam = 1;

  // reset the flag for gridno's
  FOR_EACH_WORLD_TILE(i) { i->uiFlags &= ~MAPELEMENT_REACHABLE; }

  ReconfigurePathAI(ABSMAX_SKIPLIST_LEVEL, ABSMAX_TRAIL_TREE, ABSMAX_PATHQ);
  FindBestPath(&s, NOWHERE, 0, WALKING, COPYREACHABLE, PATH_THROUGH_PEOPLE);
  if (sStartGridNo2 != NOWHERE) {
    s.sGridNo = sStartGridNo2;
    FindBestPath(&s, NOWHERE, 0, WALKING, COPYREACHABLE, PATH_THROUGH_PEOPLE);
  }
  RestorePathAIToDefaults();
}

void RoofReachableTest(int16_t sStartGridNo, uint8_t ubBuildingID) {
  SOLDIERTYPE s;

  memset(&s, 0, sizeof(SOLDIERTYPE));
  s.sGridNo = sStartGridNo;
  s.bLevel = 1;
  s.bTeam = 1;

  gubBuildingInfoToSet = ubBuildingID;

  ReconfigurePathAI(ABSMAX_SKIPLIST_LEVEL, ABSMAX_TRAIL_TREE, ABSMAX_PATHQ);
  FindBestPath(&s, NOWHERE, 1, WALKING, COPYREACHABLE, 0);
  RestorePathAIToDefaults();

  // set start position to reachable since path code sets it unreachable
  gpWorldLevelData[sStartGridNo].uiFlags |= MAPELEMENT_REACHABLE;

  // reset building variable
  gubBuildingInfoToSet = 0;
}

void ErasePath() {
  int16_t iCnt;

  // NOTE: This routine must be called BEFORE anything happens that changes
  //       a merc's gridno, else the....

  // EraseAPCursor();

  if (gfUIHandleShowMoveGrid) {
    gfUIHandleShowMoveGrid = FALSE;

    RemoveTopmost(gsUIHandleShowMoveGridLocation, FIRSTPOINTERS4);
    RemoveTopmost(gsUIHandleShowMoveGridLocation, FIRSTPOINTERS9);
    RemoveTopmost(gsUIHandleShowMoveGridLocation, FIRSTPOINTERS2);
    RemoveTopmost(gsUIHandleShowMoveGridLocation, FIRSTPOINTERS13);
    RemoveTopmost(gsUIHandleShowMoveGridLocation, FIRSTPOINTERS15);
    RemoveTopmost(gsUIHandleShowMoveGridLocation, FIRSTPOINTERS19);
    RemoveTopmost(gsUIHandleShowMoveGridLocation, FIRSTPOINTERS20);
  }

  if (!gusPathShown) {
    // OldPath = FALSE;
    return;
  }

  // if (OldPath > 0 && !eraseOldOne)
  //   return;

  // OldPath = FALSE;

  gusPathShown = FALSE;

  for (iCnt = 0; iCnt < giPlotCnt; iCnt++) {
    // Grid[PlottedPath[cnt]].fstep = 0;

    RemoveAllObjectsOfTypeRange(guiPlottedPath[iCnt], FOOTPRINTS, FOOTPRINTS);

    RemoveAllOnRoofsOfTypeRange(guiPlottedPath[iCnt], FOOTPRINTS, FOOTPRINTS);

    // RemoveAllObjectsOfTypeRange( guiPlottedPath[iCnt], FIRSTPOINTERS,
    // FIRSTPOINTERS );
  }

  // for (cnt=0; cnt < GRIDSIZE; cnt++)
  //    Grid[cnt].fstep = 0;

  giPlotCnt = 0;
  memset(guiPlottedPath, 0, 256 * sizeof(uint32_t));
}

int16_t PlotPath(SOLDIERTYPE *const pSold, const int16_t sDestGridno, const int8_t bCopyRoute,
                 const int8_t bPlot, const uint16_t usMovementMode, const int16_t sAPBudget) {
  int16_t sTileCost, sPoints = 0, sTempGrid, sAnimCost = 0;
  int16_t sPointsWalk = 0, sPointsCrawl = 0, sPointsRun = 0, sPointsSwat = 0;
  int16_t sExtraCostStand, sExtraCostSwat, sExtraCostCrawl;
  int32_t iCnt;
  int16_t sFootOrderIndex;
  int16_t sSwitchValue;
  int16_t sFootOrder[5] = {GREENSTEPSTART, PURPLESTEPSTART, BLUESTEPSTART, ORANGESTEPSTART,
                           REDSTEPSTART};
  uint16_t usTileNum;
  LEVELNODE *pNode;
  uint16_t usMovementModeToUseForAPs;
  BOOLEAN bIgnoreNextCost = FALSE;
  int16_t sTestGridno;

  if (bPlot && gusPathShown) {
    ErasePath();
  }

  gusAPtsToMove = 0;
  sTempGrid = (int16_t)pSold->sGridNo;

  sFootOrderIndex = 0;

  // gubNPCMovementMode = (uint8_t) usMovementMode;
  // distance limit to reduce the cost of plotting a path to a location we can't
  // reach

  // For now, use known hight adjustment
  if (gfRecalculatingExistingPathCost ||
      FindBestPath(pSold, sDestGridno, (int8_t)pSold->bLevel, usMovementMode, bCopyRoute, 0)) {
    // if soldier would be STARTING to run then he pays a penalty since it takes
    // time to run full speed
    if (pSold->usAnimState != RUNNING) {
      // for estimation purposes, always pay penalty
      sPointsRun = AP_START_RUN_COST;
    }

    // Add to points, those needed to start from different stance!
    sPoints += MinAPsToStartMovement(pSold, usMovementMode);

    // We should reduce points for starting to run if first tile is a fence...
    sTestGridno = NewGridNo(pSold->sGridNo, (int16_t)DirectionInc((uint16_t)guiPathingData[0]));
    if (gubWorldMovementCosts[sTestGridno][(int8_t)guiPathingData[0]][pSold->bLevel] ==
        TRAVELCOST_FENCE) {
      if (usMovementMode == RUNNING && pSold->usAnimState != RUNNING) {
        sPoints -= AP_START_RUN_COST;
      }
    }

    // FIRST, add up "startup" additional costs - such as intermediate
    // animations, etc. switch(pSold->usAnimState)
    {
      // case START_AID   :
      // case GIVING_AID  :	sAnimCost = AP_STOP_FIRST_AID;
      //										break;
      // case TWISTOMACH  :
      // case COLLAPSED   :	sAnimCost = AP_GET_UP;
      //										break;
      // case TWISTBACK   :
      // case UNCONSCIOUS :	sAnimCost = (AP_ROLL_OVER+AP_GET_UP);
      //										break;

      //	case CROUCHING	 :  if (usMovementMode == WALKING ||
      // usMovementMode == RUNNING)
      // sAnimCost = AP_CROUCH;
      // break;
    }

    sPoints += sAnimCost;
    gusAPtsToMove += sAnimCost;

    const int32_t iLastGrid = giPathDataSize;
    for (iCnt = 0; iCnt < iLastGrid; iCnt++) {
      sExtraCostStand = 0;
      sExtraCostSwat = 0;
      sExtraCostCrawl = 0;

      sTempGrid = NewGridNo(sTempGrid, (int16_t)DirectionInc((uint16_t)guiPathingData[iCnt]));

      // Get switch value...
      sSwitchValue = gubWorldMovementCosts[sTempGrid][(int8_t)guiPathingData[iCnt]][pSold->bLevel];

      // get the tile cost for that tile based on WALKING
      sTileCost =
          TerrainActionPoints(pSold, sTempGrid, (int8_t)guiPathingData[iCnt], pSold->bLevel);

      usMovementModeToUseForAPs = usMovementMode;

      // ATE - MAKE MOVEMENT ALWAYS WALK IF IN WATER
      if (gpWorldLevelData[sTempGrid].ubTerrainID == DEEP_WATER ||
          gpWorldLevelData[sTempGrid].ubTerrainID == MED_WATER ||
          gpWorldLevelData[sTempGrid].ubTerrainID == LOW_WATER) {
        usMovementModeToUseForAPs = WALKING;
      }

      if (bIgnoreNextCost) {
        bIgnoreNextCost = FALSE;
      } else {
        // ATE: If we have a 'special cost, like jump fence...
        if (sSwitchValue == TRAVELCOST_FENCE) {
          sPoints += sTileCost;

          bIgnoreNextCost = TRUE;

          // If we are changeing stance ( either before or after getting
          // there.... We need to reflect that...
          switch (usMovementModeToUseForAPs) {
            case RUNNING:
            case WALKING:

              // Add here cost to go from crouch to stand AFTER fence hop....
              // Since it's AFTER.. make sure we will be moving after jump...
              if ((iCnt + 2) < iLastGrid) {
                sExtraCostStand += AP_CROUCH;

                // ATE: if running, charge extra point to srart again
                if (usMovementModeToUseForAPs == RUNNING) {
                  sExtraCostStand++;
                }

                sPoints += sExtraCostStand;
              }
              break;

            case SWATTING:

              // Add cost to stand once there BEFORE....
              sExtraCostSwat += AP_CROUCH;
              sPoints += sExtraCostSwat;
              break;

            case CRAWLING:

              // Can't do it here.....
              break;
          }
        } else if (sTileCost > 0) {
          // else, movement is adjusted based on mode...

          if (sSwitchValue == TRAVELCOST_NOT_STANDING) {
            switch (usMovementModeToUseForAPs) {
              case RUNNING:
              case WALKING:
                // charge crouch APs for ducking head!
                sExtraCostStand += AP_CROUCH;
                break;

              default:
                break;
            }
          }

          // so, then we must modify it for other movement styles and accumulate
          switch (usMovementModeToUseForAPs) {
            case RUNNING:
              sPoints += (int16_t)(double)((sTileCost / RUNDIVISOR)) + sExtraCostStand;
              break;
            case WALKING:
              sPoints += (sTileCost + WALKCOST) + sExtraCostStand;
              break;
            case SWATTING:
              sPoints += (sTileCost + SWATCOST) + sExtraCostSwat;
              break;
            case CRAWLING:
              sPoints += (sTileCost + CRAWLCOST) + sExtraCostCrawl;
              break;
            default:
              sPoints += sTileCost;
              break;
          }
        }
      }

      // THIS NEXT SECTION ONLY NEEDS TO HAPPEN FOR CURSOR UI FEEDBACK, NOT
      // ACTUAL COSTING

      if (bPlot && ((gTacticalStatus.uiFlags & TURNBASED) &&
                    (gTacticalStatus.uiFlags & INCOMBAT)))  // OR USER OPTION ON... ***)
      {
        // ATE; TODO: Put stuff in here to allow for fact of costs other than
        // movement ( jump fence, open door )

        // store WALK cost
        sPointsWalk += (sTileCost + WALKCOST) + sExtraCostStand;

        // now get cost as if CRAWLING
        sPointsCrawl += (sTileCost + CRAWLCOST) + sExtraCostCrawl;

        // now get cost as if SWATTING
        sPointsSwat += (sTileCost + SWATCOST) + sExtraCostSwat;

        // now get cost as if RUNNING
        sPointsRun += (int16_t)(double)((sTileCost / RUNDIVISOR)) + sExtraCostStand;
      }

      if (iCnt == 0 && bPlot) {
        gusAPtsToMove = sPoints;

        giPlotCnt = 0;
      }

      // if ( gTacticalStatus.uiFlags & TURNBASED && (gTacticalStatus.uiFlags &
      // INCOMBAT) ) // OR USER OPTION "show paths" ON... ***
      {
        if (bPlot && iCnt < iLastGrid - 1) {
          Assert(giPlotCnt < lengthof(guiPlottedPath));  // XXX TODO001C
          guiPlottedPath[giPlotCnt++] = sTempGrid;

          // we need a footstep graphic ENTERING the next tile

          // get the direction
          usTileNum = (uint16_t)guiPathingData[iCnt] + 2;
          if (usTileNum > 8) {
            usTileNum = 1;
          }

          // Are we a vehicle?
          if (pSold->uiStatusFlags & SOLDIER_VEHICLE) {
            // did we exceed WALK cost?
            if (sPointsSwat > sAPBudget) {
              sFootOrderIndex = 4;
            } else {
              sFootOrderIndex = 3;
            }
          } else {
            // did we exceed CRAWL cost?
            if (sFootOrderIndex == 0 && sPointsCrawl > sAPBudget) {
              sFootOrderIndex++;
            }

            // did we exceed WALK cost?
            if (sFootOrderIndex == 1 && sPointsSwat > sAPBudget) {
              sFootOrderIndex++;
            }

            // did we exceed SWAT cost?
            if (sFootOrderIndex == 2 && sPointsWalk > sAPBudget) {
              sFootOrderIndex++;
            }

            // did we exceed RUN cost?
            if (sFootOrderIndex == 3 && sPointsRun > sAPBudget) {
              sFootOrderIndex++;
            }
          }

          uint16_t usTileIndex;
          usTileIndex = GetTileIndexFromTypeSubIndex(FOOTPRINTS, usTileNum);

          // Adjust based on what mode we are in...
          if ((gTacticalStatus.uiFlags & REALTIME) || !(gTacticalStatus.uiFlags & INCOMBAT)) {
            // find out which color we're using
            usTileIndex += sFootOrder[4];
          } else  // turn based
          {
            // find out which color we're using
            usTileIndex += sFootOrder[sFootOrderIndex];
          }

          /*
          if ( sPoints <= sAPBudget)
          {
                  // find out which color we're using
                  usTileIndex += sFootOrder[sFootOrderIndex];
          }
          else
          {
                  // use red footprints ( offset by 16 )
                  usTileIndex += REDSTEPSTART;
          }
          */

          if (pSold->bLevel == 0) {
            pNode = AddObjectToTail(sTempGrid, usTileIndex);
            pNode->ubShadeLevel = DEFAULT_SHADE_LEVEL;
            pNode->ubNaturalShadeLevel = DEFAULT_SHADE_LEVEL;
          } else {
            pNode = AddOnRoofToTail(sTempGrid, usTileIndex);
            pNode->ubShadeLevel = DEFAULT_SHADE_LEVEL;
            pNode->ubNaturalShadeLevel = DEFAULT_SHADE_LEVEL;
          }

          // we need a footstep graphic LEAVING this tile

          // get the direction using the NEXT tile (thus iCnt+1 as index)
          usTileNum = (uint16_t)guiPathingData[iCnt + 1] + 2;
          if (usTileNum > 8) {
            usTileNum = 1;
          }

          // this is a LEAVING footstep which is always the second set of 8
          usTileNum += 8;

          usTileIndex = GetTileIndexFromTypeSubIndex(FOOTPRINTS, usTileNum);

          // Adjust based on what mode we are in...
          if ((gTacticalStatus.uiFlags & REALTIME) || !(gTacticalStatus.uiFlags & INCOMBAT)) {
            // find out which color we're using
            usTileIndex += sFootOrder[4];
          } else  // turnbased
          {
            // find out which color we're using
            usTileIndex += sFootOrder[sFootOrderIndex];
          }

          if (pSold->bLevel == 0) {
            pNode = AddObjectToTail(sTempGrid, usTileIndex);
            pNode->ubShadeLevel = DEFAULT_SHADE_LEVEL;
            pNode->ubNaturalShadeLevel = DEFAULT_SHADE_LEVEL;
          } else {
            pNode = AddOnRoofToTail(sTempGrid, usTileIndex);
            pNode->ubShadeLevel = DEFAULT_SHADE_LEVEL;
            pNode->ubNaturalShadeLevel = DEFAULT_SHADE_LEVEL;
          }
        }

      }  // end of if turn based or real-time user option "show paths" on...
    }

    if (bPlot) {
      gusPathShown = TRUE;
    }

  }  // end of found a path

  // reset distance limit
  gubNPCDistLimit = 0;

  return (sPoints);
}

int16_t UIPlotPath(SOLDIERTYPE *const pSold, const int16_t sDestGridno, const int8_t bCopyRoute,
                   int8_t bPlot, const uint16_t usMovementMode, const int16_t sAPBudget) {
  // This function is specifically for UI calls to the pathing routine, to
  // check whether the shift key is pressed, etc.

  if (IsKeyDown(SHIFT)) {
    gfPlotDirectPath = TRUE;
  }

  // If we are on the same level as the interface level, continue, else return
  if (pSold->bLevel != gsInterfaceLevel) {
    return (0);
  }

  if (gGameSettings.fOptions[TOPTION_ALWAYS_SHOW_MOVEMENT_PATH]) {
    bPlot = TRUE;
  }

  const int16_t sRet = PlotPath(pSold, sDestGridno, bCopyRoute, bPlot, usMovementMode, sAPBudget);
  gfPlotDirectPath = FALSE;
  return (sRet);
}

int16_t RecalculatePathCost(SOLDIERTYPE *pSoldier, uint16_t usMovementMode) {
  // AI function for a soldier already with a path; this will return the cost of
  // that path using the given movement mode

  if (!pSoldier->bPathStored || pSoldier->usPathDataSize == 0) {
    return (0);
  }

  gfRecalculatingExistingPathCost = TRUE;
  const int16_t sRet =
      PlotPath(pSoldier, pSoldier->sFinalDestination, NO_COPYROUTE, FALSE, usMovementMode, 0);
  gfRecalculatingExistingPathCost = FALSE;
  return (sRet);
}

int16_t EstimatePlotPath(SOLDIERTYPE *const pSold, const int16_t sDestGridno,
                         const int8_t bCopyRoute, const int8_t bPlot, const uint16_t usMovementMode,
                         const int16_t sAPBudget) {
  // This function is specifically for AI calls to estimate path cost to a
  // location It sets stuff up to ignore all people

  gfEstimatePath = TRUE;

  const int16_t sRet = PlotPath(pSold, sDestGridno, bCopyRoute, bPlot, usMovementMode, sAPBudget);

  gfEstimatePath = FALSE;

  return (sRet);
}

uint8_t InternalDoorTravelCost(const SOLDIERTYPE *pSoldier, int32_t iGridNo, uint8_t ubMovementCost,
                               BOOLEAN fReturnPerceivedValue, int32_t *piDoorGridNo,
                               BOOLEAN fReturnDoorCost) {
  // This function will return either TRAVELCOST_DOOR (in place of closed door
  // cost), TRAVELCOST_OBSTACLE, or the base ground terrain travel cost,
  // depending on whether or not the door is open or closed etc.
  BOOLEAN fDoorIsObstacleIfClosed = FALSE;
  int32_t iDoorGridNo = -1;
  DOOR_STATUS *pDoorStatus;
  DOOR *pDoor;
  STRUCTURE *pDoorStructure;
  BOOLEAN fDoorIsOpen;
  uint8_t ubReplacementCost;

  if (IS_TRAVELCOST_DOOR(ubMovementCost)) {
    ubReplacementCost = TRAVELCOST_OBSTACLE;

    switch (ubMovementCost) {
      case TRAVELCOST_DOOR_CLOSED_HERE:
        fDoorIsObstacleIfClosed = TRUE;
        iDoorGridNo = iGridNo;
        ubReplacementCost = TRAVELCOST_DOOR;
        break;
      case TRAVELCOST_DOOR_CLOSED_N:
        fDoorIsObstacleIfClosed = TRUE;
        iDoorGridNo = iGridNo + dirDelta[NORTH];
        ubReplacementCost = TRAVELCOST_DOOR;
        break;
      case TRAVELCOST_DOOR_CLOSED_W:
        fDoorIsObstacleIfClosed = TRUE;
        iDoorGridNo = iGridNo + dirDelta[WEST];
        ubReplacementCost = TRAVELCOST_DOOR;
        break;
      case TRAVELCOST_DOOR_OPEN_HERE:
        fDoorIsObstacleIfClosed = FALSE;
        iDoorGridNo = iGridNo;
        break;
      case TRAVELCOST_DOOR_OPEN_N:
        fDoorIsObstacleIfClosed = FALSE;
        iDoorGridNo = iGridNo + dirDelta[NORTH];
        break;
      case TRAVELCOST_DOOR_OPEN_NE:
        fDoorIsObstacleIfClosed = FALSE;
        iDoorGridNo = iGridNo + dirDelta[NORTHEAST];
        break;
      case TRAVELCOST_DOOR_OPEN_E:
        fDoorIsObstacleIfClosed = FALSE;
        iDoorGridNo = iGridNo + dirDelta[EAST];
        break;
      case TRAVELCOST_DOOR_OPEN_SE:
        fDoorIsObstacleIfClosed = FALSE;
        iDoorGridNo = iGridNo + dirDelta[SOUTHEAST];
        break;
      case TRAVELCOST_DOOR_OPEN_S:
        fDoorIsObstacleIfClosed = FALSE;
        iDoorGridNo = iGridNo + dirDelta[SOUTH];
        break;
      case TRAVELCOST_DOOR_OPEN_SW:
        fDoorIsObstacleIfClosed = FALSE;
        iDoorGridNo = iGridNo + dirDelta[SOUTHWEST];
        break;
      case TRAVELCOST_DOOR_OPEN_W:
        fDoorIsObstacleIfClosed = FALSE;
        iDoorGridNo = iGridNo + dirDelta[WEST];
        break;
      case TRAVELCOST_DOOR_OPEN_NW:
        fDoorIsObstacleIfClosed = FALSE;
        iDoorGridNo = iGridNo + dirDelta[NORTHWEST];
        break;
      case TRAVELCOST_DOOR_OPEN_N_N:
        fDoorIsObstacleIfClosed = FALSE;
        iDoorGridNo = iGridNo + dirDelta[NORTH] + dirDelta[NORTH];
        break;
      case TRAVELCOST_DOOR_OPEN_NW_N:
        fDoorIsObstacleIfClosed = FALSE;
        iDoorGridNo = iGridNo + dirDelta[NORTHWEST] + dirDelta[NORTH];
        break;
      case TRAVELCOST_DOOR_OPEN_NE_N:
        fDoorIsObstacleIfClosed = FALSE;
        iDoorGridNo = iGridNo + dirDelta[NORTHEAST] + dirDelta[NORTH];
        break;
      case TRAVELCOST_DOOR_OPEN_W_W:
        fDoorIsObstacleIfClosed = FALSE;
        iDoorGridNo = iGridNo + dirDelta[WEST] + dirDelta[WEST];
        break;
      case TRAVELCOST_DOOR_OPEN_SW_W:
        fDoorIsObstacleIfClosed = FALSE;
        iDoorGridNo = iGridNo + dirDelta[SOUTHWEST] + dirDelta[WEST];
        break;
      case TRAVELCOST_DOOR_OPEN_NW_W:
        fDoorIsObstacleIfClosed = FALSE;
        iDoorGridNo = iGridNo + dirDelta[NORTHWEST] + dirDelta[WEST];
        break;
      default:
        break;
    }

    if (pSoldier &&
        (pSoldier->uiStatusFlags & SOLDIER_MONSTER || pSoldier->uiStatusFlags & SOLDIER_ANIMAL)) {
      // can't open doors!
      ubReplacementCost = TRAVELCOST_OBSTACLE;
    }

    if (piDoorGridNo) {
      // return gridno of door through pointer
      *piDoorGridNo = iDoorGridNo;
    }

    if (fReturnPerceivedValue &&
        gpWorldLevelData[iDoorGridNo].ubExtFlags[0] & MAPELEMENT_EXT_DOOR_STATUS_PRESENT) {
      // check door status
      pDoorStatus = GetDoorStatus((int16_t)iDoorGridNo);
      if (pDoorStatus) {
        fDoorIsOpen = (pDoorStatus->ubFlags & DOOR_PERCEIVED_OPEN) != 0;
      } else {
        // abort!
        return (ubMovementCost);
      }
    } else {
      // check door structure
      pDoorStructure = FindStructure((int16_t)iDoorGridNo, STRUCTURE_ANYDOOR);
      if (pDoorStructure) {
        fDoorIsOpen = (pDoorStructure->fFlags & STRUCTURE_OPEN) != 0;
      } else {
        // abort!
        return (ubMovementCost);
      }
    }
    // now determine movement cost
    if (fDoorIsOpen) {
      if (fDoorIsObstacleIfClosed) {
        ubMovementCost = gTileTypeMovementCost[gpWorldLevelData[iGridNo].ubTerrainID];
      } else {
        ubMovementCost = ubReplacementCost;
      }
    } else {
      if (fDoorIsObstacleIfClosed) {
        // door is closed and this should be an obstacle, EXCEPT if we are
        // calculating a path for an enemy or NPC with keys

        // creatures and animals can't open doors!
        if (fReturnPerceivedValue || (pSoldier && (pSoldier->uiStatusFlags & SOLDIER_MONSTER ||
                                                   pSoldier->uiStatusFlags & SOLDIER_ANIMAL))) {
          ubMovementCost = ubReplacementCost;
        } else {
          // have to check if door is locked and NPC does not have keys!
          pDoor = FindDoorInfoAtGridNo(iDoorGridNo);
          if (pDoor) {
            if ((!pDoor->fLocked || (pSoldier && pSoldier->bHasKeys)) && !fReturnDoorCost) {
              ubMovementCost = gTileTypeMovementCost[gpWorldLevelData[iGridNo].ubTerrainID];
            } else {
              ubMovementCost = ubReplacementCost;
            }
          } else {
            ubMovementCost = ubReplacementCost;
          }
        }
      } else {
        ubMovementCost = gTileTypeMovementCost[gpWorldLevelData[iGridNo].ubTerrainID];
      }
    }
  }
  return (ubMovementCost);
}

uint8_t DoorTravelCost(const SOLDIERTYPE *pSoldier, int32_t iGridNo, uint8_t ubMovementCost,
                       BOOLEAN fReturnPerceivedValue, int32_t *piDoorGridNo) {
  return (InternalDoorTravelCost(pSoldier, iGridNo, ubMovementCost, fReturnPerceivedValue,
                                 piDoorGridNo, FALSE));
}
