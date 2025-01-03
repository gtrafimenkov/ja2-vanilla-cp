// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "OptionsScreen.h"

#include "Directories.h"
#include "GameScreen.h"
#include "GameSettings.h"
#include "Local.h"
#include "MessageBoxScreen.h"
#include "SGP/ButtonSystem.h"
#include "SGP/English.h"
#include "SGP/Font.h"
#include "SGP/SoundMan.h"
#include "SGP/Types.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "SaveLoadScreen.h"
#include "Strategic/GameClock.h"
#include "Strategic/GameInit.h"
#include "Tactical/Gap.h"
#include "Tactical/HandleItems.h"
#include "Tactical/MapInformation.h"
#include "Tactical/Overhead.h"
#include "TileEngine/AmbientControl.h"
#include "TileEngine/ExitGrids.h"
#include "TileEngine/RenderDirty.h"
#include "TileEngine/SmokeEffects.h"
#include "TileEngine/SysUtil.h"
#include "TileEngine/WorldDat.h"
#include "TileEngine/WorldDef.h"
#include "TileEngine/WorldMan.h"
#include "Utils/Cursors.h"
#include "Utils/FontControl.h"
#include "Utils/Message.h"
#include "Utils/MultiLanguageGraphicUtils.h"
#include "Utils/MusicControl.h"
#include "Utils/Slider.h"
#include "Utils/SoundControl.h"
#include "Utils/Text.h"
#include "Utils/TextInput.h"
#include "Utils/TimerControl.h"
#include "Utils/WordWrap.h"

#include "SDL_keycode.h"

#define OPT_MAIN_FONT FONT12ARIAL
#define OPT_MAIN_COLOR OPT_BUTTON_ON_COLOR     // FONT_MCOLOR_WHITE
#define OPT_HIGHLIGHT_COLOR FONT_MCOLOR_WHITE  // FONT_MCOLOR_LTYELLOW

#define OPTIONS_SCREEN_WIDTH 440
#define OPTIONS_SCREEN_HEIGHT 400

#define OPTIONS__TOP_LEFT_X 100
#define OPTIONS__TOP_LEFT_Y 40
#define OPTIONS__BOTTOM_RIGHT_X OPTIONS__TOP_LEFT_X + OPTIONS_SCREEN_WIDTH
#define OPTIONS__BOTTOM_RIGHT_Y OPTIONS__TOP_LEFT_Y + OPTIONS_SCREEN_HEIGHT

#define OPT_SAVE_BTN_X 51
#define OPT_LOAD_BTN_X 190
#define OPT_QUIT_BTN_X 329
#define OPT_DONE_BTN_X 469
#define OPT_BTN_Y 438

#define OPT_GAP_BETWEEN_TOGGLE_BOXES 31  // 40

// Text
#define OPT_TOGGLE_BOX_FIRST_COL_TEXT_X \
  OPT_TOGGLE_BOX_FIRST_COLUMN_X + OPT_SPACE_BETWEEN_TEXT_AND_TOGGLE_BOX  // 350
#define OPT_TOGGLE_BOX_SECOND_TEXT_X \
  OPT_TOGGLE_BOX_SECOND_COLUMN_X + OPT_SPACE_BETWEEN_TEXT_AND_TOGGLE_BOX  // 350

// toggle boxes
#define OPT_SPACE_BETWEEN_TEXT_AND_TOGGLE_BOX 30  // 220
#define OPT_TOGGLE_TEXT_OFFSET_Y 2                // 3

#define OPT_TOGGLE_BOX_FIRST_COLUMN_X \
  265  // 257 //OPT_TOGGLE_BOX_TEXT_X + OPT_SPACE_BETWEEN_TEXT_AND_TOGGLE_BOX
#define OPT_TOGGLE_BOX_SECOND_COLUMN_X \
  428  // OPT_TOGGLE_BOX_TEXT_X + OPT_SPACE_BETWEEN_TEXT_AND_TOGGLE_BOX
#define OPT_TOGGLE_BOX_START_Y 89

#define OPT_TOGGLE_BOX_TEXT_WIDTH \
  OPT_TOGGLE_BOX_SECOND_COLUMN_X - OPT_TOGGLE_BOX_FIRST_COLUMN_X - 20

// Slider bar defines
#define OPT_SLIDER_BAR_SIZE 258

#define OPT_SLIDER_TEXT_WIDTH 45

#define OPT_SOUND_FX_TEXT_X 38
#define OPT_SOUND_FX_TEXT_Y 87  // 116//110

#define OPT_SPEECH_TEXT_X 85  // OPT_SOUND_FX_TEXT_X + OPT_SLIDER_TEXT_WIDTH
#define OPT_SPEECH_TEXT_Y OPT_SOUND_FX_TEXT_Y

#define OPT_MUSIC_TEXT_X 137
#define OPT_MUSIC_TEXT_Y OPT_SOUND_FX_TEXT_Y

#define OPT_SOUND_EFFECTS_SLIDER_X 56
#define OPT_SOUND_EFFECTS_SLIDER_Y 126

#define OPT_SPEECH_SLIDER_X 107
#define OPT_SPEECH_SLIDER_Y OPT_SOUND_EFFECTS_SLIDER_Y

#define OPT_MUSIC_SLIDER_X 158
#define OPT_MUSIC_SLIDER_Y OPT_SOUND_EFFECTS_SLIDER_Y

#define OPT_MUSIC_SLIDER_PLAY_SOUND_DELAY 75

#define OPT_FIRST_COLUMN_TOGGLE_CUT_OFF 10  // 8

static SGPVObject *guiOptionBackGroundImage;
static SGPVObject *guiOptionsAddOnImages;

static SLIDER *guiSoundEffectsSlider;
static SLIDER *guiSpeechSlider;
static SLIDER *guiMusicSlider;

static BOOLEAN gfOptionsScreenEntry = TRUE;
static BOOLEAN gfOptionsScreenExit = FALSE;
static BOOLEAN gfRedrawOptionsScreen = TRUE;

static ScreenID guiOptionsScreen = OPTIONS_SCREEN;
ScreenID guiPreviousOptionScreen = OPTIONS_SCREEN;

static BOOLEAN gfExitOptionsDueToMessageBox = FALSE;
static BOOLEAN gfExitOptionsAfterMessageBox = FALSE;

static uint32_t guiSoundFxSliderMoving = 0xFFFFFFFF;
static uint32_t guiSpeechSliderMoving = 0xFFFFFFFF;

static int8_t gbHighLightedOptionText = -1;

static BOOLEAN gfSettingOfTreeTopStatusOnEnterOfOptionScreen;
static BOOLEAN gfSettingOfItemGlowStatusOnEnterOfOptionScreen;
static BOOLEAN gfSettingOfDontAnimateSmoke;

static BUTTON_PICS *giOptionsButtonImages;
static GUIButtonRef guiOptGotoSaveGameBtn;
static GUIButtonRef guiOptGotoLoadGameBtn;
static GUIButtonRef guiQuitButton;
static GUIButtonRef guiDoneButton;

// checkbox to toggle tracking mode on or off
static GUIButtonRef guiOptionsToggles[NUM_GAME_OPTIONS];
static void BtnOptionsTogglesCallback(GUI_BUTTON *btn, int32_t reason);

// Mouse regions for the name of the option
static MOUSE_REGION gSelectedOptionTextRegion[NUM_GAME_OPTIONS];

// Mouse regions for the area around the toggle boxs
static MOUSE_REGION gSelectedToggleBoxAreaRegion;

static void EnterOptionsScreen();
static void ExitOptionsScreen();
static void GetOptionsScreenUserInput();
static void HandleOptionsScreen();
static void RenderOptionsScreen();

ScreenID OptionsScreenHandle() {
  if (gfOptionsScreenEntry) {
    PauseGame();
    EnterOptionsScreen();
    gfOptionsScreenEntry = FALSE;
    gfOptionsScreenExit = FALSE;
    gfRedrawOptionsScreen = TRUE;
    RenderOptionsScreen();

    // Blit the background to the save buffer
    BltVideoSurface(guiSAVEBUFFER, FRAME_BUFFER, 0, 0, NULL);
    InvalidateScreen();
  }

  RestoreBackgroundRects();

  GetOptionsScreenUserInput();

  HandleOptionsScreen();

  if (gfRedrawOptionsScreen) {
    RenderOptionsScreen();
    RenderButtons();

    gfRedrawOptionsScreen = FALSE;
  }

  // Render the active slider bars
  RenderAllSliderBars();

  // render buttons marked dirty
  MarkButtonsDirty();
  RenderButtons();

  // ATE: Put here to save RECTS before any fast help being drawn...
  SaveBackgroundRects();
  RenderButtonsFastHelp();

  ExecuteBaseDirtyRectQueue();
  EndFrameBufferRender();

  if (gfOptionsScreenExit) {
    ExitOptionsScreen();
    gfOptionsScreenExit = FALSE;
    gfOptionsScreenEntry = TRUE;

    UnPauseGame();
  }

  return (guiOptionsScreen);
}

static GUIButtonRef MakeButton(int16_t x, GUI_CALLBACK click, const wchar_t *text) {
  return CreateIconAndTextButton(giOptionsButtonImages, text, OPT_BUTTON_FONT, OPT_BUTTON_ON_COLOR,
                                 DEFAULT_SHADOW, OPT_BUTTON_OFF_COLOR, DEFAULT_SHADOW, x, OPT_BTN_Y,
                                 MSYS_PRIORITY_HIGH, click);
}

static void BtnOptGotoSaveGameCallback(GUI_BUTTON *btn, int32_t reason);
static void BtnOptGotoLoadGameCallback(GUI_BUTTON *btn, int32_t reason);
static void BtnOptQuitCallback(GUI_BUTTON *btn, int32_t reason);
static void BtnDoneCallback(GUI_BUTTON *btn, int32_t reason);
static void MusicSliderChangeCallBack(int32_t iNewValue);
static void SelectedOptionTextRegionCallBack(MOUSE_REGION *pRegion, int32_t iReason);
static void SelectedOptionTextRegionMovementCallBack(MOUSE_REGION *pRegion, int32_t reason);
static void SelectedToggleBoxAreaRegionMovementCallBack(MOUSE_REGION *pRegion, int32_t reason);
static void SetOptionsScreenToggleBoxes();
static void SoundFXSliderChangeCallBack(int32_t iNewValue);
static void SpeechSliderChangeCallBack(int32_t iNewValue);

static void EnterOptionsScreen() {
  // Stop ambients...
  StopAmbients();

  guiOptionsScreen = OPTIONS_SCREEN;

  // Init the slider bar;
  InitSlider();

  if (gfExitOptionsDueToMessageBox) {
    gfRedrawOptionsScreen = TRUE;
    gfExitOptionsDueToMessageBox = FALSE;
    return;
  }

  gfExitOptionsDueToMessageBox = FALSE;

  // load the options screen background graphic and add it
  guiOptionBackGroundImage = AddVideoObjectFromFile(INTERFACEDIR "/optionscreenbase.sti");

  // load button, title graphic and add it
  const char *const ImageFile = GetMLGFilename(MLG_OPTIONHEADER);
  guiOptionsAddOnImages = AddVideoObjectFromFile(ImageFile);

  giOptionsButtonImages = LoadButtonImage(INTERFACEDIR "/optionscreenaddons.sti", 2, 3);

  // Save game button
  guiOptGotoSaveGameBtn =
      MakeButton(OPT_SAVE_BTN_X, BtnOptGotoSaveGameCallback, zOptionsText[OPT_SAVE_GAME]);
  guiOptGotoSaveGameBtn->SpecifyDisabledStyle(GUI_BUTTON::DISABLED_STYLE_HATCHED);
  if (guiPreviousOptionScreen == MAINMENU_SCREEN || !CanGameBeSaved()) {
    DisableButton(guiOptGotoSaveGameBtn);
  }

  guiOptGotoLoadGameBtn =
      MakeButton(OPT_LOAD_BTN_X, BtnOptGotoLoadGameCallback, zOptionsText[OPT_LOAD_GAME]);
  guiQuitButton = MakeButton(OPT_QUIT_BTN_X, BtnOptQuitCallback, zOptionsText[OPT_MAIN_MENU]);
  guiDoneButton = MakeButton(OPT_DONE_BTN_X, BtnDoneCallback, zOptionsText[OPT_DONE]);

  // Toggle Boxes
  uint16_t usTextHeight = GetFontHeight(OPT_MAIN_FONT);

  // Create the first column of check boxes
  uint32_t pos_x = OPT_TOGGLE_BOX_FIRST_COLUMN_X;
  uint16_t pos_y = OPT_TOGGLE_BOX_START_Y;
  for (uint8_t cnt = 0; cnt < NUM_GAME_OPTIONS; cnt++) {
    // if this is the blood and gore option, and we are to hide the option
    if (cnt == OPT_FIRST_COLUMN_TOGGLE_CUT_OFF) {
      pos_y = OPT_TOGGLE_BOX_START_Y;
      pos_x = OPT_TOGGLE_BOX_SECOND_COLUMN_X;
    }

    // Check box to toggle tracking mode
    GUIButtonRef const check =
        CreateCheckBoxButton(pos_x, pos_y, INTERFACEDIR "/optionscheckboxes.sti",
                             MSYS_PRIORITY_HIGH + 10, BtnOptionsTogglesCallback);
    guiOptionsToggles[cnt] = check;
    check->SetUserData(cnt);

    uint32_t height;
    uint16_t usTextWidth = StringPixLength(zOptionsToggleText[cnt], OPT_MAIN_FONT);
    if (usTextWidth > OPT_TOGGLE_BOX_TEXT_WIDTH) {
      // Get how many lines will be used to display the string, without
      // displaying the string
      usTextWidth = OPT_TOGGLE_BOX_TEXT_WIDTH;
      height = DisplayWrappedString(0, 0, OPT_TOGGLE_BOX_TEXT_WIDTH, 2, OPT_MAIN_FONT,
                                    OPT_HIGHLIGHT_COLOR, zOptionsToggleText[cnt], FONT_MCOLOR_BLACK,
                                    LEFT_JUSTIFIED | DONT_DISPLAY_TEXT);
    } else {
      height = usTextHeight;
    }
    MOUSE_REGION *reg = &gSelectedOptionTextRegion[cnt];
    MSYS_DefineRegion(reg, pos_x + 13, pos_y,
                      pos_x + OPT_SPACE_BETWEEN_TEXT_AND_TOGGLE_BOX + usTextWidth, pos_y + height,
                      MSYS_PRIORITY_HIGH, CURSOR_NORMAL, SelectedOptionTextRegionMovementCallBack,
                      SelectedOptionTextRegionCallBack);
    MSYS_SetRegionUserData(reg, 0, cnt);

    reg->SetFastHelpText(zOptionsScreenHelpText[cnt]);
    check->SetFastHelpText(zOptionsScreenHelpText[cnt]);

    pos_y += OPT_GAP_BETWEEN_TOGGLE_BOXES;
  }

  // Create a mouse region so when the user leaves a togglebox text region we
  // can detect it then unselect the region
  MSYS_DefineRegion(&gSelectedToggleBoxAreaRegion, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                    MSYS_PRIORITY_NORMAL, CURSOR_NORMAL,
                    SelectedToggleBoxAreaRegionMovementCallBack, MSYS_NO_CALLBACK);

  // Render the scene before adding the slider boxes
  RenderOptionsScreen();

  // Add a slider bar for the Sound Effects
  guiSoundEffectsSlider = AddSlider(
      SLIDER_VERTICAL_STEEL, CURSOR_NORMAL, OPT_SOUND_EFFECTS_SLIDER_X, OPT_SOUND_EFFECTS_SLIDER_Y,
      OPT_SLIDER_BAR_SIZE, MAXVOLUME, MSYS_PRIORITY_HIGH, SoundFXSliderChangeCallBack);
  SetSliderValue(guiSoundEffectsSlider, GetSoundEffectsVolume());

  // Add a slider bar for the Speech
  guiSpeechSlider =
      AddSlider(SLIDER_VERTICAL_STEEL, CURSOR_NORMAL, OPT_SPEECH_SLIDER_X, OPT_SPEECH_SLIDER_Y,
                OPT_SLIDER_BAR_SIZE, MAXVOLUME, MSYS_PRIORITY_HIGH, SpeechSliderChangeCallBack);
  SetSliderValue(guiSpeechSlider, GetSpeechVolume());

  // Add a slider bar for the Music
  guiMusicSlider =
      AddSlider(SLIDER_VERTICAL_STEEL, CURSOR_NORMAL, OPT_MUSIC_SLIDER_X, OPT_MUSIC_SLIDER_Y,
                OPT_SLIDER_BAR_SIZE, MAXVOLUME, MSYS_PRIORITY_HIGH, MusicSliderChangeCallBack);
  SetSliderValue(guiMusicSlider, MusicGetVolume());

  // Remove the mouse region over the clock
  RemoveMouseRegionForPauseOfClock();

  // Draw the screen
  gfRedrawOptionsScreen = TRUE;

  // Set the option screen toggle boxes
  SetOptionsScreenToggleBoxes();

  DisableScrollMessages();

  // reset
  gbHighLightedOptionText = -1;

  // get the status of the tree top option
  gfSettingOfTreeTopStatusOnEnterOfOptionScreen = gGameSettings.fOptions[TOPTION_TOGGLE_TREE_TOPS];

  // Get the status of the item glow option
  gfSettingOfItemGlowStatusOnEnterOfOptionScreen = gGameSettings.fOptions[TOPTION_GLOW_ITEMS];

  gfSettingOfDontAnimateSmoke = gGameSettings.fOptions[TOPTION_ANIMATE_SMOKE];
}

static void GetOptionsScreenToggleBoxes();

static void ExitOptionsScreen() {
  uint8_t cnt;

  if (gfExitOptionsDueToMessageBox) {
    gfOptionsScreenExit = FALSE;

    if (!gfExitOptionsAfterMessageBox) return;
    gfExitOptionsAfterMessageBox = FALSE;
    gfExitOptionsDueToMessageBox = FALSE;
  }

  // Get the current status of the toggle boxes
  GetOptionsScreenToggleBoxes();
  // The save the current settings to disk
  SaveGameSettings();

  CreateMouseRegionForPauseOfClock();

  if (guiOptionsScreen == GAME_SCREEN) EnterTacticalScreen();

  RemoveButton(guiOptGotoSaveGameBtn);
  RemoveButton(guiOptGotoLoadGameBtn);
  RemoveButton(guiQuitButton);
  RemoveButton(guiDoneButton);

  UnloadButtonImage(giOptionsButtonImages);

  DeleteVideoObject(guiOptionBackGroundImage);
  DeleteVideoObject(guiOptionsAddOnImages);

  // Remove the toggle buttons
  for (cnt = 0; cnt < NUM_GAME_OPTIONS; cnt++) {
    RemoveButton(guiOptionsToggles[cnt]);

    MSYS_RemoveRegion(&gSelectedOptionTextRegion[cnt]);
  }

  // REmove the slider bars
  RemoveSliderBar(guiSoundEffectsSlider);
  RemoveSliderBar(guiSpeechSlider);
  RemoveSliderBar(guiMusicSlider);

  MSYS_RemoveRegion(&gSelectedToggleBoxAreaRegion);

  ShutDownSlider();

  // if the user changed the  TREE TOP option, AND a world is loaded
  if (gfSettingOfTreeTopStatusOnEnterOfOptionScreen !=
          gGameSettings.fOptions[TOPTION_TOGGLE_TREE_TOPS] &&
      gfWorldLoaded) {
    SetTreeTopStateForMap();
  }

  // if the user has changed the item glow option AND a world is loaded
  if (gfSettingOfItemGlowStatusOnEnterOfOptionScreen !=
          gGameSettings.fOptions[TOPTION_GLOW_ITEMS] &&
      gfWorldLoaded) {
    ToggleItemGlow(gGameSettings.fOptions[TOPTION_GLOW_ITEMS]);
  }

  if (gfSettingOfDontAnimateSmoke != gGameSettings.fOptions[TOPTION_ANIMATE_SMOKE] &&
      gfWorldLoaded) {
    UpdateSmokeEffectGraphics();
  }
}

static void HandleHighLightedText(BOOLEAN fHighLight);
static void HandleSliderBarMovementSounds();

static void HandleOptionsScreen() {
  HandleSliderBarMovementSounds();

  HandleHighLightedText(TRUE);
}

static void RenderOptionsScreen() {
  BltVideoObject(FRAME_BUFFER, guiOptionBackGroundImage, 0, 0, 0);

  // Get and display the titla image
  BltVideoObject(FRAME_BUFFER, guiOptionsAddOnImages, 0, 0, 0);
  BltVideoObject(FRAME_BUFFER, guiOptionsAddOnImages, 1, 0, 434);

  //
  // Text for the toggle boxes
  //

  uint32_t pos_x = OPT_TOGGLE_BOX_FIRST_COL_TEXT_X;
  uint16_t pos_y = OPT_TOGGLE_BOX_START_Y + OPT_TOGGLE_TEXT_OFFSET_Y;
  for (uint8_t cnt = 0; cnt < NUM_GAME_OPTIONS; cnt++) {
    if (cnt == OPT_FIRST_COLUMN_TOGGLE_CUT_OFF) {
      pos_x = OPT_TOGGLE_BOX_SECOND_TEXT_X;
      pos_y = OPT_TOGGLE_BOX_START_Y + OPT_TOGGLE_TEXT_OFFSET_Y;
    }

    uint16_t usWidth = StringPixLength(zOptionsToggleText[cnt], OPT_MAIN_FONT);

    // if the string is going to wrap, move the string up a bit
    if (usWidth > OPT_TOGGLE_BOX_TEXT_WIDTH)
      DisplayWrappedString(pos_x, pos_y, OPT_TOGGLE_BOX_TEXT_WIDTH, 2, OPT_MAIN_FONT,
                           OPT_MAIN_COLOR, zOptionsToggleText[cnt], FONT_MCOLOR_BLACK,
                           LEFT_JUSTIFIED);
    else
      DrawTextToScreen(zOptionsToggleText[cnt], pos_x, pos_y, 0, OPT_MAIN_FONT, OPT_MAIN_COLOR,
                       FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);

    pos_y += OPT_GAP_BETWEEN_TOGGLE_BOXES;
  }

  //
  // Text for the Slider Bars
  //

  // Display the Sound Fx text
  DisplayWrappedString(OPT_SOUND_FX_TEXT_X, OPT_SOUND_FX_TEXT_Y, OPT_SLIDER_TEXT_WIDTH, 2,
                       OPT_MAIN_FONT, OPT_MAIN_COLOR, zOptionsText[OPT_SOUND_FX], FONT_MCOLOR_BLACK,
                       CENTER_JUSTIFIED);

  // Display the Speech text
  DisplayWrappedString(OPT_SPEECH_TEXT_X, OPT_SPEECH_TEXT_Y, OPT_SLIDER_TEXT_WIDTH, 2,
                       OPT_MAIN_FONT, OPT_MAIN_COLOR, zOptionsText[OPT_SPEECH], FONT_MCOLOR_BLACK,
                       CENTER_JUSTIFIED);

  // Display the Music text
  DisplayWrappedString(OPT_MUSIC_TEXT_X, OPT_MUSIC_TEXT_Y, OPT_SLIDER_TEXT_WIDTH, 2, OPT_MAIN_FONT,
                       OPT_MAIN_COLOR, zOptionsText[OPT_MUSIC], FONT_MCOLOR_BLACK,
                       CENTER_JUSTIFIED);

  InvalidateRegion(OPTIONS__TOP_LEFT_X, OPTIONS__TOP_LEFT_Y, OPTIONS__BOTTOM_RIGHT_X,
                   OPTIONS__BOTTOM_RIGHT_Y);
}

static void SetOptionsExitScreen(ScreenID);

static void GetOptionsScreenUserInput() {
  SGPPoint MousePos;
  GetMousePos(&MousePos);

  InputAtom Event;
  while (DequeueEvent(&Event)) {
    MouseSystemHook(Event.usEvent, MousePos.iX, MousePos.iY);

    if (!HandleTextInput(&Event) && Event.usEvent == KEY_DOWN) {
      switch (Event.usParam) {
        case SDLK_ESCAPE:
          SetOptionsExitScreen(guiPreviousOptionScreen);
          break;

        // Enter the save game screen
        case SDLK_s:
          // if the save game button isnt disabled
          if (guiOptGotoSaveGameBtn->Enabled()) {
            SetOptionsExitScreen(SAVE_LOAD_SCREEN);
            gfSaveGame = TRUE;
          }
          break;

        // Enter the Load game screen
        case SDLK_l:
          SetOptionsExitScreen(SAVE_LOAD_SCREEN);
          gfSaveGame = FALSE;
          break;
      }
    }
  }
}

static void SetOptionsExitScreen(ScreenID const uiExitScreen) {
  guiOptionsScreen = uiExitScreen;
  gfOptionsScreenExit = TRUE;
}

static void BtnOptGotoSaveGameCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    SetOptionsExitScreen(SAVE_LOAD_SCREEN);
    gfSaveGame = TRUE;
  }
}

static void BtnOptGotoLoadGameCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    SetOptionsExitScreen(SAVE_LOAD_SCREEN);
    gfSaveGame = FALSE;
  }
}

static void ConfirmQuitToMainMenuMessageBoxCallBack(MessageBoxReturnValue);
static void DoOptionsMessageBox(wchar_t const *zString, ScreenID uiExitScreen, MessageBoxFlags,
                                MSGBOX_CALLBACK ReturnCallback);

static void BtnOptQuitCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    // Confirm the Exit to the main menu screen
    DoOptionsMessageBox(zOptionsText[OPT_RETURN_TO_MAIN], OPTIONS_SCREEN, MSG_BOX_FLAG_YESNO,
                        ConfirmQuitToMainMenuMessageBoxCallBack);
  }
}

static void BtnDoneCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    SetOptionsExitScreen(guiPreviousOptionScreen);
  }
}

static void HandleOptionToggle(uint8_t button_id, bool state, bool down, bool play_sound);

static void BtnOptionsTogglesCallback(GUI_BUTTON *btn, int32_t reason) {
  bool down;
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    down = false;
  } else if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
    down = true;
  } else {
    return;
  }
  bool const clicked = btn->Clicked();
  uint8_t const ubButton = btn->GetUserData();
  HandleOptionToggle(ubButton, clicked, down, false);
}

static void HandleOptionToggle(uint8_t const button_id, bool const state, bool const down,
                               bool const play_sound) {
  gGameSettings.fOptions[button_id] = state;

  GUI_BUTTON &b = *guiOptionsToggles[button_id];
  b.uiFlags &= ~BUTTON_CLICKED_ON;
  b.uiFlags |= state ? BUTTON_CLICKED_ON : 0;

  if (down) b.DrawCheckBoxOnOff(state);

  /* Check if the user is unselecting either the spech or subtitles toggle.
   * Make sure that at least one of the toggles is still enabled. */
  if (!state &&
      ((button_id == TOPTION_SPEECH && !guiOptionsToggles[TOPTION_SUBTITLES]->Clicked()) ||
       (button_id == TOPTION_SUBTITLES && !guiOptionsToggles[TOPTION_SPEECH]->Clicked()))) {
    gGameSettings.fOptions[button_id] = TRUE;
    b.uiFlags |= BUTTON_CLICKED_ON;
    DoOptionsMessageBox(zOptionsText[OPT_NEED_AT_LEAST_SPEECH_OR_SUBTITLE_OPTION_ON],
                        OPTIONS_SCREEN, MSG_BOX_FLAG_OK, 0);
    gfExitOptionsDueToMessageBox = FALSE;
  }

  if (play_sound) {
    SoundID const sound = down ? BIG_SWITCH3_IN : BIG_SWITCH3_OUT;
    PlayJA2Sample(sound, BTNVOLUME, 1, MIDDLEPAN);
  }
}

static void SoundFXSliderChangeCallBack(int32_t iNewValue) {
  SetSoundEffectsVolume(iNewValue);

  guiSoundFxSliderMoving = GetJA2Clock();
}

static void SpeechSliderChangeCallBack(int32_t iNewValue) {
  SetSpeechVolume(iNewValue);

  guiSpeechSliderMoving = GetJA2Clock();
}

static void MusicSliderChangeCallBack(int32_t iNewValue) { MusicSetVolume(iNewValue); }

void DoOptionsMessageBoxWithRect(wchar_t const *const zString, ScreenID const uiExitScreen,
                                 MessageBoxFlags const usFlags,
                                 MSGBOX_CALLBACK const ReturnCallback,
                                 SGPBox const *const centering_rect) {
  // reset exit mode
  gfExitOptionsDueToMessageBox = TRUE;

  // do message box and return
  DoMessageBox(MSG_BOX_BASIC_STYLE, zString, uiExitScreen, usFlags, ReturnCallback, centering_rect);
}

static void DoOptionsMessageBox(wchar_t const *const zString, ScreenID const uiExitScreen,
                                MessageBoxFlags const usFlags,
                                MSGBOX_CALLBACK const ReturnCallback) {
  DoOptionsMessageBoxWithRect(zString, uiExitScreen, usFlags, ReturnCallback, NULL);
}

static void ConfirmQuitToMainMenuMessageBoxCallBack(MessageBoxReturnValue const bExitValue) {
  // yes, Quit to main menu
  if (bExitValue == MSG_BOX_RETURN_YES) {
    gfExitOptionsAfterMessageBox = TRUE;
    SetOptionsExitScreen(MAINMENU_SCREEN);

    // We want to reinitialize the game
    ReStartingGame();
  } else {
    gfExitOptionsAfterMessageBox = FALSE;
    gfExitOptionsDueToMessageBox = FALSE;
  }
}

static void SetOptionsScreenToggleBoxes() {
  uint8_t cnt;

  for (cnt = 0; cnt < NUM_GAME_OPTIONS; cnt++) {
    if (gGameSettings.fOptions[cnt])
      guiOptionsToggles[cnt]->uiFlags |= BUTTON_CLICKED_ON;
    else
      guiOptionsToggles[cnt]->uiFlags &= ~BUTTON_CLICKED_ON;
  }
}

static void GetOptionsScreenToggleBoxes() {
  uint8_t cnt;

  for (cnt = 0; cnt < NUM_GAME_OPTIONS; cnt++) {
    gGameSettings.fOptions[cnt] = guiOptionsToggles[cnt]->Clicked();
  }
}

static void HandleSliderBarMovementSounds() {
  static uint32_t uiLastSoundFxTime = 0;
  static uint32_t uiLastSpeechTime = 0;
  static uint32_t uiLastPlayingSoundID = NO_SAMPLE;
  static uint32_t uiLastPlayingSpeechID = NO_SAMPLE;

  if ((uiLastSoundFxTime - OPT_MUSIC_SLIDER_PLAY_SOUND_DELAY) > guiSoundFxSliderMoving) {
    guiSoundFxSliderMoving = 0xffffffff;

    // The slider has stopped moving, reset the ambient sector sounds ( so it
    // will change the volume )
    if (!DidGameJustStart()) HandleNewSectorAmbience(gTilesets[giCurrentTilesetID].ubAmbientID);

    if (!SoundIsPlaying(uiLastPlayingSoundID))
      uiLastPlayingSoundID =
          PlayJA2SampleFromFile(SOUNDSDIR "/weapons/lmg reload.wav", HIGHVOLUME, 1, MIDDLEPAN);
  } else
    uiLastSoundFxTime = GetJA2Clock();

  if ((uiLastSpeechTime - OPT_MUSIC_SLIDER_PLAY_SOUND_DELAY) > guiSpeechSliderMoving) {
    guiSpeechSliderMoving = 0xffffffff;

    if (!SoundIsPlaying(uiLastPlayingSpeechID))
      uiLastPlayingSpeechID =
          PlayJA2GapSample(BATTLESNDSDIR "/m_cool.wav", HIGHVOLUME, 1, MIDDLEPAN, NULL);
  } else
    uiLastSpeechTime = GetJA2Clock();
}

static void SelectedOptionTextRegionCallBack(MOUSE_REGION *pRegion, int32_t iReason) {
  uint8_t ubButton = (uint8_t)MSYS_GetRegionUserData(pRegion, 0);

  if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    HandleOptionToggle(ubButton, !gGameSettings.fOptions[ubButton], FALSE, true);
    InvalidateRegion(pRegion->RegionTopLeftX, pRegion->RegionTopLeftY, pRegion->RegionBottomRightX,
                     pRegion->RegionBottomRightY);
  } else if (iReason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
    HandleOptionToggle(ubButton, gGameSettings.fOptions[ubButton], TRUE, true);
  }
}

static void SelectedOptionTextRegionMovementCallBack(MOUSE_REGION *pRegion, int32_t reason) {
  int8_t bButton = (int8_t)MSYS_GetRegionUserData(pRegion, 0);

  if (reason & MSYS_CALLBACK_REASON_LOST_MOUSE) {
    HandleHighLightedText(FALSE);

    gbHighLightedOptionText = -1;

    InvalidateRegion(pRegion->RegionTopLeftX, pRegion->RegionTopLeftY, pRegion->RegionBottomRightX,
                     pRegion->RegionBottomRightY);
  } else if (reason & MSYS_CALLBACK_REASON_GAIN_MOUSE) {
    gbHighLightedOptionText = bButton;

    InvalidateRegion(pRegion->RegionTopLeftX, pRegion->RegionTopLeftY, pRegion->RegionBottomRightX,
                     pRegion->RegionBottomRightY);
  }
}

static void HandleHighLightedText(BOOLEAN fHighLight) {
  uint16_t usPosX = 0;
  uint16_t usPosY = 0;
  uint8_t ubCnt;
  int8_t bHighLight = -1;
  uint16_t usWidth;

  static int8_t bLastRegion = -1;

  if (gbHighLightedOptionText == -1) fHighLight = FALSE;

  // if the user has the mouse in one of the checkboxes
  for (ubCnt = 0; ubCnt < NUM_GAME_OPTIONS; ubCnt++) {
    if (guiOptionsToggles[ubCnt]->Area.uiFlags & MSYS_MOUSE_IN_AREA) {
      gbHighLightedOptionText = ubCnt;
      fHighLight = TRUE;
    }
  }

  // If there is a valid section being highlighted
  if (gbHighLightedOptionText != -1) {
    bLastRegion = gbHighLightedOptionText;
  }

  bHighLight = gbHighLightedOptionText;

  if (bLastRegion != -1 && gbHighLightedOptionText == -1) {
    fHighLight = FALSE;
    bHighLight = bLastRegion;
    bLastRegion = -1;
  }

  if (bHighLight != -1) {
    if (bHighLight < OPT_FIRST_COLUMN_TOGGLE_CUT_OFF) {
      usPosX = OPT_TOGGLE_BOX_FIRST_COL_TEXT_X;
      usPosY = OPT_TOGGLE_BOX_START_Y + OPT_TOGGLE_TEXT_OFFSET_Y +
               bHighLight * OPT_GAP_BETWEEN_TOGGLE_BOXES;
    } else {
      usPosX = OPT_TOGGLE_BOX_SECOND_TEXT_X;
      usPosY = OPT_TOGGLE_BOX_START_Y + OPT_TOGGLE_TEXT_OFFSET_Y +
               (bHighLight - OPT_FIRST_COLUMN_TOGGLE_CUT_OFF) * OPT_GAP_BETWEEN_TOGGLE_BOXES;
    }

    usWidth = StringPixLength(zOptionsToggleText[bHighLight], OPT_MAIN_FONT);

    // if the string is going to wrap, move the string up a bit
    uint8_t color = fHighLight ? OPT_HIGHLIGHT_COLOR : OPT_MAIN_COLOR;
    if (usWidth > OPT_TOGGLE_BOX_TEXT_WIDTH) {
      DisplayWrappedString(usPosX, usPosY, OPT_TOGGLE_BOX_TEXT_WIDTH, 2, OPT_MAIN_FONT, color,
                           zOptionsToggleText[bHighLight], FONT_MCOLOR_BLACK,
                           LEFT_JUSTIFIED | MARK_DIRTY);
    } else {
      DrawTextToScreen(zOptionsToggleText[bHighLight], usPosX, usPosY, 0, OPT_MAIN_FONT, color,
                       FONT_MCOLOR_BLACK, LEFT_JUSTIFIED | MARK_DIRTY);
    }
  }
}

static void SelectedToggleBoxAreaRegionMovementCallBack(MOUSE_REGION *pRegion, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_GAIN_MOUSE) {
    uint8_t ubCnt;

    // loop through all the toggle box's and remove the in area flag
    for (ubCnt = 0; ubCnt < NUM_GAME_OPTIONS; ubCnt++) {
      guiOptionsToggles[ubCnt]->Area.uiFlags &= ~MSYS_MOUSE_IN_AREA;
    }

    gbHighLightedOptionText = -1;

    InvalidateRegion(pRegion->RegionTopLeftX, pRegion->RegionTopLeftY, pRegion->RegionBottomRightX,
                     pRegion->RegionBottomRightY);
  }
}
