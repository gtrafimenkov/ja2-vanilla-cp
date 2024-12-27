// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#pragma once

/* This file contains copied of data structures of the original game. */

#include "SGP/Types.h"

/** Vanilla Data Structures */
namespace VDS {

typedef uint16_t CHAR16;

/* -------------------------------------------------------------------------
 * ja2/Build/GameSettings.h
 * ------------------------------------------------------------------------- */

typedef struct {
  BOOLEAN fGunNut;
  BOOLEAN fSciFi;
  uint8_t ubDifficultyLevel;
  BOOLEAN fTurnTimeLimit;
  BOOLEAN fIronManMode;

  uint8_t ubFiller[7];

} GAME_OPTIONS;

/* -------------------------------------------------------------------------
 * ja2/Build/SaveLoadGame.h
 * ------------------------------------------------------------------------- */

#define SIZE_OF_SAVE_GAME_DESC 128

#define GAME_VERSION_LENGTH 16

typedef struct {
  uint32_t uiSavedGameVersion;
  int8_t zGameVersionNumber[GAME_VERSION_LENGTH];

  CHAR16 sSavedGameDesc[SIZE_OF_SAVE_GAME_DESC];

  uint32_t uiFlags;

#ifdef CRIPPLED_VERSION
  uint8_t ubCrippleFiller[20];
#endif

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

  uint8_t ubFiller[110];

} SAVED_GAME_HEADER;

/* -------------------------------------------------------------------------
 *
 * ------------------------------------------------------------------------- */

}  // namespace VDS
