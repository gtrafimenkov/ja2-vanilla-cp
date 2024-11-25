// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef BUILDING_H

#define BUILDING_H

#include "TileEngine/WorldDef.h"
// for what it's worth, 2 bytes, we use roof climb spots as 1-based
// so the 0th entry is always 0 and can be compared with (and not equal)
// NOWHERE or any other location
#define MAX_CLIMBSPOTS_PER_BUILDING 21

// similarly for buildings, only we really want 0 to be invalid index
#define NO_BUILDING 0
#define MAX_BUILDINGS 31

struct BUILDING {
  int16_t sUpClimbSpots[MAX_CLIMBSPOTS_PER_BUILDING];
  int16_t sDownClimbSpots[MAX_CLIMBSPOTS_PER_BUILDING];
  uint8_t ubNumClimbSpots;
};

extern uint8_t gubBuildingInfo[WORLD_MAX];

BUILDING *FindBuilding(int16_t sGridNo);
void GenerateBuildings();
int16_t FindClosestClimbPoint(int16_t sStartGridNo, int16_t sDesiredGridNo, BOOLEAN fClimbUp);
BOOLEAN SameBuilding(int16_t sGridNo1, int16_t sGridNo2);

#endif
