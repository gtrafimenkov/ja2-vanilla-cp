#include "Macro.h"
#include "Strategic/Quests.h"
#include "Tactical/LOS.h"
#include "Tactical/OppList.h"
#include "Tactical/SoldierMacros.h"
#include "Tactical/SoldierProfile.h"
#include "TacticalAI/AI.h"
#include "TacticalAI/AIInternals.h"
#include "TacticalAI/NPC.h"
#include "TileEngine/IsometricUtils.h"
#include "TileEngine/RenderFun.h"
#include "Utils/TimerControl.h"

void CallAvailableEnemiesTo(GridNo grid_no) {
  // All enemy teams become aware of a very important "noise" coming from here!
  for (int32_t team = 0; team != LAST_TEAM; ++team) {
    CallAvailableTeamEnemiesTo(grid_no, team);
  }
}

void CallAvailableTeamEnemiesTo(GridNo const grid_no, int8_t const team) {
  if (!IsTeamActive(team)) return;

  // If this team is computer-controlled, and isn't the CIVILIAN "team"
  if (gTacticalStatus.Team[team].bHuman || team == CIV_TEAM) return;

  // Make this team (publicly) aware of the "noise"
  gsPublicNoiseGridno[team] = grid_no;
  gubPublicNoiseVolume[team] = MAX_MISC_NOISE_DURATION;

  // New situation for everyone
  FOR_EACH_IN_TEAM(s, team) {
    if (!s->bInSector || s->bLife < OKLIFE) continue;

    SetNewSituation(s);
    WearGasMaskIfAvailable(s);
  }
}

void CallAvailableKingpinMenTo(int16_t sGridNo) {
  // like call all enemies, but only affects civgroup KINGPIN guys with
  // NO PROFILE

  // All enemy teams become aware of a very important "noise" coming from here!
  // if this team is active
  if (IsTeamActive(CIV_TEAM)) {
    // make this team (publicly) aware of the "noise"
    gsPublicNoiseGridno[CIV_TEAM] = sGridNo;
    gubPublicNoiseVolume[CIV_TEAM] = MAX_MISC_NOISE_DURATION;

    // new situation for everyone...
    FOR_EACH_IN_TEAM(s, CIV_TEAM) {
      if (s->bInSector && s->bLife >= OKLIFE && s->ubCivilianGroup == KINGPIN_CIV_GROUP &&
          s->ubProfile == NO_PROFILE) {
        SetNewSituation(s);
      }
    }
  }
}

void CallEldinTo(int16_t sGridNo) {
  // like call all enemies, but only affects Eldin
  // Eldin becomes aware of a very important "noise" coming from here!
  // So long as he hasn't already heard a noise a sec ago...
  if (IsTeamActive(CIV_TEAM)) {
    // new situation for Eldin
    SOLDIERTYPE *const pSoldier = FindSoldierByProfileID(ELDIN);
    if (pSoldier && pSoldier->bInSector && pSoldier->bLife >= OKLIFE &&
        (pSoldier->bAlertStatus == STATUS_GREEN ||
         pSoldier->ubNoiseVolume < MAX_MISC_NOISE_DURATION / 2)) {
      if (SoldierToLocationLineOfSightTest(pSoldier, sGridNo, (uint8_t)MaxDistanceVisible(),
                                           TRUE)) {
        // sees the player now!
        TriggerNPCWithIHateYouQuote(ELDIN);
        SetNewSituation(pSoldier);
      } else {
        pSoldier->sNoiseGridno = sGridNo;
        pSoldier->ubNoiseVolume = MAX_MISC_NOISE_DURATION;
        pSoldier->bAlertStatus = STATUS_RED;
        if (pSoldier->bAction != AI_ACTION_GET_CLOSER ||
            !CheckFact(FACT_MUSEUM_ALARM_WENT_OFF, 0)) {
          CancelAIAction(pSoldier);
          pSoldier->bNextAction = AI_ACTION_GET_CLOSER;
          pSoldier->usNextActionData = sGridNo;
          RESETTIMECOUNTER(pSoldier->AICounter, 100);
        }
        // otherwise let AI handle this normally
        //				SetNewSituation( pSoldier );
        // reduce any delay to minimal
      }
      SetFactTrue(FACT_MUSEUM_ALARM_WENT_OFF);
    }
  }
}

int16_t MostImportantNoiseHeard(SOLDIERTYPE *pSoldier, int32_t *piRetValue,
                                BOOLEAN *pfClimbingNecessary, BOOLEAN *pfReachable) {
  int8_t *pbPersOL, *pbPublOL;
  int16_t *psLastLoc, *psNoiseGridNo;
  int8_t *pbNoiseLevel;
  int8_t *pbLastLevel;
  uint8_t *pubNoiseVolume;
  int32_t iDistAway;
  int32_t iNoiseValue, iBestValue = -10000;
  int16_t sBestGridNo = NOWHERE;
  int8_t bBestLevel = 0;
  int16_t sClimbingGridNo;
  BOOLEAN fClimbingNecessary = FALSE;

  pubNoiseVolume = &gubPublicNoiseVolume[pSoldier->bTeam];
  psNoiseGridNo = &gsPublicNoiseGridno[pSoldier->bTeam];
  pbNoiseLevel = &gbPublicNoiseLevel[pSoldier->bTeam];

  psLastLoc = gsLastKnownOppLoc[pSoldier->ubID];

  // hang pointers at start of this guy's personal and public opponent opplists
  pbPersOL = pSoldier->bOppList;
  pbPublOL = gbPublicOpplist[pSoldier->bTeam];

  // look through this man's personal & public opplists for opponents heard
  FOR_EACH_MERC(i) {
    const SOLDIERTYPE *const pTemp = *i;

    // if this merc is inactive, at base, on assignment, or dead
    if (!pTemp->bLife) continue;  // next merc

    // if this merc is neutral/on same side, he's not an opponent
    if (CONSIDERED_NEUTRAL(pSoldier, pTemp) || (pSoldier->bSide == pTemp->bSide))
      continue;  // next merc

    pbPersOL = pSoldier->bOppList + pTemp->ubID;
    pbPublOL = gbPublicOpplist[pSoldier->bTeam] + pTemp->ubID;
    psLastLoc = gsLastKnownOppLoc[pSoldier->ubID] + pTemp->ubID;
    pbLastLevel = gbLastKnownOppLevel[pSoldier->ubID] + pTemp->ubID;

    // if this guy's been personally heard within last 3 turns
    if (*pbPersOL < NOT_HEARD_OR_SEEN) {
      // calculate how far this noise was, and its relative "importance"
      iDistAway = SpacesAway(pSoldier->sGridNo, *psLastLoc);
      iNoiseValue = (*pbPersOL) * iDistAway;  // always a negative number!

      if (iNoiseValue > iBestValue) {
        iBestValue = iNoiseValue;
        sBestGridNo = *psLastLoc;
        bBestLevel = *pbLastLevel;
      }
    }

    // if this guy's been publicly heard within last 3 turns
    if (*pbPublOL < NOT_HEARD_OR_SEEN) {
      // calculate how far this noise was, and its relative "importance"
      iDistAway =
          SpacesAway(pSoldier->sGridNo, gsPublicLastKnownOppLoc[pSoldier->bTeam][pTemp->ubID]);
      iNoiseValue = (*pbPublOL) * iDistAway;  // always a negative number!

      if (iNoiseValue > iBestValue) {
        iBestValue = iNoiseValue;
        sBestGridNo = gsPublicLastKnownOppLoc[pSoldier->bTeam][pTemp->ubID];
        bBestLevel = gbPublicLastKnownOppLevel[pSoldier->bTeam][pTemp->ubID];
      }
    }
  }

  // if any "misc. noise" was also heard recently
  if (pSoldier->sNoiseGridno != NOWHERE) {
    if (pSoldier->bNoiseLevel != pSoldier->bLevel ||
        PythSpacesAway(pSoldier->sGridNo, pSoldier->sNoiseGridno) >= 6 ||
        SoldierTo3DLocationLineOfSightTest(pSoldier, pSoldier->sNoiseGridno, pSoldier->bNoiseLevel,
                                           0, (uint8_t)MaxDistanceVisible(), FALSE) == 0) {
      // calculate how far this noise was, and its relative "importance"
      iDistAway = SpacesAway(pSoldier->sGridNo, pSoldier->sNoiseGridno);
      iNoiseValue = ((pSoldier->ubNoiseVolume / 2) - 6) * iDistAway;

      if (iNoiseValue > iBestValue) {
        iBestValue = iNoiseValue;
        sBestGridNo = pSoldier->sNoiseGridno;
        bBestLevel = pSoldier->bNoiseLevel;
      }
    } else {
      // we are there or near
      pSoldier->sNoiseGridno = NOWHERE;  // wipe it out, not useful anymore
      pSoldier->ubNoiseVolume = 0;
    }
  }

  // if any recent PUBLIC "misc. noise" is also known
  if ((pSoldier->bTeam != CIV_TEAM) || (pSoldier->ubCivilianGroup == KINGPIN_CIV_GROUP)) {
    if (*psNoiseGridNo != NOWHERE) {
      // if we are NOT there (at the noise gridno)
      if (*pbNoiseLevel != pSoldier->bLevel ||
          PythSpacesAway(pSoldier->sGridNo, *psNoiseGridNo) >= 6 ||
          SoldierTo3DLocationLineOfSightTest(pSoldier, *psNoiseGridNo, *pbNoiseLevel, 0,
                                             (uint8_t)MaxDistanceVisible(), FALSE) == 0) {
        // calculate how far this noise was, and its relative "importance"
        iDistAway = SpacesAway(pSoldier->sGridNo, *psNoiseGridNo);
        iNoiseValue = ((*pubNoiseVolume / 2) - 6) * iDistAway;

        if (iNoiseValue > iBestValue) {
          iBestValue = iNoiseValue;
          sBestGridNo = *psNoiseGridNo;
          bBestLevel = *pbNoiseLevel;
        }
      }
    }
  }

  if (sBestGridNo != NOWHERE && pfReachable) {
    *pfReachable = TRUE;

    // make civs not walk to noises outside their room if on close
    // patrol/onguard
    if (pSoldier->bOrders <= CLOSEPATROL &&
        (pSoldier->bTeam == CIV_TEAM || pSoldier->ubProfile != NO_PROFILE)) {
      uint8_t const room = GetRoom(pSoldier->usPatrolGrid[0]);
      if (room != NO_ROOM) {
        uint8_t const new_room = GetRoom(pSoldier->usPatrolGrid[0]);
        if (new_room == NO_ROOM || new_room != room) {
          *pfReachable = FALSE;
        }
      }
    }

    if (*pfReachable) {
      // if there is a climb involved then we should store the location
      // of where we have to climb to instead
      sClimbingGridNo =
          GetInterveningClimbingLocation(pSoldier, sBestGridNo, bBestLevel, &fClimbingNecessary);
      if (fClimbingNecessary) {
        if (sClimbingGridNo == NOWHERE) {
          // can't investigate!
          *pfReachable = FALSE;
        } else {
          sBestGridNo = sClimbingGridNo;
          fClimbingNecessary = TRUE;
        }
      } else {
        fClimbingNecessary = FALSE;
      }
    }
  }

  if (piRetValue) {
    *piRetValue = iBestValue;
  }

  if (pfClimbingNecessary) {
    *pfClimbingNecessary = fClimbingNecessary;
  }

#ifdef DEBUGDECISIONS
  if (sBestGridNo != NOWHERE) AINumMessage("MOST IMPORTANT NOISE HEARD FROM GRID #", sBestGridNo);
#endif

  return (sBestGridNo);
}

int16_t WhatIKnowThatPublicDont(SOLDIERTYPE *pSoldier, uint8_t ubInSightOnly) {
  uint8_t ubTotal = 0;
  int8_t *pbPersOL, *pbPublOL;

  // if merc knows of a more important misc. noise than his team does
  if (!(CREATURE_OR_BLOODCAT(pSoldier)) &&
      (pSoldier->ubNoiseVolume > gubPublicNoiseVolume[pSoldier->bTeam])) {
    // the difference in volume is added to the "new info" total
    ubTotal += pSoldier->ubNoiseVolume - gubPublicNoiseVolume[pSoldier->bTeam];
  }

  // hang pointers at start of this guy's personal and public opponent opplists
  pbPersOL = &(pSoldier->bOppList[0]);
  pbPublOL = &(gbPublicOpplist[pSoldier->bTeam][0]);

  FOR_EACH_MERC(i) {
    const SOLDIERTYPE *const pTemp = *i;

    // if this merc is neutral/on same side, he's not an opponent
    if (CONSIDERED_NEUTRAL(pSoldier, pTemp) || (pSoldier->bSide == pTemp->bSide)) {
      continue;  // next merc
    }

    pbPersOL = pSoldier->bOppList + pTemp->ubID;
    pbPublOL = gbPublicOpplist[pSoldier->bTeam] + pTemp->ubID;

    // if we're only interested in guys currently is sight, and he's not
    if (ubInSightOnly) {
      if ((*pbPersOL == SEEN_CURRENTLY) && (*pbPublOL != SEEN_CURRENTLY)) {
        // just count the number of them
        ubTotal++;
      }
    } else {
      // add value of personal knowledge compared to public knowledge to total
      ubTotal += gubKnowledgeValue[*pbPublOL - OLDEST_HEARD_VALUE][*pbPersOL - OLDEST_HEARD_VALUE];
    }
  }

#ifdef DEBUGDECISIONS
  if (ubTotal > 0) {
    AINumMessage("WHAT I KNOW THAT PUBLIC DON'T = ", ubTotal);
  }
#endif

  return (ubTotal);
}
