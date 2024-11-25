// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "MainMenuScreen.h"

#include "Directories.h"
#include "GameLoop.h"
#include "GameSettings.h"
#include "JA2Splash.h"
#include "JAScreens.h"
#include "Local.h"
#include "MessageBoxScreen.h"
#include "OptionsScreen.h"
#include "SGP/ButtonSystem.h"
#include "SGP/CursorControl.h"
#include "SGP/English.h"
#include "SGP/Input.h"
#include "SGP/SGP.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "SaveLoadScreen.h"
#include "TileEngine/RenderDirty.h"
#include "TileEngine/SysUtil.h"
#include "Utils/Cursors.h"
#include "Utils/FontControl.h"
#include "Utils/MultiLanguageGraphicUtils.h"
#include "Utils/MusicControl.h"
#include "Utils/Text.h"
#include "Utils/TimerControl.h"
#include "Utils/WordWrap.h"

// #define TESTFOREIGNFONTS

// MENU ITEMS
enum { NEW_GAME, LOAD_GAME, PREFERENCES, CREDITS, QUIT, NUM_MENU_ITEMS };

#if defined TESTFOREIGNFONTS
#define MAINMENU_Y 0
#define MAINMENU_Y_SPACE 18
#else
#define MAINMENU_Y 277
#define MAINMENU_Y_SPACE 37
#endif

static BUTTON_PICS *iMenuImages[NUM_MENU_ITEMS];
static GUIButtonRef iMenuButtons[NUM_MENU_ITEMS];

static SGPVObject *guiMainMenuBackGroundImage;
static SGPVObject *guiJa2LogoImage;

static int8_t gbHandledMainMenu = 0;
static BOOLEAN fInitialRender = FALSE;

static BOOLEAN gfMainMenuScreenEntry = FALSE;
static BOOLEAN gfMainMenuScreenExit = FALSE;

static ScreenID guiMainMenuExitScreen = MAINMENU_SCREEN;

extern BOOLEAN gfLoadGameUponEntry;

static void ExitMainMenu();
static void HandleMainMenuInput();
static void HandleMainMenuScreen();
static void RenderMainMenu();
static void RestoreButtonBackGrounds();

ScreenID MainMenuScreenHandle() {
  if (guiSplashStartTime + 4000 > GetJA2Clock()) {
    SetCurrentCursorFromDatabase(VIDEO_NO_CURSOR);
    SetMusicMode(MUSIC_NONE);
    return MAINMENU_SCREEN;  // The splash screen hasn't been up long enough yet.
  }
  if (guiSplashFrameFade) {
    // Fade the splash screen.
    if (guiSplashFrameFade > 2) {
      FRAME_BUFFER->ShadowRectUsingLowPercentTable(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    } else if (guiSplashFrameFade > 1) {
      FRAME_BUFFER->Fill(0);
    } else {
      SetMusicMode(MUSIC_MAIN_MENU);
      SetCurrentCursorFromDatabase(CURSOR_NORMAL);
    }

    guiSplashFrameFade--;

    InvalidateScreen();
    return MAINMENU_SCREEN;
  }

  if (gfMainMenuScreenEntry) {
    InitMainMenu();
    gfMainMenuScreenEntry = FALSE;
    gfMainMenuScreenExit = FALSE;
    guiMainMenuExitScreen = MAINMENU_SCREEN;
    SetMusicMode(MUSIC_MAIN_MENU);
  }

  if (fInitialRender) {
    ClearMainMenu();
    RenderMainMenu();

    fInitialRender = FALSE;
  }

  RestoreButtonBackGrounds();

  // Render buttons
  for (uint32_t cnt = 0; cnt < NUM_MENU_ITEMS; ++cnt) {
    MarkAButtonDirty(iMenuButtons[cnt]);
  }

  RenderButtons();

  EndFrameBufferRender();

  HandleMainMenuInput();
  HandleMainMenuScreen();

  if (gfMainMenuScreenExit) {
    ExitMainMenu();
    gfMainMenuScreenExit = FALSE;
    gfMainMenuScreenEntry = TRUE;
  }

  if (guiMainMenuExitScreen != MAINMENU_SCREEN) gfMainMenuScreenEntry = TRUE;

  return guiMainMenuExitScreen;
}

static void SetMainMenuExitScreen(ScreenID uiNewScreen);

static void HandleMainMenuScreen() {
  if (gbHandledMainMenu == 0) return;

  // Exit according to handled value!
  switch (gbHandledMainMenu) {
    case QUIT:
      gfMainMenuScreenExit = TRUE;
      requestGameExit();
      break;

    case LOAD_GAME:
      // Select the game which is to be restored
      guiPreviousOptionScreen = guiCurrentScreen;
      gbHandledMainMenu = 0;
      gfSaveGame = FALSE;
      SetMainMenuExitScreen(SAVE_LOAD_SCREEN);
      break;

    case PREFERENCES:
      guiPreviousOptionScreen = guiCurrentScreen;
      gbHandledMainMenu = 0;
      SetMainMenuExitScreen(OPTIONS_SCREEN);
      break;

    case CREDITS:
      gbHandledMainMenu = 0;
      SetMainMenuExitScreen(CREDIT_SCREEN);
      break;
  }
}

static void CreateDestroyMainMenuButtons(BOOLEAN fCreate);

void InitMainMenu() {
  CreateDestroyMainMenuButtons(TRUE);

#define GFX_DIR LOADSCREENSDIR
  guiMainMenuBackGroundImage = AddVideoObjectFromFile(GFX_DIR "/mainmenubackground.sti");
  guiJa2LogoImage = AddVideoObjectFromFile(GFX_DIR "/ja2logo.sti");
#undef GFX_DIR

  // If there are no saved games, disable the button
  if (!AreThereAnySavedGameFiles()) DisableButton(iMenuButtons[LOAD_GAME]);

  gbHandledMainMenu = 0;
  fInitialRender = TRUE;

  SetPendingNewScreen(MAINMENU_SCREEN);
  guiMainMenuExitScreen = MAINMENU_SCREEN;

  InitGameOptions();

  DequeueAllKeyBoardEvents();
}

static void ExitMainMenu() {
  CreateDestroyMainMenuButtons(FALSE);
  DeleteVideoObject(guiMainMenuBackGroundImage);
  DeleteVideoObject(guiJa2LogoImage);
  gMsgBox.uiExitScreen = MAINMENU_SCREEN;
}

static void MenuButtonCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    int8_t const bID = btn->GetUserData();

    gbHandledMainMenu = bID;
    RenderMainMenu();

    switch (gbHandledMainMenu) {
      case NEW_GAME:
        SetMainMenuExitScreen(GAME_INIT_OPTIONS_SCREEN);
        break;
      case LOAD_GAME:
        if (IsKeyDown(ALT)) gfLoadGameUponEntry = TRUE;
        break;
    }
  }
}

static void HandleMainMenuInput() {
  InputAtom InputEvent;
  while (DequeueEvent(&InputEvent)) {
    if (InputEvent.usEvent == KEY_UP) {
      switch (InputEvent.usParam) {
          /*
                                          case SDLK_ESCAPE: gbHandledMainMenu =
             QUIT; break;
          */

        case 'c':
          if (IsKeyDown(ALT)) gfLoadGameUponEntry = TRUE;
          gbHandledMainMenu = LOAD_GAME;
          break;

        case 'o':
          gbHandledMainMenu = PREFERENCES;
          break;

        case 's':
          gbHandledMainMenu = CREDITS;
          break;
      }
    }
  }
}

void ClearMainMenu() {
  FRAME_BUFFER->Fill(0);
  InvalidateScreen();
}

static void SetMainMenuExitScreen(ScreenID const uiNewScreen) {
  guiMainMenuExitScreen = uiNewScreen;
  gfMainMenuScreenExit = TRUE;
}

static void CreateDestroyMainMenuButtons(BOOLEAN fCreate) {
  static BOOLEAN fButtonsCreated = FALSE;

  if (fCreate) {
    if (fButtonsCreated) return;

    // Reset the variable that allows the user to ALT click on the continue save
    // btn to load the save instantly
    gfLoadGameUponEntry = FALSE;

    // Load button images
    const char *const filename = GetMLGFilename(MLG_TITLETEXT);

    int32_t Slot;
    Slot = 0;
    iMenuImages[NEW_GAME] = LoadButtonImage(filename, Slot, Slot, Slot + 1, Slot + 2, -1);
    iMenuImages[LOAD_GAME] = UseLoadedButtonImage(iMenuImages[NEW_GAME], 6, 3, 4, 5, -1);
    iMenuImages[PREFERENCES] = UseLoadedButtonImage(iMenuImages[NEW_GAME], 7, 7, 8, 9, -1);
    iMenuImages[CREDITS] = UseLoadedButtonImage(iMenuImages[NEW_GAME], 13, 10, 11, 12, -1);
    iMenuImages[QUIT] = UseLoadedButtonImage(iMenuImages[NEW_GAME], 14, 14, 15, 16, -1);

    for (uint32_t cnt = 0; cnt < NUM_MENU_ITEMS; ++cnt) {
      BUTTON_PICS *const img = iMenuImages[cnt];
      const uint16_t w = GetDimensionsOfButtonPic(img)->w;
      const int16_t x = (SCREEN_WIDTH - w) / 2;
      const int16_t y = MAINMENU_Y + cnt * MAINMENU_Y_SPACE;
      GUIButtonRef const b =
          QuickCreateButton(img, x, y, MSYS_PRIORITY_HIGHEST, MenuButtonCallback);
      iMenuButtons[cnt] = b;
      b->SetUserData(cnt);
    }

    fButtonsCreated = TRUE;
  } else {
    if (!fButtonsCreated) return;

    // Delete images/buttons
    for (uint32_t cnt = 0; cnt < NUM_MENU_ITEMS; ++cnt) {
      RemoveButton(iMenuButtons[cnt]);
      UnloadButtonImage(iMenuImages[cnt]);
    }
    fButtonsCreated = FALSE;
  }
}

static void RenderMainMenu() {
  BltVideoObject(guiSAVEBUFFER, guiMainMenuBackGroundImage, 0, 0, 0);
  BltVideoObject(guiSAVEBUFFER, guiJa2LogoImage, 0, 188, 15);

  RestoreExternBackgroundRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

#if defined TESTFOREIGNFONTS
  uint16_t y = 105;
#define TEST_FONT(font)                                                                \
  DrawTextToScreen(#font L": ÄÀÁÂÇËÈÉÊÏÖÒÓÔÜÙÚÛäàáâçëèéêïöòóôüùúûÌì", 0, y, 640, font, \
                   FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);              \
  y += 20;
  TEST_FONT(LARGEFONT1);
  TEST_FONT(SMALLFONT1);
  TEST_FONT(TINYFONT1);
  TEST_FONT(FONT12POINT1);
  TEST_FONT(COMPFONT);
  TEST_FONT(SMALLCOMPFONT);
  TEST_FONT(MILITARYFONT1);
  TEST_FONT(FONT10ARIAL);
  TEST_FONT(FONT14ARIAL);
  TEST_FONT(FONT12ARIAL);
  TEST_FONT(FONT10ARIALBOLD);
  TEST_FONT(BLOCKFONT);
  TEST_FONT(BLOCKFONT2);
  TEST_FONT(FONT12ARIALFIXEDWIDTH);
  TEST_FONT(FONT16ARIAL);
  TEST_FONT(BLOCKFONTNARROW);
  TEST_FONT(FONT14HUMANIST);
#undef TEST_FONT
#else
  DrawTextToScreen(gzCopyrightText, 0, SCREEN_HEIGHT - 15, SCREEN_WIDTH, FONT10ARIAL,
                   FONT_MCOLOR_WHITE, FONT_MCOLOR_BLACK, CENTER_JUSTIFIED);
#endif
}

static void RestoreButtonBackGrounds() {
#ifndef TESTFOREIGNFONTS
  for (uint32_t cnt = 0; cnt < NUM_MENU_ITEMS; ++cnt) {
    GUI_BUTTON const &b = *iMenuButtons[cnt];
    RestoreExternBackgroundRect(b.X(), b.Y(), b.W() + 1, b.H() + 1);
  }
#endif
}
