// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef MEANWHILE_H
#define MEANWHILE_H

#include "SGP/Types.h"

enum {
  END_OF_PLAYERS_FIRST_BATTLE,
  DRASSEN_LIBERATED,
  CAMBRIA_LIBERATED,
  ALMA_LIBERATED,
  GRUMM_LIBERATED,
  CHITZENA_LIBERATED,
  NW_SAM,
  NE_SAM,
  CENTRAL_SAM,
  FLOWERS,
  LOST_TOWN,
  INTERROGATION,
  CREATURES,
  KILL_CHOPPER,
  AWOL_SCIENTIST,
  OUTSKIRTS_MEDUNA,
  BALIME_LIBERATED,
  NUM_MEANWHILES
};

struct MEANWHILE_DEFINITION {
  int16_t sSectorX;
  int16_t sSectorY;
  uint16_t usTriggerEvent;

  uint8_t ubMeanwhileID;
  uint8_t ubNPCNumber;
};

void ScheduleMeanwhileEvent(int16_t x, int16_t y, uint16_t trigger_event, uint8_t meanwhile_id,
                            uint8_t npc_profile, uint32_t time);

void BeginMeanwhile(uint8_t ubMeanwhileID);

void CheckForMeanwhileOKStart();
void EndMeanwhile();

bool AreInMeanwhile();
uint8_t GetMeanwhileID();
BOOLEAN AreReloadingFromMeanwhile();

void LocateToMeanwhileCharacter();

// post meanwhile event for town liberation
void HandleMeanWhileEventPostingForTownLiberation(uint8_t bTownId);

// post meanwhile event for SAM liberation
void HandleMeanWhileEventPostingForSAMLiberation(int8_t bSAMId);

// trash world has been called, should we do the first meanwhile?
void HandleFirstMeanWhileSetUpWithTrashWorld();

// battle ended, check if we should set up a meanwhile?
void HandleFirstBattleEndingWhileInTown(int16_t sSectorX, int16_t sSectorY, int16_t bSectorZ,
                                        BOOLEAN fFromAutoResolve);

// lost an entire town to the enemy!
void HandleMeanWhileEventPostingForTownLoss();

// handle short cutting past a meanwhilescene while it is being handled
void HandleShortCuttingOfMeanwhileSceneByPlayer(uint8_t ubMeanwhileID, int32_t iLastProfileId,
                                                int32_t iLastProfileAction);

// handle release of creatures meanwhile
void HandleCreatureRelease();

// handle sending flowers to the queen
void HandleFlowersMeanwhileScene(int8_t bTimeCode);

// player reaches the outskirts of Meduna
void HandleOutskirtsOfMedunaMeanwhileScene();

// let player know about Madlab after certain status % reached
void HandleScientistAWOLMeanwhileScene();

// handle chopper used meanwhile
void HandleKillChopperMeanwhileScene();

extern MEANWHILE_DEFINITION gCurrentMeanwhileDef;
extern MEANWHILE_DEFINITION gMeanwhileDef[NUM_MEANWHILES];
extern BOOLEAN gfMeanwhileTryingToStart;
extern BOOLEAN gfInMeanwhile;
extern uint32_t uiMeanWhileFlags;

#endif
