// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef LOADSAVEVEHICLETYPE_H
#define LOADSAVEVEHICLETYPE_H

#include "JA2Types.h"

void ExtractVehicleTypeFromFile(HWFILE, VEHICLETYPE *, uint32_t savegame_version);
void InjectVehicleTypeIntoFile(HWFILE, VEHICLETYPE const *);

#endif
