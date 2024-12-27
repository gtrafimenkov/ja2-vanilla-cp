// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Laptop/Personnel.h"

#include <stdexcept>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "Directories.h"
#include "Laptop/EMail.h"
#include "Laptop/Finances.h"
#include "Laptop/Laptop.h"
#include "Laptop/LaptopSave.h"
#include "Macro.h"
#include "MercPortrait.h"
#include "SGP/ButtonSystem.h"
#include "SGP/Debug.h"
#include "SGP/Font.h"
#include "SGP/HImage.h"
#include "SGP/Input.h"
#include "SGP/Line.h"
#include "SGP/Random.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "Strategic/Assignments.h"
#include "Strategic/CampaignTypes.h"
#include "Strategic/GameClock.h"
#include "Strategic/MapScreen.h"
#include "Strategic/MercContract.h"
#include "Strategic/StrategicMap.h"
#include "Tactical/AnimationData.h"
#include "Tactical/InterfaceItems.h"
#include "Tactical/Overhead.h"
#include "Tactical/SoldierMacros.h"
#include "Tactical/SoldierProfile.h"
#include "Tactical/Weapons.h"
#include "TileEngine/RenderDirty.h"
#include "Utils/Cursors.h"
#include "Utils/FontControl.h"
#include "Utils/Text.h"
#include "Utils/WordWrap.h"

#define INVENTORY_BOX_X 399
#define INVENTORY_BOX_Y 205
#define INVENTORY_BOX_W 171
#define INVENTORY_BOX_H 232

#define IMAGE_BOX_X 395
#define IMAGE_BOX_Y LAPTOP_SCREEN_UL_Y + 24
#define IMAGE_NAME_WIDTH 106
#define IMAGE_FULL_NAME_OFFSET_Y 111
#define TEXT_BOX_WIDTH 160
#define TEXT_DELTA_OFFSET 9
#define PERS_CURR_TEAM_X LAPTOP_SCREEN_UL_X + 39 - 15
#define PERS_CURR_TEAM_Y LAPTOP_SCREEN_UL_Y + 218
#define PERS_DEPART_TEAM_Y LAPTOP_SCREEN_UL_Y + 247

#define MAX_STATS 20
#define PERS_FONT FONT10ARIAL
#define CHAR_NAME_FONT FONT12ARIAL
#define CHAR_NAME_LOC_X 432
#define CHAR_NAME_LOC_WIDTH 124
#define CHAR_NAME_Y 177
#define CHAR_LOC_Y 189
#define PERS_TEXT_FONT_COLOR FONT_WHITE
#define PERS_TEXT_FONT_ALTERNATE_COLOR FONT_YELLOW
#define PERS_FONT_COLOR FONT_WHITE

#define NEXT_MERC_FACE_X LAPTOP_SCREEN_UL_X + 448
#define MERC_FACE_SCROLL_Y LAPTOP_SCREEN_UL_Y + 150
#define PREV_MERC_FACE_X LAPTOP_SCREEN_UL_X + 285

#define DEPARTED_X LAPTOP_SCREEN_UL_X + 29 - 10
#define DEPARTED_Y LAPTOP_SCREEN_UL_Y + 207

#define PERSONNEL_PORTRAIT_NUMBER 20
#define PERSONNEL_PORTRAIT_NUMBER_WIDTH 5

#define SMALL_PORTRAIT_WIDTH 46
#define SMALL_PORTRAIT_HEIGHT 42

#define SMALL_PORT_WIDTH 52
#define SMALL_PORT_HEIGHT 45

#define SMALL_PORTRAIT_WIDTH_NO_BORDERS 48

#define SMALL_PORTRAIT_START_X 141 - 10
#define SMALL_PORTRAIT_START_Y 53

#define PERS_CURR_TEAM_COST_X LAPTOP_SCREEN_UL_X + 150 - 10
#define PERS_CURR_TEAM_COST_Y LAPTOP_SCREEN_UL_Y + 218

#define PERS_CURR_TEAM_HIGHEST_Y PERS_CURR_TEAM_COST_Y + 15
#define PERS_CURR_TEAM_LOWEST_Y PERS_CURR_TEAM_HIGHEST_Y + 15

#define PERS_CURR_TEAM_WIDTH 286 - 160

#define PERS_DEPART_TEAM_WIDTH PERS_CURR_TEAM_WIDTH - 20

#define PERS_STAT_AVG_X LAPTOP_SCREEN_UL_X + 157 - 10
#define PERS_STAT_AVG_Y LAPTOP_SCREEN_UL_Y + 274
#define PERS_STAT_AVG_WIDTH 202 - 159
#define PERS_STAT_LOWEST_X LAPTOP_SCREEN_UL_X + 72 - 10
#define PERS_STAT_LOWEST_WIDTH 155 - 75
#define PERS_STAT_HIGHEST_X LAPTOP_SCREEN_UL_X + 205 - 10
#define PERS_STAT_LIST_X LAPTOP_SCREEN_UL_X + 33 - 10

#define PERS_TOGGLE_CUR_DEPART_WIDTH 106 - 35
#define PERS_TOGGLE_CUR_DEPART_HEIGHT 236 - 212

#define PERS_TOGGLE_CUR_DEPART_X LAPTOP_SCREEN_UL_X + 35 - 10
#define PERS_TOGGLE_CUR_Y LAPTOP_SCREEN_UL_Y + 208
#define PERS_TOGGLE_DEPART_Y LAPTOP_SCREEN_UL_Y + 238

#define PERS_DEPARTED_UP_X LAPTOP_SCREEN_UL_X + 265 - 10
#define PERS_DEPARTED_UP_Y LAPTOP_SCREEN_UL_Y + 210
#define PERS_DEPARTED_DOWN_Y LAPTOP_SCREEN_UL_Y + 237

#define PERS_TITLE_X 140
#define PERS_TITLE_Y 33

#define ATM_UL_X LAPTOP_SCREEN_UL_X + 397
#define ATM_UL_Y LAPTOP_SCREEN_UL_Y + 27

#define ATM_FONT PERS_FONT

// departed states
enum {
  DEPARTED_DEAD = 0,
  DEPARTED_FIRED,
  DEPARTED_MARRIED,
  DEPARTED_CONTRACT_EXPIRED,
  DEPARTED_QUIT,
};

// atm button positions
#define ATM_DISPLAY_X 509
#define ATM_DISPLAY_Y 58
#define ATM_DISPLAY_HEIGHT 10
#define ATM_DISPLAY_WIDTH 81

// the number of inventory items per personnel page
#define NUMBER_OF_INVENTORY_PERSONNEL 8
#define Y_SIZE_OF_PERSONNEL_SCROLL_REGION (422 - 219)
#define X_SIZE_OF_PERSONNEL_SCROLL_REGION (589 - 573)
#define Y_OF_PERSONNEL_SCROLL_REGION 219
#define X_OF_PERSONNEL_SCROLL_REGION 573
#define SIZE_OF_PERSONNEL_CURSOR 19

#define CFOR_EACH_PERSONNEL(iter)            \
  CFOR_EACH_IN_TEAM(iter, OUR_TEAM)          \
  if (iter->uiStatusFlags & SOLDIER_VEHICLE) \
    continue;                                \
  else

// enums for the buttons in the information side bar (used with
// giPersonnelATMStartButton[])
enum { PERSONNEL_STAT_BTN, PERSONNEL_EMPLOYMENT_BTN, PERSONNEL_INV_BTN };

// enums for the current state of the information side bar (stat panel)
enum {
  PRSNL_STATS,
  PRSNL_EMPLOYMENT,
  PRSNL_INV,
};
static uint8_t gubPersonnelInfoState = PRSNL_STATS;

// enums for the pPersonnelScreenStrings[]
enum {
  PRSNL_TXT_MED_DEPOSIT,       // AMOUNT OF MEDICAL DEPOSIT PUT DOWN ON THE MERC
  PRSNL_TXT_CURRENT_CONTRACT,  // COST OF CURRENT CONTRACT
  PRSNL_TXT_KILLS,             // NUMBER OF KILLS BY MERC
  PRSNL_TXT_ASSISTS,           // NUMBER OF ASSISTS ON KILLS BY MERC
  PRSNL_TXT_DAILY_COST,        // DAILY COST OF MERC
  PRSNL_TXT_TOTAL_COST,        // TOTAL COST OF MERC
  PRSNL_TXT_CONTRACT,          // COST OF CURRENT CONTRACT
  PRSNL_TXT_TOTAL_SERVICE,     // TOTAL SERVICE RENDERED BY MERC
  PRSNL_TXT_UNPAID_AMOUNT,     // AMOUNT LEFT ON MERC MERC TO BE PAID
  PRSNL_TXT_HIT_PERCENTAGE,    // PERCENTAGE OF SHOTS THAT HIT TARGET
  PRSNL_TXT_BATTLES,           // NUMBER OF BATTLES FOUGHT
  PRSNL_TXT_TIMES_WOUNDED,     // NUMBER OF TIMES MERC HAS BEEN WOUNDED
  PRSNL_TXT_SKILLS,
  PRSNL_TXT_NOSKILLS,
};

static uint8_t uiCurrentInventoryIndex = 0;

static uint32_t guiSliderPosition;

static const int16_t pers_stat_x = 407;
static const int16_t pers_stat_delta_x = 407 + TEXT_BOX_WIDTH - 20 + TEXT_DELTA_OFFSET;
static const int16_t pers_stat_data_x = 407 + 36;
static const int16_t pers_stat_y[] = {215, 225, 235, 245, 255, 265, 325, 280, 290, 300,
                                      310,  // 10
                                      405, 395, 425, 435, 455,
                                      390,  // for contract price
                                      445,
                                      33,  // Personnel Header // XXX unused
                                      340,
                                      350,  // 20
                                      365, 375, 385, 395, 405};

static SGPVObject *guiSCREEN;
static SGPVObject *guiTITLE;
static SGPVObject *guiDEPARTEDTEAM;
static SGPVObject *guiCURRENTTEAM;
static SGPVObject *guiPersonnelInventory;

static struct {
  GUIButtonRef prev;
  GUIButtonRef next;
  GUIButtonRef depart_up;
  GUIButtonRef depart_dn;
} g_personnel;

static GUIButtonRef giPersonnelInventoryButtons[2];

// buttons for ATM
static BUTTON_PICS *giPersonnelATMStartButtonImage[3];
static GUIButtonRef giPersonnelATMStartButton[3];

// the id of currently displayed merc in right half of screen
static int32_t iCurrentPersonSelectedId = -1;

static int32_t giCurrentUpperLeftPortraitNumber = 0;

// which mode are we showing?..current team?...or deadly departed?
static BOOLEAN fCurrentTeamMode = TRUE;

// mouse regions
static MOUSE_REGION gPortraitMouseRegions[PERSONNEL_PORTRAIT_NUMBER];

static MOUSE_REGION gTogglePastCurrentTeam[2];

static MOUSE_REGION gMouseScrollPersonnelINV;

static void InitPastCharactersList();

void GameInitPersonnel() {
  // init past characters lists
  InitPastCharactersList();
}

static void CreateDestroyCurrentDepartedMouseRegions(BOOLEAN create);
static void CreateDestroyMouseRegionsForPersonnelPortraits(BOOLEAN create);
static void CreateDestroyStartATMButton(BOOLEAN create);
static void CreatePersonnelButtons();
static void SelectFirstDisplayedMerc();
static void LoadPersonnelGraphics();
static void LoadPersonnelScreenBackgroundGraphics();
static void SetPersonnelButtonStates();

void EnterPersonnel() {
  fReDrawScreenFlag = TRUE;

  uiCurrentInventoryIndex = 0;
  guiSliderPosition = 0;

  // load graphics for screen
  LoadPersonnelGraphics();

  // show atm panel
  CreateDestroyStartATMButton(TRUE);

  // load personnel
  LoadPersonnelScreenBackgroundGraphics();

  SelectFirstDisplayedMerc();

  // render screen
  RenderPersonnel();

  CreateDestroyMouseRegionsForPersonnelPortraits(TRUE);
  CreateDestroyCurrentDepartedMouseRegions(TRUE);

  // create buttons for screen
  CreatePersonnelButtons();

  // set states of en- dis able buttons
  SetPersonnelButtonStates();
}

static void CreateDestroyButtonsForDepartedTeamList(BOOLEAN create);
static void CreateDestroyPersonnelInventoryScrollButtons();
static void DeletePersonnelButtons();
static void DeletePersonnelScreenBackgroundGraphics();
static void RemovePersonnelGraphics();

void ExitPersonnel() {
  CreateDestroyButtonsForDepartedTeamList(FALSE);

  // get rid of atm panel buttons
  CreateDestroyStartATMButton(FALSE);

  gubPersonnelInfoState = PRSNL_STATS;

  CreateDestroyPersonnelInventoryScrollButtons();

  // get rid of graphics
  RemovePersonnelGraphics();

  DeletePersonnelScreenBackgroundGraphics();

  // delete buttons
  DeletePersonnelButtons();

  CreateDestroyMouseRegionsForPersonnelPortraits(FALSE);
  CreateDestroyCurrentDepartedMouseRegions(FALSE);
}

static void EnableDisableDeparturesButtons();
static void EnableDisableInventoryScrollButtons();
static void HandlePersonnelKeyboard();

void HandlePersonnel() {
  // create / destroy buttons for scrolling departed list
  CreateDestroyButtonsForDepartedTeamList(!fCurrentTeamMode);

  // enable / disable departures buttons
  EnableDisableDeparturesButtons();

  // create destroy inv buttons as needed
  CreateDestroyPersonnelInventoryScrollButtons();

  // enable disable buttons as needed
  EnableDisableInventoryScrollButtons();

  HandlePersonnelKeyboard();
}

static void LoadPersonnelGraphics() {
  // load graphics needed for personnel screen

  // title bar
  guiTITLE = AddVideoObjectFromFile(LAPTOPDIR "/programtitlebar.sti");

  // the background grpahics
  guiSCREEN = AddVideoObjectFromFile(LAPTOPDIR "/personnelwindow.sti");

  guiPersonnelInventory = AddVideoObjectFromFile(LAPTOPDIR "/personnel_inventory.sti");
}

static void RemovePersonnelGraphics() {
  // delete graphics needed for personnel screen
  DeleteVideoObject(guiSCREEN);
  DeleteVideoObject(guiTITLE);
  DeleteVideoObject(guiPersonnelInventory);
}

static void DisplayFaceOfDisplayedMerc();
static void DisplayPastMercsPortraits();
static void DisplayPersonnelSummary();
static void DisplayPersonnelTextOnTitleBar();
static void DisplayPicturesOfCurrentTeam();
static void DisplayTeamStats();
static void RenderAtmPanel();
static void RenderPersonnelScreenBackground();
static void UpDateStateOfStartButton();

void RenderPersonnel() {
  // re-renders personnel screen
  // render main background

  BltVideoObject(FRAME_BUFFER, guiTITLE, 0, LAPTOP_SCREEN_UL_X, LAPTOP_SCREEN_UL_Y - 2);
  BltVideoObject(FRAME_BUFFER, guiSCREEN, 0, LAPTOP_SCREEN_UL_X, LAPTOP_SCREEN_UL_Y + 22);

  // render personnel screen background
  RenderPersonnelScreenBackground();

  // show team
  DisplayPicturesOfCurrentTeam();

  DisplayPastMercsPortraits();

  RenderAtmPanel();

  // show selected merc
  DisplayFaceOfDisplayedMerc();

  DisplayPersonnelSummary();
  DisplayTeamStats();

  // title bar
  BlitTitleBarIcons();

  // show text on titlebar
  DisplayPersonnelTextOnTitleBar();

  // en-dis-able start button
  UpDateStateOfStartButton();
}

static void DisplayCharStats(SOLDIERTYPE const &s);
static void DisplayEmploymentinformation(SOLDIERTYPE const &s);
static void DisplayCharInventory(SOLDIERTYPE const &);

static void RenderPersonnelStats(SOLDIERTYPE const &s) {
  // will render the stats of person iId
  SetFontAttributes(PERS_FONT, PERS_TEXT_FONT_COLOR);

  switch (gubPersonnelInfoState) {
    case PRSNL_STATS:
      DisplayCharStats(s);
      break;
    case PRSNL_EMPLOYMENT:
      DisplayEmploymentinformation(s);
      break;
    case PRSNL_INV:
      DisplayCharInventory(s);
      break;
  }
}

static void RenderPersonnelFace(MERCPROFILESTRUCT const &p, BOOLEAN const alive) try {
  {
    AutoSGPVObject guiFACE(LoadBigPortrait(p));
    if (!alive) {
      guiFACE->pShades[0] =
          Create16BPPPaletteShaded(guiFACE->Palette(), DEAD_MERC_COLOR_RED, DEAD_MERC_COLOR_GREEN,
                                   DEAD_MERC_COLOR_BLUE, TRUE);
      // set the red pallete to the face
      guiFACE->CurrentShade(0);
    }
    BltVideoObject(FRAME_BUFFER, guiFACE, 0, IMAGE_BOX_X, IMAGE_BOX_Y);
  }

  // Display the merc's name on the portrait
  const wchar_t *name = p.zName;

  const uint16_t x = IMAGE_BOX_X;
  const uint16_t y = IMAGE_BOX_Y + IMAGE_FULL_NAME_OFFSET_Y;
  const uint16_t w = IMAGE_NAME_WIDTH;
  const int32_t iHeightOfText = DisplayWrappedString(x, y, w, 1, PERS_FONT, PERS_FONT_COLOR, name,
                                                     0, CENTER_JUSTIFIED | DONT_DISPLAY_TEXT);

  const uint16_t line_height = GetFontHeight(PERS_FONT);
  if (iHeightOfText - 2 > line_height)  // If the string will wrap
  {
    // Raise where we display it, and wrap it
    DisplayWrappedString(x, y - line_height, w, 1, PERS_FONT, PERS_FONT_COLOR, name, 0,
                         CENTER_JUSTIFIED);
  } else {
    DrawTextToScreen(name, x, y, w, PERS_FONT, PERS_FONT_COLOR, 0, CENTER_JUSTIFIED);
  }
} catch (...) { /* XXX ignore */
}

static int32_t GetNumberOfMercsDeadOrAliveOnPlayersTeam();
static int32_t GetNumberOfPastMercsOnPlayersTeam();

static void NextPersonnelFace() {
  if (iCurrentPersonSelectedId == -1) return;

  if (fCurrentTeamMode) {
    // wrap around?
    if (iCurrentPersonSelectedId == GetNumberOfMercsDeadOrAliveOnPlayersTeam() - 1) {
      iCurrentPersonSelectedId = 0;
    } else {
      iCurrentPersonSelectedId++;
    }
  } else {
    if (iCurrentPersonSelectedId + 1 ==
        GetNumberOfPastMercsOnPlayersTeam() - giCurrentUpperLeftPortraitNumber) {
      // about to go off the end
      giCurrentUpperLeftPortraitNumber = 0;
      iCurrentPersonSelectedId = 0;
    } else if (iCurrentPersonSelectedId == 19) {
      giCurrentUpperLeftPortraitNumber += PERSONNEL_PORTRAIT_NUMBER;
      iCurrentPersonSelectedId = 0;
    } else {
      ++iCurrentPersonSelectedId;
    }
    fReDrawScreenFlag = TRUE;
  }
}

static void PrevPersonnelFace() {
  if (iCurrentPersonSelectedId == -1) return;

  if (fCurrentTeamMode) {
    // wrap around?
    if (iCurrentPersonSelectedId == 0) {
      iCurrentPersonSelectedId = GetNumberOfMercsDeadOrAliveOnPlayersTeam() - 1;
    } else {
      iCurrentPersonSelectedId--;
    }
  } else {
    if (iCurrentPersonSelectedId == 0 && giCurrentUpperLeftPortraitNumber == 0) {
      // about to go off the end
      int32_t count_past = GetNumberOfPastMercsOnPlayersTeam();
      giCurrentUpperLeftPortraitNumber = count_past - count_past % PERSONNEL_PORTRAIT_NUMBER;
      iCurrentPersonSelectedId = count_past % PERSONNEL_PORTRAIT_NUMBER - 1;
    } else if (iCurrentPersonSelectedId == 0) {
      giCurrentUpperLeftPortraitNumber -= PERSONNEL_PORTRAIT_NUMBER;
      iCurrentPersonSelectedId = 19;
    } else {
      --iCurrentPersonSelectedId;
    }
    fReDrawScreenFlag = TRUE;
  }
}

static GUIButtonRef MakeButton(char const *const gfx, int32_t const off_normal,
                               int32_t const on_normal, int16_t const x, int16_t const y,
                               GUI_CALLBACK const click) {
  GUIButtonRef const b =
      QuickCreateButtonImg(gfx, off_normal, on_normal, x, y, MSYS_PRIORITY_HIGHEST - 1, click);
  b->SetCursor(CURSOR_LAPTOP_SCREEN);
  return b;
}

static void LeftButtonCallBack(GUI_BUTTON *btn, int32_t reason);
static void RightButtonCallBack(GUI_BUTTON *btn, int32_t reason);

static void CreatePersonnelButtons() {
  // left/right buttons
  g_personnel.prev = MakeButton(LAPTOPDIR "/personnelbuttons.sti", 0, 1, PREV_MERC_FACE_X,
                                MERC_FACE_SCROLL_Y, LeftButtonCallBack);
  g_personnel.next = MakeButton(LAPTOPDIR "/personnelbuttons.sti", 2, 3, NEXT_MERC_FACE_X,
                                MERC_FACE_SCROLL_Y, RightButtonCallBack);
}

static void DeletePersonnelButtons() {
  RemoveButton(g_personnel.prev);
  RemoveButton(g_personnel.next);
}

static void LeftButtonCallBack(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    fReDrawScreenFlag = TRUE;
    PrevPersonnelFace();
    uiCurrentInventoryIndex = 0;
    guiSliderPosition = 0;
  }
}

static void RightButtonCallBack(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    fReDrawScreenFlag = TRUE;
    NextPersonnelFace();
    uiCurrentInventoryIndex = 0;
    guiSliderPosition = 0;
  }
}

static void DisplayCharName(SOLDIERTYPE const &s) {
  // get merc's nickName, assignment, and sector location info
  int16_t sX, sY;

  SetFontAttributes(CHAR_NAME_FONT, PERS_TEXT_FONT_COLOR);

  const wchar_t *sTownName = NULL;
  if (s.bAssignment != ASSIGNMENT_POW && s.bAssignment != IN_TRANSIT) {
    // name of town, if any
    int8_t const bTownId = GetTownIdForSector(SECTOR(s.sSectorX, s.sSectorY));
    if (bTownId != BLANK_SECTOR) sTownName = pTownNames[bTownId];
  }

  wchar_t sString[64];
  if (sTownName != NULL) {
    // nick name - town name
    swprintf(sString, lengthof(sString), L"%ls - %ls", s.name, sTownName);
  } else {
    // nick name
    wcsncpy(sString, s.name, lengthof(sString));
  }
  FindFontCenterCoordinates(CHAR_NAME_LOC_X, 0, CHAR_NAME_LOC_WIDTH, 0, sString, CHAR_NAME_FONT,
                            &sX, &sY);
  MPrint(sX, CHAR_NAME_Y, sString);

  wchar_t const *const Assignment = pLongAssignmentStrings[s.bAssignment];
  FindFontCenterCoordinates(CHAR_NAME_LOC_X, 0, CHAR_NAME_LOC_WIDTH, 0, Assignment, CHAR_NAME_FONT,
                            &sX, &sY);
  MPrint(sX, CHAR_LOC_Y, Assignment);
}

static void PrintStatWithDelta(uint32_t idx, int8_t stat, int8_t delta) {
  wchar_t sString[50];
  int16_t sX;
  int16_t sY;

  const int32_t y = pers_stat_y[idx];
  if (delta > 0) {
    swprintf(sString, lengthof(sString), L"( %+d )", delta);
    FindFontRightCoordinates(pers_stat_delta_x, 0, 30, 0, sString, PERS_FONT, &sX, &sY);
    MPrint(sX, y, sString);
  }
  swprintf(sString, lengthof(sString), L"%d", stat);
  mprintf(pers_stat_x, y, L"%ls:", str_stat_list[idx]);
  FindFontRightCoordinates(pers_stat_x, 0, TEXT_BOX_WIDTH - 20, 0, sString, PERS_FONT, &sX, &sY);
  MPrint(sX, y, sString);
}

static void PrintStat(uint16_t stat, int32_t y, const wchar_t *text) {
  wchar_t sString[50];
  int16_t sX;
  int16_t sY;

  mprintf(pers_stat_x, y, L"%ls:", text);
  swprintf(sString, lengthof(sString), L"%d", stat);
  FindFontRightCoordinates(pers_stat_x, 0, TEXT_BOX_WIDTH - 20, 0, sString, PERS_FONT, &sX, &sY);
  MPrint(sX, y, sString);
}

static void DisplayCharStats(SOLDIERTYPE const &s) {
  wchar_t sString[50];
  int16_t sX;
  int16_t sY;

  MERCPROFILESTRUCT const &p = GetProfile(s.ubProfile);
  BOOLEAN const fAmIaRobot = AM_A_ROBOT(&s);

  // Health
  if (s.bAssignment != ASSIGNMENT_POW) {
    if (p.bLifeDelta > 0) {
      swprintf(sString, lengthof(sString), L"( %+d )", p.bLifeDelta);
      FindFontRightCoordinates(pers_stat_delta_x, 0, 30, 0, sString, PERS_FONT, &sX, &sY);
      MPrint(sX, pers_stat_y[0], sString);
    }
    swprintf(sString, lengthof(sString), L"%d/%d", s.bLife, s.bLifeMax);
  } else {
    wcsncpy(sString, pPOWStrings[1], lengthof(sString));
  }
  mprintf(pers_stat_x, pers_stat_y[0], L"%ls:", str_stat_health);
  FindFontRightCoordinates(pers_stat_x, 0, TEXT_BOX_WIDTH - 20, 0, sString, PERS_FONT, &sX, &sY);
  MPrint(sX, pers_stat_y[0], sString);

  if (fAmIaRobot) {
    for (int32_t i = 1; i < 11; ++i) {
      const int32_t y = pers_stat_y[i];
      mprintf(pers_stat_x, y, L"%ls:", str_stat_list[i]);
      const wchar_t *const na = gpStrategicString[STR_PB_NOTAPPLICABLE_ABBREVIATION];
      FindFontRightCoordinates(pers_stat_x, 0, TEXT_BOX_WIDTH - 20, 0, na, PERS_FONT, &sX, &sY);
      MPrint(sX, y, na);
    }
  } else {
    PrintStatWithDelta(1, p.bAgility, p.bAgilityDelta);
    PrintStatWithDelta(2, p.bDexterity, p.bDexterityDelta);
    PrintStatWithDelta(3, p.bStrength, p.bStrengthDelta);
    PrintStatWithDelta(4, p.bLeadership, p.bLeadershipDelta);
    PrintStatWithDelta(5, p.bWisdom, p.bWisdomDelta);
    PrintStatWithDelta(6, p.bExpLevel, p.bExpLevelDelta);
    PrintStatWithDelta(7, p.bMarksmanship, p.bMarksmanshipDelta);
    PrintStatWithDelta(8, p.bMechanical, p.bMechanicDelta);
    PrintStatWithDelta(9, p.bExplosive, p.bExplosivesDelta);
    PrintStatWithDelta(10, p.bMedical, p.bMedicalDelta);
  }

  PrintStat(p.usKills, pers_stat_y[21], pPersonnelScreenStrings[PRSNL_TXT_KILLS]);
  PrintStat(p.usAssists, pers_stat_y[22], pPersonnelScreenStrings[PRSNL_TXT_ASSISTS]);

  // Shots/hits
  MPrint(pers_stat_x, pers_stat_y[23], pPersonnelScreenStrings[PRSNL_TXT_HIT_PERCENTAGE]);
  // check we have shot at least once
  const uint32_t fired = p.usShotsFired;
  const uint32_t hits = (fired > 0 ? 100 * p.usShotsHit / fired : 0);
  swprintf(sString, lengthof(sString), L"%d %%", hits);
  FindFontRightCoordinates(pers_stat_x, 0, TEXT_BOX_WIDTH - 20, 0, sString, PERS_FONT, &sX, &sY);
  MPrint(sX, pers_stat_y[23], sString);

  PrintStat(p.usBattlesFought, pers_stat_y[24], pPersonnelScreenStrings[PRSNL_TXT_BATTLES]);
  PrintStat(p.usTimesWounded, pers_stat_y[25], pPersonnelScreenStrings[PRSNL_TXT_TIMES_WOUNDED]);

  // Display the 'Skills' text
  MPrint(pers_stat_x, pers_stat_y[19], pPersonnelScreenStrings[PRSNL_TXT_SKILLS]);

  /* KM: April 16, 1999
   * Added support for the German version, which has potential string
   * overrun problems.  For example, the text "Skills:" can overlap
   * "NightOps (Expert)" because the German strings are much longer.  In
   * these cases, I ensure that the right justification of the traits
   * do not overlap.  If it would, I move it over to the right. */
  const int32_t iWidth = StringPixLength(pPersonnelScreenStrings[PRSNL_TXT_SKILLS], PERS_FONT);
  const int32_t iMinimumX = iWidth + pers_stat_x + 2;

  if (!fAmIaRobot) {
    int8_t bSkill1 = p.bSkillTrait;
    int8_t bSkill2 = p.bSkillTrait2;

    if (bSkill1 == NO_SKILLTRAIT) {
      bSkill1 = bSkill2;
      bSkill2 = NO_SKILLTRAIT;
    }

    if (bSkill1 != NO_SKILLTRAIT) {
      if (bSkill1 == bSkill2) {
        // The 2 skills are the same, add the '(expert)' at the end
        swprintf(sString, lengthof(sString), L"%ls %ls", gzMercSkillText[bSkill1],
                 gzMercSkillText[NUM_SKILLTRAITS]);
        FindFontRightCoordinates(pers_stat_x, 0, TEXT_BOX_WIDTH - 20, 0, sString, PERS_FONT, &sX,
                                 &sY);

        // KM: April 16, 1999
        // Perform the potential overrun check
        if (sX <= iMinimumX) {
          FindFontRightCoordinates(pers_stat_x + TEXT_BOX_WIDTH - 20 + TEXT_DELTA_OFFSET, 0, 30, 0,
                                   sString, PERS_FONT, &sX, &sY);
          sX = std::max((int16_t)sX, (int16_t)iMinimumX);
        }

        MPrint(sX, pers_stat_y[19], sString);
      } else {
        const wchar_t *Skill = gzMercSkillText[bSkill1];

        FindFontRightCoordinates(pers_stat_x, 0, TEXT_BOX_WIDTH - 20, 0, Skill, PERS_FONT, &sX,
                                 &sY);

        // KM: April 16, 1999
        // Perform the potential overrun check
        sX = std::max((int16_t)sX, (int16_t)iMinimumX);

        MPrint(sX, pers_stat_y[19], Skill);

        if (bSkill2 != NO_SKILLTRAIT) {
          const wchar_t *Skill = gzMercSkillText[bSkill2];

          FindFontRightCoordinates(pers_stat_x, 0, TEXT_BOX_WIDTH - 20, 0, Skill, PERS_FONT, &sX,
                                   &sY);

          // KM: April 16, 1999
          // Perform the potential overrun check
          sX = std::max((int16_t)sX, (int16_t)iMinimumX);

          MPrint(sX, pers_stat_y[20], Skill);
        }
      }
    } else {
      const wchar_t *NoSkill = pPersonnelScreenStrings[PRSNL_TXT_NOSKILLS];
      FindFontRightCoordinates(pers_stat_x, 0, TEXT_BOX_WIDTH - 20, 0, NoSkill, PERS_FONT, &sX,
                               &sY);
      MPrint(sX, pers_stat_y[19], NoSkill);
    }
  }
}

static void SetPersonnelButtonStates() {
  // this function will look at what page we are viewing, enable and disable
  // buttons as needed
  const int32_t merc_count = fCurrentTeamMode ? GetNumberOfMercsDeadOrAliveOnPlayersTeam()
                                              : GetNumberOfPastMercsOnPlayersTeam();
  bool const enable = merc_count > 1;
  EnableButton(g_personnel.prev, enable);
  EnableButton(g_personnel.next, enable);
}

static void RenderPersonnelScreenBackground() {
  // this fucntion will render the background for the personnel screen
  BltVideoObject(FRAME_BUFFER, fCurrentTeamMode ? guiCURRENTTEAM : guiDEPARTEDTEAM, 0, DEPARTED_X,
                 DEPARTED_Y);
}

static void LoadPersonnelScreenBackgroundGraphics() {
  // will load the graphics for the personeel screen background

  // departed bar
  guiDEPARTEDTEAM = AddVideoObjectFromFile(LAPTOPDIR "/departed.sti");

  // current bar
  guiCURRENTTEAM = AddVideoObjectFromFile(LAPTOPDIR "/currentteam.sti");
}

static void DeletePersonnelScreenBackgroundGraphics() {
  // delete background V/O's
  DeleteVideoObject(guiCURRENTTEAM);
  DeleteVideoObject(guiDEPARTEDTEAM);
}

static int32_t GetNumberOfMercsDeadOrAliveOnPlayersTeam() {
  int32_t iCounter = 0;

  // grab number on team
  CFOR_EACH_PERSONNEL(s)++ iCounter;
  return iCounter;
}

static void PersonnelPortraitCallback(MOUSE_REGION *pRegion, int32_t iReason);

static void CreateDestroyMouseRegionsForPersonnelPortraits(BOOLEAN create) {
  // creates/destroys mouse regions for portraits
  static BOOLEAN fCreated = FALSE;

  if (!fCreated && create) {
    // create regions
    for (int16_t i = 0; i < PERSONNEL_PORTRAIT_NUMBER; i++) {
      const uint16_t tlx =
          SMALL_PORTRAIT_START_X + i % PERSONNEL_PORTRAIT_NUMBER_WIDTH * SMALL_PORT_WIDTH;
      const uint16_t tly =
          SMALL_PORTRAIT_START_Y + i / PERSONNEL_PORTRAIT_NUMBER_WIDTH * SMALL_PORT_HEIGHT;
      const uint16_t brx = tlx + SMALL_PORTRAIT_WIDTH;
      const uint16_t bry = tly + SMALL_PORTRAIT_HEIGHT;
      MSYS_DefineRegion(&gPortraitMouseRegions[i], tlx, tly, brx, bry, MSYS_PRIORITY_HIGHEST,
                        CURSOR_LAPTOP_SCREEN, MSYS_NO_CALLBACK, PersonnelPortraitCallback);
      MSYS_SetRegionUserData(&gPortraitMouseRegions[i], 0, i);
    }

    fCreated = TRUE;
  } else if (fCreated && !create) {
    // destroy regions
    for (int16_t i = 0; i < PERSONNEL_PORTRAIT_NUMBER; i++) {
      MSYS_RemoveRegion(&gPortraitMouseRegions[i]);
    }

    fCreated = FALSE;
  }
}

static void DisplayPicturesOfCurrentTeam() try {
  // will display the small portraits of the current team
  if (!fCurrentTeamMode) return;

  int32_t i = 0;
  CFOR_EACH_PERSONNEL(s) {
    // found the next actual guy
    int32_t const x =
        SMALL_PORTRAIT_START_X + i % PERSONNEL_PORTRAIT_NUMBER_WIDTH * SMALL_PORT_WIDTH;
    int32_t const y =
        SMALL_PORTRAIT_START_Y + i / PERSONNEL_PORTRAIT_NUMBER_WIDTH * SMALL_PORT_HEIGHT;
    {
      AutoSGPVObject guiFACE(LoadSmallPortrait(GetProfile(s->ubProfile)));
      if (s->bLife <= 0) {
        guiFACE->pShades[0] =
            Create16BPPPaletteShaded(guiFACE->Palette(), DEAD_MERC_COLOR_RED, DEAD_MERC_COLOR_GREEN,
                                     DEAD_MERC_COLOR_BLUE, TRUE);
        // set the red pallete to the face
        guiFACE->CurrentShade(0);
      }
      BltVideoObject(FRAME_BUFFER, guiFACE, 0, x, y);
    }

    if (s->bLife <= 0) {
      // if the merc is dead, display it
      DrawTextToScreen(AimPopUpText[AIM_MEMBER_DEAD], x, y + SMALL_PORT_HEIGHT / 2,
                       SMALL_PORTRAIT_WIDTH_NO_BORDERS, FONT10ARIAL, 145, FONT_MCOLOR_BLACK,
                       CENTER_JUSTIFIED);
    }

    i++;
  }
} catch (...) { /* XXX ignore */
}

static SOLDIERTYPE const &GetSoldierOfCurrentSlot();

static void PersonnelPortraitCallback(MOUSE_REGION *pRegion, int32_t iReason) {
  int32_t iPortraitId = 0;
  int32_t iOldPortraitId;

  iPortraitId = MSYS_GetRegionUserData(pRegion, 0);
  iOldPortraitId = iCurrentPersonSelectedId;

  // callback handler for the minize region that is attatched to the laptop
  // program icon
  if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    // get id of portrait
    if (fCurrentTeamMode) {
      // valid portrait, set up id
      if (iPortraitId >= GetNumberOfMercsDeadOrAliveOnPlayersTeam()) {
        // not a valid id, leave
        return;
      }

      iCurrentPersonSelectedId = iPortraitId;
      fReDrawScreenFlag = TRUE;

      if (iCurrentPersonSelectedId != -1 &&
          GetSoldierOfCurrentSlot().bAssignment == ASSIGNMENT_POW &&
          gubPersonnelInfoState == PRSNL_INV) {
        gubPersonnelInfoState = PRSNL_STATS;
      }
    } else {
      if (iPortraitId >= GetNumberOfPastMercsOnPlayersTeam() - giCurrentUpperLeftPortraitNumber) {
        return;
      }
      iCurrentPersonSelectedId = iPortraitId;
      fReDrawScreenFlag = TRUE;
    }

    if (iOldPortraitId != iPortraitId) {
      uiCurrentInventoryIndex = 0;
      guiSliderPosition = 0;
    }
  }

  if (iReason & MSYS_CALLBACK_REASON_RBUTTON_UP) {
    if (fCurrentTeamMode) {
      // valid portrait, set up id
      if (iPortraitId >= GetNumberOfMercsDeadOrAliveOnPlayersTeam()) {
        // not a valid id, leave
        return;
      }

      // if the user is rigt clicking on the same face
      if (iCurrentPersonSelectedId == iPortraitId) {
        // increment the info page when the user right clicks
        if (gubPersonnelInfoState < PRSNL_INV)
          gubPersonnelInfoState++;
        else
          gubPersonnelInfoState = PRSNL_STATS;
      }

      iCurrentPersonSelectedId = iPortraitId;
      fReDrawScreenFlag = TRUE;

      uiCurrentInventoryIndex = 0;
      guiSliderPosition = 0;

      // if the selected merc is valid, and they are a POW, change to the
      // inventory display
      if (iCurrentPersonSelectedId != -1 &&
          GetSoldierOfCurrentSlot().bAssignment == ASSIGNMENT_POW &&
          gubPersonnelInfoState == PRSNL_INV) {
        gubPersonnelInfoState = PRSNL_STATS;
      }
    }
  }
}

struct PastMercInfo {
  PastMercInfo(MERCPROFILESTRUCT const *const profile_, int8_t const state_)
      : profile(profile_), state(state_) {}

  MERCPROFILESTRUCT const *profile;
  int8_t state;
};

static void DisplayAmountOnChar(SOLDIERTYPE const &);
static void DisplayDepartedCharName(MERCPROFILESTRUCT const &, int32_t iState);
static void DisplayDepartedCharStats(MERCPROFILESTRUCT const &, int32_t iState);
static void DisplayHighLightBox(int32_t sel_id);
static PastMercInfo GetSelectedPastMercInfo();

static void DisplayFaceOfDisplayedMerc() {
  // valid person?, display
  if (iCurrentPersonSelectedId == -1) return;

  // highlight it
  DisplayHighLightBox(iCurrentPersonSelectedId);

  // if showing inventory, leave
  if (fCurrentTeamMode) {
    SOLDIERTYPE const &s = GetSoldierOfCurrentSlot();
    RenderPersonnelFace(GetProfile(s.ubProfile), s.bLife > 0);
    DisplayCharName(s);
    RenderPersonnelStats(s);
    DisplayAmountOnChar(s);
  } else {
    const PastMercInfo info = GetSelectedPastMercInfo();
    const MERCPROFILESTRUCT *const p = info.profile;
    if (p == NULL) return;
    RenderPersonnelFace(*p, info.state != DEPARTED_DEAD);
    DisplayDepartedCharName(*p, info.state);
    DisplayDepartedCharStats(*p, info.state);
  }
}

static void RenderSliderBarForPersonnelInventory();

// display the inventory for this merc
static void DisplayCharInventory(SOLDIERTYPE const &s) {
  CreateDestroyPersonnelInventoryScrollButtons();

  BltVideoObject(FRAME_BUFFER, guiPersonnelInventory, 0, 397, 200);

  // render the bar for the character
  RenderSliderBarForPersonnelInventory();

  // if this is a robot, don't display any inventory
  if (AM_A_ROBOT(&s)) return;

  int32_t item_count = -(int32_t)uiCurrentInventoryIndex;
  for (uint32_t pos = 0; pos < NUM_INV_SLOTS; ++pos) {
    // if the character is a robot, only display the inv for the hand pos
    if (s.ubProfile == ROBOT && pos != HANDPOS)
      continue;  // XXX can this ever be true? before is if (AM_A_ROBOT())
                 // return;

    OBJECTTYPE const &o = s.inv[pos];
    int32_t const o_count = o.ubNumberOfObjects;
    if (o_count == 0) continue;

    if (item_count < 0) {
      ++item_count;
      continue;
    }

    wchar_t sString[128];
    int16_t sX;
    int16_t sY;

    const int16_t PosX = 397 + 3;
    const int16_t PosY = 200 + 8 + item_count * 29;

    uint16_t const item_idx = o.usItem;
    INVTYPE const &item = Item[item_idx];

    SGPVObject const &gfx = GetInterfaceGraphicForItem(item);
    ETRLEObject const &pTrav = gfx.SubregionProperties(item.ubGraphicNum);
    int16_t const cen_x = PosX + abs(57 - pTrav.usWidth) / 2 - pTrav.sOffsetX;
    int16_t const cen_y = PosY + abs(22 - pTrav.usHeight) / 2 - pTrav.sOffsetY;
    BltVideoObjectOutline(FRAME_BUFFER, &gfx, item.ubGraphicNum, cen_x, cen_y, SGP_TRANSPARENT);

    SetFontDestBuffer(FRAME_BUFFER);

    wcsncpy(sString, ItemNames[item_idx], lengthof(sString));
    ReduceStringLength(sString, lengthof(sString), 171 - 75, FONT10ARIAL);
    MPrint(PosX + 65, PosY + 3, sString);

    // condition
    if (item.usItemClass & IC_AMMO) {
      int32_t total_ammo = 0;
      for (int32_t i = 0; i < o_count; ++i) total_ammo += o.ubShotsLeft[i];
      swprintf(sString, lengthof(sString), L"%d/%d", total_ammo,
               o_count * Magazine[item.ubClassIndex].ubMagSize);
    } else {
      swprintf(sString, lengthof(sString), L"%2d%%", o.bStatus[0]);
    }

    FindFontRightCoordinates(PosX + 65, PosY + 15, 171 - 75, GetFontHeight(FONT10ARIAL), sString,
                             FONT10ARIAL, &sX, &sY);
    MPrint(sX, sY, sString);

    if (item.usItemClass & IC_GUN) {
      wcsncpy(sString, AmmoCaliber[Weapon[item.ubClassIndex].ubCalibre], lengthof(sString));
      ReduceStringLength(sString, lengthof(sString), 171 - 75, FONT10ARIAL);
      MPrint(PosX + 65, PosY + 15, sString);
    }

    // if more than 1?
    if (o_count > 1) {
      swprintf(sString, lengthof(sString), L"x%d", o_count);
      FindFontRightCoordinates(PosX, PosY + 15, 58, GetFontHeight(FONT10ARIAL), sString,
                               FONT10ARIAL, &sX, &sY);
      MPrint(sX, sY, sString);
    }

    if (++item_count == NUMBER_OF_INVENTORY_PERSONNEL) break;
  }
}

static void FindPositionOfPersInvSlider();

static void InventoryUp() {
  if (uiCurrentInventoryIndex == 0) return;
  uiCurrentInventoryIndex--;
  fReDrawScreenFlag = TRUE;
  FindPositionOfPersInvSlider();
}

static int32_t GetNumberOfInventoryItemsOnCurrentMerc();

static void InventoryDown() {
  if ((int32_t)uiCurrentInventoryIndex >=
      (int32_t)(GetNumberOfInventoryItemsOnCurrentMerc() - NUMBER_OF_INVENTORY_PERSONNEL)) {
    return;
  }
  uiCurrentInventoryIndex++;
  fReDrawScreenFlag = TRUE;
  FindPositionOfPersInvSlider();
}

static void InventoryUpButtonCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_REPEAT || reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    InventoryUp();
  }
}

static void InventoryDownButtonCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_REPEAT || reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    InventoryDown();
  }
}

// decide which buttons can and can't be accessed based on what the current item
// is
static void EnableDisableInventoryScrollButtons() {
  if (gubPersonnelInfoState != PRSNL_INV) {
    return;
  }

  if (uiCurrentInventoryIndex == 0) {
    giPersonnelInventoryButtons[0]->uiFlags &= ~BUTTON_CLICKED_ON;
    DisableButton(giPersonnelInventoryButtons[0]);
  } else {
    EnableButton(giPersonnelInventoryButtons[0]);
  }

  if ((int32_t)uiCurrentInventoryIndex >=
      (int32_t)(GetNumberOfInventoryItemsOnCurrentMerc() - NUMBER_OF_INVENTORY_PERSONNEL)) {
    giPersonnelInventoryButtons[1]->uiFlags &= ~BUTTON_CLICKED_ON;
    DisableButton(giPersonnelInventoryButtons[1]);
  } else {
    EnableButton(giPersonnelInventoryButtons[1]);
  }
}

static int32_t GetNumberOfInventoryItemsOnCurrentMerc() {
  // in current team mode?..nope...move on
  if (!fCurrentTeamMode) return 0;

  uint32_t ubCount = 0;
  SOLDIERTYPE const &s = GetSoldierOfCurrentSlot();
  CFOR_EACH_SOLDIER_INV_SLOT(i, s) {
    OBJECTTYPE const &o = *i;
    if (o.ubNumberOfObjects != 0 && o.usItem != NOTHING) ubCount++;
  }

  return ubCount;
}

static void HandleInventoryCallBack(MOUSE_REGION *pRegion, int32_t iReason) {
  if (iReason & MSYS_CALLBACK_REASON_WHEEL_UP) {
    InventoryUp();
  } else if (iReason & MSYS_CALLBACK_REASON_WHEEL_DOWN) {
    InventoryDown();
  }
}

static MOUSE_REGION InventoryRegion;

static void HandleSliderBarClickCallback(MOUSE_REGION *pRegion, int32_t iReason);

static void CreateDestroyPersonnelInventoryScrollButtons() {
  static BOOLEAN fCreated = FALSE;

  if (gubPersonnelInfoState == PRSNL_INV && !fCreated) {
    giPersonnelInventoryButtons[0] = MakeButton(LAPTOPDIR "/personnel_inventory.sti", 1, 2,
                                                176 + 397, 2 + 200, InventoryUpButtonCallback);
    giPersonnelInventoryButtons[1] = MakeButton(LAPTOPDIR "/personnel_inventory.sti", 3, 4,
                                                176 + 397, 200 + 223, InventoryDownButtonCallback);

    MSYS_DefineRegion(
        &gMouseScrollPersonnelINV, X_OF_PERSONNEL_SCROLL_REGION, Y_OF_PERSONNEL_SCROLL_REGION,
        X_OF_PERSONNEL_SCROLL_REGION + X_SIZE_OF_PERSONNEL_SCROLL_REGION,
        Y_OF_PERSONNEL_SCROLL_REGION + Y_SIZE_OF_PERSONNEL_SCROLL_REGION, MSYS_PRIORITY_HIGHEST - 3,
        CURSOR_LAPTOP_SCREEN, MSYS_NO_CALLBACK, HandleSliderBarClickCallback);

    const uint16_t x = INVENTORY_BOX_X;
    const uint16_t y = INVENTORY_BOX_Y;
    const uint16_t w = INVENTORY_BOX_W;
    const uint16_t h = INVENTORY_BOX_H;
    MSYS_DefineRegion(&InventoryRegion, x, y, x + w, y + h, MSYS_PRIORITY_HIGHEST - 3,
                      CURSOR_LAPTOP_SCREEN, MSYS_NO_CALLBACK, HandleInventoryCallBack);

    fCreated = TRUE;
  } else if (fCreated && gubPersonnelInfoState != PRSNL_INV) {
    // destroy buttons
    RemoveButton(giPersonnelInventoryButtons[0]);
    RemoveButton(giPersonnelInventoryButtons[1]);

    MSYS_RemoveRegion(&gMouseScrollPersonnelINV);
    MSYS_RemoveRegion(&InventoryRegion);

    fCreated = FALSE;
  }
}

static void DisplayCostOfCurrentTeam();
static void DisplayStateOfPastTeamMembers();

static void DisplayPersonnelSummary() {
  // display number on team
  SetFontAttributes(FONT10ARIAL, PERS_TEXT_FONT_COLOR);

  if (fCurrentTeamMode) {
    mprintf(PERS_CURR_TEAM_X, PERS_CURR_TEAM_Y, L"%ls ( %d )", pPersonelTeamStrings[0],
            GetNumberOfMercsDeadOrAliveOnPlayersTeam());
    DisplayCostOfCurrentTeam();

    const wchar_t *const s = pPersonelTeamStrings[1];
    int16_t sX = 0;
    int16_t sY = 0;
    FindFontCenterCoordinates(PERS_CURR_TEAM_X, 0, 65, 0, s, FONT10ARIAL, &sX, &sY);
    MPrint(sX, PERS_DEPART_TEAM_Y, s);
  } else {
    const wchar_t *const s = pPersonelTeamStrings[0];
    int16_t sX = 0;
    int16_t sY = 0;
    FindFontCenterCoordinates(PERS_CURR_TEAM_X, 0, 65, 0, s, FONT10ARIAL, &sX, &sY);
    MPrint(sX, PERS_CURR_TEAM_Y, s);

    mprintf(PERS_CURR_TEAM_X, PERS_DEPART_TEAM_Y, L"%ls ( %d )", pPersonelTeamStrings[1],
            GetNumberOfPastMercsOnPlayersTeam());
    DisplayStateOfPastTeamMembers();
  }
}

static void DisplayCostOfCurrentTeam() {
  int32_t min_cost = 999999;
  int32_t max_cost = 0;
  int32_t sum_cost = 0;

  CFOR_EACH_PERSONNEL(s) {
    if (s->bLife <= 0) continue;

    // valid soldier, get cost
    int32_t cost;
    MERCPROFILESTRUCT const &p = GetProfile(s->ubProfile);
    switch (s->ubWhatKindOfMercAmI) {
      case MERC_TYPE__AIM_MERC:
        switch (s->bTypeOfLastContract) {
          case CONTRACT_EXTEND_2_WEEK:
            cost = p.uiBiWeeklySalary / 14;
            break;
          case CONTRACT_EXTEND_1_WEEK:
            cost = p.uiWeeklySalary / 7;
            break;
          default:
            cost = p.sSalary;
            break;
        }
        break;

      default:
        cost = p.sSalary;
        break;
    }

    if (cost > max_cost) max_cost = cost;
    if (cost < min_cost) min_cost = cost;
    sum_cost += cost;
  }

  if (min_cost == 999999) min_cost = 0;

  wchar_t sString[32];
  int16_t sX;
  int16_t sY;

  // daily cost
  MPrint(PERS_CURR_TEAM_COST_X, PERS_CURR_TEAM_COST_Y, pPersonelTeamStrings[2]);
  SPrintMoney(sString, sum_cost);
  FindFontRightCoordinates(PERS_CURR_TEAM_COST_X, 0, PERS_CURR_TEAM_WIDTH, 0, sString, PERS_FONT,
                           &sX, &sY);
  MPrint(sX, PERS_CURR_TEAM_COST_Y, sString);

  // highest cost
  MPrint(PERS_CURR_TEAM_COST_X, PERS_CURR_TEAM_HIGHEST_Y, pPersonelTeamStrings[3]);
  SPrintMoney(sString, max_cost);
  FindFontRightCoordinates(PERS_CURR_TEAM_COST_X, 0, PERS_CURR_TEAM_WIDTH, 0, sString, PERS_FONT,
                           &sX, &sY);
  MPrint(sX, PERS_CURR_TEAM_HIGHEST_Y, sString);

  // the lowest cost
  MPrint(PERS_CURR_TEAM_COST_X, PERS_CURR_TEAM_LOWEST_Y, pPersonelTeamStrings[4]);
  SPrintMoney(sString, min_cost);
  FindFontRightCoordinates(PERS_CURR_TEAM_COST_X, 0, PERS_CURR_TEAM_WIDTH, 0, sString, PERS_FONT,
                           &sX, &sY);
  MPrint(sX, PERS_CURR_TEAM_LOWEST_Y, sString);
}

static void DisplayTeamStats() {
  int16_t sX;
  int16_t sY;

  SetFontAttributes(FONT10ARIAL, PERS_TEXT_FONT_COLOR);

  // display headers
  // lowest
  FindFontCenterCoordinates(PERS_STAT_LOWEST_X, 0, PERS_STAT_LOWEST_WIDTH, 0,
                            pPersonnelCurrentTeamStatsStrings[0], FONT10ARIAL, &sX, &sY);
  MPrint(sX, PERS_STAT_AVG_Y, pPersonnelCurrentTeamStatsStrings[0]);
  // average
  FindFontCenterCoordinates(PERS_STAT_AVG_X, 0, PERS_STAT_AVG_WIDTH, 0,
                            pPersonnelCurrentTeamStatsStrings[1], FONT10ARIAL, &sX, &sY);
  MPrint(sX, PERS_STAT_AVG_Y, pPersonnelCurrentTeamStatsStrings[1]);
  // highest
  FindFontCenterCoordinates(PERS_STAT_HIGHEST_X, 0, PERS_STAT_LOWEST_WIDTH, 0,
                            pPersonnelCurrentTeamStatsStrings[2], FONT10ARIAL, &sX, &sY);
  MPrint(sX, PERS_STAT_AVG_Y, pPersonnelCurrentTeamStatsStrings[2]);

  for (int32_t stat = 0; stat < 11; stat++) {
    // even or odd?..color black or yellow?
    SetFontForeground(stat % 2 == 0 ? PERS_TEXT_FONT_ALTERNATE_COLOR : PERS_TEXT_FONT_COLOR);

    const int32_t y = PERS_STAT_AVG_Y + (stat + 1) * (GetFontHeight(FONT10ARIAL) + 3);

    // row header
    MPrint(PERS_STAT_LIST_X, y, pPersonnelTeamStatsStrings[stat]);

    const wchar_t *min_name = NULL;
    const wchar_t *max_name = NULL;
    int32_t min_val = 100;
    int32_t max_val = 0;
    int32_t sum_val = 0;
    int32_t count = 0;
    if (fCurrentTeamMode) {
      CFOR_EACH_PERSONNEL(s) {
        if (s->bLife <= 0 || AM_A_ROBOT(s)) continue;

        int32_t val;  // XXX HACK000E
        switch (stat) {
          case 0:
            val = s->bLifeMax;
            break;
          case 1:
            val = s->bAgility;
            break;
          case 2:
            val = s->bDexterity;
            break;
          case 3:
            val = s->bStrength;
            break;
          case 4:
            val = s->bLeadership;
            break;
          case 5:
            val = s->bWisdom;
            break;
          case 6:
            val = s->bExpLevel;
            break;
          case 7:
            val = s->bMarksmanship;
            break;
          case 8:
            val = s->bMechanical;
            break;
          case 9:
            val = s->bExplosive;
            break;
          case 10:
            val = s->bMedical;
            break;

          default:
            abort();  // HACK000E
        }
        if (min_val >= val) {
          min_name = s->name;
          min_val = val;
        }
        if (max_val <= val) {
          max_name = s->name;
          max_val = val;
        }
        sum_val += val;
        ++count;
      }
    } else {
      for (uint32_t CurrentList = 0; CurrentList < 3; ++CurrentList) {
        const int16_t *CurrentListValue;  // XXX HACK000E
        switch (CurrentList) {
          case 0:
            CurrentListValue = LaptopSaveInfo.ubDeadCharactersList;
            break;
          case 1:
            CurrentListValue = LaptopSaveInfo.ubLeftCharactersList;
            break;
          case 2:
            CurrentListValue = LaptopSaveInfo.ubOtherCharactersList;
            break;

          default:
            abort();  // HACK000E
        }

        for (uint32_t i = 0; i < 256; i++) {
          const int32_t id = CurrentListValue[i];
          if (id == -1) continue;

          int32_t val;  // XXX HACK000E
          MERCPROFILESTRUCT const &p = GetProfile(id);
          switch (stat) {
            case 0:
              val = p.bLifeMax;
              break;
            case 1:
              val = p.bAgility;
              break;
            case 2:
              val = p.bDexterity;
              break;
            case 3:
              val = p.bStrength;
              break;
            case 4:
              val = p.bLeadership;
              break;
            case 5:
              val = p.bWisdom;
              break;
            case 6:
              val = p.bExpLevel;
              break;
            case 7:
              val = p.bMarksmanship;
              break;
            case 8:
              val = p.bMechanical;
              break;
            case 9:
              val = p.bExplosive;
              break;
            case 10:
              val = p.bMedical;
              break;

            default:
              abort();  // HACK000E
          }
          if (min_val >= val) {
            min_name = p.zNickname;
            min_val = val;
          }
          if (max_val <= val) {
            max_name = p.zNickname;
            max_val = val;
          }
          sum_val += val;
          ++count;
        }
      }
    }

    if (count == 0) continue;

    MPrint(PERS_STAT_LOWEST_X, y, min_name);
    MPrint(PERS_STAT_HIGHEST_X, y, max_name);

    wchar_t val_str[32];

    swprintf(val_str, lengthof(val_str), L"%d", min_val);
    FindFontRightCoordinates(PERS_STAT_LOWEST_X, 0, PERS_STAT_LOWEST_WIDTH, 0, val_str, FONT10ARIAL,
                             &sX, &sY);
    MPrint(sX, y, val_str);

    swprintf(val_str, lengthof(val_str), L"%d", sum_val / count);
    FindFontCenterCoordinates(PERS_STAT_AVG_X, 0, PERS_STAT_AVG_WIDTH, 0, val_str, FONT10ARIAL, &sX,
                              &sY);
    MPrint(sX, y, val_str);

    swprintf(val_str, lengthof(val_str), L"%d", max_val);
    FindFontRightCoordinates(PERS_STAT_HIGHEST_X, 0, PERS_STAT_LOWEST_WIDTH, 0, val_str,
                             FONT10ARIAL, &sX, &sY);
    MPrint(sX, y, val_str);
  }
}

static int32_t GetNumberOfDeadOnPastTeam();
static int32_t GetNumberOfLeftOnPastTeam();
static int32_t GetNumberOfOtherOnPastTeam();

static int32_t GetNumberOfPastMercsOnPlayersTeam() {
  int32_t iPastNumberOfMercs = 0;
  // will run through the list of past mercs on the players team and return
  // their number

  iPastNumberOfMercs += GetNumberOfDeadOnPastTeam();
  iPastNumberOfMercs += GetNumberOfLeftOnPastTeam();
  iPastNumberOfMercs += GetNumberOfOtherOnPastTeam();

  return iPastNumberOfMercs;
}

static void InitPastCharactersList() {
  // inits the past characters list
  memset(&LaptopSaveInfo.ubDeadCharactersList, -1, sizeof(LaptopSaveInfo.ubDeadCharactersList));
  memset(&LaptopSaveInfo.ubLeftCharactersList, -1, sizeof(LaptopSaveInfo.ubLeftCharactersList));
  memset(&LaptopSaveInfo.ubOtherCharactersList, -1, sizeof(LaptopSaveInfo.ubOtherCharactersList));
}

static int32_t CountList(const int16_t *const list) {
  int32_t count = 0;
  for (const int16_t *i = list, *const end = list + 256; i != end; ++i) {
    if (*i != -1) ++count;
  }
  return count;
}

static int32_t GetNumberOfDeadOnPastTeam() {
  return CountList(LaptopSaveInfo.ubDeadCharactersList);
}

static int32_t GetNumberOfLeftOnPastTeam() {
  return CountList(LaptopSaveInfo.ubLeftCharactersList);
}

static int32_t GetNumberOfOtherOnPastTeam() {
  return CountList(LaptopSaveInfo.ubOtherCharactersList);
}

// diplays numbers fired, dead and other
static void DisplayStateOfPastTeamMembers() {
  int16_t sX, sY;
  wchar_t sString[32];

  // dead
  MPrint(PERS_CURR_TEAM_COST_X, PERS_CURR_TEAM_COST_Y, pPersonelTeamStrings[5]);
  swprintf(sString, lengthof(sString), L"%d", GetNumberOfDeadOnPastTeam());
  FindFontRightCoordinates(PERS_CURR_TEAM_COST_X, 0, PERS_DEPART_TEAM_WIDTH, 0, sString, PERS_FONT,
                           &sX, &sY);
  MPrint(sX, PERS_CURR_TEAM_COST_Y, sString);

  // fired
  MPrint(PERS_CURR_TEAM_COST_X, PERS_CURR_TEAM_HIGHEST_Y, pPersonelTeamStrings[6]);
  swprintf(sString, lengthof(sString), L"%d", GetNumberOfLeftOnPastTeam());
  FindFontRightCoordinates(PERS_CURR_TEAM_COST_X, 0, PERS_DEPART_TEAM_WIDTH, 0, sString, PERS_FONT,
                           &sX, &sY);
  MPrint(sX, PERS_CURR_TEAM_HIGHEST_Y, sString);

  // other
  MPrint(PERS_CURR_TEAM_COST_X, PERS_CURR_TEAM_LOWEST_Y, pPersonelTeamStrings[7]);
  swprintf(sString, lengthof(sString), L"%d", GetNumberOfOtherOnPastTeam());
  FindFontRightCoordinates(PERS_CURR_TEAM_COST_X, 0, PERS_DEPART_TEAM_WIDTH, 0, sString, PERS_FONT,
                           &sX, &sY);
  MPrint(sX, PERS_CURR_TEAM_LOWEST_Y, sString);
}

static void PersonnelCurrentTeamCallback(MOUSE_REGION *pRegion, int32_t iReason);
static void PersonnelDepartedTeamCallback(MOUSE_REGION *pRegion, int32_t iReason);

static void CreateDestroyCurrentDepartedMouseRegions(BOOLEAN create) {
  static BOOLEAN fCreated = FALSE;

  // will arbitrate the creation/deletion of mouse regions for current/past team
  // toggles

  if (create && !fCreated) {
    // not created, create
    uint16_t tlx = PERS_TOGGLE_CUR_DEPART_X;
    uint16_t tly = PERS_TOGGLE_CUR_Y;
    uint16_t brx = tlx + PERS_TOGGLE_CUR_DEPART_WIDTH;
    uint16_t bry = tly + PERS_TOGGLE_CUR_DEPART_HEIGHT;
    MSYS_DefineRegion(&gTogglePastCurrentTeam[0], tlx, tly, brx, bry, MSYS_PRIORITY_HIGHEST - 3,
                      CURSOR_LAPTOP_SCREEN, MSYS_NO_CALLBACK, PersonnelCurrentTeamCallback);

    tly = PERS_TOGGLE_DEPART_Y;
    bry = tly + PERS_TOGGLE_CUR_DEPART_HEIGHT;
    MSYS_DefineRegion(&gTogglePastCurrentTeam[1], tlx, tly, brx, bry, MSYS_PRIORITY_HIGHEST - 3,
                      CURSOR_LAPTOP_SCREEN, MSYS_NO_CALLBACK, PersonnelDepartedTeamCallback);

    fCreated = TRUE;
  } else if (!create && fCreated) {
    // created, get rid of

    MSYS_RemoveRegion(&gTogglePastCurrentTeam[0]);
    MSYS_RemoveRegion(&gTogglePastCurrentTeam[1]);
    fCreated = FALSE;
  }
}

static void PersonnelCurrentTeamCallback(MOUSE_REGION *pRegion, int32_t iReason) {
  if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    if (fCurrentTeamMode) return;
    fCurrentTeamMode = TRUE;

    SelectFirstDisplayedMerc();
    SetPersonnelButtonStates();
    fReDrawScreenFlag = TRUE;
  }
}

static void PersonnelDepartedTeamCallback(MOUSE_REGION *pRegion, int32_t iReason) {
  if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    if (!fCurrentTeamMode) return;
    fCurrentTeamMode = FALSE;

    SelectFirstDisplayedMerc();
    SetPersonnelButtonStates();

    // Switch the panel on the right to be the stat panel
    gubPersonnelInfoState = PRSNL_STATS;

    fReDrawScreenFlag = TRUE;
  }
}

static void DepartedDownCallBack(GUI_BUTTON *btn, int32_t reason);
static void DepartedUpCallBack(GUI_BUTTON *btn, int32_t reason);

static void CreateDestroyButtonsForDepartedTeamList(const BOOLEAN create) {
  // creates/ destroys the buttons for cdeparted team list
  static BOOLEAN fCreated = FALSE;

  if (create) {
    if (fCreated) return;
    // not created. create
    g_personnel.depart_up = MakeButton(LAPTOPDIR "/departuresbuttons.sti", 0, 2, PERS_DEPARTED_UP_X,
                                       PERS_DEPARTED_UP_Y, DepartedUpCallBack);
    g_personnel.depart_dn = MakeButton(LAPTOPDIR "/departuresbuttons.sti", 1, 3, PERS_DEPARTED_UP_X,
                                       PERS_DEPARTED_DOWN_Y, DepartedDownCallBack);
  } else {
    if (!fCreated) return;
    // created. destroy
    RemoveButton(g_personnel.depart_up);
    RemoveButton(g_personnel.depart_dn);
    fReDrawScreenFlag = TRUE;
  }
  fCreated = create;
}

static void DepartedUpCallBack(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    if (giCurrentUpperLeftPortraitNumber - PERSONNEL_PORTRAIT_NUMBER >= 0) {
      giCurrentUpperLeftPortraitNumber -= PERSONNEL_PORTRAIT_NUMBER;
      fReDrawScreenFlag = TRUE;
    }
  }
}

static void DepartedDownCallBack(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    int32_t const n_past = GetNumberOfPastMercsOnPlayersTeam();
    if (n_past - giCurrentUpperLeftPortraitNumber > PERSONNEL_PORTRAIT_NUMBER) {
      giCurrentUpperLeftPortraitNumber += PERSONNEL_PORTRAIT_NUMBER;
      if (iCurrentPersonSelectedId >= n_past - giCurrentUpperLeftPortraitNumber) {
        iCurrentPersonSelectedId = n_past - giCurrentUpperLeftPortraitNumber - 1;
      }
      fReDrawScreenFlag = TRUE;
    }
  }
}

static void DisplayPortraitOfPastMerc(int32_t iId, int32_t iCounter, BOOLEAN fDead);

/* Display past mercs portraits, starting at giCurrentUpperLeftPortraitNumber
 * and going up PERSONNEL_PORTRAIT_NUMBER mercs. Start at dead mercs, then
 * fired, then other */
static void DisplayPastMercsPortraits() {
  if (fCurrentTeamMode) return;  // Not time to display

  LaptopSaveInfoStruct const &l = LaptopSaveInfo;
  int32_t pos = -giCurrentUpperLeftPortraitNumber;

  FOR_EACH(int16_t const, i, l.ubDeadCharactersList) {
    if (*i == -1) continue;
    if (pos >= 0) DisplayPortraitOfPastMerc(*i, pos, TRUE);
    if (++pos == PERSONNEL_PORTRAIT_NUMBER) return;
  }

  FOR_EACH(int16_t const, i, l.ubLeftCharactersList) {
    if (*i == -1) continue;
    if (pos >= 0) DisplayPortraitOfPastMerc(*i, pos, FALSE);
    if (++pos == PERSONNEL_PORTRAIT_NUMBER) return;
  }

  FOR_EACH(int16_t const, i, l.ubOtherCharactersList) {
    if (*i == -1) continue;
    if (pos >= 0) DisplayPortraitOfPastMerc(*i, pos, FALSE);
    if (++pos == PERSONNEL_PORTRAIT_NUMBER) return;
  }
}

// returns ID of Merc in this slot
static PastMercInfo GetSelectedPastMercInfo() {
  int32_t slot = giCurrentUpperLeftPortraitNumber + iCurrentPersonSelectedId;
  Assert(slot < GetNumberOfPastMercsOnPlayersTeam());

  LaptopSaveInfoStruct const &l = LaptopSaveInfo;
  FOR_EACH(int16_t const, i, l.ubDeadCharactersList) {
    if (*i == -1 || slot-- != 0) continue;
    return PastMercInfo(&GetProfile(*i), DEPARTED_DEAD);
  }
  FOR_EACH(int16_t const, i, l.ubLeftCharactersList) {
    if (*i == -1 || slot-- != 0) continue;
    return PastMercInfo(&GetProfile(*i), DEPARTED_FIRED);
  }
  FOR_EACH(int16_t const, i, l.ubOtherCharactersList) {
    if (*i == -1 || slot-- != 0) continue;
    MERCPROFILESTRUCT &p = GetProfile(*i);
    int32_t const state = p.ubMiscFlags2 & PROFILE_MISC_FLAG2_MARRIED_TO_HICKS ? DEPARTED_MARRIED
                          : *i < BIFF ? DEPARTED_CONTRACT_EXPIRED
                                      : DEPARTED_QUIT;
    return PastMercInfo(&p, state);
  }
  return PastMercInfo(0, -1);
}

static void DisplayPortraitOfPastMerc(const int32_t iId, const int32_t iCounter,
                                      const BOOLEAN fDead) try {
  AutoSGPVObject guiFACE(LoadSmallPortrait(GetProfile(iId)));

  if (fDead) {
    guiFACE->pShades[0] = Create16BPPPaletteShaded(
        guiFACE->Palette(), DEAD_MERC_COLOR_RED, DEAD_MERC_COLOR_GREEN, DEAD_MERC_COLOR_BLUE, TRUE);
    // set the red pallete to the face
    guiFACE->CurrentShade(0);
  }

  const int32_t x =
      SMALL_PORTRAIT_START_X + iCounter % PERSONNEL_PORTRAIT_NUMBER_WIDTH * SMALL_PORT_WIDTH;
  const int32_t y =
      SMALL_PORTRAIT_START_Y + iCounter / PERSONNEL_PORTRAIT_NUMBER_WIDTH * SMALL_PORT_HEIGHT;
  BltVideoObject(FRAME_BUFFER, guiFACE, 0, x, y);
} catch (...) { /* XXX ignore */
}

static void DisplayDepartedCharStats(MERCPROFILESTRUCT const &p, int32_t const iState) {
  wchar_t sString[50];
  int16_t sX;
  int16_t sY;

  SetFontAttributes(FONT10ARIAL, PERS_TEXT_FONT_COLOR);

  int8_t const life = p.bLife;
  int8_t const cur = (iState == DEPARTED_DEAD ? 0 : life);
  swprintf(sString, lengthof(sString), L"%d/%d", cur, life);
  mprintf(pers_stat_x, pers_stat_y[0], L"%ls:", str_stat_health);
  FindFontRightCoordinates(pers_stat_x, 0, TEXT_BOX_WIDTH - 20, 0, sString, PERS_FONT, &sX, &sY);
  MPrint(sX, pers_stat_y[0], sString);

  PrintStat(p.bAgility, pers_stat_y[1], str_stat_agility);
  PrintStat(p.bDexterity, pers_stat_y[2], str_stat_dexterity);
  PrintStat(p.bStrength, pers_stat_y[3], str_stat_strength);
  PrintStat(p.bLeadership, pers_stat_y[4], str_stat_leadership);
  PrintStat(p.bWisdom, pers_stat_y[5], str_stat_wisdom);
  PrintStat(p.bExpLevel, pers_stat_y[6], str_stat_exp_level);
  PrintStat(p.bMarksmanship, pers_stat_y[7], str_stat_marksmanship);
  PrintStat(p.bMechanical, pers_stat_y[8], str_stat_mechanical);
  PrintStat(p.bExplosive, pers_stat_y[9], str_stat_explosive);
  PrintStat(p.bMedical, pers_stat_y[10], str_stat_medical);
  PrintStat(p.usKills, pers_stat_y[21], pPersonnelScreenStrings[PRSNL_TXT_KILLS]);
  PrintStat(p.usAssists, pers_stat_y[22], pPersonnelScreenStrings[PRSNL_TXT_ASSISTS]);

  // Shots/hits
  MPrint(pers_stat_x, pers_stat_y[23], pPersonnelScreenStrings[PRSNL_TXT_HIT_PERCENTAGE]);
  // check we have shot at least once
  uint32_t const fired = p.usShotsFired;
  uint32_t const hits = (fired > 0 ? 100 * p.usShotsHit / fired : 0);
  swprintf(sString, lengthof(sString), L"%d %%", hits);
  FindFontRightCoordinates(pers_stat_x, 0, TEXT_BOX_WIDTH - 20, 0, sString, PERS_FONT, &sX, &sY);
  MPrint(sX, pers_stat_y[23], sString);

  PrintStat(p.usBattlesFought, pers_stat_y[24], pPersonnelScreenStrings[PRSNL_TXT_BATTLES]);
  PrintStat(p.usTimesWounded, pers_stat_y[25], pPersonnelScreenStrings[PRSNL_TXT_TIMES_WOUNDED]);
}

static void EnableDisableDeparturesButtons() {
  // will enable or disable departures buttons based on upperleft picutre index
  // value
  if (fCurrentTeamMode || fNewMailFlag) {
    return;
  }

  // disable both buttons
  DisableButton(g_personnel.depart_up);
  DisableButton(g_personnel.depart_dn);

  if (giCurrentUpperLeftPortraitNumber != 0) {
    // enable up button
    EnableButton(g_personnel.depart_up);
  }
  if (GetNumberOfPastMercsOnPlayersTeam() - giCurrentUpperLeftPortraitNumber >
      PERSONNEL_PORTRAIT_NUMBER) {
    // enable down button
    EnableButton(g_personnel.depart_dn);
  }
}

static void DisplayDepartedCharName(MERCPROFILESTRUCT const &p, const int32_t iState) {
  // get merc's nickName, assignment, and sector location info
  int16_t sX, sY;

  SetFontAttributes(CHAR_NAME_FONT, PERS_TEXT_FONT_COLOR);

  wchar_t const *const name = p.zNickname;
  FindFontCenterCoordinates(CHAR_NAME_LOC_X, 0, CHAR_NAME_LOC_WIDTH, 0, name, CHAR_NAME_FONT, &sX,
                            &sY);
  MPrint(sX, CHAR_NAME_Y, name);

  const wchar_t *const state_txt = pPersonnelDepartedStateStrings[iState];
  FindFontCenterCoordinates(CHAR_NAME_LOC_X, 0, CHAR_NAME_LOC_WIDTH, 0, state_txt, CHAR_NAME_FONT,
                            &sX, &sY);
  MPrint(sX, CHAR_LOC_Y, state_txt);
}

static void DisplayPersonnelTextOnTitleBar() {
  SetFontAttributes(FONT14ARIAL, FONT_WHITE);
  MPrint(PERS_TITLE_X, PERS_TITLE_Y, pPersTitleText);
}

// display box around currently selected merc
static void DisplayHighLightBox(int32_t const sel_id) {
  // will display highlight box around selected merc
  const int32_t x =
      SMALL_PORTRAIT_START_X + sel_id % PERSONNEL_PORTRAIT_NUMBER_WIDTH * SMALL_PORT_WIDTH - 2;
  const int32_t y =
      SMALL_PORTRAIT_START_Y + sel_id / PERSONNEL_PORTRAIT_NUMBER_WIDTH * SMALL_PORT_HEIGHT - 3;
  BltVideoObjectOnce(FRAME_BUFFER, LAPTOPDIR "/picborde.sti", 0, x, y);
}

// add to dead list
void AddCharacterToDeadList(SOLDIERTYPE *pSoldier) {
  for (int32_t i = 0; i < 256; i++) {
    if (LaptopSaveInfo.ubDeadCharactersList[i] == -1) {
      // valid slot, merc not found yet, inset here
      LaptopSaveInfo.ubDeadCharactersList[i] = pSoldier->ubProfile;
      return;
    }

    // are they already in the list?
    if (LaptopSaveInfo.ubDeadCharactersList[i] == pSoldier->ubProfile) {
      return;
    }
  }
}

void AddCharacterToFiredList(SOLDIERTYPE *pSoldier) {
  for (int32_t i = 0; i < 256; i++) {
    if (LaptopSaveInfo.ubLeftCharactersList[i] == -1) {
      // valid slot, merc not found yet, inset here
      LaptopSaveInfo.ubLeftCharactersList[i] = pSoldier->ubProfile;
      return;
    }

    // are they already in the list?
    if (LaptopSaveInfo.ubLeftCharactersList[i] == pSoldier->ubProfile) {
      return;
    }
  }
}

void AddCharacterToOtherList(SOLDIERTYPE *pSoldier) {
  for (int32_t i = 0; i < 256; i++) {
    if (LaptopSaveInfo.ubOtherCharactersList[i] == -1) {
      // valid slot, merc not found yet, inset here
      LaptopSaveInfo.ubOtherCharactersList[i] = pSoldier->ubProfile;
      return;
    }

    // are they already in the list?
    if (LaptopSaveInfo.ubOtherCharactersList[i] == pSoldier->ubProfile) {
      return;
    }
  }
}

// If you have hired a merc before, then the they left for whatever reason, and
// now you are hiring them again, we must get rid of them from the departed
// section in the personnel screen.  (wouldnt make sense for them
// to be on your team list, and departed list)
BOOLEAN RemoveNewlyHiredMercFromPersonnelDepartedList(uint8_t ubProfile) {
  for (int32_t i = 0; i < 256; i++) {
    // are they already in the Dead list?
    if (LaptopSaveInfo.ubDeadCharactersList[i] == ubProfile) {
      // Reset the fact that they were once hired
      LaptopSaveInfo.ubDeadCharactersList[i] = -1;
      return TRUE;
    }

    // are they already in the other list?
    if (LaptopSaveInfo.ubLeftCharactersList[i] == ubProfile) {
      // Reset the fact that they were once hired
      LaptopSaveInfo.ubLeftCharactersList[i] = -1;
      return TRUE;
    }

    // are they already in the list?
    if (LaptopSaveInfo.ubOtherCharactersList[i] == ubProfile) {
      // Reset the fact that they were once hired
      LaptopSaveInfo.ubOtherCharactersList[i] = -1;
      return TRUE;
    }
  }

  return FALSE;
}

// Select the first displayed merc, if there is any
static void SelectFirstDisplayedMerc() {
  // set current soldier
  if (fCurrentTeamMode) {
    CFOR_EACH_PERSONNEL(s) {
      iCurrentPersonSelectedId = 0;
      return;
    }
    iCurrentPersonSelectedId = -1;
  } else {
    iCurrentPersonSelectedId = GetNumberOfPastMercsOnPlayersTeam() > 0 ? 0 : -1;
  }
}

static SOLDIERTYPE const &GetSoldierOfCurrentSlot() {
  Assert(fCurrentTeamMode);

  int32_t slot = iCurrentPersonSelectedId;
  CFOR_EACH_PERSONNEL(s) {
    if (slot-- == 0) return *s;
  }

  throw std::logic_error("nobody selected");
}

static void RenderAtmPanel() try {
  // just show basic panel
  // bounding
  AutoSGPVObject uiBox(AddVideoObjectFromFile(LAPTOPDIR "/atmbuttons.sti"));
  BltVideoObject(FRAME_BUFFER, uiBox, 0, ATM_UL_X, ATM_UL_Y);
  BltVideoObject(FRAME_BUFFER, uiBox, 1, ATM_UL_X + 1, ATM_UL_Y + 18);
} catch (...) { /* XXX ignore */
}

static void MakeButton(uint32_t idx, int16_t y, GUI_CALLBACK click, const wchar_t *text) {
  BUTTON_PICS *const img = LoadButtonImage(LAPTOPDIR "/atmbuttons.sti", 2, 3);
  giPersonnelATMStartButtonImage[idx] = img;
  GUIButtonRef const btn = QuickCreateButtonNoMove(img, 519, y, MSYS_PRIORITY_HIGHEST - 1, click);
  giPersonnelATMStartButton[idx] = btn;
  btn->SpecifyGeneralTextAttributes(text, PERS_FONT, FONT_BLACK, FONT_BLACK);
  btn->SetCursor(CURSOR_LAPTOP_SCREEN);
}

static void EmployementInfoButtonCallback(GUI_BUTTON *btn, int32_t reason);
static void PersonnelINVStartButtonCallback(GUI_BUTTON *btn, int32_t reason);
static void PersonnelStatStartButtonCallback(GUI_BUTTON *btn, int32_t reason);

static void CreateDestroyStartATMButton(const BOOLEAN create) {
  static BOOLEAN fCreated = FALSE;
  // create/destroy atm start button as needed

  if (!fCreated && create) {
    // not created, must create
    MakeButton(PERSONNEL_STAT_BTN, 80, PersonnelStatStartButtonCallback, gsAtmStartButtonText[0]);
    MakeButton(PERSONNEL_EMPLOYMENT_BTN, 110, EmployementInfoButtonCallback,
               gsAtmStartButtonText[2]);
    MakeButton(PERSONNEL_INV_BTN, 140, PersonnelINVStartButtonCallback, gsAtmStartButtonText[1]);

    fCreated = TRUE;
  } else if (fCreated && !create) {
    // stop showing
    RemoveButton(giPersonnelATMStartButton[PERSONNEL_STAT_BTN]);
    UnloadButtonImage(giPersonnelATMStartButtonImage[PERSONNEL_STAT_BTN]);
    RemoveButton(giPersonnelATMStartButton[PERSONNEL_EMPLOYMENT_BTN]);
    UnloadButtonImage(giPersonnelATMStartButtonImage[PERSONNEL_EMPLOYMENT_BTN]);
    RemoveButton(giPersonnelATMStartButton[PERSONNEL_INV_BTN]);
    UnloadButtonImage(giPersonnelATMStartButtonImage[PERSONNEL_INV_BTN]);

    fCreated = FALSE;
  }
}

static void FindPositionOfPersInvSlider() {
  const int32_t item_count = GetNumberOfInventoryItemsOnCurrentMerc();
  const int32_t scroll_count = item_count - NUMBER_OF_INVENTORY_PERSONNEL;
  guiSliderPosition = (Y_SIZE_OF_PERSONNEL_SCROLL_REGION - SIZE_OF_PERSONNEL_CURSOR) *
                      uiCurrentInventoryIndex / scroll_count;
}

static void HandleSliderBarClickCallback(MOUSE_REGION *pRegion, int32_t iReason) {
  if (iReason & MSYS_CALLBACK_REASON_LBUTTON_DWN || iReason & MSYS_CALLBACK_REASON_LBUTTON_REPEAT) {
    const int32_t item_count = GetNumberOfInventoryItemsOnCurrentMerc();
    if (item_count <= NUMBER_OF_INVENTORY_PERSONNEL) return;

    const int32_t scroll_count = item_count - NUMBER_OF_INVENTORY_PERSONNEL;

    // get the actual item position
    const int16_t new_item_idx = scroll_count * pRegion->RelativeYPos /
                                 (Y_SIZE_OF_PERSONNEL_SCROLL_REGION - SIZE_OF_PERSONNEL_CURSOR);

    if (uiCurrentInventoryIndex != new_item_idx) {
      // get slider position
      guiSliderPosition = (Y_SIZE_OF_PERSONNEL_SCROLL_REGION - SIZE_OF_PERSONNEL_CURSOR) *
                          new_item_idx / scroll_count;

      // set current inventory value
      uiCurrentInventoryIndex = new_item_idx;

      // force update
      fReDrawScreenFlag = TRUE;
    }
  } else if (iReason & MSYS_CALLBACK_REASON_WHEEL_UP) {
    InventoryUp();
  } else if (iReason & MSYS_CALLBACK_REASON_WHEEL_DOWN) {
    InventoryDown();
  }
}

static void RenderSliderBarForPersonnelInventory() {
  // render slider bar for personnel
  BltVideoObject(FRAME_BUFFER, guiPersonnelInventory, 5, X_OF_PERSONNEL_SCROLL_REGION,
                 guiSliderPosition + Y_OF_PERSONNEL_SCROLL_REGION);
}

static void PersonnelINVStartButtonCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
    fReDrawScreenFlag = TRUE;
    btn->uiFlags |= BUTTON_CLICKED_ON;
    giPersonnelATMStartButton[PERSONNEL_STAT_BTN]->uiFlags &= ~BUTTON_CLICKED_ON;
    giPersonnelATMStartButton[PERSONNEL_EMPLOYMENT_BTN]->uiFlags &= ~BUTTON_CLICKED_ON;
    gubPersonnelInfoState = PRSNL_INV;
  }
}

static void PersonnelStatStartButtonCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
    fReDrawScreenFlag = TRUE;
    btn->uiFlags |= BUTTON_CLICKED_ON;
    giPersonnelATMStartButton[PERSONNEL_EMPLOYMENT_BTN]->uiFlags &= ~BUTTON_CLICKED_ON;
    giPersonnelATMStartButton[PERSONNEL_INV_BTN]->uiFlags &= ~BUTTON_CLICKED_ON;
    gubPersonnelInfoState = PRSNL_STATS;
  }
}

static void EmployementInfoButtonCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
    fReDrawScreenFlag = TRUE;
    btn->uiFlags |= BUTTON_CLICKED_ON;
    giPersonnelATMStartButton[PERSONNEL_INV_BTN]->uiFlags &= ~BUTTON_CLICKED_ON;
    giPersonnelATMStartButton[PERSONNEL_STAT_BTN]->uiFlags &= ~BUTTON_CLICKED_ON;
    gubPersonnelInfoState = PRSNL_EMPLOYMENT;
  }
}

// get the total amt of money on this guy
static int32_t GetFundsOnMerc(SOLDIERTYPE const &s) {
  int32_t iCurrentAmount = 0;
  // run through mercs pockets, if any money in them, add to total

  // run through grunts pockets and count all the spare change
  CFOR_EACH_SOLDIER_INV_SLOT(i, s) {
    if (Item[i->usItem].usItemClass == IC_MONEY) {
      iCurrentAmount += i->uiMoneyAmount;
    }
  }

  return iCurrentAmount;
}

// check if current guy can have atm
static void UpDateStateOfStartButton() {
  if (gubPersonnelInfoState == PRSNL_INV) {
    giPersonnelATMStartButton[PERSONNEL_INV_BTN]->uiFlags |= BUTTON_CLICKED_ON;
    giPersonnelATMStartButton[PERSONNEL_STAT_BTN]->uiFlags &= ~BUTTON_CLICKED_ON;
    giPersonnelATMStartButton[PERSONNEL_EMPLOYMENT_BTN]->uiFlags &= ~BUTTON_CLICKED_ON;
  } else if (gubPersonnelInfoState == PRSNL_STATS) {
    giPersonnelATMStartButton[PERSONNEL_INV_BTN]->uiFlags &= ~BUTTON_CLICKED_ON;
    giPersonnelATMStartButton[PERSONNEL_STAT_BTN]->uiFlags |= BUTTON_CLICKED_ON;
    giPersonnelATMStartButton[PERSONNEL_EMPLOYMENT_BTN]->uiFlags &= ~BUTTON_CLICKED_ON;
  } else {
    giPersonnelATMStartButton[PERSONNEL_STAT_BTN]->uiFlags &= ~BUTTON_CLICKED_ON;
    giPersonnelATMStartButton[PERSONNEL_INV_BTN]->uiFlags &= ~BUTTON_CLICKED_ON;
    giPersonnelATMStartButton[PERSONNEL_EMPLOYMENT_BTN]->uiFlags |= BUTTON_CLICKED_ON;
  }

  // if in current mercs and the currently selected guy is valid, enable button,
  // else disable it
  if (fCurrentTeamMode) {
    // is the current guy valid
    if (GetNumberOfMercsDeadOrAliveOnPlayersTeam() > 0) {
      EnableButton(giPersonnelATMStartButton[PERSONNEL_STAT_BTN]);
      EnableButton(giPersonnelATMStartButton[PERSONNEL_INV_BTN]);
      EnableButton(giPersonnelATMStartButton[PERSONNEL_EMPLOYMENT_BTN]);

      if (GetSoldierOfCurrentSlot().bAssignment == ASSIGNMENT_POW) {
        DisableButton(giPersonnelATMStartButton[PERSONNEL_INV_BTN]);

        if (gubPersonnelInfoState == PRSNL_INV) {
          gubPersonnelInfoState = PRSNL_STATS;
          fPausedReDrawScreenFlag = TRUE;
        }
      }
    } else {
      // not valid, disable
      DisableButton(giPersonnelATMStartButton[PERSONNEL_STAT_BTN]);
      DisableButton(giPersonnelATMStartButton[PERSONNEL_INV_BTN]);
      DisableButton(giPersonnelATMStartButton[PERSONNEL_EMPLOYMENT_BTN]);
    }
  } else {
    // disable button
    EnableButton(giPersonnelATMStartButton[PERSONNEL_STAT_BTN]);
    DisableButton(giPersonnelATMStartButton[PERSONNEL_INV_BTN]);
    DisableButton(giPersonnelATMStartButton[PERSONNEL_EMPLOYMENT_BTN]);
  }
}

static void DisplayAmountOnChar(SOLDIERTYPE const &s) {
  // will display the amount that the merc is carrying on him or herself
  wchar_t sString[64];
  SPrintMoney(sString, GetFundsOnMerc(s));

  SetFontAttributes(ATM_FONT, FONT_WHITE);

  int16_t sX;
  int16_t sY;
  FindFontRightCoordinates(ATM_DISPLAY_X, ATM_DISPLAY_Y, ATM_DISPLAY_WIDTH, ATM_DISPLAY_HEIGHT,
                           sString, ATM_FONT, &sX, &sY);
  MPrint(sX, sY, sString);
}

static void HandlePersonnelKeyboard() {
  InputAtom InputEvent;
  while (DequeueEvent(&InputEvent)) {
    HandleKeyBoardShortCutsForLapTop(InputEvent.usEvent, InputEvent.usParam, InputEvent.usKeyState);
  }
}

static void DisplayEmploymentinformation(SOLDIERTYPE const &s) {
  wchar_t sString[50];
  wchar_t sStringA[50];
  int16_t sX, sY;

  MERCPROFILESTRUCT const &p = GetProfile(s.ubProfile);

  // display the stats for a char
  for (int32_t i = 0; i < MAX_STATS; i++) {
    switch (i) {
      case 0:  // Remaining Contract:
      {
        if (s.ubWhatKindOfMercAmI == MERC_TYPE__AIM_MERC || s.ubProfile == SLAY) {
          const uint32_t uiMinutesInDay = 24 * 60;
          int32_t iTimeLeftOnContract = s.iEndofContractTime - GetWorldTotalMin();
          if (iTimeLeftOnContract < 0) iTimeLeftOnContract = 0;

          // if the merc is in transit
          if (s.bAssignment == IN_TRANSIT) {
            // and if the ttime left on the cotract is greater then the contract
            // time
            if (iTimeLeftOnContract > (int32_t)(s.iTotalContractLength * uiMinutesInDay)) {
              iTimeLeftOnContract = (s.iTotalContractLength * uiMinutesInDay);
            }
          }
          // if there is going to be a both days and hours left on the contract
          const int32_t days = iTimeLeftOnContract / uiMinutesInDay;
          const int32_t hours = iTimeLeftOnContract % uiMinutesInDay / 60;
          if (days > 0) {
            swprintf(sString, lengthof(sString), L"%d%ls %d%ls / %d%ls", days,
                     gpStrategicString[STR_PB_DAYS_ABBREVIATION], hours,
                     gpStrategicString[STR_PB_HOURS_ABBREVIATION], s.iTotalContractLength,
                     gpStrategicString[STR_PB_DAYS_ABBREVIATION]);
          } else  // else there is under a day left
          {
            // DEF: removed 2/7/99
            swprintf(sString, lengthof(sString), L"%d%ls / %d%ls", hours,
                     gpStrategicString[STR_PB_HOURS_ABBREVIATION], s.iTotalContractLength,
                     gpStrategicString[STR_PB_DAYS_ABBREVIATION]);
          }
        } else if (s.ubWhatKindOfMercAmI == MERC_TYPE__MERC) {
          wcscpy(sString, gpStrategicString[STR_PB_NOTAPPLICABLE_ABBREVIATION]);
        } else {
          wcscpy(sString, gpStrategicString[STR_PB_NOTAPPLICABLE_ABBREVIATION]);
        }

        MPrint(pers_stat_x, pers_stat_y[i], pPersonnelScreenStrings[PRSNL_TXT_CURRENT_CONTRACT]);
        FindFontRightCoordinates(pers_stat_data_x, 0, TEXT_BOX_WIDTH - 20, 0, sString, PERS_FONT,
                                 &sX, &sY);
        MPrint(sX, pers_stat_y[i], sString);
      } break;

      case 1:  // total contract time served
        MPrint(pers_stat_x, pers_stat_y[i], pPersonnelScreenStrings[PRSNL_TXT_TOTAL_SERVICE]);
        //./DEF 2/4/99: total service days used to be calced as 'days -1'
        swprintf(sString, lengthof(sString), L"%d %ls", p.usTotalDaysServed,
                 gpStrategicString[STR_PB_DAYS_ABBREVIATION]);
        FindFontRightCoordinates(pers_stat_data_x, 0, TEXT_BOX_WIDTH - 20, 0, sString, PERS_FONT,
                                 &sX, &sY);
        MPrint(sX, pers_stat_y[i], sString);
        break;

      case 3:  // cost (PRSNL_TXT_TOTAL_COST)
      {
        SPrintMoney(sString, p.uiTotalCostToDate);
        FindFontRightCoordinates(pers_stat_data_x, 0, TEXT_BOX_WIDTH - 20, 0, sString, PERS_FONT,
                                 &sX, &sY);
        MPrint(pers_stat_x, pers_stat_y[i], pPersonnelScreenStrings[PRSNL_TXT_TOTAL_COST]);

        // print contract cost
        MPrint(sX, pers_stat_y[i], sString);

        int32_t salary;
        switch (s.ubWhatKindOfMercAmI) {
          case MERC_TYPE__AIM_MERC:
            switch (s.bTypeOfLastContract) {
              case CONTRACT_EXTEND_2_WEEK:
                salary = p.uiBiWeeklySalary / 14;
                break;
              case CONTRACT_EXTEND_1_WEEK:
                salary = p.uiWeeklySalary / 7;
                break;
              default:
                salary = p.sSalary;
                break;
            }
            break;

          default:
            salary = p.sSalary;
            break;
        }

        SPrintMoney(sStringA, salary);
        FindFontRightCoordinates(pers_stat_data_x, 0, TEXT_BOX_WIDTH - 20, 0, sStringA, PERS_FONT,
                                 &sX, &sY);

        i++;

        // now print daily rate
        MPrint(sX, pers_stat_y[i + 1], sStringA);
        MPrint(pers_stat_x, pers_stat_y[i + 1], pPersonnelScreenStrings[PRSNL_TXT_DAILY_COST]);
        break;
      }

      case 5:  // medical deposit
        // if its a merc merc, display the salary oweing
        if (s.ubWhatKindOfMercAmI == MERC_TYPE__MERC) {
          MPrint(pers_stat_x, pers_stat_y[i - 1], pPersonnelScreenStrings[PRSNL_TXT_UNPAID_AMOUNT]);
          SPrintMoney(sString, p.sSalary * p.iMercMercContractLength);
        } else {
          MPrint(pers_stat_x, pers_stat_y[i - 1], pPersonnelScreenStrings[PRSNL_TXT_MED_DEPOSIT]);
          SPrintMoney(sString, p.sMedicalDepositAmount);
        }
        FindFontRightCoordinates(pers_stat_data_x, 0, TEXT_BOX_WIDTH - 20, 0, sString, PERS_FONT,
                                 &sX, &sY);
        MPrint(sX, pers_stat_y[i - 1], sString);
        break;
    }
  }
}
