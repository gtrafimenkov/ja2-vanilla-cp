#ifndef __STRATEGIC_H
#define __STRATEGIC_H
// header for strategic structure

#include "JA2Types.h"

enum InsertionCode {
  INSERTION_CODE_NORTH,
  INSERTION_CODE_SOUTH,
  INSERTION_CODE_EAST,
  INSERTION_CODE_WEST,
  INSERTION_CODE_GRIDNO,
  INSERTION_CODE_ARRIVING_GAME,
  INSERTION_CODE_CHOPPER,
  INSERTION_CODE_PRIMARY_EDGEINDEX,
  INSERTION_CODE_SECONDARY_EDGEINDEX,
  INSERTION_CODE_CENTER,
};

void HandleStrategicDeath(SOLDIERTYPE &);

#endif
