// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef LOADSAVEEXPLOSIONTYPE_H
#define LOADSAVEEXPLOSIONTYPE_H

#include "TileEngine/ExplosionControl.h"

void ExtractExplosionTypeFromFile(HWFILE, EXPLOSIONTYPE *);
void InjectExplosionTypeIntoFile(HWFILE, EXPLOSIONTYPE const *);

#endif
