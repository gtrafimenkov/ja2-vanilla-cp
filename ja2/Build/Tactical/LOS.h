#ifndef LOS_H
#define LOS_H

#include "JA2Types.h"

// #define LOS_DEBUG

// fixed-point arithmetic definitions start here

typedef int32_t FIXEDPT;
// rem 1 signed bit at the top
#define FIXEDPT_WHOLE_BITS 11
#define FIXEDPT_FRACTIONAL_BITS 20
#define FIXEDPT_FRACTIONAL_RESOLUTION 1048576

#define INT32_TO_FIXEDPT(n) ((n) << FIXEDPT_FRACTIONAL_BITS)
#define FIXEDPT_TO_INT32(n) ((n) / FIXEDPT_FRACTIONAL_RESOLUTION)

#define FIXEDPT_TO_TILE_NUM(n) (FIXEDPT_TO_INT32((n)) / CELL_X_SIZE)
#define FIXEDPT_TO_LOS_INDEX(n) (CONVERT_WITHINTILE_TO_INDEX(FIXEDPT_TO_INT32((n)) % CELL_X_SIZE))

// fixed-point arithmetic definitions end here

#define OK_CHANCE_TO_GET_THROUGH 10

enum CollisionEnums {
  COLLISION_NONE,
  COLLISION_GROUND,
  COLLISION_MERC,
  COLLISION_WINDOW_SOUTHEAST,
  COLLISION_WINDOW_SOUTHWEST,
  COLLISION_WINDOW_NORTHEAST,
  COLLISION_WINDOW_NORTHWEST,
  COLLISION_WINDOW_NORTH,
  COLLISION_WALL_SOUTHEAST,
  COLLISION_WALL_SOUTHWEST,
  COLLISION_WALL_NORTHEAST,
  COLLISION_WALL_NORTHWEST,
  COLLISION_STRUCTURE,
  COLLISION_ROOF,
  COLLISION_INTERIOR_ROOF,
  COLLISION_STRUCTURE_Z,
  COLLISION_WATER
};

int32_t CheckForCollision(float dX, float dY, float dZ, float dDeltaX, float dDeltaY, float dDeltaZ,
                          uint16_t *pusStructureID, float *pdNormalX, float *pdNormalY,
                          float *pdNormalZ);

int8_t FireBulletGivenTarget(SOLDIERTYPE *pFirer, float dEndX, float dEndY, float dEndZ,
                             uint16_t usHandItem, int16_t sHitBy, BOOLEAN fBuckshot, BOOLEAN fFake);

int32_t SoldierToSoldierLineOfSightTest(const SOLDIERTYPE *pStartSoldier,
                                        const SOLDIERTYPE *pEndSoldier, uint8_t ubTileSightLimit,
                                        int8_t bAware);
int32_t SoldierToLocationLineOfSightTest(SOLDIERTYPE *pStartSoldier, int16_t sGridNo,
                                         uint8_t ubSightLimit, int8_t bAware);
int32_t SoldierTo3DLocationLineOfSightTest(const SOLDIERTYPE *pStartSoldier, int16_t sGridNo,
                                           int8_t bLevel, int8_t bCubeLevel,
                                           uint8_t ubTileSightLimit, int8_t bAware);
int32_t SoldierToBodyPartLineOfSightTest(SOLDIERTYPE *pStartSoldier, int16_t sGridNo, int8_t bLevel,
                                         uint8_t ubAimLocation, uint8_t ubTileSightLimit,
                                         int8_t bAware);
int32_t SoldierToVirtualSoldierLineOfSightTest(const SOLDIERTYPE *pStartSoldier, int16_t sGridNo,
                                               int8_t bLevel, int8_t bStance,
                                               uint8_t ubTileSightLimit, int8_t bAware);
uint8_t SoldierToSoldierBodyPartChanceToGetThrough(SOLDIERTYPE *pStartSoldier,
                                                   const SOLDIERTYPE *pEndSoldier,
                                                   uint8_t ubAimLocation);
uint8_t AISoldierToSoldierChanceToGetThrough(SOLDIERTYPE *pStartSoldier,
                                             const SOLDIERTYPE *pEndSoldier);
uint8_t AISoldierToLocationChanceToGetThrough(SOLDIERTYPE *pStartSoldier, int16_t sGridNo,
                                              int8_t bLevel, int8_t bCubeLevel);
uint8_t SoldierToLocationChanceToGetThrough(SOLDIERTYPE *pStartSoldier, int16_t sGridNo,
                                            int8_t bLevel, int8_t bCubeLevel,
                                            const SOLDIERTYPE *target);
int16_t SoldierToLocationWindowTest(const SOLDIERTYPE *pStartSoldier, int16_t sEndGridNo);
int32_t LocationToLocationLineOfSightTest(int16_t sStartGridNo, int8_t bStartLevel,
                                          int16_t sEndGridNo, int8_t bEndLevel,
                                          uint8_t ubTileSightLimit, int8_t bAware);

BOOLEAN CalculateSoldierZPos(const SOLDIERTYPE *pSoldier, uint8_t ubPosType, float *pdZPos);

#define HEIGHT_UNITS 256
#define HEIGHT_UNITS_PER_INDEX (HEIGHT_UNITS / PROFILE_Z_SIZE)
#define MAX_STRUCTURE_HEIGHT 50
// 5.12 == HEIGHT_UNITS / MAX_STRUCTURE_HEIGHT
#define CONVERT_PIXELS_TO_HEIGHTUNITS(n) ((n) * HEIGHT_UNITS / MAX_STRUCTURE_HEIGHT)
#define CONVERT_PIXELS_TO_INDEX(n) \
  ((n) * HEIGHT_UNITS / MAX_STRUCTURE_HEIGHT / HEIGHT_UNITS_PER_INDEX)
#define CONVERT_HEIGHTUNITS_TO_INDEX(n) ((n) / HEIGHT_UNITS_PER_INDEX)
#define CONVERT_HEIGHTUNITS_TO_DISTANCE(n) ((n) / (HEIGHT_UNITS / CELL_X_SIZE))
#define CONVERT_HEIGHTUNITS_TO_PIXELS(n) ((n) * MAX_STRUCTURE_HEIGHT / HEIGHT_UNITS)
#define CONVERT_WITHINTILE_TO_INDEX(n) ((n) >> 1)
#define CONVERT_INDEX_TO_WITHINTILE(n) ((n) << 1)
#define CONVERT_INDEX_TO_PIXELS(n) \
  ((n) * MAX_STRUCTURE_HEIGHT * HEIGHT_UNITS_PER_INDEX / HEIGHT_UNITS)

enum {
  LOS_POS,
  FIRING_POS,
  TARGET_POS,
  HEAD_TARGET_POS,
  TORSO_TARGET_POS,
  LEGS_TARGET_POS,
  HEIGHT
};

// 191 is 6' (structures of height 3)
// 127 is 4' (structures of height 2)
//  63 is 2' (structures of height 1)

#define STANDING_HEIGHT 191.0f
#define STANDING_LOS_POS 175.0f
#define STANDING_FIRING_POS 175.0f
#define STANDING_HEAD_TARGET_POS 175.0f
#define STANDING_HEAD_BOTTOM_POS 159.0f
#define STANDING_TORSO_TARGET_POS 127.0f
#define STANDING_TORSO_BOTTOM_POS 95.0f
#define STANDING_LEGS_TARGET_POS 47.0f
#define STANDING_TARGET_POS STANDING_HEAD_TARGET_POS

#define CROUCHED_HEIGHT 130.0f
#define CROUCHED_LOS_POS 111.0f
#define CROUCHED_FIRING_POS 111.0f

#define CROUCHED_HEAD_TARGET_POS 111.0f
#define CROUCHED_HEAD_BOTTOM_POS 95.0f
#define CROUCHED_TORSO_TARGET_POS 71.0f
#define CROUCHED_TORSO_BOTTOM_POS 47.0f
#define CROUCHED_LEGS_TARGET_POS 31.0f
#define CROUCHED_TARGET_POS CROUCHED_HEAD_TARGET_POS

#define PRONE_HEIGHT 63.0f
#define PRONE_LOS_POS 31.0f
#define PRONE_FIRING_POS 31.0f
#define PRONE_TORSO_TARGET_POS 31.0f
#define PRONE_HEAD_TARGET_POS 31.0f
#define PRONE_LEGS_TARGET_POS 31.0f
#define PRONE_TARGET_POS PRONE_HEAD_TARGET_POS

#define WALL_HEIGHT_UNITS HEIGHT_UNITS
#define WINDOW_BOTTOM_HEIGHT_UNITS 87
#define WINDOW_TOP_HEIGHT_UNITS 220

#define CLOSE_TO_FIRER 25
#define VERY_CLOSE_TO_FIRER 21

#ifdef LOS_DEBUG
struct LOSResults {
  BOOLEAN fLOSTestPerformed;
  BOOLEAN fLOSClear;
  BOOLEAN fOutOfRange;
  int32_t iDistance;
  int32_t iMaxDistance;
  uint8_t ubTreeSpotsHit;
  int32_t iStartX;
  int32_t iStartY;
  int32_t iStartZ;
  int32_t iEndX;
  int32_t iEndY;
  int32_t iEndZ;
  int32_t iStoppedX;
  int32_t iStoppedY;
  int32_t iStoppedZ;
  int32_t iCurrCubesZ;
  uint8_t ubChanceToGetThrough;
};

extern LOSResults gLOSTestResults;

#endif

void MoveBullet(BULLET *b);

#endif
