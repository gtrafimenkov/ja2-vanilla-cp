// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __STRATPATH_H
#define __STRATPATH_H

#include "Strategic/MapScreenInterfaceMap.h"
#include "Strategic/StrategicMovement.h"

// directions of movement for badsector determination ( blocking off of a sector
// exit from foot or vehicle travel)

// Shortest Path Defines
#define NORTH_MOVE -18
#define EAST_MOVE 1
#define WEST_MOVE -1
#define SOUTH_MOVE 18

// Movement speed defines
#define NORMAL_MVT 1
#define SLOW_MVT 0

// movment modes
enum {
  MVT_MODE_AIR,
  MVT_MODE_VEHICLE,
  MVT_MODE_FOOT,
};

int32_t FindStratPath(int16_t sStart, int16_t sDestination, GROUP const &,
                      BOOLEAN fTacticalTraversal);

// build a stategic path
PathSt *BuildAStrategicPath(int16_t iStartSectorNum, int16_t iEndSectorNum, GROUP const &,
                            BOOLEAN fTacticalTraversal);

// append onto path list
PathSt *AppendStrategicPath(PathSt *pNewSection, PathSt *pHeadOfPathList);

// clear out strategic path list
PathSt *ClearStrategicPathList(PathSt *pHeadOfPath, int16_t sMvtGroup);

// remove head of list
PathSt *RemoveHeadFromStrategicPath(PathSt *pList);

// clear out path list after/including this sector sX, sY..will start at end of
// path and work it's way back till sector is found...removes most recent
// sectors first
PathSt *ClearStrategicPathListAfterThisSector(PathSt *pHeadOfPath, int16_t sX, int16_t sY,
                                              int16_t sMvtGroup);

// get id of last sector in mercs path list
int16_t GetLastSectorIdInCharactersPath(const SOLDIERTYPE *pCharacter);

// copy paths
PathSt *CopyPaths(PathSt *src);

// rebuild way points for strategic mapscreen path changes
void RebuildWayPointsForGroupPath(PathSt *pHeadOfPath, GROUP &);

// clear strategic movement (mercpaths and waypoints) for this soldier, and his
// group (including its vehicles)
void ClearMvtForThisSoldierAndGang(SOLDIERTYPE *pSoldier);

// start movement of this group to this sector...not to be used by the player
// merc groups.
BOOLEAN MoveGroupFromSectorToSector(GROUP &, int16_t sStartX, int16_t sStartY, int16_t sDestX,
                                    int16_t sDestY);

BOOLEAN MoveGroupFromSectorToSectorButAvoidPlayerInfluencedSectors(GROUP &, int16_t sStartX,
                                                                   int16_t sStartY, int16_t sDestX,
                                                                   int16_t sDestY);
BOOLEAN
MoveGroupFromSectorToSectorButAvoidPlayerInfluencedSectorsAndStopOneSectorBeforeEnd(
    GROUP &, int16_t sStartX, int16_t sStartY, int16_t sDestX, int16_t sDestY);

// get length of path
int32_t GetLengthOfPath(PathSt *pHeadPath);
int32_t GetLengthOfMercPath(const SOLDIERTYPE *pSoldier);

PathSt *GetSoldierMercPathPtr(SOLDIERTYPE const *);
PathSt *GetGroupMercPathPtr(GROUP const &);

GROUP *GetSoldierGroup(SOLDIERTYPE const &);

// clears this groups strategic movement (mercpaths and waypoints), include
// those in the vehicle structs(!)
void ClearMercPathsAndWaypointsForAllInGroup(GROUP &);

void AddSectorToFrontOfMercPathForAllSoldiersInGroup(GROUP *pGroup, uint8_t ubSectorX,
                                                     uint8_t ubSectorY);

#endif
