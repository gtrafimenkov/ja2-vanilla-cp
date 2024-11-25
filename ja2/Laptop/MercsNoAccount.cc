// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Laptop/MercsNoAccount.h"

#include "Directories.h"
#include "Laptop/Laptop.h"
#include "Laptop/LaptopSave.h"
#include "Laptop/Mercs.h"
#include "Laptop/SpeckQuotes.h"
#include "SGP/ButtonSystem.h"
#include "SGP/Font.h"
#include "SGP/Random.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "Utils/Cursors.h"
#include "Utils/FontControl.h"
#include "Utils/Text.h"
#include "Utils/WordWrap.h"

#define MERC_NA_TEXT_FONT FONT12ARIAL
#define MERC_NA_TEXT_COLOR FONT_MCOLOR_WHITE

#define MERC_NO_ACCOUNT_IMAGE_X LAPTOP_SCREEN_UL_X + 23
#define MERC_NO_ACCOUNT_IMAGE_Y LAPTOP_SCREEN_UL_Y + 52

#define MERC_OPEN_BUTTON_X 130
#define MERC_CANCEL_BUTTON_X 490
#define MERC_BUTTON_Y 380

#define MERC_NA_SENTENCE_X MERC_NO_ACCOUNT_IMAGE_X + 10
#define MERC_NA_SENTENCE_Y MERC_NO_ACCOUNT_IMAGE_Y + 75
#define MERC_NA_SENTENCE_WIDTH 460 - 20

static SGPVObject *guiNoAccountImage;

// The Open Account Box button
static void BtnOpenAccountBoxButtonCallback(GUI_BUTTON *btn, int32_t reason);
static BUTTON_PICS *guiOpenAccountBoxButtonImage;
GUIButtonRef guiOpenAccountBoxButton;

// The Cancel Account Box button
static void BtnCancelBoxButtonCallback(GUI_BUTTON *btn, int32_t reason);
GUIButtonRef guiCancelBoxButton;

static GUIButtonRef MakeButton(const wchar_t *text, int16_t x, GUI_CALLBACK click) {
  const int16_t shadow_col = DEFAULT_SHADOW;
  GUIButtonRef const btn = CreateIconAndTextButton(
      guiOpenAccountBoxButtonImage, text, FONT12ARIAL, MERC_BUTTON_UP_COLOR, shadow_col,
      MERC_BUTTON_DOWN_COLOR, shadow_col, x, MERC_BUTTON_Y, MSYS_PRIORITY_HIGH, click);
  btn->SetCursor(CURSOR_LAPTOP_SCREEN);
  return btn;
}

void EnterMercsNoAccount() {
  InitMercBackGround();

  // load the Account box graphic and add it
  guiNoAccountImage = AddVideoObjectFromFile(LAPTOPDIR "/noaccountbox.sti");

  // Open Accouint button
  guiOpenAccountBoxButtonImage = LoadButtonImage(LAPTOPDIR "/bigbuttons.sti", 0, 1);
  guiOpenAccountBoxButton = MakeButton(MercNoAccountText[MERC_NO_ACC_OPEN_ACCOUNT],
                                       MERC_OPEN_BUTTON_X, BtnOpenAccountBoxButtonCallback);
  guiCancelBoxButton = MakeButton(MercNoAccountText[MERC_NO_ACC_CANCEL], MERC_CANCEL_BUTTON_X,
                                  BtnCancelBoxButtonCallback);

  RenderMercsNoAccount();
}

void ExitMercsNoAccount() {
  DeleteVideoObject(guiNoAccountImage);

  UnloadButtonImage(guiOpenAccountBoxButtonImage);
  RemoveButton(guiOpenAccountBoxButton);
  RemoveButton(guiCancelBoxButton);

  RemoveMercBackGround();
}

void RenderMercsNoAccount() {
  DrawMecBackGround();

  BltVideoObject(FRAME_BUFFER, guiNoAccountImage, 0, MERC_NO_ACCOUNT_IMAGE_X,
                 MERC_NO_ACCOUNT_IMAGE_Y);

  // Display the sentence
  DisplayWrappedString(MERC_NA_SENTENCE_X, MERC_NA_SENTENCE_Y, MERC_NA_SENTENCE_WIDTH, 2,
                       MERC_NA_TEXT_FONT, MERC_NA_TEXT_COLOR,
                       MercNoAccountText[MERC_NO_ACC_NO_ACCOUNT_OPEN_ONE], FONT_MCOLOR_BLACK,
                       CENTER_JUSTIFIED);

  MarkButtonsDirty();
  RenderWWWProgramTitleBar();
  InvalidateRegion(LAPTOP_SCREEN_UL_X, LAPTOP_SCREEN_UL_Y, LAPTOP_SCREEN_LR_X,
                   LAPTOP_SCREEN_WEB_LR_Y);
}

static void BtnOpenAccountBoxButtonCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    // open an account
    LaptopSaveInfo.gubPlayersMercAccountStatus = MERC_ACCOUNT_VALID;

    // Get an account number
    LaptopSaveInfo.guiPlayersMercAccountNumber = Random(99999);

    gusMercVideoSpeckSpeech = SPECK_QUOTE_THANK_PLAYER_FOR_OPENING_ACCOUNT;

    guiCurrentLaptopMode = LAPTOP_MODE_MERC;
    gubArrivedFromMercSubSite = MERC_CAME_FROM_ACCOUNTS_PAGE;
  }
}

static void BtnCancelBoxButtonCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    guiCurrentLaptopMode = LAPTOP_MODE_MERC;
    gubArrivedFromMercSubSite = MERC_CAME_FROM_ACCOUNTS_PAGE;
  }
}
