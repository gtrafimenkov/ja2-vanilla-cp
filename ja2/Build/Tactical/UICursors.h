// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __UI_CURSORS_H
#define __UI_CURSORS_H

#include "JA2Types.h"
#include "Tactical/HandleUI.h"
#include "Tactical/InterfaceCursors.h"
#include "Tactical/ItemTypes.h"

#define REFINE_PUNCH_1 0
#define REFINE_PUNCH_2 6

#define REFINE_KNIFE_1 0
#define REFINE_KNIFE_2 6

UICursorID GetProperItemCursor(SOLDIERTYPE *, GridNo map_pos, BOOLEAN activated);

void HandleLeftClickCursor(SOLDIERTYPE *pSoldier);
void HandleRightClickAdjustCursor(SOLDIERTYPE *pSoldier, int16_t usMapPos);

ItemCursor GetActionModeCursor(SOLDIERTYPE const *);

void HandleUICursorRTFeedback(SOLDIERTYPE *pSoldier);

BOOLEAN GetMouseRecalcAndShowAPFlags(MouseMoveState *, BOOLEAN *pfShowAPs);

#endif
