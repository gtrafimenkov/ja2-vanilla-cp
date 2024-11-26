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

void ChangeSelectedMapSector(INT16 sMapX, INT16 sMapY, INT8 bMapZ);

BOOLEAN CanExtendContractForSoldier(const SOLDIERTYPE *s);

void TellPlayerWhyHeCantCompressTime();

// the info character
extern INT8 bSelectedInfoChar;

SOLDIERTYPE *GetSelectedInfoChar();
void ChangeSelectedInfoChar(INT8 bCharNumber, BOOLEAN fResetSelectedList);

void MAPEndItemPointer();

void CopyPathToAllSelectedCharacters(PathSt *pPath);
void CancelPathsOfAllSelectedCharacters();

INT32 GetPathTravelTimeDuringPlotting(PathSt *pPath);

void AbortMovementPlottingMode();

BOOLEAN CanChangeSleepStatusForSoldier(const SOLDIERTYPE *s);

bool MapCharacterHasAccessibleInventory(SOLDIERTYPE const &);

wchar_t const *GetMapscreenMercAssignmentString(SOLDIERTYPE const &);
void GetMapscreenMercLocationString(SOLDIERTYPE const &, wchar_t *buf, size_t n);
void GetMapscreenMercDestinationString(SOLDIERTYPE const &, wchar_t *buf, size_t n);
void GetMapscreenMercDepartureString(SOLDIERTYPE const &, wchar_t *buf, size_t n,
                                     UINT8 *text_colour);

// mapscreen wrapper to init the item description box
void MAPInternalInitItemDescriptionBox(OBJECTTYPE *pObject, UINT8 ubStatusIndex,
                                       SOLDIERTYPE *pSoldier);

// rebuild contract box this character
void RebuildContractBoxForMerc(const SOLDIERTYPE *s);

void InternalMAPBeginItemPointer(SOLDIERTYPE *pSoldier);
BOOLEAN ContinueDialogue(SOLDIERTYPE *pSoldier, BOOLEAN fDone);
BOOLEAN GetMouseMapXY(INT16 *psMapWorldX, INT16 *psMapWorldY);
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
