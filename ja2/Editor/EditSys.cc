// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Editor/EditSys.h"

#include "Editor/CursorModes.h"
#include "Editor/EditScreen.h"
#include "Editor/EditorBuildings.h"
#include "Editor/EditorDefines.h"
#include "Editor/EditorMercs.h"
#include "Editor/EditorTerrain.h"
#include "Editor/EditorUndo.h"
#include "Editor/NewSmooth.h"
#include "Editor/RoadSmoothing.h"
#include "Editor/SelectWin.h"
#include "Editor/Smooth.h"
#include "Editor/SmoothingUtils.h"
#include "Macro.h"
#include "SGP/HImage.h"
#include "SGP/MemMan.h"
#include "TileEngine/Environment.h"
#include "TileEngine/ExitGrids.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/RenderFun.h"
#include "TileEngine/SimpleRenderUtils.h"
#include "TileEngine/Structure.h"
#include "TileEngine/TileDef.h"
#include "TileEngine/WorldDef.h"
#include "TileEngine/WorldMan.h"

uint16_t CurrentPaste = NO_TILE;

//---------------------------------------------------------------------------------------------------------------
//	QuickEraseMapTile
//
//	Performs ersing operation when the DEL key is hit in the editor
//
void QuickEraseMapTile(uint32_t iMapIndex) {
  if (iMapIndex >= 0x8000) return;
  AddToUndoList(iMapIndex);
  DeleteStuffFromMapTile(iMapIndex);
  MarkWorldDirty();
}

//---------------------------------------------------------------------------------------------------------------
//	DeleteStuffFromMapTile
//
//	Common delete function for both QuickEraseMapTile and EraseMapTile
//
void DeleteStuffFromMapTile(uint32_t iMapIndex) {
  // uint16_t		usUseIndex;
  // uint16_t		usType;
  // uint16_t		usDummy;

  // const uint32_t uiCheckType =
  // GetTileType(gpWorldLevelData[iMapIndex].pLandHead->usIndex); RemoveLand(
  // iMapIndex, gpWorldLevelData[ iMapIndex ].pLandHead->usIndex );
  // SmoothTerrainRadius( iMapIndex, uiCheckType, 1, TRUE );

  RemoveExitGridFromWorld(iMapIndex);
  RemoveAllStructsOfTypeRange(iMapIndex, FIRSTTEXTURE, WIREFRAMES);
  RemoveAllObjectsOfTypeRange(iMapIndex, FIRSTTEXTURE, WIREFRAMES);
  RemoveAllShadowsOfTypeRange(iMapIndex, FIRSTTEXTURE, WIREFRAMES);
  RemoveAllLandsOfTypeRange(iMapIndex, FIRSTTEXTURE, WIREFRAMES);
  RemoveAllRoofsOfTypeRange(iMapIndex, FIRSTTEXTURE, WIREFRAMES);
  RemoveAllOnRoofsOfTypeRange(iMapIndex, FIRSTTEXTURE, WIREFRAMES);
  RemoveAllTopmostsOfTypeRange(iMapIndex, FIRSTTEXTURE, WIREFRAMES);
  PasteRoomNumber(iMapIndex, 0);
}

//---------------------------------------------------------------------------------------------------------------
//	EraseMapTile
//
//	Generic tile erasing function. Erases things from the world depending on
// the current drawing mode
//
void EraseMapTile(uint32_t iMapIndex) {
  int32_t iEraseMode;
  if (iMapIndex >= 0x8000) return;

  // Figure out what it is we are trying to erase
  iEraseMode = iDrawMode - DRAW_MODE_ERASE;

  switch (iEraseMode) {
    case DRAW_MODE_NORTHPOINT:
    case DRAW_MODE_WESTPOINT:
    case DRAW_MODE_EASTPOINT:
    case DRAW_MODE_SOUTHPOINT:
    case DRAW_MODE_CENTERPOINT:
    case DRAW_MODE_ISOLATEDPOINT:
      SpecifyEntryPoint(iMapIndex);
      break;
    case DRAW_MODE_EXITGRID:
      AddToUndoList(iMapIndex);
      RemoveExitGridFromWorld(iMapIndex);
      RemoveTopmost((uint16_t)iMapIndex, FIRSTPOINTERS8);
      break;
    case DRAW_MODE_GROUND: {
      // Is there ground on this tile? if not, get out o here
      if (gpWorldLevelData[iMapIndex].pLandHead == NULL) break;

      // is there only 1 ground tile here? if so, get out o here
      if (gpWorldLevelData[iMapIndex].pLandHead->pNext == NULL) break;
      AddToUndoList(iMapIndex);
      const uint32_t uiCheckType = GetTileType(gpWorldLevelData[iMapIndex].pLandHead->usIndex);
      RemoveLand(iMapIndex, gpWorldLevelData[iMapIndex].pLandHead->usIndex);
      SmoothTerrainRadius(iMapIndex, uiCheckType, 1, TRUE);
      break;
    }

    case DRAW_MODE_OSTRUCTS:
    case DRAW_MODE_OSTRUCTS1:
    case DRAW_MODE_OSTRUCTS2:
      AddToUndoList(iMapIndex);
      RemoveAllStructsOfTypeRange(iMapIndex, FIRSTOSTRUCT, LASTOSTRUCT);
      RemoveAllStructsOfTypeRange(iMapIndex, FIRSTVEHICLE, SECONDVEHICLE);
      RemoveAllStructsOfTypeRange(iMapIndex, FIRSTDEBRISSTRUCT, SECONDDEBRISSTRUCT);
      RemoveAllStructsOfTypeRange(iMapIndex, NINTHOSTRUCT, TENTHOSTRUCT);
      RemoveAllStructsOfTypeRange(iMapIndex, FIRSTLARGEEXPDEBRIS, SECONDLARGEEXPDEBRIS);
      RemoveAllObjectsOfTypeRange(iMapIndex, DEBRIS2MISC, DEBRIS2MISC);
      RemoveAllObjectsOfTypeRange(iMapIndex, ANOTHERDEBRIS, ANOTHERDEBRIS);
      break;
    case DRAW_MODE_DEBRIS:
      AddToUndoList(iMapIndex);
      RemoveAllObjectsOfTypeRange(iMapIndex, DEBRISROCKS, LASTDEBRIS);
      RemoveAllObjectsOfTypeRange(iMapIndex, DEBRIS2MISC, DEBRIS2MISC);
      RemoveAllObjectsOfTypeRange(iMapIndex, ANOTHERDEBRIS, ANOTHERDEBRIS);
      break;
    case DRAW_MODE_BANKS:
      AddToUndoList(iMapIndex);
      RemoveAllObjectsOfTypeRange(iMapIndex, FIRSTROAD, LASTROAD);
      // Note, for this routine, cliffs are considered a subset of banks
      RemoveAllStructsOfTypeRange(iMapIndex, ANIOSTRUCT, ANIOSTRUCT);
      RemoveAllStructsOfTypeRange(iMapIndex, FIRSTCLIFF, LASTBANKS);
      RemoveAllShadowsOfTypeRange(iMapIndex, FIRSTCLIFFSHADOW, LASTCLIFFSHADOW);
      RemoveAllObjectsOfTypeRange(iMapIndex, FIRSTCLIFFHANG, LASTCLIFFHANG);
      RemoveAllStructsOfTypeRange(iMapIndex, FENCESTRUCT, FENCESTRUCT);
      RemoveAllShadowsOfTypeRange(iMapIndex, FENCESHADOW, FENCESHADOW);
      break;
    case DRAW_MODE_FLOORS:
      AddToUndoList(iMapIndex);
      RemoveAllLandsOfTypeRange(iMapIndex, FIRSTFLOOR, LASTFLOOR);
      break;
    case DRAW_MODE_ROOFS:
    case DRAW_MODE_NEWROOF:
      AddToUndoList(iMapIndex);
      RemoveAllRoofsOfTypeRange(iMapIndex, FIRSTTEXTURE, LASTITEM);
      RemoveAllOnRoofsOfTypeRange(iMapIndex, FIRSTTEXTURE, LASTITEM);
      break;
    case DRAW_MODE_WALLS:
    case DRAW_MODE_DOORS:
    case DRAW_MODE_WINDOWS:
    case DRAW_MODE_BROKEN_WALLS:
      AddToUndoList(iMapIndex);
      RemoveAllStructsOfTypeRange(iMapIndex, FIRSTWALL, LASTWALL);
      RemoveAllShadowsOfTypeRange(iMapIndex, FIRSTWALL, LASTWALL);
      RemoveAllStructsOfTypeRange(iMapIndex, FIRSTDOOR, LASTDOOR);
      RemoveAllShadowsOfTypeRange(iMapIndex, FIRSTDOORSHADOW, LASTDOORSHADOW);
      break;
    case DRAW_MODE_DECOR:
    case DRAW_MODE_DECALS:
    case DRAW_MODE_ROOM:
    case DRAW_MODE_TOILET:
      AddToUndoList(iMapIndex);
      RemoveAllStructsOfTypeRange(iMapIndex, FIRSTWALLDECAL, LASTWALLDECAL);
      RemoveAllStructsOfTypeRange(iMapIndex, FIFTHWALLDECAL, EIGTHWALLDECAL);
      RemoveAllStructsOfTypeRange(iMapIndex, FIRSTDECORATIONS, LASTDECORATIONS);
      RemoveAllStructsOfTypeRange(iMapIndex, FIRSTISTRUCT, LASTISTRUCT);
      RemoveAllStructsOfTypeRange(iMapIndex, FIFTHISTRUCT, EIGHTISTRUCT);
      RemoveAllStructsOfTypeRange(iMapIndex, FIRSTSWITCHES, FIRSTSWITCHES);
      break;
    case DRAW_MODE_CAVES:
      AddToUndoList(iMapIndex);
      RemoveAllStructsOfTypeRange(iMapIndex, FIRSTWALL, LASTWALL);
      break;
    case DRAW_MODE_ROOMNUM:
      PasteRoomNumber(iMapIndex, 0);
      break;
    case DRAW_MODE_ROADS:
      RemoveAllObjectsOfTypeRange(iMapIndex, ROADPIECES, ROADPIECES);
      break;
    default:
      // DeleteStuffFromMapTile( iMapIndex );
      break;
  }
}

//---------------------------------------------------------------------------------------------------------------
//	PasteDebris
//
//	Place some "debris" on the map at the current mouse coordinates. This
// function is called repeatedly if 	the current brush size is larger than 1 tile.
//
void PasteDebris(uint32_t iMapIndex) {
  uint16_t usUseIndex;
  uint16_t usUseObjIndex;
  int32_t iRandSelIndex;

  // Get selection list for debris
  pSelList = SelDebris;
  pNumSelList = &iNumDebrisSelected;

  if (iMapIndex < 0x8000) {
    AddToUndoList(iMapIndex);

    // Remove any debris that is currently at this map location
    if (gpWorldLevelData[iMapIndex].pObjectHead != NULL) {
      RemoveAllObjectsOfTypeRange(iMapIndex, ANOTHERDEBRIS, FIRSTPOINTERS - 1);
    }

    // Get a random debris from current selection
    iRandSelIndex = GetRandomSelection();
    if (iRandSelIndex != -1) {
      // Add debris to the world
      usUseIndex = pSelList[iRandSelIndex].usIndex;
      usUseObjIndex = (uint16_t)pSelList[iRandSelIndex].uiObject;

      AddObjectToTail(iMapIndex, (uint16_t)(gTileTypeStartIndex[usUseObjIndex] + usUseIndex));
    }
  }
}

static void PasteSingleWallCommon(uint32_t map_idx, Selections *sel_list, int32_t &n_sel_list);

void PasteSingleWall(uint32_t iMapIndex) {
  PasteSingleWallCommon(iMapIndex, SelSingleWall, iNumWallsSelected);
}

void PasteSingleDoor(uint32_t iMapIndex) {
  PasteSingleWallCommon(iMapIndex, SelSingleDoor, iNumDoorsSelected);
}

void PasteSingleWindow(uint32_t iMapIndex) {
  PasteSingleWallCommon(iMapIndex, SelSingleWindow, iNumWindowsSelected);
}

void PasteSingleRoof(uint32_t iMapIndex) {
  PasteSingleWallCommon(iMapIndex, SelSingleRoof, iNumRoofsSelected);
}

void PasteRoomNumber(uint32_t iMapIndex, uint8_t ubRoomNumber) {
  if (gubWorldRoomInfo[iMapIndex] != ubRoomNumber) {
    AddToUndoList(iMapIndex);
    gubWorldRoomInfo[iMapIndex] = ubRoomNumber;
  }
}

void PasteSingleBrokenWall(uint32_t iMapIndex) {
  Selections *const sel_list = SelSingleBrokenWall;
  uint16_t usIndex = sel_list[iCurBank].usIndex;
  uint16_t usObjIndex = sel_list[iCurBank].uiObject;
  uint16_t usTileIndex = GetTileIndexFromTypeSubIndex(usObjIndex, usIndex);
  uint16_t usWallOrientation = GetWallOrientation(usTileIndex);
  if (usWallOrientation == INSIDE_TOP_LEFT || usWallOrientation == INSIDE_TOP_RIGHT)
    EraseHorizontalWall(iMapIndex);
  else
    EraseVerticalWall(iMapIndex);

  PasteSingleWallCommon(iMapIndex, sel_list, iNumBrokenWallsSelected);
}

void PasteSingleDecoration(uint32_t iMapIndex) {
  PasteSingleWallCommon(iMapIndex, SelSingleDecor, iNumDecorSelected);
}

void PasteSingleDecal(uint32_t iMapIndex) {
  PasteSingleWallCommon(iMapIndex, SelSingleDecal, iNumDecalsSelected);
}

void PasteSingleFloor(uint32_t iMapIndex) {
  PasteSingleWallCommon(iMapIndex, SelSingleFloor, iNumFloorsSelected);
}

void PasteSingleToilet(uint32_t iMapIndex) {
  PasteSingleWallCommon(iMapIndex, SelSingleToilet, iNumToiletsSelected);
}

/* Common paste routine for PasteSingleWall, PasteSingleDoor,
 * PasteSingleDecoration, and PasteSingleDecor (above). */
static void PasteSingleWallCommon(uint32_t const map_idx, Selections *const sel_list,
                                  int32_t &n_sel_list) try {
  pSelList = sel_list;
  pNumSelList = &n_sel_list;

  if (map_idx >= 0x8000) return;

  AddToUndoList(map_idx);

  uint16_t const use_idx = sel_list[iCurBank].usIndex;
  uint16_t const use_obj_idx = sel_list[iCurBank].uiObject;
  uint16_t const idx = gTileTypeStartIndex[use_obj_idx] + use_idx;

  // Temp stuff for onroof things
  if (FIRSTONROOF <= use_obj_idx && use_obj_idx <= SECONDONROOF) {
    // Add to onroof section
    AddOnRoofToTail(map_idx, idx);

    int16_t const buddy_num = gTileDatabase[idx].sBuddyNum;
    if (buddy_num != -1) AddOnRoofToTail(map_idx, buddy_num);
    return;
  }

  // Make sure A-frames are on roof level
  if (WALL_AFRAME_START <= use_idx && use_idx <= WALL_AFRAME_END) {
    AddRoofToTail(map_idx, idx);
    return;
  }

  if (FIRSTDOOR <= use_obj_idx && use_obj_idx <= LASTDOOR) {
    // Place shadow for doors
    if (!gfBasement)
      AddExclusiveShadow(map_idx,
                         gTileTypeStartIndex[use_obj_idx - FIRSTDOOR + FIRSTDOORSHADOW] + use_idx);
  }

  // Is it a wall?
  if (FIRSTWALL <= use_obj_idx && use_obj_idx <= LASTWALL) {
    // ATE: If it is a wall shadow, place differenty!
    if (use_idx == 29 || use_idx == 30) {
      if (!gfBasement) AddExclusiveShadow(map_idx, idx);
    } else {  // Slap down wall/window/door/decoration (no smoothing)
      AddWallToStructLayer(map_idx, idx, TRUE);
    }
  } else if ((FIRSTDOOR <= use_obj_idx && use_obj_idx <= LASTDOOR) ||
             (FIRSTDECORATIONS <= use_obj_idx &&
              use_obj_idx <= LASTDECORATIONS)) {  // Slap down wall/window/door/decoration
                                                  // (no smoothing)
    AddWallToStructLayer(map_idx, idx, TRUE);
  } else if ((FIRSTROOF <= use_obj_idx && use_obj_idx <= LASTROOF) ||
             (FIRSTSLANTROOF <= use_obj_idx &&
              use_obj_idx <= LASTSLANTROOF)) {  // Put a roof on this tile (even
                                                // if nothing else is here)
    RemoveAllRoofsOfTypeRange(map_idx, FIRSTROOF, LASTROOF);
    AddRoofToTail(map_idx, idx);
  } else if (FIRSTFLOOR <= use_obj_idx && use_obj_idx <= LASTFLOOR) {  // Drop a floor on this tile
    if (LEVELNODE const *const land = FindTypeInLandLayer(map_idx, use_obj_idx)) {
      RemoveLand(map_idx, land->usIndex);
    }
    AddLandToHead(map_idx, idx);
  } else if ((FIRSTWALLDECAL <= use_obj_idx && use_obj_idx <= LASTWALLDECAL) ||
             ((FIFTHWALLDECAL <= use_obj_idx &&
               use_obj_idx <= EIGTHWALLDECAL))) {  // Plop a decal here
    RemoveAllStructsOfTypeRange(map_idx, FIRSTWALLDECAL, LASTWALLDECAL);
    RemoveAllStructsOfTypeRange(map_idx, FIFTHWALLDECAL, EIGTHWALLDECAL);
    AddStructToTail(map_idx, idx);
  } else if ((FIRSTISTRUCT <= use_obj_idx && use_obj_idx <= LASTISTRUCT) ||
             ((FIFTHISTRUCT <= use_obj_idx && use_obj_idx <= EIGHTISTRUCT))) {
    AddStructToHead(map_idx, idx);
  } else if (use_obj_idx == FIRSTSWITCHES) {
    AddStructToTail(map_idx, idx);
  }
} catch (FailedToAddNode const &) { /* XXX TODO0021 ignore */
}

//---------------------------------------------------------------------------------------------------------------
//	GetRandomIndexByRange
//
//	Returns a randomly picked object index given the current selection list,
// and the type or types of objects we want 	from that list. If no such objects
// are in the list, we return 0xffff (-1).
uint16_t GetRandomIndexByRange(uint16_t usRangeStart, uint16_t usRangeEnd) {
  uint16_t usPickList[50];
  uint16_t usNumInPickList;
  uint16_t usWhich;
  uint16_t usObject;
  // Get a list of valid object to select from
  usNumInPickList = 0;
  for (usWhich = 0; usWhich < *pNumSelList; usWhich++) {
    usObject = (uint16_t)pSelList[usWhich].uiObject;
    if ((usObject >= usRangeStart) && (usObject <= usRangeEnd)) {
      usPickList[usNumInPickList] = usObject;
      usNumInPickList++;
    }
  }
  return (usNumInPickList) ? usPickList[rand() % usNumInPickList] : 0xffff;
}

static void PasteStructureCommon(uint32_t iMapIndex);

//---------------------------------------------------------------------------------------------------------------
//	PasteStructure			(See also PasteStructure1,
// PasteStructure2, and PasteStructureCommon)
//
//	Puts a structure (trees, trucks, etc.) into the world
//
void PasteStructure(uint32_t iMapIndex) {
  pSelList = SelOStructs;
  pNumSelList = &iNumOStructsSelected;

  PasteStructureCommon(iMapIndex);
}

//---------------------------------------------------------------------------------------------------------------
//	PasteStructure1			(See also PasteStructure,
// PasteStructure2, and PasteStructureCommon)
//
//	Puts a structure (trees, trucks, etc.) into the world
//
void PasteStructure1(uint32_t iMapIndex) {
  pSelList = SelOStructs1;
  pNumSelList = &iNumOStructs1Selected;

  PasteStructureCommon(iMapIndex);
}

//---------------------------------------------------------------------------------------------------------------
//	PasteStructure2			(See also PasteStructure,
// PasteStructure1, and PasteStructureCommon)
//
//	Puts a structure (trees, trucks, etc.) into the world
//
void PasteStructure2(uint32_t iMapIndex) {
  pSelList = SelOStructs2;
  pNumSelList = &iNumOStructs2Selected;

  PasteStructureCommon(iMapIndex);
}

//	This is the main (common) structure pasting function. The above three
// wrappers are only required because they 	each use different selection lists.
// Other than that, they are COMPLETELY identical.
static void PasteStructureCommon(const uint32_t iMapIndex) {
  if (iMapIndex >= 0x8000) return;

  const int32_t iRandSelIndex = GetRandomSelection();
  if (iRandSelIndex == -1) return;

  AddToUndoList(iMapIndex);

  const uint16_t usUseIndex = pSelList[iRandSelIndex].usIndex;
  const uint16_t usUseObjIndex = pSelList[iRandSelIndex].uiObject;

  // Check with Structure Database (aka ODB) if we can put the object here!
  const DB_STRUCTURE_REF *const sr =
      gTileDatabase[gTileTypeStartIndex[usUseObjIndex] + usUseIndex].pDBStructureRef;
  if (!OkayToAddStructureToWorld(iMapIndex, 0, sr, INVALID_STRUCTURE_ID) && sr != NULL) return;

  // Actual structure info is added by the functions below
  AddStructToHead(iMapIndex, gTileTypeStartIndex[usUseObjIndex] + usUseIndex);
}

//---------------------------------------------------------------------------------------------------------------
//	PasteBanks
//
//	Places a river bank or cliff into the world
//
void PasteBanks(uint32_t const iMapIndex, BOOLEAN const fReplace) {
  BOOLEAN fDoPaste = FALSE;
  uint16_t usUseIndex;
  uint16_t usUseObjIndex;

  pSelList = SelBanks;
  pNumSelList = &iNumBanksSelected;

  usUseIndex = pSelList[iCurBank].usIndex;
  usUseObjIndex = (uint16_t)pSelList[iCurBank].uiObject;

  if (iMapIndex < 0x8000) {
    fDoPaste = TRUE;

    if (gpWorldLevelData[iMapIndex].pStructHead != NULL) {
      // CHECK IF THE SAME TILE IS HERE
      if (gpWorldLevelData[iMapIndex].pStructHead->usIndex ==
          (uint16_t)(gTileTypeStartIndex[usUseObjIndex] + usUseIndex)) {
        fDoPaste = FALSE;
      }
    } else {
      // Nothing is here, paste
      fDoPaste = TRUE;
    }

    if (fDoPaste) {
      AddToUndoList(iMapIndex);

      {
        if (usUseObjIndex == FIRSTROAD) {
          AddObjectToHead(iMapIndex, (uint16_t)(gTileTypeStartIndex[usUseObjIndex] + usUseIndex));
        } else {
          AddStructToHead(iMapIndex, (uint16_t)(gTileTypeStartIndex[usUseObjIndex] + usUseIndex));
          // Add shadows
          if (!gfBasement && usUseObjIndex == FIRSTCLIFF) {
            // AddShadowToHead( iMapIndex, (uint16_t)( gTileTypeStartIndex[
            // usUseObjIndex - FIRSTCLIFF + FIRSTCLIFFSHADOW ] + usUseIndex ) );
            AddObjectToHead(
                iMapIndex,
                (uint16_t)(gTileTypeStartIndex[usUseObjIndex - FIRSTCLIFF + FIRSTCLIFFHANG] +
                           usUseIndex));
          }
        }
      }
    }
  }
}

void PasteRoads(uint32_t iMapIndex) {
  uint16_t usUseIndex;

  pSelList = SelRoads;
  pNumSelList = &iNumRoadsSelected;

  usUseIndex = pSelList[iCurBank].usIndex;

  PlaceRoadMacroAtGridNo(iMapIndex, usUseIndex);
}

//---------------------------------------------------------------------------------------------------------------
//	PasteTexture
//
//	Puts a ground texture in the world. Ground textures are then "smoothed"
// in order to blend the edges with one 	another. The current drawing brush also
// affects this function.
//
void PasteTexture(uint32_t iMapIndex) {
  ChooseWeightedTerrainTile();  // Kris
  PasteTextureCommon(iMapIndex);
}

static void PasteHigherTexture(uint32_t iMapIndex, uint32_t fNewType);
static void PasteTextureEx(GridNo, uint16_t type);

/* PasteTexture() calls this one to actually put a ground tile down. If the
 * brush size is larger than one tile, then the above function will call this
 * one and indicate that they should all be placed into the undo stack as the
 * same undo command. */
void PasteTextureCommon(uint32_t const map_idx) {
  uint16_t const paste = CurrentPaste;
  if (paste == NO_TILE) return;
  if (map_idx >= 0x8000) return;

  // Set undo, then set new
  AddToUndoList(map_idx);

  if (paste == DEEPWATERTEXTURE) {  // If we are pasting deep water and we are
                                    // not over water, ignore!
    LEVELNODE const *const land = FindTypeInLandLayer(map_idx, REGWATERTEXTURE);
    if (!land || !gTileDatabase[land->usIndex].ubFullTile) return;
  }

  // Don't draw over floors
  if (TypeRangeExistsInLandLayer(map_idx, FIRSTFLOOR, FOURTHFLOOR)) return;

  // Compare heights and do appropriate action
  uint8_t last_high_level;
  if (AnyHeigherLand(map_idx, paste, &last_high_level)) {
    /* Here we do the following:
     * - Remove old type from layer
     * - Smooth World with old type
     * - Add a 3 by 3 square of new type at head
     * - Smooth World with new type */
    PasteHigherTexture(map_idx, paste);
  } else {
    PasteTextureEx(map_idx, paste);
    SmoothTerrainRadius(map_idx, paste, 1, TRUE);
  }
}

static BOOLEAN SetLowerLandIndexWithRadius(int32_t iMapIndex, uint32_t uiNewType, uint8_t ubRadius,
                                           BOOLEAN fReplace);

//	Some ground textures should be placed "above" others. That is, grass
// needs to be placed "above" sand etc. 	This function performs the appropriate
// actions.
static void PasteHigherTexture(uint32_t iMapIndex, uint32_t fNewType) {
  uint8_t ubLastHighLevel;
  uint32_t *puiDeletedTypes = NULL;
  uint8_t ubNumTypes;
  uint8_t cnt;

  // Here we do the following:
  // - Remove old type from layer
  // - Smooth World with old type
  // - Add a 3 by 3 square of new type at head
  // - Smooth World with new type

  // if (iMapIndex < 0x8000 && TypeRangeExistsInLandLayer(iMapIndex, FIRSTFLOOR,
  // LASTFLOOR)) ATE: DONOT DO THIS!!!!!!! - I know what was intended - not to
  // draw over floors - this
  // I don't know is the right way to do it!
  // return;

  if (iMapIndex < 0x8000 && AnyHeigherLand(iMapIndex, fNewType, &ubLastHighLevel)) {
    AddToUndoList(iMapIndex);

    // - For all heigher level, remove
    RemoveHigherLandLevels(iMapIndex, fNewType, puiDeletedTypes, ubNumTypes);

    // Set with a radius of 1 and smooth according to height difference
    SetLowerLandIndexWithRadius(iMapIndex, fNewType, 1, TRUE);

    // Smooth all deleted levels
    for (cnt = 0; cnt < ubNumTypes; cnt++) {
      SmoothTerrainRadius(iMapIndex, puiDeletedTypes[cnt], 1, TRUE);
    }

    MemFree(puiDeletedTypes);

  } else if (iMapIndex < 0x8000) {
    AddToUndoList(iMapIndex);

    uint16_t NewTile = GetTileIndexFromTypeSubIndex(fNewType, REQUIRES_SMOOTHING_TILE);
    SetLandIndex(iMapIndex, NewTile, fNewType);

    // Smooth item then adding here
    SmoothTerrain(iMapIndex, fNewType, &NewTile, FALSE);

    if (NewTile != NO_TILE) {
      // Change tile
      SetLandIndex(iMapIndex, NewTile, fNewType);
    }
  }
}

static BOOLEAN PasteExistingTexture(uint32_t iMapIndex, uint16_t usIndex) {
  uint16_t usNewIndex;

  // If here, we want to make, esentially, what is a type in
  // a level other than TOP-MOST the TOP-MOST level.
  // We should:
  // - remove what was top-most
  // - re-adjust the world to reflect missing top-most peice

  if (iMapIndex >= 0x8000) return (FALSE);

  // if (TypeRangeExistsInLandLayer(iMapIndex, FIRSTFLOOR, LASTFLOOR))
  //	return( FALSE );

  // Get top tile index
  // Remove all land peices except
  const uint32_t uiNewType = GetTileType(usIndex);

  DeleteAllLandLayers(iMapIndex);

  // ADD BASE LAND AT LEAST!
  usNewIndex = (uint16_t)(rand() % 10);

  // Adjust for type
  usNewIndex += gTileTypeStartIndex[gCurrentBackground];

  // Set land index
  AddLandToHead(iMapIndex, usNewIndex);

  SetLandIndex(iMapIndex, usIndex, uiNewType);

  // ATE: Set this land peice to require smoothing again!
  SmoothAllTerrainTypeRadius(iMapIndex, 2, TRUE);

  return (TRUE);
}

//	Puts a land index "under" an existing ground texture. Affects a radial
// area.
static BOOLEAN SetLowerLandIndexWithRadius(int32_t iMapIndex, uint32_t uiNewType, uint8_t ubRadius,
                                           BOOLEAN fReplace) {
  int16_t sTop, sBottom;
  int16_t sLeft, sRight;
  int16_t cnt1, cnt2;
  int32_t iNewIndex;
  BOOLEAN fDoPaste = FALSE;
  int32_t leftmost;
  uint8_t ubLastHighLevel;
  uint32_t *puiSmoothTiles = NULL;
  int16_t sNumSmoothTiles = 0;
  uint16_t usTemp;

  // Determine start end end indicies and num rows
  sTop = ubRadius;
  sBottom = -ubRadius;
  sLeft = -ubRadius;
  sRight = ubRadius;

  if (iMapIndex >= 0x8000) return (FALSE);

  for (cnt1 = sBottom; cnt1 <= sTop; cnt1++) {
    leftmost = ((iMapIndex + (WORLD_COLS * cnt1)) / WORLD_COLS) * WORLD_COLS;

    for (cnt2 = sLeft; cnt2 <= sRight; cnt2++) {
      iNewIndex = iMapIndex + (WORLD_COLS * cnt1) + cnt2;

      if (iNewIndex >= 0 && iNewIndex < WORLD_MAX && iNewIndex >= leftmost &&
          iNewIndex < (leftmost + WORLD_COLS)) {
        if (fReplace) {
          fDoPaste = TRUE;
        } else {
          if (FindTypeInLandLayer(iNewIndex, uiNewType)) {
            fDoPaste = TRUE;
          }
        }

        // if (fDoPaste && !TypeRangeExistsInLandLayer(iMapIndex, FIRSTFLOOR,
        // LASTFLOOR))
        if (fDoPaste) {
          if (iMapIndex == iNewIndex) {
            AddToUndoList(iMapIndex);

            // Force middle one to NOT smooth, and set to random 'full' tile
            usTemp = (rand() % 10) + 1;
            uint16_t NewTile = GetTileIndexFromTypeSubIndex(uiNewType, usTemp);
            SetLandIndex(iNewIndex, NewTile, uiNewType);
          } else if (AnyHeigherLand(iNewIndex, uiNewType, &ubLastHighLevel)) {
            AddToUndoList(iMapIndex);

            // Force middle one to NOT smooth, and set to random 'full' tile
            usTemp = (rand() % 10) + 1;
            uint16_t NewTile = GetTileIndexFromTypeSubIndex(uiNewType, usTemp);
            SetLandIndex(iNewIndex, NewTile, uiNewType);
          } else {
            AddToUndoList(iMapIndex);

            // Set tile to 'smooth target' tile
            uint16_t NewTile = GetTileIndexFromTypeSubIndex(uiNewType, REQUIRES_SMOOTHING_TILE);
            SetLandIndex(iNewIndex, NewTile, uiNewType);

            // If we are top-most, add to smooth list
            sNumSmoothTiles++;
            puiSmoothTiles = REALLOC(puiSmoothTiles, uint32_t, sNumSmoothTiles);
            puiSmoothTiles[sNumSmoothTiles - 1] = iNewIndex;
          }
        }
      }
    }
  }

  // Once here, smooth any tiles that need it
  if (sNumSmoothTiles > 0) {
    for (cnt1 = 0; cnt1 < sNumSmoothTiles; cnt1++) {
      SmoothTerrainRadius(puiSmoothTiles[cnt1], uiNewType, 10, FALSE);
    }
    MemFree(puiSmoothTiles);
  }

  return (TRUE);
}

// ATE FIXES
static void PasteTextureEx(GridNo const grid_no, uint16_t const type) {
  // Check if this texture exists
  if (LEVELNODE const *const land = FindTypeInLandLayer(grid_no, type)) {
    uint8_t type_level;
    if (GetTypeLandLevel(grid_no, type, &type_level)) {
      // If top-land, do not change
      if (type_level != LANDHEAD) {
        PasteExistingTexture(grid_no, land->usIndex);
      }
    }
  } else {
    // Fill with just first tile, smoothworld() will pick proper piece later
    uint16_t const new_tile = GetTileIndexFromTypeSubIndex(type, REQUIRES_SMOOTHING_TILE);
    SetLandIndex(grid_no, new_tile, type);
  }
}

// FUNCTION TO GIVE NEAREST GRIDNO OF A CLIFF
#define LAND_DROP_1 FIRSTCLIFF1
#define LAND_DROP_2 FIRSTCLIFF11
#define LAND_DROP_3 FIRSTCLIFF12
#define LAND_DROP_4 FIRSTCLIFF15
#define LAND_DROP_5 FIRSTCLIFF8
void RaiseWorldLand() {
  int32_t cnt;
  uint32_t sTempGridNo;
  LEVELNODE *pStruct;
  TILE_ELEMENT *pTileElement;
  BOOLEAN fRaiseSet;
  BOOLEAN fSomethingRaised = FALSE;
  uint8_t ubLoop;
  int32_t iStartNumberOfRaises = 0;
  int32_t iNumberOfRaises = 0;
  BOOLEAN fAboutToRaise = FALSE;

  fRaiseSet = FALSE;

  FOR_EACH_WORLD_TILE(i) {
    i->uiFlags &= ~MAPELEMENT_RAISE_LAND_START;
    i->uiFlags &= ~MAPELEMENT_RAISE_LAND_END;
  }

  uint16_t usIndex = (uint16_t)-1;  // XXX HACK000E
  for (cnt = 0; cnt < WORLD_MAX; cnt++) {
    // Get Structure levelnode
    pStruct = gpWorldLevelData[cnt].pStructHead;
    gpWorldLevelData[cnt].sHeight = 0;

    while (pStruct) {
      pTileElement = &(gTileDatabase[pStruct->usIndex]);
      if (pTileElement->fType == FIRSTCLIFF) {
        fSomethingRaised = TRUE;
        // DebugMsg(TOPIC_JA2,DBG_LEVEL_3,String("Cliff found at
        // count=%d",cnt));
        if (pTileElement->ubNumberOfTiles > 1) {
          // DebugMsg(TOPIC_JA2,DBG_LEVEL_3,String("Cliff has %d children",
          // pTileElement->ubNumberOfTiles));
          for (ubLoop = 0; ubLoop < pTileElement->ubNumberOfTiles; ubLoop++) {
            usIndex = pStruct->usIndex;
            // need means to turn land raising on and off based on the tile ID
            // and the offset in the tile database when reading into the
            // mapsystem turning off of land raising can only be done presently
            // by CLIFF object/tileset 1 so simply detect this tile set and turn
            // off instead of on element 1 is 12 tiles and is unique

            sTempGridNo = cnt + pTileElement->pTileLocData[ubLoop].bTileOffsetX +
                          pTileElement->pTileLocData[ubLoop].bTileOffsetY * WORLD_COLS;
            // Check for valid gridno
            if (OutOfBounds((int16_t)cnt, (int16_t)sTempGridNo)) {
              continue;
            }
            // if (pTileElement->ubNumberOfTiles==10)
            if ((usIndex == LAND_DROP_1) || (usIndex == LAND_DROP_2) || (usIndex == LAND_DROP_4)) {
              gpWorldLevelData[sTempGridNo].uiFlags &= (~MAPELEMENT_RAISE_LAND_START);
              gpWorldLevelData[sTempGridNo].uiFlags |= MAPELEMENT_RAISE_LAND_END;
            } else if ((usIndex == LAND_DROP_5) && (ubLoop == 4)) {
              gpWorldLevelData[sTempGridNo].uiFlags &= (~MAPELEMENT_RAISE_LAND_START);
              gpWorldLevelData[sTempGridNo].uiFlags |= MAPELEMENT_RAISE_LAND_END;
              if (!(gpWorldLevelData[sTempGridNo + 1].uiFlags & MAPELEMENT_RAISE_LAND_START)) {
                gpWorldLevelData[sTempGridNo + 1].uiFlags |= MAPELEMENT_RAISE_LAND_START;
              }
            } else if ((usIndex == LAND_DROP_3) &&
                       ((ubLoop == 0) || (ubLoop == 1) || (ubLoop == 2))) {
              gpWorldLevelData[sTempGridNo].uiFlags &= (~MAPELEMENT_RAISE_LAND_START);
              gpWorldLevelData[sTempGridNo].uiFlags |= MAPELEMENT_RAISE_LAND_END;
            } else {
              gpWorldLevelData[sTempGridNo].uiFlags |= MAPELEMENT_RAISE_LAND_START;
            }
          }
        } else {
          if (usIndex == LAND_DROP_3) {
            gpWorldLevelData[cnt].uiFlags &= (~MAPELEMENT_RAISE_LAND_START);
            gpWorldLevelData[cnt].uiFlags |= MAPELEMENT_RAISE_LAND_END;
          } else {
            // if (pTileElement->ubNumberOfTiles==10)
            if (usIndex == LAND_DROP_1) {
              gpWorldLevelData[cnt].uiFlags &= (~MAPELEMENT_RAISE_LAND_START);
              gpWorldLevelData[cnt].uiFlags |= MAPELEMENT_RAISE_LAND_END;
            } else
              gpWorldLevelData[cnt].uiFlags |= MAPELEMENT_RAISE_LAND_START;
          }
        }
      }
      pStruct = pStruct->pNext;
    }
  }

  // no cliffs?
  if (!fSomethingRaised) return;

  // run through again, this pass is for placing raiselandstart in rows that
  // have raiseland end but no raiselandstart
  for (cnt = WORLD_MAX - 1; cnt >= 0; cnt--) {
    if (cnt % WORLD_ROWS == WORLD_ROWS - 1) {
      // start of new row
      fRaiseSet = FALSE;
    }
    if (gpWorldLevelData[cnt].uiFlags & MAPELEMENT_RAISE_LAND_START) {
      fRaiseSet = TRUE;
    } else if ((gpWorldLevelData[cnt].uiFlags & MAPELEMENT_RAISE_LAND_END) && (!fRaiseSet)) {
      // there is a dropoff without a rise.
      // back up and set beginning to raiseland start
      gpWorldLevelData[cnt + ((WORLD_ROWS - 1) - (cnt % WORLD_ROWS))].uiFlags &=
          (~MAPELEMENT_RAISE_LAND_END);
      gpWorldLevelData[cnt + ((WORLD_ROWS - 1) - (cnt % WORLD_ROWS))].uiFlags |=
          MAPELEMENT_RAISE_LAND_START;
      if (cnt + ((WORLD_ROWS - 1) - (cnt % WORLD_ROWS)) - WORLD_ROWS > 0) {
        gpWorldLevelData[cnt + ((WORLD_ROWS - 1) - (cnt % WORLD_ROWS)) - WORLD_ROWS].uiFlags &=
            (~MAPELEMENT_RAISE_LAND_END);
        gpWorldLevelData[cnt + ((WORLD_ROWS - 1) - (cnt % WORLD_ROWS)) - WORLD_ROWS].uiFlags |=
            MAPELEMENT_RAISE_LAND_START;
      }
      fRaiseSet = TRUE;
    }
  }
  fRaiseSet = FALSE;
  // Look for a cliff face that is along either the lower edge or the right edge
  // of the map, this is used for a special case fill start at y=159, x= 80 and
  // go to x=159, y=80

  // now check along x=159, y=80 to x=80, y=0
  for (cnt = ((WORLD_COLS * WORLD_ROWS) - (WORLD_ROWS / 2) * (WORLD_ROWS - 2) - 1);
       cnt > WORLD_ROWS - 1; cnt -= (WORLD_ROWS + 1)) {
    if (fAboutToRaise) {
      fRaiseSet = TRUE;
      fAboutToRaise = FALSE;
    }

    if ((gpWorldLevelData[cnt].uiFlags & MAPELEMENT_RAISE_LAND_START) ||
        (gpWorldLevelData[cnt - 1].uiFlags & MAPELEMENT_RAISE_LAND_START) ||
        (gpWorldLevelData[cnt + 1].uiFlags & MAPELEMENT_RAISE_LAND_START)) {
      fAboutToRaise = TRUE;
      fRaiseSet = FALSE;
    } else if ((gpWorldLevelData[cnt].uiFlags & MAPELEMENT_RAISE_LAND_END) ||
               (gpWorldLevelData[cnt - 1].uiFlags & MAPELEMENT_RAISE_LAND_END) ||
               (gpWorldLevelData[cnt + 1].uiFlags & MAPELEMENT_RAISE_LAND_END)) {
      fRaiseSet = FALSE;
    }
    if (fRaiseSet) {
      gpWorldLevelData[cnt + ((WORLD_ROWS - 1) - (cnt % WORLD_ROWS))].uiFlags |=
          MAPELEMENT_RAISE_LAND_START;
      // gpWorldLevelData[cnt].uiFlags|=MAPELEMENT_RAISE_LAND_START;
      // gpWorldLevelData[cnt-1].uiFlags|=MAPELEMENT_RAISE_LAND_START;
      // DebugMsg(TOPIC_JA2, DBG_LEVEL_3, String("Land Raise start at count: %d
      // is raised",cnt )); DebugMsg(TOPIC_JA2, DBG_LEVEL_3, String("Land Raise
      // start at count: %d is raised",cnt - 1 ));
    }
  }

  // fRaiseSet = FALSE;

  // Now go through the world, starting at x=max(x) and y=max(y) and work
  // backwards if a cliff is found turn raise flag on, if the end of a screen is
  // found, turn off the system uses world_cord=x+y*(row_size)

  for (cnt = WORLD_MAX - 1; cnt >= 0; cnt--) {
    //  reset the RAISE to FALSE
    // End of the row
    if (!(cnt % WORLD_ROWS)) {
      iNumberOfRaises = 0;
      iStartNumberOfRaises = 0;
    }

    if ((gpWorldLevelData[cnt].uiFlags & MAPELEMENT_RAISE_LAND_END)) {
      if (cnt > 1) {
        if ((!(gpWorldLevelData[cnt - 1].uiFlags & MAPELEMENT_RAISE_LAND_END) &&
             !(gpWorldLevelData[cnt - 2].uiFlags & MAPELEMENT_RAISE_LAND_END))) {
          iNumberOfRaises--;
        }
      }
    } else if (gpWorldLevelData[cnt].uiFlags & MAPELEMENT_RAISE_LAND_START) {
      // check tile before and after, if either are raise land flagged, then
      // don't increment number of raises
      if (cnt < WORLD_MAX - 2) {
        if ((!(gpWorldLevelData[cnt + 1].uiFlags & MAPELEMENT_RAISE_LAND_START) &&
             !(gpWorldLevelData[cnt + 2].uiFlags & MAPELEMENT_RAISE_LAND_START))) {
          iNumberOfRaises++;
        }
      }
    }

    // look at number of raises.. if negative, then we have more downs than ups,
    // restart row with raises + 1; now raise land of any tile while the flag is
    // on
    if (iNumberOfRaises < 0) {
      // something wrong, reset cnt
      iStartNumberOfRaises++;
      cnt += WORLD_ROWS - cnt % WORLD_ROWS;
      iNumberOfRaises = iStartNumberOfRaises;
      continue;
    }

    if (iNumberOfRaises >= 0) {
      // DebugMsg(TOPIC_JA2, DBG_LEVEL_3, String("Land Raise start at count: %d
      // is raised",cnt ));
      gpWorldLevelData[cnt].sHeight = iNumberOfRaises * WORLD_CLIFF_HEIGHT;
    }
  }

  for (cnt = WORLD_MAX - 1; cnt >= 0; cnt--) {
    if ((cnt < WORLD_MAX - WORLD_ROWS) && (cnt > WORLD_ROWS)) {
      if ((gpWorldLevelData[cnt + WORLD_ROWS].sHeight ==
           gpWorldLevelData[cnt - WORLD_ROWS].sHeight) &&
          (gpWorldLevelData[cnt].sHeight != gpWorldLevelData[cnt - WORLD_ROWS].sHeight)) {
        gpWorldLevelData[cnt].sHeight = gpWorldLevelData[cnt + WORLD_ROWS].sHeight;
      } else if ((gpWorldLevelData[cnt].sHeight > gpWorldLevelData[cnt - WORLD_ROWS].sHeight) &&
                 (gpWorldLevelData[cnt + WORLD_ROWS].sHeight !=
                  gpWorldLevelData[cnt - WORLD_ROWS].sHeight) &&
                 (gpWorldLevelData[cnt].sHeight > gpWorldLevelData[cnt + WORLD_ROWS].sHeight)) {
        if (gpWorldLevelData[cnt - WORLD_ROWS].sHeight >
            gpWorldLevelData[cnt + WORLD_ROWS].sHeight) {
          gpWorldLevelData[cnt].sHeight = gpWorldLevelData[cnt - WORLD_ROWS].sHeight;
        } else {
          gpWorldLevelData[cnt].sHeight = gpWorldLevelData[cnt + WORLD_ROWS].sHeight;
        }
      }
    }
  }

  //*/
}
