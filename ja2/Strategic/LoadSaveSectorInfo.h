// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef LOADSAVESECTORINFO_H
#define LOADSAVESECTORINFO_H

#include "Strategic/CampaignTypes.h"

void ExtractSectorInfoFromFile(HWFILE, SECTORINFO &);
void InjectSectorInfoIntoFile(HWFILE, SECTORINFO const &);

#endif
