#include "Strategic/Strategic.h"

#include "Laptop/Personnel.h"
#include "Macro.h"
#include "SGP/Types.h"
#include "Strategic/Assignments.h"
#include "Strategic/GameClock.h"
#include "Strategic/MapScreen.h"
#include "Tactical/DialogueControl.h"
#include "Tactical/Overhead.h"
#include "Tactical/SoldierProfile.h"
#include "Tactical/Squads.h"
#include "Tactical/TacticalSave.h"
#include "Tactical/Vehicles.h"
#include "TileEngine/IsometricUtils.h"

static void HandleSoldierDeadComments(SOLDIERTYPE const *);

void HandleStrategicDeath(SOLDIERTYPE &s) {
  if (s.bAssignment == VEHICLE && s.iVehicleId != -1) {
    TakeSoldierOutOfVehicle(&s);
  }

  RemoveCharacterFromSquads(&s);
  if (s.bAssignment != ASSIGNMENT_DEAD) {
    SetTimeOfAssignmentChangeForMerc(&s);
  }
  ChangeSoldiersAssignment(&s, ASSIGNMENT_DEAD);

  if (fInMapMode) {
    ReBuildCharactersList();

    HandleSoldierDeadComments(&s);
    AddDeadSoldierToUnLoadedSector(s.sSectorX, s.sSectorY, s.bSectorZ, &s, RandomGridNo(),
                                   ADD_DEAD_SOLDIER_TO_SWEETSPOT);

    fReDrawFace = TRUE;

    StopTimeCompression();
  }
}

static void HandleSoldierDeadComments(SOLDIERTYPE const *const dead) {
  FOR_EACH_IN_TEAM(s, dead->bTeam) {
    if (s->bLife < OKLIFE) continue;

    uint16_t quote_num;
    int8_t const buddy_idx = WhichBuddy(s->ubProfile, dead->ubProfile);
    switch (buddy_idx) {
      case 0:
        quote_num = QUOTE_BUDDY_ONE_KILLED;
        break;
      case 1:
        quote_num = QUOTE_BUDDY_TWO_KILLED;
        break;
      case 2:
        quote_num = QUOTE_LEARNED_TO_LIKE_MERC_KILLED;
        break;

      default:
        continue;
    }
    TacticalCharacterDialogue(s, quote_num);
  }
}
