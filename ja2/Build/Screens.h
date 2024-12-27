// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __SCREEN_MANAGER
#define __SCREEN_MANAGER

#include "SGP/Types.h"
#include "ScreenIDs.h"

// Each screen in the game comes with a Status flag (what was the last thing the
// screen was doing), an Initialization function (which loads up the screen if
// necessary), a Handler function which is called while the screen is showing
// and a shutdown function which is called when the screen is getting ready to
// make another screen active.

struct Screens {
  void (*InitializeScreen)();
  ScreenID (*HandleScreen)();
  void (*ShutdownScreen)();
};

// This extern is made available to make sure that external modules will have
// access to the screen information

extern Screens const GameScreens[];

#endif
