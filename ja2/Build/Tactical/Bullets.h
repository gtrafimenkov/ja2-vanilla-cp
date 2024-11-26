#ifndef __BULLETS_H
#define __BULLETS_H

#include "JA2Types.h"
#include "SGP/Types.h"
#include "Tactical/LOS.h"
#include "TileEngine/TileAnimation.h"

#define BULLET_FLAG_CREATURE_SPIT 0x0001
#define BULLET_FLAG_KNIFE 0x0002
#define BULLET_FLAG_MISSILE 0x0004
#define BULLET_FLAG_SMALL_MISSILE 0x0008
#define BULLET_STOPPED 0x0010
#define BULLET_FLAG_TANK_CANNON 0x0020
#define BULLET_FLAG_BUCKSHOT 0x0040
#define BULLET_FLAG_FLAME 0x0080

struct BULLET {
  const SOLDIERTYPE *target;
  int8_t bStartCubesAboveLevelZ;
  int8_t bEndCubesAboveLevelZ;
  uint16_t usLastStructureHit;
  uint32_t sGridNo;
  FIXEDPT qCurrX;
  FIXEDPT qCurrY;
  FIXEDPT qCurrZ;
  FIXEDPT qIncrX;
  FIXEDPT qIncrY;
  FIXEDPT qIncrZ;
  double ddHorizAngle;
  int32_t iCurrTileX;
  int32_t iCurrTileY;
  int8_t bLOSIndexX;
  int8_t bLOSIndexY;
  BOOLEAN fCheckForRoof;
  int32_t iCurrCubesZ;
  int32_t iLoop;
  BOOLEAN fAllocated;
  BOOLEAN fToDelete;
  BOOLEAN fLocated;
  BOOLEAN fReal;
  BOOLEAN fAimed;
  uint32_t uiLastUpdate;
  uint8_t ubTilesPerUpdate;
  uint16_t usClockTicksPerUpdate;
  SOLDIERTYPE *pFirer;
  uint32_t sTargetGridNo;
  int16_t sHitBy;
  int32_t iImpact;
  int32_t iImpactReduction;
  int32_t iRange;
  int32_t iDistanceLimit;
  uint16_t usFlags;
  ANITILE *pAniTile;
  ANITILE *pShadowAniTile;
  uint8_t ubItemStatus;
};

extern uint32_t guiNumBullets;

BULLET *CreateBullet(SOLDIERTYPE *firer, BOOLEAN fFake, uint16_t usFlags);
void RemoveBullet(BULLET *b);
void StopBullet(BULLET *b);
void UpdateBullets();

void DeleteAllBullets();

void LocateBullet(BULLET *b);

void HandleBulletSpecialFlags(BULLET *b);

void AddMissileTrail(BULLET *pBullet, FIXEDPT qCurrX, FIXEDPT qCurrY, FIXEDPT qCurrZ);

// Save the bullet table to the saved game file
void SaveBulletStructureToSaveGameFile(HWFILE);

// Load the bullet table from the saved game file
void LoadBulletStructureFromSavedGameFile(HWFILE);

#endif
