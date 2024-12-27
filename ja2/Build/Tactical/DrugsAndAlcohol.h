// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __DRUGS_AND_ALCOHOL_H
#define __DRUGS_AND_ALCOHOL_H

#define DRUG_TYPE_ADRENALINE 0
#define DRUG_TYPE_ALCOHOL 1
#define NO_DRUG 2
#define NUM_COMPLEX_DRUGS 2
#define DRUG_TYPE_REGENERATION 3

#define SOBER 0
#define FEELING_GOOD 1
#define BORDERLINE 2
#define DRUNK 3
#define HUNGOVER 4

#define REGEN_POINTS_PER_BOOSTER 4
#define LIFE_GAIN_PER_REGEN_POINT 10

#include "JA2Types.h"
#include "SGP/Types.h"

BOOLEAN ApplyDrugs(SOLDIERTYPE *pSoldier, OBJECTTYPE *pObject);

void HandleEndTurnDrugAdjustments(SOLDIERTYPE *pSoldier);
void HandleAPEffectDueToDrugs(const SOLDIERTYPE *pSoldier, uint8_t *pubPoints);
void HandleBPEffectDueToDrugs(SOLDIERTYPE *pSoldier, int16_t *psPoints);

int8_t GetDrugEffect(SOLDIERTYPE *pSoldier, uint8_t ubDrugType);
int8_t GetDrunkLevel(const SOLDIERTYPE *pSoldier);
int32_t EffectStatForBeingDrunk(const SOLDIERTYPE *pSoldier, int32_t iStat);
BOOLEAN MercUnderTheInfluence(const SOLDIERTYPE *pSoldier);

#endif
