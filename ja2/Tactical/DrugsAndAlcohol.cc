// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Tactical/DrugsAndAlcohol.h"

#include <algorithm>

#include "Macro.h"
#include "SGP/Random.h"
#include "Tactical/Interface.h"
#include "Tactical/Items.h"
#include "Tactical/Morale.h"
#include "Tactical/Points.h"
#include "Tactical/SoldierControl.h"
#include "Tactical/SoldierProfile.h"
#include "Utils/FontControl.h"
#include "Utils/Message.h"
#include "Utils/Text.h"
#include "Utils/TimerControl.h"

uint8_t ubDrugTravelRate[] = {4, 2};
uint8_t ubDrugWearoffRate[] = {2, 2};
uint8_t ubDrugEffect[] = {15, 8};
uint8_t ubDrugSideEffect[] = {20, 10};
uint8_t ubDrugSideEffectRate[] = {2, 1};

int32_t giDrunkModifier[] = {
    100,  // Sober
    75,   // Feeling good,
    65,   // Bporderline
    50,   // Drunk
    100,  // HungOver
};

#define HANGOVER_AP_REDUCE 5
#define HANGOVER_BP_REDUCE 200

static uint8_t GetDrugType(uint16_t usItem) {
  if (usItem == ADRENALINE_BOOSTER) {
    return (DRUG_TYPE_ADRENALINE);
  }

  if (usItem == REGEN_BOOSTER) {
    return (DRUG_TYPE_REGENERATION);
  }

  if (usItem == ALCOHOL || usItem == WINE || usItem == BEER) {
    return (DRUG_TYPE_ALCOHOL);
  }

  return (NO_DRUG);
}

BOOLEAN ApplyDrugs(SOLDIERTYPE *pSoldier, OBJECTTYPE *pObject) {
  uint8_t ubDrugType;
  uint8_t ubKitPoints;
  uint16_t usItem;

  usItem = pObject->usItem;

  // If not a syringe, return

  ubDrugType = GetDrugType(usItem);

  // Determine what type of drug....
  if (ubDrugType == NO_DRUG) {
    return (FALSE);
  }

  // do switch for Larry!!
  if (pSoldier->ubProfile == LARRY_NORMAL) {
    pSoldier = SwapLarrysProfiles(pSoldier);
  } else if (pSoldier->ubProfile == LARRY_DRUNK) {
    gMercProfiles[LARRY_DRUNK].bNPCData = 0;
  }

  if (ubDrugType < NUM_COMPLEX_DRUGS) {
    // Add effects
    if ((pSoldier->bFutureDrugEffect[ubDrugType] + ubDrugEffect[ubDrugType]) < 127) {
      pSoldier->bFutureDrugEffect[ubDrugType] += ubDrugEffect[ubDrugType];
    }
    pSoldier->bDrugEffectRate[ubDrugType] = ubDrugTravelRate[ubDrugType];

    // Increment times used during lifetime...
    // CAP!
    if (ubDrugType == DRUG_TYPE_ADRENALINE) {
      if (gMercProfiles[pSoldier->ubProfile].ubNumTimesDrugUseInLifetime != 255) {
        gMercProfiles[pSoldier->ubProfile].ubNumTimesDrugUseInLifetime++;
      }
    }

    // Increment side effects..
    if ((pSoldier->bDrugSideEffect[ubDrugType] + ubDrugSideEffect[ubDrugType]) < 127) {
      pSoldier->bDrugSideEffect[ubDrugType] += (ubDrugSideEffect[ubDrugType]);
    }
    // Stop side effects until were done....
    pSoldier->bDrugSideEffectRate[ubDrugType] = 0;

    if (ubDrugType == DRUG_TYPE_ALCOHOL) {
      // ATE: use kit points...
      if (usItem == ALCOHOL) {
        ubKitPoints = 10;
      } else if (usItem == WINE) {
        ubKitPoints = 20;
      } else {
        ubKitPoints = 100;
      }

      UseKitPoints(*pObject, ubKitPoints, *pSoldier);
    } else {
      // Remove the object....
      if (--pObject->ubNumberOfObjects == 0) DeleteObj(pObject);

      // ATE: Make guy collapse from heart attack if too much stuff taken....
      if (pSoldier->bDrugSideEffectRate[ubDrugType] > (ubDrugSideEffect[ubDrugType] * 3)) {
        // Keel over...
        DeductPoints(pSoldier, 0, 10000);

        // Permanently lower certain stats...
        pSoldier->bWisdom -= 5;
        pSoldier->bDexterity -= 5;
        pSoldier->bStrength -= 5;

        if (pSoldier->bWisdom < 1) pSoldier->bWisdom = 1;
        if (pSoldier->bDexterity < 1) pSoldier->bDexterity = 1;
        if (pSoldier->bStrength < 1) pSoldier->bStrength = 1;

        // export stat changes to profile
        gMercProfiles[pSoldier->ubProfile].bWisdom = pSoldier->bWisdom;
        gMercProfiles[pSoldier->ubProfile].bDexterity = pSoldier->bDexterity;
        gMercProfiles[pSoldier->ubProfile].bStrength = pSoldier->bStrength;

        // make those stats RED for a while...
        pSoldier->uiChangeWisdomTime = GetJA2Clock();
        pSoldier->usValueGoneUp &= ~(WIS_INCREASE);
        pSoldier->uiChangeDexterityTime = GetJA2Clock();
        pSoldier->usValueGoneUp &= ~(DEX_INCREASE);
        pSoldier->uiChangeStrengthTime = GetJA2Clock();
        pSoldier->usValueGoneUp &= ~(STRENGTH_INCREASE);
      }
    }
  } else {
    if (ubDrugType == DRUG_TYPE_REGENERATION) {
      uint8_t const n = --pObject->ubNumberOfObjects;
      int8_t const status = pObject->bStatus[n];
      if (n == 0) DeleteObj(pObject);

      // each use of a regen booster over 1, each day, reduces the effect
      int8_t bRegenPointsGained = REGEN_POINTS_PER_BOOSTER * status / 100;
      // are there fractional %s left over?
      if (status % (100 / REGEN_POINTS_PER_BOOSTER) != 0) {
        // chance of an extra point
        if (PreRandom(100 / REGEN_POINTS_PER_BOOSTER) <
            (uint32_t)(status % (100 / REGEN_POINTS_PER_BOOSTER))) {
          bRegenPointsGained++;
        }
      }

      bRegenPointsGained -= pSoldier->bRegenBoostersUsedToday;
      if (bRegenPointsGained > 0) {
        // can't go above the points you get for a full boost
        pSoldier->bRegenerationCounter =
            std::min(pSoldier->bRegenerationCounter + bRegenPointsGained, REGEN_POINTS_PER_BOOSTER);
      }
      pSoldier->bRegenBoostersUsedToday++;
    }
  }

  if (ubDrugType == DRUG_TYPE_ALCOHOL) {
    ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, pMessageStrings[MSG_DRANK_SOME], pSoldier->name,
              ShortItemNames[usItem]);
  } else {
    ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, pMessageStrings[MSG_MERC_TOOK_DRUG],
              pSoldier->name);
  }

  // Dirty panel
  fInterfacePanelDirty = DIRTYLEVEL2;

  return (TRUE);
}

void HandleEndTurnDrugAdjustments(SOLDIERTYPE *pSoldier) {
  int32_t cnt, cnt2;
  int32_t iNumLoops;
  //	int8_t bBandaged;

  for (cnt = 0; cnt < NUM_COMPLEX_DRUGS; cnt++) {
    // If side effect aret is non-zero....
    if (pSoldier->bDrugSideEffectRate[cnt] > 0) {
      // Subtract some...
      pSoldier->bDrugSideEffect[cnt] -= pSoldier->bDrugSideEffectRate[cnt];

      // If we're done, we're done!
      if (pSoldier->bDrugSideEffect[cnt] <= 0) {
        pSoldier->bDrugSideEffect[cnt] = 0;
        fInterfacePanelDirty = DIRTYLEVEL1;
      }
    }

    // IF drug rate is -ve, it's being worn off...
    if (pSoldier->bDrugEffectRate[cnt] < 0) {
      pSoldier->bDrugEffect[cnt] -= (-1 * pSoldier->bDrugEffectRate[cnt]);

      // Have we run out?
      if (pSoldier->bDrugEffect[cnt] <= 0) {
        pSoldier->bDrugEffect[cnt] = 0;

        // Dirty panel
        fInterfacePanelDirty = DIRTYLEVEL2;

        // Start the bad news!
        pSoldier->bDrugSideEffectRate[cnt] = ubDrugSideEffectRate[cnt];

        // The drug rate is 0 now too
        pSoldier->bDrugEffectRate[cnt] = 0;

        // Once for each 'level' of crash....
        iNumLoops = (pSoldier->bDrugSideEffect[cnt] / ubDrugSideEffect[cnt]) + 1;

        for (cnt2 = 0; cnt2 < iNumLoops; cnt2++) {
          // OK, give a much BIGGER morale downer
          if (cnt == DRUG_TYPE_ALCOHOL) {
            HandleMoraleEvent(pSoldier, MORALE_ALCOHOL_CRASH, pSoldier->sSectorX,
                              pSoldier->sSectorY, pSoldier->bSectorZ);
          } else {
            HandleMoraleEvent(pSoldier, MORALE_DRUGS_CRASH, pSoldier->sSectorX, pSoldier->sSectorY,
                              pSoldier->bSectorZ);
          }
        }
      }
    }

    // Add increase ineffect....
    if (pSoldier->bDrugEffectRate[cnt] > 0) {
      // Seap some in....
      pSoldier->bFutureDrugEffect[cnt] -= pSoldier->bDrugEffectRate[cnt];
      pSoldier->bDrugEffect[cnt] += pSoldier->bDrugEffectRate[cnt];

      // Refresh morale w/ new drug value...
      RefreshSoldierMorale(pSoldier);

      // Check if we need to stop 'adding'
      if (pSoldier->bFutureDrugEffect[cnt] <= 0) {
        pSoldier->bFutureDrugEffect[cnt] = 0;
        // Change rate to -ve..
        pSoldier->bDrugEffectRate[cnt] = -ubDrugWearoffRate[cnt];
      }
    }
  }

  if (pSoldier->bRegenerationCounter > 0) {
    //		bBandaged = BANDAGED( pSoldier );

    // increase life
    pSoldier->bLife =
        std::min((int8_t)(pSoldier->bLife + LIFE_GAIN_PER_REGEN_POINT), pSoldier->bLifeMax);

    if (pSoldier->bLife == pSoldier->bLifeMax) {
      pSoldier->bBleeding = 0;
    } else if (pSoldier->bBleeding + pSoldier->bLife > pSoldier->bLifeMax) {
      // got to reduce amount of bleeding
      pSoldier->bBleeding = (pSoldier->bLifeMax - pSoldier->bLife);
    }

    // decrement counter
    pSoldier->bRegenerationCounter--;
  }
}

int8_t GetDrugEffect(SOLDIERTYPE *pSoldier, uint8_t ubDrugType) {
  return (pSoldier->bDrugEffect[ubDrugType]);
}

void HandleAPEffectDueToDrugs(const SOLDIERTYPE *const pSoldier, uint8_t *const pubPoints) {
  int8_t bDrunkLevel;
  int16_t sPoints = (*pubPoints);

  // Are we in a side effect or good effect?
  if (pSoldier->bDrugEffect[DRUG_TYPE_ADRENALINE]) {
    // Adjust!
    sPoints += pSoldier->bDrugEffect[DRUG_TYPE_ADRENALINE];
  } else if (pSoldier->bDrugSideEffect[DRUG_TYPE_ADRENALINE]) {
    // Adjust!
    sPoints -= pSoldier->bDrugSideEffect[DRUG_TYPE_ADRENALINE];

    if (sPoints < AP_MINIMUM) {
      sPoints = AP_MINIMUM;
    }
  }

  bDrunkLevel = GetDrunkLevel(pSoldier);

  if (bDrunkLevel == HUNGOVER) {
    // Reduce....
    sPoints -= HANGOVER_AP_REDUCE;

    if (sPoints < AP_MINIMUM) {
      sPoints = AP_MINIMUM;
    }
  }

  (*pubPoints) = (uint8_t)sPoints;
}

void HandleBPEffectDueToDrugs(SOLDIERTYPE *pSoldier, int16_t *psPointReduction) {
  int8_t bDrunkLevel;

  // Are we in a side effect or good effect?
  if (pSoldier->bDrugEffect[DRUG_TYPE_ADRENALINE]) {
    // Adjust!
    (*psPointReduction) -=
        (pSoldier->bDrugEffect[DRUG_TYPE_ADRENALINE] * BP_RATIO_RED_PTS_TO_NORMAL);
  } else if (pSoldier->bDrugSideEffect[DRUG_TYPE_ADRENALINE]) {
    // Adjust!
    (*psPointReduction) +=
        (pSoldier->bDrugSideEffect[DRUG_TYPE_ADRENALINE] * BP_RATIO_RED_PTS_TO_NORMAL);
  }

  bDrunkLevel = GetDrunkLevel(pSoldier);

  if (bDrunkLevel == HUNGOVER) {
    // Reduce....
    (*psPointReduction) += HANGOVER_BP_REDUCE;
  }
}

int8_t GetDrunkLevel(const SOLDIERTYPE *pSoldier) {
  int8_t bNumDrinks;

  // If we have a -ve effect ...
  if (pSoldier->bDrugEffect[DRUG_TYPE_ALCOHOL] == 0 &&
      pSoldier->bDrugSideEffect[DRUG_TYPE_ALCOHOL] == 0) {
    return (SOBER);
  }

  if (pSoldier->bDrugEffect[DRUG_TYPE_ALCOHOL] == 0 &&
      pSoldier->bDrugSideEffect[DRUG_TYPE_ALCOHOL] != 0) {
    return (HUNGOVER);
  }

  // Calculate how many dinks we have had....
  bNumDrinks = (pSoldier->bDrugEffect[DRUG_TYPE_ALCOHOL] / ubDrugEffect[DRUG_TYPE_ALCOHOL]);

  if (bNumDrinks <= 3) {
    return (FEELING_GOOD);
  } else if (bNumDrinks <= 4) {
    return (BORDERLINE);
  } else {
    return (DRUNK);
  }
}

int32_t EffectStatForBeingDrunk(const SOLDIERTYPE *pSoldier, int32_t iStat) {
  return ((iStat * giDrunkModifier[GetDrunkLevel(pSoldier)] / 100));
}

BOOLEAN MercUnderTheInfluence(const SOLDIERTYPE *pSoldier) {
  // Are we in a side effect or good effect?
  return pSoldier->bDrugEffect[DRUG_TYPE_ADRENALINE] ||
         pSoldier->bDrugSideEffect[DRUG_TYPE_ADRENALINE] || GetDrunkLevel(pSoldier) != SOBER;
}
