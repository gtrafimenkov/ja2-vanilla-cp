// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef STRATEGIC_MOVEMENT_COSTS_H
#define STRATEGIC_MOVEMENT_COSTS_H

#include "SGP/Types.h"

void InitStrategicMovementCosts();

uint8_t GetTraversability(int16_t sStartSector, int16_t sEndSector);

bool SectorIsPassable(int16_t sSector);

#endif
