#ifndef __GAME_EVENTS_H
#define __GAME_EVENTS_H

#include "Strategic/GameEventHook.h"

#define SEF_DELETION_PENDING 0x02

struct STRATEGICEVENT {
  STRATEGICEVENT *next;
  uint32_t uiTimeStamp;
  uint32_t uiParam;
  uint32_t uiTimeOffset;
  uint8_t ubEventType;
  uint8_t ubCallbackID;
  uint8_t ubFlags;
};

enum StrategicEventFrequency {
  ONETIME_EVENT,
  RANGED_EVENT,
  ENDRANGED_EVENT,
  EVERYDAY_EVENT,
  PERIODIC_EVENT,
  QUEUED_EVENT
};

// part of the game.sav files (not map files)
void SaveStrategicEventsToSavedGame(HWFILE);
void LoadStrategicEventsFromSavedGame(HWFILE);

STRATEGICEVENT *AddAdvancedStrategicEvent(StrategicEventFrequency, StrategicEventKind,
                                          uint32_t uiTimeStamp, uint32_t uiParam);

BOOLEAN ExecuteStrategicEvent(STRATEGICEVENT *pEvent);

extern STRATEGICEVENT *gpEventList;

/* Determines if there are any events that will be processed between the current
 * global time, and the beginning of the next global time. */
bool GameEventsPending(uint32_t adjustment);

/* If there are any events pending, they are processed, until the time limit is
 * reached, or a major event is processed (one that requires the player's
 * attention). */
void ProcessPendingGameEvents(uint32_t uiAdjustment, uint8_t ubWarpCode);

#endif
