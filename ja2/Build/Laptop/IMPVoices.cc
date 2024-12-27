// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Laptop/IMPVoices.h"

#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "Directories.h"
#include "Laptop/CharProfile.h"
#include "Laptop/IMPMainPage.h"
#include "Laptop/IMPTextSystem.h"
#include "Laptop/IMPVideoObjects.h"
#include "Laptop/Laptop.h"
#include "Laptop/LaptopSave.h"
#include "Macro.h"
#include "SGP/ButtonSystem.h"
#include "SGP/Font.h"
#include "SGP/Input.h"
#include "SGP/SoundMan.h"
#include "TileEngine/RenderDirty.h"
#include "Utils/Cursors.h"
#include "Utils/FontControl.h"
#include "Utils/SoundControl.h"
#include "Utils/Text.h"

// current and last pages
int32_t iCurrentVoices = 0;
static int32_t const iLastVoice = 2;

// int32_t iVoiceId = 0;

static uint32_t uiVocVoiceSound = 0;
// buttons needed for the IMP Voices screen
static GUIButtonRef giIMPVoicesButton[3];
static BUTTON_PICS *giIMPVoicesButtonImage[3];

// redraw protrait screen
static BOOLEAN fReDrawVoicesScreenFlag = FALSE;

// the portrait region, for player to click on and re-hear voice
static MOUSE_REGION gVoicePortraitRegion;

static void CreateIMPVoiceMouseRegions();
static void CreateIMPVoicesButtons();
static void PlayVoice();

void EnterIMPVoices() {
  // create buttons
  CreateIMPVoicesButtons();

  // create mouse regions
  CreateIMPVoiceMouseRegions();

  // render background
  RenderIMPVoices();

  // play voice once
  PlayVoice();
}

static void RenderVoiceIndex();

void RenderIMPVoices() {
  // render background
  RenderProfileBackGround();

  // the Voices frame
  RenderPortraitFrame(191, 167);

  // the sillouette
  RenderLargeSilhouette(200, 176);

  // indent for the text
  RenderAttrib1IndentFrame(128, 65);

  // render voice index value
  RenderVoiceIndex();

  // text
  PrintImpText();
}

static void DestroyIMPVoiceMouseRegions();
static void DestroyIMPVoicesButtons();

void ExitIMPVoices() {
  // destroy buttons for IMP Voices page
  DestroyIMPVoicesButtons();

  // destroy mouse regions for this screen
  DestroyIMPVoiceMouseRegions();
}

void HandleIMPVoices() {
  // do we need to re write screen
  if (fReDrawVoicesScreenFlag) {
    RenderIMPVoices();

    // reset redraw flag
    fReDrawVoicesScreenFlag = FALSE;
  }
}

static void IncrementVoice() {
  // cycle to next voice

  iCurrentVoices++;

  // gone too far?
  if (iCurrentVoices > iLastVoice) {
    iCurrentVoices = 0;
  }
}

static void DecrementVoice() {
  // cycle to previous voice

  iCurrentVoices--;

  // gone too far?
  if (iCurrentVoices < 0) {
    iCurrentVoices = iLastVoice;
  }
}

static void MakeButton(uint32_t idx, const char *img_file, int32_t off_normal, int32_t on_normal,
                       const wchar_t *text, int16_t x, int16_t y, GUI_CALLBACK click) {
  BUTTON_PICS *const img = LoadButtonImage(img_file, off_normal, on_normal);
  giIMPVoicesButtonImage[idx] = img;
  const int16_t text_col = FONT_WHITE;
  const int16_t shadow_col = DEFAULT_SHADOW;
  GUIButtonRef const btn =
      CreateIconAndTextButton(img, text, FONT12ARIAL, text_col, shadow_col, text_col, shadow_col, x,
                              y, MSYS_PRIORITY_HIGH, click);
  giIMPVoicesButton[idx] = btn;
  btn->SetCursor(CURSOR_WWW);
}

static void BtnIMPVoicesDoneCallback(GUI_BUTTON *btn, int32_t reason);
static void BtnIMPVoicesNextCallback(GUI_BUTTON *btn, int32_t reason);
static void BtnIMPVoicesPreviousCallback(GUI_BUTTON *btn, int32_t reason);

static void CreateIMPVoicesButtons() {
  // will create buttons need for the IMP Voices screen
  const int16_t dx = LAPTOP_SCREEN_UL_X;
  const int16_t dy = LAPTOP_SCREEN_WEB_UL_Y;
  MakeButton(0, LAPTOPDIR "/voicearrows.sti", 1, 3, pImpButtonText[13], dx + 343, dy + 205,
             BtnIMPVoicesNextCallback);  // Next button
  MakeButton(1, LAPTOPDIR "/voicearrows.sti", 0, 2, pImpButtonText[12], dx + 93, dy + 205,
             BtnIMPVoicesPreviousCallback);  // Previous button
  MakeButton(2, LAPTOPDIR "/button_5.sti", 0, 1, pImpButtonText[11], dx + 187, dy + 330,
             BtnIMPVoicesDoneCallback);  // Done button
}

static void DestroyIMPVoicesButtons() {
  // will destroy buttons created for IMP Voices screen

  // the next button
  RemoveButton(giIMPVoicesButton[0]);
  UnloadButtonImage(giIMPVoicesButtonImage[0]);

  // the previous button
  RemoveButton(giIMPVoicesButton[1]);
  UnloadButtonImage(giIMPVoicesButtonImage[1]);

  // the done button
  RemoveButton(giIMPVoicesButton[2]);
  UnloadButtonImage(giIMPVoicesButtonImage[2]);
}

static void BtnIMPVoicesNextCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    IncrementVoice();
    if (SoundIsPlaying(uiVocVoiceSound)) SoundStop(uiVocVoiceSound);
    PlayVoice();
    fReDrawVoicesScreenFlag = TRUE;
  }
}

static void BtnIMPVoicesPreviousCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    DecrementVoice();
    if (SoundIsPlaying(uiVocVoiceSound)) SoundStop(uiVocVoiceSound);
    PlayVoice();
    fReDrawVoicesScreenFlag = TRUE;
  }
}

static void BtnIMPVoicesDoneCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    iCurrentImpPage = IMP_MAIN_PAGE;

    // if we are already done, leave
    if (iCurrentProfileMode == 5) {
      iCurrentImpPage = IMP_FINISH;
    } else if (iCurrentProfileMode < 4) {
      // current mode now is voice
      iCurrentProfileMode = 4;
    } else if (iCurrentProfileMode == 4) {
      // all done profiling
      iCurrentProfileMode = 5;
      iCurrentImpPage = IMP_FINISH;
    }

    // set voice id, to grab character slot
    LaptopSaveInfo.iVoiceId = iCurrentVoices + (fCharacterIsMale ? 0 : 3);

    // set button up image pending
    fButtonPendingFlag = TRUE;
  }
}

static void PlayVoice() {
  char const *filename;
  if (fCharacterIsMale) {
    switch (iCurrentVoices) {
      case 0:
        filename = SPEECHDIR "/051_001.wav";
        break;
      case 1:
        filename = SPEECHDIR "/052_001.wav";
        break;
      case 2:
        filename = SPEECHDIR "/053_001.wav";
        break;
      default:
        return;
    }
  } else {
    switch (iCurrentVoices) {
      case 0:
        filename = SPEECHDIR "/054_001.wav";
        break;
      case 1:
        filename = SPEECHDIR "/055_001.wav";
        break;
      case 2:
        filename = SPEECHDIR "/056_001.wav";
        break;
      default:
        return;
    }
  }
  uiVocVoiceSound = PlayJA2SampleFromFile(filename, MIDVOLUME, 1, MIDDLEPAN);
}

static void IMPPortraitRegionButtonCallback(MOUSE_REGION *pRegion, int32_t iReason);

static void CreateIMPVoiceMouseRegions() {
  // will create mouse regions needed for the IMP voices page
  MSYS_DefineRegion(&gVoicePortraitRegion, LAPTOP_SCREEN_UL_X + 200, LAPTOP_SCREEN_WEB_UL_Y + 176,
                    LAPTOP_SCREEN_UL_X + 200 + 100, LAPTOP_SCREEN_WEB_UL_Y + 176 + 100,
                    MSYS_PRIORITY_HIGH, MSYS_NO_CURSOR, MSYS_NO_CALLBACK,
                    IMPPortraitRegionButtonCallback);
}

static void DestroyIMPVoiceMouseRegions() {
  // will destroy already created mouse reiogns for IMP voices page
  MSYS_RemoveRegion(&gVoicePortraitRegion);
}

static void IMPPortraitRegionButtonCallback(MOUSE_REGION *pRegion, int32_t iReason) {
  // callback handler for imp portrait region button events
  if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    if (!SoundIsPlaying(uiVocVoiceSound)) {
      PlayVoice();
    }
  }
}

static void RenderVoiceIndex() {
  wchar_t sString[32];
  int16_t sX, sY;

  // render the voice index value on the the blank portrait
  swprintf(sString, lengthof(sString), L"%ls %d", pIMPVoicesStrings, iCurrentVoices + 1);
  FindFontCenterCoordinates(290 + LAPTOP_UL_X, 0, 100, 0, sString, FONT12ARIAL, &sX, &sY);
  SetFontAttributes(FONT12ARIAL, FONT_WHITE);
  MPrint(sX, 320, sString);
}
