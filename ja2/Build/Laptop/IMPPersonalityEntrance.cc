#include "Laptop/IMPPersonalityEntrance.h"

#include "Directories.h"
#include "Laptop/CharProfile.h"
#include "Laptop/IMPVideoObjects.h"
#include "Laptop/Laptop.h"
#include "SGP/ButtonSystem.h"
#include "SGP/Font.h"
#include "TileEngine/RenderDirty.h"
#include "Utils/Cursors.h"
#include "Utils/FontControl.h"
#include "Utils/Text.h"

// IMP personality entrance buttons
static BUTTON_PICS *giIMPPersonalityEntranceButtonImage[1];
static GUIButtonRef giIMPPersonalityEntranceButton[1];

static void BtnIMPPersonalityEntranceDoneCallback(GUI_BUTTON *btn, INT32 reason);

static void CreateIMPPersonalityEntranceButtons();

void EnterIMPPersonalityEntrance() {
  // create buttons needed
  CreateIMPPersonalityEntranceButtons();
}

void RenderIMPPersonalityEntrance() {
  // the background
  RenderProfileBackGround();

  // the IMP symbol
  // RenderIMPSymbol( 112, 30 );

  // indent
  RenderAvgMercIndentFrame(90, 40);
}

static void DestroyIMPPersonalityEntranceButtons();

void ExitIMPPersonalityEntrance() {
  // destroy buttons needed
  DestroyIMPPersonalityEntranceButtons();
}

void HandleIMPPersonalityEntrance() {}

static void CreateIMPPersonalityEntranceButtons() {
  // this function will create the buttons needed for the IMP personality Page

  // ths begin button
  giIMPPersonalityEntranceButtonImage[0] = LoadButtonImage(LAPTOPDIR "/button_2.sti", 0, 1);
  giIMPPersonalityEntranceButton[0] = CreateIconAndTextButton(
      giIMPPersonalityEntranceButtonImage[0], pImpButtonText[1], FONT12ARIAL, FONT_WHITE,
      DEFAULT_SHADOW, FONT_WHITE, DEFAULT_SHADOW, LAPTOP_SCREEN_UL_X + 136,
      LAPTOP_SCREEN_WEB_UL_Y + 314, MSYS_PRIORITY_HIGH, BtnIMPPersonalityEntranceDoneCallback);

  giIMPPersonalityEntranceButton[0]->SetCursor(CURSOR_WWW);
}

static void DestroyIMPPersonalityEntranceButtons() {
  // this function will destroy the buttons needed for the IMP personality page

  // the begin button
  RemoveButton(giIMPPersonalityEntranceButton[0]);
  UnloadButtonImage(giIMPPersonalityEntranceButtonImage[0]);
}

static void BtnIMPPersonalityEntranceDoneCallback(GUI_BUTTON *btn, INT32 reason) {
  // btn callback for IMP Begin Screen done button

  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    // done with begin screen, next screen
    iCurrentImpPage = IMP_PERSONALITY_QUIZ;
    fButtonPendingFlag = TRUE;
  }
}
