#ifndef __SOLDIER_ANI_H
#define __SOLDIER_ANI_H

#include "JA2Types.h"

BOOLEAN AdjustToNextAnimationFrame(SOLDIERTYPE *pSoldier);

void CheckForAndHandleSoldierDeath(SOLDIERTYPE *pSoldier, BOOLEAN *pfMadeCorpse);

BOOLEAN CheckForAndHandleSoldierDyingNotFromHit(SOLDIERTYPE *pSoldier);

BOOLEAN HandleSoldierDeath(SOLDIERTYPE *pSoldier, BOOLEAN *pfMadeCorpse);

BOOLEAN OKFallDirection(SOLDIERTYPE *pSoldier, int16_t sGridNo, int8_t bLevel,
                        int8_t bTestDirection, uint16_t usAnimState);

void HandleCheckForDeathCommonCode(SOLDIERTYPE *pSoldier);

void KickOutWheelchair(SOLDIERTYPE *pSoldier);

void HandlePlayerTeamMemberDeathAfterSkullAnimation(SOLDIERTYPE *pSoldier);
void HandleKilledQuote(SOLDIERTYPE *pKilledSoldier, SOLDIERTYPE *pKillerSoldier, int16_t sGridNo,
                       int8_t bLevel);

extern const SOLDIERTYPE *gLastMercTalkedAboutKilling;

#endif
