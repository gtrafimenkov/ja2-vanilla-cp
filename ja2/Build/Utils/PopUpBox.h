#ifndef __POPUP_BOX
#define __POPUP_BOX

#include "JA2Types.h"

#define MAX_POPUP_BOX_STRING_COUNT \
  50  // worst case = 45: move menu with 20 soldiers, each on different squad +
      // overhead

// PopUpBox Flags
#define POPUP_BOX_FLAG_CENTER_TEXT 2
#define POPUP_BOX_FLAG_RESIZE 4

#define NO_POPUP_BOX NULL

PopUpBox *CreatePopUpBox(SGPPoint Position, uint32_t uiFlags, SGPVSurface *,
                         const SGPVObject *border, SGPVSurface *background, uint32_t margin_l,
                         uint32_t margin_t, uint32_t margin_b, uint32_t margin_r,
                         uint32_t line_space);

uint32_t GetTopMarginSize(const PopUpBox *);
uint32_t GetLineSpace(const PopUpBox *);
void SetBoxXY(PopUpBox *, int16_t x, int16_t y);
void SetBoxX(PopUpBox *, int16_t x);
void SetBoxY(PopUpBox *, int16_t y);
uint32_t GetNumberOfLinesOfTextInBox(const PopUpBox *);
SGPBox const &GetBoxArea(PopUpBox const *);
void AddMonoString(PopUpBox *, const wchar_t *pString);
void SetBoxFont(PopUpBox *, Font);
Font GetBoxFont(const PopUpBox *);
void SetBoxForeground(PopUpBox *, uint8_t colour);
void SetBoxBackground(PopUpBox *, uint8_t colour);
void SetBoxHighLight(PopUpBox *, uint8_t colour);
void SetBoxShade(PopUpBox *, uint8_t colour);

void ShadeStringInBox(PopUpBox *, int32_t line, bool shade);

enum PopUpShade { POPUP_SHADE_NONE, POPUP_SHADE, POPUP_SHADE_SECONDARY };
void ShadeStringInBox(PopUpBox *, int32_t line, PopUpShade);

void HighLightBoxLine(PopUpBox *, int32_t iLineNumber);
void UnHighLightBox(PopUpBox *);
void RemoveAllBoxStrings(PopUpBox *);
void RemoveBox(PopUpBox *);
void ShowBox(PopUpBox *);
void HideBox(PopUpBox *);
void DisplayBoxes(SGPVSurface *buffer);
void DisplayOnePopupBox(PopUpBox *, SGPVSurface *buffer);

// resize this box to the text it contains
void ResizeBoxToText(PopUpBox *);

// force update/redraw of this boxes background
void ForceUpDateOfBox(PopUpBox *);

// force redraw of ALL boxes
void MarkAllBoxesAsAltered();

// is the box being displayed?
BOOLEAN IsBoxShown(const PopUpBox *);

// is this line in the current box set to a shaded state ?
BOOLEAN GetBoxShadeFlag(const PopUpBox *, int32_t iLineNumber);

// set boxes foreground color
void SetBoxLineForeground(PopUpBox *, int32_t iStringValue, uint8_t ubColor);

// hide all visible boxes
void HideAllBoxes();

// add a second column monochrome string
void AddSecondColumnMonoString(PopUpBox *, const wchar_t *pString);

// set the minimum offset
void SetBoxSecondColumnMinimumOffset(PopUpBox *, uint32_t uiWidth);

// now on a box wide basis, one if recomened to use this function after adding
// all the strings..rather than on an individual basis
void SetBoxSecondColumnForeground(PopUpBox *, uint8_t colour);
void SetBoxSecondColumnBackground(PopUpBox *, uint8_t colour);
void SetBoxSecondColumnHighLight(PopUpBox *, uint8_t colour);
void SetBoxSecondColumnShade(PopUpBox *, uint8_t colour);

// secondary shades for boxes
void SetBoxSecondaryShade(PopUpBox *, uint8_t colour);

// min width for box
void SpecifyBoxMinWidth(PopUpBox *, int32_t iMinWidth);

#endif
