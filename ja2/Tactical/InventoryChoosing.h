// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __INVENTORY_CHOOSING_H
#define __INVENTORY_CHOOSING_H

#include "SGP/Types.h"
#include "Tactical/SoldierCreate.h"

#define MORTAR_GRENADE_CLASS 100

enum {
  // administrator ratings (1-5)
  BAD_ADMINISTRATOR_EQUIPMENT_RATING = 1,
  POOR_ADMINISTRATOR_EQUIPMENT_RATING,
  AVERAGE_ADMINISTRATOR_EQUIPMENT_RATING,
  GOOD_ADMINISTRATOR_EQUIPMENT_RATING,
  GREAT_ADMINISTRATOR_EQUIPMENT_RATING,
  // army ratings (3-7)
  BAD_ARMY_EQUIPMENT_RATING = AVERAGE_ADMINISTRATOR_EQUIPMENT_RATING,
  POOR_ARMY_EQUIPMENT_RATING,
  AVERAGE_ARMY_EQUIPMENT_RATING,
  GOOD_ARMY_EQUIPMENT_RATING,
  GREAT_ARMY_EQUIPMENT_RATING,
  // elite ratings (5-9)
  BAD_ELITE_EQUIPMENT_RATING = AVERAGE_ARMY_EQUIPMENT_RATING,
  POOR_ELITE_EQUIPMENT_RATING,
  AVERAGE_ELITE_EQUIPMENT_RATING,
  GOOD_ELITE_EQUIPMENT_RATING,
  GREAT_ELITE_EQUIPMENT_RATING,

  // militia ratings (1-5)
  BAD_MILITIA_EQUIPMENT_RATING = BAD_ADMINISTRATOR_EQUIPMENT_RATING,
  POOR_MILITIA_EQUIPMENT_RATING,
  AVERAGE_MILITIA_EQUIPMENT_RATING,
  GOOD_MILITIA_EQUIPMENT_RATING,
  GREAT_MILITIA_EQUIPMENT_RATING,
};

#define MIN_EQUIPMENT_CLASS 1
#define MAX_EQUIPMENT_CLASS 11

// Selects at the start of the game the set of guns the Queen's army will use
// during this game
void InitArmyGunTypes();

// Chooses equipment based on the equipment level (1-11) with best being 11.  It
// allocates a range of equipment to choose from.
void GenerateRandomEquipment(SOLDIERCREATE_STRUCT *pp, int8_t bSoldierClass,
                             int8_t bEquipmentRating);

void AssignCreatureInventory(SOLDIERTYPE *pSoldier);

void ReplaceExtendedGuns(SOLDIERCREATE_STRUCT *pp, int8_t bSoldierClass);

void ResetMortarsOnTeamCount();

#endif
