#ifndef REAL_TIME_INPUT_H
#define REAL_TIME_INPUT_H

#include "Tactical/Handle_UI.h"
#include "sgp/Types.h"

extern BOOLEAN gfBeginBurstSpreadTracking;

extern BOOLEAN gfRTClickLeftHoldIntercepted;

void GetRTMouseButtonInput(UIEventKind* puiNewEvent);
void GetRTMousePositionInput(UIEventKind* puiNewEvent);

#endif
