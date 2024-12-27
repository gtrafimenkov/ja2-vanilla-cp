// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef _UTILITIES_H_
#define _UTILITIES_H_

#include "Tactical/OverheadTypes.h"

BOOLEAN CreateSGPPaletteFromCOLFile(SGPPaletteEntry *pal, const char *col_file);

void DisplayPaletteRep(const PaletteRepID aPalRep, uint8_t ubXPos, uint8_t ubYPos,
                       SGPVSurface *dst);

#endif
