// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "SysGlobals.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "GameLoop.h"
#include "SGP/Types.h"
#include "ScreenIDs.h"

char g_filename[200];

char gubErrorText[200];
BOOLEAN gfEditMode = FALSE;
BOOLEAN fFirstTimeInGameScreen = TRUE;
int8_t gbFPSDisplay = SHOW_MIN_FPS;
BOOLEAN gfGlobalError = FALSE;

uint32_t guiGameCycleCounter = 0;

void SET_ERROR(char const *const String, ...) {
  va_list ArgPtr;

  va_start(ArgPtr, String);
  vsprintf(gubErrorText, String, ArgPtr);
  va_end(ArgPtr);

  SetPendingNewScreen(ERROR_SCREEN);

  gfGlobalError = TRUE;
}
