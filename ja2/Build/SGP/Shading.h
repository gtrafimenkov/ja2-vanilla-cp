#ifndef SHADING_H
#define SHADING_H

#include "SGP/Types.h"

void BuildShadeTable();
void BuildIntensityTable();
void SetShadeTablePercent(float uiShadePercent);

extern UINT16 IntensityTable[65536];
extern UINT16 ShadeTable[65536];
extern UINT16 White16BPPPalette[256];

#define DEFAULT_SHADE_LEVEL 4

#endif
