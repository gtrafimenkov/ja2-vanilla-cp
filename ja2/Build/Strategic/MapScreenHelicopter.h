#ifndef __MAP_SCREEN_HELICOPTER_H
#define __MAP_SCREEN_HELICOPTER_H

#include "JA2Types.h"
#include "SGP/Debug.h"
#include "Strategic/Assignments.h"
#include "Strategic/StrategicMovement.h"
#include "Tactical/SoldierControl.h"
#include "Tactical/Vehicles.h"

// costs of flying through sectors
#define COST_AIRSPACE_SAFE 100
#define COST_AIRSPACE_UNSAFE 1000  // VERY dangerous

#define MIN_PROGRESS_FOR_SKYRIDER_QUOTE_DOING_WELL 25  // scale of 0-100
#define MIN_REGRESS_FOR_SKYRIDER_QUOTE_DOING_BADLY 10  // scale of 0-100

// skyrider quotes
#define OWED_MONEY_TO_SKYRIDER 11
#define MENTION_DRASSEN_SAM_SITE 20
#define SECOND_HALF_OF_MENTION_DRASSEN_SAM_SITE 21
#define SAM_SITE_TAKEN 22
#define SKYRIDER_SAYS_HI 23
#define SPIEL_ABOUT_OTHER_SAM_SITES 24
#define SECOND_HALF_OF_SPIEL_ABOUT_OTHER_SAM_SITES 25

#define SPIEL_ABOUT_ESTONI_AIRSPACE 26
#define CONFIRM_DESTINATION 27
// #define DESTINATION_TOO_FAR 28		// unused
#define ALTERNATE_FUEL_SITE 26
#define ARRIVED_IN_HOSTILE_SECTOR 29
#define BELIEVED_ENEMY_SECTOR 30  // may become unused
#define ARRIVED_IN_NON_HOSTILE_SECTOR 31
#define HOVERING_A_WHILE 32
#define RETURN_TO_BASE 33
#define ENEMIES_SPOTTED_EN_ROUTE_IN_FRIENDLY_SECTOR_A 34
#define ENEMIES_SPOTTED_EN_ROUTE_IN_FRIENDLY_SECTOR_B 35
#define MENTION_HOSPITAL_IN_CAMBRIA 45
#define THINGS_ARE_GOING_BADLY 46
#define THINGS_ARE_GOING_WELL 47
#define CHOPPER_NOT_ACCESSIBLE 48
#define DOESNT_WANT_TO_FLY 49
#define HELI_TOOK_MINOR_DAMAGE 52
#define HELI_TOOK_MAJOR_DAMAGE 53
#define HELI_GOING_DOWN 54

enum {
  DRASSEN_REFUELING_SITE = 0,
  ESTONI_REFUELING_SITE,
  NUMBER_OF_REFUEL_SITES,
};

// the sam site enums
enum {
  SAM_SITE_ONE = 0,  // near Chitzena
  SAM_SITE_TWO,      // near Drassen
  SAM_SITE_THREE,    // near Cambria
  SAM_SITE_FOUR,     // near Meduna
  NUMBER_OF_SAM_SITES,
};

// helicopter vehicle id value
extern int32_t iHelicopterVehicleId;

static inline VEHICLETYPE &GetHelicopter() {
  Assert(0 <= iHelicopterVehicleId && iHelicopterVehicleId < ubNumberOfVehicles);
  VEHICLETYPE &v = pVehicleList[iHelicopterVehicleId];
  Assert(v.fValid);
  return v;
}

static inline bool IsHelicopter(VEHICLETYPE const &v) {
  return VEHICLE2ID(v) == iHelicopterVehicleId;
}

static inline bool InHelicopter(SOLDIERTYPE const &s) {
  return s.bAssignment == VEHICLE && s.iVehicleId == iHelicopterVehicleId;
}

// heli is hovering
extern BOOLEAN fHoveringHelicopter;

// helicopter destroyed
extern BOOLEAN fHelicopterDestroyed;

// is the pilot returning straight to base?
extern BOOLEAN fHeliReturnStraightToBase;

// is the heli in the air?
extern BOOLEAN fHelicopterIsAirBorne;

// total owed to player
// extern int32_t iTotalAccumlatedCostByPlayer;

// time started hovering
extern uint32_t uiStartHoverTime;

// what state is skyrider's dialogue in in?
extern uint32_t guiHelicopterSkyriderTalkState;

// plot for helicopter
extern BOOLEAN fPlotForHelicopter;

// the flags for skyrider events
extern BOOLEAN fShowEstoniRefuelHighLight;
extern BOOLEAN fShowOtherSAMHighLight;
extern BOOLEAN fShowDrassenSAMHighLight;
extern BOOLEAN fShowCambriaHospitalHighLight;

extern int32_t iTotalAccumulatedCostByPlayer;
extern uint32_t guiTimeOfLastSkyriderMonologue;
extern BOOLEAN fSkyRiderSetUp;
extern BOOLEAN fRefuelingSiteAvailable[NUMBER_OF_REFUEL_SITES];

extern uint8_t gubHelicopterHitsTaken;
extern BOOLEAN gfSkyriderSaidCongratsOnTakingSAM;
extern uint8_t gubPlayerProgressSkyriderLastCommentedOn;

BOOLEAN RemoveSoldierFromHelicopter(SOLDIERTYPE *pSoldier);

// have pilot say different stuff
void HelicopterDialogue(uint8_t ubDialogueCondition);

// is the helicopter available for flight?
BOOLEAN CanHelicopterFly();

// is the pilot alive and on our side?
BOOLEAN IsHelicopterPilotAvailable();

// have helicopter take off
void TakeOffHelicopter();

// test whether or not a sector contains a fuel site
bool IsRefuelSiteInSector(int16_t sector);

// update which refueling sites are controlled by player & therefore available
void UpdateRefuelSiteAvailability();

// setup helicopter for player
void SetUpHelicopterForPlayer(int16_t sX, int16_t sY);

// the intended path of the helicopter
int32_t DistanceOfIntendedHelicopterPath();

// handle a little wait for hover
void HandleHeliHoverLong();

// handle a LONG wait in hover mode
void HandleHeliHoverTooLong();

// drop off everyone in helicopter
void DropOffEveryOneInHelicopter();

// handle heli entering this sector
BOOLEAN HandleHeliEnteringSector(int16_t sX, int16_t sY);

// set up helic, if it doesn't have a mvt group
void SetUpHelicopterForMovement();

// number of passengers in helicopter
int32_t GetNumberOfPassengersInHelicopter();

// skyrider talking to player
void SkyRiderTalk(uint16_t usQuoteNum);

// handle animation of sectors for mapscreen
void HandleAnimationOfSectors();

// check and handle skyrider monologue
void CheckAndHandleSkyriderMonologues();

void HandleHelicopterOnGroundGraphic();

void HandleHelicopterOnGroundSkyriderProfile();

// will a sam site under the players control shoot down an airraid?
// BOOLEAN WillAirRaidBeStopped( int16_t sSectorX, int16_t sSectorY );

// is the helicopter capable of taking off for the player?
BOOLEAN CanHelicopterTakeOff();

void InitializeHelicopter();

bool IsSkyriderIsFlyingInSector(int16_t x, int16_t y);

bool IsGroupTheHelicopterGroup(GROUP const &);

int16_t GetNumSafeSectorsInPath();

int16_t GetNumUnSafeSectorsInPath();

bool SoldierAboardAirborneHeli(SOLDIERTYPE const &);

void MoveAllInHelicopterToFootMovementGroup();

void PayOffSkyriderDebtIfAny();

#endif
