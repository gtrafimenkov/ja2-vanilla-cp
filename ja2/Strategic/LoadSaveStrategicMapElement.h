// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef LOADSAVESTRATEGICMAPELEMENT_H
#define LOADSAVESTRATEGICMAPELEMENT_H

#include "Strategic/StrategicMap.h"

void ExtractStrategicMapElementFromFile(HWFILE, StrategicMapElement &);
void InjectStrategicMapElementIntoFile(HWFILE, StrategicMapElement const &);

#endif
