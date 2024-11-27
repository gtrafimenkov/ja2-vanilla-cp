#include "Tactical/MercHiring.h"

#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "JAScreens.h"
#include "Laptop/History.h"
#include "Laptop/LaptopSave.h"
#include "Laptop/Personnel.h"
#include "Macro.h"
#include "MessageBoxScreen.h"
#include "SGP/Debug.h"
#include "SGP/MouseSystem.h"
#include "SGP/Random.h"
#include "ScreenIDs.h"
#include "Strategic/Assignments.h"
#include "Strategic/AutoResolve.h"
#include "Strategic/GameClock.h"
#include "Strategic/GameEventHook.h"
#include "Strategic/MapScreen.h"
#include "Strategic/MapScreenInterface.h"
#include "Strategic/MapScreenInterfaceBottom.h"
#include "Strategic/MapScreenInterfaceMap.h"
#include "Strategic/MercContract.h"
#include "Strategic/Quests.h"
#include "Strategic/Strategic.h"
#include "Strategic/StrategicMap.h"
#include "Strategic/StrategicMercHandler.h"
#include "Tactical/AnimationData.h"
#include "Tactical/DialogueControl.h"
#include "Tactical/Items.h"
#include "Tactical/Overhead.h"
#include "Tactical/Points.h"
#include "Tactical/SoldierAdd.h"
#include "Tactical/SoldierAni.h"
#include "Tactical/SoldierControl.h"
#include "Tactical/SoldierCreate.h"
#include "Tactical/SoldierProfile.h"
#include "Tactical/Squads.h"
#include "TacticalAI/AI.h"
#include "TileEngine/RenderDirty.h"
#include "TileEngine/RenderFun.h"
#include "TileEngine/SysUtil.h"
#include "TileEngine/WorldDef.h"
#include "Utils/FontControl.h"
#include "Utils/Message.h"
#include "Utils/Text.h"
#include "math.h"

#define MIN_FLIGHT_PREP_TIME 6

extern BOOLEAN gfTacticalDoHeliRun;
extern BOOLEAN gfFirstHeliRun;

// ATE: Globals that dictate where the mercs will land once being hired
// Default to start sector
// Saved in general saved game structure
int16_t g_merc_arrive_sector = START_SECTOR;

int8_t HireMerc(MERC_HIRE_STRUCT &h) {
  ProfileID const pid = h.ubProfileID;
  MERCPROFILESTRUCT &p = GetProfile(pid);

  // If we are to disregard the status of the merc
  {
    switch (p.bMercStatus) {
      case 0:
      case MERC_ANNOYED_BUT_CAN_STILL_CONTACT:
      case MERC_HIRED_BUT_NOT_ARRIVED_YET:
        break;

      default:
        return MERC_HIRE_FAILED;
    }
  }

  if (NumberOfMercsOnPlayerTeam() >= 18) return MERC_HIRE_OVER_20_MERCS_HIRED;

  // ATE: if we are to use landing zone, update to latest value
  // they will be updated again just before arrival...
  if (h.fUseLandingZoneForArrival) {
    h.sSectorX = SECTORX(g_merc_arrive_sector);
    h.sSectorY = SECTORY(g_merc_arrive_sector);
    h.bSectorZ = 0;
  }

  SOLDIERCREATE_STRUCT MercCreateStruct;
  memset(&MercCreateStruct, 0, sizeof(MercCreateStruct));
  MercCreateStruct.ubProfile = pid;
  MercCreateStruct.sSectorX = h.sSectorX;
  MercCreateStruct.sSectorY = h.sSectorY;
  MercCreateStruct.bSectorZ = h.bSectorZ;
  MercCreateStruct.bTeam = OUR_TEAM;
  MercCreateStruct.fCopyProfileItemsOver = h.fCopyProfileItemsOver;
  SOLDIERTYPE *const s = TacticalCreateSoldier(MercCreateStruct);
  if (s == NULL) {
    DebugMsg(TOPIC_JA2, DBG_LEVEL_3, "TacticalCreateSoldier in HireMerc():  Failed to Add Merc");
    return MERC_HIRE_FAILED;
  }

  if (DidGameJustStart()) {
    // OK, CHECK FOR FIRST GUY, GIVE HIM SPECIAL ITEM!
    if (s->ubID == 0) {
      // OK, give this item to our merc!
      OBJECTTYPE o;
      memset(&o, 0, sizeof(o));
      o.usItem = LETTER;
      o.ubNumberOfObjects = 1;
      o.bStatus[0] = 100;
      const BOOLEAN fReturn = AutoPlaceObject(s, &o, FALSE);
      (void)fReturn;
      Assert(fReturn);
    }

    // Set insertion for first time in chopper
    h.ubInsertionCode = INSERTION_CODE_CHOPPER;
  }

  // Record how long the merc will be gone for
  p.bMercStatus = (uint8_t)h.iTotalContractLength;

  // Copy over insertion data
  s->ubStrategicInsertionCode = h.ubInsertionCode;
  s->usStrategicInsertionData = h.usInsertionData;
  // ATE: Copy over value for using landing zone to soldier type
  s->fUseLandingZoneForArrival = h.fUseLandingZoneForArrival;

  // Set assignment
  if (s->bAssignment != IN_TRANSIT) SetTimeOfAssignmentChangeForMerc(s);
  ChangeSoldiersAssignment(s, IN_TRANSIT);

  // Set the contract length
  s->iTotalContractLength = h.iTotalContractLength;

  // Reset the insurance values
  s->iStartOfInsuranceContract = 0;
  s->iTotalLengthOfInsuranceContract = 0;

  // Store arrival time in soldier structure so map screen can display it
  s->uiTimeSoldierWillArrive = h.uiTimeTillMercArrives;

  if (DidGameJustStart()) {
    // Set time of initial merc arrival in minutes
    h.uiTimeTillMercArrives = (STARTING_TIME + FIRST_ARRIVAL_DELAY) / NUM_SEC_IN_MIN;

    // set when the merc's contract is finished
    s->iEndofContractTime =
        GetMidnightOfFutureDayInMinutes(s->iTotalContractLength) + GetHourWhenContractDone(s) * 60;
  } else {
    // set when the merc's contract is finished (+1 cause it takes a day for the
    // merc to arrive)
    s->iEndofContractTime = GetMidnightOfFutureDayInMinutes(1 + s->iTotalContractLength) +
                            GetHourWhenContractDone(s) * 60;
  }

  // Set the time and ID of the last hired merc will arrive
  LaptopSaveInfo.sLastHiredMerc.iIdOfMerc = pid;
  LaptopSaveInfo.sLastHiredMerc.uiArrivalTime = h.uiTimeTillMercArrives;

  // If we are trying to hire a merc that should arrive later, put the merc in
  // the queue
  if (h.uiTimeTillMercArrives != 0) {
    AddStrategicEvent(EVENT_DELAYED_HIRING_OF_MERC, h.uiTimeTillMercArrives, s->ubID);
    // Specify that the merc is hired but has not arrived yet
    p.bMercStatus = MERC_HIRED_BUT_NOT_ARRIVED_YET;
  }

  // Set the type of merc
  if (pid < BIFF) {
    s->ubWhatKindOfMercAmI = MERC_TYPE__AIM_MERC;

    // Determine how much the contract is, and remember what type of contract he
    // got
    switch (h.iTotalContractLength) {
      case 1:
        s->bTypeOfLastContract = CONTRACT_EXTEND_1_DAY;
        s->iTimeCanSignElsewhere = GetWorldTotalMin();
        break;

      case 7:
        s->bTypeOfLastContract = CONTRACT_EXTEND_1_WEEK;
        s->iTimeCanSignElsewhere = GetWorldTotalMin();
        break;

      case 14:
        s->bTypeOfLastContract = CONTRACT_EXTEND_2_WEEK;
        // This fellow needs to stay the whole duration!
        s->iTimeCanSignElsewhere = s->iEndofContractTime;
        break;
    }

    // remember the medical deposit we PAID.  The one in his profile can
    // increase when he levels!
    s->usMedicalDeposit = p.sMedicalDepositAmount;
  } else if (pid <= BUBBA) {
    s->ubWhatKindOfMercAmI = MERC_TYPE__MERC;

    p.iMercMercContractLength = 1;

    // Set starting conditions for the merc
    s->iStartContractTime = GetWorldDay();

    AddHistoryToPlayersLog(HISTORY_HIRED_MERC_FROM_MERC, pid, GetWorldTotalMin(), -1, -1);
  } else if (pid < MIGUEL) {
    s->ubWhatKindOfMercAmI = MERC_TYPE__PLAYER_CHARACTER;
  } else {
    s->ubWhatKindOfMercAmI = MERC_TYPE__NPC;
  }

  // remove the merc from the Personnel screens departed list (if they have
  // never been hired before, its ok to call it)
  RemoveNewlyHiredMercFromPersonnelDepartedList(s->ubProfile);

  gfAtLeastOneMercWasHired = TRUE;
  return MERC_HIRE_OK;
}

static void CheckForValidArrivalSector();

void MercArrivesCallback(SOLDIERTYPE &s) {
  uint32_t uiTimeOfPost;

  if (!DidGameJustStart() &&
      g_merc_arrive_sector == START_SECTOR) { /* Mercs arriving in start sector. This sector has
                                               * been deemed as the always safe sector. Seeing we
                                               * don't support entry into a hostile sector (except
                                               * for the beginning), we will nuke any enemies in
                                               * this sector first. */
    if (gWorldSectorX != SECTORX(START_SECTOR) || gWorldSectorY != SECTORY(START_SECTOR) ||
        gbWorldSectorZ != 0) {
      EliminateAllEnemies(SECTORX(g_merc_arrive_sector), SECTORY(g_merc_arrive_sector));
    }
  }

  // This will update ANY soldiers currently schedules to arrive too
  CheckForValidArrivalSector();

  // stop time compression until player restarts it
  StopTimeCompression();

  MERCPROFILESTRUCT &p = GetProfile(s.ubProfile);

  // add the guy to a squad
  AddCharacterToAnySquad(&s);

  // ATE: Make sure we use global.....
  if (s.fUseLandingZoneForArrival) {
    s.sSectorX = SECTORX(g_merc_arrive_sector);
    s.sSectorY = SECTORY(g_merc_arrive_sector);
    s.bSectorZ = 0;
  }

  // Add merc to sector ( if it's the current one )
  if (gWorldSectorX == s.sSectorX && gWorldSectorY == s.sSectorY && s.bSectorZ == gbWorldSectorZ) {
    // OK, If this sector is currently loaded, and guy does not have CHOPPER
    // insertion code.... ( which means we are at beginning of game if so )
    // Setup chopper....
    if (s.ubStrategicInsertionCode != INSERTION_CODE_CHOPPER &&
        SECTOR(s.sSectorX, s.sSectorY) == START_SECTOR) {
      gfTacticalDoHeliRun = TRUE;

      // OK, If we are in mapscreen, get out...
      if (guiCurrentScreen == MAP_SCREEN) {
        // ATE: Make sure the current one is selected!
        ChangeSelectedMapSector(gWorldSectorX, gWorldSectorY, 0);

        RequestTriggerExitFromMapscreen(MAP_EXIT_TO_TACTICAL);
      }

      s.ubStrategicInsertionCode = INSERTION_CODE_CHOPPER;
    }

    UpdateMercInSector(s, s.sSectorX, s.sSectorY, s.bSectorZ);
  } else {
    // OK, otherwise, set them in north area, so once we load again, they are
    // here.
    s.ubStrategicInsertionCode = INSERTION_CODE_NORTH;
  }

  if (s.ubStrategicInsertionCode != INSERTION_CODE_CHOPPER) {
    ScreenMsg(FONT_MCOLOR_WHITE, MSG_INTERFACE, TacticalStr[MERC_HAS_ARRIVED_STR], s.name);

    // ATE: He's going to say something, now that they've arrived...
    if (!gTacticalStatus.bMercArrivingQuoteBeingUsed && !gfFirstHeliRun) {
      gTacticalStatus.bMercArrivingQuoteBeingUsed = TRUE;

      TacticalCharacterDialogue(&s, QUOTE_MERC_REACHED_DESTINATION);

      class DialogueEventUnsetArrivesFlag : public DialogueEvent {
       public:
        bool Execute() {
          gTacticalStatus.bMercArrivingQuoteBeingUsed = FALSE;
          return false;
        }
      };

      DialogueEvent::Add(new DialogueEventUnsetArrivesFlag());
    }
  }

  // record how long the merc will be gone for
  p.bMercStatus = (uint8_t)s.iTotalContractLength;

  // remember when excatly he ARRIVED in Arulco, in case he gets fired early
  s.uiTimeOfLastContractUpdate = GetWorldTotalMin();

  // set when the merc's contract is finished
  s.iEndofContractTime =
      GetMidnightOfFutureDayInMinutes(s.iTotalContractLength) + GetHourWhenContractDone(&s) * 60;

  // Do initial check for bad items
  if (s.bTeam == OUR_TEAM) {
    // ATE: Try to see if our equipment sucks!
    if (SoldierHasWorseEquipmentThanUsedTo(&s)) {
      // Randomly anytime between 9:00, and 10:00
      uiTimeOfPost = 540 + Random(660);

      if (GetWorldMinutesInDay() < uiTimeOfPost) {
        AddSameDayStrategicEvent(EVENT_MERC_COMPLAIN_EQUIPMENT, uiTimeOfPost, s.ubProfile);
      }
    }
  }

  HandleMercArrivesQuotes(s);

  fTeamPanelDirty = TRUE;

  // if the currently selected sector has no one in it, select this one instead
  if (!CanGoToTacticalInSector(sSelMapX, sSelMapY, (uint8_t)iCurrentMapSectorZ)) {
    ChangeSelectedMapSector(s.sSectorX, s.sSectorY, 0);
  }
}

bool IsMercHireable(MERCPROFILESTRUCT const &p) {
  /* If the merc has an .edt file, is not away on assignment, and isn't already
   * hired (but not arrived yet), he is not dead and he isn't returning home */
  return p.bMercStatus <= 0 && p.bMercStatus != MERC_HAS_NO_TEXT_FILE &&
         p.bMercStatus != MERC_HIRED_BUT_NOT_ARRIVED_YET && p.bMercStatus != MERC_IS_DEAD &&
         p.bMercStatus != MERC_RETURNING_HOME && p.bMercStatus != MERC_WORKING_ELSEWHERE &&
         p.bMercStatus != MERC_FIRED_AS_A_POW && p.uiDayBecomesAvailable == 0;
}

bool IsMercDead(MERCPROFILESTRUCT const &p) { return p.bMercStatus == MERC_IS_DEAD; }

void HandleMercArrivesQuotes(SOLDIERTYPE &s) {
  // If we are approaching with helicopter, don't say any ( yet )
  if (s.ubStrategicInsertionCode == INSERTION_CODE_CHOPPER) return;

  // Player-generated characters issue a comment about arriving in Omerta.
  if (s.ubWhatKindOfMercAmI == MERC_TYPE__PLAYER_CHARACTER &&
      gubQuest[QUEST_DELIVER_LETTER] == QUESTINPROGRESS) {
    TacticalCharacterDialogue(&s, QUOTE_PC_DROPPED_OMERTA);
  }

  // Check to see if anyone hates this merc and will now complain
  FOR_EACH_IN_TEAM(other, OUR_TEAM) {
    if (other->ubWhatKindOfMercAmI != MERC_TYPE__AIM_MERC) continue;

    // hates the merc who has arrived and is going to gripe about it!
    switch (WhichHated(other->ubProfile, s.ubProfile)) {
      case 0:
        TacticalCharacterDialogue(other, QUOTE_HATED_1_ARRIVES);
        break;
      case 1:
        TacticalCharacterDialogue(other, QUOTE_HATED_2_ARRIVES);
        break;
      default:
        break;
    }
  }
}

uint32_t GetMercArrivalTimeOfDay() {
  uint32_t uiCurrHour;
  uint32_t uiMinHour;

  // Pick a time...

  // First get the current time of day.....
  uiCurrHour = GetWorldHour();

  // Subtract the min time for any arrival....
  uiMinHour = uiCurrHour + MIN_FLIGHT_PREP_TIME;

  // OK, first check if we need to advance a whole day's time...
  // See if we have missed the last flight for the day...
  if ((uiCurrHour) > 13)  // ( > 1:00 pm - too bad )
  {
    // 7:30 flight....
    return (GetMidnightOfFutureDayInMinutes(1) + MERC_ARRIVE_TIME_SLOT_1);
  }

  // Well, now we can handle flights all in one day....
  // Find next possible flight
  if (uiMinHour <= 7) {
    return (GetWorldDayInMinutes() + MERC_ARRIVE_TIME_SLOT_1);  // 7:30 am
  } else if (uiMinHour <= 13) {
    return (GetWorldDayInMinutes() + MERC_ARRIVE_TIME_SLOT_2);  // 1:30 pm
  } else {
    return (GetWorldDayInMinutes() + MERC_ARRIVE_TIME_SLOT_3);  // 7:30 pm
  }
}

void UpdateAnyInTransitMercsWithGlobalArrivalSector() {
  FOR_EACH_IN_TEAM(s, OUR_TEAM) {
    if (s->bAssignment == IN_TRANSIT && s->fUseLandingZoneForArrival) {
      s->sSectorX = SECTORX(g_merc_arrive_sector);
      s->sSectorY = SECTORY(g_merc_arrive_sector);
      s->bSectorZ = 0;
    }
  }
}

static int16_t StrategicPythSpacesAway(int16_t sOrigin, int16_t sDest) {
  int16_t sRows, sCols, sResult;

  sRows = abs((sOrigin / MAP_WORLD_X) - (sDest / MAP_WORLD_X));
  sCols = abs((sOrigin % MAP_WORLD_X) - (sDest % MAP_WORLD_X));

  // apply Pythagoras's theorem for right-handed triangle:
  // dist^2 = rows^2 + cols^2, so use the square root to get the distance
  sResult = (int16_t)sqrt((double)((sRows * sRows) + (sCols * sCols)));

  return (sResult);
}

// ATE: This function will check if the current arrival sector
// is valid
// if there are enemies present, it's invalid
// if so, search around for nearest non-occupied sector.
static void CheckForValidArrivalSector() {
  int16_t sTop, sBottom;
  int16_t sLeft, sRight;
  int16_t cnt1, cnt2;
  uint8_t ubRadius = 4;
  int32_t leftmost;
  int16_t sSectorGridNo, sSectorGridNo2;
  int32_t uiRange, uiLowestRange = 999999;
  BOOLEAN fFound = FALSE;
  wchar_t sString[1024];
  wchar_t zShortTownIDString1[50];
  wchar_t zShortTownIDString2[50];

  sSectorGridNo = SECTOR_INFO_TO_STRATEGIC_INDEX(g_merc_arrive_sector);

  // Check if valid...
  if (!StrategicMap[sSectorGridNo].fEnemyControlled) {
    return;
  }

  GetShortSectorString(SECTORX(g_merc_arrive_sector), SECTORY(g_merc_arrive_sector),
                       zShortTownIDString1, lengthof(zShortTownIDString1));

  // If here - we need to do a search!
  sTop = ubRadius;
  sBottom = -ubRadius;
  sLeft = -ubRadius;
  sRight = ubRadius;

  int16_t sGoodX = 0;  // XXX HACK000E
  int16_t sGoodY = 0;  // XXX HACK000E
  for (cnt1 = sBottom; cnt1 <= sTop; cnt1++) {
    leftmost = ((sSectorGridNo + (MAP_WORLD_X * cnt1)) / MAP_WORLD_X) * MAP_WORLD_X;

    for (cnt2 = sLeft; cnt2 <= sRight; cnt2++) {
      sSectorGridNo2 = sSectorGridNo + (MAP_WORLD_X * cnt1) + cnt2;

      if (sSectorGridNo2 >= 1 && sSectorGridNo2 < ((MAP_WORLD_X - 1) * (MAP_WORLD_X - 1)) &&
          sSectorGridNo2 >= leftmost && sSectorGridNo2 < (leftmost + MAP_WORLD_X)) {
        if (!StrategicMap[sSectorGridNo2].fEnemyControlled &&
            !StrategicMap[sSectorGridNo2].fEnemyAirControlled) {
          uiRange = StrategicPythSpacesAway(sSectorGridNo2, sSectorGridNo);

          if (uiRange < uiLowestRange) {
            sGoodY = cnt1;
            sGoodX = cnt2;
            uiLowestRange = uiRange;
            fFound = TRUE;
          }
        }
      }
    }
  }

  if (fFound) {
    g_merc_arrive_sector =
        SECTOR(SECTORX(g_merc_arrive_sector) + sGoodX, SECTORY(g_merc_arrive_sector) + sGoodY);

    UpdateAnyInTransitMercsWithGlobalArrivalSector();

    GetShortSectorString(SECTORX(g_merc_arrive_sector), SECTORY(g_merc_arrive_sector),
                         zShortTownIDString2, lengthof(zShortTownIDString2));

    swprintf(sString, lengthof(sString), str_arrival_rerouted, zShortTownIDString2,
             zShortTownIDString1);

    DoScreenIndependantMessageBox(sString, MSG_BOX_FLAG_OK, NULL);
  }
}
