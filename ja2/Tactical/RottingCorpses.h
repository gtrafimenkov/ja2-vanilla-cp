// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef _ROTTING_CORPSES_H
#define _ROTTING_CORPSES_H

#include "SGP/Debug.h"
#include "Tactical/OverheadTypes.h"
#include "TileEngine/TileAnimation.h"

#define NUM_CORPSE_SHADES 17

enum RottingCorpseDefines {
  NO_CORPSE,
  SMERC_JFK,
  SMERC_BCK,
  SMERC_FWD,
  SMERC_DHD,
  SMERC_PRN,
  SMERC_WTR,
  SMERC_FALL,
  SMERC_FALLF,

  MMERC_JFK,
  MMERC_BCK,
  MMERC_FWD,
  MMERC_DHD,
  MMERC_PRN,
  MMERC_WTR,
  MMERC_FALL,
  MMERC_FALLF,

  FMERC_JFK,
  FMERC_BCK,
  FMERC_FWD,
  FMERC_DHD,
  FMERC_PRN,
  FMERC_WTR,
  FMERC_FALL,
  FMERC_FALLF,

  // CIVS
  M_DEAD1,
  K_DEAD1,
  H_DEAD1,
  FT_DEAD1,
  S_DEAD1,
  W_DEAD1,
  C_DEAD1,
  M_DEAD2,
  K_DEAD2,
  H_DEAD2,

  FT_DEAD2,
  S_DEAD2,
  W_DEAD2,
  C_DEAD2,
  BLOODCAT_DEAD,
  COW_DEAD,
  ADULTMONSTER_DEAD,
  INFANTMONSTER_DEAD,
  LARVAEMONSTER_DEAD,
  ROTTING_STAGE2,

  TANK1_DEAD,
  TANK2_DEAD,
  HUMMER_DEAD,
  ICECREAM_DEAD,
  QUEEN_MONSTER_DEAD,
  ROBOT_DEAD,
  BURNT_DEAD,
  EXPLODE_DEAD,

  NUM_CORPSES
};

#define ROTTING_CORPSE_FIND_SWEETSPOT_FROM_GRIDNO 0x01  // Find the closest spot to the given gridno
#define ROTTING_CORPSE_USE_NORTH_ENTRY_POINT 0x02  // Find the spot closest to the north entry grid
#define ROTTING_CORPSE_USE_SOUTH_ENTRY_POINT 0x04  // Find the spot closest to the south entry grid
#define ROTTING_CORPSE_USE_EAST_ENTRY_POINT 0x08   // Find the spot closest to the east entry grid
#define ROTTING_CORPSE_USE_WEST_ENTRY_POINT 0x10   // Find the spot closest to the west entry grid
#define ROTTING_CORPSE_USE_CAMO_PALETTE 0x20       // We use camo palette here....
#define ROTTING_CORPSE_VEHICLE 0x40                // Vehicle Corpse

struct ROTTING_CORPSE_DEFINITION {
  uint8_t ubType;
  uint8_t ubBodyType;
  int16_t sGridNo;
  int16_t sHeightAdjustment;

  PaletteRepID HeadPal;  // Palette reps
  PaletteRepID PantsPal;
  PaletteRepID VestPal;
  PaletteRepID SkinPal;

  int8_t bDirection;
  uint32_t uiTimeOfDeath;

  uint16_t usFlags;

  int8_t bLevel;

  int8_t bVisible;
  int8_t bNumServicingCrows;
  uint8_t ubProfile;
  BOOLEAN fHeadTaken;
  uint8_t ubAIWarningValue;
};

struct ROTTING_CORPSE {
  ROTTING_CORPSE_DEFINITION def;
  BOOLEAN fActivated;

  ANITILE *pAniTile;

  uint16_t *pShades[NUM_CORPSE_SHADES];
};

ROTTING_CORPSE *AddRottingCorpse(ROTTING_CORPSE_DEFINITION *pCorpseDef);

void RemoveCorpses();

BOOLEAN TurnSoldierIntoCorpse(SOLDIERTYPE &);

int16_t FindNearestRottingCorpse(SOLDIERTYPE *pSoldier);

int16_t FindNearestAvailableGridNoForCorpse(ROTTING_CORPSE_DEFINITION *pCorpseDef, int8_t ubRadius);

void HandleRottingCorpses();

void VaporizeCorpse(int16_t sGridNo, uint16_t usStructureID);
void CorpseHit(int16_t sGridNo, uint16_t usStructureID);

void HandleCrowLeave(SOLDIERTYPE *pSoldier);

void HandleCrowFlyAway(SOLDIERTYPE *pSoldier);

#define MAX_ROTTING_CORPSES 100

extern ROTTING_CORPSE gRottingCorpse[MAX_ROTTING_CORPSES];
extern int32_t giNumRottingCorpse;
extern uint8_t gb4DirectionsFrom8[8];

static inline uint32_t Corpse2ID(const ROTTING_CORPSE *const c) {
  Assert(gRottingCorpse <= c && c < endof(gRottingCorpse));
  Assert(c->fActivated);
  return (uint32_t)(c - gRottingCorpse);
}

static inline ROTTING_CORPSE *ID2Corpse(const uint32_t id) {
  Assert(id < lengthof(gRottingCorpse));
  ROTTING_CORPSE *const c = &gRottingCorpse[id];
  Assert(c->fActivated);
  return c;
}

#define CORPSE2ID(c) (Corpse2ID((c)))
#define ID2CORPSE(i) (ID2Corpse((i)))

#define BASE_FOR_EACH_ROTTING_CORPSE(type, iter)                                              \
  for (type *iter = gRottingCorpse, *const end__##iter = gRottingCorpse + giNumRottingCorpse; \
       iter != end__##iter; ++iter)                                                           \
    if (!iter->fActivated)                                                                    \
      continue;                                                                               \
    else
#define FOR_EACH_ROTTING_CORPSE(iter) BASE_FOR_EACH_ROTTING_CORPSE(ROTTING_CORPSE, iter)
#define CFOR_EACH_ROTTING_CORPSE(iter) BASE_FOR_EACH_ROTTING_CORPSE(const ROTTING_CORPSE, iter)

ROTTING_CORPSE *GetCorpseAtGridNo(int16_t sGridNo, int8_t bLevel);
BOOLEAN IsValidDecapitationCorpse(const ROTTING_CORPSE *c);
void DecapitateCorpse(int16_t sGridNo, int8_t bLevel);

void GetBloodFromCorpse(SOLDIERTYPE *pSoldier);

uint16_t GetCorpseStructIndex(const ROTTING_CORPSE_DEFINITION *pCorpseDef, BOOLEAN fForImage);

void LookForAndMayCommentOnSeeingCorpse(SOLDIERTYPE *pSoldier, int16_t sGridNo, uint8_t ubLevel);

int16_t GetGridNoOfCorpseGivenProfileID(uint8_t ubProfileID);

void DecayRottingCorpseAIWarnings();
uint8_t GetNearestRottingCorpseAIWarning(int16_t sGridNo);

void ReduceAmmoDroppedByNonPlayerSoldiers(SOLDIERTYPE const &, OBJECTTYPE &ammo);

#endif
