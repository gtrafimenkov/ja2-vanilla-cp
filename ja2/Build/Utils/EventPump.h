// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef EVENT_PROCESSOR_H
#define EVENT_PROCESSOR_H

#include "SGP/Types.h"

// Enumerate all events for JA2
enum GameEvent {
  S_SETDESIREDDIRECTION,
  S_BEGINFIREWEAPON,
  S_FIREWEAPON,
  S_WEAPONHIT,
  S_NOISE,
  S_GETNEWPATH
};

// This definition is used to denote events with a special delay value;
// it indicates that these events will not be processed until specifically
// called for in a special loop.
#define DEMAND_EVENT_DELAY 0xFFFF

struct EV_S_GETNEWPATH {
  uint16_t usSoldierID;
  uint32_t uiUniqueId;
  int16_t sDestGridNo;
  uint16_t usMovementAnim;
};

struct EV_S_SETDESIREDDIRECTION {
  uint16_t usSoldierID;
  uint32_t uiUniqueId;
  uint16_t usDesiredDirection;
};

struct EV_S_BEGINFIREWEAPON {
  uint16_t usSoldierID;
  uint32_t uiUniqueId;
  int16_t sTargetGridNo;
  int8_t bTargetLevel;
  int8_t bTargetCubeLevel;
};

struct EV_S_FIREWEAPON {
  uint16_t usSoldierID;
  uint32_t uiUniqueId;
  int16_t sTargetGridNo;
  int8_t bTargetLevel;
  int8_t bTargetCubeLevel;
};

struct EV_S_WEAPONHIT {
  uint16_t usSoldierID;
  uint16_t usWeaponIndex;
  int16_t sDamage;
  int16_t sBreathLoss;
  uint16_t usDirection;
  int16_t sXPos;
  int16_t sYPos;
  int16_t sZPos;
  int16_t sRange;
  uint8_t ubAttackerID;
  uint8_t ubSpecial;
  uint8_t ubLocation;
};

struct EV_S_NOISE {
  uint8_t ubNoiseMaker;
  int16_t sGridNo;
  uint8_t bLevel;
  uint8_t ubVolume;
  uint8_t ubNoiseType;
};

void AddGameEvent(GameEvent, uint16_t usDelay, void* pEventData);
BOOLEAN DequeAllGameEvents();
BOOLEAN DequeueAllDemandGameEvents();

// clean out the event queue
BOOLEAN ClearEventQueue();

#endif
