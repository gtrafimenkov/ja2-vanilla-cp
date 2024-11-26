#ifndef __LIGHT_EFFECTS
#define __LIGHT_EFFECTS

#include "JA2Types.h"

// Light effect types
enum {
  NO_LIGHT_EFFECT,
  LIGHT_FLARE_MARK_1,
};

struct LIGHTEFFECT {
  int16_t sGridNo;  // gridno at which the tear gas cloud is centered

  uint8_t ubDuration;  // the number of turns will remain effective
  uint8_t bRadius;     // the current radius
  int8_t bAge;         // the number of turns light has been around
  BOOLEAN fAllocated;
  int8_t bType;
  LIGHT_SPRITE *light;
  uint32_t uiTimeOfLastUpdate;
};

// Decays all light effects...
void DecayLightEffects(uint32_t uiTime);

LIGHTEFFECT *NewLightEffect(int16_t sGridNo, int8_t bType);

void LoadLightEffectsFromLoadGameFile(HWFILE);

void SaveLightEffectsToMapTempFile(int16_t sMapX, int16_t sMapY, int8_t bMapZ);
void LoadLightEffectsFromMapTempFile(int16_t sMapX, int16_t sMapY, int8_t bMapZ);
void ResetLightEffects();

#endif
