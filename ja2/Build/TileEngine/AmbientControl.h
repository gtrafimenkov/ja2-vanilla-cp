#ifndef __AMBIENT_CONTROL
#define __AMBIENT_CONTROL

#include "TileEngine/AmbientTypes.h"

void HandleNewSectorAmbience(uint8_t ubAmbientID);
uint32_t SetupNewAmbientSound(uint32_t uiAmbientID);

void StopAmbients();
void DeleteAllAmbients();

extern AMBIENTDATA_STRUCT gAmbData[MAX_AMBIENT_SOUNDS];
extern int16_t gsNumAmbData;

#endif
