#ifndef _SELECTION_WIN_H_
#define _SELECTION_WIN_H_

#include "SGP/Types.h"

enum SelectWindow {
  SELWIN_SINGLEWALL,
  SELWIN_SINGLEDOOR,
  SELWIN_SINGLEWINDOW,
  SELWIN_SINGLEROOF,
  SELWIN_SINGLENEWROOF,
  SELWIN_SINGLEBROKENWALL,
  SELWIN_SINGLEDECOR,
  SELWIN_SINGLEDECAL,
  SELWIN_SINGLEFLOOR,
  SELWIN_SINGLETOILET,

  SELWIN_ROOM,
  SELWIN_BANKS,
  SELWIN_ROADS,
  SELWIN_DEBRIS,
  SELWIN_OSTRUCTS,
  SELWIN_OSTRUCTS1,
  SELWIN_OSTRUCTS2
};

extern BOOLEAN fAllDone;

void CreateJA2SelectionWindow(SelectWindow);
extern void InitJA2SelectionWindow();
extern void ShutdownJA2SelectionWindow();
extern void RemoveJA2SelectionWindow();
extern void RenderSelectionWindow();

extern void ScrollSelWinUp();
extern void ScrollSelWinDown();

#define MAX_SELECTIONS 120

struct Selections {
  uint32_t uiObject;
  uint16_t usIndex;
  int16_t sCount;
};

int32_t GetRandomSelection();
void RestoreSelectionList();
BOOLEAN ClearSelectionList();

void DisplaySelectionWindowGraphicalInformation();

extern int32_t iCurBank;
extern Selections *pSelList;
extern int32_t *pNumSelList;

extern Selections SelOStructs[MAX_SELECTIONS];
extern Selections SelOStructs1[MAX_SELECTIONS];
extern Selections SelOStructs2[MAX_SELECTIONS];
extern Selections SelBanks[MAX_SELECTIONS];
extern Selections SelRoads[MAX_SELECTIONS];
extern Selections SelDebris[MAX_SELECTIONS];

extern Selections SelSingleWall[MAX_SELECTIONS];
extern Selections SelSingleDoor[MAX_SELECTIONS];
extern Selections SelSingleWindow[MAX_SELECTIONS];
extern Selections SelSingleRoof[MAX_SELECTIONS];
extern Selections SelSingleNewRoof[MAX_SELECTIONS];
extern Selections SelSingleBrokenWall[MAX_SELECTIONS];
extern Selections SelSingleDecor[MAX_SELECTIONS];
extern Selections SelSingleDecal[MAX_SELECTIONS];
extern Selections SelSingleFloor[MAX_SELECTIONS];
extern Selections SelSingleToilet[MAX_SELECTIONS];

extern Selections SelRoom[MAX_SELECTIONS];

extern int32_t iNumOStructsSelected;
extern int32_t iNumOStructs1Selected;
extern int32_t iNumOStructs2Selected;
extern int32_t iNumBanksSelected;
extern int32_t iNumRoadsSelected;
extern int32_t iNumDebrisSelected;
extern int32_t iNumWallsSelected;
extern int32_t iNumDoorsSelected;
extern int32_t iNumWindowsSelected;
extern int32_t iNumDecorSelected;
extern int32_t iNumDecalsSelected;
extern int32_t iNumBrokenWallsSelected;
extern int32_t iNumFloorsSelected;
extern int32_t iNumToiletsSelected;
extern int32_t iNumRoofsSelected;
extern int32_t iNumNewRoofsSelected;
extern int32_t iNumRoomsSelected;

extern int32_t iDrawMode;

#endif
