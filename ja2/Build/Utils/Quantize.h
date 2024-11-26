#ifndef QUANTIZE_H
#define QUANTIZE_H

#include "SGP/Types.h"

void QuantizeImage(uint8_t *pDest, const SGPPaletteEntry *pSrc, int16_t sWidth, int16_t sHeight,
                   SGPPaletteEntry *pPalette);

#endif
