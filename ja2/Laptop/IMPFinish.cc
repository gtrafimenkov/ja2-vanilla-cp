// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Laptop/IMPFinish.h"

#include <string.h>
#include <wchar.h>

#include "Directories.h"
#include "Laptop/CharProfile.h"
#include "Laptop/IMPAttributeSelection.h"
#include "Laptop/IMPCompileCharacter.h"
#include "Laptop/IMPMainPage.h"
#include "Laptop/IMPPortraits.h"
#include "Laptop/IMPVideoObjects.h"
#include "Laptop/IMPVoices.h"
#include "Laptop/Laptop.h"
#include "Laptop/LaptopSave.h"
#include "Macro.h"
#include "MessageBoxScreen.h"
#include "SGP/ButtonSystem.h"
#include "SGP/Font.h"
#include "SGP/SoundMan.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "ScreenIDs.h"
#include "TileEngine/RenderDirty.h"
#include "Utils/Cursors.h"
#include "Utils/FontControl.h"
#include "Utils/Text.h"
#include "Utils/TimerControl.h"

// min time btween frames of animation
#define ANIMATE_MIN_TIME 200

// buttons
static BUTTON_PICS *giIMPFinishButtonImage[6];
GUIButtonRef giIMPFinishButton[6];

// we are in fact done
BOOLEAN fFinishedCharGeneration = FALSE;

// image handle
SGPVObject *guiCHARACTERPORTRAIT;
extern int32_t iCurrentVoices;

extern void BtnIMPMainPageVoiceCallback(GUI_BUTTON *btn, int32_t reason);
extern void BtnIMPMainPagePortraitCallback(GUI_BUTTON *btn, int32_t reason);

static void CreateIMPFinishButtons();
static void LoadCharacterPortrait();

void EnterIMPFinish() {
  // load graphic for portrait
  LoadCharacterPortrait();

  //	CREATE buttons for IMP finish screen
  CreateIMPFinishButtons();

  // set review mode
  fReviewStats = TRUE;
  iCurrentProfileMode = 5;

  // note that we are in fact done char generation
  fFinishedCharGeneration = TRUE;
}

static void RenderCharFullName();

void RenderIMPFinish() {
  // the background
  RenderProfileBackGround();

  // render merc fullname
  RenderCharFullName();

  // indent for text
  RenderBeginIndent(110, 50);
}

static void DeleteIMPFinishButtons();
static void DestroyCharacterPortrait();

void ExitIMPFinish() {
  // remove buttons for IMP finish screen
  DeleteIMPFinishButtons();

  // get rid of graphic for portrait
  DestroyCharacterPortrait();
}

void HandleIMPFinish() {}

static void MakeButton(uint32_t idx, const char *img_file, const wchar_t *text, int16_t x,
                       int16_t y, GUI_CALLBACK click) {
  BUTTON_PICS *const img = LoadButtonImage(img_file, 0, 1);
  giIMPFinishButtonImage[idx] = img;
  const int16_t text_col = FONT_WHITE;
  const int16_t shadow_col = DEFAULT_SHADOW;
  GUIButtonRef const btn =
      CreateIconAndTextButton(img, text, FONT12ARIAL, text_col, shadow_col, text_col, shadow_col, x,
                              y, MSYS_PRIORITY_HIGH, click);
  giIMPFinishButton[idx] = btn;
  btn->SetCursor(CURSOR_WWW);
}

static void BtnIMPFinishAttributesCallback(GUI_BUTTON *btn, int32_t reason);
static void BtnIMPFinishDoneCallback(GUI_BUTTON *btn, int32_t reason);
static void BtnIMPFinishPersonalityCallback(GUI_BUTTON *btn, int32_t reason);
static void BtnIMPFinishStartOverCallback(GUI_BUTTON *btn, int32_t reason);

static void CreateIMPFinishButtons() {
  // this function will create the buttons needed for th IMP about us page
  const int16_t dx = LAPTOP_SCREEN_UL_X;
  const int16_t dy = LAPTOP_SCREEN_WEB_UL_Y;

  // the start over button button
  MakeButton(0, LAPTOPDIR "/button_2.sti", pImpButtonText[7], dx + 136, dy + 174,
             BtnIMPFinishStartOverCallback);

  // the done button
  MakeButton(1, LAPTOPDIR "/button_2.sti", pImpButtonText[6], dx + 136, dy + 114,
             BtnIMPFinishDoneCallback);

  // the personality button
  MakeButton(2, LAPTOPDIR "/button_8.sti", pImpButtonText[2], dx + 13, dy + 245,
             BtnIMPFinishPersonalityCallback);
  giIMPFinishButton[2]->SpecifyIcon(guiANALYSE, 0, 33, 23, FALSE);

  // the attribs button
  MakeButton(3, LAPTOPDIR "/button_8.sti", pImpButtonText[3], dx + 133, dy + 245,
             BtnIMPFinishAttributesCallback);
  giIMPFinishButton[3]->SpecifyIcon(guiATTRIBUTEGRAPH, 0, 25, 25, FALSE);

  // the portrait button
  MakeButton(4, LAPTOPDIR "/button_8.sti", pImpButtonText[4], dx + 253, dy + 245,
             BtnIMPMainPagePortraitCallback);
  giIMPFinishButton[4]->SpecifyIcon(guiCHARACTERPORTRAIT, 0, 33, 23, FALSE);

  // the voice button
  wchar_t sString[128];
  swprintf(sString, lengthof(sString), pImpButtonText[5], iCurrentVoices + 1);
  MakeButton(5, LAPTOPDIR "/button_8.sti", sString, dx + 373, dy + 245,
             BtnIMPMainPageVoiceCallback);
  giIMPFinishButton[5]->SpecifyIcon(guiSMALLSILHOUETTE, 0, 33, 23, FALSE);
}

static void DeleteIMPFinishButtons() {
  // this function destroys the buttons needed for the IMP about Us Page

  // the back  button
  RemoveButton(giIMPFinishButton[0]);
  UnloadButtonImage(giIMPFinishButtonImage[0]);

  // begin profiling button
  RemoveButton(giIMPFinishButton[1]);
  UnloadButtonImage(giIMPFinishButtonImage[1]);

  // begin personna button
  RemoveButton(giIMPFinishButton[2]);
  UnloadButtonImage(giIMPFinishButtonImage[2]);

  // begin attribs button
  RemoveButton(giIMPFinishButton[3]);
  UnloadButtonImage(giIMPFinishButtonImage[3]);

  // begin portrait button
  RemoveButton(giIMPFinishButton[4]);
  UnloadButtonImage(giIMPFinishButtonImage[4]);

  // begin voice button
  RemoveButton(giIMPFinishButton[5]);
  UnloadButtonImage(giIMPFinishButtonImage[5]);
}

static void FinishMessageBoxCallBack(MessageBoxReturnValue);

static void BtnIMPFinishStartOverCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    DoLapTopMessageBox(MSG_BOX_IMP_STYLE, pImpPopUpStrings[1], LAPTOP_SCREEN, MSG_BOX_FLAG_YESNO,
                       FinishMessageBoxCallBack);
  }
}

static void BtnIMPFinishDoneCallback(GUI_BUTTON *btn, int32_t reason) {
  // btn callback for Main Page Begin Profiling
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    iCurrentImpPage = IMP_CONFIRM;
    CreateACharacterFromPlayerEnteredStats();
    fButtonPendingFlag = TRUE;
    iCurrentProfileMode = 0;
    fFinishedCharGeneration = FALSE;
    // ResetCharacterStats();
  }
}

static void BtnIMPFinishPersonalityCallback(GUI_BUTTON *btn, int32_t reason) {
  // btn callback for Main Page Begin Profiling
  static BOOLEAN fAnimateFlag = FALSE;
  static uint32_t uiBaseTime = 0;
  static BOOLEAN fState = 0;

  int32_t iDifference = 0;

  if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
    uiBaseTime = GetJA2Clock();
    btn->SpecifyText(pImpButtonText[23]);
    fAnimateFlag = TRUE;
  } else if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    fButtonPendingFlag = TRUE;
    uiBaseTime = 0;
    fAnimateFlag = FALSE;
    btn->SpecifyText(pImpButtonText[2]);
  }

  // get amount of time between callbacks
  iDifference = GetJA2Clock() - uiBaseTime;

  if (fAnimateFlag) {
    if (iDifference > ANIMATE_MIN_TIME) {
      uiBaseTime = GetJA2Clock();
      fState = !fState;
      btn->SpecifyIcon(guiANALYSE, fState ? 0 : 1, 33, 23, FALSE);
    }
  }
}

static void BtnIMPFinishAttributesCallback(GUI_BUTTON *btn, int32_t reason) {
  // btn callback for Main Page Begin Profiling

  // if not this far in char generation, don't alot ANY action
  if (iCurrentProfileMode < 2) {
    btn->uiFlags &= ~BUTTON_CLICKED_ON;
    return;
  }

  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    iCurrentImpPage = IMP_ATTRIBUTE_PAGE;
    fButtonPendingFlag = TRUE;
    giIMPFinishButton[2]->SpecifyText(pImpButtonText[2]);
  }
}

static void RenderCharFullName() {
  wchar_t sString[64];
  int16_t sX, sY;

  // render the characters full name
  SetFontAttributes(FONT14ARIAL, FONT_WHITE);
  swprintf(sString, lengthof(sString), pIMPFinishStrings, pFullName);
  FindFontCenterCoordinates(LAPTOP_SCREEN_UL_X, 0, LAPTOP_SCREEN_LR_X - LAPTOP_SCREEN_UL_X, 0,
                            sString, FONT14ARIAL, &sX, &sY);
  MPrint(sX, LAPTOP_SCREEN_WEB_DELTA_Y + 33, sString);
}

static void LoadCharacterPortrait() {
  // this function will load the character's portrait, to be used on portrait
  // button load it
  guiCHARACTERPORTRAIT = LoadIMPPortait();
}

static void DestroyCharacterPortrait() {
  // remove the portrait that was loaded by loadcharacterportrait
  DeleteVideoObject(guiCHARACTERPORTRAIT);
}

static void FinishMessageBoxCallBack(MessageBoxReturnValue const bExitValue) {
  // yes, so start over, else stay here and do nothing for now
  if (bExitValue == MSG_BOX_RETURN_YES) {
    iCurrentImpPage = IMP_HOME_PAGE;
    fButtonPendingFlag = TRUE;
    iCurrentProfileMode = 0;
    fFinishedCharGeneration = FALSE;
    ResetCharacterStats();
  }

  if (bExitValue == MSG_BOX_RETURN_NO) {
    MarkButtonsDirty();
  }
}
