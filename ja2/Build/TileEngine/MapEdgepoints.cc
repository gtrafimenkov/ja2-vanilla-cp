#include "TileEngine/MapEdgepoints.h"

#include <algorithm>
#include <string.h>

#include "Macro.h"
#include "SGP/Buffer.h"
#include "SGP/Debug.h"
#include "SGP/FileMan.h"
#include "SGP/MemMan.h"
#include "SGP/Random.h"
#include "Strategic/Strategic.h"
#include "Strategic/StrategicMap.h"
#include "Tactical/AnimationControl.h"
#include "Tactical/MapInformation.h"
#include "Tactical/PathAI.h"
#include "Tactical/SoldierControl.h"
#include "TacticalAI/AI.h"
#include "TileEngine/Environment.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/RenderFun.h"
#include "TileEngine/TileDef.h"
#include "TileEngine/WorldMan.h"
#include "Utils/Message.h"

// dynamic arrays that contain the valid gridno's for each edge
int16_t *gps1stNorthEdgepointArray = NULL;
int16_t *gps1stEastEdgepointArray = NULL;
int16_t *gps1stSouthEdgepointArray = NULL;
int16_t *gps1stWestEdgepointArray = NULL;
// contains the size for each array
uint16_t gus1stNorthEdgepointArraySize = 0;
uint16_t gus1stEastEdgepointArraySize = 0;
uint16_t gus1stSouthEdgepointArraySize = 0;
uint16_t gus1stWestEdgepointArraySize = 0;
// contains the index value for the first array index of the second row of each
// edgepoint array. Because each edgepoint side has two rows, the outside most
// row is calculated first, then the inside row. For purposes of AI, it may
// become necessary to avoid this.
uint16_t gus1stNorthEdgepointMiddleIndex = 0;
uint16_t gus1stEastEdgepointMiddleIndex = 0;
uint16_t gus1stSouthEdgepointMiddleIndex = 0;
uint16_t gus1stWestEdgepointMiddleIndex = 0;

// dynamic arrays that contain the valid gridno's for each edge
int16_t *gps2ndNorthEdgepointArray = NULL;
int16_t *gps2ndEastEdgepointArray = NULL;
int16_t *gps2ndSouthEdgepointArray = NULL;
int16_t *gps2ndWestEdgepointArray = NULL;
// contains the size for each array
uint16_t gus2ndNorthEdgepointArraySize = 0;
uint16_t gus2ndEastEdgepointArraySize = 0;
uint16_t gus2ndSouthEdgepointArraySize = 0;
uint16_t gus2ndWestEdgepointArraySize = 0;
// contains the index value for the first array index of the second row of each
// edgepoint array. Because each edgepoint side has two rows, the outside most
// row is calculated first, then the inside row. For purposes of AI, it may
// become necessary to avoid this.
uint16_t gus2ndNorthEdgepointMiddleIndex = 0;
uint16_t gus2ndEastEdgepointMiddleIndex = 0;
uint16_t gus2ndSouthEdgepointMiddleIndex = 0;
uint16_t gus2ndWestEdgepointMiddleIndex = 0;

BOOLEAN gfEdgepointsExist = FALSE;
BOOLEAN gfGeneratingMapEdgepoints = FALSE;

int16_t gsTLGridNo = 13286;
int16_t gsTRGridNo = 1043;
int16_t gsBLGridNo = 24878;
int16_t gsBRGridNo = 12635;

extern uint8_t gubTacticalDirection;

void TrashMapEdgepoints() {
  // Primary edgepoints
  if (gps1stNorthEdgepointArray) MemFree(gps1stNorthEdgepointArray);
  if (gps1stEastEdgepointArray) MemFree(gps1stEastEdgepointArray);
  if (gps1stSouthEdgepointArray) MemFree(gps1stSouthEdgepointArray);
  if (gps1stWestEdgepointArray) MemFree(gps1stWestEdgepointArray);
  gps1stNorthEdgepointArray = NULL;
  gps1stEastEdgepointArray = NULL;
  gps1stSouthEdgepointArray = NULL;
  gps1stWestEdgepointArray = NULL;
  gus1stNorthEdgepointArraySize = 0;
  gus1stEastEdgepointArraySize = 0;
  gus1stSouthEdgepointArraySize = 0;
  gus1stWestEdgepointArraySize = 0;
  gus1stNorthEdgepointMiddleIndex = 0;
  gus1stEastEdgepointMiddleIndex = 0;
  gus1stSouthEdgepointMiddleIndex = 0;
  gus1stWestEdgepointMiddleIndex = 0;
  // Secondary edgepoints
  if (gps2ndNorthEdgepointArray) MemFree(gps2ndNorthEdgepointArray);
  if (gps2ndEastEdgepointArray) MemFree(gps2ndEastEdgepointArray);
  if (gps2ndSouthEdgepointArray) MemFree(gps2ndSouthEdgepointArray);
  if (gps2ndWestEdgepointArray) MemFree(gps2ndWestEdgepointArray);
  gps2ndNorthEdgepointArray = NULL;
  gps2ndEastEdgepointArray = NULL;
  gps2ndSouthEdgepointArray = NULL;
  gps2ndWestEdgepointArray = NULL;
  gus2ndNorthEdgepointArraySize = 0;
  gus2ndEastEdgepointArraySize = 0;
  gus2ndSouthEdgepointArraySize = 0;
  gus2ndWestEdgepointArraySize = 0;
  gus2ndNorthEdgepointMiddleIndex = 0;
  gus2ndEastEdgepointMiddleIndex = 0;
  gus2ndSouthEdgepointMiddleIndex = 0;
  gus2ndWestEdgepointMiddleIndex = 0;
}

static BOOLEAN VerifyEdgepoint(SOLDIERTYPE *pSoldier, int16_t sEdgepoint);

static void ValidateMapEdge(SOLDIERTYPE &s, uint16_t &n, uint16_t &middle_idx,
                            int16_t *const array) {
  int16_t *dst = array;
  int16_t const *middle = array + middle_idx;
  int16_t const *const end = array + n;
  for (int16_t const *i = array; i != end; ++i) {
    if (VerifyEdgepoint(&s, *i)) {
      *dst = *i;
      if (middle == i) middle = dst;  // Adjust the middle index to the new one
      ++dst;
    } else if (middle == i) {  // Increment the middle index because its
                               // edgepoint is no longer valid
      ++middle;
    }
  }
  middle_idx = middle - array;
  n = dst - array;
}

/* This final step eliminates some edgepoints which actually don't path directly
 * to the edge of the map. Cases would include an area that is close to the
 * edge, but a fence blocks it from direct access to the edge of the map. */
static void ValidateEdgepoints() {
  SOLDIERTYPE s;
  memset(&s, 0, sizeof(s));
  s.bTeam = ENEMY_TEAM;

  ValidateMapEdge(s, gus1stNorthEdgepointArraySize, gus1stNorthEdgepointMiddleIndex,
                  gps1stNorthEdgepointArray);
  ValidateMapEdge(s, gus1stEastEdgepointArraySize, gus1stEastEdgepointMiddleIndex,
                  gps1stEastEdgepointArray);
  ValidateMapEdge(s, gus1stSouthEdgepointArraySize, gus1stSouthEdgepointMiddleIndex,
                  gps1stSouthEdgepointArray);
  ValidateMapEdge(s, gus1stWestEdgepointArraySize, gus1stWestEdgepointMiddleIndex,
                  gps1stWestEdgepointArray);

  ValidateMapEdge(s, gus2ndNorthEdgepointArraySize, gus2ndNorthEdgepointMiddleIndex,
                  gps2ndNorthEdgepointArray);
  ValidateMapEdge(s, gus2ndEastEdgepointArraySize, gus2ndEastEdgepointMiddleIndex,
                  gps2ndEastEdgepointArray);
  ValidateMapEdge(s, gus2ndSouthEdgepointArraySize, gus2ndSouthEdgepointMiddleIndex,
                  gps2ndSouthEdgepointArray);
  ValidateMapEdge(s, gus2ndWestEdgepointArraySize, gus2ndWestEdgepointMiddleIndex,
                  gps2ndWestEdgepointArray);
}

static void CompactEdgepointArray(int16_t **psArray, uint16_t *pusMiddleIndex,
                                  uint16_t *pusArraySize) {
  int32_t i;
  uint16_t usArraySize, usValidIndex = 0;

  usArraySize = *pusArraySize;

  for (i = 0; i < usArraySize; i++) {
    if ((*psArray)[i] == -1) {
      (*pusArraySize)--;
      if (i < *pusMiddleIndex) {
        (*pusMiddleIndex)--;
      }
    } else {
      if (usValidIndex != i) {
        (*psArray)[usValidIndex] = (*psArray)[i];
      }
      usValidIndex++;
    }
  }
  *psArray = REALLOC(*psArray, int16_t, *pusArraySize);
}

static BOOLEAN EdgepointsClose(SOLDIERTYPE *pSoldier, int16_t sEdgepoint1, int16_t sEdgepoint2);

static void InternallyClassifyEdgepoints(SOLDIERTYPE *pSoldier, int16_t sGridNo, int16_t **psArray1,
                                         uint16_t *pusMiddleIndex1, uint16_t *pusArraySize1,
                                         int16_t **psArray2, uint16_t *pusMiddleIndex2,
                                         uint16_t *pusArraySize2) {
  int32_t i;
  uint16_t us1stBenchmarkID, us2ndBenchmarkID;
  us1stBenchmarkID = us2ndBenchmarkID = 0xffff;
  if (!(*psArray2)) {
    *psArray2 = MALLOCN(int16_t, 400);
  }
  for (i = 0; i < *pusArraySize1; i++) {
    if (sGridNo == (*psArray1)[i]) {
      if (i < *pusMiddleIndex1) {  // in the first half of the array
        us1stBenchmarkID = (uint16_t)i;
        // find the second benchmark
        for (i = *pusMiddleIndex1; i < *pusArraySize1; i++) {
          if (EdgepointsClose(pSoldier, (*psArray1)[us1stBenchmarkID], (*psArray1)[i])) {
            us2ndBenchmarkID = (uint16_t)i;
            break;
          }
        }
      } else {  // in the second half of the array
        us2ndBenchmarkID = (uint16_t)i;
        // find the first benchmark
        for (i = 0; i < *pusMiddleIndex1; i++) {
          if (EdgepointsClose(pSoldier, (*psArray1)[us2ndBenchmarkID], (*psArray1)[i])) {
            us1stBenchmarkID = (uint16_t)i;
            break;
          }
        }
      }
      break;
    }
  }
  // Now we have found the two benchmarks, so go in both directions for each one
  // to determine which entrypoints are going to be used in the primary array.
  // All rejections will be positioned in the secondary array for use for
  // isolated entry when tactically traversing.
  if (us1stBenchmarkID != 0xffff) {
    for (i = us1stBenchmarkID; i > 0; i--) {
      if (!EdgepointsClose(pSoldier, (*psArray1)[i],
                           (*psArray1)[i - 1])) {  // All edgepoints from index 0
                                                   // to i-1 are rejected.
        while (i) {
          i--;
          (*psArray2)[*pusArraySize2] = (*psArray1)[i];
          (*pusMiddleIndex2)++;
          (*pusArraySize2)++;
          (*psArray1)[i] = -1;
        }
        break;
      }
    }
    for (i = us1stBenchmarkID; i < *pusMiddleIndex1 - 1; i++) {
      if (!EdgepointsClose(pSoldier, (*psArray1)[i],
                           (*psArray1)[i + 1])) {  // All edgepoints from index i+1 to 1st
                                                   // middle index are rejected.
        while (i < *pusMiddleIndex1 - 1) {
          i++;
          (*psArray2)[*pusArraySize2] = (*psArray1)[i];
          (*pusMiddleIndex2)++;
          (*pusArraySize2)++;
          (*psArray1)[i] = -1;
        }
        break;
      }
    }
  }
  if (us2ndBenchmarkID != 0xffff) {
    for (i = us2ndBenchmarkID; i > *pusMiddleIndex1; i--) {
      if (!EdgepointsClose(pSoldier, (*psArray1)[i],
                           (*psArray1)[i - 1])) {  // All edgepoints from 1st middle index  to
                                                   // i-1 are rejected.
        while (i > *pusMiddleIndex1) {
          i--;
          (*psArray2)[*pusArraySize2] = (*psArray1)[i];
          (*pusArraySize2)++;
          (*psArray1)[i] = -1;
        }
        break;
      }
    }
    for (i = us2ndBenchmarkID; i < *pusArraySize1 - 1; i++) {
      if (!EdgepointsClose(pSoldier, (*psArray1)[i],
                           (*psArray1)[i + 1])) {  // All edgepoints from index 0
                                                   // to i-1 are rejected.
        while (i < *pusArraySize1 - 1) {
          i++;
          (*psArray2)[(*pusArraySize2)] = (*psArray1)[i];
          (*pusArraySize2)++;
          (*psArray1)[i] = -1;
        }
        break;
      }
    }
  }
  // Now compact the primary array, because some edgepoints have been removed.
  CompactEdgepointArray(psArray1, pusMiddleIndex1, pusArraySize1);
  *psArray2 = REALLOC(*psArray2, int16_t, *pusArraySize2);
}

static void ClassifyEdgepoints() {
  SOLDIERTYPE Soldier;
  int16_t sGridNo = -1;

  memset(&Soldier, 0, sizeof(SOLDIERTYPE));
  Soldier.bTeam = 1;

  // north
  if (gMapInformation.sNorthGridNo != -1) {
    sGridNo =
        FindNearestEdgepointOnSpecifiedEdge(gMapInformation.sNorthGridNo, NORTH_EDGEPOINT_SEARCH);
    if (sGridNo != NOWHERE) {
      InternallyClassifyEdgepoints(&Soldier, sGridNo, &gps1stNorthEdgepointArray,
                                   &gus1stNorthEdgepointMiddleIndex, &gus1stNorthEdgepointArraySize,
                                   &gps2ndNorthEdgepointArray, &gus2ndNorthEdgepointMiddleIndex,
                                   &gus2ndNorthEdgepointArraySize);
    }
  }
  // east
  if (gMapInformation.sEastGridNo != -1) {
    sGridNo =
        FindNearestEdgepointOnSpecifiedEdge(gMapInformation.sEastGridNo, EAST_EDGEPOINT_SEARCH);
    if (sGridNo != NOWHERE) {
      InternallyClassifyEdgepoints(&Soldier, sGridNo, &gps1stEastEdgepointArray,
                                   &gus1stEastEdgepointMiddleIndex, &gus1stEastEdgepointArraySize,
                                   &gps2ndEastEdgepointArray, &gus2ndEastEdgepointMiddleIndex,
                                   &gus2ndEastEdgepointArraySize);
    }
  }
  // south
  if (gMapInformation.sSouthGridNo != -1) {
    sGridNo =
        FindNearestEdgepointOnSpecifiedEdge(gMapInformation.sSouthGridNo, SOUTH_EDGEPOINT_SEARCH);
    if (sGridNo != NOWHERE) {
      InternallyClassifyEdgepoints(&Soldier, sGridNo, &gps1stSouthEdgepointArray,
                                   &gus1stSouthEdgepointMiddleIndex, &gus1stSouthEdgepointArraySize,
                                   &gps2ndSouthEdgepointArray, &gus2ndSouthEdgepointMiddleIndex,
                                   &gus2ndSouthEdgepointArraySize);
    }
  }
  // west
  if (gMapInformation.sWestGridNo != -1) {
    sGridNo =
        FindNearestEdgepointOnSpecifiedEdge(gMapInformation.sWestGridNo, WEST_EDGEPOINT_SEARCH);
    if (sGridNo != NOWHERE) {
      InternallyClassifyEdgepoints(&Soldier, sGridNo, &gps1stWestEdgepointArray,
                                   &gus1stWestEdgepointMiddleIndex, &gus1stWestEdgepointArraySize,
                                   &gps2ndWestEdgepointArray, &gus2ndWestEdgepointMiddleIndex,
                                   &gus2ndWestEdgepointArraySize);
    }
  }
}

void GenerateMapEdgepoints() {
  int32_t i = -1;
  int16_t sGridNo = -1;
  int16_t sVGridNo[400];

  // Get rid of the current edgepoint lists.
  TrashMapEdgepoints();

  gfGeneratingMapEdgepoints = TRUE;

  if (gMapInformation.sNorthGridNo != -1)
    sGridNo = gMapInformation.sNorthGridNo;
  else if (gMapInformation.sEastGridNo != -1)
    sGridNo = gMapInformation.sEastGridNo;
  else if (gMapInformation.sSouthGridNo != -1)
    sGridNo = gMapInformation.sSouthGridNo;
  else if (gMapInformation.sWestGridNo != -1)
    sGridNo = gMapInformation.sWestGridNo;
  else if (gMapInformation.sCenterGridNo != -1)
    sGridNo = gMapInformation.sCenterGridNo;
  else
    return;

  GlobalReachableTest(sGridNo);

  // Calculate the north edges
  if (gMapInformation.sNorthGridNo != -1) {
    // 1st row
    sGridNo = gsTLGridNo;
    if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
        (!gubWorldRoomInfo[sGridNo] || gfBasement))
      sVGridNo[gus1stNorthEdgepointArraySize++] = sGridNo;
    while (sGridNo > gsTRGridNo) {
      sGridNo++;
      if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
          (!gubWorldRoomInfo[sGridNo] || gfBasement))
        sVGridNo[gus1stNorthEdgepointArraySize++] = sGridNo;
      sGridNo -= 160;
      if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
          (!gubWorldRoomInfo[sGridNo] || gfBasement))
        sVGridNo[gus1stNorthEdgepointArraySize++] = sGridNo;
    }
    // 2nd row
    gus1stNorthEdgepointMiddleIndex = gus1stNorthEdgepointArraySize;
    sGridNo = gsTLGridNo + 161;
    if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
        (!gubWorldRoomInfo[sGridNo] || gfBasement))
      sVGridNo[gus1stNorthEdgepointArraySize++] = sGridNo;
    while (sGridNo > gsTRGridNo + 161) {
      sGridNo++;
      if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
          (!gubWorldRoomInfo[sGridNo] || gfBasement))
        sVGridNo[gus1stNorthEdgepointArraySize++] = sGridNo;
      sGridNo -= 160;
      if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
          (!gubWorldRoomInfo[sGridNo] || gfBasement))
        sVGridNo[gus1stNorthEdgepointArraySize++] = sGridNo;
    }
    if (gus1stNorthEdgepointArraySize) {
      // Allocate and copy over the valid gridnos.
      gps1stNorthEdgepointArray = MALLOCN(int16_t, gus1stNorthEdgepointArraySize);
      for (i = 0; i < gus1stNorthEdgepointArraySize; i++)
        gps1stNorthEdgepointArray[i] = sVGridNo[i];
    }
  }
  // Calculate the east edges
  if (gMapInformation.sEastGridNo != -1) {
    // 1st row
    sGridNo = gsTRGridNo;
    if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
        (!gubWorldRoomInfo[sGridNo] || gfBasement))
      sVGridNo[gus1stEastEdgepointArraySize++] = sGridNo;
    while (sGridNo < gsBRGridNo) {
      sGridNo += 160;
      if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
          (!gubWorldRoomInfo[sGridNo] || gfBasement))
        sVGridNo[gus1stEastEdgepointArraySize++] = sGridNo;
      sGridNo++;
      if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
          (!gubWorldRoomInfo[sGridNo] || gfBasement))
        sVGridNo[gus1stEastEdgepointArraySize++] = sGridNo;
    }
    // 2nd row
    gus1stEastEdgepointMiddleIndex = gus1stEastEdgepointArraySize;
    sGridNo = gsTRGridNo + 159;
    if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
        (!gubWorldRoomInfo[sGridNo] || gfBasement))
      sVGridNo[gus1stEastEdgepointArraySize++] = sGridNo;
    while (sGridNo < gsBRGridNo + 159) {
      sGridNo += 160;
      if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
          (!gubWorldRoomInfo[sGridNo] || gfBasement))
        sVGridNo[gus1stEastEdgepointArraySize++] = sGridNo;
      sGridNo++;
      if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
          (!gubWorldRoomInfo[sGridNo] || gfBasement))
        sVGridNo[gus1stEastEdgepointArraySize++] = sGridNo;
    }
    if (gus1stEastEdgepointArraySize) {  // Allocate and copy over the valid
                                         // gridnos.
      gps1stEastEdgepointArray = MALLOCN(int16_t, gus1stEastEdgepointArraySize);
      for (i = 0; i < gus1stEastEdgepointArraySize; i++) gps1stEastEdgepointArray[i] = sVGridNo[i];
    }
  }
  // Calculate the south edges
  if (gMapInformation.sSouthGridNo != -1) {
    // 1st row
    sGridNo = gsBLGridNo;
    if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
        (!gubWorldRoomInfo[sGridNo] || gfBasement))
      sVGridNo[gus1stSouthEdgepointArraySize++] = sGridNo;
    while (sGridNo > gsBRGridNo) {
      sGridNo++;
      if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
          (!gubWorldRoomInfo[sGridNo] || gfBasement))
        sVGridNo[gus1stSouthEdgepointArraySize++] = sGridNo;
      sGridNo -= 160;
      if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
          (!gubWorldRoomInfo[sGridNo] || gfBasement))
        sVGridNo[gus1stSouthEdgepointArraySize++] = sGridNo;
    }
    // 2nd row
    gus1stSouthEdgepointMiddleIndex = gus1stSouthEdgepointArraySize;
    sGridNo = gsBLGridNo - 161;
    if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
        (!gubWorldRoomInfo[sGridNo] || gfBasement))
      sVGridNo[gus1stSouthEdgepointArraySize++] = sGridNo;
    while (sGridNo > gsBRGridNo - 161) {
      sGridNo++;
      if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
          (!gubWorldRoomInfo[sGridNo] || gfBasement))
        sVGridNo[gus1stSouthEdgepointArraySize++] = sGridNo;
      sGridNo -= 160;
      if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
          (!gubWorldRoomInfo[sGridNo] || gfBasement))
        sVGridNo[gus1stSouthEdgepointArraySize++] = sGridNo;
    }
    if (gus1stSouthEdgepointArraySize) {  // Allocate and copy over the valid
                                          // gridnos.
      gps1stSouthEdgepointArray = MALLOCN(int16_t, gus1stSouthEdgepointArraySize);
      for (i = 0; i < gus1stSouthEdgepointArraySize; i++)
        gps1stSouthEdgepointArray[i] = sVGridNo[i];
    }
  }
  // Calculate the west edges
  if (gMapInformation.sWestGridNo != -1) {
    // 1st row
    sGridNo = gsTLGridNo;
    if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
        (!gubWorldRoomInfo[sGridNo] || gfBasement))
      sVGridNo[gus1stWestEdgepointArraySize++] = sGridNo;
    while (sGridNo < gsBLGridNo) {
      sGridNo++;
      if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
          (!gubWorldRoomInfo[sGridNo] || gfBasement))
        sVGridNo[gus1stWestEdgepointArraySize++] = sGridNo;
      sGridNo += 160;
      if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
          (!gubWorldRoomInfo[sGridNo] || gfBasement))
        sVGridNo[gus1stWestEdgepointArraySize++] = sGridNo;
    }
    // 2nd row
    gus1stWestEdgepointMiddleIndex = gus1stWestEdgepointArraySize;
    sGridNo = gsTLGridNo - 159;
    if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
        (!gubWorldRoomInfo[sGridNo] || gfBasement))
      sVGridNo[gus1stWestEdgepointArraySize++] = sGridNo;
    while (sGridNo < gsBLGridNo - 159) {
      sGridNo++;
      if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
          (!gubWorldRoomInfo[sGridNo] || gfBasement))
        sVGridNo[gus1stWestEdgepointArraySize++] = sGridNo;
      sGridNo += 160;
      if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
          (!gubWorldRoomInfo[sGridNo] || gfBasement))
        sVGridNo[gus1stWestEdgepointArraySize++] = sGridNo;
    }
    if (gus1stWestEdgepointArraySize) {  // Allocate and copy over the valid
                                         // gridnos.
      gps1stWestEdgepointArray = MALLOCN(int16_t, gus1stWestEdgepointArraySize);
      for (i = 0; i < gus1stWestEdgepointArraySize; i++) gps1stWestEdgepointArray[i] = sVGridNo[i];
    }
  }

  // CHECK FOR ISOLATED EDGEPOINTS (but only if the entrypoint is ISOLATED!!!)
  if (gMapInformation.sIsolatedGridNo != -1 &&
      !(gpWorldLevelData[gMapInformation.sIsolatedGridNo].uiFlags & MAPELEMENT_REACHABLE)) {
    GlobalReachableTest(gMapInformation.sIsolatedGridNo);
    if (gMapInformation.sNorthGridNo != -1) {
      // 1st row
      sGridNo = gsTLGridNo;
      if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
          (!gubWorldRoomInfo[sGridNo] || gfBasement))
        sVGridNo[gus2ndNorthEdgepointArraySize++] = sGridNo;
      while (sGridNo > gsTRGridNo) {
        sGridNo++;
        if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
            (!gubWorldRoomInfo[sGridNo] || gfBasement))
          sVGridNo[gus2ndNorthEdgepointArraySize++] = sGridNo;
        sGridNo -= 160;
        if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
            (!gubWorldRoomInfo[sGridNo] || gfBasement))
          sVGridNo[gus2ndNorthEdgepointArraySize++] = sGridNo;
      }
      // 2nd row
      gus2ndNorthEdgepointMiddleIndex = gus2ndNorthEdgepointArraySize;
      sGridNo = gsTLGridNo + 161;
      if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
          (!gubWorldRoomInfo[sGridNo] || gfBasement))
        sVGridNo[gus2ndNorthEdgepointArraySize++] = sGridNo;
      while (sGridNo > gsTRGridNo + 161) {
        sGridNo++;
        if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
            (!gubWorldRoomInfo[sGridNo] || gfBasement))
          sVGridNo[gus2ndNorthEdgepointArraySize++] = sGridNo;
        sGridNo -= 160;
        if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
            (!gubWorldRoomInfo[sGridNo] || gfBasement))
          sVGridNo[gus2ndNorthEdgepointArraySize++] = sGridNo;
      }
      if (gus2ndNorthEdgepointArraySize) {
        // Allocate and copy over the valid gridnos.
        gps2ndNorthEdgepointArray = MALLOCN(int16_t, gus2ndNorthEdgepointArraySize);
        for (i = 0; i < gus2ndNorthEdgepointArraySize; i++)
          gps2ndNorthEdgepointArray[i] = sVGridNo[i];
      }
    }
    // Calculate the east edges
    if (gMapInformation.sEastGridNo != -1) {
      // 1st row
      sGridNo = gsTRGridNo;
      if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
          (!gubWorldRoomInfo[sGridNo] || gfBasement))
        sVGridNo[gus2ndEastEdgepointArraySize++] = sGridNo;
      while (sGridNo < gsBRGridNo) {
        sGridNo += 160;
        if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
            (!gubWorldRoomInfo[sGridNo] || gfBasement))
          sVGridNo[gus2ndEastEdgepointArraySize++] = sGridNo;
        sGridNo++;
        if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
            (!gubWorldRoomInfo[sGridNo] || gfBasement))
          sVGridNo[gus2ndEastEdgepointArraySize++] = sGridNo;
      }
      // 2nd row
      gus2ndEastEdgepointMiddleIndex = gus2ndEastEdgepointArraySize;
      sGridNo = gsTRGridNo + 159;
      if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
          (!gubWorldRoomInfo[sGridNo] || gfBasement))
        sVGridNo[gus2ndEastEdgepointArraySize++] = sGridNo;
      while (sGridNo < gsBRGridNo + 159) {
        sGridNo += 160;
        if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
            (!gubWorldRoomInfo[sGridNo] || gfBasement))
          sVGridNo[gus2ndEastEdgepointArraySize++] = sGridNo;
        sGridNo++;
        if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
            (!gubWorldRoomInfo[sGridNo] || gfBasement))
          sVGridNo[gus2ndEastEdgepointArraySize++] = sGridNo;
      }
      if (gus2ndEastEdgepointArraySize) {  // Allocate and copy over the valid
                                           // gridnos.
        gps2ndEastEdgepointArray = MALLOCN(int16_t, gus2ndEastEdgepointArraySize);
        for (i = 0; i < gus2ndEastEdgepointArraySize; i++)
          gps2ndEastEdgepointArray[i] = sVGridNo[i];
      }
    }
    // Calculate the south edges
    if (gMapInformation.sSouthGridNo != -1) {
      // 1st row
      sGridNo = gsBLGridNo;
      if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
          (!gubWorldRoomInfo[sGridNo] || gfBasement))
        sVGridNo[gus2ndSouthEdgepointArraySize++] = sGridNo;
      while (sGridNo > gsBRGridNo) {
        sGridNo++;
        if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
            (!gubWorldRoomInfo[sGridNo] || gfBasement))
          sVGridNo[gus2ndSouthEdgepointArraySize++] = sGridNo;
        sGridNo -= 160;
        if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
            (!gubWorldRoomInfo[sGridNo] || gfBasement))
          sVGridNo[gus2ndSouthEdgepointArraySize++] = sGridNo;
      }
      // 2nd row
      gus2ndSouthEdgepointMiddleIndex = gus2ndSouthEdgepointArraySize;
      sGridNo = gsBLGridNo - 161;
      if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
          (!gubWorldRoomInfo[sGridNo] || gfBasement))
        sVGridNo[gus2ndSouthEdgepointArraySize++] = sGridNo;
      while (sGridNo > gsBRGridNo - 161) {
        sGridNo++;
        if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
            (!gubWorldRoomInfo[sGridNo] || gfBasement))
          sVGridNo[gus2ndSouthEdgepointArraySize++] = sGridNo;
        sGridNo -= 160;
        if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
            (!gubWorldRoomInfo[sGridNo] || gfBasement))
          sVGridNo[gus2ndSouthEdgepointArraySize++] = sGridNo;
      }
      if (gus2ndSouthEdgepointArraySize) {  // Allocate and copy over the valid
                                            // gridnos.
        gps2ndSouthEdgepointArray = MALLOCN(int16_t, gus2ndSouthEdgepointArraySize);
        for (i = 0; i < gus2ndSouthEdgepointArraySize; i++)
          gps2ndSouthEdgepointArray[i] = sVGridNo[i];
      }
    }
    // Calculate the west edges
    if (gMapInformation.sWestGridNo != -1) {
      // 1st row
      sGridNo = gsTLGridNo;
      if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
          (!gubWorldRoomInfo[sGridNo] || gfBasement))
        sVGridNo[gus2ndWestEdgepointArraySize++] = sGridNo;
      while (sGridNo < gsBLGridNo) {
        sGridNo++;
        if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
            (!gubWorldRoomInfo[sGridNo] || gfBasement))
          sVGridNo[gus2ndWestEdgepointArraySize++] = sGridNo;
        sGridNo += 160;
        if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
            (!gubWorldRoomInfo[sGridNo] || gfBasement))
          sVGridNo[gus2ndWestEdgepointArraySize++] = sGridNo;
      }
      // 2nd row
      gus2ndWestEdgepointMiddleIndex = gus2ndWestEdgepointArraySize;
      sGridNo = gsTLGridNo - 159;
      if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
          (!gubWorldRoomInfo[sGridNo] || gfBasement))
        sVGridNo[gus2ndWestEdgepointArraySize++] = sGridNo;
      while (sGridNo < gsBLGridNo - 159) {
        sGridNo++;
        if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
            (!gubWorldRoomInfo[sGridNo] || gfBasement))
          sVGridNo[gus2ndWestEdgepointArraySize++] = sGridNo;
        sGridNo += 160;
        if (gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE &&
            (!gubWorldRoomInfo[sGridNo] || gfBasement))
          sVGridNo[gus2ndWestEdgepointArraySize++] = sGridNo;
      }
      if (gus2ndWestEdgepointArraySize) {  // Allocate and copy over the valid
                                           // gridnos.
        gps2ndWestEdgepointArray = MALLOCN(int16_t, gus2ndWestEdgepointArraySize);
        for (i = 0; i < gus2ndWestEdgepointArraySize; i++)
          gps2ndWestEdgepointArray[i] = sVGridNo[i];
      }
    }
  }

  // Eliminates any edgepoints not accessible to the edge of the world.  This is
  // done to the primary edgepoints
  ValidateEdgepoints();
  // Second step is to process the primary edgepoints and determine if any of
  // the edgepoints aren't accessible from the associated entrypoint.  These
  // edgepoints that are rejected are placed in the secondary list.
  if (gMapInformation.sIsolatedGridNo !=
      -1) {  // only if there is an isolated gridno in the map.  There is a flaw
             // in the design of this system.  The classification
    // process will automatically assign areas to be isolated if there is an
    // obstacle between one normal edgepoint and another causing a 5 tile
    // connection check to fail.  So, all maps with isolated edgepoints will need
    // to be checked manually to make sure there are no obstacles causing this to
    // happen (except for obstacles between normal areas and the isolated area)

    // Good thing most maps don't have isolated sections.  This is one expensive
    // function to call!  Maybe 200MI!
    ClassifyEdgepoints();
  }

  gfGeneratingMapEdgepoints = FALSE;
}

static void SaveMapEdgepoint(HWFILE const f, uint16_t const &n, uint16_t const &idx,
                             int16_t const *const array) {
  FileWrite(f, &n, sizeof(n));
  FileWrite(f, &idx, sizeof(idx));
  if (n != 0) FileWrite(f, array, sizeof(*array) * n);
}

void SaveMapEdgepoints(HWFILE const f) {
  // 1st priority edgepoints -- for common entry -- tactical placement gui uses
  // only these points.
  SaveMapEdgepoint(f, gus1stNorthEdgepointArraySize, gus1stNorthEdgepointMiddleIndex,
                   gps1stNorthEdgepointArray);
  SaveMapEdgepoint(f, gus1stEastEdgepointArraySize, gus1stEastEdgepointMiddleIndex,
                   gps1stEastEdgepointArray);
  SaveMapEdgepoint(f, gus1stSouthEdgepointArraySize, gus1stSouthEdgepointMiddleIndex,
                   gps1stSouthEdgepointArray);
  SaveMapEdgepoint(f, gus1stWestEdgepointArraySize, gus1stWestEdgepointMiddleIndex,
                   gps1stWestEdgepointArray);

  // 2nd priority edgepoints -- for isolated areas.  Okay to be zero
  SaveMapEdgepoint(f, gus2ndNorthEdgepointArraySize, gus2ndNorthEdgepointMiddleIndex,
                   gps2ndNorthEdgepointArray);
  SaveMapEdgepoint(f, gus2ndEastEdgepointArraySize, gus2ndEastEdgepointMiddleIndex,
                   gps2ndEastEdgepointArray);
  SaveMapEdgepoint(f, gus2ndSouthEdgepointArraySize, gus2ndSouthEdgepointMiddleIndex,
                   gps2ndSouthEdgepointArray);
  SaveMapEdgepoint(f, gus2ndWestEdgepointArraySize, gus2ndWestEdgepointMiddleIndex,
                   gps2ndWestEdgepointArray);
}

static void LoadMapEdgepoint(HWFILE const f, uint16_t &n, uint16_t &idx, int16_t *&array) {
  FileRead(f, &n, sizeof(n));
  FileRead(f, &idx, sizeof(idx));
  if (n != 0) {
    array = MALLOCN(int16_t, n);
    FileRead(f, array, sizeof(*array) * n);
  }
}

bool LoadMapEdgepoints(HWFILE const f) {
  TrashMapEdgepoints();

  LoadMapEdgepoint(f, gus1stNorthEdgepointArraySize, gus1stNorthEdgepointMiddleIndex,
                   gps1stNorthEdgepointArray);
  LoadMapEdgepoint(f, gus1stEastEdgepointArraySize, gus1stEastEdgepointMiddleIndex,
                   gps1stEastEdgepointArray);
  LoadMapEdgepoint(f, gus1stSouthEdgepointArraySize, gus1stSouthEdgepointMiddleIndex,
                   gps1stSouthEdgepointArray);
  LoadMapEdgepoint(f, gus1stWestEdgepointArraySize, gus1stWestEdgepointMiddleIndex,
                   gps1stWestEdgepointArray);

  if (gMapInformation.ubMapVersion < 17) { /* To prevent invalidation of older maps, which only used
                                            * one layer of edgepoints, and a uint8_t for containing
                                            * the size, we will preserve that paradigm, then kill
                                            * the loaded edgepoints and regenerate them. */
    TrashMapEdgepoints();
    return false;
  }

  LoadMapEdgepoint(f, gus2ndNorthEdgepointArraySize, gus2ndNorthEdgepointMiddleIndex,
                   gps2ndNorthEdgepointArray);
  LoadMapEdgepoint(f, gus2ndEastEdgepointArraySize, gus2ndEastEdgepointMiddleIndex,
                   gps2ndEastEdgepointArray);
  LoadMapEdgepoint(f, gus2ndSouthEdgepointArraySize, gus2ndSouthEdgepointMiddleIndex,
                   gps2ndSouthEdgepointArray);
  LoadMapEdgepoint(f, gus2ndWestEdgepointArraySize, gus2ndWestEdgepointMiddleIndex,
                   gps2ndWestEdgepointArray);

  if (gMapInformation.ubMapVersion < 22) {  // Regenerate them
    TrashMapEdgepoints();
    return false;
  }

  return true;
}

uint16_t ChooseMapEdgepoint(uint8_t ubStrategicInsertionCode) {
  int16_t *psArray = NULL;
  uint16_t usArraySize = 0;

  // First validate and get access to the correct array based on strategic
  // direction. We will use the selected array to choose insertion gridno's.
  switch (ubStrategicInsertionCode) {
    case INSERTION_CODE_NORTH:
      psArray = gps1stNorthEdgepointArray;
      usArraySize = gus1stNorthEdgepointArraySize;
      break;
    case INSERTION_CODE_EAST:
      psArray = gps1stEastEdgepointArray;
      usArraySize = gus1stEastEdgepointArraySize;
      break;
    case INSERTION_CODE_SOUTH:
      psArray = gps1stSouthEdgepointArray;
      usArraySize = gus1stSouthEdgepointArraySize;
      break;
    case INSERTION_CODE_WEST:
      psArray = gps1stWestEdgepointArray;
      usArraySize = gus1stWestEdgepointArraySize;
      break;
    default:
      AssertMsg(0,
                "ChooseMapEdgepoints:  Failed to pass a valid strategic "
                "insertion code.");
      break;
  }
  if (!usArraySize) {
    return NOWHERE;
  }
  return psArray[Random(usArraySize)];
}

void ChooseMapEdgepoints(MAPEDGEPOINTINFO *const pMapEdgepointInfo,
                         const uint8_t ubStrategicInsertionCode, uint8_t ubNumDesiredPoints) {
  AssertMsg(
      ubNumDesiredPoints > 0 && ubNumDesiredPoints <= 32,
      String("ChooseMapEdgepoints:  Desired points = %d, valid range is 1-32", ubNumDesiredPoints));

  /* First validate and get access to the correct array based on strategic
   * direction.  We will use the selected array to choose insertion gridno's. */
  int16_t *psArray;
  uint16_t usArraySize;
  switch (ubStrategicInsertionCode) {
    case INSERTION_CODE_NORTH:
      psArray = gps1stNorthEdgepointArray;
      usArraySize = gus1stNorthEdgepointArraySize;
      break;

    case INSERTION_CODE_EAST:
      psArray = gps1stEastEdgepointArray;
      usArraySize = gus1stEastEdgepointArraySize;
      break;

    case INSERTION_CODE_SOUTH:
      psArray = gps1stSouthEdgepointArray;
      usArraySize = gus1stSouthEdgepointArraySize;
      break;

    case INSERTION_CODE_WEST:
      psArray = gps1stWestEdgepointArray;
      usArraySize = gus1stWestEdgepointArraySize;
      break;

    default:
      AssertMsg(0,
                "ChooseMapEdgepoints:  Failed to pass a valid strategic "
                "insertion code.");
      psArray = NULL;
      usArraySize = 0;
      break;
  }
  pMapEdgepointInfo->ubStrategicInsertionCode = ubStrategicInsertionCode;

  if (usArraySize == 0) {
    pMapEdgepointInfo->ubNumPoints = 0;
    return;
  }

  /* JA2 Gold: don't place people in the water.  If any of the waypoints is on a
   * water spot, we're going to have to remove it */
  SGP::Buffer<int16_t> psTempArray(usArraySize);
  size_t n_usable = 0;
  for (int32_t i = 0; i < usArraySize; ++i) {
    const uint8_t terrain = GetTerrainType(psArray[i]);
    if (terrain == MED_WATER || terrain == DEEP_WATER) continue;

    psTempArray[n_usable++] = psArray[i];
  }

  if (ubNumDesiredPoints >=
      n_usable) {  // We don't have enough points for everyone, return them all.
    pMapEdgepointInfo->ubNumPoints = n_usable;
    for (int32_t i = 0; i < n_usable; ++i) {
      pMapEdgepointInfo->sGridNo[i] = psTempArray[i];
    }
    return;
  }

  // We have more points, so choose them randomly.
  uint16_t usSlots = n_usable;
  uint16_t usCurrSlot = 0;
  pMapEdgepointInfo->ubNumPoints = ubNumDesiredPoints;
  for (int32_t i = 0; i < n_usable; ++i) {
    if (Random(usSlots) < ubNumDesiredPoints) {
      pMapEdgepointInfo->sGridNo[usCurrSlot++] = psTempArray[i];
      --ubNumDesiredPoints;
    }
    --usSlots;
  }
}

int16_t *gpReservedGridNos = NULL;
int16_t gsReservedIndex = 0;

void BeginMapEdgepointSearch() {
  int16_t sGridNo;

  // Create the reserved list
  AssertMsg(!gpReservedGridNos,
            "Attempting to BeginMapEdgepointSearch that has already been created.");
  gpReservedGridNos = MALLOCN(int16_t, 20);
  gsReservedIndex = 0;

  if (gMapInformation.sNorthGridNo != -1)
    sGridNo = gMapInformation.sNorthGridNo;
  else if (gMapInformation.sEastGridNo != -1)
    sGridNo = gMapInformation.sEastGridNo;
  else if (gMapInformation.sSouthGridNo != -1)
    sGridNo = gMapInformation.sSouthGridNo;
  else if (gMapInformation.sWestGridNo != -1)
    sGridNo = gMapInformation.sWestGridNo;
  else
    return;

  GlobalReachableTest(sGridNo);

  // Now, we have the path values calculated.  Now, we can check for closest
  // edgepoints.
}

void EndMapEdgepointSearch() {
  AssertMsg(gpReservedGridNos,
            "Attempting to EndMapEdgepointSearch that has already been removed.");
  MemFree(gpReservedGridNos);
  gpReservedGridNos = NULL;
  gsReservedIndex = 0;
}

// THIS CODE ISN'T RECOMMENDED FOR TIME CRITICAL AREAS.
int16_t SearchForClosestPrimaryMapEdgepoint(int16_t sGridNo, uint8_t ubInsertionCode) {
  int32_t i, iDirectionLoop;
  int16_t *psArray = NULL;
  int16_t sRadius, sDistance, sDirection, sOriginalGridNo;
  uint16_t usArraySize = 0;
  BOOLEAN fReserved;

  if (gsReservedIndex >= 20) {  // Everything is reserved.
    AssertMsg(0,
              "All closest map edgepoints have been reserved.  We should "
              "only have 20 soldiers maximum...");
  }
  switch (ubInsertionCode) {
    case INSERTION_CODE_NORTH:
      psArray = gps1stNorthEdgepointArray;
      usArraySize = gus1stNorthEdgepointArraySize;
      AssertMsg(usArraySize != 0,
                String("Sector %c%d level %d doesn't have any north mapedgepoints. LC:1",
                       gWorldSectorY + 'A' - 1, gWorldSectorX, gbWorldSectorZ));
      break;
    case INSERTION_CODE_EAST:
      psArray = gps1stEastEdgepointArray;
      usArraySize = gus1stEastEdgepointArraySize;
      AssertMsg(usArraySize != 0,
                String("Sector %c%d level %d doesn't have any east mapedgepoints. LC:1",
                       gWorldSectorY + 'A' - 1, gWorldSectorX, gbWorldSectorZ));
      break;
    case INSERTION_CODE_SOUTH:
      psArray = gps1stSouthEdgepointArray;
      usArraySize = gus1stSouthEdgepointArraySize;
      AssertMsg(usArraySize != 0,
                String("Sector %c%d level %d doesn't have any south mapedgepoints. LC:1",
                       gWorldSectorY + 'A' - 1, gWorldSectorX, gbWorldSectorZ));
      break;
    case INSERTION_CODE_WEST:
      psArray = gps1stWestEdgepointArray;
      usArraySize = gus1stWestEdgepointArraySize;
      AssertMsg(usArraySize != 0,
                String("Sector %c%d level %d doesn't have any west mapedgepoints. LC:1",
                       gWorldSectorY + 'A' - 1, gWorldSectorX, gbWorldSectorZ));
      break;
  }
  if (!usArraySize) {
    return NOWHERE;
  }

  // Check the initial gridno, to see if it is available and an edgepoint.
  fReserved = FALSE;
  for (i = 0; i < gsReservedIndex; i++) {
    if (gpReservedGridNos[i] == sGridNo) {
      fReserved = TRUE;
      break;
    }
  }
  if (!fReserved) {  // Not reserved, so see if we can find this gridno in the
                     // edgepoint array.
    for (i = 0; i < usArraySize; i++) {
      if (psArray[i] == sGridNo) {  // Yes, the gridno is in the edgepoint array.
        gpReservedGridNos[gsReservedIndex] = sGridNo;
        gsReservedIndex++;
        return sGridNo;
      }
    }
  }

  // spiral outwards, until we find an unreserved mapedgepoint.
  //
  // 09 08 07 06
  // 10	01 00 05
  // 11 02 03 04
  // 12 13 14 15 ..
  sRadius = 1;
  sDirection = WORLD_COLS;
  sOriginalGridNo = sGridNo;
  while (sRadius < (int16_t)(gbWorldSectorZ ? 30 : 10)) {
    sGridNo = sOriginalGridNo + (-1 - WORLD_COLS) * sRadius;  // start at the TOP-LEFT gridno
    for (iDirectionLoop = 0; iDirectionLoop < 4; iDirectionLoop++) {
      switch (iDirectionLoop) {
        case 0:
          sDirection = WORLD_COLS;
          break;
        case 1:
          sDirection = 1;
          break;
        case 2:
          sDirection = -WORLD_COLS;
          break;
        case 3:
          sDirection = -1;
          break;
      }
      sDistance = sRadius * 2;
      while (sDistance--) {
        sGridNo += sDirection;
        if (sGridNo < 0 || sGridNo >= WORLD_MAX) continue;
        // Check the gridno, to see if it is available and an edgepoint.
        fReserved = FALSE;
        for (i = 0; i < gsReservedIndex; i++) {
          if (gpReservedGridNos[i] == sGridNo) {
            fReserved = TRUE;
            break;
          }
        }
        if (!fReserved) {  // Not reserved, so see if we can find this gridno in
                           // the edgepoint array.
          for (i = 0; i < usArraySize; i++) {
            if (psArray[i] == sGridNo) {  // Yes, the gridno is in the edgepoint array.
              gpReservedGridNos[gsReservedIndex] = sGridNo;
              gsReservedIndex++;
              return sGridNo;
            }
          }
        }
      }
    }
    sRadius++;
  }
  return NOWHERE;
}

int16_t SearchForClosestSecondaryMapEdgepoint(int16_t sGridNo, uint8_t ubInsertionCode) {
  int32_t i, iDirectionLoop;
  int16_t *psArray = NULL;
  int16_t sRadius, sDistance, sDirection, sOriginalGridNo;
  uint16_t usArraySize = 0;
  BOOLEAN fReserved;

  if (gsReservedIndex >= 20) {  // Everything is reserved.
    AssertMsg(0,
              "All closest map edgepoints have been reserved.  We should "
              "only have 20 soldiers maximum...");
  }
  switch (ubInsertionCode) {
    case INSERTION_CODE_NORTH:
      psArray = gps2ndNorthEdgepointArray;
      usArraySize = gus2ndNorthEdgepointArraySize;
      AssertMsg(usArraySize != 0, String("Sector %c%d level %d doesn't have any isolated north "
                                         "mapedgepoints. KM:1",
                                         gWorldSectorY + 'A' - 1, gWorldSectorX, gbWorldSectorZ));
      break;
    case INSERTION_CODE_EAST:
      psArray = gps2ndEastEdgepointArray;
      usArraySize = gus2ndEastEdgepointArraySize;
      AssertMsg(usArraySize != 0, String("Sector %c%d level %d doesn't have any isolated east "
                                         "mapedgepoints. KM:1",
                                         gWorldSectorY + 'A' - 1, gWorldSectorX, gbWorldSectorZ));
      break;
    case INSERTION_CODE_SOUTH:
      psArray = gps2ndSouthEdgepointArray;
      usArraySize = gus2ndSouthEdgepointArraySize;
      AssertMsg(usArraySize != 0, String("Sector %c%d level %d doesn't have any isolated south "
                                         "mapedgepoints. KM:1",
                                         gWorldSectorY + 'A' - 1, gWorldSectorX, gbWorldSectorZ));
      break;
    case INSERTION_CODE_WEST:
      psArray = gps2ndWestEdgepointArray;
      usArraySize = gus2ndWestEdgepointArraySize;
      AssertMsg(usArraySize != 0, String("Sector %c%d level %d doesn't have any isolated west "
                                         "mapedgepoints. KM:1",
                                         gWorldSectorY + 'A' - 1, gWorldSectorX, gbWorldSectorZ));
      break;
  }
  if (!usArraySize) {
    return NOWHERE;
  }

  // Check the initial gridno, to see if it is available and an edgepoint.
  fReserved = FALSE;
  for (i = 0; i < gsReservedIndex; i++) {
    if (gpReservedGridNos[i] == sGridNo) {
      fReserved = TRUE;
      break;
    }
  }
  if (!fReserved) {  // Not reserved, so see if we can find this gridno in the
                     // edgepoint array.
    for (i = 0; i < usArraySize; i++) {
      if (psArray[i] == sGridNo) {  // Yes, the gridno is in the edgepoint array.
        gpReservedGridNos[gsReservedIndex] = sGridNo;
        gsReservedIndex++;
        return sGridNo;
      }
    }
  }

  // spiral outwards, until we find an unreserved mapedgepoint.
  //
  // 09 08 07 06
  // 10	01 00 05
  // 11 02 03 04
  // 12 13 14 15 ..
  sRadius = 1;
  sDirection = WORLD_COLS;
  sOriginalGridNo = sGridNo;
  while (sRadius < (int16_t)(gbWorldSectorZ ? 30 : 10)) {
    sGridNo = sOriginalGridNo + (-1 - WORLD_COLS) * sRadius;  // start at the TOP-LEFT gridno
    for (iDirectionLoop = 0; iDirectionLoop < 4; iDirectionLoop++) {
      switch (iDirectionLoop) {
        case 0:
          sDirection = WORLD_COLS;
          break;
        case 1:
          sDirection = 1;
          break;
        case 2:
          sDirection = -WORLD_COLS;
          break;
        case 3:
          sDirection = -1;
          break;
      }
      sDistance = sRadius * 2;
      while (sDistance--) {
        sGridNo += sDirection;
        if (sGridNo < 0 || sGridNo >= WORLD_MAX) continue;
        // Check the gridno, to see if it is available and an edgepoint.
        fReserved = FALSE;
        for (i = 0; i < gsReservedIndex; i++) {
          if (gpReservedGridNos[i] == sGridNo) {
            fReserved = TRUE;
            break;
          }
        }
        if (!fReserved) {  // Not reserved, so see if we can find this gridno in
                           // the edgepoint array.
          for (i = 0; i < usArraySize; i++) {
            if (psArray[i] == sGridNo) {  // Yes, the gridno is in the edgepoint array.
              gpReservedGridNos[gsReservedIndex] = sGridNo;
              gsReservedIndex++;
              return sGridNo;
            }
          }
        }
      }
    }
    sRadius++;
  }
  return NOWHERE;
}

#define EDGE_OF_MAP_SEARCH 5

static BOOLEAN VerifyEdgepoint(SOLDIERTYPE *pSoldier, int16_t sEdgepoint) {
  int32_t iSearchRange;
  int16_t sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;
  int16_t sGridNo;
  int8_t bDirection;

  pSoldier->sGridNo = sEdgepoint;

  iSearchRange = EDGE_OF_MAP_SEARCH;

  // determine maximum horizontal limits
  sMaxLeft = std::min(iSearchRange, (pSoldier->sGridNo % MAXCOL));
  // NumMessage("sMaxLeft = ",sMaxLeft);
  sMaxRight = std::min(iSearchRange, MAXCOL - ((pSoldier->sGridNo % MAXCOL) + 1));
  // NumMessage("sMaxRight = ",sMaxRight);

  // determine maximum vertical limits
  sMaxUp = std::min(iSearchRange, (pSoldier->sGridNo / MAXROW));
  // NumMessage("sMaxUp = ",sMaxUp);
  sMaxDown = std::min(iSearchRange, MAXROW - ((pSoldier->sGridNo / MAXROW) + 1));

  // Call FindBestPath to set flags in all locations that we can
  // walk into within range.  We have to set some things up first...

  // set the distance limit of the square region
  gubNPCDistLimit = EDGE_OF_MAP_SEARCH;

  // reset the "reachable" flags in the region we're looking at
  for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++) {
    for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++) {
      sGridNo = sEdgepoint + sXOffset + (MAXCOL * sYOffset);
      gpWorldLevelData[sGridNo].uiFlags &= ~(MAPELEMENT_REACHABLE);
    }
  }

  FindBestPath(pSoldier, NOWHERE, pSoldier->bLevel, WALKING, COPYREACHABLE, PATH_THROUGH_PEOPLE);

  // Turn off the "reachable" flag for the current location
  // so we don't consider it
  // gpWorldLevelData[sEdgepoint].uiFlags &= ~(MAPELEMENT_REACHABLE);

  // SET UP double-LOOP TO STEP THROUGH POTENTIAL GRID #s
  for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++) {
    for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++) {
      // calculate the next potential gridno
      sGridNo = sEdgepoint + sXOffset + (MAXCOL * sYOffset);

      if (!(gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE)) {
        continue;
      }

      if (GridNoOnEdgeOfMap(sGridNo, &bDirection)) {
        // ok!
        return TRUE;
      }
    }
  }

  // no spots right on edge of map within 5 tiles
  return FALSE;
}

static BOOLEAN EdgepointsClose(SOLDIERTYPE *pSoldier, int16_t sEdgepoint1, int16_t sEdgepoint2) {
  int32_t iSearchRange;
  int16_t sMaxLeft, sMaxRight, sMaxUp, sMaxDown, sXOffset, sYOffset;
  int16_t sGridNo;

  pSoldier->sGridNo = sEdgepoint1;

  if (gWorldSectorX == 14 && gWorldSectorY == 9 &&
      !gbWorldSectorZ) {  // BRUTAL CODE  -- special case map.
    iSearchRange = 250;
  } else {
    iSearchRange = EDGE_OF_MAP_SEARCH;
  }

  // determine maximum horizontal limits
  sMaxLeft = std::min(iSearchRange, (pSoldier->sGridNo % MAXCOL));
  // NumMessage("sMaxLeft = ",sMaxLeft);
  sMaxRight = std::min(iSearchRange, MAXCOL - ((pSoldier->sGridNo % MAXCOL) + 1));
  // NumMessage("sMaxRight = ",sMaxRight);

  // determine maximum vertical limits
  sMaxUp = std::min(iSearchRange, (pSoldier->sGridNo / MAXROW));
  // NumMessage("sMaxUp = ",sMaxUp);
  sMaxDown = std::min(iSearchRange, MAXROW - ((pSoldier->sGridNo / MAXROW) + 1));

  // Call FindBestPath to set flags in all locations that we can
  // walk into within range.  We have to set some things up first...

  // set the distance limit of the square region
  gubNPCDistLimit = (uint8_t)iSearchRange;

  // reset the "reachable" flags in the region we're looking at
  for (sYOffset = -sMaxUp; sYOffset <= sMaxDown; sYOffset++) {
    for (sXOffset = -sMaxLeft; sXOffset <= sMaxRight; sXOffset++) {
      sGridNo = sEdgepoint1 + sXOffset + (MAXCOL * sYOffset);
      gpWorldLevelData[sGridNo].uiFlags &= ~(MAPELEMENT_REACHABLE);
    }
  }

  if (FindBestPath(pSoldier, sEdgepoint2, pSoldier->bLevel, WALKING, COPYREACHABLE,
                   PATH_THROUGH_PEOPLE)) {
    return TRUE;
  }
  return FALSE;
}

uint8_t CalcMapEdgepointClassInsertionCode(int16_t sGridNo) {
  SOLDIERTYPE Soldier;
  int32_t iLoop;
  int16_t *psEdgepointArray1, *psEdgepointArray2;
  int32_t iEdgepointArraySize1, iEdgepointArraySize2;
  int16_t sClosestSpot1 = NOWHERE, sClosestDist1 = 0x7FFF, sTempDist;
  int16_t sClosestSpot2 = NOWHERE, sClosestDist2 = 0x7FFF;
  BOOLEAN fPrimaryValid = FALSE, fSecondaryValid = FALSE;

  memset(&Soldier, 0, sizeof(SOLDIERTYPE));
  Soldier.bTeam = 1;
  Soldier.sGridNo = sGridNo;

  if (gMapInformation.sIsolatedGridNo ==
      -1) {  // If the map has no isolated area, then all edgepoints are primary.
    return INSERTION_CODE_PRIMARY_EDGEINDEX;
  }

  switch (gubTacticalDirection) {
    case NORTH:
      psEdgepointArray1 = gps1stNorthEdgepointArray;
      iEdgepointArraySize1 = gus1stNorthEdgepointArraySize;
      psEdgepointArray2 = gps2ndNorthEdgepointArray;
      iEdgepointArraySize2 = gus2ndNorthEdgepointArraySize;
      break;
    case EAST:
      psEdgepointArray1 = gps1stEastEdgepointArray;
      iEdgepointArraySize1 = gus1stEastEdgepointArraySize;
      psEdgepointArray2 = gps2ndEastEdgepointArray;
      iEdgepointArraySize2 = gus2ndEastEdgepointArraySize;
      break;
    case SOUTH:
      psEdgepointArray1 = gps1stSouthEdgepointArray;
      iEdgepointArraySize1 = gus1stSouthEdgepointArraySize;
      psEdgepointArray2 = gps2ndSouthEdgepointArray;
      iEdgepointArraySize2 = gus2ndSouthEdgepointArraySize;
      break;
    case WEST:
      psEdgepointArray1 = gps1stWestEdgepointArray;
      iEdgepointArraySize1 = gus1stWestEdgepointArraySize;
      psEdgepointArray2 = gps2ndWestEdgepointArray;
      iEdgepointArraySize2 = gus2ndWestEdgepointArraySize;
      break;
    default:
      // WTF???
      return INSERTION_CODE_PRIMARY_EDGEINDEX;
  }

  // Do a 2D search to find the closest map edgepoint and
  // try to create a path there
  for (iLoop = 0; iLoop < iEdgepointArraySize1; iLoop++) {
    sTempDist = PythSpacesAway(sGridNo, psEdgepointArray1[iLoop]);
    if (sTempDist < sClosestDist1) {
      sClosestDist1 = sTempDist;
      sClosestSpot1 = psEdgepointArray1[iLoop];
    }
  }
  for (iLoop = 0; iLoop < iEdgepointArraySize2; iLoop++) {
    sTempDist = PythSpacesAway(sGridNo, psEdgepointArray2[iLoop]);
    if (sTempDist < sClosestDist2) {
      sClosestDist2 = sTempDist;
      sClosestSpot2 = psEdgepointArray2[iLoop];
    }
  }

  // set the distance limit of the square region
  gubNPCDistLimit = 15;

  if (!sClosestDist1 ||
      FindBestPath(&Soldier, sClosestSpot1, 0, WALKING, NO_COPYROUTE, PATH_THROUGH_PEOPLE)) {
    fPrimaryValid = TRUE;
  }
  if (!sClosestDist2 ||
      FindBestPath(&Soldier, sClosestSpot2, 0, WALKING, NO_COPYROUTE, PATH_THROUGH_PEOPLE)) {
    fSecondaryValid = TRUE;
  }

  if (fPrimaryValid == fSecondaryValid) {
    if (sClosestDist2 < sClosestDist1) {
      return INSERTION_CODE_SECONDARY_EDGEINDEX;
    }
    return INSERTION_CODE_PRIMARY_EDGEINDEX;
  }
  if (fPrimaryValid) {
    return INSERTION_CODE_PRIMARY_EDGEINDEX;
  }
  return INSERTION_CODE_SECONDARY_EDGEINDEX;
}

static bool ShowMapEdgepoint(uint16_t const n, int16_t const *const array, uint16_t const idx) {
  int32_t n_illegal = 0;
  int16_t const *const end = array + n;
  for (int16_t const *i = array; i != end; ++i) {
    if (*i != -1) {
      AddTopmostToTail(*i, idx);
    } else {
      ++n_illegal;
    }
  }
  return n_illegal;
}

void ShowMapEdgepoints() {
  int32_t n_illegal1 = 0;
  n_illegal1 +=
      ShowMapEdgepoint(gus1stNorthEdgepointArraySize, gps1stNorthEdgepointArray, FIRSTPOINTERS5);
  n_illegal1 +=
      ShowMapEdgepoint(gus1stEastEdgepointArraySize, gps1stEastEdgepointArray, FIRSTPOINTERS5);
  n_illegal1 +=
      ShowMapEdgepoint(gus1stSouthEdgepointArraySize, gps1stSouthEdgepointArray, FIRSTPOINTERS5);
  n_illegal1 +=
      ShowMapEdgepoint(gus1stWestEdgepointArraySize, gps1stWestEdgepointArray, FIRSTPOINTERS5);

  int32_t n_illegal2 = 0;
  n_illegal2 +=
      ShowMapEdgepoint(gus2ndNorthEdgepointArraySize, gps2ndNorthEdgepointArray, FIRSTPOINTERS6);
  n_illegal2 +=
      ShowMapEdgepoint(gus2ndEastEdgepointArraySize, gps2ndEastEdgepointArray, FIRSTPOINTERS6);
  n_illegal2 +=
      ShowMapEdgepoint(gus2ndSouthEdgepointArraySize, gps2ndSouthEdgepointArray, FIRSTPOINTERS6);
  n_illegal2 +=
      ShowMapEdgepoint(gus2ndWestEdgepointArraySize, gps2ndWestEdgepointArray, FIRSTPOINTERS6);

  if (n_illegal1 == 0 && n_illegal2 == 0) {
    ScreenMsg(0, MSG_TESTVERSION, L"Showing display of map edgepoints");
  } else {
    ScreenMsg(0, MSG_TESTVERSION,
              L"Showing display of map edgepoints (%d illegal primary, %d "
              L"illegal secondary)",
              n_illegal1, n_illegal2);
  }
  ScreenMsg(0, MSG_TESTVERSION, L"N:%d:%d E:%d:%d S:%d:%d W:%d:%d", gus1stNorthEdgepointArraySize,
            gus2ndNorthEdgepointArraySize, gus1stEastEdgepointArraySize,
            gus2ndEastEdgepointArraySize, gus1stSouthEdgepointArraySize,
            gus2ndSouthEdgepointArraySize, gus1stWestEdgepointArraySize,
            gus2ndWestEdgepointArraySize);
}

static void HideMapEdgepoint(uint16_t const n, int16_t const *const array) {
  int16_t const *const end = array + n;
  for (int16_t const *i = array; i != end; ++i) {
    if (*i == -1) continue;
    RemoveAllTopmostsOfTypeRange(*i, FIRSTPOINTERS, FIRSTPOINTERS);
  }
}

void HideMapEdgepoints() {
  ScreenMsg(0, MSG_TESTVERSION, L"Removing display of map edgepoints");

  HideMapEdgepoint(gus1stNorthEdgepointArraySize, gps1stNorthEdgepointArray);
  HideMapEdgepoint(gus1stEastEdgepointArraySize, gps1stEastEdgepointArray);
  HideMapEdgepoint(gus1stSouthEdgepointArraySize, gps1stSouthEdgepointArray);
  HideMapEdgepoint(gus1stWestEdgepointArraySize, gps1stWestEdgepointArray);

  HideMapEdgepoint(gus2ndNorthEdgepointArraySize, gps2ndNorthEdgepointArray);
  HideMapEdgepoint(gus2ndEastEdgepointArraySize, gps2ndEastEdgepointArray);
  HideMapEdgepoint(gus2ndSouthEdgepointArraySize, gps2ndSouthEdgepointArray);
  HideMapEdgepoint(gus2ndWestEdgepointArraySize, gps2ndWestEdgepointArray);
}
