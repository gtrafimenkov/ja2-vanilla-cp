// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __IMP_HOME_H
#define __IMP_HOME_H

#include "SGP/Types.h"

void EnterImpHomePage();
void RenderImpHomePage();
void ExitImpHomePage();
void HandleImpHomePage();

// minimun glow time
#define MIN_GLOW_DELTA 100
#define CURSOR_HEIGHT GetFontHeight(FONT14ARIAL) + 6

extern const uint32_t GlowColorsList[11];
#endif
