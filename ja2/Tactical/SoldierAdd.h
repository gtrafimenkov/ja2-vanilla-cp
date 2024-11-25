// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef _SOLDIER_ADD_H
#define _SOLDIER_ADD_H

#include "JA2Types.h"

// Finds a gridno given a sweet spot
uint16_t FindGridNoFromSweetSpot(const SOLDIERTYPE *pSoldier, int16_t sSweetGridNo,
                                 int8_t ubRadius);

// Ensures a good path.....
uint16_t FindGridNoFromSweetSpotThroughPeople(const SOLDIERTYPE *pSoldier, int16_t sSweetGridNo,
                                              int8_t ubRadius);

// Returns a good sweetspot but not the swetspot!
uint16_t FindGridNoFromSweetSpotExcludingSweetSpot(const SOLDIERTYPE *pSoldier,
                                                   int16_t sSweetGridNo, int8_t ubRadius);

uint16_t FindGridNoFromSweetSpotExcludingSweetSpotInQuardent(const SOLDIERTYPE *pSoldier,
                                                             int16_t sSweetGridNo, int8_t ubRadius,
                                                             int8_t ubQuardentDir);

// Finds a gridno near a sweetspot but a random one!
uint16_t FindRandomGridNoFromSweetSpot(const SOLDIERTYPE *pSoldier, int16_t sSweetGridNo,
                                       int8_t ubRadius);

// Adds a soldier ( already created in mercptrs[] array )!
// Finds a good placement based on data in the loaded sector and if they are
// enemy's or not, etc...
void AddSoldierToSector(SOLDIERTYPE *);

void AddSoldierToSectorNoCalculateDirection(SOLDIERTYPE *);

void AddSoldierToSectorNoCalculateDirectionUseAnimation(SOLDIERTYPE *, uint16_t usAnimState,
                                                        uint16_t usAnimCode);

// IsMercOnTeam() checks to see if the passed in Merc Profile ID is currently on
// the player's team
BOOLEAN IsMercOnTeam(uint8_t ubMercID);
// ATE: Added for contract renewals
BOOLEAN IsMercOnTeamAndInOmertaAlreadyAndAlive(uint8_t ubMercID);

uint16_t FindGridNoFromSweetSpotWithStructData(SOLDIERTYPE *pSoldier, uint16_t usAnimState,
                                               int16_t sSweetGridNo, int8_t ubRadius,
                                               uint8_t *pubDirection, BOOLEAN fClosestToMerc);

uint16_t FindGridNoFromSweetSpotWithStructDataFromSoldier(const SOLDIERTYPE *pSoldier,
                                                          uint16_t usAnimState, int8_t ubRadius,
                                                          BOOLEAN fClosestToMerc,
                                                          const SOLDIERTYPE *pSrcSoldier);

void SoldierInSectorPatient(SOLDIERTYPE *pSoldier, int16_t sGridNo);
void SoldierInSectorDoctor(SOLDIERTYPE *pSoldier, int16_t sGridNo);
void SoldierInSectorRepair(SOLDIERTYPE *pSoldier, int16_t sGridNo);

BOOLEAN CanSoldierReachGridNoInGivenTileLimit(SOLDIERTYPE *pSoldier, int16_t sGridNo,
                                              int16_t sMaxTiles, int8_t bLevel);

#endif
