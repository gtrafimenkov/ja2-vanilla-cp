// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef _INTRO__C_
#define _INTRO__C_

#include "SGP/Types.h"
#include "ScreenIDs.h"

ScreenID IntroScreenHandle();

// enums used for when the intro screen can come up, used with
// 'gbIntroScreenMode'
enum {
  INTRO_BEGINING,  // set when viewing the intro at the begining of the game
  INTRO_ENDING,    // set when viewing the end game video.

  INTRO_SPLASH,
};

extern uint32_t guiSmackerSurface;

void SetIntroType(int8_t bIntroType);

#endif
