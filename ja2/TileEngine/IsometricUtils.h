// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __ISOMETRIC_UTILSH
#define __ISOMETRIC_UTILSH

#include "Tactical/OverheadTypes.h"
#include "TileEngine/WorldDef.h"

#define MAXCOL WORLD_COLS
#define MAXROW WORLD_ROWS
#define GRIDSIZE (MAXCOL * MAXROW)
#define RIGHTMOSTGRID (MAXCOL - 1)
#define LASTROWSTART (GRIDSIZE - MAXCOL)
#define NOWHERE (GRIDSIZE + 1)
#define MAPWIDTH (WORLD_COLS)
#define MAPHEIGHT (WORLD_ROWS)
#define MAPLENGTH (MAPHEIGHT * MAPWIDTH)

static inline uint8_t OppositeDirection(uint32_t dir) { return (dir + 4) % NUM_WORLD_DIRECTIONS; }
static inline uint8_t TwoCCDirection(uint32_t dir) { return (dir + 6) % NUM_WORLD_DIRECTIONS; }
static inline uint8_t TwoCDirection(uint32_t dir) { return (dir + 2) % NUM_WORLD_DIRECTIONS; }
static inline uint8_t OneCCDirection(uint32_t dir) { return (dir + 7) % NUM_WORLD_DIRECTIONS; }
static inline uint8_t OneCDirection(uint32_t dir) { return (dir + 1) % NUM_WORLD_DIRECTIONS; }

extern const uint8_t gPurpendicularDirection[NUM_WORLD_DIRECTIONS][NUM_WORLD_DIRECTIONS];

// Macros

//                                                |Check for map
//                                                bounds------------------------------------------|
//                                                |Invalid-|
//                                                |Valid-------------------|
#define MAPROWCOLTOPOS(r, c)                                                 \
  (((r < 0) || (r >= WORLD_ROWS) || (c < 0) || (c >= WORLD_COLS)) ? (0xffff) \
                                                                  : ((r) * WORLD_COLS + (c)))

#define GETWORLDINDEXFROMWORLDCOORDS(r, c) \
  ((int16_t)(r / CELL_X_SIZE)) * WORLD_COLS + ((int16_t)(c / CELL_Y_SIZE))

void ConvertGridNoToXY(int16_t sGridNo, int16_t *sXPos, int16_t *sYPos);
void ConvertGridNoToCellXY(int16_t sGridNo, int16_t *sXPos, int16_t *sYPos);
void ConvertGridNoToCenterCellXY(int16_t sGridNo, int16_t *sXPos, int16_t *sYPos);

// GRID NO MANIPULATION FUNCTIONS
int16_t NewGridNo(int16_t sGridno, int16_t sDirInc);
int16_t DirectionInc(int16_t sDirection);
int32_t OutOfBounds(int16_t sGridno, int16_t sProposedGridno);

BOOLEAN GetMouseXY(int16_t *psMouseX, int16_t *psMouseY);
BOOLEAN GetMouseWorldCoords(int16_t *psMouseX, int16_t *psMouseY);

/* Returns the GridNo of the tile the mouse cursor is currently over or NOWHERE
 * if the cursor is not over any tile. */
GridNo GetMouseMapPos();

void GetAbsoluteScreenXYFromMapPos(GridNo pos, int16_t *psWorldScreenX, int16_t *psWorldScreenY);
GridNo GetMapPosFromAbsoluteScreenXY(int16_t sWorldScreenX, int16_t sWorldScreenY);

void FromCellToScreenCoordinates(int16_t sCellX, int16_t sCellY, int16_t *psScreenX,
                                 int16_t *psScreenY);
void FromScreenToCellCoordinates(int16_t sScreenX, int16_t sScreenY, int16_t *psCellX,
                                 int16_t *psCellY);

// Higher resolution convertion functions
void FloatFromCellToScreenCoordinates(float dCellX, float dCellY, float *pdScreenX,
                                      float *pdScreenY);

BOOLEAN GridNoOnVisibleWorldTile(int16_t sGridNo);
BOOLEAN GridNoOnEdgeOfMap(int16_t sGridNo, int8_t *pbDirection);

void CellXYToScreenXY(int16_t sCellX, int16_t sCellY, int16_t *sScreenX, int16_t *sScreenY);

int32_t GetRangeFromGridNoDiff(int16_t sGridNo1, int16_t sGridNo2);
int32_t GetRangeInCellCoordsFromGridNoDiff(int16_t sGridNo1, int16_t sGridNo2);

bool IsPointInScreenRect(int16_t x, int16_t y, SGPRect const &);
BOOLEAN IsPointInScreenRectWithRelative(int16_t sXPos, int16_t sYPos, SGPRect *pRect,
                                        int16_t *sXRel, int16_t *sRelY);

int16_t PythSpacesAway(int16_t sOrigin, int16_t sDest);
int16_t SpacesAway(int16_t sOrigin, int16_t sDest);
int16_t CardinalSpacesAway(int16_t sOrigin, int16_t sDest);
bool FindHigherLevel(SOLDIERTYPE const *, int8_t *out_direction = 0);
bool FindLowerLevel(SOLDIERTYPE const *, int8_t *out_direction = 0);

int16_t QuickestDirection(int16_t origin, int16_t dest);
int16_t ExtQuickestDirection(int16_t origin, int16_t dest);

// Returns the (center ) cell coordinates in X
int16_t CenterX(int16_t sGridno);

// Returns the (center ) cell coordinates in Y
int16_t CenterY(int16_t sGridno);

BOOLEAN FindFenceJumpDirection(SOLDIERTYPE const *, int8_t *out_direction = 0);

// Simply chooses a random gridno within valid boundaries (for dropping things
// in unloaded sectors)
int16_t RandomGridNo();

extern uint32_t guiForceRefreshMousePositionCalculation;

extern const int16_t DirIncrementer[8];

#endif
