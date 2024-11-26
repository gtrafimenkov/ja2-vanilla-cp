#ifndef CINEMATICS_H
#define CINEMATICS_H

#include "SGP/Types.h"

struct SMKFLIC;

void SmkInitialize();
void SmkShutdown();
SMKFLIC *SmkPlayFlic(const char *filename, UINT32 left, UINT32 top, BOOLEAN auto_close);
BOOLEAN SmkPollFlics();
void SmkCloseFlic(SMKFLIC *);

#endif
