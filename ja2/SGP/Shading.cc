// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "SGP/Shading.h"

#include <string.h>

#include "SGP/HImage.h"
#include "SGP/VObject.h"

uint16_t IntensityTable[65536];
uint16_t ShadeTable[65536];
uint16_t White16BPPPalette[256];
static float guiShadePercent = 0.48f;

/* Builds a 16-bit color shading table. This function should be called only
 * after the current video adapter's pixel format is known (IE:
 * GetRgbDistribution() has been called, and the globals for masks and shifts
 * have been initialized by that function), and before any blitting is done.
 * Using the table is a straight lookup. The pixel to be shaded down is used as
 * the index into the table and the entry at that point will be a pixel that is
 * 25% darker.
 */
void BuildShadeTable() {
  for (uint16_t red = 0; red < 256; red += 4) {
    for (uint16_t green = 0; green < 256; green += 4) {
      for (uint16_t blue = 0; blue < 256; blue += 4) {
        uint16_t index = Get16BPPColor(FROMRGB(red, green, blue));
        ShadeTable[index] = Get16BPPColor(
            FROMRGB(red * guiShadePercent, green * guiShadePercent, blue * guiShadePercent));
      }
    }
  }

  memset(White16BPPPalette, 255, sizeof(White16BPPPalette));
}

/* Builds a 16-bit color shading table. This function should be called only
 * after the current video adapter's pixel format is known (IE:
 * GetRgbDistribution() has been called, and the globals for masks and shifts
 * have been initialized by that function), and before any blitting is done.
 */
void BuildIntensityTable() {
  const float dShadedPercent = 0.80f;

  for (uint16_t red = 0; red < 256; red += 4) {
    for (uint16_t green = 0; green < 256; green += 4) {
      for (uint16_t blue = 0; blue < 256; blue += 4) {
        uint16_t index = Get16BPPColor(FROMRGB(red, green, blue));
        IntensityTable[index] = Get16BPPColor(
            FROMRGB(red * dShadedPercent, green * dShadedPercent, blue * dShadedPercent));
      }
    }
  }
}

void SetShadeTablePercent(float uiShadePercent) {
  guiShadePercent = uiShadePercent;
  BuildShadeTable();
}
