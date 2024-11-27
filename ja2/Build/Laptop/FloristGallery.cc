#include "Laptop/FloristGallery.h"

#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "Directories.h"
#include "Laptop/Florist.h"
#include "Laptop/Laptop.h"
#include "Macro.h"
#include "SGP/ButtonSystem.h"
#include "SGP/VObject.h"
#include "SGP/Video.h"
#include "Utils/Cursors.h"
#include "Utils/EncryptedFile.h"
#include "Utils/FontControl.h"
#include "Utils/Text.h"
#include "Utils/WordWrap.h"

#define FLOR_GALLERY_TITLE_FONT FONT10ARIAL
#define FLOR_GALLERY_TITLE_COLOR FONT_MCOLOR_WHITE

#define FLOR_GALLERY_FLOWER_TITLE_FONT FONT14ARIAL
#define FLOR_GALLERY_FLOWER_TITLE_COLOR FONT_MCOLOR_WHITE

#define FLOR_GALLERY_FLOWER_PRICE_FONT FONT12ARIAL
#define FLOR_GALLERY_FLOWER_PRICE_COLOR FONT_MCOLOR_WHITE

#define FLOR_GALLERY_FLOWER_DESC_FONT FONT12ARIAL
#define FLOR_GALLERY_FLOWER_DESC_COLOR FONT_MCOLOR_WHITE

#define FLOR_GALLERY_NUMBER_FLORAL_BUTTONS 3
#define FLOR_GALLERY_NUMBER_FLORAL_IMAGES 10

#define FLOR_GALLERY_FLOWER_DESC_TEXT_FONT FONT12ARIAL
#define FLOR_GALLERY_FLOWER_DESC_TEXT_COLOR FONT_MCOLOR_WHITE

#define FLOR_GALLERY_BACK_BUTTON_X (LAPTOP_SCREEN_UL_X + 8)
#define FLOR_GALLERY_NEXT_BUTTON_X (LAPTOP_SCREEN_UL_X + 420)
#define FLOR_GALLERY_BUTTON_Y (LAPTOP_SCREEN_WEB_UL_Y + 12)

#define FLOR_GALLERY_FLOWER_BUTTON_X LAPTOP_SCREEN_UL_X + 7
#define FLOR_GALLERY_FLOWER_BUTTON_Y LAPTOP_SCREEN_WEB_UL_Y + 74

// #define FLOR_GALLERY_FLOWER_BUTTON_OFFSET_X		250

#define FLOR_GALLERY_FLOWER_BUTTON_OFFSET_Y 112

#define FLOR_GALLERY_TITLE_TEXT_X LAPTOP_SCREEN_UL_X + 0
#define FLOR_GALLERY_TITLE_TEXT_Y LAPTOP_SCREEN_WEB_UL_Y + 48
#define FLOR_GALLERY_TITLE_TEXT_WIDTH LAPTOP_SCREEN_LR_X - LAPTOP_SCREEN_UL_X

#define FLOR_GALLERY_FLOWER_TITLE_X FLOR_GALLERY_FLOWER_BUTTON_X + 88

#define FLOR_GALLERY_DESC_WIDTH 390

#define FLOR_GALLERY_FLOWER_TITLE_OFFSET_Y 9
#define FLOR_GALLERY_FLOWER_PRICE_OFFSET_Y FLOR_GALLERY_FLOWER_TITLE_OFFSET_Y + 17
#define FLOR_GALLERY_FLOWER_DESC_OFFSET_Y FLOR_GALLERY_FLOWER_PRICE_OFFSET_Y + 15

static SGPVObject *guiFlowerImages[3];

uint32_t guiCurrentlySelectedFlower = 0;

uint8_t gubCurFlowerIndex = 0;
uint8_t gubCurNumberOfFlowers = 0;
uint8_t gubPrevNumberOfFlowers = 0;
BOOLEAN gfRedrawFloristGallery = FALSE;

BOOLEAN FloristGallerySubPagesVisitedFlag[4];

// Floral buttons
static BUTTON_PICS *guiGalleryButtonImage;
static GUIButtonRef guiGalleryButton[FLOR_GALLERY_NUMBER_FLORAL_BUTTONS];

// Next Previous buttons
static BUTTON_PICS *guiFloralGalleryButtonImage;
static void BtnFloralGalleryNextButtonCallback(GUI_BUTTON *btn, int32_t reason);
static void BtnFloralGalleryBackButtonCallback(GUI_BUTTON *btn, int32_t reason);
GUIButtonRef guiFloralGalleryButton[2];

void EnterInitFloristGallery() { memset(&FloristGallerySubPagesVisitedFlag, 0, 4); }

static GUIButtonRef MakeButton(const wchar_t *text, int16_t x, GUI_CALLBACK click) {
  const int16_t shadow_col = FLORIST_BUTTON_TEXT_SHADOW_COLOR;
  GUIButtonRef const btn = CreateIconAndTextButton(
      guiFloralGalleryButtonImage, text, FLORIST_BUTTON_TEXT_FONT, FLORIST_BUTTON_TEXT_UP_COLOR,
      shadow_col, FLORIST_BUTTON_TEXT_DOWN_COLOR, shadow_col, x, FLOR_GALLERY_BUTTON_Y,
      MSYS_PRIORITY_HIGH, click);
  btn->SetCursor(CURSOR_WWW);
  return btn;
}

static void InitFlowerButtons();

BOOLEAN EnterFloristGallery() {
  InitFloristDefaults();

  // the next previous buttons
  guiFloralGalleryButtonImage = LoadButtonImage(LAPTOPDIR "/floristbuttons.sti", 0, 1);
  guiFloralGalleryButton[0] =
      MakeButton(sFloristGalleryText[FLORIST_GALLERY_PREV], FLOR_GALLERY_BACK_BUTTON_X,
                 BtnFloralGalleryBackButtonCallback);
  guiFloralGalleryButton[1] =
      MakeButton(sFloristGalleryText[FLORIST_GALLERY_NEXT], FLOR_GALLERY_NEXT_BUTTON_X,
                 BtnFloralGalleryNextButtonCallback);

  RenderFloristGallery();

  InitFlowerButtons();

  return (TRUE);
}

static void DeleteFlowerButtons();

void ExitFloristGallery() {
  uint16_t i;

  RemoveFloristDefaults();

  for (i = 0; i < 2; i++) RemoveButton(guiFloralGalleryButton[i]);

  UnloadButtonImage(guiFloralGalleryButtonImage);

  DeleteFlowerButtons();
}

void HandleFloristGallery() {
  if (gfRedrawFloristGallery) {
    gfRedrawFloristGallery = FALSE;

    //
    DeleteFlowerButtons();
    InitFlowerButtons();

    fPausedReDrawScreenFlag = TRUE;
  }
}

static BOOLEAN DisplayFloralDescriptions();

void RenderFloristGallery() {
  DisplayFloristDefaults();

  DrawTextToScreen(sFloristGalleryText[FLORIST_GALLERY_CLICK_TO_ORDER], FLOR_GALLERY_TITLE_TEXT_X,
                   FLOR_GALLERY_TITLE_TEXT_Y, FLOR_GALLERY_TITLE_TEXT_WIDTH,
                   FLOR_GALLERY_TITLE_FONT, FLOR_GALLERY_TITLE_COLOR, FONT_MCOLOR_BLACK,
                   CENTER_JUSTIFIED);
  DrawTextToScreen(sFloristGalleryText[FLORIST_GALLERY_ADDIFTIONAL_FEE], FLOR_GALLERY_TITLE_TEXT_X,
                   FLOR_GALLERY_TITLE_TEXT_Y + 11, FLOR_GALLERY_TITLE_TEXT_WIDTH,
                   FLOR_GALLERY_TITLE_FONT, FLOR_GALLERY_TITLE_COLOR, FONT_MCOLOR_BLACK,
                   CENTER_JUSTIFIED);

  DisplayFloralDescriptions();

  MarkButtonsDirty();
  RenderWWWProgramTitleBar();
  InvalidateRegion(LAPTOP_SCREEN_UL_X, LAPTOP_SCREEN_WEB_UL_Y, LAPTOP_SCREEN_LR_X,
                   LAPTOP_SCREEN_WEB_LR_Y);
}

static void ChangingFloristGallerySubPage(uint8_t ubSubPageNumber);

static void BtnFloralGalleryNextButtonCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    if (gubCurFlowerIndex + 3 <= FLOR_GALLERY_NUMBER_FLORAL_IMAGES) gubCurFlowerIndex += 3;

    ChangingFloristGallerySubPage(gubCurFlowerIndex);
    gfRedrawFloristGallery = TRUE;
  }
}

static void BtnFloralGalleryBackButtonCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    if (gubCurFlowerIndex != 0) {
      if (gubCurFlowerIndex >= 3)
        gubCurFlowerIndex -= 3;
      else
        gubCurFlowerIndex = 0;
      ChangingFloristGallerySubPage(gubCurFlowerIndex);
    } else {
      guiCurrentLaptopMode = LAPTOP_MODE_FLORIST;
    }
    gfRedrawFloristGallery = TRUE;
  }
}

static void BtnGalleryFlowerButtonCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    guiCurrentlySelectedFlower = btn->GetUserData();
    guiCurrentLaptopMode = LAPTOP_MODE_FLORIST_ORDERFORM;
    gfShowBookmarks = FALSE;
  }
}

static void InitFlowerButtons() {
  uint16_t i, j, count;
  uint16_t usPosY;
  char sTemp[40];

  if ((FLOR_GALLERY_NUMBER_FLORAL_IMAGES - gubCurFlowerIndex) >= 3)
    gubCurNumberOfFlowers = 3;
  else
    gubCurNumberOfFlowers = FLOR_GALLERY_NUMBER_FLORAL_IMAGES - gubCurFlowerIndex;

  gubPrevNumberOfFlowers = gubCurNumberOfFlowers;

  // the 10 pictures of the flowers
  count = gubCurFlowerIndex;
  for (i = 0; i < gubCurNumberOfFlowers; i++) {
    // load the handbullet graphic and add it
    sprintf(sTemp, LAPTOPDIR "/flower_%d.sti", count);
    guiFlowerImages[i] = AddVideoObjectFromFile(sTemp);
    count++;
  }

  // the buttons with the flower pictures on them
  usPosY = FLOR_GALLERY_FLOWER_BUTTON_Y;
  //	usPosX = FLOR_GALLERY_FLOWER_BUTTON_X;
  count = gubCurFlowerIndex;
  guiGalleryButtonImage = LoadButtonImage(LAPTOPDIR "/gallerybuttons.sti", 0, 1);
  for (j = 0; j < gubCurNumberOfFlowers; j++) {
    guiGalleryButton[j] =
        QuickCreateButton(guiGalleryButtonImage, FLOR_GALLERY_FLOWER_BUTTON_X, usPosY,
                          MSYS_PRIORITY_HIGH, BtnGalleryFlowerButtonCallback);
    guiGalleryButton[j]->SetCursor(CURSOR_WWW);
    guiGalleryButton[j]->SetUserData(count);
    guiGalleryButton[j]->SpecifyIcon(guiFlowerImages[j], 0, 5, 5, FALSE);

    usPosY += FLOR_GALLERY_FLOWER_BUTTON_OFFSET_Y;
    count++;
  }

  // if its the first page, display the 'back' text  in place of the 'prev' text
  // on the top left button
  wchar_t const *const text = gubCurFlowerIndex == 0 ? sFloristGalleryText[FLORIST_GALLERY_HOME]
                                                     : sFloristGalleryText[FLORIST_GALLERY_PREV];
  guiFloralGalleryButton[0]->SpecifyText(text);

  // if it is the last page disable the next button
  EnableButton(guiFloralGalleryButton[1],
               gubCurFlowerIndex != FLOR_GALLERY_NUMBER_FLORAL_IMAGES - 1);
}

static void DeleteFlowerButtons() {
  uint16_t i;

  for (i = 0; i < gubPrevNumberOfFlowers; i++) {
    DeleteVideoObject(guiFlowerImages[i]);
  }

  UnloadButtonImage(guiGalleryButtonImage);

  for (i = 0; i < gubPrevNumberOfFlowers; i++) {
    RemoveButton(guiGalleryButton[i]);
  }
}

static BOOLEAN DisplayFloralDescriptions() {
  uint32_t uiStartLoc = 0, i;
  uint16_t usPosY, usPrice;

  if ((FLOR_GALLERY_NUMBER_FLORAL_IMAGES - gubCurFlowerIndex) >= 3)
    gubCurNumberOfFlowers = 3;
  else
    gubCurNumberOfFlowers = FLOR_GALLERY_NUMBER_FLORAL_IMAGES - gubCurFlowerIndex;

  usPosY = FLOR_GALLERY_FLOWER_BUTTON_Y;
  for (i = 0; i < gubCurNumberOfFlowers; i++) {
    {
      // Display Flower title
      wchar_t sTemp[FLOR_GALLERY_TEXT_TITLE_SIZE];
      uiStartLoc = FLOR_GALLERY_TEXT_TOTAL_SIZE * (i + gubCurFlowerIndex);
      LoadEncryptedDataFromFile(FLOR_GALLERY_TEXT_FILE, sTemp, uiStartLoc,
                                FLOR_GALLERY_TEXT_TITLE_SIZE);
      DrawTextToScreen(sTemp, FLOR_GALLERY_FLOWER_TITLE_X,
                       usPosY + FLOR_GALLERY_FLOWER_TITLE_OFFSET_Y, 0,
                       FLOR_GALLERY_FLOWER_TITLE_FONT, FLOR_GALLERY_FLOWER_TITLE_COLOR,
                       FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
    }

    {
      // Display Flower Price
      wchar_t sTemp[FLOR_GALLERY_TEXT_PRICE_SIZE];
      uiStartLoc += FLOR_GALLERY_TEXT_TITLE_SIZE;
      LoadEncryptedDataFromFile(FLOR_GALLERY_TEXT_FILE, sTemp, uiStartLoc,
                                FLOR_GALLERY_TEXT_PRICE_SIZE);
      swscanf(sTemp, L"%hu", &usPrice);
      swprintf(sTemp, lengthof(sTemp), L"$%d.00 %ls", usPrice,
               pMessageStrings[MSG_USDOLLAR_ABBREVIATION]);
      DrawTextToScreen(sTemp, FLOR_GALLERY_FLOWER_TITLE_X,
                       usPosY + FLOR_GALLERY_FLOWER_PRICE_OFFSET_Y, 0,
                       FLOR_GALLERY_FLOWER_PRICE_FONT, FLOR_GALLERY_FLOWER_PRICE_COLOR,
                       FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
    }

    {
      // Display Flower Desc
      wchar_t sTemp[FLOR_GALLERY_TEXT_DESC_SIZE];
      uiStartLoc += FLOR_GALLERY_TEXT_PRICE_SIZE;
      LoadEncryptedDataFromFile(FLOR_GALLERY_TEXT_FILE, sTemp, uiStartLoc,
                                FLOR_GALLERY_TEXT_DESC_SIZE);
      DisplayWrappedString(FLOR_GALLERY_FLOWER_TITLE_X, usPosY + FLOR_GALLERY_FLOWER_DESC_OFFSET_Y,
                           FLOR_GALLERY_DESC_WIDTH, 2, FLOR_GALLERY_FLOWER_DESC_FONT,
                           FLOR_GALLERY_FLOWER_DESC_COLOR, sTemp, FONT_MCOLOR_BLACK,
                           LEFT_JUSTIFIED);
    }

    usPosY += FLOR_GALLERY_FLOWER_BUTTON_OFFSET_Y;
  }

  return (TRUE);
}

static void ChangingFloristGallerySubPage(uint8_t ubSubPageNumber) {
  fLoadPendingFlag = TRUE;

  // there are 3 flowers per page
  if (ubSubPageNumber == FLOR_GALLERY_NUMBER_FLORAL_IMAGES)
    ubSubPageNumber = 4;
  else
    ubSubPageNumber = ubSubPageNumber / 3;

  if (!FloristGallerySubPagesVisitedFlag[ubSubPageNumber]) {
    fConnectingToSubPage = TRUE;
    fFastLoadFlag = FALSE;

    FloristGallerySubPagesVisitedFlag[ubSubPageNumber] = TRUE;
  } else {
    fConnectingToSubPage = TRUE;
    fFastLoadFlag = TRUE;
  }
}
