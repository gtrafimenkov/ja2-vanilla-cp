// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef _EXPLOSION_CONTROL_H
#define _EXPLOSION_CONTROL_H

#include "JA2Types.h"
#include "Tactical/Weapons.h"

#define MAX_DISTANCE_EXPLOSIVE_CAN_DESTROY_STRUCTURES 2

struct EXPLOSIONTYPE {
  SOLDIERTYPE *owner;
  uint8_t ubTypeID;

  uint16_t usItem;

  int16_t sX;       // World X ( optional )
  int16_t sY;       // World Y ( optional )
  int16_t sZ;       // World Z ( optional )
  int16_t sGridNo;  // World GridNo
  int8_t bLevel;    // World level

  BOOLEAN fAllocated;
  int16_t sCurrentFrame;
  LIGHT_SPRITE *light;
};

enum EXPLOSION_TYPES {
  NO_BLAST,
  BLAST_1,
  BLAST_2,
  BLAST_3,
  STUN_BLAST,
  WATER_BLAST,
  TARGAS_EXP,
  SMOKE_EXP,
  MUSTARD_EXP,

  NUM_EXP_TYPES
};

struct ExplosionQueueElement {
  uint32_t uiWorldBombIndex;
  uint32_t uiTimeStamp;
  uint8_t fExists;
};

#define ERASE_SPREAD_EFFECT 2
#define BLOOD_SPREAD_EFFECT 3
#define REDO_SPREAD_EFFECT 4

extern uint8_t gubElementsOnExplosionQueue;
extern BOOLEAN gfExplosionQueueActive;

void IgniteExplosion(SOLDIERTYPE *owner, int16_t z, int16_t sGridNo, uint16_t item, int8_t level);
void IgniteExplosionXY(SOLDIERTYPE *owner, int16_t sX, int16_t sY, int16_t sZ, int16_t sGridNo,
                       uint16_t usItem, int8_t bLevel);
void InternalIgniteExplosion(SOLDIERTYPE *owner, int16_t sX, int16_t sY, int16_t sZ,
                             int16_t sGridNo, uint16_t usItem, BOOLEAN fLocate, int8_t bLevel);

void SpreadEffect(int16_t sGridNo, uint8_t ubRadius, uint16_t usItem, SOLDIERTYPE *owner,
                  BOOLEAN fSubsequent, int8_t bLevel, const SMOKEEFFECT *s);
void SpreadEffectSmoke(const SMOKEEFFECT *s, BOOLEAN subsequent, int8_t level);

void DecayBombTimers();
void SetOffBombsByFrequency(SOLDIERTYPE *s, int8_t bFrequency);
BOOLEAN SetOffBombsInGridNo(SOLDIERTYPE *s, int16_t sGridNo, BOOLEAN fAllBombs, int8_t bLevel);
void ActivateSwitchInGridNo(SOLDIERTYPE *s, int16_t sGridNo);
void SetOffPanicBombs(SOLDIERTYPE *s, int8_t bPanicTrigger);

void UpdateExplosionFrame(EXPLOSIONTYPE *e, int16_t sCurrentFrame);
void RemoveExplosionData(EXPLOSIONTYPE *e);

void UpdateAndDamageSAMIfFound(int16_t sSectorX, int16_t sSectorY, int16_t sSectorZ,
                               int16_t sGridNo, uint8_t ubDamage);
void UpdateSAMDoneRepair(int16_t x, int16_t y, int16_t z);

void SaveExplosionTableToSaveGameFile(HWFILE);
void LoadExplosionTableFromSavedGameFile(HWFILE);

BOOLEAN ActiveTimedBombExists();
void RemoveAllActiveTimedBombs();

#define GASMASK_MIN_STATUS 70

BOOLEAN DishOutGasDamage(SOLDIERTYPE *pSoldier, EXPLOSIVETYPE const *pExplosive,
                         int16_t sSubsequent, BOOLEAN fRecompileMovementCosts, int16_t sWoundAmt,
                         int16_t sBreathAmt, SOLDIERTYPE *owner);

void HandleExplosionQueue();

bool DoesSAMExistHere(int16_t x, int16_t y, int16_t z, GridNo);

#endif
