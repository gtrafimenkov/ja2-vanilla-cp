// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __EDITORMAPINFO_H
#define __EDITORMAPINFO_H

#include "SGP/Types.h"

void SetupTextInputForMapInfo();
void UpdateMapInfo();
void ExtractAndUpdateMapInfo();
BOOLEAN ApplyNewExitGridValuesToTextFields();
void UpdateMapInfoFields();

extern SGPPaletteEntry gEditorLightColor;

extern BOOLEAN gfEditorForceShadeTableRebuild;

void LocateNextExitGrid();

enum { PRIMETIME_LIGHT, NIGHTTIME_LIGHT, ALWAYSON_LIGHT };
void ChangeLightDefault(int8_t bLightType);
extern int8_t gbDefaultLightType;

#endif
