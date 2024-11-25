// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef _PLAYER_COMMAND_H
#define _PLAYER_COMMAND_H

// header file to track the information a player 'knows' about a sector, which
// may in fact not be true

#include <stdlib.h>

#include "Strategic/StrategicMovement.h"

// build main facilities strings for sector
void GetSectorFacilitiesFlags(int16_t sMapX, int16_t sMapY, wchar_t *sFacilitiesString,
                              size_t Length);

// set sector as enemy controlled
BOOLEAN SetThisSectorAsEnemyControlled(int16_t sMapX, int16_t sMapY, int8_t bMapZ);

// set sector as player controlled
BOOLEAN SetThisSectorAsPlayerControlled(int16_t sMapX, int16_t sMapY, int8_t bMapZ,
                                        BOOLEAN fContested);

#endif
