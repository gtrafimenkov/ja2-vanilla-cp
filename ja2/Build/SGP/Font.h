#ifndef FONT_H
#define FONT_H

#include "SGP/Types.h"

#define DEFAULT_SHADOW 2
#define MILITARY_SHADOW 67
#define NO_SHADOW 0

// these are bogus! No palette is set yet!
// font foreground color symbols
#define FONT_FCOLOR_WHITE 208
#define FONT_FCOLOR_RED 162
#define FONT_FCOLOR_NICERED 164
#define FONT_FCOLOR_BLUE 203
#define FONT_FCOLOR_GREEN 184
#define FONT_FCOLOR_YELLOW 144
#define FONT_FCOLOR_BROWN 184
#define FONT_FCOLOR_ORANGE 76
#define FONT_FCOLOR_PURPLE 160

extern Font FontDefault;

void SetFontColors(uint16_t usColors);
void SetFontForeground(uint8_t ubForeground);
void SetFontBackground(uint8_t ubBackground);
void SetFontShadow(uint8_t ubBackground);

/* Print to the currently selected destination buffer, at the X/Y coordinates
 * specified, using the currently selected font. Other than the X/Y coordinates,
 * the parameters are identical to printf. The resulting string may be no longer
 * than 512 word-characters. */
void gprintf(int32_t x, int32_t y, wchar_t const *fmt, ...);

uint32_t MPrintChar(int32_t x, int32_t y, wchar_t);
void MPrintBuffer(uint16_t *pDestBuf, uint32_t uiDestPitchBYTES, int32_t x, int32_t y,
                  wchar_t const *str);
void MPrint(int32_t x, int32_t y, wchar_t const *str);
void mprintf(int32_t x, int32_t y, wchar_t const *fmt, ...);
void mprintf_buffer(uint16_t *pDestBuf, uint32_t uiDestPitchBYTES, int32_t x, int32_t y,
                    wchar_t const *fmt, ...);

/* Sets the destination buffer for printing to and the clipping rectangle. */
void SetFontDestBuffer(SGPVSurface *dst, int32_t x1, int32_t y1, int32_t x2, int32_t y2);

/* Set the destination buffer for printing while using the whole surface. */
void SetFontDestBuffer(SGPVSurface *dst);

/** Replace backbuffer if it is used by the font system. */
void ReplaceFontBackBuffer(SGPVSurface *oldBackbuffer, SGPVSurface *newBackbuffer);

void SetFont(Font);

void SetFontAttributes(Font, uint8_t foreground, uint8_t shadow = DEFAULT_SHADOW,
                       uint8_t background = 0);

Font LoadFontFile(const char *filename);
uint16_t GetFontHeight(Font);
void InitializeFontManager();
void UnloadFont(Font);

uint32_t GetCharWidth(HVOBJECT Font, wchar_t c);

int16_t StringPixLength(const wchar_t *string, Font);
extern void SaveFontSettings();
extern void RestoreFontSettings();

void FindFontRightCoordinates(int16_t sLeft, int16_t sTop, int16_t sWidth, int16_t sHeight,
                              const wchar_t *pStr, Font, int16_t *psNewX, int16_t *psNewY);
void FindFontCenterCoordinates(int16_t sLeft, int16_t sTop, int16_t sWidth, int16_t sHeight,
                               const wchar_t *pStr, Font, int16_t *psNewX, int16_t *psNewY);

bool IsPrintableChar(wchar_t);

#endif
