#ifndef LOADSAVELIGHTSPRITE_H
#define LOADSAVELIGHTSPRITE_H

#include "TileEngine/Lighting.h"

void ExtractLightSprite(HWFILE, uint32_t light_time);
void InjectLightSpriteIntoFile(HWFILE, LIGHT_SPRITE const *);

#endif
