// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

/*
        Filename        :       pathai.h
        Author          :       Ray E. Bornert II
        Date            :       1992-MAR-15

        Copyright (C) 1993 HixoxiH Software
*/

#ifndef _PATHAI_H
#define _PATHAI_H

#include "JA2Types.h"

void InitPathAI();
void ShutDownPathAI();
int16_t PlotPath(SOLDIERTYPE *pSold, int16_t sDestGridno, int8_t bCopyRoute, int8_t bPlot,
                 uint16_t usMovementMode, int16_t sAPBudget);
int16_t UIPlotPath(SOLDIERTYPE *pSold, int16_t sDestGridno, int8_t bCopyRoute, int8_t bPlot,
                   uint16_t usMovementMode, int16_t sAPBudget);
int16_t EstimatePlotPath(SOLDIERTYPE *pSold, int16_t sDestGridno, int8_t bCopyRoute, int8_t bPlot,
                         uint16_t usMovementMode, int16_t sAPBudget);

void ErasePath();
int32_t FindBestPath(SOLDIERTYPE *s, int16_t sDestination, int8_t ubLevel, int16_t usMovementMode,
                     int8_t bCopy, uint8_t fFlags);
void GlobalReachableTest(int16_t sStartGridNo);
void GlobalItemsReachableTest(int16_t sStartGridNo1, int16_t sStartGridNo2);
void RoofReachableTest(int16_t sStartGridNo, uint8_t ubBuildingID);
void LocalReachableTest(int16_t sStartGridNo, int8_t bRadius);

uint8_t DoorTravelCost(const SOLDIERTYPE *pSoldier, int32_t iGridNo, uint8_t ubMovementCost,
                       BOOLEAN fReturnPerceivedValue, int32_t *piDoorGridNo);
uint8_t InternalDoorTravelCost(const SOLDIERTYPE *pSoldier, int32_t iGridNo, uint8_t ubMovementCost,
                               BOOLEAN fReturnPerceivedValue, int32_t *piDoorGridNo,
                               BOOLEAN fReturnDoorCost);

int16_t RecalculatePathCost(SOLDIERTYPE *pSoldier, uint16_t usMovementMode);

// Exporting these global variables
extern uint32_t guiPathingData[256];
extern uint8_t gubNPCAPBudget;
extern uint8_t gubNPCDistLimit;
extern uint8_t gubNPCPathCount;
extern BOOLEAN gfPlotPathToExitGrid;
extern BOOLEAN gfNPCCircularDistLimit;
extern BOOLEAN gfEstimatePath;
extern BOOLEAN gfPathAroundObstacles;
extern uint8_t gubGlobalPathFlags;

// Ian's terrain values for travelling speed/pathing purposes
// Fixed by CJC March 4, 1998.  Please do not change these unless familiar
// with how this will affect the path code!

#define TRAVELCOST_NONE 0
#define TRAVELCOST_FLAT 10
#define TRAVELCOST_BUMPY 12
#define TRAVELCOST_GRASS 12
#define TRAVELCOST_THICK 16
#define TRAVELCOST_DEBRIS 20
#define TRAVELCOST_SHORE 30
#define TRAVELCOST_KNEEDEEP 36
#define TRAVELCOST_DEEPWATER 50
#define TRAVELCOST_FENCE 40

// these values are used to indicate "this is an obstacle
// if there is a door (perceived) open/closed in this tile
#define TRAVELCOST_DOOR_CLOSED_HERE 220
#define TRAVELCOST_DOOR_CLOSED_N 221
#define TRAVELCOST_DOOR_CLOSED_W 222
#define TRAVELCOST_DOOR_OPEN_HERE 223
#define TRAVELCOST_DOOR_OPEN_N 224
#define TRAVELCOST_DOOR_OPEN_NE 225
#define TRAVELCOST_DOOR_OPEN_E 226
#define TRAVELCOST_DOOR_OPEN_SE 227
#define TRAVELCOST_DOOR_OPEN_S 228
#define TRAVELCOST_DOOR_OPEN_SW 229
#define TRAVELCOST_DOOR_OPEN_W 230
#define TRAVELCOST_DOOR_OPEN_NW 231
#define TRAVELCOST_DOOR_OPEN_N_N 232
#define TRAVELCOST_DOOR_OPEN_NW_N 233
#define TRAVELCOST_DOOR_OPEN_NE_N 234
#define TRAVELCOST_DOOR_OPEN_W_W 235
#define TRAVELCOST_DOOR_OPEN_SW_W 236
#define TRAVELCOST_DOOR_OPEN_NW_W 237
#define TRAVELCOST_NOT_STANDING 248
#define TRAVELCOST_OFF_MAP 249
#define TRAVELCOST_CAVEWALL 250
#define TRAVELCOST_HIDDENOBSTACLE 251
#define TRAVELCOST_DOOR 252
#define TRAVELCOST_OBSTACLE 253
#define TRAVELCOST_WALL 254
#define TRAVELCOST_EXITGRID 255

#define TRAVELCOST_TRAINTRACKS 30
#define TRAVELCOST_DIRTROAD 9
#define TRAVELCOST_PAVEDROAD 9
#define TRAVELCOST_FLATFLOOR 10

#define TRAVELCOST_BLOCKED (TRAVELCOST_OFF_MAP)
#define IS_TRAVELCOST_DOOR(x) (x >= TRAVELCOST_DOOR_CLOSED_HERE && x <= TRAVELCOST_DOOR_OPEN_NW_W)
#define IS_TRAVELCOST_CLOSED_DOOR(x) \
  (x >= TRAVELCOST_DOOR_CLOSED_HERE && x << TRAVELCOST_DOOR_CLOSED_W)

// ------------------------------------------
// PLOT PATH defines
#define NO_PLOT 0
#define PLOT 1

#define NO_COPYROUTE 0
#define COPYROUTE 1
#define COPYREACHABLE 2
#define COPYREACHABLE_AND_APS 3

#define PATH_THROUGH_PEOPLE 0x01
#define PATH_IGNORE_PERSON_AT_DEST 0x02
#define PATH_CLOSE_GOOD_ENOUGH 0x04

#define PATH_CLOSE_RADIUS 5

// ------------------------------------------

#endif
