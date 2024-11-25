// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef _SAVE_LOAD_GAME_H_
#define _SAVE_LOAD_GAME_H_

#include "GameSettings.h"
#include "ScreenIDs.h"

#define BYTESINMEGABYTE 1048576  // 1024*1024
#define REQUIRED_FREE_SPACE (20 * BYTESINMEGABYTE)

#define SIZE_OF_SAVE_GAME_DESC 128

#define GAME_VERSION_LENGTH 16

#define SAVE__ERROR_NUM 99
#define SAVE__END_TURN_NUM 98

#define SAVED_GAME_HEADER_ON_DISK_SIZE                                     \
  (432) /** Size of SAVED_GAME_HEADER on disk in Vanilla and Stracciatella \
           Windows  */
#define SAVED_GAME_HEADER_ON_DISK_SIZE_STRAC_LIN \
  (688) /** Size of SAVED_GAME_HEADER on disk in Stracciatella Linux */

struct SAVED_GAME_HEADER {
  uint32_t uiSavedGameVersion;
  char zGameVersionNumber[GAME_VERSION_LENGTH];

  wchar_t sSavedGameDesc[SIZE_OF_SAVE_GAME_DESC];

  /* (vanilla) uint32_t	uiFlags; */

  // The following will be used to quickly access info to display in the
  // save/load screen
  uint32_t uiDay;
  uint8_t ubHour;
  uint8_t ubMin;
  int16_t sSectorX;
  int16_t sSectorY;
  int8_t bSectorZ;
  uint8_t ubNumOfMercsOnPlayersTeam;
  int32_t iCurrentBalance;

  uint32_t uiCurrentScreen;

  BOOLEAN fAlternateSector;

  BOOLEAN fWorldLoaded;

  uint8_t ubLoadScreenID;  // The load screen that should be used when loading the
                           // saved game

  GAME_OPTIONS sInitialGameOptions;  // need these in the header so we can get
                                     // the info from it on the save load screen.

  uint32_t uiRandom;

  /* (vanilla) uint8_t		ubFiller[110]; */
};

/** Parse binary data and fill SAVED_GAME_HEADER structure.
 * @param data Data to be parsed.
 * @param h Header structure to be filled.
 * @param stracLinuxFormat Flag, telling to use "Stracciatella Linux" format. */
extern void ParseSavedGameHeader(const uint8_t *data, SAVED_GAME_HEADER &h, bool stracLinuxFormat);

/** @brief Check if SAVED_GAME_HEADER structure contains valid data.
 * This function does the basic check. */
extern bool isValidSavedGameHeader(SAVED_GAME_HEADER &h);

/** @brief Extract saved game header from a file.
 * Return \a stracLinuxFormat = true, when the file is in "Stracciatella Linux"
 * format. */
void ExtractSavedGameHeaderFromFile(HWFILE, SAVED_GAME_HEADER &, bool *stracLinuxFormat);

extern ScreenID guiScreenToGotoAfterLoadingSavedGame;

void CreateSavedGameFileNameFromNumber(uint8_t ubSaveGameID, char *pzNewFileName);

BOOLEAN SaveGame(uint8_t ubSaveGameID, const wchar_t *pGameDesc);
void LoadSavedGame(uint8_t save_slot_id);

void SaveFilesToSavedGame(char const *pSrcFileName, HWFILE);
void LoadFilesFromSavedGame(char const *pSrcFileName, HWFILE);

BOOLEAN DoesUserHaveEnoughHardDriveSpace();

void GetBestPossibleSectorXYZValues(int16_t *psSectorX, int16_t *psSectorY, int8_t *pbSectorZ);

void SaveMercPath(HWFILE, PathSt const *head);
void LoadMercPath(HWFILE, PathSt **head);

extern uint32_t guiLastSaveGameNum;
int8_t GetNumberForAutoSave(BOOLEAN fLatestAutoSave);

extern uint32_t guiJA2EncryptionSet;

extern BOOLEAN gfUseConsecutiveQuickSaveSlots;

#endif
