// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Tactical/Weapons.h"

#include <algorithm>
#include <stdio.h>
#include <string.h>

#include "Directories.h"
#include "GameSettings.h"
#include "Macro.h"
#include "SGP/Debug.h"
#include "SGP/MemMan.h"
#include "SGP/Random.h"
#include "SGP/SoundMan.h"
#include "Strategic/AutoResolve.h"
#include "Strategic/Meanwhile.h"
#include "Strategic/Quests.h"
#include "Tactical/AnimationControl.h"
#include "Tactical/Bullets.h"
#include "Tactical/Campaign.h"
#include "Tactical/DialogueControl.h"
#include "Tactical/HandleItems.h"
#include "Tactical/HandleUI.h"
#include "Tactical/Interface.h"
#include "Tactical/Items.h"
#include "Tactical/LOS.h"
#include "Tactical/Morale.h"
#include "Tactical/OppList.h"
#include "Tactical/Overhead.h"
#include "Tactical/OverheadTypes.h"
#include "Tactical/Points.h"
#include "Tactical/SkillCheck.h"
#include "Tactical/SoldierControl.h"
#include "Tactical/SoldierMacros.h"
#include "Tactical/SoldierProfile.h"
#include "Tactical/Vehicles.h"
#include "TacticalAI/AI.h"
#include "TileEngine/ExplosionControl.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/Physics.h"
#include "TileEngine/RenderWorld.h"
#include "TileEngine/SaveLoadMap.h"
#include "TileEngine/SmokeEffects.h"
#include "TileEngine/Structure.h"
#include "TileEngine/TileAnimation.h"
#include "TileEngine/TileDef.h"
#include "TileEngine/WorldMan.h"
#include "Utils/EventPump.h"
#include "Utils/FontControl.h"
#include "Utils/Message.h"
#include "Utils/Text.h"
#include "Utils/TimerControl.h"

#define MINCHANCETOHIT 1
#define MAXCHANCETOHIT 99

// NB this is arbitrary, chances in DG ranged from 1 in 6 to 1 in 20
#define BASIC_DEPRECIATE_CHANCE 15

#define NORMAL_RANGE 90     // # world units considered an 'avg' shot
#define MIN_SCOPE_RANGE 60  // # world units after which scope's useful

#define MIN_TANK_RANGE 120  // range at which tank starts really having trouble aiming

// percent reduction in sight range per point of aiming
#define SNIPERSCOPE_AIM_BONUS 20
// bonus to hit with working laser scope
#define LASERSCOPE_BONUS 20

#define NO_WEAPON_SOUND NO_SOUND

#define HEAD_DAMAGE_ADJUSTMENT(x) ((x * 3) / 2)
#define LEGS_DAMAGE_ADJUSTMENT(x) (x / 2)

#define CRITICAL_HIT_THRESHOLD 30

#define HTH_MODE_PUNCH 1
#define HTH_MODE_STAB 2
#define HTH_MODE_STEAL 3

// JA2 GOLD: for weapons and attachments, give penalties only for status values
// below 85
#define WEAPON_STATUS_MOD(x) ((x) >= 85 ? 100 : (((x) * 100) / 85))

BOOLEAN gfNextFireJam = FALSE;
BOOLEAN gfNextShotKills = FALSE;
BOOLEAN gfReportHitChances = FALSE;

// GLOBALS

// TODO: Move strings to extern file

#define NOWEAPON(range) \
  {NOGUNCLASS,          \
   NOT_GUN,             \
   NOAMMO,              \
   0,                   \
   0,                   \
   0,                   \
   0,                   \
   0,                   \
   0,                   \
   0,                   \
   range,               \
   0,                   \
   0,                   \
   0,                   \
   0,                   \
   NO_WEAPON_SOUND,     \
   NO_WEAPON_SOUND,     \
   NO_WEAPON_SOUND,     \
   NO_WEAPON_SOUND}  // XXX it is magazine size, not range
#define PISTOL(ammo, update, impact, rt, rof, deadl, clip, range, av, hv, sd) \
  {HANDGUNCLASS,    GUN_PISTOL,      ammo,        rt,    rof, 0,  0,  update, \
   impact,          deadl,           clip,        range, 200, av, hv, sd,     \
   NO_WEAPON_SOUND, S_RELOAD_PISTOL, S_LNL_PISTOL}
#define M_PISTOL(ammo, update, impact, rt, rof, burstrof, burstpenal, deadl, clip, range, av, hv, \
                 sd, bsd)                                                                         \
  {HANDGUNCLASS, GUN_M_PISTOL,    ammo,        rt,    rof, burstrof, burstpenal, update,          \
   impact,       deadl,           clip,        range, 200, av,       hv,         sd,              \
   bsd,          S_RELOAD_PISTOL, S_LNL_PISTOL}
#define SMG(ammo, update, impact, rt, rof, burstrof, burstpenal, deadl, clip, range, av, hv, sd, \
            bsd)                                                                                 \
  {SMGCLASS, GUN_SMG, ammo, rt, rof, burstrof, burstpenal, update,       impact,   deadl,        \
   clip,     range,   200,  av, hv,  sd,       bsd,        S_RELOAD_SMG, S_LNL_SMG}
#define SN_RIFLE(ammo, update, impact, rt, rof, deadl, clip, range, av, hv, sd) \
  {RIFLECLASS,      GUN_SN_RIFLE,   ammo,       rt,    rof, 0,  0,  update,     \
   impact,          deadl,          clip,       range, 200, av, hv, sd,         \
   NO_WEAPON_SOUND, S_RELOAD_RIFLE, S_LNL_RIFLE}
#define RIFLE(ammo, update, impact, rt, rof, deadl, clip, range, av, hv, sd) \
  {RIFLECLASS,      GUN_RIFLE,      ammo,       rt,    rof, 0,  0,  update,  \
   impact,          deadl,          clip,       range, 200, av, hv, sd,      \
   NO_WEAPON_SOUND, S_RELOAD_RIFLE, S_LNL_RIFLE}
#define ASRIFLE(ammo, update, impact, rt, rof, burstrof, burstpenal, deadl, clip, range, av, hv, \
                sd, bsd)                                                                         \
  {                                                                                              \
      RIFLECLASS, GUN_AS_RIFLE,   ammo,       rt,    rof, burstrof, burstpenal, update,          \
      impact,     deadl,          clip,       range, 200, av,       hv,         sd,              \
      bsd,        S_RELOAD_RIFLE, S_LNL_RIFLE}
#define SHOTGUN(ammo, update, impact, rt, rof, burstrof, burstpenal, deadl, clip, range, av, hv, \
                sd, bsd)                                                                         \
  {SHOTGUNCLASS, GUN_SHOTGUN, ammo,  rt,  rof, burstrof, burstpenal, update, impact,             \
   deadl,        clip,        range, 200, av,  hv,       sd,         bsd,    S_RELOAD_SHOTGUN,   \
   S_LNL_SHOTGUN}
#define LMG(ammo, update, impact, rt, rof, burstrof, burstpenal, deadl, clip, range, av, hv, sd, \
            bsd)                                                                                 \
  {MGCLASS, GUN_LMG, ammo, rt, rof, burstrof, burstpenal, update,       impact,   deadl,         \
   clip,    range,   200,  av, hv,  sd,       bsd,        S_RELOAD_LMG, S_LNL_LMG}
#define BLADE(impact, rof, deadl, range, av, sd) \
  {KNIFECLASS,                                   \
   NOT_GUN,                                      \
   NOAMMO,                                       \
   AP_READY_KNIFE,                               \
   rof,                                          \
   0,                                            \
   0,                                            \
   0,                                            \
   impact,                                       \
   deadl,                                        \
   0,                                            \
   range,                                        \
   200,                                          \
   av,                                           \
   0,                                            \
   sd,                                           \
   NO_WEAPON_SOUND,                              \
   NO_WEAPON_SOUND,                              \
   NO_WEAPON_SOUND}
#define THROWINGBLADE(impact, rof, deadl, range, av, sd) \
  {KNIFECLASS,                                           \
   NOT_GUN,                                              \
   NOAMMO,                                               \
   AP_READY_KNIFE,                                       \
   rof,                                                  \
   0,                                                    \
   0,                                                    \
   0,                                                    \
   impact,                                               \
   deadl,                                                \
   0,                                                    \
   range,                                                \
   200,                                                  \
   av,                                                   \
   0,                                                    \
   sd,                                                   \
   NO_WEAPON_SOUND,                                      \
   NO_WEAPON_SOUND,                                      \
   NO_WEAPON_SOUND}
#define PUNCHWEAPON(impact, rof, deadl, av, sd) \
  {KNIFECLASS,                                  \
   NOT_GUN,                                     \
   NOAMMO,                                      \
   0,                                           \
   rof,                                         \
   0,                                           \
   0,                                           \
   0,                                           \
   impact,                                      \
   deadl,                                       \
   0,                                           \
   10,                                          \
   200,                                         \
   av,                                          \
   0,                                           \
   sd,                                          \
   NO_WEAPON_SOUND,                             \
   NO_WEAPON_SOUND,                             \
   NO_WEAPON_SOUND}
#define LAUNCHER(update, rt, rof, deadl, range, av, hv, sd)                             \
  {RIFLECLASS,     NOT_GUN, NOAMMO, rt,  rof, 0,  0,  update,          1,               \
   deadl,          0,       range,  200, av,  hv, sd, NO_WEAPON_SOUND, NO_WEAPON_SOUND, \
   NO_WEAPON_SOUND}
#define LAW(update, rt, rof, deadl, range, av, hv, sd)                                  \
  {RIFLECLASS,     NOT_GUN, NOAMMO, rt,  rof, 0,  0,  update,          80,              \
   deadl,          1,       range,  200, av,  hv, sd, NO_WEAPON_SOUND, NO_WEAPON_SOUND, \
   NO_WEAPON_SOUND}
#define CANNON(update, rt, rof, deadl, range, av, hv, sd)                               \
  {RIFLECLASS,     NOT_GUN, NOAMMO, rt,  rof, 0,  0,  update,          80,              \
   deadl,          1,       range,  200, av,  hv, sd, NO_WEAPON_SOUND, NO_WEAPON_SOUND, \
   NO_WEAPON_SOUND}
#define MONSTSPIT(impact, rof, deadl, clip, range, av, hv, sd)                         \
  {MONSTERCLASS,    NOT_GUN,         AMMOMONST,      AP_READY_KNIFE, rof, 0,  0,  250, \
   impact,          deadl,           clip,           range,          200, av, hv, sd,  \
   NO_WEAPON_SOUND, NO_WEAPON_SOUND, NO_WEAPON_SOUND}

// ranges are in world units, calculated by:
// 100 + real-range-in-metres/10
// then I scaled them down... I forget how much by!

// JA2 GOLD: reduced pistol ready time to 0, tweaked sniper rifle values and G11
// range
WEAPONTYPE const Weapon[] = {
    /* Nothing           */ NOWEAPON(10),  // must have min range of 10

    /* Description                        Ammo        Bullet speed   Burst rate
     * of fire   Attack volume                    Burst sound |           |
     * Impact     |      Deadlineass   |   Impact volume                | | | |
     * Ready time    |    Clip size|   |   Sound                    | | |   | |
     * Rate of fire    |   Range|   |   |                        | |           |
     * |   |  |   |  Burst penalty|    |   |   |                        | | |
     * |   |  |   |  |   |    |   |    |   |   |                        | */
    /* Glock 17          */
    PISTOL(AMMO9, 24, 21, 0, 14, 8, 15, 120, 60, 5,
           S_GLOCK17),  // wt 6  // Austria
                        /* Glock 18          */
    M_PISTOL(AMMO9, 24, 21, 0, 14, 5, 15, 9, 15, 120, 60, 5, S_GLOCK18,
             S_BURSTTYPE1),  // wt 6  // Austria
                             /* Beretta 92F       */
    PISTOL(AMMO9, 23, 22, 0, 16, 9, 15, 120, 60, 5,
           S_BERETTA92),  // wt 11 // Italy
                          /* Beretta 93R       */
    M_PISTOL(AMMO9, 23, 22, 0, 13, 5, 15, 9, 15, 120, 60, 5, S_BERETTA93,
             S_BURSTTYPE1),  // wt 11 // Italy
                             /* .38 S&W Special   */
    PISTOL(AMMO38, 23, 22, 0, 11, 6, 6, 130, 63, 5,
           S_SWSPECIAL),  // wt 11 // Britain
                          /* .357 Barracuda    */
    PISTOL(AMMO357, 23, 24, 0, 11, 7, 6, 135, 66, 6,
           S_BARRACUDA),  // wt 10 // Belgium
                          /* .357 DesertEagle  */
    PISTOL(AMMO357, 24, 24, 0, 11, 7, 9, 135, 66, 6,
           S_DESERTEAGLE),                                     // wt 17 // US
                                                               /* .45 M1911         */
    PISTOL(AMMO45, 24, 23, 0, 13, 9, 7, 125, 69, 6, S_M1911),  // wt 12 // US

    /* H&K MP5K          */
    SMG(AMMO9, 23, 23, 1, 15, 5, 8, 17, 30, 200, 75, 7, S_MP5K,
        S_BURSTTYPE1),  // wt 21 // Germany; ROF 900 ?
                        /* .45 MAC-10        */
    SMG(AMMO45, 23, 27, 2, 13, 5, 8, 20, 30, 200, 75, 7, S_MAC10,
        S_BURSTTYPE1),  // wt 28 // US; ROF 1090
                        /* Thompson M1A1     */
    SMG(AMMO45, 23, 24, 2, 10, 4, 8, 14, 30, 200, 75, 7, S_THOMPSON,
        S_BURSTTYPE1),  // wt 48 // US; ROF 700
                        /* Colt Commando     */
    SMG(AMMO556, 20, 29, 2, 15, 4, 8, 23, 30, 200, 75, 7, S_COMMANDO,
        S_BURSTTYPE1),  // wt 26 // US; ROF
                        /* H&K MP53          */
    SMG(AMMO556, 22, 25, 2, 12, 3, 8, 15, 30, 200, 75, 7, S_MP53,
        S_BURSTTYPE1),  // wt 31 // Germany // eff range assumed; ROF 700 ?
                        /* AKSU-74           */
    SMG(AMMO545, 21, 26, 2, 17, 4, 8, 21, 30, 200, 75, 7, S_AKSU74,
        S_BURSTTYPE1),  // wt 39 // USSR; ROF 800
                        /* 5.7mm FN P90      */
    SMG(AMMO57, 21, 30, 2, 15, 5, 8, 42, 50, 225, 75, 7, S_P90,
        S_BURSTTYPE1),  // wt 28 // Belgium; ROF 800-1000
                        /* Type-85           */
    SMG(AMMO762W, 23, 23, 1, 10, 4, 11, 12, 30, 200, 75, 7, S_TYPE85,
        S_BURSTTYPE1),  // wt 19 // China; ROF 780

    /* SKS               */
    RIFLE(AMMO762W, 22, 31, 2, 13, 24, 10, 300, 80, 8, S_SKS),  // wt 39 // USSR
    /* Dragunov          */
    SN_RIFLE(AMMO762W, 21, 36, 5, 11, 32, 10, 750, 80, 8,
             S_DRAGUNOV),                                        // wt 43 // USSR
                                                                 /* M24               */
    SN_RIFLE(AMMO762N, 21, 36, 5, 8, 32, 5, 800, 80, 8, S_M24),  // wt 66 // US

    /* Steyr AUG         */
    ASRIFLE(AMMO556, 20, 30, 2, 13, 3, 8, 38, 30, 500, 77, 8, S_AUG,
            S_BURSTTYPE1),  // wt 36 // Austria; ROF 650
                            /* H&K G41           */
    ASRIFLE(AMMO556, 20, 29, 2, 13, 4, 8, 27, 30, 300, 77, 8, S_G41,
            S_BURSTTYPE1),  // wt 41 // Germany; ROF 850
                            /* Ruger Mini-14     */
    RIFLE(AMMO556, 20, 30, 2, 13, 20, 30, 250, 77, 8,
          S_RUGERMINI),  // wt 29 // US; ROF 750
                         /* C-7               */
    ASRIFLE(AMMO556, 20, 30, 2, 15, 5, 8, 41, 30, 400, 77, 8, S_C7,
            S_BURSTTYPE1),  // wt 36 // Canada; ROF 600-940
                            /* FA-MAS            */
    ASRIFLE(AMMO556, 20, 30, 2, 17, 5, 8, 32, 30, 250, 77, 8, S_FAMAS,
            S_BURSTTYPE1),  // wt 36 // France; ROF 900-1000
                            /* AK-74             */
    ASRIFLE(AMMO545, 20, 28, 2, 17, 3, 8, 30, 30, 350, 77, 8, S_AK74,
            S_BURSTTYPE1),  // wt 36 // USSR; ROF 650
                            /* AKM               */
    ASRIFLE(AMMO762W, 22, 29, 2, 17, 3, 11, 25, 30, 250, 77, 8, S_AKM,
            S_BURSTTYPE1),  // wt 43 // USSR; ROF 600
                            /* M-14              */
    ASRIFLE(AMMO762N, 20, 33, 2, 13, 4, 11, 33, 20, 330, 80, 8, S_M14,
            S_BURSTTYPE1),  // wt 29 // US; ROF 750
                            /* FN-FAL            */
    ASRIFLE(AMMO762N, 20, 32, 2, 17, 3, 11, 41, 20, 425, 80, 8, S_FNFAL,
            S_BURSTTYPE1),  // wt 43 // Belgium; ROF
                            /* H&K G3A3          */
    ASRIFLE(AMMO762N, 21, 31, 2, 13, 3, 11, 26, 20, 300, 80, 8, S_G3A3,
            S_BURSTTYPE1),  // wt 44 // Germany; ROF 500-600
                            /* H&K G11           */
    ASRIFLE(AMMO47, 20, 27, 2, 13, 3, 0, 40, 50, 300, 80, 8, S_G11,
            S_BURSTTYPE1),  // wt 38 // Germany; ROF 600

    /* Remington M870    */
    SHOTGUN(AMMO12G, 24, 32, 2, 7, 0, 0, 14, 7, 135, 80, 8, S_M870,
            S_BURSTTYPE1),  // wt 36 // US; damage for solid slug
                            /* SPAS-15           */
    SHOTGUN(AMMO12G, 24, 32, 2, 10, 0, 0, 18, 7, 135, 80, 8, S_SPAS,
            S_BURSTTYPE1),  // wt 38 // Italy; semi-auto; damage for solid slug
                            /* CAWS              */
    SHOTGUN(AMMOCAWS, 24, 40, 2, 10, 3, 11, 44, 10, 135, 80, 8, S_CAWS,
            S_BURSTTYPE1),  // wt 41 // US; fires 8 flechettes at once in very
                            // close fixed pattern

    /* FN Minimi         */
    LMG(AMMO556, 20, 28, 3, 13, 6, 5, 48, 30, 500, 82, 8, S_FNMINI,
        S_BURSTTYPE1),  // wt 68 // Belgium; ROF 750-1000
                        /* RPK-74            */
    LMG(AMMO545, 21, 30, 2, 13, 5, 5, 49, 30, 500, 82, 8, S_RPK74,
        S_BURSTTYPE1),  // wt 48 // USSR; ROF 800?
                        /* H&K 21E           */
    LMG(AMMO762N, 21, 32, 3, 13, 5, 7, 52, 20, 500, 82, 8, S_21E,
        S_BURSTTYPE1),  // wt 93 // Germany; ROF 800

    // NB blade distances will be = strength + dexterity /2

    /* Combat knife      */ BLADE(18, 12, 5, 40, 2, NO_WEAPON_SOUND),
    /* Throwing knife    */ THROWINGBLADE(15, 12, 4, 150, 2, S_THROWKNIFE),
    /* rock              */ NOWEAPON(0),
    /* grenade launcher  */ LAUNCHER(30, 3, 5, 80, 500, 20, 10, S_GLAUNCHER),
    /* mortar            */ LAUNCHER(30, 0, 5, 100, 550, 20, 10, S_MORTAR_SHOT),
    /* another rock      */ NOWEAPON(0),
    /* young male claws  */ BLADE(14, 10, 1, 10, 2, NO_WEAPON_SOUND),
    /* young fem claws   */ BLADE(18, 10, 1, 10, 2, NO_WEAPON_SOUND),
    /* old male claws    */ BLADE(20, 10, 1, 10, 2, NO_WEAPON_SOUND),
    /* old fem claws     */ BLADE(24, 10, 1, 10, 2, NO_WEAPON_SOUND),
    /* queen's tentacles */ BLADE(20, 10, 1, 70, 2, NO_WEAPON_SOUND),
    /* queen's spit      */ MONSTSPIT(20, 10, 1, 50, 300, 10, 5, ACR_SPIT),
    /* brass knuckles    */ PUNCHWEAPON(12, 15, 1, 0, NO_WEAPON_SOUND),
    /* underslung GL     */
    LAUNCHER(30, 3, 7, 80, 450, 20, 10, S_UNDER_GLAUNCHER),
    /* rocket launcher   */ LAW(30, 0, 5, 80, 500, 80, 10, S_ROCKET_LAUNCHER),
    /* bloodcat claws    */ BLADE(12, 14, 1, 10, 2, NO_WEAPON_SOUND),
    /* bloodcat bite     */ BLADE(24, 10, 1, 10, 2, NO_WEAPON_SOUND),
    /* machete           */ BLADE(24, 9, 6, 40, 2, NO_WEAPON_SOUND),
    /* rocket rifle      */
    RIFLE(AMMOROCKET, 20, 38, 2, 10, 62, 5, 600, 80, 10, S_SMALL_ROCKET_LAUNCHER),
    /* automag III       */
    PISTOL(AMMO762N, 24, 29, 1, 9, 13, 5, 220, 72, 6, S_AUTOMAG),
    /* infant spit       */ MONSTSPIT(12, 13, 1, 5, 200, 10, 5, ACR_SPIT),
    /* young male spit   */ MONSTSPIT(16, 10, 1, 10, 200, 10, 5, ACR_SPIT),
    /* old male spit     */ MONSTSPIT(20, 10, 1, 20, 200, 10, 5, ACR_SPIT),
    /* tank cannon       */ CANNON(30, 0, 8, 80, 800, 90, 10, S_TANK_CANNON),
    /* dart gun          */
    PISTOL(AMMODART, 25, 2, 1, 13, 10, 1, 200, 0, 0, NO_WEAPON_SOUND),
    /* Bloody Thrw knife */ THROWINGBLADE(15, 12, 3, 150, 2, S_THROWKNIFE),

    /* Flamethrower      */
    SHOTGUN(AMMOFLAME, 24, 60, 2, 10, 0, 0, 53, 5, 130, 40, 8, S_CAWS, S_BURSTTYPE1),
    /* crowbar           */ PUNCHWEAPON(25, 10, 4, 0, NO_WEAPON_SOUND),
    /* auto rocket rifle */
    ASRIFLE(AMMOROCKET, 20, 38, 2, 12, 5, 10, 97, 5, 600, 80, 10, S_SMALL_ROCKET_LAUNCHER,
            S_BURSTTYPE1),

    /* unused            */ NOWEAPON(0),
    /* unused            */ NOWEAPON(0),
    /* unused            */ NOWEAPON(0),
    /* unused            */ NOWEAPON(0),
};

MAGTYPE const Magazine[] = {
    // calibre,			 mag size,			ammo type
    {AMMO9, 15, AMMO_REGULAR},      {AMMO9, 30, AMMO_REGULAR},
    {AMMO9, 15, AMMO_AP},           {AMMO9, 30, AMMO_AP},
    {AMMO9, 15, AMMO_HP},           {AMMO9, 30, AMMO_HP},
    {AMMO38, 6, AMMO_REGULAR},      {AMMO38, 6, AMMO_AP},
    {AMMO38, 6, AMMO_HP},           {AMMO45, 7, AMMO_REGULAR},
    {AMMO45, 30, AMMO_REGULAR},     {AMMO45, 7, AMMO_AP},
    {AMMO45, 30, AMMO_AP},          {AMMO45, 7, AMMO_HP},
    {AMMO45, 30, AMMO_HP},          {AMMO357, 6, AMMO_REGULAR},
    {AMMO357, 9, AMMO_REGULAR},     {AMMO357, 6, AMMO_AP},
    {AMMO357, 9, AMMO_AP},          {AMMO357, 6, AMMO_HP},
    {AMMO357, 9, AMMO_HP},          {AMMO545, 30, AMMO_AP},
    {AMMO545, 30, AMMO_HP},         {AMMO556, 30, AMMO_AP},
    {AMMO556, 30, AMMO_HP},         {AMMO762W, 10, AMMO_AP},
    {AMMO762W, 30, AMMO_AP},        {AMMO762W, 10, AMMO_HP},
    {AMMO762W, 30, AMMO_HP},        {AMMO762N, 5, AMMO_AP},
    {AMMO762N, 20, AMMO_AP},        {AMMO762N, 5, AMMO_HP},
    {AMMO762N, 20, AMMO_HP},        {AMMO47, 50, AMMO_SUPER_AP},
    {AMMO57, 50, AMMO_AP},          {AMMO57, 50, AMMO_HP},
    {AMMO12G, 7, AMMO_BUCKSHOT},    {AMMO12G, 7, AMMO_REGULAR},
    {AMMOCAWS, 10, AMMO_BUCKSHOT},  {AMMOCAWS, 10, AMMO_SUPER_AP},
    {AMMOROCKET, 5, AMMO_SUPER_AP}, {AMMOROCKET, 5, AMMO_HE},
    {AMMOROCKET, 5, AMMO_HEAT},     {AMMODART, 1, AMMO_SLEEP_DART},
    {AMMOFLAME, 5, AMMO_BUCKSHOT},  {NOAMMO, 0, 0}};

ARMOURTYPE const Armour[] = {
    //	Class					      Protection	Degradation%
    // Description
    //  -------------       ----------  ------------      ----------------
    {ARMOURCLASS_VEST, 10, 25},     /* Flak jacket     */
    {ARMOURCLASS_VEST, 13, 20},     /* Flak jacket w X */
    {ARMOURCLASS_VEST, 16, 15},     /* Flak jacket w Y */
    {ARMOURCLASS_VEST, 15, 20},     /* Kevlar jacket   */
    {ARMOURCLASS_VEST, 19, 15},     /* Kevlar jack w X */
    {ARMOURCLASS_VEST, 24, 10},     /* Kevlar jack w Y */
    {ARMOURCLASS_VEST, 30, 15},     /* Spectra jacket  */
    {ARMOURCLASS_VEST, 36, 10},     /* Spectra jack w X*/
    {ARMOURCLASS_VEST, 42, 5},      /* Spectra jack w Y*/
    {ARMOURCLASS_LEGGINGS, 15, 20}, /* Kevlar leggings */
    {ARMOURCLASS_LEGGINGS, 19, 15}, /* Kevlar legs w X */

    {ARMOURCLASS_LEGGINGS, 24, 10}, /* Kevlar legs w Y */
    {ARMOURCLASS_LEGGINGS, 30, 15}, /* Spectra leggings*/
    {ARMOURCLASS_LEGGINGS, 36, 10}, /* Spectra legs w X*/
    {ARMOURCLASS_LEGGINGS, 42, 5},  /* Spectra legs w Y*/
    {ARMOURCLASS_HELMET, 10, 5},    /* Steel helmet    */
    {ARMOURCLASS_HELMET, 15, 20},   /* Kevlar helmet   */
    {ARMOURCLASS_HELMET, 19, 15},   /* Kevlar helm w X */
    {ARMOURCLASS_HELMET, 24, 10},   /* Kevlar helm w Y */
    {ARMOURCLASS_HELMET, 30, 15},   /* Spectra helmet  */
    {ARMOURCLASS_HELMET, 36, 10},   /* Spectra helm w X*/

    {ARMOURCLASS_HELMET, 42, 5},  /* Spectra helm w Y*/
    {ARMOURCLASS_PLATE, 15, 200}, /* Ceramic plates  */
    {ARMOURCLASS_MONST, 3, 0},    /* Infant creature hide */
    {ARMOURCLASS_MONST, 5, 0},    /* Young male creature hide  */
    {ARMOURCLASS_MONST, 6, 0},    /* Male creature hide  */
    {ARMOURCLASS_MONST, 20, 0},   /* Queen creature hide  */
    {ARMOURCLASS_VEST, 2, 25},    /* Leather jacket    */
    {ARMOURCLASS_VEST, 12, 30},   /* Leather jacket w kevlar */
    {ARMOURCLASS_VEST, 16, 25},   /* Leather jacket w kevlar & compound 18 */
    {ARMOURCLASS_VEST, 19, 20},   /* Leather jacket w kevlar & queen blood */

    {ARMOURCLASS_MONST, 7, 0},  /* Young female creature hide */
    {ARMOURCLASS_MONST, 8, 0},  /* Old female creature hide  */
    {ARMOURCLASS_VEST, 1, 25},  /* T-shirt */
    {ARMOURCLASS_VEST, 22, 20}, /* Kevlar 2 jacket   */
    {ARMOURCLASS_VEST, 27, 15}, /* Kevlar 2 jack w X */
    {ARMOURCLASS_VEST, 32, 10}, /* Kevlar 2 jack w Y */
};

EXPLOSIVETYPE const Explosive[] = {
    //	Type							Yield		Yield2
    // Radius
    // Volume
    // Volatility
    // Animation			Description
    //										-----
    //-------
    //------
    //------
    //----------
    //--------- 		------------------
    {EXPLOSV_STUN, 1, 70, 4, 0, 0, STUN_BLAST /* stun grenade       */},
    {EXPLOSV_TEARGAS, 0, 20, 4, 0, 0, TARGAS_EXP /* tear gas grenade   */},
    {EXPLOSV_MUSTGAS, 15, 40, 4, 0, 0, MUSTARD_EXP /* mustard gas grenade*/},
    {EXPLOSV_NORMAL, 15, 7, 3, 15, 1, BLAST_1 /* mini hand grenade  */},
    {EXPLOSV_NORMAL, 25, 10, 4, 25, 1, BLAST_1 /* reg hand grenade   */},
    {EXPLOSV_NORMAL, 40, 12, 5, 20, 10, BLAST_2 /* RDX                */},
    {EXPLOSV_NORMAL, 50, 15, 5, 50, 2, BLAST_2 /* TNT (="explosives")*/},
    {EXPLOSV_NORMAL, 60, 15, 6, 60, 2, BLAST_2 /* HMX (=RDX+TNT)     */},
    {EXPLOSV_NORMAL, 55, 15, 6, 55, 0, BLAST_2 /* C1  (=RDX+min oil) */},
    {EXPLOSV_NORMAL, 50, 22, 6, 50, 2, BLAST_2 /* mortar shell       */},

    {EXPLOSV_NORMAL, 30, 30, 2, 30, 2, BLAST_1 /* mine               */},
    {EXPLOSV_NORMAL, 65, 30, 7, 65, 0, BLAST_1 /* C4  ("plastique")  */},
    {EXPLOSV_FLARE, 0, 0, 10, 0, 0, BLAST_1 /* trip flare				  */},
    {EXPLOSV_NOISE, 0, 0, 50, 50, 0, BLAST_1 /* trip klaxon        */},
    {EXPLOSV_NORMAL, 20, 0, 1, 20, 0, BLAST_1 /* shaped charge      */},
    {EXPLOSV_FLARE, 0, 0, 10, 0, 0, BLAST_1, /* break light        */},
    {EXPLOSV_NORMAL, 25, 5, 4, 25, 1, BLAST_1,
     /* GL grenade					*/},
    {EXPLOSV_TEARGAS, 0, 20, 3, 0, 0, TARGAS_EXP, /* GL tear gas grenade*/},
    {EXPLOSV_STUN, 1, 50, 4, 0, 0, STUN_BLAST,
     /* GL stun grenade		*/},
    {EXPLOSV_SMOKE, 0, 0, 3, 0, 0, SMOKE_EXP,
     /* GL smoke grenade		*/},

    {EXPLOSV_SMOKE, 0, 0, 4, 0, 0, SMOKE_EXP,
     /* smoke grenade			*/},
    {EXPLOSV_NORMAL, 60, 20, 6, 60, 2, BLAST_2, /* Tank Shell         */},
    {EXPLOSV_NORMAL, 100, 0, 0, 0, 0, BLAST_1, /* Fake structure igniter*/},
    {EXPLOSV_NORMAL, 100, 0, 1, 0, 0, BLAST_1, /* creature cocktail */},
    {EXPLOSV_NORMAL, 50, 10, 5, 50, 2, BLAST_2, /* fake struct explosion*/},
    {EXPLOSV_NORMAL, 50, 10, 5, 50, 2, BLAST_3, /* fake vehicle explosion*/},
    {EXPLOSV_TEARGAS, 0, 40, 4, 0, 0, TARGAS_EXP /* big tear gas */},
    {EXPLOSV_CREATUREGAS, 5, 0, 1, 0, 0, NO_BLAST /* small creature gas*/},
    {EXPLOSV_CREATUREGAS, 8, 0, 3, 0, 0, NO_BLAST /* big creature gas*/},
    {EXPLOSV_CREATUREGAS, 0, 0, 0, 0, 0, NO_BLAST /* vry sm creature gas*/},
};

static const char *const gzBurstSndStrings[] = {
    "",                    // NOAMMO
    "",                    // 38
    "9mm burst ",          // 9mm
    "45 caliber burst ",   // 45
    "",                    // 357
    "",                    // 12G
    "shotgun burst ",      // CAWS
    "5,45 burst ",         // 5.45
    "5,56 burst ",         // 5.56
    "7,62 nato burst ",    // 7,62 N
    "7,62 wp burst ",      // 7,62 W
    "4,7 caliber burst ",  // 4.7
    "5,7 burst ",          // 5,7
    "",                    // Monster
    "rl automatic ",       // Rocket
    "",                    // Dart
    "",                    // Flame (unused)
};

// the amount of momentum reduction for the head, torso, and legs
// used to determine whether the bullet will go through someone
static const uint8_t BodyImpactReduction[4] = {0, 15, 30, 23};

uint16_t GunRange(OBJECTTYPE const &o) {
  // return a minimal value of 1 tile
  if (!(Item[o.usItem].usItemClass & IC_WEAPON)) return CELL_X_SIZE;

  int8_t const attach_pos = FindAttachment(&o, GUN_BARREL_EXTENDER);
  uint16_t range = Weapon[o.usItem].usRange;
  if (attach_pos != ITEM_NOT_FOUND) {
    range += GUN_BARREL_RANGE_BONUS * WEAPON_STATUS_MOD(o.bAttachStatus[attach_pos]) / 100;
  }
  return range;
}

int8_t EffectiveArmour(OBJECTTYPE const *const o) {
  if (!o) return 0;

  INVTYPE const &item = Item[o->usItem];
  if (item.usItemClass != IC_ARMOUR) return 0;

  int32_t armour_val = Armour[item.ubClassIndex].ubProtection * o->bStatus[0] / 100;
  int8_t const plate_pos = FindAttachment(o, CERAMIC_PLATES);
  if (plate_pos != ITEM_NOT_FOUND) {
    armour_val +=
        Armour[Item[CERAMIC_PLATES].ubClassIndex].ubProtection * o->bAttachStatus[plate_pos] / 100;
  }
  return armour_val;
}

int8_t ArmourPercent(const SOLDIERTYPE *pSoldier) {
  int32_t iVest, iHelmet, iLeg;

  if (pSoldier->inv[VESTPOS].usItem) {
    iVest = EffectiveArmour(&(pSoldier->inv[VESTPOS]));
    // convert to % of best; ignoring bug-treated stuff
    iVest = 65 * iVest /
            (Armour[Item[SPECTRA_VEST_18].ubClassIndex].ubProtection +
             Armour[Item[CERAMIC_PLATES].ubClassIndex].ubProtection);
  } else {
    iVest = 0;
  }

  if (pSoldier->inv[HELMETPOS].usItem) {
    iHelmet = EffectiveArmour(&(pSoldier->inv[HELMETPOS]));
    // convert to % of best; ignoring bug-treated stuff
    iHelmet = 15 * iHelmet / Armour[Item[SPECTRA_HELMET_18].ubClassIndex].ubProtection;
  } else {
    iHelmet = 0;
  }

  if (pSoldier->inv[LEGPOS].usItem) {
    iLeg = EffectiveArmour(&(pSoldier->inv[LEGPOS]));
    // convert to % of best; ignoring bug-treated stuff
    iLeg = 25 * iLeg / Armour[Item[SPECTRA_LEGGINGS_18].ubClassIndex].ubProtection;
  } else {
    iLeg = 0;
  }
  return ((int8_t)(iHelmet + iVest + iLeg));
}

static int8_t ExplosiveEffectiveArmour(OBJECTTYPE *pObj) {
  int32_t iValue;
  int8_t bPlate;

  if (pObj == NULL || Item[pObj->usItem].usItemClass != IC_ARMOUR) {
    return (0);
  }
  iValue = Armour[Item[pObj->usItem].ubClassIndex].ubProtection;
  iValue = iValue * pObj->bStatus[0] / 100;
  if (pObj->usItem == FLAK_JACKET || pObj->usItem == FLAK_JACKET_18 ||
      pObj->usItem == FLAK_JACKET_Y) {
    // increase value for flak jackets!
    iValue *= 3;
  }

  bPlate = FindAttachment(pObj, CERAMIC_PLATES);
  if (bPlate != ITEM_NOT_FOUND) {
    int32_t iValue2;

    iValue2 = Armour[Item[CERAMIC_PLATES].ubClassIndex].ubProtection;
    iValue2 = iValue2 * pObj->bAttachStatus[bPlate] / 100;

    iValue += iValue2;
  }
  return ((int8_t)iValue);
}

int8_t ArmourVersusExplosivesPercent(SOLDIERTYPE *pSoldier) {
  // returns the % damage reduction from grenades
  int32_t iVest, iHelmet, iLeg;

  if (pSoldier->inv[VESTPOS].usItem) {
    iVest = ExplosiveEffectiveArmour(&(pSoldier->inv[VESTPOS]));
    // convert to % of best; ignoring bug-treated stuff
    iVest = std::min(65, 65 * iVest /
                             (Armour[Item[SPECTRA_VEST_18].ubClassIndex].ubProtection +
                              Armour[Item[CERAMIC_PLATES].ubClassIndex].ubProtection));
  } else {
    iVest = 0;
  }

  if (pSoldier->inv[HELMETPOS].usItem) {
    iHelmet = ExplosiveEffectiveArmour(&(pSoldier->inv[HELMETPOS]));
    // convert to % of best; ignoring bug-treated stuff
    iHelmet =
        std::min(15, 15 * iHelmet / Armour[Item[SPECTRA_HELMET_18].ubClassIndex].ubProtection);
  } else {
    iHelmet = 0;
  }

  if (pSoldier->inv[LEGPOS].usItem) {
    iLeg = ExplosiveEffectiveArmour(&(pSoldier->inv[LEGPOS]));
    // convert to % of best; ignoring bug-treated stuff
    iLeg = std::min(25, 25 * iLeg / Armour[Item[SPECTRA_LEGGINGS_18].ubClassIndex].ubProtection);
  } else {
    iLeg = 0;
  }
  return ((int8_t)(iHelmet + iVest + iLeg));
}

static void AdjustImpactByHitLocation(int32_t iImpact, uint8_t ubHitLocation, int32_t *piNewImpact,
                                      int32_t *piImpactForCrits) {
  switch (ubHitLocation) {
    case AIM_SHOT_HEAD:
      // 1.5x damage from successful hits to the head!
      *piImpactForCrits = HEAD_DAMAGE_ADJUSTMENT(iImpact);
      *piNewImpact = *piImpactForCrits;
      break;
    case AIM_SHOT_LEGS:
      // half damage for determining critical hits
      // quarter actual damage
      *piImpactForCrits = LEGS_DAMAGE_ADJUSTMENT(iImpact);
      *piNewImpact = LEGS_DAMAGE_ADJUSTMENT(*piImpactForCrits);
      break;
    default:
      *piImpactForCrits = iImpact;
      *piNewImpact = iImpact;
      break;
  }
}

// #define	TESTGUNJAM

BOOLEAN CheckForGunJam(SOLDIERTYPE *pSoldier) {
  OBJECTTYPE *pObj;
  int32_t iChance, iResult;

  // should jams apply to enemies?
  if (pSoldier->uiStatusFlags & SOLDIER_PC) {
    if (Item[pSoldier->usAttackingWeapon].usItemClass == IC_GUN &&
        !EXPLOSIVE_GUN(pSoldier->usAttackingWeapon)) {
      pObj = &(pSoldier->inv[pSoldier->ubAttackingHand]);
      if (pObj->bGunAmmoStatus > 0) {
        // gun might jam, figure out the chance
        iChance = (80 - pObj->bGunStatus);

        // CJC: removed reliability from formula...

        // jams can happen to unreliable guns "earlier" than normal or reliable
        // ones.
        // iChance = iChance - Item[pObj->usItem].bReliability * 2;

        // decrease the chance of a jam by 20% per point of reliability;
        // increased by 20% per negative point...
        // iChance = iChance * (10 - Item[pObj->usItem].bReliability * 2) / 10;

        if (pSoldier->bDoBurst > 1) {
          // if at bullet in a burst after the first, higher chance
          iChance -= PreRandom(80);
        } else {
          iChance -= PreRandom(100);
        }

#ifdef TESTGUNJAM
        if (1)
#else
        if ((int32_t)PreRandom(100) < iChance || gfNextFireJam)
#endif
        {
          gfNextFireJam = FALSE;

          // jam! negate the gun ammo status.
          pObj->bGunAmmoStatus *= -1;

          // Deduct AMMO!
          DeductAmmo(pSoldier, pSoldier->ubAttackingHand);

          TacticalCharacterDialogue(pSoldier, QUOTE_JAMMED_GUN);
          return (TRUE);
        }
      } else if (pObj->bGunAmmoStatus < 0) {
        // try to unjam gun
        iResult =
            SkillCheck(pSoldier, UNJAM_GUN_CHECK, (int8_t)(Item[pObj->usItem].bReliability * 4));
        if (iResult > 0) {
          // yay! unjammed the gun
          pObj->bGunAmmoStatus *= -1;

          // MECHANICAL/DEXTERITY GAIN: Unjammed a gun
          StatChange(*pSoldier, MECHANAMT, 5, FROM_SUCCESS);
          StatChange(*pSoldier, DEXTAMT, 5, FROM_SUCCESS);

          DirtyMercPanelInterface(pSoldier, DIRTYLEVEL2);

          // We unjammed gun, return appropriate value!
          return (255);
        } else {
          return (TRUE);
        }
      }
    }
  }
  return (FALSE);
}

BOOLEAN OKFireWeapon(SOLDIERTYPE *pSoldier) {
  BOOLEAN bGunJamVal;

  // 1) Are we attacking with our second hand?
  if (pSoldier->ubAttackingHand == SECONDHANDPOS) {
    if (!EnoughAmmo(pSoldier, FALSE, pSoldier->ubAttackingHand)) {
      if (pSoldier->bTeam == OUR_TEAM) {
        ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, g_langRes->Message[STR_2ND_CLIP_DEPLETED]);
        return (FALSE);
      }
    }
  }

  bGunJamVal = CheckForGunJam(pSoldier);

  if (bGunJamVal == 255) {
    return (255);
  }

  if (bGunJamVal) {
    return (FALSE);
  }

  return (TRUE);
}

static BOOLEAN UseGun(SOLDIERTYPE *pSoldier, int16_t sTargetGridNo);
static void UseBlade(SOLDIERTYPE *pSoldier, int16_t sTargetGridNo);
static void UseThrown(SOLDIERTYPE *pSoldier, int16_t sTargetGridNo);
static BOOLEAN UseLauncher(SOLDIERTYPE *pSoldier, int16_t sTargetGridNo);

BOOLEAN FireWeapon(SOLDIERTYPE *pSoldier, int16_t sTargetGridNo) {
  // ignore passed in target gridno for now

  // if target gridno is the same as ours, do not fire!
  if (sTargetGridNo == pSoldier->sGridNo) {
    // FREE UP NPC!
    DebugMsg(TOPIC_JA2, DBG_LEVEL_3, "@@@@@@@ Freeing up attacker - attack on own gridno!");
    FreeUpAttacker(pSoldier);
    return (FALSE);
  }

  // SET ATTACKER TO NOBODY, WILL GET SET EVENTUALLY
  pSoldier->opponent = NULL;

  switch (Item[pSoldier->usAttackingWeapon].usItemClass) {
    case IC_THROWING_KNIFE:
    case IC_GUN:

      // ATE: PAtch up - bookkeeping for spreading done out of whak
      if (pSoldier->fDoSpread && !pSoldier->bDoBurst) {
        pSoldier->fDoSpread = FALSE;
      }

      if (pSoldier->fDoSpread >= 6) {
        pSoldier->fDoSpread = FALSE;
      }

      if (pSoldier->fDoSpread) {
        if (pSoldier->sSpreadLocations[pSoldier->fDoSpread - 1] != 0) {
          UseGun(pSoldier, pSoldier->sSpreadLocations[pSoldier->fDoSpread - 1]);
        } else {
          UseGun(pSoldier, sTargetGridNo);
        }
        pSoldier->fDoSpread++;
      } else {
        UseGun(pSoldier, sTargetGridNo);
      }
      break;
    case IC_BLADE:

      UseBlade(pSoldier, sTargetGridNo);
      break;
    case IC_PUNCH:
      UseHandToHand(pSoldier, sTargetGridNo, FALSE);
      break;

    case IC_LAUNCHER:
      UseLauncher(pSoldier, sTargetGridNo);
      break;

    default:
      // attempt to throw
      UseThrown(pSoldier, sTargetGridNo);
      break;
  }
  return (TRUE);
}

void GetTargetWorldPositions(SOLDIERTYPE *pSoldier, int16_t sTargetGridNo, float *pdXPos,
                             float *pdYPos, float *pdZPos) {
  float dTargetX;
  float dTargetY;
  float dTargetZ;
  int16_t sXMapPos, sYMapPos;
  uint32_t uiRoll;

  SOLDIERTYPE *const pTargetSoldier = WhoIsThere2(sTargetGridNo, pSoldier->bTargetLevel);
  if (pTargetSoldier) {
    pSoldier->opponent = pTargetSoldier;
    dTargetX = (float)CenterX(pTargetSoldier->sGridNo);
    dTargetY = (float)CenterY(pTargetSoldier->sGridNo);
    if (pSoldier->bAimShotLocation == AIM_SHOT_RANDOM) {
      uiRoll = PreRandom(100);
      if (uiRoll < 15) {
        pSoldier->bAimShotLocation = AIM_SHOT_LEGS;
      } else if (uiRoll > 94) {
        pSoldier->bAimShotLocation = AIM_SHOT_HEAD;
      } else {
        pSoldier->bAimShotLocation = AIM_SHOT_TORSO;
      }
      if (pSoldier->bAimShotLocation != AIM_SHOT_HEAD) {
        uint32_t uiChanceToGetThrough = SoldierToSoldierBodyPartChanceToGetThrough(
            pSoldier, pTargetSoldier, pSoldier->bAimShotLocation);

        if (uiChanceToGetThrough < 25) {
          if (SoldierToSoldierBodyPartChanceToGetThrough(pSoldier, pTargetSoldier, AIM_SHOT_HEAD) >
              uiChanceToGetThrough * 2) {
            // try for a head shot then
            pSoldier->bAimShotLocation = AIM_SHOT_HEAD;
          }
        }
      }
    }

    switch (pSoldier->bAimShotLocation) {
      case AIM_SHOT_HEAD:
        CalculateSoldierZPos(pTargetSoldier, HEAD_TARGET_POS, &dTargetZ);
        break;
      case AIM_SHOT_TORSO:
        CalculateSoldierZPos(pTargetSoldier, TORSO_TARGET_POS, &dTargetZ);
        break;
      case AIM_SHOT_LEGS:
        CalculateSoldierZPos(pTargetSoldier, LEGS_TARGET_POS, &dTargetZ);
        break;
      default:
        // %)@#&(%?
        CalculateSoldierZPos(pTargetSoldier, TARGET_POS, &dTargetZ);
        break;
    }
  } else {
    // GET TARGET XY VALUES
    ConvertGridNoToCenterCellXY(sTargetGridNo, &sXMapPos, &sYMapPos);

    // fire at centre of tile
    dTargetX = (float)sXMapPos;
    dTargetY = (float)sYMapPos;
    if (pSoldier->bTargetCubeLevel) {
      // fire at the centre of the cube specified
      dTargetZ =
          ((float)(pSoldier->bTargetCubeLevel + pSoldier->bTargetLevel * PROFILE_Z_SIZE) - 0.5f) *
          HEIGHT_UNITS_PER_INDEX;
    } else {
      int8_t bStructHeight = GetStructureTargetHeight(sTargetGridNo, pSoldier->bTargetLevel == 1);
      if (bStructHeight > 0) {
        // fire at the centre of the cube *one below* the tallest of the tallest
        // structure
        if (bStructHeight > 1) {
          // reduce target level by 1
          bStructHeight--;
        }
        dTargetZ = ((float)(bStructHeight + pSoldier->bTargetLevel * PROFILE_Z_SIZE) - 0.5f) *
                   HEIGHT_UNITS_PER_INDEX;
      } else {
        // fire at 1 unit above the level of the ground
        dTargetZ = (float)(pSoldier->bTargetLevel * PROFILE_Z_SIZE) * HEIGHT_UNITS_PER_INDEX + 1;
      }
    }
    // adjust for terrain height
    dTargetZ += CONVERT_PIXELS_TO_HEIGHTUNITS(gpWorldLevelData[sTargetGridNo].sHeight);
  }

  *pdXPos = dTargetX;
  *pdYPos = dTargetY;
  *pdZPos = dTargetZ;
}

static uint16_t ModifyExpGainByTarget(const uint16_t exp_gain, const SOLDIERTYPE *const tgt) {
  if (tgt->ubBodyType == COW || tgt->ubBodyType == CROW) {
    return exp_gain / 2;
  } else if (IsMechanical(*tgt) || TANK(tgt)) {
    // no exp from shooting a vehicle that you can't damage and can't move!
    return 0;
  }
  return exp_gain;
}

static BOOLEAN WillExplosiveWeaponFail(const SOLDIERTYPE *pSoldier, const OBJECTTYPE *pObj);

static BOOLEAN UseGun(SOLDIERTYPE *pSoldier, int16_t sTargetGridNo) {
  uint32_t uiHitChance, uiDiceRoll;
  int16_t sAPCost;
  float dTargetX;
  float dTargetY;
  float dTargetZ;
  uint16_t usItemNum;
  BOOLEAN fBuckshot;
  uint8_t ubVolume;
  int8_t bSilencerPos;
  char zBurstString[50];
  uint8_t ubDirection;
  int16_t sNewGridNo;
  BOOLEAN fGonnaHit = FALSE;
  uint16_t usExpGain = 0;
  uint32_t uiDepreciateTest;

  // Deduct points!
  sAPCost = CalcTotalAPsToAttack(pSoldier, sTargetGridNo, FALSE, pSoldier->bAimTime);

  usItemNum = pSoldier->usAttackingWeapon;

  if (pSoldier->bDoBurst) {
    // ONly deduct points once
    if (pSoldier->bDoBurst == 1) {
      if (Weapon[usItemNum].sBurstSound != NO_WEAPON_SOUND) {
        // IF we are silenced?
        if (FindAttachment(&(pSoldier->inv[pSoldier->ubAttackingHand]), SILENCER) != NO_SLOT) {
          // Pick sound file baed on how many bullets we are going to fire...
          sprintf(zBurstString, SOUNDSDIR "/weapons/silencer burst %d.wav", pSoldier->bBulletsLeft);

          // Try playing sound...
          pSoldier->iBurstSoundID =
              PlayLocationJA2SampleFromFile(pSoldier->sGridNo, zBurstString, HIGHVOLUME, 1);
        } else {
          // Pick sound file baed on how many bullets we are going to fire...
          sprintf(zBurstString, SOUNDSDIR "/weapons/%s%d.wav",
                  gzBurstSndStrings[Weapon[usItemNum].ubCalibre], pSoldier->bBulletsLeft);

          // Try playing sound...
          pSoldier->iBurstSoundID =
              PlayLocationJA2SampleFromFile(pSoldier->sGridNo, zBurstString, HIGHVOLUME, 1);
        }

        if (pSoldier->iBurstSoundID == NO_SAMPLE) {
          // If failed, play normal default....
          pSoldier->iBurstSoundID = PlayLocationJA2Sample(
              pSoldier->sGridNo, Weapon[usItemNum].sBurstSound, HIGHVOLUME, 1);
        }
      }

      DeductPoints(pSoldier, sAPCost, 0);
    }

  } else {
    // ONLY DEDUCT FOR THE FIRST HAND when doing two-pistol attacks
    if (IsValidSecondHandShot(pSoldier) && pSoldier->inv[HANDPOS].bGunStatus >= USABLE &&
        pSoldier->inv[HANDPOS].bGunAmmoStatus > 0) {
      // only deduct APs when the main gun fires
      if (pSoldier->ubAttackingHand == HANDPOS) {
        DeductPoints(pSoldier, sAPCost, 0);
      }
    } else {
      DeductPoints(pSoldier, sAPCost, 0);
    }

    // PLAY SOUND
    // ( For throwing knife.. it's earlier in the animation
    if (Weapon[usItemNum].sSound != NO_WEAPON_SOUND &&
        Item[usItemNum].usItemClass != IC_THROWING_KNIFE) {
      // Switch on silencer...
      if (FindAttachment(&(pSoldier->inv[pSoldier->ubAttackingHand]), SILENCER) != NO_SLOT) {
        SoundID uiSound;
        if (Weapon[usItemNum].ubCalibre == AMMO9 || Weapon[usItemNum].ubCalibre == AMMO38 ||
            Weapon[usItemNum].ubCalibre == AMMO57) {
          uiSound = S_SILENCER_1;
        } else {
          uiSound = S_SILENCER_2;
        }

        PlayLocationJA2Sample(pSoldier->sGridNo, uiSound, HIGHVOLUME, 1);
      } else {
        PlayLocationJA2Sample(pSoldier->sGridNo, Weapon[usItemNum].sSound, HIGHVOLUME, 1);
      }
    }
  }

  // CALC CHANCE TO HIT
  if (Item[usItemNum].usItemClass == IC_THROWING_KNIFE) {
    uiHitChance = CalcThrownChanceToHit(pSoldier, sTargetGridNo, pSoldier->bAimTime,
                                        pSoldier->bAimShotLocation);
  } else {
    uiHitChance =
        CalcChanceToHitGun(pSoldier, sTargetGridNo, pSoldier->bAimTime, pSoldier->bAimShotLocation);
  }

  // ATE: Added if we are in meanwhile, we always hit...
  if (AreInMeanwhile()) {
    uiHitChance = 100;
  }

  // ROLL DICE
  uiDiceRoll = PreRandom(100);

  fGonnaHit = uiDiceRoll <= uiHitChance;

  // ATE; Moved a whole blotch if logic code for finding target positions to a
  // function so other places can use it
  GetTargetWorldPositions(pSoldier, sTargetGridNo, &dTargetX, &dTargetY, &dTargetZ);

  // Some things we don't do for knives...
  if (Item[usItemNum].usItemClass != IC_THROWING_KNIFE) {
    // Deduct AMMO!
    DeductAmmo(pSoldier, pSoldier->ubAttackingHand);

    // ATE: Check if we should say quote...
    if (pSoldier->inv[pSoldier->ubAttackingHand].ubGunShotsLeft == 0 &&
        pSoldier->usAttackingWeapon != ROCKET_LAUNCHER) {
      if (pSoldier->bTeam == OUR_TEAM) {
        pSoldier->fSayAmmoQuotePending = TRUE;
      }
    }

    // NB bDoBurst will be 2 at this point for the first shot since it was
    // incremented above
    if (IsOnOurTeam(*pSoldier) && pSoldier->target != NULL &&
        (!pSoldier->bDoBurst || pSoldier->bDoBurst == 2) && gTacticalStatus.uiFlags & INCOMBAT) {
      const SOLDIERTYPE *const tgt = pSoldier->target;
      if (SoldierToSoldierBodyPartChanceToGetThrough(pSoldier, tgt, pSoldier->bAimShotLocation) >
          0) {
        if (fGonnaHit) {
          // grant extra exp for hitting a difficult target
          usExpGain += (uint8_t)(100 - uiHitChance) / 25;

          if (pSoldier->bAimTime && !pSoldier->bDoBurst) {
            // gain extra exp for aiming, up to the amount from
            // the difficulty of the shot
            usExpGain += std::min((uint16_t)pSoldier->bAimTime, usExpGain);
          }

          // base pts extra for hitting
          usExpGain += 3;
        }

        // add base pts for taking a shot, whether it hits or misses
        usExpGain += 3;

        if (IsValidSecondHandShot(pSoldier) && pSoldier->inv[HANDPOS].bGunStatus >= USABLE &&
            pSoldier->inv[HANDPOS].bGunAmmoStatus > 0) {
          // reduce exp gain for two pistol shooting since both shots give xp
          usExpGain = (usExpGain * 2) / 3;
        }

        usExpGain = ModifyExpGainByTarget(usExpGain, tgt);

        // MARKSMANSHIP GAIN: gun attack
        StatChange(*pSoldier, MARKAMT, usExpGain, fGonnaHit ? FROM_SUCCESS : FROM_FAILURE);
      }
    }

    // set buckshot and muzzle flash
    fBuckshot = FALSE;
    if (!CREATURE_OR_BLOODCAT(pSoldier)) {
      pSoldier->fMuzzleFlash = TRUE;
      switch (pSoldier->inv[pSoldier->ubAttackingHand].ubGunAmmoType) {
        case AMMO_BUCKSHOT:
          fBuckshot = TRUE;
          break;
        case AMMO_SLEEP_DART:
          pSoldier->fMuzzleFlash = FALSE;
          break;
        default:
          break;
      }
    }
  } else  //  throwing knife
  {
    fBuckshot = FALSE;
    pSoldier->fMuzzleFlash = FALSE;

    // Deduct knife from inv! (not here, later?)

    // Improve for using a throwing knife....
    if (IsOnOurTeam(*pSoldier) && pSoldier->target != NULL) {
      if (fGonnaHit) {
        // grant extra exp for hitting a difficult target
        usExpGain += (uint8_t)(100 - uiHitChance) / 10;

        if (pSoldier->bAimTime) {
          // gain extra exp for aiming, up to the amount from
          // the difficulty of the throw
          usExpGain += (2 * std::min((uint16_t)pSoldier->bAimTime, usExpGain));
        }

        // base pts extra for hitting
        usExpGain += 10;
      }

      // add base pts for taking a shot, whether it hits or misses
      usExpGain += 10;

      usExpGain = ModifyExpGainByTarget(usExpGain, pSoldier->target);

      // MARKSMANSHIP/DEXTERITY GAIN: throwing knife attack
      StatChangeCause const cause = fGonnaHit ? FROM_SUCCESS : FROM_FAILURE;
      StatChange(*pSoldier, MARKAMT, usExpGain / 2, cause);
      StatChange(*pSoldier, DEXTAMT, usExpGain / 2, cause);
    }
  }

  if (usItemNum == ROCKET_LAUNCHER) {
    if (WillExplosiveWeaponFail(pSoldier, &(pSoldier->inv[HANDPOS]))) {
      CreateItem(DISCARDED_LAW, pSoldier->inv[HANDPOS].bStatus[0], &(pSoldier->inv[HANDPOS]));
      DirtyMercPanelInterface(pSoldier, DIRTYLEVEL2);

      IgniteExplosion(pSoldier, 0, pSoldier->sGridNo, C1, pSoldier->bLevel);

      // Reduce again for attack end 'cause it has been incremented for a normal
      // attack
      //
      DebugMsg(
          TOPIC_JA2, DBG_LEVEL_3,
          String("@@@@@@@ Freeing up attacker - ATTACK ANIMATION %hs "
                 "ENDED BY BAD EXPLOSIVE CHECK, Now %d",
                 gAnimControl[pSoldier->usAnimState].zAnimStr, gTacticalStatus.ubAttackBusyCount));
      ReduceAttackBusyCount(pSoldier, FALSE);

      return (FALSE);
    }
  }

  FireBulletGivenTarget(pSoldier, dTargetX, dTargetY, dTargetZ, pSoldier->usAttackingWeapon,
                        (uint16_t)(uiHitChance - uiDiceRoll), fBuckshot, FALSE);

  ubVolume = Weapon[pSoldier->usAttackingWeapon].ubAttackVolume;

  if (Item[usItemNum].usItemClass == IC_THROWING_KNIFE) {
    // Here, remove the knife...	or (for now) rocket launcher
    RemoveObjs(&(pSoldier->inv[HANDPOS]), 1);
    DirtyMercPanelInterface(pSoldier, DIRTYLEVEL2);
  } else if (usItemNum == ROCKET_LAUNCHER) {
    CreateItem(DISCARDED_LAW, pSoldier->inv[HANDPOS].bStatus[0], &(pSoldier->inv[HANDPOS]));
    DirtyMercPanelInterface(pSoldier, DIRTYLEVEL2);

    // Direction to center of explosion
    ubDirection = OppositeDirection(pSoldier->bDirection);
    sNewGridNo = NewGridNo((uint16_t)pSoldier->sGridNo, (uint16_t)(1 * DirectionInc(ubDirection)));

    // Check if a person exists here and is not prone....
    SOLDIERTYPE *const tgt = WhoIsThere2(sNewGridNo, pSoldier->bLevel);
    if (tgt != NULL) {
      if (gAnimControl[tgt->usAnimState].ubHeight != ANIM_PRONE) {
        // Increment attack counter...
        gTacticalStatus.ubAttackBusyCount++;
        DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
                 String("Incrementing Attack: Exaust from LAW", gTacticalStatus.ubAttackBusyCount));

        EVENT_SoldierGotHit(tgt, MINI_GRENADE, 10, 200, pSoldier->bDirection, 0, pSoldier, 0,
                            ANIM_CROUCH, sNewGridNo);
      }
    }
  } else {
    // if the weapon has a silencer attached
    bSilencerPos = FindAttachment(&(pSoldier->inv[HANDPOS]), SILENCER);
    if (bSilencerPos != -1) {
      // reduce volume by a percentage equal to silencer's work %age (min 1)
      ubVolume =
          1 + ((100 - WEAPON_STATUS_MOD(pSoldier->inv[HANDPOS].bAttachStatus[bSilencerPos])) /
               (100 / (ubVolume - 1)));
    }
  }

  MakeNoise(pSoldier, pSoldier->sGridNo, pSoldier->bLevel, ubVolume, NOISE_GUNFIRE);

  if (pSoldier->bDoBurst) {
    // done, if bursting, increment
    pSoldier->bDoBurst++;
  }

  // CJC: since jamming is no longer affected by reliability, increase chance of
  // status going down for really unreliabile guns
  uiDepreciateTest = BASIC_DEPRECIATE_CHANCE + 3 * Item[usItemNum].bReliability;

  if (!PreRandom(uiDepreciateTest) && (pSoldier->inv[pSoldier->ubAttackingHand].bStatus[0] > 1)) {
    pSoldier->inv[pSoldier->ubAttackingHand].bStatus[0]--;
  }

  // reduce monster smell (gunpowder smell)
  if (pSoldier->bMonsterSmell > 0 && Random(2) == 0) {
    pSoldier->bMonsterSmell--;
  }

  return (TRUE);
}

static void AgilityForEnemyMissingPlayer(const SOLDIERTYPE *const attacker,
                                         SOLDIERTYPE *const target, const uint32_t agil_amt) {
  // if it was another team attacking someone under our control
  if (target->bTeam != attacker->bTeam && target->bTeam == OUR_TEAM) {
    StatChange(*target, AGILAMT, agil_amt, FROM_SUCCESS);
  }
}

static void UseBlade(SOLDIERTYPE *const pSoldier, int16_t const sTargetGridNo) {
  int32_t iHitChance, iDiceRoll;
  int16_t sAPCost;
  EV_S_WEAPONHIT SWeaponHit;
  int32_t iImpact, iImpactForCrits;
  BOOLEAN fGonnaHit = FALSE;
  uint16_t usExpGain = 0;
  int8_t bMaxDrop;
  BOOLEAN fSurpriseAttack;

  // Deduct points!
  sAPCost = CalcTotalAPsToAttack(pSoldier, sTargetGridNo, FALSE, pSoldier->bAimTime);

  DeductPoints(pSoldier, sAPCost, 0);

  // See if a guy is here!
  SOLDIERTYPE *const pTargetSoldier = WhoIsThere2(sTargetGridNo, pSoldier->bTargetLevel);
  if (pTargetSoldier) {
    // set target as noticed attack
    pSoldier->uiStatusFlags |= SOLDIER_ATTACK_NOTICED;
    pTargetSoldier->fIntendedTarget = TRUE;

    pSoldier->opponent = pTargetSoldier;

    // CHECK IF BUDDY KNOWS ABOUT US
    if (pTargetSoldier->bOppList[pSoldier->ubID] == NOT_HEARD_OR_SEEN ||
        pTargetSoldier->bLife < OKLIFE || pTargetSoldier->bCollapsed) {
      iHitChance = 100;
      fSurpriseAttack = TRUE;
    } else {
      iHitChance = CalcChanceToStab(pSoldier, pTargetSoldier, pSoldier->bAimTime);
      fSurpriseAttack = FALSE;
    }

    // ROLL DICE
    iDiceRoll = (int32_t)PreRandom(100);

    if (iDiceRoll <= iHitChance) {
      fGonnaHit = TRUE;

      // CALCULATE DAMAGE!
      // attack HITS, calculate damage (base damage is 1-maximum knife sImpact)
      iImpact = HTHImpact(pSoldier, pTargetSoldier, (iHitChance - iDiceRoll), TRUE);

      // modify this by the knife's condition (if it's dull, not much good)
      iImpact =
          (iImpact * WEAPON_STATUS_MOD(pSoldier->inv[pSoldier->ubAttackingHand].bStatus[0])) / 100;

      // modify by hit location
      AdjustImpactByHitLocation(iImpact, pSoldier->bAimShotLocation, &iImpact, &iImpactForCrits);

      // bonus for surprise
      if (fSurpriseAttack) {
        iImpact = (iImpact * 3) / 2;
      }

      // any successful hit does at LEAST 1 pt minimum damage
      if (iImpact < 1) {
        iImpact = 1;
      }

      if (pSoldier->inv[pSoldier->ubAttackingHand].bStatus[0] > USABLE) {
        bMaxDrop = (iImpact / 20);

        // the duller they get, the slower they get any worse...
        bMaxDrop =
            std::min(bMaxDrop, (int8_t)(pSoldier->inv[pSoldier->ubAttackingHand].bStatus[0] / 10));

        // as long as its still > USABLE, it drops another point 1/2 the time
        bMaxDrop = std::max(bMaxDrop, (int8_t)2);

        pSoldier->inv[pSoldier->ubAttackingHand].bStatus[0] -=
            (int8_t)Random(bMaxDrop);  // 0 to (maxDrop - 1)
      }

      // Send event for getting hit
      memset(&(SWeaponHit), 0, sizeof(SWeaponHit));
      SWeaponHit.usSoldierID = pTargetSoldier->ubID;
      SWeaponHit.usWeaponIndex = pSoldier->usAttackingWeapon;
      SWeaponHit.sDamage = (int16_t)iImpact;
      SWeaponHit.usDirection = GetDirectionFromGridNo(pSoldier->sGridNo, pTargetSoldier);
      SWeaponHit.sXPos = (int16_t)pTargetSoldier->dXPos;
      SWeaponHit.sYPos = (int16_t)pTargetSoldier->dYPos;
      SWeaponHit.sZPos = 20;
      SWeaponHit.sRange = 1;
      SWeaponHit.ubAttackerID = pSoldier->ubID;
      SWeaponHit.ubSpecial = FIRE_WEAPON_NO_SPECIAL;
      AddGameEvent(S_WEAPONHIT, (uint16_t)20, &SWeaponHit);
    } else {
      // AGILITY GAIN (10):  Target avoids a knife attack
      AgilityForEnemyMissingPlayer(pSoldier, pTargetSoldier, 10);
      DebugMsg(TOPIC_JA2, DBG_LEVEL_3, "@@@@@@@ Freeing up attacker - missed in knife attack");
      FreeUpAttacker(pSoldier);
    }

    if (IsOnOurTeam(*pSoldier) && pSoldier->target != NULL) {
      if (fGonnaHit) {
        // grant extra exp for hitting a difficult target
        usExpGain += (uint8_t)(100 - iHitChance) / 10;

        if (pSoldier->bAimTime) {
          // gain extra exp for aiming, up to the amount from
          // the difficulty of the attack
          usExpGain += (2 * std::min((uint16_t)pSoldier->bAimTime, usExpGain));
        }

        // base pts extra for hitting
        usExpGain += 10;
      }

      // add base pts for taking a shot, whether it hits or misses
      usExpGain += 10;

      usExpGain = ModifyExpGainByTarget(usExpGain, pSoldier->target);

      // DEXTERITY GAIN:  Made a knife attack, successful or not
      StatChange(*pSoldier, DEXTAMT, usExpGain, fGonnaHit ? FROM_SUCCESS : FROM_FAILURE);
    }
  } else {
    DebugMsg(TOPIC_JA2, DBG_LEVEL_3, "@@@@@@@ Freeing up attacker - missed in knife attack");
    FreeUpAttacker(pSoldier);
  }

  // possibly reduce monster smell
  if (pSoldier->bMonsterSmell > 0 && Random(5) == 0) {
    pSoldier->bMonsterSmell--;
  }
}

static uint32_t CalcChanceToSteal(SOLDIERTYPE *pAttacker, SOLDIERTYPE *pDefender,
                                  uint8_t ubAimTime);

void UseHandToHand(SOLDIERTYPE *const pSoldier, int16_t const sTargetGridNo,
                   BOOLEAN const fStealing) {
  int32_t iHitChance, iDiceRoll;
  int16_t sAPCost;
  EV_S_WEAPONHIT SWeaponHit;
  int32_t iImpact;
  uint16_t usOldItem;

  // Deduct points!
  // August 13 2002: unless stealing - APs already deducted elsewhere

  //	if (!fStealing)
  {
    sAPCost = CalcTotalAPsToAttack(pSoldier, sTargetGridNo, FALSE, pSoldier->bAimTime);

    DeductPoints(pSoldier, sAPCost, 0);
  }

  // See if a guy is here!
  SOLDIERTYPE *const pTargetSoldier = WhoIsThere2(sTargetGridNo, pSoldier->bTargetLevel);
  if (pTargetSoldier) {
    // set target as noticed attack
    pSoldier->uiStatusFlags |= SOLDIER_ATTACK_NOTICED;
    pTargetSoldier->fIntendedTarget = TRUE;

    pSoldier->opponent = pTargetSoldier;

    if (fStealing) {
      if (AM_A_ROBOT(pTargetSoldier) || TANK(pTargetSoldier) ||
          CREATURE_OR_BLOODCAT(pTargetSoldier) || TANK(pTargetSoldier)) {
        iHitChance = 0;
      } else if (pTargetSoldier->bOppList[pSoldier->ubID] == NOT_HEARD_OR_SEEN) {
        // give bonus for surprise, but not so much as struggle would still
        // occur
        iHitChance = CalcChanceToSteal(pSoldier, pTargetSoldier, pSoldier->bAimTime) + 20;
      } else if (pTargetSoldier->bLife < OKLIFE || pTargetSoldier->bCollapsed) {
        iHitChance = 100;
      } else {
        iHitChance = CalcChanceToSteal(pSoldier, pTargetSoldier, pSoldier->bAimTime);
      }
    } else {
      if (pTargetSoldier->bOppList[pSoldier->ubID] == NOT_HEARD_OR_SEEN ||
          pTargetSoldier->bLife < OKLIFE || pTargetSoldier->bCollapsed) {
        iHitChance = 100;
      } else {
        iHitChance = CalcChanceToPunch(pSoldier, pTargetSoldier, pSoldier->bAimTime);
      }
    }

    // ROLL DICE
    iDiceRoll = (int32_t)PreRandom(100);

    if (fStealing) {
      if (pTargetSoldier->inv[HANDPOS].usItem != NOTHING) {
        if (iDiceRoll <= iHitChance) {
          // Was a good steal!
          ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, g_langRes->Message[STR_STOLE_SOMETHING],
                    pSoldier->name, ShortItemNames[pTargetSoldier->inv[HANDPOS].usItem]);

          usOldItem = pTargetSoldier->inv[HANDPOS].usItem;

          if (pSoldier->bTeam == OUR_TEAM && pTargetSoldier->bTeam != OUR_TEAM &&
              !IsMechanical(*pTargetSoldier) && !TANK(pTargetSoldier)) {
            // made a steal; give experience
            StatChange(*pSoldier, STRAMT, 8, FROM_SUCCESS);
          }

          if (iDiceRoll <= iHitChance * 2 / 3) {
            // Grabbed item
            if (AutoPlaceObject(pSoldier, &(pTargetSoldier->inv[HANDPOS]), TRUE)) {
              // Item transferred; remove it from the target's inventory
              DeleteObj(&(pTargetSoldier->inv[HANDPOS]));
            } else {
              // No room to hold it so the item should drop in our tile again
              AddItemToPool(pSoldier->sGridNo, &pTargetSoldier->inv[HANDPOS], VISIBLE,
                            pSoldier->bLevel, 0, -1);
              DeleteObj(&(pTargetSoldier->inv[HANDPOS]));
            }
          } else {
            if (pSoldier->bTeam == OUR_TEAM) {
              DoMercBattleSound(pSoldier, BATTLE_SOUND_CURSE1);
            }

            // Item dropped somewhere... roll based on the same chance to
            // determine where!
            iDiceRoll = (int32_t)PreRandom(100);
            if (iDiceRoll < iHitChance) {
              // Drop item in the our tile
              AddItemToPool(pSoldier->sGridNo, &(pTargetSoldier->inv[HANDPOS]), VISIBLE,
                            pSoldier->bLevel, 0, -1);
            } else {
              // Drop item in the target's tile
              AddItemToPool(pTargetSoldier->sGridNo, &pTargetSoldier->inv[HANDPOS], VISIBLE,
                            pSoldier->bLevel, 0, -1);
            }
            DeleteObj(&(pTargetSoldier->inv[HANDPOS]));
          }

          // Reload buddy's animation...
          ReLoadSoldierAnimationDueToHandItemChange(pTargetSoldier, usOldItem, NOTHING);

        } else {
          ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE,
                    g_langRes->Message[STR_FAILED_TO_STEAL_SOMETHING], pSoldier->name,
                    ShortItemNames[pTargetSoldier->inv[HANDPOS].usItem]);
          if (pSoldier->bTeam == OUR_TEAM) {
            DoMercBattleSound(pSoldier, BATTLE_SOUND_CURSE1);
          }

          if (iHitChance > 0 && pSoldier->bTeam == OUR_TEAM && pTargetSoldier->bTeam != OUR_TEAM &&
              !IsMechanical(*pTargetSoldier) && !TANK(pTargetSoldier)) {
            // failed a steal; give some experience
            StatChange(*pSoldier, STRAMT, 4, FROM_FAILURE);
          }
        }
      }

      FreeUpAttacker(pSoldier);
    } else {
      // ATE/CC: if doing ninja spin kick (only), automatically make it a hit
      if (pSoldier->usAnimState == NINJA_SPINKICK) {
        // Let him to succeed by a random amount
        iDiceRoll = PreRandom(iHitChance);
      }

      if (pSoldier->bTeam == OUR_TEAM && pTargetSoldier->bTeam != OUR_TEAM) {
        // made an HTH attack; give experience
        uint8_t ubExpGain;
        StatChangeCause reason;
        if (iDiceRoll <= iHitChance) {
          ubExpGain = 8;
          reason = FROM_SUCCESS;
        } else {
          ubExpGain = 4;
          reason = FROM_FAILURE;
        }

        ubExpGain = ModifyExpGainByTarget(ubExpGain, pTargetSoldier);

        StatChange(*pSoldier, STRAMT, ubExpGain, reason);
        StatChange(*pSoldier, DEXTAMT, ubExpGain, reason);
      } else if (iDiceRoll > iHitChance) {
        // being attacked... successfully dodged, give experience
        AgilityForEnemyMissingPlayer(pSoldier, pTargetSoldier, 8);
      }

      if (iDiceRoll <= iHitChance || AreInMeanwhile()) {
        // CALCULATE DAMAGE!
        iImpact = HTHImpact(pSoldier, pTargetSoldier, (iHitChance - iDiceRoll), FALSE);

        // Send event for getting hit
        memset(&(SWeaponHit), 0, sizeof(SWeaponHit));
        SWeaponHit.usSoldierID = pTargetSoldier->ubID;
        SWeaponHit.usWeaponIndex = pSoldier->usAttackingWeapon;
        SWeaponHit.sDamage = (int16_t)iImpact;
        SWeaponHit.usDirection = GetDirectionFromGridNo(pSoldier->sGridNo, pTargetSoldier);
        SWeaponHit.sXPos = (int16_t)pTargetSoldier->dXPos;
        SWeaponHit.sYPos = (int16_t)pTargetSoldier->dYPos;
        SWeaponHit.sZPos = 20;
        SWeaponHit.sRange = 1;
        SWeaponHit.ubAttackerID = pSoldier->ubID;
        SWeaponHit.ubSpecial = FIRE_WEAPON_NO_SPECIAL;
        AddGameEvent(S_WEAPONHIT, (uint16_t)20, &SWeaponHit);
      } else {
        DebugMsg(TOPIC_JA2, DBG_LEVEL_3, "@@@@@@@ Freeing up attacker - missed in HTH attack");
        FreeUpAttacker(pSoldier);
      }
    }
  }

  // possibly reduce monster smell (gunpowder smell)
  if (pSoldier->bMonsterSmell > 0 && Random(5) == 0) {
    pSoldier->bMonsterSmell--;
  }
}

static void UseThrown(SOLDIERTYPE *const pSoldier, int16_t const sTargetGridNo) {
  uint32_t uiHitChance, uiDiceRoll;
  int8_t bLoop;

  uiHitChance = CalcThrownChanceToHit(pSoldier, sTargetGridNo, pSoldier->bAimTime, AIM_SHOT_TORSO);

  uiDiceRoll = PreRandom(100);

  if (pSoldier->bTeam == OUR_TEAM && gTacticalStatus.uiFlags & INCOMBAT) {
    // check target gridno
    const SOLDIERTYPE *pTargetSoldier =
        WhoIsThere2(pSoldier->sTargetGridNo, pSoldier->bTargetLevel);
    if (pTargetSoldier && pTargetSoldier->bTeam == pSoldier->bTeam) {
      // ignore!
      pTargetSoldier = NULL;
    }

    if (pTargetSoldier == NULL) {
      // search for an opponent near the target gridno
      for (bLoop = 0; bLoop < NUM_WORLD_DIRECTIONS; bLoop++) {
        pTargetSoldier = WhoIsThere2(NewGridNo(pSoldier->sTargetGridNo, DirectionInc(bLoop)),
                                     pSoldier->bTargetLevel);
        if (pTargetSoldier != NULL && pTargetSoldier->bTeam != pSoldier->bTeam) break;
      }
    }

    if (pTargetSoldier) {
      // ok this is a real attack on someone, grant experience
      StatChange(*pSoldier, STRAMT, 5, FROM_SUCCESS);
      if (uiDiceRoll < uiHitChance) {
        StatChange(*pSoldier, DEXTAMT, 5, FROM_SUCCESS);
        StatChange(*pSoldier, MARKAMT, 5, FROM_SUCCESS);
      } else {
        StatChange(*pSoldier, DEXTAMT, 2, FROM_FAILURE);
        StatChange(*pSoldier, MARKAMT, 2, FROM_FAILURE);
      }
    }
  }

  CalculateLaunchItemParamsForThrow(
      pSoldier, sTargetGridNo, pSoldier->bTargetLevel, (int16_t)(pSoldier->bTargetLevel * 256),
      &(pSoldier->inv[HANDPOS]), (int8_t)(uiDiceRoll - uiHitChance), THROW_ARM_ITEM, 0);

  // OK, goto throw animation
  HandleSoldierThrowItem(pSoldier, pSoldier->sTargetGridNo);

  RemoveObjs(&(pSoldier->inv[HANDPOS]), 1);
}

static BOOLEAN UseLauncher(SOLDIERTYPE *pSoldier, int16_t sTargetGridNo) {
  uint32_t uiHitChance, uiDiceRoll;
  int16_t sAPCost = 0;
  int8_t bAttachPos;
  OBJECTTYPE Launchable;
  OBJECTTYPE *pObj;
  uint16_t usItemNum;

  usItemNum = pSoldier->usAttackingWeapon;

  if (!EnoughAmmo(pSoldier, TRUE, pSoldier->ubAttackingHand)) {
    return (FALSE);
  }

  pObj = &(pSoldier->inv[HANDPOS]);
  for (bAttachPos = 0; bAttachPos < MAX_ATTACHMENTS; bAttachPos++) {
    if (pObj->usAttachItem[bAttachPos] != NOTHING) {
      if (Item[pObj->usAttachItem[bAttachPos]].usItemClass & IC_EXPLOSV) {
        break;
      }
    }
  }
  if (bAttachPos == MAX_ATTACHMENTS) {
    // this should not happen!!
    return (FALSE);
  }

  CreateItem(pObj->usAttachItem[bAttachPos], pObj->bAttachStatus[bAttachPos], &Launchable);

  if (pSoldier->usAttackingWeapon == pObj->usItem) {
    DeductAmmo(pSoldier, HANDPOS);
  } else {
    // Firing an attached grenade launcher... the attachment we found above
    // is the one to remove!
    RemoveAttachment(pObj, bAttachPos, NULL);
  }

  // ATE: Check here if the launcher should fail 'cause of bad status.....
  if (WillExplosiveWeaponFail(pSoldier, pObj)) {
    // Explode dude!

    // So we still should have ABC > 0
    // Begin explosion due to failure...
    IgniteExplosion(pSoldier, 0, pSoldier->sGridNo, Launchable.usItem, pSoldier->bLevel);

    // Reduce again for attack end 'cause it has been incremented for a normal
    // attack
    //
    DebugMsg(
        TOPIC_JA2, DBG_LEVEL_3,
        String("@@@@@@@ Freeing up attacker - ATTACK ANIMATION %hs ENDED "
               "BY BAD EXPLOSIVE CHECK, Now %d",
               gAnimControl[pSoldier->usAnimState].zAnimStr, gTacticalStatus.ubAttackBusyCount));
    ReduceAttackBusyCount(pSoldier, FALSE);

    // So all's well, should be good from here....
    return (FALSE);
  }

  if (Weapon[usItemNum].sSound != NO_WEAPON_SOUND) {
    PlayLocationJA2Sample(pSoldier->sGridNo, Weapon[usItemNum].sSound, HIGHVOLUME, 1);
  }

  uiHitChance = CalcThrownChanceToHit(pSoldier, sTargetGridNo, pSoldier->bAimTime, AIM_SHOT_TORSO);

  uiDiceRoll = PreRandom(100);

  if (Item[usItemNum].usItemClass == IC_LAUNCHER) {
    // Preserve gridno!
    // pSoldier->sLastTarget = sTargetGridNo;

    sAPCost = MinAPsToAttack(pSoldier, sTargetGridNo, TRUE);
  } else {
    // Throw....
    sAPCost = MinAPsToThrow(*pSoldier, sTargetGridNo, FALSE);
  }

  DeductPoints(pSoldier, sAPCost, 0);

  CalculateLaunchItemParamsForThrow(pSoldier, pSoldier->sTargetGridNo, pSoldier->bTargetLevel, 0,
                                    &Launchable, (int8_t)(uiDiceRoll - uiHitChance), THROW_ARM_ITEM,
                                    0);

  const THROW_PARAMS *const t = pSoldier->pThrowParams;
  CreatePhysicalObject(pSoldier->pTempObject, t->dLifeSpan, t->dX, t->dY, t->dZ, t->dForceX,
                       t->dForceY, t->dForceZ, pSoldier, t->ubActionCode, t->target);

  MemFree(pSoldier->pTempObject);
  pSoldier->pTempObject = NULL;

  MemFree(pSoldier->pThrowParams);
  pSoldier->pThrowParams = NULL;

  return (TRUE);
}

static BOOLEAN DoSpecialEffectAmmoMiss(SOLDIERTYPE *const attacker, const int16_t sGridNo,
                                       const int16_t sXPos, const int16_t sYPos,
                                       const int16_t sZPos, const BOOLEAN fSoundOnly,
                                       const BOOLEAN fFreeupAttacker, BULLET *const bullet) {
  ANITILE_PARAMS AniParams;
  uint8_t ubAmmoType;
  uint16_t usItem;

  ubAmmoType = attacker->inv[attacker->ubAttackingHand].ubGunAmmoType;
  usItem = attacker->inv[attacker->ubAttackingHand].usItem;

  memset(&AniParams, 0, sizeof(ANITILE_PARAMS));

  if (ubAmmoType == AMMO_HE || ubAmmoType == AMMO_HEAT) {
    if (!fSoundOnly) {
      AniParams.sGridNo = sGridNo;
      AniParams.ubLevelID = ANI_TOPMOST_LEVEL;
      AniParams.sDelay = (int16_t)(100);
      AniParams.sStartFrame = 0;
      AniParams.uiFlags = ANITILE_FORWARD | ANITILE_ALWAYS_TRANSLUCENT;
      AniParams.sX = sXPos;
      AniParams.sY = sYPos;
      AniParams.sZ = sZPos;
      AniParams.zCachedFile = TILECACHEDIR "/miniboom.sti";
      CreateAnimationTile(&AniParams);

      if (fFreeupAttacker) {
        if (bullet) RemoveBullet(bullet);
        DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
                 "@@@@@@@ Freeing up attacker - bullet hit structure - "
                 "explosive ammo");
        FreeUpAttacker(attacker);
      }
    }

    if (sGridNo != NOWHERE) {
      PlayLocationJA2Sample(sGridNo, SMALL_EXPLODE_1, HIGHVOLUME, 1);
    } else {
      PlayJA2Sample(SMALL_EXPLODE_1, MIDVOLUME, 1, MIDDLE);
    }

    return (TRUE);
  } else {
    uint16_t gas;
    switch (usItem) {
      case CREATURE_YOUNG_MALE_SPIT:
      case CREATURE_INFANT_SPIT:
        gas = VERY_SMALL_CREATURE_GAS;
        break;
      case CREATURE_OLD_MALE_SPIT:
        gas = SMALL_CREATURE_GAS;
        break;
      case CREATURE_QUEEN_SPIT:
        gas = LARGE_CREATURE_GAS;
        break;
      default:
        return FALSE;
    }

    // Increment attack busy...
    // gTacticalStatus.ubAttackBusyCount++;
    // DebugMsg( TOPIC_JA2, DBG_LEVEL_3, String("Incrementing Attack: Explosion
    // gone off, COunt now %d", gTacticalStatus.ubAttackBusyCount ) );

    PlayLocationJA2Sample(sGridNo, CREATURE_GAS_NOISE, HIGHVOLUME, 1);

    NewSmokeEffect(sGridNo, gas, 0, attacker);
  }

  return (FALSE);
}

void WeaponHit(SOLDIERTYPE *const pTargetSoldier, const uint16_t usWeaponIndex,
               const int16_t sDamage, const int16_t sBreathLoss, const uint16_t usDirection,
               const int16_t sXPos, const int16_t sYPos, const int16_t sZPos, const int16_t sRange,
               SOLDIERTYPE *const attacker, const uint8_t ubSpecial, const uint8_t ubHitLocation) {
  MakeNoise(attacker, pTargetSoldier->sGridNo, pTargetSoldier->bLevel,
            Weapon[usWeaponIndex].ubHitVolume, NOISE_BULLET_IMPACT);

  if (EXPLOSIVE_GUN(usWeaponIndex)) {
    // Reduce attacker count!
    const uint16_t item = (usWeaponIndex == ROCKET_LAUNCHER ? C1 : TANK_SHELL);
    IgniteExplosionXY(attacker, sXPos, sYPos, 0, GETWORLDINDEXFROMWORLDCOORDS(sYPos, sXPos), item,
                      pTargetSoldier->bLevel);

    DebugMsg(TOPIC_JA2, DBG_LEVEL_3, "@@@@@@@ Freeing up attacker - end of LAW fire");
    FreeUpAttacker(attacker);
    return;
  }

  DoSpecialEffectAmmoMiss(attacker, pTargetSoldier->sGridNo, sXPos, sYPos, sZPos, FALSE, FALSE,
                          NULL);

  // OK, SHOT HAS HIT, DO THINGS APPROPRIATELY
  // ATE: This is 'cause of that darn smoke effect that could potnetially kill
  // the poor bastard .. so check
  if (!pTargetSoldier->fDoingExternalDeath) {
    EVENT_SoldierGotHit(pTargetSoldier, usWeaponIndex, sDamage, sBreathLoss, usDirection, sRange,
                        attacker, ubSpecial, ubHitLocation, NOWHERE);
  } else {
    // Buddy had died from additional dammage - free up attacker here...
    ReduceAttackBusyCount(pTargetSoldier->attacker, FALSE);
    DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
             String("Special effect killed before bullet impact, attack count now %d",
                    gTacticalStatus.ubAttackBusyCount));
  }
}

void StructureHit(BULLET *const pBullet, const int16_t sXPos, const int16_t sYPos,
                  const int16_t sZPos, const uint16_t usStructureID, const int32_t iImpact,
                  const BOOLEAN fStopped) {
  BOOLEAN fDoMissForGun = FALSE;
  ANITILE *pNode;
  int16_t sGridNo;
  ANITILE_PARAMS AniParams;
  uint32_t uiMissVolume = MIDVOLUME;

  SOLDIERTYPE *const attacker = pBullet->pFirer;
  const uint16_t usWeaponIndex = attacker->usAttackingWeapon;
  const int8_t bWeaponStatus = pBullet->ubItemStatus;

  if (fStopped) {
    // AGILITY GAIN: Opponent "dodged" a bullet shot at him (it missed)
    SOLDIERTYPE *const opp = attacker->opponent;
    if (opp != NULL) AgilityForEnemyMissingPlayer(attacker, opp, 5);
  }

  const BOOLEAN fHitSameStructureAsBefore = (usStructureID == pBullet->usLastStructureHit);

  sGridNo = MAPROWCOLTOPOS((sYPos / CELL_Y_SIZE), (sXPos / CELL_X_SIZE));
  if (!fHitSameStructureAsBefore) {
    const int8_t level = (sZPos > WALL_HEIGHT ? 1 : 0);
    MakeNoise(attacker, sGridNo, level, Weapon[usWeaponIndex].ubHitVolume, NOISE_BULLET_IMPACT);
  }

  if (fStopped) {
    if (usWeaponIndex == ROCKET_LAUNCHER) {
      RemoveBullet(pBullet);

      // Reduce attacker count!
      DebugMsg(TOPIC_JA2, DBG_LEVEL_3, "@@@@@@@ Freeing up attacker - end of LAW fire");
      FreeUpAttacker(attacker);

      IgniteExplosion(attacker, 0, sGridNo, C1, sZPos >= WALL_HEIGHT);
      // FreeUpAttacker(attacker);

      return;
    }

    if (usWeaponIndex == TANK_CANNON) {
      RemoveBullet(pBullet);

      // Reduce attacker count!
      DebugMsg(TOPIC_JA2, DBG_LEVEL_3, "@@@@@@@ Freeing up attacker - end of TANK fire");
      FreeUpAttacker(attacker);

      IgniteExplosion(attacker, 0, sGridNo, TANK_SHELL, sZPos >= WALL_HEIGHT);
      // FreeUpAttacker(attacker);

      return;
    }
  }

  // Get Structure pointer and damage it!
  if (usStructureID != INVALID_STRUCTURE_ID) {
    STRUCTURE *const pStructure = FindStructureByID(sGridNo, usStructureID);
    DamageStructure(pStructure, iImpact, STRUCTURE_DAMAGE_GUNFIRE, sGridNo, sXPos, sYPos, attacker);
  }

  switch (Weapon[usWeaponIndex].ubWeaponClass) {
    case HANDGUNCLASS:
    case RIFLECLASS:
    case SHOTGUNCLASS:
    case SMGCLASS:
    case MGCLASS:
      // Guy has missed, play random sound
      if (attacker->bTeam == OUR_TEAM && !attacker->bDoBurst && Random(40) == 0) {
        DoMercBattleSound(attacker, BATTLE_SOUND_CURSE1);
      }
      // fDoMissForGun = TRUE;
      // break;
      fDoMissForGun = TRUE;
      break;

    case MONSTERCLASS:
      DoSpecialEffectAmmoMiss(attacker, sGridNo, sXPos, sYPos, sZPos, FALSE, TRUE, pBullet);

      RemoveBullet(pBullet);
      DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
               "@@@@@@@ Freeing up attacker - monster attack hit structure");
      FreeUpAttacker(attacker);

      // PlayJA2Sample(SPIT_RICOCHET, uiMissVolume, 1, SoundDir(sGridNo));
      break;

    case KNIFECLASS:

      // When it hits the ground, leave on map...
      if (Item[usWeaponIndex].usItemClass == IC_THROWING_KNIFE) {
        OBJECTTYPE Object;

        // OK, have we hit ground?
        if (usStructureID == INVALID_STRUCTURE_ID) {
          // Add item
          CreateItem(THROWING_KNIFE, bWeaponStatus, &Object);

          AddItemToPool(sGridNo, &Object, INVISIBLE, 0, 0, -1);

          // Make team look for items
          NotifySoldiersToLookforItems();
        }

        if (!fHitSameStructureAsBefore) {
          PlayJA2Sample(MISS_KNIFE, uiMissVolume, 1, SoundDir(sGridNo));
        }

        RemoveBullet(pBullet);
        DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
                 "@@@@@@@ Freeing up attacker - knife attack hit structure");
        FreeUpAttacker(attacker);
      }
  }

  if (fDoMissForGun) {
    // OK, are we a shotgun, if so , make sounds lower...
    if (Weapon[usWeaponIndex].ubWeaponClass == SHOTGUNCLASS) {
      uiMissVolume = LOWVOLUME;
    }

    // Free guy!
    // DebugMsg(TOPIC_JA2, DBG_LEVEL_3, "@@@@@@@ Freeing up attacker - bullet
    // hit structure"); FreeUpAttacker(attacker);

    // PLAY SOUND AND FLING DEBRIS
    // RANDOMIZE SOUND SYSTEM

    // IF WE HIT THE GROUND

    if (fHitSameStructureAsBefore) {
      if (fStopped) {
        RemoveBullet(pBullet);
        DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
                 "@@@@@@@ Freeing up attacker - bullet hit same structure twice");
        FreeUpAttacker(attacker);
      }
    } else {
      if (!fStopped ||
          !DoSpecialEffectAmmoMiss(attacker, sGridNo, sXPos, sYPos, sZPos, FALSE, TRUE, pBullet)) {
        if (sZPos == 0) {
          PlayJA2Sample(MISS_G2, uiMissVolume, 1, SoundDir(sGridNo));
        } else {
          PlayJA2Sample(SoundRange<MISS_1, MISS_8>(), uiMissVolume, 1, SoundDir(sGridNo));
        }

        // Default hit is the ground
        uint16_t usMissTileIndex = FIRSTMISS1;

        // Check if we are in water...
        if (gpWorldLevelData[sGridNo].ubTerrainID == LOW_WATER ||
            gpWorldLevelData[sGridNo].ubTerrainID == DEEP_WATER) {
          usMissTileIndex = SECONDMISS1;

          // Add ripple
          memset(&AniParams, 0, sizeof(ANITILE_PARAMS));
          AniParams.sGridNo = sGridNo;
          AniParams.ubLevelID = ANI_STRUCT_LEVEL;
          AniParams.usTileIndex = THIRDMISS1;
          AniParams.sDelay = 50;
          AniParams.sStartFrame = 0;
          AniParams.uiFlags = ANITILE_FORWARD;

          pNode = CreateAnimationTile(&AniParams);

          // Adjust for absolute positioning
          pNode->pLevelNode->uiFlags |= LEVELNODE_USEABSOLUTEPOS;
          pNode->pLevelNode->sRelativeX = sXPos;
          pNode->pLevelNode->sRelativeY = sYPos;
          pNode->pLevelNode->sRelativeZ = sZPos;
        }

        memset(&AniParams, 0, sizeof(ANITILE_PARAMS));
        AniParams.sGridNo = sGridNo;
        AniParams.ubLevelID = ANI_STRUCT_LEVEL;
        AniParams.usTileIndex = usMissTileIndex;
        AniParams.sDelay = 80;
        AniParams.sStartFrame = 0;
        if (fStopped) {
          AniParams.uiFlags = ANITILE_FORWARD | ANITILE_RELEASE_ATTACKER_WHEN_DONE;
          AniParams.v.bullet = pBullet;
        } else {
          AniParams.uiFlags = ANITILE_FORWARD;
        }

        pNode = CreateAnimationTile(&AniParams);

        // Adjust for absolute positioning
        pNode->pLevelNode->uiFlags |= LEVELNODE_USEABSOLUTEPOS;
        pNode->pLevelNode->sRelativeX = sXPos;
        pNode->pLevelNode->sRelativeY = sYPos;
        pNode->pLevelNode->sRelativeZ = sZPos;

        // ATE: Show misses...( if our team )
        if (gGameSettings.fOptions[TOPTION_SHOW_MISSES] && attacker->bTeam == OUR_TEAM) {
          LocateGridNo(sGridNo);
        }
      }

      pBullet->usLastStructureHit = usStructureID;
    }
  }
}

void WindowHit(int16_t sGridNo, uint16_t usStructureID, BOOLEAN fBlowWindowSouth,
               BOOLEAN fLargeForce) {
  STRUCTURE *pWallAndWindow;
  DB_STRUCTURE *pWallAndWindowInDB;
  int16_t sShatterGridNo;
  uint16_t usTileIndex;
  ANITILE *pNode;
  ANITILE_PARAMS AniParams;

  // ATE: Make large force always for now ( feel thing )
  fLargeForce = TRUE;

  // we have to do two things here: swap the window structure
  // (right now just using the partner stuff in a chain from
  // intact to cracked to shattered) and display the
  // animation if we've reached shattered

  // find the wall structure, and go one length along the chain
  pWallAndWindow = FindStructureByID(sGridNo, usStructureID);
  if (pWallAndWindow == NULL) {
    return;
  }

  pWallAndWindow = SwapStructureForPartner(pWallAndWindow);
  if (pWallAndWindow == NULL) {
    return;
  }

  // record window smash
  AddWindowHitToMapTempFile(sGridNo);

  pWallAndWindowInDB = pWallAndWindow->pDBStructureRef->pDBStructure;

  if (fLargeForce) {
    // Force to destruction animation!
    if (pWallAndWindowInDB->bPartnerDelta != NO_PARTNER_STRUCTURE) {
      pWallAndWindow = SwapStructureForPartner(pWallAndWindow);
      if (pWallAndWindow) {
        // record 2nd window smash
        AddWindowHitToMapTempFile(sGridNo);

        pWallAndWindowInDB = pWallAndWindow->pDBStructureRef->pDBStructure;
      }
    }
  }

  SetRenderFlags(RENDER_FLAG_FULL);

  if (pWallAndWindowInDB->ubArmour == MATERIAL_THICKER_METAL_WITH_SCREEN_WINDOWS) {
    // don't play any sort of animation or sound
    return;
  }

  if (pWallAndWindowInDB->bPartnerDelta !=
      NO_PARTNER_STRUCTURE) {  // just cracked; don't display the animation
    MakeNoise(NULL, sGridNo, 0, WINDOW_CRACK_VOLUME, NOISE_BULLET_IMPACT);
    return;
  }
  MakeNoise(NULL, sGridNo, 0, WINDOW_SMASH_VOLUME, NOISE_BULLET_IMPACT);
  if (pWallAndWindowInDB->ubWallOrientation == INSIDE_TOP_RIGHT ||
      pWallAndWindowInDB->ubWallOrientation == OUTSIDE_TOP_RIGHT) {
    /*
            sShatterGridNo = sGridNo + 1;
            // check for wrapping around edge of map
            if (sShatterGridNo % WORLD_COLS == 0)
            {
                    // in which case we don't play the animation!
                    return;
            }*/
    if (fBlowWindowSouth) {
      usTileIndex = WINDOWSHATTER1;
      sShatterGridNo = sGridNo + 1;
    } else {
      usTileIndex = WINDOWSHATTER11;
      sShatterGridNo = sGridNo;
    }

  } else {
    /*
            sShatterGridNo = sGridNo + WORLD_COLS;
            // check for wrapping around edge of map
            if (sShatterGridNo % WORLD_ROWS == 0)
            {
                    // in which case we don't play the animation!
                    return;
            }*/
    if (fBlowWindowSouth) {
      usTileIndex = WINDOWSHATTER6;
      sShatterGridNo = sGridNo + WORLD_COLS;
    } else {
      usTileIndex = WINDOWSHATTER16;
      sShatterGridNo = sGridNo;
    }
  }

  memset(&AniParams, 0, sizeof(ANITILE_PARAMS));
  AniParams.sGridNo = sShatterGridNo;
  AniParams.ubLevelID = ANI_STRUCT_LEVEL;
  AniParams.usTileIndex = usTileIndex;
  AniParams.sDelay = 50;
  AniParams.sStartFrame = 0;
  AniParams.uiFlags = ANITILE_FORWARD;

  pNode = CreateAnimationTile(&AniParams);

  PlayJA2Sample(SoundRange<GLASS_SHATTER1, GLASS_SHATTER2>(), MIDVOLUME, 1, SoundDir(sGridNo));
}

BOOLEAN InRange(const SOLDIERTYPE *pSoldier, int16_t sGridNo) {
  int16_t sRange;
  uint16_t usInHand;

  usInHand = pSoldier->inv[HANDPOS].usItem;

  if (Item[usInHand].usItemClass == IC_GUN || Item[usInHand].usItemClass == IC_THROWING_KNIFE) {
    // Determine range
    sRange = (int16_t)GetRangeInCellCoordsFromGridNoDiff(pSoldier->sGridNo, sGridNo);

    if (Item[usInHand].usItemClass == IC_THROWING_KNIFE) {
      // NB CalcMaxTossRange returns range in tiles, not in world units
      if (sRange <= CalcMaxTossRange(pSoldier, THROWING_KNIFE, TRUE) * CELL_X_SIZE) {
        return (TRUE);
      }
    } else {
      // For given weapon, check range
      if (sRange <= GunRange(pSoldier->inv[HANDPOS])) {
        return (TRUE);
      }
    }
  }
  return (FALSE);
}

uint32_t CalcChanceToHitGun(SOLDIERTYPE *pSoldier, uint16_t sGridNo, uint8_t ubAimTime,
                            uint8_t ubAimPos) {
  int32_t iChance, iRange, iSightRange, iMaxRange, iScopeBonus,
      iBonus;  //, minRange;
  int32_t iGunCondition, iMarksmanship;
  int32_t iPenalty;
  uint16_t usInHand;
  OBJECTTYPE *pInHand;
  int8_t bAttachPos;
  int8_t bBandaged;
  int16_t sDistVis;
  uint8_t ubAdjAimPos;

  if (pSoldier->bMarksmanship == 0) {
    // always min chance
    return (MINCHANCETOHIT);
  }

  // make sure the guy's actually got a weapon in his hand!
  pInHand = &(pSoldier->inv[pSoldier->ubAttackingHand]);
  usInHand = pSoldier->usAttackingWeapon;

  // DETERMINE BASE CHANCE OF HITTING
  iGunCondition = WEAPON_STATUS_MOD(pInHand->bGunStatus);

  if (usInHand == ROCKET_LAUNCHER) {
    // use the same calculation as for mechanical thrown weapons
    iMarksmanship = (EffectiveDexterity(pSoldier) + EffectiveMarksmanship(pSoldier) +
                     EffectiveWisdom(pSoldier) + (10 * EffectiveExpLevel(pSoldier))) /
                    4;
    // heavy weapons trait helps out
    iMarksmanship += gbSkillTraitBonus[HEAVY_WEAPS] * NUM_SKILL_TRAITS(pSoldier, HEAVY_WEAPS);
  } else {
    iMarksmanship = EffectiveMarksmanship(pSoldier);

    if (AM_A_ROBOT(pSoldier)) {
      SOLDIERTYPE *pSoldier2;

      pSoldier2 = GetRobotController(pSoldier);
      if (pSoldier2) {
        iMarksmanship = std::max(iMarksmanship, (int32_t)EffectiveMarksmanship(pSoldier2));
      }
    }
  }

  // modify chance to hit by morale
  iMarksmanship += GetMoraleModifier(pSoldier);

  // penalize marksmanship for fatigue
  iMarksmanship -= GetSkillCheckPenaltyForFatigue(pSoldier, iMarksmanship);

  if (iGunCondition >= iMarksmanship)
    // base chance is equal to the shooter's marksmanship skill
    iChance = iMarksmanship;
  else
    // base chance is equal to the average of marksmanship & gun's condition!
    iChance = (iMarksmanship + iGunCondition) / 2;

  // if shooting same target as the last shot
  if (sGridNo == pSoldier->sLastTarget) iChance += AIM_BONUS_SAME_TARGET;  // give a bonus to hit

  if (pSoldier->ubProfile != NO_PROFILE &&
      gMercProfiles[pSoldier->ubProfile].bPersonalityTrait == PSYCHO) {
    iChance += AIM_BONUS_PSYCHO;
  }

  // calculate actual range (in units, 10 units = 1 tile)
  iRange = GetRangeInCellCoordsFromGridNoDiff(pSoldier->sGridNo, sGridNo);

  // if shooter is crouched, he aims slightly better (to max of
  // AIM_BONUS_CROUCHING)
  if (gAnimControl[pSoldier->usAnimState].ubEndHeight == ANIM_CROUCH) {
    iBonus = iRange / 10;
    if (iBonus > AIM_BONUS_CROUCHING) {
      iBonus = AIM_BONUS_CROUCHING;
    }
    iChance += iBonus;
  }
  // if shooter is prone, he aims even better, except at really close range
  else if (gAnimControl[pSoldier->usAnimState].ubEndHeight == ANIM_PRONE) {
    if (iRange > MIN_PRONE_RANGE) {
      iBonus = iRange / 10;
      if (iBonus > AIM_BONUS_PRONE) {
        iBonus = AIM_BONUS_PRONE;
      }
      bAttachPos = FindAttachment(pInHand, BIPOD);
      if (bAttachPos != ITEM_NOT_FOUND) {  // extra bonus to hit for a bipod, up
                                           // to half the prone bonus itself
        iBonus += (iBonus * WEAPON_STATUS_MOD(pInHand->bAttachStatus[bAttachPos]) / 100) / 2;
      }
      iChance += iBonus;
    }
  }

  if (!(Item[usInHand].fFlags & ITEM_TWO_HANDED)) {
    // SMGs are treated as pistols for these purpose except there is a -5
    // penalty;
    if (Weapon[usInHand].ubWeaponClass == SMGCLASS) {
      iChance -= AIM_PENALTY_SMG;  // TODO0007
    }

    /*
    if (pSoldier->inv[SECONDHANDPOS].usItem == NOTHING)
    {
            // firing with pistol in right hand, and second hand empty.
            iChance += AIM_BONUS_TWO_HANDED_PISTOL;
    }
    else */
    if (!HAS_SKILL_TRAIT(pSoldier, AMBIDEXT)) {
      if (IsValidSecondHandShot(pSoldier)) {
        // penalty to aim when firing two pistols
        iChance -= AIM_PENALTY_DUAL_PISTOLS;
      }
      /*
      else
      {
              // penalty to aim with pistol being fired one-handed
              iChance -= AIM_PENALTY_ONE_HANDED_PISTOL;
      }
      */
    }
  }

  // If in burst mode, deduct points for change to hit for each shot after the
  // first
  if (pSoldier->bDoBurst) {
    iPenalty = Weapon[usInHand].ubBurstPenalty * (pSoldier->bDoBurst - 1);

    // halve the penalty for people with the autofire trait
    uint32_t AutoWeaponsSkill = NUM_SKILL_TRAITS(pSoldier, AUTO_WEAPS);
    if (AutoWeaponsSkill != 0) {
      iPenalty /= 2 * AutoWeaponsSkill;
    }
    iChance -= iPenalty;
  }

  sDistVis = DistanceVisible(pSoldier, DIRECTION_IRRELEVANT, DIRECTION_IRRELEVANT, sGridNo, 0);

  // give some leeway to allow people to spot for each other...
  // use distance limitation for LOS routine of 2 x maximum distance EVER
  // visible, so that we get accurate calculations out to around 50 tiles.
  // Because we multiply max distance by 2, we must divide by 2 later

  // CJC August 13 2002:  Wow, this has been wrong the whole time.
  // bTargetCubeLevel seems to be generally set to 2 - but if a character is
  // shooting at an enemy in a particular spot, then we should be using the
  // target position on the body.

  // CJC August 13, 2002
  // If the start soldier has a body part they are aiming at, and know about the
  // person in the tile, then use that height instead
  iSightRange = -1;

  // best to use team knowledge as well, in case of spotting for someone else
  const SOLDIERTYPE *const tgt = WhoIsThere2(sGridNo, pSoldier->bTargetLevel);
  if (tgt != NULL && (pSoldier->bOppList[tgt->ubID] == SEEN_CURRENTLY ||
                      gbPublicOpplist[pSoldier->bTeam][tgt->ubID] == SEEN_CURRENTLY)) {
    iSightRange = SoldierToBodyPartLineOfSightTest(pSoldier, sGridNo, pSoldier->bTargetLevel,
                                                   pSoldier->bAimShotLocation,
                                                   (uint8_t)(MaxDistanceVisible() * 2), TRUE);
  }

  if (iSightRange == -1)  // didn't do a bodypart-based test
  {
    iSightRange = SoldierTo3DLocationLineOfSightTest(pSoldier, sGridNo, pSoldier->bTargetLevel,
                                                     pSoldier->bTargetCubeLevel,
                                                     (uint8_t)(MaxDistanceVisible() * 2), TRUE);
  }

  iSightRange *= 2;

  if (iSightRange > (sDistVis * CELL_X_SIZE)) {
    // shooting beyond max normal vision... penalize such distance at double
    // (also later we halve the remaining chance)
    iSightRange += (iSightRange - sDistVis * CELL_X_SIZE);
  }

  // if shooter spent some extra time aiming and can see the target
  if (iSightRange > 0 && ubAimTime && !pSoldier->bDoBurst)
    iChance += (AIM_BONUS_PER_AP * ubAimTime);  // bonus for every pt of aiming

  if (!(pSoldier->uiStatusFlags & SOLDIER_PC))  // if this is a computer AI controlled enemy
  {
    if (gGameOptions.ubDifficultyLevel == DIF_LEVEL_EASY) {
      // On easy, penalize all enemies by 5%
      iChance -= 5;
    } else {
      // max with 0 to prevent this being a bonus, for JA2 it's just a penalty
      // to make early enemies easy CJC note: IDIOT!  This should have been a
      // min.  It's kind of too late now... CJC 2002-05-17: changed the max to a
      // min to make this work.
      iChance +=
          std::min(0, (int32_t)gbDiff[DIFF_ENEMY_TO_HIT_MOD][SoldierDifficultyLevel(pSoldier)]);
    }
  }

  // if shooter is being affected by gas
  if (pSoldier->uiStatusFlags & SOLDIER_GASSED) {
    iChance -= AIM_PENALTY_GASSED;
  }

  // if shooter is being bandaged at the same time, his concentration is off
  if (pSoldier->ubServiceCount > 0) iChance -= AIM_PENALTY_GETTINGAID;

  // if shooter is still in shock
  if (pSoldier->bShock) iChance -= (pSoldier->bShock * AIM_PENALTY_PER_SHOCK);

  if (Item[usInHand].usItemClass == IC_GUN) {
    bAttachPos = FindAttachment(pInHand, GUN_BARREL_EXTENDER);
    if (bAttachPos != ITEM_NOT_FOUND) {
      // reduce status and see if it falls off
      pInHand->bAttachStatus[bAttachPos] -= (int8_t)Random(2);

      if (pInHand->bAttachStatus[bAttachPos] - Random(35) - Random(35) < USABLE) {
        // barrel extender falls off!
        OBJECTTYPE Temp;

        // since barrel extenders are not removable we cannot call
        // RemoveAttachment here and must create the item by hand
        CreateItem(GUN_BARREL_EXTENDER, pInHand->bAttachStatus[bAttachPos], &Temp);
        pInHand->usAttachItem[bAttachPos] = NOTHING;
        pInHand->bAttachStatus[bAttachPos] = 0;

        // drop it to ground
        AddItemToPool(pSoldier->sGridNo, &Temp, VISIBLE, pSoldier->bLevel, 0, -1);

        // big penalty to hit
        iChance -= 30;

        // curse!
        if (pSoldier->bTeam == OUR_TEAM) {
          DoMercBattleSound(pSoldier, BATTLE_SOUND_CURSE1);

          ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, gzLateLocalizedString[STR_LATE_46],
                    pSoldier->name);
        }
      }
    }

    iMaxRange = GunRange(*pInHand);
  } else {
    iMaxRange = CELL_X_SIZE;  // one tile
  }

  if (iSightRange > 0) {
    if (IsWearingHeadGear(*pSoldier, SUNGOGGLES)) {
      // decrease effective range by 10% when using sungoggles (w or w/o scope)
      iSightRange -= iRange / 10;  // basically, +1% to hit per every 2 squares
    }

    bAttachPos = FindAttachment(pInHand, SNIPERSCOPE);

    // does gun have scope, long range recommends its use, and shooter's aiming?
    if (bAttachPos != NO_SLOT && (iRange > MIN_SCOPE_RANGE) && (ubAimTime > 0)) {
      // reduce effective sight range by 20% per extra aiming time AP of the
      // distance beyond MIN_SCOPE_RANGE.  Max reduction is 80% of the range
      // beyond.
      iScopeBonus = ((SNIPERSCOPE_AIM_BONUS * ubAimTime) * (iRange - MIN_SCOPE_RANGE)) / 100;

      // adjust for scope condition, only has full affect at 100%
      iScopeBonus = (iScopeBonus * WEAPON_STATUS_MOD(pInHand->bAttachStatus[bAttachPos])) / 100;

      // reduce effective range by the bonus obtained from the scope
      iSightRange -= iScopeBonus;
      if (iSightRange < 1) {
        iSightRange = 1;
      }
    }

    bAttachPos = FindAttachment(pInHand, LASERSCOPE);
    if (usInHand == ROCKET_RIFLE || usInHand == AUTO_ROCKET_RIFLE ||
        bAttachPos != NO_SLOT)  // rocket rifle has one built in
    {
      int8_t bLaserStatus;

      if (usInHand == ROCKET_RIFLE || usInHand == AUTO_ROCKET_RIFLE) {
        bLaserStatus = WEAPON_STATUS_MOD(pInHand->bGunStatus);
      } else {
        bLaserStatus = WEAPON_STATUS_MOD(pInHand->bAttachStatus[bAttachPos]);
      }

      // laser scope isn't of much use in high light levels; add something for
      // that
      if (bLaserStatus > 50) {
        iScopeBonus = LASERSCOPE_BONUS * (bLaserStatus - 50) / 50;
      } else {
        // laser scope in bad condition creates aim penalty!
        iScopeBonus = -LASERSCOPE_BONUS * (50 - bLaserStatus) / 50;
      }

      iChance += iScopeBonus;
    }
  }

  // if aiming at the head, reduce chance to hit
  if (ubAimPos == AIM_SHOT_HEAD) {
    // penalty of 3% per tile
    iPenalty = 3 * iSightRange / 10;
    iChance -= iPenalty;
  } else if (ubAimPos == AIM_SHOT_LEGS) {
    // penalty of 1% per tile
    iPenalty = iSightRange / 10;
    iChance -= iPenalty;
  }

  // NumMessage("EFFECTIVE RANGE = ",range);

  // ADJUST FOR RANGE
  // bonus if range is less than normal range, penalty if it's more
  // iChance += (NORMAL_RANGE - iRange) / (CELL_X_SIZE / 5);	// 5% per tile

  // Effects of actual gun max range... the numbers are based on wanting -40%
  // at range 26for a pistol with range 13, and -0 for a sniper rifle with range
  // 80
  iPenalty = ((iMaxRange - iRange * 3) * 10) / (17 * CELL_X_SIZE);
  if (iPenalty < 0) {
    iChance += iPenalty;
  }
  // iChance -= 20 * iRange / iMaxRange;

  if (TANK(pSoldier) && (iRange / CELL_X_SIZE < MaxDistanceVisible())) {
    // tank; penalize at close range!
    // 2 percent per tile closer than max visible distance
    iChance -= 2 * (MaxDistanceVisible() - (iRange / CELL_X_SIZE));
  }

  if (iSightRange == 0) {
    // firing blind!
    iChance -= AIM_PENALTY_BLIND;
  } else {
    // Effects based on aiming & sight
    // From for JA2.5:  3% bonus/penalty for each tile different from range
    // NORMAL_RANGE. This doesn't provide a bigger bonus at close range, but
    // stretches it out, making medium range less penalized, and longer range
    // more penalized
    iChance += 3 * (NORMAL_RANGE - iSightRange) / CELL_X_SIZE;
    /*
    if (iSightRange < NORMAL_RANGE)
    {
            // bonus to hit of 20% at point blank (would be 25% at range 0);
            //at NORMAL_RANGE, bonus is 0
            iChance += 25 * (NORMAL_RANGE - iSightRange) / NORMAL_RANGE;
    }
    else
    {
            // penalty of 2% / tile
            iChance -= (iSightRange - NORMAL_RANGE) / 5;
    }
    */
  }

  // adjust for roof/not on roof
  if (pSoldier->bLevel == 0) {
    if (pSoldier->bTargetLevel > 0) {
      // penalty for firing up
      iChance -= AIM_PENALTY_FIRING_UP;
    }
  } else  // pSoldier->bLevel > 0 )
  {
    if (pSoldier->bTargetLevel == 0) {
      iChance += AIM_BONUS_FIRING_DOWN;
    }
    // if have roof trait, give bonus
    iChance += gbSkillTraitBonus[ONROOF] * NUM_SKILL_TRAITS(pSoldier, ONROOF);
  }

  const SOLDIERTYPE *const pTarget = WhoIsThere2(sGridNo, pSoldier->bTargetLevel);
  if (pTarget != NULL) {
    // targeting a merc
    // adjust for crouched/prone target
    switch (gAnimControl[pTarget->usAnimState].ubHeight) {
      case ANIM_CROUCH:
        if (TANK(pSoldier) && iRange < MIN_TANK_RANGE) {
          // 13% penalty per tile closer than min range
          iChance -= 13 * ((MIN_TANK_RANGE - iRange) / CELL_X_SIZE);
        } else {
          // at anything other than point-blank range
          if (iRange > POINT_BLANK_RANGE + 10 * (AIM_PENALTY_TARGET_CROUCHED / 3)) {
            iChance -= AIM_PENALTY_TARGET_CROUCHED;
          } else if (iRange > POINT_BLANK_RANGE) {
            // at close range give same bonus as prone, up to maximum of
            // AIM_PENALTY_TARGET_CROUCHED
            iChance -= 3 * ((iRange - POINT_BLANK_RANGE) / CELL_X_SIZE);  // penalty -3%/tile
          }
        }
        break;
      case ANIM_PRONE:
        if (TANK(pSoldier) && iRange < MIN_TANK_RANGE) {
          // 25% penalty per tile closer than min range
          iChance -= 25 * ((MIN_TANK_RANGE - iRange) / CELL_X_SIZE);
        } else {
          // at anything other than point-blank range
          if (iRange > POINT_BLANK_RANGE) {
            // reduce chance to hit with distance to the prone/immersed target
            iPenalty = 3 * ((iRange - POINT_BLANK_RANGE) / CELL_X_SIZE);  // penalty -3%/tile
            iPenalty = std::min(iPenalty, AIM_PENALTY_TARGET_PRONE);

            iChance -= iPenalty;
          }
        }
        break;
      case ANIM_STAND:
        // if we are prone and at close range, then penalize shots to the torso or
        // head!
        if (iRange <= MIN_PRONE_RANGE &&
            gAnimControl[pSoldier->usAnimState].ubEndHeight == ANIM_PRONE) {
          if (ubAimPos == AIM_SHOT_RANDOM || ubAimPos == AIM_SHOT_GLAND) {
            ubAdjAimPos = AIM_SHOT_TORSO;
          } else {
            ubAdjAimPos = ubAimPos;
          }
          // lose 10% per height difference, lessened by distance
          // e.g. 30% to aim at head at range 1, only 10% at range 3
          // or 20% to aim at torso at range 1, no penalty at range 3
          // NB torso aim position is 2, so (5-aimpos) is 3, for legs it's 2, for
          // head 4
          iChance -= (5 - ubAdjAimPos - iRange / CELL_X_SIZE) * 10;
        }
        break;
      default:
        break;
    }

    // penalty for amount that enemy has moved
    iPenalty = std::min(((pTarget->bTilesMoved * 3) / 2), 30);
    iChance -= iPenalty;

    // if target sees us, he may have a chance to dodge before the gun goes off
    // but ability to dodge is reduced if crouched or prone!
    if (pTarget->bOppList[pSoldier->ubID] == SEEN_CURRENTLY && !TANK(pTarget) &&
        !(pSoldier->ubBodyType != QUEENMONSTER)) {
      iPenalty = (EffectiveAgility(pTarget) / 5 + EffectiveExpLevel(pTarget) * 2);
      switch (gAnimControl[pTarget->usAnimState].ubHeight) {
        case ANIM_CROUCH:
          iPenalty = iPenalty * 2 / 3;
          break;
        case ANIM_PRONE:
          iPenalty /= 3;
          break;
      }

      // reduce dodge ability by the attacker's stats
      iBonus = (EffectiveDexterity(pSoldier) / 5 + EffectiveExpLevel(pSoldier) * 2);
      if (TANK(pTarget) || (pSoldier->ubBodyType != QUEENMONSTER)) {
        // reduce ability to track shots
        iBonus = iBonus / 2;
      }

      if (iPenalty > iBonus) {
        iChance -= (iPenalty - iBonus);
      }
    }
  } else if (TANK(pSoldier) && iRange < MIN_TANK_RANGE) {
    // 25% penalty per tile closer than min range
    iChance -= 25 * ((MIN_TANK_RANGE - iRange) / CELL_X_SIZE);
  }

  // IF CHANCE EXISTS, BUT SHOOTER IS INJURED
  if ((iChance > 0) && (pSoldier->bLife < pSoldier->bLifeMax)) {
    // if bandaged, give 1/2 of the bandaged life points back into equation
    bBandaged = pSoldier->bLifeMax - pSoldier->bLife - pSoldier->bBleeding;

    // injury penalty is based on % damage taken (max 2/3rds chance)
    iPenalty = (iChance * 2 * (pSoldier->bLifeMax - pSoldier->bLife + (bBandaged / 2))) /
               (3 * pSoldier->bLifeMax);

    // reduce injury penalty due to merc's experience level (he can take it!)
    iChance -= (iPenalty * (100 - (10 * (EffectiveExpLevel(pSoldier) - 1)))) / 100;
  }

  // IF CHANCE EXISTS, BUT SHOOTER IS LOW ON BREATH
  if ((iChance > 0) && (pSoldier->bBreath < 100)) {
    // breath penalty is based on % breath missing (max 1/2 chance)
    iPenalty = (iChance * (100 - pSoldier->bBreath)) / 200;
    // reduce breath penalty due to merc's dexterity (he can compensate!)
    iChance -= (iPenalty * (100 - (EffectiveDexterity(pSoldier) - 10))) / 100;
  }

  // CHECK IF TARGET IS WITHIN GUN'S EFFECTIVE MAXIMUM RANGE
  if (iRange > iMaxRange) {
    // a bullet WILL travel that far if not blocked, but it's NOT accurate,
    // because beyond maximum range, the bullet drops rapidly

    // This won't cause the bullet to be off to the left or right, only make it
    // drop in flight.
    iChance /= 2;
  }
  if (iSightRange > (sDistVis * CELL_X_SIZE)) {
    // penalize out of sight shots, cumulative to effective range penalty
    iChance /= 2;
  }

  // MAKE SURE CHANCE TO HIT IS WITHIN DEFINED LIMITS
  if (iChance < MINCHANCETOHIT) {
    if (TANK(pSoldier)) {
      // allow absolute minimums
      iChance = 0;
    } else {
      iChance = MINCHANCETOHIT;
    }
  } else {
    if (iChance > MAXCHANCETOHIT) iChance = MAXCHANCETOHIT;
  }

  return (iChance);
}

uint32_t AICalcChanceToHitGun(SOLDIERTYPE *pSoldier, uint16_t sGridNo, uint8_t ubAimTime,
                              uint8_t ubAimPos) {
  uint16_t usTrueState;
  uint32_t uiChance;

  // same as CCTHG but fakes the attacker always standing
  usTrueState = pSoldier->usAnimState;
  pSoldier->usAnimState = STANDING;
  uiChance = CalcChanceToHitGun(pSoldier, sGridNo, ubAimTime, ubAimPos);
  pSoldier->usAnimState = usTrueState;
  return (uiChance);
}

int32_t CalcBodyImpactReduction(uint8_t ubAmmoType, uint8_t ubHitLocation) {
  // calculate how much bullets are slowed by passing through someone
  int32_t iReduction = BodyImpactReduction[ubHitLocation];

  switch (ubAmmoType) {
    case AMMO_HP:
      iReduction = AMMO_ARMOUR_ADJUSTMENT_HP(iReduction);
      break;
    case AMMO_AP:
    case AMMO_HEAT:
      iReduction = AMMO_ARMOUR_ADJUSTMENT_AP(iReduction);
      break;
    case AMMO_SUPER_AP:
      iReduction = AMMO_ARMOUR_ADJUSTMENT_SAP(iReduction);
      break;
    default:
      break;
  }
  return (iReduction);
}

static int32_t ArmourProtection(SOLDIERTYPE const &pTarget, uint8_t const ubArmourType,
                                int8_t *const pbStatus, int32_t const iImpact,
                                uint8_t const ubAmmoType) {
  int32_t iProtection, iAppliedProtection, iFailure;

  iProtection = Armour[ubArmourType].ubProtection;

  if (!AM_A_ROBOT(&pTarget)) {
    // check for the bullet hitting a weak spot in the armour
    iFailure = PreRandom(100) + 1 - *pbStatus;
    if (iFailure > 0) {
      iProtection -= iFailure;
      if (iProtection < 0) {
        return (0);
      }
    }
  }

  // adjust protection of armour due to different ammo types
  switch (ubAmmoType) {
    case AMMO_HP:
      iProtection = AMMO_ARMOUR_ADJUSTMENT_HP(iProtection);
      break;
    case AMMO_AP:
    case AMMO_HEAT:
      iProtection = AMMO_ARMOUR_ADJUSTMENT_AP(iProtection);
      break;
    case AMMO_SUPER_AP:
      iProtection = AMMO_ARMOUR_ADJUSTMENT_SAP(iProtection);
      break;
    default:
      break;
  }

  // figure out how much of the armour's protection value is necessary
  // in defending against this bullet
  if (iProtection > iImpact) {
    iAppliedProtection = iImpact;
  } else {
    // applied protection is the full strength of the armour, before AP/HP
    // changes
    iAppliedProtection = Armour[ubArmourType].ubProtection;
  }

  // reduce armour condition

  if (ubAmmoType == AMMO_KNIFE || ubAmmoType == AMMO_SLEEP_DART) {
    // knives and darts damage armour but are not stopped by kevlar
    if (Armour[ubArmourType].ubArmourClass == ARMOURCLASS_VEST ||
        Armour[ubArmourType].ubArmourClass == ARMOURCLASS_LEGGINGS) {
      iProtection = 0;
    }
  } else if (ubAmmoType == AMMO_MONSTER) {
    // creature spit damages armour a lot! an extra 3x for a total of 4x normal
    *pbStatus -= 3 * (iAppliedProtection * Armour[ubArmourType].ubDegradePercent) / 100;

    // reduce amount of protection from armour
    iProtection /= 2;
  }

  if (!AM_A_ROBOT(&pTarget)) {
    *pbStatus -= (iAppliedProtection * Armour[ubArmourType].ubDegradePercent) / 100;
  }

  // return armour protection
  return (iProtection);
}

int32_t TotalArmourProtection(SOLDIERTYPE &pTarget, const uint8_t ubHitLocation,
                              const int32_t iImpact, const uint8_t ubAmmoType) {
  int32_t iTotalProtection = 0, iSlot;
  OBJECTTYPE *pArmour;
  int8_t bPlatePos = -1;

  if (pTarget.uiStatusFlags & SOLDIER_VEHICLE) {
    int8_t bDummyStatus = 100;
    iTotalProtection += ArmourProtection(pTarget, GetVehicleArmourType(pTarget.bVehicleID),
                                         &bDummyStatus, iImpact, ubAmmoType);
  } else {
    switch (ubHitLocation) {
      case AIM_SHOT_GLAND:
        // creature hit in the glands!!! no armour there!
        return (0);
      case AIM_SHOT_HEAD:
        iSlot = HELMETPOS;
        break;
      case AIM_SHOT_LEGS:
        iSlot = LEGPOS;
        break;
      case AIM_SHOT_TORSO:
      default:
        iSlot = VESTPOS;
        break;
    }

    pArmour = &pTarget.inv[iSlot];
    if (pArmour->usItem != NOTHING) {
      // check plates first
      if (iSlot == VESTPOS) {
        bPlatePos = FindAttachment(pArmour, CERAMIC_PLATES);
        if (bPlatePos != -1) {
          // bullet got through jacket; apply ceramic plate armour
          iTotalProtection +=
              ArmourProtection(pTarget, Item[pArmour->usAttachItem[bPlatePos]].ubClassIndex,
                               &(pArmour->bAttachStatus[bPlatePos]), iImpact, ubAmmoType);
          if (pArmour->bAttachStatus[bPlatePos] < USABLE) {
            // destroy plates!
            pArmour->usAttachItem[bPlatePos] = NOTHING;
            pArmour->bAttachStatus[bPlatePos] = 0;
            DirtyMercPanelInterface(&pTarget, DIRTYLEVEL2);
            if (pTarget.bTeam == OUR_TEAM) {
              // report plates destroyed!
              ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, str_ceramic_plates_smashed,
                        pTarget.name);
            }
          }
        }
      }

      // if the plate didn't stop the bullet...
      if (iImpact > iTotalProtection) {
        iTotalProtection += ArmourProtection(pTarget, Item[pArmour->usItem].ubClassIndex,
                                             &(pArmour->bStatus[0]), iImpact, ubAmmoType);
        if (pArmour->bStatus[0] < USABLE) {
          DeleteObj(pArmour);
          DirtyMercPanelInterface(&pTarget, DIRTYLEVEL2);
        }
      }
    }
  }
  return (iTotalProtection);
}

int32_t BulletImpact(SOLDIERTYPE *pFirer, SOLDIERTYPE *pTarget, uint8_t ubHitLocation,
                     int32_t iOrigImpact, int16_t sHitBy, uint8_t *pubSpecial) {
  int32_t iImpact, iFluke, iBonus, iImpactForCrits = 0;
  int8_t bStatLoss;
  uint8_t ubAmmoType;

  // NOTE: reduction of bullet impact due to range and obstacles is handled
  // in MoveBullet.

  // Set a few things up:
  if (Item[pFirer->usAttackingWeapon].usItemClass == IC_THROWING_KNIFE) {
    ubAmmoType = AMMO_KNIFE;
  } else {
    ubAmmoType = pFirer->inv[pFirer->ubAttackingHand].ubGunAmmoType;
  }

  if (TANK(pTarget)) {
    if (ubAmmoType != AMMO_HEAT) {
      // ping!
      return (0);
    }
  }

  // plus/minus up to 25% due to "random" factors (major organs hit or missed,
  // lucky lighter in breast pocket, divine intervention on behalf of "Rev"...)
  iFluke = PreRandom(51) - 25;  // gives (0 to 50 -25) -> -25% to +25%
  // NumMessage("Fluke = ",fluke);

  // up to 50% extra impact for making particularly accurate successful shots
  iBonus = sHitBy / 2;
  // NumMessage("Bonus = ",bonus);

  iOrigImpact = iOrigImpact * (100 + iFluke + iBonus) / 100;

  // at very long ranges (1.5x maxRange and beyond) impact could go negative
  if (iOrigImpact < 1) {
    iOrigImpact = 1;  // raise impact to a minimum of 1 for any hit
  }

  // adjust for HE rounds
  if (ubAmmoType == AMMO_HE || ubAmmoType == AMMO_HEAT) {
    iOrigImpact = AMMO_DAMAGE_ADJUSTMENT_HE(iOrigImpact);

    if (TANK(pTarget)) {
      // HEAT round on tank, divide by 3 for damage
      iOrigImpact /= 2;
    }
  }

  if (pubSpecial && *pubSpecial == FIRE_WEAPON_BLINDED_BY_SPIT_SPECIAL) {
    iImpact = iOrigImpact;
  } else {
    iImpact = iOrigImpact - TotalArmourProtection(*pTarget, ubHitLocation, iOrigImpact, ubAmmoType);
  }

  // calc minimum damage
  if (ubAmmoType == AMMO_HP || ubAmmoType == AMMO_SLEEP_DART) {
    if (iImpact < 0) {
      iImpact = 0;
    }
  } else {
    if (iImpact < ((iOrigImpact + 5) / 10)) {
      iImpact = (iOrigImpact + 5) / 10;
    }

    if ((ubAmmoType == AMMO_BUCKSHOT) && (pTarget->bNumPelletsHitBy > 0)) {
      iImpact += (pTarget->bNumPelletsHitBy - 1) / 2;
    }
  }

  if (gfNextShotKills) {
    // big time cheat key effect!
    iImpact = 100;
    gfNextShotKills = FALSE;
  }

  if (iImpact > 0 && !TANK(pTarget)) {
    if (ubAmmoType == AMMO_SLEEP_DART && sHitBy > 20) {
      if (pubSpecial) {
        *pubSpecial = FIRE_WEAPON_SLEEP_DART_SPECIAL;
      }
      return (iImpact);
    }

    if (ubAmmoType == AMMO_HP) {  // good solid hit with a hollow-point bullet,
                                  // which got through armour!
      iImpact = AMMO_DAMAGE_ADJUSTMENT_HP(iImpact);
    }

    AdjustImpactByHitLocation(iImpact, ubHitLocation, &iImpact, &iImpactForCrits);

    switch (ubHitLocation) {
      case AIM_SHOT_HEAD:
        // is the blow deadly enough for an instant kill?
        if (PythSpacesAway(pFirer->sGridNo, pTarget->sGridNo) <= MAX_DISTANCE_FOR_MESSY_DEATH) {
          if (iImpactForCrits > MIN_DAMAGE_FOR_INSTANT_KILL && iImpactForCrits < pTarget->bLife) {
            // blow to the head is so deadly that it causes instant death;
            // the target has more life than iImpact so we increase it
            iImpact = pTarget->bLife + Random(10);
            iImpactForCrits = iImpact;
          }

          if (pubSpecial) {
            // is the blow deadly enough to cause a head explosion?
            if (iImpactForCrits >= pTarget->bLife) {
              if (iImpactForCrits > MIN_DAMAGE_FOR_HEAD_EXPLOSION) {
                *pubSpecial = FIRE_WEAPON_HEAD_EXPLODE_SPECIAL;
              } else if (iImpactForCrits > (MIN_DAMAGE_FOR_HEAD_EXPLOSION / 2) &&
                         (PreRandom(MIN_DAMAGE_FOR_HEAD_EXPLOSION / 2) <
                          (uint32_t)(iImpactForCrits - MIN_DAMAGE_FOR_HEAD_EXPLOSION / 2))) {
                *pubSpecial = FIRE_WEAPON_HEAD_EXPLODE_SPECIAL;
              }
            }
          }
        }
        break;
      case AIM_SHOT_LEGS:
        // is the damage enough to make us fall over?
        if (pubSpecial && IS_MERC_BODY_TYPE(pTarget) &&
            gAnimControl[pTarget->usAnimState].ubEndHeight == ANIM_STAND && !MercInWater(pTarget)) {
          if (iImpactForCrits > MIN_DAMAGE_FOR_AUTO_FALL_OVER) {
            *pubSpecial = FIRE_WEAPON_LEG_FALLDOWN_SPECIAL;
          }
          // else ramping up chance from 1/2 the automatic value onwards
          else if (iImpactForCrits > (MIN_DAMAGE_FOR_AUTO_FALL_OVER / 2) &&
                   (PreRandom(MIN_DAMAGE_FOR_AUTO_FALL_OVER / 2) <
                    (uint32_t)(iImpactForCrits - MIN_DAMAGE_FOR_AUTO_FALL_OVER / 2))) {
            *pubSpecial = FIRE_WEAPON_LEG_FALLDOWN_SPECIAL;
          }
        }
        break;
      case AIM_SHOT_TORSO:
        // normal damage to torso
        // is the blow deadly enough for an instant kill?
        // since this value is much lower than the others, it only applies at
        // short range...
        if (PythSpacesAway(pFirer->sGridNo, pTarget->sGridNo) <= MAX_DISTANCE_FOR_MESSY_DEATH) {
          if (iImpact > MIN_DAMAGE_FOR_INSTANT_KILL && iImpact < pTarget->bLife) {
            // blow to the chest is so deadly that it causes instant death;
            // the target has more life than iImpact so we increase it
            iImpact = pTarget->bLife + Random(10);
            iImpactForCrits = iImpact;
          }
          // special thing for hitting chest - allow cumulative damage to count
          else if ((iImpact + pTarget->sDamage) >
                   (MIN_DAMAGE_FOR_BLOWN_AWAY + MIN_DAMAGE_FOR_INSTANT_KILL)) {
            iImpact = pTarget->bLife + Random(10);
            iImpactForCrits = iImpact;
          }

          // is the blow deadly enough to cause a chest explosion?
          if (pubSpecial) {
            if (iImpact > MIN_DAMAGE_FOR_BLOWN_AWAY && iImpact >= pTarget->bLife) {
              *pubSpecial = FIRE_WEAPON_CHEST_EXPLODE_SPECIAL;
            }
          }
        }
        break;
    }
  }

  if (AM_A_ROBOT(pTarget)) {
    iImpactForCrits = 0;
  }

  // don't do critical hits against people who are gonna die!
  if (!IsAutoResolveActive()) {
    if (ubAmmoType == AMMO_KNIFE && pFirer->bOppList[pTarget->ubID] == SEEN_CURRENTLY) {
      // is this a stealth attack?
      if (pTarget->bOppList[pFirer->ubID] == NOT_HEARD_OR_SEEN && !CREATURE_OR_BLOODCAT(pTarget) &&
          (ubHitLocation == AIM_SHOT_HEAD || ubHitLocation == AIM_SHOT_TORSO)) {
        if (PreRandom(100) < (uint32_t)(sHitBy + 10 * NUM_SKILL_TRAITS(pFirer, THROWING))) {
          // instant death!
          iImpact = pTarget->bLife + Random(10);
          iImpactForCrits = iImpact;
        }
      }
    }

    if (iImpactForCrits > 0 && iImpactForCrits < pTarget->bLife) {
      if (PreRandom(iImpactForCrits / 2 + pFirer->bAimTime * 5) + 1 > CRITICAL_HIT_THRESHOLD) {
        bStatLoss = (int8_t)PreRandom(iImpactForCrits / 2) + 1;
        switch (ubHitLocation) {
          case AIM_SHOT_HEAD:
            if (bStatLoss >= pTarget->bWisdom) {
              bStatLoss = pTarget->bWisdom - 1;
            }
            if (bStatLoss > 0) {
              pTarget->bWisdom -= bStatLoss;

              if (pTarget->ubProfile != NO_PROFILE) {
                gMercProfiles[pTarget->ubProfile].bWisdom = pTarget->bWisdom;
              }

              if (pTarget->name[0] && pTarget->bVisible == TRUE) {
                // make stat RED for a while...
                pTarget->uiChangeWisdomTime = GetJA2Clock();
                pTarget->usValueGoneUp &= ~(WIS_INCREASE);

                if (bStatLoss == 1) {
                  ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE,
                            g_langRes->Message[STR_LOSES_1_WISDOM], pTarget->name);
                } else {
                  ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE,
                            g_langRes->Message[STR_LOSES_WISDOM], pTarget->name, bStatLoss);
                }
              }
            } else if (pTarget->bNumPelletsHitBy == 0) {
              ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, g_langRes->Message[STR_HEAD_HIT],
                        pTarget->name);
            }
            break;
          case AIM_SHOT_TORSO:
            if (PreRandom(1) == 0 && !(pTarget->uiStatusFlags & SOLDIER_MONSTER)) {
              if (bStatLoss >= pTarget->bDexterity) {
                bStatLoss = pTarget->bDexterity - 1;
              }
              if (bStatLoss > 0) {
                pTarget->bDexterity -= bStatLoss;

                if (pTarget->ubProfile != NO_PROFILE) {
                  gMercProfiles[pTarget->ubProfile].bDexterity = pTarget->bDexterity;
                }

                if (pTarget->name[0] && pTarget->bVisible == TRUE) {
                  // make stat RED for a while...
                  pTarget->uiChangeDexterityTime = GetJA2Clock();
                  pTarget->usValueGoneUp &= ~(DEX_INCREASE);

                  if (bStatLoss == 1) {
                    ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE,
                              g_langRes->Message[STR_LOSES_1_DEX], pTarget->name);
                  } else {
                    ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE,
                              g_langRes->Message[STR_LOSES_DEX], pTarget->name, bStatLoss);
                  }
                }
              }
            } else {
              if (bStatLoss >= pTarget->bStrength) {
                bStatLoss = pTarget->bStrength - 1;
              }
              if (bStatLoss > 0) {
                pTarget->bStrength -= bStatLoss;

                if (pTarget->ubProfile != NO_PROFILE) {
                  gMercProfiles[pTarget->ubProfile].bStrength = pTarget->bStrength;
                }

                if (pTarget->name[0] && pTarget->bVisible == TRUE) {
                  // make stat RED for a while...
                  pTarget->uiChangeStrengthTime = GetJA2Clock();
                  pTarget->usValueGoneUp &= ~(STRENGTH_INCREASE);

                  if (bStatLoss == 1) {
                    ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE,
                              g_langRes->Message[STR_LOSES_1_STRENGTH], pTarget->name);
                  } else {
                    ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE,
                              g_langRes->Message[STR_LOSES_STRENGTH], pTarget->name, bStatLoss);
                  }
                }
              }
            }
            break;
          case AIM_SHOT_LEGS:
            if (bStatLoss >= pTarget->bAgility) {
              bStatLoss = pTarget->bAgility - 1;
            }
            if (bStatLoss > 0) {
              pTarget->bAgility -= bStatLoss;

              if (pTarget->ubProfile != NO_PROFILE) {
                gMercProfiles[pTarget->ubProfile].bAgility = pTarget->bAgility;
              }

              if (pTarget->name[0] && pTarget->bVisible == TRUE) {
                // make stat RED for a while...
                pTarget->uiChangeAgilityTime = GetJA2Clock();
                pTarget->usValueGoneUp &= ~(AGIL_INCREASE);

                if (bStatLoss == 1) {
                  ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE,
                            g_langRes->Message[STR_LOSES_1_AGIL], pTarget->name);
                } else {
                  ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, g_langRes->Message[STR_LOSES_AGIL],
                            pTarget->name, bStatLoss);
                }
              }
            }
            break;
        }
      } else if (ubHitLocation == AIM_SHOT_HEAD && pTarget->bNumPelletsHitBy == 0) {
        ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, g_langRes->Message[STR_HEAD_HIT],
                  pTarget->name);
      }
    }
  }

  return (iImpact);
}

int32_t HTHImpact(const SOLDIERTYPE *const att, const SOLDIERTYPE *const tgt, const int32_t iHitBy,
                  const BOOLEAN fBladeAttack) {
  int32_t impact = EffectiveExpLevel(att) / 2;  // 0 to 4 for level
  const int8_t strength = EffectiveStrength(att);
  const uint16_t weapon = att->usAttackingWeapon;
  if (fBladeAttack) {
    impact += strength / 20;  // 0 to 5 for strength, adjusted by damage taken
    impact += Weapon[weapon].ubImpact;

    if (AM_A_ROBOT(tgt)) impact /= 4;
  } else {
    impact += strength / 5;  // 0 to 20 for strength, adjusted by damage taken

    // NB martial artists don't get a bonus for using brass knuckles!
    if (weapon && !HAS_SKILL_TRAIT(att, MARTIALARTS)) {
      impact += Weapon[weapon].ubImpact;
      if (AM_A_ROBOT(tgt)) impact /= 2;
    } else {
      // base HTH damage
      impact += 5;
      if (AM_A_ROBOT(tgt)) impact = 0;
    }
  }

  const int32_t fluke = PreRandom(51) - 25;  // +/-25% bonus due to random factors
  const int32_t bonus = iHitBy / 2;          // up to 50% extra impact for accurate attacks
  impact = impact * (100 + fluke + bonus) / 100;

  if (!fBladeAttack) {
    // add bonuses for hand-to-hand and martial arts
    if (HAS_SKILL_TRAIT(att, MARTIALARTS)) {
      impact = impact *
               (100 + gbSkillTraitBonus[MARTIALARTS] * NUM_SKILL_TRAITS(att, MARTIALARTS)) / 100;
      if (att->usAnimState == NINJA_SPINKICK) impact *= 2;
    }
    // SPECIAL  - give TRIPLE bonus for damage for hand-to-hand trait
    // because the HTH bonus is half that of martial arts, and gets only 1x for
    // to-hit bonus
    impact = impact *
             (100 + 3 * gbSkillTraitBonus[HANDTOHAND] * NUM_SKILL_TRAITS(att, HANDTOHAND)) / 100;
  }

  return impact;
}

void ShotMiss(const BULLET *const b) {
  SOLDIERTYPE *const pAttacker = b->pFirer;
  SOLDIERTYPE *const opponent = pAttacker->opponent;
  // AGILITY GAIN: Opponent "dodged" a bullet shot at him (it missed)
  if (opponent != NULL) AgilityForEnemyMissingPlayer(pAttacker, opponent, 5);

  switch (Weapon[pAttacker->usAttackingWeapon].ubWeaponClass) {
    case HANDGUNCLASS:
    case RIFLECLASS:
    case SHOTGUNCLASS:
    case SMGCLASS:
    case MGCLASS:
      // Guy has missed, play random sound
      if (pAttacker->bTeam == OUR_TEAM && Random(40) == 0) {
        DoMercBattleSound(pAttacker, BATTLE_SOUND_CURSE1);
      }

      // PLAY SOUND AND FLING DEBRIS
      // RANDOMIZE SOUND SYSTEM
      if (!DoSpecialEffectAmmoMiss(pAttacker, NOWHERE, 0, 0, 0, TRUE, TRUE, NULL)) {
        PlayJA2Sample(SoundRange<MISS_1, MISS_8>(), HIGHVOLUME, 1, MIDDLEPAN);
      }

      // ATE: Show misses...( if our team )
      if (gGameSettings.fOptions[TOPTION_SHOW_MISSES] && pAttacker->bTeam == OUR_TEAM) {
        LocateGridNo(b->sGridNo);
      }
      break;

    case MONSTERCLASS:
      PlayJA2Sample(SPIT_RICOCHET, HIGHVOLUME, 1, MIDDLEPAN);
      break;
  }

  DebugMsg(TOPIC_JA2, DBG_LEVEL_3, "@@@@@@@ Freeing up attacker - bullet missed");
  FreeUpAttacker(pAttacker);
}

static uint32_t CalcChanceHTH(SOLDIERTYPE *pAttacker, SOLDIERTYPE *pDefender, uint8_t ubAimTime,
                              uint8_t ubMode) {
  uint16_t usInHand;
  uint8_t ubBandaged;
  int32_t iAttRating, iDefRating;
  int32_t iChance;

  usInHand = pAttacker->usAttackingWeapon;

  if ((usInHand != CREATURE_QUEEN_TENTACLES) &&
      (pDefender->bLife < OKLIFE || pDefender->bBreath < OKBREATH)) {
    // there is NO way to miss
    return (100);
  }

  if (ubMode == HTH_MODE_STAB) {
    // safety check
    if (Weapon[usInHand].ubWeaponClass != KNIFECLASS) {
#ifdef BETAVERSION
      NumMessage(
          "CalcChanceToStab: ERROR - Attacker isn't holding a knife, "
          "usInHand = ",
          usInHand);
#endif
      return (0);
    }
  } else {
    if (Item[usInHand].usItemClass != IC_PUNCH) {
      return (0);
    }
  }

  // CALCULATE ATTACKER'S CLOSE COMBAT RATING (1-100)
  if (ubMode == HTH_MODE_STEAL) {
    // this is more of a brute force strength-vs-strength check
    iAttRating = (EffectiveDexterity(pAttacker) +        // coordination, accuracy
                  EffectiveAgility(pAttacker) +          // speed & reflexes
                  3 * pAttacker->bStrength +             // physical strength (TRIPLED!)
                  (10 * EffectiveExpLevel(pAttacker)));  // experience, knowledge
  } else {
    iAttRating = (3 * EffectiveDexterity(pAttacker) +    // coordination, accuracy (TRIPLED!)
                  EffectiveAgility(pAttacker) +          // speed & reflexes
                  pAttacker->bStrength +                 // physical strength
                  (10 * EffectiveExpLevel(pAttacker)));  // experience, knowledge
  }

  iAttRating /= 6;  // convert from 6-600 to 1-100

  // psycho bonus
  if (pAttacker->ubProfile != NO_PROFILE &&
      gMercProfiles[pAttacker->ubProfile].bPersonalityTrait == PSYCHO) {
    iAttRating += AIM_BONUS_PSYCHO;
  }

  // modify chance to hit by morale
  iAttRating += GetMoraleModifier(pAttacker);

  // modify for fatigue
  iAttRating -= GetSkillCheckPenaltyForFatigue(pAttacker, iAttRating);

  // if attacker spent some extra time aiming
  if (ubAimTime) {
    // use only HALF of the normal aiming bonus for knife aiming.
    // since there's no range penalty, the bonus is otherwise too generous
    iAttRating += ((AIM_BONUS_PER_AP * ubAimTime) / 2);  // bonus for aiming
  }

  if (!(pAttacker->uiStatusFlags & SOLDIER_PC))  // if attacker is a computer AI controlled enemy
  {
    iAttRating += gbDiff[DIFF_ENEMY_TO_HIT_MOD][SoldierDifficultyLevel(pAttacker)];
  }

  // if attacker is being affected by gas
  if (pAttacker->uiStatusFlags & SOLDIER_GASSED) iAttRating -= AIM_PENALTY_GASSED;

  // if attacker is being bandaged at the same time, his concentration is off
  if (pAttacker->ubServiceCount > 0) iAttRating -= AIM_PENALTY_GETTINGAID;

  // if attacker is still in shock
  if (pAttacker->bShock) iAttRating -= (pAttacker->bShock * AIM_PENALTY_PER_SHOCK);

  // If attacker injured, reduce chance accordingly (by up to 2/3rds)
  if ((iAttRating > 0) && (pAttacker->bLife < pAttacker->bLifeMax)) {
    // if bandaged, give 1/2 of the bandaged life points back into equation
    ubBandaged = pAttacker->bLifeMax - pAttacker->bLife - pAttacker->bBleeding;

    iAttRating -= (2 * iAttRating * (pAttacker->bLifeMax - pAttacker->bLife + (ubBandaged / 2))) /
                  (3 * pAttacker->bLifeMax);
  }

  // If attacker tired, reduce chance accordingly (by up to 1/2)
  if ((iAttRating > 0) && (pAttacker->bBreath < 100))
    iAttRating -= (iAttRating * (100 - pAttacker->bBreath)) / 200;

  if (pAttacker->ubProfile != NO_PROFILE) {
    if (ubMode == HTH_MODE_STAB) {
      iAttRating += gbSkillTraitBonus[KNIFING] * NUM_SKILL_TRAITS(pAttacker, KNIFING);
    } else {
      // add bonuses for hand-to-hand and martial arts
      iAttRating += gbSkillTraitBonus[MARTIALARTS] * NUM_SKILL_TRAITS(pAttacker, MARTIALARTS);
      iAttRating += gbSkillTraitBonus[HANDTOHAND] * NUM_SKILL_TRAITS(pAttacker, HANDTOHAND);
    }
  }

  if (iAttRating < 1) iAttRating = 1;

  // CALCULATE DEFENDER'S CLOSE COMBAT RATING (0-100)
  if (ubMode == HTH_MODE_STEAL) {
    iDefRating = (EffectiveAgility(pDefender)) +       // speed & reflexes
                 EffectiveDexterity(pDefender) +       // coordination, accuracy
                 3 * pDefender->bStrength +            // physical strength (TRIPLED!)
                 (10 * EffectiveExpLevel(pDefender));  // experience, knowledge
  } else {
    iDefRating = (3 * EffectiveAgility(pDefender)) +   // speed & reflexes (TRIPLED!)
                 EffectiveDexterity(pDefender) +       // coordination, accuracy
                 pDefender->bStrength +                // physical strength
                 (10 * EffectiveExpLevel(pDefender));  // experience, knowledge
  }

  iDefRating /= 6;  // convert from 6-600 to 1-100

  // modify chance to dodge by morale
  iDefRating += GetMoraleModifier(pDefender);

  // modify for fatigue
  iDefRating -= GetSkillCheckPenaltyForFatigue(pDefender, iDefRating);

  // if attacker is being affected by gas
  if (pDefender->uiStatusFlags & SOLDIER_GASSED) iDefRating -= AIM_PENALTY_GASSED;

  // if defender is being bandaged at the same time, his concentration is off
  if (pDefender->ubServiceCount > 0) iDefRating -= AIM_PENALTY_GETTINGAID;

  // if defender is still in shock
  if (pDefender->bShock) iDefRating -= (pDefender->bShock * AIM_PENALTY_PER_SHOCK);

  // If defender injured, reduce chance accordingly (by up to 2/3rds)
  if ((iDefRating > 0) && (pDefender->bLife < pDefender->bLifeMax)) {
    // if bandaged, give 1/2 of the bandaged life points back into equation
    ubBandaged = pDefender->bLifeMax - pDefender->bLife - pDefender->bBleeding;

    iDefRating -= (2 * iDefRating * (pDefender->bLifeMax - pDefender->bLife + (ubBandaged / 2))) /
                  (3 * pDefender->bLifeMax);
  }

  // If defender tired, reduce chance accordingly (by up to 1/2)
  if ((iDefRating > 0) && (pDefender->bBreath < 100))
    iDefRating -= (iDefRating * (100 - pDefender->bBreath)) / 200;

  if ((usInHand == CREATURE_QUEEN_TENTACLES && pDefender->ubBodyType == LARVAE_MONSTER) ||
      pDefender->ubBodyType == INFANT_MONSTER) {
    // try to prevent queen from killing the kids, ever!
    iDefRating += 10000;
  }

  if (gAnimControl[pDefender->usAnimState].ubEndHeight < ANIM_STAND) {
    if (usInHand == CREATURE_QUEEN_TENTACLES) {
      if (gAnimControl[pDefender->usAnimState].ubEndHeight == ANIM_PRONE) {
        // make it well-nigh impossible to hit someone who is prone!
        iDefRating += 1000;
      } else {
        iDefRating += BAD_DODGE_POSITION_PENALTY * 2;
      }
    } else {
      // if defender crouched, reduce chance accordingly (harder to dodge)
      iDefRating -= BAD_DODGE_POSITION_PENALTY;
      // If our target is prone, double the penalty!
      if (gAnimControl[pDefender->usAnimState].ubEndHeight == ANIM_PRONE) {
        iDefRating -= BAD_DODGE_POSITION_PENALTY;
      }
    }
  }

  if (pDefender->ubProfile != NO_PROFILE) {
    if (ubMode == HTH_MODE_STAB) {
      if (Item[pDefender->inv[HANDPOS].usItem].usItemClass == IC_BLADE) {
        // good with knives, got one, so we're good at parrying
        iDefRating += gbSkillTraitBonus[KNIFING] * NUM_SKILL_TRAITS(pDefender, KNIFING);
        // the knife gets in the way but we're still better than nobody
        iDefRating += gbSkillTraitBonus[MARTIALARTS] * NUM_SKILL_TRAITS(pDefender, MARTIALARTS) / 3;
      } else {
        // good with knives, don't have one, but we know a bit about dodging
        iDefRating += gbSkillTraitBonus[KNIFING] * NUM_SKILL_TRAITS(pDefender, KNIFING) / 3;
        // bonus for dodging knives
        iDefRating += gbSkillTraitBonus[MARTIALARTS] * NUM_SKILL_TRAITS(pDefender, MARTIALARTS) / 2;
      }
    } else {  // punch/hand-to-hand/martial arts attack/steal
      if (Item[pDefender->inv[HANDPOS].usItem].usItemClass == IC_BLADE &&
          ubMode != HTH_MODE_STEAL) {
        // with our knife, we get some bonus at defending from HTH attacks
        iDefRating += gbSkillTraitBonus[KNIFING] * NUM_SKILL_TRAITS(pDefender, KNIFING) / 2;
      } else {
        iDefRating += gbSkillTraitBonus[MARTIALARTS] * NUM_SKILL_TRAITS(pDefender, MARTIALARTS);
        iDefRating += gbSkillTraitBonus[HANDTOHAND] * NUM_SKILL_TRAITS(pDefender, HANDTOHAND);
      }
    }
  }

  if (iDefRating < 1) iDefRating = 1;

  // NumMessage("CalcChanceToStab - Attacker's Rating = ",iAttRating);
  // NumMessage("CalcChanceToStab - Defender's Rating = ",iDefRating);

  // calculate chance to hit by comparing the 2 opponent's ratings
  //  iChance = (100 * iAttRating) / (iAttRating + iDefRating);

  if (ubMode == HTH_MODE_STEAL) {
    // make this more extreme so that weak people have a harder time stealing
    // from the stronger
    iChance = 50 * iAttRating / iDefRating;
  } else {
    // Changed from DG by CJC to give higher chances of hitting with a stab or
    // punch
    iChance = 67 + (iAttRating - iDefRating) / 3;

    if (pAttacker->bAimShotLocation == AIM_SHOT_HEAD) {
      // make this harder!
      iChance -= 20;
    }
  }

  // MAKE SURE CHANCE TO HIT IS WITHIN DEFINED LIMITS
  if (iChance < MINCHANCETOHIT) {
    iChance = MINCHANCETOHIT;
  } else {
    if (iChance > MAXCHANCETOHIT) iChance = MAXCHANCETOHIT;
  }

  // NumMessage("ChanceToStab = ",chance);

  return (iChance);
}

uint32_t CalcChanceToStab(SOLDIERTYPE *pAttacker, SOLDIERTYPE *pDefender, uint8_t ubAimTime) {
  return (CalcChanceHTH(pAttacker, pDefender, ubAimTime, HTH_MODE_STAB));
}

uint32_t CalcChanceToPunch(SOLDIERTYPE *pAttacker, SOLDIERTYPE *pDefender, uint8_t ubAimTime) {
  return (CalcChanceHTH(pAttacker, pDefender, ubAimTime, HTH_MODE_PUNCH));
}

static uint32_t CalcChanceToSteal(SOLDIERTYPE *pAttacker, SOLDIERTYPE *pDefender,
                                  uint8_t ubAimTime) {
  return (CalcChanceHTH(pAttacker, pDefender, ubAimTime, HTH_MODE_STEAL));
}

void ReloadWeapon(SOLDIERTYPE *const s, uint8_t const inv_pos) {
  // NB this is a cheat function, don't award experience
  OBJECTTYPE &o = s->inv[inv_pos];
  if (o.usItem == NOTHING) return;

  o.ubGunShotsLeft = Weapon[o.usItem].ubMagSize;
  DirtyMercPanelInterface(s, DIRTYLEVEL1);
}

bool IsGunBurstCapable(SOLDIERTYPE const *const s, uint8_t const inv_pos) {
  uint16_t const item = s->inv[inv_pos].usItem;
  return Item[item].usItemClass & IC_WEAPON && Weapon[item].ubShotsPerBurst > 0;
}

int32_t CalcMaxTossRange(const SOLDIERTYPE *pSoldier, uint16_t usItem, BOOLEAN fArmed) {
  int32_t iRange;
  uint16_t usSubItem;

  if (EXPLOSIVE_GUN(usItem)) {
    // oops! return value in weapons table
    return (Weapon[usItem].usRange / CELL_X_SIZE);
  }

  // if item's fired mechanically
  // ATE: If we are sent in a LAUNCHABLE, get the LAUCNHER, and sub ONLY if we
  // are armed...
  usSubItem = GetLauncherFromLaunchable(usItem);

  if (fArmed && usSubItem != NOTHING) {
    usItem = usSubItem;
  }

  if (Item[usItem].usItemClass == IC_LAUNCHER && fArmed) {
    // this function returns range in tiles so, stupidly, we have to divide by
    // 10 here
    iRange = Weapon[usItem].usRange / CELL_X_SIZE;
  } else {
    if (Item[usItem].fFlags & ITEM_UNAERODYNAMIC) {
      iRange = 1;
    } else if (Item[usItem].usItemClass == IC_GRENADE) {
      // start with the range based on the soldier's strength and the item's
      // weight
      int32_t iThrowingStrength = (EffectiveStrength(pSoldier) * 2 + 100) / 3;
      iRange = 2 + (iThrowingStrength / std::min((3 + (Item[usItem].ubWeight) / 3), 4));
    } else {  // not as aerodynamic!

      // start with the range based on the soldier's strength and the item's
      // weight
      iRange = 2 + ((EffectiveStrength(pSoldier) / (5 + Item[usItem].ubWeight)));
    }

    // adjust for thrower's remaining breath (lose up to 1/2 of range)
    iRange -= (iRange * (100 - pSoldier->bBreath)) / 200;

    // better max range due to expertise
    iRange =
        iRange * (100 + gbSkillTraitBonus[THROWING] * NUM_SKILL_TRAITS(pSoldier, THROWING)) / 100;
  }

  if (iRange < 1) {
    iRange = 1;
  }

  return (iRange);
}

uint32_t CalcThrownChanceToHit(SOLDIERTYPE *pSoldier, int16_t sGridNo, uint8_t ubAimTime,
                               uint8_t ubAimPos) {
  int32_t iChance, iMaxRange, iRange;
  uint16_t usHandItem;
  int8_t bPenalty, bBandaged;

  if (pSoldier->bWeaponMode == WM_ATTACHED) {
    usHandItem = UNDER_GLAUNCHER;
  } else {
    usHandItem = pSoldier->inv[HANDPOS].usItem;
  }

  /*
          // CJC: Grenade Launchers don't fire in a straight line!
          #ifdef BETAVERSION
          if (usHandItem == GLAUNCHER)
          {
                  PopMessage("CalcThrownChanceToHit: DOESN'T WORK ON
     GLAUNCHERs!"); return(0);
          }
          #endif
  */

  if (Item[usHandItem].usItemClass != IC_LAUNCHER && pSoldier->bWeaponMode != WM_ATTACHED) {
    // PHYSICALLY THROWN arced projectile (ie. grenade)
    // for lack of anything better, base throwing accuracy on dex & marskmanship
    iChance = (EffectiveDexterity(pSoldier) + EffectiveMarksmanship(pSoldier)) / 2;
    // throwing trait helps out
    iChance += gbSkillTraitBonus[THROWING] * NUM_SKILL_TRAITS(pSoldier, THROWING);
  } else {
    // MECHANICALLY FIRED arced projectile (ie. mortar), need brains & know-how
    iChance = (EffectiveDexterity(pSoldier) + EffectiveMarksmanship(pSoldier) +
               EffectiveWisdom(pSoldier) + pSoldier->bExpLevel) /
              4;

    // heavy weapons trait helps out
    iChance += gbSkillTraitBonus[HEAVY_WEAPS] * NUM_SKILL_TRAITS(pSoldier, HEAVY_WEAPS);
  }

  // modify based on morale
  iChance += GetMoraleModifier(pSoldier);

  // modify by fatigue
  iChance -= GetSkillCheckPenaltyForFatigue(pSoldier, iChance);

  // if shooting same target from same position as the last shot
  if (sGridNo == pSoldier->sLastTarget) {
    iChance += AIM_BONUS_SAME_TARGET;  // give a bonus to hit
  }

  // ADJUST FOR EXTRA AIMING TIME
  if (ubAimTime) {
    iChance += (AIM_BONUS_PER_AP * ubAimTime);  // bonus for every pt of aiming
  }

  // if shooter is being affected by gas
  if (pSoldier->uiStatusFlags & SOLDIER_GASSED) {
    iChance -= AIM_PENALTY_GASSED;
  }

  // if shooter is being bandaged at the same time, his concentration is off
  if (pSoldier->ubServiceCount > 0) {
    iChance -= AIM_PENALTY_GETTINGAID;
  }

  // if shooter is still in shock
  if (pSoldier->bShock) {
    iChance -= (pSoldier->bShock * AIM_PENALTY_PER_SHOCK);
  }

  // calculate actual range (in world units)
  iRange = (int16_t)GetRangeInCellCoordsFromGridNoDiff(pSoldier->sGridNo, sGridNo);

  // NumMessage("ACTUAL RANGE = ",range);

  if (IsWearingHeadGear(*pSoldier, SUNGOGGLES)) {
    // decrease effective range by 10% when using sungoggles (w or w/o scope)
    iRange -= iRange / 10;  // basically, +1% to hit per every 2 squares
  }

  // NumMessage("EFFECTIVE RANGE = ",range);

  // ADJUST FOR RANGE

  if (usHandItem == MORTAR && iRange < MIN_MORTAR_RANGE) {
    return (0);
  } else {
    iMaxRange = CalcMaxTossRange(pSoldier, usHandItem, TRUE) * CELL_X_SIZE;

    // NumMessage("std::max RANGE = ",maxRange);

    // bonus if range is less than 1/2 maximum range, penalty if it's more

    // bonus is 50% at range 0, -50% at maximum range

    iChance += 50 * 2 * ((iMaxRange / 2) - iRange) / iMaxRange;
    // iChance += ((iMaxRange / 2) - iRange);		// increments of 1% per
    // pixel

    // IF TARGET IS BEYOND MAXIMUM THROWING RANGE
    if (iRange > iMaxRange) {
      // the object CAN travel that far if not blocked, but it's NOT accurate!
      iChance /= 2;
    }
  }

  // IF CHANCE EXISTS, BUT ATTACKER IS INJURED
  if ((iChance > 0) && (pSoldier->bLife < pSoldier->bLifeMax)) {
    // if bandaged, give 1/2 of the bandaged life points back into equation
    bBandaged = pSoldier->bLifeMax - pSoldier->bLife - pSoldier->bBleeding;

    // injury penalty is based on % damage taken (max 2/3rds iChance)
    bPenalty = (2 * iChance * (pSoldier->bLifeMax - pSoldier->bLife + (bBandaged / 2))) /
               (3 * pSoldier->bLifeMax);

    // for mechanically-fired projectiles, reduce penalty in half
    if (Item[usHandItem].usItemClass == IC_LAUNCHER) {
      bPenalty /= 2;
    }

    // reduce injury penalty due to merc's experience level (he can take it!)
    iChance -= (bPenalty * (100 - (10 * (EffectiveExpLevel(pSoldier) - 1)))) / 100;
  }

  // IF CHANCE EXISTS, BUT ATTACKER IS LOW ON BREATH
  if ((iChance > 0) && (pSoldier->bBreath < 100)) {
    // breath penalty is based on % breath missing (max 1/2 iChance)
    bPenalty = (iChance * (100 - pSoldier->bBreath)) / 200;

    // for mechanically-fired projectiles, reduce penalty in half
    if (Item[usHandItem].usItemClass == IC_LAUNCHER) bPenalty /= 2;

    // reduce breath penalty due to merc's dexterity (he can compensate!)
    iChance -= (bPenalty * (100 - (EffectiveDexterity(pSoldier) - 10))) / 100;
  }

  // if iChance exists, but it's a mechanical item being used
  if ((iChance > 0) && (Item[usHandItem].usItemClass == IC_LAUNCHER))
    // reduce iChance to hit DIRECTLY by the item's working condition
    iChance = (iChance * WEAPON_STATUS_MOD(pSoldier->inv[HANDPOS].bStatus[0])) / 100;

  // MAKE SURE CHANCE TO HIT IS WITHIN DEFINED LIMITS
  if (iChance < MINCHANCETOHIT)
    iChance = MINCHANCETOHIT;
  else {
    if (iChance > MAXCHANCETOHIT) iChance = MAXCHANCETOHIT;
  }

  // NumMessage("ThrownChanceToHit = ",iChance);
  return (iChance);
}

static BOOLEAN HasLauncher(const SOLDIERTYPE *const s) {
  OBJECTTYPE const &o = s->inv[HANDPOS];
  return FindAttachment(&o, UNDER_GLAUNCHER) != ITEM_NOT_FOUND &&
         FindLaunchableAttachment(&o, UNDER_GLAUNCHER) != ITEM_NOT_FOUND;
}

void ChangeWeaponMode(SOLDIERTYPE *const s) {
  // ATE: Don't do this if in a fire amimation.....
  if (gAnimControl[s->usAnimState].uiFlags & ANIM_FIRE) return;

  int8_t mode = s->bWeaponMode;
  switch (mode) {
    case WM_NORMAL:
      if (IsGunBurstCapable(s, HANDPOS)) {
        mode = WM_BURST;
      } else if (HasLauncher(s)) {
        mode = WM_ATTACHED;
      } else {
        ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, g_langRes->Message[STR_NOT_BURST_CAPABLE],
                  s->name);
      }
      break;

    case WM_BURST:
      mode = (HasLauncher(s) ? WM_ATTACHED : WM_NORMAL);
      break;

    default:
    case WM_ATTACHED:
      mode = WM_NORMAL;
      break;
  }

  s->bWeaponMode = mode;
  s->bDoBurst = (mode == WM_BURST);
  DirtyMercPanelInterface(s, DIRTYLEVEL2);
  gfUIForceReExamineCursorData = TRUE;
}

void DishoutQueenSwipeDamage(SOLDIERTYPE *pQueenSoldier) {
  static const int8_t bValidDishoutDirs[3][3] = {
      {NORTH, NORTHEAST, -1}, {EAST, SOUTHEAST, -1}, {SOUTH, -1, -1}};

  int8_t bDir;
  int32_t iChance;
  int32_t iImpact;
  int32_t iHitBy;

  // Loop through all mercs and make go
  FOR_EACH_MERC(i) {
    SOLDIERTYPE *const pSoldier = *i;
    if (pSoldier == pQueenSoldier) continue;

    // ATE: Ok, lets check for some basic things here!
    if (pSoldier->bLife >= OKLIFE && pSoldier->sGridNo != NOWHERE && pSoldier->bInSector) {
      // Get Pyth spaces away....
      if (GetRangeInCellCoordsFromGridNoDiff(pQueenSoldier->sGridNo, pSoldier->sGridNo) <=
          Weapon[CREATURE_QUEEN_TENTACLES].usRange) {
        // get direction
        bDir = (int8_t)GetDirectionFromGridNo(pSoldier->sGridNo, pQueenSoldier);

        //
        for (uint32_t cnt2 = 0; cnt2 < 2; ++cnt2) {
          if (bValidDishoutDirs[pQueenSoldier->uiPendingActionData1][cnt2] == bDir) {
            iChance = CalcChanceToStab(pQueenSoldier, pSoldier, 0);

            // CC: Look here for chance to hit, damage, etc...
            // May want to not hit if target is prone, etc....
            iHitBy = iChance - (int32_t)PreRandom(100);
            if (iHitBy > 0) {
              // Hit!
              iImpact = HTHImpact(pQueenSoldier, pSoldier, iHitBy, TRUE);
              EVENT_SoldierGotHit(pSoldier, CREATURE_QUEEN_TENTACLES, iImpact, iImpact,
                                  OppositeDirection(bDir), 50, pQueenSoldier, 0, ANIM_CROUCH, 0);
            }
          }
        }
      }
    }
  }

  pQueenSoldier->uiPendingActionData1++;
}

static BOOLEAN WillExplosiveWeaponFail(const SOLDIERTYPE *pSoldier, const OBJECTTYPE *pObj) {
  if (pSoldier->bTeam == OUR_TEAM || pSoldier->bVisible == 1) {
    if ((int8_t)(PreRandom(40) + PreRandom(40)) > pObj->bStatus[0]) {
      // Do second dice roll
      if (PreRandom(2) == 1) {
        // Fail
        return (TRUE);
      }
    }
  }

  return (FALSE);
}

#include "gtest/gtest.h"

TEST(Weapons, asserts) { EXPECT_EQ(lengthof(Weapon), MAX_WEAPONS); }
