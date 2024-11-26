#ifndef _MAP_SCREEN_INTERFACE_MAP_H
#define _MAP_SCREEN_INTERFACE_MAP_H

#include "JA2Types.h"

// functions
void DrawMapIndexBigMap(BOOLEAN fSelectedCursorIsYellow);
// void DrawMapIndexSmallMap( BOOLEAN fSelectedCursorIsYellow );

void DrawMap();

void GetScreenXYFromMapXY(int16_t sMapX, int16_t sMapY, int16_t *psX, int16_t *psY);

void InitializePalettesForMap();
void ShutDownPalettesForMap();

// plot path for helicopter
void PlotPathForHelicopter(int16_t sX, int16_t sY);

// the temp path, where the helicopter could go
void PlotATemporaryPathForHelicopter(int16_t sX, int16_t sY);

// show arrows for this char
void DisplayPathArrows(uint16_t usCharNum, HVOBJECT hMapHandle);

// build path for character
void PlotPathForCharacter(SOLDIERTYPE &, int16_t x, int16_t y, bool tactical_traversal);

// build temp path for character
void PlotATemporaryPathForCharacter(const SOLDIERTYPE *s, int16_t sX, int16_t sY);

// display current/temp paths
void DisplaySoldierPath(SOLDIERTYPE *pCharacter);
void DisplaySoldierTempPath();
void DisplayHelicopterPath();
void DisplayHelicopterTempPath();

// clear path after this sector
uint32_t ClearPathAfterThisSectorForCharacter(SOLDIERTYPE *pCharacter, int16_t sX, int16_t sY);

// cancel path : clear the path completely and gives player feedback message
// that the route was canceled
void CancelPathForCharacter(SOLDIERTYPE *pCharacter);
void CancelPathForVehicle(VEHICLETYPE &, BOOLEAN fAlreadyReversed);

// check if we have waited long enought o update temp path
void DisplayThePotentialPathForHelicopter(int16_t sMapX, int16_t sMapY);

// clear out helicopter list after this sector
uint32_t ClearPathAfterThisSectorForHelicopter(int16_t sX, int16_t sY);

// check to see if sector is highlightable
bool IsTheCursorAllowedToHighLightThisSector(int16_t x, int16_t y);

// restore background for map grids
void RestoreBackgroundForMapGrid(int16_t sMapX, int16_t sMapY);

// clip blits to map view region
void ClipBlitsToMapViewRegion();
void ClipBlitsToMapViewRegionForRectangleAndABit(uint32_t uiDestPitchBYTES);

// clip blits to full screen....restore after use of ClipBlitsToMapViewRegion( )
void RestoreClipRegionToFullScreen();
void RestoreClipRegionToFullScreenForRectangle(uint32_t uiDestPitchBYTES);

// last sector in helicopter's path
int16_t GetLastSectorOfHelicoptersPath();

// display info about helicopter path
void DisplayDistancesForHelicopter();

// display where hei is
void DisplayPositionOfHelicopter();

// check for click
BOOLEAN CheckForClickOverHelicopterIcon(int16_t sX, int16_t sY);

void LoadMapScreenInterfaceMapGraphics();
void DeleteMapScreenInterfaceMapGraphics();

// grab the total number of militia in sector
int32_t GetNumberOfMilitiaInSector(int16_t sSectorX, int16_t sSectorY, int8_t bSectorZ);

// create destroy
void CreateDestroyMilitiaPopUPRegions();

// draw the militia box
void DrawMilitiaPopUpBox();

// Returns true if the player knows how many enemies are in the sector if that
// number is greater than 0. Returns false for all other cases.
uint32_t WhatPlayerKnowsAboutEnemiesInSector(int16_t sSectorX, int16_t sSectorY);

// There is a special case flag used when players encounter enemies in a sector,
// then retreat.  The number of enemies will display on mapscreen until time is
// compressed.  When time is compressed, the flag is cleared, and a question mark
// is displayed to reflect that the player no longer knows.  This is the function
// that clears that flag.
void ClearAnySectorsFlashingNumberOfEnemies();

void InitMapSecrets();

enum {
  ABORT_PLOTTING = 0,
  PATH_CLEARED,
  PATH_SHORTENED,
};

// what the player knows about the enemies in a given sector
enum {
  KNOWS_NOTHING = 0,
  KNOWS_THEYRE_THERE,
  KNOWS_HOW_MANY,
};

// size of squares on the map
#define MAP_GRID_X 21
#define MAP_GRID_Y 18

// scroll bounds
#define EAST_ZOOM_BOUND 378
#define WEST_ZOOM_BOUND 42
#define SOUTH_ZOOM_BOUND 324
#define NORTH_ZOOM_BOUND 36

// map view region
#define MAP_VIEW_START_X 270
#define MAP_VIEW_START_Y 10
#define MAP_VIEW_WIDTH 336
#define MAP_VIEW_HEIGHT 298

// zoomed in grid sizes
#define MAP_GRID_ZOOM_X MAP_GRID_X * 2
#define MAP_GRID_ZOOM_Y MAP_GRID_Y * 2

// number of units wide
#define WORLD_MAP_X 18

// dirty regions for the map
#define DMAP_GRID_X (MAP_GRID_X + 1)
#define DMAP_GRID_Y (MAP_GRID_Y + 1)
#define DMAP_GRID_ZOOM_X (MAP_GRID_ZOOM_X + 1)
#define DMAP_GRID_ZOOM_Y (MAP_GRID_ZOOM_Y + 1)

// Orta position on the map
#define ORTA_SECTOR_X 4
#define ORTA_SECTOR_Y 11

#define TIXA_SECTOR_X 9
#define TIXA_SECTOR_Y 10

// wait time until temp path is drawn, from placing cursor on a map grid
#define MIN_WAIT_TIME_FOR_TEMP_PATH 200

// zoom UL coords
extern int32_t iZoomX;
extern int32_t iZoomY;

// the number of militia on the cursor
extern int16_t sGreensOnCursor;
extern int16_t sRegularsOnCursor;
extern int16_t sElitesOnCursor;

// highlighted sectors
extern int16_t gsHighlightSectorX;
extern int16_t gsHighlightSectorY;

// the viewable map bound region
extern SGPRect MapScreenRect;

// draw temp path
extern BOOLEAN fDrawTempHeliPath;

// selected destination char
extern int8_t bSelectedDestChar;

// current assignment character
extern int8_t bSelectedAssignChar;

// the contract char
extern int8_t bSelectedContractChar;

// has temp path for character path or helicopter been already drawn
extern BOOLEAN fTempPathAlreadyDrawn;

// the currently selected town militia
extern int16_t sSelectedMilitiaTown;

// the selected sectors
extern uint16_t sSelMapX;
extern uint16_t sSelMapY;

extern BOOLEAN fFoundTixa;

void CreateDestroyMilitiaSectorButtons();
BOOLEAN CanRedistributeMilitiaInSector(int16_t sClickedSectorX, int16_t sClickedSectorY,
                                       int8_t bClickedTownId);

#endif
