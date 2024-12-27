// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Laptop/IMPAttributeSelection.h"

#include <algorithm>

#include "Directories.h"
#include "Laptop/CharProfile.h"
#include "Laptop/IMPCompileCharacter.h"
#include "Laptop/IMPTextSystem.h"
#include "Laptop/IMPVideoObjects.h"
#include "Laptop/Laptop.h"
#include "Macro.h"
#include "MessageBoxScreen.h"
#include "SGP/ButtonSystem.h"
#include "SGP/Font.h"
#include "SGP/Input.h"
#include "SGP/Video.h"
#include "ScreenIDs.h"
#include "TileEngine/RenderDirty.h"
#include "Utils/Cursors.h"
#include "Utils/FontControl.h"
#include "Utils/Text.h"

// width of the slider bar region
#define BAR_WIDTH 423 - 197

// width of the slider bar itself
#define SLIDER_BAR_WIDTH 37

// the sizeof one skill unit on the sliding bar in pixels
#define BASE_SKILL_PIXEL_UNIT_SIZE (423 - 230)

enum {
  HEALTH_ATTRIBUTE,
  DEXTERITY_ATTRIBUTE,
  AGILITY_ATTRIBUTE,
  STRENGTH_ATTRIBUTE,
  WISDOM_ATTRIBUTE,
  LEADERSHIP_ATTRIBUTE,
  MARKSMANSHIP_SKILL,
  EXPLOSIVE_SKILL,
  MEDICAL_SKILL,
  MECHANICAL_SKILL
};

// the skills as they stand
static int32_t iCurrentStrength = 55;
static int32_t iCurrentAgility = 55;
static int32_t iCurrentDexterity = 55;
static int32_t iCurrentHealth = 55;
static int32_t iCurrentLeaderShip = 55;
static int32_t iCurrentWisdom = 55;
static int32_t iCurrentMarkmanship = 55;
static int32_t iCurrentMechanical = 55;
static int32_t iCurrentMedical = 55;
static int32_t iCurrentExplosives = 55;

// which stat is message about stat at zero about
static int32_t iCurrentStatAtZero = 0;

// total number of bonus points
static int32_t iCurrentBonusPoints = 40;

// diplsay the 0 skill point warning..if skill set to 0, warn character
static BOOLEAN fSkillAtZeroWarning = FALSE;

// is the sliding of the sliding bar active right now?
static BOOLEAN fSlideIsActive = TRUE;

// first time in game
BOOLEAN fFirstIMPAttribTime = TRUE;

// review mode
BOOLEAN fReviewStats = FALSE;

// buttons
static GUIButtonRef giIMPAttributeSelectionButton[1];
static BUTTON_PICS *giIMPAttributeSelectionButtonImage[1];

// slider buttons
static GUIButtonRef giIMPAttributeSelectionSliderButton[20];
static BUTTON_PICS *giIMPAttributeSelectionSliderButtonImage[20];

// mouse regions
static MOUSE_REGION pSliderRegions[10];

// The currently "anchored scroll bar"
static MOUSE_REGION *gpCurrentScrollBox = NULL;
static int32_t giCurrentlySelectedStat = -1;

// has any of the sliding bars moved?...for re-rendering puposes
static BOOLEAN fHasAnySlidingBarMoved = FALSE;

static int32_t uiBarToReRender = -1;

// are we actually coming back to edit, or are we restarting?
BOOLEAN fReturnStatus = FALSE;

void SetAttributes();
void DrawBonusPointsRemaining();
void SetGeneratedCharacterAttributes();

static void CreateAttributeSliderButtons();
static void CreateIMPAttributeSelectionButtons();
static void CreateSlideRegionMouseRegions();

void EnterIMPAttributeSelection() {
  // set attributes and skills
  if (!fReturnStatus && fFirstIMPAttribTime) {
    // re starting
    SetAttributes();

    gpCurrentScrollBox = NULL;
    giCurrentlySelectedStat = -1;

    // does character have PROBLEMS?
    /*
    if (DoesCharacterHaveAnAttitude())   iCurrentBonusPoints += 10;
    if (DoesCharacterHaveAPersoanlity()) iCurrentBonusPoints += 10;
    */
  }
  fReturnStatus = TRUE;
  fFirstIMPAttribTime = FALSE;

  CreateIMPAttributeSelectionButtons();
  CreateAttributeSliderButtons();
  CreateSlideRegionMouseRegions();
  RenderIMPAttributeSelection();
}

void RenderIMPAttributeSelection() {
  RenderProfileBackGround();
  RenderAttributeFrame(51, 87);
  RenderAttributeBoxes();
  RenderAttrib1IndentFrame(51, 30);

  if (!fReviewStats) RenderAttrib2IndentFrame(350, 42);

  // reset rerender flag
  fHasAnySlidingBarMoved = FALSE;

  PrintImpText();
  DrawBonusPointsRemaining();
}

static void DestroyAttributeSliderButtons();
static void DestroyIMPAttributeSelectionButtons();
static void DestroySlideRegionMouseRegions();

void ExitIMPAttributeSelection() {
  DestroyAttributeSliderButtons();
  DestroySlideRegionMouseRegions();
  DestroyIMPAttributeSelectionButtons();

  fReturnStatus = FALSE;
}

static void DecrementStat(int32_t iStatToDecrement);
static int32_t GetCurrentAttributeValue(int32_t iAttribute);
static void IncrementStat(int32_t iStatToIncrement);
static void ProcessAttributes();
static void StatAtZeroBoxCallBack(MessageBoxReturnValue);

void HandleIMPAttributeSelection() {
  // review mode, do not allow changes
  if (fReviewStats) return;

  // set the currently selectd slider bar
  if (gfLeftButtonState && gpCurrentScrollBox != NULL) {
    // if theuser is holding down the mouse cursor to left of the start of the
    // slider bars
    if (gusMouseXPos < SKILL_SLIDE_START_X + LAPTOP_SCREEN_UL_X) {
      DecrementStat(giCurrentlySelectedStat);
    } else if (gusMouseXPos > LAPTOP_SCREEN_UL_X + SKILL_SLIDE_START_X + BAR_WIDTH) {
      // else if the user is holding down the mouse button to the right of the
      // scroll bars
      IncrementStat(giCurrentlySelectedStat);
    } else {
      // get old stat value
      int32_t iCurrentAttributeValue = GetCurrentAttributeValue(giCurrentlySelectedStat);
      int32_t sNewX = gusMouseXPos - (SKILL_SLIDE_START_X + LAPTOP_SCREEN_UL_X);
      int32_t iNewValue = sNewX * 50 / BASE_SKILL_PIXEL_UNIT_SIZE + 35;

      // chenged, move mouse region if change large enough
      if (iCurrentAttributeValue != iNewValue) {
        // update screen
        fHasAnySlidingBarMoved = TRUE;
      }

      // change is enough
      if (iNewValue - iCurrentAttributeValue > 0) {
        // positive, increment stat
        for (int32_t i = iNewValue - iCurrentAttributeValue; i > 0; --i) {
          IncrementStat(giCurrentlySelectedStat);
        }
      } else {
        // negative, decrement stat
        for (int32_t i = iCurrentAttributeValue - iNewValue; i > 0; --i) {
          DecrementStat(giCurrentlySelectedStat);
        }
      }
    }

    RenderIMPAttributeSelection();
  } else {
    gpCurrentScrollBox = NULL;
    giCurrentlySelectedStat = -1;
  }

  // prcoess current state of attributes
  ProcessAttributes();

  // has any bar moved?
  if (fHasAnySlidingBarMoved) {
    // render
    if (uiBarToReRender == -1) {
      RenderIMPAttributeSelection();
    } else {
      RenderAttributeFrameForIndex(51, 87, uiBarToReRender);
      /*
                              // print text for screen
                              PrintImpText();

                              // amt of bonus pts
                              DrawBonusPointsRemaining();

                              RenderAttributeFrame(51, 87);

                              // render attribute boxes
                              RenderAttributeBoxes();

                              PrintImpText();

                              InvalidateRegion(LAPTOP_SCREEN_UL_X + 51,
         LAPTOP_SCREEN_WEB_UL_Y + 87, LAPTOP_SCREEN_UL_X + 51 + 400,
         LAPTOP_SCREEN_WEB_UL_Y + 87 + 220);
      */
      uiBarToReRender = -1;
      MarkButtonsDirty();
    }

    fHasAnySlidingBarMoved = FALSE;
  }
  if (fSkillAtZeroWarning) {
    DoLapTopMessageBox(MSG_BOX_IMP_STYLE, pSkillAtZeroWarning, LAPTOP_SCREEN, MSG_BOX_FLAG_YESNO,
                       StatAtZeroBoxCallBack);
    fSkillAtZeroWarning = FALSE;
  }
}

static void ProcessAttributes() {
  if (iCurrentStrength < 35) iCurrentStrength = 35;
  if (iCurrentDexterity < 35) iCurrentDexterity = 35;
  if (iCurrentAgility < 35) iCurrentAgility = 35;
  if (iCurrentWisdom < 35) iCurrentWisdom = 35;
  if (iCurrentLeaderShip < 35) iCurrentLeaderShip = 35;
  if (iCurrentHealth < 35) iCurrentHealth = 35;

  if (iCurrentStrength > 85) iCurrentStrength = 85;
  if (iCurrentDexterity > 85) iCurrentDexterity = 85;
  if (iCurrentAgility > 85) iCurrentAgility = 85;
  if (iCurrentWisdom > 85) iCurrentWisdom = 85;
  if (iCurrentLeaderShip > 85) iCurrentLeaderShip = 85;
  if (iCurrentHealth > 85) iCurrentHealth = 85;
}

static void IncrementStat(int32_t iStatToIncrement) {
  // review mode, do not allow changes
  if (fReviewStats) return;

  int32_t *val = NULL;
  switch (iStatToIncrement) {
    case STRENGTH_ATTRIBUTE:
      val = &iCurrentStrength;
      break;
    case DEXTERITY_ATTRIBUTE:
      val = &iCurrentDexterity;
      break;
    case AGILITY_ATTRIBUTE:
      val = &iCurrentAgility;
      break;
    case WISDOM_ATTRIBUTE:
      val = &iCurrentWisdom;
      break;
    case LEADERSHIP_ATTRIBUTE:
      val = &iCurrentLeaderShip;
      break;
    case HEALTH_ATTRIBUTE:
      val = &iCurrentHealth;
      break;
    case MARKSMANSHIP_SKILL:
      val = &iCurrentMarkmanship;
      break;
    case MEDICAL_SKILL:
      val = &iCurrentMedical;
      break;
    case MECHANICAL_SKILL:
      val = &iCurrentMechanical;
      break;
    case EXPLOSIVE_SKILL:
      val = &iCurrentExplosives;
      break;
  }

  if (*val == 0) {
    if (iCurrentBonusPoints >= 15) {
      *val = 35;
      iCurrentBonusPoints -= 15;
      fSkillAtZeroWarning = FALSE;
    }
  } else if (*val < 85) {
    if (iCurrentBonusPoints >= 1) {
      ++*val;
      --iCurrentBonusPoints;
    }
  }
}

static void DecrementStat(int32_t iStatToDecrement) {
  // review mode, do not allow changes
  if (fReviewStats) return;

  BOOLEAN may_be_zero = FALSE;
  int32_t *val = NULL;
  switch (iStatToDecrement) {
    case STRENGTH_ATTRIBUTE:
      val = &iCurrentStrength;
      break;
    case DEXTERITY_ATTRIBUTE:
      val = &iCurrentDexterity;
      break;
    case AGILITY_ATTRIBUTE:
      val = &iCurrentAgility;
      break;
    case WISDOM_ATTRIBUTE:
      val = &iCurrentWisdom;
      break;
    case LEADERSHIP_ATTRIBUTE:
      val = &iCurrentLeaderShip;
      break;
    case HEALTH_ATTRIBUTE:
      val = &iCurrentHealth;
      break;
    case MARKSMANSHIP_SKILL:
      val = &iCurrentMarkmanship;
      may_be_zero = TRUE;
      break;
    case MEDICAL_SKILL:
      val = &iCurrentMedical;
      may_be_zero = TRUE;
      break;
    case MECHANICAL_SKILL:
      val = &iCurrentMechanical;
      may_be_zero = TRUE;
      break;
    case EXPLOSIVE_SKILL:
      val = &iCurrentExplosives;
      may_be_zero = TRUE;
      break;
  }

  if (*val > 35) {
    --*val;
    ++iCurrentBonusPoints;
  } else if (may_be_zero && *val == 35) {
    *val = 0;
    iCurrentBonusPoints += 15;
    iCurrentStatAtZero = iStatToDecrement;
    fSkillAtZeroWarning = TRUE;
  }
}

static void BtnIMPAttributeFinishCallback(GUI_BUTTON *btn, int32_t reason);

static void CreateIMPAttributeSelectionButtons() {
  // the finished button
  giIMPAttributeSelectionButtonImage[0] = LoadButtonImage(LAPTOPDIR "/button_2.sti", 0, 1);
  giIMPAttributeSelectionButton[0] = CreateIconAndTextButton(
      giIMPAttributeSelectionButtonImage[0], pImpButtonText[11], FONT12ARIAL, FONT_WHITE,
      DEFAULT_SHADOW, FONT_WHITE, DEFAULT_SHADOW, LAPTOP_SCREEN_UL_X + 136,
      LAPTOP_SCREEN_WEB_UL_Y + 314, MSYS_PRIORITY_HIGH, BtnIMPAttributeFinishCallback);
  giIMPAttributeSelectionButton[0]->SetCursor(CURSOR_WWW);
}

static void DestroyIMPAttributeSelectionButtons() {
  // Destroy the buttons needed for the IMP attrib enter page
  RemoveButton(giIMPAttributeSelectionButton[0]);
  UnloadButtonImage(giIMPAttributeSelectionButtonImage[0]);
}

static void BtnIMPAttributeFinishCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    // are we done diting, or just reviewing the stats?
    if (fReviewStats) {
      iCurrentImpPage = IMP_FINISH;
    } else {
      iCurrentImpPage = IMP_ATTRIBUTE_FINISH;
    }
    fButtonPendingFlag = TRUE;
  }
}

void RenderAttributeBoxes() {
  // this function will render the boxes in the sliding attribute bar, based on
  // position
  SetFontAttributes(FONT10ARIAL, FONT_WHITE, NO_SHADOW);

  // run through and render each slider bar
  for (int32_t i = HEALTH_ATTRIBUTE; i <= MECHANICAL_SKILL; ++i) {
    int32_t val = GetCurrentAttributeValue(i);

    // Compensate for zeroed skills: x pos is at least 0
    int16_t sX = std::max(0, val - 35) * BASE_SKILL_PIXEL_UNIT_SIZE / 50;
    int16_t sY = SKILL_SLIDE_START_Y + SKILL_SLIDE_HEIGHT * i;

    sX += SKILL_SLIDE_START_X;
    RenderSliderBar(sX, sY);

    sX += LAPTOP_SCREEN_UL_X;
    sY += LAPTOP_SCREEN_WEB_UL_Y;
    mprintf(sX + 13, sY + 3, L"%d", val);
  }

  SetFontShadow(DEFAULT_SHADOW);
}

static void BtnIMPAttributeSliderLeftCallback(GUI_BUTTON *btn, int32_t reason);
static void BtnIMPAttributeSliderRightCallback(GUI_BUTTON *btn, int32_t reason);

static void CreateAttributeSliderButtons() {
  // Create the buttons for the attribute slider
  // the finished button
  giIMPAttributeSelectionSliderButtonImage[0] =
      LoadButtonImage(LAPTOPDIR "/attributearrows.sti", 0, 1);
  giIMPAttributeSelectionSliderButtonImage[1] =
      LoadButtonImage(LAPTOPDIR "/attributearrows.sti", 3, 4);

  for (int32_t iCounter = 0; iCounter < 20; iCounter += 2) {
    const int16_t y = LAPTOP_SCREEN_WEB_UL_Y + (99 + iCounter / 2 * 20);
    // left/right buttons - decrement/increment stat
    giIMPAttributeSelectionSliderButton[iCounter] =
        QuickCreateButton(giIMPAttributeSelectionSliderButtonImage[0], LAPTOP_SCREEN_UL_X + 163, y,
                          MSYS_PRIORITY_HIGHEST - 1, BtnIMPAttributeSliderLeftCallback);
    giIMPAttributeSelectionSliderButton[iCounter + 1] =
        QuickCreateButton(giIMPAttributeSelectionSliderButtonImage[1], LAPTOP_SCREEN_UL_X + 419, y,
                          MSYS_PRIORITY_HIGHEST - 1, BtnIMPAttributeSliderRightCallback);

    giIMPAttributeSelectionSliderButton[iCounter]->SetCursor(CURSOR_WWW);
    giIMPAttributeSelectionSliderButton[iCounter + 1]->SetCursor(CURSOR_WWW);
    // set user data
    giIMPAttributeSelectionSliderButton[iCounter]->SetUserData(iCounter / 2);
    giIMPAttributeSelectionSliderButton[iCounter + 1]->SetUserData(iCounter / 2);
  }

  MarkButtonsDirty();
}

static void DestroyAttributeSliderButtons() {
  // Destroy the buttons used for attribute manipulation
  // get rid of image
  UnloadButtonImage(giIMPAttributeSelectionSliderButtonImage[0]);
  UnloadButtonImage(giIMPAttributeSelectionSliderButtonImage[1]);

  for (int32_t iCounter = 0; iCounter < 20; iCounter++) {
    // get rid of button
    RemoveButton(giIMPAttributeSelectionSliderButton[iCounter]);
  }
}

static void BtnIMPAttributeSliderLeftCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN || reason & MSYS_CALLBACK_REASON_LBUTTON_REPEAT) {
    int32_t const iValue = btn->GetUserData();
    DecrementStat(iValue);
    fHasAnySlidingBarMoved = TRUE;
    uiBarToReRender = iValue;
  }
}

static void BtnIMPAttributeSliderRightCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN || reason & MSYS_CALLBACK_REASON_LBUTTON_REPEAT) {
    int32_t const iValue = btn->GetUserData();
    IncrementStat(iValue);
    fHasAnySlidingBarMoved = TRUE;
    uiBarToReRender = iValue;
  }
}

static void SliderRegionButtonCallback(MOUSE_REGION *pRegion, int32_t iReason);

static void CreateSlideRegionMouseRegions() {
  /* Create the mouse regions on the sliding area, that, if the player clicks
   * on, the bar will automatically jump to */
  for (uint32_t i = 0; i != lengthof(pSliderRegions); ++i) {
    MOUSE_REGION &r = pSliderRegions[i];
    uint16_t const x = LAPTOP_SCREEN_UL_X + SKILL_SLIDE_START_X;
    uint16_t const y = LAPTOP_SCREEN_WEB_UL_Y + SKILL_SLIDE_START_Y + i * SKILL_SLIDE_HEIGHT;
    MSYS_DefineRegion(&r, x, y, x + BAR_WIDTH, y + 15, MSYS_PRIORITY_HIGH + 2, CURSOR_WWW,
                      MSYS_NO_CALLBACK, SliderRegionButtonCallback);
    MSYS_SetRegionUserData(&r, 0, i);
  }
}

static void DestroySlideRegionMouseRegions() {  // Destroy the regions used for
                                                // the slider 'jumping'
  FOR_EACH(MOUSE_REGION, i, pSliderRegions) MSYS_RemoveRegion(&*i);
}

static void SliderRegionButtonCallback(MOUSE_REGION *pRegion, int32_t iReason) {
  static int16_t sOldX = -1;
  static int32_t iAttribute = -1;

  // if we already have an anchored slider bar
  if (gpCurrentScrollBox != pRegion && gpCurrentScrollBox != NULL) return;

  if (iReason & MSYS_CALLBACK_REASON_LBUTTON_REPEAT) {
    if (!fSlideIsActive) return;

    // check to see if we have moved
    if (MSYS_GetRegionUserData(pRegion, 0) != iAttribute) {
      // different regions
      iAttribute = MSYS_GetRegionUserData(pRegion, 0);
      sOldX = -1;
      return;
    }

    uiBarToReRender = iAttribute;

    giCurrentlySelectedStat = iAttribute;
    gpCurrentScrollBox = pRegion;

    // get new attribute value x
    int16_t sNewX = pRegion->MouseXPos;

    // sOldX has been reset, set to sNewX
    if (sOldX == -1) {
      sOldX = sNewX;
      return;
    }
    // check against old x
    if (sNewX != sOldX) {
      // get old stat value
      const int32_t iCurrentAttributeValue = GetCurrentAttributeValue(iAttribute);
      sNewX -= SKILL_SLIDE_START_X + LAPTOP_SCREEN_UL_X;
      int32_t iNewValue = (sNewX * 50) / BASE_SKILL_PIXEL_UNIT_SIZE + 35;

      // chenged, move mouse region if change large enough
      if (iCurrentAttributeValue != iNewValue) {
        // update screen
        fHasAnySlidingBarMoved = TRUE;
      }

      // change is enough
      if (iNewValue - iCurrentAttributeValue > 0) {
        // positive, increment stat
        for (int32_t iCounter = iNewValue - iCurrentAttributeValue; iCounter > 0; iCounter--) {
          IncrementStat(iAttribute);
        }
      } else {
        // negative, decrement stat
        for (int32_t iCounter = iCurrentAttributeValue - iNewValue; iCounter > 0; iCounter--) {
          DecrementStat(iAttribute);
        }
      }

      sOldX = sNewX;
    }
  } else if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    if (fSlideIsActive) {
      // reset slide is active flag
      fSlideIsActive = FALSE;
      return;
    }

    // get mouse XY
    const int16_t sX = pRegion->MouseXPos;

    // which region are we in?

    // get attribute
    iAttribute = MSYS_GetRegionUserData(pRegion, 0);
    uiBarToReRender = iAttribute;

    // get value of attribute
    const int32_t iCurrentAttributeValue = GetCurrentAttributeValue(iAttribute);
    // set the new attribute value based on position of mouse click, include
    // screen resolution
    int32_t iNewAttributeValue = (sX - SKILL_SLIDE_START_X) * 50 / BASE_SKILL_PIXEL_UNIT_SIZE;

    // too high, reset to 85
    if (iNewAttributeValue > 85) iNewAttributeValue = 85;

    // get the delta
    const int32_t iAttributeDelta = iCurrentAttributeValue - iNewAttributeValue;

    // check if increment or decrement
    if (iAttributeDelta > 0) {
      // decrement
      for (int32_t iCounter = 0; iCounter < iAttributeDelta; iCounter++) {
        DecrementStat(iAttribute);
      }
    } else {
      // increment attribute
      for (int32_t iCounter = iAttributeDelta; iCounter < 0; iCounter++) {
        if (iCurrentAttributeValue == 0) iCounter = 0;
        IncrementStat(iAttribute);
      }
    }

    // update screen
    fHasAnySlidingBarMoved = TRUE;
  } else if (iReason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
    // get mouse positions
    const int16_t sX = pRegion->MouseXPos;

    // get attribute
    iAttribute = MSYS_GetRegionUserData(pRegion, 0);
    uiBarToReRender = iAttribute;

    // get value of attribute
    const int32_t iCurrentAttributeValue = GetCurrentAttributeValue(iAttribute);

    // get the boxes bounding x
    int16_t sNewX = (iCurrentAttributeValue - 35) * BASE_SKILL_PIXEL_UNIT_SIZE / 50 +
                    SKILL_SLIDE_START_X + LAPTOP_SCREEN_UL_X;

    // the sNewX is below 0, reset to zero
    if (sNewX < 0) sNewX = 0;

    if (sX > sNewX && sX < sNewX + SLIDER_BAR_WIDTH) {
      // we are within the slide bar, set fact we want to drag and draw
      fSlideIsActive = TRUE;
    } else {
      // otherwise want to jump to position
      fSlideIsActive = FALSE;
    }
  }
}

// Get the value of the attribute that was passed
static int32_t GetCurrentAttributeValue(int32_t attribute) {
  int32_t val = 0;
  switch (attribute) {
    case HEALTH_ATTRIBUTE:
      val = iCurrentHealth;
      break;
    case DEXTERITY_ATTRIBUTE:
      val = iCurrentDexterity;
      break;
    case AGILITY_ATTRIBUTE:
      val = iCurrentAgility;
      break;
    case STRENGTH_ATTRIBUTE:
      val = iCurrentStrength;
      break;
    case WISDOM_ATTRIBUTE:
      val = iCurrentWisdom;
      break;
    case LEADERSHIP_ATTRIBUTE:
      val = iCurrentLeaderShip;
      break;
    case MARKSMANSHIP_SKILL:
      val = iCurrentMarkmanship;
      break;
    case EXPLOSIVE_SKILL:
      val = iCurrentExplosives;
      break;
    case MEDICAL_SKILL:
      val = iCurrentMedical;
      break;
    case MECHANICAL_SKILL:
      val = iCurrentMechanical;
      break;
  }
  return val;
}

void SetAttributes() {
  iCurrentStrength = 55;
  iCurrentDexterity = 55;
  iCurrentHealth = 55;
  iCurrentLeaderShip = 55;
  iCurrentWisdom = 55;
  iCurrentAgility = 55;
  iCurrentMarkmanship = 55;
  iCurrentMechanical = 55;
  iCurrentMedical = 55;
  iCurrentExplosives = 55;

  // reset bonus pts
  iCurrentBonusPoints = 40;
}

void DrawBonusPointsRemaining() {
  // draws the amount of points remaining player has

  // just reviewing, don't blit stats
  if (fReviewStats) return;

  SetFontAttributes(FONT12ARIAL, FONT_WHITE);
  mprintf(LAPTOP_SCREEN_UL_X + 425, LAPTOP_SCREEN_WEB_UL_Y + 51, L"%d", iCurrentBonusPoints);
  InvalidateRegion(LAPTOP_SCREEN_UL_X + 425, LAPTOP_SCREEN_WEB_UL_Y + 51, LAPTOP_SCREEN_UL_X + 475,
                   LAPTOP_SCREEN_WEB_UL_Y + 71);
}

void SetGeneratedCharacterAttributes() {
  // Copy over the attributes and skills of the player generated character
  iStrength = iCurrentStrength;
  iDexterity = iCurrentDexterity;
  iHealth = iCurrentHealth;
  iLeadership = iCurrentLeaderShip;
  iWisdom = iCurrentWisdom;
  iAgility = iCurrentAgility;
  iMarksmanship = iCurrentMarkmanship;
  iMechanical = iCurrentMechanical;
  iMedical = iCurrentMedical;
  iExplosives = iCurrentExplosives;
}

static void StatAtZeroBoxCallBack(MessageBoxReturnValue const bExitValue) {
  // yes, so start over, else stay here and do nothing for now
  switch (bExitValue) {
    case MSG_BOX_RETURN_YES:
      MarkButtonsDirty();
      break;

    case MSG_BOX_RETURN_NO:
      IncrementStat(iCurrentStatAtZero);
      fHasAnySlidingBarMoved = TRUE;
      MarkButtonsDirty();
      break;
    default:
      break;
  }
}
