// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#include "Strategic/GameClock.h"

#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "FadeScreen.h"
#include "GameScreen.h"
#include "JAScreens.h"
#include "Local.h"
#include "Macro.h"
#include "SGP/ButtonSystem.h"
#include "SGP/Debug.h"
#include "SGP/FileMan.h"
#include "SGP/Font.h"
#include "SGP/VSurface.h"
#include "SGP/Video.h"
#include "ScreenIDs.h"
#include "Strategic/Assignments.h"
#include "Strategic/GameEvents.h"
#include "Strategic/MapScreen.h"
#include "Strategic/MapScreenInterface.h"
#include "Strategic/MapScreenInterfaceBottom.h"
#include "Strategic/MapScreenInterfaceMap.h"
#include "Strategic/PreBattleInterface.h"
#include "Tactical/MapInformation.h"
#include "Tactical/Overhead.h"
#include "TileEngine/Environment.h"
#include "TileEngine/Lighting.h"
#include "TileEngine/RenderDirty.h"
#include "TileEngine/RenderWorld.h"
#include "Utils/EventPump.h"
#include "Utils/FontControl.h"
#include "Utils/MercTextBox.h"
#include "Utils/Message.h"
#include "Utils/Text.h"
#include "Utils/TimerControl.h"

// #define DEBUG_GAME_CLOCK

extern BOOLEAN gfFadeOut;

// is the clock pause region created currently?
static BOOLEAN fClockMouseRegionCreated = FALSE;

static BOOLEAN fTimeCompressHasOccured = FALSE;

// This value represents the time that the sector was loaded.  If you are in
// sector A9, and leave the game clock at that moment will get saved into the
// temp file associated with it.  The next time you enter A9, this value will
// contain that time.  Used for scheduling purposes.
uint32_t guiTimeCurrentSectorWasLastLoaded = 0;

// did we JUST finish up a game pause by the player
BOOLEAN gfJustFinishedAPause = FALSE;

// clock mouse region
static MOUSE_REGION gClockMouseRegion;
static MOUSE_REGION gClockScreenMaskMouseRegion;

#define SECONDS_PER_COMPRESSION 1  // 1/2 minute passes every 1 second of real time

#define CLOCK_X 554
#define CLOCK_Y (SCREEN_HEIGHT - 23)
#define CLOCK_HEIGHT 13
#define CLOCK_WIDTH 66
#define CLOCK_FONT COMPFONT

// These contain all of the information about the game time, rate of time, etc.
// All of these get saved and loaded.
int32_t giTimeCompressMode = TIME_COMPRESS_X0;
static uint8_t gubClockResolution = 1;
BOOLEAN gfGamePaused = TRUE;
BOOLEAN gfTimeInterrupt = FALSE;
static BOOLEAN gfTimeInterruptPause = FALSE;
static BOOLEAN fSuperCompression = FALSE;
uint32_t guiGameClock = STARTING_TIME;
static uint32_t guiPreviousGameClock = 0;  // used only for error-checking purposes
static uint32_t guiGameSecondsPerRealSecond;
static uint32_t guiTimesThisSecondProcessed = 0;
static MercPopUpBox *g_paused_popup_box;
uint32_t guiDay;
uint32_t guiHour;
uint32_t guiMin;
wchar_t gswzWorldTimeStr[20];
int32_t giTimeCompressSpeeds[NUM_TIME_COMPRESS_SPEEDS] = {0, 1, 5 * 60, 30 * 60, 60 * 60};
static uint16_t usPausedActualWidth;
static uint16_t usPausedActualHeight;
uint32_t guiTimeOfLastEventQuery = 0;
BOOLEAN gfLockPauseState = FALSE;
BOOLEAN gfPauseDueToPlayerGamePause = FALSE;
BOOLEAN gfResetAllPlayerKnowsEnemiesFlags = FALSE;
static BOOLEAN gfTimeCompressionOn = FALSE;
uint32_t guiLockPauseStateLastReasonId = 0;
//***When adding new saved time variables, make sure you remove the appropriate
// amount from the paddingbytes and
//   more IMPORTANTLY, add appropriate code in Save/LoadGameClock()!
#define TIME_PADDINGBYTES 20

extern uint32_t guiEnvTime;
extern uint32_t guiEnvDay;

void InitNewGameClock() {
  guiGameClock = STARTING_TIME;
  guiPreviousGameClock = STARTING_TIME;
  guiDay = (guiGameClock / NUM_SEC_IN_DAY);
  guiHour = (guiGameClock - (guiDay * NUM_SEC_IN_DAY)) / NUM_SEC_IN_HOUR;
  guiMin =
      (guiGameClock - ((guiDay * NUM_SEC_IN_DAY) + (guiHour * NUM_SEC_IN_HOUR))) / NUM_SEC_IN_MIN;
  swprintf(WORLDTIMESTR, lengthof(WORLDTIMESTR), L"%ls %d, %02d:%02d", pDayStrings, guiDay, guiHour,
           guiMin);
  guiTimeCurrentSectorWasLastLoaded = 0;
  guiGameSecondsPerRealSecond = 0;
  gubClockResolution = 1;
}

uint32_t GetWorldTotalMin() { return (guiGameClock / NUM_SEC_IN_MIN); }

uint32_t GetWorldTotalSeconds() { return (guiGameClock); }

uint32_t GetWorldHour() { return (guiHour); }

uint32_t GetWorldMinutesInDay() { return ((guiHour * 60) + guiMin); }

uint32_t GetWorldDay() { return (guiDay); }

uint32_t GetWorldDayInSeconds() { return (guiDay * NUM_SEC_IN_DAY); }

uint32_t GetWorldDayInMinutes() { return ((guiDay * NUM_SEC_IN_DAY) / NUM_SEC_IN_MIN); }

uint32_t GetFutureDayInMinutes(uint32_t uiDay) {
  return ((uiDay * NUM_SEC_IN_DAY) / NUM_SEC_IN_MIN);
}

// this function returns the amount of minutes there has been from start of game
// to midnight of the uiDay.
uint32_t GetMidnightOfFutureDayInMinutes(uint32_t uiDay) {
  return (GetWorldTotalMin() + (uiDay * 1440) - GetWorldMinutesInDay());
}

static void AdvanceClock(uint8_t ubWarpCode);

// Not to be used too often by things other than internally
void WarpGameTime(uint32_t uiAdjustment, uint8_t ubWarpCode) {
  uint32_t uiSaveTimeRate;
  uiSaveTimeRate = guiGameSecondsPerRealSecond;
  guiGameSecondsPerRealSecond = uiAdjustment;
  AdvanceClock(ubWarpCode);
  guiGameSecondsPerRealSecond = uiSaveTimeRate;
}

static void AdvanceClock(uint8_t ubWarpCode) {
  if (ubWarpCode != WARPTIME_NO_PROCESSING_OF_EVENTS) {
    guiTimeOfLastEventQuery = guiGameClock;
    // First of all, events are posted for movements, pending attacks, equipment
    // arrivals, etc.  This time adjustment using time compression can possibly
    // pass one or more events in a single pass.  So, this list is looked at and
    // processed in sequential order, until the uiAdjustment is fully applied.
    if (GameEventsPending(guiGameSecondsPerRealSecond)) {
      // If a special event, justifying the cancellation of time compression is
      // reached, the adjustment will be shortened to the time of that event, and
      // will stop processing events, otherwise, all of the events in the time
      // slice will be processed.  The time is adjusted internally as events are
      // processed.
      ProcessPendingGameEvents(guiGameSecondsPerRealSecond, ubWarpCode);
    } else {
      // Adjust the game clock now.
      guiGameClock += guiGameSecondsPerRealSecond;
    }
  } else {
    guiGameClock += guiGameSecondsPerRealSecond;
  }

  if (guiGameClock < guiPreviousGameClock) {
    AssertMsg(FALSE, String("AdvanceClock: TIME FLOWING BACKWARDS!!! "
                            "guiPreviousGameClock %d, now %d",
                            guiPreviousGameClock, guiGameClock));

    // fix it if assertions are disabled
    guiGameClock = guiPreviousGameClock;
  }

  // store previous game clock value (for error-checking purposes only)
  guiPreviousGameClock = guiGameClock;

  // Calculate the day, hour, and minutes.
  guiDay = (guiGameClock / NUM_SEC_IN_DAY);
  guiHour = (guiGameClock - (guiDay * NUM_SEC_IN_DAY)) / NUM_SEC_IN_HOUR;
  guiMin =
      (guiGameClock - ((guiDay * NUM_SEC_IN_DAY) + (guiHour * NUM_SEC_IN_HOUR))) / NUM_SEC_IN_MIN;

  swprintf(WORLDTIMESTR, lengthof(WORLDTIMESTR), L"%ls %d, %02d:%02d", gpGameClockString, guiDay,
           guiHour, guiMin);

  if (gfResetAllPlayerKnowsEnemiesFlags && !gTacticalStatus.fEnemyInSector) {
    ClearAnySectorsFlashingNumberOfEnemies();

    gfResetAllPlayerKnowsEnemiesFlags = FALSE;
  }

  ForecastDayEvents();
}

void AdvanceToNextDay() {
  int32_t uiDiff;
  uint32_t uiTomorrowTimeInSec;

  uiTomorrowTimeInSec = (guiDay + 1) * NUM_SEC_IN_DAY + 8 * NUM_SEC_IN_HOUR + 15 * NUM_SEC_IN_MIN;
  uiDiff = uiTomorrowTimeInSec - guiGameClock;
  WarpGameTime(uiDiff, WARPTIME_PROCESS_EVENTS_NORMALLY);

  ForecastDayEvents();
}

// set the flag that time compress has occured
void SetFactTimeCompressHasOccured() { fTimeCompressHasOccured = TRUE; }

// reset fact the time compress has occured
void ResetTimeCompressHasOccured() { fTimeCompressHasOccured = FALSE; }

// has time compress occured?
BOOLEAN HasTimeCompressOccured() { return (fTimeCompressHasOccured); }

void RenderClock() {
  // Are we in combat?
  uint8_t const foreground =
      gTacticalStatus.uiFlags & INCOMBAT ? FONT_FCOLOR_NICERED : FONT_LTGREEN;
  SetFontAttributes(CLOCK_FONT, foreground);

  // Erase first!
  int16_t x = CLOCK_X;
  int16_t y = CLOCK_Y;
  RestoreExternBackgroundRect(x, y, CLOCK_WIDTH, CLOCK_HEIGHT);

  const wchar_t *const str = (gfPauseDueToPlayerGamePause ? pPausedGameText[0] : WORLDTIMESTR);
  FindFontCenterCoordinates(x, y, CLOCK_WIDTH, CLOCK_HEIGHT, str, CLOCK_FONT, &x, &y);
  MPrint(x, y, str);
}

static void ToggleSuperCompression() {
  static uint32_t uiOldTimeCompressMode = 0;

  // Display message
  if (gTacticalStatus.uiFlags & INCOMBAT) {
    // ScreenMsg( MSG_FONT_YELLOW, MSG_INTERFACE, L"Cannot toggle compression in
    // Combat Mode."  );
    return;
  }

  fSuperCompression = !fSuperCompression;

  if (fSuperCompression) {
    uiOldTimeCompressMode = giTimeCompressMode;
    giTimeCompressMode = TIME_SUPER_COMPRESS;
    guiGameSecondsPerRealSecond =
        giTimeCompressSpeeds[giTimeCompressMode] * SECONDS_PER_COMPRESSION;

    // ScreenMsg( MSG_FONT_YELLOW, MSG_INTERFACE, L"Time compression ON."  );
  } else {
    giTimeCompressMode = uiOldTimeCompressMode;
    guiGameSecondsPerRealSecond =
        giTimeCompressSpeeds[giTimeCompressMode] * SECONDS_PER_COMPRESSION;

    // ScreenMsg( MSG_FONT_YELLOW, MSG_INTERFACE, L"Time compression OFF."  );
  }
}

bool DidGameJustStart() { return gTacticalStatus.fDidGameJustStart; }

static void SetClockResolutionToCompressMode(int32_t iCompressMode);

void StopTimeCompression() {
  if (gfTimeCompressionOn) {
    // change the clock resolution to no time passage, but don't actually change
    // the compress mode (remember it)
    SetClockResolutionToCompressMode(TIME_COMPRESS_X0);
  }
}

void StartTimeCompression() {
  if (!gfTimeCompressionOn) {
    if (GamePaused()) {
      // first have to be allowed to unpause the game
      UnPauseGame();

      // if we couldn't, ignore this request
      if (GamePaused()) {
        return;
      }
    }

    // check that we can start compressing
    if (!AllowedToTimeCompress()) {
      // not allowed to compress time
      TellPlayerWhyHeCantCompressTime();
      return;
    }

    // if no compression mode is set, increase it first
    if (giTimeCompressMode <= TIME_COMPRESS_X1) {
      IncreaseGameTimeCompressionRate();
    }

    // change clock resolution to the current compression mode
    SetClockResolutionToCompressMode(giTimeCompressMode);

    // if it's the first time we're doing this since entering map screen (which
    // reset the flag)
    if (!HasTimeCompressOccured()) {
      // set fact that we have compressed time during this map screen session
      SetFactTimeCompressHasOccured();

      ClearTacticalStuffDueToTimeCompression();
    }
  }
}

// returns FALSE if time isn't currently being compressed for ANY reason
// (various pauses, etc.)
BOOLEAN IsTimeBeingCompressed() {
  if (!gfTimeCompressionOn || (giTimeCompressMode == TIME_COMPRESS_X0) || gfGamePaused)
    return (FALSE);
  else
    return (TRUE);
}

// returns TRUE if the player currently doesn't want time to be compressing
BOOLEAN IsTimeCompressionOn() { return (gfTimeCompressionOn); }

void IncreaseGameTimeCompressionRate() {
  // if not already at maximum time compression rate
  if (giTimeCompressMode < TIME_COMPRESS_60MINS) {
    // check that we can
    if (!AllowedToTimeCompress()) {
      // not allowed to compress time
      TellPlayerWhyHeCantCompressTime();
      return;
    }

    giTimeCompressMode++;

    // in map screen, we wanna have to skip over x1 compression and go straight
    // to 5x
    if ((guiCurrentScreen == MAP_SCREEN) && (giTimeCompressMode == TIME_COMPRESS_X1)) {
      giTimeCompressMode++;
    }

    SetClockResolutionToCompressMode(giTimeCompressMode);
  }
}

void DecreaseGameTimeCompressionRate() {
  // if not already at minimum time compression rate
  if (giTimeCompressMode > TIME_COMPRESS_X0) {
    // check that we can
    if (!AllowedToTimeCompress()) {
      // not allowed to compress time
      TellPlayerWhyHeCantCompressTime();
      return;
    }

    giTimeCompressMode--;

    // in map screen, we wanna have to skip over x1 compression and go straight
    // to 5x
    if ((guiCurrentScreen == MAP_SCREEN) && (giTimeCompressMode == TIME_COMPRESS_X1)) {
      giTimeCompressMode--;
    }

    SetClockResolutionToCompressMode(giTimeCompressMode);
  }
}

void SetGameTimeCompressionLevel(uint32_t uiCompressionRate) {
  Assert(uiCompressionRate < NUM_TIME_COMPRESS_SPEEDS);

  if (guiCurrentScreen == GAME_SCREEN) {
    if (uiCompressionRate != TIME_COMPRESS_X1) {
      uiCompressionRate = TIME_COMPRESS_X1;
    }
  }

  if (guiCurrentScreen == MAP_SCREEN) {
    if (uiCompressionRate == TIME_COMPRESS_X1) {
      uiCompressionRate = TIME_COMPRESS_X0;
    }
  }

  // if we're attempting time compression
  if (uiCompressionRate >= TIME_COMPRESS_5MINS) {
    // check that we can
    if (!AllowedToTimeCompress()) {
      // not allowed to compress time
      TellPlayerWhyHeCantCompressTime();
      return;
    }
  }

  giTimeCompressMode = uiCompressionRate;
  SetClockResolutionToCompressMode(giTimeCompressMode);
}

static void SetClockResolutionPerSecond(uint8_t ubNumTimesPerSecond);

static void SetClockResolutionToCompressMode(int32_t iCompressMode) {
  guiGameSecondsPerRealSecond = giTimeCompressSpeeds[iCompressMode] * SECONDS_PER_COMPRESSION;

  // ok this is a bit confusing, but for time compression (e.g. 30x60) we want
  // updates 30x per second, but for standard unpaused time, like in tactical,
  // we want 1x per second
  if (guiGameSecondsPerRealSecond == 0) {
    SetClockResolutionPerSecond(0);
  } else {
    SetClockResolutionPerSecond(
        (uint8_t)std::max((uint8_t)1, (uint8_t)(guiGameSecondsPerRealSecond / 60)));
  }

  // if the compress mode is X0 or X1
  if (iCompressMode <= TIME_COMPRESS_X1) {
    gfTimeCompressionOn = FALSE;
  } else {
    gfTimeCompressionOn = TRUE;

    // handle the player just starting a game
    HandleTimeCompressWithTeamJackedInAndGearedToGo();
  }

  fMapScreenBottomDirty = TRUE;
}

void SetGameHoursPerSecond(uint32_t uiGameHoursPerSecond) {
  giTimeCompressMode = NOT_USING_TIME_COMPRESSION;
  guiGameSecondsPerRealSecond = uiGameHoursPerSecond * 3600;
  if (uiGameHoursPerSecond == 1) {
    SetClockResolutionPerSecond(60);
  } else {
    SetClockResolutionPerSecond(59);
  }
}

void SetGameMinutesPerSecond(uint32_t uiGameMinutesPerSecond) {
  giTimeCompressMode = NOT_USING_TIME_COMPRESSION;
  guiGameSecondsPerRealSecond = uiGameMinutesPerSecond * 60;
  SetClockResolutionPerSecond((uint8_t)uiGameMinutesPerSecond);
}

// call this to prevent player from changing the time compression state via the
// interface

void LockPauseState(LockPauseReason const uiUniqueReasonId) {
  gfLockPauseState = TRUE;

  // if adding a new call, please choose a new uiUniqueReasonId, this helps
  // track down the cause when it's left locked Highest # used was 21 on Feb 15
  // '99.
  guiLockPauseStateLastReasonId = uiUniqueReasonId;
}

// call this to allow player to change the time compression state via the
// interface once again
void UnLockPauseState() { gfLockPauseState = FALSE; }

// tells you whether the player is currently locked out from messing with the
// time compression state
BOOLEAN PauseStateLocked() { return gfLockPauseState; }

void PauseGame() {
  // always allow pausing, even if "locked".  Locking applies only to trying to
  // compress time, not to pausing it
  if (!gfGamePaused) {
    gfGamePaused = TRUE;
    fMapScreenBottomDirty = TRUE;
  }
}

void UnPauseGame() {
  // if we're paused
  if (gfGamePaused) {
    // ignore request if locked
    if (gfLockPauseState) {
      ScreenMsg(FONT_ORANGE, MSG_TESTVERSION,
                L"Call to UnPauseGame() while Pause State is LOCKED! AM-4");
      return;
    }

    gfGamePaused = FALSE;
    fMapScreenBottomDirty = TRUE;
  }
}

BOOLEAN GamePaused() { return gfGamePaused; }

// ONLY APPLICABLE INSIDE EVENT CALLBACKS!
void InterruptTime() { gfTimeInterrupt = TRUE; }

void PauseTimeForInterupt() { gfTimeInterruptPause = TRUE; }

// USING CLOCK RESOLUTION
// Note, that changing the clock resolution doesn't effect the amount of game
// time that passes per real second, but how many times per second the clock is
// updated.  This rate will break up the actual time slices per second into
// smaller chunks.  This is useful for animating strategic movement under fast
// time compression, so objects don't warp around.
static void SetClockResolutionToDefault() { gubClockResolution = 1; }

// Valid range is 0 - 60 times per second.
static void SetClockResolutionPerSecond(uint8_t ubNumTimesPerSecond) {
  ubNumTimesPerSecond = (uint8_t)(std::max((uint8_t)0, std::min((uint8_t)60, ubNumTimesPerSecond)));
  gubClockResolution = ubNumTimesPerSecond;
}

static void CreateDestroyScreenMaskForPauseGame();

// There are two factors that influence the flow of time in the game.
//-Speed:  The speed is the amount of game time passes per real second of time.
// The higher this
//         value, the faster the game time flows.
//-Resolution:  The higher the resolution, the more often per second the clock
// is actually updated. 				 This value doesn't affect how much game
// time passes per
// real second, but allows for 				 a more accurate representation of faster
// time flows.
void UpdateClock() {
  uint32_t uiNewTime;
  uint32_t uiThousandthsOfThisSecondProcessed;
  uint32_t uiTimeSlice;
  uint32_t uiNewTimeProcessed;
  uint32_t uiAmountToAdvanceTime;
  static uint8_t ubLastResolution = 1;
  static uint32_t uiLastSecondTime = 0;
  static uint32_t uiLastTimeProcessed = 0;
#ifdef DEBUG_GAME_CLOCK
  uint32_t uiOrigNewTime;
  uint32_t uiOrigLastSecondTime;
  uint32_t uiOrigThousandthsOfThisSecondProcessed;
  uint8_t ubOrigClockResolution;
  uint32_t uiOrigTimesThisSecondProcessed;
  uint8_t ubOrigLastResolution;
#endif
  // check game state for pause screen masks
  CreateDestroyScreenMaskForPauseGame();

  if (guiCurrentScreen != GAME_SCREEN && guiCurrentScreen != MAP_SCREEN &&
      guiCurrentScreen != GAME_SCREEN) {
    uiLastSecondTime = GetJA2Clock();
    gfTimeInterruptPause = FALSE;
    return;
  }

  if (gfGamePaused || gfTimeInterruptPause || (gubClockResolution == 0) ||
      !guiGameSecondsPerRealSecond || ARE_IN_FADE_IN() || gfFadeOut) {
    uiLastSecondTime = GetJA2Clock();
    gfTimeInterruptPause = FALSE;
    return;
  }

  if ((gTacticalStatus.uiFlags & TURNBASED && gTacticalStatus.uiFlags & INCOMBAT))
    return;  // time is currently stopped!

  uiNewTime = GetJA2Clock();

#ifdef DEBUG_GAME_CLOCK
  uiOrigNewTime = uiNewTime;
  uiOrigLastSecondTime = uiLastSecondTime;
  uiOrigThousandthsOfThisSecondProcessed = uiThousandthsOfThisSecondProcessed;
  ubOrigClockResolution = gubClockResolution;
  uiOrigTimesThisSecondProcessed = guiTimesThisSecondProcessed;
  ubOrigLastResolution = ubLastResolution;
#endif

  // Because we debug so much, breakpoints tend to break the game, and cause
  // unnecessary headaches. This line ensures that no more than 1 real-second
  // passes between frames.  This otherwise has no effect on anything else.
  uiLastSecondTime = std::max(uiNewTime - 1000, uiLastSecondTime);

  // 1000's of a second difference since last second.
  uiThousandthsOfThisSecondProcessed = uiNewTime - uiLastSecondTime;

  if (uiThousandthsOfThisSecondProcessed >= 1000 && gubClockResolution == 1) {
    uiLastSecondTime = uiNewTime;
    guiTimesThisSecondProcessed = uiLastTimeProcessed = 0;
    AdvanceClock(WARPTIME_PROCESS_EVENTS_NORMALLY);
  } else if (gubClockResolution > 1) {
    if (gubClockResolution != ubLastResolution) {
      // guiTimesThisSecondProcessed = guiTimesThisSecondProcessed *
      // ubLastResolution / gubClockResolution % gubClockResolution;
      guiTimesThisSecondProcessed =
          guiTimesThisSecondProcessed * gubClockResolution / ubLastResolution;
      uiLastTimeProcessed = uiLastTimeProcessed * gubClockResolution / ubLastResolution;
      ubLastResolution = gubClockResolution;
    }
    uiTimeSlice = 1000000 / gubClockResolution;
    if (uiThousandthsOfThisSecondProcessed >=
        uiTimeSlice * (guiTimesThisSecondProcessed + 1) / 1000) {
      guiTimesThisSecondProcessed = uiThousandthsOfThisSecondProcessed * 1000 / uiTimeSlice;
      uiNewTimeProcessed =
          guiGameSecondsPerRealSecond * guiTimesThisSecondProcessed / gubClockResolution;

      uiNewTimeProcessed = std::max(uiNewTimeProcessed, uiLastTimeProcessed);

      uiAmountToAdvanceTime = uiNewTimeProcessed - uiLastTimeProcessed;

#ifdef DEBUG_GAME_CLOCK
      if (uiAmountToAdvanceTime > 0x80000000 ||
          guiGameClock + uiAmountToAdvanceTime < guiPreviousGameClock) {
        uiNewTimeProcessed = uiNewTimeProcessed;
      }
#endif

      WarpGameTime(uiNewTimeProcessed - uiLastTimeProcessed, WARPTIME_PROCESS_EVENTS_NORMALLY);
      if (uiNewTimeProcessed < guiGameSecondsPerRealSecond) {  // Processed the same real second
        uiLastTimeProcessed = uiNewTimeProcessed;
      } else {  // We have moved into a new real second.
        uiLastTimeProcessed = uiNewTimeProcessed % guiGameSecondsPerRealSecond;
        if (gubClockResolution > 0) {
          guiTimesThisSecondProcessed %= gubClockResolution;
        } else {
          // this branch occurs whenever an event during WarpGameTime stops time
          // compression!
          guiTimesThisSecondProcessed = 0;
        }
        uiLastSecondTime = uiNewTime;
      }
    }
  }
}

void SaveGameClock(HWFILE const hFile, BOOLEAN const fGamePaused, BOOLEAN const fLockPauseState) {
  FileWrite(hFile, &giTimeCompressMode, sizeof(int32_t));
  FileWrite(hFile, &gubClockResolution, sizeof(uint8_t));
  FileWrite(hFile, &fGamePaused, sizeof(BOOLEAN));
  FileWrite(hFile, &gfTimeInterrupt, sizeof(BOOLEAN));
  FileWrite(hFile, &fSuperCompression, sizeof(BOOLEAN));
  FileWrite(hFile, &guiGameClock, sizeof(uint32_t));
  FileWrite(hFile, &guiGameSecondsPerRealSecond, sizeof(uint32_t));
  FileWrite(hFile, &ubAmbientLightLevel, sizeof(uint8_t));
  FileWrite(hFile, &guiEnvTime, sizeof(uint32_t));
  FileWrite(hFile, &guiEnvDay, sizeof(uint32_t));
  FileWrite(hFile, &gubEnvLightValue, sizeof(uint8_t));
  FileWrite(hFile, &guiTimeOfLastEventQuery, sizeof(uint32_t));
  FileWrite(hFile, &fLockPauseState, sizeof(BOOLEAN));
  FileWrite(hFile, &gfPauseDueToPlayerGamePause, sizeof(BOOLEAN));
  FileWrite(hFile, &gfResetAllPlayerKnowsEnemiesFlags, sizeof(BOOLEAN));
  FileWrite(hFile, &gfTimeCompressionOn, sizeof(BOOLEAN));
  FileWrite(hFile, &guiPreviousGameClock, sizeof(uint32_t));
  FileWrite(hFile, &guiLockPauseStateLastReasonId, sizeof(uint32_t));

  FileSeek(hFile, TIME_PADDINGBYTES, FILE_SEEK_FROM_CURRENT);
}

void LoadGameClock(HWFILE const hFile) {
  FileRead(hFile, &giTimeCompressMode, sizeof(int32_t));
  FileRead(hFile, &gubClockResolution, sizeof(uint8_t));
  FileRead(hFile, &gfGamePaused, sizeof(BOOLEAN));
  FileRead(hFile, &gfTimeInterrupt, sizeof(BOOLEAN));
  FileRead(hFile, &fSuperCompression, sizeof(BOOLEAN));
  FileRead(hFile, &guiGameClock, sizeof(uint32_t));
  FileRead(hFile, &guiGameSecondsPerRealSecond, sizeof(uint32_t));
  FileRead(hFile, &ubAmbientLightLevel, sizeof(uint8_t));
  FileRead(hFile, &guiEnvTime, sizeof(uint32_t));
  FileRead(hFile, &guiEnvDay, sizeof(uint32_t));
  FileRead(hFile, &gubEnvLightValue, sizeof(uint8_t));
  FileRead(hFile, &guiTimeOfLastEventQuery, sizeof(uint32_t));
  FileRead(hFile, &gfLockPauseState, sizeof(BOOLEAN));
  FileRead(hFile, &gfPauseDueToPlayerGamePause, sizeof(BOOLEAN));
  FileRead(hFile, &gfResetAllPlayerKnowsEnemiesFlags, sizeof(BOOLEAN));
  FileRead(hFile, &gfTimeCompressionOn, sizeof(BOOLEAN));
  FileRead(hFile, &guiPreviousGameClock, sizeof(uint32_t));
  FileRead(hFile, &guiLockPauseStateLastReasonId, sizeof(uint32_t));

  FileSeek(hFile, TIME_PADDINGBYTES, FILE_SEEK_FROM_CURRENT);

  // Update the game clock
  guiDay = (guiGameClock / NUM_SEC_IN_DAY);
  guiHour = (guiGameClock - (guiDay * NUM_SEC_IN_DAY)) / NUM_SEC_IN_HOUR;
  guiMin =
      (guiGameClock - ((guiDay * NUM_SEC_IN_DAY) + (guiHour * NUM_SEC_IN_HOUR))) / NUM_SEC_IN_MIN;

  swprintf(WORLDTIMESTR, lengthof(WORLDTIMESTR), L"%ls %d, %02d:%02d", pDayStrings, guiDay, guiHour,
           guiMin);

  if (!gfBasement && !gfCaves) gfDoLighting = TRUE;
}

static void PauseOfClockBtnCallback(MOUSE_REGION *pRegion, int32_t iReason);

void CreateMouseRegionForPauseOfClock() {
  if (!fClockMouseRegionCreated) {
    // create a mouse region for pausing of game clock
    MSYS_DefineRegion(&gClockMouseRegion, CLOCK_X, CLOCK_Y, CLOCK_X + CLOCK_WIDTH,
                      CLOCK_Y + CLOCK_HEIGHT, MSYS_PRIORITY_HIGHEST, MSYS_NO_CURSOR,
                      MSYS_NO_CALLBACK, PauseOfClockBtnCallback);

    fClockMouseRegionCreated = TRUE;

    wchar_t const *const help = gfGamePaused ? pPausedGameText[1] : pPausedGameText[2];
    gClockMouseRegion.SetFastHelpText(help);
  }
}

void RemoveMouseRegionForPauseOfClock() {
  // remove pause region
  if (fClockMouseRegionCreated) {
    MSYS_RemoveRegion(&gClockMouseRegion);
    fClockMouseRegionCreated = FALSE;
  }
}

static void PauseOfClockBtnCallback(MOUSE_REGION *pRegion, int32_t iReason) {
  if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    HandlePlayerPauseUnPauseOfGame();
  }
}

void HandlePlayerPauseUnPauseOfGame() {
  if (gTacticalStatus.uiFlags & ENGAGED_IN_CONV) {
    return;
  }

  // check if the game is paused BY THE PLAYER or not and reverse
  if (gfGamePaused && gfPauseDueToPlayerGamePause) {
    // If in game screen...
    if (guiCurrentScreen == GAME_SCREEN) {
      if (giTimeCompressMode == TIME_COMPRESS_X0) {
        giTimeCompressMode++;
      }

      // ATE: re-render
      SetRenderFlags(RENDER_FLAG_FULL);
    }

    UnPauseGame();
    PauseTime(FALSE);
    gfIgnoreScrolling = FALSE;
    gfPauseDueToPlayerGamePause = FALSE;
  } else {
    // pause game
    PauseGame();
    PauseTime(TRUE);
    gfIgnoreScrolling = TRUE;
    gfPauseDueToPlayerGamePause = TRUE;
  }
}

static void ScreenMaskForGamePauseBtnCallBack(MOUSE_REGION *pRegion, int32_t iReason);

static void CreateDestroyScreenMaskForPauseGame() {
  static BOOLEAN fCreated = FALSE;

  if ((!fClockMouseRegionCreated || !gfGamePaused || !gfPauseDueToPlayerGamePause) && fCreated) {
    fCreated = FALSE;
    MSYS_RemoveRegion(&gClockScreenMaskMouseRegion);
    RemoveMercPopupBox(g_paused_popup_box);
    g_paused_popup_box = 0;
    SetRenderFlags(RENDER_FLAG_FULL);
    fTeamPanelDirty = TRUE;
    fMapPanelDirty = TRUE;
    fMapScreenBottomDirty = TRUE;
    gfJustFinishedAPause = TRUE;
    MarkButtonsDirty();
    SetRenderFlags(RENDER_FLAG_FULL);
  } else if (gfPauseDueToPlayerGamePause && !fCreated) {
    // create a mouse region for pausing of game clock
    MSYS_DefineRegion(&gClockScreenMaskMouseRegion, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                      MSYS_PRIORITY_HIGHEST, 0, MSYS_NO_CALLBACK,
                      ScreenMaskForGamePauseBtnCallBack);
    fCreated = TRUE;

    // re create region on top of this
    RemoveMouseRegionForPauseOfClock();
    CreateMouseRegionForPauseOfClock();

    gClockMouseRegion.SetFastHelpText(pPausedGameText[1]);

    fMapScreenBottomDirty = TRUE;

    // UnMarkButtonsDirty( );

    // now create the pop up box to say the game is paused
    g_paused_popup_box = PrepareMercPopupBox(0, BASIC_MERC_POPUP_BACKGROUND,
                                             BASIC_MERC_POPUP_BORDER, pPausedGameText[0], 300, 0, 0,
                                             0, &usPausedActualWidth, &usPausedActualHeight);
  }
}

static void ScreenMaskForGamePauseBtnCallBack(MOUSE_REGION *pRegion, int32_t iReason) {
  if (iReason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
    // unpause the game
    HandlePlayerPauseUnPauseOfGame();
  }
}

void RenderPausedGameBox() {
  if (gfPauseDueToPlayerGamePause && gfGamePaused && g_paused_popup_box) {
    const int32_t x = (SCREEN_WIDTH - usPausedActualWidth) / 2;
    const int32_t y = 200 - usPausedActualHeight / 2;
    RenderMercPopUpBox(g_paused_popup_box, x, y, FRAME_BUFFER);
    InvalidateRegion(x, y, x + usPausedActualWidth, y + usPausedActualHeight);
  }

  // reset we've just finished a pause by the player
  gfJustFinishedAPause = FALSE;
}

BOOLEAN DayTime() {  // between 7AM and 9PM
  return (guiHour >= 7 && guiHour < 21);
}

BOOLEAN NightTime() {  // before 7AM or after 9PM
  return (guiHour < 7 || guiHour >= 21);
}

void ClearTacticalStuffDueToTimeCompression() {
  // is this test the right thing?  ARM
  if (!fInMapMode) return;  // XXX necessary?

  // clear tactical event queue
  ClearEventQueue();

  // clear tactical message queue
  ClearTacticalMessageQueue();

  if (gfWorldLoaded) {
    // clear tactical actions
    CencelAllActionsForTimeCompression();
  }
}
