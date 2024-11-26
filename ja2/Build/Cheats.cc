#include "Cheats.h"

#include "GameRes.h"

uint8_t gubCheatLevel;

/** Reset cheat level to initial value. */
void resetCheatLevelToInitialValue() {
  if (isGermanVersion()) {
#define STARTING_CHEAT_LEVEL 0
    gubCheatLevel = STARTING_CHEAT_LEVEL;
  } else {
#define STARTING_CHEAT_LEVEL 0
    gubCheatLevel = STARTING_CHEAT_LEVEL;
  }
}

/** Get cheat code. */
const char *getCheatCode() { return isGermanVersion() ? "iguana" : "gabbi"; }
