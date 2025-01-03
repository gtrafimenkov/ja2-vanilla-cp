// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Strategic/StrategicTurns.h"

#include <stdlib.h>

#include "JAScreens.h"
#include "ScreenIDs.h"
#include "Strategic/Assignments.h"
#include "Strategic/GameClock.h"
#include "Tactical/AnimationData.h"
#include "Tactical/Overhead.h"
#include "Tactical/RTTimeDefines.h"
#include "Tactical/RottingCorpses.h"
#include "Tactical/SoldierAdd.h"
#include "Tactical/SoldierCreate.h"
#include "Tactical/TacticalTurns.h"
#include "TileEngine/Environment.h"
#include "TileEngine/RenderDirty.h"
#include "TileEngine/WorldDef.h"
#include "Utils/TimerControl.h"

#define NUM_SEC_PER_STRATEGIC_TURN (NUM_SEC_IN_MIN * 15)  // Every fifteen minutes

static uint32_t guiLastTacticalRealTime = 0;

void StrategicTurnsNewGame() {
  // Sync game start time
  SyncStrategicTurnTimes();
}

void SyncStrategicTurnTimes() { guiLastTacticalRealTime = GetJA2Clock(); }

void HandleStrategicTurn() {
  uint32_t uiTime;
  uint32_t uiCheckTime;  // XXX HACK000E

  // OK, DO THIS CHECK EVERY ONCE AND A WHILE...
  if (COUNTERDONE(STRATEGIC_OVERHEAD)) {
    RESETCOUNTER(STRATEGIC_OVERHEAD);

    // if the game is paused, or we're in mapscreen and time is not being
    // compressed
    if (GamePaused() || ((guiCurrentScreen == MAP_SCREEN) && !IsTimeBeingCompressed())) {
      // don't do any of this
      return;
    }

    // Kris -- What to do?
    if (giTimeCompressMode == NOT_USING_TIME_COMPRESSION) {
      SetGameTimeCompressionLevel(TIME_COMPRESS_X1);
    }

    uiTime = GetJA2Clock();

    // Do not handle turns update if in turnbased combat
    if ((gTacticalStatus.uiFlags & TURNBASED) && (gTacticalStatus.uiFlags & INCOMBAT)) {
      guiLastTacticalRealTime = uiTime;
    } else {
      if (giTimeCompressMode == TIME_COMPRESS_X1 || giTimeCompressMode == 0) {
        uiCheckTime = NUM_REAL_SEC_PER_TACTICAL_TURN;
      } else {
        // OK, if we have compressed time...., adjust our check value to be
        // faster....
        if (giTimeCompressSpeeds[giTimeCompressMode] > 0) {
          uiCheckTime = NUM_REAL_SEC_PER_TACTICAL_TURN / (giTimeCompressSpeeds[giTimeCompressMode] *
                                                          RT_COMPRESSION_TACTICAL_TURN_MODIFIER);
        } else {
          abort();  // XXX HACK000E
        }
      }

      if ((uiTime - guiLastTacticalRealTime) > uiCheckTime) {
        HandleTacticalEndTurn();

        guiLastTacticalRealTime = uiTime;
      }
    }
  }
}

void HandleStrategicTurnImplicationsOfExitingCombatMode() {
  SyncStrategicTurnTimes();
  HandleTacticalEndTurn();
}
