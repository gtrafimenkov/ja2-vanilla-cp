#include "Laptop/Finances.h"

#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "Directories.h"
#include "Laptop/Laptop.h"
#include "Laptop/LaptopSave.h"
#include "SGP/ButtonSystem.h"
#include "SGP/Debug.h"
#include "SGP/FileMan.h"
#include "SGP/Font.h"
#include "SGP/LoadSaveData.h"
#include "SGP/MemMan.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "Strategic/CampaignTypes.h"
#include "Strategic/GameClock.h"
#include "Strategic/MapScreenInterfaceBottom.h"
#include "Strategic/StrategicMap.h"
#include "Strategic/StrategicMines.h"
#include "Tactical/SoldierProfile.h"
#include "TileEngine/RenderDirty.h"
#include "Utils/Cursors.h"
#include "Utils/FontControl.h"
#include "Utils/Text.h"

#define FINANCE_HEADER_SIZE 4
#define FINANCE_RECORD_SIZE (1 + 1 + 4 + 4 + 4)

// the financial structure
struct FinanceUnit {
  uint8_t ubCode;        // the code index in the finance code table
  uint8_t ubSecondCode;  // secondary code
  uint32_t uiDate;       // time in the world in global time
  int32_t iAmount;       // the amount of the transaction
  int32_t iBalanceToDate;
  FinanceUnit *Next;  // next unit in the list
};

// the global defines

// graphical positions
#define TOP_X 0 + LAPTOP_SCREEN_UL_X
#define TOP_Y LAPTOP_SCREEN_UL_Y
#define BLOCK_HEIGHT 10
#define TOP_DIVLINE_Y 102
#define DIVLINE_X 130
#define MID_DIVLINE_Y 205
#define BOT_DIVLINE_Y 180
#define MID_DIVLINE_Y2 263 + 20
#define BOT_DIVLINE_Y2 MID_DIVLINE_Y2 + MID_DIVLINE_Y - BOT_DIVLINE_Y
#define TITLE_X 140
#define TITLE_Y 33
#define TEXT_X 140
#define PAGE_SIZE 17

// yesterdyas/todays income and balance text positions
#define YESTERDAYS_INCOME 114
#define YESTERDAYS_OTHER 138
#define YESTERDAYS_DEBITS 162
#define YESTERDAYS_BALANCE 188
#define TODAYS_INCOME 215
#define TODAYS_OTHER 239
#define TODAYS_DEBITS 263
#define TODAYS_CURRENT_BALANCE 263 + 28
#define TODAYS_CURRENT_FORCAST_INCOME 330
#define TODAYS_CURRENT_FORCAST_BALANCE 354
#define FINANCE_HEADER_FONT FONT14ARIAL
#define FINANCE_TEXT_FONT FONT12ARIAL
#define NUM_RECORDS_PER_PAGE PAGE_SIZE

// records text positions
#define RECORD_CREDIT_WIDTH 106 - 47
#define RECORD_DEBIT_WIDTH RECORD_CREDIT_WIDTH
#define RECORD_DATE_X TOP_X + 10
#define RECORD_TRANSACTION_X RECORD_DATE_X + RECORD_DATE_WIDTH
#define RECORD_TRANSACTION_WIDTH 500 - 280
#define RECORD_DEBIT_X RECORD_TRANSACTION_X + RECORD_TRANSACTION_WIDTH
#define RECORD_CREDIT_X RECORD_DEBIT_X + RECORD_DEBIT_WIDTH
#define RECORD_Y 107 - 10
#define RECORD_DATE_WIDTH 47
#define RECORD_BALANCE_X RECORD_DATE_X + 385
#define RECORD_BALANCE_WIDTH 479 - 385
#define RECORD_HEADER_Y 90

#define PAGE_NUMBER_X TOP_X + 297  // 345
#define PAGE_NUMBER_Y TOP_Y + 33

// BUTTON defines
enum {
  PREV_PAGE_BUTTON = 0,
  NEXT_PAGE_BUTTON,
  FIRST_PAGE_BUTTON,
  LAST_PAGE_BUTTON,
};

// button positions

#define FIRST_PAGE_X 505
#define NEXT_BTN_X 553  // 577
#define PREV_BTN_X 529  // 553
#define LAST_PAGE_X 577
#define BTN_Y 53

// sizeof one record
#define RECORD_SIZE \
  (sizeof(uint32_t) + sizeof(int32_t) + sizeof(int32_t) + sizeof(uint8_t) + sizeof(uint8_t))

// the financial record list
static FinanceUnit *pFinanceListHead = NULL;

// current page displayed
static int32_t iCurrentPage = 0;

// video object id's
static SGPVObject *guiTITLE;
static SGPVObject *guiTOP;
static SGPVObject *guiLINE;
static SGPVObject *guiLONGLINE;
static SGPVObject *guiLISTCOLUMNS;

// are in the financial system right now?
static BOOLEAN fInFinancialMode = FALSE;

// the last page altogether
static uint32_t guiLastPageInRecordsList = 0;

// finance screen buttons
static GUIButtonRef giFinanceButton[4];
static BUTTON_PICS *giFinanceButtonImage[4];
static MOUSE_REGION g_scroll_region;

// internal functions
static void ProcessAndEnterAFinacialRecord(uint8_t ubCode, uint32_t uiDate, int32_t iAmount,
                                           uint8_t ubSecondCode, int32_t iBalanceToDate);
static void LoadFinances();
static void RemoveFinances();
static void ClearFinanceList();
static void DrawRecordsColumnHeadersText();
static void CreateFinanceButtons();
static void DestroyFinanceButtons();
static void GetBalanceFromDisk();
static void WriteBalanceToDisk();
static void AppendFinanceToEndOfFile();
static void SetLastPageInRecords();
static void LoadInRecords(uint32_t page);

static void SetFinanceButtonStates();
static int32_t GetTodaysDebits();
static int32_t GetYesterdaysDebits();

void AddTransactionToPlayersBook(uint8_t ubCode, uint8_t ubSecondCode, uint32_t uiDate,
                                 int32_t iAmount) {
  // adds transaction to player's book(Financial List)
  // outside of the financial system(the code in this .c file), this is the only
  // function you'll ever need

  // read in balance from file

  GetBalanceFromDisk();
  // process the actual data

  //
  // If this transaction is for the hiring/extending of a mercs contract
  //
  if (ubCode == HIRED_MERC || ubCode == IMP_PROFILE || ubCode == PAYMENT_TO_NPC ||
      ubCode == EXTENDED_CONTRACT_BY_1_DAY || ubCode == EXTENDED_CONTRACT_BY_1_WEEK ||
      ubCode == EXTENDED_CONTRACT_BY_2_WEEKS) {
    gMercProfiles[ubSecondCode].uiTotalCostToDate += -iAmount;
  }

  // clear list
  ClearFinanceList();

  // update balance
  LaptopSaveInfo.iCurrentBalance += iAmount;

  ProcessAndEnterAFinacialRecord(ubCode, uiDate, iAmount, ubSecondCode,
                                 LaptopSaveInfo.iCurrentBalance);

  // write balance to disk
  WriteBalanceToDisk();

  // append to end of file
  AppendFinanceToEndOfFile();

  // set number of pages
  SetLastPageInRecords();

  if (!fInFinancialMode) {
    ClearFinanceList();
  } else {
    SetFinanceButtonStates();

    // force update
    fPausedReDrawScreenFlag = TRUE;
  }

  fMapScreenBottomDirty = TRUE;
}

int32_t GetCurrentBalance() {
  // get balance to this minute
  return (LaptopSaveInfo.iCurrentBalance);
}

int32_t GetProjectedTotalDailyIncome() {
  // return total  projected income, including what is earned today already

  // CJC: I DON'T THINK SO!
  // The point is:  PredictIncomeFromPlayerMines isn't dependant on the time of
  // day (anymore) and this would report income of 0 at midnight!
  /*
if (GetWorldMinutesInDay() <= 0)
  {
          return ( 0 );
  }
  */
  // look at we earned today

  // then there is how many deposits have been made, now look at how many mines
  // we have, thier rate, amount of ore left and predict if we still had these
  // mines how much more would we get?

  return (PredictIncomeFromPlayerMines());
}

void GameInitFinances() {
  // initialize finances on game start up
  FileDelete(FINANCES_DATA_FILE);
  GetBalanceFromDisk();
}

void EnterFinances() {
  // entry into finanacial system, load graphics, set variables..draw screen
  // once
  // set the fact we are in the financial display system

  fInFinancialMode = TRUE;

  // get the balance
  GetBalanceFromDisk();

  // set number of pages
  SetLastPageInRecords();

  // load graphics into memory
  LoadFinances();

  // create buttons
  CreateFinanceButtons();

  // reset page we are on
  LoadInRecords(LaptopSaveInfo.iCurrentFinancesPage);

  RenderFinances();
}

void ExitFinances() {
  LaptopSaveInfo.iCurrentFinancesPage = iCurrentPage;

  // not in finance system anymore
  fInFinancialMode = FALSE;

  // destroy buttons
  DestroyFinanceButtons();

  // clear out list
  ClearFinanceList();

  // remove graphics
  RemoveFinances();
}

static void DisplayFinancePageNumberAndDateRange();
static void DrawAPageOfRecords();
static void DrawFinanceTitleText();
static void DrawSummary();
static void RenderBackGround();

void RenderFinances() {
  RenderBackGround();

  // if we are on the first page, draw the summary
  if (iCurrentPage == 0)
    DrawSummary();
  else
    DrawAPageOfRecords();

  DrawFinanceTitleText();

  DisplayFinancePageNumberAndDateRange();

  BltVideoObject(FRAME_BUFFER, guiLaptopBACKGROUND, 0, 108, 23);

  BlitTitleBarIcons();
}

static void LoadFinances() {
  // load Finance video objects into memory

  // title bar
  guiTITLE = AddVideoObjectFromFile(LAPTOPDIR "/programtitlebar.sti");

  // top portion of the screen background
  guiTOP = AddVideoObjectFromFile(LAPTOPDIR "/financeswindow.sti");

  // black divider line - long ( 480 length)
  guiLONGLINE = AddVideoObjectFromFile(LAPTOPDIR "/divisionline480.sti");

  // the records columns
  guiLISTCOLUMNS = AddVideoObjectFromFile(LAPTOPDIR "/recordcolumns.sti");

  // black divider line - long ( 480 length)
  guiLINE = AddVideoObjectFromFile(LAPTOPDIR "/divisionline.sti");
}

static void RemoveFinances() {
  // delete Finance video objects from memory
  DeleteVideoObject(guiLONGLINE);
  DeleteVideoObject(guiLINE);
  DeleteVideoObject(guiLISTCOLUMNS);
  DeleteVideoObject(guiTOP);
  DeleteVideoObject(guiTITLE);
}

static void RenderBackGround() {
  // render generic background for Finance system
  BltVideoObject(FRAME_BUFFER, guiTITLE, 0, TOP_X, TOP_Y - 2);
  BltVideoObject(FRAME_BUFFER, guiTOP, 0, TOP_X, TOP_Y + 22);
}

static void DrawSummaryLines();
static void DrawSummaryText();

static void DrawSummary() {
  // draw day's summary to screen
  DrawSummaryLines();
  DrawSummaryText();
}

static void DrawSummaryLines() {
  // draw divider lines on screen
  // blit summary LINE object to screen
  BltVideoObject(FRAME_BUFFER, guiLINE, 0, DIVLINE_X, TOP_DIVLINE_Y);
  BltVideoObject(FRAME_BUFFER, guiLINE, 0, DIVLINE_X, TOP_DIVLINE_Y + 2);
  // BltVideoObject(FRAME_BUFFER, guiLINE, 0,DIVLINE_X, MID_DIVLINE_Y);
  BltVideoObject(FRAME_BUFFER, guiLINE, 0, DIVLINE_X, BOT_DIVLINE_Y);
  BltVideoObject(FRAME_BUFFER, guiLINE, 0, DIVLINE_X, MID_DIVLINE_Y2);
  // BltVideoObject(FRAME_BUFFER, guiLINE, 0,DIVLINE_X, BOT_DIVLINE_Y2);
}

static void DrawRecordsBackGround();
static void DrawRecordsText();

static void DrawAPageOfRecords() {
  // this procedure will draw a series of financial records to the screen

  // (re-)render background
  DrawRecordsBackGround();

  // error check
  if (iCurrentPage == -1) return;

  // current page is found, render  from here
  DrawRecordsText();
}

static void DrawRecordsBackGround() {
  // proceudre will draw the background for the list of financial records
  int32_t iCounter;

  // now the columns
  for (iCounter = 6; iCounter < 35; iCounter++) {
    BltVideoObject(FRAME_BUFFER, guiLISTCOLUMNS, 0, TOP_X + 10,
                   TOP_Y + 18 + iCounter * BLOCK_HEIGHT + 1);
  }

  // the divisorLines
  BltVideoObject(FRAME_BUFFER, guiLONGLINE, 0, TOP_X + 10, TOP_Y + 17 + 6 * BLOCK_HEIGHT);
  BltVideoObject(FRAME_BUFFER, guiLONGLINE, 0, TOP_X + 10, TOP_Y + 19 + 6 * BLOCK_HEIGHT);
  BltVideoObject(FRAME_BUFFER, guiLONGLINE, 0, TOP_X + 10, TOP_Y + 19 + iCounter * BLOCK_HEIGHT);

  // the header text
  DrawRecordsColumnHeadersText();
}

static void DrawRecordsColumnHeadersText() {
  // write the headers text for each column
  SetFontAttributes(FINANCE_TEXT_FONT, FONT_BLACK, NO_SHADOW);

  int16_t usX;
  int16_t usY;

  // the date header
  FindFontCenterCoordinates(RECORD_DATE_X, 0, RECORD_DATE_WIDTH, 0, pFinanceHeaders[0],
                            FINANCE_TEXT_FONT, &usX, &usY);
  MPrint(usX, RECORD_HEADER_Y, pFinanceHeaders[0]);

  // debit header
  FindFontCenterCoordinates(RECORD_DEBIT_X, 0, RECORD_DEBIT_WIDTH, 0, pFinanceHeaders[1],
                            FINANCE_TEXT_FONT, &usX, &usY);
  MPrint(usX, RECORD_HEADER_Y, pFinanceHeaders[1]);

  // credit header
  FindFontCenterCoordinates(RECORD_CREDIT_X, 0, RECORD_CREDIT_WIDTH, 0, pFinanceHeaders[2],
                            FINANCE_TEXT_FONT, &usX, &usY);
  MPrint(usX, RECORD_HEADER_Y, pFinanceHeaders[2]);

  // balance header
  FindFontCenterCoordinates(RECORD_BALANCE_X, 0, RECORD_BALANCE_WIDTH, 0, pFinanceHeaders[4],
                            FINANCE_TEXT_FONT, &usX, &usY);
  MPrint(usX, RECORD_HEADER_Y, pFinanceHeaders[4]);

  // transaction header
  FindFontCenterCoordinates(RECORD_TRANSACTION_X, 0, RECORD_TRANSACTION_WIDTH, 0,
                            pFinanceHeaders[3], FINANCE_TEXT_FONT, &usX, &usY);
  MPrint(usX, RECORD_HEADER_Y, pFinanceHeaders[3]);

  SetFontShadow(DEFAULT_SHADOW);
}

static void DrawStringCentered(const int32_t x, const int32_t y, const int32_t w,
                               const wchar_t *const str) {
  int16_t sx;
  int16_t sy;
  FindFontCenterCoordinates(x, 0, w, 0, str, FINANCE_TEXT_FONT, &sx, &sy);
  MPrint(sx, y, str);
}

static void ProcessTransactionString(wchar_t pString[], size_t Length, const FinanceUnit *pFinance);

// draws the text of the records
static void DrawRecordsText() {
  SetFont(FINANCE_TEXT_FONT);
  SetFontBackground(FONT_BLACK);
  SetFontShadow(NO_SHADOW);

  const FinanceUnit *fu = pFinanceListHead;
  for (int32_t i = 0; i < NUM_RECORDS_PER_PAGE && fu != NULL; ++i, fu = fu->Next) {
    const int32_t y = 12 + RECORD_Y + i * (GetFontHeight(FINANCE_TEXT_FONT) + 6);
    wchar_t sString[512];

    SetFontForeground(FONT_BLACK);

    // get and write the date
    swprintf(sString, lengthof(sString), L"%d", fu->uiDate / (24 * 60));
    DrawStringCentered(RECORD_DATE_X, y, RECORD_DATE_WIDTH, sString);

    // get and write debit/credit
    if (fu->iAmount >= 0) {
      // increase in asset - debit
      SPrintMoney(sString, fu->iAmount);
      DrawStringCentered(RECORD_DEBIT_X, y, RECORD_DEBIT_WIDTH, sString);
    } else {
      // decrease in asset - credit
      SetFontForeground(FONT_RED);
      SPrintMoney(sString, -fu->iAmount);
      DrawStringCentered(RECORD_CREDIT_X, y, RECORD_CREDIT_WIDTH, sString);
    }

    // the balance to this point
    int32_t balance = fu->iBalanceToDate;
    if (balance >= 0) {
      SetFontForeground(FONT_BLACK);
    } else {
      SetFontForeground(FONT_RED);
      balance = -balance;
    }
    SPrintMoney(sString, balance);
    DrawStringCentered(RECORD_BALANCE_X, y, RECORD_BALANCE_WIDTH, sString);

    // transaction string
    ProcessTransactionString(sString, lengthof(sString), fu);
    DrawStringCentered(RECORD_TRANSACTION_X, y, RECORD_TRANSACTION_WIDTH, sString);
  }
}

static void DrawFinanceTitleText() {
  // draw the pages title
  SetFontAttributes(FINANCE_HEADER_FONT, FONT_WHITE);
  MPrint(TITLE_X, TITLE_Y, pFinanceTitle);
}

static int32_t GetPreviousDaysIncome();
static int32_t GetTodaysBalance();
static int32_t GetTodaysDaysIncome();
static int32_t GetTodaysOtherDeposits();
static int32_t GetYesterdaysOtherDeposits();
static void SPrintMoneyNoDollarOnZero(wchar_t *Str, int32_t Amount);

static void DrawSummaryText() {
  int16_t usX, usY;
  wchar_t pString[100];
  int32_t iBalance = 0;

  SetFontAttributes(FINANCE_TEXT_FONT, FONT_BLACK, NO_SHADOW);

  // draw summary text to the screen
  MPrint(TEXT_X, YESTERDAYS_INCOME, pFinanceSummary[2]);
  MPrint(TEXT_X, YESTERDAYS_OTHER, pFinanceSummary[3]);
  MPrint(TEXT_X, YESTERDAYS_DEBITS, pFinanceSummary[4]);
  MPrint(TEXT_X, YESTERDAYS_BALANCE, pFinanceSummary[5]);
  MPrint(TEXT_X, TODAYS_INCOME, pFinanceSummary[6]);
  MPrint(TEXT_X, TODAYS_OTHER, pFinanceSummary[7]);
  MPrint(TEXT_X, TODAYS_DEBITS, pFinanceSummary[8]);
  MPrint(TEXT_X, TODAYS_CURRENT_BALANCE, pFinanceSummary[9]);
  MPrint(TEXT_X, TODAYS_CURRENT_FORCAST_INCOME, pFinanceSummary[10]);
  MPrint(TEXT_X, TODAYS_CURRENT_FORCAST_BALANCE, pFinanceSummary[11]);

  // draw the actual numbers

  // yesterdays income
  SPrintMoneyNoDollarOnZero(pString, GetPreviousDaysIncome());
  FindFontRightCoordinates(0, 0, 580, 0, pString, FINANCE_TEXT_FONT, &usX, &usY);
  MPrint(usX, YESTERDAYS_INCOME, pString);

  SetFontForeground(FONT_BLACK);

  // yesterdays other
  SPrintMoneyNoDollarOnZero(pString, GetYesterdaysOtherDeposits());
  FindFontRightCoordinates(0, 0, 580, 0, pString, FINANCE_TEXT_FONT, &usX, &usY);
  MPrint(usX, YESTERDAYS_OTHER, pString);

  SetFontForeground(FONT_RED);

  // yesterdays debits
  iBalance = GetYesterdaysDebits();
  if (iBalance < 0) {
    SetFontForeground(FONT_RED);
    iBalance *= -1;
  }

  SPrintMoneyNoDollarOnZero(pString, iBalance);
  FindFontRightCoordinates(0, 0, 580, 0, pString, FINANCE_TEXT_FONT, &usX, &usY);
  MPrint(usX, YESTERDAYS_DEBITS, pString);

  SetFontForeground(FONT_BLACK);

  // yesterdays balance..ending balance..so todays balance then
  iBalance = GetTodaysBalance();

  if (iBalance < 0) {
    SetFontForeground(FONT_RED);
    iBalance *= -1;
  }

  SPrintMoneyNoDollarOnZero(pString, iBalance);
  FindFontRightCoordinates(0, 0, 580, 0, pString, FINANCE_TEXT_FONT, &usX, &usY);
  MPrint(usX, YESTERDAYS_BALANCE, pString);

  SetFontForeground(FONT_BLACK);

  // todays income
  SPrintMoneyNoDollarOnZero(pString, GetTodaysDaysIncome());
  FindFontRightCoordinates(0, 0, 580, 0, pString, FINANCE_TEXT_FONT, &usX, &usY);
  MPrint(usX, TODAYS_INCOME, pString);

  SetFontForeground(FONT_BLACK);

  // todays other
  SPrintMoneyNoDollarOnZero(pString, GetTodaysOtherDeposits());
  FindFontRightCoordinates(0, 0, 580, 0, pString, FINANCE_TEXT_FONT, &usX, &usY);
  MPrint(usX, TODAYS_OTHER, pString);

  SetFontForeground(FONT_RED);

  // todays debits
  iBalance = GetTodaysDebits();

  // absolute value
  if (iBalance < 0) {
    iBalance *= (-1);
  }

  SPrintMoneyNoDollarOnZero(pString, iBalance);
  FindFontRightCoordinates(0, 0, 580, 0, pString, FINANCE_TEXT_FONT, &usX, &usY);
  MPrint(usX, TODAYS_DEBITS, pString);

  SetFontForeground(FONT_BLACK);

  // todays current balance
  iBalance = GetCurrentBalance();
  if (iBalance < 0) {
    iBalance *= -1;
    SetFontForeground(FONT_RED);
  }

  SPrintMoneyNoDollarOnZero(pString, iBalance);
  FindFontRightCoordinates(0, 0, 580, 0, pString, FINANCE_TEXT_FONT, &usX, &usY);
  MPrint(usX, TODAYS_CURRENT_BALANCE, pString);

  SetFontForeground(FONT_BLACK);

  // todays forcast income
  SPrintMoneyNoDollarOnZero(pString, GetProjectedTotalDailyIncome());
  FindFontRightCoordinates(0, 0, 580, 0, pString, FINANCE_TEXT_FONT, &usX, &usY);
  MPrint(usX, TODAYS_CURRENT_FORCAST_INCOME, pString);

  SetFontForeground(FONT_BLACK);

  // todays forcast balance
  iBalance = GetCurrentBalance() + GetProjectedTotalDailyIncome();
  if (iBalance < 0) {
    iBalance *= -1;
    SetFontForeground(FONT_RED);
  }

  SPrintMoneyNoDollarOnZero(pString, iBalance);
  FindFontRightCoordinates(0, 0, 580, 0, pString, FINANCE_TEXT_FONT, &usX, &usY);
  MPrint(usX, TODAYS_CURRENT_FORCAST_BALANCE, pString);

  SetFontForeground(FONT_BLACK);

  // reset the shadow
  SetFontShadow(DEFAULT_SHADOW);
}

static void ClearFinanceList() {
  // remove each element from list of transactions
  FinanceUnit *pFinanceList = pFinanceListHead;
  FinanceUnit *pFinanceNode = pFinanceList;

  // while there are elements in the list left, delete them
  while (pFinanceList) {
    // set node to list head
    pFinanceNode = pFinanceList;

    // set list head to next node
    pFinanceList = pFinanceList->Next;

    // delete current node
    MemFree(pFinanceNode);
  }
  pFinanceListHead = NULL;
}

static void ProcessAndEnterAFinacialRecord(const uint8_t ubCode, const uint32_t uiDate,
                                           const int32_t iAmount, const uint8_t ubSecondCode,
                                           const int32_t iBalanceToDate) {
  FinanceUnit *const fu = MALLOC(FinanceUnit);
  fu->Next = NULL;
  fu->ubCode = ubCode;
  fu->ubSecondCode = ubSecondCode;
  fu->uiDate = uiDate;
  fu->iAmount = iAmount;
  fu->iBalanceToDate = iBalanceToDate;

  // Append to end of list
  FinanceUnit **i = &pFinanceListHead;
  while (*i != NULL) i = &(*i)->Next;
  *i = fu;
}

static void LoadPreviousPage();
static void LoadNextPage();

static void ScrollRegionCallback(MOUSE_REGION *const, int32_t const reason) {
  if (reason & MSYS_CALLBACK_REASON_WHEEL_UP) {
    LoadPreviousPage();
  } else if (reason & MSYS_CALLBACK_REASON_WHEEL_DOWN) {
    LoadNextPage();
  }
}

static void BtnFinanceDisplayPrevPageCallBack(GUI_BUTTON *const, int32_t const reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    LoadPreviousPage();
  }
}

static void BtnFinanceDisplayNextPageCallBack(GUI_BUTTON *const, int32_t const reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    LoadNextPage();
  }
}

static void BtnFinanceFirstPageCallBack(GUI_BUTTON *const, int32_t const reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    LoadInRecords(0);
  }
}

static void BtnFinanceLastPageCallBack(GUI_BUTTON *const, int32_t const reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    LoadInRecords(guiLastPageInRecordsList + 1);
  }
}

static void MakeButton(size_t const idx, BUTTON_PICS *const img, int16_t const x,
                       GUI_CALLBACK const callback) {
  giFinanceButtonImage[idx] = img;
  GUIButtonRef const btn = QuickCreateButton(img, x, BTN_Y, MSYS_PRIORITY_HIGHEST - 1, callback);
  giFinanceButton[idx] = btn;
  btn->SetCursor(CURSOR_LAPTOP_SCREEN);
}

static void CreateFinanceButtons() {
  BUTTON_PICS *const img = LoadButtonImage(LAPTOPDIR "/arrows.sti", 0, 1);
  MakeButton(PREV_PAGE_BUTTON, img, PREV_BTN_X, BtnFinanceDisplayPrevPageCallBack);
  MakeButton(NEXT_PAGE_BUTTON, UseLoadedButtonImage(img, 6, 7), NEXT_BTN_X,
             BtnFinanceDisplayNextPageCallBack);
  MakeButton(FIRST_PAGE_BUTTON, UseLoadedButtonImage(img, 3, 4), FIRST_PAGE_X,
             BtnFinanceFirstPageCallBack);
  MakeButton(LAST_PAGE_BUTTON, UseLoadedButtonImage(img, 9, 10), LAST_PAGE_X,
             BtnFinanceLastPageCallBack);

  uint16_t const x = TOP_X + 8;
  uint16_t const y = TOP_Y + 53;
  uint16_t const w = 482;
  uint16_t const h = 354;
  MSYS_DefineRegion(&g_scroll_region, x, y, x + w, y + h, MSYS_PRIORITY_HIGH, MSYS_NO_CURSOR,
                    MSYS_NO_CALLBACK, ScrollRegionCallback);
}

static void DestroyFinanceButtons() {
  MSYS_RemoveRegion(&g_scroll_region);
  for (uint32_t i = 0; i != 4; ++i) {
    RemoveButton(giFinanceButton[i]);
    UnloadButtonImage(giFinanceButtonImage[i]);
  }
}

static void ProcessTransactionString(wchar_t pString[], const size_t Length,
                                     const FinanceUnit *const f) {
  uint8_t code = f->ubCode;
  switch (code) {
    case DEPOSIT_FROM_SILVER_MINE:
      code = DEPOSIT_FROM_GOLD_MINE;
      /* FALLTHROUGH */

    case ACCRUED_INTEREST:
    case ANONYMOUS_DEPOSIT:
    case BOBBYR_PURCHASE:
    case DEPOSIT_FROM_GOLD_MINE:
    case IMP_PROFILE:
    case PAY_SPECK_FOR_MERC:
    case PURCHASED_FLOWERS:
    case TRANSACTION_FEE:
      wcsncpy(pString, pTransactionText[code], Length);
      break;

    case CANCELLED_INSURANCE:
    case EXTENDED_CONTRACT_BY_1_DAY:
    case EXTENDED_CONTRACT_BY_1_WEEK:
    case EXTENDED_CONTRACT_BY_2_WEEKS:
    case EXTENDED_INSURANCE:
    case FULL_MEDICAL_REFUND:
    case HIRED_MERC:
    case INSURANCE_PAYOUT:
    case MEDICAL_DEPOSIT:
    case MERC_DEPOSITED_MONEY_TO_PLAYER_ACCOUNT:
    case NO_MEDICAL_REFUND:
    case PARTIAL_MEDICAL_REFUND:
    case PAYMENT_TO_NPC:
    case PURCHASED_INSURANCE:
    case PURCHASED_ITEM_FROM_DEALER:
    case REDUCED_INSURANCE:
    case TRANSFER_FUNDS_FROM_MERC:
    case TRANSFER_FUNDS_TO_MERC:
      swprintf(pString, Length, pTransactionText[code], GetProfile(f->ubSecondCode).zNickname);
      break;

    case TRAIN_TOWN_MILITIA: {
      wchar_t str[128];
      const uint8_t ubSectorX = SECTORX(f->ubSecondCode);
      const uint8_t ubSectorY = SECTORY(f->ubSecondCode);
      GetSectorIDString(ubSectorX, ubSectorY, 0, str, lengthof(str), TRUE);
      swprintf(pString, Length, pTransactionText[TRAIN_TOWN_MILITIA], str);
      break;
    }
  }
}

static void DisplayFinancePageNumberAndDateRange() {
  SetFontAttributes(FINANCE_TEXT_FONT, FONT_BLACK, NO_SHADOW);
  mprintf(PAGE_NUMBER_X, PAGE_NUMBER_Y, L"%ls %d / %d", pFinanceHeaders[5], iCurrentPage + 1,
          guiLastPageInRecordsList + 2);
  SetFontShadow(DEFAULT_SHADOW);
}

static void WriteBalanceToDisk() {
  // will write the current balance to disk
  AutoSGPFile hFileHandle(FileMan::openForWriting(FINANCES_DATA_FILE));
  FileWrite(hFileHandle, &LaptopSaveInfo.iCurrentBalance, sizeof(int32_t));
}

static void GetBalanceFromDisk() {
  // will grab the current blanace from disk
  // assuming file already openned
  // this procedure will open and read in data to the finance list
  AutoSGPFile f;
  try {
    f = FileMan::openForReadingSmart(FINANCES_DATA_FILE, true);
  } catch (...) {
    LaptopSaveInfo.iCurrentBalance = 0;
    return; /* XXX TODO0019 ignore */
  }

  // get balance from disk first
  FileRead(f, &LaptopSaveInfo.iCurrentBalance, sizeof(int32_t));
}

// will write the current finance to disk
static void AppendFinanceToEndOfFile() {
  AutoSGPFile f(FileMan::openForAppend(FINANCES_DATA_FILE));

  const FinanceUnit *const fu = pFinanceListHead;
  uint8_t data[FINANCE_RECORD_SIZE];
  uint8_t *d = data;
  INJ_U8(d, fu->ubCode);
  INJ_U8(d, fu->ubSecondCode);
  INJ_U32(d, fu->uiDate);
  INJ_I32(d, fu->iAmount);
  INJ_I32(d, fu->iBalanceToDate);
  Assert(d == endof(data));

  FileWrite(f, data, sizeof(data));
}

// Grabs the size of the file and interprets number of pages it will take up
static void SetLastPageInRecords() {
  AutoSGPFile f(FileMan::openForReadingSmart(FINANCES_DATA_FILE, true));

  const uint32_t size = FileGetSize(f);

  if (size < FINANCE_HEADER_SIZE + FINANCE_RECORD_SIZE) {
    guiLastPageInRecordsList = 0;
    return;
  }

  guiLastPageInRecordsList = (size - FINANCE_HEADER_SIZE - FINANCE_RECORD_SIZE) /
                             (FINANCE_RECORD_SIZE * NUM_RECORDS_PER_PAGE);
}

static void LoadPreviousPage() {
  if (iCurrentPage == 0) return;
  LoadInRecords(iCurrentPage - 1);
}

static void LoadNextPage() {
  if (iCurrentPage > guiLastPageInRecordsList) return;
  LoadInRecords(iCurrentPage + 1);
}

// Loads in records belonging to page
static void LoadInRecords(uint32_t const page) {
  iCurrentPage = page;
  fReDrawScreenFlag = TRUE;
  SetFinanceButtonStates();
  ClearFinanceList();
  if (page == 0) return;  // check if bad page

  AutoSGPFile f(FileMan::openForReadingSmart(FINANCES_DATA_FILE, true));

  uint32_t const size = FileGetSize(f);
  if (size < FINANCE_HEADER_SIZE) return;

  uint32_t records = (size - FINANCE_HEADER_SIZE) / FINANCE_RECORD_SIZE;
  uint32_t const skip_records = NUM_RECORDS_PER_PAGE * (page - 1);
  if (records <= skip_records) return;

  records -= skip_records;
  FileSeek(f, FINANCE_HEADER_SIZE + FINANCE_RECORD_SIZE * skip_records, FILE_SEEK_FROM_START);

  if (records > NUM_RECORDS_PER_PAGE) records = NUM_RECORDS_PER_PAGE;
  for (; records > 0; --records) {
    uint8_t data[FINANCE_RECORD_SIZE];
    FileRead(f, data, sizeof(data));

    uint8_t code;
    uint8_t second_code;
    uint32_t date;
    int32_t amount;
    int32_t balance_to_date;
    const uint8_t *d = data;
    EXTR_U8(d, code);
    EXTR_U8(d, second_code);
    EXTR_U32(d, date);
    EXTR_I32(d, amount);
    EXTR_I32(d, balance_to_date);
    Assert(d == endof(data));

    ProcessAndEnterAFinacialRecord(code, date, amount, second_code, balance_to_date);
  }
}

static void InternalSPrintMoney(wchar_t *Str, int32_t Amount) {
  if (Amount == 0) {
    *Str++ = L'0';
    *Str = L'\0';
  } else {
    if (Amount < 0) {
      *Str++ = L'-';
      Amount = -Amount;
    }

    uint32_t Digits = 0;
    for (int32_t Tmp = Amount; Tmp != 0; Tmp /= 10) ++Digits;
    Str += Digits + (Digits - 1) / 3;
    *Str-- = L'\0';
    Digits = 0;
    do {
      if (Digits != 0 && Digits % 3 == 0) *Str-- = L',';
      ++Digits;
      *Str-- = L'0' + Amount % 10;
      Amount /= 10;
    } while (Amount != 0);
  }
}

void SPrintMoney(wchar_t *Str, int32_t Amount) {
  *Str++ = L'$';
  InternalSPrintMoney(Str, Amount);
}

static void SPrintMoneyNoDollarOnZero(wchar_t *Str, int32_t Amount) {
  if (Amount != 0) *Str++ = L'$';
  InternalSPrintMoney(Str, Amount);
}

// find out what today is, then go back 2 days, get balance for that day
static int32_t GetPreviousDaysBalance() {
  const uint32_t date_in_minutes = GetWorldTotalMin() - 60 * 24;
  const uint32_t date_in_days = date_in_minutes / (24 * 60);

  if (date_in_days < 2) return 0;

  AutoSGPFile f(FileMan::openForReadingSmart(FINANCES_DATA_FILE, true));

  int32_t balance = 0;
  // start at the end, move back until Date / 24 * 60 on the record equals
  // date_in_days - 2 loop, make sure we don't pass beginning of file, if so, we
  // have an error, and check for condifition above
  for (int32_t pos = FileGetSize(f); pos >= FINANCE_HEADER_SIZE + RECORD_SIZE;) {
    FileSeek(f, pos -= RECORD_SIZE, FILE_SEEK_FROM_START);

    uint8_t data[RECORD_SIZE];
    FileRead(f, data, sizeof(data));

    uint32_t date;
    int32_t balance_to_date;
    const uint8_t *d = data;
    EXTR_SKIP(d, 2);
    EXTR_U32(d, date);
    EXTR_SKIP(d, 4);
    EXTR_I32(d, balance_to_date);
    Assert(d == endof(data));

    // check to see if we are far enough
    if (date / (24 * 60) == date_in_days - 2) {
      balance = balance_to_date;
      break;
    }

    // there are no entries for the previous day
    if (date / (24 * 60) < date_in_days - 2) break;
  }

  return balance;
}

static int32_t GetTodaysBalance() {
  const uint32_t date_in_minutes = GetWorldTotalMin();
  const uint32_t date_in_days = date_in_minutes / (24 * 60);

  AutoSGPFile f(FileMan::openForReadingSmart(FINANCES_DATA_FILE, true));

  int32_t balance = 0;
  // loop, make sure we don't pass beginning of file, if so, we have an error,
  // and check for condifition above
  for (int32_t pos = FileGetSize(f); pos >= FINANCE_HEADER_SIZE + RECORD_SIZE;) {
    FileSeek(f, pos -= RECORD_SIZE, FILE_SEEK_FROM_START);

    uint8_t data[RECORD_SIZE];
    FileRead(f, data, sizeof(data));

    uint32_t date;
    int32_t balance_to_date;
    const uint8_t *d = data;
    EXTR_SKIP(d, 2);
    EXTR_U32(d, date);
    EXTR_SKIP(d, 4);
    EXTR_I32(d, balance_to_date);
    Assert(d == endof(data));

    // check to see if we are far enough
    if (date / (24 * 60) == date_in_days - 1) {
      balance = balance_to_date;
      break;
    }
  }

  return balance;
}

/* will return the income from the previous day, which is todays starting
 * balance - yesterdays starting balance */
static int32_t GetPreviousDaysIncome() {
  const uint32_t date_in_minutes = GetWorldTotalMin();
  const uint32_t date_in_days = date_in_minutes / (24 * 60);

  AutoSGPFile f(FileMan::openForReadingSmart(FINANCES_DATA_FILE, true));

  int32_t iTotalPreviousIncome = 0;
  // start at the end, move back until Date / 24 * 60 on the record is =
  // date_in_days - 2 loop, make sure we don't pass beginning of file, if so, we
  // have an error, and check for condifition above
  BOOLEAN fOkToIncrement = FALSE;
  for (int32_t pos = FileGetSize(f); pos >= FINANCE_HEADER_SIZE + RECORD_SIZE;) {
    FileSeek(f, pos -= RECORD_SIZE, FILE_SEEK_FROM_START);

    uint8_t data[RECORD_SIZE];
    FileRead(f, data, sizeof(data));

    uint8_t code;
    uint32_t date;
    int32_t amount;
    const uint8_t *d = data;
    EXTR_U8(d, code);
    EXTR_SKIP(d, 1);
    EXTR_U32(d, date);
    EXTR_I32(d, amount);
    EXTR_SKIP(d, 4);
    Assert(d == endof(data));

    // now ok to increment amount
    if (date / (24 * 60) == date_in_days - 1) fOkToIncrement = TRUE;

    if (fOkToIncrement && (code == DEPOSIT_FROM_GOLD_MINE || code == DEPOSIT_FROM_SILVER_MINE)) {
      // increment total
      iTotalPreviousIncome += amount;
    }

    // check to see if we are far enough
    if (date / (24 * 60) <= date_in_days - 2) break;
  }

  return iTotalPreviousIncome;
}

static int32_t GetTodaysDaysIncome() {
  const uint32_t date_in_minutes = GetWorldTotalMin();
  const uint32_t date_in_days = date_in_minutes / (24 * 60);

  AutoSGPFile f(FileMan::openForReadingSmart(FINANCES_DATA_FILE, true));

  int32_t iTotalIncome = 0;
  // loop, make sure we don't pass beginning of file, if so, we have an error,
  // and check for condifition above
  BOOLEAN fOkToIncrement = FALSE;
  for (int32_t pos = FileGetSize(f); pos >= FINANCE_HEADER_SIZE + RECORD_SIZE;) {
    FileSeek(f, pos -= RECORD_SIZE, FILE_SEEK_FROM_START);

    uint8_t data[RECORD_SIZE];
    FileRead(f, data, sizeof(data));

    uint8_t code;
    uint32_t date;
    int32_t amount;
    const uint8_t *d = data;
    EXTR_U8(d, code);
    EXTR_SKIP(d, 1);
    EXTR_U32(d, date);
    EXTR_I32(d, amount);
    EXTR_SKIP(d, 4);
    Assert(d == endof(data));

    // now ok to increment amount
    if (date / (24 * 60) > date_in_days - 1) fOkToIncrement = TRUE;

    if (fOkToIncrement && (code == DEPOSIT_FROM_GOLD_MINE || code == DEPOSIT_FROM_SILVER_MINE)) {
      // increment total
      iTotalIncome += amount;
      fOkToIncrement = FALSE;
    }

    // check to see if we are far enough
    if (date / (24 * 60) == date_in_days - 1) break;
  }

  return iTotalIncome;
}

static void SetFinanceButtonStates() {
  // this function will look at what page we are viewing, enable and disable
  // buttons as needed

  bool const has_prev = iCurrentPage != 0;
  EnableButton(giFinanceButton[PREV_PAGE_BUTTON], has_prev);
  EnableButton(giFinanceButton[FIRST_PAGE_BUTTON], has_prev);

  bool const has_next = iCurrentPage <= guiLastPageInRecordsList;
  EnableButton(giFinanceButton[NEXT_PAGE_BUTTON], has_next);
  EnableButton(giFinanceButton[LAST_PAGE_BUTTON], has_next);
}

// grab todays other deposits
static int32_t GetTodaysOtherDeposits() {
  const uint32_t date_in_minutes = GetWorldTotalMin();
  const uint32_t date_in_days = date_in_minutes / (24 * 60);

  AutoSGPFile f(FileMan::openForReadingSmart(FINANCES_DATA_FILE, true));

  int32_t iTotalIncome = 0;
  // loop, make sure we don't pass beginning of file, if so, we have an error,
  // and check for condifition above
  BOOLEAN fOkToIncrement = FALSE;
  for (int32_t pos = FileGetSize(f); pos >= FINANCE_HEADER_SIZE + RECORD_SIZE;) {
    FileSeek(f, pos -= RECORD_SIZE, FILE_SEEK_FROM_START);

    uint8_t data[RECORD_SIZE];
    FileRead(f, data, sizeof(data));

    uint8_t code;
    uint32_t date;
    int32_t amount;
    const uint8_t *d = data;
    EXTR_U8(d, code);
    EXTR_SKIP(d, 1);
    EXTR_U32(d, date);
    EXTR_I32(d, amount);
    EXTR_SKIP(d, 4);
    Assert(d == endof(data));

    // now ok to increment amount
    if (date / (24 * 60) > date_in_days - 1) fOkToIncrement = TRUE;

    if (fOkToIncrement && (code != DEPOSIT_FROM_GOLD_MINE && code != DEPOSIT_FROM_SILVER_MINE) &&
        amount > 0) {
      // increment total
      iTotalIncome += amount;
      fOkToIncrement = FALSE;
    }

    // check to see if we are far enough
    if (date / (24 * 60) == date_in_days - 1) break;
  }

  return iTotalIncome;
}

static int32_t GetYesterdaysOtherDeposits() {
  const uint32_t iDateInMinutes = GetWorldTotalMin();
  const uint32_t date_in_days = iDateInMinutes / (24 * 60);

  AutoSGPFile f(FileMan::openForReadingSmart(FINANCES_DATA_FILE, true));

  int32_t iTotalPreviousIncome = 0;
  // start at the end, move back until Date / 24 * 60 on the record is =
  // date_in_days - 2 loop, make sure we don't pass beginning of file, if so, we
  // have an error, and check for condifition above
  BOOLEAN fOkToIncrement = FALSE;
  for (int32_t pos = FileGetSize(f); pos >= FINANCE_HEADER_SIZE + RECORD_SIZE;) {
    FileSeek(f, pos -= RECORD_SIZE, FILE_SEEK_FROM_START);

    uint8_t data[RECORD_SIZE];
    FileRead(f, data, sizeof(data));

    uint8_t code;
    uint32_t date;
    int32_t amount;
    const uint8_t *d = data;
    EXTR_U8(d, code);
    EXTR_SKIP(d, 1);
    EXTR_U32(d, date);
    EXTR_I32(d, amount);
    EXTR_SKIP(d, 4);
    Assert(d == endof(data));

    // now ok to increment amount
    if (date / (24 * 60) == date_in_days - 1) fOkToIncrement = TRUE;

    if (fOkToIncrement && (code != DEPOSIT_FROM_GOLD_MINE && code != DEPOSIT_FROM_SILVER_MINE) &&
        amount > 0) {
      // increment total
      iTotalPreviousIncome += amount;
    }

    // check to see if we are far enough
    if (date / (24 * 60) <= date_in_days - 2) break;
  }

  return iTotalPreviousIncome;
}

static int32_t GetTodaysDebits() {
  // return the expenses for today

  // currentbalance - todays balance - Todays income - other deposits

  return (GetCurrentBalance() - GetTodaysBalance() - GetTodaysDaysIncome() -
          GetTodaysOtherDeposits());
}

static int32_t GetYesterdaysDebits() {
  // return the expenses for yesterday

  return (GetTodaysBalance() - GetPreviousDaysBalance() - GetPreviousDaysIncome() -
          GetYesterdaysOtherDeposits());
}
