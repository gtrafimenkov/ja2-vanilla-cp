// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Laptop/IMPCompileCharacter.h"

#include <string.h>
#include <wchar.h>

#include "Directories.h"
#include "Laptop/CharProfile.h"
#include "Laptop/IMPPortraits.h"
#include "Laptop/Laptop.h"
#include "Laptop/LaptopSave.h"
#include "Macro.h"
#include "SGP/Debug.h"
#include "SGP/Random.h"
#include "Tactical/AnimationData.h"
#include "Tactical/SoldierProfile.h"
#include "Tactical/SoldierProfileType.h"
#include "TileEngine/RenderDirty.h"

#define ATTITUDE_LIST_SIZE 20

static int32_t AttitudeList[ATTITUDE_LIST_SIZE];
static int32_t iLastElementInAttitudeList = 0;

static int32_t SkillsList[ATTITUDE_LIST_SIZE];
static int32_t iLastElementInSkillsList = 0;

static int32_t PersonalityList[ATTITUDE_LIST_SIZE];
static int32_t iLastElementInPersonalityList = 0;

extern BOOLEAN fLoadingCharacterForPreviousImpProfile;

static void SelectMercFace();

void CreateACharacterFromPlayerEnteredStats() {
  MERCPROFILESTRUCT &p = GetProfile(PLAYER_GENERATED_CHARACTER_ID + LaptopSaveInfo.iVoiceId);

  wcscpy(p.zName, pFullName);
  wcscpy(p.zNickname, pNickName);

  p.bSex = fCharacterIsMale ? MALE : FEMALE;

  p.bLifeMax = iHealth;
  p.bLife = iHealth;
  p.bAgility = iAgility;
  p.bStrength = iStrength;
  p.bDexterity = iDexterity;
  p.bWisdom = iWisdom;
  p.bLeadership = iLeadership;

  p.bMarksmanship = iMarksmanship;
  p.bMedical = iMedical;
  p.bMechanical = iMechanical;
  p.bExplosive = iExplosives;

  p.bPersonalityTrait = iPersonality;

  p.bAttitude = iAttitude;

  p.bExpLevel = 1;

  // set time away
  p.bMercStatus = 0;

  SelectMercFace();
}

static BOOLEAN DoesCharacterHaveAnAttitude() {
  // simply checks if caracter has an attitude other than normal
  switch (iAttitude) {
    case ATT_LONER:
    case ATT_PESSIMIST:
    case ATT_ARROGANT:
    case ATT_BIG_SHOT:
    case ATT_ASSHOLE:
    case ATT_COWARD:
      return TRUE;

    default:
      return FALSE;
  }
}

static BOOLEAN DoesCharacterHaveAPersoanlity() {
  // only one we can get is PSYCHO, and that is not much of a penalty
  return (FALSE);
  /*
  // simply checks if caracter has a personality other than normal
if( iPersonality != NO_PERSONALITYTRAIT )
  {
          // yep
    return ( TRUE );
  }
  else
  {
          // nope
          return ( FALSE );
  }
  */
}

static void CreatePlayerAttitude() {
  // this function will 'roll a die' and decide if any attitude does exists
  int32_t iAttitudeHits[NUM_ATTITUDES] = {0};

  iAttitude = ATT_NORMAL;

  if (iLastElementInAttitudeList == 0) {
    return;
  }

  // count # of hits for each attitude
  for (int32_t i = 0; i < iLastElementInAttitudeList; i++) {
    iAttitudeHits[AttitudeList[i]]++;
  }

  // find highest # of hits for any attitude
  int32_t iHighestHits = 0;
  int32_t iNumAttitudesWithHighestHits = 0;
  for (int32_t i = 0; i < NUM_ATTITUDES; i++) {
    if (iAttitudeHits[i]) {
      if (iAttitudeHits[i] > iHighestHits) {
        iHighestHits = iAttitudeHits[i];
        iNumAttitudesWithHighestHits = 1;
      } else if (iAttitudeHits[i] == iHighestHits) {
        iNumAttitudesWithHighestHits++;
      }
    }
  }

  int32_t iDiceValue = Random(iNumAttitudesWithHighestHits + 1);  // XXX TODO0008

  // find attitude
  int32_t iCounter2 = 0;
  for (int32_t i = 0; i < NUM_ATTITUDES; i++) {
    if (iAttitudeHits[i] == iHighestHits) {
      if (iCounter2 == iDiceValue) {
        // this is it!
        iAttitude = iCounter2;
        break;
      } else {
        // one of the next attitudes...
        iCounter2++;
      }
    }
  }
}

void AddAnAttitudeToAttitudeList(int8_t bAttitude) {
  // adds an attitude to attitude list
  if (iLastElementInAttitudeList < ATTITUDE_LIST_SIZE) {
    AttitudeList[iLastElementInAttitudeList++] = bAttitude;
  }
}

void AddSkillToSkillList(int8_t bSkill) {
  // adds a skill to skills list
  if (iLastElementInSkillsList < ATTITUDE_LIST_SIZE) {
    SkillsList[iLastElementInSkillsList++] = bSkill;
  }
}

static void RemoveSkillFromSkillsList(int32_t const Skill) {
  for (size_t i = 0; i != iLastElementInSkillsList;) {
    if (SkillsList[i] == Skill) {
      SkillsList[i] = SkillsList[--iLastElementInSkillsList];
    } else {
      ++i;
    }
  }
}

static void ValidateSkillsList() {
  // remove from the generated traits list any traits that don't match
  // the character's skills
  MERCPROFILESTRUCT &p = GetProfile(PLAYER_GENERATED_CHARACTER_ID + LaptopSaveInfo.iVoiceId);

  if (p.bMechanical == 0) {
    // without mechanical, electronics is useless
    RemoveSkillFromSkillsList(ELECTRONICS);
  }

  // special check for lockpicking
  int32_t iValue = p.bMechanical;
  iValue = iValue * p.bWisdom / 100;
  iValue = iValue * p.bDexterity / 100;
  if (iValue + gbSkillTraitBonus[LOCKPICKING] < 50) {
    // not good enough for lockpicking!
    RemoveSkillFromSkillsList(LOCKPICKING);
  }

  if (p.bMarksmanship == 0) {
    // without marksmanship, the following traits are useless:
    RemoveSkillFromSkillsList(AUTO_WEAPS);
    RemoveSkillFromSkillsList(HEAVY_WEAPS);
  }
}

static void CreatePlayerSkills() {
  ValidateSkillsList();

  iSkillA = SkillsList[Random(iLastElementInSkillsList)];

  // there is no expert level these skills
  if (iSkillA == ELECTRONICS || iSkillA == AMBIDEXT) RemoveSkillFromSkillsList(iSkillA);

  if (iLastElementInSkillsList == 0) {
    iSkillB = NO_SKILLTRAIT;
  } else {
    iSkillB = SkillsList[Random(iLastElementInSkillsList)];
  }
}

void AddAPersonalityToPersonalityList(int8_t bPersonality) {
  // CJC, Oct 26 98: prevent personality list from being generated
  // because no dialogue was written to support PC personality quotes

  // BUT we can manage this for PSYCHO okay
  if (bPersonality != PSYCHO) return;

  // will add a persoanlity to persoanlity list
  if (iLastElementInPersonalityList < ATTITUDE_LIST_SIZE) {
    PersonalityList[iLastElementInPersonalityList++] = bPersonality;
  }
}

static void CreatePlayerPersonality() {
  // only psycho is available since we have no quotes
  // SO if the array is not empty, give them psycho!
  if (iLastElementInPersonalityList == 0) {
    iPersonality = NO_PERSONALITYTRAIT;
  } else {
    iPersonality = PSYCHO;
  }

  /*
    // this function will 'roll a die' and decide if any Personality does exists
    int32_t iDiceValue = 0;
    int32_t iCounter = 0;
          int32_t iSecondAttempt = -1;

          // roll dice
          iDiceValue = Random( iLastElementInPersonalityList + 1 );

          iPersonality = NO_PERSONALITYTRAIT;
    if( PersonalityList[ iDiceValue ] !=  NO_PERSONALITYTRAIT )
          {
                  for( iCounter = 0; iCounter < iLastElementInPersonalityList;
    iCounter++ )
                  {
                          if( iCounter != iDiceValue )
                          {
                                  if( PersonalityList[ iCounter ] ==
    PersonalityList[ iDiceValue ] )
                                  {
                                          if( PersonalityList[ iDiceValue ] !=
    PSYCHO )
                                          {
              iPersonality = PersonalityList[ iDiceValue ];
                                          }
                                          else
                                          {
              iSecondAttempt = iCounter;
                                          }
                                          if( iSecondAttempt != iCounter )
                                          {
                                                  iPersonality =
    PersonalityList[ iDiceValue ];
                                          }

                                  }
                          }
                  }
          }
  */
}

void CreatePlayersPersonalitySkillsAndAttitude() {
  // creates personality and attitudes from curretly built list
  CreatePlayerPersonality();
  CreatePlayerAttitude();
}

void ResetSkillsAttributesAndPersonality() {
  // reset count of skills attributes and personality
  iLastElementInPersonalityList = 0;
  iLastElementInSkillsList = 0;
  iLastElementInAttitudeList = 0;
}

static void SetMercSkinAndHairColors();

static void SelectMercFace() {
  // this procedure will select the approriate face for the merc and save
  // offsets
  MERCPROFILESTRUCT &p = GetProfile(PLAYER_GENERATED_CHARACTER_ID + LaptopSaveInfo.iVoiceId);

  // now the offsets
  p.ubFaceIndex = 200 + iPortraitNumber;

  // eyes
  p.usEyesX = 0;
  p.usEyesY = 0;

  // mouth
  p.usMouthX = 0;
  p.usMouthY = 0;

  // set merc skins and hair color
  SetMercSkinAndHairColors();
}

static void SetMercSkinAndHairColors() {
#define PINKSKIN "PINKSKIN"
#define TANSKIN "TANSKIN"
#define DARKSKIN "DARKSKIN"
#define BLACKSKIN "BLACKSKIN"

#define BROWNHEAD "BROWNHEAD"
#define BLACKHEAD "BLACKHEAD"  // black skin till here
#define WHITEHEAD "WHITEHEAD"  // dark skin till here
#define BLONDHEAD "BLONDHEAD"
#define REDHEAD "REDHEAD"  // pink/tan skin till here

  static const struct {
    const char *Skin;
    const char *Hair;
  } Colors[] = {
      {BLACKSKIN, BROWNHEAD}, {TANSKIN, BROWNHEAD},   {TANSKIN, BROWNHEAD}, {DARKSKIN, BROWNHEAD},
      {TANSKIN, BROWNHEAD},   {DARKSKIN, BLACKHEAD},  {TANSKIN, BROWNHEAD}, {TANSKIN, BROWNHEAD},
      {TANSKIN, BROWNHEAD},   {PINKSKIN, BROWNHEAD},  {TANSKIN, BLACKHEAD}, {TANSKIN, BLACKHEAD},
      {PINKSKIN, BROWNHEAD},  {BLACKSKIN, BROWNHEAD}, {TANSKIN, REDHEAD},   {TANSKIN, BLONDHEAD}};

  Assert(iPortraitNumber < lengthof(Colors));
  MERCPROFILESTRUCT &p = GetProfile(PLAYER_GENERATED_CHARACTER_ID + LaptopSaveInfo.iVoiceId);
  strcpy(p.HAIR, Colors[iPortraitNumber].Hair);
  strcpy(p.SKIN, Colors[iPortraitNumber].Skin);
}

static BOOLEAN ShouldThisMercHaveABigBody();

void HandleMercStatsForChangesInFace() {
  if (fLoadingCharacterForPreviousImpProfile) return;

  CreatePlayerSkills();

  MERCPROFILESTRUCT &p = GetProfile(PLAYER_GENERATED_CHARACTER_ID + LaptopSaveInfo.iVoiceId);

  // body type
  if (fCharacterIsMale) {
    if (ShouldThisMercHaveABigBody()) {
      p.ubBodyType = BIGMALE;
      if (iSkillA == MARTIALARTS) iSkillA = HANDTOHAND;
      if (iSkillB == MARTIALARTS) iSkillB = HANDTOHAND;
    } else {
      p.ubBodyType = REGMALE;
    }
  } else {
    p.ubBodyType = REGFEMALE;
    if (iSkillA == MARTIALARTS) iSkillA = HANDTOHAND;
    if (iSkillB == MARTIALARTS) iSkillB = HANDTOHAND;
  }

  // skill trait
  p.bSkillTrait = iSkillA;
  p.bSkillTrait2 = iSkillB;
}

static BOOLEAN ShouldThisMercHaveABigBody() {
  // should this merc be a big body typ
  return (iPortraitNumber == 0 || iPortraitNumber == 6 || iPortraitNumber == 7) &&
         gMercProfiles[PLAYER_GENERATED_CHARACTER_ID + LaptopSaveInfo.iVoiceId].bStrength >= 75;
}
