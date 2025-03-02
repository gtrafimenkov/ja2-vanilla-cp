// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Tactical/TeamTurns.h"

#include <string.h>

#include "Macro.h"
#include "SGP/Debug.h"
#include "SGP/FileMan.h"
#include "SGP/LoadSaveData.h"
#include "SGP/Types.h"
#include "Strategic/GameClock.h"
#include "Strategic/Meanwhile.h"
#include "Strategic/QueenCommand.h"
#include "Strategic/StrategicTurns.h"
#include "Tactical/AirRaid.h"
#include "Tactical/AnimationControl.h"
#include "Tactical/DialogueControl.h"
#include "Tactical/Interface.h"
#include "Tactical/Items.h"
#include "Tactical/OppList.h"
#include "Tactical/Overhead.h"
#include "Tactical/PathAI.h"
#include "Tactical/Points.h"
#include "Tactical/RottingCorpses.h"
#include "Tactical/SkillCheck.h"
#include "Tactical/SoldierControl.h"
#include "Tactical/SoldierFunctions.h"
#include "Tactical/SoldierMacros.h"
#include "Tactical/SoldierProfileType.h"
#include "Tactical/Squads.h"
#include "TacticalAI/AI.h"
#include "TacticalAI/AIInternals.h"
#include "TacticalAI/AIList.h"
#include "TacticalAI/NPC.h"
#include "TileEngine/Environment.h"
#include "TileEngine/ExplosionControl.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/LightEffects.h"
#include "TileEngine/Lighting.h"
#include "TileEngine/RenderWorld.h"
#include "TileEngine/Smell.h"
#include "TileEngine/SmokeEffects.h"
#include "Utils/FontControl.h"
#include "Utils/Message.h"
#include "Utils/SoundControl.h"
#include "Utils/Text.h"
#include "Utils/TimerControl.h"

static SOLDIERTYPE *gOutOfTurnOrder[MAXMERCS];
uint8_t gubOutOfTurnPersons = 0;

static inline SOLDIERTYPE *LatestInterruptGuy() { return gOutOfTurnOrder[gubOutOfTurnPersons]; }

#define REMOVE_LATEST_INTERRUPT_GUY() (DeleteFromIntList((uint8_t)(gubOutOfTurnPersons), TRUE))
#define INTERRUPTS_OVER (gubOutOfTurnPersons == 1)

BOOLEAN gfHiddenInterrupt = FALSE;
static SOLDIERTYPE *gLastInterruptedGuy = NULL;

extern SightFlags gubSightFlags;

#define MIN_APS_TO_INTERRUPT 4

void ClearIntList() {
  memset(gOutOfTurnOrder, 0, sizeof(gOutOfTurnOrder));
  gubOutOfTurnPersons = 0;
}

void StartPlayerTeamTurn(BOOLEAN fDoBattleSnd, BOOLEAN fEnteringCombatMode) {
  // Start the turn of player charactors

  //
  // PATCH 1.06:
  //
  // make sure set properly in gTacticalStatus:
  gTacticalStatus.ubCurrentTeam = OUR_TEAM;

  InitPlayerUIBar(FALSE);

  if (gTacticalStatus.uiFlags & TURNBASED) {
    // Are we in combat already?
    if (gTacticalStatus.uiFlags & INCOMBAT) {
      PlayJA2Sample(ENDTURN_1, MIDVOLUME, 1, MIDDLEPAN);
    }

    // Remove deadlock message
    EndDeadlockMsg();

    // Check for victory conditions

    // Are we in combat already?
    if (gTacticalStatus.uiFlags & INCOMBAT) {
      SOLDIERTYPE *sel = GetSelectedMan();
      if (sel != NULL) {
        // Check if this guy is able to be selected....
        if (sel->bLife < OKLIFE) {
          SelectNextAvailSoldier(sel);
          sel = GetSelectedMan();
        }

        // Slide to selected guy...
        if (sel != NULL) {
          SlideTo(sel, SETLOCATOR);

          // Say ATTENTION SOUND...
          if (fDoBattleSnd) DoMercBattleSound(sel, BATTLE_SOUND_ATTN1);

          if (gsInterfaceLevel == 1) {
            gTacticalStatus.uiFlags |= SHOW_ALL_ROOFS;
            InvalidateWorldRedundency();
            SetRenderFlags(RENDER_FLAG_FULL);
            ErasePath();
          }
        }
      }
    }

    // Dirty panel interface!
    fInterfacePanelDirty = DIRTYLEVEL2;

    // Adjust time now!
    UpdateClock();

    if (!fEnteringCombatMode) {
      CheckForEndOfCombatMode(TRUE);
    }
  }
  // Signal UI done enemy's turn
  guiPendingOverrideEvent = LU_ENDUILOCK;

  // ATE: Reset killed on attack variable.. this is because sometimes timing is
  // such
  /// that a baddie can die and still maintain it's attacker ID
  gTacticalStatus.fKilledEnemyOnAttack = FALSE;

  HandleTacticalUI();
}

static void FreezeInterfaceForEnemyTurn() {
  // Reset flags
  gfPlotNewMovement = TRUE;

  // Erase path
  ErasePath();

  // Setup locked UI
  guiPendingOverrideEvent = LU_BEGINUILOCK;

  // Remove any UI messages!
  if (g_ui_message_overlay != NULL) {
    EndUIMessage();
  }
}

static void EndInterrupt(BOOLEAN fMarkInterruptOccurred);

void EndTurn(uint8_t ubNextTeam) {
  // Check for enemy pooling (add enemies if there happens to be more than the
  // max in the current battle.  If one or more slots have freed up, we can add
  // them now.

  EndDeadlockMsg();

  /*
          if ( CheckForEndOfCombatMode( FALSE ) )
          {
                  return;
          }
          */

  if (INTERRUPT_QUEUED) {
    EndInterrupt(FALSE);
  } else {
    AddPossiblePendingEnemiesToBattle();

    //		InitEnemyUIBar( );

    FreezeInterfaceForEnemyTurn();

    // Loop through all mercs and set to moved
    FOR_EACH_IN_TEAM(s, gTacticalStatus.ubCurrentTeam) { s->bMoved = TRUE; }

    gTacticalStatus.ubCurrentTeam = ubNextTeam;

    BeginTeamTurn(gTacticalStatus.ubCurrentTeam);

    BetweenTurnsVisibilityAdjustments();
  }
}

void EndAITurn() {
  // Remove any deadlock message
  EndDeadlockMsg();
  if (INTERRUPT_QUEUED) {
    EndInterrupt(FALSE);
  } else {
    FOR_EACH_IN_TEAM(s, gTacticalStatus.ubCurrentTeam) {
      s->bMoved = TRUE;
      // record old life value... for creature AI; the human AI might
      // want to use this too at some point
      s->bOldLife = s->bLife;
    }

    gTacticalStatus.ubCurrentTeam++;
    BeginTeamTurn(gTacticalStatus.ubCurrentTeam);
  }
}

void EndAllAITurns() {
  // warp turn to the player's turn

  // Remove any deadlock message
  EndDeadlockMsg();
  if (INTERRUPT_QUEUED) {
    EndInterrupt(FALSE);
  }

  if (gTacticalStatus.ubCurrentTeam != OUR_TEAM) {
    FOR_EACH_IN_TEAM(s, gTacticalStatus.ubCurrentTeam) {
      s->bMoved = TRUE;
      s->uiStatusFlags &= ~SOLDIER_UNDERAICONTROL;
      // record old life value... for creature AI; the human AI might
      // want to use this too at some point
      s->bOldLife = s->bLife;
    }

    gTacticalStatus.ubCurrentTeam = OUR_TEAM;
    // BeginTeamTurn( gTacticalStatus.ubCurrentTeam );
  }
}

static void EndTurnEvents() {
  // HANDLE END OF TURN EVENTS
  // handle team services like healing
  HandleTeamServices(OUR_TEAM);
  // handle smell and blood decay
  DecaySmells();
  // decay bomb timers and maybe set some off!
  DecayBombTimers();

  DecaySmokeEffects(GetWorldTotalSeconds());
  DecayLightEffects(GetWorldTotalSeconds());

  // decay AI warning values from corpses
  DecayRottingCorpseAIWarnings();
}

void BeginTeamTurn(uint8_t ubTeam) {
  while (1) {
    if (ubTeam > LAST_TEAM) {
      if (HandleAirRaidEndTurn(ubTeam)) {
        // End turn!!
        ubTeam = OUR_TEAM;
        gTacticalStatus.ubCurrentTeam = OUR_TEAM;
        EndTurnEvents();
      } else {
        break;
      }
    } else if (!IsTeamActive(ubTeam)) {
      // inactive team, skip to the next one
      ubTeam++;
      gTacticalStatus.ubCurrentTeam++;
      // skip back to the top, as we are processing another team now.
      continue;
    }

    if (gTacticalStatus.uiFlags & TURNBASED) {
      BeginLoggingForBleedMeToos(TRUE);

      // decay team's public opplist
      DecayPublicOpplist(ubTeam);

      FOR_EACH_IN_TEAM(i, ubTeam) {
        SOLDIERTYPE &s = *i;
        if (s.bLife <= 0) continue;
        // decay personal opplist, and refresh APs and BPs
        EVENT_BeginMercTurn(s);
      }

      if (gTacticalStatus.bBoxingState == LOST_ROUND || gTacticalStatus.bBoxingState == WON_ROUND ||
          gTacticalStatus.bBoxingState == DISQUALIFIED) {
        // we have no business being in here any more!
        return;
      }

      BeginLoggingForBleedMeToos(FALSE);
    }

    if (ubTeam == OUR_TEAM) {
      // ATE: Check if we are still in a valid battle...
      // ( they could have blead to death above )
      if ((gTacticalStatus.uiFlags & INCOMBAT)) {
        StartPlayerTeamTurn(TRUE, FALSE);
      }
      break;
    } else {
      // Set First enemy merc to AI control
      if (BuildAIListForTeam(ubTeam)) {
        SOLDIERTYPE *const s = RemoveFirstAIListEntry();
        if (s != NULL) {
          // Dirty panel interface!
          fInterfacePanelDirty = DIRTYLEVEL2;
          AddTopMessage(COMPUTER_TURN_MESSAGE);
          StartNPCAI(*s);
          return;
        }
      }

      // This team is dead/inactive/being skipped in boxing
      // skip back to the top to process the next team
      ubTeam++;
      gTacticalStatus.ubCurrentTeam++;
    }
  }
}

void DisplayHiddenInterrupt(SOLDIERTYPE *pSoldier) {
  // If the AI got an interrupt but this has been hidden from the player until
  // this point, this code will display the interrupt

  if (!gfHiddenInterrupt) {
    return;
  }
  EndDeadlockMsg();

  if (pSoldier->bVisible != -1) SlideTo(pSoldier, SETLOCATOR);

  guiPendingOverrideEvent = LU_BEGINUILOCK;

  // Dirty panel interface!
  fInterfacePanelDirty = DIRTYLEVEL2;

  // Erase path!
  ErasePath();

  // Reset flags
  gfPlotNewMovement = TRUE;

  // Stop our guy....
  SOLDIERTYPE *const latest = LatestInterruptGuy();
  AdjustNoAPToFinishMove(latest, TRUE);
  // Stop him from going to prone position if doing a turn while prone
  latest->fTurningFromPronePosition = FALSE;

  // get rid of any old overlay message
  const MESSAGE_TYPES msg =
      pSoldier->bTeam == MILITIA_TEAM ? MILITIA_INTERRUPT_MESSAGE : COMPUTER_INTERRUPT_MESSAGE;
  AddTopMessage(msg);

  gfHiddenInterrupt = FALSE;
}

void DisplayHiddenTurnbased(SOLDIERTYPE *pActingSoldier) {
  // This code should put the game in turn-based and give control to the
  // AI-controlled soldier whose pointer has been passed in as an argument (we
  // were in non-combat and the AI is doing something visible, i.e. making an
  // attack)

  if (AreInMeanwhile()) {
    return;
  }

  if (gTacticalStatus.uiFlags & REALTIME || gTacticalStatus.uiFlags & INCOMBAT) {
    // pointless call here; do nothing
    return;
  }

  // Enter combat mode starting with this side's turn
  gTacticalStatus.ubCurrentTeam = pActingSoldier->bTeam;

  CommonEnterCombatModeCode();

  SetSoldierAsUnderAiControl(pActingSoldier);
  DebugAI(String("Giving AI control to %d", pActingSoldier->ubID));
  pActingSoldier->fTurnInProgress = TRUE;
  gTacticalStatus.uiTimeSinceMercAIStart = GetJA2Clock();

  if (gTacticalStatus.ubTopMessageType != COMPUTER_TURN_MESSAGE) {
    // Dirty panel interface!
    fInterfacePanelDirty = DIRTYLEVEL2;
    AddTopMessage(COMPUTER_TURN_MESSAGE);
  }

  // freeze the user's interface
  FreezeInterfaceForEnemyTurn();
}

static BOOLEAN EveryoneInInterruptListOnSameTeam() {
  uint8_t ubLoop;
  uint8_t ubTeam = 255;

  for (ubLoop = 1; ubLoop <= gubOutOfTurnPersons; ubLoop++) {
    if (ubTeam == 255) {
      ubTeam = gOutOfTurnOrder[ubLoop]->bTeam;
    } else {
      if (gOutOfTurnOrder[ubLoop]->bTeam != ubTeam) {
        return (FALSE);
      }
    }
  }
  return (TRUE);
}

void SayCloseCallQuotes() {
  // report any close call quotes for us here
  FOR_EACH_IN_TEAM(s, OUR_TEAM) {
    if (OkControllableMerc(s) && s->fCloseCall && s->bNumHitsThisTurn == 0 &&
        !(s->usQuoteSaidExtFlags & SOLDIER_QUOTE_SAID_EXT_CLOSE_CALL) && Random(3) == 0) {
      // say close call quote!
      TacticalCharacterDialogue(s, QUOTE_CLOSE_CALL);
      s->usQuoteSaidExtFlags |= SOLDIER_QUOTE_SAID_EXT_CLOSE_CALL;
    }
    s->fCloseCall = FALSE;
  }
}

static void DeleteFromIntList(uint8_t ubIndex, BOOLEAN fCommunicate);

static void StartInterrupt() {
  SOLDIERTYPE *first_interrupter = LatestInterruptGuy();
  const int8_t bTeam = first_interrupter->bTeam;
  SOLDIERTYPE *Interrupter = first_interrupter;

  // display everyone on int queue!
  for (int32_t cnt = gubOutOfTurnPersons; cnt > 0; --cnt) {
    DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
             String("STARTINT:  Q position %d: %d", cnt, gOutOfTurnOrder[cnt]->ubID));
  }

  gTacticalStatus.fInterruptOccurred = TRUE;

  FOR_EACH_SOLDIER(s) {
    s->bMovedPriorToInterrupt = s->bMoved;
    s->bMoved = TRUE;
  }

  if (first_interrupter->bTeam == OUR_TEAM) {
    // start interrupts for everyone on our side at once
    wchar_t sTemp[255];
    uint8_t ubInterrupters = 0;

    // build string for display of who gets interrupt
    while (1) {
      Interrupter->bMoved = FALSE;
      DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
               String("INTERRUPT: popping %d off of the interrupt queue", Interrupter->ubID));

      REMOVE_LATEST_INTERRUPT_GUY();
      // now LatestInterruptGuy() is the guy before the previous
      Interrupter = LatestInterruptGuy();

      if (Interrupter == NULL)  // previously emptied slot!
      {
        continue;
      } else if (Interrupter->bTeam != bTeam) {
        break;
      }
    }

    wcscpy(sTemp, g_langRes->Message[STR_INTERRUPT_FOR]);

    // build string in separate loop here, want to linearly process squads...
    for (int32_t iSquad = 0; iSquad < NUMBER_OF_SQUADS; ++iSquad) {
      FOR_EACH_IN_SQUAD(i, iSquad) {
        SOLDIERTYPE const *const s = *i;
        if (!s->bActive) continue;
        if (!s->bInSector) continue;
        if (s->bMoved) continue;
        // then this guy got an interrupt...
        ubInterrupters++;
        if (ubInterrupters > 6) {
          // flush... display string, then clear it (we could have 20 names!)
          // add comma to end, we know we have another person after this...
          wcscat(sTemp, L", ");
          ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, sTemp);
          wcscpy(sTemp, L"");
          ubInterrupters = 1;
        }

        if (ubInterrupters > 1) {
          wcscat(sTemp, L", ");
        }
        wcscat(sTemp, s->name);
      }
    }

    ScreenMsg(FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, sTemp);

    DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
             String("INTERRUPT: starting interrupt for %d", first_interrupter->ubID));

    // Remove deadlock message
    EndDeadlockMsg();

    // Select guy....
    SelectSoldier(first_interrupter, SELSOLDIER_ACKNOWLEDGE | SELSOLDIER_FORCE_RESELECT);

    // ATE; Slide to guy who got interrupted!
    SlideTo(gLastInterruptedGuy, SETLOCATOR);

    // Dirty panel interface!
    fInterfacePanelDirty = DIRTYLEVEL2;
    gTacticalStatus.ubCurrentTeam = first_interrupter->bTeam;

    // Signal UI done enemy's turn
    guiPendingOverrideEvent = LU_ENDUILOCK;
    HandleTacticalUI();

    InitPlayerUIBar(TRUE);
    // AddTopMessage(PLAYER_INTERRUPT_MESSAGE);

    PlayJA2Sample(ENDTURN_1, MIDVOLUME, 1, MIDDLEPAN);

    SayCloseCallQuotes();
  } else {
    // start interrupts for everyone on that side at once... and start AI with
    // the lowest # guy

    // what we do is set everyone to moved except for people with interrupts at
    // the moment
    /*
    FOR_EACH_IN_TEAM(s, first_interrupter->bTeam)
    {
            s->bMovedPriorToInterrupt = s->bMoved;
            s->bMoved                 = TRUE;
    }
    */

    while (1) {
      Interrupter->bMoved = FALSE;

      DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
               String("INTERRUPT: popping %d off of the interrupt queue", Interrupter->ubID));

      REMOVE_LATEST_INTERRUPT_GUY();
      // now LatestInterruptGuy() is the guy before the previous
      Interrupter = LatestInterruptGuy();
      if (Interrupter == NULL)  // previously emptied slot!
      {
        continue;
      } else if (Interrupter->bTeam != bTeam) {
        break;
      } else if (Interrupter->ubID < first_interrupter->ubID) {
        first_interrupter = Interrupter;
      }
    }

    // here we have to rebuilt the AI list!
    BuildAIListForTeam(bTeam);

    // set to the new first interrupter
    SOLDIERTYPE *const pSoldier = RemoveFirstAIListEntry();

    // if ( gTacticalStatus.ubCurrentTeam == OUR_TEAM)
    if (pSoldier->bTeam != OUR_TEAM) {
      // we're being interrupted by the computer!
      // we delay displaying any interrupt message until the computer
      // does something...
      gfHiddenInterrupt = TRUE;
      gTacticalStatus.fUnLockUIAfterHiddenInterrupt = FALSE;
    }
    // otherwise it's the AI interrupting another AI team

    gTacticalStatus.ubCurrentTeam = pSoldier->bTeam;

    StartNPCAI(*pSoldier);
  }

  if (!gfHiddenInterrupt) {
    // Stop this guy....
    SOLDIERTYPE *const latest = LatestInterruptGuy();
    AdjustNoAPToFinishMove(latest, TRUE);
    latest->fTurningFromPronePosition = FALSE;
  }
}

static void EndInterrupt(BOOLEAN fMarkInterruptOccurred) {
  BOOLEAN fFound;
  uint8_t ubMinAPsToAttack;

  for (int32_t cnt = gubOutOfTurnPersons; cnt > 0; --cnt) {
    DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
             String("ENDINT:  Q position %d: %d", cnt, gOutOfTurnOrder[cnt]->ubID));
  }

  // ATE: OK, now if this all happended on one frame, we may not have to stop
  // guy from walking... so set this flag to false if so...
  if (fMarkInterruptOccurred) {
    // flag as true if an int occurs which ends an interrupt (int loop)
    gTacticalStatus.fInterruptOccurred = TRUE;
  } else {
    gTacticalStatus.fInterruptOccurred = FALSE;
  }

  // Loop through all mercs and see if any passed on this interrupt
  FOR_EACH_IN_TEAM(s, gTacticalStatus.ubCurrentTeam) {
    if (s->bInSector && !s->bMoved && s->bActionPoints == s->bIntStartAPs) {
      ubMinAPsToAttack = MinAPsToAttack(s, s->sLastTarget, FALSE);
      if (0 < ubMinAPsToAttack && ubMinAPsToAttack <= s->bActionPoints) {
        s->bPassedLastInterrupt = TRUE;
      }
    }
  }

  if (!EveryoneInInterruptListOnSameTeam()) {
    gfHiddenInterrupt = FALSE;

    // resume interrupted interrupt
    StartInterrupt();
  } else {
    SOLDIERTYPE *const interrupted = LatestInterruptGuy();
    DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
             String("INTERRUPT: interrupt over, %d's team regains control", interrupted->ubID));

    FOR_EACH_SOLDIER(s) {
      // AI guys only here...
      if (s->bActionPoints == 0) {
        s->bMoved = TRUE;
      } else if (s->bTeam != OUR_TEAM && s->bNewSituation == IS_NEW_SITUATION) {
        s->bMoved = FALSE;
      } else {
        s->bMoved = s->bMovedPriorToInterrupt;
      }
    }

    // change team
    gTacticalStatus.ubCurrentTeam = interrupted->bTeam;
    // switch appropriate messages & flags
    if (interrupted->bTeam == OUR_TEAM) {
      // set everyone on the team to however they were set moved before the
      // interrupt must do this before selecting soldier...
      /*
      FOR_EACH_IN_TEAM(s, gTacticalStatus.ubCurrentTeam)
      {
              s->bMoved = s->bMovedPriorToInterrupt;
      }
      */

      ClearIntList();

      // Select soldier....
      if (interrupted->bLife < OKLIFE) {
        SelectNextAvailSoldier(interrupted);
      } else {
        SelectSoldier(interrupted, SELSOLDIER_NONE);
      }

      if (gfHiddenInterrupt) {
        // Try to make things look like nothing happened at all.
        gfHiddenInterrupt = FALSE;

        // If we can continue a move, do so!
        SOLDIERTYPE *const sel = GetSelectedMan();
        if (sel->fNoAPToFinishMove && interrupted->ubReasonCantFinishMove != REASON_STOPPED_SIGHT) {
          // Continue
          AdjustNoAPToFinishMove(sel, FALSE);

          if (sel->sGridNo != sel->sFinalDestination) {
            EVENT_GetNewSoldierPath(sel, sel->sFinalDestination, sel->usUIMovementMode);
          } else {
            UnSetUIBusy(interrupted);
          }
        } else {
          UnSetUIBusy(interrupted);
        }

        if (gTacticalStatus.fUnLockUIAfterHiddenInterrupt) {
          gTacticalStatus.fUnLockUIAfterHiddenInterrupt = FALSE;
          UnSetUIBusy(interrupted);
        }
      } else {
        // Signal UI done enemy's turn
        /// ATE: This used to be ablow so it would get done for
        // both hidden interrupts as well - NOT good because
        // hidden interrupts should leave it locked if it was already...
        guiPendingOverrideEvent = LU_ENDUILOCK;
        HandleTacticalUI();

        SOLDIERTYPE *const sel = GetSelectedMan();
        if (sel != NULL) {
          SlideTo(sel, SETLOCATOR);

          // Say ATTENTION SOUND...
          DoMercBattleSound(sel, BATTLE_SOUND_ATTN1);

          if (gsInterfaceLevel == 1) {
            gTacticalStatus.uiFlags |= SHOW_ALL_ROOFS;
            InvalidateWorldRedundency();
            SetRenderFlags(RENDER_FLAG_FULL);
            ErasePath();
          }
        }
        // 2 indicates that we're ending an interrupt and going back to
        // normal player's turn without readjusting time left in turn (for
        // timed turns)
        InitPlayerUIBar(2);
      }

    } else {
      // this could be set to true for AI-vs-AI interrupts
      gfHiddenInterrupt = FALSE;

      // Dirty panel interface!
      fInterfacePanelDirty = DIRTYLEVEL2;

      // Erase path!
      ErasePath();

      // Reset flags
      gfPlotNewMovement = TRUE;

      // restart AI with first available soldier
      fFound = FALSE;

      // rebuild list for this team if anyone on the team is still available
      int32_t cnt = gTacticalStatus.Team[ENEMY_TEAM].bFirstID;
      for (SOLDIERTYPE *pTempSoldier = &GetMan(cnt);
           cnt <= gTacticalStatus.Team[gTacticalStatus.ubCurrentTeam].bLastID;
           cnt++, pTempSoldier++) {
        if (pTempSoldier->bActive && pTempSoldier->bInSector && pTempSoldier->bLife >= OKLIFE) {
          fFound = TRUE;
          break;
        }
      }

      if (fFound) {
        // reset found flag because we are rebuilding the AI list
        fFound = FALSE;

        if (BuildAIListForTeam(gTacticalStatus.ubCurrentTeam)) {
          // now bubble up everyone left in the interrupt queue, starting
          // at the front of the array
          for (int32_t cnt = 1; cnt <= gubOutOfTurnPersons; ++cnt) {
            MoveToFrontOfAIList(gOutOfTurnOrder[cnt]);
          }

          SOLDIERTYPE *const s = RemoveFirstAIListEntry();
          if (s != NULL) {
            fFound = TRUE;
            StartNPCAI(*s);
          }
        }
      }

      AddTopMessage(COMPUTER_TURN_MESSAGE);

      // Signal UI done enemy's turn
      guiPendingOverrideEvent = LU_BEGINUILOCK;

      ClearIntList();

      if (!fFound) EndAITurn();
    }

    // Reset our interface!
    fInterfacePanelDirty = DIRTYLEVEL2;
  }
}

BOOLEAN StandardInterruptConditionsMet(const SOLDIERTYPE *const pSoldier,
                                       const SOLDIERTYPE *const pOpponent,
                                       const int8_t bOldOppList) {
  //	uint8_t ubAniType;
  uint8_t ubMinPtsNeeded;
  int8_t bDir;

  if ((gTacticalStatus.uiFlags & TURNBASED) && (gTacticalStatus.uiFlags & INCOMBAT) &&
      !(gubSightFlags & SIGHT_INTERRUPT)) {
    return (FALSE);
  }

  if (gTacticalStatus.ubAttackBusyCount > 0) {
    return (FALSE);
  }

  if (pOpponent == NULL) {
    // no opponent, so controller of 'ptr' makes the call instead
    // ALEX
    if (gWhoThrewRock == NULL) {
#ifdef BETAVERSION
      NumMessage(
          "StandardInterruptConditions: ERROR - pOpponent is NULL, "
          "don't know who threw rock, guynum = ",
          pSoldier->guynum);
#endif

      return (FALSE);
    }
  }

  // in non-combat allow interrupt points to be calculated freely (everyone's in
  // control!) also allow calculation for storing in AllTeamsLookForAll
  if ((gTacticalStatus.uiFlags & INCOMBAT) &&
      (gubBestToMakeSightingSize != BEST_SIGHTING_ARRAY_SIZE_ALL_TEAMS_LOOK_FOR_ALL)) {
    // if his team's already in control
    if (pSoldier->bTeam == gTacticalStatus.ubCurrentTeam) {
      // CJC, July 9 1998
      // NO ONE EVER interrupts his own team
      return FALSE;
    } else if (gTacticalStatus.bBoxingState != NOT_BOXING) {
      // while anything to do with boxing is going on, skip interrupts!
      return (FALSE);
    }
  }

  if (!(pSoldier->bActive) || !(pSoldier->bInSector)) {
    return (FALSE);
  }

  // soldiers at less than OKLIFE can't perform any actions
  if (pSoldier->bLife < OKLIFE) {
    return (FALSE);
  }

  // soldiers out of breath are about to fall over, no interrupt
  if (pSoldier->bBreath < OKBREATH || pSoldier->bCollapsed) {
    return (FALSE);
  }

  // if soldier doesn't have enough APs
  if (pSoldier->bActionPoints < MIN_APS_TO_INTERRUPT) {
    return (FALSE);
  }

  // soldiers gagging on gas are too busy about holding their cookies down...
  if (pSoldier->uiStatusFlags & SOLDIER_GASSED) {
    return (FALSE);
  }

  // a soldier already engaged in a life & death battle is too busy doing his
  // best to survive to worry about "getting the jump" on additional threats
  if (pSoldier->bUnderFire) {
    return (FALSE);
  }

  if (pSoldier->bCollapsed) {
    return (FALSE);
  }

  // don't allow neutral folks to get interrupts
  if (pSoldier->bNeutral) {
    return (FALSE);
  }

  // no EPCs allowed to get interrupts
  if (AM_AN_EPC(pSoldier) && !AM_A_ROBOT(pSoldier)) {
    return (FALSE);
  }

  // don't let mercs on assignment get interrupts
  if (pSoldier->bTeam == OUR_TEAM && pSoldier->bAssignment >= ON_DUTY) {
    return (FALSE);
  }

  // the bare minimum default is enough APs left to TURN
  ubMinPtsNeeded = AP_CHANGE_FACING;

  // if the opponent is SOMEBODY
  if (pOpponent != NULL) {
    // if the soldiers are on the same side
    if (pSoldier->bSide == pOpponent->bSide) {
      // human/civilians on same side can't interrupt each other
      if (pSoldier->uiStatusFlags & SOLDIER_PC || IsOnCivTeam(pSoldier)) {
        return (FALSE);
      } else  // enemy
      {
        // enemies can interrupt EACH OTHER, but enemies and civilians on the
        // same side (but different teams) can't interrupt each other.
        if (pSoldier->bTeam != pOpponent->bTeam) {
          return (FALSE);
        }
      }
    }

    // if the interrupted opponent is not the selected character, then the only
    // people eligible to win an interrupt are those on the SAME SIDE AS
    // the selected character, ie. his friends...
    if (pOpponent->bTeam == OUR_TEAM) {
      const SOLDIERTYPE *const sel = GetSelectedMan();
      if (pOpponent != sel && pSoldier->bSide != sel->bSide) {
        return (FALSE);
      }
    } else {
      if (!(pOpponent->uiStatusFlags & SOLDIER_UNDERAICONTROL) &&
          (pSoldier->bSide != pOpponent->bSide)) {
        return (FALSE);
      }
    }

    // an non-active soldier can't interrupt a soldier who is also non-active!
    if ((pOpponent->bTeam != gTacticalStatus.ubCurrentTeam) &&
        (pSoldier->bTeam != gTacticalStatus.ubCurrentTeam)) {
      return (FALSE);
    }

    // if this is a "SEEING" interrupt
    if (pSoldier->bOppList[pOpponent->ubID] == SEEN_CURRENTLY) {
      // if pSoldier already saw the opponent last "look" or at least this turn
      if ((bOldOppList == SEEN_CURRENTLY) || (bOldOppList == SEEN_THIS_TURN)) {
        return (FALSE);  // no interrupt is possible
      }

      // if the soldier is behind him and not very close, forget it
      bDir = atan8(pSoldier->sX, pSoldier->sY, pOpponent->sX, pOpponent->sY);
      if (OppositeDirection(pSoldier->bDesiredDirection) == bDir) {
        // directly behind; allow interrupts only within # of tiles equal to
        // level
        if (PythSpacesAway(pSoldier->sGridNo, pOpponent->sGridNo) > EffectiveExpLevel(pSoldier)) {
          return (FALSE);
        }
      }

      // if the soldier isn't currently crouching
      if (!PTR_CROUCHED) {
        ubMinPtsNeeded = AP_CROUCH;
      } else {
        ubMinPtsNeeded = MinPtsToMove(pSoldier);
      }
    } else  // this is a "HEARING" interrupt
    {
      // if the opponent can't see the "interrupter" either, OR
      // if the "interrupter" already has any opponents already in sight, OR
      // if the "interrupter" already heard the active soldier this turn
      if ((pOpponent->bOppList[pSoldier->ubID] != SEEN_CURRENTLY) || (pSoldier->bOppCnt > 0) ||
          (bOldOppList == HEARD_THIS_TURN)) {
        return (FALSE);  // no interrupt is possible
      }
    }
  }

  // soldiers without sufficient APs to do something productive can't interrupt
  if (pSoldier->bActionPoints < ubMinPtsNeeded) {
    return (FALSE);
  }

  // soldier passed on the chance to react during previous interrupt this turn
  if (pSoldier->bPassedLastInterrupt) {
    return (FALSE);
  }

#ifdef RECORDINTERRUPT
  // this usually starts a new series of logs, so that's why the blank line
  fprintf(InterruptFile, "\nStandardInterruptConditionsMet by %d vs. %d\n", pSoldier->guynum,
          pOpponent->ubID);
#endif

  return (TRUE);
}

int8_t CalcInterruptDuelPts(const SOLDIERTYPE *const pSoldier, const SOLDIERTYPE *const opponent,
                            BOOLEAN fUseWatchSpots) {
  int8_t bPoints;
  int8_t bLightLevel;

  // extra check to make sure neutral folks never get interrupts
  if (pSoldier->bNeutral) {
    return (NO_INTERRUPT);
  }

  // BASE is one point for each experience level.

  // Robot has interrupt points based on the controller...
  // Controller's interrupt points are reduced by 2 for being distracted...
  if (pSoldier->uiStatusFlags & SOLDIER_ROBOT && CanRobotBeControlled(pSoldier)) {
    bPoints = EffectiveExpLevel(pSoldier->robot_remote_holder) - 2;
  } else {
    bPoints = EffectiveExpLevel(pSoldier);
    /*
    if ( pSoldier->bTeam == ENEMY_TEAM )
    {
            // modify by the difficulty level setting
            bPoints += gbDiff[ DIFF_ENEMY_INTERRUPT_MOD ][
    SoldierDifficultyLevel( pSoldier ) ]; bPoints = std::max( bPoints, 9 );
    }
    */

    if (ControllingRobot(pSoldier)) {
      bPoints -= 2;
    }
  }

  if (fUseWatchSpots) {
    // if this is a previously noted spot of enemies, give bonus points!
    bPoints += GetWatchedLocPoints(pSoldier->ubID, opponent->sGridNo, opponent->bLevel);
  }

  // LOSE one point for each 2 additional opponents he currently sees, above 2
  if (pSoldier->bOppCnt > 2) {
    // subtract 1 here so there is a penalty of 1 for seeing 3 enemies
    bPoints -= (pSoldier->bOppCnt - 1) / 2;
  }

  // LOSE one point if he's trying to interrupt only by hearing
  if (pSoldier->bOppList[opponent->ubID] == HEARD_THIS_TURN) {
    bPoints--;
  }

  // if soldier is still in shock from recent injuries, that penalizes him
  bPoints -= pSoldier->bShock;

  const uint8_t ubDistance = PythSpacesAway(pSoldier->sGridNo, opponent->sGridNo);

  // if we are in combat mode - thus doing an interrupt rather than determine
  // who gets first turn - then give bonus
  if ((gTacticalStatus.uiFlags & INCOMBAT) && (pSoldier->bTeam != gTacticalStatus.ubCurrentTeam)) {
    // passive player gets penalty due to range
    bPoints -= (ubDistance / 10);
  } else {
    // either non-combat or the player with the current turn... i.e. active...
    // unfortunately we can't use opplist here to record whether or not we saw
    // this guy before, because at this point the opplist has been updated to
    // seen.  But we can use gbSeenOpponents ...

    // this soldier is moving, so give them a bonus for crawling or swatting at
    // long distances
    if (!gbSeenOpponents[opponent->ubID][pSoldier->ubID]) {
      if (pSoldier->usAnimState == SWATTING &&
          ubDistance > (MaxDistanceVisible() / 2))  // more than 1/2 sight distance
      {
        bPoints++;
      } else if (pSoldier->usAnimState == CRAWLING &&
                 ubDistance > (MaxDistanceVisible() / 4))  // more than 1/4 sight distance
      {
        bPoints += ubDistance / STRAIGHT;
      }
    }
  }

  // whether active or not, penalize people who are running
  if (pSoldier->usAnimState == RUNNING && !gbSeenOpponents[pSoldier->ubID][opponent->ubID]) {
    bPoints -= 2;
  }

  if (pSoldier->service_partner != NULL) {
    // distracted by being bandaged/doing bandaging
    bPoints -= 2;
  }

  if (HAS_SKILL_TRAIT(pSoldier, NIGHTOPS)) {
    bLightLevel = LightTrueLevel(pSoldier->sGridNo, pSoldier->bLevel);
    if (bLightLevel > NORMAL_LIGHTLEVEL_DAY + 3) {
      // it's dark, give a bonus for interrupts
      bPoints += 1 * NUM_SKILL_TRAITS(pSoldier, NIGHTOPS);
    }
  }

  // if he's a computer soldier

  // CJC note: this will affect friendly AI as well...

  if (pSoldier->uiStatusFlags & SOLDIER_PC) {
    if (pSoldier->bAssignment >= ON_DUTY) {
      // make sure don't get interrupts!
      bPoints = -10;
    }

    // GAIN one point if he's previously seen the opponent
    // check for TRUE because -1 means we JUST saw him (always so here)
    if (gbSeenOpponents[pSoldier->ubID][opponent->ubID] == TRUE) {
      bPoints++;  // seen him before, easier to react to him
    }
  } else if (pSoldier->bTeam == ENEMY_TEAM) {
    // GAIN one point if he's previously seen the opponent
    // check for TRUE because -1 means we JUST saw him (always so here)
    if (gbSeenOpponents[pSoldier->ubID][opponent->ubID] == TRUE) {
      bPoints++;  // seen him before, easier to react to him
    } else if (gbPublicOpplist[pSoldier->bTeam][opponent->ubID] != NOT_HEARD_OR_SEEN) {
      // GAIN one point if opponent has been recently radioed in by his team
      bPoints++;
    }
  }

  if (TANK(pSoldier)) {
    // reduce interrupt possibilities for tanks!
    bPoints /= 2;
  }

  if (bPoints >= AUTOMATIC_INTERRUPT) {
#ifdef BETAVERSION
    NumMessage(
        "CalcInterruptDuelPts: ERROR - Invalid bInterruptDuelPts "
        "calculated for soldier ",
        pSoldier->guynum);
#endif
    bPoints = AUTOMATIC_INTERRUPT - 1;  // hack it to one less than max so its legal
  }

#ifdef DEBUG_INTERRUPTS
  DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
           String("Calculating int pts for %d vs %d, number is %d", pSoldier->ubID, opponent->ubID,
                  bPoints));
#endif

  return (bPoints);
}

BOOLEAN InterruptDuel(SOLDIERTYPE *pSoldier, SOLDIERTYPE *pOpponent) {
  BOOLEAN fResult = FALSE;

  // if opponent can't currently see us and we can see them
  if (pSoldier->bOppList[pOpponent->ubID] == SEEN_CURRENTLY &&
      pOpponent->bOppList[pSoldier->ubID] != SEEN_CURRENTLY) {
    fResult = TRUE;  // we automatically interrupt
    // fix up our interrupt duel pts if necessary
    if (pSoldier->bInterruptDuelPts < pOpponent->bInterruptDuelPts) {
      pSoldier->bInterruptDuelPts = pOpponent->bInterruptDuelPts;
    }
  } else {
    // If our total points is HIGHER, then we interrupt him anyway
    if (pSoldier->bInterruptDuelPts > pOpponent->bInterruptDuelPts) {
      fResult = TRUE;
    }
  }
  //	ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE, L"Interrupt duel %d (%d
  // pts) vs %d (%d pts)", pSoldier->ubID, pSoldier->bInterruptDuelPts,
  // pOpponent->ubID, pOpponent->bInterruptDuelPts );
  return (fResult);
}

static void DeleteFromIntList(uint8_t ubIndex, BOOLEAN fCommunicate) {
  uint8_t ubLoop;

  if (ubIndex > gubOutOfTurnPersons) {
    return;
  }

#ifdef DEBUG_INTERRUPTS
  DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
           String("INTERRUPT: removing ID %d", gOutOfTurnOrder[ubIndex]->ubID));
#endif
  // if we're NOT deleting the LAST entry in the int list
  if (ubIndex < gubOutOfTurnPersons) {
    // not the last entry, must move all those behind it over to fill the gap
    for (ubLoop = ubIndex; ubLoop < gubOutOfTurnPersons; ubLoop++) {
      gOutOfTurnOrder[ubLoop] = gOutOfTurnOrder[ubLoop + 1];
    }
  }

  // either way, whack the last entry to NOBODY and decrement the list size
  gOutOfTurnOrder[gubOutOfTurnPersons] = NULL;
  gubOutOfTurnPersons--;
}

void AddToIntList(SOLDIERTYPE *const s, const BOOLEAN fGainControl, const BOOLEAN fCommunicate) {
  uint8_t ubLoop;

  DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
           String("INTERRUPT: adding ID %d who %s", s->ubID,
                  fGainControl ? "gains control" : "loses control"));

  // check whether 'who' is already anywhere on the queue after the first index
  // which we want to preserve so we can restore turn order
  for (ubLoop = 2; ubLoop <= gubOutOfTurnPersons; ubLoop++) {
    if (gOutOfTurnOrder[ubLoop] == s) {
      if (!fGainControl) {
        // he's LOSING control; that's it, we're done, DON'T add him to the
        // queue again
        gLastInterruptedGuy = s;
        return;
      } else {
        // GAINING control, so delete him from this slot (because later he'll
        // get added to the end and we don't want him listed more than once!)
        DeleteFromIntList(ubLoop, FALSE);
      }
    }
  }

  // increment total (making index valid) and add him to list
  gubOutOfTurnPersons++;
  gOutOfTurnOrder[gubOutOfTurnPersons] = s;

  // if the guy is gaining control
  if (fGainControl) {
    // record his initial APs at the start of his interrupt at this time
    // this is not the ideal place for this, but it's the best I could do...
    s->bIntStartAPs = s->bActionPoints;
  } else {
    gLastInterruptedGuy = s;
    // turn off AI control flag if they lost control
    if (s->uiStatusFlags & SOLDIER_UNDERAICONTROL) {
      DebugAI(String("Taking away AI control from %d", s->ubID));
      s->uiStatusFlags &= ~SOLDIER_UNDERAICONTROL;
    }
  }
}

static void VerifyOutOfTurnOrderArray() {
  uint8_t ubTeamHighest[MAXTEAMS] = {0};
  uint8_t ubTeamsInList;
  uint8_t ubNextIndex;
  uint8_t ubTeam;
  uint8_t ubLoop, ubLoop2;
  BOOLEAN fFoundLoop = FALSE;

  for (ubLoop = 1; ubLoop <= gubOutOfTurnPersons; ubLoop++) {
    ubTeam = gOutOfTurnOrder[ubLoop]->bTeam;
    if (ubTeamHighest[ubTeam] > 0) {
      // check the other teams to see if any of them are between our last team's
      // mention in the array and this
      for (ubLoop2 = 0; ubLoop2 < MAXTEAMS; ubLoop2++) {
        if (ubLoop2 == ubTeam) {
          continue;
        } else {
          if (ubTeamHighest[ubLoop2] > ubTeamHighest[ubTeam]) {
            // there's a loop!! delete it!
            const SOLDIERTYPE *const NextInArrayOnTeam = gOutOfTurnOrder[ubLoop];
            ubNextIndex = ubTeamHighest[ubTeam] + 1;

            while (gOutOfTurnOrder[ubNextIndex] != NextInArrayOnTeam) {
              // Pause them...
              AdjustNoAPToFinishMove(gOutOfTurnOrder[ubNextIndex], TRUE);

              // If they were turning from prone, stop them
              gOutOfTurnOrder[ubNextIndex]->fTurningFromPronePosition = FALSE;

              DeleteFromIntList(ubNextIndex, FALSE);
            }

            fFoundLoop = TRUE;
            break;
          }
        }
      }

      if (fFoundLoop) {
        // at this point we should restart our outside loop (ugh)
        fFoundLoop = FALSE;
        for (ubLoop2 = 0; ubLoop2 < MAXTEAMS; ubLoop2++) {
          ubTeamHighest[ubLoop2] = 0;
        }
        ubLoop = 0;
        continue;
      }
    }

    ubTeamHighest[ubTeam] = ubLoop;
  }

  // Another potential problem: the player is interrupted by the enemy who is
  // interrupted by the militia.  In this situation the enemy should just lose
  // their interrupt. (Or, the militia is interrupted by the enemy who is
  // interrupted by the player.)

  // Check for 3+ teams in the interrupt queue.  If three exist then abort all
  // interrupts (return control to the first team)
  ubTeamsInList = 0;
  for (ubLoop = 0; ubLoop < MAXTEAMS; ubLoop++) {
    if (ubTeamHighest[ubLoop] > 0) {
      ubTeamsInList++;
    }
  }
  if (ubTeamsInList >= 3) {
    // This is bad.  Loop through everyone but the first person in the int32_t list
    // and remove 'em
    for (ubLoop = 2; ubLoop <= gubOutOfTurnPersons;) {
      if (gOutOfTurnOrder[ubLoop]->bTeam != gOutOfTurnOrder[1]->bTeam) {
        // remove!

        // Pause them...
        AdjustNoAPToFinishMove(gOutOfTurnOrder[ubLoop], TRUE);

        // If they were turning from prone, stop them
        gOutOfTurnOrder[ubLoop]->fTurningFromPronePosition = FALSE;

        DeleteFromIntList(ubLoop, FALSE);

        // since we deleted someone from the list, we want to check the same
        // index in the array again, hence we DON'T increment.
      } else {
        ubLoop++;
      }
    }
  }
}

void DoneAddingToIntList() {
  VerifyOutOfTurnOrderArray();
  if (EveryoneInInterruptListOnSameTeam()) {
    EndInterrupt(TRUE);
  } else {
    StartInterrupt();
  }
}

void ResolveInterruptsVs(SOLDIERTYPE *pSoldier, uint8_t ubInterruptType) {
  uint8_t ubIntCnt;
  SOLDIERTYPE *IntList[MAXMERCS];
  uint8_t ubIntDiff[MAXMERCS];
  uint8_t ubSmallestDiff;
  uint8_t ubSlot, ubSmallestSlot;
  BOOLEAN fIntOccurs;

  if ((gTacticalStatus.uiFlags & TURNBASED) && (gTacticalStatus.uiFlags & INCOMBAT)) {
    ubIntCnt = 0;

    for (uint8_t ubTeam = 0; ubTeam < MAXTEAMS; ++ubTeam) {
      if (IsTeamActive(ubTeam) && gTacticalStatus.Team[ubTeam].bSide != pSoldier->bSide &&
          ubTeam != CIV_TEAM) {
        FOR_EACH_IN_TEAM(pOpponent, ubTeam) {
          if (pOpponent->bInSector && pOpponent->bLife >= OKLIFE && !pOpponent->bCollapsed) {
            if (ubInterruptType == NOISEINTERRUPT) {
              // don't grant noise interrupts at greater than max. visible
              // distance
              if (PythSpacesAway(pSoldier->sGridNo, pOpponent->sGridNo) > MaxDistanceVisible()) {
                pOpponent->bInterruptDuelPts = NO_INTERRUPT;
#ifdef DEBUG_INTERRUPTS
                DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
                         String("Resetting int pts for %d - NOISE BEYOND SIGHT "
                                "DISTANCE!?",
                                pOpponent->ubID));
#endif
                continue;
              }
            } else if (pOpponent->bOppList[pSoldier->ubID] != SEEN_CURRENTLY) {
              pOpponent->bInterruptDuelPts = NO_INTERRUPT;
#ifdef DEBUG_INTERRUPTS
              DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
                       String("Resetting int pts for %d - DOESN'T SEE ON SIGHT "
                              "INTERRUPT!?",
                              pOpponent->ubID));
#endif

              continue;
            }

            switch (pOpponent->bInterruptDuelPts) {
              case NO_INTERRUPT:  // no interrupt possible, no duel necessary
                fIntOccurs = FALSE;
                break;

              case AUTOMATIC_INTERRUPT:           // interrupts occurs automatically
                pSoldier->bInterruptDuelPts = 0;  // just to have a valid intDiff later
                fIntOccurs = TRUE;
#ifdef DEBUG_INTERRUPTS
                DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
                         String("INTERRUPT: automatic interrupt on %d by %d", pSoldier->ubID,
                                pOpponent->ubID));
#endif
                break;

              default:  // interrupt is possible, run a duel
                DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
                         "Calculating int duel pts for onlooker in "
                         "ResolveInterruptsVs");
                pSoldier->bInterruptDuelPts = CalcInterruptDuelPts(pSoldier, pOpponent, TRUE);
                fIntOccurs = InterruptDuel(pOpponent, pSoldier);
#ifdef DEBUG_INTERRUPTS
                if (fIntOccurs) {
                  DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
                           String("INTERRUPT: standard interrupt on %d (%d pts) "
                                  "by %d (%d pts)",
                                  pSoldier->ubID, pSoldier->bInterruptDuelPts, pOpponent->ubID,
                                  pOpponent->bInterruptDuelPts));
                }
#endif

                break;
            }

            if (fIntOccurs) {
              // remember that this opponent's scheduled to interrupt us
              IntList[ubIntCnt] = pOpponent;

              // and by how much he beat us in the duel
              ubIntDiff[ubIntCnt] = pOpponent->bInterruptDuelPts - pSoldier->bInterruptDuelPts;

              // increment counter of interrupts lost
              ubIntCnt++;
            } else {
              /*
                      if (pOpponent->bInterruptDuelPts != NO_INTERRUPT)
                      {
                              ScreenMsg( FONT_MCOLOR_LTYELLOW, MSG_INTERFACE,
                 L"%d fails to interrupt %d (%d vs %d pts)", pOpponent->ubID,
                 pSoldier->ubID, pOpponent->bInterruptDuelPts,
                 pSoldier->bInterruptDuelPts);
                      }
                      */
            }

// either way, clear out both sides' bInterruptDuelPts field to prepare next one
#ifdef DEBUG_INTERRUPTS
            if (pSoldier->bInterruptDuelPts != NO_INTERRUPT) {
              DebugMsg(TOPIC_JA2, DBG_LEVEL_3, String("Resetting int pts for %d", pSoldier->ubID));
            }
#endif

            pSoldier->bInterruptDuelPts = NO_INTERRUPT;

#ifdef DEBUG_INTERRUPTS
            if (pOpponent->bInterruptDuelPts != NO_INTERRUPT) {
              DebugMsg(TOPIC_JA2, DBG_LEVEL_3, String("Resetting int pts for %d", pOpponent->ubID));
            }
#endif
            pOpponent->bInterruptDuelPts = NO_INTERRUPT;
          }
        }
      }
    }

    // if any interrupts are scheduled to occur (ie. I lost at least once)
    if (ubIntCnt) {
      // First add currently active character to the interrupt queue.  This is
      // USUALLY pSoldier->guynum, but NOT always, because one enemy can
      // "interrupt" on another enemy's turn if he hears another team's wound
      // victim's screaming...  the guy screaming is pSoldier here, it's not his
      // turn!
      // AddToIntList(GetSelectedMan(), FALSE, TRUE);

      if ((gTacticalStatus.ubCurrentTeam != pSoldier->bTeam) &&
          !(gTacticalStatus.Team[gTacticalStatus.ubCurrentTeam].bHuman)) {
        // if anyone on this team is under AI control, remove
        // their AI control flag and put them on the queue instead of this guy
        FOR_EACH_IN_TEAM(s, gTacticalStatus.ubCurrentTeam) {
          if (s->uiStatusFlags & SOLDIER_UNDERAICONTROL) {
            // this guy lost control
            s->uiStatusFlags &= ~SOLDIER_UNDERAICONTROL;
            AddToIntList(s, FALSE, TRUE);
            break;
          }
        }

      } else {
        // this guy lost control
        AddToIntList(pSoldier, FALSE, TRUE);
      }

      // loop once for each opponent who interrupted
      for (uint8_t ubLoop = 0; ubLoop < ubIntCnt; ++ubLoop) {
        // find the smallest intDiff still remaining in the list
        ubSmallestDiff = NO_INTERRUPT;
        ubSmallestSlot = NOBODY;

        for (ubSlot = 0; ubSlot < ubIntCnt; ubSlot++) {
          if (ubIntDiff[ubSlot] < ubSmallestDiff) {
            ubSmallestDiff = ubIntDiff[ubSlot];
            ubSmallestSlot = ubSlot;
          }
        }

        if (ubSmallestSlot < NOBODY) {
          // add this guy to everyone's interrupt queue
          AddToIntList(IntList[ubSmallestSlot], TRUE, TRUE);
          if (INTERRUPTS_OVER) {
            // a loop was created which removed all the people in the interrupt
            // queue!
            EndInterrupt(TRUE);
            return;
          }

          ubIntDiff[ubSmallestSlot] = NO_INTERRUPT;  // mark slot as been handled
        }
      }

      // sends off an end-of-list msg telling everyone whether to switch
      // control, unless it's a MOVEMENT interrupt, in which case that is
      // delayed til later
      DoneAddingToIntList();
    }
  }
}

void SaveTeamTurnsToTheSaveGameFile(HWFILE const f) {
  uint8_t data[174];
  uint8_t *d = data;
  for (size_t i = 0; i != lengthof(gOutOfTurnOrder); ++i) {
    INJ_SOLDIER(d, gOutOfTurnOrder[i])
  }
  INJ_U8(d, gubOutOfTurnPersons)
  INJ_SKIP(d, 3)
  INJ_SOLDIER(d, gWhoThrewRock)
  INJ_SKIP(d, 2)
  INJ_BOOL(d, gfHiddenInterrupt)
  INJ_SOLDIER(d, gLastInterruptedGuy)
  INJ_SKIP(d, 17)
  Assert(d == endof(data));

  FileWrite(f, data, sizeof(data));
}

void LoadTeamTurnsFromTheSavedGameFile(HWFILE const f) {
  uint8_t data[174];
  FileRead(f, data, sizeof(data));

  uint8_t const *d = data;
  EXTR_SKIP(d, 1)
  for (size_t i = 1; i != lengthof(gOutOfTurnOrder); ++i) {
    EXTR_SOLDIER(d, gOutOfTurnOrder[i])
  }
  EXTR_U8(d, gubOutOfTurnPersons)
  EXTR_SKIP(d, 3)
  EXTR_SOLDIER(d, gWhoThrewRock)
  EXTR_SKIP(d, 2)
  EXTR_BOOL(d, gfHiddenInterrupt)
  EXTR_SOLDIER(d, gLastInterruptedGuy)
  EXTR_SKIP(d, 17)
  Assert(d == endof(data));
}

BOOLEAN NPCFirstDraw(SOLDIERTYPE *pSoldier, SOLDIERTYPE *pTargetSoldier) {
  // if attacking an NPC check to see who draws first!

  if (pTargetSoldier->ubProfile != NO_PROFILE && pTargetSoldier->ubProfile != SLAY &&
      pTargetSoldier->bNeutral && pTargetSoldier->bOppList[pSoldier->ubID] == SEEN_CURRENTLY &&
      (FindAIUsableObjClass(pTargetSoldier, IC_WEAPON) != NO_SLOT)) {
    uint8_t ubLargerHalf, ubSmallerHalf, ubTargetLargerHalf, ubTargetSmallerHalf;

    // roll the dice!
    // e.g. if level 5, roll Random( 3 + 1 ) + 2 for result from 2 to 5
    // if level 4, roll Random( 2 + 1 ) + 2 for result from 2 to 4
    ubSmallerHalf = EffectiveExpLevel(pSoldier) / 2;
    ubLargerHalf = EffectiveExpLevel(pSoldier) - ubSmallerHalf;

    ubTargetSmallerHalf = EffectiveExpLevel(pTargetSoldier) / 2;
    ubTargetLargerHalf = EffectiveExpLevel(pTargetSoldier) - ubTargetSmallerHalf;
    if (gMercProfiles[pTargetSoldier->ubProfile].bApproached &
        gbFirstApproachFlags[APPROACH_THREATEN - 1]) {
      // gains 1 to 2 points
      ubTargetSmallerHalf += 1;
      ubTargetLargerHalf += 1;
    }
    if (Random(ubTargetSmallerHalf + 1) + ubTargetLargerHalf >
        Random(ubSmallerHalf + 1) + ubLargerHalf) {
      return (TRUE);
    }
  }
  return (FALSE);
}
