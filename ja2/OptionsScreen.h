// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef _OPTIONS_SCREEN__H_
#define _OPTIONS_SCREEN__H_

#include "SGP/Types.h"
#include "ScreenIDs.h"

#define OPT_BUTTON_FONT FONT14ARIAL
#define OPT_BUTTON_ON_COLOR 73   // FONT_MCOLOR_WHITE
#define OPT_BUTTON_OFF_COLOR 73  // FONT_MCOLOR_WHITE

// Record the previous screen the user was in.
extern ScreenID guiPreviousOptionScreen;

ScreenID OptionsScreenHandle();

#endif
