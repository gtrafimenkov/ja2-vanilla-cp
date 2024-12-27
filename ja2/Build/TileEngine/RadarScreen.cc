// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "TileEngine/RadarScreen.h"

#include "Directories.h"
#include "JAScreens.h"
#include "Local.h"
#include "Macro.h"
#include "SGP/ButtonSystem.h"
#include "SGP/Font.h"
#include "SGP/HImage.h"
#include "SGP/Line.h"
#include "SGP/MouseSystem.h"
#include "SGP/SGPStrings.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "ScreenIDs.h"
#include "Strategic/GameClock.h"
#include "Strategic/MapScreen.h"
#include "Strategic/MapScreenInterfaceMapInventory.h"
#include "Strategic/Meanwhile.h"
#include "Strategic/StrategicMap.h"
#include "Tactical/AnimationData.h"
#include "Tactical/Interface.h"
#include "Tactical/InterfaceControl.h"
#include "Tactical/Overhead.h"
#include "Tactical/SoldierControl.h"
#include "Tactical/Squads.h"
#include "TileEngine/Environment.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/OverheadMap.h"
#include "TileEngine/RenderDirty.h"
#include "TileEngine/RenderWorld.h"
#include "TileEngine/SysUtil.h"
#include "Utils/FontControl.h"
#include "Utils/Text.h"
#include "Utils/TimerControl.h"

extern int32_t iCurrentMapSectorZ;

// the squad list font
#define SQUAD_FONT COMPFONT

#define SQUAD_REGION_HEIGHT 2 * RADAR_WINDOW_HEIGHT
#define SQUAD_WINDOW_TM_Y RADAR_WINDOW_TM_Y + GetFontHeight(SQUAD_FONT)

// subtractor for squad list from size of radar view region height
#define SUBTRACTOR_FOR_SQUAD_LIST 0

static int16_t gsRadarX;
static int16_t gsRadarY;
static SGPVObject *gusRadarImage;
BOOLEAN fRenderRadarScreen = TRUE;
static int16_t sSelectedSquadLine = -1;

BOOLEAN gfRadarCurrentGuyFlash = FALSE;

static MOUSE_REGION gRadarRegionSquadList[NUMBER_OF_SQUADS];

static void RadarRegionButtonCallback(MOUSE_REGION *pRegion, int32_t iReason);
static void RadarRegionMoveCallback(MOUSE_REGION *pRegion, int32_t iReason);

void InitRadarScreen() {
  // Add region for radar
  uint16_t const x = RADAR_WINDOW_X;
  uint16_t const y = RADAR_WINDOW_TM_Y;
  uint16_t const w = RADAR_WINDOW_WIDTH;
  uint16_t const h = RADAR_WINDOW_HEIGHT;
  MOUSE_REGION *const r = &gRadarRegion;
  MSYS_DefineRegion(r, x, y, x + w, y + h, MSYS_PRIORITY_HIGHEST, 0, RadarRegionMoveCallback,
                    RadarRegionButtonCallback);
  r->Disable();

  gsRadarX = x;
  gsRadarY = y;
}

void LoadRadarScreenBitmap(const char *const filename) {
  ClearOutRadarMapImage();

  // Grab the Map image
  SGPFILENAME image_filename;
  ReplacePath(image_filename, lengthof(image_filename), RADARMAPSDIR "/", filename, ".sti");
  SGPVObject *const radar = AddVideoObjectFromFile(image_filename);
  gusRadarImage = radar;

  // ATE: Add a shade table!
  const SGPPaletteEntry *const pal = radar->Palette();
  radar->pShades[0] = Create16BPPPaletteShaded(pal, 255, 255, 255, FALSE);
  radar->pShades[1] = Create16BPPPaletteShaded(pal, 100, 100, 100, FALSE);

  // Dirty interface
  fInterfacePanelDirty = DIRTYLEVEL1;
}

void ClearOutRadarMapImage() {
  // If we have loaded, remove old one
  if (gusRadarImage) {
    DeleteVideoObject(gusRadarImage);
    gusRadarImage = 0;
  }
}

void MoveRadarScreen() {
  // check if we are allowed to do anything?
  if (!fRenderRadarScreen) return;

  // Remove old region
  MSYS_RemoveRegion(&gRadarRegion);

  // Add new one

  // Move based on inventory panel
  if (gsCurInterfacePanel == SM_PANEL) {
    gsRadarY = RADAR_WINDOW_TM_Y;
  } else {
    gsRadarY = RADAR_WINDOW_TM_Y;
  }

  // Add region for radar
  MSYS_DefineRegion(&gRadarRegion, RADAR_WINDOW_X, (uint16_t)(gsRadarY),
                    RADAR_WINDOW_X + RADAR_WINDOW_WIDTH, (uint16_t)(gsRadarY + RADAR_WINDOW_HEIGHT),
                    MSYS_PRIORITY_HIGHEST, 0, RadarRegionMoveCallback, RadarRegionButtonCallback);
}

static void AdjustWorldCenterFromRadarCoords(int16_t sRadarX, int16_t sRadarY);

static void RadarRegionMoveCallback(MOUSE_REGION *pRegion, int32_t iReason) {
  int16_t sRadarX, sRadarY;

  // check if we are allowed to do anything?
  if (!fRenderRadarScreen) return;

  if (iReason == MSYS_CALLBACK_REASON_MOVE) {
    if (pRegion->ButtonState & MSYS_LEFT_BUTTON) {
      // Use relative coordinates to set center of viewport
      sRadarX = pRegion->RelativeXPos - (RADAR_WINDOW_WIDTH / 2);
      sRadarY = pRegion->RelativeYPos - (RADAR_WINDOW_HEIGHT / 2);

      AdjustWorldCenterFromRadarCoords(sRadarX, sRadarY);

      SetRenderFlags(RENDER_FLAG_FULL);
    }
  }
}

static void RadarRegionButtonCallback(MOUSE_REGION *pRegion, int32_t iReason) {
  int16_t sRadarX, sRadarY;

  // check if we are allowed to do anything?
  if (!fRenderRadarScreen) return;

  if (iReason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
    if (!InOverheadMap()) {
      // Use relative coordinates to set center of viewport
      sRadarX = pRegion->RelativeXPos - (RADAR_WINDOW_WIDTH / 2);
      sRadarY = pRegion->RelativeYPos - (RADAR_WINDOW_HEIGHT / 2);

      AdjustWorldCenterFromRadarCoords(sRadarX, sRadarY);
    } else {
      KillOverheadMap();
    }
  } else if (iReason & MSYS_CALLBACK_REASON_RBUTTON_DWN) {
    if (!InOverheadMap()) {
      GoIntoOverheadMap();
    } else {
      KillOverheadMap();
    }
  }
}

static void CreateDestroyMouseRegionsForSquadList();
static void RenderSquadList();

void RenderRadarScreen() {
  // create / destroy squad list regions as nessacary
  CreateDestroyMouseRegionsForSquadList();

  // check if we are allowed to do anything?
  if (!fRenderRadarScreen) {
    RenderSquadList();
    return;
  }

  // in a meanwhile, don't render any map
  if (AreInMeanwhile()) ClearOutRadarMapImage();

  if (fInterfacePanelDirty == DIRTYLEVEL2 && gusRadarImage) {
    // If night time and on surface, darken the radarmap.
    size_t const shade =
        NightTime() && ((guiCurrentScreen == MAP_SCREEN && iCurrentMapSectorZ == 0) ||
                        (guiCurrentScreen == GAME_SCREEN && gbWorldSectorZ == 0))
            ? 1
            : 0;
    gusRadarImage->CurrentShade(shade);
    BltVideoObject(guiSAVEBUFFER, gusRadarImage, 0, RADAR_WINDOW_X, gsRadarY);
  }

  // First delete what's there
  RestoreExternBackgroundRect(RADAR_WINDOW_X, gsRadarY, RADAR_WINDOW_WIDTH + 1,
                              RADAR_WINDOW_HEIGHT + 1);

  {
    SGPVSurface::Lock l(FRAME_BUFFER);

    SetClippingRegionAndImageWidth(l.Pitch(), RADAR_WINDOW_X, gsRadarY, RADAR_WINDOW_WIDTH - 1,
                                   RADAR_WINDOW_HEIGHT - 1);
    uint16_t *const pDestBuf = l.Buffer<uint16_t>();

    // Cycle fFlash variable
    if (COUNTERDONE(RADAR_MAP_BLINK)) {
      RESETCOUNTER(RADAR_MAP_BLINK);
      gfRadarCurrentGuyFlash = !gfRadarCurrentGuyFlash;
    }

    if (!fInMapMode) {
      // Find the diustance from render center to true world center
      int16_t const sDistToCenterX = gsRenderCenterX - gCenterWorldX;
      int16_t const sDistToCenterY = gsRenderCenterY - gCenterWorldY;

      // From render center in world coords, convert to render center in
      // "screen" coords
      int16_t sScreenCenterX;
      int16_t sScreenCenterY;
      FromCellToScreenCoordinates(sDistToCenterX, sDistToCenterY, &sScreenCenterX, &sScreenCenterY);

      // Subtract screen center
      sScreenCenterX += gsCX;
      sScreenCenterY += gsCY;

      // Get corners in screen coords
      // TOP LEFT
      int16_t const sX_S = (gsVIEWPORT_END_X - gsVIEWPORT_START_X) / 2;
      int16_t const sY_S = (gsVIEWPORT_END_Y - gsVIEWPORT_START_Y) / 2;

      int16_t const sTopLeftWorldX = sScreenCenterX - sX_S;
      int16_t const sTopLeftWorldY = sScreenCenterY - sY_S;
      int16_t const sBottomRightWorldX = sScreenCenterX + sX_S;
      int16_t const sBottomRightWorldY = sScreenCenterY + sY_S;

      // Determine radar coordinates
      int16_t const sRadarCX = gsCX * gdScaleX;
      int16_t const sRadarCY = gsCY * gdScaleY;

      int16_t const sWidth = RADAR_WINDOW_WIDTH;
      int16_t const sHeight = RADAR_WINDOW_HEIGHT;
      int16_t const sX = RADAR_WINDOW_X;

      int16_t const sRadarTLX = sTopLeftWorldX * gdScaleX - sRadarCX + sX + sWidth / 2;
      int16_t const sRadarTLY = sTopLeftWorldY * gdScaleY - sRadarCY + gsRadarY + sHeight / 2;
      int16_t const sRadarBRX = sBottomRightWorldX * gdScaleX - sRadarCX + sX + sWidth / 2;
      int16_t const sRadarBRY = sBottomRightWorldY * gdScaleY - sRadarCY + gsRadarY + sHeight / 2;

      uint16_t const line_colour = Get16BPPColor(FROMRGB(0, 255, 0));
      RectangleDraw(TRUE, sRadarTLX, sRadarTLY, sRadarBRX, sRadarBRY - 1, line_colour, pDestBuf);

      // Re-render radar
      FOR_EACH_MERC(i) {
        SOLDIERTYPE const *const s = *i;

        // Don't place guys in radar until visible!
        if (s->bVisible == -1 && !(gTacticalStatus.uiFlags & SHOW_ALL_MERCS) &&
            !(s->ubMiscSoldierFlags & SOLDIER_MISC_XRAYED)) {
          continue;
        }

        if (s->uiStatusFlags & SOLDIER_DEAD) continue;
        if (s->ubBodyType == CROW) continue;
        if (!GridNoOnVisibleWorldTile(s->sGridNo)) continue;

        // Get fullscreen coordinate for guy's position
        int16_t sXSoldScreen;
        int16_t sYSoldScreen;
        GetAbsoluteScreenXYFromMapPos(s->sGridNo, &sXSoldScreen, &sYSoldScreen);

        // Get radar x and y postion and add starting relative to interface
        int16_t const x = sXSoldScreen * gdScaleX + RADAR_WINDOW_X;
        int16_t const y = sYSoldScreen * gdScaleY + gsRadarY;

        uint32_t const line_colour =
            /* flash selected merc */
            s == GetSelectedMan() && gfRadarCurrentGuyFlash ? 0 :
                                                            /* on roof */
                s->bTeam == OUR_TEAM && s->bLevel > 0 ? FROMRGB(150, 150, 0)
                                                      :
                                                      /* unconscious enemy */
                s->bTeam != OUR_TEAM && s->bLife < OKLIFE ? FROMRGB(128, 128, 128)
                                                          :
                                                          /* hostile civilian */
                s->bTeam == CIV_TEAM && !s->bNeutral && s->bSide != OUR_TEAM
                ? FROMRGB(255, 0, 0)
                : gTacticalStatus.Team[s->bTeam].RadarColor;

        RectangleDraw(TRUE, x, y, x + 1, y + 1, Get16BPPColor(line_colour), pDestBuf);
      }
    } else if (fShowMapInventoryPool) {
      if (iCurrentlyHighLightedItem != -1) {
        int32_t const item_idx =
            iCurrentInventoryPoolPage * MAP_INVENTORY_POOL_SLOT_COUNT + iCurrentlyHighLightedItem;
        WORLDITEM const &wi = pInventoryPoolList[item_idx];
        if (wi.o.ubNumberOfObjects != 0 && wi.sGridNo != 0) {
          int16_t sXSoldScreen;
          int16_t sYSoldScreen;
          GetAbsoluteScreenXYFromMapPos(wi.sGridNo, &sXSoldScreen, &sYSoldScreen);

          // Get radar x and y postion and add starting relative to interface
          int16_t const x = sXSoldScreen * gdScaleX + RADAR_WINDOW_X;
          int16_t const y = sYSoldScreen * gdScaleY + gsRadarY;

          uint16_t const line_colour = fFlashHighLightInventoryItemOnradarMap
                                           ? Get16BPPColor(FROMRGB(0, 255, 0))
                                           : Get16BPPColor(FROMRGB(255, 255, 255));

          RectangleDraw(TRUE, x, y, x + 1, y + 1, line_colour, pDestBuf);
        }
      }
      InvalidateRegion(RADAR_WINDOW_X, gsRadarY, RADAR_WINDOW_X + RADAR_WINDOW_WIDTH,
                       gsRadarY + RADAR_WINDOW_HEIGHT);
    }
  }
}

static void AdjustWorldCenterFromRadarCoords(int16_t sRadarX, int16_t sRadarY) {
  const int16_t SCROLL_X_STEP = WORLD_TILE_X;
  const int16_t SCROLL_Y_STEP = WORLD_TILE_Y * 2;

  int16_t sScreenX, sScreenY;
  int16_t sTempX_W, sTempY_W;
  int16_t sNewCenterWorldX, sNewCenterWorldY;
  int16_t sNumXSteps, sNumYSteps;

  // Use radar scale values to get screen values, then convert ot map values,
  // rounding to nearest middle tile
  sScreenX = (int16_t)(sRadarX / gdScaleX);
  sScreenY = (int16_t)(sRadarY / gdScaleY);

  // Adjust to viewport start!
  sScreenX -= ((gsVIEWPORT_END_X - gsVIEWPORT_START_X) / 2);
  sScreenY -= ((gsVIEWPORT_END_Y - gsVIEWPORT_START_Y) / 2);

  // Make sure these coordinates are multiples of scroll steps
  sNumXSteps = sScreenX / SCROLL_X_STEP;
  sNumYSteps = sScreenY / SCROLL_Y_STEP;

  sScreenX = (sNumXSteps * SCROLL_X_STEP);
  sScreenY = (sNumYSteps * SCROLL_Y_STEP);

  // Adjust back
  sScreenX += ((gsVIEWPORT_END_X - gsVIEWPORT_START_X) / 2);
  sScreenY += ((gsVIEWPORT_END_Y - gsVIEWPORT_START_Y) / 2);

  // Subtract world center
  // sScreenX += gsCX;
  // sScreenY += gsCY;

  // Convert these into world coordinates
  FromScreenToCellCoordinates(sScreenX, sScreenY, &sTempX_W, &sTempY_W);

  // Adjust these to world center
  sNewCenterWorldX = (int16_t)(gCenterWorldX + sTempX_W);
  sNewCenterWorldY = (int16_t)(gCenterWorldY + sTempY_W);

  SetRenderCenter(sNewCenterWorldX, sNewCenterWorldY);
}

void ToggleRadarScreenRender() { fRenderRadarScreen = !fRenderRadarScreen; }

static void TacticalSquadListBtnCallBack(MOUSE_REGION *pRegion, int32_t iReason);
static void TacticalSquadListMvtCallback(MOUSE_REGION *pRegion, int32_t iReason);

// create destroy squad list regions as needed
static void CreateDestroyMouseRegionsForSquadList() {
  // will check the state of renderradarscreen flag and decide if we need to
  // create mouse regions for
  static BOOLEAN fCreated = FALSE;

  if (!fRenderRadarScreen && !fCreated) {
    BltVideoObjectOnce(guiSAVEBUFFER, INTERFACEDIR "/squadpanel.sti", 0, 538, gsVIEWPORT_END_Y);
    RestoreExternBackgroundRect(538, gsVIEWPORT_END_Y, 102, 120);

    // create regions
    int16_t const w = RADAR_WINDOW_WIDTH / 2 - 1;
    int16_t const h = (SQUAD_REGION_HEIGHT - SUBTRACTOR_FOR_SQUAD_LIST) / (NUMBER_OF_SQUADS / 2);
    for (uint32_t i = 0; i < NUMBER_OF_SQUADS; ++i) {
      // run through list of squads and place appropriatly
      int16_t x = RADAR_WINDOW_X;
      int16_t y = SQUAD_WINDOW_TM_Y;
      if (i < NUMBER_OF_SQUADS / 2) {
        // left half of list
        y += i * h;
      } else {
        // right half of list
        x += RADAR_WINDOW_WIDTH / 2;
        y += (i - NUMBER_OF_SQUADS / 2) * h;
      }

      MOUSE_REGION *const r = &gRadarRegionSquadList[i];
      MSYS_DefineRegion(r, x, y, x + w, y + h, MSYS_PRIORITY_HIGHEST, 0,
                        TacticalSquadListMvtCallback, TacticalSquadListBtnCallBack);
      MSYS_SetRegionUserData(r, 0, i);
    }

    sSelectedSquadLine = -1;

    fCreated = TRUE;
  } else if (fRenderRadarScreen && fCreated) {
    // destroy regions
    for (uint32_t i = 0; i < NUMBER_OF_SQUADS; ++i) {
      MSYS_RemoveRegion(&gRadarRegionSquadList[i]);
    }

    if (guiCurrentScreen == GAME_SCREEN) {
      fInterfacePanelDirty = DIRTYLEVEL2;
      MarkButtonsDirty();
      RenderTacticalInterface();
      RenderButtons();
      RenderPausedGameBox();
    }

    fCreated = FALSE;
  }
}

// show list of squads
static void RenderSquadList() {
  int16_t const dx = RADAR_WINDOW_X;
  int16_t const dy = gsRadarY;

  RestoreExternBackgroundRect(dx, dy, RADAR_WINDOW_WIDTH, SQUAD_REGION_HEIGHT);
  ColorFillVideoSurfaceArea(FRAME_BUFFER, dx, dy, dx + RADAR_WINDOW_WIDTH, dy + SQUAD_REGION_HEIGHT,
                            Get16BPPColor(FROMRGB(0, 0, 0)));

  SetFont(SQUAD_FONT);
  SetFontBackground(FONT_BLACK);

  for (uint32_t i = 0; i < NUMBER_OF_SQUADS; ++i) {
    const uint8_t colour = sSelectedSquadLine == i ? FONT_WHITE :  // highlight line?
                               !IsSquadOnCurrentTacticalMap(i) ? FONT_BLACK
                           : CurrentSquad() == (int32_t)i      ? FONT_LTGREEN
                                                               : FONT_DKGREEN;
    SetFontForeground(colour);

    int16_t sX;
    int16_t sY;
    int16_t x = dx;
    int16_t y = SQUAD_WINDOW_TM_Y;
    int16_t const w = RADAR_WINDOW_WIDTH / 2 - 1;
    int16_t const h = 2 * (SQUAD_REGION_HEIGHT - SUBTRACTOR_FOR_SQUAD_LIST) / NUMBER_OF_SQUADS;
    if (i < NUMBER_OF_SQUADS / 2) {
      x += 2;
      y += i * h;
    } else {
      x += RADAR_WINDOW_WIDTH / 2 - 2;
      y += (i - NUMBER_OF_SQUADS / 2) * h;
    }
    FindFontCenterCoordinates(x, y, w, h, pSquadMenuStrings[i], SQUAD_FONT, &sX, &sY);
    MPrint(x, sY, pSquadMenuStrings[i]);
  }
}

static void TacticalSquadListMvtCallback(MOUSE_REGION *pRegion, int32_t iReason) {
  int32_t iValue = -1;

  iValue = MSYS_GetRegionUserData(pRegion, 0);

  if (iReason & MSYS_CALLBACK_REASON_GAIN_MOUSE) {
    if (IsSquadOnCurrentTacticalMap(iValue)) {
      sSelectedSquadLine = (int16_t)iValue;
    }
  }
  if (iReason & MSYS_CALLBACK_REASON_LOST_MOUSE) {
    sSelectedSquadLine = -1;
  }
}

static void TacticalSquadListBtnCallBack(MOUSE_REGION *pRegion, int32_t iReason) {
  // btn callback handler for team list info region
  int32_t iValue = 0;

  iValue = MSYS_GetRegionUserData(pRegion, 0);

  if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    // find out if this squad is valid and on this map..if so, set as selected
    if (IsSquadOnCurrentTacticalMap(iValue)) {
      // ok, squad is here, set as selected
      SetCurrentSquad(iValue, FALSE);

      // stop showing
      fRenderRadarScreen = TRUE;
    }
  }
}
