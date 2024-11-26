#include "Laptop/IMPPortraits.h"

#include <stdio.h>
#include <string.h>

#include "Directories.h"
#include "Laptop/CharProfile.h"
#include "Laptop/IMPMainPage.h"
#include "Laptop/IMPTextSystem.h"
#include "Laptop/IMPVideoObjects.h"
#include "Laptop/Laptop.h"
#include "SGP/ButtonSystem.h"
#include "SGP/Font.h"
#include "SGP/Input.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "TileEngine/RenderDirty.h"
#include "Utils/Cursors.h"
#include "Utils/FontControl.h"
#include "Utils/Text.h"

// current and last pages
int32_t iCurrentPortrait = 0;
int32_t iLastPicture = 7;

// buttons needed for the IMP portrait screen
GUIButtonRef giIMPPortraitButton[3];
static BUTTON_PICS *giIMPPortraitButtonImage[3];

// redraw protrait screen
BOOLEAN fReDrawPortraitScreenFlag = FALSE;

// face index
int32_t iPortraitNumber = 0;

static void CreateIMPPortraitButtons();

void EnterIMPPortraits() {
  // create buttons
  CreateIMPPortraitButtons();

  // render background
  RenderIMPPortraits();
}

static void RenderPortrait(int16_t x, int16_t y);

void RenderIMPPortraits() {
  // render background
  RenderProfileBackGround();

  // the Voices frame
  RenderPortraitFrame(191, 167);

  // render the current portrait
  RenderPortrait(200, 176);

  // indent for the text
  RenderAttrib1IndentFrame(128, 65);

  // text
  PrintImpText();
}

static void DestroyIMPPortraitButtons();

void ExitIMPPortraits() {
  // destroy buttons for IMP portrait page
  DestroyIMPPortraitButtons();
}

void HandleIMPPortraits() {
  // do we need to re write screen
  if (fReDrawPortraitScreenFlag) {
    RenderIMPPortraits();

    // reset redraw flag
    fReDrawPortraitScreenFlag = FALSE;
  }
}

static void RenderPortrait(int16_t const x,
                           int16_t const y) {  // Render the portrait of the current picture
  SGPFILENAME filename;
  int32_t const portrait = (fCharacterIsMale ? 200 : 208) + iCurrentPortrait;
  snprintf(filename, lengthof(filename), FACESDIR "/bigfaces/%d.sti", portrait);
  BltVideoObjectOnce(FRAME_BUFFER, filename, 0, LAPTOP_SCREEN_UL_X + x, LAPTOP_SCREEN_WEB_UL_Y + y);
}

static void IncrementPictureIndex() {
  // cycle to next picture

  iCurrentPortrait++;

  // gone too far?
  if (iCurrentPortrait > iLastPicture) {
    iCurrentPortrait = 0;
  }
}

static void DecrementPicture() {
  // cycle to previous picture

  iCurrentPortrait--;

  // gone too far?
  if (iCurrentPortrait < 0) {
    iCurrentPortrait = iLastPicture;
  }
}

static void MakeButton(uint32_t idx, const char *img_file, int32_t off_normal, int32_t on_normal,
                       const wchar_t *text, int16_t x, int16_t y, GUI_CALLBACK click) {
  BUTTON_PICS *const img = LoadButtonImage(img_file, off_normal, on_normal);
  giIMPPortraitButtonImage[idx] = img;
  const int16_t text_col = FONT_WHITE;
  const int16_t shadow_col = DEFAULT_SHADOW;
  GUIButtonRef const btn =
      CreateIconAndTextButton(img, text, FONT12ARIAL, text_col, shadow_col, text_col, shadow_col, x,
                              y, MSYS_PRIORITY_HIGH, click);
  giIMPPortraitButton[idx] = btn;
  btn->SetCursor(CURSOR_WWW);
}

static void BtnIMPPortraitDoneCallback(GUI_BUTTON *btn, int32_t reason);
static void BtnIMPPortraitNextCallback(GUI_BUTTON *btn, int32_t reason);
static void BtnIMPPortraitPreviousCallback(GUI_BUTTON *btn, int32_t reason);

static void CreateIMPPortraitButtons() {
  // will create buttons need for the IMP portrait screen
  const int16_t dx = LAPTOP_SCREEN_UL_X;
  const int16_t dy = LAPTOP_SCREEN_WEB_UL_Y;
  MakeButton(0, LAPTOPDIR "/voicearrows.sti", 1, 3, pImpButtonText[13], dx + 343, dy + 205,
             BtnIMPPortraitNextCallback);  // Next button
  MakeButton(1, LAPTOPDIR "/voicearrows.sti", 0, 2, pImpButtonText[12], dx + 93, dy + 205,
             BtnIMPPortraitPreviousCallback);  // Previous button
  MakeButton(2, LAPTOPDIR "/button_5.sti", 0, 1, pImpButtonText[11], dx + 187, dy + 330,
             BtnIMPPortraitDoneCallback);  // Done button
}

static void DestroyIMPPortraitButtons() {
  // will destroy buttons created for IMP Portrait screen

  // the next button
  RemoveButton(giIMPPortraitButton[0]);
  UnloadButtonImage(giIMPPortraitButtonImage[0]);

  // the previous button
  RemoveButton(giIMPPortraitButton[1]);
  UnloadButtonImage(giIMPPortraitButtonImage[1]);

  // the done button
  RemoveButton(giIMPPortraitButton[2]);
  UnloadButtonImage(giIMPPortraitButtonImage[2]);
}

static void BtnIMPPortraitNextCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    IncrementPictureIndex();
    fReDrawPortraitScreenFlag = TRUE;
  }
}

static void BtnIMPPortraitPreviousCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    DecrementPicture();
    fReDrawPortraitScreenFlag = TRUE;
  }
}

static void BtnIMPPortraitDoneCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    iCurrentImpPage = IMP_MAIN_PAGE;

    // current mode now is voice
    if (iCurrentProfileMode < 4) iCurrentProfileMode = 4;

    // if we are already done, leave
    if (iCurrentProfileMode == 5) iCurrentImpPage = IMP_FINISH;

    // grab picture number
    iPortraitNumber = iCurrentPortrait + (fCharacterIsMale ? 0 : 8);

    fButtonPendingFlag = TRUE;
  }
}
