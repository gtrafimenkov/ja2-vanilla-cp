// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef LOADSAVEREALOBJECT_H
#define LOADSAVEREALOBJECT_H

#include "TileEngine/Physics.h"

void ExtractRealObjectFromFile(HWFILE, REAL_OBJECT *);
void InjectRealObjectIntoFile(HWFILE, REAL_OBJECT const *);

#endif
