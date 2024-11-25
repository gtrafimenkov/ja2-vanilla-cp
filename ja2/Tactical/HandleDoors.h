// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef _DOORS_H
#define _DOORS_H

#include "JA2Types.h"

enum HandleDoor {
  HANDLE_DOOR_OPEN = 1,
  HANDLE_DOOR_EXAMINE = 2,
  HANDLE_DOOR_LOCKPICK = 3,
  HANDLE_DOOR_FORCE = 4,
  HANDLE_DOOR_LOCK = 5,
  HANDLE_DOOR_UNLOCK = 6,
  HANDLE_DOOR_EXPLODE = 7,
  HANDLE_DOOR_UNTRAP = 8,
  HANDLE_DOOR_CROWBAR = 9
};

BOOLEAN HandleOpenableStruct(SOLDIERTYPE *pSoldier, int16_t sGridNo, STRUCTURE *pStructure);

void InteractWithOpenableStruct(SOLDIERTYPE &, STRUCTURE &, uint8_t direction);

void InteractWithClosedDoor(SOLDIERTYPE *, HandleDoor);

void SetDoorString(int16_t sGridNo);

void HandleDoorChangeFromGridNo(SOLDIERTYPE *pSoldier, int16_t sGridNo, BOOLEAN fNoAnimations);

uint16_t GetAnimStateForInteraction(SOLDIERTYPE const &, BOOLEAN door, uint16_t anim_state);

#endif
