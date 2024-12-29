// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef LOADSAVEBASICSOLDIERCREATESTRUCT_H
#define LOADSAVEBASICSOLDIERCREATESTRUCT_H

#include "JA2Types.h"

void ExtractBasicSoldierCreateStructFromFile(HWFILE, BASIC_SOLDIERCREATE_STRUCT &);
void InjectBasicSoldierCreateStructIntoFile(HWFILE, BASIC_SOLDIERCREATE_STRUCT const &);

#endif