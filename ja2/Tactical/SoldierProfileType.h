// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __SOLDER_PROFILE_TYPE_H
#define __SOLDER_PROFILE_TYPE_H

#include "SGP/Types.h"
#include "Tactical/OverheadTypes.h"

#define NUM_PROFILES 170
#define FIRST_RPC 57
#define FIRST_NPC 75

#define NAME_LENGTH 30
#define NICKNAME_LENGTH 10

// ONLY HAVE 8 MISC FLAGS.. SHOULD BE ENOUGH
#define PROFILE_MISC_FLAG_RECRUITED 0x01
#define PROFILE_MISC_FLAG_HAVESEENCREATURE 0x02
#define PROFILE_MISC_FLAG_FORCENPCQUOTE 0x04
#define PROFILE_MISC_FLAG_WOUNDEDBYPLAYER 0x08
#define PROFILE_MISC_FLAG_TEMP_NPC_QUOTE_DATA_EXISTS 0x10
#define PROFILE_MISC_FLAG_SAID_HOSTILE_QUOTE 0x20
#define PROFILE_MISC_FLAG_EPCACTIVE 0x40
#define PROFILE_MISC_FLAG_ALREADY_USED_ITEMS \
  0x80  // The player has already purchased the mercs items.

#define PROFILE_MISC_FLAG2_DONT_ADD_TO_SECTOR 0x01
#define PROFILE_MISC_FLAG2_LEFT_COUNTRY 0x02
#define PROFILE_MISC_FLAG2_BANDAGED_TODAY 0x04
#define PROFILE_MISC_FLAG2_SAID_FIRSTSEEN_QUOTE 0x08
#define PROFILE_MISC_FLAG2_NEEDS_TO_SAY_HOSTILE_QUOTE 0x10
#define PROFILE_MISC_FLAG2_MARRIED_TO_HICKS 0x20
#define PROFILE_MISC_FLAG2_ASKED_BY_HICKS 0x40

#define PROFILE_MISC_FLAG3_PLAYER_LEFT_MSG_FOR_MERC_AT_AIM \
  0x01  // In the aimscreen, the merc was away and the player left a message
#define PROFILE_MISC_FLAG3_PERMANENT_INSERTION_CODE 0x02
#define PROFILE_MISC_FLAG3_PLAYER_HAD_CHANCE_TO_HIRE \
  0x04  // player's had a chance to hire this merc
#define PROFILE_MISC_FLAG3_HANDLE_DONE_TRAVERSAL 0x08

#define PROFILE_MISC_FLAG3_NPC_PISSED_OFF 0x10
#define PROFILE_MISC_FLAG3_MERC_MERC_IS_DEAD_AND_QUOTE_SAID \
  0x20  // In the merc site, the merc has died and Speck quote for the dead merc
        // has been said

#define PROFILE_MISC_FLAG3_TOWN_DOESNT_CARE_ABOUT_DEATH 0x40
#define PROFILE_MISC_FLAG3_GOODGUY 0x80
//
// The following variables are used with the 'bMercStatus' variable
//
//

// Merc is ready
#define MERC_OK 0

// if the merc doesnt have a EDT file
#define MERC_HAS_NO_TEXT_FILE -1

// used in the aim video conferencing screen
#define MERC_ANNOYED_BUT_CAN_STILL_CONTACT -2
#define MERC_ANNOYED_WONT_CONTACT -3
#define MERC_HIRED_BUT_NOT_ARRIVED_YET -4

// self explanatory
#define MERC_IS_DEAD -5

// set when the merc is returning home.  A delay for 1,2 or 3 days
#define MERC_RETURNING_HOME -6

// used when merc starts game on assignment, goes on assignment later, or leaves
// to go on another contract
#define MERC_WORKING_ELSEWHERE -7

// When the merc was fired, they were a POW, make sure they dont show up in AIM,
// or MERC as available
#define MERC_FIRED_AS_A_POW -8

// the values for categories of stats
#define SUPER_STAT_VALUE 80
#define NEEDS_TRAINING_STAT_VALUE 50
#define NO_CHANCE_IN_HELL_STAT_VALUE 40

#define SUPER_SKILL_VALUE 80
#define NEEDS_TRAINING_SKILL_VALUE 50
#define NO_CHANCE_IN_HELL_SKILL_VALUE 0

enum SkillTrait {
  NO_SKILLTRAIT = 0,
  LOCKPICKING,
  HANDTOHAND,
  ELECTRONICS,
  NIGHTOPS,
  THROWING,
  TEACHING,
  HEAVY_WEAPS,
  AUTO_WEAPS,
  STEALTHY,
  AMBIDEXT,
  THIEF,
  MARTIALARTS,
  KNIFING,
  ONROOF,
  CAMOUFLAGED,
  NUM_SKILLTRAITS
};

enum PersonalityTrait {
  NO_PERSONALITYTRAIT = 0,
  HEAT_INTOLERANT,
  NERVOUS,
  CLAUSTROPHOBIC,
  NONSWIMMER,
  FEAR_OF_INSECTS,
  FORGETFUL,
  PSYCHO
};

#define NERVOUS_RADIUS 10

enum Attitudes {
  ATT_NORMAL = 0,
  ATT_FRIENDLY,
  ATT_LONER,
  ATT_OPTIMIST,
  ATT_PESSIMIST,
  ATT_AGGRESSIVE,
  ATT_ARROGANT,
  ATT_BIG_SHOT,
  ATT_ASSHOLE,
  ATT_COWARD,
  NUM_ATTITUDES
};

enum Sexes { MALE = 0, FEMALE };

enum SexistLevels { NOT_SEXIST = 0, SOMEWHAT_SEXIST, VERY_SEXIST, GENTLEMAN };

// training defines for evolution, no stat increase, stat decrease( de-evolve )
enum CharacterEvolution {
  NORMAL_EVOLUTION = 0,
  NO_EVOLUTION,
  DEVOLVE,
};

#define BUDDY_MERC(prof, bud) \
  ((prof).bBuddy[0] == (bud) || (prof).bBuddy[1] == (bud) || (prof).bBuddy[2] == (bud))
#define HATED_MERC(prof, hat) \
  ((prof).bHated[0] == (hat) || (prof).bHated[1] == (hat) || (prof).bHated[2] == (hat))

#define BUDDY_OPINION +25
#define HATED_OPINION -25

struct MERCPROFILESTRUCT {
  wchar_t zName[NAME_LENGTH];
  wchar_t zNickname[NICKNAME_LENGTH];
  uint8_t ubFaceIndex;
  PaletteRepID PANTS;
  PaletteRepID VEST;
  PaletteRepID SKIN;
  PaletteRepID HAIR;
  int8_t bSex;
  int8_t bArmourAttractiveness;
  uint8_t ubMiscFlags2;
  int8_t bEvolution;
  uint8_t ubMiscFlags;
  uint8_t bSexist;
  int8_t bLearnToHate;

  // skills
  uint8_t ubQuoteRecord;
  int8_t bDeathRate;

  int16_t sExpLevelGain;
  int16_t sLifeGain;
  int16_t sAgilityGain;
  int16_t sDexterityGain;
  int16_t sWisdomGain;
  int16_t sMarksmanshipGain;
  int16_t sMedicalGain;
  int16_t sMechanicGain;
  int16_t sExplosivesGain;

  uint8_t ubBodyType;
  int8_t bMedical;

  uint16_t usEyesX;
  uint16_t usEyesY;
  uint16_t usMouthX;
  uint16_t usMouthY;
  uint32_t uiBlinkFrequency;
  uint32_t uiExpressionFrequency;
  uint16_t sSectorX;
  uint16_t sSectorY;

  uint32_t uiDayBecomesAvailable;  // day the merc will be available.  used with
                                   // the bMercStatus

  int8_t bStrength;

  int8_t bLifeMax;
  int8_t bExpLevelDelta;
  int8_t bLifeDelta;
  int8_t bAgilityDelta;
  int8_t bDexterityDelta;
  int8_t bWisdomDelta;
  int8_t bMarksmanshipDelta;
  int8_t bMedicalDelta;
  int8_t bMechanicDelta;
  int8_t bExplosivesDelta;
  int8_t bStrengthDelta;
  int8_t bLeadershipDelta;
  uint16_t usKills;
  uint16_t usAssists;
  uint16_t usShotsFired;
  uint16_t usShotsHit;
  uint16_t usBattlesFought;
  uint16_t usTimesWounded;
  uint16_t usTotalDaysServed;

  int16_t sLeadershipGain;
  int16_t sStrengthGain;

  // BODY TYPE SUBSITUTIONS
  uint32_t uiBodyTypeSubFlags;

  int16_t sSalary;
  int8_t bLife;
  int8_t bDexterity;  // dexterity (hand coord) value
  int8_t bPersonalityTrait;
  int8_t bSkillTrait;

  int8_t bReputationTolerance;
  int8_t bExplosive;
  int8_t bSkillTrait2;
  int8_t bLeadership;

  int8_t bBuddy[5];
  int8_t bHated[5];
  int8_t bExpLevel;  // general experience level

  int8_t bMarksmanship;
  int8_t bWisdom;

  uint8_t bInvStatus[19];
  uint8_t bInvNumber[19];
  uint16_t usApproachFactor[4];

  int8_t bMainGunAttractiveness;
  int8_t bAgility;  // agility (speed) value

  BOOLEAN fUseProfileInsertionInfo;  // Set to various flags, ( contained in
                                     // TacticalSave.h )
  int16_t sGridNo;                   // The Gridno the NPC was in before leaving the sector
  uint8_t ubQuoteActionID;
  int8_t bMechanical;

  uint8_t ubInvUndroppable;
  uint8_t ubRoomRangeStart[2];
  uint16_t inv[19];

  uint16_t usStatChangeChances[12];    // used strictly for balancing, never shown!
  uint16_t usStatChangeSuccesses[12];  // used strictly for balancing, never shown!

  uint8_t ubStrategicInsertionCode;

  uint8_t ubRoomRangeEnd[2];

  uint8_t ubLastQuoteSaid;

  int8_t bRace;
  int8_t bNationality;
  int8_t bAppearance;
  int8_t bAppearanceCareLevel;
  int8_t bRefinement;
  int8_t bRefinementCareLevel;
  int8_t bHatedNationality;
  int8_t bHatedNationalityCareLevel;
  int8_t bRacist;
  uint32_t uiWeeklySalary;
  uint32_t uiBiWeeklySalary;
  int8_t bMedicalDeposit;
  int8_t bAttitude;
  uint16_t sMedicalDepositAmount;

  int8_t bLearnToLike;
  uint8_t ubApproachVal[4];
  uint8_t ubApproachMod[3][4];
  int8_t bTown;
  int8_t bTownAttachment;
  uint16_t usOptionalGearCost;
  int8_t bMercOpinion[75];
  int8_t bApproached;
  int8_t bMercStatus;  // The status of the merc.  If negative, see flags at the
                       // top of this file.  Positive:  The number of days the merc
                       // is away for.  0:  Not hired but ready to be.
  int8_t bHatedTime[5];
  int8_t bLearnToLikeTime;
  int8_t bLearnToHateTime;
  int8_t bHatedCount[5];
  int8_t bLearnToLikeCount;
  int8_t bLearnToHateCount;
  uint8_t ubLastDateSpokenTo;
  uint8_t bLastQuoteSaidWasSpecial;
  int8_t bSectorZ;
  uint16_t usStrategicInsertionData;
  int8_t bFriendlyOrDirectDefaultResponseUsedRecently;
  int8_t bRecruitDefaultResponseUsedRecently;
  int8_t bThreatenDefaultResponseUsedRecently;
  int8_t bNPCData;  // NPC specific
  int32_t iBalance;
  uint8_t ubCivilianGroup;
  uint8_t ubNeedForSleep;
  uint32_t uiMoney;
  int8_t bNPCData2;  // NPC specific

  uint8_t ubMiscFlags3;

  uint8_t ubDaysOfMoraleHangover;       // used only when merc leaves team while having
                                        // poor morale
  uint8_t ubNumTimesDrugUseInLifetime;  // The # times a drug has been used in the
                                        // player's lifetime...

  // Flags used for the precedent to repeating oneself in Contract negotiations.
  // Used for quote 80 -  ~107.  Gets reset every day
  uint32_t uiPrecedentQuoteSaid;
  int16_t sPreCombatGridNo;
  uint8_t ubTimeTillNextHatedComplaint;
  uint8_t ubSuspiciousDeath;

  int32_t iMercMercContractLength;  // Used for MERC mercs, specifies how many days
                                    // the merc has gone since last page

  uint32_t uiTotalCostToDate;  // The total amount of money that has been paid to
                               // the merc for their salary
};

static inline bool HasSkillTrait(MERCPROFILESTRUCT const &p, SkillTrait const skill) {
  return p.bSkillTrait == skill || p.bSkillTrait2 == skill;
}

#define TIME_BETWEEN_HATED_COMPLAINTS 24

#define SUSPICIOUS_DEATH 1
#define VERY_SUSPICIOUS_DEATH 2

#endif
