// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __WORDWRAP_H_
#define __WORDWRAP_H_

#include <stdlib.h>

#include "SGP/Types.h"

// Flags for DrawTextToScreen()

// Defines for coded text For use with IanDisplayWrappedString()
#define TEXT_SPACE 32
#define TEXT_CODE_NEWLINE 177
#define TEXT_CODE_BOLD 178
#define TEXT_CODE_CENTER 179
#define TEXT_CODE_NEWCOLOR 180
#define TEXT_CODE_DEFCOLOR 181

uint16_t IanDisplayWrappedString(uint16_t usPosX, uint16_t usPosY, uint16_t usWidth, uint8_t ubGap,
                                 Font, uint8_t ubColor, const wchar_t *pString,
                                 uint8_t ubBackGroundColor, uint32_t uiFlags);

#define LEFT_JUSTIFIED 0x00000001
#define CENTER_JUSTIFIED 0x00000002
#define RIGHT_JUSTIFIED 0x00000004
#define TEXT_SHADOWED 0x00000008

#define INVALIDATE_TEXT 0x00000010
#define DONT_DISPLAY_TEXT \
  0x00000020  // Wont display the text.  Used if you just want to get how many
              // lines will be displayed

#define MARK_DIRTY 0x00000040

#define IAN_WRAP_NO_SHADOW 32

struct WRAPPED_STRING {
  WRAPPED_STRING *pNextWrappedString;
  wchar_t sString[];
};

WRAPPED_STRING *LineWrap(Font, uint16_t usLineWidthPixels, wchar_t const *pString);
uint16_t DisplayWrappedString(uint16_t usPosX, uint16_t usPosY, uint16_t usWidth, uint8_t ubGap,
                              Font, uint8_t ubColor, const wchar_t *pString,
                              uint8_t ubBackGroundColor, uint32_t ulFlags);
void CleanOutControlCodesFromString(const wchar_t *pSourceString, wchar_t *pDestString);
void DrawTextToScreen(const wchar_t *pStr, uint16_t LocX, uint16_t LocY, uint16_t usWidth, Font,
                      uint8_t ubColor, uint8_t ubBackGroundColor, uint32_t ulFlags);
uint16_t IanWrappedStringHeight(uint16_t usWidth, uint8_t ubGap, Font, const wchar_t *pString);

void ReduceStringLength(wchar_t *pString, size_t Length, uint32_t uiWidth, Font);

#endif
