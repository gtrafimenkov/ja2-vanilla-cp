// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __MS_I_TOWNMINE_INFO_H
#define __MS_I_TOWNMINE_INFO_H

#include "JA2Types.h"

extern BOOLEAN fShowTownInfo;

// display the box
void DisplayTownInfo(int16_t sMapX, int16_t sMapY, int8_t bMapZ);

// create or destroy the town info box..should only be directly called the exit
// code for mapscreen
void CreateDestroyTownInfoBox();

extern PopUpBox *ghTownMineBox;

#endif
