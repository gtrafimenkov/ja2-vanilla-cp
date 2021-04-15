#ifndef LOADSAVELIGHTSPRITE_H
#define LOADSAVELIGHTSPRITE_H

#include "TileEngine/Lighting.h"

void ExtractLightSprite(HWFILE, UINT32 light_time);
void InjectLightSpriteIntoFile(HWFILE, LIGHT_SPRITE const *);

#endif
