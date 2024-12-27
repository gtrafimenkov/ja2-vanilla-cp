// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

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
