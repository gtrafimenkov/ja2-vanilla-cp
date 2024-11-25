// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __IMP_COMPILE_H
#define __IMP_COMPILE_H

#include "SGP/Types.h"

#define PLAYER_GENERATED_CHARACTER_ID 51
#define NUMBER_OF_PLAYER_PORTRAITS 16

void AddAnAttitudeToAttitudeList(int8_t bAttitude);
void CreateACharacterFromPlayerEnteredStats();
void CreatePlayersPersonalitySkillsAndAttitude();
void AddAPersonalityToPersonalityList(int8_t bPersonlity);
void AddSkillToSkillList(int8_t bSkill);
void ResetSkillsAttributesAndPersonality();
void HandleMercStatsForChangesInFace();

#endif
