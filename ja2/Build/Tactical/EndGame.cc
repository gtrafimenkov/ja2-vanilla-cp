#include "Tactical/EndGame.h"

#include "FadeScreen.h"
#include "Intro.h"
#include "Macro.h"
#include "SGP/Random.h"
#include "ScreenIDs.h"
#include "Strategic/PlayerCommand.h"
#include "Strategic/PreBattleInterface.h"
#include "Strategic/QueenCommand.h"
#include "Strategic/Quests.h"
#include "Strategic/Strategic.h"
#include "Strategic/StrategicAI.h"
#include "Strategic/StrategicMap.h"
#include "Strategic/StrategicTownLoyalty.h"
#include "Tactical/Boxing.h"
#include "Tactical/DialogueControl.h"
#include "Tactical/HandleUI.h"
#include "Tactical/LOS.h"
#include "Tactical/Morale.h"
#include "Tactical/OppList.h"
#include "Tactical/Overhead.h"
#include "Tactical/Points.h"
#include "Tactical/QArray.h"
#include "Tactical/SoldierControl.h"
#include "Tactical/SoldierMacros.h"
#include "Tactical/SoldierProfile.h"
#include "Tactical/Squads.h"
#include "Tactical/TacticalSave.h"
#include "TacticalAI/AI.h"
#include "TacticalAI/NPC.h"
#include "TileEngine/ExitGrids.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/RenderFun.h"
#include "TileEngine/RenderWorld.h"
#include "TileEngine/SaveLoadMap.h"
#include "TileEngine/TileDef.h"
#include "TileEngine/WorldMan.h"
#include "Utils/MusicControl.h"
#include "Utils/SoundControl.h"
#include "Utils/TimerControl.h"

int16_t sStatueGridNos[] = {13829, 13830, 13669, 13670};

SOLDIERTYPE *gpKillerSoldier = NULL;
int16_t gsGridNo;
int8_t gbLevel;

// This function checks if our statue exists in the current sector at given
// gridno
BOOLEAN DoesO3SectorStatueExistHere(int16_t sGridNo) {
  int32_t cnt;
  EXITGRID ExitGrid;

  // First check current sector......
  if (gWorldSectorX == 3 && gWorldSectorY == MAP_ROW_O && gbWorldSectorZ == 0) {
    // Check for exitence of and exit grid here...
    // ( if it doesn't then the change has already taken place )
    if (!GetExitGrid(13669, &ExitGrid)) {
      for (cnt = 0; cnt < 4; cnt++) {
        if (sStatueGridNos[cnt] == sGridNo) {
          return (TRUE);
        }
      }
    }
  }

  return (FALSE);
}

// This function changes the graphic of the statue and adds the exit grid...
void ChangeO3SectorStatue(BOOLEAN fFromExplosion) {
  EXITGRID ExitGrid;
  uint16_t usTileIndex;

  // Remove old graphic
  {
    ApplyMapChangesToMapTempFile app;
    // Remove it!
    // Get index for it...
    usTileIndex = GetTileIndexFromTypeSubIndex(EIGHTOSTRUCT, 5);
    RemoveStruct(13830, usTileIndex);

    // Add new one...
    if (fFromExplosion) {
      // Use damaged peice
      usTileIndex = GetTileIndexFromTypeSubIndex(EIGHTOSTRUCT, 7);
    } else {
      usTileIndex = GetTileIndexFromTypeSubIndex(EIGHTOSTRUCT, 8);
      // Play sound...

      PlayJA2Sample(OPEN_STATUE, HIGHVOLUME, 1, MIDDLEPAN);
    }
    AddStructToHead(13830, usTileIndex);

    // Add exit grid
    ExitGrid.ubGotoSectorX = 3;
    ExitGrid.ubGotoSectorY = MAP_ROW_O;
    ExitGrid.ubGotoSectorZ = 1;
    ExitGrid.usGridNo = 13037;

    AddExitGridToWorld(13669, &ExitGrid);
    gpWorldLevelData[13669].uiFlags |= MAPELEMENT_REVEALED;
  }

  // Re-render the world!
  gTacticalStatus.uiFlags |= NOHIDE_REDUNDENCY;
  // FOR THE NEXT RENDER LOOP, RE-EVALUATE REDUNDENT TILES
  InvalidateWorldRedundency();
  SetRenderFlags(RENDER_FLAG_FULL);

  RecompileLocalMovementCostsFromRadius(13830, 5);
}

static void HandleDeidrannaDeath(SOLDIERTYPE *pKillerSoldier, int16_t sGridNo, int8_t bLevel);

static void DeidrannaTimerCallback() { HandleDeidrannaDeath(gpKillerSoldier, gsGridNo, gbLevel); }

void BeginHandleDeidrannaDeath(SOLDIERTYPE *pKillerSoldier, int16_t sGridNo, int8_t bLevel) {
  gpKillerSoldier = pKillerSoldier;
  gsGridNo = sGridNo;
  gbLevel = bLevel;

  // Lock the UI.....
  gTacticalStatus.uiFlags |= ENGAGED_IN_CONV;
  // Increment refrence count...
  giNPCReferenceCount = 1;

  gTacticalStatus.uiFlags |= IN_DEIDRANNA_ENDGAME;

  SetCustomizableTimerCallbackAndDelay(2000, DeidrannaTimerCallback, FALSE);
}

static void DoneFadeOutKilledQueen();

static void HandleDeidrannaDeath(SOLDIERTYPE *const pKillerSoldier, const int16_t sGridNo,
                                 const int8_t bLevel) {
  // Start victory music here...
  SetMusicMode(MUSIC_TACTICAL_VICTORY);

  if (pKillerSoldier) {
    TacticalCharacterDialogue(pKillerSoldier, QUOTE_KILLING_DEIDRANNA);
  }

  // STEP 1 ) START ALL QUOTES GOING!
  // OK - loop through all witnesses and see if they want to say something abou
  // this...
  FOR_EACH_IN_TEAM(s, OUR_TEAM) {
    if (s != pKillerSoldier && OkControllableMerc(s) && !(s->uiStatusFlags & SOLDIER_GASSED) &&
        !AM_AN_EPC(s) && QuoteExp_WitnessDeidrannaDeath[s->ubProfile]) {
      // Can we see location?
      const int16_t sDistVisible =
          DistanceVisible(s, DIRECTION_IRRELEVANT, DIRECTION_IRRELEVANT, sGridNo, bLevel);
      if (SoldierTo3DLocationLineOfSightTest(s, sGridNo, bLevel, 3, sDistVisible, TRUE)) {
        TacticalCharacterDialogue(s, QUOTE_KILLING_DEIDRANNA);
      }
    }
  }

  // Set fact that she is dead!
  SetFactTrue(FACT_QUEEN_DEAD);

  ExecuteStrategicAIAction(STRATEGIC_AI_ACTION_QUEEN_DEAD, 0, 0);

  class DialogueEventDoneKillingDeidranna : public DialogueEvent {
   public:
    bool Execute() {  // Called after all player quotes are done
      gFadeOutDoneCallback = DoneFadeOutKilledQueen;
      FadeOutGameScreen();
      return false;
    }
  };

  // AFTER LAST ONE IS DONE - PUT SPECIAL EVENT ON QUEUE TO BEGIN FADE< ETC
  DialogueEvent::Add(new DialogueEventDoneKillingDeidranna());
}

static void DoneFadeInKilledQueen() {
  // Run NPC script
  const SOLDIERTYPE *const pNPCSoldier = FindSoldierByProfileID(DEREK);
  if (!pNPCSoldier) {
    return;
  }

  TriggerNPCRecordImmediately(pNPCSoldier->ubProfile, 6);
}

static void DoneFadeOutKilledQueen() {
  // Move current squad over
  FOR_EACH_IN_TEAM(i, OUR_TEAM) {
    SOLDIERTYPE &s = *i;
    // Are we in this sector, on the current squad?
    if (s.bLife < OKLIFE) continue;
    if (!s.bInSector) continue;
    if (s.bAssignment != CurrentSquad()) continue;

    gfTacticalTraversal = TRUE;
    SetGroupSectorValue(3, MAP_ROW_P, 0, *GetGroup(s.ubGroupID));

    // XXX redundant, SetGroupSectorValue() handles this
    s.sSectorX = 3;
    s.sSectorY = MAP_ROW_P;
    s.bSectorZ = 0;
    // Set gridno
    s.ubStrategicInsertionCode = INSERTION_CODE_GRIDNO;
    s.usStrategicInsertionData = 5687;
    // Set direction to face
    s.ubInsertionDirection = 100 + NORTHWEST;
  }

  // Kill all enemies in world
  CFOR_EACH_IN_TEAM(i, ENEMY_TEAM) {
    SOLDIERTYPE const &s = *i;
    // For sure for flag thet they are dead is not set
    // Check for any more badguys
    // ON THE STRAGETY LAYER KILL BAD GUYS!
    if (s.bNeutral) continue;
    if (s.bSide == OUR_TEAM) continue;
    ProcessQueenCmdImplicationsOfDeath(&s);
  }

  // 'End' battle
  ExitCombatMode();
  gTacticalStatus.fLastBattleWon = TRUE;
  // Set enemy presence to false
  gTacticalStatus.fEnemyInSector = FALSE;

  SetMusicMode(MUSIC_TACTICAL_VICTORY);

  HandleMoraleEvent(0, MORALE_QUEEN_BATTLE_WON, 3, MAP_ROW_P, 0);
  HandleGlobalLoyaltyEvent(GLOBAL_LOYALTY_QUEEN_BATTLE_WON, 3, MAP_ROW_P, 0);

  SetMusicMode(MUSIC_TACTICAL_VICTORY);

  SetThisSectorAsPlayerControlled(gWorldSectorX, gWorldSectorY, gbWorldSectorZ, TRUE);

  // ATE: Force change of level set z to 1
  gbWorldSectorZ = 1;

  // Clear out dudes
  SECTORINFO &sector = SectorInfo[SEC_P3];
  sector.ubNumAdmins = 0;
  sector.ubNumTroops = 0;
  sector.ubNumElites = 0;
  sector.ubAdminsInBattle = 0;
  sector.ubTroopsInBattle = 0;
  sector.ubElitesInBattle = 0;

  // ATE: Get rid of Elliot in P3
  GetProfile(ELLIOT).sSectorX = 1;

  ChangeNpcToDifferentSector(GetProfile(DEREK), 3, MAP_ROW_P, 0);
  ChangeNpcToDifferentSector(GetProfile(OLIVER), 3, MAP_ROW_P, 0);

  SetCurrentWorldSector(3, MAP_ROW_P, 0);

  gfTacticalTraversal = FALSE;
  gpTacticalTraversalGroup = 0;
  gpTacticalTraversalChosenSoldier = 0;

  gFadeInDoneCallback = DoneFadeInKilledQueen;
  FadeInGameScreen();
}

static void DoneFadeOutEndCinematic();

void EndQueenDeathEndgameBeginEndCimenatic() {
  // Start end cimimatic....
  gTacticalStatus.uiFlags |= IN_ENDGAME_SEQUENCE;

  // first thing is to loop through team and say end quote...
  FOR_EACH_IN_TEAM(s, OUR_TEAM) {
    if (s->bLife >= OKLIFE && !AM_AN_EPC(s)) {
      TacticalCharacterDialogue(s, QUOTE_END_GAME_COMMENT);
    }
  }

  class DialogueEventTeamMembersDoneTalking : public DialogueEvent {
   public:
    bool Execute() {  // End death UI - fade to smaker
      EndQueenDeathEndgame();
      gFadeOutDoneCallback = DoneFadeOutEndCinematic;
      FadeOutGameScreen();
      return false;
    }
  };

  // Add queue event to proceed w/ smacker cimimatic
  DialogueEvent::Add(new DialogueEventTeamMembersDoneTalking());
}

void EndQueenDeathEndgame() {
  // Unset flags...
  gTacticalStatus.uiFlags &= (~ENGAGED_IN_CONV);
  // Increment refrence count...
  giNPCReferenceCount = 0;

  gTacticalStatus.uiFlags &= (~IN_DEIDRANNA_ENDGAME);
}

static void DoneFadeOutEndCinematic() {
  // DAVE PUT SMAKER STUFF HERE!!!!!!!!!!!!
  // :)
  gTacticalStatus.uiFlags &= (~IN_ENDGAME_SEQUENCE);

  // For now, just quit the freaken game...
  //	InternalLeaveTacticalScreen( MAINMENU_SCREEN );

  InternalLeaveTacticalScreen(INTRO_SCREEN);
  //	guiCurrentScreen = INTRO_SCREEN;

  SetIntroType(INTRO_ENDING);
}

static void HandleQueenBitchDeath(SOLDIERTYPE *pKillerSoldier, int16_t sGridNo, int8_t bLevel);

static void QueenBitchTimerCallback() { HandleQueenBitchDeath(gpKillerSoldier, gsGridNo, gbLevel); }

void BeginHandleQueenBitchDeath(SOLDIERTYPE *pKillerSoldier, int16_t sGridNo, int8_t bLevel) {
  gpKillerSoldier = pKillerSoldier;
  gsGridNo = sGridNo;
  gbLevel = bLevel;

  // Lock the UI.....
  gTacticalStatus.uiFlags |= ENGAGED_IN_CONV;
  // Increment refrence count...
  giNPCReferenceCount = 1;

  // gTacticalStatus.uiFlags |= IN_DEIDRANNA_ENDGAME;

  SetCustomizableTimerCallbackAndDelay(3000, QueenBitchTimerCallback, FALSE);

  // Kill all enemies in creature team.....
  FOR_EACH_IN_TEAM(s, CREATURE_TEAM) {
    // Are we ALIVE.....
    if (s->bLife > 0) {
      // For sure for flag thet they are dead is not set
      // Check for any more badguys
      // ON THE STRAGETY LAYER KILL BAD GUYS!

      // HELLO!  THESE ARE CREATURES!  THEY CAN'T BE NEUTRAL!
      // if (!s->bNeutral && s->bSide != OUR_TEAM)
      {
        gTacticalStatus.ubAttackBusyCount++;
        EVENT_SoldierGotHit(s, 0, 10000, 0, s->bDirection, 320, NULL, FIRE_WEAPON_NO_SPECIAL,
                            s->bAimShotLocation, NOWHERE);
      }
    }
  }
}

static void HandleQueenBitchDeath(SOLDIERTYPE *const pKillerSoldier, const int16_t sGridNo,
                                  const int8_t bLevel) {
  // Start victory music here...
  SetMusicMode(MUSIC_TACTICAL_VICTORY);

  if (pKillerSoldier) {
    TacticalCharacterDialogue(pKillerSoldier, QUOTE_KILLING_QUEEN);
  }

  // STEP 1 ) START ALL QUOTES GOING!
  // OK - loop through all witnesses and see if they want to say something abou
  // this...
  FOR_EACH_IN_TEAM(s, OUR_TEAM) {
    if (s != pKillerSoldier && OkControllableMerc(s) && !(s->uiStatusFlags & SOLDIER_GASSED) &&
        !AM_AN_EPC(s) && QuoteExp_WitnessQueenBugDeath[s->ubProfile]) {
      // Can we see location?
      const int16_t sDistVisible =
          DistanceVisible(s, DIRECTION_IRRELEVANT, DIRECTION_IRRELEVANT, sGridNo, bLevel);
      if (SoldierTo3DLocationLineOfSightTest(s, sGridNo, bLevel, 3, sDistVisible, TRUE)) {
        TacticalCharacterDialogue(s, QUOTE_KILLING_QUEEN);
      }
    }
  }

  // Set fact that she is dead!
  if (CheckFact(FACT_QUEEN_DEAD, 0)) {
    EndQueenDeathEndgameBeginEndCimenatic();
  } else {
    // Unset flags...
    gTacticalStatus.uiFlags &= (~ENGAGED_IN_CONV);
    // Increment refrence count...
    giNPCReferenceCount = 0;
  }
}
