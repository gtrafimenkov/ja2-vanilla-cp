// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Macro.h"
/****************************************************************************************
 * JA2 Lighting Module
 *
 *		Tile-based, ray-casted lighting system.
 *
 *		Lights are precalculated into linked lists containing offsets from
 *0,0, and a light level to add at that tile. Lists are constructed by casting a
 *ray from the origin of the light, and each tile stopped at is stored as a node
 *in the list. To draw the light during runtime, you traverse the list, checking
 *at each tile that it isn't of the type that can obscure light. If it is, you
 *keep traversing the list until you hit a node with a marker LIGHT_NEW_RAY,
 *which means you're back at the origin, and have skipped the remainder of the
 *last ray.
 *
 * Written by Derek Beland, April 14, 1997
 *
 ***************************************************************************************/
#include <algorithm>
#include <stdexcept>
#include <stdio.h>

#include "Editor/EditSys.h"
#include "Macro.h"
#include "SGP/Buffer.h"
#include "SGP/Debug.h"
#include "SGP/FileMan.h"
#include "SGP/HImage.h"
#include "SGP/Input.h"
#include "SGP/Line.h"
#include "SGP/MemMan.h"
#include "SGP/VObject.h"
#include "SGP/WCheck.h"
#include "SysGlobals.h"
#include "Tactical/AnimationData.h"
#include "Tactical/Overhead.h"
#include "Tactical/PathAI.h"
#include "Tactical/RottingCorpses.h"
#include "Tactical/StructureWrap.h"
#include "TileEngine/Environment.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/Lighting.h"
#include "TileEngine/RadarScreen.h"
#include "TileEngine/RenderDirty.h"
#include "TileEngine/RenderWorld.h"
#include "TileEngine/Structure.h"
#include "TileEngine/SysUtil.h"
#include "TileEngine/TileDef.h"
#include "TileEngine/WorldDef.h"
// #include "math.h"

#include "SDL_pixels.h"

#define MAX_LIGHT_TEMPLATES 32  // maximum number of light types

// stucture of node in linked list for lights
struct LIGHT_NODE {
  int16_t iDX;
  int16_t iDY;
  uint8_t uiFlags;
  uint8_t ubLight;
};

struct LightTemplate {
  LIGHT_NODE *lights;
  uint16_t *rays;
  uint16_t n_lights;
  uint16_t n_rays;
  char *name;
};

static LightTemplate g_light_templates[MAX_LIGHT_TEMPLATES];

#define FOR_EACH_LIGHT_TEMPLATE_SLOT(iter) FOR_EACH(LightTemplate, iter, g_light_templates)

#define FOR_EACH_LIGHT_TEMPLATE(iter) \
  FOR_EACH_LIGHT_TEMPLATE_SLOT(iter)  \
  if (!iter->lights)                  \
    continue;                         \
  else

// Sprite data
LIGHT_SPRITE LightSprites[MAX_LIGHT_SPRITES];

// Lighting system general data
uint8_t ubAmbientLightLevel = DEFAULT_SHADE_LEVEL;

SGPPaletteEntry g_light_color = {0, 0, 0, 0};

static SGPPaletteEntry gpOrigLight = {0, 0, 0, 0};

/*
uint16_t gusShadeLevels[16][3]={{500, 500, 500},				// green
table {450, 450, 450},		// bright {350, 350, 350}, {300, 300, 300},
                                                                                                                        {255, 255, 255},		// normal
                                                                                                                        {227, 227, 227},
                                                                                                                        {198, 198, 198},
                                                                                                                        {171, 171, 171},
                                                                                                                        {143, 143, 143},
                                                                                                                        {115, 115, 160},				// darkening
                                                                                                                        {87, 87, 176},
                                                                                                                        {60, 60, 160},
                                                                                                                        {48, 48, 192},
                                                                                                                        {36, 36, 208},
                                                                                                                        {18, 18, 224},
                                                                                                                        {48, 222, 48}};
*/
// Externed in Rotting Corpses.c
// Kris' attempt at blue night lights
/*
uint16_t gusShadeLevels[16][3]={{500, 500, 500},				// green
table {450, 450, 450},		// bright {350, 350, 350}, {300, 300, 300},
                                                                                                                        {255, 255, 255},		// normal
                                                                                                                        {215, 215, 227},
                                                                                                                        {179, 179, 179},
                                                                                                                        {149, 149, 149},
                                                                                                                        {125, 125, 128},
                                                                                                                        {104, 104, 128},				// darkening
                                                                                                                        {86, 86, 128},
                                                                                                                        {72, 72, 128},
                                                                                                                        {60, 60, 128},
                                                                                                                        {36, 36, 208},
                                                                                                                        {18, 18, 224},
                                                                                                                        {48, 222, 48}};
*/

/*
//Linda's final version

uint16_t gusShadeLevels[16][3] =
{
        500, 500, 500,
        450, 450, 450,	//bright
        350, 350, 350,
        300, 300, 300,
        255, 255, 255,	//normal
        222, 200, 200,
        174, 167, 167,
        150, 137, 137,
        122, 116, 116,	//darkening
        96, 96, 96,
        77, 77, 84,
        58, 58, 69,
        44, 44, 66,			//night
        36, 36, 244,
        18, 18, 224,
        48, 222, 48,
};
*/

// JA2 Gold:
static uint16_t gusShadeLevels[16][3] = {
    {500, 500, 500}, {450, 450, 450},                                    // bright
    {350, 350, 350}, {300, 300, 300}, {255, 255, 255},                   // normal
    {231, 199, 199}, {209, 185, 185}, {187, 171, 171}, {165, 157, 157},  // darkening
    {143, 143, 143}, {121, 121, 129}, {99, 99, 115},   {77, 77, 101},    // night
    {36, 36, 244},   {18, 18, 224},   {48, 222, 48}};

// Set this true if you want the shadetables to be loaded from the text file.
BOOLEAN gfLoadShadeTablesFromTextFile = FALSE;

void LoadShadeTablesFromTextFile() {
  FILE *fp;
  int32_t i, j;
  int32_t num;
  if (gfLoadShadeTablesFromTextFile) {
    fp = fopen("ShadeTables.txt", "r");
    Assert(fp);
    if (fp) {
      for (i = 0; i < 16; i++) {
        for (j = 0; j < 3; j++) {
          char str[10];
          fscanf(fp, "%s", str);
          sscanf(str, "%d", &num);
          gusShadeLevels[i][j] = (uint16_t)num;
        }
      }
      fclose(fp);
    }
  }
}

static LightTemplate *LightLoad(const char *pFilename);

/****************************************************************************************
 InitLightingSystem

        Initializes the lighting system.

***************************************************************************************/
void InitLightingSystem() {
  LoadShadeTablesFromTextFile();

  // init all light lists
  memset(g_light_templates, 0, sizeof(g_light_templates));

  // init all light sprites
  memset(LightSprites, 0, sizeof(LightSprites));

  LightLoad("TRANSLUC.LHT");
}

// THIS MUST BE CALLED ONCE ALL SURFACE VIDEO OBJECTS HAVE BEEN LOADED!
void SetDefaultWorldLightingColors() {
  static const SGPPaletteEntry pPal = {0, 0, 0};
  LightSetColor(&pPal);
}

static BOOLEAN LightDelete(LightTemplate *);

/****************************************************************************************
 ShutdownLightingSystem

        Closes down the lighting system. Any lights that were created are
destroyed, and the memory attached to them freed up.

***************************************************************************************/
void ShutdownLightingSystem() {
  // free up all allocated light nodes
  FOR_EACH_LIGHT_TEMPLATE(t) { LightDelete(t); }
}

/****************************************************************************************
 LightReset

        Removes all currently active lights, without undrawing them.

***************************************************************************************/
void LightReset() {
  // reset all light lists
  FOR_EACH_LIGHT_TEMPLATE(t) { LightDelete(t); }

  // init all light sprites
  memset(LightSprites, 0, sizeof(LightSprites));

  LightLoad("TRANSLUC.LHT");

  // Loop through mercs and reset light value
  FOR_EACH_SOLDIER(s) s->light = 0;
}

/* Creates a new node, and appends it to the template list. The index into the
 * list is returned. */
static uint16_t LightCreateTemplateNode(LightTemplate *const t, const int16_t iX, const int16_t iY,
                                        const uint8_t ubLight) {
  const uint16_t n_lights = t->n_lights;
  Assert((t->lights == NULL) == (n_lights == 0));

  t->lights = REALLOC(t->lights, LIGHT_NODE, n_lights + 1);

  LIGHT_NODE *const l = &t->lights[n_lights];
  l->iDX = iX;
  l->iDY = iY;
  l->ubLight = ubLight;
  l->uiFlags = 0;

  t->n_lights = n_lights + 1;
  return n_lights;
}

/* Adds a node to the template list. If the node does not exist, it creates a
 * new one.  Returns the index into the list. */
static uint16_t LightAddTemplateNode(LightTemplate *const t, const int16_t iX, const int16_t iY,
                                     const uint8_t ubLight) {
  for (uint16_t i = 0; i < t->n_lights; ++i) {
    if (t->lights[i].iDX == iX && t->lights[i].iDY == iY) return i;
  }
  return LightCreateTemplateNode(t, iX, iY, ubLight);
}

// Adds a node to the ray casting list.
static uint16_t LightAddRayNode(LightTemplate *const t, const int16_t iX, const int16_t iY,
                                const uint8_t ubLight, const uint16_t usFlags) {
  const uint16_t n_rays = t->n_rays;
  Assert((t->rays == NULL) == (n_rays == 0));

  t->rays = REALLOC(t->rays, uint16_t, n_rays + 1);

  t->rays[n_rays] = LightAddTemplateNode(t, iX, iY, ubLight) | usFlags;
  t->n_rays = n_rays + 1;
  return n_rays;
}

// Adds a node to the ray casting list.
static uint16_t LightInsertRayNode(LightTemplate *const t, const uint16_t usIndex, const int16_t iX,
                                   const int16_t iY, const uint8_t ubLight,
                                   const uint16_t usFlags) {
  const uint16_t n_rays = t->n_rays;
  Assert((t->rays == NULL) == (n_rays == 0));
  Assert(usIndex <= n_rays);

  t->rays = REALLOC(t->rays, uint16_t, n_rays + 1);

  memmove(t->rays + usIndex + 1, t->rays + usIndex, (n_rays - usIndex) * sizeof(*t->rays));

  t->rays[usIndex] = LightAddTemplateNode(t, iX, iY, ubLight) | usFlags;
  t->n_rays = n_rays + 1;
  return n_rays;
}

static BOOLEAN LightTileHasWall(int16_t iSrcX, int16_t iSrcY, int16_t iX, int16_t iY);

// Returns TRUE/FALSE if the tile at the specified tile number can block light.
static BOOLEAN LightTileBlocked(int16_t iSrcX, int16_t iSrcY, int16_t iX, int16_t iY) {
  uint16_t usTileNo, usSrcTileNo;

  Assert(gpWorldLevelData != NULL);

  usTileNo = MAPROWCOLTOPOS(iY, iX);
  usSrcTileNo = MAPROWCOLTOPOS(iSrcY, iSrcX);

  if (usTileNo >= NOWHERE) {
    return (FALSE);
  }

  if (usSrcTileNo >= NOWHERE) {
    return (FALSE);
  }

  if (gpWorldLevelData[usTileNo].sHeight > gpWorldLevelData[usSrcTileNo].sHeight) return (TRUE);
  {
    uint16_t usTileNo;
    LEVELNODE *pStruct;

    usTileNo = MAPROWCOLTOPOS(iY, iX);

    pStruct = gpWorldLevelData[usTileNo].pStructHead;
    if (pStruct != NULL) {
      // IF WE ARE A WINDOW, DO NOT BLOCK!
      if (FindStructure(usTileNo, STRUCTURE_WALLNWINDOW) != NULL) {
        return (FALSE);
      }
    }
  }

  return (LightTileHasWall(iSrcX, iSrcY, iX, iY));
}

// Returns TRUE/FALSE if the tile at the specified coordinates contains a wall.
static BOOLEAN LightTileHasWall(int16_t iSrcX, int16_t iSrcY, int16_t iX, int16_t iY) {
  // LEVELNODE *pStruct;
  uint16_t usTileNo;
  uint16_t usSrcTileNo;
  int8_t bDirection;
  uint8_t ubTravelCost;
  // int8_t		bWallCount = 0;

  Assert(gpWorldLevelData != NULL);

  usTileNo = MAPROWCOLTOPOS(iY, iX);
  usSrcTileNo = MAPROWCOLTOPOS(iSrcY, iSrcX);

  if (usTileNo == usSrcTileNo) {
    return (FALSE);
  }

  // if ( usTileNo == 10125 || usTileNo == 10126 )
  //{
  //	int i = 0;
  //}

  if (usTileNo >= NOWHERE) {
    return (FALSE);
  }

  if (usSrcTileNo >= NOWHERE) {
    return (FALSE);
  }

  // Get direction
  // bDirection = atan8( iX, iY, iSrcX, iSrcY );
  bDirection = atan8(iSrcX, iSrcY, iX, iY);

  ubTravelCost = gubWorldMovementCosts[usTileNo][bDirection][0];

  if (ubTravelCost == TRAVELCOST_WALL) {
    return (TRUE);
  }

  if (IS_TRAVELCOST_DOOR(ubTravelCost)) {
    ubTravelCost = DoorTravelCost(NULL, usTileNo, ubTravelCost, TRUE, NULL);

    if (ubTravelCost == TRAVELCOST_OBSTACLE || ubTravelCost == TRAVELCOST_DOOR) {
      return (TRUE);
    }
  }

#if 0
	uint16_t usWallOrientation;
	pStruct = gpWorldLevelData[ usTileNo ].pStructHead;
	while ( pStruct != NULL )
	{
		if ( pStruct->usIndex < NUMBEROFTILES )
		{
			const uint32_t uiType = GetTileType(pStruct->usIndex);

			// ATE: Changed to use last decordations rather than last decal
			// Could maybe check orientation value? Depends on our
			// use of the orientation value flags..
			if((uiType >= FIRSTWALL) && (uiType <=LASTDECORATIONS ))
			{
				usWallOrientation = GetWallOrientation(pStruct->usIndex);
				bWallCount++;
			}
		}

		pStruct=pStruct->pNext;
	}

	if ( bWallCount )
	{
		// ATE: If TWO or more - assume it's BLOCKED and return TRUE
		if ( bWallCount != 1 )
		{
			return( TRUE );
		}

		switch(usWallOrientation)
		{
			case INSIDE_TOP_RIGHT:
			case OUTSIDE_TOP_RIGHT:
				return( iSrcX < iX );

			case INSIDE_TOP_LEFT:
			case OUTSIDE_TOP_LEFT:
				return( iSrcY < iY );

		}
	}

#endif

  return (FALSE);
}

// Removes a light template from the list, and frees up the associated node
// memory.
static BOOLEAN LightDelete(LightTemplate *const t) {
  if (t->lights == NULL) return FALSE;

  MemFree(t->lights);
  t->lights = NULL;

  if (t->rays != NULL) {
    MemFree(t->rays);
    t->rays = NULL;
  }

  if (t->name != NULL) {
    MemFree(t->name);
    t->name = NULL;
  }

  t->n_lights = 0;
  t->n_rays = 0;
  return TRUE;
}

/* Returns an available slot for a new light template. */
static LightTemplate *LightGetFree() {
  FOR_EACH_LIGHT_TEMPLATE_SLOT(t) {
    if (!t->lights) return t;
  }
  throw std::runtime_error("Out of light template slots");
}

/* Calculates the 2D linear distance between two points. Returns the result in
 * a double for greater accuracy. */
static double LinearDistanceDouble(int16_t iX1, int16_t iY1, int16_t iX2, int16_t iY2) {
  int32_t iDx, iDy;

  iDx = abs(iX1 - iX2);
  iDx *= iDx;
  iDy = abs(iY1 - iY2);
  iDy *= iDy;

  return (sqrt((double)(iDx + iDy)));
}

/****************************************************************************************
        LightTrueLevel

                Returns the light level at a particular level without fake
lights

***************************************************************************************/
uint8_t LightTrueLevel(int16_t sGridNo, int8_t bLevel) {
  LEVELNODE *pNode;
  int32_t iSum;

  if (bLevel == 0) {
    pNode = gpWorldLevelData[sGridNo].pLandHead;
  } else {
    pNode = gpWorldLevelData[sGridNo].pRoofHead;
  }

  if (pNode == NULL) {
    return (ubAmbientLightLevel);
  } else {
    iSum = pNode->ubNaturalShadeLevel - (pNode->ubSumLights - pNode->ubFakeShadeLevel);

    iSum = std::min(SHADE_MIN, iSum);
    iSum = std::max(SHADE_MAX, iSum);
    return ((uint8_t)iSum);
  }
}

// Does the addition of light values to individual LEVELNODEs in the world tile
// list.
static void LightAddTileNode(LEVELNODE *const pNode, const uint8_t ubShadeAdd,
                             const BOOLEAN fFake) {
  int16_t sSum;

  pNode->ubSumLights += ubShadeAdd;
  if (fFake) {
    pNode->ubFakeShadeLevel += ubShadeAdd;
  }

  // Now set max
  pNode->ubMaxLights = std::max(pNode->ubMaxLights, ubShadeAdd);

  sSum = pNode->ubNaturalShadeLevel - pNode->ubMaxLights;

  sSum = std::min((int16_t)SHADE_MIN, sSum);
  sSum = std::max((int16_t)SHADE_MAX, sSum);

  pNode->ubShadeLevel = (uint8_t)sSum;
}

// Does the subtraction of light values to individual LEVELNODEs in the world
// tile list.
static void LightSubtractTileNode(LEVELNODE *const pNode, const uint8_t ubShadeSubtract,
                                  const BOOLEAN fFake) {
  int16_t sSum;

  if (ubShadeSubtract > pNode->ubSumLights) {
    pNode->ubSumLights = 0;
  } else {
    pNode->ubSumLights -= ubShadeSubtract;
  }
  if (fFake) {
    if (ubShadeSubtract > pNode->ubFakeShadeLevel) {
      pNode->ubFakeShadeLevel = 0;
    } else {
      pNode->ubFakeShadeLevel -= ubShadeSubtract;
    }
  }

  // Now set max
  pNode->ubMaxLights = std::min(pNode->ubMaxLights, pNode->ubSumLights);

  sSum = pNode->ubNaturalShadeLevel - pNode->ubMaxLights;

  sSum = std::min((int16_t)SHADE_MIN, sSum);
  sSum = std::max((int16_t)SHADE_MAX, sSum);

  pNode->ubShadeLevel = (uint8_t)sSum;
}

static BOOLEAN LightIlluminateWall(int16_t iSourceX, int16_t iSourceY, int16_t iTileX,
                                   int16_t iTileY, LEVELNODE *pStruct);

// Adds a specified amount of light to all objects on a given tile.
static BOOLEAN LightAddTile(const int16_t iSrcX, const int16_t iSrcY, const int16_t iX,
                            const int16_t iY, const uint8_t ubShade, const uint32_t uiFlags,
                            const BOOLEAN fOnlyWalls) {
  LEVELNODE *pLand, *pStruct, *pObject, *pMerc, *pRoof, *pOnRoof;
  uint8_t ubShadeAdd;
  uint32_t uiTile;
  BOOLEAN fLitWall = FALSE;
  BOOLEAN fFake;

  Assert(gpWorldLevelData != NULL);

  uiTile = MAPROWCOLTOPOS(iY, iX);

  if (uiTile >= NOWHERE) {
    return (FALSE);
  }

  gpWorldLevelData[uiTile].uiFlags |= MAPELEMENT_REDRAW;

  // if((uiFlags&LIGHT_BACKLIGHT) && !(uiFlags&LIGHT_ROOF_ONLY))
  //	ubShadeAdd=ubShade*7/10;
  // else
  ubShadeAdd = ubShade;

  if (uiFlags & LIGHT_FAKE) {
    fFake = TRUE;
  } else {
    fFake = FALSE;
  }

  if (!(uiFlags & LIGHT_ROOF_ONLY) || (uiFlags & LIGHT_EVERYTHING)) {
    pStruct = gpWorldLevelData[uiTile].pStructHead;
    while (pStruct != NULL) {
      if (pStruct->usIndex < NUMBEROFTILES) {
        if ((gTileDatabase[pStruct->usIndex].fType != FIRSTCLIFFHANG) ||
            (uiFlags & LIGHT_EVERYTHING)) {
          if ((uiFlags & LIGHT_IGNORE_WALLS) || gfCaves)
            LightAddTileNode(pStruct, ubShadeAdd, FALSE);
          else if (LightIlluminateWall(iSrcX, iSrcY, iX, iY, pStruct)) {
            if (LightTileHasWall(iSrcX, iSrcY, iX, iY)) fLitWall = TRUE;

            // ATE: Limit shade for walls if in caves
            if (fLitWall && gfCaves) {
              LightAddTileNode(pStruct, std::min(ubShadeAdd, (uint8_t)(SHADE_MAX + 5)), FALSE);
            } else if (fLitWall) {
              LightAddTileNode(pStruct, ubShadeAdd, FALSE);
            } else if (!fOnlyWalls) {
              LightAddTileNode(pStruct, ubShadeAdd, FALSE);
            }
          }
        }
      } else {
        LightAddTileNode(pStruct, ubShadeAdd, FALSE);
      }
      pStruct = pStruct->pNext;
    }

    ubShadeAdd = ubShade;

    if (!fOnlyWalls) {
      pLand = gpWorldLevelData[uiTile].pLandHead;

      while (pLand) {
        if (gfCaves || !fLitWall) {
          LightAddTileNode(pLand, ubShadeAdd, fFake);
        }
        pLand = pLand->pNext;
      }

      pObject = gpWorldLevelData[uiTile].pObjectHead;
      while (pObject != NULL) {
        if (pObject->usIndex < NUMBEROFTILES) {
          LightAddTileNode(pObject, ubShadeAdd, FALSE);
        }
        pObject = pObject->pNext;
      }

      if (uiFlags & LIGHT_BACKLIGHT) ubShadeAdd = (int16_t)ubShade * 7 / 10;

      pMerc = gpWorldLevelData[uiTile].pMercHead;
      while (pMerc != NULL) {
        LightAddTileNode(pMerc, ubShadeAdd, FALSE);
        pMerc = pMerc->pNext;
      }
    }
  }

  if ((uiFlags & LIGHT_ROOF_ONLY) || (uiFlags & LIGHT_EVERYTHING)) {
    pRoof = gpWorldLevelData[uiTile].pRoofHead;
    while (pRoof != NULL) {
      if (pRoof->usIndex < NUMBEROFTILES) {
        LightAddTileNode(pRoof, ubShadeAdd, fFake);
      }
      pRoof = pRoof->pNext;
    }

    pOnRoof = gpWorldLevelData[uiTile].pOnRoofHead;
    while (pOnRoof != NULL) {
      LightAddTileNode(pOnRoof, ubShadeAdd, FALSE);

      pOnRoof = pOnRoof->pNext;
    }
  }
  return (TRUE);
}

// Subtracts a specified amount of light to a given tile.
static BOOLEAN LightSubtractTile(const int16_t iSrcX, const int16_t iSrcY, const int16_t iX,
                                 const int16_t iY, const uint8_t ubShade, const uint32_t uiFlags,
                                 const BOOLEAN fOnlyWalls) {
  LEVELNODE *pLand, *pStruct, *pObject, *pMerc, *pRoof, *pOnRoof;
  uint8_t ubShadeSubtract;
  uint32_t uiTile;
  BOOLEAN fLitWall = FALSE;
  BOOLEAN fFake;  // only passed in to land and roof layers; others get fed FALSE

  Assert(gpWorldLevelData != NULL);

  uiTile = MAPROWCOLTOPOS(iY, iX);

  if (uiTile >= NOWHERE) {
    return (FALSE);
  }

  gpWorldLevelData[uiTile].uiFlags |= MAPELEMENT_REDRAW;

  //	if((uiFlags&LIGHT_BACKLIGHT) && !(uiFlags&LIGHT_ROOF_ONLY))
  //		ubShadeSubtract=ubShade*7/10;
  //	else
  ubShadeSubtract = ubShade;

  if (uiFlags & LIGHT_FAKE) {
    fFake = TRUE;
  } else {
    fFake = FALSE;
  }

  if (!(uiFlags & LIGHT_ROOF_ONLY) || (uiFlags & LIGHT_EVERYTHING)) {
    pStruct = gpWorldLevelData[uiTile].pStructHead;
    while (pStruct != NULL) {
      if (pStruct->usIndex < NUMBEROFTILES) {
        if ((gTileDatabase[pStruct->usIndex].fType != FIRSTCLIFFHANG) ||
            (uiFlags & LIGHT_EVERYTHING)) {
          if ((uiFlags & LIGHT_IGNORE_WALLS) || gfCaves)
            LightSubtractTileNode(pStruct, ubShadeSubtract, FALSE);
          else if (LightIlluminateWall(iSrcX, iSrcY, iX, iY, pStruct)) {
            if (LightTileHasWall(iSrcX, iSrcY, iX, iY)) fLitWall = TRUE;

            // ATE: Limit shade for walls if in caves
            if (fLitWall && gfCaves) {
              LightSubtractTileNode(pStruct, std::max(ubShadeSubtract - 5, 0), FALSE);
            } else if (fLitWall) {
              LightSubtractTileNode(pStruct, ubShadeSubtract, FALSE);
            } else if (!fOnlyWalls) {
              LightSubtractTileNode(pStruct, ubShadeSubtract, FALSE);
            }
          }
        }
      } else {
        LightSubtractTileNode(pStruct, ubShadeSubtract, FALSE);
      }
      pStruct = pStruct->pNext;
    }

    ubShadeSubtract = ubShade;

    if (!fOnlyWalls) {
      pLand = gpWorldLevelData[uiTile].pLandHead;

      while (pLand) {
        if (gfCaves || !fLitWall) {
          LightSubtractTileNode(pLand, ubShadeSubtract, fFake);
        }
        pLand = pLand->pNext;
      }

      pObject = gpWorldLevelData[uiTile].pObjectHead;
      while (pObject != NULL) {
        if (pObject->usIndex < NUMBEROFTILES) {
          LightSubtractTileNode(pObject, ubShadeSubtract, FALSE);
        }
        pObject = pObject->pNext;
      }

      if (uiFlags & LIGHT_BACKLIGHT) ubShadeSubtract = (int16_t)ubShade * 7 / 10;

      pMerc = gpWorldLevelData[uiTile].pMercHead;
      while (pMerc != NULL) {
        LightSubtractTileNode(pMerc, ubShadeSubtract, FALSE);
        pMerc = pMerc->pNext;
      }
    }
  }

  if ((uiFlags & LIGHT_ROOF_ONLY) || (uiFlags & LIGHT_EVERYTHING)) {
    pRoof = gpWorldLevelData[uiTile].pRoofHead;
    while (pRoof != NULL) {
      if (pRoof->usIndex < NUMBEROFTILES) {
        LightSubtractTileNode(pRoof, ubShadeSubtract, fFake);
      }
      pRoof = pRoof->pNext;
    }

    pOnRoof = gpWorldLevelData[uiTile].pOnRoofHead;
    while (pOnRoof != NULL) {
      if (pOnRoof->usIndex < NUMBEROFTILES) {
        LightSubtractTileNode(pOnRoof, ubShadeSubtract, FALSE);
      }
      pOnRoof = pOnRoof->pNext;
    }
  }

  return (TRUE);
}

/* Set the natural light level (as well as the current) on all LEVELNODEs on a
 * level. */
static void LightSetNaturalLevel(LEVELNODE *n, uint8_t const shade) {
  for (; n; n = n->pNext) {
    n->ubSumLights = 0;
    n->ubMaxLights = 0;
    n->ubNaturalShadeLevel = shade;
    n->ubShadeLevel = shade;
  }
}

/* Set the natural light value of all objects on a given tile to the specified
 * value.  This is the light value a tile has with no artificial lighting
 * affecting it. */
static void LightSetNaturalTile(MAP_ELEMENT const &e, uint8_t shade) {
  LightSetNaturalLevel(e.pLandHead, shade);
  LightSetNaturalLevel(e.pObjectHead, shade);
  LightSetNaturalLevel(e.pStructHead, shade);
  LightSetNaturalLevel(e.pMercHead, shade);
  LightSetNaturalLevel(e.pRoofHead, shade);
  LightSetNaturalLevel(e.pOnRoofHead, shade);
  LightSetNaturalLevel(e.pTopmostHead, shade);
}

/* Reset the light level of all LEVELNODEs on a level to the value contained in
 * the natural light level. */
static void LightResetLevel(LEVELNODE *n) {
  for (; n; n = n->pNext) {
    n->ubSumLights = 0;
    n->ubMaxLights = 0;
    n->ubShadeLevel = n->ubNaturalShadeLevel;
    n->ubFakeShadeLevel = 0;
  }
}

// Reset all tiles on the map to their baseline values.
static void LightResetAllTiles() {
  FOR_EACH_WORLD_TILE(i) {
    LightResetLevel(i->pLandHead);
    LightResetLevel(i->pObjectHead);
    LightResetLevel(i->pStructHead);
    LightResetLevel(i->pMercHead);
    LightResetLevel(i->pRoofHead);
    LightResetLevel(i->pOnRoofHead);
    LightResetLevel(i->pTopmostHead);
  }
}

// Creates a new node, and adds it to the end of a light list.
static void LightAddNode(LightTemplate *const t, const int16_t iHotSpotX, const int16_t iHotSpotY,
                         int16_t iX, int16_t iY, const uint8_t ubIntensity,
                         const uint16_t uiFlags) {
  double dDistance;
  uint8_t ubShade;
  int32_t iLightDecay;

  dDistance = LinearDistanceDouble(iX, iY, iHotSpotX, iHotSpotY);
  dDistance /= DISTANCE_SCALE;

  iLightDecay = (int32_t)(dDistance * LIGHT_DECAY);

  if ((iLightDecay >= (int32_t)ubIntensity))
    ubShade = 0;
  else
    ubShade = ubIntensity - (uint8_t)iLightDecay;

  iX /= DISTANCE_SCALE;
  iY /= DISTANCE_SCALE;

  LightAddRayNode(t, iX, iY, ubShade, uiFlags);
}

// Creates a new node, and inserts it after the specified node.
static void LightInsertNode(LightTemplate *const t, const uint16_t usLightIns,
                            const int16_t iHotSpotX, const int16_t iHotSpotY, int16_t iX,
                            int16_t iY, const uint8_t ubIntensity, const uint16_t uiFlags) {
  double dDistance;
  uint8_t ubShade;
  int32_t iLightDecay;

  dDistance = LinearDistanceDouble(iX, iY, iHotSpotX, iHotSpotY);
  dDistance /= DISTANCE_SCALE;

  iLightDecay = (int32_t)(dDistance * LIGHT_DECAY);

  if ((iLightDecay >= (int32_t)ubIntensity))
    ubShade = 0;
  else
    ubShade = ubIntensity - (uint8_t)iLightDecay;

  iX /= DISTANCE_SCALE;
  iY /= DISTANCE_SCALE;

  LightInsertRayNode(t, usLightIns, iX, iY, ubShade, uiFlags);
}

/* Traverses the linked list until a node with the LIGHT_NEW_RAY marker, and
 * returns the pointer. If the end of list is reached, NULL is returned. */
static uint16_t LightFindNextRay(const LightTemplate *const t, const uint16_t usIndex) {
  uint16_t usNodeIndex = usIndex;
  while ((usNodeIndex < t->n_rays) && !(t->rays[usNodeIndex] & LIGHT_NEW_RAY)) usNodeIndex++;

  return (usNodeIndex);
}

/* Casts a ray from an origin to an end point, creating nodes and adding them
 * to the light list. */
static BOOLEAN LightCastRay(LightTemplate *const t, const int16_t iStartX, const int16_t iStartY,
                            const int16_t iEndPointX, const int16_t iEndPointY,
                            const uint8_t ubStartIntens, const uint8_t ubEndIntens) {
  int16_t AdjUp, AdjDown, ErrorTerm, XAdvance, XDelta, YDelta;
  int32_t WholeStep, InitialPixelCount, FinalPixelCount, i, j, RunLength;
  int16_t iXPos, iYPos, iEndY, iEndX;
  uint16_t usCurNode = 0, usFlags = 0;
  BOOLEAN fInsertNodes = FALSE;

  if ((iEndPointX > 0) && (iEndPointY > 0)) usFlags = LIGHT_BACKLIGHT;

  /* We'll always draw top to bottom, to reduce the number of cases we have to
  handle, and to make lines between the same endpoints draw the same pixels */
  if (iStartY > iEndPointY) {
    iXPos = iEndPointX;
    iEndX = iStartX;
    iYPos = iEndPointY;
    iEndY = iStartY;
    fInsertNodes = TRUE;
  } else {
    iXPos = iStartX;
    iEndX = iEndPointX;
    iYPos = iStartY;
    iEndY = iEndPointY;
  }

  /* Figure out whether we're going left or right, and how far we're
going horizontally */
  if ((XDelta = (iEndX - iXPos)) < 0) {
    XAdvance = -1;
    XDelta = -XDelta;
  } else {
    XAdvance = 1;
  }
  /* Figure out how far we're going vertically */
  YDelta = iEndY - iYPos;

  // Check for 0 length ray
  if ((XDelta == 0) && (YDelta == 0)) return (FALSE);

  // DebugMsg(TOPIC_GAME, DBG_LEVEL_0, String("Drawing (%d,%d) to (%d,%d)",
  // iXPos, iYPos, iEndX, iEndY));
  LightAddNode(t, 32767, 32767, 32767, 32767, 0, LIGHT_NEW_RAY);
  if (fInsertNodes) usCurNode = t->n_rays;

  /* Special-case horizontal, vertical, and diagonal lines, for speed
and to avoid nasty boundary conditions and division by 0 */
  if (XDelta == 0) {
    /* Vertical line */
    if (fInsertNodes) {
      for (i = 0; i <= YDelta; i++) {
        LightInsertNode(t, usCurNode, iStartX, iStartY, iXPos, iYPos, ubStartIntens, usFlags);
        iYPos++;
      }
    } else {
      for (i = 0; i <= YDelta; i++) {
        LightAddNode(t, iStartX, iStartY, iXPos, iYPos, ubStartIntens, usFlags);
        iYPos++;
      }
    }
    return (TRUE);
  }
  if (YDelta == 0) {
    /* Horizontal line */
    if (fInsertNodes) {
      for (i = 0; i <= XDelta; i++) {
        LightInsertNode(t, usCurNode, iStartX, iStartY, iXPos, iYPos, ubStartIntens, usFlags);
        iXPos += XAdvance;
      }
    } else {
      for (i = 0; i <= XDelta; i++) {
        LightAddNode(t, iStartX, iStartY, iXPos, iYPos, ubStartIntens, usFlags);
        iXPos += XAdvance;
      }
    }
    return (TRUE);
  }
  if (XDelta == YDelta) {
    /* Diagonal line */
    if (fInsertNodes) {
      for (i = 0; i <= XDelta; i++) {
        LightInsertNode(t, usCurNode, iStartX, iStartY, iXPos, iYPos, ubStartIntens, usFlags);
        iXPos += XAdvance;
        iYPos++;
      }
    } else {
      for (i = 0; i <= XDelta; i++) {
        LightAddNode(t, iStartX, iStartY, iXPos, iYPos, ubStartIntens, usFlags);
        iXPos += XAdvance;
        iYPos++;
      }
    }
    return (TRUE);
  }

  /* Determine whether the line is X or Y major, and handle accordingly */
  if (XDelta >= YDelta) {
    /* X major line */
    /* Minimum # of pixels in a run in this line */
    WholeStep = XDelta / YDelta;

    /* Error term adjust each time Y steps by 1; used to tell when one
       extra pixel should be drawn as part of a run, to account for
       fractional steps along the X axis per 1-pixel steps along Y */
    AdjUp = (XDelta % YDelta) * 2;

    /* Error term adjust when the error term turns over, used to factor
       out the X step made at that time */
    AdjDown = YDelta * 2;

    /* Initial error term; reflects an initial step of 0.5 along the Y
       axis */
    ErrorTerm = (XDelta % YDelta) - (YDelta * 2);

    /* The initial and last runs are partial, because Y advances only 0.5
       for these runs, rather than 1. Divide one full run, plus the
       initial pixel, between the initial and last runs */
    InitialPixelCount = (WholeStep / 2) + 1;
    FinalPixelCount = InitialPixelCount;

    /* If the basic run length is even and there's no fractional
       advance, we have one pixel that could go to either the initial
       or last partial run, which we'll arbitrarily allocate to the
       last run */
    if ((AdjUp == 0) && ((WholeStep & 0x01) == 0)) {
      InitialPixelCount--;
    }
    /* If there're an odd number of pixels per run, we have 1 pixel that can't
       be allocated to either the initial or last partial run, so we'll add 0.5
       to error term so this pixel will be handled by the normal full-run loop
     */
    if ((WholeStep & 0x01) != 0) {
      ErrorTerm += YDelta;
    }
    /* Draw the first, partial run of pixels */
    // DrawHorizontalRun(&ScreenPtr, XAdvance, InitialPixelCount, Color);
    if (fInsertNodes) {
      for (i = 0; i < InitialPixelCount; i++) {
        LightInsertNode(t, usCurNode, iStartX, iStartY, iXPos, iYPos, ubStartIntens, usFlags);
        iXPos += XAdvance;
      }
    } else {
      for (i = 0; i < InitialPixelCount; i++) {
        LightAddNode(t, iStartX, iStartY, iXPos, iYPos, ubStartIntens, usFlags);
        iXPos += XAdvance;
      }
    }
    iYPos++;

    /* Draw all full runs */
    for (j = 0; j < (YDelta - 1); j++) {
      RunLength = WholeStep; /* run is at least this long */
      /* Advance the error term and add an extra pixel if the error
         term so indicates */
      if ((ErrorTerm += AdjUp) > 0) {
        RunLength++;
        ErrorTerm -= AdjDown; /* reset the error term */
      }
      /* Draw this scan line's run */
      // DrawHorizontalRun(&ScreenPtr, XAdvance, RunLength, Color);
      if (fInsertNodes) {
        for (i = 0; i < RunLength; i++) {
          LightInsertNode(t, usCurNode, iStartX, iStartY, iXPos, iYPos, ubStartIntens, usFlags);
          iXPos += XAdvance;
        }
      } else {
        for (i = 0; i < RunLength; i++) {
          LightAddNode(t, iStartX, iStartY, iXPos, iYPos, ubStartIntens, usFlags);
          iXPos += XAdvance;
        }
      }
      iYPos++;
    }
    /* Draw the final run of pixels */
    // DrawHorizontalRun(&ScreenPtr, XAdvance, FinalPixelCount, Color);
    if (fInsertNodes) {
      for (i = 0; i < FinalPixelCount; i++) {
        LightInsertNode(t, usCurNode, iStartX, iStartY, iXPos, iYPos, ubStartIntens, usFlags);
        iXPos += XAdvance;
      }
    } else {
      for (i = 0; i < FinalPixelCount; i++) {
        LightAddNode(t, iStartX, iStartY, iXPos, iYPos, ubStartIntens, usFlags);
        iXPos += XAdvance;
      }
    }
    iYPos++;
  } else {
    /* Y major line */

    /* Minimum # of pixels in a run in this line */
    WholeStep = YDelta / XDelta;

    /* Error term adjust each time X steps by 1; used to tell when 1 extra
       pixel should be drawn as part of a run, to account for
       fractional steps along the Y axis per 1-pixel steps along X */
    AdjUp = (YDelta % XDelta) * 2;

    /* Error term adjust when the error term turns over, used to factor
       out the Y step made at that time */
    AdjDown = XDelta * 2;

    /* Initial error term; reflects initial step of 0.5 along the X axis */
    ErrorTerm = (YDelta % XDelta) - (XDelta * 2);

    /* The initial and last runs are partial, because X advances only 0.5
       for these runs, rather than 1. Divide one full run, plus the
       initial pixel, between the initial and last runs */
    InitialPixelCount = (WholeStep / 2) + 1;
    FinalPixelCount = InitialPixelCount;

    /* If the basic run length is even and there's no fractional advance, we
       have 1 pixel that could go to either the initial or last partial run,
       which we'll arbitrarily allocate to the last run */
    if ((AdjUp == 0) && ((WholeStep & 0x01) == 0)) {
      InitialPixelCount--;
    }
    /* If there are an odd number of pixels per run, we have one pixel
       that can't be allocated to either the initial or last partial
       run, so we'll add 0.5 to the error term so this pixel will be
       handled by the normal full-run loop */
    if ((WholeStep & 0x01) != 0) {
      ErrorTerm += XDelta;
    }
    /* Draw the first, partial run of pixels */
    if (fInsertNodes) {
      for (i = 0; i < InitialPixelCount; i++) {
        LightInsertNode(t, usCurNode, iStartX, iStartY, iXPos, iYPos, ubStartIntens, usFlags);
        iYPos++;
      }
    } else {
      for (i = 0; i < InitialPixelCount; i++) {
        LightAddNode(t, iStartX, iStartY, iXPos, iYPos, ubStartIntens, usFlags);
        iYPos++;
      }
    }
    iXPos += XAdvance;
    // DrawVerticalRun(&ScreenPtr, XAdvance, InitialPixelCount, Color);

    /* Draw all full runs */
    for (j = 0; j < (XDelta - 1); j++) {
      RunLength = WholeStep; /* run is at least this long */
      /* Advance the error term and add an extra pixel if the error
         term so indicates */
      if ((ErrorTerm += AdjUp) > 0) {
        RunLength++;
        ErrorTerm -= AdjDown; /* reset the error term */
      }
      /* Draw this scan line's run */
      // DrawVerticalRun(&ScreenPtr, XAdvance, RunLength, Color);
      if (fInsertNodes) {
        for (i = 0; i < RunLength; i++) {
          LightInsertNode(t, usCurNode, iStartX, iStartY, iXPos, iYPos, ubStartIntens, usFlags);
          iYPos++;
        }
      } else {
        for (i = 0; i < RunLength; i++) {
          LightAddNode(t, iStartX, iStartY, iXPos, iYPos, ubStartIntens, usFlags);
          iYPos++;
        }
      }
      iXPos += XAdvance;
    }
    /* Draw the final run of pixels */
    // DrawVerticalRun(&ScreenPtr, XAdvance, FinalPixelCount, Color);
    if (fInsertNodes) {
      for (i = 0; i < FinalPixelCount; i++) {
        LightInsertNode(t, usCurNode, iStartX, iStartY, iXPos, iYPos, ubStartIntens, usFlags);
        iYPos++;
      }
    } else {
      for (i = 0; i < FinalPixelCount; i++) {
        LightAddNode(t, iStartX, iStartY, iXPos, iYPos, ubStartIntens, usFlags);
        iYPos++;
      }
    }
    iXPos += XAdvance;
  }
  return (TRUE);
}

// Creates an elliptical light, taking two radii.
static void LightGenerateElliptical(LightTemplate *const t, const uint8_t iIntensity,
                                    const int16_t iA, const int16_t iB) {
  int16_t iX, iY;
  int32_t WorkingX, WorkingY;
  double ASquared;
  double BSquared;
  double Temp;

  iX = 0;
  iY = 0;
  ASquared = (double)iA * iA;
  BSquared = (double)iB * iB;

  /* Draw the four symmetric arcs for which X advances faster (that is,
     for which X is the major axis) */
  /* Draw the initial top & bottom points */
  LightCastRay(t, iX, iY, (int16_t)iX, (int16_t)(iY + iB), iIntensity, 1);
  LightCastRay(t, iX, iY, (int16_t)iX, (int16_t)(iY - iB), iIntensity, 1);

  /* Draw the four arcs */
  for (WorkingX = 0;;) {
    /* Advance one pixel along the X axis */
    WorkingX++;

    /* Calculate the corresponding point along the Y axis. Guard
       against floating-point roundoff making the intermediate term
       less than 0 */
    Temp = BSquared - (BSquared * WorkingX * WorkingX / ASquared);

    if (Temp >= 0)
      WorkingY = (int32_t)(sqrt(Temp) + 0.5);
    else
      WorkingY = 0;

    /* Stop if X is no longer the major axis (the arc has passed the
       45-degree point) */
    if (((double)WorkingY / BSquared) <= ((double)WorkingX / ASquared)) break;

    /* Draw the 4 symmetries of the current point */
    LightCastRay(t, iX, iY, (int16_t)(iX + WorkingX), (int16_t)(iY - WorkingY), iIntensity, 1);
    LightCastRay(t, iX, iY, (int16_t)(iX - WorkingX), (int16_t)(iY - WorkingY), iIntensity, 1);
    LightCastRay(t, iX, iY, (int16_t)(iX + WorkingX), (int16_t)(iY + WorkingY), iIntensity, 1);
    LightCastRay(t, iX, iY, (int16_t)(iX - WorkingX), (int16_t)(iY + WorkingY), iIntensity, 1);
  }

  /* Draw the four symmetric arcs for which Y advances faster (that is,
     for which Y is the major axis) */
  /* Draw the initial left & right points */
  LightCastRay(t, iX, iY, (int16_t)(iX + iA), iY, iIntensity, 1);
  LightCastRay(t, iX, iY, (int16_t)(iX - iA), iY, iIntensity, 1);

  /* Draw the four arcs */
  for (WorkingY = 0;;) {
    /* Advance one pixel along the Y axis */
    WorkingY++;

    /* Calculate the corresponding point along the X axis. Guard
       against floating-point roundoff making the intermediate term
       less than 0 */
    Temp = ASquared - (ASquared * WorkingY * WorkingY / BSquared);

    if (Temp >= 0)
      WorkingX = (int32_t)(sqrt(Temp) + 0.5);
    else
      WorkingX = 0;

    /* Stop if Y is no longer the major axis (the arc has passed the
       45-degree point) */
    if (((double)WorkingX / ASquared) < ((double)WorkingY / BSquared)) break;

    /* Draw the 4 symmetries of the current point */
    LightCastRay(t, iX, iY, (int16_t)(iX + WorkingX), (int16_t)(iY - WorkingY), iIntensity, 1);
    LightCastRay(t, iX, iY, (int16_t)(iX - WorkingX), (int16_t)(iY - WorkingY), iIntensity, 1);
    LightCastRay(t, iX, iY, (int16_t)(iX + WorkingX), (int16_t)(iY + WorkingY), iIntensity, 1);
    LightCastRay(t, iX, iY, (int16_t)(iX - WorkingX), (int16_t)(iY + WorkingY), iIntensity, 1);
  }
}

// Creates an square light, taking two radii.
static void LightGenerateSquare(LightTemplate *const t, const uint8_t iIntensity, const int16_t iA,
                                const int16_t iB) {
  int16_t iX, iY;

  for (iX = 0 - iA; iX <= 0 + iA; iX++) LightCastRay(t, 0, 0, iX, (int16_t)(0 - iB), iIntensity, 1);

  for (iX = 0 - iA; iX <= 0 + iA; iX++) LightCastRay(t, 0, 0, iX, (int16_t)(0 + iB), iIntensity, 1);

  for (iY = 0 - iB; iY <= 0 + iB; iY++) LightCastRay(t, 0, 0, (int16_t)(0 - iA), iY, iIntensity, 1);

  for (iY = 0 - iB; iY <= 0 + iB; iY++) LightCastRay(t, 0, 0, (int16_t)(0 + iA), iY, iIntensity, 1);

  /*for(iY=0-iB; iY <= 0+iB; iY++)
          LightCastRay(t, 0, iY, (int16_t)(0+iA), iY, iIntensity, 1);

  for(iY=0+iB; iY >= 0-iB; iY--)
          LightCastRay(t, 0, iY, (int16_t)(0-iA), iY, iIntensity, 1);

  for(iX=0-iA; iX <= 0+iA; iX++)
          LightCastRay(t, iX, 0, iX, (int16_t)(0+iB), iIntensity, 1);

  for(iX=0+iA; iX >= 0-iA; iX--)
          LightCastRay(t, iX, 0, iX, (int16_t)(0-iB), iIntensity, 1); */
}

/****************************************************************************************
        LightSetBaseLevel

                Sets the current and natural light settings for all tiles in the
world.

***************************************************************************************/
void LightSetBaseLevel(uint8_t iIntensity) {
  ubAmbientLightLevel = iIntensity;

  if (!gfEditMode) {
    // Loop for all good guys in tactical map and add a light if required
    FOR_EACH_MERC(i) {
      SOLDIERTYPE *const s = *i;
      if (s->bTeam == OUR_TEAM) ReCreateSoldierLight(s);
    }
  }

  uint16_t shade = iIntensity;
  shade = std::max((uint16_t)SHADE_MAX, (uint16_t)shade);
  shade = std::min((uint16_t)SHADE_MIN, (uint16_t)shade);
  FOR_EACH_WORLD_TILE(i) { LightSetNaturalTile(*i, shade); }

  LightSpriteRenderAll();

  if (iIntensity >= LIGHT_DUSK_CUTOFF)
    RenderSetShadows(FALSE);
  else
    RenderSetShadows(TRUE);
}

void LightAddBaseLevel(const uint8_t iIntensity) {
  int16_t iCountY, iCountX;

  ubAmbientLightLevel = std::max(SHADE_MAX, ubAmbientLightLevel - iIntensity);

  for (iCountY = 0; iCountY < WORLD_ROWS; iCountY++)
    for (iCountX = 0; iCountX < WORLD_COLS; iCountX++)
      LightAddTile(iCountX, iCountY, iCountX, iCountY, iIntensity,
                   LIGHT_IGNORE_WALLS | LIGHT_EVERYTHING, FALSE);

  if (ubAmbientLightLevel >= LIGHT_DUSK_CUTOFF)
    RenderSetShadows(FALSE);
  else
    RenderSetShadows(TRUE);
}

void LightSubtractBaseLevel(const uint8_t iIntensity) {
  int16_t iCountY, iCountX;

  ubAmbientLightLevel = std::min(SHADE_MIN, ubAmbientLightLevel + iIntensity);

  for (iCountY = 0; iCountY < WORLD_ROWS; iCountY++)
    for (iCountX = 0; iCountX < WORLD_COLS; iCountX++)
      LightSubtractTile(iCountX, iCountY, iCountX, iCountY, iIntensity,
                        LIGHT_IGNORE_WALLS | LIGHT_EVERYTHING, FALSE);

  if (ubAmbientLightLevel >= LIGHT_DUSK_CUTOFF)
    RenderSetShadows(FALSE);
  else
    RenderSetShadows(TRUE);
}

LightTemplate *LightCreateOmni(const uint8_t ubIntensity, const int16_t iRadius) {
  LightTemplate *const t = LightGetFree();

  LightGenerateElliptical(t, ubIntensity, iRadius * DISTANCE_SCALE, iRadius * DISTANCE_SCALE);

  char usName[14];
  sprintf(usName, "LTO%d.LHT", iRadius);
  t->name = MALLOCN(char, strlen(usName) + 1);
  strcpy(t->name, usName);

  return t;
}

// Creates a square light
static LightTemplate *LightCreateSquare(uint8_t ubIntensity, int16_t iRadius1, int16_t iRadius2) {
  LightTemplate *const t = LightGetFree();

  LightGenerateSquare(t, ubIntensity, iRadius1 * DISTANCE_SCALE, iRadius2 * DISTANCE_SCALE);

  char usName[14];
  sprintf(usName, "LTS%d-%d.LHT", iRadius1, iRadius2);
  t->name = MALLOCN(char, strlen(usName) + 1);
  strcpy(t->name, usName);

  return t;
}

// Creates an elliptical light (two separate radii)
static LightTemplate *LightCreateElliptical(uint8_t ubIntensity, int16_t iRadius1,
                                            int16_t iRadius2) {
  LightTemplate *const t = LightGetFree();

  LightGenerateElliptical(t, ubIntensity, iRadius1 * DISTANCE_SCALE, iRadius2 * DISTANCE_SCALE);

  char usName[14];
  sprintf(usName, "LTE%d-%d.LHT", iRadius1, iRadius2);
  t->name = MALLOCN(char, strlen(usName) + 1);
  strcpy(t->name, usName);

  return t;
}

// Renders a light template at the specified X,Y coordinates.
static BOOLEAN LightIlluminateWall(int16_t iSourceX, int16_t iSourceY, int16_t iTileX,
                                   int16_t iTileY, LEVELNODE *pStruct) {
  //	return( LightTileHasWall( iSourceX, iSourceY, iTileX, iTileY ) );

#if 0
	uint16_t usWallOrientation = GetWallOrientation(pStruct->usIndex);
	switch(usWallOrientation)
	{
		case NO_ORIENTATION:
			return(TRUE);

		case INSIDE_TOP_RIGHT:
		case OUTSIDE_TOP_RIGHT:
			return(iSourceX >= iTileX);

		case INSIDE_TOP_LEFT:
		case OUTSIDE_TOP_LEFT:
			return(iSourceY >= iTileY);

	}
	return(FALSE);

#endif

  return (TRUE);
}

BOOLEAN LightDraw(const LIGHT_SPRITE *const l) {
  uint32_t uiFlags;
  int32_t iOldX, iOldY;
  BOOLEAN fBlocked = FALSE;
  BOOLEAN fOnlyWalls;

  const LightTemplate *const t = l->light_template;
  if (t->lights == NULL) return FALSE;

  // clear out all the flags
  for (uint16_t uiCount = 0; uiCount < t->n_lights; ++uiCount) {
    t->lights[uiCount].uiFlags &= ~LIGHT_NODE_DRAWN;
  }

  const int16_t iX = l->iX;
  const int16_t iY = l->iY;

  iOldX = iX;
  iOldY = iY;

  for (uint16_t uiCount = 0; uiCount < t->n_rays; ++uiCount) {
    const uint16_t usNodeIndex = t->rays[uiCount];
    if (!(usNodeIndex & LIGHT_NEW_RAY)) {
      fBlocked = FALSE;
      fOnlyWalls = FALSE;

      LIGHT_NODE *const pLight = &t->lights[usNodeIndex & ~LIGHT_BACKLIGHT];

      if (!(l->uiFlags & LIGHT_SPR_ONROOF)) {
        if (LightTileBlocked((int16_t)iOldX, (int16_t)iOldY, (int16_t)(iX + pLight->iDX),
                             (int16_t)(iY + pLight->iDY))) {
          uiCount = LightFindNextRay(t, uiCount);

          fOnlyWalls = TRUE;
          fBlocked = TRUE;
        }
      }

      if (!(pLight->uiFlags & LIGHT_NODE_DRAWN) && (pLight->ubLight)) {
        uiFlags = (uint32_t)(usNodeIndex & LIGHT_BACKLIGHT);
        if (l->uiFlags & MERC_LIGHT) uiFlags |= LIGHT_FAKE;
        if (l->uiFlags & LIGHT_SPR_ONROOF) uiFlags |= LIGHT_ROOF_ONLY;

        LightAddTile(iOldX, iOldY, iX + pLight->iDX, iY + pLight->iDY, pLight->ubLight, uiFlags,
                     fOnlyWalls);

        pLight->uiFlags |= LIGHT_NODE_DRAWN;
      }

      if (fBlocked) {
        iOldX = iX;
        iOldY = iY;
      } else {
        iOldX = iX + pLight->iDX;
        iOldY = iY + pLight->iDY;
      }

    } else {
      iOldX = iX;
      iOldY = iY;
    }
  }

  return (TRUE);
}

static BOOLEAN LightRevealWall(const int16_t sX, const int16_t sY, const int16_t sSrcX,
                               const int16_t sSrcY) {
  Assert(gpWorldLevelData != NULL);

  const uint32_t uiTile = MAPROWCOLTOPOS(sY, sX);

  // IF A FENCE, RETURN FALSE
  if (IsFencePresentAtGridno(uiTile)) return FALSE;

  LEVELNODE *const head = gpWorldLevelData[uiTile].pStructHead;

  BOOLEAN fDoRightWalls = (sX >= sSrcX);
  BOOLEAN fDoLeftWalls = (sY >= sSrcY);

  for (const LEVELNODE *i = head; i != NULL; i = i->pNext) {
    if (i->uiFlags & LEVELNODE_CACHEDANITILE) continue;

    const TILE_ELEMENT *const TileElem = &gTileDatabase[i->usIndex];
    switch (TileElem->usWallOrientation) {
      case INSIDE_TOP_RIGHT:
      case OUTSIDE_TOP_RIGHT:
        if (!fDoRightWalls) fDoLeftWalls = FALSE;
        break;

      case INSIDE_TOP_LEFT:
      case OUTSIDE_TOP_LEFT:
        if (!fDoLeftWalls) fDoRightWalls = FALSE;
        break;
    }
  }

  BOOLEAN fHitWall = FALSE;
  BOOLEAN fRerender = FALSE;
  for (LEVELNODE *i = head; i != NULL; i = i->pNext) {
    if (i->uiFlags & LEVELNODE_CACHEDANITILE) continue;

    const TILE_ELEMENT *const TileElem = &gTileDatabase[i->usIndex];
    switch (TileElem->usWallOrientation) {
      case INSIDE_TOP_RIGHT:
      case OUTSIDE_TOP_RIGHT:
        fHitWall = TRUE;
        if (fDoRightWalls && sX >= sSrcX) {
          i->uiFlags |= LEVELNODE_REVEAL;
          fRerender = TRUE;
        }
        break;

      case INSIDE_TOP_LEFT:
      case OUTSIDE_TOP_LEFT:
        fHitWall = TRUE;
        if (fDoLeftWalls && sY >= sSrcY) {
          i->uiFlags |= LEVELNODE_REVEAL;
          fRerender = TRUE;
        }
        break;
    }
  }

  if (fRerender) SetRenderFlags(RENDER_FLAG_FULL);
  return fHitWall;
}

static BOOLEAN LightHideWall(const int16_t sX, const int16_t sY, const int16_t sSrcX,
                             const int16_t sSrcY) {
  Assert(gpWorldLevelData != NULL);

  uint32_t const uiTile = MAPROWCOLTOPOS(sY, sX);
  LEVELNODE *const head = gpWorldLevelData[uiTile].pStructHead;

  BOOLEAN fDoRightWalls = (sX >= sSrcX);
  BOOLEAN fDoLeftWalls = (sY >= sSrcY);

  for (const LEVELNODE *i = head; i != NULL; i = i->pNext) {
    if (i->uiFlags & LEVELNODE_CACHEDANITILE) continue;

    const TILE_ELEMENT *const te = &gTileDatabase[i->usIndex];
    switch (te->usWallOrientation) {
      case INSIDE_TOP_RIGHT:
      case OUTSIDE_TOP_RIGHT:
        if (!fDoRightWalls) fDoLeftWalls = FALSE;
        break;

      case INSIDE_TOP_LEFT:
      case OUTSIDE_TOP_LEFT:
        if (!fDoLeftWalls) fDoRightWalls = FALSE;
        break;
    }
  }

  BOOLEAN fHitWall = FALSE;
  BOOLEAN fRerender = FALSE;
  for (LEVELNODE *i = head; i != NULL; i = i->pNext) {
    if (i->uiFlags & LEVELNODE_CACHEDANITILE) continue;

    const TILE_ELEMENT *const te = &gTileDatabase[i->usIndex];
    switch (te->usWallOrientation) {
      case INSIDE_TOP_RIGHT:
      case OUTSIDE_TOP_RIGHT:
        fHitWall = TRUE;
        if (fDoRightWalls && sX >= sSrcX) {
          i->uiFlags &= ~LEVELNODE_REVEAL;
          fRerender = TRUE;
        }
        break;

      case INSIDE_TOP_LEFT:
      case OUTSIDE_TOP_LEFT:
        fHitWall = TRUE;
        if (fDoLeftWalls && sY >= sSrcY) {
          i->uiFlags &= ~LEVELNODE_REVEAL;
          fRerender = TRUE;
        }
        break;
    }
  }

  if (fRerender) SetRenderFlags(RENDER_FLAG_FULL);
  return fHitWall;
}

// Tags walls as being translucent using a light template.
static BOOLEAN CalcTranslucentWalls(int16_t iX, int16_t iY) {
  LightTemplate *const t = &g_light_templates[0];
  if (t->lights == NULL) return FALSE;

  for (uint16_t uiCount = 0; uiCount < t->n_rays; ++uiCount) {
    const uint16_t usNodeIndex = t->rays[uiCount];
    if (!(usNodeIndex & LIGHT_NEW_RAY)) {
      const LIGHT_NODE *const pLight = &t->lights[usNodeIndex & ~LIGHT_BACKLIGHT];

      // Kris:  added map boundary checking!!!
      if (LightRevealWall((int16_t)std::min(std::max((iX + pLight->iDX), 0), WORLD_COLS - 1),
                          (int16_t)std::min(std::max((iY + pLight->iDY), 0), WORLD_ROWS - 1),
                          (int16_t)std::min(std::max(iX, (int16_t)0), (int16_t)(WORLD_COLS - 1)),
                          (int16_t)std::min(std::max(iY, (int16_t)0), (int16_t)(WORLD_ROWS - 1)))) {
        uiCount = LightFindNextRay(t, uiCount);
        SetRenderFlags(RENDER_FLAG_FULL);
      }
    }
  }

  return (TRUE);
}

// Removes the green from the tiles that was drawn to show the path of the rays.
static BOOLEAN LightHideGreen(int16_t sX, int16_t sY, int16_t sSrcX, int16_t sSrcY) {
  LEVELNODE *pStruct, *pLand;
  uint32_t uiTile;
  BOOLEAN fRerender = FALSE, fHitWall = FALSE;
  TILE_ELEMENT *TileElem;

  Assert(gpWorldLevelData != NULL);

  uiTile = MAPROWCOLTOPOS(sY, sX);
  pStruct = gpWorldLevelData[uiTile].pStructHead;

  while (pStruct != NULL) {
    TileElem = &(gTileDatabase[pStruct->usIndex]);
    switch (TileElem->usWallOrientation) {
      case NO_ORIENTATION:
        break;

      case INSIDE_TOP_RIGHT:
      case OUTSIDE_TOP_RIGHT:
        fHitWall = TRUE;
        if (sX >= sSrcX) {
          pStruct->uiFlags &= (~LEVELNODE_REVEAL);
          fRerender = TRUE;
        }
        break;

      case INSIDE_TOP_LEFT:
      case OUTSIDE_TOP_LEFT:
        fHitWall = TRUE;
        if (sY >= sSrcY) {
          pStruct->uiFlags &= (~LEVELNODE_REVEAL);
          fRerender = TRUE;
        }
        break;
    }
    pStruct = pStruct->pNext;
  }

  // if(fRerender)
  //{
  pLand = gpWorldLevelData[uiTile].pLandHead;
  while (pLand != NULL) {
    pLand->ubShadeLevel = pLand->ubNaturalShadeLevel;
    pLand = pLand->pNext;
  }

  gpWorldLevelData[uiTile].uiFlags |= MAPELEMENT_REDRAW;
  SetRenderFlags(RENDER_FLAG_MARKED);
  //}

  return (fHitWall);
}

/****************************************************************************************
        ApplyTranslucencyToWalls

                Hides walls that were revealed by CalcTranslucentWalls.

***************************************************************************************/
BOOLEAN ApplyTranslucencyToWalls(int16_t iX, int16_t iY) {
  LightTemplate *const t = &g_light_templates[0];
  if (t->lights == NULL) return FALSE;

  for (uint16_t uiCount = 0; uiCount < t->n_rays; ++uiCount) {
    const uint16_t usNodeIndex = t->rays[uiCount];
    if (!(usNodeIndex & LIGHT_NEW_RAY)) {
      const LIGHT_NODE *const pLight = &t->lights[usNodeIndex & ~LIGHT_BACKLIGHT];
      // Kris:  added map boundary checking!!!
      if (LightHideWall((int16_t)std::min(std::max((iX + pLight->iDX), 0), WORLD_COLS - 1),
                        (int16_t)std::min(std::max((iY + pLight->iDY), 0), WORLD_ROWS - 1),
                        (int16_t)std::min(std::max(iX, (int16_t)0), (int16_t)(WORLD_COLS - 1)),
                        (int16_t)std::min(std::max(iY, (int16_t)0), (int16_t)(WORLD_ROWS - 1)))) {
        uiCount = LightFindNextRay(t, uiCount);
        SetRenderFlags(RENDER_FLAG_FULL);
      }
    }
  }

  return (TRUE);
}

// Reverts all tiles a given light affects to their natural light levels.
static BOOLEAN LightErase(const LIGHT_SPRITE *const l) {
  uint32_t uiFlags;
  int32_t iOldX, iOldY;
  BOOLEAN fBlocked = FALSE;
  BOOLEAN fOnlyWalls;

  LightTemplate *const t = l->light_template;
  if (t->lights == NULL) return FALSE;

  // clear out all the flags
  for (uint16_t uiCount = 0; uiCount < t->n_lights; ++uiCount) {
    t->lights[uiCount].uiFlags &= ~LIGHT_NODE_DRAWN;
  }

  const int16_t iX = l->iX;
  const int16_t iY = l->iY;
  iOldX = iX;
  iOldY = iY;

  for (uint16_t uiCount = 0; uiCount < t->n_rays; ++uiCount) {
    const uint16_t usNodeIndex = t->rays[uiCount];
    if (!(usNodeIndex & LIGHT_NEW_RAY)) {
      fBlocked = FALSE;
      fOnlyWalls = FALSE;

      LIGHT_NODE *const pLight = &t->lights[usNodeIndex & ~LIGHT_BACKLIGHT];

      if (!(l->uiFlags & LIGHT_SPR_ONROOF)) {
        if (LightTileBlocked((int16_t)iOldX, (int16_t)iOldY, (int16_t)(iX + pLight->iDX),
                             (int16_t)(iY + pLight->iDY))) {
          uiCount = LightFindNextRay(t, uiCount);

          fOnlyWalls = TRUE;
          fBlocked = TRUE;
        }
      }

      if (!(pLight->uiFlags & LIGHT_NODE_DRAWN) && (pLight->ubLight)) {
        uiFlags = (uint32_t)(usNodeIndex & LIGHT_BACKLIGHT);
        if (l->uiFlags & MERC_LIGHT) uiFlags |= LIGHT_FAKE;
        if (l->uiFlags & LIGHT_SPR_ONROOF) uiFlags |= LIGHT_ROOF_ONLY;

        LightSubtractTile(iOldX, iOldY, iX + pLight->iDX, iY + pLight->iDY, pLight->ubLight,
                          uiFlags, fOnlyWalls);
        pLight->uiFlags |= LIGHT_NODE_DRAWN;
      }

      if (fBlocked) {
        iOldX = iX;
        iOldY = iY;
      } else {
        iOldX = iX + pLight->iDX;
        iOldY = iY + pLight->iDY;
      }
    } else {
      iOldX = iX;
      iOldY = iY;
    }
  }

  return (TRUE);
}

/****************************************************************************************
        LightSave

                Saves the light list of a given template to a file. Passing in
NULL for the filename forces the system to save the light with the internal
filename (recommended).

***************************************************************************************/
void LightSave(LightTemplate const *const t, char const *const pFilename) {
  if (t->lights == NULL) throw std::logic_error("Tried to save invalid light template");

  const char *const pName = (pFilename != NULL ? pFilename : t->name);
  AutoSGPFile f(FileMan::openForWriting(pName));
  FileWriteArray(f, t->n_lights, t->lights);
  FileWriteArray(f, t->n_rays, t->rays);
}

/* Loads a light template from disk. The light template is returned, or NULL if
 * the file wasn't loaded. */
static LightTemplate *LightLoad(const char *pFilename) {
  AutoSGPFile hFile(FileMan::openForReadingSmart(pFilename, true));

  uint16_t n_lights;
  FileRead(hFile, &n_lights, sizeof(n_lights));
  SGP::Buffer<LIGHT_NODE> lights(n_lights);
  FileRead(hFile, lights, sizeof(*lights) * n_lights);

  uint16_t n_rays;
  FileRead(hFile, &n_rays, sizeof(n_rays));
  SGP::Buffer<uint16_t> rays(n_rays);
  FileRead(hFile, rays, sizeof(*rays) * n_rays);

  SGP::Buffer<char> name(strlen(pFilename) + 1);
  strcpy(name, pFilename);

  LightTemplate *const t = LightGetFree();
  t->n_lights = n_lights;
  t->lights = lights.Release();
  t->n_rays = n_rays;
  t->rays = rays.Release();
  t->name = name.Release();
  return t;
}

/* Figures out whether a light template is already in memory, or needs to be
 * loaded from disk. */
static LightTemplate *LightLoadCachedTemplate(const char *pFilename) {
  FOR_EACH_LIGHT_TEMPLATE(t) {
    if (strcasecmp(pFilename, t->name) == 0) return t;
  }
  return LightLoad(pFilename);
}

const SGPPaletteEntry *LightGetColor() { return &gpOrigLight; }

void LightSetColor(const SGPPaletteEntry *const pPal) {
  Assert(pPal != NULL);

  if (pPal->r != g_light_color.r || pPal->g != g_light_color.g ||
      pPal->b != g_light_color.b) {  // Set the entire tileset database so that it
                                     // reloads everything.  It has to because the
    // colors have changed.
    SetAllNewTileSurfacesLoaded(TRUE);
  }

  // before doing anything, get rid of all the old palettes
  DestroyTileShadeTables();

  g_light_color = *pPal;
  gpOrigLight = *pPal;

  BuildTileShadeTables();

  // Build all palettes for all soldiers in the world
  // ( THIS FUNCTION WILL ERASE THEM IF THEY EXIST )
  RebuildAllSoldierShadeTables();

  SetRenderFlags(RENDER_FLAG_FULL);
}

//---------------------------------------------------------------------------------------
// Light Manipulation Layer
//---------------------------------------------------------------------------------------

// Returns the next available sprite.
static LIGHT_SPRITE *LightSpriteGetFree() {
  FOR_EACH(LIGHT_SPRITE, l, LightSprites) {
    if (!(l->uiFlags & LIGHT_SPR_ACTIVE)) return l;
  }
  throw std::runtime_error("Out of light sprite slots");
}

LIGHT_SPRITE *LightSpriteCreate(const char *const pName) try {
  LIGHT_SPRITE *const l = LightSpriteGetFree();

  memset(l, 0, sizeof(LIGHT_SPRITE));
  l->iX = WORLD_COLS + 1;
  l->iY = WORLD_ROWS + 1;

  l->light_template = LightLoadCachedTemplate(pName);

  l->uiFlags |= LIGHT_SPR_ACTIVE;
  return l;
} catch (...) {
  return 0;
}

BOOLEAN LightSpriteFake(LIGHT_SPRITE *const l) {
  if (l->uiFlags & LIGHT_SPR_ACTIVE) {
    l->uiFlags |= MERC_LIGHT;
    return (TRUE);
  } else {
    return (FALSE);
  }
}

static void LightSpriteDirty(const LIGHT_SPRITE *l);

BOOLEAN LightSpriteDestroy(LIGHT_SPRITE *const l) {
  if (l->uiFlags & LIGHT_SPR_ACTIVE) {
    if (l->uiFlags & LIGHT_SPR_ERASE) {
      if (l->iX < WORLD_COLS && l->iY < WORLD_ROWS) {
        LightErase(l);
        LightSpriteDirty(l);
      }
      l->uiFlags &= ~LIGHT_SPR_ERASE;
    }

    l->uiFlags &= ~LIGHT_SPR_ACTIVE;
    return (TRUE);
  }

  return (FALSE);
}

void LightSpriteRenderAll() {
  LightResetAllTiles();
  FOR_EACH(LIGHT_SPRITE, i, LightSprites) {
    LIGHT_SPRITE &l = *i;
    l.uiFlags &= ~LIGHT_SPR_ERASE;
    if (!(l.uiFlags & LIGHT_SPR_ACTIVE)) continue;
    if (!(l.uiFlags & LIGHT_SPR_ON)) continue;
    LightDraw(&l);
    l.uiFlags |= LIGHT_SPR_ERASE;
    LightSpriteDirty(&l);
  }
}

void LightSpritePosition(LIGHT_SPRITE *const l, const int16_t iX, const int16_t iY) {
  Assert(l->uiFlags & LIGHT_SPR_ACTIVE);

  if (l->iX == iX && l->iY == iY) return;

  if (l->uiFlags & LIGHT_SPR_ERASE) {
    if (l->iX < WORLD_COLS && l->iY < WORLD_ROWS) {
      LightErase(l);
      LightSpriteDirty(l);
    }
  }

  l->iX = iX;
  l->iY = iY;

  if (l->uiFlags & LIGHT_SPR_ON) {
    if (l->iX < WORLD_COLS && l->iY < WORLD_ROWS) {
      LightDraw(l);
      l->uiFlags |= LIGHT_SPR_ERASE;
      LightSpriteDirty(l);
    }
  }
}

BOOLEAN LightSpriteRoofStatus(LIGHT_SPRITE *const l, BOOLEAN fOnRoof) {
  if (fOnRoof && (l->uiFlags & LIGHT_SPR_ONROOF)) return FALSE;
  if (!fOnRoof && !(l->uiFlags & LIGHT_SPR_ONROOF)) return FALSE;

  if (l->uiFlags & LIGHT_SPR_ACTIVE) {
    if (l->uiFlags & LIGHT_SPR_ERASE) {
      if (l->iX < WORLD_COLS && l->iY < WORLD_ROWS) {
        LightErase(l);
        LightSpriteDirty(l);
      }
    }

    if (fOnRoof) {
      l->uiFlags |= LIGHT_SPR_ONROOF;
    } else {
      l->uiFlags &= ~LIGHT_SPR_ONROOF;
    }

    if (l->uiFlags & LIGHT_SPR_ON) {
      if (l->iX < WORLD_COLS && l->iY < WORLD_ROWS) {
        LightDraw(l);
        l->uiFlags |= LIGHT_SPR_ERASE;
        LightSpriteDirty(l);
      }
    }
  } else
    return (FALSE);

  return (TRUE);
}

void LightSpritePower(LIGHT_SPRITE *const l, const BOOLEAN fOn) {
  l->uiFlags = (l->uiFlags & ~LIGHT_SPR_ON) | (fOn ? LIGHT_SPR_ON : 0);
}

// Sets the flag for the renderer to draw all marked tiles.
static void LightSpriteDirty(LIGHT_SPRITE const *const l) {
#if 0  // XXX was commented out
	int16_t iLeft_s;
	int16_t iTop_s;
	CellXYToScreenXY(l->iX * CELL_X_SIZE, l->iY * CELL_Y_SIZE, &iLeft_s, &iTop_s);

	const LightTemplate* const t = &g_light_templates[l->iTemplate];

	iLeft_s += t->x_off;
	iTop_s  += t->y_off;

	const int16_t iMapLeft   = l->iX + t->map_left;
	const int16_t iMapTop    = l->iY + t->map_top;
	const int16_t iMapRight  = l->iX + t->map_right;
	const int16_t iMapBottom = l->iY + t->map_bottom;

	UpdateSaveBuffer();
	AddBaseDirtyRect(gsVIEWPORT_START_X, gsVIEWPORT_START_Y, gsVIEWPORT_END_X, gsVIEWPORT_END_Y);
	AddBaseDirtyRect(iLeft_s, iTop_s, (int16_t)(iLeft_s + t->width), (int16_t)(iTop_s + t->height));
#endif

  SetRenderFlags(RENDER_FLAG_MARKED);
}

static void AddSaturatePalette(SGPPaletteEntry Dst[256], const SGPPaletteEntry Src[256],
                               const SGPPaletteEntry *Bias) {
  uint8_t r = Bias->r;
  uint8_t g = Bias->g;
  uint8_t b = Bias->b;
  for (uint32_t i = 0; i < 256; i++) {
    Dst[i].r = std::min(Src[i].r + r, 255);
    Dst[i].g = std::min(Src[i].g + g, 255);
    Dst[i].b = std::min(Src[i].b + b, 255);
  }
}

static void CreateShadedPalettes(uint16_t *Shades[16], const SGPPaletteEntry ShadePal[256]) {
  const uint16_t *sl0 = gusShadeLevels[0];
  Shades[0] = Create16BPPPaletteShaded(ShadePal, sl0[0], sl0[1], sl0[2], TRUE);
  for (uint32_t i = 1; i < 16; i++) {
    const uint16_t *sl = gusShadeLevels[i];
    Shades[i] = Create16BPPPaletteShaded(ShadePal, sl[0], sl[1], sl[2], FALSE);
  }
}

void CreateBiasedShadedPalettes(uint16_t *Shades[16], const SGPPaletteEntry ShadePal[256]) {
  SGPPaletteEntry LightPal[256];
  AddSaturatePalette(LightPal, ShadePal, &g_light_color);
  CreateShadedPalettes(Shades, LightPal);
}

/**********************************************************************************************
 CreateObjectPaletteTables

                Creates the shading tables for 8-bit brushes. One highlight
table is created, based on the object-type, 3 brightening tables, 1 normal, and
11 darkening tables. The entries are created iteratively, rather than in a loop
to allow hand-tweaking of the values. If you change the HVOBJECT_SHADE_TABLES
symbol, remember to add/delete entries here, it won't adjust automagically.

**********************************************************************************************/
void CreateTilePaletteTables(const HVOBJECT pObj) {
  Assert(pObj != NULL);

  // build the shade tables
  CreateBiasedShadedPalettes(pObj->pShades, pObj->Palette());

  // build neutral palette as well!
  // Set current shade table to neutral color
  pObj->CurrentShade(4);
}

const char *LightSpriteGetTypeName(const LIGHT_SPRITE *const l) { return l->light_template->name; }

#undef FAIL
#include "gtest/gtest.h"

TEST(Lighting, asserts) { EXPECT_EQ(sizeof(LIGHT_NODE), 6); }
