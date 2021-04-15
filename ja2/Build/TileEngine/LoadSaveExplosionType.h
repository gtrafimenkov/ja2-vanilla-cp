#ifndef LOADSAVEEXPLOSIONTYPE_H
#define LOADSAVEEXPLOSIONTYPE_H

#include "TileEngine/ExplosionControl.h"

void ExtractExplosionTypeFromFile(HWFILE, EXPLOSIONTYPE *);
void InjectExplosionTypeIntoFile(HWFILE, EXPLOSIONTYPE const *);

#endif
