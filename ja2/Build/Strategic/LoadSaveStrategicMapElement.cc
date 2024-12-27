// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Strategic/LoadSaveStrategicMapElement.h"

#include "SGP/Debug.h"
#include "SGP/FileMan.h"
#include "SGP/LoadSaveData.h"

void ExtractStrategicMapElementFromFile(HWFILE const f, StrategicMapElement &e) {
  uint8_t data[41];
  FileRead(f, data, sizeof(data));

  uint8_t const *d = data;
  EXTR_SKIP(d, 16)
  EXTR_I8(d, e.bNameId)
  EXTR_BOOL(d, e.fEnemyControlled)
  EXTR_BOOL(d, e.fEnemyAirControlled)
  EXTR_SKIP(d, 1)
  EXTR_I8(d, e.bSAMCondition)
  EXTR_SKIP(d, 20)
  Assert(d == endof(data));
}

void InjectStrategicMapElementIntoFile(HWFILE const f, StrategicMapElement const &e) {
  uint8_t data[41];
  uint8_t *d = data;
  INJ_SKIP(d, 16)
  INJ_I8(d, e.bNameId)
  INJ_BOOL(d, e.fEnemyControlled)
  INJ_BOOL(d, e.fEnemyAirControlled)
  INJ_SKIP(d, 1)
  INJ_I8(d, e.bSAMCondition)
  INJ_SKIP(d, 20)
  Assert(d == endof(data));

  FileWrite(f, data, sizeof(data));
}
