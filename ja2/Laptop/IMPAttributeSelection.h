// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef IMP_ATTRIBUTE_SELECTION_H
#define IMP_ATTRIBUTE_SELECTION_H

#include "SGP/Types.h"

void EnterIMPAttributeSelection();
void RenderIMPAttributeSelection();
void ExitIMPAttributeSelection();
void HandleIMPAttributeSelection();

void RenderAttributeBoxes();

extern BOOLEAN fReviewStats;
extern BOOLEAN fFirstIMPAttribTime;
extern BOOLEAN fReturnStatus;

// starting point of skill boxes on bar
#define SKILL_SLIDE_START_X 186
#define SKILL_SLIDE_START_Y 100
#define SKILL_SLIDE_HEIGHT 20

#endif
