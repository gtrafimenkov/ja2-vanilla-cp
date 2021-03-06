#include "Laptop/IMPAttributeFinish.h"

#include "Directories.h"
#include "Laptop/CharProfile.h"
#include "Laptop/IMPAttributeSelection.h"
#include "Laptop/IMPMainPage.h"
#include "Laptop/IMPVideoObjects.h"
#include "Laptop/Laptop.h"
#include "SGP/ButtonSystem.h"
#include "SGP/Font.h"
#include "SGP/Input.h"
#include "TileEngine/RenderDirty.h"
#include "Utils/Cursors.h"
#include "Utils/FontControl.h"
#include "Utils/Text.h"

// buttons
static BUTTON_PICS *giIMPAttributeFinishButtonImage[2];
GUIButtonRef giIMPAttributeFinishButton[2];

// function definitions
extern void SetGeneratedCharacterAttributes(void);

static void BtnIMPAttributeFinishYesCallback(GUI_BUTTON *btn, INT32 reason);
static void BtnIMPAttributeFinishNoCallback(GUI_BUTTON *btn, INT32 reason);

static void CreateAttributeFinishButtons(void);

void EnterIMPAttributeFinish(void) {
  // create the needed buttons
  CreateAttributeFinishButtons();

  // render screen
  RenderIMPAttributeFinish();
}

void RenderIMPAttributeFinish(void) {
  // render background
  RenderProfileBackGround();

  // indent for text
  RenderBeginIndent(110, 93);
}

static void DestroyAttributeFinishButtons(void);

void ExitIMPAttributeFinish(void) {
  // destroy the buttons for this screen
  DestroyAttributeFinishButtons();
}

void HandleIMPAttributeFinish(void) {}

static void MakeButton(UINT idx, const wchar_t *text, INT16 y, GUI_CALLBACK click) {
  BUTTON_PICS *const img = LoadButtonImage(LAPTOPDIR "/button_2.sti", 0, 1);
  giIMPAttributeFinishButtonImage[idx] = img;
  const INT16 text_col = FONT_WHITE;
  const INT16 shadow_col = DEFAULT_SHADOW;
  GUIButtonRef const btn =
      CreateIconAndTextButton(img, text, FONT12ARIAL, text_col, shadow_col, text_col, shadow_col,
                              LAPTOP_SCREEN_UL_X + 130, y, MSYS_PRIORITY_HIGH, click);
  giIMPAttributeFinishButton[idx] = btn;
  btn->SetCursor(CURSOR_WWW);
}

static void CreateAttributeFinishButtons(void) {
  // this procedure will create the buttons needed for the attribute finish
  // screen
  const INT16 dy = LAPTOP_SCREEN_WEB_UL_Y;
  MakeButton(0, pImpButtonText[20], dy + 180,
             BtnIMPAttributeFinishYesCallback);  // Yes button
  MakeButton(1, pImpButtonText[21], dy + 264,
             BtnIMPAttributeFinishNoCallback);  // No button
}

static void DestroyAttributeFinishButtons(void) {
  // this procedure will destroy the buttons for the attribute finish screen

  // the yes  button
  RemoveButton(giIMPAttributeFinishButton[0]);
  UnloadButtonImage(giIMPAttributeFinishButtonImage[0]);

  // the no  button
  RemoveButton(giIMPAttributeFinishButton[1]);
  UnloadButtonImage(giIMPAttributeFinishButtonImage[1]);
}

static void BtnIMPAttributeFinishYesCallback(GUI_BUTTON *btn, INT32 reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    // gone far enough
    iCurrentImpPage = IMP_MAIN_PAGE;
    if (iCurrentProfileMode < 3) {
      iCurrentProfileMode = 3;
    }
    // if we are already done, leave
    if (iCurrentProfileMode == 5) {
      iCurrentImpPage = IMP_FINISH;
    }

    // SET ATTRIBUTES NOW
    SetGeneratedCharacterAttributes();
    fButtonPendingFlag = TRUE;
  }
}

static void BtnIMPAttributeFinishNoCallback(GUI_BUTTON *btn, INT32 reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    // if no, return to attribute
    iCurrentImpPage = IMP_ATTRIBUTE_PAGE;
    fReturnStatus = TRUE;
    fButtonPendingFlag = TRUE;
  }
}
