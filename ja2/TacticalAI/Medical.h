// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef MEDICAL_H
#define MEDICAL_H

#include "JA2Types.h"

// Can this soldier autobandage others in sector
BOOLEAN CanCharacterAutoBandageTeammate(const SOLDIERTYPE *s);

// Can this grunt be bandaged by a teammate?
BOOLEAN CanCharacterBeAutoBandagedByTeammate(const SOLDIERTYPE *s);

#endif
