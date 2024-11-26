#ifndef _OPPLIST_H
#define _OPPLIST_H

#include "JA2Types.h"
#include "Tactical/OverheadTypes.h"

/* For RadioSightings() parameter about */
#define EVERYBODY NULL

#define MAX_MISC_NOISE_DURATION 12  // max dur for VERY loud NOBODY noises

#define DOOR_NOISE_VOLUME 2
#define WINDOW_CRACK_VOLUME 4
#define WINDOW_SMASH_VOLUME 8
#define MACHETE_VOLUME 9
#define TRIMMER_VOLUME 18
#define CHAINSAW_VOLUME 30
#define SMASHING_DOOR_VOLUME 6
#define CROWBAR_DOOR_VOLUME 4
#define ITEM_THROWN_VOLUME 2

#define TIME_BETWEEN_RT_OPPLIST_DECAYS 20

// this is a fake "level" value (0 on ground, 1 on roof) for
// HearNoise to ignore the effects of lighting(?)
#define LIGHT_IRRELEVANT 127

#define AUTOMATIC_INTERRUPT 100
#define NO_INTERRUPT 127

#define MOVEINTERRUPT 0
#define SIGHTINTERRUPT 1
#define NOISEINTERRUPT 2

// noise type constants
enum NoiseKind {
  NOISE_UNKNOWN = 0,
  NOISE_MOVEMENT,
  NOISE_CREAKING,
  NOISE_SPLASHING,
  NOISE_BULLET_IMPACT,
  NOISE_GUNFIRE,
  NOISE_EXPLOSION,
  NOISE_SCREAM,
  NOISE_ROCK_IMPACT,
  NOISE_GRENADE_IMPACT,
  NOISE_WINDOW_SMASHING,
  NOISE_DOOR_SMASHING,
  NOISE_SILENT_ALARM,  // only heard by enemies
  MAX_NOISES
};

#define NUM_WATCHED_LOCS 3

extern int8_t gbPublicOpplist[MAXTEAMS][TOTAL_SOLDIERS];
extern int8_t gbSeenOpponents[TOTAL_SOLDIERS][TOTAL_SOLDIERS];
extern int16_t gsLastKnownOppLoc[TOTAL_SOLDIERS][TOTAL_SOLDIERS];  // merc vs. merc
extern int8_t gbLastKnownOppLevel[TOTAL_SOLDIERS][TOTAL_SOLDIERS];
extern int16_t gsPublicLastKnownOppLoc[MAXTEAMS][TOTAL_SOLDIERS];  // team vs. merc
extern int8_t gbPublicLastKnownOppLevel[MAXTEAMS][TOTAL_SOLDIERS];
extern uint8_t gubPublicNoiseVolume[MAXTEAMS];
extern int16_t gsPublicNoiseGridno[MAXTEAMS];
extern int8_t gbPublicNoiseLevel[MAXTEAMS];
extern uint8_t gubKnowledgeValue[10][10];
extern int8_t gfKnowAboutOpponents;

extern BOOLEAN gfPlayerTeamSawJoey;
extern BOOLEAN gfMikeShouldSayHi;

extern int16_t gsWatchedLoc[TOTAL_SOLDIERS][NUM_WATCHED_LOCS];
extern int8_t gbWatchedLocLevel[TOTAL_SOLDIERS][NUM_WATCHED_LOCS];
extern uint8_t gubWatchedLocPoints[TOTAL_SOLDIERS][NUM_WATCHED_LOCS];
extern BOOLEAN gfWatchedLocReset[TOTAL_SOLDIERS][NUM_WATCHED_LOCS];

#define BEST_SIGHTING_ARRAY_SIZE 6
#define BEST_SIGHTING_ARRAY_SIZE_ALL_TEAMS_LOOK_FOR_ALL 6
#define BEST_SIGHTING_ARRAY_SIZE_NONCOMBAT 3
#define BEST_SIGHTING_ARRAY_SIZE_INCOMBAT 0
extern uint8_t gubBestToMakeSightingSize;

int16_t AdjustMaxSightRangeForEnvEffects(int8_t bLightLevel, int16_t sDistVisible);
void HandleSight(SOLDIERTYPE &, SightFlags);
void AllTeamsLookForAll(uint8_t ubAllowInterrupts);
void GloballyDecideWhoSeesWho();
uint16_t GetClosestMerc(uint16_t usSoldierIndex);
int16_t MaxDistanceVisible();
int16_t DistanceVisible(const SOLDIERTYPE *pSoldier, int8_t bFacingDir, int8_t bSubjectDir,
                        int16_t sSubjectGridNo, int8_t bLevel);
void RecalculateOppCntsDueToNoLongerNeutral(SOLDIERTYPE *pSoldier);

void InitOpponentKnowledgeSystem();
void InitSoldierOppList(SOLDIERTYPE &);
void BetweenTurnsVisibilityAdjustments();
void RemoveManAsTarget(SOLDIERTYPE *pSoldier);
void RadioSightings(SOLDIERTYPE *pSoldier, SOLDIERTYPE *about, uint8_t ubTeamToRadioTo);
void DebugSoldierPage1();
void DebugSoldierPage2();
void DebugSoldierPage3();
void DebugSoldierPage4();

uint8_t MovementNoise(SOLDIERTYPE *pSoldier);
uint8_t DoorOpeningNoise(SOLDIERTYPE *pSoldier);
void MakeNoise(SOLDIERTYPE *noise_maker, int16_t sGridNo, int8_t bLevel, uint8_t ubVolume,
               NoiseKind);
void OurNoise(SOLDIERTYPE *noise_maker, int16_t sGridNo, int8_t bLevel, uint8_t ubVolume,
              NoiseKind);

void ResolveInterruptsVs(SOLDIERTYPE *pSoldier, uint8_t ubInterruptType);

void VerifyAndDecayOpplist(SOLDIERTYPE *pSoldier);
void DecayIndividualOpplist(SOLDIERTYPE *pSoldier);
void VerifyPublicOpplistDueToDeath(SOLDIERTYPE *pSoldier);
void NoticeUnseenAttacker(SOLDIERTYPE *pAttacker, SOLDIERTYPE *pDefender, int8_t bReason);

bool MercSeesCreature(SOLDIERTYPE const &);

int8_t GetWatchedLocPoints(uint8_t ubID, int16_t sGridNo, int8_t bLevel);
int8_t GetHighestVisibleWatchedLoc(const SOLDIERTYPE *s);
int8_t GetHighestWatchedLocPoints(const SOLDIERTYPE *s);

void TurnOffEveryonesMuzzleFlashes();
void TurnOffTeamsMuzzleFlashes(uint8_t ubTeam);
void EndMuzzleFlash(SOLDIERTYPE *pSoldier);
void NonCombatDecayPublicOpplist(uint32_t uiTime);

void CheckHostileOrSayQuoteList();
void InitOpplistForDoorOpening();

void AddToShouldBecomeHostileOrSayQuoteList(SOLDIERTYPE *s);

extern int8_t gbLightSighting[1][16];

void CheckForAlertWhenEnemyDies(SOLDIERTYPE *pDyingSoldier);

extern SOLDIERTYPE *gInterruptProvoker;

extern const SOLDIERTYPE *gWhoThrewRock;

void DecayPublicOpplist(int8_t bTeam);

void RecalculateOppCntsDueToBecomingNeutral(SOLDIERTYPE *pSoldier);

#endif
