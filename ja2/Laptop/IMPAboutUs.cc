// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Laptop/IMPAboutUs.h"

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

// IMP AboutUs buttons
static GUIButtonRef giIMPAboutUsButton[1];
static BUTTON_PICS *giIMPAboutUsButtonImage[1];

static void BtnIMPBackCallback(GUI_BUTTON *btn, int32_t reason);

static void CreateIMPAboutUsButtons();

void EnterIMPAboutUs() {
  // create buttons
  CreateIMPAboutUsButtons();

  // entry into IMP about us page
  RenderIMPAboutUs();
}

static void DeleteIMPAboutUsButtons();

void ExitIMPAboutUs() {
  // exit from IMP About us page

  // delete Buttons
  DeleteIMPAboutUsButtons();
}

void RenderIMPAboutUs() {
  // rneders the IMP about us page

  // the background
  RenderProfileBackGround();

  // the IMP symbol
  RenderIMPSymbol(106, 1);

  // about us indent
  RenderAboutUsIndentFrame(8, 130);
  // about us indent
  RenderAboutUsIndentFrame(258, 130);
}

void HandleIMPAboutUs() {
  // handles the IMP about us page
}

static void CreateIMPAboutUsButtons() {
  // this function will create the buttons needed for th IMP about us page
  // the back button button
  giIMPAboutUsButtonImage[0] = LoadButtonImage(LAPTOPDIR "/button_3.sti", 0, 1);
  giIMPAboutUsButton[0] = CreateIconAndTextButton(
      giIMPAboutUsButtonImage[0], pImpButtonText[6], FONT12ARIAL, FONT_WHITE, DEFAULT_SHADOW,
      FONT_WHITE, DEFAULT_SHADOW, LAPTOP_SCREEN_UL_X + 216, LAPTOP_SCREEN_WEB_UL_Y + 360,
      MSYS_PRIORITY_HIGH, BtnIMPBackCallback);

  giIMPAboutUsButton[0]->SetCursor(CURSOR_WWW);
}

static void DeleteIMPAboutUsButtons() {
  // this function destroys the buttons needed for the IMP about Us Page

  // the about back button
  RemoveButton(giIMPAboutUsButton[0]);
  UnloadButtonImage(giIMPAboutUsButtonImage[0]);
}

static void BtnIMPBackCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    iCurrentImpPage = IMP_HOME_PAGE;
  }
}
