// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef _TOWN_MILITIA_H
#define _TOWN_MILITIA_H

// header for town militia strategic control module

#include <stdlib.h>

#include "JA2Types.h"

// how many militia of all ranks can be in any one sector at once
#define MAX_ALLOWABLE_MILITIA_PER_SECTOR 20

// how many new green militia civilians are trained at a time
#define MILITIA_TRAINING_SQUAD_SIZE 10  // was 6

// cost of starting a new militia training assignment
#define MILITIA_TRAINING_COST 750

// minimum loyalty rating before training is allowed in a town
#define MIN_RATING_TO_TRAIN_TOWN 20

// this handles what happens when a new militia unit is finishes getting trained
void TownMilitiaTrainingCompleted(SOLDIERTYPE *pTrainer, int16_t sMapX, int16_t sMapY);

// Given a SOLDIER_CLASS_ returns a _MITILIA rank or -1 if it is not militia
int8_t SoldierClassToMilitiaRank(uint8_t soldier_class);

// remove militias of a certain rank
void StrategicRemoveMilitiaFromSector(int16_t sMapX, int16_t sMapY, uint8_t ubRank,
                                      uint8_t ubHowMany);

// Check for promotions and handle them
uint8_t CheckOneMilitiaForPromotion(int16_t x, int16_t y, uint8_t current_rank,
                                    uint8_t kill_points);

void BuildMilitiaPromotionsString(wchar_t *str, size_t Length);

uint8_t CountAllMilitiaInSector(int16_t sMapX, int16_t sMapY);
uint8_t MilitiaInSectorOfRank(int16_t sMapX, int16_t sMapY, uint8_t ubRank);

// Returns TRUE if sector is under player control, has no enemies in it, and
// isn't currently in combat mode
BOOLEAN SectorOursAndPeaceful(int16_t sMapX, int16_t sMapY, int8_t bMapZ);

// tell player how much it will cost
void HandleInterfaceMessageForCostOfTrainingMilitia(SOLDIERTYPE *pSoldier);

// call this when the sector changes...
void HandleMilitiaStatusInCurrentMapBeforeLoadingNewMap();

// Is there a town with militia here or nearby?
bool CanNearbyMilitiaScoutThisSector(int16_t x, int16_t y);

// Is the town or SAM site here full of milita?
BOOLEAN IsAreaFullOfMilitia(const int16_t sector_x, const int16_t sector_y, const int8_t sector_z);

// now that town training is complete, handle the continue boxes
void HandleContinueOfTownTraining();

// clear the list of training completed sectors
void ClearSectorListForCompletedTrainingOfMilitia();

BOOLEAN MilitiaTrainingAllowedInSector(int16_t sSectorX, int16_t sSectorY, int8_t bSectorZ);
BOOLEAN MilitiaTrainingAllowedInTown(int8_t bTownId);

void AddSectorForSoldierToListOfSectorsThatCompletedMilitiaTraining(SOLDIERTYPE *pSoldier);

extern SOLDIERTYPE *pMilitiaTrainerSoldier;

#endif
