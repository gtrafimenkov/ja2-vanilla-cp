#ifndef _QUEST_DEBUG_SYSTEM_H_
#define _QUEST_DEBUG_SYSTEM_H_

#include "ScreenIDs.h"
#include "TacticalAI/NPC.h"

extern int16_t gsQdsEnteringGridNo;

void NpcRecordLoggingInit(ProfileID npc_id, ProfileID merc_id, uint8_t quote_id, Approach);
void NpcRecordLogging(Approach, char const *fmt, ...);

void QuestDebugScreenInit();
ScreenID QuestDebugScreenHandle();

#endif
