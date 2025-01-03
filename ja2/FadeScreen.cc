// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "FadeScreen.h"

#include "GameLoop.h"
#include "Local.h"
#include "SGP/CursorControl.h"
#include "SGP/HImage.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "SysGlobals.h"
#include "TileEngine/RenderDirty.h"
#include "TileEngine/SysUtil.h"
#include "Utils/MusicControl.h"
#include "Utils/TimerControl.h"

static ScreenID guiExitScreen;
BOOLEAN gfFadeInitialized = FALSE;
int16_t gsFadeLimit;
uint32_t guiTime;
uint32_t guiFadeDelay;
BOOLEAN gfFirstTimeInFade = FALSE;
int16_t gsFadeCount;
static int8_t gbFadeType;
int16_t gsFadeRealCount;
BOOLEAN gfFadeInVideo;

FADE_FUNCTION gFadeFunction = NULL;

FADE_HOOK gFadeInDoneCallback = NULL;
FADE_HOOK gFadeOutDoneCallback = NULL;

BOOLEAN gfFadeIn = FALSE;
BOOLEAN gfFadeOut = FALSE;
BOOLEAN gfFadeOutDone = FALSE;
BOOLEAN gfFadeInDone = FALSE;

void FadeInNextFrame() {
  gfFadeIn = TRUE;
  gfFadeInDone = FALSE;
}

void FadeOutNextFrame() {
  gfFadeOut = TRUE;
  gfFadeOutDone = FALSE;
}

static void BeginFade(ScreenID uiExitScreen, int8_t bFadeValue, int8_t bType, uint32_t uiDelay);

BOOLEAN HandleBeginFadeIn(ScreenID const uiScreenExit) {
  if (gfFadeIn) {
    BeginFade(uiScreenExit, 35, FADE_IN_REALFADE, 5);

    gfFadeIn = FALSE;

    gfFadeInDone = TRUE;

    return (TRUE);
  }

  return (FALSE);
}

BOOLEAN HandleBeginFadeOut(ScreenID const uiScreenExit) {
  if (gfFadeOut) {
    BeginFade(uiScreenExit, 35, FADE_OUT_REALFADE, 5);

    gfFadeOut = FALSE;

    gfFadeOutDone = TRUE;

    return (TRUE);
  }

  return (FALSE);
}

BOOLEAN HandleFadeOutCallback() {
  if (gfFadeOutDone) {
    gfFadeOutDone = FALSE;

    if (gFadeOutDoneCallback != NULL) {
      gFadeOutDoneCallback();

      gFadeOutDoneCallback = NULL;

      return (TRUE);
    }
  }

  return (FALSE);
}

BOOLEAN HandleFadeInCallback() {
  if (gfFadeInDone) {
    gfFadeInDone = FALSE;

    if (gFadeInDoneCallback != NULL) {
      gFadeInDoneCallback();
    }

    gFadeInDoneCallback = NULL;

    return (TRUE);
  }

  return (FALSE);
}

static void FadeFrameBufferRealFade();
static void FadeInFrameBufferRealFade();

static void BeginFade(ScreenID const uiExitScreen, int8_t const bFadeValue, int8_t const bType,
                      uint32_t const uiDelay) {
  // Init some paramters
  guiExitScreen = uiExitScreen;
  guiFadeDelay = uiDelay;
  gfFadeIn = FALSE;
  gfFadeInVideo = TRUE;

  // Calculate step;
  switch (bType) {
    case FADE_IN_REALFADE:
      gsFadeRealCount = -1;
      gsFadeLimit = 8;
      gFadeFunction = FadeInFrameBufferRealFade;
      gfFadeInVideo = FALSE;

      BltVideoSurface(guiSAVEBUFFER, FRAME_BUFFER, 0, 0, NULL);
      FRAME_BUFFER->Fill(Get16BPPColor(FROMRGB(0, 0, 0)));
      break;

    case FADE_OUT_REALFADE:
      gsFadeRealCount = -1;
      gsFadeLimit = 10;
      gFadeFunction = FadeFrameBufferRealFade;
      gfFadeInVideo = FALSE;
      break;
  }

  gfFadeInitialized = TRUE;
  gfFirstTimeInFade = TRUE;
  gsFadeCount = 0;
  gbFadeType = bType;

  SetPendingNewScreen(FADE_SCREEN);
}

ScreenID FadeScreenHandle() {
  uint32_t uiTime;

  if (!gfFadeInitialized) {
    SET_ERROR("Fade Screen called but not intialized ");
    return (ERROR_SCREEN);
  }

  // ATE: Remove cursor
  SetCurrentCursorFromDatabase(VIDEO_NO_CURSOR);

  if (gfFirstTimeInFade) {
    gfFirstTimeInFade = FALSE;

    // Calcuate delay
    guiTime = GetJA2Clock();
  }

  // Get time
  uiTime = GetJA2Clock();

  MusicPoll();

  if ((uiTime - guiTime) > guiFadeDelay) {
    // Fade!
    if (!gfFadeIn) {
      // gFadeFunction( );
    }

    InvalidateScreen();

    if (!gfFadeInVideo) {
      gFadeFunction();
    }

    gsFadeCount++;

    if (gsFadeCount > gsFadeLimit) {
      switch (gbFadeType) {
        case FADE_OUT_REALFADE:
          FRAME_BUFFER->Fill(Get16BPPColor(FROMRGB(0, 0, 0)));
          break;
      }

      // End!
      gfFadeInitialized = FALSE;
      gfFadeIn = FALSE;

      return (guiExitScreen);
    }
  }

  return (FADE_SCREEN);
}

static void FadeFrameBufferRealFade() {
  if (gsFadeRealCount != gsFadeCount) {
    FRAME_BUFFER->ShadowRectUsingLowPercentTable(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    gsFadeRealCount = gsFadeCount;
  }
}

static void FadeInFrameBufferRealFade() {
  int32_t cnt;

  if (gsFadeRealCount != gsFadeCount) {
    for (cnt = 0; cnt < (gsFadeLimit - gsFadeCount); cnt++) {
      FRAME_BUFFER->ShadowRectUsingLowPercentTable(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    }

    RefreshScreen();

    // Copy save buffer back
    RestoreExternBackgroundRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    gsFadeRealCount = gsFadeCount;
  }
}
