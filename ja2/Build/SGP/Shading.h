#ifndef SHADING_H
#define SHADING_H

#include "SGP/Types.h"

void BuildShadeTable();
void BuildIntensityTable();
void SetShadeTablePercent(float uiShadePercent);

extern uint16_t IntensityTable[65536];
extern uint16_t ShadeTable[65536];
extern uint16_t White16BPPPalette[256];

#define DEFAULT_SHADE_LEVEL 4

#endif
