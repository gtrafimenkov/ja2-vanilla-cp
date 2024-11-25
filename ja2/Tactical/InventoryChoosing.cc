// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Tactical/InventoryChoosing.h"

#include <algorithm>
#include <string.h>

#include "GameSettings.h"
#include "Macro.h"
#include "SGP/Debug.h"
#include "SGP/Random.h"
#include "Strategic/AutoResolve.h"
#include "Strategic/MapScreenInterfaceMap.h"
#include "Strategic/StrategicMap.h"
#include "Strategic/StrategicStatus.h"
#include "Tactical/AnimationData.h"
#include "Tactical/Campaign.h"
#include "Tactical/Items.h"
#include "Tactical/Weapons.h"

#define ENEMYAMMODROPRATE 50     // % of time enemies drop ammunition
#define ENEMYGRENADEDROPRATE 25  // % of time enemies drop grenades
#define ENEMYEQUIPDROPRATE 15    // % of stuff enemies drop equipment

// only 1/10th of what enemies drop...
#define MILITIAAMMODROPRATE 5     // % of time enemies drop ammunition
#define MILITIAGRENADEDROPRATE 3  // % of time enemies drop grenades
#define MILITIAEQUIPDROPRATE 2    // % of stuff enemies drop equipment

#define MAX_MORTARS_PER_TEAM \
  1  // one team can't randomly roll more than this many mortars per sector

uint32_t guiMortarsRolledByTeam = 0;

struct ARMY_GUN_CHOICE_TYPE {
  uint8_t ubChoices;  // how many valid choices there are in this category
  int8_t bItemNo[5];  // room for up to 5 choices of gun in each category
};

static const ARMY_GUN_CHOICE_TYPE gRegularArmyGunChoices[ARMY_GUN_LEVELS] = {
    {/* 0 - lo pistols     */ 2, {SW38, DESERTEAGLE, -1, -1, -1}},
    {/* 1 - hi pistols     */ 2, {GLOCK_17, BERETTA_93R, -1, -1, -1}},
    {/* 2 - lo SMG/shotgun */ 2, {M870, MP5K, -1, -1, -1}},
    {/* 3 - lo rifles      */ 1, {MINI14, -1, -1, -1, -1}},
    {/* 4 - hi SMGs        */ 2, {MAC10, COMMANDO, -1, -1, -1}},
    {/* 5 - med rifles     */ 1, {G41, -1, -1, -1, -1}},
    {/* 6 - sniper rifles  */ 1, {M24, -1, -1, -1, -1}},
    {/* 7 - hi rifles      */ 2, {M14, C7, -1, -1, -1}},
    {/* 8 - best rifle     */ 1, {FNFAL, -1, -1, -1, -1}},
    {/* 9 - machine guns   */ 1, {MINIMI, -1, -1, -1, -1}},
    {/* 10- rocket rifle   */ 2, {ROCKET_RIFLE, MINIMI, -1, -1, -1}}};

static const ARMY_GUN_CHOICE_TYPE gExtendedArmyGunChoices[ARMY_GUN_LEVELS] = {
    {/* 0 - lo pistols    */ 5, {SW38, BARRACUDA, DESERTEAGLE, GLOCK_17, M1911}},
    {/* 1 - hi pist/shtgn */ 4, {GLOCK_18, BERETTA_93R, BERETTA_92F, M870, -1}},
    {/* 2 - lo SMGs/shtgn */ 5, {TYPE85, THOMPSON, MP53, MP5K, SPAS15}},
    {/* 3 - lo rifles     */ 2, {MINI14, SKS, -1, -1, -1}},
    {/* 4 - hi SMGs       */ 3, {MAC10, AKSU74, COMMANDO, -1, -1}},
    {/* 5 - med rifles    */ 4, {AKM, G3A3, G41, AK74, -1}},
    {/* 6 - sniper rifles */ 2, {DRAGUNOV, M24, -1, -1, -1}},
    {/* 7 - hi rifles     */ 4, {FAMAS, M14, AUG, C7, -1}},
    {/* 8 - best rifle    */ 1, {FNFAL, -1, -1, -1, -1}},
    {/* 9 - machine guns  */ 3, {MINIMI, RPK74, HK21E, -1, -1}},
    {/* 10- rocket rifle  */ 4, {ROCKET_RIFLE, ROCKET_RIFLE, RPK74, HK21E, -1}}};

static void MarkAllWeaponsOfSameGunClassAsDropped(uint16_t usWeapon);

void InitArmyGunTypes() {
  uint32_t uiGunLevel;
  uint32_t uiChoice;
  int8_t bItemNo;
  uint8_t ubWeapon;

  // depending on selection of the gun nut option
  const ARMY_GUN_CHOICE_TYPE *pGunChoiceTable;
  if (gGameOptions.fGunNut) {
    // use table of extended gun choices
    pGunChoiceTable = gExtendedArmyGunChoices;
  } else {
    // use table of regular gun choices
    pGunChoiceTable = gRegularArmyGunChoices;
  }

  // for each gun category
  for (uiGunLevel = 0; uiGunLevel < ARMY_GUN_LEVELS; uiGunLevel++) {
    // choose one the of the possible gun choices to be used by the army for
    // this game & store it
    uiChoice = Random(pGunChoiceTable[uiGunLevel].ubChoices);
    bItemNo = pGunChoiceTable[uiGunLevel].bItemNo[uiChoice];
    AssertMsg(bItemNo != -1, "Invalid army gun choice in table");
    gStrategicStatus.ubStandardArmyGunIndex[uiGunLevel] = (uint8_t)bItemNo;
  }

  // set all flags that track whether this weapon type has been dropped before
  // to FALSE
  for (ubWeapon = 0; ubWeapon < MAX_WEAPONS; ubWeapon++) {
    gStrategicStatus.fWeaponDroppedAlready[ubWeapon] = FALSE;
  }

  // avoid auto-drops for the gun class with the crappiest guns in it
  MarkAllWeaponsOfSameGunClassAsDropped(SW38);
}

static int8_t GetWeaponClass(uint16_t usGun) {
  uint32_t uiGunLevel, uiLoop;

  // always use the extended list since it contains all guns...
  for (uiGunLevel = 0; uiGunLevel < ARMY_GUN_LEVELS; uiGunLevel++) {
    for (uiLoop = 0; uiLoop < gExtendedArmyGunChoices[uiGunLevel].ubChoices; uiLoop++) {
      if (gExtendedArmyGunChoices[uiGunLevel].bItemNo[uiLoop] == usGun) {
        return ((int8_t)uiGunLevel);
      }
    }
  }
  return (-1);
}

static void MarkAllWeaponsOfSameGunClassAsDropped(uint16_t usWeapon) {
  int8_t bGunClass;
  uint32_t uiLoop;

  // mark that item itself as dropped, whether or not it's part of a gun class
  gStrategicStatus.fWeaponDroppedAlready[usWeapon] = TRUE;

  bGunClass = GetWeaponClass(usWeapon);

  // if the gun belongs to a gun class (mortars, GLs, LAWs, etc. do not and are
  // handled independently)
  if (bGunClass != -1) {
    // then mark EVERY gun in that class as dropped
    for (uiLoop = 0; uiLoop < gExtendedArmyGunChoices[bGunClass].ubChoices; uiLoop++) {
      gStrategicStatus.fWeaponDroppedAlready[gExtendedArmyGunChoices[bGunClass].bItemNo[uiLoop]] =
          TRUE;
    }
  }
}

static void ChooseArmourForSoldierCreateStruct(SOLDIERCREATE_STRUCT *pp, int8_t bHelmetClass,
                                               int8_t bVestClass, int8_t bLeggingsClass);
static void ChooseBombsForSoldierCreateStruct(SOLDIERCREATE_STRUCT *pp, int8_t bBombClass);
static void ChooseFaceGearForSoldierCreateStruct(SOLDIERCREATE_STRUCT *pp);
static void ChooseGrenadesForSoldierCreateStruct(SOLDIERCREATE_STRUCT *pp, int8_t bGrenades,
                                                 int8_t bGrenadeClass, BOOLEAN fGrenadeLauncher);
static void ChooseKitsForSoldierCreateStruct(SOLDIERCREATE_STRUCT *pp, int8_t bKitClass);
static void ChooseLocationSpecificGearForSoldierCreateStruct(SOLDIERCREATE_STRUCT *pp);
static void ChooseMiscGearForSoldierCreateStruct(SOLDIERCREATE_STRUCT *pp, int8_t bMiscClass);
static void ChooseSpecialWeaponsForSoldierCreateStruct(SOLDIERCREATE_STRUCT *pp, int8_t bKnifeClass,
                                                       BOOLEAN fGrenadeLauncher, BOOLEAN fLAW,
                                                       BOOLEAN fMortar);
static void ChooseWeaponForSoldierCreateStruct(SOLDIERCREATE_STRUCT *pp, int8_t bWeaponClass,
                                               int8_t bAmmoClips, int8_t bAttachClass,
                                               BOOLEAN fAttachment);
static void EquipTank(SOLDIERCREATE_STRUCT *pp);
static void RandomlyChooseWhichItemsAreDroppable(SOLDIERCREATE_STRUCT *pp, int8_t bSoldierClass);

// Chooses equipment based on the relative equipment level (0-4) with best
// being 4.  It allocates a range of equipment to choose from. NOTE:  I'm just
// winging it for the decisions on which items that different groups can have.
// Basically,
// there are variations, so a guy at a certain level may get a better gun and
// worse armour or vice versa.
void GenerateRandomEquipment(SOLDIERCREATE_STRUCT *pp, int8_t bSoldierClass,
                             int8_t bEquipmentRating) {
  OBJECTTYPE *pItem;
  // general rating information
  int8_t bRating = 0;
  // numbers of items
  int8_t bAmmoClips = 0;
  int8_t bGrenades = 0;
  BOOLEAN fAttachment = FALSE;
  // item levels
  int8_t bWeaponClass = 0;
  int8_t bHelmetClass = 0;
  int8_t bVestClass = 0;
  int8_t bLeggingClass = 0;
  int8_t bAttachClass = 0;
  int8_t bGrenadeClass = 0;
  int8_t bKnifeClass = 0;
  int8_t bKitClass = 0;
  int8_t bMiscClass = 0;
  int8_t bBombClass = 0;
  // special weapons
  BOOLEAN fMortar = FALSE;
  BOOLEAN fGrenadeLauncher = FALSE;
  BOOLEAN fLAW = FALSE;
  int32_t i;
  int8_t bEquipmentModifier;
  uint8_t ubMaxSpecialWeaponRoll;

  Assert(pp);

  // kids don't get anything 'cause they don't have any weapon animations and
  // the rest is inappropriate
  if ((pp->bBodyType == HATKIDCIV) || (pp->bBodyType == KIDCIV)) {
    return;
  }

  if ((pp->bBodyType == TANK_NE) || (pp->bBodyType == TANK_NW)) {
    EquipTank(pp);
    return;
  }

  Assert((bSoldierClass >= SOLDIER_CLASS_NONE) && (bSoldierClass <= SOLDIER_CLASS_ELITE_MILITIA));
  Assert((bEquipmentRating >= 0) && (bEquipmentRating <= 4));

  // equipment level is modified by 1/10 of the difficulty percentage, -5, so
  // it's between -5 to +5 (on normal, this is actually -4 to +4, easy is -5 to
  // +3, and hard is -3 to +5)
  bEquipmentModifier = bEquipmentRating + ((CalcDifficultyModifier(bSoldierClass) / 10) - 5);

  switch (bSoldierClass) {
    case SOLDIER_CLASS_NONE:
      // ammo is here only so that civilians with pre-assigned ammo will get some
      // clips for it!
      bAmmoClips = (int8_t)(1 + Random(2));

      // civilians get nothing, anyone who should get something should be
      // preassigned by Linda
      break;

    case SOLDIER_CLASS_ADMINISTRATOR:
    case SOLDIER_CLASS_GREEN_MILITIA:
      bRating = BAD_ADMINISTRATOR_EQUIPMENT_RATING + bEquipmentModifier;
      bRating = (int8_t)std::max((int8_t)MIN_EQUIPMENT_CLASS,
                                 std::min((int8_t)MAX_EQUIPMENT_CLASS, bRating));

      bWeaponClass = bRating;

      // Note:  in some cases the class of armour and/or helmet won't be high
      // enough to make 			 the lowest level.
      bVestClass = bRating;
      bHelmetClass = bRating;
      // no leggings

      if (Random(2)) bKnifeClass = bRating;

      bAmmoClips = (int8_t)(1 + Random(2));

      if (bRating >= GOOD_ADMINISTRATOR_EQUIPMENT_RATING) {
        bAmmoClips++;

        bKitClass = bRating;
        bMiscClass = bRating;
      }

      if (bRating >= GREAT_ADMINISTRATOR_EQUIPMENT_RATING) {
        bGrenades = 1, bGrenadeClass = bRating;
      }

      if ((bRating > MIN_EQUIPMENT_CLASS) &&
          bRating < MAX_EQUIPMENT_CLASS) {  // Randomly decide if there is to be an
                                            // upgrade of guns vs armour (one goes
                                            // up, the other down)
        switch (Random(5)) {
          case 0:
            bWeaponClass++, bVestClass--;
            break;  // better gun, worse armour
          case 1:
            bWeaponClass--, bVestClass++;
            break;  // worse gun, better armour
        }
      }
      break;

    case SOLDIER_CLASS_ARMY:
    case SOLDIER_CLASS_REG_MILITIA:
      // army guys tend to have a broad range of equipment
      bRating = BAD_ARMY_EQUIPMENT_RATING + bEquipmentModifier;
      bRating = (int8_t)std::max((int8_t)MIN_EQUIPMENT_CLASS,
                                 std::min((int8_t)MAX_EQUIPMENT_CLASS, bRating));

      bWeaponClass = bRating;
      bVestClass = bRating;
      bHelmetClass = bRating;
      bGrenadeClass = bRating;

      if ((bRating >= GOOD_ARMY_EQUIPMENT_RATING) && (Random(100) < 33)) {
        fAttachment = TRUE;
        bAttachClass = bRating;
      }

      bAmmoClips = (int8_t)(2 + Random(2));

      if (bRating >= AVERAGE_ARMY_EQUIPMENT_RATING) {
        bGrenades = (int8_t)Random(2);
        bKitClass = bRating;
        bMiscClass = bRating;
      }

      if (bRating >= GOOD_ARMY_EQUIPMENT_RATING) {
        bGrenades++;
      }

      if (bRating >= GREAT_ARMY_EQUIPMENT_RATING) {
        bGrenades++;

        bLeggingClass = bRating;

        if (Chance(25)) {
          bBombClass = bRating;
        }
      }

      if (Random(2)) bKnifeClass = bRating;

      if ((bRating > MIN_EQUIPMENT_CLASS) && bRating < MAX_EQUIPMENT_CLASS) {
        switch (Random(7)) {
          case 3:
            bWeaponClass++, bVestClass--;
            break;  // better gun, worse armour
          case 4:
            bWeaponClass--, bVestClass++;
            break;  // worse gun, better armour
          case 5:
            bVestClass++, bHelmetClass--;
            break;  // better armour, worse helmet
          case 6:
            bVestClass--, bHelmetClass++;
            break;  // worse armour, better helmet
        }
      }

      // if well-enough equipped, 1/5 chance of something really special
      if ((bRating >= GREAT_ARMY_EQUIPMENT_RATING) && (Random(100) < 20)) {
        // give this man a special weapon!  No mortars if underground, however
        ubMaxSpecialWeaponRoll = (!IsAutoResolveActive() && (gbWorldSectorZ != 0)) ? 6 : 7;
        switch (Random(ubMaxSpecialWeaponRoll)) {
          case 0:
          case 1:
          case 2:
            if (pp->bExpLevel >= 3) {
              // grenade launcher
              fGrenadeLauncher = TRUE;
              bGrenades = 3 + (int8_t)(Random(3));  // 3-5
            }
            break;

          case 3:
          case 4:
          case 5:
            if (pp->bExpLevel >= 4) {
              // LAW rocket launcher
              fLAW = TRUE;
            }
            break;

          case 6:
            // one per team maximum!
            if ((pp->bExpLevel >= 5) && (guiMortarsRolledByTeam < MAX_MORTARS_PER_TEAM)) {
              // mortar
              fMortar = TRUE;
              guiMortarsRolledByTeam++;

              // the grenades will actually represent mortar shells in this case
              bGrenades = 2 + (int8_t)(Random(3));  // 2-4
              bGrenadeClass = MORTAR_GRENADE_CLASS;
            }
            break;
        }
      }
      break;

    case SOLDIER_CLASS_ELITE:
    case SOLDIER_CLASS_ELITE_MILITIA:
      bRating = BAD_ELITE_EQUIPMENT_RATING + bEquipmentModifier;
      bRating = (int8_t)std::max((int8_t)MIN_EQUIPMENT_CLASS,
                                 std::min((int8_t)MAX_EQUIPMENT_CLASS, bRating));

      bWeaponClass = bRating;
      bHelmetClass = bRating;
      bVestClass = bRating;
      bLeggingClass = bRating;
      bAttachClass = bRating;
      bGrenadeClass = bRating;
      bKitClass = bRating;
      bMiscClass = bRating;

      if (Chance(25)) {
        bBombClass = bRating;
      }

      bAmmoClips = (int8_t)(3 + Random(2));
      bGrenades = (int8_t)(1 + Random(3));

      if ((bRating >= AVERAGE_ELITE_EQUIPMENT_RATING) && (Random(100) < 75)) {
        fAttachment = TRUE;
        bAttachClass = bRating;
      }

      if (Random(2)) bKnifeClass = bRating;

      if ((bRating > MIN_EQUIPMENT_CLASS) && bRating < MAX_EQUIPMENT_CLASS) {
        switch (Random(11)) {
          case 4:
            bWeaponClass++, bVestClass--;
            break;
          case 5:
            bWeaponClass--, bVestClass--;
            break;
          case 6:
            bVestClass++, bHelmetClass--;
            break;
          case 7:
            bGrenades += 2;
            break;
          case 8:
            bHelmetClass++;
            break;
          case 9:
            bVestClass++;
            break;
          case 10:
            bWeaponClass++;
            break;
        }
      }

      // if well-enough equipped, 1/3 chance of something really special
      if ((bRating >= GOOD_ELITE_EQUIPMENT_RATING) && (Random(100) < 33)) {
        // give this man a special weapon!  No mortars if underground, however
        ubMaxSpecialWeaponRoll = (!IsAutoResolveActive() && (gbWorldSectorZ != 0)) ? 6 : 7;
        switch (Random(ubMaxSpecialWeaponRoll)) {
          case 0:
          case 1:
          case 2:
            // grenade launcher
            fGrenadeLauncher = TRUE;
            bGrenades = 4 + (int8_t)(Random(4));  // 4-7
            break;
          case 3:
          case 4:
          case 5:
            // LAW rocket launcher
            fLAW = TRUE;
            break;
          case 6:
            // one per team maximum!
            if (guiMortarsRolledByTeam < MAX_MORTARS_PER_TEAM) {
              // mortar
              fMortar = TRUE;
              guiMortarsRolledByTeam++;

              // the grenades will actually represent mortar shells in this case
              bGrenades = 3 + (int8_t)(Random(5));  // 3-7
              bGrenadeClass = MORTAR_GRENADE_CLASS;
            }
            break;
        }
      }
      break;
  }

  for (i = 0; i < NUM_INV_SLOTS; i++) {  // clear items, but only if they have write status.
    if (!(pp->Inv[i].fFlags & OBJECT_NO_OVERWRITE)) {
      memset(&(pp->Inv[i]), 0, sizeof(OBJECTTYPE));
      pp->Inv[i].fFlags |= OBJECT_UNDROPPABLE;
    } else {  // check to see what kind of item is here.  If we find a gun, for
              // example, it'll make the
      // bWeaponClass negative to signify that a gun has already been specified,
      // and later code will use that to determine that and to pick ammo for it.
      pItem = &pp->Inv[i];
      if (!pItem) continue;
      switch (Item[pItem->usItem].usItemClass) {
        case IC_GUN:
          if (pItem->usItem != ROCKET_LAUNCHER) {
            bWeaponClass *= -1;
          } else  // rocket launcher!
          {
            fLAW = FALSE;
          }
          break;
        case IC_AMMO:
          bAmmoClips = 0;
          break;
        case IC_BLADE:
        case IC_THROWING_KNIFE:
          bKnifeClass = 0;
          break;
        case IC_LAUNCHER:
          fGrenadeLauncher = FALSE;
          fMortar = FALSE;
          break;
        case IC_ARMOUR:
          if (i == HELMETPOS)
            bHelmetClass = 0;
          else if (i == VESTPOS)
            bVestClass = 0;
          else if (i == LEGPOS)
            bLeggingClass = 0;
          break;
        case IC_GRENADE:
          bGrenades = 0;
          bGrenadeClass = 0;
          break;
        case IC_MEDKIT:
        case IC_KIT:
          bKitClass = 0;
          break;
        case IC_MISC:
          bMiscClass = 0;
          break;
        case IC_BOMB:
          bBombClass = 0;
          break;
      }
    }
  }

  // special: militia shouldn't drop bombs
  if (!(SOLDIER_CLASS_ENEMY(bSoldierClass))) {
    bBombClass = 0;
  }

  // Now actually choose the equipment!
  ChooseWeaponForSoldierCreateStruct(pp, bWeaponClass, bAmmoClips, bAttachClass, fAttachment);
  ChooseGrenadesForSoldierCreateStruct(pp, bGrenades, bGrenadeClass, fGrenadeLauncher);
  ChooseArmourForSoldierCreateStruct(pp, bHelmetClass, bVestClass, bLeggingClass);
  ChooseSpecialWeaponsForSoldierCreateStruct(pp, bKnifeClass, fGrenadeLauncher, fLAW, fMortar);
  ChooseFaceGearForSoldierCreateStruct(pp);
  ChooseKitsForSoldierCreateStruct(pp, bKitClass);
  ChooseMiscGearForSoldierCreateStruct(pp, bMiscClass);
  ChooseBombsForSoldierCreateStruct(pp, bBombClass);
  ChooseLocationSpecificGearForSoldierCreateStruct(pp);
  RandomlyChooseWhichItemsAreDroppable(pp, bSoldierClass);
}

static BOOLEAN PlaceObjectInSoldierCreateStruct(SOLDIERCREATE_STRUCT *pp, OBJECTTYPE *pObject);
static uint16_t SelectStandardArmyGun(uint8_t uiGunLevel);

// When using the class values, they should all range from 0-11, 0 meaning that
// there will be no selection for that particular type of item, and 1-11 means to
// choose an item if possible.  1 is the worst class of item, while 11 is the
// best.
static void ChooseWeaponForSoldierCreateStruct(SOLDIERCREATE_STRUCT *pp, int8_t bWeaponClass,
                                               int8_t bAmmoClips, int8_t bAttachClass,
                                               BOOLEAN fAttachment) {
  OBJECTTYPE Object;
  uint16_t i;
  uint16_t usRandom;
  uint16_t usNumMatches = 0;
  uint16_t usGunIndex = 0;
  uint16_t usAmmoIndex = 0;
  uint16_t usAttachIndex = 0;
  uint8_t ubChanceStandardAmmo;
  int8_t bStatus;

  // Choose weapon:
  // WEAPONS are very important, and are therefore handled differently using
  // special pre-generated tables. It was requested that enemies use only a
  // small subset of guns with a lot duplication of the same gun type.

  // if gun was pre-selected (rcvd negative weapon class) and needs ammo
  if (bWeaponClass < 0 && bAmmoClips) {  // Linda has added a specific gun to the
                                         // merc's inventory, but no ammo.  So, we
    // will choose ammunition that works with the gun.
    for (i = 0; i < NUM_INV_SLOTS; i++) {
      if (Item[pp->Inv[i].usItem].usItemClass == IC_GUN) {
        usGunIndex = pp->Inv[i].usItem;
        ubChanceStandardAmmo = 100 - (bWeaponClass * -9);  // weapon class is negative!
        usAmmoIndex = RandomMagazine(usGunIndex, ubChanceStandardAmmo);

        if (usGunIndex == ROCKET_RIFLE) {
          pp->Inv[i].ubImprintID = (NO_PROFILE + 1);
        }

        break;
      }
    }
    if (bAmmoClips && usAmmoIndex) {
      CreateItems(usAmmoIndex, 100, bAmmoClips, &Object);
      Object.fFlags |= OBJECT_UNDROPPABLE;
      PlaceObjectInSoldierCreateStruct(pp, &Object);
    }

    return;
  }

  if (bWeaponClass < 1) return;  // empty handed / pre-selected

  // reduce anything over 11 to 11
  if (bWeaponClass > ARMY_GUN_LEVELS) {
    bWeaponClass = ARMY_GUN_LEVELS;
  }

  // the weapon class here ranges from 1 to 11, so subtract 1 to get a gun level
  usGunIndex = SelectStandardArmyGun((uint8_t)(bWeaponClass - 1));

  if (bAmmoClips) {  // We have a gun, so choose ammo clips

    // check default ammo first...
    usAmmoIndex = DefaultMagazine(usGunIndex);
    switch (Magazine[Item[usAmmoIndex].ubClassIndex].ubAmmoType) {
      case AMMO_AP:
      case AMMO_SUPER_AP:
        // assault rifle, rocket rifle (etc) - high chance of having AP ammo
        ubChanceStandardAmmo = 80;
        break;
      default:
        ubChanceStandardAmmo = 100 - (bWeaponClass * 9);
        break;
    }
    usAmmoIndex = RandomMagazine(usGunIndex, ubChanceStandardAmmo);
  }

  // Choose attachment
  if (bAttachClass && fAttachment) {
    usNumMatches = 0;
    while (bAttachClass && !usNumMatches) {  // Count the number of valid attachments.
      for (i = 0; i < MAXITEMS; i++) {
        const INVTYPE *const pItem = &Item[i];
        if (pItem->usItemClass == IC_MISC && pItem->ubCoolness == bAttachClass &&
            ValidAttachment(i, usGunIndex))
          usNumMatches++;
      }
      if (!usNumMatches) {
        bAttachClass--;
      }
    }
    if (usNumMatches) {
      usRandom = (uint16_t)Random(usNumMatches);
      for (i = 0; i < MAXITEMS; i++) {
        const INVTYPE *const pItem = &Item[i];
        if (pItem->usItemClass == IC_MISC && pItem->ubCoolness == bAttachClass &&
            ValidAttachment(i, usGunIndex)) {
          if (usRandom)
            usRandom--;
          else {
            usAttachIndex = i;
            break;
          }
        }
      }
    }
  }
  // Now, we have chosen all of the correct items.  Now, we will assign them
  // into the slots. Because we are dealing with enemies, automatically give them
  // full ammo in their weapon.
  if (!(pp->Inv[HANDPOS].fFlags & OBJECT_NO_OVERWRITE)) {
    switch (pp->ubSoldierClass) {
      case SOLDIER_CLASS_ADMINISTRATOR:
      case SOLDIER_CLASS_ARMY:
      case SOLDIER_CLASS_GREEN_MILITIA:
      case SOLDIER_CLASS_REG_MILITIA:
        // Admins/Troops: 60-75% + 1% every 4% progress
        bStatus = (int8_t)(60 + Random(16));
        bStatus += (int8_t)(HighestPlayerProgressPercentage() / 4);
        bStatus = (int8_t)std::min((int8_t)100, bStatus);
        break;
      case SOLDIER_CLASS_ELITE:
      case SOLDIER_CLASS_ELITE_MILITIA:
        // 85-90% +  1% every 10% progress
        bStatus = (int8_t)(85 + Random(6));
        bStatus += (int8_t)(HighestPlayerProgressPercentage() / 10);
        bStatus = (int8_t)std::min((int8_t)100, bStatus);
        break;
      default:
        bStatus = (int8_t)(50 + Random(51));
        break;
    }
    // don't allow it to be lower than marksmanship, we don't want it to affect
    // their chances of hitting
    bStatus = (int8_t)std::max(pp->bMarksmanship, bStatus);

    CreateItem(usGunIndex, bStatus, &(pp->Inv[HANDPOS]));
    pp->Inv[HANDPOS].fFlags |= OBJECT_UNDROPPABLE;

    // Rocket Rifles must come pre-imprinted, in case carrier gets killed
    // without getting a shot off
    if (usGunIndex == ROCKET_RIFLE) {
      pp->Inv[HANDPOS].ubImprintID = (NO_PROFILE + 1);
    }
  } else {  // slot locked, so don't add any attachments to it!
    usAttachIndex = 0;
  }
  // Ammo
  if (bAmmoClips && usAmmoIndex) {
    CreateItems(usAmmoIndex, 100, bAmmoClips, &Object);
    Object.fFlags |= OBJECT_UNDROPPABLE;
    PlaceObjectInSoldierCreateStruct(pp, &Object);
  }
  if (usAttachIndex) {
    CreateItem(usAttachIndex, 100, &Object);
    Object.fFlags |= OBJECT_UNDROPPABLE;
    AttachObject(NULL, &(pp->Inv[HANDPOS]), &Object);
  }
}

static void ChooseGrenadesForSoldierCreateStruct(SOLDIERCREATE_STRUCT *pp, int8_t bGrenades,
                                                 int8_t bGrenadeClass, BOOLEAN fGrenadeLauncher) {
  OBJECTTYPE Object;
  int16_t sNumPoints;
  uint16_t usItem;
  uint8_t ubBaseQuality;
  uint8_t ubQualityVariation;
  // numbers of each type the player will get!
  uint8_t ubNumStun = 0;
  uint8_t ubNumTear = 0;
  uint8_t ubNumMustard = 0;
  uint8_t ubNumMini = 0;
  uint8_t ubNumReg = 0;
  uint8_t ubNumSmoke = 0;
  uint8_t ubNumFlare = 0;

  // determine how many *points* the enemy will get to spend on grenades...
  sNumPoints = bGrenades * bGrenadeClass;

  // no points, no grenades!
  if (!sNumPoints) return;

  // special mortar shell handling
  if (bGrenadeClass == MORTAR_GRENADE_CLASS) {
    CreateItems(MORTAR_SHELL, (int8_t)(80 + Random(21)), bGrenades, &Object);
    Object.fFlags |= OBJECT_UNDROPPABLE;
    PlaceObjectInSoldierCreateStruct(pp, &Object);
    return;
  }

  Assert(bGrenadeClass <= 11);

  // determine the quality of grenades.  The elite guys get the best quality,
  // while the others get progressively worse.
  ubBaseQuality = (uint8_t)std::min(45 + bGrenadeClass * 5, 90);
  ubQualityVariation = 101 - ubBaseQuality;

  // now, purchase the grenades.
  while (sNumPoints > 0) {
    if (sNumPoints >= 20) {  // Choose randomly between mustard and regular
      if (Random(2) && !fGrenadeLauncher)
        ubNumMustard++, sNumPoints -= 10;
      else
        ubNumReg++, sNumPoints -= 9;
    }

    if (sNumPoints >= 10) {  // Choose randomly between any
      switch (Random(7)) {
        case 0:
          if (!fGrenadeLauncher) {
            ubNumMustard++;
            sNumPoints -= 10;
            break;
          }
          // if grenade launcher, pick regular instead
        case 1:
          ubNumReg++;
          sNumPoints -= 9;
          break;
        case 2:
          if (!fGrenadeLauncher) {
            ubNumMini++;
            sNumPoints -= 7;
            break;
          }
          // if grenade launcher, pick tear instead
        case 3:
          ubNumTear++;
          sNumPoints -= 6;
          break;
        case 4:
          ubNumStun++;
          sNumPoints -= 5;
          break;
        case 5:
          ubNumSmoke++;
          sNumPoints -= 4;
          break;
        case 6:
          if (!fGrenadeLauncher) {
            ubNumFlare++;
            sNumPoints -= 3;
          }
          break;
      }
    }

    // JA2 Gold: don't make mini-grenades take all points available, and add
    // chance of break lights
    if (sNumPoints >= 1 && sNumPoints < 10) {
      switch (Random(10)) {
        case 0:
        case 1:
        case 2:
          ubNumSmoke++;
          sNumPoints -= 4;
          break;
        case 3:
        case 4:
          ubNumTear++;
          sNumPoints -= 6;
          break;
        case 5:
        case 6:
          if (!fGrenadeLauncher) {
            ubNumFlare++;
            sNumPoints -= 3;
          }
          break;
        case 7:
        case 8:
          ubNumStun++;
          sNumPoints -= 5;
          break;
        case 9:
          if (!fGrenadeLauncher) {
            ubNumMini++;
            sNumPoints -= 7;
          }
          break;
      }
    }
    /*
    if( usNumPoints >= 1 && usNumPoints < 10 )
    { //choose randomly between either stun or tear, (normal with rare chance)
            switch( Random( 10 ) )
            {
                    case 0:
                    case 1:
                    case 2:
                    case 3:
                            ubNumSmoke++;
                            if( usNumPoints > 4 )
                                    usNumPoints -= 4;
                            else
                                    usNumPoints = 0;
                            break;
                    case 4:
                    case 5:
                    case 6:
                            ubNumTear++;
                            if( usNumPoints > 6 )
                                    usNumPoints -= 6;
                            else
                                    usNumPoints = 0;
                            break;
                    case 7:
                    case 8:
                            ubNumStun++;
                            if( usNumPoints > 5 )
                                    usNumPoints -= 5;
                            else
                                    usNumPoints = 0;
                            break;
                    case 9:
                            ubNumMini++;
                            usNumPoints = 0;
                            break;
            }
    }
    */
  }

  // Create the grenades and add them to the soldier

  if (ubNumSmoke) {
    if (fGrenadeLauncher) {
      usItem = GL_SMOKE_GRENADE;
    } else {
      usItem = SMOKE_GRENADE;
    }
    CreateItems(usItem, (int8_t)(ubBaseQuality + Random(ubQualityVariation)), ubNumSmoke, &Object);
    Object.fFlags |= OBJECT_UNDROPPABLE;
    PlaceObjectInSoldierCreateStruct(pp, &Object);
  }
  if (ubNumTear) {
    if (fGrenadeLauncher) {
      usItem = GL_TEARGAS_GRENADE;
    } else {
      usItem = TEARGAS_GRENADE;
    }
    CreateItems(usItem, (int8_t)(ubBaseQuality + Random(ubQualityVariation)), ubNumTear, &Object);
    Object.fFlags |= OBJECT_UNDROPPABLE;
    PlaceObjectInSoldierCreateStruct(pp, &Object);
  }
  if (ubNumStun) {
    if (fGrenadeLauncher) {
      usItem = GL_STUN_GRENADE;
    } else {
      usItem = STUN_GRENADE;
    }
    CreateItems(usItem, (int8_t)(ubBaseQuality + Random(ubQualityVariation)), ubNumStun, &Object);
    Object.fFlags |= OBJECT_UNDROPPABLE;
    PlaceObjectInSoldierCreateStruct(pp, &Object);
  }
  if (ubNumReg) {
    if (fGrenadeLauncher) {
      usItem = GL_HE_GRENADE;
    } else {
      usItem = HAND_GRENADE;
    }
    CreateItems(usItem, (int8_t)(ubBaseQuality + Random(ubQualityVariation)), ubNumReg, &Object);
    Object.fFlags |= OBJECT_UNDROPPABLE;
    PlaceObjectInSoldierCreateStruct(pp, &Object);
  }

  if (ubNumMini) {
    CreateItems(MINI_GRENADE, (int8_t)(ubBaseQuality + Random(ubQualityVariation)), ubNumMini,
                &Object);
    Object.fFlags |= OBJECT_UNDROPPABLE;
    PlaceObjectInSoldierCreateStruct(pp, &Object);
  }
  if (ubNumMustard) {
    CreateItems(MUSTARD_GRENADE, (int8_t)(ubBaseQuality + Random(ubQualityVariation)), ubNumMustard,
                &Object);
    Object.fFlags |= OBJECT_UNDROPPABLE;
    PlaceObjectInSoldierCreateStruct(pp, &Object);
  }
  if (ubNumFlare) {
    CreateItems(BREAK_LIGHT, (int8_t)(ubBaseQuality + Random(ubQualityVariation)), ubNumFlare,
                &Object);
    Object.fFlags |= OBJECT_UNDROPPABLE;
    PlaceObjectInSoldierCreateStruct(pp, &Object);
  }
}

static void ChooseArmourForSoldierCreateStruct(SOLDIERCREATE_STRUCT *pp, int8_t bHelmetClass,
                                               int8_t bVestClass, int8_t bLeggingsClass) {
  uint16_t i;
  uint16_t usRandom;
  uint16_t usNumMatches;
  int8_t bOrigVestClass = bVestClass;
  OBJECTTYPE Object;

  // Choose helmet
  if (bHelmetClass) {
    usNumMatches = 0;
    while (bHelmetClass && !usNumMatches) {  // First step is to count the number of helmets in
                                             // the helmet class range.  If we
      // don't find one, we keep lowering the class until we do.
      for (i = 0; i < MAXITEMS; i++) {
        const INVTYPE *const pItem = &Item[i];
        // NOTE: This relies on treated armor to have a coolness of 0 in order
        // for enemies not to be equipped with it
        if (pItem->usItemClass == IC_ARMOUR && pItem->ubCoolness == bHelmetClass) {
          if (Armour[pItem->ubClassIndex].ubArmourClass == ARMOURCLASS_HELMET) usNumMatches++;
        }
      }
      if (!usNumMatches) bHelmetClass--;
    }
    if (usNumMatches) {  // There is a helmet that we can choose.
      usRandom = (uint16_t)Random(usNumMatches);
      for (i = 0; i < MAXITEMS; i++) {
        const INVTYPE *const pItem = &Item[i];
        if (pItem->usItemClass == IC_ARMOUR && pItem->ubCoolness == bHelmetClass) {
          if (Armour[pItem->ubClassIndex].ubArmourClass == ARMOURCLASS_HELMET) {
            if (usRandom)
              usRandom--;
            else {
              if (!(pp->Inv[HELMETPOS].fFlags & OBJECT_NO_OVERWRITE)) {
                CreateItem(i, (int8_t)(70 + Random(31)), &(pp->Inv[HELMETPOS]));
                pp->Inv[HELMETPOS].fFlags |= OBJECT_UNDROPPABLE;
              }
              break;
            }
          }
        }
      }
    }
  }
  // Choose vest
  if (bVestClass) {
    usNumMatches = 0;
    while (bVestClass && !usNumMatches) {  // First step is to count the number of armours in
                                           // the armour class range.  If we
      // don't find one, we keep lowering the class until we do.
      for (i = 0; i < MAXITEMS; i++) {
        // these 3 have a non-zero coolness, and would otherwise be selected, so
        // skip them
        if ((i == TSHIRT) || (i == LEATHER_JACKET) || (i == TSHIRT_DEIDRANNA)) continue;

        const INVTYPE *const pItem = &Item[i];
        // NOTE: This relies on treated armor to have a coolness of 0 in order
        // for enemies not to be equipped with it
        if (pItem->usItemClass == IC_ARMOUR && pItem->ubCoolness == bVestClass) {
          if (Armour[pItem->ubClassIndex].ubArmourClass == ARMOURCLASS_VEST) usNumMatches++;
        }
      }
      if (!usNumMatches) bVestClass--;
    }
    if (usNumMatches) {  // There is an armour that we can choose.
      usRandom = (uint16_t)Random(usNumMatches);
      for (i = 0; i < MAXITEMS; i++) {
        // these 3 have a non-zero coolness, and would otherwise be selected, so
        // skip them
        if ((i == TSHIRT) || (i == LEATHER_JACKET) || (i == TSHIRT_DEIDRANNA)) continue;

        const INVTYPE *const pItem = &Item[i];
        if (pItem->usItemClass == IC_ARMOUR && pItem->ubCoolness == bVestClass) {
          if (Armour[pItem->ubClassIndex].ubArmourClass == ARMOURCLASS_VEST) {
            if (usRandom)
              usRandom--;
            else {
              if (!(pp->Inv[VESTPOS].fFlags & OBJECT_NO_OVERWRITE)) {
                CreateItem(i, (int8_t)(70 + Random(31)), &(pp->Inv[VESTPOS]));
                pp->Inv[VESTPOS].fFlags |= OBJECT_UNDROPPABLE;

                if ((i == KEVLAR_VEST) || (i == SPECTRA_VEST)) {
                  // roll to see if he gets a CERAMIC PLATES, too.  Higher
                  // chance the higher his entitled vest class is
                  if ((int8_t)Random(100) < (15 * (bOrigVestClass - pItem->ubCoolness))) {
                    CreateItem(CERAMIC_PLATES, (int8_t)(70 + Random(31)), &Object);
                    Object.fFlags |= OBJECT_UNDROPPABLE;
                    AttachObject(NULL, &(pp->Inv[VESTPOS]), &Object);
                  }
                }
              }
              break;
            }
          }
        }
      }
    }
  }
  // Choose Leggings
  if (bLeggingsClass) {
    usNumMatches = 0;
    while (bLeggingsClass && !usNumMatches) {  // First step is to count the number of Armours in
                                               // the Armour class range.  If we
      // don't find one, we keep lowering the class until we do.
      for (i = 0; i < MAXITEMS; i++) {
        const INVTYPE *const pItem = &Item[i];
        // NOTE: This relies on treated armor to have a coolness of 0 in order
        // for enemies not to be equipped with it
        if (pItem->usItemClass == IC_ARMOUR && pItem->ubCoolness == bLeggingsClass) {
          if (Armour[pItem->ubClassIndex].ubArmourClass == ARMOURCLASS_LEGGINGS) usNumMatches++;
        }
      }
      if (!usNumMatches) bLeggingsClass--;
    }
    if (usNumMatches) {  // There is a legging that we can choose.
      usRandom = (uint16_t)Random(usNumMatches);
      for (i = 0; i < MAXITEMS; i++) {
        const INVTYPE *const pItem = &Item[i];
        if (pItem->usItemClass == IC_ARMOUR && pItem->ubCoolness == bLeggingsClass) {
          if (Armour[pItem->ubClassIndex].ubArmourClass == ARMOURCLASS_LEGGINGS) {
            if (usRandom)
              usRandom--;
            else {
              if (!(pp->Inv[LEGPOS].fFlags & OBJECT_NO_OVERWRITE)) {
                CreateItem(i, (int8_t)(70 + Random(31)), &(pp->Inv[LEGPOS]));
                pp->Inv[LEGPOS].fFlags |= OBJECT_UNDROPPABLE;
                break;
              }
            }
          }
        }
      }
    }
  }
}

static void ChooseSpecialWeaponsForSoldierCreateStruct(SOLDIERCREATE_STRUCT *pp, int8_t bKnifeClass,
                                                       BOOLEAN fGrenadeLauncher, BOOLEAN fLAW,
                                                       BOOLEAN fMortar) {
  uint16_t i;
  uint16_t usRandom;
  uint16_t usNumMatches = 0;
  uint16_t usKnifeIndex = 0;
  OBJECTTYPE Object;

  // Choose knife
  while (bKnifeClass && !usNumMatches) {  // First step is to count the number of weapons in the
                                          // weapon class range.  If we
    // don't find one, we keep lowering the class until we do.
    for (i = 0; i < MAXITEMS; i++) {
      const INVTYPE *const pItem = &Item[i];
      if ((pItem->usItemClass == IC_BLADE || pItem->usItemClass == IC_THROWING_KNIFE) &&
          pItem->ubCoolness == bKnifeClass) {
        usNumMatches++;
      }
    }
    if (!usNumMatches) bKnifeClass--;
  }
  if (usNumMatches) {  // There is a knife that we can choose.
    usRandom = (uint16_t)Random(usNumMatches);
    for (i = 0; i < MAXITEMS; i++) {
      const INVTYPE *const pItem = &Item[i];
      if ((pItem->usItemClass == IC_BLADE || pItem->usItemClass == IC_THROWING_KNIFE) &&
          pItem->ubCoolness == bKnifeClass) {
        if (usRandom)
          usRandom--;
        else {
          usKnifeIndex = i;
          break;
        }
      }
    }
  }

  if (usKnifeIndex) {
    CreateItem(usKnifeIndex, (int8_t)(70 + Random(31)), &Object);
    Object.fFlags |= OBJECT_UNDROPPABLE;
    PlaceObjectInSoldierCreateStruct(pp, &Object);
  }

  if (fGrenadeLauncher) {
    // give grenade launcher
    CreateItem(GLAUNCHER, (int8_t)(50 + Random(51)), &Object);
    Object.fFlags |= OBJECT_UNDROPPABLE;
    PlaceObjectInSoldierCreateStruct(pp, &Object);
  }

  if (fLAW) {
    // give rocket launcher
    CreateItem(ROCKET_LAUNCHER, (int8_t)(50 + Random(51)), &Object);
    Object.fFlags |= OBJECT_UNDROPPABLE;
    PlaceObjectInSoldierCreateStruct(pp, &Object);
  }

  if (fMortar) {
    // make sure we're not distributing them underground!
    Assert(IsAutoResolveActive() || (gbWorldSectorZ == 0));

    // give mortar
    CreateItem(MORTAR, (int8_t)(50 + Random(51)), &Object);
    Object.fFlags |= OBJECT_UNDROPPABLE;
    PlaceObjectInSoldierCreateStruct(pp, &Object);
  }
}

static void ChooseFaceGearForSoldierCreateStruct(SOLDIERCREATE_STRUCT *pp) {
  int32_t i;
  int8_t bDifficultyRating = CalcDifficultyModifier(pp->ubSoldierClass);

  if (gWorldSectorX == TIXA_SECTOR_X && gWorldSectorY == TIXA_SECTOR_Y &&
      StrategicMap[TIXA_SECTOR_X + TIXA_SECTOR_Y * MAP_WORLD_X]
          .fEnemyControlled) {  // Tixa is a special case that is handled
                                // differently.
    return;
  }

  // Look for any face item in the big pocket positions (the only place they can
  // be added in the editor) If any are found, then don't assign any
  for (i = BIGPOCK1POS; i < BIGPOCK4POS; i++) {
    if (Item[pp->Inv[i].usItem].usItemClass == IC_FACE) {
      return;
    }
  }

  // KM: (NEW)
  // Note the lack of overwritable item checking here.  This is because
  // faceitems are not supported in the editor, hence they can't have this
  // status.
  switch (pp->ubSoldierClass) {
    case SOLDIER_CLASS_ELITE:
    case SOLDIER_CLASS_ELITE_MILITIA:
      // All elites get a gasmask and either night goggles or uv goggles.
      if (Chance(75)) {
        CreateItem(GASMASK, (int8_t)(70 + Random(31)), &(pp->Inv[HEAD1POS]));
        pp->Inv[HEAD1POS].fFlags |= OBJECT_UNDROPPABLE;
      } else {
        CreateItem(EXTENDEDEAR, (int8_t)(70 + Random(31)), &(pp->Inv[HEAD1POS]));
        pp->Inv[HEAD1POS].fFlags |= OBJECT_UNDROPPABLE;
      }
      if (Chance(75)) {
        CreateItem(NIGHTGOGGLES, (int8_t)(70 + Random(31)), &(pp->Inv[HEAD2POS]));
        pp->Inv[HEAD2POS].fFlags |= OBJECT_UNDROPPABLE;
      } else {
        CreateItem(UVGOGGLES, (int8_t)(70 + Random(31)), &(pp->Inv[HEAD2POS]));
        pp->Inv[HEAD2POS].fFlags |= OBJECT_UNDROPPABLE;
      }
      break;
    case SOLDIER_CLASS_ARMY:
    case SOLDIER_CLASS_REG_MILITIA:
      if (Chance(bDifficultyRating / 2)) {  // chance of getting a face item
        if (Chance(50)) {
          CreateItem(GASMASK, (int8_t)(70 + Random(31)), &(pp->Inv[HEAD1POS]));
          pp->Inv[HEAD1POS].fFlags |= OBJECT_UNDROPPABLE;
        } else {
          CreateItem(NIGHTGOGGLES, (int8_t)(70 + Random(31)), &(pp->Inv[HEAD1POS]));
          pp->Inv[HEAD1POS].fFlags |= OBJECT_UNDROPPABLE;
        }
      }
      if (Chance(bDifficultyRating / 3)) {  // chance of getting a extended ear
        CreateItem(EXTENDEDEAR, (int8_t)(70 + Random(31)), &(pp->Inv[HEAD2POS]));
        pp->Inv[HEAD2POS].fFlags |= OBJECT_UNDROPPABLE;
      }
      break;
    case SOLDIER_CLASS_ADMINISTRATOR:
    case SOLDIER_CLASS_GREEN_MILITIA:
      break;
  }
}

static void ChooseKitsForSoldierCreateStruct(SOLDIERCREATE_STRUCT *pp, int8_t bKitClass) {
  uint16_t i;
  uint16_t usRandom;
  uint16_t usNumMatches = 0;
  OBJECTTYPE Object;
  uint16_t usKitItem = 0;

  // we want these mostly to be first aid and medical kits, and for those kit
  // class doesn't matter, they're always useful
  if (Chance(50)) {
    usKitItem = FIRSTAIDKIT;
  } else if (Chance(25)) {
    usKitItem = MEDICKIT;
  } else {
    // count how many non-medical KITS are eligible ( of sufficient or lower
    // coolness)
    for (i = 0; i < MAXITEMS; i++) {
      const INVTYPE *const pItem = &Item[i];
      // skip toolkits
      if ((pItem->usItemClass == IC_KIT) && (pItem->ubCoolness > 0) &&
          pItem->ubCoolness <= bKitClass && (i != TOOLKIT)) {
        usNumMatches++;
      }
    }

    // if any are eligible, pick one of them at random
    if (usNumMatches) {
      usRandom = (uint16_t)Random(usNumMatches);
      for (i = 0; i < MAXITEMS; i++) {
        const INVTYPE *const pItem = &Item[i];
        // skip toolkits
        if ((pItem->usItemClass == IC_KIT) && (pItem->ubCoolness > 0) &&
            pItem->ubCoolness <= bKitClass && (i != TOOLKIT)) {
          if (usRandom)
            usRandom--;
          else {
            usKitItem = i;
            break;
          }
        }
      }
    }
  }

  if (usKitItem != 0) {
    CreateItem(usKitItem, (int8_t)(80 + Random(21)), &Object);
    Object.fFlags |= OBJECT_UNDROPPABLE;
    PlaceObjectInSoldierCreateStruct(pp, &Object);
  }
}

static void ChooseMiscGearForSoldierCreateStruct(SOLDIERCREATE_STRUCT *pp, int8_t bMiscClass) {
  // not all of these are IC_MISC, some are IC_PUNCH (not covered anywhere else)
  static const int32_t iMiscItemsList[] = {CANTEEN,
                                           CANTEEN,
                                           CANTEEN,
                                           CANTEEN,
                                           ALCOHOL,
                                           ALCOHOL,
                                           ADRENALINE_BOOSTER,
                                           ADRENALINE_BOOSTER,
                                           REGEN_BOOSTER,
                                           BRASS_KNUCKLES,
                                           CHEWING_GUM,
                                           CIGARS,
                                           GOLDWATCH,
                                           NOTHING};

  // count how many are eligible
  uint16_t usNumMatches = 0;
  for (const int32_t *i = iMiscItemsList; *i != NOTHING; ++i) {
    const INVTYPE *const pItem = &Item[*i];
    if (pItem->ubCoolness > 0 && pItem->ubCoolness <= bMiscClass &&
        (*i != REGEN_BOOSTER ||
         gGameOptions.fSciFi))  // exclude REGEN_BOOSTERs unless Sci-Fi flag is on
    {
      ++usNumMatches;
    }
  }

  // if any are eligible, pick one of them at random
  if (usNumMatches == 0) return;

  uint16_t usRandom = Random(usNumMatches);
  for (const int32_t *i = iMiscItemsList; *i != NOTHING; ++i) {
    const INVTYPE *const pItem = &Item[*i];
    if (pItem->ubCoolness > 0 && pItem->ubCoolness <= bMiscClass &&
        (*i != REGEN_BOOSTER ||
         gGameOptions.fSciFi))  // exclude REGEN_BOOSTERs unless Sci-Fi flag is on
    {
      if (usRandom) {
        --usRandom;
      } else {
        OBJECTTYPE Object;
        CreateItem(*i, 80 + Random(21), &Object);
        Object.fFlags |= OBJECT_UNDROPPABLE;
        PlaceObjectInSoldierCreateStruct(pp, &Object);
        break;
      }
    }
  }
}

static void ChooseBombsForSoldierCreateStruct(SOLDIERCREATE_STRUCT *pp, int8_t bBombClass) {
  uint16_t i;
  uint16_t usRandom;
  uint16_t usNumMatches = 0;
  OBJECTTYPE Object;

  // count how many are eligible
  for (i = 0; i < MAXITEMS; i++) {
    const INVTYPE *const pItem = &Item[i];
    if ((pItem->usItemClass == IC_BOMB) && (pItem->ubCoolness > 0) &&
        (pItem->ubCoolness <= bBombClass)) {
      usNumMatches++;
    }
  }

  // if any are eligible, pick one of them at random
  if (usNumMatches) {
    usRandom = (uint16_t)Random(usNumMatches);
    for (i = 0; i < MAXITEMS; i++) {
      const INVTYPE *const pItem = &Item[i];
      if ((pItem->usItemClass == IC_BOMB) && (pItem->ubCoolness > 0) &&
          (pItem->ubCoolness <= bBombClass)) {
        if (usRandom)
          usRandom--;
        else {
          CreateItem(i, (int8_t)(80 + Random(21)), &Object);
          Object.fFlags |= OBJECT_UNDROPPABLE;
          PlaceObjectInSoldierCreateStruct(pp, &Object);
          break;
        }
      }
    }
  }
}

static void ChooseLocationSpecificGearForSoldierCreateStruct(SOLDIERCREATE_STRUCT *pp) {
  OBJECTTYPE Object;

  // If this is Tixa and the player doesn't control Tixa then give all enemies
  // gas masks, but somewhere on their person, not in their face positions
  if (gWorldSectorX == TIXA_SECTOR_X && gWorldSectorY == TIXA_SECTOR_Y &&
      StrategicMap[TIXA_SECTOR_X + TIXA_SECTOR_Y * MAP_WORLD_X].fEnemyControlled) {
    CreateItem(GASMASK, (int8_t)(95 + Random(6)), &Object);
    PlaceObjectInSoldierCreateStruct(pp, &Object);
  }
}

static BOOLEAN PlaceObjectInSoldierCreateStruct(SOLDIERCREATE_STRUCT *pp, OBJECTTYPE *pObject) {
  int8_t i;
  if (!Item[pObject->usItem].ubPerPocket) {  // ubPerPocket == 0 will only fit in large pockets.
    pObject->ubNumberOfObjects = 1;
    for (i = BIGPOCK1POS; i <= BIGPOCK4POS; i++) {
      if (!(pp->Inv[i].usItem) && !(pp->Inv[i].fFlags & OBJECT_NO_OVERWRITE)) {
        pp->Inv[i] = *pObject;
        return TRUE;
      }
    }
    return FALSE;
  } else {
    pObject->ubNumberOfObjects =
        (uint8_t)std::min(Item[pObject->usItem].ubPerPocket, pObject->ubNumberOfObjects);
    // try to get it into a small pocket first
    for (i = SMALLPOCK1POS; i <= SMALLPOCK8POS; i++) {
      if (!(pp->Inv[i].usItem) && !(pp->Inv[i].fFlags & OBJECT_NO_OVERWRITE)) {
        pp->Inv[i] = *pObject;
        return TRUE;
      }
    }
    for (i = BIGPOCK1POS; i <= BIGPOCK4POS; i++) {  // no space free in small pockets, so put it
                                                    // into a large pocket.
      if (!(pp->Inv[i].usItem) && !(pp->Inv[i].fFlags & OBJECT_NO_OVERWRITE)) {
        pp->Inv[i] = *pObject;
        return TRUE;
      }
    }
  }
  return FALSE;
}

static void MakeOneItemOfClassDroppable(SOLDIERCREATE_STRUCT *const sc, uint32_t const item_class) {
  /* XXX TODO001B: OBJECT_NO_OVERWRITE test should probably continue instead of
   * break, but it was this way in the original code.  This is even more
   * plausible, because the OBJECT_NO_OVERWRITE condition in the second loop
   * never can be true in the current configuration.  A comment below says that
   * no object of that class should be dropped if any has this flag set, but the
   * code did not do this. */
  uint8_t n_matches = 0;
  for (int32_t i = 0; i != NUM_INV_SLOTS; ++i) {
    OBJECTTYPE const &o = sc->Inv[i];
    if (!(Item[o.usItem].usItemClass & item_class)) continue;
    if (o.fFlags & OBJECT_NO_OVERWRITE) break;
    ++n_matches;
  }
  if (n_matches == 0) return;

  for (int32_t i = 0; i != NUM_INV_SLOTS; ++i) {
    OBJECTTYPE &o = sc->Inv[i];
    if (!(Item[o.usItem].usItemClass & item_class)) continue;
    if (o.fFlags & OBJECT_NO_OVERWRITE) break;
    if (Random(n_matches--) != 0) continue;
    o.fFlags &= ~OBJECT_UNDROPPABLE;
    break;
  }
}

static void RandomlyChooseWhichItemsAreDroppable(SOLDIERCREATE_STRUCT *pp, int8_t bSoldierClass) {
  int32_t i;
  //	uint16_t usRandomNum;
  uint32_t uiItemClass;
  uint16_t usItem;
  uint8_t ubAmmoDropRate;
  uint8_t ubGrenadeDropRate;
  uint8_t ubOtherDropRate;
  BOOLEAN fWeapon = FALSE;
  BOOLEAN fGrenades = FALSE;  // this includes all  grenades!
  BOOLEAN fAmmo = FALSE;
  BOOLEAN fArmour = FALSE;
  BOOLEAN fKnife = FALSE;
  BOOLEAN fKit = FALSE;
  BOOLEAN fFace = FALSE;
  BOOLEAN fMisc = FALSE;

  /*
          //40% of soldiers will have droppable items.
          usRandomNum = (uint16_t)Random( 1000 );
          if( usRandomNum >= 400 )
                  return;
          //so now the number is 0-399.  This is kind of like a D&D die roll
     where
          //various numbers drop different items, or even more than one item. At
     this
          //point, we don't care if the enemy has anything in the slot that is
     made droppable.
          //Any items containing the OBJECT_NO_OVERWRITE slot is rejected for
     droppable
          //consideration.

          if( usRandomNum < 32 ) //3.2% of dead bodies present the possibility
     of several items (0-5 items : avg 3). { //31 is the magic number that
     allows all 5 item classes to be dropped! if( usRandomNum & 16 ) fWeapon =
     TRUE; if( usRandomNum & 8 ) fAmmo = TRUE; if( usRandomNum & 4 ) fGrenades =
     TRUE; if( usRandomNum & 2 ) fArmour = TRUE; if( usRandomNum & 1 ) fMisc =
     TRUE;
          }
          else if( usRandomNum < 100 ) //6.8% chance of getting 2-3 different
     items. { //do a more generalized approach to dropping items. switch(
     usRandomNum / 10 )
                  {
                          case 3:	fWeapon = TRUE;
     fAmmo = TRUE;
     break; case 4:	fWeapon = TRUE;	fGrenades = TRUE;
     break;
                          case 5:
     fGrenades = TRUE;
     fMisc = TRUE;	break; case 6:
     fGrenades = TRUE;								fArmour = TRUE;
     break; case 7:
     fAmmo = TRUE;	fArmour = TRUE;
     break; case 8:
     fAmmo = TRUE;	fArmour = TRUE;	fMisc = TRUE;	break; case 9:
     fGrenades = TRUE;	fAmmo = TRUE;
     fMisc = TRUE; break;
                  }
          }
          else
          {
                  switch( usRandomNum / 50 ) //30% chance of getting 1-2 items
     (no weapons)
                  {
                          case 2:
     fGrenades = TRUE;
     break; case 3:
     fAmmo = TRUE;
     break; case 4:
     fArmour = TRUE;
     break; case 5:
     fMisc = TRUE;	break;
                          case 6:
     fAmmo = TRUE;									fMisc =
     TRUE; break; case 7: fGrenades = TRUE;	fAmmo = TRUE; break;
                  }
          }

          fKnife = (Random(3)) ? FALSE : TRUE;
  */

  // only enemy soldiers use auto-drop system!
  // don't use the auto-drop system in auto-resolve: player won't see what's
  // being used & enemies will often win & keep'em
  if (SOLDIER_CLASS_ENEMY(bSoldierClass) && !IsAutoResolveActive()) {
    // SPECIAL handling for weapons: we'll always drop a weapon type that has
    // never been dropped before
    for (i = 0; i < NUM_INV_SLOTS; i++) {
      usItem = pp->Inv[i].usItem;
      // if it's a weapon (monster parts included - they won't drop due to
      // checks elsewhere!)
      if ((usItem > NONE) && (usItem < MAX_WEAPONS)) {
        // and we're allowed to change its flags
        if (!(pp->Inv[i].fFlags & OBJECT_NO_OVERWRITE)) {
          // and it's never been dropped before in this game
          if (!gStrategicStatus.fWeaponDroppedAlready[usItem]) {
            // mark it as droppable, and remember we did so.  If the player
            // never kills this particular dude, oh well, tough luck, he missed
            // his chance for an easy reward, he'll have to wait til next time
            // and need some luck...
            pp->Inv[i].fFlags &= ~OBJECT_UNDROPPABLE;

            MarkAllWeaponsOfSameGunClassAsDropped(usItem);
          }
        }
      }
    }
  }

  if (SOLDIER_CLASS_MILITIA(bSoldierClass)) {
    // militia (they drop much less stuff)
    ubAmmoDropRate = MILITIAAMMODROPRATE;
    ubGrenadeDropRate = MILITIAGRENADEDROPRATE;
    ubOtherDropRate = MILITIAEQUIPDROPRATE;
  } else {
    // enemy army
    ubAmmoDropRate = ENEMYAMMODROPRATE;
    ubGrenadeDropRate = ENEMYGRENADEDROPRATE;
    ubOtherDropRate = ENEMYEQUIPDROPRATE;
  }

  if (Random(100) < ubAmmoDropRate) fAmmo = TRUE;

  if (Random(100) < ubOtherDropRate) fWeapon = TRUE;

  if (Random(100) < ubOtherDropRate) fArmour = TRUE;

  if (Random(100) < ubOtherDropRate) fKnife = TRUE;

  if (Random(100) < ubGrenadeDropRate) fGrenades = TRUE;

  if (Random(100) < ubOtherDropRate) fKit = TRUE;

  if (Random(100) < (uint32_t)(ubOtherDropRate / 3)) fFace = TRUE;

  if (Random(100) < ubOtherDropRate) fMisc = TRUE;

  // Now, that the flags are set for each item, we now have to search through
  // the item slots to see if we can find a matching item, however, if we find
  // any items in a particular class that have the OBJECT_NO_OVERWRITE flag set,
  // we will not make any items droppable for that class because the editor would
  // have specified it already.
  if (fAmmo) {
    // now drops ALL ammo found, not just the first slot
    for (i = 0; i < NUM_INV_SLOTS; i++) {
      uiItemClass = Item[pp->Inv[i].usItem].usItemClass;
      if (uiItemClass == IC_AMMO) {
        if (pp->Inv[i].fFlags & OBJECT_NO_OVERWRITE)
          continue;
        else {
          pp->Inv[i].fFlags &= ~OBJECT_UNDROPPABLE;
        }
      }
    }
  }

  if (fWeapon) MakeOneItemOfClassDroppable(pp, IC_LAUNCHER | IC_GUN);
  if (fArmour) MakeOneItemOfClassDroppable(pp, IC_ARMOUR);

  if (fKnife) {
    for (i = 0; i < NUM_INV_SLOTS; i++) {
      // drops FIRST knife found
      uiItemClass = Item[pp->Inv[i].usItem].usItemClass;
      if (uiItemClass == IC_BLADE || uiItemClass == IC_THROWING_KNIFE) {
        if (pp->Inv[i].fFlags & OBJECT_NO_OVERWRITE)
          break;
        else {
          pp->Inv[i].fFlags &= ~OBJECT_UNDROPPABLE;
          break;
        }
      }
    }
  }

  // note that they'll only drop ONE TYPE of grenade if they have multiple types
  // (very common)
  if (fGrenades) MakeOneItemOfClassDroppable(pp, IC_GRENADE);
  if (fKit) MakeOneItemOfClassDroppable(pp, IC_KIT | IC_MEDKIT);
  if (fFace) MakeOneItemOfClassDroppable(pp, IC_FACE);
  if (fMisc) MakeOneItemOfClassDroppable(pp, IC_MISC);
}

void AssignCreatureInventory(SOLDIERTYPE *pSoldier) {
  uint32_t uiChanceToDrop = 0;
  BOOLEAN fMaleCreature = FALSE;
  BOOLEAN fBloodcat = FALSE;

  // all creature items in this first section are only offensive/defensive
  // placeholders, and never get dropped, because they're not real items!
  switch (pSoldier->ubBodyType) {
    case ADULTFEMALEMONSTER:
      CreateItem(CREATURE_OLD_FEMALE_CLAWS, 100, &(pSoldier->inv[HANDPOS]));
      CreateItem(CREATURE_OLD_FEMALE_HIDE, 100, &(pSoldier->inv[HELMETPOS]));
      CreateItem(CREATURE_OLD_FEMALE_HIDE, 100, &(pSoldier->inv[VESTPOS]));
      CreateItem(CREATURE_OLD_FEMALE_HIDE, 100, &(pSoldier->inv[LEGPOS]));
      uiChanceToDrop = 30;
      break;
    case AM_MONSTER:
      CreateItem(CREATURE_OLD_MALE_CLAWS, 100, &(pSoldier->inv[HANDPOS]));
      CreateItem(CREATURE_OLD_MALE_SPIT, 100, &(pSoldier->inv[SECONDHANDPOS]));
      CreateItem(CREATURE_OLD_MALE_HIDE, 100, &(pSoldier->inv[HELMETPOS]));
      CreateItem(CREATURE_OLD_MALE_HIDE, 100, &(pSoldier->inv[VESTPOS]));
      CreateItem(CREATURE_OLD_MALE_HIDE, 100, &(pSoldier->inv[LEGPOS]));
      uiChanceToDrop = 30;
      fMaleCreature = TRUE;
      break;
    case YAF_MONSTER:
      CreateItem(CREATURE_YOUNG_FEMALE_CLAWS, 100, &(pSoldier->inv[HANDPOS]));
      CreateItem(CREATURE_YOUNG_FEMALE_HIDE, 100, &(pSoldier->inv[HELMETPOS]));
      CreateItem(CREATURE_YOUNG_FEMALE_HIDE, 100, &(pSoldier->inv[VESTPOS]));
      CreateItem(CREATURE_YOUNG_FEMALE_HIDE, 100, &(pSoldier->inv[LEGPOS]));
      uiChanceToDrop = 15;
      break;
    case YAM_MONSTER:
      CreateItem(CREATURE_YOUNG_MALE_CLAWS, 100, &(pSoldier->inv[HANDPOS]));
      CreateItem(CREATURE_YOUNG_MALE_SPIT, 100, &(pSoldier->inv[SECONDHANDPOS]));
      CreateItem(CREATURE_YOUNG_MALE_HIDE, 100, &(pSoldier->inv[HELMETPOS]));
      CreateItem(CREATURE_YOUNG_MALE_HIDE, 100, &(pSoldier->inv[VESTPOS]));
      CreateItem(CREATURE_YOUNG_MALE_HIDE, 100, &(pSoldier->inv[LEGPOS]));
      uiChanceToDrop = 15;
      fMaleCreature = TRUE;
      break;
    case INFANT_MONSTER:
      CreateItem(CREATURE_INFANT_SPIT, 100, &(pSoldier->inv[HANDPOS]));
      CreateItem(CREATURE_INFANT_HIDE, 100, &(pSoldier->inv[HELMETPOS]));
      CreateItem(CREATURE_INFANT_HIDE, 100, &(pSoldier->inv[VESTPOS]));
      CreateItem(CREATURE_INFANT_HIDE, 100, &(pSoldier->inv[LEGPOS]));
      uiChanceToDrop = 5;
      break;
    case LARVAE_MONSTER:
      uiChanceToDrop = 0;
      break;
    case QUEENMONSTER:
      CreateItem(CREATURE_QUEEN_SPIT, 100, &(pSoldier->inv[HANDPOS]));
      CreateItem(CREATURE_QUEEN_TENTACLES, 100, &(pSoldier->inv[SECONDHANDPOS]));
      CreateItem(CREATURE_QUEEN_HIDE, 100, &(pSoldier->inv[HELMETPOS]));
      CreateItem(CREATURE_QUEEN_HIDE, 100, &(pSoldier->inv[VESTPOS]));
      CreateItem(CREATURE_QUEEN_HIDE, 100, &(pSoldier->inv[LEGPOS]));
      // she can't drop anything, because the items are unreachable anyways (she's
      // too big!)
      uiChanceToDrop = 0;
      break;
    case BLOODCAT:
      CreateItem(BLOODCAT_CLAW_ATTACK, 100, &(pSoldier->inv[HANDPOS]));
      CreateItem(BLOODCAT_BITE, 100, &(pSoldier->inv[SECONDHANDPOS]));
      fBloodcat = TRUE;
      uiChanceToDrop = 30;
      break;

    default:
      AssertMsg(FALSE, String("Invalid creature bodytype %d", pSoldier->ubBodyType));
      return;
  }

  // decide if the creature will drop any REAL bodyparts
  if (Random(100) < uiChanceToDrop) {
    CreateItem((uint16_t)(fBloodcat ? BLOODCAT_CLAWS : CREATURE_PART_CLAWS),
               (int8_t)(80 + Random(21)), &(pSoldier->inv[BIGPOCK1POS]));
  }

  if (Random(100) < uiChanceToDrop) {
    CreateItem((uint16_t)(fBloodcat ? BLOODCAT_TEETH : CREATURE_PART_FLESH),
               (int8_t)(80 + Random(21)), &(pSoldier->inv[BIGPOCK2POS]));
  }

  // as requested by ATE, males are more likely to drop their "organs" (he
  // actually suggested this, I'm serious!)
  if (fMaleCreature) {
    // increase chance by 50%
    uiChanceToDrop += (uiChanceToDrop / 2);
  }

  if (Random(100) < uiChanceToDrop) {
    CreateItem((uint16_t)(fBloodcat ? BLOODCAT_PELT : CREATURE_PART_ORGAN),
               (int8_t)(80 + Random(21)), &(pSoldier->inv[BIGPOCK3POS]));
  }
}

void ReplaceExtendedGuns(SOLDIERCREATE_STRUCT *pp, int8_t bSoldierClass) {
  uint32_t uiLoop, uiLoop2, uiAttachDestIndex;
  int8_t bWeaponClass;
  OBJECTTYPE OldObj;
  uint16_t usItem, usNewGun, usAmmo, usNewAmmo;

  for (uiLoop = 0; uiLoop < NUM_INV_SLOTS; uiLoop++) {
    usItem = pp->Inv[uiLoop].usItem;

    if ((Item[usItem].usItemClass & IC_GUN) && ExtendedGunListGun(usItem)) {
      if (bSoldierClass == SOLDIER_CLASS_NONE) {
        usNewGun = StandardGunListReplacement(usItem);
      } else {
        bWeaponClass = GetWeaponClass(usItem);
        AssertMsg(bWeaponClass != -1,
                  String("Gun %d does not have a match in the extended gun array", usItem));
        usNewGun = SelectStandardArmyGun(bWeaponClass);
      }

      if (usNewGun != NOTHING) {
        // have to replace!  but first (ugh) must store backup (b/c of
        // attachments)
        OldObj = pp->Inv[uiLoop];
        CreateItem(usNewGun, OldObj.bGunStatus, &(pp->Inv[uiLoop]));
        pp->Inv[uiLoop].fFlags = OldObj.fFlags;

        // copy any valid attachments; for others, just drop them...
        if (ItemHasAttachments(OldObj)) {
          // we're going to copy into the first attachment position first :-)
          uiAttachDestIndex = 0;
          // loop!
          for (uiLoop2 = 0; uiLoop2 < MAX_ATTACHMENTS; uiLoop2++) {
            if ((OldObj.usAttachItem[uiLoop2] != NOTHING) &&
                ValidAttachment(OldObj.usAttachItem[uiLoop2], usNewGun)) {
              pp->Inv[uiLoop].usAttachItem[uiAttachDestIndex] = OldObj.usAttachItem[uiLoop2];
              pp->Inv[uiLoop].bAttachStatus[uiAttachDestIndex] = OldObj.bAttachStatus[uiLoop2];
              uiAttachDestIndex++;
            }
          }
        }

        // must search through inventory and replace ammo accordingly
        for (uiLoop2 = 0; uiLoop2 < NUM_INV_SLOTS; uiLoop2++) {
          usAmmo = pp->Inv[uiLoop2].usItem;
          if ((Item[usAmmo].usItemClass & IC_AMMO)) {
            usNewAmmo = FindReplacementMagazineIfNecessary(usItem, usAmmo, usNewGun);
            if (usNewAmmo != NOTHING) {
              // found a new magazine, replace...
              CreateItems(usNewAmmo, 100, pp->Inv[uiLoop2].ubNumberOfObjects, &(pp->Inv[uiLoop2]));
            }
          }
        }
      }
    }
  }
}

static uint16_t SelectStandardArmyGun(uint8_t uiGunLevel) {
  uint32_t uiChoice;
  uint16_t usGunIndex;

  // pick the standard army gun for this weapon class from table
  //	usGunIndex = gStrategicStatus.ubStandardArmyGunIndex[uiGunLevel];

  // decided to randomize it afterall instead of repeating the same weapon over
  // and over

  // depending on selection of the gun nut option
  const ARMY_GUN_CHOICE_TYPE *pGunChoiceTable;
  if (gGameOptions.fGunNut) {
    // use table of extended gun choices
    pGunChoiceTable = gExtendedArmyGunChoices;
  } else {
    // use table of regular gun choices
    pGunChoiceTable = gRegularArmyGunChoices;
  }

  // choose one the of the possible gun choices
  uiChoice = Random(pGunChoiceTable[uiGunLevel].ubChoices);
  usGunIndex = pGunChoiceTable[uiGunLevel].bItemNo[uiChoice];

  Assert(usGunIndex);

  return (usGunIndex);
}

static void EquipTank(SOLDIERCREATE_STRUCT *pp) {
  OBJECTTYPE Object;

  // tanks get special equipment, and they drop nothing (MGs are hard-mounted &
  // non-removable)

  // main cannon
  CreateItem(TANK_CANNON, (int8_t)(80 + Random(21)), &(pp->Inv[HANDPOS]));
  pp->Inv[HANDPOS].fFlags |= OBJECT_UNDROPPABLE;

  // machine gun
  CreateItems(MINIMI, (int8_t)(80 + Random(21)), 1, &Object);
  Object.fFlags |= OBJECT_UNDROPPABLE;
  PlaceObjectInSoldierCreateStruct(pp, &Object);

  // tanks don't deplete shells or ammo...
  CreateItems(TANK_SHELL, 100, 1, &Object);
  Object.fFlags |= OBJECT_UNDROPPABLE;
  PlaceObjectInSoldierCreateStruct(pp, &Object);

  // armour equal to spectra all over (for vs explosives)
  CreateItem(SPECTRA_VEST, 100, &(pp->Inv[VESTPOS]));
  pp->Inv[VESTPOS].fFlags |= OBJECT_UNDROPPABLE;
  CreateItem(SPECTRA_HELMET, 100, &(pp->Inv[HELMETPOS]));
  pp->Inv[HELMETPOS].fFlags |= OBJECT_UNDROPPABLE;
  CreateItem(SPECTRA_LEGGINGS, 100, &(pp->Inv[LEGPOS]));
  pp->Inv[LEGPOS].fFlags |= OBJECT_UNDROPPABLE;
}

void ResetMortarsOnTeamCount() { guiMortarsRolledByTeam = 0; }
