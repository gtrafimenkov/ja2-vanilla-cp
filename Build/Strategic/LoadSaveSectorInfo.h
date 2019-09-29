#ifndef LOADSAVESECTORINFO_H
#define LOADSAVESECTORINFO_H

#include "Build/Strategic/Campaign_Types.h"


void ExtractSectorInfoFromFile(HWFILE, SECTORINFO&);
void InjectSectorInfoIntoFile(HWFILE, SECTORINFO const&);

#endif
