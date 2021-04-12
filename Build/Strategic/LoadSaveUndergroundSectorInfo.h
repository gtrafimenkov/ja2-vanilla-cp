#ifndef LOADSAVEUNDERGROUNDSECTORINFO_H
#define LOADSAVEUNDERGROUNDSECTORINFO_H

#include "Strategic/Campaign_Types.h"


void ExtractUndergroundSectorInfoFromFile(HWFILE, UNDERGROUND_SECTORINFO*);
void InjectUndergroundSectorInfoIntoFile(HWFILE, UNDERGROUND_SECTORINFO const*);

#endif
