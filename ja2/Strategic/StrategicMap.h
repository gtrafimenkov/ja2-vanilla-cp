// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __STRATMAP_H
#define __STRATMAP_H
// this file is for manipulation of the strategic map structure

#include <stdlib.h>

#include "JA2Types.h"

// The maximum size for any team strategically speaking.  For example, we can't
// have more than 20 enemies, militia, or creatures at a time.
#define MAX_STRATEGIC_TEAM_SIZE 20

// Codes for jumoing into adjacent sectors..
enum { JUMP_ALL_LOAD_NEW, JUMP_ALL_NO_LOAD, JUMP_SINGLE_LOAD_NEW, JUMP_SINGLE_NO_LOAD };

enum {
  CONTROLLED = 0,
  UNCONTROLLED,
};

// For speed, etc lets make these globals, forget the functions if you want
extern int16_t gWorldSectorX;
extern int16_t gWorldSectorY;
extern int8_t gbWorldSectorZ;

#define NO_SECTOR ((uint32_t) - 1)

uint32_t GetWorldSector();

static inline void SetWorldSectorInvalid() {
  gWorldSectorX = 0;
  gWorldSectorY = 0;
  gbWorldSectorZ = -1;
}

#define NUMBER_OF_SAMS 4

extern int16_t const pSamList[NUMBER_OF_SAMS];
extern int16_t pSamGridNoAList[NUMBER_OF_SAMS];
extern int16_t pSamGridNoBList[NUMBER_OF_SAMS];

extern BOOLEAN fFoundOrta;
extern BOOLEAN fSamSiteFound[NUMBER_OF_SAMS];

extern BOOLEAN gfUseAlternateMap;

// min condition for sam site to be functional
#define MIN_CONDITION_FOR_SAM_SITE_TO_WORK 80

// FUNCTIONS FOR DERTERMINING GOOD SECTOR EXIT DATA
#define CHECK_DIR_X_DELTA (WORLD_TILE_X * 4)
#define CHECK_DIR_Y_DELTA (WORLD_TILE_Y * 10)

#define MAP_WORLD_X 18
#define MAP_WORLD_Y 18

// get index into aray
#define CALCULATE_STRATEGIC_INDEX(x, y) (x + (y * MAP_WORLD_X))
#define GET_X_FROM_STRATEGIC_INDEX(i) (i % MAP_WORLD_X)
#define GET_Y_FROM_STRATEGIC_INDEX(i) (i / MAP_WORLD_X)

// macros to convert between the 2 different sector numbering systems
#define SECTOR_INFO_TO_STRATEGIC_INDEX(i) (CALCULATE_STRATEGIC_INDEX(SECTORX(i), SECTORY(i)))
#define STRATEGIC_INDEX_TO_SECTOR_INFO(i) \
  (SECTOR(GET_X_FROM_STRATEGIC_INDEX(i), GET_Y_FROM_STRATEGIC_INDEX(i)))

// grab the town id value
uint8_t GetTownIdForSector(uint8_t sector);

void SetCurrentWorldSector(int16_t x, int16_t y, int8_t z);

void UpdateMercsInSector();
void UpdateMercInSector(SOLDIERTYPE &, int16_t sSectorX, int16_t sSectorY, int8_t bSectorZ);

// get short sector name without town name
void GetShortSectorString(int16_t sMapX, int16_t sMapY, wchar_t *sString, size_t Length);

// Return a string like 'A9: Omerta'
void GetSectorIDString(int16_t x, int16_t y, int8_t z, wchar_t *buf, size_t length,
                       BOOLEAN detailed);

void GetMapFileName(int16_t x, int16_t y, int8_t z, char *buf, BOOLEAN add_alternate_map_letter);

// Called from within tactical.....
void JumpIntoAdjacentSector(uint8_t ubDirection, uint8_t ubJumpCode, int16_t sAdditionalData);

bool CanGoToTacticalInSector(int16_t x, int16_t y, uint8_t z);

void UpdateAirspaceControl();

bool IsThisSectorASAMSector(int16_t x, int16_t y, int8_t z);

void InitializeSAMSites();

// Number of sectors this town takes up
uint8_t GetTownSectorSize(int8_t town_id);

// Number of sectors under player control for this town
uint8_t GetTownSectorsUnderControl(int8_t town_id);

BOOLEAN OKForSectorExit(int8_t bExitDirection, uint16_t usAdditionalData,
                        uint32_t *puiTraverseTimeInMinutes = 0);
void SetupNewStrategicGame();

void LoadStrategicInfoFromSavedFile(HWFILE);
void SaveStrategicInfoToSavedFile(HWFILE);

void AllMercsHaveWalkedOffSector();

void AdjustSoldierPathToGoOffEdge(SOLDIERTYPE *pSoldier, int16_t sEndGridNo,
                                  uint8_t ubTacticalDirection);

void AllMercsWalkedToExitGrid();

void PrepareLoadedSector();

// handle for slay...no better place to really put this stuff
void HandleSlayDailyEvent();

void HandleQuestCodeOnSectorEntry(int16_t sNewSectorX, int16_t sNewSectorY, int8_t bNewSectorZ);

// handle a soldier leaving thier squad behind, this sets them up for mvt and
// potential rejoining of group
void HandleSoldierLeavingSectorByThemSelf(SOLDIERTYPE *pSoldier);

BOOLEAN CheckAndHandleUnloadingOfCurrentWorld();

// number of SAM sites under player control
int32_t GetNumberOfSAMSitesUnderPlayerControl();

// is there a FUNCTIONAL SAM site in this sector?
bool IsThereAFunctionalSAMSiteInSector(int16_t x, int16_t y, int8_t z);

bool IsSectorDesert(int16_t x, int16_t y);

int8_t GetSAMIdFromSector(int16_t sSectorX, int16_t sSectorY, int8_t bSectorZ);

void SetupProfileInsertionDataForSoldier(const SOLDIERTYPE *s);

BOOLEAN HandlePotentialBringUpAutoresolveToFinishBattle();

void BeginLoadScreen();

void RemoveMercsInSector();

void InitStrategicEngine();

// Used for determining the type of error message that comes up when you can't
// traverse to an adjacent sector.  THESE VALUES DO NOT NEED TO BE SAVED!
extern BOOLEAN gfInvalidTraversal;
extern BOOLEAN gfLoneEPCAttemptingTraversal;
extern BOOLEAN gfRobotWithoutControllerAttemptingTraversal;
extern uint8_t gubLoneMercAttemptingToAbandonEPCs;
extern const SOLDIERTYPE *gPotentiallyAbandonedEPC;

extern int8_t gbGreenToElitePromotions;
extern int8_t gbGreenToRegPromotions;
extern int8_t gbRegToElitePromotions;
extern int8_t gbMilitiaPromotions;

extern BOOLEAN gfGettingNameFromSaveLoadScreen;

struct StrategicMapElement {
  int8_t bNameId;
  BOOLEAN fEnemyControlled;  // enemy controlled or not
  BOOLEAN fEnemyAirControlled;
  int8_t bSAMCondition;  // SAM Condition .. 0 - 100, just like an item's status
};

extern StrategicMapElement StrategicMap[MAP_WORLD_X * MAP_WORLD_Y];

#endif
