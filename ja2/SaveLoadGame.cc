// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "SaveLoadGame.h"

#include <stdexcept>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "Cheats.h"
#include "Directories.h"
#include "GameLoop.h"
#include "GameRes.h"
#include "GameSettings.h"
#include "GameVersion.h"
#include "HelpScreen.h"
#include "JAScreens.h"
#include "Laptop/BobbyR.h"
#include "Laptop/BobbyRMailOrder.h"
#include "Laptop/Files.h"
#include "Laptop/Finances.h"
#include "Laptop/History.h"
#include "Laptop/IMPConfirm.h"
#include "Laptop/IMPPortraits.h"
#include "Laptop/Laptop.h"
#include "Laptop/LaptopSave.h"
#include "Laptop/Mercs.h"
#include "LoadSaveEMail.h"
#include "LoadSaveTacticalStatusType.h"
#include "LoadingScreen.h"
#include "Local.h"
#include "Macro.h"
#include "OptionsScreen.h"
#include "SGP/Buffer.h"
#include "SGP/Debug.h"
#include "SGP/FileMan.h"
#include "SGP/Font.h"
#include "SGP/LoadSaveData.h"
#include "SGP/MemMan.h"
#include "SGP/Random.h"
#include "SGP/Types.h"
#include "SGP/UTF8String.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "SaveLoadScreen.h"
#include "Strategic/Assignments.h"
#include "Strategic/CreatureSpreading.h"
#include "Strategic/GameClock.h"
#include "Strategic/GameEvents.h"
#include "Strategic/MapScreen.h"
#include "Strategic/MapScreenHelicopter.h"
#include "Strategic/MapScreenInterface.h"
#include "Strategic/MapScreenInterfaceBorder.h"
#include "Strategic/MapScreenInterfaceBottom.h"
#include "Strategic/MapScreenInterfaceMap.h"
#include "Strategic/Meanwhile.h"
#include "Strategic/MercContract.h"
#include "Strategic/PreBattleInterface.h"
#include "Strategic/QueenCommand.h"
#include "Strategic/Quests.h"
#include "Strategic/Scheduling.h"
#include "Strategic/StrategicAI.h"
#include "Strategic/StrategicEventHandler.h"
#include "Strategic/StrategicMap.h"
#include "Strategic/StrategicMines.h"
#include "Strategic/StrategicMovement.h"
#include "Strategic/StrategicPathing.h"
#include "Strategic/StrategicStatus.h"
#include "Strategic/StrategicTownLoyalty.h"
#include "Tactical/AirRaid.h"
#include "Tactical/AnimationData.h"
#include "Tactical/ArmsDealerInit.h"
#include "Tactical/Boxing.h"
#include "Tactical/Bullets.h"
#include "Tactical/CivQuotes.h"
#include "Tactical/DialogueControl.h"
#include "Tactical/EnemySoldierSave.h"
#include "Tactical/Interface.h"
#include "Tactical/InterfaceDialogue.h"
#include "Tactical/InterfaceItems.h"
#include "Tactical/InterfacePanels.h"
#include "Tactical/InterfaceUtils.h"
#include "Tactical/Keys.h"
#include "Tactical/LoadSaveMercProfile.h"
#include "Tactical/LoadSaveSoldierType.h"
#include "Tactical/MapInformation.h"
#include "Tactical/MercHiring.h"
#include "Tactical/OppList.h"
#include "Tactical/Overhead.h"
#include "Tactical/SoldierCreate.h"
#include "Tactical/SoldierProfile.h"
#include "Tactical/Squads.h"
#include "Tactical/TacticalSave.h"
#include "Tactical/TeamTurns.h"
#include "Tactical/Vehicles.h"
#include "TacticalAI/AI.h"
#include "TacticalAI/NPC.h"
#include "TileEngine/Environment.h"
#include "TileEngine/ExplosionControl.h"
#include "TileEngine/LightEffects.h"
#include "TileEngine/Lighting.h"
#include "TileEngine/Physics.h"
#include "TileEngine/RenderDirty.h"
#include "TileEngine/RenderWorld.h"
#include "TileEngine/SmokeEffects.h"
#include "TileEngine/TacticalPlacementGUI.h"
#include "TileEngine/WorldDef.h"
#include "Utils/AnimatedProgressBar.h"
#include "Utils/FontControl.h"
#include "Utils/MercTextBox.h"
#include "Utils/Message.h"
#include "Utils/MusicControl.h"
#include "Utils/Text.h"
#include "Utils/TimerControl.h"

static const char g_quicksave_name[] = "QuickSave";
static const char g_savegame_name[] = "SaveGame";
static const char g_savegame_ext[] = "sav";
static const char g_savegame_dir[] = "../SavedGames";

extern int32_t giSortStateForMapScreenList;
extern int16_t sDeadMercs[NUMBER_OF_SQUADS][NUMBER_OF_SOLDIERS_PER_SQUAD];
extern int32_t giRTAILastUpdateTime;
extern BOOLEAN gfRedrawSaveLoadScreen;
extern uint8_t gubScreenCount;
extern int16_t sWorldSectorLocationOfFirstBattle;
extern BOOLEAN gfLoadedGame;
extern uint8_t gubDesertTemperature;
extern uint8_t gubGlobalTemperature;
extern BOOLEAN gfCreatureMeanwhileScenePlayed;

static uint8_t gMusicModeToPlay;

BOOLEAN gfUseConsecutiveQuickSaveSlots = FALSE;
uint32_t guiLastSaveGameNum;

uint32_t guiJA2EncryptionSet = 0;

ScreenID guiScreenToGotoAfterLoadingSavedGame =
    ERROR_SCREEN;  // XXX TODO001A was not properly initialised (0)

extern uint32_t guiCurrentUniqueSoldierId;

// static void SaveTempFileToSavedGame(const char* fileName, HWFILE const
// hFile); static void LoadTempFileFromSavedGame(const char* tempFileName,
// HWFILE const hFile);

static uint8_t const *ExtractGameOptions(uint8_t const *const data, GAME_OPTIONS &g) {
  uint8_t const *d = data;
  EXTR_BOOL(d, g.fGunNut)
  EXTR_BOOL(d, g.fSciFi)
  EXTR_U8(d, g.ubDifficultyLevel)
  EXTR_BOOL(d, g.fTurnTimeLimit)
  EXTR_BOOL(d, g.fIronManMode)
  EXTR_SKIP(d, 7)
  Assert(d == data + 12);
  return d;
}

static uint8_t *InjectGameOptions(uint8_t *const data, GAME_OPTIONS const &g) {
  uint8_t *d = data;
  INJ_BOOL(d, g.fGunNut)
  INJ_BOOL(d, g.fSciFi)
  INJ_U8(d, g.ubDifficultyLevel)
  INJ_BOOL(d, g.fTurnTimeLimit)
  INJ_BOOL(d, g.fIronManMode)
  INJ_SKIP(d, 7)
  Assert(d == data + 12);
  return d;
}

static void CalcJA2EncryptionSet(SAVED_GAME_HEADER const &);
static void PauseBeforeSaveGame();
static void UnPauseAfterSaveGame();
static void SaveGeneralInfo(HWFILE);
static void SaveMeanwhileDefsToSaveGameFile(HWFILE);
static void SaveMercProfiles(HWFILE);
static void SaveOppListInfoToSavedGame(HWFILE);
static void SavePreRandomNumbersToSaveGameFile(HWFILE);
static void SaveSoldierStructure(HWFILE hFile);
static void SaveTacticalStatusToSavedGame(HWFILE);
static void SaveWatchedLocsToSavedGame(HWFILE);

#define InitSaveGameFilePosition(slot) ((void)0)
#define InitShutDownMapTempFileTest(init, name, id) ((void)0)
#define SaveGameFilePosition(slot, file, msg) ((void)0)

BOOLEAN SaveGame(uint8_t const ubSaveGameID, wchar_t const *GameDesc) {
  BOOLEAN fPausedStateBeforeSaving = gfGamePaused;
  BOOLEAN fLockPauseStateBeforeSaving = gfLockPauseState;

  bool fWePausedIt = false;
  if (!GamePaused()) {
    PauseBeforeSaveGame();
    fWePausedIt = true;
  }

  InitShutDownMapTempFileTest(TRUE, "SaveMapTempFile", ubSaveGameID);

  // Place a message on the screen telling the user that we are saving the game
  {
    uint16_t actual_w;
    uint16_t actual_h;
    AutoMercPopUpBox const save_load_game_message_box(PrepareMercPopupBox(
        0, BASIC_MERC_POPUP_BACKGROUND, BASIC_MERC_POPUP_BORDER,
        zSaveLoadText[SLG_SAVING_GAME_MESSAGE], 300, 0, 0, 0, &actual_w, &actual_h));
    uint16_t const x = (SCREEN_WIDTH - actual_w) / 2;
    RenderMercPopUpBox(save_load_game_message_box, x, 160, FRAME_BUFFER);
  }

  InvalidateScreen();
  ExecuteBaseDirtyRectQueue();
  RefreshScreen();

  // Make sure we redraw the screen when we are done
  switch (guiCurrentScreen) {
    case GAME_SCREEN:
      SetRenderFlags(RENDER_FLAG_FULL);
      break;

    case MAP_SCREEN:
      fMapPanelDirty = TRUE;
      fTeamPanelDirty = TRUE;
      fCharacterInfoPanelDirty = TRUE;
      break;

    case SAVE_LOAD_SCREEN:
      gfRedrawSaveLoadScreen = TRUE;
      break;
    default:
      break;
  }

  InitSaveGameFilePosition(ubSaveGameID);

  // Set the fact that we are saving a game
  gTacticalStatus.uiFlags |= LOADING_SAVED_GAME;

  try {
    // Save the current sectors open temp files to the disk
    SaveCurrentSectorsInformationToTempItemFile();

    SAVED_GAME_HEADER header;
    memset(&header, 0, sizeof(header));

    wchar_t(&desc)[lengthof(header.sSavedGameDesc)] = header.sSavedGameDesc;
    if (ubSaveGameID == 0) {  // We are saving the quick save
      swprintf(desc, lengthof(desc), L"%hs", g_quicksave_name);
    } else {
      if (GameDesc[0] == L'\0') GameDesc = pMessageStrings[MSG_NODESC];
      wcsncpy(desc, GameDesc, lengthof(desc));
    }

    FileMan::createDir(g_savegame_dir);

    // Create the save game file
    char savegame_name[512];
    CreateSavedGameFileNameFromNumber(ubSaveGameID, savegame_name);
    AutoSGPFile f(FileMan::openForWriting(savegame_name));
    SaveGameFilePosition(ubSaveGameID, f, "Just Opened File");

    /* If there are no enemy or civilians to save, we have to check BEFORE
     * saving the sector info struct because the
     * NewWayOfSavingEnemyAndCivliansToTempFile() will RESET the civ or enemy
     * flag AFTER they have been saved. */
    NewWayOfSavingEnemyAndCivliansToTempFile(gWorldSectorX, gWorldSectorY, gbWorldSectorZ, TRUE,
                                             TRUE);
    NewWayOfSavingEnemyAndCivliansToTempFile(gWorldSectorX, gWorldSectorY, gbWorldSectorZ, FALSE,
                                             TRUE);

    // Setup the save game header
    header.uiSavedGameVersion = guiSavedGameVersion;
    strcpy(header.zGameVersionNumber, g_version_number);

    // The following will be used to quickly access info to display in the
    // save/load screen
    header.uiDay = GetWorldDay();
    header.ubHour = (uint8_t)GetWorldHour();
    header.ubMin = (uint8_t)guiMin;
    header.sInitialGameOptions = gGameOptions;
    GetBestPossibleSectorXYZValues(&header.sSectorX, &header.sSectorY, &header.bSectorZ);
    header.ubNumOfMercsOnPlayersTeam = NumberOfMercsOnPlayerTeam();
    header.iCurrentBalance = LaptopSaveInfo.iCurrentBalance;
    header.uiCurrentScreen = guiPreviousOptionScreen;
    header.fAlternateSector =
        GetSectorFlagStatus(gWorldSectorX, gWorldSectorY, gbWorldSectorZ, SF_USE_ALTERNATE_MAP);

    if (gfWorldLoaded) {
      header.fWorldLoaded = TRUE;
      header.ubLoadScreenID = GetLoadScreenID(gWorldSectorX, gWorldSectorY, gbWorldSectorZ);
    } else {
      header.fWorldLoaded = FALSE;
      header.ubLoadScreenID = 0;
    }

    header.uiRandom = Random(RAND_MAX);

    // Save the savegame header
    uint8_t data[432];
    uint8_t *d = data;
    INJ_U32(d, header.uiSavedGameVersion)
    INJ_STR(d, header.zGameVersionNumber, lengthof(header.zGameVersionNumber)) {
      DataWriter writer(d);
      writer.writeStringAsUTF16(header.sSavedGameDesc, lengthof(header.sSavedGameDesc));
      d += writer.getConsumed();
    }
    INJ_SKIP(d, 4)
    INJ_U32(d, header.uiDay)
    INJ_U8(d, header.ubHour)
    INJ_U8(d, header.ubMin)
    INJ_I16(d, header.sSectorX)
    INJ_I16(d, header.sSectorY)
    INJ_I8(d, header.bSectorZ)
    INJ_U8(d, header.ubNumOfMercsOnPlayersTeam)
    INJ_I32(d, header.iCurrentBalance)
    INJ_U32(d, header.uiCurrentScreen)
    INJ_BOOL(d, header.fAlternateSector)
    INJ_BOOL(d, header.fWorldLoaded)
    INJ_U8(d, header.ubLoadScreenID)
    d = InjectGameOptions(d, header.sInitialGameOptions);
    INJ_SKIP(d, 1)
    INJ_U32(d, header.uiRandom)
    INJ_SKIP(d, 112)
    Assert(d == endof(data));

    FileWrite(f, data, sizeof(data));
    SaveGameFilePosition(ubSaveGameID, f, "Save Game Header");

    CalcJA2EncryptionSet(header);

    // Save the gTactical Status array, plus the curent sector location
    SaveTacticalStatusToSavedGame(f);
    SaveGameFilePosition(ubSaveGameID, f, "Tactical Status");

    SaveGameClock(f, fPausedStateBeforeSaving, fLockPauseStateBeforeSaving);
    SaveGameFilePosition(ubSaveGameID, f, "Game Clock");

    SaveStrategicEventsToSavedGame(f);
    SaveGameFilePosition(ubSaveGameID, f, "Strategic Events");

    SaveLaptopInfoToSavedGame(f);
    SaveGameFilePosition(ubSaveGameID, f, "Laptop Info");

    SaveMercProfiles(f);
    SaveGameFilePosition(ubSaveGameID, f, "Merc Profiles");

    SaveSoldierStructure(f);
    SaveGameFilePosition(ubSaveGameID, f, "Soldier Structure");

    SaveFilesToSavedGame(FINANCES_DATA_FILE, f);
    SaveGameFilePosition(ubSaveGameID, f, "Finances Data File");

    SaveFilesToSavedGame(HISTORY_DATA_FILE, f);
    SaveGameFilePosition(ubSaveGameID, f, "History file");

    SaveFilesToSavedGame(FILES_DAT_FILE, f);
    SaveGameFilePosition(ubSaveGameID, f, "The Laptop FILES file");

    SaveEmailToSavedGame(f);
    SaveGameFilePosition(ubSaveGameID, f, "Email");

    SaveStrategicInfoToSavedFile(f);
    SaveGameFilePosition(ubSaveGameID, f, "Strategic Information");

    SaveUnderGroundSectorInfoToSaveGame(f);
    SaveGameFilePosition(ubSaveGameID, f, "Underground Information");

    SaveSquadInfoToSavedGameFile(f);
    SaveGameFilePosition(ubSaveGameID, f, "Squad Info");

    SaveStrategicMovementGroupsToSaveGameFile(f);
    SaveGameFilePosition(ubSaveGameID, f, "Strategic Movement Groups");

    SaveMapTempFilesToSavedGameFile(f);
    SaveGameFilePosition(ubSaveGameID, f, "All the Map Temp files");

    SaveQuestInfoToSavedGameFile(f);
    SaveGameFilePosition(ubSaveGameID, f, "Quest Info");

    SaveOppListInfoToSavedGame(f);
    SaveGameFilePosition(ubSaveGameID, f, "OppList info");

    SaveMapScreenMessagesToSaveGameFile(f);
    SaveGameFilePosition(ubSaveGameID, f, "MapScreen Messages");

    SaveNPCInfoToSaveGameFile(f);
    SaveGameFilePosition(ubSaveGameID, f, "NPC Info");

    SaveKeyTableToSaveGameFile(f);
    SaveGameFilePosition(ubSaveGameID, f, "KeyTable");

    SaveTempNpcQuoteArrayToSaveGameFile(f);
    SaveGameFilePosition(ubSaveGameID, f, "NPC Temp Quote File");

    SavePreRandomNumbersToSaveGameFile(f);
    SaveGameFilePosition(ubSaveGameID, f, "PreGenerated Random Files");

    SaveArmsDealerInventoryToSaveGameFile(f);
    SaveGameFilePosition(ubSaveGameID, f, "Arms Dealers Inventory");

    SaveGeneralInfo(f);
    SaveGameFilePosition(ubSaveGameID, f, "Misc. Info");

    SaveMineStatusToSaveGameFile(f);
    SaveGameFilePosition(ubSaveGameID, f, "Mine Status");

    SaveStrategicTownLoyaltyToSaveGameFile(f);
    SaveGameFilePosition(ubSaveGameID, f, "Town Loyalty");

    SaveVehicleInformationToSaveGameFile(f);
    SaveGameFilePosition(ubSaveGameID, f, "Vehicle Information");

    SaveBulletStructureToSaveGameFile(f);
    SaveGameFilePosition(ubSaveGameID, f, "Bullet Information");

    SavePhysicsTableToSaveGameFile(f);
    SaveGameFilePosition(ubSaveGameID, f, "Physics Table");

    SaveAirRaidInfoToSaveGameFile(f);
    SaveGameFilePosition(ubSaveGameID, f, "Air Raid Info");

    SaveTeamTurnsToTheSaveGameFile(f);
    SaveGameFilePosition(ubSaveGameID, f, "Team Turn Info");

    SaveExplosionTableToSaveGameFile(f);
    SaveGameFilePosition(ubSaveGameID, f, "Explosion Table");

    SaveCreatureDirectives(f);
    SaveGameFilePosition(ubSaveGameID, f, "Creature Spreading");

    SaveStrategicStatusToSaveGameFile(f);
    SaveGameFilePosition(ubSaveGameID, f, "Strategic Status");

    SaveStrategicAI(f);
    SaveGameFilePosition(ubSaveGameID, f, "Strategic AI");

    SaveWatchedLocsToSavedGame(f);
    SaveGameFilePosition(ubSaveGameID, f, "Watched Locs Info");

    SaveItemCursorToSavedGame(f);
    SaveGameFilePosition(ubSaveGameID, f, "ItemCursor Info");

    SaveCivQuotesToSaveGameFile(f);
    SaveGameFilePosition(ubSaveGameID, f, "Civ Quote System");

    SaveBackupNPCInfoToSaveGameFile(f);
    SaveGameFilePosition(ubSaveGameID, f, "Backed up NPC Info");

    SaveMeanwhileDefsToSaveGameFile(f);
    SaveGameFilePosition(ubSaveGameID, f, "Meanwhile Definitions");

    SaveSchedules(f);
    SaveGameFilePosition(ubSaveGameID, f, "Schedules");

    NewSaveVehicleMovementInfoToSavedGameFile(f);
    SaveGameFilePosition(ubSaveGameID, f, "Vehicle Movement Stuff");

    SaveContractRenewalDataToSaveGameFile(f);
    SaveGameFilePosition(ubSaveGameID, f, "Contract Renewal Data");

    SaveLeaveItemList(f);
    SaveGameFilePosition(ubSaveGameID, f, "leave list");

    NewWayOfSavingBobbyRMailOrdersToSaveGameFile(f);
    SaveGameFilePosition(ubSaveGameID, f, "New way of saving Bobby R mailorders");
  } catch (...) {
    if (fWePausedIt) UnPauseAfterSaveGame();

    // Delete the failed attempt at saving
    DeleteSaveGameNumber(ubSaveGameID);

    // Put out an error message
    ScreenMsg(FONT_MCOLOR_WHITE, MSG_INTERFACE, zSaveLoadText[SLG_SAVE_GAME_ERROR]);

    InitShutDownMapTempFileTest(FALSE, "SaveMapTempFile", ubSaveGameID);

    NextLoopCheckForEnoughFreeHardDriveSpace();

    gTacticalStatus.uiFlags &= ~LOADING_SAVED_GAME;
    return FALSE;
  }

  // If we succesfully saved the game, mark this entry as the last saved game
  // file
  if (ubSaveGameID != SAVE__ERROR_NUM && ubSaveGameID != SAVE__END_TURN_NUM) {
    gGameSettings.bLastSavedGameSlot = ubSaveGameID;
  }

  SaveGameSettings();

  // Display a screen message that the save was succesful
  if (ubSaveGameID != SAVE__END_TURN_NUM) {
    ScreenMsg(FONT_MCOLOR_WHITE, MSG_INTERFACE, pMessageStrings[MSG_SAVESUCCESS]);
  }

  // Restore the music mode
  SetMusicMode(gubMusicMode);

  // Unset the fact that we are saving a game
  gTacticalStatus.uiFlags &= ~LOADING_SAVED_GAME;

  UnPauseAfterSaveGame();

  InitShutDownMapTempFileTest(FALSE, "SaveMapTempFile", ubSaveGameID);

  NextLoopCheckForEnoughFreeHardDriveSpace();
  return TRUE;
}

/** Parse binary data and fill SAVED_GAME_HEADER structure.
 * @param data Data to be parsed.
 * @param h Header structure to be filled.
 * @param stracLinuxFormat Flag, telling to use "Stracciatella Linux" format. */
void ParseSavedGameHeader(const uint8_t *data, SAVED_GAME_HEADER &h, bool stracLinuxFormat) {
  uint8_t const *d = data;
  EXTR_U32(d, h.uiSavedGameVersion);
  EXTR_STR(d, h.zGameVersionNumber, lengthof(h.zGameVersionNumber));
  if (stracLinuxFormat) {
    DataReader reader(d);
    reader.readUTF32(h.sSavedGameDesc, SIZE_OF_SAVE_GAME_DESC);
    d += reader.getConsumed();
  } else {
    DataReader reader(d);
    reader.readUTF16(h.sSavedGameDesc, SIZE_OF_SAVE_GAME_DESC);
    d += reader.getConsumed();
  }
  EXTR_SKIP(d, 4)
  EXTR_U32(d, h.uiDay)
  EXTR_U8(d, h.ubHour)
  EXTR_U8(d, h.ubMin)
  EXTR_I16(d, h.sSectorX)
  EXTR_I16(d, h.sSectorY)
  EXTR_I8(d, h.bSectorZ)
  EXTR_U8(d, h.ubNumOfMercsOnPlayersTeam)
  EXTR_I32(d, h.iCurrentBalance)
  EXTR_U32(d, h.uiCurrentScreen)
  EXTR_BOOL(d, h.fAlternateSector)
  EXTR_BOOL(d, h.fWorldLoaded)
  EXTR_U8(d, h.ubLoadScreenID)
  d = ExtractGameOptions(d, h.sInitialGameOptions);
  EXTR_SKIP(d, 1)
  EXTR_U32(d, h.uiRandom)
  EXTR_SKIP(d, 112)
  // XXX: this assert doesn't work anymore
  // Assert(d == endof(data));
}

/** @brief Check if SAVED_GAME_HEADER structure contains valid data.
 * This function does only a basic check.  It might not detect all problems. */
bool isValidSavedGameHeader(SAVED_GAME_HEADER &h) {
  if ((h.sSectorX == 0) && (h.sSectorY == 0) && (h.bSectorZ == -1)) {
    // Special case: sector N/A at the game start
    if ((h.uiDay == 0) || (h.iCurrentBalance < 0)) {
      // invalid for sure
      return false;
    }
  } else {
    if ((h.uiDay == 0) || (h.sSectorX <= 0) || (h.sSectorX > 16) || (h.sSectorY <= 0) ||
        (h.sSectorY > 16) || (h.bSectorZ < 0) || (h.bSectorZ > 3) || (h.iCurrentBalance < 0)) {
      // invalid for sure
      return false;
    }
  }
  return true;
}

void ExtractSavedGameHeaderFromFile(HWFILE const f, SAVED_GAME_HEADER &h, bool *stracLinuxFormat) {
  // first try Strac Linux format
  try {
    uint8_t data[SAVED_GAME_HEADER_ON_DISK_SIZE_STRAC_LIN];
    FileRead(f, data, sizeof(data));
    ParseSavedGameHeader(data, h, true);
    if (isValidSavedGameHeader(h)) {
      *stracLinuxFormat = true;
      return;
    }
  } catch (...) {
  }

  {
    // trying vanilla format
    uint8_t data[SAVED_GAME_HEADER_ON_DISK_SIZE];
    FileSeek(f, 0, FILE_SEEK_FROM_START);
    FileRead(f, data, sizeof(data));
    ParseSavedGameHeader(data, h, false);
    *stracLinuxFormat = false;
  }
}

static void HandleOldBobbyRMailOrders();
static void LoadGeneralInfo(HWFILE, uint32_t savegame_version);
static void LoadMeanwhileDefsFromSaveGameFile(HWFILE, uint32_t savegame_version);
static void LoadOppListInfoFromSavedGame(HWFILE);
static void LoadPreRandomNumbersFromSaveGameFile(HWFILE);
static void LoadSavedMercProfiles(HWFILE, uint32_t savegame_version, bool stracLinuxFormat);
static void LoadSoldierStructure(HWFILE, uint32_t savegame_version, bool stracLinuxFormat);
static void LoadTacticalStatusFromSavedGame(HWFILE, bool stracLinuxFormat);
static void LoadWatchedLocsFromSavedGame(HWFILE);
static void TruncateStrategicGroupSizes();
static void UpdateMercMercContractInfo();

#define InitLoadGameFilePosition(slot) ((void)0)
#define LoadGameFilePosition(slot, file, msg) ((void)0)

void LoadSavedGame(uint8_t const save_slot_id) {
  TrashAllSoldiers();
  RemoveAllGroups();

  // Empty the dialogue Queue cause someone could still have a quote in waiting
  EmptyDialogueQueue();

  // If there is someone talking, stop them
  StopAnyCurrentlyTalkingSpeech();

  ZeroAnimSurfaceCounts();

  ShutdownNPCQuotes();

  InitShutDownMapTempFileTest(TRUE, "LoadMapTempFile", save_slot_id);

  // Used in mapescreen to disable the annoying 'swoosh' transitions
  gfDontStartTransitionFromLaptop = TRUE;

  // Reset timer callbacks
  gpCustomizableTimerCallback = NULL;

  InitLoadGameFilePosition(save_slot_id);

  // Set the fact that we are loading a saved game
  gTacticalStatus.uiFlags |= LOADING_SAVED_GAME;

  /* Trash the existing world.  This is done to ensure that if we are loading a
   * game that doesn't have a world loaded, that we trash it beforehand -- else
   * the player could theoretically enter that sector where it would be in a
   * pre-load state. */
  TrashWorld();

  InitTacticalSave();

  // ATE: Added to empty dialogue q
  EmptyDialogueQueue();

  char zSaveGameName[512];
  CreateSavedGameFileNameFromNumber(save_slot_id, zSaveGameName);
  AutoSGPFile f(FileMan::openForReadingSmart(zSaveGameName, false));
  LoadGameFilePosition(save_slot_id, f, "Just Opened File");

  SAVED_GAME_HEADER SaveGameHeader;
  bool stracLinuxFormat;
  ExtractSavedGameHeaderFromFile(f, SaveGameHeader, &stracLinuxFormat);
  LoadGameFilePosition(save_slot_id, f, "Save Game Header");

  CalcJA2EncryptionSet(SaveGameHeader);

  uint32_t const version = SaveGameHeader.uiSavedGameVersion;

  /* If the player is loading up an older version of the game and the person
   * DOESN'T have the cheats on. */
  if (version < 65 && !CHEATER_CHEAT_LEVEL()) throw std::runtime_error("Savegame too old");

  // Store the loading screenID that was saved
  gubLastLoadingScreenID = static_cast<LoadingScreenID>(SaveGameHeader.ubLoadScreenID);

#if 0  // XXX was commented out
	LoadGeneralInfo(f, version);
	LoadGameFilePosition(save_slot_id, f, "Misc info");
#endif

  // Load the gtactical status structure plus the current sector x,y,z
  LoadTacticalStatusFromSavedGame(f, stracLinuxFormat);
  LoadGameFilePosition(save_slot_id, f, "Tactical Status");

  // This gets reset by the above function
  gTacticalStatus.uiFlags |= LOADING_SAVED_GAME;

  // Load the game clock ingo
  LoadGameClock(f);
  LoadGameFilePosition(save_slot_id, f, "Game Clock");

  // if we are suppose to use the alternate sector
  if (SaveGameHeader.fAlternateSector) {
    SetSectorFlag(gWorldSectorX, gWorldSectorY, gbWorldSectorZ, SF_USE_ALTERNATE_MAP);
    gfUseAlternateMap = TRUE;
  }

  // if the world was loaded when saved, reload it, otherwise dont
  if (SaveGameHeader.fWorldLoaded || version < 50) {
    // Get the current world sector coordinates
    int16_t const sLoadSectorX = gWorldSectorX;
    int16_t const sLoadSectorY = gWorldSectorY;
    int8_t const bLoadSectorZ = gbWorldSectorZ;

    // This will guarantee that the sector will be loaded
    SetWorldSectorInvalid();

    // if we should load a sector (if the person didnt just start the game game)
    if (sLoadSectorX != 0 && sLoadSectorY != 0) {
      // Load the sector
      SetCurrentWorldSector(sLoadSectorX, sLoadSectorY, bLoadSectorZ);
    }
  } else {  // By clearing these values, we can avoid "in sector" checks -- at
            // least, that's the theory.
    SetWorldSectorInvalid();

    int16_t const x = SaveGameHeader.sSectorX;
    int16_t const y = SaveGameHeader.sSectorY;
    int8_t const z = SaveGameHeader.bSectorZ;
    gubLastLoadingScreenID =
        x == -1 || y == -1 || z == -1 ? LOADINGSCREEN_HELI : GetLoadScreenID(x, y, z);

    BeginLoadScreen();
  }

  CreateLoadingScreenProgressBar();
  SetProgressBarColor(0, 0, 0, 150);

  uint32_t uiRelStartPerc = 0;
  uint32_t uiRelEndPerc = 0;

#define BAR(delta, text)                                                      \
  (uiRelEndPerc += (delta),                                                   \
   SetRelativeStartAndEndPercentage(0, uiRelStartPerc, uiRelEndPerc, (text)), \
   RenderProgressBar(0, 100), uiRelStartPerc = uiRelEndPerc)

  BAR(1, L"Strategic Events...");
  LoadStrategicEventsFromSavedGame(f);
  LoadGameFilePosition(save_slot_id, f, "Strategic Events");

  BAR(0, L"Laptop Info");
  LoadLaptopInfoFromSavedGame(f);
  LoadGameFilePosition(save_slot_id, f, "Laptop Info");

  BAR(0, L"Merc Profiles...");
  LoadSavedMercProfiles(f, version, stracLinuxFormat);
  LoadGameFilePosition(save_slot_id, f, "Merc Profiles");

  BAR(30, L"Soldier Structure...");
  LoadSoldierStructure(f, version, stracLinuxFormat);
  LoadGameFilePosition(save_slot_id, f, "Soldier Structure");

  BAR(1, L"Finances Data File...");
  LoadFilesFromSavedGame(FINANCES_DATA_FILE, f);
  LoadGameFilePosition(save_slot_id, f, "Finances Data File");

  BAR(1, L"History File...");
  LoadFilesFromSavedGame(HISTORY_DATA_FILE, f);
  LoadGameFilePosition(save_slot_id, f, "History File");

  BAR(1, L"The Laptop FILES file...");
  LoadFilesFromSavedGame(FILES_DAT_FILE, f);
  LoadGameFilePosition(save_slot_id, f, "The Laptop FILES file");

  BAR(1, L"Email...");
  LoadEmailFromSavedGame(f);
  LoadGameFilePosition(save_slot_id, f, "Email");

  BAR(1, L"Strategic Information...");
  LoadStrategicInfoFromSavedFile(f);
  LoadGameFilePosition(save_slot_id, f, "Strategic Information");

  BAR(1, L"UnderGround Information...");
  LoadUnderGroundSectorInfoFromSavedGame(f);
  LoadGameFilePosition(save_slot_id, f, "UnderGround Information");

  BAR(1, L"Squad Info...");
  LoadSquadInfoFromSavedGameFile(f);
  LoadGameFilePosition(save_slot_id, f, "Squad Info");

  BAR(1, L"Strategic Movement Groups...");
  LoadStrategicMovementGroupsFromSavedGameFile(f);
  LoadGameFilePosition(save_slot_id, f, "Strategic Movement Groups");

  BAR(30, L"All the Map Temp files...");
  LoadMapTempFilesFromSavedGameFile(f, version);
  LoadGameFilePosition(save_slot_id, f, "All the Map Temp files");

  BAR(1, L"Quest Info...");
  LoadQuestInfoFromSavedGameFile(f);
  LoadGameFilePosition(save_slot_id, f, "Quest Info");

  BAR(1, L"OppList Info...");
  LoadOppListInfoFromSavedGame(f);
  LoadGameFilePosition(save_slot_id, f, "OppList Info");

  BAR(1, L"MapScreen Messages...");
  LoadMapScreenMessagesFromSaveGameFile(f, stracLinuxFormat);
  LoadGameFilePosition(save_slot_id, f, "MapScreen Messages");

  BAR(1, L"NPC Info...");
  LoadNPCInfoFromSavedGameFile(f, version);
  LoadGameFilePosition(save_slot_id, f, "NPC Info");

  BAR(1, L"KeyTable...");
  LoadKeyTableFromSaveedGameFile(f);
  LoadGameFilePosition(save_slot_id, f, "KeyTable");

  BAR(1, L"Npc Temp Quote File...");
  LoadTempNpcQuoteArrayToSaveGameFile(f);
  LoadGameFilePosition(save_slot_id, f, "Npc Temp Quote File");

  BAR(0, L"PreGenerated Random Files...");
  LoadPreRandomNumbersFromSaveGameFile(f);
  LoadGameFilePosition(save_slot_id, f, "PreGenerated Random Files");

  BAR(0, L"Smoke Effect Structures...");
  // No longer need to load smoke effects.  They are now in temp files
  if (version < 75) LoadSmokeEffectsFromLoadGameFile(f, version);
  LoadGameFilePosition(save_slot_id, f, "Smoke Effect Structures");

  BAR(1, L"Arms Dealers Inventory...");
  LoadArmsDealerInventoryFromSavedGameFile(f, version);
  LoadGameFilePosition(save_slot_id, f, "Arms Dealers Inventory");

  BAR(0, L"Misc info...");
  LoadGeneralInfo(f, version);
  LoadGameFilePosition(save_slot_id, f, "Misc info");

  BAR(1, L"Mine Status...");
  LoadMineStatusFromSavedGameFile(f);
  LoadGameFilePosition(save_slot_id, f, "Mine Status");

  BAR(0, L"Town Loyalty...");
  if (version >= 21) {
    LoadStrategicTownLoyaltyFromSavedGameFile(f);
    LoadGameFilePosition(save_slot_id, f, "Town Loyalty");
  }

  BAR(1, L"Vehicle Information...");
  if (version >= 22) {
    LoadVehicleInformationFromSavedGameFile(f, version);
    LoadGameFilePosition(save_slot_id, f, "Vehicle Information");
  }

  BAR(1, L"Bullet Information...");
  if (version >= 24) {
    LoadBulletStructureFromSavedGameFile(f);
    LoadGameFilePosition(save_slot_id, f, "Bullet Information");
  }

  BAR(1, L"Physics table...");
  if (version >= 24) {
    LoadPhysicsTableFromSavedGameFile(f);
    LoadGameFilePosition(save_slot_id, f, "Physics table");
  }

  BAR(1, L"Air Raid Info...");
  if (version >= 24) {
    LoadAirRaidInfoFromSaveGameFile(f);
    LoadGameFilePosition(save_slot_id, f, "Air Raid Info");
  }

  BAR(0, L"Team Turn Info...");
  if (version >= 24) {
    LoadTeamTurnsFromTheSavedGameFile(f);
    LoadGameFilePosition(save_slot_id, f, "Team Turn Info");
  }

  BAR(1, L"Explosion Table...");
  if (version >= 25) {
    LoadExplosionTableFromSavedGameFile(f);
    LoadGameFilePosition(save_slot_id, f, "Explosion Table");
  }

  BAR(1, L"Creature Spreading...");
  if (version >= 27) {
    LoadCreatureDirectives(f, version);
    LoadGameFilePosition(save_slot_id, f, "Creature Spreading");
  }

  BAR(1, L"Strategic Status...");
  if (version >= 28) {
    LoadStrategicStatusFromSaveGameFile(f);
    LoadGameFilePosition(save_slot_id, f, "Strategic Status");
  }

  BAR(1, L"Strategic AI...");
  if (version >= 31) {
    LoadStrategicAI(f);
    LoadGameFilePosition(save_slot_id, f, "Strategic AI");
  }

  BAR(1, L"Lighting Effects...");
  // No longer need to load Light effects.  They are now in temp files
  if (37 <= version && version < 76) {
    LoadLightEffectsFromLoadGameFile(f);
    LoadGameFilePosition(save_slot_id, f, "Lighting Effects");
  }

  BAR(1, L"Watched Locs Info...");
  if (version >= 38) {
    LoadWatchedLocsFromSavedGame(f);
  }
  LoadGameFilePosition(save_slot_id, f, "Watched Locs Info");

  BAR(1, L"Item cursor Info...");
  if (version >= 39) {
    LoadItemCursorFromSavedGame(f);
  }
  LoadGameFilePosition(save_slot_id, f, "Item cursor Info");

  BAR(1, L"Civ Quote System...");
  if (version >= 51) {
    LoadCivQuotesFromLoadGameFile(f);
  }
  LoadGameFilePosition(save_slot_id, f, "Civ Quote System");

  BAR(1, L"Backed up NPC Info...");
  if (version >= 53) {
    LoadBackupNPCInfoFromSavedGameFile(f);
  }
  LoadGameFilePosition(save_slot_id, f, "Backed up NPC Info");

  BAR(1, L"Meanwhile definitions...");
  if (version >= 58) {
    LoadMeanwhileDefsFromSaveGameFile(f, version);
    LoadGameFilePosition(save_slot_id, f, "Meanwhile definitions");
  } else {
    gMeanwhileDef[gCurrentMeanwhileDef.ubMeanwhileID] = gCurrentMeanwhileDef;
  }

  BAR(1, L"Schedules...");
  if (version >= 59) {
    // trash schedules loaded from map
    DestroyAllSchedulesWithoutDestroyingEvents();
    LoadSchedulesFromSave(f);
    LoadGameFilePosition(save_slot_id, f, "Schedules");
  }

  BAR(1, L"Extra Vehicle Info...");
  if (version >= 61) {
    if (version < 84) {
      LoadVehicleMovementInfoFromSavedGameFile(f);
    } else {
      NewLoadVehicleMovementInfoFromSavedGameFile(f);
    }
    LoadGameFilePosition(save_slot_id, f, "Extra Vehicle Info");
  }

  BAR(1, L"Contract renweal sequence stuff...");
  if (version < 62) {  // The older games had a bug where this flag was never being set
    gfResetAllPlayerKnowsEnemiesFlags = TRUE;
  }

  if (version >= 67) {
    LoadContractRenewalDataFromSaveGameFile(f);
    LoadGameFilePosition(save_slot_id, f, "Contract renweal sequence stuff");
  }

  if (version >= 70) {
    LoadLeaveItemList(f);
    LoadGameFilePosition(save_slot_id, f, "Leave List");
  }

  if (version <= 73) {  // Patch vehicle fuel
    AddVehicleFuelToSave();
  }

  if (version >= 85) {
    NewWayOfLoadingBobbyRMailOrdersToSaveGameFile(f);
    LoadGameFilePosition(save_slot_id, f, "New way of loading Bobby R mailorders");
  }

  // If there are any old Bobby R Mail orders, tranfer them to the new system
  if (version < 85) {
    HandleOldBobbyRMailOrders();
  }

  BAR(1, L"Final Checks...");

  // ATE: Patch? Patch up groups (will only do for old saves)
  UpdatePersistantGroupsFromOldSave(version);

  if (version <= 40) { /* Cancel all pending purchase orders for BobbyRay's.  Starting with
                        * version 41, the BR orders events are posted with the usItemIndex
                        * itself as the parameter, rather than the inventory slot index.
                        * This was done to make it easier to modify BR's traded inventory
                        * lists later on without breaking saves. */
    CancelAllPendingBRPurchaseOrders();
  }

  // if the world is loaded, apply the temp files to the loaded map
  if (SaveGameHeader.fWorldLoaded || version < 50) {
    try {  // Load the current sectors Information From the temporary files
      LoadCurrentSectorsInformationFromTempItemsFile();
    } catch (...) {
      InitExitGameDialogBecauseFileHackDetected();
      return;
    }
  }

  BAR(1, L"Final Checks...");

  InitAI();

  // Update the mercs in the sector with the new soldier info
  UpdateMercsInSector();

  PostSchedules();

  BAR(1, L"Final Checks...");

  // Reset the lighting level if we are outside
  if (gbWorldSectorZ == 0) LightSetBaseLevel(GetTimeOfDayAmbientLightLevel());

  // if we have been to this sector before
  //	if (SectorInfo[SECTOR(gWorldSectorX, gWorldSectorY)].uiFlags &
  // SF_ALREADY_VISITED)
  {
    // Reset the fact that we are loading a saved game
    gTacticalStatus.uiFlags &= ~LOADING_SAVED_GAME;
  }

  /* CJC January 13: we can't do this because (a) it resets militia IN THE
   * MIDDLE OF COMBAT, and (b) if we add militia to the teams while
   * LOADING_SAVED_GAME is set, the team counters will not be updated properly!
   */
  //	ResetMilitia();

  BAR(1, L"Final Checks...");

  // if the UI was locked in the saved game file
  if (gTacticalStatus.ubAttackBusyCount > 1) {  // Lock the ui
    SetUIBusy(GetSelectedMan());
  }

  // if we succesfully LOADED! the game, mark this entry as the last saved game
  // file
  gGameSettings.bLastSavedGameSlot = save_slot_id;

  // Save the save game settings
  SaveGameSettings();

  BAR(1, L"Final Checks...");

  // Reset the AI Timer clock
  giRTAILastUpdateTime = 0;

  // If we are in tactical
  if (guiScreenToGotoAfterLoadingSavedGame == GAME_SCREEN) {  // Initialize the current panel
    InitializeCurrentPanel();
    SOLDIERTYPE *const sel = GetSelectedMan();
    if (sel) SelectSoldier(sel, SELSOLDIER_FORCE_RESELECT);
  }

  BAR(1, L"Final Checks...");

#undef BAR

  InitalizeStaticExternalNPCFaces();
  LoadCarPortraitValues();

  // OK, turn OFF show all enemies....
  gTacticalStatus.uiFlags &= ~(SHOW_ALL_MERCS | SHOW_ALL_ITEMS);

  InitShutDownMapTempFileTest(FALSE, "LoadMapTempFile", save_slot_id);

  if (gTacticalStatus.uiFlags & INCOMBAT) {
    DebugMsg(TOPIC_JA2, DBG_LEVEL_3, "Setting attack busy count to 0 from load");
    gTacticalStatus.ubAttackBusyCount = 0;
  }

  if (version < 64) { /* Militia/enemies/creature team sizes have been changed
                       * from 32 to 20. This function will simply kill off the
                       * excess.  This will allow the ability to load previous
                       * saves, though there will still be problems, though a
                       * LOT less than there would be without this call. */
    TruncateStrategicGroupSizes();
  }

  // ATE: if we are within this window where skyridder was foobared, fix!
  if (61 <= version && version <= 65 && !fSkyRiderSetUp) {
    // see if we can find him and remove him if so....
    SOLDIERTYPE *const s = FindSoldierByProfileID(SKYRIDER);
    if (s) TacticalRemoveSoldier(*s);

    // add the pilot at a random location!
    int16_t x;
    int16_t y;
    switch (Random(4)) {
      case 0:
        x = 15;
        y = MAP_ROW_B;
        break;
      case 1:
        x = 14;
        y = MAP_ROW_E;
        break;
      case 2:
        x = 12;
        y = MAP_ROW_D;
        break;
      case 3:
        x = 16;
        y = MAP_ROW_C;
        break;
      default:
        abort();  // HACK000E
    }
    MERCPROFILESTRUCT &p = GetProfile(SKYRIDER);
    p.sSectorX = x;
    p.sSectorY = y;
    p.bSectorZ = 0;
  }

  if (version < 68) {
    // correct bVehicleUnderRepairID for all mercs
    FOR_EACH_SOLDIER(s) s->bVehicleUnderRepairID = -1;
  }

  if (version < 73 && LaptopSaveInfo.fMercSiteHasGoneDownYet) {
    LaptopSaveInfo.fFirstVisitSinceServerWentDown = 2;
  }

  /* Update the MERC merc contract length.  Before save version 77 the data was
   * stored in the SOLDIERTYPE, after 77 the data is stored in the profile */
  if (version < 77) {
    UpdateMercMercContractInfo();
  }

  if (version <= 89) { /* ARM: A change was made in version 89 where refuel site
                        * availability now also depends on whether the player
                        * has airspace control over that sector. To update the
                        * settings immediately, must call it here. */
    UpdateRefuelSiteAvailability();
  }

  if (version < 91) {  // Update the amount of money that has been paid to speck
    CalcAproximateAmountPaidToSpeck();
  }

  gfLoadedGame = TRUE;

  uiRelEndPerc = 100;
  SetRelativeStartAndEndPercentage(0, uiRelStartPerc, uiRelEndPerc, L"Done!");
  RenderProgressBar(0, 100);

  RemoveLoadingScreenProgressBar();

  SetMusicMode(gMusicModeToPlay);

  // reset once-per-convo records for everyone in the loaded sector
  ResetOncePerConvoRecordsForAllNPCsInLoadedSector();

  if (!(gTacticalStatus.uiFlags &
        INCOMBAT)) { /* Fix lingering attack busy count problem on loading saved
                      * game by resetting a.b.c if we're not in combat. */
    gTacticalStatus.ubAttackBusyCount = 0;
  }

  // fix squads
  CheckSquadMovementGroups();

  /* The above function LightSetBaseLevel adjusts ALL the level node light
   * values including the merc node, we must reset the values */
  HandlePlayerTogglingLightEffects(FALSE);
}

static void SaveMercProfiles(HWFILE const f) {
  // Loop through all the profiles to save
  void (&writer)(HWFILE, uint8_t const *, uint32_t) =
      guiSavedGameVersion < 87 ? JA2EncryptedFileWrite : NewJA2EncryptedFileWrite;
  FOR_EACH(MERCPROFILESTRUCT, i, gMercProfiles) {
    uint8_t data[716];
    InjectMercProfile(data, *i);
    writer(f, data, sizeof(data));
  }
}

static void LoadSavedMercProfiles(HWFILE const f, uint32_t const savegame_version,
                                  bool stracLinuxFormat) {
  // Loop through all the profiles to load
  void (&reader)(HWFILE, uint8_t *, uint32_t) =
      savegame_version < 87 ? JA2EncryptedFileRead : NewJA2EncryptedFileRead;
  FOR_EACH(MERCPROFILESTRUCT, i, gMercProfiles) {
    uint32_t checksum;
    uint32_t dataSize = stracLinuxFormat ? MERC_PROFILE_SIZE_STRAC_LINUX : MERC_PROFILE_SIZE;
    std::vector<uint8_t> data(dataSize);
    reader(f, data.data(), dataSize);
    ExtractMercProfile(data.data(), *i, stracLinuxFormat, &checksum, NULL);
    if (checksum != SoldierProfileChecksum(*i)) {
      throw std::runtime_error("Merc profile checksum mismatch");
    }
  }
}

static void SaveSoldierStructure(HWFILE const f) {
  // Loop through all the soldier structs to save
  void (&writer)(HWFILE, uint8_t const *, uint32_t) =
      guiSavedGameVersion < 87 ? JA2EncryptedFileWrite : NewJA2EncryptedFileWrite;
  for (uint16_t i = 0; i < TOTAL_SOLDIERS; ++i) {
    SOLDIERTYPE const &s = GetMan(i);

    // If the soldier isn't active, don't add them to the saved game file.
    FileWrite(f, &s.bActive, 1);
    if (!s.bActive) continue;

    // Save the soldier structure
    uint8_t data[2328];
    InjectSoldierType(data, &s);
    writer(f, data, sizeof(data));

    // Save all the pointer info from the structure
    SaveMercPath(f, s.pMercPath);

    // Save the key ring
    uint8_t const has_keyring = s.pKeyRing != 0;
    FileWrite(f, &has_keyring, sizeof(has_keyring));
    if (!has_keyring) continue;
    FileWrite(f, s.pKeyRing, NUM_KEYS * sizeof(KEY_ON_RING));
  }
}

static void LoadSoldierStructure(HWFILE const f, uint32_t savegame_version, bool stracLinuxFormat) {
  // Loop through all the soldier and delete them all
  FOR_EACH_SOLDIER(i) TacticalRemoveSoldier(*i);

  // Loop through all the soldier structs to load
  void (&reader)(HWFILE, uint8_t *, uint32_t) =
      savegame_version < 87 ? JA2EncryptedFileRead : NewJA2EncryptedFileRead;
  for (uint16_t i = 0; i < TOTAL_SOLDIERS; ++i) {
    // Update the progress bar
    uint32_t const percentage = (i * 100) / (TOTAL_SOLDIERS - 1);
    RenderProgressBar(0, percentage);

    // Read in a byte to tell us whether or not there is a soldier loaded here.
    uint8_t active;
    FileRead(f, &active, 1);
    if (!active) continue;

    // Read in the saved soldier info into a Temp structure
    SOLDIERTYPE SavedSoldierInfo;
    if (stracLinuxFormat) {
      uint8_t Data[2352];
      reader(f, Data, sizeof(Data));
      ExtractSoldierType(Data, &SavedSoldierInfo, stracLinuxFormat);
    } else {
      uint8_t Data[2328];
      reader(f, Data, sizeof(Data));
      ExtractSoldierType(Data, &SavedSoldierInfo, stracLinuxFormat);
    }

    SOLDIERTYPE *const s = TacticalCreateSoldierFromExisting(&SavedSoldierInfo);
    Assert(s->ubID == i);

    LoadMercPath(f, &s->pMercPath);

    // Read the file to see if we have to load the keys
    uint8_t has_keyring;
    FileRead(f, &has_keyring, 1);
    if (has_keyring) {
      // Now Load the ....
      FileRead(f, s->pKeyRing, NUM_KEYS * sizeof(KEY_ON_RING));
    } else {
      Assert(s->pKeyRing == NULL);
    }

    // if the soldier is an IMP character
    if (s->ubWhatKindOfMercAmI == MERC_TYPE__PLAYER_CHARACTER && s->bTeam == OUR_TEAM) {
      ResetIMPCharactersEyesAndMouthOffsets(s->ubProfile);
    }

    // If the saved game version is before x, calculate the amount of money paid
    // to mercs
    if (savegame_version < 83 && s->ubProfile != NO_PROFILE) {
      MERCPROFILESTRUCT &p = GetProfile(s->ubProfile);
      if (s->ubWhatKindOfMercAmI == MERC_TYPE__MERC) {
        p.uiTotalCostToDate = p.sSalary * p.iMercMercContractLength;
      } else {
        p.uiTotalCostToDate = p.sSalary * s->iTotalContractLength;
      }
    }

    if (isGermanVersion()) {
      // Fix neutral flags
      if (savegame_version < 94 && s->bTeam == OUR_TEAM && s->bNeutral &&
          s->bAssignment != ASSIGNMENT_POW) {
        // turn off neutral flag
        s->bNeutral = FALSE;
      }
    }
    // JA2Gold: fix next-to-previous attacker value
    if (savegame_version < 99) {
      s->next_to_previous_attacker = NULL;
    }
  }

  // Fix robot
  if (savegame_version <= 87) {
    MERCPROFILESTRUCT &robot_p = GetProfile(ROBOT);
    if (robot_p.inv[VESTPOS] == SPECTRA_VEST) {
      // update this
      robot_p.inv[VESTPOS] = SPECTRA_VEST_18;
      robot_p.inv[HELMETPOS] = SPECTRA_HELMET_18;
      robot_p.inv[LEGPOS] = SPECTRA_LEGGINGS_18;
      robot_p.bAgility = 50;
      SOLDIERTYPE *const robot_s = FindSoldierByProfileID(ROBOT);
      if (robot_s) {
        robot_s->inv[VESTPOS].usItem = SPECTRA_VEST_18;
        robot_s->inv[HELMETPOS].usItem = SPECTRA_HELMET_18;
        robot_s->inv[LEGPOS].usItem = SPECTRA_LEGGINGS_18;
        robot_s->bAgility = 50;
      }
    }
  }
}

// static void SaveFileToSavedGame(SGPFile* fileToSave, HWFILE const hFile)
// {
// 	AutoSGPFile hSrcFile(FileMan::openForReadingSmart(pSrcFileName, true));

// 	//Get the file size of the source data file
// 	uint32_t uiFileSize = FileGetSize( fileToSave );

// 	// Write the the size of the file to the saved game file
// 	FileWrite(hFile, &uiFileSize, sizeof(uint32_t));

// 	if (uiFileSize == 0) return;

// 	// Read the saource file into the buffer
// 	SGP::Buffer<uint8_t> pData(uiFileSize);
// 	FileRead(fileToSave, pData, uiFileSize);

// 	// Write the buffer to the saved game file
// 	FileWrite(hFile, pData, uiFileSize);
// }

// static void SaveTempFileToSavedGame(const char* fileName, HWFILE const hFile)
// {
// 	AutoSGPFile fileToSave(GCM->openTempFileForReading(fileName));
//   SaveFileToSavedGame(fileToSave, hFile);
// }

void SaveFilesToSavedGame(char const *const pSrcFileName, HWFILE const hFile) {
  AutoSGPFile hSrcFile(FileMan::openForReadingSmart(pSrcFileName, true));

  // Get the file size of the source data file
  uint32_t uiFileSize = FileGetSize(hSrcFile);

  // Write the the size of the file to the saved game file
  FileWrite(hFile, &uiFileSize, sizeof(uint32_t));

  if (uiFileSize == 0) return;

  // Read the saource file into the buffer
  SGP::Buffer<uint8_t> pData(uiFileSize);
  FileRead(hSrcFile, pData, uiFileSize);

  // Write the buffer to the saved game file
  FileWrite(hFile, pData, uiFileSize);
}

static void LoadFileFromSavedGame(SGPFile *fileToWrite, HWFILE const hFile) {
  // Read the size of the data
  uint32_t uiFileSize;
  FileRead(hFile, &uiFileSize, sizeof(uint32_t));

  if (uiFileSize == 0) return;

  // Read into the buffer
  SGP::Buffer<uint8_t> pData(uiFileSize);
  FileRead(hFile, pData, uiFileSize);

  // Write the buffer to the new file
  FileWrite(fileToWrite, pData, uiFileSize);
}

// void LoadTempFileFromSavedGame(const char* tempFileName, HWFILE const hFile)
// {
// 	AutoSGPFile fileToWrite(GCM->openTempFileForWriting(tempFileName,
// true));
//   LoadFileFromSavedGame(fileToWrite, hFile);
// }

void LoadFilesFromSavedGame(char const *const pSrcFileName, HWFILE const hFile) {
  AutoSGPFile hSrcFile(FileMan::openForWriting(pSrcFileName));
  LoadFileFromSavedGame(hSrcFile, hFile);
}

static void SaveTacticalStatusToSavedGame(HWFILE const f) {
  InjectTacticalStatusTypeIntoFile(f);

  // Save the current sector location
  uint8_t data[5];
  uint8_t *d = data;
  INJ_I16(d, gWorldSectorX)
  INJ_I16(d, gWorldSectorY)
  INJ_I8(d, gbWorldSectorZ)
  Assert(d == endof(data));

  FileWrite(f, data, sizeof(data));
}

static void LoadTacticalStatusFromSavedGame(HWFILE const f, bool stracLinuxFormat) {
  ExtractTacticalStatusTypeFromFile(f, stracLinuxFormat);

  // Load the current sector location
  uint8_t data[5];
  FileRead(f, data, sizeof(data));

  uint8_t const *d = data;
  EXTR_I16(d, gWorldSectorX)
  EXTR_I16(d, gWorldSectorY)
  EXTR_I8(d, gbWorldSectorZ)
  Assert(d == endof(data));
}

static void SaveOppListInfoToSavedGame(HWFILE const hFile) {
  uint32_t uiSaveSize = 0;

  // Save the Public Opplist
  uiSaveSize = MAXTEAMS * TOTAL_SOLDIERS;
  FileWrite(hFile, gbPublicOpplist, uiSaveSize);

  // Save the Seen Oppenents
  uiSaveSize = TOTAL_SOLDIERS * TOTAL_SOLDIERS;
  FileWrite(hFile, gbSeenOpponents, uiSaveSize);

  // Save the Last Known Opp Locations
  uiSaveSize = TOTAL_SOLDIERS * TOTAL_SOLDIERS;  // XXX TODO000F
  FileWrite(hFile, gsLastKnownOppLoc, uiSaveSize);

  // Save the Last Known Opp Level
  uiSaveSize = TOTAL_SOLDIERS * TOTAL_SOLDIERS;
  FileWrite(hFile, gbLastKnownOppLevel, uiSaveSize);

  // Save the Public Last Known Opp Locations
  uiSaveSize = MAXTEAMS * TOTAL_SOLDIERS;  // XXX TODO000F
  FileWrite(hFile, gsPublicLastKnownOppLoc, uiSaveSize);

  // Save the Public Last Known Opp Level
  uiSaveSize = MAXTEAMS * TOTAL_SOLDIERS;
  FileWrite(hFile, gbPublicLastKnownOppLevel, uiSaveSize);

  // Save the Public Noise Volume
  uiSaveSize = MAXTEAMS;
  FileWrite(hFile, gubPublicNoiseVolume, uiSaveSize);

  // Save the Public Last Noise Gridno
  uiSaveSize = MAXTEAMS;  // XXX TODO000F
  FileWrite(hFile, gsPublicNoiseGridno, uiSaveSize);
}

static void LoadOppListInfoFromSavedGame(HWFILE const hFile) {
  uint32_t uiLoadSize = 0;

  // Load the Public Opplist
  uiLoadSize = MAXTEAMS * TOTAL_SOLDIERS;
  FileRead(hFile, gbPublicOpplist, uiLoadSize);

  // Load the Seen Oppenents
  uiLoadSize = TOTAL_SOLDIERS * TOTAL_SOLDIERS;
  FileRead(hFile, gbSeenOpponents, uiLoadSize);

  // Load the Last Known Opp Locations
  uiLoadSize = TOTAL_SOLDIERS * TOTAL_SOLDIERS;  // XXX TODO000F
  FileRead(hFile, gsLastKnownOppLoc, uiLoadSize);

  // Load the Last Known Opp Level
  uiLoadSize = TOTAL_SOLDIERS * TOTAL_SOLDIERS;
  FileRead(hFile, gbLastKnownOppLevel, uiLoadSize);

  // Load the Public Last Known Opp Locations
  uiLoadSize = MAXTEAMS * TOTAL_SOLDIERS;  // XXX TODO000F
  FileRead(hFile, gsPublicLastKnownOppLoc, uiLoadSize);

  // Load the Public Last Known Opp Level
  uiLoadSize = MAXTEAMS * TOTAL_SOLDIERS;
  FileRead(hFile, gbPublicLastKnownOppLevel, uiLoadSize);

  // Load the Public Noise Volume
  uiLoadSize = MAXTEAMS;
  FileRead(hFile, gubPublicNoiseVolume, uiLoadSize);

  // Load the Public Last Noise Gridno
  uiLoadSize = MAXTEAMS;  // XXX TODO000F
  FileRead(hFile, gsPublicNoiseGridno, uiLoadSize);
}

static void SaveWatchedLocsToSavedGame(HWFILE const hFile) {
  uint32_t uiArraySize;
  uint32_t uiSaveSize = 0;

  uiArraySize = TOTAL_SOLDIERS * NUM_WATCHED_LOCS;

  // save locations of watched points
  uiSaveSize = uiArraySize * sizeof(int16_t);
  FileWrite(hFile, gsWatchedLoc, uiSaveSize);

  uiSaveSize = uiArraySize * sizeof(int8_t);

  FileWrite(hFile, gbWatchedLocLevel, uiSaveSize);

  FileWrite(hFile, gubWatchedLocPoints, uiSaveSize);

  FileWrite(hFile, gfWatchedLocReset, uiSaveSize);
}

static void LoadWatchedLocsFromSavedGame(HWFILE const hFile) {
  uint32_t uiArraySize;
  uint32_t uiLoadSize = 0;

  uiArraySize = TOTAL_SOLDIERS * NUM_WATCHED_LOCS;

  uiLoadSize = uiArraySize * sizeof(int16_t);
  FileRead(hFile, gsWatchedLoc, uiLoadSize);

  uiLoadSize = uiArraySize * sizeof(int8_t);
  FileRead(hFile, gbWatchedLocLevel, uiLoadSize);

  FileRead(hFile, gubWatchedLocPoints, uiLoadSize);

  FileRead(hFile, gfWatchedLocReset, uiLoadSize);
}

void CreateSavedGameFileNameFromNumber(const uint8_t ubSaveGameID, char *const pzNewFileName) {
  char const *const dir = g_savegame_dir;
  char const *const ext = g_savegame_ext;

  switch (ubSaveGameID) {
    case 0:  // we are creating the QuickSave file
    {
      char const *const quick = g_quicksave_name;
      sprintf(pzNewFileName, "%s/%s.%s", dir, quick, ext);
      break;
    }

    case SAVE__END_TURN_NUM:
      sprintf(pzNewFileName, "%s/Auto%02d.%s", dir, guiLastSaveGameNum, ext);
      guiLastSaveGameNum = (guiLastSaveGameNum + 1) % 2;
      break;

    case SAVE__ERROR_NUM:
      sprintf(pzNewFileName, "%s/error.%s", dir, ext);
      break;

    default:
      sprintf(pzNewFileName, "%s/%s%02d.%s", dir, g_savegame_name, ubSaveGameID, ext);
      break;
  }
}

void SaveMercPath(HWFILE const f, PathSt const *const head) {
  uint32_t n_nodes = 0;
  for (const PathSt *p = head; p != NULL; p = p->pNext) ++n_nodes;
  FileWrite(f, &n_nodes, sizeof(uint32_t));

  for (const PathSt *p = head; p != NULL; p = p->pNext) {
    uint8_t data[20];
    uint8_t *d = data;
    INJ_U32(d, p->uiSectorId)
    INJ_SKIP(d, 16)
    Assert(d == endof(data));

    FileWrite(f, data, sizeof(data));
  }
}

void LoadMercPath(HWFILE const hFile, PathSt **const head) {
  // Load the number of the nodes
  uint32_t uiNumOfNodes = 0;
  FileRead(hFile, &uiNumOfNodes, sizeof(uint32_t));

  // load all the nodes
  PathSt *path = NULL;
  for (uint32_t cnt = 0; cnt < uiNumOfNodes; ++cnt) {
    PathSt *const n = MALLOC(PathSt);

    uint8_t data[20];
    FileRead(hFile, data, sizeof(data));

    const uint8_t *d = data;
    EXTR_U32(d, n->uiSectorId)
    EXTR_SKIP(d, 16)
    Assert(d == endof(data));

    n->pPrev = path;
    n->pNext = NULL;

    // Put the node into the list
    if (path == NULL) {
      *head = n;
    } else {
      path->pNext = n;
    }
    path = n;
  }
}

static uint8_t *InjectMeanwhileDefinition(uint8_t *const data, MEANWHILE_DEFINITION const &m) {
  uint8_t *d = data;
  INJ_I16(d, m.sSectorX)
  INJ_I16(d, m.sSectorY)
  INJ_U16(d, m.usTriggerEvent)
  INJ_U8(d, m.ubMeanwhileID)
  INJ_U8(d, m.ubNPCNumber)
  Assert(d == data + 8);
  return d;
}

static uint8_t const *ExtractMeanwhileDefinition(uint8_t const *const data,
                                                 MEANWHILE_DEFINITION &m) {
  uint8_t const *d = data;
  EXTR_I16(d, m.sSectorX)
  EXTR_I16(d, m.sSectorY)
  EXTR_U16(d, m.usTriggerEvent)
  EXTR_U8(d, m.ubMeanwhileID)
  EXTR_U8(d, m.ubNPCNumber)
  Assert(d == data + 8);
  return d;
}

static void SaveGeneralInfo(HWFILE const f) {
  uint8_t data[1024];
  uint8_t *d = data;
  INJ_U32(d, guiPreviousOptionScreen)
  INJ_U32(d, guiCurrentUniqueSoldierId)
  INJ_U8(d, gubMusicMode)
  INJ_SKIP(d, 1)
  INJ_U16(d, Soldier2ID(GetSelectedMan()))
  INJ_I16(d, gsRenderCenterX)
  INJ_I16(d, gsRenderCenterY)
  INJ_BOOL(d, gfAtLeastOneMercWasHired)
  INJ_BOOL(d, fShowItemsFlag)
  INJ_BOOL(d, fShowTownFlag)
  INJ_BOOL(d, fShowTeamFlag)
  INJ_BOOL(d, fShowMineFlag)
  INJ_BOOL(d, fShowAircraftFlag)
  INJ_BOOL(d, iHelicopterVehicleId != -1)  // XXX HACK000B
  INJ_SKIP(d, 1)
  INJ_I32(d, iHelicopterVehicleId)
  INJ_SKIP(d, 4)
  INJ_I32(d, iTotalAccumulatedCostByPlayer)
  INJ_BOOL(d, iHelicopterVehicleId != -1)  // XXX HACK000B
  INJ_SKIP(d, 9)
  INJ_BOOL(d, fHelicopterIsAirBorne)
  INJ_BOOL(d, fHeliReturnStraightToBase)
  INJ_BOOL(d, fHoveringHelicopter)
  INJ_SKIP(d, 3)
  INJ_U32(d, uiStartHoverTime)
  INJ_U32(d, guiHelicopterSkyriderTalkState)
  INJ_BOOL(d, fShowEstoniRefuelHighLight)
  INJ_BOOL(d, fShowOtherSAMHighLight)
  INJ_BOOL(d, fShowDrassenSAMHighLight)
  INJ_SKIP(d, 1)
  INJ_U32(d, guiEnvWeather)
  INJ_U8(d, gubDefaultButton)
  INJ_BOOL(d, gfSkyriderEmptyHelpGiven)
  INJ_BOOL(d, fEnterMapDueToContract)
  INJ_U8(d, gubHelicopterHitsTaken)
  INJ_SKIP(d, 1)
  INJ_BOOL(d, gfSkyriderSaidCongratsOnTakingSAM)
  INJ_I16(d, pContractReHireSoldier ? pContractReHireSoldier->ubID : -1)
  d = InjectGameOptions(d, gGameOptions);
  INJ_SKIP(d, 4)
  INJ_U32(d, guiBaseJA2Clock)
  INJ_I16(d, gsCurInterfacePanel)
  INJ_U8(d, gpSMCurrentMerc ? gpSMCurrentMerc->ubID : 255)
  INJ_SKIP(d, 1)
  INJ_BOOL(d, fDisableDueToBattleRoster)
  INJ_BOOL(d, fDisableMapInterfaceDueToBattle)
  INJ_I16A(d, gsBoxerGridNo, lengthof(gsBoxerGridNo))
  FOR_EACH(SOLDIERTYPE *const, i, gBoxer){INJ_SOLDIER(d, *i)} INJ_BOOLA(
      d, gfBoxerFought, lengthof(gfBoxerFought)) INJ_BOOL(d, fHelicopterDestroyed) INJ_SKIP(d, 1)
      INJ_I32(d, giSortStateForMapScreenList) INJ_BOOL(d, fFoundTixa) INJ_SKIP(d, 3)
          INJ_U32(d, guiTimeOfLastSkyriderMonologue) INJ_BOOL(d, fShowCambriaHospitalHighLight)
              INJ_BOOL(d, fSkyRiderSetUp)
                  INJ_BOOLA(d, fRefuelingSiteAvailable, lengthof(fRefuelingSiteAvailable)) d =
                      InjectMeanwhileDefinition(d, gCurrentMeanwhileDef);
  INJ_BOOL(d, gubPlayerProgressSkyriderLastCommentedOn)
  INJ_BOOL(d, gfMeanwhileTryingToStart)
  INJ_BOOL(d, gfInMeanwhile)
  INJ_SKIP(d, 1)
  for (int16_t(*i)[NUMBER_OF_SOLDIERS_PER_SQUAD] = sDeadMercs; i != endof(sDeadMercs); ++i) {
    INJ_I16A(d, *i, lengthof(*i))
  }
  INJ_I8A(d, gbPublicNoiseLevel, lengthof(gbPublicNoiseLevel))
  INJ_U8(d, gubScreenCount)
  INJ_SKIP(d, 5)
  INJ_I32(d, iPortraitNumber)
  INJ_I16(d, sWorldSectorLocationOfFirstBattle)
  INJ_BOOL(d, fUnReadMailFlag)
  INJ_BOOL(d, fNewMailFlag)
  INJ_BOOL(d, FALSE)  // XXX HACK000B
  INJ_BOOL(d, fOldNewMailFlag)
  INJ_BOOL(d, fShowMilitia)
  INJ_BOOL(d, fNewFilesInFileViewer)
  INJ_BOOL(d, gfLastBoxingMatchWonByPlayer)
  INJ_SKIP(d, 7)
  INJ_BOOLA(d, fSamSiteFound, lengthof(fSamSiteFound))
  INJ_U8(d, gubNumTerrorists)
  INJ_U8(d, gubCambriaMedicalObjects)
  INJ_BOOL(d, gfDisableTacticalPanelButtons)
  INJ_SKIP(d, 1)
  INJ_I16(d, sSelMapX)
  INJ_I16(d, sSelMapY)
  INJ_I32(d, iCurrentMapSectorZ)
  INJ_U16(d, gHelpScreen.usHasPlayerSeenHelpScreenInCurrentScreen)
  INJ_SKIP(d, 1)
  INJ_U8(d, gubBoxingMatchesWon)
  INJ_U8(d, gubBoxersRests)
  INJ_BOOL(d, gfBoxersResting)
  INJ_U8(d, gubDesertTemperature)
  INJ_U8(d, gubGlobalTemperature)
  INJ_I16(d, SECTORX(g_merc_arrive_sector))
  INJ_I16(d, SECTORY(g_merc_arrive_sector))
  INJ_BOOL(d, gfCreatureMeanwhileScenePlayed)
  INJ_SKIP_U8(d)
  INJ_BOOL(d, gfPersistantPBI)
  INJ_U8(d, gubEnemyEncounterCode)
  INJ_BOOL(d, gubExplicitEnemyEncounterCode)
  INJ_BOOL(d, gfBlitBattleSectorLocator)
  INJ_U8(d, gubPBSectorX)
  INJ_U8(d, gubPBSectorY)
  INJ_U8(d, gubPBSectorZ)
  INJ_BOOL(d, gfCantRetreatInPBI)
  INJ_BOOL(d, gfExplosionQueueActive)
  INJ_SKIP(d, 1)
  INJ_U32(d, uiMeanWhileFlags)
  INJ_I8(d, bSelectedInfoChar)
  INJ_I8(d, gbHospitalPriceModifier)
  INJ_SKIP(d, 2)
  INJ_I32(d, giHospitalTempBalance)
  INJ_I32(d, giHospitalRefund)
  INJ_I8(d, gfPlayerTeamSawJoey)
  INJ_I8(d, gfMikeShouldSayHi)
  INJ_SKIP(d, 550)
  Assert(d == endof(data));

  FileWrite(f, data, sizeof(data));
}

static void LoadGeneralInfo(HWFILE const f, uint32_t const savegame_version) {
  uint8_t data[1024];
  FileRead(f, data, sizeof(data));

  uint8_t const *d = data;
  uint32_t screen_after_loading;
  EXTR_U32(d, screen_after_loading)
  guiScreenToGotoAfterLoadingSavedGame =
      static_cast<ScreenID>(screen_after_loading);  // XXX TODO001A unchecked conversion
  EXTR_U32(d, guiCurrentUniqueSoldierId)
  EXTR_U8(d, gMusicModeToPlay)
  EXTR_SKIP(d, 1)
  uint16_t sel;
  EXTR_U16(d, sel)
  SetSelectedMan(ID2Soldier(sel));
  EXTR_I16(d, gsRenderCenterX)
  EXTR_I16(d, gsRenderCenterY)
  EXTR_BOOL(d, gfAtLeastOneMercWasHired)
  EXTR_BOOL(d, fShowItemsFlag)
  EXTR_BOOL(d, fShowTownFlag)
  EXTR_BOOL(d, fShowTeamFlag)
  EXTR_BOOL(d, fShowMineFlag)
  EXTR_BOOL(d, fShowAircraftFlag)
  EXTR_SKIP(d, 2)
  EXTR_I32(d, iHelicopterVehicleId)
  EXTR_SKIP(d, 4)
  EXTR_I32(d, iTotalAccumulatedCostByPlayer)
  EXTR_SKIP(d, 10)
  EXTR_BOOL(d, fHelicopterIsAirBorne)
  EXTR_BOOL(d, fHeliReturnStraightToBase)
  EXTR_BOOL(d, fHoveringHelicopter)
  EXTR_SKIP(d, 3)
  EXTR_U32(d, uiStartHoverTime)
  EXTR_U32(d, guiHelicopterSkyriderTalkState)
  EXTR_BOOL(d, fShowEstoniRefuelHighLight)
  EXTR_BOOL(d, fShowOtherSAMHighLight)
  EXTR_BOOL(d, fShowDrassenSAMHighLight)
  EXTR_SKIP(d, 1)
  EXTR_U32(d, guiEnvWeather)
  EXTR_U8(d, gubDefaultButton)
  EXTR_BOOL(d, gfSkyriderEmptyHelpGiven)
  EXTR_BOOL(d, fEnterMapDueToContract)
  EXTR_U8(d, gubHelicopterHitsTaken)
  EXTR_SKIP(d, 1)
  EXTR_BOOL(d, gfSkyriderSaidCongratsOnTakingSAM)
  int16_t contract_rehire_soldier;
  EXTR_I16(d, contract_rehire_soldier)
  pContractReHireSoldier = contract_rehire_soldier != -1 ? &GetMan(contract_rehire_soldier) : 0;
  d = ExtractGameOptions(d, gGameOptions);
  EXTR_SKIP(d, 4)
  EXTR_U32(d, guiBaseJA2Clock)
  ResetJA2ClockGlobalTimers();
  int16_t cur_interface_panel;
  EXTR_I16(d, cur_interface_panel)
  uint8_t sm_current_merc;
  EXTR_U8(d, sm_current_merc)
  gpSMCurrentMerc = sm_current_merc != 255 ? &GetMan(sm_current_merc) : 0;
  // Set the interface panel to the team panel
  ShutdownCurrentPanel();
  gsCurInterfacePanel =
      static_cast<InterfacePanelKind>(cur_interface_panel);  // XXX TODO001A unchecked conversion
  EXTR_SKIP(d, 1)
  EXTR_BOOL(d, fDisableDueToBattleRoster)
  EXTR_BOOL(d, fDisableMapInterfaceDueToBattle)
  EXTR_I16A(d, gsBoxerGridNo, lengthof(gsBoxerGridNo))
  FOR_EACH(SOLDIERTYPE *, i, gBoxer){EXTR_SOLDIER(d, *i)} EXTR_BOOLA(
      d, gfBoxerFought, lengthof(gfBoxerFought)) EXTR_BOOL(d, fHelicopterDestroyed) EXTR_SKIP(d, 1)
      EXTR_I32(d, giSortStateForMapScreenList) EXTR_BOOL(d, fFoundTixa) EXTR_SKIP(d, 3)
          EXTR_U32(d, guiTimeOfLastSkyriderMonologue) EXTR_BOOL(d, fShowCambriaHospitalHighLight)
              EXTR_BOOL(d, fSkyRiderSetUp)
                  EXTR_BOOLA(d, fRefuelingSiteAvailable, lengthof(fRefuelingSiteAvailable)) d =
                      ExtractMeanwhileDefinition(d, gCurrentMeanwhileDef);
  EXTR_BOOL(d, gubPlayerProgressSkyriderLastCommentedOn)
  EXTR_BOOL(d, gfMeanwhileTryingToStart)
  EXTR_BOOL(d, gfInMeanwhile)
  EXTR_SKIP(d, 1)
  for (int16_t(*i)[NUMBER_OF_SOLDIERS_PER_SQUAD] = sDeadMercs; i != endof(sDeadMercs); ++i) {
    EXTR_I16A(d, *i, lengthof(*i))
  }
  EXTR_I8A(d, gbPublicNoiseLevel, lengthof(gbPublicNoiseLevel))
  EXTR_U8(d, gubScreenCount)
  EXTR_SKIP(d, 1)
  uint16_t old_meanwhile_flags;
  EXTR_U16(d, old_meanwhile_flags)
  EXTR_SKIP(d, 2)
  EXTR_I32(d, iPortraitNumber)
  EXTR_I16(d, sWorldSectorLocationOfFirstBattle)
  EXTR_BOOL(d, fUnReadMailFlag)
  EXTR_BOOL(d, fNewMailFlag)
  EXTR_SKIP(d, 1)
  EXTR_BOOL(d, fOldNewMailFlag)
  EXTR_BOOL(d, fShowMilitia)
  EXTR_BOOL(d, fNewFilesInFileViewer)
  EXTR_BOOL(d, gfLastBoxingMatchWonByPlayer)
  EXTR_SKIP(d, 7)
  EXTR_BOOLA(d, fSamSiteFound, lengthof(fSamSiteFound))
  EXTR_U8(d, gubNumTerrorists)
  EXTR_U8(d, gubCambriaMedicalObjects)
  EXTR_BOOL(d, gfDisableTacticalPanelButtons)
  EXTR_SKIP(d, 1)
  EXTR_I16(d, sSelMapX)
  EXTR_I16(d, sSelMapY)
  EXTR_I32(d, iCurrentMapSectorZ)
  EXTR_U16(d, gHelpScreen.usHasPlayerSeenHelpScreenInCurrentScreen)
  EXTR_SKIP(d, 1)
  EXTR_U8(d, gubBoxingMatchesWon)
  EXTR_U8(d, gubBoxersRests)
  EXTR_BOOL(d, gfBoxersResting)
  EXTR_U8(d, gubDesertTemperature)
  EXTR_U8(d, gubGlobalTemperature)
  int16_t merc_arrive_x;
  int16_t merc_arrive_y;
  EXTR_I16(d, merc_arrive_x)
  EXTR_I16(d, merc_arrive_y)
  g_merc_arrive_sector =
      1 <= merc_arrive_x && merc_arrive_x <= 16 && 1 <= merc_arrive_y && merc_arrive_y <= 16
          ? SECTOR(merc_arrive_x, merc_arrive_y)
          : START_SECTOR;
  EXTR_BOOL(d, gfCreatureMeanwhileScenePlayed)
  EXTR_SKIP_U8(d)
  EXTR_BOOL(d, gfPersistantPBI)
  EXTR_U8(d, gubEnemyEncounterCode)
  EXTR_BOOL(d, gubExplicitEnemyEncounterCode)
  EXTR_BOOL(d, gfBlitBattleSectorLocator)
  EXTR_U8(d, gubPBSectorX)
  EXTR_U8(d, gubPBSectorY)
  EXTR_U8(d, gubPBSectorZ)
  EXTR_BOOL(d, gfCantRetreatInPBI)
  EXTR_BOOL(d, gfExplosionQueueActive)
  EXTR_SKIP(d, 1)
  uint32_t meanwhile_flags;
  EXTR_U32(d, meanwhile_flags)
  uiMeanWhileFlags = savegame_version < 71 ? old_meanwhile_flags : meanwhile_flags;
  EXTR_I8(d, bSelectedInfoChar)
  EXTR_I8(d, gbHospitalPriceModifier)
  EXTR_SKIP(d, 2)
  EXTR_I32(d, giHospitalTempBalance)
  EXTR_I32(d, giHospitalRefund)
  EXTR_I8(d, gfPlayerTeamSawJoey)
  EXTR_I8(d, gfMikeShouldSayHi)
  EXTR_SKIP(d, 550)
  Assert(d == endof(data));
}

static void SavePreRandomNumbersToSaveGameFile(HWFILE const hFile) {
  // Save the Prerandom number index
  FileWrite(hFile, &guiPreRandomIndex, sizeof(uint32_t));

  // Save the Prerandom number index
  FileWrite(hFile, guiPreRandomNums, sizeof(uint32_t) * MAX_PREGENERATED_NUMS);
}

static void LoadPreRandomNumbersFromSaveGameFile(HWFILE const hFile) {
  // Load the Prerandom number index
  FileRead(hFile, &guiPreRandomIndex, sizeof(uint32_t));

  // Load the Prerandom number index
  FileRead(hFile, guiPreRandomNums, sizeof(uint32_t) * MAX_PREGENERATED_NUMS);
}

static void LoadMeanwhileDefsFromSaveGameFile(HWFILE const f, uint32_t const savegame_version) {
  MEANWHILE_DEFINITION const *end;
  if (savegame_version < 72) {
    memset(&gMeanwhileDef[NUM_MEANWHILES - 1], 0, sizeof(gMeanwhileDef[NUM_MEANWHILES - 1]));
    end = gMeanwhileDef + NUM_MEANWHILES - 1;
  } else {
    end = endof(gMeanwhileDef);
  }
  for (MEANWHILE_DEFINITION *i = gMeanwhileDef; i != end; ++i) {
    uint8_t data[8];
    FileRead(f, data, sizeof(data));
    ExtractMeanwhileDefinition(data, *i);
  }
}

static void SaveMeanwhileDefsToSaveGameFile(HWFILE const f) {
  FOR_EACH(MEANWHILE_DEFINITION, i, gMeanwhileDef) {
    uint8_t data[8];
    InjectMeanwhileDefinition(data, *i);
    FileWrite(f, data, sizeof(data));
  }
}

BOOLEAN DoesUserHaveEnoughHardDriveSpace() {
  uint32_t uiBytesFree = 0;

  uiBytesFree = GetFreeSpaceOnHardDriveWhereGameIsRunningFrom();

  // check to see if there is enough hard drive space
  if (uiBytesFree < REQUIRED_FREE_SPACE) {
    return (FALSE);
  }

  return (TRUE);
}

void GetBestPossibleSectorXYZValues(int16_t *const psSectorX, int16_t *const psSectorY,
                                    int8_t *const pbSectorZ) {
  // if the current sector is valid
  if (gfWorldLoaded) {
    *psSectorX = gWorldSectorX;
    *psSectorY = gWorldSectorY;
    *pbSectorZ = gbWorldSectorZ;
    return;
  }

  if (iCurrentTacticalSquad != NO_CURRENT_SQUAD) {
    const SOLDIERTYPE *const s = Squad[iCurrentTacticalSquad][0];
    if (s != NULL && s->bAssignment != IN_TRANSIT) {
      *psSectorX = s->sSectorX;
      *psSectorY = s->sSectorY;
      *pbSectorZ = s->bSectorZ;
      return;
    }
  }

  // loop through all the mercs on the players team to find the one that is not
  // moving
  CFOR_EACH_IN_TEAM(s, OUR_TEAM) {
    if (s->bAssignment != IN_TRANSIT && !s->fBetweenSectors) {
      // we found an alive, merc that is not moving
      *psSectorX = s->sSectorX;
      *psSectorY = s->sSectorY;
      *pbSectorZ = s->bSectorZ;
      return;
    }
  }

  // loop through all the mercs and find one that is moving
  CFOR_EACH_IN_TEAM(s, OUR_TEAM) {
    // we found an alive, merc that is not moving
    *psSectorX = s->sSectorX;
    *psSectorY = s->sSectorY;
    *pbSectorZ = s->bSectorZ;
    return;
  }

  // if we STILL havent found a merc, give up and use the -1, -1, -1
  *psSectorX = gWorldSectorX;
  *psSectorY = gWorldSectorY;
  *pbSectorZ = gbWorldSectorZ;
}

static void PauseBeforeSaveGame() {
  // if we are not in the save load screen
  if (guiCurrentScreen != SAVE_LOAD_SCREEN) {
    // Pause the game
    PauseGame();
  }
}

static void UnPauseAfterSaveGame() {
  // if we are not in the save load screen
  if (guiCurrentScreen != SAVE_LOAD_SCREEN) {
    // UnPause time compression
    UnPauseGame();
  }
}

static void TruncateStrategicGroupSizes() {
  SECTORINFO *pSector;
  int32_t i;
  for (i = SEC_A1; i < SEC_P16; i++) {
    pSector = &SectorInfo[i];
    if (pSector->ubNumAdmins + pSector->ubNumTroops + pSector->ubNumElites >
        MAX_STRATEGIC_TEAM_SIZE) {
      if (pSector->ubNumAdmins > pSector->ubNumTroops) {
        if (pSector->ubNumAdmins > pSector->ubNumElites) {
          pSector->ubNumAdmins = 20;
          pSector->ubNumTroops = 0;
          pSector->ubNumElites = 0;
        } else {
          pSector->ubNumAdmins = 0;
          pSector->ubNumTroops = 0;
          pSector->ubNumElites = 20;
        }
      } else if (pSector->ubNumTroops > pSector->ubNumElites) {
        if (pSector->ubNumTroops > pSector->ubNumAdmins) {
          pSector->ubNumAdmins = 0;
          pSector->ubNumTroops = 20;
          pSector->ubNumElites = 0;
        } else {
          pSector->ubNumAdmins = 20;
          pSector->ubNumTroops = 0;
          pSector->ubNumElites = 0;
        }
      } else {
        if (pSector->ubNumElites > pSector->ubNumTroops) {
          pSector->ubNumAdmins = 0;
          pSector->ubNumTroops = 0;
          pSector->ubNumElites = 20;
        } else {
          pSector->ubNumAdmins = 0;
          pSector->ubNumTroops = 20;
          pSector->ubNumElites = 0;
        }
      }
    }
    // militia
    if (pSector->ubNumberOfCivsAtLevel[0] + pSector->ubNumberOfCivsAtLevel[1] +
            pSector->ubNumberOfCivsAtLevel[2] >
        MAX_STRATEGIC_TEAM_SIZE) {
      if (pSector->ubNumberOfCivsAtLevel[0] > pSector->ubNumberOfCivsAtLevel[1]) {
        if (pSector->ubNumberOfCivsAtLevel[0] > pSector->ubNumberOfCivsAtLevel[2]) {
          pSector->ubNumberOfCivsAtLevel[0] = 20;
          pSector->ubNumberOfCivsAtLevel[1] = 0;
          pSector->ubNumberOfCivsAtLevel[2] = 0;
        } else {
          pSector->ubNumberOfCivsAtLevel[0] = 0;
          pSector->ubNumberOfCivsAtLevel[1] = 0;
          pSector->ubNumberOfCivsAtLevel[2] = 20;
        }
      } else if (pSector->ubNumberOfCivsAtLevel[1] > pSector->ubNumberOfCivsAtLevel[2]) {
        if (pSector->ubNumberOfCivsAtLevel[1] > pSector->ubNumberOfCivsAtLevel[0]) {
          pSector->ubNumberOfCivsAtLevel[0] = 0;
          pSector->ubNumberOfCivsAtLevel[1] = 20;
          pSector->ubNumberOfCivsAtLevel[2] = 0;
        } else {
          pSector->ubNumberOfCivsAtLevel[0] = 20;
          pSector->ubNumberOfCivsAtLevel[1] = 0;
          pSector->ubNumberOfCivsAtLevel[2] = 0;
        }
      } else {
        if (pSector->ubNumberOfCivsAtLevel[2] > pSector->ubNumberOfCivsAtLevel[1]) {
          pSector->ubNumberOfCivsAtLevel[0] = 0;
          pSector->ubNumberOfCivsAtLevel[1] = 0;
          pSector->ubNumberOfCivsAtLevel[2] = 20;
        } else {
          pSector->ubNumberOfCivsAtLevel[0] = 0;
          pSector->ubNumberOfCivsAtLevel[1] = 20;
          pSector->ubNumberOfCivsAtLevel[2] = 0;
        }
      }
    }
  }
  // Enemy groups
  FOR_EACH_ENEMY_GROUP(pGroup) {
    if (pGroup->pEnemyGroup->ubNumAdmins + pGroup->pEnemyGroup->ubNumTroops +
            pGroup->pEnemyGroup->ubNumElites >
        MAX_STRATEGIC_TEAM_SIZE) {
      pGroup->ubGroupSize = 20;
      if (pGroup->pEnemyGroup->ubNumAdmins > pGroup->pEnemyGroup->ubNumTroops) {
        if (pGroup->pEnemyGroup->ubNumAdmins > pGroup->pEnemyGroup->ubNumElites) {
          pGroup->pEnemyGroup->ubNumAdmins = 20;
          pGroup->pEnemyGroup->ubNumTroops = 0;
          pGroup->pEnemyGroup->ubNumElites = 0;
        } else {
          pGroup->pEnemyGroup->ubNumAdmins = 0;
          pGroup->pEnemyGroup->ubNumTroops = 0;
          pGroup->pEnemyGroup->ubNumElites = 20;
        }
      } else if (pGroup->pEnemyGroup->ubNumTroops > pGroup->pEnemyGroup->ubNumElites) {
        if (pGroup->pEnemyGroup->ubNumTroops > pGroup->pEnemyGroup->ubNumAdmins) {
          pGroup->pEnemyGroup->ubNumAdmins = 0;
          pGroup->pEnemyGroup->ubNumTroops = 20;
          pGroup->pEnemyGroup->ubNumElites = 0;
        } else {
          pGroup->pEnemyGroup->ubNumAdmins = 20;
          pGroup->pEnemyGroup->ubNumTroops = 0;
          pGroup->pEnemyGroup->ubNumElites = 0;
        }
      } else {
        if (pGroup->pEnemyGroup->ubNumElites > pGroup->pEnemyGroup->ubNumTroops) {
          pGroup->pEnemyGroup->ubNumAdmins = 0;
          pGroup->pEnemyGroup->ubNumTroops = 0;
          pGroup->pEnemyGroup->ubNumElites = 20;
        } else {
          pGroup->pEnemyGroup->ubNumAdmins = 0;
          pGroup->pEnemyGroup->ubNumTroops = 20;
          pGroup->pEnemyGroup->ubNumElites = 0;
        }
      }
    }
  }
}

static void UpdateMercMercContractInfo() {
  uint8_t ubCnt;

  for (ubCnt = BIFF; ubCnt <= BUBBA; ubCnt++) {
    SOLDIERTYPE *const pSoldier = FindSoldierByProfileIDOnPlayerTeam(ubCnt);

    // if the merc is on the team
    if (pSoldier == NULL) continue;

    gMercProfiles[ubCnt].iMercMercContractLength = pSoldier->iTotalContractLength;

    pSoldier->iTotalContractLength = 0;
  }
}

int8_t GetNumberForAutoSave(BOOLEAN fLatestAutoSave) {
  BOOLEAN fFile1Exist, fFile2Exist;
  SGP_FILETIME CreationTime1, LastAccessedTime1, LastWriteTime1;
  SGP_FILETIME CreationTime2, LastAccessedTime2, LastWriteTime2;

  fFile1Exist = FALSE;
  fFile2Exist = FALSE;

  // The name of the file
  char zFileName1[256];
  sprintf(zFileName1, "%s/Auto%02d.%s", g_savegame_dir, 0, g_savegame_ext);
  char zFileName2[256];
  sprintf(zFileName2, "%s/Auto%02d.%s", g_savegame_dir, 1, g_savegame_ext);

  if (FileExists(zFileName1)) {
    AutoSGPFile hFile(FileMan::openForReadingSmart(zFileName1, false));
    GetFileManFileTime(hFile, &CreationTime1, &LastAccessedTime1, &LastWriteTime1);
    fFile1Exist = TRUE;
  }

  if (FileExists(zFileName2)) {
    AutoSGPFile hFile(FileMan::openForReadingSmart(zFileName2, false));
    GetFileManFileTime(hFile, &CreationTime2, &LastAccessedTime2, &LastWriteTime2);
    fFile2Exist = TRUE;
  }

  if (!fFile1Exist && !fFile2Exist)
    return (-1);
  else if (fFile1Exist && !fFile2Exist) {
    if (fLatestAutoSave)
      return (0);
    else
      return (-1);
  } else if (!fFile1Exist && fFile2Exist) {
    if (fLatestAutoSave)
      return (1);
    else
      return (-1);
  } else {
    if (CompareSGPFileTimes(&LastWriteTime1, &LastWriteTime2) > 0)
      return (0);
    else
      return (1);
  }
}

static void HandleOldBobbyRMailOrders() {
  int32_t iCnt;
  int32_t iNewListCnt = 0;

  if (LaptopSaveInfo.usNumberOfBobbyRayOrderUsed != 0) {
    gpNewBobbyrShipments =
        MALLOCN(NewBobbyRayOrderStruct, LaptopSaveInfo.usNumberOfBobbyRayOrderUsed);

    giNumberOfNewBobbyRShipment = LaptopSaveInfo.usNumberOfBobbyRayOrderUsed;

    // loop through and add the old items to the new list
    for (iCnt = 0; iCnt < LaptopSaveInfo.usNumberOfBobbyRayOrderItems; iCnt++) {
      // if this slot is used
      if (LaptopSaveInfo.BobbyRayOrdersOnDeliveryArray[iCnt].fActive) {
        // copy over the purchase info
        memcpy(gpNewBobbyrShipments[iNewListCnt].BobbyRayPurchase,
               LaptopSaveInfo.BobbyRayOrdersOnDeliveryArray[iCnt].BobbyRayPurchase,
               sizeof(BobbyRayPurchaseStruct) * MAX_PURCHASE_AMOUNT);

        gpNewBobbyrShipments[iNewListCnt].fActive = TRUE;
        gpNewBobbyrShipments[iNewListCnt].ubDeliveryLoc = BR_DRASSEN;
        gpNewBobbyrShipments[iNewListCnt].ubDeliveryMethod = 0;
        gpNewBobbyrShipments[iNewListCnt].ubNumberPurchases =
            LaptopSaveInfo.BobbyRayOrdersOnDeliveryArray[iCnt].ubNumberPurchases;
        gpNewBobbyrShipments[iNewListCnt].uiPackageWeight = 1;
        gpNewBobbyrShipments[iNewListCnt].uiOrderedOnDayNum = GetWorldDay();
        gpNewBobbyrShipments[iNewListCnt].fDisplayedInShipmentPage = TRUE;

        iNewListCnt++;
      }
    }

    // Clear out the old list
    LaptopSaveInfo.usNumberOfBobbyRayOrderUsed = 0;
    MemFree(LaptopSaveInfo.BobbyRayOrdersOnDeliveryArray);
    LaptopSaveInfo.BobbyRayOrdersOnDeliveryArray = NULL;
  }
}

static void CalcJA2EncryptionSet(SAVED_GAME_HEADER const &h) {
  uint32_t set;

  set = h.iCurrentBalance;
  set *= h.ubNumOfMercsOnPlayersTeam + 1;
  set += h.bSectorZ * 3;
  set += h.ubLoadScreenID;

  if (h.fAlternateSector) set += 7;

  uint32_t const r = h.uiRandom;
  if (r % 2 == 0) {
    set++;
    if (r % 7 == 0) {
      set++;
      if (r % 23 == 0) set++;
      if (r % 79 == 0) set += 2;
    }
  }

  if (isGermanVersion()) {
    set *= 11;
  }

  set %= 10;
  set += h.uiDay / 10;
  set %= BASE_NUMBER_OF_ROTATION_ARRAYS;

  // now pick a different set of #s depending on what game options we've chosen
  GAME_OPTIONS const &o = h.sInitialGameOptions;
  if (o.fGunNut) set += BASE_NUMBER_OF_ROTATION_ARRAYS * 6;
  if (o.fSciFi) set += BASE_NUMBER_OF_ROTATION_ARRAYS * 3;
  switch (o.ubDifficultyLevel) {
    case DIF_LEVEL_EASY:
      set += 0;
      break;
    case DIF_LEVEL_MEDIUM:
      set += BASE_NUMBER_OF_ROTATION_ARRAYS;
      break;
    case DIF_LEVEL_HARD:
      set += BASE_NUMBER_OF_ROTATION_ARRAYS * 2;
      break;
  }

  Assert(set < BASE_NUMBER_OF_ROTATION_ARRAYS * 12);
  guiJA2EncryptionSet = set;
}
