#ifndef AILIST_H
#define AILIST_H

#include "JA2Types.h"

SOLDIERTYPE *RemoveFirstAIListEntry();
bool BuildAIListForTeam(int8_t team);
bool MoveToFrontOfAIList(SOLDIERTYPE *);

#endif
