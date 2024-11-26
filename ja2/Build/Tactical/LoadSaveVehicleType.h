#ifndef LOADSAVEVEHICLETYPE_H
#define LOADSAVEVEHICLETYPE_H

#include "JA2Types.h"

void ExtractVehicleTypeFromFile(HWFILE, VEHICLETYPE *, uint32_t savegame_version);
void InjectVehicleTypeIntoFile(HWFILE, VEHICLETYPE const *);

#endif
