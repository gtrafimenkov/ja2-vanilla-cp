// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef _ASSIGNMENTS_H
#define _ASSIGNMENTS_H

// header for assignment manipulation/updating for characters

#include "JA2Types.h"
#include "Strategic/StrategicMovement.h"

// this distinguishes whether we're only looking for patients healable THIS HOUR
// (those that have been on their assignment long enough), or those that will be
// healable EVER (regardless of whether they're getting healed during this hour)
#define HEALABLE_EVER 0
#define HEALABLE_THIS_HOUR 1

// merc collapses from fatigue if max breath drops to this.  Can't go any lower!
#define BREATHMAX_ABSOLUTE_MINIMUM 10
#define BREATHMAX_GOTTA_STOP_MOVING 30
#define BREATHMAX_PRETTY_TIRED 50
#define BREATHMAX_CANCEL_COLLAPSE 60
#define BREATHMAX_CANCEL_TIRED 75
#define BREATHMAX_FULLY_RESTED 95

#define VEHICLE_REPAIR_POINTS_DIVISOR 10

// Assignments Defines
enum {
  SQUAD_1 = 0,
  SQUAD_2,
  SQUAD_3,
  SQUAD_4,
  SQUAD_5,
  SQUAD_6,
  SQUAD_7,
  SQUAD_8,
  SQUAD_9,
  SQUAD_10,
  SQUAD_11,
  SQUAD_12,
  SQUAD_13,
  SQUAD_14,
  SQUAD_15,
  SQUAD_16,
  SQUAD_17,
  SQUAD_18,
  SQUAD_19,
  SQUAD_20,
  ON_DUTY,
  DOCTOR,
  PATIENT,
  VEHICLE,
  IN_TRANSIT,
  REPAIR,
  TRAIN_SELF,
  TRAIN_TOWN,
  TRAIN_TEAMMATE,
  TRAIN_BY_OTHER,
  ASSIGNMENT_DEAD,
  ASSIGNMENT_UNCONCIOUS,  // unused
  ASSIGNMENT_POW,
  ASSIGNMENT_HOSPITAL,
  ASSIGNMENT_EMPTY,
};

#define NO_ASSIGNMENT 127  // used when no pSoldier->ubDesiredSquad

// Train stats defines (must match ATTRIB_MENU_ defines, and
// pAttributeMenuStrings )
enum {
  STRENGTH = 0,
  DEXTERITY,
  AGILITY,
  HEALTH,
  MARKSMANSHIP,
  MEDICAL,
  MECHANICAL,
  LEADERSHIP,
  EXPLOSIVE_ASSIGN,
  NUM_TRAINABLE_STATS
  // NOTE: Wisdom isn't trainable!
};

struct TOWN_TRAINER_TYPE {
  SOLDIERTYPE *pSoldier;
  int16_t sTrainingPts;
};

// can character train militia?
BOOLEAN CanCharacterTrainMilitia(const SOLDIERTYPE *s);

// if merc could train militia here, do they have sufficient loyalty?
BOOLEAN
DoesSectorMercIsInHaveSufficientLoyaltyToTrainMilitia(const SOLDIERTYPE *s);

// is the character in transit?
bool IsCharacterInTransit(SOLDIERTYPE const &);

// handler for assignments -- called once per hour via event
void UpdateAssignments();

void MakeSoldiersTacticalAnimationReflectAssignment(SOLDIERTYPE *pSoldier);

// build list of sectors with mercs
void BuildSectorsWithSoldiersList();

// init sectors with soldiers list
void InitSectorsWithSoldiersList();

// is there a soldier in this sector?..only use after
// BuildSectorsWithSoldiersList is called
BOOLEAN IsThereASoldierInThisSector(int16_t sSectorX, int16_t sSectorY, int8_t bSectorZ);

void CheckIfSoldierUnassigned(SOLDIERTYPE *pSoldier);

// figure out the assignment menu pop up box positions
void DetermineBoxPositions();

// set x,y position in tactical
void SetTacticalPopUpAssignmentBoxXY();

// get number of pts that are being used this strategic turn
int16_t GetTownTrainPtsForCharacter(const SOLDIERTYPE *pTrainer, uint16_t *pusMaxPts);

// find number of healing pts
uint16_t CalculateHealingPointsForDoctor(SOLDIERTYPE *pSoldier, uint16_t *pusMaxPts,
                                         BOOLEAN fMakeSureKitIsInHand);

// find number of repair pts repairman has available
uint8_t CalculateRepairPointsForRepairman(SOLDIERTYPE *pSoldier, uint16_t *pusMaxPts,
                                          BOOLEAN fMakeSureKitIsInHand);

// get bonus tarining pts due to an instructor for this student
int16_t GetBonusTrainingPtsDueToInstructor(const SOLDIERTYPE *pInstructor,
                                           const SOLDIERTYPE *pStudent, int8_t bTrainStat,
                                           BOOLEAN fAtGunRange, uint16_t *pusMaxPts);

// get training pts for this soldier
int16_t GetSoldierTrainingPts(const SOLDIERTYPE *s, int8_t bTrainStat, BOOLEAN fAtGunRange,
                              uint16_t *pusMaxPts);

// pts for being a student for this soldier
int16_t GetSoldierStudentPts(const SOLDIERTYPE *s, int8_t bTrainStat, BOOLEAN fAtGunRange,
                             uint16_t *pusMaxPts);

// Handle assignment done
void AssignmentDone(SOLDIERTYPE *pSoldier, BOOLEAN fSayQuote, BOOLEAN fMeToo);

extern PopUpBox *ghAssignmentBox;
extern PopUpBox *ghEpcBox;
extern PopUpBox *ghSquadBox;
extern PopUpBox *ghRepairBox;
extern PopUpBox *ghTrainingBox;
extern PopUpBox *ghAttributeBox;
extern PopUpBox *ghRemoveMercAssignBox;
extern PopUpBox *ghContractBox;
extern PopUpBox *ghMoveBox;

extern BOOLEAN fShownContractMenu;
extern BOOLEAN fShownAssignmentMenu;
extern BOOLEAN fShowRepairMenu;

extern BOOLEAN fFirstClickInAssignmentScreenMask;

extern BOOLEAN gfReEvaluateEveryonesNothingToDo;

void CreateDestroyMouseRegionsForContractMenu();
void HandleShadingOfLinesForAssignmentMenus();
void CreateDestroyScreenMaskForAssignmentAndContractMenus();

void CreateDestroyAssignmentPopUpBoxes();
void SetSoldierAssignmentHospital(SOLDIERTYPE &);
void SetSoldierAssignmentRepair(SOLDIERTYPE &, BOOLEAN sam, BOOLEAN robot, int8_t vehicle_id);

// set merc asleep and awake under the new sleep system implemented June 29,
// 1998 if give warning is false, the function can be used as an internal
// function
BOOLEAN SetMercAwake(SOLDIERTYPE *pSoldier, BOOLEAN fGiveWarning, BOOLEAN fForceHim);
bool SetMercAsleep(SOLDIERTYPE &, bool give_warning);
void PutMercInAsleepState(SOLDIERTYPE &);
BOOLEAN PutMercInAwakeState(SOLDIERTYPE *pSoldier);

// set what time this merc undertook this assignment
void SetTimeOfAssignmentChangeForMerc(SOLDIERTYPE *pSoldier);

// check if any merc in group is too tired to keep moving
BOOLEAN AnyMercInGroupCantContinueMoving(GROUP const &);

// handle selected group of mercs being put to sleep
BOOLEAN HandleSelectedMercsBeingPutAsleep(BOOLEAN fWakeUp, BOOLEAN fDisplayWarning);

// is any one on the team on this assignment?
BOOLEAN IsAnyOneOnPlayersTeamOnThisAssignment(int8_t bAssignment);

// rebuild assignments box
void RebuildAssignmentsBox();

void BandageBleedingDyingPatientsBeingTreated();

void ReEvaluateEveryonesNothingToDo();

// set assignment for list of characters
void SetAssignmentForList(int8_t bAssignment, int8_t bParam);

// function where we actually set someone's assignment so we can trap certain
// situations
void ChangeSoldiersAssignment(SOLDIERTYPE *pSoldier, int8_t bAssignment);

void UnEscortEPC(SOLDIERTYPE *pSoldier);

SOLDIERTYPE *AnyDoctorWhoCanHealThisPatient(SOLDIERTYPE *pPatient, BOOLEAN fThisHour);

void DetermineWhichAssignmentMenusCanBeShown();
void ResumeOldAssignment(SOLDIERTYPE *pSoldier);
bool PlayerSoldierTooTiredToTravel(SOLDIERTYPE &);

void CreateContractBox(const SOLDIERTYPE *s);

// screen mask for pop up menus
void ClearScreenMaskForMapScreenExit();

void CreateMercRemoveAssignBox();

#endif