// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "GameScreen.h"

#include "Editor/EditScreen.h"
#include "FadeScreen.h"
#include "GameLoop.h"
#include "GameState.h"
#include "HelpScreen.h"
#include "JAScreens.h"
#include "Local.h"
#include "Macro.h"
#include "SGP/ButtonSystem.h"
#include "SGP/CursorControl.h"
#include "SGP/Debug.h"
#include "SGP/Font.h"
#include "SGP/Input.h"
#include "SGP/VObjectBlitters.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "Screens.h"
#include "Strategic/Assignments.h"
#include "Strategic/GameClock.h"
#include "Strategic/GameInit.h"
#include "Strategic/Meanwhile.h"
#include "Strategic/PreBattleInterface.h"
#include "Strategic/StrategicAI.h"
#include "Strategic/StrategicMap.h"
#include "Strategic/StrategicMovement.h"
#include "Strategic/StrategicTurns.h"
#include "SysGlobals.h"
#include "Tactical/AirRaid.h"
#include "Tactical/AutoBandage.h"
#include "Tactical/Bullets.h"
#include "Tactical/DialogueControl.h"
#include "Tactical/Faces.h"
#include "Tactical/HandleUI.h"
#include "Tactical/Interface.h"
#include "Tactical/InterfaceControl.h"
#include "Tactical/InterfaceDialogue.h"
#include "Tactical/InterfacePanels.h"
#include "Tactical/MapInformation.h"
#include "Tactical/MercEntering.h"
#include "Tactical/Overhead.h"
#include "Tactical/SoldierCreate.h"
#include "Tactical/SoldierInitList.h"
#include "Tactical/SoldierMacros.h"
#include "Tactical/Squads.h"
#include "Tactical/StrategicExitGUI.h"
#include "TileEngine/Environment.h"
#include "TileEngine/OverheadMap.h"
#include "TileEngine/Physics.h"
#include "TileEngine/RadarScreen.h"
#include "TileEngine/RenderDirty.h"
#include "TileEngine/RenderWorld.h"
#include "TileEngine/SysUtil.h"
#include "TileEngine/TacticalPlacementGUI.h"
#include "TileEngine/WorldDef.h"
#include "Utils/Cursors.h"
#include "Utils/EventPump.h"
#include "Utils/FontControl.h"
#include "Utils/Message.h"
#include "Utils/SoundControl.h"
#include "Utils/Text.h"
#include "Utils/TimerControl.h"

#define ARE_IN_FADE_IN() (gfFadeIn || gfFadeInitialized)

BOOLEAN gfTacticalDoHeliRun = FALSE;

// VIDEO OVERLAYS
VIDEO_OVERLAY *g_fps_overlay = NULL;
VIDEO_OVERLAY *g_counter_period_overlay = NULL;

BOOLEAN gfGameScreenLocateToSoldier = FALSE;
BOOLEAN gfEnteringMapScreen = FALSE;
SOLDIERTYPE *gPreferredInitialSelectedGuy = NULL;

static BOOLEAN gfTacticalIsModal = FALSE;
static MOUSE_REGION gTacticalDisableRegion;
static BOOLEAN gfTacticalDisableRegionActive = FALSE;
MODAL_HOOK gModalDoneCallback;
BOOLEAN gfBeginEndTurn = FALSE;
extern BOOLEAN gfFirstHeliRun;
extern BOOLEAN gfRenderFullThisFrame;

// The InitializeGame function is responsible for setting up all data and Gaming
// Engine tasks which will run the game
static RENDER_HOOK gRenderOverride = 0;

static ScreenID guiTacticalLeaveScreenID =
    ERROR_SCREEN;  // XXX TODO001A had no explicit initialisation
static BOOLEAN guiTacticalLeaveScreen = FALSE;

static void BlitMFont(VIDEO_OVERLAY *const ovr) {
  SetFontAttributes(ovr->uiFontID, ovr->ubFontFore, DEFAULT_SHADOW, ovr->ubFontBack);
  SGPVSurface::Lock l(ovr->uiDestBuff);
  MPrintBuffer(l.Buffer<uint16_t>(), l.Pitch(), ovr->sX, ovr->sY, ovr->zText);
}

void MainGameScreenInit() {
  gpZBuffer = InitZBuffer(SCREEN_WIDTH, SCREEN_HEIGHT);
  InitializeBackgroundRects();

  // EnvSetTimeInHours(ENV_TIME_12);

  SetRenderFlags(RENDER_FLAG_FULL);

  // Init Video Overlays
  // FIRST, FRAMERATE
  g_fps_overlay = RegisterVideoOverlay(BlitMFont, 0, 0, SMALLFONT1, FONT_MCOLOR_DKGRAY,
                                       FONT_MCOLOR_BLACK, L"90");
  EnableVideoOverlay(false, g_fps_overlay);

  // SECOND, PERIOD COUNTER
  g_counter_period_overlay = RegisterVideoOverlay(BlitMFont, 30, 0, SMALLFONT1, FONT_MCOLOR_DKGRAY,
                                                  FONT_MCOLOR_BLACK, L"Levelnodes: 100000");
  EnableVideoOverlay(false, g_counter_period_overlay);
}

// The ShutdownGame function will free up/undo all things that were started in
// InitializeGame() It will also be responsible to making sure that all Gaming
// Engine tasks exit properly
void MainGameScreenShutdown() {
  ShutdownZBuffer(gpZBuffer);
  ShutdownBackgroundRects();
  RemoveVideoOverlay(g_fps_overlay);
  RemoveVideoOverlay(g_counter_period_overlay);
}

void FadeInGameScreen() {
  fFirstTimeInGameScreen = TRUE;

  FadeInNextFrame();
}

void FadeOutGameScreen() { FadeOutNextFrame(); }

void EnterTacticalScreen() {
  guiTacticalLeaveScreen = FALSE;

  SetPositionSndsActive();

  // Set pending screen
  SetPendingNewScreen(GAME_SCREEN);

  // Set as active...
  gTacticalStatus.uiFlags |= ACTIVE;

  fInterfacePanelDirty = DIRTYLEVEL2;

  // Disable all faces
  SetAllAutoFacesInactive();

  // CHECK IF OURGUY IS NOW OFF DUTY
  const SOLDIERTYPE *sel = GetSelectedMan();
  if (sel != NULL) {
    if (!OK_CONTROLLABLE_MERC(sel)) {
      SelectNextAvailSoldier(sel);
      sel = GetSelectedMan();
    }
    // ATE: If the current guy is sleeping, change....
    if (sel != NULL && sel->fMercAsleep) SelectNextAvailSoldier(sel);
  } else {
    // otherwise, make sure interface is team panel...
    SetCurrentInterfacePanel(TEAM_PANEL);
  }

  if (!gfTacticalPlacementGUIActive) gRadarRegion.Enable();
  gViewportRegion.Enable();

  // set default squad on sector entry
  // ATE: moved these 2 call after initalizing the interface!
  // SetDefaultSquadOnSectorEntry( FALSE );
  // ExamineCurrentSquadLights( );

  // Init interface ( ALWAYS TO TEAM PANEL.  DEF changed it to go back to the
  // previous panel )
  if (!gfTacticalPlacementGUIActive) {
    // make sure the gsCurInterfacePanel is valid
    if (gsCurInterfacePanel < 0 || gsCurInterfacePanel >= NUM_UI_PANELS)
      gsCurInterfacePanel = TEAM_PANEL;

    SetCurrentInterfacePanel(gsCurInterfacePanel);
  }

  // set default squad on sector entry
  SetDefaultSquadOnSectorEntry(FALSE);
  ExamineCurrentSquadLights();

  fFirstTimeInGameScreen = FALSE;

  // Make sure it gets re-created....
  DirtyTopMessage();

  // Set compression to normal!
  // SetGameTimeCompressionLevel( TIME_COMPRESS_X1 );

  // Select current guy...
  // gfGameScreenLocateToSoldier = TRUE;

  // Locate if in meanwhile...
  if (AreInMeanwhile()) {
    LocateToMeanwhileCharacter();
  }

  if (gTacticalStatus.uiFlags & IN_DEIDRANNA_ENDGAME) {
    InternalLocateGridNo(4561, TRUE);
  }

  // Clear tactical message q
  ClearTacticalMessageQueue();

  // ATE: Enable messages again...
  EnableScrollMessages();
}

void LeaveTacticalScreen(ScreenID const uiNewScreen) {
  guiTacticalLeaveScreenID = uiNewScreen;
  guiTacticalLeaveScreen = TRUE;
}

void InternalLeaveTacticalScreen(ScreenID const uiNewScreen) {
  gpCustomizableTimerCallback = NULL;

  // unload the sector they teleported out of
  if (!gfAutomaticallyStartAutoResolve) {
    CheckAndHandleUnloadingOfCurrentWorld();
  }

  SetPositionSndsInActive();

  // Turn off active flag
  gTacticalStatus.uiFlags &= (~ACTIVE);

  fFirstTimeInGameScreen = TRUE;

  SetPendingNewScreen(uiNewScreen);

  // Disable all faces
  SetAllAutoFacesInactive();

  ResetInterfaceAndUI();

  // Remove cursor and reset height....
  gsGlobalCursorYOffset = 0;
  SetCurrentCursorFromDatabase(VIDEO_NO_CURSOR);

  // Shutdown panel
  ShutdownCurrentPanel();

  // disable the radar map
  gRadarRegion.Disable();
  // gViewportRegion.Disable();

  // We are leaving... turn off pedning autobadage...
  SetAutoBandagePending(FALSE);

  // ATE: Disable messages....
  DisableScrollMessages();

  if (uiNewScreen == MAINMENU_SCREEN) {
    // We want to reinitialize the game
    ReStartingGame();
  }

  if (uiNewScreen != MAP_SCREEN) {
    StopAnyCurrentlyTalkingSpeech();
  }

  // If we have some disabled screens up.....remove...
  CheckForDisabledRegionRemove();

  // ATE: Record last time we were in tactical....
  gTacticalStatus.uiTimeSinceLastInTactical = GetWorldTotalMin();

  FinishAnySkullPanelAnimations();
}

static void HandleModalTactical();
static void TacticalScreenLocateToSoldier();

ScreenID MainGameScreenHandle() {
  // DO NOT MOVE THIS FUNCTION CALL!!!
  // This determines if the help screen should be active
  //	if( ( !gfTacticalDoHeliRun && !gfFirstHeliRun ) &&
  // ShouldTheHelpScreenComeUp( HELP_SCREEN_TACTICAL, FALSE ) )
  if (!gfPreBattleInterfaceActive && ShouldTheHelpScreenComeUp(HELP_SCREEN_TACTICAL, FALSE)) {
    // handle the help screen
    HelpScreenHandler();
    return (GAME_SCREEN);
  }

  if (HandleAutoBandage()) {
#ifndef VISIBLE_AUTO_BANDAGE
    return (GAME_SCREEN);
#endif
  }

  if (gfBeginEndTurn) {
    UIHandleEndTurn(NULL);
    gfBeginEndTurn = FALSE;
  }

  if (gfTacticalIsModal) {
    if (gfTacticalIsModal == 1) {
      gfTacticalIsModal++;
    } else {
      HandleModalTactical();

      return (GAME_SCREEN);
    }
  }

  // OK, this is the pause system for when we see a guy...
  if (!ARE_IN_FADE_IN()) {
    if (gTacticalStatus.fEnemySightingOnTheirTurn) {
      if ((GetJA2Clock() - gTacticalStatus.uiTimeSinceDemoOn) > 3000) {
        SOLDIERTYPE *const s = gTacticalStatus.enemy_sighting_on_their_turn_enemy;
        if (gTacticalStatus.ubCurrentTeam != OUR_TEAM) {
          AdjustNoAPToFinishMove(s, FALSE);
        }
        s->fPauseAllAnimation = FALSE;

        gTacticalStatus.fEnemySightingOnTheirTurn = FALSE;
      }
    }
  }

  // see if the helicopter is coming in this time for the initial entrance by
  // the mercs
  InitHelicopterEntranceByMercs();

  // Handle Environment controller here
  EnvironmentController(TRUE);

  if (!ARE_IN_FADE_IN()) {
    HandleWaitTimerForNPCTrigger();

    // Check timer that could have been set to do anything
    CheckCustomizableTimer();

    // HAndle physics engine
    SimulateWorld();

    // Handle strategic engine
    HandleStrategicTurn();
  }

  if (gfTacticalDoHeliRun) {
    gfGameScreenLocateToSoldier = FALSE;
    InternalLocateGridNo(gMapInformation.sNorthGridNo, TRUE);

    // Start heli Run...
    StartHelicopterRun(gMapInformation.sNorthGridNo);

    // Update clock by one so that our DidGameJustStatrt() returns now false for
    // things like LAPTOP, etc...
    SetGameTimeCompressionLevel(TIME_COMPRESS_X1);
    // UpdateClock( 1 );

    gfTacticalDoHeliRun = FALSE;
    // SetMusicMode( MUSIC_TACTICAL_NOTHING );
  }

  if (InOverheadMap()) {
    HandleOverheadMap();
    return (GAME_SCREEN);
  }

  if (!ARE_IN_FADE_IN()) {
    HandleAirRaid();
  }

  if (gfGameScreenLocateToSoldier) {
    TacticalScreenLocateToSoldier();
    gfGameScreenLocateToSoldier = FALSE;
  }

  if (fFirstTimeInGameScreen) {
    EnterTacticalScreen();

    // Select a guy if he hasn;'
    if (!gfTacticalPlacementGUIActive) {
      SOLDIERTYPE *const sel = GetSelectedMan();
      if (sel != NULL && OK_INTERRUPT_MERC(sel)) {
        SelectSoldier(sel, SELSOLDIER_FORCE_RESELECT);
      }
    }
  }

  if (HandleFadeOutCallback()) {
    return (GAME_SCREEN);
  }

  if (guiCurrentScreen != MSG_BOX_SCREEN) {
    if (HandleBeginFadeOut(GAME_SCREEN)) {
      return (GAME_SCREEN);
    }
  }

  HandleHeliDrop();

  if (!ARE_IN_FADE_IN()) {
    HandleAutoBandagePending();
  }

  // ATE: CHRIS_C LOOK HERE FOR GETTING AI CONSTANTLY GOING
  // if ( gTacticalStatus.uiFlags & TURNBASED )
  //{
  //	if ( !(gTacticalStatus.uiFlags & ENEMYS_TURN) )
  //	{
  //		EndTurn( );
  //	}
  //}

  if (!ARE_IN_FADE_IN()) {
    UpdateBullets();

    // Execute Tactical Overhead
    ExecuteOverhead();
  }

  // Handle animated cursors
  if (gfWorldLoaded) {
    HandleAnimatedCursors();

    // Handle Interface
    ScreenID const uiNewScreen = HandleTacticalUI();

    HandleTalkingAutoFaces();

    if (uiNewScreen != GAME_SCREEN) return uiNewScreen;
  } else if (gfIntendOnEnteringEditor && GameState::getInstance()->isEditorMode()) {
    DebugMsg(TOPIC_JA2EDITOR, DBG_LEVEL_1, "Aborting normal game mode and entering editor mode...");
    SetPendingNewScreen(NO_PENDING_SCREEN);
    return EDIT_SCREEN;
  } else if (!gfEnteringMapScreen) {
    gfEnteringMapScreen = TRUE;
  }

  // Deque all game events
  if (!ARE_IN_FADE_IN()) {
    DequeAllGameEvents();
  }

  HandleTopMessages();

  if (!gfScrollPending && !g_scroll_inertia) {
    // Handle Interface Stuff
    SetUpInterface();
    HandleTacticalPanelSwitch();
  }

  // Handle Scroll Of World
  ScrollWorld();

  // SetRenderFlags( RENDER_FLAG_FULL );

  RenderWorld();

  if (gRenderOverride != NULL) {
    gRenderOverride();
  }

  if (gfScrollPending || g_scroll_inertia) {
    RenderTacticalInterfaceWhileScrolling();
  } else {
    // Handle Interface Stuff
    // RenderTacticalInterface( );
  }

  // Render Interface
  RenderTopmostTacticalInterface();

  // Render view window
  RenderRadarScreen();

  ResetInterface();

  if (gfScrollPending) {
    AllocateVideoOverlaysArea();
    SaveVideoOverlaysArea(FRAME_BUFFER);
    ExecuteVideoOverlays();
  } else {
    ExecuteVideoOverlays();
  }

  // Adding/deleting of video overlays needs to be done below
  // ExecuteVideoOverlays( )....

  // Handle dialogue queue system
  if (!ARE_IN_FADE_IN()) {
    HandleDialogue();
  }

  // Don't render if we have a scroll pending!
  if (!gfScrollPending && !g_scroll_inertia && !gfRenderFullThisFrame) {
    RenderButtonsFastHelp();
  }

  // Display Framerate
  DisplayFrameRate();

  CheckForMeanwhileOKStart();

  ScrollString();

  ExecuteBaseDirtyRectQueue();

  // KillBackgroundRects( );

  /////////////////////////////////////////////////////
  EndFrameBufferRender();

  if (HandleFadeInCallback()) {
    // Re-render the scene!
    SetRenderFlags(RENDER_FLAG_FULL);
    fInterfacePanelDirty = DIRTYLEVEL2;
  }

  if (HandleBeginFadeIn(GAME_SCREEN)) {
    guiTacticalLeaveScreenID = FADE_SCREEN;
  }

  if (guiTacticalLeaveScreen) {
    guiTacticalLeaveScreen = FALSE;

    InternalLeaveTacticalScreen(guiTacticalLeaveScreenID);
  }

  // Check if we are to enter map screen
  if (gfEnteringMapScreen == 2) {
    gfEnteringMapScreen = FALSE;
    LeaveTacticalScreen(MAP_SCREEN);
  }

  // Are we entering map screen? if so, wait a frame!
  if (gfEnteringMapScreen > 0) {
    gfEnteringMapScreen++;
  }

  return (GAME_SCREEN);
}

void SetRenderHook(RENDER_HOOK pRenderOverride) { gRenderOverride = pRenderOverride; }

void EnableFPSOverlay(BOOLEAN fEnable) {
  EnableVideoOverlay(fEnable, g_fps_overlay);
  EnableVideoOverlay(fEnable, g_counter_period_overlay);
}

static void TacticalScreenLocateToSoldier() {
  SOLDIERTYPE *const prefer = gPreferredInitialSelectedGuy;
  if (prefer != NULL) {
    gPreferredInitialSelectedGuy = NULL;
    if (OK_CONTROLLABLE_MERC(prefer) && OK_INTERRUPT_MERC(prefer)) {
      LocateSoldier(prefer, 10);
      SelectSoldier(prefer, SELSOLDIER_FORCE_RESELECT);
      return;
    }
  }

  // Set locator to first merc
  FOR_EACH_IN_TEAM(s, OUR_TEAM) {
    if (OkControllableMerc(s) && OK_INTERRUPT_MERC(s)) {
      LocateSoldier(s, 10);
      SelectSoldier(s, SELSOLDIER_FORCE_RESELECT);
      break;
    }
  }
}

void UpdateTeamPanelAssignments() {
  // Remove all players
  RemoveAllPlayersFromSlot();

  FOR_EACH_IN_TEAM(s, OUR_TEAM) {
    // Setup team interface
    CheckForAndAddMercToTeamPanel(s);
  }
}

void EnterModalTactical(int8_t bMode) {
  gfTacticalIsModal = TRUE;

  if (bMode == TACTICAL_MODAL_NOMOUSE) {
    if (!gfTacticalDisableRegionActive) {
      gfTacticalDisableRegionActive = TRUE;

      MSYS_DefineRegion(&gTacticalDisableRegion, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                        MSYS_PRIORITY_HIGH, VIDEO_NO_CURSOR, MSYS_NO_CALLBACK, MSYS_NO_CALLBACK);
    }
  }

  UpdateSaveBuffer();
}

void EndModalTactical() {
  if (gfTacticalDisableRegionActive) {
    MSYS_RemoveRegion(&gTacticalDisableRegion);

    gfTacticalDisableRegionActive = FALSE;
  }

  if (gModalDoneCallback != NULL) {
    gModalDoneCallback();

    gModalDoneCallback = NULL;
  }

  gfTacticalIsModal = FALSE;

  SetRenderFlags(RENDER_FLAG_FULL);
}

static void HandleModalTactical() {
  RestoreBackgroundRects();

  RenderWorld();
  RenderRadarScreen();
  ExecuteVideoOverlays();

  // Handle dialogue queue system
  HandleDialogue();

  HandleTalkingAutoFaces();

  // Handle faces
  HandleAutoFaces();

  if (gfInSectorExitMenu) {
    RenderSectorExitMenu();
  }
  RenderButtons();

  SaveBackgroundRects();
  RenderButtonsFastHelp();
  RenderPausedGameBox();

  ExecuteBaseDirtyRectQueue();
  EndFrameBufferRender();
}

void InitHelicopterEntranceByMercs() {
  if (DidGameJustStart()) {
    AIR_RAID_DEFINITION AirRaidDef;

    // Update clock ahead from STARTING_TIME to make mercs arrive!
    WarpGameTime(FIRST_ARRIVAL_DELAY, WARPTIME_PROCESS_EVENTS_NORMALLY);

    AirRaidDef.sSectorX = 9;
    AirRaidDef.sSectorY = 1;
    AirRaidDef.sSectorZ = 0;
    AirRaidDef.bIntensity = 2;
    AirRaidDef.uiFlags = AIR_RAID_BEGINNING_GAME;
    AirRaidDef.ubNumMinsFromCurrentTime = 1;

    //	ScheduleAirRaid( &AirRaidDef );

    gfTacticalDoHeliRun = TRUE;
    gfFirstHeliRun = TRUE;

    gTacticalStatus.fDidGameJustStart = FALSE;
  }
}
