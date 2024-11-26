#include "TileEngine/RenderWorld.h"

#include <algorithm>
#include <math.h>
#include <stdint.h>

#include "GameSettings.h"
#include "GameState.h"
#include "Local.h"
#include "Macro.h"
#include "SGP/Debug.h"
#include "SGP/English.h"
#include "SGP/Font.h"
#include "SGP/HImage.h"
#include "SGP/Shading.h"
#include "SGP/VObject.h"
#include "SGP/VObjectBlitters.h"
#include "SGP/VSurface.h"
#include "SGP/WCheck.h"
#include "SysGlobals.h"
#include "Tactical/AnimationControl.h"
#include "Tactical/AnimationData.h"
#include "Tactical/HandleItems.h"
#include "Tactical/HandleUI.h"
#include "Tactical/Interface.h"
#include "Tactical/InterfaceControl.h"
#include "Tactical/Overhead.h"
#include "Tactical/RottingCorpses.h"
#include "Tactical/SoldierFind.h"
#include "TileEngine/InteractiveTiles.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/RadarScreen.h"
#include "TileEngine/RenderDirty.h"
#include "TileEngine/RenderFun.h"
#include "TileEngine/Structure.h"
#include "TileEngine/SysUtil.h"
#include "TileEngine/TileCache.h"
#include "TileEngine/TileDef.h"
#include "Utils/FontControl.h"
#include "Utils/SoundControl.h"
#include "Utils/TimerControl.h"

#include "SDL_keycode.h"

uint16_t *gpZBuffer = NULL;

static int16_t gsCurrentGlowFrame = 0;
static int16_t gsCurrentItemGlowFrame = 0;

// VIEWPORT OFFSET VALUES
// NOTE: THESE VALUES MUST BE MULTIPLES OF TILE SIZES!
#define VIEWPORT_XOFFSET_S (WORLD_TILE_X * 1)
#define VIEWPORT_YOFFSET_S (WORLD_TILE_Y * 2)
#define LARGER_VIEWPORT_XOFFSET_S (VIEWPORT_XOFFSET_S * 3)
#define LARGER_VIEWPORT_YOFFSET_S (VIEWPORT_YOFFSET_S * 5)

enum RenderTilesFlags {
  TILES_NONE = 0,
  TILES_DYNAMIC_CHECKFOR_INT_TILE = 0x00000400,
  TILES_DIRTY = 0x80000000,
  TILES_MARKED = 0x10000000,
  TILES_OBSCURED = 0x01000000
};
ENUM_BITSET(RenderTilesFlags)

#define MAX_RENDERED_ITEMS 2

// RENDERER FLAGS FOR DIFFERENT RENDER LEVELS
enum RenderLayerID {
  RENDER_STATIC_LAND,
  RENDER_STATIC_OBJECTS,
  RENDER_STATIC_SHADOWS,
  RENDER_STATIC_STRUCTS,
  RENDER_STATIC_ROOF,
  RENDER_STATIC_ONROOF,
  RENDER_STATIC_TOPMOST,
  RENDER_DYNAMIC_LAND,
  RENDER_DYNAMIC_OBJECTS,
  RENDER_DYNAMIC_SHADOWS,
  RENDER_DYNAMIC_STRUCT_MERCS,
  RENDER_DYNAMIC_MERCS,
  RENDER_DYNAMIC_STRUCTS,
  RENDER_DYNAMIC_ROOF,
  RENDER_DYNAMIC_HIGHMERCS,
  RENDER_DYNAMIC_ONROOF,
  RENDER_DYNAMIC_TOPMOST,
  NUM_RENDER_FX_TYPES
};

#define NUM_ITEM_CYCLE_COLORS 20

static uint16_t us16BPPItemCycleWhiteColors[NUM_ITEM_CYCLE_COLORS];
static uint16_t us16BPPItemCycleRedColors[NUM_ITEM_CYCLE_COLORS];
static uint16_t us16BPPItemCycleYellowColors[NUM_ITEM_CYCLE_COLORS];

static int16_t gusNormalItemOutlineColor;
static int16_t gusYellowItemOutlineColor;

int16_t gsRenderHeight = 0;
BOOLEAN gfRenderFullThisFrame = 0;

uint8_t gubCurScrollSpeedID = 1;
BOOLEAN gfDoVideoScroll = TRUE;
BOOLEAN gfScrollPending = FALSE;

static RenderLayerFlags uiLayerUsedFlags = TILES_LAYER_ALL;
static RenderLayerFlags uiAdditiveLayerUsedFlags = TILES_LAYER_ALL;

static const uint8_t gsGlowFrames[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                       0, 0, 0, 0, 0, 2, 4, 6, 8, 9, 8, 6, 4, 2, 0};

const int16_t gsVIEWPORT_START_X = 0;
const int16_t gsVIEWPORT_START_Y = 0;
const int16_t gsVIEWPORT_END_X = SCREEN_WIDTH;
const int16_t gsVIEWPORT_END_Y = SCREEN_HEIGHT - 120;
int16_t gsVIEWPORT_WINDOW_START_Y = 0;
int16_t gsVIEWPORT_WINDOW_END_Y = SCREEN_HEIGHT - 120;

int16_t gsTopLeftWorldX;
int16_t gsTopLeftWorldY;
int16_t gsBottomRightWorldX;
int16_t gsBottomRightWorldY;
BOOLEAN gfIgnoreScrolling = FALSE;

BOOLEAN gfIgnoreScrollDueToCenterAdjust = FALSE;

// GLOBAL SCROLLING PARAMS
int16_t gCenterWorldX;
int16_t gCenterWorldY;
int16_t gsTLX;
int16_t gsTLY;
int16_t gsTRX;
int16_t gsTRY;
int16_t gsBLX;
int16_t gsBLY;
int16_t gsBRX;
int16_t gsBRY;
int16_t gsCX;
int16_t gsCY;
double gdScaleX;
double gdScaleY;

#define FASTMAPROWCOLTOPOS(r, c) ((r) * WORLD_COLS + (c))

bool g_scroll_inertia = false;

// GLOBALS FOR CALCULATING STARTING PARAMETERS
static int16_t gsStartPointX_W;
static int16_t gsStartPointY_W;
static int16_t gsStartPointX_S;
static int16_t gsStartPointY_S;
static int16_t gsStartPointX_M;
static int16_t gsStartPointY_M;
static int16_t gsEndXS;
static int16_t gsEndYS;
// LARGER OFFSET VERSION FOR GIVEN LAYERS
static int16_t gsLStartPointX_W;
static int16_t gsLStartPointY_W;
static int16_t gsLStartPointX_S;
static int16_t gsLStartPointY_S;
static int16_t gsLStartPointX_M;
static int16_t gsLStartPointY_M;
static int16_t gsLEndXS;
static int16_t gsLEndYS;

int16_t gsScrollXIncrement;
int16_t gsScrollYIncrement;

// Rendering flags (full, partial, etc.)
static RenderFlags gRenderFlags = RENDER_FLAG_NONE;

static SGPRect gClippingRect = {0, 0, SCREEN_WIDTH, 360};
static SGPRect gOldClipRect;
int16_t gsRenderCenterX;
int16_t gsRenderCenterY;
int16_t gsRenderWorldOffsetX = 0;
int16_t gsRenderWorldOffsetY = 10;

struct RenderFXType {
  BOOLEAN fDynamic;
  BOOLEAN fZWrite;
  BOOLEAN fZBlitter;
  BOOLEAN fShadowBlitter;
  BOOLEAN fLinkedListDirection;
  BOOLEAN fMerc;
  BOOLEAN fCheckForRedundency;
  BOOLEAN fObscured;
};

static const RenderFXType RenderFX[] = {
    {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE},  // STATIC LAND
    {FALSE, TRUE, TRUE, FALSE, TRUE, FALSE, TRUE, FALSE},     // STATIC OBJECTS
    {FALSE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE},     // STATIC SHADOWS
    {FALSE, TRUE, TRUE, FALSE, TRUE, FALSE, FALSE, TRUE},     // STATIC STRUCTS
    {FALSE, TRUE, TRUE, FALSE, TRUE, FALSE, FALSE, FALSE},    // STATIC ROOF
    {FALSE, TRUE, TRUE, FALSE, TRUE, FALSE, FALSE, TRUE},     // STATIC ONROOF
    {FALSE, TRUE, TRUE, FALSE, TRUE, FALSE, FALSE, FALSE},    // STATIC TOPMOST
    {TRUE, FALSE, TRUE, FALSE, FALSE, FALSE, TRUE, FALSE},    // DYNAMIC LAND
    {TRUE, FALSE, TRUE, FALSE, TRUE, FALSE, TRUE, FALSE},     // DYNAMIC OBJECT
    {TRUE, FALSE, FALSE, TRUE, TRUE, FALSE, FALSE, FALSE},    // DYNAMIC SHADOW
    {TRUE, FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, FALSE},     // DYNAMIC STRUCT MERCS
    {TRUE, FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, FALSE},     // DYNAMIC MERCS
    {TRUE, FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, FALSE},    // DYNAMIC STRUCT
    {TRUE, FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, FALSE},    // DYNAMIC ROOF
    {TRUE, FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, FALSE},     // DYNAMIC HIGHMERCS
    {TRUE, FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, FALSE},    // DYNAMIC ONROOF
    {TRUE, FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, FALSE}     // DYNAMIC TOPMOST
};

static const uint8_t RenderFXStartIndex[] = {
    LAND_START_INDEX,     // STATIC LAND
    OBJECT_START_INDEX,   // STATIC OBJECTS
    SHADOW_START_INDEX,   // STATIC SHADOWS
    STRUCT_START_INDEX,   // STATIC STRUCTS
    ROOF_START_INDEX,     // STATIC ROOF
    ONROOF_START_INDEX,   // STATIC ONROOF
    TOPMOST_START_INDEX,  // STATIC TOPMOST
    LAND_START_INDEX,     // DYNAMIC LAND
    OBJECT_START_INDEX,   // DYNAMIC OBJECT
    SHADOW_START_INDEX,   // DYNAMIC SHADOW
    MERC_START_INDEX,     // DYNAMIC STRUCT MERCS
    MERC_START_INDEX,     // DYNAMIC MERCS
    STRUCT_START_INDEX,   // DYNAMIC STRUCT
    ROOF_START_INDEX,     // DYNAMIC ROOF
    MERC_START_INDEX,     // DYNAMIC HIGHMERCS
    ONROOF_START_INDEX,   // DYNAMIC ONROOF
    TOPMOST_START_INDEX,  // DYNAMIC TOPMOST
};

static RenderLayerFlags const g_render_fx_layer_flags[] = {
    TILES_STATIC_LAND,        TILES_STATIC_OBJECTS,       TILES_STATIC_SHADOWS,
    TILES_STATIC_STRUCTURES,  TILES_STATIC_ROOF,          TILES_STATIC_ONROOF,
    TILES_STATIC_TOPMOST,     TILES_DYNAMIC_LAND,         TILES_DYNAMIC_OBJECTS,
    TILES_DYNAMIC_SHADOWS,    TILES_DYNAMIC_STRUCT_MERCS, TILES_DYNAMIC_MERCS,
    TILES_DYNAMIC_STRUCTURES, TILES_DYNAMIC_ROOF,         TILES_DYNAMIC_HIGHMERCS,
    TILES_DYNAMIC_ONROOF,     TILES_DYNAMIC_TOPMOST};

static void ResetLayerOptimizing() {
  uiLayerUsedFlags = TILES_LAYER_ALL;
  uiAdditiveLayerUsedFlags = TILES_LAYER_NONE;
}

void ResetSpecificLayerOptimizing(RenderLayerFlags const uiRowFlag) {
  uiLayerUsedFlags |= uiRowFlag;
}

static void SumAdditiveLayerOptimization() { uiLayerUsedFlags = uiAdditiveLayerUsedFlags; }

void SetRenderFlags(RenderFlags const uiFlags) { gRenderFlags |= uiFlags; }

void ClearRenderFlags(RenderFlags const uiFlags) { gRenderFlags &= ~uiFlags; }

void RenderSetShadows(BOOLEAN fShadows) {
  if (fShadows) {
    gRenderFlags |= RENDER_FLAG_SHADOWS;
  } else {
    gRenderFlags &= ~RENDER_FLAG_SHADOWS;
  }
}

static inline int16_t GetMapXYWorldY(int32_t WorldCellX, int32_t WorldCellY) {
  int16_t RDistToCenterX = WorldCellX * CELL_X_SIZE - gCenterWorldX;
  int16_t RDistToCenterY = WorldCellY * CELL_Y_SIZE - gCenterWorldY;
  int16_t RScreenCenterY = RDistToCenterX + RDistToCenterY;
  return RScreenCenterY + gsCY - gsTLY;
}

static void Blt8BPPDataTo16BPPBufferTransZIncClip(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                                  uint16_t *pZBuffer, uint16_t usZValue,
                                                  HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                                  uint16_t usIndex, SGPRect *clipregion);
static void Blt8BPPDataTo16BPPBufferTransZIncClipZSameZBurnsThrough(
    uint16_t *pBuffer, uint32_t uiDestPitchBYTES, uint16_t *pZBuffer, uint16_t usZValue,
    HVOBJECT hSrcVObject, int32_t iX, int32_t iY, uint16_t usIndex, SGPRect *clipregion);
static void Blt8BPPDataTo16BPPBufferTransZIncObscureClip(
    uint16_t *pBuffer, uint32_t uiDestPitchBYTES, uint16_t *pZBuffer, uint16_t usZValue,
    HVOBJECT hSrcVObject, int32_t iX, int32_t iY, uint16_t usIndex, SGPRect *clipregion);
static void Blt8BPPDataTo16BPPBufferTransZTransShadowIncClip(
    uint16_t *pBuffer, uint32_t uiDestPitchBYTES, uint16_t *pZBuffer, uint16_t usZValue,
    HVOBJECT hSrcVObject, int32_t iX, int32_t iY, uint16_t usIndex, SGPRect *clipregion,
    int16_t sZIndex, const uint16_t *p16BPPPalette);
static void Blt8BPPDataTo16BPPBufferTransZTransShadowIncObscureClip(
    uint16_t *pBuffer, uint32_t uiDestPitchBYTES, uint16_t *pZBuffer, uint16_t usZValue,
    HVOBJECT hSrcVObject, int32_t iX, int32_t iY, uint16_t usIndex, SGPRect *clipregion,
    int16_t sZIndex, const uint16_t *p16BPPPalette);

static void RenderTiles(RenderTilesFlags const uiFlags, int32_t const iStartPointX_M,
                        int32_t const iStartPointY_M, int32_t const iStartPointX_S,
                        int32_t const iStartPointY_S, int32_t const iEndXS, int32_t const iEndYS,
                        uint8_t const ubNumLevels, RenderLayerID const *const psLevelIDs) {
  static uint8_t ubLevelNodeStartIndex[NUM_RENDER_FX_TYPES];
  static RenderFXType RenderFXList[NUM_RENDER_FX_TYPES];

  HVOBJECT hVObject = NULL;  // XXX HACK000E
  BOOLEAN fPixelate = FALSE;
  int16_t sMultiTransShadowZBlitterIndex = -1;

  int16_t sZOffsetX = -1;
  int16_t sZOffsetY = -1;
  const ROTTING_CORPSE *pCorpse = NULL;
  uint32_t uiTileElemFlags = 0;

  uint16_t usImageIndex = 0;
  int16_t sZLevel = 0;
  BackgroundFlags uiDirtyFlags = BGND_FLAG_NONE;
  uint16_t const *pShadeTable = 0;

  int32_t iAnchorPosX_M = iStartPointX_M;
  int32_t iAnchorPosY_M = iStartPointY_M;
  int32_t iAnchorPosX_S = iStartPointX_S;
  int32_t iAnchorPosY_S = iStartPointY_S;

  uint32_t uiDestPitchBYTES = 0;
  uint16_t *pDestBuf = 0;
  SGPVSurface::Lockable lock;
  if (!(uiFlags & TILES_DIRTY)) {
    lock.Lock(FRAME_BUFFER);
    pDestBuf = lock.Buffer<uint16_t>();
    uiDestPitchBYTES = lock.Pitch();
  }

  bool check_for_mouse_detections = false;
  if (uiFlags & TILES_DYNAMIC_CHECKFOR_INT_TILE && ShouldCheckForMouseDetections()) {
    BeginCurInteractiveTileCheck();
    // If we are in edit mode, don't do this
    check_for_mouse_detections = !gfEditMode;
  }

  for (uint32_t i = 0; i < ubNumLevels; i++) {
    ubLevelNodeStartIndex[i] = RenderFXStartIndex[psLevelIDs[i]];
    RenderFXList[i] = RenderFX[psLevelIDs[i]];
  }

  int8_t bXOddFlag = 0;
  do {
    static int32_t iTileMapPos[500];

    {
      int32_t iTempPosX_M = iAnchorPosX_M;
      int32_t iTempPosY_M = iAnchorPosY_M;
      int32_t iTempPosX_S = iAnchorPosX_S;
      uint32_t uiMapPosIndex = 0;

      // Build tile index list
      do {
        iTileMapPos[uiMapPosIndex] = FASTMAPROWCOLTOPOS(iTempPosY_M, iTempPosX_M);

        iTempPosX_S += 40;
        iTempPosX_M++;
        iTempPosY_M--;

        uiMapPosIndex++;
      } while (iTempPosX_S < iEndXS);
    }

    for (uint32_t cnt = 0; cnt < ubNumLevels; cnt++) {
      RenderLayerFlags const uiRowFlags = g_render_fx_layer_flags[psLevelIDs[cnt]];

      if (uiRowFlags & TILES_ALL_DYNAMICS && !(uiLayerUsedFlags & uiRowFlags) &&
          !(uiFlags & TILES_DYNAMIC_CHECKFOR_INT_TILE))
        continue;

      int32_t iTempPosX_M = iAnchorPosX_M;
      int32_t iTempPosY_M = iAnchorPosY_M;
      int32_t iTempPosX_S = iAnchorPosX_S;
      int32_t iTempPosY_S = iAnchorPosY_S;
      uint32_t uiMapPosIndex = 0;

      if (bXOddFlag) iTempPosX_S += 20;

      do {
        const uint32_t uiTileIndex = iTileMapPos[uiMapPosIndex];
        uiMapPosIndex++;

        if (uiTileIndex < GRIDSIZE) {
          MAP_ELEMENT const &me = gpWorldLevelData[uiTileIndex];

          /* OK, we're searching through this loop anyway, might as well check
           * for mouse position over objects. Experimental! */
          if (check_for_mouse_detections && me.pStructHead) {
            LogMouseOverInteractiveTile(uiTileIndex);
          }

          if (uiFlags & TILES_MARKED && !(me.uiFlags & MAPELEMENT_REDRAW)) goto next_tile;

          int8_t n_visible_items = 0;
          ITEM_POOL const *item_pool = 0;
          for (LEVELNODE *pNode = me.pLevelNodes[ubLevelNodeStartIndex[cnt]]; pNode;) {
            const RenderFXType RenderingFX = RenderFXList[cnt];
            const BOOLEAN fObscured = RenderingFX.fObscured;
            const BOOLEAN fDynamic = RenderingFX.fDynamic;
            BOOLEAN fMerc = RenderingFX.fMerc;
            BOOLEAN fZWrite = RenderingFX.fZWrite;
            BOOLEAN fZBlitter = RenderingFX.fZBlitter;
            BOOLEAN fShadowBlitter = RenderingFX.fShadowBlitter;
            const BOOLEAN fLinkedListDirection = RenderingFX.fLinkedListDirection;
            const BOOLEAN fCheckForRedundency = RenderingFX.fCheckForRedundency;

            BOOLEAN fMultiZBlitter = FALSE;
            BOOLEAN fIntensityBlitter = FALSE;
            BOOLEAN fSaveZ = FALSE;
            BOOLEAN fWallTile = FALSE;
            BOOLEAN fMultiTransShadowZBlitter = FALSE;
            BOOLEAN fObscuredBlitter = FALSE;
            uint32_t uiAniTileFlags = 0;
            int16_t gsForceSoldierZLevel = 0;

            const uint32_t uiLevelNodeFlags = pNode->uiFlags;

            if (fCheckForRedundency && me.uiFlags & MAPELEMENT_REDUNDENT &&
                !(me.uiFlags & MAPELEMENT_REEVALUATE_REDUNDENCY) &&  // If we donot want to
                                                                     // re-evaluate first
                !(gTacticalStatus.uiFlags & NOHIDE_REDUNDENCY)) {
              break;
            }

            // Force z-buffer blitting for marked tiles (even ground!)
            if (uiFlags & TILES_MARKED) fZBlitter = TRUE;

            // Looking up height every time here is alot better than doing it
            // above!
            int16_t const sTileHeight = me.sHeight;

            int16_t sModifiedTileHeight = (sTileHeight / 80 - 1) * 80;
            if (sModifiedTileHeight < 0) sModifiedTileHeight = 0;

            BOOLEAN fRenderTile = TRUE;
            if (!(uiLevelNodeFlags & LEVELNODE_REVEAL)) {
              fPixelate = FALSE;
            } else if (fDynamic) {
              fPixelate = TRUE;
            } else {
              fRenderTile = FALSE;
            }

            // non-type specific setup
            int16_t sXPos = iTempPosX_S;
            int16_t sYPos = iTempPosY_S;

            TILE_ELEMENT const *TileElem = 0;
            // setup for any tile type except mercs
            if (!fMerc) {
              if (uiLevelNodeFlags & (LEVELNODE_ROTTINGCORPSE | LEVELNODE_CACHEDANITILE)) {
                if (fDynamic) {
                  if (!(uiLevelNodeFlags & LEVELNODE_DYNAMIC) &&
                      !(uiLevelNodeFlags & LEVELNODE_LASTDYNAMIC)) {
                    fRenderTile = FALSE;
                  }
                } else if (uiLevelNodeFlags & LEVELNODE_DYNAMIC) {
                  fRenderTile = FALSE;
                }
              } else {
                TileElem = uiLevelNodeFlags & LEVELNODE_REVEALTREES
                               ? &gTileDatabase[pNode->usIndex + 2]
                               : &gTileDatabase[pNode->usIndex];

                // Handle independent-per-tile animations (i.e.: doors,
                // exploding things, etc.)
                if (fDynamic && uiLevelNodeFlags & LEVELNODE_ANIMATION &&
                    pNode->sCurrentFrame != -1) {
                  Assert(TileElem->pAnimData);
                  TileElem = &gTileDatabase[TileElem->pAnimData->pusFrames[pNode->sCurrentFrame]];
                }

                // Set Tile elem flags here!
                uiTileElemFlags = TileElem->uiFlags;

                if (!fPixelate) {
                  if (fDynamic) {
                    if (!(uiLevelNodeFlags & LEVELNODE_DYNAMIC) &&
                        !(uiLevelNodeFlags & LEVELNODE_LASTDYNAMIC) &&
                        !(uiTileElemFlags & DYNAMIC_TILE)) {
                      if (uiTileElemFlags & ANIMATED_TILE) {
                        Assert(TileElem->pAnimData);
                        TileElem =
                            &gTileDatabase[TileElem->pAnimData
                                               ->pusFrames[TileElem->pAnimData->bCurrentFrame]];
                        uiTileElemFlags = TileElem->uiFlags;
                      } else {
                        fRenderTile = FALSE;
                      }
                    }
                  } else {
                    if (uiTileElemFlags & ANIMATED_TILE ||
                        ((uiTileElemFlags & DYNAMIC_TILE || uiLevelNodeFlags & LEVELNODE_DYNAMIC) &&
                         !(uiFlags & TILES_OBSCURED))) {
                      fRenderTile = FALSE;
                    }
                  }
                }
              }

              // OK, ATE, CHECK FOR AN OBSCURED TILE AND MAKE SURE IF LEVELNODE
              // IS SET WE DON'T RENDER UNLESS WE HAVE THE RENDER FLAG SET!
              if (fObscured) {
                if (uiFlags & TILES_OBSCURED) {
                  if (uiLevelNodeFlags & LEVELNODE_SHOW_THROUGH) {
                    fObscuredBlitter = TRUE;
                  } else {
                    // Do not render if we are not on this render loop!
                    fRenderTile = FALSE;
                  }
                } else {
                  if (uiLevelNodeFlags & LEVELNODE_SHOW_THROUGH) {
                    fRenderTile = FALSE;
                  }
                }
              }

              if (fRenderTile) {
                // Set flag to set layer as used
                if (fDynamic || fPixelate) {
                  uiAdditiveLayerUsedFlags |= uiRowFlags;
                }

                if (uiLevelNodeFlags & LEVELNODE_DYNAMICZ) {
                  fSaveZ = TRUE;
                  fZWrite = TRUE;
                }

                if (uiLevelNodeFlags & LEVELNODE_CACHEDANITILE) {
                  ANITILE const &a = *pNode->pAniTile;
                  hVObject = gpTileCache[a.sCachedTileID].pImagery->vo;
                  usImageIndex = a.sCurrentFrame;
                  uiAniTileFlags = a.uiFlags;

                  float dOffsetX;
                  float dOffsetY;
                  // Position corpse based on it's float position
                  if (uiLevelNodeFlags & LEVELNODE_ROTTINGCORPSE) {
                    pCorpse = ID2CORPSE(a.v.user.uiData);
                    pShadeTable = pCorpse->pShades[pNode->ubShadeLevel];

                    // OK, if this is a corpse.... stop if not visible
                    if (pCorpse->def.bVisible != 1 && !(gTacticalStatus.uiFlags & SHOW_ALL_MERCS))
                      goto next_prev_node;

                    int16_t x;
                    int16_t y;
                    ConvertGridNoToCenterCellXY(pCorpse->def.sGridNo, &x, &y);
                    dOffsetX = x - gsRenderCenterX;
                    dOffsetY = y - gsRenderCenterY;
                  } else {
                    dOffsetX = a.sRelativeX - gsRenderCenterX;
                    dOffsetY = a.sRelativeY - gsRenderCenterY;
                  }

                  // Calculate guy's position
                  float dTempX_S;
                  float dTempY_S;
                  FloatFromCellToScreenCoordinates(dOffsetX, dOffsetY, &dTempX_S, &dTempY_S);

                  sXPos = (gsVIEWPORT_END_X - gsVIEWPORT_START_X) / 2 + (int16_t)dTempX_S;
                  sYPos =
                      (gsVIEWPORT_END_Y - gsVIEWPORT_START_Y) / 2 + (int16_t)dTempY_S - sTileHeight;

                  // Adjust for offset position on screen
                  sXPos -= gsRenderWorldOffsetX;
                  sYPos -= gsRenderWorldOffsetY;
                } else {
                  hVObject = TileElem->hTileSurface;
                  usImageIndex = TileElem->usRegionIndex;

                  if (TileElem->uiFlags & IGNORE_WORLD_HEIGHT) {
                    sYPos -= sModifiedTileHeight;
                  } else if (!(uiLevelNodeFlags & LEVELNODE_IGNOREHEIGHT)) {
                    sYPos -= sTileHeight;
                  }

                  if (!(uiFlags & TILES_DIRTY)) {
                    hVObject->CurrentShade(pNode->ubShadeLevel);
                  }
                }

                // ADJUST FOR RELATIVE OFFSETS
                if (uiLevelNodeFlags & LEVELNODE_USERELPOS) {
                  sXPos += pNode->sRelativeX;
                  sYPos += pNode->sRelativeY;
                }

                if (uiLevelNodeFlags & LEVELNODE_USEZ) {
                  sYPos -= pNode->sRelativeZ;
                }

                // ADJUST FOR ABSOLUTE POSITIONING
                if (uiLevelNodeFlags & LEVELNODE_USEABSOLUTEPOS) {
                  float dOffsetX = pNode->sRelativeX - gsRenderCenterX;
                  float dOffsetY = pNode->sRelativeY - gsRenderCenterY;

                  // OK, DONT'T ASK... CONVERSION TO PROPER Y NEEDS THIS...
                  dOffsetX -= CELL_Y_SIZE;

                  float dTempX_S;
                  float dTempY_S;
                  FloatFromCellToScreenCoordinates(dOffsetX, dOffsetY, &dTempX_S, &dTempY_S);

                  sXPos = (gsVIEWPORT_END_X - gsVIEWPORT_START_X) / 2 + (int16_t)dTempX_S;
                  sYPos = (gsVIEWPORT_END_Y - gsVIEWPORT_START_Y) / 2 + (int16_t)dTempY_S;

                  // Adjust for offset position on screen
                  sXPos -= gsRenderWorldOffsetX;
                  sYPos -= gsRenderWorldOffsetY;

                  sYPos -= pNode->sRelativeZ;
                }
              }

              // COUNT # OF ITEMS AT THIS LOCATION
              if (uiLevelNodeFlags & LEVELNODE_ITEM) {
                // Set item pool for this location.
                item_pool = item_pool ? item_pool->pNext : pNode->pItemPool;
                WORLDITEM const &wi = GetWorldItem(item_pool->iItemIndex);

                /* Limit rendering of items to MAX_RENDERED_ITEMS. Do not render
                 * hidden items either. */
                if (wi.bVisible != VISIBLE || wi.usFlags & WORLD_ITEM_DONTRENDER ||
                    n_visible_items == MAX_RENDERED_ITEMS) {
                  if (!(gTacticalStatus.uiFlags & SHOW_ALL_ITEMS)) goto next_prev_node;
                }
                ++n_visible_items;

                if (wi.bRenderZHeightAboveLevel > 0) {
                  sYPos -= wi.bRenderZHeightAboveLevel;
                }
              }

              // If render tile is false...
              if (!fRenderTile) goto next_prev_node;
            }

            // specific code for node types on a per-tile basis
            switch (uiRowFlags) {
              case TILES_STATIC_LAND:
                goto zlevel_land;

              case TILES_STATIC_OBJECTS:
                // ATE: Modified to use constant z level, as these are same level
                // as land items
                goto zlevel_objects;

              case TILES_STATIC_SHADOWS:
                if (uiLevelNodeFlags & LEVELNODE_EXITGRID) {
                  fIntensityBlitter = TRUE;
                  fShadowBlitter = FALSE;
                }
                goto zlevel_shadows;

              case TILES_STATIC_STRUCTURES:
                if (TileElem) {
                  if (TileElem->uiFlags & MULTI_Z_TILE) fMultiZBlitter = TRUE;
                  if (TileElem->uiFlags & WALL_TILE) fWallTile = TRUE;
                }
                goto zlevel_structures;

              case TILES_STATIC_ROOF:
                // ATE: Added for shadows on roofs
                if (TileElem && TileElem->uiFlags & ROOFSHADOW_TILE) {
                  fShadowBlitter = TRUE;
                }
                goto zlevel_roof;

              case TILES_STATIC_ONROOF:
                goto zlevel_onroof;

              case TILES_STATIC_TOPMOST:
                goto zlevel_topmost;

              case TILES_DYNAMIC_LAND:
                uiDirtyFlags = BGND_FLAG_SINGLE | BGND_FLAG_ANIMATED;
              zlevel_land:
                sZLevel = LAND_Z_LEVEL;
                break;

              case TILES_DYNAMIC_OBJECTS:
                uiDirtyFlags = BGND_FLAG_SINGLE | BGND_FLAG_ANIMATED;
              zlevel_objects:
                if (uiTileElemFlags & CLIFFHANG_TILE) {
                  sZLevel = LAND_Z_LEVEL;
                } else if (uiTileElemFlags & OBJECTLAYER_USEZHEIGHT) {
                  int16_t const world_y = GetMapXYWorldY(iTempPosX_M, iTempPosY_M);
                  sZLevel = (world_y * Z_SUBLAYERS) + LAND_Z_LEVEL;
                } else {
                  sZLevel = OBJECT_Z_LEVEL;
                }
                break;

              case TILES_DYNAMIC_SHADOWS: {
                uiDirtyFlags = BGND_FLAG_SINGLE | BGND_FLAG_ANIMATED;
              zlevel_shadows:
                int16_t const world_y = GetMapXYWorldY(iTempPosX_M, iTempPosY_M);
                sZLevel = std::max(((world_y - 80) * Z_SUBLAYERS) + SHADOW_Z_LEVEL, 0);
                break;
              }

              case TILES_DYNAMIC_STRUCTURES: {
                uiDirtyFlags = BGND_FLAG_SINGLE | BGND_FLAG_ANIMATED;
              zlevel_structures:
                int16_t world_y = GetMapXYWorldY(iTempPosX_M, iTempPosY_M);
                if (uiLevelNodeFlags & LEVELNODE_ROTTINGCORPSE) {
                  if (pCorpse->def.usFlags & ROTTING_CORPSE_VEHICLE) {
                    if (pNode->pStructureData) {
                      DB_STRUCTURE const &dbs =
                          *pNode->pStructureData->pDBStructureRef->pDBStructure;
                      sZOffsetX = dbs.bZTileOffsetX;
                      sZOffsetY = dbs.bZTileOffsetY;
                    }
                    world_y = GetMapXYWorldY(iTempPosX_M + sZOffsetX, iTempPosY_M + sZOffsetY);
                    sZLevel = STRUCT_Z_LEVEL;
                  } else {
                    sZOffsetX = -1;
                    sZOffsetY = -1;
                    world_y = GetMapXYWorldY(iTempPosX_M + sZOffsetX, iTempPosY_M + sZOffsetY);
                    world_y += 20;
                    sZLevel = LAND_Z_LEVEL;
                  }
                } else if (uiLevelNodeFlags & LEVELNODE_PHYSICSOBJECT) {
                  world_y += pNode->sRelativeZ;
                  sZLevel = ONROOF_Z_LEVEL;
                } else if (uiLevelNodeFlags & LEVELNODE_ITEM) {
                  WORLDITEM const &wi = GetWorldItem(pNode->pItemPool->iItemIndex);
                  if (wi.bRenderZHeightAboveLevel > 0) {
                    sZLevel = STRUCT_Z_LEVEL + wi.bRenderZHeightAboveLevel;
                  } else {
                    sZLevel = OBJECT_Z_LEVEL;
                  }
                } else if (uiAniTileFlags & ANITILE_SMOKE_EFFECT) {
                  sZLevel = OBJECT_Z_LEVEL;
                } else if (uiLevelNodeFlags & LEVELNODE_USEZ) {
                  if (uiLevelNodeFlags & LEVELNODE_NOZBLITTER) {
                    world_y += 40;
                  } else {
                    world_y += pNode->sRelativeZ;
                  }
                  sZLevel = ONROOF_Z_LEVEL;
                } else {
                  sZLevel = STRUCT_Z_LEVEL;
                }
                sZLevel += world_y * Z_SUBLAYERS;
                break;
              }

              case TILES_DYNAMIC_ROOF: {
                uiDirtyFlags = BGND_FLAG_SINGLE | BGND_FLAG_ANIMATED;
              zlevel_roof:
                // Automatically adjust height.
                sYPos -= WALL_HEIGHT;

                int16_t const world_y = WALL_HEIGHT + GetMapXYWorldY(iTempPosX_M, iTempPosY_M);
                sZLevel = (world_y * Z_SUBLAYERS) + ROOF_Z_LEVEL;
                break;
              }

              case TILES_DYNAMIC_ONROOF: {
                uiDirtyFlags = BGND_FLAG_SINGLE | BGND_FLAG_ANIMATED;
              zlevel_onroof:
                // Automatically adjust height.
                sYPos -= WALL_HEIGHT;

                int16_t world_y = GetMapXYWorldY(iTempPosX_M, iTempPosY_M);
                if (uiLevelNodeFlags & LEVELNODE_ROTTINGCORPSE) {
                  world_y += WALL_HEIGHT + 40;
                }
                if (uiLevelNodeFlags & LEVELNODE_ROTTINGCORPSE) {  // XXX duplicate?
                  world_y += WALL_HEIGHT + 40;
                } else {
                  world_y += WALL_HEIGHT;
                }
                sZLevel = (world_y * Z_SUBLAYERS) + ONROOF_Z_LEVEL;
                break;
              }

              case TILES_DYNAMIC_TOPMOST:
                uiDirtyFlags = BGND_FLAG_SINGLE | BGND_FLAG_ANIMATED;
              zlevel_topmost:
                sZLevel = TOPMOST_Z_LEVEL;
                break;

              case TILES_DYNAMIC_MERCS:
              case TILES_DYNAMIC_HIGHMERCS:
              case TILES_DYNAMIC_STRUCT_MERCS: {
                // Set flag to set layer as used
                uiAdditiveLayerUsedFlags |= uiRowFlags;

                SOLDIERTYPE const &s = *pNode->pSoldier;
                switch (uiRowFlags) {
                  case TILES_DYNAMIC_MERCS:
                    // If we are multi-tiled, ignore here
                    if (s.uiStatusFlags & SOLDIER_MULTITILE) goto next_node;
                    // If we are at a higher level, no not do anything unless we are
                    // at the highmerc stage
                    if (s.bLevel > 0) goto next_node;
                    break;

                  case TILES_DYNAMIC_HIGHMERCS:
                    // If we are multi-tiled, ignore here
                    if (s.uiStatusFlags & SOLDIER_MULTITILE) goto next_node;
                    // If we are at a lower level, no not do anything unless we are
                    // at the highmerc stage
                    if (s.bLevel == 0) goto next_node;
                    break;

                  case TILES_DYNAMIC_STRUCT_MERCS:
                    // If we are not multi-tiled, ignore here
                    if (!(s.uiStatusFlags & SOLDIER_MULTITILE)) {
                      // If we are at a low level, no not do anything unless we are
                      // at the merc stage
                      if (s.bLevel == 0) goto next_node;
                    } else {
                      fSaveZ = TRUE;
                      fMultiTransShadowZBlitter = TRUE;
                      fZBlitter = TRUE;

                      // ATE: Use one direction for queen!
                      sMultiTransShadowZBlitterIndex =
                          s.ubBodyType == QUEENMONSTER ? 0 : OneCDirection(s.bDirection);
                    }
                    break;
                  default:
                    break;
                }

                // IF we are not active, or are a placeholder for multi-tile
                // animations do nothing
                if (!s.bActive || uiLevelNodeFlags & LEVELNODE_MERCPLACEHOLDER) goto next_node;

                // Skip if we cannot see the guy!
                if (s.bLastRenderVisibleValue == -1 && !(gTacticalStatus.uiFlags & SHOW_ALL_MERCS))
                  goto next_node;

                // Get animation surface....
                uint16_t const usAnimSurface = GetSoldierAnimationSurface(&s);
                if (usAnimSurface == INVALID_ANIMATION_SURFACE) goto next_node;

                // Shade guy always lighter than sceane default!
                uint8_t ubShadeLevel;
                if (s.fBeginFade) {
                  ubShadeLevel = s.ubFadeLevel;
                } else {
                  ubShadeLevel = pNode->ubShadeLevel & 0x0f;
                  ubShadeLevel = std::max(ubShadeLevel - 2, DEFAULT_SHADE_LEVEL);
                  ubShadeLevel |= pNode->ubShadeLevel & 0x30;
                }
                pShadeTable = s.pShades[ubShadeLevel];

                // Position guy based on guy's position
                float const dOffsetX = s.dXPos - gsRenderCenterX;
                float const dOffsetY = s.dYPos - gsRenderCenterY;

                // Calculate guy's position
                float dTempX_S;
                float dTempY_S;
                FloatFromCellToScreenCoordinates(dOffsetX, dOffsetY, &dTempX_S, &dTempY_S);

                sXPos = (gsVIEWPORT_END_X - gsVIEWPORT_START_X) / 2 + (int16_t)dTempX_S;
                sYPos =
                    (gsVIEWPORT_END_Y - gsVIEWPORT_START_Y) / 2 + (int16_t)dTempY_S - sTileHeight;

                // Adjust for offset position on screen
                sXPos -= gsRenderWorldOffsetX;
                sYPos -= gsRenderWorldOffsetY;

                // Adjust for soldier height
                sYPos -= s.sHeightAdjustment;

                // Handle shade stuff....
                if (!s.fBeginFade) {
                  // Special effect - draw ghost if is seen by a guy in player's
                  // team but not current guy ATE: Todo: setup flag for 'bad-guy'
                  // - can releive some checks in renderer
                  if (!s.bNeutral && s.bSide != OUR_TEAM) {
                    int8_t bGlowShadeOffset = 0;

                    if (gTacticalStatus.ubCurrentTeam == OUR_TEAM) {
                      // Shade differently depending on visiblity
                      if (s.bLastRenderVisibleValue == 0) {
                        bGlowShadeOffset = 10;
                      }

                      const SOLDIERTYPE *const sel = GetSelectedMan();
                      if (sel && sel->bOppList[s.ubID] != SEEN_CURRENTLY &&
                          s.usAnimState != CHARIOTS_OF_FIRE && s.usAnimState != BODYEXPLODING) {
                        bGlowShadeOffset = 10;
                      }
                    }

                    uint16_t *const *pShadeStart =
                        s.bLevel == 0 ? &s.pGlowShades[0] : &s.pShades[20];

                    // Set shade
                    // If a bad guy is highlighted
                    if (gSelectedGuy != NULL && gSelectedGuy->bSide != OUR_TEAM) {
                      if (gSelectedGuy == &s) {
                        pShadeTable =
                            pShadeStart[gsGlowFrames[gsCurrentGlowFrame] + bGlowShadeOffset];
                        gsForceSoldierZLevel = TOPMOST_Z_LEVEL;
                      } else {
                        // Are we dealing with a not-so visible merc?
                        if (bGlowShadeOffset == 10) {
                          pShadeTable = s.effect_shade;
                        }
                      }
                    } else {
                      // Not highlighted, but maybe we are in enemy's turn and
                      // they have the baton
                      if (gTacticalStatus.ubCurrentTeam == OUR_TEAM ||
                          s.uiStatusFlags & SOLDIER_UNDERAICONTROL)  // Does he have baton?
                      {
                        pShadeTable =
                            pShadeStart[gsGlowFrames[gsCurrentGlowFrame] + bGlowShadeOffset];
                        if (gsGlowFrames[gsCurrentGlowFrame] >= 7) {
                          gsForceSoldierZLevel = TOPMOST_Z_LEVEL;
                        }
                      }
                    }
                  }
                }

                {  // Calculate Z level
                  int16_t world_y;
                  if (s.uiStatusFlags & SOLDIER_MULTITILE) {
                    if (pNode->pStructureData) {
                      DB_STRUCTURE const &dbs =
                          *pNode->pStructureData->pDBStructureRef->pDBStructure;
                      sZOffsetX = dbs.bZTileOffsetX;
                      sZOffsetY = dbs.bZTileOffsetY;
                    }
                    world_y = GetMapXYWorldY(iTempPosX_M + sZOffsetX, iTempPosY_M + sZOffsetY);
                  } else {
                    world_y = GetMapXYWorldY(iTempPosX_M, iTempPosY_M);
                  }

                  if (s.uiStatusFlags & SOLDIER_VEHICLE) {
                    sZLevel = (world_y * Z_SUBLAYERS) + STRUCT_Z_LEVEL;
                  } else if (gsForceSoldierZLevel != 0) {
                    sZLevel = gsForceSoldierZLevel;
                  } else if (s.sZLevelOverride != -1) {
                    sZLevel = s.sZLevelOverride;
                  } else if (s.dHeightAdjustment > 0) {
                    world_y += WALL_HEIGHT + 20;
                    sZLevel = (world_y * Z_SUBLAYERS) + ONROOF_Z_LEVEL;
                  } else {
                    sZLevel = (world_y * Z_SUBLAYERS) + MERC_Z_LEVEL;
                  }
                }

                if (!(uiFlags & TILES_DIRTY) && s.fForceShade) {
                  pShadeTable = s.pForcedShade;
                }

                hVObject = gAnimSurfaceDatabase[usAnimSurface].hVideoObject;
                if (!hVObject) goto next_node;

                // ATE: If we are in a gridno that we should not use obscure
                // blitter, set!
                if (!(me.ubExtFlags[0] & MAPELEMENT_EXT_NOBURN_STRUCT)) {
                  fObscuredBlitter = TRUE;
                } else {
                  // ATE: Artificially increase z-level...
                  sZLevel += 2;
                }

                usImageIndex = s.usAniFrame;

                uiDirtyFlags = BGND_FLAG_SINGLE | BGND_FLAG_ANIMATED;
                break;
              }
              default:
                break;
            }

            // Adjust for interface level
            sYPos += gsRenderHeight;

            if (!fRenderTile) goto next_prev_node;

            if (uiLevelNodeFlags & LEVELNODE_HIDDEN &&
                /* If it is a roof and SHOW_ALL_ROOFS is on, turn off hidden
                   tile check */
                (!TileElem || !(TileElem->uiFlags & ROOF_TILE) ||
                 !(gTacticalStatus.uiFlags & SHOW_ALL_ROOFS)))
              goto next_prev_node;

            if (uiLevelNodeFlags & LEVELNODE_ROTTINGCORPSE) {
              // Set fmerc flag!
              fMerc = TRUE;
              fZWrite = TRUE;

              sMultiTransShadowZBlitterIndex = GetCorpseStructIndex(&pCorpse->def, TRUE);
              fMultiTransShadowZBlitter = TRUE;
            }

            if (uiLevelNodeFlags & LEVELNODE_LASTDYNAMIC && !(uiFlags & TILES_DIRTY)) {
              // Remove flags!
              pNode->uiFlags &= ~LEVELNODE_LASTDYNAMIC;
              fZWrite = TRUE;
            }

            // RENDER
            if (uiLevelNodeFlags & LEVELNODE_WIREFRAME &&
                !gGameSettings.fOptions[TOPTION_TOGGLE_WIREFRAME]) {
            } else if (uiFlags & TILES_DIRTY) {
              if (!(uiLevelNodeFlags & LEVELNODE_LASTDYNAMIC)) {
                ETRLEObject const &pTrav = hVObject->SubregionProperties(usImageIndex);
                uint32_t const uiBrushHeight = pTrav.usHeight;
                uint32_t const uiBrushWidth = pTrav.usWidth;
                sXPos += pTrav.sOffsetX;
                sYPos += pTrav.sOffsetY;

                int16_t const h =
                    std::min((int16_t)uiBrushHeight, (int16_t)(gsVIEWPORT_WINDOW_END_Y - sYPos));
                RegisterBackgroundRect(uiDirtyFlags, sXPos, sYPos, uiBrushWidth, h);
                if (fSaveZ) {
                  RegisterBackgroundRect(uiDirtyFlags | BGND_FLAG_SAVE_Z, sXPos, sYPos,
                                         uiBrushWidth, h);
                }
              }
            } else if (uiLevelNodeFlags & LEVELNODE_DISPLAY_AP) {
              ETRLEObject const &pTrav = hVObject->SubregionProperties(usImageIndex);
              sXPos += pTrav.sOffsetX;
              sYPos += pTrav.sOffsetY;

              uint8_t const foreground =
                  gfUIDisplayActionPointsBlack ? FONT_MCOLOR_BLACK : FONT_MCOLOR_WHITE;
              SetFontAttributes(TINYFONT1, foreground);
              SetFontDestBuffer(guiSAVEBUFFER, 0, gsVIEWPORT_WINDOW_START_Y, SCREEN_WIDTH,
                                gsVIEWPORT_WINDOW_END_Y);
              wchar_t buf[16];
              swprintf(buf, lengthof(buf), L"%d", pNode->uiAPCost);
              int16_t sX;
              int16_t sY;
              FindFontCenterCoordinates(sXPos, sYPos, 1, 1, buf, TINYFONT1, &sX, &sY);
              MPrintBuffer(pDestBuf, uiDestPitchBYTES, sX, sY, buf);
              SetFontDestBuffer(FRAME_BUFFER);
            } else if (uiLevelNodeFlags & LEVELNODE_ITEM) {
              uint16_t outline_colour;
              bool const on_roof =
                  uiRowFlags == TILES_STATIC_ONROOF || uiRowFlags == TILES_DYNAMIC_ONROOF;
              if (gGameSettings.fOptions[TOPTION_GLOW_ITEMS]) {
                uint16_t const *palette = on_roof ? us16BPPItemCycleYellowColors
                                          : gTacticalStatus.uiFlags & RED_ITEM_GLOW_ON
                                              ? us16BPPItemCycleRedColors
                                              : us16BPPItemCycleWhiteColors;
                outline_colour = palette[gsCurrentItemGlowFrame];
              } else {
                outline_colour = on_roof ? gusYellowItemOutlineColor : gusNormalItemOutlineColor;
              }

              const BOOLEAN bBlitClipVal =
                  BltIsClippedOrOffScreen(hVObject, sXPos, sYPos, usImageIndex, &gClippingRect);
              if (bBlitClipVal == FALSE) {
                if (fObscuredBlitter) {
                  Blt8BPPDataTo16BPPBufferOutlineZPixelateObscured(
                      pDestBuf, uiDestPitchBYTES, gpZBuffer, sZLevel, hVObject, sXPos, sYPos,
                      usImageIndex, outline_colour);
                } else {
                  Blt8BPPDataTo16BPPBufferOutlineZ(pDestBuf, uiDestPitchBYTES, gpZBuffer, sZLevel,
                                                   hVObject, sXPos, sYPos, usImageIndex,
                                                   outline_colour);
                }
              } else if (bBlitClipVal == TRUE) {
                if (fObscuredBlitter) {
                  Blt8BPPDataTo16BPPBufferOutlineZPixelateObscuredClip(
                      pDestBuf, uiDestPitchBYTES, gpZBuffer, sZLevel, hVObject, sXPos, sYPos,
                      usImageIndex, outline_colour, &gClippingRect);
                } else {
                  Blt8BPPDataTo16BPPBufferOutlineZClip(
                      pDestBuf, uiDestPitchBYTES, gpZBuffer, sZLevel, hVObject, sXPos, sYPos,
                      usImageIndex, outline_colour, &gClippingRect);
                }
              }
            }
            // ATE: Check here for a lot of conditions!
            else if (uiLevelNodeFlags & LEVELNODE_PHYSICSOBJECT) {
              const BOOLEAN bBlitClipVal =
                  BltIsClippedOrOffScreen(hVObject, sXPos, sYPos, usImageIndex, &gClippingRect);

              if (fShadowBlitter) {
                if (bBlitClipVal == FALSE) {
                  Blt8BPPDataTo16BPPBufferShadowZNB(pDestBuf, uiDestPitchBYTES, gpZBuffer, sZLevel,
                                                    hVObject, sXPos, sYPos, usImageIndex);
                } else {
                  Blt8BPPDataTo16BPPBufferShadowZNBClip(pDestBuf, uiDestPitchBYTES, gpZBuffer,
                                                        sZLevel, hVObject, sXPos, sYPos,
                                                        usImageIndex, &gClippingRect);
                }
              } else {
                if (bBlitClipVal == FALSE) {
                  Blt8BPPDataTo16BPPBufferOutlineZNB(pDestBuf, uiDestPitchBYTES, gpZBuffer, sZLevel,
                                                     hVObject, sXPos, sYPos, usImageIndex);
                } else if (bBlitClipVal == TRUE) {
                  Blt8BPPDataTo16BPPBufferOutlineClip(pDestBuf, uiDestPitchBYTES, hVObject, sXPos,
                                                      sYPos, usImageIndex, SGP_TRANSPARENT,
                                                      &gClippingRect);
                }
              }
            } else {
              if (fMultiTransShadowZBlitter) {
                if (fZBlitter) {
                  if (fObscuredBlitter) {
                    Blt8BPPDataTo16BPPBufferTransZTransShadowIncObscureClip(
                        pDestBuf, uiDestPitchBYTES, gpZBuffer, sZLevel, hVObject, sXPos, sYPos,
                        usImageIndex, &gClippingRect, sMultiTransShadowZBlitterIndex, pShadeTable);
                  } else {
                    Blt8BPPDataTo16BPPBufferTransZTransShadowIncClip(
                        pDestBuf, uiDestPitchBYTES, gpZBuffer, sZLevel, hVObject, sXPos, sYPos,
                        usImageIndex, &gClippingRect, sMultiTransShadowZBlitterIndex, pShadeTable);
                  }
                }
              } else if (fMultiZBlitter) {
                if (fZBlitter) {
                  if (fObscuredBlitter) {
                    Blt8BPPDataTo16BPPBufferTransZIncObscureClip(
                        pDestBuf, uiDestPitchBYTES, gpZBuffer, sZLevel, hVObject, sXPos, sYPos,
                        usImageIndex, &gClippingRect);
                  } else {
                    if (fWallTile) {
                      Blt8BPPDataTo16BPPBufferTransZIncClipZSameZBurnsThrough(
                          pDestBuf, uiDestPitchBYTES, gpZBuffer, sZLevel, hVObject, sXPos, sYPos,
                          usImageIndex, &gClippingRect);
                    } else {
                      Blt8BPPDataTo16BPPBufferTransZIncClip(pDestBuf, uiDestPitchBYTES, gpZBuffer,
                                                            sZLevel, hVObject, sXPos, sYPos,
                                                            usImageIndex, &gClippingRect);
                    }
                  }
                } else {
                  Blt8BPPDataTo16BPPBufferTransparentClip(pDestBuf, uiDestPitchBYTES, hVObject,
                                                          sXPos, sYPos, usImageIndex,
                                                          &gClippingRect);
                }
              } else {
                const BOOLEAN bBlitClipVal =
                    BltIsClippedOrOffScreen(hVObject, sXPos, sYPos, usImageIndex, &gClippingRect);
                if (bBlitClipVal == TRUE) {
                  if (fPixelate) {
                    Blt8BPPDataTo16BPPBufferTransZNBClipTranslucent(
                        pDestBuf, uiDestPitchBYTES, gpZBuffer, sZLevel, hVObject, sXPos, sYPos,
                        usImageIndex, &gClippingRect);
                  } else if (fMerc) {
                    if (fZBlitter) {
                      if (fZWrite) {
                        Blt8BPPDataTo16BPPBufferTransShadowZClip(
                            pDestBuf, uiDestPitchBYTES, gpZBuffer, sZLevel, hVObject, sXPos, sYPos,
                            usImageIndex, &gClippingRect, pShadeTable);
                      } else {
                        if (fObscuredBlitter) {
                          Blt8BPPDataTo16BPPBufferTransShadowZNBObscuredClip(
                              pDestBuf, uiDestPitchBYTES, gpZBuffer, sZLevel, hVObject, sXPos,
                              sYPos, usImageIndex, &gClippingRect, pShadeTable);
                        } else {
                          Blt8BPPDataTo16BPPBufferTransShadowZNBClip(
                              pDestBuf, uiDestPitchBYTES, gpZBuffer, sZLevel, hVObject, sXPos,
                              sYPos, usImageIndex, &gClippingRect, pShadeTable);
                        }
                      }

                      if (uiLevelNodeFlags & LEVELNODE_UPDATESAVEBUFFERONCE) {
                        SGPVSurface::Lock l(guiSAVEBUFFER);

                        // BLIT HERE
                        Blt8BPPDataTo16BPPBufferTransShadowClip(
                            l.Buffer<uint16_t>(), l.Pitch(), hVObject, sXPos, sYPos, usImageIndex,
                            &gClippingRect, pShadeTable);

                        // Turn it off!
                        pNode->uiFlags &= ~LEVELNODE_UPDATESAVEBUFFERONCE;
                      }
                    } else {
                      Blt8BPPDataTo16BPPBufferTransShadowClip(pDestBuf, uiDestPitchBYTES, hVObject,
                                                              sXPos, sYPos, usImageIndex,
                                                              &gClippingRect, pShadeTable);
                    }
                  } else if (fShadowBlitter) {
                    if (fZBlitter) {
                      if (fZWrite) {
                        Blt8BPPDataTo16BPPBufferShadowZClip(pDestBuf, uiDestPitchBYTES, gpZBuffer,
                                                            sZLevel, hVObject, sXPos, sYPos,
                                                            usImageIndex, &gClippingRect);
                      } else {
                        Blt8BPPDataTo16BPPBufferShadowZClip(pDestBuf, uiDestPitchBYTES, gpZBuffer,
                                                            sZLevel, hVObject, sXPos, sYPos,
                                                            usImageIndex, &gClippingRect);
                      }
                    } else {
                      Blt8BPPDataTo16BPPBufferShadowClip(pDestBuf, uiDestPitchBYTES, hVObject,
                                                         sXPos, sYPos, usImageIndex,
                                                         &gClippingRect);
                    }
                  } else if (fIntensityBlitter) {
                    if (fZBlitter) {
                      if (fZWrite) {
                        Blt8BPPDataTo16BPPBufferIntensityZClip(pDestBuf, uiDestPitchBYTES,
                                                               gpZBuffer, sZLevel, hVObject, sXPos,
                                                               sYPos, usImageIndex, &gClippingRect);
                      } else {
                        Blt8BPPDataTo16BPPBufferIntensityZClip(pDestBuf, uiDestPitchBYTES,
                                                               gpZBuffer, sZLevel, hVObject, sXPos,
                                                               sYPos, usImageIndex, &gClippingRect);
                      }
                    } else {
                      Blt8BPPDataTo16BPPBufferIntensityClip(pDestBuf, uiDestPitchBYTES, hVObject,
                                                            sXPos, sYPos, usImageIndex,
                                                            &gClippingRect);
                    }
                  } else if (fZBlitter) {
                    if (fZWrite) {
                      if (fObscuredBlitter) {
                        Blt8BPPDataTo16BPPBufferTransZClipPixelateObscured(
                            pDestBuf, uiDestPitchBYTES, gpZBuffer, sZLevel, hVObject, sXPos, sYPos,
                            usImageIndex, &gClippingRect);
                      } else {
                        Blt8BPPDataTo16BPPBufferTransZClip(pDestBuf, uiDestPitchBYTES, gpZBuffer,
                                                           sZLevel, hVObject, sXPos, sYPos,
                                                           usImageIndex, &gClippingRect);
                      }
                    } else {
                      Blt8BPPDataTo16BPPBufferTransZNBClip(pDestBuf, uiDestPitchBYTES, gpZBuffer,
                                                           sZLevel, hVObject, sXPos, sYPos,
                                                           usImageIndex, &gClippingRect);
                    }

                    if (uiLevelNodeFlags & LEVELNODE_UPDATESAVEBUFFERONCE) {
                      SGPVSurface::Lock l(guiSAVEBUFFER);

                      // BLIT HERE
                      Blt8BPPDataTo16BPPBufferTransZClip(l.Buffer<uint16_t>(), l.Pitch(), gpZBuffer,
                                                         sZLevel, hVObject, sXPos, sYPos,
                                                         usImageIndex, &gClippingRect);

                      // Turn it off!
                      pNode->uiFlags &= ~LEVELNODE_UPDATESAVEBUFFERONCE;
                    }
                  } else {
                    Blt8BPPDataTo16BPPBufferTransparentClip(pDestBuf, uiDestPitchBYTES, hVObject,
                                                            sXPos, sYPos, usImageIndex,
                                                            &gClippingRect);
                  }
                } else if (bBlitClipVal == FALSE) {
                  if (fPixelate) {
                    if (fZWrite) {
                      Blt8BPPDataTo16BPPBufferTransZTranslucent(pDestBuf, uiDestPitchBYTES,
                                                                gpZBuffer, sZLevel, hVObject, sXPos,
                                                                sYPos, usImageIndex);
                    } else {
                      Blt8BPPDataTo16BPPBufferTransZNBTranslucent(pDestBuf, uiDestPitchBYTES,
                                                                  gpZBuffer, sZLevel, hVObject,
                                                                  sXPos, sYPos, usImageIndex);
                    }
                  } else if (fMerc) {
                    if (fZBlitter) {
                      if (fZWrite) {
                        Blt8BPPDataTo16BPPBufferTransShadowZ(pDestBuf, uiDestPitchBYTES, gpZBuffer,
                                                             sZLevel, hVObject, sXPos, sYPos,
                                                             usImageIndex, pShadeTable);
                      } else {
                        if (fObscuredBlitter) {
                          Blt8BPPDataTo16BPPBufferTransShadowZNBObscured(
                              pDestBuf, uiDestPitchBYTES, gpZBuffer, sZLevel, hVObject, sXPos,
                              sYPos, usImageIndex, pShadeTable);
                        } else {
                          Blt8BPPDataTo16BPPBufferTransShadowZNB(
                              pDestBuf, uiDestPitchBYTES, gpZBuffer, sZLevel, hVObject, sXPos,
                              sYPos, usImageIndex, pShadeTable);
                        }
                      }

                      if (uiLevelNodeFlags & LEVELNODE_UPDATESAVEBUFFERONCE) {
                        SGPVSurface::Lock l(guiSAVEBUFFER);

                        // BLIT HERE
                        Blt8BPPDataTo16BPPBufferTransShadow(l.Buffer<uint16_t>(), l.Pitch(),
                                                            hVObject, sXPos, sYPos, usImageIndex,
                                                            pShadeTable);

                        // Turn it off!
                        pNode->uiFlags &= ~LEVELNODE_UPDATESAVEBUFFERONCE;
                      }
                    } else {
                      Blt8BPPDataTo16BPPBufferTransShadow(pDestBuf, uiDestPitchBYTES, hVObject,
                                                          sXPos, sYPos, usImageIndex, pShadeTable);
                    }
                  } else if (fShadowBlitter) {
                    if (fZBlitter) {
                      if (fZWrite) {
                        Blt8BPPDataTo16BPPBufferShadowZ(pDestBuf, uiDestPitchBYTES, gpZBuffer,
                                                        sZLevel, hVObject, sXPos, sYPos,
                                                        usImageIndex);
                      } else {
                        Blt8BPPDataTo16BPPBufferShadowZNB(pDestBuf, uiDestPitchBYTES, gpZBuffer,
                                                          sZLevel, hVObject, sXPos, sYPos,
                                                          usImageIndex);
                      }
                    } else {
                      Blt8BPPDataTo16BPPBufferShadow(pDestBuf, uiDestPitchBYTES, hVObject, sXPos,
                                                     sYPos, usImageIndex);
                    }
                  } else if (fIntensityBlitter) {
                    if (fZBlitter) {
                      if (fZWrite) {
                        Blt8BPPDataTo16BPPBufferIntensityZ(pDestBuf, uiDestPitchBYTES, gpZBuffer,
                                                           sZLevel, hVObject, sXPos, sYPos,
                                                           usImageIndex);
                      } else {
                        Blt8BPPDataTo16BPPBufferIntensityZNB(pDestBuf, uiDestPitchBYTES, gpZBuffer,
                                                             sZLevel, hVObject, sXPos, sYPos,
                                                             usImageIndex);
                      }
                    } else {
                      Blt8BPPDataTo16BPPBufferIntensity(pDestBuf, uiDestPitchBYTES, hVObject, sXPos,
                                                        sYPos, usImageIndex);
                    }
                  } else if (fZBlitter) {
                    if (fZWrite) {
                      if (fObscuredBlitter) {
                        Blt8BPPDataTo16BPPBufferTransZPixelateObscured(pDestBuf, uiDestPitchBYTES,
                                                                       gpZBuffer, sZLevel, hVObject,
                                                                       sXPos, sYPos, usImageIndex);
                      } else {
                        Blt8BPPDataTo16BPPBufferTransZ(pDestBuf, uiDestPitchBYTES, gpZBuffer,
                                                       sZLevel, hVObject, sXPos, sYPos,
                                                       usImageIndex);
                      }
                    } else {
                      Blt8BPPDataTo16BPPBufferTransZNB(pDestBuf, uiDestPitchBYTES, gpZBuffer,
                                                       sZLevel, hVObject, sXPos, sYPos,
                                                       usImageIndex);
                    }

                    if (uiLevelNodeFlags & LEVELNODE_UPDATESAVEBUFFERONCE) {
                      SGPVSurface::Lock l(guiSAVEBUFFER);

                      // BLIT HERE
                      Blt8BPPDataTo16BPPBufferTransZ(l.Buffer<uint16_t>(), l.Pitch(), gpZBuffer,
                                                     sZLevel, hVObject, sXPos, sYPos, usImageIndex);

                      // Turn it off!
                      pNode->uiFlags &= ~LEVELNODE_UPDATESAVEBUFFERONCE;
                    }

                  } else {
                    Blt8BPPDataTo16BPPBufferTransparent(pDestBuf, uiDestPitchBYTES, hVObject, sXPos,
                                                        sYPos, usImageIndex);
                  }
                }
              }
            }

          next_prev_node:
            if (fLinkedListDirection) {
            next_node:
              pNode = pNode->pNext;
            } else {
              pNode = pNode->pPrevNode;
            }
          }
        } else {
          if (gfEditMode) {
            // ATE: Used here in the editor to denote when an area is not in the
            // world
            /* Kris:  Fixed a couple things here...
             * It seems that scrolling to the bottom right hand corner of the
             * map, would cause the end of the world to be drawn.  Now, this
             * would only crash on my computer and not Emmons, so this should
             * work.  Also, I changed the color from fluorescent green to
             * black, which is easier on the eyes, and prevent the drawing of
             * the end of the world if it would be drawn on the editor's
             * taskbar. */
            if (iTempPosY_S < 360) {
              ColorFillVideoSurfaceArea(FRAME_BUFFER, iTempPosX_S, iTempPosY_S, iTempPosX_S + 40,
                                        std::min(iTempPosY_S + 20, 360),
                                        Get16BPPColor(FROMRGB(0, 0, 0)));
            }
          }
        }

      next_tile:
        iTempPosX_S += 40;
        iTempPosX_M++;
        iTempPosY_M--;
      } while (iTempPosX_S < iEndXS);
    }

    if (bXOddFlag) {
      iAnchorPosY_M++;
    } else {
      iAnchorPosX_M++;
    }

    bXOddFlag = !bXOddFlag;
    iAnchorPosY_S += 10;
  } while (iAnchorPosY_S < iEndYS);

  if (uiFlags & TILES_DYNAMIC_CHECKFOR_INT_TILE) EndCurInteractiveTileCheck();
}

// memcpy's the background to the new scroll position, and renders the missing
// strip via the RenderStaticWorldRect. Dynamic stuff will be updated on the
// next frame by the normal render cycle
static void ScrollBackground(int16_t sScrollXIncrement, int16_t sScrollYIncrement) {
  if (!gfDoVideoScroll) {
    // Clear z-buffer
    memset(gpZBuffer, LAND_Z_LEVEL, gsVIEWPORT_END_Y * SCREEN_WIDTH * 2);

    RenderStaticWorldRect(gsVIEWPORT_START_X, gsVIEWPORT_START_Y, gsVIEWPORT_END_X,
                          gsVIEWPORT_END_Y, FALSE);

    FreeBackgroundRectType(BGND_FLAG_ANIMATED);
  } else {
    gsScrollXIncrement += sScrollXIncrement;
    gsScrollYIncrement += sScrollYIncrement;
  }
}

static BOOLEAN ApplyScrolling(int16_t sTempRenderCenterX, int16_t sTempRenderCenterY,
                              BOOLEAN fForceAdjust, BOOLEAN fCheckOnly);
static void ClearMarkedTiles();
static void ExamineZBufferRect(int16_t sLeft, int16_t sTop, int16_t sRight, int16_t sBottom);
static void RenderDynamicWorld();
static void RenderMarkedWorld();
static void RenderRoomInfo(int16_t sStartPointX_M, int16_t sStartPointY_M, int16_t sStartPointX_S,
                           int16_t sStartPointY_S, int16_t sEndXS, int16_t sEndYS);
static void RenderStaticWorld();

// Render routine takes center X, Y and Z coordinate and gets world
// Coordinates for the window from that using the following functions
// For coordinate transformations
void RenderWorld() {
  gfRenderFullThisFrame = FALSE;

  // If we are testing renderer, set background to pink!
  if (gTacticalStatus.uiFlags & DEBUGCLIFFS) {
    ColorFillVideoSurfaceArea(FRAME_BUFFER, 0, gsVIEWPORT_WINDOW_START_Y, SCREEN_WIDTH,
                              gsVIEWPORT_WINDOW_END_Y, Get16BPPColor(FROMRGB(0, 255, 0)));
    SetRenderFlags(RENDER_FLAG_FULL);
  }

  if (gTacticalStatus.uiFlags & SHOW_Z_BUFFER) {
    SetRenderFlags(RENDER_FLAG_FULL);
  }

  // For now here, update animated tiles
  if (COUNTERDONE(ANIMATETILES)) {
    RESETCOUNTER(ANIMATETILES);
    for (uint32_t i = 0; i != gusNumAnimatedTiles; ++i) {
      TILE_ANIMATION_DATA &a = *gTileDatabase[gusAnimatedTiles[i]].pAnimData;
      if (++a.bCurrentFrame >= a.ubNumFrames) a.bCurrentFrame = 0;
    }
  }

  // HERE, UPDATE GLOW INDEX
  if (COUNTERDONE(GLOW_ENEMYS)) {
    RESETCOUNTER(GLOW_ENEMYS);
    gsCurrentGlowFrame = (gsCurrentGlowFrame + 1) % lengthof(gsGlowFrames);
    gsCurrentItemGlowFrame = (gsCurrentItemGlowFrame + 1) % NUM_ITEM_CYCLE_COLORS;
  }

  if (gRenderFlags & RENDER_FLAG_FULL) {
    gfRenderFullThisFrame = TRUE;
    gfTopMessageDirty = TRUE;

    // Dirty the interface...
    fInterfacePanelDirty = DIRTYLEVEL2;

    // Apply scrolling sets some world variables
    ApplyScrolling(gsRenderCenterX, gsRenderCenterY, TRUE, FALSE);
    ResetLayerOptimizing();

    if (gRenderFlags & RENDER_FLAG_NOZ) {
      RenderStaticWorldRect(gsVIEWPORT_START_X, gsVIEWPORT_START_Y, gsVIEWPORT_END_X,
                            gsVIEWPORT_END_Y, FALSE);
    } else {
      RenderStaticWorld();
    }

    if (!(gRenderFlags & RENDER_FLAG_SAVEOFF)) UpdateSaveBuffer();
  } else if (gRenderFlags & RENDER_FLAG_MARKED) {
    ResetLayerOptimizing();
    RenderMarkedWorld();
    if (!(gRenderFlags & RENDER_FLAG_SAVEOFF)) UpdateSaveBuffer();
  }

  if (!g_scroll_inertia || gRenderFlags & RENDER_FLAG_NOZ || gRenderFlags & RENDER_FLAG_FULL ||
      gRenderFlags & RENDER_FLAG_MARKED) {
    RenderDynamicWorld();
  }

  if (g_scroll_inertia) EmptyBackgroundRects();

  if (gRenderFlags & RENDER_FLAG_ROOMIDS) {
    RenderRoomInfo(gsStartPointX_M, gsStartPointY_M, gsStartPointX_S, gsStartPointY_S, gsEndXS,
                   gsEndYS);
  }

  if (gRenderFlags & RENDER_FLAG_MARKED) ClearMarkedTiles();

  if (gRenderFlags & RENDER_FLAG_CHECKZ && !(gTacticalStatus.uiFlags & NOHIDE_REDUNDENCY)) {
    ExamineZBufferRect(gsVIEWPORT_START_X, gsVIEWPORT_WINDOW_START_Y, gsVIEWPORT_END_X,
                       gsVIEWPORT_WINDOW_END_Y);
  }

  gRenderFlags &=
      ~(RENDER_FLAG_FULL | RENDER_FLAG_MARKED | RENDER_FLAG_ROOMIDS | RENDER_FLAG_CHECKZ);

  if (gTacticalStatus.uiFlags & SHOW_Z_BUFFER) {
    // COPY Z BUFFER TO FRAME BUFFER
    SGPVSurface::Lock l(FRAME_BUFFER);
    uint16_t *const pDestBuf = l.Buffer<uint16_t>();

    for (uint32_t i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; ++i) {
      pDestBuf[i] = gpZBuffer[i];
    }
  }
}

static void CalcRenderParameters(int16_t sLeft, int16_t sTop, int16_t sRight, int16_t sBottom);
static void ResetRenderParameters();

// Start with a center X,Y,Z world coordinate and render direction
// Determine WorldIntersectionPoint and the starting block from these
// Then render away!
void RenderStaticWorldRect(int16_t sLeft, int16_t sTop, int16_t sRight, int16_t sBottom,
                           BOOLEAN fDynamicsToo) {
  RenderLayerID sLevelIDs[10];

  // Calculate render starting parameters
  CalcRenderParameters(sLeft, sTop, sRight, sBottom);

  // Reset layer optimizations
  ResetLayerOptimizing();

  // STATICS
  sLevelIDs[0] = RENDER_STATIC_LAND;
  RenderTiles(TILES_NONE, gsLStartPointX_M, gsLStartPointY_M, gsLStartPointX_S, gsLStartPointY_S,
              gsLEndXS, gsLEndYS, 1, sLevelIDs);

  sLevelIDs[0] = RENDER_STATIC_OBJECTS;
  RenderTiles(TILES_NONE, gsLStartPointX_M, gsLStartPointY_M, gsLStartPointX_S, gsLStartPointY_S,
              gsLEndXS, gsLEndYS, 1, sLevelIDs);

  if (gRenderFlags & RENDER_FLAG_SHADOWS) {
    sLevelIDs[0] = RENDER_STATIC_SHADOWS;
    RenderTiles(TILES_NONE, gsLStartPointX_M, gsLStartPointY_M, gsLStartPointX_S, gsLStartPointY_S,
                gsLEndXS, gsLEndYS, 1, sLevelIDs);
  }

  sLevelIDs[0] = RENDER_STATIC_STRUCTS;
  sLevelIDs[1] = RENDER_STATIC_ROOF;
  sLevelIDs[2] = RENDER_STATIC_ONROOF;
  sLevelIDs[3] = RENDER_STATIC_TOPMOST;
  RenderTiles(TILES_NONE, gsLStartPointX_M, gsLStartPointY_M, gsLStartPointX_S, gsLStartPointY_S,
              gsLEndXS, gsLEndYS, 4, sLevelIDs);

  // ATE: Do obsucred layer!
  sLevelIDs[0] = RENDER_STATIC_STRUCTS;
  sLevelIDs[1] = RENDER_STATIC_ONROOF;
  RenderTiles(TILES_OBSCURED, gsLStartPointX_M, gsLStartPointY_M, gsLStartPointX_S,
              gsLStartPointY_S, gsLEndXS, gsLEndYS, 2, sLevelIDs);

  if (fDynamicsToo) {
    // DYNAMICS
    sLevelIDs[0] = RENDER_DYNAMIC_LAND;
    sLevelIDs[1] = RENDER_DYNAMIC_OBJECTS;
    sLevelIDs[2] = RENDER_DYNAMIC_SHADOWS;
    sLevelIDs[3] = RENDER_DYNAMIC_STRUCT_MERCS;
    sLevelIDs[4] = RENDER_DYNAMIC_MERCS;
    sLevelIDs[5] = RENDER_DYNAMIC_STRUCTS;
    sLevelIDs[6] = RENDER_DYNAMIC_ROOF;
    sLevelIDs[7] = RENDER_DYNAMIC_HIGHMERCS;
    sLevelIDs[8] = RENDER_DYNAMIC_ONROOF;
    RenderTiles(TILES_NONE, gsLStartPointX_M, gsLStartPointY_M, gsLStartPointX_S, gsLStartPointY_S,
                gsLEndXS, gsLEndYS, 9, sLevelIDs);

    SumAdditiveLayerOptimization();
  }

  ResetRenderParameters();

  if (!gfDoVideoScroll) AddBaseDirtyRect(sLeft, sTop, sRight, sBottom);
}

static void RenderStaticWorld() {
  RenderLayerID sLevelIDs[9];

  // Calculate render starting parameters
  CalcRenderParameters(gsVIEWPORT_START_X, gsVIEWPORT_START_Y, gsVIEWPORT_END_X, gsVIEWPORT_END_Y);

  // Clear z-buffer
  memset(gpZBuffer, LAND_Z_LEVEL, gsVIEWPORT_END_Y * SCREEN_WIDTH * 2);

  FreeBackgroundRectType(BGND_FLAG_ANIMATED);
  InvalidateBackgroundRects();

  sLevelIDs[0] = RENDER_STATIC_LAND;
  RenderTiles(TILES_NONE, gsLStartPointX_M, gsLStartPointY_M, gsLStartPointX_S, gsLStartPointY_S,
              gsLEndXS, gsLEndYS, 1, sLevelIDs);

  sLevelIDs[0] = RENDER_STATIC_OBJECTS;
  RenderTiles(TILES_NONE, gsLStartPointX_M, gsLStartPointY_M, gsLStartPointX_S, gsLStartPointY_S,
              gsLEndXS, gsLEndYS, 1, sLevelIDs);

  if (gRenderFlags & RENDER_FLAG_SHADOWS) {
    sLevelIDs[0] = RENDER_STATIC_SHADOWS;
    RenderTiles(TILES_NONE, gsLStartPointX_M, gsLStartPointY_M, gsLStartPointX_S, gsLStartPointY_S,
                gsLEndXS, gsLEndYS, 1, sLevelIDs);
  }

  sLevelIDs[0] = RENDER_STATIC_STRUCTS;
  sLevelIDs[1] = RENDER_STATIC_ROOF;
  sLevelIDs[2] = RENDER_STATIC_ONROOF;
  sLevelIDs[3] = RENDER_STATIC_TOPMOST;
  RenderTiles(TILES_NONE, gsLStartPointX_M, gsLStartPointY_M, gsLStartPointX_S, gsLStartPointY_S,
              gsLEndXS, gsLEndYS, 4, sLevelIDs);

  // ATE: Do obsucred layer!
  sLevelIDs[0] = RENDER_STATIC_STRUCTS;
  sLevelIDs[1] = RENDER_STATIC_ONROOF;
  RenderTiles(TILES_OBSCURED, gsLStartPointX_M, gsLStartPointY_M, gsLStartPointX_S,
              gsLStartPointY_S, gsLEndXS, gsLEndYS, 2, sLevelIDs);

  AddBaseDirtyRect(gsVIEWPORT_START_X, gsVIEWPORT_WINDOW_START_Y, gsVIEWPORT_END_X,
                   gsVIEWPORT_WINDOW_END_Y);
  ResetRenderParameters();
}

static void RenderMarkedWorld() {
  RenderLayerID sLevelIDs[4];

  CalcRenderParameters(gsVIEWPORT_START_X, gsVIEWPORT_START_Y, gsVIEWPORT_END_X, gsVIEWPORT_END_Y);

  RestoreBackgroundRects();
  FreeBackgroundRectType(BGND_FLAG_ANIMATED);
  InvalidateBackgroundRects();

  ResetLayerOptimizing();

  sLevelIDs[0] = RENDER_STATIC_LAND;
  sLevelIDs[1] = RENDER_STATIC_OBJECTS;
  RenderTiles(TILES_MARKED, gsStartPointX_M, gsStartPointY_M, gsStartPointX_S, gsStartPointY_S,
              gsEndXS, gsEndYS, 2, sLevelIDs);

  if (gRenderFlags & RENDER_FLAG_SHADOWS) {
    sLevelIDs[0] = RENDER_STATIC_SHADOWS;
    RenderTiles(TILES_MARKED, gsStartPointX_M, gsStartPointY_M, gsStartPointX_S, gsStartPointY_S,
                gsEndXS, gsEndYS, 1, sLevelIDs);
  }

  sLevelIDs[0] = RENDER_STATIC_STRUCTS;
  RenderTiles(TILES_MARKED, gsStartPointX_M, gsStartPointY_M, gsStartPointX_S, gsStartPointY_S,
              gsEndXS, gsEndYS, 1, sLevelIDs);

  sLevelIDs[0] = RENDER_STATIC_ROOF;
  RenderTiles(TILES_MARKED, gsStartPointX_M, gsStartPointY_M, gsStartPointX_S, gsStartPointY_S,
              gsEndXS, gsEndYS, 1, sLevelIDs);

  sLevelIDs[0] = RENDER_STATIC_ONROOF;
  RenderTiles(TILES_MARKED, gsStartPointX_M, gsStartPointY_M, gsStartPointX_S, gsStartPointY_S,
              gsEndXS, gsEndYS, 1, sLevelIDs);

  sLevelIDs[0] = RENDER_STATIC_TOPMOST;
  RenderTiles(TILES_MARKED, gsStartPointX_M, gsStartPointY_M, gsStartPointX_S, gsStartPointY_S,
              gsEndXS, gsEndYS, 1, sLevelIDs);

  AddBaseDirtyRect(gsVIEWPORT_START_X, gsVIEWPORT_WINDOW_START_Y, gsVIEWPORT_END_X,
                   gsVIEWPORT_WINDOW_END_Y);

  ResetRenderParameters();
}

static void RenderDynamicWorld() {
  RenderLayerID sLevelIDs[10];

  CalcRenderParameters(gsVIEWPORT_START_X, gsVIEWPORT_START_Y, gsVIEWPORT_END_X, gsVIEWPORT_END_Y);

  RestoreBackgroundRects();

  sLevelIDs[0] = RENDER_DYNAMIC_OBJECTS;
  sLevelIDs[1] = RENDER_DYNAMIC_SHADOWS;
  sLevelIDs[2] = RENDER_DYNAMIC_STRUCT_MERCS;
  sLevelIDs[3] = RENDER_DYNAMIC_MERCS;
  sLevelIDs[4] = RENDER_DYNAMIC_STRUCTS;
  sLevelIDs[5] = RENDER_DYNAMIC_HIGHMERCS;
  sLevelIDs[6] = RENDER_DYNAMIC_ROOF;
  sLevelIDs[7] = RENDER_DYNAMIC_ONROOF;
  sLevelIDs[8] = RENDER_DYNAMIC_TOPMOST;
  RenderTiles(TILES_DIRTY, gsStartPointX_M, gsStartPointY_M, gsStartPointX_S, gsStartPointY_S,
              gsEndXS, gsEndYS, 9, sLevelIDs);

  if (!GameState::getInstance()->isEditorMode() || (!gfEditMode && !gfAniEditMode)) {
    RenderTacticalInterface();
  }

  SaveBackgroundRects();

  sLevelIDs[0] = RENDER_DYNAMIC_OBJECTS;
  sLevelIDs[1] = RENDER_DYNAMIC_SHADOWS;
  sLevelIDs[2] = RENDER_DYNAMIC_STRUCT_MERCS;
  sLevelIDs[3] = RENDER_DYNAMIC_MERCS;
  sLevelIDs[4] = RENDER_DYNAMIC_STRUCTS;
  RenderTiles(TILES_NONE, gsStartPointX_M, gsStartPointY_M, gsStartPointX_S, gsStartPointY_S,
              gsEndXS, gsEndYS, 5, sLevelIDs);

  sLevelIDs[0] = RENDER_DYNAMIC_ROOF;
  sLevelIDs[1] = RENDER_DYNAMIC_HIGHMERCS;
  sLevelIDs[2] = RENDER_DYNAMIC_ONROOF;
  RenderTiles(TILES_NONE, gsStartPointX_M, gsStartPointY_M, gsStartPointX_S, gsStartPointY_S,
              gsEndXS, gsEndYS, 3, sLevelIDs);

  sLevelIDs[0] = RENDER_DYNAMIC_TOPMOST;
  // ATE: check here for mouse over structs.....
  RenderTiles(TILES_DYNAMIC_CHECKFOR_INT_TILE, gsStartPointX_M, gsStartPointY_M, gsStartPointX_S,
              gsStartPointY_S, gsEndXS, gsEndYS, 1, sLevelIDs);

  SumAdditiveLayerOptimization();
  ResetRenderParameters();
}

static BOOLEAN HandleScrollDirections(uint32_t ScrollFlags, int16_t sScrollXStep,
                                      int16_t sScrollYStep, BOOLEAN fCheckOnly) {
  int16_t scroll_x = 0;
  if (ScrollFlags & SCROLL_LEFT) scroll_x -= sScrollXStep;
  if (ScrollFlags & SCROLL_RIGHT) scroll_x += sScrollXStep;

  int16_t scroll_y = 0;
  if (ScrollFlags & SCROLL_UP) scroll_y -= sScrollYStep;
  if (ScrollFlags & SCROLL_DOWN) scroll_y += sScrollYStep;

  if (scroll_x != 0) {
    // Check horizontal
    int16_t sTempX_W;
    int16_t sTempY_W;
    FromScreenToCellCoordinates(scroll_x, 0, &sTempX_W, &sTempY_W);
    const int16_t sTempRenderCenterX = gsRenderCenterX + sTempX_W;
    const int16_t sTempRenderCenterY = gsRenderCenterY + sTempY_W;
    if (!ApplyScrolling(sTempRenderCenterX, sTempRenderCenterY, FALSE, fCheckOnly)) {
      scroll_x = 0;
    }
  }

  if (scroll_y != 0) {
    // Check vertical
    int16_t sTempX_W;
    int16_t sTempY_W;
    FromScreenToCellCoordinates(0, scroll_y, &sTempX_W, &sTempY_W);
    const int16_t sTempRenderCenterX = gsRenderCenterX + sTempX_W;
    const int16_t sTempRenderCenterY = gsRenderCenterY + sTempY_W;
    if (!ApplyScrolling(sTempRenderCenterX, sTempRenderCenterY, FALSE, fCheckOnly)) {
      scroll_y = 0;
    }
  }

  const BOOLEAN fAGoodMove = (scroll_x != 0 || scroll_y != 0);
  if (fAGoodMove && !fCheckOnly) ScrollBackground(scroll_x, scroll_y);

  return fAGoodMove;
}

static uint32_t ScrollSpeed() {
  uint32_t speed = 20 << (IsKeyDown(SHIFT) ? 2 : gubCurScrollSpeedID);
  if (!gfDoVideoScroll) speed *= 2;
  return speed;
}

void ScrollWorld() {
  static uint8_t ubOldScrollSpeed = 0;
  static BOOLEAN fFirstTimeInSlideToMode = TRUE;

  if (gfIgnoreScrollDueToCenterAdjust) {
    //	gfIgnoreScrollDueToCenterAdjust = FALSE;
    return;
  }

  BOOLEAN fIgnoreInput = FALSE;

  if (gfIgnoreScrolling) return;
  if (gCurrentUIMode == LOCKUI_MODE) fIgnoreInput = TRUE;

  // If in editor, ignore scrolling if any of the shift keys pressed with arrow
  // keys
  if (gfEditMode && IsKeyDown(CTRL)) return;

  if (IsKeyDown(ALT)) return;

  uint32_t ScrollFlags = 0;

  do {
    // Check for sliding
    if (gTacticalStatus.sSlideTarget != NOWHERE) {
      // Ignore all input...
      // Check if we have reached out dest!
      if (fFirstTimeInSlideToMode) {
        ubOldScrollSpeed = gubCurScrollSpeedID;
        fFirstTimeInSlideToMode = FALSE;
      }

      ScrollFlags = 0;
      int8_t bDirection;
      if (SoldierLocationRelativeToScreen(gTacticalStatus.sSlideTarget, &bDirection,
                                          &ScrollFlags) &&
          GridNoOnVisibleWorldTile(gTacticalStatus.sSlideTarget)) {
        static const uint32_t gScrollDirectionFlags[] = {
            SCROLL_UP | SCROLL_RIGHT,  SCROLL_RIGHT, SCROLL_DOWN | SCROLL_RIGHT, SCROLL_DOWN,
            SCROLL_DOWN | SCROLL_LEFT, SCROLL_LEFT,  SCROLL_UP | SCROLL_LEFT,    SCROLL_UP,
        };

        ScrollFlags = gScrollDirectionFlags[bDirection];
        fIgnoreInput = TRUE;
      } else {
        // We've stopped!
        gTacticalStatus.sSlideTarget = NOWHERE;
      }
    } else {
      // Restore old scroll speed
      if (!fFirstTimeInSlideToMode) {
        gubCurScrollSpeedID = ubOldScrollSpeed;
      }
      fFirstTimeInSlideToMode = TRUE;
    }

    if (!fIgnoreInput) {
      // Check keys
      if (IsKeyDown(SDLK_UP)) ScrollFlags |= SCROLL_UP;
      if (IsKeyDown(SDLK_DOWN)) ScrollFlags |= SCROLL_DOWN;
      if (IsKeyDown(SDLK_RIGHT)) ScrollFlags |= SCROLL_RIGHT;
      if (IsKeyDown(SDLK_LEFT)) ScrollFlags |= SCROLL_LEFT;

      // Do mouse - PUT INTO A TIMER!
      // Put a counter on starting from mouse, if we have not started already!
      if (!g_scroll_inertia && !gfScrollPending) {
        if (!COUNTERDONE(STARTSCROLL)) break;
        RESETCOUNTER(STARTSCROLL);
      }

      if (gusMouseYPos == 0) ScrollFlags |= SCROLL_UP;
      if (gusMouseYPos >= SCREEN_HEIGHT - 1) ScrollFlags |= SCROLL_DOWN;
      if (gusMouseXPos >= SCREEN_WIDTH - 1) ScrollFlags |= SCROLL_RIGHT;
      if (gusMouseXPos == 0) ScrollFlags |= SCROLL_LEFT;
    }
  } while (FALSE);

  BOOLEAN fAGoodMove = FALSE;
  int16_t sScrollXStep = -1;
  int16_t sScrollYStep = -1;
  if (ScrollFlags != 0) {
    // Adjust speed based on whether shift is down
    const uint32_t speed = ScrollSpeed();
    sScrollXStep = speed;
    sScrollYStep = speed / 2;

    fAGoodMove = HandleScrollDirections(ScrollFlags, sScrollXStep, sScrollYStep, TRUE);
  }

  // Has this been an OK scroll?
  if (fAGoodMove) {
    if (COUNTERDONE(NEXTSCROLL)) {
      RESETCOUNTER(NEXTSCROLL);

      // Are we starting a new scroll?
      if (!g_scroll_inertia && !gfScrollPending) {
        // We are starting to scroll - setup scroll pending
        gfScrollPending = TRUE;

        // Remove any interface stuff
        ClearInterface();

        // Return so that next frame things will be erased!
        return;
      }

      // If here, set scroll pending to false
      gfScrollPending = FALSE;

      g_scroll_inertia = true;

      // Now we actually begin our scrolling
      HandleScrollDirections(ScrollFlags, sScrollXStep, sScrollYStep, FALSE);
    }
  } else {
    // ATE: Also if scroll pending never got to scroll....
    if (gfScrollPending) {
      // Do a complete rebuild!
      gfScrollPending = FALSE;

      // Restore Interface!
      RestoreInterface();

      DeleteVideoOverlaysArea();
    }

    // Check if we have just stopped scrolling!
    if (g_scroll_inertia) {
      SetRenderFlags(RENDER_FLAG_FULL | RENDER_FLAG_CHECKZ);

      // Restore Interface!
      RestoreInterface();

      DeleteVideoOverlaysArea();
    }

    g_scroll_inertia = false;
    gfScrollPending = FALSE;
  }
}

void InitRenderParams(uint8_t ubRestrictionID) {
  int16_t gTopLeftWorldLimitX;      // XXX HACK000E
  int16_t gTopLeftWorldLimitY;      // XXX HACK000E
  int16_t gTopRightWorldLimitX;     // XXX HACK000E
  int16_t gTopRightWorldLimitY;     // XXX HACK000E
  int16_t gBottomLeftWorldLimitX;   // XXX HACK000E
  int16_t gBottomLeftWorldLimitY;   // XXX HACK000E
  int16_t gBottomRightWorldLimitX;  // XXX HACK000E
  int16_t gBottomRightWorldLimitY;  // XXX HACK000E
  switch (ubRestrictionID) {
    case 0:  // Default!
      gTopLeftWorldLimitX = CELL_X_SIZE;
      gTopLeftWorldLimitY = CELL_X_SIZE * WORLD_ROWS / 2;

      gTopRightWorldLimitX = CELL_Y_SIZE * WORLD_COLS / 2;
      gTopRightWorldLimitY = CELL_X_SIZE;

      gBottomLeftWorldLimitX = CELL_Y_SIZE * WORLD_COLS / 2;
      gBottomLeftWorldLimitY = CELL_Y_SIZE * WORLD_ROWS;

      gBottomRightWorldLimitX = CELL_Y_SIZE * WORLD_COLS;
      gBottomRightWorldLimitY = CELL_X_SIZE * WORLD_ROWS / 2;
      break;

    case 1:  // BAEMENT LEVEL 1
      gTopLeftWorldLimitX = CELL_X_SIZE * WORLD_ROWS * 3 / 10;
      gTopLeftWorldLimitY = CELL_X_SIZE * WORLD_ROWS / 2;

      gTopRightWorldLimitX = CELL_X_SIZE * WORLD_ROWS / 2;
      gTopRightWorldLimitY = CELL_X_SIZE * WORLD_COLS * 3 / 10;

      gBottomLeftWorldLimitX = CELL_X_SIZE * WORLD_ROWS / 2;
      gBottomLeftWorldLimitY = CELL_X_SIZE * WORLD_COLS * 7 / 10;

      gBottomRightWorldLimitX = CELL_X_SIZE * WORLD_ROWS * 7 / 10;
      gBottomRightWorldLimitY = CELL_X_SIZE * WORLD_ROWS / 2;
      break;

    default:
      abort();  // HACK000E
  }

  gCenterWorldX = CELL_X_SIZE * WORLD_ROWS / 2;
  gCenterWorldY = CELL_X_SIZE * WORLD_COLS / 2;

  // Convert Bounding box into screen coords
  FromCellToScreenCoordinates(gTopLeftWorldLimitX, gTopLeftWorldLimitY, &gsTLX, &gsTLY);
  FromCellToScreenCoordinates(gTopRightWorldLimitX, gTopRightWorldLimitY, &gsTRX, &gsTRY);
  FromCellToScreenCoordinates(gBottomLeftWorldLimitX, gBottomLeftWorldLimitY, &gsBLX, &gsBLY);
  FromCellToScreenCoordinates(gBottomRightWorldLimitX, gBottomRightWorldLimitY, &gsBRX, &gsBRY);
  FromCellToScreenCoordinates(gCenterWorldX, gCenterWorldY, &gsCX, &gsCY);

  // Adjust for interface height tabbing!
  gsTLY += ROOF_LEVEL_HEIGHT;
  gsTRY += ROOF_LEVEL_HEIGHT;
  gsCY += ROOF_LEVEL_HEIGHT / 2;

  DebugMsg(TOPIC_JA2, DBG_LEVEL_0,
           String("World Screen Width %d Height %d", gsTRX - gsTLX, gsBRY - gsTRY));

  // Determine scale factors
  // First scale world screen coords for VIEWPORT ratio
  const double dWorldX = gsTRX - gsTLX;
  const double dWorldY = gsBRY - gsTRY;

  gdScaleX = (double)RADAR_WINDOW_WIDTH / dWorldX;
  gdScaleY = (double)RADAR_WINDOW_HEIGHT / dWorldY;

  const uint32_t n = NUM_ITEM_CYCLE_COLORS;
  for (uint32_t i = 0; i < n; ++i) {
    const uint32_t l = (i < n / 2 ? i + 1 : n - i) * (250 / (n / 2));
    us16BPPItemCycleWhiteColors[i] = Get16BPPColor(FROMRGB(l, l, l));
    us16BPPItemCycleRedColors[i] = Get16BPPColor(FROMRGB(l, 0, 0));
    us16BPPItemCycleYellowColors[i] = Get16BPPColor(FROMRGB(l, l, 0));
  }

  gusNormalItemOutlineColor = Get16BPPColor(FROMRGB(255, 255, 255));
  gusYellowItemOutlineColor = Get16BPPColor(FROMRGB(255, 255, 0));
}

static void CorrectRenderCenter(int16_t sRenderX, int16_t sRenderY, int16_t *pSNewX,
                                int16_t *pSNewY);

static BOOLEAN ApplyScrolling(int16_t sTempRenderCenterX, int16_t sTempRenderCenterY,
                              BOOLEAN fForceAdjust, BOOLEAN fCheckOnly) {
  // Make sure it's a multiple of 5
  sTempRenderCenterX = sTempRenderCenterX / CELL_X_SIZE * CELL_X_SIZE + CELL_X_SIZE / 2;
  sTempRenderCenterY = sTempRenderCenterY / CELL_X_SIZE * CELL_Y_SIZE + CELL_Y_SIZE / 2;

  // Find the diustance from render center to true world center
  const int16_t sDistToCenterX = sTempRenderCenterX - gCenterWorldX;
  const int16_t sDistToCenterY = sTempRenderCenterY - gCenterWorldY;

  // From render center in world coords, convert to render center in "screen"
  // coords
  int16_t sScreenCenterX;
  int16_t sScreenCenterY;
  FromCellToScreenCoordinates(sDistToCenterX, sDistToCenterY, &sScreenCenterX, &sScreenCenterY);

  // Subtract screen center
  sScreenCenterX += gsCX;
  sScreenCenterY += gsCY;

  // Adjust for offset position on screen
  sScreenCenterX -= 0;
  sScreenCenterY -= 10;

  // Get corners in screen coords
  // TOP LEFT
  const int16_t sX_S = (gsVIEWPORT_END_X - gsVIEWPORT_START_X) / 2;
  const int16_t sY_S = (gsVIEWPORT_END_Y - gsVIEWPORT_START_Y) / 2;

  const int16_t sTopLeftWorldX = sScreenCenterX - sX_S;
  const int16_t sTopLeftWorldY = sScreenCenterY - sY_S;

  const int16_t sTopRightWorldX = sScreenCenterX + sX_S;
  const int16_t sTopRightWorldY = sScreenCenterY - sY_S;

  const int16_t sBottomLeftWorldX = sScreenCenterX - sX_S;
  const int16_t sBottomLeftWorldY = sScreenCenterY + sY_S;

  const int16_t sBottomRightWorldX = sScreenCenterX + sX_S;
  const int16_t sBottomRightWorldY = sScreenCenterY + sY_S;

  BOOLEAN fOutLeft = FALSE;
  BOOLEAN fOutRight = FALSE;
  BOOLEAN fOutTop = FALSE;
  BOOLEAN fOutBottom = FALSE;

  double dOpp;
  double dAdj;
  double dAngle;

  // Get angles
  // TOP LEFT CORNER FIRST
  dOpp = sTopLeftWorldY - gsTLY;
  dAdj = sTopLeftWorldX - gsTLX;

  dAngle = atan2(dAdj, dOpp);
  if (dAngle < 0) {
    fOutLeft = TRUE;
  } else if (dAngle > PI / 2) {
    fOutTop = TRUE;
  }

  // TOP RIGHT CORNER
  dOpp = sTopRightWorldY - gsTRY;
  dAdj = gsTRX - sTopRightWorldX;

  dAngle = atan2(dAdj, dOpp);
  if (dAngle < 0) {
    fOutRight = TRUE;
  } else if (dAngle > PI / 2) {
    fOutTop = TRUE;
  }

  // BOTTOM LEFT CORNER
  dOpp = gsBLY - sBottomLeftWorldY;
  dAdj = sBottomLeftWorldX - gsBLX;

  dAngle = atan2(dAdj, dOpp);
  if (dAngle < 0) {
    fOutLeft = TRUE;
  } else if (dAngle > PI / 2) {
    fOutBottom = TRUE;
  }

  // BOTTOM RIGHT CORNER
  dOpp = gsBRY - sBottomRightWorldY;
  dAdj = gsBRX - sBottomRightWorldX;

  dAngle = atan2(dAdj, dOpp);

  if (dAngle < 0) {
    fOutRight = TRUE;
  } else if (dAngle > PI / 2) {
    fOutBottom = TRUE;
  }

  BOOLEAN fScrollGood = FALSE;

  if (!fOutRight && !fOutLeft && !fOutTop && !fOutBottom) fScrollGood = TRUE;

  // If in editor, anything goes
  if (gfEditMode && IsKeyDown(SHIFT)) fScrollGood = TRUE;

  // Reset some UI flags
  gfUIShowExitEast = FALSE;
  gfUIShowExitWest = FALSE;
  gfUIShowExitNorth = FALSE;
  gfUIShowExitSouth = FALSE;

  if (!fScrollGood) {
    if (fForceAdjust) {
      if (fOutTop) {
        // Adjust screen coordinates on the Y!
        int16_t sNewScreenX;
        int16_t sNewScreenY;
        CorrectRenderCenter(sScreenCenterX, gsTLY + sY_S, &sNewScreenX, &sNewScreenY);
        int16_t sTempPosX_W;
        int16_t sTempPosY_W;
        FromScreenToCellCoordinates(sNewScreenX, sNewScreenY, &sTempPosX_W, &sTempPosY_W);

        sTempRenderCenterX = sTempPosX_W;
        sTempRenderCenterY = sTempPosY_W;
        fScrollGood = TRUE;
      }

      if (fOutBottom) {
        // OK, Ajust this since we get rounding errors in our two different
        // calculations.
        int16_t sNewScreenX;
        int16_t sNewScreenY;
        CorrectRenderCenter(sScreenCenterX, gsBLY - sY_S - 50, &sNewScreenX, &sNewScreenY);
        int16_t sTempPosX_W;
        int16_t sTempPosY_W;
        FromScreenToCellCoordinates(sNewScreenX, sNewScreenY, &sTempPosX_W, &sTempPosY_W);

        sTempRenderCenterX = sTempPosX_W;
        sTempRenderCenterY = sTempPosY_W;
        fScrollGood = TRUE;
      }

      if (fOutLeft) {
        int16_t sNewScreenX;
        int16_t sNewScreenY;
        CorrectRenderCenter(gsTLX + sX_S, sScreenCenterY, &sNewScreenX, &sNewScreenY);
        int16_t sTempPosX_W;
        int16_t sTempPosY_W;
        FromScreenToCellCoordinates(sNewScreenX, sNewScreenY, &sTempPosX_W, &sTempPosY_W);

        sTempRenderCenterX = sTempPosX_W;
        sTempRenderCenterY = sTempPosY_W;
        fScrollGood = TRUE;
      }

      if (fOutRight) {
        int16_t sNewScreenX;
        int16_t sNewScreenY;
        CorrectRenderCenter(gsTRX - sX_S, sScreenCenterY, &sNewScreenX, &sNewScreenY);
        int16_t sTempPosX_W;
        int16_t sTempPosY_W;
        FromScreenToCellCoordinates(sNewScreenX, sNewScreenY, &sTempPosX_W, &sTempPosY_W);

        sTempRenderCenterX = sTempPosX_W;
        sTempRenderCenterY = sTempPosY_W;
        fScrollGood = TRUE;
      }
    } else {
      if (fOutRight && gusMouseXPos >= SCREEN_WIDTH - 1) gfUIShowExitEast = TRUE;
      if (fOutLeft && gusMouseXPos == 0) gfUIShowExitWest = TRUE;
      if (fOutTop && gusMouseYPos == 0) gfUIShowExitNorth = TRUE;
      if (fOutBottom && gusMouseYPos >= SCREEN_HEIGHT - 1) gfUIShowExitSouth = TRUE;
    }
  }

  if (fScrollGood && !fCheckOnly) {
    // Make sure it's a multiple of 5
    gsRenderCenterX = sTempRenderCenterX / CELL_X_SIZE * CELL_X_SIZE + CELL_X_SIZE / 2;
    gsRenderCenterY = sTempRenderCenterY / CELL_X_SIZE * CELL_Y_SIZE + CELL_Y_SIZE / 2;

    gsTopLeftWorldX = sTopLeftWorldX - gsTLX;
    gsTopLeftWorldY = sTopLeftWorldY - gsTLY;

    gsBottomRightWorldX = sBottomRightWorldX - gsTLX;
    gsBottomRightWorldY = sBottomRightWorldY - gsTLY;

    SetPositionSndsVolumeAndPanning();
  }

  return fScrollGood;
}

static void ClearMarkedTiles() {
  FOR_EACH_WORLD_TILE(i) { i->uiFlags &= ~MAPELEMENT_REDRAW; }
}

void InvalidateWorldRedundency() {
  SetRenderFlags(RENDER_FLAG_CHECKZ);
  FOR_EACH_WORLD_TILE(i) { i->uiFlags |= MAPELEMENT_REEVALUATE_REDUNDENCY; }
}

#define Z_STRIP_DELTA_Y (Z_SUBLAYERS * 10)

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferTransZIncClip

        Blits an image into the destination buffer, using an ETRLE brush as a
source, and a 16-bit buffer as a destination. As it is blitting, it checks the Z
value of the ZBuffer, and if the pixel's Z level is below that of the current
pixel, it is written on, and the Z value is updated to the current value, for
any non-transparent pixels. The Z-buffer is 16 bit, and must be the same
dimensions (including Pitch) as the destination.

**********************************************************************************************/
static void Blt8BPPDataTo16BPPBufferTransZIncClip(uint16_t *pBuffer, uint32_t uiDestPitchBYTES,
                                                  uint16_t *pZBuffer, uint16_t usZValue,
                                                  HVOBJECT hSrcVObject, int32_t iX, int32_t iY,
                                                  uint16_t usIndex, SGPRect *clipregion) {
  uint32_t Unblitted;
  int32_t LSCount;
  uint16_t usZLevel, usZColsToGo, usZIndex;

  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  int32_t const usHeight = pTrav.usHeight;
  int32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  int32_t ClipX1;
  int32_t ClipY1;
  int32_t ClipX2;
  int32_t ClipY2;
  if (clipregion == NULL) {
    ClipX1 = ClippingRect.iLeft;
    ClipY1 = ClippingRect.iTop;
    ClipX2 = ClippingRect.iRight;
    ClipY2 = ClippingRect.iBottom;
  } else {
    ClipX1 = clipregion->iLeft;
    ClipY1 = clipregion->iTop;
    ClipX2 = clipregion->iRight;
    ClipY2 = clipregion->iBottom;
  }

  // Calculate rows hanging off each side of the screen
  const int32_t LeftSkip = std::min(ClipX1 - std::min(ClipX1, iTempX), usWidth);
  int32_t TopSkip = std::min(ClipY1 - std::min(ClipY1, iTempY), usHeight);
  const int32_t RightSkip = std::min(std::max(ClipX2, iTempX + usWidth) - ClipX2, usWidth);
  const int32_t BottomSkip = std::min(std::max(ClipY2, iTempY + usHeight) - ClipY2, usHeight);

  // calculate the remaining rows and columns to blit
  const int32_t BlitLength = usWidth - LeftSkip - RightSkip;
  int32_t BlitHeight = usHeight - TopSkip - BottomSkip;

  // check if whole thing is clipped
  if (LeftSkip >= usWidth || RightSkip >= usWidth) return;
  if (TopSkip >= usHeight || BottomSkip >= usHeight) return;

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  uint8_t *DestPtr =
      (uint8_t *)pBuffer + uiDestPitchBYTES * (iTempY + TopSkip) + (iTempX + LeftSkip) * 2;
  uint8_t *ZPtr =
      (uint8_t *)pZBuffer + uiDestPitchBYTES * (iTempY + TopSkip) + (iTempX + LeftSkip) * 2;
  const uint32_t LineSkip = uiDestPitchBYTES - BlitLength * 2;
  uint16_t const *const p16BPPPalette = hSrcVObject->CurrentShade();

  if (hSrcVObject->ppZStripInfo == NULL) {
    DebugMsg(TOPIC_VIDEOOBJECT, DBG_LEVEL_0, "Missing Z-Strip info on multi-Z object");
    return;
  }
  // setup for the z-column blitting stuff
  const ZStripInfo *const pZInfo = hSrcVObject->ppZStripInfo[usIndex];
  if (pZInfo == NULL) {
    DebugMsg(TOPIC_VIDEOOBJECT, DBG_LEVEL_0, "Missing Z-Strip info on multi-Z object");
    return;
  }

  uint16_t usZStartLevel = (int16_t)usZValue + pZInfo->bInitialZChange * Z_STRIP_DELTA_Y;
  // set to odd number of pixels for first column

  uint16_t usZStartCols;
  if (LeftSkip > pZInfo->ubFirstZStripWidth) {
    usZStartCols = LeftSkip - pZInfo->ubFirstZStripWidth;
    usZStartCols = 20 - usZStartCols % 20;
  } else if (LeftSkip < pZInfo->ubFirstZStripWidth) {
    usZStartCols = pZInfo->ubFirstZStripWidth - LeftSkip;
  } else {
    usZStartCols = 20;
  }

  usZColsToGo = usZStartCols;

  const int8_t *const pZArray = pZInfo->pbZChange;

  uint16_t usZStartIndex;
  if (LeftSkip >= pZInfo->ubFirstZStripWidth) {
    // Index into array after doing left clipping
    usZStartIndex = 1 + (LeftSkip - pZInfo->ubFirstZStripWidth) / 20;

    // calculates the Z-value after left-side clipping
    if (usZStartIndex) {
      for (uint16_t i = 0; i < usZStartIndex; i++) {
        switch (pZArray[i]) {
          case -1:
            usZStartLevel -= Z_STRIP_DELTA_Y;
            break;
          case 0: /* no change */
            break;
          case 1:
            usZStartLevel += Z_STRIP_DELTA_Y;
            break;
        }
      }
    }
  } else {
    usZStartIndex = 0;
  }

  usZLevel = usZStartLevel;
  usZIndex = usZStartIndex;

#if 1  // XXX TODO
  uint32_t PxCount;

  while (TopSkip > 0) {
    for (;;) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) continue;
      if (PxCount == 0) break;
      SrcPtr += PxCount;
    }
    TopSkip--;
  }

  do {
    usZLevel = usZStartLevel;
    usZIndex = usZStartIndex;
    usZColsToGo = usZStartCols;
    for (LSCount = LeftSkip; LSCount > 0; LSCount -= PxCount) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
        PxCount &= 0x7F;
        if (PxCount > LSCount) {
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitTransparent;
        }
      } else {
        if (PxCount > LSCount) {
          SrcPtr += LSCount;
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitNonTransLoop;
        }
        SrcPtr += PxCount;
      }
    }

    LSCount = BlitLength;
    while (LSCount > 0) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
      BlitTransparent:  // skip transparent pixels
        PxCount &= 0x7F;
        if (PxCount > LSCount) PxCount = LSCount;
        LSCount -= PxCount;
        DestPtr += 2 * PxCount;
        ZPtr += 2 * PxCount;
        for (;;) {
          if (PxCount >= usZColsToGo) {
            PxCount -= usZColsToGo;
            usZColsToGo = 20;

            int8_t delta = pZArray[usZIndex++];
            if (delta < 0) {
              usZLevel -= Z_STRIP_DELTA_Y;
            } else if (delta > 0) {
              usZLevel += Z_STRIP_DELTA_Y;
            }
          } else {
            usZColsToGo -= PxCount;
            break;
          }
        }
      } else {
      BlitNonTransLoop:  // blit non-transparent pixels
        if (PxCount > LSCount) {
          Unblitted = PxCount - LSCount;
          PxCount = LSCount;
        } else {
          Unblitted = 0;
        }
        LSCount -= PxCount;

        do {
          if (*(uint16_t *)ZPtr < usZLevel) {
            *(uint16_t *)ZPtr = usZLevel;
            *(uint16_t *)DestPtr = p16BPPPalette[*SrcPtr];
          }
          SrcPtr++;
          DestPtr += 2;
          ZPtr += 2;
          if (--usZColsToGo == 0) {
            usZColsToGo = 20;

            int8_t delta = pZArray[usZIndex++];
            if (delta < 0) {
              usZLevel -= Z_STRIP_DELTA_Y;
            } else if (delta > 0) {
              usZLevel += Z_STRIP_DELTA_Y;
            }
          }
        } while (--PxCount > 0);
        SrcPtr += Unblitted;
      }
    }

    while (*SrcPtr++ != 0) {
    }  // skip along until we hit and end-of-line marker
    DestPtr += LineSkip;
    ZPtr += LineSkip;
  } while (--BlitHeight > 0);
#else
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

		cmp		TopSkip, 0  // check for nothing clipped on top
		je		LeftSkipSetup

        // Skips the number of lines clipped at the top
TopSkipLoop:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:
		dec		TopSkip
		jnz		TopSkipLoop

        // Start of line loop

        // Skips the pixels hanging outside the left-side boundry
LeftSkipSetup:

		mov		Unblitted, 0  // Unblitted counts any pixels left from a run
		mov		eax, LeftSkip  // after we have skipped enough left-side pixels
		mov		LSCount, eax  // LSCount counts how many pixels skipped so far
		or		eax, eax
		jz		BlitLineSetup  // check for nothing to skip

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, LSCount
		je		LSSkip2  // if equal, skip whole, and start blit with new run
		jb		LSSkip1  // if less, skip whole thing

		add		esi, LSCount  // skip partial run, jump into normal loop for rest
		sub		ecx, LSCount
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitNTL1  // *** jumps into non-transparent blit loop

LSSkip2:
		add		esi, ecx  // skip whole run, and start blit with new run
		jmp		BlitLineSetup


LSSkip1:
		add		esi, ecx  // skip whole run, continue skipping
		sub		LSCount, ecx
		jmp		LeftSkipLoop


LSTrans:
		and		ecx, 07fH
		cmp		ecx, LSCount
		je		BlitLineSetup  // if equal, skip whole, and start blit with new run
		jb		LSTrans1  // if less, skip whole thing

		sub		ecx, LSCount  // skip partial run, jump into normal loop for rest
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitTransparent  // *** jumps into transparent blit loop


LSTrans1:
		sub		LSCount, ecx  // skip whole run, continue skipping
		jmp		LeftSkipLoop

            //-------------------------------------------------
            // setup for beginning of line

BlitLineSetup:
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0

BlitDispatch:

		cmp		LSCount, 0  // Check to see if we're done blitting
		je		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		RSLoop2

        //--------------------------------
        // blitting non-transparent pixels

		and		ecx, 07fH

BlitNTL1:
		mov		ax, [ebx]  // check z-level of pixel
		cmp		ax, usZLevel
		jae		BlitNTL2

		mov		ax, usZLevel  // update z-level of pixel
		mov		[ebx], ax

		xor		eax, eax
		mov		al, [esi]  // copy pixel
		mov		ax, [edx+eax*2]
		mov		[edi], ax

BlitNTL2:
		inc		esi
		add		edi, 2
		add		ebx, 2

		dec		usZColsToGo
		jnz		BlitNTL6

        // update the z-level according to the z-table

		push	edx
		mov		edx, pZArray  // get pointer to array
		xor		eax, eax
		mov		ax, usZIndex  // pick up the current array index
		add		edx, eax
		inc		eax  // increment it
		mov		usZIndex, ax  // store incremented value

		mov		al, [edx]  // get direction instruction
		mov		dx, usZLevel  // get current z-level

		or		al, al
		jz		BlitNTL5  // dir = 0 no change
		js		BlitNTL4               // dir < 0 z-level down
                    // dir > 0 z-level up (default)
		add		dx, Z_STRIP_DELTA_Y
		jmp		BlitNTL5

BlitNTL4:
		sub		dx, Z_STRIP_DELTA_Y

BlitNTL5:
		mov		usZLevel, dx  // store the now-modified z-level
		mov		usZColsToGo, 20  // reset the next z-level change to 20 cols
		pop		edx

BlitNTL6:
		dec		LSCount  // decrement pixel length to blit
		jz		RightSkipLoop  // done blitting the visible line

		dec		ecx
		jnz		BlitNTL1  // continue current run

		jmp		BlitDispatch  // done current run, go for another

                    //----------------------------
                    // skipping transparent pixels

BlitTransparent:  // skip transparent pixels

		and		ecx, 07fH

BlitTrans2:

		add		edi, 2  // move up the destination pointer
		add		ebx, 2

		dec		usZColsToGo
		jnz		BlitTrans1

        // update the z-level according to the z-table

		push	edx
		mov		edx, pZArray  // get pointer to array
		xor		eax, eax
		mov		ax, usZIndex  // pick up the current array index
		add		edx, eax
		inc		eax  // increment it
		mov		usZIndex, ax  // store incremented value

		mov		al, [edx]  // get direction instruction
		mov		dx, usZLevel  // get current z-level

		or		al, al
		jz		BlitTrans5  // dir = 0 no change
		js		BlitTrans4         // dir < 0 z-level down
                      // dir > 0 z-level up (default)
		add		dx, Z_STRIP_DELTA_Y
		jmp		BlitTrans5

BlitTrans4:
		sub		dx, Z_STRIP_DELTA_Y

BlitTrans5:
		mov		usZLevel, dx  // store the now-modified z-level
		mov		usZColsToGo, 20  // reset the next z-level change to 20 cols
		pop		edx

BlitTrans1:

		dec		LSCount  // decrement the pixels to blit
		jz		RightSkipLoop  // done the line

		dec		ecx
		jnz		BlitTrans2

		jmp		BlitDispatch

                        //---------------------------------------------
                        // Scans the ETRLE until it finds an EOL marker

RightSkipLoop:


RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

RSLoop2:

		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip

        // reset all the z-level stuff for a new line

		mov		ax, usZStartLevel
		mov		usZLevel, ax
		mov		ax, usZStartIndex
		mov		usZIndex, ax
		mov		ax, usZStartCols
		mov		usZColsToGo, ax


		jmp		LeftSkipSetup


BlitDone:
  }
#endif
}

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferTransZIncClipSaveZBurnsThrough

        Blits an image into the destination buffer, using an ETRLE brush as a
source, and a 16-bit buffer as a destination. As it is blitting, it checks the Z
value of the ZBuffer, and if the pixel's Z level is below that of the current
pixel, it is written on, and the Z value is updated to the current value, for
any non-transparent pixels. The Z-buffer is 16 bit, and must be the same
dimensions (including Pitch) as the destination.

**********************************************************************************************/
static void Blt8BPPDataTo16BPPBufferTransZIncClipZSameZBurnsThrough(
    uint16_t *pBuffer, uint32_t uiDestPitchBYTES, uint16_t *pZBuffer, uint16_t usZValue,
    HVOBJECT hSrcVObject, int32_t iX, int32_t iY, uint16_t usIndex, SGPRect *clipregion) {
  uint32_t Unblitted;
  int32_t LSCount;
  uint16_t usZLevel, usZColsToGo, usZStartIndex, usZIndex;

  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  int32_t const usHeight = pTrav.usHeight;
  int32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  int32_t ClipX1;
  int32_t ClipY1;
  int32_t ClipX2;
  int32_t ClipY2;
  if (clipregion == NULL) {
    ClipX1 = ClippingRect.iLeft;
    ClipY1 = ClippingRect.iTop;
    ClipX2 = ClippingRect.iRight;
    ClipY2 = ClippingRect.iBottom;
  } else {
    ClipX1 = clipregion->iLeft;
    ClipY1 = clipregion->iTop;
    ClipX2 = clipregion->iRight;
    ClipY2 = clipregion->iBottom;
  }

  // Calculate rows hanging off each side of the screen
  const int32_t LeftSkip = std::min(ClipX1 - std::min(ClipX1, iTempX), usWidth);
  int32_t TopSkip = std::min(ClipY1 - std::min(ClipY1, iTempY), usHeight);
  const int32_t RightSkip = std::min(std::max(ClipX2, iTempX + usWidth) - ClipX2, usWidth);
  const int32_t BottomSkip = std::min(std::max(ClipY2, iTempY + usHeight) - ClipY2, usHeight);

  // calculate the remaining rows and columns to blit
  const int32_t BlitLength = usWidth - LeftSkip - RightSkip;
  int32_t BlitHeight = usHeight - TopSkip - BottomSkip;

  // check if whole thing is clipped
  if (LeftSkip >= usWidth || RightSkip >= usWidth) return;
  if (TopSkip >= usHeight || BottomSkip >= usHeight) return;

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  uint8_t *DestPtr =
      (uint8_t *)pBuffer + uiDestPitchBYTES * (iTempY + TopSkip) + (iTempX + LeftSkip) * 2;
  uint8_t *ZPtr =
      (uint8_t *)pZBuffer + uiDestPitchBYTES * (iTempY + TopSkip) + (iTempX + LeftSkip) * 2;
  const uint32_t LineSkip = uiDestPitchBYTES - BlitLength * 2;
  uint16_t const *const p16BPPPalette = hSrcVObject->CurrentShade();

  if (hSrcVObject->ppZStripInfo == NULL) {
    DebugMsg(TOPIC_VIDEOOBJECT, DBG_LEVEL_0, "Missing Z-Strip info on multi-Z object");
    return;
  }
  // setup for the z-column blitting stuff
  const ZStripInfo *const pZInfo = hSrcVObject->ppZStripInfo[usIndex];
  if (pZInfo == NULL) {
    DebugMsg(TOPIC_VIDEOOBJECT, DBG_LEVEL_0, "Missing Z-Strip info on multi-Z object");
    return;
  }

  uint16_t usZStartLevel = (int16_t)usZValue + pZInfo->bInitialZChange * Z_STRIP_DELTA_Y;
  // set to odd number of pixels for first column

  uint16_t usZStartCols;
  if (LeftSkip > pZInfo->ubFirstZStripWidth) {
    usZStartCols = LeftSkip - pZInfo->ubFirstZStripWidth;
    usZStartCols = 20 - usZStartCols % 20;
  } else if (LeftSkip < pZInfo->ubFirstZStripWidth) {
    usZStartCols = pZInfo->ubFirstZStripWidth - LeftSkip;
  } else {
    usZStartCols = 20;
  }

  usZColsToGo = usZStartCols;

  const int8_t *const pZArray = pZInfo->pbZChange;

  if (LeftSkip >= pZInfo->ubFirstZStripWidth) {
    // Index into array after doing left clipping
    usZStartIndex = 1 + (LeftSkip - pZInfo->ubFirstZStripWidth) / 20;

    // calculates the Z-value after left-side clipping
    if (usZStartIndex) {
      for (uint16_t i = 0; i < usZStartIndex; i++) {
        switch (pZArray[i]) {
          case -1:
            usZStartLevel -= Z_STRIP_DELTA_Y;
            break;
          case 0: /* no change */
            break;
          case 1:
            usZStartLevel += Z_STRIP_DELTA_Y;
            break;
        }
      }
    }
  } else {
    usZStartIndex = 0;
  }

  usZLevel = usZStartLevel;
  usZIndex = usZStartIndex;

#if 1  // XXX TODO
  uint32_t PxCount;

  while (TopSkip > 0) {
    for (;;) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) continue;
      if (PxCount == 0) break;
      SrcPtr += PxCount;
    }
    TopSkip--;
  }

  do {
    usZLevel = usZStartLevel;
    usZIndex = usZStartIndex;
    usZColsToGo = usZStartCols;
    for (LSCount = LeftSkip; LSCount > 0; LSCount -= PxCount) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
        PxCount &= 0x7F;
        if (PxCount > LSCount) {
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitTransparent;
        }
      } else {
        if (PxCount > LSCount) {
          SrcPtr += LSCount;
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitNonTransLoop;
        }
        SrcPtr += PxCount;
      }
    }

    LSCount = BlitLength;
    while (LSCount > 0) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
      BlitTransparent:  // skip transparent pixels
        PxCount &= 0x7F;
        if (PxCount > LSCount) PxCount = LSCount;
        LSCount -= PxCount;
        DestPtr += 2 * PxCount;
        ZPtr += 2 * PxCount;
        for (;;) {
          if (PxCount >= usZColsToGo) {
            PxCount -= usZColsToGo;
            usZColsToGo = 20;

            int8_t delta = pZArray[usZIndex++];
            if (delta < 0) {
              usZLevel -= Z_STRIP_DELTA_Y;
            } else if (delta > 0) {
              usZLevel += Z_STRIP_DELTA_Y;
            }
          } else {
            usZColsToGo -= PxCount;
            break;
          }
        }
      } else {
      BlitNonTransLoop:  // blit non-transparent pixels
        if (PxCount > LSCount) {
          Unblitted = PxCount - LSCount;
          PxCount = LSCount;
        } else {
          Unblitted = 0;
        }
        LSCount -= PxCount;

        do {
          if (*(uint16_t *)ZPtr <= usZLevel) {
            *(uint16_t *)ZPtr = usZLevel;
            *(uint16_t *)DestPtr = p16BPPPalette[*SrcPtr];
          }
          SrcPtr++;
          DestPtr += 2;
          ZPtr += 2;
          if (--usZColsToGo == 0) {
            usZColsToGo = 20;

            int8_t delta = pZArray[usZIndex++];
            if (delta < 0) {
              usZLevel -= Z_STRIP_DELTA_Y;
            } else if (delta > 0) {
              usZLevel += Z_STRIP_DELTA_Y;
            }
          }
        } while (--PxCount > 0);
        SrcPtr += Unblitted;
      }
    }

    while (*SrcPtr++ != 0) {
    }  // skip along until we hit and end-of-line marker
    DestPtr += LineSkip;
    ZPtr += LineSkip;
  } while (--BlitHeight > 0);
#else
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

		cmp		TopSkip, 0  // check for nothing clipped on top
		je		LeftSkipSetup

        // Skips the number of lines clipped at the top
TopSkipLoop:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:
		dec		TopSkip
		jnz		TopSkipLoop

        // Start of line loop

        // Skips the pixels hanging outside the left-side boundry
LeftSkipSetup:

		mov		Unblitted, 0  // Unblitted counts any pixels left from a run
		mov		eax, LeftSkip  // after we have skipped enough left-side pixels
		mov		LSCount, eax  // LSCount counts how many pixels skipped so far
		or		eax, eax
		jz		BlitLineSetup  // check for nothing to skip

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, LSCount
		je		LSSkip2  // if equal, skip whole, and start blit with new run
		jb		LSSkip1  // if less, skip whole thing

		add		esi, LSCount  // skip partial run, jump into normal loop for rest
		sub		ecx, LSCount
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitNTL1  // *** jumps into non-transparent blit loop

LSSkip2:
		add		esi, ecx  // skip whole run, and start blit with new run
		jmp		BlitLineSetup


LSSkip1:
		add		esi, ecx  // skip whole run, continue skipping
		sub		LSCount, ecx
		jmp		LeftSkipLoop


LSTrans:
		and		ecx, 07fH
		cmp		ecx, LSCount
		je		BlitLineSetup  // if equal, skip whole, and start blit with new run
		jb		LSTrans1  // if less, skip whole thing

		sub		ecx, LSCount  // skip partial run, jump into normal loop for rest
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitTransparent  // *** jumps into transparent blit loop


LSTrans1:
		sub		LSCount, ecx  // skip whole run, continue skipping
		jmp		LeftSkipLoop

            //-------------------------------------------------
            // setup for beginning of line

BlitLineSetup:
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0

BlitDispatch:

		cmp		LSCount, 0  // Check to see if we're done blitting
		je		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		RSLoop2

        //--------------------------------
        // blitting non-transparent pixels

		and		ecx, 07fH

BlitNTL1:
		mov		ax, [ebx]  // check z-level of pixel
		cmp		ax, usZLevel
		ja		BlitNTL2

		mov		ax, usZLevel  // update z-level of pixel
		mov		[ebx], ax

		xor		eax, eax
		mov		al, [esi]  // copy pixel
		mov		ax, [edx+eax*2]
		mov		[edi], ax

BlitNTL2:
		inc		esi
		add		edi, 2
		add		ebx, 2

		dec		usZColsToGo
		jnz		BlitNTL6

        // update the z-level according to the z-table

		push	edx
		mov		edx, pZArray  // get pointer to array
		xor		eax, eax
		mov		ax, usZIndex  // pick up the current array index
		add		edx, eax
		inc		eax  // increment it
		mov		usZIndex, ax  // store incremented value

		mov		al, [edx]  // get direction instruction
		mov		dx, usZLevel  // get current z-level

		or		al, al
		jz		BlitNTL5  // dir = 0 no change
		js		BlitNTL4               // dir < 0 z-level down
                    // dir > 0 z-level up (default)
		add		dx, Z_STRIP_DELTA_Y
		jmp		BlitNTL5

BlitNTL4:
		sub		dx, Z_STRIP_DELTA_Y

BlitNTL5:
		mov		usZLevel, dx  // store the now-modified z-level
		mov		usZColsToGo, 20  // reset the next z-level change to 20 cols
		pop		edx

BlitNTL6:
		dec		LSCount  // decrement pixel length to blit
		jz		RightSkipLoop  // done blitting the visible line

		dec		ecx
		jnz		BlitNTL1  // continue current run

		jmp		BlitDispatch  // done current run, go for another

                    //----------------------------
                    // skipping transparent pixels

BlitTransparent:  // skip transparent pixels

		and		ecx, 07fH

BlitTrans2:

		add		edi, 2  // move up the destination pointer
		add		ebx, 2

		dec		usZColsToGo
		jnz		BlitTrans1

        // update the z-level according to the z-table

		push	edx
		mov		edx, pZArray  // get pointer to array
		xor		eax, eax
		mov		ax, usZIndex  // pick up the current array index
		add		edx, eax
		inc		eax  // increment it
		mov		usZIndex, ax  // store incremented value

		mov		al, [edx]  // get direction instruction
		mov		dx, usZLevel  // get current z-level

		or		al, al
		jz		BlitTrans5  // dir = 0 no change
		js		BlitTrans4         // dir < 0 z-level down
                      // dir > 0 z-level up (default)
		add		dx, Z_STRIP_DELTA_Y
		jmp		BlitTrans5

BlitTrans4:
		sub		dx, Z_STRIP_DELTA_Y

BlitTrans5:
		mov		usZLevel, dx  // store the now-modified z-level
		mov		usZColsToGo, 20  // reset the next z-level change to 20 cols
		pop		edx

BlitTrans1:

		dec		LSCount  // decrement the pixels to blit
		jz		RightSkipLoop  // done the line

		dec		ecx
		jnz		BlitTrans2

		jmp		BlitDispatch

                        //---------------------------------------------
                        // Scans the ETRLE until it finds an EOL marker

RightSkipLoop:


RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

RSLoop2:

		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip

        // reset all the z-level stuff for a new line

		mov		ax, usZStartLevel
		mov		usZLevel, ax
		mov		ax, usZStartIndex
		mov		usZIndex, ax
		mov		ax, usZStartCols
		mov		usZColsToGo, ax


		jmp		LeftSkipSetup


BlitDone:
  }
#endif
}

/**********************************************************************************************
 Blt8BPPDataTo16BPPBufferTransZIncObscureClip

        Blits an image into the destination buffer, using an ETRLE brush as a
source, and a 16-bit buffer as a destination. As it is blitting, it checks the Z
value of the ZBuffer, and if the pixel's Z level is below that of the current
pixel, it is written on, and the Z value is updated to the current value, for
any non-transparent pixels. The Z-buffer is 16 bit, and must be the same
dimensions (including Pitch) as the destination.

        //ATE: This blitter makes the values that are =< z value pixellate
rather than not
        // render at all

**********************************************************************************************/
static void Blt8BPPDataTo16BPPBufferTransZIncObscureClip(
    uint16_t *pBuffer, uint32_t uiDestPitchBYTES, uint16_t *pZBuffer, uint16_t usZValue,
    HVOBJECT hSrcVObject, int32_t iX, int32_t iY, uint16_t usIndex, SGPRect *clipregion) {
  uint32_t Unblitted;
  int32_t LSCount;
  uint16_t usZLevel, usZColsToGo, usZIndex;

  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  int32_t const usHeight = pTrav.usHeight;
  int32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  int32_t ClipX1;
  int32_t ClipY1;
  int32_t ClipX2;
  int32_t ClipY2;
  if (clipregion == NULL) {
    ClipX1 = ClippingRect.iLeft;
    ClipY1 = ClippingRect.iTop;
    ClipX2 = ClippingRect.iRight;
    ClipY2 = ClippingRect.iBottom;
  } else {
    ClipX1 = clipregion->iLeft;
    ClipY1 = clipregion->iTop;
    ClipX2 = clipregion->iRight;
    ClipY2 = clipregion->iBottom;
  }

  // Calculate rows hanging off each side of the screen
  const int32_t LeftSkip = std::min(ClipX1 - std::min(ClipX1, iTempX), usWidth);
  int32_t TopSkip = std::min(ClipY1 - std::min(ClipY1, iTempY), usHeight);
  const int32_t RightSkip = std::min(std::max(ClipX2, iTempX + usWidth) - ClipX2, usWidth);
  const int32_t BottomSkip = std::min(std::max(ClipY2, iTempY + usHeight) - ClipY2, usHeight);

  uint32_t uiLineFlag = iTempY & 1;

  // calculate the remaining rows and columns to blit
  const int32_t BlitLength = usWidth - LeftSkip - RightSkip;
  int32_t BlitHeight = usHeight - TopSkip - BottomSkip;

  // check if whole thing is clipped
  if (LeftSkip >= usWidth || RightSkip >= usWidth) return;
  if (TopSkip >= usHeight || BottomSkip >= usHeight) return;

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  uint8_t *DestPtr =
      (uint8_t *)pBuffer + uiDestPitchBYTES * (iTempY + TopSkip) + (iTempX + LeftSkip) * 2;
  uint8_t *ZPtr =
      (uint8_t *)pZBuffer + uiDestPitchBYTES * (iTempY + TopSkip) + (iTempX + LeftSkip) * 2;
  const uint32_t LineSkip = uiDestPitchBYTES - BlitLength * 2;
  uint16_t const *const p16BPPPalette = hSrcVObject->CurrentShade();

  if (hSrcVObject->ppZStripInfo == NULL) {
    DebugMsg(TOPIC_VIDEOOBJECT, DBG_LEVEL_0, "Missing Z-Strip info on multi-Z object");
    return;
  }
  // setup for the z-column blitting stuff
  const ZStripInfo *const pZInfo = hSrcVObject->ppZStripInfo[usIndex];
  if (pZInfo == NULL) {
    DebugMsg(TOPIC_VIDEOOBJECT, DBG_LEVEL_0, "Missing Z-Strip info on multi-Z object");
    return;
  }

  uint16_t usZStartLevel = (int16_t)usZValue + pZInfo->bInitialZChange * Z_STRIP_DELTA_Y;
  // set to odd number of pixels for first column

  uint16_t usZStartCols;
  if (LeftSkip > pZInfo->ubFirstZStripWidth) {
    usZStartCols = LeftSkip - pZInfo->ubFirstZStripWidth;
    usZStartCols = 20 - usZStartCols % 20;
  } else if (LeftSkip < pZInfo->ubFirstZStripWidth) {
    usZStartCols = pZInfo->ubFirstZStripWidth - LeftSkip;
  } else {
    usZStartCols = 20;
  }

  usZColsToGo = usZStartCols;

  const int8_t *const pZArray = pZInfo->pbZChange;

  uint16_t usZStartIndex;
  if (LeftSkip >= pZInfo->ubFirstZStripWidth) {
    // Index into array after doing left clipping
    usZStartIndex = 1 + (LeftSkip - pZInfo->ubFirstZStripWidth) / 20;

    // calculates the Z-value after left-side clipping
    if (usZStartIndex) {
      for (uint16_t i = 0; i < usZStartIndex; i++) {
        switch (pZArray[i]) {
          case -1:
            usZStartLevel -= Z_STRIP_DELTA_Y;
            break;
          case 0: /* no change */
            break;
          case 1:
            usZStartLevel += Z_STRIP_DELTA_Y;
            break;
        }
      }
    }
  } else {
    usZStartIndex = 0;
  }

  usZLevel = usZStartLevel;
  usZIndex = usZStartIndex;

#if 1  // XXX TODO
  uint32_t PxCount;

  while (TopSkip > 0) {
    for (;;) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) continue;
      if (PxCount == 0) break;
      SrcPtr += PxCount;
    }
    uiLineFlag ^= 1;  // XXX evaluate before loop
    TopSkip--;
  }

  do {
    usZLevel = usZStartLevel;
    usZIndex = usZStartIndex;
    usZColsToGo = usZStartCols;
    for (LSCount = LeftSkip; LSCount > 0; LSCount -= PxCount) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
        PxCount &= 0x7F;
        if (PxCount > LSCount) {
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitTransparent;
        }
      } else {
        if (PxCount > LSCount) {
          SrcPtr += LSCount;
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitNonTransLoop;
        }
        SrcPtr += PxCount;
      }
    }

    LSCount = BlitLength;
    while (LSCount > 0) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
      BlitTransparent:  // skip transparent pixels
        PxCount &= 0x7F;
        if (PxCount > LSCount) PxCount = LSCount;
        LSCount -= PxCount;
        DestPtr += 2 * PxCount;
        ZPtr += 2 * PxCount;
        for (;;) {
          if (PxCount >= usZColsToGo) {
            PxCount -= usZColsToGo;
            usZColsToGo = 20;

            int8_t delta = pZArray[usZIndex++];
            if (delta < 0) {
              usZLevel -= Z_STRIP_DELTA_Y;
            } else if (delta > 0) {
              usZLevel += Z_STRIP_DELTA_Y;
            }
          } else {
            usZColsToGo -= PxCount;
            break;
          }
        }
      } else {
      BlitNonTransLoop:  // blit non-transparent pixels
        if (PxCount > LSCount) {
          Unblitted = PxCount - LSCount;
          PxCount = LSCount;
        } else {
          Unblitted = 0;
        }
        LSCount -= PxCount;

        do {
          if (*(uint16_t *)ZPtr < usZLevel ||
              uiLineFlag == (((uintptr_t)DestPtr & 2) != 0))  // XXX update Z when pixelating?
          {
            *(uint16_t *)ZPtr = usZLevel;
            *(uint16_t *)DestPtr = p16BPPPalette[*SrcPtr];
          }
          SrcPtr++;
          DestPtr += 2;
          ZPtr += 2;
          if (--usZColsToGo == 0) {
            usZColsToGo = 20;

            int8_t delta = pZArray[usZIndex++];
            if (delta < 0) {
              usZLevel -= Z_STRIP_DELTA_Y;
            } else if (delta > 0) {
              usZLevel += Z_STRIP_DELTA_Y;
            }
          }
        } while (--PxCount > 0);
        SrcPtr += Unblitted;
      }
    }

    while (*SrcPtr++ != 0) {
    }  // skip along until we hit and end-of-line marker
    uiLineFlag ^= 1;
    DestPtr += LineSkip;
    ZPtr += LineSkip;
  } while (--BlitHeight > 0);
#else
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

		cmp		TopSkip, 0  // check for nothing clipped on top
		je		LeftSkipSetup

        // Skips the number of lines clipped at the top
TopSkipLoop:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:

		xor		uiLineFlag, 1
		dec		TopSkip
		jnz		TopSkipLoop

        // Start of line loop

        // Skips the pixels hanging outside the left-side boundry
LeftSkipSetup:

		mov		Unblitted, 0  // Unblitted counts any pixels left from a run
		mov		eax, LeftSkip  // after we have skipped enough left-side pixels
		mov		LSCount, eax  // LSCount counts how many pixels skipped so far
		or		eax, eax
		jz		BlitLineSetup  // check for nothing to skip

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, LSCount
		je		LSSkip2  // if equal, skip whole, and start blit with new run
		jb		LSSkip1  // if less, skip whole thing

		add		esi, LSCount  // skip partial run, jump into normal loop for rest
		sub		ecx, LSCount
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitNTL1  // *** jumps into non-transparent blit loop

LSSkip2:
		add		esi, ecx  // skip whole run, and start blit with new run
		jmp		BlitLineSetup


LSSkip1:
		add		esi, ecx  // skip whole run, continue skipping
		sub		LSCount, ecx
		jmp		LeftSkipLoop


LSTrans:
		and		ecx, 07fH
		cmp		ecx, LSCount
		je		BlitLineSetup  // if equal, skip whole, and start blit with new run
		jb		LSTrans1  // if less, skip whole thing

		sub		ecx, LSCount  // skip partial run, jump into normal loop for rest
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitTransparent  // *** jumps into transparent blit loop


LSTrans1:
		sub		LSCount, ecx  // skip whole run, continue skipping
		jmp		LeftSkipLoop

            //-------------------------------------------------
            // setup for beginning of line

BlitLineSetup:
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0

BlitDispatch:

		cmp		LSCount, 0  // Check to see if we're done blitting
		je		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		RSLoop2

        //--------------------------------
        // blitting non-transparent pixels

		and		ecx, 07fH

BlitNTL1:
		mov		ax, [ebx]  // check z-level of pixel
		cmp		ax, usZLevel
		jae		BlitPixellate1
		jmp   BlitPixel1

BlitPixellate1:

        // OK, DO PIXELLATE SCHEME HERE!
		test	uiLineFlag, 1
		jz		BlitSkip1

		test	edi, 2
		jz		BlitNTL2
		jmp		BlitPixel1

BlitSkip1:
		test	edi, 2
		jnz		BlitNTL2

BlitPixel1:

		mov		ax, usZLevel  // update z-level of pixel
		mov		[ebx], ax

		xor		eax, eax
		mov		al, [esi]  // copy pixel
		mov		ax, [edx+eax*2]
		mov		[edi], ax

BlitNTL2:
		inc		esi
		add		edi, 2
		add		ebx, 2

		dec		usZColsToGo
		jnz		BlitNTL6

        // update the z-level according to the z-table

		push	edx
		mov		edx, pZArray  // get pointer to array
		xor		eax, eax
		mov		ax, usZIndex  // pick up the current array index
		add		edx, eax
		inc		eax  // increment it
		mov		usZIndex, ax  // store incremented value

		mov		al, [edx]  // get direction instruction
		mov		dx, usZLevel  // get current z-level

		or		al, al
		jz		BlitNTL5  // dir = 0 no change
		js		BlitNTL4               // dir < 0 z-level down
                    // dir > 0 z-level up (default)
		add		dx, Z_STRIP_DELTA_Y
		jmp		BlitNTL5

BlitNTL4:
		sub		dx, Z_STRIP_DELTA_Y

BlitNTL5:
		mov		usZLevel, dx  // store the now-modified z-level
		mov		usZColsToGo, 20  // reset the next z-level change to 20 cols
		pop		edx

BlitNTL6:
		dec		LSCount  // decrement pixel length to blit
		jz		RightSkipLoop  // done blitting the visible line

		dec		ecx
		jnz		BlitNTL1  // continue current run

		jmp		BlitDispatch  // done current run, go for another

                    //----------------------------
                    // skipping transparent pixels

BlitTransparent:  // skip transparent pixels

		and		ecx, 07fH

BlitTrans2:

		add		edi, 2  // move up the destination pointer
		add		ebx, 2

		dec		usZColsToGo
		jnz		BlitTrans1

        // update the z-level according to the z-table

		push	edx
		mov		edx, pZArray  // get pointer to array
		xor		eax, eax
		mov		ax, usZIndex  // pick up the current array index
		add		edx, eax
		inc		eax  // increment it
		mov		usZIndex, ax  // store incremented value

		mov		al, [edx]  // get direction instruction
		mov		dx, usZLevel  // get current z-level

		or		al, al
		jz		BlitTrans5  // dir = 0 no change
		js		BlitTrans4         // dir < 0 z-level down
                      // dir > 0 z-level up (default)
		add		dx, Z_STRIP_DELTA_Y
		jmp		BlitTrans5

BlitTrans4:
		sub		dx, Z_STRIP_DELTA_Y

BlitTrans5:
		mov		usZLevel, dx  // store the now-modified z-level
		mov		usZColsToGo, 20  // reset the next z-level change to 20 cols
		pop		edx

BlitTrans1:

		dec		LSCount  // decrement the pixels to blit
		jz		RightSkipLoop  // done the line

		dec		ecx
		jnz		BlitTrans2

		jmp		BlitDispatch

                        //---------------------------------------------
                        // Scans the ETRLE until it finds an EOL marker

RightSkipLoop:


RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

RSLoop2:

		xor		uiLineFlag, 1
		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip

        // reset all the z-level stuff for a new line

		mov		ax, usZStartLevel
		mov		usZLevel, ax
		mov		ax, usZStartIndex
		mov		usZIndex, ax
		mov		ax, usZStartCols
		mov		usZColsToGo, ax


		jmp		LeftSkipSetup


BlitDone:
  }
#endif
}

/* Blitter Specs
 * 1) 8 to 16 bpp
 * 2) strip z-blitter
 * 3) clipped
 * 4) trans shadow - if value is 254, makes a shadow */
static void Blt8BPPDataTo16BPPBufferTransZTransShadowIncObscureClip(
    uint16_t *pBuffer, uint32_t uiDestPitchBYTES, uint16_t *pZBuffer, uint16_t usZValue,
    HVOBJECT hSrcVObject, int32_t iX, int32_t iY, uint16_t usIndex, SGPRect *clipregion,
    int16_t sZIndex, const uint16_t *p16BPPPalette) {
  uint32_t Unblitted;
  int32_t LSCount;
  uint16_t usZLevel, usZColsToGo, usZIndex;

  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  int32_t const usHeight = pTrav.usHeight;
  int32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  int32_t ClipX1;
  int32_t ClipY1;
  int32_t ClipX2;
  int32_t ClipY2;
  if (clipregion == NULL) {
    ClipX1 = ClippingRect.iLeft;
    ClipY1 = ClippingRect.iTop;
    ClipX2 = ClippingRect.iRight;
    ClipY2 = ClippingRect.iBottom;
  } else {
    ClipX1 = clipregion->iLeft;
    ClipY1 = clipregion->iTop;
    ClipX2 = clipregion->iRight;
    ClipY2 = clipregion->iBottom;
  }

  // Calculate rows hanging off each side of the screen
  const int32_t LeftSkip = std::min(ClipX1 - std::min(ClipX1, iTempX), usWidth);
  int32_t TopSkip = std::min(ClipY1 - std::min(ClipY1, iTempY), usHeight);
  const int32_t RightSkip = std::min(std::max(ClipX2, iTempX + usWidth) - ClipX2, usWidth);
  const int32_t BottomSkip = std::min(std::max(ClipY2, iTempY + usHeight) - ClipY2, usHeight);

  uint32_t uiLineFlag = iTempY & 1;

  // calculate the remaining rows and columns to blit
  const int32_t BlitLength = usWidth - LeftSkip - RightSkip;
  int32_t BlitHeight = usHeight - TopSkip - BottomSkip;

  // check if whole thing is clipped
  if (LeftSkip >= usWidth || RightSkip >= usWidth) return;
  if (TopSkip >= usHeight || BottomSkip >= usHeight) return;

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  uint8_t *DestPtr =
      (uint8_t *)pBuffer + uiDestPitchBYTES * (iTempY + TopSkip) + (iTempX + LeftSkip) * 2;
  uint8_t *ZPtr =
      (uint8_t *)pZBuffer + uiDestPitchBYTES * (iTempY + TopSkip) + (iTempX + LeftSkip) * 2;
  const uint32_t LineSkip = uiDestPitchBYTES - BlitLength * 2;

  if (hSrcVObject->ppZStripInfo == NULL) {
    DebugMsg(TOPIC_VIDEOOBJECT, DBG_LEVEL_0, "Missing Z-Strip info on multi-Z object");
    return;
  }
  // setup for the z-column blitting stuff
  const ZStripInfo *const pZInfo = hSrcVObject->ppZStripInfo[sZIndex];
  if (pZInfo == NULL) {
    DebugMsg(TOPIC_VIDEOOBJECT, DBG_LEVEL_0, "Missing Z-Strip info on multi-Z object");
    return;
  }

  uint16_t usZStartLevel = (int16_t)usZValue + pZInfo->bInitialZChange * Z_SUBLAYERS * 10;

  uint16_t usZStartCols;
  if (LeftSkip > pZInfo->ubFirstZStripWidth) {
    usZStartCols = LeftSkip - pZInfo->ubFirstZStripWidth;
    usZStartCols = 20 - usZStartCols % 20;
  } else if (LeftSkip < pZInfo->ubFirstZStripWidth) {
    usZStartCols = pZInfo->ubFirstZStripWidth - LeftSkip;
  } else {
    usZStartCols = 20;
  }

  // set to odd number of pixels for first column
  usZColsToGo = usZStartCols;

  const int8_t *const pZArray = pZInfo->pbZChange;

  uint16_t usZStartIndex;
  if (LeftSkip >= usZColsToGo) {
    // Index into array after doing left clipping
    usZStartIndex = 1 + (LeftSkip - pZInfo->ubFirstZStripWidth) / 20;

    // calculates the Z-value after left-side clipping
    if (usZStartIndex) {
      for (uint16_t i = 0; i < usZStartIndex; i++) {
        switch (pZArray[i]) {
          case -1:
            usZStartLevel -= Z_SUBLAYERS;
            break;
          case 0: /* no change */
            break;
          case 1:
            usZStartLevel += Z_SUBLAYERS;
            break;
        }
      }
    }
  } else {
    usZStartIndex = 0;
  }

  usZLevel = usZStartLevel;
  usZIndex = usZStartIndex;

#if 1  // XXX TODO
  uint32_t PxCount;

  while (TopSkip > 0) {
    for (;;) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) continue;
      if (PxCount == 0) break;
      SrcPtr += PxCount;
    }
    TopSkip--;
  }

  do {
    usZLevel = usZStartLevel;
    usZIndex = usZStartIndex;
    usZColsToGo = usZStartCols;
    for (LSCount = LeftSkip; LSCount > 0; LSCount -= PxCount) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
        PxCount &= 0x7F;
        if (PxCount > LSCount) {
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitTransparent;
        }
      } else {
        if (PxCount > LSCount) {
          SrcPtr += LSCount;
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitNonTransLoop;
        }
        SrcPtr += PxCount;
      }
    }

    LSCount = BlitLength;
    while (LSCount > 0) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
      BlitTransparent:  // skip transparent pixels
        PxCount &= 0x7F;
        if (PxCount > LSCount) PxCount = LSCount;
        LSCount -= PxCount;
        DestPtr += 2 * PxCount;
        ZPtr += 2 * PxCount;
        for (;;) {
          if (PxCount >= usZColsToGo) {
            PxCount -= usZColsToGo;
            usZColsToGo = 20;

            int8_t delta = pZArray[usZIndex++];
            if (delta < 0) {
              usZLevel -= Z_STRIP_DELTA_Y;
            } else if (delta > 0) {
              usZLevel += Z_STRIP_DELTA_Y;
            }
          } else {
            usZColsToGo -= PxCount;
            break;
          }
        }
      } else {
      BlitNonTransLoop:  // blit non-transparent pixels
        if (PxCount > LSCount) {
          Unblitted = PxCount - LSCount;
          PxCount = LSCount;
        } else {
          Unblitted = 0;
        }
        LSCount -= PxCount;

        do {
          if (*(uint16_t *)ZPtr < usZLevel ||
              uiLineFlag == (((uintptr_t)DestPtr & 2) != 0))  // XXX update Z when pixelating?
          {
            *(uint16_t *)ZPtr = usZLevel;
            uint8_t Px = *SrcPtr;
            if (Px == 254) {
              *(uint16_t *)DestPtr = ShadeTable[*(uint16_t *)DestPtr];
            } else {
              *(uint16_t *)DestPtr = p16BPPPalette[Px];
            }
          }
          SrcPtr++;
          DestPtr += 2;
          ZPtr += 2;
          if (--usZColsToGo == 0) {
            usZColsToGo = 20;

            int8_t delta = pZArray[usZIndex++];
            if (delta < 0) {
              usZLevel -= Z_STRIP_DELTA_Y;
            } else if (delta > 0) {
              usZLevel += Z_STRIP_DELTA_Y;
            }
          }
        } while (--PxCount > 0);
        SrcPtr += Unblitted;
      }
    }

    while (*SrcPtr++ != 0) {
    }  // skip along until we hit and end-of-line marker
    uiLineFlag ^= 1;
    DestPtr += LineSkip;
    ZPtr += LineSkip;
  } while (--BlitHeight > 0);
#else
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

		cmp		TopSkip, 0  // check for nothing clipped on top
		je		LeftSkipSetup

        // Skips the number of lines clipped at the top
TopSkipLoop:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:

		xor		uiLineFlag, 1
		dec		TopSkip
		jnz		TopSkipLoop

        // Start of line loop

        // Skips the pixels hanging outside the left-side boundry
LeftSkipSetup:

		mov		Unblitted, 0  // Unblitted counts any pixels left from a run
		mov		eax, LeftSkip  // after we have skipped enough left-side pixels
		mov		LSCount, eax  // LSCount counts how many pixels skipped so far
		or		eax, eax
		jz		BlitLineSetup  // check for nothing to skip

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, LSCount
		je		LSSkip2  // if equal, skip whole, and start blit with new run
		jb		LSSkip1  // if less, skip whole thing

		add		esi, LSCount  // skip partial run, jump into normal loop for rest
		sub		ecx, LSCount
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitNTL1  // *** jumps into non-transparent blit loop

LSSkip2:
		add		esi, ecx  // skip whole run, and start blit with new run
		jmp		BlitLineSetup


LSSkip1:
		add		esi, ecx  // skip whole run, continue skipping
		sub		LSCount, ecx
		jmp		LeftSkipLoop


LSTrans:
		and		ecx, 07fH
		cmp		ecx, LSCount
		je		BlitLineSetup  // if equal, skip whole, and start blit with new run
		jb		LSTrans1  // if less, skip whole thing

		sub		ecx, LSCount  // skip partial run, jump into normal loop for rest
		mov		eax, BlitLength
		mov		LSCount, eax

		mov		Unblitted, 0
		jmp		BlitTransparent  // *** jumps into transparent blit loop


LSTrans1:
		sub		LSCount, ecx  // skip whole run, continue skipping
		jmp		LeftSkipLoop

            //-------------------------------------------------
            // setup for beginning of line

BlitLineSetup:
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0

BlitDispatch:

		cmp		LSCount, 0  // Check to see if we're done blitting
		je		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		RSLoop2

        //--------------------------------
        // blitting non-transparent pixels

		and		ecx, 07fH

BlitNTL1:
		mov		ax, [ebx]  // check z-level of pixel
		cmp		ax, usZLevel
		jae		BlitPixellate1
		jmp		BlitPixel1

BlitPixellate1:

        // OK, DO PIXELLATE SCHEME HERE!
		test	uiLineFlag, 1
		jz		BlitSkip1

		test	edi, 2
		jz		BlitNTL2
		jmp		BlitPixel1

BlitSkip1:
		test	edi, 2
		jnz		BlitNTL2

BlitPixel1:

		mov		ax, usZLevel  // update z-level of pixel
		mov		[ebx], ax

        // Check for shadow...
		xor		eax, eax
		mov		al, [esi]
		cmp		al, 254
		jne		BlitNTL66

		mov		ax, [edi]
		mov		ax, ShadeTable[eax*2]
		mov		[edi], ax
		jmp		BlitNTL2

BlitNTL66:

		mov		ax, [edx+eax*2]  // Copy pixel
		mov		[edi], ax

BlitNTL2:
		inc		esi
		add		edi, 2
		add		ebx, 2

		dec		usZColsToGo
		jnz		BlitNTL6

        // update the z-level according to the z-table

		push	edx
		mov		edx, pZArray  // get pointer to array
		xor		eax, eax
		mov		ax, usZIndex  // pick up the current array index
		add		edx, eax
		inc		eax  // increment it
		mov		usZIndex, ax  // store incremented value

		mov		al, [edx]  // get direction instruction
		mov		dx, usZLevel  // get current z-level

		or		al, al
		jz		BlitNTL5  // dir = 0 no change
		js		BlitNTL4               // dir < 0 z-level down
                    // dir > 0 z-level up (default)
		add		dx, Z_SUBLAYERS
		jmp		BlitNTL5

BlitNTL4:
		sub		dx, Z_SUBLAYERS

BlitNTL5:
		mov		usZLevel, dx  // store the now-modified z-level
		mov		usZColsToGo, 20  // reset the next z-level change to 20 cols
		pop		edx

BlitNTL6:
		dec		LSCount  // decrement pixel length to blit
		jz		RightSkipLoop  // done blitting the visible line

		dec		ecx
		jnz		BlitNTL1  // continue current run

		jmp		BlitDispatch  // done current run, go for another

                    //----------------------------
                    // skipping transparent pixels

BlitTransparent:  // skip transparent pixels

		and		ecx, 07fH

BlitTrans2:

		add		edi, 2  // move up the destination pointer
		add		ebx, 2

		dec		usZColsToGo
		jnz		BlitTrans1

        // update the z-level according to the z-table

		push	edx
		mov		edx, pZArray  // get pointer to array
		xor		eax, eax
		mov		ax, usZIndex  // pick up the current array index
		add		edx, eax
		inc		eax  // increment it
		mov		usZIndex, ax  // store incremented value

		mov		al, [edx]  // get direction instruction
		mov		dx, usZLevel  // get current z-level

		or		al, al
		jz		BlitTrans5  // dir = 0 no change
		js		BlitTrans4         // dir < 0 z-level down
                      // dir > 0 z-level up (default)
		add		dx, Z_SUBLAYERS
		jmp		BlitTrans5

BlitTrans4:
		sub		dx, Z_SUBLAYERS

BlitTrans5:
		mov		usZLevel, dx  // store the now-modified z-level
		mov		usZColsToGo, 20  // reset the next z-level change to 20 cols
		pop		edx

BlitTrans1:

		dec		LSCount  // decrement the pixels to blit
		jz		RightSkipLoop  // done the line

		dec		ecx
		jnz		BlitTrans2

		jmp		BlitDispatch

                        //---------------------------------------------
                        // Scans the ETRLE until it finds an EOL marker

RightSkipLoop:


RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

RSLoop2:

		xor		uiLineFlag, 1
		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip

        // reset all the z-level stuff for a new line

		mov		ax, usZStartLevel
		mov		usZLevel, ax
		mov		ax, usZStartIndex
		mov		usZIndex, ax
		mov		ax, usZStartCols
		mov		usZColsToGo, ax


		jmp		LeftSkipSetup


BlitDone:
  }
#endif
}

static void CorrectRenderCenter(int16_t sRenderX, int16_t sRenderY, int16_t *pSNewX,
                                int16_t *pSNewY) {
  // Use radar scale values to get screen values, then convert ot map values,
  // rounding to nearest middle tile
  int16_t sScreenX = sRenderX;
  int16_t sScreenY = sRenderY;

  // Adjust for offsets!
  sScreenX += 0;
  sScreenY += 10;

  // Adjust to viewport start!
  sScreenX -= (gsVIEWPORT_END_X - gsVIEWPORT_START_X) / 2;
  sScreenY -= (gsVIEWPORT_END_Y - gsVIEWPORT_START_Y) / 2;

  // Make sure these coordinates are multiples of scroll steps
  const uint32_t speed = ScrollSpeed();
  const int16_t speed_x = speed;
  const int16_t speed_y = speed / 2;

  sScreenX = sScreenX / speed_x * speed_x;
  sScreenY = sScreenY / speed_y * speed_y;

  // Adjust back
  sScreenX += (gsVIEWPORT_END_X - gsVIEWPORT_START_X) / 2;
  sScreenY += (gsVIEWPORT_END_Y - gsVIEWPORT_START_Y) / 2;

  *pSNewX = sScreenX;
  *pSNewY = sScreenY;
}

/* Blitter Specs
 * 1) 8 to 16 bpp
 * 2) strip z-blitter
 * 3) clipped
 * 4) trans shadow - if value is 254, makes a shadow */
static void Blt8BPPDataTo16BPPBufferTransZTransShadowIncClip(
    uint16_t *pBuffer, uint32_t uiDestPitchBYTES, uint16_t *pZBuffer, uint16_t usZValue,
    HVOBJECT hSrcVObject, int32_t iX, int32_t iY, uint16_t usIndex, SGPRect *clipregion,
    int16_t sZIndex, const uint16_t *p16BPPPalette) {
  uint32_t Unblitted;
  int32_t LSCount;
  uint16_t usZLevel, usZColsToGo, usZIndex;

  Assert(hSrcVObject != NULL);
  Assert(pBuffer != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  int32_t const usHeight = pTrav.usHeight;
  int32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  int32_t ClipX1;
  int32_t ClipY1;
  int32_t ClipX2;
  int32_t ClipY2;
  if (clipregion == NULL) {
    ClipX1 = ClippingRect.iLeft;
    ClipY1 = ClippingRect.iTop;
    ClipX2 = ClippingRect.iRight;
    ClipY2 = ClippingRect.iBottom;
  } else {
    ClipX1 = clipregion->iLeft;
    ClipY1 = clipregion->iTop;
    ClipX2 = clipregion->iRight;
    ClipY2 = clipregion->iBottom;
  }

  // Calculate rows hanging off each side of the screen
  const int32_t LeftSkip = std::min(ClipX1 - std::min(ClipX1, iTempX), usWidth);
  int32_t TopSkip = std::min(ClipY1 - std::min(ClipY1, iTempY), usHeight);
  const int32_t RightSkip = std::min(std::max(ClipX2, iTempX + usWidth) - ClipX2, usWidth);
  const int32_t BottomSkip = std::min(std::max(ClipY2, iTempY + usHeight) - ClipY2, usHeight);

  // calculate the remaining rows and columns to blit
  const int32_t BlitLength = usWidth - LeftSkip - RightSkip;
  int32_t BlitHeight = usHeight - TopSkip - BottomSkip;

  // check if whole thing is clipped
  if (LeftSkip >= usWidth || RightSkip >= usWidth) return;
  if (TopSkip >= usHeight || BottomSkip >= usHeight) return;

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  uint8_t *DestPtr =
      (uint8_t *)pBuffer + uiDestPitchBYTES * (iTempY + TopSkip) + (iTempX + LeftSkip) * 2;
  uint8_t *ZPtr =
      (uint8_t *)pZBuffer + uiDestPitchBYTES * (iTempY + TopSkip) + (iTempX + LeftSkip) * 2;
  const uint32_t LineSkip = uiDestPitchBYTES - BlitLength * 2;

  if (hSrcVObject->ppZStripInfo == NULL) {
    DebugMsg(TOPIC_VIDEOOBJECT, DBG_LEVEL_0, "Missing Z-Strip info on multi-Z object");
    return;
  }
  // setup for the z-column blitting stuff
  const ZStripInfo *const pZInfo = hSrcVObject->ppZStripInfo[sZIndex];
  if (pZInfo == NULL) {
    DebugMsg(TOPIC_VIDEOOBJECT, DBG_LEVEL_0, "Missing Z-Strip info on multi-Z object");
    return;
  }

  uint16_t usZStartLevel = (int16_t)usZValue + pZInfo->bInitialZChange * Z_SUBLAYERS * 10;

  uint16_t usZStartCols;
  if (LeftSkip > pZInfo->ubFirstZStripWidth) {
    usZStartCols = LeftSkip - pZInfo->ubFirstZStripWidth;
    usZStartCols = 20 - usZStartCols % 20;
  } else if (LeftSkip < pZInfo->ubFirstZStripWidth) {
    usZStartCols = pZInfo->ubFirstZStripWidth - LeftSkip;
  } else {
    usZStartCols = 20;
  }

  // set to odd number of pixels for first column
  usZColsToGo = usZStartCols;

  const int8_t *const pZArray = pZInfo->pbZChange;

  uint16_t usZStartIndex;
  if (LeftSkip >= usZColsToGo) {
    // Index into array after doing left clipping
    usZStartIndex = 1 + (LeftSkip - pZInfo->ubFirstZStripWidth) / 20;

    // calculates the Z-value after left-side clipping
    if (usZStartIndex) {
      for (uint16_t i = 0; i < usZStartIndex; i++) {
        switch (pZArray[i]) {
          case -1:
            usZStartLevel -= Z_SUBLAYERS;
            break;
          case 0: /* no change */
            break;
          case 1:
            usZStartLevel += Z_SUBLAYERS;
            break;
        }
      }
    }
  } else {
    usZStartIndex = 0;
  }

  usZLevel = usZStartLevel;
  usZIndex = usZStartIndex;

#if 1  // XXX TODO
  uint32_t PxCount;

  while (TopSkip > 0) {
    for (;;) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) continue;
      if (PxCount == 0) break;
      SrcPtr += PxCount;
    }
    TopSkip--;
  }

  do {
    usZLevel = usZStartLevel;
    usZIndex = usZStartIndex;
    usZColsToGo = usZStartCols;
    for (LSCount = LeftSkip; LSCount > 0; LSCount -= PxCount) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
        PxCount &= 0x7F;
        if (PxCount > LSCount) {
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitTransparent;
        }
      } else {
        if (PxCount > LSCount) {
          SrcPtr += LSCount;
          PxCount -= LSCount;
          LSCount = BlitLength;
          goto BlitNonTransLoop;
        }
        SrcPtr += PxCount;
      }
    }

    LSCount = BlitLength;
    while (LSCount > 0) {
      PxCount = *SrcPtr++;
      if (PxCount & 0x80) {
      BlitTransparent:  // skip transparent pixels
        PxCount &= 0x7F;
        if (PxCount > LSCount) PxCount = LSCount;
        LSCount -= PxCount;
        DestPtr += 2 * PxCount;
        ZPtr += 2 * PxCount;
        for (;;) {
          if (PxCount >= usZColsToGo) {
            PxCount -= usZColsToGo;
            usZColsToGo = 20;

            int8_t delta = pZArray[usZIndex++];
            if (delta < 0) {
              usZLevel -= Z_STRIP_DELTA_Y;
            } else if (delta > 0) {
              usZLevel += Z_STRIP_DELTA_Y;
            }
          } else {
            usZColsToGo -= PxCount;
            break;
          }
        }
      } else {
      BlitNonTransLoop:  // blit non-transparent pixels
        if (PxCount > LSCount) {
          Unblitted = PxCount - LSCount;
          PxCount = LSCount;
        } else {
          Unblitted = 0;
        }
        LSCount -= PxCount;

        do {
          if (*(uint16_t *)ZPtr <= usZLevel) {
            *(uint16_t *)ZPtr = usZLevel;

            uint32_t Px = *SrcPtr;
            if (Px == 254) {
              *(uint16_t *)DestPtr = ShadeTable[*(uint16_t *)DestPtr];
            } else {
              *(uint16_t *)DestPtr = p16BPPPalette[*SrcPtr];
            }
          }
          SrcPtr++;
          DestPtr += 2;
          ZPtr += 2;
          if (--usZColsToGo == 0) {
            usZColsToGo = 20;

            int8_t delta = pZArray[usZIndex++];
            if (delta < 0) {
              usZLevel -= Z_SUBLAYERS;
            } else if (delta > 0) {
              usZLevel += Z_SUBLAYERS;
            }
          }
        } while (--PxCount > 0);
        SrcPtr += Unblitted;
      }
    }

    while (*SrcPtr++ != 0) {
    }  // skip along until we hit and end-of-line marker
    DestPtr += LineSkip;
    ZPtr += LineSkip;
  } while (--BlitHeight > 0);
#else
  __asm {

		mov		esi, SrcPtr
		mov		edi, DestPtr
		mov		edx, p16BPPPalette
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

		cmp		TopSkip, 0  // check for nothing clipped on top
		je		LeftSkipSetup

        // Skips the number of lines clipped at the top
TopSkipLoop:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		TopSkipLoop
		jz		TSEndLine

		add		esi, ecx
		jmp		TopSkipLoop

TSEndLine:
		dec		TopSkip
		jnz		TopSkipLoop

        // Start of line loop

        // Skips the pixels hanging outside the left-side boundry
LeftSkipSetup:

		mov		Unblitted, 0  // Unblitted counts any pixels left from a run
		mov		eax, LeftSkip  // after we have skipped enough left-side pixels
		mov		LSCount, eax  // LSCount counts how many pixels skipped so far
		or		eax, eax
		jz		BlitLineSetup  // check for nothing to skip

LeftSkipLoop:

		mov		cl, [esi]
		inc		esi

		or		cl, cl
		js		LSTrans

		cmp		ecx, LSCount
		je		LSSkip2  // if equal, skip whole, and start blit with new run
		jb		LSSkip1  // if less, skip whole thing

		add		esi, LSCount  // skip partial run, jump into normal loop for rest
		sub		ecx, LSCount
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0
		jmp		BlitNTL1  // *** jumps into non-transparent blit loop

LSSkip2:
		add		esi, ecx  // skip whole run, and start blit with new run
		jmp		BlitLineSetup


LSSkip1:
		add		esi, ecx  // skip whole run, continue skipping
		sub		LSCount, ecx
		jmp		LeftSkipLoop


LSTrans:
		and		ecx, 07fH
		cmp		ecx, LSCount
		je		BlitLineSetup  // if equal, skip whole, and start blit with new run
		jb		LSTrans1  // if less, skip whole thing

		sub		ecx, LSCount  // skip partial run, jump into normal loop for rest
		mov		eax, BlitLength
		mov		LSCount, eax

		mov		Unblitted, 0
		jmp		BlitTransparent  // *** jumps into transparent blit loop


LSTrans1:
		sub		LSCount, ecx  // skip whole run, continue skipping
		jmp		LeftSkipLoop

            //-------------------------------------------------
            // setup for beginning of line

BlitLineSetup:
		mov		eax, BlitLength
		mov		LSCount, eax
		mov		Unblitted, 0

BlitDispatch:

		cmp		LSCount, 0  // Check to see if we're done blitting
		je		RightSkipLoop

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		RSLoop2

        //--------------------------------
        // blitting non-transparent pixels

		and		ecx, 07fH

BlitNTL1:
		mov		ax, [ebx]  // check z-level of pixel
		cmp		ax, usZLevel
		ja		BlitNTL2

		mov		ax, usZLevel  // update z-level of pixel
		mov		[ebx], ax

        // Check for shadow...
		xor		eax, eax
		mov		al, [esi]
		cmp		al, 254
		jne		BlitNTL66

		mov		ax, [edi]
		mov		ax, ShadeTable[eax*2]
		mov		[edi], ax
		jmp		BlitNTL2

BlitNTL66:

		mov		ax, [edx+eax*2]  // Copy pixel
		mov		[edi], ax

BlitNTL2:
		inc		esi
		add		edi, 2
		add		ebx, 2

		dec		usZColsToGo
		jnz		BlitNTL6

        // update the z-level according to the z-table

		push	edx
		mov		edx, pZArray  // get pointer to array
		xor		eax, eax
		mov		ax, usZIndex  // pick up the current array index
		add		edx, eax
		inc		eax  // increment it
		mov		usZIndex, ax  // store incremented value

		mov		al, [edx]  // get direction instruction
		mov		dx, usZLevel  // get current z-level

		or		al, al
		jz		BlitNTL5  // dir = 0 no change
		js		BlitNTL4               // dir < 0 z-level down
                    // dir > 0 z-level up (default)
		add		dx, Z_SUBLAYERS
		jmp		BlitNTL5

BlitNTL4:
		sub		dx, Z_SUBLAYERS

BlitNTL5:
		mov		usZLevel, dx  // store the now-modified z-level
		mov		usZColsToGo, 20  // reset the next z-level change to 20 cols
		pop		edx

BlitNTL6:
		dec		LSCount  // decrement pixel length to blit
		jz		RightSkipLoop  // done blitting the visible line

		dec		ecx
		jnz		BlitNTL1  // continue current run

		jmp		BlitDispatch  // done current run, go for another

                    //----------------------------
                    // skipping transparent pixels

BlitTransparent:  // skip transparent pixels

		and		ecx, 07fH

BlitTrans2:

		add		edi, 2  // move up the destination pointer
		add		ebx, 2

		dec		usZColsToGo
		jnz		BlitTrans1

        // update the z-level according to the z-table

		push	edx
		mov		edx, pZArray  // get pointer to array
		xor		eax, eax
		mov		ax, usZIndex  // pick up the current array index
		add		edx, eax
		inc		eax  // increment it
		mov		usZIndex, ax  // store incremented value

		mov		al, [edx]  // get direction instruction
		mov		dx, usZLevel  // get current z-level

		or		al, al
		jz		BlitTrans5  // dir = 0 no change
		js		BlitTrans4         // dir < 0 z-level down
                      // dir > 0 z-level up (default)
		add		dx, Z_SUBLAYERS
		jmp		BlitTrans5

BlitTrans4:
		sub		dx, Z_SUBLAYERS

BlitTrans5:
		mov		usZLevel, dx  // store the now-modified z-level
		mov		usZColsToGo, 20  // reset the next z-level change to 20 cols
		pop		edx

BlitTrans1:

		dec		LSCount  // decrement the pixels to blit
		jz		RightSkipLoop  // done the line

		dec		ecx
		jnz		BlitTrans2

		jmp		BlitDispatch

                        //---------------------------------------------
                        // Scans the ETRLE until it finds an EOL marker

RightSkipLoop:


RSLoop1:
		mov		al, [esi]
		inc		esi
		or		al, al
		jnz		RSLoop1

RSLoop2:

		dec		BlitHeight
		jz		BlitDone
		add		edi, LineSkip
		add		ebx, LineSkip

        // reset all the z-level stuff for a new line

		mov		ax, usZStartLevel
		mov		usZLevel, ax
		mov		ax, usZStartIndex
		mov		usZIndex, ax
		mov		ax, usZStartCols
		mov		usZColsToGo, ax


		jmp		LeftSkipSetup


BlitDone:
  }
#endif
}

static void RenderRoomInfo(int16_t sStartPointX_M, int16_t sStartPointY_M, int16_t sStartPointX_S,
                           int16_t sStartPointY_S, int16_t sEndXS, int16_t sEndYS) {
  int16_t sAnchorPosX_M = sStartPointX_M;
  int16_t sAnchorPosY_M = sStartPointY_M;
  int16_t sAnchorPosX_S = sStartPointX_S;
  int16_t sAnchorPosY_S = sStartPointY_S;

  SGPVSurface::Lock l(FRAME_BUFFER);
  uint16_t *const pDestBuf = l.Buffer<uint16_t>();
  uint32_t const uiDestPitchBYTES = l.Pitch();

  BOOLEAN bXOddFlag = FALSE;
  do {
    int16_t sTempPosX_M = sAnchorPosX_M;
    int16_t sTempPosY_M = sAnchorPosY_M;
    int16_t sTempPosX_S = sAnchorPosX_S;
    int16_t sTempPosY_S = sAnchorPosY_S;

    if (bXOddFlag) sTempPosX_S += 20;

    do {
      const uint16_t usTileIndex = FASTMAPROWCOLTOPOS(sTempPosY_M, sTempPosX_M);
      if (usTileIndex < GRIDSIZE) {
        const int16_t sX = sTempPosX_S + WORLD_TILE_X / 2 - 5;
        int16_t sY = sTempPosY_S + WORLD_TILE_Y / 2 - 5;

        // THIS ROOM STUFF IS ONLY DONE IN THE EDITOR...
        // ADJUST BY SHEIGHT
        sY -= gpWorldLevelData[usTileIndex].sHeight;

        if (gubWorldRoomInfo[usTileIndex] != NO_ROOM) {
          SetFont(SMALLCOMPFONT);
          SetFontDestBuffer(FRAME_BUFFER, 0, 0, SCREEN_WIDTH, gsVIEWPORT_END_Y);
          switch (gubWorldRoomInfo[usTileIndex] % 5) {
            case 0:
              SetFontForeground(FONT_GRAY3);
              break;
            case 1:
              SetFontForeground(FONT_YELLOW);
              break;
            case 2:
              SetFontForeground(FONT_LTRED);
              break;
            case 3:
              SetFontForeground(FONT_LTBLUE);
              break;
            case 4:
              SetFontForeground(FONT_LTGREEN);
              break;
          }
          mprintf_buffer(pDestBuf, uiDestPitchBYTES, sX, sY, L"%d", gubWorldRoomInfo[usTileIndex]);
          SetFontDestBuffer(FRAME_BUFFER);
        }
      }

      sTempPosX_S += 40;
      sTempPosX_M++;
      sTempPosY_M--;
    } while (sTempPosX_S < sEndXS);

    if (bXOddFlag) {
      sAnchorPosY_M++;
    } else {
      sAnchorPosX_M++;
    }

    bXOddFlag = !bXOddFlag;
    sAnchorPosY_S += 10;
  } while (sAnchorPosY_S < sEndYS);
}

static void ExamineZBufferForHiddenTiles(int16_t sStartPointX_M, int16_t sStartPointY_M,
                                         int16_t sStartPointX_S, int16_t sStartPointY_S,
                                         int16_t sEndXS, int16_t sEndYS);

static void ExamineZBufferRect(int16_t sLeft, int16_t sTop, int16_t sRight, int16_t sBottom) {
  CalcRenderParameters(sLeft, sTop, sRight, sBottom);
  ExamineZBufferForHiddenTiles(gsStartPointX_M, gsStartPointY_M, gsStartPointX_S, gsStartPointY_S,
                               gsEndXS, gsEndYS);
}

static BOOLEAN IsTileRedundant(uint16_t *pZBuffer, uint16_t usZValue, HVOBJECT hSrcVObject,
                               int32_t iX, int32_t iY, uint16_t usIndex);

static void ExamineZBufferForHiddenTiles(int16_t sStartPointX_M, int16_t sStartPointY_M,
                                         int16_t sStartPointX_S, int16_t sStartPointY_S,
                                         int16_t sEndXS, int16_t sEndYS) {
  // Begin Render Loop
  int16_t sAnchorPosX_M = sStartPointX_M;
  int16_t sAnchorPosY_M = sStartPointY_M;
  int16_t sAnchorPosX_S = sStartPointX_S;
  int16_t sAnchorPosY_S = sStartPointY_S;

  // Get VObject for firt land peice!
  const TILE_ELEMENT *const TileElem = &gTileDatabase[FIRSTTEXTURE1];

  BOOLEAN bXOddFlag = FALSE;
  do {
    int16_t sTempPosX_M = sAnchorPosX_M;
    int16_t sTempPosY_M = sAnchorPosY_M;
    int16_t sTempPosX_S = sAnchorPosX_S;
    const int16_t sTempPosY_S = sAnchorPosY_S;

    if (bXOddFlag) sTempPosX_S += 20;

    do {
      const uint16_t usTileIndex = FASTMAPROWCOLTOPOS(sTempPosY_M, sTempPosX_M);
      if (usTileIndex < GRIDSIZE) {
        // ATE: Don;t let any vehicle sit here....
        if (FindStructure(usTileIndex, STRUCTURE_MOBILE)) {
          // Continue...
          goto ENDOFLOOP;
        }

        const int16_t sX = sTempPosX_S;
        int16_t sY = sTempPosY_S - gpWorldLevelData[usTileIndex].sHeight;

        // Adjust for interface level
        sY += gsRenderHeight;

        // Caluluate zvalue
        // Look for anything less than struct layer!
        int16_t sWorldX;
        int16_t sZLevel;
        GetAbsoluteScreenXYFromMapPos(usTileIndex, &sWorldX, &sZLevel);

        sZLevel += gsRenderHeight;
        sZLevel = sZLevel * Z_SUBLAYERS + STRUCT_Z_LEVEL;

        if (gpWorldLevelData[usTileIndex].uiFlags & MAPELEMENT_REEVALUATE_REDUNDENCY) {
          const int8_t bBlitClipVal = BltIsClippedOrOffScreen(
              TileElem->hTileSurface, sX, sY, TileElem->usRegionIndex, &gClippingRect);
          if (bBlitClipVal == FALSE) {
            // Set flag to not evaluate again!
            gpWorldLevelData[usTileIndex].uiFlags &= ~MAPELEMENT_REEVALUATE_REDUNDENCY;

            if (IsTileRedundant(gpZBuffer, sZLevel, TileElem->hTileSurface, sX, sY,
                                TileElem->usRegionIndex)) {
              // Mark in the world!
              gpWorldLevelData[usTileIndex].uiFlags |= MAPELEMENT_REDUNDENT;
            } else {
              // Un Mark in the world!
              gpWorldLevelData[usTileIndex].uiFlags &= ~MAPELEMENT_REDUNDENT;
            }
          }
        }
      }

    ENDOFLOOP:
      sTempPosX_S += 40;
      sTempPosX_M++;
      sTempPosY_M--;
    } while (sTempPosX_S < sEndXS);

    if (bXOddFlag) {
      ++sAnchorPosY_M;
    } else {
      ++sAnchorPosX_M;
    }

    bXOddFlag = !bXOddFlag;
    sAnchorPosY_S += 10;
  } while (sAnchorPosY_S < sEndYS);
}

static void CalcRenderParameters(int16_t sLeft, int16_t sTop, int16_t sRight, int16_t sBottom) {
  int16_t sTempPosX_W, sTempPosY_W;

  gOldClipRect = gClippingRect;

  // Set new clipped rect
  gClippingRect.iLeft = std::max(gsVIEWPORT_START_X, sLeft);
  gClippingRect.iRight = std::min(gsVIEWPORT_END_X, sRight);
  gClippingRect.iTop = std::max(gsVIEWPORT_WINDOW_START_Y, sTop);
  gClippingRect.iBottom = std::min(gsVIEWPORT_WINDOW_END_Y, sBottom);

  gsEndXS = sRight + VIEWPORT_XOFFSET_S;
  gsEndYS = sBottom + VIEWPORT_YOFFSET_S;

  const int16_t sRenderCenterX_W = gsRenderCenterX;
  const int16_t sRenderCenterY_W = gsRenderCenterY;

  // STEP THREE - determine starting point in world coords
  // a) Determine where in screen coords to start rendering
  gsStartPointX_S = (gsVIEWPORT_END_X - gsVIEWPORT_START_X) / 2 - (sLeft - VIEWPORT_XOFFSET_S);
  gsStartPointY_S = (gsVIEWPORT_END_Y - gsVIEWPORT_START_Y) / 2 - (sTop - VIEWPORT_YOFFSET_S);

  // b) Convert these distances into world distances
  FromScreenToCellCoordinates(gsStartPointX_S, gsStartPointY_S, &sTempPosX_W, &sTempPosY_W);

  // c) World start point is Render center minus this distance
  gsStartPointX_W = sRenderCenterX_W - sTempPosX_W;
  gsStartPointY_W = sRenderCenterY_W - sTempPosY_W;

  // NOTE: Increase X map value by 1 tile to offset where on screen we are...
  if (gsStartPointX_W > 0) gsStartPointX_W += CELL_X_SIZE;

  // d) screen start point is screen distances minus screen center
  gsStartPointX_S = sLeft - VIEWPORT_XOFFSET_S;
  gsStartPointY_S = sTop - VIEWPORT_YOFFSET_S;

  // STEP FOUR - Determine Start block
  // a) Find start block
  gsStartPointX_M = gsStartPointX_W / CELL_X_SIZE;
  gsStartPointY_M = gsStartPointY_W / CELL_Y_SIZE;

  // STEP 5 - Determine Deltas for center and find screen values
  // Make sure these coordinates are multiples of scroll steps
  const int16_t sOffsetX_W = abs(gsStartPointX_W) - abs(gsStartPointX_M * CELL_X_SIZE);
  const int16_t sOffsetY_W = abs(gsStartPointY_W) - abs(gsStartPointY_M * CELL_Y_SIZE);

  int16_t sOffsetX_S;
  int16_t sOffsetY_S;
  FromCellToScreenCoordinates(sOffsetX_W, sOffsetY_W, &sOffsetX_S, &sOffsetY_S);

  if (gsStartPointY_W < 0) {
    gsStartPointY_S += 0;
  } else {
    gsStartPointY_S -= sOffsetY_S;
  }
  gsStartPointX_S -= sOffsetX_S;

  /////////////////////////////////////////
  // ATE: CALCULATE LARGER OFFSET VALUES
  gsLEndXS = sRight + LARGER_VIEWPORT_XOFFSET_S;
  gsLEndYS = sBottom + LARGER_VIEWPORT_YOFFSET_S;

  // STEP THREE - determine starting point in world coords
  // a) Determine where in screen coords to start rendering
  gsLStartPointX_S =
      (gsVIEWPORT_END_X - gsVIEWPORT_START_X) / 2 - (sLeft - LARGER_VIEWPORT_XOFFSET_S);
  gsLStartPointY_S =
      (gsVIEWPORT_END_Y - gsVIEWPORT_START_Y) / 2 - (sTop - LARGER_VIEWPORT_YOFFSET_S);

  // b) Convert these distances into world distances
  FromScreenToCellCoordinates(gsLStartPointX_S, gsLStartPointY_S, &sTempPosX_W, &sTempPosY_W);

  // c) World start point is Render center minus this distance
  gsLStartPointX_W = sRenderCenterX_W - sTempPosX_W;
  gsLStartPointY_W = sRenderCenterY_W - sTempPosY_W;

  // NOTE: Increase X map value by 1 tile to offset where on screen we are...
  if (gsLStartPointX_W > 0) gsLStartPointX_W += CELL_X_SIZE;

  // d) screen start point is screen distances minus screen center
  gsLStartPointX_S = sLeft - LARGER_VIEWPORT_XOFFSET_S;
  gsLStartPointY_S = sTop - LARGER_VIEWPORT_YOFFSET_S;

  // STEP FOUR - Determine Start block
  // a) Find start block
  gsLStartPointX_M = gsLStartPointX_W / CELL_X_SIZE;
  gsLStartPointY_M = gsLStartPointY_W / CELL_Y_SIZE;

  // Adjust starting screen coordinates
  gsLStartPointX_S -= sOffsetX_S;

  if (gsLStartPointY_W < 0) {
    gsLStartPointY_S += 0;
    gsLStartPointX_S -= 20;
  } else {
    gsLStartPointY_S -= sOffsetY_S;
  }
}

static void ResetRenderParameters() {
  // Restore clipping rect
  gClippingRect = gOldClipRect;
}

static BOOLEAN IsTileRedundant(uint16_t *pZBuffer, uint16_t usZValue, HVOBJECT hSrcVObject,
                               int32_t iX, int32_t iY, uint16_t usIndex) {
  BOOLEAN fHidden = TRUE;

  Assert(hSrcVObject != NULL);

  // Get Offsets from Index into structure
  ETRLEObject const &pTrav = hSrcVObject->SubregionProperties(usIndex);
  uint32_t usHeight = pTrav.usHeight;
  uint32_t const usWidth = pTrav.usWidth;

  // Add to start position of dest buffer
  int32_t const iTempX = iX + pTrav.sOffsetX;
  int32_t const iTempY = iY + pTrav.sOffsetY;

  CHECKF(iTempX >= 0);
  CHECKF(iTempY >= 0);

  uint8_t const *SrcPtr = hSrcVObject->PixData(pTrav);
  const uint8_t *ZPtr = (const uint8_t *)(pZBuffer + iTempY * SCREEN_WIDTH + iTempX);
  const uint32_t LineSkip = (SCREEN_WIDTH - usWidth) * 2;

#if 1  // XXX TODO
  do {
    for (;;) {
      uint8_t data = *SrcPtr++;

      if (data == 0) break;
      if (data & 0x80) {
        data &= 0x7F;
        ZPtr += 2 * data;
      } else {
        SrcPtr += data;
        do {
          if (*(const uint16_t *)ZPtr < usZValue) return FALSE;
          ZPtr += 2;
        } while (--data > 0);
      }
    }
    ZPtr += LineSkip;
  } while (--usHeight > 0);
#else
  __asm {

		mov		esi, SrcPtr
		xor		eax, eax
		mov		ebx, ZPtr
		xor		ecx, ecx

BlitDispatch:

		mov		cl, [esi]
		inc		esi
		or		cl, cl
		js		BlitTransparent
		jz		BlitDoneLine

        // BlitNonTransLoop:

		xor		eax, eax

BlitNTL4:

		mov		ax, usZValue
		cmp		ax, [ebx]
		jle		BlitNTL5

        //    Set false, flag
		mov   fHidden, 0
		jmp		BlitDone


BlitNTL5:
		inc		esi
		inc		ebx
		inc		ebx

		dec		cl
		jnz		BlitNTL4

		jmp		BlitDispatch


BlitTransparent:

		and		ecx, 07fH
    //		shl		ecx, 1
		add   ecx, ecx
		add		ebx, ecx
		jmp		BlitDispatch


BlitDoneLine:

		dec		usHeight
		jz		BlitDone
		add		ebx, LineSkip
		jmp		BlitDispatch


BlitDone:
  }
#endif

  return fHidden;
}

void SetRenderCenter(int16_t sNewX, int16_t sNewY) {
  if (gfIgnoreScrolling) return;

  // Apply these new coordinates to the renderer!
  ApplyScrolling(sNewX, sNewY, TRUE, FALSE);

  // Set flag to ignore scrolling this frame
  gfIgnoreScrollDueToCenterAdjust = TRUE;

  // Set full render flag!
  // DIRTY THE WORLD!
  SetRenderFlags(RENDER_FLAG_FULL);

  gfPlotNewMovement = TRUE;

  if (gfScrollPending) {
    // Do a complete rebuild!
    gfScrollPending = FALSE;

    // Restore Interface!
    RestoreInterface();

    DeleteVideoOverlaysArea();
  }

  g_scroll_inertia = false;
}

#undef FAIL
#include "gtest/gtest.h"

TEST(RenderWorld, asserts) {
  EXPECT_EQ(lengthof(RenderFX), NUM_RENDER_FX_TYPES);
  EXPECT_EQ(lengthof(RenderFXStartIndex), NUM_RENDER_FX_TYPES);
  EXPECT_EQ(lengthof(g_render_fx_layer_flags), NUM_RENDER_FX_TYPES);
}
