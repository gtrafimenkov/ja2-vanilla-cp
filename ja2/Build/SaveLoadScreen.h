// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef _SAVE_LOAD_SCREEN__H_
#define _SAVE_LOAD_SCREEN__H_

#include "JA2Types.h"
#include "MessageBoxScreen.h"
#include "ScreenIDs.h"

#define NUM_SAVE_GAMES 11

// This flag is used to diferentiate between loading a game and saveing a game.
// gfSaveGame=TRUE		For saving a game
// gfSaveGame=FALSE		For loading a game
extern BOOLEAN gfSaveGame;

extern BOOLEAN gfCameDirectlyFromGame;

ScreenID SaveLoadScreenHandle();

void DoSaveLoadMessageBox(wchar_t const *zString, ScreenID uiExitScreen, MessageBoxFlags,
                          MSGBOX_CALLBACK ReturnCallback);

void DoQuickSave();
void DoQuickLoad();

bool AreThereAnySavedGameFiles();

void DeleteSaveGameNumber(uint8_t save_slot_id);

#endif
