// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Strategic/PlayerCommand.h"

#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "GameSettings.h"
#include "Laptop/EMail.h"
#include "Laptop/LaptopSave.h"
#include "Strategic/CampaignTypes.h"
#include "Strategic/GameClock.h"
#include "Strategic/MapScreen.h"
#include "Strategic/MapScreenHelicopter.h"
#include "Strategic/MapScreenInterface.h"
#include "Strategic/MapScreenInterfaceBottom.h"
#include "Strategic/Meanwhile.h"
#include "Strategic/PreBattleInterface.h"
#include "Strategic/QueenCommand.h"
#include "Strategic/Quests.h"
#include "Strategic/StrategicAI.h"
#include "Strategic/StrategicMap.h"
#include "Strategic/StrategicMines.h"
#include "Strategic/StrategicStatus.h"
#include "Strategic/StrategicTownLoyalty.h"
#include "Tactical/Morale.h"
#include "Tactical/Overhead.h"
#include "Tactical/TacticalSave.h"
#include "Utils/Text.h"

void GetSectorFacilitiesFlags(int16_t const x, int16_t const y, wchar_t *const buf,
                              size_t const length) {
  // Build a string stating current facilities present in sector
  uint32_t const facilities = SectorInfo[SECTOR(x, y)].uiFacilitiesFlags;
  if (facilities == 0) {
    wcsncpy(buf, sFacilitiesStrings[0], length);
    return;
  }

  wchar_t const *fmt = L"%ls";
  size_t n = 0;
  for (size_t i = 0;; ++i) {
    uint32_t const bit = 1 << i;
    if (!(facilities & bit)) continue;
    n += swprintf(buf + n, length - n, fmt, sFacilitiesStrings[i + 1]);
    fmt = L",%ls";
    if ((facilities & ~(bit - 1)) == bit) break;
  }
}

// ALL changes of control to player must be funneled through here!
BOOLEAN SetThisSectorAsPlayerControlled(int16_t sMapX, int16_t sMapY, int8_t bMapZ,
                                        BOOLEAN fContested) {
  // NOTE: MapSector must be 16-bit, cause MAX_WORLD_X is actually 18, so the
  // sector numbers exceed 256 although we use only 16x16
  uint16_t usMapSector = 0;
  BOOLEAN fWasEnemyControlled = FALSE;
  int8_t bTownId = 0;

  if (AreInMeanwhile()) {
    return FALSE;
  }

  uint8_t const sector = SECTOR(sMapX, sMapY);
  if (bMapZ == 0) {
    usMapSector = sMapX + (sMapY * MAP_WORLD_X);

    /*
                    // if enemies formerly controlled this sector
                    if (StrategicMap[ usMapSector ].fEnemyControlled)
                    {
                            // remember that the enemies have lost it
                            StrategicMap[ usMapSector ].fLostControlAtSomeTime =
       TRUE;
                    }
    */
    if (NumHostilesInSector(sMapX, sMapY, bMapZ)) {  // too premature:  enemies still in sector.
      return FALSE;
    }

    // check if we ever grabbed drassen airport, if so, set fact we can go to
    // BR's
    if ((sMapX == BOBBYR_SHIPPING_DEST_SECTOR_X) && (sMapY == BOBBYR_SHIPPING_DEST_SECTOR_Y)) {
      LaptopSaveInfo.fBobbyRSiteCanBeAccessed = TRUE;

      // If the player has been to Bobbyr when it was down, and we havent
      // already sent email, send him an email
      if (LaptopSaveInfo.ubHaveBeenToBobbyRaysAtLeastOnceWhileUnderConstruction ==
              BOBBYR_BEEN_TO_SITE_ONCE &&
          LaptopSaveInfo.ubHaveBeenToBobbyRaysAtLeastOnceWhileUnderConstruction !=
              BOBBYR_ALREADY_SENT_EMAIL) {
        AddEmail(BOBBYR_NOW_OPEN, BOBBYR_NOW_OPEN_LENGTH, BOBBY_R, GetWorldTotalMin());
        LaptopSaveInfo.ubHaveBeenToBobbyRaysAtLeastOnceWhileUnderConstruction =
            BOBBYR_ALREADY_SENT_EMAIL;
      }
    }

    fWasEnemyControlled = StrategicMap[usMapSector].fEnemyControlled;

    StrategicMap[usMapSector].fEnemyControlled = FALSE;

    bTownId = StrategicMap[usMapSector].bNameId;

    // check if there's a town in the sector
    if ((bTownId >= FIRST_TOWN) && (bTownId < NUM_TOWNS)) {
      // yes, start tracking (& displaying) this town's loyalty if not already
      // doing so
      StartTownLoyaltyIfFirstTime(bTownId);
    }

    // if player took control away from enemy
    if (fWasEnemyControlled && fContested) {
      // and it's a town
      if ((bTownId >= FIRST_TOWN) && (bTownId < NUM_TOWNS)) {
        // don't do these for takeovers of Omerta sectors at the beginning of
        // the game
        if ((bTownId != OMERTA) || (GetWorldDay() != 1)) {
          if (bMapZ == 0 && sector != SEC_J9 && sector != SEC_K4) {
            HandleMoraleEvent(NULL, MORALE_TOWN_LIBERATED, sMapX, sMapY, bMapZ);
            HandleGlobalLoyaltyEvent(GLOBAL_LOYALTY_GAIN_TOWN_SECTOR, sMapX, sMapY, bMapZ);

            // liberation by definition requires that the place was enemy
            // controlled in the first place
            CheckIfEntireTownHasBeenLiberated(bTownId, sMapX, sMapY);
          }
        }
      }

      // if it's a mine that's still worth something
      int8_t const mine_id = GetMineIndexForSector(sector);
      if (mine_id != -1 && GetTotalLeftInMine(mine_id) > 0) {
        HandleMoraleEvent(NULL, MORALE_MINE_LIBERATED, sMapX, sMapY, bMapZ);
        HandleGlobalLoyaltyEvent(GLOBAL_LOYALTY_GAIN_MINE, sMapX, sMapY, bMapZ);
      }

      // if it's a SAM site sector
      int8_t const sam_id = GetSAMIdFromSector(sMapX, sMapY, bMapZ);
      if (sam_id != -1) {
        if (1 /*!GetSectorFlagStatus( sMapX, sMapY, bMapZ, SF_SECTOR_HAS_BEEN_LIBERATED_ONCE ) */) {
          // SAM site liberated for first time, schedule meanwhile
          HandleMeanWhileEventPostingForSAMLiberation(sam_id);
        }

        HandleMoraleEvent(NULL, MORALE_SAM_SITE_LIBERATED, sMapX, sMapY, bMapZ);
        HandleGlobalLoyaltyEvent(GLOBAL_LOYALTY_GAIN_SAM, sMapX, sMapY, bMapZ);

        // if Skyrider has been delivered to chopper, and already mentioned
        // Drassen SAM site, but not used this quote yet
        if (IsHelicopterPilotAvailable() && (guiHelicopterSkyriderTalkState >= 1) &&
            (!gfSkyriderSaidCongratsOnTakingSAM)) {
          SkyRiderTalk(SAM_SITE_TAKEN);
          gfSkyriderSaidCongratsOnTakingSAM = TRUE;
        }

        if (!SectorInfo[sector].fSurfaceWasEverPlayerControlled) {
          // grant grace period
          if (gGameOptions.ubDifficultyLevel >= DIF_LEVEL_HARD) {
            UpdateLastDayOfPlayerActivity((uint16_t)(GetWorldDay() + 2));
          } else {
            UpdateLastDayOfPlayerActivity((uint16_t)(GetWorldDay() + 1));
          }
        }
      }

      // if it's a helicopter refueling site sector
      if (IsRefuelSiteInSector(usMapSector)) {
        UpdateRefuelSiteAvailability();
      }

      //			SetSectorFlag( sMapX, sMapY, bMapZ,
      // SF_SECTOR_HAS_BEEN_LIBERATED_ONCE );
      if (bMapZ == 0 && ((sMapY == MAP_ROW_M && (sMapX >= 2 && sMapX <= 6)) ||
                         (sMapY == MAP_ROW_N && sMapX == 6))) {
        HandleOutskirtsOfMedunaMeanwhileScene();
      }
    }

    if (fContested) {
      StrategicHandleQueenLosingControlOfSector((uint8_t)sMapX, (uint8_t)sMapY, (uint8_t)bMapZ);
    }
  } else {
    if (sector == SEC_P3 && bMapZ == 1) {  // Basement sector (P3_b1)
      gfUseAlternateQueenPosition = TRUE;
    }
  }

  if (bMapZ == 0) {
    SectorInfo[sector].fSurfaceWasEverPlayerControlled = TRUE;
  }

  // KM : Aug 11, 1999 -- Patch fix:  Relocated this check so it gets called
  // everytime a sector changes hands,
  //     even if the sector isn't a SAM site.  There is a bug _somewhere_ that
  //     fails to update the airspace, even though the player controls it.
  UpdateAirspaceControl();

  // redraw map/income if in mapscreen
  fMapPanelDirty = TRUE;
  fMapScreenBottomDirty = TRUE;

  return fWasEnemyControlled;
}

// ALL changes of control to enemy must be funneled through here!
BOOLEAN SetThisSectorAsEnemyControlled(int16_t const sMapX, int16_t const sMapY,
                                       int8_t const bMapZ) {
  uint16_t usMapSector = 0;
  BOOLEAN fWasPlayerControlled = FALSE;
  int8_t bTownId = 0;
  uint8_t ubTheftChance;

  // KM : August 6, 1999 Patch fix
  //     This check was added because this function gets called when player
  //     mercs retreat from an unresolved battle between militia and enemies. It
  //     will get called again AFTER autoresolve is finished.
  if (gfAutomaticallyStartAutoResolve) {
    return (FALSE);
  }

  if (bMapZ == 0) {
    usMapSector = sMapX + (sMapY * MAP_WORLD_X);

    fWasPlayerControlled = !StrategicMap[usMapSector].fEnemyControlled;

    StrategicMap[usMapSector].fEnemyControlled = TRUE;

    // if player lost control to the enemy
    if (fWasPlayerControlled) {
      if (PlayerMercsInSector((uint8_t)sMapX, (uint8_t)sMapY,
                              (uint8_t)bMapZ)) {  // too premature:  Player mercs still in sector.
        return FALSE;
      }

      uint8_t const sector = SECTOR(sMapX, sMapY);
      // check if there's a town in the sector
      bTownId = StrategicMap[usMapSector].bNameId;

      // and it's a town
      if ((bTownId >= FIRST_TOWN) && (bTownId < NUM_TOWNS)) {
        if (bMapZ == 0 && sector != SEC_J9 && sector != SEC_K4) {
          HandleMoraleEvent(NULL, MORALE_TOWN_LOST, sMapX, sMapY, bMapZ);
          HandleGlobalLoyaltyEvent(GLOBAL_LOYALTY_LOSE_TOWN_SECTOR, sMapX, sMapY, bMapZ);

          CheckIfEntireTownHasBeenLost(bTownId, sMapX, sMapY);
        }
      }

      // if the sector has a mine which is still worth something
      int8_t const mine_id = GetMineIndexForSector(sector);
      if (mine_id != -1 && GetTotalLeftInMine(mine_id) > 0) {
        QueenHasRegainedMineSector(mine_id);
        HandleMoraleEvent(NULL, MORALE_MINE_LOST, sMapX, sMapY, bMapZ);
        HandleGlobalLoyaltyEvent(GLOBAL_LOYALTY_LOSE_MINE, sMapX, sMapY, bMapZ);
      }

      // if it's a SAM site sector
      if (IsThisSectorASAMSector(sMapX, sMapY, bMapZ)) {
        HandleMoraleEvent(NULL, MORALE_SAM_SITE_LOST, sMapX, sMapY, bMapZ);
        HandleGlobalLoyaltyEvent(GLOBAL_LOYALTY_LOSE_SAM, sMapX, sMapY, bMapZ);
      }

      // if it's a helicopter refueling site sector
      if (IsRefuelSiteInSector(usMapSector)) {
        UpdateRefuelSiteAvailability();
      }

      // ARM: this must be AFTER all resulting loyalty effects are resolved, or
      // reduced mine income shown won't be accurate
      NotifyPlayerWhenEnemyTakesControlOfImportantSector(sMapX, sMapY, 0);
    }

    // NOTE: Stealing is intentionally OUTSIDE the fWasPlayerControlled branch.
    // This function gets called if new enemy reinforcements arrive, and they
    // deserve another crack at stealing what the first group missed! :-)

    // stealing should fail anyway 'cause there shouldn't be a temp file for
    // unvisited sectors, but let's check anyway
    if (GetSectorFlagStatus(sMapX, sMapY, bMapZ, SF_ALREADY_VISITED)) {
      // enemies can steal items left lying about (random chance).  The more
      // there are, the more they take!
      ubTheftChance = 5 * NumEnemiesInAnySector(sMapX, sMapY, bMapZ);
      // max 90%, some stuff may just simply not get found
      if (ubTheftChance > 90) {
        ubTheftChance = 90;
      }
      RemoveRandomItemsInSector(sMapX, sMapY, bMapZ, ubTheftChance);
    }

    // don't touch fPlayer flag for a surface sector lost to the enemies!
    // just because player has lost the sector doesn't mean he realizes it -
    // that's up to our caller to decide!
  }

  // KM : Aug 11, 1999 -- Patch fix:  Relocated this check so it gets called
  // everytime a sector changes hands,
  //     even if the sector isn't a SAM site.  There is a bug _somewhere_ that
  //     fails to update the airspace, even though the player controls it.
  UpdateAirspaceControl();

  // redraw map/income if in mapscreen
  fMapPanelDirty = TRUE;
  fMapScreenBottomDirty = TRUE;

  return fWasPlayerControlled;
}
