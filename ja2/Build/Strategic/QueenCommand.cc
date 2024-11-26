#include "Strategic/QueenCommand.h"

#include <algorithm>
#include <stdexcept>

#include "Macro.h"
#include "SGP/Debug.h"
#include "SGP/FileMan.h"
#include "SGP/MemMan.h"
#include "SGP/Random.h"
#include "Strategic/Assignments.h"
#include "Strategic/AutoResolve.h"
#include "Strategic/CampaignInit.h"
#include "Strategic/CreatureSpreading.h"
#include "Strategic/GameClock.h"
#include "Strategic/GameEventHook.h"
#include "Strategic/LoadSaveUndergroundSectorInfo.h"
#include "Strategic/Meanwhile.h"
#include "Strategic/PreBattleInterface.h"
#include "Strategic/Quests.h"
#include "Strategic/Strategic.h"
#include "Strategic/StrategicAI.h"
#include "Strategic/StrategicEventHandler.h"
#include "Strategic/StrategicMap.h"
#include "Strategic/StrategicMovement.h"
#include "Strategic/StrategicPathing.h"
#include "Strategic/StrategicStatus.h"
#include "Strategic/StrategicTownLoyalty.h"
#include "Tactical/AnimationData.h"
#include "Tactical/Items.h"
#include "Tactical/Morale.h"
#include "Tactical/Overhead.h"
#include "Tactical/OverheadTypes.h"
#include "Tactical/SoldierAni.h"
#include "Tactical/SoldierInitList.h"
#include "Tactical/SoldierMacros.h"
#include "Tactical/SoldierProfile.h"
#include "Tactical/Squads.h"
#include "Tactical/TacticalSave.h"
#include "Tactical/Vehicles.h"
#include "TileEngine/MapEdgepoints.h"
#include "TileEngine/RenderWorld.h"
#include "Utils/FontControl.h"
#include "Utils/Message.h"

// The sector information required for the strategic AI.  Contains the number of
// enemy troops, as well as intentions, etc.
SECTORINFO SectorInfo[256];
UNDERGROUND_SECTORINFO *gpUndergroundSectorInfoHead = NULL;
extern UNDERGROUND_SECTORINFO *gpUndergroundSectorInfoTail;
BOOLEAN gfPendingEnemies = FALSE;

extern GARRISON_GROUP *gGarrisonGroup;

int16_t gsInterrogationGridNo[3] = {7756, 7757, 7758};

static void ValidateEnemiesHaveWeapons() {}

// Counts enemies and crepitus, but not bloodcats.
uint8_t NumHostilesInSector(int16_t sSectorX, int16_t sSectorY, int16_t sSectorZ) {
  uint8_t ubNumHostiles = 0;

  Assert(sSectorX >= 1 && sSectorX <= 16);
  Assert(sSectorY >= 1 && sSectorY <= 16);
  Assert(sSectorZ >= 0 && sSectorZ <= 3);

  if (sSectorZ) {
    UNDERGROUND_SECTORINFO *pSector;
    pSector = FindUnderGroundSector(sSectorX, sSectorY, (uint8_t)sSectorZ);
    if (pSector) {
      ubNumHostiles = (uint8_t)(pSector->ubNumAdmins + pSector->ubNumTroops + pSector->ubNumElites +
                                pSector->ubNumCreatures);
    }
  } else {
    SECTORINFO *pSector;

    // Count stationary hostiles
    pSector = &SectorInfo[SECTOR(sSectorX, sSectorY)];
    ubNumHostiles = (uint8_t)(pSector->ubNumAdmins + pSector->ubNumTroops + pSector->ubNumElites +
                              pSector->ubNumCreatures);

    // Count mobile enemies
    CFOR_EACH_ENEMY_GROUP(pGroup) {
      if (!pGroup->fVehicle && pGroup->ubSectorX == sSectorX && pGroup->ubSectorY == sSectorY) {
        ubNumHostiles += pGroup->ubGroupSize;
      }
    }
  }

  return ubNumHostiles;
}

uint8_t NumEnemiesInAnySector(int16_t sSectorX, int16_t sSectorY, int16_t sSectorZ) {
  uint8_t ubNumEnemies = 0;

  Assert(sSectorX >= 1 && sSectorX <= 16);
  Assert(sSectorY >= 1 && sSectorY <= 16);
  Assert(sSectorZ >= 0 && sSectorZ <= 3);

  if (sSectorZ) {
    UNDERGROUND_SECTORINFO *pSector;
    pSector = FindUnderGroundSector(sSectorX, sSectorY, (uint8_t)sSectorZ);
    if (pSector) {
      ubNumEnemies = (uint8_t)(pSector->ubNumAdmins + pSector->ubNumTroops + pSector->ubNumElites);
    }
  } else {
    SECTORINFO *pSector;

    // Count stationary enemies
    pSector = &SectorInfo[SECTOR(sSectorX, sSectorY)];
    ubNumEnemies = (uint8_t)(pSector->ubNumAdmins + pSector->ubNumTroops + pSector->ubNumElites);

    // Count mobile enemies
    CFOR_EACH_ENEMY_GROUP(pGroup) {
      if (!pGroup->fVehicle && pGroup->ubSectorX == sSectorX && pGroup->ubSectorY == sSectorY) {
        ubNumEnemies += pGroup->ubGroupSize;
      }
    }
  }

  return ubNumEnemies;
}

uint8_t NumEnemiesInSector(int16_t sSectorX, int16_t sSectorY) {
  SECTORINFO *pSector;
  uint8_t ubNumTroops;
  Assert(sSectorX >= 1 && sSectorX <= 16);
  Assert(sSectorY >= 1 && sSectorY <= 16);
  pSector = &SectorInfo[SECTOR(sSectorX, sSectorY)];
  ubNumTroops = (uint8_t)(pSector->ubNumAdmins + pSector->ubNumTroops + pSector->ubNumElites);

  CFOR_EACH_ENEMY_GROUP(pGroup) {
    if (!pGroup->fVehicle && pGroup->ubSectorX == sSectorX && pGroup->ubSectorY == sSectorY) {
      ubNumTroops += pGroup->ubGroupSize;
    }
  }
  return ubNumTroops;
}

uint8_t NumStationaryEnemiesInSector(int16_t sSectorX, int16_t sSectorY) {
  SECTORINFO *pSector;
  Assert(sSectorX >= 1 && sSectorX <= 16);
  Assert(sSectorY >= 1 && sSectorY <= 16);
  pSector = &SectorInfo[SECTOR(sSectorX, sSectorY)];

  if (pSector->ubGarrisonID == NO_GARRISON) {  // If no garrison, no stationary.
    return (0);
  }

  // don't count roadblocks as stationary garrison, we want to see how many
  // enemies are in them, not question marks
  if (gGarrisonGroup[pSector->ubGarrisonID].ubComposition == ROADBLOCK) {
    // pretend they're not stationary
    return (0);
  }

  return (uint8_t)(pSector->ubNumAdmins + pSector->ubNumTroops + pSector->ubNumElites);
}

uint8_t NumMobileEnemiesInSector(int16_t sSectorX, int16_t sSectorY) {
  SECTORINFO *pSector;
  uint8_t ubNumTroops;
  Assert(sSectorX >= 1 && sSectorX <= 16);
  Assert(sSectorY >= 1 && sSectorY <= 16);

  ubNumTroops = 0;
  CFOR_EACH_ENEMY_GROUP(pGroup) {
    if (!pGroup->fVehicle && pGroup->ubSectorX == sSectorX && pGroup->ubSectorY == sSectorY) {
      ubNumTroops += pGroup->ubGroupSize;
    }
  }

  pSector = &SectorInfo[SECTOR(sSectorX, sSectorY)];
  if (pSector->ubGarrisonID == ROADBLOCK) {  // consider these troops as mobile troops even though
                                             // they are in a garrison
    ubNumTroops += (uint8_t)(pSector->ubNumAdmins + pSector->ubNumTroops + pSector->ubNumElites);
  }

  return ubNumTroops;
}

static void GetNumberOfMobileEnemiesInSector(int16_t sSectorX, int16_t sSectorY,
                                             uint8_t *pubNumAdmins, uint8_t *pubNumTroops,
                                             uint8_t *pubNumElites) {
  SECTORINFO *pSector;
  Assert(sSectorX >= 1 && sSectorX <= 16);
  Assert(sSectorY >= 1 && sSectorY <= 16);

  // Now count the number of mobile groups in the sector.
  *pubNumTroops = *pubNumElites = *pubNumAdmins = 0;
  CFOR_EACH_ENEMY_GROUP(pGroup) {
    if (!pGroup->fVehicle && pGroup->ubSectorX == sSectorX && pGroup->ubSectorY == sSectorY) {
      *pubNumTroops += pGroup->pEnemyGroup->ubNumTroops;
      *pubNumElites += pGroup->pEnemyGroup->ubNumElites;
      *pubNumAdmins += pGroup->pEnemyGroup->ubNumAdmins;
    }
  }

  pSector = &SectorInfo[SECTOR(sSectorX, sSectorY)];
  if (pSector->ubGarrisonID == ROADBLOCK) {  // consider these troops as mobile troops even though
                                             // they are in a garrison
    *pubNumAdmins += pSector->ubNumAdmins;
    *pubNumTroops += pSector->ubNumTroops;
    *pubNumElites += pSector->ubNumElites;
  }
}

static void GetNumberOfStationaryEnemiesInSector(int16_t sSectorX, int16_t sSectorY,
                                                 uint8_t *pubNumAdmins, uint8_t *pubNumTroops,
                                                 uint8_t *pubNumElites) {
  SECTORINFO *pSector;
  Assert(sSectorX >= 1 && sSectorX <= 16);
  Assert(sSectorY >= 1 && sSectorY <= 16);
  pSector = &SectorInfo[SECTOR(sSectorX, sSectorY)];

  // grab the number of each type in the stationary sector
  *pubNumAdmins = pSector->ubNumAdmins;
  *pubNumTroops = pSector->ubNumTroops;
  *pubNumElites = pSector->ubNumElites;
}

void GetNumberOfEnemiesInSector(int16_t sSectorX, int16_t sSectorY, uint8_t *pubNumAdmins,
                                uint8_t *pubNumTroops, uint8_t *pubNumElites) {
  uint8_t ubNumAdmins, ubNumTroops, ubNumElites;

  GetNumberOfStationaryEnemiesInSector(sSectorX, sSectorY, pubNumAdmins, pubNumTroops,
                                       pubNumElites);

  GetNumberOfMobileEnemiesInSector(sSectorX, sSectorY, &ubNumAdmins, &ubNumTroops, &ubNumElites);

  *pubNumAdmins += ubNumAdmins;
  *pubNumTroops += ubNumTroops;
  *pubNumElites += ubNumElites;
}

static bool IsAnyOfTeamOKInSector(int8_t const team) {
  CFOR_EACH_IN_TEAM(i, team) {
    SOLDIERTYPE const &s = *i;
    if (s.bInSector && s.bLife >= OKLIFE) return true;
  }
  return false;
}

void EndTacticalBattleForEnemy() {
  int16_t const x = gWorldSectorX;
  int16_t const y = gWorldSectorY;
  int8_t const z = gbWorldSectorZ;
  // Clear enemies in battle for all stationary groups in the sector
  if (z == 0) {
    SECTORINFO &sector = SectorInfo[SECTOR(x, y)];
    sector.ubAdminsInBattle = 0;
    sector.ubTroopsInBattle = 0;
    sector.ubElitesInBattle = 0;
    sector.ubNumCreatures = 0;
    sector.ubCreaturesInBattle = 0;
  } else if (z > 0) {
    UNDERGROUND_SECTORINFO &sector = *FindUnderGroundSector(x, y, z);
    sector.ubAdminsInBattle = 0;
    sector.ubTroopsInBattle = 0;
    sector.ubElitesInBattle = 0;
  } else {   // Negative
    return;  // XXX exception?
  }

  /* Clear this value so that profiled enemies can be added into battles in the
   * future */
  gfProfiledEnemyAdded = FALSE;

  // Clear enemies in battle for all mobile groups in the sector
  CFOR_EACH_ENEMY_GROUP(i) {
    GROUP const &g = *i;
    if (g.fVehicle) continue;
    if (g.ubSectorX != x) continue;
    if (g.ubSectorY != y) continue;
    // XXX test for z missing?
    ENEMYGROUP &eg = *g.pEnemyGroup;
    eg.ubTroopsInBattle = 0;
    eg.ubElitesInBattle = 0;
    eg.ubAdminsInBattle = 0;
  }

  /* Check to see if any of our mercs have abandoned the militia during a
   * battle. This is cause for a rather severe loyalty blow. */
  if (IsAnyOfTeamOKInSector(MILITIA_TEAM) &&
      (IsAnyOfTeamOKInSector(ENEMY_TEAM) || IsAnyOfTeamOKInSector(CREATURE_TEAM))) {
    HandleGlobalLoyaltyEvent(GLOBAL_LOYALTY_ABANDON_MILITIA, x, y, 0);
  }
}

static uint8_t NumFreeEnemySlots() {
  uint8_t ubNumFreeSlots = 0;
  int32_t i;
  // Count the number of free enemy slots.  It is possible to have multiple
  // groups exceed the maximum.
  for (i = gTacticalStatus.Team[ENEMY_TEAM].bFirstID; i <= gTacticalStatus.Team[ENEMY_TEAM].bLastID;
       i++) {
    if (!GetMan(i).bActive) ++ubNumFreeSlots;
  }
  return ubNumFreeSlots;
}

static void PrepareEnemyForUndergroundBattle();

void PrepareEnemyForSectorBattle() {
  gfPendingEnemies = FALSE;

  if (gbWorldSectorZ > 0) {
    PrepareEnemyForUndergroundBattle();
    return;
  }

  GROUP *const bg = gpBattleGroup;
  if (bg && !bg->fPlayer) { /* The enemy has instigated the battle which means
                             * they are the ones entering the conflict. The
                             * player was actually in the sector first, and
                             * the enemy doesn't use reinforced placements */
    HandleArrivalOfReinforcements(bg);
    /* It is possible that other enemy groups have also arrived. Add them in the
     * same manner. */
    FOR_EACH_ENEMY_GROUP(g) {
      if (g == bg) continue;
      if (g->fVehicle) continue;
      if (g->ubSectorX != bg->ubSectorX) continue;
      if (g->ubSectorY != bg->ubSectorY) continue;
      if (g->pEnemyGroup->ubAdminsInBattle != 0) continue;
      if (g->pEnemyGroup->ubTroopsInBattle != 0) continue;
      if (g->pEnemyGroup->ubElitesInBattle != 0) continue;
      HandleArrivalOfReinforcements(g);
    }
    ValidateEnemiesHaveWeapons();
    return;
  }

  int16_t const x = gWorldSectorX;
  int16_t const y = gWorldSectorY;

  if (gbWorldSectorZ == 0 && NumEnemiesInSector(x, y) > 32) {
    gfPendingEnemies = TRUE;
  }

  uint8_t total_admins;
  uint8_t total_troops;
  uint8_t total_elites;
  SECTORINFO &sector = SectorInfo[SECTOR(x, y)];
  if (sector.uiFlags & SF_USE_MAP_SETTINGS) {  // Count the number of enemy placements in a map
                                               // and use those
    total_admins = 0;
    total_troops = 0;
    total_elites = 0;
    CFOR_EACH_SOLDIERINITNODE(curr) {
      BASIC_SOLDIERCREATE_STRUCT const &bp = *curr->pBasicPlacement;
      if (bp.bTeam != ENEMY_TEAM) continue;
      switch (bp.ubSoldierClass) {
        case SOLDIER_CLASS_ADMINISTRATOR:
          ++total_admins;
          break;
        case SOLDIER_CLASS_ARMY:
          ++total_troops;
          break;
        case SOLDIER_CLASS_ELITE:
          ++total_elites;
          break;
      }
    }
    sector.ubNumAdmins = total_admins;
    sector.ubNumTroops = total_troops;
    sector.ubNumElites = total_elites;
    sector.ubAdminsInBattle = 0;
    sector.ubTroopsInBattle = 0;
    sector.ubElitesInBattle = 0;
  } else {
    total_admins = sector.ubNumAdmins - sector.ubAdminsInBattle;
    total_troops = sector.ubNumTroops - sector.ubTroopsInBattle;
    total_elites = sector.ubNumElites - sector.ubElitesInBattle;
  }

  uint8_t const n_stationary_enemies = total_admins + total_troops + total_elites;
  if (n_stationary_enemies > 32) {
    total_admins = std::min(total_admins, (uint8_t)32);
    total_troops = std::min(total_troops, (uint8_t)(32 - total_admins));
    total_elites = std::min(total_elites, (uint8_t)(32 - total_admins + total_troops));
  }

  sector.ubAdminsInBattle += total_admins;
  sector.ubTroopsInBattle += total_troops;
  sector.ubElitesInBattle += total_elites;

  // Search for movement groups that happen to be in the sector.
  int16_t n_slots = NumFreeEnemySlots();
  // Test:  All slots should be free at this point!
  if (n_slots !=
      gTacticalStatus.Team[ENEMY_TEAM].bLastID - gTacticalStatus.Team[ENEMY_TEAM].bFirstID + 1) {
  }
  /* Subtract the total number of stationary enemies from the available slots,
   * as stationary forces take precendence in combat. The mobile forces that
   * could also be in the same sector are considered later if all the slots fill
   * up. */
  n_slots -= total_admins + total_troops + total_elites;
  /* Now, process all of the groups and search for both enemy and player groups
   * in the sector. For enemy groups, we fill up the slots until we have none
   * left or all of the groups have been processed. */
  FOR_EACH_GROUP(g) {
    if (n_slots == 0) break;

    if (g->fVehicle) continue;
    if (g->ubSectorX != x) continue;
    if (g->ubSectorY != y) continue;
    if (gbWorldSectorZ != 0) continue;

    if (!g->fPlayer) {  // Process enemy group in sector
      ENEMYGROUP &eg = *g->pEnemyGroup;
      if (n_slots > 0) {
        uint8_t n_admins = eg.ubNumAdmins - eg.ubAdminsInBattle;
        n_slots -= n_admins;
        if (n_slots < 0) {  // Adjust the value to zero
          n_admins += n_slots;
          n_slots = 0;
          gfPendingEnemies = TRUE;
        }
        eg.ubAdminsInBattle += n_admins;
        total_admins += n_admins;
      }
      if (n_slots > 0) {  // Add regular army forces
        uint8_t n_troops = eg.ubNumTroops - eg.ubTroopsInBattle;
        n_slots -= n_troops;
        if (n_slots < 0) {  // Adjust the value to zero
          n_troops += n_slots;
          n_slots = 0;
          gfPendingEnemies = TRUE;
        }
        eg.ubTroopsInBattle += n_troops;
        total_troops += n_troops;
      }
      if (n_slots > 0) {  // Add elite troops
        uint8_t n_elites = eg.ubNumElites - eg.ubElitesInBattle;
        n_slots -= n_elites;
        if (n_slots < 0) {  // Adjust the value to zero
          n_elites += n_slots;
          n_slots = 0;
          gfPendingEnemies = TRUE;
        }
        eg.ubElitesInBattle += n_elites;
        total_elites += n_elites;
      }
      // NOTE: No provisions for profile troop leader or retreat groups yet
    } else if (!g->fBetweenSectors) { /* TEMP: The player path needs to get
                                       * destroyed, otherwise, it'll be
                                       * impossible to move the group after the
                                       * battle is resolved. */
      // XXX TODO001F This does not work, when n_slots drops to 0 before all
      // player groups are handled.

      // No one in the group any more continue loop
      if (!g->pPlayerList) continue;

      RemoveGroupWaypoints(*g);
    }
  }

  // If there are no troops in the current groups, then we're done.
  if (total_admins == 0 && total_troops == 0 && total_elites == 0) return;

  AddSoldierInitListEnemyDefenceSoldiers(total_admins, total_troops, total_elites);

  /* Now, we have to go through all of the enemies in the new map and assign
   * their respective groups if in a mobile group, but only for the ones that
   * were assigned from the */
  n_slots = 32 - n_stationary_enemies;
  CFOR_EACH_ENEMY_GROUP(g) {
    if (n_slots == 0) break;

    if (g->fVehicle) continue;
    if (g->ubSectorX != x) continue;
    if (g->ubSectorY != y) continue;
    if (gbWorldSectorZ != 0) continue;

    int32_t n = g->ubGroupSize;
    uint8_t n_admins = g->pEnemyGroup->ubAdminsInBattle;
    uint8_t n_troops = g->pEnemyGroup->ubTroopsInBattle;
    uint8_t n_elites = g->pEnemyGroup->ubElitesInBattle;
    FOR_EACH_IN_TEAM(s, ENEMY_TEAM) {
      if (n == 0 || n_slots == 0) break;
      if (s->ubGroupID != 0) continue;

      switch (s->ubSoldierClass) {
        case SOLDIER_CLASS_ADMINISTRATOR:
          if (n_admins == 0) continue;
          --n_admins;
          break;

        case SOLDIER_CLASS_ARMY:
          if (n_troops == 0) continue;
          --n_troops;
          break;

        case SOLDIER_CLASS_ELITE:
          if (n_elites == 0) continue;
          --n_elites;
          break;

        default:
          continue;
      }

      --n;
      --n_slots;
      s->ubGroupID = g->ubGroupID;
    }
    AssertMsg(n == 0 || n_slots == 0,
              "Failed to assign battle counters for enemies properly. Please "
              "send save. KM:0.");
  }

  ValidateEnemiesHaveWeapons();
}

static void PrepareEnemyForUndergroundBattle() {
  // This is the sector we are going to be fighting in.
  UNDERGROUND_SECTORINFO *const u =
      FindUnderGroundSector(gWorldSectorX, gWorldSectorY, gbWorldSectorZ);
  Assert(u);
  if (!u) return;

  if (u->ubNumAdmins == 0 && u->ubNumTroops == 0 && u->ubNumElites == 0) return;

  uint8_t const ubTotalAdmins = u->ubNumAdmins - u->ubAdminsInBattle;
  uint8_t const ubTotalTroops = u->ubNumTroops - u->ubTroopsInBattle;
  uint8_t const ubTotalElites = u->ubNumElites - u->ubElitesInBattle;
  u->ubAdminsInBattle += ubTotalAdmins;
  u->ubTroopsInBattle += ubTotalTroops;
  u->ubElitesInBattle += ubTotalElites;
  AddSoldierInitListEnemyDefenceSoldiers(u->ubNumAdmins, u->ubNumTroops, u->ubNumElites);
  ValidateEnemiesHaveWeapons();
}

// The queen AI layer must process the event by subtracting forces, etc.
void ProcessQueenCmdImplicationsOfDeath(const SOLDIERTYPE *const pSoldier) {
  EvaluateDeathEffectsToSoldierInitList(*pSoldier);

  switch (pSoldier->ubProfile) {
    case MIKE:
    case IGGY:
      if (pSoldier->ubProfile == IGGY &&
          !gubFact[FACT_IGGY_AVAILABLE_TO_ARMY]) {  // Iggy is on our team!
        break;
      }
      if (!pSoldier->bSectorZ) {
        SECTORINFO *pSector = &SectorInfo[SECTOR(pSoldier->sSectorX, pSoldier->sSectorY)];
        if (pSector->ubNumElites) {
          pSector->ubNumElites--;
        }
        if (pSector->ubElitesInBattle) {
          pSector->ubElitesInBattle--;
        }
      } else {
        UNDERGROUND_SECTORINFO *pUnderground;
        pUnderground = FindUnderGroundSector(
            (uint8_t)pSoldier->sSectorX, (uint8_t)pSoldier->sSectorY, (uint8_t)pSoldier->bSectorZ);
        Assert(pUnderground);
        if (pUnderground->ubNumElites) {
          pUnderground->ubNumElites--;
        }
        if (pUnderground->ubElitesInBattle) {
          pUnderground->ubElitesInBattle--;
        }
      }
      break;
  }

  if (pSoldier->bNeutral || (pSoldier->bTeam != ENEMY_TEAM && pSoldier->bTeam != CREATURE_TEAM))
    return;
  // we are recording an enemy death
  if (pSoldier->ubGroupID) {  // The enemy was in a mobile group
    GROUP *pGroup;
    pGroup = GetGroup(pSoldier->ubGroupID);
    if (!pGroup) {
      return;
    }
    if (pGroup->fPlayer) {
      return;
    }
    switch (pSoldier->ubSoldierClass) {
      case SOLDIER_CLASS_ELITE:
        if (pGroup->pEnemyGroup->ubNumElites) {
          pGroup->pEnemyGroup->ubNumElites--;
        }
        if (pGroup->pEnemyGroup->ubElitesInBattle) {
          pGroup->pEnemyGroup->ubElitesInBattle--;
        }
        break;
      case SOLDIER_CLASS_ARMY:
        if (pGroup->pEnemyGroup->ubNumTroops) {
          pGroup->pEnemyGroup->ubNumTroops--;
        }
        if (pGroup->pEnemyGroup->ubTroopsInBattle) {
          pGroup->pEnemyGroup->ubTroopsInBattle--;
        }
        break;
      case SOLDIER_CLASS_ADMINISTRATOR:
        if (pGroup->pEnemyGroup->ubNumAdmins) {
          pGroup->pEnemyGroup->ubNumAdmins--;
        }
        if (pGroup->pEnemyGroup->ubAdminsInBattle) {
          pGroup->pEnemyGroup->ubAdminsInBattle--;
        }
        break;
    }
    if (pGroup->ubGroupSize) pGroup->ubGroupSize--;
    RecalculateGroupWeight(*pGroup);
    if (!pGroup->ubGroupSize) {
      RemoveGroup(*pGroup);
    }
  } else {                                           // The enemy was in a stationary defence group
    if (!gbWorldSectorZ || IsAutoResolveActive()) {  // ground level (SECTORINFO)
      SECTORINFO *pSector;

      if (!IsAutoResolveActive()) {
        pSector = &SectorInfo[SECTOR(pSoldier->sSectorX, pSoldier->sSectorY)];
      } else {
        pSector = &SectorInfo[GetAutoResolveSectorID()];
      }

      switch (pSoldier->ubSoldierClass) {
        case SOLDIER_CLASS_ADMINISTRATOR:
          if (pSector->ubNumAdmins) {
            pSector->ubNumAdmins--;
          }
          if (pSector->ubAdminsInBattle) {
            pSector->ubAdminsInBattle--;
          }
          break;
        case SOLDIER_CLASS_ARMY:
          if (pSector->ubNumTroops) {
            pSector->ubNumTroops--;
          }
          if (pSector->ubTroopsInBattle) {
            pSector->ubTroopsInBattle--;
          }
          break;
        case SOLDIER_CLASS_ELITE:
          if (pSector->ubNumElites) {
            pSector->ubNumElites--;
          }
          if (pSector->ubElitesInBattle) {
            pSector->ubElitesInBattle--;
          }
          break;
        case SOLDIER_CLASS_CREATURE:
          if (pSoldier->ubBodyType != BLOODCAT) {
            if (pSector->ubNumCreatures) {
              pSector->ubNumCreatures--;
            }
            if (pSector->ubCreaturesInBattle) {
              pSector->ubCreaturesInBattle--;
            }
          } else {
            if (pSector->bBloodCats) {
              pSector->bBloodCats--;
            }
          }

          break;
      }
      RecalculateSectorWeight((uint8_t)SECTOR(pSoldier->sSectorX, pSoldier->sSectorY));
    } else {  // basement level (UNDERGROUND_SECTORINFO)
      UNDERGROUND_SECTORINFO *pSector =
          FindUnderGroundSector(gWorldSectorX, gWorldSectorY, gbWorldSectorZ);
      if (pSector) {
        switch (pSoldier->ubSoldierClass) {
          case SOLDIER_CLASS_ADMINISTRATOR:
            if (pSector->ubNumAdmins) {
              pSector->ubNumAdmins--;
            }
            if (pSector->ubAdminsInBattle) {
              pSector->ubAdminsInBattle--;
            }
            break;
          case SOLDIER_CLASS_ARMY:
            if (pSector->ubNumTroops) {
              pSector->ubNumTroops--;
            }
            if (pSector->ubTroopsInBattle) {
              pSector->ubTroopsInBattle--;
            }
            break;
          case SOLDIER_CLASS_ELITE:
            if (pSector->ubNumElites) {
              pSector->ubNumElites--;
            }
            if (pSector->ubElitesInBattle) {
              pSector->ubElitesInBattle--;
            }
            break;
          case SOLDIER_CLASS_CREATURE:
            if (pSector->ubNumCreatures) {
              pSector->ubNumCreatures--;
            }
            if (pSector->ubCreaturesInBattle) {
              pSector->ubCreaturesInBattle--;
            }

            if (!pSector->ubNumCreatures && gWorldSectorX != 9 &&
                gWorldSectorY != 10) {  // If the player has successfully killed all creatures
                                        // in ANY underground sector except J9
              // then cancel any pending creature town attack.
              DeleteAllStrategicEventsOfType(EVENT_CREATURE_ATTACK);
            }

            // a monster has died.  Post an event to immediately check whether a
            // mine has been cleared.
            AddStrategicEventUsingSeconds(EVENT_CHECK_IF_MINE_CLEARED, GetWorldTotalSeconds() + 15,
                                          0);

            if (pSoldier->ubBodyType == QUEENMONSTER) {
              // Need to call this, as the queen is really big, and killing her
              // leaves a bunch of bad tiles in behind her.  Calling this function
              // cleans it up.
              InvalidateWorldRedundency();
              // Now that the queen is dead, turn off the creature quest.
              EndCreatureQuest();
              EndQuest(QUEST_CREATURES, gWorldSectorX, gWorldSectorY);
            }
            break;
        }
      }
    }
  }
}

static void AddEnemiesToBattle(GROUP const &, uint8_t strategic_insertion_code, uint8_t n_admins,
                               uint8_t n_troops, uint8_t n_elites);

/* Rarely, there will be more enemies than supported by the engine. In this
 * case, these soldier's are waiting for a slot to be free so that they can
 * enter the battle. This essentially allows for an infinite number of troops,
 * though only 32 at a time can fight. This is also called whenever an enemy
 * group's reinforcements arrive because the code is identical, though it is
 * highly likely that they will all be successfully added on the first call. */
void AddPossiblePendingEnemiesToBattle() {
  if (!gfPendingEnemies) { /* Optimization: No point in checking if we know that
                            * there aren't any more enemies that can be added to
                            * this battle. This changes whenever a new enemy
                            * group arrives at the scene. */
    return;
  }

  uint8_t n_slots = NumFreeEnemySlots();
  CFOR_EACH_ENEMY_GROUP(i) {
    if (n_slots == 0) break;

    GROUP const &g = *i;
    if (g.fVehicle) continue;
    if (g.ubSectorX != gWorldSectorX) continue;
    if (g.ubSectorY != gWorldSectorY) continue;
    if (gbWorldSectorZ != 0) continue;
    // This enemy group is currently in the sector.
    ENEMYGROUP &eg = *g.pEnemyGroup;
    uint8_t n_elites = 0;
    uint8_t n_troops = 0;
    uint8_t n_admins = 0;
    uint8_t n_available =
        g.ubGroupSize - eg.ubElitesInBattle - eg.ubTroopsInBattle - eg.ubAdminsInBattle;
    while (n_available != 0 && n_slots != 0) {     // This group has enemies waiting for a chance to
                                                   // enter the battle.
      if (eg.ubTroopsInBattle < eg.ubNumTroops) {  // Add a regular troop.
        ++eg.ubTroopsInBattle;
        ++n_troops;
      } else if (eg.ubElitesInBattle < eg.ubNumElites) {  // Add an elite troop.
        ++eg.ubElitesInBattle;
        ++n_elites;
      } else if (eg.ubAdminsInBattle < eg.ubNumAdmins) {  // Add an admin troop.
        ++eg.ubAdminsInBattle;
        ++n_admins;
      } else {
        throw std::logic_error("AddPossiblePendingEnemiesToBattle(): Logic Error");
      }
      --n_available;
      --n_slots;
    }

    if (n_admins != 0 || n_troops != 0 ||
        n_elites != 0) { /* This group has contributed forces, then add them
                          * now, because different groups appear on different
                          * sides of the map. */
      uint8_t strategic_insertion_code = 0;
      // First, determine which entrypoint to use, based on the travel direction
      // of the group.
      if (g.ubPrevX != 0 && g.ubPrevY != 0) {
        strategic_insertion_code = g.ubSectorX < g.ubPrevX   ? INSERTION_CODE_EAST
                                   : g.ubSectorX > g.ubPrevX ? INSERTION_CODE_WEST
                                   : g.ubSectorY < g.ubPrevY ? INSERTION_CODE_SOUTH
                                   : g.ubSectorY > g.ubPrevY ? INSERTION_CODE_NORTH
                                                             : 0;  // XXX exception?
      } else if (g.ubNextX != 0 && g.ubNextY != 0) {
        strategic_insertion_code = g.ubSectorX < g.ubNextX   ? INSERTION_CODE_EAST
                                   : g.ubSectorX > g.ubNextX ? INSERTION_CODE_WEST
                                   : g.ubSectorY < g.ubNextY ? INSERTION_CODE_SOUTH
                                   : g.ubSectorY > g.ubNextY ? INSERTION_CODE_NORTH
                                                             : 0;  // XXX exception?
      }  // XXX else exception?
      /* Add the number of each type of troop and place them in the appropriate
       * positions */
      AddEnemiesToBattle(g, strategic_insertion_code, n_admins, n_troops, n_elites);
    }
  }

  if (n_slots != 0) { /* After going through the process, we have finished with some free
                       * slots and no more enemies to add. So, we can turn off the flag, as
                       * this check is no longer needed. */
    gfPendingEnemies = FALSE;
  }
}

static void AddEnemiesToBattle(GROUP const &g, uint8_t const strategic_insertion_code,
                               uint8_t n_admins, uint8_t n_troops, uint8_t n_elites) {
  uint8_t desired_direction;
  switch (strategic_insertion_code) {
    case INSERTION_CODE_NORTH:
      desired_direction = SOUTHEAST;
      break;
    case INSERTION_CODE_EAST:
      desired_direction = SOUTHWEST;
      break;
    case INSERTION_CODE_SOUTH:
      desired_direction = NORTHWEST;
      break;
    case INSERTION_CODE_WEST:
      desired_direction = NORTHEAST;
      break;
    default:
      throw std::logic_error("Invalid direction passed to AddEnemiesToBattle()");
  }

  uint8_t n_total = n_admins + n_troops + n_elites;
  uint8_t curr_slot = 0;
  MAPEDGEPOINTINFO edgepoint_info;
  ChooseMapEdgepoints(&edgepoint_info, strategic_insertion_code, n_total);
  while (n_total != 0) {
    uint32_t const roll = Random(n_total--);
    SoldierClass const sc = roll < n_elites ? --n_elites,
                       SOLDIER_CLASS_ELITE : roll < n_elites + n_troops ? --n_troops,
                       SOLDIER_CLASS_ARMY : (--n_admins, SOLDIER_CLASS_ADMINISTRATOR);

    SOLDIERTYPE &s = *TacticalCreateEnemySoldier(sc);
    s.ubGroupID = g.ubGroupID;
    s.ubInsertionDirection = desired_direction;
    // Setup the position
    if (curr_slot < edgepoint_info.ubNumPoints) {  // Use an edgepoint
      s.ubStrategicInsertionCode = INSERTION_CODE_GRIDNO;
      s.usStrategicInsertionData = edgepoint_info.sGridNo[curr_slot++];
    } else {  // No edgepoints left, so put him at the entrypoint
      s.ubStrategicInsertionCode = strategic_insertion_code;
    }
    UpdateMercInSector(s, gWorldSectorX, gWorldSectorY, 0);
  }
  Assert(n_admins == 0);
  Assert(n_troops == 0);
  Assert(n_elites == 0);
}

void SaveUnderGroundSectorInfoToSaveGame(HWFILE const f) {
  // Save the number of nodes
  uint32_t n_records = 0;
  for (UNDERGROUND_SECTORINFO const *i = gpUndergroundSectorInfoHead; i; i = i->next) {
    ++n_records;
  }
  FileWrite(f, &n_records, sizeof(uint32_t));

  // Save the nodes
  for (UNDERGROUND_SECTORINFO const *i = gpUndergroundSectorInfoHead; i; i = i->next) {
    InjectUndergroundSectorInfoIntoFile(f, i);
  }
}

void LoadUnderGroundSectorInfoFromSavedGame(HWFILE const f) {
  TrashUndergroundSectorInfo();

  // Read the number of nodes stored
  uint32_t n_records;
  FileRead(f, &n_records, sizeof(uint32_t));

  UNDERGROUND_SECTORINFO **anchor = &gpUndergroundSectorInfoHead;
  for (uint32_t n = n_records; n != 0; --n) {
    UNDERGROUND_SECTORINFO *const u = MALLOC(UNDERGROUND_SECTORINFO);
    ExtractUndergroundSectorInfoFromFile(f, u);

    gpUndergroundSectorInfoTail = u;
    *anchor = u;
    anchor = &u->next;
  }
}

UNDERGROUND_SECTORINFO *FindUnderGroundSector(int16_t const x, int16_t const y, uint8_t const z) {
  UNDERGROUND_SECTORINFO *i = gpUndergroundSectorInfoHead;
  for (; i; i = i->next) {
    if (i->ubSectorX != x) continue;
    if (i->ubSectorY != y) continue;
    if (i->ubSectorZ != z) continue;
    break;
  }
  return i;
}

void BeginCaptureSquence() {
  if (!(gStrategicStatus.uiFlags & STRATEGIC_PLAYER_CAPTURED_FOR_RESCUE) ||
      !(gStrategicStatus.uiFlags & STRATEGIC_PLAYER_CAPTURED_FOR_ESCAPE)) {
    gStrategicStatus.ubNumCapturedForRescue = 0;
  }
}

void EndCaptureSequence() {
  // Set flag...
  if (!(gStrategicStatus.uiFlags & STRATEGIC_PLAYER_CAPTURED_FOR_RESCUE) ||
      !(gStrategicStatus.uiFlags & STRATEGIC_PLAYER_CAPTURED_FOR_ESCAPE)) {
    // CJC Dec 1 2002: fixing multiple captures:
    // gStrategicStatus.uiFlags |= STRATEGIC_PLAYER_CAPTURED_FOR_RESCUE;

    if (gubQuest[QUEST_HELD_IN_ALMA] == QUESTNOTSTARTED) {
      // CJC Dec 1 2002: fixing multiple captures:
      gStrategicStatus.uiFlags |= STRATEGIC_PLAYER_CAPTURED_FOR_RESCUE;
      StartQuest(QUEST_HELD_IN_ALMA, gWorldSectorX, gWorldSectorY);
    }
    // CJC Dec 1 2002: fixing multiple captures:
    // else if ( gubQuest[ QUEST_HELD_IN_ALMA ] == QUESTDONE )
    else if (gubQuest[QUEST_HELD_IN_ALMA] == QUESTDONE &&
             gubQuest[QUEST_INTERROGATION] == QUESTNOTSTARTED) {
      StartQuest(QUEST_INTERROGATION, gWorldSectorX, gWorldSectorY);
      // CJC Dec 1 2002: fixing multiple captures:
      gStrategicStatus.uiFlags |= STRATEGIC_PLAYER_CAPTURED_FOR_ESCAPE;

      ScheduleMeanwhileEvent(7, 14, 0, INTERROGATION, QUEEN, 10);
    }
    // CJC Dec 1 2002: fixing multiple captures
    else {
      // !?!? set both flags
      gStrategicStatus.uiFlags |= STRATEGIC_PLAYER_CAPTURED_FOR_RESCUE;
      gStrategicStatus.uiFlags |= STRATEGIC_PLAYER_CAPTURED_FOR_ESCAPE;
    }
  }
}

static void CaptureSoldier(SOLDIERTYPE *const s, int16_t const x, int16_t const y,
                           GridNo const soldier_pos, GridNo const item_pos) {
  s->sSectorX = x;
  s->sSectorY = y;
  s->bSectorZ = 0;
  s->bLevel = 0;  // put him on the floor
  s->ubStrategicInsertionCode = INSERTION_CODE_GRIDNO;
  s->usStrategicInsertionData = soldier_pos;

  // Drop all items
  FOR_EACH_SOLDIER_INV_SLOT(i, *s) {
    OBJECTTYPE &o = *i;
    if (o.usItem == NOTHING) continue;

    AddItemsToUnLoadedSector(x, y, 0, item_pos, 1, &o, 0, 0, 0, VISIBILITY_0);
    DeleteObj(&o);
  }
}

void EnemyCapturesPlayerSoldier(SOLDIERTYPE *pSoldier) {
  BOOLEAN fMadeCorpse;
  int32_t iNumEnemiesInSector;

  static int16_t sAlmaCaptureGridNos[] = {9208, 9688, 9215};
  static int16_t sAlmaCaptureItemsGridNo[] = {12246, 12406, 13046};

  static int16_t sInterrogationItemGridNo[] = {12089, 12089, 12089};

  // ATE: Check first if ! in player captured sequence already
  // CJC Dec 1 2002: fixing multiple captures
  if ((gStrategicStatus.uiFlags & STRATEGIC_PLAYER_CAPTURED_FOR_RESCUE) &&
      (gStrategicStatus.uiFlags & STRATEGIC_PLAYER_CAPTURED_FOR_ESCAPE)) {
    return;
  }

  // ATE: If maximum prisoners captured, return!
  if (gStrategicStatus.ubNumCapturedForRescue > 3) {
    return;
  }

  // If this is an EPC , just kill them...
  if (AM_AN_EPC(pSoldier)) {
    pSoldier->bLife = 0;
    HandleSoldierDeath(pSoldier, &fMadeCorpse);
    return;
  }

  if (pSoldier->uiStatusFlags & SOLDIER_VEHICLE) {
    return;
  }

  // ATE: Patch fix If in a vehicle, remove from vehicle...
  TakeSoldierOutOfVehicle(pSoldier);

  // Are there anemies in ALMA? ( I13 )
  iNumEnemiesInSector = NumEnemiesInSector(13, 9);

  // IF there are no enemies, and we need to do alma, skip!
  if (gubQuest[QUEST_HELD_IN_ALMA] == QUESTNOTSTARTED && iNumEnemiesInSector == 0) {
    InternalStartQuest(QUEST_HELD_IN_ALMA, gWorldSectorX, gWorldSectorY, FALSE);
    InternalEndQuest(QUEST_HELD_IN_ALMA, gWorldSectorX, gWorldSectorY, FALSE);
  }

  HandleMoraleEvent(pSoldier, MORALE_MERC_CAPTURED, pSoldier->sSectorX, pSoldier->sSectorY,
                    pSoldier->bSectorZ);

  // Change to POW....
  //-add him to a POW assignment/group
  if ((pSoldier->bAssignment != ASSIGNMENT_POW)) {
    SetTimeOfAssignmentChangeForMerc(pSoldier);
  }

  ChangeSoldiersAssignment(pSoldier, ASSIGNMENT_POW);
  // ATE: Make them neutral!
  if (gubQuest[QUEST_HELD_IN_ALMA] == QUESTNOTSTARTED) {
    pSoldier->bNeutral = TRUE;
  }

  RemoveCharacterFromSquads(pSoldier);

  // Is this the first one..?
  if (gubQuest[QUEST_HELD_IN_ALMA] == QUESTNOTSTARTED) {
    uint8_t &idx = gStrategicStatus.ubNumCapturedForRescue;
    // Teleport him to NE Alma sector (not Tixa as originally planned)
    CaptureSoldier(pSoldier, 13, 9, sAlmaCaptureGridNos[idx], sAlmaCaptureItemsGridNo[idx]);
    ++idx;
  } else if (gubQuest[QUEST_HELD_IN_ALMA] == QUESTDONE) {
    // Teleport him to N7
    uint8_t &idx = gStrategicStatus.ubNumCapturedForRescue;
    CaptureSoldier(pSoldier, 7, 14, gsInterrogationGridNo[idx], sInterrogationItemGridNo[idx]);
    ++idx;
  }

  // Bandaging him would prevent him from dying (due to low HP)
  pSoldier->bBleeding = 0;

  // wake him up
  if (pSoldier->fMercAsleep) {
    PutMercInAwakeState(pSoldier);
    pSoldier->fForcedToStayAwake = FALSE;
  }

  // Set his life to 50% + or - 10 HP.
  pSoldier->bLife = pSoldier->bLifeMax / 2;
  if (pSoldier->bLife <= 35) {
    pSoldier->bLife = 35;
  } else if (pSoldier->bLife >= 45) {
    pSoldier->bLife += (int8_t)(10 - Random(21));
  }

  // make him quite exhausted when found
  pSoldier->bBreath = pSoldier->bBreathMax = 50;
  pSoldier->sBreathRed = 0;
  pSoldier->fMercCollapsedFlag = FALSE;
}

BOOLEAN PlayerSectorDefended(uint8_t ubSectorID) {
  SECTORINFO *pSector;
  pSector = &SectorInfo[ubSectorID];
  if (pSector->ubNumberOfCivsAtLevel[GREEN_MILITIA] +
      pSector->ubNumberOfCivsAtLevel[REGULAR_MILITIA] +
      pSector->ubNumberOfCivsAtLevel[ELITE_MILITIA]) {  // militia in sector
    return TRUE;
  }
  // Player in sector?
  return FindPlayerMovementGroupInSector(SECTORX(ubSectorID), SECTORY(ubSectorID)) != NULL;
}

static BOOLEAN AnyNonNeutralOfTeamInSector(int8_t team) {
  CFOR_EACH_IN_TEAM(s, team) {
    if (s->bInSector && s->bLife != 0 && !s->bNeutral) {
      return TRUE;
    }
  }
  return FALSE;
}

// Assumes gTacticalStatus.fEnemyInSector
BOOLEAN OnlyHostileCivsInSector() {
  // Look for any hostile civs.
  if (!AnyNonNeutralOfTeamInSector(CIV_TEAM)) return FALSE;
  // Look for anybody else hostile.  If found, return FALSE immediately.
  if (AnyNonNeutralOfTeamInSector(ENEMY_TEAM)) return FALSE;
  if (AnyNonNeutralOfTeamInSector(CREATURE_TEAM)) return FALSE;
  if (AnyNonNeutralOfTeamInSector(MILITIA_TEAM)) return FALSE;
  // We only have hostile civilians, don't allow time compression.
  return TRUE;
}
