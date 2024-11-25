// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef QUANTIZE_H
#define QUANTIZE_H

#include "SGP/Types.h"

void QuantizeImage(uint8_t *pDest, const SGPPaletteEntry *pSrc, int16_t sWidth, int16_t sHeight,
                   SGPPaletteEntry *pPalette);

#endif
