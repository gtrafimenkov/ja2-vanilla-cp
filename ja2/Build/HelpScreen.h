#ifndef _HELP_SCREEN__H_
#define _HELP_SCREEN__H_

#include "SGP/Types.h"

// enum used for the different help screens that can come up
enum HelpScreenID {
  HELP_SCREEN_NONE = -1,
  HELP_SCREEN_LAPTOP,
  HELP_SCREEN_MAPSCREEN,
  HELP_SCREEN_MAPSCREEN_NO_ONE_HIRED,
  HELP_SCREEN_MAPSCREEN_NOT_IN_ARULCO,
  HELP_SCREEN_MAPSCREEN_SECTOR_INVENTORY,
  HELP_SCREEN_TACTICAL,
  HELP_SCREEN_OPTIONS,
  HELP_SCREEN_LOAD_GAME,

  HELP_SCREEN_NUMBER_OF_HELP_SCREENS,
};

struct HELP_SCREEN_STRUCT {
  HelpScreenID bCurrentHelpScreen;
  uint32_t uiFlags;

  uint16_t usHasPlayerSeenHelpScreenInCurrentScreen;

  uint8_t ubHelpScreenDirty;

  uint16_t usScreenLocX;
  uint16_t usScreenLocY;
  uint16_t usScreenWidth;
  uint16_t usScreenHeight;

  int32_t iLastMouseClickY;  // last position the mouse was clicked ( if != -1 )

  int8_t bCurrentHelpScreenActiveSubPage;  // used to keep track of the current
                                           // page being displayed

  int8_t bNumberOfButtons;

  // used so if the user checked the box to show the help, it doesnt
  // automatically come up every frame
  BOOLEAN fHaveAlreadyBeenInHelpScreenSinceEnteringCurrenScreen;

  int8_t bDelayEnteringHelpScreenBy1FrameCount;
  uint16_t usLeftMarginPosX;

  uint16_t usCursor;

  BOOLEAN fWasTheGamePausedPriorToEnteringHelpScreen;

  // scroll variables
  uint16_t usTotalNumberOfPixelsInBuffer;
  int32_t iLineAtTopOfTextBuffer;
  uint16_t usTotalNumberOfLinesInBuffer;
  BOOLEAN fForceHelpScreenToComeUp;
};

extern HELP_SCREEN_STRUCT gHelpScreen;

BOOLEAN ShouldTheHelpScreenComeUp(HelpScreenID, BOOLEAN fForceHelpScreenToComeUp);
void HelpScreenHandler();
void InitHelpScreenSystem();
void NewScreenSoResetHelpScreen();
HelpScreenID HelpScreenDetermineWhichMapScreenHelpToShow();

#endif
