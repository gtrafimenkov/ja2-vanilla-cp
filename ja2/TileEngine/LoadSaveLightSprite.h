// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef LOADSAVELIGHTSPRITE_H
#define LOADSAVELIGHTSPRITE_H

#include "TileEngine/Lighting.h"

void ExtractLightSprite(HWFILE, uint32_t light_time);
void InjectLightSpriteIntoFile(HWFILE, LIGHT_SPRITE const *);

#endif
