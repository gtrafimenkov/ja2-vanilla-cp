#ifndef __NEWSMOOTH_H
#define __NEWSMOOTH_H

#include "SGP/Types.h"

void AddBuildingSectionToWorld(SGPRect *pSelectRegion);
void RemoveBuildingSectionFromWorld(SGPRect *pSelectRegion);

void AddCaveSectionToWorld(SGPRect *pSelectRegion);
void RemoveCaveSectionFromWorld(SGPRect *pSelectRegion);

void EraseBuilding(uint32_t iMapIndex);
void RebuildRoof(uint32_t iMapIndex, uint16_t usRoofType);
void RebuildRoofUsingFloorInfo(int32_t iMapIndex, uint16_t usRoofType);

void AddCave(int32_t iMapIndex, uint16_t usIndex);

void AnalyseCaveMapForStructureInfo();

uint16_t PickAWallPiece(uint16_t usWallPieceType);

#endif
