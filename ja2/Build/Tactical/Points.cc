#include "Tactical/Points.h"

#include <algorithm>

#include "Macro.h"
#include "SGP/Debug.h"
#include "SGP/WCheck.h"
#include "Tactical/AnimationControl.h"
#include "Tactical/DialogueControl.h"
#include "Tactical/DrugsAndAlcohol.h"
#include "Tactical/HandleItems.h"
#include "Tactical/HandleUI.h"
#include "Tactical/Interface.h"
#include "Tactical/InterfaceItems.h"
#include "Tactical/Items.h"
#include "Tactical/Overhead.h"
#include "Tactical/PathAI.h"
#include "Tactical/RTTimeDefines.h"
#include "Tactical/SkillCheck.h"
#include "Tactical/SoldierFind.h"
#include "Tactical/SoldierMacros.h"
#include "Tactical/SoldierProfile.h"
#include "Tactical/StructureWrap.h"
#include "Tactical/Weapons.h"
#include "TacticalAI/AI.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/TileDef.h"
#include "TileEngine/WorldDef.h"
#include "TileEngine/WorldMan.h"
#include "Utils/FontControl.h"
#include "Utils/Message.h"
#include "Utils/Text.h"

int16_t TerrainActionPoints(const SOLDIERTYPE *const pSoldier, const int16_t sGridno,
                            const int8_t bDir, const int8_t bLevel) {
  int16_t sAPCost = 0;
  int16_t sSwitchValue;

  if (pSoldier->bStealthMode) sAPCost += AP_STEALTH_MODIFIER;

  if (pSoldier->bReverse || gUIUseReverse) sAPCost += AP_REVERSE_MODIFIER;

  sSwitchValue = gubWorldMovementCosts[sGridno][bDir][bLevel];

  // Check reality vs what the player knows....
  if (sSwitchValue == TRAVELCOST_NOT_STANDING) {
    // use the cost of the terrain!
    sSwitchValue = gTileTypeMovementCost[gpWorldLevelData[sGridno].ubTerrainID];
  } else if (IS_TRAVELCOST_DOOR(sSwitchValue)) {
    sSwitchValue =
        DoorTravelCost(pSoldier, sGridno, (uint8_t)sSwitchValue, pSoldier->bTeam == OUR_TEAM, NULL);
  }

  if (sSwitchValue >= TRAVELCOST_BLOCKED && sSwitchValue != TRAVELCOST_DOOR) {
    return (100);  // Cost too much to be considered!
  }

  switch (sSwitchValue) {
    case TRAVELCOST_DIRTROAD:
    case TRAVELCOST_FLAT:
      sAPCost += AP_MOVEMENT_FLAT;
      break;
      // case TRAVELCOST_BUMPY		:
    case TRAVELCOST_GRASS:
      sAPCost += AP_MOVEMENT_GRASS;
      break;
    case TRAVELCOST_THICK:
      sAPCost += AP_MOVEMENT_BUSH;
      break;
    case TRAVELCOST_DEBRIS:
      sAPCost += AP_MOVEMENT_RUBBLE;
      break;
    case TRAVELCOST_SHORE:
      sAPCost += AP_MOVEMENT_SHORE;  // wading shallow water
      break;
    case TRAVELCOST_KNEEDEEP:
      sAPCost += AP_MOVEMENT_LAKE;  // wading waist/chest deep - very slow
      break;

    case TRAVELCOST_DEEPWATER:
      sAPCost += AP_MOVEMENT_OCEAN;  // can swim, so it's faster than wading
      break;
    case TRAVELCOST_DOOR:
      sAPCost += AP_MOVEMENT_FLAT;
      break;

      // cost for jumping a fence REPLACES all other AP costs!
    case TRAVELCOST_FENCE:
      return (AP_JUMPFENCE);

    case TRAVELCOST_NONE:
      return (0);

    default:

      DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
               String("Calc AP: Unrecongnized MP type %d in %d, direction %d", sSwitchValue,
                      sGridno, bDir));
      break;
  }

  if (bDir & 1) {
    sAPCost = (sAPCost * 14) / 10;
  }

  return (sAPCost);
}

static int16_t BreathPointAdjustmentForCarriedWeight(SOLDIERTYPE *pSoldier) {
  uint32_t uiCarriedPercent;
  uint32_t uiPercentCost;

  uiCarriedPercent = CalculateCarriedWeight(pSoldier);
  if (uiCarriedPercent < 101) {
    // normal BP costs
    uiPercentCost = 100;
  } else {
    if (uiCarriedPercent < 151) {
      // between 101 and 150% of max carried weight, extra BP cost
      // of 1% per % above 100... so at 150%, we pay 150%
      uiPercentCost = 100 + (uiCarriedPercent - 100) * 3;
    } else if (uiCarriedPercent < 201) {
      // between 151 and 200% of max carried weight, extra BP cost
      // of 2% per % above 150... so at 200%, we pay 250%
      uiPercentCost = 100 + (uiCarriedPercent - 100) * 3 + (uiCarriedPercent - 150);
    } else {
      // over 200%, extra BP cost of 3% per % above 200
      uiPercentCost =
          100 + (uiCarriedPercent - 100) * 3 + (uiCarriedPercent - 150) + (uiCarriedPercent - 200);
      // so at 250% weight, we pay 400% breath!
    }
  }
  return ((int16_t)uiPercentCost);
}

int16_t TerrainBreathPoints(SOLDIERTYPE *pSoldier, int16_t sGridno, int8_t bDir,
                            uint16_t usMovementMode) {
  int32_t iPoints = 0;
  uint8_t ubMovementCost;

  ubMovementCost = gubWorldMovementCosts[sGridno][bDir][0];

  switch (ubMovementCost) {
    case TRAVELCOST_DIRTROAD:
    case TRAVELCOST_FLAT:
      iPoints = BP_MOVEMENT_FLAT;
      break;
      // case TRAVELCOST_BUMPY			:
    case TRAVELCOST_GRASS:
      iPoints = BP_MOVEMENT_GRASS;
      break;
    case TRAVELCOST_THICK:
      iPoints = BP_MOVEMENT_BUSH;
      break;
    case TRAVELCOST_DEBRIS:
      iPoints = BP_MOVEMENT_RUBBLE;
      break;
    case TRAVELCOST_SHORE:
      iPoints = BP_MOVEMENT_SHORE;
      break;  // wading shallow water
    case TRAVELCOST_KNEEDEEP:
      iPoints = BP_MOVEMENT_LAKE;
      break;  // wading waist/chest deep - very slow
    case TRAVELCOST_DEEPWATER:
      iPoints = BP_MOVEMENT_OCEAN;
      break;  // can swim, so it's faster than wading
    default:
      if (IS_TRAVELCOST_DOOR(ubMovementCost)) {
        iPoints = BP_MOVEMENT_FLAT;
        break;
      }
      return (0);
  }

  iPoints = iPoints * BreathPointAdjustmentForCarriedWeight(pSoldier) / 100;

  // ATE - MAKE MOVEMENT ALWAYS WALK IF IN WATER
  if (gpWorldLevelData[sGridno].ubTerrainID == DEEP_WATER ||
      gpWorldLevelData[sGridno].ubTerrainID == MED_WATER ||
      gpWorldLevelData[sGridno].ubTerrainID == LOW_WATER) {
    usMovementMode = WALKING;
  }

  // so, then we must modify it for other movement styles and accumulate
  switch (usMovementMode) {
    case RUNNING:
    case ADULTMONSTER_WALKING:
    case BLOODCAT_RUN:

      iPoints *= BP_RUN_ENERGYCOSTFACTOR;
      break;

    case SIDE_STEP:
    case WALK_BACKWARDS:
    case BLOODCAT_WALK_BACKWARDS:
    case MONSTER_WALK_BACKWARDS:
    case WALKING:
      iPoints *= BP_WALK_ENERGYCOSTFACTOR;
      break;

    case START_SWAT:
    case SWATTING:
    case SWAT_BACKWARDS:
      iPoints *= BP_SWAT_ENERGYCOSTFACTOR;
      break;
    case CRAWLING:
      iPoints *= BP_CRAWL_ENERGYCOSTFACTOR;
      break;
  }

  // ATE: Adjust these by realtime movement
  if (!(gTacticalStatus.uiFlags & TURNBASED) || !(gTacticalStatus.uiFlags & INCOMBAT)) {
    // ATE: ADJUST FOR RT - MAKE BREATH GO A LITTLE FASTER!
    iPoints = (int32_t)(iPoints * TB_BREATH_DEDUCT_MODIFIER);
  }

  return ((int16_t)iPoints);
}

int16_t ActionPointCost(const SOLDIERTYPE *const pSoldier, const int16_t sGridNo, const int8_t bDir,
                        uint16_t usMovementMode) {
  int16_t sTileCost, sPoints, sSwitchValue;

  sPoints = 0;

  // get the tile cost for that tile based on WALKING
  sTileCost = TerrainActionPoints(pSoldier, sGridNo, bDir, pSoldier->bLevel);

  // Get switch value...
  sSwitchValue = gubWorldMovementCosts[sGridNo][bDir][pSoldier->bLevel];

  // Tile cost should not be reduced based on movement mode...
  if (sSwitchValue == TRAVELCOST_FENCE) {
    return (sTileCost);
  }

  // ATE - MAKE MOVEMENT ALWAYS WALK IF IN WATER
  if (gpWorldLevelData[sGridNo].ubTerrainID == DEEP_WATER ||
      gpWorldLevelData[sGridNo].ubTerrainID == MED_WATER ||
      gpWorldLevelData[sGridNo].ubTerrainID == LOW_WATER) {
    usMovementMode = WALKING;
  }

  // so, then we must modify it for other movement styles and accumulate
  if (sTileCost > 0) {
    switch (usMovementMode) {
      case RUNNING:
      case ADULTMONSTER_WALKING:
      case BLOODCAT_RUN:
        sPoints = (int16_t)(double)((sTileCost / RUNDIVISOR));
        break;

      case CROW_FLY:
      case SIDE_STEP:
      case WALK_BACKWARDS:
      case ROBOT_WALK:
      case BLOODCAT_WALK_BACKWARDS:
      case MONSTER_WALK_BACKWARDS:
      case LARVAE_WALK:
      case WALKING:
        sPoints = (sTileCost + WALKCOST);
        break;

      case START_SWAT:
      case SWAT_BACKWARDS:
      case SWATTING:
        sPoints = (sTileCost + SWATCOST);
        break;
      case CRAWLING:
        sPoints = (sTileCost + CRAWLCOST);
        break;

      default:

        // Invalid movement mode
        DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
                 String("Invalid movement mode %d used in ActionPointCost", usMovementMode));
        sPoints = 1;
    }
  }

  if (sSwitchValue == TRAVELCOST_NOT_STANDING) {
    switch (usMovementMode) {
      case RUNNING:
      case WALKING:
      case LARVAE_WALK:
      case SIDE_STEP:
      case WALK_BACKWARDS:
        // charge crouch APs for ducking head!
        sPoints += AP_CROUCH;
        break;

      default:
        break;
    }
  }

  return (sPoints);
}

int16_t EstimateActionPointCost(SOLDIERTYPE *pSoldier, int16_t sGridNo, int8_t bDir,
                                uint16_t usMovementMode, int8_t bPathIndex, int8_t bPathLength) {
  // This action point cost code includes the penalty for having to change
  // stance after jumping a fence IF our path continues...
  int16_t sTileCost, sPoints, sSwitchValue;
  sPoints = 0;

  // get the tile cost for that tile based on WALKING
  sTileCost = TerrainActionPoints(pSoldier, sGridNo, bDir, pSoldier->bLevel);

  // so, then we must modify it for other movement styles and accumulate
  if (sTileCost > 0) {
    switch (usMovementMode) {
      case RUNNING:
      case ADULTMONSTER_WALKING:
      case BLOODCAT_RUN:
        sPoints = (int16_t)(double)((sTileCost / RUNDIVISOR));
        break;

      case CROW_FLY:
      case SIDE_STEP:
      case ROBOT_WALK:
      case WALK_BACKWARDS:
      case BLOODCAT_WALK_BACKWARDS:
      case MONSTER_WALK_BACKWARDS:
      case LARVAE_WALK:
      case WALKING:
        sPoints = (sTileCost + WALKCOST);
        break;

      case START_SWAT:
      case SWAT_BACKWARDS:
      case SWATTING:
        sPoints = (sTileCost + SWATCOST);
        break;
      case CRAWLING:
        sPoints = (sTileCost + CRAWLCOST);
        break;

      default:

        // Invalid movement mode
        DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
                 String("Invalid movement mode %d used in ActionPointCost", usMovementMode));
        sPoints = 1;
    }
  }

  // Get switch value...
  sSwitchValue = gubWorldMovementCosts[sGridNo][bDir][pSoldier->bLevel];

  // ATE: If we have a 'special cost, like jump fence...
  if (sSwitchValue == TRAVELCOST_FENCE) {
    // If we are changeing stance ( either before or after getting there....
    // We need to reflect that...
    switch (usMovementMode) {
      case SIDE_STEP:
      case WALK_BACKWARDS:
      case RUNNING:
      case WALKING:

        // Add here cost to go from crouch to stand AFTER fence hop....
        // Since it's AFTER.. make sure we will be moving after jump...
        if ((bPathIndex + 2) < bPathLength) {
          sPoints += AP_CROUCH;
        }
        break;

      case SWATTING:
      case START_SWAT:
      case SWAT_BACKWARDS:

        // Add cost to stand once there BEFORE....
        sPoints += AP_CROUCH;
        break;

      case CRAWLING:

        // Can't do it here.....
        break;
    }
  } else if (sSwitchValue == TRAVELCOST_NOT_STANDING) {
    switch (usMovementMode) {
      case RUNNING:
      case WALKING:
      case SIDE_STEP:
      case WALK_BACKWARDS:
        // charge crouch APs for ducking head!
        sPoints += AP_CROUCH;
        break;

      default:
        break;
    }
  }

  return (sPoints);
}

BOOLEAN EnoughPoints(const SOLDIERTYPE *pSoldier, int16_t sAPCost, int16_t sBPCost,
                     BOOLEAN fDisplayMsg) {
  int16_t sNewAP = 0;

  // If this guy is on a special move... don't care about APS, OR BPSs!
  if (pSoldier->ubWaitActionToDo) {
    return (TRUE);
  }

  if (pSoldier->ubQuoteActionID >= QUOTE_ACTION_ID_TRAVERSE_EAST &&
      pSoldier->ubQuoteActionID <= QUOTE_ACTION_ID_TRAVERSE_NORTH) {
    // AI guy on special move off map
    return (TRUE);
  }

  // IN realtime.. only care about BPs
  if ((gTacticalStatus.uiFlags & REALTIME) || !(gTacticalStatus.uiFlags & INCOMBAT)) {
    sAPCost = 0;
  }

  // Get New points
  sNewAP = pSoldier->bActionPoints - sAPCost;

  // If we cannot deduct points, return FALSE
  if (sNewAP < 0) {
    // Display message if it's our own guy
    if (pSoldier->bTeam == OUR_TEAM && fDisplayMsg) {
      ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, TacticalStr[NOT_ENOUGH_APS_STR]);
    }
    return (FALSE);
  }

  return (TRUE);
}

static int16_t AdjustBreathPts(SOLDIERTYPE *pSold, int16_t sBPCost);

void DeductPoints(SOLDIERTYPE *pSoldier, int16_t sAPCost, int16_t sBPCost) {
  int16_t sNewAP = 0;
  int8_t bNewBreath;

  // in real time, there IS no AP cost, (only breath cost)
  if (!(gTacticalStatus.uiFlags & TURNBASED) || !(gTacticalStatus.uiFlags & INCOMBAT)) {
    sAPCost = 0;
  }

  // Get New points
  sNewAP = pSoldier->bActionPoints - sAPCost;

  // If this is the first time with no action points, set UI flag
  if (sNewAP <= 0 && pSoldier->bActionPoints > 0) {
    fInterfacePanelDirty = DIRTYLEVEL1;
  }

  // If we cannot deduct points, return FALSE
  if (sNewAP < 0) {
    sNewAP = 0;
  }

  pSoldier->bActionPoints = (int8_t)sNewAP;

  DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
           String("Deduct Points (%d at %d) %d %d", pSoldier->ubID, pSoldier->sGridNo, sAPCost,
                  sBPCost));

  if (AM_A_ROBOT(pSoldier)) {
    // zap all breath costs for robot
    sBPCost = 0;
  }

  // is there a BREATH deduction/transaction to be made?  (REMEMBER: could be a
  // GAIN!)
  if (sBPCost) {
    // Adjust breath changes due to spending or regaining of energy
    sBPCost = AdjustBreathPts(pSoldier, sBPCost);
    sBPCost *= -1;

    pSoldier->sBreathRed -= sBPCost;

    // CJC: moved check for high breathred to below so that negative breath can
    // be detected

    // cap breathred
    if (pSoldier->sBreathRed < 0) {
      pSoldier->sBreathRed = 0;
    }
    if (pSoldier->sBreathRed > 10000) {
      pSoldier->sBreathRed = 10000;
    }

    // Get new breath
    bNewBreath = (uint8_t)(pSoldier->bBreathMax - ((float)pSoldier->sBreathRed / (float)100));

    if (bNewBreath > 100) {
      bNewBreath = 100;
    }
    if (bNewBreath < 00) {
      // Take off 1 AP per 5 breath... rem adding a negative subtracts
      pSoldier->bActionPoints += (bNewBreath / 5);
      if (pSoldier->bActionPoints < 0) {
        pSoldier->bActionPoints = 0;
      }

      bNewBreath = 0;
    }

    if (bNewBreath > pSoldier->bBreathMax) {
      bNewBreath = pSoldier->bBreathMax;
    }
    pSoldier->bBreath = bNewBreath;
  }

  // UPDATE BAR
  DirtyMercPanelInterface(pSoldier, DIRTYLEVEL1);
}

static int16_t AdjustBreathPts(SOLDIERTYPE *pSold, int16_t sBPCost) {
  int16_t sBreathFactor = 100;
  uint8_t ubBandaged;

  // NumMessage("BEFORE adjustments, BREATH PTS = ",breathPts);

  // in real time, there IS no AP cost, (only breath cost)
  /*
  if (!(gTacticalStatus.uiFlags & TURNBASED) || !(gTacticalStatus.uiFlags &
  INCOMBAT ) )
  {
          // ATE: ADJUST FOR RT - MAKE BREATH GO A LITTLE FASTER!
          sBPCost	*= TB_BREATH_DEDUCT_MODIFIER;
  }
  */

  // adjust breath factor for current breath deficiency
  sBreathFactor += (100 - pSold->bBreath);

  // adjust breath factor for current life deficiency (but add 1/2 bandaging)
  ubBandaged = pSold->bLifeMax - pSold->bLife - pSold->bBleeding;
  // sBreathFactor += (pSold->bLifeMax - (pSold->bLife + (ubBandaged / 2)));
  sBreathFactor += 100 * (pSold->bLifeMax - (pSold->bLife + (ubBandaged / 2))) / pSold->bLifeMax;

  if (pSold->bStrength > 80) {
    // give % reduction to breath costs for high strength mercs
    sBreathFactor -= (pSold->bStrength - 80) / 2;
  }

  // if a non-swimmer type is thrashing around in deep water
  if ((pSold->ubProfile != NO_PROFILE) &&
      (gMercProfiles[pSold->ubProfile].bPersonalityTrait == NONSWIMMER)) {
    if (pSold->usAnimState == DEEP_WATER_TRED || pSold->usAnimState == DEEP_WATER_SWIM) {
      sBreathFactor *= 5;  // lose breath 5 times faster in deep water!
    }
  }

  if (sBreathFactor == 0) {
    sBPCost = 0;
  } else if (sBPCost > 0)  // breath DECREASE
    // increase breath COST by breathFactor
    sBPCost = ((sBPCost * sBreathFactor) / 100);
  else  // breath INCREASE
    // decrease breath GAIN by breathFactor
    sBPCost = ((sBPCost * 100) / sBreathFactor);

  return (sBPCost);
}

static int16_t GetBreathPerAP(SOLDIERTYPE *pSoldier, uint16_t usAnimState);

void UnusedAPsToBreath(SOLDIERTYPE *pSold) {
  int16_t sUnusedAPs, sBreathPerAP = 0, sBreathChange, sRTBreathMod;

  // Note to Andrew (or whomever else it may concern):

  // This function deals with BETWEEN TURN breath/energy gains. The basic
  // concept is:
  //
  //	- look at LAST (current) animation of merc to see what he's now doing
  //	- look at how many AP remain unspent (indicating duration of time doing
  // that anim)
  //
  //  figure out how much breath/energy (if any) he should recover. Obviously if
  //  a merc
  //	is STANDING BREATHING and hasn't spent any AP then it means he *stood
  // around* for
  //  the entire duration of one turn (which, instead of spending energy,
  //  REGAINS energy)

  // COMMENTED OUT FOR NOW SINCE MOST OF THE ANIMATION DEFINES DO NOT MATCH

  // If we are not in turn-based combat...

  if (pSold->uiStatusFlags & SOLDIER_VEHICLE) {
    return;
  }

  if (!(gTacticalStatus.uiFlags & TURNBASED) || !(gTacticalStatus.uiFlags & INCOMBAT)) {
    // ALRIGHT, GIVE A FULL AMOUNT BACK, UNLES MODIFIED BY WHAT ACTIONS WE WERE
    // DOING
    sBreathPerAP = GetBreathPerAP(pSold, pSold->usAnimState);

    // adjust for carried weight
    sBreathPerAP = sBreathPerAP * 100 / BreathPointAdjustmentForCarriedWeight(pSold);

    // If this value is -ve, we have a gain, else we have a loos which we should
    // not really do We just want to limit this to no gain if we were doing
    // stuff...
    sBreathChange = 3 * sBreathPerAP;

    // Adjust for on drugs
    HandleBPEffectDueToDrugs(pSold, &sBreathChange);

    if (sBreathChange > 0) {
      sBreathChange = 0;
    } else {
      // We have a gain, now limit this depending on what we were doing...
      // OK for RT, look at how many tiles we have moved, our last move anim
      if (pSold->ubTilesMovedPerRTBreathUpdate > 0) {
        // How long have we done this for?
        // And what anim were we doing?
        sBreathPerAP = GetBreathPerAP(pSold, pSold->usLastMovementAnimPerRTBreathUpdate);

        sRTBreathMod = sBreathPerAP * pSold->ubTilesMovedPerRTBreathUpdate;

        // Deduct some if we were exerting ourselves
        // We add here because to gain breath, sBreathChange needs to be -ve
        if (sRTBreathMod > 0) {
          sBreathChange += sRTBreathMod;
        }

        if (sBreathChange < 0) {
          sBreathChange = 0;
        }
      }
    }

    // Divide by a number to adjust that in realtimer we do not want to recover
    // as as fast as the TB values do
    sBreathChange *= TB_BREATH_RECOVER_MODIFIER;

    // adjust breath only, don't touch action points!
    DeductPoints(pSold, 0, (int16_t)sBreathChange);

    // Reset value for RT breath update
    pSold->ubTilesMovedPerRTBreathUpdate = 0;

  } else {
    // if merc has any APs left unused this turn (that aren't carrying over)
    if (pSold->bActionPoints > MAX_AP_CARRIED) {
      sUnusedAPs = pSold->bActionPoints - MAX_AP_CARRIED;

      sBreathPerAP = GetBreathPerAP(pSold, pSold->usAnimState);

      if (sBreathPerAP < 0) {
        // can't gain any breath when we've just been gassed, OR
        // if standing in tear gas without a gas mask on
        if (pSold->uiStatusFlags & SOLDIER_GASSED) {
          return;  // can't breathe here, so get no breath back!
        }
      }

      // adjust for carried weight
      sBreathPerAP = sBreathPerAP * 100 / BreathPointAdjustmentForCarriedWeight(pSold);

      sBreathChange = sUnusedAPs * sBreathPerAP;
    } else {
      sBreathChange = 0;
    }
    // Adjust for on drugs
    HandleBPEffectDueToDrugs(pSold, &sBreathChange);

    // adjust breath only, don't touch action points!
    DeductPoints(pSold, 0, (int16_t)sBreathChange);
  }
}

static int16_t GetBreathPerAP(SOLDIERTYPE *pSoldier, uint16_t usAnimState) {
  int16_t sBreathPerAP = 0;
  BOOLEAN fAnimTypeFound = FALSE;

  if (gAnimControl[usAnimState].uiFlags & ANIM_VARIABLE_EFFORT) {
    // Default effort
    sBreathPerAP = BP_PER_AP_MIN_EFFORT;

    // OK, check if we are in water and are waling/standing
    if (MercInWater(pSoldier)) {
      switch (usAnimState) {
        case STANDING:

          sBreathPerAP = BP_PER_AP_LT_EFFORT;
          break;

        case WALKING:

          sBreathPerAP = BP_PER_AP_MOD_EFFORT;
          break;
      }
    } else {
      switch (usAnimState) {
        case STANDING:

          sBreathPerAP = BP_PER_AP_NO_EFFORT;
          break;

        case WALKING:

          sBreathPerAP = BP_PER_AP_LT_EFFORT;
          break;
      }
    }
    fAnimTypeFound = TRUE;
  }

  if (gAnimControl[usAnimState].uiFlags & ANIM_NO_EFFORT) {
    sBreathPerAP = BP_PER_AP_NO_EFFORT;
    fAnimTypeFound = TRUE;
  }

  if (gAnimControl[usAnimState].uiFlags & ANIM_MIN_EFFORT) {
    sBreathPerAP = BP_PER_AP_MIN_EFFORT;
    fAnimTypeFound = TRUE;
  }

  if (gAnimControl[usAnimState].uiFlags & ANIM_LIGHT_EFFORT) {
    sBreathPerAP = BP_PER_AP_LT_EFFORT;
    fAnimTypeFound = TRUE;
  }

  if (gAnimControl[usAnimState].uiFlags & ANIM_MODERATE_EFFORT) {
    sBreathPerAP = BP_PER_AP_MOD_EFFORT;
    fAnimTypeFound = TRUE;
  }

  if (!fAnimTypeFound) {
    DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
             String("Unknown end-of-turn breath anim: %hs", gAnimControl[usAnimState].zAnimStr));
  }

  return (sBreathPerAP);
}

uint8_t CalcAPsToBurst(int8_t const bBaseActionPoints, OBJECTTYPE const &o) {
  // base APs is what you'd get from CalcActionPoints();
  if (o.usItem == G11) {
    return (1);
  } else {
    // NB round UP, so 21-25 APs pay full
    int8_t const bAttachPos = FindAttachment(&o, SPRING_AND_BOLT_UPGRADE);
    if (bAttachPos != -1) {
      return (std::max(3, (AP_BURST * bBaseActionPoints + (AP_MAXIMUM - 1)) / AP_MAXIMUM) * 100) /
             (100 + o.bAttachStatus[bAttachPos] / 5);
    } else {
      return std::max(3, (AP_BURST * bBaseActionPoints + (AP_MAXIMUM - 1)) / AP_MAXIMUM);
    }
  }
}

uint8_t CalcTotalAPsToAttack(SOLDIERTYPE *const s, int16_t const grid_no,
                             uint8_t const ubAddTurningCost, int8_t const aim_time) {
  uint16_t ap_cost = 0;
  OBJECTTYPE const &in_hand = s->inv[HANDPOS];
  switch (Item[in_hand.usItem].usItemClass) {
    case IC_GUN:
    case IC_LAUNCHER:
    case IC_TENTACLES:
    case IC_THROWING_KNIFE:
      ap_cost = MinAPsToAttack(s, grid_no, ubAddTurningCost);
      ap_cost += s->bDoBurst ? CalcAPsToBurst(CalcActionPoints(s), in_hand) : aim_time;
      break;

    case IC_GRENADE:
    case IC_BOMB:
#if 0  // XXX always was this way
			ap_cost = MinAPsToAttack(s, grid_no, ubAddTurningCost);
#else
      ap_cost = 5;
#endif
      break;

    case IC_PUNCH:
    case IC_BLADE:
      /* If we are at this gridno, calc min APs but if not, calc cost to goto this
       * location */
      int16_t adjusted_grid_no;
      if (s->sGridNo == grid_no) {
        adjusted_grid_no = grid_no;
      } else {
        if (s->sWalkToAttackGridNo == grid_no) { /* In order to avoid path calculations here all the
                                                  * time, save and check if it's changed! */
          adjusted_grid_no = grid_no;
        } else {
          GridNo got_location = NOWHERE;

          if (SOLDIERTYPE const *const tgt = WhoIsThere2(grid_no, s->bLevel)) {
            if (s->ubBodyType == BLOODCAT) {
              got_location =
                  FindNextToAdjacentGridEx(s, grid_no, 0, &adjusted_grid_no, TRUE, FALSE);
              if (got_location == -1) got_location = NOWHERE;
            } else {
              got_location = FindAdjacentPunchTarget(s, tgt, &adjusted_grid_no);
            }
          }

          bool got_adjacent = false;
          if (got_location == NOWHERE && s->ubBodyType != BLOODCAT) {
            got_location = FindAdjacentGridEx(s, grid_no, 0, &adjusted_grid_no, TRUE, FALSE);
            if (got_location == -1) got_location = NOWHERE;
            got_adjacent = true;
          }

          if (got_location == NOWHERE) return 0;

          if (s->sGridNo == got_location || !got_adjacent) {
            s->sWalkToAttackWalkToCost = 0;
          } else {
            // Save for next time
            s->sWalkToAttackWalkToCost = PlotPath(s, got_location, NO_COPYROUTE, NO_PLOT,
                                                  s->usUIMovementMode, s->bActionPoints);
            if (s->sWalkToAttackWalkToCost == 0) return 99;
          }

          // Save old location!
          s->sWalkToAttackGridNo = grid_no;
        }
        ap_cost += s->sWalkToAttackWalkToCost;
      }
      ap_cost += MinAPsToAttack(s, adjusted_grid_no, ubAddTurningCost);
      ap_cost += aim_time;
      break;
  }

  return ap_cost;
}

static uint8_t MinAPsToPunch(SOLDIERTYPE const &, GridNo, bool add_turning_cost);

uint8_t MinAPsToAttack(SOLDIERTYPE *const s, GridNo const grid_no, uint8_t const add_turning_cost) {
  OBJECTTYPE const &in_hand = s->inv[HANDPOS];
  uint16_t item = in_hand.usItem;
  if (s->bWeaponMode == WM_ATTACHED) {  // Look for an attached grenade launcher
    int8_t const attach_slot = FindAttachment(&in_hand, UNDER_GLAUNCHER);
    if (attach_slot != NO_SLOT) item = UNDER_GLAUNCHER;
  }

  switch (Item[item].usItemClass) {
    case IC_BLADE:
    case IC_GUN:
    case IC_LAUNCHER:
    case IC_TENTACLES:
    case IC_THROWING_KNIFE:
      return MinAPsToShootOrStab(*s, grid_no, add_turning_cost);
    case IC_GRENADE:
    case IC_THROWN:
      return MinAPsToThrow(*s, grid_no, add_turning_cost);
    case IC_PUNCH:
      return MinAPsToPunch(*s, grid_no, add_turning_cost);
    default:
      return 0;
  }
}

static int8_t CalcAimSkill(SOLDIERTYPE const &s, uint16_t const weapon) {
  switch (Item[weapon].usItemClass) {
    case IC_GUN:
    case IC_LAUNCHER:
      // Guns: Modify aiming cost by shooter's marksmanship
      return EffectiveMarksmanship(&s);

    default:
      // Knives: Modify aiming cost by avg of attacker's dexterity & agility
      return (EffectiveDexterity(&s) + EffectiveAgility(&s)) / 2;
  }
}

uint8_t BaseAPsToShootOrStab(int8_t const bAPs, int8_t const bAimSkill, OBJECTTYPE const &o) {
  int16_t sTop, sBottom;

  // Calculate default top & bottom of the magic "aiming" formula!

  // get this man's maximum possible action points (ignoring carryovers)
  // the 2 times is here only to allow rounding off using integer math later
  sTop = 2 * bAPs;  // CalcActionPoints( pSoldier );

  // Shots per turn rating is for max. aimSkill(100), drops down to 1/2 at = 0
  // DIVIDE BY 4 AT THE END HERE BECAUSE THE SHOTS PER TURN IS NOW QUADRUPLED!
  // NB need to define shots per turn for ALL Weapons then.
  sBottom = ((50 + (bAimSkill / 2)) * Weapon[o.usItem].ubShotsPer4Turns) / 4;

  int8_t const bAttachPos = FindAttachment(&o, SPRING_AND_BOLT_UPGRADE);
  if (bAttachPos != -1) {
    sBottom = sBottom * (100 + o.bAttachStatus[bAttachPos] / 5) / 100;
  }

  // add minimum aiming time to the overall minimum AP_cost
  //     This here ROUNDS UP fractions of 0.5 or higher using integer math
  //     This works because 'top' is 2x what it really should be throughout
  return ((((100 * sTop) / sBottom) + 1) / 2);
}

void GetAPChargeForShootOrStabWRTGunRaises(SOLDIERTYPE const *const s, GridNo grid_no,
                                           uint8_t const ubAddTurningCost,
                                           BOOLEAN *const charge_turning,
                                           BOOLEAN *const charge_raise) {
  bool adding_turning_cost = FALSE;
  if (ubAddTurningCost) {
    if (grid_no != NOWHERE) {  // Get direction and see if we need to turn
      // Given a gridno here, check if we are on a guy - if so - get his gridno
      SOLDIERTYPE const *const tgt = FindSoldier(grid_no, FIND_SOLDIER_GRIDNO);
      if (tgt) grid_no = tgt->sGridNo;

      // Is it the same as he's facing?
      uint8_t const direction = GetDirectionFromGridNo(grid_no, s);
      adding_turning_cost = direction != s->bDirection;
    } else {  // Assume we need to add cost!
      adding_turning_cost = true;
    }
  }
  *charge_turning = adding_turning_cost;

  // Do we need to ready weapon?
  *charge_raise = Item[s->inv[HANDPOS].usItem].usItemClass != IC_THROWING_KNIFE &&
                  !(gAnimControl[s->usAnimState].uiFlags & (ANIM_FIREREADY | ANIM_FIRE));
}

uint8_t MinAPsToShootOrStab(SOLDIERTYPE &s, GridNo gridno, bool const add_turning_cost) {
  OBJECTTYPE const &in_hand = s.inv[HANDPOS];
  uint16_t const item = s.bWeaponMode == WM_ATTACHED ? UNDER_GLAUNCHER : in_hand.usItem;

  BOOLEAN adding_turning_cost;
  BOOLEAN adding_raise_gun_cost;
  GetAPChargeForShootOrStabWRTGunRaises(&s, gridno, add_turning_cost, &adding_turning_cost,
                                        &adding_raise_gun_cost);

  uint8_t ap_cost = AP_MIN_AIM_ATTACK;

  if (Item[item].usItemClass == IC_THROWING_KNIFE ||
      item == ROCKET_LAUNCHER) {  // Do we need to stand up?
    ap_cost += GetAPsToChangeStance(&s, ANIM_STAND);
  }

  // ATE: Look at stance
  if (gAnimControl[s.usAnimState].ubHeight == ANIM_STAND) {  // Don't charge turning if gun-ready
    if (adding_raise_gun_cost) adding_turning_cost = FALSE;
  } else {  // Just charge turning costs
    if (adding_turning_cost) adding_raise_gun_cost = FALSE;
  }

  if (AM_A_ROBOT(&s)) adding_raise_gun_cost = FALSE;

  if (adding_turning_cost) {
    if (Item[item].usItemClass == IC_THROWING_KNIFE) {
      ap_cost += AP_LOOK_STANDING;
    } else {
      ap_cost += GetAPsToLook(&s);
    }
  }

  if (adding_raise_gun_cost) {
    ap_cost += GetAPsToReadyWeapon(&s, s.usAnimState);
    s.fDontChargeReadyAPs = FALSE;
  }

  if (gridno != NOWHERE) {  // Given a gridno here, check if we are on a guy - if
                            // so - get his gridno
    SOLDIERTYPE const *const tgt = FindSoldier(gridno, FIND_SOLDIER_GRIDNO);
    if (tgt) gridno = tgt->sGridNo;
  }

  // if attacking a new target (or if the specific target is uncertain)
  if (gridno != s.sLastTarget && item != ROCKET_LAUNCHER) {
    ap_cost += AP_CHANGE_TARGET;
  }

  int8_t const full_aps = CalcActionPoints(&s);
  // Aim skill is the same whether we are using 1 or 2 guns
  int8_t const aim_skill = CalcAimSkill(s, item);

  if (s.bWeaponMode == WM_ATTACHED) {
    // Create temporary grenade launcher and use that
    int8_t const attach_slot = FindAttachment(&in_hand, UNDER_GLAUNCHER);
    int8_t const status = attach_slot != NO_SLOT ? in_hand.bAttachStatus[attach_slot]
                                                 : 100;  // Fake it, use a 100 status
    OBJECTTYPE grenade_launcher;
    CreateItem(UNDER_GLAUNCHER, status, &grenade_launcher);

    ap_cost += BaseAPsToShootOrStab(full_aps, aim_skill, grenade_launcher);
  } else if (IsValidSecondHandShot(&s)) {  // Charge the maximum of the two
    uint8_t const ap_1st = BaseAPsToShootOrStab(full_aps, aim_skill, in_hand);
    uint8_t const ap_2nd = BaseAPsToShootOrStab(full_aps, aim_skill, s.inv[SECONDHANDPOS]);
    ap_cost += std::max(ap_1st, ap_2nd);
  } else {
    ap_cost += BaseAPsToShootOrStab(full_aps, aim_skill, in_hand);
  }

  // The minimum AP cost of ANY shot can NEVER be more than merc's maximum APs
  if (ap_cost > full_aps) ap_cost = full_aps;

  // this SHOULD be impossible, but nevertheless
  if (ap_cost < 1) ap_cost = 1;

  return ap_cost;
}

static uint8_t MinAPsToPunch(SOLDIERTYPE const &s, GridNo gridno, bool const add_turning_cost) {
  uint8_t ap = 4;

  if (gridno == NOWHERE) return ap;

  if (SOLDIERTYPE const *const tgt =
          WhoIsThere2(gridno, s.bTargetLevel)) {  // On a guy, get his gridno
    gridno = tgt->sGridNo;

    // Check if target is prone, if so, calc cost
    if (gAnimControl[tgt->usAnimState].ubEndHeight == ANIM_PRONE) {
      ap += GetAPsToChangeStance(&s, ANIM_CROUCH);
    } else if (s.sGridNo == gridno) {
      ap += GetAPsToChangeStance(&s, ANIM_STAND);
    }
  }

  if (add_turning_cost && s.sGridNo == gridno) {  // Is it the same as he's facing?
    uint8_t const direction = GetDirectionFromGridNo(gridno, &s);
    if (direction != s.bDirection) ap += AP_LOOK_STANDING;  // ATE: Use standing turn cost
  }

  return ap;
}

int8_t MinPtsToMove(const SOLDIERTYPE *const pSoldier) {
  // look around all 8 directions and return lowest terrain cost
  int32_t cnt;
  int16_t sLowest = 127;
  int16_t sGridno, sCost;

  if (TANK(pSoldier)) {
    return ((int8_t)sLowest);
  }

  for (cnt = 0; cnt <= 7; cnt++) {
    sGridno = NewGridNo(pSoldier->sGridNo, DirectionInc((int16_t)cnt));
    if (sGridno != pSoldier->sGridNo) {
      if ((sCost = ActionPointCost(pSoldier, sGridno, (uint8_t)cnt, pSoldier->usUIMovementMode)) <
          sLowest) {
        sLowest = sCost;
      }
    }
  }
  return ((int8_t)sLowest);
}

int8_t PtsToMoveDirection(const SOLDIERTYPE *const pSoldier, const int8_t bDirection) {
  int16_t sGridno, sCost;
  int8_t bOverTerrainType;
  uint16_t usMoveModeToUse;

  sGridno = NewGridNo(pSoldier->sGridNo, DirectionInc((int16_t)bDirection));

  usMoveModeToUse = pSoldier->usUIMovementMode;

  // ATE: Check if the new place is watter and we were tying to run....
  bOverTerrainType = GetTerrainType(sGridno);

  if (bOverTerrainType == MED_WATER || bOverTerrainType == DEEP_WATER ||
      bOverTerrainType == LOW_WATER) {
    usMoveModeToUse = WALKING;
  }

  sCost = ActionPointCost(pSoldier, sGridno, bDirection, usMoveModeToUse);

  if (gubWorldMovementCosts[sGridno][bDirection][pSoldier->bLevel] != TRAVELCOST_FENCE) {
    if (usMoveModeToUse == RUNNING && pSoldier->usAnimState != RUNNING) {
      sCost += AP_START_RUN_COST;
    }
  }

  return ((int8_t)sCost);
}

int8_t MinAPsToStartMovement(const SOLDIERTYPE *pSoldier, uint16_t usMovementMode) {
  int8_t bAPs = 0;

  switch (usMovementMode) {
    case RUNNING:
    case WALKING:
      if (gAnimControl[pSoldier->usAnimState].ubEndHeight == ANIM_PRONE) {
        bAPs += AP_CROUCH + AP_PRONE;
      } else if (gAnimControl[pSoldier->usAnimState].ubEndHeight == ANIM_CROUCH) {
        bAPs += AP_CROUCH;
      }
      break;
    case SWATTING:
      if (gAnimControl[pSoldier->usAnimState].ubEndHeight == ANIM_PRONE) {
        bAPs += AP_PRONE;
      } else if (gAnimControl[pSoldier->usAnimState].ubEndHeight == ANIM_STAND) {
        bAPs += AP_CROUCH;
      }
      break;
    case CRAWLING:
      if (gAnimControl[pSoldier->usAnimState].ubEndHeight == ANIM_STAND) {
        bAPs += AP_CROUCH + AP_PRONE;
      } else if (gAnimControl[pSoldier->usAnimState].ubEndHeight == ANIM_CROUCH) {
        bAPs += AP_CROUCH;
      }
      break;
    default:
      break;
  }

  if (usMovementMode == RUNNING && pSoldier->usAnimState != RUNNING) {
    bAPs += AP_START_RUN_COST;
  }
  return (bAPs);
}

BOOLEAN EnoughAmmo(SOLDIERTYPE *const s, BOOLEAN const fDisplay, int8_t const inv_pos) {
  OBJECTTYPE const &o = s->inv[inv_pos];
  uint16_t const item_idx = o.usItem;
  if (item_idx == NOTHING) return FALSE;

  if (s->bWeaponMode == WM_ATTACHED) return TRUE;

  // hack... they turn empty afterwards anyways
  if (item_idx == ROCKET_LAUNCHER) return TRUE;

  INVTYPE const &item = Item[item_idx];
  if (item.usItemClass == IC_LAUNCHER || item_idx == TANK_CANNON) {
    if (FindAttachmentByClass(&o, IC_GRENADE) != ITEM_NOT_FOUND) return TRUE;
    if (FindAttachmentByClass(&o, IC_BOMB) != ITEM_NOT_FOUND) return TRUE;
  } else if (item.usItemClass == IC_GUN) {
    if (o.ubGunShotsLeft != 0) return TRUE;
  } else {
    return TRUE;
  }

  if (fDisplay) TacticalCharacterDialogue(s, QUOTE_OUT_OF_AMMO);
  return FALSE;
}

void DeductAmmo(SOLDIERTYPE *pSoldier, int8_t bInvPos) {
  OBJECTTYPE *pObj;

  // tanks never run out of MG ammo!
  // unlimited cannon ammo is handled in AI
  if (TANK(pSoldier) && pSoldier->inv[bInvPos].usItem != TANK_CANNON) {
    return;
  }

  pObj = &(pSoldier->inv[bInvPos]);
  if (pObj->usItem != NOTHING) {
    if (pObj->usItem == TANK_CANNON) {
    } else if (Item[pObj->usItem].usItemClass == IC_GUN && pObj->usItem != TANK_CANNON) {
      if (pSoldier->usAttackingWeapon == pObj->usItem) {
        // OK, let's see, don't overrun...
        if (pObj->ubGunShotsLeft != 0) {
          pObj->ubGunShotsLeft--;
        }
      } else {
        // firing an attachment?
      }
    } else if (Item[pObj->usItem].usItemClass == IC_LAUNCHER || pObj->usItem == TANK_CANNON) {
      int8_t bAttachPos;

      bAttachPos = FindAttachmentByClass(pObj, IC_GRENADE);
      if (bAttachPos == ITEM_NOT_FOUND) {
        bAttachPos = FindAttachmentByClass(pObj, IC_BOMB);
      }

      if (bAttachPos != ITEM_NOT_FOUND) {
        RemoveAttachment(pObj, bAttachPos, NULL);
      }
    }

    // Dirty Bars
    DirtyMercPanelInterface(pSoldier, DIRTYLEVEL1);
  }
}

static int16_t GetMovePlusActionAPCosts(SOLDIERTYPE *const s, GridNo const pos,
                                        int16_t const action_ap) {
  if (s->sGridNo == pos) return action_ap;
  int16_t const move_ap =
      PlotPath(s, pos, NO_COPYROUTE, NO_PLOT, s->usUIMovementMode, s->bActionPoints);
  if (move_ap == 0) return 0;  // Destination unreachable
  return move_ap + action_ap;
}

uint16_t GetAPsToPickupItem(SOLDIERTYPE *const s, uint16_t const usMapPos) {
  // Check if we are over an item pool
  if (!GetItemPool(usMapPos, s->bLevel)) return 0;
  int16_t const sActionGridNo = AdjustGridNoForItemPlacement(s, usMapPos);
  return GetMovePlusActionAPCosts(s, sActionGridNo, AP_PICKUP_ITEM);
}

uint16_t GetAPsToGiveItem(SOLDIERTYPE *const s, uint16_t const usMapPos) {
  return GetMovePlusActionAPCosts(s, usMapPos, AP_GIVE_ITEM);
}

int8_t GetAPsToReloadGunWithAmmo(OBJECTTYPE *pGun, OBJECTTYPE *pAmmo) {
  if (Item[pGun->usItem].usItemClass == IC_LAUNCHER) {
    // always standard AP cost
    return (AP_RELOAD_GUN);
  }
  if (Weapon[pGun->usItem].ubMagSize == Magazine[Item[pAmmo->usItem].ubClassIndex].ubMagSize) {
    // normal situation
    return (AP_RELOAD_GUN);
  } else {
    // trying to reload with wrong size of magazine
    return (AP_RELOAD_GUN + AP_RELOAD_GUN);
  }
}

int8_t GetAPsToAutoReload(SOLDIERTYPE *pSoldier) {
  OBJECTTYPE *pObj;
  int8_t bSlot, bSlot2, bExcludeSlot;
  int8_t bAPCost = 0;
  int8_t bAPCost2 = 0;

  CHECKF(pSoldier);
  pObj = &(pSoldier->inv[HANDPOS]);

  if (Item[pObj->usItem].usItemClass == IC_GUN || Item[pObj->usItem].usItemClass == IC_LAUNCHER) {
    bSlot = FindAmmoToReload(pSoldier, HANDPOS, NO_SLOT);
    if (bSlot != NO_SLOT) {
      // we would reload using this ammo!
      bAPCost += GetAPsToReloadGunWithAmmo(pObj, &(pSoldier->inv[bSlot]));
    }

    if (IsValidSecondHandShotForReloadingPurposes(pSoldier)) {
      pObj = &(pSoldier->inv[SECONDHANDPOS]);
      bExcludeSlot = NO_SLOT;
      bSlot2 = NO_SLOT;

      // if the ammo for the first gun is the same we have to do special checks
      if (ValidAmmoType(pObj->usItem, pSoldier->inv[bSlot].usItem)) {
        if (pSoldier->inv[bSlot].ubNumberOfObjects == 1) {
          // we must not consider this slot for reloading!
          bExcludeSlot = bSlot;
        } else {
          // we can reload the 2nd gun from the same pocket!
          bSlot2 = bSlot;
        }
      }

      if (bSlot2 == NO_SLOT) {
        bSlot2 = FindAmmoToReload(pSoldier, SECONDHANDPOS, bExcludeSlot);
      }

      if (bSlot2 != NO_SLOT) {
        // we would reload using this ammo!
        bAPCost2 = GetAPsToReloadGunWithAmmo(pObj, &(pSoldier->inv[bSlot2]));
        if (EnoughPoints(pSoldier, (int16_t)(bAPCost + bAPCost2), 0, FALSE)) {
          // we can afford to reload both guns; otherwise display just for 1 gun
          bAPCost += bAPCost2;
        }
      }
    }
  }

  return (bAPCost);
}

uint16_t GetAPsToReloadRobot(SOLDIERTYPE *const s, SOLDIERTYPE const *const robot) {
  GridNo const sActionGridNo = FindAdjacentGridEx(s, robot->sGridNo, NULL, NULL, TRUE, FALSE);
  return GetMovePlusActionAPCosts(s, sActionGridNo, 4);
}

uint16_t GetAPsToChangeStance(const SOLDIERTYPE *pSoldier, int8_t bDesiredHeight) {
  uint16_t sAPCost = 0;
  int8_t bCurrentHeight;

  bCurrentHeight = gAnimControl[pSoldier->usAnimState].ubEndHeight;

  if (bCurrentHeight == bDesiredHeight) {
    sAPCost = 0;
  }

  if (bCurrentHeight == ANIM_STAND && bDesiredHeight == ANIM_PRONE) {
    sAPCost = AP_CROUCH + AP_PRONE;
  }
  if (bCurrentHeight == ANIM_STAND && bDesiredHeight == ANIM_CROUCH) {
    sAPCost = AP_CROUCH;
  }
  if (bCurrentHeight == ANIM_CROUCH && bDesiredHeight == ANIM_PRONE) {
    sAPCost = AP_PRONE;
  }
  if (bCurrentHeight == ANIM_CROUCH && bDesiredHeight == ANIM_STAND) {
    sAPCost = AP_CROUCH;
  }
  if (bCurrentHeight == ANIM_PRONE && bDesiredHeight == ANIM_STAND) {
    sAPCost = AP_PRONE + AP_CROUCH;
  }
  if (bCurrentHeight == ANIM_PRONE && bDesiredHeight == ANIM_CROUCH) {
    sAPCost = AP_PRONE;
  }

  return (sAPCost);
}

static uint16_t GetBPsToChangeStance(SOLDIERTYPE *pSoldier, int8_t bDesiredHeight) {
  uint16_t sBPCost = 0;
  int8_t bCurrentHeight;

  bCurrentHeight = gAnimControl[pSoldier->usAnimState].ubEndHeight;

  if (bCurrentHeight == bDesiredHeight) {
    sBPCost = 0;
  }

  if (bCurrentHeight == ANIM_STAND && bDesiredHeight == ANIM_PRONE) {
    sBPCost = BP_CROUCH + BP_PRONE;
  }
  if (bCurrentHeight == ANIM_STAND && bDesiredHeight == ANIM_CROUCH) {
    sBPCost = BP_CROUCH;
  }
  if (bCurrentHeight == ANIM_CROUCH && bDesiredHeight == ANIM_PRONE) {
    sBPCost = BP_PRONE;
  }
  if (bCurrentHeight == ANIM_CROUCH && bDesiredHeight == ANIM_STAND) {
    sBPCost = BP_CROUCH;
  }
  if (bCurrentHeight == ANIM_PRONE && bDesiredHeight == ANIM_STAND) {
    sBPCost = BP_PRONE + BP_CROUCH;
  }
  if (bCurrentHeight == ANIM_PRONE && bDesiredHeight == ANIM_CROUCH) {
    sBPCost = BP_PRONE;
  }

  return (sBPCost);
}

uint16_t GetAPsToLook(const SOLDIERTYPE *pSoldier) {
  // Set # of APs
  switch (gAnimControl[pSoldier->usAnimState].ubEndHeight) {
    // Now change to appropriate animation
    case ANIM_STAND:
      return AP_LOOK_STANDING;
    case ANIM_CROUCH:
      return AP_LOOK_CROUCHED;

    case ANIM_PRONE:
      // AP_PRONE is the AP cost to go to or from the prone stance.  To turn while
      // prone, your merc has to get up to crouched, turn, and then go back down.
      // Hence you go up (AP_PRONE), turn (AP_LOOK_PRONE) and down (AP_PRONE).
      return (AP_LOOK_PRONE + AP_PRONE + AP_PRONE);

    // no other values should be possible
    default:
      Assert(FALSE);
      return (0);
  }
}

BOOLEAN CheckForMercContMove(SOLDIERTYPE *const s) {
  if (!(gTacticalStatus.uiFlags & INCOMBAT)) return FALSE;

  if (gpItemPointer != NULL) return FALSE;

  if (s->bLife < OKLIFE) return FALSE;

  if (s->sGridNo == s->sFinalDestination && !s->bGoodContPath) return FALSE;

  if (s != GetSelectedMan()) return FALSE;

  if (!SoldierOnScreen(s)) return FALSE;

  const int16_t sGridNo = (s->bGoodContPath ? s->sContPathLocation : s->sFinalDestination);
  if (!FindBestPath(s, sGridNo, s->bLevel, s->usUIMovementMode, NO_COPYROUTE, 0)) return FALSE;

  const int16_t sAPCost = PtsToMoveDirection(s, guiPathingData[0]);
  if (!EnoughPoints(s, sAPCost, 0, FALSE)) return FALSE;

  return TRUE;
}

int16_t GetAPsToReadyWeapon(const SOLDIERTYPE *const pSoldier, const uint16_t usAnimState) {
  uint16_t usItem;

  // If this is a dwel pistol anim
  // ATE: What was I thinking, hooking into animations like this....
  // if ( usAnimState == READY_DUAL_STAND || usAnimState == READY_DUAL_CROUCH )
  //{
  // return( AP_READY_DUAL );
  //}
  if (IsValidSecondHandShot(pSoldier)) {
    return (AP_READY_DUAL);
  }

  // OK, now check type of weapon
  usItem = pSoldier->inv[HANDPOS].usItem;

  if (usItem == NOTHING) {
    return (0);
  } else {
    // CHECK FOR RIFLE
    if (Item[usItem].usItemClass == IC_GUN) {
      return (Weapon[usItem].ubReadyTime);
    }
  }

  return (0);
}

int8_t GetAPsToClimbRoof(SOLDIERTYPE *pSoldier, BOOLEAN fClimbDown) {
  if (!fClimbDown) {
    // OK, add aps to goto stand stance...
    return ((int8_t)(AP_CLIMBROOF + GetAPsToChangeStance(pSoldier, ANIM_STAND)));
  } else {
    // Add aps to goto crouch
    return ((int8_t)(AP_CLIMBOFFROOF + GetAPsToChangeStance(pSoldier, ANIM_CROUCH)));
  }
}

static int16_t GetBPsToClimbRoof(SOLDIERTYPE *pSoldier, BOOLEAN fClimbDown) {
  if (!fClimbDown) {
    return (BP_CLIMBROOF);
  } else {
    return (BP_CLIMBOFFROOF);
  }
}

int8_t GetAPsToCutFence(SOLDIERTYPE *pSoldier) {
  // OK, it's normally just cost, but add some if different stance...
  return (GetAPsToChangeStance(pSoldier, ANIM_CROUCH) + AP_USEWIRECUTTERS);
}

int8_t GetAPsToBeginFirstAid(SOLDIERTYPE *pSoldier) {
  // OK, it's normally just cost, but add some if different stance...
  return (GetAPsToChangeStance(pSoldier, ANIM_CROUCH) + AP_START_FIRST_AID);
}

int8_t GetAPsToBeginRepair(SOLDIERTYPE *pSoldier) {
  // OK, it's normally just cost, but add some if different stance...
  return (GetAPsToChangeStance(pSoldier, ANIM_CROUCH) + AP_START_REPAIR);
}

int8_t GetAPsToRefuelVehicle(SOLDIERTYPE *pSoldier) {
  // OK, it's normally just cost, but add some if different stance...
  return (GetAPsToChangeStance(pSoldier, ANIM_CROUCH) + AP_REFUEL_VEHICLE);
}

#define TOSSES_PER_10TURNS 18  // max # of grenades tossable in 10 turns
#define AP_MIN_AIM_ATTACK 0    // minimum permitted extra aiming
#define AP_MAX_AIM_ATTACK 4    // maximum permitted extra aiming

int16_t MinAPsToThrow(SOLDIERTYPE const &s, GridNo gridno, bool const add_turning_cost) {
  int32_t ap = AP_MIN_AIM_ATTACK;

  // Make sure the guy's actually got a throwable item in his hand
  uint16_t const in_hand = s.inv[HANDPOS].usItem;
  if (!Item[in_hand].usItemClass & IC_GRENADE) {
#ifdef JA2TESTVERSION
    ScreenMsg(MSG_FONT_YELLOW, MSG_DEBUG, L"MinAPsToThrow - Called when in-hand item is %s",
              in_hand);
#endif
    return 0;
  }

  if (gridno != NOWHERE) {
    SOLDIERTYPE const *const tgt = FindSoldier(gridno, FIND_SOLDIER_GRIDNO);
    if (tgt) gridno = tgt->sGridNo;  // On a guy, get his gridno
  }

  // if attacking a new target (or if the specific target is uncertain)
  if (gridno != s.sLastTarget) ap += AP_CHANGE_TARGET;

  ap += GetAPsToChangeStance(&s, ANIM_STAND);

  // Calculate default top & bottom of the magic "aiming" formula)

  // Get this man's maximum possible action points (ignoring carryovers)
  int32_t const full_ap = CalcActionPoints(&s);

  // The 2 times is here only to around rounding off using integer math later
  int32_t const top = 2 * full_ap;

  // Tosses per turn is for max dexterity, drops down to 1/2 at dexterity = 0
  int32_t const bottom = TOSSES_PER_10TURNS * (50 + s.bDexterity / 2) / 10;

  /* Add minimum aiming time to the overall minimum AP_cost
   * This here ROUNDS UP fractions of 0.5 or higher using integer math
   * This works because 'top' is 2x what it really should be throughout */
  ap += (100 * top / bottom + 1) / 2;

  // The minimum AP cost of ANY throw can NEVER be more than merc has APs!
  if (ap > full_ap) ap = full_ap;

  // This SHOULD be impossible, but nevertheless
  if (ap < 1) ap = 1;

  return ap;
}

uint16_t GetAPsToDropBomb(SOLDIERTYPE *pSoldier) { return (AP_DROP_BOMB); }

uint16_t GetTotalAPsToDropBomb(SOLDIERTYPE *const s, int16_t const sGridNo) {
  return GetMovePlusActionAPCosts(s, sGridNo, AP_DROP_BOMB);
}

uint16_t GetAPsToUseRemote(SOLDIERTYPE *pSoldier) { return (AP_USE_REMOTE); }

int8_t GetAPsToStealItem(SOLDIERTYPE *pSoldier, int16_t usMapPos) {
  uint16_t sAPCost = PlotPath(pSoldier, usMapPos, NO_COPYROUTE, NO_PLOT, pSoldier->usUIMovementMode,
                              pSoldier->bActionPoints);

  // ADD APS TO PICKUP
  sAPCost += AP_STEAL_ITEM;

  // CJC August 13 2002: added cost to stand into equation
  if (!(PTR_STANDING)) {
    sAPCost += GetAPsToChangeStance(pSoldier, ANIM_STAND);
  }

  return ((int8_t)sAPCost);
}

static int8_t GetBPsToStealItem(SOLDIERTYPE *pSoldier) { return (BP_STEAL_ITEM); }

int8_t GetAPsToUseJar(SOLDIERTYPE *pSoldier, int16_t usMapPos) {
  uint16_t sAPCost = PlotPath(pSoldier, usMapPos, NO_COPYROUTE, NO_PLOT, pSoldier->usUIMovementMode,
                              pSoldier->bActionPoints);

  // If point cost is zero, return 0
  if (sAPCost != 0) {
    // ADD APS TO PICKUP
    sAPCost += AP_TAKE_BLOOD;
  }

  return ((int8_t)sAPCost);
}

static int8_t GetAPsToUseCan(SOLDIERTYPE *pSoldier, int16_t usMapPos) {
  uint16_t sAPCost = PlotPath(pSoldier, usMapPos, NO_COPYROUTE, NO_PLOT, pSoldier->usUIMovementMode,
                              pSoldier->bActionPoints);

  // If point cost is zero, return 0
  if (sAPCost != 0) {
    // ADD APS TO PICKUP
    sAPCost += AP_ATTACH_CAN;
  }

  return ((int8_t)sAPCost);
}

int8_t GetAPsToJumpOver(const SOLDIERTYPE *pSoldier) {
  return (GetAPsToChangeStance(pSoldier, ANIM_STAND) + AP_JUMP_OVER);
}
