// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __MAP_INTERFACE_BOTTOM
#define __MAP_INTERFACE_BOTTOM

#include "SGP/ButtonSystem.h"
#include "SGP/Types.h"

#define MAX_MESSAGES_ON_MAP_BOTTOM 9

enum ExitToWhere {
  MAP_EXIT_TO_INVALID = -1,
  MAP_EXIT_TO_LAPTOP = 0,
  MAP_EXIT_TO_TACTICAL = 1,
  MAP_EXIT_TO_OPTIONS = 2,
  MAP_EXIT_TO_LOAD,
  MAP_EXIT_TO_SAVE
};

static inline ExitToWhere operator++(ExitToWhere &a) { return a = (ExitToWhere)(a + 1); }

// there's no button for entering SAVE/LOAD screen directly...
extern GUIButtonRef guiMapBottomExitButtons[3];

extern BOOLEAN fLapTop;
extern BOOLEAN fLeavingMapScreen;
extern BOOLEAN gfDontStartTransitionFromLaptop;
extern BOOLEAN gfStartMapScreenToLaptopTransition;

// function prototypes

void LoadMapScreenInterfaceBottom();
void DeleteMapScreenInterfaceBottom();
void RenderMapScreenInterfaceBottom();

// delete map bottom graphics
void DeleteMapBottomGraphics();

// load bottom graphics
void HandleLoadOfMapBottomGraphics();

// allowed to time compress?
BOOLEAN AllowedToTimeCompress();

void EnableDisAbleMapScreenOptionsButton(BOOLEAN fEnable);

// create and destroy masks to cover the time compression buttons as needed
void CreateDestroyMouseRegionMasksForTimeCompressionButtons();

BOOLEAN CommonTimeCompressionChecks();

bool AnyUsableRealMercenariesOnTeam();

void RequestTriggerExitFromMapscreen(ExitToWhere);
BOOLEAN AllowedToExitFromMapscreenTo(ExitToWhere);
void HandleExitsFromMapScreen();

void MapScreenMsgScrollDown(uint8_t ubLinesDown);
void MapScreenMsgScrollUp(uint8_t ubLinesUp);

void ChangeCurrentMapscreenMessageIndex(uint8_t ubNewMessageIndex);
void MoveToEndOfMapScreenMessageList();

// the dirty state of the mapscreen interface bottom
extern BOOLEAN fMapScreenBottomDirty;

#endif
