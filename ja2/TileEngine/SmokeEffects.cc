// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "TileEngine/SmokeEffects.h"

#include <stdexcept>
#include <string.h>

#include "Directories.h"
#include "Macro.h"
#include "SGP/Debug.h"
#include "SGP/FileMan.h"
#include "SGP/Random.h"
#include "SaveLoadGame.h"
#include "Strategic/CampaignTypes.h"
#include "Strategic/GameClock.h"
#include "Tactical/HandleItems.h"
#include "Tactical/OppList.h"
#include "Tactical/Overhead.h"
#include "Tactical/SoldierControl.h"
#include "Tactical/TacticalSave.h"
#include "Tactical/Weapons.h"
#include "TileEngine/ExplosionControl.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/LoadSaveSmokeEffect.h"
#include "TileEngine/RenderWorld.h"
#include "TileEngine/TileAnimation.h"
#include "TileEngine/TileDef.h"
#include "TileEngine/WorldDef.h"
#include "TileEngine/WorldMan.h"

#define NUM_SMOKE_EFFECT_SLOTS 25

// GLOBAL FOR SMOKE LISTING
static SMOKEEFFECT gSmokeEffectData[NUM_SMOKE_EFFECT_SLOTS];
static uint32_t guiNumSmokeEffects = 0;

#define BASE_FOR_EACH_SMOKE_EFFECT(type, iter)                                              \
  for (type *iter = gSmokeEffectData, *iter##__end = gSmokeEffectData + guiNumSmokeEffects; \
       iter != iter##__end; ++iter)                                                         \
    if (!iter->fAllocated)                                                                  \
      continue;                                                                             \
    else
#define FOR_EACH_SMOKE_EFFECT(iter) BASE_FOR_EACH_SMOKE_EFFECT(SMOKEEFFECT, iter)
#define CFOR_EACH_SMOKE_EFFECT(iter) BASE_FOR_EACH_SMOKE_EFFECT(const SMOKEEFFECT, iter)

static SMOKEEFFECT *GetFreeSmokeEffect() {
  for (SMOKEEFFECT *s = gSmokeEffectData; s != gSmokeEffectData + guiNumSmokeEffects; ++s) {
    if (!s->fAllocated) return s;
  }
  if (guiNumSmokeEffects < NUM_SMOKE_EFFECT_SLOTS) {
    return &gSmokeEffectData[guiNumSmokeEffects++];
  }
  return NULL;
}

static SmokeEffectKind FromWorldFlagsToSmokeType(uint8_t ubWorldFlags);

// Returns NO_SMOKE_EFFECT if none there...
SmokeEffectKind GetSmokeEffectOnTile(int16_t const sGridNo, int8_t const bLevel) {
  uint8_t ubExtFlags;

  ubExtFlags = gpWorldLevelData[sGridNo].ubExtFlags[bLevel];

  // Look at worldleveldata to find flags..
  if (ubExtFlags & ANY_SMOKE_EFFECT) {
    // Which smoke am i?
    return (FromWorldFlagsToSmokeType(ubExtFlags));
  }

  return (NO_SMOKE_EFFECT);
}

static SmokeEffectKind FromWorldFlagsToSmokeType(uint8_t ubWorldFlags) {
  if (ubWorldFlags & MAPELEMENT_EXT_SMOKE) {
    return (NORMAL_SMOKE_EFFECT);
  } else if (ubWorldFlags & MAPELEMENT_EXT_TEARGAS) {
    return (TEARGAS_SMOKE_EFFECT);
  } else if (ubWorldFlags & MAPELEMENT_EXT_MUSTARDGAS) {
    return (MUSTARDGAS_SMOKE_EFFECT);
  } else if (ubWorldFlags & MAPELEMENT_EXT_CREATUREGAS) {
    return (CREATURE_SMOKE_EFFECT);
  } else {
    return (NO_SMOKE_EFFECT);
  }
}

static uint8_t FromSmokeTypeToWorldFlags(SmokeEffectKind const bType) {
  switch (bType) {
    case NORMAL_SMOKE_EFFECT:
      return MAPELEMENT_EXT_SMOKE;
    case TEARGAS_SMOKE_EFFECT:
      return MAPELEMENT_EXT_TEARGAS;
    case MUSTARDGAS_SMOKE_EFFECT:
      return MAPELEMENT_EXT_MUSTARDGAS;
    case CREATURE_SMOKE_EFFECT:
      return MAPELEMENT_EXT_CREATUREGAS;
    default:
      return 0;
  }
}

void NewSmokeEffect(const int16_t sGridNo, const uint16_t usItem, const int8_t bLevel,
                    SOLDIERTYPE *const owner) {
  int8_t bSmokeEffectType = 0;
  uint8_t ubDuration = 0;
  uint8_t ubStartRadius = 0;

  SMOKEEFFECT *const pSmoke = GetFreeSmokeEffect();
  if (pSmoke == NULL) return;

  memset(pSmoke, 0, sizeof(*pSmoke));

  // Set some values...
  pSmoke->sGridNo = sGridNo;
  pSmoke->usItem = usItem;
  pSmoke->uiTimeOfLastUpdate = GetWorldTotalSeconds();

  // Are we indoors?
  if (GetTerrainType(sGridNo) == FLAT_FLOOR) {
    pSmoke->bFlags |= SMOKE_EFFECT_INDOORS;
  }

  switch (usItem) {
    case MUSTARD_GRENADE:

      bSmokeEffectType = MUSTARDGAS_SMOKE_EFFECT;
      ubDuration = 5;
      ubStartRadius = 1;
      break;

    case TEARGAS_GRENADE:
    case GL_TEARGAS_GRENADE:
      bSmokeEffectType = TEARGAS_SMOKE_EFFECT;
      ubDuration = 5;
      ubStartRadius = 1;
      break;

    case BIG_TEAR_GAS:
      bSmokeEffectType = TEARGAS_SMOKE_EFFECT;
      ubDuration = 5;
      ubStartRadius = 1;
      break;

    case SMOKE_GRENADE:
    case GL_SMOKE_GRENADE:

      bSmokeEffectType = NORMAL_SMOKE_EFFECT;
      ubDuration = 5;
      ubStartRadius = 1;
      break;

    case SMALL_CREATURE_GAS:
      bSmokeEffectType = CREATURE_SMOKE_EFFECT;
      ubDuration = 3;
      ubStartRadius = 1;
      break;

    case LARGE_CREATURE_GAS:
      bSmokeEffectType = CREATURE_SMOKE_EFFECT;
      ubDuration = 3;
      ubStartRadius = Explosive[Item[LARGE_CREATURE_GAS].ubClassIndex].ubRadius;
      break;

    case VERY_SMALL_CREATURE_GAS:

      bSmokeEffectType = CREATURE_SMOKE_EFFECT;
      ubDuration = 2;
      ubStartRadius = 0;
      break;
  }

  pSmoke->ubDuration = ubDuration;
  pSmoke->ubRadius = ubStartRadius;
  pSmoke->bAge = 0;
  pSmoke->fAllocated = TRUE;
  pSmoke->bType = bSmokeEffectType;
  pSmoke->owner = owner;

  if (pSmoke->bFlags & SMOKE_EFFECT_INDOORS) {
    // Duration is increased by 2 turns...indoors
    pSmoke->ubDuration += 3;
  }

  if (bLevel) {
    pSmoke->bFlags |= SMOKE_EFFECT_ON_ROOF;
  }

  // ATE: FALSE into subsequent-- it's the first one!
  SpreadEffectSmoke(pSmoke, FALSE, bLevel);
}

// Add smoke to gridno
// ( Replacement algorithm uses distance away )
void AddSmokeEffectToTile(SMOKEEFFECT const *const smoke, SmokeEffectKind const bType,
                          int16_t const sGridNo, int8_t const bLevel) {
  BOOLEAN dissipating = FALSE;
  if (smoke->ubDuration - smoke->bAge < 2) {
    dissipating = TRUE;
    // Remove old one...
    RemoveSmokeEffectFromTile(sGridNo, bLevel);
  }

  // If smoke effect exists already.... stop
  if (gpWorldLevelData[sGridNo].ubExtFlags[bLevel] & ANY_SMOKE_EFFECT) return;

  // Use the right graphic based on type..
  AnimationFlags ani_flags = ANITILE_FORWARD | ANITILE_SMOKE_EFFECT | ANITILE_LOOPING;
  char const *cached_file;
  int16_t start_frame;
  if (gGameSettings.fOptions[TOPTION_ANIMATE_SMOKE]) {
    if (dissipating) {
      switch (bType) {
        case NORMAL_SMOKE_EFFECT:
          cached_file = TILECACHEDIR "/smalsmke.sti";
          break;
        case TEARGAS_SMOKE_EFFECT:
          cached_file = TILECACHEDIR "/smaltear.sti";
          break;
        case MUSTARDGAS_SMOKE_EFFECT:
          cached_file = TILECACHEDIR "/smalmust.sti";
          break;
        case CREATURE_SMOKE_EFFECT:
          cached_file = TILECACHEDIR "/spit_gas.sti";
          break;
        default:
          throw std::logic_error("Invalid smoke effect type");
      }
    } else {
      switch (bType) {
        case NORMAL_SMOKE_EFFECT:
          cached_file = TILECACHEDIR "/smoke.sti";
          break;
        case TEARGAS_SMOKE_EFFECT:
          cached_file = TILECACHEDIR "/teargas.sti";
          break;
        case MUSTARDGAS_SMOKE_EFFECT:
          cached_file = TILECACHEDIR "/mustard2.sti";
          break;
        case CREATURE_SMOKE_EFFECT:
          cached_file = TILECACHEDIR "/spit_gas.sti";
          break;
        default:
          throw std::logic_error("Invalid smoke effect type");
      }
    }
    start_frame = Random(5);
    ani_flags |= ANITILE_ALWAYS_TRANSLUCENT;
  } else {
    switch (bType) {
      case NORMAL_SMOKE_EFFECT:
        cached_file = TILECACHEDIR "/smkechze.sti";
        break;
      case TEARGAS_SMOKE_EFFECT:
        cached_file = TILECACHEDIR "/tearchze.sti";
        break;
      case MUSTARDGAS_SMOKE_EFFECT:
        cached_file = TILECACHEDIR "/mustchze.sti";
        break;
      case CREATURE_SMOKE_EFFECT:
        cached_file = TILECACHEDIR "/spit_gas.sti";
        break;
      default:
        throw std::logic_error("Invalid smoke effect type");
    }
    start_frame = 0;
    ani_flags |= ANITILE_PAUSED;
  }

  ANITILE_PARAMS ani_params;
  memset(&ani_params, 0, sizeof(ani_params));
  ani_params.uiFlags = ani_flags;
  ani_params.zCachedFile = cached_file;
  ani_params.sStartFrame = start_frame;
  ani_params.sGridNo = sGridNo;
  ani_params.ubLevelID = (bLevel == 0 ? ANI_STRUCT_LEVEL : ANI_ONROOF_LEVEL);
  ani_params.sDelay = 300 + Random(300);
  ani_params.sX = CenterX(sGridNo);
  ani_params.sY = CenterY(sGridNo);
  ani_params.sZ = 0;
  CreateAnimationTile(&ani_params);

  gpWorldLevelData[sGridNo].ubExtFlags[bLevel] |= FromSmokeTypeToWorldFlags(bType);
  SetRenderFlags(RENDER_FLAG_FULL);
}

void RemoveSmokeEffectFromTile(int16_t sGridNo, int8_t bLevel) {
  ANITILE *pAniTile;
  uint8_t ubLevelID;

  // Get ANI tile...
  if (bLevel == 0) {
    ubLevelID = ANI_STRUCT_LEVEL;
  } else {
    ubLevelID = ANI_ONROOF_LEVEL;
  }

  pAniTile = GetCachedAniTileOfType(sGridNo, ubLevelID, ANITILE_SMOKE_EFFECT);

  if (pAniTile != NULL) {
    DeleteAniTile(pAniTile);

    SetRenderFlags(RENDER_FLAG_FULL);
  }

  // Unset flags in world....
  // ( // check to see if we are the last one....
  if (GetCachedAniTileOfType(sGridNo, ubLevelID, ANITILE_SMOKE_EFFECT) == NULL) {
    gpWorldLevelData[sGridNo].ubExtFlags[bLevel] &= (~ANY_SMOKE_EFFECT);
  }
}

void DecaySmokeEffects(uint32_t uiTime) {
  BOOLEAN fUpdate = FALSE;
  BOOLEAN fSpreadEffect;
  int8_t bLevel;
  uint16_t usNumUpdates = 1;

  // reset 'hit by gas' flags
  FOR_EACH_MERC(i)(*i)->fHitByGasFlags = 0;

  // ATE: 1 ) make first pass and delete/mark any smoke effect for update
  // all the deleting has to be done first///

  // age all active tear gas clouds, deactivate those that are just dispersing
  FOR_EACH_SMOKE_EFFECT(pSmoke) {
    fSpreadEffect = TRUE;

    if (pSmoke->bFlags & SMOKE_EFFECT_ON_ROOF) {
      bLevel = 1;
    } else {
      bLevel = 0;
    }

    // Do things differently for combat /vs realtime
    // always try to update during combat
    if (gTacticalStatus.uiFlags & INCOMBAT) {
      fUpdate = TRUE;
    } else {
      // ATE: Do this every so ofte, to acheive the effect we want...
      if ((uiTime - pSmoke->uiTimeOfLastUpdate) > 10) {
        fUpdate = TRUE;

        usNumUpdates = (uint16_t)((uiTime - pSmoke->uiTimeOfLastUpdate) / 10);
      }
    }

    if (fUpdate) {
      pSmoke->uiTimeOfLastUpdate = uiTime;

      for (uint32_t cnt2 = 0; cnt2 < usNumUpdates; ++cnt2) {
        pSmoke->bAge++;

        if (pSmoke->bAge == 1) {
          // ATE: At least mark for update!
          pSmoke->bFlags |= SMOKE_EFFECT_MARK_FOR_UPDATE;
          fSpreadEffect = FALSE;
        } else {
          fSpreadEffect = TRUE;
        }

        if (fSpreadEffect) {
          // if this cloud remains effective (duration not reached)
          if (pSmoke->bAge <= pSmoke->ubDuration) {
            // ATE: Only mark now and increse radius - actual drawing is done
            // in another pass cause it could
            // just get erased...
            pSmoke->bFlags |= SMOKE_EFFECT_MARK_FOR_UPDATE;

            // calculate the new cloud radius
            // cloud expands by 1 every turn outdoors, and every other turn
            // indoors

            // ATE: If radius is < maximun, increase radius, otherwise keep at
            // max
            if (pSmoke->ubRadius < Explosive[Item[pSmoke->usItem].ubClassIndex].ubRadius) {
              pSmoke->ubRadius++;
            }
          } else {
            // deactivate tear gas cloud (use last known radius)
            SpreadEffectSmoke(pSmoke, ERASE_SPREAD_EFFECT, bLevel);
            pSmoke->fAllocated = FALSE;
            break;
          }
        }
      }
    } else {
      // damage anyone standing in cloud
      SpreadEffectSmoke(pSmoke, REDO_SPREAD_EFFECT, 0);
    }
  }

  FOR_EACH_SMOKE_EFFECT(pSmoke) {
    if (pSmoke->bFlags & SMOKE_EFFECT_ON_ROOF) {
      bLevel = 1;
    } else {
      bLevel = 0;
    }

    // if this cloud remains effective (duration not reached)
    if (pSmoke->bFlags & SMOKE_EFFECT_MARK_FOR_UPDATE) {
      SpreadEffectSmoke(pSmoke, TRUE, bLevel);
      pSmoke->bFlags &= (~SMOKE_EFFECT_MARK_FOR_UPDATE);
    }
  }

  AllTeamsLookForAll(TRUE);
}

void LoadSmokeEffectsFromLoadGameFile(HWFILE const hFile, uint32_t const savegame_version) {
  uint32_t uiCnt = 0;

  // Clear out the old list
  memset(gSmokeEffectData, 0, sizeof(SMOKEEFFECT) * NUM_SMOKE_EFFECT_SLOTS);

  // Load the Number of Smoke Effects
  FileRead(hFile, &guiNumSmokeEffects, sizeof(uint32_t));

  // This is a TEMP hack to allow us to use the saves
  if (savegame_version < 37 && guiNumSmokeEffects == 0) {
    ExtractSmokeEffectFromFile(hFile, &gSmokeEffectData[0]);
  }

  // loop through and load the list
  for (uiCnt = 0; uiCnt < guiNumSmokeEffects; uiCnt++) {
    ExtractSmokeEffectFromFile(hFile, &gSmokeEffectData[uiCnt]);
    // This is a TEMP hack to allow us to use the saves
    if (savegame_version < 37) break;
  }

  // loop through and apply the smoke effects to the map
  FOR_EACH_SMOKE_EFFECT(s) {
    const int8_t bLevel = (s->bFlags & SMOKE_EFFECT_ON_ROOF ? 1 : 0);
    SpreadEffectSmoke(s, TRUE, bLevel);
  }
}

void SaveSmokeEffectsToMapTempFile(int16_t const sMapX, int16_t const sMapY, int8_t const bMapZ) {
  uint32_t uiNumSmokeEffects = 0;
  char zMapName[128];

  // get the name of the map
  GetMapTempFileName(SF_SMOKE_EFFECTS_TEMP_FILE_EXISTS, zMapName, sMapX, sMapY, bMapZ);

  // delete file the file.
  FileDelete(zMapName);

  // loop through and count the number of smoke effects
  CFOR_EACH_SMOKE_EFFECT(s)++ uiNumSmokeEffects;

  // if there are no smoke effects
  if (uiNumSmokeEffects == 0) {
    // set the fact that there are no smoke effects for this sector
    ReSetSectorFlag(sMapX, sMapY, bMapZ, SF_SMOKE_EFFECTS_TEMP_FILE_EXISTS);
    return;
  }

  AutoSGPFile hFile(FileMan::openForWriting(zMapName));

  // Save the Number of Smoke Effects
  FileWrite(hFile, &uiNumSmokeEffects, sizeof(uint32_t));

  CFOR_EACH_SMOKE_EFFECT(s) { InjectSmokeEffectIntoFile(hFile, s); }

  SetSectorFlag(sMapX, sMapY, bMapZ, SF_SMOKE_EFFECTS_TEMP_FILE_EXISTS);
}

void LoadSmokeEffectsFromMapTempFile(int16_t const sMapX, int16_t const sMapY, int8_t const bMapZ) {
  uint32_t uiCnt = 0;
  char zMapName[128];

  GetMapTempFileName(SF_SMOKE_EFFECTS_TEMP_FILE_EXISTS, zMapName, sMapX, sMapY, bMapZ);

  AutoSGPFile hFile(FileMan::openForReadingSmart(zMapName, true));

  // Clear out the old list
  ResetSmokeEffects();

  // Load the Number of Smoke Effects
  FileRead(hFile, &guiNumSmokeEffects, sizeof(uint32_t));

  // loop through and load the list
  for (uiCnt = 0; uiCnt < guiNumSmokeEffects; uiCnt++) {
    ExtractSmokeEffectFromFile(hFile, &gSmokeEffectData[uiCnt]);
  }

  // loop through and apply the smoke effects to the map
  FOR_EACH_SMOKE_EFFECT(s) {
    const int8_t bLevel = (s->bFlags & SMOKE_EFFECT_ON_ROOF ? 1 : 0);
    SpreadEffectSmoke(s, TRUE, bLevel);
  }
}

void ResetSmokeEffects() {
  // Clear out the old list
  memset(gSmokeEffectData, 0, sizeof(SMOKEEFFECT) * NUM_SMOKE_EFFECT_SLOTS);
  guiNumSmokeEffects = 0;
}

void UpdateSmokeEffectGraphics() {
  FOR_EACH_SMOKE_EFFECT(s) {
    const int8_t bLevel = (s->bFlags & SMOKE_EFFECT_ON_ROOF ? 1 : 0);
    SpreadEffectSmoke(s, ERASE_SPREAD_EFFECT, bLevel);
    SpreadEffectSmoke(s, TRUE, bLevel);
  }
}
