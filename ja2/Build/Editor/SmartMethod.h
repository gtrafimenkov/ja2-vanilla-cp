#ifndef __SMARTMETHOD_H
#define __SMARTMETHOD_H

#include "SGP/Types.h"

void CalcSmartWallDefault(uint16_t *pusObjIndex, uint16_t *pusUseIndex);
void CalcSmartDoorDefault(uint16_t *pusObjIndex, uint16_t *pusUseIndex);
void CalcSmartWindowDefault(uint16_t *pusObjIndex, uint16_t *pusUseIndex);
void CalcSmartBrokenWallDefault(uint16_t *pusObjIndex, uint16_t *pusUseIndex);

void IncSmartWallUIValue();
void DecSmartWallUIValue();
void IncSmartDoorUIValue();
void DecSmartDoorUIValue();
void IncSmartWindowUIValue();
void DecSmartWindowUIValue();
void IncSmartBrokenWallUIValue();
void DecSmartBrokenWallUIValue();

BOOLEAN CalcWallInfoUsingSmartMethod(uint32_t iMapIndex, uint16_t *pusWallType, uint16_t *pusIndex);
BOOLEAN CalcDoorInfoUsingSmartMethod(uint32_t iMapIndex, uint16_t *pusDoorType, uint16_t *pusIndex);
BOOLEAN CalcWindowInfoUsingSmartMethod(uint32_t iMapIndex, uint16_t *pusWallType,
                                       uint16_t *pusIndex);
BOOLEAN CalcBrokenWallInfoUsingSmartMethod(uint32_t iMapIndex, uint16_t *pusWallType,
                                           uint16_t *pusIndex);

void PasteSmartWall(uint32_t iMapIndex);
void PasteSmartDoor(uint32_t iMapIndex);
void PasteSmartWindow(uint32_t iMapIndex);
void PasteSmartBrokenWall(uint32_t iMapIndex);

extern uint8_t gubDoorUIValue;
extern uint8_t gubWindowUIValue;
extern uint8_t gubWallUIValue;
extern uint8_t gubBrokenWallUIValue;

#endif
