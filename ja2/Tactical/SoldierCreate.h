// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef _SOLDIER_CREATE_H
#define _SOLDIER_CREATE_H

#include "JA2Types.h"
#include "Tactical/ItemTypes.h"
#include "Tactical/OverheadTypes.h"
#include "Tactical/SoldierControl.h"

// Kris:  SERIALIZING INFORMATION
// All maps must have:
//	-MAPCREATE_STRUCT
//		MAPCREATE_STRUCT.ubNumIndividuals determines how many
// BASIC_SOLDIERCREATE_STRUCTs there are
//  -The BASIC_SOLDIERCREATE_STRUCTS are saved contiguously, but if any of them
//		fDetailedPlacement set, then there is a SOLDIERCREATE_STRUCT saved
// immediately after.

// These are the placement slots used by the editor to define where characters
// are in a map, what they are, what team they are on, personality traits, etc.
// The Merc section of the editor is what is used to define these values.
struct BASIC_SOLDIERCREATE_STRUCT {
  uint16_t usStartingGridNo;   // Where the placement position is.
  BOOLEAN fDetailedPlacement;  // Specialized information.  Has a counterpart
                               // containing all info.
  int8_t bTeam;                // The team this individual is part of.
  int8_t bRelativeAttributeLevel;
  int8_t bRelativeEquipmentLevel;
  int8_t bDirection;  // 1 of 8 values (always mandatory)
  int8_t bOrders;
  int8_t bAttitude;
  int8_t bBodyType;                     // up to 128 body types, -1 means random
  int16_t sPatrolGrid[MAXPATROLGRIDS];  // possible locations to visit, patrol,
                                        // etc.
  int8_t bPatrolCnt;
  BOOLEAN fOnRoof;
  uint8_t ubSoldierClass;  // army, administrator, elite
  uint8_t ubCivilianGroup;
  BOOLEAN fPriorityExistance;  // These slots are used first
  BOOLEAN fHasKeys;
};

struct SOLDIERCREATE_STRUCT {
  // Bulletproofing so static detailed placements aren't used to tactically
  // create soldiers. Used by editor for validation purposes.
  BOOLEAN fStatic;

  // Profile information used for special NPCs and player mercs.
  uint8_t ubProfile;
  BOOLEAN fCopyProfileItemsOver;

  // Location information
  int16_t sSectorX;
  int16_t sSectorY;
  int8_t bDirection;
  int16_t sInsertionGridNo;

  // Can force a team, but needs flag set
  int8_t bTeam;
  int8_t bBodyType;

  // Orders and attitude settings
  int8_t bAttitude;
  int8_t bOrders;

  // Attributes
  int8_t bLifeMax;
  int8_t bLife;
  int8_t bAgility;
  int8_t bDexterity;
  int8_t bExpLevel;
  int8_t bMarksmanship;
  int8_t bMedical;
  int8_t bMechanical;
  int8_t bExplosive;
  int8_t bLeadership;
  int8_t bStrength;
  int8_t bWisdom;
  int8_t bMorale;
  int8_t bAIMorale;

  // Inventory
  OBJECTTYPE Inv[NUM_INV_SLOTS];

  // Palette information for soldiers.
  PaletteRepID HeadPal;
  PaletteRepID PantsPal;
  PaletteRepID VestPal;
  PaletteRepID SkinPal;

  // Waypoint information for patrolling
  int16_t sPatrolGrid[MAXPATROLGRIDS];
  int8_t bPatrolCnt;

  // Kris:  Additions November 16, 1997 (padding down to 129 from 150)
  BOOLEAN fVisible;
  wchar_t name[10];

  uint8_t ubSoldierClass;  // army, administrator, elite

  BOOLEAN fOnRoof;

  int8_t bSectorZ;

  uint8_t ubCivilianGroup;

  uint8_t ubScheduleID;

  BOOLEAN fUseGivenVehicle;
  int8_t bUseGivenVehicleID;
  BOOLEAN fHasKeys;
};

// Original functions currently used throughout the game.
void TacticalRemoveSoldier(SOLDIERTYPE &);

int8_t CalcDifficultyModifier(uint8_t ubSoldierClass);

void RandomizeNewSoldierStats(SOLDIERCREATE_STRUCT *pCreateStruct);

// Kris:
// Modified return type from BOOLEAN to SOLDIERTYPE*
SOLDIERTYPE *TacticalCreateSoldier(SOLDIERCREATE_STRUCT const &);
SOLDIERTYPE *TacticalCreateSoldierFromExisting(const SOLDIERTYPE *);

// Randomly generated enemies used by strategic AI.
SOLDIERTYPE *TacticalCreateEnemySoldier(SoldierClass);
SOLDIERTYPE *TacticalCreateMilitia(uint8_t ubMilitiaClass);
SOLDIERTYPE *TacticalCreateCreature(int8_t bCreatureBodyType);

// randomly generates a relative level rating (attributes or equipment)
void RandomizeRelativeLevel(int8_t *pbRelLevel, uint8_t ubSoldierClass);

// get the pythag. distance from the passed sector to the palace..
uint8_t GetPythDistanceFromPalace(int16_t sSectorX, int16_t sSectorY);

// These following functions are currently used exclusively by the editor.
// Now, this will be useful for the strategic AI.
// Definitions:
// Soldier (s):	Currently in the game, but subject to modifications.  The editor
// has the capability to
//  modify soldier attributes on the fly for testing purposes.
// BasicPlacement (bp):  A BASIC_SOLDIERCREATE_STRUCT that contains compact,
// very general, information about
//  a soldier.  The BasicPlacement is then used to generate a DetailedPlacement
//  before creating a soldier.
//	Most of the soldiers saved in the maps will be saved in this manner.
// DetailedPlacement (pp):  A SOLDIERCREATE_STRUCT ready to be passed to
// TacticalCreateSoldier to generate 	and add a new soldier to the world.  The
// DetailedPlacement contains all of the necessary information
//  to do this.  This information won't be saved in maps. In most cases, only
//  very few attributes are static, and the rest are generated at runtime.
//  Because of this situation, saved detailed placements must be in a different
//  format.
// StaticDetailedPlacement (spp):  A hybrid version of the DetailedPlacement.
// This is the information saved in 	the map via the editor.  When loaded, this
// information is converted to a normal detailed placement, but 	must also use the
// BasicPlacement information to complete this properly.  Once the conversion is
// complete, 	the static information is lost.  This gives us complete flexibility.
// The basic placements contain relative
//  values that work in conjunction with the strategic AI's relative values to
//  generate soldiers.  In no circumstances will static detailed placements be
//  used outside of the editor.  Note, that this hybrid version uses the
//  identical structure as detailed placements.  All non-static values are set
//  to -1.

// Used to generate a detailed placement from a basic placement.  This assumes
// that the detailed placement doesn't exist, meaning there are no static
// attributes.  This is called when you wish to convert a basic placement into a
// detailed placement just before creating a soldier.
void CreateDetailedPlacementGivenBasicPlacementInfo(SOLDIERCREATE_STRUCT *pp,
                                                    BASIC_SOLDIERCREATE_STRUCT *bp);

// Used exclusively by the editor when the user wishes to change a basic
// placement into a detailed placement. Because the intention is to make some of
// the attributes static, all of the information that can be generated are
// defaulted to -1.  When an attribute is made to be static, that value in
// replaced by the new static value. This information is NOT compatible with
// TacticalCreateSoldier.  Before doing so, you must first convert the static
// detailed placement to a regular detailed placement.
void CreateStaticDetailedPlacementGivenBasicPlacementInfo(SOLDIERCREATE_STRUCT *spp,
                                                          BASIC_SOLDIERCREATE_STRUCT *bp);

// When you are ready to generate a soldier with a static detailed placement
// slot, this function will generate the proper priority placement slot given the
// static detailed placement and it's accompanying basic placment. For the
// purposes of merc editing, the static detailed placement is preserved.
void CreateDetailedPlacementGivenStaticDetailedPlacementAndBasicPlacementInfo(
    SOLDIERCREATE_STRUCT *pp, SOLDIERCREATE_STRUCT *spp, BASIC_SOLDIERCREATE_STRUCT *bp);

// Used to update a existing soldier's attributes with the new static detailed
// placement info.  This is used by the editor upon exiting the editor into the
// game, to update the existing soldiers with new information. This gives
// flexibility of testing mercs.  Upon entering the editor again, this call will
// reset all the mercs to their original states.
void UpdateSoldierWithStaticDetailedInformation(SOLDIERTYPE *s, SOLDIERCREATE_STRUCT *spp);

// When the editor modifies the soldier's relative attribute level,
// this function is called to update that information.
void ModifySoldierAttributesWithNewRelativeLevel(SOLDIERTYPE *s, int8_t bLevel);

// Force the soldier to be a different ID
void ForceSoldierProfileID(SOLDIERTYPE *pSoldier, uint8_t ubProfileID);

void QuickCreateProfileMerc(int8_t bTeam, uint8_t ubProfileID);

void InternalTacticalRemoveSoldier(SOLDIERTYPE &, BOOLEAN fRemoveVehicle);

// SPECIAL!  Certain events in the game can cause profiled NPCs to become
// enemies.  The two cases are adding Mike and Iggy.  We will only add one NPC in
// any given combat and the conditions for setting the associated facts are done
// elsewhere.  The function will set the profile for the SOLDIERCREATE_STRUCT and
// the rest will be handled automatically so long the ubProfile field doesn't get
// changed. NOTE:  We don't want to add Mike or Iggy if this is being called from
// autoresolve!
void OkayToUpgradeEliteToSpecialProfiledEnemy(SOLDIERCREATE_STRUCT *pp);
extern BOOLEAN gfProfiledEnemyAdded;  // needs to be saved (used by the above function)

void TrashAllSoldiers();

#endif
