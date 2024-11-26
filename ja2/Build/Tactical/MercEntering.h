#ifndef _MERC_ENTRING_H
#define _MERC_ENTRING_H

#include "JA2Types.h"

void ResetHeliSeats();
void AddMercToHeli(SOLDIERTYPE *s);

void StartHelicopterRun(int16_t sGridNoSweetSpot);

void HandleHeliDrop();

extern BOOLEAN gfIngagedInDrop;

#endif
