#ifndef REAL_TIME_INPUT_H
#define REAL_TIME_INPUT_H

#include "SGP/Types.h"
#include "Tactical/HandleUI.h"

extern BOOLEAN gfBeginBurstSpreadTracking;

extern BOOLEAN gfRTClickLeftHoldIntercepted;

void GetRTMouseButtonInput(UIEventKind *puiNewEvent);
void GetRTMousePositionInput(UIEventKind *puiNewEvent);

#endif
