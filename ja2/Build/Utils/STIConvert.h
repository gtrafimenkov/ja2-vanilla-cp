#ifndef __STCICONVERT_H
#define __STCICONVERT_H

#include "SGP/Types.h"

#define CONVERT_ETRLE_COMPRESS 0x0020
#define CONVERT_TO_8_BIT 0x1000

void WriteSTIFile(uint8_t *pData, SGPPaletteEntry *pPalette, int16_t sWidth, int16_t sHeight,
                  const char *cOutputName, uint32_t fFlags, uint32_t uiAppDataSize);

#endif
