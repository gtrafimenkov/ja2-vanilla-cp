// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __SMOKE_EFFECTS
#define __SMOKE_EFFECTS

#include "JA2Types.h"

// Smoke effect types
enum SmokeEffectKind {
  NO_SMOKE_EFFECT,
  NORMAL_SMOKE_EFFECT,
  TEARGAS_SMOKE_EFFECT,
  MUSTARDGAS_SMOKE_EFFECT,
  CREATURE_SMOKE_EFFECT,
};

#define SMOKE_EFFECT_INDOORS 0x01
#define SMOKE_EFFECT_ON_ROOF 0x02
#define SMOKE_EFFECT_MARK_FOR_UPDATE 0x04

struct SMOKEEFFECT {
  int16_t sGridNo;  // gridno at which the tear gas cloud is centered

  uint8_t ubDuration;  // the number of turns gas will remain effective
  uint8_t ubRadius;    // the current radius of the cloud in map tiles
  uint8_t bFlags;      // 0 - outdoors (fast spread), 1 - indoors (slow)
  int8_t bAge;         // the number of turns gas has been around
  BOOLEAN fAllocated;
  int8_t bType;
  uint16_t usItem;
  SOLDIERTYPE *owner;
  uint32_t uiTimeOfLastUpdate;
};

// Returns NO_SMOKE_EFFECT if none there...
SmokeEffectKind GetSmokeEffectOnTile(int16_t sGridNo, int8_t bLevel);

// Decays all smoke effects...
void DecaySmokeEffects(uint32_t uiTime);

// Add smoke to gridno
// ( Replacement algorithm uses distance away )
void AddSmokeEffectToTile(SMOKEEFFECT const *, SmokeEffectKind, int16_t sGridNo, int8_t bLevel);

void RemoveSmokeEffectFromTile(int16_t sGridNo, int8_t bLevel);

void NewSmokeEffect(int16_t sGridNo, uint16_t usItem, int8_t bLevel, SOLDIERTYPE *owner);

void LoadSmokeEffectsFromLoadGameFile(HWFILE, uint32_t savegame_version);

void SaveSmokeEffectsToMapTempFile(int16_t sMapX, int16_t sMapY, int8_t bMapZ);
void LoadSmokeEffectsFromMapTempFile(int16_t sMapX, int16_t sMapY, int8_t bMapZ);

void ResetSmokeEffects();

void UpdateSmokeEffectGraphics();

#endif
