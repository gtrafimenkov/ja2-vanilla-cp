// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __MAPSCREEN_H
#define __MAPSCREEN_H

#include <stdlib.h>

#include "JA2Types.h"
#include "MessageBoxScreen.h"
#include "SGP/ButtonSystem.h"
#include "ScreenIDs.h"
#include "Tactical/ItemTypes.h"

// Sector name identifiers
enum Towns {
  BLANK_SECTOR = 0,
  OMERTA,
  DRASSEN,
  ALMA,
  GRUMM,
  TIXA,
  CAMBRIA,
  SAN_MONA,
  ESTONI,
  ORTA,
  BALIME,
  MEDUNA,
  CHITZENA,
  NUM_TOWNS
};

#define FIRST_TOWN OMERTA

extern BOOLEAN fCharacterInfoPanelDirty;
extern BOOLEAN fTeamPanelDirty;
extern BOOLEAN fMapPanelDirty;

extern BOOLEAN fMapInventoryItem;
extern BOOLEAN gfInConfirmMapMoveMode;
extern BOOLEAN gfInChangeArrivalSectorMode;

extern BOOLEAN gfSkyriderEmptyHelpGiven;

void SetInfoChar(SOLDIERTYPE const *);
void EndMapScreen(BOOLEAN fDuringFade);
void ReBuildCharactersList();

void HandlePreloadOfMapGraphics();
void HandleRemovalOfPreLoadedMapGraphics();

void ChangeSelectedMapSector(int16_t sMapX, int16_t sMapY, int8_t bMapZ);

BOOLEAN CanExtendContractForSoldier(const SOLDIERTYPE *s);

void TellPlayerWhyHeCantCompressTime();

// the info character
extern int8_t bSelectedInfoChar;

SOLDIERTYPE *GetSelectedInfoChar();
void ChangeSelectedInfoChar(int8_t bCharNumber, BOOLEAN fResetSelectedList);

void MAPEndItemPointer();

void CopyPathToAllSelectedCharacters(PathSt *pPath);
void CancelPathsOfAllSelectedCharacters();

int32_t GetPathTravelTimeDuringPlotting(PathSt *pPath);

void AbortMovementPlottingMode();

BOOLEAN CanChangeSleepStatusForSoldier(const SOLDIERTYPE *s);

bool MapCharacterHasAccessibleInventory(SOLDIERTYPE const &);

wchar_t const *GetMapscreenMercAssignmentString(SOLDIERTYPE const &);
void GetMapscreenMercLocationString(SOLDIERTYPE const &, wchar_t *buf, size_t n);
void GetMapscreenMercDestinationString(SOLDIERTYPE const &, wchar_t *buf, size_t n);
void GetMapscreenMercDepartureString(SOLDIERTYPE const &, wchar_t *buf, size_t n,
                                     uint8_t *text_colour);

// mapscreen wrapper to init the item description box
void MAPInternalInitItemDescriptionBox(OBJECTTYPE *pObject, uint8_t ubStatusIndex,
                                       SOLDIERTYPE *pSoldier);

// rebuild contract box this character
void RebuildContractBoxForMerc(const SOLDIERTYPE *s);

void InternalMAPBeginItemPointer(SOLDIERTYPE *pSoldier);
BOOLEAN ContinueDialogue(SOLDIERTYPE *pSoldier, BOOLEAN fDone);
BOOLEAN GetMouseMapXY(int16_t *psMapWorldX, int16_t *psMapWorldY);
void EndConfirmMapMoveMode();
BOOLEAN CanDrawSectorCursor();
void RememberPreviousPathForAllSelectedChars();
void MapScreenDefaultOkBoxCallback(MessageBoxReturnValue);
void SetUpCursorForStrategicMap();
void DrawFace();

extern GUIButtonRef giMapInvDoneButton;
extern BOOLEAN fInMapMode;
extern BOOLEAN fReDrawFace;
extern BOOLEAN fShowInventoryFlag;
extern BOOLEAN fShowDescriptionFlag;
extern GUIButtonRef giMapContractButton;
extern GUIButtonRef giCharInfoButton[2];
extern BOOLEAN fDrawCharacterList;

// create/destroy inventory button as needed
void CreateDestroyMapInvButton();

void MapScreenInit();
ScreenID MapScreenHandle();
void MapScreenShutdown();

void LockMapScreenInterface(bool lock);
void MakeDialogueEventEnterMapScreen();

void SetMapCursorItem();

#endif
