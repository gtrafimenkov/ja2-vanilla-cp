#ifndef __OVERHEAD_H
#define __OVERHEAD_H

#include "SGP/Debug.h"
#include "Tactical/SoldierControl.h"

#define MAX_REALTIME_SPEED_VAL 10

// Enums for waiting for mercs to finish codes
enum {
  NO_WAIT_EVENT = 0,
  WAIT_FOR_MERCS_TO_WALKOFF_SCREEN,
  WAIT_FOR_MERCS_TO_WALKON_SCREEN,
  WAIT_FOR_MERCS_TO_WALK_TO_GRIDNO
};

// TACTICAL ENGINE STATUS FLAGS
struct TacticalTeamType {
  uint8_t bFirstID;
  uint8_t bLastID;
  COLORVAL RadarColor;
  int8_t bSide;
  int8_t bMenInSector;
  int8_t bAwareOfOpposition;
  int8_t bHuman;
  SOLDIERTYPE *last_merc_to_radio;
};

#define PANIC_BOMBS_HERE 0x01
#define PANIC_TRIGGERS_HERE 0x02

#define NUM_PANIC_TRIGGERS 3

#define ENEMY_OFFERED_SURRENDER 0x01

struct TacticalStatusType {
  uint32_t uiFlags;
  TacticalTeamType Team[MAXTEAMS];
  uint8_t ubCurrentTeam;
  int16_t sSlideTarget;
  uint32_t uiTimeSinceMercAIStart;
  int8_t fPanicFlags;
  uint8_t ubSpottersCalledForBy;
  SOLDIERTYPE *the_chosen_one;
  uint32_t uiTimeOfLastInput;
  uint32_t uiTimeSinceDemoOn;
  BOOLEAN fCivGroupHostile[NUM_CIV_GROUPS];
  uint8_t ubLastBattleSectorX;
  uint8_t ubLastBattleSectorY;
  BOOLEAN fLastBattleWon;
  BOOLEAN fVirginSector;
  BOOLEAN fEnemyInSector;
  BOOLEAN fInterruptOccurred;
  int8_t bRealtimeSpeed;
  SOLDIERTYPE *enemy_sighting_on_their_turn_enemy;
  BOOLEAN fEnemySightingOnTheirTurn;
  BOOLEAN fAutoBandageMode;
  uint8_t ubAttackBusyCount;
  uint8_t ubEngagedInConvFromActionMercID;
  uint16_t usTactialTurnLimitCounter;
  BOOLEAN fInTopMessage;
  uint8_t ubTopMessageType;
  uint16_t usTactialTurnLimitMax;
  uint32_t uiTactialTurnLimitClock;
  BOOLEAN fTactialTurnLimitStartedBeep;
  int8_t bBoxingState;
  int8_t bConsNumTurnsNotSeen;
  uint8_t ubArmyGuysKilled;

  int16_t sPanicTriggerGridNo[NUM_PANIC_TRIGGERS];
  int8_t bPanicTriggerIsAlarm[NUM_PANIC_TRIGGERS];
  uint8_t ubPanicTolerance[NUM_PANIC_TRIGGERS];
  BOOLEAN fAtLeastOneGuyOnMultiSelect;
  BOOLEAN fKilledEnemyOnAttack;
  SOLDIERTYPE *enemy_killed_on_attack;
  int8_t bEnemyKilledOnAttackLevel;
  uint16_t ubEnemyKilledOnAttackLocation;
  BOOLEAN fItemsSeenOnAttack;
  SOLDIERTYPE *items_seen_on_attack_soldier;
  uint16_t usItemsSeenOnAttackGridNo;
  BOOLEAN fLockItemLocators;
  uint8_t ubLastQuoteSaid;
  uint8_t ubLastQuoteProfileNUm;
  BOOLEAN fCantGetThrough;
  int16_t sCantGetThroughGridNo;
  int16_t sCantGetThroughSoldierGridNo;
  SOLDIERTYPE *cant_get_through;
  BOOLEAN fDidGameJustStart;
  uint8_t ubLastRequesterTargetID;
  uint8_t ubNumCrowsPossible;
  BOOLEAN fUnLockUIAfterHiddenInterrupt;
  int8_t bNumFoughtInBattle[MAXTEAMS];
  uint32_t uiDecayBloodLastUpdate;
  uint32_t uiTimeSinceLastInTactical;
  BOOLEAN fHasAGameBeenStarted;
  int8_t bConsNumTurnsWeHaventSeenButEnemyDoes;
  BOOLEAN fSomeoneHit;
  uint32_t uiTimeSinceLastOpplistDecay;
  SOLDIERTYPE *enemy_killed_on_attack_killer;
  int8_t bMercArrivingQuoteBeingUsed;
  BOOLEAN fCountingDownForGuideDescription;
  int8_t bGuideDescriptionCountDown;
  uint8_t ubGuideDescriptionToUse;
  int8_t bGuideDescriptionSectorX;
  int8_t bGuideDescriptionSectorY;
  int8_t fEnemyFlags;
  BOOLEAN fAutoBandagePending;
  BOOLEAN fHasEnteredCombatModeSinceEntering;
  BOOLEAN fDontAddNewCrows;
  uint16_t sCreatureTenseQuoteDelay;
  uint32_t uiCreatureTenseQuoteLastUpdate;
};

static inline bool IsOnOurTeam(SOLDIERTYPE const &s) { return s.bTeam == OUR_TEAM; }

extern SOLDIERTYPE *g_selected_man;

extern const char *const gzActionStr[];

// Soldier List used for all soldier overhead interaction
extern SOLDIERTYPE Menptr[TOTAL_SOLDIERS];

static inline SOLDIERTYPE &GetMan(uint32_t const idx) {
  Assert(idx < lengthof(Menptr));
  return Menptr[idx];
}

typedef uint8_t SoldierID;

static inline SoldierID Soldier2ID(const SOLDIERTYPE *const s) {
  return s != NULL ? s->ubID : NOBODY;
}

static inline SOLDIERTYPE *ID2Soldier(const SoldierID id) { return id != NOBODY ? &GetMan(id) : 0; }

// For temporary use
#define SOLDIER2ID(s) (Soldier2ID((s)))
#define ID2SOLDIER(i) (ID2Soldier((i)))

static inline SOLDIERTYPE *GetSelectedMan() {
  SOLDIERTYPE *const sel = g_selected_man;
  Assert(sel == NULL || sel->bActive);
  return sel;
}

static inline void SetSelectedMan(SOLDIERTYPE *const s) {
  Assert(s == NULL || s->bActive);
  g_selected_man = s;
}

// MERC SLOTS - A LIST OF ALL ACTIVE MERCS
extern SOLDIERTYPE *MercSlots[TOTAL_SOLDIERS];
extern uint32_t guiNumMercSlots;

#define FOR_EACH_MERC(iter)                                                               \
  for (SOLDIERTYPE **iter = MercSlots, **const end__##iter = MercSlots + guiNumMercSlots; \
       iter != end__##iter; ++iter)                                                       \
    if (Assert(!*iter || (*iter)->bActive), !*iter)                                       \
      continue;                                                                           \
    else

extern TacticalStatusType gTacticalStatus;

static inline BOOLEAN IsTeamActive(const uint32_t team) {
  return gTacticalStatus.Team[team].bMenInSector > 0;
}

#define BASE_FOR_EACH_IN_TEAM(type, iter, team)                                     \
  for (type *iter = &Menptr[gTacticalStatus.Team[(team)].bFirstID],                 \
            *const end__##iter = &Menptr[gTacticalStatus.Team[(team)].bLastID + 1]; \
       iter != end__##iter; ++iter)                                                 \
    if (!iter->bActive)                                                             \
      continue;                                                                     \
    else
#define FOR_EACH_IN_TEAM(iter, team) BASE_FOR_EACH_IN_TEAM(SOLDIERTYPE, iter, (team))
#define CFOR_EACH_IN_TEAM(iter, team) BASE_FOR_EACH_IN_TEAM(const SOLDIERTYPE, iter, (team))

#define BASE_FOR_EACH_SOLDIER(type, iter)                              \
  for (type *iter = Menptr; iter != Menptr + MAX_NUM_SOLDIERS; ++iter) \
    if (!iter->bActive)                                                \
      continue;                                                        \
    else
#define FOR_EACH_SOLDIER(iter) BASE_FOR_EACH_SOLDIER(SOLDIERTYPE, iter)
#define CFOR_EACH_SOLDIER(iter) BASE_FOR_EACH_SOLDIER(const SOLDIERTYPE, iter)

#define BASE_FOR_EACH_NON_PLAYER_SOLDIER(type, iter)                                  \
  for (type *iter = &Menptr[gTacticalStatus.Team[ENEMY_TEAM].bFirstID],               \
            *const end__##iter = &Menptr[gTacticalStatus.Team[CIV_TEAM].bLastID + 1]; \
       iter != end__##iter; ++iter)                                                   \
    if (!iter->bActive)                                                               \
      continue;                                                                       \
    else
#define FOR_EACH_NON_PLAYER_SOLDIER(iter) BASE_FOR_EACH_NON_PLAYER_SOLDIER(SOLDIERTYPE, iter)
#define CFOR_EACH_NON_PLAYER_SOLDIER(iter) BASE_FOR_EACH_NON_PLAYER_SOLDIER(const SOLDIERTYPE, iter)

void InitTacticalEngine();
void ShutdownTacticalEngine();

void InitOverhead();
void ShutdownOverhead();

int16_t NewOKDestination(const SOLDIERTYPE *pCurrSoldier, int16_t sGridNo, BOOLEAN fPeopleToo,
                         int8_t bLevel);

// Simple check to see if a (one-tiled) soldier can occupy a given location on
// the ground or roof.
extern BOOLEAN IsLocationSittable(int32_t iMapIndex, BOOLEAN fOnRoof);
extern BOOLEAN IsLocationSittableExcludingPeople(int32_t iMapIndex, BOOLEAN fOnRoof);
extern BOOLEAN FlatRoofAboveGridNo(int32_t iMapIndex);

void ExecuteOverhead();

void EndTurn(uint8_t ubNextTeam);
void StartPlayerTeamTurn(BOOLEAN fDoBattleSnd, BOOLEAN fEnteringCombatMode);

enum SelSoldierFlags {
  SELSOLDIER_NONE = 0,
  SELSOLDIER_ACKNOWLEDGE = 1U << 0,
  SELSOLDIER_FORCE_RESELECT = 1U << 1,
  SELSOLDIER_FROM_UI = 1U << 2
};
ENUM_BITSET(SelSoldierFlags)

void SelectSoldier(SOLDIERTYPE *s, SelSoldierFlags flags);

void InternalLocateGridNo(uint16_t sGridNo, BOOLEAN fForce);
void LocateGridNo(uint16_t sGridNo);
void LocateSoldier(SOLDIERTYPE *s, BOOLEAN fSetLocator);

void BeginTeamTurn(uint8_t ubTeam);
void SlideTo(SOLDIERTYPE *tgt, BOOLEAN fSetLocator);
void SlideToLocation(int16_t sDestGridNo);

void RebuildAllSoldierShadeTables();
void HandlePlayerTeamMemberDeath(SOLDIERTYPE *pSoldier);

SOLDIERTYPE *FindNextActiveAndAliveMerc(const SOLDIERTYPE *curr, BOOLEAN fGoodForLessOKLife,
                                        BOOLEAN fOnlyRegularMercs);
SOLDIERTYPE *FindPrevActiveAndAliveMerc(const SOLDIERTYPE *curr, BOOLEAN fGoodForLessOKLife,
                                        BOOLEAN fOnlyRegularMercs);

void HandleNPCTeamMemberDeath(SOLDIERTYPE *pSoldier);

BOOLEAN UIOKMoveDestination(const SOLDIERTYPE *pSoldier, uint16_t usMapPos);

int16_t FindAdjacentGridEx(SOLDIERTYPE *pSoldier, int16_t sGridNo, uint8_t *pubDirection,
                           int16_t *psAdjustedGridNo, BOOLEAN fForceToPerson, BOOLEAN fDoor);
int16_t FindNextToAdjacentGridEx(SOLDIERTYPE *pSoldier, int16_t sGridNo, uint8_t *pubDirection,
                                 int16_t *psAdjustedGridNo, BOOLEAN fForceToPerson, BOOLEAN fDoor);

void SelectNextAvailSoldier(const SOLDIERTYPE *s);
BOOLEAN TeamMemberNear(int8_t bTeam, int16_t sGridNo, int32_t iRange);

// FUNCTIONS FOR MANIPULATING MERC SLOTS - A LIST OF ALL ACTIVE MERCS
void AddMercSlot(SOLDIERTYPE *pSoldier);
BOOLEAN RemoveMercSlot(SOLDIERTYPE *pSoldier);

int32_t AddAwaySlot(SOLDIERTYPE *pSoldier);
BOOLEAN RemoveAwaySlot(SOLDIERTYPE *pSoldier);
int32_t MoveSoldierFromMercToAwaySlot(SOLDIERTYPE *pSoldier);
void MoveSoldierFromAwayToMercSlot(SOLDIERTYPE *pSoldier);

void EnterCombatMode(uint8_t ubStartingTeam);
void ExitCombatMode();

void HandleTeamServices(uint8_t ubTeamNum);
void HandlePlayerServices(SOLDIERTYPE &);

void SetEnemyPresence();

void CycleThroughKnownEnemies();

BOOLEAN CheckForEndOfCombatMode(BOOLEAN fIncrementTurnsNotSeen);

SOLDIERTYPE *FreeUpAttacker(SOLDIERTYPE *attacker);

BOOLEAN PlayerTeamFull();

void SetActionToDoOnceMercsGetToLocation(uint8_t ubActionCode, int8_t bNumMercsWaiting);

void ResetAllMercSpeeds();

BOOLEAN HandleGotoNewGridNo(SOLDIERTYPE *pSoldier, BOOLEAN *pfKeepMoving, BOOLEAN fInitialMove,
                            uint16_t usAnimState);

SOLDIERTYPE *ReduceAttackBusyCount(SOLDIERTYPE *attacker, BOOLEAN fCalledByAttacker);

void CommonEnterCombatModeCode();

void CheckForPotentialAddToBattleIncrement(SOLDIERTYPE *pSoldier);

void CencelAllActionsForTimeCompression();

BOOLEAN CheckForEndOfBattle(BOOLEAN fAnEnemyRetreated);

void AddManToTeam(int8_t bTeam);

void RemoveManFromTeam(int8_t bTeam);

void RemoveSoldierFromTacticalSector(SOLDIERTYPE &);

void MakeCivHostile(SOLDIERTYPE *pSoldier, int8_t bNewSide);

#define REASON_NORMAL_ATTACK 1
#define REASON_EXPLOSION 2

BOOLEAN ProcessImplicationsOfPCAttack(SOLDIERTYPE *pSoldier, SOLDIERTYPE *pTarget, int8_t bReason);

int16_t FindAdjacentPunchTarget(const SOLDIERTYPE *pSoldier, const SOLDIERTYPE *pTargetSoldier,
                                int16_t *psAdjustedTargetGridNo);

SOLDIERTYPE *CivilianGroupMemberChangesSides(SOLDIERTYPE *pAttacked);
void CivilianGroupChangesSides(uint8_t ubCivilianGroup);

void CycleVisibleEnemies(SOLDIERTYPE *pSrcSoldier);
uint8_t CivilianGroupMembersChangeSidesWithinProximity(SOLDIERTYPE *pAttacked);

void PauseAITemporarily();
void PauseAIUntilManuallyUnpaused();
void UnPauseAI();

void DoPOWPathChecks();

BOOLEAN HostileCiviliansPresent();
BOOLEAN HostileBloodcatsPresent();
uint8_t NumPCsInSector();
uint8_t NumEnemyInSector();

void SetSoldierNonNeutral(SOLDIERTYPE *pSoldier);
void SetSoldierNeutral(SOLDIERTYPE *pSoldier);

void CaptureTimerCallback();
SOLDIERTYPE *FindNextActiveSquad(SOLDIERTYPE *pSoldier);

extern BOOLEAN gfSurrendered;
extern BOOLEAN gfKillingGuysForLosingBattle;
extern uint8_t gubWaitingForAllMercsToExitCode;

uint8_t NumCapableEnemyInSector();
SOLDIERTYPE *FreeUpAttackerGivenTarget(SOLDIERTYPE *target);
SOLDIERTYPE *ReduceAttackBusyGivenTarget(SOLDIERTYPE *target);

uint32_t NumberOfMercsOnPlayerTeam();

void InitializeTacticalStatusAtBattleStart();

void MakeCharacterDialogueEventSignalItemLocatorStart(SOLDIERTYPE &, GridNo location);

#endif
