// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __SMOOTH_H
#define __SMOOTH_H

#include "SGP/Types.h"

//   Area (pointer to SGP rect) +
//      Location to check-+--|  |       |---- Check left and right edges -----|
//      |---- Check top and bottom edges -----|
#define IsLocationInArea(x, y, r) \
  (((x) >= r->iLeft) && ((x) <= r->iRight) && ((y) >= r->iTop) && ((y) <= r->iBottom))

void SmoothAllTerrainWorld();
void SmoothTerrain(int gridno, int origType, uint16_t *piNewTile, BOOLEAN fForceSmooth);

void SmoothTerrainRadius(uint32_t iMapIndex, uint32_t uiCheckType, uint8_t ubRadius,
                         BOOLEAN fForceSmooth);
void SmoothAllTerrainTypeRadius(uint32_t iMapIndex, uint8_t ubRadius, BOOLEAN fForceSmooth);

#endif
