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
