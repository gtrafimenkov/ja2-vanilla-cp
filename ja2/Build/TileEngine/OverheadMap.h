#ifndef __OVERHEADMAP_H
#define __OVERHEADMAP_H

#include "JA2Types.h"
#include "TileEngine/WorldTilesetEnums.h"

void InitNewOverheadDB(TileSetID);
void RenderOverheadMap(int16_t sStartPointX_M, int16_t sStartPointY_M, int16_t sStartPointX_S,
                       int16_t sStartPointY_S, int16_t sEndXS, int16_t sEndYS,
                       BOOLEAN fFromMapUtility);

void HandleOverheadMap();
BOOLEAN InOverheadMap();
void GoIntoOverheadMap();
void KillOverheadMap();

void CalculateRestrictedMapCoords(int8_t bDirection, int16_t *psX1, int16_t *psY1, int16_t *psX2,
                                  int16_t *psY2, int16_t sEndXS, int16_t sEndYS);

void TrashOverheadMap();

GridNo GetOverheadMouseGridNo();

extern BOOLEAN gfOverheadMapDirty;

#endif
