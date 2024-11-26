#ifndef _GAMESCREEN_H
#define _GAMESCREEN_H

#include "JA2Types.h"
#include "ScreenIDs.h"

#define ARE_IN_FADE_IN() (gfFadeIn || gfFadeInitialized)

void FadeInGameScreen();
void FadeOutGameScreen();

typedef void (*MODAL_HOOK)();

extern BOOLEAN gfGameScreenLocateToSoldier;
extern BOOLEAN gfEnteringMapScreen;
extern SOLDIERTYPE *gPreferredInitialSelectedGuy;

void UpdateTeamPanelAssignments();

#define TACTICAL_MODAL_NOMOUSE 1
#define TACTICAL_MODAL_WITHMOUSE 2

extern MODAL_HOOK gModalDoneCallback;

void EnterModalTactical(int8_t bMode);
void EndModalTactical();

// handle the entrance of the mercs at the beginning of the game
void InitHelicopterEntranceByMercs();

void InternalLeaveTacticalScreen(ScreenID uiNewScreen);

extern BOOLEAN gfBeginEndTurn;

extern VIDEO_OVERLAY *g_fps_overlay;
extern VIDEO_OVERLAY *g_counter_period_overlay;

void EnterTacticalScreen();
void LeaveTacticalScreen(ScreenID uiNewScreen);

void MainGameScreenInit();
ScreenID MainGameScreenHandle();
void MainGameScreenShutdown();

#endif
