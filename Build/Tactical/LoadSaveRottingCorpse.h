#ifndef LOADSAVEROTTINGCORPSE_H
#define LOADSAVEROTTINGCORPSE_H

#include "Tactical/Rotting_Corpses.h"


void ExtractRottingCorpseFromFile(HWFILE, ROTTING_CORPSE_DEFINITION*);
void InjectRottingCorpseIntoFile(HWFILE, ROTTING_CORPSE_DEFINITION const*);

#endif
