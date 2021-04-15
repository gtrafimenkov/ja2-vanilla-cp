#ifndef LOADSAVESECTORINFO_H
#define LOADSAVESECTORINFO_H

#include "Strategic/CampaignTypes.h"


void ExtractSectorInfoFromFile(HWFILE, SECTORINFO&);
void InjectSectorInfoIntoFile(HWFILE, SECTORINFO const&);

#endif
