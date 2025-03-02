// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Laptop/IMPMainPage.h"

#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "Directories.h"
#include "Laptop/CharProfile.h"
#include "Laptop/Finances.h"
#include "Laptop/IMPAttributeSelection.h"
#include "Laptop/IMPFinish.h"
#include "Laptop/IMPPortraits.h"
#include "Laptop/IMPVideoObjects.h"
#include "Laptop/Laptop.h"
#include "Laptop/LaptopSave.h"
#include "Macro.h"
#include "MessageBoxScreen.h"
#include "SGP/ButtonSystem.h"
#include "SGP/Font.h"
#include "SGP/MouseSystem.h"
#include "SGP/VObject.h"
#include "ScreenIDs.h"
#include "Tactical/MercHiring.h"
#include "Tactical/Overhead.h"
#include "TileEngine/RenderDirty.h"
#include "Utils/Cursors.h"
#include "Utils/FontControl.h"
#include "Utils/Text.h"

#define MAIN_PAGE_BUTTON_TEXT_WIDTH 95

// main page buttons
static BUTTON_PICS *giIMPMainPageButtonImage[6];
GUIButtonRef giIMPMainPageButton[6];

extern int32_t iCurrentVoices;

// mouse regions for not entablable warning
static MOUSE_REGION pIMPMainPageMouseRegions[4];

static SGPVObject *guiCHARACTERPORTRAITFORMAINPAGE;

void BtnIMPMainPagePortraitCallback(GUI_BUTTON *btn, int32_t reason);
void BtnIMPMainPageVoiceCallback(GUI_BUTTON *btn, int32_t reason);

// this is the current state of profiling the player is in.
/*
  0 - Beginning
        1 - Personnality
        2 - Attributes and Skills
        3 - Portrait
        4 - Voice
        5 - Done
        */
int32_t iCurrentProfileMode = 0;

static void CreateIMPMainPageButtons();
static void CreateMouseRegionsForIMPMainPageBasedOnCharGenStatus();
static void LoadCharacterPortraitForMainPage();
static void UpDateIMPMainPageButtons();

void EnterIMPMainPage() {
  // turn off review mode
  fReviewStats = FALSE;

  // create buttons
  CreateIMPMainPageButtons();

  // load portrait for face button, if applicable
  LoadCharacterPortraitForMainPage();

  // create button masks for this screen
  CreateMouseRegionsForIMPMainPageBasedOnCharGenStatus();

  // alter states
  UpDateIMPMainPageButtons();

  // entry into IMP about us page
  RenderIMPMainPage();
}

static void DeleteIMPMainPageButtons();
static void DestoryMouseRegionsForIMPMainPageBasedOnCharGenStatus();

void ExitIMPMainPage() {
  // exit from IMP About us page

  // delete Buttons
  DeleteIMPMainPageButtons();
  DestoryMouseRegionsForIMPMainPageBasedOnCharGenStatus();
}

void RenderIMPMainPage() {
  // rneders the IMP about us page

  // the background
  RenderProfileBackGround();

  // the IMP symbol
  // RenderIMPSymbol( 106, 1 );
  // indent
  RenderMainIndentFrame(164, 74);
}

static BOOLEAN CheckIfFinishedCharacterGeneration();

void HandleIMPMainPage() {
  // handles the IMP about main page

  if (CheckIfFinishedCharacterGeneration()) {
    iCurrentImpPage = IMP_FINISH;
  }
}

static void MakeButton(uint32_t idx, const char *img_file, const wchar_t *text, int16_t x,
                       int16_t y, GUI_CALLBACK click) {
  BUTTON_PICS *const img = LoadButtonImage(img_file, 0, 1);
  giIMPMainPageButtonImage[idx] = img;
  const int16_t text_col = FONT_WHITE;
  const int16_t shadow_col = DEFAULT_SHADOW;
  GUIButtonRef const btn =
      CreateIconAndTextButton(img, text, FONT12ARIAL, text_col, shadow_col, text_col, shadow_col, x,
                              y, MSYS_PRIORITY_HIGH, click);
  giIMPMainPageButton[idx] = btn;
  btn->SetCursor(CURSOR_WWW);
  if (idx >= 2) {
    btn->SpecifyTextOffsets(10, 40, TRUE);
    btn->SpecifyTextWrappedWidth(MAIN_PAGE_BUTTON_TEXT_WIDTH);
  }
}

static void BtnIMPMainPageAttributesCallback(GUI_BUTTON *btn, int32_t reason);
static void BtnIMPMainPageBackCallback(GUI_BUTTON *btn, int32_t reason);
static void BtnIMPMainPageBeginCallback(GUI_BUTTON *btn, int32_t reason);
static void BtnIMPMainPagePersonalityCallback(GUI_BUTTON *btn, int32_t reason);

static void CreateIMPMainPageButtons() {
  // this function will create the buttons needed for th IMP about us page
  const int16_t dx = LAPTOP_SCREEN_UL_X;
  const int16_t dy = LAPTOP_SCREEN_WEB_UL_Y;

  // the back button button
  MakeButton(0, LAPTOPDIR "/button_3.sti", pImpButtonText[19], dx + 15, dy + 360,
             BtnIMPMainPageBackCallback);
  giIMPMainPageButton[0]->SpecifyTextSubOffsets(0, -1, FALSE);

  // the begin profiling button
  const wchar_t *const profiling_text =
      (iCurrentProfileMode == 0 || iCurrentProfileMode > 2 ? pImpButtonText[1]
                                                           : pImpButtonText[22]);
  MakeButton(1, LAPTOPDIR "/button_2.sti", profiling_text, dx + 136, dy + 174,
             BtnIMPMainPageBeginCallback);

  // the personality button
  MakeButton(2, LAPTOPDIR "/button_8.sti", pImpButtonText[2], dx + 13, dy + 245,
             BtnIMPMainPagePersonalityCallback);

  // the attribs button
  MakeButton(3, LAPTOPDIR "/button_8.sti", pImpButtonText[3], dx + 133, dy + 245,
             BtnIMPMainPageAttributesCallback);

  // the portrait button
  MakeButton(4, LAPTOPDIR "/button_8.sti", pImpButtonText[4], dx + 253, dy + 245,
             BtnIMPMainPagePortraitCallback);

  // the voice button
  wchar_t sString[128];
  if (iCurrentProfileMode == 5) {
    swprintf(sString, lengthof(sString), pImpButtonText[5], iCurrentVoices + 1);
  } else {
    swprintf(sString, lengthof(sString), pImpButtonText[25]);
  }
  MakeButton(5, LAPTOPDIR "/button_8.sti", sString, dx + 373, dy + 245,
             BtnIMPMainPageVoiceCallback);
}

static void DeleteIMPMainPageButtons() {
  // this function destroys the buttons needed for the IMP about Us Page

  // the back  button
  RemoveButton(giIMPMainPageButton[0]);
  UnloadButtonImage(giIMPMainPageButtonImage[0]);

  // begin profiling button
  RemoveButton(giIMPMainPageButton[1]);
  UnloadButtonImage(giIMPMainPageButtonImage[1]);

  // begin personna button
  RemoveButton(giIMPMainPageButton[2]);
  UnloadButtonImage(giIMPMainPageButtonImage[2]);

  // begin attribs button
  RemoveButton(giIMPMainPageButton[3]);
  UnloadButtonImage(giIMPMainPageButtonImage[3]);

  // begin portrait button
  RemoveButton(giIMPMainPageButton[4]);
  UnloadButtonImage(giIMPMainPageButtonImage[4]);

  // begin voice button
  RemoveButton(giIMPMainPageButton[5]);
  UnloadButtonImage(giIMPMainPageButtonImage[5]);
}

static void BtnIMPMainPageBackCallback(GUI_BUTTON *btn, int32_t reason) {
  // btn callback for IMP Homepage About US button

  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    iCurrentImpPage = IMP_HOME_PAGE;
    fButtonPendingFlag = TRUE;
    iCurrentProfileMode = 0;
    fFinishedCharGeneration = FALSE;
    ResetCharacterStats();
  }
}

static void BeginMessageBoxCallBack(MessageBoxReturnValue);

static void BtnIMPMainPageBeginCallback(GUI_BUTTON *btn, int32_t reason) {
  // btn callback for Main Page Begin Profiling

  // too far along to change gender

  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    // are we going to change name, or do we have to start over from scratch
    if (iCurrentProfileMode > 2) {
      // too far along, restart
      DoLapTopMessageBox(MSG_BOX_IMP_STYLE, pImpPopUpStrings[1], LAPTOP_SCREEN, MSG_BOX_FLAG_YESNO,
                         BeginMessageBoxCallBack);
    } else {
      if (LaptopSaveInfo.iCurrentBalance < COST_OF_PROFILE) {
        DoLapTopMessageBox(MSG_BOX_IMP_STYLE, pImpPopUpStrings[3], LAPTOP_SCREEN, MSG_BOX_FLAG_OK,
                           BeginMessageBoxCallBack);
      } else if (NumberOfMercsOnPlayerTeam() >= 18) {
        DoLapTopMessageBox(MSG_BOX_IMP_STYLE, pImpPopUpStrings[5], LAPTOP_SCREEN, MSG_BOX_FLAG_OK,
                           BeginMessageBoxCallBack);
      } else {
        // change name
        iCurrentImpPage = IMP_BEGIN;
        fButtonPendingFlag = TRUE;
      }
    }
  }
}

static void BtnIMPMainPagePersonalityCallback(GUI_BUTTON *btn, int32_t reason) {
  // btn callback for Main Page Begin Profiling

  // if not this far in char generation, don't alot ANY action
  if (iCurrentProfileMode != 1) {
    btn->uiFlags &= ~BUTTON_CLICKED_ON;
    return;
  }

  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    iCurrentImpPage = IMP_PERSONALITY;
    fButtonPendingFlag = TRUE;
  }
}

static void BtnIMPMainPageAttributesCallback(GUI_BUTTON *btn, int32_t reason) {
  // btn callback for Main Page Begin Profiling

  // if not this far in char generation, don't alot ANY action
  if (iCurrentProfileMode < 2) {
    btn->uiFlags &= ~BUTTON_CLICKED_ON;
    return;
  }

  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    iCurrentImpPage = IMP_ATTRIBUTE_ENTRANCE;
    fButtonPendingFlag = TRUE;
  }
}

void BtnIMPMainPagePortraitCallback(GUI_BUTTON *btn, int32_t reason) {
  // btn callback for Main Page Begin Profiling

  // if not this far in char generation, don't alot ANY action
  if (iCurrentProfileMode != 3 && iCurrentProfileMode != 4 && iCurrentProfileMode > 5) {
    btn->uiFlags &= ~BUTTON_CLICKED_ON;
    return;
  }

  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    iCurrentImpPage = IMP_PORTRAIT;
    fButtonPendingFlag = TRUE;
  }
}

void BtnIMPMainPageVoiceCallback(GUI_BUTTON *btn, int32_t reason) {
  // btn callback for Main Page Begin Profiling

  // if not this far in char generation, don't alot ANY action
  if (iCurrentProfileMode != 4 && iCurrentProfileMode > 5) {
    btn->uiFlags &= ~BUTTON_CLICKED_ON;
    return;
  }

  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    iCurrentImpPage = IMP_VOICE;
    fButtonPendingFlag = TRUE;
  }
}

static BOOLEAN CheckIfFinishedCharacterGeneration() {
  // this function checks to see if character is done character generation

  // are we done character generation
  if (iCurrentProfileMode == 5) {
    // yes
    return (TRUE);
  } else {
    // no
    return (FALSE);
  }
}

static void UpDateIMPMainPageButtons() {
  // update mainpage button states
  int32_t iCount = 0;

  // disable all
  for (iCount = 2; iCount < 6; iCount++) {
    DisableButton(giIMPMainPageButton[iCount]);
  }

  for (iCount = 0; iCount < 4; iCount++) {
    pIMPMainPageMouseRegions[iCount].Disable();
  }
  // enable
  switch (iCurrentProfileMode) {
    case 0:
      pIMPMainPageMouseRegions[0].Enable();
      pIMPMainPageMouseRegions[1].Enable();
      pIMPMainPageMouseRegions[2].Enable();
      pIMPMainPageMouseRegions[3].Enable();
      break;
    case (1):
      EnableButton(giIMPMainPageButton[2]);
      pIMPMainPageMouseRegions[1].Enable();
      pIMPMainPageMouseRegions[2].Enable();
      pIMPMainPageMouseRegions[3].Enable();
      break;
    case (2):
      EnableButton(giIMPMainPageButton[3]);
      pIMPMainPageMouseRegions[0].Enable();
      pIMPMainPageMouseRegions[2].Enable();
      pIMPMainPageMouseRegions[3].Enable();
      break;
    case (3):
      EnableButton(giIMPMainPageButton[3]);
      EnableButton(giIMPMainPageButton[4]);
      pIMPMainPageMouseRegions[0].Enable();
      // pIMPMainPageMouseRegions[1].Enable();
      pIMPMainPageMouseRegions[3].Enable();
      break;

    case (4):
      // pIMPMainPageMouseRegions[1].Enable();
      pIMPMainPageMouseRegions[0].Enable();
      EnableButton(giIMPMainPageButton[3]);
      EnableButton(giIMPMainPageButton[4]);
      EnableButton(giIMPMainPageButton[5]);
      break;
  }
}

static void BeginMessageBoxCallBack(MessageBoxReturnValue const bExitValue) {
  // yes, so start over, else stay here and do nothing for now
  if (bExitValue == MSG_BOX_RETURN_YES) {
    iCurrentImpPage = IMP_BEGIN;
    iCurrentProfileMode = 0;
  }

  else if (bExitValue == MSG_BOX_RETURN_OK) {
    // if ok, then we are coming from financial warning, allow continue
  }
}

static void IMPMainPageNotSelectableBtnCallback(MOUSE_REGION *pRegion, int32_t iReason);

static void CreateMouseRegionsForIMPMainPageBasedOnCharGenStatus() {
  // this procedure will create masks for the char generation main page
  /* create masks for the personality, attrib, portrait and page buttons on the
   * character generation main page */
  uint16_t x = LAPTOP_SCREEN_UL_X + 13;
  uint16_t const y = LAPTOP_SCREEN_WEB_UL_Y + 245;
  uint16_t const w = 115;
  uint16_t const h = 93;
  FOR_EACHX(MOUSE_REGION, r, pIMPMainPageMouseRegions, x += 120) {
    MSYS_DefineRegion(r, x, y, x + w, y + h, MSYS_PRIORITY_HIGH + 5, CURSOR_WWW, MSYS_NO_CALLBACK,
                      IMPMainPageNotSelectableBtnCallback);
  }
}

static void DestoryMouseRegionsForIMPMainPageBasedOnCharGenStatus() {
  // will destroy button masks for the char gen pages
  FOR_EACH(MOUSE_REGION, r, pIMPMainPageMouseRegions) { MSYS_RemoveRegion(r); }
}

static void IMPMainPageNotSelectableBtnCallback(MOUSE_REGION *pRegion, int32_t iReason) {
  if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    DoLapTopMessageBox(MSG_BOX_IMP_STYLE, pImpPopUpStrings[4], LAPTOP_SCREEN, MSG_BOX_FLAG_OK,
                       BeginMessageBoxCallBack);
  }
}

SGPVObject *LoadIMPPortait() {
  SGPFILENAME filename;
  snprintf(filename, lengthof(filename), FACESDIR "/%d.sti", 200 + iPortraitNumber);
  return AddVideoObjectFromFile(filename);
}

static void LoadCharacterPortraitForMainPage() {
  // this function will load the character's portrait, to be used on portrait
  // button
  if (iCurrentProfileMode >= 4) {
    guiCHARACTERPORTRAITFORMAINPAGE = LoadIMPPortait();
    giIMPMainPageButton[4]->SpecifyIcon(guiCHARACTERPORTRAITFORMAINPAGE, 0, 33, 23, FALSE);
  }
}
