#ifndef EDITSCREEN_H
#define EDITSCREEN_H

#include "Editor/EditorDefines.h"
#include "JA2Types.h"
#include "ScreenIDs.h"

#define EDITOR_LIGHT_MAX (SHADE_MIN + SHADE_MAX)
#define EDITOR_LIGHT_FAKE (EDITOR_LIGHT_MAX - SHADE_MAX - 2)

extern BOOLEAN gfFakeLights;

void DisplayWayPoints();

/* For all lights that are in the world (except lights attached to mercs), this
 * function places a marker at its location for editing purposes. */
void ShowLightPositionHandles();

extern void ShowCurrentDrawingMode();

// Create and place a light of selected radius into the world.
BOOLEAN PlaceLight(int16_t radius, GridNo pos);

BOOLEAN RemoveLight(GridNo pos);
extern BOOLEAN gfMercResetUponEditorEntry;

// These go together.  The taskbar has a specific color scheme.
extern uint16_t gusEditorTaskbarColor;
extern uint16_t gusEditorTaskbarHiColor;
extern uint16_t gusEditorTaskbarLoColor;

extern TaskMode iOldTaskMode;
extern TaskMode iCurrentTaskbar;
extern TaskMode iTaskMode;

void ProcessAreaSelection(BOOLEAN fWithLeftButton);

void ShowEntryPoints();

extern BOOLEAN gfConfirmExitFirst;
extern BOOLEAN gfIntendOnEnteringEditor;

void EditScreenInit();
ScreenID EditScreenHandle();
void EditScreenShutdown();

#endif
