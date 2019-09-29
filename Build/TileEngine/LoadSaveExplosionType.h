#ifndef LOADSAVEEXPLOSIONTYPE_H
#define LOADSAVEEXPLOSIONTYPE_H

#include "Build/TileEngine/Explosion_Control.h"


void ExtractExplosionTypeFromFile(HWFILE, EXPLOSIONTYPE*);
void InjectExplosionTypeIntoFile(HWFILE, EXPLOSIONTYPE const*);

#endif
