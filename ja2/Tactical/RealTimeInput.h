// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef REAL_TIME_INPUT_H
#define REAL_TIME_INPUT_H

#include "SGP/Types.h"
#include "Tactical/HandleUI.h"

extern BOOLEAN gfBeginBurstSpreadTracking;

extern BOOLEAN gfRTClickLeftHoldIntercepted;

void GetRTMouseButtonInput(UIEventKind *puiNewEvent);
void GetRTMousePositionInput(UIEventKind *puiNewEvent);

#endif
