// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Laptop/AIMPolicies.h"

#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "Directories.h"
#include "Laptop/AIM.h"
#include "Laptop/Laptop.h"
#include "Macro.h"
#include "SGP/ButtonSystem.h"
#include "SGP/Font.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "Utils/Cursors.h"
#include "Utils/EncryptedFile.h"
#include "Utils/FontControl.h"
#include "Utils/Text.h"
#include "Utils/WordWrap.h"

#define NUM_AIM_POLICY_PAGES 11
#define NUM_AIM_POLICY_TOC_BUTTONS 9
#define AIMPOLICYFILE BINARYDATADIR "/aimpol.edt"
#define AIM_POLICY_LINE_SIZE 80 * 5

#define AIM_POLICY_TITLE_FONT FONT14ARIAL
#define AIM_POLICY_TITLE_COLOR AIM_GREEN
#define AIM_POLICY_TEXT_FONT FONT10ARIAL
#define AIM_POLICY_TEXT_COLOR FONT_MCOLOR_WHITE
#define AIM_POLICY_TOC_FONT FONT12ARIAL
#define AIM_POLICY_TOC_COLOR FONT_MCOLOR_WHITE
#define AIM_POLICY_SUBTITLE_FONT FONT12ARIAL
#define AIM_POLICY_SUBTITLE_COLOR FONT_MCOLOR_WHITE
#define AIM_POLICY_AGREE_TOC_COLOR_ON FONT_MCOLOR_WHITE
#define AIM_POLICY_AGREE_TOC_COLOR_OFF FONT_MCOLOR_DKWHITE

#define AIM_POLICY_MENU_X LAPTOP_SCREEN_UL_X + 40
#define AIM_POLICY_MENU_Y 390 + LAPTOP_SCREEN_WEB_DELTA_Y
#define AIM_POLICY_MENU_BUTTON_AMOUNT 4
#define AIM_POLICY_GAP_X 40 + BOTTOM_BUTTON_START_WIDTH

#define AIM_POLICY_TITLE_X IMAGE_OFFSET_X + 149
#define AIM_POLICY_TITLE_Y AIM_SYMBOL_Y + AIM_SYMBOL_SIZE_Y + 11
#define AIM_POLICY_TITLE_WIDTH AIM_SYMBOL_WIDTH

#define AIM_POLICY_TITLE_STATEMENT_WIDTH 300
#define AIM_POLICY_TITLE_STATEMENT_X \
  IMAGE_OFFSET_X + (500 - AIM_POLICY_TITLE_STATEMENT_WIDTH) / 2 + 5  // 80
#define AIM_POLICY_TITLE_STATEMENT_Y AIM_SYMBOL_Y + AIM_SYMBOL_SIZE_Y + 75

#define AIM_POLICY_SUBTITLE_NUMBER AIM_POLICY_TITLE_STATEMENT_X - 75
#define AIM_POLICY_SUBTITLE_X AIM_POLICY_SUBTITLE_NUMBER + 20
#define AIM_POLICY_SUBTITLE_Y 115 + LAPTOP_SCREEN_WEB_DELTA_Y

#define AIM_POLICY_PARAGRAPH_NUMBER AIM_POLICY_SUBTITLE_X - 12
#define AIM_POLICY_PARAGRAPH_X AIM_POLICY_PARAGRAPH_NUMBER + 23
#define AIM_POLICY_PARAGRAPH_Y AIM_POLICY_SUBTITLE_Y + 20
#define AIM_POLICY_PARAGRAPH_WIDTH 380
#define AIM_POLICY_PARAGRAPH_GAP 6
#define AIM_POLICY_SUBPARAGRAPH_NUMBER AIM_POLICY_PARAGRAPH_X
#define AIM_POLICY_SUBPARAGRAPH_X AIM_POLICY_SUBPARAGRAPH_NUMBER + 25

#define AIM_POLICY_TOC_X 259
#define AIM_POLICY_TOC_Y AIM_POLICY_SUBTITLE_Y
#define AIM_POLICY_TOC_GAP_Y 25
#define AIM_POLICY_TOC_TEXT_OFFSET_X 5
#define AIM_POLICY_TOC_TEXT_OFFSET_Y 5

#define AIM_POLICY_AGREEMENT_X IMAGE_OFFSET_X + 150
#define AIM_POLICY_AGREEMENT_Y 350 + LAPTOP_SCREEN_WEB_DELTA_Y

#define AIM_POLICY_TOC_PAGE 1
#define AIM_POLICY_LAST_PAGE 10

// These enums represent which paragraph they are located in the AIMPOLICYFILE
// file
enum AimPolicyTextLocations {
  AIM_STATEMENT_OF_POLICY,
  AIM_STATEMENT_OF_POLICY_1,
  AIM_STATEMENT_OF_POLICY_2,

  DEFINITIONS,
  DEFINITIONS_1,
  DEFINITIONS_2,
  DEFINITIONS_3,
  DEFINITIONS_4,

  LENGTH_OF_ENGAGEMENT,
  LENGTH_OF_ENGAGEMENT_1,
  LENGTH_OF_ENGAGEMENT_1_1,
  LENGTH_OF_ENGAGEMENT_1_2,
  LENGTH_OF_ENGAGEMENT_1_3,
  LENGTH_OF_ENGAGEMENT_2,

  LOCATION_0F_ENGAGEMENT,
  LOCATION_0F_ENGAGEMENT_1,
  LOCATION_0F_ENGAGEMENT_2,
  LOCATION_0F_ENGAGEMENT_2_1,
  LOCATION_0F_ENGAGEMENT_2_2,
  LOCATION_0F_ENGAGEMENT_2_3,
  LOCATION_0F_ENGAGEMENT_2_4,
  LOCATION_0F_ENGAGEMENT_3,

  CONTRACT_EXTENSIONS,
  CONTRACT_EXTENSIONS_1,
  CONTRACT_EXTENSIONS_2,
  CONTRACT_EXTENSIONS_3,

  TERMS_OF_PAYMENT,
  TERMS_OF_PAYMENT_1,

  TERMS_OF_ENGAGEMENT,
  TERMS_OF_ENGAGEMENT_1,
  TERMS_OF_ENGAGEMENT_2A,
  TERMS_OF_ENGAGEMENT_2B,

  ENGAGEMENT_TERMINATION,
  ENGAGEMENT_TERMINATION_1,
  ENGAGEMENT_TERMINATION_1_1,
  ENGAGEMENT_TERMINATION_1_2,
  ENGAGEMENT_TERMINATION_1_3,

  EQUIPMENT_AND_INVENTORY,
  EQUIPMENT_AND_INVENTORY_1,
  EQUIPMENT_AND_INVENTORY_2,

  POLICY_MEDICAL,
  POLICY_MEDICAL_1,
  POLICY_MEDICAL_2,
  POLICY_MEDICAL_3A,
  POLICY_MEDICAL_3B,
  POLICY_MEDICAL_4,

  NUM_AIM_POLICY_LOCATIONS
};

// Toc menu mouse regions
static MOUSE_REGION gSelectedPolicyTocMenuRegion[NUM_AIM_POLICY_TOC_BUTTONS];

// Agree/Disagree menu Buttons regions
static void BtnPoliciesAgreeButtonCallback(GUI_BUTTON *btn, int32_t reason);
static BUTTON_PICS *guiPoliciesButtonImage;
static GUIButtonRef guiPoliciesAgreeButton[2];

// Bottom Menu Buttons
static BUTTON_PICS *guiPoliciesMenuButtonImage;
static GUIButtonRef guiPoliciesMenuButton[AIM_POLICY_MENU_BUTTON_AMOUNT];

static uint8_t gubCurPageNum;
static BOOLEAN gfInPolicyToc = FALSE;
static BOOLEAN gfInAgreementPage = FALSE;
static BOOLEAN gfAimPolicyMenuBarLoaded = FALSE;
static SGPVObject *guiContentButton;
static BOOLEAN gfExitingAimPolicy;
static BOOLEAN AimPoliciesSubPagesVisitedFlag[NUM_AIM_POLICY_PAGES];

void EnterInitAimPolicies() { memset(&AimPoliciesSubPagesVisitedFlag, 0, NUM_AIM_POLICY_PAGES); }

static void InitAimPolicyMenuBar();

void EnterAimPolicies() {
  InitAimDefaults();

  gubCurPageNum = (uint8_t)giCurrentSubPage;

  gfAimPolicyMenuBarLoaded = FALSE;
  gfExitingAimPolicy = FALSE;

  if (gubCurPageNum != 0) InitAimPolicyMenuBar();

  gfInPolicyToc = FALSE;

  // load the Content Buttons graphic and add it
  guiContentButton = AddVideoObjectFromFile(LAPTOPDIR "/contentbutton.sti");

  RenderAimPolicies();
}

static void ExitAgreementButton();
static void ExitAimPolicyMenuBar();
static void ExitAimPolicyTocMenu();

void ExitAimPolicies() {
  gfExitingAimPolicy = TRUE;

  DeleteVideoObject(guiContentButton);

  if (gfAimPolicyMenuBarLoaded) ExitAimPolicyMenuBar();

  if (gfInPolicyToc) ExitAimPolicyTocMenu();

  if (gfInAgreementPage) ExitAgreementButton();
  RemoveAimDefaults();

  giCurrentSubPage = gubCurPageNum;
}

void HandleAimPolicies() {
  if (!gfAimPolicyMenuBarLoaded && gubCurPageNum != 0) {
    InitAimPolicyMenuBar();
    //		RenderAimPolicies();
    fPausedReDrawScreenFlag = TRUE;
  }
}

static void DisableAimPolicyButton();
static uint16_t DisplayAimPolicyParagraph(uint16_t usPosY, uint8_t ubPageNum, float fNumber);
static void DisplayAimPolicyStatement();
static uint16_t DisplayAimPolicySubParagraph(uint16_t usPosY, uint8_t ubPageNum, float fNumber);
static void DisplayAimPolicyTitle(uint16_t usPosY, uint8_t ubPageNum);
static void DisplayAimPolicyTitleText();
static void DrawAimPolicyMenu();
static void InitAgreementRegion();
static void InitAimPolicyTocMenu();

void RenderAimPolicies() {
  uint16_t usNumPixles;

  DrawAimDefaults();

  DisplayAimPolicyTitleText();

  if (gfInAgreementPage) ExitAgreementButton();

  switch (gubCurPageNum) {
    case 0:
      DisplayAimPolicyStatement();
      InitAgreementRegion();
      break;

    case 1:
      InitAimPolicyTocMenu();
      InitAimPolicyMenuBar();
      DisableAimPolicyButton();
      DrawAimPolicyMenu();
      break;

    case 2:
      // Display the Definitions title
      DisplayAimPolicyTitle(AIM_POLICY_SUBTITLE_Y, DEFINITIONS);
      usNumPixles = AIM_POLICY_PARAGRAPH_Y;
      usNumPixles += DisplayAimPolicyParagraph(usNumPixles, DEFINITIONS_1, (float)1.1) +
                     AIM_POLICY_PARAGRAPH_GAP;
      usNumPixles += DisplayAimPolicyParagraph(usNumPixles, DEFINITIONS_2, (float)1.2) +
                     AIM_POLICY_PARAGRAPH_GAP;
      usNumPixles += DisplayAimPolicyParagraph(usNumPixles, DEFINITIONS_3, (float)1.3) +
                     AIM_POLICY_PARAGRAPH_GAP;
      usNumPixles += DisplayAimPolicyParagraph(usNumPixles, DEFINITIONS_4, (float)1.4);
      break;

    case 3:
      DisplayAimPolicyTitle(AIM_POLICY_SUBTITLE_Y, LENGTH_OF_ENGAGEMENT);
      usNumPixles = AIM_POLICY_PARAGRAPH_Y;
      usNumPixles += DisplayAimPolicyParagraph(usNumPixles, LENGTH_OF_ENGAGEMENT_1, (float)2.1) +
                     AIM_POLICY_PARAGRAPH_GAP;
      usNumPixles +=
          DisplayAimPolicySubParagraph(usNumPixles, LENGTH_OF_ENGAGEMENT_1_1, (float)2.11) +
          AIM_POLICY_PARAGRAPH_GAP;
      usNumPixles +=
          DisplayAimPolicySubParagraph(usNumPixles, LENGTH_OF_ENGAGEMENT_1_2, (float)2.12) +
          AIM_POLICY_PARAGRAPH_GAP;
      usNumPixles +=
          DisplayAimPolicySubParagraph(usNumPixles, LENGTH_OF_ENGAGEMENT_1_3, (float)2.13) +
          AIM_POLICY_PARAGRAPH_GAP;
      usNumPixles += DisplayAimPolicyParagraph(usNumPixles, LENGTH_OF_ENGAGEMENT_2, (float)2.2) +
                     AIM_POLICY_PARAGRAPH_GAP;
      break;

    case 4:
      DisplayAimPolicyTitle(AIM_POLICY_SUBTITLE_Y, LOCATION_0F_ENGAGEMENT);
      usNumPixles = AIM_POLICY_PARAGRAPH_Y;
      usNumPixles += DisplayAimPolicyParagraph(usNumPixles, LOCATION_0F_ENGAGEMENT_1, (float)3.1) +
                     AIM_POLICY_PARAGRAPH_GAP;
      usNumPixles += DisplayAimPolicyParagraph(usNumPixles, LOCATION_0F_ENGAGEMENT_2, (float)3.2) +
                     AIM_POLICY_PARAGRAPH_GAP;

      usNumPixles +=
          DisplayAimPolicySubParagraph(usNumPixles, LOCATION_0F_ENGAGEMENT_2_1, (float)3.21) +
          AIM_POLICY_PARAGRAPH_GAP;
      usNumPixles +=
          DisplayAimPolicySubParagraph(usNumPixles, LOCATION_0F_ENGAGEMENT_2_2, (float)3.22) +
          AIM_POLICY_PARAGRAPH_GAP;
      usNumPixles +=
          DisplayAimPolicySubParagraph(usNumPixles, LOCATION_0F_ENGAGEMENT_2_3, (float)3.23) +
          AIM_POLICY_PARAGRAPH_GAP;
      //			usNumPixles += DisplayAimPolicySubParagraph(usNumPixles,
      // LOCATION_0F_ENGAGEMENT_2_4, (float)3.24) + AIM_POLICY_PARAGRAPH_GAP;

      usNumPixles +=
          DisplayAimPolicyParagraph(usNumPixles, LOCATION_0F_ENGAGEMENT_2_4, (float)3.3) +
          AIM_POLICY_PARAGRAPH_GAP;

      usNumPixles += DisplayAimPolicyParagraph(usNumPixles, LOCATION_0F_ENGAGEMENT_3, (float)3.4) +
                     AIM_POLICY_PARAGRAPH_GAP;
      break;

    case 5:
      DisplayAimPolicyTitle(AIM_POLICY_SUBTITLE_Y, CONTRACT_EXTENSIONS);
      usNumPixles = AIM_POLICY_PARAGRAPH_Y;
      usNumPixles += DisplayAimPolicyParagraph(usNumPixles, CONTRACT_EXTENSIONS_1, (float)4.1) +
                     AIM_POLICY_PARAGRAPH_GAP;
      usNumPixles += DisplayAimPolicyParagraph(usNumPixles, CONTRACT_EXTENSIONS_2, (float)4.2) +
                     AIM_POLICY_PARAGRAPH_GAP;
      usNumPixles += DisplayAimPolicyParagraph(usNumPixles, CONTRACT_EXTENSIONS_3, (float)4.3) +
                     AIM_POLICY_PARAGRAPH_GAP;
      break;

    case 6:
      DisplayAimPolicyTitle(AIM_POLICY_SUBTITLE_Y, TERMS_OF_PAYMENT);
      usNumPixles = AIM_POLICY_PARAGRAPH_Y;
      usNumPixles += DisplayAimPolicyParagraph(usNumPixles, TERMS_OF_PAYMENT_1, (float)5.1) +
                     AIM_POLICY_PARAGRAPH_GAP;
      break;

    case 7:
      DisplayAimPolicyTitle(AIM_POLICY_SUBTITLE_Y, TERMS_OF_ENGAGEMENT);
      usNumPixles = AIM_POLICY_PARAGRAPH_Y;
      usNumPixles += DisplayAimPolicyParagraph(usNumPixles, TERMS_OF_ENGAGEMENT_1, (float)6.1) +
                     AIM_POLICY_PARAGRAPH_GAP;
      usNumPixles += DisplayAimPolicyParagraph(usNumPixles, TERMS_OF_ENGAGEMENT_2A, (float)6.2) +
                     AIM_POLICY_PARAGRAPH_GAP;
      usNumPixles += DisplayAimPolicyParagraph(usNumPixles, TERMS_OF_ENGAGEMENT_2B, (float)0.0) +
                     AIM_POLICY_PARAGRAPH_GAP;
      break;

    case 8:
      DisplayAimPolicyTitle(AIM_POLICY_SUBTITLE_Y, ENGAGEMENT_TERMINATION);
      usNumPixles = AIM_POLICY_PARAGRAPH_Y;
      usNumPixles += DisplayAimPolicyParagraph(usNumPixles, ENGAGEMENT_TERMINATION_1, (float)7.1) +
                     AIM_POLICY_PARAGRAPH_GAP;

      usNumPixles +=
          DisplayAimPolicySubParagraph(usNumPixles, ENGAGEMENT_TERMINATION_1_1, (float)7.11) +
          AIM_POLICY_PARAGRAPH_GAP;
      usNumPixles +=
          DisplayAimPolicySubParagraph(usNumPixles, ENGAGEMENT_TERMINATION_1_2, (float)7.12) +
          AIM_POLICY_PARAGRAPH_GAP;
      usNumPixles +=
          DisplayAimPolicySubParagraph(usNumPixles, ENGAGEMENT_TERMINATION_1_3, (float)7.13) +
          AIM_POLICY_PARAGRAPH_GAP;
      break;

    case 9:
      DisplayAimPolicyTitle(AIM_POLICY_SUBTITLE_Y, EQUIPMENT_AND_INVENTORY);
      usNumPixles = AIM_POLICY_PARAGRAPH_Y;
      usNumPixles += DisplayAimPolicyParagraph(usNumPixles, EQUIPMENT_AND_INVENTORY_1, (float)8.1) +
                     AIM_POLICY_PARAGRAPH_GAP;
      usNumPixles += DisplayAimPolicyParagraph(usNumPixles, EQUIPMENT_AND_INVENTORY_2, (float)8.2) +
                     AIM_POLICY_PARAGRAPH_GAP;
      break;

    case 10:
      DisableAimPolicyButton();

      DisplayAimPolicyTitle(AIM_POLICY_SUBTITLE_Y, POLICY_MEDICAL);
      usNumPixles = AIM_POLICY_PARAGRAPH_Y;
      usNumPixles += DisplayAimPolicyParagraph(usNumPixles, POLICY_MEDICAL_1, (float)9.1) +
                     AIM_POLICY_PARAGRAPH_GAP;
      usNumPixles += DisplayAimPolicyParagraph(usNumPixles, POLICY_MEDICAL_2, (float)9.2) +
                     AIM_POLICY_PARAGRAPH_GAP;
      usNumPixles += DisplayAimPolicyParagraph(usNumPixles, POLICY_MEDICAL_3A, (float)9.3) +
                     AIM_POLICY_PARAGRAPH_GAP;
      usNumPixles += DisplayAimPolicyParagraph(usNumPixles, POLICY_MEDICAL_3B, (float)0.0) +
                     AIM_POLICY_PARAGRAPH_GAP;
      usNumPixles += DisplayAimPolicyParagraph(usNumPixles, POLICY_MEDICAL_4, (float)9.4) +
                     AIM_POLICY_PARAGRAPH_GAP;
      break;
  }

  MarkButtonsDirty();

  RenderWWWProgramTitleBar();

  InvalidateRegion(LAPTOP_SCREEN_UL_X, LAPTOP_SCREEN_WEB_UL_Y, LAPTOP_SCREEN_LR_X,
                   LAPTOP_SCREEN_WEB_LR_Y);
}

static void BtnPoliciesMenuButtonCallback(GUI_BUTTON *btn, int32_t reason);

static void InitAimPolicyMenuBar() {
  if (gfAimPolicyMenuBarLoaded) return;
  gfAimPolicyMenuBarLoaded = TRUE;

  // Load graphic for buttons
  BUTTON_PICS *const gfx = LoadButtonImage(LAPTOPDIR "/bottombuttons2.sti", 0, 1);
  guiPoliciesMenuButtonImage = gfx;

  uint16_t x = AIM_POLICY_MENU_X;
  uint16_t const y = AIM_POLICY_MENU_Y;
  const StrPointer *text = AimPolicyText;
  int32_t idx = 0;
  FOR_EACHX(GUIButtonRef, i, guiPoliciesMenuButton, x += AIM_POLICY_GAP_X) {
    GUIButtonRef const b = CreateIconAndTextButton(
        gfx, *text++, FONT10ARIAL, AIM_BUTTON_ON_COLOR, DEFAULT_SHADOW, AIM_BUTTON_OFF_COLOR,
        DEFAULT_SHADOW, x, y, MSYS_PRIORITY_HIGH, BtnPoliciesMenuButtonCallback);
    b->SetCursor(CURSOR_WWW);
    b->SetUserData(idx++);
    *i = b;
  }
}

static void ExitAimPolicyMenuBar() {
  if (!gfAimPolicyMenuBarLoaded) return;
  gfAimPolicyMenuBarLoaded = FALSE;
  FOR_EACH(GUIButtonRef, i, guiPoliciesMenuButton) RemoveButton(*i);
  UnloadButtonImage(guiPoliciesMenuButtonImage);
}

static void LoadAIMPolicyText(wchar_t *Text, uint32_t Offset) {
  LoadEncryptedDataFromFile(AIMPOLICYFILE, Text, Offset * AIM_POLICY_LINE_SIZE,
                            AIM_POLICY_LINE_SIZE);
}

static void DrawAimPolicyMenu() {
  uint16_t i, usPosY;
  uint8_t ubLocInFile[] = {
      DEFINITIONS,      LENGTH_OF_ENGAGEMENT, LOCATION_0F_ENGAGEMENT, CONTRACT_EXTENSIONS,
      TERMS_OF_PAYMENT, TERMS_OF_ENGAGEMENT,  ENGAGEMENT_TERMINATION, EQUIPMENT_AND_INVENTORY,
      POLICY_MEDICAL};

  usPosY = AIM_POLICY_TOC_Y;
  for (i = 0; i < NUM_AIM_POLICY_TOC_BUTTONS; i++) {
    BltVideoObject(FRAME_BUFFER, guiContentButton, 0, AIM_POLICY_TOC_X, usPosY);

    wchar_t sText[AIM_POLICY_LINE_SIZE];
    LoadAIMPolicyText(sText, ubLocInFile[i]);
    DrawTextToScreen(sText, AIM_POLICY_TOC_X + AIM_POLICY_TOC_TEXT_OFFSET_X,
                     usPosY + AIM_POLICY_TOC_TEXT_OFFSET_Y, AIM_CONTENTBUTTON_WIDTH,
                     AIM_POLICY_TOC_FONT, AIM_POLICY_TOC_COLOR, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);

    usPosY += AIM_POLICY_TOC_GAP_Y;
  }
  gfInPolicyToc = TRUE;
}

static void SelectPolicyTocMenuRegionCallBack(MOUSE_REGION *pRegion, int32_t iReason);

static void InitAimPolicyTocMenu() {
  if (gfInPolicyToc) return;
  gfInPolicyToc = TRUE;

  uint16_t const x = AIM_POLICY_TOC_X;
  uint16_t y = AIM_POLICY_TOC_Y;
  int32_t page = 2;
  FOR_EACHX(MOUSE_REGION, i, gSelectedPolicyTocMenuRegion,
            y += AIM_POLICY_TOC_GAP_Y) {  // Mouse region for the toc buttons
    MOUSE_REGION &r = *i;
    MSYS_DefineRegion(&r, x, y, x + AIM_CONTENTBUTTON_WIDTH, y + AIM_CONTENTBUTTON_HEIGHT,
                      MSYS_PRIORITY_HIGH, CURSOR_WWW, MSYS_NO_CALLBACK,
                      SelectPolicyTocMenuRegionCallBack);
    MSYS_SetRegionUserData(&r, 0, page++);
  }
}

static void ExitAimPolicyTocMenu() {
  gfInPolicyToc = FALSE;
  FOR_EACH(MOUSE_REGION, i, gSelectedPolicyTocMenuRegion)
  MSYS_RemoveRegion(&*i);
}

static void ChangingAimPoliciesSubPage(uint8_t ubSubPageNumber);
static void ResetAimPolicyButtons();

static void SelectPolicyTocMenuRegionCallBack(MOUSE_REGION *pRegion, int32_t iReason) {
  if (gfInPolicyToc) {
    if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
      gubCurPageNum = (uint8_t)MSYS_GetRegionUserData(pRegion, 0);

      ChangingAimPoliciesSubPage(gubCurPageNum);

      ExitAimPolicyTocMenu();
      ResetAimPolicyButtons();
      DisableAimPolicyButton();
    }
  }
}

static void DisplayAimPolicyTitleText() {
  wchar_t sText[AIM_POLICY_LINE_SIZE];
  LoadAIMPolicyText(sText, AIM_STATEMENT_OF_POLICY);

  uint16_t y = (gubCurPageNum == 0 ? AIM_POLICY_TITLE_STATEMENT_Y - 25 : AIM_POLICY_TITLE_Y);
  DrawTextToScreen(sText, AIM_POLICY_TITLE_X, y, AIM_POLICY_TITLE_WIDTH, AIM_POLICY_TITLE_FONT,
                   AIM_POLICY_TITLE_COLOR, FONT_MCOLOR_BLACK, CENTER_JUSTIFIED);
}

static void DisplayAimPolicyStatement() {
  wchar_t sText[AIM_POLICY_LINE_SIZE];
  uint16_t usNumPixels;

  // load and display the statment of policies
  LoadAIMPolicyText(sText, AIM_STATEMENT_OF_POLICY_1);
  usNumPixels = DisplayWrappedString(
      AIM_POLICY_TITLE_STATEMENT_X, AIM_POLICY_TITLE_STATEMENT_Y, AIM_POLICY_TITLE_STATEMENT_WIDTH,
      2, AIM_POLICY_TEXT_FONT, AIM_POLICY_TEXT_COLOR, sText, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);

  // load and display the statment of policies
  LoadAIMPolicyText(sText, AIM_STATEMENT_OF_POLICY_2);
  DisplayWrappedString(AIM_POLICY_TITLE_STATEMENT_X,
                       AIM_POLICY_TITLE_STATEMENT_Y + usNumPixels + 15,
                       AIM_POLICY_TITLE_STATEMENT_WIDTH, 2, AIM_POLICY_TEXT_FONT,
                       AIM_POLICY_TEXT_COLOR, sText, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
}

static void InitAgreementRegion() {
  // Load graphic for buttons
  BUTTON_PICS *const gfx = LoadButtonImage(LAPTOPDIR "/bottombuttons2.sti", 0, 1);
  guiPoliciesButtonImage = gfx;

  uint16_t x = AIM_POLICY_AGREEMENT_X;
  uint16_t const y = AIM_POLICY_AGREEMENT_Y;
  const StrPointer *text = AimPolicyText + AIM_POLICIES_DISAGREE;
  int32_t idx = 0;
  FOR_EACHX(GUIButtonRef, i, guiPoliciesAgreeButton, x += 125) {
    GUIButtonRef const b =
        CreateIconAndTextButton(gfx, *text++, AIM_POLICY_TOC_FONT, AIM_POLICY_AGREE_TOC_COLOR_ON,
                                DEFAULT_SHADOW, AIM_POLICY_AGREE_TOC_COLOR_OFF, DEFAULT_SHADOW, x,
                                y, MSYS_PRIORITY_HIGH, BtnPoliciesAgreeButtonCallback);
    b->SetCursor(CURSOR_WWW);
    b->SetUserData(idx++);
    *i = b;
  }
  gfInAgreementPage = TRUE;
}

static void ExitAgreementButton() {
  FOR_EACH(GUIButtonRef, i, guiPoliciesAgreeButton) RemoveButton(*i);
  UnloadButtonImage(guiPoliciesButtonImage);
  gfInAgreementPage = FALSE;
}

static void DisplayAimPolicyTitle(uint16_t usPosY, uint8_t ubPageNum) {
  wchar_t sText[AIM_POLICY_LINE_SIZE];
  LoadAIMPolicyText(sText, ubPageNum);
  DrawTextToScreen(sText, AIM_POLICY_SUBTITLE_NUMBER, usPosY, 0, AIM_POLICY_SUBTITLE_FONT,
                   AIM_POLICY_SUBTITLE_COLOR, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
}

static uint16_t DisplayAimPolicyParagraph(uint16_t usPosY, uint8_t ubPageNum, float fNumber) {
  wchar_t sTemp[20];
  uint16_t usNumPixels;

  wchar_t sText[AIM_POLICY_LINE_SIZE];
  LoadAIMPolicyText(sText, ubPageNum);

  if (fNumber != 0.0) {
    // Display the section number
    swprintf(sTemp, lengthof(sTemp), L"%2.1f", fNumber);
    DrawTextToScreen(sTemp, AIM_POLICY_PARAGRAPH_NUMBER, usPosY, 0, AIM_POLICY_TEXT_FONT,
                     AIM_POLICY_TEXT_COLOR, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);
  }

  // Display the text beside the section number
  usNumPixels = DisplayWrappedString(AIM_POLICY_PARAGRAPH_X, usPosY, AIM_POLICY_PARAGRAPH_WIDTH, 2,
                                     AIM_POLICY_TEXT_FONT, AIM_POLICY_TEXT_COLOR, sText,
                                     FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);

  return (usNumPixels);
}

static uint16_t DisplayAimPolicySubParagraph(uint16_t usPosY, uint8_t ubPageNum, float fNumber) {
  wchar_t sTemp[20];
  uint16_t usNumPixels;

  wchar_t sText[AIM_POLICY_LINE_SIZE];
  LoadAIMPolicyText(sText, ubPageNum);

  // Display the section number
  swprintf(sTemp, lengthof(sTemp), L"%2.2f", fNumber);
  DrawTextToScreen(sTemp, AIM_POLICY_SUBPARAGRAPH_NUMBER, usPosY, 0, AIM_POLICY_TEXT_FONT,
                   AIM_POLICY_TEXT_COLOR, FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);

  // Display the text beside the section number
  usNumPixels = DisplayWrappedString(AIM_POLICY_SUBPARAGRAPH_X, usPosY, AIM_POLICY_PARAGRAPH_WIDTH,
                                     2, AIM_POLICY_TEXT_FONT, AIM_POLICY_TEXT_COLOR, sText,
                                     FONT_MCOLOR_BLACK, LEFT_JUSTIFIED);

  return (usNumPixels);
}

static void BtnPoliciesAgreeButtonCallback(GUI_BUTTON *btn, int32_t reason) {
  static BOOLEAN fOnPage = TRUE;

  if (fOnPage) {
    if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
      uint8_t const ubRetValue = btn->GetUserData();

      fOnPage = FALSE;
      if (ubRetValue == 1) {
        // Agree
        gubCurPageNum++;
        ChangingAimPoliciesSubPage(gubCurPageNum);
      } else {
        // Disagree
        guiCurrentLaptopMode = LAPTOP_MODE_AIM;
      }
      fOnPage = TRUE;
    }
  }
}

static void BtnPoliciesMenuButtonCallback(GUI_BUTTON *btn, int32_t reason) {
  static BOOLEAN fOnPage = TRUE;

  if (fOnPage) {
    if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
      uint8_t const ubRetValue = btn->GetUserData();
      switch (ubRetValue) {
        case 0:  // If previous Page
          if (gubCurPageNum > 1) {
            gubCurPageNum--;
            ChangingAimPoliciesSubPage(gubCurPageNum);
          }
          break;

        case 1:  // Home Page
          guiCurrentLaptopMode = LAPTOP_MODE_AIM;
          break;

        case 2:  // Company policies index
          if (gubCurPageNum != 1) {
            gubCurPageNum = 1;
            ChangingAimPoliciesSubPage(gubCurPageNum);
          }
          break;

        case 3:  // Next Page
          if (gubCurPageNum < NUM_AIM_POLICY_PAGES - 1) {
            gubCurPageNum++;
            ChangingAimPoliciesSubPage(gubCurPageNum);

            fOnPage = FALSE;
            if (gfInPolicyToc) {
              ExitAimPolicyTocMenu();
            }
            fOnPage = TRUE;
          }
          break;
      }
      ResetAimPolicyButtons();
      DisableAimPolicyButton();
      fOnPage = TRUE;
    }
  }
}

static void ResetAimPolicyButtons() {
  FOR_EACH(GUIButtonRef, i, guiPoliciesMenuButton) { (*i)->uiFlags &= ~BUTTON_CLICKED_ON; }
}

static void DisableAimPolicyButton() {
  if (gfExitingAimPolicy || !gfAimPolicyMenuBarLoaded) return;

  if (gubCurPageNum == AIM_POLICY_TOC_PAGE) {
    guiPoliciesMenuButton[0]->uiFlags |= BUTTON_CLICKED_ON;
    guiPoliciesMenuButton[2]->uiFlags |= BUTTON_CLICKED_ON;
  } else if (gubCurPageNum == AIM_POLICY_LAST_PAGE) {
    guiPoliciesMenuButton[3]->uiFlags |= BUTTON_CLICKED_ON;
  }
}

static void ChangingAimPoliciesSubPage(uint8_t ubSubPageNumber) {
  fLoadPendingFlag = TRUE;

  if (!AimPoliciesSubPagesVisitedFlag[ubSubPageNumber]) {
    fConnectingToSubPage = TRUE;
    fFastLoadFlag = FALSE;

    AimPoliciesSubPagesVisitedFlag[ubSubPageNumber] = TRUE;
  } else {
    fConnectingToSubPage = TRUE;
    fFastLoadFlag = TRUE;
  }
}
