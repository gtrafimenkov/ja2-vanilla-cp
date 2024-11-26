#ifndef CINEMATICS_H
#define CINEMATICS_H

#include "SGP/Types.h"

struct SMKFLIC;

void SmkInitialize();
void SmkShutdown();
SMKFLIC *SmkPlayFlic(const char *filename, uint32_t left, uint32_t top, BOOLEAN auto_close);
BOOLEAN SmkPollFlics();
void SmkCloseFlic(SMKFLIC *);

#endif
