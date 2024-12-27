// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef AILIST_H
#define AILIST_H

#include "JA2Types.h"

SOLDIERTYPE *RemoveFirstAIListEntry();
bool BuildAIListForTeam(int8_t team);
bool MoveToFrontOfAIList(SOLDIERTYPE *);

#endif
