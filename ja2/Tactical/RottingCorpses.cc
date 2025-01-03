// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Tactical/RottingCorpses.h"

#include <string.h>

#include "Directories.h"
#include "GameSettings.h"
#include "Macro.h"
#include "SGP/Debug.h"
#include "SGP/MemMan.h"
#include "SGP/Random.h"
#include "SGP/VObject.h"
#include "Strategic/GameClock.h"
#include "Strategic/Strategic.h"
#include "Strategic/StrategicMap.h"
#include "Tactical/AnimationControl.h"
#include "Tactical/DialogueControl.h"
#include "Tactical/HandleItems.h"
#include "Tactical/Interface.h"
#include "Tactical/InterfaceItems.h"
#include "Tactical/Items.h"
#include "Tactical/Keys.h"
#include "Tactical/LOS.h"
#include "Tactical/Overhead.h"
#include "Tactical/PathAI.h"
#include "Tactical/QArray.h"
#include "Tactical/SoldierAdd.h"
#include "Tactical/SoldierControl.h"
#include "Tactical/SoldierCreate.h"
#include "Tactical/SoldierFind.h"
#include "Tactical/SoldierMacros.h"
#include "Tactical/SoldierProfile.h"
#include "Tactical/Weapons.h"
#include "Tactical/WorldItems.h"
#include "TileEngine/ExplosionControl.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/Lighting.h"
#include "TileEngine/RenderFun.h"
#include "TileEngine/RenderWorld.h"
#include "TileEngine/Smell.h"
#include "TileEngine/Structure.h"
#include "TileEngine/TileCache.h"
#include "TileEngine/TileDef.h"
#include "TileEngine/WorldDef.h"
#include "Utils/FontControl.h"
#include "Utils/Message.h"
#include "Utils/SoundControl.h"
#include "Utils/Utilities.h"

#include "SDL_keycode.h"
#include "SDL_pixels.h"

#define CORPSE_WARNING_MAX 5
#define CORPSE_WARNING_DIST 5

#define CORPSE_INDEX_OFFSET 10000

// #define		DELAY_UNTIL_ROTTING		( 1 * NUM_SEC_IN_DAY )
#define DELAY_UNTIL_ROTTING (1 * NUM_SEC_IN_DAY / 60)
#define DELAY_UNTIL_DONE_ROTTING (3 * NUM_SEC_IN_DAY / 60)

#define MAX_NUM_CROWS 6

// When adding a corpse, add struct data...
static const char *const zCorpseFilenames[NUM_CORPSES] = {
    "",
    ANIMSDIR "/corpses/s_d_jfk.sti",
    ANIMSDIR "/corpses/s_d_bck.sti",
    ANIMSDIR "/corpses/s_d_fwd.sti",
    ANIMSDIR "/corpses/s_d_dhd.sti",
    ANIMSDIR "/corpses/s_d_prn.sti",
    ANIMSDIR "/corpses/s_d_wtr.sti",
    ANIMSDIR "/corpses/s_d_fall.sti",
    ANIMSDIR "/corpses/s_d_fallf.sti",

    ANIMSDIR "/corpses/m_d_jfk.sti",
    ANIMSDIR "/corpses/m_d_bck.sti",
    ANIMSDIR "/corpses/m_d_fwd.sti",
    ANIMSDIR "/corpses/m_d_dhd.sti",
    ANIMSDIR "/corpses/m_d_prn.sti",
    ANIMSDIR "/corpses/s_d_wtr.sti",
    ANIMSDIR "/corpses/m_d_fall.sti",
    ANIMSDIR "/corpses/m_d_fallf.sti",

    ANIMSDIR "/corpses/f_d_jfk.sti",
    ANIMSDIR "/corpses/f_d_bck.sti",
    ANIMSDIR "/corpses/f_d_fwd.sti",
    ANIMSDIR "/corpses/f_d_dhd.sti",
    ANIMSDIR "/corpses/f_d_prn.sti",
    ANIMSDIR "/corpses/s_d_wtr.sti",
    ANIMSDIR "/corpses/f_d_fall.sti",
    ANIMSDIR "/corpses/f_d_fallf.sti",

    // Civs....
    ANIMSDIR "/corpses/m_dead1.sti",
    ANIMSDIR "/corpses/k_dead2.sti",
    ANIMSDIR "/corpses/h_dead2.sti",
    ANIMSDIR "/corpses/ft_dead1.sti",
    ANIMSDIR "/corpses/s_dead1.sti",
    ANIMSDIR "/corpses/w_dead1.sti",
    ANIMSDIR "/corpses/cp_dead1.sti",
    ANIMSDIR "/corpses/m_dead2.sti",
    ANIMSDIR "/corpses/k_dead1.sti",
    ANIMSDIR "/corpses/h_dead1.sti",

    ANIMSDIR "/corpses/ft_dead2.sti",
    ANIMSDIR "/corpses/s_dead2.sti",
    ANIMSDIR "/corpses/w_dead2.sti",
    ANIMSDIR "/corpses/cp_dead2.sti",
    ANIMSDIR "/corpses/ct_dead.sti",
    ANIMSDIR "/corpses/cw_dead1.sti",
    ANIMSDIR "/corpses/mn_dead2.sti",
    ANIMSDIR "/corpses/i_dead1.sti",
    ANIMSDIR "/corpses/l_dead1.sti",

    ANIMSDIR "/corpses/p_decomp2.sti",
    ANIMSDIR "/corpses/tk_wrek.sti",
    ANIMSDIR "/corpses/tk2_wrek.sti",
    ANIMSDIR "/corpses/hm_wrek.sti",
    ANIMSDIR "/corpses/ic_wrek.sti",
    ANIMSDIR "/corpses/qn_dead.sti",
    ANIMSDIR "/corpses/j_dead.sti",
    ANIMSDIR "/corpses/s_burnt.sti",
    ANIMSDIR "/corpses/s_expld.sti",
};

// When adding a corpse, add struct data...
static const char *const zNoBloodCorpseFilenames[NUM_CORPSES] = {
    "",
    ANIMSDIR "/corpses/m_d_jfk_nb.sti",
    ANIMSDIR "/corpses/s_d_bck_nb.sti",
    ANIMSDIR "/corpses/s_d_fwd_nb.sti",
    ANIMSDIR "/corpses/s_d_dhd_nb.sti",
    ANIMSDIR "/corpses/s_d_prn_nb.sti",
    ANIMSDIR "/corpses/s_d_wtr.sti",
    ANIMSDIR "/corpses/s_d_fall_nb.sti",
    ANIMSDIR "/corpses/s_d_fallf_nb.sti",

    ANIMSDIR "/corpses/m_d_jfk_nb.sti",
    ANIMSDIR "/corpses/m_d_bck_nb.sti",
    ANIMSDIR "/corpses/m_d_fwd_nb.sti",
    ANIMSDIR "/corpses/m_d_dhd_nb.sti",
    ANIMSDIR "/corpses/m_d_prn_nb.sti",
    ANIMSDIR "/corpses/s_d_wtr.sti",
    ANIMSDIR "/corpses/m_d_fall_nb.sti",
    ANIMSDIR "/corpses/m_d_fallf_nb.sti",

    ANIMSDIR "/corpses/f_d_jfk_nb.sti",
    ANIMSDIR "/corpses/f_d_bck_nb.sti",
    ANIMSDIR "/corpses/f_d_fwd_nb.sti",
    ANIMSDIR "/corpses/f_d_dhd_nb.sti",
    ANIMSDIR "/corpses/f_d_prn_nb.sti",
    ANIMSDIR "/corpses/s_d_wtr.sti",
    ANIMSDIR "/corpses/f_d_fall_nb.sti",
    ANIMSDIR "/corpses/f_d_fallf_nb.sti",

    // Civs....
    ANIMSDIR "/corpses/m_dead1_nb.sti",
    ANIMSDIR "/corpses/k_dead2_nb.sti",
    ANIMSDIR "/corpses/h_dead2_nb.sti",
    ANIMSDIR "/corpses/ft_dead1_nb.sti",
    ANIMSDIR "/corpses/s_dead1_nb.sti",
    ANIMSDIR "/corpses/w_dead1_nb.sti",
    ANIMSDIR "/corpses/cp_dead1_nb.sti",
    ANIMSDIR "/corpses/m_dead2_nb.sti",
    ANIMSDIR "/corpses/k_dead1_nb.sti",
    ANIMSDIR "/corpses/h_dead1_nb.sti",

    ANIMSDIR "/corpses/ft_dead2_nb.sti",
    ANIMSDIR "/corpses/s_dead2_nb.sti",
    ANIMSDIR "/corpses/w_dead2_nb.sti",
    ANIMSDIR "/corpses/cp_dead2_nb.sti",
    ANIMSDIR "/corpses/ct_dead.sti",
    ANIMSDIR "/corpses/cw_dead1.sti",
    ANIMSDIR "/corpses/mn_dead2.sti",
    ANIMSDIR "/corpses/i_dead1.sti",
    ANIMSDIR "/corpses/l_dead1.sti",
    ANIMSDIR "/corpses/p_decomp2.sti",

    ANIMSDIR "/corpses/tk_wrek.sti",
    ANIMSDIR "/corpses/tk2_wrek.sti",
    ANIMSDIR "/corpses/hm_wrek.sti",
    ANIMSDIR "/corpses/ic_wrek.sti",
    ANIMSDIR "/corpses/qn_dead.sti",
    ANIMSDIR "/corpses/j_dead.sti",
    ANIMSDIR "/corpses/s_burnt.sti",
    ANIMSDIR "/corpses/s_expld.sti",
};

uint8_t gb4DirectionsFrom8[8] = {
    0,  // NORTH
    0,  // NE
    0,  // E
    0,  // SE
    1,  // S
    0,  // SW,
    2,  // W,
    0   // NW
};

static const uint8_t gb2DirectionsFrom8[8] = {
    0,  // NORTH
    7,  // NE
    7,  // E
    7,  // SE
    0,  // S
    7,  // SW,
    7,  // W,
    7   // NW
};

static const BOOLEAN gbCorpseValidForDecapitation[NUM_CORPSES] = {
    0,
    0,
    1,
    1,
    1,
    1,
    1,
    1,
    1,

    0,
    1,
    1,
    1,
    1,
    1,
    1,
    1,

    0,
    1,
    1,
    1,
    1,
    1,
    1,
    1,

    // Civs....
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,

    1,
    1,
    1,
    1,
    0,
    0,
    0,
    0,
    0,
    1,

    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};

static const int8_t gDecapitatedCorpse[NUM_CORPSES] = {
    0,
    SMERC_JFK,
    SMERC_JFK,
    SMERC_JFK,
    SMERC_JFK,
    SMERC_JFK,
    SMERC_JFK,
    SMERC_JFK,
    SMERC_JFK,

    MMERC_JFK,
    MMERC_JFK,
    MMERC_JFK,
    MMERC_JFK,
    MMERC_JFK,
    MMERC_JFK,
    MMERC_JFK,
    MMERC_JFK,

    FMERC_JFK,
    FMERC_JFK,
    FMERC_JFK,
    FMERC_JFK,
    FMERC_JFK,
    FMERC_JFK,
    FMERC_JFK,
    FMERC_JFK,

    // Civs....
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,

    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,

    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};

ROTTING_CORPSE gRottingCorpse[MAX_ROTTING_CORPSES];
int32_t giNumRottingCorpse = 0;

static ROTTING_CORPSE *GetFreeRottingCorpse() {
  for (ROTTING_CORPSE *c = gRottingCorpse; c != gRottingCorpse + giNumRottingCorpse; ++c) {
    if (!c->fActivated) return c;
  }
  if (giNumRottingCorpse < MAX_ROTTING_CORPSES) {
    return &gRottingCorpse[giNumRottingCorpse++];
  }
  return NULL;
}

uint16_t GetCorpseStructIndex(const ROTTING_CORPSE_DEFINITION *pCorpseDef, BOOLEAN fForImage) {
  int8_t bDirection;

  switch (pCorpseDef->ubType) {
    case QUEEN_MONSTER_DEAD:
    case BURNT_DEAD:
    case EXPLODE_DEAD:

      bDirection = 0;
      break;

    case ICECREAM_DEAD:
    case HUMMER_DEAD:

      // OK , these have 2 directions....
      bDirection = gb2DirectionsFrom8[pCorpseDef->bDirection];
      if (fForImage) bDirection = OneCDirection(bDirection);
      break;

    case SMERC_FALL:
    case SMERC_FALLF:
    case MMERC_FALL:
    case MMERC_FALLF:
    case FMERC_FALL:
    case FMERC_FALLF:

      // OK , these have 4 directions....
      bDirection = gb4DirectionsFrom8[pCorpseDef->bDirection];

      if (fForImage) {
        bDirection = OneCDirection(bDirection);
      }
      break;

    default:

      // Uses 8
      bDirection = pCorpseDef->bDirection;
      if (fForImage) bDirection = OneCDirection(bDirection);
      break;
  }

  return (bDirection);
}

static void CreateCorpsePalette(ROTTING_CORPSE *);

ROTTING_CORPSE *AddRottingCorpse(ROTTING_CORPSE_DEFINITION *const pCorpseDef) try {
  if (pCorpseDef->sGridNo == NOWHERE) return NULL;
  if (pCorpseDef->ubType == NO_CORPSE) return NULL;

  ROTTING_CORPSE *const c = GetFreeRottingCorpse();
  if (c == NULL) return NULL;

  // Copy elements in
  c->def = *pCorpseDef;

  // If we are a soecial type...
  AnimationFlags uiDirectionUseFlag;
  switch (pCorpseDef->ubType) {
    case SMERC_FALL:
    case SMERC_FALLF:
    case MMERC_FALL:
    case MMERC_FALLF:
    case FMERC_FALL:
    case FMERC_FALLF:
      uiDirectionUseFlag = ANITILE_USE_4DIRECTION_FOR_START_FRAME;
      break;
    default:
      uiDirectionUseFlag = ANITILE_USE_DIRECTION_FOR_START_FRAME;
      break;
  }

  if (!(gTacticalStatus.uiFlags & LOADING_SAVED_GAME)) {
    // OK, AS WE ADD, CHECK FOR TOD AND DECAY APPROPRIATELY
    if (GetWorldTotalMin() - c->def.uiTimeOfDeath > DELAY_UNTIL_ROTTING &&
        c->def.ubType < ROTTING_STAGE2 && c->def.ubType <= FMERC_FALLF) {
      // Rott!
      c->def.ubType = ROTTING_STAGE2;
    }

    // If time of death is a few days, now, don't add at all!
    if (GetWorldTotalMin() - c->def.uiTimeOfDeath > DELAY_UNTIL_DONE_ROTTING) return NULL;
  }

  // Check if on roof or not...
  AnimationLevel const ubLevelID = (c->def.bLevel == 0 ? ANI_STRUCT_LEVEL : ANI_ONROOF_LEVEL);

  ANITILE_PARAMS AniParams;
  memset(&AniParams, 0, sizeof(AniParams));
  AniParams.sGridNo = c->def.sGridNo;
  AniParams.ubLevelID = ubLevelID;
  AniParams.sDelay = 150;
  AniParams.sStartFrame = 0;
  AniParams.uiFlags = ANITILE_PAUSED | ANITILE_OPTIMIZEFORSLOWMOVING | ANITILE_ANIMATE_Z |
                      ANITILE_ERASEITEMFROMSAVEBUFFFER | uiDirectionUseFlag;
  AniParams.sX = CenterX(c->def.sGridNo);
  AniParams.sY = CenterY(c->def.sGridNo);
  AniParams.sZ = c->def.sHeightAdjustment;
  AniParams.v.user.uiData3 = c->def.bDirection;

  if (!gGameSettings.fOptions[TOPTION_BLOOD_N_GORE]) {
    AniParams.zCachedFile = zNoBloodCorpseFilenames[c->def.ubType];
  } else {
    AniParams.zCachedFile = zCorpseFilenames[c->def.ubType];
  }

  ANITILE *const ani = CreateAnimationTile(&AniParams);
  c->pAniTile = ani;

  LEVELNODE *const n = ani->pLevelNode;
  const LEVELNODE *const land = gpWorldLevelData[c->def.sGridNo].pLandHead;

  // Set flag and index values
  n->uiFlags |= LEVELNODE_ROTTINGCORPSE;
  n->ubShadeLevel = land->ubShadeLevel;
  n->ubSumLights = land->ubSumLights;
  n->ubMaxLights = land->ubMaxLights;
  n->ubNaturalShadeLevel = land->ubNaturalShadeLevel;

  // Get palette and create palettes and do substitutions
  CreateCorpsePalette(c);

  c->fActivated = TRUE;
  ani->v.user.uiData = CORPSE2ID(c);

  SetRenderFlags(RENDER_FLAG_FULL);

  if (c->def.usFlags & ROTTING_CORPSE_VEHICLE) {
    ani->uiFlags |= ANITILE_FORWARD | ANITILE_LOOPING;
    ani->uiFlags &= ~ANITILE_PAUSED;
  }

  InvalidateWorldRedundency();

  // OK, loop through gridnos for corpse and remove blood.....

  // Get root filename... this removes path and extension
  // Used to find struct data for this corpse...
  char zFilename[150];
  GetRootName(zFilename, lengthof(zFilename), AniParams.zCachedFile);

  // Add structure data.....
  CheckForAndAddTileCacheStructInfo(n, c->def.sGridNo, ani->sCachedTileID,
                                    GetCorpseStructIndex(pCorpseDef, TRUE));

  const STRUCTURE_FILE_REF *const pStructureFileRef =
      GetCachedTileStructureRefFromFilename(zFilename);
  if (pStructureFileRef != NULL) {
    const uint16_t usStructIndex = GetCorpseStructIndex(pCorpseDef, TRUE);
    const DB_STRUCTURE_REF *const pDBStructureRef =
        &pStructureFileRef->pDBStructureRef[usStructIndex];
    for (uint8_t ubLoop = 0; ubLoop < pDBStructureRef->pDBStructure->ubNumberOfTiles; ++ubLoop) {
      DB_STRUCTURE_TILE *const *const ppTile = pDBStructureRef->ppTile;
      const int16_t sTileGridNo = pCorpseDef->sGridNo + ppTile[ubLoop]->sPosRelToBase;
      RemoveBlood(sTileGridNo, pCorpseDef->bLevel);
    }
  }

  return c;
} catch (...) {
  return 0;
}

static void FreeCorpsePalettes(ROTTING_CORPSE *pCorpse) {
  int32_t cnt;

  for (cnt = 0; cnt < NUM_CORPSE_SHADES; cnt++) {
    if (pCorpse->pShades[cnt] != NULL) {
      MemFree(pCorpse->pShades[cnt]);
      pCorpse->pShades[cnt] = NULL;
    }
  }
}

static void RemoveCorpse(ROTTING_CORPSE *c);

void RemoveCorpses() {
  FOR_EACH_ROTTING_CORPSE(c) RemoveCorpse(c);
  giNumRottingCorpse = 0;
}

static void RemoveCorpse(ROTTING_CORPSE *const c) {
  Assert(gRottingCorpse <= c && c < endof(gRottingCorpse));
  Assert(c->fActivated);

  c->fActivated = FALSE;
  DeleteAniTile(c->pAniTile);
  FreeCorpsePalettes(c);
}

static void CreateCorpsePalette(ROTTING_CORPSE *const c) {
  char const *const substitution = c->def.ubType == ROTTING_STAGE2 ? ""
                                   : c->def.usFlags & ROTTING_CORPSE_USE_CAMO_PALETTE
                                       ? ANIMSDIR "/camo.COL"
                                       : GetBodyTypePaletteSubstitution(0, c->def.ubBodyType);

  const SGPPaletteEntry *pal;
  SGPPaletteEntry tmp_pal[256];
  if (!substitution) {
    // Use palette from HVOBJECT, then use substitution for pants, etc
    memcpy(tmp_pal, gpTileCache[c->pAniTile->sCachedTileID].pImagery->vo->Palette(),
           sizeof(tmp_pal));
    SetPaletteReplacement(tmp_pal, c->def.HeadPal);
    SetPaletteReplacement(tmp_pal, c->def.VestPal);
    SetPaletteReplacement(tmp_pal, c->def.PantsPal);
    SetPaletteReplacement(tmp_pal, c->def.SkinPal);
    pal = tmp_pal;
  } else if (substitution[0] != '\0' && CreateSGPPaletteFromCOLFile(tmp_pal, substitution)) {
    pal = tmp_pal;
  } else {
    // Use palette from hvobject
    pal = gpTileCache[c->pAniTile->sCachedTileID].pImagery->vo->Palette();
  }

  CreateBiasedShadedPalettes(c->pShades, pal);
}

BOOLEAN TurnSoldierIntoCorpse(SOLDIERTYPE &s) {
  ROTTING_CORPSE_DEFINITION Corpse;
  uint8_t ubType;
  uint16_t usItemFlags = 0;  // WORLD_ITEM_DONTRENDER;
  uint8_t ubNumGoo;
  int16_t sNewGridNo;
  OBJECTTYPE ItemObject;

  if (s.sGridNo == NOWHERE) {
    return (FALSE);
  }

  // ATE: Change to fix crash when item in hand
  if (gpItemPointer && gpItemPointerSoldier == &s) {
    CancelItemPointer();
  }

  // Setup some values!
  memset(&Corpse, 0, sizeof(Corpse));
  Corpse.ubBodyType = s.ubBodyType;
  Corpse.sGridNo = s.sGridNo;
  Corpse.bLevel = s.bLevel;
  Corpse.ubProfile = s.ubProfile;

  if (Corpse.bLevel > 0) {
    Corpse.sHeightAdjustment = s.sHeightAdjustment - WALL_HEIGHT;
  }

  SET_PALETTEREP_ID(Corpse.HeadPal, s.HeadPal);
  SET_PALETTEREP_ID(Corpse.VestPal, s.VestPal);
  SET_PALETTEREP_ID(Corpse.SkinPal, s.SkinPal);
  SET_PALETTEREP_ID(Corpse.PantsPal, s.PantsPal);

  if (s.bCamo != 0) {
    Corpse.usFlags |= ROTTING_CORPSE_USE_CAMO_PALETTE;
  }

  // Determine corpse type!
  ubType = gubAnimSurfaceCorpseID[s.ubBodyType][s.usAnimState];

  Corpse.bDirection = s.bDirection;

  // If we are a vehicle.... only use 1 direction....
  if (s.uiStatusFlags & SOLDIER_VEHICLE) {
    Corpse.usFlags |= ROTTING_CORPSE_VEHICLE;

    if (s.ubBodyType != ICECREAMTRUCK && s.ubBodyType != HUMVEE) {
      Corpse.bDirection = 7;
    } else {
      Corpse.bDirection = gb2DirectionsFrom8[Corpse.bDirection];
    }
  }

  if (ubType == QUEEN_MONSTER_DEAD || ubType == BURNT_DEAD || ubType == EXPLODE_DEAD) {
    Corpse.bDirection = 7;
  }

  // ATE: If bDirection, get opposite
  //	if ( ubType == SMERC_FALLF || ubType == MMERC_FALLF || ubType ==
  // FMERC_FALLF )
  //{
  //	Corpse.bDirection = OppositeDirection(Corpse.bDirection);
  //	}

  // Set time of death
  Corpse.uiTimeOfDeath = GetWorldTotalMin();

  // If corpse is not valid. make items visible
  if (ubType == NO_CORPSE && s.bTeam != OUR_TEAM) {
    usItemFlags &= (~WORLD_ITEM_DONTRENDER);
  }

  // ATE: If the queen is killed, she should
  // make items visible because it ruins end sequence....
  Visibility const bVisible = s.bTeam == OUR_TEAM || s.ubProfile == QUEEN ? VISIBLE : INVISIBLE;

  // Not for a robot...
  if (AM_A_ROBOT(&s)) {
  } else if (ubType == QUEEN_MONSTER_DEAD) {
    gTacticalStatus.fLockItemLocators = FALSE;

    ubNumGoo = 6 - (gGameOptions.ubDifficultyLevel - DIF_LEVEL_EASY);

    sNewGridNo = s.sGridNo + WORLD_COLS * 2;

    for (int32_t cnt = 0; cnt < ubNumGoo; ++cnt) {
      CreateItem(JAR_QUEEN_CREATURE_BLOOD, 100, &ItemObject);

      AddItemToPool(sNewGridNo, &ItemObject, bVisible, s.bLevel, usItemFlags, -1);
    }
  } else {
    // OK, Place what objects this guy was carrying on the ground!
    FOR_EACH_SOLDIER_INV_SLOT(pObj, s) {
      if (pObj->usItem != NOTHING) {
        // Check if it's supposed to be dropped
        if (!(pObj->fFlags & OBJECT_UNDROPPABLE) || s.bTeam == OUR_TEAM) {
          // and make sure that it really is a droppable item type
          if (!(Item[pObj->usItem].fFlags & ITEM_DEFAULT_UNDROPPABLE)) {
            ReduceAmmoDroppedByNonPlayerSoldiers(s, *pObj);
            AddItemToPool(s.sGridNo, pObj, bVisible, s.bLevel, usItemFlags, -1);
          }
        }
      }
    }

    DropKeysInKeyRing(s, s.sGridNo, s.bLevel, bVisible, false, 0, false);
  }

  // Make team look for items
  AllSoldiersLookforItems();

  // If not a player, you can completely remove soldiertype
  // otherwise, just remove their graphic
  if (s.bTeam != OUR_TEAM) {
    // Remove merc!
    // ATE: Remove merc slot first - will disappear if no corpse data found!
    TacticalRemoveSoldier(s);
  } else {
    RemoveSoldierFromGridNo(s);
  }

  if (ubType == NO_CORPSE) {
    return (TRUE);
  }

  // Set type
  Corpse.ubType = ubType;
  ROTTING_CORPSE *const added_corpse = AddRottingCorpse(&Corpse);

  // If this is our guy......make visible...
  // if (s.bTeam == OUR_TEAM)
  { added_corpse->def.bVisible = 1; }

  return (TRUE);
}

int16_t FindNearestRottingCorpse(SOLDIERTYPE *pSoldier) {
  int32_t uiLowestRange = 999999;
  int16_t sLowestGridNo = NOWHERE;

  // OK, loop through our current listing of bodies
  CFOR_EACH_ROTTING_CORPSE(c) {
    // Check rotting state
    if (c->def.ubType == ROTTING_STAGE2) {
      const int32_t uiRange = GetRangeInCellCoordsFromGridNoDiff(pSoldier->sGridNo, c->def.sGridNo);
      if (uiRange < uiLowestRange) {
        sLowestGridNo = c->def.sGridNo;
        uiLowestRange = uiRange;
      }
    }
  }

  return (sLowestGridNo);
}

static void AddCrowToCorpse(ROTTING_CORPSE *pCorpse) {
  SOLDIERCREATE_STRUCT MercCreateStruct;
  int8_t bBodyType = CROW;

  // No crows inside :(
  if (GetRoom(pCorpse->def.sGridNo) != NO_ROOM) return;

  // Put him flying over corpse pisition
  memset(&MercCreateStruct, 0, sizeof(MercCreateStruct));
  MercCreateStruct.ubProfile = NO_PROFILE;
  MercCreateStruct.sSectorX = gWorldSectorX;
  MercCreateStruct.sSectorY = gWorldSectorY;
  MercCreateStruct.bSectorZ = gbWorldSectorZ;
  MercCreateStruct.bBodyType = bBodyType;
  MercCreateStruct.bDirection = SOUTH;
  MercCreateStruct.bTeam = CIV_TEAM;
  MercCreateStruct.sInsertionGridNo = pCorpse->def.sGridNo;
  RandomizeNewSoldierStats(&MercCreateStruct);

  SOLDIERTYPE *const pSoldier = TacticalCreateSoldier(MercCreateStruct);
  if (pSoldier != NULL) {
    const int16_t sGridNo = FindRandomGridNoFromSweetSpot(pSoldier, pCorpse->def.sGridNo, 2);
    if (sGridNo != NOWHERE) {
      pSoldier->ubStrategicInsertionCode = INSERTION_CODE_GRIDNO;
      pSoldier->usStrategicInsertionData = sGridNo;

      pSoldier->sInsertionGridNo = sGridNo;
      pSoldier->sDesiredHeight = 0;

      AddSoldierToSector(pSoldier);

      // Change to fly animation
      // sGridNo = FindRandomGridNoFromSweetSpot(pSoldier, pCorpse->def.sGridNo,
      // 5); pSoldier->usUIMovementMode = CROW_FLY; EVENT_GetNewSoldierPath(
      // pSoldier, sGridNo, pSoldier->usUIMovementMode );

      // Setup action data to point back to corpse....
      pSoldier->uiPendingActionData1 = CORPSE2ID(pCorpse);
      pSoldier->sPendingActionData2 = pCorpse->def.sGridNo;

      pCorpse->def.bNumServicingCrows++;
    }
  }
}

void HandleCrowLeave(SOLDIERTYPE *pSoldier) {
  // Check if this crow is still referencing the same corpse...
  ROTTING_CORPSE *const pCorpse = ID2CORPSE(pSoldier->uiPendingActionData1);

  // Double check grindo...
  if (pSoldier->sPendingActionData2 == pCorpse->def.sGridNo) {
    // We have a match
    // Adjust crow servicing count...
    pCorpse->def.bNumServicingCrows--;

    if (pCorpse->def.bNumServicingCrows < 0) {
      pCorpse->def.bNumServicingCrows = 0;
    }
  }
}

void HandleCrowFlyAway(SOLDIERTYPE *pSoldier) {
  // Set desired height
  pSoldier->sDesiredHeight = 100;

  // Change to fly animation
  const int16_t sGridNo = FindRandomGridNoFromSweetSpot(pSoldier, pSoldier->sGridNo, 5);
  pSoldier->usUIMovementMode = CROW_FLY;
  SendGetNewSoldierPathEvent(pSoldier, sGridNo);
}

void HandleRottingCorpses() {
  ROTTING_CORPSE *pCorpse;
  int8_t bNumCrows = 0;
  uint32_t uiChosenCorpseID;

  // ATE: If it's too late, don't!
  if (NightTime()) {
    return;
  }

  if (gbWorldSectorZ > 0) {
    return;
  }

  // ATE: Check for multiple crows.....
  // Couint how many we have now...
  CFOR_EACH_IN_TEAM(s, CIV_TEAM) {
    if (s->bInSector && s->bLife >= OKLIFE && !(s->uiStatusFlags & SOLDIER_GASSED) &&
        s->ubBodyType == CROW) {
      bNumCrows++;
    }
  }

  // Once population gets down to 0, we can add more again....
  if (bNumCrows == 0) {
    gTacticalStatus.fDontAddNewCrows = FALSE;
  }

  if (gTacticalStatus.fDontAddNewCrows) {
    return;
  }

  if (bNumCrows >= gTacticalStatus.ubNumCrowsPossible) {
    gTacticalStatus.fDontAddNewCrows = TRUE;
    return;
  }

  if (IsTeamActive(CREATURE_TEAM)) {
    // don't add any crows while there are predators around
    return;
  }

  // Pick one to attact a crow...
  {
    uiChosenCorpseID = Random(giNumRottingCorpse);

    pCorpse = &(gRottingCorpse[uiChosenCorpseID]);

    if (pCorpse->fActivated) {
      if (!(pCorpse->def.usFlags & ROTTING_CORPSE_VEHICLE)) {
        if (pCorpse->def.ubType == ROTTING_STAGE2) {
          if (GridNoOnScreen(pCorpse->def.sGridNo)) {
            return;
          }

          AddCrowToCorpse(pCorpse);
          AddCrowToCorpse(pCorpse);
        }
      }
    }
  }
}

static ROTTING_CORPSE *FindCorpseBasedOnStructure(GridNo const grid_no,
                                                  STRUCTURE *const structure) {
  for (LEVELNODE const *i = gpWorldLevelData[grid_no].pStructHead; i; i = i->pNext) {
    if (i->pStructureData != structure) continue;
    return ID2CORPSE(i->pAniTile->v.user.uiData);
  }
  return 0;
}

void CorpseHit(int16_t sGridNo, uint16_t usStructureID) {}

void VaporizeCorpse(int16_t sGridNo, uint16_t usStructureID) {
  STRUCTURE *pStructure, *pBaseStructure;
  ROTTING_CORPSE *pCorpse = NULL;
  int16_t sBaseGridNo;
  ANITILE_PARAMS AniParams;

  pStructure = FindStructureByID(sGridNo, usStructureID);

  // Get base....
  pBaseStructure = FindBaseStructure(pStructure);

  // Find base gridno...
  sBaseGridNo = pBaseStructure->sGridNo;

  // Get corpse ID.....
  pCorpse = FindCorpseBasedOnStructure(sBaseGridNo, pBaseStructure);

  if (pCorpse == NULL) {
    return;
  }

  if (pCorpse->def.usFlags & ROTTING_CORPSE_VEHICLE) {
    return;
  }

  if (GridNoOnScreen(sBaseGridNo)) {
    // Add explosion
    memset(&AniParams, 0, sizeof(ANITILE_PARAMS));
    AniParams.sGridNo = sBaseGridNo;
    AniParams.ubLevelID = ANI_STRUCT_LEVEL;
    AniParams.sDelay = (int16_t)(80);
    AniParams.sStartFrame = 0;
    AniParams.uiFlags = ANITILE_FORWARD;
    AniParams.sX = CenterX(sBaseGridNo);
    AniParams.sY = CenterY(sBaseGridNo);
    AniParams.sZ = (int16_t)pCorpse->def.sHeightAdjustment;
    AniParams.zCachedFile = TILECACHEDIR "/gen_blow.sti";
    CreateAnimationTile(&AniParams);

    RemoveCorpse(pCorpse);
    SetRenderFlags(RENDER_FLAG_FULL);

    if (pCorpse->def.bLevel == 0) {
      // Set some blood......
      SpreadEffect(sBaseGridNo, 2, 0, NULL, BLOOD_SPREAD_EFFECT, 0, NULL);
    }
  }

  // PLay a sound....
  PlayLocationJA2Sample(sGridNo, BODY_EXPLODE_1, HIGHVOLUME, 1);
}

int16_t FindNearestAvailableGridNoForCorpse(ROTTING_CORPSE_DEFINITION *pDef, int8_t ubRadius) {
  int16_t sSweetGridNo;
  int16_t sTop, sBottom;
  int16_t sLeft, sRight;
  int16_t cnt1, cnt2, cnt3;
  int16_t sGridNo;
  int32_t uiRange, uiLowestRange = 999999;
  int16_t sLowestGridNo = 0;
  int32_t leftmost;
  BOOLEAN fFound = FALSE;
  SOLDIERTYPE soldier;
  uint8_t ubSaveNPCAPBudget;
  uint8_t ubSaveNPCDistLimit;
  STRUCTURE_FILE_REF *pStructureFileRef = NULL;
  uint8_t ubBestDirection = 0;
  BOOLEAN fSetDirection = FALSE;

  cnt3 = 0;

  // Get root filename... this removes path and extension
  // USed to find struct data fo rthis corpse...
  char zFilename[150];
  GetRootName(zFilename, lengthof(zFilename), zCorpseFilenames[pDef->ubType]);

  pStructureFileRef = GetCachedTileStructureRefFromFilename(zFilename);

  sSweetGridNo = pDef->sGridNo;

  // Save AI pathing vars.  changing the distlimit restricts how
  // far away the pathing will consider.
  ubSaveNPCAPBudget = gubNPCAPBudget;
  ubSaveNPCDistLimit = gubNPCDistLimit;
  gubNPCAPBudget = 0;
  gubNPCDistLimit = ubRadius;

  // create dummy soldier, and use the pathing to determine which nearby slots
  // are reachable.
  memset(&soldier, 0, sizeof(SOLDIERTYPE));
  soldier.bTeam = 1;
  soldier.sGridNo = sSweetGridNo;

  sTop = ubRadius;
  sBottom = -ubRadius;
  sLeft = -ubRadius;
  sRight = ubRadius;

  // clear the mapelements of potential residue MAPELEMENT_REACHABLE flags
  // in the square region.
  for (cnt1 = sBottom; cnt1 <= sTop; cnt1++) {
    for (cnt2 = sLeft; cnt2 <= sRight; cnt2++) {
      sGridNo = sSweetGridNo + (WORLD_COLS * cnt1) + cnt2;
      if (sGridNo >= 0 && sGridNo < WORLD_MAX) {
        gpWorldLevelData[sGridNo].uiFlags &= (~MAPELEMENT_REACHABLE);
      }
    }
  }

  // Now, find out which of these gridnos are reachable
  //(use the fake soldier and the pathing settings)
  FindBestPath(&soldier, NOWHERE, 0, WALKING, COPYREACHABLE, 0);

  uiLowestRange = 999999;

  for (cnt1 = sBottom; cnt1 <= sTop; cnt1++) {
    leftmost = ((sSweetGridNo + (WORLD_COLS * cnt1)) / WORLD_COLS) * WORLD_COLS;

    for (cnt2 = sLeft; cnt2 <= sRight; cnt2++) {
      sGridNo = sSweetGridNo + (WORLD_COLS * cnt1) + cnt2;
      if (sGridNo >= 0 && sGridNo < WORLD_MAX && sGridNo >= leftmost &&
          sGridNo < (leftmost + WORLD_COLS) &&
          gpWorldLevelData[sGridNo].uiFlags & MAPELEMENT_REACHABLE) {
        // Go on sweet stop
        if (NewOKDestination(&soldier, sGridNo, TRUE, soldier.bLevel)) {
          BOOLEAN fDirectionFound = FALSE;
          BOOLEAN fCanSetDirection = FALSE;

          // Check each struct in each direction
          if (pStructureFileRef == NULL) {
            fDirectionFound = TRUE;
          } else {
            for (cnt3 = 0; cnt3 < 8; cnt3++) {
              if (OkayToAddStructureToWorld(
                      sGridNo, pDef->bLevel,
                      &pStructureFileRef->pDBStructureRef[OneCDirection(cnt3)],
                      INVALID_STRUCTURE_ID)) {
                fDirectionFound = TRUE;
                fCanSetDirection = TRUE;
                break;
              }
            }
          }

          if (fDirectionFound) {
            uiRange = GetRangeInCellCoordsFromGridNoDiff(sSweetGridNo, sGridNo);

            if (uiRange < uiLowestRange) {
              if (fCanSetDirection) {
                ubBestDirection = (uint8_t)cnt3;
                fSetDirection = TRUE;
              }
              sLowestGridNo = sGridNo;
              uiLowestRange = uiRange;
              fFound = TRUE;
            }
          }
        }
      }
    }
  }
  gubNPCAPBudget = ubSaveNPCAPBudget;
  gubNPCDistLimit = ubSaveNPCDistLimit;
  if (fFound) {
    if (fSetDirection) {
      pDef->bDirection = ubBestDirection;
    }

    return sLowestGridNo;
  }
  return NOWHERE;
}

BOOLEAN IsValidDecapitationCorpse(const ROTTING_CORPSE *const c) {
  if (c->def.fHeadTaken) return FALSE;
  return gbCorpseValidForDecapitation[c->def.ubType];
}

ROTTING_CORPSE *GetCorpseAtGridNo(int16_t sGridNo, int8_t bLevel) {
  STRUCTURE *pStructure, *pBaseStructure;
  int16_t sBaseGridNo;

  pStructure = FindStructure(sGridNo, STRUCTURE_CORPSE);

  if (pStructure != NULL) {
    // Get base....
    pBaseStructure = FindBaseStructure(pStructure);

    // Find base gridno...
    sBaseGridNo = pBaseStructure->sGridNo;

    if (pBaseStructure != NULL) {
      return (FindCorpseBasedOnStructure(sBaseGridNo, pBaseStructure));
    }
  }

  return (NULL);
}

void DecapitateCorpse(const int16_t sGridNo, const int8_t bLevel) {
  OBJECTTYPE Object;
  ROTTING_CORPSE *pCorpse;
  ROTTING_CORPSE_DEFINITION CorpseDef;

  pCorpse = GetCorpseAtGridNo(sGridNo, bLevel);

  if (pCorpse == NULL) {
    return;
  }

  if (IsValidDecapitationCorpse(pCorpse)) {
    // Decapitate.....
    // Copy corpse definition...
    CorpseDef = pCorpse->def;

    // Add new one...
    CorpseDef.ubType = gDecapitatedCorpse[CorpseDef.ubType];

    pCorpse->def.fHeadTaken = TRUE;

    if (CorpseDef.ubType != 0) {
      RemoveCorpse(pCorpse);
      AddRottingCorpse(&CorpseDef);
    }

    // Add head item.....

    // Pick the head based on profile type...
    uint16_t head_index;
    switch (pCorpse->def.ubProfile) {
      case CHRIS:
        head_index = HEAD_2;
        break;
      case T_REX:
        head_index = HEAD_3;
        break;
      case SLAY:
        head_index = HEAD_4;
        break;
      case DRUGGIST:
        head_index = HEAD_5;
        break;
      case ANNIE:
        head_index = HEAD_6;
        break;
      case TIFFANY:
        head_index = HEAD_7;
        break;
      default:
        head_index = HEAD_1;
        break;
    }

    CreateItem(head_index, 100, &Object);
    AddItemToPool(sGridNo, &Object, INVISIBLE, 0, 0, 0);

    // All teams lok for this...
    NotifySoldiersToLookforItems();
  }
}

void GetBloodFromCorpse(SOLDIERTYPE *pSoldier) {
  const ROTTING_CORPSE *const pCorpse = ID2CORPSE(pSoldier->uiPendingActionData4);
  int8_t bObjSlot;
  OBJECTTYPE Object;

  bObjSlot = FindObj(pSoldier, JAR);

  // What kind of corpse ami I?
  switch (pCorpse->def.ubType) {
    case ADULTMONSTER_DEAD:
    case INFANTMONSTER_DEAD:

      // Can get creature blood....
      CreateItem(JAR_CREATURE_BLOOD, 100, &Object);
      break;

    case QUEEN_MONSTER_DEAD:
      CreateItem(JAR_QUEEN_CREATURE_BLOOD, 100, &Object);
      break;

    default:

      CreateItem(JAR_HUMAN_BLOOD, 100, &Object);
      break;
  }

  if (bObjSlot != NO_SLOT) {
    SwapObjs(&(pSoldier->inv[bObjSlot]), &Object);
  }
}

void ReduceAmmoDroppedByNonPlayerSoldiers(SOLDIERTYPE const &s, OBJECTTYPE &o) {
  if (s.bTeam == OUR_TEAM) return;
  if (Item[o.usItem].usItemClass != IC_AMMO) return;

  /* Don't drop all the clips, just a random # of them between 1 and how
   * many there are */
  o.ubNumberOfObjects = 1 + Random(o.ubNumberOfObjects);
  o.ubWeight = CalculateObjectWeight(&o);
}

void LookForAndMayCommentOnSeeingCorpse(SOLDIERTYPE *pSoldier, int16_t sGridNo, uint8_t ubLevel) {
  ROTTING_CORPSE *pCorpse;
  int8_t bToleranceThreshold = 0;

  if (QuoteExp_HeadShotOnly[pSoldier->ubProfile] == 1) {
    return;
  }

  pCorpse = GetCorpseAtGridNo(sGridNo, ubLevel);

  if (pCorpse == NULL) {
    return;
  }

  if (pCorpse->def.ubType != ROTTING_STAGE2) {
    return;
  }

  // If servicing qrows, tolerance is now 1
  if (pCorpse->def.bNumServicingCrows > 0) {
    bToleranceThreshold++;
  }

  // Check tolerance
  if (pSoldier->bCorpseQuoteTolerance <= bToleranceThreshold) {
    // Say quote...
    TacticalCharacterDialogue(pSoldier, QUOTE_HEADSHOT);

    BeginMultiPurposeLocator(sGridNo, ubLevel);

    // Reset values....
    pSoldier->bCorpseQuoteTolerance = (int8_t)(Random(3) + 1);

    // 50% chance of adding 1 to other mercs....
    if (Random(2) == 1) {
      FOR_EACH_IN_TEAM(s, OUR_TEAM) {
        // ATE: Ok, lets check for some basic things here!
        if (s->bLife >= OKLIFE && s->sGridNo != NOWHERE && s->bInSector) {
          ++s->bCorpseQuoteTolerance;
        }
      }
    }
  }
}

int16_t GetGridNoOfCorpseGivenProfileID(const uint8_t ubProfileID) {
  // Loop through all corpses....
  CFOR_EACH_ROTTING_CORPSE(c) {
    if (c->def.ubProfile == ubProfileID) return c->def.sGridNo;
  }
  return NOWHERE;
}

void DecayRottingCorpseAIWarnings() {
  FOR_EACH_ROTTING_CORPSE(c) {
    if (c->def.ubAIWarningValue > 0) --c->def.ubAIWarningValue;
  }
}

uint8_t GetNearestRottingCorpseAIWarning(const int16_t sGridNo) {
  uint8_t ubHighestWarning = 0;
  CFOR_EACH_ROTTING_CORPSE(c) {
    if (c->def.ubAIWarningValue > 0 &&
        PythSpacesAway(sGridNo, c->def.sGridNo) <= CORPSE_WARNING_DIST &&
        c->def.ubAIWarningValue > ubHighestWarning) {
      ubHighestWarning = c->def.ubAIWarningValue;
    }
  }
  return ubHighestWarning;
}
