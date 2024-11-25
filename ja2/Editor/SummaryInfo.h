// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __SUMMARY_INFO_H
#define __SUMMARY_INFO_H

#include "SGP/Types.h"
#include "Tactical/MapInformation.h"
#include "TileEngine/ExitGrids.h"

#define GLOBAL_SUMMARY_VERSION 14
#define MINIMUMVERSION 7

struct TEAMSUMMARY {
  uint8_t ubTotal;
  uint8_t ubDetailed;
  uint8_t ubProfile;
  uint8_t ubExistance;
  uint8_t ubNumAnimals;
  uint8_t ubBadA, ubPoorA, ubAvgA, ubGoodA, ubGreatA;  // attributes
  uint8_t ubBadE, ubPoorE, ubAvgE, ubGoodE, ubGreatE;  // equipment
};  // 15 bytes

struct SUMMARYFILE {
  // start version 1
  uint8_t ubSummaryVersion;
  uint8_t ubSpecial;
  uint16_t usNumItems;
  uint16_t usNumLights;
  MAPCREATE_STRUCT MapInfo;
  TEAMSUMMARY EnemyTeam;
  TEAMSUMMARY CreatureTeam;
  TEAMSUMMARY RebelTeam;
  TEAMSUMMARY CivTeam;
  uint8_t ubNumDoors;
  uint8_t ubNumDoorsLocked;
  uint8_t ubNumDoorsTrapped;
  uint8_t ubNumDoorsLockedAndTrapped;
  // start version 2
  uint8_t ubTilesetID;
  uint8_t ubNumRooms;
  // start version	3
  uint8_t ubNumElites;
  uint8_t ubNumAdmins;
  uint8_t ubNumTroops;
  // start version 4
  uint8_t ubEliteDetailed;
  uint8_t ubAdminDetailed;
  uint8_t ubTroopDetailed;
  // start version 5
  uint8_t ubEliteProfile;
  uint8_t ubAdminProfile;
  uint8_t ubTroopProfile;
  // start version 6
  uint8_t ubEliteExistance;
  uint8_t ubAdminExistance;
  uint8_t ubTroopExistance;
  // start version 7
  float dMajorMapVersion;
  // start version 8
  uint8_t ubCivSchedules;
  // start version 9
  uint8_t ubCivCows;
  uint8_t ubCivBloodcats;
  //																//-----
  //	190
  // start version 10
  EXITGRID ExitGrid[4];           // 5*4 //	 20
  uint16_t usExitGridSize[4];     // 2*4 //    8
  BOOLEAN fInvalidDest[4];        //    4
  uint8_t ubNumExitGridDests;     //		1
  BOOLEAN fTooManyExitGridDests;  //		1
  //																//-----
  //																//
  // 224 start version 11
  uint8_t ubEnemiesReqWaypoints;  //		1
  //																//-----
  //																		225
  // start version 12
  uint16_t usWarningRoomNums;  //    2
                               //	227
  // start version 13
  uint8_t ubEnemiesHaveWaypoints;  //		1
  uint32_t uiNumItemsPosition;     //		4
                                   //-----
                                   //	232
  // start version 14
  uint32_t uiEnemyPlacementPosition;  //		4
                                      //-----
                                      //	236

  uint8_t ubPadding
      [164];  //	164 // XXX HACK000B
              //																//-----
              //																		400
              // total bytes
};

extern BOOLEAN gfAutoLoadA9;

extern BOOLEAN EvaluateWorld(const char *pSector, uint8_t ubLevel);
void WriteSectorSummaryUpdate(const char *filename, uint8_t ubLevel, SUMMARYFILE *);

extern BOOLEAN gfMustForceUpdateAllMaps;
extern BOOLEAN gfMajorUpdate;
void ApologizeOverrideAndForceUpdateEverything();

#endif
