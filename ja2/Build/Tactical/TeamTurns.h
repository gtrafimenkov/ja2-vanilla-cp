#ifndef TEAMTURNS_H
#define TEAMTURNS_H

#include "JA2Types.h"

extern uint8_t gubOutOfTurnPersons;
extern BOOLEAN gfHiddenInterrupt;
extern BOOLEAN gfHiddenTurnbased;

#define INTERRUPT_QUEUED (gubOutOfTurnPersons > 0)

BOOLEAN StandardInterruptConditionsMet(const SOLDIERTYPE *pSoldier, const SOLDIERTYPE *pOpponent,
                                       int8_t bOldOppList);
int8_t CalcInterruptDuelPts(const SOLDIERTYPE *pSoldier, const SOLDIERTYPE *opponent,
                            BOOLEAN fUseWatchSpots);
extern void EndAITurn();
extern void DisplayHiddenInterrupt(SOLDIERTYPE *pSoldier);
extern BOOLEAN InterruptDuel(SOLDIERTYPE *pSoldier, SOLDIERTYPE *pOpponent);
void AddToIntList(SOLDIERTYPE *s, BOOLEAN fGainControl, BOOLEAN fCommunicate);
void DoneAddingToIntList();

void ClearIntList();

void SaveTeamTurnsToTheSaveGameFile(HWFILE);

void LoadTeamTurnsFromTheSavedGameFile(HWFILE);

void EndAllAITurns();

BOOLEAN NPCFirstDraw(SOLDIERTYPE *pSoldier, SOLDIERTYPE *pTargetSoldier);

void SayCloseCallQuotes();

void DisplayHiddenTurnbased(SOLDIERTYPE *pActingSoldier);

#endif
