// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Strategic/GameInit.h"

#include <stdexcept>
#include <string.h>

#include "Cheats.h"
#include "GameLoop.h"
#include "GameSettings.h"
#include "HelpScreen.h"
#include "JAScreens.h"
#include "Laptop/AIMMembers.h"
#include "Laptop/BobbyR.h"
#include "Laptop/EMail.h"
#include "Laptop/Finances.h"
#include "Laptop/History.h"
#include "Laptop/Laptop.h"
#include "Laptop/LaptopSave.h"
#include "Macro.h"
#include "SGP/Debug.h"
#include "SGP/Random.h"
#include "SGP/SoundMan.h"
#include "ScreenIDs.h"
#include "Strategic/CampaignInit.h"
#include "Strategic/CampaignTypes.h"
#include "Strategic/CreatureSpreading.h"
#include "Strategic/GameClock.h"
#include "Strategic/GameEventHook.h"
#include "Strategic/MapScreen.h"
#include "Strategic/MapScreenHelicopter.h"
#include "Strategic/MapScreenInterface.h"
#include "Strategic/MapScreenInterfaceBorder.h"
#include "Strategic/MapScreenInterfaceMap.h"
#include "Strategic/Meanwhile.h"
#include "Strategic/PreBattleInterface.h"
#include "Strategic/QueenCommand.h"
#include "Strategic/Quests.h"
#include "Strategic/Strategic.h"
#include "Strategic/StrategicAI.h"
#include "Strategic/StrategicMap.h"
#include "Strategic/StrategicMines.h"
#include "Strategic/StrategicMovement.h"
#include "Strategic/StrategicTownLoyalty.h"
#include "Tactical/AirRaid.h"
#include "Tactical/AnimationData.h"
#include "Tactical/ArmsDealerInit.h"
#include "Tactical/DialogueControl.h"
#include "Tactical/Interface.h"
#include "Tactical/InterfaceDialogue.h"
#include "Tactical/MercEntering.h"
#include "Tactical/MercHiring.h"
#include "Tactical/OppList.h"
#include "Tactical/Overhead.h"
#include "Tactical/ShopKeeperInterface.h"
#include "Tactical/SoldierControl.h"
#include "Tactical/SoldierCreate.h"
#include "Tactical/SoldierInitList.h"
#include "Tactical/SoldierProfile.h"
#include "Tactical/Squads.h"
#include "Tactical/TacticalSave.h"
#include "Tactical/Vehicles.h"
#include "TacticalAI/NPC.h"
#include "TileEngine/WorldDef.h"
#include "Utils/FontControl.h"
#include "Utils/Message.h"
#include "Utils/TimerControl.h"

uint8_t gubScreenCount = 0;

static void InitNPCs() {
  {  // add the pilot at a random location!
    MERCPROFILESTRUCT &p = GetProfile(SKYRIDER);
    switch (Random(4)) {
      case 0:
        p.sSectorX = 15;
        p.sSectorY = MAP_ROW_B;
        p.bSectorZ = 0;
        break;
      case 1:
        p.sSectorX = 14;
        p.sSectorY = MAP_ROW_E;
        p.bSectorZ = 0;
        break;
      case 2:
        p.sSectorX = 12;
        p.sSectorY = MAP_ROW_D;
        p.bSectorZ = 0;
        break;
      case 3:
        p.sSectorX = 16;
        p.sSectorY = MAP_ROW_C;
        p.bSectorZ = 0;
        break;
    }

    // use alternate map, with Skyrider's shack, in this sector
    SectorInfo[SECTOR(p.sSectorX, p.sSectorY)].uiFlags |= SF_USE_ALTERNATE_MAP;
  }

  // set up Madlab's secret lab (he'll be added when the meanwhile scene occurs)

  switch (Random(4)) {
    case 0:
      // use alternate map in this sector
      SectorInfo[SECTOR(7, MAP_ROW_H)].uiFlags |= SF_USE_ALTERNATE_MAP;
      break;
    case 1:
      SectorInfo[SECTOR(16, MAP_ROW_H)].uiFlags |= SF_USE_ALTERNATE_MAP;
      break;
    case 2:
      SectorInfo[SECTOR(11, MAP_ROW_I)].uiFlags |= SF_USE_ALTERNATE_MAP;
      break;
    case 3:
      SectorInfo[SECTOR(4, MAP_ROW_E)].uiFlags |= SF_USE_ALTERNATE_MAP;
      break;
  }

  {  // add Micky in random location
    MERCPROFILESTRUCT &p = GetProfile(MICKY);
    switch (Random(5)) {
      case 0:
        p.sSectorX = 9;
        p.sSectorY = MAP_ROW_G;
        p.bSectorZ = 0;
        break;
      case 1:
        p.sSectorX = 13;
        p.sSectorY = MAP_ROW_D;
        p.bSectorZ = 0;
        break;
      case 2:
        p.sSectorX = 5;
        p.sSectorY = MAP_ROW_C;
        p.bSectorZ = 0;
        break;
      case 3:
        p.sSectorX = 2;
        p.sSectorY = MAP_ROW_H;
        p.bSectorZ = 0;
        break;
      case 4:
        p.sSectorX = 6;
        p.sSectorY = MAP_ROW_C;
        p.bSectorZ = 0;
        break;
    }

    // use alternate map in this sector
    // SectorInfo[SECTOR(p.sSectorX, p.sSectorY)].uiFlags |=
    // SF_USE_ALTERNATE_MAP;
  }

  gfPlayerTeamSawJoey = FALSE;

  if (gGameOptions.fSciFi) {
    {  // add Bob
      MERCPROFILESTRUCT &p = GetProfile(BOB);
      p.sSectorX = 8;
      p.sSectorY = MAP_ROW_F;
      p.bSectorZ = 0;
    }

    {  // add Gabby in random location
      MERCPROFILESTRUCT &p = gMercProfiles[GABBY];
      switch (Random(2)) {
        case 0:
          p.sSectorX = 11;
          p.sSectorY = MAP_ROW_H;
          p.bSectorZ = 0;
          break;
        case 1:
          p.sSectorX = 4;
          p.sSectorY = MAP_ROW_I;
          p.bSectorZ = 0;
          break;
      }

      // use alternate map in this sector
      SectorInfo[SECTOR(p.sSectorX, p.sSectorY)].uiFlags |= SF_USE_ALTERNATE_MAP;
    }
  } else {  // not scifi, so use alternate map in Tixa's b1 level that doesn't
            // have the stairs going down to the caves.
    UNDERGROUND_SECTORINFO *pSector;
    pSector = FindUnderGroundSector(9, 10, 1);  // j9_b1
    if (pSector) {
      pSector->uiFlags |= SF_USE_ALTERNATE_MAP;
    }
  }

  // init hospital variables
  giHospitalTempBalance = 0;
  giHospitalRefund = 0;
  gbHospitalPriceModifier = 0;

  // set up Devin so he will be placed ASAP
  gMercProfiles[DEVIN].bNPCData = 3;
}

void InitBloodCatSectors() {
  int32_t i;
  // Hard coded table of bloodcat populations.  We don't have
  // access to the real population (if different) until we physically
  // load the map.  If the real population is different, then an error
  // will be reported.
  for (i = 0; i < 255; i++) {
    SectorInfo[i].bBloodCats = -1;
  }
  SectorInfo[SEC_A15].bBloodCatPlacements = 9;
  SectorInfo[SEC_B4].bBloodCatPlacements = 9;
  SectorInfo[SEC_B16].bBloodCatPlacements = 8;
  SectorInfo[SEC_C3].bBloodCatPlacements = 12;
  SectorInfo[SEC_C8].bBloodCatPlacements = 13;
  SectorInfo[SEC_C11].bBloodCatPlacements = 7;
  SectorInfo[SEC_D4].bBloodCatPlacements = 8;
  SectorInfo[SEC_D9].bBloodCatPlacements = 12;
  SectorInfo[SEC_E11].bBloodCatPlacements = 10;
  SectorInfo[SEC_E13].bBloodCatPlacements = 14;
  SectorInfo[SEC_F3].bBloodCatPlacements = 13;
  SectorInfo[SEC_F5].bBloodCatPlacements = 7;
  SectorInfo[SEC_F7].bBloodCatPlacements = 12;
  SectorInfo[SEC_F12].bBloodCatPlacements = 9;
  SectorInfo[SEC_F14].bBloodCatPlacements = 14;
  SectorInfo[SEC_F15].bBloodCatPlacements = 8;
  SectorInfo[SEC_G6].bBloodCatPlacements = 7;
  SectorInfo[SEC_G10].bBloodCatPlacements = 12;
  SectorInfo[SEC_G12].bBloodCatPlacements = 11;
  SectorInfo[SEC_H5].bBloodCatPlacements = 9;
  SectorInfo[SEC_I4].bBloodCatPlacements = 8;
  SectorInfo[SEC_I15].bBloodCatPlacements = 8;
  SectorInfo[SEC_J6].bBloodCatPlacements = 11;
  SectorInfo[SEC_K3].bBloodCatPlacements = 12;
  SectorInfo[SEC_K6].bBloodCatPlacements = 14;
  SectorInfo[SEC_K10].bBloodCatPlacements = 12;
  SectorInfo[SEC_K14].bBloodCatPlacements = 14;

  switch (gGameOptions.ubDifficultyLevel) {
    case DIF_LEVEL_EASY:  // 50%
      SectorInfo[SEC_I16].bBloodCatPlacements = 14;
      SectorInfo[SEC_I16].bBloodCats = 14;
      SectorInfo[SEC_N5].bBloodCatPlacements = 8;
      SectorInfo[SEC_N5].bBloodCats = 8;
      break;
    case DIF_LEVEL_MEDIUM:  // 75%
      SectorInfo[SEC_I16].bBloodCatPlacements = 19;
      SectorInfo[SEC_I16].bBloodCats = 19;
      SectorInfo[SEC_N5].bBloodCatPlacements = 10;
      SectorInfo[SEC_N5].bBloodCats = 10;
      break;
    case DIF_LEVEL_HARD:  // 100%
      SectorInfo[SEC_I16].bBloodCatPlacements = 26;
      SectorInfo[SEC_I16].bBloodCats = 26;
      SectorInfo[SEC_N5].bBloodCatPlacements = 12;
      SectorInfo[SEC_N5].bBloodCats = 12;
      break;
  }
}

void InitStrategicLayer() {
  // Clear starategic layer!
  SetupNewStrategicGame();
  InitQuestEngine();

  // Setup a new campaign via the enemy perspective.
  InitNewCampaign();
  // Init Squad Lists
  InitSquads();
  // Init vehicles
  InitVehicles();
  // init town loyalty
  InitTownLoyalty();
  // init the mine management system
  InitializeMines();
  // initialize map screen flags
  InitMapScreenFlags();
  // initialize NPCs, select alternate maps, etc
  InitNPCs();
  // init Skyrider and his helicopter
  InitializeHelicopter();
  // Clear out the vehicle list
  ClearOutVehicleList();

  InitBloodCatSectors();

  InitializeSAMSites();

  // make Orta, Tixa, SAM sites not found
  InitMapSecrets();

  // free up any leave list arrays that were left allocated
  ShutDownLeaveList();
  // re-set up leave list arrays for dismissed mercs
  InitLeaveList();

  // reset time compression mode to X0 (this will also pause it)
  SetGameTimeCompressionLevel(TIME_COMPRESS_X0);

  // Select the start sector as the initial selected sector
  ChangeSelectedMapSector(SECTORX(START_SECTOR), SECTORY(START_SECTOR), 0);

  // Reset these flags or mapscreen could be disabled and cause major headache.
  fDisableDueToBattleRoster = FALSE;
  fDisableMapInterfaceDueToBattle = FALSE;
}

void ShutdownStrategicLayer() {
  DeleteAllStrategicEvents();
  RemoveAllGroups();
  TrashUndergroundSectorInfo();
  DeleteCreatureDirectives();
  KillStrategicAI();
}

void InitNewGame() {
  uiMeanWhileFlags = 0;
  SetSelectedMan(0);

  resetCheatLevelToInitialValue();

  if (gubScreenCount == 0) {
    LoadMercProfiles();
    InitAllArmsDealers();
    InitBobbyRayInventory();
  }

  ClearTacticalMessageQueue();
  FreeGlobalMessageList();  // Clear mapscreen messages

  if (gubScreenCount == 0) {  // Our first time, go into laptop
    InitLaptopAndLaptopScreens();
    InitStrategicLayer();
    SetLaptopNewGameFlag();

    // This is for the "mercs climbing down from a rope" animation, NOT Skyrider
    ResetHeliSeats();

    uint32_t const now = GetWorldTotalMin();
    AddPreReadEmail(OLD_ENRICO_1, OLD_ENRICO_1_LENGTH, MAIL_ENRICO, now);
    AddPreReadEmail(OLD_ENRICO_2, OLD_ENRICO_2_LENGTH, MAIL_ENRICO, now);
    AddPreReadEmail(RIS_REPORT, RIS_REPORT_LENGTH, RIS_EMAIL, now);
    AddPreReadEmail(OLD_ENRICO_3, OLD_ENRICO_3_LENGTH, MAIL_ENRICO, now);
    AddEmail(IMP_EMAIL_INTRO, IMP_EMAIL_INTRO_LENGTH, CHAR_PROFILE_SITE, now);

    // ATE: Set starting cash
    int32_t starting_cash;
    switch (gGameOptions.ubDifficultyLevel) {
      case DIF_LEVEL_EASY:
        starting_cash = 45000;
        break;
      case DIF_LEVEL_MEDIUM:
        starting_cash = 35000;
        break;
      case DIF_LEVEL_HARD:
        starting_cash = 30000;
        break;
      default:
        throw std::logic_error("invalid difficulty level");
    }
    AddTransactionToPlayersBook(ANONYMOUS_DEPOSIT, 0, now, starting_cash);

    // Schedule email for message from Speck at 7am 1 to 2 days in the future
    uint32_t const days_time_merc_site_available = Random(2) + 1;
    AddFutureDayStrategicEvent(EVENT_DAY3_ADD_EMAIL_FROM_SPECK, 60 * 7, 0,
                               days_time_merc_site_available);

    SetLaptopExitScreen(INIT_SCREEN);
    SetPendingNewScreen(LAPTOP_SCREEN);
    gubScreenCount = 1;

    // Set the fact the game is in progress
    gTacticalStatus.fHasAGameBeenStarted = TRUE;
  } else if (gubScreenCount == 1) {
    gubScreenCount = 2;
  }
}

BOOLEAN AnyMercsHired() {
  CFOR_EACH_IN_TEAM(s, OUR_TEAM) { return TRUE; }
  return FALSE;
}

static BOOLEAN QuickGameMemberHireMerc(uint8_t ubCurrentSoldier);
static void QuickSetupOfMercProfileItems(uint32_t uiCount, uint8_t ubProfileIndex);

static void QuickStartGame() {
  int32_t cnt;
  uint16_t usVal;
  uint8_t ub1 = 0, ub2 = 0;

  for (cnt = 0; cnt < 3; cnt++) {
    if (cnt == 0) {
      usVal = (uint16_t)Random(40);

      QuickSetupOfMercProfileItems(cnt, (uint8_t)usVal);
      QuickGameMemberHireMerc((uint8_t)usVal);
    } else if (cnt == 1) {
      do {
        usVal = (uint16_t)Random(40);
      } while (usVal != ub1);

      QuickSetupOfMercProfileItems(cnt, (uint8_t)usVal);
      QuickGameMemberHireMerc((uint8_t)usVal);
    } else if (cnt == 2) {
      do {
        usVal = (uint16_t)Random(40);
      } while (usVal != ub1 && usVal != ub2);

      QuickSetupOfMercProfileItems(cnt, (uint8_t)usVal);
      QuickGameMemberHireMerc((uint8_t)usVal);
    }
  }
}

static void GiveItemN(MERCPROFILESTRUCT &p, const uint32_t pos, const uint16_t item_id,
                      const uint8_t status, const uint8_t count) {
  p.inv[pos] = item_id;
  p.bInvStatus[pos] = status;
  p.bInvNumber[pos] = count;
}

static void GiveItem(MERCPROFILESTRUCT &p, const uint32_t pos, const uint16_t item_id) {
  GiveItemN(p, pos, item_id, 100, 1);
}

// TEMP FUNCTION!
static void QuickSetupOfMercProfileItems(const uint32_t uiCount, const uint8_t ubProfileIndex) {
  MERCPROFILESTRUCT &p = GetProfile(ubProfileIndex);
  // Quickly give some guys we hire some items
  switch (uiCount) {
    case 0:
      p.bSkillTrait = MARTIALARTS;

      // GiveItemN(p, HANDPOS, HAND_GRENADE, 100, 3);
      GiveItem(p, HANDPOS, C7);
      GiveItem(p, BIGPOCK1POS, CAWS);
      GiveItem(p, BIGPOCK3POS, MEDICKIT);
      GiveItem(p, BIGPOCK4POS, SHAPED_CHARGE);
      GiveItem(p, SMALLPOCK3POS, KEY_2);
      GiveItem(p, SMALLPOCK5POS, LOCKSMITHKIT);

      // TEMP!
      // make carman's opinion of us high!
      GetProfile(CARMEN).bMercOpinion[ubProfileIndex] = 25;
      break;

    case 1:
      GiveItem(p, HANDPOS, CAWS);
      GiveItem(p, SMALLPOCK3POS, KEY_1);
      break;

    case 2:
      GiveItem(p, HANDPOS, GLOCK_17);
      GiveItem(p, SECONDHANDPOS, SW38);
      GiveItem(p, SMALLPOCK1POS, SILENCER);
      GiveItem(p, SMALLPOCK2POS, SNIPERSCOPE);
      GiveItem(p, SMALLPOCK3POS, LASERSCOPE);
      GiveItem(p, SMALLPOCK5POS, BIPOD);
      GiveItem(p, SMALLPOCK6POS, LOCKSMITHKIT);
      break;

    default:
      p.inv[HANDPOS] = Random(30);
      p.bInvNumber[HANDPOS] = 1;
      break;
  }

  GiveItem(p, HELMETPOS, KEVLAR_HELMET);
  GiveItem(p, VESTPOS, KEVLAR_VEST);
  GiveItemN(p, BIGPOCK2POS, RDX, 10, 1);
  GiveItemN(p, SMALLPOCK4POS, HAND_GRENADE, 100, 4);

  // Give special items to some NPCs
  // GiveItem(GetProfile(CARMEN), SMALLPOCK4POS, TERRORIST_INFO);
}

static BOOLEAN QuickGameMemberHireMerc(uint8_t ubCurrentSoldier) {
  MERC_HIRE_STRUCT HireMercStruct;

  memset(&HireMercStruct, 0, sizeof(MERC_HIRE_STRUCT));

  HireMercStruct.ubProfileID = ubCurrentSoldier;

  HireMercStruct.sSectorX = SECTORX(g_merc_arrive_sector);
  HireMercStruct.sSectorY = SECTORY(g_merc_arrive_sector);
  HireMercStruct.fUseLandingZoneForArrival = TRUE;

  HireMercStruct.fCopyProfileItemsOver = TRUE;
  HireMercStruct.ubInsertionCode = INSERTION_CODE_CHOPPER;

  HireMercStruct.iTotalContractLength = 7;

  // specify when the merc should arrive
  HireMercStruct.uiTimeTillMercArrives = 0;

  // if we succesfully hired the merc
  if (!HireMerc(HireMercStruct)) {
    return (FALSE);
  }

  // add an entry in the finacial page for the hiring of the merc
  AddTransactionToPlayersBook(HIRED_MERC, ubCurrentSoldier, GetWorldTotalMin(),
                              -(int32_t)gMercProfiles[ubCurrentSoldier].uiWeeklySalary);

  if (gMercProfiles[ubCurrentSoldier].bMedicalDeposit) {
    // add an entry in the finacial page for the medical deposit
    AddTransactionToPlayersBook(MEDICAL_DEPOSIT, ubCurrentSoldier, GetWorldTotalMin(),
                                -(gMercProfiles[ubCurrentSoldier].sMedicalDepositAmount));
  }

  // add an entry in the history page for the hiring of the merc
  AddHistoryToPlayersLog(HISTORY_HIRED_MERC_FROM_AIM, ubCurrentSoldier, GetWorldTotalMin(), -1, -1);

  return (TRUE);
}

// This function is called when the game is REstarted.  Things that need to be
// reinited are placed in here
void ReStartingGame() {
  // Pause the game
  gfGamePaused = TRUE;

  // Reset the sectors
  SetWorldSectorInvalid();

  SoundStopAll();

  // we are going to restart a game so initialize the variable so we can
  // initialize a new game
  gubScreenCount = 0;

  InitTacticalSave();

  // Loop through all the soldier and delete them all
  FOR_EACH_SOLDIER(i) TacticalRemoveSoldier(*i);

  // Re-init overhead...
  InitOverhead();

  // Reset the email list
  ShutDownEmailList();

  // Reinit the laptopn screen variables
  InitLaptopAndLaptopScreens();
  LaptopScreenInit();

  // Reload the Merc profiles
  LoadMercProfiles();

  // Reload quote files
  ReloadAllQuoteFiles();

  // Initialize the ShopKeeper Interface ( arms dealer inventory, etc. )
  ShopKeeperScreenInit();

  // Delete the world info
  TrashWorld();

  // Init the help screen system
  InitHelpScreenSystem();

  EmptyDialogueQueue();

  if (InAirRaid()) {
    EndAirRaid();
  }

  // Make sure the game starts in the TEAM panel ( it wasnt being reset )
  gsCurInterfacePanel = TEAM_PANEL;

  // Delete all the strategic events
  DeleteAllStrategicEvents();

  // This function gets called when ur in a game a click the quit to main menu
  // button, therefore no game is in progress
  gTacticalStatus.fHasAGameBeenStarted = FALSE;

  // Reset timer callbacks
  gpCustomizableTimerCallback = NULL;

  resetCheatLevelToInitialValue();
}
