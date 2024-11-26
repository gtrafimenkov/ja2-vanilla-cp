#ifndef __WEAPONS_H
#define __WEAPONS_H

#include "JA2Types.h"
#include "Tactical/ItemTypes.h"
#include "Utils/SoundControl.h"

#define MAXCHANCETOHIT 99
#define BAD_DODGE_POSITION_PENALTY 20

#define GUN_BARREL_RANGE_BONUS 100

// Special deaths can only occur within a limited distance to the target
#define MAX_DISTANCE_FOR_MESSY_DEATH 7
// If you do a lot of damage with a close-range shot, instant kill
#define MIN_DAMAGE_FOR_INSTANT_KILL 55
// If you happen to kill someone with a close-range shot doing a lot of damage
// to the head, head explosion
#define MIN_DAMAGE_FOR_HEAD_EXPLOSION 45
// If you happen to kill someone with a close-range shot doing a lot of damage
// to the chest, chest explosion This value is lower than head because of the
// damage bonus for shooting the head
#define MIN_DAMAGE_FOR_BLOWN_AWAY 30
// If you happen to hit someone in the legs for enough damage, REGARDLESS of
// distance, person falls down Leg damage is halved for these purposes
#define MIN_DAMAGE_FOR_AUTO_FALL_OVER 20

// short range at which being prone provides to hit penalty when shooting
// standing people
#define MIN_PRONE_RANGE 50

// can't miss at this range?
#define POINT_BLANK_RANGE 16

#define BODY_IMPACT_ABSORPTION 20

#define BUCKSHOT_SHOTS 9

#define MIN_MORTAR_RANGE 150  // minimum range of a mortar

// WEAPON CLASSES
enum {
  NOGUNCLASS,
  HANDGUNCLASS,
  SMGCLASS,
  RIFLECLASS,
  MGCLASS,
  SHOTGUNCLASS,
  KNIFECLASS,
  MONSTERCLASS,
  NUM_WEAPON_CLASSES
};

// exact gun types
enum {
  NOT_GUN = 0,
  GUN_PISTOL,
  GUN_M_PISTOL,
  GUN_SMG,
  GUN_RIFLE,
  GUN_SN_RIFLE,
  GUN_AS_RIFLE,
  GUN_LMG,
  GUN_SHOTGUN
};

// ARMOUR CLASSES
enum {
  ARMOURCLASS_HELMET,
  ARMOURCLASS_VEST,
  ARMOURCLASS_LEGGINGS,
  ARMOURCLASS_PLATE,
  ARMOURCLASS_MONST,
  ARMOURCLASS_VEHICLE
};

// Warning: There is a table in weapons.c that indexes using these
// enumberations... BurstSndStrings[]....
enum AmmoKind {
  NOAMMO = 0,
  AMMO38,
  AMMO9,
  AMMO45,
  AMMO357,
  AMMO12G,
  AMMOCAWS,
  AMMO545,
  AMMO556,
  AMMO762N,
  AMMO762W,
  AMMO47,
  AMMO57,
  AMMOMONST,
  AMMOROCKET,
  AMMODART,
  AMMOFLAME,
};

enum {
  AMMO_REGULAR = 0,
  AMMO_HP,
  AMMO_AP,
  AMMO_SUPER_AP,
  AMMO_BUCKSHOT,
  AMMO_FLECHETTE,
  AMMO_GRENADE,
  AMMO_MONSTER,
  AMMO_KNIFE,
  AMMO_HE,
  AMMO_HEAT,
  AMMO_SLEEP_DART,
  AMMO_FLAME,
};

enum {
  EXPLOSV_NORMAL,
  EXPLOSV_STUN,
  EXPLOSV_TEARGAS,
  EXPLOSV_MUSTGAS,
  EXPLOSV_FLARE,
  EXPLOSV_NOISE,
  EXPLOSV_SMOKE,
  EXPLOSV_CREATUREGAS,
};

#define AMMO_DAMAGE_ADJUSTMENT_BUCKSHOT(x) (x / 4)
#define NUM_BUCKSHOT_PELLETS 9

// hollow point bullets do lots of damage to people
#define AMMO_DAMAGE_ADJUSTMENT_HP(x) ((x * 17) / 10)
// but they SUCK at penetrating armour
#define AMMO_ARMOUR_ADJUSTMENT_HP(x) ((x * 3) / 2)
// armour piercing bullets are good at penetrating armour
#define AMMO_ARMOUR_ADJUSTMENT_AP(x) ((x * 3) / 4)
// "super" AP bullets are great at penetrating armour
#define AMMO_ARMOUR_ADJUSTMENT_SAP(x) (x / 2)

// high explosive damage value (PRIOR to armour subtraction)
#define AMMO_DAMAGE_ADJUSTMENT_HE(x) ((x * 4) / 3)

// but they SUCK at penetrating armour
#define AMMO_STRUCTURE_ADJUSTMENT_HP(x) (x * 2)
// armour piercing bullets are good at penetrating structure
#define AMMO_STRUCTURE_ADJUSTMENT_AP(x) ((x * 3) / 4)
// "super" AP bullets are great at penetrating structures
#define AMMO_STRUCTURE_ADJUSTMENT_SAP(x) (x / 2)

// one quarter of punching damage is "real" rather than breath damage
#define PUNCH_REAL_DAMAGE_PORTION 4

#define AIM_BONUS_SAME_TARGET 10  // chance-to-hit bonus (in %)
#define AIM_BONUS_PER_AP 10       // chance-to-hit bonus (in %) for aim
#define AIM_BONUS_CROUCHING 10
#define AIM_BONUS_PRONE 20
#define AIM_BONUS_TWO_HANDED_PISTOL 5
#define AIM_BONUS_FIRING_DOWN 15
#define AIM_PENALTY_ONE_HANDED_PISTOL 5
#define AIM_PENALTY_DUAL_PISTOLS 20
#define AIM_PENALTY_SMG 5
#define AIM_PENALTY_GASSED 50
#define AIM_PENALTY_GETTINGAID 20
#define AIM_PENALTY_PER_SHOCK 5  // 5% penalty per point of shock
#define AIM_BONUS_TARGET_HATED 20
#define AIM_BONUS_PSYCHO 15
#define AIM_PENALTY_TARGET_BUDDY 20
#define AIM_PENALTY_BURSTING 10
#define AIM_PENALTY_GENTLEMAN 15
#define AIM_PENALTY_TARGET_CROUCHED 20
#define AIM_PENALTY_TARGET_PRONE 40
#define AIM_PENALTY_BLIND 80
#define AIM_PENALTY_FIRING_UP 25

struct WEAPONTYPE {
  uint8_t ubWeaponClass;     // handgun/shotgun/rifle/knife
  uint8_t ubWeaponType;      // exact type (for display purposes)
  AmmoKind ubCalibre;        // type of ammunition needed
  uint8_t ubReadyTime;       // APs to ready/unready weapon
  uint8_t ubShotsPer4Turns;  // maximum (mechanical) firing rate
  uint8_t ubShotsPerBurst;
  uint8_t ubBurstPenalty;  // % penalty per shot after first
  uint8_t ubBulletSpeed;   // bullet's travelling speed
  uint8_t ubImpact;        // weapon's max damage impact (size & speed)
  uint8_t ubDeadliness;    // comparative ratings of guns
  uint8_t ubMagSize;
  uint16_t usRange;
  uint16_t usReloadDelay; /**< not used */
  uint8_t ubAttackVolume;
  uint8_t ubHitVolume;
  SoundID sSound;
  SoundID sBurstSound;
  SoundID sReloadSound;
  SoundID sLocknLoadSound;
};

struct MAGTYPE {
  AmmoKind ubCalibre;
  uint8_t ubMagSize;
  uint8_t ubAmmoType;
};

struct ARMOURTYPE {
  uint8_t ubArmourClass;
  uint8_t ubProtection;
  uint8_t ubDegradePercent;
};

struct EXPLOSIVETYPE {
  uint8_t ubType;         // type of explosive
  uint8_t ubDamage;       // damage value
  uint8_t ubStunDamage;   // stun amount / 100
  uint8_t ubRadius;       // radius of effect
  uint8_t ubVolume;       // sound radius of explosion
  uint8_t ubVolatility;   // maximum chance of accidental explosion
  uint8_t ubAnimationID;  // Animation enum to use
};

// GLOBALS

extern WEAPONTYPE const Weapon[];
extern ARMOURTYPE const Armour[];
extern MAGTYPE const Magazine[];
extern EXPLOSIVETYPE const Explosive[];

int8_t EffectiveArmour(const OBJECTTYPE *pObj);
extern int8_t ArmourVersusExplosivesPercent(SOLDIERTYPE *pSoldier);
extern BOOLEAN FireWeapon(SOLDIERTYPE *pSoldier, int16_t sTargetGridNo);
void WeaponHit(SOLDIERTYPE *target, uint16_t usWeaponIndex, int16_t sDamage, int16_t sBreathLoss,
               uint16_t usDirection, int16_t sXPos, int16_t sYPos, int16_t sZPos, int16_t sRange,
               SOLDIERTYPE *attacker, uint8_t ubSpecial, uint8_t ubHitLocation);
void StructureHit(BULLET *b, int16_t sXPos, int16_t sYPos, int16_t sZPos, uint16_t usStructureID,
                  int32_t iImpact, BOOLEAN fStopped);
extern void WindowHit(int16_t sGridNo, uint16_t usStructureID, BOOLEAN fBlowWindowSouth,
                      BOOLEAN fLargeForce);
extern int32_t BulletImpact(SOLDIERTYPE *pFirer, SOLDIERTYPE *pTarget, uint8_t ubHitLocation,
                            int32_t iImpact, int16_t sHitBy, uint8_t *pubSpecial);
BOOLEAN InRange(const SOLDIERTYPE *pSoldier, int16_t sGridNo);
void ShotMiss(const BULLET *b);
extern uint32_t CalcChanceToHitGun(SOLDIERTYPE *pSoldier, uint16_t sGridNo, uint8_t ubAimTime,
                                   uint8_t ubAimPos);
extern uint32_t AICalcChanceToHitGun(SOLDIERTYPE *pSoldier, uint16_t sGridNo, uint8_t ubAimTime,
                                     uint8_t ubAimPos);
extern uint32_t CalcChanceToPunch(SOLDIERTYPE *pAttacker, SOLDIERTYPE *pDefender,
                                  uint8_t ubAimTime);
extern uint32_t CalcChanceToStab(SOLDIERTYPE *pAttacker, SOLDIERTYPE *pDefender, uint8_t ubAimTime);
void ReloadWeapon(SOLDIERTYPE *, uint8_t inv_pos);
bool IsGunBurstCapable(SOLDIERTYPE const *, uint8_t inv_pos);
extern int32_t CalcBodyImpactReduction(uint8_t ubAmmoType, uint8_t ubHitLocation);
int32_t TotalArmourProtection(SOLDIERTYPE &, uint8_t ubHitLocation, int32_t iImpact,
                              uint8_t ubAmmoType);
int8_t ArmourPercent(const SOLDIERTYPE *pSoldier);

extern void GetTargetWorldPositions(SOLDIERTYPE *pSoldier, int16_t sTargetGridNo, float *pdXPos,
                                    float *pdYPos, float *pdZPos);

extern BOOLEAN OKFireWeapon(SOLDIERTYPE *pSoldier);
extern BOOLEAN CheckForGunJam(SOLDIERTYPE *pSoldier);

int32_t CalcMaxTossRange(const SOLDIERTYPE *pSoldier, uint16_t usItem, BOOLEAN fArmed);
extern uint32_t CalcThrownChanceToHit(SOLDIERTYPE *pSoldier, int16_t sGridNo, uint8_t ubAimTime,
                                      uint8_t ubAimPos);

extern void ChangeWeaponMode(SOLDIERTYPE *pSoldier);

void UseHandToHand(SOLDIERTYPE *pSoldier, int16_t sTargetGridNo, BOOLEAN fStealing);

void DishoutQueenSwipeDamage(SOLDIERTYPE *pQueenSoldier);

int32_t HTHImpact(const SOLDIERTYPE *pSoldier, const SOLDIERTYPE *pTarget, int32_t iHitBy,
                  BOOLEAN fBladeAttack);

uint16_t GunRange(OBJECTTYPE const &);

extern BOOLEAN gfNextFireJam;
extern BOOLEAN gfNextShotKills;
extern BOOLEAN gfReportHitChances;

#endif
