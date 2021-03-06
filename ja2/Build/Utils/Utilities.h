#ifndef _UTILITIES_H_
#define _UTILITIES_H_

#include "Tactical/OverheadTypes.h"

BOOLEAN CreateSGPPaletteFromCOLFile(SGPPaletteEntry *pal, const char *col_file);

void DisplayPaletteRep(const PaletteRepID aPalRep, UINT8 ubXPos, UINT8 ubYPos, SGPVSurface *dst);

#endif
