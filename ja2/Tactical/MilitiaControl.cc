// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Tactical/MilitiaControl.h"

#include "Macro.h"
#include "Strategic/CampaignTypes.h"
#include "Strategic/PreBattleInterface.h"
#include "Strategic/StrategicMap.h"
#include "Strategic/TownMilitia.h"
#include "Tactical/Overhead.h"
#include "Tactical/SoldierControl.h"
#include "Tactical/SoldierInitList.h"

BOOLEAN gfStrategicMilitiaChangesMade = FALSE;

static void RemoveMilitiaFromTactical();

void ResetMilitia() {
  if (gfStrategicMilitiaChangesMade || gTacticalStatus.uiFlags & LOADING_SAVED_GAME) {
    gfStrategicMilitiaChangesMade = FALSE;
    RemoveMilitiaFromTactical();
    PrepareMilitiaForTactical();
  }
}

static void RemoveMilitiaFromTactical() {
  FOR_EACH_IN_TEAM(i, MILITIA_TEAM) TacticalRemoveSoldier(*i);
  FOR_EACH_SOLDIERINITNODE(curr) {
    if (curr->pBasicPlacement->bTeam == MILITIA_TEAM) {
      curr->pSoldier = NULL;
    }
  }
}

void PrepareMilitiaForTactical() {
  SECTORINFO *pSector;
  //	int32_t i;
  uint8_t ubGreen, ubRegs, ubElites;
  if (gbWorldSectorZ > 0) return;

  // Do we have a loaded sector?
  if (gWorldSectorX == 0 && gWorldSectorY == 0) return;

  pSector = &SectorInfo[SECTOR(gWorldSectorX, gWorldSectorY)];
  ubGreen = pSector->ubNumberOfCivsAtLevel[GREEN_MILITIA];
  ubRegs = pSector->ubNumberOfCivsAtLevel[REGULAR_MILITIA];
  ubElites = pSector->ubNumberOfCivsAtLevel[ELITE_MILITIA];
  AddSoldierInitListMilitia(ubGreen, ubRegs, ubElites);
}

void HandleMilitiaPromotions() {
  gbGreenToElitePromotions = 0;
  gbGreenToRegPromotions = 0;
  gbRegToElitePromotions = 0;
  gbMilitiaPromotions = 0;

  FOR_EACH_IN_TEAM(i, MILITIA_TEAM) {
    SOLDIERTYPE &s = *i;
    if (!s.bInSector) continue;
    if (s.bLife <= 0) continue;
    if (s.ubMilitiaKills == 0) continue;

    uint8_t const militia_rank = SoldierClassToMilitiaRank(s.ubSoldierClass);
    uint8_t const promotions =
        CheckOneMilitiaForPromotion(gWorldSectorX, gWorldSectorY, militia_rank, s.ubMilitiaKills);
    if (promotions != 0) {
      if (promotions == 2) {
        ++gbGreenToElitePromotions;
        ++gbMilitiaPromotions;
      } else if (s.ubSoldierClass == SOLDIER_CLASS_GREEN_MILITIA) {
        ++gbGreenToRegPromotions;
        ++gbMilitiaPromotions;
      } else if (s.ubSoldierClass == SOLDIER_CLASS_REG_MILITIA) {
        ++gbRegToElitePromotions;
        ++gbMilitiaPromotions;
      }
    }

    s.ubMilitiaKills = 0;
  }
}
