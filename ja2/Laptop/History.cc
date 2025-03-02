// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Laptop/History.h"

#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "Directories.h"
#include "Laptop/Laptop.h"
#include "Laptop/LaptopSave.h"
#include "Macro.h"
#include "SGP/ButtonSystem.h"
#include "SGP/Debug.h"
#include "SGP/FileMan.h"
#include "SGP/Font.h"
#include "SGP/LoadSaveData.h"
#include "SGP/MemMan.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "Strategic/GameClock.h"
#include "Strategic/QuestText.h"
#include "Strategic/Quests.h"
#include "Strategic/StrategicMap.h"
#include "Tactical/SoldierControl.h"
#include "Tactical/SoldierProfile.h"
#include "TileEngine/RenderDirty.h"
#include "Utils/Cursors.h"
#include "Utils/EncryptedFile.h"
#include "Utils/FontControl.h"
#include "Utils/Message.h"
#include "Utils/Text.h"
#include "Utils/WordWrap.h"

#define HISTORY_QUEST_TEXT_SIZE 80

struct HistoryUnit {
  uint8_t ubCode;        // the code index in the finance code table
  uint8_t ubSecondCode;  // secondary code
  uint32_t uiDate;       // time in the world in global time
  int16_t sSectorX;      // sector X this took place in
  int16_t sSectorY;      // sector Y this took place in
  int8_t bSectorZ;
  HistoryUnit *Next;  // next unit in the list
};

#define TOP_X 0 + LAPTOP_SCREEN_UL_X
#define TOP_Y LAPTOP_SCREEN_UL_Y
#define BOX_HEIGHT 14
#define TOP_DIVLINE_Y 101
#define TITLE_X 140
#define TITLE_Y 33
#define PAGE_SIZE 22
#define RECORD_Y TOP_DIVLINE_Y
#define RECORD_HISTORY_WIDTH 200
#define PAGE_NUMBER_X TOP_X + 20
#define PAGE_NUMBER_Y TOP_Y + 33
#define HISTORY_DATE_X PAGE_NUMBER_X + 85
#define HISTORY_DATE_Y PAGE_NUMBER_Y
#define RECORD_LOCATION_WIDTH 142  // 95

#define HISTORY_HEADER_FONT FONT14ARIAL
#define HISTORY_TEXT_FONT FONT12ARIAL
#define RECORD_DATE_X TOP_X + 10
#define RECORD_DATE_WIDTH 31  // 68
#define RECORD_HEADER_Y 90

#define NUM_RECORDS_PER_PAGE PAGE_SIZE
#define SIZE_OF_HISTORY_FILE_RECORD                                                             \
  (sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t) + \
   sizeof(uint8_t) + sizeof(uint8_t))

// button positions
#define NEXT_BTN_X 577
#define PREV_BTN_X 553
#define BTN_Y 53

// graphics handles
static SGPVObject *guiTITLE;
static SGPVObject *guiTOP;
static SGPVObject *guiLONGLINE;
static SGPVObject *guiSHADELINE;

enum {
  PREV_PAGE_BUTTON = 0,
  NEXT_PAGE_BUTTON,
};

// the page flipping buttons
static GUIButtonRef giHistoryButton[2];

static MOUSE_REGION g_scroll_region;

static BOOLEAN fInHistoryMode = FALSE;
// current page displayed
static int32_t iCurrentHistoryPage = 1;

// the History record list
static HistoryUnit *pHistoryListHead = NULL;

void ClearHistoryList();

static void AppendHistoryToEndOfFile();
static BOOLEAN LoadInHistoryRecords(const uint32_t uiPage);
static void ProcessAndEnterAHistoryRecord(uint8_t ubCode, uint32_t uiDate, uint8_t ubSecondCode,
                                          int16_t sSectorX, int16_t sSectorY, int8_t bSectorZ);

void AddHistoryToPlayersLog(uint8_t ubCode, uint8_t ubSecondCode, uint32_t uiDate, int16_t sSectorX,
                            int16_t sSectorY) {
  ClearHistoryList();

  ProcessAndEnterAHistoryRecord(ubCode, uiDate, ubSecondCode, sSectorX, sSectorY, 0);
  ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, pMessageStrings[MSG_HISTORY_UPDATED]);

  AppendHistoryToEndOfFile();

  // if in history mode, reload current page
  if (fInHistoryMode) LoadInHistoryRecords(iCurrentHistoryPage);
}

void GameInitHistory() { FileDelete(HISTORY_DATA_FILE); }

static void CreateHistoryButtons();
static void LoadHistory();
static void SetHistoryButtonStates();

void EnterHistory() {
  // load the graphics
  LoadHistory();

  // create History buttons
  CreateHistoryButtons();

  // reset current to first page
  iCurrentHistoryPage = LaptopSaveInfo.iCurrentHistoryPage;
  if (iCurrentHistoryPage <= 0) iCurrentHistoryPage = 1;

  LoadInHistoryRecords(iCurrentHistoryPage);

  // render hbackground
  RenderHistory();

  // set the fact we are in the history viewer
  fInHistoryMode = TRUE;

  // build Historys list
  // OpenAndReadHistoryFile( );

  // force redraw of the entire screen
  // fReDrawScreenFlag=TRUE;

  // set inital states
  SetHistoryButtonStates();
}

static void DestroyHistoryButtons();
static void RemoveHistory();

void ExitHistory() {
  LaptopSaveInfo.iCurrentHistoryPage = iCurrentHistoryPage;

  // not in History system anymore
  fInHistoryMode = FALSE;

  // delete graphics
  RemoveHistory();

  // delete buttons
  DestroyHistoryButtons();

  ClearHistoryList();
}

static void DrawAPageofHistoryRecords();
static void RenderHistoryBackGround();

void RenderHistory() {
  // render the background to the display
  RenderHistoryBackGround();

  // render the currentpage of records
  DrawAPageofHistoryRecords();

  // title bar icon
  BlitTitleBarIcons();
}

static void LoadHistory() {
  // load History video objects into memory

  // title bar
  guiTITLE = AddVideoObjectFromFile(LAPTOPDIR "/programtitlebar.sti");

  // top portion of the screen background
  guiTOP = AddVideoObjectFromFile(LAPTOPDIR "/historywindow.sti");

  // shaded line
  guiSHADELINE = AddVideoObjectFromFile(LAPTOPDIR "/historylines.sti");

  // black divider line - long ( 480 length)
  guiLONGLINE = AddVideoObjectFromFile(LAPTOPDIR "/divisionline480.sti");
}

static void RemoveHistory() {
  // delete history video objects from memory
  DeleteVideoObject(guiLONGLINE);
  DeleteVideoObject(guiTOP);
  DeleteVideoObject(guiTITLE);
  DeleteVideoObject(guiSHADELINE);
}

static void RenderHistoryBackGround() {
  // render generic background for history system
  BltVideoObject(FRAME_BUFFER, guiTITLE, 0, TOP_X, TOP_Y - 2);
  BltVideoObject(FRAME_BUFFER, guiTOP, 0, TOP_X, TOP_Y + 22);
}

static void DrawHistoryTitleText() {
  // draw the pages title
  SetFontAttributes(HISTORY_HEADER_FONT, FONT_WHITE);
  MPrint(TITLE_X, TITLE_Y, pHistoryTitle);
}

static void LoadNextHistoryPage();
static void LoadPreviousHistoryPage();

static void ScrollRegionCallback(MOUSE_REGION *const, int32_t const reason) {
  if (reason & MSYS_CALLBACK_REASON_WHEEL_UP) {
    LoadPreviousHistoryPage();
  } else if (reason & MSYS_CALLBACK_REASON_WHEEL_DOWN) {
    LoadNextHistoryPage();
  }
}

static void BtnHistoryDisplayNextPageCallBack(GUI_BUTTON *btn, int32_t reason);
static void BtnHistoryDisplayPrevPageCallBack(GUI_BUTTON *btn, int32_t reason);

static void CreateHistoryButtons() {
  // the prev/next page buttons
  giHistoryButton[PREV_PAGE_BUTTON] =
      QuickCreateButtonImg(LAPTOPDIR "/arrows.sti", 0, 1, PREV_BTN_X, BTN_Y,
                           MSYS_PRIORITY_HIGHEST - 1, BtnHistoryDisplayPrevPageCallBack);
  giHistoryButton[NEXT_PAGE_BUTTON] =
      QuickCreateButtonImg(LAPTOPDIR "/arrows.sti", 6, 7, NEXT_BTN_X, BTN_Y,
                           MSYS_PRIORITY_HIGHEST - 1, BtnHistoryDisplayNextPageCallBack);

  // set buttons
  giHistoryButton[0]->SetCursor(CURSOR_LAPTOP_SCREEN);
  giHistoryButton[1]->SetCursor(CURSOR_LAPTOP_SCREEN);

  uint16_t const x = TOP_X + 8;
  uint16_t const y = TOP_Y + 53;
  uint16_t const w = 482;
  uint16_t const h = 354;
  MSYS_DefineRegion(&g_scroll_region, x, y, x + w, y + h, MSYS_PRIORITY_HIGH, MSYS_NO_CURSOR,
                    MSYS_NO_CALLBACK, ScrollRegionCallback);
}

static void DestroyHistoryButtons() {
  // remove History buttons and images from memory
  MSYS_RemoveRegion(&g_scroll_region);
  // next page button
  RemoveButton(giHistoryButton[1]);
  // prev page button
  RemoveButton(giHistoryButton[0]);
}

static void BtnHistoryDisplayPrevPageCallBack(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
    fReDrawScreenFlag = TRUE;
  }

  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    LoadPreviousHistoryPage();
  }
}

static void BtnHistoryDisplayNextPageCallBack(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
    fReDrawScreenFlag = TRUE;
  }

  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    LoadNextHistoryPage();
  }
}

static void ProcessAndEnterAHistoryRecord(const uint8_t ubCode, const uint32_t uiDate,
                                          const uint8_t ubSecondCode, const int16_t sSectorX,
                                          const int16_t sSectorY, const int8_t bSectorZ) {
  HistoryUnit *const h = MALLOC(HistoryUnit);
  h->Next = NULL;
  h->ubCode = ubCode;
  h->ubSecondCode = ubSecondCode;
  h->uiDate = uiDate;
  h->sSectorX = sSectorX;
  h->sSectorY = sSectorY;
  h->bSectorZ = bSectorZ;

  // Append node to list
  HistoryUnit **anchor = &pHistoryListHead;
  while (*anchor != NULL) anchor = &(*anchor)->Next;
  *anchor = h;
}

// open and read in data to the History list
static void OpenAndReadHistoryFile() {
  ClearHistoryList();

  AutoSGPFile f(FileMan::openForReadingSmart(HISTORY_DATA_FILE, true));

  uint32_t entry_count = FileGetSize(f) / SIZE_OF_HISTORY_FILE_RECORD;
  while (entry_count-- > 0) {
    uint8_t ubCode;
    uint8_t ubSecondCode;
    uint32_t uiDate;
    int16_t sSectorX;
    int16_t sSectorY;
    int8_t bSectorZ;

    FileRead(f, &ubCode, sizeof(uint8_t));
    FileRead(f, &ubSecondCode, sizeof(uint8_t));
    FileRead(f, &uiDate, sizeof(uint32_t));
    FileRead(f, &sSectorX, sizeof(int16_t));
    FileRead(f, &sSectorY, sizeof(int16_t));
    FileRead(f, &bSectorZ, sizeof(int8_t));
    FileSeek(f, 1, FILE_SEEK_FROM_CURRENT);

    ProcessAndEnterAHistoryRecord(ubCode, uiDate, ubSecondCode, sSectorX, sSectorY, bSectorZ);
  }
}

void ClearHistoryList() {
  for (HistoryUnit *h = pHistoryListHead; h != NULL;) {
    HistoryUnit *const next = h->Next;
    MemFree(h);
    h = next;
  }
  pHistoryListHead = NULL;
}

static void DisplayHistoryListHeaders() {
  // this procedure will display the headers to each column in History
  SetFontAttributes(HISTORY_TEXT_FONT, FONT_BLACK, NO_SHADOW);

  int16_t usX;
  int16_t usY;

  // the date header
  FindFontCenterCoordinates(RECORD_DATE_X + 5, 0, RECORD_DATE_WIDTH, 0, pHistoryHeaders[0],
                            HISTORY_TEXT_FONT, &usX, &usY);
  MPrint(usX, RECORD_HEADER_Y, pHistoryHeaders[0]);

  // the date header
  FindFontCenterCoordinates(RECORD_DATE_X + RECORD_DATE_WIDTH + 5, 0, RECORD_LOCATION_WIDTH, 0,
                            pHistoryHeaders[3], HISTORY_TEXT_FONT, &usX, &usY);
  MPrint(usX, RECORD_HEADER_Y, pHistoryHeaders[3]);

  // event header
  FindFontCenterCoordinates(RECORD_DATE_X + RECORD_DATE_WIDTH + RECORD_LOCATION_WIDTH + 5, 0,
                            RECORD_LOCATION_WIDTH, 0, pHistoryHeaders[3], HISTORY_TEXT_FONT, &usX,
                            &usY);
  MPrint(usX, RECORD_HEADER_Y, pHistoryHeaders[4]);
  // reset shadow
  SetFontShadow(DEFAULT_SHADOW);
}

static void DisplayHistoryListBackground() {
  // this function will display the History list display background
  int32_t iCounter = 0;

  // get shaded line object
  for (iCounter = 0; iCounter < 11; iCounter++) {
    // blt title bar to screen
    BltVideoObject(FRAME_BUFFER, guiSHADELINE, 0, TOP_X + 15,
                   TOP_DIVLINE_Y + BOX_HEIGHT * 2 * iCounter);
  }

  // the long hortizontal line int he records list display region
  BltVideoObject(FRAME_BUFFER, guiLONGLINE, 0, TOP_X + 9, TOP_DIVLINE_Y);
  BltVideoObject(FRAME_BUFFER, guiLONGLINE, 0, TOP_X + 9, TOP_DIVLINE_Y + BOX_HEIGHT * 2 * 11);
}

static void ProcessHistoryTransactionString(wchar_t *pString, size_t Length,
                                            const HistoryUnit *pHistory);

// draw the text of the records
static void DrawHistoryRecordsText() {
  wchar_t sString[512];
  int16_t sX;
  int16_t sY;

  SetFont(HISTORY_TEXT_FONT);
  SetFontBackground(FONT_BLACK);
  SetFontShadow(NO_SHADOW);

  uint32_t entry_count = 0;
  for (const HistoryUnit *h = pHistoryListHead; h != NULL; h = h->Next) {
    const uint8_t colour =
        h->ubCode == HISTORY_CHEAT_ENABLED ||
                (h->ubCode == HISTORY_QUEST_STARTED && gubQuest[h->ubSecondCode] == QUESTINPROGRESS)
            ? FONT_RED
            : FONT_BLACK;
    SetFontForeground(colour);

    const int32_t y = RECORD_Y + entry_count * BOX_HEIGHT + 3;

    // get and write the date
    swprintf(sString, lengthof(sString), L"%d", h->uiDate / (24 * 60));
    int16_t usX;
    int16_t usY;
    FindFontCenterCoordinates(RECORD_DATE_X + 5, 0, RECORD_DATE_WIDTH, 0, sString,
                              HISTORY_TEXT_FONT, &usX, &usY);
    MPrint(usX, y, sString);

    if (h->sSectorX == -1 || h->sSectorY == -1) {
      // no location
      FindFontCenterCoordinates(RECORD_DATE_X + RECORD_DATE_WIDTH, 0, RECORD_LOCATION_WIDTH + 10, 0,
                                pHistoryLocations, HISTORY_TEXT_FONT, &sX, &sY);
      MPrint(sX, y, pHistoryLocations);
    } else {
      GetSectorIDString(h->sSectorX, h->sSectorY, h->bSectorZ, sString, lengthof(sString), TRUE);
      FindFontCenterCoordinates(RECORD_DATE_X + RECORD_DATE_WIDTH, 0, RECORD_LOCATION_WIDTH + 10, 0,
                                sString, HISTORY_TEXT_FONT, &sX, &sY);
      ReduceStringLength(sString, lengthof(sString), RECORD_LOCATION_WIDTH + 10, HISTORY_TEXT_FONT);
      MPrint(sX, y, sString);
    }

    // the actual history text
    ProcessHistoryTransactionString(sString, lengthof(sString), h);
    MPrint(RECORD_DATE_X + RECORD_LOCATION_WIDTH + RECORD_DATE_WIDTH + 15, y, sString);

    if (++entry_count == NUM_RECORDS_PER_PAGE) break;
  }

  // restore shadow
  SetFontShadow(DEFAULT_SHADOW);
}

static void DisplayPageNumberAndDateRange();

static void DrawAPageofHistoryRecords() {
  // this procedure will draw a series of history records to the screen

  // (re-)render background

  // the title bar text
  DrawHistoryTitleText();

  // the actual lists background
  DisplayHistoryListBackground();

  // the headers to each column
  DisplayHistoryListHeaders();

  // error check
  if (iCurrentHistoryPage == -1) {
    iCurrentHistoryPage = 0;
  }

  // current page is found, render  from here
  DrawHistoryRecordsText();

  // update page numbers, and date ranges
  DisplayPageNumberAndDateRange();
}

static int32_t GetNumberOfHistoryPages();

/* go through the list of 'histories' starting at current until end or
 * NUM_RECORDS_PER_PAGE and get the date range and the page number */
static void DisplayPageNumberAndDateRange() {
  uint32_t current_page;
  uint32_t count_pages;
  uint32_t first_date;
  uint32_t last_date;
  const HistoryUnit *h = pHistoryListHead;
  if (h == NULL) {
    current_page = 1;
    count_pages = 1;
    first_date = 1;
    last_date = 1;
  } else {
    current_page = iCurrentHistoryPage;
    count_pages = GetNumberOfHistoryPages();
    first_date = h->uiDate / (24 * 60);

    uint32_t entry_count = NUM_RECORDS_PER_PAGE;
    while (--entry_count != 0 && h->Next != NULL) h = h->Next;

    last_date = h->uiDate / (24 * 60);
  }

  SetFontAttributes(HISTORY_TEXT_FONT, FONT_BLACK, NO_SHADOW);
  mprintf(PAGE_NUMBER_X, PAGE_NUMBER_Y, L"%ls  %d / %d", pHistoryHeaders[1], current_page,
          count_pages);
  mprintf(HISTORY_DATE_X, HISTORY_DATE_Y, L"%ls %d - %d", pHistoryHeaders[2], first_date,
          last_date);
  SetFontShadow(DEFAULT_SHADOW);
}

static void GetQuestEndedString(uint8_t ubQuestValue, wchar_t *sQuestString);
static void GetQuestStartedString(uint8_t ubQuestValue, wchar_t *sQuestString);

static void ProcessHistoryTransactionString(wchar_t *const pString, const size_t Length,
                                            const HistoryUnit *const h) {
  const uint8_t code = h->ubCode;
  switch (code) {
    case HISTORY_QUEST_STARTED:
      GetQuestStartedString(h->ubSecondCode, pString);
      break;

    case HISTORY_QUEST_FINISHED:
      GetQuestEndedString(h->ubSecondCode, pString);
      break;

    case HISTORY_LIBERATED_TOWN:
    case HISTORY_MINE_RAN_OUT:
    case HISTORY_MINE_REOPENED:
    case HISTORY_MINE_RUNNING_OUT:
    case HISTORY_MINE_SHUTDOWN:
    case HISTORY_TALKED_TO_MINER:
      swprintf(pString, Length, pHistoryStrings[code], pTownNames[h->ubSecondCode]);
      break;

    case HISTORY_MERC_KILLED:
      if (h->ubSecondCode == NO_PROFILE) {
        break;
      }
      swprintf(pString, Length, pHistoryStrings[code], GetProfile(h->ubSecondCode).zName);
      break;

    case HISTORY_HIRED_MERC_FROM_AIM:
    case HISTORY_HIRED_MERC_FROM_MERC:
    case HISTORY_MERC_CONTRACT_EXPIRED:
    case HISTORY_RPC_JOINED_TEAM:
      swprintf(pString, Length, pHistoryStrings[code], GetProfile(h->ubSecondCode).zName);
      break;

    case HISTORY_CANCELLED_INSURANCE:
    case HISTORY_DISQUALIFIED_BOXING:
    case HISTORY_EXTENDED_CONTRACT_1_DAY:
    case HISTORY_EXTENDED_CONTRACT_1_WEEK:
    case HISTORY_EXTENDED_CONTRACT_2_WEEK:
    case HISTORY_INSURANCE_CLAIM_PAYOUT:
    case HISTORY_LOST_BOXING:
    case HISTORY_MERC_FIRED:
    case HISTORY_MERC_KILLED_CHARACTER:
    case HISTORY_MERC_MARRIED_OFF:
    case HISTORY_MERC_QUIT:
    case HISTORY_NPC_KILLED:
    case HISTORY_PURCHASED_INSURANCE:
    case HISTORY_WON_BOXING:
      swprintf(pString, Length, pHistoryStrings[code], GetProfile(h->ubSecondCode).zNickname);
      break;

    // all simple history log msgs, no params
    case HISTORY_ACCEPTED_ASSIGNMENT_FROM_ENRICO:
    case HISTORY_ARNOLD:
    case HISTORY_ASSASSIN:
    case HISTORY_BOXING_MATCHES:
    case HISTORY_BUM_KEYCARD:
    case HISTORY_CHARACTER_GENERATED:
    case HISTORY_CHEAT_ENABLED:
    case HISTORY_CREATURESATTACKED:
    case HISTORY_DAVE:
    case HISTORY_DEFENDEDTOWNSECTOR:
    case HISTORY_DEIDRANNA_DEAD_BODIES:
    case HISTORY_DEVIN:
    case HISTORY_DISCOVERED_ORTA:
    case HISTORY_DISCOVERED_TIXA:
    case HISTORY_ENRICO_COMPLAINED:
    case HISTORY_ENTERED_HISTORY_MODE:
    case HISTORY_FATALAMBUSH:
    case HISTORY_FOUND_MONEY:
    case HISTORY_FRANZ:
    case HISTORY_FREDO:
    case HISTORY_GABBY:
    case HISTORY_GAVE_CARMEN_HEAD:
    case HISTORY_GOT_ROCKET_RIFLES:
    case HISTORY_HOWARD:
    case HISTORY_HOWARD_CYANIDE:
    case HISTORY_JAKE:
    case HISTORY_KEITH:
    case HISTORY_KEITH_OUT_OF_BUSINESS:
    case HISTORY_KILLEDBYBLOODCATS:
    case HISTORY_KINGPIN_MONEY:
    case HISTORY_KROTT:
    case HISTORY_KYLE:
    case HISTORY_LOSTBATTLE:
    case HISTORY_LOSTTOWNSECTOR:
    case HISTORY_MADLAB:
    case HISTORY_MIKE:
    case HISTORY_PABLO:
    case HISTORY_PERKO:
    case HISTORY_RICHGUY_BALIME:
    case HISTORY_SAM:
    case HISTORY_SETTLED_ACCOUNTS_AT_MERC:
    case HISTORY_SLAUGHTEREDBLOODCATS:
    case HISTORY_SLAY_MYSTERIOUSLY_LEFT:
    case HISTORY_SOMETHING_IN_MINES:
    case HISTORY_SUCCESSFULATTACK:
    case HISTORY_TALKED_TO_FATHER_WALKER:
    case HISTORY_TONY:
    case HISTORY_UNSUCCESSFULATTACK:
    case HISTORY_WALTER:
    case HISTORY_WIPEDOUTENEMYAMBUSH:
    case HISTORY_WONBATTLE:
      swprintf(pString, Length, pHistoryStrings[code]);
      break;
  }
}

// look at what page we are viewing, enable and disable buttons as needed
static void SetHistoryButtonStates() {
  EnableButton(giHistoryButton[PREV_PAGE_BUTTON], iCurrentHistoryPage != 1);
  EnableButton(giHistoryButton[NEXT_PAGE_BUTTON], iCurrentHistoryPage < GetNumberOfHistoryPages());
}

// loads in records belogning, to page uiPage
static BOOLEAN LoadInHistoryRecords(const uint32_t uiPage) try {
  ClearHistoryList();

  // check if bad page
  if (uiPage == 0) return FALSE;

  AutoSGPFile f(FileMan::openForReadingSmart(HISTORY_DATA_FILE, true));

  uint32_t entry_count = FileGetSize(f) / SIZE_OF_HISTORY_FILE_RECORD;
  uint32_t const skip = (uiPage - 1) * NUM_RECORDS_PER_PAGE;
  if (entry_count <= skip) return FALSE;

  FileSeek(f, skip * SIZE_OF_HISTORY_FILE_RECORD, FILE_SEEK_FROM_START);
  entry_count -= skip;

  if (entry_count > NUM_RECORDS_PER_PAGE) entry_count = NUM_RECORDS_PER_PAGE;

  while (entry_count-- > 0) {
    uint8_t ubCode;
    uint8_t ubSecondCode;
    uint32_t uiDate;
    int16_t sSectorX;
    int16_t sSectorY;
    int8_t bSectorZ;

    FileRead(f, &ubCode, sizeof(uint8_t));
    FileRead(f, &ubSecondCode, sizeof(uint8_t));
    FileRead(f, &uiDate, sizeof(uint32_t));
    FileRead(f, &sSectorX, sizeof(int16_t));
    FileRead(f, &sSectorY, sizeof(int16_t));
    FileRead(f, &bSectorZ, sizeof(int8_t));
    FileSeek(f, 1, FILE_SEEK_FROM_CURRENT);

    ProcessAndEnterAHistoryRecord(ubCode, uiDate, ubSecondCode, sSectorX, sSectorY, bSectorZ);
  }

  return TRUE;
} catch (...) {
  return FALSE;
}

// clear out old list of records, and load in next page worth of records
static void LoadNextHistoryPage() {
  // now load in previous page's records, if we can
  if (LoadInHistoryRecords(iCurrentHistoryPage + 1)) {
    iCurrentHistoryPage++;
  } else {
    LoadInHistoryRecords(iCurrentHistoryPage);
  }
  SetHistoryButtonStates();
  fReDrawScreenFlag = TRUE;
}

// clear out old list of records and load in previous page worth of records
static void LoadPreviousHistoryPage() {
  if (iCurrentHistoryPage <= 1) return;
  LoadInHistoryRecords(--iCurrentHistoryPage);
  SetHistoryButtonStates();
  fReDrawScreenFlag = TRUE;
}

static void AppendHistoryToEndOfFile() {
  AutoSGPFile f(FileMan::openForAppend(HISTORY_DATA_FILE));

  const HistoryUnit *const h = pHistoryListHead;

  uint8_t data[12];
  uint8_t *d = data;
  INJ_U8(d, h->ubCode)
  INJ_U8(d, h->ubSecondCode)
  INJ_U32(d, h->uiDate)
  INJ_I16(d, h->sSectorX)
  INJ_I16(d, h->sSectorY)
  INJ_I8(d, h->bSectorZ)
  INJ_SKIP(d, 1)
  Assert(d == endof(data));

  FileWrite(f, data, sizeof(data));
}

uint32_t GetTimeQuestWasStarted(uint8_t ubCode) {
  iCurrentHistoryPage = 0;
  OpenAndReadHistoryFile();

  uint32_t uiTime = 0;
  for (const HistoryUnit *h = pHistoryListHead; h != NULL; h = h->Next) {
    if (h->ubSecondCode == ubCode && h->ubCode == HISTORY_QUEST_STARTED) {
      uiTime = h->uiDate;
      break;
    }
  }

  if (fInHistoryMode) LoadInHistoryRecords(iCurrentHistoryPage);

  return uiTime;
}

static void GetQuestStartedString(const uint8_t ubQuestValue, wchar_t *const sQuestString) {
  // open the file and copy the string
  LoadEncryptedDataFromFile(BINARYDATADIR "/quests.edt", sQuestString,
                            HISTORY_QUEST_TEXT_SIZE * ubQuestValue * 2, HISTORY_QUEST_TEXT_SIZE);
}

static void GetQuestEndedString(const uint8_t ubQuestValue, wchar_t *const sQuestString) {
  // open the file and copy the string
  LoadEncryptedDataFromFile(BINARYDATADIR "/quests.edt", sQuestString,
                            HISTORY_QUEST_TEXT_SIZE * (ubQuestValue * 2 + 1),
                            HISTORY_QUEST_TEXT_SIZE);
}

static int32_t GetNumberOfHistoryPages() {
  AutoSGPFile f(FileMan::openForReadingSmart(HISTORY_DATA_FILE, true));

  const uint32_t uiFileSize = FileGetSize(f);

  if (uiFileSize == 0) return 1;

  return (uiFileSize / SIZE_OF_HISTORY_FILE_RECORD + NUM_RECORDS_PER_PAGE - 1) /
         NUM_RECORDS_PER_PAGE;
}
