#include "Laptop/Files.h"

#include "Directories.h"
#include "Laptop/Laptop.h"
#include "MercPortrait.h"
#include "SGP/ButtonSystem.h"
#include "SGP/Debug.h"
#include "SGP/FileMan.h"
#include "SGP/Font.h"
#include "SGP/HImage.h"
#include "SGP/LoadSaveData.h"
#include "SGP/MemMan.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "Strategic/GameClock.h"
#include "Tactical/SoldierControl.h"
#include "Tactical/SoldierProfile.h"
#include "TileEngine/RenderDirty.h"
#include "Utils/Cursors.h"
#include "Utils/EncryptedFile.h"
#include "Utils/FontControl.h"
#include "Utils/Text.h"
#include "Utils/WordWrap.h"

struct FilesUnit {
  uint8_t ubCode;  // the code index in the files code table
  BOOLEAN fRead;
  FilesUnit *Next;  // next unit in the list
};

struct FileString {
  wchar_t *pString;
  FileString *Next;
};

struct FileRecordWidth {
  int32_t iRecordNumber;
  int32_t iRecordWidth;
  int32_t iRecordHeightAdjustment;
  uint8_t ubFlags;
  FileRecordWidth *Next;
};

enum {
  ENRICO_BACKGROUND = 0,
  SLAY_BACKGROUND,
  MATRON_BACKGROUND,
  IMPOSTER_BACKGROUND,
  TIFFANY_BACKGROUND,
  REXALL_BACKGROUND,
  ELGIN_BACKGROUND
};

#define TOP_X 0 + LAPTOP_SCREEN_UL_X
#define TOP_Y LAPTOP_SCREEN_UL_Y
#define TITLE_X 140
#define TITLE_Y 33
#define FILES_TITLE_FONT FONT14ARIAL
#define FILES_TEXT_FONT FONT10ARIAL  // FONT12ARIAL
#define FILES_SENDER_TEXT_X (FILES_LIST_X + 5)
#define MAX_FILES_LIST_LENGTH 28
#define FILE_VIEWER_X 236
#define FILE_VIEWER_Y 81
#define FILE_VIEWER_W 364
#define FILE_VIEWER_H 353
#define FILE_GAP 2
#define FILE_TEXT_COLOR FONT_BLACK
#define FILE_STRING_SIZE 400
#define MAX_FILES_PAGE MAX_FILES_LIST_LENGTH
#define FILES_LIST_X (TOP_X + 10)
#define FILES_LIST_Y 85
#define FILES_LIST_W 107
#define FILES_LIST_H 12
#define LENGTH_OF_ENRICO_FILE 68
#define MAX_FILE_MESSAGE_PAGE_SIZE 325
#define PREVIOUS_FILE_PAGE_BUTTON_X 553
#define PREVIOUS_FILE_PAGE_BUTTON_Y 53
#define NEXT_FILE_PAGE_BUTTON_X 577
#define NEXT_FILE_PAGE_BUTTON_Y PREVIOUS_FILE_PAGE_BUTTON_Y

#define FILES_COUNTER_1_WIDTH 7
#define FILES_COUNTER_2_WIDTH 43
#define FILES_COUNTER_3_WIDTH 45

// the highlighted line
static int32_t iHighLightFileLine = -1;

// the files record list
static FilesUnit *pFilesListHead = NULL;

// are we in files mode
static BOOLEAN fInFilesMode = FALSE;
static BOOLEAN fOnLastFilesPageFlag = FALSE;

//. did we enter due to new file icon?
BOOLEAN fEnteredFileViewerFromNewFileIcon = FALSE;
static BOOLEAN fWaitAFrame = FALSE;

// are there any new files
BOOLEAN fNewFilesInFileViewer = FALSE;

// graphics handles
static SGPVObject *guiTITLE;
static SGPVObject *guiTOP;

// currewnt page of multipage files we are on
static int32_t giFilesPage = 0;
// strings

#define SLAY_LENGTH 12

struct FileInfo {
  uint16_t profile_id;
  uint16_t file_offset;
};

static FileInfo const g_file_info[] = {
    {NO_PROFILE, 0},
    {SLAY, 0},                   // Slay
    {ANNIE, SLAY_LENGTH * 1},    // MOM
    {CHRIS, SLAY_LENGTH * 2},    // Imposter
    {TIFFANY, SLAY_LENGTH * 3},  // Tiff
    {T_REX, SLAY_LENGTH * 4},    // T-Rex
    {DRUGGIST, SLAY_LENGTH * 5}  // Elgin
};

// buttons for next and previous pages
static GUIButtonRef giFilesPageButtons[2];
static MOUSE_REGION g_scroll_region;

// the previous and next pages buttons

enum {
  PREVIOUS_FILES_PAGE_BUTTON = 0,
  NEXT_FILES_PAGE_BUTTON,
};
// mouse regions
static MOUSE_REGION pFilesRegions[MAX_FILES_PAGE];

static void CheckForUnreadFiles();
static void OpenAndReadFilesFile();
static void OpenAndWriteFilesFile();
static void ProcessAndEnterAFilesRecord(uint8_t ubCode, BOOLEAN fRead);

static void AddFilesToPlayersLog(uint8_t ubCode) {
  // adds Files item to player's log(Files List)
  // outside of the Files system(the code in this .c file), this is the only
  // function you'll ever need

  // if not in Files mode, read in from file
  if (!fInFilesMode) OpenAndReadFilesFile();

  // process the actual data
  ProcessAndEnterAFilesRecord(ubCode, FALSE);

  // set unread flag, if nessacary
  CheckForUnreadFiles();

  // write out to file if not in Files mode
  if (!fInFilesMode) OpenAndWriteFilesFile();
}

static void ClearFilesList();

void GameInitFiles() {
  FileDelete(FILES_DAT_FILE);
  ClearFilesList();

  // add background check by RIS
  AddFilesToPlayersLog(ENRICO_BACKGROUND);
}

static void CreateButtonsForFilesPage();
static void HandleFileViewerButtonStates();
static void InitializeFilesMouseRegions();
static void LoadFiles();
static void OpenFirstUnreadFile();

void EnterFiles() {
  // load grpahics for files system
  LoadFiles();

  // in files mode now, set the fact
  fInFilesMode = TRUE;

  // initialize mouse regions
  InitializeFilesMouseRegions();

  // create buttons
  CreateButtonsForFilesPage();

  // now set start states
  HandleFileViewerButtonStates();

  // build files list
  OpenAndReadFilesFile();

  // render files system
  RenderFiles();

  // entered due to icon
  if (fEnteredFileViewerFromNewFileIcon) {
    OpenFirstUnreadFile();
    fEnteredFileViewerFromNewFileIcon = FALSE;
  }
}

static void DeleteButtonsForFilesPage();
static void RemoveFiles();
static void RemoveFilesMouseRegions();

void ExitFiles() {
  // write files list out to disk
  OpenAndWriteFilesFile();

  // remove mouse regions
  RemoveFilesMouseRegions();

  // delete buttons
  DeleteButtonsForFilesPage();

  fInFilesMode = FALSE;

  // remove files
  RemoveFiles();
}

void HandleFiles() { CheckForUnreadFiles(); }

static void DisplayFileMessage();
static void DisplayFilesList();
static void DrawFilesTitleText();
static void RenderFilesBackGround();

void RenderFiles() {
  // render the background
  RenderFilesBackGround();

  // draw the title bars text
  DrawFilesTitleText();

  // display the list of senders
  DisplayFilesList();

  // draw the highlighted file
  DisplayFileMessage();

  // title bar icon
  BlitTitleBarIcons();

  BltVideoObject(FRAME_BUFFER, guiLaptopBACKGROUND, 0, 108, 23);
}

static void RenderFilesBackGround() {
  // render generic background for file system
  BltVideoObject(FRAME_BUFFER, guiTITLE, 0, TOP_X, TOP_Y - 2);
  BltVideoObject(FRAME_BUFFER, guiTOP, 0, TOP_X, TOP_Y + 22);
}

static void DrawFilesTitleText() {
  // draw the pages title
  SetFontAttributes(FILES_TITLE_FONT, FONT_WHITE);
  MPrint(TITLE_X, TITLE_Y, pFilesTitle);
}

static void LoadFiles() {
  // load files video objects into memory

  // title bar
  guiTITLE = AddVideoObjectFromFile(LAPTOPDIR "/programtitlebar.sti");

  // top portion of the screen background
  guiTOP = AddVideoObjectFromFile(LAPTOPDIR "/fileviewer.sti");
}

static void RemoveFiles() {
  // delete files video objects from memory
  DeleteVideoObject(guiTOP);
  DeleteVideoObject(guiTITLE);
}

static void ProcessAndEnterAFilesRecord(const uint8_t ubCode, const BOOLEAN fRead) {
  // Append node to list
  FilesUnit **anchor;
  for (anchor = &pFilesListHead; *anchor != NULL; anchor = &(*anchor)->Next) {
    // Check if the file is already there
    if ((*anchor)->ubCode == ubCode) return;
  }

  FilesUnit *const f = MALLOC(FilesUnit);
  f->Next = NULL;
  f->ubCode = ubCode;
  f->fRead = fRead;

  *anchor = f;
}

#define FILE_ENTRY_SIZE 263

static void OpenAndReadFilesFile() {
  ClearFilesList();

  AutoSGPFile f;
  try {
    f = FileMan::openForReadingSmart(FILES_DAT_FILE, true);
  } catch (...) {
    return; /* XXX TODO0019 ignore */
  }

  // file exists, read in data, continue until file end
  for (uint32_t i = FileGetSize(f) / FILE_ENTRY_SIZE; i != 0; --i) {
    uint8_t data[FILE_ENTRY_SIZE];
    FileRead(f, data, sizeof(data));

    uint8_t code;
    uint8_t already_read;

    const uint8_t *d = data;
    EXTR_U8(d, code)
    EXTR_SKIP(d, 261)
    EXTR_U8(d, already_read)
    Assert(d == endof(data));

    ProcessAndEnterAFilesRecord(code, already_read);
  }
}

static void OpenAndWriteFilesFile() {
  AutoSGPFile f(FileMan::openForWriting(FILES_DAT_FILE));

  for (const FilesUnit *i = pFilesListHead; i; i = i->Next) {
    uint8_t data[FILE_ENTRY_SIZE];
    uint8_t *d = data;
    INJ_U8(d, i->ubCode)
    INJ_SKIP(d, 261)
    INJ_U8(d, i->fRead)
    Assert(d == endof(data));

    FileWrite(f, data, sizeof(data));
  }

  ClearFilesList();
}

static void ClearFilesList() {
  FilesUnit *i = pFilesListHead;
  pFilesListHead = NULL;
  while (i) {
    FilesUnit *const del = i;
    i = i->Next;
    MemFree(del);
  }
}

static void DisplayFilesList() {
  // this function will run through the list of files of files and display the
  // 'sender'
  SetFontAttributes(FILES_TEXT_FONT, FONT_BLACK, NO_SHADOW);

  int32_t i = 0;
  int32_t const x = FILES_LIST_X;
  int32_t y = FILES_LIST_Y;
  int32_t const w = FILES_LIST_W;
  int32_t const h = FILES_LIST_H;
  for (FilesUnit const *fu = pFilesListHead; fu; ++i, fu = fu->Next) {
    if (i == iHighLightFileLine) {
      uint16_t const colour = Get16BPPColor(FROMRGB(240, 240, 200));
      ColorFillVideoSurfaceArea(FRAME_BUFFER, x, y, x + w, y + h, colour);
    }
    MPrint(FILES_SENDER_TEXT_X, y + 2, pFilesSenderList[fu->ubCode]);
    y += h;
  }

  SetFontShadow(DEFAULT_SHADOW);
}

static void DisplayFormattedText();

static void DisplayFileMessage() {
  if (iHighLightFileLine != -1) {
    DisplayFormattedText();
  } else {
    HandleFileViewerButtonStates();
  }
}

static void FilesBtnCallBack(MOUSE_REGION *pRegion, int32_t iReason);

static void InitializeFilesMouseRegions() {
  uint16_t const x = FILES_LIST_X;
  uint16_t y = FILES_LIST_Y;
  uint16_t const w = FILES_LIST_W;
  uint16_t const h = FILES_LIST_H;
  for (int32_t i = 0; i != MAX_FILES_PAGE; ++i) {
    MOUSE_REGION *const r = &pFilesRegions[i];
    MSYS_DefineRegion(r, x, y, x + w - 1, y + h - 1, MSYS_PRIORITY_NORMAL + 2, MSYS_NO_CURSOR,
                      MSYS_NO_CALLBACK, FilesBtnCallBack);
    y += h;
    MSYS_SetRegionUserData(r, 0, i);
  }
}

static void RemoveFilesMouseRegions() {
  FOR_EACH(MOUSE_REGION, i, pFilesRegions) MSYS_RemoveRegion(&*i);
}

static void FilesBtnCallBack(MOUSE_REGION *pRegion, int32_t iReason) {
  if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    FilesUnit *pFilesList = pFilesListHead;
    int32_t iFileId = MSYS_GetRegionUserData(pRegion, 0);
    int32_t iCounter = 0;

    // reset iHighLightListLine
    iHighLightFileLine = -1;

    if (iHighLightFileLine == iFileId) return;

    // make sure is a valid
    while (pFilesList != NULL) {
      if (iCounter == iFileId) {
        giFilesPage = 0;
        iHighLightFileLine = iFileId;
      }

      pFilesList = pFilesList->Next;
      iCounter++;
    }
    fReDrawScreenFlag = TRUE;
  }
}

static void HandleSpecialFiles();
static void HandleSpecialTerroristFile(int32_t file_idx);

static void DisplayFormattedText() {
  fWaitAFrame = FALSE;

  uint16_t const white = Get16BPPColor(FROMRGB(255, 255, 255));
  int32_t const x = FILE_VIEWER_X;
  int32_t const y = FILE_VIEWER_Y;
  ColorFillVideoSurfaceArea(FRAME_BUFFER, x, y, x + FILE_VIEWER_W, y + FILE_VIEWER_H, white);

  // Get the file that was highlighted
  FilesUnit *fu = pFilesListHead;
  for (int32_t n = iHighLightFileLine; n != 0; --n) {
    fu = fu->Next;
  }

  fu->fRead = TRUE;

  switch (fu->ubCode) {
    case ENRICO_BACKGROUND:
      HandleSpecialFiles();
      break;
    default:
      HandleSpecialTerroristFile(fu->ubCode);
      break;
  }

  HandleFileViewerButtonStates();
  SetFontShadow(DEFAULT_SHADOW);
}

static FileString const *GetFirstStringOnThisPage(FileString const *RecordList, Font const font,
                                                  uint16_t usWidth, uint8_t ubGap, int32_t iPage,
                                                  int32_t iPageSize, FileRecordWidth *WidthList) {
  // get the first record on this page - build pages up until this point
  FileString const *CurrentRecord = NULL;

  int32_t iCurrentPositionOnThisPage = 0;
  int32_t iCurrentPage = 0;
  int32_t iCounter = 0;
  FileRecordWidth *pWidthList = WidthList;
  uint16_t usCurrentWidth = usWidth;

  // null record list, nothing to do
  if (RecordList == NULL) {
    return (CurrentRecord);
  }

  CurrentRecord = RecordList;

  // while we are not on the current page
  while (iCurrentPage < iPage) {
    usCurrentWidth = usWidth;
    pWidthList = WidthList;

    while (pWidthList) {
      if (iCounter == pWidthList->iRecordNumber) {
        usCurrentWidth = (int16_t)pWidthList->iRecordWidth;
        //				iCurrentPositionOnThisPage +=
        // pWidthList->iRecordHeightAdjustment;

        if (pWidthList->iRecordHeightAdjustment == iPageSize) {
          if (iCurrentPositionOnThisPage != 0)
            iCurrentPositionOnThisPage += iPageSize - iCurrentPositionOnThisPage;
        } else
          iCurrentPositionOnThisPage += pWidthList->iRecordHeightAdjustment;
      }
      pWidthList = pWidthList->Next;
    }

    // build record list to this point
    for (;;) {
      uint16_t const h =
          IanWrappedStringHeight(usCurrentWidth, ubGap, font, CurrentRecord->pString);
      if (iCurrentPositionOnThisPage + h >= iPageSize) break;

      // still room on this page
      iCurrentPositionOnThisPage += h;

      // next record
      CurrentRecord = CurrentRecord->Next;
      iCounter++;

      usCurrentWidth = usWidth;
      pWidthList = WidthList;
      while (pWidthList) {
        if (iCounter == pWidthList->iRecordNumber) {
          usCurrentWidth = (int16_t)pWidthList->iRecordWidth;

          if (pWidthList->iRecordHeightAdjustment == iPageSize) {
            if (iCurrentPositionOnThisPage != 0)
              iCurrentPositionOnThisPage += iPageSize - iCurrentPositionOnThisPage;
          } else
            iCurrentPositionOnThisPage += pWidthList->iRecordHeightAdjustment;
        }
        pWidthList = pWidthList->Next;
      }
    }

    // reset position
    iCurrentPositionOnThisPage = 0;

    // next page
    iCurrentPage++;
    //		iCounter++;
  }

  return (CurrentRecord);
}

static FileString *LoadStringsIntoFileList(char const *const filename, uint32_t offset, size_t n) {
  FileString *head = 0;
  FileString **anchor = &head;
  AutoSGPFile f(FileMan::openForReadingSmart(filename, true));
  for (; n != 0; ++offset, --n) {
    wchar_t str[FILE_STRING_SIZE];
    LoadEncryptedData(f, str, lengthof(str) * offset, lengthof(str));

    FileString *const fs = MALLOC(FileString);
    fs->Next = 0;
    fs->pString = MALLOCN(wchar_t, wcslen(str) + 1);
    wcscpy(fs->pString, str);

    // Append node to list
    *anchor = fs;
    anchor = &fs->Next;
  }
  return head;
}

namespace {
void ClearFileStringList(FileString *i) {
  while (i) {
    FileString *const del = i;
    i = i->Next;
    MemFree(del->pString);
    MemFree(del);
  }
}

typedef SGP::AutoObj<FileString, ClearFileStringList> AutoStringList;
}  // namespace

static void ClearOutWidthRecordsList(FileRecordWidth *pFileRecordWidthList);
static FileRecordWidth *CreateWidthRecordsForAruloIntelFile();

static void HandleSpecialFiles() {
  FileRecordWidth *const width_list = CreateWidthRecordsForAruloIntelFile();
  AutoStringList const head(
      LoadStringsIntoFileList(BINARYDATADIR "/ris.edt", 0, LENGTH_OF_ENRICO_FILE));
  FileString const *const start = GetFirstStringOnThisPage(
      head, FILES_TEXT_FONT, 350, FILE_GAP, giFilesPage, MAX_FILE_MESSAGE_PAGE_SIZE, width_list);
  ClearOutWidthRecordsList(width_list);

  // Find out where this string is
  FileString const *i = head;
  int32_t clause = 0;
  for (; i != start; i = i->Next) {
    ++clause;
  }

  // Move through list and display
  for (int32_t y = 0; i; clause++, i = i->Next) {
    Font const font = giFilesPage == 0 && clause == 0 ? FILES_TITLE_FONT : FILES_TEXT_FONT;

    /* Based on the record we are at, selected X start position and the width to
     * wrap the line, to fit around pictures */
    int32_t max_width = 350;
    int32_t start_x = FILE_VIEWER_X + 10;
    switch (clause) {
      case FILES_COUNTER_1_WIDTH:
        if (giFilesPage == 0) y = MAX_FILE_MESSAGE_PAGE_SIZE;
        break;

      case FILES_COUNTER_2_WIDTH:
      case FILES_COUNTER_3_WIDTH:
        max_width = 200;
        start_x = FILE_VIEWER_X + 150;
        break;
    }

    wchar_t const *const txt = i->pString;
    if (y + IanWrappedStringHeight(max_width, FILE_GAP, font, txt) >= MAX_FILE_MESSAGE_PAGE_SIZE) {
      // gonna get cut off, end now
      break;
    }

    y += IanDisplayWrappedString(start_x, FILE_VIEWER_Y + 4 + y, max_width, FILE_GAP, font,
                                 FILE_TEXT_COLOR, txt, 0, IAN_WRAP_NO_SHADOW);
  }
  fOnLastFilesPageFlag = !i;

  // place pictures
  switch (giFilesPage) {
    case 0:
      BltVideoObjectOnce(FRAME_BUFFER, LAPTOPDIR "/arucofilesmap.sti", 0, 300, 270);
      break;  // Picture of country
    case 4:
      BltVideoObjectOnce(FRAME_BUFFER, LAPTOPDIR "/enrico_y.sti", 0, 260, 225);
      break;  // Kid pic
    case 5:
      BltVideoObjectOnce(FRAME_BUFFER, LAPTOPDIR "/enrico_w.sti", 0, 260, 85);
      break;  // Wedding pic
  }
}

static void LoadPreviousPage() {
  if (fWaitAFrame) return;
  if (giFilesPage == 0) return;

  --giFilesPage;
  fWaitAFrame = TRUE;
  fReDrawScreenFlag = TRUE;
  MarkButtonsDirty();
}

static void LoadNextPage() {
  if (fWaitAFrame) return;
  if (fOnLastFilesPageFlag) return;

  ++giFilesPage;
  fWaitAFrame = TRUE;
  fReDrawScreenFlag = TRUE;
  MarkButtonsDirty();
}

static void ScrollRegionCallback(MOUSE_REGION *const, int32_t const reason) {
  if (reason & MSYS_CALLBACK_REASON_WHEEL_UP) {
    LoadPreviousPage();
  } else if (reason & MSYS_CALLBACK_REASON_WHEEL_DOWN) {
    LoadNextPage();
  }
}

static void BtnNextFilePageCallback(GUI_BUTTON *btn, int32_t reason);
static void BtnPreviousFilePageCallback(GUI_BUTTON *btn, int32_t reason);

static void CreateButtonsForFilesPage() {
  // will create buttons for the files page
  giFilesPageButtons[0] = QuickCreateButtonImg(
      LAPTOPDIR "/arrows.sti", 0, 1, PREVIOUS_FILE_PAGE_BUTTON_X, PREVIOUS_FILE_PAGE_BUTTON_Y,
      MSYS_PRIORITY_HIGHEST - 1, BtnPreviousFilePageCallback);
  giFilesPageButtons[1] = QuickCreateButtonImg(LAPTOPDIR "/arrows.sti", 6, 7,
                                               NEXT_FILE_PAGE_BUTTON_X, NEXT_FILE_PAGE_BUTTON_Y,
                                               MSYS_PRIORITY_HIGHEST - 1, BtnNextFilePageCallback);

  giFilesPageButtons[0]->SetCursor(CURSOR_LAPTOP_SCREEN);
  giFilesPageButtons[1]->SetCursor(CURSOR_LAPTOP_SCREEN);

  uint16_t const x = FILE_VIEWER_X;
  uint16_t const y = FILE_VIEWER_Y;
  MSYS_DefineRegion(&g_scroll_region, x, y, x + FILE_VIEWER_W - 1, y + FILE_VIEWER_H - 1,
                    MSYS_PRIORITY_HIGH, MSYS_NO_CURSOR, MSYS_NO_CALLBACK, ScrollRegionCallback);
}

static void DeleteButtonsForFilesPage() {
  // destroy buttons for the files page
  MSYS_RemoveRegion(&g_scroll_region);
  RemoveButton(giFilesPageButtons[0]);
  RemoveButton(giFilesPageButtons[1]);
}

static void BtnPreviousFilePageCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    LoadPreviousPage();
  }
}

static void BtnNextFilePageCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    LoadNextPage();
  }
}

static void HandleFileViewerButtonStates() {
  if (iHighLightFileLine == -1) {
    // not displaying message, leave
    DisableButton(giFilesPageButtons[0]);
    DisableButton(giFilesPageButtons[1]);
    return;
  }

  // Turn on/off previous page button
  EnableButton(giFilesPageButtons[0], giFilesPage != 0);
  // Turn on/off next page button
  EnableButton(giFilesPageButtons[1], !fOnLastFilesPageFlag);
}

static FileRecordWidth *CreateRecordWidth(int32_t iRecordNumber, int32_t iRecordWidth,
                                          int32_t iRecordHeightAdjustment, uint8_t ubFlags) {
  // allocs and inits a width info record for the multipage file viewer...this
  // will tell the procedure that does inital computation on which record is the
  // start of the current page how wide special records are ( ones that share
  // space with pictures )
  FileRecordWidth *const pTempRecord = MALLOC(FileRecordWidth);
  pTempRecord->Next = NULL;
  pTempRecord->iRecordNumber = iRecordNumber;
  pTempRecord->iRecordWidth = iRecordWidth;
  pTempRecord->iRecordHeightAdjustment = iRecordHeightAdjustment;
  pTempRecord->ubFlags = ubFlags;

  return (pTempRecord);
}

static FileRecordWidth *CreateWidthRecordsForAruloIntelFile() {
  // this fucntion will create the width list for the Arulco intelligence file
  FileRecordWidth *pTempRecord = NULL;
  FileRecordWidth *pRecordListHead = NULL;

  // first record width
  //	pTempRecord = CreateRecordWidth( 7, 350, 200,0 );
  pTempRecord = CreateRecordWidth(FILES_COUNTER_1_WIDTH, 350, MAX_FILE_MESSAGE_PAGE_SIZE, 0);

  // set up head of list now
  pRecordListHead = pTempRecord;

  // next record
  //	pTempRecord -> Next = CreateRecordWidth( 43, 200,0, 0 );
  pTempRecord->Next = CreateRecordWidth(FILES_COUNTER_2_WIDTH, 200, 0, 0);
  pTempRecord = pTempRecord->Next;

  // and the next..
  //	pTempRecord -> Next = CreateRecordWidth( 45, 200,0, 0 );
  pTempRecord->Next = CreateRecordWidth(FILES_COUNTER_3_WIDTH, 200, 0, 0);
  pTempRecord = pTempRecord->Next;

  return (pRecordListHead);
}

static FileRecordWidth *CreateWidthRecordsForTerroristFile() {
  // this fucntion will create the width list for the Arulco intelligence file
  FileRecordWidth *pTempRecord = NULL;
  FileRecordWidth *pRecordListHead = NULL;

  // first record width
  pTempRecord = CreateRecordWidth(4, 170, 0, 0);

  // set up head of list now
  pRecordListHead = pTempRecord;

  // next record
  pTempRecord->Next = CreateRecordWidth(5, 170, 0, 0);
  pTempRecord = pTempRecord->Next;

  pTempRecord->Next = CreateRecordWidth(6, 170, 0, 0);
  pTempRecord = pTempRecord->Next;

  return (pRecordListHead);
}

static void ClearOutWidthRecordsList(FileRecordWidth *i) {
  while (i) {
    FileRecordWidth *const del = i;
    i = i->Next;
    MemFree(del);
  }
}

// open new files for viewing
static void OpenFirstUnreadFile() {
  // open the first unread file in the list
  int32_t i = 0;
  for (FilesUnit *fu = pFilesListHead; fu; ++i, fu = fu->Next) {
    if (fu->fRead) continue;
    iHighLightFileLine = i;
  }
}

static void CheckForUnreadFiles() {
  // will check for any unread files and set flag if any
  BOOLEAN any_unread = FALSE;
  for (FilesUnit const *i = pFilesListHead; i; i = i->Next) {
    if (i->fRead) continue;
    any_unread = TRUE;
    break;
  }

  /* If the old flag and the new flag aren't the same, either create or destory
   * the fast help region */
  if (fNewFilesInFileViewer == any_unread) return;

  fNewFilesInFileViewer = any_unread;
  CreateFileAndNewEmailIconFastHelpText(LAPTOP_BN_HLP_TXT_YOU_HAVE_NEW_FILE,
                                        !fNewFilesInFileViewer);
}

static void HandleSpecialTerroristFile(int32_t const file_idx) {
  FileRecordWidth *const width_list = CreateWidthRecordsForTerroristFile();
  FileInfo const &info = g_file_info[file_idx];
  AutoStringList const head(
      LoadStringsIntoFileList(BINARYDATADIR "/files.edt", info.file_offset, SLAY_LENGTH));
  FileString const *const start = GetFirstStringOnThisPage(
      head, FILES_TEXT_FONT, 350, FILE_GAP, giFilesPage, MAX_FILE_MESSAGE_PAGE_SIZE, width_list);
  ClearOutWidthRecordsList(width_list);

  // Find out where this string is
  FileString const *i = head;
  int32_t clause = 0;
  for (; i != start; i = i->Next) {
    ++clause;
  }

  // Move through list and display
  for (int32_t y = 0; i; ++clause, i = i->Next) {
    // Show picture
    if (giFilesPage == 0 && clause == 4) {
      AutoSGPVObject vo(LoadBigPortrait(GetProfile(info.profile_id)));
      BltVideoObject(FRAME_BUFFER, vo, 0, FILE_VIEWER_X + 30, FILE_VIEWER_Y + 136);
      BltVideoObjectOnce(FRAME_BUFFER, LAPTOPDIR "/interceptborder.sti", 0, FILE_VIEWER_X + 25,
                         FILE_VIEWER_Y + 131);
    }

    Font const font = giFilesPage == 0 && clause == 0 ? FILES_TITLE_FONT : FILES_TEXT_FONT;

    /* Based on the record we are at, selected X start position and the width to
     * wrap the line, to fit around pictures */
    int32_t max_width;
    int32_t start_x;
    if (4 <= clause && clause < 7) {
      max_width = 170;
      start_x = FILE_VIEWER_X + 180;
    } else {
      max_width = 350;
      start_x = FILE_VIEWER_X + 10;
    }

    wchar_t const *const txt = i->pString;
    if (y + IanWrappedStringHeight(max_width, FILE_GAP, font, txt) >= MAX_FILE_MESSAGE_PAGE_SIZE) {
      // gonna get cut off, end now
      break;
    }

    y += IanDisplayWrappedString(start_x, FILE_VIEWER_Y + 4 + y, max_width, FILE_GAP, font,
                                 FILE_TEXT_COLOR, txt, 0, IAN_WRAP_NO_SHADOW);
  }
  fOnLastFilesPageFlag = !i;
}

void AddFilesAboutTerrorists() {
  for (int32_t i = 1; i != lengthof(g_file_info); ++i) {
    AddFilesToPlayersLog(i);
  }
}
