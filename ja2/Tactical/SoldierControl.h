// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __SOLDER_CONTROL_H
#define __SOLDER_CONTROL_H

// Kris:  November 10, 1997
// Please don't change this value from 10.  It will invalidate all of the maps
// and soldiers.
#define MAXPATROLGRIDS 10  // *** THIS IS A DUPLICATION - MUST BE MOVED !

#include "JA2Types.h"
#include "Tactical/AnimationCache.h"
#include "Tactical/ItemTypes.h"
#include "Tactical/OverheadTypes.h"

// ANDREW: these are defines for OKDestanation usage - please move to approprite
// file
#define IGNOREPEOPLE 0
#define PEOPLETOO 1
#define ALLPEOPLE 2
#define FALLINGTEST 3

#define LOCKED_NO_NEWGRIDNO 2

#define NO_PROFILE 200

#define BATTLE_SND_LOWER_VOLUME 1

#define TAKE_DAMAGE_GUNFIRE 1
#define TAKE_DAMAGE_BLADE 2
#define TAKE_DAMAGE_HANDTOHAND 3
#define TAKE_DAMAGE_FALLROOF 4
#define TAKE_DAMAGE_BLOODLOSS 5
#define TAKE_DAMAGE_EXPLOSION 6
#define TAKE_DAMAGE_ELECTRICITY 7
#define TAKE_DAMAGE_GAS 8
#define TAKE_DAMAGE_TENTACLES 9
#define TAKE_DAMAGE_STRUCTURE_EXPLOSION 10
#define TAKE_DAMAGE_OBJECT 11

#define SOLDIER_MULTI_SELECTED 0x00000004
#define SOLDIER_PC 0x00000008
#define SOLDIER_ATTACK_NOTICED 0x00000010
#define SOLDIER_PCUNDERAICONTROL 0x00000020
#define SOLDIER_UNDERAICONTROL 0x00000040
#define SOLDIER_DEAD 0x00000080
#define SOLDIER_LOOKFOR_ITEMS 0x00000200
#define SOLDIER_ENEMY 0x00000400
#define SOLDIER_ENGAGEDINACTION 0x00000800
#define SOLDIER_ROBOT 0x00001000
#define SOLDIER_MONSTER 0x00002000
#define SOLDIER_ANIMAL 0x00004000
#define SOLDIER_VEHICLE 0x00008000
#define SOLDIER_MULTITILE 0x00020000
#define SOLDIER_TURNINGFROMHIT 0x00080000
#define SOLDIER_BOXER 0x00100000
#define SOLDIER_LOCKPENDINGACTIONCOUNTER 0x00200000
#define SOLDIER_COWERING 0x00400000
#define SOLDIER_MUTE 0x00800000
#define SOLDIER_GASSED 0x01000000
#define SOLDIER_OFF_MAP 0x02000000
#define SOLDIER_PAUSEANIMOVE 0x04000000
#define SOLDIER_DRIVER 0x08000000
#define SOLDIER_PASSENGER 0x10000000
#define SOLDIER_NPC_DOING_PUNCH 0x20000000
#define SOLDIER_NPC_SHOOTING 0x40000000
#define SOLDIER_LOOK_NEXT_TURNSOLDIER 0x80000000

/*
#define	SOLDIER_TRAIT_LOCKPICKING		0x0001
#define	SOLDIER_TRAIT_HANDTOHAND		0x0002
#define	SOLDIER_TRAIT_ELECTRONICS		0x0004
#define	SOLDIER_TRAIT_NIGHTOPS			0x0008
#define	SOLDIER_TRAIT_THROWING			0x0010
#define	SOLDIER_TRAIT_TEACHING			0x0020
#define	SOLDIER_TRAIT_HEAVY_WEAPS		0x0040
#define	SOLDIER_TRAIT_AUTO_WEAPS		0x0080
#define	SOLDIER_TRAIT_STEALTHY			0x0100
#define	SOLDIER_TRAIT_AMBIDEXT			0x0200
#define	SOLDIER_TRAIT_THIEF					0x0400
#define	SOLDIER_TRAIT_MARTIALARTS		0x0800
#define	SOLDIER_TRAIT_KNIFING				0x1000
*/
#define HAS_SKILL_TRAIT(s, t) ((s)->ubSkillTrait1 == (t) || (s)->ubSkillTrait2 == (t))
#define NUM_SKILL_TRAITS(s, t) (((s)->ubSkillTrait1 == (t)) + ((s)->ubSkillTrait2 == (t)))

#define SOLDIER_QUOTE_SAID_IN_SHIT 0x0001
#define SOLDIER_QUOTE_SAID_LOW_BREATH 0x0002
#define SOLDIER_QUOTE_SAID_BEING_PUMMELED 0x0004
#define SOLDIER_QUOTE_SAID_NEED_SLEEP 0x0008
#define SOLDIER_QUOTE_SAID_LOW_MORAL 0x0010
#define SOLDIER_QUOTE_SAID_MULTIPLE_CREATURES 0x0020
#define SOLDIER_QUOTE_SAID_ANNOYING_MERC 0x0040
#define SOLDIER_QUOTE_SAID_LIKESGUN 0x0080
#define SOLDIER_QUOTE_SAID_DROWNING 0x0100
#define SOLDIER_QUOTE_SAID_SPOTTING_CREATURE_ATTACK 0x0400
#define SOLDIER_QUOTE_SAID_SMELLED_CREATURE 0x0800
#define SOLDIER_QUOTE_SAID_ANTICIPATING_DANGER 0x1000
#define SOLDIER_QUOTE_SAID_WORRIED_ABOUT_CREATURES 0x2000
#define SOLDIER_QUOTE_SAID_PERSONALITY 0x4000
#define SOLDIER_QUOTE_SAID_FOUND_SOMETHING_NICE 0x8000

#define SOLDIER_QUOTE_SAID_EXT_HEARD_SOMETHING 0x0001
#define SOLDIER_QUOTE_SAID_EXT_SEEN_CREATURE_ATTACK 0x0002
#define SOLDIER_QUOTE_SAID_EXT_USED_BATTLESOUND_HIT 0x0004
#define SOLDIER_QUOTE_SAID_EXT_CLOSE_CALL 0x0008
#define SOLDIER_QUOTE_SAID_EXT_MIKE 0x0010
#define SOLDIER_QUOTE_SAID_DONE_ASSIGNMENT 0x0020
#define SOLDIER_QUOTE_SAID_BUDDY_1_WITNESSED 0x0040
#define SOLDIER_QUOTE_SAID_BUDDY_2_WITNESSED 0x0080
#define SOLDIER_QUOTE_SAID_BUDDY_3_WITNESSED 0x0100

#define SOLDIER_CONTRACT_RENEW_QUOTE_NOT_USED 0
#define SOLDIER_CONTRACT_RENEW_QUOTE_89_USED 1
#define SOLDIER_CONTRACT_RENEW_QUOTE_115_USED 2

#define SOLDIER_MISC_HEARD_GUNSHOT 0x01
// make sure soldiers (esp tanks) are not hurt multiple times by explosions
#define SOLDIER_MISC_HURT_BY_EXPLOSION 0x02
// should be revealed due to xrays
#define SOLDIER_MISC_XRAYED 0x04

#define NOBLOOD 40
#define MIN_BLEEDING_THRESHOLD 12  // you're OK while <4 Yellow life bars

#define BANDAGED(s) (s->bLifeMax - s->bLife - s->bBleeding)

// amount of time a stats is to be displayed differently, due to change
#define CHANGE_STAT_RECENTLY_DURATION 60000

#define NO_PENDING_ACTION 255
#define NO_PENDING_ANIMATION 32001
#define NO_PENDING_DIRECTION 253
#define NO_PENDING_STANCE 254
#define NO_DESIRED_HEIGHT 255

// ENUMERATIONS FOR ACTIONS
enum {
  MERC_OPENDOOR,
  MERC_OPENSTRUCT,
  MERC_PICKUPITEM,
  MERC_PUNCH,
  MERC_KNIFEATTACK,
  MERC_GIVEAID,
  MERC_GIVEITEM,
  MERC_WAITFOROTHERSTOTRIGGER,
  MERC_CUTFFENCE,
  MERC_DROPBOMB,
  MERC_STEAL,
  MERC_TALK,
  MERC_ENTER_VEHICLE,
  MERC_REPAIR,
  MERC_RELOADROBOT,
  MERC_TAKEBLOOD,
  MERC_ATTACH_CAN,
  MERC_FUEL_VEHICLE,
};

// ENUMERATIONS FOR THROW ACTIONS
enum {
  NO_THROW_ACTION,
  THROW_ARM_ITEM,
  THROW_TARGET_MERC_CATCH,
};

// An enumeration for playing battle sounds
enum BattleSound {
  BATTLE_SOUND_OK1,
  BATTLE_SOUND_OK2,
  BATTLE_SOUND_COOL1,
  BATTLE_SOUND_CURSE1,
  BATTLE_SOUND_HIT1,
  BATTLE_SOUND_HIT2,
  BATTLE_SOUND_LAUGH1,
  BATTLE_SOUND_ATTN1,
  BATTLE_SOUND_DIE1,
  BATTLE_SOUND_HUMM,
  BATTLE_SOUND_NOTHING,
  BATTLE_SOUND_GOTIT,
  BATTLE_SOUND_LOWMARALE_OK1,
  BATTLE_SOUND_LOWMARALE_OK2,
  BATTLE_SOUND_LOWMARALE_ATTN1,
  BATTLE_SOUND_LOCKED,
  BATTLE_SOUND_ENEMY,
  NUM_MERC_BATTLE_SOUNDS
};

// different kinds of merc
enum {
  MERC_TYPE__PLAYER_CHARACTER,
  MERC_TYPE__AIM_MERC,
  MERC_TYPE__MERC,
  MERC_TYPE__NPC,
  MERC_TYPE__EPC,
  MERC_TYPE__NPC_WITH_UNEXTENDABLE_CONTRACT,
  MERC_TYPE__VEHICLE,
};

// I don't care if this isn't intuitive!  The hand positions go right
// before the big pockets so we can loop through them that way. --CJC
#define NO_SLOT -1

// vehicle/human path structure
struct PathSt {
  uint32_t uiSectorId;
  PathSt *pNext;
  PathSt *pPrev;
};

enum InvSlotPos {
  HELMETPOS = 0,
  VESTPOS,
  LEGPOS,
  HEAD1POS,
  HEAD2POS,
  HANDPOS,
  SECONDHANDPOS,
  BIGPOCK1POS,
  BIGPOCK2POS,
  BIGPOCK3POS,
  BIGPOCK4POS,
  SMALLPOCK1POS,
  SMALLPOCK2POS,
  SMALLPOCK3POS,
  SMALLPOCK4POS,
  SMALLPOCK5POS,
  SMALLPOCK6POS,
  SMALLPOCK7POS,
  SMALLPOCK8POS,  // = 18, so 19 pockets needed

  NUM_INV_SLOTS,
};

// used for color codes, but also shows the enemy type for debugging purposes
enum SoldierClass {
  SOLDIER_CLASS_NONE,
  SOLDIER_CLASS_ADMINISTRATOR,
  SOLDIER_CLASS_ELITE,
  SOLDIER_CLASS_ARMY,
  SOLDIER_CLASS_GREEN_MILITIA,
  SOLDIER_CLASS_REG_MILITIA,
  SOLDIER_CLASS_ELITE_MILITIA,
  SOLDIER_CLASS_CREATURE,
  SOLDIER_CLASS_MINER,
};

#define SOLDIER_CLASS_ENEMY(bSoldierClass) \
  ((bSoldierClass >= SOLDIER_CLASS_ADMINISTRATOR) && (bSoldierClass <= SOLDIER_CLASS_ARMY))
#define SOLDIER_CLASS_MILITIA(bSoldierClass) \
  ((bSoldierClass >= SOLDIER_CLASS_GREEN_MILITIA) && (bSoldierClass <= SOLDIER_CLASS_ELITE_MILITIA))

// This macro should be used whenever we want to see if someone is neutral
// IF WE ARE CONSIDERING ATTACKING THEM.  Creatures & bloodcats will attack
// neutrals but they can't attack empty vehicles!!
#define CONSIDERED_NEUTRAL(me, them) \
  ((them)->bNeutral && ((me)->bTeam != CREATURE_TEAM || (them)->uiStatusFlags & SOLDIER_VEHICLE))

struct KEY_ON_RING {
  uint8_t ubKeyID;
  uint8_t ubNumber;
};

struct THROW_PARAMS {
  float dX;
  float dY;
  float dZ;
  float dForceX;
  float dForceY;
  float dForceZ;
  float dLifeSpan;
  uint8_t ubActionCode;
  SOLDIERTYPE *target;
};

#define DELAYED_MOVEMENT_FLAG_PATH_THROUGH_PEOPLE 0x01

// reasons for being unable to continue movement
enum {
  REASON_STOPPED_NO_APS,
  REASON_STOPPED_SIGHT,
};

enum {
  HIT_BY_TEARGAS = 0x01,
  HIT_BY_MUSTARDGAS = 0x02,
  HIT_BY_CREATUREGAS = 0x04,
};

struct SOLDIERTYPE {
  // ID
  uint8_t ubID;

  // DESCRIPTION / STATS, ETC
  uint8_t ubBodyType;
  int8_t bActionPoints;
  int8_t bInitialActionPoints;

  uint32_t uiStatusFlags;

  OBJECTTYPE inv[NUM_INV_SLOTS];
  OBJECTTYPE *pTempObject;
  KEY_ON_RING *pKeyRing;

  int8_t bOldLife;  // life at end of last turn, recorded for monster AI
  // attributes
  uint8_t bInSector;
  int8_t bFlashPortraitFrame;
  int16_t sFractLife;  // fraction of life pts (in hundreths)
  int8_t bBleeding;    // blood loss control variable
  int8_t bBreath;      // current breath value
  int8_t bBreathMax;   // max breath, affected by fatigue/sleep
  int8_t bStealthMode;

  int16_t sBreathRed;  // current breath value
  BOOLEAN fDelayedMovement;

  uint8_t ubWaitActionToDo;
  int8_t ubInsertionDirection;
  // skills
  SOLDIERTYPE *opponent;
  int8_t bLastRenderVisibleValue;
  uint8_t ubAttackingHand;
  // traits
  int16_t sWeightCarriedAtTurnStart;
  wchar_t name[10];

  int8_t bVisible;  // to render or not to render...

  int8_t bActive;

  int8_t bTeam;  // Team identifier

  // NEW MOVEMENT INFORMATION for Strategic Movement
  uint8_t ubGroupID;        // the movement group the merc is currently part of.
  BOOLEAN fBetweenSectors;  // set when the group isn't actually in a sector.
                            // sSectorX and sSectorY will reflect the sector the
                            // merc was at last.

  uint8_t ubMovementNoiseHeard;  // 8 flags by direction

  // 23 bytes so far

  // WORLD POSITION STUFF
  float dXPos;
  float dYPos;
  int16_t sInitialGridNo;
  int16_t sGridNo;
  int8_t bDirection;
  int16_t sHeightAdjustment;
  int16_t sDesiredHeight;
  int16_t sTempNewGridNo;  // New grid no for advanced animations
  int8_t bOverTerrainType;

  int8_t bCollapsed;        // collapsed due to being out of APs
  int8_t bBreathCollapsed;  // collapsed due to being out of APs
  // 50 bytes so far

  uint8_t ubDesiredHeight;
  uint16_t usPendingAnimation;
  uint8_t ubPendingStanceChange;
  uint16_t usAnimState;
  BOOLEAN fNoAPToFinishMove;
  BOOLEAN fPausedMove;
  BOOLEAN fUIdeadMerc;   // UI Flags for removing a newly dead merc
  BOOLEAN fUICloseMerc;  // UI Flags for closing panels

  TIMECOUNTER UpdateCounter;
  TIMECOUNTER DamageCounter;
  TIMECOUNTER AICounter;
  TIMECOUNTER FadeCounter;

  uint8_t ubSkillTrait1;
  uint8_t ubSkillTrait2;

  int8_t bDexterity;  // dexterity (hand coord) value
  int8_t bWisdom;
  SOLDIERTYPE *attacker;
  SOLDIERTYPE *previous_attacker;
  SOLDIERTYPE *next_to_previous_attacker;
  BOOLEAN fTurnInProgress;

  BOOLEAN fIntendedTarget;  // intentionally shot?
  BOOLEAN fPauseAllAnimation;

  int8_t bExpLevel;  // general experience level
  int16_t sInsertionGridNo;

  BOOLEAN fContinueMoveAfterStanceChange;

  // 60
  AnimationSurfaceCacheType AnimCache;  // will be 9 bytes once changed to pointers

  int8_t bLife;  // current life (hit points or health)
  uint8_t bSide;
  int8_t bNewOppCnt;

  uint16_t usAniCode;
  uint16_t usAniFrame;
  int16_t sAniDelay;

  // MOVEMENT TO NEXT TILE HANDLING STUFF
  int8_t bAgility;  // agility (speed) value
  int16_t sDelayedMovementCauseGridNo;
  int16_t sReservedMovementGridNo;

  int8_t bStrength;

  // Weapon Stuff
  int16_t sTargetGridNo;
  int8_t bTargetLevel;
  int8_t bTargetCubeLevel;
  int16_t sLastTarget;
  int8_t bTilesMoved;
  int8_t bLeadership;
  float dNextBleed;
  BOOLEAN fWarnedAboutBleeding;
  BOOLEAN fDyingComment;

  uint8_t ubTilesMovedPerRTBreathUpdate;
  uint16_t usLastMovementAnimPerRTBreathUpdate;

  BOOLEAN fTurningToShoot;
  BOOLEAN fTurningUntilDone;
  BOOLEAN fGettingHit;
  BOOLEAN fInNonintAnim;
  BOOLEAN fFlashLocator;
  int16_t sLocatorFrame;
  BOOLEAN fShowLocator;
  BOOLEAN fFlashPortrait;
  int8_t bMechanical;
  int8_t bLifeMax;  // maximum life for this merc

  FACETYPE *face;

  // PALETTE MANAGEMENT STUFF
  PaletteRepID HeadPal;   // 30
  PaletteRepID PantsPal;  // 30
  PaletteRepID VestPal;   // 30
  PaletteRepID SkinPal;   // 30

  uint16_t *pShades[NUM_SOLDIER_SHADES];  // Shading tables
  uint16_t *pGlowShades[20];              //
  int8_t bMedical;
  BOOLEAN fBeginFade;
  uint8_t ubFadeLevel;
  uint8_t ubServiceCount;
  SOLDIERTYPE *service_partner;
  int8_t bMarksmanship;
  int8_t bExplosive;
  THROW_PARAMS *pThrowParams;
  BOOLEAN fTurningFromPronePosition;
  int8_t bReverse;
  LEVELNODE *pLevelNode;

  // WALKING STUFF
  int8_t bDesiredDirection;
  int16_t sDestXPos;
  int16_t sDestYPos;
  int16_t sDestination;
  int16_t sFinalDestination;
  int8_t bLevel;

  // PATH STUFF
  uint16_t usPathingData[MAX_PATH_LIST_SIZE];
  uint16_t usPathDataSize;
  uint16_t usPathIndex;
  int16_t sBlackList;
  int8_t bAimTime;
  int8_t bShownAimTime;
  int8_t bPathStored;  // good for AI to reduct redundancy
  int8_t bHasKeys;     // allows AI controlled dudes to open locked doors

  uint8_t ubStrategicInsertionCode;
  uint16_t usStrategicInsertionData;

  LIGHT_SPRITE *light;
  LIGHT_SPRITE *muzzle_flash;
  int8_t bMuzFlashCount;

  int16_t sX;
  int16_t sY;

  uint16_t usOldAniState;
  int16_t sOldAniCode;

  int8_t bBulletsLeft;
  uint8_t ubSuppressionPoints;

  // STUFF FOR RANDOM ANIMATIONS
  uint32_t uiTimeOfLastRandomAction;

  // AI STUFF
  int8_t bOppList[MAX_NUM_SOLDIERS];  // AI knowledge database
  int8_t bLastAction;
  int8_t bAction;
  uint16_t usActionData;
  int8_t bNextAction;
  uint16_t usNextActionData;
  int8_t bActionInProgress;
  int8_t bAlertStatus;
  int8_t bOppCnt;
  int8_t bNeutral;
  int8_t bNewSituation;
  int8_t bNextTargetLevel;
  int8_t bOrders;
  int8_t bAttitude;
  int8_t bUnderFire;
  int8_t bShock;
  int8_t bBypassToGreen;
  int8_t bDominantDir;                   // AI main direction to face...
  int8_t bPatrolCnt;                     // number of patrol gridnos
  int8_t bNextPatrolPnt;                 // index to next patrol gridno
  int16_t usPatrolGrid[MAXPATROLGRIDS];  // AI list for ptr->orders==PATROL
  int16_t sNoiseGridno;
  uint8_t ubNoiseVolume;
  int8_t bLastAttackHit;
  SOLDIERTYPE *xrayed_by;
  float dHeightAdjustment;
  int8_t bMorale;
  int8_t bTeamMoraleMod;
  int8_t bTacticalMoraleMod;
  int8_t bStrategicMoraleMod;
  int8_t bAIMorale;
  uint8_t ubPendingAction;
  uint8_t ubPendingActionAnimCount;
  uint32_t uiPendingActionData1;
  int16_t sPendingActionData2;
  int8_t bPendingActionData3;
  int8_t ubDoorHandleCode;
  uint32_t uiPendingActionData4;
  int8_t bInterruptDuelPts;
  int8_t bPassedLastInterrupt;
  int8_t bIntStartAPs;
  int8_t bMoved;
  int8_t bHunting;
  uint8_t ubCaller;
  int16_t sCallerGridNo;
  uint8_t bCallPriority;
  int8_t bCallActedUpon;
  int8_t bFrenzied;
  int8_t bNormalSmell;
  int8_t bMonsterSmell;
  int8_t bMobility;
  int8_t fAIFlags;

  BOOLEAN fDontChargeReadyAPs;
  uint16_t usAnimSurface;
  uint16_t sZLevel;
  BOOLEAN fPrevInWater;
  BOOLEAN fGoBackToAimAfterHit;

  int16_t sWalkToAttackGridNo;
  int16_t sWalkToAttackWalkToCost;

  BOOLEAN fForceShade;
  uint16_t *pForcedShade;

  int8_t bDisplayDamageCount;
  int8_t fDisplayDamage;
  int16_t sDamage;
  int16_t sDamageX;
  int16_t sDamageY;
  int8_t bDoBurst;
  int16_t usUIMovementMode;
  BOOLEAN fUIMovementFast;

  TIMECOUNTER BlinkSelCounter;
  TIMECOUNTER PortraitFlashCounter;
  BOOLEAN fDeadSoundPlayed;
  uint8_t ubProfile;
  uint8_t ubQuoteRecord;
  uint8_t ubQuoteActionID;
  uint8_t ubBattleSoundID;

  BOOLEAN fClosePanel;
  BOOLEAN fClosePanelToDie;
  uint8_t ubClosePanelFrame;
  BOOLEAN fDeadPanel;
  uint8_t ubDeadPanelFrame;

  int16_t sPanelFaceX;
  int16_t sPanelFaceY;

  // QUOTE STUFF
  int8_t bNumHitsThisTurn;
  uint16_t usQuoteSaidFlags;
  int8_t fCloseCall;
  int8_t bLastSkillCheck;
  int8_t ubSkillCheckAttempts;

  int8_t bStartFallDir;
  int8_t fTryingToFall;

  uint8_t ubPendingDirection;
  uint32_t uiAnimSubFlags;

  uint8_t bAimShotLocation;
  uint8_t ubHitLocation;

  uint16_t *effect_shade;  // Shading table for effects

  int16_t sSpreadLocations[6];
  BOOLEAN fDoSpread;
  int16_t sStartGridNo;
  int16_t sEndGridNo;
  int16_t sForcastGridno;
  int16_t sZLevelOverride;
  int8_t bMovedPriorToInterrupt;
  int32_t iEndofContractTime;  // time, in global time(resolution, minutes) that merc
                               // will leave, or if its a M.E.R.C. merc it will be
                               // set to -1.  -2 for NPC and player generated
  int32_t iStartContractTime;
  int32_t iTotalContractLength;    // total time of AIM mercs contract	or the
                                   // time since last paid for a M.E.R.C. merc
  int32_t iNextActionSpecialData;  // AI special action data record for the next
                                   // action
  uint8_t ubWhatKindOfMercAmI;     // Set to the type of character it is
  int8_t bAssignment;              // soldiers current assignment
  BOOLEAN fForcedToStayAwake;      // forced by player to stay awake, reset to false,
                                   // the moment they are set to rest or sleep
  int8_t bTrainStat;               // current stat soldier is training
  int16_t sSectorX;                // X position on the Stategic Map
  int16_t sSectorY;                // Y position on the Stategic Map
  int8_t bSectorZ;                 // Z sector location
  int32_t iVehicleId;              // the id of the vehicle the char is in
  PathSt *pMercPath;               // Path Structure
  uint8_t fHitByGasFlags;          // flags
  uint16_t usMedicalDeposit;       // is there a medical deposit on merc
  uint16_t usLifeInsurance;        // is there life insurance taken out on merc

  int32_t iStartOfInsuranceContract;
  uint32_t uiLastAssignmentChangeMin;  // timestamp of last assignment change in
                                       // minutes
  int32_t iTotalLengthOfInsuranceContract;

  uint8_t ubSoldierClass;  // admin, elite, troop (creature types?)
  uint8_t ubAPsLostToSuppression;
  BOOLEAN fChangingStanceDueToSuppression;
  SOLDIERTYPE *suppressor;

  uint8_t ubCivilianGroup;

  // time changes...when a stat was changed according to GetJA2Clock();
  uint32_t uiChangeLevelTime;
  uint32_t uiChangeHealthTime;
  uint32_t uiChangeStrengthTime;
  uint32_t uiChangeDexterityTime;
  uint32_t uiChangeAgilityTime;
  uint32_t uiChangeWisdomTime;
  uint32_t uiChangeLeadershipTime;
  uint32_t uiChangeMarksmanshipTime;
  uint32_t uiChangeExplosivesTime;
  uint32_t uiChangeMedicalTime;
  uint32_t uiChangeMechanicalTime;

  uint32_t uiUniqueSoldierIdValue;  // the unique value every instance of a soldier
                                    // gets - 1 is the first valid value
  int8_t bBeingAttackedCount;       // Being attacked counter

  int8_t bNewItemCount[NUM_INV_SLOTS];
  int8_t bNewItemCycleCount[NUM_INV_SLOTS];
  BOOLEAN fCheckForNewlyAddedItems;
  int8_t bEndDoorOpenCode;

  uint8_t ubScheduleID;
  int16_t sEndDoorOpenCodeData;
  TIMECOUNTER NextTileCounter;
  BOOLEAN fBlockedByAnotherMerc;
  int8_t bBlockedByAnotherMercDirection;
  uint16_t usAttackingWeapon;
  SOLDIERTYPE *target;
  int8_t bWeaponMode;
  int8_t bAIScheduleProgress;
  int16_t sOffWorldGridNo;
  ANITILE *pAniTile;
  int8_t bCamo;
  int16_t sAbsoluteFinalDestination;
  uint8_t ubHiResDirection;
  uint8_t ubLastFootPrintSound;
  int8_t bVehicleID;
  int8_t fPastXDest;
  int8_t fPastYDest;
  int8_t bMovementDirection;
  int16_t sOldGridNo;
  uint16_t usDontUpdateNewGridNoOnMoveAnimChange;
  int16_t sBoundingBoxWidth;
  int16_t sBoundingBoxHeight;
  int16_t sBoundingBoxOffsetX;
  int16_t sBoundingBoxOffsetY;
  uint32_t uiTimeSameBattleSndDone;
  int8_t bOldBattleSnd;
  BOOLEAN fContractPriceHasIncreased;
  int32_t iBurstSoundID;
  BOOLEAN fFixingSAMSite;
  BOOLEAN fFixingRobot;
  int8_t bSlotItemTakenFrom;
  BOOLEAN fSignedAnotherContract;
  SOLDIERTYPE *auto_bandaging_medic;
  BOOLEAN fDontChargeTurningAPs;
  SOLDIERTYPE *robot_remote_holder;
  uint32_t uiTimeOfLastContractUpdate;
  int8_t bTypeOfLastContract;
  int8_t bTurnsCollapsed;
  int8_t bSleepDrugCounter;
  uint8_t ubMilitiaKills;

  int8_t bFutureDrugEffect[2];    // value to represent effect of a needle
  int8_t bDrugEffectRate[2];      // represents rate of increase and decrease of effect
  int8_t bDrugEffect[2];          // value that affects AP & morale calc ( -ve is poorly )
  int8_t bDrugSideEffectRate[2];  // duration of negative AP and morale effect
  int8_t bDrugSideEffect[2];      // duration of negative AP and morale effect

  int8_t bBlindedCounter;
  BOOLEAN fMercCollapsedFlag;
  BOOLEAN fDoneAssignmentAndNothingToDoFlag;
  BOOLEAN fMercAsleep;
  BOOLEAN fDontChargeAPsForStanceChange;

  uint8_t ubTurnsUntilCanSayHeardNoise;
  uint16_t usQuoteSaidExtFlags;

  uint16_t sContPathLocation;
  int8_t bGoodContPath;
  int8_t bNoiseLevel;
  int8_t bRegenerationCounter;
  int8_t bRegenBoostersUsedToday;
  int8_t bNumPelletsHitBy;
  int16_t sSkillCheckGridNo;
  uint8_t ubLastEnemyCycledID;

  uint8_t ubPrevSectorID;
  uint8_t ubNumTilesMovesSinceLastForget;
  int8_t bTurningIncrement;
  uint32_t uiBattleSoundID;

  BOOLEAN fSoldierWasMoving;
  BOOLEAN fSayAmmoQuotePending;
  uint16_t usValueGoneUp;

  uint8_t ubNumLocateCycles;
  uint8_t ubDelayedMovementFlags;
  BOOLEAN fMuzzleFlash;
  const SOLDIERTYPE *CTGTTarget;

  TIMECOUNTER PanelAnimateCounter;

  int8_t bCurrentCivQuote;
  int8_t bCurrentCivQuoteDelta;
  uint8_t ubMiscSoldierFlags;
  uint8_t ubReasonCantFinishMove;

  int16_t sLocationOfFadeStart;
  uint8_t bUseExitGridForReentryDirection;

  uint32_t uiTimeSinceLastSpoke;
  uint8_t ubContractRenewalQuoteCode;
  int16_t sPreTraversalGridNo;
  uint32_t uiXRayActivatedTime;
  int8_t bTurningFromUI;
  int8_t bPendingActionData5;

  int8_t bDelayedStrategicMoraleMod;
  uint8_t ubDoorOpeningNoise;

  uint8_t ubLeaveHistoryCode;
  BOOLEAN fDontUnsetLastTargetFromTurn;
  int8_t bOverrideMoveSpeed;
  BOOLEAN fUseMoverrideMoveSpeed;

  uint32_t uiTimeSoldierWillArrive;
  BOOLEAN fUseLandingZoneForArrival;
  BOOLEAN fFallClockwise;
  int8_t bVehicleUnderRepairID;
  int32_t iTimeCanSignElsewhere;
  int8_t bHospitalPriceModifier;
  uint32_t uiStartTimeOfInsuranceContract;
  BOOLEAN fRTInNonintAnim;
  BOOLEAN fDoingExternalDeath;
  int8_t bCorpseQuoteTolerance;
  int32_t iPositionSndID;
  int32_t iTuringSoundID;
  uint8_t ubLastDamageReason;
  BOOLEAN fComplainedThatTired;
  int16_t sLastTwoLocations[2];
  int32_t uiTimeSinceLastBleedGrunt;
};

#define BASE_FOR_EACH_SOLDIER_INV_SLOT(type, iter, soldier)                                        \
  for (type *iter = (soldier).inv, *const iter##__end = endof((soldier).inv); iter != iter##__end; \
       ++iter)
#define FOR_EACH_SOLDIER_INV_SLOT(iter, soldier) \
  BASE_FOR_EACH_SOLDIER_INV_SLOT(OBJECTTYPE, iter, soldier)
#define CFOR_EACH_SOLDIER_INV_SLOT(iter, soldier) \
  BASE_FOR_EACH_SOLDIER_INV_SLOT(OBJECTTYPE const, iter, soldier)

#define HEALTH_INCREASE 0x0001
#define STRENGTH_INCREASE 0x0002
#define DEX_INCREASE 0x0004
#define AGIL_INCREASE 0x0008
#define WIS_INCREASE 0x0010
#define LDR_INCREASE 0x0020

#define MRK_INCREASE 0x0040
#define MED_INCREASE 0x0080
#define EXP_INCREASE 0x0100
#define MECH_INCREASE 0x0200

#define LVL_INCREASE 0x0400

enum WeaponModes { WM_NORMAL = 0, WM_BURST, WM_ATTACHED, NUM_WEAPON_MODES };

// TYPEDEFS FOR ANIMATION PROFILES
struct ANIM_PROF_TILE {
  uint16_t usTileFlags;
  int8_t bTileX;
  int8_t bTileY;
};

struct ANIM_PROF_DIR {
  uint8_t ubNumTiles;
  ANIM_PROF_TILE *pTiles;
};

struct ANIM_PROF {
  ANIM_PROF_DIR Dirs[8];
};

struct PaletteReplacementType {
  uint8_t ubType;
  PaletteRepID ID;
  uint8_t ubPaletteSize;
  SGPPaletteEntry *rgb;
};

// VARIABLES FOR PALETTE REPLACEMENTS FOR HAIR, ETC
extern uint8_t *gubpNumReplacementsPerRange;
extern PaletteReplacementType *gpPalRep;

extern uint8_t bHealthStrRanges[];

void DeleteSoldier(SOLDIERTYPE &);
void DeleteSoldierLight(SOLDIERTYPE *);

void CreateSoldierCommon(SOLDIERTYPE &);

// Soldier Management functions, called by Event Pump.c
void EVENT_InitNewSoldierAnim(SOLDIERTYPE *, uint16_t new_state, uint16_t starting_ani_code,
                              BOOLEAN force);

void ChangeSoldierState(SOLDIERTYPE *pSoldier, uint16_t usNewState, uint16_t usStartingAniCode,
                        BOOLEAN fForce);

enum SetSoldierPosFlags {
  SSP_NONE = 0,
  SSP_NO_DEST = 1U << 0,
  SSP_NO_FINAL_DEST = 1U << 1,
  SSP_FORCE_DELETE = 1U << 2
};
ENUM_BITSET(SetSoldierPosFlags)

void EVENT_SetSoldierPosition(SOLDIERTYPE *s, GridNo gridno, SetSoldierPosFlags flags);
void EVENT_SetSoldierPositionNoCenter(SOLDIERTYPE *s, GridNo gridno, SetSoldierPosFlags flags);
void EVENT_SetSoldierPositionXY(SOLDIERTYPE *s, float dNewXPos, float dNewYPos,
                                SetSoldierPosFlags flags);

void EVENT_GetNewSoldierPath(SOLDIERTYPE *pSoldier, uint16_t sDestGridNo, uint16_t usMovementAnim);
BOOLEAN EVENT_InternalGetNewSoldierPath(SOLDIERTYPE *pSoldier, uint16_t sDestGridNo,
                                        uint16_t usMovementAnim, BOOLEAN fFromUI,
                                        BOOLEAN fForceRestart);

void EVENT_SetSoldierDirection(SOLDIERTYPE *pSoldier, uint16_t usNewDirection);
void EVENT_SetSoldierDesiredDirection(SOLDIERTYPE *pSoldier, uint16_t usNewDirection);
void EVENT_SetSoldierDesiredDirectionForward(SOLDIERTYPE *s, uint16_t new_direction);
void EVENT_FireSoldierWeapon(SOLDIERTYPE *pSoldier, int16_t sTargetGridNo);
void EVENT_SoldierGotHit(SOLDIERTYPE *pSoldier, uint16_t usWeaponIndex, int16_t sDamage,
                         int16_t sBreathLoss, uint16_t bDirection, uint16_t sRange,
                         SOLDIERTYPE *att, uint8_t ubSpecial, uint8_t ubHitLocation,
                         int16_t sLocationGrid);
void EVENT_SoldierBeginBladeAttack(SOLDIERTYPE *pSoldier, int16_t sGridNo, uint8_t ubDirection);
void EVENT_SoldierBeginPunchAttack(SOLDIERTYPE *pSoldier, int16_t sGridNo, uint8_t ubDirection);
void EVENT_SoldierBeginFirstAid(SOLDIERTYPE *pSoldier, int16_t sGridNo, uint8_t ubDirection);
void EVENT_StopMerc(SOLDIERTYPE *);
void EVENT_StopMerc(SOLDIERTYPE *, GridNo, int8_t direction);
void EVENT_SoldierBeginCutFence(SOLDIERTYPE *pSoldier, int16_t sGridNo, uint8_t ubDirection);
void EVENT_SoldierBeginRepair(SOLDIERTYPE &, GridNo, uint8_t direction);
void EVENT_SoldierBeginRefuel(SOLDIERTYPE *pSoldier, int16_t sGridNo, uint8_t ubDirection);

BOOLEAN SoldierReadyWeapon(SOLDIERTYPE *pSoldier, GridNo tgt_pos, BOOLEAN fEndReady);
void SetSoldierHeight(SOLDIERTYPE *, float new_height);
void BeginSoldierClimbUpRoof(SOLDIERTYPE *pSoldier);
void BeginSoldierClimbDownRoof(SOLDIERTYPE *);
void BeginSoldierClimbFence(SOLDIERTYPE *);

BOOLEAN CheckSoldierHitRoof(SOLDIERTYPE *pSoldier);
void BeginSoldierGetup(SOLDIERTYPE *pSoldier);

// Soldier Management functions called by Overhead.c
BOOLEAN ConvertAniCodeToAniFrame(SOLDIERTYPE *pSoldier, uint16_t usAniFrame);
void TurnSoldier(SOLDIERTYPE *pSold);
void EVENT_BeginMercTurn(SOLDIERTYPE &);
void ChangeSoldierStance(SOLDIERTYPE *pSoldier, uint8_t ubDesiredStance);
void ModifySoldierAniSpeed(SOLDIERTYPE *pSoldier);
void StopSoldier(SOLDIERTYPE *pSoldier);
uint8_t SoldierTakeDamage(SOLDIERTYPE *pSoldier, int16_t sLifeDeduct, int16_t sBreathLoss,
                          uint8_t ubReason, SOLDIERTYPE *attacker);
void ReviveSoldier(SOLDIERTYPE *pSoldier);

// Palette functions for soldiers
void CreateSoldierPalettes(SOLDIERTYPE &);
uint8_t GetPaletteRepIndexFromID(const PaletteRepID pal_rep);
void SetPaletteReplacement(SGPPaletteEntry *, PaletteRepID);
void LoadPaletteData();
void DeletePaletteData();

// UTILITY FUNCTUIONS
void MoveMerc(SOLDIERTYPE *pSoldier, float dMovementChange, float dAngle, BOOLEAN fCheckRange);
void MoveMercFacingDirection(SOLDIERTYPE *pSoldier, BOOLEAN fReverse, float dMovementDist);
int16_t GetDirectionFromGridNo(int16_t sGridNo, const SOLDIERTYPE *pSoldier);
uint8_t atan8(int16_t sXPos, int16_t sYPos, int16_t sXPos2, int16_t sYPos2);
int8_t CalcActionPoints(const SOLDIERTYPE *);
int16_t GetDirectionToGridNoFromGridNo(int16_t sGridNoDest, int16_t sGridNoSrc);
void ReleaseSoldiersAttacker(SOLDIERTYPE *pSoldier);
BOOLEAN MercInWater(const SOLDIERTYPE *pSoldier);
uint16_t GetMoveStateBasedOnStance(const SOLDIERTYPE *, uint8_t ubStanceHeight);
void SoldierGotoStationaryStance(SOLDIERTYPE *pSoldier);
void ReCreateSoldierLight(SOLDIERTYPE *);

void MakeCharacterDialogueEventDoBattleSound(SOLDIERTYPE &s, BattleSound, uint32_t delay);
BOOLEAN DoMercBattleSound(SOLDIERTYPE *, BattleSound);
BOOLEAN InternalDoMercBattleSound(SOLDIERTYPE *, BattleSound, int8_t bSpecialCode);

uint32_t SoldierDressWound(SOLDIERTYPE *pSoldier, SOLDIERTYPE *pVictim, int16_t sKitPts,
                           int16_t sStatus);
void ReceivingSoldierCancelServices(SOLDIERTYPE *pSoldier);
void GivingSoldierCancelServices(SOLDIERTYPE *pSoldier);
void InternalGivingSoldierCancelServices(SOLDIERTYPE *pSoldier, BOOLEAN fPlayEndAnim);

// WRAPPERS FOR SOLDIER EVENTS
void SendGetNewSoldierPathEvent(SOLDIERTYPE *, uint16_t sDestGridNo);
void SendSoldierSetDesiredDirectionEvent(const SOLDIERTYPE *pSoldier, uint16_t usDesiredDirection);
void SendBeginFireWeaponEvent(SOLDIERTYPE *pSoldier, int16_t sTargetGridNo);

void HaultSoldierFromSighting(SOLDIERTYPE *pSoldier, BOOLEAN fFromSightingEnemy);
void ReLoadSoldierAnimationDueToHandItemChange(SOLDIERTYPE *pSoldier, uint16_t usOldItem,
                                               uint16_t usNewItem);

bool CheckForBreathCollapse(SOLDIERTYPE &);

static inline BOOLEAN IsOnCivTeam(const SOLDIERTYPE *const s) { return s->bTeam == CIV_TEAM; }

#define PTR_CROUCHED (gAnimControl[pSoldier->usAnimState].ubHeight == ANIM_CROUCH)
#define PTR_STANDING (gAnimControl[pSoldier->usAnimState].ubHeight == ANIM_STAND)
#define PTR_PRONE (gAnimControl[pSoldier->usAnimState].ubHeight == ANIM_PRONE)

void EVENT_SoldierBeginGiveItem(SOLDIERTYPE *pSoldier);

void DoNinjaAttack(SOLDIERTYPE *pSoldier);

BOOLEAN InternalSoldierReadyWeapon(SOLDIERTYPE *pSoldier, uint8_t sFacingDir, BOOLEAN fEndReady);

void RemoveSoldierFromGridNo(SOLDIERTYPE &);

void PositionSoldierLight(SOLDIERTYPE *pSoldier);

void EVENT_InternalSetSoldierDestination(SOLDIERTYPE *pSoldier, uint16_t usNewDirection,
                                         BOOLEAN fFromMove, uint16_t usAnimState);

void ChangeToFallbackAnimation(SOLDIERTYPE *pSoldier, int8_t bDirection);

void EVENT_SoldierBeginKnifeThrowAttack(SOLDIERTYPE *pSoldier, int16_t sGridNo,
                                        uint8_t ubDirection);
void EVENT_SoldierBeginUseDetonator(SOLDIERTYPE *pSoldier);
void EVENT_SoldierBeginDropBomb(SOLDIERTYPE *pSoldier);
void EVENT_SoldierEnterVehicle(SOLDIERTYPE &, GridNo);

void SetSoldierCowerState(SOLDIERTYPE *pSoldier, BOOLEAN fOn);

BOOLEAN PlayerSoldierStartTalking(SOLDIERTYPE *pSoldier, uint8_t ubTargetID, BOOLEAN fValidate);

void CalcNewActionPoints(SOLDIERTYPE *pSoldier);

BOOLEAN InternalIsValidStance(const SOLDIERTYPE *pSoldier, int8_t bDirection, int8_t bNewStance);

void AdjustNoAPToFinishMove(SOLDIERTYPE *pSoldier, BOOLEAN fSet);

void UpdateRobotControllerGivenController(SOLDIERTYPE *pSoldier);
void UpdateRobotControllerGivenRobot(SOLDIERTYPE *pSoldier);
SOLDIERTYPE *GetRobotController(SOLDIERTYPE *pSoldier);
BOOLEAN CanRobotBeControlled(const SOLDIERTYPE *pSoldier);
BOOLEAN ControllingRobot(const SOLDIERTYPE *s);

void EVENT_SoldierBeginReloadRobot(SOLDIERTYPE *pSoldier, int16_t sGridNo, uint8_t ubDirection,
                                   uint8_t ubMercSlot);

void EVENT_SoldierBeginTakeBlood(SOLDIERTYPE *pSoldier, int16_t sGridNo, uint8_t ubDirection);

void EVENT_SoldierBeginAttachCan(SOLDIERTYPE *pSoldier, int16_t sGridNo, uint8_t ubDirection);

void PickDropItemAnimation(SOLDIERTYPE *pSoldier);

bool IsValidSecondHandShot(SOLDIERTYPE const *);
bool IsValidSecondHandShotForReloadingPurposes(SOLDIERTYPE const *);

void CrowsFlyAway(uint8_t ubTeam);

void DebugValidateSoldierData();

void BeginTyingToFall(SOLDIERTYPE *pSoldier);

void SetSoldierAsUnderAiControl(SOLDIERTYPE *pSoldier);
void HandlePlayerTogglingLightEffects(BOOLEAN fToggleValue);

void HandleSystemNewAISituation(SOLDIERTYPE *);
void SetSoldierAniSpeed(SOLDIERTYPE *pSoldier);
void PlaySoldierFootstepSound(SOLDIERTYPE *pSoldier);
void PlayStealthySoldierFootstepSound(SOLDIERTYPE *pSoldier);

// DO NOT CALL UNLESS THROUGH EVENT_SetSoldierPosition
uint16_t PickSoldierReadyAnimation(SOLDIERTYPE *pSoldier, BOOLEAN fEndReady);

extern BOOLEAN gfGetNewPathThroughPeople;

void FlashSoldierPortrait(SOLDIERTYPE *);

static inline bool IsWearingHeadGear(SOLDIERTYPE const &s, uint16_t const item) {
  return s.inv[HEAD1POS].usItem == item || s.inv[HEAD2POS].usItem == item;
}

#endif
