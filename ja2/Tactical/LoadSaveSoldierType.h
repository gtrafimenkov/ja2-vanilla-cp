// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef LOADSAVESOLDIERTYPE_H
#define LOADSAVESOLDIERTYPE_H

#include "JA2Types.h"

void ExtractSoldierType(const uint8_t *Src, SOLDIERTYPE *Soldier, bool stracLinuxFormat);

void InjectSoldierType(uint8_t *Dst, const SOLDIERTYPE *Soldier);

#endif
