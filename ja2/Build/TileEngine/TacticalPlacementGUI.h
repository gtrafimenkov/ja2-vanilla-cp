#ifndef __TACTICAL_PLACEMENT_GUI_H
#define __TACTICAL_PLACEMENT_GUI_H

#include "JA2Types.h"

void InitTacticalPlacementGUI();
void TacticalPlacementHandle();

void HandleTacticalPlacementClicksInOverheadMap(int32_t reason);

extern BOOLEAN gfTacticalPlacementGUIActive;
extern BOOLEAN gfEnterTacticalPlacementGUI;

extern SOLDIERTYPE *gpTacticalPlacementSelectedSoldier;
extern SOLDIERTYPE *gpTacticalPlacementHilightedSoldier;

// Saved value.  Contains the last choice for future battles.
extern uint8_t gubDefaultButton;

extern BOOLEAN gfTacticalPlacementGUIDirty;
extern BOOLEAN gfValidLocationsChanged;
extern SGPVObject *giMercPanelImage;

#endif
