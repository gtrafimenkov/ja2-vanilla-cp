// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Strategic/LoadSaveUndergroundSectorInfo.h"

#include "SGP/Debug.h"
#include "SGP/FileMan.h"
#include "SGP/LoadSaveData.h"

void ExtractUndergroundSectorInfoFromFile(HWFILE const file, UNDERGROUND_SECTORINFO *const u) {
  uint8_t data[72];
  FileRead(file, data, sizeof(data));

  const uint8_t *d = data;
  EXTR_U32(d, u->uiFlags)
  EXTR_U8(d, u->ubSectorX)
  EXTR_U8(d, u->ubSectorY)
  EXTR_U8(d, u->ubSectorZ)
  EXTR_U8(d, u->ubNumElites)
  EXTR_U8(d, u->ubNumTroops)
  EXTR_U8(d, u->ubNumAdmins)
  EXTR_U8(d, u->ubNumCreatures)
  EXTR_SKIP(d, 5)
  EXTR_U32(d, u->uiTimeCurrentSectorWasLastLoaded)
  EXTR_PTR(d, u->next)
  EXTR_U8(d, u->ubAdjacentSectors)
  EXTR_U8(d, u->ubCreatureHabitat)
  EXTR_U8(d, u->ubElitesInBattle)
  EXTR_U8(d, u->ubTroopsInBattle)
  EXTR_U8(d, u->ubAdminsInBattle)
  EXTR_U8(d, u->ubCreaturesInBattle)
  EXTR_SKIP(d, 2)
  EXTR_U32(d, u->uiNumberOfWorldItemsInTempFileThatCanBeSeenByPlayer)
  EXTR_SKIP(d, 36)
  Assert(d == endof(data));
}

void InjectUndergroundSectorInfoIntoFile(HWFILE const file, UNDERGROUND_SECTORINFO const *const u) {
  uint8_t data[72];
  uint8_t *d = data;
  INJ_U32(d, u->uiFlags)
  INJ_U8(d, u->ubSectorX)
  INJ_U8(d, u->ubSectorY)
  INJ_U8(d, u->ubSectorZ)
  INJ_U8(d, u->ubNumElites)
  INJ_U8(d, u->ubNumTroops)
  INJ_U8(d, u->ubNumAdmins)
  INJ_U8(d, u->ubNumCreatures)
  INJ_SKIP(d, 5)
  INJ_U32(d, u->uiTimeCurrentSectorWasLastLoaded)
  INJ_PTR(d, u->next)
  INJ_U8(d, u->ubAdjacentSectors)
  INJ_U8(d, u->ubCreatureHabitat)
  INJ_U8(d, u->ubElitesInBattle)
  INJ_U8(d, u->ubTroopsInBattle)
  INJ_U8(d, u->ubAdminsInBattle)
  INJ_U8(d, u->ubCreaturesInBattle)
  INJ_SKIP(d, 2)
  INJ_U32(d, u->uiNumberOfWorldItemsInTempFileThatCanBeSeenByPlayer)
  INJ_SKIP(d, 36)
  Assert(d == endof(data));

  FileWrite(file, data, sizeof(data));
}
