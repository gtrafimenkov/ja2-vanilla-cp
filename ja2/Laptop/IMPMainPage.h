// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __IMP_MAINPAGE_H
#define __IMP_MAINPAGE_H
#include "SGP/Types.h"

void RenderIMPMainPage();
void ExitIMPMainPage();
void EnterIMPMainPage();
void HandleIMPMainPage();

extern int32_t iCurrentProfileMode;

SGPVObject *LoadIMPPortait();

#endif
