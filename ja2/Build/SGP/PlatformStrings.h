// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#pragma once

#include "SGP/Platform.h"

/* Platform and compiler-specifics */

/**************************************************************
 * String functions
 *************************************************************/

#if defined(_MSC_VER)
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#else
#include <strings.h>
#endif

/**************************************************************
 *
 *************************************************************/
