// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef _AMBIENT_TYPES_H
#define _AMBIENT_TYPES_H

#include <string>

#include "SGP/Types.h"

#define MAX_AMBIENT_SOUNDS 100

#define AMB_TOD_DAWN 0
#define AMB_TOD_DAY 1
#define AMB_TOD_DUSK 2
#define AMB_TOD_NIGHT 3

struct AMBIENTDATA_STRUCT {
  uint32_t uiMinTime;
  uint32_t uiMaxTime;
  uint8_t ubTimeCatagory;
  SGPFILENAME zFilename;
  uint32_t uiVol;
};

#endif
