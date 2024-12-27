// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef FADE_SCREEN_H
#define FADE_SCREEN_H

#include "SGP/Types.h"
#include "ScreenIDs.h"

#define FADE_OUT_REALFADE 5

#define FADE_IN_REALFADE 12

typedef void (*FADE_HOOK)();

extern FADE_HOOK gFadeInDoneCallback;
extern FADE_HOOK gFadeOutDoneCallback;

typedef void (*FADE_FUNCTION)();

extern BOOLEAN gfFadeInitialized;
extern BOOLEAN gfFadeIn;
extern FADE_FUNCTION gFadeFunction;
extern BOOLEAN gfFadeInVideo;

BOOLEAN HandleBeginFadeIn(ScreenID uiScreenExit);
BOOLEAN HandleBeginFadeOut(ScreenID uiScreenExit);

BOOLEAN HandleFadeOutCallback();
BOOLEAN HandleFadeInCallback();

void FadeInNextFrame();
void FadeOutNextFrame();

ScreenID FadeScreenHandle();

#endif
