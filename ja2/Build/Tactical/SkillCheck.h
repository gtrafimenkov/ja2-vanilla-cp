// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __SKILL_CHECK_H
#define __SKILL_CHECK_H

#include "JA2Types.h"

void ReducePointsForFatigue(const SOLDIERTYPE *s, uint16_t *pusPoints);
extern int32_t GetSkillCheckPenaltyForFatigue(SOLDIERTYPE *pSoldier, int32_t iSkill);
extern int32_t SkillCheck(SOLDIERTYPE *pSoldier, int8_t bReason, int8_t bDifficulty);
extern int8_t CalcTrapDetectLevel(SOLDIERTYPE *pSoldier, BOOLEAN fExamining);

int8_t EffectiveStrength(const SOLDIERTYPE *s);
int8_t EffectiveWisdom(const SOLDIERTYPE *s);
int8_t EffectiveAgility(const SOLDIERTYPE *s);
int8_t EffectiveMechanical(const SOLDIERTYPE *s);
int8_t EffectiveExplosive(const SOLDIERTYPE *s);
int8_t EffectiveLeadership(const SOLDIERTYPE *s);
int8_t EffectiveMarksmanship(const SOLDIERTYPE *s);
int8_t EffectiveDexterity(const SOLDIERTYPE *s);
int8_t EffectiveExpLevel(const SOLDIERTYPE *s);
int8_t EffectiveMedical(const SOLDIERTYPE *s);

enum SkillChecks {
  NO_CHECK = 0,
  LOCKPICKING_CHECK,
  ELECTRONIC_LOCKPICKING_CHECK,
  ATTACHING_DETONATOR_CHECK,
  ATTACHING_REMOTE_DETONATOR_CHECK,
  PLANTING_BOMB_CHECK,
  PLANTING_REMOTE_BOMB_CHECK,
  OPEN_WITH_CROWBAR,
  SMASH_DOOR_CHECK,
  DISARM_TRAP_CHECK,
  UNJAM_GUN_CHECK,
  NOTICE_DART_CHECK,
  LIE_TO_QUEEN_CHECK,
  ATTACHING_SPECIAL_ITEM_CHECK,
  ATTACHING_SPECIAL_ELECTRONIC_ITEM_CHECK,
  DISARM_ELECTRONIC_TRAP_CHECK
};

#endif
