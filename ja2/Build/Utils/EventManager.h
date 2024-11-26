#ifndef __EVENT_MANAGER_H
#define __EVENT_MANAGER_H

#include "SGP/Types.h"

struct EVENT {
  uint32_t TimeStamp;
  uint32_t uiFlags;
  uint16_t usDelay;
  uint32_t uiEvent;
  uint32_t uiDataSize;
  uint8_t Data[];
};

enum EventQueueID { PRIMARY_EVENT_QUEUE, SECONDARY_EVENT_QUEUE, DEMAND_EVENT_QUEUE };

#define EVENT_EXPIRED 0x00000002

void InitializeEventManager();
void ShutdownEventManager();

void AddEvent(uint32_t uiEvent, uint16_t usDelay, void *pEventData, uint32_t uiDataSize,
              EventQueueID);
EVENT *RemoveEvent(uint32_t uiIndex, EventQueueID);
EVENT *PeekEvent(uint32_t uiIndex, EventQueueID);
BOOLEAN FreeEvent(EVENT *pEvent);
uint32_t EventQueueSize(EventQueueID);

#endif
