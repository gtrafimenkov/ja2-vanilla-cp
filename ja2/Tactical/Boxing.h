// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef BOXING_H
#define BOXING_H

#include "JA2Types.h"

#define BOXING_SECTOR_X 5
#define BOXING_SECTOR_Y 4
#define BOXING_SECTOR_Z 0
#define ROOM_SURROUNDING_BOXING_RING 3
#define BOXING_RING 29

#define BOXING_AI_START_POSITION 11235

#define NUM_BOXERS 3

enum DisqualificationReasons { BOXER_OUT_OF_RING, NON_BOXER_IN_RING, BAD_ATTACK };

extern int16_t gsBoxerGridNo[NUM_BOXERS];
extern SOLDIERTYPE *gBoxer[NUM_BOXERS];
extern BOOLEAN gfBoxerFought[NUM_BOXERS];
extern int8_t gbBoxingState;
extern BOOLEAN gfLastBoxingMatchWonByPlayer;
extern uint8_t gubBoxingMatchesWon;
extern uint8_t gubBoxersRests;
extern BOOLEAN gfBoxersResting;

extern void BoxingPlayerDisqualified(SOLDIERTYPE *pOffender, int8_t bReason);
bool CheckOnBoxers();
extern void EndBoxingMatch(SOLDIERTYPE *pLoser);
bool BoxerAvailable();
extern BOOLEAN AnotherFightPossible();
extern void TriggerEndOfBoxingRecord(SOLDIERTYPE *pSolier);
extern void BoxingMovementCheck(SOLDIERTYPE *pSoldier);
extern void ExitBoxing();
extern void SetBoxingState(int8_t bNewState);
bool BoxerExists();
extern uint8_t CountPeopleInBoxingRing();
extern void ClearAllBoxerFlags();

#endif
