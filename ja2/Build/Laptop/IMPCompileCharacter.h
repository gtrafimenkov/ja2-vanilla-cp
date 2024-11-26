#ifndef __IMP_COMPILE_H
#define __IMP_COMPILE_H

#include "SGP/Types.h"

#define PLAYER_GENERATED_CHARACTER_ID 51
#define NUMBER_OF_PLAYER_PORTRAITS 16

void AddAnAttitudeToAttitudeList(INT8 bAttitude);
void CreateACharacterFromPlayerEnteredStats();
void CreatePlayersPersonalitySkillsAndAttitude();
void AddAPersonalityToPersonalityList(INT8 bPersonlity);
void AddSkillToSkillList(INT8 bSkill);
void ResetSkillsAttributesAndPersonality();
void HandleMercStatsForChangesInFace();

#endif
