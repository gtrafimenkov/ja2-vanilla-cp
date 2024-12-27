// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __CURSOR_MODES_H
#define __CURSOR_MODES_H

#include "SGP/Types.h"

enum {
  SMALLSELECTION,   // 1x1
  MEDIUMSELECTION,  // 3x3
  LARGESELECTION,   // 5x5
  XLARGESELECTION,  // 7x7
  LINESELECTION,    // v or h line with a specified width
  AREASELECTION,    // user controlled area
  NUMSELECTIONTYPES
};

extern uint16_t gusSelectionType;
extern uint16_t gusSelectionWidth;
extern uint16_t gusPreserveSelectionWidth;
extern uint16_t gusSelectionDensity;
extern uint16_t gusSavedSelectionType;
extern uint16_t gusSavedBuildingSelectionType;

BOOLEAN PerformDensityTest();
void SetDrawingMode(int32_t iMode);
void UpdateCursorAreas();
void IncreaseSelectionDensity();
void DecreaseSelectionDensity();
void RemoveCursors();

extern wchar_t SelTypeWidth[10];
extern const wchar_t* const wszSelType[6];

extern BOOLEAN gfCurrentSelectionWithRightButton;

extern SGPRect gSelectRegion;

void RemoveBuildingLayout();

#endif
