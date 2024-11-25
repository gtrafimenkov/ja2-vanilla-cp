// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

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
