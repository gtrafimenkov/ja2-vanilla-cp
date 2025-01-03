// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "TileEngine/LightEffects.h"

#include <string.h>

#include "Macro.h"
#include "SGP/FileMan.h"
#include "SGP/Random.h"
#include "SaveLoadGame.h"
#include "Strategic/CampaignTypes.h"
#include "Strategic/GameClock.h"
#include "Tactical/HandleItems.h"
#include "Tactical/OppList.h"
#include "Tactical/SoldierControl.h"
#include "Tactical/TacticalSave.h"
#include "Tactical/Weapons.h"
#include "TileEngine/ExplosionControl.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/Lighting.h"
#include "TileEngine/LoadSaveLightEffect.h"
#include "TileEngine/TileAnimation.h"
#include "TileEngine/WorldDef.h"

#define NUM_LIGHT_EFFECT_SLOTS 25

// GLOBAL FOR LIGHT LISTING
static LIGHTEFFECT gLightEffectData[NUM_LIGHT_EFFECT_SLOTS];
static uint32_t guiNumLightEffects = 0;

#define BASE_FOR_EACH_LIGHTEFFECT(type, iter)                                                     \
  for (type *iter = gLightEffectData, *const iter##__end = gLightEffectData + guiNumLightEffects; \
       iter != iter##__end; ++iter)                                                               \
    if (!iter->fAllocated)                                                                        \
      continue;                                                                                   \
    else
#define FOR_EACH_LIGHTEFFECT(iter) BASE_FOR_EACH_LIGHTEFFECT(LIGHTEFFECT, iter)
#define CFOR_EACH_LIGHTEFFECT(iter) BASE_FOR_EACH_LIGHTEFFECT(const LIGHTEFFECT, iter)

static LIGHTEFFECT *GetFreeLightEffect() {
  for (LIGHTEFFECT *l = gLightEffectData; l != gLightEffectData + guiNumLightEffects; ++l) {
    if (!l->fAllocated) return l;
  }
  if (guiNumLightEffects < NUM_LIGHT_EFFECT_SLOTS) {
    return &gLightEffectData[guiNumLightEffects++];
  }
  return NULL;
}

static void UpdateLightingSprite(LIGHTEFFECT *pLight) {
  char LightName[20];
  // Build light....

  sprintf(LightName, "Light%d", pLight->bRadius);

  // Delete old one if one exists...
  if (pLight->light != NULL) {
    LightSpriteDestroy(pLight->light);
    pLight->light = NULL;
  }

  // Effect light.....
  LIGHT_SPRITE *const l = LightSpriteCreate(LightName);
  pLight->light = l;
  if (l == NULL) {
    // Could not light!
    return;
  }

  LightSpritePower(l, TRUE);
  LightSpritePosition(l, CenterX(pLight->sGridNo) / CELL_X_SIZE,
                      CenterY(pLight->sGridNo) / CELL_Y_SIZE);
}

LIGHTEFFECT *NewLightEffect(const int16_t sGridNo, const int8_t bType) {
  uint8_t ubDuration = 0;
  uint8_t ubStartRadius = 0;

  LIGHTEFFECT *const l = GetFreeLightEffect();
  if (l == NULL) return NULL;

  memset(l, 0, sizeof(*l));

  // Set some values...
  l->sGridNo = sGridNo;
  l->bType = bType;
  l->light = NULL;
  l->uiTimeOfLastUpdate = GetWorldTotalSeconds();

  switch (bType) {
    case LIGHT_FLARE_MARK_1:

      ubDuration = 6;
      ubStartRadius = 6;
      break;
  }

  l->ubDuration = ubDuration;
  l->bRadius = ubStartRadius;
  l->bAge = 0;
  l->fAllocated = TRUE;

  UpdateLightingSprite(l);

  // Handle sight here....
  AllTeamsLookForAll(FALSE);

  return l;
}

void DecayLightEffects(uint32_t uiTime) {
  // age all active tear gas clouds, deactivate those that are just dispersing
  FOR_EACH_LIGHTEFFECT(l) {
    // ATE: Do this every so ofte, to acheive the effect we want...
    if (uiTime - l->uiTimeOfLastUpdate <= 350) continue;

    const uint16_t usNumUpdates = (uiTime - l->uiTimeOfLastUpdate) / 350;

    l->uiTimeOfLastUpdate = uiTime;

    BOOLEAN fDelete = FALSE;
    for (uint32_t i = 0; i < usNumUpdates; ++i) {
      l->bAge++;

      // if this cloud remains effective (duration not reached)
      if (l->bAge >= l->ubDuration) {
        fDelete = TRUE;
        break;
      }

      // calculate the new cloud radius
      // cloud expands by 1 every turn outdoors, and every other turn indoors
      if (l->bAge % 2) l->bRadius--;

      if (l->bRadius == 0) {
        fDelete = TRUE;
        break;
      }

      UpdateLightingSprite(l);
    }

    if (fDelete) {
      l->fAllocated = FALSE;
      if (l->light != NULL) LightSpriteDestroy(l->light);
    }

    // Handle sight here....
    AllTeamsLookForAll(FALSE);
  }
}

void LoadLightEffectsFromLoadGameFile(HWFILE const hFile) {
  memset(gLightEffectData, 0, sizeof(LIGHTEFFECT) * NUM_LIGHT_EFFECT_SLOTS);

  // Load the Number of Light Effects
  FileRead(hFile, &guiNumLightEffects, sizeof(uint32_t));

  // if there are lights saved.
  if (guiNumLightEffects != 0) {
    // loop through and apply the light effects to the map
    for (uint32_t uiCount = 0; uiCount < guiNumLightEffects; ++uiCount) {
      ExtractLightEffectFromFile(hFile, &gLightEffectData[uiCount]);
    }
  }

  // loop through and apply the light effects to the map
  FOR_EACH_LIGHTEFFECT(l) { UpdateLightingSprite(l); }
}

void SaveLightEffectsToMapTempFile(int16_t const sMapX, int16_t const sMapY, int8_t const bMapZ) {
  uint32_t uiNumLightEffects = 0;
  char zMapName[128];

  // get the name of the map
  GetMapTempFileName(SF_LIGHTING_EFFECTS_TEMP_FILE_EXISTS, zMapName, sMapX, sMapY, bMapZ);

  // delete file the file.
  FileDelete(zMapName);

  // loop through and count the number of Light effects
  CFOR_EACH_LIGHTEFFECT(l) { ++uiNumLightEffects; }

  // if there are no Light effects
  if (uiNumLightEffects == 0) {
    // set the fact that there are no Light effects for this sector
    ReSetSectorFlag(sMapX, sMapY, bMapZ, SF_LIGHTING_EFFECTS_TEMP_FILE_EXISTS);
    return;
  }

  AutoSGPFile hFile(FileMan::openForWriting(zMapName));

  // Save the Number of Light Effects
  FileWrite(hFile, &uiNumLightEffects, sizeof(uint32_t));

  // loop through and save the number of Light effects
  CFOR_EACH_LIGHTEFFECT(l) { InjectLightEffectIntoFile(hFile, l); }

  SetSectorFlag(sMapX, sMapY, bMapZ, SF_LIGHTING_EFFECTS_TEMP_FILE_EXISTS);
}

void LoadLightEffectsFromMapTempFile(int16_t const sMapX, int16_t const sMapY, int8_t const bMapZ) {
  uint32_t uiCnt = 0;
  char zMapName[128];

  GetMapTempFileName(SF_LIGHTING_EFFECTS_TEMP_FILE_EXISTS, zMapName, sMapX, sMapY, bMapZ);

  AutoSGPFile hFile(FileMan::openForReadingSmart(zMapName, true));

  // Clear out the old list
  ResetLightEffects();

  // Load the Number of Light Effects
  FileRead(hFile, &guiNumLightEffects, sizeof(uint32_t));

  // loop through and load the list
  for (uiCnt = 0; uiCnt < guiNumLightEffects; uiCnt++) {
    ExtractLightEffectFromFile(hFile, &gLightEffectData[uiCnt]);
  }

  // loop through and apply the light effects to the map
  FOR_EACH_LIGHTEFFECT(l) { UpdateLightingSprite(l); }
}

void ResetLightEffects() {
  // Clear out the old list
  memset(gLightEffectData, 0, sizeof(LIGHTEFFECT) * NUM_LIGHT_EFFECT_SLOTS);
  guiNumLightEffects = 0;
}
