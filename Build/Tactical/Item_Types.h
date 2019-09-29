#ifndef ITEM_TYPES_H
#define ITEM_TYPES_H

#include "sgp/Types.h"

enum ItemCursor
{
	INVALIDCURS    =  0,
	QUESTCURS      =  1,
	PUNCHCURS      =  2,
	TARGETCURS     =  3,
	KNIFECURS      =  4,
	AIDCURS        =  5,
	TOSSCURS       =  6,
	MINECURS       =  8,
	LPICKCURS      =  9,
	MDETECTCURS    = 10,
	CROWBARCURS    = 11,
	SURVCAMCURS    = 12,
	CAMERACURS     = 13,
	KEYCURS        = 14,
	SAWCURS        = 15,
	WIRECUTCURS    = 16,
	REMOTECURS     = 17,
	BOMBCURS       = 18, // (only calculated, not set item table)
	REPAIRCURS     = 19,
	TRAJECTORYCURS = 20,
	JARCURS        = 21,
	TINCANCURS     = 22,
	REFUELCURS     = 23
};

#define ITEM_NOT_FOUND -1


#define USABLE          10      // minimum work% of items to still be usable

#define MAX_OBJECTS_PER_SLOT 8
#define MAX_ATTACHMENTS 4
#define MAX_MONEY_PER_SLOT 20000

enum DetonatorType
{
	BOMB_TIMED = 1,
	BOMB_REMOTE,
	BOMB_PRESSURE,
	BOMB_SWITCH
};

#define FIRST_MAP_PLACED_FREQUENCY 50
#define PANIC_FREQUENCY 127
#define PANIC_FREQUENCY_2 126
#define PANIC_FREQUENCY_3 125

#define OBJECT_UNDROPPABLE					0x01
#define OBJECT_MODIFIED							0x02
#define OBJECT_AI_UNUSABLE					0x04
#define OBJECT_ARMED_BOMB						0x08
#define OBJECT_KNOWN_TO_BE_TRAPPED	0x10
#define OBJECT_DISABLED_BOMB				0x20
#define OBJECT_ALARM_TRIGGER				0x40
#define OBJECT_NO_OVERWRITE					0x80

struct OBJECTTYPE
{
	UINT16	usItem;
	UINT8		ubNumberOfObjects;
	union
	{
		struct
		{
			INT8		bGunStatus;			// status % of gun
			UINT8		ubGunAmmoType;	// ammo type, as per weapons.h
			UINT8		ubGunShotsLeft;	// duh, amount of ammo left
			UINT16	usGunAmmoItem;	// the item # for the item table
			INT8		bGunAmmoStatus; // only for "attached ammo" - grenades, mortar shells
			UINT8		ubGunUnused[MAX_OBJECTS_PER_SLOT - 6]; // XXX HACK000B
		};
		struct
		{
			UINT8		ubShotsLeft[MAX_OBJECTS_PER_SLOT];
		};
		struct
		{
			INT8		bStatus[MAX_OBJECTS_PER_SLOT];
		};
		struct
		{
			INT8		bMoneyStatus;
			UINT32	uiMoneyAmount;
			UINT8		ubMoneyUnused[MAX_OBJECTS_PER_SLOT - 5]; // XXX HACK000B
		};
		struct
		{ // this is used by placed bombs, switches, and the action item
			INT8		bBombStatus;			// % status
			INT8		bDetonatorType;		// timed, remote, or pressure-activated
			UINT16	usBombItem;				// the usItem of the bomb.
			union
			{
				struct
				{
					INT8		bDelay;				// >=0 values used only
				};
				struct
				{
					INT8		bFrequency;		// >=0 values used only
				};
			};
			UINT8 ubBombOwner; // side which placed the bomb
			UINT8	bActionValue;// this is used by the ACTION_ITEM fake item
			UINT8 ubTolerance; // tolerance value for panic triggers
		};
		struct
		{
			INT8 bKeyStatus[ 6 ];
			UINT8 ubKeyID;
			UINT8 ubKeyUnused[1]; // XXX HACK000B
		};
		struct
		{
			UINT8 ubOwnerProfile;
			UINT8 ubOwnerCivGroup;
			UINT8 ubOwnershipUnused[6]; // XXX HACK000B
		};
	};
  // attached objects
	UINT16	usAttachItem[MAX_ATTACHMENTS];
	INT8		bAttachStatus[MAX_ATTACHMENTS];

	INT8		fFlags;
	UINT8		ubMission;
	INT8		bTrap;        // 1-10 exp_lvl to detect
	UINT8		ubImprintID;	// ID of merc that item is imprinted on
	UINT8		ubWeight;
	UINT8		fUsed;				// flags for whether the item is used or not
};


// SUBTYPES
#define IC_NONE						0x00000001
#define IC_GUN						0x00000002
#define IC_BLADE					0x00000004
#define IC_THROWING_KNIFE	0x00000008

#define IC_LAUNCHER				0x00000010
#define IC_TENTACLES			0x00000020

#define IC_THROWN					0x00000040
#define IC_PUNCH					0x00000080

#define IC_GRENADE				0x00000100
#define IC_BOMB						0x00000200
#define IC_AMMO						0x00000400
#define IC_ARMOUR					0x00000800

#define IC_MEDKIT					0x00001000
#define IC_KIT						0x00002000
#define IC_FACE           0x00008000

#define IC_KEY						0x00010000

#define IC_MISC						0x10000000
#define IC_MONEY					0x20000000

// PARENT TYPES
#define IC_ALL            0xFFFFFFFF

#define IC_WEAPON					( IC_GUN | IC_BLADE | IC_THROWING_KNIFE | IC_LAUNCHER | IC_TENTACLES )
#define IC_EXPLOSV				( IC_GRENADE | IC_BOMB )

#define IC_BOBBY_GUN			( IC_GUN | IC_LAUNCHER )
#define IC_BOBBY_MISC			( IC_GRENADE | IC_BOMB | IC_MISC | IC_MEDKIT | IC_KIT | IC_BLADE | IC_THROWING_KNIFE | IC_PUNCH | IC_FACE )


// replaces candamage
#define ITEM_DAMAGEABLE			0x0001
// replaces canrepair
#define ITEM_REPAIRABLE			0x0002
// replaces waterdamage
#define ITEM_WATER_DAMAGES	0x0004
// replaces metal
#define ITEM_METAL					0x0008
// replaces sinkable
#define ITEM_SINKS					0x0010
// replaces seemeter
#define ITEM_SHOW_STATUS		0x0020
// for attachers/merges, hidden
#define ITEM_HIDDEN_ADDON		0x0040
// needs two hands
#define ITEM_TWO_HANDED			0x0080
// can't be found for sale
#define ITEM_NOT_BUYABLE		0x0100
// item is an attachment for something
#define ITEM_ATTACHMENT			0x0200
// item only belongs in the "big gun list"
#define ITEM_BIGGUNLIST			0x0400
// item should not be placed via the editor
#define ITEM_NOT_EDITOR			0x0800
// item defaults to undroppable
#define ITEM_DEFAULT_UNDROPPABLE	0x1000
// item is terrible for throwing
#define ITEM_UNAERODYNAMIC	0x2000
// item is electronic for repair (etc) purposes
#define ITEM_ELECTRONIC			0x4000
// item is a PERMANENT attachment
#define ITEM_INSEPARABLE		0x8000

// item flag combinations

#define IF_STANDARD_GUN ITEM_DAMAGEABLE | ITEM_WATER_DAMAGES | ITEM_REPAIRABLE | ITEM_SHOW_STATUS | ITEM_METAL | ITEM_SINKS
#define IF_TWOHANDED_GUN IF_STANDARD_GUN | ITEM_TWO_HANDED
#define IF_STANDARD_BLADE ITEM_DAMAGEABLE | ITEM_WATER_DAMAGES | ITEM_REPAIRABLE | ITEM_SHOW_STATUS | ITEM_METAL | ITEM_SINKS
#define IF_STANDARD_ARMOUR ITEM_DAMAGEABLE | ITEM_REPAIRABLE | ITEM_SHOW_STATUS | ITEM_SINKS
#define IF_STANDARD_KIT ITEM_DAMAGEABLE | ITEM_SHOW_STATUS | ITEM_SINKS
#define IF_STANDARD_CLIP ITEM_SINKS | ITEM_METAL

#define EXPLOSIVE_GUN( x ) ( x == ROCKET_LAUNCHER || x == TANK_CANNON )

#define FIRST_WEAPON 1
#define LAST_WEAPON 70
#define FIRST_AMMO 71
#define LAST_AMMO 130
#define MAX_AMMO (LAST_AMMO - FIRST_AMMO + 1)
#define FIRST_EXPLOSIVE 131
#define FIRST_ARMOUR 161
#define FIRST_KEY 271

#define NOTHING NONE
enum ITEMDEFINE
{
	NONE									= 0,

	// weapons
	__ITEM_1							= FIRST_WEAPON,
	__ITEM_2,
	__ITEM_3,
	__ITEM_4,
	__ITEM_5,
	__ITEM_6,
	__ITEM_7,
	__ITEM_8,
	__ITEM_9,
	__ITEM_10,

	__ITEM_11,
	__ITEM_12,
	__ITEM_13,
	__ITEM_14,
	__ITEM_15,
	__ITEM_16,
	__ITEM_17,
	__ITEM_18,
	__ITEM_19,
	__ITEM_20,

	__ITEM_21,
	__ITEM_22,
	__ITEM_23,
	__ITEM_24,
	__ITEM_25,
	__ITEM_26,
	__ITEM_27,
	__ITEM_28,
	__ITEM_29,
	__ITEM_30,

	__ITEM_31,
	__ITEM_32,
	__ITEM_33,
	__ITEM_34,
	__ITEM_35,
	__ITEM_36,
	COMBAT_KNIFE,
	THROWING_KNIFE,
	ROCK,
	GLAUNCHER,

	MORTAR,
	ROCK2,
	CREATURE_YOUNG_MALE_CLAWS,
	CREATURE_OLD_MALE_CLAWS,
	CREATURE_YOUNG_FEMALE_CLAWS,
	CREATURE_OLD_FEMALE_CLAWS,
	CREATURE_QUEEN_TENTACLES,
	CREATURE_QUEEN_SPIT,
	BRASS_KNUCKLES,
	UNDER_GLAUNCHER,

	ROCKET_LAUNCHER,
	BLOODCAT_CLAW_ATTACK,
	BLOODCAT_BITE,
	__ITEM_54,
	ROCKET_RIFLE,
	__ITEM_56,
	CREATURE_INFANT_SPIT,
	CREATURE_YOUNG_MALE_SPIT,
	CREATURE_OLD_MALE_SPIT,
	TANK_CANNON,

	DART_GUN,
	BLOODY_THROWING_KNIFE,
	__ITEM_63,
	CROWBAR,
	AUTO_ROCKET_RIFLE,
  __ITEM_66,      // NOTHING,
  __ITEM_67,      // NOTHING,
  __ITEM_68,      // NOTHING,
  __ITEM_69,      // NOTHING,
  __ITEM_70,      // NOTHING,
	MAX_WEAPONS							= ( FIRST_AMMO - 1 ),

	__ITEM_71 = FIRST_AMMO,
	__ITEM_72,
	__ITEM_73,
	__ITEM_74,
	__ITEM_75,
	__ITEM_76,
	__ITEM_77,
	__ITEM_78,
	__ITEM_79,
	__ITEM_80,

	__ITEM_81,
	__ITEM_82,
	__ITEM_83,
	__ITEM_84,
	__ITEM_85,
	__ITEM_86,
	__ITEM_87,
	__ITEM_88,
	__ITEM_89,
	__ITEM_90,

	__ITEM_91,
	__ITEM_92,
	__ITEM_93,
	__ITEM_94,
	__ITEM_95,
	__ITEM_96,
	__ITEM_97,
	__ITEM_98,
	__ITEM_99,
	__ITEM_100,

	__ITEM_101,
	__ITEM_102,
	__ITEM_103,
	__ITEM_104,
	__ITEM_105,
	__ITEM_106,
	__ITEM_107,
	__ITEM_108,
	__ITEM_109,
	__ITEM_110,

	__ITEM_111,
	__ITEM_112,
	__ITEM_113,
	__ITEM_114,

	__ITEM_115,
  __ITEM_116,
  __ITEM_117,
  __ITEM_118,
  __ITEM_119,
  __ITEM_120,
  __ITEM_121,
  __ITEM_122,
  __ITEM_123,
  __ITEM_124,
  __ITEM_125,
  __ITEM_126,
  __ITEM_127,
  __ITEM_128,
  __ITEM_129,
  __ITEM_130,

	// explosives
	STUN_GRENADE = FIRST_EXPLOSIVE,
	TEARGAS_GRENADE,
	MUSTARD_GRENADE,
	MINI_GRENADE,
	HAND_GRENADE,
	RDX,
	TNT,
	HMX,
	C1,
	MORTAR_SHELL,

	MINE,
	C4,
	TRIP_FLARE,
	TRIP_KLAXON,
	SHAPED_CHARGE,
	BREAK_LIGHT,
	GL_HE_GRENADE,
	GL_TEARGAS_GRENADE,
	GL_STUN_GRENADE,
	GL_SMOKE_GRENADE,

	SMOKE_GRENADE,
	TANK_SHELL,
	STRUCTURE_IGNITE,
	CREATURE_COCKTAIL,
	STRUCTURE_EXPLOSION,
	GREAT_BIG_EXPLOSION,
	BIG_TEAR_GAS,
	SMALL_CREATURE_GAS,
	LARGE_CREATURE_GAS,
  VERY_SMALL_CREATURE_GAS,

	// armor
	FLAK_JACKET,
	FLAK_JACKET_18,
	FLAK_JACKET_Y,
	KEVLAR_VEST,
	KEVLAR_VEST_18,
	KEVLAR_VEST_Y,
	SPECTRA_VEST,
	SPECTRA_VEST_18,
	SPECTRA_VEST_Y,
	KEVLAR_LEGGINGS,

	KEVLAR_LEGGINGS_18,
	KEVLAR_LEGGINGS_Y,
	SPECTRA_LEGGINGS,
	SPECTRA_LEGGINGS_18,
	SPECTRA_LEGGINGS_Y,
	STEEL_HELMET,
	KEVLAR_HELMET,
	KEVLAR_HELMET_18,
	KEVLAR_HELMET_Y,
	SPECTRA_HELMET,

	SPECTRA_HELMET_18,
	SPECTRA_HELMET_Y,
	CERAMIC_PLATES,
	CREATURE_INFANT_HIDE,
	CREATURE_YOUNG_MALE_HIDE,
	CREATURE_OLD_MALE_HIDE,
	CREATURE_QUEEN_HIDE,
	LEATHER_JACKET,
	LEATHER_JACKET_W_KEVLAR,
	LEATHER_JACKET_W_KEVLAR_18,

	LEATHER_JACKET_W_KEVLAR_Y,
	CREATURE_YOUNG_FEMALE_HIDE,
	CREATURE_OLD_FEMALE_HIDE,
	TSHIRT,
	TSHIRT_DEIDRANNA,
	KEVLAR2_VEST,
	KEVLAR2_VEST_18,
	KEVLAR2_VEST_Y,
  __ITEM_199,
  __ITEM_200,

	// kits
	FIRSTAIDKIT,
	MEDICKIT,
	TOOLKIT,
	LOCKSMITHKIT,
	CAMOUFLAGEKIT,
	BOOBYTRAPKIT,
	SILENCER,
	SNIPERSCOPE,
	BIPOD,
	EXTENDEDEAR,

	NIGHTGOGGLES,
	SUNGOGGLES,
	GASMASK,
	CANTEEN,
	METALDETECTOR,
	COMPOUND18,
	JAR_QUEEN_CREATURE_BLOOD,
	JAR_ELIXIR,
	MONEY,
	JAR,

	JAR_CREATURE_BLOOD,
	ADRENALINE_BOOSTER,
	DETONATOR,
	REMDETONATOR,
	__ITEM_225,     // VIDEOTAPE,
	DEED,
	LETTER,
	__ITEM_228,     // TERRORIST_INFO,
	CHALICE,
	BLOODCAT_CLAWS,

	BLOODCAT_TEETH,
	BLOODCAT_PELT,
	SWITCH,
	ACTION_ITEM,
	REGEN_BOOSTER,
	SYRINGE_3,
	SYRINGE_4,
	SYRINGE_5,
	JAR_HUMAN_BLOOD,
	OWNERSHIP,

	// additional items
	LASERSCOPE,
	REMOTEBOMBTRIGGER,
	WIRECUTTERS,
	DUCKBILL,
	ALCOHOL,
	UVGOGGLES,
	DISCARDED_LAW,
	HEAD_1,
	HEAD_2,
	HEAD_3,
	HEAD_4,
	HEAD_5,
	HEAD_6,
	HEAD_7,
	WINE,
	BEER,
	__ITEM_257,     // PORNOS,
	VIDEO_CAMERA,
	ROBOT_REMOTE_CONTROL,
	CREATURE_PART_CLAWS,
	CREATURE_PART_FLESH,
	CREATURE_PART_ORGAN,
	REMOTETRIGGER,
	GOLDWATCH,
	__ITEM_265,     // GOLFCLUBS,
	WALKMAN,
	__ITEM_267,     // PORTABLETV,
	MONEY_FOR_PLAYERS_ACCOUNT,
	CIGARS,
  __ITEM_270,     // NOTHING

	KEY_1 = FIRST_KEY,
	__ITEM_272,     // KEY_2,
	__ITEM_273,     // KEY_3,
	__ITEM_274,     // KEY_4,
	__ITEM_275,     // KEY_5,
	__ITEM_276,     // KEY_6,
	__ITEM_277,     // KEY_7,
	__ITEM_278,     // KEY_8,
	__ITEM_279,     // KEY_9,
	__ITEM_280,     // KEY_10,

	__ITEM_281,     // KEY_11,
	__ITEM_282,     // KEY_12,
	__ITEM_283,     // KEY_13,
	__ITEM_284,     // KEY_14,
	__ITEM_285,     // KEY_15,
	__ITEM_286,     // KEY_16,
	__ITEM_287,     // KEY_17,
	__ITEM_288,     // KEY_18,
	__ITEM_289,     // KEY_19,
	__ITEM_290,     // KEY_20,

	__ITEM_291,     // KEY_21,
	__ITEM_292,     // KEY_22,
	__ITEM_293,     // KEY_23,
	__ITEM_294,     // KEY_24,
	__ITEM_295,     // KEY_25,
	__ITEM_296,     // KEY_26,
	__ITEM_297,     // KEY_27,
	__ITEM_298,     // KEY_28,
	__ITEM_299,     // KEY_29,
	__ITEM_300,     // KEY_30,

	__ITEM_301,     // KEY_31,
	KEY_32,
	SILVER_PLATTER,
	DUCT_TAPE,
	ALUMINUM_ROD,
	SPRING,
	SPRING_AND_BOLT_UPGRADE,
	STEEL_ROD,
	QUICK_GLUE,
	GUN_BARREL_EXTENDER,

	STRING,
	TIN_CAN,
	STRING_TIED_TO_TIN_CAN,
	MARBLES,
	LAME_BOY,
	COPPER_WIRE,
	DISPLAY_UNIT,
	FUMBLE_PAK,
	XRAY_BULB,
	CHEWING_GUM,

	FLASH_DEVICE,
	BATTERIES,
	__ITEM_323,     // ELASTIC,
	XRAY_DEVICE,
	SILVER,
	GOLD,
	GAS_CAN,
	__ITEM_328,     // UNUSED_26,
	__ITEM_329,     // UNUSED_27,
	__ITEM_330,     // UNUSED_28,

	__ITEM_331,     // UNUSED_29,
	__ITEM_332,     // UNUSED_30,
	__ITEM_333,     // UNUSED_31,
	__ITEM_334,     // UNUSED_32,
	__ITEM_335,     // UNUSED_33,
	__ITEM_336,     // UNUSED_34,
	__ITEM_337,     // UNUSED_35,
	__ITEM_338,     // UNUSED_36,
	__ITEM_339,     // UNUSED_37,
	__ITEM_340,     // UNUSED_38,

	__ITEM_341,     // UNUSED_39,
	__ITEM_342,     // UNUSED_40,
	__ITEM_343,     // UNUSED_41,
	__ITEM_344,     // UNUSED_42,
	__ITEM_345,     // UNUSED_43,
	__ITEM_346,     // UNUSED_44,
	__ITEM_347,     // UNUSED_45,
	__ITEM_348,     // UNUSED_46,
	__ITEM_349,     // UNUSED_47,
	__ITEM_350,     // UNUSED_48,

	MAXITEMS
};

#define FIRST_HELMET STEEL_HELMET
#define LAST_HELMET SPECTRA_HELMET_Y

#define FIRST_VEST FLAK_JACKET
#define LAST_VEST KEVLAR2_VEST_Y

#define FIRST_LEGGINGS KEVLAR_LEGGINGS
#define LAST_LEGGINGS SPECTRA_LEGGINGS_Y

#define FIRST_HEAD_ITEM EXTENDEDEAR
#define LAST_HEAD_ITEM SUNGOGGLES

#endif
