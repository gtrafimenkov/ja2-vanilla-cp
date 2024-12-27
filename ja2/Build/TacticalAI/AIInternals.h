// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "SGP/Random.h"
#include "SGP/Types.h"
#include "Tactical/Overhead.h"
#include "Tactical/Points.h"

extern BOOLEAN gfTurnBasedAI;

// THIS IS AN ITEM #  - AND FOR NOW JUST COMPLETELY FAKE...

#define MAX_TOSS_SEARCH_DIST 1    // must throw within this of opponent
#define NPC_TOSS_SAFETY_MARGIN 4  // all friends must be this far away

#define ACTING_ON_SCHEDULE(p) ((p)->fAIFlags & AI_CHECK_SCHEDULE)

// the AI should try to have this many APs before climbing a roof, if possible
#define AI_AP_CLIMBROOF 15

#define TEMPORARILY 0
#define FOREVER 1

#define IGNORE_PATH 0
#define ENSURE_PATH 1
#define ENSURE_PATH_COST 2

// Kris:  November 10, 1997
// Please don't change this value from 10.  It will invalidate all of the maps
// and soldiers.
#define MAXPATROLGRIDS 10

#define NOWATER 0
#define WATEROK 1

#define DONTADDTURNCOST 0
#define ADDTURNCOST 1

enum { URGENCY_LOW = 0, URGENCY_MED, URGENCY_HIGH, NUM_URGENCY_STATES };

#define NOWATER 0
#define WATEROK 1

#define IGNORE_PATH 0
#define ENSURE_PATH 1
#define ENSURE_PATH_COST 2

#define MAX_ROAMING_RANGE WORLD_COLS

#define PTR_CIV_OR_MILITIA (IsOnCivTeam(pSoldier) || pSoldier->bTeam == MILITIA_TEAM)

#define REALTIME_AI_DELAY (10000 + Random(1000))
#define REALTIME_CIV_AI_DELAY                                \
  (1000 * (gTacticalStatus.Team[MILITIA_TEAM].bMenInSector + \
           gTacticalStatus.Team[CIV_TEAM].bMenInSector) +    \
   5000 + 2000 * Random(3))
#define REALTIME_CREATURE_AI_DELAY (10000 + 1000 * Random(3))

#define NOSHOOT_WAITABIT -1
#define NOSHOOT_WATER -2
#define NOSHOOT_MYSELF -3
#define NOSHOOT_HURT -4
#define NOSHOOT_NOAMMO -5
#define NOSHOOT_NOLOAD -6
#define NOSHOOT_NOWEAPON -7

#define PERCENT_TO_IGNORE_THREAT 50  // any less, use threat value
#define ACTION_TIMEOUT_CYCLES 50     // # failed cycles through AI
#define MAX_THREAT_RANGE 400         // 30 tiles worth
#define MIN_PERCENT_BETTER 5         // 5% improvement in cover is good

#define TOSSES_PER_10TURNS 18  // max # of grenades tossable in 10 turns
#define SHELLS_PER_10TURNS 13  // max # of shells   firable  in 10 turns

#define SEE_THRU_COVER_THRESHOLD 5  // min chance to get through

struct THREATTYPE {
  SOLDIERTYPE *pOpponent;
  int16_t sGridNo;
  int32_t iValue;
  int32_t iAPs;
  int32_t iCertainty;
  int32_t iOrigRange;
};

// define for bAimTime for bursting
#define BURSTING 5

struct ATTACKTYPE {
  SOLDIERTYPE *opponent;        // which soldier is the victim?
  uint8_t ubPossible;           // is this attack form possible?  T/F
  uint8_t ubAimTime;            // how many extra APs to spend on aiming
  uint8_t ubChanceToReallyHit;  // chance to hit * chance to get through cover
  int32_t iAttackValue;         // relative worthiness of this type of attack
  int16_t sTarget;              // target gridno of this attack
  int8_t bTargetLevel;          // target level of this attack
  uint8_t ubAPCost;             // how many APs the attack will use up
  int8_t bWeaponIn;             // the inv slot of the weapon in question
};

extern THREATTYPE Threat[MAXMERCS];
extern int ThreatPercent[10];
extern uint8_t SkipCoverCheck;

enum ItemSearchReason { SEARCH_GENERAL_ITEMS, SEARCH_AMMO, SEARCH_WEAPONS };

// go as far as possible flags
#define FLAG_CAUTIOUS 0x01
#define FLAG_STOPSHORT 0x02

#define STOPSHORTDIST 5

int16_t AdvanceToFiringRange(SOLDIERTYPE *pSoldier, int16_t sClosestOpponent);

void CalcBestShot(SOLDIERTYPE *pSoldier, ATTACKTYPE *pBestShot);
void CalcBestStab(SOLDIERTYPE *pSoldier, ATTACKTYPE *pBestStab, BOOLEAN fBladeAttack);
void CalcTentacleAttack(SOLDIERTYPE *pSoldier, ATTACKTYPE *pBestStab);

int16_t CalcSpreadBurst(SOLDIERTYPE *pSoldier, int16_t sFirstTarget, int8_t bTargetLevel);
int32_t CalcManThreatValue(SOLDIERTYPE *pSoldier, int16_t sMyGrid, uint8_t ubReduceForCover,
                           SOLDIERTYPE *pMe);
int8_t CanNPCAttack(SOLDIERTYPE *pSoldier);
void CheckIfTossPossible(SOLDIERTYPE *pSoldier, ATTACKTYPE *pBestThrow);
BOOLEAN ClimbingNecessary(SOLDIERTYPE *pSoldier, int16_t sDestGridNo, int8_t bDestLevel);
int8_t ClosestPanicTrigger(SOLDIERTYPE *pSoldier);
int16_t ClosestReachableDisturbance(SOLDIERTYPE *pSoldier, uint8_t ubUnconsciousOK,
                                    BOOLEAN *pfChangeLevel);
int16_t ClosestReachableFriendInTrouble(SOLDIERTYPE *pSoldier, BOOLEAN *pfClimbingNecessary);
int16_t ClosestSeenOpponent(SOLDIERTYPE *pSoldier, int16_t *psGridNo, int8_t *pbLevel);
void CreatureCall(SOLDIERTYPE *pCaller);
int8_t CreatureDecideAction(SOLDIERTYPE *pCreature);
void CreatureDecideAlertStatus(SOLDIERTYPE *pCreature);
int8_t CrowDecideAction(SOLDIERTYPE *pSoldier);
void DecideAlertStatus(SOLDIERTYPE *pSoldier);
int8_t DecideAutoBandage(SOLDIERTYPE *pSoldier);
uint16_t DetermineMovementMode(SOLDIERTYPE *pSoldier, int8_t bAction);

int16_t EstimatePathCostToLocation(SOLDIERTYPE *pSoldier, int16_t sDestGridNo, int8_t bDestLevel,
                                   BOOLEAN fAddCostAfterClimbingUp, BOOLEAN *pfClimbingNecessary,
                                   int16_t *psClimbGridNo);

bool FindBetterSpotForItem(SOLDIERTYPE &, int8_t slot);
int16_t FindClosestBoxingRingSpot(SOLDIERTYPE *pSoldier, BOOLEAN fInRing);
int16_t GetInterveningClimbingLocation(SOLDIERTYPE *pSoldier, int16_t sDestGridNo,
                                       int8_t bDestLevel, BOOLEAN *pfClimbingNecessary);
uint8_t GetTraversalQuoteActionID(int8_t bDirection);
int16_t GoAsFarAsPossibleTowards(SOLDIERTYPE *pSoldier, int16_t sDesGrid, int8_t bAction);

int8_t HeadForTheStairCase(SOLDIERTYPE *pSoldier);

bool InGas(SOLDIERTYPE const *, GridNo);
bool InGasOrSmoke(SOLDIERTYPE const *, GridNo);
bool InWaterGasOrSmoke(SOLDIERTYPE const *, GridNo);

void InitAttackType(ATTACKTYPE *pAttack);

int16_t InternalGoAsFarAsPossibleTowards(SOLDIERTYPE *pSoldier, int16_t sDesGrid,
                                         int8_t bReserveAPs, int8_t bAction, int8_t fFlags);

int LegalNPCDestination(SOLDIERTYPE *pSoldier, int16_t sGridno, uint8_t ubPathMode,
                        uint8_t ubWaterOK, uint8_t fFlags);
void LoadWeaponIfNeeded(SOLDIERTYPE *pSoldier);
int16_t MostImportantNoiseHeard(SOLDIERTYPE *pSoldier, int32_t *piRetValue,
                                BOOLEAN *pfClimbingNecessary, BOOLEAN *pfReachable);
void NPCDoesAct(SOLDIERTYPE *pSoldier);
int8_t OKToAttack(SOLDIERTYPE *ptr, int target);
BOOLEAN NeedToRadioAboutPanicTrigger();
int8_t PointPatrolAI(SOLDIERTYPE *pSoldier);
void PossiblyMakeThisEnemyChosenOne(SOLDIERTYPE *pSoldier);
int8_t RandomPointPatrolAI(SOLDIERTYPE *pSoldier);
int32_t RangeChangeDesire(SOLDIERTYPE *pSoldier);
uint16_t RealtimeDelay(SOLDIERTYPE *pSoldier);
void RearrangePocket(SOLDIERTYPE *pSoldier, int8_t bPocket1, int8_t bPocket2, uint8_t bPermanent);
void RTHandleAI(SOLDIERTYPE *pSoldier);
int8_t SearchForItems(SOLDIERTYPE &, ItemSearchReason, uint16_t usItem);
uint8_t ShootingStanceChange(SOLDIERTYPE *pSoldier, ATTACKTYPE *pAttack, int8_t bDesiredDirection);
uint8_t StanceChange(SOLDIERTYPE *pSoldier, uint8_t ubAttackAPCost);
int16_t TrackScent(SOLDIERTYPE *pSoldier);
void RefreshAI(SOLDIERTYPE *pSoldier);
BOOLEAN InLightAtNight(int16_t sGridNo, int8_t bLevel);
int16_t FindNearbyDarkerSpot(SOLDIERTYPE *pSoldier);

BOOLEAN ArmySeesOpponents();
