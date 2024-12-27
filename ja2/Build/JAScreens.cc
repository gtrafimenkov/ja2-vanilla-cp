// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "JAScreens.h"

#include <algorithm>

#include "Directories.h"
#include "Editor/EditScreen.h"
#include "GameLoop.h"
#include "GameScreen.h"
#include "GameVersion.h"
#include "Init.h"
#include "Local.h"
#include "Macro.h"
#include "MainMenuScreen.h"
#include "SGP/CursorControl.h"
#include "SGP/Debug.h"
#include "SGP/English.h"
#include "SGP/Font.h"
#include "SGP/HImage.h"
#include "SGP/Input.h"
#include "SGP/MouseSystem.h"
#include "SGP/Random.h"
#include "SGP/SGP.h"
#include "SGP/Timer.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "Screens.h"
#include "Strategic/GameClock.h"
#include "Strategic/GameInit.h"
#include "SysGlobals.h"
#include "Tactical/AnimationCache.h"
#include "Tactical/HandleUI.h"
#include "Tactical/Overhead.h"
#include "TileEngine/Environment.h"
#include "TileEngine/RenderDirty.h"
#include "TileEngine/SysUtil.h"
#include "TileEngine/WorldDef.h"
#include "Utils/FontControl.h"
#include "Utils/MultiLanguageGraphicUtils.h"
#include "Utils/SoundControl.h"
#include "Utils/Text.h"
#include "Utils/TimerControl.h"
#include "Utils/Utilities.h"

#include "SDL_keycode.h"

#define MAX_DEBUG_PAGES 4

// GLOBAL FOR PAL EDITOR
uint8_t CurrentPalette = 0;
static BACKGROUND_SAVE *guiBackgroundRect = NO_BGND_RECT;
BOOLEAN gfExitPalEditScreen = FALSE;
BOOLEAN gfExitDebugScreen = FALSE;
static BOOLEAN FirstTime = TRUE;
BOOLEAN gfDoneWithSplashScreen = FALSE;

int8_t gCurDebugPage = 0;

static void DefaultDebugPage1();
static void DefaultDebugPage2();
static void DefaultDebugPage3();
static void DefaultDebugPage4();

RENDER_HOOK gDebugRenderOverride[MAX_DEBUG_PAGES] = {DefaultDebugPage1, DefaultDebugPage2,
                                                     DefaultDebugPage3, DefaultDebugPage4};

void DisplayFrameRate() {
  static uint32_t uiFPS = 0;
  static uint32_t uiFrameCount = 0;

  // Increment frame count
  uiFrameCount++;

  if (COUNTERDONE(FPSCOUNTER)) {
    // Reset counter
    RESETCOUNTER(FPSCOUNTER);

    uiFPS = uiFrameCount;
    uiFrameCount = 0;
  }

  if (gbFPSDisplay == SHOW_FULL_FPS) {
    // FRAME RATE
    SetVideoOverlayTextF(g_fps_overlay, L"%ld", std::min(uiFPS, (uint32_t)1000));

    // TIMER COUNTER
    SetVideoOverlayTextF(g_counter_period_overlay, L"%ld", std::min(giTimerDiag, 1000));
  }
}

ScreenID ErrorScreenHandle() {
  InputAtom InputEvent;
  static BOOLEAN fFirstTime = FALSE;

  // Create string
  SetFontAttributes(LARGEFONT1, FONT_MCOLOR_LTGRAY);
  MPrint(50, 200, L"RUNTIME ERROR");
  MPrint(50, 225, L"PRESS <ESC> TO EXIT");

  SetFontAttributes(FONT12ARIAL, FONT_YELLOW);
  mprintf(50, 255, L"%hs", gubErrorText);

  if (!fFirstTime) {
    DebugMsg(TOPIC_JA2, DBG_LEVEL_0, String("Runtime Error: %s ", gubErrorText));
    fFirstTime = TRUE;
  }

  // For quick setting of new video stuff / to be changed
  InvalidateScreen();

  // Check for esc
  while (DequeueEvent(&InputEvent)) {
    if (InputEvent.usEvent == KEY_DOWN) {
      if (InputEvent.usParam == SDLK_ESCAPE ||
          (InputEvent.usParam == 'x' && InputEvent.usKeyState & ALT_DOWN)) {  // Exit the program
        DebugMsg(TOPIC_GAME, DBG_LEVEL_0, "GameLoop: User pressed ESCape, TERMINATING");

        // handle shortcut exit
        HandleShortCutExitState();
      }
    }
  }

  return (ERROR_SCREEN);
}

ScreenID InitScreenHandle() {
  static uint32_t splashDisplayedMoment = 0;
  static uint8_t ubCurrentScreen = 255;

  if (ubCurrentScreen == 255) {
    if (isEnglishVersion()) {
      if (gfDoneWithSplashScreen) {
        ubCurrentScreen = 0;
      } else {
        SetCurrentCursorFromDatabase(VIDEO_NO_CURSOR);
        return (INTRO_SCREEN);
      }
    } else {
      ubCurrentScreen = 0;
    }
  }

  if (ubCurrentScreen == 0) {
    ubCurrentScreen = 1;

    // Init screen

    SetFontAttributes(TINYFONT1, FONT_MCOLOR_WHITE);

    const int32_t x = 10;
    const int32_t y = SCREEN_HEIGHT;

    mprintf(x, y - 50, L"%hs", g_version_label, g_version_number);

    InvalidateScreen();

    // ATE: Set to true to reset before going into main screen!

    SetCurrentCursorFromDatabase(VIDEO_NO_CURSOR);
    splashDisplayedMoment = GetClock();
    return (INIT_SCREEN);
  }

  if (ubCurrentScreen == 1) {
    ubCurrentScreen = 2;
    return (InitializeJA2());
  }

  if (ubCurrentScreen == 2) {
    // wait 3 seconds since the splash displayed and then switch
    // to the main menu
    if ((GetClock() - splashDisplayedMoment) >= 3000) {
      InitMainMenu();
      ubCurrentScreen = 3;
    }
    return (INIT_SCREEN);
  }

  // Let one frame pass....
  if (ubCurrentScreen == 3) {
    ubCurrentScreen = 4;
    SetCurrentCursorFromDatabase(VIDEO_NO_CURSOR);
    return (INIT_SCREEN);
  }

  if (ubCurrentScreen == 4) {
    SetCurrentCursorFromDatabase(VIDEO_NO_CURSOR);
    InitNewGame();
  }
  return (INIT_SCREEN);
}

static BOOLEAN PalEditKeyboardHook(InputAtom *pInputEvent);
static void PalEditRenderHook();

ScreenID PalEditScreenHandle() {
  static BOOLEAN FirstTime = TRUE;

  if (gfExitPalEditScreen) {
    gfExitPalEditScreen = FALSE;
    FirstTime = TRUE;
    FreeBackgroundRect(guiBackgroundRect);
    guiBackgroundRect = NO_BGND_RECT;
    SetRenderHook(NULL);
    SetUIKeyboardHook(NULL);
    return (GAME_SCREEN);
  }

  if (FirstTime) {
    FirstTime = FALSE;

    SetRenderHook(PalEditRenderHook);
    SetUIKeyboardHook(PalEditKeyboardHook);

    guiBackgroundRect = RegisterBackgroundRect(BGND_FLAG_PERMANENT, 50, 10, 550, 390);
  } else {
    (*(GameScreens[GAME_SCREEN].HandleScreen))();
  }

  return (PALEDIT_SCREEN);
}

static void PalEditRenderHook() {
  const SOLDIERTYPE *const sel = GetSelectedMan();
  if (sel != NULL) {
    // Set to current
    DisplayPaletteRep(sel->HeadPal, 50, 10, FRAME_BUFFER);
    DisplayPaletteRep(sel->PantsPal, 50, 50, FRAME_BUFFER);
    DisplayPaletteRep(sel->VestPal, 50, 90, FRAME_BUFFER);
    DisplayPaletteRep(sel->SkinPal, 50, 130, FRAME_BUFFER);
  }
}

static void CyclePaletteReplacement(SOLDIERTYPE &s, PaletteRepID pal) {
  uint8_t ubPaletteRep = GetPaletteRepIndexFromID(pal);
  const uint8_t ubType = gpPalRep[ubPaletteRep].ubType;

  ubPaletteRep++;

  // Count start and end index
  uint8_t ubStartRep = 0;
  for (uint32_t cnt = 0; cnt < ubType; ++cnt) {
    ubStartRep = ubStartRep + gubpNumReplacementsPerRange[cnt];
  }

  const uint8_t ubEndRep = ubStartRep + gubpNumReplacementsPerRange[ubType];

  if (ubPaletteRep == ubEndRep) ubPaletteRep = ubStartRep;
  SET_PALETTEREP_ID(pal, gpPalRep[ubPaletteRep].ID);

  CreateSoldierPalettes(s);
}

static BOOLEAN PalEditKeyboardHook(InputAtom *pInputEvent) {
  if (pInputEvent->usEvent != KEY_DOWN) return FALSE;

  SOLDIERTYPE *const sel = GetSelectedMan();
  if (sel == NULL) return FALSE;

  switch (pInputEvent->usParam) {
    case SDLK_ESCAPE:
      gfExitPalEditScreen = TRUE;
      break;

    case 'h':
      CyclePaletteReplacement(*sel, sel->HeadPal);
      break;
    case 'v':
      CyclePaletteReplacement(*sel, sel->VestPal);
      break;
    case 'p':
      CyclePaletteReplacement(*sel, sel->PantsPal);
      break;
    case 's':
      CyclePaletteReplacement(*sel, sel->SkinPal);
      break;

    default:
      return FALSE;
  }
  return TRUE;
}

static BOOLEAN CheckForAndExitTacticalDebug() {
  if (gfExitDebugScreen) {
    FirstTime = TRUE;
    gfExitDebugScreen = FALSE;
    FreeBackgroundRect(guiBackgroundRect);
    guiBackgroundRect = NO_BGND_RECT;
    SetRenderHook(NULL);
    SetUIKeyboardHook(NULL);

    return (TRUE);
  }

  return (FALSE);
}

static BOOLEAN DebugKeyboardHook(InputAtom *pInputEvent);
static void DebugRenderHook();

ScreenID DebugScreenHandle() {
  if (CheckForAndExitTacticalDebug()) {
    return (GAME_SCREEN);
  }

  if (guiBackgroundRect == NO_BGND_RECT) {
    guiBackgroundRect = RegisterBackgroundRect(BGND_FLAG_PERMANENT, 0, 0, 600, 360);
  }

  if (FirstTime) {
    FirstTime = FALSE;

    SetRenderHook(DebugRenderHook);
    SetUIKeyboardHook(DebugKeyboardHook);
  } else {
    (*(GameScreens[GAME_SCREEN].HandleScreen))();
  }

  return (DEBUG_SCREEN);
}

static void DebugRenderHook() { gDebugRenderOverride[gCurDebugPage](); }

static BOOLEAN DebugKeyboardHook(InputAtom *pInputEvent) {
  if (pInputEvent->usEvent == KEY_UP) {
    switch (pInputEvent->usParam) {
      case 'q':
        gfExitDebugScreen = TRUE;
        return TRUE;

      case SDLK_PAGEUP:
        gCurDebugPage++;
        if (gCurDebugPage == MAX_DEBUG_PAGES) gCurDebugPage = 0;
        FreeBackgroundRect(guiBackgroundRect);
        guiBackgroundRect = NO_BGND_RECT;
        break;

      case SDLK_PAGEDOWN:
        gCurDebugPage--;
        if (gCurDebugPage < 0) gCurDebugPage = MAX_DEBUG_PAGES - 1;
        FreeBackgroundRect(guiBackgroundRect);
        guiBackgroundRect = NO_BGND_RECT;
        break;
    }
  }

  return FALSE;
}

void SetDebugRenderHook(RENDER_HOOK pDebugRenderOverride, int8_t ubPage) {
  gDebugRenderOverride[ubPage] = pDebugRenderOverride;
}

static void DefaultDebugPage1() {
  SetFont(LARGEFONT1);
  gprintf(0, 0, L"DEBUG PAGE ONE");
}

static void DefaultDebugPage2() {
  SetFont(LARGEFONT1);
  gprintf(0, 0, L"DEBUG PAGE TWO");
}

static void DefaultDebugPage3() {
  SetFont(LARGEFONT1);
  gprintf(0, 0, L"DEBUG PAGE THREE");
}

static void DefaultDebugPage4() {
  SetFont(LARGEFONT1);
  gprintf(0, 0, L"DEBUG PAGE FOUR");
}

#define SMILY_DELAY 100
#define SMILY_END_DELAY 1000

ScreenID SexScreenHandle() {
  static uint8_t ubCurrentScreen = 0;
  static SGPVObject *guiSMILY;
  static int8_t bCurFrame = 0;
  static uint32_t uiTimeOfLastUpdate = 0, uiTime;

  // OK, Clear screen and show smily face....
  FRAME_BUFFER->Fill(Get16BPPColor(FROMRGB(0, 0, 0)));
  InvalidateScreen();
  // Remove cursor....
  SetCurrentCursorFromDatabase(VIDEO_NO_CURSOR);

  if (ubCurrentScreen == 0) {
    // Load face....
    guiSMILY = AddVideoObjectFromFile(INTERFACEDIR "/luckysmile.sti");

    // Init screen
    bCurFrame = 0;

    ubCurrentScreen = 1;

    uiTimeOfLastUpdate = GetJA2Clock();

    return (SEX_SCREEN);
  }

  // Update frame
  uiTime = GetJA2Clock();

  // if we are animation smile...
  if (ubCurrentScreen == 1) {
    PlayJA2StreamingSampleFromFile(SOUNDSDIR "/sex.wav", HIGHVOLUME, 1, MIDDLEPAN, NULL);
    if ((uiTime - uiTimeOfLastUpdate) > SMILY_DELAY) {
      uiTimeOfLastUpdate = uiTime;

      bCurFrame++;

      if (bCurFrame == 32) {
        // Start end delay
        ubCurrentScreen = 2;
      }
    }
  }

  if (ubCurrentScreen == 2) {
    if ((uiTime - uiTimeOfLastUpdate) > SMILY_END_DELAY) {
      uiTimeOfLastUpdate = uiTime;

      ubCurrentScreen = 0;

      // Remove video object...
      DeleteVideoObject(guiSMILY);

      FadeInGameScreen();

      // Advance time...
      // Chris.... do this based on stats?
      WarpGameTime(((5 + Random(20)) * NUM_SEC_IN_MIN), WARPTIME_NO_PROCESSING_OF_EVENTS);

      return (GAME_SCREEN);
    }
  }

  // Calculate smily face positions...
  ETRLEObject const &pTrav = guiSMILY->SubregionProperties(0);
  int16_t const sX = (SCREEN_WIDTH - pTrav.usWidth) / 2;
  int16_t const sY = (SCREEN_HEIGHT - pTrav.usHeight) / 2;

  BltVideoObject(FRAME_BUFFER, guiSMILY, bCurFrame < 24 ? 0 : bCurFrame % 8, sX, sY);

  InvalidateRegion(sX, sY, sX + pTrav.usWidth, sY + pTrav.usHeight);

  return (SEX_SCREEN);
}
