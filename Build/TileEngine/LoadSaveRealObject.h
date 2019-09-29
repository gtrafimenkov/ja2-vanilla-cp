#ifndef LOADSAVEREALOBJECT_H
#define LOADSAVEREALOBJECT_H

#include "Build/TileEngine/Physics.h"


void ExtractRealObjectFromFile(HWFILE, REAL_OBJECT*);
void InjectRealObjectIntoFile(HWFILE, REAL_OBJECT const*);

#endif
