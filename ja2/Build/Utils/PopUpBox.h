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

PopUpBox *CreatePopUpBox(SGPPoint Position, UINT32 uiFlags, SGPVSurface *, const SGPVObject *border,
                         SGPVSurface *background, UINT32 margin_l, UINT32 margin_t, UINT32 margin_b,
                         UINT32 margin_r, UINT32 line_space);

UINT32 GetTopMarginSize(const PopUpBox *);
UINT32 GetLineSpace(const PopUpBox *);
void SetBoxXY(PopUpBox *, INT16 x, INT16 y);
void SetBoxX(PopUpBox *, INT16 x);
void SetBoxY(PopUpBox *, INT16 y);
UINT32 GetNumberOfLinesOfTextInBox(const PopUpBox *);
SGPBox const &GetBoxArea(PopUpBox const *);
void AddMonoString(PopUpBox *, const wchar_t *pString);
void SetBoxFont(PopUpBox *, Font);
Font GetBoxFont(const PopUpBox *);
void SetBoxForeground(PopUpBox *, UINT8 colour);
void SetBoxBackground(PopUpBox *, UINT8 colour);
void SetBoxHighLight(PopUpBox *, UINT8 colour);
void SetBoxShade(PopUpBox *, UINT8 colour);

void ShadeStringInBox(PopUpBox *, INT32 line, bool shade);

enum PopUpShade { POPUP_SHADE_NONE, POPUP_SHADE, POPUP_SHADE_SECONDARY };
void ShadeStringInBox(PopUpBox *, INT32 line, PopUpShade);

void HighLightBoxLine(PopUpBox *, INT32 iLineNumber);
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
void MarkAllBoxesAsAltered(void);

// is the box being displayed?
BOOLEAN IsBoxShown(const PopUpBox *);

// is this line in the current box set to a shaded state ?
BOOLEAN GetBoxShadeFlag(const PopUpBox *, INT32 iLineNumber);

// set boxes foreground color
void SetBoxLineForeground(PopUpBox *, INT32 iStringValue, UINT8 ubColor);

// hide all visible boxes
void HideAllBoxes(void);

// add a second column monochrome string
void AddSecondColumnMonoString(PopUpBox *, const wchar_t *pString);

// set the minimum offset
void SetBoxSecondColumnMinimumOffset(PopUpBox *, UINT32 uiWidth);

// now on a box wide basis, one if recomened to use this function after adding
// all the strings..rather than on an individual basis
void SetBoxSecondColumnForeground(PopUpBox *, UINT8 colour);
void SetBoxSecondColumnBackground(PopUpBox *, UINT8 colour);
void SetBoxSecondColumnHighLight(PopUpBox *, UINT8 colour);
void SetBoxSecondColumnShade(PopUpBox *, UINT8 colour);

// secondary shades for boxes
void SetBoxSecondaryShade(PopUpBox *, UINT8 colour);

// min width for box
void SpecifyBoxMinWidth(PopUpBox *, INT32 iMinWidth);

#endif
