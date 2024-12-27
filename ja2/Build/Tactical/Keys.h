// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef _KEYS_H_
#define _KEYS_H_

#include "SGP/Types.h"
#include "Tactical/HandleItems.h"
#include "Tactical/ItemTypes.h"

struct KEY {
  uint16_t usSectorFound;  // where and
  uint16_t usDateFound;    // when the key was found
};

#define KEY_USED 0x01

#define LOCK_UNOPENABLE 255
#define NO_KEY 255

#define LOCK_REGULAR 1
#define LOCK_PADLOCK 2
#define LOCK_CARD 3
#define LOCK_ELECTRONIC 4
#define LOCK_SPECIAL 5

#define MAXLOCKDESCLENGTH 40
struct LOCK {
  uint8_t ubEditorName[MAXLOCKDESCLENGTH];  // name to display in editor
  uint16_t usKeyItem;                       // key for this door uses which graphic (item #)?
  uint8_t ubLockType;                       // regular, padlock, electronic, etc
  uint8_t ubPickDifficulty;                 // difficulty to pick such a lock
  uint8_t ubSmashDifficulty;                // difficulty to smash such a lock
  uint8_t ubFiller;                         // XXX HACK000B
};

// Defines below for the perceived value of the door
#define DOOR_PERCEIVED_UNKNOWN 0
#define DOOR_PERCEIVED_LOCKED 1
#define DOOR_PERCEIVED_UNLOCKED 2
#define DOOR_PERCEIVED_BROKEN 3

#define DOOR_PERCEIVED_TRAPPED 1
#define DOOR_PERCEIVED_UNTRAPPED 2

struct DOOR {
  int16_t sGridNo;
  BOOLEAN fLocked;           // is the door locked
  uint8_t ubTrapLevel;       // difficulty of finding the trap, 0-10
  uint8_t ubTrapID;          // the trap type (0 is no trap)
  uint8_t ubLockID;          // the lock (0 is no lock)
  int8_t bPerceivedLocked;   // The perceived lock value can be different than the
                             // fLocked. Values for this include the fact that we
                             // don't know the status of the door, etc
  int8_t bPerceivedTrapped;  // See above, but with respect to traps rather than
                             // locked status
  int8_t bLockDamage;        // Damage to the lock
  int8_t bPadding[4];        // extra bytes // XXX HACK000B
};

enum DoorTrapTypes {
  NO_TRAP = 0,
  EXPLOSION,
  ELECTRIC,
  SIREN,
  SILENT_ALARM,
  BROTHEL_SIREN,
  SUPER_ELECTRIC,
  NUM_DOOR_TRAPS
};

#define DOOR_TRAP_STOPS_ACTION 0x01
#define DOOR_TRAP_RECURRING 0x02
#define DOOR_TRAP_SILENT 0x04

struct DOORTRAP {
  uint8_t fFlags;  // stops action?  recurring trap?
};

// The status of the door, either open or closed
#define DOOR_OPEN 0x01
#define DOOR_PERCEIVED_OPEN 0x02
#define DOOR_PERCEIVED_NOTSET 0x04
#define DOOR_BUSY 0x08
#define DOOR_HAS_TIN_CAN 0x10

#define DONTSETDOORSTATUS 2

struct DOOR_STATUS {
  int16_t sGridNo;
  uint8_t ubFlags;
};

// This is the number of different types of doors we can have
// in one map at a time...

#define NUM_KEYS 64
#define NUM_LOCKS 64
#define INVALID_KEY_NUMBER 255

#define ANYKEY 252
#define AUTOUNLOCK 253
#define OPENING_NOT_POSSIBLE 254

extern KEY KeyTable[NUM_KEYS];
extern LOCK LockTable[NUM_LOCKS];
extern DOORTRAP const DoorTrapTable[NUM_DOOR_TRAPS];

extern BOOLEAN AddKeysToKeyRing(SOLDIERTYPE *pSoldier, uint8_t ubKeyID, uint8_t ubNumber);
extern BOOLEAN RemoveKeyFromKeyRing(SOLDIERTYPE *pSoldier, uint8_t ubPos, OBJECTTYPE *pObj);
extern BOOLEAN RemoveAllOfKeyFromKeyRing(SOLDIERTYPE *pSoldier, uint8_t ubPos, OBJECTTYPE *pObj);
bool KeyExistsInKeyRing(SOLDIERTYPE const &, uint8_t key_id);
bool SoldierHasKey(SOLDIERTYPE const &, uint8_t key_id);

/**********************************
 * Door utils add by Kris Morness *
 **********************************/

// Dynamic array of Doors.  For general game purposes, the doors that are locked
// and/or trapped are permanently saved within the map, and are loaded and
// allocated when the map is loaded.  Because the editor allows more doors to be
// added, or removed, the actual size of the DoorTable may change.
extern DOOR *DoorTable;

// Current number of doors in world.
extern uint8_t gubNumDoors;
// File I/O for loading the door information from the map.  This automatically
// allocates the exact number of slots when loading.

#define FOR_EACH_DOOR(iter) \
  for (DOOR *iter = DoorTable, *const iter##__end = iter + gubNumDoors; iter != iter##__end; ++iter)

void LoadDoorTableFromMap(HWFILE);

// Saves the existing door information to the map.  Before it actually saves,
// it'll verify that the door still exists.  Otherwise, it'll ignore it.  It is
// possible in the editor to delete doors in many different ways, so I opted to
// put it in the saving routine.
extern void SaveDoorTableToMap(HWFILE fp);

// The editor adds locks to the world.  If the gridno already exists, then the
// currently existing door information is overwritten.
extern void AddDoorInfoToTable(DOOR *pDoor);
// When the editor removes a door from the world, this function looks for and
// removes accompanying door information.  If the entry is not the last entry,
// the last entry is move to it's current slot, to keep everything contiguous.
extern void RemoveDoorInfoFromTable(int32_t iMapIndex);
// This is the link to see if a door exists at a gridno.
DOOR *FindDoorInfoAtGridNo(int32_t iMapIndex);
// Upon world deallocation, the door table needs to be deallocated.
void TrashDoorTable();

wchar_t const *GetTrapName(DOOR const &);

BOOLEAN AttemptToUnlockDoor(const SOLDIERTYPE *pSoldier, DOOR *pDoor);
BOOLEAN AttemptToLockDoor(const SOLDIERTYPE *pSoldier, DOOR *pDoor);
BOOLEAN AttemptToSmashDoor(SOLDIERTYPE *pSoldier, DOOR *pDoor);
BOOLEAN AttemptToPickLock(SOLDIERTYPE *pSoldier, DOOR *pDoor);
BOOLEAN AttemptToBlowUpLock(SOLDIERTYPE *pSoldier, DOOR *pDoor);
BOOLEAN AttemptToUntrapDoor(SOLDIERTYPE *pSoldier, DOOR *pDoor);
BOOLEAN ExamineDoorForTraps(SOLDIERTYPE *pSoldier, DOOR *pDoor);
BOOLEAN HasDoorTrapGoneOff(SOLDIERTYPE *pSoldier, DOOR *pDoor);
void HandleDoorTrap(SOLDIERTYPE &, DOOR const &);

// Updates the perceived value to the user of the state of the door
void UpdateDoorPerceivedValue(DOOR *pDoor);

// Saves the Door Table array to the temp file
void SaveDoorTableToDoorTableTempFile(int16_t sSectorX, int16_t sSectorY, int8_t bSectorZ);

// Load the door table from the temp file
void LoadDoorTableFromDoorTableTempFile();

/* Add a door to the door status array. As the user comes across the door, they
 * are added. If the door already exists, nothing happens.
 * is_open is True if the door is to be initially open, false if it is closed
 * perceived_open is true if the door is to be initially open, else false */
bool ModifyDoorStatus(GridNo, BOOLEAN is_open, BOOLEAN perceived_open);

// Deletes the door status array
void TrashDoorStatusArray();

// Saves the Door Status array to the MapTempfile
void SaveDoorStatusArrayToDoorStatusTempFile(int16_t sSectorX, int16_t sSectorY, int8_t bSectorZ);

// Load the door status from the door status temp file
void LoadDoorStatusArrayFromDoorStatusTempFile();

// Save the key table to the saved game file
void SaveKeyTableToSaveGameFile(HWFILE);

// Load the key table from the saved game file
void LoadKeyTableFromSaveedGameFile(HWFILE);

// Returns a doors status value, NULL if not found
DOOR_STATUS *GetDoorStatus(int16_t sGridNo);

bool AllMercsLookForDoor(GridNo);

void MercLooksForDoors(SOLDIERTYPE const &);

void UpdateDoorGraphicsFromStatus();

BOOLEAN AttemptToCrowbarLock(SOLDIERTYPE *pSoldier, DOOR *pDoor);

void LoadLockTable();

void ExamineDoorsOnEnteringSector();

void AttachStringToDoor(int16_t sGridNo);

void DropKeysInKeyRing(SOLDIERTYPE &, GridNo, int8_t level, Visibility, bool add_to_drop_list,
                       int32_t drop_list_slot, bool use_unloaded);

#endif
