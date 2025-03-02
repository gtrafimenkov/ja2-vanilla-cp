// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Utils/EventPump.h"

#include <cstring>
#include <stdexcept>

#include "SGP/Debug.h"
#include "Tactical/OppList.h"
#include "Tactical/Overhead.h"
#include "Tactical/SoldierControl.h"
#include "Tactical/Weapons.h"
#include "Utils/EventManager.h"
#include "Utils/TimerControl.h"

static void AddGameEventToQueue(uint32_t uiEvent, uint16_t usDelay, void *pEventData,
                                EventQueueID ubQueueID);

void AddGameEvent(GameEvent const uiEvent, uint16_t const usDelay, void *const pEventData) {
  if (usDelay == DEMAND_EVENT_DELAY) {
    // DebugMsg( TOPIC_JA2, DBG_LEVEL_3, String("AddGameEvent: Sending Local
    // #%d", uiEvent));
    AddGameEventToQueue(uiEvent, 0, pEventData, DEMAND_EVENT_QUEUE);
  } else {
    // DebugMsg( TOPIC_JA2, DBG_LEVEL_3, String("AddGameEvent: Sending Local
    // #%d", uiEvent));
    AddGameEventToQueue(uiEvent, usDelay, pEventData, PRIMARY_EVENT_QUEUE);
  }
}

static void AddGameEventToQueue(uint32_t const uiEvent, uint16_t const usDelay,
                                void *const pEventData, EventQueueID const ubQueueID) {
  // Switch on event type and set size accordingly
  uint32_t uiDataSize;
  switch (uiEvent) {
    case S_SETDESIREDDIRECTION:
      uiDataSize = sizeof(EV_S_SETDESIREDDIRECTION);
      break;
    case S_BEGINFIREWEAPON:
      uiDataSize = sizeof(EV_S_BEGINFIREWEAPON);
      break;  // Delay this event
    case S_FIREWEAPON:
      uiDataSize = sizeof(EV_S_FIREWEAPON);
      break;
    case S_WEAPONHIT:
      uiDataSize = sizeof(EV_S_WEAPONHIT);
      break;
    case S_NOISE:
      uiDataSize = sizeof(EV_S_NOISE);
      break;
    case S_GETNEWPATH:
      uiDataSize = sizeof(EV_S_GETNEWPATH);
      break;

    default:
      throw std::logic_error("Event Pump: Unknown event type");
  }

  AddEvent(uiEvent, usDelay, pEventData, uiDataSize, ubQueueID);
}

static BOOLEAN ExecuteGameEvent(EVENT *pEvent);

BOOLEAN DequeAllGameEvents() {
  uint32_t uiQueueSize;
  uint32_t cnt;

  // First dequeue all primary events
  while (EventQueueSize(PRIMARY_EVENT_QUEUE) > 0) {
    EVENT *pEvent = RemoveEvent(0, PRIMARY_EVENT_QUEUE);
    if (pEvent == NULL) return FALSE;

    // Check if event has a delay and add to secondary queue if so
    if (pEvent->usDelay > 0) {
      AddGameEventToQueue(pEvent->uiEvent, pEvent->usDelay, pEvent->Data, SECONDARY_EVENT_QUEUE);
    } else {
      ExecuteGameEvent(pEvent);
    }

    FreeEvent(pEvent);
  }

  // NOW CHECK SECONDARY QUEUE FOR ANY EXPRIED EVENTS
  uiQueueSize = EventQueueSize(SECONDARY_EVENT_QUEUE);
  for (cnt = 0; cnt < uiQueueSize; cnt++) {
    EVENT *pEvent = PeekEvent(cnt, SECONDARY_EVENT_QUEUE);
    if (pEvent == NULL) return FALSE;

    // Check time
    if (GetJA2Clock() - pEvent->TimeStamp > pEvent->usDelay) {
      ExecuteGameEvent(pEvent);
      pEvent->uiFlags = EVENT_EXPIRED;
    }
  }

  do {
    uiQueueSize = EventQueueSize(SECONDARY_EVENT_QUEUE);
    for (cnt = 0; cnt < uiQueueSize; cnt++) {
      EVENT *pEvent = PeekEvent(cnt, SECONDARY_EVENT_QUEUE);
      if (pEvent == NULL) {
        return FALSE;
      }

      // Check time
      if (pEvent->uiFlags & EVENT_EXPIRED) {
        pEvent = RemoveEvent(cnt, SECONDARY_EVENT_QUEUE);
        FreeEvent(pEvent);
        // Restart loop
        break;
      }
    }
  } while (cnt != uiQueueSize);

  return TRUE;
}

BOOLEAN DequeueAllDemandGameEvents() {
  // Dequeue all events on the demand queue (only)

  while (EventQueueSize(DEMAND_EVENT_QUEUE) > 0) {
    EVENT *pEvent = RemoveEvent(0, DEMAND_EVENT_QUEUE);
    if (pEvent == NULL) return FALSE;

    // Check if event has a delay and add to secondary queue if so
    if (pEvent->usDelay > 0) {
      AddGameEventToQueue(pEvent->uiEvent, pEvent->usDelay, pEvent->Data, SECONDARY_EVENT_QUEUE);
    } else {
      ExecuteGameEvent(pEvent);
    }

    FreeEvent(pEvent);
  }

  return TRUE;
}

static SOLDIERTYPE *GetSoldier(const uint16_t soldier_idx) {
  if (soldier_idx >= TOTAL_SOLDIERS) return NULL;  // XXX assert?
  SOLDIERTYPE &s = GetMan(soldier_idx);
  return s.bActive ? &s : 0;
}

static BOOLEAN ExecuteGameEvent(EVENT *pEvent) {
  // Switch on event type
  switch (pEvent->uiEvent) {
    case S_GETNEWPATH: {
      EV_S_GETNEWPATH SGetNewPath;
      memcpy(&SGetNewPath, pEvent->Data, pEvent->uiDataSize);

      SOLDIERTYPE *pSoldier = GetSoldier(SGetNewPath.usSoldierID);
      if (pSoldier == NULL) {
        // Handle Error?
        DebugMsg(TOPIC_JA2, DBG_LEVEL_3, "Event Pump: Invalid Soldier ID");
        break;
      }

      // check for error
      if (pSoldier->uiUniqueSoldierIdValue != SGetNewPath.uiUniqueId) {
        break;
      }
      // Call soldier function
      DebugMsg(TOPIC_JA2, DBG_LEVEL_3, "Event Pump: GetNewPath");
      EVENT_GetNewSoldierPath(pSoldier, SGetNewPath.sDestGridNo, SGetNewPath.usMovementAnim);
      break;
    }

    case S_SETDESIREDDIRECTION: {
      EV_S_SETDESIREDDIRECTION SSetDesiredDirection;
      memcpy(&SSetDesiredDirection, pEvent->Data, pEvent->uiDataSize);

      SOLDIERTYPE *pSoldier = GetSoldier(SSetDesiredDirection.usSoldierID);
      if (pSoldier == NULL) {
        // Handle Error?
        DebugMsg(TOPIC_JA2, DBG_LEVEL_3, "Event Pump: Invalid Soldier ID");
        break;
      }

      // check for error
      if (pSoldier->uiUniqueSoldierIdValue != SSetDesiredDirection.uiUniqueId) {
        break;
      }

      // Call soldier function
      DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
               String("Event Pump: SetDesiredDirection: Dir( %d )",
                      SSetDesiredDirection.usDesiredDirection));
      EVENT_SetSoldierDesiredDirection(pSoldier, SSetDesiredDirection.usDesiredDirection);
      break;
    }

    case S_BEGINFIREWEAPON: {
      EV_S_BEGINFIREWEAPON SBeginFireWeapon;
      memcpy(&SBeginFireWeapon, pEvent->Data, pEvent->uiDataSize);

      SOLDIERTYPE *pSoldier = GetSoldier(SBeginFireWeapon.usSoldierID);
      if (pSoldier == NULL) {
        pSoldier = NULL;
        break;
        // Handle Error?
        // DebugMsg( TOPIC_JA2, DBG_LEVEL_3, "Event Pump: Invalid Soldier ID");
      }

      // check for error
      if (pSoldier->uiUniqueSoldierIdValue != SBeginFireWeapon.uiUniqueId) {
        break;
      }

      // Call soldier function
      DebugMsg(TOPIC_JA2, DBG_LEVEL_3, "Event Pump: Begin Fire Weapon");
      pSoldier->sTargetGridNo = SBeginFireWeapon.sTargetGridNo;
      pSoldier->bTargetLevel = SBeginFireWeapon.bTargetLevel;
      pSoldier->bTargetCubeLevel = SBeginFireWeapon.bTargetCubeLevel;
      EVENT_FireSoldierWeapon(pSoldier, SBeginFireWeapon.sTargetGridNo);
      break;
    }

    case S_FIREWEAPON: {
      EV_S_FIREWEAPON SFireWeapon;
      memcpy(&SFireWeapon, pEvent->Data, pEvent->uiDataSize);

      SOLDIERTYPE *pSoldier = GetSoldier(SFireWeapon.usSoldierID);
      if (pSoldier == NULL) {
        // Handle Error?
        DebugMsg(TOPIC_JA2, DBG_LEVEL_3, "Event Pump: Invalid Soldier ID");
        break;
      }

      // check for error
      if (pSoldier->uiUniqueSoldierIdValue != SFireWeapon.uiUniqueId) {
        break;
      }

      // Call soldier function
      DebugMsg(TOPIC_JA2, DBG_LEVEL_3, "Event Pump: FireWeapon");
      pSoldier->sTargetGridNo = SFireWeapon.sTargetGridNo;
      pSoldier->bTargetLevel = SFireWeapon.bTargetLevel;
      pSoldier->bTargetCubeLevel = SFireWeapon.bTargetCubeLevel;
      FireWeapon(pSoldier, SFireWeapon.sTargetGridNo);
      break;
    }

    case S_WEAPONHIT: {
      EV_S_WEAPONHIT SWeaponHit;
      memcpy(&SWeaponHit, pEvent->Data, pEvent->uiDataSize);
      DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
               String("Event Pump: WeaponHit %d Damage", SWeaponHit.sDamage));
      WeaponHit(&GetMan(SWeaponHit.usSoldierID), SWeaponHit.usWeaponIndex, SWeaponHit.sDamage,
                SWeaponHit.sBreathLoss, SWeaponHit.usDirection, SWeaponHit.sXPos, SWeaponHit.sYPos,
                SWeaponHit.sZPos, SWeaponHit.sRange, &GetMan(SWeaponHit.ubAttackerID),
                SWeaponHit.ubSpecial, SWeaponHit.ubLocation);
      break;
    }

    case S_NOISE: {
      EV_S_NOISE SNoise;
      memcpy(&SNoise, pEvent->Data, pEvent->uiDataSize);
      DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
               String("Event Pump: Noise from %d at %d/%d, type %d volume %d", SNoise.ubNoiseMaker,
                      SNoise.sGridNo, SNoise.bLevel, SNoise.ubNoiseType, SNoise.ubVolume));
      OurNoise(ID2SOLDIER(SNoise.ubNoiseMaker), SNoise.sGridNo, SNoise.bLevel, SNoise.ubVolume,
               static_cast<NoiseKind>(SNoise.ubNoiseType));
      break;
    }

    default:
      DebugMsg(TOPIC_JA2, DBG_LEVEL_3, "Event Pump: Invalid Event Received");
      return FALSE;
  }

  return TRUE;
}

BOOLEAN ClearEventQueue() {
  // clear out the event queue
  while (EventQueueSize(PRIMARY_EVENT_QUEUE) > 0) {
    EVENT *Event = RemoveEvent(0, PRIMARY_EVENT_QUEUE);
    if (Event == NULL) return FALSE;
    FreeEvent(Event);
  }

  return TRUE;
}
