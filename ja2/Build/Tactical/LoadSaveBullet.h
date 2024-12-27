// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef LOADSAVEBULLET_H
#define LOADSAVEBULLET_H

#include "Tactical/Bullets.h"

void ExtractBulletFromFile(HWFILE, BULLET *);
void InjectBulletIntoFile(HWFILE, const BULLET *);

#endif
