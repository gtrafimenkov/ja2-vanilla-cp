#ifndef TURN_BASED_INPUT_H
#define TURN_BASED_INPUT_H

#include "JA2Types.h"

extern const SOLDIERTYPE *gUITargetSoldier;

BOOLEAN ConfirmActionCancel(uint16_t usMapPos, uint16_t usOldMapPos);
int8_t HandleMoveModeInteractiveClick(uint16_t usMapPos);
BOOLEAN HandleUIReloading(SOLDIERTYPE *pSoldier);

#endif
