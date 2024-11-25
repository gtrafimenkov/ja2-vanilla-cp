// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef _MORALE_H
#define _MORALE_H

#include "JA2Types.h"

#define DEFAULT_MORALE 50

enum MoraleEventNames {
  MORALE_KILLED_ENEMY = 0,
  MORALE_SQUADMATE_DIED,
  MORALE_SUPPRESSED,
  MORALE_AIRSTRIKE,
  MORALE_DID_LOTS_OF_DAMAGE,
  MORALE_TOOK_LOTS_OF_DAMAGE,  // 5
  MORALE_KILLED_CIVILIAN,
  MORALE_BATTLE_WON,
  MORALE_RAN_AWAY,
  MORALE_HEARD_BATTLE_WON,
  MORALE_HEARD_BATTLE_LOST,  // 10
  MORALE_TOWN_LIBERATED,
  MORALE_TOWN_LOST,
  MORALE_MINE_LIBERATED,
  MORALE_MINE_LOST,
  MORALE_SAM_SITE_LIBERATED,  // 15
  MORALE_SAM_SITE_LOST,
  MORALE_BUDDY_DIED,
  MORALE_HATED_DIED,
  MORALE_TEAMMATE_DIED,
  MORALE_LOW_DEATHRATE,  // 20
  MORALE_HIGH_DEATHRATE,
  MORALE_GREAT_MORALE,
  MORALE_POOR_MORALE,
  MORALE_DRUGS_CRASH,
  MORALE_ALCOHOL_CRASH,  // 25
  MORALE_MONSTER_QUEEN_KILLED,
  MORALE_DEIDRANNA_KILLED,
  MORALE_CLAUSTROPHOBE_UNDERGROUND,
  MORALE_INSECT_PHOBIC_SEES_CREATURE,
  MORALE_NERVOUS_ALONE,  // 30
  MORALE_MERC_CAPTURED,
  MORALE_MERC_MARRIED,
  MORALE_QUEEN_BATTLE_WON,
  MORALE_SEX,
  NUM_MORALE_EVENTS
};

enum MoraleEventType { TACTICAL_MORALE_EVENT = 0, STRATEGIC_MORALE_EVENT };

struct MoraleEvent {
  uint8_t ubType;
  int8_t bChange;
};

extern void HandleMoraleEvent(SOLDIERTYPE *pSoldier, int8_t bMoraleEvent, int16_t sMapX,
                              int16_t sMapY, int8_t bMapZ);
extern void RefreshSoldierMorale(SOLDIERTYPE *pSoldier);
extern int8_t GetMoraleModifier(SOLDIERTYPE *pSoldier);

void HourlyMoraleUpdate();
void DailyMoraleUpdate(SOLDIERTYPE *pSoldier);

void DecayTacticalMoraleModifiers();

#endif
