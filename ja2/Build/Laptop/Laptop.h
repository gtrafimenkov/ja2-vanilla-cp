// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef LAPTOP_H
#define LAPTOP_H

#include "MessageBoxScreen.h"
#include "SGP/MouseSystem.h"
#include "SGP/Types.h"
#include "ScreenIDs.h"

void ExitLaptop();
void HandleLapTopESCKey();
void SetBookMark(int32_t iBookId);
void SetLaptopExitScreen(ScreenID uiExitScreen);
void SetLaptopNewGameFlag();
void LapTopScreenCallBack(MOUSE_REGION *pRegion, int32_t iReason);
void HandleRightButtonUpEvent();
void DoLapTopMessageBox(MessageBoxStyleID, wchar_t const *zString, ScreenID uiExitScreen,
                        MessageBoxFlags, MSGBOX_CALLBACK ReturnCallback);
void GoToWebPage(int32_t iPageId);
void WebPageTileBackground(uint8_t ubNumX, uint8_t ubNumY, uint16_t usWidth, uint16_t usHeight,
                           const SGPVObject *background);
void BlitTitleBarIcons();
void HandleKeyBoardShortCutsForLapTop(uint16_t usEvent, uint32_t usParam, uint16_t usKeyState);
void RenderWWWProgramTitleBar();
void DisplayProgramBoundingBox(BOOLEAN fMarkButtons);
void DoLapTopSystemMessageBox(wchar_t const *zString, ScreenID uiExitScreen, MessageBoxFlags,
                              MSGBOX_CALLBACK ReturnCallback);
void CreateFileAndNewEmailIconFastHelpText(uint32_t uiHelpTextID, BOOLEAN fClearHelpText);
void InitLaptopAndLaptopScreens();

// clear out all temp files from laptop
void ClearOutTempLaptopFiles();

void HaventMadeImpMercEmailCallBack();

enum LaptopMode {
  LAPTOP_MODE_NONE = 0,
  LAPTOP_MODE_FINANCES,
  LAPTOP_MODE_PERSONNEL,
  LAPTOP_MODE_HISTORY,
  LAPTOP_MODE_FILES,
  LAPTOP_MODE_FILES_ENRICO,
  LAPTOP_MODE_FILES_PLANS,
  LAPTOP_MODE_EMAIL,
  LAPTOP_MODE_EMAIL_NEW,
  LAPTOP_MODE_EMAIL_VIEW,
  LAPTOP_MODE_WWW,
  LAPTOP_MODE_AIM,
  LAPTOP_MODE_AIM_MEMBERS,
  LAPTOP_MODE_AIM_MEMBERS_FACIAL_INDEX,
  LAPTOP_MODE_AIM_MEMBERS_SORTED_FILES,
  LAPTOP_MODE_AIM_MEMBERS_SORTED_FILES_VIDEO,
  LAPTOP_MODE_AIM_MEMBERS_ARCHIVES,
  LAPTOP_MODE_AIM_POLICIES,
  LAPTOP_MODE_AIM_HISTORY,
  LAPTOP_MODE_AIM_LINKS,
  LAPTOP_MODE_MERC,
  LAPTOP_MODE_MERC_ACCOUNT,
  LAPTOP_MODE_MERC_NO_ACCOUNT,
  LAPTOP_MODE_MERC_FILES,
  LAPTOP_MODE_BOBBY_R,
  LAPTOP_MODE_BOBBY_R_GUNS,
  LAPTOP_MODE_BOBBY_R_AMMO,
  LAPTOP_MODE_BOBBY_R_ARMOR,
  LAPTOP_MODE_BOBBY_R_MISC,
  LAPTOP_MODE_BOBBY_R_USED,
  LAPTOP_MODE_BOBBY_R_MAILORDER,
  LAPTOP_MODE_CHAR_PROFILE,
  LAPTOP_MODE_CHAR_PROFILE_QUESTIONAIRE,
  LAPTOP_MODE_FLORIST,
  LAPTOP_MODE_FLORIST_FLOWER_GALLERY,
  LAPTOP_MODE_FLORIST_ORDERFORM,
  LAPTOP_MODE_FLORIST_CARD_GALLERY,
  LAPTOP_MODE_INSURANCE,
  LAPTOP_MODE_INSURANCE_INFO,
  LAPTOP_MODE_INSURANCE_CONTRACT,
  LAPTOP_MODE_INSURANCE_COMMENTS,
  LAPTOP_MODE_FUNERAL,
  LAPTOP_MODE_BROKEN_LINK,
  LAPTOP_MODE_BOBBYR_SHIPMENTS,
  LAPTOP_MODE_END
};

extern LaptopMode guiCurrentLaptopMode;
extern LaptopMode guiPreviousLaptopMode;
extern int32_t giCurrentSubPage;
extern BOOLEAN fReDrawScreenFlag;
extern BOOLEAN fPausedReDrawScreenFlag;
extern BOOLEAN fLoadPendingFlag;
extern BOOLEAN fReDrawPostButtonRender;
extern BOOLEAN fCurrentlyInLaptop;
extern SGPVObject *guiLaptopBACKGROUND;
extern SGPVObject *guiTITLEBARICONS;
extern BOOLEAN fDoneLoadPending;
extern BOOLEAN fConnectingToSubPage;
extern BOOLEAN fFastLoadFlag;
extern BOOLEAN gfShowBookmarks;
extern BOOLEAN fShowBookmarkInfo;
extern BOOLEAN fReDrawBookMarkInfo;

// bookamrks for WWW bookmark list

#define LAPTOP_X 0
#define LAPTOP_Y 0

#define LAPTOP_SCREEN_UL_X 111
#define LAPTOP_SCREEN_UL_Y 27
#define LAPTOP_SCREEN_LR_X 613
#define LAPTOP_SCREEN_LR_Y 427
#define LAPTOP_UL_X 24
#define LAPTOP_UL_Y 27
#define LAPTOP_SCREEN_WIDTH LAPTOP_SCREEN_LR_X - LAPTOP_SCREEN_UL_X
#define LAPTOP_SCREEN_HEIGHT LAPTOP_SCREEN_LR_Y - LAPTOP_SCREEN_UL_Y

// new positions for web browser
#define LAPTOP_SCREEN_WEB_UL_Y LAPTOP_SCREEN_UL_Y + 19
#define LAPTOP_SCREEN_WEB_LR_Y LAPTOP_SCREEN_WEB_UL_Y + LAPTOP_SCREEN_HEIGHT
#define LAPTOP_SCREEN_WEB_DELTA_Y LAPTOP_SCREEN_WEB_UL_Y - LAPTOP_SCREEN_UL_Y

// the bookmark values, move cancel down as bookmarks added

enum {
  AIM_BOOKMARK = 0,
  BOBBYR_BOOKMARK,
  IMP_BOOKMARK,
  MERC_BOOKMARK,
  FUNERAL_BOOKMARK,
  FLORIST_BOOKMARK,
  INSURANCE_BOOKMARK,
  CANCEL_STRING,
};

#define DEAD_MERC_COLOR_RED 255
#define DEAD_MERC_COLOR_GREEN 55
#define DEAD_MERC_COLOR_BLUE 55

void DoLapTopSystemMessageBoxWithRect(MessageBoxStyleID, wchar_t const *zString,
                                      ScreenID uiExitScreen, MessageBoxFlags,
                                      MSGBOX_CALLBACK ReturnCallback, SGPBox const *centering_rect);

void LaptopScreenInit();
ScreenID LaptopScreenHandle();
void LaptopScreenShutdown();

#endif
