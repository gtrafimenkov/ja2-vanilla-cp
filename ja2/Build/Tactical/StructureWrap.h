// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef STRUCURE_WRAP_H
#define STRUCURE_WRAP_H

#include "JA2Types.h"
#include "SGP/Types.h"

BOOLEAN IsTreePresentAtGridno(int16_t sGridNo);
BOOLEAN IsFencePresentAtGridno(int16_t sGridNo);
BOOLEAN IsJumpableFencePresentAtGridno(int16_t sGridNo);

BOOLEAN IsDoorVisibleAtGridNo(int16_t sGridNo);

BOOLEAN WallExistsOfTopLeftOrientation(int16_t sGridNo);

BOOLEAN WallExistsOfTopRightOrientation(int16_t sGridNo);

BOOLEAN WallOrClosedDoorExistsOfTopLeftOrientation(int16_t sGridNo);

BOOLEAN WallOrClosedDoorExistsOfTopRightOrientation(int16_t sGridNo);

BOOLEAN OpenRightOrientedDoorWithDoorOnRightOfEdgeExists(int16_t sGridNo);
BOOLEAN OpenLeftOrientedDoorWithDoorOnLeftOfEdgeExists(int16_t sGridNo);

STRUCTURE *GetWallStructOfSameOrientationAtGridno(GridNo, int8_t orientation);

BOOLEAN CutWireFence(int16_t sGridNo);
BOOLEAN IsCuttableWireFenceAtGridNo(int16_t sGridNo);

BOOLEAN IsRepairableStructAtGridNo(int16_t sGridNo, SOLDIERTYPE **tgt);
SOLDIERTYPE *GetRefuelableStructAtGridNo(int16_t sGridNo);

BOOLEAN IsRoofPresentAtGridno(int16_t sGridNo);

int16_t FindDoorAtGridNoOrAdjacent(int16_t sGridNo);

BOOLEAN IsCorpseAtGridNo(int16_t sGridNo, uint8_t ubLevel);

BOOLEAN SetOpenableStructureToClosed(int16_t sGridNo, uint8_t ubLevel);

#endif
