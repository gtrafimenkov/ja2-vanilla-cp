#ifndef STRATEGIC_MOVEMENT_COSTS_H
#define STRATEGIC_MOVEMENT_COSTS_H

#include "SGP/Types.h"

void InitStrategicMovementCosts();

uint8_t GetTraversability(int16_t sStartSector, int16_t sEndSector);

bool SectorIsPassable(int16_t sSector);

#endif
