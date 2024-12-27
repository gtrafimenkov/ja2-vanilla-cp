// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef _GAME_VERSION_H_
#define _GAME_VERSION_H_

#include "SGP/Types.h"

//
//	Keeps track of the game version
//

extern const char g_version_label[];
extern const char g_version_number[16];

//
//		Keeps track of the saved game version.  Increment the saved game
// version whenever 	you will invalidate the saved game file
//

extern const uint32_t guiSavedGameVersion;

#endif
