// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "TileEngine/OverheadMap.h"

#include <algorithm>
#include <stdio.h>

#include "Directories.h"
#include "GameLoop.h"
#include "GameState.h"
#include "Local.h"
#include "Macro.h"
#include "SGP/ButtonSystem.h"
#include "SGP/English.h"
#include "SGP/Font.h"
#include "SGP/HImage.h"
#include "SGP/Line.h"
#include "SGP/VObject.h"
#include "SGP/VObjectBlitters.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "Strategic/GameClock.h"
#include "SysGlobals.h"
#include "Tactical/Faces.h"
#include "Tactical/HandleItems.h"
#include "Tactical/Interface.h"
#include "Tactical/InterfaceControl.h"
#include "Tactical/InterfaceItems.h"
#include "Tactical/InterfacePanels.h"
#include "Tactical/MapInformation.h"
#include "Tactical/Overhead.h"
#include "Tactical/SoldierFind.h"
#include "Tactical/SoldierInitList.h"
#include "Tactical/Squads.h"
#include "Tactical/WorldItems.h"
#include "TileEngine/Environment.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/RadarScreen.h"
#include "TileEngine/RenderDirty.h"
#include "TileEngine/RenderWorld.h"
#include "TileEngine/Structure.h"
#include "TileEngine/SysUtil.h"
#include "TileEngine/TacticalPlacementGUI.h"
#include "TileEngine/TileDef.h"
#include "TileEngine/TileSurface.h"
#include "TileEngine/WorldDat.h"
#include "TileEngine/WorldDef.h"
#include "Utils/Cursors.h"
#include "Utils/FontControl.h"

#include "SDL_keycode.h"
extern SOLDIERINITNODE *gpSelected;

// OK, these are values that are calculated in InitRenderParams( ) with normal
// view settings. These would be different if we change ANYTHING about the game
// worlkd map sizes...
#define NORMAL_MAP_SCREEN_WIDTH 3160
#define NORMAL_MAP_SCREEN_HEIGHT 1540
#define NORMAL_MAP_SCREEN_X 1580
#define NORMAL_MAP_SCREEN_BY 2400
#define NORMAL_MAP_SCREEN_TY 860

#define FASTMAPROWCOLTOPOS(r, c) ((r) * WORLD_COLS + (c))

struct SMALL_TILE_SURF {
  HVOBJECT vo;
};

struct SMALL_TILE_DB {
  HVOBJECT vo;
  uint16_t usSubIndex;
};

static SMALL_TILE_SURF gSmTileSurf[NUMBEROFTILETYPES];
static SMALL_TILE_DB gSmTileDB[NUMBEROFTILES];
static TileSetID gubSmTileNum = TILESET_INVALID;
static BOOLEAN gfInOverheadMap = FALSE;
static MOUSE_REGION OverheadRegion;
static MOUSE_REGION OverheadBackgroundRegion;
static SGPVObject *uiOVERMAP;
static SGPVObject *uiPERSONS;
BOOLEAN gfOverheadMapDirty = FALSE;
extern BOOLEAN gfRadarCurrentGuyFlash;
static int16_t gsStartRestrictedX;
static int16_t gsStartRestrictedY;
static int16_t gsOveritemPoolGridNo = NOWHERE;

static void CopyOverheadDBShadetablesFromTileset();

void InitNewOverheadDB(TileSetID const ubTilesetID) {
  if (gubSmTileNum == ubTilesetID) return;
  TrashOverheadMap();

  for (uint32_t i = 0; i < NUMBEROFTILETYPES; ++i) {
    const char *filename = gTilesets[ubTilesetID].TileSurfaceFilenames[i];
    TileSetID use_tileset = ubTilesetID;
    if (filename[0] == '\0') {
      // Try loading from default tileset
      filename = gTilesets[GENERIC_1].TileSurfaceFilenames[i];
      use_tileset = GENERIC_1;
    }

    char adjusted_file[128];
    sprintf(adjusted_file, TILESETSDIR "/%d/t/%s", use_tileset, filename);
    SGPVObject *vo;
    try {
      vo = AddVideoObjectFromFile(adjusted_file);
    } catch (...) {
      // Load one we know about
      vo = AddVideoObjectFromFile(TILESETSDIR "/0/t/grass.sti");
    }

    gSmTileSurf[i].vo = vo;
  }

  // Create database
  uint32_t dbSize = 0;
  for (uint32_t i = 0; i < NUMBEROFTILETYPES; ++i) {
    SGPVObject *const vo = gSmTileSurf[i].vo;

    // Get number of regions and check for overflow
    uint32_t const NumRegions = std::min(vo->SubregionCount(), gNumTilesPerType[i]);

    uint32_t k = 0;
    for (; k < NumRegions; ++k) {
      gSmTileDB[dbSize].vo = vo;
      gSmTileDB[dbSize].usSubIndex = k;
      ++dbSize;
    }

    // Handle underflow
    for (; k < gNumTilesPerType[i]; ++k) {
      gSmTileDB[dbSize].vo = vo;
      gSmTileDB[dbSize].usSubIndex = 0;
      ++dbSize;
    }
  }

  gsStartRestrictedX = 0;
  gsStartRestrictedY = 0;

  // Calculate Scale factors because of restricted map scroll regions
  if (gMapInformation.ubRestrictedScrollID != 0) {
    int16_t sX1;
    int16_t sY1;
    int16_t sX2;
    int16_t sY2;
    CalculateRestrictedMapCoords(NORTH, &sX1, &sY1, &sX2, &gsStartRestrictedY, SCREEN_WIDTH, 320);
    CalculateRestrictedMapCoords(WEST, &sX1, &sY1, &gsStartRestrictedX, &sY2, SCREEN_WIDTH, 320);
  }

  // Copy over shade tables from main tileset
  CopyOverheadDBShadetablesFromTileset();
  gubSmTileNum = ubTilesetID;
}

static ITEM_POOL const *GetClosestItemPool(int16_t const sweet_gridno, uint8_t const radius,
                                           int8_t const level) {
  ITEM_POOL const *closest_item_pool = 0;
  int32_t lowest_range = 999999;
  for (int16_t y = -radius; y <= radius; ++y) {
    int32_t const leftmost = (sweet_gridno + WORLD_COLS * y) / WORLD_COLS * WORLD_COLS;
    for (int16_t x = -radius; x <= radius; ++x) {
      int16_t const gridno = sweet_gridno + WORLD_COLS * y + x;
      if (gridno < 0 || WORLD_MAX <= gridno) continue;
      if (gridno < leftmost || leftmost + WORLD_COLS <= gridno) continue;

      ITEM_POOL const *item_pool = GetItemPool(gridno, level);
      if (!item_pool) continue;

      int32_t const range = GetRangeInCellCoordsFromGridNoDiff(sweet_gridno, gridno);
      if (lowest_range <= range) continue;

      lowest_range = range;
      closest_item_pool = item_pool;
    }
  }
  return closest_item_pool;
}

static SOLDIERTYPE *GetClosestMercInOverheadMap(int16_t const sweet_gridno, uint8_t const radius) {
  SOLDIERTYPE *res = 0;
  int32_t lowest_range = 999999;
  for (int16_t y = -radius; y <= radius; ++y) {
    int32_t const leftmost = (sweet_gridno + WORLD_COLS * y) / WORLD_COLS * WORLD_COLS;
    for (int16_t x = -radius; x <= radius; ++x) {
      int16_t const gridno = sweet_gridno + WORLD_COLS * y + x;
      if (gridno < 0 || WORLD_MAX <= gridno) continue;
      if (gridno < leftmost || leftmost + WORLD_COLS <= gridno) continue;

      // Go on sweet stop
      LEVELNODE const *const l = gpWorldLevelData[gridno].pMercHead;
      if (!l) continue;
      SOLDIERTYPE *const s = l->pSoldier;
      if (!l || s->bVisible == -1) continue;

      int32_t const range = GetRangeInCellCoordsFromGridNoDiff(sweet_gridno, gridno);
      if (lowest_range <= range) continue;

      lowest_range = range;
      res = s;
    }
  }
  return res;
}

static int16_t GetOffsetLandHeight(int32_t const gridno) {
  return gpWorldLevelData[gridno].sHeight;
}

static void GetOverheadScreenXYFromGridNo(int16_t const gridno, int16_t *const out_x,
                                          int16_t *const out_y) {
  GetAbsoluteScreenXYFromMapPos(gridno, out_x, out_y);
  int16_t x = *out_x / 5;
  int16_t y = *out_y / 5;

  x += gsStartRestrictedX + 5;
  y += gsStartRestrictedY + 5;

  y -= GetOffsetLandHeight(gridno) / 5;
  y += gsRenderHeight / 5;

  *out_x = x;
  *out_y = y;
}

static void DisplayMercNameInOverhead(SOLDIERTYPE const &s) {
  // Get Screen position of guy
  int16_t x;
  int16_t y;
  GetOverheadScreenXYFromGridNo(s.sGridNo, &x, &y);
  y -= s.sHeightAdjustment / 5 + 13;

  int16_t sX;
  int16_t sY;
  SetFontAttributes(TINYFONT1, FONT_MCOLOR_WHITE);
  FindFontCenterCoordinates(x, y, 1, 1, s.name, TINYFONT1, &sX, &sY);
  GDirtyPrint(sX, sY, s.name);
}

static GridNo GetOverheadMouseGridNoForFullSoldiersGridNo();
static void HandleOverheadUI();
static void RenderOverheadOverlays();

void HandleOverheadMap() {
  gfInOverheadMap = TRUE;
  gsOveritemPoolGridNo = NOWHERE;

  InitNewOverheadDB(giCurrentTilesetID);

  RestoreBackgroundRects();

  RenderOverheadMap(0, WORLD_COLS / 2, 0, 0, SCREEN_WIDTH, 320, FALSE);

  HandleTalkingAutoFaces();

  if (!gfEditMode) {
    if (gfTacticalPlacementGUIActive) {
      TacticalPlacementHandle();
      if (!gfTacticalPlacementGUIActive) return;
    } else {
      HandleOverheadUI();

      if (!gfInOverheadMap) return;
      RenderTacticalInterface();
      RenderRadarScreen();
      RenderClock();
      RenderTownIDString();

      HandleAutoFaces();
    }
  }

  if (!gfEditMode && !gfTacticalPlacementGUIActive) {
    HandleAnyMercInSquadHasCompatibleStuff(NULL);

    int16_t const usMapPos = GetOverheadMouseGridNo();
    if (usMapPos != NOWHERE) {
      const ITEM_POOL *pItemPool;

      // ATE: Find the closest item pool within 5 tiles....
      pItemPool = GetClosestItemPool(usMapPos, 1, 0);
      if (pItemPool != NULL) {
        const STRUCTURE *const structure =
            FindStructure(usMapPos, STRUCTURE_HASITEMONTOP | STRUCTURE_OPENABLE);
        int8_t const bZLevel = GetZLevelOfItemPoolGivenStructure(usMapPos, 0, structure);
        if (AnyItemsVisibleOnLevel(pItemPool, bZLevel)) {
          DrawItemPoolList(pItemPool, bZLevel, gusMouseXPos, gusMouseYPos);
          gsOveritemPoolGridNo = GetWorldItem(pItemPool->iItemIndex).sGridNo;
        }
      }

      pItemPool = GetClosestItemPool(usMapPos, 1, 1);
      if (pItemPool != NULL) {
        const int8_t bZLevel = 0;
        if (AnyItemsVisibleOnLevel(pItemPool, bZLevel)) {
          DrawItemPoolList(pItemPool, bZLevel, gusMouseXPos, gusMouseYPos - 5);
          gsOveritemPoolGridNo = GetWorldItem(pItemPool->iItemIndex).sGridNo;
        }
      }
    }
  }

  RenderOverheadOverlays();

  if (!gfEditMode && !gfTacticalPlacementGUIActive) {
    const SOLDIERTYPE *const sel = GetSelectedMan();
    if (sel != NULL) DisplayMercNameInOverhead(*sel);

    gSelectedGuy = NULL;
    int16_t const usMapPos = GetOverheadMouseGridNoForFullSoldiersGridNo();
    if (usMapPos != NOWHERE) {
      SOLDIERTYPE *const s = GetClosestMercInOverheadMap(usMapPos, 1);
      if (s != NULL) {
        if (s->bTeam == OUR_TEAM) gSelectedGuy = s;
        DisplayMercNameInOverhead(*s);
      }
    }
  }

  RenderButtons();
  SaveBackgroundRects();
  RenderButtonsFastHelp();
  ExecuteBaseDirtyRectQueue();
  EndFrameBufferRender();
  fInterfacePanelDirty = DIRTYLEVEL0;
}

BOOLEAN InOverheadMap() { return (gfInOverheadMap); }

static void ClickOverheadRegionCallback(MOUSE_REGION *reg, int32_t reason);

void GoIntoOverheadMap() {
  gfInOverheadMap = TRUE;

  MSYS_DefineRegion(&OverheadBackgroundRegion, 0, 0, SCREEN_WIDTH, 360, MSYS_PRIORITY_HIGH,
                    CURSOR_NORMAL, MSYS_NO_CALLBACK, MSYS_NO_CALLBACK);

  MSYS_DefineRegion(&OverheadRegion, 0, 0, gsVIEWPORT_END_X, 320, MSYS_PRIORITY_HIGH, CURSOR_NORMAL,
                    MSYS_NO_CALLBACK, ClickOverheadRegionCallback);

  // LOAD CLOSE ANIM
  uiOVERMAP = AddVideoObjectFromFile(INTERFACEDIR "/map_bord.sti");

  // LOAD PERSONS
  uiPERSONS = AddVideoObjectFromFile(INTERFACEDIR "/persons.sti");

  // Add shades to persons....
  SGPVObject *const vo = uiPERSONS;
  SGPPaletteEntry const *const pal = vo->Palette();
  vo->pShades[0] = Create16BPPPaletteShaded(pal, 256, 256, 256, FALSE);
  vo->pShades[1] = Create16BPPPaletteShaded(pal, 310, 310, 310, FALSE);
  vo->pShades[2] = Create16BPPPaletteShaded(pal, 0, 0, 0, FALSE);

  gfOverheadMapDirty = TRUE;

  if (!gfEditMode) {
    // Make sure we are in team panel mode...
    SetNewPanel(0);
    fInterfacePanelDirty = DIRTYLEVEL2;

    // Disable tactical buttons......
    if (!gfEnterTacticalPlacementGUI) {
      // Handle switch of panel....
      HandleTacticalPanelSwitch();
      DisableTacticalTeamPanelButtons(TRUE);
    }

    EmptyBackgroundRects();
  }
}

static void HandleOverheadUI() {
  InputAtom a;
  while (DequeueEvent(&a)) {
    if (a.usEvent == KEY_DOWN) {
      switch (a.usParam) {
        case SDLK_ESCAPE:
        case SDLK_INSERT:
          KillOverheadMap();
          break;

        case 'x':
          if (a.usKeyState & ALT_DOWN) {
            HandleShortCutExitState();
          }
          break;
      }
    }
  }
}

void KillOverheadMap() {
  gfInOverheadMap = FALSE;
  SetRenderFlags(RENDER_FLAG_FULL);
  RenderWorld();

  MSYS_RemoveRegion(&OverheadRegion);
  MSYS_RemoveRegion(&OverheadBackgroundRegion);

  DeleteVideoObject(uiOVERMAP);
  DeleteVideoObject(uiPERSONS);

  HandleTacticalPanelSwitch();
  DisableTacticalTeamPanelButtons(FALSE);
}

static int16_t GetModifiedOffsetLandHeight(int32_t const gridno) {
  int16_t const h = GetOffsetLandHeight(gridno);
  int16_t const mod_h = (h / 80 - 1) * 80;
  return mod_h < 0 ? 0 : mod_h;
}

void RenderOverheadMap(int16_t const sStartPointX_M, int16_t const sStartPointY_M,
                       int16_t const sStartPointX_S, int16_t const sStartPointY_S,
                       int16_t const sEndXS, int16_t const sEndYS, BOOLEAN const fFromMapUtility) {
  if (!gfOverheadMapDirty) return;

  // Black out
  ColorFillVideoSurfaceArea(FRAME_BUFFER, sStartPointX_S, sStartPointY_S, sEndXS, sEndYS, 0);

  InvalidateScreen();
  gfOverheadMapDirty = FALSE;

  {
    SGPVSurface::Lock l(FRAME_BUFFER);
    uint16_t *const pDestBuf = l.Buffer<uint16_t>();
    uint32_t const uiDestPitchBYTES = l.Pitch();

    {  // Begin Render Loop
      int16_t sAnchorPosX_M = sStartPointX_M;
      int16_t sAnchorPosY_M = sStartPointY_M;
      int16_t sAnchorPosX_S = sStartPointX_S;
      int16_t sAnchorPosY_S = sStartPointY_S;
      bool bXOddFlag = false;
      do {
        int16_t sTempPosX_M = sAnchorPosX_M;
        int16_t sTempPosY_M = sAnchorPosY_M;
        int16_t sTempPosX_S = sAnchorPosX_S;
        int16_t sTempPosY_S = sAnchorPosY_S;
        if (bXOddFlag) sTempPosX_S += 4;
        do {
          uint32_t const usTileIndex = FASTMAPROWCOLTOPOS(sTempPosY_M, sTempPosX_M);
          if (usTileIndex < GRIDSIZE) {
            int16_t const sHeight = GetOffsetLandHeight(usTileIndex) / 5;
            for (LEVELNODE const *n = gpWorldLevelData[usTileIndex].pLandStart; n;
                 n = n->pPrevNode) {
              SMALL_TILE_DB const &pTile = gSmTileDB[n->usIndex];
              int16_t const sX = sTempPosX_S;
              int16_t const sY = sTempPosY_S - sHeight + gsRenderHeight / 5;
              pTile.vo->CurrentShade(n->ubShadeLevel);
              Blt8BPPDataTo16BPPBufferTransparent(pDestBuf, uiDestPitchBYTES, pTile.vo, sX, sY,
                                                  pTile.usSubIndex);
            }
          }

          sTempPosX_S += 8;
          ++sTempPosX_M;
          --sTempPosY_M;
        } while (sTempPosX_S < sEndXS);

        if (bXOddFlag) {
          ++sAnchorPosY_M;
        } else {
          ++sAnchorPosX_M;
        }

        bXOddFlag = !bXOddFlag;
        sAnchorPosY_S += 2;
      } while (sAnchorPosY_S < sEndYS);
    }

    {  // Begin Render Loop
      int16_t sAnchorPosX_M = sStartPointX_M;
      int16_t sAnchorPosY_M = sStartPointY_M;
      int16_t sAnchorPosX_S = sStartPointX_S;
      int16_t sAnchorPosY_S = sStartPointY_S;
      bool bXOddFlag = false;
      do {
        int16_t sTempPosX_M = sAnchorPosX_M;
        int16_t sTempPosY_M = sAnchorPosY_M;
        int16_t sTempPosX_S = sAnchorPosX_S;
        int16_t sTempPosY_S = sAnchorPosY_S;
        if (bXOddFlag) sTempPosX_S += 4;
        do {
          uint32_t const usTileIndex = FASTMAPROWCOLTOPOS(sTempPosY_M, sTempPosX_M);
          if (usTileIndex < GRIDSIZE) {
            int16_t const sHeight = GetOffsetLandHeight(usTileIndex) / 5;
            int16_t const sModifiedHeight = GetModifiedOffsetLandHeight(usTileIndex) / 5;

            for (LEVELNODE const *n = gpWorldLevelData[usTileIndex].pObjectHead; n; n = n->pNext) {
              if (n->usIndex >= NUMBEROFTILES) continue;
              // Don't render itempools!
              if (n->uiFlags & LEVELNODE_ITEM) continue;

              SMALL_TILE_DB const &pTile = gSmTileDB[n->usIndex];
              int16_t const sX = sTempPosX_S;
              int16_t sY = sTempPosY_S;

              if (gTileDatabase[n->usIndex].uiFlags & IGNORE_WORLD_HEIGHT) {
                sY -= sModifiedHeight;
              } else {
                sY -= sHeight;
              }

              sY += gsRenderHeight / 5;

              pTile.vo->CurrentShade(n->ubShadeLevel);
              Blt8BPPDataTo16BPPBufferTransparent(pDestBuf, uiDestPitchBYTES, pTile.vo, sX, sY,
                                                  pTile.usSubIndex);
            }

            for (LEVELNODE const *n = gpWorldLevelData[usTileIndex].pShadowHead; n; n = n->pNext) {
              if (n->usIndex >= NUMBEROFTILES) continue;

              SMALL_TILE_DB const &pTile = gSmTileDB[n->usIndex];
              int16_t const sX = sTempPosX_S;
              int16_t sY = sTempPosY_S - sHeight;

              sY += gsRenderHeight / 5;

              pTile.vo->CurrentShade(n->ubShadeLevel);
              Blt8BPPDataTo16BPPBufferShadow(pDestBuf, uiDestPitchBYTES, pTile.vo, sX, sY,
                                             pTile.usSubIndex);
            }

            for (LEVELNODE const *n = gpWorldLevelData[usTileIndex].pStructHead; n; n = n->pNext) {
              if (n->usIndex >= NUMBEROFTILES) continue;
              // Don't render itempools!
              if (n->uiFlags & LEVELNODE_ITEM) continue;

              SMALL_TILE_DB const &pTile = gSmTileDB[n->usIndex];
              int16_t const sX = sTempPosX_S;
              int16_t sY = sTempPosY_S;

              if (gTileDatabase[n->usIndex].uiFlags & IGNORE_WORLD_HEIGHT) {
                sY -= sModifiedHeight;
              } else {
                sY -= sHeight;
              }

              sY += gsRenderHeight / 5;

              pTile.vo->CurrentShade(n->ubShadeLevel);
              Blt8BPPDataTo16BPPBufferTransparent(pDestBuf, uiDestPitchBYTES, pTile.vo, sX, sY,
                                                  pTile.usSubIndex);
            }
          }

          sTempPosX_S += 8;
          ++sTempPosX_M;
          --sTempPosY_M;
        } while (sTempPosX_S < sEndXS);

        if (bXOddFlag) {
          ++sAnchorPosY_M;
        } else {
          ++sAnchorPosX_M;
        }

        bXOddFlag = !bXOddFlag;
        sAnchorPosY_S += 2;
      } while (sAnchorPosY_S < sEndYS);
    }

    {  // ROOF RENDR LOOP
      // Begin Render Loop
      int16_t sAnchorPosX_M = sStartPointX_M;
      int16_t sAnchorPosY_M = sStartPointY_M;
      int16_t sAnchorPosX_S = sStartPointX_S;
      int16_t sAnchorPosY_S = sStartPointY_S;
      bool bXOddFlag = false;
      do {
        int16_t sTempPosX_M = sAnchorPosX_M;
        int16_t sTempPosY_M = sAnchorPosY_M;
        int16_t sTempPosX_S = sAnchorPosX_S;
        int16_t sTempPosY_S = sAnchorPosY_S;
        if (bXOddFlag) sTempPosX_S += 4;
        do {
          uint32_t const usTileIndex = FASTMAPROWCOLTOPOS(sTempPosY_M, sTempPosX_M);
          if (usTileIndex < GRIDSIZE) {
            int16_t const sHeight = GetOffsetLandHeight(usTileIndex) / 5;

            for (LEVELNODE const *n = gpWorldLevelData[usTileIndex].pRoofHead; n; n = n->pNext) {
              if (n->usIndex >= NUMBEROFTILES) continue;
              if (n->uiFlags & LEVELNODE_HIDDEN) continue;

              SMALL_TILE_DB const &pTile = gSmTileDB[n->usIndex];
              int16_t const sX = sTempPosX_S;
              int16_t sY = sTempPosY_S - sHeight;

              sY -= WALL_HEIGHT / 5;
              sY += gsRenderHeight / 5;

              pTile.vo->CurrentShade(n->ubShadeLevel);

              // RENDER!
              Blt8BPPDataTo16BPPBufferTransparent(pDestBuf, uiDestPitchBYTES, pTile.vo, sX, sY,
                                                  pTile.usSubIndex);
            }
          }

          sTempPosX_S += 8;
          ++sTempPosX_M;
          --sTempPosY_M;
        } while (sTempPosX_S < sEndXS);

        if (bXOddFlag) {
          ++sAnchorPosY_M;
        } else {
          ++sAnchorPosX_M;
        }

        bXOddFlag = !bXOddFlag;
        sAnchorPosY_S += 2;
      } while (sAnchorPosY_S < sEndYS);
    }
  }

  // OK, blacken out edges of smaller maps...
  if (gMapInformation.ubRestrictedScrollID != 0) {
    uint16_t const black = Get16BPPColor(FROMRGB(0, 0, 0));
    int16_t sX1;
    int16_t sX2;
    int16_t sY1;
    int16_t sY2;

    CalculateRestrictedMapCoords(NORTH, &sX1, &sY1, &sX2, &sY2, sEndXS, sEndYS);
    ColorFillVideoSurfaceArea(FRAME_BUFFER, sX1, sY1, sX2, sY2, black);

    CalculateRestrictedMapCoords(WEST, &sX1, &sY1, &sX2, &sY2, sEndXS, sEndYS);
    ColorFillVideoSurfaceArea(FRAME_BUFFER, sX1, sY1, sX2, sY2, black);

    CalculateRestrictedMapCoords(SOUTH, &sX1, &sY1, &sX2, &sY2, sEndXS, sEndYS);
    ColorFillVideoSurfaceArea(FRAME_BUFFER, sX1, sY1, sX2, sY2, black);

    CalculateRestrictedMapCoords(EAST, &sX1, &sY1, &sX2, &sY2, sEndXS, sEndYS);
    ColorFillVideoSurfaceArea(FRAME_BUFFER, sX1, sY1, sX2, sY2, black);
  }

  if (!fFromMapUtility) {  // Render border!
    BltVideoObject(FRAME_BUFFER, uiOVERMAP, 0, 0, 0);
  }

  // Update the save buffer
  BltVideoSurface(guiSAVEBUFFER, FRAME_BUFFER, 0, 0, NULL);
}

static void RenderOverheadOverlays() {
  SGPVSurface::Lock l(FRAME_BUFFER);
  uint16_t *const pDestBuf = l.Buffer<uint16_t>();
  uint32_t const uiDestPitchBYTES = l.Pitch();

  // Soldier overlay
  SGPVObject *const marker = uiPERSONS;
  SOLDIERTYPE const *const sel =
      gfTacticalPlacementGUIActive || !gfRadarCurrentGuyFlash ? 0 : GetSelectedMan();
  uint16_t const end =
      gfTacticalPlacementGUIActive ? gTacticalStatus.Team[OUR_TEAM].bLastID : MAX_NUM_SOLDIERS;
  for (uint32_t i = 0; i < end; ++i) {
    SOLDIERTYPE const &s = GetMan(i);
    if (!s.bActive || !s.bInSector) continue;

    if (!gfTacticalPlacementGUIActive && s.bLastRenderVisibleValue == -1 &&
        !(gTacticalStatus.uiFlags & SHOW_ALL_MERCS)) {
      continue;
    }

    if (s.sGridNo == NOWHERE) continue;

    // Soldier is here.  Calculate his screen position based on his current
    // gridno.
    int16_t sX;
    int16_t sY;
    GetOverheadScreenXYFromGridNo(s.sGridNo, &sX, &sY);
    // Now, draw his "doll"

    // adjust for position.
    sX += 2;
    sY -= 5;

    sY -= s.sHeightAdjustment / 5;  // Adjust for height

    uint32_t const shade = &s == sel             ? 2
                           : s.sHeightAdjustment ? 1
                                                 :  // On roof
                               0;
    marker->CurrentShade(shade);

    if (gfEditMode && GameState::getInstance()->isEditorMode() && gpSelected &&
        gpSelected->pSoldier == &s) {  // editor:  show the selected edited merc as the yellow one.
      Blt8BPPDataTo16BPPBufferTransparent(pDestBuf, uiDestPitchBYTES, marker, sX, sY, 0);
    } else {
      uint16_t const region = !gfTacticalPlacementGUIActive              ? s.bTeam
                              : s.uiStatusFlags & SOLDIER_VEHICLE        ? 9
                              : &s == gpTacticalPlacementSelectedSoldier ? 7
                              : &s == gpTacticalPlacementHilightedSoldier && s.uiStatusFlags
                                  ? 8
                                  : s.bTeam;
      Blt8BPPDataTo16BPPBufferTransparent(pDestBuf, uiDestPitchBYTES, marker, sX, sY, region);
      ETRLEObject const &e = marker->SubregionProperties(region);
      RegisterBackgroundRect(BGND_FLAG_SINGLE, sX + e.sOffsetX, sY + e.sOffsetY, e.usWidth,
                             e.usHeight);
    }
  }

  // Items overlay
  if (!gfTacticalPlacementGUIActive) {
    CFOR_EACH_WORLD_ITEM(wi) {
      if (wi->bVisible != VISIBLE && !(gTacticalStatus.uiFlags & SHOW_ALL_ITEMS)) {
        continue;
      }

      int16_t sX;
      int16_t sY;
      GetOverheadScreenXYFromGridNo(wi->sGridNo, &sX, &sY);

      // adjust for position.
      sY += 6;

      uint32_t col;
      if (gsOveritemPoolGridNo == wi->sGridNo) {
        col = FROMRGB(255, 0, 0);
      } else if (gfRadarCurrentGuyFlash) {
        col = FROMRGB(0, 0, 0);
      } else
        switch (wi->bVisible) {
          case HIDDEN_ITEM:
            col = FROMRGB(0, 0, 255);
            break;
          case BURIED:
            col = FROMRGB(255, 0, 0);
            break;
          case HIDDEN_IN_OBJECT:
            col = FROMRGB(0, 0, 255);
            break;
          case INVISIBLE:
            col = FROMRGB(0, 255, 0);
            break;
          case VISIBLE:
            col = FROMRGB(255, 255, 255);
            break;
          default:
            abort();
        }
      PixelDraw(FALSE, sX, sY, Get16BPPColor(col), pDestBuf);
      InvalidateRegion(sX, sY, sX + 1, sY + 1);
    }
  }
}

static void ClickOverheadRegionCallback(MOUSE_REGION *reg, int32_t reason) {
  int16_t sWorldScreenX, sWorldScreenY;

  if (gfTacticalPlacementGUIActive) {
    HandleTacticalPlacementClicksInOverheadMap(reason);
    return;
  }

  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    sWorldScreenX = (gusMouseXPos - gsStartRestrictedX) * 5;
    sWorldScreenY = (gusMouseYPos - gsStartRestrictedY) * 5;

    // Get new proposed center location.
    const GridNo pos = GetMapPosFromAbsoluteScreenXY(sWorldScreenX, sWorldScreenY);
    int16_t cell_x;
    int16_t cell_y;
    ConvertGridNoToCenterCellXY(pos, &cell_x, &cell_y);

    SetRenderCenter(cell_x, cell_y);

    KillOverheadMap();
  } else if (reason & MSYS_CALLBACK_REASON_RBUTTON_DWN) {
    KillOverheadMap();
  }
}

static GridNo InternalGetOverheadMouseGridNo(const int32_t dy) {
  if (!(OverheadRegion.uiFlags & MSYS_MOUSE_IN_AREA)) return NOWHERE;

  // ATE: Adjust alogrithm values a tad to reflect map positioning
  int16_t const sWorldScreenX = (gusMouseXPos - gsStartRestrictedX - 5) * 5;
  int16_t sWorldScreenY = (gusMouseYPos - gsStartRestrictedY + dy) * 5;

  // Get new proposed center location.
  const GridNo grid_no = GetMapPosFromAbsoluteScreenXY(sWorldScreenX, sWorldScreenY);

  // Adjust for height.....
  sWorldScreenY += GetOffsetLandHeight(grid_no);
  sWorldScreenY -= gsRenderHeight;

  return GetMapPosFromAbsoluteScreenXY(sWorldScreenX, sWorldScreenY);
}

GridNo GetOverheadMouseGridNo() { return InternalGetOverheadMouseGridNo(-8); }

static GridNo GetOverheadMouseGridNoForFullSoldiersGridNo() {
  return InternalGetOverheadMouseGridNo(0);
}

void CalculateRestrictedMapCoords(int8_t bDirection, int16_t *psX1, int16_t *psY1, int16_t *psX2,
                                  int16_t *psY2, int16_t sEndXS, int16_t sEndYS) {
  switch (bDirection) {
    case NORTH:

      *psX1 = 0;
      *psX2 = sEndXS;
      *psY1 = 0;
      *psY2 = (abs(NORMAL_MAP_SCREEN_TY - gsTLY) / 5);
      break;

    case WEST:

      *psX1 = 0;
      *psX2 = (abs(-NORMAL_MAP_SCREEN_X - gsTLX) / 5);
      *psY1 = 0;
      *psY2 = sEndYS;
      break;

    case SOUTH:

      *psX1 = 0;
      *psX2 = sEndXS;
      *psY1 = (NORMAL_MAP_SCREEN_HEIGHT - abs(NORMAL_MAP_SCREEN_BY - gsBLY)) / 5;
      *psY2 = sEndYS;
      break;

    case EAST:

      *psX1 = (NORMAL_MAP_SCREEN_WIDTH - abs(NORMAL_MAP_SCREEN_X - gsTRX)) / 5;
      *psX2 = sEndXS;
      *psY1 = 0;
      *psY2 = sEndYS;
      break;
  }
}

static void CopyOverheadDBShadetablesFromTileset() {
  // Loop through tileset
  for (size_t i = 0; i < NUMBEROFTILETYPES; ++i) {
    gSmTileSurf[i].vo->ShareShadetables(gTileSurfaceArray[i]->vo);
  }
}

void TrashOverheadMap() {
  if (gubSmTileNum == TILESET_INVALID) return;
  gubSmTileNum = TILESET_INVALID;

  FOR_EACH(SMALL_TILE_SURF, i, gSmTileSurf) { DeleteVideoObject(i->vo); }
}
