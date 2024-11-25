// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Tactical/AirRaid.h"

#include "JAScreens.h"
#include "Macro.h"
#include "SGP/Debug.h"
#include "SGP/FileMan.h"
#include "SGP/Random.h"
#include "SGP/SoundMan.h"
#include "ScreenIDs.h"
#include "Strategic/CampaignTypes.h"
#include "Strategic/GameClock.h"
#include "Strategic/GameEventHook.h"
#include "Strategic/MapScreen.h"
#include "Strategic/MapScreenHelicopter.h"
#include "Strategic/MapScreenInterfaceBottom.h"
#include "Strategic/PreBattleInterface.h"
#include "Strategic/StrategicMap.h"
#include "Tactical/AutoBandage.h"
#include "Tactical/DialogueControl.h"
#include "Tactical/Interface.h"
#include "Tactical/LOS.h"
#include "Tactical/Morale.h"
#include "Tactical/Overhead.h"
#include "Tactical/OverheadTypes.h"
#include "Tactical/SoldierMacros.h"
#include "Tactical/StructureWrap.h"
#include "TileEngine/ExplosionControl.h"
#include "TileEngine/IsometricUtils.h"
#include "Utils/FontControl.h"
#include "Utils/Message.h"
#include "Utils/MusicControl.h"
#include "Utils/SoundControl.h"
#include "Utils/Text.h"
#include "Utils/TimerControl.h"
#include "math.h"

#define SCRIPT_DELAY 10
#define AIR_RAID_SAY_QUOTE_TIME 3000
#define AIR_RAID_DIVE_INTERVAL 10000
#define RAID_DELAY 40
#define TIME_FROM_DIVE_SOUND_TO_ATTACK_DELAY 8000
#define TIME_FROM_BOMB_SOUND_TO_ATTACK_DELAY 3000
#define MOVE_X 5
#define MOVE_Y 5
#define STRAFE_DIST 80
#define BOMB_DIST 150

// BEGIN SERALIZATION
extern int32_t giTimerAirRaidQuote;
extern int32_t giTimerAirRaidDiveStarted;
extern int32_t giTimerAirRaidUpdate;

BOOLEAN gfInAirRaid = FALSE;
BOOLEAN gfAirRaidScheduled = FALSE;
uint8_t gubAirRaidMode;
uint32_t guiSoundSample;
uint32_t guiRaidLastUpdate;
BOOLEAN gfFadingRaidIn = FALSE;
BOOLEAN gfQuoteSaid = FALSE;
int8_t gbNumDives = 0;
int8_t gbMaxDives = 0;
BOOLEAN gfFadingRaidOut = FALSE;
int16_t gsDiveX;
int16_t gsDiveY;
int16_t gsDiveTargetLocation;
uint8_t gubDiveDirection;
int16_t gsNumGridNosMoved;
int32_t giNumTurnsSinceLastDive;
int32_t giNumTurnsSinceDiveStarted;
int32_t giNumGridNosMovedThisTurn;
BOOLEAN gfAirRaidHasHadTurn = FALSE;
uint8_t gubBeginTeamTurn = 0;
BOOLEAN gfHaveTBBatton = FALSE;
int16_t gsNotLocatedYet = FALSE;
static int32_t giNumFrames;

AIR_RAID_DEFINITION gAirRaidDef;

struct AIR_RAID_SAVE_STRUCT {
  BOOLEAN fInAirRaid;
  BOOLEAN fAirRaidScheduled;
  uint8_t ubAirRaidMode;
  uint32_t uiSoundSample;
  uint32_t uiRaidLastUpdate;
  BOOLEAN fFadingRaidIn;
  BOOLEAN fQuoteSaid;
  int8_t bNumDives;
  int8_t bMaxDives;
  BOOLEAN fFadingRaidOut;
  int16_t sDiveX;
  int16_t sDiveY;
  int16_t sDiveTargetLocation;
  uint8_t ubDiveDirection;
  int16_t sNumGridNosMoved;
  int32_t iNumTurnsSinceLastDive;
  int32_t iNumTurnsSinceDiveStarted;
  int32_t iNumGridNosMovedThisTurn;
  BOOLEAN fAirRaidHasHadTurn;
  uint8_t ubBeginTeamTurn;
  BOOLEAN fHaveTBBatton;
  AIR_RAID_DEFINITION AirRaidDef;
  int16_t sRaidSoldierID;

  int16_t sNotLocatedYet;
  int32_t iNumFrames;

  int8_t bLevel;
  int8_t bTeam;
  int8_t bSide;
  uint8_t ubAttackerID;
  uint16_t usAttackingWeapon;
  float dXPos;
  float dYPos;
  int16_t sX;
  int16_t sY;
  int16_t sGridNo;

  uint8_t ubFiller[32];  // XXX HACK000B
};

// END SERIALIZATION
SOLDIERTYPE *gpRaidSoldier;

struct AIR_RAID_DIR {
  int8_t bDir1;
  int8_t bDir2;
};

struct AIR_RAID_POS {
  int8_t bX;
  int8_t bY;
};

AIR_RAID_DIR ubPerpDirections[] = {{2, 6}, {3, 7}, {0, 4}, {1, 5}, {2, 6}, {3, 7}, {0, 4}, {1, 5}};

AIR_RAID_POS ubXYTragetInvFromDirection[] = {{0, -1}, {1, -1}, {1, 0},  {1, 1},
                                             {0, 1},  {-1, 1}, {-1, 0}, {-1, -1}};

static void ScheduleAirRaid(AIR_RAID_DEFINITION *pAirRaidDef) {
  // Make sure only one is cheduled...
  if (gfAirRaidScheduled) {
    return;
  }

  // Copy definiaiotn structure into global struct....
  gAirRaidDef = *pAirRaidDef;

  AddSameDayStrategicEvent(EVENT_BEGIN_AIR_RAID,
                           (GetWorldMinutesInDay() + pAirRaidDef->ubNumMinsFromCurrentTime), 0);

  gfAirRaidScheduled = TRUE;
}

static int16_t PickLocationNearAnyMercInSector() {
  // Loop through all our guys and randomly say one from someone in our sector
  int32_t num_mercs = 0;
  const SOLDIERTYPE *mercs_in_sector[20];
  CFOR_EACH_IN_TEAM(s, OUR_TEAM) {
    // Add guy if he's a candidate...
    if (OkControllableMerc(s)) mercs_in_sector[num_mercs++] = s;
  }

  return num_mercs > 0 ? mercs_in_sector[Random(num_mercs)]->sGridNo : NOWHERE;
}

static int16_t PickRandomLocationAtMinSpacesAway(int16_t sGridNo, int16_t sMinValue,
                                                 int16_t sRandomVar) {
  int16_t sNewGridNo = NOWHERE;

  int16_t sX, sY, sNewX, sNewY;

  sX = CenterX(sGridNo);
  sY = CenterY(sGridNo);

  while (sNewGridNo == NOWHERE) {
    sNewX = sX + sMinValue + (int16_t)Random(sRandomVar);
    sNewY = sY + sMinValue + (int16_t)Random(sRandomVar);

    if (Random(2)) {
      sNewX = -1 * sNewX;
    }

    if (Random(2)) {
      sNewY = -1 * sNewY;
    }

    // Make gridno....
    sNewGridNo = GETWORLDINDEXFROMWORLDCOORDS(sNewY, sNewX);

    // Check if visible on screen....
    if (!GridNoOnVisibleWorldTile(sNewGridNo)) {
      sNewGridNo = NOWHERE;
    }
  }

  return (sNewGridNo);
}

static void TryToStartRaid() {
  // OK, check conditions,

  // Some are:

  // Cannot be in battle ( this is handled by the fact of it begin shceduled in
  // the first place...

  // Cannot be auto-bandaging?
  if (gTacticalStatus.fAutoBandageMode) {
    return;
  }

  // Cannot be in conversation...
  if (gTacticalStatus.uiFlags & ENGAGED_IN_CONV) {
    return;
  }

  // Cannot be traversing.....

  // Ok, go...
  gubAirRaidMode = AIR_RAID_START;
}

static void AirRaidStart() {
  // Begin ambient sound....
  guiSoundSample = PlayJA2Sample(S_RAID_AMBIENT, 0, 10000, MIDDLEPAN);

  gfFadingRaidIn = TRUE;

  // Setup start time....
  RESETTIMECOUNTER(giTimerAirRaidQuote, AIR_RAID_SAY_QUOTE_TIME);

  gubAirRaidMode = AIR_RAID_LOOK_FOR_DIVE;

  // If we are not in combat, change music mode...
  if (!(gTacticalStatus.uiFlags & INCOMBAT)) {
    SetMusicMode(MUSIC_TACTICAL_BATTLE);
  }
}

static void AirRaidLookForDive() {
  BOOLEAN fDoDive = FALSE;
  BOOLEAN fDoQuote = FALSE;

  if (!(gTacticalStatus.uiFlags & INCOMBAT)) {
    if (!gfQuoteSaid) {
      if (TIMECOUNTERDONE(giTimerAirRaidQuote, AIR_RAID_SAY_QUOTE_TIME)) {
        fDoQuote = TRUE;
      }
    }
  } else {
    if (giNumTurnsSinceLastDive > 1 && !gfQuoteSaid) {
      fDoQuote = TRUE;
    }
  }

  // OK, check if we should say something....
  if (fDoQuote) {
    gfQuoteSaid = TRUE;

    // Someone in group say quote...
    SayQuoteFromAnyBodyInSector(QUOTE_AIR_RAID);

    // Update timer
    RESETTIMECOUNTER(giTimerAirRaidDiveStarted, AIR_RAID_DIVE_INTERVAL);

    giNumTurnsSinceLastDive = 0;

    // Do morale hit on our guys
    HandleMoraleEvent(NULL, MORALE_AIRSTRIKE, gAirRaidDef.sSectorX, gAirRaidDef.sSectorY,
                      (int8_t)gAirRaidDef.sSectorZ);
  }

  // If NOT in combat....
  if (!(gTacticalStatus.uiFlags & INCOMBAT)) {
    // OK, for now on, all we try to do is look for dives to make...
    if (gfQuoteSaid) {
      if (TIMECOUNTERDONE(giTimerAirRaidDiveStarted, AIR_RAID_DIVE_INTERVAL)) {
        // IN realtime, give a bit more leeway for time....
        if (Random(2)) {
          fDoDive = TRUE;
        }
      }
    }
  } else {
    // How many turns have gone by?
    if ((uint32_t)giNumTurnsSinceLastDive > (Random(2) + 1)) {
      fDoDive = TRUE;
    }
  }

  if (fDoDive) {
    // If we are are beginning game, only to gun dives..
    if (gAirRaidDef.uiFlags & AIR_RAID_BEGINNING_GAME) {
      if (gbNumDives == 0) {
        gubAirRaidMode = AIR_RAID_BEGIN_DIVE;
      } else if (gbNumDives == 1) {
        gubAirRaidMode = AIR_RAID_BEGIN_BOMBING;
      } else {
        gubAirRaidMode = AIR_RAID_BEGIN_DIVE;
      }
    } else {
      // Randomly do dive...
      if (Random(2)) {
        gubAirRaidMode = AIR_RAID_BEGIN_DIVE;
      } else {
        gubAirRaidMode = AIR_RAID_BEGIN_BOMBING;
      }
    }
    gbNumDives++;
    return;
  } else {
    if ((gTacticalStatus.uiFlags & INCOMBAT)) {
      if (giNumGridNosMovedThisTurn == 0) {
        // Free up attacker...
        FreeUpAttacker(gpRaidSoldier);
        DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
                 String("!!!!!!! Tried to free up attacker AIR RAID NO DIVE, "
                        "attack count now %d",
                        gTacticalStatus.ubAttackBusyCount));
      }
    }
  }

  // End if we have made desired # of dives...
  if (gbNumDives == gbMaxDives) {
    // Air raid is over....
    gubAirRaidMode = AIR_RAID_START_END;
  }
}

static void AirRaidStartEnding() {
  // Fade out sound.....
  gfFadingRaidOut = TRUE;
}

static void BeginBombing() {
  int16_t sGridNo;
  uint32_t iSoundStartDelay;

  if (!(gTacticalStatus.uiFlags & INCOMBAT)) {
    // Start diving sound...
    PlayJA2Sample(S_RAID_WHISTLE, HIGHVOLUME, 1, MIDDLEPAN);
  }

  gubAirRaidMode = AIR_RAID_BOMBING;

  // Pick location...
  gsDiveTargetLocation = PickLocationNearAnyMercInSector();

  if (gsDiveTargetLocation == NOWHERE) {
    gsDiveTargetLocation = 10234;
  }

  // Get location of aircraft....
  sGridNo = PickRandomLocationAtMinSpacesAway(gsDiveTargetLocation, 300, 200);

  // Save X, y:
  gsDiveX = CenterX(sGridNo);
  gsDiveY = CenterY(sGridNo);

  RESETTIMECOUNTER(giTimerAirRaidUpdate, RAID_DELAY);

  if ((gTacticalStatus.uiFlags & INCOMBAT)) {
    iSoundStartDelay = 0;
  } else {
    iSoundStartDelay = TIME_FROM_BOMB_SOUND_TO_ATTACK_DELAY;
  }
  RESETTIMECOUNTER(giTimerAirRaidDiveStarted, iSoundStartDelay);

  giNumTurnsSinceDiveStarted = 0;

  // Get direction....
  gubDiveDirection = (int8_t)GetDirectionToGridNoFromGridNo(sGridNo, gsDiveTargetLocation);

  gsNumGridNosMoved = 0;
  gsNotLocatedYet = TRUE;
}

static void BeginDive() {
  int16_t sGridNo;
  uint32_t iSoundStartDelay;

  // Start diving sound...
  PlayJA2Sample(S_RAID_DIVE, HIGHVOLUME, 1, MIDDLEPAN);

  gubAirRaidMode = AIR_RAID_DIVING;

  // Increment attacker bust count....
  gTacticalStatus.ubAttackBusyCount++;
  DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
           String("!!!!!!! Starting attack BEGIN DIVE %d", gTacticalStatus.ubAttackBusyCount));

  // Pick location...
  gsDiveTargetLocation = PickLocationNearAnyMercInSector();

  if (gsDiveTargetLocation == NOWHERE) {
    gsDiveTargetLocation = 10234;
  }

  // Get location of aircraft....
  sGridNo = PickRandomLocationAtMinSpacesAway(gsDiveTargetLocation, 300, 200);

  // Save X, y:
  gsDiveX = CenterX(sGridNo);
  gsDiveY = CenterY(sGridNo);

  RESETTIMECOUNTER(giTimerAirRaidUpdate, RAID_DELAY);
  giNumTurnsSinceDiveStarted = 0;

  if ((gTacticalStatus.uiFlags & INCOMBAT)) {
    iSoundStartDelay = 0;
  } else {
    iSoundStartDelay = TIME_FROM_DIVE_SOUND_TO_ATTACK_DELAY;
  }
  RESETTIMECOUNTER(giTimerAirRaidDiveStarted, iSoundStartDelay);

  // Get direction....
  gubDiveDirection = (int8_t)GetDirectionToGridNoFromGridNo(sGridNo, gsDiveTargetLocation);

  gsNumGridNosMoved = 0;
  gsNotLocatedYet = TRUE;
}

static void MoveDiveAirplane(float dAngle) {
  float dDeltaPos;

  // Find delta Movement for X pos
  dDeltaPos = MOVE_X * (float)sin(dAngle);

  // Find new position
  gsDiveX = (int16_t)(gsDiveX + dDeltaPos);

  // Find delta Movement for Y pos
  dDeltaPos = MOVE_X * (float)cos(dAngle);

  // Find new pos
  gsDiveY = (int16_t)(gsDiveY + dDeltaPos);
}

static void DoDive() {
  int16_t sRange;
  int16_t sGridNo, sOldGridNo;

  int16_t sTargetX, sTargetY;
  int16_t sStrafeX, sStrafeY;
  float dDeltaX, dDeltaY, dAngle, dDeltaXPos, dDeltaYPos;
  int16_t sX, sY;

  // Delay for a specific perion of time to allow sound to Q up...
  if (TIMECOUNTERDONE(giTimerAirRaidDiveStarted, 0)) {
    // OK, rancomly decide to not do this dive...
    if (gAirRaidDef.uiFlags & AIR_RAID_CAN_RANDOMIZE_TEASE_DIVES) {
      if (Random(10) == 0) {
        // Finish....
        gubAirRaidMode = AIR_RAID_END_DIVE;
        return;
      }
    }

    if (gsNotLocatedYet && !(gTacticalStatus.uiFlags & INCOMBAT)) {
      gsNotLocatedYet = FALSE;
      LocateGridNo(gsDiveTargetLocation);
    }

    sOldGridNo = GETWORLDINDEXFROMWORLDCOORDS(gsDiveY, gsDiveX);

    // Dive until we are a certain range to target....
    sRange = PythSpacesAway(sOldGridNo, gsDiveTargetLocation);

    // If sRange
    if (sRange < 3) {
      // Finish....
      gubAirRaidMode = AIR_RAID_END_DIVE;
      return;
    }

    if (TIMECOUNTERDONE(giTimerAirRaidUpdate, RAID_DELAY)) {
      RESETTIMECOUNTER(giTimerAirRaidUpdate, RAID_DELAY);

      // Move Towards target....
      sTargetX = CenterX(gsDiveTargetLocation);
      sTargetY = CenterY(gsDiveTargetLocation);

      // Determine deltas
      dDeltaX = (float)(sTargetX - gsDiveX);
      dDeltaY = (float)(sTargetY - gsDiveY);

      // Determine angle
      dAngle = (float)atan2(dDeltaX, dDeltaY);

      MoveDiveAirplane(dAngle);

      gpRaidSoldier->dXPos = gsDiveX;
      gpRaidSoldier->sX = gsDiveX;
      gpRaidSoldier->dYPos = gsDiveY;
      gpRaidSoldier->sY = gsDiveY;

      // Figure gridno....
      sGridNo = GETWORLDINDEXFROMWORLDCOORDS(gsDiveY, gsDiveX);
      gpRaidSoldier->sGridNo = sGridNo;

      if (sOldGridNo != sGridNo) {
        gsNumGridNosMoved++;

        giNumGridNosMovedThisTurn++;

        // OK, shoot bullets....
        // Get positions of guns...

        // Get target.....
        dDeltaXPos = STRAFE_DIST * (float)sin(dAngle);
        sStrafeX = (int16_t)(gsDiveX + dDeltaXPos);

        // Find delta Movement for Y pos
        dDeltaYPos = STRAFE_DIST * (float)cos(dAngle);
        sStrafeY = (int16_t)(gsDiveY + dDeltaYPos);

        if ((gTacticalStatus.uiFlags & INCOMBAT)) {
          LocateGridNo(sGridNo);
        }

        if (GridNoOnVisibleWorldTile((int16_t)(GETWORLDINDEXFROMWORLDCOORDS(sStrafeY, sStrafeX)))) {
          // if ( gsNotLocatedYet && !( gTacticalStatus.uiFlags & INCOMBAT ) )
          //	{
          //	gsNotLocatedYet = FALSE;
          //		LocateGridNo( sGridNo );
          //	}

          // if ( ( gTacticalStatus.uiFlags & INCOMBAT ) )
          {
            // Increase attacker busy...
            // gTacticalStatus.ubAttackBusyCount++;
            // DebugMsg( TOPIC_JA2, DBG_LEVEL_3, String("!!!!!!! Starting attack
            // AIR RAID ( fire gun ), attack count now %d",
            // gTacticalStatus.ubAttackBusyCount) );

            // INcrement bullet fired...
            gpRaidSoldier->bBulletsLeft++;
          }

          // For now use first position....

          gpRaidSoldier->target = NULL;
          FireBulletGivenTarget(gpRaidSoldier, sStrafeX, sStrafeY, 0,
                                gpRaidSoldier->usAttackingWeapon, 10, FALSE, FALSE);
        }

        // Do second one.... ( ll )
        sX = (int16_t)(gsDiveX + ((float)sin(dAngle + (PI / 2)) * 40));
        sY = (int16_t)(gsDiveY + ((float)cos(dAngle + (PI / 2)) * 40));

        gpRaidSoldier->dXPos = sX;
        gpRaidSoldier->sX = sX;
        gpRaidSoldier->dYPos = sY;
        gpRaidSoldier->sY = sY;
        gpRaidSoldier->sGridNo = GETWORLDINDEXFROMWORLDCOORDS(sY, sX);

        // Get target.....
        sStrafeX = (int16_t)(sX + dDeltaXPos);

        // Find delta Movement for Y pos
        sStrafeY = (int16_t)(sY + dDeltaYPos);

        if (GridNoOnVisibleWorldTile((int16_t)(GETWORLDINDEXFROMWORLDCOORDS(sStrafeY, sStrafeX)))) {
          // if ( ( gTacticalStatus.uiFlags & INCOMBAT ) )
          {
            // Increase attacker busy...
            // gTacticalStatus.ubAttackBusyCount++;
            // DebugMsg( TOPIC_JA2, DBG_LEVEL_3, String("!!!!!!! Starting attack
            // AIR RAID ( second one ), attack count now %d",
            // gTacticalStatus.ubAttackBusyCount) );

            // INcrement bullet fired...
            gpRaidSoldier->bBulletsLeft++;
          }

          // For now use first position....
          FireBulletGivenTarget(gpRaidSoldier, sStrafeX, sStrafeY, 0,
                                gpRaidSoldier->usAttackingWeapon, 10, FALSE, FALSE);
        }
      }

      if (giNumGridNosMovedThisTurn >= 6) {
        if ((gTacticalStatus.uiFlags & INCOMBAT)) {
          // Free up attacker...
          FreeUpAttacker(gpRaidSoldier);
          DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
                   String("!!!!!!! Tried to free up attacker AIR RAID DIVE "
                          "DONE FOR THIS TURN, attack count now %d",
                          gTacticalStatus.ubAttackBusyCount));
        }
      }
    }
  }
}

static void DoBombing() {
  int16_t sRange;
  int16_t sGridNo, sOldGridNo, sBombGridNo;

  int16_t sTargetX, sTargetY;
  uint16_t usItem;
  int16_t sStrafeX, sStrafeY;
  float dDeltaX, dDeltaY, dAngle, dDeltaXPos, dDeltaYPos;
  BOOLEAN fLocate = FALSE;

  // Delay for a specific perion of time to allow sound to Q up...
  if (TIMECOUNTERDONE(giTimerAirRaidDiveStarted, 0)) {
    // OK, rancomly decide to not do this dive...
    if (gAirRaidDef.uiFlags & AIR_RAID_CAN_RANDOMIZE_TEASE_DIVES) {
      if (Random(10) == 0) {
        // Finish....
        gubAirRaidMode = AIR_RAID_END_BOMBING;
        return;
      }
    }

    if (gsNotLocatedYet && !(gTacticalStatus.uiFlags & INCOMBAT)) {
      gsNotLocatedYet = FALSE;
      LocateGridNo(gsDiveTargetLocation);
    }

    sOldGridNo = GETWORLDINDEXFROMWORLDCOORDS(gsDiveY, gsDiveX);

    // Dive until we are a certain range to target....
    sRange = PythSpacesAway(sOldGridNo, gsDiveTargetLocation);

    // If sRange
    if (sRange < 3) {
      // Finish....
      gubAirRaidMode = AIR_RAID_END_BOMBING;
      return;
    }

    if (TIMECOUNTERDONE(giTimerAirRaidUpdate, RAID_DELAY)) {
      RESETTIMECOUNTER(giTimerAirRaidUpdate, RAID_DELAY);

      // Move Towards target....
      sTargetX = CenterX(gsDiveTargetLocation);
      sTargetY = CenterY(gsDiveTargetLocation);

      // Determine deltas
      dDeltaX = (float)(sTargetX - gsDiveX);
      dDeltaY = (float)(sTargetY - gsDiveY);

      // Determine angle
      dAngle = (float)atan2(dDeltaX, dDeltaY);

      MoveDiveAirplane(dAngle);

      gpRaidSoldier->dXPos = gsDiveX;
      gpRaidSoldier->sX = gsDiveX;
      gpRaidSoldier->dYPos = gsDiveY;
      gpRaidSoldier->sY = gsDiveY;

      // Figure gridno....
      sGridNo = GETWORLDINDEXFROMWORLDCOORDS(gsDiveY, gsDiveX);
      gpRaidSoldier->sGridNo = sGridNo;

      if (sOldGridNo != sGridNo) {
        // Every once and a while, drop bomb....
        gsNumGridNosMoved++;

        giNumGridNosMovedThisTurn++;

        if ((gsNumGridNosMoved % 4) == 0) {
          // Get target.....
          dDeltaXPos = BOMB_DIST * (float)sin(dAngle);
          sStrafeX = (int16_t)(gsDiveX + dDeltaXPos);

          // Find delta Movement for Y pos
          dDeltaYPos = BOMB_DIST * (float)cos(dAngle);
          sStrafeY = (int16_t)(gsDiveY + dDeltaYPos);

          if (GridNoOnVisibleWorldTile(
                  (int16_t)(GETWORLDINDEXFROMWORLDCOORDS(sStrafeY, sStrafeX)))) {
            // if ( gsNotLocatedYet && !( gTacticalStatus.uiFlags & INCOMBAT ) )
            //{
            //	gsNotLocatedYet = FALSE;
            //	LocateGridNo( sGridNo );
            //}

            if (Random(2)) {
              usItem = HAND_GRENADE;
            } else {
              usItem = RDX;
            }

            // Pick random gridno....
            sBombGridNo = PickRandomLocationAtMinSpacesAway(
                (int16_t)(GETWORLDINDEXFROMWORLDCOORDS(sStrafeY, sStrafeX)), 40, 40);

            if ((gTacticalStatus.uiFlags & INCOMBAT)) {
              fLocate = TRUE;
              // Increase attacker busy...
              gTacticalStatus.ubAttackBusyCount++;
              DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
                       String("!!!!!!! Starting attack AIR RAID ( bombs away "
                              "), attack count now %d",
                              gTacticalStatus.ubAttackBusyCount));
            }

            // Drop bombs...
            InternalIgniteExplosion(NULL, CenterX(sBombGridNo), CenterY(sBombGridNo), 0,
                                    sBombGridNo, usItem, fLocate,
                                    IsRoofPresentAtGridno(sBombGridNo));
          }
        }

        if (giNumGridNosMovedThisTurn >= 6) {
          if ((gTacticalStatus.uiFlags & INCOMBAT)) {
            // Free up attacker...
            FreeUpAttacker(gpRaidSoldier);
            DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
                     String("!!!!!!! Tried to free up attacker AIR RAID BOMB "
                            "ATTACK DONE FOR THIS TURN, attack count now %d",
                            gTacticalStatus.ubAttackBusyCount));
          }
        }
      }
    }
  }
}

void HandleAirRaid() {
  int32_t iVol;
  uint32_t uiClock;

  // OK,
  if (gfInAirRaid) {
    // Are we in TB?
    if ((gTacticalStatus.uiFlags & INCOMBAT)) {
      // Do we have the batton?
      if (!gfHaveTBBatton) {
        // Don;t do anything else!
        return;
      }
    }

    uiClock = GetJA2Clock();

    if ((uiClock - guiRaidLastUpdate) > SCRIPT_DELAY) {
      giNumFrames++;

      guiRaidLastUpdate = uiClock;

      if (gfFadingRaidIn) {
        if (guiSoundSample != NO_SAMPLE) {
          if ((giNumFrames % 10) == 0) {
            iVol = SoundGetVolume(guiSoundSample);
            iVol = std::min(HIGHVOLUME, iVol + 1);
            SoundSetVolume(guiSoundSample, iVol);
            if (iVol == HIGHVOLUME) gfFadingRaidIn = FALSE;
          }
        } else {
          gfFadingRaidIn = FALSE;
        }
      } else if (gfFadingRaidOut) {
        if (guiSoundSample != NO_SAMPLE) {
          if ((giNumFrames % 10) == 0) {
            iVol = SoundGetVolume(guiSoundSample);

            iVol = std::max(0, iVol - 1);

            SoundSetVolume(guiSoundSample, iVol);
            if (iVol == 0) {
              gfFadingRaidOut = FALSE;

              gubAirRaidMode = AIR_RAID_END;
            }
          }
        } else {
          gfFadingRaidOut = FALSE;
          gubAirRaidMode = AIR_RAID_END;
        }
      }

      switch (gubAirRaidMode) {
        case AIR_RAID_TRYING_TO_START:

          TryToStartRaid();
          break;

        case AIR_RAID_START:

          AirRaidStart();
          break;

        case AIR_RAID_LOOK_FOR_DIVE:

          AirRaidLookForDive();
          break;

        case AIR_RAID_START_END:

          AirRaidStartEnding();
          break;

        case AIR_RAID_END:

          EndAirRaid();
          break;

        case AIR_RAID_BEGIN_DIVE:

          BeginDive();
          break;

        case AIR_RAID_DIVING:
          // If in combat, check if we have reached our max...
          if (!(gTacticalStatus.uiFlags & INCOMBAT) || giNumGridNosMovedThisTurn < 6) {
            DoDive();
          }
          break;

        case AIR_RAID_END_DIVE:

          giNumTurnsSinceLastDive = 0;
          RESETTIMECOUNTER(giTimerAirRaidDiveStarted, AIR_RAID_DIVE_INTERVAL);

          if ((gTacticalStatus.uiFlags & INCOMBAT)) {
            // Free up attacker...
            FreeUpAttacker(gpRaidSoldier);
            DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
                     String("!!!!!!! Tried to free up attacker AIR RAID ENDING "
                            "DIVE, attack count now %d",
                            gTacticalStatus.ubAttackBusyCount));
          }

          gubAirRaidMode = AIR_RAID_LOOK_FOR_DIVE;
          break;

        case AIR_RAID_END_BOMBING:

          RESETTIMECOUNTER(giTimerAirRaidDiveStarted, AIR_RAID_DIVE_INTERVAL);
          giNumTurnsSinceLastDive = 0;

          if ((gTacticalStatus.uiFlags & INCOMBAT)) {
            // Free up attacker...
            FreeUpAttacker(gpRaidSoldier);
            DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
                     String("!!!!!!! Tried to free up attacker AIR RAID ENDING "
                            "DIVE, attack count now %d",
                            gTacticalStatus.ubAttackBusyCount));
          }

          gubAirRaidMode = AIR_RAID_LOOK_FOR_DIVE;
          break;

        case AIR_RAID_BEGIN_BOMBING:
          BeginBombing();
          break;

        case AIR_RAID_BOMBING:
          DoBombing();
          break;
      }
    }

    if ((gTacticalStatus.uiFlags & INCOMBAT)) {
      // Do we have the batton?
      if (gfHaveTBBatton) {
        // Are we through with attacker busy count?
        if (gTacticalStatus.ubAttackBusyCount == 0) {
          // Relinquish control....
          gfAirRaidHasHadTurn = TRUE;
          gfHaveTBBatton = FALSE;
          BeginTeamTurn(gubBeginTeamTurn);
        }
      }
    }
  }
}

BOOLEAN InAirRaid() { return (gfInAirRaid); }

BOOLEAN HandleAirRaidEndTurn(uint8_t ubTeam) {
  if (!gfInAirRaid) {
    return (TRUE);
  }

  if (gfAirRaidHasHadTurn) {
    gfAirRaidHasHadTurn = FALSE;
    return (TRUE);
  }

  giNumTurnsSinceLastDive++;
  giNumTurnsSinceDiveStarted++;
  giNumGridNosMovedThisTurn = 0;
  gubBeginTeamTurn = ubTeam;
  gfHaveTBBatton = TRUE;

  // ATE: Even if we have an attacker busy problem.. init to 0 now
  // gTacticalStatus.ubAttackBusyCount = 0;

  // Increment attacker bust count....
  gTacticalStatus.ubAttackBusyCount++;
  DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
           String("!!!!!!! Starting attack AIR RAID, attack count now %d",
                  gTacticalStatus.ubAttackBusyCount));

  AddTopMessage(AIR_RAID_TURN_MESSAGE);

  // OK, handle some sound effects, depending on the mode we are in...
  if ((gTacticalStatus.uiFlags & INCOMBAT)) {
    switch (gubAirRaidMode) {
      case AIR_RAID_BOMBING:

        // Start diving sound...
        PlayJA2Sample(S_RAID_TB_BOMB, HIGHVOLUME, 1, MIDDLEPAN);
        break;

      case AIR_RAID_BEGIN_DIVE:

        PlayJA2Sample(S_RAID_TB_DIVE, HIGHVOLUME, 1, MIDDLEPAN);
        break;
    }
  }

  return (FALSE);
}

void SaveAirRaidInfoToSaveGameFile(HWFILE const hFile) {
  AIR_RAID_SAVE_STRUCT sAirRaidSaveStruct;

  // Put all the globals into the save struct
  sAirRaidSaveStruct.fInAirRaid = gfInAirRaid;
  sAirRaidSaveStruct.fAirRaidScheduled = gfAirRaidScheduled;
  sAirRaidSaveStruct.ubAirRaidMode = gubAirRaidMode;
  sAirRaidSaveStruct.uiSoundSample = guiSoundSample;
  sAirRaidSaveStruct.uiRaidLastUpdate = guiRaidLastUpdate;
  sAirRaidSaveStruct.fFadingRaidIn = gfFadingRaidIn;
  sAirRaidSaveStruct.fQuoteSaid = gfQuoteSaid;
  sAirRaidSaveStruct.bNumDives = gbNumDives;
  sAirRaidSaveStruct.bMaxDives = gbMaxDives;
  sAirRaidSaveStruct.fFadingRaidOut = gfFadingRaidOut;
  sAirRaidSaveStruct.sDiveX = gsDiveX;
  sAirRaidSaveStruct.sDiveY = gsDiveY;
  sAirRaidSaveStruct.sDiveTargetLocation = gsDiveTargetLocation;
  sAirRaidSaveStruct.ubDiveDirection = gubDiveDirection;
  sAirRaidSaveStruct.sNumGridNosMoved = gsNumGridNosMoved;
  sAirRaidSaveStruct.iNumTurnsSinceLastDive = giNumTurnsSinceLastDive;
  sAirRaidSaveStruct.iNumTurnsSinceDiveStarted = giNumTurnsSinceDiveStarted;
  sAirRaidSaveStruct.iNumGridNosMovedThisTurn = giNumGridNosMovedThisTurn;
  sAirRaidSaveStruct.fAirRaidHasHadTurn = gfAirRaidHasHadTurn;
  sAirRaidSaveStruct.ubBeginTeamTurn = gubBeginTeamTurn;
  sAirRaidSaveStruct.fHaveTBBatton = gfHaveTBBatton;

  sAirRaidSaveStruct.sNotLocatedYet = gsNotLocatedYet;
  sAirRaidSaveStruct.iNumFrames = giNumFrames;

  if (gpRaidSoldier) {
    sAirRaidSaveStruct.bLevel = gpRaidSoldier->bLevel;
    sAirRaidSaveStruct.bTeam = gpRaidSoldier->bTeam;
    sAirRaidSaveStruct.bSide = gpRaidSoldier->bSide;
    sAirRaidSaveStruct.ubAttackerID = Soldier2ID(gpRaidSoldier->attacker);
    sAirRaidSaveStruct.usAttackingWeapon = gpRaidSoldier->usAttackingWeapon;
    sAirRaidSaveStruct.dXPos = gpRaidSoldier->dXPos;
    sAirRaidSaveStruct.dYPos = gpRaidSoldier->dYPos;
    sAirRaidSaveStruct.sX = gpRaidSoldier->sX;
    sAirRaidSaveStruct.sY = gpRaidSoldier->sY;
    sAirRaidSaveStruct.sGridNo = gpRaidSoldier->sGridNo;

    sAirRaidSaveStruct.sRaidSoldierID = MAX_NUM_SOLDIERS - 1;
  } else
    sAirRaidSaveStruct.sRaidSoldierID = -1;

  sAirRaidSaveStruct.AirRaidDef = gAirRaidDef;

  // Save the Air Raid Save Struct
  FileWrite(hFile, &sAirRaidSaveStruct, sizeof(AIR_RAID_SAVE_STRUCT));
}

void LoadAirRaidInfoFromSaveGameFile(HWFILE const hFile) {
  AIR_RAID_SAVE_STRUCT sAirRaidSaveStruct;

  FileRead(hFile, &sAirRaidSaveStruct, sizeof(AIR_RAID_SAVE_STRUCT));

  // Put all the globals into the save struct
  gfInAirRaid = sAirRaidSaveStruct.fInAirRaid;
  gfAirRaidScheduled = sAirRaidSaveStruct.fAirRaidScheduled;
  gubAirRaidMode = sAirRaidSaveStruct.ubAirRaidMode;
  guiSoundSample = sAirRaidSaveStruct.uiSoundSample;
  guiRaidLastUpdate = sAirRaidSaveStruct.uiRaidLastUpdate;
  gfFadingRaidIn = sAirRaidSaveStruct.fFadingRaidIn;
  gfQuoteSaid = sAirRaidSaveStruct.fQuoteSaid;
  gbNumDives = sAirRaidSaveStruct.bNumDives;
  gbMaxDives = sAirRaidSaveStruct.bMaxDives;
  gfFadingRaidOut = sAirRaidSaveStruct.fFadingRaidOut;
  gsDiveX = sAirRaidSaveStruct.sDiveX;
  gsDiveY = sAirRaidSaveStruct.sDiveY;
  gsDiveTargetLocation = sAirRaidSaveStruct.sDiveTargetLocation;
  gubDiveDirection = sAirRaidSaveStruct.ubDiveDirection;
  gsNumGridNosMoved = sAirRaidSaveStruct.sNumGridNosMoved;
  giNumTurnsSinceLastDive = sAirRaidSaveStruct.iNumTurnsSinceLastDive;
  giNumTurnsSinceDiveStarted = sAirRaidSaveStruct.iNumTurnsSinceDiveStarted;
  giNumGridNosMovedThisTurn = sAirRaidSaveStruct.iNumGridNosMovedThisTurn;
  gfAirRaidHasHadTurn = sAirRaidSaveStruct.fAirRaidHasHadTurn;
  gubBeginTeamTurn = sAirRaidSaveStruct.ubBeginTeamTurn;
  gfHaveTBBatton = sAirRaidSaveStruct.fHaveTBBatton;

  gsNotLocatedYet = sAirRaidSaveStruct.sNotLocatedYet;
  giNumFrames = sAirRaidSaveStruct.iNumFrames;

  if (sAirRaidSaveStruct.sRaidSoldierID != -1) {
    SOLDIERTYPE &s = GetMan(sAirRaidSaveStruct.sRaidSoldierID);
    s.bLevel = sAirRaidSaveStruct.bLevel;
    s.bTeam = sAirRaidSaveStruct.bTeam;
    s.bSide = sAirRaidSaveStruct.bSide;
    s.attacker = ID2Soldier(sAirRaidSaveStruct.ubAttackerID);
    s.usAttackingWeapon = sAirRaidSaveStruct.usAttackingWeapon;
    s.dXPos = sAirRaidSaveStruct.dXPos;
    s.dYPos = sAirRaidSaveStruct.dYPos;
    s.sX = sAirRaidSaveStruct.sX;
    s.sY = sAirRaidSaveStruct.sY;
    s.sGridNo = sAirRaidSaveStruct.sGridNo;
    gpRaidSoldier = &s;
  } else
    gpRaidSoldier = NULL;

  gAirRaidDef = sAirRaidSaveStruct.AirRaidDef;
}

static void SetTeamStatusGreen(int8_t team) {
  FOR_EACH_IN_TEAM(s, team) {
    if (s->bInSector) s->bAlertStatus = STATUS_GREEN;
  }
  gTacticalStatus.Team[team].bAwareOfOpposition = FALSE;
}

void EndAirRaid() {
  gfInAirRaid = FALSE;

  // Stop sound
  SoundStop(guiSoundSample);

  // Change music back...
  if (!(gTacticalStatus.uiFlags & INCOMBAT)) {
    SetMusicMode(MUSIC_TACTICAL_NOTHING);

    if (!IsTeamActive(ENEMY_TEAM) && !IsTeamActive(CREATURE_TEAM)) {
      SetTeamStatusGreen(MILITIA_TEAM);
      SetTeamStatusGreen(CIV_TEAM);
    }
  }

  // OK, look at flags...
  if (gAirRaidDef.uiFlags &
      AIR_RAID_BEGINNING_GAME) {  // OK, make enemy appear in Omerta
                                  // Talk to strategic AI for this...
                                  // GROUP *pGroup;
                                  // Create a patrol group originating from
                                  // sector B9 pGroup =
                                  // CreateNewEnemyGroupDepartingFromSector(
                                  // SEC_B9, (uint8_t)(2 + Random( 2 ) +
                                  // gGameOptions.ubDifficultyLevel), 0 ); Move
                                  // the patrol group north to attack Omerta
                                  // AddWaypointToPGroup( pGroup, 9, 1 ); //A9
                                  // Because we want them to arrive right away,
                                  // we will toast the arrival event.  The
                                  // information is already set up though.
                                  // DeleteStrategicEvent( EVENT_GROUP_ARRIVAL,
                                  // pGroup->ubGroupID ); Simply reinsert the
                                  // event, but the time is now.
                                  // AddStrategicEvent( EVENT_GROUP_ARRIVAL,
                                  // GetWorldTotalMin(), pGroup->ubGroupID );
  }

  ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_BETAVERSION, L"Ending Air Raid.");
}

#include "gtest/gtest.h"

TEST(AirRaid, asserts) {
  EXPECT_EQ(sizeof(AIR_RAID_SAVE_STRUCT), 132);
  EXPECT_EQ(sizeof(AIR_RAID_DEFINITION), 24);
}
