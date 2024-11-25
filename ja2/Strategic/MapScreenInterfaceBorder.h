// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __MAP_INTERFACE_BORDER_H
#define __MAP_INTERFACE_BORDER_H

#include "SGP/ButtonSystem.h"
#include "SGP/Types.h"

#define MAP_BORDER_START_X 261
#define MAP_BORDER_START_Y 0

// scroll directions
enum {
  ZOOM_MAP_SCROLL_UP = 0,
  ZOOM_MAP_SCROLL_DWN,
  ZOOM_MAP_SCROLL_RIGHT,
  ZOOM_MAP_SCROLL_LEFT,
};

enum {
  EAST_DIR = 0,
  WEST_DIR,
  NORTH_DIR,
  SOUTH_DIR,
};
enum {
  MAP_BORDER_TOWN_BTN = 0,
  MAP_BORDER_MINE_BTN,
  MAP_BORDER_TEAMS_BTN,
  MAP_BORDER_AIRSPACE_BTN,
  MAP_BORDER_ITEM_BTN,
  MAP_BORDER_MILITIA_BTN,
};

/*
enum{
        MAP_BORDER_RAISE_LEVEL=0,
        MAP_BORDER_LOWER_LEVEL,
};
*/

extern BOOLEAN fShowTownFlag;
extern BOOLEAN fShowMineFlag;
extern BOOLEAN fShowTeamFlag;
extern BOOLEAN fShowMilitia;
extern BOOLEAN fShowAircraftFlag;
extern BOOLEAN fShowItemsFlag;
extern BOOLEAN fZoomFlag;
// extern BOOLEAN fShowVehicleFlag;

// extern BOOLEAN fMapScrollDueToPanelButton;
// extern BOOLEAN fCursorIsOnMapScrollButtons;
// extern BOOLEAN fDisabledMapBorder;

// scroll animation
extern int32_t giScrollButtonState;

void LoadMapBorderGraphics();
void DeleteMapBorderGraphics();
void RenderMapBorder();
// void RenderMapBorderCorner( void );
// void ResetAircraftButton( void );
// void HandleMapScrollButtonStates( void );

void ToggleShowTownsMode();
void ToggleShowMinesMode();
void ToggleShowMilitiaMode();
void ToggleShowTeamsMode();
void ToggleAirspaceMode();
void ToggleItemsFilter();

void TurnOnShowTeamsMode();
void TurnOnAirSpaceMode();

/*
// enable disable map border
void DisableMapBorderRegion( void );
void EnableMapBorderRegion( void );
*/

// create/destroy buttons for map border region
void DeleteMapBorderButtons();
void CreateButtonsForMapBorder();

// render the pop up for eta  in path plotting in map screen
void RenderMapBorderEtaPopUp();

// void UpdateLevelButtonStates( void );

// create mouse regions for level markers
void CreateMouseRegionsForLevelMarkers();
void DeleteMouseRegionsForLevelMarkers();

void InitMapScreenFlags();

extern GUIButtonRef giMapBorderButtons[];

#endif
