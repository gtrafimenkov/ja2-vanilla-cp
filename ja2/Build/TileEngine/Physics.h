#ifndef __PHYSICS_H
#define __PHYSICS_H

#include "JA2Types.h"
#include "Tactical/ItemTypes.h"
#include "TileEngine/PhysMath.h"
#include "TileEngine/WorldDef.h"

extern uint32_t guiNumObjectSlots;

struct REAL_OBJECT {
  BOOLEAN fAllocated;
  BOOLEAN fAlive;
  BOOLEAN fApplyFriction;
  BOOLEAN fVisible;
  BOOLEAN fInWater;
  BOOLEAN fTestObject;
  BOOLEAN fTestEndedWithCollision;
  BOOLEAN fTestPositionNotSet;

  real TestZTarget;
  real OneOverMass;
  real AppliedMu;

  vector_3 Position;
  vector_3 TestTargetPosition;
  vector_3 OldPosition;
  vector_3 Velocity;
  vector_3 OldVelocity;
  vector_3 InitialForce;
  vector_3 Force;
  vector_3 CollisionNormal;
  vector_3 CollisionVelocity;
  real CollisionElasticity;

  int16_t sGridNo;
  LEVELNODE *pNode;
  LEVELNODE *pShadow;

  int16_t sConsecutiveCollisions;
  int16_t sConsecutiveZeroVelocityCollisions;
  int32_t iOldCollisionCode;

  float dLifeLength;
  float dLifeSpan;
  OBJECTTYPE Obj;
  SOLDIERTYPE *owner;
  int16_t sFirstGridNo;
  BOOLEAN fFirstTimeMoved;
  uint8_t ubActionCode;
  SOLDIERTYPE *target;
  BOOLEAN fDropItem;
  uint32_t uiNumTilesMoved;
  BOOLEAN fCatchGood;
  BOOLEAN fAttemptedCatch;
  BOOLEAN fCatchAnimOn;
  BOOLEAN fCatchCheckDone;
  BOOLEAN fEndedWithCollisionPositionSet;
  vector_3 EndedWithCollisionPosition;
  BOOLEAN fHaveHitGround;
  BOOLEAN fPotentialForDebug;
  int16_t sLevelNodeGridNo;
  int32_t iSoundID;
  uint8_t ubLastTargetTakenDamage;
};

// OBJECT LIST STUFF
REAL_OBJECT *CreatePhysicalObject(const OBJECTTYPE *pGameObj, real dLifeLength, real xPos,
                                  real yPos, real zPos, real xForce, real yForce, real zForce,
                                  SOLDIERTYPE *owner, uint8_t ubActionCode, SOLDIERTYPE *target);
void RemoveAllPhysicsObjects();

BOOLEAN CalculateLaunchItemChanceToGetThrough(const SOLDIERTYPE *pSoldier, const OBJECTTYPE *pItem,
                                              int16_t sGridNo, uint8_t ubLevel, int16_t sEndZ,
                                              int16_t *psFinalGridNo, BOOLEAN fArmed,
                                              int8_t *pbLevel, BOOLEAN fFromUI);

void CalculateLaunchItemParamsForThrow(SOLDIERTYPE *pSoldier, int16_t sGridNo, uint8_t ubLevel,
                                       int16_t sZPos, OBJECTTYPE *pItem, int8_t bMissBy,
                                       uint8_t ubActionCode, SOLDIERTYPE *target);

// SIMULATE WORLD
void SimulateWorld();

void SavePhysicsTableToSaveGameFile(HWFILE);
void LoadPhysicsTableFromSavedGameFile(HWFILE);

#endif
