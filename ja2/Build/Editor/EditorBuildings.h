// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __EDITORBUILDINGS_H
#define __EDITORBUILDINGS_H

#include "SGP/Types.h"

extern BOOLEAN fBuildingShowRoofs, fBuildingShowWalls, fBuildingShowRoomInfo;
extern uint8_t gubCurrRoomNumber;
extern uint8_t gubMaxRoomNumber;

void SetupTextInputForBuildings();
void ExtractAndUpdateBuildingInfo();

// Initialization routines
void GameInitEditorBuildingInfo();

// Selection method callbacks
// Building utility functions
void UpdateBuildingsInfo();
void KillBuilding(uint32_t iMapIndex);

struct BUILDINGLAYOUTNODE {
  BUILDINGLAYOUTNODE *next;
  int16_t sGridNo;
};

extern BUILDINGLAYOUTNODE *gpBuildingLayoutList;
extern int16_t gsBuildingLayoutAnchorGridNo;

// The first step is copying a building.  After that, it either must be pasted
// or moved.
void CopyBuilding(int32_t iMapIndex);
void MoveBuilding(int32_t iMapIndex);
void PasteBuilding(int32_t iMapIndex);
void DeleteBuildingLayout();

void ReplaceBuildingWithNewRoof(int32_t iMapIndex);
void UpdateWallsView();
void UpdateRoofsView();

void InitDoorEditing(int32_t iMapIndex);
void ExtractAndUpdateDoorInfo();
void KillDoorEditing();
void RenderDoorEditingWindow();

void AddLockedDoorCursors();
void RemoveLockedDoorCursors();
void FindNextLockedDoor();

extern BOOLEAN gfEditingDoor;

extern uint16_t usCurrentMode;

#endif
