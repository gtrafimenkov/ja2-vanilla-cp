// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Tactical/Bullets.h"

#include <string.h>

#include "Directories.h"
#include "GameSettings.h"
#include "Macro.h"
#include "SGP/Debug.h"
#include "SGP/FileMan.h"
#include "SGP/Random.h"
#include "Tactical/Faces.h"
#include "Tactical/LOS.h"
#include "Tactical/LoadSaveBullet.h"
#include "Tactical/Overhead.h"
#include "Tactical/SoldierProfile.h"
#include "TileEngine/RenderDirty.h"
#include "TileEngine/SysUtil.h"
#include "TileEngine/TileDat.h"
#include "TileEngine/WorldDef.h"
#include "TileEngine/WorldMan.h"

#define NUM_BULLET_SLOTS 50

static BULLET gBullets[NUM_BULLET_SLOTS];
uint32_t guiNumBullets = 0;

static BULLET *GetFreeBullet() {
  BULLET *b;
  for (b = gBullets; b != gBullets + guiNumBullets; ++b) {
    if (!b->fAllocated) return b;
  }
  if (b == endof(gBullets)) return NULL;
  ++guiNumBullets;
  return b;
}

static void RecountBullets() {
  int32_t uiCount;

  for (uiCount = guiNumBullets - 1; (uiCount >= 0); uiCount--) {
    if ((gBullets[uiCount].fAllocated)) {
      guiNumBullets = (uint32_t)(uiCount + 1);
      return;
    }
  }
  guiNumBullets = 0;
}

BULLET *CreateBullet(SOLDIERTYPE *const firer, const BOOLEAN fFake, const uint16_t usFlags) {
  BULLET *const b = GetFreeBullet();
  if (b == NULL) return NULL;

  memset(b, 0, sizeof(*b));
  b->fAllocated = TRUE;
  b->fLocated = FALSE;
  b->pFirer = firer;
  b->usFlags = usFlags;
  b->usLastStructureHit = 0;
  b->fReal = !fFake;

  return b;
}

void HandleBulletSpecialFlags(BULLET *pBullet) {
  ANITILE_PARAMS AniParams;
  float dX, dY;
  uint8_t ubDirection;

  memset(&AniParams, 0, sizeof(ANITILE_PARAMS));

  if (pBullet->fReal) {
    // Create ani tile if this is a spit!
    if (pBullet->usFlags & (BULLET_FLAG_KNIFE)) {
      AniParams.sGridNo = (int16_t)pBullet->sGridNo;
      AniParams.ubLevelID = ANI_STRUCT_LEVEL;
      AniParams.sDelay = 100;
      AniParams.sStartFrame = 3;
      AniParams.uiFlags = ANITILE_FORWARD | ANITILE_LOOPING | ANITILE_USE_DIRECTION_FOR_START_FRAME;
      AniParams.sX = FIXEDPT_TO_INT32(pBullet->qCurrX);
      AniParams.sY = FIXEDPT_TO_INT32(pBullet->qCurrY);
      AniParams.sZ = CONVERT_HEIGHTUNITS_TO_PIXELS(FIXEDPT_TO_INT32(pBullet->qCurrZ));

      if (pBullet->usFlags & (BULLET_FLAG_CREATURE_SPIT)) {
        AniParams.zCachedFile = TILECACHEDIR "/spit2.sti";
      } else if (pBullet->usFlags & (BULLET_FLAG_KNIFE)) {
        AniParams.zCachedFile = TILECACHEDIR "/knifing.sti";
        pBullet->ubItemStatus = pBullet->pFirer->inv[HANDPOS].bStatus[0];
      }

      // Get direction to use for this guy....
      dX = ((float)(pBullet->qIncrX) / FIXEDPT_FRACTIONAL_RESOLUTION);
      dY = ((float)(pBullet->qIncrY) / FIXEDPT_FRACTIONAL_RESOLUTION);

      ubDirection = atan8(0, 0, (int16_t)(dX * 100), (int16_t)(dY * 100));

      AniParams.v.user.uiData3 = ubDirection;

      pBullet->pAniTile = CreateAnimationTile(&AniParams);

      // IF we are anything that needs a shadow.. set it here....
      if (pBullet->usFlags & (BULLET_FLAG_KNIFE)) {
        AniParams.ubLevelID = ANI_SHADOW_LEVEL;
        AniParams.sZ = 0;
        pBullet->pShadowAniTile = CreateAnimationTile(&AniParams);
      }
    }
  }
}

void RemoveBullet(BULLET *b) {
  // decrease soldier's bullet count

  if (b->fReal) {
    // set to be deleted at next update
    b->fToDelete = TRUE;

    // decrement reference to bullet in the firer
    b->pFirer->bBulletsLeft--;
    DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
             String("!!!!!!! Ending bullet, bullets left %d", b->pFirer->bBulletsLeft));

    if (b->usFlags & BULLET_FLAG_KNIFE) {
      // Delete ani tile
      if (b->pAniTile != NULL) {
        DeleteAniTile(b->pAniTile);
        b->pAniTile = NULL;
      }

      // Delete shadow
      if (b->usFlags & BULLET_FLAG_KNIFE) {
        if (b->pShadowAniTile != NULL) {
          DeleteAniTile(b->pShadowAniTile);
          b->pShadowAniTile = NULL;
        }
      }
    }
  } else {
    // delete this fake bullet right away!
    b->fAllocated = FALSE;
    RecountBullets();
  }
}

void LocateBullet(BULLET *b) {
  if (!gGameSettings.fOptions[TOPTION_SHOW_MISSES]) return;
  if (b->pFirer == NULL) return;
  // Check if a bad guy fired!
  if (b->pFirer->bSide != OUR_TEAM) return;
  if (b->fLocated) return;

  b->fLocated = TRUE;

  // Only if we are in turnbased and noncombat
  if (!(gTacticalStatus.uiFlags & TURNBASED)) return;
  if (!(gTacticalStatus.uiFlags & INCOMBAT)) return;

  LocateGridNo(b->sGridNo);
}

void UpdateBullets() {
  uint32_t uiCount;
  LEVELNODE *pNode;
  BOOLEAN fDeletedSome = FALSE;

  for (uiCount = 0; uiCount < guiNumBullets; uiCount++) {
    BULLET *const b = &gBullets[uiCount];
    if (b->fAllocated) {
      if (b->fReal && !(b->usFlags & BULLET_STOPPED)) {
        // there are duplicate checks for deletion in case the bullet is deleted
        // by shooting someone at point blank range, in the first MoveBullet
        // call in the FireGun code
        if (b->fToDelete) {
          // Remove from old position
          b->fAllocated = FALSE;
          fDeletedSome = TRUE;
          continue;
        }

        // if ( !( gGameSettings.fOptions[ TOPTION_HIDE_BULLETS ] ) )
        {
          // ALRIGHTY, CHECK WHAT TYPE OF BULLET WE ARE

          if (b->usFlags &
              (BULLET_FLAG_CREATURE_SPIT | BULLET_FLAG_KNIFE | BULLET_FLAG_MISSILE |
               BULLET_FLAG_SMALL_MISSILE | BULLET_FLAG_TANK_CANNON | BULLET_FLAG_FLAME)) {
          } else {
            RemoveStruct(b->sGridNo, BULLETTILE1);
            RemoveStruct(b->sGridNo, BULLETTILE2);
          }
        }

        MoveBullet(b);
        if (b->fToDelete) {
          // Remove from old position
          b->fAllocated = FALSE;
          fDeletedSome = TRUE;
          continue;
        }

        if (b->usFlags & BULLET_STOPPED) continue;

        // Display bullet
        // if ( !( gGameSettings.fOptions[ TOPTION_HIDE_BULLETS ] ) )
        {
          if (b->usFlags & BULLET_FLAG_KNIFE) {
            if (b->pAniTile != NULL) {
              b->pAniTile->sRelativeX = FIXEDPT_TO_INT32(b->qCurrX);
              b->pAniTile->sRelativeY = FIXEDPT_TO_INT32(b->qCurrY);
              b->pAniTile->pLevelNode->sRelativeZ =
                  CONVERT_HEIGHTUNITS_TO_PIXELS(FIXEDPT_TO_INT32(b->qCurrZ));

              if (b->usFlags & BULLET_FLAG_KNIFE) {
                b->pShadowAniTile->sRelativeX = FIXEDPT_TO_INT32(b->qCurrX);
                b->pShadowAniTile->sRelativeY = FIXEDPT_TO_INT32(b->qCurrY);
              }
            }
          }
          // Are we a missle?
          else if (b->usFlags &
                   (BULLET_FLAG_MISSILE | BULLET_FLAG_SMALL_MISSILE | BULLET_FLAG_TANK_CANNON |
                    BULLET_FLAG_FLAME | BULLET_FLAG_CREATURE_SPIT)) {
          } else {
            pNode = AddStructToTail(b->sGridNo, BULLETTILE1);
            pNode->ubShadeLevel = DEFAULT_SHADE_LEVEL;
            pNode->ubNaturalShadeLevel = DEFAULT_SHADE_LEVEL;
            pNode->uiFlags |= (LEVELNODE_USEABSOLUTEPOS | LEVELNODE_IGNOREHEIGHT);
            pNode->sRelativeX = FIXEDPT_TO_INT32(b->qCurrX);
            pNode->sRelativeY = FIXEDPT_TO_INT32(b->qCurrY);
            pNode->sRelativeZ = CONVERT_HEIGHTUNITS_TO_PIXELS(FIXEDPT_TO_INT32(b->qCurrZ));

            // Display shadow
            pNode = AddStructToTail(b->sGridNo, BULLETTILE2);
            pNode->ubShadeLevel = DEFAULT_SHADE_LEVEL;
            pNode->ubNaturalShadeLevel = DEFAULT_SHADE_LEVEL;
            pNode->uiFlags |= (LEVELNODE_USEABSOLUTEPOS | LEVELNODE_IGNOREHEIGHT);
            pNode->sRelativeX = FIXEDPT_TO_INT32(b->qCurrX);
            pNode->sRelativeY = FIXEDPT_TO_INT32(b->qCurrY);
            pNode->sRelativeZ = gpWorldLevelData[b->sGridNo].sHeight;
          }
        }
      } else {
        if (b->fToDelete) {
          b->fAllocated = FALSE;
          fDeletedSome = TRUE;
        }
      }
    }
  }

  if (fDeletedSome) {
    RecountBullets();
  }
}

void AddMissileTrail(BULLET *pBullet, FIXEDPT qCurrX, FIXEDPT qCurrY, FIXEDPT qCurrZ) {
  ANITILE_PARAMS AniParams;

  // If we are a small missle, don't show
  if (pBullet->usFlags &
      (BULLET_FLAG_SMALL_MISSILE | BULLET_FLAG_FLAME | BULLET_FLAG_CREATURE_SPIT)) {
    if (pBullet->iLoop < 5) {
      return;
    }
  }

  // If we are a small missle, don't show
  if (pBullet->usFlags & (BULLET_FLAG_TANK_CANNON)) {
    // if ( pBullet->iLoop < 40 )
    //{
    return;
    //}
  }

  memset(&AniParams, 0, sizeof(ANITILE_PARAMS));
  AniParams.sGridNo = (int16_t)pBullet->sGridNo;
  AniParams.ubLevelID = ANI_STRUCT_LEVEL;
  AniParams.sDelay = (int16_t)(100 + Random(100));
  AniParams.sStartFrame = 0;
  AniParams.uiFlags = ANITILE_FORWARD | ANITILE_ALWAYS_TRANSLUCENT;
  AniParams.sX = FIXEDPT_TO_INT32(qCurrX);
  AniParams.sY = FIXEDPT_TO_INT32(qCurrY);
  AniParams.sZ = CONVERT_HEIGHTUNITS_TO_PIXELS(FIXEDPT_TO_INT32(qCurrZ));

  if (pBullet->usFlags & (BULLET_FLAG_MISSILE | BULLET_FLAG_TANK_CANNON)) {
    AniParams.zCachedFile = TILECACHEDIR "/msle_smk.sti";
  } else if (pBullet->usFlags & (BULLET_FLAG_SMALL_MISSILE)) {
    AniParams.zCachedFile = TILECACHEDIR "/msle_sma.sti";
  } else if (pBullet->usFlags & (BULLET_FLAG_CREATURE_SPIT)) {
    AniParams.zCachedFile = TILECACHEDIR "/msle_spt.sti";
  } else if (pBullet->usFlags & (BULLET_FLAG_FLAME)) {
    AniParams.zCachedFile = TILECACHEDIR "/flmthr2.sti";
    AniParams.sDelay = (int16_t)(100);
  }

  CreateAnimationTile(&AniParams);
}

void SaveBulletStructureToSaveGameFile(HWFILE const hFile) {
  uint16_t usCnt;
  uint32_t uiBulletCount = 0;

  // loop through and count the number of bullets
  for (usCnt = 0; usCnt < NUM_BULLET_SLOTS; usCnt++) {
    // if the bullet is active, save it
    if (gBullets[usCnt].fAllocated) {
      uiBulletCount++;
    }
  }

  // Save the number of Bullets in the array
  FileWrite(hFile, &uiBulletCount, sizeof(uint32_t));

  if (uiBulletCount != 0) {
    for (usCnt = 0; usCnt < NUM_BULLET_SLOTS; usCnt++) {
      // if the bullet is active, save it
      if (gBullets[usCnt].fAllocated) {
        // Save the the Bullet structure
        InjectBulletIntoFile(hFile, &gBullets[usCnt]);
      }
    }
  }
}

void LoadBulletStructureFromSavedGameFile(HWFILE const hFile) {
  // make sure the bullets are not allocated
  memset(gBullets, 0, sizeof(gBullets));

  // Load the number of Bullets in the array
  FileRead(hFile, &guiNumBullets, sizeof(uint32_t));

  for (uint32_t i = 0; i < guiNumBullets; ++i) {
    BULLET *const b = &gBullets[i];
    // Load the the Bullet structure
    ExtractBulletFromFile(hFile, b);

    // Set some parameters
    b->uiLastUpdate = 0;

    HandleBulletSpecialFlags(b);
  }
}

void StopBullet(BULLET *b) {
  b->usFlags |= BULLET_STOPPED;

  RemoveStruct(b->sGridNo, BULLETTILE1);
  RemoveStruct(b->sGridNo, BULLETTILE2);
}

void DeleteAllBullets() {
  for (uint32_t i = 0; i < guiNumBullets; ++i) {
    BULLET *const b = &gBullets[i];
    if (b->fAllocated) {
      // Remove from old position
      RemoveBullet(b);
      b->fAllocated = FALSE;
    }
  }

  RecountBullets();
}
