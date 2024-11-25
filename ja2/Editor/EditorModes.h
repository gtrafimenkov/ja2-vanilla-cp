// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __EDITOR_MODES_H
#define __EDITOR_MODES_H

#include "SGP/Types.h"

void SetEditorItemsTaskbarMode(uint16_t usNewMode);
void SetEditorBuildingTaskbarMode(uint16_t usNewMode);
void SetEditorTerrainTaskbarMode(uint16_t usNewMode);
void SetEditorMapInfoTaskbarMode(uint16_t usNewMode);
void SetEditorSmoothingMode(uint8_t ubNewMode);

void HideExitGrids();

#endif
