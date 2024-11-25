// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef _VEHICLES_H
#define _VEHICLES_H

#include "JA2Types.h"
#include "Strategic/StrategicMovement.h"

#define MAX_VEHICLES 10

// type of vehicles
enum {
  ELDORADO_CAR = 0,
  HUMMER,
  ICE_CREAM_TRUCK,
  JEEP_CAR,
  TANK_CAR,
  HELICOPTER,
  NUMBER_OF_TYPES_OF_VEHICLES,
};

// struct for vehicles
struct VEHICLETYPE {
  PathSt *pMercPath;        // vehicle's stategic path list
  uint8_t ubMovementGroup;  // the movement group this vehicle belongs to
  uint8_t ubVehicleType;    // type of vehicle
  int16_t sSectorX;         // X position on the Stategic Map
  int16_t sSectorY;         // Y position on the Stategic Map
  int16_t sSectorZ;
  BOOLEAN fBetweenSectors;  // between sectors?
  int16_t sGridNo;          // location in tactical
  SOLDIERTYPE *pPassengers[10];
  BOOLEAN fDestroyed;
  int32_t iMovementSoundID;
  BOOLEAN fValid;
};

#define CFOR_EACH_PASSENGER(v, iter)                                                           \
  for (SOLDIERTYPE *const *iter = (v).pPassengers, *const *const end__##iter =                 \
                                                       (v).pPassengers + GetVehicleSeats((v)); \
       iter != end__##iter; ++iter)                                                            \
    if (!*iter)                                                                                \
      continue;                                                                                \
    else

// the list of vehicles
extern VEHICLETYPE *pVehicleList;

// number of vehicles on the list
extern uint8_t ubNumberOfVehicles;

#define VEHICLE2ID(v) (uint32_t)((&(v) - pVehicleList))

#define BASE_FOR_EACH_VEHICLE(type, iter)                                                 \
  for (type *iter = pVehicleList, *const end__##iter = pVehicleList + ubNumberOfVehicles; \
       iter != end__##iter; ++iter)                                                       \
    if (!iter->fValid)                                                                    \
      continue;                                                                           \
    else
#define FOR_EACH_VEHICLE(iter) BASE_FOR_EACH_VEHICLE(VEHICLETYPE, iter)
#define CFOR_EACH_VEHICLE(iter) BASE_FOR_EACH_VEHICLE(const VEHICLETYPE, iter)

void SetVehicleValuesIntoSoldierType(SOLDIERTYPE *pVehicle);

// add vehicle to list and return id value
int32_t AddVehicleToList(int16_t sMapX, int16_t sMapY, int16_t sGridNo, uint8_t ubType);

// remove this vehicle from the list
void RemoveVehicleFromList(VEHICLETYPE &);

// clear out the vehicle list
void ClearOutVehicleList();

bool AnyAccessibleVehiclesInSoldiersSector(SOLDIERTYPE const &);

// is this vehicle in the same sector (not between sectors), and accesible
bool IsThisVehicleAccessibleToSoldier(SOLDIERTYPE const &, VEHICLETYPE const &);

// strategic mvt stuff
// move character path to the vehicle
BOOLEAN MoveCharactersPathToVehicle(SOLDIERTYPE *pSoldier);

// Return the vehicle, iff the vehicle ID is valid, NULL otherwise
VEHICLETYPE &GetVehicle(int32_t vehicle_id);

/* Given this grunt, find out if asscoiated vehicle has a mvt group, if so,
 * set this grunts mvt group to the vehicle.  For pathing purposes, will be
 * reset to zero in copying of path */
void SetUpMvtGroupForVehicle(SOLDIERTYPE *);

// find vehicle id of group with this vehicle
VEHICLETYPE &GetVehicleFromMvtGroup(GROUP const &);

// kill everyone in vehicle
BOOLEAN KillAllInVehicle(VEHICLETYPE const &);

// grab number of occupants in vehicles
int32_t GetNumberInVehicle(VEHICLETYPE const &);

// grab # in vehicle skipping EPCs (who aren't allowed to drive :-)
int32_t GetNumberOfNonEPCsInVehicle(int32_t iId);

BOOLEAN ExitVehicle(SOLDIERTYPE *pSoldier);

void VehicleTakeDamage(uint8_t ubID, uint8_t ubReason, int16_t sDamage, int16_t sGridNo,
                       SOLDIERTYPE *att);

// the soldiertype containing this tactical incarnation of this vehicle
SOLDIERTYPE &GetSoldierStructureForVehicle(VEHICLETYPE const &);

// does it need fixing?
bool DoesVehicleNeedAnyRepairs(VEHICLETYPE const &);

// repair the vehicle
int8_t RepairVehicle(VEHICLETYPE const &, int8_t bTotalPts, BOOLEAN *pfNothingToRepair);

// Save all the vehicle information to the saved game file
void SaveVehicleInformationToSaveGameFile(HWFILE);

// Load all the vehicle information From the saved game file
void LoadVehicleInformationFromSavedGameFile(HWFILE, uint32_t uiSavedGameVersion);

// take soldier out of vehicle
BOOLEAN TakeSoldierOutOfVehicle(SOLDIERTYPE *pSoldier);

bool PutSoldierInVehicle(SOLDIERTYPE &, VEHICLETYPE &);

void SetVehicleSectorValues(VEHICLETYPE &, uint8_t x, uint8_t y);

void UpdateAllVehiclePassengersGridNo(SOLDIERTYPE *pSoldier);

void LoadVehicleMovementInfoFromSavedGameFile(HWFILE);
void NewSaveVehicleMovementInfoToSavedGameFile(HWFILE);
void NewLoadVehicleMovementInfoFromSavedGameFile(HWFILE);

BOOLEAN OKUseVehicle(uint8_t ubProfile);

BOOLEAN IsRobotControllerInVehicle(int32_t iId);

void AddVehicleFuelToSave();

bool SoldierMustDriveVehicle(SOLDIERTYPE const &, bool trying_to_travel);

bool IsEnoughSpaceInVehicle(VEHICLETYPE const &);

BOOLEAN IsSoldierInThisVehicleSquad(const SOLDIERTYPE *pSoldier, int8_t bSquadNumber);

SOLDIERTYPE *PickRandomPassengerFromVehicle(SOLDIERTYPE *pSoldier);

bool DoesVehicleGroupHaveAnyPassengers(GROUP const &);

void SetSoldierExitHelicopterInsertionData(SOLDIERTYPE *);

void HandleVehicleMovementSound(const SOLDIERTYPE *, BOOLEAN fOn);

uint8_t GetVehicleArmourType(uint8_t vehicle_id);

uint8_t GetVehicleSeats(VEHICLETYPE const &);

void InitVehicles();

#endif
