#include "SGP/Font.h"

#include <stdarg.h>
#include <wchar.h>

#include "GameRes.h"
#include "Local.h"
#include "Macro.h"
#include "SGP/Debug.h"
#include "SGP/HImage.h"
#include "SGP/MemMan.h"
#include "SGP/TranslationTable.h"
#include "SGP/Types.h"
#include "SGP/VObject.h"
#include "SGP/VObjectBlitters.h"
#include "SGP/VSurface.h"

#include "SDL_pixels.h"

typedef uint8_t GlyphIdx;

// Destination printing parameters
Font FontDefault = 0;
static SGPVSurface *FontDestBuffer;
static SGPRect FontDestRegion = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
static uint16_t FontForeground16 = 0;
static uint16_t FontBackground16 = 0;
static uint16_t FontShadow16 = DEFAULT_SHADOW;

// Temp, for saving printing parameters
static Font SaveFontDefault = 0;
static SGPVSurface *SaveFontDestBuffer = NULL;
static SGPRect SaveFontDestRegion = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
static uint16_t SaveFontForeground16 = 0;
static uint16_t SaveFontShadow16 = 0;
static uint16_t SaveFontBackground16 = 0;

/* Sets both the foreground and the background colors of the current font. The
 * top byte of the parameter word is the background color, and the bottom byte
 * is the foreground. */
void SetFontColors(uint16_t usColors) {
  uint8_t ubForeground = usColors & 0xFF;
  uint8_t ubBackground = (usColors >> 8) & 0xFF;

  SetFontForeground(ubForeground);
  SetFontBackground(ubBackground);
}

/* Sets the foreground color of the currently selected font. The parameter is
 * the index into the 8-bit palette. In 16BPP mode, the RGB values from the
 * palette are used to create the pixel color. Note that if you change fonts,
 * the selected foreground/background colors will stay at what they are
 * currently set to. */
void SetFontForeground(uint8_t ubForeground) {
  if (!FontDefault) return;
  const SGPPaletteEntry *const c = &FontDefault->Palette()[ubForeground];
  FontForeground16 = Get16BPPColor(FROMRGB(c->r, c->g, c->b));
}

void SetFontShadow(uint8_t ubShadow) {
  if (!FontDefault) return;
  const SGPPaletteEntry *const c = &FontDefault->Palette()[ubShadow];
  FontShadow16 = Get16BPPColor(FROMRGB(c->r, c->g, c->b));

  if (ubShadow != 0 && FontShadow16 == 0) FontShadow16 = 1;
}

/* Sets the Background color of the currently selected font. The parameter is
 * the index into the 8-bit palette. In 16BPP mode, the RGB values from the
 * palette are used to create the pixel color. If the background value is zero,
 * the background of the font will be transparent.  Note that if you change
 * fonts, the selected foreground/background colors will stay at what they are
 * currently set to. */
void SetFontBackground(uint8_t ubBackground) {
  if (!FontDefault) return;
  const SGPPaletteEntry *const c = &FontDefault->Palette()[ubBackground];
  FontBackground16 = Get16BPPColor(FROMRGB(c->r, c->g, c->b));
}

/* Loads a font from an ETRLE file */
Font LoadFontFile(const char *filename) {
  Font const font = AddVideoObjectFromFile(filename);
  if (!FontDefault) FontDefault = font;
  return font;
}

/* Deletes the video object of a particular font. Frees up the memory and
 * resources allocated for it. */
void UnloadFont(Font const font) {
  Assert(font);
  DeleteVideoObject(font);
}

/* Returns the width of a given character in the font. */
static uint32_t GetWidth(HVOBJECT const hSrcVObject, GlyphIdx const ssIndex) {
  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(ssIndex);
  return pTrav.usWidth + pTrav.sOffsetX;
}

/* Returns the length of a string in pixels, depending on the font given. */
int16_t StringPixLength(wchar_t const *const string, Font const font) {
  if (!string) return 0;

  uint32_t w = 0;
  for (wchar_t const *c = string; *c != L'\0'; ++c) {
    w += GetCharWidth(font, *c);
  }
  return w;
}

/* Saves the current font printing settings into temporary locations. */
void SaveFontSettings() {
  SaveFontDefault = FontDefault;
  SaveFontDestBuffer = FontDestBuffer;
  SaveFontDestRegion = FontDestRegion;
  SaveFontForeground16 = FontForeground16;
  SaveFontShadow16 = FontShadow16;
  SaveFontBackground16 = FontBackground16;
}

/* Restores the last saved font printing settings from the temporary lactions */
void RestoreFontSettings() {
  FontDefault = SaveFontDefault;
  FontDestBuffer = SaveFontDestBuffer;
  FontDestRegion = SaveFontDestRegion;
  FontForeground16 = SaveFontForeground16;
  FontShadow16 = SaveFontShadow16;
  FontBackground16 = SaveFontBackground16;
}

/* Returns the height of a given character in the font. */
static uint32_t GetHeight(HVOBJECT hSrcVObject, int16_t ssIndex) {
  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(ssIndex);
  return pTrav.usHeight + pTrav.sOffsetY;
}

/* Returns the height of the first character in a font. */
uint16_t GetFontHeight(Font const font) { return GetHeight(font, 0); }

bool IsPrintableChar(wchar_t const c) {
  if (TRANSLATION_TABLE_SIZE <= c) return false;
  return TranslationTable[c] != 0 || c == getZeroGlyphChar();
}

/* Given a wide char, this function returns the index of the glyph. If no glyph
 * exists for the requested wide char, the glyph index of '?' is returned. */
static GlyphIdx GetGlyphIndex(wchar_t const c) {
  if ((0 <= c) && (c < TRANSLATION_TABLE_SIZE)) {
    GlyphIdx const idx = TranslationTable[c];
    if (idx != 0 || c == getZeroGlyphChar()) return idx;
  }
  DebugMsg(TOPIC_FONT_HANDLER, DBG_LEVEL_0, String("Error: Invalid character given U+%04X", c));
  return TranslationTable[L'?'];
}

uint32_t GetCharWidth(HVOBJECT Font, wchar_t c) { return GetWidth(Font, GetGlyphIndex(c)); }

/* Sets the current font number. */
void SetFont(Font const font) {
  Assert(font);
  FontDefault = font;
}

void SetFontAttributes(Font const font, uint8_t const foreground, uint8_t const shadow,
                       uint8_t const background) {
  SetFont(font);
  SetFontForeground(foreground);
  SetFontShadow(shadow);
  SetFontBackground(background);
}

void SetFontDestBuffer(SGPVSurface *const dst, const int32_t x1, const int32_t y1, const int32_t x2,
                       const int32_t y2) {
  Assert(x2 > x1);
  Assert(y2 > y1);

  FontDestBuffer = dst;
  FontDestRegion.iLeft = x1;
  FontDestRegion.iTop = y1;
  FontDestRegion.iRight = x2;
  FontDestRegion.iBottom = y2;
}

void SetFontDestBuffer(SGPVSurface *const dst) {
  SetFontDestBuffer(dst, 0, 0, dst->Width(), dst->Height());
}

/** Replace backbuffer if it is used by the font system. */
void ReplaceFontBackBuffer(SGPVSurface *oldBackbuffer, SGPVSurface *newBackbuffer) {
  if (FontDestBuffer == oldBackbuffer) {
    FontDestBuffer = newBackbuffer;
  }

  if (SaveFontDestBuffer == oldBackbuffer) {
    SaveFontDestBuffer = newBackbuffer;
  }
}

void FindFontRightCoordinates(int16_t sLeft, int16_t sTop, int16_t sWidth, int16_t sHeight,
                              const wchar_t *pStr, Font const font, int16_t *psNewX,
                              int16_t *psNewY) {
  // Compute the coordinates to right justify the text
  int16_t xp = sWidth - StringPixLength(pStr, font) + sLeft;
  int16_t yp = (sHeight - GetFontHeight(font)) / 2 + sTop;

  *psNewX = xp;
  *psNewY = yp;
}

void FindFontCenterCoordinates(int16_t sLeft, int16_t sTop, int16_t sWidth, int16_t sHeight,
                               const wchar_t *pStr, Font const font, int16_t *psNewX,
                               int16_t *psNewY) {
  // Compute the coordinates to center the text
  int16_t xp = (sWidth - StringPixLength(pStr, font) + 1) / 2 + sLeft;
  int16_t yp = (sHeight - GetFontHeight(font)) / 2 + sTop;

  *psNewX = xp;
  *psNewY = yp;
}

void gprintf(int32_t x, int32_t const y, wchar_t const *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  wchar_t string[512];
  vswprintf(string, lengthof(string), fmt, ap);
  va_end(ap);

  SGPVSurface::Lock l(FontDestBuffer);
  uint16_t *const buf = l.Buffer<uint16_t>();
  uint32_t const pitch = l.Pitch();
  Font const font = FontDefault;
  for (wchar_t const *i = string; *i != L'\0'; ++i) {
    GlyphIdx const glyph = GetGlyphIndex(*i);
    Blt8BPPDataTo16BPPBufferTransparentClip(buf, pitch, font, x, y, glyph, &FontDestRegion);
    x += GetWidth(font, glyph);
  }
}

uint32_t MPrintChar(int32_t const x, int32_t const y, wchar_t const c) {
  GlyphIdx const glyph = GetGlyphIndex(c);
  Font const font = FontDefault;
  {
    SGPVSurface::Lock l(FontDestBuffer);
    Blt8BPPDataTo16BPPBufferMonoShadowClip(l.Buffer<uint16_t>(), l.Pitch(), font, x, y, glyph,
                                           &FontDestRegion, FontForeground16, FontBackground16,
                                           FontShadow16);
  }
  return GetWidth(font, glyph);
}

void MPrintBuffer(uint16_t *const pDestBuf, uint32_t const uiDestPitchBYTES, int32_t x,
                  int32_t const y, wchar_t const *str) {
  Font const font = FontDefault;
  for (; *str != L'\0'; ++str) {
    GlyphIdx const glyph = GetGlyphIndex(*str);
    Blt8BPPDataTo16BPPBufferMonoShadowClip(pDestBuf, uiDestPitchBYTES, font, x, y, glyph,
                                           &FontDestRegion, FontForeground16, FontBackground16,
                                           FontShadow16);
    x += GetWidth(font, glyph);
  }
}

void MPrint(int32_t const x, int32_t const y, wchar_t const *const str) {
  SGPVSurface::Lock l(FontDestBuffer);
  MPrintBuffer(l.Buffer<uint16_t>(), l.Pitch(), x, y, str);
}

/* Prints to the currently selected destination buffer, at the X/Y coordinates
 * specified, using the currently selected font. Other than the X/Y coordinates,
 * the parameters are identical to printf. The resulting string may be no longer
 * than 512 word-characters. Uses monochrome font color settings */
void mprintf(int32_t const x, int32_t const y, wchar_t const *const fmt, ...) {
  wchar_t str[512];
  va_list ap;
  va_start(ap, fmt);
  vswprintf(str, lengthof(str), fmt, ap);
  va_end(ap);
  MPrint(x, y, str);
}

void mprintf_buffer(uint16_t *const pDestBuf, uint32_t const uiDestPitchBYTES, int32_t const x,
                    int32_t const y, wchar_t const *const fmt, ...) {
  wchar_t str[512];
  va_list ap;
  va_start(ap, fmt);
  vswprintf(str, lengthof(str), fmt, ap);
  va_end(ap);
  MPrintBuffer(pDestBuf, uiDestPitchBYTES, x, y, str);
}

void InitializeFontManager() {
  FontDefault = 0;
  SetFontDestBuffer(BACKBUFFER);
}
