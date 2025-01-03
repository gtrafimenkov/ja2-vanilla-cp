// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Laptop/IMPHomePage.h"

#include <string.h>

#include "Directories.h"
#include "Laptop/CharProfile.h"
#include "Laptop/IMPVideoObjects.h"
#include "Laptop/Laptop.h"
#include "Laptop/LaptopSave.h"
#include "Local.h"
#include "MessageBoxScreen.h"
#include "SGP/ButtonSystem.h"
#include "SGP/English.h"
#include "SGP/Font.h"
#include "SGP/HImage.h"
#include "SGP/Line.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "ScreenIDs.h"
#include "TileEngine/RenderDirty.h"
#include "Utils/Cursors.h"
#include "Utils/FontControl.h"
#include "Utils/Text.h"
#include "Utils/TextInput.h"
#include "Utils/TimerControl.h"

#include "SDL_keycode.h"

const uint32_t GlowColorsList[] = {FROMRGB(0, 0, 0),   FROMRGB(0, 25, 0),  FROMRGB(0, 50, 0),
                                   FROMRGB(0, 75, 0),  FROMRGB(0, 100, 0), FROMRGB(0, 125, 0),
                                   FROMRGB(0, 150, 0), FROMRGB(0, 175, 0), FROMRGB(0, 200, 0),
                                   FROMRGB(0, 225, 0), FROMRGB(0, 255, 0)};

static void BtnIMPAboutUsCallback(GUI_BUTTON *btn, int32_t reason);

// position defines
#define IMP_PLAYER_ACTIVATION_STRING_X LAPTOP_SCREEN_UL_X + 261
#define IMP_PLAYER_ACTIVATION_STRING_Y LAPTOP_SCREEN_WEB_UL_Y + 336
#define CURSOR_Y IMP_PLAYER_ACTIVATION_STRING_Y - 5

// IMP homepage buttons
GUIButtonRef giIMPHomePageButton[1];
static BUTTON_PICS *giIMPHomePageButtonImage[1];

// the player activation string
wchar_t pPlayerActivationString[32];

// position within player activation string
int32_t iStringPos = 0;
uint16_t uiCursorPosition = IMP_PLAYER_ACTIVATION_STRING_X;

// has a new char been added or deleted?
BOOLEAN fNewCharInActivationString = FALSE;

static void CreateIMPHomePageButtons();

void EnterImpHomePage() {
  // upon entry to Imp home page
  memset(pPlayerActivationString, 0, sizeof(pPlayerActivationString));

  // reset string position
  iStringPos = 0;

  // reset activation  cursor position
  uiCursorPosition = IMP_PLAYER_ACTIVATION_STRING_X;

  // load buttons
  CreateIMPHomePageButtons();

  // render screen once
  RenderImpHomePage();
}

static void DisplayPlayerActivationString();

void RenderImpHomePage() {
  // the background
  RenderProfileBackGround();

  // the IMP symbol
  RenderIMPSymbol(107, 45);

  // the second button image
  RenderButton2Image(134, 314);

  // render the indents

  // activation indents
  RenderActivationIndent(257, 328);

  // the two font page indents
  RenderFrontPageIndent(3, 64);
  RenderFrontPageIndent(396, 64);

  // render the  activation string
  DisplayPlayerActivationString();
}

static void RemoveIMPHomePageButtons();

void ExitImpHomePage() {
  // remove buttons
  RemoveIMPHomePageButtons();
}

static void DisplayActivationStringCursor();
static void GetPlayerKeyBoardInputForIMPHomePage();

void HandleImpHomePage() {
  // handle keyboard input for this screen
  GetPlayerKeyBoardInputForIMPHomePage();

  // has a new char been added to activation string
  if (fNewCharInActivationString) {
    // display string
    DisplayPlayerActivationString();
  }

  // render the cursor
  DisplayActivationStringCursor();
}

static void DisplayPlayerActivationString() {
  // this function will grab the string that the player will enter for
  // activation

  // player gone too far, move back
  if (iStringPos > 64) {
    iStringPos = 64;
  }

  // restore background
  RenderActivationIndent(257, 328);

  SetFontAttributes(FONT14ARIAL, 184);
  MPrint(IMP_PLAYER_ACTIVATION_STRING_X, IMP_PLAYER_ACTIVATION_STRING_Y, pPlayerActivationString);

  fNewCharInActivationString = FALSE;
  fReDrawScreenFlag = TRUE;
}

static void DisplayActivationStringCursor() {
  // this procdure will draw the activation string cursor on the screen at
  // position cursorx cursory
  static uint32_t uiBaseTime = 0;
  uint32_t uiDeltaTime = 0;
  static uint32_t iCurrentState = 0;
  static BOOLEAN fIncrement = TRUE;

  if (uiBaseTime == 0) {
    uiBaseTime = GetJA2Clock();
  }

  // get difference
  uiDeltaTime = GetJA2Clock() - uiBaseTime;

  // if difference is long enough, rotate colors
  if (uiDeltaTime > MIN_GLOW_DELTA) {
    if (iCurrentState == 10) {
      // start rotating downward
      fIncrement = FALSE;
    }
    if (iCurrentState == 0) {
      // rotate colors upward
      fIncrement = TRUE;
    }
    // if increment upward, increment iCurrentState
    if (fIncrement) {
      iCurrentState++;
    } else {
      // else downwards
      iCurrentState--;
    }
    // reset basetime to current clock
    uiBaseTime = GetJA2Clock();
  }

  {
    SGPVSurface::Lock l(FRAME_BUFFER);
    SetClippingRegionAndImageWidth(l.Pitch(), 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    // draw line in current state
    LineDraw(TRUE, uiCursorPosition, CURSOR_Y, uiCursorPosition, CURSOR_Y + CURSOR_HEIGHT,
             Get16BPPColor(GlowColorsList[iCurrentState]), l.Buffer<uint16_t>());
  }

  InvalidateRegion((uint16_t)uiCursorPosition, CURSOR_Y, (uint16_t)uiCursorPosition + 1,
                   CURSOR_Y + CURSOR_HEIGHT + 1);
}

static void HandleTextEvent(const InputAtom *Inp);
static void ProcessPlayerInputActivationString();

static void GetPlayerKeyBoardInputForIMPHomePage() {
  InputAtom InputEvent;
  while (DequeueEvent(&InputEvent)) {
    if (!HandleTextInput(&InputEvent) && (InputEvent.usEvent != KEY_UP)) {
      switch (InputEvent.usParam) {
        case SDLK_RETURN:
          // return hit, check to see if current player activation string is a
          // valid one
          ProcessPlayerInputActivationString();
          fNewCharInActivationString = TRUE;
          break;

        case SDLK_ESCAPE:
          HandleLapTopESCKey();
          break;

        default:
          HandleTextEvent(&InputEvent);
          break;
      }
    }
  }
}

static void HandleTextEvent(const InputAtom *Inp) {
  // this function checks to see if a letter or a backspace was pressed, if so,
  // either put char to screen or delete it

  switch (Inp->usParam) {
    case SDLK_BACKSPACE:
      if (iStringPos >= 0) {
        if (iStringPos > 0) {
          // decrement iStringPosition
          iStringPos -= 1;
        }

        // null out char
        pPlayerActivationString[iStringPos] = 0;

        // move back cursor
        uiCursorPosition =
            StringPixLength(pPlayerActivationString, FONT14ARIAL) + IMP_PLAYER_ACTIVATION_STRING_X;

        // string has been altered, redisplay
        fNewCharInActivationString = TRUE;
      }

      break;

    default: {
      wchar_t Char = Inp->Char;
      if ((Char >= 'A' && Char <= 'Z') || (Char >= 'a' && Char <= 'z') ||
          (Char >= '0' && Char <= '9') || Char == '_' || Char == '.') {
        // if the current string position is at max or great, do nothing
        if (iStringPos >= 6) {
          break;
        } else {
          if (iStringPos < 0) {
            iStringPos = 0;
          }
          // valid char, capture and convert to wchar_t
          pPlayerActivationString[iStringPos] = Char;

          // null out next char position
          pPlayerActivationString[iStringPos + 1] = 0;

          // move cursor position ahead
          uiCursorPosition = StringPixLength(pPlayerActivationString, FONT14ARIAL) +
                             IMP_PLAYER_ACTIVATION_STRING_X;

          // increment string position
          iStringPos += 1;

          // string has been altered, redisplay
          fNewCharInActivationString = TRUE;
        }
      }
      break;
    }
  }
}

static void ProcessPlayerInputActivationString() {
  wchar_t const *msg;
  if (wcscmp(pPlayerActivationString, L"XEP624") != 0 &&
      wcscmp(pPlayerActivationString, L"xep624") != 0) {
    msg = pImpPopUpStrings[0];
  } else if (LaptopSaveInfo.fIMPCompletedFlag) {
    msg = pImpPopUpStrings[6];
  } else {
    iCurrentImpPage = IMP_MAIN_PAGE;
    return;
  }
  DoLapTopMessageBox(MSG_BOX_IMP_STYLE, msg, LAPTOP_SCREEN, MSG_BOX_FLAG_OK, 0);
}

static void CreateIMPHomePageButtons() {
  // this procedure will create the buttons needed for the IMP homepage

  // ths about us button
  giIMPHomePageButtonImage[0] = LoadButtonImage(LAPTOPDIR "/button_1.sti", 0, 1);
  giIMPHomePageButton[0] = CreateIconAndTextButton(
      giIMPHomePageButtonImage[0], pImpButtonText[0], FONT12ARIAL, FONT_WHITE, DEFAULT_SHADOW,
      FONT_WHITE, DEFAULT_SHADOW, LAPTOP_SCREEN_UL_X + 286 - 106, LAPTOP_SCREEN_WEB_UL_Y + 248 - 48,
      MSYS_PRIORITY_HIGH, BtnIMPAboutUsCallback);

  giIMPHomePageButton[0]->SetCursor(CURSOR_WWW);
}

static void RemoveIMPHomePageButtons() {
  // this procedure will destroy the already created buttosn for the IMP
  // homepage

  // the about us button
  RemoveButton(giIMPHomePageButton[0]);
  UnloadButtonImage(giIMPHomePageButtonImage[0]);
}

static void BtnIMPAboutUsCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    iCurrentImpPage = IMP_ABOUT_US;
    fButtonPendingFlag = TRUE;
  }
}
