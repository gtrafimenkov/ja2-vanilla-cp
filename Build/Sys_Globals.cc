#include <stdarg.h>
#include "sgp/Types.h"
#include "Build/ScreenIDs.h"
#include "Build/Sys_Globals.h"
#include "Build/GameLoop.h"


char g_filename[200];

char    gubErrorText[200];
BOOLEAN gfEditMode             = FALSE;
BOOLEAN fFirstTimeInGameScreen = TRUE;
INT8    gbFPSDisplay           = SHOW_MIN_FPS;
BOOLEAN gfGlobalError          = FALSE;

UINT32	guiGameCycleCounter = 0;


void SET_ERROR(char const* const String, ...)
{
  va_list  ArgPtr;

  va_start(ArgPtr, String);
  vsprintf( gubErrorText, String, ArgPtr);
  va_end(ArgPtr);

	SetPendingNewScreen( ERROR_SCREEN );

	gfGlobalError = TRUE;
}
