#include "Utils/AnimatedProgressBar.h"

#include <algorithm>
#include <wchar.h>

#include "Macro.h"
#include "SGP/Debug.h"
#include "SGP/Font.h"
#include "SGP/HImage.h"
#include "SGP/MemMan.h"
#include "SGP/Types.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "TileEngine/RenderDirty.h"
#include "TileEngine/SysUtil.h"
#include "Utils/FontControl.h"
#include "Utils/MusicControl.h"
#include "Utils/TimerControl.h"

#define MAX_PROGRESSBARS 4

enum ProgressBarFlags {
  PROGRESS_NONE = 0,
  PROGRESS_PANEL = 1 << 0,
  PROGRESS_DISPLAY_TEXT = 1 << 1,
  PROGRESS_USE_SAVEBUFFER = 1 << 2,  // use the save buffer when display the text
  PROGRESS_LOAD_BAR = 1 << 3
};
ENUM_BITSET(ProgressBarFlags)

struct PROGRESSBAR {
  ProgressBarFlags flags;
  SGPBox pos;
  uint16_t usPanelLeft, usPanelTop, usPanelRight, usPanelBottom;
  uint16_t usColor, usLtColor, usDkColor;
  wchar_t *swzTitle;
  Font usTitleFont;
  uint8_t ubTitleFontForeColor, ubTitleFontShadowColor;
  Font usMsgFont;
  uint8_t ubMsgFontForeColor, ubMsgFontShadowColor;
  uint32_t fill_colour;
  double rStart, rEnd;
  double rLastActual;
};

static PROGRESSBAR *pBar[MAX_PROGRESSBARS];

void CreateLoadingScreenProgressBar() {
  CreateProgressBar(0, 162, 427, 318, 16);
  pBar[0]->flags |= PROGRESS_LOAD_BAR;
}

void RemoveLoadingScreenProgressBar() {
  RemoveProgressBar(0);
  SetFontShadow(DEFAULT_SHADOW);
}

void CreateProgressBar(const uint8_t ubProgressBarID, const uint16_t x, const uint16_t y,
                       const uint16_t w, const uint16_t h) {
  PROGRESSBAR *const pNew = MALLOCZ(PROGRESSBAR);

  if (pBar[ubProgressBarID]) RemoveProgressBar(ubProgressBarID);

  pBar[ubProgressBarID] = pNew;
  // Assign coordinates
  pNew->flags = PROGRESS_NONE;
  pNew->pos.x = x;
  pNew->pos.y = y;
  pNew->pos.w = w;
  pNew->pos.h = h;
  // Init default data
  pNew->usMsgFont = FONT12POINT1;
  pNew->ubMsgFontForeColor = FONT_BLACK;
  pNew->ubMsgFontShadowColor = 0;
  SetRelativeStartAndEndPercentage(ubProgressBarID, 0, 100, NULL);
  pNew->swzTitle = NULL;

  // Default the progress bar's color to be red
  pNew->fill_colour = FROMRGB(150, 0, 0);
}

// You may also define a panel to go in behind the progress bar.  You can now
// assign a title to go with the panel.
void DefineProgressBarPanel(uint32_t ubID, uint8_t r, uint8_t g, uint8_t b, uint16_t usLeft,
                            uint16_t usTop, uint16_t usRight, uint16_t usBottom) {
  PROGRESSBAR *pCurr;
  Assert(ubID < MAX_PROGRESSBARS);
  pCurr = pBar[ubID];
  if (!pCurr) return;

  pCurr->flags |= PROGRESS_PANEL;
  pCurr->usPanelLeft = usLeft;
  pCurr->usPanelTop = usTop;
  pCurr->usPanelRight = usRight;
  pCurr->usPanelBottom = usBottom;
  pCurr->usColor = Get16BPPColor(FROMRGB(r, g, b));
  // Calculate the slightly lighter and darker versions of the same rgb color
  pCurr->usLtColor = Get16BPPColor(FROMRGB((uint8_t)std::min((uint16_t)255, (uint16_t)(r * 1.33)),
                                           (uint8_t)std::min((uint16_t)255, (uint16_t)(g * 1.33)),
                                           (uint8_t)std::min((uint16_t)255, (uint16_t)(b * 1.33))));
  pCurr->usDkColor =
      Get16BPPColor(FROMRGB((uint8_t)(r * 0.75), (uint8_t)(g * 0.75), (uint8_t)(b * 0.75)));
}

// Assigning a title for the panel will automatically position the text
// horizontally centered on the panel and vertically centered from the top of the
// panel, to the top of the progress bar.
void SetProgressBarTitle(uint32_t ubID, const wchar_t *pString, Font const font,
                         uint8_t ubForeColor, uint8_t ubShadowColor) {
  PROGRESSBAR *pCurr;
  Assert(ubID < MAX_PROGRESSBARS);
  pCurr = pBar[ubID];
  if (!pCurr) return;
  if (pCurr->swzTitle) {
    MemFree(pCurr->swzTitle);
    pCurr->swzTitle = NULL;
  }
  if (pString && wcslen(pString)) {
    pCurr->swzTitle = MALLOCN(wchar_t, wcslen(pString) + 1);
    wcscpy(pCurr->swzTitle, pString);
  }
  pCurr->usTitleFont = font;
  pCurr->ubTitleFontForeColor = ubForeColor;
  pCurr->ubTitleFontShadowColor = ubShadowColor;
}

// Unless you set up the attributes, any text you pass to
// SetRelativeStartAndEndPercentage will default to FONT12POINT1 in a black
// color.
void SetProgressBarMsgAttributes(uint32_t ubID, Font const font, uint8_t ubForeColor,
                                 uint8_t ubShadowColor) {
  PROGRESSBAR *pCurr;
  Assert(ubID < MAX_PROGRESSBARS);
  pCurr = pBar[ubID];
  if (!pCurr) return;
  pCurr->usMsgFont = font;
  pCurr->ubMsgFontForeColor = ubForeColor;
  pCurr->ubMsgFontShadowColor = ubShadowColor;
}

// When finished, the progress bar needs to be removed.
void RemoveProgressBar(uint8_t ubID) {
  Assert(ubID < MAX_PROGRESSBARS);
  if (pBar[ubID]) {
    if (pBar[ubID]->swzTitle) MemFree(pBar[ubID]->swzTitle);
    MemFree(pBar[ubID]);
    pBar[ubID] = NULL;
    return;
  }
}

/* An important setup function.  The best explanation is through example.  The
 * example being the loading of a file -- there are many stages of the map
 * loading.  In JA2, the first step is to load the tileset.  Because it is a
 * large chunk of the total loading of the map, we may gauge that it takes up
 * 30% of the total load.  Because it is also at the beginning, we would pass in
 * the arguments (0, 30, "text").  As the process animates using
 * UpdateProgressBar(0 to 100), the total progress bar will only reach 30% at
 * the 100% mark within UpdateProgressBar.  At that time, you would go onto the
 * next step, resetting the relative start and end percentage from 30 to
 * whatever, until your done. */
void SetRelativeStartAndEndPercentage(uint8_t const id, uint32_t const uiRelStartPerc,
                                      uint32_t const uiRelEndPerc, wchar_t const *const str) {
  Assert(id < MAX_PROGRESSBARS);
  PROGRESSBAR *const bar = pBar[id];
  if (!bar) return;

  bar->rStart = uiRelStartPerc * 0.01;
  bar->rEnd = uiRelEndPerc * 0.01;

  // Render the entire panel now, as it doesn't need update during the normal
  // rendering
  if (bar->flags & PROGRESS_PANEL) {  // Draw panel
    uint16_t const l = bar->usPanelLeft;
    uint16_t const t = bar->usPanelTop;
    uint16_t const r = bar->usPanelRight;
    uint16_t const b = bar->usPanelBottom;
    ColorFillVideoSurfaceArea(FRAME_BUFFER, l, t, r, b, bar->usLtColor);
    ColorFillVideoSurfaceArea(FRAME_BUFFER, l + 1, t + 1, r, b, bar->usDkColor);
    ColorFillVideoSurfaceArea(FRAME_BUFFER, l + 1, t + 1, r - 1, b - 1, bar->usColor);
    InvalidateRegion(l, t, r, b);
    wchar_t const *const title = bar->swzTitle;
    if (title) {  // Draw title
      Font const font = bar->usTitleFont;
      int32_t const x = (r + l - StringPixLength(title, font)) / 2;  // Center
      SetFontAttributes(font, bar->ubTitleFontForeColor, bar->ubTitleFontShadowColor);
      MPrint(x, t + 3, title);
    }
  }

  if (bar->flags & PROGRESS_DISPLAY_TEXT && str) {  // Draw message
    int32_t const x = bar->pos.x;
    int32_t const y = bar->pos.y + bar->pos.h;
    Font const font = bar->usMsgFont;
    if (bar->flags & PROGRESS_USE_SAVEBUFFER) {
      uint16_t const h = GetFontHeight(font);
      RestoreExternBackgroundRect(x, y, bar->pos.w, h + 3);
    }

    SetFontAttributes(font, bar->ubMsgFontForeColor, bar->ubMsgFontShadowColor);
    MPrint(x, y + 3, str);
  }
}

// This part renders the progress bar at the percentage level that you specify.
// If you have set relative percentage values in the above function, then the
// uiPercentage will be reflected based off of the relative percentages.
void RenderProgressBar(uint8_t ubID, uint32_t uiPercentage) {
  static uint32_t uiLastTime = 0;
  uint32_t uiCurTime = GetJA2Clock();
  double rActual;
  PROGRESSBAR *pCurr = NULL;
  // uint32_t r, g;

  Assert(ubID < MAX_PROGRESSBARS);
  pCurr = pBar[ubID];

  if (pCurr == NULL) return;

  if (pCurr) {
    rActual = pCurr->rStart + (pCurr->rEnd - pCurr->rStart) * uiPercentage * 0.01;

    if (rActual - pCurr->rLastActual < 0.01) {
      return;
    }

    pCurr->rLastActual = (double)((int32_t)(rActual * 100) * 0.01);

    int32_t const x = pCurr->pos.x;
    int32_t const y = pCurr->pos.y;
    int32_t const w = pCurr->pos.w;
    int32_t const h = pCurr->pos.h;
    int32_t const end = (int32_t)(x + 2.0 + rActual * (w - 4));
    if (end < x + 2 || x + w - 2 < end) return;
    if (pCurr->flags & PROGRESS_LOAD_BAR) {
      ColorFillVideoSurfaceArea(FRAME_BUFFER, x, y, end, y + h, Get16BPPColor(pCurr->fill_colour));
    } else {
      // Border edge of the progress bar itself in gray
      ColorFillVideoSurfaceArea(FRAME_BUFFER, x, y, x + w, y + h,
                                Get16BPPColor(FROMRGB(160, 160, 160)));
      // Interior of progress bar in black
      ColorFillVideoSurfaceArea(FRAME_BUFFER, x + 2, y + 2, x + w - 2, y + h - 2,
                                Get16BPPColor(FROMRGB(0, 0, 0)));
      ColorFillVideoSurfaceArea(FRAME_BUFFER, x + 2, y + 2, end, y + h - 2,
                                Get16BPPColor(FROMRGB(72, 155, 24)));
    }
    InvalidateRegion(x, y, x + w, y + h);
    ExecuteBaseDirtyRectQueue();
    EndFrameBufferRender();
    RefreshScreen();
  }

  // update music here
  if (uiCurTime > (uiLastTime + 200)) {
    MusicPoll();
    uiLastTime = GetJA2Clock();
  }
}

void SetProgressBarColor(uint8_t ubID, uint8_t ubColorFillRed, uint8_t ubColorFillGreen,
                         uint8_t ubColorFillBlue) {
  PROGRESSBAR *pCurr = NULL;

  Assert(ubID < MAX_PROGRESSBARS);

  pCurr = pBar[ubID];
  if (pCurr == NULL) return;

  pCurr->fill_colour = FROMRGB(ubColorFillRed, ubColorFillGreen, ubColorFillBlue);
}

void SetProgressBarTextDisplayFlag(uint8_t ubID, BOOLEAN fDisplayText, BOOLEAN fUseSaveBuffer,
                                   BOOLEAN fSaveScreenToFrameBuffer) {
  PROGRESSBAR *pCurr = NULL;

  Assert(ubID < MAX_PROGRESSBARS);

  pCurr = pBar[ubID];
  if (pCurr == NULL) return;

  ProgressBarFlags flags = pCurr->flags & ~(PROGRESS_DISPLAY_TEXT | PROGRESS_USE_SAVEBUFFER);
  if (fDisplayText) flags |= PROGRESS_DISPLAY_TEXT;
  if (fUseSaveBuffer) flags |= PROGRESS_USE_SAVEBUFFER;
  pCurr->flags = flags;

  // if we are to use the save buffer, blit the portion of the screen to the
  // save buffer
  if (fSaveScreenToFrameBuffer) {
    uint16_t usFontHeight = GetFontHeight(pCurr->usMsgFont) + 3;

    // blit everything to the save buffer ( cause the save buffer can bleed
    // through )
    BlitBufferToBuffer(FRAME_BUFFER, guiSAVEBUFFER, pCurr->pos.x, pCurr->pos.y + pCurr->pos.h,
                       pCurr->pos.w, usFontHeight);
  }
}
