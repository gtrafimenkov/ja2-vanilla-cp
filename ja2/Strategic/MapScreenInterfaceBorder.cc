// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Strategic/MapScreenInterfaceBorder.h"

#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "Directories.h"
#include "Macro.h"
#include "SGP/ButtonSystem.h"
#include "SGP/Debug.h"
#include "SGP/MouseSystem.h"
#include "SGP/Types.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "Strategic/Assignments.h"
#include "Strategic/CampaignTypes.h"
#include "Strategic/MapScreen.h"
#include "Strategic/MapScreenHelicopter.h"
#include "Strategic/MapScreenInterface.h"
#include "Strategic/MapScreenInterfaceMap.h"
#include "Strategic/MapScreenInterfaceMapInventory.h"
#include "Strategic/StrategicMap.h"
#include "Strategic/StrategicTownLoyalty.h"
#include "Tactical/Interface.h"
#include "TileEngine/RenderDirty.h"
#include "TileEngine/SysUtil.h"
#include "Utils/FontControl.h"
#include "Utils/Message.h"
#include "Utils/Text.h"

#ifdef JA2DEMO
#define MAP_BORDER_FILE INTERFACEDIR "/mapborder0225.sti"
#define BTN_TOWN_X 272
#define BTN_MINE_X 315
#define BTN_TEAMS_X 358
#define BTN_MILITIA_X 401
#define BTN_AIR_X 444
#define BTN_ITEM_X 546

#define MAP_LEVEL_MARKER_X 485
#else
#define MAP_BORDER_FILE INTERFACEDIR "/mbs.sti"
#define BTN_TOWN_X 299
#define BTN_MINE_X 342
#define BTN_TEAMS_X 385
#define BTN_MILITIA_X 428
#define BTN_AIR_X 471
#define BTN_ITEM_X 514

#define MAP_LEVEL_MARKER_X 565
#endif
#define MAP_LEVEL_MARKER_Y 323
#define MAP_LEVEL_MARKER_DELTA 8
#define MAP_LEVEL_MARKER_WIDTH 55

#define MAP_BORDER_X 261
#define MAP_BORDER_Y 0

#define MAP_BORDER_CORNER_X 584
#define MAP_BORDER_CORNER_Y 279

// mouse levels
static MOUSE_REGION LevelMouseRegions[4];

// graphics
static SGPVObject *guiLEVELMARKER;  // the white rectangle highlighting the
                                    // current level on the map border
static SGPVObject *guiMapBorder;
static SGPVObject *guiMapBorderEtaPopUp;  // the map border eta pop up
// static SGPVObject* guiMapBorderCorner;

// scroll direction
int32_t giScrollButtonState = -1;

// flags
BOOLEAN fShowTownFlag = FALSE;
BOOLEAN fShowMineFlag = FALSE;
BOOLEAN fShowTeamFlag = FALSE;
BOOLEAN fShowMilitia = FALSE;
BOOLEAN fShowAircraftFlag = FALSE;
BOOLEAN fShowItemsFlag = FALSE;

BOOLEAN fZoomFlag = FALSE;
// BOOLEAN fShowVehicleFlag = FALSE;

// BOOLEAN fMapScrollDueToPanelButton = FALSE;
// BOOLEAN fCursorIsOnMapScrollButtons = FALSE;
// BOOLEAN fDisabledMapBorder = FALSE;

// buttons & button images
GUIButtonRef giMapBorderButtons[6];
static BUTTON_PICS *giMapBorderButtonsImage[6];

// uint32_t guiMapBorderScrollButtons[ 4 ] = { -1, -1, -1, -1 };

// raise/lower land buttons
// uint32_t guiMapBorderLandRaiseButtons[ 2 ] = { -1, -1 };
// uint32_t guiMapBorderLandRaiseButtonsImage[ 2 ];

// void MapScrollButtonMvtCheck( void );
// BOOLEAN ScrollButtonsDisplayingHelpMessage( void );
// void UpdateScrollButtonStatesWhileScrolling( void );

extern void CancelMapUIMessage();

// void BtnZoomCallback(GUI_BUTTON *btn,int32_t reason);

/*
void BtnScrollNorthMapScreenCallback( GUI_BUTTON *btn,int32_t reason );
void BtnScrollSouthMapScreenCallback( GUI_BUTTON *btn,int32_t reason );
void BtnScrollWestMapScreenCallback( GUI_BUTTON *btn,int32_t reason );
void BtnScrollEastMapScreenCallback( GUI_BUTTON *btn,int32_t reason );
void BtnLowerLevelBtnCallback(GUI_BUTTON *btn,int32_t reason);
void BtnRaiseLevelBtnCallback(GUI_BUTTON *btn,int32_t reason);
*/

void LoadMapBorderGraphics() {
  // this procedure will load the graphics needed for the map border
  guiLEVELMARKER = AddVideoObjectFromFile(INTERFACEDIR "/greenarr.sti");
  guiMapBorder = AddVideoObjectFromFile(MAP_BORDER_FILE);
  guiMapBorderEtaPopUp = AddVideoObjectFromFile(INTERFACEDIR "/eta_pop_up.sti");

  /* corner was removed along with the Zoom feature
          // will load map border corner
          guiMapBorderCorner = AddVideoObjectFromFile(INTERFACEDIR
     "/map_screen_cutout.sti");

          fCursorIsOnMapScrollButtons = FALSE;
  */
}

void DeleteMapBorderGraphics() {
  // procedure will delete graphics loaded for map border
  DeleteVideoObject(guiLEVELMARKER);
  DeleteVideoObject(guiMapBorder);
  DeleteVideoObject(guiMapBorderEtaPopUp);
  //	DeleteVideoObject(guiMapBorderCorner);
}

static void DisplayCurrentLevelMarker();

void RenderMapBorder() {
  // renders the actual border to the guiSAVEBUFFER
  /*
          if( fDisabledMapBorder )
          {
                  return;
          }
  */

  if (fShowMapInventoryPool) {
    // render background, then leave
    BlitInventoryPoolGraphic();
    return;
  }

  BltVideoObject(guiSAVEBUFFER, guiMapBorder, 0, MAP_BORDER_X, MAP_BORDER_Y);

  // show the level marker
  DisplayCurrentLevelMarker();
}

/*
void RenderMapBorderCorner( void )
{
        // renders map border corner to the FRAME_BUFFER
        if( fDisabledMapBorder )
        {
                return;
        }

        if( fShowMapInventoryPool )
        {
                return;
        }

        BltVideoObject(FRAME_BUFFER, guiMapBorderCorner, 0, MAP_BORDER_CORNER_X,
MAP_BORDER_CORNER_Y);

        InvalidateRegion( MAP_BORDER_CORNER_X, MAP_BORDER_CORNER_Y, 635, 315);
}
*/

void RenderMapBorderEtaPopUp() {
  // renders map border corner to the FRAME_BUFFER
  /*
          if( fDisabledMapBorder )
          {
                  return;
          }
  */

  if (fShowMapInventoryPool) {
    return;
  }

  if (fPlotForHelicopter) {
    DisplayDistancesForHelicopter();
    return;
  }

  BltVideoObject(FRAME_BUFFER, guiMapBorderEtaPopUp, 0, MAP_BORDER_X + 215, 291);

  InvalidateRegion(MAP_BORDER_X + 215, 291, MAP_BORDER_X + 215 + 100, 310);
}

static void MakeButton(uint32_t idx, uint32_t gfx, int16_t x, GUI_CALLBACK click,
                       const wchar_t *help) {
  BUTTON_PICS *const img = LoadButtonImage(INTERFACEDIR "/map_border_buttons.sti", gfx, gfx + 9);
  giMapBorderButtonsImage[idx] = img;
  GUIButtonRef const btn = QuickCreateButtonNoMove(img, x, 323, MSYS_PRIORITY_HIGH, click);
  giMapBorderButtons[idx] = btn;
  btn->SetFastHelpText(help);
  btn->SetCursor(MSYS_NO_CURSOR);
}

#if 0
static void MakeButtonScroll(uint32_t idx, int32_t gray, int32_t normal, int16_t x, int16_t y, GUI_CALLBACK click, const wchar_t* help)
{
	int32_t btn = QuickCreateButtonImg(INTERFACEDIR "/map_screen_bottom_arrows.sti", gray, normal, -1, normal + 2, -1, x, y, MSYS_PRIORITY_HIGH, click);
	guiMapBorderScrollButtons[idx] = btn;
	btn->SetFastHelpText(help);
}
#endif

static void BtnAircraftCallback(GUI_BUTTON *btn, int32_t reason);
static void BtnItemCallback(GUI_BUTTON *btn, int32_t reason);
static void BtnMilitiaCallback(GUI_BUTTON *btn, int32_t reason);
static void BtnMineCallback(GUI_BUTTON *btn, int32_t reason);
static void BtnTeamCallback(GUI_BUTTON *btn, int32_t reason);
static void BtnTownCallback(GUI_BUTTON *btn, int32_t reason);
static void InitializeMapBorderButtonStates();

void CreateButtonsForMapBorder() {
  // will create the buttons needed for the map screen border region
#if 0
	MakeButtonScroll(ZOOM_MAP_SCROLL_UP,    11, 4, 602, 303, BtnScrollNorthMapScreenCallback, pMapScreenBorderButtonHelpText[6]);
	MakeButtonScroll(ZOOM_MAP_SCROLL_DWN,   12, 5, 602, 338, BtnScrollSouthMapScreenCallback, pMapScreenBorderButtonHelpText[7]);
	MakeButtonScroll(ZOOM_MAP_SCROLL_LEFT,   9, 0, 584, 322, BtnScrollWestMapScreenCallback,  pMapScreenBorderButtonHelpText[9]);
	MakeButtonScroll(ZOOM_MAP_SCROLL_RIGHT, 10, 1, 619, 322, BtnScrollEastMapScreenCallback,  pMapScreenBorderButtonHelpText[8]);
#endif

  MakeButton(MAP_BORDER_TOWN_BTN, 5, BTN_TOWN_X, BtnTownCallback,
             pMapScreenBorderButtonHelpText[0]);  // towns
  MakeButton(MAP_BORDER_MINE_BTN, 4, BTN_MINE_X, BtnMineCallback,
             pMapScreenBorderButtonHelpText[1]);  // mines
  MakeButton(MAP_BORDER_TEAMS_BTN, 3, BTN_TEAMS_X, BtnTeamCallback,
             pMapScreenBorderButtonHelpText[2]);  // people
  MakeButton(MAP_BORDER_MILITIA_BTN, 8, BTN_MILITIA_X, BtnMilitiaCallback,
             pMapScreenBorderButtonHelpText[5]);  // militia
  MakeButton(MAP_BORDER_AIRSPACE_BTN, 2, BTN_AIR_X, BtnAircraftCallback,
             pMapScreenBorderButtonHelpText[3]);  // airspace
  MakeButton(MAP_BORDER_ITEM_BTN, 1, BTN_ITEM_X, BtnItemCallback,
             pMapScreenBorderButtonHelpText[4]);  // items

  // raise and lower view level

  // raise
  /*
  guiMapBorderLandRaiseButtonsImage[MAP_BORDER_RAISE_LEVEL] =
  LoadButtonImage(INTERFACEDIR "/map_screen_bottom_arrows.sti", 11, 4, -1, 6,
  -1); guiMapBorderLandRaiseButtons[MAP_BORDER_RAISE_LEVEL] =
  QuickCreateButtonNoMove(guiMapBorderLandRaiseButtonsImage[MAP_BORDER_RAISE_LEVEL],
  MAP_BORDER_X + 264, 322, MSYS_PRIORITY_HIGH, BtnRaiseLevelBtnCallback);

  // lower
  guiMapBorderLandRaiseButtonsImage[MAP_BORDER_LOWER_LEVEL] =
  LoadButtonImage(INTERFACEDIR "/map_screen_bottom_arrows.sti", 12, 5, -1, 7,
  -1); guiMapBorderLandRaiseButtons[MAP_BORDER_LOWER_LEVEL] =
  QuickCreateButtonNoMove(guiMapBorderLandRaiseButtonsImage[MAP_BORDER_LOWER_LEVEL],
  MAP_BORDER_X + 264, 340, MSYS_PRIORITY_HIGH, BtnLowerLevelBtnCallback);

*/

  // guiMapBorderLandRaiseButtons[0]->SetFastHelpText(pMapScreenBorderButtonHelpText[10]);
  // guiMapBorderLandRaiseButtons[1]->SetFastHelpText(pMapScreenBorderButtonHelpText[11]);

  //	guiMapBorderLandRaiseButtons[0]->SetCursor(MSYS_NO_CURSOR);
  //	guiMapBorderLandRaiseButtons[1]->SetCursor(MSYS_NO_CURSOR);

  InitializeMapBorderButtonStates();
}

void DeleteMapBorderButtons() {
  uint8_t ubCnt;

  /*
          RemoveButton( guiMapBorderScrollButtons[ 0 ]);
          RemoveButton( guiMapBorderScrollButtons[ 1 ]);
          RemoveButton( guiMapBorderScrollButtons[ 2 ]);
          RemoveButton( guiMapBorderScrollButtons[ 3 ]);
  */

  RemoveButton(giMapBorderButtons[0]);
  RemoveButton(giMapBorderButtons[1]);
  RemoveButton(giMapBorderButtons[2]);
  RemoveButton(giMapBorderButtons[3]);
  RemoveButton(giMapBorderButtons[4]);
  RemoveButton(giMapBorderButtons[5]);

  // RemoveButton( guiMapBorderLandRaiseButtons[ 0 ]);
  // RemoveButton( guiMapBorderLandRaiseButtons[ 1 ]);

  // images

  UnloadButtonImage(giMapBorderButtonsImage[0]);
  UnloadButtonImage(giMapBorderButtonsImage[1]);
  UnloadButtonImage(giMapBorderButtonsImage[2]);
  UnloadButtonImage(giMapBorderButtonsImage[3]);
  UnloadButtonImage(giMapBorderButtonsImage[4]);
  UnloadButtonImage(giMapBorderButtonsImage[5]);

  // UnloadButtonImage( guiMapBorderLandRaiseButtonsImage[ 0 ] );
  // UnloadButtonImage( guiMapBorderLandRaiseButtonsImage[ 1 ] );

  for (ubCnt = 0; ubCnt < 6; ubCnt++) {
    giMapBorderButtonsImage[ubCnt] = NULL;
  }
}

// callbacks

/*
void BtnLowerLevelBtnCallback(GUI_BUTTON *btn,int32_t reason)
{
        if(reason & MSYS_CALLBACK_REASON_LBUTTON_DWN )
        {
                // are help messages being displayed?..redraw
                if( ScrollButtonsDisplayingHelpMessage( ) )
                {
                        fMapPanelDirty = TRUE;
                }

                MarkButtonsDirty( );
        }
        else if(reason & MSYS_CALLBACK_REASON_LBUTTON_UP )
  {
                // go down one level
                GoDownOneLevelInMap( );
        }
}


void BtnRaiseLevelBtnCallback(GUI_BUTTON *btn,int32_t reason)
{
        if(reason & MSYS_CALLBACK_REASON_LBUTTON_DWN )
        {
                // are help messages being displayed?..redraw
                if( ScrollButtonsDisplayingHelpMessage( ) )
                {
                        fMapPanelDirty = TRUE;
                }


                MarkButtonsDirty( );
        }
        else if(reason & MSYS_CALLBACK_REASON_LBUTTON_UP )
  {
                // go up one level
                GoUpOneLevelInMap( );
        }
}
*/

static void CommonBtnCallbackBtnDownChecks();

static void BtnMilitiaCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
    CommonBtnCallbackBtnDownChecks();
    ToggleShowMilitiaMode();
  } else if (reason & MSYS_CALLBACK_REASON_RBUTTON_DWN) {
    CommonBtnCallbackBtnDownChecks();
  }
}

static void BtnTeamCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
    CommonBtnCallbackBtnDownChecks();
    ToggleShowTeamsMode();
  } else if (reason & MSYS_CALLBACK_REASON_RBUTTON_DWN) {
    CommonBtnCallbackBtnDownChecks();
  }
}

static void BtnTownCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
    CommonBtnCallbackBtnDownChecks();
    ToggleShowTownsMode();
  } else if (reason & MSYS_CALLBACK_REASON_RBUTTON_DWN) {
    CommonBtnCallbackBtnDownChecks();
  }
}

static void BtnMineCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
    CommonBtnCallbackBtnDownChecks();
    ToggleShowMinesMode();
  } else if (reason & MSYS_CALLBACK_REASON_RBUTTON_DWN) {
    CommonBtnCallbackBtnDownChecks();
  }
}

static void BtnAircraftCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
    CommonBtnCallbackBtnDownChecks();

    ToggleAirspaceMode();
  } else if (reason & MSYS_CALLBACK_REASON_RBUTTON_DWN) {
    CommonBtnCallbackBtnDownChecks();
  }
}

static void BtnItemCallback(GUI_BUTTON *btn, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
    CommonBtnCallbackBtnDownChecks();

    ToggleItemsFilter();
  } else if (reason & MSYS_CALLBACK_REASON_RBUTTON_DWN) {
    CommonBtnCallbackBtnDownChecks();
  }
}

/*
void BtnZoomCallback(GUI_BUTTON *btn,int32_t reason)
{
        uint16_t sTempXOff=0;
        uint16_t sTempYOff=0;


        if(reason & MSYS_CALLBACK_REASON_LBUTTON_DWN )
        {
                CommonBtnCallbackBtnDownChecks();

                btn->uiFlags ^= BUTTON_CLICKED_ON;
                fZoomFlag = btn->Clicked();
                if (fZoomFlag)
                {
                 if( sSelMapX > 14 )
                 {
                         iZoomX = ( ( sSelMapX + 2 ) / 2 ) * ( MAP_GRID_X * 2 );
                 }
                 else
                 {
                         iZoomX=sSelMapX/2*MAP_GRID_X*2;
                 }

                 if( sOldSelMapY > 14 )
                 {
                         iZoomY = ( ( sSelMapY + 2 ) / 2 ) * ( MAP_GRID_Y * 2 );
                 }
                 else
                 {
                         iZoomY=sSelMapY/2*MAP_GRID_Y*2;
                 }

                }

                fMapPanelDirty=TRUE;
        }
        else if(reason & MSYS_CALLBACK_REASON_RBUTTON_DWN )
        {
                CommonBtnCallbackBtnDownChecks();
        }
}
*/

static void MapBorderButtonOff(uint8_t ubBorderButtonIndex);
static void MapBorderButtonOn(uint8_t ubBorderButtonIndex);

void ToggleShowTownsMode() {
  if (fShowTownFlag) {
    fShowTownFlag = FALSE;
    MapBorderButtonOff(MAP_BORDER_TOWN_BTN);
  } else {
    fShowTownFlag = TRUE;
    MapBorderButtonOn(MAP_BORDER_TOWN_BTN);

    if (fShowMineFlag) {
      fShowMineFlag = FALSE;
      MapBorderButtonOff(MAP_BORDER_MINE_BTN);
    }

    if (fShowAircraftFlag) {
      fShowAircraftFlag = FALSE;
      MapBorderButtonOff(MAP_BORDER_AIRSPACE_BTN);
    }

    if (fShowItemsFlag) {
      fShowItemsFlag = FALSE;
      MapBorderButtonOff(MAP_BORDER_ITEM_BTN);
    }
  }

  fMapPanelDirty = TRUE;
}

void ToggleShowMinesMode() {
  if (fShowMineFlag) {
    fShowMineFlag = FALSE;
    MapBorderButtonOff(MAP_BORDER_MINE_BTN);
  } else {
    fShowMineFlag = TRUE;
    MapBorderButtonOn(MAP_BORDER_MINE_BTN);

    if (fShowTownFlag) {
      fShowTownFlag = FALSE;
      MapBorderButtonOff(MAP_BORDER_TOWN_BTN);
    }

    if (fShowAircraftFlag) {
      fShowAircraftFlag = FALSE;
      MapBorderButtonOff(MAP_BORDER_AIRSPACE_BTN);
    }

    if (fShowItemsFlag) {
      fShowItemsFlag = FALSE;
      MapBorderButtonOff(MAP_BORDER_ITEM_BTN);
    }
  }

  fMapPanelDirty = TRUE;
}

static bool DoesPlayerHaveAnyMilitia();

void ToggleShowMilitiaMode() {
  if (fShowMilitia) {
    fShowMilitia = FALSE;
    MapBorderButtonOff(MAP_BORDER_MILITIA_BTN);
  } else {
    // toggle militia ON
    fShowMilitia = TRUE;
    MapBorderButtonOn(MAP_BORDER_MILITIA_BTN);

    // if Team is ON, turn it OFF
    if (fShowTeamFlag) {
      fShowTeamFlag = FALSE;
      MapBorderButtonOff(MAP_BORDER_TEAMS_BTN);
    }

    /*
                    // if Airspace is ON, turn it OFF
                    if (fShowAircraftFlag)
                    {
                            fShowAircraftFlag = FALSE;
                            MapBorderButtonOff( MAP_BORDER_AIRSPACE_BTN );
                    }
    */

    if (fShowItemsFlag) {
      fShowItemsFlag = FALSE;
      MapBorderButtonOff(MAP_BORDER_ITEM_BTN);
    }

    // check if player has any militia
    if (!DoesPlayerHaveAnyMilitia()) {
      const wchar_t *pwString = NULL;

      // no - so put up a message explaining how it works

      // if he's already training some
      if (IsAnyOneOnPlayersTeamOnThisAssignment(TRAIN_TOWN)) {
        // say they'll show up when training is completed
        pwString = pMapErrorString[28];
      } else {
        // say you need to train them first
        pwString = zMarksMapScreenText[1];
      }

      BeginMapUIMessage(0, pwString);
    }
  }

  fMapPanelDirty = TRUE;
}

void ToggleShowTeamsMode() {
  if (fShowTeamFlag) {
    // turn show teams OFF
    fShowTeamFlag = FALSE;
    MapBorderButtonOff(MAP_BORDER_TEAMS_BTN);

    // dirty regions
    fMapPanelDirty = TRUE;
    fTeamPanelDirty = TRUE;
    fCharacterInfoPanelDirty = TRUE;
  } else {  // turn show teams ON
    TurnOnShowTeamsMode();
  }
}

void ToggleAirspaceMode() {
  if (fShowAircraftFlag) {
    // turn airspace OFF
    fShowAircraftFlag = FALSE;
    MapBorderButtonOff(MAP_BORDER_AIRSPACE_BTN);

    if (fPlotForHelicopter) {
      AbortMovementPlottingMode();
    }

    // dirty regions
    fMapPanelDirty = TRUE;
    fTeamPanelDirty = TRUE;
    fCharacterInfoPanelDirty = TRUE;
  } else {  // turn airspace ON
    TurnOnAirSpaceMode();
  }
}

static void TurnOnItemFilterMode();

void ToggleItemsFilter() {
  if (fShowItemsFlag) {
    // turn items OFF
    fShowItemsFlag = FALSE;
    MapBorderButtonOff(MAP_BORDER_ITEM_BTN);

    // dirty regions
    fMapPanelDirty = TRUE;
    fTeamPanelDirty = TRUE;
    fCharacterInfoPanelDirty = TRUE;
  } else {
    // turn items ON
    TurnOnItemFilterMode();
  }
}

/*
void BtnScrollNorthMapScreenCallback( GUI_BUTTON *btn,int32_t reason )
{
        if(reason & MSYS_CALLBACK_REASON_LBUTTON_DWN )
        {
                // not zoomed in?...don't push down
                if (!fZoomFlag) return;

                // are help messages being displayed?..redraw
                if( ScrollButtonsDisplayingHelpMessage( ) )
                {
                        fMapPanelDirty = TRUE;
                }
        }
        else if(reason & MSYS_CALLBACK_REASON_LBUTTON_UP )
  {
                giScrollButtonState = NORTH_DIR;
                fMapScrollDueToPanelButton = TRUE;
        }
        if( reason & MSYS_CALLBACK_REASON_LBUTTON_REPEAT )
        {
                giScrollButtonState = NORTH_DIR;
        }
}

void BtnScrollSouthMapScreenCallback( GUI_BUTTON *btn,int32_t reason )
{

        if(reason & MSYS_CALLBACK_REASON_LBUTTON_DWN )
        {
                // not zoomed in?...don't push down
                if (!fZoomFlag) return;

                // are help messages being displayed?..redraw
                 if( ScrollButtonsDisplayingHelpMessage( ) )
                 {
                         fMapPanelDirty = TRUE;
                 }
        }
        else if(reason & MSYS_CALLBACK_REASON_LBUTTON_UP )
  {
                giScrollButtonState = SOUTH_DIR;
                fMapScrollDueToPanelButton = TRUE;
        }
        if( reason & MSYS_CALLBACK_REASON_LBUTTON_REPEAT )
        {
                 giScrollButtonState = SOUTH_DIR;
        }
}

void BtnScrollEastMapScreenCallback( GUI_BUTTON *btn,int32_t reason )
{

        if(reason & MSYS_CALLBACK_REASON_LBUTTON_DWN )
        {
          // not zoomed in?...don't push down
                if (!fZoomFlag) return;

                // are help messages being displayed?..redraw
                if( ScrollButtonsDisplayingHelpMessage( ) )
                {
                        fMapPanelDirty = TRUE;
                }
        }
        else if(reason & MSYS_CALLBACK_REASON_LBUTTON_UP )
  {
                giScrollButtonState = EAST_DIR;
                fMapScrollDueToPanelButton = TRUE;
        }
        if( reason & MSYS_CALLBACK_REASON_LBUTTON_REPEAT )
        {
                 giScrollButtonState = EAST_DIR;
        }
}

void BtnScrollWestMapScreenCallback( GUI_BUTTON *btn,int32_t reason )
{

        if(reason & MSYS_CALLBACK_REASON_LBUTTON_DWN )
        {
          // not zoomed in?...don't push down
                if (!fZoomFlag) return;

                // are help messages being displayed?..redraw
                if( ScrollButtonsDisplayingHelpMessage( ) )
                {
                        fMapPanelDirty = TRUE;
                }
        }
        else if(reason & MSYS_CALLBACK_REASON_LBUTTON_UP )
  {
                giScrollButtonState = WEST_DIR;
                fMapScrollDueToPanelButton = TRUE;
        }
        if( reason & MSYS_CALLBACK_REASON_LBUTTON_REPEAT )
        {
                 giScrollButtonState = WEST_DIR;
        }
}


void MapScrollButtonMvtCheck( void  )
{
        // run through each button's mouse region, if mouse cursor there...don't
show map white sector highlight fCursorIsOnMapScrollButtons = FALSE;

        if (guiMapBorderScrollButtons[0]->Area.uiFlags & MSYS_MOUSE_IN_AREA)
        {
                fCursorIsOnMapScrollButtons = TRUE;
        }

        if (guiMapBorderScrollButtons[1]->Area.uiFlags & MSYS_MOUSE_IN_AREA)
        {
                fCursorIsOnMapScrollButtons = TRUE;
        }

        if (guiMapBorderScrollButtons[2]->Area.uiFlags & MSYS_MOUSE_IN_AREA)
        {
                fCursorIsOnMapScrollButtons = TRUE;
        }

        if (guiMapBorderScrollButtons[3]->Area.uiFlags & MSYS_MOUSE_IN_AREA)
        {
                fCursorIsOnMapScrollButtons = TRUE;
        }
}
*/

/*
void HandleMapScrollButtonStates( void )
{
        // will enable/disable map scroll buttons based on zoom mode

        if( fDisabledMapBorder || fShowMapInventoryPool )
        {
                return;
        }

        // if underground, don't want zoom in
        if( iCurrentMapSectorZ )
        {
                if (fZoomFlag)
                {
                        fZoomFlag = FALSE;
                        fMapPanelDirty = TRUE;
                }

                MapBorderButtonOff( MAP_BORDER_ZOOM_BTN );
                DisableButton( giMapBorderButtons[ MAP_BORDER_ZOOM_BTN ]);
        }
        else
        {
                EnableButton( giMapBorderButtons[ MAP_BORDER_ZOOM_BTN ]);
        }

        if( fZoomFlag )
        {
                EnableButton( guiMapBorderScrollButtons[ 0 ]);
          EnableButton( guiMapBorderScrollButtons[ 1 ]);
          EnableButton( guiMapBorderScrollButtons[ 2 ]);
          EnableButton( guiMapBorderScrollButtons[ 3 ]);

                UpdateScrollButtonStatesWhileScrolling(  );

        }
        else
        {

                DisableButton( guiMapBorderScrollButtons[ 0 ]);
          DisableButton( guiMapBorderScrollButtons[ 1 ]);
          DisableButton( guiMapBorderScrollButtons[ 2 ]);
          DisableButton( guiMapBorderScrollButtons[ 3 ]);

        }

        // check mvt too
        MapScrollButtonMvtCheck( );
}
*/

/*
BOOLEAN ScrollButtonsDisplayingHelpMessage( void )
{
        // return if any help messages are being displayed for the scroll
buttons

        if (guiMapBorderScrollButtons[0]->Area.uiFlags & MSYS_HAS_BACKRECT ||
                        guiMapBorderScrollButtons[1]->Area.uiFlags &
MSYS_HAS_BACKRECT || guiMapBorderScrollButtons[2]->Area.uiFlags &
MSYS_HAS_BACKRECT || guiMapBorderScrollButtons[3]->Area.uiFlags &
MSYS_HAS_BACKRECT)
        {
                return( TRUE );
        }

        return( FALSE );
}
*/

static void DisplayCurrentLevelMarker() {
  // display the current level marker on the map border
  /*
          if( fDisabledMapBorder )
          {
                  return;
          }
  */

  // it's actually a white rectangle, not a green arrow!
  BltVideoObject(guiSAVEBUFFER, guiLEVELMARKER, 0, MAP_LEVEL_MARKER_X,
                 MAP_LEVEL_MARKER_Y + MAP_LEVEL_MARKER_DELTA * iCurrentMapSectorZ);
}

static void LevelMarkerBtnCallback(MOUSE_REGION *pRegion, int32_t iReason);

void CreateMouseRegionsForLevelMarkers() {
  for (uint32_t sCounter = 0; sCounter < 4; ++sCounter) {
    MOUSE_REGION *const r = &LevelMouseRegions[sCounter];
    const uint16_t x = MAP_LEVEL_MARKER_X;
    const uint16_t y = MAP_LEVEL_MARKER_Y + MAP_LEVEL_MARKER_DELTA * sCounter;
    const uint16_t w = MAP_LEVEL_MARKER_WIDTH;
    const uint16_t h = MAP_LEVEL_MARKER_DELTA;
    MSYS_DefineRegion(r, x, y, x + w, y + h, MSYS_PRIORITY_HIGH, MSYS_NO_CURSOR, MSYS_NO_CALLBACK,
                      LevelMarkerBtnCallback);

    MSYS_SetRegionUserData(r, 0, sCounter);

    wchar_t sString[64];
    swprintf(sString, lengthof(sString), L"%ls %d", zMarksMapScreenText[0], sCounter + 1);
    r->SetFastHelpText(sString);
  }
}

void DeleteMouseRegionsForLevelMarkers() {
  FOR_EACH(MOUSE_REGION, i, LevelMouseRegions) MSYS_RemoveRegion(&*i);
}

static void LevelMarkerBtnCallback(MOUSE_REGION *pRegion, int32_t iReason) {
  // btn callback handler for assignment screen mask region
  int32_t iCounter = 0;

  iCounter = MSYS_GetRegionUserData(pRegion, 0);

  if ((iReason & MSYS_CALLBACK_REASON_LBUTTON_UP)) {
    JumpToLevel(iCounter);
  }
}

/*
void DisableMapBorderRegion( void )
{
        // will shutdown map border region

        if( fDisabledMapBorder )
        {
                // checked, failed
                return;
        }

        // get rid of graphics and mouse regions
        DeleteMapBorderGraphics( );


        fDisabledMapBorder = TRUE;
}

void EnableMapBorderRegion( void )
{
        // will re-enable mapborder region

        if (!fDisabledMapBorder)
        {
                // checked, failed
                return;
        }

        // re load graphics and buttons
        LoadMapBorderGraphics( );

        fDisabledMapBorder = FALSE;

}
*/

void TurnOnShowTeamsMode() {
  // if mode already on, leave, else set and redraw

  if (!fShowTeamFlag) {
    fShowTeamFlag = TRUE;
    MapBorderButtonOn(MAP_BORDER_TEAMS_BTN);

    if (fShowMilitia) {
      fShowMilitia = FALSE;
      MapBorderButtonOff(MAP_BORDER_MILITIA_BTN);
    }

    /*
                    if (fShowAircraftFlag)
                    {
                            fShowAircraftFlag = FALSE;
                            MapBorderButtonOff( MAP_BORDER_AIRSPACE_BTN );
                    }
    */

    if (fShowItemsFlag) {
      fShowItemsFlag = FALSE;
      MapBorderButtonOff(MAP_BORDER_ITEM_BTN);
    }

    // dirty regions
    fMapPanelDirty = TRUE;
    fTeamPanelDirty = TRUE;
    fCharacterInfoPanelDirty = TRUE;
  }
}

void TurnOnAirSpaceMode() {
  // if mode already on, leave, else set and redraw

  if (!fShowAircraftFlag) {
    fShowAircraftFlag = TRUE;
    MapBorderButtonOn(MAP_BORDER_AIRSPACE_BTN);

    // Turn off towns & mines (mostly because town/mine names overlap SAM site
    // names)
    if (fShowTownFlag) {
      fShowTownFlag = FALSE;
      MapBorderButtonOff(MAP_BORDER_TOWN_BTN);
    }

    if (fShowMineFlag) {
      fShowMineFlag = FALSE;
      MapBorderButtonOff(MAP_BORDER_MINE_BTN);
    }

    /*
                    // Turn off teams and militia
                    if (fShowTeamFlag)
                    {
                            fShowTeamFlag = FALSE;
                            MapBorderButtonOff( MAP_BORDER_TEAMS_BTN );
                    }

                    if (fShowMilitia)
                    {
                            fShowMilitia = FALSE;
                            MapBorderButtonOff( MAP_BORDER_MILITIA_BTN );
                    }
    */

    // Turn off items
    if (fShowItemsFlag) {
      fShowItemsFlag = FALSE;
      MapBorderButtonOff(MAP_BORDER_ITEM_BTN);
    }

    if (bSelectedDestChar != -1) {
      AbortMovementPlottingMode();
    }

    // if showing underground
    if (iCurrentMapSectorZ != 0) {
      // switch to the surface
      JumpToLevel(0);
    }

    // dirty regions
    fMapPanelDirty = TRUE;
    fTeamPanelDirty = TRUE;
    fCharacterInfoPanelDirty = TRUE;
  }
}

static void TurnOnItemFilterMode() {
  // if mode already on, leave, else set and redraw

  if (!fShowItemsFlag) {
    fShowItemsFlag = TRUE;
    MapBorderButtonOn(MAP_BORDER_ITEM_BTN);

    // Turn off towns, mines, teams, militia & airspace if any are on
    if (fShowTownFlag) {
      fShowTownFlag = FALSE;
      MapBorderButtonOff(MAP_BORDER_TOWN_BTN);
    }

    if (fShowMineFlag) {
      fShowMineFlag = FALSE;
      MapBorderButtonOff(MAP_BORDER_MINE_BTN);
    }

    if (fShowTeamFlag) {
      fShowTeamFlag = FALSE;
      MapBorderButtonOff(MAP_BORDER_TEAMS_BTN);
    }

    if (fShowMilitia) {
      fShowMilitia = FALSE;
      MapBorderButtonOff(MAP_BORDER_MILITIA_BTN);
    }

    if (fShowAircraftFlag) {
      fShowAircraftFlag = FALSE;
      MapBorderButtonOff(MAP_BORDER_AIRSPACE_BTN);
    }

    if (bSelectedDestChar != -1 || fPlotForHelicopter) {
      AbortMovementPlottingMode();
    }

    // dirty regions
    fMapPanelDirty = TRUE;
    fTeamPanelDirty = TRUE;
    fCharacterInfoPanelDirty = TRUE;
  }
}

/*
void UpdateLevelButtonStates( void )
{
        EnableButton(guiMapBorderLandRaiseButtons[MAP_BORDER_RAISE_LEVEL],
iCurrentMapSectorZ != 0);
        EnableButton(guiMapBorderLandRaiseButtons[MAP_BORDER_LOWER_LEVEL],
iCurrentMapSectorZ != 3);
}
*/

/*
void UpdateScrollButtonStatesWhileScrolling( void )
{
        // too far west, disable
        if ( iZoomY == NORTH_ZOOM_BOUND )
        {
                guiMapBorderScrollButtons[ZOOM_MAP_SCROLL_UP]->uiFlags &=
~BUTTON_CLICKED_ON; DisableButton( guiMapBorderScrollButtons[ ZOOM_MAP_SCROLL_UP
] );
        }
        else if(iZoomY == SOUTH_ZOOM_BOUND )
        {
                guiMapBorderScrollButtons[ZOOM_MAP_SCROLL_DWN]->uiFlags &=
~BUTTON_CLICKED_ON; DisableButton( guiMapBorderScrollButtons[
ZOOM_MAP_SCROLL_DWN ] );
        }

        // too far west, disable
        if ( iZoomX == WEST_ZOOM_BOUND )
        {
                guiMapBorderScrollButtons[ZOOM_MAP_SCROLL_LEFT]->uiFlags &=
~BUTTON_CLICKED_ON; DisableButton( guiMapBorderScrollButtons[
ZOOM_MAP_SCROLL_LEFT ] );
        }
        else if(iZoomX == EAST_ZOOM_BOUND )
        {
                guiMapBorderScrollButtons[ZOOM_MAP_SCROLL_RIGHT]->uiFlags &=
~BUTTON_CLICKED_ON; DisableButton( guiMapBorderScrollButtons[
ZOOM_MAP_SCROLL_RIGHT ] );
        }

}
*/

// set button states to match map flags
static void InitializeMapBorderButtonStates() {
  if (fShowItemsFlag) {
    MapBorderButtonOn(MAP_BORDER_ITEM_BTN);
  } else {
    MapBorderButtonOff(MAP_BORDER_ITEM_BTN);
  }

  if (fShowTownFlag) {
    MapBorderButtonOn(MAP_BORDER_TOWN_BTN);
  } else {
    MapBorderButtonOff(MAP_BORDER_TOWN_BTN);
  }

  if (fShowMineFlag) {
    MapBorderButtonOn(MAP_BORDER_MINE_BTN);
  } else {
    MapBorderButtonOff(MAP_BORDER_MINE_BTN);
  }

  if (fShowTeamFlag) {
    MapBorderButtonOn(MAP_BORDER_TEAMS_BTN);
  } else {
    MapBorderButtonOff(MAP_BORDER_TEAMS_BTN);
  }

  if (fShowAircraftFlag) {
    MapBorderButtonOn(MAP_BORDER_AIRSPACE_BTN);
  } else {
    MapBorderButtonOff(MAP_BORDER_AIRSPACE_BTN);
  }

  if (fShowMilitia) {
    MapBorderButtonOn(MAP_BORDER_MILITIA_BTN);
  } else {
    MapBorderButtonOff(MAP_BORDER_MILITIA_BTN);
  }
}

static bool DoesPlayerHaveAnyMilitia() {
  FOR_EACH(SECTORINFO const, i, SectorInfo) {
    uint8_t const(&n)[MAX_MILITIA_LEVELS] = i->ubNumberOfCivsAtLevel;
    if (n[GREEN_MILITIA] + n[REGULAR_MILITIA] + n[ELITE_MILITIA] != 0) return true;
  }
  return false;
}

static void CommonBtnCallbackBtnDownChecks() {
  // any click cancels MAP UI messages, unless we're in confirm map move mode
  if (g_ui_message_overlay != NULL && !gfInConfirmMapMoveMode) {
    CancelMapUIMessage();
  }
}

void InitMapScreenFlags() {
  fShowTownFlag = TRUE;
  fShowMineFlag = FALSE;

  fShowTeamFlag = TRUE;
  fShowMilitia = FALSE;

  fShowAircraftFlag = FALSE;
  fShowItemsFlag = FALSE;
}

static void MapBorderButtonOff(uint8_t ubBorderButtonIndex) {
  Assert(ubBorderButtonIndex < 6);

  if (fShowMapInventoryPool) {
    return;
  }

  // if button doesn't exist, return
  GUIButtonRef const b = giMapBorderButtons[ubBorderButtonIndex];
  if (b) b->uiFlags &= ~BUTTON_CLICKED_ON;
}

static void MapBorderButtonOn(uint8_t ubBorderButtonIndex) {
  Assert(ubBorderButtonIndex < 6);

  if (fShowMapInventoryPool) {
    return;
  }

  GUIButtonRef const b = giMapBorderButtons[ubBorderButtonIndex];
  if (b) b->uiFlags |= BUTTON_CLICKED_ON;
}
