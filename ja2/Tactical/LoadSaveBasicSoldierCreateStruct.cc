// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Tactical/LoadSaveBasicSoldierCreateStruct.h"

#include "Macro.h"
#include "SGP/Debug.h"
#include "SGP/FileMan.h"
#include "SGP/LoadSaveData.h"
#include "Tactical/SoldierCreate.h"

void ExtractBasicSoldierCreateStructFromFile(HWFILE const f, BASIC_SOLDIERCREATE_STRUCT &b) {
  uint8_t data[52];
  FileRead(f, data, sizeof(data));

  uint8_t const *d = data;
  EXTR_BOOL(d, b.fDetailedPlacement)
  EXTR_SKIP(d, 1)
  EXTR_U16(d, b.usStartingGridNo)
  EXTR_I8(d, b.bTeam)
  EXTR_I8(d, b.bRelativeAttributeLevel)
  EXTR_I8(d, b.bRelativeEquipmentLevel)
  EXTR_I8(d, b.bDirection)
  EXTR_I8(d, b.bOrders)
  EXTR_I8(d, b.bAttitude)
  EXTR_I8(d, b.bBodyType)
  EXTR_SKIP(d, 1)
  EXTR_I16A(d, b.sPatrolGrid, lengthof(b.sPatrolGrid))
  EXTR_I8(d, b.bPatrolCnt)
  EXTR_BOOL(d, b.fOnRoof)
  EXTR_U8(d, b.ubSoldierClass)
  EXTR_U8(d, b.ubCivilianGroup)
  EXTR_BOOL(d, b.fPriorityExistance)
  EXTR_BOOL(d, b.fHasKeys)
  EXTR_SKIP(d, 14)
  Assert(d == endof(data));
}

void InjectBasicSoldierCreateStructIntoFile(HWFILE const f, BASIC_SOLDIERCREATE_STRUCT const &b) {
  uint8_t data[52];
  uint8_t *d = data;
  INJ_BOOL(d, b.fDetailedPlacement)
  INJ_SKIP(d, 1)
  INJ_U16(d, b.usStartingGridNo)
  INJ_I8(d, b.bTeam)
  INJ_I8(d, b.bRelativeAttributeLevel)
  INJ_I8(d, b.bRelativeEquipmentLevel)
  INJ_I8(d, b.bDirection)
  INJ_I8(d, b.bOrders)
  INJ_I8(d, b.bAttitude)
  INJ_I8(d, b.bBodyType)
  INJ_SKIP(d, 1)
  INJ_I16A(d, b.sPatrolGrid, lengthof(b.sPatrolGrid))
  INJ_I8(d, b.bPatrolCnt)
  INJ_BOOL(d, b.fOnRoof)
  INJ_U8(d, b.ubSoldierClass)
  INJ_U8(d, b.ubCivilianGroup)
  INJ_BOOL(d, b.fPriorityExistance)
  INJ_BOOL(d, b.fHasKeys)
  INJ_SKIP(d, 14)
  Assert(d == endof(data));

  FileWrite(f, data, sizeof(data));
}
