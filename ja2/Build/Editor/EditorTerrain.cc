#include "Editor/EditorTerrain.h"

#include "Editor/CursorModes.h"
#include "Editor/EditScreen.h"
#include "Editor/EditSys.h"
#include "Editor/EditorCallbackPrototypes.h"
#include "Editor/EditorDefines.h"
#include "Editor/EditorTaskbarUtils.h"
#include "Editor/SelectWin.h"
#include "SGP/ButtonSystem.h"
#include "SGP/English.h"
#include "SGP/Font.h"
#include "SGP/HImage.h"
#include "SGP/Input.h"
#include "SGP/MouseSystem.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "Tactical/WorldItems.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/SysUtil.h"
#include "TileEngine/TileDef.h"
#include "TileEngine/WorldDef.h"
#include "Utils/FontControl.h"

BOOLEAN gfShowTerrainTileButtons;
uint8_t ubTerrainTileButtonWeight[NUM_TERRAIN_TILE_REGIONS];
uint16_t usTotalWeight;
BOOLEAN fPrevShowTerrainTileButtons = TRUE;
BOOLEAN fUseTerrainWeights = FALSE;
int32_t TerrainTileSelected = 0, TerrainForegroundTile, TerrainBackgroundTile;
int32_t TerrainTileDrawMode = TERRAIN_TILES_NODRAW;

void EntryInitEditorTerrainInfo() {
  // ResetTerrainTileWeights();
  if (!fUseTerrainWeights) {
    ResetTerrainTileWeights();
  }
}

void ResetTerrainTileWeights() {
  int8_t x;
  for (x = 0; x < NUM_TERRAIN_TILE_REGIONS; x++) {
    ubTerrainTileButtonWeight[x] = 0;
  }
  usTotalWeight = 0;
  fUseTerrainWeights = FALSE;
  gfRenderTaskbar = TRUE;
}

void HideTerrainTileButtons() {
  int8_t x;
  if (gfShowTerrainTileButtons) {
    for (x = BASE_TERRAIN_TILE_REGION_ID; x < NUM_TERRAIN_TILE_REGIONS; x++) {
      DisableEditorRegion(x);
    }
    gfShowTerrainTileButtons = FALSE;
  }
}

void ShowTerrainTileButtons() {
  int8_t x;
  if (!gfShowTerrainTileButtons) {
    for (x = BASE_TERRAIN_TILE_REGION_ID; x < NUM_TERRAIN_TILE_REGIONS; x++) {
      EnableEditorRegion(x);
    }
    gfShowTerrainTileButtons = TRUE;
  }
}

void RenderTerrainTileButtons() {
  // If needed, display the ground tile images
  if (gfShowTerrainTileButtons) {
    uint16_t usFillColorDark, usFillColorLight, usFillColorRed;
    uint16_t x, usX, usX2, usY, usY2;

    usFillColorDark = Get16BPPColor(FROMRGB(24, 61, 81));
    usFillColorLight = Get16BPPColor(FROMRGB(136, 138, 135));
    usFillColorRed = Get16BPPColor(FROMRGB(255, 0, 0));

    usY = 369;
    usY2 = 391;

    SetFont(SMALLCOMPFONT);
    SetFontForeground(FONT_YELLOW);

    for (x = 0; x < NUM_TERRAIN_TILE_REGIONS; x++) {
      usX = 261 + (x * 42);
      usX2 = usX + 42;

      if (x == CurrentPaste && !fUseTerrainWeights) {
        ColorFillVideoSurfaceArea(ButtonDestBuffer, usX, usY, usX2, usY2, usFillColorRed);
      } else {
        ColorFillVideoSurfaceArea(ButtonDestBuffer, usX, usY, usX2, usY2, usFillColorDark);
        ColorFillVideoSurfaceArea(ButtonDestBuffer, usX + 1, usY + 1, usX2, usY2, usFillColorLight);
      }
      ColorFillVideoSurfaceArea(ButtonDestBuffer, usX + 1, usY + 1, usX2 - 1, usY2 - 1, 0);

      const HVOBJECT ts = TileElemFromTileType(x)->hTileSurface;
      ts->CurrentShade(DEFAULT_SHADE_LEVEL);
      BltVideoObject(ButtonDestBuffer, ts, 0, usX + 1, usY + 1);

      if (fUseTerrainWeights) {
        mprintf(usX + 2, usY + 2, L"%d", ubTerrainTileButtonWeight[x]);
      }
    }
  }
}

// This callback is used for each of the terrain tile buttons.  The userData[0]
// field contains the terrain button's index value.
void TerrainTileButtonRegionCallback(MOUSE_REGION *reg, int32_t reason) {
  if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    gfRenderTaskbar = TRUE;
    TerrainTileSelected = MSYS_GetRegionUserData(reg, 0);
    if (TerrainTileDrawMode == TERRAIN_TILES_FOREGROUND) {
      TerrainForegroundTile = TerrainTileSelected;
      CurrentPaste = (uint16_t)TerrainForegroundTile;
      // iEditorToolbarState = TBAR_MODE_DRAW;
      if (IsKeyDown(SHIFT)) {
        fUseTerrainWeights = TRUE;
      }
      if (fUseTerrainWeights) {
        // SHIFT+LEFTCLICK adds weight to the selected terrain tile.
        if (ubTerrainTileButtonWeight[TerrainTileSelected] < 10) {
          ubTerrainTileButtonWeight[TerrainTileSelected]++;
          usTotalWeight++;
        }
      } else {  // Regular LEFTCLICK selects only that terrain tile.
        // When total weight is 0, then the only selected tile is drawn.
        ResetTerrainTileWeights();
      }
    } else if (TerrainTileDrawMode == TERRAIN_TILES_BACKGROUND) {
      TerrainBackgroundTile = TerrainTileSelected;
      iEditorToolbarState = TBAR_MODE_SET_BGRND;
    }
  }
  if (reason & MSYS_CALLBACK_REASON_RBUTTON_UP) {
    gfRenderTaskbar = TRUE;
    TerrainTileSelected = MSYS_GetRegionUserData(reg, 0);
    if (TerrainTileDrawMode == TERRAIN_TILES_FOREGROUND) {
      TerrainForegroundTile = TerrainTileSelected;
      iEditorToolbarState = TBAR_MODE_DRAW;
      if (ubTerrainTileButtonWeight[TerrainTileSelected]) {
        ubTerrainTileButtonWeight[TerrainTileSelected]--;
        usTotalWeight--;
      }
    }
  }
}

void ChooseWeightedTerrainTile() {
  uint16_t x, usWeight;
  int16_t sRandomNum;
  if (!usTotalWeight) {  // Not in the weighted mode.  CurrentPaste will already
                         // contain the selected tile.
    return;
  }
  sRandomNum = rand() % usTotalWeight;
  for (x = 0; x < NUM_TERRAIN_TILE_REGIONS; x++) {
    usWeight = ubTerrainTileButtonWeight[x];
    sRandomNum -= usWeight;
    if (sRandomNum <= 0 && usWeight) {
      CurrentPaste = x;
      return;
    }
  }
}

uint32_t guiSearchType;
uint32_t count, maxCount = 0, calls = 0;

static void Fill(int32_t x, int32_t y) {
  int32_t iMapIndex;

  count++;
  calls++;

  if (count > maxCount) maxCount = count;

  iMapIndex = y * WORLD_COLS + x;
  if (!GridNoOnVisibleWorldTile((int16_t)iMapIndex)) {
    count--;
    return;
  }
  const uint32_t uiCheckType = GetTileType(gpWorldLevelData[iMapIndex].pLandHead->usIndex);
  if (guiSearchType == uiCheckType)
    PasteTextureCommon(iMapIndex);
  else {
    count--;
    return;
  }

  if (y > 0) Fill(x, y - 1);
  if (y < WORLD_ROWS - 1) Fill(x, y + 1);
  if (x > 0) Fill(x - 1, y);
  if (x < WORLD_COLS - 1) Fill(x + 1, y);
  count--;
}

void TerrainFill(uint32_t iMapIndex) {
  int16_t sX, sY;
  // determine what we should be looking for to replace...
  guiSearchType = GetTileType(gpWorldLevelData[iMapIndex].pLandHead->usIndex);

  // check terminating conditions
  if (guiSearchType == CurrentPaste) return;

  ConvertGridNoToXY((int16_t)iMapIndex, &sX, &sY);

  count = 0;

  Fill(sX, sY);
}
