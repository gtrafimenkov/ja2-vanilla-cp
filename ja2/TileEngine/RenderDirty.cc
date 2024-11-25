// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "TileEngine/RenderDirty.h"

#include <algorithm>
#include <cstring>
#include <stdarg.h>
#include <stdexcept>
#include <vector>

#include "Local.h"
#include "Macro.h"
#include "SGP/Debug.h"
#include "SGP/Font.h"
#include "SGP/MemMan.h"
#include "SGP/VObjectBlitters.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "TileEngine/RenderWorld.h"
#include "TileEngine/SysUtil.h"
#include "TileEngine/WorldDef.h"
#include "slog/slog.h"
#define TAG "Render"

#define BACKGROUND_BUFFERS 500

// Struct for backgrounds
struct BACKGROUND_SAVE {
  BOOLEAN fAllocated;
  BOOLEAN fFilled;
  BOOLEAN fFreeMemory;
  BackgroundFlags uiFlags;
  uint16_t *pSaveArea;
  uint16_t *pZSaveArea;
  int16_t sLeft;
  int16_t sTop;
  int16_t sRight;
  int16_t sBottom;
  int16_t sWidth;
  int16_t sHeight;
  BOOLEAN fPendingDelete;
  BOOLEAN fDisabled;
};

static std::vector<BACKGROUND_SAVE *> gBackSaves;
static uint32_t guiNumBackSaves = 0;

static VIDEO_OVERLAY *gVideoOverlays;

#define FOR_EACH_VIDEO_OVERLAY(iter)                                  \
  for (VIDEO_OVERLAY *iter = gVideoOverlays; iter; iter = iter->next) \
    if (iter->fDisabled)                                              \
      continue;                                                       \
    else

#define FOR_EACH_VIDEO_OVERLAY_SAFE(iter)                                              \
  for (VIDEO_OVERLAY *iter = gVideoOverlays, *iter##__next; iter; iter = iter##__next) \
    if (iter##__next = iter->next, iter->fDisabled)                                    \
      continue;                                                                        \
    else

SGPRect gDirtyClipRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};

static BOOLEAN gfViewportDirty = FALSE;

void AddBaseDirtyRect(int32_t iLeft, int32_t iTop, int32_t iRight, int32_t iBottom) {
  if (iLeft < 0) iLeft = 0;
  if (iLeft > SCREEN_WIDTH) iLeft = SCREEN_WIDTH;

  if (iTop < 0) iTop = 0;
  if (iTop > SCREEN_HEIGHT) iTop = SCREEN_HEIGHT;

  if (iRight < 0) iRight = 0;
  if (iRight > SCREEN_WIDTH) iRight = SCREEN_WIDTH;

  if (iBottom < 0) iBottom = 0;
  if (iBottom > SCREEN_HEIGHT) iBottom = SCREEN_HEIGHT;

  if (iLeft == iRight || iTop == iBottom) return;

  if (iLeft == gsVIEWPORT_START_X && iRight == gsVIEWPORT_END_X &&
      iTop == gsVIEWPORT_WINDOW_START_Y && iBottom == gsVIEWPORT_WINDOW_END_Y) {
    gfViewportDirty = TRUE;
    return;
  }

  InvalidateRegionEx(iLeft, iTop, iRight, iBottom);
}

void ExecuteBaseDirtyRectQueue() {
  if (!gfViewportDirty) return;
  gfViewportDirty = FALSE;

  InvalidateScreen();
}

static BACKGROUND_SAVE *GetFreeBackgroundBuffer() {
  for (uint32_t i = 0; i < guiNumBackSaves; ++i) {
    BACKGROUND_SAVE *const b = gBackSaves[i];
    if (!b->fAllocated && !b->fFilled) return b;
  }

  if (guiNumBackSaves == gBackSaves.size()) {
    // out of back saves capacity
    // let's add some more
    const int increment = 100;
    SLOGI(TAG, "Increasing background slots to %d", gBackSaves.size() + increment);
    for (int i = 0; i < increment; i++) {
      gBackSaves.push_back(new BACKGROUND_SAVE());
    }
  }

  return gBackSaves[guiNumBackSaves++];
}

BACKGROUND_SAVE *RegisterBackgroundRect(BackgroundFlags const uiFlags, int16_t sLeft, int16_t sTop,
                                        int16_t const usWidth, int16_t const usHeight) {
  const int32_t ClipX1 = gDirtyClipRect.iLeft;
  const int32_t ClipY1 = gDirtyClipRect.iTop;
  const int32_t ClipX2 = gDirtyClipRect.iRight;
  const int32_t ClipY2 = gDirtyClipRect.iBottom;

  int16_t sRight = sLeft + usWidth;
  int16_t sBottom = sTop + usHeight;

  const int32_t iTempX = sLeft;
  const int32_t iTempY = sTop;

  // Clip to rect
  const int32_t uiLeftSkip = std::min(ClipX1 - std::min(ClipX1, iTempX), (int32_t)usWidth);
  const int32_t uiTopSkip = std::min(ClipY1 - std::min(ClipY1, iTempY), (int32_t)usHeight);
  const int32_t uiRightSkip =
      std::min(std::max(ClipX2, iTempX + (int32_t)usWidth) - ClipX2, (int32_t)usWidth);
  const int32_t uiBottomSkip =
      std::min(std::max(ClipY2, iTempY + (int32_t)usHeight) - ClipY2, (int32_t)usHeight);

  // check if whole thing is clipped
  if (uiLeftSkip >= (int32_t)usWidth || uiRightSkip >= (int32_t)usWidth) return NO_BGND_RECT;
  if (uiTopSkip >= (int32_t)usHeight || uiBottomSkip >= (int32_t)usHeight) return NO_BGND_RECT;

  // Set re-set values given based on clipping
  sLeft += uiLeftSkip;
  sRight -= uiRightSkip;
  sTop += uiTopSkip;
  sBottom -= uiBottomSkip;

  BACKGROUND_SAVE *const b = GetFreeBackgroundBuffer();
  memset(b, 0, sizeof(*b));

  const uint32_t uiBufSize = (sRight - sLeft) * (sBottom - sTop);
  if (uiBufSize == 0) return NO_BGND_RECT;

  if (uiFlags & BGND_FLAG_SAVERECT) b->pSaveArea = MALLOCN(uint16_t, uiBufSize);
  if (uiFlags & BGND_FLAG_SAVE_Z) b->pZSaveArea = MALLOCN(uint16_t, uiBufSize);

  b->fFreeMemory = TRUE;
  b->fAllocated = TRUE;
  b->uiFlags = uiFlags;
  b->sLeft = sLeft;
  b->sTop = sTop;
  b->sRight = sRight;
  b->sBottom = sBottom;
  b->sWidth = sRight - sLeft;
  b->sHeight = sBottom - sTop;
  b->fFilled = FALSE;

  return b;
}

void RegisterBackgroundRectSingleFilled(int16_t const x, int16_t const y, int16_t const w,
                                        int16_t const h) {
  BACKGROUND_SAVE *const b = RegisterBackgroundRect(BGND_FLAG_SINGLE, x, y, w, h);
  if (b == NO_BGND_RECT) return;

  b->fFilled = TRUE;
  AddBaseDirtyRect(b->sLeft, b->sTop, b->sRight, b->sBottom);
}

void RestoreBackgroundRects() {
  {
    SGPVSurface::Lock lsrc(guiSAVEBUFFER);
    SGPVSurface::Lock ldst(FRAME_BUFFER);
    uint16_t *const pSrcBuf = lsrc.Buffer<uint16_t>();
    uint32_t uiSrcPitchBYTES = lsrc.Pitch();
    uint16_t *const pDestBuf = ldst.Buffer<uint16_t>();
    uint32_t uiDestPitchBYTES = ldst.Pitch();

    for (uint32_t i = 0; i < guiNumBackSaves; ++i) {
      const BACKGROUND_SAVE *const b = gBackSaves[i];
      if (!b->fFilled || b->fDisabled) continue;

      if (b->pSaveArea != NULL) {
        Blt16BPPTo16BPP(pDestBuf, uiDestPitchBYTES, b->pSaveArea, b->sWidth * 2, b->sLeft, b->sTop,
                        0, 0, b->sWidth, b->sHeight);
        AddBaseDirtyRect(b->sLeft, b->sTop, b->sRight, b->sBottom);
      } else if (b->pZSaveArea != NULL) {
        Blt16BPPTo16BPP(gpZBuffer, uiDestPitchBYTES, b->pZSaveArea, b->sWidth * 2, b->sLeft,
                        b->sTop, 0, 0, b->sWidth, b->sHeight);
      } else {
        Blt16BPPTo16BPP(pDestBuf, uiDestPitchBYTES, pSrcBuf, uiSrcPitchBYTES, b->sLeft, b->sTop,
                        b->sLeft, b->sTop, b->sWidth, b->sHeight);
        AddBaseDirtyRect(b->sLeft, b->sTop, b->sRight, b->sBottom);
      }
    }
  }

  EmptyBackgroundRects();
}

void EmptyBackgroundRects() {
  for (uint32_t i = 0; i < guiNumBackSaves; ++i) {
    BACKGROUND_SAVE *const b = gBackSaves[i];
    if (b->fFilled) {
      b->fFilled = FALSE;

      if (!b->fAllocated && b->fFreeMemory) {
        if (b->pSaveArea != NULL) MemFree(b->pSaveArea);
        if (b->pZSaveArea != NULL) MemFree(b->pZSaveArea);

        b->fAllocated = FALSE;
        b->fFreeMemory = FALSE;
        b->fFilled = FALSE;
        b->pSaveArea = NULL;
      }
    }

    if (b->uiFlags & BGND_FLAG_SINGLE || b->fPendingDelete) {
      if (b->fFreeMemory) {
        if (b->pSaveArea != NULL) MemFree(b->pSaveArea);
        if (b->pZSaveArea != NULL) MemFree(b->pZSaveArea);
      }

      b->fAllocated = FALSE;
      b->fFreeMemory = FALSE;
      b->fFilled = FALSE;
      b->pSaveArea = NULL;
      b->fPendingDelete = FALSE;
    }
  }
}

void SaveBackgroundRects() {
  SGPVSurface::Lock l(FRAME_BUFFER);
  uint16_t *const pSrcBuf = l.Buffer<uint16_t>();
  uint32_t const uiDestPitchBYTES = l.Pitch();

  for (uint32_t i = 0; i < guiNumBackSaves; ++i) {
    BACKGROUND_SAVE *const b = gBackSaves[i];
    if (!b->fAllocated || b->fDisabled) continue;

    if (b->pSaveArea != NULL) {
      Blt16BPPTo16BPP(b->pSaveArea, b->sWidth * 2, pSrcBuf, uiDestPitchBYTES, 0, 0, b->sLeft,
                      b->sTop, b->sWidth, b->sHeight);
    } else if (b->pZSaveArea != NULL) {
      Blt16BPPTo16BPP(b->pZSaveArea, b->sWidth * 2, gpZBuffer, uiDestPitchBYTES, 0, 0, b->sLeft,
                      b->sTop, b->sWidth, b->sHeight);
    } else {
      AddBaseDirtyRect(b->sLeft, b->sTop, b->sRight, b->sBottom);
    }

    b->fFilled = TRUE;
  }
}

void FreeBackgroundRect(BACKGROUND_SAVE *const b) {
  if (b == NULL) return;

  b->fAllocated = FALSE;
}

void FreeBackgroundRectPending(BACKGROUND_SAVE *const b) { b->fPendingDelete = TRUE; }

static void FreeBackgroundRectNow(BACKGROUND_SAVE *const b) {
  if (b->fFreeMemory) {
    if (b->pSaveArea) MemFree(b->pSaveArea);
    if (b->pZSaveArea) MemFree(b->pZSaveArea);
  }

  b->fAllocated = FALSE;
  b->fFreeMemory = FALSE;
  b->fFilled = FALSE;
  b->pSaveArea = NULL;
}

void FreeBackgroundRectType(BackgroundFlags const uiFlags) {
  for (uint32_t i = 0; i < guiNumBackSaves; ++i) {
    BACKGROUND_SAVE *const b = gBackSaves[i];
    if (b->uiFlags & uiFlags) FreeBackgroundRectNow(b);
  }
}

void InitializeBackgroundRects() { guiNumBackSaves = 0; }

void InvalidateBackgroundRects() {
  for (uint32_t i = 0; i < guiNumBackSaves; ++i) {
    gBackSaves[i]->fFilled = FALSE;
  }
}

void ShutdownBackgroundRects() {
  for (uint32_t i = 0; i < guiNumBackSaves; ++i) {
    BACKGROUND_SAVE *const b = gBackSaves[i];
    if (b->fAllocated) FreeBackgroundRectNow(b);
  }
}

void UpdateSaveBuffer() {
  // Update saved buffer - do for the viewport size ony!
  BlitBufferToBuffer(FRAME_BUFFER, guiSAVEBUFFER, 0, gsVIEWPORT_WINDOW_START_Y, SCREEN_WIDTH,
                     gsVIEWPORT_WINDOW_END_Y - gsVIEWPORT_WINDOW_START_Y);
}

void RestoreExternBackgroundRect(const int16_t sLeft, const int16_t sTop, const int16_t sWidth,
                                 const int16_t sHeight) {
  Assert(0 <= sLeft && sLeft + sWidth <= SCREEN_WIDTH && 0 <= sTop &&
         sTop + sHeight <= SCREEN_HEIGHT);

  BlitBufferToBuffer(guiSAVEBUFFER, FRAME_BUFFER, sLeft, sTop, sWidth, sHeight);

  // Add rect to frame buffer queue
  InvalidateRegionEx(sLeft, sTop, sLeft + sWidth, sTop + sHeight);
}

void RestoreExternBackgroundRectGivenID(const BACKGROUND_SAVE *const b) {
  if (!b->fAllocated) return;
  RestoreExternBackgroundRect(b->sLeft, b->sTop, b->sWidth, b->sHeight);
}

/* Dirties a single-frame rect exactly the size needed to save the background
 * for a given call to gprintf. Note that this must be called before the
 * backgrounds are saved, and before the actual call to gprintf that writes to
 * the video buffer. */
static void GDirty(int16_t const x, int16_t const y, wchar_t const *const str) {
  uint16_t const length = StringPixLength(str, FontDefault);
  if (length > 0) {
    uint16_t const height = GetFontHeight(FontDefault);
    RegisterBackgroundRectSingleFilled(x, y, length, height);
  }
}

void GDirtyPrint(int16_t const x, int16_t const y,
                 wchar_t const *const str)  // XXX TODO0017
{
  GDirty(x, y, str);
  MPrint(x, y, str);
}

void GDirtyPrintF(int16_t const x, int16_t const y, wchar_t const *const fmt, ...) {
  wchar_t str[512];
  va_list ap;
  va_start(ap, fmt);
  vswprintf(str, lengthof(str), fmt, ap);
  va_end(ap);
  GDirtyPrint(x, y, str);
}

void GPrintDirty(int16_t const x, int16_t const y,
                 wchar_t const *const str)  // XXX TODO0017
{
  MPrint(x, y, str);
  GDirty(x, y, str);
}

void GPrintDirtyF(int16_t const x, int16_t const y, wchar_t const *const fmt, ...) {
  wchar_t str[512];
  va_list ap;
  va_start(ap, fmt);
  vswprintf(str, lengthof(str), fmt, ap);
  va_end(ap);
  GDirtyPrint(x, y, str);
}

void GPrintInvalidate(int16_t const x, int16_t const y, wchar_t const *const str) {
  MPrint(x, y, str);

  uint16_t const length = StringPixLength(str, FontDefault);
  if (length > 0) {
    uint16_t const height = GetFontHeight(FontDefault);
    InvalidateRegionEx(x, y, x + length, y + height);
  }
}

void GPrintInvalidateF(int16_t const x, int16_t const y, wchar_t const *const fmt, ...) {
  wchar_t str[512];
  va_list ap;
  va_start(ap, fmt);
  vswprintf(str, lengthof(str), fmt, ap);
  va_end(ap);
  GPrintInvalidate(x, y, str);
}

VIDEO_OVERLAY *RegisterVideoOverlay(OVERLAY_CALLBACK const callback, int16_t const x,
                                    int16_t const y, int16_t const w, int16_t const h) try {
  BACKGROUND_SAVE *const bgs = RegisterBackgroundRect(BGND_FLAG_PERMANENT, x, y, w, h);
  if (!bgs) return 0;

  VIDEO_OVERLAY *const v = MALLOCZ(VIDEO_OVERLAY);
  VIDEO_OVERLAY *const head = gVideoOverlays;
  v->prev = 0;
  v->next = head;
  v->fAllocated = 2;
  v->background = bgs;
  v->sX = x;
  v->sY = y;
  v->uiDestBuff = FRAME_BUFFER;
  v->BltCallback = callback;

  if (head) head->prev = v;
  gVideoOverlays = v;
  return v;
} catch (...) {
  return 0;
}

VIDEO_OVERLAY *RegisterVideoOverlay(OVERLAY_CALLBACK const callback, int16_t const x,
                                    int16_t const y, Font const font, uint8_t const foreground,
                                    uint8_t const background, wchar_t const *const text) {
  int16_t const w = StringPixLength(text, font);
  int16_t const h = GetFontHeight(font);
  VIDEO_OVERLAY *const v = RegisterVideoOverlay(callback, x, y, w, h);
  if (v) {
    v->uiFontID = font;
    v->ubFontFore = foreground;
    v->ubFontBack = background;
    wcsncpy(v->zText, text, lengthof(v->zText));
  }
  return v;
}

void RemoveVideoOverlay(VIDEO_OVERLAY *const v) {
  if (!v) return;

  // Check if we are actively scrolling
  if (v->fActivelySaving) {
    v->fDeletionPending = TRUE;
  } else {
    // RestoreExternBackgroundRectGivenID(v->background);

    FreeBackgroundRect(v->background);

    if (v->pSaveArea != NULL) MemFree(v->pSaveArea);
    v->pSaveArea = NULL;

    VIDEO_OVERLAY *const prev = v->prev;
    VIDEO_OVERLAY *const next = v->next;
    *(prev ? &prev->next : &gVideoOverlays) = next;
    if (next) next->prev = prev;
    MemFree(v);
  }
}

// FUnctions for entrie array of blitters
void ExecuteVideoOverlays() {
  FOR_EACH_VIDEO_OVERLAY(v) {
    // If we are scrolling but haven't saved yet, don't!
    if (!v->fActivelySaving && g_scroll_inertia) continue;

    // ATE: Wait a frame before executing!
    switch (v->fAllocated) {
      case 1:
        v->BltCallback(v);
        break;
      case 2:
        v->fAllocated = 1;
        break;
    }
  }
}

void ExecuteVideoOverlaysToAlternateBuffer(SGPVSurface *const buffer) {
  FOR_EACH_VIDEO_OVERLAY(v) {
    if (!v->fActivelySaving) continue;

    SGPVSurface *const old_dst = v->uiDestBuff;
    v->uiDestBuff = buffer;
    v->BltCallback(v);
    v->uiDestBuff = old_dst;
  }
}

static void AllocateVideoOverlayArea(VIDEO_OVERLAY *const v) {
  Assert(!v->fDisabled);

  // Get buffer size
  const BACKGROUND_SAVE *const bgs = v->background;
  uint32_t const buf_size = (bgs->sRight - bgs->sLeft) * (bgs->sBottom - bgs->sTop);

  v->fActivelySaving = TRUE;
  v->pSaveArea = MALLOCN(uint16_t, buf_size);
}

void AllocateVideoOverlaysArea() {
  FOR_EACH_VIDEO_OVERLAY(v) { AllocateVideoOverlayArea(v); }
}

void SaveVideoOverlaysArea(SGPVSurface *const src) {
  SGPVSurface::Lock l(src);
  uint16_t *const pSrcBuf = l.Buffer<uint16_t>();
  uint32_t const uiSrcPitchBYTES = l.Pitch();

  FOR_EACH_VIDEO_OVERLAY(v) {
    // OK, if our saved area is null, allocate it here!
    if (v->pSaveArea == NULL) {
      AllocateVideoOverlayArea(v);
      if (v->pSaveArea == NULL) continue;
    }

    // Save data from frame buffer!
    const BACKGROUND_SAVE *const b = v->background;
    Blt16BPPTo16BPP(v->pSaveArea, b->sWidth * 2, pSrcBuf, uiSrcPitchBYTES, 0, 0, b->sLeft, b->sTop,
                    b->sWidth, b->sHeight);
  }
}

void DeleteVideoOverlaysArea() {
  FOR_EACH_VIDEO_OVERLAY_SAFE(v) {
    if (v->pSaveArea != NULL) MemFree(v->pSaveArea);
    v->pSaveArea = NULL;
    v->fActivelySaving = FALSE;
    if (v->fDeletionPending) RemoveVideoOverlay(v);
  }
}

void RestoreShiftedVideoOverlays(const int16_t sShiftX, const int16_t sShiftY) {
  const int32_t ClipX1 = 0;
  const int32_t ClipY1 = gsVIEWPORT_WINDOW_START_Y;
  const int32_t ClipX2 = SCREEN_WIDTH;
  const int32_t ClipY2 = gsVIEWPORT_WINDOW_END_Y - 1;

  SGPVSurface::Lock l(BACKBUFFER);
  uint16_t *const pDestBuf = l.Buffer<uint16_t>();
  uint32_t const uiDestPitchBYTES = l.Pitch();

  FOR_EACH_VIDEO_OVERLAY_SAFE(v) {
    if (v->pSaveArea == NULL) continue;

    // Get restore background values
    const BACKGROUND_SAVE *const b = v->background;
    int16_t sLeft = b->sLeft;
    int16_t sTop = b->sTop;
    int16_t sRight = b->sRight;
    int16_t sBottom = b->sBottom;
    uint32_t usHeight = b->sHeight;
    uint32_t usWidth = b->sWidth;

    // Clip!!
    const int32_t iTempX = sLeft - sShiftX;
    const int32_t iTempY = sTop - sShiftY;

    // Clip to rect
    const int32_t uiLeftSkip = std::min(ClipX1 - std::min(ClipX1, iTempX), (int32_t)usWidth);
    const int32_t uiTopSkip = std::min(ClipY1 - std::min(ClipY1, iTempY), (int32_t)usHeight);
    const int32_t uiRightSkip =
        std::min(std::max(ClipX2, iTempX + (int32_t)usWidth) - ClipX2, (int32_t)usWidth);
    const int32_t uiBottomSkip =
        std::min(std::max(ClipY2, iTempY + (int32_t)usHeight) - ClipY2, (int32_t)usHeight);

    // check if whole thing is clipped
    if (uiLeftSkip >= (int32_t)usWidth || uiRightSkip >= (int32_t)usWidth) continue;
    if (uiTopSkip >= (int32_t)usHeight || uiBottomSkip >= (int32_t)usHeight) continue;

    // Set re-set values given based on clipping
    sLeft = iTempX + (int16_t)uiLeftSkip;
    sTop = iTempY + (int16_t)uiTopSkip;
    sRight = sRight - sShiftX - (int16_t)uiRightSkip;
    sBottom = sBottom - sShiftY - (int16_t)uiBottomSkip;

    usHeight = sBottom - sTop;
    usWidth = sRight - sLeft;

    Blt16BPPTo16BPP(pDestBuf, uiDestPitchBYTES, v->pSaveArea, b->sWidth * 2, sLeft, sTop,
                    uiLeftSkip, uiTopSkip, usWidth, usHeight);

    // Once done, check for pending deletion
    if (v->fDeletionPending) RemoveVideoOverlay(v);
  }
}

void BlitBufferToBuffer(SGPVSurface *const src, SGPVSurface *const dst, const uint16_t usSrcX,
                        const uint16_t usSrcY, const uint16_t usWidth, const uint16_t usHeight) {
  SGPBox const r = {usSrcX, usSrcY, usWidth, usHeight};
  BltVideoSurface(dst, src, usSrcX, usSrcY, &r);
}

void EnableVideoOverlay(const BOOLEAN fEnable, VIDEO_OVERLAY *const v) {
  if (!v) return;
  v->fDisabled = !fEnable;
  v->background->fDisabled = !fEnable;
}

void SetVideoOverlayTextF(VIDEO_OVERLAY *const v, const wchar_t *Fmt, ...) {
  if (!v) return;
  va_list Arg;
  va_start(Arg, Fmt);
  vswprintf(v->zText, lengthof(v->zText), Fmt, Arg);
  va_end(Arg);
}

void SetVideoOverlayPos(VIDEO_OVERLAY *const v, const int16_t X, const int16_t Y) {
  if (!v) return;

  // If position has changed and there is text, adjust
  if (v->zText[0] != L'\0') {
    uint16_t uiStringLength = StringPixLength(v->zText, v->uiFontID);
    uint16_t uiStringHeight = GetFontHeight(v->uiFontID);

    // Delete old rect
    // Remove background
    FreeBackgroundRectPending(v->background);

    v->background =
        RegisterBackgroundRect(BGND_FLAG_PERMANENT, X, Y, uiStringLength, uiStringHeight);
    v->sX = X;
    v->sY = Y;
  }
}
