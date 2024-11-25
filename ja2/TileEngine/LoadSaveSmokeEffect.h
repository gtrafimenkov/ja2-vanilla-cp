// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef LOADSAVESMOKEEFFECT_H
#define LOADSAVESMOKEEFFECT_H

#include "JA2Types.h"

void ExtractSmokeEffectFromFile(HWFILE, SMOKEEFFECT *);
void InjectSmokeEffectIntoFile(HWFILE, SMOKEEFFECT const *);

#endif
