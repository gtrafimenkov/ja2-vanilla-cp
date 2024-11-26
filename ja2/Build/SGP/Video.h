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
void InvalidateRegion(int32_t iLeft, int32_t iTop, int32_t iRight, int32_t iBottom);
void InvalidateScreen();
void GetPrimaryRGBDistributionMasks(uint32_t *RedBitMask, uint32_t *GreenBitMask,
                                    uint32_t *BlueBitMask);
void EndFrameBufferRender();
void PrintScreen();

/* Toggle between fullscreen and window mode after initialising the video
 * manager */
void VideoToggleFullScreen();

void SetMouseCursorProperties(int16_t sOffsetX, int16_t sOffsetY, uint16_t usCursorHeight,
                              uint16_t usCursorWidth);

void VideoCaptureToggle();

void InvalidateRegionEx(int32_t iLeft, int32_t iTop, int32_t iRight, int32_t iBottom);

void RefreshScreen();

// Creates a list to contain video Surfaces
void InitializeVideoSurfaceManager();

// Deletes any video Surface placed into list
void ShutdownVideoSurfaceManager();

#endif
