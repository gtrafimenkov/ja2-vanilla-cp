#ifndef __RENDER_DIRTY_H
#define __RENDER_DIRTY_H

#include "JA2Types.h"

#define NO_BGND_RECT NULL

enum BackgroundFlags {
  BGND_FLAG_NONE = 0,
  BGND_FLAG_PERMANENT = 0x80000000,
  BGND_FLAG_SINGLE = 0x40000000,
  BGND_FLAG_SAVE_Z = 0x20000000,
  BGND_FLAG_SAVERECT = 0x08000000,
  BGND_FLAG_ANIMATED = 0x00000001
};
ENUM_BITSET(BackgroundFlags)

// Callback for topmost blitters
typedef void (*OVERLAY_CALLBACK)(VIDEO_OVERLAY *);

// Struct for topmost blitters
struct VIDEO_OVERLAY {
  VIDEO_OVERLAY *prev;
  VIDEO_OVERLAY *next;
  BOOLEAN fAllocated;
  BOOLEAN fDisabled;
  BOOLEAN fActivelySaving;
  BOOLEAN fDeletionPending;
  BACKGROUND_SAVE *background;
  uint16_t *pSaveArea;
  Font uiFontID;
  int16_t sX;
  int16_t sY;
  uint8_t ubFontBack;
  uint8_t ubFontFore;
  wchar_t zText[200];
  SGPVSurface *uiDestBuff;
  OVERLAY_CALLBACK BltCallback;
};

// GLOBAL VARIABLES
extern SGPRect gDirtyClipRect;

// DIRTY QUEUE
void AddBaseDirtyRect(int32_t iLeft, int32_t iTop, int32_t iRight, int32_t iBottom);
void ExecuteBaseDirtyRectQueue();

// BACKGROUND RECT BUFFERING STUFF
void InitializeBackgroundRects();
void ShutdownBackgroundRects();
BACKGROUND_SAVE *RegisterBackgroundRect(BackgroundFlags, int16_t x, int16_t y, int16_t w,
                                        int16_t h);
void FreeBackgroundRect(BACKGROUND_SAVE *);
void FreeBackgroundRectPending(BACKGROUND_SAVE *);
void FreeBackgroundRectType(BackgroundFlags);
void RestoreBackgroundRects();
void SaveBackgroundRects();
void InvalidateBackgroundRects();
void UpdateSaveBuffer();
void RestoreExternBackgroundRect(int16_t sLeft, int16_t sTop, int16_t sWidth, int16_t sHeight);
void RegisterBackgroundRectSingleFilled(int16_t x, int16_t y, int16_t w, int16_t h);
void EmptyBackgroundRects();
void RestoreExternBackgroundRectGivenID(const BACKGROUND_SAVE *);

void GDirtyPrint(int16_t x, int16_t y, wchar_t const *str);
void GDirtyPrintF(int16_t x, int16_t y, wchar_t const *fmt, ...);

void GPrintDirty(int16_t x, int16_t y, wchar_t const *str);
void GPrintDirtyF(int16_t x, int16_t y, wchar_t const *fmt, ...);

void GPrintInvalidate(int16_t x, int16_t y, wchar_t const *str);
void GPrintInvalidateF(int16_t x, int16_t y, wchar_t const *fmt, ...);

// VIDEO OVERLAY STUFF
VIDEO_OVERLAY *RegisterVideoOverlay(OVERLAY_CALLBACK callback, int16_t x, int16_t y, int16_t w,
                                    int16_t h);
VIDEO_OVERLAY *RegisterVideoOverlay(OVERLAY_CALLBACK callback, int16_t x, int16_t y, Font font,
                                    uint8_t foreground, uint8_t background, wchar_t const *text);
void ExecuteVideoOverlays();
void SaveVideoOverlaysArea(SGPVSurface *src);

// Delete Topmost blitters saved areas
void DeleteVideoOverlaysArea();

void AllocateVideoOverlaysArea();
void ExecuteVideoOverlaysToAlternateBuffer(SGPVSurface *buffer);
void RemoveVideoOverlay(VIDEO_OVERLAY *);
void RestoreShiftedVideoOverlays(int16_t sShiftX, int16_t sShiftY);
void EnableVideoOverlay(BOOLEAN fEnable, VIDEO_OVERLAY *);
void SetVideoOverlayTextF(VIDEO_OVERLAY *, const wchar_t *fmt, ...);
void SetVideoOverlayPos(VIDEO_OVERLAY *, int16_t X, int16_t Y);

void BlitBufferToBuffer(SGPVSurface *src, SGPVSurface *dst, uint16_t usSrcX, uint16_t usSrcY,
                        uint16_t usWidth, uint16_t usHeight);

#endif
