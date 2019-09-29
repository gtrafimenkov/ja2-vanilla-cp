#ifndef LOADSAVESMOKEEFFECT_H
#define LOADSAVESMOKEEFFECT_H

#include "Build/JA2Types.h"


void ExtractSmokeEffectFromFile(HWFILE, SMOKEEFFECT*);
void InjectSmokeEffectIntoFile(HWFILE, SMOKEEFFECT const*);

#endif
