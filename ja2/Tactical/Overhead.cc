// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Tactical/Overhead.h"

#include <algorithm>
#include <string.h>

#include "Editor/EditorMercs.h"
#include "GameSettings.h"
#include "JAScreens.h"
#include "Laptop/History.h"
#include "Macro.h"
#include "MessageBoxScreen.h"
#include "SGP/Debug.h"
#include "SGP/MouseSystem.h"
#include "SGP/Random.h"
#include "SGP/VObject.h"
#include "SGP/WCheck.h"
#include "ScreenIDs.h"
#include "Strategic/Assignments.h"
#include "Strategic/GameClock.h"
#include "Strategic/GameEventHook.h"
#include "Strategic/GameInit.h"
#include "Strategic/MapScreen.h"
#include "Strategic/MapScreenHelicopter.h"
#include "Strategic/Meanwhile.h"
#include "Strategic/PlayerCommand.h"
#include "Strategic/PreBattleInterface.h"
#include "Strategic/QueenCommand.h"
#include "Strategic/Quests.h"
#include "Strategic/StrategicAI.h"
#include "Strategic/StrategicMap.h"
#include "Strategic/StrategicMercHandler.h"
#include "Strategic/StrategicMines.h"
#include "Strategic/StrategicStatus.h"
#include "Strategic/StrategicTownLoyalty.h"
#include "Strategic/StrategicTurns.h"
#include "Strategic/TownMilitia.h"
#include "Tactical/AirRaid.h"
#include "Tactical/AnimationControl.h"
#include "Tactical/AnimationData.h"
#include "Tactical/ArmsDealerInit.h"
#include "Tactical/AutoBandage.h"
#include "Tactical/Boxing.h"
#include "Tactical/Campaign.h"
#include "Tactical/CivQuotes.h"
#include "Tactical/DialogueControl.h"
#include "Tactical/DrugsAndAlcohol.h"
#include "Tactical/EndGame.h"
#include "Tactical/FOV.h"
#include "Tactical/Faces.h"
#include "Tactical/HandleDoors.h"
#include "Tactical/HandleItems.h"
#include "Tactical/Interface.h"
#include "Tactical/InterfaceControl.h"
#include "Tactical/InterfaceCursors.h"
#include "Tactical/InterfaceDialogue.h"
#include "Tactical/InterfaceItems.h"
#include "Tactical/InterfacePanels.h"
#include "Tactical/InterfaceUtils.h"
#include "Tactical/Items.h"
#include "Tactical/Keys.h"
#include "Tactical/LOS.h"
#include "Tactical/Morale.h"
#include "Tactical/OppList.h"
#include "Tactical/PathAI.h"
#include "Tactical/Points.h"
#include "Tactical/RottingCorpses.h"
#include "Tactical/SkillCheck.h"
#include "Tactical/SoldierAni.h"
#include "Tactical/SoldierControl.h"
#include "Tactical/SoldierFind.h"
#include "Tactical/SoldierFunctions.h"
#include "Tactical/SoldierMacros.h"
#include "Tactical/SoldierProfile.h"
#include "Tactical/SoldierTile.h"
#include "Tactical/SpreadBurst.h"
#include "Tactical/Squads.h"
#include "Tactical/StructureWrap.h"
#include "Tactical/Weapons.h"
#include "Tactical/WorldItems.h"
#include "TacticalAI/AI.h"
#include "TacticalAI/NPC.h"
#include "TileEngine/ExitGrids.h"
#include "TileEngine/ExplosionControl.h"
#include "TileEngine/InteractiveTiles.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/Lighting.h"
#include "TileEngine/RenderDirty.h"
#include "TileEngine/RenderFun.h"
#include "TileEngine/RenderWorld.h"
#include "TileEngine/Smell.h"
#include "TileEngine/Structure.h"
#include "TileEngine/SysUtil.h"
#include "TileEngine/TileAnimation.h"
#include "TileEngine/TileDef.h"
#include "TileEngine/WorldDef.h"
#include "TileEngine/WorldMan.h"
#include "Utils/EventPump.h"
#include "Utils/FontControl.h"
#include "Utils/Message.h"
#include "Utils/MusicControl.h"
#include "Utils/SoundControl.h"
#include "Utils/Text.h"
#include "Utils/TimerControl.h"

#define RT_DELAY_BETWEEN_AI_HANDLING 50
#define RT_AI_TIMESLICE 10

int32_t giRTAILastUpdateTime = 0;
static uint32_t guiAISlotToHandle = 0;
#define HANDLE_OFF_MAP_MERC 0xFFFF
#define RESET_HANDLE_OF_OFF_MAP_MERCS 0xFFFF
static uint32_t guiAIAwaySlotToHandle = RESET_HANDLE_OF_OFF_MAP_MERCS;

#define PAUSE_ALL_AI_DELAY 1500

static BOOLEAN gfPauseAllAI = FALSE;
static int32_t giPauseAllAITimer = 0;

BOOLEAN gfSurrendered = FALSE;

#define NEW_FADE_DELAY 60

// Soldier List used for all soldier overhead interaction
SOLDIERTYPE Menptr[TOTAL_SOLDIERS];

SOLDIERTYPE *MercSlots[TOTAL_SOLDIERS];
uint32_t guiNumMercSlots = 0;

TacticalStatusType gTacticalStatus;

static SOLDIERTYPE *AwaySlots[TOTAL_SOLDIERS];
static uint32_t guiNumAwaySlots = 0;

// Global for current selected soldier
SOLDIERTYPE *g_selected_man;

const char *const gzActionStr[] = {"NONE",

                                   "RANDOM PATROL",
                                   "SEEK FRIEND",
                                   "SEEK OPPONENT",
                                   "TAKE COVER",
                                   "GET CLOSER",

                                   "POINT PATROL",
                                   "LEAVE WATER GAS",
                                   "SEEK NOISE",
                                   "ESCORTED MOVE",
                                   "RUN AWAY",

                                   "KNIFE MOVE",
                                   "APPROACH MERC",
                                   "TRACK",
                                   "EAT",
                                   "PICK UP ITEM",

                                   "SCHEDULE MOVE",
                                   "WALK",
                                   "RUN",
                                   "MOVE TO CLIMB",
                                   "CHG FACING",

                                   "CHG STANCE",
                                   "YELLOW ALERT",
                                   "RED ALERT",
                                   "CREATURE CALL",
                                   "PULL TRIGGER",

                                   "USE DETONATOR",
                                   "FIRE GUN",
                                   "TOSS PROJECTILE",
                                   "KNIFE STAB",
                                   "THROW KNIFE",

                                   "GIVE AID",
                                   "WAIT",
                                   "PENDING ACTION",
                                   "DROP ITEM",
                                   "COWER",

                                   "STOP COWERING",
                                   "OPEN/CLOSE DOOR",
                                   "UNLOCK DOOR",
                                   "LOCK DOOR",
                                   "LOWER GUN",

                                   "ABSOLUTELY NONE",
                                   "CLIMB ROOF",
                                   "END TURN",
                                   "EC&M",
                                   "TRAVERSE DOWN",
                                   "OFFER SURRENDER"};

struct TeamInfo {
  uint8_t size;
  int8_t side;
  bool human;
  COLORVAL colour;
};

/* Militia guys on our side.
 * Creatures are on no one's side but their own.
 * NB: side 2 is used for hostile rebels.
 * Rest hostile (enemies, or civilians; civs are potentially hostile but
 * neutral) */
static TeamInfo const g_default_team_info[] = {
    {20, 0, true, FROMRGB(255, 255, 0)},               // Us
    {32, 1, false, FROMRGB(255, 0, 0)},                // Enemy
    {32, 3, false, FROMRGB(255, 0, 255)},              // Creature
    {32, 0, false, FROMRGB(0, 255, 0)},                // Rebels (our guys)
    {32, 1, false, FROMRGB(255, 255, 255)},            // Civilians
    {NUM_PLANNING_MERCS, 0, true, FROMRGB(0, 0, 255)}  // Planning soldiers
};

uint8_t gubWaitingForAllMercsToExitCode = 0;
static int8_t gbNumMercsUntilWaitingOver = 0;
static uint32_t guiWaitingForAllMercsToExitTimer = 0;
BOOLEAN gfKillingGuysForLosingBattle = FALSE;

static int32_t GetFreeMercSlot() {
  for (uint32_t i = 0; i < guiNumMercSlots; ++i) {
    if (MercSlots[i] == NULL) return i;
  }

  if (guiNumMercSlots < TOTAL_SOLDIERS) return guiNumMercSlots++;

  return -1;
}

static void RecountMercSlots() {
  // set equal to 0 as a default
  for (int32_t i = (int32_t)guiNumMercSlots - 1; i >= 0; --i) {
    if (MercSlots[i] != NULL) {
      guiNumMercSlots = i + 1;
      return;
    }
  }
  // no mercs found
  guiNumMercSlots = 0;
}

void AddMercSlot(SOLDIERTYPE *pSoldier) {
  const int32_t iMercIndex = GetFreeMercSlot();
  if (iMercIndex != -1) MercSlots[iMercIndex] = pSoldier;
}

BOOLEAN RemoveMercSlot(SOLDIERTYPE *pSoldier) {
  CHECKF(pSoldier != NULL);

  for (uint32_t i = 0; i < guiNumMercSlots; ++i) {
    if (MercSlots[i] == pSoldier) {
      MercSlots[i] = NULL;
      RecountMercSlots();
      return TRUE;
    }
  }

  // TOLD TO DELETE NON-EXISTANT SOLDIER
  return FALSE;
}

static int32_t GetFreeAwaySlot() {
  for (uint32_t i = 0; i < guiNumAwaySlots; ++i) {
    if (AwaySlots[i] == NULL) return i;
  }

  if (guiNumAwaySlots < TOTAL_SOLDIERS) return guiNumAwaySlots++;

  return -1;
}

static void RecountAwaySlots() {
  for (int32_t i = guiNumAwaySlots - 1; i >= 0; --i) {
    if (AwaySlots[i] != NULL) {
      guiNumAwaySlots = i + 1;
      return;
    }
  }
  // no mercs found
  guiNumAwaySlots = 0;
}

int32_t AddAwaySlot(SOLDIERTYPE *pSoldier) {
  const int32_t iAwayIndex = GetFreeAwaySlot();
  if (iAwayIndex != -1) AwaySlots[iAwayIndex] = pSoldier;
  return iAwayIndex;
}

BOOLEAN RemoveAwaySlot(SOLDIERTYPE *pSoldier) {
  CHECKF(pSoldier != NULL);

  for (uint32_t i = 0; i < guiNumAwaySlots; ++i) {
    if (AwaySlots[i] == pSoldier) {
      AwaySlots[i] = NULL;
      RecountAwaySlots();
      return TRUE;
    }
  }

  // TOLD TO DELETE NON-EXISTANT SOLDIER
  return FALSE;
}

int32_t MoveSoldierFromMercToAwaySlot(SOLDIERTYPE *pSoldier) {
  BOOLEAN fRet = RemoveMercSlot(pSoldier);
  if (!fRet) return -1;

  if (!(pSoldier->uiStatusFlags & SOLDIER_OFF_MAP)) {
    RemoveManFromTeam(pSoldier->bTeam);
  }

  pSoldier->bInSector = FALSE;
  pSoldier->uiStatusFlags |= SOLDIER_OFF_MAP;
  return AddAwaySlot(pSoldier);
}

void MoveSoldierFromAwayToMercSlot(SOLDIERTYPE *const pSoldier) {
  if (!RemoveAwaySlot(pSoldier)) return;

  AddManToTeam(pSoldier->bTeam);

  pSoldier->bInSector = TRUE;
  pSoldier->uiStatusFlags &= ~SOLDIER_OFF_MAP;
  AddMercSlot(pSoldier);
}

void InitTacticalEngine() {
  InitRenderParams(0);
  InitializeTacticalInterface();
  InitializeGameVideoObjects();
  LoadPaletteData();
  LoadDialogueControlGraphics();
  LoadFacesGraphics();
  LoadInterfacePanelGraphics();
  LoadSpreadBurstGraphics();

  LoadLockTable();
  InitPathAI();
  InitAI();
  InitOverhead();
}

void ShutdownTacticalEngine() {
  DeleteSpreadBurstGraphics();
  DeleteInterfacePanelGraphics();
  DeleteFacesGraphics();
  DeleteDialogueControlGraphics();
  DeletePaletteData();
  ShutdownStaticExternalNPCFaces();
  ShutDownPathAI();
  UnLoadCarPortraits();
  ShutdownNPCQuotes();
}

void InitOverhead() {
  memset(MercSlots, 0, sizeof(MercSlots));
  memset(AwaySlots, 0, sizeof(AwaySlots));
  memset(Menptr, 0, sizeof(Menptr));

  TacticalStatusType &t = gTacticalStatus;
  memset(&t, 0, sizeof(TacticalStatusType));
  t.uiFlags = TURNBASED;
  t.sSlideTarget = NOWHERE;
  t.uiTimeOfLastInput = GetJA2Clock();
  t.uiTimeSinceDemoOn = GetJA2Clock();
  t.bRealtimeSpeed = MAX_REALTIME_SPEED_VAL / 2;
  FOR_EACH(int16_t, i, t.sPanicTriggerGridNo) *i = NOWHERE;
  t.fDidGameJustStart = TRUE;
  t.ubLastRequesterTargetID = NO_PROFILE;

  // Set team values
  uint8_t team_start = 0;
  for (uint32_t i = 0; i != MAXTEAMS; ++i) {
    TacticalTeamType &team = t.Team[i];
    TeamInfo const &info = g_default_team_info[i];
    // For now, set hard-coded values
    team.bFirstID = team_start;
    team_start += info.size;
    team.bLastID = team_start - 1;
    team.RadarColor = info.colour;
    team.bSide = info.side;
    team.bAwareOfOpposition = FALSE;
    team.bHuman = info.human;
    team.last_merc_to_radio = 0;
  }

  gfInAirRaid = FALSE;
  gpCustomizableTimerCallback = 0;

  // Reset cursor
  gpItemPointer = 0;
  gpItemPointerSoldier = 0;
  memset(gbInvalidPlacementSlot, 0, sizeof(gbInvalidPlacementSlot));

  InitCivQuoteSystem();
  ZeroAnimSurfaceCounts();
}

void ShutdownOverhead() {
  // Delete any soldiers which have been created!
  FOR_EACH_SOLDIER(i) DeleteSoldier(*i);
}

static BOOLEAN NextAIToHandle(uint32_t uiCurrAISlot) {
  uint32_t cnt;
  if (uiCurrAISlot >= guiNumMercSlots) {
    // last person to handle was an off-map merc, so now we start looping at the
    // beginning again
    cnt = 0;
  } else {
    // continue on from the last person we handled
    cnt = uiCurrAISlot + 1;
  }

  for (; cnt < guiNumMercSlots; ++cnt) {
    if (MercSlots[cnt] && (MercSlots[cnt]->bTeam != OUR_TEAM ||
                           MercSlots[cnt]->uiStatusFlags & SOLDIER_PCUNDERAICONTROL)) {
      // aha! found an AI guy!
      guiAISlotToHandle = cnt;
      return TRUE;
    }
  }

  // set so that even if there are no off-screen mercs to handle, we will loop
  // back to the start of the array
  guiAISlotToHandle = HANDLE_OFF_MAP_MERC;

  // didn't find an AI guy to handle after the last one handled and the # of
  // slots it's time to check for an off-map merc... maybe
  if (guiNumAwaySlots > 0) {
    if (guiAIAwaySlotToHandle + 1 >= guiNumAwaySlots) {
      // start looping from the beginning
      cnt = 0;
    } else {
      // continue on from the last person we handled
      cnt = guiAIAwaySlotToHandle + 1;
    }

    for (; cnt < guiNumAwaySlots; ++cnt) {
      if (AwaySlots[cnt] && AwaySlots[cnt]->bTeam != OUR_TEAM) {
        // aha! found an AI guy!
        guiAIAwaySlotToHandle = cnt;
        return FALSE;
      }
    }

    // reset awayAISlotToHandle, but DON'T loop again, away slots not that
    // important
    guiAIAwaySlotToHandle = RESET_HANDLE_OF_OFF_MAP_MERCS;
  }

  return FALSE;
}

void PauseAITemporarily() {
  gfPauseAllAI = TRUE;
  giPauseAllAITimer = GetJA2Clock();
}

void PauseAIUntilManuallyUnpaused() {
  gfPauseAllAI = TRUE;
  giPauseAllAITimer = 0;
}

void UnPauseAI() {
  // overrides any timer too
  gfPauseAllAI = FALSE;
  giPauseAllAITimer = 0;
}

static void HandleBloodForNewGridNo(const SOLDIERTYPE *pSoldier);
static BOOLEAN HandleAtNewGridNo(SOLDIERTYPE *pSoldier, BOOLEAN *pfKeepMoving);
static void HandleCreatureTenseQuote();

void ExecuteOverhead() {
  // Diagnostic Stuff
  static int32_t iTimerTest = 0;

  BOOLEAN fKeepMoving;

  if (COUNTERDONE(TOVERHEAD)) {
    RESETCOUNTER(TOVERHEAD);

    // Diagnostic Stuff
    int32_t iTimerVal = GetJA2Clock();
    giTimerDiag = iTimerVal - iTimerTest;
    iTimerTest = iTimerVal;

    // ANIMATED TILE STUFF
    UpdateAniTiles();

    // BOMBS!!!
    HandleExplosionQueue();

    HandleCreatureTenseQuote();

    CheckHostileOrSayQuoteList();

    if (gfPauseAllAI && giPauseAllAITimer && iTimerVal - giPauseAllAITimer > PAUSE_ALL_AI_DELAY) {
      // ok, stop pausing the AI!
      gfPauseAllAI = FALSE;
    }

    BOOLEAN fHandleAI = FALSE;
    if (!gfPauseAllAI) {
      // AI limiting crap
      if (!(gTacticalStatus.uiFlags & TURNBASED) || !(gTacticalStatus.uiFlags & INCOMBAT)) {
        if (iTimerVal - giRTAILastUpdateTime > RT_DELAY_BETWEEN_AI_HANDLING) {
          giRTAILastUpdateTime = iTimerVal;
          // figure out which AI guy to handle this time around,
          // starting with the slot AFTER the current AI guy
          fHandleAI = NextAIToHandle(guiAISlotToHandle);
        }
      }
    }

    for (uint32_t cnt = 0; cnt < guiNumMercSlots; ++cnt) {
      SOLDIERTYPE *pSoldier = MercSlots[cnt];

      // Syncronize for upcoming soldier counters
      SYNCTIMECOUNTER();

      if (pSoldier != NULL) {
        HandlePanelFaceAnimations(pSoldier);

        // Handle damage counters
        if (pSoldier->fDisplayDamage) {
          if (TIMECOUNTERDONE(pSoldier->DamageCounter, DAMAGE_DISPLAY_DELAY)) {
            pSoldier->bDisplayDamageCount++;
            pSoldier->sDamageX += 1;
            pSoldier->sDamageY -= 1;

            RESETTIMECOUNTER(pSoldier->DamageCounter, DAMAGE_DISPLAY_DELAY);
          }

          if (pSoldier->bDisplayDamageCount >= 8) {
            pSoldier->bDisplayDamageCount = 0;
            pSoldier->sDamage = 0;
            pSoldier->fDisplayDamage = FALSE;
          }
        }

        // Checkout fading
        if (pSoldier->fBeginFade && TIMECOUNTERDONE(pSoldier->FadeCounter, NEW_FADE_DELAY)) {
          RESETTIMECOUNTER(pSoldier->FadeCounter, NEW_FADE_DELAY);

          // Fade out....
          if (pSoldier->fBeginFade == 1) {
            int8_t bShadeLevel = pSoldier->ubFadeLevel & 0x0f;
            bShadeLevel = std::min(bShadeLevel + 1, SHADE_MIN);

            if (bShadeLevel >= SHADE_MIN - 3) {
              pSoldier->fBeginFade = FALSE;
              pSoldier->bVisible = -1;

              // Set levelnode shade level....
              if (pSoldier->pLevelNode) {
                pSoldier->pLevelNode->ubShadeLevel = bShadeLevel;
              }

              // Set Anim speed accordingly!
              SetSoldierAniSpeed(pSoldier);
            }

            bShadeLevel |= pSoldier->ubFadeLevel & 0x30;
            pSoldier->ubFadeLevel = bShadeLevel;
          } else if (pSoldier->fBeginFade == 2) {
            int8_t bShadeLevel = pSoldier->ubFadeLevel & 0x0f;
            // ubShadeLevel =std::max(ubShadeLevel-1, gpWorldLevelData[
            // pSoldier->sGridNo ].pLandHead->ubShadeLevel );
            bShadeLevel = std::max(0, bShadeLevel - 1);

            if (bShadeLevel <= gpWorldLevelData[pSoldier->sGridNo].pLandHead->ubShadeLevel) {
              bShadeLevel = gpWorldLevelData[pSoldier->sGridNo].pLandHead->ubShadeLevel;

              pSoldier->fBeginFade = FALSE;
              // pSoldier->bVisible = -1;
              // pSoldier->ubFadeLevel = gpWorldLevelData[ pSoldier->sGridNo
              // ].pLandHead->ubShadeLevel;

              // Set levelnode shade level....
              if (pSoldier->pLevelNode) {
                pSoldier->pLevelNode->ubShadeLevel = bShadeLevel;
              }

              // Set Anim speed accordingly!
              SetSoldierAniSpeed(pSoldier);
            }

            bShadeLevel |= pSoldier->ubFadeLevel & 0x30;
            pSoldier->ubFadeLevel = bShadeLevel;
          }
        }

        // Check if we have a new visiblity and shade accordingly down
        if (pSoldier->bLastRenderVisibleValue != pSoldier->bVisible) {
          HandleCrowShadowVisibility(*pSoldier);

          // Check for fade out....
          if (pSoldier->bVisible == -1 && pSoldier->bLastRenderVisibleValue >= 0) {
            if (pSoldier->sGridNo != NOWHERE) {
              pSoldier->ubFadeLevel = gpWorldLevelData[pSoldier->sGridNo].pLandHead->ubShadeLevel;
            }
            pSoldier->fBeginFade = TRUE;
            pSoldier->sLocationOfFadeStart = pSoldier->sGridNo;

            // OK, re-evaluate guy's roof marker
            HandlePlacingRoofMarker(*pSoldier, false, false);

            pSoldier->bVisible = -2;
          }

          // Check for fade in.....
          if (pSoldier->bVisible != -1 && pSoldier->bLastRenderVisibleValue == -1 &&
              pSoldier->bTeam != OUR_TEAM) {
            pSoldier->ubFadeLevel = SHADE_MIN - 3;
            pSoldier->fBeginFade = 2;
            pSoldier->sLocationOfFadeStart = pSoldier->sGridNo;

            // OK, re-evaluate guy's roof marker
            HandlePlacingRoofMarker(*pSoldier, true, false);
          }
        }
        pSoldier->bLastRenderVisibleValue = pSoldier->bVisible;

        // Handle stationary polling...
        if (gAnimControl[pSoldier->usAnimState].uiFlags & ANIM_STATIONARY ||
            pSoldier->fNoAPToFinishMove) {
          // Are are stationary....
          // Were we once moving...?
          if (pSoldier->fSoldierWasMoving && pSoldier->bVisible > -1) {
            pSoldier->fSoldierWasMoving = FALSE;

            HandlePlacingRoofMarker(*pSoldier, true, false);

            if (!gGameSettings.fOptions[TOPTION_MERC_ALWAYS_LIGHT_UP]) {
              DeleteSoldierLight(pSoldier);
              PositionSoldierLight(pSoldier);
            }
          }
        } else {
          // We are moving....
          // Were we once stationary?
          if (!pSoldier->fSoldierWasMoving) {
            pSoldier->fSoldierWasMoving = TRUE;
            HandlePlacingRoofMarker(*pSoldier, false, false);
          }
        }

        // Handle animation update counters
        // ATE: Added additional check here for special value of anispeed that
        // pauses all updates
#ifndef BOUNDS_CHECKER
        if (TIMECOUNTERDONE(pSoldier->UpdateCounter, pSoldier->sAniDelay) &&
            pSoldier->sAniDelay != 10000)
#endif
        {
          // Check if we need to look for items
          if (pSoldier->uiStatusFlags & SOLDIER_LOOKFOR_ITEMS) {
            RevealRoofsAndItems(pSoldier, FALSE);
            pSoldier->uiStatusFlags &= ~SOLDIER_LOOKFOR_ITEMS;
          }

          RESETTIMECOUNTER(pSoldier->UpdateCounter, pSoldier->sAniDelay);

          BOOLEAN fNoAPsForPendingAction = FALSE;

          // Check if we are moving and we deduct points and we have no points
          if ((!(gAnimControl[pSoldier->usAnimState].uiFlags & (ANIM_MOVING | ANIM_SPECIALMOVE)) ||
               !pSoldier->fNoAPToFinishMove) &&
              !pSoldier->fPauseAllAnimation) {
            if (!AdjustToNextAnimationFrame(pSoldier)) {
              continue;
            }

            if (!(gAnimControl[pSoldier->usAnimState].uiFlags & ANIM_SPECIALMOVE)) {
              // Check if we are waiting for an opened path
              HandleNextTileWaiting(pSoldier);
            }

            // Update world data with new position, etc
            // Determine gameworld cells corrds of guy
            if (gAnimControl[pSoldier->usAnimState].uiFlags & (ANIM_MOVING | ANIM_SPECIALMOVE) &&
                !(pSoldier->uiStatusFlags & SOLDIER_PAUSEANIMOVE)) {
              fKeepMoving = TRUE;

              pSoldier->fPausedMove = FALSE;

              // CHECK TO SEE IF WE'RE ON A MIDDLE TILE
              if (pSoldier->fPastXDest && pSoldier->fPastYDest) {
                pSoldier->fPastXDest = pSoldier->fPastYDest = FALSE;
                // assign X/Y values back to make sure we are at the center of
                // the tile (to prevent mercs from going through corners of
                // tiles and producing structure data complaints)

                // pSoldier->dXPos = pSoldier->sDestXPos;
                // pSoldier->dYPos = pSoldier->sDestYPos;

                HandleBloodForNewGridNo(pSoldier);

                if (!(gAnimControl[pSoldier->usAnimState].uiFlags & ANIM_SPECIALMOVE) ||
                    pSoldier->sGridNo == pSoldier->sFinalDestination) {
                  // OK, we're at the MIDDLE of a new tile...
                  HandleAtNewGridNo(pSoldier, &fKeepMoving);
                }

                if (gTacticalStatus.bBoxingState != NOT_BOXING &&
                    (gTacticalStatus.bBoxingState == BOXING_WAITING_FOR_PLAYER ||
                     gTacticalStatus.bBoxingState == PRE_BOXING ||
                     gTacticalStatus.bBoxingState == BOXING)) {
                  BoxingMovementCheck(pSoldier);
                }

                // Are we at our final destination?
                if (pSoldier->sFinalDestination == pSoldier->sGridNo) {
                  // Cancel path....
                  pSoldier->usPathIndex = pSoldier->usPathDataSize = 0;

                  // Cancel reverse
                  pSoldier->bReverse = FALSE;

                  // OK, if we are the selected soldier, refresh some UI stuff
                  if (pSoldier == GetSelectedMan()) gfUIRefreshArrows = TRUE;

                  // ATE: Play landing sound.....
                  if (pSoldier->usAnimState == JUMP_OVER_BLOCKING_PERSON) {
                    PlaySoldierFootstepSound(pSoldier);
                  }

                  // If we are a robot, play stop sound...
                  if (pSoldier->uiStatusFlags & SOLDIER_ROBOT) {
                    PlaySoldierJA2Sample(pSoldier, ROBOT_STOP, HIGHVOLUME, 1, TRUE);
                  }

                  // Update to middle if we're on destination
                  const float dXPos = pSoldier->sDestXPos;
                  const float dYPos = pSoldier->sDestYPos;
                  EVENT_SetSoldierPositionXY(pSoldier, dXPos, dYPos, SSP_NONE);

                  // CHECK IF WE HAVE A PENDING ANIMATION
                  if (pSoldier->usPendingAnimation != NO_PENDING_ANIMATION) {
                    ChangeSoldierState(pSoldier, pSoldier->usPendingAnimation, 0, FALSE);
                    pSoldier->usPendingAnimation = NO_PENDING_ANIMATION;

                    if (pSoldier->ubPendingDirection != NO_PENDING_DIRECTION) {
                      EVENT_SetSoldierDesiredDirection(pSoldier, pSoldier->ubPendingDirection);
                      pSoldier->ubPendingDirection = NO_PENDING_DIRECTION;
                    }
                  }

                  // CHECK IF WE HAVE A PENDING ACTION
                  if (pSoldier->ubWaitActionToDo) {
                    if (pSoldier->ubWaitActionToDo == 2) {
                      pSoldier->ubWaitActionToDo = 1;

                      if (gubWaitingForAllMercsToExitCode == WAIT_FOR_MERCS_TO_WALKOFF_SCREEN) {
                        // ATE wanted this line here...
                        pSoldier->usPathIndex--;
                        AdjustSoldierPathToGoOffEdge(pSoldier, pSoldier->sGridNo,
                                                     (uint8_t)pSoldier->uiPendingActionData1);
                        continue;
                      }
                    } else if (pSoldier->ubWaitActionToDo == 1) {
                      pSoldier->ubWaitActionToDo = 0;

                      gbNumMercsUntilWaitingOver--;

                      SoldierGotoStationaryStance(pSoldier);

                      // If we are at an exit-grid, make disappear.....
                      if (gubWaitingForAllMercsToExitCode == WAIT_FOR_MERCS_TO_WALK_TO_GRIDNO) {
                        RemoveSoldierFromTacticalSector(*pSoldier);
                      }
                    }
                  } else if (pSoldier->ubPendingAction != NO_PENDING_ACTION) {
                    DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
                             String("We are inside the IF PENDING Animation "
                                    "with soldier #%d",
                                    pSoldier->ubID));

                    if (pSoldier->ubPendingAction == MERC_OPENDOOR ||
                        pSoldier->ubPendingAction == MERC_OPENSTRUCT) {
                      const int16_t sGridNo = pSoldier->sPendingActionData2;
                      // usStructureID           =
                      // (uint16_t)pSoldier->uiPendingActionData1; pStructure =
                      // FindStructureByID( sGridNo, usStructureID );

                      // LOOK FOR STRUCT OPENABLE
                      STRUCTURE *const pStructure = FindStructure(sGridNo, STRUCTURE_OPENABLE);
                      if (pStructure == NULL) {
                        fKeepMoving = FALSE;
                      } else {
                        if (EnoughPoints(pSoldier, AP_OPEN_DOOR, BP_OPEN_DOOR, TRUE)) {
                          InteractWithOpenableStruct(*pSoldier, *pStructure,
                                                     pSoldier->bPendingActionData3);
                        } else {
                          fNoAPsForPendingAction = TRUE;
                        }
                      }
                    }

                    if (pSoldier->ubPendingAction == MERC_PICKUPITEM) {
                      const int16_t sGridNo = pSoldier->sPendingActionData2;
                      if (sGridNo == pSoldier->sGridNo) {
                        // OK, now, if in realtime
                        if (!(gTacticalStatus.uiFlags & INCOMBAT)) {
                          // If the two gridnos are not the same, check to see
                          // if we can now go into it
                          if (sGridNo != (int16_t)pSoldier->uiPendingActionData4) {
                            if (NewOKDestination(pSoldier, (int16_t)pSoldier->uiPendingActionData4,
                                                 TRUE, pSoldier->bLevel)) {
                              // GOTO NEW TILE!
                              SoldierPickupItem(pSoldier, pSoldier->uiPendingActionData1,
                                                (int16_t)pSoldier->uiPendingActionData4,
                                                pSoldier->bPendingActionData3);
                              continue;
                            }
                          }
                        }

                        PickPickupAnimation(pSoldier, pSoldier->uiPendingActionData1,
                                            (int16_t)pSoldier->uiPendingActionData4,
                                            pSoldier->bPendingActionData3);
                      } else {
                        SoldierGotoStationaryStance(pSoldier);
                      }
                    } else if (pSoldier->ubPendingAction == MERC_PUNCH) {
                      // for the benefit of the AI
                      pSoldier->bAction = AI_ACTION_KNIFE_STAB;

                      EVENT_SoldierBeginPunchAttack(pSoldier, pSoldier->sPendingActionData2,
                                                    pSoldier->bPendingActionData3);
                      pSoldier->ubPendingAction = NO_PENDING_ACTION;
                    } else if (pSoldier->ubPendingAction == MERC_TALK) {
                      PlayerSoldierStartTalking(pSoldier, (uint8_t)pSoldier->uiPendingActionData1,
                                                TRUE);
                      pSoldier->ubPendingAction = NO_PENDING_ACTION;
                    } else if (pSoldier->ubPendingAction == MERC_DROPBOMB) {
                      EVENT_SoldierBeginDropBomb(pSoldier);
                      pSoldier->ubPendingAction = NO_PENDING_ACTION;
                    } else if (pSoldier->ubPendingAction == MERC_STEAL) {
                      // pSoldier->bDesiredDirection =
                      // pSoldier->bPendingActionData3;
                      EVENT_SetSoldierDesiredDirection(pSoldier, pSoldier->bPendingActionData3);

                      EVENT_InitNewSoldierAnim(pSoldier, STEAL_ITEM, 0, FALSE);
                      pSoldier->ubPendingAction = NO_PENDING_ACTION;
                    } else if (pSoldier->ubPendingAction == MERC_KNIFEATTACK) {
                      // for the benefit of the AI
                      pSoldier->bAction = AI_ACTION_KNIFE_STAB;

                      EVENT_SoldierBeginBladeAttack(pSoldier, pSoldier->sPendingActionData2,
                                                    pSoldier->bPendingActionData3);
                      pSoldier->ubPendingAction = NO_PENDING_ACTION;
                    } else if (pSoldier->ubPendingAction == MERC_GIVEAID) {
                      EVENT_SoldierBeginFirstAid(pSoldier, pSoldier->sPendingActionData2,
                                                 pSoldier->bPendingActionData3);
                      pSoldier->ubPendingAction = NO_PENDING_ACTION;
                    } else if (pSoldier->ubPendingAction == MERC_REPAIR) {
                      EVENT_SoldierBeginRepair(*pSoldier, pSoldier->sPendingActionData2,
                                               pSoldier->bPendingActionData3);
                      pSoldier->ubPendingAction = NO_PENDING_ACTION;
                    } else if (pSoldier->ubPendingAction == MERC_FUEL_VEHICLE) {
                      EVENT_SoldierBeginRefuel(pSoldier, pSoldier->sPendingActionData2,
                                               pSoldier->bPendingActionData3);
                      pSoldier->ubPendingAction = NO_PENDING_ACTION;
                    } else if (pSoldier->ubPendingAction == MERC_RELOADROBOT) {
                      EVENT_SoldierBeginReloadRobot(pSoldier, pSoldier->sPendingActionData2,
                                                    pSoldier->bPendingActionData3,
                                                    (int8_t)pSoldier->uiPendingActionData1);
                      pSoldier->ubPendingAction = NO_PENDING_ACTION;
                    } else if (pSoldier->ubPendingAction == MERC_TAKEBLOOD) {
                      EVENT_SoldierBeginTakeBlood(pSoldier, pSoldier->sPendingActionData2,
                                                  pSoldier->bPendingActionData3);
                      pSoldier->ubPendingAction = NO_PENDING_ACTION;
                    } else if (pSoldier->ubPendingAction == MERC_ATTACH_CAN) {
                      EVENT_SoldierBeginAttachCan(pSoldier, pSoldier->sPendingActionData2,
                                                  pSoldier->bPendingActionData3);
                      pSoldier->ubPendingAction = NO_PENDING_ACTION;
                    } else if (pSoldier->ubPendingAction == MERC_ENTER_VEHICLE) {
                      EVENT_SoldierEnterVehicle(*pSoldier, pSoldier->sPendingActionData2);
                      pSoldier->ubPendingAction = NO_PENDING_ACTION;
                      continue;
                    } else if (pSoldier->ubPendingAction == MERC_CUTFFENCE) {
                      EVENT_SoldierBeginCutFence(pSoldier, pSoldier->sPendingActionData2,
                                                 pSoldier->bPendingActionData3);
                      pSoldier->ubPendingAction = NO_PENDING_ACTION;
                    } else if (pSoldier->ubPendingAction == MERC_GIVEITEM) {
                      EVENT_SoldierBeginGiveItem(pSoldier);
                      pSoldier->ubPendingAction = NO_PENDING_ACTION;
                    }

                    if (fNoAPsForPendingAction) {
                      // Change status of guy to waiting
                      HaltMoveForSoldierOutOfPoints(*pSoldier);
                      fKeepMoving = FALSE;
                      pSoldier->usPendingAnimation = NO_PENDING_ANIMATION;
                      pSoldier->ubPendingDirection = NO_PENDING_DIRECTION;
                    }
                  } else {
                    // OK, ADJUST TO STANDING, WE ARE DONE
                    // DO NOTHING IF WE ARE UNCONSCIOUS
                    if (pSoldier->bLife >= OKLIFE) {
                      if (pSoldier->ubBodyType == CROW) {
                        // If we are flying, don't stop!
                        if (pSoldier->sHeightAdjustment == 0) {
                          SoldierGotoStationaryStance(pSoldier);
                        }
                      } else {
                        UnSetUIBusy(pSoldier);
                        SoldierGotoStationaryStance(pSoldier);
                      }
                    }
                  }

                  // RESET MOVE FAST FLAG
                  if (pSoldier->ubProfile == NO_PROFILE) {
                    pSoldier->fUIMovementFast = FALSE;
                  }

                  // if AI moving and waiting to process something at end of
                  // move, have them handled the very next frame
                  if (pSoldier->ubQuoteActionID == QUOTE_ACTION_ID_CHECKFORDEST) {
                    pSoldier->fAIFlags |= AI_HANDLE_EVERY_FRAME;
                  }

                  fKeepMoving = FALSE;
                } else if (!pSoldier->fNoAPToFinishMove) {
                  // Increment path....
                  pSoldier->usPathIndex++;
                  if (pSoldier->usPathIndex > pSoldier->usPathDataSize) {
                    pSoldier->usPathIndex = pSoldier->usPathDataSize;
                  }

                  // Are we at the end?
                  if (pSoldier->usPathIndex == pSoldier->usPathDataSize) {
                    // ATE: Pop up warning....
                    if (pSoldier->usPathDataSize != MAX_PATH_LIST_SIZE) {
                    }

                    // In case this is an AI person with the path-stored flag
                    // set, turn it OFF since we have exhausted our stored path
                    pSoldier->bPathStored = FALSE;
                    if (pSoldier->sAbsoluteFinalDestination != NOWHERE) {
                      // We have not made it to our dest... but it's better to
                      // let the AI handle this itself, on the very next fram
                      pSoldier->fAIFlags |= AI_HANDLE_EVERY_FRAME;
                    } else {
                      // ATE: Added this to fcalilitate the fact
                      // that our final dest may now have people on it....
                      if (FindBestPath(pSoldier, pSoldier->sFinalDestination, pSoldier->bLevel,
                                       pSoldier->usUIMovementMode, NO_COPYROUTE,
                                       PATH_THROUGH_PEOPLE) != 0) {
                        const int16_t sNewGridNo =
                            NewGridNo(pSoldier->sGridNo, DirectionInc((uint8_t)guiPathingData[0]));
                        SetDelayedTileWaiting(pSoldier, sNewGridNo, 1);
                      }

                      // We have not made it to our dest... set flag that we are
                      // waiting....
                      if (!EVENT_InternalGetNewSoldierPath(pSoldier, pSoldier->sFinalDestination,
                                                           pSoldier->usUIMovementMode, 2, FALSE)) {
                        // ATE: To do here.... we could not get path, so we have
                        // to stop
                        SoldierGotoStationaryStance(pSoldier);
                        continue;
                      }
                    }
                  } else {
                    // OK, Now find another dest grindo....
                    if (!(gAnimControl[pSoldier->usAnimState].uiFlags & ANIM_SPECIALMOVE)) {
                      // OK, now we want to see if we can continue to another
                      // tile...
                      if (!HandleGotoNewGridNo(pSoldier, &fKeepMoving, FALSE,
                                               pSoldier->usAnimState)) {
                        continue;
                      }
                    } else {
                      // Change desired direction
                      // Just change direction
                      EVENT_InternalSetSoldierDestination(
                          pSoldier, pSoldier->usPathingData[pSoldier->usPathIndex], FALSE,
                          pSoldier->usAnimState);
                    }

                    if (gTacticalStatus.bBoxingState != NOT_BOXING &&
                        (gTacticalStatus.bBoxingState == BOXING_WAITING_FOR_PLAYER ||
                         gTacticalStatus.bBoxingState == PRE_BOXING ||
                         gTacticalStatus.bBoxingState == BOXING)) {
                      BoxingMovementCheck(pSoldier);
                    }
                  }
                }
              }

              if (pSoldier->uiStatusFlags & SOLDIER_PAUSEANIMOVE) {
                fKeepMoving = FALSE;
              }

              // DO WALKING
              if (!pSoldier->fPausedMove && fKeepMoving) {
                // Determine deltas
                //	dDeltaX = pSoldier->sDestXPos - pSoldier->dXPos;
                // dDeltaY = pSoldier->sDestYPos - pSoldier->dYPos;

                // Determine angle
                //	dAngle = (float)atan2( dDeltaX, dDeltaY );

                static const float gdRadiansForAngle[] = {
                    (float)PI, (float)(PI * 3 / 4), (float)(PI / 2),  (float)(PI / 4),

                    0,         (float)(-PI / 4),    (float)(-PI / 2), (float)(-PI * 3 / 4),
                };
                const float dAngle = gdRadiansForAngle[pSoldier->bMovementDirection];

                // For walking, base it on body type!
                if (pSoldier->usAnimState == WALKING) {
                  MoveMerc(pSoldier, gubAnimWalkSpeeds[pSoldier->ubBodyType].dMovementChange,
                           dAngle, TRUE);
                } else {
                  MoveMerc(pSoldier, gAnimControl[pSoldier->usAnimState].dMovementChange, dAngle,
                           TRUE);
                }
              }
            }

            // Check for direction change
            if (gAnimControl[pSoldier->usAnimState].uiFlags & ANIM_TURNING) {
              TurnSoldier(pSoldier);
            }
          }
        }

        if (!gfPauseAllAI &&
            ((gTacticalStatus.uiFlags & TURNBASED && (gTacticalStatus.uiFlags & INCOMBAT)) ||
             (fHandleAI && guiAISlotToHandle == cnt) ||
             pSoldier->fAIFlags & AI_HANDLE_EVERY_FRAME || gTacticalStatus.fAutoBandageMode)) {
          HandleSoldierAI(pSoldier);
          if (!(gTacticalStatus.uiFlags & TURNBASED) || !(gTacticalStatus.uiFlags & INCOMBAT)) {
            if (GetJA2Clock() - iTimerVal > RT_AI_TIMESLICE) {
              // don't do any more AI this time!
              fHandleAI = FALSE;
            } else {
              // we still have time to handle AI; skip to the next person
              fHandleAI = NextAIToHandle(guiAISlotToHandle);
            }
          }
        }
      }
    }

    if (guiNumAwaySlots > 0 && !gfPauseAllAI && !(gTacticalStatus.uiFlags & INCOMBAT) &&
        guiAISlotToHandle == HANDLE_OFF_MAP_MERC &&
        guiAIAwaySlotToHandle != RESET_HANDLE_OF_OFF_MAP_MERCS) {
      // Syncronize for upcoming soldier counters
      SYNCTIMECOUNTER();

      // the ONLY thing to do with away soldiers is process their schedule if
      // they have one and there is an action for them to do (like go on-sector)
      SOLDIERTYPE *const pSoldier = AwaySlots[guiAIAwaySlotToHandle];
      if (pSoldier != NULL && pSoldier->fAIFlags & AI_CHECK_SCHEDULE) {
        HandleSoldierAI(pSoldier);
      }
    }

    // Turn off auto bandage if we need to...
    if (gTacticalStatus.fAutoBandageMode && !CanAutoBandage(TRUE)) {
      SetAutoBandageComplete();
    }

    // Check if we should be doing a special event once guys get to a
    // location...
    if (gubWaitingForAllMercsToExitCode != 0) {
      // Check if we have gone past our time...
      if (GetJA2Clock() - guiWaitingForAllMercsToExitTimer > 2500) {
        // OK, set num waiting to 0
        gbNumMercsUntilWaitingOver = 0;

        // Reset all waitng codes
        FOR_EACH_MERC(i)(*i)->ubWaitActionToDo = 0;
      }

      if (gbNumMercsUntilWaitingOver == 0) {
        // ATE: Unset flag to ignore sight...
        gTacticalStatus.uiFlags &= ~DISALLOW_SIGHT;

        // OK cheif, do something here....
        switch (gubWaitingForAllMercsToExitCode) {
          case WAIT_FOR_MERCS_TO_WALKOFF_SCREEN:
            if (gTacticalStatus.ubCurrentTeam == OUR_TEAM) {
              guiPendingOverrideEvent = LU_ENDUILOCK;
              HandleTacticalUI();
            }
            AllMercsHaveWalkedOffSector();
            break;

          case WAIT_FOR_MERCS_TO_WALKON_SCREEN:
            // OK, unset UI
            if (gTacticalStatus.ubCurrentTeam == OUR_TEAM) {
              guiPendingOverrideEvent = LU_ENDUILOCK;
              HandleTacticalUI();
            }
            break;

          case WAIT_FOR_MERCS_TO_WALK_TO_GRIDNO:
            // OK, unset UI
            if (gTacticalStatus.ubCurrentTeam == OUR_TEAM) {
              guiPendingOverrideEvent = LU_ENDUILOCK;
              HandleTacticalUI();
            }
            AllMercsWalkedToExitGrid();
            break;
        }

        // ATE; Turn off tactical status flag...
        gTacticalStatus.uiFlags &= ~IGNORE_ALL_OBSTACLES;
        gubWaitingForAllMercsToExitCode = 0;
      }
    }
  }

  // reset these AI-related global variables to 0 to ensure they don't interfere
  // with the UI
  gubNPCAPBudget = 0;
  gubNPCDistLimit = 0;
}

static void HaltGuyFromNewGridNoBecauseOfNoAPs(SOLDIERTYPE &s) {
  HaltMoveForSoldierOutOfPoints(s);
  s.usPendingAnimation = NO_PENDING_ANIMATION;
  s.ubPendingDirection = NO_PENDING_DIRECTION;
  s.ubPendingAction = NO_PENDING_ACTION;

  UnMarkMovementReserved(s);

  // Display message if our merc
  if (s.bTeam == OUR_TEAM && gTacticalStatus.uiFlags & INCOMBAT) {
    ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, TacticalStr[GUY_HAS_RUN_OUT_OF_APS_STR], s.name);
  }

  UnSetUIBusy(&s);
  // Unset engaged in CONV, something changed
  UnSetEngagedInConvFromPCAction(&s);
}

static void HandleLocateToGuyAsHeWalks(SOLDIERTYPE *pSoldier) {
  // Our guys if option set,
  if (pSoldier->bTeam == OUR_TEAM) {
    // IF tracking on, center on guy....
    if (gGameSettings.fOptions[TOPTION_TRACKING_MODE]) {
      LocateSoldier(pSoldier, FALSE);
    }
  } else {
    // Others if visible...
    if (pSoldier->bVisible != -1) {
      // ATE: If we are visible, and have not already removed roofs, goforit
      if (pSoldier->bLevel > 0) {
        if (!(gTacticalStatus.uiFlags & SHOW_ALL_ROOFS)) {
          gTacticalStatus.uiFlags |= SHOW_ALL_ROOFS;
          SetRenderFlags(RENDER_FLAG_FULL);
        }
      }
      LocateSoldier(pSoldier, FALSE);
    }
  }
}

static void CheckIfNearbyGroundSeemsWrong(SOLDIERTYPE *const s, uint16_t const gridno,
                                          BOOLEAN const check_around, BOOLEAN *const keep_moving) {
  int16_t mine_gridno;
  if (!NearbyGroundSeemsWrong(s, gridno, check_around, &mine_gridno)) return;

  if (s->uiStatusFlags & SOLDIER_PC) { /* NearbyGroundSeemsWrong() returns true with gridno NOWHERE
                                        * if we find something by metal detector.  We should
                                        * definitely stop but we won't place a locator or say
                                        * anything */
    if (gTacticalStatus.uiFlags & INCOMBAT) {
      EVENT_StopMerc(s);
    } else {  // Not in combat, stop them all
      for (int32_t i = gTacticalStatus.Team[OUR_TEAM].bLastID;
           i >= gTacticalStatus.Team[OUR_TEAM].bFirstID; i--) {
        SOLDIERTYPE &s2 = GetMan(i);
        if (!s2.bActive) continue;
        EVENT_StopMerc(&s2);
      }
    }

    *keep_moving = FALSE;

    if (mine_gridno == NOWHERE) return;

    // We reuse the boobytrap gridno variable here
    gpBoobyTrapSoldier = s;
    gsBoobyTrapGridNo = mine_gridno;
    LocateGridNo(mine_gridno);
    SetStopTimeQuoteCallback(MineSpottedDialogueCallBack);
    TacticalCharacterDialogue(s, QUOTE_SUSPICIOUS_GROUND);
  } else {
    if (mine_gridno == NOWHERE) return;

    EVENT_StopMerc(s);
    *keep_moving = FALSE;

    gpWorldLevelData[mine_gridno].uiFlags |= MAPELEMENT_ENEMY_MINE_PRESENT;

    // Better stop and reconsider what to do
    SetNewSituation(s);
    ActionDone(s);
  }
}

BOOLEAN HandleGotoNewGridNo(SOLDIERTYPE *pSoldier, BOOLEAN *pfKeepMoving, BOOLEAN fInitialMove,
                            uint16_t usAnimState) {
  if (gTacticalStatus.uiFlags & INCOMBAT && fInitialMove) {
    HandleLocateToGuyAsHeWalks(pSoldier);
  }

  // Default to TRUE
  *pfKeepMoving = TRUE;

  // Check for good breath....
  // if ( pSoldier->bBreath < OKBREATH && !fInitialMove )
  if (pSoldier->bBreath < OKBREATH) {
    // OK, first check for b== 0
    // If our currentd state is moving already....( this misses the first tile,
    // so the user Sees some change in their click, but just one tile
    if (pSoldier->bBreath == 0) {
      // Collapse!
      pSoldier->bBreathCollapsed = TRUE;
      pSoldier->bEndDoorOpenCode = FALSE;

      if (fInitialMove) UnSetUIBusy(pSoldier);

      DebugMsg(TOPIC_JA2, DBG_LEVEL_3, "HandleGotoNewGridNo() Failed: Out of Breath");
      return FALSE;
    }

    // OK, if we are collapsed now, check for OK breath instead...
    if (pSoldier->bCollapsed) {
      // Collapse!
      DebugMsg(TOPIC_JA2, DBG_LEVEL_3, "HandleGotoNewGridNo() Failed: Has Collapsed");
      pSoldier->bBreathCollapsed = TRUE;
      pSoldier->bEndDoorOpenCode = FALSE;
      return FALSE;
    }
  }

  const uint16_t usNewGridNo = NewGridNo(
      pSoldier->sGridNo, DirectionInc((uint8_t)pSoldier->usPathingData[pSoldier->usPathIndex]));

  // OK, check if this is a fence cost....
  if (gubWorldMovementCosts[usNewGridNo][(uint8_t)pSoldier->usPathingData[pSoldier->usPathIndex]]
                           [pSoldier->bLevel] == TRAVELCOST_FENCE) {
    // We have been told to jump fence....

    // Do we have APs?
    const int16_t sAPCost = AP_JUMPFENCE;
    const int16_t sBPCost = BP_JUMPFENCE;

    if (EnoughPoints(pSoldier, sAPCost, sBPCost, FALSE)) {
      // ATE: Check for tile being clear....
      const uint16_t sOverFenceGridNo = NewGridNo(
          usNewGridNo, DirectionInc((uint8_t)pSoldier->usPathingData[pSoldier->usPathIndex + 1]));

      if (HandleNextTile(pSoldier, (int8_t)pSoldier->usPathingData[pSoldier->usPathIndex + 1],
                         sOverFenceGridNo, pSoldier->sFinalDestination)) {
        // We do, adjust path data....
        pSoldier->usPathIndex++;
        // We go two, because we really want to start moving towards the NEXT
        // gridno, if we have any...

        // LOCK PENDING ACTION COUNTER
        pSoldier->uiStatusFlags |= SOLDIER_LOCKPENDINGACTIONCOUNTER;

        SoldierGotoStationaryStance(pSoldier);

        // OK, jump!
        BeginSoldierClimbFence(pSoldier);

        pSoldier->fContinueMoveAfterStanceChange = 2;
      }
    } else {
      HaltGuyFromNewGridNoBecauseOfNoAPs(*pSoldier);
      *pfKeepMoving = FALSE;
    }

    return FALSE;
  } else if (InternalDoorTravelCost(
                 pSoldier, usNewGridNo,
                 gubWorldMovementCosts[usNewGridNo]
                                      [(uint8_t)pSoldier->usPathingData[pSoldier->usPathIndex]]
                                      [pSoldier->bLevel],
                 pSoldier->bTeam == OUR_TEAM, NULL, TRUE) == TRAVELCOST_DOOR) {
    // OK, if we are here, we have been told to get a pth through a door.

    // No need to check if for AI

    // No need to check for right key ( since the path checks for that? )

    // Just for now play the $&&% animation
    const int8_t bDirection = pSoldier->usPathingData[pSoldier->usPathIndex];

    // OK, based on the direction, get door gridno
    int16_t sDoorGridNo;
    if (bDirection == NORTH || bDirection == WEST) {
      sDoorGridNo = NewGridNo(
          pSoldier->sGridNo, DirectionInc((uint8_t)pSoldier->usPathingData[pSoldier->usPathIndex]));
    } else if (bDirection == SOUTH || bDirection == EAST) {
      sDoorGridNo = pSoldier->sGridNo;
    } else {
      DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
               "HandleGotoNewGridNo() Failed: Open door - invalid approach "
               "direction");

      HaltGuyFromNewGridNoBecauseOfNoAPs(*pSoldier);
      pSoldier->bEndDoorOpenCode = FALSE;
      (*pfKeepMoving) = FALSE;
      return (FALSE);
    }

    // Get door
    STRUCTURE *const pStructure = FindStructure(sDoorGridNo, STRUCTURE_ANYDOOR);
    if (pStructure == NULL) {
      DebugMsg(TOPIC_JA2, DBG_LEVEL_3, "HandleGotoNewGridNo() Failed: Door does not exist");
      HaltGuyFromNewGridNoBecauseOfNoAPs(*pSoldier);
      pSoldier->bEndDoorOpenCode = FALSE;
      *pfKeepMoving = FALSE;
      return FALSE;
    }

    // OK, open!
    StartInteractiveObject(sDoorGridNo, *pStructure, *pSoldier, bDirection);
    InteractWithOpenableStruct(*pSoldier, *pStructure, bDirection);

    // One needs to walk after....
    if (pSoldier->bTeam != OUR_TEAM || gTacticalStatus.fAutoBandageMode ||
        pSoldier->uiStatusFlags & SOLDIER_PCUNDERAICONTROL) {
      pSoldier->bEndDoorOpenCode = 1;
      pSoldier->sEndDoorOpenCodeData = sDoorGridNo;
    }
    *pfKeepMoving = FALSE;
    return FALSE;
  }

  // Find out how much it takes to move here!
  const int16_t sAPCost = ActionPointCost(
      pSoldier, usNewGridNo, (int8_t)pSoldier->usPathingData[pSoldier->usPathIndex], usAnimState);
  const int16_t sBPCost = TerrainBreathPoints(
      pSoldier, usNewGridNo, (int8_t)pSoldier->usPathingData[pSoldier->usPathIndex], usAnimState);

  // CHECK IF THIS TILE IS A GOOD ONE!
  if (!HandleNextTile(pSoldier, (int8_t)pSoldier->usPathingData[pSoldier->usPathIndex], usNewGridNo,
                      pSoldier->sFinalDestination)) {
    DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
             String("HandleGotoNewGridNo() Failed: Tile %d Was blocked", usNewGridNo));

    // ATE: If our own guy and an initial move.. display message
    // if ( fInitialMove && pSoldier->bTeam == OUR_TEAM  )
    //{
    //	ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, TacticalStr[
    // NO_PATH_FOR_MERC ], pSoldier->name );
    //}

    pSoldier->bEndDoorOpenCode = FALSE;
    // GO on to next guy!
    return FALSE;
  }

  // just check the tile we're going to walk into
  CheckIfNearbyGroundSeemsWrong(pSoldier, usNewGridNo, FALSE, pfKeepMoving);

  // ATE: Check if we have sighted anyone, if so, don't do anything else...
  // IN other words, we have stopped from sighting...
  if (pSoldier->fNoAPToFinishMove && !fInitialMove) {
    DebugMsg(TOPIC_JA2, DBG_LEVEL_3, "HandleGotoNewGridNo() Failed: No APs to finish move set");
    pSoldier->bEndDoorOpenCode = FALSE;
    *pfKeepMoving = FALSE;
  } else if (pSoldier->usPathIndex == pSoldier->usPathDataSize && pSoldier->usPathDataSize == 0) {
    DebugMsg(TOPIC_JA2, DBG_LEVEL_3, "HandleGotoNewGridNo() Failed: No Path");
    pSoldier->bEndDoorOpenCode = FALSE;
    *pfKeepMoving = FALSE;
  }
  // else if ( gTacticalStatus.fEnemySightingOnTheirTurn )
  //{
  // Hault guy!
  //	AdjustNoAPToFinishMove( pSoldier, TRUE );
  //	(*pfKeepMoving ) = FALSE;
  //}
  else if (EnoughPoints(pSoldier, sAPCost, 0, FALSE)) {
    BOOLEAN fDontContinue = FALSE;

    if (pSoldier->usPathIndex > 0) {
      // check for running into gas

      // note: this will have to use the minimum types of structures for
      // tear/creature gas since there isn't a way to retrieve the smoke effect
      // structure
      if (gpWorldLevelData[pSoldier->sGridNo].ubExtFlags[pSoldier->bLevel] & ANY_SMOKE_EFFECT &&
          PreRandom(5) == 0) {
        int8_t bPosOfMask;
        if (pSoldier->inv[HEAD1POS].usItem == GASMASK &&
            pSoldier->inv[HEAD1POS].bStatus[0] >= GASMASK_MIN_STATUS) {
          bPosOfMask = HEAD1POS;
        } else if (pSoldier->inv[HEAD2POS].usItem == GASMASK &&
                   pSoldier->inv[HEAD2POS].bStatus[0] >= GASMASK_MIN_STATUS) {
          bPosOfMask = HEAD2POS;
        } else {
          bPosOfMask = NO_SLOT;
        }

        EXPLOSIVETYPE const *pExplosive = 0;
        if (!AM_A_ROBOT(pSoldier)) {
          if (gpWorldLevelData[pSoldier->sGridNo].ubExtFlags[pSoldier->bLevel] &
              MAPELEMENT_EXT_TEARGAS) {
            if (!(pSoldier->fHitByGasFlags & HIT_BY_TEARGAS) && bPosOfMask == NO_SLOT) {
              // check for gas mask
              pExplosive = &Explosive[Item[TEARGAS_GRENADE].ubClassIndex];
            }
          }
          if (gpWorldLevelData[pSoldier->sGridNo].ubExtFlags[pSoldier->bLevel] &
              MAPELEMENT_EXT_MUSTARDGAS) {
            if (!(pSoldier->fHitByGasFlags & HIT_BY_MUSTARDGAS) && bPosOfMask == NO_SLOT) {
              pExplosive = &Explosive[Item[MUSTARD_GRENADE].ubClassIndex];
            }
          }
        }
        if (gpWorldLevelData[pSoldier->sGridNo].ubExtFlags[pSoldier->bLevel] &
            MAPELEMENT_EXT_CREATUREGAS) {
          if (!(pSoldier->fHitByGasFlags &
                HIT_BY_CREATUREGAS))  // gas mask doesn't help vs creaturegas
          {
            pExplosive = &Explosive[Item[SMALL_CREATURE_GAS].ubClassIndex];
          }
        }
        if (pExplosive) {
          EVENT_StopMerc(pSoldier);
          fDontContinue = TRUE;
          DishOutGasDamage(
              pSoldier, pExplosive, TRUE, FALSE,
              pExplosive->ubDamage + PreRandom(pExplosive->ubDamage),
              100 * (pExplosive->ubStunDamage + PreRandom(pExplosive->ubStunDamage / 2)), NULL);
        }
      }

      if (!fDontContinue) {
        if ((pSoldier->bOverTerrainType == FLAT_FLOOR ||
             pSoldier->bOverTerrainType == PAVED_ROAD) &&
            pSoldier->bLevel == 0) {
          int32_t iMarblesIndex;
          if (ItemTypeExistsAtLocation(pSoldier->sGridNo, MARBLES, 0, &iMarblesIndex)) {
            // Slip on marbles!
            DoMercBattleSound(pSoldier, BATTLE_SOUND_CURSE1);
            if (pSoldier->bTeam == OUR_TEAM) {
              ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK,
                        g_langRes->Message[STR_SLIPPED_MARBLES], pSoldier->name);
            }
            RemoveItemFromPool(&GetWorldItem(iMarblesIndex));
            SoldierCollapse(pSoldier);
            if (pSoldier->bActionPoints > 0) {
              pSoldier->bActionPoints -= (int8_t)(Random(pSoldier->bActionPoints) + 1);
            }
            return FALSE;
          }
        }

        if (pSoldier->bBlindedCounter > 0 && pSoldier->usAnimState == RUNNING && Random(5) == 0 &&
            OKFallDirection(pSoldier, pSoldier->sGridNo + DirectionInc(pSoldier->bDirection),
                            pSoldier->bLevel, pSoldier->bDirection, pSoldier->usAnimState)) {
          // 20% chance of falling over!
          DoMercBattleSound(pSoldier, BATTLE_SOUND_CURSE1);
          ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, gzLateLocalizedString[STR_LATE_37],
                    pSoldier->name);
          SoldierCollapse(pSoldier);
          if (pSoldier->bActionPoints > 0) {
            pSoldier->bActionPoints -= (int8_t)(Random(pSoldier->bActionPoints) + 1);
          }
          return FALSE;
        } else if (GetDrunkLevel(pSoldier) == DRUNK && (Random(5) == 0) &&
                   OKFallDirection(pSoldier, pSoldier->sGridNo + DirectionInc(pSoldier->bDirection),
                                   pSoldier->bLevel, pSoldier->bDirection, pSoldier->usAnimState)) {
          // 20% chance of falling over!
          DoMercBattleSound(pSoldier, BATTLE_SOUND_CURSE1);
          ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, gzLateLocalizedString[STR_LATE_37],
                    pSoldier->name);
          SoldierCollapse(pSoldier);
          if (pSoldier->bActionPoints > 0) {
            pSoldier->bActionPoints -= (int8_t)(Random(pSoldier->bActionPoints) + 1);
          }
          return FALSE;
        } else if (pSoldier->bTeam == OUR_TEAM && pSoldier->ubProfile != NO_PROFILE &&
                   gMercProfiles[pSoldier->ubProfile].bPersonalityTrait == FORGETFUL) {
          // ATE; First check for profile
          // Forgetful guy might forget his path
          if (pSoldier->ubNumTilesMovesSinceLastForget < 255) {
            pSoldier->ubNumTilesMovesSinceLastForget++;
          }

          if (pSoldier->usPathIndex > 2 && Random(100) == 0 &&
              pSoldier->ubNumTilesMovesSinceLastForget > 200) {
            pSoldier->ubNumTilesMovesSinceLastForget = 0;

            TacticalCharacterDialogue(pSoldier, QUOTE_PERSONALITY_TRAIT);
            EVENT_StopMerc(pSoldier);
            if (pSoldier->bActionPoints > 0) {
              pSoldier->bActionPoints -= (int8_t)(Random(pSoldier->bActionPoints) + 1);
            }

            fDontContinue = TRUE;
            UnSetUIBusy(pSoldier);
          }
        }
      }
    }

    if (!fDontContinue) {
      // Don't apply the first deduction in points...
      if (usAnimState == CRAWLING && pSoldier->fTurningFromPronePosition > 1) {
      } else {
        // Adjust AP/Breathing points to move
        DeductPoints(pSoldier, sAPCost, sBPCost);
      }

      // OK, let's check for monsters....
      if (pSoldier->uiStatusFlags & SOLDIER_MONSTER) {
        if (!ValidCreatureTurn(pSoldier, (int8_t)pSoldier->usPathingData[pSoldier->usPathIndex])) {
          if (!pSoldier->bReverse) {
            pSoldier->bReverse = TRUE;

            if (pSoldier->ubBodyType == INFANT_MONSTER) {
              ChangeSoldierState(pSoldier, WALK_BACKWARDS, 1, TRUE);
            } else {
              ChangeSoldierState(pSoldier, MONSTER_WALK_BACKWARDS, 1, TRUE);
            }
          }
        } else {
          pSoldier->bReverse = FALSE;
        }
      }

      // OK, let's check for monsters....
      if (pSoldier->ubBodyType == BLOODCAT) {
        if (!ValidCreatureTurn(pSoldier, (int8_t)pSoldier->usPathingData[pSoldier->usPathIndex])) {
          if (!pSoldier->bReverse) {
            pSoldier->bReverse = TRUE;
            ChangeSoldierState(pSoldier, BLOODCAT_WALK_BACKWARDS, 1, TRUE);
          }
        } else {
          pSoldier->bReverse = FALSE;
        }
      }

      // Change desired direction
      EVENT_InternalSetSoldierDestination(pSoldier, pSoldier->usPathingData[pSoldier->usPathIndex],
                                          fInitialMove, usAnimState);

      // CONTINUE
      // IT'S SAVE TO GO AGAIN, REFRESH flag
      AdjustNoAPToFinishMove(pSoldier, FALSE);
    }
  } else {
    // HALT GUY HERE
    DebugMsg(
        TOPIC_JA2, DBG_LEVEL_3,
        String("HandleGotoNewGridNo() Failed: No APs %d %d", sAPCost, pSoldier->bActionPoints));
    HaltGuyFromNewGridNoBecauseOfNoAPs(*pSoldier);
    pSoldier->bEndDoorOpenCode = FALSE;
    *pfKeepMoving = FALSE;
  }

  return TRUE;
}

static void HandleMaryArrival(SOLDIERTYPE *pSoldier) {
  if (!pSoldier) {
    pSoldier = FindSoldierByProfileIDOnPlayerTeam(MARY);
    if (!pSoldier) return;
  }

  if (CheckFact(FACT_JOHN_ALIVE, 0)) return;

  // new requirements: player close by
  else if (PythSpacesAway(pSoldier->sGridNo, 8228) < 40) {
    int16_t sDist;
    if (ClosestPC(pSoldier, &sDist) != NOWHERE && sDist > NPC_TALK_RADIUS * 2) {
      // too far away
      return;
    }

    // Mary has arrived
    SetFactTrue(FACT_MARY_OR_JOHN_ARRIVED);
    EVENT_StopMerc(pSoldier);
    TriggerNPCRecord(MARY, 13);
  }
}

static void HandleJohnArrival(SOLDIERTYPE *pSoldier) {
  if (!pSoldier) {
    pSoldier = FindSoldierByProfileIDOnPlayerTeam(JOHN);
    if (!pSoldier) return;
  }

  if (PythSpacesAway(pSoldier->sGridNo, 8228) < 40) {
    int16_t sDist;
    if (ClosestPC(pSoldier, &sDist) != NOWHERE && sDist > NPC_TALK_RADIUS * 2) {
      // too far away
      return;
    }

    SOLDIERTYPE *pSoldier2 = NULL;
    if (CheckFact(FACT_MARY_ALIVE, 0)) {
      pSoldier2 = FindSoldierByProfileID(MARY);
      if (pSoldier2) {
        if (PythSpacesAway(pSoldier->sGridNo, pSoldier2->sGridNo) > 8) {
          // too far away!
          return;
        }
      }
    }

    SetFactTrue(FACT_MARY_OR_JOHN_ARRIVED);

    EVENT_StopMerc(pSoldier);

    // if Mary is alive/dead
    if (pSoldier2) {
      EVENT_StopMerc(pSoldier2);
      TriggerNPCRecord(JOHN, 13);
    } else {
      TriggerNPCRecord(JOHN, 12);
    }
  }
}

static BOOLEAN HandleAtNewGridNo(SOLDIERTYPE *pSoldier, BOOLEAN *pfKeepMoving) {
  // ATE; Handle bad guys, as they fade, to cancel it if
  // too long...
  // ONLY if fading IN!
  if (pSoldier->fBeginFade == 1) {
    if (pSoldier->sLocationOfFadeStart != pSoldier->sGridNo) {
      // Turn off
      pSoldier->fBeginFade = FALSE;

      if (pSoldier->bLevel > 0 && gpWorldLevelData[pSoldier->sGridNo].pRoofHead != NULL) {
        pSoldier->ubFadeLevel = gpWorldLevelData[pSoldier->sGridNo].pRoofHead->ubShadeLevel;
      } else {
        pSoldier->ubFadeLevel = gpWorldLevelData[pSoldier->sGridNo].pLandHead->ubShadeLevel;
      }

      // Set levelnode shade level....
      if (pSoldier->pLevelNode) {
        pSoldier->pLevelNode->ubShadeLevel = pSoldier->ubFadeLevel;
      }
      pSoldier->bVisible = -1;
    }
  }

  if (gTacticalStatus.uiFlags & INCOMBAT) HandleLocateToGuyAsHeWalks(pSoldier);

  // Default to TRUE
  *pfKeepMoving = TRUE;

  pSoldier->bTilesMoved++;
  if (pSoldier->usAnimState == RUNNING) {
    // count running as double
    pSoldier->bTilesMoved++;
  }

  // First if we are in realtime combat or noncombat
  if (gTacticalStatus.uiFlags & REALTIME || !(gTacticalStatus.uiFlags & INCOMBAT)) {
    // Update value for RT breath update
    pSoldier->ubTilesMovedPerRTBreathUpdate++;
    // Update last anim
    pSoldier->usLastMovementAnimPerRTBreathUpdate = pSoldier->usAnimState;
  }

  // Update path if showing path in RT
  if (gGameSettings.fOptions[TOPTION_ALWAYS_SHOW_MOVEMENT_PATH] &&
      !(gTacticalStatus.uiFlags & INCOMBAT)) {
    gfPlotNewMovement = TRUE;
  }

  // ATE: Put some stuff in here to not handle certain things if we are
  // trversing...
  if (gubWaitingForAllMercsToExitCode == WAIT_FOR_MERCS_TO_WALKOFF_SCREEN ||
      gubWaitingForAllMercsToExitCode == WAIT_FOR_MERCS_TO_WALK_TO_GRIDNO) {
    return TRUE;
  }

  // Check if they are out of breath
  if (CheckForBreathCollapse(*pSoldier)) {
    *pfKeepMoving = TRUE;
    return FALSE;
  }

  // see if a mine gets set off...
  if (SetOffBombsInGridNo(pSoldier, pSoldier->sGridNo, FALSE, pSoldier->bLevel)) {
    *pfKeepMoving = FALSE;
    EVENT_StopMerc(pSoldier);
    return FALSE;
  }

  // Set "interrupt occurred" flag to false so that we will know whether *this
  // particular call* to HandleSight caused an interrupt
  gTacticalStatus.fInterruptOccurred = FALSE;

  if (!(gTacticalStatus.uiFlags & LOADING_SAVED_GAME)) {
    const uint8_t ubVolume = MovementNoise(pSoldier);
    if (ubVolume > 0) {
      MakeNoise(pSoldier, pSoldier->sGridNo, pSoldier->bLevel, ubVolume, NOISE_MOVEMENT);
      if (pSoldier->uiStatusFlags & SOLDIER_PC && pSoldier->bStealthMode) {
        PlayStealthySoldierFootstepSound(pSoldier);
      }
    }
  }

  // ATE: Make sure we don't make another interrupt...
  if (!gTacticalStatus.fInterruptOccurred) {
    // Handle New sight
    HandleSight(*pSoldier, SIGHT_LOOK | SIGHT_RADIO | SIGHT_INTERRUPT);
  }

  // ATE: Check if we have sighted anyone, if so, don't do anything else...
  // IN other words, we have stopped from sighting...
  if (gTacticalStatus.fInterruptOccurred) {
    // Unset no APs value
    AdjustNoAPToFinishMove(pSoldier, TRUE);

    *pfKeepMoving = FALSE;
    pSoldier->usPendingAnimation = NO_PENDING_ANIMATION;
    pSoldier->ubPendingDirection = NO_PENDING_DIRECTION;

    // ATE: Cancel only if our final destination
    if (pSoldier->sGridNo == pSoldier->sFinalDestination) {
      pSoldier->ubPendingAction = NO_PENDING_ACTION;
    }

    // this flag is set only to halt the currently moving guy; reset it now
    gTacticalStatus.fInterruptOccurred = FALSE;

    // ATE: Remove this if we were stopped....
    if (gTacticalStatus.fEnemySightingOnTheirTurn) {
      if (gTacticalStatus.enemy_sighting_on_their_turn_enemy == pSoldier) {
        pSoldier->fPauseAllAnimation = FALSE;
        gTacticalStatus.fEnemySightingOnTheirTurn = FALSE;
      }
    }
  } else if (pSoldier->fNoAPToFinishMove) {
    *pfKeepMoving = FALSE;
  } else if (pSoldier->usPathIndex == pSoldier->usPathDataSize && pSoldier->usPathDataSize == 0) {
    *pfKeepMoving = FALSE;
  } else if (gTacticalStatus.fEnemySightingOnTheirTurn) {
    // Hault guy!
    AdjustNoAPToFinishMove(pSoldier, TRUE);
    *pfKeepMoving = FALSE;
  }

  // OK, check for other stuff like mines...
  CheckIfNearbyGroundSeemsWrong(pSoldier, pSoldier->sGridNo, TRUE, pfKeepMoving);

  HandleSystemNewAISituation(pSoldier);

  if (pSoldier->bTeam == OUR_TEAM) {
    if (pSoldier->ubWhatKindOfMercAmI == MERC_TYPE__EPC) {
      // are we there yet?
      if (pSoldier->sSectorX == 13 && pSoldier->sSectorY == MAP_ROW_B && pSoldier->bSectorZ == 0) {
        switch (pSoldier->ubProfile) {
          case SKYRIDER:
            if (PythSpacesAway(pSoldier->sGridNo, 8842) < 11) {
              // Skyrider has arrived!
              EVENT_StopMerc(pSoldier);
              SetFactTrue(FACT_SKYRIDER_CLOSE_TO_CHOPPER);
              TriggerNPCRecord(SKYRIDER, 15);
              SetUpHelicopterForPlayer(13, MAP_ROW_B);
            }
            break;

          case MARY:
            HandleMaryArrival(pSoldier);
            break;
          case JOHN:
            HandleJohnArrival(pSoldier);
            break;
        }
      } else if (pSoldier->ubProfile == MARIA && pSoldier->sSectorX == 6 &&
                 pSoldier->sSectorY == MAP_ROW_C && pSoldier->bSectorZ == 0 &&
                 CheckFact(FACT_MARIA_ESCORTED_AT_LEATHER_SHOP, MARIA) == TRUE) {
        // check that Angel is there!
        if (NPCInRoom(ANGEL, 2))  // room 2 is leather shop
        {
          //	UnRecruitEPC( MARIA );
          TriggerNPCRecord(ANGEL, 12);
        }
      } else if (pSoldier->ubProfile == JOEY && pSoldier->sSectorX == 8 &&
                 pSoldier->sSectorY == MAP_ROW_G && pSoldier->bSectorZ == 0) {
        // if Joey walks near Martha then trigger Martha record 7
        if (CheckFact(FACT_JOEY_NEAR_MARTHA, 0)) {
          EVENT_StopMerc(pSoldier);
          TriggerNPCRecord(JOEY, 9);
        }
      }

    }
    // Drassen stuff for John & Mary
    else if (gubQuest[QUEST_ESCORT_TOURISTS] == QUESTINPROGRESS && pSoldier->sSectorX == 13 &&
             pSoldier->sSectorY == MAP_ROW_B && pSoldier->bSectorZ == 0) {
      if (CheckFact(FACT_JOHN_ALIVE, 0)) {
        HandleJohnArrival(NULL);
      } else {
        HandleMaryArrival(NULL);
      }
    }

  } else if (pSoldier->bTeam == CIV_TEAM && pSoldier->ubProfile != NO_PROFILE &&
             pSoldier->bNeutral) {
    switch (pSoldier->ubProfile) {
      case JIM:
      case JACK:
      case OLAF:
      case RAY:
      case OLGA:
      case TYRONE: {
        int16_t sDesiredMercDist;
        if (ClosestPC(pSoldier, &sDesiredMercDist) != NOWHERE) {
          if (sDesiredMercDist <= NPC_TALK_RADIUS * 2) {
            // stop
            CancelAIAction(pSoldier);
            // aaaaaaaaaaaaaaaaaaaaatttaaaack!!!!
            AddToShouldBecomeHostileOrSayQuoteList(pSoldier);
            // MakeCivHostile( pSoldier, 2 );
            // TriggerNPCWithIHateYouQuote( pSoldier->ubProfile );
          }
        }
      } break;

      default:
        break;
    }
  }
  return (TRUE);
}

void SelectNextAvailSoldier(const SOLDIERTYPE *const last) {
  // IF IT'S THE SELECTED GUY, MAKE ANOTHER SELECTED!
  FOR_EACH_IN_TEAM(s, last->bTeam) {
    if (OkControllableMerc(s)) {
      SelectSoldier(s, SELSOLDIER_NONE);
      return;
    }
  }

  SetSelectedMan(NULL);
  // Change UI mode to reflact that we are selected
  guiPendingOverrideEvent = I_ON_TERRAIN;
}

void SelectSoldier(SOLDIERTYPE *const s, const SelSoldierFlags flags) {
  // ARM: can't call SelectSoldier() in mapscreen, that will initialize
  // interface panels!!! ATE: Adjusted conditions a bit ( sometimes were not
  // getting selected )
  if (guiCurrentScreen == LAPTOP_SCREEN || guiCurrentScreen == MAP_SCREEN) return;

  // if we are in the shop keeper interface
  if (guiCurrentScreen == SHOPKEEPER_SCREEN) {
    // dont allow the player to change the selected merc
    return;
  }

  // If we are dead, ignore
  if (!OK_CONTROLLABLE_MERC(s)) return;

  // Don't do it if we don't have an interrupt
  if (!OK_INTERRUPT_MERC(s)) {
    // OK, we want to display message that we can't....
    if (flags & SELSOLDIER_FROM_UI) {
      ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, TacticalStr[MERC_IS_UNAVAILABLE_STR],
                s->name);
    }
    return;
  }

  SOLDIERTYPE *const old_sel = GetSelectedMan();
  if (s == old_sel && !(flags & SELSOLDIER_FORCE_RESELECT)) return;

  // Unselect old selected guy
  if (old_sel != NULL) {
    old_sel->fShowLocator = FALSE;
    old_sel->fFlashLocator = FALSE;

    // DB This used to say "s"... I fixed it
    if (old_sel->bLevel == 0) {
      //	ApplyTranslucencyToWalls((int16_t)(old_sel->dXPos / CELL_X_SIZE),
      //(int16_t)(old_sel->dYPos / CELL_Y_SIZE));
    }
    // DeleteSoldierLight(old_sel);

    UpdateForContOverPortrait(old_sel, FALSE);
  }

  SetSelectedMan(s);

  // find which squad this guy is, then set selected squad to this guy
  SetCurrentSquad(s->bAssignment, FALSE);

  if (s->bLevel == 0) {
    // CalcTranslucentWalls( s->dXPos / CELL_X_SIZE, s->dYPos / CELL_Y_SIZE);
  }

  // Set interface to reflect new selection!
  SetCurrentTacticalPanelCurrentMerc(s);

  // PLay ATTN SOUND
  if (flags & SELSOLDIER_ACKNOWLEDGE && !gGameSettings.fOptions[TOPTION_MUTE_CONFIRMATIONS]) {
    DoMercBattleSound(s, BATTLE_SOUND_ATTN1);
  }

  // Change UI mode to reflact that we are selected
  // NOT if we are locked inthe UI
  if (gTacticalStatus.ubCurrentTeam == OUR_TEAM && gCurrentUIMode != LOCKUI_MODE &&
      gCurrentUIMode != LOCKOURTURN_UI_MODE) {
    guiPendingOverrideEvent = M_ON_TERRAIN;
  }

  ChangeInterfaceLevel(s->bLevel);

  if (s->fMercAsleep) PutMercInAwakeState(s);

  // possibly say personality quote
  if (s->bTeam == OUR_TEAM && s->ubProfile != NO_PROFILE &&
      s->ubWhatKindOfMercAmI != MERC_TYPE__PLAYER_CHARACTER &&
      !(s->usQuoteSaidFlags & SOLDIER_QUOTE_SAID_PERSONALITY)) {
    switch (gMercProfiles[s->ubProfile].bPersonalityTrait) {
      case PSYCHO:
        if (Random(50) == 0) {
          TacticalCharacterDialogue(s, QUOTE_PERSONALITY_TRAIT);
          s->usQuoteSaidFlags |= SOLDIER_QUOTE_SAID_PERSONALITY;
        }
        break;

      default:
        break;
    }
  }

  UpdateForContOverPortrait(s, TRUE);

  // Remove any interactive tiles we could be over!
  BeginCurInteractiveTileCheck();
}

void LocateSoldier(SOLDIERTYPE *s, BOOLEAN fSetLocator) {
  // if (!bCenter && SoldierOnScreen(s)) return;

  // do we need to move the screen?
  // ATE: Force this baby to locate if told to
  if (!SoldierOnScreen(s) || fSetLocator == 10) {
    // Center on guy
    const int16_t sNewCenterWorldX = (int16_t)s->dXPos;
    const int16_t sNewCenterWorldY = (int16_t)s->dYPos;
    SetRenderCenter(sNewCenterWorldX, sNewCenterWorldY);

    // Plot new path!
    gfPlotNewMovement = TRUE;
  }

  // do we flash the name & health bars/health string above?
  switch (fSetLocator) {
    case DONTSETLOCATOR:
      break;
    case 10:
    case SETLOCATOR:
      ShowRadioLocator(s, SHOW_LOCATOR_NORMAL);
      break;
    default:
      ShowRadioLocator(s, SHOW_LOCATOR_FAST);
      break;
  }
}

void InternalLocateGridNo(uint16_t sGridNo, BOOLEAN fForce) {
  int16_t sNewCenterWorldX;
  int16_t sNewCenterWorldY;
  ConvertGridNoToCenterCellXY(sGridNo, &sNewCenterWorldX, &sNewCenterWorldY);

  // FIRST CHECK IF WE ARE ON SCREEN
  if (GridNoOnScreen(sGridNo) && !fForce) return;

  SetRenderCenter(sNewCenterWorldX, sNewCenterWorldY);
}

void LocateGridNo(uint16_t sGridNo) { InternalLocateGridNo(sGridNo, FALSE); }

void SlideTo(SOLDIERTYPE *const tgt, const BOOLEAN fSetLocator) {
  if (fSetLocator == SETANDREMOVEPREVIOUSLOCATOR) {
    FOR_EACH_SOLDIER(s) {
      if (s->bInSector) {
        // Remove all existing locators...
        s->fFlashLocator = FALSE;
      }
    }
  }

  // Locate even if on screen
  if (fSetLocator) ShowRadioLocator(tgt, SHOW_LOCATOR_NORMAL);

  // FIRST CHECK IF WE ARE ON SCREEN
  if (GridNoOnScreen(tgt->sGridNo)) return;

  // sGridNo here for DG compatibility
  gTacticalStatus.sSlideTarget = tgt->sGridNo;

  // Plot new path!
  gfPlotNewMovement = TRUE;
}

void SlideToLocation(const int16_t sDestGridNo) {
  if (sDestGridNo == NOWHERE) return;

  // FIRST CHECK IF WE ARE ON SCREEN
  if (GridNoOnScreen(sDestGridNo)) return;

  // sGridNo here for DG compatibility
  gTacticalStatus.sSlideTarget = sDestGridNo;

  // Plot new path!
  gfPlotNewMovement = TRUE;
}

void RebuildAllSoldierShadeTables() { FOR_EACH_SOLDIER(i) CreateSoldierPalettes(*i); }

void HandlePlayerTeamMemberDeath(SOLDIERTYPE *pSoldier) {
  VerifyPublicOpplistDueToDeath(pSoldier);
  ReceivingSoldierCancelServices(pSoldier);

  // look for all mercs on the same team,
  SOLDIERTYPE *new_selected_soldier = NULL;
  FOR_EACH_IN_TEAM(s, pSoldier->bTeam) {
    if (s->bLife >= OKLIFE && s->bInSector) {
      new_selected_soldier = s;
      break;
    }
  }

  if (new_selected_soldier != NULL) {
    if (gTacticalStatus.fAutoBandageMode && pSoldier->auto_bandaging_medic != NULL) {
      CancelAIAction(pSoldier->auto_bandaging_medic);
    }

    // see if this was the friend of a living merc
    FOR_EACH_IN_TEAM(s, pSoldier->bTeam) {
      if (s->bInSector && s->bLife >= OKLIFE) {
        const int8_t bBuddyIndex = WhichBuddy(s->ubProfile, pSoldier->ubProfile);
        switch (bBuddyIndex) {
          case 0:
            TacticalCharacterDialogue(s, QUOTE_BUDDY_ONE_KILLED);
            break;
          case 1:
            TacticalCharacterDialogue(s, QUOTE_BUDDY_TWO_KILLED);
            break;
          case 2:
            TacticalCharacterDialogue(s, QUOTE_LEARNED_TO_LIKE_MERC_KILLED);
            break;
          default:
            break;
        }
      }
    }

    switch (pSoldier->ubProfile) {
      case SLAY: {
        // handle stuff for Carmen if Slay is killed
        const SOLDIERTYPE *const s = FindSoldierByProfileID(CARMEN);
        if (s && s->bAttitude == ATTACKSLAYONLY && ClosestPC(s, NULL) != NOWHERE) {
          // Carmen now becomes friendly again
          TriggerNPCRecord(CARMEN, 29);
        }
        break;
      }

      case ROBOT:
        if (!CheckFact(FACT_FIRST_ROBOT_DESTROYED, 0)) {
          SetFactTrue(FACT_FIRST_ROBOT_DESTROYED);
          SetFactFalse(FACT_ROBOT_READY);
        } else {
          SetFactTrue(FACT_SECOND_ROBOT_DESTROYED);
        }
        break;
    }
  }

  // Make a call to handle the strategic things, such as Life Insurance, record
  // it in history file etc.
  StrategicHandlePlayerTeamMercDeath(*pSoldier);

  CheckForEndOfBattle(FALSE);

  if (GetSelectedMan() == pSoldier) {
    if (new_selected_soldier) {
      SelectSoldier(new_selected_soldier, SELSOLDIER_NONE);
    } else {
      SetSelectedMan(NULL);
      // Change UI mode to reflact that we are selected
      guiPendingOverrideEvent = I_ON_TERRAIN;
    }
  }
}

void HandleNPCTeamMemberDeath(SOLDIERTYPE *const pSoldierOld) {
  pSoldierOld->uiStatusFlags |= SOLDIER_DEAD;
  const int8_t bVisible = pSoldierOld->bVisible;

  VerifyPublicOpplistDueToDeath(pSoldierOld);

  SOLDIERTYPE *const killer = pSoldierOld->attacker;

  if (pSoldierOld->ubProfile != NO_PROFILE) {
    // mark as dead!
    gMercProfiles[pSoldierOld->ubProfile].bMercStatus = MERC_IS_DEAD;
    gMercProfiles[pSoldierOld->ubProfile].bLife = 0;

    if (!(pSoldierOld->uiStatusFlags & SOLDIER_VEHICLE) && !TANK(pSoldierOld)) {
      const uint8_t code = (killer && killer->bTeam == OUR_TEAM ? HISTORY_MERC_KILLED_CHARACTER
                                                                : HISTORY_NPC_KILLED);
      AddHistoryToPlayersLog(code, pSoldierOld->ubProfile, GetWorldTotalMin(), gWorldSectorX,
                             gWorldSectorY);
    }
  }

  if (pSoldierOld->bTeam == CIV_TEAM) {
    // ATE: Added string to player
    if (bVisible != -1 && pSoldierOld->ubProfile != NO_PROFILE) {
      ScreenMsg(FONT_RED, MSG_INTERFACE, pMercDeadString, pSoldierOld->name);
    }

    switch (pSoldierOld->ubProfile) {
      case BRENDA:
        SetFactTrue(FACT_BRENDA_DEAD);
        {
          const SOLDIERTYPE *const pOther = FindSoldierByProfileID(HANS);
          if (pOther && pOther->bLife >= OKLIFE && pOther->bNeutral &&
              SpacesAway(pSoldierOld->sGridNo, pOther->sGridNo) <= 12) {
            TriggerNPCRecord(HANS, 10);
          }
        }
        break;

      case PABLO:
        AddFutureDayStrategicEvent(EVENT_SECOND_AIRPORT_ATTENDANT_ARRIVED, 480 + Random(60), 0, 1);
        break;

      case ROBOT:
        if (!CheckFact(FACT_FIRST_ROBOT_DESTROYED, 0)) {
          SetFactTrue(FACT_FIRST_ROBOT_DESTROYED);
        } else {
          SetFactTrue(FACT_SECOND_ROBOT_DESTROYED);
        }
        break;

      case DRUGGIST:
      case SLAY:
      case ANNIE:
      case CHRIS:
      case TIFFANY:
      case T_REX:
        MakeRemainingTerroristsTougher();
        if (pSoldierOld->ubProfile == DRUGGIST) {
          SOLDIERTYPE *const pOther = FindSoldierByProfileID(MANNY);
          if (pOther && pOther->bInSector && pOther->bLife >= OKLIFE) {
            // try to make sure he isn't cowering etc
            pOther->sNoiseGridno = NOWHERE;
            pOther->bAlertStatus = STATUS_GREEN;
            TriggerNPCRecord(MANNY, 10);
          }
        }
        break;

      case JIM:
      case JACK:
      case OLAF:
      case RAY:
      case OLGA:
      case TYRONE:
        MakeRemainingAssassinsTougher();
        break;

      case ELDIN:
        // the security guard...  Results in an extra loyalty penalty for Balime
        // (in addition to civilian murder)
        DecrementTownLoyalty(BALIME, LOYALTY_PENALTY_ELDIN_KILLED);
        break;

      case JOEY: {
        // check to see if Martha can see this
        const SOLDIERTYPE *const pOther = FindSoldierByProfileID(MARTHA);
        if (pOther && (PythSpacesAway(pOther->sGridNo, pSoldierOld->sGridNo) < 10 ||
                       SoldierToSoldierLineOfSightTest(pOther, pSoldierOld, MaxDistanceVisible(),
                                                       TRUE) != 0)) {
          // Martha has a heart attack and croaks
          TriggerNPCRecord(MARTHA, 17);
          DecrementTownLoyalty(CAMBRIA, LOYALTY_PENALTY_MARTHA_HEART_ATTACK);
        } else  // Martha doesn't see it.  She lives, but Joey is found a day or so
                // later anyways
        {
          DecrementTownLoyalty(CAMBRIA, LOYALTY_PENALTY_JOEY_KILLED);
        }
        break;
      }

      case DYNAMO:
        // check to see if dynamo quest is on
        if (gubQuest[QUEST_FREE_DYNAMO] == QUESTINPROGRESS) {
          EndQuest(QUEST_FREE_DYNAMO, pSoldierOld->sSectorX, pSoldierOld->sSectorY);
        }
        break;

      case KINGPIN:
        // check to see if Kingpin money quest is on
        if (gubQuest[QUEST_KINGPIN_MONEY] == QUESTINPROGRESS) {
          EndQuest(QUEST_KINGPIN_MONEY, pSoldierOld->sSectorX, pSoldierOld->sSectorY);
          HandleNPCDoAction(KINGPIN, NPC_ACTION_GRANT_EXPERIENCE_3, 0);
        }
        SetFactTrue(FACT_KINGPIN_DEAD);
        ExecuteStrategicAIAction(STRATEGIC_AI_ACTION_KINGPIN_DEAD, 0, 0);
        break;

      case DOREEN:
        // Doreen's dead
        if (CheckFact(FACT_DOREEN_HAD_CHANGE_OF_HEART, 0)) {
          // tsk tsk, player killed her after getting her to reconsider, lose the
          // bonus for sparing her
          DecrementTownLoyalty(DRASSEN, LOYALTY_BONUS_CHILDREN_FREED_DOREEN_SPARED);
        }  // then get the points for freeing the kids though killing her
        IncrementTownLoyalty(DRASSEN, LOYALTY_BONUS_CHILDREN_FREED_DOREEN_KILLED);
        // set the fact true so we have a universal check for whether the kids can
        // go
        SetFactTrue(FACT_DOREEN_HAD_CHANGE_OF_HEART);
        EndQuest(QUEST_FREE_CHILDREN, gWorldSectorX, gWorldSectorY);
        if (!CheckFact(FACT_KIDS_ARE_FREE, 0)) {
          HandleNPCDoAction(DOREEN, NPC_ACTION_FREE_KIDS, 0);
        }
        break;
    }

    // Are we looking at the queen?
    if (pSoldierOld->ubProfile == QUEEN) {
      BeginHandleDeidrannaDeath(killer, pSoldierOld->sGridNo, pSoldierOld->bLevel);
    }

    // crows/cows are on the civilian team, but none of the following applies to
    // them
    if (pSoldierOld->ubBodyType != CROW && pSoldierOld->ubBodyType != COW) {
      // handle death of civilian..and if it was intentional
      HandleMurderOfCivilian(pSoldierOld);
    }
  } else if (pSoldierOld->bTeam == MILITIA_TEAM) {
    const int8_t bMilitiaRank = SoldierClassToMilitiaRank(pSoldierOld->ubSoldierClass);
    if (bMilitiaRank != -1) {
      // remove this militia from the strategic records
      StrategicRemoveMilitiaFromSector(gWorldSectorX, gWorldSectorY, bMilitiaRank, 1);
    }

    // also treat this as murder - but player will never be blamed for militia
    // death he didn't cause
    HandleMurderOfCivilian(pSoldierOld);

    HandleGlobalLoyaltyEvent(GLOBAL_LOYALTY_NATIVE_KILLED, gWorldSectorX, gWorldSectorY,
                             gbWorldSectorZ);
  } else  // enemies and creatures... should any of this stuff not be called if a
          // creature dies?
  {
    if (pSoldierOld->ubBodyType == QUEENMONSTER && killer != NULL) {
      BeginHandleQueenBitchDeath(killer, pSoldierOld->sGridNo, pSoldierOld->bLevel);
    }

    if (pSoldierOld->bTeam == ENEMY_TEAM) {
      gTacticalStatus.ubArmyGuysKilled++;
      TrackEnemiesKilled(ENEMY_KILLED_IN_TACTICAL, pSoldierOld->ubSoldierClass);
    }
    // If enemy guy was killed by the player, give morale boost to player's
    // team!
    if (killer != NULL && killer->bTeam == OUR_TEAM) {
      HandleMoraleEvent(killer, MORALE_KILLED_ENEMY, gWorldSectorX, gWorldSectorY, gbWorldSectorZ);
    }

    HandleGlobalLoyaltyEvent(GLOBAL_LOYALTY_ENEMY_KILLED, gWorldSectorX, gWorldSectorY,
                             gbWorldSectorZ);

    CheckForAlertWhenEnemyDies(pSoldierOld);

    if (gTacticalStatus.the_chosen_one == pSoldierOld) {
      // reset the chosen one!
      gTacticalStatus.the_chosen_one = NULL;
    }

    if (pSoldierOld->ubProfile == QUEEN) {
      HandleMoraleEvent(NULL, MORALE_DEIDRANNA_KILLED, gWorldSectorX, gWorldSectorY,
                        gbWorldSectorZ);
      MaximizeLoyaltyForDeidrannaKilled();
    } else if (pSoldierOld->ubBodyType == QUEENMONSTER) {
      HandleMoraleEvent(NULL, MORALE_MONSTER_QUEEN_KILLED, gWorldSectorX, gWorldSectorY,
                        gbWorldSectorZ);
      IncrementTownLoyaltyEverywhere(LOYALTY_BONUS_KILL_QUEEN_MONSTER);

      // Grant experience gain.....
      HandleNPCDoAction(0, NPC_ACTION_GRANT_EXPERIENCE_5, 0);
    }
  }

  // killing crows/cows is not worth any experience!
  if (pSoldierOld->ubBodyType != CROW && pSoldierOld->ubBodyType != COW &&
      pSoldierOld->ubLastDamageReason != TAKE_DAMAGE_BLOODLOSS) {
    // if it was a kill by a player's merc
    if (killer != NULL && killer->bTeam == OUR_TEAM) {
      // EXPERIENCE CLASS GAIN:  Earned a kill
      StatChange(*killer, EXPERAMT, 10 * pSoldierOld->bExpLevel, FROM_SUCCESS);
    }

    // JA2 Gold: if previous and current attackers are the same, the
    // next-to-previous attacker gets the assist
    SOLDIERTYPE *assister = pSoldierOld->previous_attacker;
    if (assister == killer) assister = pSoldierOld->next_to_previous_attacker;

    // if it was assisted by a player's merc
    if (assister != NULL && assister->bActive && assister->bTeam == OUR_TEAM) {
      // EXPERIENCE CLASS GAIN:  Earned an assist
      StatChange(*assister, EXPERAMT, 5 * pSoldierOld->bExpLevel, FROM_SUCCESS);
    }
  }

  if (killer != NULL && killer->bTeam == MILITIA_TEAM) {
    killer->ubMilitiaKills++;
  }

  // if the NPC is a dealer, add the dealers items to the ground
  AddDeadArmsDealerItemsToWorld(pSoldierOld);

  // The queen AI layer must process the event by subtracting forces, etc.
  ProcessQueenCmdImplicationsOfDeath(pSoldierOld);

  // OK, check for existence of any more badguys!
  CheckForEndOfBattle(FALSE);
}

void CheckForPotentialAddToBattleIncrement(SOLDIERTYPE *pSoldier) {
  // Check if we are a threat!
  if (pSoldier->bNeutral || pSoldier->bSide == OUR_TEAM) return;

  if (pSoldier->bTeam == CIV_TEAM) {
    // maybe increment num enemy attacked
    switch (pSoldier->ubCivilianGroup) {
      case REBEL_CIV_GROUP:
      case KINGPIN_CIV_GROUP:
      case HICKS_CIV_GROUP:
        /* We need to exclude cases where a kid is not neutral anymore, but is
         * defenseless! */
        if (FindObjClass(pSoldier, IC_WEAPON) != NO_SLOT) {
          gTacticalStatus.bNumFoughtInBattle[pSoldier->bTeam]++;
        }
        break;

      default:
        break;
    }
  } else {
    // Increment num enemy attacked
    gTacticalStatus.bNumFoughtInBattle[pSoldier->bTeam]++;
  }
}

// internal function for turning neutral to FALSE
void SetSoldierNonNeutral(SOLDIERTYPE *pSoldier) {
  pSoldier->bNeutral = FALSE;
  if (gTacticalStatus.bBoxingState == NOT_BOXING) {
    // Special code for strategic implications
    CalculateNonPersistantPBIInfo();
  }
}

// internal function for turning neutral to TRUE
void SetSoldierNeutral(SOLDIERTYPE *pSoldier) {
  pSoldier->bNeutral = TRUE;
  if (gTacticalStatus.bBoxingState == NOT_BOXING) {
    // Special code for strategic implications
    // search through civ team looking for non-neutral civilian!
    if (!HostileCiviliansPresent()) {
      CalculateNonPersistantPBIInfo();
    }
  }
}

void MakeCivHostile(SOLDIERTYPE *pSoldier, int8_t bNewSide) {
  if (pSoldier->ubBodyType == COW) return;

  // override passed-in value; default is hostile to player, allied to army
  bNewSide = 1;

  switch (pSoldier->ubProfile) {
    case IRA:
    case DIMITRI:
    case MIGUEL:
    case CARLOS:
    case MADLAB:
    case DYNAMO:
    case SHANK:
      // rebels and rebel sympathizers become hostile to player and enemy
      bNewSide = 2;
      break;

    case MARIA:
    case ANGEL:
      if (gubQuest[QUEST_RESCUE_MARIA] == QUESTINPROGRESS ||
          gubQuest[QUEST_RESCUE_MARIA] == QUESTDONE) {
        bNewSide = 2;
      }
      break;

    default:
      switch (pSoldier->ubCivilianGroup) {
        case REBEL_CIV_GROUP:
          bNewSide = 2;
          break;
        default:
          break;
      }
      break;
  }

  if (!pSoldier->bNeutral && bNewSide == pSoldier->bSide) {
    // already hostile!
    return;
  }

  if (pSoldier->ubProfile == CONRAD || pSoldier->ubProfile == GENERAL) {
    // change to enemy team
    SetSoldierNonNeutral(pSoldier);
    pSoldier->bSide = bNewSide;
    pSoldier = ChangeSoldierTeam(pSoldier, ENEMY_TEAM);
  } else {
    if (pSoldier->ubCivilianGroup == KINGPIN_CIV_GROUP) {
      /* if Maria is in the sector and escorted, set fact that the escape has
       * been noticed */
      if (gubQuest[QUEST_RESCUE_MARIA] == QUESTINPROGRESS &&
          gTacticalStatus.bBoxingState == NOT_BOXING) {
        const SOLDIERTYPE *const pMaria = FindSoldierByProfileID(MARIA);
        if (pMaria && pMaria->bInSector) {
          SetFactTrue(FACT_MARIA_ESCAPE_NOTICED);
        }
      }
    }
    if (pSoldier->ubProfile == BILLY) pSoldier->bOrders = FARPATROL;
    if (bNewSide != -1) pSoldier->bSide = bNewSide;
    if (pSoldier->bNeutral) {
      SetSoldierNonNeutral(pSoldier);
      RecalculateOppCntsDueToNoLongerNeutral(pSoldier);
    }
  }

  // If we are already in combat...
  if (gTacticalStatus.uiFlags & INCOMBAT) {
    CheckForPotentialAddToBattleIncrement(pSoldier);
  }
}

uint8_t CivilianGroupMembersChangeSidesWithinProximity(SOLDIERTYPE *pAttacked) {
  if (pAttacked->ubCivilianGroup == NON_CIV_GROUP) return pAttacked->ubProfile;

  uint8_t ubFirstProfile = NO_PROFILE;
  FOR_EACH_IN_TEAM(s, CIV_TEAM) {
    if (!s->bInSector || s->bLife == 0 || !s->bNeutral) continue;
    if (s->ubCivilianGroup != pAttacked->ubCivilianGroup || s->ubBodyType == COW) continue;

    // if in LOS of this guy's attacker
    const SOLDIERTYPE *const attacker = pAttacked->attacker;
    if ((attacker != NULL && s->bOppList[attacker->ubID] == SEEN_CURRENTLY) ||
        (PythSpacesAway(s->sGridNo, pAttacked->sGridNo) < MaxDistanceVisible()) ||
        (attacker != NULL &&
         PythSpacesAway(s->sGridNo, attacker->sGridNo) < MaxDistanceVisible())) {
      MakeCivHostile(s, 2);
      if (s->bOppCnt > 0) {
        AddToShouldBecomeHostileOrSayQuoteList(s);
      }

      if (s->ubProfile != NO_PROFILE && s->bOppCnt > 0 &&
          (ubFirstProfile == NO_PROFILE || Random(2))) {
        ubFirstProfile = s->ubProfile;
      }
    }
  }

  return ubFirstProfile;
}

SOLDIERTYPE *CivilianGroupMemberChangesSides(SOLDIERTYPE *pAttacked) {
  SOLDIERTYPE *pNewAttacked = pAttacked;

  if (pAttacked->ubCivilianGroup == NON_CIV_GROUP) {
    // abort
    return (pNewAttacked);
  }

  // remove anyone (rebels) on our team and put them back in the civ team
  uint8_t ubFirstProfile = NO_PROFILE;
  FOR_EACH_IN_TEAM(pSoldier, OUR_TEAM) {
    if (pSoldier->bInSector && pSoldier->bLife != 0) {
      if (pSoldier->ubCivilianGroup == pAttacked->ubCivilianGroup) {
        // should become hostile
        if (pSoldier->ubProfile != NO_PROFILE && (ubFirstProfile == NO_PROFILE || Random(2))) {
          ubFirstProfile = pSoldier->ubProfile;
        }

        SOLDIERTYPE *const pNew = ChangeSoldierTeam(pSoldier, CIV_TEAM);
        if (pSoldier == pAttacked) pNewAttacked = pNew;
      }
    }
  }

  // now change sides for anyone on the civ team within proximity
  if (ubFirstProfile == NO_PROFILE) {
    // get first profile value
    ubFirstProfile = CivilianGroupMembersChangeSidesWithinProximity(pNewAttacked);
  } else {
    // just call
    CivilianGroupMembersChangeSidesWithinProximity(pNewAttacked);
  }

  /*
          if ( ubFirstProfile != NO_PROFILE )
          {
                  TriggerFriendWithHostileQuote( ubFirstProfile );
          }
  */

  if (gTacticalStatus.fCivGroupHostile[pNewAttacked->ubCivilianGroup] == CIV_GROUP_NEUTRAL) {
    // if the civilian group turning hostile is the Rebels
    if (pAttacked->ubCivilianGroup == REBEL_CIV_GROUP) {
      // we haven't already reduced the loyalty back when we first set the flag
      // to BECOME hostile
      ReduceLoyaltyForRebelsBetrayed();
    }

    AddStrategicEvent(EVENT_MAKE_CIV_GROUP_HOSTILE_ON_NEXT_SECTOR_ENTRANCE,
                      GetWorldTotalMin() + 300, pNewAttacked->ubCivilianGroup);
    gTacticalStatus.fCivGroupHostile[pNewAttacked->ubCivilianGroup] =
        CIV_GROUP_WILL_EVENTUALLY_BECOME_HOSTILE;
  }

  return (pNewAttacked);
}

void CivilianGroupChangesSides(uint8_t ubCivilianGroup) {
  // change civ group side due to external event (wall blowing up)
  // uint8_t ubFirstProfile = NO_PROFILE;

  gTacticalStatus.fCivGroupHostile[ubCivilianGroup] = CIV_GROUP_HOSTILE;

  // now change sides for anyone on the civ team
  FOR_EACH_IN_TEAM(pSoldier, CIV_TEAM) {
    if (pSoldier->bInSector && pSoldier->bLife && pSoldier->bNeutral) {
      if (pSoldier->ubCivilianGroup == ubCivilianGroup && pSoldier->ubBodyType != COW) {
        MakeCivHostile(pSoldier, 2);
        if (pSoldier->bOppCnt > 0) {
          AddToShouldBecomeHostileOrSayQuoteList(pSoldier);
        }
        /*
        if ( (pSoldier->ubProfile != NO_PROFILE) && (pSoldier->bOppCnt > 0) && (
        ubFirstProfile == NO_PROFILE || Random( 2 ) ) )
        {
                ubFirstProfile = pSoldier->ubProfile;
        }
        */
      }
    }
  }

  /*
  if ( ubFirstProfile != NO_PROFILE )
  {
          TriggerFriendWithHostileQuote( ubFirstProfile );
  }
  */
}

static void HickCowAttacked(SOLDIERTYPE *pNastyGuy, SOLDIERTYPE *pTarget) {
  // now change sides for anyone on the civ team
  FOR_EACH_IN_TEAM(pSoldier, CIV_TEAM) {
    if (pSoldier->bInSector && pSoldier->bLife && pSoldier->bNeutral &&
        pSoldier->ubCivilianGroup == HICKS_CIV_GROUP) {
      if (SoldierToSoldierLineOfSightTest(pSoldier, pNastyGuy, (uint8_t)MaxDistanceVisible(),
                                          TRUE)) {
        CivilianGroupMemberChangesSides(pSoldier);
        break;
      }
    }
  }
}

static void MilitiaChangesSides() {
  // make all the militia change sides
  if (!IsTeamActive(MILITIA_TEAM)) return;

  // remove anyone (rebels) on our team and put them back in the civ team
  FOR_EACH_IN_TEAM(s, MILITIA_TEAM) {
    if (s->bInSector && s->bLife != 0) {
      MakeCivHostile(s, 2);
      RecalculateOppCntsDueToNoLongerNeutral(s);
    }
  }
}

static SOLDIERTYPE *FindActiveAndAliveMerc(const SOLDIERTYPE *const curr, const uint32_t step,
                                           const BOOLEAN fGoodForLessOKLife,
                                           const BOOLEAN fOnlyRegularMercs) {
  const TacticalTeamType *const t = &gTacticalStatus.Team[curr->bTeam];

  SOLDIERTYPE *s;
  SoldierID i = curr->ubID;
  do {
    uint32_t di = step;
    if (i > t->bLastID - di) di -= t->bLastID - t->bFirstID + 1;  // modulo, subtract span
    i += di;
    s = &GetMan(i);
    if (!s->bActive) continue;

    if (fOnlyRegularMercs && (AM_AN_EPC(s) || AM_A_ROBOT(s))) continue;
    if (s->bAssignment != curr->bAssignment) continue;
    if (!OK_INTERRUPT_MERC(s)) continue;
    if (!s->bInSector) continue;
    if (s->bLife < (fGoodForLessOKLife ? 1 : OKLIFE)) continue;
    break;
  } while (s != curr);
  return s;
}

SOLDIERTYPE *FindNextActiveAndAliveMerc(const SOLDIERTYPE *const curr,
                                        const BOOLEAN fGoodForLessOKLife,
                                        const BOOLEAN fOnlyRegularMercs) {
  return FindActiveAndAliveMerc(curr, 1, fGoodForLessOKLife, fOnlyRegularMercs);
}

SOLDIERTYPE *FindPrevActiveAndAliveMerc(const SOLDIERTYPE *const curr,
                                        const BOOLEAN fGoodForLessOKLife,
                                        const BOOLEAN fOnlyRegularMercs) {
  const TacticalTeamType *const t = &gTacticalStatus.Team[curr->bTeam];
  uint32_t const step = t->bLastID - t->bFirstID;  // modulo: -1
  return FindActiveAndAliveMerc(curr, step, fGoodForLessOKLife, fOnlyRegularMercs);
}

static SOLDIERTYPE *FindNextActiveSquadRange(int8_t begin, int8_t end) {
  for (int32_t i = begin; i != end; ++i) {
    FOR_EACH_IN_SQUAD(j, i) {
      SOLDIERTYPE *const s = *j;
      if (!s->bInSector) continue;
      if (!OK_INTERRUPT_MERC(s)) continue;
      if (!OK_CONTROLLABLE_MERC(s)) continue;
      if (s->uiStatusFlags & SOLDIER_VEHICLE) continue;
      return s;
    }
  }
  return NULL;
}

SOLDIERTYPE *FindNextActiveSquad(SOLDIERTYPE *s) {
  const int8_t assignment = s->bAssignment;
  SOLDIERTYPE *res;

  res = FindNextActiveSquadRange(assignment + 1, NUMBER_OF_SQUADS);
  if (res != NULL) return res;

  // none found, now loop back
  res = FindNextActiveSquadRange(0, assignment);
  if (res != NULL) return res;

  // IF we are here, keep as we always were!
  return s;
}

static bool IsDestinationBlocked(GridNo const grid_no, int8_t const level, SOLDIERTYPE const &s) {
  /* ATE: If we are trying to get a path to an exit grid, still allow this */
  if (gfPlotPathToExitGrid) return false;

  // Check obstruction in future
  int16_t const desired_level = level == 0 ? STRUCTURE_ON_GROUND : STRUCTURE_ON_ROOF;
  FOR_EACH_STRUCTURE(i, grid_no, STRUCTURE_BLOCKSMOVES) {
    if (i->fFlags & STRUCTURE_PASSABLE) continue;

    // Check if this is a multi-tile and check IDs with soldier's ID
    if (i->fFlags & STRUCTURE_MOBILE && s.uiStatusFlags & SOLDIER_MULTITILE && s.pLevelNode &&
        s.pLevelNode->pStructureData &&
        s.pLevelNode->pStructureData->usStructureID == i->usStructureID) {
      continue;
    }

    if (i->sCubeOffset == desired_level) return true;
  }

  return false;
}

// NB if making changes don't forget to update NewOKDestinationAndDirection
int16_t NewOKDestination(const SOLDIERTYPE *pCurrSoldier, int16_t sGridNo, BOOLEAN fPeopleToo,
                         int8_t bLevel) {
  if (!GridNoOnVisibleWorldTile(sGridNo)) return TRUE;

  if (fPeopleToo) {
    const SOLDIERTYPE *const person = WhoIsThere2(sGridNo, bLevel);
    if (person != NULL) {
      // we could be multitiled... if the person there is us, and the gridno is
      // not our base gridno, skip past these checks
      if (person != pCurrSoldier || sGridNo == pCurrSoldier->sGridNo) {
        if (pCurrSoldier->bTeam != OUR_TEAM || person->bVisible >= 0 ||
            gTacticalStatus.uiFlags & SHOW_ALL_MERCS) {
          return FALSE;  // if someone there it's NOT OK
        }
      }
    }
  }

  // Check structure database
  if (pCurrSoldier->uiStatusFlags & SOLDIER_MULTITILE && !gfEstimatePath) {
    // this could be kinda slow...

    // Get animation surface...
    const uint16_t usAnimSurface =
        DetermineSoldierAnimationSurface(pCurrSoldier, pCurrSoldier->usUIMovementMode);
    // Get structure ref...
    const STRUCTURE_FILE_REF *const pStructureFileRef =
        GetAnimationStructureRef(pCurrSoldier, usAnimSurface, pCurrSoldier->usUIMovementMode);

    // opposite directions should be mirrors, so only check 4
    if (pStructureFileRef) {
      // if ANY direction is valid, consider moving here valid
      for (int8_t i = 0; i < NUM_WORLD_DIRECTIONS; ++i) {
        // ATE: Only if we have a levelnode...
        uint16_t usStructureID;
        if (pCurrSoldier->pLevelNode != NULL && pCurrSoldier->pLevelNode->pStructureData != NULL) {
          usStructureID = pCurrSoldier->pLevelNode->pStructureData->usStructureID;
        } else {
          usStructureID = INVALID_STRUCTURE_ID;
        }

        if (InternalOkayToAddStructureToWorld(sGridNo, bLevel,
                                              &pStructureFileRef->pDBStructureRef[i], usStructureID,
                                              !fPeopleToo)) {
          return TRUE;
        }
      }
    }
    return FALSE;
  }

  if (IsDestinationBlocked(sGridNo, bLevel, *pCurrSoldier)) return FALSE;

  return TRUE;
}

// NB if making changes don't forget to update NewOKDestination
static int16_t NewOKDestinationAndDirection(const SOLDIERTYPE *pCurrSoldier, int16_t sGridNo,
                                            int8_t bDirection, BOOLEAN fPeopleToo, int8_t bLevel) {
  if (fPeopleToo) {
    const SOLDIERTYPE *const person = WhoIsThere2(sGridNo, bLevel);
    if (person != NULL) {
      /* we could be multitiled... if the person there is us, and the gridno is
       * not our base gridno, skip past these checks */
      if (person != pCurrSoldier || sGridNo == pCurrSoldier->sGridNo) {
        if (pCurrSoldier->bTeam != OUR_TEAM || person->bVisible >= 0 ||
            gTacticalStatus.uiFlags & SHOW_ALL_MERCS) {
          return FALSE;  // if someone there it's NOT OK
        }
      }
    }
  }

  // Check structure database
  if (pCurrSoldier->uiStatusFlags & SOLDIER_MULTITILE && !gfEstimatePath) {
    // this could be kinda slow...

    // Get animation surface...
    const uint16_t usAnimSurface =
        DetermineSoldierAnimationSurface(pCurrSoldier, pCurrSoldier->usUIMovementMode);
    // Get structure ref...
    const STRUCTURE_FILE_REF *const pStructureFileRef =
        GetAnimationStructureRef(pCurrSoldier, usAnimSurface, pCurrSoldier->usUIMovementMode);
    if (pStructureFileRef) {
      // use the specified direction for checks
      const int8_t bLoop = bDirection;
      // ATE: Only if we have a levelnode...
      uint16_t usStructureID = INVALID_STRUCTURE_ID;
      if (pCurrSoldier->pLevelNode != NULL && pCurrSoldier->pLevelNode->pStructureData != NULL) {
        usStructureID = pCurrSoldier->pLevelNode->pStructureData->usStructureID;
      }

      if (InternalOkayToAddStructureToWorld(
              sGridNo, pCurrSoldier->bLevel,
              &pStructureFileRef->pDBStructureRef[OneCDirection(bLoop)], usStructureID,
              !fPeopleToo)) {
        return TRUE;
      }
    }
    return FALSE;
  }

  if (IsDestinationBlocked(sGridNo, bLevel, *pCurrSoldier)) return FALSE;

  return TRUE;
}

// Kris:
BOOLEAN FlatRoofAboveGridNo(int32_t iMapIndex) {
  for (const LEVELNODE *i = gpWorldLevelData[iMapIndex].pRoofHead; i; i = i->pNext) {
    if (i->usIndex == NO_TILE) continue;

    const uint32_t uiTileType = GetTileType(i->usIndex);
    if (uiTileType >= FIRSTROOF && uiTileType <= LASTROOF) return TRUE;
  }
  return FALSE;
}

/* Kris:
 * ASSUMPTION:  This function assumes that we are checking on behalf of a single
 *              tiled merc.  This function should not be used for checking on
 *              behalf of multi-tiled "things".
 * I wrote this function for editor use.  I don't personally care about
 * multi-tiled stuff.  All I want to know is whether or not I can put a merc
 * here.  In most cases, I won't be dealing with multi-tiled mercs, and the
 * rarity doesn't justify the needs.  I just wrote this to be quick and dirty,
 * and I don't expect it to perform perfectly in all situations. */
BOOLEAN IsLocationSittable(int32_t iMapIndex, BOOLEAN fOnRoof) {
  int16_t sDesiredLevel;
  if (WhoIsThere2(iMapIndex, 0) != NULL) return FALSE;
  // Locations on roofs without a roof is not possible, so
  // we convert the onroof intention to ground.
  if (fOnRoof && !FlatRoofAboveGridNo(iMapIndex)) fOnRoof = FALSE;
  // Check structure database
  if (gpWorldLevelData[iMapIndex].pStructureHead) {
    // Something is here, check obstruction in future
    sDesiredLevel = fOnRoof ? STRUCTURE_ON_ROOF : STRUCTURE_ON_GROUND;
    FOR_EACH_STRUCTURE(pStructure, (int16_t)iMapIndex, STRUCTURE_BLOCKSMOVES) {
      if (!(pStructure->fFlags & STRUCTURE_PASSABLE) && pStructure->sCubeOffset == sDesiredLevel)
        return FALSE;
    }
  }
  return TRUE;
}

BOOLEAN IsLocationSittableExcludingPeople(int32_t iMapIndex, BOOLEAN fOnRoof) {
  int16_t sDesiredLevel;

  // Locations on roofs without a roof is not possible, so
  // we convert the onroof intention to ground.
  if (fOnRoof && !FlatRoofAboveGridNo(iMapIndex)) fOnRoof = FALSE;
  // Check structure database
  if (gpWorldLevelData[iMapIndex].pStructureHead) {
    // Something is here, check obstruction in future
    sDesiredLevel = fOnRoof ? STRUCTURE_ON_ROOF : STRUCTURE_ON_GROUND;
    FOR_EACH_STRUCTURE(pStructure, (int16_t)iMapIndex, STRUCTURE_BLOCKSMOVES) {
      if (!(pStructure->fFlags & STRUCTURE_PASSABLE) && pStructure->sCubeOffset == sDesiredLevel)
        return FALSE;
    }
  }
  return TRUE;
}

BOOLEAN TeamMemberNear(int8_t bTeam, int16_t sGridNo, int32_t iRange) {
  CFOR_EACH_IN_TEAM(s, bTeam) {
    if (s->bInSector && s->bLife >= OKLIFE && !(s->uiStatusFlags & SOLDIER_GASSED) &&
        PythSpacesAway(s->sGridNo, sGridNo) <= iRange) {
      return TRUE;
    }
  }
  return FALSE;
}

int16_t FindAdjacentGridEx(SOLDIERTYPE *pSoldier, int16_t sGridNo, uint8_t *pubDirection,
                           int16_t *psAdjustedGridNo, BOOLEAN fForceToPerson, BOOLEAN fDoor) {
  // psAdjustedGridNo gets the original gridno or the new one if updated
  // It will ONLY be updated IF we were over a merc, ( it's updated to their
  // gridno ) pubDirection gets the direction to the final gridno
  // fForceToPerson: forces the grid under consideration to be the one
  // occupiedby any target in that location, because we could be passed a gridno
  // based on the overlap of soldier's graphic fDoor determines whether special
  // door-handling code should be used (for interacting with doors)

  int16_t sDistance = 0;
  int16_t sDirs[4] = {NORTH, EAST, SOUTH, WEST};
  int32_t cnt;
  int16_t sClosest = NOWHERE, sSpot;
  int16_t sCloseGridNo = NOWHERE;
  uint8_t ubDir;
  STRUCTURE *pDoor;
  // STRUCTURE                            *pWall;
  uint8_t ubWallOrientation;
  BOOLEAN fCheckGivenGridNo = TRUE;
  uint8_t ubTestDirection;
  EXITGRID ExitGrid;

  // Set default direction
  if (pubDirection) {
    *pubDirection = pSoldier->bDirection;
  }

  // CHECK IF WE WANT TO FORCE GRIDNO TO PERSON
  if (psAdjustedGridNo != NULL) {
    *psAdjustedGridNo = sGridNo;
  }

  // CHECK IF IT'S THE SAME ONE AS WE'RE ON, IF SO, RETURN THAT!
  if (pSoldier->sGridNo == sGridNo && !FindStructure(sGridNo, STRUCTURE_SWITCH)) {
    /* OK, if we are looking for a door, it may be in the same tile as us, so
     * find the direction we have to face to get to the door, not just our
     * initial direction... */
    // If we are in the same tile as a switch, we can NEVER pull it....
    if (fDoor) {
      // This can only happen if a door was to the south to east of us!

      // Do south!
      // sSpot = NewGridNo( sGridNo, DirectionInc( SOUTH ) );

      // ATE: Added: Switch behave EXACTLY like doors
      pDoor = FindStructure(sGridNo, STRUCTURE_ANYDOOR);

      if (pDoor != NULL) {
        // Get orinetation
        ubWallOrientation = pDoor->ubWallOrientation;

        if (ubWallOrientation == OUTSIDE_TOP_LEFT || ubWallOrientation == INSIDE_TOP_LEFT) {
          // To the south!
          sSpot = NewGridNo(sGridNo, DirectionInc(SOUTH));
          if (pubDirection) {
            *pubDirection = GetDirectionFromGridNo(sSpot, pSoldier);
          }
        }

        if (ubWallOrientation == OUTSIDE_TOP_RIGHT || ubWallOrientation == INSIDE_TOP_RIGHT) {
          // TO the east!
          sSpot = NewGridNo(sGridNo, DirectionInc(EAST));
          if (pubDirection) {
            *pubDirection = GetDirectionFromGridNo(sSpot, pSoldier);
          }
        }
      }
    }

    // Use soldier's direction
    return sGridNo;
  }

  // Look for a door!
  if (fDoor) {
    pDoor = FindStructure(sGridNo, STRUCTURE_ANYDOOR | STRUCTURE_SWITCH);
  } else {
    pDoor = NULL;
  }

  if (fForceToPerson) {
    const SOLDIERTYPE *const s = FindSoldier(sGridNo, FIND_SOLDIER_GRIDNO);
    if (s != NULL) {
      sGridNo = s->sGridNo;
      if (psAdjustedGridNo != NULL) {
        *psAdjustedGridNo = sGridNo;

        // Use direction to this guy!
        if (pubDirection) {
          *pubDirection = GetDirectionFromGridNo(sGridNo, pSoldier);
        }
      }
    }
  }

  if (NewOKDestination(pSoldier, sGridNo, TRUE, pSoldier->bLevel) >
      0)  // no problem going there! nobody on it!
  {
    // OK, if we are looking to goto a switch, ignore this....
    if (pDoor) {
      if (pDoor->fFlags & STRUCTURE_SWITCH) {
        // Don't continuel
        fCheckGivenGridNo = FALSE;
      }
    }

    // If there is an exit grid....
    if (GetExitGrid(sGridNo, &ExitGrid)) {
      // Don't continuel
      fCheckGivenGridNo = FALSE;
    }

    if (fCheckGivenGridNo) {
      sDistance = PlotPath(pSoldier, sGridNo, NO_COPYROUTE, NO_PLOT, pSoldier->usUIMovementMode,
                           pSoldier->bActionPoints);
      if (sDistance > 0 && sDistance < sClosest) {
        sClosest = sDistance;
        sCloseGridNo = sGridNo;
      }
    }
  }

  for (cnt = 0; cnt < 4; cnt++) {
    // MOVE OUT TWO DIRECTIONS
    sSpot = NewGridNo(sGridNo, DirectionInc(sDirs[cnt]));

    ubTestDirection = sDirs[cnt];

    // For switches, ALLOW them to walk through walls to reach it....
    if (pDoor && pDoor->fFlags & STRUCTURE_SWITCH) {
      ubTestDirection = OppositeDirection(ubTestDirection);
    }

    if (fDoor) {
      if (gubWorldMovementCosts[sSpot][ubTestDirection][pSoldier->bLevel] >= TRAVELCOST_BLOCKED) {
        // obstacle or wall there!
        continue;
      }
    } else {
      // this function returns original MP cost if not a door cost
      if (DoorTravelCost(pSoldier, sSpot,
                         gubWorldMovementCosts[sSpot][ubTestDirection][pSoldier->bLevel], FALSE,
                         NULL) >= TRAVELCOST_BLOCKED) {
        // obstacle or wall there!
        continue;
      }
    }

    // Eliminate some directions if we are looking at doors!
    if (pDoor != NULL) {
      // Get orinetation
      ubWallOrientation = pDoor->ubWallOrientation;

      // Refuse the south and north and west  directions if our orientation is
      // top-right
      if (ubWallOrientation == OUTSIDE_TOP_RIGHT || ubWallOrientation == INSIDE_TOP_RIGHT) {
        if (sDirs[cnt] == NORTH || sDirs[cnt] == WEST || sDirs[cnt] == SOUTH) continue;
      }

      // Refuse the north and west and east directions if our orientation is
      // top-right
      if (ubWallOrientation == OUTSIDE_TOP_LEFT || ubWallOrientation == INSIDE_TOP_LEFT) {
        if (sDirs[cnt] == NORTH || sDirs[cnt] == WEST || sDirs[cnt] == EAST) continue;
      }
    }

    // If this spot is our soldier's gridno use that!
    if (sSpot == pSoldier->sGridNo) {
      // Use default diurection ) soldier's direction )

      // OK, at least get direction to face......
      // Defaults to soldier's facing dir unless we change it!
      // if ( pDoor != NULL )
      {
        // Use direction to the door!
        if (pubDirection) {
          *pubDirection = GetDirectionFromGridNo(sGridNo, pSoldier);
        }
      }
      return sSpot;
    }

    // don't store path, just measure it
    ubDir = (uint8_t)GetDirectionToGridNoFromGridNo(sSpot, sGridNo);

    if (NewOKDestinationAndDirection(pSoldier, sSpot, ubDir, TRUE, pSoldier->bLevel) > 0 &&
        (sDistance = PlotPath(pSoldier, sSpot, NO_COPYROUTE, NO_PLOT, pSoldier->usUIMovementMode,
                              pSoldier->bActionPoints)) > 0) {
      if (sDistance < sClosest) {
        sClosest = sDistance;
        sCloseGridNo = sSpot;
      }
    }
  }

  if (sClosest == NOWHERE) return -1;

  // Take last direction and use opposite!
  // This will be usefull for ours and AI mercs

  // If our gridno is the same ( which can be if we are look at doors )
  if (sGridNo == sCloseGridNo) {
    if (pubDirection) {
      // ATE: Only if we have a valid door!
      if (pDoor) {
        switch (pDoor->pDBStructureRef->pDBStructure->ubWallOrientation) {
          case OUTSIDE_TOP_LEFT:
          case INSIDE_TOP_LEFT:
            *pubDirection = SOUTH;
            break;
          case OUTSIDE_TOP_RIGHT:
          case INSIDE_TOP_RIGHT:
            *pubDirection = EAST;
            break;
        }
      }
    }
  } else {
    // Calculate direction if our gridno is different....
    ubDir = (uint8_t)GetDirectionToGridNoFromGridNo(sCloseGridNo, sGridNo);
    if (pubDirection) {
      *pubDirection = ubDir;
    }
  }
  // if ( psAdjustedGridNo != NULL )
  //{
  //		(*psAdjustedGridNo) = sCloseGridNo;
  //}
  if (sCloseGridNo == NOWHERE) return -1;
  return sCloseGridNo;
}

int16_t FindNextToAdjacentGridEx(SOLDIERTYPE *pSoldier, int16_t sGridNo, uint8_t *pubDirection,
                                 int16_t *psAdjustedGridNo, BOOLEAN fForceToPerson, BOOLEAN fDoor) {
  // This function works in a similar way as FindAdjacentGridEx, but looks for a
  // location 2 tiles away

  // psAdjustedGridNo gets the original gridno or the new one if updated
  // pubDirection gets the direction to the final gridno
  // fForceToPerson: forces the grid under consideration to be the one
  // occupiedby any target in that location, because we could be passed a gridno
  // based on the overlap of soldier's graphic fDoor determines whether special
  // door-handling code should be used (for interacting with doors)

  int16_t sDistance = 0;
  int16_t sDirs[4] = {NORTH, EAST, SOUTH, WEST};
  int32_t cnt;
  int16_t sClosest = WORLD_MAX, sSpot, sSpot2;
  int16_t sCloseGridNo = NOWHERE;
  uint8_t ubDir;
  STRUCTURE *pDoor;
  uint8_t ubWallOrientation;
  BOOLEAN fCheckGivenGridNo = TRUE;
  uint8_t ubTestDirection;

  // CHECK IF WE WANT TO FORCE GRIDNO TO PERSON
  if (psAdjustedGridNo != NULL) *psAdjustedGridNo = sGridNo;

  // CHECK IF IT'S THE SAME ONE AS WE'RE ON, IF SO, RETURN THAT!
  if (pSoldier->sGridNo == sGridNo) {
    if (pubDirection != NULL) *pubDirection = pSoldier->bDirection;
    return sGridNo;
  }

  // Look for a door!
  if (fDoor) {
    pDoor = FindStructure(sGridNo, STRUCTURE_ANYDOOR | STRUCTURE_SWITCH);
  } else {
    pDoor = NULL;
  }

  if (fForceToPerson) {
    const SOLDIERTYPE *const s = FindSoldier(sGridNo, FIND_SOLDIER_GRIDNO);
    if (s != NULL) {
      sGridNo = s->sGridNo;
      if (psAdjustedGridNo != NULL) *psAdjustedGridNo = sGridNo;
    }
  }

  if (NewOKDestination(pSoldier, sGridNo, TRUE, pSoldier->bLevel) >
      0)  // no problem going there! nobody on it!
  {
    // OK, if we are looking to goto a switch, ignore this....
    if (pDoor) {
      if (pDoor->fFlags & STRUCTURE_SWITCH) {
        // Don't continuel
        fCheckGivenGridNo = FALSE;
      }
    }

    if (fCheckGivenGridNo) {
      sDistance = PlotPath(pSoldier, sGridNo, NO_COPYROUTE, NO_PLOT, pSoldier->usUIMovementMode,
                           pSoldier->bActionPoints);
      if (sDistance > 0 && sDistance < sClosest) {
        sClosest = sDistance;
        sCloseGridNo = sGridNo;
      }
    }
  }

  for (cnt = 0; cnt < 4; cnt++) {
    // MOVE OUT TWO DIRECTIONS
    sSpot = NewGridNo(sGridNo, DirectionInc(sDirs[cnt]));

    ubTestDirection = sDirs[cnt];

    if (pDoor && pDoor->fFlags & STRUCTURE_SWITCH) {
      ubTestDirection = OppositeDirection(ubTestDirection);
    }

    if (gubWorldMovementCosts[sSpot][ubTestDirection][pSoldier->bLevel] >= TRAVELCOST_BLOCKED) {
      // obstacle or wall there!
      continue;
    }

    const SOLDIERTYPE *const tgt = WhoIsThere2(sSpot, pSoldier->bLevel);
    if (tgt != NULL && tgt != pSoldier) {
      // skip this direction b/c it's blocked by another merc!
      continue;
    }

    // Eliminate some directions if we are looking at doors!
    if (pDoor != NULL) {
      // Get orinetation
      ubWallOrientation = pDoor->ubWallOrientation;

      // Refuse the south and north and west  directions if our orientation is
      // top-right
      if (ubWallOrientation == OUTSIDE_TOP_RIGHT || ubWallOrientation == INSIDE_TOP_RIGHT) {
        if (sDirs[cnt] == NORTH || sDirs[cnt] == WEST || sDirs[cnt] == SOUTH) continue;
      }

      // Refuse the north and west and east directions if our orientation is
      // top-right
      if (ubWallOrientation == OUTSIDE_TOP_LEFT || ubWallOrientation == INSIDE_TOP_LEFT) {
        if (sDirs[cnt] == NORTH || sDirs[cnt] == WEST || sDirs[cnt] == EAST) continue;
      }
    }

    // first tile is okay, how about the second?
    sSpot2 = NewGridNo(sSpot, DirectionInc(sDirs[cnt]));
    if (gubWorldMovementCosts[sSpot2][sDirs[cnt]][pSoldier->bLevel] >= TRAVELCOST_BLOCKED ||
        DoorTravelCost(pSoldier, sSpot2,
                       gubWorldMovementCosts[sSpot2][sDirs[cnt]][pSoldier->bLevel],
                       pSoldier->bTeam == OUR_TEAM,
                       NULL) == TRAVELCOST_DOOR)  // closed door blocks!
    {
      // obstacle or wall there!
      continue;
    }

    const SOLDIERTYPE *const tgt2 = WhoIsThere2(sSpot2, pSoldier->bLevel);
    if (tgt2 != NULL && tgt2 != pSoldier) {
      // skip this direction b/c it's blocked by another merc!
      continue;
    }

    sSpot = sSpot2;

    // If this spot is our soldier's gridno use that!
    if (sSpot == pSoldier->sGridNo) {
      if (pubDirection) {
        *pubDirection = (uint8_t)GetDirectionFromGridNo(sGridNo, pSoldier);
      }
      //*pubDirection = pSoldier->bDirection;
      return (sSpot);
    }

    ubDir = GetDirectionToGridNoFromGridNo(sSpot, sGridNo);

    // don't store path, just measure it
    if (NewOKDestinationAndDirection(pSoldier, sSpot, ubDir, TRUE, pSoldier->bLevel) > 0 &&
        (sDistance = PlotPath(pSoldier, sSpot, NO_COPYROUTE, NO_PLOT, pSoldier->usUIMovementMode,
                              pSoldier->bActionPoints)) > 0) {
      if (sDistance < sClosest) {
        sClosest = sDistance;
        sCloseGridNo = sSpot;
      }
    }
  }

  if (sClosest == NOWHERE) return -1;

  // Take last direction and use opposite!
  // This will be usefull for ours and AI mercs

  // If our gridno is the same ( which can be if we are look at doors )
  if (sGridNo == sCloseGridNo) {
    if (pubDirection) {
      // ATE: Only if we have a valid door!
      if (pDoor) {
        switch (pDoor->pDBStructureRef->pDBStructure->ubWallOrientation) {
          case OUTSIDE_TOP_LEFT:
          case INSIDE_TOP_LEFT:
            *pubDirection = SOUTH;
            break;

          case OUTSIDE_TOP_RIGHT:
          case INSIDE_TOP_RIGHT:
            *pubDirection = EAST;
            break;
        }
      }
    }
  } else {
    // Calculate direction if our gridno is different....
    ubDir = (uint8_t)GetDirectionToGridNoFromGridNo(sCloseGridNo, sGridNo);
    if (pubDirection) {
      *pubDirection = ubDir;
    }
  }

  if (sCloseGridNo == NOWHERE) return -1;
  return sCloseGridNo;

  /*
  if (sCloseGridNo == NOWHERE) return -1;

  // Take last direction and use opposite!
  // This will be usefull for ours and AI mercs

  // If our gridno is the same ( which can be if we are look at doors )
  if ( sGridNo == sCloseGridNo )
  {
          switch( pDoor->pDBStructureRef->pDBStructure->ubWallOrientation )
          {
                  case OUTSIDE_TOP_LEFT:
                  case INSIDE_TOP_LEFT:
                          *pubDirection = SOUTH;
                          break;

                  case OUTSIDE_TOP_RIGHT:
                  case INSIDE_TOP_RIGHT:
                          *pubDirection = EAST;
                          break;
          }
  }
  else
  {
          // Calculate direction if our gridno is different....
          ubDir = (uint8_t)GetDirectionToGridNoFromGridNo( sCloseGridNo, sGridNo
  ); *pubDirection = ubDir;
  }
  return sCloseGridNo;
  */
}

int16_t FindAdjacentPunchTarget(const SOLDIERTYPE *const pSoldier,
                                const SOLDIERTYPE *const pTargetSoldier,
                                int16_t *const psAdjustedTargetGridNo) {
  Assert(pTargetSoldier != NULL);

  for (int16_t i = 0; i < NUM_WORLD_DIRECTIONS; ++i) {
    const int16_t sSpot = NewGridNo(pSoldier->sGridNo, DirectionInc(i));

    if (DoorTravelCost(pSoldier, sSpot, gubWorldMovementCosts[sSpot][i][pSoldier->bLevel], FALSE,
                       NULL) >= TRAVELCOST_BLOCKED) {
      // blocked!
      continue;
    }

    // Check for who is there...
    if (pTargetSoldier == WhoIsThere2(sSpot, pSoldier->bLevel)) {
      // We've got a guy here....
      // Who is the one we want......
      *psAdjustedTargetGridNo = pTargetSoldier->sGridNo;
      return sSpot;
    }
  }

  return NOWHERE;
}

BOOLEAN UIOKMoveDestination(const SOLDIERTYPE *pSoldier, uint16_t usMapPos) {
  if (!NewOKDestination(pSoldier, usMapPos, FALSE, (int8_t)gsInterfaceLevel)) {
    return (FALSE);
  }

  // ATE: If we are a robot, see if we are being validly controlled...
  if (pSoldier->uiStatusFlags & SOLDIER_ROBOT) {
    if (!CanRobotBeControlled(pSoldier)) {
      // Display message that robot cannot be controlled....
      return (2);
    }
  }

  // ATE: Experiment.. take out
  // else if ( IsRoofVisible( usMapPos ) && gsInterfaceLevel == 0 )
  //{
  //	return( FALSE );
  //}

  return (TRUE);
}

void HandleTeamServices(uint8_t ubTeamNum) {
  FOR_EACH_IN_TEAM(i, ubTeamNum) {
    SOLDIERTYPE &s = *i;
    if (s.bInSector) HandlePlayerServices(s);
  }
}

void HandlePlayerServices(SOLDIERTYPE &s) {
  if (s.bLife < OKLIFE) return;
  if (s.usAnimState != GIVING_AID) return;

  SOLDIERTYPE *const tgt = WhoIsThere2(s.sTargetGridNo, s.bLevel);
  if (!tgt) return;
  if (tgt->ubServiceCount == 0) return;

  OBJECTTYPE &in_hand = s.inv[HANDPOS];
  uint16_t const kit_pts = TotalPoints(&in_hand);
  uint32_t const points_used = SoldierDressWound(&s, tgt, kit_pts, kit_pts);

  // Determine if they are all banagded
  bool done = FALSE;
  if (tgt->bBleeding == 0 && tgt->bLife >= OKLIFE) {
    ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, TacticalStr[MERC_IS_ALL_BANDAGED_STR],
              tgt->name);
    // Cancel all services for this guy!
    ReceivingSoldierCancelServices(tgt);
    done = TRUE;
  }

  UseKitPoints(in_hand, points_used, s);

  // Whether or not recipient is all bandaged, check if we've used them up
  if (TotalPoints(&in_hand) > 0) return;
  // No more bandages

  int8_t slot;
  if (done) {  // Don't swap if we're done
    slot = NO_SLOT;
  } else {
    slot = FindObj(&s, FIRSTAIDKIT);
    if (slot == NO_SLOT) {
      slot = FindObj(&s, MEDICKIT);
    }
  }

  if (slot != NO_SLOT) {
    SwapObjs(&in_hand, &s.inv[slot]);
  } else {
    ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, TacticalStr[MERC_IS_OUT_OF_BANDAGES_STR],
              s.name);
    GivingSoldierCancelServices(&s);
    if (!gTacticalStatus.fAutoBandageMode) {
      DoMercBattleSound(&s, BATTLE_SOUND_CURSE1);
    }
  }
}

void CommonEnterCombatModeCode() {
  gTacticalStatus.uiFlags |= INCOMBAT;

  // gTacticalStatus.ubAttackBusyCount = 0;

  // Reset num enemies fought flag...
  memset(&(gTacticalStatus.bNumFoughtInBattle), 0, MAXTEAMS);
  gTacticalStatus.ubLastBattleSectorX = (uint8_t)gWorldSectorX;
  gTacticalStatus.ubLastBattleSectorY = (uint8_t)gWorldSectorY;
  gTacticalStatus.fLastBattleWon = FALSE;
  gTacticalStatus.fItemsSeenOnAttack = FALSE;

  // ATE: If we have an item pointer end it!
  CancelItemPointer();

  ResetInterfaceAndUI();
  ResetMultiSelection();

  // OK, loop thorugh all guys and stop them!
  // Loop through all mercs and make go
  FOR_EACH_SOLDIER(pSoldier) {
    if (pSoldier->bInSector && pSoldier->ubBodyType != CROW) {
      // Set some flags for quotes
      pSoldier->usQuoteSaidFlags &= (~SOLDIER_QUOTE_SAID_IN_SHIT);
      pSoldier->usQuoteSaidFlags &= (~SOLDIER_QUOTE_SAID_MULTIPLE_CREATURES);

      // Hault!
      EVENT_StopMerc(pSoldier);

      // END AI actions
      CancelAIAction(pSoldier);

      // turn off AI controlled flag
      pSoldier->uiStatusFlags &= ~SOLDIER_UNDERAICONTROL;

      // Check if this guy is an enemy....
      CheckForPotentialAddToBattleIncrement(pSoldier);

      // If guy is sleeping, wake him up!
      if (pSoldier->fMercAsleep) {
        ChangeSoldierState(pSoldier, WKAEUP_FROM_SLEEP, 1, TRUE);
      }

      // ATE: Refresh APs
      CalcNewActionPoints(pSoldier);

      if (pSoldier->ubProfile != NO_PROFILE) {
        if (pSoldier->bTeam == CIV_TEAM && pSoldier->bNeutral) {
          // only set precombat gridno if unset
          if (gMercProfiles[pSoldier->ubProfile].sPreCombatGridNo == 0 ||
              gMercProfiles[pSoldier->ubProfile].sPreCombatGridNo == NOWHERE) {
            gMercProfiles[pSoldier->ubProfile].sPreCombatGridNo = pSoldier->sGridNo;
          }
        } else {
          gMercProfiles[pSoldier->ubProfile].sPreCombatGridNo = NOWHERE;
        }
      }

      if (!gTacticalStatus.fHasEnteredCombatModeSinceEntering) {
        // ATE: reset player's movement mode at the very start of
        // combat
        // if ( pSoldier->bTeam == OUR_TEAM )
        //{
        // pSoldier->usUIMovementMode = RUNNING;
        //}
      }
    }
  }

  gTacticalStatus.fHasEnteredCombatModeSinceEntering = TRUE;

  SyncStrategicTurnTimes();

  // Play tune..
  PlayJA2Sample(ENDTURN_1, MIDVOLUME, 1, MIDDLEPAN);

  // Say quote.....

  // Change music modes
  SetMusicMode(MUSIC_TACTICAL_BATTLE);
}

void EnterCombatMode(uint8_t ubStartingTeam) {
  if (gTacticalStatus.uiFlags & INCOMBAT) {
    DebugMsg(TOPIC_JA2, DBG_LEVEL_3, "Can't enter combat when already in combat");
    // we're already in combat!
    return;
  }

  // Alrighty, don't do this if no enemies in sector
  if (NumCapableEnemyInSector() == 0) {
    DebugMsg(TOPIC_JA2, DBG_LEVEL_3, "Can't enter combat when no capable enemies");
    // ScreenMsg( MSG_FONT_RED, MSG_DEBUG, L"Trying to init combat when no
    // enemies around!." );
    return;
  }

  DebugMsg(TOPIC_JA2, DBG_LEVEL_3, "Entering combat mode");

  // ATE: Added here to guarentee we have fEnemyInSector
  // Mostly this was not getting set if:
  // 1 ) we see bloodcats ( which makes them hostile )
  // 2 ) we make civs hostile
  // only do this once they are seen.....
  SetEnemyPresence();

  CommonEnterCombatModeCode();

  if (ubStartingTeam == OUR_TEAM) {
    // OK, make sure we have a selected guy
    const SOLDIERTYPE *const sel = GetSelectedMan();
    if (sel == NULL || sel->bOppCnt == 0) {
      // OK, look through and find one....
      FOR_EACH_IN_TEAM(s, OUR_TEAM) {
        if (OkControllableMerc(s) && s->bOppCnt > 0) {
          SelectSoldier(s, SELSOLDIER_FORCE_RESELECT);
        }
      }
    }

    StartPlayerTeamTurn(FALSE, TRUE);
  } else {
    // have to call EndTurn so that we freeze the interface etc
    EndTurn(ubStartingTeam);
  }
}

void ExitCombatMode() {
  DebugMsg(TOPIC_JA2, DBG_LEVEL_3, "Exiting combat mode");

  // Leave combat mode
  gTacticalStatus.uiFlags &= (~INCOMBAT);

  EndTopMessage();

  // OK, we have exited combat mode.....
  // Reset some flags for no aps to move, etc

  // Set virgin sector to true....
  gTacticalStatus.fVirginSector = TRUE;

  FOR_EACH_SOLDIER(pSoldier) {
    if (pSoldier->bInSector) {
      // Reset some flags
      if (pSoldier->fNoAPToFinishMove && pSoldier->bLife >= OKLIFE) {
        AdjustNoAPToFinishMove(pSoldier, FALSE);
        SoldierGotoStationaryStance(pSoldier);
      }

      // Cancel pending events
      pSoldier->usPendingAnimation = NO_PENDING_ANIMATION;
      pSoldier->ubPendingDirection = NO_PENDING_DIRECTION;
      pSoldier->ubPendingAction = NO_PENDING_ACTION;

      // Reset moved flag
      pSoldier->bMoved = FALSE;

      // Set final destination
      pSoldier->sFinalDestination = pSoldier->sGridNo;

      // remove AI controlled flag
      pSoldier->uiStatusFlags &= ~SOLDIER_UNDERAICONTROL;
    }
  }

  // Change music modes
  gfForceMusicToTense = TRUE;

  SetMusicMode(MUSIC_TACTICAL_ENEMYPRESENT);

  BetweenTurnsVisibilityAdjustments();

  // pause the AI for a bit
  PauseAITemporarily();

  // reset muzzle flashes
  TurnOffEveryonesMuzzleFlashes();

  // zap interrupt list
  ClearIntList();

  fInterfacePanelDirty = DIRTYLEVEL2;

  // ATE: If we are IN_CONV - DONT'T DO THIS!
  if (!(gTacticalStatus.uiFlags & ENGAGED_IN_CONV)) {
    HandleStrategicTurnImplicationsOfExitingCombatMode();
  }

  // Make sure next opplist decay DOES happen right after we go to RT
  // since this would be the same as what would happen at the end of the turn
  gTacticalStatus.uiTimeSinceLastOpplistDecay =
      std::max((uint32_t)0, (uint32_t)(GetWorldTotalSeconds() - TIME_BETWEEN_RT_OPPLIST_DECAYS));
  NonCombatDecayPublicOpplist(GetWorldTotalSeconds());
}

void SetEnemyPresence() {
  // We have an ememy present....

  // Check if we previously had no enemys present and we are in a virgin secotr
  // ( no enemys spotted yet )
  if (!gTacticalStatus.fEnemyInSector && gTacticalStatus.fVirginSector) {
    // If we have a guy selected, say quote!
    // For now, display ono status message
    ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, TacticalStr[ENEMY_IN_SECTOR_STR]);

    // Change music modes..

    // If we are just starting game, don't do this!
    if (!DidGameJustStart() && !AreInMeanwhile()) {
      SetMusicMode(MUSIC_TACTICAL_ENEMYPRESENT);
    }

    // Say quote...
    // SayQuoteFromAnyBodyInSector( QUOTE_ENEMY_PRESENCE );

    gTacticalStatus.fEnemyInSector = TRUE;
  }
}

static bool SoldierHasSeenEnemiesLastFewTurns(SOLDIERTYPE const &s) {
  for (int32_t team = 0; team != MAXTEAMS; ++team) {
    if (gTacticalStatus.Team[team].bSide == s.bSide) continue;

    // Check this team for possible enemies
    CFOR_EACH_IN_TEAM(i, team) {
      SOLDIERTYPE const &other = *i;
      if (!other.bInSector) continue;
      if (other.bTeam != OUR_TEAM && other.bLife < OKLIFE) continue;
      if (CONSIDERED_NEUTRAL(&s, &other)) continue;
      if (s.bSide == other.bSide) continue;
      // Have we not seen this guy
      int8_t const seen = s.bOppList[other.ubID];
      if (seen < SEEN_CURRENTLY || SEEN_THIS_TURN < seen) continue;

      gTacticalStatus.bConsNumTurnsNotSeen = 0;
      return true;
    }
  }

  return false;
}

static BOOLEAN WeSeeNoOne() {
  FOR_EACH_MERC(i) {
    const SOLDIERTYPE *const s = *i;
    if (s->bTeam == OUR_TEAM && s->bOppCnt > 0) return FALSE;
  }
  return TRUE;
}

static BOOLEAN NobodyAlerted() {
  FOR_EACH_MERC(i) {
    const SOLDIERTYPE *const s = *i;
    if (s->bTeam != OUR_TEAM && !s->bNeutral && s->bLife >= OKLIFE &&
        s->bAlertStatus >= STATUS_RED) {
      return FALSE;
    }
  }
  return (TRUE);
}

static BOOLEAN WeSawSomeoneThisTurn() {
  FOR_EACH_MERC(i) {
    const SOLDIERTYPE *const s = *i;
    if (s->bTeam != OUR_TEAM) continue;

    for (uint32_t uiLoop2 = gTacticalStatus.Team[ENEMY_TEAM].bFirstID; uiLoop2 < TOTAL_SOLDIERS;
         ++uiLoop2) {
      if (s->bOppList[uiLoop2] == SEEN_THIS_TURN) return TRUE;
    }
  }
  return FALSE;
}

static void SayBattleSoundFromAnyBodyInSector(BattleSound const iBattleSnd) {
  uint8_t ubNumMercs = 0;

  // Loop through all our guys and randomly say one from someone in our sector
  SOLDIERTYPE *mercs_in_sector[20];
  FOR_EACH_IN_TEAM(s, OUR_TEAM) {
    // Add guy if he's a candidate...
    if (OkControllableMerc(s) && !AM_AN_EPC(s) && !(s->uiStatusFlags & SOLDIER_GASSED) &&
        !AM_A_ROBOT(s) && !s->fMercAsleep) {
      mercs_in_sector[ubNumMercs++] = s;
    }
  }

  if (ubNumMercs > 0) {
    SOLDIERTYPE *const chosen = mercs_in_sector[Random(ubNumMercs)];
    DoMercBattleSound(chosen, iBattleSnd);
  }
}

static uint8_t NumBloodcatsInSectorNotDeadOrDying();
static uint8_t NumEnemyInSectorNotDeadOrDying();

BOOLEAN CheckForEndOfCombatMode(BOOLEAN fIncrementTurnsNotSeen) {
  BOOLEAN fWeSeeNoOne, fNobodyAlerted;
  BOOLEAN fSayQuote = FALSE;
  BOOLEAN fWeSawSomeoneRecently = FALSE, fSomeoneSawSomeoneRecently = FALSE;

  // We can only check for end of combat if in combat mode
  if (!(gTacticalStatus.uiFlags & INCOMBAT)) {
    return (FALSE);
  }

  // if we're boxing NEVER end combat mode
  if (gTacticalStatus.bBoxingState == BOXING) {
    return (FALSE);
  }

  // First check for end of battle....
  // If there are no enemies at all in the sector
  // Battle end should take presedence!
  if (CheckForEndOfBattle(FALSE)) {
    return (TRUE);
  }

  fWeSeeNoOne = WeSeeNoOne();
  fNobodyAlerted = NobodyAlerted();
  if (fWeSeeNoOne && fNobodyAlerted) {
    // hack!!!
    gTacticalStatus.bConsNumTurnsNotSeen = 5;
  } else {
    // we have to loop through EVERYONE to see if anyone sees a hostile... if
    // so, stay in turnbased...
    FOR_EACH_MERC(i) {
      SOLDIERTYPE const &s = **i;
      if (s.bLife >= OKLIFE && !s.bNeutral && SoldierHasSeenEnemiesLastFewTurns(s)) {
        gTacticalStatus.bConsNumTurnsNotSeen = 0;
        fSomeoneSawSomeoneRecently = TRUE;
        if (s.bTeam == OUR_TEAM ||
            (s.bTeam == MILITIA_TEAM && s.bSide == 0))  // or friendly militia
        {
          fWeSawSomeoneRecently = TRUE;
          break;
        }
      }
    }

    if (fSomeoneSawSomeoneRecently) {
      if (fWeSawSomeoneRecently) {
        gTacticalStatus.bConsNumTurnsWeHaventSeenButEnemyDoes = 0;
      } else {
        // start tracking this
        gTacticalStatus.bConsNumTurnsWeHaventSeenButEnemyDoes++;
      }
      return (FALSE);
    }

    // IF here, we don;t see anybody.... increment count for end check
    if (fIncrementTurnsNotSeen) {
      gTacticalStatus.bConsNumTurnsNotSeen++;
    }
  }

  gTacticalStatus.bConsNumTurnsWeHaventSeenButEnemyDoes = 0;

  // If we have reach a point where a cons. number of turns gone by....
  if (gTacticalStatus.bConsNumTurnsNotSeen > 1) {
    gTacticalStatus.bConsNumTurnsNotSeen = 0;

    // Exit mode!
    ExitCombatMode();

    if (fNobodyAlerted) {
      // if we don't see anyone currently BUT we did see someone this turn, THEN
      // don't say quote
      if (fWeSeeNoOne && WeSawSomeoneThisTurn()) {
        // don't say quote
      } else {
        fSayQuote = TRUE;
      }
    } else {
      fSayQuote = TRUE;
    }

    // ATE: Are there creatures here? If so, say another quote...
    if (fSayQuote && (gTacticalStatus.uiFlags & IN_CREATURE_LAIR)) {
      SayQuoteFromAnyBodyInSector(QUOTE_WORRIED_ABOUT_CREATURE_PRESENCE);
    }
    // Are we fighting bloodcats?
    else if (NumBloodcatsInSectorNotDeadOrDying() > 0) {
    } else {
      if (fSayQuote) {
        // Double check by seeing if we have any active enemies in sector!
        if (NumEnemyInSectorNotDeadOrDying() > 0) {
          SayQuoteFromAnyBodyInSector(QUOTE_WARNING_OUTSTANDING_ENEMY_AFTER_RT);
        }
      }
    }
    /*
                    if ( (!fWeSeeNoOne || !fNobodyAlerted) &&
       WeSawSomeoneThisTurn() )
                    {
                            // Say quote to the effect that the battle has
       lulled SayQuoteFromAnyBodyInSector(
       QUOTE_WARNING_OUTSTANDING_ENEMY_AFTER_RT );
                    }
    */

    // Begin tense music....
    gfForceMusicToTense = TRUE;
    SetMusicMode(MUSIC_TACTICAL_ENEMYPRESENT);

    return (TRUE);
  }

  return (FALSE);
}

static void DeathNoMessageTimerCallback() { CheckAndHandleUnloadingOfCurrentWorld(); }

static BOOLEAN CheckForLosingEndOfBattle();
static bool KillIncompacitatedEnemyInSector();
static uint8_t NumEnemyInSectorExceptCreatures();

//!!!!
// IMPORTANT NEW NOTE:
// Whenever returning TRUE, make sure you clear gfBlitBattleSectorLocator;
BOOLEAN CheckForEndOfBattle(BOOLEAN fAnEnemyRetreated) {
  BOOLEAN fBattleWon = TRUE;
  BOOLEAN fBattleLost = FALSE;
  uint16_t usAnimState;

  if (gTacticalStatus.bBoxingState == BOXING) {
    // no way are we going to exit boxing prematurely, thank you! :-)
    return (FALSE);
  }

  // We can only check for end of battle if in combat mode or there are enemies
  // present (they might bleed to death or run off the map!)
  if (!(gTacticalStatus.uiFlags & INCOMBAT)) {
    if (!(gTacticalStatus.fEnemyInSector)) {
      return (FALSE);
    }
  }

  // ATE: If attack busy count.. get out...
  if ((gTacticalStatus.ubAttackBusyCount > 0)) {
    return (FALSE);
  }

  // OK, this is to releave infinate looping...becasue we can kill guys in this
  // function
  if (gfKillingGuysForLosingBattle) {
    return (FALSE);
  }

  // Check if the battle is won!
  if (NumCapableEnemyInSector() > 0) {
    fBattleWon = FALSE;
  }

  if (CheckForLosingEndOfBattle()) {
    fBattleLost = TRUE;
  }

  // NEW (Nov 24, 98)  by Kris
  if (!gbWorldSectorZ && fBattleWon) {  // Check to see if more enemy soldiers
                                        // exist in the strategic layer
    // It is possible to have more than 20 enemies in a sector.  By failing
    // here, it gives the engine a chance to add these soldiers as
    // reinforcements.  This is naturally handled.
    if (gfPendingEnemies) {
      fBattleWon = FALSE;
    }
  }

  // We should NEVER have a battle lost and won at the same time...

  if (fBattleLost) {
    // CJC: End AI's turn here.... first... so that UnSetUIBusy will succeed if
    // militia win battle for us
    EndAllAITurns();

    // Set enemy presence to false
    // This is safe 'cause we're about to unload the friggen sector anyway....
    gTacticalStatus.fEnemyInSector = FALSE;

    // If here, the battle has been lost!
    UnSetUIBusy(GetSelectedMan());

    if (gTacticalStatus.uiFlags & INCOMBAT) {
      // Exit mode!
      ExitCombatMode();
    }

    HandleMoraleEvent(NULL, MORALE_HEARD_BATTLE_LOST, gWorldSectorX, gWorldSectorY, gbWorldSectorZ);
    HandleGlobalLoyaltyEvent(GLOBAL_LOYALTY_BATTLE_LOST, gWorldSectorX, gWorldSectorY,
                             gbWorldSectorZ);

    // Play death music
    SetMusicMode(MUSIC_TACTICAL_DEATH);
    SetCustomizableTimerCallbackAndDelay(10000, DeathNoMessageTimerCallback, FALSE);

    if (CheckFact(FACT_FIRST_BATTLE_BEING_FOUGHT, 0)) {
      // this is our first battle... and we lost it!
      SetFactTrue(FACT_FIRST_BATTLE_FOUGHT);
      SetFactFalse(FACT_FIRST_BATTLE_BEING_FOUGHT);
      SetTheFirstBattleSector((int16_t)(gWorldSectorX + gWorldSectorY * MAP_WORLD_X));
      HandleFirstBattleEndingWhileInTown(gWorldSectorX, gWorldSectorY, gbWorldSectorZ, FALSE);
    }

    if (NumEnemyInSectorExceptCreatures()) {
      SetThisSectorAsEnemyControlled(gWorldSectorX, gWorldSectorY, gbWorldSectorZ);
    }

    // ATE: Important! THis is delayed until music ends so we can have proper
    // effect! CheckAndHandleUnloadingOfCurrentWorld();

    // Whenever returning TRUE, make sure you clear gfBlitBattleSectorLocator;
    LogBattleResults(LOG_DEFEAT);
    gfBlitBattleSectorLocator = FALSE;
    return (TRUE);
  }

  // If battle won, do stuff right away!
  if (fBattleWon) {
    if (gTacticalStatus.bBoxingState == NOT_BOXING)  // if boxing don't do any of this stuff
    {
      gTacticalStatus.fLastBattleWon = TRUE;

      // OK, KILL any enemies that are incompacitated
      if (KillIncompacitatedEnemyInSector()) {
        return (FALSE);
      }
    }

    // If here, the battle has been won!
    // hurray! a glorious victory!

    // Set enemy presence to false
    gTacticalStatus.fEnemyInSector = FALSE;

    // CJC: End AI's turn here.... first... so that UnSetUIBusy will succeed if
    // militia win battle for us
    EndAllAITurns();

    UnSetUIBusy(GetSelectedMan());

    // ATE:
    // If we ended battle in any team other than the player's
    // we need to end the UI lock using this method....
    guiPendingOverrideEvent = LU_ENDUILOCK;
    HandleTacticalUI();

    if (gTacticalStatus.uiFlags & INCOMBAT) {
      // Exit mode!
      ExitCombatMode();
    }

    if (gTacticalStatus.bBoxingState == NOT_BOXING)  // if boxing don't do any of this stuff
    {
      // Only do some stuff if we actually faught a battle
      if (gTacticalStatus.bNumFoughtInBattle[ENEMY_TEAM] +
              gTacticalStatus.bNumFoughtInBattle[CREATURE_TEAM] +
              gTacticalStatus.bNumFoughtInBattle[CIV_TEAM] >
          0)
      // if ( gTacticalStatus.bNumEnemiesFoughtInBattle > 0 )
      {
        FOR_EACH_IN_TEAM(pTeamSoldier, OUR_TEAM) {
          if (pTeamSoldier->bInSector && pTeamSoldier->bTeam == OUR_TEAM) {
            gMercProfiles[pTeamSoldier->ubProfile].usBattlesFought++;

            // If this guy is OKLIFE & not standing, make stand....
            if (pTeamSoldier->bLife >= OKLIFE && !pTeamSoldier->bCollapsed &&
                pTeamSoldier->bAssignment < ON_DUTY) {
              // Reset some quote flags....
              pTeamSoldier->usQuoteSaidExtFlags &= ~SOLDIER_QUOTE_SAID_BUDDY_1_WITNESSED;
              pTeamSoldier->usQuoteSaidExtFlags &= ~SOLDIER_QUOTE_SAID_BUDDY_2_WITNESSED;
              pTeamSoldier->usQuoteSaidExtFlags &= ~SOLDIER_QUOTE_SAID_BUDDY_3_WITNESSED;

              // toggle stealth mode....
              gfUIStanceDifferent = TRUE;
              pTeamSoldier->bStealthMode = FALSE;
              fInterfacePanelDirty = DIRTYLEVEL2;

              if (gAnimControl[pTeamSoldier->usAnimState].ubHeight != ANIM_STAND) {
                ChangeSoldierStance(pTeamSoldier, ANIM_STAND);
              } else {
                // If they are aiming, end aim!
                usAnimState = PickSoldierReadyAnimation(pTeamSoldier, TRUE);

                if (usAnimState != INVALID_ANIMATION) {
                  EVENT_InitNewSoldierAnim(pTeamSoldier, usAnimState, 0, FALSE);
                }
              }
            }
          }
        }

        HandleMoraleEvent(NULL, MORALE_BATTLE_WON, gWorldSectorX, gWorldSectorY, gbWorldSectorZ);
        HandleGlobalLoyaltyEvent(GLOBAL_LOYALTY_BATTLE_WON, gWorldSectorX, gWorldSectorY,
                                 gbWorldSectorZ);

        // Change music modes
        if (gLastMercTalkedAboutKilling == NULL ||
            !(gLastMercTalkedAboutKilling->uiStatusFlags & SOLDIER_MONSTER)) {
          SetMusicMode(MUSIC_TACTICAL_VICTORY);
        }
        ShouldBeginAutoBandage();

        // Say battle end quote....

        if (fAnEnemyRetreated) {
          SayQuoteFromAnyBodyInSector(QUOTE_ENEMY_RETREATED);
        } else {
          // OK, If we have just finished a battle with creatures........ play
          // killed creature quote...
          //
          if (gLastMercTalkedAboutKilling != NULL &&
              gLastMercTalkedAboutKilling->uiStatusFlags & SOLDIER_MONSTER) {
          }
          if (gLastMercTalkedAboutKilling != NULL &&
              gLastMercTalkedAboutKilling->ubBodyType == BLOODCAT) {
            SayBattleSoundFromAnyBodyInSector(BATTLE_SOUND_COOL1);
          } else {
            SayQuoteFromAnyBodyInSector(QUOTE_SECTOR_SAFE);
          }
        }

      } else {
        // Change to nothing music...
        SetMusicMode(MUSIC_TACTICAL_NOTHING);
        ShouldBeginAutoBandage();
      }

      // Loop through all militia and restore them to peaceful status
      FOR_EACH_IN_TEAM(pTeamSoldier, MILITIA_TEAM) {
        if (pTeamSoldier->bInSector) {
          pTeamSoldier->bAlertStatus = STATUS_GREEN;
          CheckForChangingOrders(pTeamSoldier);
          pTeamSoldier->sNoiseGridno = NOWHERE;
          pTeamSoldier->ubNoiseVolume = 0;
          pTeamSoldier->bNewSituation = FALSE;
          pTeamSoldier->bOrders = STATIONARY;
          if (pTeamSoldier->bLife >= OKLIFE) {
            pTeamSoldier->bBleeding = 0;
          }
        }
      }
      gTacticalStatus.Team[MILITIA_TEAM].bAwareOfOpposition = FALSE;

      // Loop through all civs and restore them to peaceful status
      FOR_EACH_IN_TEAM(pTeamSoldier, CIV_TEAM) {
        if (pTeamSoldier->bInSector) {
          pTeamSoldier->bAlertStatus = STATUS_GREEN;
          pTeamSoldier->sNoiseGridno = NOWHERE;
          pTeamSoldier->ubNoiseVolume = 0;
          pTeamSoldier->bNewSituation = FALSE;
          CheckForChangingOrders(pTeamSoldier);
        }
      }
      gTacticalStatus.Team[CIV_TEAM].bAwareOfOpposition = FALSE;
    }

    fInterfacePanelDirty = DIRTYLEVEL2;

    if (gTacticalStatus.bBoxingState == NOT_BOXING)  // if boxing don't do any of this stuff
    {
      LogBattleResults(LOG_VICTORY);

      SetThisSectorAsPlayerControlled(gWorldSectorX, gWorldSectorY, gbWorldSectorZ, TRUE);
      HandleVictoryInNPCSector(gWorldSectorX, gWorldSectorY, (int16_t)gbWorldSectorZ);
      if (CheckFact(FACT_FIRST_BATTLE_BEING_FOUGHT, 0)) {
        // ATE: Need to trigger record for this event .... for NPC scripting
        TriggerNPCRecord(PACOS, 18);

        // this is our first battle... and we won!
        SetFactTrue(FACT_FIRST_BATTLE_FOUGHT);
        SetFactTrue(FACT_FIRST_BATTLE_WON);
        SetFactFalse(FACT_FIRST_BATTLE_BEING_FOUGHT);
        SetTheFirstBattleSector((int16_t)(gWorldSectorX + gWorldSectorY * MAP_WORLD_X));
        HandleFirstBattleEndingWhileInTown(gWorldSectorX, gWorldSectorY, gbWorldSectorZ, FALSE);
      }
    }

    // Whenever returning TRUE, make sure you clear gfBlitBattleSectorLocator;
    gfBlitBattleSectorLocator = FALSE;
    return (TRUE);
  }

  return (FALSE);
}

void CycleThroughKnownEnemies() {
  // static to indicate last position we were at:
  static BOOLEAN fFirstTime = TRUE;
  static uint16_t usStartToLook;
  BOOLEAN fEnemyBehindStartLook = FALSE;
  BOOLEAN fEnemiesFound = FALSE;

  if (fFirstTime) {
    fFirstTime = FALSE;

    usStartToLook = gTacticalStatus.Team[OUR_TEAM].bLastID;
  }

  FOR_EACH_NON_PLAYER_SOLDIER(s) {
    // try to find first active, OK enemy
    if (s->bInSector && !s->bNeutral && s->bSide != OUR_TEAM && s->bLife > 0 && s->bVisible != -1) {
      fEnemiesFound = TRUE;

      // If we are > ok start, this is the one!
      if (s->ubID > usStartToLook) {
        usStartToLook = s->ubID;
        SlideTo(s, SETANDREMOVEPREVIOUSLOCATOR);
        return;
      } else {
        fEnemyBehindStartLook = TRUE;
      }
    }
  }

  if (!fEnemiesFound) {
    ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_UI_FEEDBACK, TacticalStr[NO_ENEMIES_IN_SIGHT_STR]);
  }

  // If here, we found nobody, but there may be someone behind
  // If to, recurse!
  if (fEnemyBehindStartLook) {
    usStartToLook = gTacticalStatus.Team[OUR_TEAM].bLastID;

    CycleThroughKnownEnemies();
  }
}

void CycleVisibleEnemies(SOLDIERTYPE *pSrcSoldier) {
  FOR_EACH_NON_PLAYER_SOLDIER(s) {
    // try to find first active, OK enemy
    if (s->bInSector && !s->bNeutral && s->bSide != OUR_TEAM && s->bLife > 0 &&
        pSrcSoldier->bOppList[s->ubID] == SEEN_CURRENTLY &&
        s->ubID > pSrcSoldier->ubLastEnemyCycledID) {
      pSrcSoldier->ubLastEnemyCycledID = s->ubID;
      SlideTo(s, SETANDREMOVEPREVIOUSLOCATOR);
      ChangeInterfaceLevel(s->bLevel);
      return;
    }
  }

  // If here.. reset to zero...
  pSrcSoldier->ubLastEnemyCycledID = 0;

  FOR_EACH_NON_PLAYER_SOLDIER(s) {
    // try to find first active, OK enemy
    if (s->bInSector && !s->bNeutral && s->bSide != OUR_TEAM && s->bLife > 0 &&
        pSrcSoldier->bOppList[s->ubID] == SEEN_CURRENTLY &&
        s->ubID > pSrcSoldier->ubLastEnemyCycledID) {
      pSrcSoldier->ubLastEnemyCycledID = s->ubID;
      SlideTo(s, SETANDREMOVEPREVIOUSLOCATOR);
      ChangeInterfaceLevel(s->bLevel);
      return;
    }
  }
}

uint32_t NumberOfMercsOnPlayerTeam() {
  int8_t bNumber = 0;
  CFOR_EACH_IN_TEAM(s, OUR_TEAM) {
    if (!(s->uiStatusFlags & SOLDIER_VEHICLE)) bNumber++;
  }
  return bNumber;
}

BOOLEAN PlayerTeamFull() {
  // last ID for the player team is 19, so long as we have at most 17
  // non-vehicles...
  if (NumberOfMercsOnPlayerTeam() <= gTacticalStatus.Team[OUR_TEAM].bLastID - 2) {
    return (FALSE);
  }

  return (TRUE);
}

uint8_t NumPCsInSector() {
  uint8_t ubNumPlayers = 0;
  FOR_EACH_MERC(i) {
    const SOLDIERTYPE *const s = *i;
    if (s->bTeam == OUR_TEAM && s->bLife > 0) ++ubNumPlayers;
  }
  return ubNumPlayers;
}

uint8_t NumEnemyInSector() {
  uint8_t n_enemies = 0;
  CFOR_EACH_SOLDIER(i) {
    SOLDIERTYPE const &s = *i;
    if (!s.bInSector) continue;
    if (s.bLife <= 0) continue;
    if (s.bNeutral) continue;
    if (s.bSide == 0) continue;
    ++n_enemies;
  }
  return n_enemies;
}

static uint8_t NumEnemyInSectorExceptCreatures() {
  uint8_t n_enemies = 0;
  CFOR_EACH_SOLDIER(i) {
    SOLDIERTYPE const &s = *i;
    if (!s.bInSector) continue;
    if (s.bLife <= 0) continue;
    if (s.bNeutral) continue;
    if (s.bSide == 0) continue;
    if (s.bTeam == CREATURE_TEAM) continue;
    ++n_enemies;
  }
  return n_enemies;
}

static uint8_t NumEnemyInSectorNotDeadOrDying() {
  uint8_t n_enemies = 0;
  CFOR_EACH_SOLDIER(i) {
    SOLDIERTYPE const &s = *i;
    if (!s.bInSector) continue;
    if (s.uiStatusFlags & SOLDIER_DEAD) continue;
    /* Also, we want to pick up unconcious guys as NOT being capable, but we
     * want to make sure we don't get those ones that are in the process of
     * dying */
    if (s.bLife < OKLIFE) continue;
    if (s.bNeutral) continue;
    if (s.bSide == 0) continue;
    ++n_enemies;
  }
  return n_enemies;
}

static uint8_t NumBloodcatsInSectorNotDeadOrDying() {
  uint8_t n_enemies = 0;
  CFOR_EACH_SOLDIER(i) {
    SOLDIERTYPE const &s = *i;
    if (!s.bInSector) continue;
    if (s.ubBodyType != BLOODCAT) continue;
    if (s.uiStatusFlags & SOLDIER_DEAD) continue;
    /* Also, we want to pick up unconcious guys as NOT being capable, but we
     * want to make sure we don't get those ones that are in the process of
     * dying */
    if (s.bLife < OKLIFE) continue;
    if (s.bNeutral) continue;
    if (s.bSide == 0) continue;
    ++n_enemies;
  }
  return n_enemies;
}

uint8_t NumCapableEnemyInSector() {
  uint8_t n_enemies = 0;
  CFOR_EACH_SOLDIER(i) {
    SOLDIERTYPE const &s = *i;
    if (!s.bInSector) continue;
    if (s.uiStatusFlags & SOLDIER_DEAD) continue;
    /* Also, we want to pick up unconcious guys as NOT being capable, but we
     * want to make sure we don't get those ones that are in the process of
     * dying */
    if (s.bLife < OKLIFE && s.bLife != 0) continue;
    if (s.bNeutral) continue;
    if (s.bSide == 0) continue;
    ++n_enemies;
  }
  return n_enemies;
}

static void DeathTimerCallback();

static BOOLEAN CheckForLosingEndOfBattle() {
  int8_t bNumDead = 0, bNumNotOK = 0, bNumInBattle = 0, bNumNotOKRealMercs = 0;
  BOOLEAN fMadeCorpse;
  BOOLEAN fDoCapture = FALSE;
  BOOLEAN fOnlyEPCsLeft = TRUE;
  BOOLEAN fMilitiaInSector = FALSE;

  // ATE: Check for MILITIA - we won't lose if we have some.....
  CFOR_EACH_IN_TEAM(s, MILITIA_TEAM) {
    if (s->bInSector && s->bSide == OUR_TEAM && s->bLife >= OKLIFE) {
      // We have at least one poor guy who will still fight....
      // we have not lost ( yet )!
      fMilitiaInSector = TRUE;
    }
  }

  CFOR_EACH_IN_TEAM(pTeamSoldier, OUR_TEAM) {
    // Are we in sector.....
    if (pTeamSoldier->bInSector && !(pTeamSoldier->uiStatusFlags & SOLDIER_VEHICLE)) {
      bNumInBattle++;

      if (pTeamSoldier->bLife == 0) {
        bNumDead++;
      } else if (pTeamSoldier->bLife < OKLIFE) {
        bNumNotOK++;

        if (!AM_AN_EPC(pTeamSoldier) && !AM_A_ROBOT(pTeamSoldier)) {
          bNumNotOKRealMercs++;
        }
      } else {
        if (!AM_A_ROBOT(pTeamSoldier) || !AM_AN_EPC(pTeamSoldier)) {
          fOnlyEPCsLeft = FALSE;
        }
      }
    }
  }

  // OK< check ALL in battle, if that matches SUM of dead, incompacitated, we're
  // done!
  if ((bNumDead + bNumNotOK) == bNumInBattle || fOnlyEPCsLeft) {
    // Are there militia in sector?
    if (fMilitiaInSector) {
      if (guiCurrentScreen != AUTORESOLVE_SCREEN) {
        // if here, check if we should autoresolve.
        // if we have at least one guy unconscious, call below function...
        if (HandlePotentialBringUpAutoresolveToFinishBattle()) {
          // return false here as we are autoresolving...
          return (FALSE);
        }
      } else {
        return (FALSE);
      }
    }

    // Bring up box if we have ANY guy incompaciteded.....
    if (bNumDead != bNumInBattle) {
      // If we get captured...
      // Your unconscious mercs are captured!

      // Check if we should get captured....
      if (bNumNotOKRealMercs < 4 && bNumNotOKRealMercs > 1) {
        // Check if any enemies exist....
        if (IsTeamActive(ENEMY_TEAM)) {
          // if( GetWorldDay() > STARTDAY_ALLOW_PLAYER_CAPTURE_FOR_RESCUE && !(
          // gStrategicStatus.uiFlags & STRATEGIC_PLAYER_CAPTURED_FOR_RESCUE ))
          {
            if (gubQuest[QUEST_HELD_IN_ALMA] == QUESTNOTSTARTED ||
                (gubQuest[QUEST_HELD_IN_ALMA] == QUESTDONE &&
                 gubQuest[QUEST_INTERROGATION] == QUESTNOTSTARTED)) {
              fDoCapture = TRUE;
              // CJC Dec 1 2002: fix capture sequences
              BeginCaptureSquence();
            }
          }
        }
      }

      gfKillingGuysForLosingBattle = TRUE;

      // Kill them now...
      FOR_EACH_IN_TEAM(i, OUR_TEAM) {
        SOLDIERTYPE &s = *i;
        // Are we in sector.....
        if (s.bInSector) {
          if ((s.bLife != 0 && s.bLife < OKLIFE) || AM_AN_EPC(&s) || AM_A_ROBOT(&s)) {
            // Captured EPCs or ROBOTS will be kiiled in capture routine....
            if (!fDoCapture) {
              // Kill!
              s.bLife = 0;

              HandleSoldierDeath(&s, &fMadeCorpse);
            }
          }

          // ATE: if we are told to do capture....
          if (s.bLife != 0 && fDoCapture) {
            EnemyCapturesPlayerSoldier(&s);
            RemoveSoldierFromTacticalSector(s);
          }
        }
      }

      gfKillingGuysForLosingBattle = FALSE;

      if (fDoCapture) {
        EndCaptureSequence();
        SetCustomizableTimerCallbackAndDelay(3000, CaptureTimerCallback, FALSE);
      } else {
        SetCustomizableTimerCallbackAndDelay(10000, DeathTimerCallback, FALSE);
      }
    }
    return (TRUE);
  }

  return (FALSE);
}

static bool KillIncompacitatedEnemyInSector() {
  bool ret = false;
  FOR_EACH_SOLDIER(i) {
    SOLDIERTYPE &s = *i;
    if (!s.bInSector) continue;
    if (s.bLife >= OKLIFE) continue;
    if (s.uiStatusFlags & SOLDIER_DEAD) continue;
    if (s.bNeutral) continue;
    if (s.bSide == OUR_TEAM) continue;
    // Kill
    SoldierTakeDamage(&s, s.bLife, 100, TAKE_DAMAGE_BLOODLOSS, 0);
    ret = true;
  }
  return ret;
}

static int8_t CalcSuppressionTolerance(SOLDIERTYPE *pSoldier) {
  int8_t bTolerance;

  // Calculate basic tolerance value
  bTolerance = pSoldier->bExpLevel * 2;
  if (pSoldier->uiStatusFlags & SOLDIER_PC) {
    // give +1 for every 10% morale from 50, for a maximum bonus/penalty of 5.
    bTolerance += (pSoldier->bMorale - 50) / 10;
  } else {
    // give +2 for every morale category from normal, for a max change of 4
    bTolerance += (pSoldier->bAIMorale - MORALE_NORMAL) * 2;
  }

  if (pSoldier->ubProfile != NO_PROFILE) {
    // change tolerance based on attitude
    switch (gMercProfiles[pSoldier->ubProfile].bAttitude) {
      case ATT_AGGRESSIVE:
        bTolerance += 2;
        break;
      case ATT_COWARD:
        bTolerance += -2;
        break;
      default:
        break;
    }
  } else {
    // generic NPC/civvie; change tolerance based on attitude
    switch (pSoldier->bAttitude) {
      case BRAVESOLO:
      case BRAVEAID:
        bTolerance += 2;
        break;
      case AGGRESSIVE:
        bTolerance += 1;
        break;
      case DEFENSIVE:
        bTolerance += -1;
        break;
      default:
        break;
    }
  }

  if (bTolerance < 0) {
    bTolerance = 0;
  }

  return (bTolerance);
}

#define MAX_APS_SUPPRESSED 8

static void HandleSuppressionFire(const SOLDIERTYPE *const targeted_merc,
                                  SOLDIERTYPE *const caused_attacker) {
  int8_t bTolerance;
  int16_t sClosestOpponent, sClosestOppLoc;
  uint8_t ubPointsLost, ubTotalPointsLost, ubNewStance;
  uint8_t ubLoop2;

  FOR_EACH_MERC(i) {
    SOLDIERTYPE *const pSoldier = *i;
    if (IS_MERC_BODY_TYPE(pSoldier) && pSoldier->bLife >= OKLIFE &&
        pSoldier->ubSuppressionPoints > 0) {
      bTolerance = CalcSuppressionTolerance(pSoldier);

      // multiply by 2, add 1 and divide by 2 to round off to nearest whole
      // number
      ubPointsLost = (((pSoldier->ubSuppressionPoints * 6) / (bTolerance + 6)) * 2 + 1) / 2;

      // reduce loss of APs based on stance
      // ATE: Taken out because we can possibly supress ourselves...
      // switch (gAnimControl[ pSoldier->usAnimState ].ubEndHeight)
      //{
      //	case ANIM_PRONE:
      //		ubPointsLost = ubPointsLost * 2 / 4;
      //		break;
      //	case ANIM_CROUCH:
      //		ubPointsLost = ubPointsLost * 3 / 4;
      //		break;
      //	default:
      //		break;
      //}

      // cap the # of APs we can lose
      if (ubPointsLost > MAX_APS_SUPPRESSED) {
        ubPointsLost = MAX_APS_SUPPRESSED;
      }

      ubTotalPointsLost = ubPointsLost;

      // Subtract off the APs lost before this point to find out how many points
      // are lost now
      if (pSoldier->ubAPsLostToSuppression >= ubPointsLost) {
        continue;
      }

      // morale modifier
      if (ubTotalPointsLost / 2 > pSoldier->ubAPsLostToSuppression / 2) {
        for (ubLoop2 = 0;
             ubLoop2 < (ubTotalPointsLost / 2) - (pSoldier->ubAPsLostToSuppression / 2);
             ubLoop2++) {
          HandleMoraleEvent(pSoldier, MORALE_SUPPRESSED, pSoldier->sSectorX, pSoldier->sSectorY,
                            pSoldier->bSectorZ);
        }
      }

      ubPointsLost -= pSoldier->ubAPsLostToSuppression;
      ubNewStance = 0;

      // merc may get to react
      if (pSoldier->ubSuppressionPoints >= (130 / (6 + bTolerance))) {
        // merc gets to use APs to react!
        switch (gAnimControl[pSoldier->usAnimState].ubEndHeight) {
          case ANIM_PRONE:
            // can't change stance below prone!
            break;
          case ANIM_CROUCH:
            if (ubTotalPointsLost >= AP_PRONE && IsValidStance(pSoldier, ANIM_PRONE)) {
              sClosestOpponent = ClosestKnownOpponent(pSoldier, &sClosestOppLoc, NULL);
              if (sClosestOpponent == NOWHERE ||
                  SpacesAway(pSoldier->sGridNo, sClosestOppLoc) > 8) {
                if (ubPointsLost < AP_PRONE) {
                  // Have to give APs back so that we can change stance without
                  // losing more APs
                  pSoldier->bActionPoints += (AP_PRONE - ubPointsLost);
                  ubPointsLost = 0;
                } else {
                  ubPointsLost -= AP_PRONE;
                }
                ubNewStance = ANIM_PRONE;
              }
            }
            break;
          default:  // standing!
            if (pSoldier->bOverTerrainType == LOW_WATER ||
                pSoldier->bOverTerrainType == DEEP_WATER) {
              // can't change stance here!
              break;
            } else if (ubTotalPointsLost >= (AP_CROUCH + AP_PRONE) &&
                       (gAnimControl[pSoldier->usAnimState].ubEndHeight != ANIM_PRONE) &&
                       IsValidStance(pSoldier, ANIM_PRONE)) {
              sClosestOpponent = ClosestKnownOpponent(pSoldier, &sClosestOppLoc, NULL);
              if (sClosestOpponent == NOWHERE ||
                  SpacesAway(pSoldier->sGridNo, sClosestOppLoc) > 8) {
                if (gAnimControl[pSoldier->usAnimState].ubEndHeight == ANIM_STAND) {
                  // can only crouch for now
                  ubNewStance = ANIM_CROUCH;
                } else {
                  // lie prone!
                  ubNewStance = ANIM_PRONE;
                }
              } else if (gAnimControl[pSoldier->usAnimState].ubEndHeight == ANIM_STAND &&
                         IsValidStance(pSoldier, ANIM_CROUCH)) {
                // crouch, at least!
                ubNewStance = ANIM_CROUCH;
              }
            } else if (ubTotalPointsLost >= AP_CROUCH &&
                       (gAnimControl[pSoldier->usAnimState].ubEndHeight != ANIM_CROUCH) &&
                       IsValidStance(pSoldier, ANIM_CROUCH)) {
              // crouch!
              ubNewStance = ANIM_CROUCH;
            }
            break;
        }
      }

      // Reduce action points!
      pSoldier->bActionPoints -= ubPointsLost;
      pSoldier->ubAPsLostToSuppression = ubTotalPointsLost;

      if (pSoldier->uiStatusFlags & SOLDIER_PC && pSoldier->ubSuppressionPoints > 8 &&
          pSoldier == targeted_merc) {
        if (!(pSoldier->usQuoteSaidFlags & SOLDIER_QUOTE_SAID_BEING_PUMMELED)) {
          pSoldier->usQuoteSaidFlags |= SOLDIER_QUOTE_SAID_BEING_PUMMELED;
          // say we're under heavy fire!

          // ATE: For some reason, we forgot #53!
          if (pSoldier->ubProfile != 53) {
            TacticalCharacterDialogue(pSoldier, QUOTE_UNDER_HEAVY_FIRE);
          }
        }
      }

      if (ubNewStance != 0) {
        // This person is going to change stance

        // This person will be busy while they crouch or go prone
        if ((gTacticalStatus.uiFlags & TURNBASED) && (gTacticalStatus.uiFlags & INCOMBAT)) {
          DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
                   String("!!!!!!! Starting suppression, on %d", pSoldier->ubID));

          gTacticalStatus.ubAttackBusyCount++;

          // make sure supressor ID is the same!
          pSoldier->suppressor = caused_attacker;
        }
        pSoldier->fChangingStanceDueToSuppression = TRUE;
        pSoldier->fDontChargeAPsForStanceChange = TRUE;

        // AI people will have to have their actions cancelled
        if (!(pSoldier->uiStatusFlags & SOLDIER_PC)) {
          CancelAIAction(pSoldier);
          pSoldier->bAction = AI_ACTION_CHANGE_STANCE;
          pSoldier->usActionData = ubNewStance;
          pSoldier->bActionInProgress = TRUE;
        }

        // go for it!
        // ATE: Cancel any PENDING ANIMATIONS...
        pSoldier->usPendingAnimation = NO_PENDING_ANIMATION;
        // ATE: Turn off non-interrupt flag ( this NEEDS to be done! )
        pSoldier->fInNonintAnim = FALSE;
        pSoldier->fRTInNonintAnim = FALSE;

        ChangeSoldierStance(pSoldier, ubNewStance);
      }

    }  // end of examining one soldier
  }  // end of loop
}

BOOLEAN ProcessImplicationsOfPCAttack(SOLDIERTYPE *const pSoldier, SOLDIERTYPE *const pTarget,
                                      const int8_t bReason) {
  BOOLEAN fEnterCombat = TRUE;

  if (pTarget->fAIFlags & AI_ASLEEP) {
    // waaaaaaaaaaaaake up!
    pTarget->fAIFlags &= (~AI_ASLEEP);
  }

  if (pTarget->ubProfile == PABLO && CheckFact(FACT_PLAYER_FOUND_ITEMS_MISSING, 0)) {
    SetFactTrue(FACT_PABLO_PUNISHED_BY_PLAYER);
  }

  if (gTacticalStatus.bBoxingState == BOXING) {
    // should have a check for "in boxing ring", no?
    if ((pSoldier->usAttackingWeapon != NOTHING && pSoldier->usAttackingWeapon != BRASS_KNUCKLES) ||
        !(pSoldier->uiStatusFlags & SOLDIER_BOXER)) {
      // someone's cheating!
      if ((Item[pSoldier->usAttackingWeapon].usItemClass == IC_BLADE ||
           Item[pSoldier->usAttackingWeapon].usItemClass == IC_PUNCH) &&
          (pTarget->uiStatusFlags & SOLDIER_BOXER)) {
        // knife or brass knuckles disqualify the player!
        BoxingPlayerDisqualified(pSoldier, BAD_ATTACK);
      } else {
        // anything else is open war!
        // gTacticalStatus.bBoxingState = NOT_BOXING;
        SetBoxingState(NOT_BOXING);
        // if we are attacking a boxer we should set them to neutral
        // (temporarily) so that the rest of the civgroup code works...
        if ((pTarget->bTeam == CIV_TEAM) && (pTarget->uiStatusFlags & SOLDIER_BOXER)) {
          SetSoldierNeutral(pTarget);
        }
      }
    }
  }

  if ((pTarget->bTeam == MILITIA_TEAM) && (pTarget->bSide == OUR_TEAM)) {
    // rebel militia attacked by the player!
    MilitiaChangesSides();
  }
  // JA2 Gold: fix Slay
  else if (pTarget->bTeam == CIV_TEAM && pTarget->bNeutral && pTarget->ubProfile == SLAY &&
           pTarget->bLife >= OKLIFE && !CheckFact(FACT_155, 0)) {
    TriggerNPCRecord(SLAY, 1);
  } else if ((pTarget->bTeam == CIV_TEAM) && (pTarget->ubCivilianGroup == 0) &&
             (pTarget->bNeutral) && !(pTarget->uiStatusFlags & SOLDIER_VEHICLE)) {
    if (pTarget->ubBodyType == COW && gWorldSectorX == 10 && gWorldSectorY == MAP_ROW_F) {
      // hicks could get mad!!!
      HickCowAttacked(pSoldier, pTarget);
    } else if (pTarget->ubProfile == PABLO && pTarget->bLife >= OKLIFE &&
               CheckFact(FACT_PABLO_PUNISHED_BY_PLAYER, 0) && !CheckFact(FACT_38, 0)) {
      TriggerNPCRecord(PABLO, 3);
    } else {
      // regular civ attacked, turn non-neutral
      AddToShouldBecomeHostileOrSayQuoteList(pTarget);

      if (pTarget->ubProfile == NO_PROFILE || !(gMercProfiles[pTarget->ubProfile].ubMiscFlags3 &
                                                PROFILE_MISC_FLAG3_TOWN_DOESNT_CARE_ABOUT_DEATH)) {
        // militia, if any, turn hostile
        MilitiaChangesSides();
      }
    }
  } else {
    if (pTarget->ubProfile == CARMEN)  // Carmen
    {
      // Special stuff for Carmen the bounty hunter
      if (pSoldier->ubProfile != SLAY)  // attacked by someone other than Slay
      {
        // change attitude
        pTarget->bAttitude = AGGRESSIVE;
      }
    }

    if (pTarget->ubCivilianGroup && ((pTarget->bTeam == OUR_TEAM) || pTarget->bNeutral)) {
      // member of a civ group, either recruited or neutral, so should
      // change sides individually or all together

      CivilianGroupMemberChangesSides(pTarget);

      if (pTarget->ubProfile != NO_PROFILE && pTarget->bLife >= OKLIFE &&
          pTarget->bVisible == TRUE) {
        // trigger quote!
        PauseAITemporarily();
        AddToShouldBecomeHostileOrSayQuoteList(pTarget);
        // TriggerNPCWithIHateYouQuote( pTarget->ubProfile );
      }
    } else if (pTarget->ubCivilianGroup != NON_CIV_GROUP &&
               !(pTarget->uiStatusFlags & SOLDIER_BOXER)) {
      // Firing at a civ in a civ group who isn't hostile... if anyone in that
      // civ group can see this going on they should become hostile.
      CivilianGroupMembersChangeSidesWithinProximity(pTarget);
    } else if (pTarget->bTeam == OUR_TEAM && !(gTacticalStatus.uiFlags & INCOMBAT)) {
      // firing at one of our own guys who is not a rebel etc
      if (pTarget->bLife >= OKLIFE && !(pTarget->bCollapsed) && !AM_A_ROBOT(pTarget) &&
          (bReason == REASON_NORMAL_ATTACK)) {
        // OK, sturn towards the prick
        // Change to fire ready animation

        pTarget->fDontChargeReadyAPs = TRUE;
        SoldierReadyWeapon(pTarget, pSoldier->sGridNo, FALSE);

        // ATE: Depending on personality, fire back.....

        // Do we have a gun in a\hand?
        if (Item[pTarget->inv[HANDPOS].usItem].usItemClass == IC_GUN) {
          // Toggle burst capable...
          if (!pTarget->bDoBurst) {
            if (IsGunBurstCapable(pTarget, HANDPOS)) {
              ChangeWeaponMode(pTarget);
            }
          }

          // Fire back!
          HandleItem(pTarget, pSoldier->sGridNo, pSoldier->bLevel, pTarget->inv[HANDPOS].usItem,
                     FALSE);
        }
      }

      // don't enter combat on attack on one of our own mercs
      fEnterCombat = FALSE;
    }

    // if we've attacked a miner
    if (IsProfileAHeadMiner(pTarget->ubProfile)) {
      PlayerAttackedHeadMiner(pTarget->ubProfile);
    }
  }

  return (fEnterCombat);
}

static SOLDIERTYPE *InternalReduceAttackBusyCount(SOLDIERTYPE *const pSoldier,
                                                  const BOOLEAN fCalledByAttacker,
                                                  SOLDIERTYPE *pTarget) {
  // Strange as this may seem, this function returns a pointer to
  // the *target* in case the target has changed sides as a result
  // of being attacked
  BOOLEAN fEnterCombat = FALSE;

  if (pSoldier == NULL) {
    pTarget = NULL;
  } else {
    if (pTarget == NULL) {
      DebugMsg(TOPIC_JA2, DBG_LEVEL_3, ">>Target ptr is null!");
    }
  }

  if (fCalledByAttacker) {
    if (pSoldier && Item[pSoldier->inv[HANDPOS].usItem].usItemClass & IC_GUN) {
      if (pSoldier->bBulletsLeft > 0) {
        return (pTarget);
      }
    }
  }

  //	if ((gTacticalStatus.uiFlags & TURNBASED) && (gTacticalStatus.uiFlags &
  // INCOMBAT))
  //	{

  if (gTacticalStatus.ubAttackBusyCount == 0) {
    // ATE: We have a problem here... if testversion, report error......
    // But for all means.... DON'T wrap!
    if ((gTacticalStatus.uiFlags & INCOMBAT)) {
      DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
               "!!!!!!! &&&&&&& Problem with attacker busy count decrementing "
               "past 0.... preventing wrap-around.");
    }
  } else {
    gTacticalStatus.ubAttackBusyCount--;
  }

  DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
           String("!!!!!!! Ending attack, attack count now %d", gTacticalStatus.ubAttackBusyCount));
  //	}

  if (gTacticalStatus.ubAttackBusyCount > 0) {
    return (pTarget);
  }

  if ((gTacticalStatus.uiFlags & TURNBASED) && (gTacticalStatus.uiFlags & INCOMBAT)) {
    // Check to see if anyone was suppressed
    const SOLDIERTYPE *const target = (pSoldier == NULL ? NULL : pSoldier->target);
    HandleSuppressionFire(target, pSoldier);

    // HandleAfterShootingGuy( pSoldier, pTarget );

    // suppression fire might cause the count to be increased, so check it again
    if (gTacticalStatus.ubAttackBusyCount > 0) {
      DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
               String("!!!!!!! Starting suppression, attack count now %d",
                      gTacticalStatus.ubAttackBusyCount));
      return (pTarget);
    }
  }

  // ATE: IN MEANWHILES, we have 'combat' in realtime....
  // this is so we DON'T call freeupattacker() which will cancel
  // the AI guy's meanwhile NPC stuff.
  // OK< let's NOT do this if it was the queen attacking....
  if (AreInMeanwhile() && pSoldier != NULL && pSoldier->ubProfile != QUEEN) {
    return (pTarget);
  }

  if (pTarget) {
    // reset # of shotgun pellets hit by
    pTarget->bNumPelletsHitBy = 0;
    // reset flag for making "ow" sound on being shot
  }

  if (pSoldier) {
    // reset attacking hand
    pSoldier->ubAttackingHand = HANDPOS;

    // if there is a valid target, and our attack was noticed
    if (pTarget && (pSoldier->uiStatusFlags & SOLDIER_ATTACK_NOTICED)) {
      // stuff that only applies to when we attack
      if (pTarget->ubBodyType != CROW) {
        if (pSoldier->bTeam == OUR_TEAM) {
          fEnterCombat = ProcessImplicationsOfPCAttack(pSoldier, pTarget, REASON_NORMAL_ATTACK);
          if (!fEnterCombat) {
            DebugMsg(TOPIC_JA2, DBG_LEVEL_3, ">>Not entering combat as a result of PC attack");
          }
        }
      }

      // global

      // ATE: If we are an animal, etc, don't change to hostile...
      // ( and don't go into combat )
      if (pTarget->ubBodyType == CROW) {
        // Loop through our team, make guys who can see this fly away....
        const uint8_t ubTeam = pTarget->bTeam;
        FOR_EACH_IN_TEAM(s, ubTeam) {
          if (s->bInSector && s->ubBodyType == CROW &&
              s->bOppList[pSoldier->ubID] == SEEN_CURRENTLY) {
            // ZEROTIMECOUNTER(s->AICounter);
            // MakeCivHostile(s, 2);
            HandleCrowFlyAway(s);
          }
        }

        // Don't enter combat...
        fEnterCombat = FALSE;
      }

      if (gTacticalStatus.bBoxingState == BOXING) {
        if (pTarget && pTarget->bLife <= 0) {
          // someone has won!
          EndBoxingMatch(pTarget);
        }
      }

      // if soldier and target were not both players and target was not under
      // fire before...
      if ((pSoldier->bTeam != OUR_TEAM || pTarget->bTeam != OUR_TEAM)) {
        if (pTarget->bOppList[pSoldier->ubID] != SEEN_CURRENTLY) {
          NoticeUnseenAttacker(pSoldier, pTarget, 0);
        }
        // "under fire" lasts for 2 turns
        pTarget->bUnderFire = 2;
      }

    } else if (pTarget) {
      // something is wrong here!
      if (!pTarget->bActive || !pTarget->bInSector) {
        DebugMsg(TOPIC_JA2, DBG_LEVEL_3, ">>Invalid target attacked!");
      } else if (!(pSoldier->uiStatusFlags & SOLDIER_ATTACK_NOTICED)) {
        DebugMsg(TOPIC_JA2, DBG_LEVEL_3, ">>Attack not noticed");
      }
    } else {
      // no target, don't enter combat
      fEnterCombat = FALSE;
    }

    if (pSoldier->fSayAmmoQuotePending) {
      pSoldier->fSayAmmoQuotePending = FALSE;
      TacticalCharacterDialogue(pSoldier, QUOTE_OUT_OF_AMMO);
    }

    if (pSoldier->uiStatusFlags & SOLDIER_PC) {
      UnSetUIBusy(pSoldier);
    } else {
      FreeUpNPCFromAttacking(pSoldier);
    }

    if (!fEnterCombat) {
      DebugMsg(TOPIC_JA2, DBG_LEVEL_3, ">>Not to enter combat from this attack");
    }

    if (fEnterCombat && !(gTacticalStatus.uiFlags & INCOMBAT)) {
      // Go into combat!

      // If we are in a meanwhile... don't enter combat here...
      if (!AreInMeanwhile()) {
        EnterCombatMode(pSoldier->bTeam);
      }
    }

    pSoldier->uiStatusFlags &= (~SOLDIER_ATTACK_NOTICED);
  }

  TacticalStatusType *const ts = &gTacticalStatus;
  if (ts->fKilledEnemyOnAttack) {
    // Check for death quote...
    HandleKilledQuote(ts->enemy_killed_on_attack, ts->enemy_killed_on_attack_killer,
                      ts->ubEnemyKilledOnAttackLocation, ts->bEnemyKilledOnAttackLevel);
    ts->fKilledEnemyOnAttack = FALSE;
  }

  // ATE: Check for stat changes....
  HandleAnyStatChangesAfterAttack();

  if (gTacticalStatus.fItemsSeenOnAttack && gTacticalStatus.ubCurrentTeam == OUR_TEAM) {
    gTacticalStatus.fItemsSeenOnAttack = FALSE;

    // Display quote!
    SOLDIERTYPE *const s = gTacticalStatus.items_seen_on_attack_soldier;
    if (!AM_AN_EPC(s)) {
      MakeCharacterDialogueEventSignalItemLocatorStart(*s,
                                                       gTacticalStatus.usItemsSeenOnAttackGridNo);
    } else {
      // Turn off item lock for locators...
      gTacticalStatus.fLockItemLocators = FALSE;
      // Slide to location!
      SlideToLocation(gTacticalStatus.usItemsSeenOnAttackGridNo);
    }
  }

  if (gTacticalStatus.uiFlags & CHECK_SIGHT_AT_END_OF_ATTACK) {
    AllTeamsLookForAll(FALSE);

    // call fov code
    FOR_EACH_IN_TEAM(pSightSoldier, OUR_TEAM) {
      if (pSightSoldier->bInSector) {
        RevealRoofsAndItems(pSightSoldier, FALSE);
      }
    }
    gTacticalStatus.uiFlags &= ~CHECK_SIGHT_AT_END_OF_ATTACK;
  }

  DequeueAllDemandGameEvents();

  CheckForEndOfBattle(FALSE);

  // if we're in realtime, turn off the attacker's muzzle flash at this point
  if (!(gTacticalStatus.uiFlags & INCOMBAT) && pSoldier) {
    EndMuzzleFlash(pSoldier);
  }

  if (pSoldier && pSoldier->bWeaponMode == WM_ATTACHED) {
    // change back to single shot
    pSoldier->bWeaponMode = WM_NORMAL;
  }

  // record last target
  // Check for valid target!
  if (pSoldier) {
    pSoldier->sLastTarget = pSoldier->sTargetGridNo;
  }

  return (pTarget);
}

SOLDIERTYPE *ReduceAttackBusyCount(SOLDIERTYPE *const attacker, const BOOLEAN fCalledByAttacker) {
  SOLDIERTYPE *const target = (attacker == NULL ? NULL : attacker->target);
  return InternalReduceAttackBusyCount(attacker, fCalledByAttacker, target);
}

SOLDIERTYPE *FreeUpAttacker(SOLDIERTYPE *const attacker) {
  // Strange as this may seem, this function returns a pointer to
  // the *target* in case the target has changed sides as a result
  // of being attacked
  return ReduceAttackBusyCount(attacker, TRUE);
}

SOLDIERTYPE *FreeUpAttackerGivenTarget(SOLDIERTYPE *const target) {
  // Strange as this may seem, this function returns a pointer to
  // the *target* in case the target has changed sides as a result
  // of being attacked
  return InternalReduceAttackBusyCount(target->attacker, TRUE, target);
}

SOLDIERTYPE *ReduceAttackBusyGivenTarget(SOLDIERTYPE *const target) {
  // Strange as this may seem, this function returns a pointer to
  // the *target* in case the target has changed sides as a result
  // of being attacked
  return InternalReduceAttackBusyCount(target->attacker, FALSE, target);
}

void ResetAllMercSpeeds() {
  FOR_EACH_SOLDIER(s) {
    if (s->bInSector) SetSoldierAniSpeed(s);
  }
}

void SetActionToDoOnceMercsGetToLocation(uint8_t ubActionCode, int8_t bNumMercsWaiting) {
  gubWaitingForAllMercsToExitCode = ubActionCode;
  gbNumMercsUntilWaitingOver = bNumMercsWaiting;

  // Setup timer counter and report back if too long....
  guiWaitingForAllMercsToExitTimer = GetJA2Clock();

  // ATE: Set flag to ignore sight...
  gTacticalStatus.uiFlags |= DISALLOW_SIGHT;
}

static void HandleBloodForNewGridNo(const SOLDIERTYPE *pSoldier) {
  // Handle bleeding...
  if ((pSoldier->bBleeding > MIN_BLEEDING_THRESHOLD)) {
    int8_t bBlood = (pSoldier->bBleeding - MIN_BLEEDING_THRESHOLD) / BLOODDIVISOR;
    if (bBlood > MAXBLOODQUANTITY) bBlood = MAXBLOODQUANTITY;

    // now, he shouldn't ALWAYS bleed the same amount; LOWER it perhaps. If it
    // goes less than zero, then no blood!
    bBlood -= (int8_t)Random(7);

    if (bBlood >= 0) {
      // this handles all soldiers' dropping blood during movement
      DropBlood(*pSoldier, bBlood);
    }
  }
}

void CencelAllActionsForTimeCompression() {
  FOR_EACH_SOLDIER(s) {
    if (!s->bInSector) continue;

    // Hault!
    EVENT_StopMerc(s);

    // END AI actions
    CancelAIAction(s);
  }
}

void AddManToTeam(int8_t bTeam) {
  // ATE: If not loading game!
  if (!(gTacticalStatus.uiFlags & LOADING_SAVED_GAME)) {
    // Increment men in sector number!
    gTacticalStatus.Team[bTeam].bMenInSector++;
  }
}

void RemoveManFromTeam(const int8_t bTeam) {
  // ATE; if not loading game!
  if (gTacticalStatus.uiFlags & LOADING_SAVED_GAME) return;
  if (!IsTeamActive(bTeam)) {
    return;
  }
  --gTacticalStatus.Team[bTeam].bMenInSector;
}

void RemoveSoldierFromTacticalSector(SOLDIERTYPE &s) {
  // reset merc's opplist
  InitSoldierOppList(s);

  // Remove!
  RemoveSoldierFromGridNo(s);

  RemoveMercSlot(&s);

  s.bInSector = FALSE;

  // Select next avialiable guy....
  if (guiCurrentScreen == GAME_SCREEN) {
    if (GetSelectedMan() == &s) {
      SOLDIERTYPE *const next = FindNextActiveAndAliveMerc(&s, FALSE, FALSE);
      if (next != &s) {
        SelectSoldier(next, SELSOLDIER_NONE);
      } else {
        // OK - let's look for another squad...
        SOLDIERTYPE *const pNewSoldier = FindNextActiveSquad(&s);
        if (pNewSoldier != &s) {
          // Good squad found!
          SelectSoldier(pNewSoldier, SELSOLDIER_NONE);
        } else {
          // if here, make nobody
          SetSelectedMan(NULL);
        }
      }
    }
    UpdateTeamPanelAssignments();
  } else {
    SetSelectedMan(NULL);
  }
}

static void DoneFadeOutDueToDeath() {
  // Quit game....
  InternalLeaveTacticalScreen(MAINMENU_SCREEN);
  // SetPendingNewScreen( MAINMENU_SCREEN );
}

static void EndBattleWithUnconsciousGuysCallback(MessageBoxReturnValue const bExitValue) {
  // Enter mapscreen.....
  CheckAndHandleUnloadingOfCurrentWorld();
}

void InitializeTacticalStatusAtBattleStart() {
  gTacticalStatus.ubArmyGuysKilled = 0;

  gTacticalStatus.fPanicFlags = 0;
  gTacticalStatus.fEnemyFlags = 0;
  for (int8_t i = 0; i < NUM_PANIC_TRIGGERS; ++i) {
    gTacticalStatus.sPanicTriggerGridNo[i] = NOWHERE;
    gTacticalStatus.bPanicTriggerIsAlarm[i] = FALSE;
    gTacticalStatus.ubPanicTolerance[i] = 0;
  }

  for (int32_t i = 0; i < MAXTEAMS; ++i) {
    gTacticalStatus.Team[i].last_merc_to_radio = NULL;
    gTacticalStatus.Team[i].bAwareOfOpposition = FALSE;
  }

  gTacticalStatus.the_chosen_one = NULL;

  ClearIntList();

  // make sure none of our guys have leftover shock values etc
  FOR_EACH_IN_TEAM(s, OUR_TEAM) {
    s->bShock = 0;
    s->bTilesMoved = 0;
  }

  // loop through everyone; clear misc flags
  FOR_EACH_SOLDIER(s) { s->ubMiscSoldierFlags = 0; }
}

static void DeathTimerCallback() {
  const wchar_t *text;
  if (gTacticalStatus.Team[CREATURE_TEAM].bMenInSector >
      gTacticalStatus.Team[ENEMY_TEAM].bMenInSector) {
    text = LargeTacticalStr[LARGESTR_NOONE_LEFT_CAPABLE_OF_BATTLE_AGAINST_CREATURES_STR];
  } else {
    text = LargeTacticalStr[LARGESTR_NOONE_LEFT_CAPABLE_OF_BATTLE_STR];
  }
  DoMessageBox(MSG_BOX_BASIC_STYLE, text, GAME_SCREEN, MSG_BOX_FLAG_OK,
               EndBattleWithUnconsciousGuysCallback, NULL);
}

void CaptureTimerCallback() {
  const wchar_t *text;
  if (gfSurrendered) {
    text = LargeTacticalStr[3];
  } else {
    text = LargeTacticalStr[LARGESTR_HAVE_BEEN_CAPTURED];
  }
  DoMessageBox(MSG_BOX_BASIC_STYLE, text, GAME_SCREEN, MSG_BOX_FLAG_OK,
               EndBattleWithUnconsciousGuysCallback, NULL);
  gfSurrendered = FALSE;
}

void DoPOWPathChecks() {
  /* loop through all mercs on our team and if they are POWs in sector, do POW
   * path check and put on a squad if available */
  FOR_EACH_IN_TEAM(s, OUR_TEAM) {
    if (!s->bInSector || s->bAssignment != ASSIGNMENT_POW) continue;

    // check to see if POW has been freed!
    // this will be true if a path can be made from the POW to either of 3
    // gridnos 10492 (hallway) or 10482 (outside), or 9381 (outside)
    if (!FindBestPath(s, 10492, 0, WALKING, NO_COPYROUTE, PATH_THROUGH_PEOPLE) &&
        !FindBestPath(s, 10482, 0, WALKING, NO_COPYROUTE, PATH_THROUGH_PEOPLE) &&
        !FindBestPath(s, 9381, 0, WALKING, NO_COPYROUTE, PATH_THROUGH_PEOPLE)) {
      continue;
    }
    // free! free!
    // put them on any available squad
    s->bNeutral = FALSE;
    AddCharacterToAnySquad(s);
    DoMercBattleSound(s, BATTLE_SOUND_COOL1);
  }
}

BOOLEAN HostileCiviliansPresent() {
  if (!IsTeamActive(CIV_TEAM)) return FALSE;

  CFOR_EACH_IN_TEAM(s, CIV_TEAM) {
    if (s->bInSector && s->bLife > 0 && !s->bNeutral) {
      return TRUE;
    }
  }

  return FALSE;
}

BOOLEAN HostileBloodcatsPresent() {
  if (!IsTeamActive(CREATURE_TEAM)) return FALSE;

  CFOR_EACH_IN_TEAM(s, CREATURE_TEAM) {
    /* KM : Aug 11, 1999 -- Patch fix:  Removed the check for bNeutral.
     * Bloodcats automatically become hostile on sight.  Because the check used
     * to be there, it was possible to get into a 2nd battle elsewhere which is
     * BAD BAD BAD! */
    if (s->bInSector && s->bLife > 0 && s->ubBodyType == BLOODCAT) {
      return TRUE;
    }
  }

  return FALSE;
}

static void DoCreatureTensionQuote(SOLDIERTYPE *pSoldier);

static void HandleCreatureTenseQuote() {
  // Check for quote seeing creature attacks....
  if (gubQuest[QUEST_CREATURES] == QUESTDONE) return;
  if (!(gTacticalStatus.uiFlags & IN_CREATURE_LAIR)) return;
  if (gTacticalStatus.uiFlags & INCOMBAT) return;

  int32_t uiTime = GetJA2Clock();
  if (uiTime - gTacticalStatus.uiCreatureTenseQuoteLastUpdate >
      (uint32_t)(gTacticalStatus.sCreatureTenseQuoteDelay * 1000)) {
    gTacticalStatus.uiCreatureTenseQuoteLastUpdate = uiTime;

    // run through list
    uint8_t ubNumMercs = 0;
    SOLDIERTYPE *mercs_in_sector[20];
    FOR_EACH_IN_TEAM(s, OUR_TEAM) {
      // Add guy if he's a candidate...
      if (OkControllableMerc(s) && !AM_AN_EPC(s) && !(s->uiStatusFlags & SOLDIER_GASSED) &&
          !AM_A_ROBOT(s) && !s->fMercAsleep) {
        mercs_in_sector[ubNumMercs++] = s;
      }
    }

    if (ubNumMercs > 0) {
      SOLDIERTYPE *const chosen = mercs_in_sector[Random(ubNumMercs)];
      DoCreatureTensionQuote(chosen);
    }

    // Adjust delay....
    gTacticalStatus.sCreatureTenseQuoteDelay = 60 + Random(60);
  }
}

static void DoCreatureTensionQuote(SOLDIERTYPE *s) {
  // Check for playing smell quote....
  int32_t quote;
  uint16_t quote_flag;
  const int32_t iRandomQuote = Random(3);
  switch (iRandomQuote) {
    case 0:
      quote = QUOTE_SMELLED_CREATURE;
      quote_flag = SOLDIER_QUOTE_SAID_SMELLED_CREATURE;
      break;

    case 1:
      quote = QUOTE_TRACES_OF_CREATURE_ATTACK;
      quote_flag = SOLDIER_QUOTE_SAID_SPOTTING_CREATURE_ATTACK;
      break;

    case 2:
      quote = QUOTE_WORRIED_ABOUT_CREATURE_PRESENCE;
      quote_flag = SOLDIER_QUOTE_SAID_WORRIED_ABOUT_CREATURES;
      break;

    default:
      abort();  // HACK000E
  }
  if (s->usQuoteSaidFlags & quote_flag) return;
  s->usQuoteSaidFlags |= quote_flag;
  TacticalCharacterDialogue(s, quote);
}

void MakeCharacterDialogueEventSignalItemLocatorStart(SOLDIERTYPE &s, GridNo const location) {
  class CharacterDialogueEventSignalItemLocatorStart : public CharacterDialogueEvent {
   public:
    CharacterDialogueEventSignalItemLocatorStart(SOLDIERTYPE &s, GridNo const location)
        : CharacterDialogueEvent(s), location_(location) {}

    bool Execute() {
      if (!MayExecute()) return true;

      // Turn off item lock for locators
      gTacticalStatus.fLockItemLocators = FALSE;

      SlideToLocation(location_);

      SOLDIERTYPE &s = soldier_;
      ExecuteCharacterDialogue(s.ubProfile, QUOTE_SPOTTED_SOMETHING_ONE + Random(2), s.face,
                               DIALOGUE_TACTICAL_UI, TRUE);

      return false;
    }

   private:
    GridNo const location_;
  };

  DialogueEvent::Add(new CharacterDialogueEventSignalItemLocatorStart(s, location));
}

#undef FAIL
#include "gtest/gtest.h"

TEST(Overhead, asserts) { EXPECT_EQ(lengthof(g_default_team_info), MAXTEAMS); }
