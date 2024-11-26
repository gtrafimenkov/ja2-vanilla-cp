#ifndef __RENDER_FUN_H
#define __RENDER_FUN_H

#include "TileEngine/WorldDef.h"

#define NO_ROOM 0
#define MAX_ROOMS 250

extern uint8_t gubWorldRoomHidden[MAX_ROOMS];
extern uint8_t gubWorldRoomInfo[WORLD_MAX];

void InitRoomDatabase();

void RemoveRoomRoof(uint16_t sGridNo, uint8_t bRoomNum, SOLDIERTYPE *pSoldier);

uint8_t GetRoom(uint16_t gridno);
BOOLEAN InAHiddenRoom(uint16_t sGridNo, uint8_t *pubRoomNo);

void SetGridNoRevealedFlag(uint16_t sGridNo);

void ExamineGridNoForSlantRoofExtraGraphic(GridNo);

void SetRecalculateWireFrameFlagRadius(GridNo pos, int16_t sRadius);

#endif
