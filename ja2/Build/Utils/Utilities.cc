#include "Utils/Utilities.h"

#include "SGP/Debug.h"
#include "SGP/FileMan.h"
#include "SGP/Font.h"
#include "SGP/HImage.h"
#include "SGP/LoadSaveData.h"
#include "SGP/Types.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "Tactical/Overhead.h"
#include "Tactical/OverheadTypes.h"
#include "Utils/FontControl.h"

#include "SDL_pixels.h"

BOOLEAN CreateSGPPaletteFromCOLFile(SGPPaletteEntry *const pal, const char *const col_file) try {
  AutoSGPFile f(FileMan::openForReadingSmart(col_file, true));

  uint8_t data[776];
  FileRead(f, data, sizeof(data));

  const uint8_t *d = data;
  EXTR_SKIP(d, 8);  // skip header
  for (uint32_t i = 0; i != 256; ++i) {
    EXTR_U8(d, pal[i].r)
    EXTR_U8(d, pal[i].g)
    EXTR_U8(d, pal[i].b)
  }
  Assert(d == endof(data));

  return TRUE;
} catch (...) {
  return FALSE;
}

void DisplayPaletteRep(const PaletteRepID aPalRep, const uint8_t ubXPos, const uint8_t ubYPos,
                       SGPVSurface *const dst) {
  uint16_t us16BPPColor;
  uint32_t cnt1;
  uint8_t ubSize;
  int16_t sTLX, sTLY, sBRX, sBRY;

  // Create 16BPP Palette
  const uint8_t ubPaletteRep = GetPaletteRepIndexFromID(aPalRep);

  SetFont(LARGEFONT1);

  ubSize = gpPalRep[ubPaletteRep].ubPaletteSize;

  for (cnt1 = 0; cnt1 < ubSize; cnt1++) {
    sTLX = ubXPos + (uint16_t)((cnt1 % 16) * 20);
    sTLY = ubYPos + (uint16_t)((cnt1 / 16) * 20);
    sBRX = sTLX + 20;
    sBRY = sTLY + 20;

    const SGPPaletteEntry *Clr = &gpPalRep[ubPaletteRep].rgb[cnt1];
    us16BPPColor = Get16BPPColor(FROMRGB(Clr->r, Clr->g, Clr->b));

    ColorFillVideoSurfaceArea(dst, sTLX, sTLY, sBRX, sBRY, us16BPPColor);
  }

  gprintf(ubXPos + 16 * 20, ubYPos, L"%hs", gpPalRep[ubPaletteRep].ID);
}
