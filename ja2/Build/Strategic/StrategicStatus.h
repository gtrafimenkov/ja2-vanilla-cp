#ifndef __STRATEGIC_STATUS_H
#define __STRATEGIC_STATUS_H

#include "JA2Types.h"
#include "Tactical/ItemTypes.h"

// Enemy is allowed to capture the player after certain day
#define STARTDAY_ALLOW_PLAYER_CAPTURE_FOR_RESCUE 4

#define STRATEGIC_PLAYER_CAPTURED_FOR_RESCUE 0x00000001
#define STRATEGIC_PLAYER_CAPTURED_FOR_ESCAPE 0x00000002

#define ARMY_GUN_LEVELS 11

// player reputation modifiers
#define REPUTATION_LOW_DEATHRATE +5
#define REPUTATION_HIGH_DEATHRATE -5
#define REPUTATION_GREAT_MORALE +3
#define REPUTATION_POOR_MORALE -3
#define REPUTATION_BATTLE_WON +2
#define REPUTATION_BATTLE_LOST -2
#define REPUTATION_TOWN_WON +5
#define REPUTATION_TOWN_LOST -5
#define REPUTATION_SOLDIER_DIED -2  // per exp. level
#define REPUTATION_SOLDIER_CAPTURED -1
#define REPUTATION_KILLED_CIVILIAN -5
#define REPUTATION_EARLY_FIRING -3
#define REPUTATION_KILLED_MONSTER_QUEEN +15
#define REPUTATION_KILLED_DEIDRANNA +25

// flags to remember whether a certain E-mail has already been sent out
#define ENRICO_EMAIL_SENT_SOME_PROGRESS 0x0001
#define ENRICO_EMAIL_SENT_ABOUT_HALFWAY 0x0002
#define ENRICO_EMAIL_SENT_NEARLY_DONE 0x0004
#define ENRICO_EMAIL_SENT_MINOR_SETBACK 0x0008
#define ENRICO_EMAIL_SENT_MAJOR_SETBACK 0x0010
#define ENRICO_EMAIL_SENT_CREATURES 0x0020
#define ENRICO_EMAIL_FLAG_SETBACK_OVER 0x0040
#define ENRICO_EMAIL_SENT_LACK_PROGRESS1 0x0080
#define ENRICO_EMAIL_SENT_LACK_PROGRESS2 0x0100
#define ENRICO_EMAIL_SENT_LACK_PROGRESS3 0x0200

// progress threshold that control Enrico E-mail timing
#define SOME_PROGRESS_THRESHOLD 20
#define ABOUT_HALFWAY_THRESHOLD 55
#define NEARLY_DONE_THRESHOLD 80
#define MINOR_SETBACK_THRESHOLD 5
#define MAJOR_SETBACK_THRESHOLD 15

#define NEW_SECTORS_EQUAL_TO_ACTIVITY 4

// enemy ranks
enum { ENEMY_RANK_ADMIN, ENEMY_RANK_TROOP, ENEMY_RANK_ELITE, NUM_ENEMY_RANKS };

// ways enemies can be killed
enum {
  ENEMY_KILLED_IN_TACTICAL,
  ENEMY_KILLED_IN_AUTO_RESOLVE,
  ENEMY_KILLED_TOTAL,
  NUM_WAYS_ENEMIES_KILLED
};

struct STRATEGIC_STATUS {
  uint32_t uiFlags;
  uint8_t ubNumCapturedForRescue;

  uint8_t ubHighestProgress;  // the highest level of progress player has attained
                              // thus far in the game (0-100)

  uint8_t ubStandardArmyGunIndex[ARMY_GUN_LEVELS];  // type of gun in each group
                                                    // that Queen's army is using
                                                    // this game
  BOOLEAN
  fWeaponDroppedAlready[MAX_WEAPONS];  // flag that tracks whether this
                                       // weapon type has been dropped before

  uint8_t ubMercDeaths;      // how many soldiers have bit it while in the player's
                             // employ (0-100)
  uint32_t uiManDaysPlayed;  // once per day, # living mercs on player's team is
                             // added to this running total

  uint8_t ubBadReputation;  // how bad a reputation player has earned through his
                            // actions, performance, etc. (0-100)

  uint16_t usEnricoEmailFlags;  // bit flags that control progress-related E-mails
                                // from Enrico

  uint8_t ubInsuranceInvestigationsCnt;  // how many times merc has been investigated
                                         // for possible insurance fraud

  uint8_t ubUnhiredMercDeaths;  // how many mercs have died while NOT working for
                                // the player

  uint16_t usPlayerKills;  // kills achieved by all mercs controlled by player
                           // together.  *Excludes* militia kills!

  uint16_t usEnemiesKilled[NUM_WAYS_ENEMIES_KILLED]
                          [NUM_ENEMY_RANKS];  // admin/troop/elite.  Includes
                                              // kills by militia, too
  uint16_t usLastDayOfPlayerActivity;
  uint8_t ubNumNewSectorsVisitedToday;
  uint8_t ubNumberOfDaysOfInactivity;

  int8_t bPadding[70];  // XXX HACK000B
};

void InitStrategicStatus();

extern STRATEGIC_STATUS gStrategicStatus;

void SaveStrategicStatusToSaveGameFile(HWFILE);
void LoadStrategicStatusFromSaveGameFile(HWFILE);

uint8_t CalcDeathRate();

void ModifyPlayerReputation(int8_t bRepChange);

BOOLEAN MercThinksDeathRateTooHigh(MERCPROFILESTRUCT const &);
BOOLEAN MercThinksBadReputationTooHigh(MERCPROFILESTRUCT const &);
BOOLEAN MercThinksHisMoraleIsTooLow(SOLDIERTYPE const *);

void HandleEnricoEmail();

void TrackEnemiesKilled(uint8_t ubKilledHow, uint8_t ubSoldierClass);

uint8_t RankIndexToSoldierClass(uint8_t ubRankIndex);

void UpdateLastDayOfPlayerActivity(uint16_t usDay);

#endif
