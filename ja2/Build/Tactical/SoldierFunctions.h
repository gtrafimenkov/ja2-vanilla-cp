// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef _SOLDIER_FUNCTIONS_H
#define _SOLDIER_FUNCTIONS_H

#include "JA2Types.h"

void ContinueMercMovement(SOLDIERTYPE *pSoldier);

BOOLEAN IsValidStance(const SOLDIERTYPE *pSoldier, int8_t bNewStance);
void SelectMoveAnimationFromStance(SOLDIERTYPE *pSoldier);
BOOLEAN IsValidMovementMode(const SOLDIERTYPE *pSoldier, int16_t usMovementMode);
void SoldierCollapse(SOLDIERTYPE *pSoldier);

BOOLEAN ReevaluateEnemyStance(SOLDIERTYPE *pSoldier, uint16_t usAnimState);

void HandlePlacingRoofMarker(SOLDIERTYPE &, bool set, bool force);

void PickPickupAnimation(SOLDIERTYPE *pSoldier, int32_t iItemIndex, int16_t sGridNo,
                         int8_t bZLevel);

void MercStealFromMerc(SOLDIERTYPE *pSoldier, const SOLDIERTYPE *pTarget);

void HandleCrowShadowVisibility(SOLDIERTYPE &);

#endif
