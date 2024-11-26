#ifndef __GAME_INIT_H
#define __GAME_INIT_H

#include "SGP/Types.h"

void InitNewGame();
BOOLEAN AnyMercsHired();

void InitStrategicLayer();
void ShutdownStrategicLayer();

void ReStartingGame();

void InitBloodCatSectors();

#endif
