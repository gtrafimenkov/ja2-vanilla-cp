#ifndef __JASCREENS_H_
#define __JASCREENS_H_

#include "SGP/Types.h"
#include "ScreenIDs.h"

ScreenID ErrorScreenHandle();

ScreenID InitScreenHandle();

ScreenID PalEditScreenHandle();

ScreenID DebugScreenHandle();

ScreenID SexScreenHandle();

// External functions
void DisplayFrameRate();

void HandleTitleScreenAnimation();

// External Globals
extern ScreenID guiCurrentScreen;

typedef void (*RENDER_HOOK)();

void SetRenderHook(RENDER_HOOK pRenderOverride);
void SetDebugRenderHook(RENDER_HOOK pDebugRenderOverride, int8_t ubPage);

void EnableFPSOverlay(BOOLEAN fEnable);

extern BOOLEAN gfExitDebugScreen;
extern int8_t gCurDebugPage;

#endif
