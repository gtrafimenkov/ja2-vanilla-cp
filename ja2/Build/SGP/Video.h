#ifndef VIDEO_H
#define VIDEO_H

#include "SGP/Types.h"

#define VIDEO_NO_CURSOR 0xFFFF
#define GAME_WINDOW g_game_window

struct SDL_Window;
extern SDL_Window *g_game_window;

void VideoSetFullScreen(BOOLEAN enable);
void InitializeVideoManager();
void ShutdownVideoManager();
void SuspendVideoManager();
BOOLEAN RestoreVideoManager();
void InvalidateRegion(INT32 iLeft, INT32 iTop, INT32 iRight, INT32 iBottom);
void InvalidateScreen();
void GetPrimaryRGBDistributionMasks(UINT32 *RedBitMask, UINT32 *GreenBitMask, UINT32 *BlueBitMask);
void EndFrameBufferRender();
void PrintScreen();

/* Toggle between fullscreen and window mode after initialising the video
 * manager */
void VideoToggleFullScreen();

void SetMouseCursorProperties(INT16 sOffsetX, INT16 sOffsetY, UINT16 usCursorHeight,
                              UINT16 usCursorWidth);

void VideoCaptureToggle();

void InvalidateRegionEx(INT32 iLeft, INT32 iTop, INT32 iRight, INT32 iBottom);

void RefreshScreen();

// Creates a list to contain video Surfaces
void InitializeVideoSurfaceManager();

// Deletes any video Surface placed into list
void ShutdownVideoSurfaceManager();

#endif
