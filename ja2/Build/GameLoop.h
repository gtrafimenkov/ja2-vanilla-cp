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
