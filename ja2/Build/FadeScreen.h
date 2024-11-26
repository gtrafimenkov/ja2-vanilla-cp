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
