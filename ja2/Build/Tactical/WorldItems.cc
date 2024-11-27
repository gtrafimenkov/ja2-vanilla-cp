#include "Tactical/WorldItems.h"

#include <stdexcept>
#include <string.h>

#include "GameSettings.h"
#include "Macro.h"
#include "SGP/FileMan.h"
#include "SGP/MemMan.h"
#include "SGP/Random.h"
#include "Strategic/CampaignTypes.h"
#include "Strategic/Quests.h"
#include "Strategic/StrategicMap.h"
#include "SysGlobals.h"
#include "Tactical/ActionItems.h"
#include "Tactical/HandleItems.h"
#include "Tactical/Items.h"
#include "Tactical/Overhead.h"
#include "Tactical/Points.h"
#include "Tactical/SoldierProfile.h"
#include "Tactical/Weapons.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/RenderDirty.h"
#include "TileEngine/Structure.h"
#include "TileEngine/TileDef.h"
#include "TileEngine/WorldDef.h"
#include "Utils/FontControl.h"

#ifdef JA2BETAVERSION
#include "Message.h"
#endif

// Global dynamic array of all of the items in a loaded map.
WORLDITEM *gWorldItems = NULL;
uint32_t guiNumWorldItems = 0;

WORLDBOMB *gWorldBombs = NULL;
uint32_t guiNumWorldBombs = 0;

static int32_t GetFreeWorldBombIndex() {
  uint32_t uiCount;
  WORLDBOMB *newWorldBombs;
  uint32_t uiOldNumWorldBombs;

  for (uiCount = 0; uiCount < guiNumWorldBombs; uiCount++) {
    if (!gWorldBombs[uiCount].fExists) return (int32_t)uiCount;
  }

  uiOldNumWorldBombs = guiNumWorldBombs;
  guiNumWorldBombs += 10;
  // Allocate new table with max+10 items.
  newWorldBombs = REALLOC(gWorldBombs, WORLDBOMB, guiNumWorldBombs);

  // Clear the rest of the new array
  memset(&newWorldBombs[uiOldNumWorldBombs], 0,
         sizeof(WORLDBOMB) * (guiNumWorldBombs - uiOldNumWorldBombs));
  gWorldBombs = newWorldBombs;

  // Return uiCount.....
  return (uiCount);
}

static int32_t AddBombToWorld(int32_t iItemIndex) {
  uint32_t iBombIndex;

  iBombIndex = GetFreeWorldBombIndex();

  // Add the new world item to the table.
  gWorldBombs[iBombIndex].fExists = TRUE;
  gWorldBombs[iBombIndex].iItemIndex = iItemIndex;

  return (iBombIndex);
}

static void RemoveBombFromWorldByItemIndex(int32_t iItemIndex) {
  // Find the world bomb which corresponds with a particular world item, then
  // remove the world bomb from the table.
  FOR_EACH_WORLD_BOMB(wb) {
    if (wb->iItemIndex != iItemIndex) continue;

    wb->fExists = FALSE;
    return;
  }
}

int32_t FindWorldItemForBombInGridNo(const int16_t sGridNo, const int8_t bLevel) {
  CFOR_EACH_WORLD_BOMB(wb) {
    WORLDITEM const &wi = GetWorldItem(wb->iItemIndex);
    if (wi.sGridNo != sGridNo || wi.ubLevel != bLevel) continue;

    return wb->iItemIndex;
  }
  throw std::logic_error("Cannot find bomb item");
}

void FindPanicBombsAndTriggers() {
  // This function searches the bomb table to find panic-trigger-tuned bombs and
  // triggers
  CFOR_EACH_WORLD_BOMB(wb) {
    WORLDITEM const &wi = GetWorldItem(wb->iItemIndex);
    OBJECTTYPE const &o = wi.o;

    int8_t bPanicIndex;
    switch (o.bFrequency) {
      case PANIC_FREQUENCY:
        bPanicIndex = 0;
        break;
      case PANIC_FREQUENCY_2:
        bPanicIndex = 1;
        break;
      case PANIC_FREQUENCY_3:
        bPanicIndex = 2;
        break;
      default:
        continue;
    }

    if (o.usItem == SWITCH) {
      int16_t sGridNo = wi.sGridNo;
      const STRUCTURE *const switch_ = FindStructure(sGridNo, STRUCTURE_SWITCH);
      if (switch_) {
        switch (switch_->ubWallOrientation) {
          case INSIDE_TOP_LEFT:
          case OUTSIDE_TOP_LEFT:
            sGridNo += DirectionInc(SOUTH);
            break;
          case INSIDE_TOP_RIGHT:
          case OUTSIDE_TOP_RIGHT:
            sGridNo += DirectionInc(EAST);
            break;

          default:
            break;
        }
      }

      gTacticalStatus.fPanicFlags |= PANIC_TRIGGERS_HERE;
      gTacticalStatus.sPanicTriggerGridNo[bPanicIndex] = sGridNo;
      gTacticalStatus.ubPanicTolerance[bPanicIndex] = o.ubTolerance;
      if (o.fFlags & OBJECT_ALARM_TRIGGER) {
        gTacticalStatus.bPanicTriggerIsAlarm[bPanicIndex] = TRUE;
      }
      if (bPanicIndex + 1 == NUM_PANIC_TRIGGERS) return;
    } else {
      gTacticalStatus.fPanicFlags |= PANIC_BOMBS_HERE;
    }
  }
}

static int32_t GetFreeWorldItemIndex() {
  uint32_t uiCount;
  WORLDITEM *newWorldItems;
  uint32_t uiOldNumWorldItems;

  for (uiCount = 0; uiCount < guiNumWorldItems; uiCount++) {
    if (!gWorldItems[uiCount].fExists) return (int32_t)uiCount;
  }

  uiOldNumWorldItems = guiNumWorldItems;
  guiNumWorldItems += 10;
  // Allocate new table with max+10 items.
  newWorldItems = REALLOC(gWorldItems, WORLDITEM, guiNumWorldItems);

  // Clear the rest of the new array
  memset(&newWorldItems[uiOldNumWorldItems], 0,
         sizeof(WORLDITEM) * (guiNumWorldItems - uiOldNumWorldItems));
  gWorldItems = newWorldItems;

  // Return uiCount.....
  return (uiCount);
}

static uint32_t GetNumUsedWorldItems() {
  uint32_t count = 0;
  CFOR_EACH_WORLD_ITEM(wi)++ count;
  return count;
}

int32_t AddItemToWorld(int16_t sGridNo, const OBJECTTYPE *const pObject, const uint8_t ubLevel,
                       const uint16_t usFlags, const int8_t bRenderZHeightAboveLevel,
                       const int8_t bVisible) {
  // ATE: Check if the gridno is OK
  if (sGridNo == NOWHERE) {
    return -1;
  }

  const uint32_t iItemIndex = GetFreeWorldItemIndex();
  WORLDITEM &wi = GetWorldItem(iItemIndex);

  // Add the new world item to the table.
  wi.fExists = TRUE;
  wi.sGridNo = sGridNo;
  wi.ubLevel = ubLevel;
  wi.usFlags = usFlags;
  wi.bVisible = bVisible;
  wi.bRenderZHeightAboveLevel = bRenderZHeightAboveLevel;
  wi.o = *pObject;

  // Add a bomb reference if needed
  if (usFlags & WORLD_ITEM_ARMED_BOMB) {
    if (AddBombToWorld(iItemIndex) == -1) return -1;
  }

  return iItemIndex;
}

void RemoveItemFromWorld(const int32_t iItemIndex) {
  WORLDITEM &wi = GetWorldItem(iItemIndex);
  if (!wi.fExists) return;

  // If it's a bomb, remove the appropriate entry from the bomb table
  if (wi.usFlags & WORLD_ITEM_ARMED_BOMB) {
    RemoveBombFromWorldByItemIndex(iItemIndex);
  }
  wi.fExists = FALSE;
}

void TrashWorldItems() {
  if (gWorldItems) {
    FOR_EACH_WORLD_ITEM(wi) { RemoveItemFromPool(wi); }
    MemFree(gWorldItems);
    gWorldItems = NULL;
    guiNumWorldItems = 0;
  }
  if (gWorldBombs) {
    MemFree(gWorldBombs);
    gWorldBombs = NULL;
    guiNumWorldBombs = 0;
  }
}

void SaveWorldItemsToMap(HWFILE const f) {
  uint32_t const n_actual_world_items = GetNumUsedWorldItems();
  FileWrite(f, &n_actual_world_items, sizeof(n_actual_world_items));

  CFOR_EACH_WORLD_ITEM(wi) FileWrite(f, wi, sizeof(WORLDITEM));
}

static void DeleteWorldItemsBelongingToQueenIfThere();
static void DeleteWorldItemsBelongingToTerroristsWhoAreNotThere();

void LoadWorldItemsFromMap(HWFILE const f) {
  // If any world items exist, we must delete them now
  TrashWorldItems();

  // Read the number of items that were saved in the map
  uint32_t n_world_items;
  FileRead(f, &n_world_items, sizeof(n_world_items));

  if (gTacticalStatus.uiFlags & LOADING_SAVED_GAME &&
      !gfEditMode) { /* The sector has already been visited. The items are saved
                      * in a different format that will be loaded later on. So,
                      * all we need to do is skip the data entirely. */
    FileSeek(f, sizeof(WORLDITEM) * n_world_items, FILE_SEEK_FROM_CURRENT);
    return;
  }

  for (uint32_t n = n_world_items; n != 0; --n) { /* Add all of the items to the world indirectly
                                                   * through AddItemToPool, but only if the chance
                                                   * associated with them succeed. */
    WORLDITEM wi;
    FileRead(f, &wi, sizeof(wi));
    OBJECTTYPE &o = wi.o;

    if (o.usItem == OWNERSHIP) wi.ubNonExistChance = 0;

    if (!gfEditMode && PreRandom(100) < wi.ubNonExistChance) continue;

    if (!gfEditMode) {
      // Check for matching item existance modes and only add if there is a
      // match
      if (wi.usFlags & (gGameOptions.fSciFi ? WORLD_ITEM_SCIFI_ONLY : WORLD_ITEM_REALISTIC_ONLY))
        continue;

      if (!gGameOptions.fGunNut) {
        // do replacements?
        INVTYPE const &item = Item[o.usItem];
        if (item.usItemClass == IC_GUN) {
          uint16_t const replacement = StandardGunListReplacement(o.usItem);
          if (replacement != NOTHING) {
            // everything else can be the same? no.
            int8_t const ammo = o.ubGunShotsLeft;
            int8_t new_ammo = Weapon[replacement].ubMagSize * ammo / Weapon[o.usItem].ubMagSize;
            if (new_ammo == 0 && ammo > 0) new_ammo = 1;
            o.usItem = replacement;
            o.ubGunShotsLeft = new_ammo;
          }
        } else if (item.usItemClass == IC_AMMO) {
          uint16_t const replacement = StandardGunListAmmoReplacement(o.usItem);
          if (replacement != NOTHING) {
            // Go through status values and scale up/down
            uint8_t const mag_size = Magazine[item.ubClassIndex].ubMagSize;
            uint8_t const new_mag_size = Magazine[Item[replacement].ubClassIndex].ubMagSize;
            for (uint8_t i = 0; i != o.ubNumberOfObjects; ++i) {
              o.bStatus[i] = o.bStatus[i] * new_mag_size / mag_size;
            }

            // then replace item #
            o.usItem = replacement;
          }
        }
      }
    }

    switch (o.usItem) {
      case ACTION_ITEM:
        // If we are loading a pit, they are typically loaded without being armed.
        if (o.bActionValue == ACTION_ITEM_SMALL_PIT || o.bActionValue == ACTION_ITEM_LARGE_PIT) {
          wi.usFlags &= ~WORLD_ITEM_ARMED_BOMB;
          wi.bVisible = BURIED;
          o.bDetonatorType = 0;
        }
        break;

      case MINE:
      case TRIP_FLARE:
      case TRIP_KLAXON:
        if (wi.bVisible == HIDDEN_ITEM && o.bTrap > 0) {
          ArmBomb(&o, BOMB_PRESSURE);
          wi.usFlags |= WORLD_ITEM_ARMED_BOMB;
          // this is coming from the map so the enemy must know about it.
          gpWorldLevelData[wi.sGridNo].uiFlags |= MAPELEMENT_ENEMY_MINE_PRESENT;
        }
        break;
    }

    // All armed bombs are buried
    if (wi.usFlags & WORLD_ITEM_ARMED_BOMB) wi.bVisible = BURIED;

    int32_t const item_idx = AddItemToPool(wi.sGridNo, &o, static_cast<Visibility>(wi.bVisible),
                                           wi.ubLevel, wi.usFlags, wi.bRenderZHeightAboveLevel);
    GetWorldItem(item_idx).ubNonExistChance = wi.ubNonExistChance;
  }

  if (!gfEditMode) {
    DeleteWorldItemsBelongingToTerroristsWhoAreNotThere();
    if (gWorldSectorX == 3 && gWorldSectorY == MAP_ROW_P && gbWorldSectorZ == 1) {
      DeleteWorldItemsBelongingToQueenIfThere();
    }
  }
}

static void DeleteWorldItemsBelongingToTerroristsWhoAreNotThere() {
  // only do this after Carmen has talked to player and terrorists have been
  // placed
  // if ( CheckFact( FACT_CARMEN_EXPLAINED_DEAL, 0 ) == TRUE )
  {
    CFOR_EACH_WORLD_ITEM(wi) {
      // loop through all items, look for ownership
      if (wi->o.usItem != OWNERSHIP) continue;

      const ProfileID pid = wi->o.ubOwnerProfile;
      // if owner is a terrorist
      if (!IsProfileATerrorist(pid)) continue;

      MERCPROFILESTRUCT const &p = GetProfile(pid);
      // and they were not set in the current sector
      if (p.sSectorX == gWorldSectorX && p.sSectorY == gWorldSectorY) continue;

      // then all items in this location should be deleted
      const int16_t sGridNo = wi->sGridNo;
      const uint8_t ubLevel = wi->ubLevel;
      FOR_EACH_WORLD_ITEM(owned_item) {
        if (owned_item->sGridNo == sGridNo && owned_item->ubLevel == ubLevel) {
          RemoveItemFromPool(owned_item);
        }
      }
    }
  }
  // else the terrorists haven't been placed yet!
}

static void DeleteWorldItemsBelongingToQueenIfThere() {
  MERCPROFILESTRUCT &q = GetProfile(QUEEN);

  if (q.sSectorX != gWorldSectorX || q.sSectorY != gWorldSectorY || q.bSectorZ != gbWorldSectorZ) {
    return;
  }

  CFOR_EACH_WORLD_ITEM(wi) {
    // Look for items belonging to the queen
    if (wi->o.usItem != OWNERSHIP) continue;
    if (wi->o.ubOwnerProfile != QUEEN) continue;

    // Delete all items on this tile
    const int16_t sGridNo = wi->sGridNo;
    const uint8_t ubLevel = wi->ubLevel;
    FOR_EACH_WORLD_ITEM(item) {
      if (item->sGridNo != sGridNo) continue;
      if (item->ubLevel != ubLevel) continue;

      // Upgrade equipment
      switch (item->o.usItem) {
        case AUTO_ROCKET_RIFLE: {
          // Give her auto rifle
          int8_t const bSlot = FindObjectInSoldierProfile(q, ROCKET_RIFLE);
          if (bSlot != NO_SLOT) q.inv[bSlot] = AUTO_ROCKET_RIFLE;
          break;
        }

        case SPECTRA_HELMET_18:
          q.inv[HELMETPOS] = SPECTRA_HELMET_18;
          break;
        case SPECTRA_VEST_18:
          q.inv[VESTPOS] = SPECTRA_VEST_18;
          break;
        case SPECTRA_LEGGINGS_18:
          q.inv[LEGPOS] = SPECTRA_LEGGINGS_18;
          break;

        default:
          break;
      }
      RemoveItemFromPool(item);
    }
  }
}

// Refresh item pools
void RefreshWorldItemsIntoItemPools(const WORLDITEM *const items, const int32_t item_count) {
  for (const WORLDITEM *i = items; i != items + item_count; ++i) {
    if (!i->fExists) continue;
    OBJECTTYPE o = i->o;  // XXX AddItemToPool() may alter the object
    AddItemToPool(i->sGridNo, &o, static_cast<Visibility>(i->bVisible), i->ubLevel, i->usFlags,
                  i->bRenderZHeightAboveLevel);
  }
}

#undef FAIL
#include "gtest/gtest.h"

TEST(WorldItems, asserts) { EXPECT_EQ(sizeof(WORLDITEM), 52); }
