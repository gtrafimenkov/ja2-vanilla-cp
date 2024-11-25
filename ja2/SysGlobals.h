// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __SYS_GLOBALS_H
#define __SYS_GLOBALS_H

#define SHOW_MIN_FPS 0
#define SHOW_FULL_FPS 1

#include "SGP/Types.h"

extern char gubErrorText[200];
extern BOOLEAN gfAniEditMode;
extern BOOLEAN gfEditMode;
extern BOOLEAN fFirstTimeInGameScreen;
extern int8_t gbFPSDisplay;
extern BOOLEAN gfGlobalError;

extern uint32_t guiGameCycleCounter;

void SET_ERROR(char const *const String, ...);

extern char g_filename[200];

#endif
