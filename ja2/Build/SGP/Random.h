// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __RANDOM_
#define __RANDOM_

#include "SGP/Types.h"

extern void InitializeRandom();
extern uint32_t Random(uint32_t uiRange);

// Chance( 74 ) returns TRUE 74% of the time.  If uiChance >= 100, then it will
// always return TRUE.
extern BOOLEAN Chance(uint32_t uiChance);

#define PRERANDOM_GENERATOR

#ifdef PRERANDOM_GENERATOR
// Returns a pregenerated random number.
// Used to deter Ian's tactic of shoot, miss, restore saved game :)
extern uint32_t PreRandom(uint32_t uiRange);
extern BOOLEAN PreChance(uint32_t uiChance);

// IMPORTANT:  Changing this define will invalidate the JA2 save.  If this
//						is necessary, please ifdef your own
// value.
#define MAX_PREGENERATED_NUMS 256
extern uint32_t guiPreRandomIndex;
extern uint32_t guiPreRandomNums[MAX_PREGENERATED_NUMS];
#endif

#endif
