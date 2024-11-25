// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef LOADSAVEUNDERGROUNDSECTORINFO_H
#define LOADSAVEUNDERGROUNDSECTORINFO_H

#include "Strategic/CampaignTypes.h"

void ExtractUndergroundSectorInfoFromFile(HWFILE, UNDERGROUND_SECTORINFO *);
void InjectUndergroundSectorInfoIntoFile(HWFILE, UNDERGROUND_SECTORINFO const *);

#endif
