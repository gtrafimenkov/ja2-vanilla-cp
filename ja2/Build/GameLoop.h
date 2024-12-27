// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef GAMELOOP_H
#define GAMELOOP_H

#include "MessageBoxScreen.h"
#include "SGP/Types.h"
#include "ScreenIDs.h"

void InitializeGame();
void ShutdownGame();
void GameLoop();

// handle exit from game due to shortcut key
void HandleShortCutExitState();

void SetPendingNewScreen(ScreenID);

extern ScreenID guiPendingScreen;

void NextLoopCheckForEnoughFreeHardDriveSpace();

// callback to confirm game is over
void EndGameMessageBoxCallBack(MessageBoxReturnValue);

#endif
