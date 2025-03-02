// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Strategic/StrategicMap.h"

#include <algorithm>
#include <stdexcept>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "Directories.h"
#include "FadeScreen.h"
#include "GameLoop.h"
#include "GameScreen.h"
#include "GameSettings.h"
#include "JAScreens.h"
#include "Laptop/History.h"
#include "LoadingScreen.h"
#include "Local.h"
#include "Macro.h"
#include "MessageBoxScreen.h"
#include "SGP/CursorControl.h"
#include "SGP/Debug.h"
#include "SGP/FileMan.h"
#include "SGP/Font.h"
#include "SGP/HImage.h"
#include "SGP/Random.h"
#include "SGP/Timer.h"
#include "SGP/Types.h"
#include "SGP/VObject.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "ScreenIDs.h"
#include "Strategic/Assignments.h"
#include "Strategic/AutoResolve.h"
#include "Strategic/CampaignTypes.h"
#include "Strategic/CreatureSpreading.h"
#include "Strategic/GameClock.h"
#include "Strategic/GameEvents.h"
#include "Strategic/LoadSaveSectorInfo.h"
#include "Strategic/LoadSaveStrategicMapElement.h"
#include "Strategic/MapScreen.h"
#include "Strategic/MapScreenHelicopter.h"
#include "Strategic/Meanwhile.h"
#include "Strategic/MercContract.h"
#include "Strategic/PlayerCommand.h"
#include "Strategic/PreBattleInterface.h"
#include "Strategic/QueenCommand.h"
#include "Strategic/Quests.h"
#include "Strategic/Scheduling.h"
#include "Strategic/Strategic.h"
#include "Strategic/StrategicEventHandler.h"
#include "Strategic/StrategicMines.h"
#include "Strategic/StrategicMovement.h"
#include "Strategic/StrategicPathing.h"
#include "Strategic/StrategicTownLoyalty.h"
#include "Strategic/StrategicTurns.h"
#include "Strategic/TownMilitia.h"
#include "SysGlobals.h"
#include "Tactical/AnimationControl.h"
#include "Tactical/Boxing.h"
#include "Tactical/Bullets.h"
#include "Tactical/DialogueControl.h"
#include "Tactical/EnemySoldierSave.h"
#include "Tactical/Faces.h"
#include "Tactical/HandleUI.h"
#include "Tactical/Interface.h"
#include "Tactical/InterfaceDialogue.h"
#include "Tactical/InterfacePanels.h"
#include "Tactical/Items.h"
#include "Tactical/Keys.h"
#include "Tactical/MapInformation.h"
#include "Tactical/MercEntering.h"
#include "Tactical/MercHiring.h"
#include "Tactical/MilitiaControl.h"
#include "Tactical/OppList.h"
#include "Tactical/Overhead.h"
#include "Tactical/PathAI.h"
#include "Tactical/Points.h"
#include "Tactical/SoldierAdd.h"
#include "Tactical/SoldierControl.h"
#include "Tactical/SoldierCreate.h"
#include "Tactical/SoldierInitList.h"
#include "Tactical/SoldierMacros.h"
#include "Tactical/Squads.h"
#include "Tactical/TacticalSave.h"
#include "Tactical/TacticalTurns.h"
#include "TacticalAI/AI.h"
#include "TacticalAI/NPC.h"
#include "TileEngine/AmbientControl.h"
#include "TileEngine/Environment.h"
#include "TileEngine/ExitGrids.h"
#include "TileEngine/ExplosionControl.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/MapEdgepoints.h"
#include "TileEngine/Physics.h"
#include "TileEngine/RadarScreen.h"
#include "TileEngine/RenderDirty.h"
#include "TileEngine/RenderWorld.h"
#include "TileEngine/SaveLoadMap.h"
#include "TileEngine/SysUtil.h"
#include "TileEngine/TacticalPlacementGUI.h"
#include "TileEngine/WorldDat.h"
#include "TileEngine/WorldDef.h"
#include "TileEngine/WorldMan.h"
#include "Utils/AnimatedProgressBar.h"
#include "Utils/Cursors.h"
#include "Utils/EventPump.h"
#include "Utils/FontControl.h"
#include "Utils/Message.h"
#include "Utils/MusicControl.h"
#include "Utils/SoundControl.h"
#include "Utils/Text.h"
#include "Utils/TimerControl.h"

// Used by PickGridNoToWalkIn
#define MAX_ATTEMPTS 200

#define QUEST_CHECK_EVENT_TIME (8 * 60)
#define BOBBYRAY_UPDATE_TIME (9 * 60)
#define INSURANCE_UPDATE_TIME 0
#define EARLY_MORNING_TIME (4 * 60)
#define ENRICO_MAIL_TIME (7 * 60)

extern int16_t gsRobotGridNo;

BOOLEAN gfGettingNameFromSaveLoadScreen;

int16_t gWorldSectorX = 0;
int16_t gWorldSectorY = 0;
int8_t gbWorldSectorZ = -1;

static int16_t gsAdjacentSectorX;
static int16_t gsAdjacentSectorY;
static int8_t gbAdjacentSectorZ;
static GROUP *gpAdjacentGroup = 0;
static uint8_t gubAdjacentJumpCode;
static uint32_t guiAdjacentTraverseTime;
uint8_t gubTacticalDirection;
static int16_t gsAdditionalData;

static BOOLEAN fUsingEdgePointsForStrategicEntry = FALSE;
BOOLEAN gfInvalidTraversal = FALSE;
BOOLEAN gfLoneEPCAttemptingTraversal = FALSE;
BOOLEAN gfRobotWithoutControllerAttemptingTraversal = FALSE;
BOOLEAN gubLoneMercAttemptingToAbandonEPCs = 0;
const SOLDIERTYPE *gPotentiallyAbandonedEPC = NULL;

int8_t gbGreenToElitePromotions = 0;
int8_t gbGreenToRegPromotions = 0;
int8_t gbRegToElitePromotions = 0;
int8_t gbMilitiaPromotions = 0;

BOOLEAN gfUseAlternateMap = FALSE;
// whether or not we have found Orta yet
BOOLEAN fFoundOrta = FALSE;

// have any of the sam sites been found
BOOLEAN fSamSiteFound[NUMBER_OF_SAMS] = {
    FALSE,
    FALSE,
    FALSE,
    FALSE,
};

int16_t const pSamList[] = {SEC_D2, SEC_D15, SEC_I8, SEC_N4};

int16_t pSamGridNoAList[NUMBER_OF_SAMS] = {
    10196,
    11295,
    16080,
    11913,
};

int16_t pSamGridNoBList[NUMBER_OF_SAMS] = {
    10195,
    11135,
    15920,
    11912,
};

// ATE: Update this w/ graphic used
// Use 3 if / orientation, 4 if \ orientation
int8_t gbSAMGraphicList[NUMBER_OF_SAMS] = {4, 3, 3, 4};

uint8_t ubSAMControlledSectors[MAP_WORLD_Y][MAP_WORLD_X] = {
    //       1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},

    {0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 2, 2, 2, 2, 2, 2, 0},    // A
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 0},    // B
    {0, 1, 1, 1, 1, 1, 1, 1, 3, 2, 2, 2, 2, 2, 2, 2, 2, 0},    // C
    {0, 1, 01, 1, 1, 1, 1, 1, 3, 3, 2, 2, 2, 2, 2, 02, 2, 0},  // D
    {0, 1, 1, 1, 1, 1, 1, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 0},    // E
    {0, 1, 1, 1, 1, 1, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 0},    // F
    {0, 1, 1, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 0},    // G
    {0, 1, 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 0},    // H
    {0, 1, 1, 3, 3, 3, 3, 3, 03, 3, 3, 3, 3, 3, 2, 2, 2, 0},   // I
    {0, 1, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 0},    // J
    {0, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 0},    // K
    {0, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 0},    // L
    {0, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 2, 2, 2, 0},    // M
    {0, 4, 4, 4, 04, 4, 4, 4, 4, 4, 4, 4, 4, 4, 2, 2, 2, 0},   // N
    {0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 2, 2, 0},    // O
    {0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 2, 0},    // P

    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

static int16_t const DirXIncrementer[8] = {
    0,   // N
    1,   // NE
    1,   // E
    1,   // SE
    0,   // S
    -1,  // SW
    -1,  // W
    -1   // NW
};

static int16_t const DirYIncrementer[8] = {
    -1,  // N
    -1,  // NE
    0,   // E
    1,   // SE
    1,   // S
    1,   // SW
    0,   // W
    -1   // NW
};

extern BOOLEAN gfOverrideSector;

StrategicMapElement StrategicMap[MAP_WORLD_X * MAP_WORLD_Y];

static uint32_t UndergroundTacticalTraversalTime(
    int8_t const exit_direction) { /* We are attempting to traverse in an underground
                                    * environment. We need to use a complete different
                                    * method.  When underground, all sectors are
                                    * instantly adjacent. */
  GridNo gridno;
  switch (exit_direction) {
    case NORTH_STRATEGIC_MOVE:
      gridno = gMapInformation.sNorthGridNo;
      break;
    case EAST_STRATEGIC_MOVE:
      gridno = gMapInformation.sEastGridNo;
      break;
    case SOUTH_STRATEGIC_MOVE:
      gridno = gMapInformation.sSouthGridNo;
      break;
    case WEST_STRATEGIC_MOVE:
      gridno = gMapInformation.sWestGridNo;
      break;
    default:
      throw std::logic_error("invalid exit direction");
  }
  return gridno != -1 ? 0 : TRAVERSE_TIME_IMPOSSIBLE;
}

void BeginLoadScreen() {
  uint32_t uiStartTime, uiCurrTime;
  int32_t iPercentage, iFactor;
  uint32_t uiTimeRange;
  int32_t iLastShadePercentage;

  SetCurrentCursorFromDatabase(VIDEO_NO_CURSOR);

  if (guiCurrentScreen == MAP_SCREEN && !(gTacticalStatus.uiFlags & LOADING_SAVED_GAME) &&
      !AreInMeanwhile()) {
    SGPBox const DstRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    uiTimeRange = 2000;
    iPercentage = 0;
    iLastShadePercentage = 0;
    uiStartTime = GetClock();
    BltVideoSurface(guiSAVEBUFFER, FRAME_BUFFER, 0, 0, NULL);
    PlayJA2SampleFromFile(SOUNDSDIR "/final psionic blast 01 (16-44).wav", HIGHVOLUME, 1,
                          MIDDLEPAN);
    while (iPercentage < 100) {
      uiCurrTime = GetClock();
      iPercentage = (uiCurrTime - uiStartTime) * 100 / uiTimeRange;
      iPercentage = std::min(iPercentage, 100);

      // Factor the percentage so that it is modified by a gravity falling
      // acceleration effect.
      iFactor = (iPercentage - 50) * 2;
      if (iPercentage < 50)
        iPercentage = (uint32_t)(iPercentage + iPercentage * iFactor * 0.01 + 0.5);
      else
        iPercentage = (uint32_t)(iPercentage + (100 - iPercentage) * iFactor * 0.01 + 0.05);

      if (iPercentage > 50) {
        // iFactor = (iPercentage - 50) * 2;
        // if( iFactor > iLastShadePercentage )
        //	{
        // Calculate the difference from last shade % to the new one.  Ex: Going
        // from 50% shade value to 60% shade value requires applying 20% to the
        // 50% to achieve 60%. if( iLastShadePercentage ) 	iReqShadePercentage =
        // 100 - (iFactor * 100 / iLastShadePercentage); else 	iReqShadePercentage
        //= iFactor; Record the new final shade percentage. iLastShadePercentage
        // = iFactor;
        guiSAVEBUFFER->ShadowRectUsingLowPercentTable(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        //	}
      }

      SGPBox const SrcRect = {536 * iPercentage / 100, 367 * iPercentage / 100,
                              SCREEN_WIDTH - 541 * iPercentage / 100,
                              SCREEN_HEIGHT - 406 * iPercentage / 100};
      BltStretchVideoSurface(FRAME_BUFFER, guiSAVEBUFFER, &SrcRect, &DstRect);
      InvalidateScreen();
      RefreshScreen();
    }
  }
  FRAME_BUFFER->Fill(Get16BPPColor(FROMRGB(0, 0, 0)));
  InvalidateScreen();
  RefreshScreen();

  // If we are loading a saved game, use the Loading screen we saved into the
  // SavedGameHeader file
  // ( which gets reloaded into gubLastLoadingScreenID )
  if (!gfGotoSectorTransition) {
    LoadingScreenID const id = gTacticalStatus.uiFlags & LOADING_SAVED_GAME
                                   ? gubLastLoadingScreenID
                                   : GetLoadScreenID(gWorldSectorX, gWorldSectorY, gbWorldSectorZ);
    DisplayLoadScreenWithID(id);
  }
}

static void EndLoadScreen() {}

static void InitializeMapStructure();

void InitStrategicEngine() {
  // this runs every time we start the application, so don't put anything in
  // here that's only supposed to run when a new *game* is started!  Those
  // belong in InitStrategicLayer() instead.

  InitializeMapStructure();

  // set up town stuff
  BuildListOfTownSectors();
}

uint8_t GetTownIdForSector(uint8_t const sector) {
  // return the name value of the town in this sector
  return StrategicMap[SECTOR_INFO_TO_STRATEGIC_INDEX(sector)].bNameId;
}

uint8_t GetTownSectorSize(int8_t const town_id) {
  uint8_t n = 0;
  FOR_EACH_SECTOR_IN_TOWN(i, town_id)++ n;
  return n;
}

uint8_t GetTownSectorsUnderControl(int8_t const town_id) {
  uint8_t n = 0;
  FOR_EACH_SECTOR_IN_TOWN(i, town_id) {
    uint32_t const x = SECTORX(i->sector);
    uint32_t const y = SECTORY(i->sector);
    if (StrategicMap[CALCULATE_STRATEGIC_INDEX(x, y)].fEnemyControlled) continue;
    if (NumEnemiesInSector(x, y) != 0) continue;
    ++n;
  }
  return n;
}

static void InitializeStrategicMapSectorTownNames();

static void InitializeMapStructure() {
  memset(StrategicMap, 0, sizeof(StrategicMap));

  InitializeStrategicMapSectorTownNames();
}

void InitializeSAMSites() {
  // Move the landing zone over to the start sector.
  g_merc_arrive_sector = START_SECTOR;

  // All SAM sites start game in perfect working condition.
  FOR_EACH(int16_t const, i, pSamList) {
    StrategicMap[SECTOR_INFO_TO_STRATEGIC_INDEX(*i)].bSAMCondition = 100;
  }

  UpdateAirspaceControl();
}

// get short sector name without town name
void GetShortSectorString(const int16_t sMapX, const int16_t sMapY, wchar_t *const sString,
                          const size_t Length) {
  // OK, build string id like J11
  swprintf(sString, Length, L"%lc%d", L'A' - 1 + sMapY, sMapX);
}

void GetMapFileName(int16_t const x, int16_t const y, int8_t const z, char *const buf,
                    BOOLEAN const add_alternate_map_letter) {
  size_t n = sprintf(buf, "%c%d", 'A' - 1 + y, x);

  if (z != 0) n += sprintf(buf + n, "_b%d", z);

  /* The gfUseAlternateMap flag is set while loading saved games. When starting
   * a new game the underground sector info has not been initialized, so we need
   * the flag to load an alternate sector. */
  if (GetSectorFlagStatus(x, y, z, SF_USE_ALTERNATE_MAP) || gfUseAlternateMap) {
    gfUseAlternateMap = FALSE;
    if (add_alternate_map_letter) n += sprintf(buf + n, "_a");
  }

  if (AreInMeanwhile() && x == 3 && y == 16 && z == 0) {
    if (add_alternate_map_letter) n += sprintf(buf + n, "_m");
  }

  sprintf(buf + n, ".dat");
}

static void HandleRPCDescriptionOfSector(int16_t const x, int16_t const y, int16_t const z) {
  struct SectorDescriptionInfo {
    uint8_t y;
    uint8_t x;
    uint8_t quote;
  };

  SectorDescriptionInfo const sector_description[] = {
      {2, 13, 0},  // B13 Drassen
      {3, 13, 1},  // C13 Drassen
      {4, 13, 2},  // D13 Drassen
      {8, 13, 3},  // H13 Alma
      {8, 14, 4},  // H14 Alma
      {9, 13, 5},  // I13 Alma (extra quote 6 if Sci-fi)
      {9, 14, 7},  // I14 Alma
      {6, 8, 8},   // F8  Cambria
      {6, 9, 9},   // F9  Cambria
      {7, 8, 10},  // G8  Cambria

      {7, 9, 11},  // G9  Cambria
      {3, 6, 12},  // C6  San Mona
      {3, 5, 13},  // C5  San Mona
      {4, 5, 14},  // D5  San Mona
      {2, 2, 15},  // B2  Chitzena
      {1, 2, 16},  // A2  Chitzena
      {7, 1, 17},  // G1  Grumm
      {8, 1, 18},  // H1  Grumm
      {7, 2, 19},  // G2  Grumm
      {8, 2, 20},  // H2  Grumm

      {9, 6, 21},    // I6  Estoni
      {11, 4, 22},   // K4  Orta
      {12, 11, 23},  // L11 Balime
      {12, 12, 24},  // L12 Balime
      {15, 3, 25},   // O3  Meduna
      {16, 3, 26},   // P3  Meduna
      {14, 4, 27},   // N4  Meduna
      {14, 3, 28},   // N3  Meduna
      {15, 4, 30},   // O4  Meduna
      {10, 9, 31},   // J9  Tixa

      {4, 15, 32},  // D15 NE SAM
      {4, 2, 33},   // D2  NW SAM
      {9, 8, 34}    // I8  CENTRAL SAM
  };

  TacticalStatusType &ts = gTacticalStatus;
  // Default to false
  ts.fCountingDownForGuideDescription = FALSE;

  if (GetSectorFlagStatus(x, y, z, SF_HAVE_USED_GUIDE_QUOTE)) return;
  if (z != 0) return;

  // Check if we are in a good sector
  FOR_EACH(SectorDescriptionInfo const, i, sector_description) {
    if (x != i->x || y != i->y) continue;

    // If we're not scifi, skip some
    if (i == &sector_description[3] && !gGameOptions.fSciFi) continue;

    SetSectorFlag(x, y, z, SF_HAVE_USED_GUIDE_QUOTE);

    ts.fCountingDownForGuideDescription = TRUE;
    ts.bGuideDescriptionCountDown = 4 + Random(5);  // 4 to 8 tactical turns
    ts.ubGuideDescriptionToUse = i->quote;
    ts.bGuideDescriptionSectorX = x;
    ts.bGuideDescriptionSectorY = y;

    // Handle guide description (will be needed if a SAM one)
    HandleRPCDescription();
    break;
  }
}

static void EnterSector(int16_t x, int16_t y, int8_t z);
enum {
  ABOUT_TO_LOAD_NEW_MAP,
  ABOUT_TO_TRASH_WORLD,
};
static void HandleDefiniteUnloadingOfWorld(uint8_t ubUnloadCode);

void SetCurrentWorldSector(int16_t const x, int16_t const y, int8_t const z) {
  SyncStrategicTurnTimes();

  // is the sector already loaded?
  if (gWorldSectorX == x && y == gWorldSectorY && z == gbWorldSectorZ) {
    /* Insert the enemies into the newly loaded map based on the strategic
     * information. Note, the flag will return TRUE only if enemies were added.
     * The game may wish to do something else in a case where no enemies are
     * present. */

    SetPendingNewScreen(GAME_SCREEN);
    if (NumEnemyInSector() == 0) {
      PrepareEnemyForSectorBattle();
    }
    if (gubNumCreaturesAttackingTown != 0 && z == 0 &&
        gubSectorIDOfCreatureAttack == SECTOR(x, y)) {
      PrepareCreaturesForBattle();
    }

    if (gfGotoSectorTransition) {
      BeginLoadScreen();
      gfGotoSectorTransition = FALSE;
    }

    HandleHelicopterOnGroundGraphic();

    ResetMilitia();
    AllTeamsLookForAll(TRUE);
    return;
  }

  if (gWorldSectorX != 0 && gWorldSectorY != 0 && gbWorldSectorZ != -1) {
    HandleDefiniteUnloadingOfWorld(ABOUT_TO_LOAD_NEW_MAP);
  }

  // make this the currently loaded sector
  gWorldSectorX = x;
  gWorldSectorY = y;
  gbWorldSectorZ = z;

  // update currently selected map sector to match
  ChangeSelectedMapSector(x, y, z);

  bool const loading_savegame = gTacticalStatus.uiFlags & LOADING_SAVED_GAME;
  if (loading_savegame) {
    SetMusicMode(MUSIC_MAIN_MENU);
  } else {
    StopAnyCurrentlyTalkingSpeech();

    /* Check to see if the sector we are loading is the cave sector under Tixa.
     * If so then we will set up the meanwhile scene to start the creature
     * quest. */
    if (x == 9 && y == 10 && z == 2) {
      InitCreatureQuest();  // Ignored if already active.
    }

    gTacticalStatus.uiTimeSinceLastInTactical = GetWorldTotalMin();
    InitializeTacticalStatusAtBattleStart();
    HandleHelicopterOnGroundSkyriderProfile();
  }

  EnterSector(x, y, z);

  if (!loading_savegame) {
    InitAI();
    ExamineDoorsOnEnteringSector();
  }

  /* Update all the doors in the sector according to the temp file previously
   * loaded, and any changes made by the schedules */
  UpdateDoorGraphicsFromStatus();

  // Set the fact we have visited the  sector
  SetSectorFlag(x, y, z, SF_ALREADY_LOADED);

  // Check for helicopter being on the ground in this sector
  HandleHelicopterOnGroundGraphic();

  if (!loading_savegame) {
    if (gubMusicMode == MUSIC_TACTICAL_ENEMYPRESENT ? NumHostilesInSector(x, y, z) == 0
                                                    : gubMusicMode != MUSIC_TACTICAL_BATTLE) {
      // ATE: Fade FAST
      SetMusicFadeSpeed(5);
      SetMusicMode(MUSIC_TACTICAL_NOTHING);
    }

    // ATE: Check what sector we are in, to show description if we have an RPC
    HandleRPCDescriptionOfSector(x, y, z);

    // ATE: Set Flag for being visited
    SetSectorFlag(x, y, z, SF_HAS_ENTERED_TACTICAL);

    ResetMultiSelection();

    gTacticalStatus.fHasEnteredCombatModeSinceEntering = FALSE;
    gTacticalStatus.fDontAddNewCrows = FALSE;

    // Adjust delay for tense quote
    gTacticalStatus.sCreatureTenseQuoteDelay = 10 + Random(20);

    int16_t sWarpWorldX;
    int16_t sWarpWorldY;
    int8_t bWarpWorldZ;
    int16_t sWarpGridNo;
    if (z >= 2 && GetWarpOutOfMineCodes(&sWarpWorldX, &sWarpWorldY, &bWarpWorldZ, &sWarpGridNo)) {
      gTacticalStatus.uiFlags |= IN_CREATURE_LAIR;
    } else {
      gTacticalStatus.uiFlags &= ~IN_CREATURE_LAIR;
    }

    gTacticalStatus.ubNumCrowsPossible = 5 + Random(5);
  }
}

void RemoveMercsInSector() {
  // ATE: only for OUR guys.. the rest is taken care of in TrashWorld() when a
  // new sector is added...
  FOR_EACH_IN_TEAM(i, OUR_TEAM) { RemoveSoldierFromGridNo(*i); }
}

void PrepareLoadedSector() {
  BOOLEAN fAddCivs = TRUE;
  int8_t bMineIndex = -1;

  if (!(gTacticalStatus.uiFlags & LOADING_SAVED_GAME)) {
    UpdateMercsInSector();
  }

  // Reset ambients!
  HandleNewSectorAmbience(gTilesets[giCurrentTilesetID].ubAmbientID);

  // if we are loading a 'pristine' map ( ie, not loading a saved game )
  if (!(gTacticalStatus.uiFlags & LOADING_SAVED_GAME)) {
    if (!AreReloadingFromMeanwhile()) {
      SetPendingNewScreen(GAME_SCREEN);

      // Make interface the team panel always...
      SetCurrentInterfacePanel(TEAM_PANEL);
    }

    // Check to see if civilians should be added.  Always add civs to maps
    // unless they are in a mine that is shutdown.
    if (gbWorldSectorZ) {
      bMineIndex = GetIdOfMineForSector(gWorldSectorX, gWorldSectorY, gbWorldSectorZ);
      if (bMineIndex != -1) {
        if (!AreThereMinersInsideThisMine((uint8_t)bMineIndex)) {
          fAddCivs = FALSE;
        }
      }
    }
    if (fAddCivs) {
      AddSoldierInitListTeamToWorld(CIV_TEAM);
    }

    AddSoldierInitListTeamToWorld(MILITIA_TEAM);
    AddSoldierInitListBloodcats();
    // Creatures are only added if there are actually some of them.  It has to
    // go through some additional checking.

    PrepareCreaturesForBattle();

    PrepareMilitiaForTactical();

    // OK, set varibles for entring this new sector...
    gTacticalStatus.fVirginSector = TRUE;

    AddProfilesNotUsingProfileInsertionData();

    if (!AreInMeanwhile() ||
        GetMeanwhileID() == INTERROGATION) {  // Insert the enemies into the newly loaded map
                                              // based on the strategic information.
      PrepareEnemyForSectorBattle();
    }

    // Regardless whether or not this was set, clear it now.
    gfRestoringEnemySoldiersFromTempFile = FALSE;

    //@@@Evaluate
    // Add profiles to world using strategic info, not editor placements.
    AddProfilesUsingProfileInsertionData();

    PostSchedules();
  }

  if (gubEnemyEncounterCode == ENEMY_AMBUSH_CODE || gubEnemyEncounterCode == BLOODCAT_AMBUSH_CODE) {
    if (gMapInformation.sCenterGridNo != -1) {
      CallAvailableEnemiesTo(gMapInformation.sCenterGridNo);
    } else {
    }
  }

  EndLoadScreen();

  if (!(gTacticalStatus.uiFlags & LOADING_SAVED_GAME)) {
    // unpause game
    UnPauseGame();
  }

  gpBattleGroup = NULL;

  if (gfTacticalTraversal) {
    CalculateNonPersistantPBIInfo();
  }

  ScreenMsg(FONT_YELLOW, MSG_DEBUG, L"Current Time is: %d", GetWorldTotalMin());

  AllTeamsLookForAll(TRUE);
}

#define RANDOM_HEAD_MINERS 4
void HandleQuestCodeOnSectorEntry(int16_t sNewSectorX, int16_t sNewSectorY, int8_t bNewSectorZ) {
  uint8_t ubRandomMiner[RANDOM_HEAD_MINERS] = {106, 156, 157, 158};
  uint8_t ubMiner, ubMinersPlaced;
  uint8_t ubMine;

  if (CheckFact(FACT_ALL_TERRORISTS_KILLED, 0)) {
    // end terrorist quest
    EndQuest(QUEST_KILL_TERRORISTS, gMercProfiles[CARMEN].sSectorX, gMercProfiles[CARMEN].sSectorY);
    // remove Carmen
    gMercProfiles[CARMEN].sSectorX = 0;
    gMercProfiles[CARMEN].sSectorY = 0;
    gMercProfiles[CARMEN].bSectorZ = 0;
  }

  uint8_t const sector = SECTOR(sNewSectorX, sNewSectorY);
  // are we in a mine sector, on the surface?
  if (bNewSectorZ == 0) {
    int8_t const ubThisMine = GetMineIndexForSector(sector);
    if (ubThisMine != -1 && !CheckFact(FACT_MINERS_PLACED, 0)) {
      // SET HEAD MINER LOCATIONS
      if (ubThisMine != MINE_SAN_MONA)  // San Mona is abandoned
      {
        ubMinersPlaced = 0;

        if (ubThisMine != MINE_ALMA) {
          // Fred Morris is always in the first mine sector we enter, unless
          // that's Alma (then he's randomized, too)
          MERCPROFILESTRUCT &fred = GetProfile(FRED);
          fred.sSectorX = sNewSectorX;
          fred.sSectorY = sNewSectorY;
          fred.bSectorZ = 0;
          fred.bTown = gMineLocation[ubThisMine].bAssociatedTown;

          // mark miner as placed
          ubRandomMiner[0] = 0;
          ubMinersPlaced++;
        }

        // assign the remaining (3) miners randomly
        for (ubMine = 0; ubMine < MAX_NUMBER_OF_MINES; ubMine++) {
          if (ubMine == ubThisMine || ubMine == MINE_ALMA || ubMine == MINE_SAN_MONA) {
            // Alma always has Matt as a miner, and we have assigned Fred to the
            // current mine and San Mona is abandoned
            continue;
          }

          do {
            ubMiner = (uint8_t)Random(RANDOM_HEAD_MINERS);
          } while (ubRandomMiner[ubMiner] == 0);

          MERCPROFILESTRUCT &p = GetProfile(ubRandomMiner[ubMiner]);
          uint8_t const sector = GetMineSector(ubMine);
          p.sSectorX = SECTORX(sector);
          p.sSectorY = SECTORY(sector);
          p.bSectorZ = 0;
          p.bTown = gMineLocation[ubMine].bAssociatedTown;

          // mark miner as placed
          ubRandomMiner[ubMiner] = 0;
          ubMinersPlaced++;

          if (ubMinersPlaced == RANDOM_HEAD_MINERS) {
            break;
          }
        }

        SetFactTrue(FACT_MINERS_PLACED);
      }
    }
  }

  if (!CheckFact(FACT_ROBOT_RECRUITED_AND_MOVED, 0)) {
    const SOLDIERTYPE *const pRobot = FindSoldierByProfileIDOnPlayerTeam(ROBOT);
    if (pRobot) {
      // robot is on our team and we have changed sectors, so we can
      // replace the robot-under-construction in Madlab's sector
      RemoveGraphicFromTempFile(gsRobotGridNo, SEVENTHISTRUCT1, gMercProfiles[MADLAB].sSectorX,
                                gMercProfiles[MADLAB].sSectorY, gMercProfiles[MADLAB].bSectorZ);
      SetFactTrue(FACT_ROBOT_RECRUITED_AND_MOVED);
    }
  }

  // Check to see if any player merc has the Chalice; if so,
  // note it as stolen
  CFOR_EACH_IN_TEAM(s, OUR_TEAM) {
    if (FindObj(s, CHALICE) != ITEM_NOT_FOUND) {
      SetFactTrue(FACT_CHALICE_STOLEN);
    }
  }

  if (gubQuest[QUEST_KINGPIN_MONEY] == QUESTINPROGRESS &&
      CheckFact(FACT_KINGPIN_CAN_SEND_ASSASSINS, 0) && GetTownIdForSector(sector) != BLANK_SECTOR &&
      Random(10 + GetNumberOfMilitiaInSector(sNewSectorX, sNewSectorY, bNewSectorZ)) < 3) {
    DecideOnAssassin();
  }

  /*
          if (sector == SEC_C5)
          {
                  // reset Madame Layla counters
                  gMercProfiles[ MADAME ].bNPCData = 0;
                  gMercProfiles[ MADAME ].bNPCData2 = 0;
          }
          */

  if (sector == SEC_C6 && gubQuest[QUEST_RESCUE_MARIA] == QUESTDONE) {
    // make sure Maria and Angel are gone
    gMercProfiles[MARIA].sSectorX = 0;
    gMercProfiles[MARIA].sSectorY = 0;
    gMercProfiles[ANGEL].sSectorX = 0;
    gMercProfiles[ANGEL].sSectorY = 0;
  }

  if (sector == SEC_D5) {
    gBoxer[0] = NULL;
    gBoxer[1] = NULL;
    gBoxer[2] = NULL;
  }

  if (sector == SEC_P3) {
    // heal up Elliot if he's been hurt
    if (gMercProfiles[ELLIOT].bMercStatus != MERC_IS_DEAD) {
      gMercProfiles[ELLIOT].bLife = gMercProfiles[ELLIOT].bLifeMax;
    }
  }

  ResetOncePerConvoRecordsForAllNPCsInLoadedSector();
}

static void HandleQuestCodeOnSectorExit(int16_t sOldSectorX, int16_t sOldSectorY,
                                        int8_t bOldSectorZ) {
  if (sOldSectorX == KINGPIN_MONEY_SECTOR_X && sOldSectorY == KINGPIN_MONEY_SECTOR_Y &&
      bOldSectorZ == KINGPIN_MONEY_SECTOR_Z) {
    CheckForKingpinsMoneyMissing(TRUE);
  }

  if (sOldSectorX == 13 && sOldSectorY == MAP_ROW_H && bOldSectorZ == 0 &&
      CheckFact(FACT_CONRAD_SHOULD_GO, 0)) {
    // remove Conrad from the map
    gMercProfiles[CONRAD].sSectorX = 0;
    gMercProfiles[CONRAD].sSectorY = 0;
  }

  if (sOldSectorX == HOSPITAL_SECTOR_X && sOldSectorY == HOSPITAL_SECTOR_Y &&
      bOldSectorZ == HOSPITAL_SECTOR_Z) {
    CheckForMissingHospitalSupplies();
  }

  // reset the state of the museum alarm for Eldin's quotes
  SetFactFalse(FACT_MUSEUM_ALARM_WENT_OFF);
}

static void SetupProfileInsertionDataForCivilians() {
  FOR_EACH_IN_TEAM(s, CIV_TEAM) {
    if (s->bInSector) SetupProfileInsertionDataForSoldier(s);
  }
}

static void EnterSector(int16_t const x, int16_t const y, int8_t const z) {
  PauseGame();
  // Stop time for this frame
  InterruptTime();

  /* Setup the tactical existance of RPCs and CIVs in the last sector before
   * moving on to a new sector. */
  //@@@Evaluate
  if (gfWorldLoaded) SetupProfileInsertionDataForCivilians();

  if (!(gTacticalStatus.uiFlags & LOADING_SAVED_GAME)) {
    // Handle NPC stuff related to changing sectors
    HandleQuestCodeOnSectorEntry(x, y, z);
  }

  BeginLoadScreen();

  /* This has to be done before loadworld, as it will remmove old gridnos if
   * present */
  RemoveMercsInSector();

  if (!AreInMeanwhile()) {
    SetSectorFlag(x, y, z, SF_ALREADY_VISITED);
  }

  CreateLoadingScreenProgressBar();

  char filename[50];
  GetMapFileName(x, y, z, filename, TRUE);
  LoadWorld(filename);
  LoadRadarScreenBitmap(filename);

  /* ATE: Moved this form above, so that we can have the benefit of changing the
   * world BEFORE adding guys to it. */
  if (!(gTacticalStatus.uiFlags & LOADING_SAVED_GAME)) {
    try {  // Load the current sectors Information From the temporary files
      LoadCurrentSectorsInformationFromTempItemsFile();
    } catch (...) { /* The integrity of the temp files have been compromised.
                     * Boot out of the game after warning message. */
      InitExitGameDialogBecauseFileHackDetected();
      return;
    }
  }

  RemoveLoadingScreenProgressBar();

  if (gfEnterTacticalPlacementGUI) {
    SetPendingNewScreen(GAME_SCREEN);
    InitTacticalPlacementGUI();
  } else {
    EndMapScreen(FALSE);
    PrepareLoadedSector();
  }

  /* This function will either hide or display the tree tops, depending on the
   * game setting */
  SetTreeTopStateForMap();
}

void UpdateMercsInSector() {
  // Remove from interface slot
  RemoveAllPlayersFromSlot();

  // Remove tactical interface stuff
  guiPendingOverrideEvent = I_CHANGE_TO_IDLE;

  // If we are in this function during the loading of a sector
  if (!(gTacticalStatus.uiFlags & LOADING_SAVED_GAME)) {
    // DONT set these values
    SetSelectedMan(NULL);
    gfGameScreenLocateToSoldier = TRUE;
  }

  SetAllAutoFacesInactive();

  if (fUsingEdgePointsForStrategicEntry) {
    BeginMapEdgepointSearch();
  }

  uint8_t pow_squad = NO_CURRENT_SQUAD;
  uint8_t const first_enemy = gTacticalStatus.Team[ENEMY_TEAM].bFirstID;
  uint8_t const last_enemy = gTacticalStatus.Team[CREATURE_TEAM].bLastID;
  int16_t const sSectorX = gWorldSectorX;
  int16_t const sSectorY = gWorldSectorY;
  int8_t const bSectorZ = gbWorldSectorZ;
  for (int32_t i = 0; i != MAX_NUM_SOLDIERS; ++i) {
    if (gfRestoringEnemySoldiersFromTempFile && first_enemy <= i &&
        i <= last_enemy) { /* Don't update enemies/creatures (consec. teams) if
                            * they were just restored via the temp map files */
      continue;
    }

    SOLDIERTYPE &s = GetMan(i);
    RemoveMercSlot(&s);

    s.bInSector = FALSE;

    if (!s.bActive) continue;
    if (s.sSectorX != sSectorX) continue;
    if (s.sSectorY != sSectorY) continue;
    if (s.bSectorZ != bSectorZ) continue;
    if (s.fBetweenSectors) continue;

    if (!(gTacticalStatus.uiFlags & LOADING_SAVED_GAME)) {
      if (gMapInformation.sCenterGridNo != -1 && gfBlitBattleSectorLocator && s.bTeam != CIV_TEAM &&
          (gubEnemyEncounterCode == ENEMY_AMBUSH_CODE ||
           gubEnemyEncounterCode == BLOODCAT_AMBUSH_CODE)) {
        s.ubStrategicInsertionCode = INSERTION_CODE_GRIDNO;
        s.usStrategicInsertionData = gMapInformation.sCenterGridNo;
      } else if (gfOverrideInsertionWithExitGrid) {
        s.ubStrategicInsertionCode = INSERTION_CODE_GRIDNO;
        s.usStrategicInsertionData = gExitGrid.usGridNo;
      }
    }

    UpdateMercInSector(s, sSectorX, sSectorY, bSectorZ);

    if (gTacticalStatus.uiFlags & LOADING_SAVED_GAME) continue;
    if (s.bAssignment != ASSIGNMENT_POW) continue;

    if (pow_squad == NO_CURRENT_SQUAD) {
      // ATE: If we are in i13 - pop up message!
      if (sSectorY == MAP_ROW_I && sSectorX == 13) {
        DoMessageBox(MSG_BOX_BASIC_STYLE, TacticalStr[POW_MERCS_ARE_HERE], GAME_SCREEN,
                     MSG_BOX_FLAG_OK, NULL, NULL);
      } else {
        AddCharacterToUniqueSquad(&s);
        pow_squad = s.bAssignment;
        s.bNeutral = FALSE;
      }
    } else {
      if (sSectorY != MAP_ROW_I && sSectorX != 13) {
        AddCharacterToSquad(&s, pow_squad);
      }
    }

    // ATE: Call actions based on what POW we are on...
    if (gubQuest[QUEST_HELD_IN_ALMA] == QUESTINPROGRESS) {
      EndQuest(QUEST_HELD_IN_ALMA, sSectorX, sSectorY);
      HandleNPCDoAction(0, NPC_ACTION_GRANT_EXPERIENCE_3, 0);
    }
  }

  if (fUsingEdgePointsForStrategicEntry) {
    EndMapEdgepointSearch();
    fUsingEdgePointsForStrategicEntry = FALSE;
  }
}

static void GetLoadedSectorString(wchar_t *pString, size_t Length);

void UpdateMercInSector(SOLDIERTYPE &s, int16_t const x, int16_t const y, int8_t const z) {
  // Determine entrance direction and get sweetspot
  // Some checks here must be fleshed out

  if (!s.bActive) return;

  if (s.bAssignment == IN_TRANSIT) return;

  if (s.ubProfile != NO_PROFILE &&
      GetProfile(s.ubProfile).ubMiscFlags3 &
          PROFILE_MISC_FLAG3_PERMANENT_INSERTION_CODE) {  // Override orders
    s.bOrders = STATIONARY;
  }

  if (s.ubStrategicInsertionCode == INSERTION_CODE_PRIMARY_EDGEINDEX ||
      s.ubStrategicInsertionCode == INSERTION_CODE_SECONDARY_EDGEINDEX) {
    if (!fUsingEdgePointsForStrategicEntry) {  // If we are not supposed to use
                                               // this now, pick something better
      s.ubStrategicInsertionCode = (uint8_t)s.usStrategicInsertionData;
    }
  }

MAPEDGEPOINT_SEARCH_FAILED:
  // Use insertion direction from loaded map
  GridNo gridno;
  switch (s.ubStrategicInsertionCode) {
    case INSERTION_CODE_NORTH:
      gridno = gMapInformation.sNorthGridNo;
      goto check_entry;
    case INSERTION_CODE_SOUTH:
      gridno = gMapInformation.sSouthGridNo;
      goto check_entry;
    case INSERTION_CODE_EAST:
      gridno = gMapInformation.sEastGridNo;
      goto check_entry;
    case INSERTION_CODE_WEST:
      gridno = gMapInformation.sWestGridNo;
      goto check_entry;
    case INSERTION_CODE_CENTER:
      gridno = gMapInformation.sCenterGridNo;
      goto check_entry;
    check_entry:
      if (gridno == -1 && !gfEditMode) { /* Strategic insertion failed because it expected to find
                                          * an entry point. This is likely a missing part of the
                                          * map or possible fault in strategic movement costs,
                                          * traversal logic, etc. */
        wchar_t sector[10];
        GetLoadedSectorString(sector, lengthof(sector));

        wchar_t const *entry;
        if (gMapInformation.sNorthGridNo != -1) {
          entry = L"north";
          gridno = gMapInformation.sNorthGridNo;
        } else if (gMapInformation.sEastGridNo != -1) {
          entry = L"east";
          gridno = gMapInformation.sEastGridNo;
        } else if (gMapInformation.sSouthGridNo != -1) {
          entry = L"south";
          gridno = gMapInformation.sSouthGridNo;
        } else if (gMapInformation.sWestGridNo != -1) {
          entry = L"west";
          gridno = gMapInformation.sWestGridNo;
        } else if (gMapInformation.sCenterGridNo != -1) {
          entry = L"center";
          gridno = gMapInformation.sCenterGridNo;
        } else {
          ScreenMsg(FONT_RED, MSG_BETAVERSION,
                    L"Sector %ls has NO entrypoints -- using precise center of "
                    L"map for %ls.",
                    sector, s.name);
          goto place_in_center;
        }
        wchar_t const *no_entry = 0;
        switch (s.ubStrategicInsertionCode) {
          case INSERTION_CODE_NORTH:
            no_entry = L"north";
            break;
          case INSERTION_CODE_EAST:
            no_entry = L"east";
            break;
          case INSERTION_CODE_SOUTH:
            no_entry = L"south";
            break;
          case INSERTION_CODE_WEST:
            no_entry = L"west";
            break;
          case INSERTION_CODE_CENTER:
            no_entry = L"center";
            break;
        }
        if (no_entry) {
          ScreenMsg(FONT_RED, MSG_BETAVERSION,
                    L"Sector %ls doesn't have a %ls entrypoint -- substituting "
                    L"%ls entrypoint for %ls.",
                    sector, no_entry, entry, s.name);
        }
      }
      break;

    case INSERTION_CODE_GRIDNO:
      gridno = s.usStrategicInsertionData;
      break;

    case INSERTION_CODE_PRIMARY_EDGEINDEX: {
      gridno = SearchForClosestPrimaryMapEdgepoint(s.sPendingActionData2,
                                                   (uint8_t)s.usStrategicInsertionData);
      if (gridno == NOWHERE) {
        ScreenMsg(FONT_RED, MSG_ERROR,
                  L"Main edgepoint search failed for %ls -- substituting entrypoint.", s.name);
        s.ubStrategicInsertionCode = (uint8_t)s.usStrategicInsertionData;
        goto MAPEDGEPOINT_SEARCH_FAILED;
      }
      break;
    }

    case INSERTION_CODE_SECONDARY_EDGEINDEX: {
      gridno = SearchForClosestSecondaryMapEdgepoint(s.sPendingActionData2,
                                                     (uint8_t)s.usStrategicInsertionData);
      if (gridno == NOWHERE) {
        ScreenMsg(FONT_RED, MSG_ERROR,
                  L"Isolated edgepoint search failed for %ls -- substituting "
                  L"entrypoint.",
                  s.name);
        s.ubStrategicInsertionCode = (uint8_t)s.usStrategicInsertionData;
        goto MAPEDGEPOINT_SEARCH_FAILED;
      }
      break;
    }

    case INSERTION_CODE_ARRIVING_GAME:
      // Are we in the start sector?
      if (SECTOR(x, y) == START_SECTOR && z == 0 &&
          SECTOR(gWorldSectorX, gWorldSectorY) == START_SECTOR &&
          gbWorldSectorZ == 0) {  // Try another location and walk into map
        gridno = 4379;
      } else {
        s.ubStrategicInsertionCode = INSERTION_CODE_NORTH;
        gridno = gMapInformation.sNorthGridNo;
      }
      break;

    case INSERTION_CODE_CHOPPER:
      AddMercToHeli(&s);
      return;

    default:
      DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
               String("Improper insertion code %d given to UpdateMercsInSector",
                      s.ubStrategicInsertionCode));
      goto place_in_center;
  }

  // If no insertion direction exists, this is bad!
  if (gridno == -1) {
    DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
             String("Insertion gridno for direction %d not added to map sector %d %d",
                    s.ubStrategicInsertionCode, x, y));
  place_in_center:
    gridno = WORLD_ROWS / 2 * WORLD_COLS + WORLD_COLS / 2;
  }

  s.sInsertionGridNo = gridno;
  AddSoldierToSector(&s);
}

static void InitializeStrategicMapSectorTownNames() {
  StrategicMap[2 + 2 * MAP_WORLD_X].bNameId = StrategicMap[2 + 1 * MAP_WORLD_X].bNameId = CHITZENA;
  StrategicMap[5 + 3 * MAP_WORLD_X].bNameId = StrategicMap[6 + 3 * MAP_WORLD_X].bNameId =
      StrategicMap[5 + 4 * MAP_WORLD_X].bNameId = StrategicMap[4 + 4 * MAP_WORLD_X].bNameId =
          SAN_MONA;
  StrategicMap[9 + 1 * MAP_WORLD_X].bNameId = StrategicMap[10 + 1 * MAP_WORLD_X].bNameId = OMERTA;
  StrategicMap[13 + 2 * MAP_WORLD_X].bNameId = StrategicMap[13 + 3 * MAP_WORLD_X].bNameId =
      StrategicMap[13 + 4 * MAP_WORLD_X].bNameId = DRASSEN;
  StrategicMap[1 + 7 * MAP_WORLD_X].bNameId = StrategicMap[1 + 8 * MAP_WORLD_X].bNameId =
      StrategicMap[2 + 7 * MAP_WORLD_X].bNameId = StrategicMap[2 + 8 * MAP_WORLD_X].bNameId =
          StrategicMap[3 + 8 * MAP_WORLD_X].bNameId = GRUMM;
  StrategicMap[6 + 9 * MAP_WORLD_X].bNameId = ESTONI;
  StrategicMap[9 + 10 * MAP_WORLD_X].bNameId = TIXA;
  StrategicMap[8 + 6 * MAP_WORLD_X].bNameId = StrategicMap[9 + 6 * MAP_WORLD_X].bNameId =
      StrategicMap[8 + 7 * MAP_WORLD_X].bNameId = StrategicMap[9 + 7 * MAP_WORLD_X].bNameId =
          StrategicMap[8 + 8 * MAP_WORLD_X].bNameId = CAMBRIA;
  StrategicMap[13 + 9 * MAP_WORLD_X].bNameId = StrategicMap[14 + 9 * MAP_WORLD_X].bNameId =
      StrategicMap[13 + 8 * MAP_WORLD_X].bNameId = StrategicMap[14 + 8 * MAP_WORLD_X].bNameId =
          ALMA;
  StrategicMap[4 + 11 * MAP_WORLD_X].bNameId = ORTA;
  StrategicMap[11 + 12 * MAP_WORLD_X].bNameId = StrategicMap[12 + 12 * MAP_WORLD_X].bNameId =
      BALIME;
  StrategicMap[3 + 14 * MAP_WORLD_X].bNameId = StrategicMap[4 + 14 * MAP_WORLD_X].bNameId =
      StrategicMap[5 + 14 * MAP_WORLD_X].bNameId = StrategicMap[3 + 15 * MAP_WORLD_X].bNameId =
          StrategicMap[4 + 15 * MAP_WORLD_X].bNameId = StrategicMap[3 + 16 * MAP_WORLD_X].bNameId =
              MEDUNA;
}

void GetSectorIDString(int16_t const x, int16_t const y, int8_t const z, wchar_t *const buf,
                       size_t const length, BOOLEAN const detailed) {
  if (x <= 0 || y <= 0 || z < 0) /* Empty? */
  {
    // swprintf(buf, L"%ls", pErrorStrings);
    return;
  }

  int8_t const mine_index = GetIdOfMineForSector(x, y, z);
  wchar_t const *add;
  if (z != 0) {
    UNDERGROUND_SECTORINFO const *const u = FindUnderGroundSector(x, y, z);
    if (!u || (!(u->uiFlags & SF_ALREADY_VISITED) &&
               !gfGettingNameFromSaveLoadScreen)) {  // Display nothing
      buf[0] = L'\0';
      return;
    }

    if (mine_index != -1) {
      add = pTownNames[GetTownAssociatedWithMine(mine_index)];
    } else
      switch (SECTOR(x, y)) {
        case SEC_A10:
          add = pLandTypeStrings[REBEL_HIDEOUT];
          break;
        case SEC_J9:
          add = pLandTypeStrings[TIXA_DUNGEON];
          break;
        case SEC_K4:
          add = pLandTypeStrings[ORTA_BASEMENT];
          break;
        case SEC_O3:
          add = pLandTypeStrings[TUNNEL];
          break;
        case SEC_P3:
          add = pLandTypeStrings[SHELTER];
          break;
        default:
          add = pLandTypeStrings[CREATURE_LAIR];
          break;
      }
  } else {
    uint8_t const sector_id = SECTOR(x, y);
    switch (sector_id) {
      case SEC_B13:
        if (!detailed) goto plain_sector;
        add = pLandTypeStrings[DRASSEN_AIRPORT_SITE];
        break;

      case SEC_D2:  // Chitzena SAM
        add = !fSamSiteFound[SAM_SITE_ONE] ? pLandTypeStrings[TROPICS]
              : detailed                   ? pLandTypeStrings[TROPICS_SAM_SITE]
                                           : pLandTypeStrings[SAM_SITE];
        break;

      case SEC_D15:  // Drassen SAM
        add = !fSamSiteFound[SAM_SITE_TWO] ? pLandTypeStrings[SPARSE]
              : detailed                   ? pLandTypeStrings[SPARSE_SAM_SITE]
                                           : pLandTypeStrings[SAM_SITE];
        break;

      case SEC_F8:
        if (!detailed) goto plain_sector;
        add = pLandTypeStrings[CAMBRIA_HOSPITAL_SITE];
        break;

      case SEC_I8:  // Cambria SAM
        add = !fSamSiteFound[SAM_SITE_THREE] ? pLandTypeStrings[SAND]
              : detailed                     ? pLandTypeStrings[SAND_SAM_SITE]
                                             : pLandTypeStrings[SAM_SITE];
        break;

      case SEC_J9:  // Tixa
        add = fFoundTixa ? pTownNames[TIXA] : pLandTypeStrings[SAND];
        break;

      case SEC_K4:  // Orta
        add = fFoundOrta ? pTownNames[ORTA] : pLandTypeStrings[SWAMP];
        break;

      case SEC_N3:
        if (!detailed) goto plain_sector;
        add = pLandTypeStrings[MEDUNA_AIRPORT_SITE];
        break;

      case SEC_N4:  // Meduna's SAM site
        if (!fSamSiteFound[SAM_SITE_FOUR]) goto plain_sector;
        add = detailed ? pLandTypeStrings[MEDUNA_SAM_SITE] : pLandTypeStrings[SAM_SITE];
        break;

      default:  // All other towns that are known since beginning of the game.
      plain_sector:;
        int8_t const town_name_id = StrategicMap[CALCULATE_STRATEGIC_INDEX(x, y)].bNameId;
        add =
            town_name_id != BLANK_SECTOR
                ? pTownNames[town_name_id]
                : pLandTypeStrings[SectorInfo[sector_id].ubTraversability[THROUGH_STRATEGIC_MOVE]];
        break;
    }
  }

  size_t const n = swprintf(buf, length, L"%c%d: %ls", 'A' + y - 1, x, add);
  if (detailed && mine_index != -1) {  // Append "Mine"
    swprintf(buf + n, length - n, L" %ls", pwMineStrings[0]);
  }
}

static void SetInsertionDataFromAdjacentMoveDirection(SOLDIERTYPE &s,
                                                      uint8_t const tactical_direction,
                                                      int16_t const additional_data) {
  // Set insertion code
  switch (tactical_direction) {
    case 255:
      // We are using an exit grid, set insertion values
      EXITGRID ExitGrid;
      if (!GetExitGrid(additional_data, &ExitGrid)) {
        AssertMsg(0,
                  "No valid Exit grid can be found when one was expected: "
                  "SetInsertionDataFromAdjacentMoveDirection.");
      }
      s.ubStrategicInsertionCode = INSERTION_CODE_GRIDNO;
      s.usStrategicInsertionData = ExitGrid.usGridNo;
      s.bUseExitGridForReentryDirection = TRUE;
      break;

    case NORTH:
      s.ubStrategicInsertionCode = INSERTION_CODE_SOUTH;
      break;
    case SOUTH:
      s.ubStrategicInsertionCode = INSERTION_CODE_NORTH;
      break;
    case EAST:
      s.ubStrategicInsertionCode = INSERTION_CODE_WEST;
      break;
    case WEST:
      s.ubStrategicInsertionCode = INSERTION_CODE_EAST;
      break;

    default:  // Wrong direction given
      s.ubStrategicInsertionCode = INSERTION_CODE_WEST;
      break;
  }
}

static uint8_t GetInsertionDataFromAdjacentMoveDirection(uint8_t ubTacticalDirection,
                                                         int16_t sAdditionalData) {
  uint8_t ubDirection;

  // Set insertion code
  switch (ubTacticalDirection) {
      // OK, we are using an exit grid - set insertion values...

    case 255:

      ubDirection = 255;
      break;

    case NORTH:
      ubDirection = NORTH_STRATEGIC_MOVE;
      break;
    case SOUTH:
      ubDirection = SOUTH_STRATEGIC_MOVE;
      break;
    case EAST:
      ubDirection = EAST_STRATEGIC_MOVE;
      break;
    case WEST:
      ubDirection = WEST_STRATEGIC_MOVE;
      break;
    default:
      // Wrong direction given!
      ubDirection = EAST_STRATEGIC_MOVE;
  }

  return (ubDirection);
}

static uint8_t GetStrategicInsertionDataFromAdjacentMoveDirection(uint8_t ubTacticalDirection,
                                                                  int16_t sAdditionalData) {
  uint8_t ubDirection;

  // Set insertion code
  switch (ubTacticalDirection) {
      // OK, we are using an exit grid - set insertion values...

    case 255:

      ubDirection = 255;
      break;

    case NORTH:
      ubDirection = INSERTION_CODE_SOUTH;
      break;
    case SOUTH:
      ubDirection = INSERTION_CODE_NORTH;
      break;
    case EAST:
      ubDirection = INSERTION_CODE_WEST;
      break;
    case WEST:
      ubDirection = INSERTION_CODE_EAST;
      break;
    default:
      // Wrong direction given!
      ubDirection = EAST_STRATEGIC_MOVE;
  }

  return (ubDirection);
}

static int16_t PickGridNoNearestEdge(SOLDIERTYPE *pSoldier, uint8_t ubTacticalDirection);

void JumpIntoAdjacentSector(uint8_t ubTacticalDirection, uint8_t ubJumpCode,
                            int16_t sAdditionalData) {
  SOLDIERTYPE *pValidSoldier = NULL;
  uint32_t uiTraverseTime = 0;
  uint8_t ubDirection = (uint8_t)-1;  // XXX HACK000E
  EXITGRID ExitGrid;

  // Set initial selected
  // ATE: moved this towards top...
  SOLDIERTYPE *const sel = GetSelectedMan();
  gPreferredInitialSelectedGuy = sel;

  if (ubJumpCode == JUMP_ALL_LOAD_NEW || ubJumpCode == JUMP_ALL_NO_LOAD) {
    // TODO: Check flags to see if we can jump!
    // Move controllable mercs!
    FOR_EACH_IN_TEAM(s, OUR_TEAM) {
      // If we are controllable
      if (OkControllableMerc(s) && s->bAssignment == CurrentSquad()) {
        pValidSoldier = s;
        // This now gets handled by strategic movement.  It is possible that the
        // group won't move instantaneously.
        // s->sSectorX = sNewX;
        // s->sSectorY = sNewY;

        ubDirection =
            GetInsertionDataFromAdjacentMoveDirection(ubTacticalDirection, sAdditionalData);
        break;
      }
    }
  } else if ((ubJumpCode == JUMP_SINGLE_LOAD_NEW || ubJumpCode == JUMP_SINGLE_NO_LOAD)) {
    // Use selected soldier...
    // This guy should always be 1 ) selected and 2 ) close enough to exit
    // sector to leave
    if (sel != NULL) {
      pValidSoldier = sel;
      ubDirection = GetInsertionDataFromAdjacentMoveDirection(ubTacticalDirection, sAdditionalData);
    }

    if (ubJumpCode == JUMP_SINGLE_NO_LOAD) {  // handle soldier moving by themselves
      HandleSoldierLeavingSectorByThemSelf(pValidSoldier);
    } else {  // now add char to a squad all their own
      AddCharacterToUniqueSquad(pValidSoldier);
    }
  } else {
    // OK, no jump code here given...
    DebugMsg(TOPIC_JA2, DBG_LEVEL_3,
             String("Improper jump code %d given to JumpIntoAdjacentSector", ubJumpCode));
  }

  Assert(pValidSoldier);

  // Now, determine the traversal time.
  GROUP *const pGroup = GetGroup(pValidSoldier->ubGroupID);
  AssertMsg(pGroup, String("%ls is not in a valid group (pSoldier->ubGroupID is %d)",
                           pValidSoldier->name, pValidSoldier->ubGroupID));

  // If we are going through an exit grid, don't get traversal direction!
  if (ubTacticalDirection != 255) {
    if (!gbWorldSectorZ) {
      uiTraverseTime = GetSectorMvtTimeForGroup(
          (uint8_t)SECTOR(pGroup->ubSectorX, pGroup->ubSectorY), ubDirection, pGroup);
    } else if (gbWorldSectorZ > 0) {  // We are attempting to traverse in an underground
                                      // environment.  We need to use a complete different
      // method.  When underground, all sectors are instantly adjacent.
      uiTraverseTime = UndergroundTacticalTraversalTime(ubDirection);
    }
    AssertMsg(uiTraverseTime != TRAVERSE_TIME_IMPOSSIBLE,
              "Attempting to tactically traverse to adjacent sector, despite "
              "being unable to do so.");
  }

  // Alrighty, we want to do whatever our omnipotent player asked us to do
  // this is what the ubJumpCode is for.
  // Regardless of that we were asked to do, we MUST walk OFF ( Ian loves
  // this... ) So..... let's setup our people to walk off... We deal with a
  // pGroup here... if an all move or a group...

  // Setup some globals so our callback that deals when guys go off screen is
  // handled.... Look in the handler function AllMercsHaveWalkedOffSector()
  // below...
  gpAdjacentGroup = pGroup;
  gubAdjacentJumpCode = ubJumpCode;
  guiAdjacentTraverseTime = uiTraverseTime;
  gubTacticalDirection = ubTacticalDirection;
  gsAdditionalData = sAdditionalData;

  // If normal direction, use it!
  if (ubTacticalDirection != 255) {
    gsAdjacentSectorX = (int16_t)(gWorldSectorX + DirXIncrementer[ubTacticalDirection]);
    gsAdjacentSectorY = (int16_t)(gWorldSectorY + DirYIncrementer[ubTacticalDirection]);
    gbAdjacentSectorZ = pValidSoldier->bSectorZ;
  } else {
    // Take directions from exit grid info!
    if (!GetExitGrid(sAdditionalData, &ExitGrid)) {
      AssertMsg(0, String("Told to use exit grid at %d but one does not exist", sAdditionalData));
    }

    gsAdjacentSectorX = ExitGrid.ubGotoSectorX;
    gsAdjacentSectorY = ExitGrid.ubGotoSectorY;
    gbAdjacentSectorZ = ExitGrid.ubGotoSectorZ;
  }

  // Give guy(s) orders to walk off sector...
  if (pGroup->fPlayer) {  // For player groups, update the soldier information
    uint8_t ubNum = 0;

    CFOR_EACH_PLAYER_IN_GROUP(curr, pGroup) {
      if (OK_CONTROLLABLE_MERC(curr->pSoldier)) {
        if (ubTacticalDirection != 255) {
          const int16_t sGridNo = PickGridNoNearestEdge(curr->pSoldier, ubTacticalDirection);

          curr->pSoldier->sPreTraversalGridNo = curr->pSoldier->sGridNo;

          if (sGridNo != NOWHERE) {
            // Save wait code - this will make buddy walk off screen into
            // oblivion
            curr->pSoldier->ubWaitActionToDo = 2;
            // This will set the direction so we know now to move into oblivion
            curr->pSoldier->uiPendingActionData1 = ubTacticalDirection;
          } else {
            AssertMsg(0, "Failed to get good exit location for adjacentmove");
          }

          EVENT_GetNewSoldierPath(curr->pSoldier, sGridNo, WALKING);

        } else {
          // Here, get closest location for exit grid....
          const int16_t sGridNo =
              FindGridNoFromSweetSpotCloseToExitGrid(curr->pSoldier, sAdditionalData, 10);

          // curr->pSoldier->
          if (sGridNo != NOWHERE) {
            // Save wait code - this will make buddy walk off screen into
            // oblivion
            //	curr->pSoldier->ubWaitActionToDo = 2;
          } else {
            AssertMsg(0, "Failed to get good exit location for adjacentmove");
          }

          // Don't worry about walk off screen, just stay at gridno...
          curr->pSoldier->ubWaitActionToDo = 1;

          // Set buddy go!
          gfPlotPathToExitGrid = TRUE;
          EVENT_GetNewSoldierPath(curr->pSoldier, sGridNo, WALKING);
          gfPlotPathToExitGrid = FALSE;
        }
        ubNum++;
      } else {
        // We will remove them later....
      }
    }

    // ATE: Do another round, removing guys from group that can't go on...
  BEGINNING_LOOP:
    CFOR_EACH_PLAYER_IN_GROUP(curr, pGroup) {
      if (!OK_CONTROLLABLE_MERC(curr->pSoldier)) {
        RemoveCharacterFromSquads(curr->pSoldier);
        goto BEGINNING_LOOP;
      }
    }

    // OK, setup TacticalOverhead polling system that will notify us once
    // everybody has made it to our destination.
    const uint8_t action = (ubTacticalDirection == 255 ? WAIT_FOR_MERCS_TO_WALK_TO_GRIDNO
                                                       : WAIT_FOR_MERCS_TO_WALKOFF_SCREEN);
    SetActionToDoOnceMercsGetToLocation(action, ubNum);

    // Lock UI!
    guiPendingOverrideEvent = LU_BEGINUILOCK;
    HandleTacticalUI();
  }
}

void HandleSoldierLeavingSectorByThemSelf(SOLDIERTYPE *pSoldier) {
  // soldier leaving thier squad behind, will rejoin later
  // if soldier in a squad, set the fact they want to return here

  if (pSoldier->bAssignment < ON_DUTY) {
    RemoveCharacterFromSquads(pSoldier);  // REDUNDANT AddCharacterToUniqueSquad()

    // are they in a group?..remove from group
    if (pSoldier->ubGroupID != 0) {
      // remove from group
      RemovePlayerFromGroup(*pSoldier);
      pSoldier->ubGroupID = 0;
    }
  } else {
    // otherwise, they are on thier own, not in a squad, simply remove mvt group
    if (pSoldier->ubGroupID &&
        pSoldier->bAssignment != VEHICLE) {  // Can only remove groups if they aren't persistant
                                             // (not in a squad or vehicle)
      // delete group
      RemoveGroup(*GetGroup(pSoldier->ubGroupID));
      pSoldier->ubGroupID = 0;
    }
  }

  // set to guard
  AddCharacterToUniqueSquad(pSoldier);

  if (pSoldier->ubGroupID == 0) {
    // create independant group
    GROUP &g = *CreateNewPlayerGroupDepartingFromSector(pSoldier->sSectorX, pSoldier->sSectorY);
    AddPlayerToGroup(g, *pSoldier);
  }
}

static void DoneFadeOutExitGridSector();
static void HandlePotentialMoraleHitForSkimmingSectors(GROUP *pGroup);

void AllMercsWalkedToExitGrid() {
  BOOLEAN fDone;

  HandlePotentialMoraleHitForSkimmingSectors(gpAdjacentGroup);

  if (gubAdjacentJumpCode == JUMP_ALL_NO_LOAD || gubAdjacentJumpCode == JUMP_SINGLE_NO_LOAD) {
    Assert(gpAdjacentGroup);
    CFOR_EACH_PLAYER_IN_GROUP(pPlayer, gpAdjacentGroup) {
      SOLDIERTYPE &s = *pPlayer->pSoldier;
      SetInsertionDataFromAdjacentMoveDirection(s, gubTacticalDirection, gsAdditionalData);
      RemoveSoldierFromTacticalSector(s);
    }

    SetGroupSectorValue(gsAdjacentSectorX, gsAdjacentSectorY, gbAdjacentSectorZ, *gpAdjacentGroup);

    SetDefaultSquadOnSectorEntry(TRUE);

  } else {
    // Because we are actually loading the new map, and we are physically
    // traversing, we don't want to bring up the prebattle interface when we
    // arrive if there are enemies there.  This flag ignores the initialization
    // of the prebattle interface and clears the flag.
    gfTacticalTraversal = TRUE;
    gpTacticalTraversalGroup = gpAdjacentGroup;

    // Check for any unconcious and/or dead merc and remove them from the
    // current squad, so that they don't get moved to the new sector.
    fDone = FALSE;
    while (!fDone) {
      fDone = FALSE;
      const PLAYERGROUP *pPlayer = gpAdjacentGroup->pPlayerList;
      while (pPlayer) {
        if (pPlayer->pSoldier->bLife < OKLIFE) {
          AddCharacterToUniqueSquad(pPlayer->pSoldier);
          break;
        }
        pPlayer = pPlayer->next;
      }
      if (!pPlayer) {
        fDone = TRUE;
      }
    }

    // OK, Set insertion direction for all these guys....
    Assert(gpAdjacentGroup);
    CFOR_EACH_PLAYER_IN_GROUP(pPlayer, gpAdjacentGroup) {
      SetInsertionDataFromAdjacentMoveDirection(*pPlayer->pSoldier, gubTacticalDirection,
                                                gsAdditionalData);
    }
    SetGroupSectorValue(gsAdjacentSectorX, gsAdjacentSectorY, gbAdjacentSectorZ, *gpAdjacentGroup);

    gFadeOutDoneCallback = DoneFadeOutExitGridSector;
    FadeOutGameScreen();
  }
  if (!PlayerMercsInSector((uint8_t)gsAdjacentSectorX, (uint8_t)gsAdjacentSectorY,
                           (uint8_t)gbAdjacentSectorZ)) {
    HandleLoyaltyImplicationsOfMercRetreat(RETREAT_TACTICAL_TRAVERSAL, gsAdjacentSectorX,
                                           gsAdjacentSectorY, gbAdjacentSectorZ);
  }
  if (gubAdjacentJumpCode == JUMP_ALL_NO_LOAD || gubAdjacentJumpCode == JUMP_SINGLE_NO_LOAD) {
    gfTacticalTraversal = FALSE;
    gpTacticalTraversalGroup = NULL;
    gpTacticalTraversalChosenSoldier = NULL;
  }
}

static void SetupTacticalTraversalInformation() {
  int16_t sScreenX, sScreenY;

  Assert(gpAdjacentGroup);
  CFOR_EACH_PLAYER_IN_GROUP(pPlayer, gpAdjacentGroup) {
    SOLDIERTYPE &s = *pPlayer->pSoldier;

    SetInsertionDataFromAdjacentMoveDirection(s, gubTacticalDirection, gsAdditionalData);

    // pass flag that this is a tactical traversal, the path built MUST go in
    // the traversed direction even if longer!
    PlotPathForCharacter(s, gsAdjacentSectorX, gsAdjacentSectorY, true);

    if (guiAdjacentTraverseTime <= 5) {
      // Determine 'mirror' gridno...
      // Convert to absolute xy
      GetAbsoluteScreenXYFromMapPos(GETWORLDINDEXFROMWORLDCOORDS(s.sY, s.sX), &sScreenX, &sScreenY);

      // Get 'mirror', depending on what direction...
      switch (gubTacticalDirection) {
        case NORTH:
          sScreenY = 1520;
          break;
        case SOUTH:
          sScreenY = 0;
          break;
        case EAST:
          sScreenX = 0;
          break;
        case WEST:
          sScreenX = 3160;
          break;
      }

      // Convert into a gridno again.....
      const GridNo sNewGridNo = GetMapPosFromAbsoluteScreenXY(sScreenX, sScreenY);

      // Save this gridNo....
      s.sPendingActionData2 = sNewGridNo;
      // Copy CODe computed earlier into data
      s.usStrategicInsertionData = s.ubStrategicInsertionCode;
      // Now use NEW code....

      s.ubStrategicInsertionCode = CalcMapEdgepointClassInsertionCode(s.sPreTraversalGridNo);

      if (gubAdjacentJumpCode == JUMP_SINGLE_LOAD_NEW || gubAdjacentJumpCode == JUMP_ALL_LOAD_NEW) {
        fUsingEdgePointsForStrategicEntry = TRUE;
      }
    }
  }
  if (gubAdjacentJumpCode == JUMP_ALL_NO_LOAD || gubAdjacentJumpCode == JUMP_SINGLE_NO_LOAD) {
    gfTacticalTraversal = FALSE;
    gpTacticalTraversalGroup = NULL;
    gpTacticalTraversalChosenSoldier = NULL;
  }
}

static void DoneFadeOutAdjacentSector();

void AllMercsHaveWalkedOffSector() {
  BOOLEAN fEnemiesInLoadedSector = FALSE;

  if (NumEnemiesInAnySector(gWorldSectorX, gWorldSectorY, gbWorldSectorZ)) {
    fEnemiesInLoadedSector = TRUE;
  }

  if (fEnemiesInLoadedSector) {
    HandleLoyaltyImplicationsOfMercRetreat(RETREAT_TACTICAL_TRAVERSAL, gWorldSectorX, gWorldSectorY,
                                           gbWorldSectorZ);
  }

  // Setup strategic traversal information
  if (guiAdjacentTraverseTime <= 5) {
    gfTacticalTraversal = TRUE;
    gpTacticalTraversalGroup = gpAdjacentGroup;

    if (gbAdjacentSectorZ > 0 &&
        guiAdjacentTraverseTime <= 5) {  // Nasty strategic movement logic
                                         // doesn't like underground sectors!
      gfUndergroundTacticalTraversal = TRUE;
    }
  }
  ClearMercPathsAndWaypointsForAllInGroup(*gpAdjacentGroup);
  AddWaypointToPGroup(gpAdjacentGroup, (uint8_t)gsAdjacentSectorX, (uint8_t)gsAdjacentSectorY);
  if (gbAdjacentSectorZ > 0 && guiAdjacentTraverseTime <= 5) {  // Nasty strategic movement logic
                                                                // doesn't like underground sectors!
    gfUndergroundTacticalTraversal = TRUE;
  }

  SetupTacticalTraversalInformation();

  // ATE: Added here: donot load another screen if we were told not to....
  if ((gubAdjacentJumpCode == JUMP_ALL_NO_LOAD ||
       gubAdjacentJumpCode == JUMP_SINGLE_NO_LOAD)) {  // Case 1:  Group is leaving sector, but
                                                       // there are other mercs in sector and player
                                                       // wants to stay, or
    //         there are other mercs in sector while a battle is in progress.
    CFOR_EACH_PLAYER_IN_GROUP(pPlayer, gpAdjacentGroup) {
      RemoveSoldierFromTacticalSector(*pPlayer->pSoldier);
    }
    SetDefaultSquadOnSectorEntry(TRUE);
  } else {
    if (fEnemiesInLoadedSector) {  // We are retreating from a sector with
                                   // enemies in it and there are no mercs left so
      // warp the game time by 5 minutes to simulate the actual retreat.  This
      // restricts the player from immediately coming back to the same sector
      // they left to perhaps take advantage of the tactical placement gui to get
      // into better position.  Additionally, if there are any enemies in this
      // sector that are part of a movement group, reset that movement group so
      // that they are "in" the sector rather than 75% of the way to the next
      // sector if that is the case.
      ResetMovementForEnemyGroupsInLocation((uint8_t)gWorldSectorX, (uint8_t)gWorldSectorY);

      if (guiAdjacentTraverseTime > 5) {
        // Because this final group is retreating, simulate extra time to
        // retreat, so they can't immediately come back.
        WarpGameTime(300, WARPTIME_NO_PROCESSING_OF_EVENTS);
      }
    }
    if (guiAdjacentTraverseTime <= 5) {
      // Case 2:  Immediatly loading the next sector
      if (!gbAdjacentSectorZ) {
        uint32_t uiWarpTime;
        uiWarpTime = (GetWorldTotalMin() + 5) * 60 - GetWorldTotalSeconds();
        WarpGameTime(uiWarpTime, WARPTIME_PROCESS_TARGET_TIME_FIRST);
      } else if (gbAdjacentSectorZ > 0) {
        uint32_t uiWarpTime;
        uiWarpTime = (GetWorldTotalMin() + 1) * 60 - GetWorldTotalSeconds();
        WarpGameTime(uiWarpTime, WARPTIME_PROCESS_TARGET_TIME_FIRST);
      }

      // Because we are actually loading the new map, and we are physically
      // traversing, we don't want to bring up the prebattle interface when we
      // arrive if there are enemies there.  This flag ignores the initialization
      // of the prebattle interface and clears the flag.
      gFadeOutDoneCallback = DoneFadeOutAdjacentSector;
      FadeOutGameScreen();
    } else {  // Case 3:  Going directly to mapscreen

      // Lock game into mapscreen mode, but after the fade is done.
      gfEnteringMapScreen = TRUE;

      // ATE; Fade FAST....
      SetMusicFadeSpeed(5);
      SetMusicMode(MUSIC_TACTICAL_NOTHING);
    }
  }
}

static void DoneFadeOutExitGridSector() {
  SetCurrentWorldSector(gsAdjacentSectorX, gsAdjacentSectorY, gbAdjacentSectorZ);
  if (gfTacticalTraversal && gpTacticalTraversalGroup && gpTacticalTraversalChosenSoldier) {
    if (gTacticalStatus.fEnemyInSector) {
      TacticalCharacterDialogue(gpTacticalTraversalChosenSoldier, QUOTE_ENEMY_PRESENCE);
    }
  }
  gfTacticalTraversal = FALSE;
  gpTacticalTraversalGroup = NULL;
  gpTacticalTraversalChosenSoldier = NULL;
  FadeInGameScreen();
}

static int16_t PickGridNoToWalkIn(SOLDIERTYPE *pSoldier, uint8_t ubInsertionDirection,
                                  uint32_t *puiNumAttempts);

static void DoneFadeOutAdjacentSector() {
  uint8_t ubDirection;
  SetCurrentWorldSector(gsAdjacentSectorX, gsAdjacentSectorY, gbAdjacentSectorZ);

  ubDirection =
      GetStrategicInsertionDataFromAdjacentMoveDirection(gubTacticalDirection, gsAdditionalData);
  if (gfTacticalTraversal && gpTacticalTraversalGroup && gpTacticalTraversalChosenSoldier) {
    if (gTacticalStatus.fEnemyInSector) {
      TacticalCharacterDialogue(gpTacticalTraversalChosenSoldier, QUOTE_ENEMY_PRESENCE);
    }
  }
  gfTacticalTraversal = FALSE;
  gpTacticalTraversalGroup = NULL;
  gpTacticalTraversalChosenSoldier = NULL;

  if (gfCaves) {
    // ATE; Set tactical status flag...
    gTacticalStatus.uiFlags |= IGNORE_ALL_OBSTACLES;
    // Set pathing flag to path through anything....
    gfPathAroundObstacles = FALSE;
  }

  // OK, give our guys new orders...
  if (gpAdjacentGroup->fPlayer) {
    // For player groups, update the soldier information
    uint32_t uiAttempts;
    int16_t sGridNo, sOldGridNo;
    uint8_t ubNum = 0;
    CFOR_EACH_PLAYER_IN_GROUP(curr, gpAdjacentGroup) {
      if (curr->pSoldier->sGridNo != NOWHERE) {
        sGridNo = PickGridNoToWalkIn(curr->pSoldier, ubDirection, &uiAttempts);

        // If the search algorithm failed due to too many attempts, simply reset
        // the the gridno as the destination is a reserved gridno and we will
        // place the merc there without walking into the sector.
        if (sGridNo == NOWHERE && uiAttempts == MAX_ATTEMPTS) {
          sGridNo = curr->pSoldier->sGridNo;
        }

        if (sGridNo != NOWHERE) {
          curr->pSoldier->ubWaitActionToDo = 1;
          // OK, here we have been given a position, a gridno has been given to
          // use as well....
          sOldGridNo = curr->pSoldier->sGridNo;
          EVENT_SetSoldierPosition(curr->pSoldier, sGridNo, SSP_NONE);
          if (sGridNo != sOldGridNo) {
            EVENT_GetNewSoldierPath(curr->pSoldier, sOldGridNo, WALKING);
          }
          ubNum++;
        }
      } else {
      }
    }
    SetActionToDoOnceMercsGetToLocation(WAIT_FOR_MERCS_TO_WALKON_SCREEN, ubNum);
    guiPendingOverrideEvent = LU_BEGINUILOCK;
    HandleTacticalUI();

    // Unset flag here.....
    gfPathAroundObstacles = TRUE;
  }
  FadeInGameScreen();
}

static BOOLEAN SoldierOKForSectorExit(SOLDIERTYPE *pSoldier, int8_t bExitDirection,
                                      uint16_t usAdditionalData) {
  int16_t sWorldX;
  int16_t sWorldY;

  // if the soldiers gridno is not NOWHERE
  if (pSoldier->sGridNo == NOWHERE) return (FALSE);

  // OK, anyone on roofs cannot!
  if (pSoldier->bLevel > 0) return (FALSE);

  // Get screen coordinates for current position of soldier
  GetAbsoluteScreenXYFromMapPos(pSoldier->sGridNo, &sWorldX, &sWorldY);

  // Check direction
  switch (bExitDirection) {
    case EAST_STRATEGIC_MOVE:

      if (sWorldX < ((gsTRX - gsTLX) - CHECK_DIR_X_DELTA)) {
        // NOT OK, return FALSE
        return (FALSE);
      }
      break;

    case WEST_STRATEGIC_MOVE:

      if (sWorldX > CHECK_DIR_X_DELTA) {
        // NOT OK, return FALSE
        return (FALSE);
      }
      break;

    case SOUTH_STRATEGIC_MOVE:

      if (sWorldY < ((gsBLY - gsTRY) - CHECK_DIR_Y_DELTA)) {
        // NOT OK, return FALSE
        return (FALSE);
      }
      break;

    case NORTH_STRATEGIC_MOVE:

      if (sWorldY > CHECK_DIR_Y_DELTA) {
        // NOT OK, return FALSE
        return (FALSE);
      }
      break;

      // This case is for an exit grid....
      // check if we are close enough.....

    case -1:

      // FOR REALTIME - DO MOVEMENT BASED ON STANCE!
      if ((gTacticalStatus.uiFlags & REALTIME) || !(gTacticalStatus.uiFlags & INCOMBAT)) {
        pSoldier->usUIMovementMode =
            GetMoveStateBasedOnStance(pSoldier, gAnimControl[pSoldier->usAnimState].ubEndHeight);
      }

      const int16_t sGridNo =
          FindGridNoFromSweetSpotCloseToExitGrid(pSoldier, usAdditionalData, 10);
      if (sGridNo == NOWHERE) {
        return (FALSE);
      }

      // ATE: if we are in combat, get cost to move here....
      if (gTacticalStatus.uiFlags & INCOMBAT) {
        // Turn off at end of function...
        const int16_t sAPs = PlotPath(pSoldier, sGridNo, NO_COPYROUTE, NO_PLOT,
                                      pSoldier->usUIMovementMode, pSoldier->bActionPoints);
        if (!EnoughPoints(pSoldier, sAPs, 0, FALSE)) {
          return (FALSE);
        }
      }
      break;
  }
  return (TRUE);
}

// ATE: Returns FALSE if NOBODY is close enough, 1 if ONLY selected guy is and 2
// if all on squad are...
BOOLEAN OKForSectorExit(int8_t bExitDirection, uint16_t usAdditionalData,
                        uint32_t *puiTraverseTimeInMinutes) {
  BOOLEAN fAtLeastOneMercControllable = FALSE;
  BOOLEAN fOnlySelectedGuy = FALSE;
  SOLDIERTYPE *pValidSoldier = NULL;
  uint8_t ubReturnVal = FALSE;
  uint8_t ubNumControllableMercs = 0;
  uint8_t ubNumMercs = 0, ubNumEPCs = 0;
  uint8_t ubPlayerControllableMercsInSquad = 0;

  const SOLDIERTYPE *const sel = GetSelectedMan();
  // must have a selected soldier to be allowed to tactically traverse.
  if (sel == NULL) return FALSE;

  /*
  //Exception code for the two sectors in San Mona that are separated by a
  cliff.  We want to allow strategic
  //traversal, but NOT tactical traversal.  The only way to tactically go from
  D4 to D5 (or viceversa) is to enter
  //the cave entrance.
  if( gWorldSectorX == 4 && gWorldSectorY == 4 && !gbWorldSectorZ &&
  bExitDirection == EAST_STRATEGIC_MOVE )
  {
          gfInvalidTraversal = TRUE;
          return FALSE;
  }
  if( gWorldSectorX == 5 && gWorldSectorY == 4 && !gbWorldSectorZ &&
  bExitDirection == WEST_STRATEGIC_MOVE )
  {
          gfInvalidTraversal = TRUE;
          return FALSE;
  }
  */

  gfInvalidTraversal = FALSE;
  gfLoneEPCAttemptingTraversal = FALSE;
  gubLoneMercAttemptingToAbandonEPCs = 0;
  gPotentiallyAbandonedEPC = NULL;

  // Look through all mercs and check if they are within range of east end....
  FOR_EACH_IN_TEAM(pSoldier, OUR_TEAM) {
    // If we are controllable
    if (OkControllableMerc(pSoldier) && pSoldier->bAssignment == CurrentSquad()) {
      // Need to keep a copy of a good soldier, so we can access it later, and
      // not more than once.
      pValidSoldier = pSoldier;

      ubNumControllableMercs++;

      // We need to keep track of the number of EPCs and mercs in this squad. If
      // we have only one merc and one or more EPCs, then we can't allow the merc
      // to tactically traverse, if he is the only merc near enough to traverse.
      if (AM_AN_EPC(pSoldier)) {
        ubNumEPCs++;
        // Also record the EPC's slot ID incase we later build a string using
        // the EPC's name.
        gPotentiallyAbandonedEPC = pSoldier;
        if (AM_A_ROBOT(pSoldier) && !CanRobotBeControlled(pSoldier)) {
          gfRobotWithoutControllerAttemptingTraversal = TRUE;
          ubNumControllableMercs--;
          continue;
        }
      } else {
        ubNumMercs++;
      }

      if (SoldierOKForSectorExit(pSoldier, bExitDirection, usAdditionalData)) {
        fAtLeastOneMercControllable++;

        if (pSoldier == sel) fOnlySelectedGuy = TRUE;
      } else {
        GROUP *pGroup;

        // ATE: Dont's assume exit grids here...
        if (bExitDirection != -1) {
          // Now, determine if this is a valid path.
          pGroup = GetGroup(pValidSoldier->ubGroupID);
          AssertMsg(pGroup, String("%ls is not in a valid group (pSoldier->ubGroupID is %d)",
                                   pValidSoldier->name, pValidSoldier->ubGroupID));
          uint32_t traverse_time = TRAVERSE_TIME_IMPOSSIBLE;
          if (!gbWorldSectorZ) {
            traverse_time = GetSectorMvtTimeForGroup(
                (uint8_t)SECTOR(pGroup->ubSectorX, pGroup->ubSectorY), bExitDirection, pGroup);
          } else if (gbWorldSectorZ > 1) {  // We are attempting to traverse in an underground
                                            // environment.  We need to use a complete different
            // method.  When underground, all sectors are instantly adjacent.
            traverse_time = UndergroundTacticalTraversalTime(bExitDirection);
          }
          if (puiTraverseTimeInMinutes) *puiTraverseTimeInMinutes = traverse_time;
          if (traverse_time == TRAVERSE_TIME_IMPOSSIBLE) {
            gfInvalidTraversal = TRUE;
            return FALSE;
          }
        } else {
          // Exit grid travel is instantaneous
          if (puiTraverseTimeInMinutes) *puiTraverseTimeInMinutes = 0;
        }
      }
    }
  }

  // If we are here, at least one guy is controllable in this sector, at least
  // he can go!
  if (fAtLeastOneMercControllable) {
    ubPlayerControllableMercsInSquad =
        (uint8_t)NumberOfPlayerControllableMercsInSquad(sel->bAssignment);
    if (fAtLeastOneMercControllable <=
        ubPlayerControllableMercsInSquad) {  // if the selected merc is an EPC
                                             // and we can only leave with that
                                             // merc, then prevent it
      // as EPCs aren't allowed to leave by themselves.  Instead of restricting
      // this in the exiting sector gui, we restrict it by explaining it with a
      // message box.
      if (AM_AN_EPC(sel)) {
        if (fAtLeastOneMercControllable < ubPlayerControllableMercsInSquad ||
            fAtLeastOneMercControllable == 1) {
          gfLoneEPCAttemptingTraversal = TRUE;
          return FALSE;
        }
      } else {  // We previously counted the number of EPCs and mercs, and if the
                // selected merc is not an EPC and there are no
        // other mercs in the squad able to escort the EPCs, we will prohibit
        // this merc from tactically traversing.
        if (ubNumEPCs && ubNumMercs == 1 &&
            fAtLeastOneMercControllable < ubPlayerControllableMercsInSquad) {
          gubLoneMercAttemptingToAbandonEPCs = ubNumEPCs;
          return FALSE;
        }
      }
    }
    if (bExitDirection != -1) {
      GROUP *pGroup;
      // Now, determine if this is a valid path.
      pGroup = GetGroup(pValidSoldier->ubGroupID);
      AssertMsg(pGroup, String("%ls is not in a valid group (pSoldier->ubGroupID is %d)",
                               pValidSoldier->name, pValidSoldier->ubGroupID));
      uint32_t traverse_time;
      if (!gbWorldSectorZ) {
        traverse_time = GetSectorMvtTimeForGroup(
            (uint8_t)SECTOR(pGroup->ubSectorX, pGroup->ubSectorY), bExitDirection, pGroup);
      } else if (gbWorldSectorZ > 0) {  // We are attempting to traverse in an underground
                                        // environment.  We need to use a complete different
        // method.  When underground, all sectors are instantly adjacent.
        traverse_time = UndergroundTacticalTraversalTime(bExitDirection);
      }
      if (puiTraverseTimeInMinutes) *puiTraverseTimeInMinutes = traverse_time;
      if (traverse_time == TRAVERSE_TIME_IMPOSSIBLE) {
        gfInvalidTraversal = TRUE;
        ubReturnVal = FALSE;
      } else {
        ubReturnVal = TRUE;
      }
    } else {
      ubReturnVal = TRUE;
      // Exit grid travel is instantaneous
      if (puiTraverseTimeInMinutes) *puiTraverseTimeInMinutes = 0;
    }
  }

  if (ubReturnVal) {
    // Default to FALSE again, until we see that we have
    ubReturnVal = FALSE;

    if (fAtLeastOneMercControllable) {
      // Do we contain the selected guy?
      if (fOnlySelectedGuy) {
        ubReturnVal = 1;
      }
      // Is the whole squad able to go here?
      if (fAtLeastOneMercControllable == ubPlayerControllableMercsInSquad) {
        ubReturnVal = 2;
      }
    }
  }

  return (ubReturnVal);
}

void SetupNewStrategicGame() {
  // Set all sectors as enemy controlled.
  FOR_EACH(StrategicMapElement, i, StrategicMap) { i->fEnemyControlled = TRUE; }

  InitNewGameClock();
  DeleteAllStrategicEvents();

  // Set up all events that get processed daily.
  BuildDayLightLevels();
  // Check for quests each morning.
  AddEveryDayStrategicEvent(EVENT_CHECKFORQUESTS, QUEST_CHECK_EVENT_TIME, 0);
  // Some things get updated in the very early morning.
  AddEveryDayStrategicEvent(EVENT_DAILY_EARLY_MORNING_EVENTS, EARLY_MORNING_TIME, 0);
  // Daily update BobbyRay Inventory.
  AddEveryDayStrategicEvent(EVENT_DAILY_UPDATE_BOBBY_RAY_INVENTORY, BOBBYRAY_UPDATE_TIME, 0);
  // Daily update of the M.E.R.C. site..
  AddEveryDayStrategicEvent(EVENT_DAILY_UPDATE_OF_MERC_SITE, 0, 0);
  // Daily update of insured mercs.
  AddEveryDayStrategicEvent(EVENT_HANDLE_INSURED_MERCS, INSURANCE_UPDATE_TIME, 0);
  // Daily update of mercs.
  AddEveryDayStrategicEvent(EVENT_MERC_DAILY_UPDATE, 0, 0);
  // Daily mine production processing events.
  AddEveryDayStrategicEvent(EVENT_SETUP_MINE_INCOME, 0, 0);
  // Daily checks for E-mail from Enrico.
  AddEveryDayStrategicEvent(EVENT_ENRICO_MAIL, ENRICO_MAIL_TIME, 0);

  // Hourly update of all sorts of things
  AddPeriodStrategicEvent(EVENT_HOURLY_UPDATE, 60, 0);
  AddPeriodStrategicEvent(EVENT_QUARTER_HOUR_UPDATE, 15, 0);

  // Clear any possible battle locator.
  gfBlitBattleSectorLocator = FALSE;

  StrategicTurnsNewGame();
}

// a -1 will be returned upon failure
int8_t GetSAMIdFromSector(int16_t sSectorX, int16_t sSectorY, int8_t bSectorZ) {
  int8_t bCounter = 0;
  int16_t sSectorValue = 0;

  // check if valid sector
  if (bSectorZ != 0) {
    return (-1);
  }

  // get the sector value
  sSectorValue = SECTOR(sSectorX, sSectorY);

  // run through list of sam sites
  for (bCounter = 0; bCounter < 4; bCounter++) {
    if (pSamList[bCounter] == sSectorValue) {
      return (bCounter);
    }
  }

  return (-1);
}

bool CanGoToTacticalInSector(int16_t const x, int16_t const y, uint8_t const z) {
  // If not a valid sector
  if (x < 1 || 16 < x) return false;
  if (y < 1 || 16 < x) return false;
  if (3 < z) return false;

  /* Look for all living, fighting mercs on player's team. Robot and EPCs
   * qualify! */
  CFOR_EACH_IN_TEAM(i, OUR_TEAM) {
    SOLDIERTYPE const &s = *i;
    /* ARM: Now allows loading of sector with all mercs below OKLIFE as long as
     * they're alive */
    if (s.bLife == 0) continue;
    if (s.uiStatusFlags & SOLDIER_VEHICLE) continue;
    if (s.bAssignment == IN_TRANSIT) continue;
    if (s.bAssignment == ASSIGNMENT_POW) continue;
    if (s.bAssignment == ASSIGNMENT_DEAD) continue;
    if (SoldierAboardAirborneHeli(s)) continue;
    if (s.fBetweenSectors) continue;
    if (s.sSectorX != x) continue;
    if (s.sSectorY != y) continue;
    if (s.bSectorZ != z) continue;
    return true;
  }

  return false;
}

int32_t GetNumberOfSAMSitesUnderPlayerControl() {
  int32_t n = 0;
  FOR_EACH(int16_t const, i, pSamList) {
    if (!StrategicMap[SECTOR_INFO_TO_STRATEGIC_INDEX(*i)].fEnemyControlled) ++n;
  }
  return n;
}

void UpdateAirspaceControl() {
  int32_t iCounterA = 0, iCounterB = 0;
  uint8_t ubControllingSAM;
  StrategicMapElement *pSAMStrategicMap = NULL;
  BOOLEAN fEnemyControlsAir;

  for (iCounterA = 1; iCounterA < (int32_t)(MAP_WORLD_X - 1); iCounterA++) {
    for (iCounterB = 1; iCounterB < (int32_t)(MAP_WORLD_Y - 1); iCounterB++) {
      // IMPORTANT: B and A are reverse here, since the table is stored
      // transposed
      ubControllingSAM = ubSAMControlledSectors[iCounterB][iCounterA];

      if ((ubControllingSAM >= 1) && (ubControllingSAM <= NUMBER_OF_SAMS)) {
        pSAMStrategicMap =
            &(StrategicMap[SECTOR_INFO_TO_STRATEGIC_INDEX(pSamList[ubControllingSAM - 1])]);

        // if the enemies own the controlling SAM site, and it's in working
        // condition
        if ((pSAMStrategicMap->fEnemyControlled) &&
            (pSAMStrategicMap->bSAMCondition >= MIN_CONDITION_FOR_SAM_SITE_TO_WORK)) {
          fEnemyControlsAir = TRUE;
        } else {
          fEnemyControlsAir = FALSE;
        }
      } else {
        // no controlling SAM site
        fEnemyControlsAir = FALSE;
      }

      StrategicMap[CALCULATE_STRATEGIC_INDEX(iCounterA, iCounterB)].fEnemyAirControlled =
          fEnemyControlsAir;
    }
  }

  // check if currently selected arrival sector still has secure airspace

  // if it's not enemy air controlled
  if (StrategicMap[SECTOR_INFO_TO_STRATEGIC_INDEX(g_merc_arrive_sector)].fEnemyAirControlled) {
    // NOPE!
    wchar_t sMsgString[256], sMsgSubString1[64], sMsgSubString2[64];

    // get the name of the old sector
    GetSectorIDString(SECTORX(g_merc_arrive_sector), SECTORY(g_merc_arrive_sector), 0,
                      sMsgSubString1, lengthof(sMsgSubString1), FALSE);

    // Move the landing zone over to the start sector.
    g_merc_arrive_sector = START_SECTOR;

    // get the name of the new sector
    GetSectorIDString(SECTORX(g_merc_arrive_sector), SECTORY(g_merc_arrive_sector), 0,
                      sMsgSubString2, lengthof(sMsgSubString2), FALSE);

    // now build the string
    swprintf(sMsgString, lengthof(sMsgString), pBullseyeStrings[4], sMsgSubString1, sMsgSubString2);

    // confirm the change with overlay message
    DoScreenIndependantMessageBox(sMsgString, MSG_BOX_FLAG_OK, NULL);

    // update position of bullseye
    fMapPanelDirty = TRUE;

    // update destination column for any mercs in transit
    fTeamPanelDirty = TRUE;
  }

  // ARM: airspace control now affects refueling site availability, so update
  // that too with every change!
  UpdateRefuelSiteAvailability();
}

bool IsThereAFunctionalSAMSiteInSector(int16_t const x, int16_t const y, int8_t const z) {
  return IsThisSectorASAMSector(x, y, z) &&
         StrategicMap[CALCULATE_STRATEGIC_INDEX(x, y)].bSAMCondition >=
             MIN_CONDITION_FOR_SAM_SITE_TO_WORK;
}

bool IsThisSectorASAMSector(int16_t const x, int16_t const y, int8_t const z) {
  if (z != 0) return false;

  FOR_EACH(int16_t const, i, pSamList) {
    if (*i == SECTOR(x, y)) return true;
  }
  return false;
}

void SaveStrategicInfoToSavedFile(HWFILE const f) {
  // Save the strategic map information
  FOR_EACH(StrategicMapElement const, i, StrategicMap) { InjectStrategicMapElementIntoFile(f, *i); }

  // Save the Sector Info
  FOR_EACH(SECTORINFO const, i, SectorInfo) { InjectSectorInfoIntoFile(f, *i); }

  // Skip the SAM controlled sector information
  FileSeek(f, MAP_WORLD_X * MAP_WORLD_Y, FILE_SEEK_FROM_CURRENT);

  // Save fFoundOrta
  FileWrite(f, &fFoundOrta, sizeof(BOOLEAN));
}

void LoadStrategicInfoFromSavedFile(HWFILE const f) {
  // Load the strategic map information
  FOR_EACH(StrategicMapElement, i, StrategicMap) { ExtractStrategicMapElementFromFile(f, *i); }

  // Load the Sector Info
  FOR_EACH(SECTORINFO, i, SectorInfo) { ExtractSectorInfoFromFile(f, *i); }

  // Skip the SAM controlled sector information
  FileSeek(f, MAP_WORLD_X * MAP_WORLD_Y, FILE_SEEK_FROM_CURRENT);

  // Load fFoundOrta
  FileRead(f, &fFoundOrta, sizeof(BOOLEAN));
}

static int16_t PickGridNoNearestEdge(SOLDIERTYPE *pSoldier, uint8_t ubTacticalDirection) {
  int16_t sGridNo, sStartGridNo, sOldGridNo;
  int8_t bOdd = 1, bOdd2 = 1;
  uint8_t bAdjustedDist = 0;
  uint32_t cnt;

  switch (ubTacticalDirection) {
    case EAST:

      sGridNo = pSoldier->sGridNo;
      sStartGridNo = pSoldier->sGridNo;
      sOldGridNo = pSoldier->sGridNo;

      // Move directly to the right!
      while (GridNoOnVisibleWorldTile(sGridNo)) {
        sOldGridNo = sGridNo;

        if (bOdd) {
          sGridNo -= WORLD_COLS;
        } else {
          sGridNo++;
        }

        bOdd = (int8_t)!bOdd;
      }

      sGridNo = sOldGridNo;
      sStartGridNo = sOldGridNo;

      do {
        // OK, here we go back one, check for OK destination...
        if (NewOKDestination(pSoldier, sGridNo, TRUE, pSoldier->bLevel) &&
            FindBestPath(pSoldier, sGridNo, pSoldier->bLevel, WALKING, NO_COPYROUTE,
                         PATH_THROUGH_PEOPLE)) {
          return (sGridNo);
        }

        // If here, try another place!
        // ( alternate up/down )
        if (bOdd2) {
          bAdjustedDist++;

          sGridNo = sStartGridNo;

          for (cnt = 0; cnt < bAdjustedDist; cnt++) {
            sGridNo = (int16_t)(sGridNo - WORLD_COLS - 1);
          }
        } else {
          sGridNo = sStartGridNo;

          for (cnt = 0; cnt < bAdjustedDist; cnt++) {
            sGridNo = (int16_t)(sGridNo + WORLD_COLS + 1);
          }
        }

        bOdd2 = (int8_t)(!bOdd2);

      } while (TRUE);

    case WEST:

      sGridNo = pSoldier->sGridNo;
      sStartGridNo = pSoldier->sGridNo;
      sOldGridNo = pSoldier->sGridNo;

      // Move directly to the left!
      while (GridNoOnVisibleWorldTile(sGridNo)) {
        sOldGridNo = sGridNo;

        if (bOdd) {
          sGridNo += WORLD_COLS;
        } else {
          sGridNo--;
        }

        bOdd = (int8_t)!bOdd;
      }

      sGridNo = sOldGridNo;
      sStartGridNo = sOldGridNo;

      do {
        // OK, here we go back one, check for OK destination...
        if (NewOKDestination(pSoldier, sGridNo, TRUE, pSoldier->bLevel) &&
            FindBestPath(pSoldier, sGridNo, pSoldier->bLevel, WALKING, NO_COPYROUTE,
                         PATH_THROUGH_PEOPLE)) {
          return (sGridNo);
        }

        // If here, try another place!
        // ( alternate up/down )
        if (bOdd2) {
          bAdjustedDist++;

          sGridNo = sStartGridNo;

          for (cnt = 0; cnt < bAdjustedDist; cnt++) {
            sGridNo = (int16_t)(sGridNo - WORLD_COLS - 1);
          }
        } else {
          sGridNo = sStartGridNo;

          for (cnt = 0; cnt < bAdjustedDist; cnt++) {
            sGridNo = (int16_t)(sGridNo + WORLD_COLS + 1);
          }
        }

        bOdd2 = (int8_t)(!bOdd2);

      } while (TRUE);

    case NORTH:

      sGridNo = pSoldier->sGridNo;
      sStartGridNo = pSoldier->sGridNo;
      sOldGridNo = pSoldier->sGridNo;

      // Move directly to the left!
      while (GridNoOnVisibleWorldTile(sGridNo)) {
        sOldGridNo = sGridNo;

        if (bOdd) {
          sGridNo -= WORLD_COLS;
        } else {
          sGridNo--;
        }

        bOdd = (int8_t)(!bOdd);
      }

      sGridNo = sOldGridNo;
      sStartGridNo = sOldGridNo;

      do {
        // OK, here we go back one, check for OK destination...
        if (NewOKDestination(pSoldier, sGridNo, TRUE, pSoldier->bLevel) &&
            FindBestPath(pSoldier, sGridNo, pSoldier->bLevel, WALKING, NO_COPYROUTE,
                         PATH_THROUGH_PEOPLE)) {
          return (sGridNo);
        }

        // If here, try another place!
        // ( alternate left/right )
        if (bOdd2) {
          bAdjustedDist++;

          sGridNo = sStartGridNo;

          for (cnt = 0; cnt < bAdjustedDist; cnt++) {
            sGridNo = (int16_t)(sGridNo + WORLD_COLS - 1);
          }
        } else {
          sGridNo = sStartGridNo;

          for (cnt = 0; cnt < bAdjustedDist; cnt++) {
            sGridNo = (int16_t)(sGridNo - WORLD_COLS + 1);
          }
        }

        bOdd2 = (int8_t)(!bOdd2);

      } while (TRUE);

    case SOUTH:

      sGridNo = pSoldier->sGridNo;
      sStartGridNo = pSoldier->sGridNo;
      sOldGridNo = pSoldier->sGridNo;

      // Move directly to the left!
      while (GridNoOnVisibleWorldTile(sGridNo)) {
        sOldGridNo = sGridNo;

        if (bOdd) {
          sGridNo += WORLD_COLS;
        } else {
          sGridNo++;
        }

        bOdd = (int8_t)(!bOdd);
      }

      sGridNo = sOldGridNo;
      sStartGridNo = sOldGridNo;

      do {
        // OK, here we go back one, check for OK destination...
        if (NewOKDestination(pSoldier, sGridNo, TRUE, pSoldier->bLevel) &&
            FindBestPath(pSoldier, sGridNo, pSoldier->bLevel, WALKING, NO_COPYROUTE,
                         PATH_THROUGH_PEOPLE)) {
          return (sGridNo);
        }

        // If here, try another place!
        // ( alternate left/right )
        if (bOdd2) {
          bAdjustedDist++;

          sGridNo = sStartGridNo;

          for (cnt = 0; cnt < bAdjustedDist; cnt++) {
            sGridNo = (int16_t)(sGridNo + WORLD_COLS - 1);
          }
        } else {
          sGridNo = sStartGridNo;

          for (cnt = 0; cnt < bAdjustedDist; cnt++) {
            sGridNo = (int16_t)(sGridNo - WORLD_COLS + 1);
          }
        }

        bOdd2 = (int8_t)(!bOdd2);

      } while (TRUE);
  }

  return (NOWHERE);
}

void AdjustSoldierPathToGoOffEdge(SOLDIERTYPE *pSoldier, int16_t sEndGridNo,
                                  uint8_t ubTacticalDirection) {
  int16_t sNewGridNo, sTempGridNo;
  int32_t iLoop;

  // will this path segment actually take us to our desired destination in the
  // first place?
  if (pSoldier->usPathDataSize + 2 > MAX_PATH_LIST_SIZE) {
    sTempGridNo = pSoldier->sGridNo;

    for (iLoop = 0; iLoop < pSoldier->usPathDataSize; iLoop++) {
      sTempGridNo += (int16_t)DirectionInc(pSoldier->usPathingData[iLoop]);
    }

    if (sTempGridNo == sEndGridNo) {
      // we can make it, but there isn't enough path room for the two steps
      // required. truncate our path so there's guaranteed the merc will have to
      // generate another path later on...
      pSoldier->usPathDataSize -= 4;
      return;
    } else {
      // can't even make it there with these 30 tiles of path, abort...
      return;
    }
  }

  switch (ubTacticalDirection) {
    case EAST:

      sNewGridNo = NewGridNo((uint16_t)sEndGridNo, (uint16_t)DirectionInc((uint8_t)NORTHEAST));

      if (OutOfBounds(sEndGridNo, sNewGridNo)) {
        return;
      }

      pSoldier->usPathingData[pSoldier->usPathDataSize] = NORTHEAST;
      pSoldier->usPathDataSize++;
      pSoldier->sFinalDestination = sNewGridNo;
      pSoldier->usActionData = sNewGridNo;

      sTempGridNo = NewGridNo((uint16_t)sNewGridNo, (uint16_t)DirectionInc((uint8_t)NORTHEAST));

      if (OutOfBounds(sNewGridNo, sTempGridNo)) {
        return;
      }
      sNewGridNo = sTempGridNo;

      pSoldier->usPathingData[pSoldier->usPathDataSize] = NORTHEAST;
      pSoldier->usPathDataSize++;
      pSoldier->sFinalDestination = sNewGridNo;
      pSoldier->usActionData = sNewGridNo;

      break;

    case WEST:

      sNewGridNo = NewGridNo((uint16_t)sEndGridNo, (uint16_t)DirectionInc((uint8_t)SOUTHWEST));

      if (OutOfBounds(sEndGridNo, sNewGridNo)) {
        return;
      }

      pSoldier->usPathingData[pSoldier->usPathDataSize] = SOUTHWEST;
      pSoldier->usPathDataSize++;
      pSoldier->sFinalDestination = sNewGridNo;
      pSoldier->usActionData = sNewGridNo;

      sTempGridNo = NewGridNo((uint16_t)sNewGridNo, (uint16_t)DirectionInc((uint8_t)SOUTHWEST));

      if (OutOfBounds(sNewGridNo, sTempGridNo)) {
        return;
      }
      sNewGridNo = sTempGridNo;

      pSoldier->usPathingData[pSoldier->usPathDataSize] = SOUTHWEST;
      pSoldier->usPathDataSize++;
      pSoldier->sFinalDestination = sNewGridNo;
      pSoldier->usActionData = sNewGridNo;
      break;

    case NORTH:

      sNewGridNo = NewGridNo((uint16_t)sEndGridNo, (uint16_t)DirectionInc((uint8_t)NORTHWEST));

      if (OutOfBounds(sEndGridNo, sNewGridNo)) {
        return;
      }

      pSoldier->usPathingData[pSoldier->usPathDataSize] = NORTHWEST;
      pSoldier->usPathDataSize++;
      pSoldier->sFinalDestination = sNewGridNo;
      pSoldier->usActionData = sNewGridNo;

      sTempGridNo = NewGridNo((uint16_t)sNewGridNo, (uint16_t)DirectionInc((uint8_t)NORTHWEST));

      if (OutOfBounds(sNewGridNo, sTempGridNo)) {
        return;
      }
      sNewGridNo = sTempGridNo;

      pSoldier->usPathingData[pSoldier->usPathDataSize] = NORTHWEST;
      pSoldier->usPathDataSize++;
      pSoldier->sFinalDestination = sNewGridNo;
      pSoldier->usActionData = sNewGridNo;

      break;

    case SOUTH:

      sNewGridNo = NewGridNo((uint16_t)sEndGridNo, (uint16_t)DirectionInc((uint8_t)SOUTHEAST));

      if (OutOfBounds(sEndGridNo, sNewGridNo)) {
        return;
      }

      pSoldier->usPathingData[pSoldier->usPathDataSize] = SOUTHEAST;
      pSoldier->usPathDataSize++;
      pSoldier->sFinalDestination = sNewGridNo;
      pSoldier->usActionData = sNewGridNo;

      sTempGridNo = NewGridNo((uint16_t)sNewGridNo, (uint16_t)DirectionInc((uint8_t)SOUTHEAST));

      if (OutOfBounds(sNewGridNo, sTempGridNo)) {
        return;
      }
      sNewGridNo = sTempGridNo;

      pSoldier->usPathingData[pSoldier->usPathDataSize] = SOUTHEAST;
      pSoldier->usPathDataSize++;
      pSoldier->sFinalDestination = sNewGridNo;
      pSoldier->usActionData = sNewGridNo;
      break;
  }
}

static int16_t PickGridNoToWalkIn(SOLDIERTYPE *pSoldier, uint8_t ubInsertionDirection,
                                  uint32_t *puiNumAttempts) {
  int16_t sGridNo, sStartGridNo, sOldGridNo;
  int8_t bOdd = 1, bOdd2 = 1;
  uint8_t bAdjustedDist = 0;
  uint32_t cnt;

  *puiNumAttempts = 0;

  switch (ubInsertionDirection) {
    // OK, we're given a direction on visible map, let's look for the first oone
    // we find that is just on the start of visible map...
    case INSERTION_CODE_WEST:

      sGridNo = (int16_t)pSoldier->sGridNo;
      sStartGridNo = (int16_t)pSoldier->sGridNo;
      sOldGridNo = (int16_t)pSoldier->sGridNo;

      // Move directly to the left!
      while (GridNoOnVisibleWorldTile(sGridNo)) {
        sOldGridNo = sGridNo;

        if (bOdd) {
          sGridNo += WORLD_COLS;
        } else {
          sGridNo--;
        }

        bOdd = (int8_t)(!bOdd);
      }

      sGridNo = sOldGridNo;
      sStartGridNo = sOldGridNo;

      while (*puiNumAttempts < MAX_ATTEMPTS) {
        (*puiNumAttempts)++;
        // OK, here we go back one, check for OK destination...
        if ((gTacticalStatus.uiFlags & IGNORE_ALL_OBSTACLES) ||
            (NewOKDestination(pSoldier, sGridNo, TRUE, pSoldier->bLevel) &&
             FindBestPath(pSoldier, sGridNo, pSoldier->bLevel, WALKING, NO_COPYROUTE,
                          PATH_THROUGH_PEOPLE))) {
          return (sGridNo);
        }

        // If here, try another place!
        // ( alternate up/down )
        if (bOdd2) {
          bAdjustedDist++;

          sGridNo = sStartGridNo;

          for (cnt = 0; cnt < bAdjustedDist; cnt++) {
            sGridNo = (int16_t)(sGridNo - WORLD_COLS - 1);
          }
        } else {
          sGridNo = sStartGridNo;

          for (cnt = 0; cnt < bAdjustedDist; cnt++) {
            sGridNo = (int16_t)(sGridNo + WORLD_COLS + 1);
          }
        }

        bOdd2 = (int8_t)(!bOdd2);
      }
      return NOWHERE;

    case INSERTION_CODE_EAST:

      sGridNo = (int16_t)pSoldier->sGridNo;
      sStartGridNo = (int16_t)pSoldier->sGridNo;
      sOldGridNo = (int16_t)pSoldier->sGridNo;

      // Move directly to the right!
      while (GridNoOnVisibleWorldTile(sGridNo)) {
        sOldGridNo = sGridNo;

        if (bOdd) {
          sGridNo -= WORLD_COLS;
        } else {
          sGridNo++;
        }

        bOdd = (int8_t)(!bOdd);
      }

      sGridNo = sOldGridNo;
      sStartGridNo = sOldGridNo;

      while (*puiNumAttempts < MAX_ATTEMPTS) {
        (*puiNumAttempts)++;
        // OK, here we go back one, check for OK destination...
        if ((gTacticalStatus.uiFlags & IGNORE_ALL_OBSTACLES) ||
            (NewOKDestination(pSoldier, sGridNo, TRUE, pSoldier->bLevel) &&
             FindBestPath(pSoldier, sGridNo, pSoldier->bLevel, WALKING, NO_COPYROUTE,
                          PATH_THROUGH_PEOPLE))) {
          return (sGridNo);
        }

        // If here, try another place!
        // ( alternate up/down )
        if (bOdd2) {
          bAdjustedDist++;

          sGridNo = sStartGridNo;

          for (cnt = 0; cnt < bAdjustedDist; cnt++) {
            sGridNo = (int16_t)(sGridNo - WORLD_COLS - 1);
          }
        } else {
          sGridNo = sStartGridNo;

          for (cnt = 0; cnt < bAdjustedDist; cnt++) {
            sGridNo = (int16_t)(sGridNo + WORLD_COLS + 1);
          }
        }

        bOdd2 = (int8_t)(!bOdd2);
      }
      return NOWHERE;

    case INSERTION_CODE_NORTH:

      sGridNo = (int16_t)pSoldier->sGridNo;
      sStartGridNo = (int16_t)pSoldier->sGridNo;
      sOldGridNo = (int16_t)pSoldier->sGridNo;

      // Move directly to the up!
      while (GridNoOnVisibleWorldTile(sGridNo)) {
        sOldGridNo = sGridNo;

        if (bOdd) {
          sGridNo -= WORLD_COLS;
        } else {
          sGridNo--;
        }

        bOdd = (int8_t)(!bOdd);
      }

      sGridNo = sOldGridNo;
      sStartGridNo = sOldGridNo;

      while (*puiNumAttempts < MAX_ATTEMPTS) {
        (*puiNumAttempts)++;
        // OK, here we go back one, check for OK destination...
        if ((gTacticalStatus.uiFlags & IGNORE_ALL_OBSTACLES) ||
            (NewOKDestination(pSoldier, sGridNo, TRUE, pSoldier->bLevel) &&
             FindBestPath(pSoldier, sGridNo, pSoldier->bLevel, WALKING, NO_COPYROUTE,
                          PATH_THROUGH_PEOPLE))) {
          return (sGridNo);
        }

        // If here, try another place!
        // ( alternate left/right )
        if (bOdd2) {
          bAdjustedDist++;

          sGridNo = sStartGridNo;

          for (cnt = 0; cnt < bAdjustedDist; cnt++) {
            sGridNo = (int16_t)(sGridNo - WORLD_COLS + 1);
          }
        } else {
          sGridNo = sStartGridNo;

          for (cnt = 0; cnt < bAdjustedDist; cnt++) {
            sGridNo = (int16_t)(sGridNo + WORLD_COLS - 1);
          }
        }

        bOdd2 = (int8_t)(!bOdd2);
      }
      return NOWHERE;

    case INSERTION_CODE_SOUTH:

      sGridNo = (int16_t)pSoldier->sGridNo;
      sStartGridNo = (int16_t)pSoldier->sGridNo;
      sOldGridNo = (int16_t)pSoldier->sGridNo;

      // Move directly to the down!
      while (GridNoOnVisibleWorldTile(sGridNo)) {
        sOldGridNo = sGridNo;

        if (bOdd) {
          sGridNo += WORLD_COLS;
        } else {
          sGridNo++;
        }

        bOdd = (int8_t)(!bOdd);
      }

      sGridNo = sOldGridNo;
      sStartGridNo = sOldGridNo;

      while (*puiNumAttempts < MAX_ATTEMPTS) {
        (*puiNumAttempts)++;
        // OK, here we go back one, check for OK destination...
        if ((gTacticalStatus.uiFlags & IGNORE_ALL_OBSTACLES) ||
            (NewOKDestination(pSoldier, sGridNo, TRUE, pSoldier->bLevel) &&
             FindBestPath(pSoldier, sGridNo, pSoldier->bLevel, WALKING, NO_COPYROUTE,
                          PATH_THROUGH_PEOPLE))) {
          return (sGridNo);
        }

        // If here, try another place!
        // ( alternate left/right )
        if (bOdd2) {
          bAdjustedDist++;

          sGridNo = sStartGridNo;

          for (cnt = 0; cnt < bAdjustedDist; cnt++) {
            sGridNo = (int16_t)(sGridNo - WORLD_COLS + 1);
          }
        } else {
          sGridNo = sStartGridNo;

          for (cnt = 0; cnt < bAdjustedDist; cnt++) {
            sGridNo = (int16_t)(sGridNo + WORLD_COLS - 1);
          }
        }

        bOdd2 = (int8_t)(!bOdd2);
      }
      return NOWHERE;
  }

  // Unhandled exit
  *puiNumAttempts = 0;

  return (NOWHERE);
}

// NEW!
// Calculates the name of the sector based on the loaded sector values.
// Examples:		A9
//						A10_B1
//						J9_B2_A ( >= BETAVERSION ) else J9_B2
//(release equivalent)
static void GetLoadedSectorString(wchar_t *const pString, const size_t Length) {
  if (!gfWorldLoaded) {
    swprintf(pString, Length, L"");
  } else if (gbWorldSectorZ == 0) {
    swprintf(pString, Length, L"%c%d", gWorldSectorY + 'A' - 1, gWorldSectorX);
  } else {
    swprintf(pString, Length, L"%c%d_b%d", gWorldSectorY + 'A' - 1, gWorldSectorX, gbWorldSectorZ);
  }
}

void HandleSlayDailyEvent() {
  SOLDIERTYPE *const pSoldier = FindSoldierByProfileIDOnPlayerTeam(SLAY);
  if (pSoldier == NULL) {
    return;
  }

  // valid soldier?
  if (pSoldier->bLife == 0 || pSoldier->bAssignment == IN_TRANSIT ||
      pSoldier->bAssignment == ASSIGNMENT_POW) {
    // no
    return;
  }

  // ATE: This function is used to check for the ultimate last day SLAY can stay
  // for he may decide to leave randomly while asleep...
  // if the user hasnt renewed yet, and is still leaving today
  if ((pSoldier->iEndofContractTime / 1440) <= (int32_t)GetWorldDay()) {
    pSoldier->ubLeaveHistoryCode = HISTORY_SLAY_MYSTERIOUSLY_LEFT;
    MakeCharacterDialogueEventContractEndingNoAskEquip(*pSoldier);
  }
}

bool IsSectorDesert(int16_t const x, int16_t const y) {
  return SectorInfo[SECTOR(x, y)].ubTraversability[THROUGH_STRATEGIC_MOVE] == SAND;
}

static void HandleDefiniteUnloadingOfWorld(uint8_t const ubUnloadCode) {
  // clear tactical queue
  ClearEventQueue();

  // ATE: End all bullets....
  DeleteAllBullets();

  // End all physics objects...
  RemoveAllPhysicsObjects();

  RemoveAllActiveTimedBombs();

  // handle any quest stuff here so world items can be affected
  HandleQuestCodeOnSectorExit(gWorldSectorX, gWorldSectorY, gbWorldSectorZ);

  // if we arent loading a saved game
  if (!(gTacticalStatus.uiFlags & LOADING_SAVED_GAME)) {
    // Clear any potential battle flags.  They will be set if necessary.
    gTacticalStatus.fEnemyInSector = FALSE;
    gTacticalStatus.uiFlags &= ~INCOMBAT;
  }

  if (ubUnloadCode == ABOUT_TO_LOAD_NEW_MAP) {
    // if we arent loading a saved game
    if (!(gTacticalStatus.uiFlags & LOADING_SAVED_GAME)) {
      // Save the current sectors Item list to a temporary file, if its not the
      // first time in
      SaveCurrentSectorsInformationToTempItemFile();

      // Update any mercs currently in sector, their profile info...
      UpdateSoldierPointerDataIntoProfile();
    }
  } else if (ubUnloadCode == ABOUT_TO_TRASH_WORLD) {
    // Save the current sectors open temp files to the disk
    SaveCurrentSectorsInformationToTempItemFile();

    // Setup the tactical existance of the current soldier.
    //@@@Evaluate
    SetupProfileInsertionDataForCivilians();

    gfBlitBattleSectorLocator = FALSE;
  }

  // Handle cases for both types of unloading
  HandleMilitiaStatusInCurrentMapBeforeLoadingNewMap();
}

BOOLEAN HandlePotentialBringUpAutoresolveToFinishBattle() {
  int32_t i;

  // We don't have mercs in the sector.  Now, we check to see if there are BOTH
  // enemies and militia.  If both co-exist in the sector, then make them fight
  // for control of the sector via autoresolve.
  for (i = gTacticalStatus.Team[ENEMY_TEAM].bFirstID;
       i <= gTacticalStatus.Team[CREATURE_TEAM].bLastID; i++) {
    SOLDIERTYPE const &creature = GetMan(i);
    if (creature.bActive && creature.bLife != 0 && creature.sSectorX == gWorldSectorX &&
        creature.sSectorY == gWorldSectorY &&
        creature.bSectorZ == gbWorldSectorZ) {  // We have enemies, now look for militia!
      for (i = gTacticalStatus.Team[MILITIA_TEAM].bFirstID;
           i <= gTacticalStatus.Team[MILITIA_TEAM].bLastID; i++) {
        SOLDIERTYPE const &milita = GetMan(i);
        if (milita.bActive && milita.bLife != 0 && milita.bSide == OUR_TEAM &&
            milita.sSectorX == gWorldSectorX && milita.sSectorY == gWorldSectorY &&
            milita.bSectorZ == gbWorldSectorZ) {  // We have militia and enemies and no mercs!
                                                  // Let's finish this battle in autoresolve.
          gfEnteringMapScreen = TRUE;
          gfEnteringMapScreenToEnterPreBattleInterface = TRUE;
          gfAutomaticallyStartAutoResolve = TRUE;
          gfUsePersistantPBI = FALSE;
          gubPBSectorX = (uint8_t)gWorldSectorX;
          gubPBSectorY = (uint8_t)gWorldSectorY;
          gubPBSectorZ = (uint8_t)gbWorldSectorZ;
          gfBlitBattleSectorLocator = TRUE;
          gfTransferTacticalOppositionToAutoResolve = TRUE;
          if (gubEnemyEncounterCode != CREATURE_ATTACK_CODE) {
            gubEnemyEncounterCode = ENEMY_INVASION_CODE;  // has to be, if militia are here.
          } else {
            // DoScreenIndependantMessageBox(gzLateLocalizedString[STR_LATE_39],
            // MSG_BOX_FLAG_OK, MapScreenDefaultOkBoxCallback);
          }

          return (TRUE);
        }
      }
    }
  }

  return (FALSE);
}

BOOLEAN CheckAndHandleUnloadingOfCurrentWorld() try {
  int16_t sBattleSectorX, sBattleSectorY, sBattleSectorZ;

  // Don't bother checking this if we don't have a world loaded.
  if (!gfWorldLoaded) {
    return FALSE;
  }

  if (DidGameJustStart() && SECTOR(gWorldSectorX, gWorldSectorY) == START_SECTOR &&
      gbWorldSectorZ == 0) {
    return FALSE;
  }

  GetCurrentBattleSectorXYZ(&sBattleSectorX, &sBattleSectorY, &sBattleSectorZ);

  if (guiCurrentScreen == AUTORESOLVE_SCREEN) {  // The user has decided to let the game autoresolve
                                                 // the current battle.
    if (gWorldSectorX == sBattleSectorX && gWorldSectorY == sBattleSectorY &&
        gbWorldSectorZ == sBattleSectorZ) {
      FOR_EACH_IN_TEAM(i, OUR_TEAM) {  // If we have a live and valid soldier
        SOLDIERTYPE &s = *i;
        if (s.bLife == 0) continue;
        if (s.fBetweenSectors) continue;
        if (IsMechanical(s)) continue;
        if (AM_AN_EPC(&s)) continue;
        if (s.sSectorX != gWorldSectorX) continue;
        if (s.sSectorY != gWorldSectorY) continue;
        if (s.bSectorZ != gbWorldSectorZ) continue;
        RemoveSoldierFromGridNo(s);
        InitSoldierOppList(s);
      }
    }
  } else {                            // Check and see if we have any live mercs in the sector.
    CFOR_EACH_IN_TEAM(s, OUR_TEAM) {  // If we have a live and valid soldier
      if (s->bLife != 0 && !s->fBetweenSectors && !IsMechanical(*s) && !AM_AN_EPC(s) &&
          s->sSectorX == gWorldSectorX && s->sSectorY == gWorldSectorY &&
          s->bSectorZ == gbWorldSectorZ) {
        return FALSE;
      }
    }
    // KM : August 6, 1999 Patch fix
    //     Added logic to prevent a crash when player mercs would retreat from a
    //     battle involving militia and enemies.
    //		 Without the return here, it would proceed to trash the world, and
    // then when autoresolve would come up to
    //     finish the tactical battle, it would fail to find the existing
    //     soldier information (because it was trashed).
    if (HandlePotentialBringUpAutoresolveToFinishBattle()) {
      return FALSE;
    }
    // end

    // HandlePotentialBringUpAutoresolveToFinishBattle( ); //prior patch logic
  }

  CheckForEndOfCombatMode(FALSE);
  EndTacticalBattleForEnemy();

  // ATE: Change cursor to wait cursor for duration of frame.....
  // save old cursor ID....
  SetCurrentCursorFromDatabase(CURSOR_WAIT_NODELAY);
  RefreshScreen();

  // JA2Gold: Leaving sector, so get rid of ambients!
  DeleteAllAmbients();

  if (guiCurrentScreen == GAME_SCREEN) {
    if (!gfTacticalTraversal) {  // if we are in tactical and don't intend on
                                 // going to another sector immediately, then
      gfEnteringMapScreen = TRUE;
    } else {  // The trashing of the world will be handled automatically.
      return FALSE;
    }
  }

  // We have passed all the checks and can Trash the world.
  HandleDefiniteUnloadingOfWorld(ABOUT_TO_TRASH_WORLD);

  if (guiCurrentScreen == AUTORESOLVE_SCREEN) {
    if (gWorldSectorX == sBattleSectorX && gWorldSectorY == sBattleSectorY &&
        gbWorldSectorZ == sBattleSectorZ) {
      /* Yes, this is and looks like a hack.  The conditions of this if
       * statement doesn't work inside TrashWorld() or more specifically,
       * TacticalRemoveSoldier() from within TrashWorld().  Because we are in
       * the autoresolve screen, soldiers are internally created different (from
       * pointers instead of Menptr[]).  It keys on the fact that we are in the
       * autoresolve screen.  So, by switching the screen, it'll delete the
       * soldiers in the loaded world properly, then later on, once autoresolve
       * is complete, it'll delete the autoresolve soldiers properly.  As you
       * can now see, the above if conditions don't change throughout this whole
       * process which makes it necessary to do it this way. */
      guiCurrentScreen = MAP_SCREEN;
      TrashWorld();
      guiCurrentScreen = AUTORESOLVE_SCREEN;
    }
  } else {
    TrashWorld();
  }

  // Clear all combat related flags.
  gTacticalStatus.fEnemyInSector = FALSE;
  gTacticalStatus.uiFlags &= ~INCOMBAT;
  EndTopMessage();

  // Clear the world sector values.
  SetWorldSectorInvalid();

  // Clear the flags regarding.
  gfCaves = FALSE;
  gfBasement = FALSE;

  return TRUE;
} catch (...) {
  return FALSE;
}

/* This is called just before the world is unloaded to preserve location
 * information for RPCs and NPCs either in the sector or strategically in the
 * sector (such as firing an NPC in a sector that isn't yet loaded.)  When
 * loading that sector, the RPC would be added. */
//@@@Evaluate
void SetupProfileInsertionDataForSoldier(const SOLDIERTYPE *const s) {
  if (s->ubProfile == NO_PROFILE) return;
  MERCPROFILESTRUCT &p = GetProfile(s->ubProfile);

  // can't be changed?
  if (p.ubMiscFlags3 & PROFILE_MISC_FLAG3_PERMANENT_INSERTION_CODE) return;

  if (gfWorldLoaded && s->bActive && s->bInSector) {
    // This soldier is currently in the sector

    //@@@Evaluate -- insert code here
    // SAMPLE CODE:  There are multiple situations that I didn't code.  The
    // gridno should be the final destination or reset???

    if (s->ubQuoteRecord && s->ubQuoteActionID) {
      // if moving to traverse
      if (s->ubQuoteActionID >= QUOTE_ACTION_ID_TRAVERSE_EAST &&
          s->ubQuoteActionID <= QUOTE_ACTION_ID_TRAVERSE_NORTH) {
        // Handle traversal.  This NPC's sector will NOT already be set
        // correctly, so we have to call for that too
        HandleNPCChangesForTacticalTraversal(s);
        p.fUseProfileInsertionInfo = FALSE;
        if (s->ubProfile != NO_PROFILE &&
            NPCHasUnusedRecordWithGivenApproach(s->ubProfile, APPROACH_DONE_TRAVERSAL)) {
          p.ubMiscFlags3 |= PROFILE_MISC_FLAG3_HANDLE_DONE_TRAVERSAL;
        }
      } else {
        if (s->sFinalDestination == s->sGridNo) {
          p.usStrategicInsertionData = s->sGridNo;
        } else if (s->sAbsoluteFinalDestination != NOWHERE) {
          p.usStrategicInsertionData = s->sAbsoluteFinalDestination;
        } else {
          p.usStrategicInsertionData = s->sFinalDestination;
        }

        p.fUseProfileInsertionInfo = TRUE;
        p.ubStrategicInsertionCode = INSERTION_CODE_GRIDNO;
        p.ubQuoteActionID = s->ubQuoteActionID;
        p.ubQuoteRecord = s->ubQuoteActionID;
      }
    } else {
      p.fUseProfileInsertionInfo = FALSE;
    }
  } else {
    // use strategic information
    /* It appears to set the soldier's strategic insertion code everytime a
     * group arrives in a new sector.  The insertion data isn't needed for these
     * cases as the code is a direction only. */
    p.ubStrategicInsertionCode = s->ubStrategicInsertionCode;
    p.usStrategicInsertionData = 0;

    // Strategic system should now work.
    p.fUseProfileInsertionInfo = TRUE;
  }
}

static void HandlePotentialMoraleHitForSkimmingSectors(GROUP *pGroup) {
  if (!gTacticalStatus.fHasEnteredCombatModeSinceEntering && gTacticalStatus.fEnemyInSector) {
    // Flag is set so if "wilderness" enemies are in the adjacent sector of this
    // group, the group has a 90% chance of ambush.  Because this typically
    // doesn't happen very often, the chance is high. This reflects the enemies
    // radioing ahead to other enemies of the group's arrival, so they have time
    // to setup a good ambush!
    pGroup->uiFlags |= GROUPFLAG_HIGH_POTENTIAL_FOR_AMBUSH;

    CFOR_EACH_PLAYER_IN_GROUP(pPlayer, pGroup) {
      // Do morale hit...
      // CC look here!
      // pPlayer->pSoldier
    }
  }
}

uint32_t GetWorldSector() {
  if (gWorldSectorX == 0 || gWorldSectorY == 0) return NO_SECTOR;
  return SECTOR(gWorldSectorX, gWorldSectorY);
}
