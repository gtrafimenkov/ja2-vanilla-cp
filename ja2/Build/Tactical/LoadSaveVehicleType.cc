// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Tactical/LoadSaveVehicleType.h"

#include "Macro.h"
#include "SGP/Debug.h"
#include "SGP/FileMan.h"
#include "SGP/LoadSaveData.h"
#include "Tactical/SoldierControl.h"
#include "Tactical/SoldierProfile.h"
#include "Tactical/Vehicles.h"

void ExtractVehicleTypeFromFile(HWFILE const file, VEHICLETYPE *const v,
                                uint32_t const savegame_version) {
  uint8_t data[128];
  FileRead(file, data, sizeof(data));

  const uint8_t *d = data;
  EXTR_PTR(d, v->pMercPath)
  EXTR_U8(d, v->ubMovementGroup)
  EXTR_U8(d, v->ubVehicleType)
  EXTR_I16(d, v->sSectorX)
  EXTR_I16(d, v->sSectorY)
  EXTR_I16(d, v->sSectorZ)
  EXTR_BOOL(d, v->fBetweenSectors)
  EXTR_SKIP(d, 1)
  EXTR_I16(d, v->sGridNo);
  const ProfileID noone = (savegame_version < 86 ? 0 : NO_PROFILE);
  /* The ProfileID of the passengers gets stored, not their SoldierID */
  FOR_EACH(SOLDIERTYPE *, i, v->pPassengers) {
    ProfileID id;
    EXTR_U8(d, id)
    EXTR_SKIP(d, 3)
    *i = (id == noone ? NULL : FindSoldierByProfileID(id));
  }
  EXTR_SKIP(d, 61)
  EXTR_BOOL(d, v->fDestroyed)
  EXTR_SKIP(d, 2)
  EXTR_I32(d, v->iMovementSoundID)
  EXTR_SKIP(d, 1)
  EXTR_BOOL(d, v->fValid)
  EXTR_SKIP(d, 2)
  Assert(d == endof(data));
}

void InjectVehicleTypeIntoFile(HWFILE const file, VEHICLETYPE const *const v) {
  uint8_t data[128];

  uint8_t *d = data;
  INJ_PTR(d, v->pMercPath)
  INJ_U8(d, v->ubMovementGroup)
  INJ_U8(d, v->ubVehicleType)
  INJ_I16(d, v->sSectorX)
  INJ_I16(d, v->sSectorY)
  INJ_I16(d, v->sSectorZ)
  INJ_BOOL(d, v->fBetweenSectors)
  INJ_SKIP(d, 1)
  INJ_I16(d, v->sGridNo);
  /* The ProfileID of the passengers gets stored, not their SoldierID */
  FOR_EACH(SOLDIERTYPE *const, i, v->pPassengers) {
    const ProfileID id = (*i == NULL ? NO_PROFILE : (*i)->ubProfile);
    INJ_U8(d, id)
    INJ_SKIP(d, 3)
  }
  INJ_SKIP(d, 61)
  INJ_BOOL(d, v->fDestroyed)
  INJ_SKIP(d, 2)
  INJ_I32(d, v->iMovementSoundID)
  INJ_SKIP(d, 1)
  INJ_BOOL(d, v->fValid)
  INJ_SKIP(d, 2)
  Assert(d == endof(data));

  FileWrite(file, data, sizeof(data));
}
