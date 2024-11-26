#include "TileEngine/SaveLoadMap.h"

#include <string.h>

#include "GameSettings.h"
#include "SGP/Buffer.h"
#include "SGP/Debug.h"
#include "SGP/FileMan.h"
#include "SGP/MemMan.h"
#include "SGP/Types.h"
#include "Strategic/CampaignTypes.h"
#include "Strategic/StrategicMap.h"
#include "Tactical/FOV.h"
#include "Tactical/HandleItems.h"
#include "Tactical/Overhead.h"
#include "Tactical/TacticalSave.h"
#include "TileEngine/ExitGrids.h"
#include "TileEngine/RenderFun.h"
#include "TileEngine/Smell.h"
#include "TileEngine/Structure.h"
#include "TileEngine/TileDef.h"
#include "TileEngine/WorldDef.h"
#include "TileEngine/WorldMan.h"
#include "Utils/FontControl.h"
#include "Utils/Message.h"

#define NUM_REVEALED_BYTES 3200

extern BOOLEAN gfLoadingExitGrids;

bool ApplyMapChangesToMapTempFile::active_ = false;

//  There are 3200 bytes, and each bit represents the revelaed status.
//	3200 bytes * 8 bits = 25600 map elements
uint8_t *gpRevealedMap;

static void SaveModifiedMapStructToMapTempFile(MODIFY_MAP const *const pMap, int16_t const sSectorX,
                                               int16_t const sSectorY, int8_t const bSectorZ) {
  char zMapName[128];

  GetMapTempFileName(SF_MAP_MODIFICATIONS_TEMP_FILE_EXISTS, zMapName, sSectorX, sSectorY, bSectorZ);

  AutoSGPFile hFile(FileMan::openForAppend(zMapName));
  FileWrite(hFile, pMap, sizeof(MODIFY_MAP));

  SetSectorFlag(sSectorX, sSectorY, bSectorZ, SF_MAP_MODIFICATIONS_TEMP_FILE_EXISTS);
}

static void AddBloodOrSmellFromMapTempFileToMap(MODIFY_MAP *pMap);
static void AddObjectFromMapTempFileToMap(uint32_t uiMapIndex, uint16_t usIndex);
static void DamageStructsFromMapTempFile(MODIFY_MAP *pMap);
static bool ModifyWindowStatus(GridNo);
static void RemoveSavedStructFromMap(uint32_t uiMapIndex, uint16_t usIndex);
static void SetOpenableStructStatusFromMapTempFile(uint32_t uiMapIndex, BOOLEAN fOpened);

void LoadAllMapChangesFromMapTempFileAndApplyThem() {
  char zMapName[128];
  uint32_t uiNumberOfElementsSavedBackToFile = 0;  // added becuase if no files get saved back to
                                                   // disk, the flag needs to be erased
  uint32_t cnt;
  MODIFY_MAP *pMap;

  GetMapTempFileName(SF_MAP_MODIFICATIONS_TEMP_FILE_EXISTS, zMapName, gWorldSectorX, gWorldSectorY,
                     gbWorldSectorZ);

  // If the file doesnt exists, its no problem.
  if (!FileExists(zMapName)) return;

  uint32_t uiNumberOfElements;
  SGP::Buffer<MODIFY_MAP> pTempArrayOfMaps;
  {
    AutoSGPFile hFile(FileMan::openForReadingSmart(zMapName, true));

    // Get the size of the file
    uiNumberOfElements = FileGetSize(hFile) / sizeof(MODIFY_MAP);

    // Read the map temp file into a buffer
    pTempArrayOfMaps.Allocate(uiNumberOfElements);
    FileRead(hFile, pTempArrayOfMaps, sizeof(*pTempArrayOfMaps) * uiNumberOfElements);
  }

  // Delete the file
  FileDelete(zMapName);

  for (cnt = 0; cnt < uiNumberOfElements; cnt++) {
    pMap = &pTempArrayOfMaps[cnt];

    // Switch on the type that should either be added or removed from the map
    switch (pMap->ubType) {
      // If we are adding to the map
      case SLM_LAND:
        break;
      case SLM_OBJECT: {
        uint16_t usIndex = GetTileIndexFromTypeSubIndex(pMap->usImageType, pMap->usSubImageIndex);
        AddObjectFromMapTempFileToMap(pMap->usGridNo, usIndex);

        // Save this struct back to the temp file
        SaveModifiedMapStructToMapTempFile(pMap, gWorldSectorX, gWorldSectorY, gbWorldSectorZ);

        // Since the element is being saved back to the temp file, increment the #
        uiNumberOfElementsSavedBackToFile++;
        break;
      }

      case SLM_STRUCT: {
        uint16_t const usIndex =
            GetTileIndexFromTypeSubIndex(pMap->usImageType, pMap->usSubImageIndex);
        if (!IndexExistsInStructLayer(pMap->usGridNo, usIndex)) {
          AddStructToTail(pMap->usGridNo, usIndex);
        }

        // Save this struct back to the temp file
        SaveModifiedMapStructToMapTempFile(pMap, gWorldSectorX, gWorldSectorY, gbWorldSectorZ);

        // Since the element is being saved back to the temp file, increment the #
        uiNumberOfElementsSavedBackToFile++;
        break;
      }

      case SLM_SHADOW:
        break;
      case SLM_MERC:
        break;
      case SLM_ROOF:
        break;
      case SLM_ONROOF:
        break;
      case SLM_TOPMOST:
        break;

      // Remove objects out of the world
      case SLM_REMOVE_LAND:
        break;
      case SLM_REMOVE_OBJECT:
        break;
      case SLM_REMOVE_STRUCT:

        // ATE: OK, dor doors, the usIndex can be varied, opened, closed, etc
        // we MUSTR delete ANY door type on this gridno
        // Since we can only have one door per gridno, we're safe to do so.....
        if (pMap->usImageType >= FIRSTDOOR && pMap->usImageType <= FOURTHDOOR) {
          // Remove ANY door...
          RemoveAllStructsOfTypeRange(pMap->usGridNo, FIRSTDOOR, FOURTHDOOR);
        } else {
          uint16_t usIndex = GetTileIndexFromTypeSubIndex(pMap->usImageType, pMap->usSubImageIndex);
          RemoveSavedStructFromMap(pMap->usGridNo, usIndex);
        }

        // Save this struct back to the temp file
        SaveModifiedMapStructToMapTempFile(pMap, gWorldSectorX, gWorldSectorY, gbWorldSectorZ);

        // Since the element is being saved back to the temp file, increment the #
        uiNumberOfElementsSavedBackToFile++;
        break;
      case SLM_REMOVE_SHADOW:
        break;
      case SLM_REMOVE_MERC:
        break;
      case SLM_REMOVE_ROOF:
        break;
      case SLM_REMOVE_ONROOF:
        break;
      case SLM_REMOVE_TOPMOST:
        break;

      case SLM_BLOOD_SMELL:
        AddBloodOrSmellFromMapTempFileToMap(pMap);
        break;

      case SLM_DAMAGED_STRUCT:
        DamageStructsFromMapTempFile(pMap);
        break;

      case SLM_EXIT_GRIDS: {
        EXITGRID ExitGrid;
        gfLoadingExitGrids = TRUE;
        ExitGrid.usGridNo = pMap->usSubImageIndex;
        ExitGrid.ubGotoSectorX = (uint8_t)pMap->usImageType;
        ExitGrid.ubGotoSectorY = (uint8_t)(pMap->usImageType >> 8);
        ExitGrid.ubGotoSectorZ = pMap->ubExtra;

        AddExitGridToWorld(pMap->usGridNo, &ExitGrid);
        gfLoadingExitGrids = FALSE;

        // Save this struct back to the temp file
        SaveModifiedMapStructToMapTempFile(pMap, gWorldSectorX, gWorldSectorY, gbWorldSectorZ);

        // Since the element is being saved back to the temp file, increment the #
        uiNumberOfElementsSavedBackToFile++;
      } break;

      case SLM_OPENABLE_STRUCT:
        SetOpenableStructStatusFromMapTempFile(pMap->usGridNo, (BOOLEAN)pMap->usImageType);
        break;

      case SLM_WINDOW_HIT:
        if (ModifyWindowStatus(pMap->usGridNo)) {
          // Save this struct back to the temp file
          SaveModifiedMapStructToMapTempFile(pMap, gWorldSectorX, gWorldSectorY, gbWorldSectorZ);

          // Since the element is being saved back to the temp file, increment the
          // #
          uiNumberOfElementsSavedBackToFile++;
        }
        break;

      default:
        AssertMsg(0,
                  "ERROR!  Map Type not in switch when loading map changes "
                  "from temp file");
        break;
    }
  }

  // if no elements are saved back to the file, remove the flag indicating that
  // there is a temp file
  if (uiNumberOfElementsSavedBackToFile == 0) {
    ReSetSectorFlag(gWorldSectorX, gWorldSectorY, gbWorldSectorZ,
                    SF_MAP_MODIFICATIONS_TEMP_FILE_EXISTS);
  }
}

static void AddToMapTempFile(uint32_t const uiMapIndex, uint16_t const usIndex,
                             uint8_t const type) {
  if (!ApplyMapChangesToMapTempFile::IsActive()) return;
  if (gTacticalStatus.uiFlags & LOADING_SAVED_GAME) return;

  uint32_t const uiType = GetTileType(usIndex);
  uint16_t const usSubIndex = GetSubIndexFromTileIndex(usIndex);

  MODIFY_MAP m;
  memset(&m, 0, sizeof(m));
  m.usGridNo = uiMapIndex;
  m.usImageType = uiType;
  m.usSubImageIndex = usSubIndex;
  m.ubType = type;
  SaveModifiedMapStructToMapTempFile(&m, gWorldSectorX, gWorldSectorY, gbWorldSectorZ);
}

void AddStructToMapTempFile(uint32_t const uiMapIndex, uint16_t const usIndex) {
  AddToMapTempFile(uiMapIndex, usIndex, SLM_STRUCT);
}

void AddObjectToMapTempFile(uint32_t const uiMapIndex, uint16_t const usIndex) {
  AddToMapTempFile(uiMapIndex, usIndex, SLM_OBJECT);
}

static void AddObjectFromMapTempFileToMap(uint32_t uiMapIndex, uint16_t usIndex) {
  AddObjectToHead(uiMapIndex, usIndex);
}

void AddRemoveObjectToMapTempFile(uint32_t const uiMapIndex, uint16_t const usIndex) {
  AddToMapTempFile(uiMapIndex, usIndex, SLM_REMOVE_OBJECT);
}

void RemoveStructFromMapTempFile(uint32_t const uiMapIndex, uint16_t const usIndex) {
  AddToMapTempFile(uiMapIndex, usIndex, SLM_REMOVE_STRUCT);
}

static void RemoveSavedStructFromMap(uint32_t uiMapIndex, uint16_t usIndex) {
  RemoveStruct(uiMapIndex, usIndex);
}

static void AddOpenableStructStatusToMapTempFile(uint32_t uiMapIndex, BOOLEAN fOpened);
static void SetSectorsRevealedBit(uint16_t usMapIndex);

void SaveBloodSmellAndRevealedStatesFromMapToTempFile() {
  MODIFY_MAP Map;
  uint16_t cnt;
  STRUCTURE *pStructure;

  gpRevealedMap = MALLOCNZ(uint8_t, NUM_REVEALED_BYTES);

  // Loop though all the map elements
  for (cnt = 0; cnt < WORLD_MAX; cnt++) {
    // if there is either blood or a smell on the tile, save it
    if (gpWorldLevelData[cnt].ubBloodInfo || gpWorldLevelData[cnt].ubSmellInfo) {
      memset(&Map, 0, sizeof(MODIFY_MAP));

      // Save the BloodInfo in the bottom byte and the smell info in the upper
      // byte
      Map.usGridNo = cnt;
      //			Map.usIndex			=
      // gpWorldLevelData[cnt].ubBloodInfo | ( gpWorldLevelData[cnt].ubSmellInfo
      //<< 8 );
      Map.usImageType = gpWorldLevelData[cnt].ubBloodInfo;
      Map.usSubImageIndex = gpWorldLevelData[cnt].ubSmellInfo;

      Map.ubType = SLM_BLOOD_SMELL;

      // Save the change to the map file
      SaveModifiedMapStructToMapTempFile(&Map, gWorldSectorX, gWorldSectorY, gbWorldSectorZ);
    }

    // if the element has been revealed
    if (gpWorldLevelData[cnt].uiFlags & MAPELEMENT_REVEALED) {
      SetSectorsRevealedBit(cnt);
    }

    // if there is a structure that is damaged
    if (gpWorldLevelData[cnt].uiFlags & MAPELEMENT_STRUCTURE_DAMAGED) {
      // loop through all the structures and add all that are damaged
      FOR_EACH_STRUCTURE(pCurrent, cnt, STRUCTURE_BASE_TILE) {
        // if the structure has been damaged
        if (pCurrent->ubHitPoints < pCurrent->pDBStructureRef->pDBStructure->ubHitPoints) {
          uint8_t ubBitToSet = 0x80;
          uint8_t ubLevel = 0;

          if (pCurrent->sCubeOffset != 0) ubLevel |= ubBitToSet;

          memset(&Map, 0, sizeof(MODIFY_MAP));

          // Save the Damaged value
          Map.usGridNo = cnt;
          //					Map.usIndex			=
          // StructureFlagToType( pCurrent->fFlags ) | ( pCurrent->ubHitPoints << 8 );
          Map.usImageType = StructureFlagToType(pCurrent->fFlags);
          Map.usSubImageIndex = pCurrent->ubHitPoints;

          Map.ubType = SLM_DAMAGED_STRUCT;
          Map.ubExtra = pCurrent->ubWallOrientation | ubLevel;

          // Save the change to the map file
          SaveModifiedMapStructToMapTempFile(&Map, gWorldSectorX, gWorldSectorY, gbWorldSectorZ);
        }
      }
    }

    pStructure = FindStructure(cnt, STRUCTURE_OPENABLE);

    // if this structure
    if (pStructure) {
      // if the current structure has an openable structure in it, and it is NOT
      // a door
      if (!(pStructure->fFlags & STRUCTURE_ANYDOOR)) {
        BOOLEAN fStatusOnTheMap;

        fStatusOnTheMap = ((pStructure->fFlags & STRUCTURE_OPEN) != 0);

        AddOpenableStructStatusToMapTempFile(cnt, fStatusOnTheMap);
      }
    }
  }
}

// The BloodInfo is saved in the bottom byte and the smell info in the upper
// byte
static void AddBloodOrSmellFromMapTempFileToMap(MODIFY_MAP *pMap) {
  gpWorldLevelData[pMap->usGridNo].ubBloodInfo = (uint8_t)pMap->usImageType;

  // if the blood and gore option IS set, add blood
  if (gGameSettings.fOptions[TOPTION_BLOOD_N_GORE]) {
    // Update graphics for both levels...
    gpWorldLevelData[pMap->usGridNo].uiFlags |= MAPELEMENT_REEVALUATEBLOOD;
    UpdateBloodGraphics(pMap->usGridNo, 0);
    gpWorldLevelData[pMap->usGridNo].uiFlags |= MAPELEMENT_REEVALUATEBLOOD;
    UpdateBloodGraphics(pMap->usGridNo, 1);
  }

  gpWorldLevelData[pMap->usGridNo].ubSmellInfo = (uint8_t)pMap->usSubImageIndex;
}

void SaveRevealedStatusArrayToRevealedTempFile(int16_t const sSectorX, int16_t const sSectorY,
                                               int8_t const bSectorZ) {
  char zMapName[128];

  Assert(gpRevealedMap != NULL);

  GetMapTempFileName(SF_REVEALED_STATUS_TEMP_FILE_EXISTS, zMapName, sSectorX, sSectorY, bSectorZ);

  AutoSGPFile hFile(FileMan::openForWriting(zMapName));

  // Write the revealed array to the Revealed temp file
  FileWrite(hFile, gpRevealedMap, NUM_REVEALED_BYTES);

  SetSectorFlag(sSectorX, sSectorY, bSectorZ, SF_REVEALED_STATUS_TEMP_FILE_EXISTS);

  MemFree(gpRevealedMap);
  gpRevealedMap = NULL;
}

static void SetMapRevealedStatus();

void LoadRevealedStatusArrayFromRevealedTempFile() {
  char zMapName[128];

  GetMapTempFileName(SF_REVEALED_STATUS_TEMP_FILE_EXISTS, zMapName, gWorldSectorX, gWorldSectorY,
                     gbWorldSectorZ);

  // If the file doesnt exists, its no problem.
  if (!FileExists(zMapName)) return;

  {
    AutoSGPFile hFile(FileMan::openForReadingSmart(zMapName, true));

    Assert(gpRevealedMap == NULL);
    gpRevealedMap = MALLOCNZ(uint8_t, NUM_REVEALED_BYTES);

    // Load the Reveal map array structure
    FileRead(hFile, gpRevealedMap, NUM_REVEALED_BYTES);
  }

  // Loop through and set the bits in the map that are revealed
  SetMapRevealedStatus();

  MemFree(gpRevealedMap);
  gpRevealedMap = NULL;
}

static void SetSectorsRevealedBit(uint16_t usMapIndex) {
  uint16_t usByteNumber;
  uint8_t ubBitNumber;

  usByteNumber = usMapIndex / 8;
  ubBitNumber = usMapIndex % 8;

  gpRevealedMap[usByteNumber] |= 1 << ubBitNumber;
}

static void SetMapRevealedStatus() {
  uint16_t usByteCnt;
  uint8_t ubBitCnt;
  uint16_t usMapIndex;

  AssertMsg(gpRevealedMap != NULL, "gpRevealedMap is NULL.  DF 1");

  ClearSlantRoofs();

  // Loop through all bytes in the array
  for (usByteCnt = 0; usByteCnt < 3200; usByteCnt++) {
    // loop through all the bits in the byte
    for (ubBitCnt = 0; ubBitCnt < 8; ubBitCnt++) {
      usMapIndex = (usByteCnt * 8) + ubBitCnt;

      if (gpRevealedMap[usByteCnt] & (1 << ubBitCnt)) {
        gpWorldLevelData[usMapIndex].uiFlags |= MAPELEMENT_REVEALED;
        SetGridNoRevealedFlag(usMapIndex);
      } else {
        gpWorldLevelData[usMapIndex].uiFlags &= (~MAPELEMENT_REVEALED);
      }
    }
  }

  ExamineSlantRoofFOVSlots();
}

static void DamageStructsFromMapTempFile(MODIFY_MAP *pMap) {
  STRUCTURE *pCurrent = NULL;
  int8_t bLevel;
  uint8_t ubWallOrientation;
  uint8_t ubBitToSet = 0x80;
  uint8_t ubType = 0;

  // Find the base structure
  pCurrent = FindStructure((int16_t)pMap->usGridNo, STRUCTURE_BASE_TILE);

  if (pCurrent == NULL) return;

  bLevel = pMap->ubExtra & ubBitToSet;
  ubWallOrientation = pMap->ubExtra & ~ubBitToSet;
  ubType = (uint8_t)pMap->usImageType;

  // Check to see if the desired strucure node is in this tile
  pCurrent = FindStructureBySavedInfo(pMap->usGridNo, ubType, ubWallOrientation, bLevel);

  if (pCurrent != NULL) {
    // Assign the hitpoints
    pCurrent->ubHitPoints = (uint8_t)(pMap->usSubImageIndex);

    gpWorldLevelData[pCurrent->sGridNo].uiFlags |= MAPELEMENT_STRUCTURE_DAMAGED;
  }
}

void AddStructToUnLoadedMapTempFile(uint32_t uiMapIndex, uint16_t usIndex, int16_t sSectorX,
                                    int16_t sSectorY, uint8_t ubSectorZ) {
  MODIFY_MAP Map;

  if (gTacticalStatus.uiFlags & LOADING_SAVED_GAME) return;

  const uint32_t uiType = GetTileType(usIndex);
  const uint16_t usSubIndex = GetSubIndexFromTileIndex(usIndex);

  memset(&Map, 0, sizeof(MODIFY_MAP));

  Map.usGridNo = (uint16_t)uiMapIndex;
  //	Map.usIndex		= usIndex;
  Map.usImageType = (uint16_t)uiType;
  Map.usSubImageIndex = usSubIndex;

  Map.ubType = SLM_STRUCT;

  SaveModifiedMapStructToMapTempFile(&Map, sSectorX, sSectorY, ubSectorZ);
}

void RemoveStructFromUnLoadedMapTempFile(uint32_t uiMapIndex, uint16_t usIndex, int16_t sSectorX,
                                         int16_t sSectorY, uint8_t ubSectorZ) {
  MODIFY_MAP Map;

  if (gTacticalStatus.uiFlags & LOADING_SAVED_GAME) return;

  const uint32_t uiType = GetTileType(usIndex);
  const uint16_t usSubIndex = GetSubIndexFromTileIndex(usIndex);

  memset(&Map, 0, sizeof(MODIFY_MAP));

  Map.usGridNo = (uint16_t)uiMapIndex;
  //	Map.usIndex			= usIndex;
  Map.usImageType = (uint16_t)uiType;
  Map.usSubImageIndex = usSubIndex;

  Map.ubType = SLM_REMOVE_STRUCT;

  SaveModifiedMapStructToMapTempFile(&Map, sSectorX, sSectorY, ubSectorZ);
}

void AddExitGridToMapTempFile(uint16_t usGridNo, EXITGRID *pExitGrid, int16_t sSectorX,
                              int16_t sSectorY, uint8_t ubSectorZ) {
  MODIFY_MAP Map;

  if (!ApplyMapChangesToMapTempFile::IsActive()) {
    ScreenMsg(FONT_MCOLOR_WHITE, MSG_BETAVERSION,
              L"Called AddExitGridToMapTempFile() without holding "
              L"ApplyMapChangesToMapTempFile");
    return;
  }

  if (gTacticalStatus.uiFlags & LOADING_SAVED_GAME) return;

  memset(&Map, 0, sizeof(MODIFY_MAP));

  Map.usGridNo = usGridNo;
  //	Map.usIndex		= pExitGrid->ubGotoSectorX;

  Map.usImageType = pExitGrid->ubGotoSectorX | (pExitGrid->ubGotoSectorY << 8);
  Map.usSubImageIndex = pExitGrid->usGridNo;

  Map.ubExtra = pExitGrid->ubGotoSectorZ;
  Map.ubType = SLM_EXIT_GRIDS;

  SaveModifiedMapStructToMapTempFile(&Map, sSectorX, sSectorY, ubSectorZ);
}

BOOLEAN RemoveGraphicFromTempFile(uint32_t uiMapIndex, uint16_t usIndex, int16_t sSectorX,
                                  int16_t sSectorY, uint8_t ubSectorZ) try {
  char zMapName[128];
  MODIFY_MAP *pMap;
  BOOLEAN fRetVal = FALSE;
  uint32_t cnt;

  GetMapTempFileName(SF_MAP_MODIFICATIONS_TEMP_FILE_EXISTS, zMapName, sSectorX, sSectorY,
                     ubSectorZ);

  uint32_t uiNumberOfElements;
  SGP::Buffer<MODIFY_MAP> pTempArrayOfMaps;
  {
    AutoSGPFile hFile(FileMan::openForReadingSmart(zMapName, true));

    // Get the number of elements in the file
    uiNumberOfElements = FileGetSize(hFile) / sizeof(MODIFY_MAP);

    // Read the map temp file into a buffer
    pTempArrayOfMaps.Allocate(uiNumberOfElements);
    FileRead(hFile, pTempArrayOfMaps, sizeof(*pTempArrayOfMaps) * uiNumberOfElements);
  }

  // Delete the file
  FileDelete(zMapName);

  // Get the image type and subindex
  const uint32_t uiType = GetTileType(usIndex);
  const uint16_t usSubIndex = GetSubIndexFromTileIndex(usIndex);

  for (cnt = 0; cnt < uiNumberOfElements; cnt++) {
    pMap = &pTempArrayOfMaps[cnt];

    // if this is the peice we are looking for
    if (pMap->usGridNo == uiMapIndex && pMap->usImageType == uiType &&
        pMap->usSubImageIndex == usSubIndex) {
      // Do nothin
      fRetVal = TRUE;
    } else {
      // save the struct back to the temp file
      SaveModifiedMapStructToMapTempFile(pMap, sSectorX, sSectorY, ubSectorZ);
    }
  }

  return (fRetVal);
} catch (...) {
  return FALSE;
}

static void AddOpenableStructStatusToMapTempFile(uint32_t uiMapIndex, BOOLEAN fOpened) {
  MODIFY_MAP Map;

  memset(&Map, 0, sizeof(MODIFY_MAP));

  Map.usGridNo = (uint16_t)uiMapIndex;
  Map.usImageType = fOpened;

  Map.ubType = SLM_OPENABLE_STRUCT;

  SaveModifiedMapStructToMapTempFile(&Map, gWorldSectorX, gWorldSectorY, gbWorldSectorZ);
}

void AddWindowHitToMapTempFile(uint32_t uiMapIndex) {
  MODIFY_MAP Map;

  memset(&Map, 0, sizeof(MODIFY_MAP));

  Map.usGridNo = (uint16_t)uiMapIndex;
  Map.ubType = SLM_WINDOW_HIT;

  SaveModifiedMapStructToMapTempFile(&Map, gWorldSectorX, gWorldSectorY, gbWorldSectorZ);
}

static bool ModifyWindowStatus(GridNo const grid_no) {
  STRUCTURE *const s = FindStructure(grid_no, STRUCTURE_WALLNWINDOW);
  if (!s) return false;  // Forget it, window could be destroyed
  SwapStructureForPartner(s);
  return true;
}

static void SetOpenableStructStatusFromMapTempFile(uint32_t uiMapIndex, BOOLEAN fOpened) {
  STRUCTURE *pStructure;
  STRUCTURE *pBase;
  BOOLEAN fStatusOnTheMap;
  int16_t sBaseGridNo = (int16_t)uiMapIndex;

  pStructure = FindStructure((uint16_t)uiMapIndex, STRUCTURE_OPENABLE);

  if (pStructure == NULL) {
    //		ScreenMsg( FONT_MCOLOR_WHITE, MSG_BETAVERSION,
    // L"SetOpenableStructStatusFromMapTempFile( %d, %d ) failed to find the
    // openable struct.  DF 1.", uiMapIndex, fOpened );
    return;
  }

  fStatusOnTheMap = ((pStructure->fFlags & STRUCTURE_OPEN) != 0);

  if (fStatusOnTheMap != fOpened) {
    // Adjust the item's gridno to the base of struct.....
    pBase = FindBaseStructure(pStructure);

    // Get LEVELNODE for struct and remove!
    if (pBase) {
      sBaseGridNo = pBase->sGridNo;
    }

    if (!SwapStructureForPartner(pStructure)) {
      // an error occured
    }

    // Adjust visiblity of any item pools here....
    // ATE: Nasty bug here - use base gridno for structure for items!
    // since items always drop to base gridno in AddItemToPool
    if (fOpened) {
      // We are open, make un-hidden if so....
      SetItemsVisibilityOn(sBaseGridNo, 0, ANY_VISIBILITY_VALUE, FALSE);
    } else {
      // Make sure items are hidden...
      SetItemsVisibilityHidden(sBaseGridNo, 0);
    }
  }
}

void ChangeStatusOfOpenableStructInUnloadedSector(uint16_t const usSectorX,
                                                  uint16_t const usSectorY, int8_t const bSectorZ,
                                                  uint16_t const usGridNo,
                                                  BOOLEAN const fChangeToOpen) {
  char map_name[128];
  GetMapTempFileName(SF_MAP_MODIFICATIONS_TEMP_FILE_EXISTS, map_name, usSectorX, usSectorY,
                     bSectorZ);

  // If the file doesn't exists, it's no problem.
  if (!FileExists(map_name)) return;

  uint32_t uiNumberOfElements;
  SGP::Buffer<MODIFY_MAP> mm;
  {
    // Read the map temp file into a buffer
    AutoSGPFile src(FileMan::openForReadingSmart(map_name, true));

    uiNumberOfElements = FileGetSize(src) / sizeof(MODIFY_MAP);

    mm.Allocate(uiNumberOfElements);
    FileRead(src, mm, sizeof(*mm) * uiNumberOfElements);
  }

  for (uint32_t i = 0; i < uiNumberOfElements; ++i) {
    MODIFY_MAP *const m = &mm[i];
    if (m->ubType != SLM_OPENABLE_STRUCT || m->usGridNo != usGridNo) continue;
    // This element is of the same type and on the same gridno

    // Change to the desired settings
    m->usImageType = fChangeToOpen;
    break;
  }

  AutoSGPFile dst(FileMan::openForWriting(map_name));
  FileWrite(dst, mm, sizeof(*mm) * uiNumberOfElements);
}
