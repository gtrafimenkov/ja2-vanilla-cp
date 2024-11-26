#include "Utils/EventManager.h"

#include <cstring>

#include "SGP/Container.h"
#include "SGP/MemMan.h"
#include "SGP/WCheck.h"
#include "Utils/TimerControl.h"

typedef SGP::List<EVENT *> EventList;

static EventList *hEventQueue = NULL;
static EventList *hDelayEventQueue = NULL;
static EventList *hDemandEventQueue = NULL;

#define QUEUE_RESIZE 20

void InitializeEventManager() {
  hEventQueue = new EventList(QUEUE_RESIZE);
  hDelayEventQueue = new EventList(QUEUE_RESIZE);
  /* Events on this queue are only processed when specifically called for by
   * code */
  hDemandEventQueue = new EventList(QUEUE_RESIZE);
}

void ShutdownEventManager() {
  delete hEventQueue;
  delete hDelayEventQueue;
  delete hDemandEventQueue;
}

static EventList *GetQueue(EventQueueID ubQueueID);

void AddEvent(uint32_t const uiEvent, uint16_t const usDelay, void *const pEventData,
              uint32_t const uiDataSize, EventQueueID const ubQueueID) {
  EVENT *pEvent = MALLOCE(EVENT, Data, uiDataSize);
  pEvent->TimeStamp = GetJA2Clock();
  pEvent->usDelay = usDelay;
  pEvent->uiEvent = uiEvent;
  pEvent->uiFlags = 0;
  pEvent->uiDataSize = uiDataSize;
  memcpy(pEvent->Data, pEventData, uiDataSize);

  // Add event to queue
  EventList *const hQueue = GetQueue(ubQueueID);
  hQueue->Add(pEvent, hQueue->Size());
}

EVENT *RemoveEvent(uint32_t uiIndex, EventQueueID ubQueueID) try {
  return GetQueue(ubQueueID)->Remove(uiIndex);
} catch (const std::exception &) {
  return 0;
}

EVENT *PeekEvent(uint32_t uiIndex, EventQueueID ubQueueID) try {
  return GetQueue(ubQueueID)->Peek(uiIndex);
} catch (const std::exception &) {
  return 0;
}

BOOLEAN FreeEvent(EVENT *pEvent) {
  CHECKF(pEvent != NULL);
  MemFree(pEvent);
  return TRUE;
}

uint32_t EventQueueSize(EventQueueID ubQueueID) { return (uint32_t)GetQueue(ubQueueID)->Size(); }

static EventList *GetQueue(EventQueueID const ubQueueID) {
  switch (ubQueueID) {
    case PRIMARY_EVENT_QUEUE:
      return hEventQueue;
    case SECONDARY_EVENT_QUEUE:
      return hDelayEventQueue;
    case DEMAND_EVENT_QUEUE:
      return hDemandEventQueue;

    default:
      throw std::logic_error("Tried to get non-existent event queue");
  }
}
