// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef _TILE_ANIMATION_H
#define _TILE_ANIMATION_H

#include "JA2Types.h"
#include "TileEngine/WorldDef.h"
#include "Utils/SoundControl.h"

enum AnimationFlags {
  ANITILE_DOOR = 0x00000001,
  ANITILE_BACKWARD = 0x00000020,
  ANITILE_FORWARD = 0x00000040,
  ANITILE_PAUSED = 0x00000200,
  ANITILE_EXISTINGTILE = 0x00000400,
  ANITILE_LOOPING = 0x00020000,
  ANITILE_NOZBLITTER = 0x00040000,
  ANITILE_REVERSE_LOOPING = 0x00080000,
  ANITILE_ALWAYS_TRANSLUCENT = 0x00100000,
  ANITILE_OPTIMIZEFORSLOWMOVING = 0x00400000,
  ANITILE_ANIMATE_Z = 0x00800000,
  ANITILE_USE_DIRECTION_FOR_START_FRAME = 0x01000000,
  ANITILE_PAUSE_AFTER_LOOP =
      0x02000000,  // XXX same value as ANITILE_USE_4DIRECTION_FOR_START_FRAME
  ANITILE_ERASEITEMFROMSAVEBUFFFER = 0x04000000,
  ANITILE_SMOKE_EFFECT = 0x10000000,
  ANITILE_EXPLOSION = 0x20000000,
  ANITILE_RELEASE_ATTACKER_WHEN_DONE = 0x40000000,
  ANITILE_USE_4DIRECTION_FOR_START_FRAME = 0x02000000  // XXX same value as ANITILE_PAUSE_AFTER_LOOP
};
ENUM_BITSET(AnimationFlags)

enum AnimationLevel {
  ANI_LAND_LEVEL = 1,
  ANI_SHADOW_LEVEL = 2,
  ANI_OBJECT_LEVEL = 3,
  ANI_STRUCT_LEVEL = 4,
  ANI_ROOF_LEVEL = 5,
  ANI_ONROOF_LEVEL = 6,
  ANI_TOPMOST_LEVEL = 7
};

union AniUserData {
  struct {
    uint32_t uiData;
    uint32_t uiData3;
  } user;
  EXPLOSIONTYPE *explosion;
  BULLET *bullet;
  const REAL_OBJECT *object;
  SoundID sound;
};

struct ANITILE {
  ANITILE *pNext;
  AnimationFlags uiFlags;     // flags struct
  uint32_t uiTimeLastUpdate;  // Stuff for animated tiles

  LEVELNODE *pLevelNode;
  uint8_t ubLevelID;
  int16_t sCurrentFrame;
  int16_t sStartFrame;
  int16_t sDelay;
  uint16_t usNumFrames;

  int16_t sRelativeX;
  int16_t sRelativeY;
  int16_t sRelativeZ;
  int16_t sGridNo;
  uint16_t usTileIndex;

  int16_t sCachedTileID;  // Index into cached tile ID

  uint8_t ubKeyFrame1;
  uint32_t uiKeyFrame1Code;
  uint8_t ubKeyFrame2;
  uint32_t uiKeyFrame2Code;

  AniUserData v;

  int8_t bFrameCountAfterStart;
};

struct ANITILE_PARAMS {
  AnimationFlags uiFlags;    // flags struct
  AnimationLevel ubLevelID;  // Level ID for rendering layer
  int16_t sStartFrame;       // Start frame
  int16_t sDelay;            // Delay time
  uint16_t usTileIndex;      // Tile database index ( optional )
  int16_t sX;                // World X ( optional )
  int16_t sY;                // World Y ( optional )
  int16_t sZ;                // World Z ( optional )
  int16_t sGridNo;           // World GridNo

  LEVELNODE *pGivenLevelNode;  // Levelnode for existing tile ( optional )
  const char *zCachedFile;     // Filename for cached tile name ( optional )

  uint8_t ubKeyFrame1;       // Key frame 1
  uint32_t uiKeyFrame1Code;  // Key frame code
  uint8_t ubKeyFrame2;       // Key frame 2
  uint32_t uiKeyFrame2Code;  // Key frame code

  AniUserData v;
};

enum KeyFrameEnums {
  ANI_KEYFRAME_NO_CODE,
  ANI_KEYFRAME_BEGIN_TRANSLUCENCY,
  ANI_KEYFRAME_BEGIN_DAMAGE,
  ANI_KEYFRAME_CHAIN_WATER_EXPLOSION,
  ANI_KEYFRAME_DO_SOUND
};

ANITILE *CreateAnimationTile(const ANITILE_PARAMS *);

void DeleteAniTile(ANITILE *pAniTile);
void UpdateAniTiles();
void DeleteAniTiles();

void HideAniTile(ANITILE *pAniTile, BOOLEAN fHide);

ANITILE *GetCachedAniTileOfType(int16_t sGridNo, uint8_t ubLevelID, AnimationFlags);

#endif
