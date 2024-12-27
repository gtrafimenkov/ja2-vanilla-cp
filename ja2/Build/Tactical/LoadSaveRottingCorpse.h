// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef LOADSAVEROTTINGCORPSE_H
#define LOADSAVEROTTINGCORPSE_H

#include "Tactical/RottingCorpses.h"

void ExtractRottingCorpseFromFile(HWFILE, ROTTING_CORPSE_DEFINITION *);
void InjectRottingCorpseIntoFile(HWFILE, ROTTING_CORPSE_DEFINITION const *);

#endif
