// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef LOADSAVETACTICALSTATUSTYPE_H
#define LOADSAVETACTICALSTATUSTYPE_H

#include "SGP/Types.h"

#define TACTICAL_STATUS_TYPE_SIZE (316)
#define TACTICAL_STATUS_TYPE_SIZE_STRAC_LINUX (360)

void ExtractTacticalStatusTypeFromFile(HWFILE, bool stracLinuxFormat);
void InjectTacticalStatusTypeIntoFile(HWFILE);

#endif
