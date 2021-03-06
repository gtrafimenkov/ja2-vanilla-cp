#ifndef LOADSAVETACTICALSTATUSTYPE_H
#define LOADSAVETACTICALSTATUSTYPE_H

#include "SGP/Types.h"

#define TACTICAL_STATUS_TYPE_SIZE (316)
#define TACTICAL_STATUS_TYPE_SIZE_STRAC_LINUX (360)

void ExtractTacticalStatusTypeFromFile(HWFILE, bool stracLinuxFormat);
void InjectTacticalStatusTypeIntoFile(HWFILE);

#endif
