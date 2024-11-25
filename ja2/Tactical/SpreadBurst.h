// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef _SPREAD_BURST_H
#define _SPREAD_BURST_H

#include "JA2Types.h"

void ResetBurstLocations();
void AccumulateBurstLocation(int16_t sGridNo);
void PickBurstLocations(SOLDIERTYPE *pSoldier);
void AIPickBurstLocations(SOLDIERTYPE *pSoldier, int8_t bTargets, SOLDIERTYPE *pTargets[5]);

void RenderAccumulatedBurstLocations();

void LoadSpreadBurstGraphics();
void DeleteSpreadBurstGraphics();

#endif
