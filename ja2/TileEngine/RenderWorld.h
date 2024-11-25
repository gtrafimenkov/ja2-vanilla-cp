// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef RENDERWORLD_H
#define RENDERWORLD_H

#include "SGP/Types.h"

extern BOOLEAN gfDoVideoScroll;
extern uint8_t gubCurScrollSpeedID;

enum RenderFlags {
  RENDER_FLAG_NONE = 0,
  RENDER_FLAG_FULL = 0x00000001,
  RENDER_FLAG_SHADOWS = 0x00000002,
  RENDER_FLAG_MARKED = 0x00000004,
  RENDER_FLAG_SAVEOFF = 0x00000008,
  RENDER_FLAG_NOZ = 0x00000010,
  RENDER_FLAG_ROOMIDS = 0x00000020,
  RENDER_FLAG_CHECKZ = 0x00000040,
  RENDER_FLAG_FOVDEBUG = 0x00000200
};
ENUM_BITSET(RenderFlags)

#define SCROLL_UP 0x00000001
#define SCROLL_DOWN 0x00000002
#define SCROLL_RIGHT 0x00000004
#define SCROLL_LEFT 0x00000008

#define Z_SUBLAYERS 8
#define LAND_Z_LEVEL 0
#define OBJECT_Z_LEVEL 1
#define SHADOW_Z_LEVEL 2
#define MERC_Z_LEVEL 3
#define STRUCT_Z_LEVEL 4
#define ROOF_Z_LEVEL 5
#define ONROOF_Z_LEVEL 6
#define TOPMOST_Z_LEVEL 32767

enum RenderLayerFlags {
  // Highest bit value is rendered first
  TILES_LAYER_ALL = 0xFFFFFFFF,
  TILES_STATIC_LAND = 0x00040000,
  TILES_STATIC_OBJECTS = 0x00020000,
  TILES_STATIC_SHADOWS = 0x00008000,
  TILES_STATIC_STRUCTURES = 0x00004000,
  TILES_STATIC_ROOF = 0x00002000,
  TILES_STATIC_ONROOF = 0x00001000,
  TILES_STATIC_TOPMOST = 0x00000800,
  TILES_ALL_DYNAMICS = 0x00000FFF,
  TILES_DYNAMIC_LAND = 0x00000200,
  TILES_DYNAMIC_OBJECTS = 0x00000100,
  TILES_DYNAMIC_SHADOWS = 0x00000080,
  TILES_DYNAMIC_STRUCT_MERCS = 0x00000040,
  TILES_DYNAMIC_MERCS = 0x00000020,
  TILES_DYNAMIC_STRUCTURES = 0x00000010,
  TILES_DYNAMIC_ROOF = 0x00000008,
  TILES_DYNAMIC_HIGHMERCS = 0x00000004,
  TILES_DYNAMIC_ONROOF = 0x00000002,
  TILES_DYNAMIC_TOPMOST = 0x00000001,
  TILES_LAYER_NONE = 0
};
ENUM_BITSET(RenderLayerFlags)

extern int16_t gsScrollXIncrement;
extern int16_t gsScrollYIncrement;
extern int16_t gsRenderHeight;

extern const int16_t gsVIEWPORT_START_X;
extern const int16_t gsVIEWPORT_START_Y;
extern const int16_t gsVIEWPORT_END_X;
extern const int16_t gsVIEWPORT_END_Y;
extern int16_t gsVIEWPORT_WINDOW_START_Y;
extern int16_t gsVIEWPORT_WINDOW_END_Y;

extern int16_t gsRenderCenterX;
extern int16_t gsRenderCenterY;
extern int16_t gsRenderWorldOffsetX;
extern int16_t gsRenderWorldOffsetY;

// CURRENT VIEWPORT IN WORLD COORDS
extern int16_t gsTopLeftWorldX;
extern int16_t gsTopLeftWorldY;
extern int16_t gsBottomRightWorldX;
extern int16_t gsBottomRightWorldY;

// GLOBAL COORDINATES
extern int16_t gCenterWorldX;
extern int16_t gCenterWorldY;
extern int16_t gsTLX;
extern int16_t gsTLY;
extern int16_t gsTRX;
extern int16_t gsTRY;
extern int16_t gsBLX;
extern int16_t gsBLY;
extern int16_t gsBRX;
extern int16_t gsBRY;
extern int16_t gsCX;
extern int16_t gsCY;
extern double gdScaleX;
extern double gdScaleY;

extern BOOLEAN gfIgnoreScrollDueToCenterAdjust;

void ScrollWorld();
void InitRenderParams(uint8_t ubRestrictionID);
void RenderWorld();

void ResetSpecificLayerOptimizing(RenderLayerFlags);

void SetRenderFlags(RenderFlags);
void ClearRenderFlags(RenderFlags);

void RenderSetShadows(BOOLEAN fShadows);

extern uint16_t *gpZBuffer;
extern BOOLEAN gfIgnoreScrolling;

extern bool g_scroll_inertia;
extern BOOLEAN gfScrollPending;

void RenderStaticWorldRect(int16_t sLeft, int16_t sTop, int16_t sRight, int16_t sBottom,
                           BOOLEAN fDynamicsToo);

void InvalidateWorldRedundency();

void SetRenderCenter(int16_t sNewX, int16_t sNewY);

#endif
