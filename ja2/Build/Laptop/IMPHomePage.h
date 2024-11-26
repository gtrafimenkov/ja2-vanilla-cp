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

extern const UINT32 GlowColorsList[11];
#endif
