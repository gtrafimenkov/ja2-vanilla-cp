#ifndef LOADSAVEVEHICLETYPE_H
#define LOADSAVEVEHICLETYPE_H

#include "Build/JA2Types.h"


void ExtractVehicleTypeFromFile(HWFILE, VEHICLETYPE*, UINT32 savegame_version);
void InjectVehicleTypeIntoFile(HWFILE, VEHICLETYPE const*);

#endif
