#ifndef _SQUADS_H
#define _SQUADS_H

#include "JA2Types.h"

// header for squad management system
#define NUMBER_OF_SOLDIERS_PER_SQUAD 6

// enums for squads
enum {
  FIRST_SQUAD = 0,
  SECOND_SQUAD,
  THIRD_SQUAD,
  FOURTH_SQUAD,
  FIFTH_SQUAD,
  SIXTH_SQUAD,
  SEVENTH_SQUAD,
  EIGTH_SQUAD,
  NINTH_SQUAD,
  TENTH_SQUAD,
  ELEVENTH_SQUAD,
  TWELTH_SQUAD,
  THIRTEENTH_SQUAD,
  FOURTEENTH_SQUAD,
  FIFTHTEEN_SQUAD,
  SIXTEENTH_SQUAD,
  SEVENTEENTH_SQUAD,
  EIGTHTEENTH_SQUAD,
  NINTEENTH_SQUAD,
  TWENTYTH_SQUAD,
  NUMBER_OF_SQUADS,
};

// ATE: Added so we can have no current squad
// happens in we move off sector via tactical, but nobody is left!
#define NO_CURRENT_SQUAD NUMBER_OF_SQUADS

// ptrs to soldier types of squads and their members

// squads
extern SOLDIERTYPE *Squad[NUMBER_OF_SQUADS][NUMBER_OF_SOLDIERS_PER_SQUAD];

#define FOR_EACH_SLOT_IN_SQUAD(iter, squad)                                                    \
  for (SOLDIERTYPE **iter = Squad[(squad)], *const *const iter##__end = endof(Squad[(squad)]); \
       iter != iter##__end; ++iter)

#define FOR_EACH_IN_SQUAD(iter, squad) \
  FOR_EACH_SLOT_IN_SQUAD(iter, squad)  \
  if (!*iter)                          \
    continue;                          \
  else

extern int32_t iCurrentTacticalSquad;

// will initialize the squad lists for game initalization
void InitSquads();

// add character to squad
BOOLEAN AddCharacterToSquad(SOLDIERTYPE *pCharacter, int8_t bSquadValue);

// find the first slot the guy will fit in, return true if he is in a squad or
// has been put in one
void AddCharacterToAnySquad(SOLDIERTYPE *);

// remove character from squads
BOOLEAN RemoveCharacterFromSquads(SOLDIERTYPE *pCharacter);

// return number of people in this squad
int8_t NumberOfPeopleInSquad(int8_t bSquadValue);

int8_t NumberOfNonEPCsInSquad(int8_t bSquadValue);

BOOLEAN IsRobotControllerInSquad(int8_t bSquadValue);

int8_t NumberOfPlayerControllableMercsInSquad(int8_t bSquadValue);

// what sector is the squad currently in?..return if anyone in squad
BOOLEAN SectorSquadIsIn(int8_t bSquadValue, int16_t *sMapX, int16_t *sMapY, int8_t *sMapZ);

// rebuild current squad list
void RebuildCurrentSquad();

// copy path from character back to squad
void CopyPathOfCharacterToSquad(SOLDIERTYPE *pCharacter, int8_t bSquadValue);

// what is the id of the current squad?
int32_t CurrentSquad();

// add character to unique squad, returns the squad #
int8_t AddCharacterToUniqueSquad(SOLDIERTYPE *pCharacter);

// is this squad empty?
BOOLEAN SquadIsEmpty(int8_t bSquadValue);

// is this squad in the current tactical sector?
BOOLEAN IsSquadOnCurrentTacticalMap(int32_t iCurrentSquad);

// set this squad as the current tatcical squad
BOOLEAN SetCurrentSquad(int32_t iCurrentSquad, BOOLEAN fForce);

// set default squad in sector
void SetDefaultSquadOnSectorEntry(BOOLEAN fForce);

// get last squad that has active mercs
int32_t GetLastSquadActive();

void ExamineCurrentSquadLights();

// Save the squad information to the saved game file
void SaveSquadInfoToSavedGameFile(HWFILE);

// Load all the squad info from the saved game file
void LoadSquadInfoFromSavedGameFile(HWFILE);

// get squad id of first free squad
int8_t GetFirstEmptySquad();

// dead soldier was on squad
BOOLEAN SoldierIsDeadAndWasOnSquad(SOLDIERTYPE *pSoldier, int8_t bSquadValue);

// now reset the table for these mercs
void ResetDeadSquadMemberList(int32_t iSquadValue);

// this passed  soldier on the current squad int he tactical map
BOOLEAN IsMercOnCurrentSquad(const SOLDIERTYPE *pSoldier);

// is this squad filled up?
BOOLEAN IsThisSquadFull(int8_t bSquadValue);

// is this squad moving?
BOOLEAN IsThisSquadOnTheMove(int8_t bSquadValue);

// is there a vehicle in this squad?
BOOLEAN DoesVehicleExistInSquad(int8_t bSquadValue);

// re-create any trashed squad movement groups
void CheckSquadMovementGroups();

#endif
