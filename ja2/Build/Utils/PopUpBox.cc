#include "Utils/PopUpBox.h"

#include <stdexcept>

#include "Local.h"
#include "SGP/Debug.h"
#include "SGP/Font.h"
#include "SGP/MemMan.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "TileEngine/SysUtil.h"

#define MAX_POPUP_BOX_COUNT 20

struct PopUpString {
  wchar_t *pString;
  uint8_t ubForegroundColor;
  uint8_t ubBackgroundColor;
  uint8_t ubHighLight;
  uint8_t ubShade;
  uint8_t ubSecondaryShade;
  BOOLEAN fHighLightFlag;
  BOOLEAN fShadeFlag;
  BOOLEAN fSecondaryShadeFlag;
};

struct PopUpBox {
  SGPBox pos;
  uint32_t uiLeftMargin;
  uint32_t uiRightMargin;
  uint32_t uiBottomMargin;
  uint32_t uiTopMargin;
  uint32_t uiLineSpace;
  const SGPVObject *iBorderObjectIndex;
  SGPVSurface *iBackGroundSurface;
  uint32_t uiFlags;
  SGPVSurface *uiBuffer;
  uint32_t uiSecondColumnMinimunOffset;
  uint32_t uiSecondColumnCurrentOffset;
  uint32_t uiBoxMinWidth;
  BOOLEAN fUpdated;
  BOOLEAN fShowBox;
  Font font;

  PopUpString *Text[MAX_POPUP_BOX_STRING_COUNT];
  PopUpString *pSecondColumnString[MAX_POPUP_BOX_STRING_COUNT];
};

static PopUpBox *PopUpBoxList[MAX_POPUP_BOX_COUNT];

#define FOR_EACH_POPUP_BOX(iter)           \
  FOR_EACH(PopUpBox *, iter, PopUpBoxList) \
  if (*iter == NULL)                       \
    continue;                              \
  else

#define BORDER_WIDTH 16
#define BORDER_HEIGHT 8
#define TOP_LEFT_CORNER 0
#define TOP_EDGE 4
#define TOP_RIGHT_CORNER 1
#define SIDE_EDGE 5
#define BOTTOM_LEFT_CORNER 2
#define BOTTOM_EDGE 4
#define BOTTOM_RIGHT_CORNER 3

uint32_t GetLineSpace(const PopUpBox *const box) {
  // return number of pixels between lines for this box
  return box->uiLineSpace;
}

void SpecifyBoxMinWidth(PopUpBox *const box, int32_t iMinWidth) {
  box->uiBoxMinWidth = iMinWidth;

  // check if the box is currently too small
  if (box->pos.w < iMinWidth) box->pos.w = iMinWidth;
}

PopUpBox *CreatePopUpBox(const SGPPoint Position, const uint32_t uiFlags, SGPVSurface *const buffer,
                         const SGPVObject *const border, SGPVSurface *const background,
                         const uint32_t margin_l, const uint32_t margin_t, const uint32_t margin_b,
                         const uint32_t margin_r, const uint32_t line_space) {
  // find first free box
  FOR_EACH(PopUpBox *, i, PopUpBoxList) {
    if (*i == NULL) {
      PopUpBox *const box = MALLOCZ(PopUpBox);
      SetBoxXY(box, Position.iX, Position.iY);
      box->uiFlags = uiFlags;
      box->uiBuffer = buffer;
      box->iBorderObjectIndex = border;
      box->iBackGroundSurface = background;
      box->uiLeftMargin = margin_l;
      box->uiRightMargin = margin_r;
      box->uiTopMargin = margin_t;
      box->uiBottomMargin = margin_b;
      box->uiLineSpace = line_space;

      *i = box;
      return box;
    }
  }

  // ran out of available popup boxes - probably not freeing them up right!
  throw std::runtime_error("Out of popup box slots");
}

uint32_t GetTopMarginSize(const PopUpBox *const box) {
  // return size of top margin, for mouse region offsets
  return box->uiTopMargin;
}

void ShadeStringInBox(PopUpBox *const box, int32_t const line, bool const shade) {
  PopUpString *const s = box->Text[line];
  if (s) s->fShadeFlag = shade;
}

void ShadeStringInBox(PopUpBox *const box, int32_t const line, PopUpShade const shade) {
  PopUpString *const s = box->Text[line];
  if (!s) return;

  s->fShadeFlag = shade == POPUP_SHADE;
  s->fSecondaryShadeFlag = shade == POPUP_SHADE_SECONDARY;
}

void SetBoxXY(PopUpBox *const box, const int16_t x, const int16_t y) {
  box->pos.x = x;
  box->pos.y = y;
  box->fUpdated = FALSE;
}

void SetBoxX(PopUpBox *const box, const int16_t x) {
  box->pos.x = x;
  box->fUpdated = FALSE;
}

void SetBoxY(PopUpBox *const box, const int16_t y) {
  box->pos.y = y;
  box->fUpdated = FALSE;
}

SGPBox const &GetBoxArea(PopUpBox const *const box) { return box->pos; }

// adds a FIRST column string to the CURRENT popup box
void AddMonoString(PopUpBox *const box, const wchar_t *pString) {
  int32_t iCounter = 0;

  // find first free slot in list
  for (iCounter = 0; iCounter < MAX_POPUP_BOX_STRING_COUNT && box->Text[iCounter] != NULL;
       iCounter++);

  if (iCounter >= MAX_POPUP_BOX_STRING_COUNT) {
    // using too many text lines, or not freeing them up properly
    Assert(0);
    return;
  }

  PopUpString *const pStringSt = MALLOC(PopUpString);
  wchar_t *const pLocalString = MALLOCN(wchar_t, wcslen(pString) + 1);

  wcscpy(pLocalString, pString);

  box->Text[iCounter] = pStringSt;
  box->Text[iCounter]->pString = pLocalString;
  box->Text[iCounter]->fShadeFlag = FALSE;
  box->Text[iCounter]->fHighLightFlag = FALSE;
  box->Text[iCounter]->fSecondaryShadeFlag = FALSE;

  box->fUpdated = FALSE;
}

static void RemoveBoxSecondaryText(PopUpBox *, int32_t hStringHandle);

void AddSecondColumnMonoString(PopUpBox *const box, const wchar_t *const pString) {
  int32_t iCounter = 0;

  // find the LAST USED text string index
  for (iCounter = 0; iCounter + 1 < MAX_POPUP_BOX_STRING_COUNT && box->Text[iCounter + 1] != NULL;
       iCounter++);

  if (iCounter >= MAX_POPUP_BOX_STRING_COUNT) {
    // using too many text lines, or not freeing them up properly
    Assert(0);
    return;
  }

  PopUpString *const pStringSt = MALLOC(PopUpString);
  wchar_t *const pLocalString = MALLOCN(wchar_t, wcslen(pString) + 1);

  wcscpy(pLocalString, pString);

  RemoveBoxSecondaryText(box, iCounter);

  box->pSecondColumnString[iCounter] = pStringSt;
  box->pSecondColumnString[iCounter]->pString = pLocalString;
  box->pSecondColumnString[iCounter]->fShadeFlag = FALSE;
  box->pSecondColumnString[iCounter]->fHighLightFlag = FALSE;
}

uint32_t GetNumberOfLinesOfTextInBox(const PopUpBox *const box) {
  int32_t iCounter = 0;

  // count number of lines
  // check string size
  for (iCounter = 0; iCounter < MAX_POPUP_BOX_STRING_COUNT; iCounter++) {
    if (box->Text[iCounter] == NULL) break;
  }

  return (iCounter);
}

void SetBoxFont(PopUpBox *const box, Font const font) {
  box->font = font;
  box->fUpdated = FALSE;
}

void SetBoxSecondColumnMinimumOffset(PopUpBox *const box, const uint32_t uiWidth) {
  box->uiSecondColumnMinimunOffset = uiWidth;
}

Font GetBoxFont(const PopUpBox *const box) { return box->font; }

// set the foreground color of this string in this pop up box
void SetBoxLineForeground(PopUpBox *const box, const int32_t iStringValue, const uint8_t ubColor) {
  box->Text[iStringValue]->ubForegroundColor = ubColor;
}

void SetBoxSecondaryShade(PopUpBox *const box, uint8_t const colour) {
  FOR_EACH(PopUpString *, i, box->Text) {
    PopUpString *const p = *i;
    if (p) p->ubSecondaryShade = colour;
  }
}

void SetBoxForeground(PopUpBox *const box, uint8_t const colour) {
  FOR_EACH(PopUpString *, i, box->Text) {
    PopUpString *const p = *i;
    if (p) p->ubForegroundColor = colour;
  }
}

void SetBoxBackground(PopUpBox *const box, uint8_t const colour) {
  FOR_EACH(PopUpString *, i, box->Text) {
    PopUpString *const p = *i;
    if (p) p->ubBackgroundColor = colour;
  }
}

void SetBoxHighLight(PopUpBox *const box, uint8_t const colour) {
  FOR_EACH(PopUpString *, i, box->Text) {
    PopUpString *const p = *i;
    if (p) p->ubHighLight = colour;
  }
}

void SetBoxShade(PopUpBox *const box, uint8_t const colour) {
  FOR_EACH(PopUpString *, i, box->Text) {
    PopUpString *const p = *i;
    if (p) p->ubShade = colour;
  }
}

void SetBoxSecondColumnForeground(PopUpBox *const box, uint8_t const colour) {
  FOR_EACH(PopUpString *, i, box->pSecondColumnString) {
    PopUpString *const p = *i;
    if (p) p->ubForegroundColor = colour;
  }
}

void SetBoxSecondColumnBackground(PopUpBox *const box, uint8_t const colour) {
  FOR_EACH(PopUpString *, i, box->pSecondColumnString) {
    PopUpString *const p = *i;
    if (p) p->ubBackgroundColor = colour;
  }
}

void SetBoxSecondColumnHighLight(PopUpBox *const box, uint8_t const colour) {
  FOR_EACH(PopUpString *, i, box->pSecondColumnString) {
    PopUpString *const p = *i;
    if (p) p->ubHighLight = colour;
  }
}

void SetBoxSecondColumnShade(PopUpBox *const box, uint8_t const colour) {
  FOR_EACH(PopUpString *, i, box->pSecondColumnString) {
    PopUpString *const p = *i;
    if (p) p->ubShade = colour;
  }
}

void HighLightBoxLine(PopUpBox *const box, const int32_t iLineNumber) {
  // highlight iLineNumber Line in box
  PopUpString *const line = box->Text[iLineNumber];
  if (line != NULL) {
    line->fHighLightFlag = TRUE;
  }
}

BOOLEAN GetBoxShadeFlag(const PopUpBox *const box, const int32_t iLineNumber) {
  if (box->Text[iLineNumber] != NULL) {
    return box->Text[iLineNumber]->fShadeFlag;
  }
  return (FALSE);
}

void UnHighLightBox(PopUpBox *const box) {
  FOR_EACH(PopUpString *, i, box->Text) {
    PopUpString *const p = *i;
    if (p) p->fHighLightFlag = FALSE;
  }
}

static void RemoveBoxPrimaryText(PopUpBox *, int32_t hStringHandle);

void RemoveAllBoxStrings(PopUpBox *const box) {
  for (uint32_t i = 0; i < MAX_POPUP_BOX_STRING_COUNT; ++i) {
    RemoveBoxPrimaryText(box, i);
    RemoveBoxSecondaryText(box, i);
  }
  box->fUpdated = FALSE;
}

void RemoveBox(PopUpBox *const box) {
  FOR_EACH_POPUP_BOX(i) {
    if (*i == box) {
      *i = NULL;
      RemoveAllBoxStrings(box);
      MemFree(box);
      return;
    }
  }
  Assert(0);
}

void ShowBox(PopUpBox *const box) {
  if (!box->fShowBox) {
    box->fShowBox = TRUE;
    box->fUpdated = FALSE;
  }
}

void HideBox(PopUpBox *const box) {
  if (box->fShowBox) {
    box->fShowBox = FALSE;
    box->fUpdated = FALSE;
  }
}

void DisplayBoxes(SGPVSurface *const uiBuffer) {
  FOR_EACH_POPUP_BOX(i) { DisplayOnePopupBox(*i, uiBuffer); }
}

static void DrawBox(const PopUpBox *);
static void DrawBoxText(const PopUpBox *);

void DisplayOnePopupBox(PopUpBox *const box, SGPVSurface *const uiBuffer) {
  if (!box->fUpdated && box->fShowBox && box->uiBuffer == uiBuffer) {
    box->fUpdated = TRUE;
    if (box->uiFlags & POPUP_BOX_FLAG_RESIZE) ResizeBoxToText(box);
    DrawBox(box);
    DrawBoxText(box);
  }
}

void ForceUpDateOfBox(PopUpBox *const box) { box->fUpdated = FALSE; }

static void DrawBox(const PopUpBox *const box) {
  const uint16_t x = box->pos.x;
  const uint16_t y = box->pos.y;
  uint16_t w = box->pos.w;
  const uint16_t h = box->pos.h;

  // make sure it will fit on screen!
  Assert(x + w <= SCREEN_WIDTH);
  Assert(y + h <= SCREEN_HEIGHT);

  // subtract 4 because the 2 2-pixel corners are handled separately
  const uint32_t uiNumTilesWide = (w - 4) / BORDER_WIDTH;
  const uint32_t uiNumTilesHigh = (h - 4) / BORDER_HEIGHT;

  SGPVSurface *const dst = box->uiBuffer;

  // blit in texture first, then borders
  SGPBox const clip = {0, 0, w, h};
  BltVideoSurface(dst, box->iBackGroundSurface, x, y, &clip);

  const SGPVObject *const border = box->iBorderObjectIndex;

  // blit in 4 corners (they're 2x2 pixels)
  BltVideoObject(dst, border, TOP_LEFT_CORNER, x, y);
  BltVideoObject(dst, border, TOP_RIGHT_CORNER, x + w - 2, y);
  BltVideoObject(dst, border, BOTTOM_RIGHT_CORNER, x + w - 2, y + h - 2);
  BltVideoObject(dst, border, BOTTOM_LEFT_CORNER, x, y + h - 2);

  // blit in edges
  if (uiNumTilesWide > 0) {
    // full pieces
    for (uint32_t i = 0; i < uiNumTilesWide; ++i) {
      const int32_t lx = x + 2 + i * BORDER_WIDTH;
      BltVideoObject(dst, border, TOP_EDGE, lx, y);
      BltVideoObject(dst, border, BOTTOM_EDGE, lx, y + h - 2);
    }

    // partial pieces
    const int32_t lx = x + w - 2 - BORDER_WIDTH;
    BltVideoObject(dst, border, TOP_EDGE, lx, y);
    BltVideoObject(dst, border, BOTTOM_EDGE, lx, y + h - 2);
  }
  if (uiNumTilesHigh > 0) {
    // full pieces
    for (uint32_t i = 0; i < uiNumTilesHigh; ++i) {
      const int32_t ly = y + 2 + i * BORDER_HEIGHT;
      BltVideoObject(dst, border, SIDE_EDGE, x, ly);
      BltVideoObject(dst, border, SIDE_EDGE, x + w - 2, ly);
    }

    // partial pieces
    const int32_t ly = y + h - 2 - BORDER_HEIGHT;
    BltVideoObject(dst, border, SIDE_EDGE, x, ly);
    BltVideoObject(dst, border, SIDE_EDGE, x + w - 2, ly);
  }

  InvalidateRegion(x, y, x + w, y + h);
}

static void DrawBoxText(const PopUpBox *const box) {
  Font const font = box->font;
  int32_t const tlx = box->pos.x + box->uiLeftMargin;
  int32_t const tly = box->pos.y + box->uiTopMargin;
  int32_t const brx = box->pos.x + box->pos.w - box->uiRightMargin;
  int32_t const bry = box->pos.y + box->pos.h - box->uiBottomMargin;
  int32_t const w = box->pos.w - (box->uiRightMargin + box->uiLeftMargin + 2);
  int32_t const h = GetFontHeight(font);

  SetFont(font);
  SetFontDestBuffer(box->uiBuffer, tlx - 1, tly, brx, bry);

  for (uint32_t i = 0; i < MAX_POPUP_BOX_STRING_COUNT; ++i) {
    // there is text in this line?
    const PopUpString *const text = box->Text[i];
    if (text) {
      // are we highlighting?...shading?..or neither
      if (text->fHighLightFlag) {
        SetFontForeground(text->ubHighLight);
      } else if (text->fSecondaryShadeFlag) {
        SetFontForeground(text->ubSecondaryShade);
      } else if (text->fShadeFlag) {
        SetFontForeground(text->ubShade);
      } else {
        SetFontForeground(text->ubForegroundColor);
      }

      SetFontBackground(text->ubBackgroundColor);

      const int32_t y = tly + i * (h + box->uiLineSpace);
      int16_t uX;
      int16_t uY;
      if (box->uiFlags & POPUP_BOX_FLAG_CENTER_TEXT) {
        FindFontCenterCoordinates(tlx, y, w, h, text->pString, font, &uX, &uY);
      } else {
        uX = tlx;
        uY = y;
      }
      MPrint(uX, uY, text->pString);
    }

    // there is secondary text in this line?
    const PopUpString *const second = box->pSecondColumnString[i];
    if (second) {
      // are we highlighting?...shading?..or neither
      if (second->fHighLightFlag) {
        SetFontForeground(second->ubHighLight);
      } else if (second->fShadeFlag) {
        SetFontForeground(second->ubShade);
      } else {
        SetFontForeground(second->ubForegroundColor);
      }

      SetFontBackground(second->ubBackgroundColor);

      const int32_t y = tly + i * (h + box->uiLineSpace);
      int16_t uX;
      int16_t uY;
      if (box->uiFlags & POPUP_BOX_FLAG_CENTER_TEXT) {
        FindFontCenterCoordinates(tlx, y, w, h, second->pString, font, &uX, &uY);
      } else {
        uX = tlx + box->uiSecondColumnCurrentOffset;
        uY = y;
      }
      MPrint(uX, uY, second->pString);
    }
  }

  if (box->uiBuffer != guiSAVEBUFFER) {
    InvalidateRegion(tlx - 1, tly, brx, bry);
  }

  SetFontDestBuffer(FRAME_BUFFER);
}

void ResizeBoxToText(PopUpBox *const box) {
  Font const font = box->font;
  uint32_t max_lw = 0;  // width of left  column
  uint32_t max_rw = 0;  // width of right column
  uint32_t i;
  for (i = 0; i < MAX_POPUP_BOX_STRING_COUNT; ++i) {
    const PopUpString *const l = box->Text[i];
    if (l == NULL) break;

    const uint32_t lw = StringPixLength(l->pString, font);
    if (lw > max_lw) max_lw = lw;

    const PopUpString *const r = box->pSecondColumnString[i];
    if (r != NULL) {
      const uint32_t rw = StringPixLength(r->pString, font);
      if (rw > max_rw) max_rw = rw;
    }
  }

  const uint32_t r_off = max_lw + box->uiSecondColumnMinimunOffset;
  box->uiSecondColumnCurrentOffset = r_off;

  uint32_t w = box->uiLeftMargin + r_off + max_rw + box->uiRightMargin;
  if (w < box->uiBoxMinWidth) w = box->uiBoxMinWidth;
  box->pos.w = w;

  box->pos.h =
      box->uiTopMargin + i * (GetFontHeight(font) + box->uiLineSpace) + box->uiBottomMargin;
}

BOOLEAN IsBoxShown(const PopUpBox *const box) {
  if (box == NULL) return FALSE;
  return box->fShowBox;
}

void MarkAllBoxesAsAltered() {
  FOR_EACH_POPUP_BOX(i) { ForceUpDateOfBox(*i); }
}

void HideAllBoxes() {
  FOR_EACH_POPUP_BOX(i) { HideBox(*i); }
}

static void RemoveBoxPrimaryText(PopUpBox *const Box, const int32_t hStringHandle) {
  Assert(Box != NULL);
  Assert(hStringHandle < MAX_POPUP_BOX_STRING_COUNT);

  // remove & release primary text
  if (Box->Text[hStringHandle] != NULL) {
    if (Box->Text[hStringHandle]->pString) {
      MemFree(Box->Text[hStringHandle]->pString);
    }

    MemFree(Box->Text[hStringHandle]);
    Box->Text[hStringHandle] = NULL;
  }
}

static void RemoveBoxSecondaryText(PopUpBox *const Box, const int32_t hStringHandle) {
  Assert(Box != NULL);
  Assert(hStringHandle < MAX_POPUP_BOX_STRING_COUNT);

  // remove & release secondary strings
  if (Box->pSecondColumnString[hStringHandle] != NULL) {
    if (Box->pSecondColumnString[hStringHandle]->pString) {
      MemFree(Box->pSecondColumnString[hStringHandle]->pString);
    }

    MemFree(Box->pSecondColumnString[hStringHandle]);
    Box->pSecondColumnString[hStringHandle] = NULL;
  }
}
